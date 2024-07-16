#include <iostream>
#include <stdint.h>

#include "../hls_src/dct_math.h"

static int16_t dcac_input_1[DCTSIZE][DCTSIZE] = {
	{ 100,   0,   0,   0,   0,   0,   0,   0 },
	{   0, 100,   0,   0,   0,   0,   0,   0 },
	{   0,   0, 100,   0,   0,   0,   0,   0 },
	{   0,   0,   0, 100,   0,   0,   0,   0 },
	{   0,   0,   0,   0, 100,   0,   0,   0 },
	{   0,   0,   0,   0,   0, 100,   0,   0 },
	{   0,   0,   0,   0,   0,   0, 100,   0 },
	{   0,   0,   0,   0,   0,   0,   0, 100 },
};

static uint8_t block_expected_1[DCTSIZE][DCTSIZE] = {
	{ 100,   0,   0,   0,   0,   0,   0,   0 },
	{   0, 100,   0,   0,   0,   0,   0,   0 },
	{   0,   0, 100,   0,   0,   0,   0,   0 },
	{   0,   0,   0, 100,   0,   0,   0,   0 },
	{   0,   0,   0,   0, 100,   0,   0,   0 },
	{   0,   0,   0,   0,   0, 100,   0,   0 },
	{   0,   0,   0,   0,   0,   0, 100,   0 },
	{   0,   0,   0,   0,   0,   0,   0, 100 },
};

static int16_t dcac_input_2[DCTSIZE][DCTSIZE] = {
	{ 1240,   0, -10,   0,   0,   0,   0,   0 },
	{  -24, -12,   0,   0,   0,   0,   0,   0 },
	{  -14, -13,   0,   0,   0,   0,   0,   0 },
	{    0,   0,   0,   0,   0,   0,   0,   0 },
	{    0,   0,   0,   0,   0,   0,   0,   0 },
	{    0,   0,   0,   0,   0,   0,   0,   0 },
	{    0,   0,   0,   0,   0,   0,   0,   0 },
	{    0,   0,   0,   0,   0,   0,   0,   0 },
};

static uint8_t block_expected_2[DCTSIZE][DCTSIZE] = {
	{ 141, 143, 146, 149, 151, 153, 153, 153 },
	{ 145, 147, 149, 151, 153, 153, 153, 153 },
	{ 152, 153, 154, 155, 155, 155, 153, 152 },
	{ 157, 158, 158, 159, 158, 156, 154, 152 },
	{ 160, 160, 161, 160, 159, 157, 154, 153 },
	{ 160, 160, 161, 161, 159, 157, 155, 154 },
	{ 157, 158, 159, 159, 159, 158, 156, 155 },
    { 155, 156, 158, 158, 159, 158, 156, 155 },
};

void idct(int16_t DCAC[DCTSIZE][DCTSIZE],
		  uint8_t block[DCTSIZE][DCTSIZE]);

static int test(int16_t dcac_input[DCTSIZE][DCTSIZE],
				uint8_t block_expected[DCTSIZE][DCTSIZE],
				uint8_t block_output[DCTSIZE][DCTSIZE])
{
	int fail = 0;
	for (int row = 0; row < DCTSIZE; row++)
		for (int col = 0; col < DCTSIZE; col++) {
			if (block_output[row][col] != block_expected[row][col]) {
				std::cout << "block[" << row << "][" << col << "] expected " << \
						     (unsigned int)block_expected[row][col] << ", but got " << \
							 (unsigned int)block_output[row][col] << std::endl;
				fail = fail || 1;
			}
		}

	return fail;
}

int main(int argc, const char *argv[])
{
	int fail = 0;
	static uint8_t block_output_1[DCTSIZE][DCTSIZE];
	static uint8_t block_output_2[DCTSIZE][DCTSIZE];

	idct(dcac_input_1, block_output_1);
	idct(dcac_input_2, block_output_2);

	std::cout << "Test 1" << std::endl;
	fail = fail || test(dcac_input_1, block_expected_1, block_output_1);
	std::cout << "Test 2" << std::endl;
	fail = fail || test(dcac_input_2, block_expected_2, block_output_2);


	return fail;
}
