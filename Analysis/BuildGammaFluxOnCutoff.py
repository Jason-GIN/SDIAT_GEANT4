import pandas as pd
import numpy as np
import uproot
from mypylib import *
from scipy import interpolate
from scipy.integrate import quad
import multiprocessing as mps

import matplotlib
matplotlib.use('TkAgg') 
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import cartopy.crs as ccrs
import cartopy.feature as cfeature
from matplotlib.colors import LogNorm
from mpl_toolkits.mplot3d import Axes3D

plt.rcParams.update({"text.usetex": False,
                     "mathtext.fontset": "stix",       # 可选：设置数学字体为STIX（与Times风格很搭）
                     "font.family": "sans-serif",   # 为所有非数学文本设置字体族（如刻度、标签）
                     "font.sans-serif": ["DejaVu Sans", "Arial", "Helvetica"], # 优先使用的无衬线字体
                     "font.weight": "bold",         # 【关键】为所有非数学文本设置全局加粗！
                     "axes.labelweight": "bold",    # 坐标轴标签也加粗
                     "xtick.labelsize": 15,
                     "ytick.labelsize": 15,
})

def move_figure(f, x, y):
    """将图形窗口 f 的左上角移动到屏幕像素位置 (x, y)"""
    backend = matplotlib.get_backend()
    if backend == 'TkAgg':
        f.canvas.manager.window.wm_geometry("+%d+%d" % (x, y))
    elif backend == 'WXAgg':
        f.canvas.manager.window.SetPosition((x, y))
    else:  # 适用于 QT 和 GTK 等后端
        f.canvas.manager.window.move(x, y)

Ek=[0.022,0.028,0.035,0.043,0.054,0.068,0.085,0.105,
    0.131,0.162,0.201,0.247,0.303,0.37,0.45,0.544,
    0.655,0.785,0.935,1.109,1.309,1.537,1.798,2.095,
    2.431,2.812,3.242,3.728,4.275,4.892,5.586,6.367,
    7.244,8.23,9.338,10.582,11.978,13.547,15.307,17.283,
    19.501,21.99,24.783,27.917,31.435,35.382,39.811,44.78,
    50.356,56.613,63.634,71.511,80.35,90.268
]  #GeV
m=0.938

sp=np.loadtxt('/mnt/d/other work/data/data/_new/能谱SDEMMA/2012-05-13_1AU_proton.dat')

ke=((sp[:,0])**2+(m*m))**0.5-m
spk=myutils.FluxChange("R2Ek",sp[:,1],sp[:,0],ke,1,m)
#spE=myutils.FluxChange("R2E",sp[:,1],sp[:,0],ke,1)

f = interpolate.splrep(ke, spk, s=0)

gtbins=np.arange(0,181,1)
gpbins=np.arange(-180,181,1)
fact=np.arange(1.84,4.01,0.05)
gEbins=np.power(10,fact) # MeV
dE=gEbins[1:]-gEbins[0:-1]

FitArry=np.abs(np.cos(np.deg2rad(gtbins[0:-1])))*np.sin(np.pi/180)
#print(FitArry[90],FitArry[89])
FitArry[90]=FitArry[90]*1e14  # cos(90)-> unlimit, so times 1e14 to make it reasonable.

