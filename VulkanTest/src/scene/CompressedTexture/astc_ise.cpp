#include "astc_ise.h"
#include "astc_bit_manipulation.h"
#include <chrono>
#include <cstdio>
#include <string>

namespace angle
{
	namespace astc
	{

		namespace {

			const uint8_t trits_of_integer[256][5] = {
				{ 0, 0, 0, 0, 0 },{ 1, 0, 0, 0, 0 },{ 2, 0, 0, 0, 0 },{ 0, 0, 2, 0, 0 },
				{ 0, 1, 0, 0, 0 },{ 1, 1, 0, 0, 0 },{ 2, 1, 0, 0, 0 },{ 1, 0, 2, 0, 0 },
				{ 0, 2, 0, 0, 0 },{ 1, 2, 0, 0, 0 },{ 2, 2, 0, 0, 0 },{ 2, 0, 2, 0, 0 },
				{ 0, 2, 2, 0, 0 },{ 1, 2, 2, 0, 0 },{ 2, 2, 2, 0, 0 },{ 2, 0, 2, 0, 0 },
				{ 0, 0, 1, 0, 0 },{ 1, 0, 1, 0, 0 },{ 2, 0, 1, 0, 0 },{ 0, 1, 2, 0, 0 },
				{ 0, 1, 1, 0, 0 },{ 1, 1, 1, 0, 0 },{ 2, 1, 1, 0, 0 },{ 1, 1, 2, 0, 0 },
				{ 0, 2, 1, 0, 0 },{ 1, 2, 1, 0, 0 },{ 2, 2, 1, 0, 0 },{ 2, 1, 2, 0, 0 },
				{ 0, 0, 0, 2, 2 },{ 1, 0, 0, 2, 2 },{ 2, 0, 0, 2, 2 },{ 0, 0, 2, 2, 2 },
				{ 0, 0, 0, 1, 0 },{ 1, 0, 0, 1, 0 },{ 2, 0, 0, 1, 0 },{ 0, 0, 2, 1, 0 },
				{ 0, 1, 0, 1, 0 },{ 1, 1, 0, 1, 0 },{ 2, 1, 0, 1, 0 },{ 1, 0, 2, 1, 0 },
				{ 0, 2, 0, 1, 0 },{ 1, 2, 0, 1, 0 },{ 2, 2, 0, 1, 0 },{ 2, 0, 2, 1, 0 },
				{ 0, 2, 2, 1, 0 },{ 1, 2, 2, 1, 0 },{ 2, 2, 2, 1, 0 },{ 2, 0, 2, 1, 0 },
				{ 0, 0, 1, 1, 0 },{ 1, 0, 1, 1, 0 },{ 2, 0, 1, 1, 0 },{ 0, 1, 2, 1, 0 },
				{ 0, 1, 1, 1, 0 },{ 1, 1, 1, 1, 0 },{ 2, 1, 1, 1, 0 },{ 1, 1, 2, 1, 0 },
				{ 0, 2, 1, 1, 0 },{ 1, 2, 1, 1, 0 },{ 2, 2, 1, 1, 0 },{ 2, 1, 2, 1, 0 },
				{ 0, 1, 0, 2, 2 },{ 1, 1, 0, 2, 2 },{ 2, 1, 0, 2, 2 },{ 1, 0, 2, 2, 2 },
				{ 0, 0, 0, 2, 0 },{ 1, 0, 0, 2, 0 },{ 2, 0, 0, 2, 0 },{ 0, 0, 2, 2, 0 },
				{ 0, 1, 0, 2, 0 },{ 1, 1, 0, 2, 0 },{ 2, 1, 0, 2, 0 },{ 1, 0, 2, 2, 0 },
				{ 0, 2, 0, 2, 0 },{ 1, 2, 0, 2, 0 },{ 2, 2, 0, 2, 0 },{ 2, 0, 2, 2, 0 },
				{ 0, 2, 2, 2, 0 },{ 1, 2, 2, 2, 0 },{ 2, 2, 2, 2, 0 },{ 2, 0, 2, 2, 0 },
				{ 0, 0, 1, 2, 0 },{ 1, 0, 1, 2, 0 },{ 2, 0, 1, 2, 0 },{ 0, 1, 2, 2, 0 },
				{ 0, 1, 1, 2, 0 },{ 1, 1, 1, 2, 0 },{ 2, 1, 1, 2, 0 },{ 1, 1, 2, 2, 0 },
				{ 0, 2, 1, 2, 0 },{ 1, 2, 1, 2, 0 },{ 2, 2, 1, 2, 0 },{ 2, 1, 2, 2, 0 },
				{ 0, 2, 0, 2, 2 },{ 1, 2, 0, 2, 2 },{ 2, 2, 0, 2, 2 },{ 2, 0, 2, 2, 2 },
				{ 0, 0, 0, 0, 2 },{ 1, 0, 0, 0, 2 },{ 2, 0, 0, 0, 2 },{ 0, 0, 2, 0, 2 },
				{ 0, 1, 0, 0, 2 },{ 1, 1, 0, 0, 2 },{ 2, 1, 0, 0, 2 },{ 1, 0, 2, 0, 2 },
				{ 0, 2, 0, 0, 2 },{ 1, 2, 0, 0, 2 },{ 2, 2, 0, 0, 2 },{ 2, 0, 2, 0, 2 },
				{ 0, 2, 2, 0, 2 },{ 1, 2, 2, 0, 2 },{ 2, 2, 2, 0, 2 },{ 2, 0, 2, 0, 2 },
				{ 0, 0, 1, 0, 2 },{ 1, 0, 1, 0, 2 },{ 2, 0, 1, 0, 2 },{ 0, 1, 2, 0, 2 },
				{ 0, 1, 1, 0, 2 },{ 1, 1, 1, 0, 2 },{ 2, 1, 1, 0, 2 },{ 1, 1, 2, 0, 2 },
				{ 0, 2, 1, 0, 2 },{ 1, 2, 1, 0, 2 },{ 2, 2, 1, 0, 2 },{ 2, 1, 2, 0, 2 },
				{ 0, 2, 2, 2, 2 },{ 1, 2, 2, 2, 2 },{ 2, 2, 2, 2, 2 },{ 2, 0, 2, 2, 2 },
				{ 0, 0, 0, 0, 1 },{ 1, 0, 0, 0, 1 },{ 2, 0, 0, 0, 1 },{ 0, 0, 2, 0, 1 },
				{ 0, 1, 0, 0, 1 },{ 1, 1, 0, 0, 1 },{ 2, 1, 0, 0, 1 },{ 1, 0, 2, 0, 1 },
				{ 0, 2, 0, 0, 1 },{ 1, 2, 0, 0, 1 },{ 2, 2, 0, 0, 1 },{ 2, 0, 2, 0, 1 },
				{ 0, 2, 2, 0, 1 },{ 1, 2, 2, 0, 1 },{ 2, 2, 2, 0, 1 },{ 2, 0, 2, 0, 1 },
				{ 0, 0, 1, 0, 1 },{ 1, 0, 1, 0, 1 },{ 2, 0, 1, 0, 1 },{ 0, 1, 2, 0, 1 },
				{ 0, 1, 1, 0, 1 },{ 1, 1, 1, 0, 1 },{ 2, 1, 1, 0, 1 },{ 1, 1, 2, 0, 1 },
				{ 0, 2, 1, 0, 1 },{ 1, 2, 1, 0, 1 },{ 2, 2, 1, 0, 1 },{ 2, 1, 2, 0, 1 },
				{ 0, 0, 1, 2, 2 },{ 1, 0, 1, 2, 2 },{ 2, 0, 1, 2, 2 },{ 0, 1, 2, 2, 2 },
				{ 0, 0, 0, 1, 1 },{ 1, 0, 0, 1, 1 },{ 2, 0, 0, 1, 1 },{ 0, 0, 2, 1, 1 },
				{ 0, 1, 0, 1, 1 },{ 1, 1, 0, 1, 1 },{ 2, 1, 0, 1, 1 },{ 1, 0, 2, 1, 1 },
				{ 0, 2, 0, 1, 1 },{ 1, 2, 0, 1, 1 },{ 2, 2, 0, 1, 1 },{ 2, 0, 2, 1, 1 },
				{ 0, 2, 2, 1, 1 },{ 1, 2, 2, 1, 1 },{ 2, 2, 2, 1, 1 },{ 2, 0, 2, 1, 1 },
				{ 0, 0, 1, 1, 1 },{ 1, 0, 1, 1, 1 },{ 2, 0, 1, 1, 1 },{ 0, 1, 2, 1, 1 },
				{ 0, 1, 1, 1, 1 },{ 1, 1, 1, 1, 1 },{ 2, 1, 1, 1, 1 },{ 1, 1, 2, 1, 1 },
				{ 0, 2, 1, 1, 1 },{ 1, 2, 1, 1, 1 },{ 2, 2, 1, 1, 1 },{ 2, 1, 2, 1, 1 },
				{ 0, 1, 1, 2, 2 },{ 1, 1, 1, 2, 2 },{ 2, 1, 1, 2, 2 },{ 1, 1, 2, 2, 2 },
				{ 0, 0, 0, 2, 1 },{ 1, 0, 0, 2, 1 },{ 2, 0, 0, 2, 1 },{ 0, 0, 2, 2, 1 },
				{ 0, 1, 0, 2, 1 },{ 1, 1, 0, 2, 1 },{ 2, 1, 0, 2, 1 },{ 1, 0, 2, 2, 1 },
				{ 0, 2, 0, 2, 1 },{ 1, 2, 0, 2, 1 },{ 2, 2, 0, 2, 1 },{ 2, 0, 2, 2, 1 },
				{ 0, 2, 2, 2, 1 },{ 1, 2, 2, 2, 1 },{ 2, 2, 2, 2, 1 },{ 2, 0, 2, 2, 1 },
				{ 0, 0, 1, 2, 1 },{ 1, 0, 1, 2, 1 },{ 2, 0, 1, 2, 1 },{ 0, 1, 2, 2, 1 },
				{ 0, 1, 1, 2, 1 },{ 1, 1, 1, 2, 1 },{ 2, 1, 1, 2, 1 },{ 1, 1, 2, 2, 1 },
				{ 0, 2, 1, 2, 1 },{ 1, 2, 1, 2, 1 },{ 2, 2, 1, 2, 1 },{ 2, 1, 2, 2, 1 },
				{ 0, 2, 1, 2, 2 },{ 1, 2, 1, 2, 2 },{ 2, 2, 1, 2, 2 },{ 2, 1, 2, 2, 2 },
				{ 0, 0, 0, 1, 2 },{ 1, 0, 0, 1, 2 },{ 2, 0, 0, 1, 2 },{ 0, 0, 2, 1, 2 },
				{ 0, 1, 0, 1, 2 },{ 1, 1, 0, 1, 2 },{ 2, 1, 0, 1, 2 },{ 1, 0, 2, 1, 2 },
				{ 0, 2, 0, 1, 2 },{ 1, 2, 0, 1, 2 },{ 2, 2, 0, 1, 2 },{ 2, 0, 2, 1, 2 },
				{ 0, 2, 2, 1, 2 },{ 1, 2, 2, 1, 2 },{ 2, 2, 2, 1, 2 },{ 2, 0, 2, 1, 2 },
				{ 0, 0, 1, 1, 2 },{ 1, 0, 1, 1, 2 },{ 2, 0, 1, 1, 2 },{ 0, 1, 2, 1, 2 },
				{ 0, 1, 1, 1, 2 },{ 1, 1, 1, 1, 2 },{ 2, 1, 1, 1, 2 },{ 1, 1, 2, 1, 2 },
				{ 0, 2, 1, 1, 2 },{ 1, 2, 1, 1, 2 },{ 2, 2, 1, 1, 2 },{ 2, 1, 2, 1, 2 },
				{ 0, 2, 2, 2, 2 },{ 1, 2, 2, 2, 2 },{ 2, 2, 2, 2, 2 },{ 2, 1, 2, 2, 2 },
			};

