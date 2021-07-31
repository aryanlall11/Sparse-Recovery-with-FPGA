#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xmultiply.h"
#include "xaxidma.h"
#include "xleastsquare.h"
#include "xuartlite.h"
#include "xtmrctr.h"

XMultiply dot;
XMultiply_Config *dot_cfg;
XLeastsquare ls;
XLeastsquare_Config *ls_cfg;
XAxiDma axidma_mult;
XAxiDma_Config *axidma_mult_cfg;
XAxiDma axidma_ls;
XAxiDma_Config *axidma_ls_cfg;


#define N 256
#define k 170
#define m 15


int *maxidx = (int *)XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR;
float *maxele = (float *)XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR + m;
float *y = (float *)XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR + 2*m;
float *residue = (float *)XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR + k + 2*m;
float *A_ADDR = (float *)XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR + 2*(k+m);
float *temp = (float *)XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR + (N+2)*k + 2*m;   //size = 2m+k
float *x = (float *)XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR + (N+3)*k + 4*m;
float *D = (float *)XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR + (N+3)*k + 4*m + N;

typedef struct {
	XTmrCtr m_AxiTimer;
	unsigned int m_tickCounter1;
	unsigned int m_tickCounter2;
	double m_clockPeriodSeconds;
	double m_timerClockFreq;
}timer;

timer tm;

