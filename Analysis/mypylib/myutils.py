import numpy as np
import uproot
import multiprocessing as mps

from .FilterModel import *
from .RotationModel import *
from .FluxCalculation import *
from .ToHigherOrbit import *
from .dipole import *

def Gedistrubtiont(colat,lon,str0,gbins,rangnumber,rnumbegine=0,flag=0):
    Carray=gbins[0:-1]
    
    a = uproot.open(str0)
    SDnum=a["DetectionInfo"]["SDnum"].array(library="np")
    #pPID=a["DetectionInfo"]["pPID"].array(library="np")
    #ParentId=a["DetectionInfo"]["ParentId"].array(library="np")
    E=a["DetectionInfo"]["Energy"].array(library="np")
    X=a["DetectionInfo"]["PosX"].array(library="np")
    Y=a["DetectionInfo"]["PosY"].array(library="np")
    Z=a["DetectionInfo"]["PosZ"].array(library="np")
    Mx=a["DetectionInfo"]["DirX"].array(library="np")
    My=a["DetectionInfo"]["DirY"].array(library="np")
    Mz=a["DetectionInfo"]["DirZ"].array(library="np")
    if flag != 0:
        idE=filterrange(E,flag,100000)
        SDnum0=SDnum[idE]
        X0=X[idE]
        Y0=Y[idE]
        X0=X[idE]
        Y0=Y[idE]
        Z0=Z[idE]
        Mx0=Mx[idE]
        My0=My[idE]
        Mz0=Mz[idE]
    else:
        SDnum0=SDnum
        X0=X
        Y0=Y
        X0=X
        Y0=Y
        Z0=Z
        Mx0=Mx
        My0=My
        Mz0=Mz
    nMx=Mx0/np.sqrt(Mx0*Mx0+My0*My0+Mz0*Mz0)
    nMy=My0/np.sqrt(Mx0*Mx0+My0*My0+Mz0*Mz0)
    nMz=Mz0/np.sqrt(Mx0*Mx0+My0*My0+Mz0*Mz0)
    for i in range (rnumbegine,rangnumber,1):        
        idh=filter(SDnum0,[i])
        r,t,p = drirenomal (X0[idh], Y0[idh], Z0[idh], nMx[idh], nMy[idh], nMz[idh],colat,lon)
        tcounts, tbin_edges = np.histogram(np.rad2deg(np.pi-t), bins=gbins)
        Carray=np.vstack((Carray,tcounts))
        
    return Carray

def GedistrubtiontpinPart(colat,lon,batch,gtbins,gpbins,rangnumber,rnumbegine=0,flag=0):
    if (flag==0):
        SDnum=batch["SDnum"].array(library="np")
        X=batch["PosX"].array(library="np")
        Y=batch["PosY"].array(library="np")
        Z=batch["PosZ"].array(library="np")
        Mx=batch["DirX"].array(library="np")
        My=batch["DirY"].array(library="np")
        Mz=batch["DirZ"].array(library="np")
    else:
        SDnum=batch["SDnum"]
        X=batch["PosX"]
        Y=batch["PosY"]
        Z=batch["PosZ"]
        Mx=batch["DirX"]
        My=batch["DirY"]
        Mz=batch["DirZ"]
    nMx=Mx/np.sqrt(Mx*Mx+My*My+Mz*Mz)
    nMy=My/np.sqrt(Mx*Mx+My*My+Mz*Mz)
    nMz=Mz/np.sqrt(Mx*Mx+My*My+Mz*Mz)
    
    for i in range (rnumbegine,rangnumber,1):        
        idh=filter(SDnum,[i])
        r,t,p = drirenomal (X[idh], Y[idh], Z[idh], nMx[idh], nMy[idh], nMz[idh],colat,lon)
        tpcounts, tbin_edges,pbin_edges = np.histogram2d(np.rad2deg(np.pi-t),
                                                         np.rad2deg(-p), bins=(gtbins,gpbins))
        #print(np.sum(tpcounts,axis=0))
        #print(len(np.sum(tpcounts,axis=0)))
        #print(np.sum(tpcounts,axis=1))
        #print(len(np.sum(tpcounts,axis=1)))
        Cp=np.vstack((gpbins[0:-1],tpcounts))
        Cpt=np.vstack((np.hstack((0,gtbins[0:-1])),Cp.T))
        if i == rnumbegine:
            Carray=Cpt
        else:
            Carray=np.vstack((Carray,Cpt))
    return Carray

