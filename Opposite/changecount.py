import numpy as np
import pandas as pd
from pathlib import Path

data=np.loadtxt("./table.txt")
#print(data)
b=np.array([])
for i in data:
    p=Path("./ctable")/f"all_{int(i[0])}_{int(i[1])}.csv"
    pdirrow=pd.DataFrame(pd.read_csv(p,header=None)).to_numpy()
    aa=np.sum(pdirrow,axis=1)
    idx = np.searchsorted(aa, 0, side='right')
    b=np.append(b,idx+1)
    #print("stop")
    
sd=np.vstack((data.T,b))
print(sd.T)