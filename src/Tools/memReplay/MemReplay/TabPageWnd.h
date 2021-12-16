#ifndef RESIZABLEPROPERTYSHEETWND_H
#define RESIZABLEPROPERTYSHEETWND_H

class CTabPageWnd : public CTabCtrl
{
public:
	enum
	{
		TPW_PAGECHANGED = WM_USER + 1,
		TPW_CLOSECLICKED
	};

	enum
	{
		TPW_ALLOWCLOSE = 1,
	};

	struct CloseClicked
	{
		NMHDR hdr;
		int index;
	};

public:
	CTabPageWnd();

	CDialog* GetPage(size_t idx) { return idx < m_tabs.size() ? m_tabs[idx].tab : NULL; }
	void AddPage(CDialog* page, DWORD flags);
	void RemovePage(size_t idx);

	void SetActivePage(CDialog* page);
	void SetActivePage(int page);
	CDialog* GetActivePage() { return GetPage(m_activeTab); }
	int GetPageIndex(CDialog* activePage);
	size_t GetPageCount() { return m_tabs.size(); }

protected:
	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	void OnSize(UINT nType, int cx, int cy);
	void OnLButtonDown(UINT nFlags, CPoint point);
	void OnLButtonUp(UINT nFlags, CPoint point);
	void OnMButtonUp(UINT nFlags, CPoint point);
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnMouseLeave();
	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()

private:
	struct TabDef
	{
		TabDef(CDialog* tab, DWORD flags) 
			: tab(tab)
			, flags(flags)
			, rcCloseBounds(0, 0, 0, 0) {}

		CDialog* tab;
		DWORD flags;
		CRect rcCloseBounds;
	};

	struct TabMatches
	{
		TabMatches(CDialog* p) : page(p) {}
		CDialog* page;
		bool operator () (const TabDef& a) const { return a.tab == page; }
	};

private:
	void SizePages();
	void RaisePageChanged();
	void RaiseCloseClicked(int index);

private:
	CImageList m_images;
	std::vector<TabDef> m_tabs;
	int m_activeTab;
	int m_hoverTab;
};

#endif
