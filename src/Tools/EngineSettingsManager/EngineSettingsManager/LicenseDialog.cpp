#include "stdafx.h"
#include "LicenseDialog.h"

#include "EngineSettingsManager.h"







IMPLEMENT_DYNAMIC( CLicenseDialog, CDialog )

BEGIN_MESSAGE_MAP( CLicenseDialog, CDialog )
	ON_BN_CLICKED( IDC_LICENSE_MANUAL, OnManualButtonClicked )
	ON_BN_CLICKED( IDC_LICENSE_MYCRYENGINE, OnMyCryEngineButtonClicked )
	ON_BN_CLICKED( IDC_AUTHENTICATE_BUTTON, ValidateWithServer )
	ON_EN_CHANGE( IDC_LICENSE_IPADDRESS, OnIpAddressChanged )
	ON_EN_CHANGE( IDC_LICENSE_PORT, OnPortChanged )
	ON_EN_CHANGE( IDC_LICENSE_KEY_0, OnLicenseKey0Changed )
	ON_EN_CHANGE( IDC_LICENSE_KEY_1, OnLicenseKey1Changed )
	ON_EN_CHANGE( IDC_LICENSE_KEY_2, OnLicenseKey2Changed )
	ON_EN_CHANGE( IDC_LICENSE_KEY_3, OnLicenseKey3Changed )
END_MESSAGE_MAP()

CLicenseDialog::CLicenseDialog( CWnd* pParent )
: CDialog( IDD, pParent )
, m_pSettingsManager( NULL )
{

}

void CLicenseDialog::Create( UINT nIDTemplate, CWnd* pParentWnd, CEngineSettingsManager* pSettingsManager )
{
	m_pSettingsManager = pSettingsManager;
	CDialog::Create( nIDTemplate, pParentWnd );
}


BOOL CLicenseDialog::OnInitDialog()
{
	__super::OnInitDialog();

	m_usingManualConfiguration = ( m_pSettingsManager->GetValue< tstring >( "EDT_LicenseManualConfig" ) == "true" );
	CString ipAddress = m_pSettingsManager->GetValue< tstring >( "EDT_LicenseIp" ).c_str();
	CString port = m_pSettingsManager->GetValue< tstring >( "EDT_LicensePort" ).c_str();
	CString licenseKey = m_pSettingsManager->GetValue< tstring >( "EDT_InstanceKey" ).c_str();

	m_myCryEngine.EnableWindow( TRUE );

	UpdateRadioBoxInterface();
	SetLicenseInUi( licenseKey );
	SetIpInUi( ipAddress );
	SetPortInUi( port );

	//ValidateWithServer();

	return TRUE;
}

bool CLicenseDialog::UsingManualConfiguration() const
{
	return m_usingManualConfiguration;
}

void CLicenseDialog::SetLicenseInUi( const CString& licenseKey )
{
	m_licenseKey[ 0 ].SetWindowText( licenseKey );
}

CString CLicenseDialog::GetLicenseFromUi()
{
	CString licenseKey;
	CString subKey[ 4 ];
	for ( int i = 0; i < 4; i++ )
	{
		m_licenseKey[ i ].GetWindowText( subKey[ i ] );
		if ( subKey[ i ].GetLength() < 4 )
		{
			return "";
		}
	}

	licenseKey.Format( "%s-%s-%s-%s", subKey[ 0 ], subKey[ 1 ], subKey[ 2 ], subKey[ 3 ] );
	return licenseKey;
}

void CLicenseDialog::SetPortInUi( const CString& port )
{
	m_port.SetWindowText( port );
}

CString CLicenseDialog::GetPortFromUi()
{
	CString port;
	m_port.GetWindowText( port );
	return port;
}

void CLicenseDialog::SetIpInUi( const CString& ip )
{
	m_ipAddress.SetWindowText( ip );
}

CString CLicenseDialog::GetIpFromUi()
{
	byte field[ 4 ];
	m_ipAddress.GetAddress( field[ 0 ], field[ 1 ], field[ 2 ], field[ 3 ] );
	CString ipAddress;
	ipAddress.Format( "%d.%d.%d.%d", field[ 0 ], field[ 1 ], field[ 2 ], field[ 3 ] );
	return ipAddress;
}

