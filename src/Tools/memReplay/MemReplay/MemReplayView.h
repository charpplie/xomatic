#pragma once

#include "TabPageWnd.h"
#include "ContextPage.h"
#include "MemReplayDoc.h"

#include "ReplaySDK/ContextTree.h"
#include "ReplaySDK/CGFPhysicsExporter.h"
#include "ReplaySDK/RangedContextExporter.h"

class CMemReplayDoc;
class ContextTreeNode;
class ITabPage;
class CUsagePage;
class IReplayDatabaseConnection;

class CMemReplayView : public CView
{
	DECLARE_DYNCREATE(CMemReplayView)

public:
	CMemReplayView();
	virtual ~CMemReplayView();

	CMemReplayDoc* GetDocument() const;
	IReplayDatabaseConnection* GetConnection() const;

	void OpenCodeViewFor(const char* name, const SharedPtr<GenericTree>& hier);
	void OpenCodeViewFor(const ContextTreeNode& node);
	void OpenCodeViewForFrames(int frameBeginIdx, int frameEndIdx);
	void OpenCodeViewForAllocEvRange(s64 begin, s64 end);
	void OpenCodeBucketViewForAllocEvRange(s64 begin, s64 end, const std::vector<int>& buckets);
	void OpenCodeQuery_Sizer(const GenericTree& tree, const GenericTreeNode* root);
	void OpenReverseCodeViewFor(const char* name, const GenericTree& hier, const std::vector<TAddress>& from);
	void OpenGatherTDCodeViewFor(const char* name, const GenericTree& hier, const std::vector<TAddress>& from);
	void OpenGatherBUCodeViewFor(const char* name, const GenericTree& hier, const std::vector<TAddress>& from);
	void OpenExcludeCodeViewFor(const char* name, const GenericTree& hier, const std::vector<TAddress>& from);
	void OpenTypeInfoViewFor(const s8 *typeName);
	void AddCodeUsagePlot(const char* name, const std::vector<TAddress>& addresses, int memoryType);

	void OpenBucketAnalysis();

	void ExportToXLS();

	ITabPage* GetActiveTab();
	void SelectTab(ITabPage* page);

	template <typename T>
	void AddTab(T* tab, bool allowClose)
	{
		tab->Create(T::IDD, &m_tabWnd);
		m_tabWnd.AddPage(tab, allowClose ? CTabPageWnd::TPW_ALLOWCLOSE : 0);
		m_tabs.push_back(tab);
	}

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	virtual int OnCreate(LPCREATESTRUCT lpcs);
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC);
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnEraseBkgnd(CDC* pDC);

protected:
	DECLARE_MESSAGE_MAP()

private:
	enum
	{
		ID_TABWND = WM_USER+1,
	};

	enum
	{
		TabWnd_Margin = 5,
	};

private:
	void OnPageChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void OnPageCloseClicked(NMHDR* pNMHDR, LRESULT* pResult);
	IReplayDatabaseConnection::CodeTreeFilter PromptForCTF();
	IReplayDatabaseConnection::CodeTreeFilter CodeTreeFilterFromType(UINT type);

private:
	void DeleteTab(size_t idx)
	{
		CDialog* page = m_tabWnd.GetPage(idx);
		m_tabWnd.RemovePage(idx);
		delete page;
		m_tabs.erase(m_tabs.begin() + idx);
	}

private:
	CImageList m_ImgList;
	CTabPageWnd m_tabWnd;
	CUsagePage* m_usagePage;

	std::vector<ITabPage*> m_tabs;
};

inline CMemReplayDoc* CMemReplayView::GetDocument() const
	{ return reinterpret_cast<CMemReplayDoc*>(m_pDocument); }
inline IReplayDatabaseConnection* CMemReplayView::GetConnection() const
	{ return GetDocument() ? &*GetDocument()->GetConnection() : NULL; }

