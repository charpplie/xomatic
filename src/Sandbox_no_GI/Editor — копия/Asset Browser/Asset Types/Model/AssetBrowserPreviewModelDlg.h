////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserPreviewModelDlg.h
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	Defines the preview model dialog panel, used for previewing
//								the current selected model asset
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __AssetBrowserPreviewModelDlg_h__
#define __AssetBrowserPreviewModelDlg_h__
#pragma once

// CAssetBrowserPreviewModelDlg dialog

class CAssetBrowserPreviewModelDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserPreviewModelDlg)

public:
	class	CAssetModelItem* m_pModel;

	void Init();
	
	CAssetBrowserPreviewModelDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAssetBrowserPreviewModelDlg();

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_PREVIEW_MODEL };

protected:

	afx_msg void OnBnClickedButtonWireframe();
	afx_msg void OnBnClickedButtonResetView();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

#endif //__AssetBrowserPreviewModelDlg_h__