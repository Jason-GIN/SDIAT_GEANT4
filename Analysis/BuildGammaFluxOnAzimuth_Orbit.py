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


def localcount(location,loca,ARange,e):
    
    BE_Alt=np.array([])
    # BE_Alt_P=np.array([])
    for i in range(e,54,1):
        altitude=6371.2+565
        str01="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/protondata_"+str(i)+"_b_gamma.root"
        str02="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/seconddata_"+str(i)+"_b_gamma.root"
        str1="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/protondata_"+str(i)+"_primary.root"
        # BE_Altip=myutils.Gedistrubtiontp_E_Ar(str0,gtbins,gpbins,[100,300],ARange)
        # BE_Altis=myutils.Gedistrubtiontp_E_Ar(str0,gtbins,gpbins,[100,300],ARange)
        # BE_Alti=np.dot((BE_Altip+BE_Altis)[:,120:125],1/(FitArry[120:125].T)) #ERGET
        BE_Altip=myutils.Gedistrubtiontp_E_Ar_HOrbit(str01,gtbins,gpbins,[200,1000],ARange,altitude)
        BE_Altis=myutils.Gedistrubtiontp_E_Ar_HOrbit(str02,gtbins,gpbins,[200,1000],ARange,altitude)
        BE_Alti=np.dot((BE_Altip+BE_Altis)[:,120:135],1/(FitArry[120:135].T)) #Fermi
        BE_AltP=CountAlt(str1,loca,[99,100],500,i,[0,90])
        # BE_AltPi=CountAlt(str1,loca,[99,100],500,i,[0,45])
    
        if len(BE_Alt)==0:
            if BE_AltP==0:
                BE_Alt=np.zeros(len(gpbins)-1)
            else:
                BE_Alt=BE_Alti/BE_AltP
            # BE_Alt_P=BE_AltPi
        else:
            if BE_AltP==0:
                BE_Alts=np.zeros(len(gpbins)-1)
                BE_Alt=np.vstack((BE_Alt,BE_Alts))
            else:
                BE_Alt=np.vstack((BE_Alt,BE_Alti/BE_AltP))
            # BE_Alt_P=np.vstack((BE_Alt_P,BE_AltPi))
    
    return BE_Alt #, BE_Alt_P

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
    BE_Alt=localcount(strlist,localist,ARange,int(arrayloca[i,2]))
    
    fnum2 = interpolate.splev(Ek[int(arrayloca[i,2]):], f)
    result, error = quad(spline_func, ke[0], ke[-1])

    Fgamma = np.dot(fnum2,BE_Alt)
    ##乘以8是因为入射和记录的粒子之间的面积差了9倍
    #理论上来说还应该除以1000，因为能谱的最后的单位是每平方米，每单位立体角，每单位面积，每MeV
    #但是由于单位的来源是每个bin要除以dE，dE刚好是按MeV算的，所以不用除1000
    
    strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/BE_Alt_120g2001000_test1.csv"
    strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_120g2001000_test1.csv"
    pd.DataFrame(BE_Alt).to_csv(strBE_Alt, header=False,  index=False)
    pd.DataFrame(Fgamma).to_csv(strp, header=False,  index=False)
    print(strBE_Alt)
    print(strp)
    # if i==0:
    #     Fgammat=Fgamma
    #     #FPprotont=FPproton/len(elist)
    # else:
    #     Fgammat=Fgammat+Fgamma