def Gedistrubtiontp(colat,lon,str0,gtbins,gpbins,rangnumber,rnumbegine=0):
    a = uproot.open(str0)
    n_entries = a["DetectionInfo"].num_entries
    if (n_entries < 1e8):
        return GedistrubtiontpinPart(colat,lon,a["DetectionInfo"],gtbins,gpbins,rangnumber,rnumbegine)
    else:
        n=0
        #Carry=[]
        for batch in uproot.iterate(str0+":DetectionInfo",
                        expressions=["SDnum","PosX","PosY","PosZ",
                                     "DirX","DirY","DirZ"],  # branch select
                        step_size="900 MB",                  # size setting
                        library="np"                         # output type
        ):
            if (n==0):
                Carry = GedistrubtiontpinPart(colat,lon,batch,gtbins,gpbins,rangnumber,rnumbegine,1)[1:,1:]
            else:
                Carry = Carry+GedistrubtiontpinPart(colat,lon,batch,gtbins,gpbins,rangnumber,rnumbegine,1)[1:,1:]
            del batch
            n=n+1
        Carry=np.vstack((gpbins[0:-1],Carry.T))
        Carry=np.vstack((np.hstack((0,gtbins[0:-1])),Carry.T))
        return Carry
               
def GedistrubtionEt(colat,lon,str0,gtbins,gEbins,SDnumber):
    
    a = uproot.open(str0)
    SDnum=a["DetectionInfo"]["SDnum"].array(library="np")
    #pPID=a["DetectionInfo"]["pPID"].array(library="np")
    #ParentId=a["DetectionInfo"]["ParentId"].array(library="np")
    E=a["DetectionInfo"]["Energy"].array(library="np")
    X=a["DetectionInfo"]["PosX"].array(library="np")
    Y=a["DetectionInfo"]["PosY"].array(library="np")
    Z=a["DetectionInfo"]["PosZ"].array(library="np")
    Mx=a["DetectionInfo"]["DirX"].array(library="np")
    My=a["DetectionInfo"]["DirY"].array(library="np")
    Mz=a["DetectionInfo"]["DirZ"].array(library="np")
    nMx=Mx/np.sqrt(Mx*Mx+My*My+Mz*Mz)
    nMy=My/np.sqrt(Mx*Mx+My*My+Mz*Mz)
    nMz=Mz/np.sqrt(Mx*Mx+My*My+Mz*Mz)
    
          
    idh=filter(SDnum,[SDnumber])
    r,t,p = drirenomal (X[idh], Y[idh], Z[idh], nMx[idh], nMy[idh], nMz[idh],colat,lon)
    Etcounts, Ebin_edges, tbin_edges = np.histogram2d(E[idh], 
                                                      np.rad2deg(np.pi-t), bins=(gEbins,gtbins))
        
    return Etcounts

