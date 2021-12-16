#include "stdafx.h"
#include "DistributionMapWnd.h"

const double CDistributionMapWnd::MaxRange = 4095.0 * 1024.0 * 1024.0;
const double CDistributionMapWnd::MaxAxisRange = 64.0 * 1024.0;

inline float Lerp(float a, float b, float t)
{
	return (1.0f - t) * a + t * b;
}

inline void LerpColor(float* a, const float* b, float t)
{
	a[0] = Lerp(a[0], b[0], t);
	a[1] = Lerp(a[1], b[1], t);
	a[2] = Lerp(a[2], b[2], t);
}

BEGIN_MESSAGE_MAP(CDistributionMapWnd, CStatic)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

CDistributionMapWnd::CDistributionMapWnd()
	: m_memMap(NULL)
	, m_panX(0)
	, m_panY(0)
	, m_stops(6)
	, m_invScale(64)
	, m_frame(0)
{
}

void CDistributionMapWnd::SetMemoryMap(const MemoryMapReplay* memMap)
{
	m_memMap = memMap;
	if (m_hWnd)
		Invalidate();
}

void CDistributionMapWnd::SetFrameMarker(int frame)
{
	m_frame = frame;
}

void CDistributionMapWnd::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	Invalidate();

	UpdateScrollBar();
}

void CDistributionMapWnd::OnPaint()
{
	CPaintDC dc(this);

	if (!m_memMap)
	{
		CBrush blackBrush(RGB(0, 0, 0));
		dc.FillRect(&dc.m_ps.rcPaint, &blackBrush);
		return;
	}

	CRect rcClient;
	GetClientRect(rcClient);

	std::vector<CellInfo> projection(rcClient.Width() * rcClient.Height());
	memset(&projection[0], 0, sizeof(CellInfo) * projection.size());

	double scale = 1.0 / m_invScale;
	u64 invScale = m_invScale;
	u64 rangeWide = 65536;

	u64 projectedRangeWide = rcClient.Width() * invScale;

	// Assume that we're undersampling. Which we almost certainly will be.

	// Grab each row of allocs and project them into the map.

	for (int projRow = 0, projWidth = rcClient.Width(), projHeight = rcClient.Height(); projRow < projHeight; ++ projRow)
	{
		u64 addressMin = m_panX * invScale + ((m_panY + projRow) * invScale * rangeWide);
		u64 addressMax = (m_panX + rcClient.Width()) * invScale + ((m_panY + projRow + rcClient.Height()) * invScale * rangeWide);

		CellInfo* projElements = &projection[projRow * projWidth];

		for (int subSampleRow = 0; subSampleRow < invScale; ++ subSampleRow)
		{
			u64 rowAddrStart = addressMin + subSampleRow * rangeWide;
			u64 rowAddrEnd = addressMin + subSampleRow * rangeWide + projectedRangeWide;

			if (rowAddrEnd >= 0x100000000)
				break;

			MemoryMapReplay::ConstIterator itBegin = m_memMap->FindFirstFromAddress(rowAddrStart);
			MemoryMapReplay::ConstIterator itEnd = m_memMap->FindLastFromAddress(rowAddrEnd);

			for (; itBegin != itEnd; ++ itBegin)
			{
				// Project the alloc into the projection map.
				TAddress addr = itBegin->first;
				const MemoryMapReplay::Alloc& alloc = itBegin->second;

				int left = (int) ((addr - rowAddrStart) / invScale);
				int right = (addr - rowAddrStart + alloc.size + (invScale - 1)) / invScale;

				left = Clamp(0, projWidth, left);
				right = Clamp(0, projWidth, right);

				if (left == right)
				{
					projElements[left].bytesUsed += alloc.size;
					projElements[left].count ++;
					projElements[left].totalAge += alloc.frame;
				}
				else
				{
					int leftSize = invScale - (addr - (left * invScale + rowAddrStart));

					projElements[left].bytesUsed += leftSize;
					projElements[left].count ++;
					projElements[left].totalAge += alloc.frame;

					++ left;

					for (; left < right; ++ left)
					{
						projElements[left].bytesUsed += invScale;
						projElements[left].count ++;
						projElements[left].totalAge += alloc.frame;
					}

					int rightSize = (addr + alloc.size) - left * invScale + rowAddrStart;
					projElements[left].bytesUsed += rightSize;
					projElements[left].count ++;
					projElements[left].totalAge += alloc.frame;
				}
			}
		}
	}

	CBitmap bm;
	bm.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());

	BITMAPINFO bi = {0};
	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = rcClient.Width();
	bi.bmiHeader.biHeight = -rcClient.Height();
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = 0;
	bi.bmiHeader.biXPelsPerMeter = 96;
	bi.bmiHeader.biYPelsPerMeter = 96;
	bi.bmiHeader.biClrUsed = 0;

	std::vector<DWORD> bits(rcClient.Width() * rcClient.Height());

	DWORD* dw = &bits[0];

	int maxPerPixelUsage = invScale * invScale;

	for (std::vector<CellInfo>::iterator itCell = projection.begin(), itCellEnd = projection.end();
		itCell != itCellEnd;
		++ itCell, ++ dw)
	{
		if (itCell->count == 0)
			continue;

		u8 usage = (u8) Clamp(0U, 255U, itCell->bytesUsed * 255 / maxPerPixelUsage);
		u8 aveTime = std::min(255, std::max(0, m_frame - (int)(itCell->totalAge / itCell->count)));

		*dw = (aveTime<<16)|(usage<<8)|(0);// itCell->count ? 0xffffffff : 0;
	}

	SetDIBits(dc.m_hDC, (HBITMAP)bm.m_hObject, 0, rcClient.Height(), &bits[0], &bi, DIB_RGB_COLORS);

	CDC bmDC;
	bmDC.CreateCompatibleDC(&dc);

	CBitmap* oldBM = bmDC.SelectObject(&bm);

	dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &bmDC, 0, 0, SRCCOPY);

	bmDC.SelectObject(oldBM);
}

void CDistributionMapWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	SetFocus();
}

BOOL CDistributionMapWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	BOOL result = CStatic::OnMouseWheel(nFlags, zDelta, pt);

	m_stops -= zDelta / WHEEL_DELTA;
	m_invScale = 1 << m_stops;

	UpdateScrollBar();
	Invalidate();

	return result;
}

void CDistributionMapWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CRect rcClient;
	GetClientRect(&rcClient);

	switch(nSBCode)
	{
	case SB_THUMBTRACK:
		{
			SCROLLINFO si = {0};
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_TRACKPOS;
			GetScrollInfo(SB_HORZ, &si);

			m_panX = si.nTrackPos;
		}
		break;

	case SB_LINELEFT:
		-- m_panX;
		break;

	case SB_LINERIGHT:
		++ m_panX;
		break;

	case SB_PAGELEFT:
		m_panX -= rcClient.Width();
		break;

	case SB_PAGERIGHT:
		m_panX += rcClient.Width();
		break;
	}
		
	SetScrollPos(SB_HORZ, m_panX);

	Invalidate();
}

void CDistributionMapWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CRect rcClient;
	GetClientRect(&rcClient);

	switch(nSBCode)
	{
	case SB_THUMBTRACK:
		{
			SCROLLINFO si = {0};
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_TRACKPOS;
			GetScrollInfo(SB_VERT, &si);

			m_panY = si.nTrackPos;
		}
		break;

	case SB_LINELEFT:
		-- m_panY;
		break;

	case SB_LINERIGHT:
		++ m_panY;
		break;

	case SB_PAGELEFT:
		m_panY -= rcClient.Height();
		break;

	case SB_PAGERIGHT:
		m_panY += rcClient.Height();
		break;
	}
		
	SetScrollPos(SB_VERT, m_panY);

	Invalidate();
}

LRESULT CDistributionMapWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		// Reenable non client messages, so the scroll bar works again.
	case WM_NCCALCSIZE:
	case WM_NCHITTEST:
	case WM_NCPAINT:
	case WM_NCACTIVATE:
	case WM_NCMOUSEMOVE:
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONUP:
	case WM_NCLBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	case WM_NCMBUTTONDBLCLK:
		return ::DefWindowProc(m_hWnd, message, wParam, lParam);
	}

	return CStatic::WindowProc(message, wParam, lParam);
}

void CDistributionMapWnd::UpdateScrollBar()
{
	CRect rcClient;
	GetClientRect(&rcClient);

	int virtualWidth = 65536 >> m_stops;// static_cast<int>(MaxAxisRange * pow(2.0, (double) m_zoomStops));
	int virtualHeight = 65536 >> m_stops;//static_cast<int>(MaxAxisRange * pow(2.0, (double) m_zoomStops));

	SCROLLINFO si = {0};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nMin = 0;
	si.nMax = virtualWidth;
	si.nPage = rcClient.Width();
	si.nPos = m_panX;

	SetScrollInfo(SB_HORZ, &si);

	si.nMax = virtualHeight;
	si.nPage = rcClient.Height();
	si.nPos = m_panY;

	SetScrollInfo(SB_VERT, &si);
}

void CDistributionMapWnd::RaiseActiveAllocsChanged()
{
	NMHDR hdr;
	hdr.code = DMW_ACTIVEALLOCSCHANGED;
	hdr.hwndFrom = m_hWnd;
	hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
	::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&hdr));
}
