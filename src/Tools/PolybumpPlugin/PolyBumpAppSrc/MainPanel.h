#pragma once
#include "afxwin.h"

#include "../Properties.h"							// CProperties

class CSRFData;

// MainPanel dialog

class CMainPanel : public CDialog
{
	DECLARE_DYNAMIC(CMainPanel)

public:
	CMainPanel( CProperties &rProperties, CWnd* pParent = NULL );   // standard constructor
	virtual ~CMainPanel();

// Dialog Data
	enum { IDD = IDD_MAINPANEL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedPickhighpoly();
	afx_msg void OnBnClickedPicklowpoly();
	afx_msg void OnBnClickedPickprojpoly();
	afx_msg void OnBnClickedPickcagepoly();
	BOOL OnInitDialog();
	LRESULT OnGetDefID(WPARAM wp, LPARAM lp);

	CEdit m_CageName;
	CEdit m_ProjName;
	CEdit m_LowPolyName;
	CEdit m_HighPolyName;
	CComboBox m_cbHitMode;
	CComboBox m_cbHorizon;
	CComboBox m_cbAntialiasing;
	CComboBox m_cbWidth;
	CComboBox m_cbHeight;
	CEdit m_RayLength;
	CButton m_HighPoly_Smooth;
	CButton m_LowPolyInfoText;			// under construction

private: // ----------------------------------------------------------------

	CProperties	&				m_rProperties;			// reference to the document properties

	// -----------------------------------------------------------------------

	// \param inoutFileName is only changed if the user wasn't canceling the action
	bool OpenOBJFile( const char *szTitle, CString &inoutFileName, const bool bMultipleFiles );

	// get the clamped ray length from the button
	float GetRayLengthFromButton() const;

public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedPreviewinput();
	CButton m_Debug;
	// Thread priority
	CComboBox m_cbThreadPrio;
};
