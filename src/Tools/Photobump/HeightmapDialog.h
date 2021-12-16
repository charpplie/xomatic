#pragma once


// CHeightmapDialog dialog

class CHeightmapDialog : public CDialog
{
	DECLARE_DYNAMIC(CHeightmapDialog)

public:
	CHeightmapDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CHeightmapDialog();

	int m_nIterations;

// Dialog Data
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