			static const uint8_t quints_of_integer[128][3] = {
				{ 0, 0, 0 },{ 1, 0, 0 },{ 2, 0, 0 },{ 3, 0, 0 },
				{ 4, 0, 0 },{ 0, 4, 0 },{ 4, 4, 0 },{ 4, 4, 4 },
				{ 0, 1, 0 },{ 1, 1, 0 },{ 2, 1, 0 },{ 3, 1, 0 },
				{ 4, 1, 0 },{ 1, 4, 0 },{ 4, 4, 1 },{ 4, 4, 4 },
				{ 0, 2, 0 },{ 1, 2, 0 },{ 2, 2, 0 },{ 3, 2, 0 },
				{ 4, 2, 0 },{ 2, 4, 0 },{ 4, 4, 2 },{ 4, 4, 4 },
				{ 0, 3, 0 },{ 1, 3, 0 },{ 2, 3, 0 },{ 3, 3, 0 },
				{ 4, 3, 0 },{ 3, 4, 0 },{ 4, 4, 3 },{ 4, 4, 4 },
				{ 0, 0, 1 },{ 1, 0, 1 },{ 2, 0, 1 },{ 3, 0, 1 },
				{ 4, 0, 1 },{ 0, 4, 1 },{ 4, 0, 4 },{ 0, 4, 4 },
				{ 0, 1, 1 },{ 1, 1, 1 },{ 2, 1, 1 },{ 3, 1, 1 },
				{ 4, 1, 1 },{ 1, 4, 1 },{ 4, 1, 4 },{ 1, 4, 4 },
				{ 0, 2, 1 },{ 1, 2, 1 },{ 2, 2, 1 },{ 3, 2, 1 },
				{ 4, 2, 1 },{ 2, 4, 1 },{ 4, 2, 4 },{ 2, 4, 4 },
				{ 0, 3, 1 },{ 1, 3, 1 },{ 2, 3, 1 },{ 3, 3, 1 },
				{ 4, 3, 1 },{ 3, 4, 1 },{ 4, 3, 4 },{ 3, 4, 4 },
				{ 0, 0, 2 },{ 1, 0, 2 },{ 2, 0, 2 },{ 3, 0, 2 },
				{ 4, 0, 2 },{ 0, 4, 2 },{ 2, 0, 4 },{ 3, 0, 4 },
				{ 0, 1, 2 },{ 1, 1, 2 },{ 2, 1, 2 },{ 3, 1, 2 },
				{ 4, 1, 2 },{ 1, 4, 2 },{ 2, 1, 4 },{ 3, 1, 4 },
				{ 0, 2, 2 },{ 1, 2, 2 },{ 2, 2, 2 },{ 3, 2, 2 },
				{ 4, 2, 2 },{ 2, 4, 2 },{ 2, 2, 4 },{ 3, 2, 4 },
				{ 0, 3, 2 },{ 1, 3, 2 },{ 2, 3, 2 },{ 3, 3, 2 },
				{ 4, 3, 2 },{ 3, 4, 2 },{ 2, 3, 4 },{ 3, 3, 4 },
				{ 0, 0, 3 },{ 1, 0, 3 },{ 2, 0, 3 },{ 3, 0, 3 },
				{ 4, 0, 3 },{ 0, 4, 3 },{ 0, 0, 4 },{ 1, 0, 4 },
				{ 0, 1, 3 },{ 1, 1, 3 },{ 2, 1, 3 },{ 3, 1, 3 },
				{ 4, 1, 3 },{ 1, 4, 3 },{ 0, 1, 4 },{ 1, 1, 4 },
				{ 0, 2, 3 },{ 1, 2, 3 },{ 2, 2, 3 },{ 3, 2, 3 },
				{ 4, 2, 3 },{ 2, 4, 3 },{ 0, 2, 4 },{ 1, 2, 4 },
				{ 0, 3, 3 },{ 1, 3, 3 },{ 2, 3, 3 },{ 3, 3, 3 },
				{ 4, 3, 3 },{ 3, 4, 3 },{ 0, 3, 4 },{ 1, 3, 4 },
			};
		}

