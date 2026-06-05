#import pandas as pd
import numpy as np

#这里所有函数默认计算得到的是弧度制的角度值

def xyz2rthetaphi1(x,y,z):
    r=np.sqrt(x*x+y*y+z*z)
    theta=np.arccos(z/r)
    phi=np.arctan2(y,x)
    return np.vstack((r,theta,phi))
    #return np.vstack((r,90-np.rad2deg(theta),np.rad2deg(phi)))
    
    
def xyz2rthetaphi(x,y,z):
    r=np.sqrt(x*x+y*y+z*z)
    theta=np.arccos(z/r)
    phi=np.arctan2(y,x)
    return r,theta,phi
    #return np.vstack((r,90-np.rad2deg(theta),np.rad2deg(phi)))

def rthetaphi2xyz(r,theta,phi):
    x=r*np.sin(theta)*np.cos(phi)
    y=r*np.sin(theta)*np.sin(phi)
    z=r*np.cos(theta)
    return x,y,z
    #return np.vstack((r,90-np.rad2deg(theta),np.rad2deg(phi)))

def rthetaphi2xyz1(r,theta,phi):
    x=r*np.sin(theta)*np.cos(phi)
    y=r*np.sin(theta)*np.sin(phi)
    z=r*np.cos(theta)
    return np.vstack((x,y,z))
    #return np.vstack((r,90-np.rad2deg(theta),np.rad2deg(phi)))

def VectorGTODinZenith2(x,y,z,theta,phi):
    # 这里的theta是余纬度，phi是经度
    # 
    xn=np.cos(phi)*np.cos(theta)*x+np.sin(phi)*np.cos(theta)*y-np.sin(theta)*z
    yn=-np.sin(phi)*x+np.cos(phi)*y
    zn=np.cos(phi)*np.sin(theta)*x+np.sin(phi)*np.sin(theta)*y+np.cos(theta)*z
    
    return np.vstack((xn,yn,zn))

def VectorGTODinZenith(x,y,z,x0,y0,z0):
    # 这里的theta是余纬度，phi是经度
    # 
    r,theta,phi=xyz2rthetaphi(x0,y0,z0) 
    xn=np.cos(phi)*np.cos(theta)*x+np.sin(phi)*np.cos(theta)*y-np.sin(theta)*z
    yn=-np.sin(phi)*x+np.cos(phi)*y
    zn=np.cos(phi)*np.sin(theta)*x+np.sin(phi)*np.sin(theta)*y+np.cos(theta)*z
    
    return np.vstack((xn,yn,zn))

def VectorGTODinZenith1(x,y,z,x0,y0,z0):
    # 这里的theta是余纬度，phi是经度
    # 
    r,theta,phi=xyz2rthetaphi(x0,y0,z0) 
    xn=np.cos(phi)*np.cos(theta)*x+np.sin(phi)*np.cos(theta)*y-np.sin(theta)*z
    yn=-np.sin(phi)*x+np.cos(phi)*y
    zn=np.cos(phi)*np.sin(theta)*x+np.sin(phi)*np.sin(theta)*y+np.cos(theta)*z
    
    return xn,yn,zn

def VectorZenithinGTOD2(mx,my,mz,theta,phi):
    # 这里的theta是余纬度，phi是经度
    # 
    mxn=np.cos(phi)*np.cos(theta)*mx-np.sin(phi)*my+np.cos(phi)*np.sin(theta)*mz
    myn=np.sin(phi)*np.cos(theta)*mx+np.cos(phi)*my+np.sin(phi)*np.sin(theta)*mz
    mzn=-np.sin(theta)*mx+np.cos(theta)*mz
    
    return np.vstack((mxn,myn,mzn))

def VectorZenithinGTOD1(mx,my,mz,theta,phi):
    # 这里的theta是余纬度，phi是经度
    # 
    mxn=np.cos(phi)*np.cos(theta)*mx-np.sin(phi)*my+np.cos(phi)*np.sin(theta)*mz
    myn=np.sin(phi)*np.cos(theta)*mx+np.cos(phi)*my+np.sin(phi)*np.sin(theta)*mz
    mzn=-np.sin(theta)*mx+np.cos(theta)*mz
    
    return mxn,myn,mzn

def VectorZenithinGTOD(mx,my,mz,x0,y0,z0):

    r,theta,phi=xyz2rthetaphi(x0,y0,z0) 
    mxn=np.cos(phi)*np.cos(theta)*mx-np.sin(phi)*my+np.cos(phi)*np.sin(theta)*mz
    myn=np.sin(phi)*np.cos(theta)*mx+np.cos(phi)*my+np.sin(phi)*np.sin(theta)*mz
    mzn=-np.sin(theta)*mx+np.cos(theta)*mz
    
    return np.vstack((mxn,myn,mzn))

def drirenomal (X,Y,Z,nMx,nMy,nMz,colat,lon):
    mx1,my1,mz1 = VectorZenithinGTOD2(nMx,nMy,nMz,colat,lon)
    x1,y1,z1 = VectorZenithinGTOD2(X,Y,Z,colat,lon)
    mx2,my2,mz2 = VectorGTODinZenith(mx1,my1,mz1,x1,y1,z1)
    #x2,y2,z2 = VectorGTODinZenith(x1,y1,z1,x1,y1,z1)
    rr1,tt1,pp1=xyz2rthetaphi(mx2,my2,mz2)
    return rr1,tt1,pp1