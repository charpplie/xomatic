////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013.
// -------------------------------------------------------------------------
//  File name:   PakManagerDlg.h
//  Version:     v1.00
//  Description: Header file for the pack manager.
//  Author: Nicusor Nedelcu
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __PAKMANAGERDLG_H__
#define __PAKMANAGERDLG_H__

#pragma once
#include "afxcmn.h"
#include "Util/PakFile.h"
#include "afxwin.h"

// CPakManagerDlg dialog
class CPakManagerDlg : public CDialog, ICryArchive::IEnumerateArchiveEntries
{
	DECLARE_DYNAMIC(CPakManagerDlg)

public:
	CPakManagerDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPakManagerDlg();

// Dialog Data
	enum { IDD = IDD_PAK_MANAGER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void SetDlgTitle();
	void SetPathText();
	void FillFiles();
	void HandleEnterKeyOnList( int index );
	void GetCurrentPakPath( CString& rPath );
	bool OnEnumArchiveEntry( const char* pFilename, ICryArchive::Handle hEntry, bool bIsFolder, int aSize, __int64 aModifiedTime );
	void UnpackFilesRecursive( ICryArchive::Handle hFolder, const CString& path, bool bFirstLevel );
	void ReloadLevelCheck();

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl						m_pakEntriesList;
	// handle and list index
	std::map<ICryArchive::Handle, int>	m_currentVisibleFolders;
	std::map<ICryArchive::Handle, int>	m_currentVisibleFiles;
	CPakFile						m_pak;
	ICryArchive::Handle	m_hCurrentFolder;
	ICryArchive::Handle	m_hRootFolder;
	std::vector<ICryArchive::Handle> m_folderStack;
	std::vector<CString> m_folderNamesStack;
	CString							m_startBrowseFolder;
	bool								m_bReloadLevelAfterPakClose;

	BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonOpenPak();
	afx_msg void OnBnClickedButtonCreatePak();
	afx_msg void OnBnClickedButtonAddFilesToPak();
	afx_msg void OnBnClickedButtonExtractFilesFromPak();
	afx_msg void OnBnClickedButtonDeleteFilesFromPak();
	afx_msg void OnBnClickedButtonClose();
	afx_msg void OnNMDblclkListFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownListFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnClose();
	afx_msg void OnBnClickedButtonAddFoldersToPak();
	CProgressCtrl m_pakProgress;
	CStatic m_stStatus;
protected:
	virtual void OnOK();
public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
};

#endif //__PAKMANAGERDLG_H__