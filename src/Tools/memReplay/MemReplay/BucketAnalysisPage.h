#pragma once

#include "MemoryGraphWnd.h"
#include "MemoryGraphDataSources.h"
#include "CheckedTreeCtrl.h"

#include "ITabPage.h"

#include "ReplaySDK/IReplayDatabaseConnection.h"

class CBucketAnalysisPage : public CDialog, public ITabPage
{
	DECLARE_DYNAMIC(CBucketAnalysisPage)

public:
	// Dialog Data
	enum { IDD = IDD_USAGEPAGE };

public:
	CBucketAnalysisPage(IReplayDatabaseConnection* db, const SharedPtr<BucketUsageResult>& usage);

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
		ID_BUCKETLIST,
	};

private:
	void OnMemGraphHoverChanged(NMHDR* nmHdr, LRESULT* result);
	void OnBucketListItemChanged(NMHDR* nmHdr, LRESULT* result);
	void OnBucketListHeaderClicked(NMHDR* nmHdr, LRESULT* result);

	void AddPlot(const char* name, COLORREF colour, std::vector<int>& values, GenericMemoryGraphStreamSource& source);

private:
	IReplayDatabaseConnection* m_db;
	SharedPtr<BucketUsageResult> m_usage;
	std::vector<int> m_buckets;

	CMemoryGraphWnd m_memGraph;
	GenericMemoryGraphStreamSource m_wasteStreamSource;
	GenericMemoryGraphStreamSource m_sizeStreamSource;
	GenericMemoryGraphStreamSource m_capacityStreamSource;

	CListCtrl m_bucketList;
};
