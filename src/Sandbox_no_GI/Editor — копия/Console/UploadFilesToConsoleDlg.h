////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	UploadFilesToConsoleDlg.h
//  Version:	v1.00
//  Created:	29/07/2010 by Nicusor Nedelcu
//  Description:	Dialog for uploading/sync arbitrary game files to the console
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __UploadFilesToConsoleDlg_h__
#define __UploadFilesToConsoleDlg_h__
#pragma once

#include "afxwin.h"
#include "afxcmn.h"
#include <vector>
#include "Controls/TreeCtrlReport.h"
#include "Console/ConsoleHotUpdate.h"

// CUploadFilesToConsoleDlg dialog

class CUploadFilesToConsoleDlg : public CXTResizeDialog, CConsoleHotUpdate::SHotUpdateJobObserver
{
	DECLARE_DYNAMIC(CUploadFilesToConsoleDlg)

public:
	CUploadFilesToConsoleDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CUploadFilesToConsoleDlg();

	void FillSourceTree();
	bool DeleteEmptyFoldersRecursive( CTreeItemRecord* pRec );

// Dialog Data
	enum { IDD = IDD_UPLOAD_FILES_TO_CONSOLE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:

	ICryPak *m_pIPak;

	struct FileInfo
	{
		CString filename;
		unsigned attrib;
		time_t time_create;    /* -1 for FAT file systems */
		time_t time_access;    /* -1 for FAT file systems */
		time_t time_write;
		_fsize_t size;
	};

	static bool FileInfoCompare( const FileInfo &f1, const FileInfo &f2 );
private:
	bool ScanDirectoryRecursive( const CString &dir, std::vector<FileInfo>& files, bool bAddFoldersAlso, bool bRecursive, bool bScanPaksAlso );
public:
	bool ScanDirectory( const CString &dir, std::vector<FileInfo>& files, bool bAddFoldersAlso = true, bool bRecursive = false, bool bScanPaksAlso = false );
	void ActivateListItem( int aIndex );
	void AddSelectedFilesAndFolders();
	void UpdateUploadInfoLabel();
	void WriteToLog( const char* pText, COLORREF aTextColor = 0 );

	// implement CConsoleHotUpdate::SHotUpdateJobObserver
	void OnUploadStart();
	void OnUploadEnd( bool bAborted );
	void OnUploadFileProgress( CHotUpdateSystem::EConsolePlatform aPlatform, const char* pCurrentFilename, UINT aUploadedFileCount, UINT aTotalFileCount, EResultCode aResult, const char* pAdditionalResultMsg );
	void OnRcMessage( CHotUpdateSystem::EConsolePlatform aPlatform, IResourceCompilerListener::MessageSeverity aSeverity, const char* pMsg );

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedUploadFiles();
	afx_msg void OnBnClickedClearFilter();

	CString								m_currentPath, m_rootFolder;
	std::map<string,int>	m_mapExtToIconIndex;
	std::vector<FileInfo> m_files;
	std::set<CString>			m_filesToUpload;
	UINT									m_totalFileCount, m_totalFolderCount, m_totalBytesToSend;
	CConsoleHotUpdate::TJobHandle m_hUploadJob;
	CTreeCtrlReport				m_treeTarget;
	CListCtrl							m_lstSource;
	bool									m_bDragItems;
	CFont									m_fntBoldButton;
	CProgressCtrl					m_progressTotal;
	CRichEditCtrl					m_reditLog;

	afx_msg void OnNMDblclkListSource(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownListSource(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void OnOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLvnBegindragListSource(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedClearAll();
	afx_msg void OnBnClickedRemoveSelected();
	afx_msg void OnBnClickedClose();
	afx_msg void OnBnClickedButtonHide();
	afx_msg void OnBnClickedAddItems();
	afx_msg void OnBnClickedButtonAbortUpload();
	afx_msg void OnBnClickedButtonClearLog();
};

#endif //__UploadFilesToConsoleDlg_h__