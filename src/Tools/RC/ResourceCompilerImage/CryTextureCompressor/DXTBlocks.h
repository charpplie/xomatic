#ifndef DXTBLOCKS_H
#define DXTBLOCKS_H

// DXT and 3DC related data structures

#include "ColorTypes.h"
#include "Cry_Math.h"          // Vec3


struct ColorBlockDXT1
{
	ColorBlockDXT1()
	{
	}

	void Set( ColorB5G6R5 color0, ColorB5G6R5 color1, const uint8* indices );

	void Set3( const Vec3& start, const Vec3& end, uint8 const* indices );

	void Set4( const Vec3& start, const Vec3& end, uint8 const* indices );

	void evaluatePalette3or4(ColorBGRA8 pal[4]) const
	{
		pal[0].b = (col0.b << 3) | (col0.b >> 2);
		pal[0].g = (col0.g << 2) | (col0.g >> 4);
		pal[0].r = (col0.r << 3) | (col0.r >> 2);
		pal[0].a = 255;

		pal[1].b = (col1.b << 3) | (col1.b >> 2);
		pal[1].g = (col1.g << 2) | (col1.g >> 4);
		pal[1].r = (col1.r << 3) | (col1.r >> 2);
		pal[1].a = 255;

		if(col0.u > col1.u)
		{
			pal[2].b = (2 * pal[0].b + pal[1].b + 1) / 3;
			pal[2].g = (2 * pal[0].g + pal[1].g + 1) / 3;
			pal[2].r = (2 * pal[0].r + pal[1].r + 1) / 3;
			pal[2].a = 255;

			pal[3].b = (2 * pal[1].b + pal[0].b + 1) / 3;
			pal[3].g = (2 * pal[1].g + pal[0].g + 1) / 3;
			pal[3].r = (2 * pal[1].r + pal[0].r + 1) / 3;
			pal[3].a = 255;
		}
		else
		{
			pal[2].b = (pal[0].b + pal[1].b + 1) >> 1;
			pal[2].g = (pal[0].g + pal[1].g + 1) >> 1;
			pal[2].r = (pal[0].r + pal[1].r + 1) >> 1;
			pal[2].a = 255;

			pal[3].b = 0;
			pal[3].g = 0;
			pal[3].r = 0;
			pal[3].a = 0;
		}
	}

	// note: keeps existing alpha color in the palette
	void evaluatePalette4KeepAlpha(ColorBGRA8 pal[4]) const
	{
		pal[0].b = (col0.b << 3) | (col0.b >> 2);
		pal[0].g = (col0.g << 2) | (col0.g >> 4);
		pal[0].r = (col0.r << 3) | (col0.r >> 2);

		pal[1].b = (col1.b << 3) | (col1.b >> 2);
		pal[1].g = (col1.g << 2) | (col1.g >> 4);
		pal[1].r = (col1.r << 3) | (col1.r >> 2);

		pal[2].b = (2 * pal[0].b + pal[1].b + 1) / 3;
		pal[2].g = (2 * pal[0].g + pal[1].g + 1) / 3;
		pal[2].r = (2 * pal[0].r + pal[1].r + 1) / 3;

		pal[3].b = (2 * pal[1].b + pal[0].b + 1) / 3;
		pal[3].g = (2 * pal[1].g + pal[0].g + 1) / 3;
		pal[3].r = (2 * pal[1].r + pal[0].r + 1) / 3;
	}

