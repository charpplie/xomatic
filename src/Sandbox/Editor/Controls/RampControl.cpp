// RampControl.cpp : implementation file
//

#include "stdafx.h"
#include "RampControl.h"
#include <algorithm>

IMPLEMENT_DYNAMIC(CRampControl,CSliderCtrl)

/*****************************************************************************************************/
CRampControl::CRampControl()
{
	RegisterControlClass();
	m_max = 5;
	m_ptLeftDown = NULL;
	m_menuPoint.x = 0;
	m_menuPoint.y = 0;
	m_movedHandles = false;
	m_canMoveSelected = false;
	m_mouseMoving = false;
}

CRampControl::~CRampControl()
{
}

BOOL CRampControl::RegisterControlClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, "CRampControl", &wndcls)))
	{
		wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = 0;
		wndcls.cbWndExtra		= 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = "CRampControl";

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

/*****************************************************************************************************/
BEGIN_MESSAGE_MAP(CRampControl, CSliderCtrl)
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_KILLFOCUS()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_MENU_ADD,OnMenuAdd)
	ON_COMMAND(ID_MENU_SELECT,OnMenuSelect)
	ON_COMMAND(ID_MENU_DELETE,OnMenuDelete)
	ON_COMMAND(ID_MENU_SELECTALL,OnMenuSelectAll)
	ON_COMMAND(ID_MENU_SELECTNONE,OnMenuSelectNone)
	ON_COMMAND_RANGE(ID_MENU_CUSTOM_BEGIN,ID_MENU_CUSTOM_END,OnMenuCustom)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
END_MESSAGE_MAP()
/*****************************************************************************************************/

void CRampControl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	RECT rect;
	GetClientRect(&rect);

	CDC memdc;
	memdc.CreateCompatibleDC(&dc);
	int SavedDC = memdc.SaveDC();

	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&dc,rect.right,rect.bottom);
	memdc.SelectObject(&bmp);

	memdc.FillSolidRect(&rect,COLORREF(0xf0f0f0));

	DrawRamp(memdc);

	const int handleCount = m_handles.size();
	for (int idx = 0; idx < handleCount; ++idx)
	{
		DrawHandle(memdc, m_handles[idx]);
	}

	dc.BitBlt(0,0,rect.right,rect.bottom,&memdc,0,0,SRCCOPY);
	memdc.RestoreDC(SavedDC);
}

void CRampControl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	float percent = DeterminePercentage(point);
	if ( !HitAnyHandles(percent) )
	{	
		if ( AddHandle(percent) )
			GetParent()->PostMessage(WM_HANDLEADDED, (int)percent);
	}
	Invalidate();
}

void CRampControl::OnLButtonDown(UINT nFlags, CPoint point)
{
	float percent = DeterminePercentage(point);
	if ( HitAnyHandles(percent) && !(GetKeyState(VK_CONTROL) & 0x8000) )
	{
		ClearSelectedHandles();
		GetParent()->PostMessage(WM_HANDLESELECTIONCLEARED);
	}

	m_ptLeftDown = new CPoint(point);
	m_canMoveSelected = IsSelectedHandle(percent);
	Invalidate();
}

void CRampControl::OnLButtonUp(UINT nFlags, CPoint point)
{
	float percent = DeterminePercentage(point);
	if ( m_movedHandles == false )
	{
		if ( !HitAnyHandles(percent) && !(GetKeyState(VK_CONTROL) & 0x8000) )
		{
			ClearSelectedHandles();
			GetParent()->PostMessage(WM_HANDLESELECTIONCLEARED);
		}
		else if ( HitAnyHandles(percent) )
		{
			if ( IsSelectedHandle(percent) )
			{
				DeSelectHandle(percent);
			}
			else
			{
				if ( SelectHandle(percent) )
					GetParent()->PostMessage(WM_HANDLESELECTED);
			}
		}
	}

	if ( m_ptLeftDown )
		delete m_ptLeftDown;
	m_ptLeftDown = NULL;

	m_mouseMoving = false;
	m_canMoveSelected = false;
	m_movedHandles = false;
	SetFocus();
	ReleaseCapture();
	Invalidate();
}