def CountAlt(filename, Geo, AltR, AreaR, trem,Trange=[0,181],):
    CountOnA=np.array([])
    a= uproot.open(filename)

    #PID=a["DetectionInfo"]["PID"].array(library="np")
    #Parentid=a["DetectionInfo"]["ParentId"].array(library="np")
    x=a["DetectionInfo"]["PosX"].array(library="np")
    y=a["DetectionInfo"]["PosY"].array(library="np")
    z=a["DetectionInfo"]["PosZ"].array(library="np")
    Mx=a["DetectionInfo"]["DirX"].array(library="np")
    My=a["DetectionInfo"]["DirY"].array(library="np")
    Mz=a["DetectionInfo"]["DirZ"].array(library="np")
    E=a["DetectionInfo"]["Energy"].array(library="np")
    SDnum=a["DetectionInfo"]["SDnum"].array(library="np")
    #Tid=a["DetectionInfo"]["TrackID"].array(library="np")

    idxE=np.where(E>=100)[0]
    x1=x[idxE]
    y1=y[idxE]
    z1=z[idxE]
    Mx1=Mx[idxE]
    My1=My[idxE]
    Mz1=Mz[idxE]
    SDnum1=SDnum[idxE]
    for i in range(AltR[0],AltR[1],1):
    
        indxalt=np.where(SDnum1==i)
        ARange=[6471.2, np.radians(90-Geo[0]),np.radians(Geo[1]),AreaR]
        tpcounts=FluxCalculation.FluxCalculationOn_T_P(x1[indxalt],y1[indxalt],z1[indxalt],
                                                                               Mx1[indxalt],My1[indxalt],Mz1[indxalt], 
                                                                               ARange, gtbins, gpbins)
        #print(tpcounts)
        #str1="./data/primary/"+path+"/Pin_"+str(trem)+"_.csv"
        #pd.DataFrame(tpcounts).to_csv(str1, header=False,  index=False)
        CountOnAi=np.sum(np.dot((1/FitArry[Trange[0]:Trange[1]]),tpcounts[:,Trange[0]:Trange[1]].T))
        CountOnA=np.append(CountOnA,CountOnAi)
    
    return CountOnA

def spline_func(x):
    return interpolate.splev(x, f) 

def ReadAndEselect(str0,Erange):#这一步是读文件并分能量
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
    
    ide=FilterModel.filterrange(E, Erange[0], Erange[1])
        
    Mxi=Mx[ide]
    Myi=My[ide]
    Mzi=Mz[ide]
    Xi0=X[ide]
    Yi0=Y[ide]
    Zi0=Z[ide]
    
    return Xi0,Yi0,Zi0,Mxi,Myi,Mzi

def OrigntoTarget(xyz,moment,altitude):#这一步是trace到指定位置（直线传播，解析向量化）
    # 原实现：多进程 + 每个粒子一次 solve_ivp（DOP853），极慢。
    # 现实现：直线与球面交点有解析解，一次性向量化求解，无需多进程。
    X = xyz.T      # (N,3) 初始位置
    V = moment.T   # (N,3) 方向矢量
    XYZ = ToHigherOrbit.trac2Orbit_vec(X, V, altitude)
    return XYZ[:,0], XYZ[:,1], XYZ[:,2]

def DirecS(X, Y, Z,Mx, My, Mz,direction):#这一步是分东西南北方向
    #dlist=["East","West","North","South"]
    #slist=[  0   ,  1   ,   2   ,   3]
    mx,my,mz = RotationModel.VectorGTODinZenith (-Mx, -My, -Mz,X, Y, Z)
    r,t,p=RotationModel.xyz2rthetaphi(mx,my,mz)
    p_deg=np.rad2deg(p)
    if direction==0:
        index=np.where((p_deg >= 45) & (p_deg < 135))
        return X[index], Y[index], Z[index], Mx[index], My[index], Mz[index]
    elif direction==1:
        index=np.where((p_deg >= -135) & (p_deg < -45))
        return X[index], Y[index], Z[index], Mx[index], My[index], Mz[index]
    elif direction==2:
        index=np.where(((p_deg >= -180) & (p_deg <-135))|((p_deg >= 135) & (p_deg <= 180)))
        return X[index], Y[index], Z[index], Mx[index], My[index], Mz[index]
    elif direction==3:
        index=np.where((p_deg >= -45) & (p_deg < 45))
        return X[index], Y[index], Z[index], Mx[index], My[index], Mz[index]
    else:
        return [], [], [], [], [], []

def gridcombine(rigidity,):
    #这个函数用来将cutoff的能量分bin，算出哪些格子属于哪些能量bin
    Rmax=max(rigidity)
    Rmin=min(rigidity)
    # indxt=np.where(rigidity==Rmin)
    # print(indxt,rigidity[indxt])
    n_bins=50
    Rbins=np.geomspace(Rmin, Rmax+0.1, n_bins + 1)
    ix = np.digitize(rigidity, Rbins) - 1 
    valid = (ix >= 0) & (ix < len(Rbins)-1)
    ix=ix[valid]
    # ix_unique,indexl=np.unique(ix, return_index=True)
    # print(ix_unique)
    # print(rigidity[3629], rigidity[3630], rigidity[3631], rigidity[3633])
    s = pd.Series(ix)
    result = {
        val: idx.tolist()
        for val, idx in s.groupby(s).groups.items()
        if len(idx) >= 0
    }
    if len(result) != 50:
        new_result = {i: [] for i in range(50)}
        new_result.update(result)
        return new_result
    else:
        return result

