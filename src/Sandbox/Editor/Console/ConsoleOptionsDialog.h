////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   ConsoleOptionsDialog.h
//  Created:     11/2/2009 by Paulo Zaffari.
//  Description: This dialog is used to configure the options for console
//               syncronization of data and files. Additional options may be
//							 added in the future and don't need to be related to the 
//							 original theme.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef ConsoleOptionsDialog_h__
#define ConsoleOptionsDialog_h__

#pragma once

#include "..\Settings.h"

// CHotUpdateSystem::eConsolePlatforms is defined in Settings.h

class CConsoleOptionsDialog: public CDialog
{
public:
	enum {IDD = IDD_CONSOLE_OPTIONS_DIALOG};

	CConsoleOptionsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConsoleOptionsDialog() {}

	void GetConsoleRootDirectory(CString& rstrConsoleRootDirectory,CHotUpdateSystem::eConsolePlatforms	ePlatform);
	void SetConsoleRootDirectory(const CString& crstrConsoleRootDirectory,CHotUpdateSystem::eConsolePlatforms	ePlatform);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

private:
	CString		m_astrConsoleRootDirectory[CHotUpdateSystem::eNumberOfPlatforms];
	CString		m_astrTargetMachine[CHotUpdateSystem::eNumberOfPlatforms];
	CString		m_astrExecutableName[CHotUpdateSystem::eNumberOfPlatforms];

	CButton		m_aoEnabledCheckButton[CHotUpdateSystem::eNumberOfPlatforms];
	CButton		m_aoAutodetectTargetMachineRadioButton[CHotUpdateSystem::eNumberOfPlatforms];
	CButton		m_aoUseSpecificTargetMachineRadioButton[CHotUpdateSystem::eNumberOfPlatforms];
	CEdit			m_aoTargetPlatformEdit[CHotUpdateSystem::eNumberOfPlatforms];
	CEdit			m_aoConsoleRootDirectoryEdit[CHotUpdateSystem::eNumberOfPlatforms];
	CEdit			m_aoExecutableNameEdit[CHotUpdateSystem::eNumberOfPlatforms];
	CButton		m_aoBrowsePlataformDirectoriesButton[CHotUpdateSystem::eNumberOfPlatforms];
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedExportPlataformRootBrowseButton();
	afx_msg void OnBnClickedConsoleOptionsXbox360EnabledCheck();
	afx_msg void OnBnClickedConsoleOptionsXbox360AutodetectTargetMachineRadio();
	afx_msg void OnBnClickedConsoleOptionsXbox360UseSpecificTargetMachineRadio();
	afx_msg void OnBnClickedConsoleOptionsPlaystation3EnabledCheck();
	afx_msg void OnBnClickedConsoleOptionsPlaystation3AutodetectTargetMachineRadio();
	afx_msg void OnBnClickedConsoleOptionsPlaystation3UseSpecificTargetMachineRadio();
	afx_msg void OnBnClickedConsoleOptionsOkButton();
	afx_msg void OnBnClickedConsoleOptionsCancelButton();
};

#endif // ConsoleOptionsDialog_h__
