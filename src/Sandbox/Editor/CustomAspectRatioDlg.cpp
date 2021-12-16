//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File Name        : CustomAspectRatioDlg.cpp
//  Author           : Axel Gneiting
//  Time of creation : 18/1/2012
//  Compilers        : VS2010
//  Description      : A dialog for getting an aspect ratio info from users
//  Notice           : Refer to ViewportTitleDlg.cpp for a use case.
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "CustomAspectRatioDlg.h"

#define MIN_ASPECT 1
#define MAX_ASPECT 16384

BEGIN_MESSAGE_MAP(CCustomAspectRatioDlg, CDialog)
END_MESSAGE_MAP()

CCustomAspectRatioDlg::CCustomAspectRatioDlg(int x, int y, CWnd* pParent /*=NULL*/)
	: CDialog(CCustomAspectRatioDlg::IDD, pParent), m_xDefault(x), m_yDefault(y)
{
}

BOOL CCustomAspectRatioDlg::OnInitDialog()
{
	m_x.Create(this, IDC_RATIO_X, CNumberCtrl::CENTER_ALIGN);
	m_x.SetInteger(true);
	m_x.SetRange(MIN_ASPECT, MAX_ASPECT);
	m_x.SetValue(m_xDefault);

	m_y.Create(this, IDC_RATIO_Y, CNumberCtrl::CENTER_ALIGN);
	m_y.SetInteger(true);
	m_y.SetRange(MIN_ASPECT, MAX_ASPECT);
	m_y.SetValue(m_yDefault);

	return __super::OnInitDialog();
}

int CCustomAspectRatioDlg::GetX() const
{
	return int(m_x.GetValue());
}

int CCustomAspectRatioDlg::GetY() const
{
	return int(m_y.GetValue());
}