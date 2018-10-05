import numpy as np

n = int(input("number of pixels"))
coordX = range(0, 2*n, 2)
mat = np.mat([[pow(x,i) for i in range(0,n)] for x in coordX])
middle = np.array([pow(n-1,i) for i in range(0,n)])
ret = middle * np.linalg.inv(mat)
print(mat)
print(middle)
print(ret)
