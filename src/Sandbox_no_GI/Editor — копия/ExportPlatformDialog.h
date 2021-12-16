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
#ifndef __EXPORTPLATFORMDIALOG_H__
#define __EXPORTPLATFORMDIALOG_H__

class CExportPlatformDialog : public CDialog
{
public:
	enum {IDD = IDD_EXPORT_PLATFORM};

	CExportPlatformDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CExportPlatformDialog() {}

	enum Platform
	{
		PlatformPC,
		PlatformConsole,

		Platform_COUNT
	};

	void SetPlatformEnabled(Platform platform, bool bEnabled);
	bool GetPlatformEnabled(Platform platform) const {return m_bPlatformsEnabled[platform];}

	void GetConsoleRootDirectory(CString& rstrConsoleRootDirectory);
	void SetConsoleRootDirectory(const CString& crstrConsoleRootDirectory);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	afx_msg void OnPlatformClicked();

	DECLARE_MESSAGE_MAP()

private:
	// Internal data
	bool		m_bPlatformsEnabled[Platform_COUNT];
	CString		m_strConsoleRootDirectory;

	// Controls
	CButton m_platformCheckButtons[Platform_COUNT];
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedExportPlataformRootBrowseButton();
};

#endif //__EXPORTPLATFORMDIALOG_H__