def Gedistrubtiontp_SD_E(colat,lon,str0,gtbins,gpbins,SDnumber,Erange):
    
    a = uproot.open(str0)
    SDnum=a["DetectionInfo"]["SDnum"].array(library="np")
    #pPID=a["DetectionInfo"]["pPID"].array(library="np")
    #ParentId=a["DetectionInfo"]["ParentId"].array(library="np")
    E=a["DetectionInfo"]["Energy"].array(library="np")
    X=a["DetectionInfo"]["PosX"].array(library="np")
    Y=a["DetectionInfo"]["PosY"].array(library="np")
    Z=a["DetectionInfo"]["PosZ"].array(library="np")
    Mx=a["DetectionInfo"]["DirX"].array(library="np")
    My=a["DetectionInfo"]["DirY"].array(library="np")
    Mz=a["DetectionInfo"]["DirZ"].array(library="np")
    
    ide=filterrange(E, Erange[0], Erange[1])
    
    nMx=Mx[ide]/np.sqrt(Mx[ide]*Mx[ide]+My[ide]*My[ide]+Mz[ide]*Mz[ide])
    nMy=My[ide]/np.sqrt(Mx[ide]*Mx[ide]+My[ide]*My[ide]+Mz[ide]*Mz[ide])
    nMz=Mz[ide]/np.sqrt(Mx[ide]*Mx[ide]+My[ide]*My[ide]+Mz[ide]*Mz[ide])
    X=X[ide]
    Y=Y[ide]
    Z=Z[ide]
    SDnum=SDnum[ide]
          
    idh=filter(SDnum,[SDnumber])
    r,t,p = drirenomal (X[idh], Y[idh], Z[idh], nMx[idh], nMy[idh], nMz[idh],colat,lon)
    tpcounts, pbin_edges, tbin_edges = np.histogram2d(np.rad2deg(-p), 
                                                      np.rad2deg(np.pi-t), bins=(gpbins,gtbins))
        
    return tpcounts

def Gedistrubtiontp_E_Ar(str0,gtbins,gpbins,Erange,ARange):
    
    a = uproot.open(str0)
    #pPID=a["DetectionInfo"]["pPID"].array(library="np")
    #ParentId=a["DetectionInfo"]["ParentId"].array(library="np")
    E=a["BoundaryInfo"]["Energy"].array(library="np")
    X=a["BoundaryInfo"]["PosX"].array(library="np")
    Y=a["BoundaryInfo"]["PosY"].array(library="np")
    Z=a["BoundaryInfo"]["PosZ"].array(library="np")
    Mx=a["BoundaryInfo"]["DirX"].array(library="np")
    My=a["BoundaryInfo"]["DirY"].array(library="np")
    Mz=a["BoundaryInfo"]["DirZ"].array(library="np")
    
    ide=filterrange(E, Erange[0], Erange[1])
    
    Mxi=Mx[ide]
    Myi=My[ide]
    Mzi=Mz[ide]
    Xi=X[ide]
    Yi=Y[ide]
    Zi=Z[ide]
    xyzlocal=ARange[0:3]
    xyz=np.vstack((Xi,Yi,Zi))
    idxInA=AreaFilter(xyz,xyzlocal,ARange[3],ARange[4])
    XinA=Xi[idxInA]
    YinA=Yi[idxInA]
    ZinA=Zi[idxInA]
    MxinA=Mxi[idxInA]
    MyinA=Myi[idxInA]
    MzinA=Mzi[idxInA]
    nMx=MxinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMy=MyinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMz=MzinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    # location=np.vstack((XinA, YinA, ZinA))
    # r0,t0,p0=Geo2Mgeo(location)
    mx,my,mz = VectorGTODinZenith (nMx, nMy, nMz,XinA, YinA, ZinA)
    r,t,p=xyz2rthetaphi(mx,my,mz)
    tpcounts, pbin_edges, tbin_edges = np.histogram2d(np.rad2deg(-p), 
                                                      np.rad2deg(np.pi-t), bins=(gpbins,gtbins))
    # aaa=np.sum(tpcounts)    
    return tpcounts