void initPeripherals()
{
	print("Initializing Multiply module...\n");
	dot_cfg = XMultiply_LookupConfig(XPAR_MULTIPLY_0_DEVICE_ID);
	if(dot_cfg)
	{
		int status = XMultiply_CfgInitialize(&dot, dot_cfg);
		if(status != XST_SUCCESS)
			print("Error Initializing Multiply module!\n");
	}
	print("Initializing Least square module...\n");
	ls_cfg = XLeastsquare_LookupConfig(XPAR_LEASTSQUARE_0_DEVICE_ID);
	if(ls_cfg)
	{
		int status = XLeastsquare_CfgInitialize(&ls, ls_cfg);
		if(status != XST_SUCCESS)
			print("Error Initializing Least square module!\n");
	}
	print("Initializing DMA module for multiply...\n");
	axidma_mult_cfg = XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID);
	if(axidma_mult_cfg)
	{
		int status = XAxiDma_CfgInitialize(&axidma_mult, axidma_mult_cfg);
		if(status != XST_SUCCESS)
			print("Error Initializing DMA module for multiply!\n");
	}
	print("Initializing DMA module for Least square...\n");
	axidma_ls_cfg = XAxiDma_LookupConfig(XPAR_AXIDMA_1_DEVICE_ID);
	if(axidma_ls_cfg)
	{
		int status = XAxiDma_CfgInitialize(&axidma_ls, axidma_ls_cfg);
		if(status != XST_SUCCESS)
			print("Error Initializing DMA module for Least square!\n");
	}

	XTmrCtr_Initialize(&tm.m_AxiTimer, XPAR_TMRCTR_0_DEVICE_ID);  //Initialize Timer
	tm.m_timerClockFreq = (double) XPAR_AXI_TIMER_0_CLOCK_FREQ_HZ;
	tm.m_clockPeriodSeconds = (double)1/tm.m_timerClockFreq;

	XAxiDma_IntrDisable(&axidma_mult, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&axidma_mult, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrDisable(&axidma_ls, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&axidma_ls, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	print("done\n");   //For marking done checkpoint
}

unsigned int startTimer() {
	// Start timer 0 (There are two, but depends how you configured in vivado)
	XTmrCtr_Reset(&tm.m_AxiTimer,0);
	tm.m_tickCounter1 =  XTmrCtr_GetValue(&tm.m_AxiTimer, 0);
	XTmrCtr_Start(&tm.m_AxiTimer, 0);
	return tm.m_tickCounter1;
}

unsigned int stopTimer() {
	XTmrCtr_Stop(&tm.m_AxiTimer, 0);
	tm.m_tickCounter2 =  XTmrCtr_GetValue(&tm.m_AxiTimer, 0);
	return tm.m_tickCounter2 - tm.m_tickCounter1;
}

float getElapsedTimerInSeconds() {
	float elapsedTimeInSeconds = (float)(tm.m_tickCounter2 - tm.m_tickCounter1) * tm.m_clockPeriodSeconds;
	return elapsedTimeInSeconds;
}

void store_samples()
{
	print("Storing Samples...\n");
	/*float A[N][k]= {{1,2,-2},
					{1.2,-3.94,0},
					{4.23,-0.99,1.09},
					{1.99,-2.01,1.98},
					{-6.35,3.24,-2.99},
					{1.238,2.12,-2.21},
					{3.45,0.98,0},
					{2.36,-1.098,-2},
					{-2.3,1.05,4},
					{4,-5.24,7}};*/
	float A[N][k] = {{1,1,-1},
					{-1,1,-1},
					{1,1,1},
					{-1,-1,-1},
					{-1,-1,1},
					{1,-1,1},
					{-1,-1,-1},
					{1,-1,-1},
					{-1,1,1},
					{1,1,1}};

	//float B[k] = {-1,1,3.1};
	float B[k] = {7.2,-2.8,7.2};

	for (int i = 0; i <k; i++)
	{
		*(y+i) = B[i];
		*(residue+i) = B[i];
	}
	for (int i = 0; i <N; i++)
		for (int j=0; j<k; j++)
			*(A_ADDR+(k*i+j)) = A[i][j];

}

void maxColumn(int idx)
{
	//print("Finding max column\n");
	XMultiply_Set_row(&dot, N);
	XMultiply_Set_col(&dot, k);
	XMultiply_Start(&dot);

	Xil_DCacheFlushRange((u32)x, N*sizeof(float));
	Xil_DCacheFlushRange((u32)residue, (N*k + k)*sizeof(float));

    XAxiDma_SimpleTransfer(&axidma_mult, (u32)residue, (N*k + k)*sizeof(float), XAXIDMA_DMA_TO_DEVICE);

	XAxiDma_SimpleTransfer(&axidma_mult, (u32)x, N*sizeof(float), XAXIDMA_DEVICE_TO_DMA);
	while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DEVICE_TO_DMA));
	//print("Received\n");

	while(!XMultiply_IsDone(&dot));

	int col = XMultiply_Get_idx(&dot);
	*(maxidx+idx-1) = col;

	if(idx==1)
		*maxele = *(x + col);
	else
	{
		XMultiply_Set_row(&dot, 1);
		XMultiply_Set_col(&dot, k);
		XMultiply_Start(&dot);

		Xil_DCacheFlushRange((u32)(maxele+idx-1), sizeof(float));
		Xil_DCacheFlushRange((u32)y, k*sizeof(float));
		Xil_DCacheFlushRange((u32)(A_ADDR + k*col), k*sizeof(float));

		XAxiDma_SimpleTransfer(&axidma_mult, (u32)y, k*sizeof(float), XAXIDMA_DMA_TO_DEVICE);
		while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DMA_TO_DEVICE));   // Continuous burst problematic
		XAxiDma_SimpleTransfer(&axidma_mult, (u32)(A_ADDR + k*col), k*sizeof(float), XAXIDMA_DMA_TO_DEVICE);

		XAxiDma_SimpleTransfer(&axidma_mult, (u32)(maxele+idx-1), sizeof(float), XAXIDMA_DEVICE_TO_DMA);
		while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DEVICE_TO_DMA));
		Xil_DCacheInvalidateRange((u32)(maxele+idx-1), sizeof(float));
		while(!XMultiply_IsDone(&dot));
	}
}

