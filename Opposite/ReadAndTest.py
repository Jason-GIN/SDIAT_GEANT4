import uproot
import numpy as np
import pandas as pd 



a=np.array([[1,2,3,4,5,6,7,8]])
b=np.array([1,2,3,4,5,6,7,8])
print(a.shape, " ", a.shape[0])
print(b.shape, " ", b.shape[0])
print(a.size)
print("stop")
'''
str0="./build/protondata_0_t0.root"
a=uproot.open(str0)
PID=a["DetectionInfo"]["PID"].array(library="np")
pPid=a["DetectionInfo"]["ParentId"].array(library="np")
x=a["DetectionInfo"]["PosX"].array(library="np")
y=a["DetectionInfo"]["PosY"].array(library="np")
z=a["DetectionInfo"]["PosZ"].array(library="np")
E=a["DetectionInfo"]["Energy"].array(library="np")

xyz=np.vstack((x,y,z))
alt=np.linalg.norm(xyz,axis=0)
array = np.vstack((alt,PID,pPid,E))
pd.DataFrame(array.T).to_csv("./PID.csv", header=False,  index=False)
print(alt)
'''

str1="./result/HitChargePdata0_0.root"
a=uproot.open(str1)
print(a['HitData'].keys())
print(a["HitData"]['PID'].array(library="np"))

