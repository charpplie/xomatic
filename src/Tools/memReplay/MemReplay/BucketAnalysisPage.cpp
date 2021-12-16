#include "stdafx.h"
#include "MemReplay.h"
#include "BucketAnalysisPage.h"

#include "MemReplayView.h"
#include "MainFrm.h"

#include <functional>

namespace
{
	struct SelectRequested
	{
		int operator() (const BucketUsageResult::UsageInfo& p) const { return p.requested; }
	};

	struct SelectConsumed
	{
		int operator() (const BucketUsageResult::UsageInfo& p) const { return p.consumed; }
	};

	struct SelectCapacity
	{
		int operator() (const BucketUsageResult::UsageInfo& p) const { return p.capacity; }
	};

	struct SelectWaste
	{
		int operator () (const BucketUsageResult::UsageInfo& p) const { return p.capacity - p.consumed; }
	};

	inline float fracf(float f)
	{
		return f - floorf(f);
	}

	COLORREF HSVtoRGB(float hue, float sat, float val)
	{
		hue *= 6.0f;
		int seg = (int) hue;
		float chroma = val * sat;
		float hueFrac = fracf(hue);
		float x = chroma * (1.0f - std::abs(hueFrac - 1.0f));

		chroma *= 255.0f;
		x *= 255.0f;
		float m = 255.0f * val - chroma;

		chroma = std::min(255.0f, chroma + m);
		x = std::min(255.0f, x + m);

		switch (seg)
		{
		case 0: return RGB(chroma, x, m);
		case 1: return RGB(x, chroma, m);
		case 2: return RGB(m, chroma, x);
		case 3: return RGB(m, x, chroma);
		case 4: return RGB(x, m, chroma);
		case 5: return RGB(chroma, m, x);
		default: return RGB(m, m, m);
		}
	}

	template <typename IteratorAT, typename IteratorBT, typename Func>
	void Update(IteratorAT a, IteratorAT b, IteratorBT c, Func func)
	{
		for (; a != b; ++ a, ++ c)
			func(*a, *c);
	}

	struct UpdateAdd
	{
		template <typename T>
		void operator () (T& a, T b) const { a += b; }
	};

	size_t SizeOfBucketStream(const BucketUsageResult::UsageStream& stream)
	{
		return (stream.size() - 1) * 16384 + stream.back().size();
	}
}

IMPLEMENT_DYNAMIC(CBucketAnalysisPage, CDialog)

