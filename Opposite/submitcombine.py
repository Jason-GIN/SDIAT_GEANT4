import numpy as np
from pathlib import Path
import htcondor2

def itemGener(num):
    tp=Path("./table.txt")
    data=np.loadtxt(tp)
    lon=int(data[num,0])
    lat=int(data[num,1])
    item=[]
    # saveHit="result/"+str(lon)+"_"+str(lat)+"/H"
    # n=1
    for j in range(int(data[num,2]),54,1):
        strHit="./result/"+str(lon)+"_"+str(lat)+"/H/HitChargePdata*_"+str(j)+".root"
        strsec="./result/"+str(lon)+"_"+str(lat)+"/H/seconddata*_"+str(j)+"_*.root"
        saveHit="./result/"+str(lon)+"_"+str(lat)+"/H/HitCharge_"+str(j)+"_all.root"
        savesec="./result/"+str(lon)+"_"+str(lat)+"/H/seconddata_"+str(j)+"_all.root"
        d=dict(input_file1=strHit,input_file2=strsec,Enum=str(j),savepath1=saveHit,savepath2=savesec,)
        item.append(d)
        # n=n+1
    
    return item

'''
每次提交记得修改这个参数、从0到47、一共48个。
前12个自成一个轨道，可以用来做快速验证
具体的值在table.txt里面能看到。
每交一次在这里标注一次，交了哪个
0, (0,25), 
1, (30,-25), 
2, (60, 35), o382520,t
3, (90, -15), o385311,
4, (120,35), o383801,t
5, (150,-20), o383802,t
6, (180,30), o384241,t
7, (-150,-30), o384243,t
8, (-120,20), o392980,t
9, (-30,-35), o392981,t
10, (-60,15), o393001,t
11, (-90,-35),o393013,

'''
# 而且实验上看来，还是先整合再下载来的快。
# 允许的话最好是整合一遍

itemdata=itemGener(11)
# print(itemdata)
# print("stop")

submit_description = {
    "universe": "vanilla",
    "executable": "combine.sh",           # 只保留一个通用脚本
    "arguments": "$(input_file1) $(input_file2) $(Enum) $(savepath1) $(savepath2)", # 传入5个参数，用空格隔开
    "output": "output/$(ClusterId).$(ProcId).out",
    "error": "output/$(ClusterId).$(ProcId).err",
    "request_cpus": "32",
    "log": "log/$(ClusterId).$(ProcId).log",
}

sub = htcondor2.Submit(submit_description)

schedd = htcondor2.Schedd()
result = schedd.submit(sub, itemdata=iter(itemdata))

print(f"提交了 {len(itemdata)} 个子作业，Cluster ID: {result.cluster()}")
