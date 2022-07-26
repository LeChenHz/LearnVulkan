#ifndef IMAGEUTIL_ASTC_ASTC_BIT_MANIPULATION_HPP_
#define IMAGEUTIL_ASTC_ASTC_BIT_MANIPULATION_HPP_

#include <stdint.h>

namespace angle
{
	namespace astc
	{

		inline int bitrev8(int p)
		{
			p = ((p & 0xF) << 4) | ((p >> 4) & 0xF);
			p = ((p & 0x33) << 2) | ((p >> 2) & 0x33);
			p = ((p & 0x55) << 1) | ((p >> 1) & 0x55);
			return p;
		}

		inline int read_bits(int bitcount, int bitoffset, const uint8_t* ptr)
		{
			int mask = (1 << bitcount) - 1;
			ptr += bitoffset >> 3;
			bitoffset &= 7;
			int value = ptr[0] | (ptr[1] << 8);
			value >>= bitoffset;
			value &= mask;
			return value;
		}

		extern const uint8_t clz_table[256];

		inline uint32_t clz16(uint16_t inp) {
			uint32_t summa = 8;
			if (inp >= UINT32_C(0x100))
			{
				inp >>= 8;
				summa -= 8;
			}
			return summa + clz_table[inp];
		}

		/*
		32-bit count-leading-zeros function: use the Assembly instruction whenever possible. */
		inline uint32_t clz32(uint32_t inp)
		{
#if defined(__GNUC__) && (defined(__i386) || defined(__amd64))
			uint32_t bsr;
			__asm__("bsrl %1, %0": "=r"(bsr) : "r"(inp | 1));
			return 31 - bsr;
#else
#if defined(__arm__) && defined(__ARMCC_VERSION)
			return __clz(inp);			/* armcc builtin */
#else
#if defined(__arm__) && defined(__GNUC__)
			uint32_t lz;
			__asm__("clz %0, %1": "=r"(lz) : "r"(inp));
			return lz;
#else
			/* slow default version */
			uint32_t summa = 24;
			if (inp >= UINT32_C(0x10000))
			{
				inp >>= 16;
				summa -= 16;
			}
			if (inp >= UINT32_C(0x100))
			{
				inp >>= 8;
				summa -= 8;
			}
			return summa + clz_table[inp];
#endif
#endif
#endif
		}


		/*	sized soft-float types. These are mapped to the sized integer types of C99, instead of C's
		floating-point types; this is because the library needs to maintain exact, bit-level control on all
		operations on these data types. */
		typedef uint16_t sf16;
		typedef uint32_t sf32;

		typedef union if32_
		{
			uint32_t u;
			int32_t s;
			float f;
		} if32;

		/* convert from FP16 to FP32. */
		inline sf32 sf16_to_sf32(sf16 inp)
		{
			uint32_t inpx = inp;

			/*
			This table contains, for every FP16 sign/exponent value combination,
			the difference between the input FP16 value and the value obtained
			by shifting the correct FP32 result right by 13 bits.
			This table allows us to handle every case except denormals and NaN
			with just 1 table lookup, 2 shifts and 1 add.
			*/

#define WITH_MB(a) INT32_C((a) | (1 << 31))
			static const int32_t tbl[64] =
			{
				WITH_MB(0x00000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000),
				INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000),
				INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000),
				INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), INT32_C(0x1C000), WITH_MB(0x38000),
				WITH_MB(0x38000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000),
				INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000),
				INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000),
				INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), INT32_C(0x54000), WITH_MB(0x70000)
			};

			int32_t res = tbl[inpx >> 10];
			res += inpx;

			/* the normal cases: the MSB of 'res' is not set. */
			if (res >= 0)				/* signed compare */
				return res << 13;

			/* Infinity and Zero: the bottom 10 bits of 'res' are clear. */
			if ((res & UINT32_C(0x3FF)) == 0)
				return res << 13;

			/* NaN: the exponent field of 'inp' is not zero; NaNs must be quietened. */
			if ((inpx & 0x7C00) != 0)
				return (res << 13) | UINT32_C(0x400000);

			/* the remaining cases are Denormals. */
			{
				uint32_t sign = (inpx & UINT32_C(0x8000)) << 16;
				uint32_t mskval = inpx & UINT32_C(0x7FFF);
				uint32_t leadingzeroes = clz32(mskval);
				mskval <<= leadingzeroes;
				return (mskval >> 8) + ((0x85 - leadingzeroes) << 23) + sign;
			}
		}


		// conversion function from 16-bit LDR value to FP16.
		// note: for LDR interpolation, it is impossible to get a denormal result;
		// this simplifies the conversion.
		// FALSE; we can receive a very small UNORM16 through the constant-block.
		inline uint16_t unorm16_to_sf16(uint16_t p)
		{
			if (p == 0xFFFF)
				return 0x3C00;			// value of 1.0 .
			if (p < 4)
				return p << 8;

			int lz = clz16(p);
			p <<= (lz + 1);
			p >>= 6;
			p |= (14 - lz) << 10;
			return p;
		}

		/* convert from soft-float to native-float */
		inline float sf16_to_float(sf16 p)
		{
			if32 i;
			i.u = sf16_to_sf32(p);
			return i.f;
		}


		inline uint32_t hash52(uint32_t inp)
		{
			inp ^= inp >> 15;

			inp *= 0xEEDE0891;			// (2^4+1)*(2^7+1)*(2^17-1)
			inp ^= inp >> 5;
			inp += inp << 16;
			inp ^= inp >> 7;
			inp ^= inp >> 3;
			inp ^= inp << 6;
			inp ^= inp >> 17;
			return inp;
		}

		inline uint16_t lns_to_sf16(uint16_t p)
		{
			uint16_t mc = p & 0x7FF;
			uint16_t ec = p >> 11;
			uint16_t mt;
			if (mc < 512)
				mt = 3 * mc;
			else if (mc < 1536)
				mt = 4 * mc - 512;
			else
				mt = 5 * mc - 2048;

			uint16_t res = (ec << 10) | (mt >> 3);
			if (res >= 0x7BFF)
				res = 0x7BFF;
			return res;

		}



	}
}

#endif