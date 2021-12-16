#include "stdafx.h"
#include "MemoryGraphWnd.h"
#include "MemReplayDoc.h"

BEGIN_MESSAGE_MAP(CMemoryGraphWnd, CStatic)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

CMemoryGraphWnd::CMemoryGraphWnd()
	: m_source(NULL)
	, m_markerSource(NULL)
	, m_cursor(0)
	, m_startCursor(0)
	, m_endCursor(0)
	, m_draggedCursor(0)
	, m_scaleX(1.0f)
	, m_scaleY(1.0f)
{
}

void CMemoryGraphWnd::SetStreamSource(const IMemoryGraphStreamSource* source)
{
	m_source = source;

	if (m_hWnd)
	{
		UpdateScrollRange();
		Invalidate();
	}
}

void CMemoryGraphWnd::SetMarkerSource(const IMemoryGraphMarkerSource* source)
{
	m_markerSource = source;

	if (m_hWnd)
	{
		Invalidate();
	}
}

void CMemoryGraphWnd::SetHoverCursorPosition(double c)
{
	if (m_cursor != c)
	{
		ResetCursor(m_cursor, c);
	}
}

void CMemoryGraphWnd::ClearSelection()
{
	ResetCursor(m_startCursor, 0.0);
	ResetCursor(m_endCursor, 0.0);
}

int CMemoryGraphWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	int result = CStatic::OnCreate(lpCreateStruct);

	if (!m_tickFont.m_hObject)
	{
		m_tickFont.CreateFont(
			-TickFontHeight,
			0,
			0,
			0,
			FW_DONTCARE,
			FALSE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_MODERN,
			NULL);
	}

	if (!m_labelFont.m_hObject)
	{
		m_labelFont.CreateFont(
			-LabelFontHeight,
			0,
			900, 900,
			FW_BOLD,
			FALSE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY,
			DEFAULT_PITCH | FF_MODERN,
			NULL);
	}

	return result;
}

void CMemoryGraphWnd::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	GetClientRect(m_rcGraphClient);

	if (m_bbBitmap.m_hObject)
		m_bbBitmap.DeleteObject();

	CDC* selfDC = GetDC();
	m_bbBitmap.CreateCompatibleBitmap(selfDC, m_rcGraphClient.Width(), m_rcGraphClient.Height());
	ReleaseDC(selfDC);

	UpdateScrollRange();
	Invalidate();
}

