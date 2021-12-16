////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   StartupTransparentText.cpp
//  Version:     v1.00
//  Created:     19/6/2009 by Francesco Roccucci.
//  Compilers:   Visual Studio.NET
//  Description: Transparent static element for the splash screen.
//				 Redraws the background every time his text is updated
// -------------------------------------------------------------------------
//  History:
//		19:6:2009   16:45 : Created by Francesco Roccucci
////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include "StartupTransparentText.h"

IMPLEMENT_DYNAMIC(CStartupTransparentText, CStatic)

CStartupTransparentText::CStartupTransparentText()
{
	m_colorText = RGB(0,0,0);
}

CStartupTransparentText::~CStartupTransparentText()
{

}

BEGIN_MESSAGE_MAP(CStartupTransparentText, CStatic)
	ON_MESSAGE(WM_SETTEXT,OnSetText)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

LRESULT CStartupTransparentText::OnSetText(WPARAM wParam,LPARAM lParam)
{
	LRESULT Result = Default();
	Invalidate();
	UpdateWindow();
	return Result;
}

HBRUSH CStartupTransparentText::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextColor(m_colorText);
	return (HBRUSH)GetStockObject(NULL_BRUSH);
}

BOOL CStartupTransparentText::OnEraseBkgnd(CDC* pDC)
{
	if (m_Bmp.GetSafeHandle() == NULL)
	{
		CRect Rect;
		GetWindowRect(&Rect);
		CWnd *pParent = GetParent();
		ASSERT(pParent);
		pParent->ScreenToClient(&Rect);  //convert our corrdinates to our parents

		//copy what's on the parents at this point
		CDC *pDC = pParent->GetDC();
		CDC MemDC;
		MemDC.CreateCompatibleDC(pDC);
		m_Bmp.CreateCompatibleBitmap(pDC,Rect.Width(),Rect.Height());
		CBitmap *pOldBmp = MemDC.SelectObject(&m_Bmp);
		MemDC.BitBlt(0,0,Rect.Width(),Rect.Height(),pDC,Rect.left,Rect.top,SRCCOPY);
		MemDC.SelectObject(pOldBmp);
		pParent->ReleaseDC(pDC);
	}
	else //copy what we copied off the parent the first time back onto the parent
	{
		CRect Rect;
		GetClientRect(Rect);
		CDC MemDC;
		MemDC.CreateCompatibleDC(pDC);
		CBitmap *pOldBmp = MemDC.SelectObject(&m_Bmp);
		pDC->BitBlt(0,0,Rect.Width(),Rect.Height(),&MemDC,0,0,SRCCOPY);
		MemDC.SelectObject(pOldBmp);
	}

	return TRUE;
}