void leastsquare(int idx)
{
	//Finding least square
	XLeastsquare_Set_idx(&ls, idx);
	XLeastsquare_Start(&ls);

	Xil_DCacheFlushRange((u32)maxele, idx*sizeof(float));
	XAxiDma_SimpleTransfer(&axidma_ls, (u32)maxele, idx*sizeof(float), XAXIDMA_DMA_TO_DEVICE);

	for(int i=0;i<idx;i++)
	{
		for(int j=0;j<idx;j++)
			{
				XMultiply_Set_row(&dot, 1);
				XMultiply_Set_col(&dot, k);
				XMultiply_Start(&dot);

				Xil_DCacheFlushRange((u32)(temp+j), sizeof(float));
				Xil_DCacheFlushRange((u32)(A_ADDR + k*maxidx[i]), k*sizeof(float));
				Xil_DCacheFlushRange((u32)(A_ADDR + k*maxidx[j]), k*sizeof(float));

				XAxiDma_SimpleTransfer(&axidma_mult, (u32)(A_ADDR + k*maxidx[i]), k*sizeof(float), XAXIDMA_DMA_TO_DEVICE);
				while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DMA_TO_DEVICE));

				XAxiDma_SimpleTransfer(&axidma_mult, (u32)(A_ADDR + k*maxidx[j]), k*sizeof(float), XAXIDMA_DMA_TO_DEVICE);

				XAxiDma_SimpleTransfer(&axidma_mult, (u32)(temp+j), sizeof(float), XAXIDMA_DEVICE_TO_DMA);
				while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DEVICE_TO_DMA));
				Xil_DCacheInvalidateRange((u32)(temp+j), sizeof(float));
				while(!XMultiply_IsDone(&dot));
			}

		Xil_DCacheFlushRange((u32)temp, idx*sizeof(float));
		XAxiDma_SimpleTransfer(&axidma_ls, (u32)temp, idx*sizeof(float), XAXIDMA_DMA_TO_DEVICE);
	}
	Xil_DCacheFlushRange((u32)temp, idx*sizeof(float));
	XAxiDma_SimpleTransfer(&axidma_ls, (u32)temp, idx*sizeof(float), XAXIDMA_DEVICE_TO_DMA);

	while(XAxiDma_Busy(&axidma_ls, XAXIDMA_DEVICE_TO_DMA));
	Xil_DCacheInvalidateRange((u32)temp, idx*sizeof(float));
	while(!XLeastsquare_IsDone(&ls));
}

void residueUpdate(int idx)
{
	//print("Updating residue\n");
	float *rw = temp + m;
	float *val = temp + 2*m;

	for(int i=0; i<k; i++)
	{
		XMultiply_Set_row(&dot, 1);
		XMultiply_Set_col(&dot, idx);
		XMultiply_Start(&dot);

		Xil_DCacheFlushRange((u32)temp, idx*sizeof(float));
		Xil_DCacheFlushRange((u32)(val+i), sizeof(float));
		Xil_DCacheFlushRange((u32)rw, idx*sizeof(float));

		XAxiDma_SimpleTransfer(&axidma_mult, (u32)temp, idx*sizeof(float), XAXIDMA_DMA_TO_DEVICE);
		while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DMA_TO_DEVICE));

		for(int j=0;j<idx;j++)
			*(rw+j) = *(A_ADDR + i + k*maxidx[j]);

		XAxiDma_SimpleTransfer(&axidma_mult, (u32)rw, idx*sizeof(float), XAXIDMA_DMA_TO_DEVICE);

		XAxiDma_SimpleTransfer(&axidma_mult, (u32)(val+i), sizeof(float), XAXIDMA_DEVICE_TO_DMA);
		while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DEVICE_TO_DMA));
		Xil_DCacheInvalidateRange((u32)(val+i), sizeof(float));
		while(!XMultiply_IsDone(&dot));

		*(residue+i) = *(y+i) - *(val+i);

	}
}

void constructImage()
{
	//print("Constructing Image\n");
	for (int i = 0; i < N; i++)
		*(x+i) = 0.0;
	for (int i = 0; i < m; i++)
		*(x + *(maxidx+i)) = *(temp+i);  //Original Image constructed

	XMultiply_Set_row(&dot, N);
	XMultiply_Set_col(&dot, N);
	XMultiply_Start(&dot);

	Xil_DCacheFlushRange((u32)x, (N+1)*N*sizeof(float));

	XAxiDma_SimpleTransfer(&axidma_mult, (u32)x, (N+1)*N*sizeof(float), XAXIDMA_DMA_TO_DEVICE);

	//while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DMA_TO_DEVICE));
	//print("sent\n");

	XAxiDma_SimpleTransfer(&axidma_mult, (u32)x, N*sizeof(float), XAXIDMA_DEVICE_TO_DMA);

	while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DEVICE_TO_DMA));
	Xil_DCacheInvalidateRange((u32)x, N*sizeof(float));
	while(!XMultiply_IsDone(&dot));
}

float receive_float()
{
	union{
		float val_float;
		unsigned char bytes[4];
	}data;
	data.bytes[0] = XUartLite_RecvByte(XPAR_UARTLITE_0_BASEADDR);
	data.bytes[1] = XUartLite_RecvByte(XPAR_UARTLITE_0_BASEADDR);
	data.bytes[2] = XUartLite_RecvByte(XPAR_UARTLITE_0_BASEADDR);
	data.bytes[3] = XUartLite_RecvByte(XPAR_UARTLITE_0_BASEADDR);
	return data.val_float;
}

