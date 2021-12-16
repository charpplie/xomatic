////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   ConsoleOptionsDialog.cpp
//  Created:     11/2/2009 by Paulo Zaffari.
//  Description: This dialog is used to configure the options for console
//               synchronization of data and files. Additional options may be
//							 added in the future and don't need to be related to the 
//							 original theme.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "ConsoleOptionsDialog.h"
#include "ConsoleSync.h"

BEGIN_MESSAGE_MAP(CConsoleOptionsDialog, CDialog)
	ON_BN_CLICKED(IDC_CONSOLE_OPTIONS_XBOX360_ENABLED_CHECK, &CConsoleOptionsDialog::OnBnClickedConsoleOptionsXbox360EnabledCheck)
	ON_BN_CLICKED(IDC_CONSOLE_OPTIONS_XBOX360_AUTODETECT_TARGET_MACHINE_RADIO, &CConsoleOptionsDialog::OnBnClickedConsoleOptionsXbox360AutodetectTargetMachineRadio)
	ON_BN_CLICKED(IDC_CONSOLE_OPTIONS_XBOX360_USE_SPECIFIC_TARGET_MACHINE_RADIO, &CConsoleOptionsDialog::OnBnClickedConsoleOptionsXbox360UseSpecificTargetMachineRadio)
	ON_BN_CLICKED(IDC_CONSOLE_OPTIONS_PLAYSTATION3_ENABLED_CHECK, &CConsoleOptionsDialog::OnBnClickedConsoleOptionsPlaystation3EnabledCheck)
	ON_BN_CLICKED(IDC_CONSOLE_OPTIONS_PLAYSTATION3_AUTODETECT_TARGET_MACHINE_RADIO, &CConsoleOptionsDialog::OnBnClickedConsoleOptionsPlaystation3AutodetectTargetMachineRadio)
	ON_BN_CLICKED(IDC_CONSOLE_OPTIONS_PLAYSTATION3_USE_SPECIFIC_TARGET_MACHINE_RADIO, &CConsoleOptionsDialog::OnBnClickedConsoleOptionsPlaystation3UseSpecificTargetMachineRadio)
	ON_BN_CLICKED(IDC_CONSOLE_OPTIONS_OK_BUTTON, &CConsoleOptionsDialog::OnBnClickedConsoleOptionsOkButton)
	ON_BN_CLICKED(IDC_CONSOLE_OPTIONS_CANCEL_BUTTON, &CConsoleOptionsDialog::OnBnClickedConsoleOptionsCancelButton)
END_MESSAGE_MAP()

CConsoleOptionsDialog::CConsoleOptionsDialog(CWnd* pParent)
:	CDialog(IDD)
{
	
}

void CConsoleOptionsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX,IDC_CONSOLE_OPTIONS_XBOX360_EXPORT_PLATAFORM_ROOT_DIRECTORY_EDIT,m_astrConsoleRootDirectory[CHotUpdateSystem::eXBOX360]);
	DDX_Text(pDX,IDC_CONSOLE_OPTIONS_XBOX360_USE_SPECIFIC_TARGET_MACHINE_EDIT,m_astrTargetMachine[CHotUpdateSystem::eXBOX360]);
	DDX_Text(pDX,IDC_CONSOLE_OPTIONS_XBOX360_EXPORT_PLATAFORM_EXECUTABLE_NAME_EDIT,m_astrExecutableName[CHotUpdateSystem::eXBOX360]);

	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_XBOX360_ENABLED_CHECK,m_aoEnabledCheckButton[CHotUpdateSystem::eXBOX360]);
	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_XBOX360_AUTODETECT_TARGET_MACHINE_RADIO,m_aoAutodetectTargetMachineRadioButton[CHotUpdateSystem::eXBOX360]);
	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_XBOX360_USE_SPECIFIC_TARGET_MACHINE_RADIO,m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::eXBOX360]);

	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_XBOX360_USE_SPECIFIC_TARGET_MACHINE_EDIT,m_aoTargetPlatformEdit[CHotUpdateSystem::eXBOX360]);
	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_XBOX360_EXPORT_PLATAFORM_ROOT_DIRECTORY_EDIT,m_aoConsoleRootDirectoryEdit[CHotUpdateSystem::eXBOX360]);
	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_XBOX360_EXPORT_PLATAFORM_EXECUTABLE_NAME_EDIT,m_aoExecutableNameEdit[CHotUpdateSystem::eXBOX360]);
	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_XBOX360_EXPORT_PLATAFORM_ROOT_BROWSE_BUTTON,m_aoBrowsePlataformDirectoriesButton[CHotUpdateSystem::eXBOX360]);


	DDX_Text(pDX,IDC_CONSOLE_OPTIONS_PLAYSTATION3_EXPORT_PLATAFORM_ROOT_DIRECTORY_EDIT,m_astrConsoleRootDirectory[CHotUpdateSystem::ePLAYSTATION3]);
	DDX_Text(pDX,IDC_CONSOLE_OPTIONS_PLAYSTATION3_USE_SPECIFIC_TARGET_MACHINE_EDIT,m_astrTargetMachine[CHotUpdateSystem::ePLAYSTATION3]);
	DDX_Text(pDX,IDC_CONSOLE_OPTIONS_PLAYSTATION3_EXPORT_PLATAFORM_EXECUTABLE_NAME_EDIT,m_astrExecutableName[CHotUpdateSystem::ePLAYSTATION3]);

	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_PLAYSTATION3_ENABLED_CHECK,m_aoEnabledCheckButton[CHotUpdateSystem::ePLAYSTATION3]);
	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_PLAYSTATION3_AUTODETECT_TARGET_MACHINE_RADIO,m_aoAutodetectTargetMachineRadioButton[CHotUpdateSystem::ePLAYSTATION3]);
	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_PLAYSTATION3_USE_SPECIFIC_TARGET_MACHINE_RADIO,m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::ePLAYSTATION3]);

	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_PLAYSTATION3_USE_SPECIFIC_TARGET_MACHINE_EDIT,m_aoTargetPlatformEdit[CHotUpdateSystem::ePLAYSTATION3]);
	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_PLAYSTATION3_EXPORT_PLATAFORM_ROOT_DIRECTORY_EDIT,m_aoConsoleRootDirectoryEdit[CHotUpdateSystem::ePLAYSTATION3]);
	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_PLAYSTATION3_EXPORT_PLATAFORM_EXECUTABLE_NAME_EDIT,m_aoExecutableNameEdit[CHotUpdateSystem::ePLAYSTATION3]);
	DDX_Control(pDX,IDC_CONSOLE_OPTIONS_PLAYSTATION3_EXPORT_PLATAFORM_ROOT_BROWSE_BUTTON,m_aoBrowsePlataformDirectoriesButton[CHotUpdateSystem::ePLAYSTATION3]);
}

