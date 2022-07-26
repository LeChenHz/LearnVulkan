#ifndef IMAGEUTIL_ASTC_ASTC_BLOCK_SIZE_DESCRIPTOR_H_
#define IMAGEUTIL_ASTC_ASTC_BLOCK_SIZE_DESCRIPTOR_H_

#include <cstdint>
namespace angle
{
	namespace astc
	{

#define MAX_WEIGHTS_PER_BLOCK 64
#define MAX_DECIMATION_MODES 87
#define MAX_WEIGHT_MODES 2048
#define MAX_TEXELS_PER_BLOCK (12 * 12)
#define TEXEL_WEIGHT_SUM 16
#define MIN_WEIGHT_BITS_PER_BLOCK 24
#define MAX_WEIGHT_BITS_PER_BLOCK 96

		/*
		In ASTC, we don't necessarily provide a weight for every texel.
		As such, for each block size, there are a number of patterns where some texels
		have their weights computed as a weighted average of more than 1 weight.
		As such, the codec uses a data structure that tells us: for each texel, which
		weights it is a combination of for each weight, which texels it contributes to.
		The decimation_table is this data structure.
		*/
		struct decimation_table
		{
			int num_texels;
			int num_weights;
			uint8_t texel_num_weights[MAX_TEXELS_PER_BLOCK];	// number of indices that go into the calculation for a texel
			uint8_t texel_weights_int[MAX_TEXELS_PER_BLOCK][4];	// the weight to assign to each weight
			float texel_weights_float[MAX_TEXELS_PER_BLOCK][4];	// the weight to assign to each weight
			uint8_t texel_weights[MAX_TEXELS_PER_BLOCK][4];	// the weights that go into a texel calculation
			uint8_t weight_num_texels[MAX_WEIGHTS_PER_BLOCK];	// the number of texels that a given weight contributes to
			uint8_t weight_texel[MAX_WEIGHTS_PER_BLOCK][MAX_TEXELS_PER_BLOCK];	// the texels that the weight contributes to
			uint8_t weights_int[MAX_WEIGHTS_PER_BLOCK][MAX_TEXELS_PER_BLOCK];	// the weights that the weight contributes to a texel.
			float weights_flt[MAX_WEIGHTS_PER_BLOCK][MAX_TEXELS_PER_BLOCK];	// the weights that the weight contributes to a texel.
		};

		/*
		data structure describing information that pertains to a block size and its associated block modes.
		*/
		struct block_mode
		{
			int8_t decimation_mode;
			int8_t quantization_mode;
			int8_t is_dual_plane;
			int8_t permit_encode;
			int8_t permit_decode;
			float percentile;
		};

		struct block_size_descriptor
		{
			int decimation_mode_count;
			int decimation_mode_samples[MAX_DECIMATION_MODES];
			int decimation_mode_maxprec_1plane[MAX_DECIMATION_MODES];
			int decimation_mode_maxprec_2planes[MAX_DECIMATION_MODES];
			float decimation_mode_percentile[MAX_DECIMATION_MODES];
			int permit_encode[MAX_DECIMATION_MODES];
			const decimation_table* decimation_tables[MAX_DECIMATION_MODES + 1];
			block_mode block_modes[MAX_WEIGHT_MODES];

			// for the k-means bed bitmap partitioning algorithm, we don't
			// want to consider more than 64 texels; this array specifies
			// which 64 texels (if that many) to consider.
			int texelcount_for_bitmap_partitioning;
			int texels_for_bitmap_partitioning[64];
		};

		const block_size_descriptor* get_block_size_descriptor(int xdim, int ydim);

	}
}

#endif