void verify()
{
	for (int i = 0; i <N; i++)
		for (int j=0; j<k; j++)
			xil_printf("%d\n", *(int *)(A_ADDR+(k*i+j)));
	print("A-matrix Sent\n");

	for (int i = 0; i <N; i++)
		for (int j=0; j<N; j++)
			xil_printf("%d\n", *(int *)(D+(N*i+j)));
	print("D-matrix Sent\n");
}

void testMult()
{
	print("Finding max column\n");

	XMultiply_Set_row(&dot, N);
	XMultiply_Set_col(&dot, k);
	XMultiply_Start(&dot);

	for(int i=0;i< N*k + k; i++)
		*(residue+i) = 1.1;

	Xil_DCacheFlushRange((u32)x, N*sizeof(float));
	Xil_DCacheFlushRange((u32)residue, (N*k + k)*sizeof(float));
	print("Storing\n");

	/*for(int i=0;i<(N+1);i++)
	{
		XAxiDma_SimpleTransfer(&axidma_mult, (u32)(residue+k*i), k*sizeof(float), XAXIDMA_DMA_TO_DEVICE);
		while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DMA_TO_DEVICE));
		//print("sent\n");
	}*/

    XAxiDma_SimpleTransfer(&axidma_mult, (u32)residue, (N*k + k)*sizeof(float), XAXIDMA_DMA_TO_DEVICE);
	//while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DMA_TO_DEVICE));
	print("Values sent!\n");

	XAxiDma_SimpleTransfer(&axidma_mult, (u32)x, N*sizeof(float), XAXIDMA_DEVICE_TO_DMA);
	while(XAxiDma_Busy(&axidma_mult, XAXIDMA_DEVICE_TO_DMA));
	print("Received\n");

	Xil_DCacheInvalidateRange((u32)x, N*sizeof(float));
	while(!XMultiply_IsDone(&dot));

	int col = XMultiply_Get_idx(&dot);
	xil_printf("%d\n", col);
}

/*int main()
{
	init_platform();
	initPeripherals();
	print("Handshake successful!\n");

	for(int i=1;i<=m;i++)
		residueUpdate(i);
	print("Done\n");
	//testMult();
	//for(int i=0; i<k; i++)
	//	xil_printf("0x%8x\r\n", *(int *)(x+i));

	cleanup_platform();
	return 0;
}*/

int main()
{
	init_platform();
	print("Handshake successful!\n");

	int iters;
	uint8_t ext = 0;
	while(ext==0)
	{
		uint8_t mode = XUartLite_RecvByte(XPAR_UARTLITE_0_BASEADDR);
		switch(mode)
		{
		case 1:
			initPeripherals();
			for (int i = 0; i <N; i++)
				for (int j=0; j<k; j++)
					*(A_ADDR+(k*i+j)) = receive_float();
			print("A-matrix Stored\n");
			for (int i = 0; i <N; i++)
				for (int j=0; j<N; j++)
					*(D+(N*i+j)) = receive_float();
			print("D-matrix Stored\n");
			break;

		case 2:
			//verify();
			iters = (int)receive_float();

			for(int it=0; it<iters; it++)
			{
				for (int i = 0; i <k; i++)
				{
					float val = receive_float();
					*(y+i) = val;
					*(residue+i) = val;
				}
				//print("Sparse signal stored\n");
				//print("Recovering original signal...\n");

				startTimer();
				//float tot_time = 0;

				for(int i=1; i<=m; i++)  //----- OMP Algorithm
				{
					maxColumn(i);

					//startTimer();
					leastsquare(i);
					//stopTimer();
					//tot_time = tot_time + getElapsedTimerInSeconds();

					residueUpdate(i);
					//print("Iteration completed\n");
				}
				constructImage();        //-----

				stopTimer();
				float tot_time = getElapsedTimerInSeconds();

				print("done\n");

				//print("Signal Recovered!\n");
				for(int i=0; i<N; i++)
					xil_printf("%d\n", *(int *)(x+i));
				xil_printf("%d\n", *(int *)(&tot_time));
			}

			break;

		case 3:
			ext = 1;
		}
	}

	cleanup_platform();
	return 0;
}
