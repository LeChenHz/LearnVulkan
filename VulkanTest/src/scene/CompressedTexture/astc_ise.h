#ifndef IMAGEUTIL_ASTC_ASTC_ISE_HPP_
#define IMAGEUTIL_ASTC_ASTC_ISE_HPP_

#include <cstdint>

namespace angle
{
	namespace astc
	{

		enum quantization_method
		{
			QUANT_2 = 0,
			QUANT_3 = 1,
			QUANT_4 = 2,
			QUANT_5 = 3,
			QUANT_6 = 4,
			QUANT_8 = 5,
			QUANT_10 = 6,
			QUANT_12 = 7,
			QUANT_16 = 8,
			QUANT_20 = 9,
			QUANT_24 = 10,
			QUANT_32 = 11,
			QUANT_40 = 12,
			QUANT_48 = 13,
			QUANT_64 = 14,
			QUANT_80 = 15,
			QUANT_96 = 16,
			QUANT_128 = 17,
			QUANT_160 = 18,
			QUANT_192 = 19,
			QUANT_256 = 20
		};


		inline void find_number_of_bits_trits_quints(quantization_method quantization_level, int* bits, int* trits, int* quints)
		{
#if 0
			static const int DT[][4] = {
				{ QUANT_2, 1, 0, 0 },
				{ QUANT_3, 0, 1, 0 },
				{ QUANT_4, 2, 0, 0 },
				{ QUANT_5, 0, 0, 1 },
				{ QUANT_6, 1, 1, 0 },
				{ QUANT_8, 3, 0, 0 },
				{ QUANT_10, 1, 0, 1 },
				{ QUANT_12, 2, 1, 0 },
				{ QUANT_16, 4, 0, 0 },
				{ QUANT_20, 2, 0, 1 },
				{ QUANT_24, 3, 1, 0 },
				{ QUANT_32, 5, 0, 0 },
				{ QUANT_40, 3, 0, 1 },
				{ QUANT_48, 4, 1, 0 },
				{ QUANT_64, 6, 0, 0 },
				{ QUANT_80, 4, 0, 1 },
				{ QUANT_96, 5, 1, 0 },
				{ QUANT_128, 7, 0, 0 },
				{ QUANT_160, 5, 0, 1 },
				{ QUANT_192, 6, 1, 0 },
				{ QUANT_256, 8, 0, 0 },
			};

			*bits = DT[quantization_level][1];
			*trits = DT[quantization_level][2];
			*quints = DT[quantization_level][3];
#else
			* bits = 0;
			*trits = 0;
			*quints = 0;
			switch (quantization_level)
			{
			case QUANT_2:
				*bits = 1;
				break;
			case QUANT_3:
				*bits = 0;
				*trits = 1;
				break;
			case QUANT_4:
				*bits = 2;
				break;
			case QUANT_5:
				*bits = 0;
				*quints = 1;
				break;
			case QUANT_6:
				*bits = 1;
				*trits = 1;
				break;
			case QUANT_8:
				*bits = 3;
				break;
			case QUANT_10:
				*bits = 1;
				*quints = 1;
				break;
			case QUANT_12:
				*bits = 2;
				*trits = 1;
				break;
			case QUANT_16:
				*bits = 4;
				break;
			case QUANT_20:
				*bits = 2;
				*quints = 1;
				break;
			case QUANT_24:
				*bits = 3;
				*trits = 1;
				break;
			case QUANT_32:
				*bits = 5;
				break;
			case QUANT_40:
				*bits = 3;
				*quints = 1;
				break;
			case QUANT_48:
				*bits = 4;
				*trits = 1;
				break;
			case QUANT_64:
				*bits = 6;
				break;
			case QUANT_80:
				*bits = 4;
				*quints = 1;
				break;
			case QUANT_96:
				*bits = 5;
				*trits = 1;
				break;
			case QUANT_128:
				*bits = 7;
				break;
			case QUANT_160:
				*bits = 5;
				*quints = 1;
				break;
			case QUANT_192:
				*bits = 6;
				*trits = 1;
				break;
			case QUANT_256:
				*bits = 8;
				break;
			}
#endif
		}

		inline int compute_ise_bitcount(int items, quantization_method quant)
		{
			switch (quant)
			{
			case QUANT_2:
				return items;
			case QUANT_3:
				return (8 * items + 4) / 5;
			case QUANT_4:
				return 2 * items;
			case QUANT_5:
				return (7 * items + 2) / 3;
			case QUANT_6:
				return (13 * items + 4) / 5;
			case QUANT_8:
				return 3 * items;
			case QUANT_10:
				return (10 * items + 2) / 3;
			case QUANT_12:
				return (18 * items + 4) / 5;
			case QUANT_16:
				return items * 4;
			case QUANT_20:
				return (13 * items + 2) / 3;
			case QUANT_24:
				return (23 * items + 4) / 5;
			case QUANT_32:
				return 5 * items;
			case QUANT_40:
				return (16 * items + 2) / 3;
			case QUANT_48:
				return (28 * items + 4) / 5;
			case QUANT_64:
				return 6 * items;
			case QUANT_80:
				return (19 * items + 2) / 3;
			case QUANT_96:
				return (33 * items + 4) / 5;
			case QUANT_128:
				return 7 * items;
			case QUANT_160:
				return (22 * items + 2) / 3;
			case QUANT_192:
				return (38 * items + 4) / 5;
			case QUANT_256:
				return 8 * items;
			default:
				return 100000;
			}
		}

		void decode_ise(quantization_method quantization_level, int elements, const uint8_t* input_data, uint8_t* output_data, int bit_offset);
	}
}

#endif