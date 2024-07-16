#include <stdio.h>

#include "dct_math.h"
#include "util.h"

/*
* This implementation is based on an algorithm described in
*   C. Loeffler, A. Ligtenberg and G. Moschytz, "Practical Fast 1-D DCT
*   Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
*   Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
* The primary algorithm described there uses 11 multiplies and 29 adds.
* We use their alternate method with 12 multiplies and 32 adds.
* The advantage is that no data path contains more than one multiplication.
*/
/* normalize the result between 0 and 255 */
/* this is required to handle precision errors that might cause the decoded result to fall out of range */
#define NORMALIZE(x) (temp = (x), ( (temp < 0) ? 0 : ( (temp > 255) ? 255 : temp  ) ) )

void idct(int16_t DCAC[DCTSIZE][DCTSIZE], uint8_t block_r[DCTSIZE][DCTSIZE])
{
    #pragma HLS interface ap_ctrl_none port=return
    #pragma HLS interface axis register both port=DCAC
    #pragma HLS interface axis register both port=block_r
    #pragma HLS array_reshape variable=DCAC cyclic factor=2 dim=2
    #pragma HLS array_reshape variable=block_r cyclic factor=4 dim=2

    int32_t tmp0, tmp1, tmp2, tmp3;
    int32_t tmp10, tmp11, tmp12, tmp13;
    int32_t z1, z2, z3, z4, z5;
    int32_t temp;
    int32_t workspace[DCTSIZE][DCTSIZE];
    int16_t dcac_tmp[DCTSIZE][DCTSIZE];
    SHIFT_TEMPS

	#pragma HLS array_reshape variable=workspace cyclic factor=1 dim=0
	#pragma HLS array_reshape variable=dcac_tmp cyclic factor=1 dim=0

    #pragma HLS dataflow

    dcac_row_copy: for (int row = 0; row < DCTSIZE; row++)
        dcac_col_copy: for (int col = 0; col < DCTSIZE; col++)
            #pragma HLS pipeline
            #pragma HLS unroll factor=2 skip_exit_check
            dcac_tmp[row][col] = DCAC[row][col];

    /* Pass 1: process columns from input, store into work array. */
    /* Note results are scaled up by sqrt(8) compared to a true IDCT; */
    /* furthermore, we scale the results by 2**PASS1_BITS. */

    idct_column_loop: for (int col = 0; col < DCTSIZE; col++) {
        #pragma HLS pipeline
        /* Even part: reverse the even part of the forward DCT. */
        /* The rotator is sqrt(2)*c(-6). */

        z2 = dcac_tmp[2][col];                       /* S4, y2 */
        z3 = dcac_tmp[6][col];                       /* S4, y6 */

        z1   = MULTIPLY(z2 + z3, FIX_0_541196100);
        tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065); /* S3, y2 */
        tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);   /* S3, y6 */

        z2 = dcac_tmp[0][col];                       /* S4, y0 */
        z3 = dcac_tmp[4][col];                       /* S4, y4 */

        tmp0 = (z2 + z3) << CONST_BITS;              /* S3, y0 */
        tmp1 = (z2 - z3) << CONST_BITS;              /* S3, y4 */

        tmp10 = tmp0 + tmp3;                         /* S2, y0 */
        tmp13 = tmp0 - tmp3;                         /* S2, y6 */
        tmp11 = tmp1 + tmp2;                         /* S2, y4 */
        tmp12 = tmp1 - tmp2;                         /* S2, y2 */

        /* Odd part per figure 8; the matrix is unitary and hence its
         * transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
         */

        tmp0 = dcac_tmp[7][col];                     /* i0 */
        tmp1 = dcac_tmp[5][col];                     /* i1 */
        tmp2 = dcac_tmp[3][col];                     /* i2 */
        tmp3 = dcac_tmp[1][col];                     /* i3 */

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = MULTIPLY(z3 + z4, FIX_1_175875602);     /* i */

        tmp0 = MULTIPLY(tmp0, FIX_0_298631336);      /* a */
        tmp1 = MULTIPLY(tmp1, FIX_2_053119869);      /* b */
        tmp2 = MULTIPLY(tmp2, FIX_3_072711026);      /* c */
        tmp3 = MULTIPLY(tmp3, FIX_1_501321110);      /* d */
        z1   = MULTIPLY(z1, - FIX_0_899976223);      /* e */
        z2   = MULTIPLY(z2, - FIX_2_562915447);      /* f */
        z3   = MULTIPLY(z3, - FIX_1_961570560);      /* g */
        z4   = MULTIPLY(z4, - FIX_0_390180644);      /* h */

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;                             /* y7 */
        tmp1 += z2 + z4;                             /* y5 */
        tmp2 += z2 + z3;                             /* y3 */
        tmp3 += z1 + z4;                             /* y1 */

        /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

        workspace[0][col] = (int32_t) DESCALE(tmp10 + tmp3, CONST_BITS-PASS1_BITS);
        workspace[7][col] = (int32_t) DESCALE(tmp10 - tmp3, CONST_BITS-PASS1_BITS);
        workspace[1][col] = (int32_t) DESCALE(tmp11 + tmp2, CONST_BITS-PASS1_BITS);
        workspace[6][col] = (int32_t) DESCALE(tmp11 - tmp2, CONST_BITS-PASS1_BITS);
        workspace[2][col] = (int32_t) DESCALE(tmp12 + tmp1, CONST_BITS-PASS1_BITS);
        workspace[5][col] = (int32_t) DESCALE(tmp12 - tmp1, CONST_BITS-PASS1_BITS);
        workspace[3][col] = (int32_t) DESCALE(tmp13 + tmp0, CONST_BITS-PASS1_BITS);
        workspace[4][col] = (int32_t) DESCALE(tmp13 - tmp0, CONST_BITS-PASS1_BITS);
    }

    /* Pass 2: process rows from work array, store into output array. */
    /* Note that we must descale the results by a factor of 8 == 2**3, */
    /* and also undo the PASS1_BITS scaling. */

    idct_row_loop: for (int row = 0; row < DCTSIZE; row++) {
        #pragma HLS pipeline
        /* Even part: reverse the even part of the forward DCT. */
        /* The rotator is sqrt(2)*c(-6). */

        z2 = (int32_t) workspace[row][2];
        z3 = (int32_t) workspace[row][6];

        z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
        tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
        tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);

        tmp0 = ((int32_t) workspace[row][0] + (int32_t) workspace[row][4]) << CONST_BITS;
        tmp1 = ((int32_t) workspace[row][0] - (int32_t) workspace[row][4]) << CONST_BITS;

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
         * transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
         */

        tmp0 = (int32_t) workspace[row][7];
        tmp1 = (int32_t) workspace[row][5];
        tmp2 = (int32_t) workspace[row][3];
        tmp3 = (int32_t) workspace[row][1];

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = MULTIPLY(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */

        tmp0 = MULTIPLY(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp1 = MULTIPLY(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp2 = MULTIPLY(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp3 = MULTIPLY(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */

        z1 = MULTIPLY(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
        z2 = MULTIPLY(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
        z3 = MULTIPLY(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
        z4 = MULTIPLY(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

        block_r[row][0] = NORMALIZE(DESCALE(tmp10 + tmp3, CONST_BITS+PASS1_BITS+3));
        block_r[row][1] = NORMALIZE(DESCALE(tmp11 + tmp2, CONST_BITS+PASS1_BITS+3));
        block_r[row][2] = NORMALIZE(DESCALE(tmp12 + tmp1, CONST_BITS+PASS1_BITS+3));
        block_r[row][3] = NORMALIZE(DESCALE(tmp13 + tmp0, CONST_BITS+PASS1_BITS+3));
        block_r[row][4] = NORMALIZE(DESCALE(tmp13 - tmp0, CONST_BITS+PASS1_BITS+3));
        block_r[row][5] = NORMALIZE(DESCALE(tmp12 - tmp1, CONST_BITS+PASS1_BITS+3));
        block_r[row][6] = NORMALIZE(DESCALE(tmp11 - tmp2, CONST_BITS+PASS1_BITS+3));
        block_r[row][7] = NORMALIZE(DESCALE(tmp10 - tmp3, CONST_BITS+PASS1_BITS+3));
    }
}
