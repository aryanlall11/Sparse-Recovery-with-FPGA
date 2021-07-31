#include "utility.h"
#include <hls_stream.h>

struct ap_float{
	T        data;
	ap_uint<1>   last;
};

void leastsquare( hls::stream<ap_float> &in_stream, int idx, hls::stream<ap_float> &out_stream)
{
	#pragma HLS INTERFACE axis port=out_stream
	#pragma HLS INTERFACE axis port=in_stream
	#pragma HLS INTERFACE s_axilite port=idx
	#pragma HLS INTERFACE s_axilite port=return bundle=CRTL_BUS

	T L[M][M], A[M][M], Y[M];

	ap_float temp;
	for(int i=0;i<idx;i++)
	{
		#pragma HLS PIPELINE II=1
		in_stream >> temp;
		Y[i] = temp.data;
	}
	for(int i=0;i<idx;i++)
		for(int j=0;j<idx;j++)
		{
			#pragma HLS PIPELINE II=1
			in_stream >> temp;
			A[i][j] = temp.data;
		}

	hls::cholesky<true, M, T, T>(A, L);
	bs_subs<T, false>( Y, L, Y, idx );
	bs_subs<T, true>( Y, L, Y, idx );

	for(int i=0;i<idx;i++)
	{
		#pragma HLS PIPELINE II=1
		temp.data = Y[i];
		temp.last = (i < idx-1)?0:1;
		out_stream << temp;
	}
}