'''
'''
for i in range(5,n,1):
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
    BE_Alt=localcount(strlist,localist,ARange,int(arrayloca[i,2]))
    
    fnum2 = interpolate.splev(Ek[int(arrayloca[i,2]):], f)
    result, error = quad(spline_func, ke[0], ke[-1])

    Fgamma = np.dot(fnum2,BE_Alt)
    ##乘以8是因为入射和记录的粒子之间的面积差了9倍
    #理论上来说还应该除以1000，因为能谱的最后的单位是每平方米，每单位立体角，每单位面积，每MeV
    #但是由于单位的来源是每个bin要除以dE，dE刚好是按MeV算的，所以不用除1000
    
    strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/BE_Alt_120g2001000.csv"
    strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_120g2001000.csv"
    pd.DataFrame(BE_Alt).to_csv(strBE_Alt, header=False,  index=False)
    pd.DataFrame(Fgamma).to_csv(strp, header=False,  index=False)
    # print(strBE_Alt)
    # print(strp)
    # if i==47:
    #     Fgammat=Fgamma
    #     #FPprotont=FPproton/len(elist)
    # else:
    #     Fgammat=Fgammat+Fgamma
'''
a=0
b=12
# n=b-a
'''
for i in range(a,b,1):
    # if i==32:
    #     continue
    strlist="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
    #strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist[i]+"/BE_Alt.csv"
    strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_120g100300.csv"
        
    Fgamma= pd.DataFrame(pd.read_csv(strp, 
                            header = None,)).to_numpy()
    
    #NPproton = pd.DataFrame(pd.read_csv("/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/gamma/Fermi/25_-160/data/PNin.csv", 
    #                        header = None,)).to_numpy()
    #NPproton=BE_Alt_P
    #zz=NPproton.T*fnum2
    #FPproton = NPproton.T[0]*fnum2/1e6/result
        
    if i==a:
        Fgammat=Fgamma/n/6
        #FPprotont=FPproton/len(elist)
    else:
        Fgammat=Fgammat+Fgamma/n/6
        #FPprotont=FPprotont+FPproton/6
    newpbins=np.arange(-180,184,4)
    newttf=[]
    for j in range(0,len(Fgammat),4):
        newttf=np.append(newttf,np.sum(Fgammat[j:j+4,0]))
    plt.figure(1,figsize=(15, 6))
    # plt.stairs(Fgammat[:,0], gpbins, fill=False, color='blue', linewidth=2,label="simulation")
    plt.stairs(newttf/4, newpbins, fill=False, color='blue', linewidth=2)
    # plt.errorbar(x, data[0:72,2], yerr=data[0:72,5:7].T, fmt='o',color='red', 
    #                 ecolor='r', capsize=5, elinewidth =2,label="EGRET")
    ax=plt.gca()
    ax.spines['bottom'].set_linewidth(2)  
    ax.spines['left'].set_linewidth(2)    
    ax.spines['top'].set_linewidth(2)     
    ax.spines['right'].set_linewidth(2)   
    ax.xaxis.set_tick_params(which="both",width=1.5)
    ax.yaxis.set_tick_params(which="both",width=1.5)
    #ax.spines['bottom'].set_color('red')  # 设置底部轴线颜色为红色
    #ax.spines['left'].set_color('red')    # 设置左侧轴线颜色为红色
    xt=list(range(-180,181,30))

    plt.xticks(xt, ['-180','-150','-120','-90','-60','-30','0','30','60','90','120','150','180'])
    plt.xlim(-181,181)

    #plt.ylim(1e-5,3e-2)
    
    ax.set_xlabel(r'Azimuth $[\mathrm{Deg}]$',family='Times New Roman',fontsize=20)
    ax.set_ylabel(r'Flux $[\mathrm{(cm^{2}~sec~srt)^{-1}}]$',family='Times New Roman',fontsize=20)
    #ax.legend(frameon=False, prop={'size': 12}) 
    str0="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/figure/gammaflux/fermi/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))+".jpg"   
    plt.savefig(str0) 
    plt.clf()
