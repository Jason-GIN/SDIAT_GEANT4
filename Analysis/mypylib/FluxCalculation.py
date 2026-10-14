import uproot 
import pandas as pd
import numpy as np

from .FilterModel import *
from .RotationModel import *
from .myutils import *
from .dipole import *

# 这个文文件我想整一个用来计算不同的情况的下的能谱的
# 1，得先明确，能谱的本质是什么
# 能谱的本质是分布密度函数，或者说就是分bin，再除以bin宽
# 2，想要实现的是什么
# 输入文件的目录，输出一个分布，在这个分布就是在给定的范围生成
# 
# 

def AreaFilter(xyz,xyzlocal,d,model):
    # xyz是输入粒子的XYZ坐标
    # xyzlocal是我需要筛选的球坐标，
    # model模式选择，点=>0，范围=>1
    # d与model相关，如果model是范围模式，则也是模式选择，磁纬度=>0，地理纬度=>1，
    # 如果model是点模式，则是面积边界到中心的距离（默认是一个圆面的弧长）
    if model==0:
        xyzlocal=rthetaphi2xyz1(xyzlocal[0],xyzlocal[1],xyzlocal[2]).T
        dot_product = np.dot(xyzlocal,xyz)
        normlocal = np.linalg.norm(xyzlocal,axis=1)
        normin = np.linalg.norm(xyz,axis=0)
        cos_theta = dot_product / (normlocal * normin)
        angle_radians = np.arccos(cos_theta)  # 结果是弧度值
        l = np.abs(angle_radians*normin)
        #indxl = np.where((l <= d)) 
        indxl=np.flatnonzero(l <= d) 
    else:
        location=xyz2rthetaphi1(xyz[0],xyz[1],xyz[2])
        if d==1:
            t=location[1]
            indxl=np.flatnonzero((t <= xyzlocal[2]) & (t >= xyzlocal[1]))
        else:
            rm,tm,pm=Geo2Mgeo(location)
            # indxl=np.flatnonzero((tm <= xyzlocal[2]) & (tm >= xyzlocal[1]))
            tr=np.abs(np.pi/2-tm)
            indxl=np.flatnonzero((tr <= xyzlocal[2]) & (tr >= xyzlocal[1]))
    return indxl


def FluxCalculationOn_T_E(filename, ARange, gtbins, gEbins):
    #ARange 是一个4个元素的数组，前三个是地理位置（r, t 弧度, p 弧度），第四个是面积边界到中心的距离（默认是一个圆面的弧长）
    a=uproot.open(filename)
    E=a["DetectionInfo"]["Energy"].array(library="np")
    X=a["DetectionInfo"]["PosX"].array(library="np")
    Y=a["DetectionInfo"]["PosY"].array(library="np")
    Z=a["DetectionInfo"]["PosZ"].array(library="np")
    Mx=a["DetectionInfo"]["DirX"].array(library="np")
    My=a["DetectionInfo"]["DirY"].array(library="np")
    Mz=a["DetectionInfo"]["DirZ"].array(library="np")
    
    # 这样一步挑出了在一定面积内的所有粒子
    xyzlocal=rthetaphi2xyz1(ARange[0],ARange[1],ARange[2]).T
    xyz=np.vstack((X,Y,Z))
    idxInA=AreaFilter(xyz,xyzlocal,ARange[3])
    EinA=E[idxInA]
    XinA=X[idxInA]
    YinA=Y[idxInA]
    ZinA=Z[idxInA]
    MxinA=Mx[idxInA]
    MyinA=My[idxInA]
    MzinA=Mz[idxInA]
    nMx=MxinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMy=MyinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMz=MzinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    
    #这个是能量和theta角的分布，一般整这个就可以
    mx,my,mz = VectorGTODinZenith (nMx, nMy, nMz,
                                  XinA, YinA, ZinA,)

    r,t,p=xyz2rthetaphi(mx,my,mz)
    Etcounts, Ebin_edges, tbin_edges = np.histogram2d(EinA, 
                                                      np.rad2deg(np.pi-t), bins=(gEbins,gtbins))
    return Etcounts
    
def FluxCalculationOn_T_P(X,Y,Z,Mx,My,Mz, ARange, gtbins, gpbins):
    # 对于指定点
    # ARange 是一个5个元素的数组，前三个是地理位置（r, t 弧度, p 弧度），第四个是面积边界到中心的距离（默认是一个圆面的弧长），第五个也是模式选择，点=0，范围=1
    # 这样一步挑出了在一定面积内的所有粒子
    # 对于轨道整一个轨道，或者纬度范围
    # ARange 是一个5个元素的数组，
    # 三个是地理位置（r=0（这个东西在这里没有用）, t_star 弧度, t_end 弧度），
    # 第五个是模式选择，点=>0，范围=>1
    # 第四个的意义与第五个相关，如果第五个是范围模式，则也是模式选择，磁纬度=>0，地理纬度=>1，
    # 如果第五个是点模式，则是面积边界到中心的距离（默认是一个圆面的弧长）
    xyzlocal=ARange[0:3]
    xyz=np.vstack((X,Y,Z))
    idxInA=AreaFilter(xyz,xyzlocal,ARange[3],0)
    XinA=X[idxInA]
    YinA=Y[idxInA]
    ZinA=Z[idxInA]
    MxinA=Mx[idxInA]
    MyinA=My[idxInA]
    MzinA=Mz[idxInA]
    nMx=MxinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMy=MyinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMz=MzinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    
    #这个是能量和theta角的分布，一般整这个就可以
    mx,my,mz = VectorGTODinZenith (nMx, nMy, nMz,
                                  XinA, YinA, ZinA,)

    r,t,p=xyz2rthetaphi(-mx,-my,-mz) #加负号是因为探测器探测的是进入的方向。
    pdeg=np.rad2deg(p)
    # tdeg=np.rad2deg(np.pi-t)
    tdeg=np.rad2deg(t)
    tpcounts, pbin_edges, tbin_edges = np.histogram2d(pdeg, 
                                                      tdeg, bins=(gpbins,gtbins))
    return tpcounts
    
