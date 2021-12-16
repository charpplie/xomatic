#pragma once

#include "ITabPage.h"
#include "DistributionMapWnd.h"
#include "MemoryGraphWnd.h"
#include "MemoryGraphDataSources.h"

class MemoryMapReplay;
class FrameUsageTracker;

class CMemoryMapPage : public CDialog, public ITabPage
{
	DECLARE_DYNAMIC(CMemoryMapPage)

public:
	CMemoryMapPage(const FrameUsageTracker& memUsage, const MemoryMapReplay& memMap);
	virtual ~CMemoryMapPage();

// Dialog Data
	enum { IDD = IDD_MEMORYMAPPAGE };

public: // ITabPage Members
	void OnTabActivate();

	void SetMemoryType(int type) {}
	int GetMemoryType() const { return MEMTYPE_COMBINED; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual void OnCancel() {}

	DECLARE_MESSAGE_MAP()

private:
	enum
	{
		ID_MEMORYMAP = 1,
		ID_TIMELINE,
	};

private:
	void OnTimelineSelectionChanged(NMHDR*, LRESULT*);

private:
	const MemoryMapReplay* m_memMap;

	CMemoryGraphWnd m_timeline;
	GenericMemoryGraphStreamSource m_timelineDataSource;

	CDistributionMapWnd m_memoryMap;
};

