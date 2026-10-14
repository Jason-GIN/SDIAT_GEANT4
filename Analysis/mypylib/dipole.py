import numpy as np
import pandas as pd
from scipy.interpolate import interp1d
#from atmosphere import *
from scipy.special import ellipeinc

import logging
#import matplotlib.pyplot as plt
#from mpl_toolkits.mplot3d import Axes3D


def init():
    """init function"""
    logging.basicConfig(format='%(asctime)s --- %(levelname)s-%(processName)s:%(message)s', level=logging.INFO)

## 磁场以及坐标系转换函数

def myxyz2polar(xyz,t=1):   #直角坐标转化为球坐标
    
    #！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
    #参数t：
    #       用于选择返回形式，t=1，返回弧度制。t=2，返回角度制。
    #       默认 t=1。
    #！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
    
    x,y,z = xyz
    r=np.sqrt(x*x+y*y+z*z)
    if t==1:
        theta=np.arccos(z/r)
        phi=np.arctan2(y,x)
        return np.vstack((r,theta,phi))
    elif t==2:
        theta=np.degrees(np.acos(z/r))
        phi=np.degrees(np.atan2(y,x))
        return np.vstack([r,theta,phi])
        

def mypolar2xyz(polar): #球坐标转化为直角坐标
    r,theta,phi = polar
    x=r*np.sin(theta)*np.cos(phi)
    y=r*np.sin(theta)*np.sin(phi)
    z=r*np.cos(theta)
    return np.vstack((x,y,z))

def myxyz2polar2(xyz):   #直角坐标转化为球坐标
    
    #！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
    #返回角度制
    
    x,y,z = xyz
    r=np.sqrt(x*x+y*y+z*z)
    theta=np.degrees(np.acos(z/r))
    phi=np.degrees(np.atan2(y,x))
    return np.vstack([r,theta,phi])


def myrotatexyz(xyz,theta,phi):     # 将直角坐标的x y z点再旋转theta phi，以求得相对地球x y z处的天顶方向
    # 这个操作完之后，z轴指向天顶方向，y轴指向东，x轴指向南。
    
    x,y,z =xyz
    costheta=np.cos(theta); sintheta=np.sin(theta)
    cosphi=np.cos(phi); sinphi=np.sin(phi)
    xnew=x*costheta*cosphi-y*sinphi+z*sintheta*cosphi
    ynew=x*costheta*sinphi+y*cosphi+z*sintheta*sinphi
    znew=-x*sintheta+z*costheta
    return np.vstack((xnew,ynew,znew))

def myrotatexyz2(xyz,theta,phi):     # 天顶方向坐标转GTOD
    x,y,z =xyz
    costheta=np.cos(theta); sintheta=np.sin(theta)
    cosphi=np.cos(phi); sinphi=np.sin(phi)
    xnew=x*costheta*cosphi+y*sinphi*costheta-z*sintheta
    ynew=-x*sinphi+y*cosphi
    znew=x*sintheta*cosphi+y*sintheta*sinphi+z*costheta
    return np.vstack((xnew,ynew,znew))

def Mmoment(year=2024):
    
    a=6371.2 # 地球半径，单位是千米
    mu0=4*np.pi*1e-7  #真空磁导率的单位（国际）是特斯拉*米/安培，值为4*np.pi*1e-7， 
    yearl=[1900.0, 1905.0, 1910.0, 1915.0, 1920.0, 1925.0, 1930.0, 1935.0, 1940.0, 1945.0, 1950.0, 1955.0, 1960.0,
        1965.0, 1970.0, 1975.0, 1980.0, 1985.0, 1990.0, 1995.0, 2000.0, 2005.0, 2010.0, 2015.0, 2020.0]
    g0l=[-31543, -31464, -31354, -31212, -31060, -30926, -30805, -30715, -30654, -30594, -30554, -30500, -30421, 
        -30334, -30220, -30100, -29992, -29873, -29775, -29692, -29619.4, -29554.63, -29496.57, -29441.46, -29404.8]
    g1l=[-2298, -2298, -2297, -2306, -2317, -2318, -2316, -2306, -2292, -2285, -2250, -2215, -2169, 
        -2119, -2068, -2013, -1956, -1905, -1848, -1784, -1728.2, -1669.05, -1586.42, -1501.77, -1450.9]
    h1l=[5922, 5909, 5898, 5875, 5845, 5817, 5808, 5812, 5821, 5810, 5815, 5820, 5791, 
        5776, 5737, 5675, 5604, 5500, 5406, 5306, 5186.1, 5077.99, 4944.26, 4795.99, 4652.5]
    fg0 = interp1d(yearl, g0l, kind='linear', fill_value="extrapolate")
    fg1 = interp1d(yearl, g1l, kind='linear', fill_value="extrapolate")
    fh1 = interp1d(yearl, h1l, kind='linear', fill_value="extrapolate")
    g0 = fg0(year)
    g1 = fg1(year)
    h1 = fh1(year)
    M=4*np.pi/mu0*((a*1e3)**3)*np.sqrt(g0**2+g1**2+h1**2)  #单位是(T^2)*m/A
    return M, g0, g1, h1

