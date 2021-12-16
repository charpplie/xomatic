////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   SmartFileOpenDialog.h
//  Version:     v1.00
//  Created:     9/4/2009 by Jaewon Jung.
//  Compilers:   Visual Studio 2008
//  Description: Smart file open dialog.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __smartfileopendialog_h__
#define __smartfileopendialog_h__
#pragma once

#include <XTToolkitPro.h>
#include "Util\FileUtil.h"
#include "CustomFileDialog.h"
#include "Controls\PreviewModelCtrl.h"

//////////////////////////////////////////////////////////////////////////
/*! CSmartFileOpenDialog Smart file selection dialog.
*/
class CSmartFileOpenDialog : public CXTResizeDialog
{
public:
	DECLARE_DYNAMIC(CSmartFileOpenDialog)

	CSmartFileOpenDialog(const CString& initialSearchTerm, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSmartFileOpenDialog();

	CString GetFilePath() const
	{
		return m_currentFile;
	}

	enum { IDD = IDD_SMART_FILE_OPEN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Controls
	CEdit m_inputCtrl;
	CFilesListCtrl m_outputListCtrl;
	CImageList m_imageListFiles;

	std::map<CString,UINT> m_extToIcons;
	CString m_currentFile;
	CString m_initialSearchTerm;
	CFileUtil::FileArray m_filesToList;

	// Preview stuff
	CPreviewModelCtrl m_previewCtrl;
	CStatic m_previewImageCtrl;
	CBitmap m_previewBitmap;
	CRect m_rcPreview;
	_smart_ptr<ISound> m_pSound;
	CButton m_playSound;
	CStatic m_fileInfoCtrl;
	CButton m_previewOn;
	int m_previewRGBA;
	enum EPreviewRGBA
	{
		ESHOW_RGB = 0,
		ESHOW_ALPHA,
		ESHOW_RGBA
	};
	//! A timer for delaying the preview.
	int m_nTimer;
	int m_previewItem;

	// Auto-completion stuff
	std::vector<CString> m_tagsOfPrefix;
	CString m_prefix;
	int m_iNextTagOfPrefix;

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	//////////////////////////////////////////////////////////////////////////
	// MESSAGE HANDLERS.
	//////////////////////////////////////////////////////////////////////////
	afx_msg void OnNMClickFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclkFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeSearchtags();
	afx_msg void OnBnClickedRefresh();
	afx_msg void OnBnClickedNormal();
	afx_msg void OnBnClickedPlay();
	afx_msg void OnBnClickedPreviewOn();
	afx_msg void OnBnClickedShowAlpha();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void GetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);

	// Check the type & validity of the selected item and update the states of buttons according to it.
	int CheckSelectedItem(bool delay);
	// A callback function for displaying the database building progress
	static bool BuildProgressCallBack(const CString& msg);

	void SchedulePreview(int item);
	void CancelPreviewTimer();
	void PreviewItem(int item);
	void PreviewOff();

	CString GetPrefixBeforeCaret() const;

	//////////////////////////////////////////////////////////////////////////
	// History.
	//////////////////////////////////////////////////////////////////////////
	static bool m_bPreviewOn;
};

#endif // __smartfileopendialog_h__