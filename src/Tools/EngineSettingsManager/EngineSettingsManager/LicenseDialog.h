#ifndef __License_Dialog__h__
#define __License_Dialog__h__

#include "resource.h"

class CEngineSettingsManager;

class CLicenseDialog
	: public CDialog
{
	DECLARE_DYNAMIC( CLicenseDialog )

public:
	CLicenseDialog( CWnd* pParent = NULL );
	enum { IDD = IDD_LICENSE_DIALOG };

	virtual void OnOK() {}

	void Create( UINT nIDTemplate, CWnd* pParentWnd, CEngineSettingsManager* pSettingsManager );

	bool UsingManualConfiguration() const;

	void SetLicenseInUi( const CString& licenseKey );
	CString GetLicenseFromUi();

	void SetPortInUi( const CString& port );
	CString GetPortFromUi();

	void SetIpInUi( const CString& ip );
	CString GetIpFromUi();

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );

	afx_msg void OnManualButtonClicked();
	afx_msg void OnMyCryEngineButtonClicked();
	afx_msg void OnIpAddressChanged();
	afx_msg void OnPortChanged();
	afx_msg void OnLicenseKey0Changed();
	afx_msg void OnLicenseKey1Changed();
	afx_msg void OnLicenseKey2Changed();
	afx_msg void OnLicenseKey3Changed();
	DECLARE_MESSAGE_MAP()

	void UpdateRadioBoxInterface();

	void OnLicenseKeyChanged( int keyId );

	void SetValidationStatus( bool success, CString text );

	void UiDataChanged();
	void ValidateWithServer();

private:
	CButton m_manual;
	CButton m_myCryEngine;

	CIPAddressCtrl m_ipAddress;
	CEdit m_port;

	CEdit m_licenseKey[ 4 ];

	CStatic m_validationStatusImage;
	CStatic m_validationStatusText;

	bool m_usingManualConfiguration;

	CEngineSettingsManager* m_pSettingsManager;
};

#endif