def geoB(geoloca,year=2024):
    
    #获得地理坐标下的磁场B
    #输入地理位置：半径r（km）、余纬度theta（rad）、经度phi（rad)
    #返回球坐标系下此地理位置的磁场分量：Br、Btheta、Bphi
    #磁场单位是 nT?
    a=6371.2
    r, theta, phi=geoloca 
    #print(geoloca)
    #print(r,type(r))
    M, g0, g1, h1=Mmoment(year)
    #print(theta)
    #B=-Delta V
    #V=(a**3)/(r**2)*(g0*np.cos(theta)+
    #                (g1*np.cos(phi)+h1*np.sin(phi))*np.sin(theta))
    Br=2*(a**3)/(r**3)*(g0*np.cos(theta)+
                    (g1*np.cos(phi)+h1*np.sin(phi))*np.sin(theta))
    Btheta=-(a**3)/(r**3)*(-g0*np.sin(theta)+
                         (g1*np.cos(phi)+h1*np.sin(phi))*np.cos(theta))
    Bphi=-(a**3)/(r**3)*(-g1*np.sin(phi)+h1*np.cos(phi))
    #print(Br,type(Br))
    return np.array([Br,Btheta,Bphi])


def geodmB(geoloca, year=2024):
    
    #输入球坐标系下地理位置坐标：半径r（km）、余纬度theta（rad）、经度phi（rad)
    #返回球坐标系下地磁坐标的磁场分量：Brm、Bthetam、Bphim
    
    Br, Btheta, Bphi=geoB(geoloca, year)
    r, theta, phi=geoloca
    M, g0, g1, h1=Mmoment(year)
    m0=np.sqrt(g0**2+g1**2+h1**2)
    theta0=np.pi-np.arccos(g0/m0)
    phi0=-np.pi+np.arctan2(h1,g1)
    psi=np.arctan2(np.sin(theta0)*np.sin(phi-phi0),
                   np.cos(theta0)*np.sin(theta)-np.sin(theta0)*np.cos(theta)*np.cos(phi-phi0))
    Brm=Br
    Bthetam=np.cos(psi)*Btheta+np.sin(psi)*Bphi
    Bphim=-np.sin(psi)*Btheta+np.cos(psi)*Bphi
    return np.array([Brm, Bthetam, Bphim])

def Geo2Mgeo(geoloca, year=2024):
    
    #！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
    #输入：
    #       geoloca 地理位置坐标：半径r（km）、余纬度theta（rad）、经度phi（rad)
    #       time 时间：以年为单位，默认2024
    
    #返回：
    #       球坐标系的地磁坐标：半径r（km）、余纬度thetad（rad）、经度phid（rad）
    
    r, theta, phi=geoloca
    #print(Mmoment(r,year))
    M, g0, g1, h1=Mmoment(year)
    M0=np.sqrt(g0**2+g1**2+h1**2)
    theta0=np.pi-np.arccos(g0/M0)
    phi0=-np.pi+np.arctan2(h1, g1)
    #print('theta0: ',np.degrees(theta0), 'phi0: ', np.degrees(phi0))
    thetad=np.arccos(np.cos(theta)*np.cos(theta0)
                     +np.sin(theta)*np.sin(theta0)*np.cos(phi-phi0))
    phid=np.arctan2(np.sin(theta)*np.sin(phi-phi0), 
                    (np.cos(theta0)*np.sin(theta)*np.cos(phi-phi0)-np.sin(theta0)*np.cos(theta)))
        
    return np.vstack((r,thetad,phid))

