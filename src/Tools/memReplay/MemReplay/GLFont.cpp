#include <stdafx.h>
#include "GLFont.h"

#include <gl\GL.h>
#include <gl\GLU.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define FT_ERROR_CHECK(func) do { int error=func; if (error) { printf("FreeType error %d %s:%d\n", error, __FILE__, __LINE__); return false; } } while (0)

float GLFont::getHeight(void)
{
	return m_height;
}

float GLFont::getWidth(const s8 *text)
{
	float a=0.0f;
	while (text[0])
	{
		charInfo *info=&m_info[text[0]];
		a+=info->advance;
		text++;
	}
	return a;
}

void GLFont::drawText(float x, float y, s8 *text)
{
	float invRes=1.0f/(float)m_resolution;
	float origX=x;
	s8 *origText=text;
	y+=0.2f*(float)m_height; // workaround to matching the alignment of the old font solution
	glBindTexture(GL_TEXTURE_2D, m_tex);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	while (text[0])
	{
		if (text[0]=='\r' || text[0]=='\n')
		{
			y+=m_height;
		}
		text++;
	}
	text=origText;
	while (text[0])
	{
		if (text[0]=='\r' || text[0]=='\n')
		{
			x=origX;
			y-=m_height;
		}
		else
		{
			charInfo *info=&m_info[text[0]];
			glTexCoord2f((info->x-1)*invRes, (info->y+info->h+1)*invRes);
			glVertex2f(x+info->offX-1, y+info->offY-info->h-1);
			glTexCoord2f((info->x+info->w+1)*invRes, (info->y+info->h+1)*invRes);
			glVertex2f(x+info->offX+info->w+1, y+info->offY-info->h-1);
			glTexCoord2f((info->x+info->w+1)*invRes, (info->y-1)*invRes);
			glVertex2f(x+info->offX+info->w+1, y+info->offY+1);
			glTexCoord2f((info->x-1)*invRes, (info->y-1)*invRes);
			glVertex2f(x+info->offX-1, y+info->offY+1);
			x+=info->advance;
		}
		text++;
	}
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

int GLFont::wordWrap(s8 *output, u32 outLength, const s8 *input, u32 wrapLength)
{
	float curW = 0;
	int numLines = 1;

	char* outputEnd = output + outLength;

	// Really naive, more character wrapping than word wrapping.
	for (; *input; ++ input)
	{
		float charWidth = m_info[static_cast<unsigned int>(*input)].advance;

		if (ceilf(charWidth + curW) > wrapLength)
		{
			*output ++ = '\n';
			if (output == outputEnd)
			{
				outputEnd[-1] = '\0';
				return numLines;
			}

			curW = 0;
			++ numLines;
		}

		*output ++ = *input;
		if (output == outputEnd)
		{
			outputEnd[-1] = '\0';
			return numLines;
		}

		curW += charWidth;
	}

	*output = '\0';
	return numLines;
}

static void blit(u8 *texture, int offX, int offY, u32 w, FT_Bitmap *bitmap)
{
	u8 *data=(u8*)bitmap->buffer;
	int x,y;
	ASSERT(bitmap->pixel_mode==FT_PIXEL_MODE_GRAY);
	for (y=0; y<bitmap->rows; y++)
	{
		for (x=0; x<bitmap->width; x++)
		{
			texture[(y+offY)*w+x+offX]=data[y*bitmap->pitch+x];
		}
	}
}

bool GLFont::loadFont(const s8 *name, u32 resolution)
{
	FT_Library ft;
	FT_Face face;
	int border=3;
	u32 i;
	u8 *textTexture=(u8*)malloc(resolution*resolution);
	u32 maxHeight=0;
	FT_ERROR_CHECK(FT_Init_FreeType(&ft));
	FT_ERROR_CHECK(FT_New_Face(ft, name, 0, &face));
	m_resolution=resolution;
	glGenTextures(1, &m_tex);
	glBindTexture(GL_TEXTURE_2D, m_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	memset(textTexture, 0, resolution*resolution);
	FT_ERROR_CHECK(FT_Set_Char_Size(face, 0, 72*64, 0, (resolution/16)-2*border)); // set font size to 1 inch and dpi to required size
	for (i=0; i<256; i++)
	{
		int texX=0, texY=0;
		FT_GlyphSlot slot=face->glyph;
		if (FT_Load_Char(face, i, FT_LOAD_RENDER))
			continue;
		texX=(i&0xF)*(resolution/16)+border;
		texY=(i>>4)*(resolution/16)+border;
		m_info[i].advance=slot->advance.x/64.0f; // 16.16 format
		m_info[i].x=texX;
		m_info[i].y=texY;
		m_info[i].offX=slot->bitmap_left;
		m_info[i].offY=slot->bitmap_top;
		m_info[i].w=slot->bitmap.width;
		m_info[i].h=slot->bitmap.rows;
		blit(textTexture, texX, texY, resolution, &slot->bitmap);
		maxHeight=std::max<int>(maxHeight, slot->bitmap.rows);
	}
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_ALPHA8, resolution, resolution, GL_ALPHA, GL_UNSIGNED_BYTE, textTexture);
	free(textTexture);
	FT_ERROR_CHECK(FT_Done_Face(face));
	m_height=(float)maxHeight;
	FT_ERROR_CHECK(FT_Done_FreeType(ft));
	return true;
}