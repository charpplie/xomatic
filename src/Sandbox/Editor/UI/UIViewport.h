////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIViewport.h
//  Version:     v1.00
//  Created:     06/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __UIViewport_h__
#define __UIViewport_h__

#include "RenderViewport.h"

class SANDBOX_API CUIViewport : public CRenderViewport
{
	DECLARE_DYNCREATE(CUIViewport)

	// Construction
public:
	CUIViewport();
	virtual ~CUIViewport();

	virtual EViewportType GetType() const { return ET_ViewportUI; }
	virtual void SetType( EViewportType type ) { assert(type == ET_ViewportUI); };

	virtual void Update();

	void SetRegularUpdateFg(bool bUpdate);
	bool IsRegularUpdateFg() { return m_bUpdateFG; }

	void Pause(bool bPaused) { m_bPaused = bPaused; }

protected:
	virtual void OnRender();
	void OnEditorNotifyEvent( EEditorNotifyEvent event );

	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short wheelData, CPoint point);

private:
		void OnGetScreenSize(int& width, int&height);

private:
	float m_lastFrameTimeUI;
	bool m_bGameMode;
	bool m_bPaused;
	bool m_bUpdateFG;
};

#endif // __UIViewport_h__
