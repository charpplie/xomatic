#pragma once

class CBitmapWnd : public CStatic
{
public:
	CBitmapWnd();

	void FromBits(int width, int height, const u8* rgb);

protected:
	void OnSize(UINT nType, int cx, int cy);
	BOOL OnEraseBkgnd(CDC* pDC);
	void OnPaint();

private:
	CBitmap m_bmp;

	CSize m_bmpSz;
	bool m_bmpUpdated;
	std::vector<u8> m_bmpRGB;

private:
	void RebuildBitmap(CDC* dc);

private:
	DECLARE_MESSAGE_MAP()
};
