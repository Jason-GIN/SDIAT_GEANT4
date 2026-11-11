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
import random
from particle import Particle
from particletools.tables import PYTHIAParticleData
from pyFANG import TFANG

efundamental=1.602176634e-19
cc = 299792458 # in m/s

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

def sample_decay_length(lambda_mfp: float) -> float:
    """
    根据指数分布抽样一个衰变长度。
    参数
    ----
    lambda_mfp : float
        平均自由程 (mean free path)，即 gamma*beta*c*tau。
    返回
    ----
    L : float
        本次抽样的实际衰变长度。
    """
    u = np.random.random()        # U ~ Uniform[0, 1)
    L = -lambda_mfp * np.log(u)
    return L
'''
def XVDinkm(t,XV,qcharge,dfp):  #数值求解常微分方程组的导函数数组定义
    x, y, z, vx, vy, vz, s = XV
    sOt=np.sqrt(vx**2+vy**2+vz**2)
    # r,theta, phi=RotationModel.xyz2rthetaphi(x,y,z)
    # Bx,By,Bz,B=myigrf14.igrf_wrapper(np.rad2deg(theta), np.rad2deg(phi),r,2025)
    # return myigrf14.rhs_particle(t,XV,qcharge,Ekg,2025)
    return [vx, vy, vz, 0, 0, 0, sOt]
'''
def XVDinkm(t,XV,dfp,qcharge,Ekg):  #数值求解常微分方程组的导函数数组定义
    # x, y, z, vx, vy, vz, s = XV
    # sOt=np.sqrt(vx**2+vy**2+vz**2)
    # r,theta, phi=RotationModel.xyz2rthetaphi(x,y,z)
    # Bx,By,Bz,B=myigrf14.igrf_wrapper(np.rad2deg(theta), np.rad2deg(phi),r,2025)
    # return myigrf14.rhs_particle(t,XV,qcharge,Ekg,2025)
    return myigrf14.rhs_dparticle(t,XV,qcharge,Ekg,2025)

def rLow(t,XV,dfp,qcharge,Ekg):     # 积分integrate.solve_ivp的内置events判别，是否撞上稠密大气
    x, y, z, vx, vy, vz, s = XV
    r,theta,phi=myxyz2polar(np.asarray([x,y,z]))    # 可以再定义一个函数只计算r以加速
    return r-90-6371

rLow.max_events = 1             # 若撞上稠密大气则积分可以终止

def sVmfp(t,XV,dfp,qcharge,Ekg):     # 积分integrate.solve_ivp的内置events判别，是否撞上稠密大气
    x, y, z, vx, vy, vz, s = XV
    return s-dfp

sVmfp.terminal = True            
# sVmfp.max_events = 1               # 这个是更新的写法，但是有个bug，触发次数必须大于给定的次数才停，而最小赋值为1。对于只会发生一次的事件无法停止。

def rHigh(t,XV,dfp,qcharge,Ekg):    # 积分integrate.solve_ivp的内置events判别，是否已逃离地磁场
    x, y, z, vx, vy, vz, s = XV
    r,theta,phi=myxyz2polar(np.asarray([x,y,z]))    # 可以再定义一个函数只计算r以加速
    return r-1e5

# rHigh.terminal = True           # 若已逃离地磁场则积分可以终止
rHigh.max_events = 1              # 这个是更新的写法   # type: ignore

def rDetector(t,XV,dfp,qcharge,Ekg):
    x, y, z, vx, vy, vz, s = XV
    r,theta,phi=myxyz2polar(np.asarray([x,y,z]))
    return r-400-6371

# rDetector.terminal = False
rDetector.max_events = np.inf  # type: ignore
rDetector.direction = 0        # type: ignore

def datasplit(i,result,result_t):
    # 输入的是解方程事件数组的前三项
    # 返回元组，元素分别是（击中90km的位置，击中时间，击中400km的位置，击中时间）
    if len(result[0])==0:
        return (i, np.array([]),0, result[2][:,0:6], result_t[2])
    else :
        return (i, result[0][0:6],result_t[0], result[2][:,0:6], result_t[2])

def paramseting(charge,rigidity,parm):
    B,beta=parm
    if charge ==0:
        mt = 'RK45'
        tspan = (0.0,100.0)
        maxstep=np.inf
    else:
        if rigidity > 50:  # 高能粒子，轨迹接近直线
            mt = 'DOP853'  # 高阶方法，大步长
            tspan = (0, 10)   # 减少积分时间
            maxstep=np.inf
        elif rigidity > 20:   # 中等能量
            mt = 'RK45'
            tspan = (0.0,100.0)
            maxstep=np.inf
        elif rigidity > 10:
            mt = 'RK45'
            tspan = (0.0,100.0)
            maxstep=69.9*rigidity/(B*beta)/20   #时间单位是s， 所以有很多单位的转换 ,69.9实际上是1e18/cc/cc*2*np.pi的近似值
            #最大步长取回旋周期的1/(20)  ，
            # 一开始太天真，光顾着考虑电子，电子在5 MeV的时候 v/c->1，直接就把beta当1忽略了。
            # 但是质子，或者说质量大的粒子，在5 MeV时，beta会很小，原来的公式会导致，步长过低。
        else:                 # 低能粒子，回旋运动
            mt = 'LSODA'      # 自动切换刚性问题
            tspan = (0.0,100.0)
            maxstep=69.9*rigidity/(B*beta)/20
    return mt,tspan,maxstep

