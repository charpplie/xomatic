#pragma once
#include "afxwin.h"

class CSimpleIndexedMesh;

// CLowPolySettingsDialog dialog

class CLowPolySettingsDialog : public CDialog
{
	DECLARE_DYNAMIC(CLowPolySettingsDialog)

public:
	CLowPolySettingsDialog( CSimpleIndexedMesh &rMesh, CWnd* pParent = NULL);   // standard constructor
	virtual ~CLowPolySettingsDialog();

	CString GetMaterial() const;

// Dialog Data
	enum { IDD = IDD_LOWPOLYSETTINGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:

	CComboBox m_LowPolyMaterial;

	CString										m_sMaterial;		//
	CSimpleIndexedMesh &			m_rMesh;				//

	afx_msg void OnBnClickedOk();
};
