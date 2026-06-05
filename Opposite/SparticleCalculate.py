import uproot
import numpy as np
import pandas as pd
from mypylib import *

def ReadAndGetNumber(filename):
    expect=uproot.open(filename)
    SDnume=expect["BoundaryInfo"]["SDnum"].array(library="np")
    return len(SDnume)

def filepart(filename, idx, Parlist): #Parlist=[x,y,z,xv,yv,zv,kE,PID]
    strPID='/generator/ParticleSet '+str(Parlist[7])+'\n'
    strkE ='/gps/ene/mono '+str(Parlist[6])+' GeV\n'
    strPos='/gps/pos/centre '+str(Parlist[0])+' '+str(Parlist[1])+' '+str(Parlist[2])+' km\n'
    strDir='/gps/direction 0. 0. -1.\n'
    if idx==0:
        with open(filename, 'a') as file:
            file.write(strPID)               
            file.write('/gps/ene/type Mono\n')                     
            file.write(strkE)                    
            file.write('/gps/pos/type Point\n')             
            file.write(strPos)            
            file.write(strDir)
            file.write('\n')
    else:
        with open(filename, 'a') as file:
            file.write('/gps/source/add 1.0\n')
            file.write(strPID)               
            file.write('/gps/ene/type Mono\n')                     
            file.write(strkE)                    
            file.write('/gps/pos/type Point\n')             
            file.write(strPos)            
            file.write(strDir)
            file.write('\n')
       
     
def Gmacfile(filename, Plist):
    #readfile=""
    with open(filename, 'w') as file:
        file.write('/vis/disable\n')
        file.write('/run/numberOfThreads 32\n')
        file.write('\n')
        file.write('/field/enable true\n')
        file.write('/field/SetType globaldipole\n')
        file.write('/field/Geolocation 1 25 -160 degree\n')
        file.write('\n')
        file.write('/run/initialize\n')
        file.write('\n')
        file.write('/myrun/setOutputFileName protondata_0.root\n')
        file.write('/control/verbose 0\n')
        file.write('/run/verbose 0\n')
        file.write('/event/verbose 0\n')
        file.write('/tracking/verbose 0\n')
        file.write('/Geo/lat 25 deg\n')
        file.write('/Geo/lon -160 deg\n')
        file.write('\n')
        file.write('/generator/select gps\n')
    
    for i in range(len(Plist[0,:])):
        #print(Plist[:,i])
        filepart(filename, i, Plist[:,i])
    with open(filename, 'a') as file:  
        file.write('/gps/source/multiplevertex true\n')   
        file.write('\n')
        file.write('/run/beamOn 1')


cc = 299792458 # in m/s
e=1.602176634e-19

m1=1.50534967e-22
m=9.3956536e+02

#print(m/cc/cc*1e6*e)
'''
location=np.array([-4.99199e+3,-2.88277e+3,2.72951e+3])
r,t,p=RotationModel.xyz2rthetaphi(-4.99199e+3,-2.88277e+3,2.72951e+3)
B=igrf14.igrf_wrapper(t, p, r, 2025)
print(np.linalg.norm(location))
print(np.degrees(np.pi/2-t),np.degrees(p))
print(B)

'''
'''
print("stop")
PIDlist=np.array([])
pnum=0
for i in range(32):
    str0="./build/protondata_0_t"+str(i)+".root"
    num=ReadAndGetNumber(str0)
    pnum=pnum+num
    a=uproot.open(str0)
    PID_raw=a["BoundaryInfo"]["PID"].array(library="np")
    PIDlist=np.append(PIDlist,PID_raw)
    
UPID, countsPID = np.unique(PIDlist, return_counts=True)
print(pnum)
print(UPID)
print(countsPID)
'''
Parlist=np.array([])
PIDlist=np.array([])
monlist=np.array([])
kElist=np.array([])
for i in range(32):
    str0="./build/protondata_0_t"+str(i)+".root"
    num=ReadAndGetNumber(str0)
    #pnum=pnum+num
    a=uproot.open(str0)
    PID_raw=a["BoundaryInfo"]["PID"].array(library="np")
    x=a["BoundaryInfo"]["PosX"].array(library="np")
    y=a["BoundaryInfo"]["PosY"].array(library="np")
    z=a["BoundaryInfo"]["PosZ"].array(library="np")
    mx=a["BoundaryInfo"]["DirX"].array(library="np")
    my=a["BoundaryInfo"]["DirY"].array(library="np")
    mz=a["BoundaryInfo"]["DirZ"].array(library="np")
    E_raw=a["BoundaryInfo"]["Energy"].array(library="np")
    mxyz_raw=np.vstack((mx,my,mz))
    monlist=np.append(monlist,np.linalg.norm(mxyz_raw,axis=0))
    PIDlist=np.append(PIDlist,PID_raw)
    kElist=np.append(kElist,E_raw)
    p_raw=np.vstack((x,y,z,mx,my,mz,E_raw,PID_raw))
    if i ==0:
        Parlist=p_raw
    else:
        Parlist=np.hstack((Parlist,p_raw))
    
nindx=np.where(PIDlist==2112)
nkElist=kElist[nindx]
nmon=monlist[nindx]
print("kE in avg: ",np.mean(nkElist))
 
nPlist=Parlist[:,nindx[0]]
Gmacfile("test2.mac",nPlist)

nElist=nkElist+m
nVelosity=nmon/nElist*cc
nGammalist=(nkElist/m)+1
print(nElist)
print(nGammalist)
#print(nVelosity)
nav=np.mean(nVelosity)
print(nav)
dist=nav*15*60/1e3
print("dist: ", dist)
print('stop')