def DecayProducts(pid,momentum):
    particle_data = PYTHIAParticleData()
 
    mGeV = particle_data.mass(pid)
    
    # 获取结构化的衰变道列表：[(分支比, [末态粒子名...]), ...]
    channels = particle_data.decay_channels(pid)

    # 提取分支比作为权重，以及对应的末态粒子列表
    weights = [ch[0] for ch in channels]
    daughter_names = [ch[1] for ch in channels]

    # 按分支比加权随机选择一条衰变道
    chosen_daughters = random.choices(daughter_names, weights=weights, k=1)[0]
    print(f"随机选中的衰变道: eta -> {chosen_daughters}")

    # --- 2. 为选中的末态粒子获取质量 ---
    # 注意：这里需要处理粒子名称到质量查询的映射.
    daughter_masses = [particle_data.mass(name) for name in chosen_daughters]
    print(f"末态粒子质量 (GeV): {daughter_masses}")

    # --- 3. 将信息送入 pyFANG 生成运动学 ---
    # 注意这个东西的四动量是 px, py, pz, m0（静止质量），不是一般的说的总能量
    parent_momentum = np.append(momentum,mGeV)

    gen = TFANG(seed=42)
    gen.SetDecay(parent_momentum, np.array(daughter_masses))
    gen.Generate()

    # 获取生成的事例
    # weight = gen.GetWeight(0)
    daughters_4mom = np.array(gen.GetDecays(0))   # 形状: [n_daughters, 4]，[px, py, pz, m]
    px, py, pz, mass = daughters_4mom.T           # 拆列
    energy = np.sqrt(px**2 + py**2 + pz**2 + mass**2)
    Ek=energy-mass
    charges=np.array([particle_data.charge(name) for name in chosen_daughters])
    daughters_array= np.column_stack([px, py, pz, Ek, daughter_names[0], charges, mass])
    
    return daughters_array 
  
def main(Xinit, monento, otherpar,i):
    mMeV, charge, B, PID = otherpar # 这个地方B的单位是nT
    # mkg=mMeV/cc/cc*1e6*efundamental
    qcharge=charge*efundamental
    mGeV=mMeV/1e3
    pGeV=np.linalg.norm(monento)/1e3 # monento unit in MeV
    EGeV=math.sqrt(pGeV*pGeV+mGeV*mGeV)
    Ekg=EGeV*1e9*efundamental/(cc**2)
    velosity=cc*monento/1e6/EGeV           # 相对论性的速度v=beta=动量/能量，距离单位km，时间单位s
    beta=pGeV/EGeV 
    # print(beta)
    gamma=1/np.sqrt(1-beta**2)         # 相对论洛伦兹因子 同样距离单位km，时间单位s         
    dtime = Particle.from_pdgid(PID).lifetime*1e-9  # in s
    mfp=gamma*beta*cc*dtime/1e3       # 单位是km
    # print(mfp)
    dfp=sample_decay_length(mfp)
    # print(dfp)
    
    rigidity = pGeV / abs(charge) if charge != 0 else pGeV  # GV
    param=(dfp,qcharge,Ekg)

    XVinit=np.hstack((Xinit,velosity,np.array([0.0,])))
    
    mt,tspan,maxstep=paramseting(charge,rigidity,[B,beta])
        
    t_eval_step=np.linspace(tspan[0], tspan[1], 2000) 
    sol=integrate.solve_ivp(XVDinkm, tspan, XVinit, method=mt, events=[rLow, rHigh, rDetector, sVmfp], 
                            args=param,max_step=maxstep,t_eval=t_eval_step)
    print("D Sol function finished!")
    res=datasplit(i,sol.y_events[0:3],sol.t_events[0:3])
    if len(sol.y_events[3])==0:
        return (res,[])
    else:
        # 这里应该接一个生成衰变产物具体分布的代码然后进行新的一轮tracing
        # 可以根据返回数组的大小判断是返回原粒子还是衰变。
        x,y,z,vx,vy,vz,s=sol.y_events[3][0]
        loc=np.array([x,y,z])
        v=np.linalg.norm([vx,vy,vz])
        vbeta=v*1e3/cc
        if abs(vbeta-beta)>1e-6:
            print("Error!!! Energy is not conserved!!","vbeta: ",vbeta, "beta: ",beta)
            return 0
        newmome=pGeV*np.array([vx,vy,vz])/v
        halfplist=DecayProducts(int(PID),newmome)
        plist = np.hstack((np.ones((halfplist.shape[0], 3))*loc, halfplist[:,0:5], 
                           np.ones((halfplist.shape[0], 1))*sol.t_events[3],
                           halfplist[:,5:]))
        return (res,plist)
