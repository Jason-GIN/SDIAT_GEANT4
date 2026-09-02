import numpy as np
from pathlib import Path
#import htcondor2


def itemGener(num):
    tp=Path("./table.txt")
    data=np.loadtxt(tp)
    lon=int(data[num,0])
    lat=int(data[num,1])
    item=[]
    savepath="/"+str(lon)+"_"+str(lat)+"/H"
    n=1
    for j in range(int(data[num,2]),54,1):
        strmac="./run/"+str(lon)+"_"+str(lat)+"/run_H/run_H_"+str(n)+".mac"
        d=dict(input_file=strmac,Enum=str(j),path=savepath)
        item.append(d)
        n=n+1
    
    return item

itemdata=itemGener(0)

'''
#这个玩意用来检查有没有非字符的字典值的
#非字符的字典值会报错
print(itemdata)
for idx, item in enumerate(itemdata):
    for key, value in item.items():
        if not isinstance(value, str):
            print(f"item[{idx}]['{key}'] 的类型是 {type(value)}，值为 {value}，请转换为字符串！")
'''
#print("stop")

'''
# 1. 提交描述：executable 固定，新增 arguments
submit_description = {
    "universe": "vanilla",
    "executable": "run_task.sh",           # 只保留一个通用脚本
    "arguments": "$(input_file) $(path)", # 传入两个参数，用空格隔开
    "output": "output/$(ClusterId).$(ProcId).out",
    "error": "output/$(ClusterId).$(ProcId).err",
    "request_cpus": "32",
    "log": "log/$(ClusterId).$(ProcId).log",
}

sub = htcondor2.Submit(submit_description)

schedd = htcondor2.Schedd()
result = schedd.submit(sub, itemdata=iter(itemdata))

print(f"提交了 {len(itemdata)} 个子作业，Cluster ID: {result.cluster()}")
'''