CBucketAnalysisPage::CBucketAnalysisPage(IReplayDatabaseConnection* db, const SharedPtr<BucketUsageResult>& usage)
	: m_db(db)
	, m_usage(usage)
	, m_wasteStreamSource(SizeOfBucketStream(usage->buckets[7*8]))
	, m_sizeStreamSource(SizeOfBucketStream(usage->buckets[7*8]))
	, m_capacityStreamSource(SizeOfBucketStream(usage->buckets[7*8]))
{
	m_memGraph.SetStreamSource(&m_sizeStreamSource);

	char name[256];

	std::vector<int> totalSize, totalCapacity, totalWaste;
	totalSize.resize(SizeOfBucketStream(usage->buckets[7*8]), int());
	totalCapacity.resize(SizeOfBucketStream(usage->buckets[7*8]), int());
	totalWaste.resize(SizeOfBucketStream(usage->buckets[7*8]), int());

	for (BucketUsageResult::BucketMap::const_iterator it = usage->buckets.begin(), itEnd = usage->buckets.end();
		it != itEnd;
		++ it)
	{
		sprintf_s(name, 256, "Bucket %i", it->first);

		std::vector<int> waste, size, capacity;

		const BucketUsageResult::UsageStream& stream = it->second;
		waste.reserve(stream.size() * 16384);
		size.reserve(stream.size() * 16384);
		capacity.reserve(stream.size() * 16384);
		for (BucketUsageResult::UsageStream::const_iterator sit = stream.begin(), sitEnd = stream.end(); sit != sitEnd; ++ sit)
			std::transform(sit->begin(), sit->end(), std::back_inserter(waste), SelectWaste());
		for (BucketUsageResult::UsageStream::const_iterator sit = stream.begin(), sitEnd = stream.end(); sit != sitEnd; ++ sit)
			std::transform(sit->begin(), sit->end(), std::back_inserter(size), SelectConsumed());
		for (BucketUsageResult::UsageStream::const_iterator sit = stream.begin(), sitEnd = stream.end(); sit != sitEnd; ++ sit)
			std::transform(sit->begin(), sit->end(), std::back_inserter(capacity), SelectCapacity());

		Update(totalWaste.begin(), totalWaste.end(), waste.begin(), UpdateAdd());
		Update(totalSize.begin(), totalSize.end(), size.begin(), UpdateAdd());
		Update(totalCapacity.begin(), totalCapacity.end(), capacity.begin(), UpdateAdd());

		m_buckets.push_back(it->first);

		float hue = it->first / 512.0f;
		AddPlot(name, HSVtoRGB(hue, 1.0f, 1.0f), waste, m_wasteStreamSource);
		AddPlot(name, HSVtoRGB(hue, 1.0f, 1.0f), size, m_sizeStreamSource);
		AddPlot(name, HSVtoRGB(hue, 1.0f, 1.0f), capacity, m_capacityStreamSource);
	}

	AddPlot("Total", RGB(255, 255, 255), totalWaste, m_wasteStreamSource);
	AddPlot("Total", RGB(255, 255, 255), totalSize, m_sizeStreamSource);
	AddPlot("Total", RGB(255, 255, 255), totalCapacity, m_capacityStreamSource);
}

void CBucketAnalysisPage::OnTabActivate()
{
	theApp.GetMainFrame()->SetActiveToolBar(CMainFrame::TB_Bucket);
}

void CBucketAnalysisPage::OnInspectClicked()
{
	CMemReplayView* view = static_cast<CMemReplayView*>(((CMainFrame*)theApp.GetMainWnd())->GetActiveView());

	if (m_memGraph.HasSelection())
	{
		s64 begin = (s64) m_memGraph.GetStartCursorPosition();
		s64 end = (s64) m_memGraph.GetEndCursorPosition();

		std::vector<int> buckets;

		for (int i = 0, c = m_bucketList.GetItemCount(); i != c; ++ i)
		{
			int stream = static_cast<int>(m_bucketList.GetItemData(i));
			if (m_wasteStreamSource.IsVisible(stream) && (i < (int) m_buckets.size()))
				buckets.push_back(m_buckets[i]);
		}
		
		if (!buckets.empty())
			view->OpenCodeBucketViewForAllocEvRange(begin, end, buckets);
	}
}

void CBucketAnalysisPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CBucketAnalysisPage, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(CMemoryGraphWnd::MGW_HOVERCHANGED, ID_MEMGRAPH, &CBucketAnalysisPage::OnMemGraphHoverChanged)
	ON_NOTIFY(LVN_ITEMCHANGED, ID_BUCKETLIST, &CBucketAnalysisPage::OnBucketListItemChanged)
	ON_NOTIFY(LVN_COLUMNCLICK, ID_BUCKETLIST, &CBucketAnalysisPage::OnBucketListHeaderClicked)
END_MESSAGE_MAP()


// CBucketAnalysisPage message handlers

