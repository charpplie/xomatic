#pragma once
#include <ILiveCreatePlatform.h>
#include "EditorLiveCreateManager.h"
#include "ConfigPanel.h"

#ifndef NO_LIVECREATE

class CLiveCreateTargetsList : public CListBox
{
	DECLARE_DYNAMIC(CLiveCreateTargetsList);

	const static int kItemHeight = 46;
	const static int kItemMargin = 7;
	const static int kLineHeight = 13;
	const static int kItemIconSize = 32;

	struct Bitmaps
	{
		CBitmap*	m_pNormal;
		CBitmap*	m_pDisabled;
		CBitmap*	m_pConnected;

		Bitmaps(const char* szBaseFileName);
		~Bitmaps();
	};

	typedef std::map<string, Bitmaps*> TPlatformBitmaps;
	TPlatformBitmaps m_platformBitmaps;

	CFont* m_pNormalFont;
	CFont* m_pBoldFont;
	CFont* m_pItalicFont;

	bool m_bIsInitialized;

public:
	CLiveCreateTargetsList();   // standard constructor
	virtual ~CLiveCreateTargetsList();

	DECLARE_MESSAGE_MAP()
	afx_msg void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	afx_msg void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
	afx_msg int CompareItem( LPCOMPAREITEMSTRUCT lpCompareItemStruct );
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

protected:
	virtual void PreSubclassWindow();
	void Init();
	void InitializeFonts(CFont* pBaseFont);
};

class CLiveCreateTargetsPanel : public CDialog
	, public IEditorNotifyListener
	, public LiveCreate::CEditorManager::IListener
	, public LiveCreate::IManagerListenerEx
{
	static const int kListMargin = 4;
	static const int kListTopMargin = 36;;

public:
	CLiveCreateTargetsPanel(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLiveCreateTargetsPanel();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual void OnDestroy();

	afx_msg void OnAddTargets();
	afx_msg void OnRemoveZomibes();

private:
	// IEditorNotifyListener interface
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );	

	// LiveCreate::CEditorManager::IListener interface
	virtual void OnHostConnected(LiveCreate::IHostInfo* pHostInfo);
	virtual void OnHostDisconnected(LiveCreate::IHostInfo* pHostInfo);
	virtual void OnHostReady(LiveCreate::IHostInfo* pHostInfo);
	virtual void OnHostBusy(LiveCreate::IHostInfo* pHostInfo);

	CColorCtrl<CLiveCreateTargetsList> m_targets;
	CButton m_btnAddHosts;
	CButton m_btnCleanHosts;

	void ClearList();
	void FillList();
};

#endif