def Calcute_cOsr(t,p,gtbins=gtbins,gpbins=gpbins):
    tpcount,tdege,pedge=np.histogram2d(t,p,bins=(gtbins,gpbins))
    cOsr=np.sum(np.dot(1/FitArry,tpcount))
    return cOsr

def loacoBin(x,y,z,mx,my,mz,rigidity,gtbins,gpbins):
    '''
    # 这一步是将粒子塞cutoff网格（向量化版本，与原始逐 bin 双循环结果一致）
    # 原始实现：50 个刚度 bin × 每个 bin 内的网格单元做 Python 双重循环 + np.append，
    # 这里改为：先算每个粒子落到的网格单元 n_cell = ix*120 + iy，再用 rbm 查其刚度 bin，
    # 最后 np.bincount 按 1/FitArry 加权一次求和，省去 O(bins×cells) 的循环与 np.append。
    '''
    latbin=np.arange(45,-45.1,-3)
    lonbin=np.arange(0,360.1,3)
    r,t,p=RotationModel.xyz2rthetaphi(x,y,z)
    t_deg=np.rad2deg(t)
    p_deg=np.rad2deg(p)
    p_deg = np.mod(p_deg, 360) #这一步很重要，因为坐标转换的p_deg ∈ [-180,180]，会导致数据减少了一大半
    ix = np.digitize(p_deg, lonbin) - 1
    iy = np.digitize(90-t_deg, latbin) - 1
    valid = (ix >= 0) & (ix < len(lonbin)-1) & (iy >= 0) & (iy < len(latbin)-1)
    ix_f, iy_f = ix[valid], iy[valid]
    M = ix_f.size
    if M == 0:
        return np.zeros((1,50))
    # 刚度网格：31 纬 × 120 经 = 3720 个单元；rbm[n] = 单元 n 所属的 50 个刚度 bin 之一
    Rmax=rigidity.max(); Rmin=rigidity.min()
    rbm=np.digitize(rigidity, np.geomspace(Rmin, Rmax+0.1, 51)) - 1
    # 与原始 mask=(ix==i)&(iy==j)（i=n//120, j=n%120）严格等价的单元索引
    n_cell = ix_f + iy_f*120
    keep = n_cell < len(rigidity)
    n_cell = n_cell[keep]
    # 忠实复现原代码：权重所用 theta 取自“未过滤”的 t_deg 前 M 个（原代码 t_deg[indel] 的行为）
    x_used = x[:M][keep]
    y_used = y[:M][keep]
    z_used = z[:M][keep]
    mx_used = mx[:M][keep]
    my_used = my[:M][keep]
    mz_used = mz[:M][keep]
    nmx,nmy,nmz=RotationModel.VectorGTODinZenith(-mx_used,-my_used,-mz_used,x_used,y_used,z_used)
    rm,tm,pm=RotationModel.xyz2rthetaphi(nmx,nmy,nmz)
    tbin = np.digitize(np.rad2deg(tm), gtbins) - 1
    weight = 1.0 / FitArry[tbin]
    cOsr = np.bincount(rbm[n_cell], weights=weight, minlength=50)
    return cOsr.reshape(1, 50)

