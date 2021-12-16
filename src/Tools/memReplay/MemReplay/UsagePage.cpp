// UsagePage.cpp : implementation file
//

#include "stdafx.h"
#include "MemReplay.h"
#include "UsagePage.h"
#include "MemReplayDoc.h"
#include "MainFrm.h"
#include "MemReplayView.h"

namespace
{
	struct SelectTotal
	{
		int operator () (const SizeInfoGroups& sz) const { return sz.GetTotal().consumed; }
	};

	struct SelectMain
	{
		int operator () (const SizeInfoGroups& sz) const { return sz.GetGroup(MemGroups::Main).consumed; }
	};

	struct SelectRSX
	{
		int operator () (const SizeInfoGroups& sz) const { return sz.GetGroup(MemGroups::RSX).consumed; }
	};

	struct SelectRequested
	{
		int operator () (const SizeInfo& sz) const { return sz.requested; }
	};

	struct SelectConsumed
	{
		int operator () (const SizeInfo& sz) const { return sz.consumed; }
	};

	struct SelectGlobal
	{
		int operator () (const SizeInfo& sz) const { return sz.global; }
	};

	struct SelectFragmentation
	{
		int operator () (const SizeInfo& sz) const { return sz.global - sz.consumed; }
	};

	template <typename SelectT, typename IteratorT>
	void AddSelectStream(GenericMemoryGraphStreamSource& graph, IteratorT begin, IteratorT end, SelectT select, const IMemoryGraphStreamSource::StreamInfo& info)
	{
		std::vector<int> streamValues;
		streamValues.reserve(std::distance(begin, end));
		std::transform(begin, end, std::back_inserter(streamValues), select);

		graph.AddStream(info, streamValues);
	}

	struct UsageStatusDistancePredicate
	{
		UsageStatusDistancePredicate(int reference)
			: m_reference(reference) {}

		bool operator () (const std::pair<int, int>& a, const std::pair<int, int>& b) const
		{
			return std::abs(a.first - m_reference) < std::abs(b.first - m_reference);
		}

		int m_reference;
	};
}

// CUsagePage dialog

IMPLEMENT_DYNAMIC(CUsagePage, CDialog)

CUsagePage::CUsagePage(IReplayDatabaseConnection* db)
	: CDialog(CUsagePage::IDD)
{
	if (db)
	{
		m_tracker = db->GetFrameUsageTracker();

		if (m_tracker.IsValid())
		{
			m_allocUsageMarkerSource = AllocUsageMemoryGraphMarkerSource(*m_tracker);
			m_allocUsageStreamSource = GenericMemoryGraphStreamSource(std::distance(m_tracker->AllocEvInfoBegin(MemGroups::Main), m_tracker->AllocEvInfoEnd(MemGroups::Main)));
			m_frameUsageMemSource = GenericMemoryGraphStreamSource(std::distance(m_tracker->FrameInfoBegin(MemGroups::Main), m_tracker->FrameInfoEnd(MemGroups::Main)));
			m_frameUsageMarkerSource = FrameUsageMemoryGraphMarkerSource(*m_tracker);

			AddSelectStream(m_frameUsageMemSource, m_tracker->FrameInfoBegin(MemGroups::Main), m_tracker->FrameInfoEnd(MemGroups::Main), SelectRequested(),
				IMemoryGraphStreamSource::StreamInfo(RGB(255,0,0), "Main Requested"));
			AddSelectStream(m_frameUsageMemSource, m_tracker->FrameInfoBegin(MemGroups::Main), m_tracker->FrameInfoEnd(MemGroups::Main), SelectConsumed(),
				IMemoryGraphStreamSource::StreamInfo(RGB(255,255,0), "Main Consumed"));
			AddSelectStream(m_frameUsageMemSource, m_tracker->FrameInfoBegin(MemGroups::Main), m_tracker->FrameInfoEnd(MemGroups::Main), SelectGlobal(), 
				IMemoryGraphStreamSource::StreamInfo(RGB(0,0,255), "Main OS"));
			AddSelectStream(m_frameUsageMemSource, m_tracker->FrameInfoBegin(MemGroups::RSX), m_tracker->FrameInfoEnd(MemGroups::RSX), SelectRequested(),
				IMemoryGraphStreamSource::StreamInfo(RGB(0,255,255), "RSX Requested"));
			AddSelectStream(m_frameUsageMemSource, m_tracker->FrameInfoBegin(MemGroups::Main), m_tracker->FrameInfoEnd(MemGroups::Main), SelectFragmentation(),
				IMemoryGraphStreamSource::StreamInfo(RGB(255,0,255), "Fragmentation"));

			AddSelectStream(m_allocUsageStreamSource, m_tracker->AllocEvInfoBegin(MemGroups::Main), m_tracker->AllocEvInfoEnd(MemGroups::Main), SelectRequested(),
				IMemoryGraphStreamSource::StreamInfo(RGB(255,0,0), "Main Requested"));
			AddSelectStream(m_allocUsageStreamSource, m_tracker->AllocEvInfoBegin(MemGroups::Main), m_tracker->AllocEvInfoEnd(MemGroups::Main), SelectConsumed(),
				IMemoryGraphStreamSource::StreamInfo(RGB(255,255,0), "Main Consumed"));
			AddSelectStream(m_allocUsageStreamSource, m_tracker->AllocEvInfoBegin(MemGroups::Main), m_tracker->AllocEvInfoEnd(MemGroups::Main), SelectGlobal(), 
				IMemoryGraphStreamSource::StreamInfo(RGB(0,0,255), "Main OS"));
			AddSelectStream(m_allocUsageStreamSource, m_tracker->AllocEvInfoBegin(MemGroups::RSX), m_tracker->AllocEvInfoEnd(MemGroups::RSX), SelectRequested(),
				IMemoryGraphStreamSource::StreamInfo(RGB(0,255,255), "RSX Requested"));
			AddSelectStream(m_allocUsageStreamSource, m_tracker->AllocEvInfoBegin(MemGroups::Main), m_tracker->AllocEvInfoEnd(MemGroups::Main), SelectFragmentation(),
				IMemoryGraphStreamSource::StreamInfo(RGB(255,0,255), "Fragmentation"));

			m_memGraph.SetStreamSource(&m_allocUsageStreamSource);
			m_memGraph.SetMarkerSource(&m_allocUsageMarkerSource);
			m_frameUsageMemGraph.SetStreamSource(&m_frameUsageMemSource);
			m_frameUsageMemGraph.SetMarkerSource(&m_frameUsageMarkerSource);
		}
	}
}

