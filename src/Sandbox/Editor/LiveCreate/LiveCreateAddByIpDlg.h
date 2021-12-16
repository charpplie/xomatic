#pragma once
#include "EditorLiveCreate.h"

#ifndef NO_LIVECREATE

class CLiveCreateAddByIpDlg : public CDialog
{
	DECLARE_DYNAMIC(CLiveCreateAddByIpDlg)

	CString m_ip;

public:
	CLiveCreateAddByIpDlg(CWnd* pParent = NULL);
	virtual ~CLiveCreateAddByIpDlg();

	const CString& GetIP() { return m_ip; }

	// Dialog Data
	enum { IDD = IDD_LIVECREATE_ADD_BY_IP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonTestConnection();

	bool CheckValidIP() const;

	CIPAddressCtrl m_edIpAddress;
};

#endif