		void decode_ise(quantization_method quantization_level, int elements, const uint8_t* input_data, uint8_t* output_data, int bit_offset)
		{
			int i;
			// note: due to how the trit/quint-block unpacking is done in this function,
			// we may write more temporary results than the number of outputs
			// The maximum actual number of results is 64 bit, but we keep 4 additional elements
			// of padding.
			uint8_t results[68];
			uint8_t tq_blocks[22] = { 0 };		// trit-blocks or quint-blocks
			// TODO, make sure tq_blocks equal to 0..

			int bits, trits, quints;
			find_number_of_bits_trits_quints(quantization_level, &bits, &trits, &quints);

			int lcounter = 0;
			int hcounter = 0;

			// collect bits for each element, as well as bits for any trit-blocks and quint-blocks.
			for (i = 0; i < elements; i++)
			{
				results[i] = static_cast<uint8_t>(read_bits(bits, bit_offset, input_data));
				bit_offset += bits;
				if (trits)
				{
					static const int bits_to_read[5] = { 2, 2, 1, 2, 1 };
					static const int block_shift[5] = { 0, 2, 4, 5, 7 };
					static const int next_lcounter[5] = { 1, 2, 3, 4, 0 };
					static const int hcounter_incr[5] = { 0, 0, 0, 0, 1 };
					int tdata = read_bits(bits_to_read[lcounter], bit_offset, input_data);
					bit_offset += bits_to_read[lcounter];
					tq_blocks[hcounter] |= tdata << block_shift[lcounter];
					hcounter += hcounter_incr[lcounter];
					lcounter = next_lcounter[lcounter];
				}
				if (quints)
				{
					static const int bits_to_read[3] = { 3, 2, 2 };
					static const int block_shift[3] = { 0, 3, 5 };
					static const int next_lcounter[3] = { 1, 2, 0 };
					static const int hcounter_incr[3] = { 0, 0, 1 };
					int tdata = read_bits(bits_to_read[lcounter], bit_offset, input_data);
					bit_offset += bits_to_read[lcounter];
					tq_blocks[hcounter] |= tdata << block_shift[lcounter];
					hcounter += hcounter_incr[lcounter];
					lcounter = next_lcounter[lcounter];
				}
			}


			// unpack trit-blocks or quint-blocks as needed
			if (trits)
			{
				int trit_blocks = (elements + 4) / 5;
				for (i = 0; i < trit_blocks; i++)
				{
					const uint8_t* tritptr = trits_of_integer[tq_blocks[i]];
					results[5 * i] |= tritptr[0] << bits;
					results[5 * i + 1] |= tritptr[1] << bits;
					results[5 * i + 2] |= tritptr[2] << bits;
					results[5 * i + 3] |= tritptr[3] << bits;
					results[5 * i + 4] |= tritptr[4] << bits;
				}
			}

			if (quints)
			{
				int quint_blocks = (elements + 2) / 3;
				for (i = 0; i < quint_blocks; i++)
				{
					const uint8_t* quintptr = quints_of_integer[tq_blocks[i]];
					results[3 * i] |= quintptr[0] << bits;
					results[3 * i + 1] |= quintptr[1] << bits;
					results[3 * i + 2] |= quintptr[2] << bits;
				}
			}

			std::memcpy(output_data, results, elements);
		}
	}
}