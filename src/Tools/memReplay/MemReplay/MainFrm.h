// MainFrm.h : interface of the CMainFrame class
//

#pragma once

#include "IIdleHandler.h"

class CMainFrame : public CFrameWnd
{
public:
	enum ToolBar
	{
		TB_None = -1,
		TB_Bucket = 0,
		TB_Code,
		TB_Context,
		TB_Usage,

		TB_Count
	};
	
protected: // create from serialization only
	CMainFrame();

	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
	void SetActiveToolBar(ToolBar tb);

// Operations
public:

	void StartFind();
	void StartTypeInfoLookup();
	void SetStatus(const TCHAR* statusText);

	void UpdateToolbar();

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBarCommon;
	CToolBar    m_wndToolBars[TB_Count];

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg LRESULT OnFindReplace(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMainUI(CCmdUI* pCmdUI);
	afx_msg void OnRSXUI(CCmdUI* pCmdUI);
	afx_msg void OnCombinedUI(CCmdUI* pCmdUI);
	afx_msg void OnCountUI(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()

private:
	CFindReplaceDialog* m_findDlg;
	bool m_doTypeLookup;

	int m_activeToolBar;
};


