import cv2
import struct
import serial
import numpy as np 
import matplotlib.pyplot as plt
from math import log10, sqrt
# %%

A_matrix = "A.txt"
D_matrix = "D.txt"
Y_matrix = "Y.txt"

baudrate = 115200
port = '/dev/ttyUSB1'

BN = 384
N = 16
img_orig = cv2.imread("lena384.jpg", 0)

# %%
recon = np.zeros((BN,BN))
boun = np.linspace(0,BN-N,int(BN/N))
img = []
tot_time = []

# %%
def send(n):
	ser.write(chr(n))
	return 1;

def send_float(n):
	arr = list(bytearray(struct.pack("f",n)))
	for val in arr:
		ser.write(chr(val))
	return 1;


def receive_float():
	val = int(ser.readline().strip())
	long_v = struct.pack('>l', val)
	return struct.unpack('>f', long_v)[0];

def send_matrix(file):
	f = open(file, "r")
	i=0
	for line in f.readlines():
		if(i>0):
			for val in line.split(" "):
				send_float(float(val))
		i = i+1
	f.close()
	return 1;

def plot_time(times):
	x = np.arange(1, len(times)+1, 1)
	plt.xlabel('Iteration number') 
	plt.ylabel('Time taken') 
	plt.title('Time taken at different iterations') 
	plt.plot(x, times)
	plt.show() 
	return 1

a = BN
b = BN
dum_var1 = (np.linspace(0,a-N,int(a/N))) #making an array with which is AP with difference N
dum_var2 = (np.linspace(0,b-N,int(b/N)))

def smooth(recon):
    for m in dum_var1:
        it=int(m+N)
        if it==a:
            break
        for j in [-1,1,0]:
            i = it + j
            for k in range(a):
                recon[i][k] = (recon[i-3][k]+recon[i-2][k]+recon[i-1][k]+recon[i+1][k]+recon[i+2][k]+recon[i+3][k])/6
    
    for q in dum_var2:
        jt=int(q+N)
        if jt==b:
            break
        for i in [-1,1,0]:
            j = jt + i
            for l in range(b):
                recon[l][j] = (recon[l][j-3]+recon[l][j-2]+recon[l][j-1]+recon[l][j+1]+recon[l][j+2]+recon[l][j+3])/6
    return recon

def spots(recon):
    w = 90
    for i in range(1,a-1):
        for j in range(1,b-1):
            pix = recon[i][j]
            tl = recon[i-1][j-1]
            tr = recon[i-1][j+1]
            bl = recon[i+1][j-1]
            br = recon[i+1][j+1]
            dec = str(int((pix-tl)>w)) + str(int((pix-tr)>w)) + str(int((pix-bl)>w)) + str(int((pix-br)>w))
            if(dec.count('1')>2):
                recon[i][j] = (tl+tr+bl+br)/4
    return recon

def PSNR(original, compressed): 
    mse = np.mean((original - compressed) ** 2) 
    if(mse == 0):  # MSE is zero means no noise is present in the signal . 
                  # Therefore PSNR have no importance. 
        return 100
    max_pixel = 255.0
    psnr = 20 * log10(max_pixel / sqrt(mse)) 
    return [sqrt(mse),psnr]

#-------------------------
ser = serial.Serial()
ser.baudrate = baudrate
ser.port = port       # Serial COM port (115200)
ser.open()
#--------------------------
print("Waiting for response...\n")
print(ser.readline().strip())
#--------------------------
ext = 0
while(ext==0):
	print("========= MENU ========")
	print("1 - Initialize System")
	print("2 - Run OMP Algorithm")
	print("3 - Exit")
	print("=======================\nEnter your choice : ")
	ch = input()
	if(ch==1):
		send(1)
		done = 0
		while(done==0):
			line = ser.readline().strip()
			if(line=="done"):
				done=1
			else:
				print(line)
		send_matrix(A_matrix)
		print(ser.readline().strip())   #A-matrix Stored
		send_matrix(D_matrix)
		print(ser.readline().strip())   #D-matrix Stored
		print("Done initializing!\n")

	elif(ch==2):
		send(2)
		
		f = open(Y_matrix, "r")
		lines = f.readlines()
		iters = int((lines[0].split(" "))[0])
		send_float(float(iters))

		for it in range(iters):
			for val in lines[it+1].split(" "):
				send_float(float(val))

			#send_matrix(Y_matrix)
			#print(ser.readline().strip())   #Sparse signal stored
			#print(ser.readline().strip())   #Recovering original signal...

			done = 0
			while(done==0):
				line = ser.readline().strip()
				if(line=="done"):
					done=1
				#else:
					#print(line)

			#print(ser.readline().strip())   #Signal Recovered!
			img = []
			for i in range(N*N):
				img.append(receive_float())
			
			tot_time.append(receive_float())  #Time elapsed in secs
			#print(img)
			img = np.array(img).reshape(N,N).astype('uint8')

			t = BN/N
			i = boun[int(it/t)]
			j = boun[it%t]

			recon[int(i):int(i+N),int(j):int(j+N)] = img
			print(it+1)

		f.close()

		print(np.mean(tot_time))

		#######
		reconm = recon.astype('uint8')
		cv2.imshow("Recovered image", reconm)
		cv2.imwrite("Recovered_FPGA.jpg", reconm)
		cv2.waitKey(0)
		cv2.destroyAllWindows()

		recon = smooth(recon)
		recon = spots(recon)
		######
		recon = recon.astype('uint8')
		cv2.imshow("Filtered Image", recon)
		cv2.imwrite("Recovered_Filtered_FPGA.jpg", recon)
		cv2.waitKey(0)
		cv2.destroyAllWindows()
		plot_time(tot_time)
		[rmse,psn] = PSNR(img_orig,recon)
		print("Root-mean square error is %f & PSNR is %f dB"%(rmse,psn))

	elif(ch==3):
		send(3)
		ext = 1
	else:
		print("Please enter correct choice!\n")

ser.close()
