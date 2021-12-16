#include "stdafx.h"
#include "CheckedTreeCtrl.h"

BEGIN_MESSAGE_MAP(CCheckedTreeCtrl, CTreeCtrl)
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

void CCheckedTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	TVHITTESTINFO ht = {0};
	ht.pt = point;

	HitTest(&ht);

	if (ht.hItem && ht.flags & TVHT_ONITEMSTATEICON)
	{
		RaiseCheckChanged(ht.hItem);
	}
	else
	{
		CTreeCtrl::OnLButtonUp(nFlags, point);
	}
}

void CCheckedTreeCtrl::RaiseCheckChanged(HTREEITEM item)
{
	CheckChanged nm;
	nm.hdr.code = CTC_CHECKCHANGED;
	nm.hdr.hwndFrom = m_hWnd;
	nm.hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
	nm.hItem = item;
	::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&nm));
}
