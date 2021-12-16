////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TestView.h
//  Version:     v1.00
//  Created:     28/07/2009 by Pau Novau
//  Description: View used to hold Ui for playing with motion params by the
//               user.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Test_View__h__
#define __Test_View__h__
#pragma once

#include "LMGEditor/DockingBaseView.h"

#include "LMGEditor/ScrollHelper.h"
#include "LMGEditor/TestJoystickControl.h"
#include "LMGEditor/TestSliderControl.h"

class CTestView 
	: public CDockingBaseView
{
	DECLARE_DYNCREATE( CTestView )

public:
	CTestView();
	virtual ~CTestView();

	void ClearTestView();
	CTestJoystickControl* CreateJoystickControl();
	CTestSliderControl* CreateSliderControl();

	void CreateNameControl( int left, int top, int width, const CString& name );

	void RecalculateCurrentLayoutSize();

protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void OnSize( UINT nType, int cx, int cy );
	DECLARE_MESSAGE_MAP()

private:
	std::vector< CWnd* > m_controls;

	CScrollHelper m_scrollHelper;
};

#endif
