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

fact=np.arange(1.48,4.01,0.05)
gEbins=np.power(10,fact) # MeV
dE=gEbins[1:]-gEbins[0:-1]

# fact=np.arange(1.84,4.01,0.05)
# gEbins=np.power(10,fact) # MeV
# dE=gEbins[1:]-gEbins[0:-1]

FitArry=np.abs(np.cos(np.deg2rad(gtbins[0:-1])))*np.sin(np.pi/180)*np.pi/180
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
    BE_Alt_P=np.array([])
    for i in range(e,54,1):
    
        str0="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/HitCharge_"+str(i)+"_proton.root"
        str1="/mnt/g/TestData/G4py/"+location+"/combineROOTfile/protondata_"+str(i)+"_primary.root"
        BE_Alti=CountE(str0,[25,-160],[0,100],ARange,Ek[i]*1000,[0,45])
        BE_AltP=CountAlt(str1,loca,[99,100],500,i,[0,90])
        BE_AltPi=CountAlt(str1,loca,[99,100],500,i,[0,45])
    
        if len(BE_Alt)==0:
            if BE_AltP==0:
                BE_Alt=np.zeros(len(gEbins)-1)
            else:
                BE_Alt=BE_Alti/BE_AltP
            BE_Alt_P=BE_AltPi
        else:
            if BE_AltP==0:
                BE_Alts=np.zeros(len(gEbins)-1)
                BE_Alt=np.vstack((BE_Alt,BE_Alts))
            else:
                BE_Alt=np.vstack((BE_Alt,BE_Alti/BE_AltP))
            BE_Alt_P=np.vstack((BE_Alt_P,BE_AltPi))
    
    return BE_Alt, BE_Alt_P
'''
# strlist= ["orbit/0_25","orbit/30_-25","orbit/60_35","orbit/90_-15","orbit/120_35","orbit/150_-20","orbit/180_30",
#           "orbit/-150_-30","orbit/-120_20","orbit/-30_-35","orbit/-60_15","orbit/-90_-35"]
# localist=[[25,0],[-25,30],[35,60],[-15,90],[35,120],[-20,150],[30,180],
#           [-30,-150],[20,-120],[-35,-30],[15,-60],[-35,-90]]
# elist=[32,27,29,31,30,30,31,29,30,26,31,30]
#print(localist[0][0])
# for i in range(len(elist)):
#     print(localist[i][1],"\t",localist[i][0],"\t",elist[i])
#Fprotont=np.array([])
#FPprotont=np.array([])
'''
arrayloca=np.loadtxt("./text.txt")

n=12 #控制arrayloca在哪里截断，

# flist0=["34","45","56","67","78"]
# flist1=[0.3,0.4,0.5,0.6,0.7]

flist0=["45"]
flist1=[0.4]
'''
for j in range(len(flist0)):
    for i in range(0,n,1):
        #ARange 是一个5个元素的数组，
        # 前三个是地理位置（r=0（这个东西在这里没有用）, t_star 弧度, t_end 弧度），
        # 第四个是模式选择，磁纬度=>0，地理纬度=>1，第五个也是模式选择，点=0，范围=1
        ARange=[0, flist1[j], flist1[j]+0.1,0,1] #(0-0.2)(0.2-0.3)(0.3-0.4)(0.4-0.5)(0.5-0.6)(0.6-0.7)(0.7-0.8)
        strlist="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
        localist=[arrayloca[i,1],arrayloca[i,0]]
        print(strlist)
        BE_Alt, BE_Alt_P=localcount(strlist,localist,ARange,int(arrayloca[i,2]))
    
        fnum2 = interpolate.splev(Ek[int(arrayloca[i,2]):], f)
        # result, error = quad(spline_func, ke[0], ke[-1])

        Fproton = np.dot(fnum2,BE_Alt)
        ##乘以8是因为入射和记录的粒子之间的面积差了9倍
        #理论上来说还应该除以1000，因为能谱的最后的单位是每平方米，每单位立体角，每单位面积，每MeV
        #但是由于单位的来源是每个bin要除以dE，dE刚好是按MeV算的，所以不用除1000
    
        strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/BE_Alt_"+flist0[j]+"_D_new.csv"
        strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_"+flist0[j]+"_D_new.csv"
        pd.DataFrame(BE_Alt).to_csv(strBE_Alt, header=False,  index=False)
        pd.DataFrame(Fproton).to_csv(strp, header=False,  index=False)
        print(strBE_Alt)
        print(strp)
        #NPproton = pd.DataFrame(pd.read_csv("/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/gamma/Fermi/25_-160/data/PNin.csv", 
        #                        header = None,)).to_numpy()
        # NPproton=BE_Alt_P
        # zz=NPproton.T*fnum2
        # FPproton = NPproton.T[0]*fnum2/1e6/result
    
        # if i==0:
        #     Fprotont=Fproton/n
        #     FPprotont=FPproton/n
        # else:
        #     Fprotont=Fprotont+Fproton/n
        #     #FPprotont=FPprotont+FPproton/6
'''

