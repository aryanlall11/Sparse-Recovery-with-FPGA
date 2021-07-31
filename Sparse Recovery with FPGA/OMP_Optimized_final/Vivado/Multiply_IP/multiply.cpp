#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <hls_math.h>

typedef ap_axis<32,2,5,6> ap_float;

typedef union{   // float32<->int32 convert
	int u;
	float f;
} uni;

#define N 256    // A is k X N : N = 1024
#define k 232		// k(Measurements) >= M(Sparsity) : k = 256

#define U 8 //unroll factor

void multiply(hls::stream<ap_float> &in_stream, int row, int col, hls::stream<ap_float> &out_stream, int &idx)
{
	#pragma HLS INTERFACE axis port=out_stream
	#pragma HLS INTERFACE axis port=in_stream
	#pragma HLS INTERFACE s_axilite port=idx
	#pragma HLS INTERFACE s_axilite port=row
	#pragma HLS INTERFACE s_axilite port=col
	#pragma HLS INTERFACE s_axilite port=return bundle=CRTL_BUS

	float B[k], max, Atemp[N], res[N];

	#pragma HLS array_partition variable=B cyclic factor=8
	#pragma HLS array_partition variable=Atemp cyclic factor=8
	//#pragma HLS array_partition variable=multtemp cyclic factor=8

	int maxidx=0;
	uni convin, convout;
	ap_float A_ele;

	//ap_float Btemp;
	L1: for(int i=0;i<col;i++)     // col <= k
	{
		#pragma HLS PIPELINE II=1
		in_stream >> A_ele;
		convin.u = A_ele.data;
		B[i] = convin.f;
	}

	max=0;

	L2: for(int i=0;i<row;i++)
	{
		float acc_part[U] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

		L3: for(int j=0;j<col;j++)
		{
			#pragma HLS PIPELINE II=1
			in_stream >> A_ele;
			convin.u = A_ele.data;
			Atemp[j] = convin.f;
		}

		L4: for(int j=0;j<col;j+=U)
		{
			for (int l = 0; l < U; l++)
			{ // Partial accumulations
				#pragma HLS pipeline
				//multtemp[j+l] = ((j+l)<col)?(Atemp[j+l]*B[j+l]):0;
				//acc_part[l] += multtemp[j + l];
				acc_part[l] += ((j+l)<col)?(Atemp[j+l]*B[j+l]):0;
			}
		}

		L5: for (int l = 1; l < U; l++)
		{ // Partial accumulations
			#pragma HLS unroll
			acc_part[0] += acc_part[l];
		}

		res[i] = acc_part[0];

		float absW = hls::abs(acc_part[0]);
		if(absW>max){
			max=absW;
			maxidx = i;
		}
	}
	idx = maxidx;

	L6: for(int i=0;i<row;i++)
	{
		#pragma HLS PIPELINE II=1
		convout.f = res[i];
		A_ele.data = convout.u;
		A_ele.last = (i < row-1)?0:1;
		out_stream << A_ele;
	}
}