def loacoBinTest(x,y,z,mx,my,mz,rigidity,gtbins,gpbins):
    cOsr=np.zeros((1,50))
    latbin=np.arange(45,-45.1,-3)
    lonbin=np.arange(0,360.1,3)
    r,t,p=RotationModel.xyz2rthetaphi(x,y,z)
    t_deg=np.rad2deg(t)
    p_deg=np.rad2deg(p)
    p_deg = np.mod(p_deg, 360) #这一步很重要，因为坐标转换的p_deg ∈ [-180,180]，会导致数据减少了一大半
    ix = np.digitize(p_deg, lonbin) - 1
    iy = np.digitize(90-t_deg, latbin) - 1
    valid = (ix >= 0) & (ix < len(lonbin)-1) & (iy >= 0) & (iy < len(latbin)-1)
    ix_f, iy_f = ix[valid], iy[valid]
    if ix_f.size == 0: # 如果没有点在LAT，LON范围内，直接退出
        return cOsr
    results=gridcombine(rigidity)
    for nRbin,result in results.items():
        i=np.array(result)//120
        j=np.array(result)%120
        mask=np.isin(ix, j)&np.isin(iy, i)
        if not any(mask):
            continue
        x_used = x[mask]
        y_used = y[mask]
        z_used = z[mask]
        mx_used = mx[mask]
        my_used = my[mask]
        mz_used = mz[mask]
        nmx,nmy,nmz=RotationModel.VectorGTODinZenith(-mx_used,-my_used,-mz_used,x_used,y_used,z_used)
        rm,tm,pm=RotationModel.xyz2rthetaphi(nmx,nmy,nmz)
        cOsr[0,nRbin]=Calcute_cOsr(tm,pm,gtbins,gpbins)
    return cOsr

def Gedistrubtiontp_E_Ar_HOrbit(str0,Erange,direction,altitude,rigidity): #这是整合函数，把上述所有函数都整合在一起
    Xi0,Yi0,Zi0,Mxi,Myi,Mzi=ReadAndEselect(str0,Erange)# 先读文件并分能量
    if len(Xi0)==0:
        cOsr=0
        return cOsr
    Xi,Yi,Zi=OrigntoTarget(np.vstack((Xi0,Yi0,Zi0)),np.vstack((Mxi,Myi,Mzi)),altitude) # trace到指定位置
    if direction==4: #四个一起算
        cOsr=np.zeros((4,50)) #这玩意的排序是东西北南
        for d in range(direction):
            strdir="/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/Fermi-LAT/2020/Cutoff"+dlist[d]+".txt"
            cut=np.loadtxt(strdir)
            Xi1,Yi1,Zi1,Mxi1,Myi1,Mzi1=DirecS(Xi,Yi,Zi,Mxi,Myi,Mzi,d)
            if len(Xi1)==0:
                continue
            cOsri=loacoBinTest(Xi1,Yi1,Zi1,Mxi1,Myi1,Mzi1,cut[:,2],gtbins,gpbins)
            cOsr[d,:]=cOsri 
        return cOsr
    else:
        Xi1,Yi1,Zi1,Mxi1,Myi1,Mzi1=DirecS(Xi,Yi,Zi,Mxi,Myi,Mzi,direction)  # 分方向
        if len(Xi1)==0:
            cOsr=0
            return cOsr
        # del Xi0,Yi0,Zi0,Mxi,Myi,Mzi # 删掉不用的数据节约内存
        cOsr=loacoBinTest(Xi1,Yi1,Zi1,Mxi1,Myi1,Mzi1,rigidity,gtbins,gpbins) # 填格子和算“粒子数除以立体角”，距离-能谱-还差除以入射粒子，再乘入射能谱（重建）
        return cOsr

def localcount(location,loca,direction,energyr,rigidity,e):# 这一步是算每个模拟的能量bin上的“粒子数除以立体角”，
    #总体的构想是这样的：
    # 先读文件并分能量
    # 分方向
    # trace到指定位置
    # 填格子
    # 算每个格子的能谱
    # 然后就能画图了
    altitude=6371.2+565
    if direction==4: #四个一起算
        BE_Alt=np.zeros((4,50,54-e))
    else:
        BE_Alt=np.zeros((54-e,50))
    
    for i in range(e,54,1):
        str01="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/protondata_"+str(i)+"_b_gamma.root"
        str02="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/seconddata_"+str(i)+"_b_gamma.root"
        str1="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/protondata_"+str(i)+"_primary.root"
        # 这次的值的范围是0.2-0.5GeV，0.5-1GeV，1-2GeV，2-5GeV，5-10GeV，10-20GeV
        BE_Altip=Gedistrubtiontp_E_Ar_HOrbit(str01,energyr,direction,altitude,rigidity)
        BE_Altis=Gedistrubtiontp_E_Ar_HOrbit(str02,energyr,direction,altitude,rigidity)
        BE_Alti=BE_Altip+BE_Altis
        BE_AltP=CountAlt(str1,loca,[99,100],500,i,[0,90])
        if direction==4: #四个一起算
            if BE_AltP.size==0 or BE_AltP[0]==0:
                continue
            else:
                BE_Alt[:,:,i-e]=BE_Alti/BE_AltP
        else:
            if BE_AltP.size==0 or BE_AltP[0]==0:
                continue
            else:
                BE_Alt[i-e,:]=BE_Alti/BE_AltP
            
    
    return BE_Alt #, BE_Alt_P

