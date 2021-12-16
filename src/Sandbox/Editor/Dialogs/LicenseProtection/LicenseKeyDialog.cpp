// LicenseKeyDialog.cpp : implementation file
//

#include "stdafx.h"
#include "LicenseKeyDialog.h"
#include "EngineSettingsManager.h"

#if defined(IS_PROSDK)

IMPLEMENT_DYNAMIC(CLicenseKeyDialog, CDialog)


//////////////////////////////////////////////////////////////////////////
CLicenseKeyDialog::CLicenseKeyDialog(CWnd* pParent /*=NULL*/) :
CDialog(CLicenseKeyDialog::IDD, pParent),
m_bFirstTry(true)
{
	//{{AFX_DATA_INIT(CLicenseKeyDialog)
	//}}AFX_DATA_INIT
}


//////////////////////////////////////////////////////////////////////////
CLicenseKeyDialog::CLicenseKeyDialog(bool bFirstTry) :
CDialog(CLicenseKeyDialog::IDD, NULL),
m_bFirstTry(bFirstTry)
{
	//{{AFX_DATA_INIT(CLicenseKeyDialog)
	//}}AFX_DATA_INIT
}


//////////////////////////////////////////////////////////////////////////
BOOL CLicenseKeyDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	const char* ManualConfigKeyName = "EDT_LicenseManualConfig";
	const char* ServerIpKeyName = "EDT_LicenseIp";
	const char* ServerPortKeyName = "EDT_LicensePort";
	
	char buffer[1024];
	CEngineSettingsManager esm;

	bool bManualConfig = false;
	if (esm.GetModuleSpecificStringEntryUtf8(ManualConfigKeyName,SettingsManagerHelpers::CCharBuffer(buffer, sizeof(buffer))) && stricmp(buffer, "true") == 0)
		bManualConfig = true;

	if (esm.GetModuleSpecificStringEntryUtf8(ServerIpKeyName,SettingsManagerHelpers::CCharBuffer(buffer, sizeof(buffer))))
		m_eIp.SetWindowText(buffer);

	if (esm.GetModuleSpecificStringEntryUtf8(ServerPortKeyName,SettingsManagerHelpers::CCharBuffer(buffer, sizeof(buffer))))
		m_ePort.SetWindowText(buffer);

	if(bManualConfig)
	{
		m_rMethod2.SetCheck(1);
		OnLicenseMethodChange();

		if (!m_bFirstTry)
			m_lDescription.SetWindowText("Can't obtain valid key from your local key server.\n\nTo request new License Keys, please go to Crytek Knowledge Network.");

		m_eIp.SetFocus();
	}
	else
	{
		m_rMethod1.SetCheck(1);
		OnLicenseMethodChange();

		if (!m_bFirstTry)
			m_lDescription.SetWindowText("The key you entered could not be validated.\nPlease enter an unused license key.\n\nTo request new License Keys, please go to Crytek Knowledge Network.");

		m_eKey1.SetFocus();
	}

	return FALSE;
}


//////////////////////////////////////////////////////////////////////////
void CLicenseKeyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLicenseKeyDialog)
	DDX_Control(pDX, IDOK, m_bOk);

	DDX_Control(pDX, IDC_EDIT1, m_eKey1);
	DDX_Control(pDX, IDC_EDIT2, m_eKey2);
	DDX_Control(pDX, IDC_EDIT3, m_eKey3);
	DDX_Control(pDX, IDC_EDIT4, m_eKey4);
	DDX_Control(pDX, IDC_STATIC1, m_lDescription);
	DDX_Control(pDX, IDC_STATIC3, m_lHelper1);
	DDX_Control(pDX, IDC_RADIO1, m_rMethod1);
	DDX_Control(pDX, IDC_RADIO2, m_rMethod2);

	DDX_Control(pDX, IDC_EDIT5, m_ePort);
	DDX_Control(pDX, IDC_STATIC4, m_eIp);
	//}}AFX_DATA_MAP
}


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CLicenseKeyDialog, CDialog)
	//{{AFX_MSG_MAP(CLicenseKeyDialog)
	// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK,&OnOkButtonClicked)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
	ON_EN_CHANGE(IDC_EDIT2, OnChangeEdit2)
	ON_EN_CHANGE(IDC_EDIT3, OnChangeEdit3)
	ON_EN_CHANGE(IDC_EDIT4, OnChangeEdit4)
	ON_BN_CLICKED(IDC_RADIO1, OnLicenseMethodChange)
	ON_BN_CLICKED(IDC_RADIO2, OnLicenseMethodChange)
	ON_STN_CLICKED(IDC_STATIC1, &CLicenseKeyDialog::OnStnClickedStatic1)
END_MESSAGE_MAP()


