////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
// -------------------------------------------------------------------------
//  File name:   VisualLogWnd.h
//  Version:     v1.00
//  Created:     6/XI/2007 by Kire.
//  Compilers:   Visual Studio.NET 2005
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "VisualLogTextPreview.h"
#include "VisualLogControls.h"



class CVisualLogWnd;



// The view within the Visual Log Viewer window (CVisualLogWnd)
class CVLogImageView : public CView
{
public:	// Construction & destruction
	CVLogImageView();
	virtual ~CVLogImageView();
	int			m_nPrevFrame;

public:	// Operations
	bool Create( DWORD dwStyle,const RECT &rect,CWnd *pParentWnd,UINT nID );	// Creates the view window
	void SetCommonData(SVLogCommonData *pCD) { m_pCD = pCD; }
	void InitDestRect();

protected:	// Virtual overrides
	virtual void OnDraw(CDC *pDC);
	virtual void PostNcDestroy();

protected:	// Message handlers
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

private:		// Member data
	CRect						m_rcClient, m_rcDest;

	SVLogCommonData	*m_pCD;
};



// The view within the CVLogTextPreview docking frame
class CVLogTextView : public CView
{
public:	// Construction & destruction
	CVLogTextView();
	virtual ~CVLogTextView();

public:	// Operations
	bool Create( DWORD dwStyle,const RECT &rect,CWnd *pParentWnd,UINT nID );	// Creates the view window
	void SetCommonData(SVLogCommonData *pCD) { m_pCD = pCD; }
	void ResetColumns();
	CDC *GetBkDC() { return &m_dcBack; }

protected:	// Virtual overrides
	virtual void OnDraw(CDC *pDC);
	virtual void PostNcDestroy();

protected:	// Message handlers
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()

private:		// Member data
	CRect						m_rcClient;
	CFont						m_fntInfo;
	CBitmap					m_bmBack;
	CDC							m_dcBack;
	int							m_nColumnWidth;
	CArray<int,int>	m_arrColHeights;

	SVLogCommonData	*m_pCD;
};



// CVisualLogWnd frame
class CVisualLogWnd : public CXTPFrameWnd
{
	friend class CVLogImageView;

	DECLARE_DYNCREATE(CVisualLogWnd)

public:	// Construction & destruction
	CVisualLogWnd();
	virtual ~CVisualLogWnd();

public:	// Operations
	BOOL Create(DWORD dwStyle,const RECT& rect,CWnd* pParentWnd);
	CXTPDockingPaneManager* GetDockingPaneManager() { return &m_paneManager; }
private:
	bool ReadLogFiles();

protected:	// Message handlers
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();

	afx_msg LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDialogNotify(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:		// Static members
	static void RegisterViewClass();

private:	// Member data
	CVLogImageView					*m_pView;					// The view within (captured images)
	CVLogTextView						*m_pTextView;			// The view within the text preview pane
	CXTPDockingPaneManager	m_paneManager;		// The pane manager

	CVLogTextPreview				m_paneText;				// The text preview pane
	CVisualLogControls			m_paneControls;		// The controls pane

	CVisualLogDialog				*m_pControlsDlg;	// The dialog within the controls pane

private:	// Other data
	SVLogCommonData *m_pCD;
};