BOOL CBucketAnalysisPage::OnInitDialog()
{
	BOOL result = CDialog::OnInitDialog();

	m_memGraph.Create("", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_NOTIFY, CRect(0, 0, 16, 16), this, ID_MEMGRAPH);
	m_bucketList.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, CRect(0, 0, 16, 16), this, ID_BUCKETLIST);
	m_bucketList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);

	LVCOLUMN col = {0};
	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	col.fmt = LVCFMT_CENTER;
	col.cx = 150;
	col.pszText = "Bucket";
	m_bucketList.InsertColumn(0, &col);

	col.pszText = "Size";
	m_bucketList.InsertColumn(1, &col);

	col.pszText = "Capacity";
	m_bucketList.InsertColumn(2, &col);

	col.pszText = "Wasted In Alignment";
	m_bucketList.InsertColumn(3, &col);

	col.pszText = "Wasted In Lists";
	m_bucketList.InsertColumn(4, &col);

	col.pszText = "Count";
	m_bucketList.InsertColumn(5, &col);

	for (int si = 0, sc = m_wasteStreamSource.GetStreamCount(); si != sc; ++ si)
	{
		IMemoryGraphStreamSource::StreamInfo info = m_wasteStreamSource.GetStreamInfo(si);
		int item = m_bucketList.InsertItem(m_bucketList.GetItemCount(), info.name.c_str());
		m_bucketList.SetItem(item, 1, LVIF_TEXT, "", 0, 0, 0, 0, 0);
		m_bucketList.SetItem(item, 2, LVIF_TEXT, "", 0, 0, 0, 0, 0);
		m_bucketList.SetItem(item, 3, LVIF_TEXT, "", 0, 0, 0, 0, 0);
		m_bucketList.SetItem(item, 4, LVIF_TEXT, "", 0, 0, 0, 0, 0);
		m_bucketList.SetItemData(item, static_cast<DWORD_PTR>(si));
		m_bucketList.SetCheck(item, TRUE);
	}

	SetWindowText(_T("Bucket Analysis"));

	return result;
}

void CBucketAnalysisPage::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (GetSafeHwnd())
	{
		CRect rcClient;
		GetClientRect(&rcClient);

		int splitVert = rcClient.Height() / 2 + rcClient.top;

		CRect rcMemGraph(rcClient.left, rcClient.top, rcClient.right, splitVert);
		CRect rcBucketList(rcClient.left, splitVert, rcClient.right, rcClient.bottom);

		m_memGraph.MoveWindow(rcMemGraph);
		m_bucketList.MoveWindow(rcBucketList);
	}
}

