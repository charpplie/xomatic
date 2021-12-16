#pragma once
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
class CXTPDockingPaneManager;
class CAssetBrowserPreviewModelDlgFooter;

class CAssetBrowserPreviewModelDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserPreviewModelDlg)

public:
	class	CAssetModelItem* m_pModel;

	void Init();
	CAssetBrowserPreviewModelDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAssetBrowserPreviewModelDlg();
	void SetFooter(CAssetBrowserPreviewModelDlgFooter* footer)
	{
		m_pFooter = footer;
	}

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_PREVIEW_MODEL };

protected:
	CXTPDockingPaneManager* GetDockingManager();
	CXTPDockingPane* GetDockPane(CXTPDockingPaneManager* pDockingManager);
	void DockViewPane();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonWireframe();
	afx_msg void OnBnClickedButtonPhysics();
	afx_msg void OnBnClickedButtonNormals();
	afx_msg void OnBnClickedButtonResetView();
	afx_msg void OnBnClickedButtonFullscreen();
	afx_msg void OnLightingChanged();
	DECLARE_MESSAGE_MAP()

private:
	CAssetBrowserPreviewModelDlgFooter* m_pFooter;
	CComboBox m_lightingCB;
	std::map<CString, XmlNodeRef> m_lightingMap;
	XmlNodeRef m_oldTod;
	CRect m_originalRect;
	CBitmapButton m_fullscreenBtn;
public:
	afx_msg void OnBnClickedButtonSaveThumbAngle();
};