BOOL CConsoleOptionsDialog::OnInitDialog()
{
	int nCount(0);

	CDialog::OnInitDialog();

	gSettings.oHotUpdateSystemSettings.GetPlatformConsoleRootDirectory(CHotUpdateSystem::eXBOX360,m_astrConsoleRootDirectory[CHotUpdateSystem::eXBOX360]);
	gSettings.oHotUpdateSystemSettings.GetPlatformConsoleRootDirectory(CHotUpdateSystem::ePLAYSTATION3,m_astrConsoleRootDirectory[CHotUpdateSystem::ePLAYSTATION3]);

	gSettings.oHotUpdateSystemSettings.GetPlatformTargetMachine(CHotUpdateSystem::eXBOX360,m_astrTargetMachine[CHotUpdateSystem::eXBOX360]);
	gSettings.oHotUpdateSystemSettings.GetPlatformTargetMachine(CHotUpdateSystem::ePLAYSTATION3,m_astrTargetMachine[CHotUpdateSystem::ePLAYSTATION3]);

	gSettings.oHotUpdateSystemSettings.GetPlatformExecutableName(CHotUpdateSystem::eXBOX360,m_astrExecutableName[CHotUpdateSystem::eXBOX360]);
	gSettings.oHotUpdateSystemSettings.GetPlatformExecutableName(CHotUpdateSystem::ePLAYSTATION3,m_astrExecutableName[CHotUpdateSystem::ePLAYSTATION3]);

	for (nCount=CHotUpdateSystem::eFirstPlatform;nCount<CHotUpdateSystem::eNumberOfPlatforms;nCount++)
	{
		// If the platform is not enabled all MUST be disabled, except for the enable button :-)
		if (!gSettings.oHotUpdateSystemSettings.boPlatformEnabled[nCount])
		{
			m_aoEnabledCheckButton[nCount].SetCheck(BST_UNCHECKED);
			m_aoAutodetectTargetMachineRadioButton[nCount].EnableWindow(FALSE);
			m_aoUseSpecificTargetMachineRadioButton[nCount].EnableWindow(FALSE);
			m_aoTargetPlatformEdit[nCount].EnableWindow(FALSE);
			m_aoConsoleRootDirectoryEdit[nCount].EnableWindow(FALSE);
			m_aoBrowsePlataformDirectoriesButton[nCount].EnableWindow(FALSE);
			m_aoExecutableNameEdit[nCount].EnableWindow(FALSE);
		}
		else
		{
			m_aoEnabledCheckButton[nCount].SetCheck(BST_CHECKED);
			m_aoAutodetectTargetMachineRadioButton[nCount].EnableWindow(TRUE);
			m_aoUseSpecificTargetMachineRadioButton[nCount].EnableWindow(TRUE);
			if (!gSettings.oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[nCount])
			{
				m_aoTargetPlatformEdit[nCount].EnableWindow(TRUE);
			}
			m_aoConsoleRootDirectoryEdit[nCount].EnableWindow(TRUE);
			m_aoExecutableNameEdit[nCount].EnableWindow(TRUE);
			m_aoBrowsePlataformDirectoriesButton[nCount].EnableWindow(FALSE);			
		}

		m_aoAutodetectTargetMachineRadioButton[nCount].SetCheck(
			gSettings.oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[nCount]?
			BST_CHECKED:BST_UNCHECKED);
		m_aoUseSpecificTargetMachineRadioButton[nCount].SetCheck(
			gSettings.oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[nCount]?
			BST_UNCHECKED:BST_CHECKED);

		m_aoTargetPlatformEdit[nCount].SetWindowText(m_astrTargetMachine[nCount]);

		m_aoConsoleRootDirectoryEdit[nCount].SetWindowText(m_astrConsoleRootDirectory[nCount]);

		m_aoExecutableNameEdit[nCount].SetWindowText(m_astrExecutableName[nCount]);
	}

	return TRUE;
}

void CConsoleOptionsDialog::GetConsoleRootDirectory(CString& rstrConsoleRootDirectory,CHotUpdateSystem::eConsolePlatforms	ePlatform)
{
	assert(ePlatform!=CHotUpdateSystem::eNumberOfPlatforms);
	rstrConsoleRootDirectory=m_astrConsoleRootDirectory[ePlatform];
}

void CConsoleOptionsDialog::SetConsoleRootDirectory(const CString& crstrConsoleRootDirectory,CHotUpdateSystem::eConsolePlatforms	ePlatform)
{
	assert(ePlatform!=CHotUpdateSystem::eNumberOfPlatforms);
	m_astrConsoleRootDirectory[ePlatform]=crstrConsoleRootDirectory;
}

void CConsoleOptionsDialog::OnBnClickedConsoleOptionsXbox360EnabledCheck()
{
	if (m_aoEnabledCheckButton[CHotUpdateSystem::eXBOX360].GetCheck()==BST_CHECKED)
	{
		m_aoAutodetectTargetMachineRadioButton[CHotUpdateSystem::eXBOX360].EnableWindow(TRUE);
		m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::eXBOX360].EnableWindow(TRUE);
		if (m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::eXBOX360].GetCheck()==BST_CHECKED)
		{
			m_aoTargetPlatformEdit[CHotUpdateSystem::eXBOX360].EnableWindow(TRUE);
		}
		else
		{
			m_aoTargetPlatformEdit[CHotUpdateSystem::eXBOX360].EnableWindow(FALSE);
		}
		m_aoConsoleRootDirectoryEdit[CHotUpdateSystem::eXBOX360].EnableWindow(TRUE);
		m_aoExecutableNameEdit[CHotUpdateSystem::eXBOX360].EnableWindow(TRUE);

		m_aoBrowsePlataformDirectoriesButton[CHotUpdateSystem::eXBOX360].EnableWindow(FALSE);
	}
	else
	{
		m_aoAutodetectTargetMachineRadioButton[CHotUpdateSystem::eXBOX360].EnableWindow(FALSE);
		m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::eXBOX360].EnableWindow(FALSE);
		m_aoTargetPlatformEdit[CHotUpdateSystem::eXBOX360].EnableWindow(FALSE);
		m_aoConsoleRootDirectoryEdit[CHotUpdateSystem::eXBOX360].EnableWindow(FALSE);
		m_aoBrowsePlataformDirectoriesButton[CHotUpdateSystem::eXBOX360].EnableWindow(FALSE);
		m_aoExecutableNameEdit[CHotUpdateSystem::eXBOX360].EnableWindow(FALSE);
	}	
}