data=pd.DataFrame(pd.read_excel("/mnt/e/Newfolder/Newfolder/中微子/其他实验的测量数据整理/AMS/protonFD.xlsx", 
                                    )).to_numpy()
yerror02=np.vstack((data[1:33,4],data[1:33,4]))
yerror23=np.vstack((data[1:33,6],data[1:33,6]))
yerror34=np.vstack((data[1:33,8],data[1:33,8]))
yerror45=np.vstack((data[1:33,10],data[1:33,10]))
yerror56=np.vstack((data[1:33,12],data[1:33,12]))
yerror67=np.vstack((data[1:33,14],data[1:33,14]))
yerror78=np.vstack((data[1:33,16],data[1:33,16]))
yerror89=np.vstack((data[1:33,18],data[1:33,18]))
yerror91=np.vstack((data[1:33,20],data[1:33,20]))
# yerror11=np.vstack((data[1:33,22],data[1:33,22]))

a=0
b=n
# n=b-a
for j in range(len(flist0)):
    for i in range(a,b,1):
        # if i==32:
        #     continue
        strlist="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
        #strBE_Alt="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist[i]+"/BE_Alt.csv"
        strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_"+flist0[j]+"_D_new.csv"
        
        Fproton= pd.DataFrame(pd.read_csv(strp, 
                                header = None,)).to_numpy()
        
        #NPproton = pd.DataFrame(pd.read_csv("/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/gamma/Fermi/25_-160/data/PNin.csv", 
        #                        header = None,)).to_numpy()
        #NPproton=BE_Alt_P
        #zz=NPproton.T*fnum2
        #FPproton = NPproton.T[0]*fnum2/1e6/result
        
        if i==a:
            Fprotont=Fproton/n/1
            #FPprotont=FPproton/len(elist)
        else:
            Fprotont=Fprotont+Fproton/n/1
            #FPprotont=FPprotont+FPproton/6    
        plt.figure(1,figsize=(10, 9))

        plt.plot(ke,spk/1000,color='gray',linewidth=1.5)

        #plt.stairs(Fprotont/dE/90, gEbins/1000, fill=False, color='blue', linewidth=2 )
        plt.stairs(Fproton[:,0]/dE, gEbins/1000, fill=False, color='blue', linewidth=2 )
        #plt.stairs(FPprotont, Ek[31:], fill=False, color='black', linewidth=2 )

        # plt.errorbar(data[1:33,2], data[1:33,3], yerr=yerror02, fmt='o',color='black', 
        #                 ecolor='black', capsize=10, elinewidth =2)
        # plt.errorbar(data[1:33,2], data[1:33,5], yerr=yerror23, fmt='o',color='brown', 
        #                 ecolor='brown', capsize=10, elinewidth =2)
        # plt.errorbar(data[1:33,2], data[1:33,7], yerr=yerror34, fmt='o',color='blue', 
        #                 ecolor='blue', capsize=10, elinewidth =2)
        plt.errorbar(data[1:33,2], data[1:33,9], yerr=yerror45, fmt='o',color='red', 
                        ecolor='r', capsize=10, elinewidth =2)
        # plt.errorbar(data[1:33,2], data[1:33,11], yerr=yerror56, fmt='o',color='yellow', 
        #                 ecolor='yellow', capsize=10, elinewidth =2)
        # plt.errorbar(data[1:33,2], data[1:33,13], yerr=yerror67, fmt='o',color='green', 
        #                 ecolor='green', capsize=10, elinewidth =2)
        # plt.errorbar(data[1:33,2], data[1:33,15], yerr=yerror78, fmt='o',color='red', 
        #                 ecolor='red', capsize=10, elinewidth =2)
        # plt.errorbar(data[1:33,2], data[1:33,17], yerr=yerror89, fmt='o',color='red', 
        #                 ecolor='red', capsize=10, elinewidth =2)
        # plt.errorbar(data[1:33,2], data[1:33,19], yerr=yerror91, fmt='o',color='red', 
        #                 ecolor='red', capsize=10, elinewidth =2)
        plt.yscale("log")
        plt.xscale("log")
        ax=plt.gca()
        ax.spines['bottom'].set_linewidth(2)  
        ax.spines['left'].set_linewidth(2)    
        ax.spines['top'].set_linewidth(2)     
        ax.spines['right'].set_linewidth(2)   
        ax.xaxis.set_tick_params(which="both",width=1.5)
        ax.yaxis.set_tick_params(which="both",width=1.5)
        #ax.spines['bottom'].set_color('red')  # 设置底部轴线颜色为红色
        #ax.spines['left'].set_color('red')    # 设置左侧轴线颜色为红色
        ax.tick_params(axis='x', labelsize=12)

        ax.set_xlim(0.06,200)
        plt.ylim(1e-6,6)

        ax.set_xlabel(r'$E_{k}~[\mathrm{GeV}]$',family='Times New Roman',fontsize=20)
        ax.set_ylabel(r'Flux $[\mathrm{(m^{2}~sec~srt~MeV)^{-1}}]$',family='Times New Roman',fontsize=20)

        ax.text(2.06, 1.2, r'GCR Proton',
                fontsize=15,color='gray', ha='center', rotation=0)

        ax.text(1.85, 0.008, r'AMS-01',
                fontsize=15, color='red', ha='center', rotation=0)

        # ax.text(42.5, 0.044, r'Simulation Primary',
        #          fontsize=15, color='black', ha='center', rotation=0)

        ax.text(0.37, 0.0001, r'Simulation Secondary',
                fontsize=15, color='blue', ha='center', rotation=0)

        # plt.show()
        str1='/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/figure/flux/p_'+strlist[6:]+'.jpg'
        plt.savefig(str1)
        plt.clf()