def testmodel(str0,Erange,direction,altitude):
    Xi0,Yi0,Zi0,Mxi,Myi,Mzi=ReadAndEselect(str0,Erange)# 先读文件并分能量
    if len(Xi0)==0:
        cOsr=0
        return cOsr
    Xi,Yi,Zi=OrigntoTarget(np.vstack((Xi0,Yi0,Zi0)),np.vstack((Mxi,Myi,Mzi)),altitude) # trace到指定位置
    Xi1,Yi1,Zi1,Mxi1,Myi1,Mzi1=DirecS(Xi,Yi,Zi,Mxi,Myi,Mzi,direction)  # 分方向
    return Xi1,Yi1,Zi1,Mxi1,Myi1,Mzi1
    
arrayloca=np.loadtxt("./text.txt")
fnum2 = interpolate.splev(Ek, f)
# result, error = quad(spline_func, ke[0], ke[-1])
cut=np.loadtxt("/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/Fermi-LAT/2020/CutoffWest.txt")
dlist=["East","West","North","South"]
'''
i=0
e=34
location="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
str01="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/protondata_"+str(e)+"_b_gamma.root"
x,y,z,mx,my,mz=testmodel(str01,[200,500],1,6371.2+565)
r,t,p=RotationModel.xyz2rthetaphi(x,y,z)
lon=np.rad2deg(p)
lat=np.rad2deg(np.pi/2-t)
zmx,zmy,zmz=RotationModel.VectorGTODinZenith(mx,my,mz,x,y,z)
fig = plt.figure(figsize=(10, 6))
ax = plt.axes(projection=ccrs.PlateCarree())
ax.set_global()                              # 或 set_extent([...])
ax.add_feature(cfeature.COASTLINE, linewidth=0.5)
# ax.add_feature(cfeature.BORDERS, linewidth=0.3)
ax.scatter(arrayloca[i,0],arrayloca[i,1],color='red',transform=ccrs.PlateCarree(), s=50)
# 注意：传给 quiver 的 lon/lat 是"源坐标"，transform 告诉它这些点在哪
q = ax.quiver(
    lon, lat, zmy, -zmx,
    transform=ccrs.PlateCarree(),   # 数据的坐标参考系
    regrid_shape=30,                # 关键：让 cartopy 在投影后均匀重采样箭头位置
    color='blue',
    scale=6000,                      # 调这个让箭头别太长/太短
    width=0.0025,
)

# 加个参考箭头（wind barb 也可以用 barbs）
# ax.quiverkey(q, X=0.9, Y=1.02, U=6000, label='6000 m/s',
#              labelpos='E', coordinates='axes')

plt.show()
'''
a=0
b=12 #控制arrayloca在哪里截断，
# d=4
energyr=[2000,5000]
'''
for d in range(4,5,1):
    for i in range(a,b,1):
        # if i==3:
        #     continue
        #ARange 是一个5个元素的数组，
        # 前三个是地理位置（r=0（这个东西在这里没有用）, t_star 弧度, t_end 弧度），
        # 第四个是模式选择，磁纬度=>0，地理纬度=>1，第五个也是模式选择，点=0，范围=1
        # ARange=[0, np.radians(61.5), np.radians(118.5),1,1] #EGRET
        # ARange=[0, np.radians(61.5), np.radians(118.5),1,1] # Fermi
        # ARange=[1, np.radians(90-int(arrayloca[i,1])), np.radians(int(arrayloca[i,0])),600,0]
        strlist="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
        localist=[arrayloca[i,1],arrayloca[i,0]]
        print(strlist)
    
        if d!=4:
            strdir="/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/Fermi-LAT/2020/Cutoff"+dlist[d]+".txt"
            cut=np.loadtxt(strdir)
            BE_Alt=localcount(strlist,localist,d,energyr,cut[:,2],int(arrayloca[i,2]))

            Fgamma = np.dot(fnum2[int(arrayloca[i,2]):],BE_Alt)
            ##乘以8是因为入射和记录的粒子之间的面积差了9倍
            #理论上来说还应该除以10000，因为能谱的最后的单位是每平方厘米，每单位立体角，每单位面积，
            #但是由于单位的来源是每个bin要除以dE，dE刚好是按MeV算的，所以不用除1000
    
            strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/BE_Alt_cutoffG"+str(energyr[0])+str(energyr[1])+dlist[d]+"1.csv"
            strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_cutoffG"+str(energyr[0])+str(energyr[1])+dlist[d]+"1.csv"
            pd.DataFrame(BE_Alt).to_csv(strBE_Alt, header=False,  index=False)
            pd.DataFrame(Fgamma).to_csv(strp, header=False,  index=False)
            # print(strBE_Alt)
            print(strp)
        else:
            BE_Alt=localcount(strlist,localist,d,energyr,0,int(arrayloca[i,2]))
            for j in range(d):
                Fgamma = np.dot(fnum2[int(arrayloca[i,2]):],BE_Alt[j,:,:].T)
                strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/BE_Alt_cutoffG"+str(energyr[0])+str(energyr[1])+dlist[j]+".csv"
                strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_cutoffG"+str(energyr[0])+str(energyr[1])+dlist[j]+".csv"
                pd.DataFrame(BE_Alt[j,:,:].T).to_csv(strBE_Alt, header=False,  index=False)
                pd.DataFrame(Fgamma).to_csv(strp, header=False,  index=False)
                # print(strBE_Alt)
                print(strp)
        # if i==0:
        #     Fgammat=Fgamma
        #     #FPprotont=FPproton/len(elist)
        # else:
        #     Fgammat=Fgammat+Fgamma/n
'''


