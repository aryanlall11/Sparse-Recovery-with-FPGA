#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <hls_math.h>

typedef ap_axis<32,2,5,6> ap_float;

/*struct ap_float{
	float        data;
	ap_uint<1>   last;
};*/
typedef union{   // float32<->int32 convert
	int u;
	float f;
} uni;

#define N 324    // A is k X N : N = 1024
#define k 324		// k(Measurements) >= M(Sparsity) : k = 256

void multiply(hls::stream<ap_float> &in_stream, int row, int col, hls::stream<ap_float> &out_stream, int &idx)
{
#pragma HLS INTERFACE axis port=out_stream
#pragma HLS INTERFACE axis port=in_stream
#pragma HLS INTERFACE s_axilite port=idx
#pragma HLS INTERFACE s_axilite port=row
#pragma HLS INTERFACE s_axilite port=col
#pragma HLS INTERFACE s_axilite port=return bundle=CRTL_BUS

	float B[k], max;
	int maxidx=0;
	uni convin, convout;

	ap_float Btemp;
	for(int i=0;i<col;i++)     // col <= k
	{
		#pragma HLS PIPELINE II=1
		in_stream >> Btemp;
		convin.u = Btemp.data;
		B[i] = convin.f;
	}

	max=0;

	for(int i=0;i<row;i++)
	{
		ap_float A_ele;
		float sum=0;

		for(int j=0;j<col;j++)
		{
			#pragma HLS PIPELINE
			in_stream >> A_ele;
			convin.u = A_ele.data;
			sum += convin.f*B[j];
		}

		convout.f = sum;
		A_ele.data = convout.u;

		A_ele.last = (i < row-1)?0:1;
		out_stream << A_ele;

		float absW = hls::abs(sum);
		if(absW>max){
			max=absW;
			maxidx = i;
		}
	}
	idx = maxidx;
}