	void decodeBGRA8(const ColorBGRA8 a_pal[4], void* const a_dst, int const a_rowStrideInBytes) const
	{
		ColorBGRA8* dst = (ColorBGRA8*)a_dst;
		dst[0] = a_pal[bits0];
		dst[1] = a_pal[bits1];
		dst[2] = a_pal[bits2];
		dst[3] = a_pal[bits3];

		dst = (ColorBGRA8*) (((uint8*)dst) + a_rowStrideInBytes);
		dst[0] = a_pal[bits4];
		dst[1] = a_pal[bits5];
		dst[2] = a_pal[bits6];
		dst[3] = a_pal[bits7];

		dst = (ColorBGRA8*) (((uint8*)dst) + a_rowStrideInBytes);
		dst[0] = a_pal[bits8];
		dst[1] = a_pal[bits9];
		dst[2] = a_pal[bitsA];
		dst[3] = a_pal[bitsB];

		dst = (ColorBGRA8*) (((uint8*)dst) + a_rowStrideInBytes);
		dst[0] = a_pal[bitsC];
		dst[1] = a_pal[bitsD];
		dst[2] = a_pal[bitsE];
		dst[3] = a_pal[bitsF];
	}

	template <int dstStepInBytes>
	void decodeB8G8R8(const ColorBGRA8 a_pal[4], void* const a_dst, int const a_rowStrideInBytes) const
	{
		const uint8* src;

		// First row
		uint8* dst = (uint8*)a_dst;

		src = (const uint8*) &a_pal[bits0];
		dst[                 0] = src[0];
		dst[                 1] = src[1];
		dst[                 2] = src[2];

		src = (const uint8*) &a_pal[bits1];
		dst[  dstStepInBytes+0] = src[0];
		dst[  dstStepInBytes+1] = src[1];
		dst[  dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bits2];
		dst[2*dstStepInBytes+0] = src[0];
		dst[2*dstStepInBytes+1] = src[1];
		dst[2*dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bits3];
		dst[3*dstStepInBytes+0] = src[0];
		dst[3*dstStepInBytes+1] = src[1];
		dst[3*dstStepInBytes+2] = src[2];

		// Second row
		dst += a_rowStrideInBytes;

		src = (const uint8*) &a_pal[bits4];
		dst[                 0] = src[0];
		dst[                 1] = src[1];
		dst[                 2] = src[2];

		src = (const uint8*) &a_pal[bits5];
		dst[  dstStepInBytes+0] = src[0];
		dst[  dstStepInBytes+1] = src[1];
		dst[  dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bits6];
		dst[2*dstStepInBytes+0] = src[0];
		dst[2*dstStepInBytes+1] = src[1];
		dst[2*dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bits7];
		dst[3*dstStepInBytes+0] = src[0];
		dst[3*dstStepInBytes+1] = src[1];
		dst[3*dstStepInBytes+2] = src[2];

		// Third row
		dst += a_rowStrideInBytes;

		src = (const uint8*) &a_pal[bits8];
		dst[                 0] = src[0];
		dst[                 1] = src[1];
		dst[                 2] = src[2];

		src = (const uint8*) &a_pal[bits9];
		dst[  dstStepInBytes+0] = src[0];
		dst[  dstStepInBytes+1] = src[1];
		dst[  dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bitsA];
		dst[2*dstStepInBytes+0] = src[0];
		dst[2*dstStepInBytes+1] = src[1];
		dst[2*dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bitsB];
		dst[3*dstStepInBytes+0] = src[0];
		dst[3*dstStepInBytes+1] = src[1];
		dst[3*dstStepInBytes+2] = src[2];

		// Fourth row
		dst += a_rowStrideInBytes;

		src = (const uint8*) &a_pal[bitsC];
		dst[                 0] = src[0];
		dst[                 1] = src[1];
		dst[                 2] = src[2];

		src = (const uint8*) &a_pal[bitsD];
		dst[  dstStepInBytes+0] = src[0];
		dst[  dstStepInBytes+1] = src[1];
		dst[  dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bitsE];
		dst[2*dstStepInBytes+0] = src[0];
		dst[2*dstStepInBytes+1] = src[1];
		dst[2*dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bitsF];
		dst[3*dstStepInBytes+0] = src[0];
		dst[3*dstStepInBytes+1] = src[1];
		dst[3*dstStepInBytes+2] = src[2];
	}

public:
	ColorB5G6R5 col0;
	ColorB5G6R5 col1;

