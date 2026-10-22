import sys
import math
#import igrf_utils as iut    # IGRF13
import numpy as np
from scipy import integrate
from scipy import interpolate
#from scipy.special import ellipeinc
#from scipy.optimize import brentq
from mypylib import myigrf14 
import pandas as pd
import time 
import logging

efundamental=1.602176634e-19
cc = 299792458 # in m/s

def init():
    """init function"""
    logging.basicConfig(format='%(asctime)s --- %(levelname)s-%(processName)s:%(message)s', level=logging.INFO)

def PitchAngle(momentun,Magnet):
    # return an 2*n array for [cos(pitch angle); sin(pitch angle)]
    if momentun.ndim==1:
        mm=np.linalg.norm(momentun)
        bb=np.linalg.norm(Magnet)
        Magnet=np.reshape(Magnet,(3,))
        dot_products = np.dot(momentun, Magnet)
    else:
        mm=np.linalg.norm(momentun,axis=1)
        bb=np.linalg.norm(Magnet,axis=1)
        dot_products = np.einsum('ij,ij->i', momentun, Magnet)

    cospA=dot_products/(mm*bb)
    #print(cospA)
    sinpA=np.sqrt((1-cospA**2))
    
    return np.vstack((cospA,sinpA))

def BMirror(pAagle, Blocal, ):
    cospA,sinpA=pAagle
    Bm = Blocal/(sinpA**2)
    return Bm

def myxyz2polar(xyz):   #直角坐标转化为球坐标
    x,y,z = xyz
    r=math.sqrt(x*x+y*y+z*z)
    theta=math.acos(z/r)
    phi=math.atan2(y,x)
    return np.asarray([r,theta,phi])

def mypolar2xyz(polar): #球坐标转化为直角坐标
    r,theta,phi = polar
    x=r*math.sin(theta)*math.cos(phi)
    y=r*math.sin(theta)*math.sin(phi)
    z=r*math.cos(theta)
    return np.asarray([x,y,z])
'''
def XVDinkm(t,XV,qcharge,Ekg):  #数值求解常微分方程组的导函数数组定义
    x, y, z, vx, vy, vz = XV    #坐标和速度
    r,theta,phi=myxyz2polar(np.asarray([x,y,z]))
    colat=math.degrees(theta)   #球坐标的theta以度表示
    lon=math.degrees(phi)       #球坐标的phi以度表示
    Br,Bt,Bp=iut.synth_values(coeffs.T, r, colat, lon, igrf.parameters['nmax']) # 输出值约为3.e4
    Bx=Br*math.sin(theta)*math.cos(phi)+Bt*math.cos(theta)*math.cos(phi)-Bp*math.sin(theta)*math.sin(phi)
    By=Br*math.sin(theta)*math.sin(phi)+Bt*math.cos(theta)*math.sin(phi)+Bp*math.sin(theta)*math.cos(phi)
    Bz=Br*math.cos(theta)-Bt*math.sin(theta)    # B的各分量转化为直角坐标
    #print(math.sqrt(vx*vx+vy*vy+vz*vz)/299792.458) #监视速度大小是否不变
    ax=(vy*Bz-vz*By)*qcharge/Ekg*1e-9    # Relativistic Lorentz force, (gamma*m)^{-1}->sqrt(1-v**2)
    ay=(vz*Bx-vx*Bz)*qcharge/Ekg*1e-9    # B in nT，地球磁场大小约3.e-5T
    az=(vx*By-vy*Bx)*qcharge/Ekg*1e-9    # huor
    return [vx, vy, vz, ax, ay, az]
'''
def XVDinkm(t,XV,qcharge,Ekg):  #数值求解常微分方程组的导函数数组定义
    return myigrf14.rhs_particle(t,XV,qcharge,Ekg,2025)

def rLow(t,XV,qcharge,Ekg):     # 积分integrate.solve_ivp的内置events判别，是否撞上稠密大气
    x, y, z, vx, vy, vz = XV
    r,theta,phi=myxyz2polar(np.asarray([x,y,z]))    # 可以再定义一个函数只计算r以加速
    return r-90-6371

rLow.terminal = True            # 若撞上稠密大气则积分可以终止
def rHigh(t,XV,qcharge,Ekg):    # 积分integrate.solve_ivp的内置events判别，是否已逃离地磁场
    x, y, z, vx, vy, vz = XV
    r,theta,phi=myxyz2polar(np.asarray([x,y,z]))    # 可以再定义一个函数只计算r以加速
    return r-1e5

rHigh.terminal = True           # 若已逃离地磁场则积分可以终止

def rDetector(t,XV,qcharge,Ekg):
    x, y, z, vx, vy, vz = XV
    r,theta,phi=myxyz2polar(np.asarray([x,y,z]))
    return r-400-6371

rDetector.terminal = False
rDetector.direction = 0

