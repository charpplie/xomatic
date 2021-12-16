#if !defined(AFX_LOWMESHDATADLG_H__2F1247DD_AF55_425E_ADC5_6DF876DA9377__INCLUDED_)
#define AFX_LOWMESHDATADLG_H__2F1247DD_AF55_425E_ADC5_6DF876DA9377__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LowMeshDataDlg.h : header file
//

#include "resource.h"					// IDD_

/////////////////////////////////////////////////////////////////////////////
// CLowMeshDataDlg dialog

class CLowMeshDataDlg : public CDialog
{
// Construction
public:
	CLowMeshDataDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLowMeshDataDlg)
	enum { IDD = IDD_LOWMESHDATA };
	CComboBox	m_ChannelNo;
	CComboBox m_MaterialId;
	//}}AFX_DATA

	int		m_iChannelNo;						//!< chosen channel no (valid after Ok)
	int		m_iMaterialId;					//!< chosen material id (valid after Ok)

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLowMeshDataDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLowMeshDataDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOWMESHDATADLG_H__2F1247DD_AF55_425E_ADC5_6DF876DA9377__INCLUDED_)
