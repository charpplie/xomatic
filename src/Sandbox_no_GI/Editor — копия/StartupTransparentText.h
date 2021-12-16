////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   StartupTransparentText.h
//  Version:     v1.00
//  Created:     19/6/2009 by Francesco Roccucci.
//  Compilers:   Visual Studio.NET
//  Description: Transparent static element for the splash screen.
//				 Redraws the background every time his text is updated
// -------------------------------------------------------------------------
//  History:
//		19:6:2009   16:45 : Created by Francesco Roccucci
////////////////////////////////////////////////////////////////////////////

#ifndef CSTARTUPTRANSPARENTTEXT_H
#define CSTARTUPTRANSPARENTTEXT_H


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StartupTransparentText.h : header file
//

class CStartupTransparentText : public CStatic
{
	DECLARE_DYNAMIC(CStartupTransparentText)
public:
	CStartupTransparentText();
	~CStartupTransparentText();
	
	void SetTextColor(COLORREF colorText){ m_colorText = colorText;};

protected:
	afx_msg LRESULT OnSetText(WPARAM,LPARAM);
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	DECLARE_MESSAGE_MAP()

private:
	CBitmap m_Bmp;
	COLORREF m_colorText;
};


#endif // CSTARTUPTRANSPARENTTEXT_H