def FluxCalculationOn_T_P_E(X,Y,Z,Mx,My,Mz,E, ARange, gtbins, gpbins,gEbins):
    # 对于指定点
    # ARange 是一个5个元素的数组，前三个是地理位置（r, t 弧度, p 弧度），第四个是面积边界到中心的距离（默认是一个圆面的弧长），第五个也是模式选择，点=0，范围=1
    # 这样一步挑出了在一定面积内的所有粒子
    # 对于轨道整一个轨道，或者纬度范围
    # ARange 是一个5个元素的数组，
    # 三个是地理位置（r=0（这个东西在这里没有用）, t_star 弧度, t_end 弧度），
    # 第五个是模式选择，点=>0，范围=>1
    # 第四个的意义与第五个相关，如果第五个是范围模式，则也是模式选择，磁纬度=>0，地理纬度=>1，
    # 如果第五个是点模式，则是面积边界到中心的距离（默认是一个圆面的弧长）
    xyzlocal=ARange[0:3]
    xyz=np.vstack((X,Y,Z))
    idxInA=AreaFilter(xyz,xyzlocal,ARange[3],ARange[4])
    XinA=X[idxInA]
    YinA=Y[idxInA]
    ZinA=Z[idxInA]
    MxinA=Mx[idxInA]
    MyinA=My[idxInA]
    MzinA=Mz[idxInA]
    EinA=E[idxInA]
    nMx=MxinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMy=MyinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMz=MzinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    
    #这个是能量和theta角的分布，一般整这个就可以
    #r,t,p = drirenomal (XinA, YinA, ZinA, nMx, nMy, nMz,ARange[1],ARange[2])
    mx,my,mz = VectorGTODinZenith (nMx, nMy, nMz,
                                  XinA, YinA, ZinA,)

    r,t,p=xyz2rthetaphi(mx,my,mz)
    pdeg=np.rad2deg(p)
    tdeg=np.rad2deg(np.pi-t)
    sample=np.vstack((np.rad2deg(p), np.rad2deg(np.pi-t), EinA))
    tpecounts, edges = np.histogramdd(sample.T, bins=(gpbins,gtbins,gEbins))
    return tpecounts

def FluxCalculationOn_T_P_E_wt(X,Y,Z,Mx,My,Mz,E,ti,ARange, gtbins, gpbins,gEbins):
    # 对于指定点
    # ARange 是一个5个元素的数组，前三个是地理位置（r, t 弧度, p 弧度），第四个是面积边界到中心的距离（默认是一个圆面的弧长），第五个也是模式选择，点=0，范围=1
    # 这样一步挑出了在一定面积内的所有粒子
    # 对于轨道整一个轨道，或者纬度范围
    # ARange 是一个5个元素的数组，
    # 三个是地理位置（r=0（这个东西在这里没有用）, t_star 弧度, t_end 弧度），
    # 第五个是模式选择，点=>0，范围=>1
    # 第四个的意义与第五个相关，如果第五个是范围模式，则也是模式选择，磁纬度=>0，地理纬度=>1，
    # 如果第五个是点模式，则是面积边界到中心的距离（默认是一个圆面的弧长）
    xyzlocal=ARange[0:3]
    xyz=np.vstack((X,Y,Z))
    idxInA=AreaFilter(xyz,xyzlocal,ARange[3],ARange[4])
    XinA=X[idxInA]
    YinA=Y[idxInA]
    ZinA=Z[idxInA]
    MxinA=Mx[idxInA]
    MyinA=My[idxInA]
    MzinA=Mz[idxInA]
    EinA=E[idxInA]
    tinA=ti[idxInA]
    nMx=MxinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMy=MyinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMz=MzinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    
    #这个是能量和theta角的分布，一般整这个就可以
    #r,t,p = drirenomal (XinA, YinA, ZinA, nMx, nMy, nMz,ARange[1],ARange[2])
    mx,my,mz = VectorGTODinZenith (nMx, nMy, nMz,
                                  XinA, YinA, ZinA,)

    r,t,p=xyz2rthetaphi(mx,my,mz)
    pdeg=np.rad2deg(p)
    tdeg=np.rad2deg(np.pi-t)
    sample=np.vstack((np.rad2deg(p), np.rad2deg(np.pi-t), EinA))
    tpecounts, edges = np.histogramdd(sample.T, bins=(gpbins,gtbins,gEbins),weights=1/tinA)
    return tpecounts