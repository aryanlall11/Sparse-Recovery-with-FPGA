#include <hls_stream.h>
#include <hls_math.h>

struct ap_float{
	float        data;
	ap_uint<1>   last;
};

void multiply(hls::stream<ap_float> &in_stream, hls::stream<ap_float> &out_stream, int &idx);

int main()
{
	hls::stream<ap_float> instream;
	hls::stream<ap_float> outstream;
	int idx;

	const int N = 10;
	const int k = 3;

	float A[N][k]= {{1,2,-2},
					{1.2,-3.94,0},
					{4.23,-0.99,1.09},
					{1.99,-2.01,1.98},
					{-6.35,3.24,-2.99},
					{1.238,2.12,-2.21},
					{3.45,0.98,0},
					{2.36,-1.098,-2},
					{-2.3,1.05,4},
					{4,-5.24,7}};

	float B[k] = {-1,1,3.1};

	for(int i=0;i<k;i++)
	{
		ap_float Btemp;
		Btemp.data = B[i];
		instream << Btemp;
	}

	for(int i=0;i<N;i++)
		for(int j=0;j<k;j++)
		{
			ap_float Atemp;
			Atemp.data = A[i][j];
			instream << Atemp;
		}

	multiply(instream,outstream,idx);

	for(int i=0;i<N;i++)
	{
		ap_float val;
		outstream >> val;
		printf("%f ", val.data);
	}
	printf("Maximum index is %f ", idx);

	return 0;
}