// MsMoneyTheme.h: interface for the CMsMoneyTheme class.
//
// This file is a part of the XTREME TOOLKIT PRO MFC class library.
// (c)1998-2009 Codejock Software, All Rights Reserved.
//
// THIS SOURCE FILE IS THE PROPERTY OF CODEJOCK SOFTWARE AND IS NOT TO BE
// RE-DISTRIBUTED BY ANY MEANS WHATSOEVER WITHOUT THE EXPRESSED WRITTEN
// CONSENT OF CODEJOCK SOFTWARE.
//
// THIS SOURCE CODE CAN ONLY BE USED UNDER THE TERMS AND CONDITIONS OUTLINED
// IN THE XTREME TOOLKIT PRO LICENSE AGREEMENT. CODEJOCK SOFTWARE GRANTS TO
// YOU (ONE SOFTWARE DEVELOPER) THE LIMITED RIGHT TO USE THIS SOFTWARE ON A
// SINGLE COMPUTER.
//
// CONTACT INFORMATION:
// support@codejock.com
// http://www.codejock.com
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CCrytekTheme_H_
#define _CCrytekTheme_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CCrytekTheme : public CXTPNativeXPTheme
{
public:
	CCrytekTheme();
	virtual ~CCrytekTheme();

	virtual void DrawImage(CDC* pDC, CPoint pt, CSize sz, CXTPImageManagerIcon* pImage, BOOL bSelected, BOOL bPressed, BOOL bEnabled = TRUE, BOOL bChecked = FALSE, BOOL bPopuped = FALSE, BOOL bToolBarImage = TRUE);
};

#endif _CCrytekTheme_H_
