#pragma once

#include "MemoryGraphWnd.h"
#include "MemoryGraphDataSources.h"
#include "BitmapWnd.h"
#include "CheckedTreeCtrl.h"

#include "ITabPage.h"

#include "ReplaySDK/IReplayDatabaseConnection.h"

class CUsagePage : public CDialog, public ITabPage
{
	DECLARE_DYNAMIC(CUsagePage)

public:
	// Dialog Data
	enum { IDD = IDD_USAGEPAGE };

public:
	explicit CUsagePage(IReplayDatabaseConnection* db);
	virtual ~CUsagePage();

	void AddPlot(const char* name, const SharedPtr<SizeInfoUsageResult>& values, int memoryType);

public: // ITabPage Members
	void OnTabActivate();

	void OnInspectClicked();
	void SetMemoryType(int type) {}
	int GetMemoryType() const { return MEMTYPE_COMBINED; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	BOOL OnInitDialog();
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual void OnCancel() {}

	DECLARE_MESSAGE_MAP()

private:
	enum
	{
		ID_MEMGRAPH = 1,
		ID_FRAMEMEMGRAPH,
		ID_FRAMEUSAGEMEMGRAPH,
		ID_SCREENSHOT,
		ID_STREAMTREE,
	};

private:
	void OnMemGraphHoverChanged(NMHDR* nmHdr, LRESULT* result);
	void OnFrameMemGraphHoverChanged(NMHDR* nmHdr, LRESULT* result);
	void OnMemGraphRangeChanged(NMHDR* nmHdr, LRESULT* result);
	void OnFrameMemGraphRangeChanged(NMHDR* nmHdr, LRESULT* result);
	void OnStreamTreeCheckChanged(NMHDR* nmHdr, LRESULT* result);

	void FormatSizeForStatus(TCHAR* st, size_t stCapacity, const IMemoryGraphStreamSource& source, s64 sample, int val);

private:
	SharedPtr<FrameUsageTracker> m_tracker;

	CMemoryGraphWnd m_memGraph;
	GenericMemoryGraphStreamSource m_allocUsageStreamSource;
	AllocUsageMemoryGraphMarkerSource m_allocUsageMarkerSource;

	CMemoryGraphWnd m_frameUsageMemGraph;
	GenericMemoryGraphStreamSource m_frameUsageMemSource;
	FrameUsageMemoryGraphMarkerSource m_frameUsageMarkerSource;

	CBitmapWnd m_screenshotControl;
	CBitmap m_screenshotBm;

	CCheckedTreeCtrl m_streamTree;
};