CUsagePage::~CUsagePage()
{
}

void CUsagePage::AddPlot(const char* name, const SharedPtr<SizeInfoUsageResult>& values, int memoryType)
{
	std::vector<int> frameValues, allocEvValues;
	frameValues.reserve(values->byFrame.size());
	allocEvValues.reserve(values->byAllocEv.size());

	COLORREF colour;

	switch (memoryType)
	{
	case MEMTYPE_MAIN:
		std::transform(values->byFrame.begin(), values->byFrame.end(), std::back_inserter(frameValues), SelectMain());
		std::transform(values->byAllocEv.begin(), values->byAllocEv.end(), std::back_inserter(allocEvValues), SelectMain());
		colour = RGB(255, 0, 0);
		break;

	case MEMTYPE_RSX:
		std::transform(values->byFrame.begin(), values->byFrame.end(), std::back_inserter(frameValues), SelectRSX());
		std::transform(values->byAllocEv.begin(), values->byAllocEv.end(), std::back_inserter(allocEvValues), SelectRSX());
		colour = RGB(0, 255, 255);
		break;

	case MEMTYPE_COMBINED:
	default:
		std::transform(values->byFrame.begin(), values->byFrame.end(), std::back_inserter(frameValues), SelectTotal());
		std::transform(values->byAllocEv.begin(), values->byAllocEv.end(), std::back_inserter(allocEvValues), SelectTotal());
		colour = RGB(0, 255, 0);
		break;
	}

	m_frameUsageMemSource.AddStream(IMemoryGraphStreamSource::StreamInfo(colour, name), frameValues);
	m_frameUsageMemGraph.Invalidate();

	m_allocUsageStreamSource.AddStream(IMemoryGraphStreamSource::StreamInfo(colour, name), allocEvValues);
	m_memGraph.Invalidate();

	HTREEITEM item = m_streamTree.InsertItem(name);
	m_streamTree.SetCheck(item, TRUE);
	m_streamTree.SetItemData(item, static_cast<DWORD_PTR>(m_streamTree.GetCount() - 1));
}

void CUsagePage::OnTabActivate()
{
	theApp.GetMainFrame()->SetActiveToolBar(CMainFrame::TB_Usage);
}

afx_msg void CUsagePage::OnInspectClicked()
{
	CMemReplayView* view = static_cast<CMemReplayView*>(((CMainFrame*)theApp.GetMainWnd())->GetActiveView());

	if (view)
	{
		if (m_memGraph.HasSelection())
		{
			s64 begin = (s64) m_memGraph.GetStartCursorPosition();
			s64 end = (s64) m_memGraph.GetEndCursorPosition();

			view->OpenCodeViewForAllocEvRange(begin, end);
		}

		else if (m_frameUsageMemGraph.HasSelection())
		{
			int frameBeginIdx = (int) m_frameUsageMemGraph.GetStartCursorPosition();
			int frameEndIdx = (int) m_frameUsageMemGraph.GetEndCursorPosition();

			view->OpenCodeViewForFrames(frameBeginIdx, frameEndIdx);
		}
	}
}