void CBucketAnalysisPage::OnMemGraphHoverChanged(NMHDR* nmHdr, LRESULT* result)
{
	double cursor = m_memGraph.GetHoverCursorPosition();
	int frameIdx = (int) cursor;

	u64 allocEv = frameIdx * m_usage->allocEvSplit;

	size_t totalSize = 0, totalCapacity = 0, totalWastedAlign = 0, totalWastedList = 0, totalCount = 0;

	for (int plot = 0, plotCount = m_bucketList.GetItemCount(); plot != plotCount; ++ plot)
	{
		size_t streamIdx = static_cast<size_t>(m_bucketList.GetItemData(plot));
		if (streamIdx >= m_buckets.size())
			continue;

		int bucket = m_buckets[streamIdx];
		BucketUsageResult::UsageStream& stream = m_usage->buckets[bucket];

		int pageIdx = frameIdx / 16384;
		std::vector<BucketUsageResult::UsageInfo>& page = stream[pageIdx];

		int offset = frameIdx % 16384;
		const BucketUsageResult::UsageInfo& info = page[offset];

		TCHAR sizeStr[64], capacityStr[64], wastedAlignStr[64], wastedListStr[64], count[64];
		FormatSize(sizeStr, 64, info.consumed);
		FormatSize(capacityStr, 64, info.capacity);
		FormatSize(wastedAlignStr, 64, info.consumed - info.requested);
		FormatSize(wastedListStr, 64, info.capacity - info.consumed);
		TCHAR* countFmt = FormatThousands(count, 64, info.count);

		m_bucketList.SetItem(plot, 1, TVIF_TEXT, sizeStr, 0, 0, 0, 0, 0);
		m_bucketList.SetItem(plot, 2, TVIF_TEXT, capacityStr, 0, 0, 0, 0, 0);
		m_bucketList.SetItem(plot, 3, TVIF_TEXT, wastedAlignStr, 0, 0, 0, 0, 0);
		m_bucketList.SetItem(plot, 4, TVIF_TEXT, wastedListStr, 0, 0, 0, 0, 0);
		m_bucketList.SetItem(plot, 5, TVIF_TEXT, countFmt, 0, 0, 0, 0, 0);

		totalSize += info.consumed;
		totalCapacity += info.capacity;
		totalWastedAlign += (info.consumed - info.requested);
		totalWastedList	 += (info.capacity - info.consumed);
		totalCount += info.count;
	}

	{
		TCHAR sizeStr[64], capacityStr[64], wastedAlignStr[64], wastedListStr[64], count[64];
		FormatSize(sizeStr, 64, totalSize);
		FormatSize(capacityStr, 64, totalCapacity);
		FormatSize(wastedAlignStr, 64, totalWastedAlign);
		FormatSize(wastedListStr, 64, totalWastedList);
		TCHAR* countFmt = FormatThousands(count, 64, totalCount);

		int plot = m_bucketList.GetItemCount() - 1;
		m_bucketList.SetItem(plot, 1, TVIF_TEXT, sizeStr, 0, 0, 0, 0, 0);
		m_bucketList.SetItem(plot, 2, TVIF_TEXT, capacityStr, 0, 0, 0, 0, 0);
		m_bucketList.SetItem(plot, 3, TVIF_TEXT, wastedAlignStr, 0, 0, 0, 0, 0);
		m_bucketList.SetItem(plot, 4, TVIF_TEXT, wastedListStr, 0, 0, 0, 0, 0);
		m_bucketList.SetItem(plot, 5, TVIF_TEXT, countFmt, 0, 0, 0, 0, 0);
	}

	TCHAR allocEvStr[64];
	TCHAR status[256];
	_stprintf_s(status, 256, "Alloc event id %s", FormatThousands(allocEvStr, 64, allocEv));
	static_cast<CMainFrame*>(theApp.GetMainWnd())->SetStatus(status);

	(*result) = TRUE;
}

void CBucketAnalysisPage::OnBucketListItemChanged(NMHDR* nmHdr, LRESULT* result)
{
	NMLISTVIEW* nm = reinterpret_cast<NMLISTVIEW*>(nmHdr);

	int si = nm->iItem;

	bool check = m_bucketList.GetCheck(si) == TRUE;
	if (m_wasteStreamSource.IsVisible(si) != check)
	{
		m_wasteStreamSource.SetVisible(si, check ? TRUE : FALSE);
		m_sizeStreamSource.SetVisible(si, check ? TRUE : FALSE);
		m_capacityStreamSource.SetVisible(si, check ? TRUE : FALSE);
		m_memGraph.Invalidate();
	}
}

void CBucketAnalysisPage::OnBucketListHeaderClicked(NMHDR* nmHdr, LRESULT* result)
{
	NMLISTVIEW* hdr = reinterpret_cast<NMLISTVIEW*>(nmHdr);
	switch (hdr->iSubItem)
	{
	case 0:
		break;

	case 1: // Size
		m_memGraph.SetStreamSource(&m_sizeStreamSource);
		break;

	case 2: // Capacity
		m_memGraph.SetStreamSource(&m_capacityStreamSource);
		break;

	case 3: // Waste in alignment
		break;

	case 4: // Waste in lists
		m_memGraph.SetStreamSource(&m_wasteStreamSource);
		break;

	case 5: // Count
		break;
	}

	m_memGraph.Invalidate();
}

void CBucketAnalysisPage::AddPlot(const char* name, COLORREF colour, std::vector<int>& values, GenericMemoryGraphStreamSource& source)
{
	source.AddStream(IMemoryGraphStreamSource::StreamInfo(colour, name), values);

	int item = m_bucketList.InsertItem(m_bucketList.GetItemCount(), name);
	m_bucketList.SetCheck(item, TRUE);
	m_bucketList.SetItemData(item, static_cast<DWORD_PTR>(m_bucketList.GetItemCount() - 1));
}