def Gedistrubtiontp_E_Ar_HOrbit(str0,gtbins,gpbins,Erange,ARange,altitude):
    
    a = uproot.open(str0)
    #pPID=a["DetectionInfo"]["pPID"].array(library="np")
    #ParentId=a["DetectionInfo"]["ParentId"].array(library="np")
    E=a["BoundaryInfo"]["Energy"].array(library="np")
    X=a["BoundaryInfo"]["PosX"].array(library="np")
    Y=a["BoundaryInfo"]["PosY"].array(library="np")
    Z=a["BoundaryInfo"]["PosZ"].array(library="np")
    Mx=a["BoundaryInfo"]["DirX"].array(library="np")
    My=a["BoundaryInfo"]["DirY"].array(library="np")
    Mz=a["BoundaryInfo"]["DirZ"].array(library="np")
    
    ide=filterrange(E, Erange[0], Erange[1])
    
    if len(ide)==0:
        return np.zeros((360,180))
    
    Mxi=Mx[ide]
    Myi=My[ide]
    Mzi=Mz[ide]
    Xi0=X[ide]
    Yi0=Y[ide]
    Zi0=Z[ide]
    del X, Y, Z, Mx, My, Mz, E
    
    xyzlocal=ARange[0:3]
    xyz=np.vstack((Xi0,Yi0,Zi0))
    moment=np.vstack((Mxi,Myi,Mzi))
    args_list = []
    # n=0
    for i, location in enumerate(xyz.T): 
        # 同时获取元素序号（索引），和元素值，这个i做为单个循环中的任务ID（task_id）
        # if n>5:
        #     break
        args = (location, moment.T[i], altitude, i)   
        args_list.append(args)
        # n=n+1
    MAX_CONCURRENT = 1   # 根据你的数据库连接限制调整
    with mps.Pool(processes=MAX_CONCURRENT) as pool:
        # starmap 用于每个参数是一个元组，会自动解包传给 main
        # 如果 main 不接受元组而需要多个参数，就用 starmap
        results = pool.starmap(trac2Orbit, args_list)
        
    # print(results)
    print("Process runing finish, Now selecting and saving data. ")
    sorted_new = sorted(results, key=lambda x: x[0],)
    values = np.array([x[1][0] for x in sorted_new])
    xyz_new =values[:,0:3]
    Xi=xyz_new[:,0]
    Yi=xyz_new[:,1]
    Zi=xyz_new[:,2]
    del args, args_list, results, sorted_new, values
    
    idxInA=AreaFilter(xyz,xyzlocal,ARange[3],ARange[4])
    XinA=Xi[idxInA]
    YinA=Yi[idxInA]
    ZinA=Zi[idxInA]
    MxinA=Mxi[idxInA]
    MyinA=Myi[idxInA]
    MzinA=Mzi[idxInA]
    nMx=MxinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMy=MyinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    nMz=MzinA/np.sqrt(MxinA*MxinA+MyinA*MyinA+MzinA*MzinA)
    del Xi,Yi,Zi,Mxi,Myi,Mzi
    
    #这一步是换成磁坐标
    # location=xyz2rthetaphi1(XinA, YinA, ZinA)
    # r0,t0,p0=Geo2Mgeo(location)
    # vector=xyz2rthetaphi1(nMx, nMy, nMz)
    # nMr, nMt, nMp=Geo2MgeoVector(vector,location)
    # XinA, YinA, ZinA=rthetaphi2xyz(r0,t0,p0)
    # nMx, nMy, nMz=rthetaphi2xyz(nMr, nMt, nMp)
    
    mx,my,mz = VectorGTODinZenith (nMx, nMy, nMz,XinA, YinA, ZinA)
    r,t,p=xyz2rthetaphi(mx,my,mz)
    tpcounts, pbin_edges, tbin_edges = np.histogram2d(np.rad2deg(-p), 
                                                      np.rad2deg(np.pi-t), bins=(gpbins,gtbins))
    # aaa=np.sum(tpcounts)    
    return tpcounts

def FluxChange(str1,Flux,x0,x1,Z,m=0):
    if str1 == "R2E" :
        newFlux=Flux*(x1/x0/(Z*Z))
        return newFlux
    elif str1 == "E2R":
        newFlux=Flux*(x1/x0*(Z*Z))
        return newFlux
    elif str1 == "R2Ek":
        newFlux=Flux*((x1+m)/x0/(Z*Z))
        return newFlux
    elif str1 == "Ek2R":
        newFlux=Flux*(x1/(x0+m)*(Z*Z))
        return newFlux            
            

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
    