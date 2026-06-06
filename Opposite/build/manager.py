import subprocess
import os
import numpy as np
import pandas as pd
import uproot
import csv

import sys
import os
current_dir = os.path.dirname(os.path.abspath(__file__))
#print(current_dir)
lib_path = os.path.join(current_dir,"..")
#print(lib_path)
sys.path.insert(0, lib_path)   # insert(0) 确保优先搜索
from mypylib import tracing

import multiprocessing as mps
import time

#PIDtable=[[11,"e-"],[-11,"e+"],[13,"muon"],[15,"tau"],[2212,"proton"],[2112,"neutron"]]
#nuetrinotable=[12, 14, 16, -12, -14, -16]
#print(PIDtable[0][1])

  
def Gmacfile(filename, RootPath, datafilpath, Pnum):
    #readfile=""
    writefile='/myrun/setOutputFileName '+RootPath+'\n'
    readfile='/generator/Readfile '+datafilpath+'\n'
    eventset='/run/beamOn '+str(Pnum)
    with open(filename, 'w') as file:
        file.write('/vis/disable\n')
        file.write('/run/numberOfThreads 32\n')
        file.write('\n')
        file.write('/field/enable true\n')
        file.write('/field/SetType globaldipole\n')
        #file.write('/field/Geolocation 1 25 -160 degree\n')
        file.write('\n')
        file.write('/run/initialize\n')
        file.write('\n')
        file.write(writefile)
        file.write('/control/verbose 0\n')
        file.write('/run/verbose 0\n')
        file.write('/event/verbose 0\n')
        file.write('/tracking/verbose 0\n')
        #file.write('/Geo/lat 25 deg\n')
        #file.write('/Geo/lon -160 deg\n')
        file.write('\n')
        file.write('/generator/select Secondary\n') 
        file.write(readfile)
        file.write('\n')
        file.write(eventset)

def GEANT4RuningFun(macro_file,):
    # 这一行会：
    # 1. 启动 GEANT4 进程
    # 2. 等待它执行完毕（无论多久）
    # 3. 自动返回，继续往下执行
    result = subprocess.run(
        ["./test02", macro_file],   # 命令和参数
        check=True,                      # 如果GEANT4异常退出，会抛出异常
        capture_output=False,            # 让GEANT4的输出直接打印到终端，方便调试
        stdout=None,          
        stderr=subprocess.PIPE,     # 捕获为管道
        text=False                # 保持 stderr 为 bytes
    )
    print("GEANT4 运行结束，开始整理数据...")
    if len(result.stderr) < 4:
        raise RuntimeError("Geant4 未输出有效数据")

    raw_count = result.stderr[:4]  # 读取 4 字节整数（粒子数）
    count = int.from_bytes(raw_count, byteorder='little', signed=False)
    print(f"接收到 {count} 个粒子")

    # 定义粒子结构对应的 numpy dtype（与 C++ 端严格对齐）
    particle_dtype = np.dtype([
        ('x', 'f8'), ('y', 'f8'), ('z', 'f8'),
        ('dx', 'f8'), ('dy', 'f8'), ('dz', 'f8'),
        ('energy', 'f8'),('PID','f8'),('charge','f8'),('t','f8') ,('m','f8')
    ], align=True)

    # 读取所有粒子二进制数据
    raw_data = result.stderr[4:]#(count * particle_dtype.itemsize)
    print(len(raw_data))
    print(particle_dtype.itemsize)

    return np.frombuffer(raw_data, dtype=particle_dtype)
  
def ParticleSelect(particles): 
    # 选择需要的粒子主要是正反质子，正反电子，各种核子。
    PID=particles['PID']
    indx=np.where((PID==11)|(PID==-11)|(PID==2212)|(PID==-2212)|(PID>1e10)|(PID<-1e10))
    if len(indx[0])==0:
        plist=np.array([])
    else:
        plist=particles[indx[0]]
        plist=plist.view(np.float64).reshape(plist.shape[0], -1)
    return plist
 
def DataWrite(data, savefile):
    with open(savefile, 'w', newline='') as file:
        writer = csv.writer(file)
        for row in  data :
            writer.writerow(row)

