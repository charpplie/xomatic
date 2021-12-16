////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TestNameControl.h
//  Version:     v1.00
//  Created:     07/09/2009 by Pau Novau
//  Description: Control used to draw names of controls ( e.g. joysticks ) 
//               in the test view.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Test_Name_Control__h__
#define __Test_Name_Control__h__
#pragma once

class CTestNameControl
	: public CWnd
{
	DECLARE_DYNCREATE( CTestNameControl )

public:
	CTestNameControl();
	CTestNameControl( CString name );
	virtual ~CTestNameControl();

	BOOL Create( DWORD dwStyle, const CRect& rect, CWnd* pParent, UINT nID );

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
	
	static void RegisterWindowClass();

private:
	CString m_name;
};

#endif