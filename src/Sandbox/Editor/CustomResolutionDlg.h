//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : CustomResolutionDlg.h
//  Author           : Jaewon Jung
//  Time of creation : 7/8/2010   16:11
//  Compilers        : VS2008
//  Description      : A dialog for getting a resolution info from users
//  Notice           : Refer to ViewportTitleDlg.cpp for a use case.
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#ifndef __CUSTOMRESOLUTIONDLG_H__
#define __CUSTOMRESOLUTIONDLG_H__
#pragma once

class CCustomResolutionDlg : public CDialog
{
public:
	CCustomResolutionDlg(int w, int h, CWnd* pParent = NULL);

	enum { IDD = IDD_CUSTOM_RESOLUTION };

	int GetWidth() const;
	int GetHeight() const;

protected:
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	CNumberCtrl m_width;
	CNumberCtrl m_height;
	int m_wDefault, m_hDefault;
};

#endif // __CUSTOMRESOLUTIONDLG_H__