def Mgeo2Geo(mgeoloca, year=2024, t=1):
    
    #！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
    #输入：
    #       mgeoloca 磁纬度坐标：半径r（km），余纬度theta（rad），经度phi（rad)
    #       t, 用于选择返回形式，
    #           t=1，返回球坐标（弧度制）。t=2，返回球坐标（角度制）。默认 t=1。
    #           t=3，返回直角坐标
    #       time 时间：以年为单位，默认2024
    #返回：
    #       球坐标系下的地理坐标：半径r（km）、余纬度thetad（rad）、经度phid（rad）

    r, theta, phi=mgeoloca
    x,y,z=mypolar2xyz([r,theta,phi])
    
    M, g0, g1, h1=Mmoment(year)
    M0=np.sqrt(g0**2+g1**2+h1**2)
    theta0=np.pi-np.arccos(g0/M0)
    phi0=-np.pi+np.arctan2(h1, g1)
    
    x1=(np.cos(theta0)*np.cos(phi0)*x
        -np.sin(phi0)*y
        +np.cos(phi0)*np.sin(theta0)*z)
    y1=(np.sin(phi0)*np.cos(theta0)*x
        +np.cos(phi0)*y
        +np.sin(phi0)*np.sin(theta0)*z)
    z1=(-np.sin(theta0)*x+np.cos(theta0)*z)
    
    if t==1:
        return myxyz2polar(np.vstack((x1,y1,z1)))   #  np.vstack((x1,y1,z1)) 
    elif t==2:
        return myxyz2polar(np.vstack((x1,y1,z1)),t)
    elif t==3:
        return np.vstack((x1,y1,z1))
    else:
        print("Error! Argument 't' is given in wrong way.")
        return 0

def Geo2MgeoVector(Vector, geoloca, year=2024):
    
    #！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
    #输入：
    #       Vector 地理坐标下的球分量：r，theta，phi
    #       geoloca 地理位置坐标：半径r（km）、余纬度theta（rad）、经度phi（rad)
    #       time 时间：以年为单位，默认2024
    #返回：
    #       NewVector 地磁坐标下的球分量：r，thetad，phid
    
    Vr, Vt, Vp=Vector
    r, theta, phi=geoloca
    #print(Mmoment(r,year))
    M, g0, g1, h1=Mmoment(year)
    M0=np.sqrt(g0**2+g1**2+h1**2)
    theta0=np.pi-np.arccos(g0/M0)
    phi0=-np.pi+np.arctan2(h1, g1)
    #print('theta0: ',np.degrees(theta0), 'phi0: ', np.degrees(phi0))
    psi=np.arctan2((np.sin(theta0)*np.sin(phi-phi0)),
                    (np.cos(theta0)*np.sin(theta)-np.sin(theta0)*np.cos(theta)*np.cos(phi-phi0)))
    nVr=Vr
    nVt=np.cos(psi)*Vt+np.sin(psi)*Vp
    nVp=-np.sin(psi)*Vt+np.cos(psi)*Vp
    
    return np.vstack((nVr,nVt,nVp))


def Mgeo2GeoVector(Vector, mgeoloca, year=2024):
    
    #！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
    #输入：
    #       Vector 地磁坐标下的球分量：r，theta，phi
    #       mgeoloca 磁纬度坐标：半径r（km），余纬度theta（rad），经度phi（rad)
    #       time 时间：以年为单位，默认2024
    #返回：
    #       NewVector 地理坐标下的球分量：r，thetad，phid
    
    Vr, Vt, Vp=Vector
    r, theta, phi=Mgeo2Geo(mgeoloca)
    #print(Mmoment(r,year))
    M, g0, g1, h1=Mmoment(year)
    M0=np.sqrt(g0**2+g1**2+h1**2)
    theta0=np.pi-np.arccos(g0/M0)
    phi0=-np.pi+np.arctan2(h1, g1)
    #print('theta0: ',np.degrees(theta0), 'phi0: ', np.degrees(phi0))
    psi=np.arctan2((np.sin(theta0)*np.sin(phi-phi0)),
                    (np.cos(theta0)*np.sin(theta)-np.sin(theta0)*np.cos(theta)*np.cos(phi-phi0)))
    nVr=Vr
    nVt=np.cos(psi)*Vt-np.sin(psi)*Vp
    nVp=np.sin(psi)*Vt+np.cos(psi)*Vp
    
    return np.vstack((nVr,nVt,nVp))

