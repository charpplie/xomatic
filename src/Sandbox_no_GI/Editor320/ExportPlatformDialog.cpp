////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002-2008.
// -------------------------------------------------------------------------
//  File name:   ExportPlatformDialog.h
//  Version:     v1.00
//  Created:     17/1/2008 by MichaelS.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ExportPlatformDialog.h"

BEGIN_MESSAGE_MAP(CExportPlatformDialog, CDialog)
	ON_COMMAND(IDC_PLATFORM_PC, OnPlatformClicked)
	ON_COMMAND(IDC_PLATFORM_CONSOLE, OnPlatformClicked)
	ON_BN_CLICKED(IDOK, &CExportPlatformDialog::OnBnClickedOk)
	ON_BN_CLICKED(IDC_EXPORT_PLATAFORM_ROOT_BROWSE_BUTTON, &CExportPlatformDialog::OnBnClickedExportPlataformRootBrowseButton)
END_MESSAGE_MAP()

CExportPlatformDialog::CExportPlatformDialog(CWnd* pParent)
:	CDialog(IDD)
{
	for (int i = 0; i < Platform_COUNT; ++i)
	{
		m_bPlatformsEnabled[i] = false;
	}

	m_strConsoleRootDirectory="";
}

void CExportPlatformDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_PLATFORM_PC, m_platformCheckButtons[PlatformPC]);
	DDX_Control(pDX, IDC_PLATFORM_CONSOLE, m_platformCheckButtons[PlatformConsole]);
}

BOOL CExportPlatformDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	for (int i = 0; i < Platform_COUNT; ++i)
	{
		m_platformCheckButtons[i].SetCheck(m_bPlatformsEnabled[i]);
	}

	return TRUE;
}

void CExportPlatformDialog::SetPlatformEnabled(Platform platform, bool bEnabled)
{
	m_bPlatformsEnabled[platform] = bEnabled;
	if (m_platformCheckButtons[platform])
	{
		m_platformCheckButtons[platform].SetCheck(m_bPlatformsEnabled[platform]);
		// Must take care is disabling one platform but keepking any console selected...
	}
}

void CExportPlatformDialog::GetConsoleRootDirectory(CString& rstrConsoleRootDirectory)
{
	rstrConsoleRootDirectory=m_strConsoleRootDirectory;
}

void CExportPlatformDialog::SetConsoleRootDirectory(const CString& crstrConsoleRootDirectory)
{
	m_strConsoleRootDirectory=crstrConsoleRootDirectory;
}

void CExportPlatformDialog::OnPlatformClicked()
{
	// Update all the boolean flags.
	for (int i = 0; i < Platform_COUNT; ++i)
	{
		m_bPlatformsEnabled[i] = m_platformCheckButtons[i].GetCheck();
	}
}

void CExportPlatformDialog::OnBnClickedOk()
{
	OnOK();
}

void CExportPlatformDialog::OnBnClickedExportPlataformRootBrowseButton()
{
	// Nothing to do, for now.
}