void CRampControl::OnMouseMove(UINT nFlags, CPoint point)
{
	float abspercent = DeterminePercentage(point);
	SetHotItem(abspercent);
	Invalidate();

	const int handleCount = m_handles.size();
	if ( !m_ptLeftDown || handleCount == 0 )
		return;

	if (m_movedHandles == false && !m_mouseMoving && !m_canMoveSelected)
		m_canMoveSelected = SelectHandle(abspercent);
	
	m_mouseMoving = true;

	if ( !m_canMoveSelected )
		return;
		
	SetCapture();

	bool modified = false;
	
	CPoint movement = point - *m_ptLeftDown;
	float percent = DeterminePercentage(movement);

	//early out if we hit the edge limits
	for (int idx = 0; idx < handleCount; ++idx)
	{
		if ( m_handles[idx].flags & hfSelected && ((movement.x < 0 && m_handles[idx].percentage <= 1) || (movement.x > 0 && m_handles[idx].percentage >= 99)))
		{
			return;
		}
	}

	if ( movement.x > 0)
	{
		// move the hanldes clamping in direction where required
		for (int idx = handleCount-1; idx >= 0; --idx)
		{
			if ( m_handles[idx].flags & hfSelected )
			{
				float amountClamped = MoveHandle(m_handles[idx], percent);
				if ( amountClamped != 0.0f)
					percent += amountClamped;
				modified = true;
			}
		}
	}
	else if (movement.x < 0)
	{
		// move the hanldes clamping in direction where required
		for (int idx = 0; idx < handleCount; ++idx)
		{
			if ( m_handles[idx].flags & hfSelected )
			{
				float amountClamped = MoveHandle(m_handles[idx], percent);
				if ( amountClamped != 0.0f)
					percent += amountClamped;
				modified = true;
			}
		}
	}
	m_ptLeftDown->SetPoint(point.x,point.y);
	
	if ( modified )
	{
		m_movedHandles = true;
		GetParent()->PostMessage(WM_HANDLECHANGED);
		Invalidate();
		SortHandles();
	}
}

void CRampControl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( nChar == VK_DELETE )
	{
		RemoveSelectedHandles();
		GetParent()->PostMessage(WM_HANDLEDELETED);
	}

	if ( nChar == VK_LEFT )
	{
		SelectPreviousHandle();
		GetParent()->PostMessage(WM_HANDLESELECTED);
	}

	if ( nChar == VK_RIGHT )
	{
		SelectNextHandle();
		GetParent()->PostMessage(WM_HANDLESELECTED);
	}

	Invalidate();
}

void CRampControl::OnKillFocus(CWnd* pNewWnd)
{
	SetHotItem(-1.0f);
	Invalidate();
}

void CRampControl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	m_menuPoint = point;
	ScreenToClient(&m_menuPoint);
	float percent = DeterminePercentage(m_menuPoint);
	bool hit = HitAnyHandles(percent);
	bool maxed = GetHandleCount() == GetMaxHandles();

	CMenu *menu = new CMenu;
	menu->CreatePopupMenu();
	menu->AppendMenu(!maxed ? MF_STRING : MF_STRING|MF_GRAYED, ID_MENU_ADD, "&Add");
	menu->AppendMenu(MF_SEPARATOR,0,"");
	menu->AppendMenu(hit ? MF_STRING : MF_STRING|MF_GRAYED, ID_MENU_DELETE, "&Delete");
	menu->AppendMenu(hit ? MF_STRING : MF_STRING|MF_GRAYED, ID_MENU_SELECT, "&Select");
	menu->AppendMenu(MF_SEPARATOR,0,"");
	menu->AppendMenu(MF_STRING, ID_MENU_SELECTALL, "Select A&ll");
	menu->AppendMenu(MF_STRING, ID_MENU_SELECTNONE, "Select &None");
	AddCustomMenuOptions(menu);
	menu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON,point.x,point.y,pWnd);
	delete menu;
}

void CRampControl::OnMenuAdd()
{
	float percent = DeterminePercentage(m_menuPoint);
	if ( AddHandle(percent) )
		GetParent()->PostMessage(WM_HANDLEADDED, (int)percent);
	Invalidate();
}

void CRampControl::OnMenuSelect()
{
	ClearSelectedHandles();
	float percent = DeterminePercentage(m_menuPoint);
	if ( SelectHandle(percent) )
		GetParent()->PostMessage(WM_HANDLESELECTED, (int)percent);
	Invalidate();
}

void CRampControl::OnMenuDelete()
{
	ClearSelectedHandles();
	float percent = DeterminePercentage(m_menuPoint);
	SelectHandle(percent);
	RemoveSelectedHandles();
	GetParent()->PostMessage(WM_HANDLEDELETED);
	Invalidate();
}

void CRampControl::OnMenuSelectAll()
{
	SelectAllHandles();
	GetParent()->PostMessage(WM_HANDLESELECTED);
	Invalidate();
}

void CRampControl::OnMenuSelectNone()
{
	ClearSelectedHandles();
	GetParent()->PostMessage(WM_HANDLESELECTIONCLEARED);
	Invalidate();
}

BOOL CRampControl::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

BOOL CRampControl::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
		switch (pMsg->wParam)
		{
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
		case VK_HOME:
		case VK_END:
			SendMessage (pMsg->message, pMsg->wParam, pMsg->lParam);
			return TRUE;
		}
		break;
	}
	return __super::PreTranslateMessage(pMsg);
}

