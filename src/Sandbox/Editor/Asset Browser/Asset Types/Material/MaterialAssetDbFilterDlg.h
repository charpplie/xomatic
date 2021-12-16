#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "IAssetItemDatabase.h"

// CMaterialAssetDbFilterDlg dialog

class CMaterialAssetDbFilterDlg : public CDialog
{
	DECLARE_DYNAMIC(CMaterialAssetDbFilterDlg)

public:
	CMaterialAssetDbFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMaterialAssetDbFilterDlg();

	void UpdateFilterUI();
	void ApplyFilter();
	void SetAssetViewer(struct IAssetViewer* pViewer)
	{
		m_pAssetViewer = pViewer;
	}

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_MATERIAL_DB_FILTER };

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

protected:
	struct IAssetViewer* m_pAssetViewer;
};
