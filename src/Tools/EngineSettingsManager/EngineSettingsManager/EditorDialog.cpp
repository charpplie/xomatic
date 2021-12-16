#include "stdafx.h"
#include "EngineSettingsManager.h"
#include "EditorDialog.h"


IMPLEMENT_DYNAMIC(CEditorDialog, CDialog)


//////////////////////////////////////////////////////////////////////////
CEditorDialog::CEditorDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CEditorDialog::IDD, pParent), m_pSettingsManager(NULL)
{
}


//////////////////////////////////////////////////////////////////////////
void CEditorDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_32BIT_CHECK, m_bPrefer32Bit);
	DDX_Control(pDX, IDC_EXPORT_KEY_0, m_exportKey[ 0 ] );
	DDX_Control(pDX, IDC_EXPORT_KEY_1, m_exportKey[ 1 ] );
	DDX_Control(pDX, IDC_EXPORT_KEY_2, m_exportKey[ 2 ] );
	DDX_Control(pDX, IDC_EXPORT_KEY_3, m_exportKey[ 3 ] );
	DDX_Control(pDX, IDC_RESET_BUTTON, m_exportKeyReset);
	DDX_Control(pDX, IDC_EXPORT_KEY_GROUP, m_exportKeyStatic);
}


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CEditorDialog, CDialog)
	ON_EN_CHANGE( IDC_EXPORT_KEY_0, OnExportKey0Changed )
	ON_EN_CHANGE( IDC_EXPORT_KEY_1, OnExportKey1Changed )
	ON_EN_CHANGE( IDC_EXPORT_KEY_2, OnExportKey2Changed )
	ON_EN_CHANGE( IDC_EXPORT_KEY_3, OnExportKey3Changed )
	ON_BN_CLICKED( IDC_RESET_BUTTON, OnResetButtonClicked )
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
void CEditorDialog::Create(UINT temp, CWnd* parent, CEngineSettingsManager* settingsManager)
{
	m_pSettingsManager = settingsManager;
	CDialog::Create(temp, parent);
}


//////////////////////////////////////////////////////////////////////////
BOOL CEditorDialog::OnInitDialog()
{
	CDialog::OnInitDialog();	
	m_bPrefer32Bit.SetCheck(m_pSettingsManager->GetValue<string>("EDT_Prefer32Bit")=="true" ? BST_CHECKED : BST_UNCHECKED);






	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
bool CEditorDialog::IsPrefer32Bit()
{
	return m_bPrefer32Bit.GetCheck() == BST_CHECKED;
}

void CEditorDialog::OnExportKey0Changed()
{	
	OnExportKeyChanged( 0 );
}

void CEditorDialog::OnExportKey1Changed()
{
	OnExportKeyChanged( 1 );
}

void CEditorDialog::OnExportKey2Changed()
{
	OnExportKeyChanged( 2 );
}

void CEditorDialog::OnExportKey3Changed()
{
	OnExportKeyChanged( 3 );
}

void CEditorDialog::OnExportKeyChanged( int keyId )
{
	CString t1;

	m_exportKey[ keyId ].GetWindowText( t1 );
	t1.Remove( '-' );
	t1.Remove( ' ' );

	if ( t1.GetLength() == 4 && ( keyId + 1 < 4 ) )
	{
		m_exportKey[ keyId + 1 ].SetFocus();
	}

	if ( t1.GetLength() > 4 )
	{
		CString tRem;
		tRem = ( LPCSTR )( & t1.GetBuffer()[ 4 ] );
		t1.Truncate( 4 );
		m_exportKey[ keyId ].SetWindowText( t1 );

		if ( keyId + 1 < 4 )
		{
			CString t2;
			m_exportKey[ keyId + 1 ].GetWindowText( t2 );
			t2 = tRem + t2;
			m_exportKey[ keyId + 1 ].SetWindowText( t2 );
		}
	}
	//UiDataChanged();
}

void CEditorDialog::OnResetButtonClicked()
{
	for(int i=0; i<4; ++i)
		m_exportKey[i].SetWindowText("");
}

CString CEditorDialog::GetLevelExportKeyFromUi()
{
	CString exportKey;
	CString subKey[ 4 ];
	for ( int i = 0; i < 4; i++ )
	{
		m_exportKey[ i ].GetWindowText( subKey[ i ] );
		if ( subKey[ i ].GetLength() < 4 )
		{
			return "";
		}
	}

	exportKey.Format( "%s-%s-%s-%s", subKey[ 0 ], subKey[ 1 ], subKey[ 2 ], subKey[ 3 ] );
	return exportKey;
}


void CEditorDialog::SetLevelExportControlsVisible()
{








}
