#ifndef IMAGEUTIL_ASTC_ASTC_PARTITION_TABLE_H_
#define IMAGEUTIL_ASTC_ASTC_PARTITION_TABLE_H_

#include <cstdint>

#include "astc_block_size_descriptor.h"

namespace angle
{
	namespace astc
	{

#define PARTITION_BITS 10
#define PARTITION_COUNT (1 << PARTITION_BITS)

		/*
		Partition table representation:
		For each block size, we have 3 tables, each with 1024 partitionings;
		these three tables correspond to 2, 3 and 4 partitions respectively.
		For each partitioning, we have:
		* a 4-entry table indicating how many texels there are in each of the 4 partitions.
		This may be from 0 to a very large value.
		* a table indicating the partition index of each of the texels in the block.
		Each index may be 0, 1, 2 or 3.
		* Each element in the table is an uint8_t indicating partition index (0, 1, 2 or 3)
		*/

		struct partition_info
		{
			int partition_count;
			uint8_t texels_per_partition[4];
			uint8_t partition_of_texel[MAX_TEXELS_PER_BLOCK];
			uint8_t texels_of_partition[4][MAX_TEXELS_PER_BLOCK];

			uint64_t coverage_bitmaps[4];	// used for the purposes of k-means partition search.
		};

		const partition_info* get_partition_table(int xdim, int ydim, int partition_count);

	}
}

#endif