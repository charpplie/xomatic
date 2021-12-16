/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk

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
#include "ColorSet.h"


ColorSet::ColorSet( const ColorBlock4x4& srcColors, bool dxt1, bool mergeMatchingColors )
  : m_count( 0 )
  ,	m_transparent( false )
{
	const ColorBGRA8* const colors = srcColors.colors();

	if(mergeMatchingColors)
	{
		for( unsigned int i = 0; i < 16; ++i )
		{
			// check for transparent pixels when using dxt1
			if( dxt1 && colors[i].a < (uint8)128 )
			{
				m_remap[i] = -1;
				m_transparent = true;
				continue;
			}

			// loop over previous points for a match
			for( unsigned int j = 0; ; ++j )
			{
				// allocate a new point
				if( j == i )
				{
					// normalise colors to [0,1]
					m_nonTransparentColor = colors[i];
					const float r = m_nonTransparentColor.r / 255.0f;
					const float g = m_nonTransparentColor.g / 255.0f;
					const float b = m_nonTransparentColor.b / 255.0f;
					
					// add the point
					m_points[m_count] = Vec3( r, g, b );
					m_weights[m_count] = 1.0f;
					m_remap[i] = m_count;	

					++m_count;
					break;
				}
			
				// check for a match
				const bool match = 
					   (  colors[i].b == colors[j].b )
					&& (  colors[i].g == colors[j].g )
					&& (  colors[i].r == colors[j].r )
					&& ( (!dxt1) || (colors[j].a >= (uint8)128));

				if( match )
				{
					// get the index of the match
					const int index = m_remap[j];
					
					// map to this point and increase the weight
					m_weights[index] += 1.0f;
					m_remap[i] = index;
					break;
				}
			}
		}
    }
    else
    {
		for( unsigned int i = 0; i < 16; ++i )
		{
			// check for transparent pixels when using dxt1
			if( dxt1 && colors[i].a < (uint8)128 )
			{
				m_remap[i] = -1;
				m_transparent = true;
                continue;
			}

			// normalise colors to [0,1]
			m_nonTransparentColor = colors[i];
			const float r = m_nonTransparentColor.r / 255.0f;
			const float g = m_nonTransparentColor.g / 255.0f;
			const float b = m_nonTransparentColor.b / 255.0f;
			
			// add the point
			m_points[m_count] = Vec3( r, g, b );
			m_weights[m_count] = 1.0f;
			m_remap[i] = m_count;

			++m_count;
		}
	}
}


void ColorSet::RemapIndices( uint8 const* source, uint8* const target ) const
{
	for( int i = 0; i < 16; ++i )
	{
		const int j = m_remap[i];
        target[i] = (j < 0) ? 3 : source[j];
	}
}

void ColorSet::RemapIndices( uint8 const source, uint8* const target ) const
{
	for( int i = 0; i < 16; ++i )
	{
		const int j = m_remap[i];
        target[i] = (j < 0) ? 3 : source;
	}
}
