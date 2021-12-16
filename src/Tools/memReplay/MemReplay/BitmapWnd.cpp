#include "stdafx.h"
#include "BitmapWnd.h"

BEGIN_MESSAGE_MAP(CBitmapWnd, CStatic)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CBitmapWnd::CBitmapWnd()
	: m_bmpSz(0, 0)
	, m_bmpUpdated(false)
{
}

void CBitmapWnd::FromBits(int width, int height, const u8* rgb)
{
	if (width > 0 && height > 0)
	{
		m_bmpRGB.resize(width * height * 3);
		m_bmpSz = CSize(width, height);

		for (int y = 0; y < height; ++ y)
			memcpy(&m_bmpRGB[y * width * 3], &rgb[(height - y - 1) * width * 3], width * 3);
	}
	else
	{
		m_bmpSz = CSize(0, 0);
		m_bmpRGB.clear();
	}

	m_bmpUpdated = true;

	if (m_hWnd)
		Invalidate();
}

void CBitmapWnd::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	Invalidate();
}

void CBitmapWnd::OnPaint()
{
	CPaintDC dc(this);

	CBrush fillBrush(RGB(0, 0, 0));
	dc.FillRect(&dc.m_ps.rcPaint, &fillBrush);

	if (!m_bmp.m_hObject)
	{
		RebuildBitmap(&dc);

		if (!m_bmp.m_hObject)
			return;
	}

	BITMAP bm;
	m_bmp.GetBitmap(&bm);

	if (CSize(bm.bmWidth, bm.bmHeight) != m_bmpSz)
	{
		RebuildBitmap(&dc);

		if (!m_bmp.m_hObject)
			return;
	}

	if (m_bmpUpdated)
		RebuildBitmap(&dc);

	CRect rcClient;
	GetClientRect(rcClient);

	CDC srcDC;
	srcDC.CreateCompatibleDC(&dc);
	CBitmap* oldSrcDCObject = srcDC.SelectObject(&m_bmp);

	dc.StretchBlt(
		rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
		&srcDC,
		0, 0, bm.bmWidth, bm.bmHeight,
		SRCCOPY);

	srcDC.SelectObject(oldSrcDCObject);
}

BOOL CBitmapWnd::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CBitmapWnd::RebuildBitmap(CDC* dc)
{
	if (m_bmp.m_hObject)
	{
		BITMAP bm;
		m_bmp.GetBitmap(&bm);

		if (m_bmpSz != CSize(bm.bmWidth, bm.bmHeight))
			m_bmp.DeleteObject();
	}

	if (m_bmpSz.cx)
	{
		BITMAPINFO info = {0};
		info.bmiHeader.biSize = sizeof(info.bmiHeader);
		info.bmiHeader.biWidth = m_bmpSz.cx;
		info.bmiHeader.biHeight = m_bmpSz.cy;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 24;
		info.bmiHeader.biCompression = 0;
		info.bmiHeader.biSizeImage = 0;
		info.bmiHeader.biXPelsPerMeter = 72;
		info.bmiHeader.biYPelsPerMeter = 72;

		if (!m_bmp.m_hObject)
			m_bmp.CreateCompatibleBitmap(dc, m_bmpSz.cx, m_bmpSz.cy);

		SetDIBits(*dc, m_bmp, 0, m_bmpSz.cy, &m_bmpRGB[0], &info, 0);
	}

	m_bmpUpdated = false;
}