plt.figure(1,figsize=(10, 9))

plt.plot(ke,spk/1000,color='gray',linewidth=1.5)

#plt.stairs(Fprotont/dE/90, gEbins/1000, fill=False, color='blue', linewidth=2 )
plt.stairs(Fprotont[:,0]/dE, gEbins/1000, fill=False, color='blue', linewidth=2 )
#plt.stairs(FPprotont, Ek[31:], fill=False, color='black', linewidth=2 )

# plt.errorbar(data[1:33,2], data[1:33,3], yerr=yerror02, fmt='o',color='black', 
#                 ecolor='black', capsize=10, elinewidth =2)
# plt.errorbar(data[1:33,2], data[1:33,5], yerr=yerror23, fmt='o',color='brown', 
#                 ecolor='brown', capsize=10, elinewidth =2)
# plt.errorbar(data[1:33,2], data[1:33,7], yerr=yerror34, fmt='o',color='blue', 
#                 ecolor='blue', capsize=10, elinewidth =2)
plt.errorbar(data[1:33,2], data[1:33,9], yerr=yerror45, fmt='o',color='red', 
                ecolor='r', capsize=10, elinewidth =2)
# plt.errorbar(data[1:33,2], data[1:33,11], yerr=yerror56, fmt='o',color='yellow', 
#                 ecolor='yellow', capsize=10, elinewidth =2)
# plt.errorbar(data[1:33,2], data[1:33,13], yerr=yerror67, fmt='o',color='green', 
#                 ecolor='green', capsize=10, elinewidth =2)
# plt.errorbar(data[1:33,2], data[1:33,15], yerr=yerror78, fmt='o',color='red', 
#                 ecolor='red', capsize=10, elinewidth =2)
# plt.errorbar(data[1:33,2], data[1:33,17], yerr=yerror89, fmt='o',color='red', 
#                 ecolor='red', capsize=10, elinewidth =2)
# plt.errorbar(data[1:33,2], data[1:33,19], yerr=yerror91, fmt='o',color='red', 
#                 ecolor='red', capsize=10, elinewidth =2)
plt.yscale("log")
plt.xscale("log")
ax=plt.gca()
ax.spines['bottom'].set_linewidth(2)  
ax.spines['left'].set_linewidth(2)    
ax.spines['top'].set_linewidth(2)     
ax.spines['right'].set_linewidth(2)   
ax.xaxis.set_tick_params(which="both",width=1.5)
ax.yaxis.set_tick_params(which="both",width=1.5)
#ax.spines['bottom'].set_color('red')  # 设置底部轴线颜色为红色
#ax.spines['left'].set_color('red')    # 设置左侧轴线颜色为红色
ax.tick_params(axis='x', labelsize=12)

