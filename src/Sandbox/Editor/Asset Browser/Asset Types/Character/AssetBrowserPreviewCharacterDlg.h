#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////

class CXTPDockingPaneManager;
class CAssetBrowserPreviewCharacterDlgFooter;

class CAssetBrowserPreviewCharacterDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserPreviewCharacterDlg)

public:
	class	CAssetCharacterItem* m_pModel;

	void Init();
	CAssetBrowserPreviewCharacterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAssetBrowserPreviewCharacterDlg();
	void SetFooter(CAssetBrowserPreviewCharacterDlgFooter* footer)
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
	afx_msg void OnBnClickedButtonSaveThumbAngle();
	DECLARE_MESSAGE_MAP()

private:
	CAssetBrowserPreviewCharacterDlgFooter* m_pFooter;
	CComboBox m_lightingCB;
	std::map<CString, XmlNodeRef> m_lightingMap;
	XmlNodeRef m_oldTod;
	CRect m_originalRect;
	CBitmapButton m_fullscreenBtn;
};