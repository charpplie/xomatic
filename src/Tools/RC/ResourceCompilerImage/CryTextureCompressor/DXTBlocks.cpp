/* -----------------------------------------------------------------------------

	Parts:
	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
	Copyright (c) 2006 Ignacio Castano                      icastano@nvidia.com

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */

#include "stdafx.h"
#include "DXTBlocks.h"


static inline int FloatToNearestInt( float const a, int const maxlimit )
{
	const int i = (int)( a + 0.5f );
	return (i < 0) ? 0 : ((i>maxlimit) ? maxlimit : i);
}


static inline ColorB5G6R5 FloatToColorB5G6R5( const Vec3& color )
{
	ColorB5G6R5 c;

	c.b = FloatToNearestInt( 31.0f*color.x, 31 );
	c.g = FloatToNearestInt( 63.0f*color.y, 63 );
	c.r = FloatToNearestInt( 31.0f*color.z, 31 );

	return c;
}


void ColorBlockDXT1::Set( ColorB5G6R5 const a_color0, ColorB5G6R5 const a_color1, const uint8* const a_indices )
{
	col0 = a_color0;
	col1 = a_color1;

	row[0] = a_indices[ 0] | ( a_indices[ 1] << 2 ) | ( a_indices[ 2] << 4 ) | ( a_indices[ 3] << 6 );
	row[1] = a_indices[ 4] | ( a_indices[ 5] << 2 ) | ( a_indices[ 6] << 4 ) | ( a_indices[ 7] << 6 );
	row[2] = a_indices[ 8] | ( a_indices[ 9] << 2 ) | ( a_indices[10] << 4 ) | ( a_indices[11] << 6 );
	row[3] = a_indices[12] | ( a_indices[13] << 2 ) | ( a_indices[14] << 4 ) | ( a_indices[15] << 6 );
}


void ColorBlockDXT1::Set3( const Vec3& start, const Vec3& end, const uint8* const indices )
{
	ColorB5G6R5 color0 = FloatToColorB5G6R5( start );
	ColorB5G6R5 color1 = FloatToColorB5G6R5( end );

	if( color0.u <= color1.u )
	{
		Set(color0, color1, indices);
	}
	else
	{
		// remap the indices
		static const uint8 remapping[4] = { 1, 0, 2, 3 };
		uint8 remapped[16];
		for( unsigned int i = 0; i < 16; ++i )
		{
			remapped[i] = remapping[ indices[i] ];
		}

		Set(color1, color0, remapped);
	}
}


void ColorBlockDXT1::Set4( const Vec3& start, const Vec3& end, const uint8* const indices )
{
	ColorB5G6R5 color0 = FloatToColorB5G6R5( start );
	ColorB5G6R5 color1 = FloatToColorB5G6R5( end );

	if( color0.u > color1.u )
	{
		Set(color0, color1, indices);
	}
	else if( color0.u == color1.u )
	{
		static const uint8 zero[16] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };

		Set(color0, color0, zero);
	}
	else
	{
		// remap the indices
		uint8 remapped[16];
		for( unsigned int i = 0; i < 16; ++i )
		{
			remapped[i] = indices[i] ^ 0x01;
		}

		Set(color1, color0, remapped);
	}
}