void CUsagePage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CUsagePage, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(CMemoryGraphWnd::MGW_HOVERCHANGED, ID_MEMGRAPH, &CUsagePage::OnMemGraphHoverChanged)
	ON_NOTIFY(CMemoryGraphWnd::MGW_HOVERCHANGED, ID_FRAMEUSAGEMEMGRAPH, &CUsagePage::OnFrameMemGraphHoverChanged)
	ON_NOTIFY(CMemoryGraphWnd::MGW_RANGECHANGED, ID_MEMGRAPH, &CUsagePage::OnMemGraphRangeChanged)
	ON_NOTIFY(CMemoryGraphWnd::MGW_RANGECHANGED, ID_FRAMEUSAGEMEMGRAPH, &CUsagePage::OnFrameMemGraphRangeChanged)
	ON_NOTIFY(CCheckedTreeCtrl::CTC_CHECKCHANGED, CUsagePage::ID_STREAMTREE, &CUsagePage::OnStreamTreeCheckChanged)
END_MESSAGE_MAP()


// CUsagePage message handlers

BOOL CUsagePage::OnInitDialog()
{
	BOOL result = CDialog::OnInitDialog();

	m_memGraph.Create("", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_NOTIFY, CRect(0, 0, 16, 16), this, ID_MEMGRAPH);
	m_frameUsageMemGraph.Create("", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_NOTIFY, CRect(0, 0, 16, 16), this, ID_FRAMEUSAGEMEMGRAPH);
	m_screenshotControl.Create("", WS_CHILD | WS_VISIBLE | SS_BITMAP, CRect(0, 0, 16, 16), this, ID_SCREENSHOT);
	m_streamTree.Create(
		WS_CHILD | WS_VISIBLE |
		TVS_NOHSCROLL | TVS_NOTOOLTIPS | TVS_CHECKBOXES | TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
		CRect(0, 0, 16, 16), this, ID_STREAMTREE);

	m_streamTree.ModifyStyle(TVS_CHECKBOXES, 0);
	m_streamTree.ModifyStyle(0, TVS_CHECKBOXES);

	for (int si = 0, sc = m_allocUsageStreamSource.GetStreamCount(); si != sc; ++ si)
	{
		IMemoryGraphStreamSource::StreamInfo info = m_allocUsageStreamSource.GetStreamInfo(si);
		HTREEITEM item = m_streamTree.InsertItem(info.name.c_str(), TVI_ROOT);
		m_streamTree.SetItemData(item, static_cast<DWORD_PTR>(si));
		m_streamTree.SetCheck(item, TRUE);
	}

	SetWindowText(_T("Usage"));

	return result;
}

void CUsagePage::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (GetSafeHwnd())
	{
		if (m_memGraph.GetSafeHwnd())
		{
			CRect rcClient;
			GetClientRect(&rcClient);

			int rightPaneWidth = std::min<int>(200, rcClient.Width() / 4);
			int bottomPaneHeight = rightPaneWidth * 9 / 16;

			CRect rcMemGraph(rcClient.left, rcClient.top, rcClient.right - rightPaneWidth, rcClient.top + rcClient.Height() / 2);
			CRect rcFrameMemGraph(rcClient.left, rcClient.top + rcClient.Height() / 2, rcClient.right - rightPaneWidth, rcClient.bottom);
			CRect rcStreamTree(rcClient.right - rightPaneWidth, rcClient.top, rcClient.right, rcClient.bottom - bottomPaneHeight);
			CRect rcScreenshot(rcClient.right - rightPaneWidth, rcClient.bottom - bottomPaneHeight, rcClient.right, rcClient.bottom);

			m_memGraph.MoveWindow(rcMemGraph);
			m_frameUsageMemGraph.MoveWindow(rcFrameMemGraph);
			m_streamTree.MoveWindow(rcStreamTree);
			m_screenshotControl.MoveWindow(rcScreenshot);
		}
	}
}

void CUsagePage::OnMemGraphHoverChanged(NMHDR* nmHdr, LRESULT* result)
{
	CMemoryGraphWnd::HoverChangedHeader* hdr = reinterpret_cast<CMemoryGraphWnd::HoverChangedHeader*>(nmHdr);

	const IMemoryGraphStreamSource* source = m_memGraph.GetStreamSource();
	if (!source)
		return;

	double cursor = hdr->cursorX;
	int cursorIdx = (int) cursor;

	TCHAR szStatus[512];
	FormatSizeForStatus(szStatus, sizeof(szStatus) / sizeof(szStatus[0]), *source, cursorIdx, (int) (hdr->cursorY * 1024.0 * 1024.0));

	char evId[64];

	u64 allocEv = cursorIdx * m_tracker->GetAllocEventSplit();

	TCHAR status[512];
	_stprintf_s(status, 512, "Alloc event id %s\t%s", FormatThousands(evId, 64, allocEv), szStatus);

	static_cast<CMainFrame*>(theApp.GetMainWnd())->SetStatus(status);

	const FrameScreenshot* ss = m_tracker->GetScreenshotForAllocEv(allocEv);
	if (ss)
		m_screenshotControl.FromBits(FrameScreenshot::Width, FrameScreenshot::Height, ss->bits);

	int frameIdx = m_tracker->FindFrameForAllocEv(allocEv);
	m_frameUsageMemGraph.SetHoverCursorPosition(static_cast<double>(frameIdx));

	(*result) = TRUE;
}