	union
	{
		struct
		{
			uint32 bits0 : 2;
			uint32 bits1 : 2;
			uint32 bits2 : 2;
			uint32 bits3 : 2;

			uint32 bits4 : 2;
			uint32 bits5 : 2;
			uint32 bits6 : 2;
			uint32 bits7 : 2;

			uint32 bits8 : 2;
			uint32 bits9 : 2;
			uint32 bitsA : 2;
			uint32 bitsB : 2;

			uint32 bitsC : 2;
			uint32 bitsD : 2;
			uint32 bitsE : 2;
			uint32 bitsF : 2;
		};
		uint8 row[4];
		uint32 indices;
	};
};

struct AlphaBlockDXT3
{
	AlphaBlockDXT3()
	{
	}

	template <int dstStepInBytes>
	void decode(void* const a_dst, int const a_rowStrideInBytes) const
	{
		uint8* dst = (uint8*) a_dst;
		dst[               0] = (alpha0 << 4) | alpha0;
		dst[  dstStepInBytes] = (alpha1 << 4) | alpha1;;
		dst[2*dstStepInBytes] = (alpha2 << 4) | alpha2;;
		dst[3*dstStepInBytes] = (alpha3 << 4) | alpha3;;

		dst += a_rowStrideInBytes;
		dst[               0] = (alpha4 << 4) | alpha4;
		dst[  dstStepInBytes] = (alpha5 << 4) | alpha5;
		dst[2*dstStepInBytes] = (alpha6 << 4) | alpha6;
		dst[3*dstStepInBytes] = (alpha7 << 4) | alpha7;

		dst += a_rowStrideInBytes;
		dst[               0] = (alpha8 << 4) | alpha8;
		dst[  dstStepInBytes] = (alpha9 << 4) | alpha9;
		dst[2*dstStepInBytes] = (alphaA << 4) | alphaA;
		dst[3*dstStepInBytes] = (alphaB << 4) | alphaB;

		dst += a_rowStrideInBytes;
		dst[               0] = (alphaC << 4) | alphaC;
		dst[  dstStepInBytes] = (alphaD << 4) | alphaD;
		dst[2*dstStepInBytes] = (alphaE << 4) | alphaE;
		dst[3*dstStepInBytes] = (alphaF << 4) | alphaF;
	}

public:
	union
	{
		struct
		{
			uint32 alpha0 : 4;
			uint32 alpha1 : 4;
			uint32 alpha2 : 4;
			uint32 alpha3 : 4;
			uint32 alpha4 : 4;
			uint32 alpha5 : 4;
			uint32 alpha6 : 4;
			uint32 alpha7 : 4;
			uint32 alpha8 : 4;
			uint32 alpha9 : 4;
			uint32 alphaA : 4;
			uint32 alphaB : 4;
			uint32 alphaC : 4;
			uint32 alphaD : 4;
			uint32 alphaE : 4;
			uint32 alphaF : 4;
		};
		uint16 row[4];
		uint64 u;
	};
};

struct AlphaBlockDXT5
{
	AlphaBlockDXT5()
	{
	}

	void setByIndex(unsigned int index, unsigned int value)
	{
		assert(index < 16);
		assert(value < 8);

		const unsigned int offset = (3 * index + 16);
		const uint64 mask = uint64(0x7) << offset;
		u = (u & ~mask) | (uint64(value) << offset);
	}

