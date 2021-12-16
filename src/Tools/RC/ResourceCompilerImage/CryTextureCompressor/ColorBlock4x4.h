#ifndef COLORBLOCK4X4_H
#define COLORBLOCK4X4_H

#include "ColorTypes.h"

/// Uncompressed 4x4 color block.
struct ColorBlock4x4
{
	ColorBlock4x4()
	{
	}

	void set(const void* imgBGRA8, unsigned int width, unsigned int heigth, unsigned int pitch, unsigned int x, unsigned int y);

	bool isSingleColorIgnoringAlpha() const;

	const ColorBGRA8* colors() const
	{
		return m_color;
	}

	ColorBGRA8* colors()
	{
		return m_color;
	}

	ColorBGRA8 color(unsigned int i) const
	{
		return m_color[i];
	}

	ColorBGRA8& color(unsigned int i)
	{
		return m_color[i];
	}

private:
	static const unsigned int COLOR_COUNT = 4*4;

	ColorBGRA8 m_color[COLOR_COUNT];
};

#endif