void CLicenseKeyDialog::OnLicenseMethodChange()
{
	if(m_rMethod1.GetCheck())
	{
		// Global License Key selected
		m_bIsUsingGlobalKey = true;
		m_lDescription.SetWindowText("Could not find the license key, needed to run your Sandbox copy.\nPlease enter the key here.\n\nTo request new License Keys, please go to Crytek Knowledge Network.");
	}
	else
	{
		// Local License Server selected
		m_bIsUsingGlobalKey = false;
		m_lDescription.SetWindowText("Please enter your server details here.\n\nTo request new License Keys, please go to Crytek Knowledge Network.");
	}
	m_eKey1.ShowWindow(m_bIsUsingGlobalKey);
	m_eKey1.ShowWindow(m_bIsUsingGlobalKey);
	m_eKey2.ShowWindow(m_bIsUsingGlobalKey);
	m_eKey3.ShowWindow(m_bIsUsingGlobalKey);
	m_eKey4.ShowWindow(m_bIsUsingGlobalKey);

	m_eIp.ShowWindow(!m_bIsUsingGlobalKey);
	m_ePort.ShowWindow(!m_bIsUsingGlobalKey);
	m_lHelper1.ShowWindow(!m_bIsUsingGlobalKey);
}

//////////////////////////////////////////////////////////////////////////
void CLicenseKeyDialog::OnOkButtonClicked()
{
	CEngineSettingsManager esm;

	if(m_bIsUsingGlobalKey)
	{
		CString t1, t2, t3, t4;
		m_eKey1.GetWindowText(t1);
		m_eKey2.GetWindowText(t2);
		m_eKey3.GetWindowText(t3);
		m_eKey4.GetWindowText(t4);

		if (t1.GetLength()<4 || t2.GetLength()<4 || t3.GetLength()<4 || t4.GetLength()<4)
			return;

		string theKey = t1 +"-"+ t2 +"-"+ t3 +"-"+ t4;

		wchar_t keybuffer[1024];
		SettingsManagerHelpers::ConvertUtf8ToUtf16(theKey.c_str(), SettingsManagerHelpers::CWCharBuffer(keybuffer, sizeof(keybuffer)));
		esm.SetKey("EDT_InstanceKey", keybuffer);
	}
	else
	{
		CString ip, port;
		m_eIp.GetWindowText(ip);
		m_ePort.GetWindowText(port);

		if (ip.GetLength()<4 || port.GetLength()<2)
			return;

		string sIp = ip;
		string sPort = port;

		wchar_t ipbuffer[1024];
		wchar_t portbuffer[512];
		SettingsManagerHelpers::ConvertUtf8ToUtf16(sIp.c_str(), SettingsManagerHelpers::CWCharBuffer(ipbuffer, sizeof(ipbuffer)));
		SettingsManagerHelpers::ConvertUtf8ToUtf16(sPort.c_str(), SettingsManagerHelpers::CWCharBuffer(portbuffer, sizeof(portbuffer)));

		esm.SetKey("EDT_LicenseIp", ipbuffer);
		esm.SetKey("EDT_LicensePort", portbuffer);
	}
	esm.SetKey("EDT_LicenseManualConfig", !m_bIsUsingGlobalKey);
	esm.StoreData();

	EndDialog(IDOK);
}


//////////////////////////////////////////////////////////////////////////
void CLicenseKeyDialog::OnChangeEdit1()
{
	CString t1,tRem, t2;
	m_eKey1.GetWindowText(t1);	
	t1.Remove('-');
	t1.Remove(' ');
	if (t1.GetLength()==4)
		m_eKey2.SetFocus();

	if (t1.GetLength()>4)
	{
		tRem = (LPCSTR)&(t1.GetBuffer()[4]);
		t1.Truncate(4);
		m_eKey1.SetWindowText(t1);
		m_eKey2.GetWindowText(t2);
		t2 = tRem + t2;
		m_eKey2.SetWindowText(t2);
	}
}


//////////////////////////////////////////////////////////////////////////
void CLicenseKeyDialog::OnChangeEdit2()
{
	CString t2, tRem, t3;
	m_eKey2.GetWindowText(t2);
	t2.Remove('-');
	t2.Remove(' ');
	if (t2.GetLength()==4)
		m_eKey3.SetFocus();

	if (t2.GetLength()>4)
	{
		tRem = (LPCSTR)&(t2.GetBuffer()[4]);
		t2.Truncate(4);
		m_eKey2.SetWindowText(t2);
		m_eKey3.GetWindowText(t3);
		t3 = tRem + t3;
		m_eKey3.SetWindowText(t3);
	}
}


//////////////////////////////////////////////////////////////////////////
void CLicenseKeyDialog::OnChangeEdit3()
{
	CString t3,tRem, t4;
	m_eKey3.GetWindowText(t3);	
	t3.Remove('-');
	t3.Remove(' ');
	if (t3.GetLength()==4)
		m_eKey4.SetFocus();

	if (t3.GetLength()>4)
	{
		tRem = (LPCSTR)&(t3.GetBuffer()[4]);
		t3.Truncate(4);
		m_eKey3.SetWindowText(t3);
		m_eKey4.GetWindowText(t4);
		t4 = tRem + t4;
		m_eKey4.SetWindowText(t4);
	}
}


//////////////////////////////////////////////////////////////////////////
void CLicenseKeyDialog::OnChangeEdit4()
{
	CString t4;
	m_eKey4.GetWindowText(t4);
	t4.Remove('-');
	t4.Remove(' ');
	if (t4.GetLength()==4)
		m_bOk.SetFocus();

	if (t4.GetLength()>4)
	{
		t4.Truncate(4);
		m_eKey4.SetWindowText(t4);
	}
}

void CLicenseKeyDialog::OnStnClickedStatic1()
{
	// TODO: Add your control notification handler code here
}
#endif 