def SaveRoot(filename,data):
    wdata = {"PosX": data[:,0], "PosY": data[:,1], "PosX": data[:,2], 
             "DirX": data[:,3], "DirY": data[:,4], "DirZ": data[:,5],
             "Energy": data[:,6], "PID": data[:,7], "Charge": data[:,8], "task_id":data[:,9]
             }
    with uproot.recreate(filename) as file:
        # 使用 mktree 明确指定创建 TTree
        file.mktree("HitData", wdata)   # 第二个参数传入数据字典
    #file.close()
    

def LoopBasic(particlelist,Enum,n):
    if len(particlelist)==0:
        return [], 0
    args_list = []
    for i, plist in enumerate(particlelist):
        args = (plist[0:3], plist[3:6], [plist[10], plist[8]], i,Enum,n)   # 注意 i 已用 enumerate
        args_list.append(args)

    # 创建一个进程池，最大并发数可以设为 len(particlelist) 或更小的值
    # 如果你仍然需要限制数据库连接数（之前 sleep(5) 的作用），可以设置并发数较小，
    # 比如同时只运行 4 个进程，而不是全部。
    MAX_CONCURRENT = 32   # 根据你的数据库连接限制调整
    with mps.Pool(processes=MAX_CONCURRENT) as pool:
        # starmap 用于每个参数是一个元组，会自动解包传给 main
        # 如果 main 不接受元组而需要多个参数，就用 starmap
        results = pool.starmap(tracing.main, args_list)
    print("Process runing finish, Now selecting and saving data. ")
    Parlist=np.array([])
    Hitlist=np.array([])
    for result in results:
        task_id, res, Hit = result
        if len(res)==0:
            if len(Hit)==0:
                continue
            else:
                hlist=np.hstack((Hit,np.ones((Hit.shape[0],4))*np.hstack((particlelist[task_id,6:9],task_id))))
                #这是一个x,y,z,mx,my,mz,Ek,PID,charge 的数组
                if len(Hitlist)==0:
                    Hitlist=hlist
                else:
                    Hitlist=np.vstack((Hitlist,hlist))
            continue
        else:
            #print(res)
            plist=np.hstack(([res[0,0:3],res[0,3:6]/ np.linalg.norm(res[0,3:6]),particlelist[task_id,6:8]]))
            #这是一个x,y,z,归一化（mx,my,mz）,Ek,PID,charge 的数组
            if len(Parlist)==0:
                Parlist=plist
            else:
                Parlist=np.vstack((Parlist,plist))
            if len(Hit)==0:
                continue
            else:
                hlist=np.hstack((Hit,np.ones((Hit.shape[0],4))*np.hstack((particlelist[task_id,6:9],task_id))))
                #这是一个x,y,z,mx,my,mz,Ek,PID,charge 的数组
                if len(Hitlist)==0:
                    Hitlist=hlist
                else:
                    Hitlist=np.vstack((Hitlist,hlist))
    strmac="../result/smac/test"+str(n)+"_"+Enum+".mac"
    strcsv="../result/TraceData/test_"+str(n)+"_"+Enum+".csv"
    strroot="../result/seconddata"+str(n)+"_"+Enum+".root"
    savehit="../result/HitChargePdata"+str(n)+"_"+Enum+".root"
    #print(Hitlist)
    print("返回粒子数：",Parlist.size/8)
    if Parlist.size==8:
        pd.DataFrame(Parlist).T.to_csv(strcsv, header=False,  index=False)
        Gmacfile(strmac,strroot, strcsv,1,)
    else:
        pd.DataFrame(Parlist).to_csv(strcsv, header=False,  index=False)
        Gmacfile(strmac,strroot, strcsv,Parlist.shape[0],)
    if len(Hitlist)!=0:
        #print(Hitlist)
        SaveRoot(savehit,Hitlist)
    #DataWrite(Parlist,strcsv)
    particlelist1=GEANT4RuningFun(strmac)
    gplist=ParticleSelect(particlelist1)
    num=gplist.shape[0]
    return gplist, num
    