def complex_rLow(t,XV,qcharge,Ekg): 
    # 积分integrate.solve_ivp的内置events判别，是否撞上稠密大气
    # 但是加入更加复杂的判断。因为很多时候在50km到100km这段距离粒子不会和大气反应，便重新返回太空
    # 对这种情况应该让他继续跑而不是启动GEANT4来计算
    # 所以我的思路是对这个粒子求磁镜像点。
    # 判断磁镜像点的位置是不是在大气层里
    x, y, z, vx, vy, vz = XV
    r,theta,phi=myxyz2polar(np.asarray([x,y,z]))
    
    # 检查粒子是否进入了指定高度
    # 为了检测“进入”这个事件，我们定义一个函数，当粒子在圆外时为正，圆内时为负。
    # 这样当它从外部进入内部时，函数值会从正变负，触发事件。
    event_value = r-100-6371
    
    # --- 在这里实现你的复杂判断逻辑 ---
    # 如果粒子进入了指定高度，进行额外判断
    if event_value < 0:
        # 磁场
        Bx,By,Bz,B=myigrf14.igrf_wrapper(np.rad2deg(theta), np.rad2deg(phi),r,2025)
        # 镜像点磁场
        PAagle=PitchAngle(np.array([vx, vy, vz]),np.array([Bx,By,Bz]))
        Bm = BMirror(PAagle,B) 
        
        if Bm > B*1.02 :
            # 满足停止条件！返回负值，表示事件发生
            return -1.0
        else:
            # 不满足停止条件！返回正值，表示事件不发生
            return 1.0
            
    # 默认情况：返回正常的事件函数值
    return event_value

complex_rLow.terminal = True 
complex_rLow.direction = -1 #表示反生从正到负的变化

''' #这个是用mps.process执行的代码
def main(Xinit, monento, otherpar,i,queue):
    mMeV, charge = otherpar
    mkg=mMeV/cc/cc*1e6*efundamental
    qcharge=charge*efundamental
    mGeV=mkg*cc**2/efundamental/1e9
    pGeV=np.linalg.norm(monento)/1e3 
    EGeV=math.sqrt(pGeV*pGeV+mGeV*mGeV)
    Ekg=EGeV*1e9*efundamental/(cc**2)
    velosity=cc*monento/1e6/EGeV           # 相对论性的速度v=beta=动量/能量，距离单位km，时间单位s
    param=(qcharge,Ekg)
    tspan=(0.0,100.0)
    time0=time.time()
    XVinit=np.append(Xinit,velosity)
    sol=integrate.solve_ivp(XVDinkm, tspan, XVinit, method='RK45', events=[rLow, rHigh, rDetector], args=param)
    print("Sol function finished! Now saving data.")
    str1="../result/TraceData/TraceTest"+str(i)+".csv"
    r=np.linalg.norm(sol.y[0:3],axis=0)
    array=np.vstack((sol.y[0:3],r))
    pd.DataFrame(array.T).to_csv(str1, header=False,  index=False)
    if len(sol.y_events[0])==0:
        queue.put((i, np.array([]), sol.y_events[2]))
    else :
        queue.put((i, sol.y_events[0], sol.y_events[2]))
'''    

def main(Xinit, monento, otherpar,i,n,thread):
    mMeV, charge = otherpar
    # mkg=mMeV/cc/cc*1e6*efundamental
    qcharge=charge*efundamental
    mGeV=mMeV/1e3
    pGeV=np.linalg.norm(monento)/1e3 # monento unit in MeV
    EGeV=math.sqrt(pGeV*pGeV+mGeV*mGeV)
    Ekg=EGeV*1e9*efundamental/(cc**2)
    
    rigidity = pGeV / abs(charge) if charge != 0 else pGeV  # GV
    
    if rigidity > 50:  # 高能粒子，轨迹接近直线
        mt = 'DOP853'  # 高阶方法，大步长
        tspan = (0, 10)   # 减少积分时间
    elif rigidity > 0.1:   # 中等能量
        mt = 'RK45'
        tspan = (0.0,100.0)
    else:                 # 低能粒子，回旋运动
        mt = 'LSODA'      # 自动切换刚性问题
        tspan = (0.0,100.0)
        
    velosity=cc*monento/1e6/EGeV           # 相对论性的速度v=beta=动量/能量，距离单位km，时间单位s
    param=(qcharge,Ekg)
    #tspan=(0.0,100.0)
    # time0=time.time()
    XVinit=np.append(Xinit,velosity)
    #可以考虑在这个玩意里面用归一化的无量纲常数，这样应该可以减小误差
    sol=integrate.solve_ivp(XVDinkm, tspan, XVinit, method=mt, events=[rLow, rHigh, rDetector], args=param)
    # sol=integrate.solve_ivp(XVDinkm, tspan, XVinit, method=mt, events=[complex_rLow, rHigh, rDetector], args=param)
    print("Sol function finished!")
    #str1="../result/TraceData/TraceTest"+str(i)+"_"+n+"_t"+str(thread)+".csv"
    #r=np.linalg.norm(sol.y[0:3],axis=0)
    #array=np.vstack((sol.y[0:3],r))
    #pd.DataFrame(array.T).to_csv(str1, header=False,  index=False)
    if len(sol.y_events[0])==0:
        return (i, np.array([]),0, sol.y_events[2],sol.t_events[2])
    else :
        return (i, sol.y_events[0],sol.t_events[0], sol.y_events[2],sol.t_events[2])
