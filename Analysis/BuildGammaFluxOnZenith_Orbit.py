import pandas as pd
import numpy as np
import uproot
from mypylib import *
from scipy import interpolate
from scipy.integrate import quad

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

def CountE(filename, Geo, AltR,AreaR,Emax, Trange=[0,181]):
    CountOnE=np.array([])
    a= uproot.open(filename)

    #PID=a["DetectionInfo"]["PID"].array(library="np")
    #Parentid=a["DetectionInfo"]["ParentId"].array(library="np")
    x=a["HitData"]["PosX"].array(library="np")
    y=a["HitData"]["PosY"].array(library="np")
    z=a["HitData"]["PosZ"].array(library="np")
    Mx=a["HitData"]["DirX"].array(library="np")
    My=a["HitData"]["DirY"].array(library="np")
    Mz=a["HitData"]["DirZ"].array(library="np")
    E=a["HitData"]["Energy"].array(library="np")
    #Tid=a["DetectionInfo"]["TrackID"].array(library="np")

    idxE=np.where((E>=30) & (E < Emax*.97))[0]
    x1=x[idxE]
    y1=y[idxE]
    z1=z[idxE]
    Mx1=Mx[idxE]
    My1=My[idxE]
    Mz1=Mz[idxE]
    #select Altitude
    tpecounts=FluxCalculation.FluxCalculationOn_T_P_E(x1,y1,z1,
                                                     Mx1,My1,Mz1, 
                                                     E[idxE],AreaR, gtbins, gpbins, gEbins)
    for i in range(len(gEbins)-1):  
        #print(tpcounts)
        CountOnEi=np.sum(np.dot((1/FitArry[Trange[0]:Trange[1]]),tpecounts[:,Trange[0]:Trange[1],i].T))
        CountOnE=np.append(CountOnE,CountOnEi)
    
    return CountOnE

def spline_func(x):
    return interpolate.splev(x, f) 


def localcount(location,loca,ARange,e,):
    
    BE_Alt_T=np.array([])
    BE_Alt_E=np.array([])
    BE_Alt_S=np.array([])
    BE_Alt_N=np.array([])
    BE_Alt_W=np.array([])
    # BE_Alt_P=np.array([])
    for i in range(e,54,1):
        altitude=6371.2+565
        str01="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/protondata_"+str(i)+"_b_gamma.root"
        str02="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/seconddata_"+str(i)+"_b_gamma.root"
        str1="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/protondata_"+str(i)+"_primary.root"
        BE_Altip=myutils.Gedistrubtiontp_E_Ar_HOrbit(str01,gtbins,gpbins,[100,300],ARange,altitude)
        BE_Altis=myutils.Gedistrubtiontp_E_Ar_HOrbit(str02,gtbins,gpbins,[100,300],ARange,altitude)
        # BE_Alti=np.dot((BE_Altip+BE_Altis)[:,120:125],1/(FitArry[120:125].T)) #ERGET
        # BE_Altip=myutils.Gedistrubtiontp_E_Ar(str01,gtbins,gpbins,[200,1000],ARange)
        # BE_Altis=myutils.Gedistrubtiontp_E_Ar(str02,gtbins,gpbins,[200,1000],ARange)
        
        # aaa=(BE_Altip+BE_Altis)[-45:0,:]
        aT=np.sum((BE_Altip+BE_Altis),axis=0) #total
        aS=np.sum((BE_Altip+BE_Altis)[0:45,:],axis=0)+np.sum((BE_Altip+BE_Altis)[315:,:],axis=0) #south
        aE=np.sum((BE_Altip+BE_Altis)[45:135,:],axis=0) #east
        aN=np.sum((BE_Altip+BE_Altis)[135:225,:],axis=0) #north
        aW=np.sum((BE_Altip+BE_Altis)[225:315,:],axis=0) #west
        # 实际你这一步不对，因为phi是从-180到180的，所以phi 方向的第一个值（pih[0]）是北，
        # 即[0:45]+[315:]实际上在角度上是[135:180]+[-180:-135]，
        # 剩下的以此类推，所以aS实际是北，aE实际是西，aN实际是南，aW实际是东
        # 但是我懒得改了，后面画图的时候知道实际的对应关系，改过来了。
        
        
        BE_Alti_T=aT/FitArry 
        BE_Alti_E=aE/FitArry 
        BE_Alti_N=aN/FitArry 
        BE_Alti_S=aS/FitArry 
        BE_Alti_W=aW/FitArry 
        BE_AltP=CountAlt(str1,loca,[99,100],500,i,[0,90])
        # BE_AltPi=CountAlt(str1,loca,[99,100],500,i,[0,45])
    
        if len(BE_Alt_T)==0:
            if BE_AltP==0:
                BE_Alt_T=np.zeros(len(gpbins)-1)
                BE_Alt_E=np.zeros(len(gpbins)-1)
                BE_Alt_N=np.zeros(len(gpbins)-1)
                BE_Alt_S=np.zeros(len(gpbins)-1)
                BE_Alt_W=np.zeros(len(gpbins)-1)
            else:
                BE_Alt_T=BE_Alti_T/BE_AltP
                BE_Alt_E=BE_Alti_E/BE_AltP
                BE_Alt_N=BE_Alti_N/BE_AltP
                BE_Alt_S=BE_Alti_S/BE_AltP
                BE_Alt_W=BE_Alti_W/BE_AltP
            # BE_Alt_P=BE_AltPi
        else:
            if BE_AltP==0:
                BE_Alts=np.zeros(len(gpbins)-1)
                BE_Alt_T=np.vstack((BE_Alt_T,BE_Alts))
                BE_Alt_E=np.vstack((BE_Alt_E,BE_Alts))
                BE_Alt_N=np.vstack((BE_Alt_N,BE_Alts))
                BE_Alt_S=np.vstack((BE_Alt_S,BE_Alts))
                BE_Alt_W=np.vstack((BE_Alt_W,BE_Alts))
            else:
                BE_Alt_T=np.vstack((BE_Alt_T,BE_Alti_T/BE_AltP))
                BE_Alt_E=np.vstack((BE_Alt_E,BE_Alti_E/BE_AltP))
                BE_Alt_N=np.vstack((BE_Alt_N,BE_Alti_N/BE_AltP))
                BE_Alt_S=np.vstack((BE_Alt_S,BE_Alti_S/BE_AltP))
                BE_Alt_W=np.vstack((BE_Alt_W,BE_Alti_W/BE_AltP))
            # BE_Alt_P=np.vstack((BE_Alt_P,BE_AltPi))
    
    return BE_Alt_T, BE_Alt_E, BE_Alt_N, BE_Alt_S, BE_Alt_W