void CRampControl::OnSize(UINT nType, int cx, int cy)
{
	Invalidate();
}

/*****************************************************************************************************/
void CRampControl::DrawRamp(CDC &dc)
{
	RECT rect;
	GetClientRect(&rect);

	DrawBackground(dc);

	POINT points[2];
	points[0].x = rect.left;
	points[1].x = rect.right;
	points[0].y = rect.top + ((rect.bottom - rect.top)/2);
	points[1].y = points[0].y;
	dc.Polyline(points,ARRAYSIZE(points));

	for (int idx = 0; idx < 100; ++idx)
	{
		points[0].x = (int)((((float)(rect.right-rect.left))/100.0f)*idx);
		points[0].y = rect.top + ((rect.bottom - rect.top)/2) - 5;
		points[1].x = points[0].x;
		points[1].y = points[0].y + 5;

		if (idx % 5 == 0)
			points[0].y -= 2;

		if (idx % 10 == 0)
			points[0].y -= 3;
		
		dc.Polyline(points,ARRAYSIZE(points));
	}

	DrawForeground(dc);

	if (GetFocus() == this)
	{
		dc.DrawFocusRect(&rect);
	}
}

void CRampControl::DrawHandle(CDC &dc, const HandleData& data)
{
	RECT rect;
	GetClientRect(&rect);
	const float pixelsPercent = ((float)rect.right - (float)rect.left) / 100.0f;

	POINT handlePoints[2];
	handlePoints[0].x = (int)(pixelsPercent * data.percentage);
	data.flags & hfSelected && m_drawPercentage ? handlePoints[0].y = rect.top + 14 : handlePoints[0].y = rect.top;
	handlePoints[1].x = handlePoints[0].x;
	handlePoints[1].y = rect.bottom;

	CPen linebrush;
	if ( data.flags & hfHotitem )
	{
		linebrush.CreatePen(0,1,COLORREF(0x0000FF));
		dc.SelectObject(&linebrush);
	}
	else
	{	
		linebrush.CreatePen(0,1,COLORREF(0x000000));
		dc.SelectObject(&linebrush);
	}
	
	dc.Polyline(handlePoints,ARRAYSIZE(handlePoints));

	handlePoints[0].x = handlePoints[0].x - 3;
	handlePoints[1].x = handlePoints[1].x + 4;
	handlePoints[1].y = handlePoints[1].y;
	handlePoints[0].y = handlePoints[1].y - 7;
	
	CBrush brush(COLORREF(0xffffff));
	dc.FillRect(CRect(handlePoints[0],handlePoints[1]),&brush);

	if ( data.flags & hfSelected )
	{
		CBrush brushNew(COLORREF(0xff0000));
		dc.FillRect(CRect(handlePoints[0],handlePoints[1]),&brushNew);

		if ( m_drawPercentage )
		{
			CString str;
			str.Format("%d",(int)data.percentage);

			handlePoints[0].x = (int)(pixelsPercent * data.percentage) - 7;
			handlePoints[1].x = handlePoints[1].x + 5;
			handlePoints[1].y = rect.top;
			handlePoints[0].y = rect.top+10;

			dc.SetBkMode(TRANSPARENT);
			dc.DrawText(str,CRect(handlePoints[0],handlePoints[1]),DT_NOCLIP|DT_CENTER|DT_SINGLELINE|DT_VCENTER);
		}
	}
}

void CRampControl::DrawBackground(CDC &dc)
{

}

void CRampControl::DrawForeground(CDC &dc)
{

}

/*****************************************************************************************************/

void CRampControl::AddCustomMenuOptions(CMenu * menu)
{

}

void CRampControl::OnMenuCustom(UINT nID)
{

}

/*****************************************************************************************************/
float CRampControl::DeterminePercentage(const CPoint& point)
{
	RECT rect;
	GetClientRect(&rect);
	float percent = ((float)(point.x - rect.left) / (float)(rect.right-rect.left)) * 100.0f;
	return percent;
}

bool CRampControl::AddHandle(const float percent)
{
	if (m_handles.size() >= m_max)
		return false;

	HandleData data;
	data.percentage = percent;
	data.flags = 0;
	m_handles.push_back(data);

	SortHandles();
	return true;
}

void CRampControl::SortHandles()
{
	std::sort(m_handles.begin(),m_handles.end(),SortHandle);
}

bool CRampControl::SortHandle(const CRampControl::HandleData& dataA, const CRampControl::HandleData& dataB)
{
	return dataA.percentage < dataB.percentage;
}

void CRampControl::ClearSelectedHandles()
{
	const int handleCount = m_handles.size();
	for (int idx = 0; idx < handleCount; ++idx)
	{
		m_handles[idx].flags &= ~hfSelected;
	}
}

void CRampControl::Reset()
{
	m_handles.clear();
}

