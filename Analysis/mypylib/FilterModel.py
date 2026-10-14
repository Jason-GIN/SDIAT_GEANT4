#import pandas as pd
import numpy as np

def dirselect(dir,ra):
    ind=np.where((ra[0]<dir)&(dir<ra[1]))
    return ind[0]

def filter(branch, num):
    if len(num)==1:
        indices=np.where(branch==num)
        return indices[0]
    elif len(num)==2:
        indices=np.where( (branch==num[0]) | (branch==num[1]) )
        return indices[0]
    else:
        print("error!! Too many parameters in num")
        return []
    
def filterrange(branch, num1,num2):
    indices=np.where((num1<=branch)&(branch<num2))
    return indices[0]   

# def filterrange_returnA(branch, num1,num2)

def SelectAltitude(Altbranch, TAlt):
    T=TAlt+6371
    newAbranch=np.round(Altbranch)
    indeices=np.where(newAbranch==T)
    return indeices[0]