void CMemoryGraphWnd::OnPaint()
{
	CPaintDC dc(this);
	
	CRect rcClient;
	GetClientRect(rcClient);

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);

	CRect rcPaint(dc.m_ps.rcPaint);

	CBitmap* oldBM = memDC.SelectObject(&m_bbBitmap);

	CBrush fillBrush(RGB(0, 0, 0));
	memDC.FillRect(rcPaint, &fillBrush);

	const int maxMBUsage = 512 * 1024 * 1024;

	if (m_source != NULL)
	{
		double lengthVisible = (m_source->GetSamplePointCount()) * m_scaleX;

		double dv = lengthVisible / static_cast<double>(m_rcGraphClient.Width());
		double layer = log(dv) / log(2.0);
		int nLayer = std::max(0, (int) layer);

		double scaleT = (lengthVisible / (1<<nLayer)) / static_cast<double>(m_rcGraphClient.Width());
		double scaleV = m_rcGraphClient.Height() / (maxMBUsage * m_scaleY);

		int panX = GetScrollPos(SB_HORZ);
		int panY = GetScrollLimit(SB_VERT) - GetScrollPos(SB_VERT);

		COLORREF selectionColour = RGB(121,188,255);
		CBrush selectionBrush(selectionColour);

		double selectionMin = std::min(m_startCursor, m_endCursor);
		double selectionMax = std::max(m_startCursor, m_endCursor);

		int scrollHorz = GetScrollPos(SB_HORZ);
		int selectionLeft = ClientXFromSampleT(selectionMin) - scrollHorz;
		int selectionRight = ClientXFromSampleT(selectionMax) - scrollHorz;

		selectionLeft = Clamp<int>(rcPaint.left, rcPaint.right, selectionLeft);
		selectionRight = Clamp<int>(rcPaint.left, rcPaint.right, selectionRight);

		if (selectionLeft != selectionRight)
		{
			CRect rcSelection(selectionLeft, rcPaint.top, selectionRight, rcPaint.bottom);
			memDC.FillRect(rcSelection, &selectionBrush);
		}

		{
			CPen tickPen(PS_SOLID, 1, RGB(0x80, 0x80, 0x80));
			CPen tickSelectionPen(PS_SOLID, 1, RGB((0x80 + 121)/2, (0x80 + 188) / 2, (0x80 + 0xff) / 2));
			CPen* oldPen = memDC.SelectObject(&tickPen);
			CFont* oldFont = memDC.SelectObject(&m_tickFont);
			int oldBkMode = memDC.SetBkMode(TRANSPARENT);
			COLORREF oldTextColor = memDC.SetTextColor(RGB(0xff, 0xff, 0xff));

			float tickScale = m_rcGraphClient.Height() / (512.0f * m_scaleY);
			int tickDelta = std::max(1, 32 / std::max(1, static_cast<int>(tickScale)));

			for (int tick = 0; tick < 512; tick += tickDelta)
			{
				int y = m_rcGraphClient.bottom - static_cast<int>(tick * tickScale) + panY;

				if (y >= rcPaint.top && y <= rcPaint.bottom)
				{
					memDC.MoveTo(rcPaint.left, y);
					memDC.LineTo(rcPaint.right, y);

					if (selectionLeft < selectionRight)
					{
						CPen* prevPen = memDC.SelectObject(&tickSelectionPen);

						memDC.MoveTo(selectionLeft, y);
						memDC.LineTo(selectionRight, y);

						memDC.SelectObject(prevPen);
					}
				}

				CRect rcTickText(0, y - TickFontHeight, rcPaint.right, y);

				CRect rcIntersection;
				if (::IntersectRect(rcIntersection, rcPaint, rcTickText))
				{
					TCHAR tickText[32];
					_stprintf_s(tickText, _T("%iMB"), tick);
					memDC.DrawText(tickText, rcTickText, DT_BOTTOM | DT_LEFT | DT_NOCLIP | DT_SINGLELINE);
				}
			}

			memDC.SetTextColor(oldTextColor);
			memDC.SetBkColor(oldBkMode);
			memDC.SelectObject(oldFont);
			memDC.SelectObject(oldPen);
		}

		if (m_markerSource)
		{
			typedef std::vector<std::pair<COLORREF, CBrush*> > BrushVector;
			BrushVector brushes;

			CFont* oldFont = memDC.SelectObject(&m_labelFont);
			int oldBkMode = memDC.SetBkMode(TRANSPARENT);
			COLORREF oldTextColor = memDC.SetTextColor(RGB(0x00, 0xff, 0xff));
			memDC.SetTextAlign(TA_BOTTOM | TA_LEFT);

			for (size_t markerIdx = 0, markerCount = m_markerSource->GetMarkerCount(); markerIdx != markerCount; ++ markerIdx)
			{
				const IMemoryGraphMarkerSource::MarkerInfo& mi = m_markerSource->GetMarkerInfo(markerIdx);
				double offset = mi.offset;

				CBrush* markerBrush;

				int l = (int) (ClientXFromSampleT((double) offset) - panX);

				COLORREF markerColour = mi.colour;

				if (rcPaint.PtInRect(CPoint(l, rcPaint.top)))
				{
					if (l >= selectionLeft && l < selectionRight)
					{
						markerColour >>= 1;
						markerColour &= 0x7f7f7f7f;

						COLORREF sc = selectionColour;
						sc >>= 1;
						sc &= 0x7f7f7f7f;

						markerColour += sc;
					}

					BrushVector::iterator itBrush = std::lower_bound(brushes.begin(), brushes.end(), std::pair<COLORREF, CBrush*>(markerColour, NULL));
					if (itBrush != brushes.end() && itBrush->first == markerColour)
						markerBrush = itBrush->second;
					else
						markerBrush = brushes.insert(itBrush, std::pair<COLORREF, CBrush*>(markerColour, new CBrush(markerColour)))->second;

					memDC.FillRect(CRect(l, m_rcGraphClient.top, l + 1, m_rcGraphClient.bottom), markerBrush);
				}

				if (mi.name[0])
				{
					CRect rcText(0, 0, 0, 0);

					rcText.bottom = memDC.DrawText(mi.name, -1, rcText, DT_SINGLELINE | DT_CALCRECT);
					rcText.MoveToXY(l, m_rcGraphClient.bottom - rcText.bottom);

					::ExtTextOut(memDC.m_hDC, rcText.left, rcText.top, 0, &rcText, mi.name, strlen(mi.name), NULL);
				}
			}

			for (BrushVector::iterator itBrush = brushes.begin(), itBrushEnd = brushes.end(); itBrush != itBrushEnd; ++ itBrush)
				delete itBrush->second;

			memDC.SelectObject(oldFont);
			memDC.SetTextColor(oldTextColor);
			memDC.SetBkColor(oldBkMode);
		}

		const int tBegin = std::max(rcPaint.left - 1, m_rcGraphClient.left);
		const int tEnd = std::min(rcPaint.right + 1, m_rcGraphClient.right);
		const int tWidth = tEnd - tBegin;

		int streamCount = m_source->GetStreamCount();
		std::vector<int> valTemp(streamCount * (tWidth + 1));
		std::vector<int> valIntermed(streamCount * 2);

		for (int t = tBegin; t < tEnd; ++ t)
		{
			float sampleT = (t + panX) * scaleT;
			float sampleFrac = sampleT - floorf(sampleT);

			int nSampleT = (int) sampleT;
			int nSampleT1 = nSampleT + 1;

			m_source->SampleAt(&valIntermed[0], nSampleT, nLayer);
			m_source->SampleAt(&valIntermed[streamCount], nSampleT1, nLayer);

			for (int j = 0; j < streamCount; ++ j)
			{
				int offset = (t - tBegin) * streamCount + j;

				valTemp[offset] = static_cast<int>(
					lerp(static_cast<float>(valIntermed[j]),
							 static_cast<float>(valIntermed[j + streamCount]),
							 sampleFrac));
			}
		}

		for (int str = streamCount - 1; str >= 0; -- str)
		{
			CPen pen(PS_SOLID, 2, m_source->GetStreamInfo(str).colour);
			CPen* oldPen = memDC.SelectObject(&pen);

			for (int t = tBegin; t < tEnd; ++ t)
			{
				float sampleT = (t + panX) * scaleT;

				int offset = (t - tBegin) * streamCount + str;
				int sa = valTemp[offset];

				int sampleUsage = static_cast<int>(valTemp[offset] * scaleV);
				int height = std::max(0, sampleUsage);

				if (t == tBegin)
					memDC.MoveTo(t, m_rcGraphClient.bottom - height + panY);
				else
					memDC.LineTo(t, m_rcGraphClient.bottom - height + panY);
			}

			memDC.SelectObject(oldPen);
		}

		CBrush cursorBrush(RGB(0x00, 0x00, 0xff));
		PaintCursor(memDC, cursorBrush, m_cursor);

		CBrush startCursorBrush(RGB(0xff, 0xff, 0x00));
		CBrush endCursorBrush(RGB(0x00, 0xff, 0xff));
		PaintCursor(memDC, startCursorBrush, m_startCursor);
		PaintCursor(memDC, endCursorBrush, m_endCursor);
	}

	dc.BitBlt(
		rcPaint.left, rcPaint.top, rcPaint.right - rcPaint.left, rcPaint.bottom - rcPaint.top,
		&memDC,
		rcPaint.left, rcPaint.top,
		SRCCOPY);

	memDC.SelectObject(oldBM);
}

