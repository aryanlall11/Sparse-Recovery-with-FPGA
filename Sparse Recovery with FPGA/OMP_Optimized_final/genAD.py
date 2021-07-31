import cv2
import numpy as np
from scipy.fftpack import dct
from config import *

#N = 16                         # Width of the image
#C = 4
#S = 28                   # Sparsity level
#img = cv2.imread("lena384.jpg", 0)
img = cv2.imread(image_name, 0)

A_matrix = "A.txt"
D_matrix = "D.txt"
Y_matrix = "Y.txt"

n = int(np.ceil(C*S*np.log(N*N/S)))  # Total no. of measurements
#n=3
print(n)
n = 232

pix = N**2
sz = (pix*n + pix*pix + pix)/256.0;
print("SRAM Storage = %f kB"%sz)

# %%
def gen_image(size, scale):
    N = size
    img = np.zeros((N,N,1), dtype="uint8")
    img = cv2.circle(img, (N/2,N/2), int(N*scale), (255, 255, 255), -1)   
    cv2.imshow("Test image", img)
    cv2.imwrite("Test.jpg", img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()
    
# %%
def read_image(file_name):
    return cv2.imread(file_name, 0)

def matrix_mul(A, B):
    return np.dot(A, B)

def store_matrix(A, file_name):
    [r,c] = np.array(A).shape
    print(r,c)
    f = open(file_name,"w")
    f.write(str(r) + " " + str(c))
    for i in range(r):
        f.write("\n")
        temp = ""
        for j in range(c):
            temp = temp + str(A[i][j]) + " "
        f.write(temp.strip())
    f.close()

# %%
#gen_image(N, 0.15)

cv2.imshow("Input image", img)
cv2.waitKey(0)
cv2.destroyAllWindows()
#=================================================================================
A = ((-1)**(np.random.randint(2, size=(n, N*N))+1))/np.sqrt(n)
#A = np.random.normal(0, 100, size=(n, N*N))
D = dct(np.eye(N**2), norm='ortho')   # DCT matrix (DCT2)
AD = matrix_mul(A, D)

store_matrix(np.transpose(AD), A_matrix)
store_matrix(D, D_matrix)

[a, b] = img.shape
dum_var1 = (np.linspace(0,a-N,int(a/N))) #making an array with which is AP with difference N
dum_var2 = (np.linspace(0,b-N,int(b/N)))

f = open(Y_matrix,"w")
f.write(str(int(a*b/(N**2))) + " " + str(n))

iter1 = 0
for i in dum_var1:
    for j in dum_var2:
        iter1 = iter1+1
        print("Iteration %d"%iter1)
        Y = img[int(i):int(i+N),int(j):int(j+N)]
        Y = Y.reshape((N*N, 1)).astype('float')
        y = matrix_mul(A, Y)
        temp = ""
        f.write("\n")
        for k in range(n):
            temp = temp + str(y[k][0]) + " "
        f.write(temp.strip())
f.close()

#=================================================================================
