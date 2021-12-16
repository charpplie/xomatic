#pragma once


// CReconstructionOptionsDialog dialog

class CReconstructionOptionsDialog : public CDialog
{
	DECLARE_DYNAMIC(CReconstructionOptionsDialog)

public:
	CReconstructionOptionsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CReconstructionOptionsDialog();

	ftype		m_fNormalMapScale;
	ftype		m_fDetailScale;
	int			m_bShowFeaturesOnly;
	int			m_bUseSingleStereoPhoto;
	int			m_bUseDoubleStereoPhoto;

// Dialog Data
	enum { IDD = IDD_DIALOGBAR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
public:
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
};