ax.set_xlim(0.06,200)
plt.ylim(1e-6,6)

ax.set_xlabel(r'$E_{k}~[\mathrm{GeV}]$',family='Times New Roman',fontsize=20)
ax.set_ylabel(r'Flux $[\mathrm{(m^{2}~sec~srt~MeV)^{-1}}]$',family='Times New Roman',fontsize=20)

ax.text(2.06, 1.2, r'GCR Proton',
         fontsize=15,color='gray', ha='center', rotation=0)

ax.text(1.85, 0.008, r'AMS-01',
         fontsize=15, color='red', ha='center', rotation=0)

# ax.text(42.5, 0.044, r'Simulation Primary',
#          fontsize=15, color='black', ha='center', rotation=0)

ax.text(0.37, 0.0001, r'Simulation Secondary',
         fontsize=15, color='blue', ha='center', rotation=0)

plt.show()
#str1='/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/figure/flux/'+strlist[i][6:]+'.jpg'
#plt.savefig(str1)
plt.clf()

'''
fig, ax1 = plt.subplots(1, 3, figsize=(18, 5), dpi=100, sharex=True, 
                              gridspec_kw={'height_ratios': [1], "width_ratios":[1,1,1]}, layout='constrained')
# flist=["45","56","67","78"]
# flist2=[0.4,0.5,0.6,0.7]
# flist=["45","56"]
# flist2=[0.4,0.5]
# flist=["67","78"]
# flist2=[0.6,0.7]
flist=["34","56","78"]
flist2=[0.3,0.5,0.7]
for j in range(len(flist)):
    for i in range(0,n,1):
        if i==32:
                continue
        strlist="orbit/"+str(int(arrayloca[i,0]))+"_"+str(int(arrayloca[i,1]))
        strp="/mnt/e/Newfolder/Newfolder/中微子/新建文件夹/proton/G4py/orbit/data/"+strlist+"/Flux_"+flist[j]+"_1.csv"
    
        Fproton= pd.DataFrame(pd.read_csv(strp, 
                                header = None,)).to_numpy()
    
        if i==0:
            Fprotont=Fproton/n/6
            #FPprotont=FPproton/len(elist)
        else:
            Fprotont=Fprotont+Fproton/n/6
            #FPprotont=FPprotont+FPproton/6    
    ax1[j].plot(ke,spk/1000,color='gray',linewidth=1.5)
    ax1[j].stairs(Fprotont[:,0]/dE, gEbins/1000, fill=False, color='blue', linewidth=2 )
    yerror=np.vstack((data[1:33,8+j*4],data[1:33,8+j*4]))
    ax1[j].errorbar(data[1:33,2], data[1:33,7+j*4], yerr=yerror, fmt='o',color='red', 
                ecolor='red', capsize=10, elinewidth =2)
    ax1[j].set_yscale("log")
    ax1[j].set_xscale("log")
    ax1[j].set_xlim(0.02,200)
    ax1[j].set_ylim(1e-6,6)
    ax1[j].spines['bottom'].set_linewidth(2)  
    ax1[j].spines['left'].set_linewidth(2)    
    ax1[j].spines['top'].set_linewidth(2)     
    ax1[j].spines['right'].set_linewidth(2)   
    ax1[j].xaxis.set_tick_params(which="both",width=1.5)
    ax1[j].yaxis.set_tick_params(which="both",width=1.5)
    ax1[j].set_xlabel(r'$E_{k}~[\mathrm{GeV}]$',family='Times New Roman',fontsize=20)
    ax1[j].text(2.7, 1.2, r'GCR Proton',
         fontsize=15,color='gray', ha='center', rotation=0)

    ax1[j].text(1.85, 0.008, r'AMS-01',
         fontsize=15, color='red', ha='center', rotation=0)

    ax1[j].text(0.37, 0.0001, 'Simulation \nSecondary',
         fontsize=15, color='blue', ha='center', rotation=0)
    texts =str(flist2[j])+r'$~<~\Theta_{\mathrm{M}}~<~$'+str(round(flist2[j]+0.1, 1))
    ax1[j].text(30, 3, texts,
         fontsize=15, color='black', ha='center', rotation=0)
ax1[0].set_ylabel(r'Flux $[\mathrm{(m^{2}~sec~srt~MeV)^{-1}}]$',family='Times New Roman',fontsize=20)
plt.savefig('/mnt/e/会/COSPAR2026/图/AMS-01protonD31.svg')    
plt.show()
plt.clf()
'''