arrayloca=np.loadtxt("./text.txt")

n=12 #控制arrayloca在哪里截断，

'''
for i in range(0,n,1):
    # if i==3:
    #     continue
    #ARange 是一个5个元素的数组，
    # 前三个是地理位置（r=0（这个东西在这里没有用）, t_star 弧度, t_end 弧度），
    # 第四个是模式选择，磁纬度=>0，地理纬度=>1，第五个也是模式选择，点=0，范围=1
    # ARange=[0, np.radians(61.5), np.radians(118.5),1,1] #EGRET
    ARange=[0, np.radians(61.5), np.radians(118.5),1,1] # Fermi
    # ARange=[1, np.radians(90-int(arrayloca[i,1])), np.radians(int(arrayloca[i,0])),600,0]
    strlist="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
    localist=[arrayloca[i,1],arrayloca[i,0]]
    print(strlist)
    BE_Alt_T, BE_Alt_E, BE_Alt_N, BE_Alt_S, BE_Alt_W=localcount(strlist,localist,ARange,int(arrayloca[i,2]))
    
    fnum2 = interpolate.splev(Ek[int(arrayloca[i,2]):], f)
    # result, error = quad(spline_func, ke[0], ke[-1])

    Fgamma_T = np.dot(fnum2,BE_Alt_T)
    Fgamma_E = np.dot(fnum2,BE_Alt_E)
    Fgamma_N = np.dot(fnum2,BE_Alt_N)
    Fgamma_S = np.dot(fnum2,BE_Alt_S)
    Fgamma_W = np.dot(fnum2,BE_Alt_W)

    ##乘以8是因为入射和记录的粒子之间的面积差了9倍
    #理论上来说还应该除以1000，因为能谱的最后的单位是每平方米，每单位立体角，每单位面积，每MeV
    #但是由于单位的来源是每个bin要除以dE，dE刚好是按MeV算的，所以不用除1000
    
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_south.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_east.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_north.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_west.csv"
    
    strp_t="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_total.csv"
    strp_s="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_south.csv"
    strp_e="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_east.csv"
    strp_n="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_north.csv"
    strp_w="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_west.csv"
    
    pd.DataFrame(Fgamma_T).to_csv(strp_t, header=False,  index=False)
    pd.DataFrame(Fgamma_N).to_csv(strp_n, header=False,  index=False)
    pd.DataFrame(Fgamma_E).to_csv(strp_e, header=False,  index=False)
    pd.DataFrame(Fgamma_S).to_csv(strp_s, header=False,  index=False)
    pd.DataFrame(Fgamma_W).to_csv(strp_w, header=False,  index=False)
    # print(strBE_Alt)
    # print(strp)
    # if i==47:
    #     Fgammat=Fgamma
    #     #FPprotont=FPproton/len(elist)
    # else:
    #     Fgammat=Fgammat+Fgamma
'''

