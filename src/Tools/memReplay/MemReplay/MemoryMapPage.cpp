// MemoryMapPage.cpp : implementation file
//

#include "stdafx.h"
#include "MemReplay.h"
#include "MemoryMapPage.h"
#include "MemReplayDoc.h"
#include "MainFrm.h"
#include "MemReplayView.h"

// CMemoryMapPage dialog

IMPLEMENT_DYNAMIC(CMemoryMapPage, CDialog)

CMemoryMapPage::CMemoryMapPage(const FrameUsageTracker& memUsage, const MemoryMapReplay& memMap)
	: CDialog(CMemoryMapPage::IDD)
	, m_memMap(&memMap)
	, m_timelineDataSource(std::distance(memUsage.FrameInfoBegin(MemGroups::Main), memUsage.FrameInfoEnd(MemGroups::Main)))
{
	m_memoryMap.SetMemoryMap(m_memMap);
	m_timeline.SetStreamSource(&m_timelineDataSource);
}

CMemoryMapPage::~CMemoryMapPage()
{
}

void CMemoryMapPage::OnTabActivate()
{
	theApp.GetMainFrame()->SetActiveToolBar(CMainFrame::TB_None);
}

void CMemoryMapPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMemoryMapPage, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(CMemoryGraphWnd::MGW_RANGECHANGED, ID_TIMELINE, OnTimelineSelectionChanged)
END_MESSAGE_MAP()


// CMemoryMapPage message handlers

BOOL CMemoryMapPage::OnInitDialog()
{
	BOOL result = CDialog::OnInitDialog();

	m_timeline.Create("", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_NOTIFY, CRect(0, 0, 16, 16), this, ID_TIMELINE);
	m_memoryMap.Create("", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_NOTIFY, CRect(0, 0, 16, 16), this, ID_MEMORYMAP);

	SetWindowText(_T("Memory Map"));

	return result;
}

void CMemoryMapPage::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (GetSafeHwnd())
	{
		CRect rcClient;
		GetClientRect(&rcClient);

		CRect rcTimeline = rcClient;
		rcTimeline.bottom = 80;

		m_timeline.MoveWindow(rcTimeline);

		CRect rcMemoryMap = rcClient;
		rcMemoryMap.top = rcTimeline.bottom;

		m_memoryMap.MoveWindow(rcMemoryMap);
	}
}

void CMemoryMapPage::OnTimelineSelectionChanged(NMHDR*, LRESULT*)
{
	int frame = (int) m_timeline.GetEndCursorPosition();
	m_memoryMap.SetFrameMarker(frame);
	m_memoryMap.Invalidate();
}
