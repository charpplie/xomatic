#pragma once
#include "EditorLiveCreate.h"

#ifndef NO_LIVECREATE

namespace LiveCreate
{
	class CBGTask_ScanBuilds;
	class CBGTask_ResolveAddress;
}

class CLiveCreateEditConnectionDlg : public CDialog
{
	DECLARE_DYNAMIC(CLiveCreateEditConnectionDlg)

public:
	struct Parameters
	{
		Parameters()
			: bAdding(false)
			, bIsEnabled(true)
			, pPlatform(NULL)
		{}

		bool bAdding;
		bool bIsEnabled;
		LiveCreate::IPlatformHandler* pPlatform;
		CString platformName;
		CString targetName;
		CString address;
		CString buildExecutable;
		CString buildDirectory;
	};

	CLiveCreateEditConnectionDlg(CWnd* pParent = NULL);
	virtual ~CLiveCreateEditConnectionDlg();

	void SetParameters(const Parameters& rParams);
	const Parameters& GetParameters();

	// Dialog Data
	enum { IDD = IDD_LIVECREATE_EDIT_CONNECTION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonTestConnection();
	afx_msg void OnBnClickedButtonRefreshIp();
	afx_msg void OnCbnSelchangeComboPlatform();
	afx_msg void OnBnClickedCheckEnableHost();
	afx_msg void OnBnBrowseGameFolder();

	void UpdateBuildControls();

	CButton m_chkEnabled;
	CComboBox m_cbPlatform;
	CEdit m_edTargetName;
	CIPAddressCtrl m_edIpAddress;
	CEdit m_edBuildPath;
	CEdit m_edBuildExecutable;
	CButton m_btnPickDirectory;
	CButton m_btnRefreshIP;

	Parameters m_params;

	LiveCreate::IPlatformHandlerFactory* m_pFactory;
	LiveCreate::IPlatformHandler* m_pPlatform;

	static const UINT_PTR kTimerRefreshBuilds = 12345;
	static const UINT_PTR kTimerRefreshAddress = 12346;
};

#endif