def main(runmac, Enum):
    particles = GEANT4RuningFun(runmac)
    #print(particles)
    tparticlelist=ParticleSelect(particles)
    pnum=tparticlelist.shape[0]
    print("粒子数量： ", pnum)
    count=0
    #设置循环参数，防止数据保存覆盖
    while pnum>0:
        tparticlelist,pnum=LoopBasic(tparticlelist,Enum,count)
        count=count+1
   
'''
macro_file = "./run.mac"

# 这一行会：
# 1. 启动 GEANT4 进程
# 2. 等待它执行完毕（无论多久）
# 3. 自动返回，继续往下执行
result = subprocess.run(
    ["./test02", macro_file],   # 命令和参数
    check=True,                      # 如果GEANT4异常退出，会抛出异常
    capture_output=False,            # 让GEANT4的输出直接打印到终端，方便调试
    stdout=None,          
    stderr=subprocess.PIPE,     # 捕获为管道
    text=False                # 保持 stderr 为 bytes
)
print("GEANT4 运行结束，开始整理数据...")


if len(result.stderr) < 4:
    raise RuntimeError("Geant4 未输出有效数据")

raw_count = result.stderr[:4]  # 读取 4 字节整数（粒子数）
count = int.from_bytes(raw_count, byteorder='little', signed=False)
print(f"接收到 {count} 个粒子")

# 定义粒子结构对应的 numpy dtype（与 C++ 端严格对齐）
particle_dtype = np.dtype([
    ('x', 'f8'), ('y', 'f8'), ('z', 'f8'),
    ('dx', 'f8'), ('dy', 'f8'), ('dz', 'f8'),
    ('energy', 'f8'),('PID','f8'),('charge','f8'),('t','f8') ,('m','f8')
], align=True)

# 读取所有粒子二进制数据
raw_data = result.stderr[4:]#(count * particle_dtype.itemsize)
print(len(raw_data))
print(particle_dtype.itemsize)

particles = np.frombuffer(raw_data, dtype=particle_dtype)
'''

'''
particles = GEANT4RuningFun("./run.mac")
print(particles)
'''

"""
fields_to_save = ['x', 'y', 'z', 'dx', 'dy', 'dz', 'energy', 'PID']
df = pd.DataFrame(particles[fields_to_save])
df.to_csv('output.csv', index=False)

"""

'''
count=0
while pnum>0:
    tparticlelist,pnum=LoopBasic(tparticlelist,count)
    count=count+1
'''

'''
PID=particles['PID']
eindx=np.where((PID==11)|(PID==-11)) #do not consider anti or not
pindx=np.where((PID==2122)|(PID==-2122)) 

#muindx=np.where(PID==13|PID==-13)
nindx=np.where(PID==2112)
protonlist=particles[nindx]
protonlist=protonlist.view(np.float64).reshape(protonlist.shape[0], -1)

result_queue = mps.Queue()
p_list = []
i=0
for plist in protonlist: 

    p = mps.Process(target=tracing.main, args=(plist[0:3],plist[3:6],[plist[10],1],i,result_queue))
    p_list.append(p)
    p.start()
    time.sleep(5) #休眠60s，以免数据库连接超出最大可连接数报错
    i=i+1
for p in p_list:
    p.join()

Parlist=[]
for i in range(len(nindx[0])):
    task_id, res = result_queue.get()
            
    if res[6]<6472:
        plist=np.hstack(([res[0:3],res[3:6]/ np.linalg.norm(res[3:6]),protonlist[task_id,6:8]]))
        if i==0:
            Parlist=plist
        else:
            Parlist=np.vstack((Parlist,plist))

pd.DataFrame(Parlist).to_csv("test.csv", header=False,  index=False)

Gmacfile("test2.mac","seconddata_0.root", "test.csv",Parlist.shape[0],)

protonlist1=GEANT4RuningFun("./test2.mac")

print(protonlist1)
'''

if __name__ == "__main__":
    runmac = sys.argv[1]
    Enum = sys.argv[2]
    main(runmac, Enum)