BOOL CMemoryGraphWnd::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CMemoryGraphWnd::PaintCursor(CDC& dc, CBrush& brush, float cursorPos)
{
	int l = ClientXFromSampleT(cursorPos) - GetScrollPos(SB_HORZ);
	dc.FillRect(CRect(l, m_rcGraphClient.top, l + 1, m_rcGraphClient.bottom), &brush);
}

void CMemoryGraphWnd::UpdateScrollRange()
{
	CRect rcClient;
	GetClientRect(rcClient);

	double width = rcClient.Width() / m_scaleX;
	double height = rcClient.Height() / m_scaleY;

	SCROLLINFO si = {0};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nMin = 0;
	si.nMax = (int) width;
	si.nPage = rcClient.Width();
	si.nPos = GetScrollPos(SB_HORZ);

	SetScrollInfo(SB_HORZ, &si);

	si.nMax = (int) height;
	si.nPage = rcClient.Height();
	si.nPos = GetScrollPos(SB_VERT);

	SetScrollInfo(SB_VERT, &si);
}

void CMemoryGraphWnd::RaiseRangeChangedMessage()
{
	NMHDR hdr;
	hdr.code = MGW_RANGECHANGED;
	hdr.hwndFrom = m_hWnd;
	hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
	::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&hdr));
}

