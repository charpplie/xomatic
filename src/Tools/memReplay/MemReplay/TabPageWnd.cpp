#include "stdafx.h"
#include "TabPageWnd.h"

#include "resource.h"

static void	Make24BppImageList(UINT inBitmapID, CImageList& outImageList)
{
	CBitmap bm;

	bm.Attach(::LoadImage (::AfxFindResourceHandle(
		MAKEINTRESOURCE(inBitmapID), RT_BITMAP),
		MAKEINTRESOURCE(inBitmapID), IMAGE_BITMAP, 0, 0,
		(LR_DEFAULTSIZE | LR_CREATEDIBSECTION)));

	outImageList.Create(16, 16, 24, 2, 0);
	outImageList.Add(&bm, RGB(0, 0, 0));
}

BEGIN_MESSAGE_MAP(CTabPageWnd, CTabCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

CTabPageWnd::CTabPageWnd()
	: m_activeTab(0)
	, m_hoverTab(-1)
{
}

void CTabPageWnd::AddPage(CDialog* page, DWORD flags)
{
	int idx = (int) m_tabs.size();
	m_tabs.push_back(TabDef(page, flags));
	const TCHAR* title = _T("Unnamed");

	TCHAR pageTitle[256] = {0};
	page->GetWindowText(pageTitle, 256);

	if (pageTitle[0])
		title = pageTitle;

	if (m_activeTab == idx)
		page->ShowWindow(SW_SHOW);
	else
		page->ShowWindow(SW_HIDE);

	InsertItem(idx, title);
	SizePages();
}

void CTabPageWnd::RemovePage(size_t idx)
{
	if (idx < m_tabs.size())
	{
		m_tabs.erase(m_tabs.begin() + idx);
		DeleteItem((int) idx);
	}

	m_activeTab = std::max(0, std::min(m_activeTab, (int) m_tabs.size() - 1));

	if ((m_activeTab >= 0) && (m_activeTab < (int) m_tabs.size()))
	{
		CDialog* tab = m_tabs[m_activeTab].tab;
		tab->ShowWindow(SW_SHOW);
		tab->SetFocus();
		SetCurSel(m_activeTab);
	}
}

void CTabPageWnd::SetActivePage(CDialog* page)
{
	std::vector<TabDef>::iterator it = std::find_if(m_tabs.begin(), m_tabs.end(), TabMatches(page));
	if (it != m_tabs.end())
	{
		size_t idx = it - m_tabs.begin();
		if (idx != m_activeTab)
		{
			CDialog* activeTab = m_tabs[m_activeTab].tab;

			activeTab->ShowWindow(SW_HIDE);

			m_activeTab = idx;
			SetCurSel(idx);

			activeTab = m_tabs[m_activeTab].tab;
			activeTab->ShowWindow(SW_SHOW);
			activeTab->SetFocus();
		}
	}
}

void CTabPageWnd::SetActivePage(int page)
{
	if (page >= 0 && page < (int) m_tabs.size())
	{
		size_t idx = (size_t) page;

		if (idx != m_activeTab)
		{
			CDialog* activeTab = m_tabs[m_activeTab].tab;

			activeTab->ShowWindow(SW_HIDE);

			SetCurSel(idx);
			m_activeTab = idx;

			activeTab = m_tabs[m_activeTab].tab;
			activeTab->ShowWindow(SW_SHOW);
			activeTab->SetFocus();

			RaisePageChanged();
		}
	}
}

int CTabPageWnd::GetPageIndex(CDialog* page)
{
	std::vector<TabDef>::iterator it = std::find_if(m_tabs.begin(), m_tabs.end(), TabMatches(page));
	if (it != m_tabs.end())
	{
		return it - m_tabs.begin();
	}

	return -1;
}

BOOL CTabPageWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CTabCtrl::PreCreateWindow(cs))
		return FALSE;

	cs.style |= WS_CLIPCHILDREN | TCS_OWNERDRAWFIXED;
	return TRUE;
}

int CTabPageWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CTabCtrl::OnCreate(lpCreateStruct);
	Make24BppImageList(IDB_TABICONS, m_images);

	return TRUE;
}

void CTabPageWnd::OnSize(UINT nType, int cx, int cy)
{
	CTabCtrl::OnSize(nType, cx, cy);
	SetPadding(CSize(16, 0));

	SizePages();
}

void CTabPageWnd::SizePages()
{
	CRect rcTab, rcItem;
	GetClientRect(&rcTab);
	GetItemRect(0, &rcItem);

	int x = rcItem.left;
	int y = rcItem.bottom + 1;
	int xc = rcTab.right - rcItem.left - 1;
	int yc = rcTab.bottom - y - 1;

	for (std::vector<TabDef>::iterator it = m_tabs.begin(), itEnd = m_tabs.end(); it != itEnd; ++ it)
		it->tab->SetWindowPos(&wndTop, x, y, xc, yc, (it - m_tabs.begin()) == m_activeTab ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
}

void CTabPageWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	TCHITTESTINFO hitTestInfo = {0};
	hitTestInfo.pt = point;
	int sel = HitTest(&hitTestInfo);

	if (sel >= 0 && (hitTestInfo.flags & TCHT_ONITEM))
	{
		const CRect& rcItem = m_tabs[sel].rcCloseBounds;

		if (!rcItem.PtInRect(point))
		{
			CTabCtrl::OnLButtonDown(nFlags, point);
			SetActivePage(GetCurSel());
		}
	}
}

void CTabPageWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	TCHITTESTINFO hitTestInfo = {0};
	hitTestInfo.pt = point;
	int sel = HitTest(&hitTestInfo);

	if (sel >= 0 && (hitTestInfo.flags & TCHT_ONITEM))
	{
		const CRect& rcItem = m_tabs[sel].rcCloseBounds;
		bool allowClose = (m_tabs[sel].flags & TPW_ALLOWCLOSE) != 0;

		if (allowClose && rcItem.PtInRect(point))
			RaiseCloseClicked(sel);
	}
}

void CTabPageWnd::OnMButtonUp(UINT nFlags, CPoint point)
{
	TCHITTESTINFO hitTestInfo = {0};
	hitTestInfo.pt = point;
	int sel = HitTest(&hitTestInfo);

	if (sel >= 0 && (hitTestInfo.flags & TCHT_ONITEM))
	{
		if (m_tabs[sel].flags & TPW_ALLOWCLOSE)
			RaiseCloseClicked(sel);
	}
}

void CTabPageWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	CTabCtrl::OnMouseMove(nFlags, point);

	TCHITTESTINFO hitTestInfo = {0};
	hitTestInfo.pt = point;
	int sel = HitTest(&hitTestInfo);

	int nht = -1;

	if (sel >= 0 && (hitTestInfo.flags & TCHT_ONITEM))
	{
		const CRect& rcTab = m_tabs[sel].rcCloseBounds;
		if (rcTab.PtInRect(point))
			nht = sel;
	}

	if (nht != m_hoverTab)
	{
		if (m_hoverTab >= 0)
		{
			CRect rcTab;
			GetItemRect(m_hoverTab, rcTab);
			InvalidateRect(rcTab);
		}

		if (nht >= 0)
		{
			CRect rcTab;
			GetItemRect(nht, rcTab);
			InvalidateRect(rcTab);

			TRACKMOUSEEVENT track = {0};
			track.cbSize = sizeof(track);
			track.dwFlags = TME_LEAVE;
			track.hwndTrack = m_hWnd;
			TrackMouseEvent(&track);
		}

		m_hoverTab = nht;
	}
}

void CTabPageWnd::OnMouseLeave()
{
	CTabCtrl::OnMouseLeave();

	if (m_hoverTab >= 0)
	{
		CRect rcTab;
		GetItemRect(m_hoverTab, rcTab);
		InvalidateRect(rcTab);
		m_hoverTab = -1;
	}
}

void CTabPageWnd::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CRect rect = lpDrawItemStruct->rcItem;
	int idx = lpDrawItemStruct->itemID;
	if (idx < 0)
		return;

	bool selected = idx == GetCurSel();

	char label[128];
	TC_ITEM tci;
	tci.mask = TCIF_TEXT|TCIF_IMAGE;
	tci.pszText = label;     
	tci.cchTextMax = 127;
	if (!GetItem(idx, &tci ))
		return;

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	if (!pDC)
		return;

	int savedDC = pDC->SaveDC();

	int margin = pDC->GetTextExtent(_T(" ")).cx;

	rect.top += ::GetSystemMetrics(SM_CYEDGE);

	bool allowClose = (m_tabs[idx].flags & TPW_ALLOWCLOSE) != 0;

	if (allowClose)
		rect.right -= margin + 16;

	pDC->SetBkMode(TRANSPARENT);
	pDC->FillSolidRect(rect, ::GetSysColor(COLOR_BTNFACE));

	if (selected)
		rect.top -= GetSystemMetrics(SM_CYEDGE);

	CRect rcCalc = rect;
	pDC->DrawText(label, rcCalc, DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_CALCRECT);
	rect.left += rect.Width() / 2 - rcCalc.Width() / 2;
	rect.right = rect.left + rcCalc.Width();
	rect.top += rect.Height() / 2 - rcCalc.Height() / 2;
	rect.bottom = rect.top + rcCalc.Height();
	pDC->DrawText(label, rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);

	if (allowClose)
	{
		int closeButtonImage = idx == m_hoverTab;

		IMAGEINFO info;
		m_images.GetImageInfo(closeButtonImage, &info);

		CRect rcImage = info.rcImage;

		m_images.Draw(pDC, closeButtonImage, CPoint(rect.right + margin, rect.top), ILD_TRANSPARENT);
		m_tabs[idx].rcCloseBounds = CRect(rect.right + margin, rect.top, rect.right + margin + rcImage.Width(), rect.top + rcImage.Height());
	}

	pDC->RestoreDC(savedDC);
}

void CTabPageWnd::RaisePageChanged()
{
	NMHDR hdr = {0};
	hdr.code = TPW_PAGECHANGED;
	hdr.hwndFrom = m_hWnd;
	hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
	::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&hdr));
}

void CTabPageWnd::RaiseCloseClicked(int index)
{
	CloseClicked cc;
	cc.hdr.code = TPW_CLOSECLICKED;
	cc.hdr.hwndFrom = m_hWnd;
	cc.hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
	cc.index = index;
	::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&cc));
}