def myDipole( mgeo ):
    #从磁偶极子的表达式求磁坐标系下的磁偶极近似的磁场
    #因为对称性，只需要考虑theta
    #
    #  输入： 磁坐标 r半径（km） 余纬度theta（rad）
    #  输出： 径向分量Br   theta方向分量 Btheta
    
    r,theta,phi=mgeo
    M=Mmoment()[0]
    Br = -2*M/((r*1e3)**3)*np.cos(theta)*1e-7  #要乘以（mu/4π）这个因子，由于mu=4π*1e-7，直接保留1e-7
    Btheta = -M/((r*1e3)**3)*np.sin(theta)*1e-7 #要乘以（mu/4π）这个因子，由于mu=4π*1e-7，直接保留1e-7
    #print(Br,"\n",Btheta)
    
    return np.vstack((Br,Btheta))

## 单位球分割（得到方向）

def arc(x, alphamax, mEllip, value):
    """compute arc"""
    return ellipeinc(np.pi*x/alphamax, mEllip)-value


## 解微分方程函数
'''
def XVRDinkm(t,XVR,qcharge,Ekg,beta,MM,rigGV):  #数值求解常微分方程组的导函数数组定义
    x, y, z, vx, vy, vz, dR = XVR    #坐标和速度和变化的刚度积分，距离单位是km，时间单位是s，刚度单位是GV
    r,theta,phi=myxyz2polar(np.hstack((x,y,z))).T[0]
    Br,Bt,Bp=geodmB([r,np.pi/2-theta,phi],2024) # 输出值约为3.e4
    Bx = Br*np.sin(theta)*np.cos(phi)+Bt*np.cos(theta)*np.cos(phi)-Bp*np.sin(theta)*np.sin(phi)
    By = Br*np.sin(theta)*np.sin(phi)+Bt*np.cos(theta)*np.sin(phi)+Bp*np.sin(theta)*np.cos(phi)
    Bz = Br*np.cos(theta)-Bt*np.sin(theta)    # B的各分量转化为直角坐标
    ax = (vy*Bz-vz*By)*qcharge/Ekg*1e-9    # Relativistic Lorentz force, (gamma*m)^{-1}->sqrt(1-v**2)
    ay = (vz*Bx-vx*Bz)*qcharge/Ekg*1e-9    # B in nT，地球磁场大小约3.e-5T
    az = (vx*By-vy*Bx)*qcharge/Ekg*1e-9    # huor
    
    
    CommonLog = np.log((2*me*beta**2/(1-beta**2))**2/(1+2*me/(MM*(1-beta**2)**0.5)+me**2/MM**2))
    fromH = 1/1.0080*(CommonLog - HLog - 2*beta**2) * HDens(r)     # in kg / m3
    fromHe = 2/4.0026*(CommonLog - HeLog - 2*beta**2) * HeDens(r)
    fromC = 6/12.011*(CommonLog - CLog - 2*beta**2) * CDens(r)
    fromN = 7/14.0067*(CommonLog - NLog - 2*beta**2) * NDens(r)
    fromO = 8/15.9994*(CommonLog - OLog - 2*beta**2) * ODens(r)
    fromAr = 18/39.948*(CommonLog - ArLog - 2*beta**2) * ArDens(r)
    dROdt = coeff*(fromH + fromHe + fromC + fromN + fromO + fromAr) * 1000 * ccinkm
    #print('gin')
    #print(type(ax))
    return np.hstack((vx, vy, vz, ax, ay, az, dROdt))

def rLow0(t,XVR,qcharge,Ekg,beta,MM,rigGV):     # 积分integrate.solve_ivp的内置events判别，是否撞上稠密大气
    x, y, z, vx, vy, vz, dR = XVR    #坐标和速度和变化的刚度积分，距离单位是km，时间单位是s，刚度单位是GV
    r,theta,phi=myxyz2polar(np.asarray([x,y,z])).T[0]    # 可以再定义一个函数只计算r以加速
    return r-0-RE

rLow0.terminal = True            # 若撞上稠密大气则积分可以终止
def rLow100(t,XVR,qcharge,Ekg,beta,MM,rigGV):     # 积分integrate.solve_ivp的内置events判别，是否撞上稠密大气
    x, y, z, vx, vy, vz, dR = XVR    #坐标和速度和变化的刚度积分，距离单位是km，时间单位是s，刚度单位是GV
    r,theta,phi=myxyz2polar(np.asarray([x,y,z])).T[0]    # 可以再定义一个函数只计算r以加速
    return r-100-RE

rLow100.terminal = False            # 若撞上稠密大气则积分可以终止
def rBetheBloch(t,XVR,qcharge,Ekg,beta,MM,rigGV):    # 积分integrate.solve_ivp的内置events判别，是否已逃离地磁场
    x, y, z, vx, vy, vz, dR = XVR    #坐标和速度和变化的刚度积分，距离单位是km，时间单位是s，刚度单位是GV
    return (0.1*rigGV)-dR

rBetheBloch.terminal = False           # 若能损过大
def rHigh(t,XVR,qcharge,Ekg,beta,MM,rigGV):    # 积分integrate.solve_ivp的内置events判别，是否已逃离地磁场
    x, y, z, vx, vy, vz, dR = XVR    #坐标和速度和变化的刚度积分，距离单位是km，时间单位是s，刚度单位是GV
    r,theta,phi=myxyz2polar(np.asarray([x,y,z])).T[0]    # 可以再定义一个函数只计算r以加速
    return r-1e5

rHigh.terminal = True            # 若已逃离地磁场则积分可以终止

def rLowV(t,XVR,qcharge,Ekg,beta,MM,rigGV):     # 积分integrate.solve_ivp的内置events判别，是否达到轨迹底部
    x, y, z, vx, vy, vz, dR = XVR
    vr = x*vx + y*vy + z*vz    # 可以再定义一个函数只计算vr以加速
    return vr


def is_recept(sol, n):
    """judge particle is allowed to fly out space or not"""
    if np.sum(sol[n].t_events[0]) or (np.sum(sol[n].t_events[2])>0 and any([np.linalg.norm(vr[0:3])<100+6371.2 for vr in sol[n].y_events[2]])):
        return 0
    elif np.sum(sol[n].t_events[1])>0:
        return 1
    else:
        return 0
'''