bool CRampControl::SelectHandle(const float percent)
{
	const int handleCount = m_handles.size();
	for (int idx = 0; idx < handleCount; ++idx)
	{
		if ( HitTestHandle(m_handles[idx],percent) && !(m_handles[idx].flags & hfSelected) )
		{
			m_handles[idx].flags |= hfSelected;
			return true;
		}
	}
	return false;
}

bool CRampControl::DeSelectHandle(const float percent)
{
	const int handleCount = m_handles.size();
	for (int idx = 0; idx < handleCount; ++idx)
	{
		if ( HitTestHandle(m_handles[idx],percent) && (m_handles[idx].flags & hfSelected))
		{
			m_handles[idx].flags &= ~hfSelected;
			return true;
		}
	}
	return false;
}

bool CRampControl::IsSelectedHandle(const float percent)
{
	const int handleCount = m_handles.size();
	for (int idx = 0; idx < handleCount; ++idx)
	{
		if ( HitTestHandle(m_handles[idx],percent) )
			if ( m_handles[idx].flags & hfSelected )
				return true;
	}
	return false;
}

void CRampControl::SelectAllHandles()
{
	const int handleCount = m_handles.size();
	for (int idx = 0; idx < handleCount; ++idx)
	{
		m_handles[idx].flags |= hfSelected;
	}
}

float CRampControl::MoveHandle(HandleData& data, const float percent)
{
	float clampedby = 0.0f;
	data.percentage += percent;
	if ( data.percentage < 0.0f )
	{
		clampedby = -data.percentage;
		data.percentage = 0.0f;
	}

	if ( data.percentage > 99.99f )
	{
		clampedby = -(data.percentage - 99.0f);
		data.percentage = 99.0f;
	}
	return clampedby;
}

bool CRampControl::HitAnyHandles(const float percent)
{
	const int handleCount = m_handles.size();
	for (int idx = 0; idx < handleCount; ++idx)
	{
		if ( HitTestHandle(m_handles[idx],percent) )
			return true;
	}
	return false;
}

bool CRampControl::HitTestHandle(const HandleData& data, const float percent)
{
	if ( percent > data.percentage-1 && percent < data.percentage+1 )
		return true;
	return false;
}

bool CRampControl::CheckSelected(const CRampControl::HandleData& data)
{
	return data.flags & CRampControl::hfSelected;
}

void CRampControl::RemoveSelectedHandles()
{
	m_handles.erase(remove_if(m_handles.begin(),m_handles.end(),CRampControl::CheckSelected),m_handles.end());
	SortHandles();
}

void CRampControl::SelectNextHandle()
{
	bool selected = false;
	const int count = m_handles.size();
	for ( int idx = count-1; idx >= 0; --idx )
	{
		if ( (m_handles[idx].flags & hfSelected) && ( idx+1 < count ) )
		{
			m_handles[idx+1].flags |= hfSelected;
			selected = true;
		}
		m_handles[idx].flags &= ~hfSelected;
	}

	if ( selected == false && count > 0 )
	{
		m_handles[0].flags |= hfSelected;
	}
}

void CRampControl::SelectPreviousHandle()
{
	bool selected = false;
	const int count = m_handles.size();
	for ( int idx = 0; idx < count; ++idx )
	{
		if ( (m_handles[idx].flags & hfSelected) && ( idx-1 >= 0) )
		{
			m_handles[idx-1].flags |= hfSelected;
			selected = true;
		}
		m_handles[idx].flags &= ~hfSelected;
	}

	if ( selected == false && count > 0 )
	{
		m_handles[count-1].flags |= hfSelected;

	}
}

/*************************************************************************************/
const bool CRampControl::GetSelectedData(CRampControl::HandleData & data)
{
	const int handleCount = m_handles.size();
	for (int idx = 0; idx < handleCount; ++idx)
	{
		if ( m_handles[idx].flags & hfSelected )
		{
			data = m_handles[idx];
			return true;
		}
	}
	return false;
}

const int CRampControl::GetHandleCount()
{
	return m_handles.size();
}

const CRampControl::HandleData CRampControl::GetHandleData(const int idx)
{
	return m_handles[idx];
}

const int CRampControl::GetHotHandleIndex()
{
	const int count = m_handles.size();
	for ( int idx = 0; idx < count; ++idx )
	{
		if ( m_handles[idx].flags & hfHotitem )
		{
			return idx;
		}
	}
	return -1;
}

void CRampControl::SetHotItem(const float percentage)
{
	const int count = m_handles.size();
	for ( int idx = 0; idx < count; ++idx )
	{
		if ( HitTestHandle(m_handles[idx],percentage) )
		{
			m_handles[idx].flags |= hfHotitem;
		}
		else
		{
			m_handles[idx].flags &= ~hfHotitem;
		}
	}
}
