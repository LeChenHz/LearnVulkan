#ifndef IMAGEUTIL_ASTC_ASTC_PERCENTILE_TABLE_H_
#define IMAGEUTIL_ASTC_ASTC_PERCENTILE_TABLE_H_

namespace angle
{
	namespace astc
	{
		const float* get_2d_percentile_table(int blockdim_x, int blockdim_y);
	}
}

#endif