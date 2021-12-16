#ifndef COLORTYPES_H
#define COLORTYPES_H

#include "platform.h"       // uint32, uint16 etc.


// 16 bit 565 BGR color
struct ColorB5G6R5
{
	ColorB5G6R5()
	{
	}

	ColorB5G6R5(const ColorB5G6R5& a_c)
		: u(a_c.u)
	{
	}

	explicit ColorB5G6R5(uint16 a_u)
		: u(a_u)
	{
	}

	union
	{
		struct
		{
			uint16 b : 5;
			uint16 g : 6;
			uint16 r : 5;
		};
		uint16 u;
	};
};

STATIC_ASSERT( sizeof(ColorB5G6R5) == 2 );


// 32 bit 8888 BGRA color
struct ColorBGRA8
{
	ColorBGRA8()
	{
	}

	ColorBGRA8(const ColorBGRA8& a_c)
		: u(a_c.u)
	{
	}

	ColorBGRA8(uint8 a_b, uint8 a_g, uint8 a_r, uint8 a_a)
		: b(a_b)
		, g(a_g)
		, r(a_r)
		, a(a_a)
	{
	}

	explicit ColorBGRA8(uint32 a_u)
		: u(a_u)
	{
	}

	void setBGRA(uint8 a_b, uint8 a_g, uint8 a_r, uint8 a_a)
	{
		b = a_b;
		g = a_g;
		r = a_r;
		a = a_a;
	}

	void setBGRA(const uint8* a_pBGRA8)
	{
		b = a_pBGRA8[0];
		g = a_pBGRA8[1];
		r = a_pBGRA8[2];
		a = a_pBGRA8[3];
	}

	union
	{
		struct
		{
			uint8 b;
			uint8 g;
			uint8 r;
			uint8 a;
		};
		uint32 u;
	};

};

STATIC_ASSERT( sizeof(ColorBGRA8) == 4 );

#endif
