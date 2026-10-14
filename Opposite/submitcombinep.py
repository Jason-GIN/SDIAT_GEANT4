import numpy as np
from pathlib import Path
import htcondor2

def itemGener(numup,numdown=0):
    # print(numup,numdown)
    tp=Path("./table.txt")
    data=np.loadtxt(tp)
    item=[]
    if numdown==0:
        numdown=numup+1
    else:
        numdown=numdown+1
    # saveHit="result/"+str(lon)+"_"+str(lat)+"/H"
    # n=1
    # print(numup,numdown)
    for j in range(int(data[numup,2]),54,1):
        # print(j)
        d=dict(file_index=str(j),Snum=str(numup),Enum=str(numdown))
        item.append(d)
        # n=n+1
    
    return item


'''
每次提交记得修改这个参数、从0到47、一共48个。
前12个自成一个轨道，可以用来做快速验证
具体的值在table.txt里面能看到。
每交一次在这里标注一次，交了哪个

之前的已经弄了，
3, (90, -15), o385345,t
5, (150,-20), o384231,t
6, (180,30), o384242,t
7, (-150,-30), o385310,t
8, (-120,20), o392953,t
9, (-30,-35), o392954,t
10, (-60,15), o393002,t
11, (-90,-35), o393014

'''

# print("start")
itemdata=itemGener(11)
# print(itemdata)

submit_description = {
    "universe": "vanilla",
    "executable": "combinep.sh",           # 只保留一个通用脚本
    "arguments": "$(file_index) $(Snum) $(Enum)", # 传入3个参数，用空格隔开
    "output": "output/$(ClusterId).$(ProcId).out",
    "error": "output/$(ClusterId).$(ProcId).err",
    "request_cpus": "32",
    "log": "log/$(ClusterId).$(ProcId).log",
}

sub = htcondor2.Submit(submit_description)

schedd = htcondor2.Schedd()
result = schedd.submit(sub, itemdata=iter(itemdata))

print(f"提交了 {len(itemdata)} 个子作业，Cluster ID: {result.cluster()}")