a=1
b=2
n=b-a
'''
for i in range(a,b,1):
    if i==3:
        continue
    strlist="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
    #strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist[i]+"/BE_Alt.csv"
    
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_south.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_east.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_north.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_west.csv"
    
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_south.csv"
    strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_east.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_north.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_west.csv"
    
    Fgamma= pd.DataFrame(pd.read_csv(strp, 
                            header = None,)).to_numpy()
    
    #NPproton = pd.DataFrame(pd.read_csv("/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/gamma/Fermi/25_-160/data/PNin.csv", 
    #                        header = None,)).to_numpy()
    #NPproton=BE_Alt_P
    #zz=NPproton.T*fnum2
    #FPproton = NPproton.T[0]*fnum2/1e6/result
    
    if i==a:
        Fgammat=Fgamma/n/4
        #FPprotont=FPproton/len(elist)
    else:
        Fgammat=Fgammat+Fgamma/n/4
        #FPprotont=FPprotont+FPproton/6    
'''
for i in range(a,b,1):
    if i==3:
        continue
    strlist="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
    #strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist[i]+"/BE_Alt.csv"
    
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_south.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_east.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_north.csv"
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg2001000_west.csv"
    
    strps="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_south.csv"
    strpe="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_east.csv"
    strpn="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_north.csv"
    strpw="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_Zenithg100300_west.csv"
    
    Fgammas= pd.DataFrame(pd.read_csv(strps,header = None,)).to_numpy()
    Fgammae= pd.DataFrame(pd.read_csv(strpe,header = None,)).to_numpy()
    Fgamman= pd.DataFrame(pd.read_csv(strpn,header = None,)).to_numpy()
    Fgammaw= pd.DataFrame(pd.read_csv(strpw,header = None,)).to_numpy()
    
    #NPproton = pd.DataFrame(pd.read_csv("/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/gamma/Fermi/25_-160/data/PNin.csv", 
    #                        header = None,)).to_numpy()
    #NPproton=BE_Alt_P
    #zz=NPproton.T*fnum2
    #FPproton = NPproton.T[0]*fnum2/1e6/result
    
    if i==a:
        Fgammats=Fgammas/n*0.2
        Fgammate=Fgammae/n*0.2
        Fgammatn=Fgamman/n*0.2
        Fgammatw=Fgammaw/n*0.2
        #FPprotont=FPproton/len(elist)
    else:
        Fgammats=Fgammats+Fgammas/n*0.2
        Fgammate=Fgammate+Fgammae/n*0.2
        Fgammatn=Fgammatn+Fgamman/n*0.2
        Fgammatw=Fgammatw+Fgammaw/n*0.2
        #FPprotont=FPprotont+FPproton/6    


# data=pd.DataFrame(pd.read_excel("/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/Fermi-LAT/能谱方.xlsx", 
#                                 )).to_numpy()
# x=np.hstack((data[0:18,0]+90,data[18:72,0]-270))


data=pd.DataFrame(pd.read_excel("/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/EGRET/能天.xlsx", 
                                )).to_numpy()

newttfs=[]
newttfe=[]
newttfn=[]
newttfw=[]
newpbins=np.arange(0,185,5)
for j in range(0,len(Fgammats),5):
    newttfs=np.append(newttfs,np.sum(Fgammats[j:j+5]))
    newttfe=np.append(newttfe,np.sum(Fgammate[j:j+5]))
    newttfn=np.append(newttfn,np.sum(Fgammatn[j:j+5]))
    newttfw=np.append(newttfw,np.sum(Fgammatw[j:j+5]))
