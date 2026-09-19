import numpy as np
import pandas as pd
from scipy.integrate import solve_ivp
# import sympy as sp
from mypylib import *
from mypylib import myigrf14 
from scipy.optimize import bisect

def LineOfForce_IGRF14(t, state, Bm):
    a=6371.2
    r,theta, phi=RotationModel.xyz2rthetaphi(state[0],state[1],state[2])
    #这个玩意接收一个角度的theta和phi，而且phi的范围是0到360（但是实际上皮的取值在-180到180，sin，cos也是一样的）
    Bx,By,Bz,B=myigrf14.igrf_wrapper(np.rad2deg(theta), np.rad2deg(phi),r,2025)
    '''
    if theta0<np.radians(90):
        return -Bx,-By,-Bz
    else:
        return Bx,By,Bz
    '''
    return Bx,By,Bz


def make_Revent(R,ter=True, dir=0):
    def ReTouch(t, state,g0, g1, h1,theta0):
        x,y,z=state
        r=np.sqrt(x**2+y**2+z**2)
        #print("empty")
        return r-R
    ReTouch.terminal=ter
    ReTouch.direction=dir
    return ReTouch

def make_Bevent_igrf(B_threshold, ter=True, dir=0):
    def event(t, state, Bm):
        Bx,By,Bz=LineOfForce_IGRF14(t, state, Bm)
        Bc=np.sqrt(Bx**2+By**2+Bz**2)
        return Bc-B_threshold 
    event.terminal=ter
    event.direction =dir
    return event

def make_Revent_igrf(R,ter=True, dir=0):
    def ReTouch(t, state, Bm):
        x,y,z=state[0:3]
        r=np.sqrt(x**2+y**2+z**2)
        #print("empty")
        return r-R
    ReTouch.terminal=ter
    ReTouch.direction=dir
    return ReTouch

def BMirror(pAagle, Blocal, ):
    cospA,sinpA=pAagle
    Bm = Blocal/(sinpA**2)
    return Bm

def BEquator_m(g0, g1, h1, r ):
    a=6371.2
    Be=(a**3)/(r**3)*np.sqrt(g0**2+g1**2+h1**2)
    return Be

'''
def Req_dipole(x,y,z,time=2025):
    a=6371.2
    M, g0, g1, h1=dipole.Mmoment(time)
    M0=a**3*np.sqrt(g0**2+g1**2+h1**2)
    r,t,p=RotationModel.xyz2rthetaphi(x,y,z)
    rm,tm,pm=dipole.Geo2Mgeo([r,t,p],time)
    Brtp=dipole.geoB([r,t,p],time)
    B=np.linalg.norm(Brtp)
    req=(M0*np.sqrt(3*np.cos(t)**2+1)/(B*np.sin(t)**6))**(1/3)
    return req
'''

def C_MirrorPoint(loca, B, Bm):
    #这个函数应该是用来找镜像点的，
    r,theta0,phi0=RotationModel.xyz2rthetaphi(loca[0],loca[1],loca[2])
    B_terminal=make_Bevent_igrf(B,ter=False,dir=1)
    R_terminal=make_Revent_igrf(np.array([6371.2]),True,1)
    tmax=10000
    #需要南北半球的判断
    if theta0 <= np.pi/2:
        sol = solve_ivp(LineOfForce_IGRF14, (0,tmax), loca,args=(Bm,),
                        events=[B_terminal,], method='RK45')
    else:
        sol = solve_ivp(LineOfForce_IGRF14, (0,-tmax), loca,args=(Bm,),
                        events=[B_terminal,], method='RK45')    
    #a=sol.y_events[0][0]
    return sol.y_events[0][0]

