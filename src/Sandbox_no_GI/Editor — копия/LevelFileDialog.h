////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002-2012.
// -------------------------------------------------------------------------
//  File name:   LevelFileDialog.h
//  Version:     v1.00
//  Created:     25/01/2012 by Axel.
//  Compilers:   Visual Studio 2010
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __LEVELFILEDIALOG_H__
#define __LEVELFILEDIALOG_H__
#pragma once

class CLevelFileDialog : public CXTResizeDialog
{
	DECLARE_DYNAMIC(CLevelFileDialog)

public:
	CLevelFileDialog( bool bOpenFileDialog );
	virtual ~CLevelFileDialog() {}

	CString GetFileName() const { return m_fileName; }

protected:
	virtual void OnOK();	

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();	
	
	afx_msg void OnTreeSelectionChanged( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTreeDoubleClicked( NMHDR* pNMHDR, LRESULT* pResult );	
	afx_msg void OnNewFolder();
	afx_msg void OnFilterChanged();

	void ReloadTree();
	void ReloadTreeRec( const CString currentFolder, const CString currentFolderName, 
		const CString itemPath, const HTREEITEM currentTreeItem );

	bool CheckLevelFolder( const CString folder, std::vector<CString> *levelFiles = NULL );
	bool CheckSubFoldersForLevelsRec( const CString folder, bool bRoot = true );	
	bool IsLevelFile( const CString file );
	bool ValidateLevelPath( const CString folder );
	bool IsValidFileName( const CString fileName );

	void SaveLastUsedLevelPath( const CString path );
	void LoadLastUsedLevelPath();	

	DECLARE_MESSAGE_MAP()

	CString m_fileName;
	CString m_filter;
	bool m_bOpenDialog;
	CTreeCtrl m_tree;
	CEdit m_nameEdit;
	CEdit m_filterEdit;
	CImageList m_imageList;
	std::map<HTREEITEM, CString> m_itemFolders;
};

#endif