void CMemoryGraphWnd::RaiseRangeChangingMessage()
{
	NMHDR hdr;
	hdr.code = MGW_RANGECHANGING;
	hdr.hwndFrom = m_hWnd;
	hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
	::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&hdr));
}

void CMemoryGraphWnd::RaiseHoverChangedMessage(double cursorX, double cursorY)
{
	HoverChangedHeader hdr = {0};
	hdr.hdr.code = MGW_HOVERCHANGED;
	hdr.hdr.hwndFrom = m_hWnd;
	hdr.hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
	hdr.cursorX = cursorX;
	hdr.cursorY = cursorY;
	::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&hdr));
}

void CMemoryGraphWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	SetFocus();

	CRect rcClient;
	GetClientRect(rcClient);

	CPoint ptCursor;
	GetCursorPos(&ptCursor);
	ScreenToClient(&ptCursor);

	double hover = SampleTFromClientX(ptCursor.x);

	if (m_draggedCursor && (nFlags & MK_LBUTTON))
	{
		ResetCursor(*m_draggedCursor, hover);

		RaiseRangeChangingMessage();
	}
	else if(nFlags & MK_RBUTTON)
	{
		CPoint dp(point.x - m_rmbDragStart.x, point.y - m_rmbDragStart.y);

		double startT = SampleTFromClientX(m_rmbDragOrigin.x);
		double startV = SampleVFromClientY(m_rmbDragOrigin.y);

		m_scaleX -= (dp.x * m_scaleX * 0.01f);
		m_scaleY += (dp.y * m_scaleY * 0.01f);

		m_scaleX = Clamp(0.0001f, 1.0f, m_scaleX);
		m_scaleY = Clamp(0.0001f, 1.0f, m_scaleY);

		UpdateScrollRange();

		int clientT = ClientXFromSampleT(startT);
		int clientV = ClientYFromSampleV(startV);

		SetScrollPos(SB_HORZ, (int) clientT - m_rmbDragOrigin.x);
		SetScrollPos(SB_VERT, (int) clientV - m_rmbDragOrigin.y);

		m_rmbDragStart = point;
		Invalidate();
	}
	else
	{
		double posTolerance = SampleTFromClientX(5 - GetScrollPos(SB_HORZ));

		if (
			within(hover, posTolerance, m_startCursor) ||
			within(hover, posTolerance, m_endCursor))
		{
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		}
		else
		{
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}

		ResetCursor(m_cursor, hover, false);
		RaiseHoverChangedMessage(hover, SampleVFromClientY(ptCursor.y));
	}
}

void CMemoryGraphWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();

	double hover = SampleTFromClientX(point.x);
	double posTolerance = SampleTFromClientX(5 - GetScrollPos(SB_HORZ));

	if (within(hover, posTolerance, m_startCursor))
	{
		m_draggedCursor = &m_startCursor;
	}
	else if (within(hover, posTolerance, m_endCursor))
	{
		m_draggedCursor = &m_endCursor;
	}
	else
	{
		ResetCursor(m_startCursor, SampleTFromClientX(point.x));
		ResetCursor(m_endCursor, SampleTFromClientX(point.x));
		m_draggedCursor = &m_endCursor;
	}
}

void CMemoryGraphWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();

	RaiseRangeChangedMessage();
}

void CMemoryGraphWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_rmbDragStart = point;
	m_rmbDragOrigin = point;
	SetCapture();
}

void CMemoryGraphWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
}

BOOL CMemoryGraphWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	ScreenToClient(&pt);
	double sampleT = SampleTFromClientX(pt.x);
	double sampleV = SampleVFromClientY(pt.y);

	float zdf = -zDelta / ((float) WHEEL_DELTA);

	m_scaleX *= powf(2.0f, zdf);
	m_scaleY *= powf(2.0f, zdf);

	m_scaleX = Clamp(0.0001f, 1.0f, m_scaleX);
	m_scaleY = Clamp(0.0001f, 1.0f, m_scaleY);

	UpdateScrollRange();

	int clientT = ClientXFromSampleT(sampleT);
	int clientV = ClientYFromSampleV(sampleV);
	SetScrollPos(SB_HORZ, (int) clientT - pt.x);
	SetScrollPos(SB_VERT, (int) clientV - pt.y);

	Invalidate();

	return TRUE;
}

