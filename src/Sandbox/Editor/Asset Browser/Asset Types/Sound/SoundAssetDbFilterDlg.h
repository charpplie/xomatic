#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "IAssetItemDatabase.h"

// CSoundAssetDbFilterDlg dialog

class CSoundAssetDbFilterDlg : public CDialog
{
	DECLARE_DYNAMIC(CSoundAssetDbFilterDlg)

public:
	CSoundAssetDbFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSoundAssetDbFilterDlg();

	void UpdateFilterUI();
	void ApplyFilter();
	void SetAssetViewer(struct IAssetViewer* pViewer)
	{
		m_pAssetViewer = pViewer;
	}

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_SOUND_DB_FILTER };

protected:
	struct IAssetViewer* m_pAssetViewer;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_cbMinLen;
	CComboBox m_cbMaxLen;
	afx_msg void OnCbnSelchangeComboMinimumLength();
	afx_msg void OnCbnSelchangeComboMaximumLength();
	afx_msg void OnBnClickedCheckLoopingSounds();
	virtual BOOL OnInitDialog();
};
