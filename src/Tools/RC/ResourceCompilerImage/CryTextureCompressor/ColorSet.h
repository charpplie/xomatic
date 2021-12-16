/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to deal in the Software without restriction, including
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

#ifndef COLORSET_H
#define COLORSET_H

#include "ColorBlock4x4.h"
#include "Cry_Math.h"       // uint8, Vec3 etc.

class ColorSet
{
public:
	ColorSet( const ColorBlock4x4& srcColors, bool dxt1, bool mergeMatchingColors );

	unsigned int GetCount() const
	{
		return m_count;
	}

	unsigned int HasTransparent() const
	{
		return m_transparent;
	}

	ColorBGRA8 GetSomeNonTransparentColor() const
	{
		return m_nonTransparentColor;
	}

	Vec3 const* GetPoints() const
	{
		return m_points;
	}

	float const* GetWeights() const
	{
		return m_weights;
	}

	void RemapIndices( uint8 const* source, uint8* target ) const;

	void RemapIndices( uint8 source, uint8* target ) const;

private:
	unsigned int m_count;
	Vec3 m_points[16];
	float m_weights[16];
	int m_remap[16];
	bool m_transparent;
	ColorBGRA8 m_nonTransparentColor;
};

#endif
