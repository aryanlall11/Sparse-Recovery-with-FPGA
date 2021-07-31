#include "utility.h"
#include <hls_stream.h>

struct ap_float{
	T        data;
	ap_uint<1>   last;
};

void leastsquare(hls::stream<ap_float> &in_stream, T maxele[M], int &S_final, int &k_final, hls::stream<ap_float> &out_stream)   // idx <= S
{
	#pragma HLS INTERFACE bram port=maxele
	#pragma HLS INTERFACE s_axilite port=S_final
	#pragma HLS INTERFACE s_axilite port=k_final
	#pragma HLS INTERFACE axis port=out_stream
	#pragma HLS INTERFACE axis port=in_stream
	#pragma HLS INTERFACE s_axilite port=return bundle=CRTL_BUS

	T L[M][M], temp, Y[k], X[M], A[M][k], res[M][M];
	ap_float tempA;

	Outer: for(int i=0;i<k_final;i++)
	{
		#pragma HLS PIPELINE II=1
		in_stream >> tempA;
		Y[i] = tempA.data;
	}

	L1: for(int i=0;i<S_final;i++)   //S_final <= M, k_final <= k
	{
		//----------------------------------- instream to bram
		for(int ii=0;ii<k_final;ii++)
		{
			#pragma HLS PIPELINE II=1
			in_stream >> tempA;
			A[i][ii] = tempA.data;
		}

		//----------------------------------- ATA Calculation
		L2: for(int j=0;j<(i+1);j++)
		{
			temp = 0;
			L3: for(int l=0;l<k_final;l++)     // ATA calculation
			{
				#pragma HLS PIPELINE II=1
				temp += A[j][l]*A[i][l];
			}

			res[j][i] = temp;
			if(i!=j)
				res[i][j] = temp;
		}

		hls::cholesky<true, M, T, T>(res, L);
		bs_subs<T, false>( X, L, maxele, i+1 );
		bs_subs<T, true>( X	, L, X, i+1 );

		L4: for(int j=0;j<k_final;j++)
		{
			temp = 0;
			for(int idx=0;idx<(i+1);idx++)
			{
				#pragma HLS PIPELINE
				temp += A[idx][j]*X[idx];
			}
			tempA.data = Y[j] - temp;
			tempA.last = (j < k_final-1)?0:1;
			out_stream << tempA;
		}

	}

	Final_X: for(int i=0;i<S_final;i++)
	{
		#pragma HLS PIPELINE
		tempA.data = X[i];
		tempA.last = (i < S_final-1)?0:1;
		out_stream << tempA;
	}
}