'''
## 磁坐标下dipole场的对称性测试
mgeolocation1=[6371.2,np.radians(90),np.radians(0)]
mgeolocation2=[6371.2,np.radians(90),np.radians(186)]
geolocation1=Mgeo2Geo(mgeolocation1)
geolocation2=Mgeo2Geo(mgeolocation2)

print(geolocation1.T)
print(geolocation2.T)
print(geoB(geolocation1).T)
print(geoB(geolocation2).T)
print(geodmB(geolocation1).T)
print(geodmB(geolocation2).T)


## 坐标转换测试
geolcation=[6371.2,np.radians(20),np.radians(0)]
#print(geoB([6371.2,0,0]))
#print(geogB2geodmB([6371.2,0,0]))
print(Geo2Mgeo(geolcation).T)
#print(np.degrees(np.pi/2-Geo2Mgeo(geolcation).T[0][1]))
#print(np.degrees(Geo2Mgeo(geolcation).T[0][2]))
mgeolcation=[6371.2,1.17865989,1.32965733]
print(Mgeo2Geo(mgeolcation).T)
print(np.degrees(Mgeo2Geo(mgeolcation).T[0][1]))
print(np.degrees(Mgeo2Geo(mgeolcation).T[0][2]))


rad=np.ones(1000)*6371.2
lat=np.zeros(1000)
lon=np.arange(0,2*np.pi,np.pi/500)
#print(lon)
geolcation=np.vstack((rad,lat,lon))
mgeolcation=Geo2Mgeo(geolcation)
regeolcation=Mgeo2Geo(mgeolcation)

xyz=mypolar2xyz(np.vstack((rad,np.pi/2-lat,lon)))
#print(xyz)
mxyz=mypolar2xyz(mgeolcation)

rexyz=regeolcation

da0=pd.DataFrame(xyz)
str0="./geolcation.csv"
da0.to_csv(str0, header=False,  index=False)

da1=pd.DataFrame(mxyz)
str1="./mgeolcation.csv"
da1.to_csv(str1, header=False,  index=False)

da2=pd.DataFrame(rexyz)
str2="./regeolcation.csv"
da2.to_csv(str2, header=False,  index=False)
'''