F=np.zeros((4,50))
for i in range(a,b,1):
    # if i==3:
    #     continue
    strlist="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
    for d in range(0,4,1):
        #strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist[i]+"/BE_Alt.csv"
        strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_cutoffG"+str(energyr[0])+str(energyr[1])+dlist[d]+".csv"
        Fgamma= pd.DataFrame(pd.read_csv(strp, 
                                header = None,)).to_numpy()
    
        # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_120g2001000_test1.csv"
        # Fgamma_test= pd.DataFrame(pd.read_csv(strp, 
        #                         header = None,)).to_numpy()

        # if i==a:
        #     Fgammat=Fgamma/(b-a)
        #     # Fgammat_test=Fgamma_test/n
        #     #FPprotont=FPproton/len(elist)
        # else:
        #     Fgammat=Fgammat+Fgamma/(b-a)
        #     # Fgammat_test=Fgammat_test+Fgamma_test/n
        #     #FPprotont=FPprotont+FPproton/6    
        F[d,:]=F[d,:]+Fgamma[:,0]/(b-a)*((energyr[1]-energyr[0])*1e-3)
        # F[d,:]=Fgamma[:,0]
    '''
    fig=plt.figure(1,figsize=(12, 6))
    # dlist=["East","West","North","South"]
    colorlist=['black','blue','yellow','red']
    for j in range(4):
        strdir="/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/Fermi-LAT/2020/Cutoff"+dlist[j]+".txt"
        cut=np.loadtxt(strdir)
        rmax=max(cut[:,2])
        rmin=min(cut[:,2])
        n_bins=50
        rbins=np.geomspace(rmin, rmax+0.1, n_bins + 1)
        plt.stairs(F[j,:], rbins, fill=False, color=colorlist[j], linewidth=2,label="simulation")
    # plt.stairs(Fgammat[:,0], rbins, fill=False, color='blue', linewidth=2,label="simulation")
    # plt.stairs(Fgammat, rbins, fill=False, color='blue', linewidth=2,label="simulation")
    # plt.stairs(Fgamma, rbins, fill=False, color='blue', linewidth=2,label="simulation")

    ax=plt.gca()
    ax.spines['bottom'].set_linewidth(2)  
    ax.spines['left'].set_linewidth(2)    
    ax.spines['top'].set_linewidth(2)     
    ax.spines['right'].set_linewidth(2)   
    ax.xaxis.set_tick_params(which="both",width=1.5)
    ax.yaxis.set_tick_params(which="both",width=1.5)
    #ax.spines['bottom'].set_color('red')  # 设置底部轴线颜色为红色
    #ax.spines['left'].set_color('red')    # 设置左侧轴线颜色为红色

    plt.xscale("log")
    # xt=list([3,4,5,6,7,8,9,10,20,30,40,50,60,70,80])
    # plt.xticks(xt, ['3','4','5','6','7','8','9','10','20','30','40','50','60','70','80'])
    # plt.xlim(3,80)

    plt.yscale("log")
    # yt=list([3,5,10,15,20,25,30,35,40,45,50])
    # plt.yticks(yt, ['3','5','10','15','20','25','30','35','40','45','50'])
    # plt.ylim(3,50)

    ax.set_xlabel(r'Rigidity $[\mathrm{GV}]$',family='Times New Roman',fontsize=20)
    ax.set_ylabel(r'Flux $[\mathrm{(cm^{2}~sec~srt)^{-1}}]$',family='Times New Roman',fontsize=20)
    #ax.legend(frameon=False, prop={'size': 12})   
    strsave="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/figure/gammacutoff"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))+".jpg"
    plt.savefig(strsave)    
    plt.close()
    # move_figure(fig, 200, 1000)
    # plt.show()
    '''