void CLicenseDialog::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_LICENSE_MANUAL, m_manual );
	DDX_Control( pDX, IDC_LICENSE_MYCRYENGINE, m_myCryEngine );

	DDX_Control( pDX, IDC_LICENSE_IPADDRESS, m_ipAddress );
	DDX_Control( pDX, IDC_LICENSE_PORT, m_port );

	DDX_Control( pDX, IDC_LICENSE_KEY_0, m_licenseKey[ 0 ] );
	DDX_Control( pDX, IDC_LICENSE_KEY_1, m_licenseKey[ 1 ] );
	DDX_Control( pDX, IDC_LICENSE_KEY_2, m_licenseKey[ 2 ] );
	DDX_Control( pDX, IDC_LICENSE_KEY_3, m_licenseKey[ 3 ] );

	DDX_Control( pDX, IDC_LICENSE_VALIDATION_STATUS_IMAGE, m_validationStatusImage );
	DDX_Control( pDX, IDC_LICENSE_VALIDATION_STATUS_TEXT, m_validationStatusText );
}

void CLicenseDialog::UpdateRadioBoxInterface()
{
	m_manual.SetCheck( m_usingManualConfiguration ? BST_CHECKED : BST_UNCHECKED );
	m_myCryEngine.SetCheck( m_usingManualConfiguration ? BST_UNCHECKED : BST_CHECKED );

	m_ipAddress.EnableWindow( m_usingManualConfiguration );
	m_port.EnableWindow( m_usingManualConfiguration );

	for ( int i = 0; i < 4; i++ )
	{
		m_licenseKey[ i ].EnableWindow( ! m_usingManualConfiguration );
	}
}

void CLicenseDialog::OnManualButtonClicked()
{
	m_usingManualConfiguration = true;
	UpdateRadioBoxInterface();
	UiDataChanged();
}

void CLicenseDialog::OnMyCryEngineButtonClicked()
{
	m_usingManualConfiguration = false;
	UpdateRadioBoxInterface();
	UiDataChanged();
}

void CLicenseDialog::OnIpAddressChanged()
{
	UiDataChanged();
}

void CLicenseDialog::OnPortChanged()
{
	UiDataChanged();
}

void CLicenseDialog::OnLicenseKeyChanged( int keyId )
{
	CString t1;

	m_licenseKey[ keyId ].GetWindowText( t1 );
	t1.Remove( '-' );
	t1.Remove( ' ' );

	if ( t1.GetLength() == 4 && ( keyId + 1 < 4 ) )
	{
		m_licenseKey[ keyId + 1 ].SetFocus();
	}

	if ( t1.GetLength() > 4 )
	{
		CString tRem;
		tRem = ( LPCSTR )( & t1.GetBuffer()[ 4 ] );
		t1.Truncate( 4 );
		m_licenseKey[ keyId ].SetWindowText( t1 );

		if ( keyId + 1 < 4 )
		{
			CString t2;
			m_licenseKey[ keyId + 1 ].GetWindowText( t2 );
			t2 = tRem + t2;
			m_licenseKey[ keyId + 1 ].SetWindowText( t2 );
		}
	}
	UiDataChanged();
}

void CLicenseDialog::OnLicenseKey0Changed()
{	
	OnLicenseKeyChanged( 0 );
}

void CLicenseDialog::OnLicenseKey1Changed()
{
	OnLicenseKeyChanged( 1 );
}

void CLicenseDialog::OnLicenseKey2Changed()
{
	OnLicenseKeyChanged( 2 );
}

void CLicenseDialog::OnLicenseKey3Changed()
{
	OnLicenseKeyChanged( 3 );
}

void CLicenseDialog::SetValidationStatus( bool success, CString text )
{
	int iconResource = ( success ? IDI_GREEN_ICON : IDI_RED_ICON );
	HICON icon = ( HICON )LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( iconResource ), IMAGE_ICON, 0, 0, 0 );
	m_validationStatusImage.SetIcon( icon );
	m_validationStatusText.SetWindowText( text );
}

void CLicenseDialog::UiDataChanged()
{
	SetValidationStatus( false, "Not validated." );
}

void CLicenseDialog::ValidateWithServer()
{





















	SetValidationStatus( false, "Validation failed." );


}