//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File Name        : CustomAspectRatioDlg.h
//  Author           : Axel Gneiting
//  Time of creation : 18/1/2012
//  Compilers        : VS2010
//  Description      : A dialog for getting an aspect ratio info from users
//  Notice           : Refer to ViewportTitleDlg.cpp for a use case.
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#ifndef __CUSTOMASPECTRATIODLG_H__
#define __CUSTOMASPECTRATIODLG_H__
#pragma once

class CCustomAspectRatioDlg : public CDialog
{
public:
	CCustomAspectRatioDlg(int x, int y, CWnd* pParent = NULL);

	enum { IDD = IDD_CUSTOM_ASPECT_RATIO };

	int GetX() const;
	int GetY() const;

protected:
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	CNumberCtrl m_x;
	CNumberCtrl m_y;
	int m_xDefault, m_yDefault;
};

#endif // __CUSTOMRESOLUTIONDLG_H__