'''


for i in range(a,b,1):
    # if i==3:
    #     continue
    strlist="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
    #strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist[i]+"/BE_Alt.csv"
    strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_120g2001000.csv"
    Fgamma= pd.DataFrame(pd.read_csv(strp, 
                            header = None,)).to_numpy()
    
    # strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_120g2001000_test1.csv"
    # Fgamma_test= pd.DataFrame(pd.read_csv(strp, 
    #                         header = None,)).to_numpy()
    
    
    if i==a:
        Fgammat=Fgamma/n*0.8
        # Fgammat_test=Fgamma_test/n
        #FPprotont=FPproton/len(elist)
    else:
        Fgammat=Fgammat+Fgamma/n*0.8
        # Fgammat_test=Fgammat_test+Fgamma_test/n
        #FPprotont=FPprotont+FPproton/6    

# data=pd.DataFrame(pd.read_excel("/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/Fermi-LAT/能谱方.xlsx", 
#                                 )).to_numpy()
# x=np.hstack((data[0:18,0]+90,data[18:72,0]-270))

data=pd.DataFrame(pd.read_excel("/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/Fermi-LAT/能谱方.xlsx", 
                                )).to_numpy()
x=180-data[1:73,0]

newttf=[]
newttf_test=[]
newpbins=np.arange(-180,185,5)
for j in range(0,len(Fgammat),5):
        newttf=np.append(newttf,np.sum(Fgammat[j:j+5]))
        # newttf_test=np.append(newttf_test,np.sum(Fgammat_test[j:j+5]))
# newttf1=np.hstack((newttf[9:],newttf[0:9])) 

fig=plt.figure(1,figsize=(15, 6))

# plt.stairs(Fgammat, gpbins, fill=False, color='blue', linewidth=2,label="simulation")
# plt.stairs(Fgammat[:,0], gpbins, fill=False, color='blue', linewidth=2,label="simulation")
plt.stairs(newttf/5/12, newpbins, fill=False, color='blue', linewidth=2)
# plt.stairs(newttf_test/5/13, newpbins, fill=False, color='black', linewidth=2)
# plt.stairs(newttf1/10, -newpbins, fill=False, color='blue', linewidth=2)
# plt.errorbar(x, data[0:72,2], yerr=data[0:72,5:7].T, fmt='o',color='red', 
#                 ecolor='r', capsize=5, elinewidth =2,label="EGRET")
plt.scatter(x,data[1:73,2], marker='o', facecolors='none', edgecolors='red',linewidths=2,s=100)
ax=plt.gca()
ax.spines['bottom'].set_linewidth(2)  
ax.spines['left'].set_linewidth(2)    
ax.spines['top'].set_linewidth(2)     
ax.spines['right'].set_linewidth(2)   
ax.xaxis.set_tick_params(which="both",width=1.5)
ax.yaxis.set_tick_params(which="both",width=1.5)
#ax.spines['bottom'].set_color('red')  # 设置底部轴线颜色为红色
#ax.spines['left'].set_color('red')    # 设置左侧轴线颜色为红色
xt=list(range(-180,181,30))

ax.text(90, 0.002, r'Fermi-LAT',
         fontsize=15, color='red', ha='center', rotation=0)

ax.text(-110, 0.004, r'Simulation',
         fontsize=15, color='black', ha='center', rotation=0)

ax.text(-180, 0.0002, r'North',
         fontsize=15, color='black', ha='center', rotation=0)
ax.text(-90, 0.0002, r'West',
         fontsize=15, color='black', ha='center', rotation=0)
ax.text(0, 0.0002, r'South',
         fontsize=15, color='black', ha='center', rotation=0)
ax.text(90, 0.0002, r'East',
         fontsize=15, color='black', ha='center', rotation=0)

plt.xticks(xt, ['-180','-150','-120','-90','-60','-30','0','30','60','90','120','150','180'])
plt.xlim(-181,181)
# plt.yscale("log")
#plt.ylim(1e-5,3e-2)

ax.set_xlabel(r'Azimuth $[\mathrm{Deg}]$',family='Times New Roman',fontsize=20)
ax.set_ylabel(r'Flux $[\mathrm{(cm^{2}~sec~srt)^{-1}}]$',family='Times New Roman',fontsize=20)
#ax.legend(frameon=False, prop={'size': 12})     
# plt.savefig('/mnt/e/会/COSPAR2026/图/gammacp.svg')    
# move_figure(fig, 200, 1000)
plt.show()
