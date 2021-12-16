////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEventLogView.cpp
//  Version:     v1.00
//  Created:     28/03/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UIEventLogView.h"

#include "Util/CryMemFile.h"

IMPLEMENT_DYNCREATE( CUIEventLogView, CXTResizeDialog )

#define BITMAP_NONE			-1
#define BITMAP_ERROR     0
#define BITMAP_WARNING   1
#define BITMAP_INFO      2

#define COLUMN_ID          0
#define COLUMN_EVTID       1
#define COLUMN_MESSAGE     2



#define ID_LIST_CONTROL 100

BEGIN_MESSAGE_MAP( CUIEventLogView, CXTResizeDialog )
	ON_WM_SIZE()
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CUILogEventEntry : public CXTPReportRecord  
{
	DECLARE_DYNAMIC(CUILogEventEntry)
public:
	CXTPReportRecordItem *m_pIconItem;

	CUILogEventEntry( const IFlashUI::SUILogEvent &event, int id )
	{
		CString msg = event.Message.c_str();
		CString evtid;
		evtid.Format("%i", id);

		int nIcon = -1;
		switch (event.Level)
		{
		default:
		case IFlashUI::eLEL_Log: nIcon = BITMAP_NONE; break;
		case IFlashUI::eLEL_Warning: nIcon = BITMAP_WARNING; break;
		case IFlashUI::eLEL_Error: nIcon = BITMAP_ERROR; break;
		}

		m_pIconItem = AddItem(new CXTPReportRecordItem());
		AddItem(new CXTPReportRecordItemText(evtid));
		AddItem(new CXTPReportRecordItemText(msg));

		m_pIconItem->SetIconIndex(nIcon);
		m_pIconItem->SetGroupPriority(nIcon);
		m_pIconItem->SetSortPriority(nIcon);
	}
};
IMPLEMENT_DYNAMIC(CUILogEventEntry, CXTPReportRecord)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CUIEventLogView::CUIEventLogView( CWnd* pParent /*= NULL*/ )
// : CXTResizeDialog(CUIEventLogView::IDD, pParent)
{
}

//////////////////////////////////////////////////////////////////////////
CUIEventLogView::~CUIEventLogView()
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
BOOL CUIEventLogView::OnInitDialog()
{
	BOOL baseInitSuccess = __super::OnInitDialog();
	if ( !baseInitSuccess )
	{
		return FALSE;
	}

	VERIFY( m_wndList.Create(WS_CHILD|WS_TABSTOP|WS_VISIBLE|WM_VSCROLL, CRect(0, 0, 0, 0), this, ID_LIST_CONTROL) );

	CMFCUtils::LoadTrueColorImageList( m_imageList,IDB_ERROR_REPORT,16,RGB(255,255,255) );

	m_wndList.SetImageList(&m_imageList); 

 	m_wndList.AddColumn(new CXTPReportColumn(COLUMN_ID, _T(""), 18, FALSE ));
 	m_wndList.AddColumn(new CXTPReportColumn(COLUMN_EVTID, _T("Evt"), 20, TRUE ));
	m_wndList.AddColumn(new CXTPReportColumn(COLUMN_MESSAGE, _T("Message"), 60, TRUE ));

	m_wndList.GetPaintManager()->m_clrHyper = ::GetSysColor(COLOR_HIGHLIGHT);

	m_wndList.RedrawWindow();
	RedrawWindow();

	CRect rc;
	GetClientRect(rc);
	m_wndList.MoveWindow(rc);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CUIEventLogView::PostNcDestroy()
{
	__super::PostNcDestroy();
}

//////////////////////////////////////////////////////////////////////////
void CUIEventLogView::OnSize( UINT nType,int cx,int cy )
{
	__super::OnSize(nType,cx,cy);

	if (m_wndList)
	{
		CRect rc;
		GetClientRect(rc);
		m_wndList.MoveWindow(rc);
	}
}

//////////////////////////////////////////////////////////////////////////
void CUIEventLogView::OnSysCommand(UINT nID, LPARAM lParam)
{
	if (nID == SC_CLOSE)
	{
		DestroyWindow();
	}
	else
	{
		__super::OnSysCommand(nID, lParam);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CUIEventLogView::ClearLog()
{
	m_LogEvents.clear();
	ReloadLogEvents();
}

//////////////////////////////////////////////////////////////////////////
void CUIEventLogView::AddLogEvent(const IFlashUI::SUILogEvent& event)
{
	m_LogEvents.push_back(event);

	m_wndList.BeginUpdate();

	m_wndList.AddRecord( new CUILogEventEntry(event, m_LogEvents.size()-1) );

	m_wndList.EndUpdate();
	m_wndList.Populate();

	m_wndList.RedrawControl();

	RedrawWindow();
	CRect rc;
	GetClientRect(rc);
	m_wndList.MoveWindow(rc);

	::SendMessage(m_wndList.GetSafeHwnd(),WM_VSCROLL, SB_BOTTOM, (LPARAM)NULL);
}

//////////////////////////////////////////////////////////////////////////
void CUIEventLogView::ReloadLogEvents()
{
	m_wndList.ResetContent();
	m_wndList.BeginUpdate();

	for (int i = 0; i < m_LogEvents.size(); i++)
	{
		const IFlashUI::SUILogEvent &event = m_LogEvents[i];
		m_wndList.AddRecord( new CUILogEventEntry(event, i) );
	}

	m_wndList.EndUpdate();
	m_wndList.Populate();

	RedrawWindow();
	CRect rc;
	GetClientRect(rc);
	m_wndList.MoveWindow(rc);

	::SendMessage(m_wndList.GetSafeHwnd(),WM_VSCROLL, SB_BOTTOM, (LPARAM)NULL);
}