data=np.loadtxt("/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/Fermi-LAT/2020/Figure4(d).txt")
yerror=  np.vstack((data[:,2]+data[:,3],data[:,2]+data[:,3]))
fig=plt.figure(1,figsize=(12, 6))
# dlist=["East","West","North","South"]
colorlist=['black','blue','orange','red']
for i in range(4):
    strdir="/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/Fermi-LAT/2020/Cutoff"+dlist[i]+".txt"
    cut=np.loadtxt(strdir)
    rmax=max(cut[:,2])
    rmin=min(cut[:,2])
    n_bins=50
    rbins=np.geomspace(rmin, rmax+0.1, n_bins + 1)
    plt.stairs(F[i,:]/100, rbins, fill=False, color=colorlist[i], linewidth=2,label="simulation")
# plt.stairs(Fgammat[:,0], rbins, fill=False, color='blue', linewidth=2,label="simulation")
# plt.stairs(Fgammat, rbins, fill=False, color='blue', linewidth=2,label="simulation")
# plt.stairs(Fgamma, rbins, fill=False, color='blue', linewidth=2,label="simulation")

plt.errorbar(data[:,0], data[:,1], yerr=yerror, fmt='o',color='red', 
                ecolor='red', capsize=2, elinewidth =2)

ax=plt.gca()
ax.spines['bottom'].set_linewidth(2)  
ax.spines['left'].set_linewidth(2)    
ax.spines['top'].set_linewidth(2)     
ax.spines['right'].set_linewidth(2)   
ax.xaxis.set_tick_params(which="both",width=1.5)
ax.yaxis.set_tick_params(which="both",width=1.5)
#ax.spines['bottom'].set_color('red')  # 设置底部轴线颜色为红色
#ax.spines['left'].set_color('red')    # 设置左侧轴线颜色为红色

# ax.text(90, 0.002, r'Fermi-LAT',
#          fontsize=15, color='red', ha='center', rotation=0)

# ax.text(-110, 0.004, r'Simulation',
#          fontsize=15, color='black', ha='center', rotation=0)

plt.xscale("log")
xt=list([3,4,5,6,7,8,9,10,20,30,40,50,60,70,80])
plt.xticks(xt, ['3','4','5','6','7','8','9','10','20','30','40','50','60','70','80'])
plt.xlim(3,80)

plt.yscale("log")
# yt=list([3,5,10,15,20,25,30,35,40,45,50])
# plt.yticks(yt, ['3','5','10','15','20','25','30','35','40','45','50'])
plt.ylim(1e-4,3e-3)

ax.set_xlabel(r'Rigidity $[\mathrm{GV}]$',family='Times New Roman',fontsize=20)
ax.set_ylabel(r'Flux $[\mathrm{(cm^{2}~sec~srt)^{-1}}]$',family='Times New Roman',fontsize=20)
#ax.legend(frameon=False, prop={'size': 12})     
# plt.savefig('/mnt/e/会/COSPAR2026/图/gammacp.svg')    
# move_figure(fig, 200, 1000)
plt.show()
