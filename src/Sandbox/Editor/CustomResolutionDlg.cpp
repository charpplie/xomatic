//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : CustomResolutionDlg.cpp
//  Author           : Jaewon Jung
//  Time of creation : 7/8/2010   16:16
//  Compilers        : VS2008
//  Description      : A dialog for getting a resolution info from users
//  Notice           : Refer to ViewportTitleDlg.cpp for a use case.
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CustomResolutionDlg.h"

#define MIN_RES 64
#define MAX_RES 8192

BEGIN_MESSAGE_MAP(CCustomResolutionDlg, CDialog)
END_MESSAGE_MAP()

CCustomResolutionDlg::CCustomResolutionDlg(int w, int h, CWnd* pParent /*=NULL*/)
	: CDialog(CCustomResolutionDlg::IDD, pParent), m_wDefault(w), m_hDefault(h)
{
}

BOOL CCustomResolutionDlg::OnInitDialog()
{
	m_width.Create(this, IDC_RES_WIDTH, CNumberCtrl::CENTER_ALIGN);
	m_width.SetInteger(true);
	m_width.SetRange(MIN_RES, MAX_RES);
	m_width.SetValue(m_wDefault);

	m_height.Create(this, IDC_RES_HEIGHT, CNumberCtrl::CENTER_ALIGN);
	m_height.SetInteger(true);
	m_height.SetRange(MIN_RES, MAX_RES);
	m_height.SetValue(m_hDefault);

	return __super::OnInitDialog();
}

int CCustomResolutionDlg::GetWidth() const
{
	return int(m_width.GetValue());
}

int CCustomResolutionDlg::GetHeight() const
{
	return int(m_height.GetValue());
}