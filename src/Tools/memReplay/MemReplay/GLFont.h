#ifndef __GLFONT_H__
#define __GLFONT_H__

class GLFont
{
public:
	float getHeight(void);
	float getWidth(const s8 *text);
	void drawText(float x, float y, s8 *text);
	int wordWrap(s8 *output, u32 outputSize, const s8 *input, u32 wrapLength);
	bool loadFont(const s8 *name, u32 resolution);

private:
	typedef struct charInfo_s
	{
		u32 x, y;
		u32 w, h;
		int offX, offY;
		float advance;
	} charInfo;

	charInfo m_info[256];
	u32 m_tex;
	u32 m_resolution;
	float m_height;
};

#endif