# newttf1=np.hstack((newttf[9:],newttf[0:9])) 

newf=np.vstack((newttfe,newttfw,newttfn,newttfs))
axlist=['0,0', '0,1', '1,0', '1,1']
latlist=["West", "East", "South", "North"]
fig, ax1 = plt.subplots(2, 2, figsize=(12, 10), dpi=100, sharex=True, 
                              gridspec_kw={'height_ratios': [1,1], "width_ratios":[1,1]}, layout='constrained')
for j in range(4):
    a=int(axlist[j].split(',')[0])
    b=int(axlist[j].split(',')[1])
    ax1[a,b].stairs(newf[j,:]/5/100, newpbins, fill=False, color='blue', linewidth=2 ) #18 comes from the Zenith
    ax1[a,b].scatter(data[1:,(0+j*3)],data[1:,(1+j*3)],color="black")
    ax1[a,b].set_yscale("log")
    ax1[a,b].set_xlim(60, 181)
    ax1[a,b].set_ylim(1e-5, 2e-1)
    ax1[a,b].spines['bottom'].set_linewidth(2)  
    ax1[a,b].spines['left'].set_linewidth(2)    
    ax1[a,b].spines['top'].set_linewidth(2)     
    ax1[a,b].spines['right'].set_linewidth(2)   
    ax1[a,b].xaxis.set_tick_params(which="both",width=1.0)
    ax1[a,b].yaxis.set_tick_params(which="both",width=1.0)
    ax1[a,b].set_ylabel(r'Flux $[\mathrm{(cm^{2}~sec~srt)^{-1}}]$',family='Times New Roman',fontsize=20)
    #ax1[a,b].set_xticks(xt)
    ax1[a,b].text(153.8, 0.01,latlist[j],
         fontsize=20, color='black', ha='center', rotation=0)
ax1[1,0].set_xlabel(r'Zenith $[\mathrm{Deg}]$',family='Times New Roman',fontsize=20)
ax1[1,1].set_xlabel(r'Zenith $[\mathrm{Deg}]$',family='Times New Roman',fontsize=20)

# plt.show()


fig=plt.figure(2,figsize=(8, 9))


plt.stairs(newttfs/5/20, newpbins, fill=False, color='yellow', linewidth=2)   #n
plt.stairs(newttfe/5/20, newpbins, fill=False, color='red', linewidth=2)  #w
plt.stairs(newttfn/5/20, newpbins, fill=False, color='black', linewidth=2)    #s
plt.stairs(newttfw/5/20, newpbins, fill=False, color='blue', linewidth=2) #e
# plt.scatter(data[2:21,3],data[2:21,4], marker='o', facecolors='black', edgecolors='black',linewidths=2,s=50)
plt.yscale('log')
ax=plt.gca()
ax.spines['bottom'].set_linewidth(2)  
ax.spines['left'].set_linewidth(2)    
ax.spines['top'].set_linewidth(2)     
ax.spines['right'].set_linewidth(2)   
ax.xaxis.set_tick_params(which="both",width=1.5)
ax.yaxis.set_tick_params(which="both",width=1.5)
#ax.spines['bottom'].set_color('red')  # 设置底部轴线颜色为红色
#ax.spines['left'].set_color('red')    # 设置左侧轴线颜色为红色
xt=list(range(60,181,30))

# ax.text(90, 0.002, r'EGRET',
#          fontsize=15, color='red', ha='center', rotation=0)

# ax.text(-110, 0.004, r'Simulation',
#          fontsize=15, color='blue', ha='center', rotation=0)


plt.xticks(xt, ['60','90','120','150','180'])
plt.xlim(60,181)
# plt.yscale("log")
#plt.ylim(1e-5,3e-2)

ax.set_xlabel(r'Azimuth $[\mathrm{Deg}]$',family='Times New Roman',fontsize=20)
ax.set_ylabel(r'Flux $[\mathrm{(cm^{2}~sec~srt)^{-1}}]$',family='Times New Roman',fontsize=20)
#ax.legend(frameon=False, prop={'size': 12})     
# plt.savefig('/mnt/e/会/COSPAR2026/图/gammacp.svg')    
# move_figure(fig, 200, 1000)
plt.show()
