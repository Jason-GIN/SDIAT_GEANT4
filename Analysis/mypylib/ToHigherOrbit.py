import numpy as np
from scipy import integrate
from .RotationModel import *

def XVDinkm(t,XV,):  #数值求解常微分方程组的导函数数组定义
    x,y,z,vx,vy,vz=XV
    return [vx, vy, vz, 0, 0, 0]


def rHigh(t,XV,):    # 积分integrate.solve_ivp的内置events判别，是否已逃离地磁场
    x, y, z, vx, vy, vz = XV
    r,theta,phi=xyz2rthetaphi(x,y,z)   # 可以再定义一个函数只计算r以加速
    return r-1e5

rHigh.terminal = True           # 若已逃离地磁场则积分可以终止

def make_Revent_rHigh(R,ter=True, dir=0):
    def ReTouch(t, XV, ):
        r=np.linalg.norm(np.array(XV[0:3]))
        #print("empty")
        return r-R
    ReTouch.terminal=ter
    ReTouch.direction=dir
    return ReTouch

def trac2Orbit(Xinit, monento,altitude,i,):
    # print("Xinit: ",Xinit)
    # print("monento: ",monento)
    R_terminal=make_Revent_rHigh(np.array([altitude]),True,0)
    tspan=(0.0,100.0)
    XVinit=np.append(Xinit,monento)
    # print(type(XVinit))
    sol=integrate.solve_ivp(XVDinkm, tspan, XVinit, method='DOP853', events=[R_terminal],)
    # print(sol.y_events[0])
    return (i,sol.y_events[0],)


def trac2Orbit_vec(X, V, altitude):
    """解析求解粒子沿直线传播到半径 altitude 球面的交点（完全向量化）。

    原 trac2Orbit 用 solve_ivp 数值积分零加速度直线运动（XVDinkm 返回 [vx,vy,vz,0,0,0]），
    对每个粒子都做一次 ODE 求解，极慢。而"直线与球面的交点"有闭式解：

        |X + V t|^2 = altitude^2  ->  a t^2 + b t + c = 0 ，取正根。

    参数：
        X: (N,3) 初始位置（km）
        V: (N,3) 方向矢量（其模长不影响交点位置，只需方向一致）
        altitude: 目标半径（km）
    返回：
        (N,3) 交点在目标半径球面上的坐标
    """
    X = np.asarray(X, dtype=np.float64)
    V = np.asarray(V, dtype=np.float64)
    a = np.einsum('ij,ij->i', V, V)
    b = 2.0 * np.einsum('ij,ij->i', X, V)
    c = np.einsum('ij,ij->i', X, X) - altitude * altitude
    disc = np.maximum(b * b - 4.0 * a * c, 0.0)     # 防浮点误差导致负值
    a_safe = np.where(a == 0.0, 1e-30, a)
    t = (-b + np.sqrt(disc)) / (2.0 * a_safe)       # 正根
    return X + V * t[:, None]