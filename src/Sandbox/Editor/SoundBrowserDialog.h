////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   SoundBrowserDialog.h
//  Version:     v1.00
//  Created:     10 Nov 2005 by Sergiy Shaykin.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __soundbrowserdialog_h__
#define __soundbrowserdialog_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include <vector>

// CSoundBrowserDialog dialog

// forward declaration.
struct ISound;
class CSoundEventParameterDialog;

struct SCachElem
{
	int m_type;
	int m_level;
	CString m_name;
	CString m_path;

	SCachElem(int type, int level, const CString & name, const CString & path)
	{
		m_type = type;
		m_level = level;
		m_name = name;
		m_path = path;
	}
};



class CSoundBrowserDialog : public CDialog
{
	DECLARE_DYNAMIC(CSoundBrowserDialog)

public:
	CSoundBrowserDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSoundBrowserDialog();

	void						Init(CString const& name);
	bool						IsBrowse();
	const CString&	GetString();
	void						Play();
	void						HandleParameterSliders(CString const& rsName);
	void						RemoveSliders();

// Dialog Data
	enum { IDD = IDD_SOUND_BROWSER };

protected:
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL		OnInitDialog();

	DECLARE_MESSAGE_MAP()

protected:
	void						CollectCach();
	void						CollectCachEventgroup(XmlNodeRef & gr, const CString & Block, const CString & Path, int level);
	void						FillSoundsTree();

	//////////////////////////////////////////////////////////////////////////
	// FIELDS
	//////////////////////////////////////////////////////////////////////////
	CTreeCtrl m_soundsTree;
	bool m_isBrowse;
	CString m_return;
	CString m_initName;
	static bool m_isAutoPlay;
	CImageList m_imageListFiles;

	int m_buttonLeft;
	int m_treeBot;
	int m_iTreeOffset;
	int m_iOriginalSizeX;
	int m_iOriginalSizeY;

	// Preview
	_smart_ptr<ISound> m_pSound;
	ListenerID m_ListenerID;


	static std::vector<SCachElem> m_cachElems;
	static bool m_isCached;

	std::map<HTREEITEM, CString>	m_treeMap;
	std::vector<CSoundEventParameterDialog*>	m_apSoundEventParameterDialogs;

	afx_msg void OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnDoubleClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnRightClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBrowseBtn();
	afx_msg void OnPlayBtn();
	afx_msg void OnAutoPlayBtn();
	afx_msg void OnRefreshBtn();
	afx_msg void OnSize(UINT nType, int cx, int cy) ;
};

#endif // __soundbrowserdialog_h__