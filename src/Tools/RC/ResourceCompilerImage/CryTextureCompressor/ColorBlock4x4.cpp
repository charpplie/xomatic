#include "stdafx.h"
#include "ColorBlock4x4.h"



STATIC_ASSERT( sizeof(int) == 4 );
STATIC_ASSERT( sizeof(int) == 4 );

void ColorBlock4x4::set(const void* imgBGRA8, unsigned int const width, unsigned int const height, unsigned int const pitch, unsigned int x, unsigned int y)
{
	assert(imgBGRA8);
	assert((width&3) == 0);
	assert((height&3) == 0);
	assert(pitch >= width*sizeof(ColorBGRA8));
	assert(x<width);
	assert(y<height);

	const unsigned int bw = min(width - x, 4U);
	const unsigned int bh = min(height - y, 4U);

	// note: it's allowed for source data to be not aligned to 4 byte boundary
	// (so, we cannot cast source data poointer to ColorBGRA8* in code below)

	if((bw==4) && (bh==4))
	{
		for(unsigned int row = 0; row < 4; ++row)
		{
			const uint8* const pSrc = ((const uint8*)imgBGRA8) + (pitch * (y + row)) + (x * sizeof(ColorBGRA8));

			ColorBGRA8* const pDst = &m_color[row << 2];

			pDst[0].setBGRA( &pSrc[ 0 * sizeof(ColorBGRA8) ] );
			pDst[1].setBGRA( &pSrc[ 1 * sizeof(ColorBGRA8) ] );
			pDst[2].setBGRA( &pSrc[ 2 * sizeof(ColorBGRA8) ] );
			pDst[3].setBGRA( &pSrc[ 3 * sizeof(ColorBGRA8) ] );

			/*
			// Test cases for principal component computation robustness:
			// Symmetrical color distribution sets (some Principal Component 
			// methods fail to handle such sets correctly).

			// principal component is perpendicular to (1,1,1) vector
			pDst[0].setBGRA( 0,0,255,255 );
			pDst[1].setBGRA( 255,0,0,255 );
			pDst[2].setBGRA( 255,0,255,255 );
			pDst[3].setBGRA( 0,0,0,255 );

			// principal component is arbitrary vector in plane XY (perpendicular to (0,1,0) vector)
			pDst[0].setBGRA( 0,0,255,255 );
			pDst[1].setBGRA( 0,0,255,255 );
			pDst[2].setBGRA( 255,0,0,255 );
			pDst[3].setBGRA( 255,0,0,255 );
			*/
		}
	}
	else
	{
		// Rare case: block is smaller than 4x4.
		// Let's repeat pixels in this case.
		// It will keep frequency of colors, except the case 
		// when width and/or height equals 3. But, this case 
		// is very rare because images usually are "power of 2" sized, and even
		// if they are not, nobody will notice that the resulting encoding 
		// for such block is not ideal.

		static unsigned int remainder[] =
		{
			0, 0, 0, 0,
			0, 1, 0, 1,
			0, 1, 2, 0,
			0, 1, 2, 3,
		};

		for(unsigned int row = 0; row < 4; ++row)
		{
			const unsigned int by = remainder[(bh - 1) * 4 + row];
			const uint8* const pSrc = ((const uint8*)imgBGRA8) + pitch * (y + by);

			ColorBGRA8* pDst = &m_color[row * 4];

			for(unsigned int col = 0; col < 4; ++col)
			{
				const unsigned int bx = remainder[(bw - 1) * 4 + col];
				pDst[col].setBGRA( &pSrc[ (x+bx) * sizeof(ColorBGRA8) ] );
			}
		}
	}
}


bool ColorBlock4x4::isSingleColorIgnoringAlpha() const
{
	for(unsigned int i = 1; i < COLOR_COUNT; ++i)
	{
		if(    (m_color[0].b != m_color[i].b)
		    || (m_color[0].g != m_color[i].g)
		    || (m_color[0].r != m_color[i].r) )
		{
			return false;
		}
	}
	return true;
}
