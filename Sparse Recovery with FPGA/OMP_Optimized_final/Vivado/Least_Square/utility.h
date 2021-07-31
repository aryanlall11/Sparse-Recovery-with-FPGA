#include "hls_linear_algebra.h"

typedef float T;
#define M 25
#define k 232

template <typename Tc, bool isTranspose>
void bs_subs(Tc Z[M], Tc L[M][M], Tc P[M], int index) {

	Tc sum, ele, temp;
	int idx, i_final, j_final;
	idx = isTranspose?(index-1):0;

	Z[idx] = P[idx]/L[idx][idx];

	L_i: for (int i = 1; i < index; i++)
		{
			sum = Tc(0);
			i_final = isTranspose?(idx-i):i;
		    L_j: for (int j = 0; j < i; j++)
			{
				#pragma HLS PIPELINE
				sum += isTranspose?(Z[idx-j]*L[idx-j][idx-i]):(Z[j]*L[i][j]);
			}
			Z[i_final] = (P[i_final] - sum)/L[i_final][i_final];
		}
}