void CConsoleOptionsDialog::OnBnClickedConsoleOptionsXbox360AutodetectTargetMachineRadio()
{
	m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::eXBOX360].SetCheck(BST_UNCHECKED);
	m_aoTargetPlatformEdit[CHotUpdateSystem::eXBOX360].EnableWindow(FALSE);
}

void CConsoleOptionsDialog::OnBnClickedConsoleOptionsXbox360UseSpecificTargetMachineRadio()
{
	m_aoAutodetectTargetMachineRadioButton[CHotUpdateSystem::eXBOX360].SetCheck(BST_UNCHECKED);
	m_aoTargetPlatformEdit[CHotUpdateSystem::eXBOX360].EnableWindow(TRUE);
}

void CConsoleOptionsDialog::OnBnClickedConsoleOptionsPlaystation3EnabledCheck()
{
	if (m_aoEnabledCheckButton[CHotUpdateSystem::ePLAYSTATION3].GetCheck()==BST_CHECKED)
	{
		m_aoAutodetectTargetMachineRadioButton[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(TRUE);
		m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(TRUE);
		if (m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::ePLAYSTATION3].GetCheck()==BST_CHECKED)
		{
			m_aoTargetPlatformEdit[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(TRUE);
		}
		else
		{
			m_aoTargetPlatformEdit[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(FALSE);
		}
		m_aoConsoleRootDirectoryEdit[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(TRUE);

		m_aoBrowsePlataformDirectoriesButton[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(FALSE);
		m_aoExecutableNameEdit[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(TRUE);
	}
	else
	{
		m_aoAutodetectTargetMachineRadioButton[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(FALSE);
		m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(FALSE);
		m_aoTargetPlatformEdit[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(FALSE);
		m_aoConsoleRootDirectoryEdit[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(FALSE);
		m_aoBrowsePlataformDirectoriesButton[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(FALSE);
		m_aoExecutableNameEdit[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(TRUE);
	}	
}

void CConsoleOptionsDialog::OnBnClickedConsoleOptionsPlaystation3AutodetectTargetMachineRadio()
{
	m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::ePLAYSTATION3].SetCheck(BST_UNCHECKED);
	m_aoTargetPlatformEdit[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(FALSE);
}

void CConsoleOptionsDialog::OnBnClickedConsoleOptionsPlaystation3UseSpecificTargetMachineRadio()
{
	m_aoAutodetectTargetMachineRadioButton[CHotUpdateSystem::ePLAYSTATION3].SetCheck(BST_UNCHECKED);
	m_aoTargetPlatformEdit[CHotUpdateSystem::ePLAYSTATION3].EnableWindow(TRUE);
}

void CConsoleOptionsDialog::OnBnClickedConsoleOptionsOkButton()
{
	int nCount(0);
	CString strPlaceholderString;

	for (nCount=CHotUpdateSystem::eFirstPlatform;nCount<CHotUpdateSystem::eNumberOfPlatforms;nCount++)
	{
		gSettings.oHotUpdateSystemSettings.boPlatformEnabled[nCount]=m_aoEnabledCheckButton[nCount].GetCheck()==BST_CHECKED;
		gSettings.oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[nCount]=(m_aoAutodetectTargetMachineRadioButton[nCount].GetCheck()==BST_CHECKED);

		m_aoTargetPlatformEdit[nCount].GetWindowText(strPlaceholderString);
		gSettings.oHotUpdateSystemSettings.SetPlatformTargetMachine(nCount,strPlaceholderString);

		m_aoConsoleRootDirectoryEdit[nCount].GetWindowText(strPlaceholderString);
		gSettings.oHotUpdateSystemSettings.SetPlatformConsoleRootDirectory(nCount,strPlaceholderString);

		m_aoExecutableNameEdit[nCount].GetWindowText(strPlaceholderString);
		gSettings.oHotUpdateSystemSettings.SetPlatformExecutableName(nCount,strPlaceholderString);		
	}

	CConsoleSynchronization::GetCConsoleSynchronization().NotifyOptionsChanged();

	EndDialog(IDOK);
}

void CConsoleOptionsDialog::OnBnClickedConsoleOptionsCancelButton()
{
	EndDialog(IDCANCEL);
}