double CMemoryGraphWnd::SampleTFromClientX(int t)
{
	if (!m_source)
		return 0;

	double lengthVisible = (m_source->GetSamplePointCount()) * m_scaleX;

	double scaleT = lengthVisible / m_rcGraphClient.Width();
	return ((t + GetScrollPos(SB_HORZ)) * scaleT);
}

int CMemoryGraphWnd::ClientXFromSampleT(double t)
{
	if (!m_source)
		return 0;

	double lengthVisible = (m_source->GetSamplePointCount()) * m_scaleX;

	double scaleT = m_rcGraphClient.Width() / lengthVisible;
	return (int) (t * scaleT);
}

double CMemoryGraphWnd::SampleVFromClientY(int y)
{
	if (!m_source)
		return 0;

	double h_p = m_rcGraphClient.Height() / m_scaleY;
	double h_v = 512;
	double s_p = GetScrollPos(SB_VERT);
	double y_p = y;

	return h_v - (y_p + s_p) / h_p * h_v;
}

int CMemoryGraphWnd::ClientYFromSampleV(double v)
{
	if (!m_source)
		return 0;

	double h_p = m_rcGraphClient.Height() / m_scaleY;
	double h_v = 512;
	double s_p = GetScrollPos(SB_VERT);
	double y_v = v;

	return static_cast<int>(-(((y_v - h_v) * h_p) / h_v));// + s_p);
}

void CMemoryGraphWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CRect rcClient;
	GetClientRect(&rcClient);

	int panX = GetScrollPos(SB_HORZ);

	switch(nSBCode)
	{
	case SB_THUMBTRACK:
		{
			SCROLLINFO si = {0};
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_TRACKPOS;
			GetScrollInfo(SB_HORZ, &si);

			panX = si.nTrackPos;
		}
		break;

	case SB_LINELEFT:
		-- panX;
		break;

	case SB_LINERIGHT:
		++ panX;
		break;

	case SB_PAGELEFT:
		panX -= rcClient.Width();
		break;

	case SB_PAGERIGHT:
		panX += rcClient.Width();
		break;
	}
		
	SetScrollPos(SB_HORZ, panX);

	Invalidate();
}

void CMemoryGraphWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CRect rcClient;
	GetClientRect(&rcClient);

	int panY = GetScrollPos(SB_VERT);

	switch(nSBCode)
	{
	case SB_THUMBTRACK:
		{
			SCROLLINFO si = {0};
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_TRACKPOS;
			GetScrollInfo(SB_VERT, &si);

			panY = si.nTrackPos;
		}
		break;

	case SB_LINELEFT:
		-- panY;
		break;

	case SB_LINERIGHT:
		++ panY;
		break;

	case SB_PAGELEFT:
		panY -= rcClient.Height();
		break;

	case SB_PAGERIGHT:
		panY += rcClient.Height();
		break;
	}
		
	SetScrollPos(SB_VERT, panY);

	Invalidate();
}

LRESULT CMemoryGraphWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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

void CMemoryGraphWnd::ResetCursor(double& cursor, double val, bool invalidateRange)
{
	if (invalidateRange)
	{
		int l = ClientXFromSampleT(cursor) - GetScrollPos(SB_HORZ);
		int r = ClientXFromSampleT(val) - GetScrollPos(SB_HORZ);

		if (l > r)
			std::swap(l, r);

		CRect rcClient;
		GetClientRect(rcClient);

		CRect rcInvalidate(std::max((int) rcClient.left, l - 1), m_rcGraphClient.top, std::min((int) rcClient.right, r + 2), m_rcGraphClient.bottom);
		if (rcInvalidate.Width() > 0)
		{
			InvalidateRect(&rcInvalidate);
		}
	}
	else
	{
		int l = ClientXFromSampleT(cursor) - GetScrollPos(SB_HORZ);
		int r = ClientXFromSampleT(val) - GetScrollPos(SB_HORZ);

		CRect rcClient;
		GetClientRect(rcClient);

		CRect rcInvalidate[] = {
			CRect(std::max((int) rcClient.left, l - 1), m_rcGraphClient.top, std::min((int) rcClient.right, l + 2), m_rcGraphClient.bottom),
			CRect(std::max((int) rcClient.left, r - 1), m_rcGraphClient.top, std::min((int) rcClient.right, r + 2), m_rcGraphClient.bottom)
		};

		for (int i = 0; i < 2; ++ i)
		{
			if (rcInvalidate[i].Width() > 0)
				InvalidateRect(&rcInvalidate[i]);
		}
	}

	cursor = val;
}