////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   DialogBrowserDialog.h
//  Version:     v1.00
//  Created:     04 Dec 2008 by Tomas Neumann, based on SoundBrowserDialog by Sergiy Shaykin.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __dialogbrowserdialog_h__
#define __dialogbrowserdialog_h__

#if _MSC_VER > 1000
#pragma once
#endif

// forward declaration.
struct ISound;


struct SDialogCachElem
{
	int m_type;
	unsigned int m_nRow;
	CString m_sKey;
	CString m_sText;
	CString m_sFolder;
	CString m_sEnglishWho;

	SDialogCachElem(int type, unsigned int nRow, const CString & sKey, const CString & sText, const CString & sFolder, const CString & sEnglishWho)
	{
		m_type = type;
		m_nRow = nRow;
		m_sKey = sKey;
		m_sText = sText;
		m_sFolder = sFolder;
		m_sEnglishWho = sEnglishWho;
	}
};



class CDialogBrowserDialog : public CDialog
{
	DECLARE_DYNAMIC(CDialogBrowserDialog)

public:
	CDialogBrowserDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogBrowserDialog();

	void Init(const CString & name);
	bool IsBrowse();
	const CString & GetString();
	void Play(CString name);

// Dialog Data
	enum { IDD = IDD_DIALOG_BROWSER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

protected:
	void CollectCach();
	void FillSoundsTree();

	//////////////////////////////////////////////////////////////////////////
	// FIELDS
	//////////////////////////////////////////////////////////////////////////
	CTreeCtrl m_soundsTree;
	bool m_isBrowse;
	CString m_return;
	CString m_initName;
	static bool m_isAutoPlay;
	static bool m_isDSPEffect;
	bool m_bClearingSoundTree;
	CImageList m_imageListFiles;

	enum eDBD_Sorting
	{
		eDBD_Sorting_NONE = -1,
		eDBD_Sorting_INDEX,
		eDBD_Sorting_SUBFOLDER,
		eDBD_Sorting_CHARACTER
	};

	eDBD_Sorting m_SortingRule1;
	eDBD_Sorting m_SortingRule2;

	int m_buttonLeft;
	int m_treeBot;

	// Preview
	_smart_ptr<ISound> m_pSound;
	ListenerID m_ListenerID;


	static std::vector<SDialogCachElem> m_cachElems;
	static bool m_isCached;

	std::map<HTREEITEM, CString> m_treeMap;

	afx_msg void OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnDoubleClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnRightClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBrowseBtn();
	afx_msg void OnPlayBtn();
	afx_msg void OnAutoPlayBtn();
	afx_msg void OnDSPEffectBtn();
	afx_msg void OnRefreshBtn();
	afx_msg void OnSortChange();
	afx_msg void OnSize(UINT nType, int cx, int cy) ;
};

#endif // __dialogbrowserdialog_h__