	void evaluatePalette(uint8 alpha[8]) const
	{
		if(alpha0 > alpha1)
		{
			// 8-alpha block
			alpha[0] = alpha0;
			alpha[1] = alpha1;
			alpha[2] = (6 * alpha[0] +     alpha[1] + 3) / 7;
			alpha[3] = (5 * alpha[0] + 2 * alpha[1] + 3) / 7;
			alpha[4] = (4 * alpha[0] + 3 * alpha[1] + 3) / 7;
			alpha[5] = (3 * alpha[0] + 4 * alpha[1] + 3) / 7;
			alpha[6] = (2 * alpha[0] + 5 * alpha[1] + 3) / 7;
			alpha[7] = (    alpha[0] + 6 * alpha[1] + 3) / 7;
		}
		else
		{
			// 6-alpha block.
			alpha[0] = alpha0;
			alpha[1] = alpha1;
			alpha[2] = (4 * alpha[0] +     alpha[1] + 2) / 5;
			alpha[3] = (3 * alpha[0] + 2 * alpha[1] + 2) / 5;
			alpha[4] = (2 * alpha[0] + 3 * alpha[1] + 2) / 5;
			alpha[5] = (    alpha[0] + 4 * alpha[1] + 2) / 5;
			alpha[6] = 0x00;
			alpha[7] = 0xFF;
		}
	}

	template <int dstStepInBytes>
	void decode(void* const a_dst, int const a_rowStrideInBytes) const
	{
		uint8 pal[8];
		evaluatePalette(pal);

		uint8* dst = (uint8*) a_dst;
		dst[               0] = pal[bits0];
		dst[  dstStepInBytes] = pal[bits1];
		dst[2*dstStepInBytes] = pal[bits2];
		dst[3*dstStepInBytes] = pal[bits3];

		dst += a_rowStrideInBytes;
		dst[               0] = pal[bits4];
		dst[  dstStepInBytes] = pal[bits5];
		dst[2*dstStepInBytes] = pal[bits6];
		dst[3*dstStepInBytes] = pal[bits7];

		dst += a_rowStrideInBytes;
		dst[               0] = pal[bits8];
		dst[  dstStepInBytes] = pal[bits9];
		dst[2*dstStepInBytes] = pal[bitsA];
		dst[3*dstStepInBytes] = pal[bitsB];

		dst += a_rowStrideInBytes;
		dst[               0] = pal[bitsC];
		dst[  dstStepInBytes] = pal[bitsD];
		dst[2*dstStepInBytes] = pal[bitsE];
		dst[3*dstStepInBytes] = pal[bitsF];
	}

public:
	union
	{
		struct
		{
			uint8 alpha0;
			uint8 alpha1;
			uint8 bitcodes[6];
		};
		struct
		{
			uint64 _a0   : 8; // 8
			uint64 _a1   : 8; // 16
			uint64 bits0 : 3; // 3 - 19
			uint64 bits1 : 3; // 6 - 22
			uint64 bits2 : 3; // 9 - 25
			uint64 bits3 : 3; // 12 - 28
			uint64 bits4 : 3; // 15 - 31
			uint64 bits5 : 3; // 18 - 34
			uint64 bits6 : 3; // 21 - 37
			uint64 bits7 : 3; // 24 - 40
			uint64 bits8 : 3; // 27 - 43
			uint64 bits9 : 3; // 30 - 46
			uint64 bitsA : 3; // 33 - 49
			uint64 bitsB : 3; // 36 - 52
			uint64 bitsC : 3; // 39 - 55
			uint64 bitsD : 3; // 42 - 58
			uint64 bitsE : 3; // 45 - 61
			uint64 bitsF : 3; // 48 - 64
		};
		uint64 u;
	};
};


struct BlockDXT1
{
	ColorBlockDXT1 color;
};


struct BlockDXT3
{
	AlphaBlockDXT3 alpha;
	ColorBlockDXT1 color;
};


struct BlockDXT5
{
	AlphaBlockDXT5 alpha;
	ColorBlockDXT1 color;
};


STATIC_ASSERT( sizeof(ColorBlockDXT1) == 8 );
STATIC_ASSERT( sizeof(AlphaBlockDXT3) == 8 );
STATIC_ASSERT( sizeof(AlphaBlockDXT5) == 8 );
STATIC_ASSERT( sizeof(BlockDXT1) == 8 );
STATIC_ASSERT( sizeof(BlockDXT3) == 16 );
STATIC_ASSERT( sizeof(BlockDXT5) == 16 );

#endif