void CUsagePage::OnFrameMemGraphHoverChanged(NMHDR* nmHdr, LRESULT* result)
{
	const CMemoryGraphWnd::HoverChangedHeader* hdr = reinterpret_cast<const CMemoryGraphWnd::HoverChangedHeader*>(nmHdr);

	const IMemoryGraphStreamSource* source = m_frameUsageMemGraph.GetStreamSource();
	if (!source)
		return;

	double cursor = hdr->cursorX;
	int frameIdx = (int) cursor;

	TCHAR szStatus[512];
	FormatSizeForStatus(szStatus, 512, *source, frameIdx, (int) (hdr->cursorY * 1024.0 * 1024.0));

	TCHAR status[512];
	_stprintf_s(status, 512, "Frame %i\t%s", frameIdx, szStatus);

	static_cast<CMainFrame*>(theApp.GetMainWnd())->SetStatus(status);

	const FrameScreenshot* ss = m_tracker->GetScreenshotForFrame(frameIdx);
	if (ss)
		m_screenshotControl.FromBits(FrameScreenshot::Width, FrameScreenshot::Height, ss->bits);

	u64 allocEv = m_tracker->FindAllocEvForFrame(frameIdx);
	m_memGraph.SetHoverCursorPosition(allocEv / static_cast<double>(m_tracker->GetAllocEventSplit()));

	(*result) = TRUE;
}

void CUsagePage::OnMemGraphRangeChanged(NMHDR* nmHdr, LRESULT* result)
{
	m_frameUsageMemGraph.ClearSelection();

	(*result) = TRUE;
}

void CUsagePage::OnFrameMemGraphRangeChanged(NMHDR* nmHdr, LRESULT* result)
{
	m_memGraph.ClearSelection();

	(*result) = TRUE;
}

void CUsagePage::OnStreamTreeCheckChanged(NMHDR* nmHdr, LRESULT* result)
{
	CCheckedTreeCtrl::CheckChanged* nm = reinterpret_cast<CCheckedTreeCtrl::CheckChanged*>(nmHdr);
	int si = static_cast<int>(m_streamTree.GetItemData(nm->hItem));

	if (m_streamTree.GetCheck(nm->hItem))
	{
		m_allocUsageStreamSource.SetVisible(si, true);
		m_frameUsageMemSource.SetVisible(si, true);
	}
	else
	{
		m_allocUsageStreamSource.SetVisible(si, false);
		m_frameUsageMemSource.SetVisible(si, false);
	}

	m_memGraph.Invalidate();
	m_frameUsageMemGraph.Invalidate();
}

void CUsagePage::FormatSizeForStatus(TCHAR* szStatus, size_t stCapacity, const IMemoryGraphStreamSource& source, s64 sample, int val)
{
	TCHAR* szStatusEnd = szStatus;

	std::vector<std::pair<int, int> > streams;
	streams.reserve(source.GetStreamCount());

	for (int stream = 0, streamCount = source.GetStreamCount(); stream != streamCount; ++ stream)
	{
		int sampleVal = source.SampleAt(stream, sample, 0);
		streams.insert(
			std::lower_bound(streams.begin(), streams.end(), std::pair<int, int>(sampleVal, stream), UsageStatusDistancePredicate(val)),
			std::pair<int, int>(sampleVal, stream));
	}

	for (size_t idx = 0; idx < std::min(5U, streams.size()); ++ idx)
	{
		int sampleVal = streams[idx].first;
		int stream = streams[idx].second;

		char szStr[32];
		FormatSize(szStr, sizeof(szStr), sampleVal);

		IMemoryGraphStreamSource::StreamInfo info = source.GetStreamInfo(stream);
		int res = _stprintf_s(szStatusEnd, stCapacity - (szStatusEnd - szStatus), "%s (%hs)\t", info.name.c_str(), szStr);
		if (res < 0)
			break;

		if (res > 28)
		{
			_tcscpy(szStatusEnd + 14, szStatusEnd + res - 14);
			szStatusEnd[13] = '.';
			szStatusEnd[14] = '.';
			szStatusEnd[15] = '.';
			res = 28;
		}

		szStatusEnd += res;
	}
}
