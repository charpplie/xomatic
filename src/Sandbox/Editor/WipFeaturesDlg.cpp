// WipFeaturesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WipFeaturesDlg.h"

#ifdef USE_WIP_FEATURES_MANAGER

// CWipFeaturesDlg dialog

IMPLEMENT_DYNAMIC(CWipFeaturesDlg, CDialog)

CWipFeaturesDlg::CWipFeaturesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWipFeaturesDlg::IDD, pParent)
{

}

CWipFeaturesDlg::~CWipFeaturesDlg()
{
}

void CWipFeaturesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FEATURES, m_lstFeatures);
}


BEGIN_MESSAGE_MAP(CWipFeaturesDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_SHOW, &CWipFeaturesDlg::OnBnClickedButtonShow)
	ON_BN_CLICKED(IDC_BUTTON_HIDE, &CWipFeaturesDlg::OnBnClickedButtonHide)
	ON_BN_CLICKED(IDC_BUTTON_ENABLE, &CWipFeaturesDlg::OnBnClickedButtonEnable)
	ON_BN_CLICKED(IDC_BUTTON_DISABLE, &CWipFeaturesDlg::OnBnClickedButtonDisable)
	ON_BN_CLICKED(IDC_BUTTON_SAFEMODE, &CWipFeaturesDlg::OnBnClickedButtonSafemode)
	ON_BN_CLICKED(IDC_BUTTON_NORMALMODE, &CWipFeaturesDlg::OnBnClickedButtonNormalmode)
	ON_BN_CLICKED(IDCANCEL, &CWipFeaturesDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CWipFeaturesDlg message handlers

BOOL CWipFeaturesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	#define LVS_EX_AUTOSIZECOLUMNS 0x10000000
	m_lstFeatures.SetExtendedStyle( LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_AUTOSIZECOLUMNS );
	m_lstFeatures.InsertColumn( 0, "Name", LVCFMT_LEFT, 300 );
	m_lstFeatures.InsertColumn( 1, "Id", LVCFMT_CENTER, 70 );
	m_lstFeatures.InsertColumn( 2, "Visible", LVCFMT_CENTER, 70 );
	m_lstFeatures.InsertColumn( 3, "Enabled", LVCFMT_CENTER, 70 );
	m_lstFeatures.InsertColumn( 4, "SafeMode", LVCFMT_CENTER, 70 );

	FillFeatures();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CWipFeaturesDlg::FillFeatures()
{
	// just gather selected IDs first, so we reselect them after clear & fill
	GatherSelectedFeatures( m_selectedFeatureIds );
	m_lstFeatures.DeleteAllItems();

	int i = 0;

	for(	CWipFeatureManager::TWipFeatures::iterator iter = CWipFeatureManager::Instance()->GetFeatures().begin(), 
				iterEnd = CWipFeatureManager::Instance()->GetFeatures().end(); iter != iterEnd; ++iter )
	{
		CWipFeatureManager::SWipFeatureInfo& feat = iter->second;
		CString strId;
		
		strId.Format( "%d", feat.m_id );
		m_lstFeatures.InsertItem( i, feat.m_displayName.c_str() );
		m_lstFeatures.SetItemText( i, 1, strId );
		m_lstFeatures.SetItemText( i, 2, feat.m_bVisible ? "X" : "" );
		m_lstFeatures.SetItemText( i, 3, feat.m_bEnabled ? "X" : "" );
		m_lstFeatures.SetItemText( i, 4, feat.m_bSafeMode ? "X" : "" );

		if( std::find( m_selectedFeatureIds.begin(), m_selectedFeatureIds.end(), feat.m_id ) != m_selectedFeatureIds.end() )
		{
			m_lstFeatures.SetItemState( i, LVIS_SELECTED, LVIS_SELECTED );
		}

		++i;
	}
}

void CWipFeaturesDlg::GatherSelectedFeatures( std::vector<int>& rFeatIds )
{
	int i = 0;
	rFeatIds.clear();

	for(	CWipFeatureManager::TWipFeatures::iterator iter = CWipFeatureManager::Instance()->GetFeatures().begin(), 
				iterEnd = CWipFeatureManager::Instance()->GetFeatures().end(); iter != iterEnd; ++iter )
	{
		CWipFeatureManager::SWipFeatureInfo& feat = iter->second;

		if( m_lstFeatures.GetItemState( i, LVIS_SELECTED ) & LVIS_SELECTED )
		{
			rFeatIds.push_back( feat.m_id );
		}

		++i;
	}
}

void CWipFeaturesDlg::OnBnClickedButtonShow()
{
	GatherSelectedFeatures( m_selectedFeatureIds );

	for( size_t i = 0, iCount = m_selectedFeatureIds.size(); i < iCount; ++i )
	{
		CWipFeatureManager::Instance()->ShowFeature( m_selectedFeatureIds[i], true );
	}

	FillFeatures();
}

void CWipFeaturesDlg::OnBnClickedButtonHide()
{
	GatherSelectedFeatures( m_selectedFeatureIds );

	for( size_t i = 0, iCount = m_selectedFeatureIds.size(); i < iCount; ++i )
	{
		CWipFeatureManager::Instance()->ShowFeature( m_selectedFeatureIds[i], false );
	}

	FillFeatures();
}

void CWipFeaturesDlg::OnBnClickedButtonEnable()
{
	GatherSelectedFeatures( m_selectedFeatureIds );

	for( size_t i = 0, iCount = m_selectedFeatureIds.size(); i < iCount; ++i )
	{
		CWipFeatureManager::Instance()->EnableFeature( m_selectedFeatureIds[i], true );
	}

	FillFeatures();
}

void CWipFeaturesDlg::OnBnClickedButtonDisable()
{
	GatherSelectedFeatures( m_selectedFeatureIds );

	for( size_t i = 0, iCount = m_selectedFeatureIds.size(); i < iCount; ++i )
	{
		CWipFeatureManager::Instance()->EnableFeature( m_selectedFeatureIds[i], false );
	}

	FillFeatures();
}

void CWipFeaturesDlg::OnBnClickedButtonSafemode()
{
	GatherSelectedFeatures( m_selectedFeatureIds );

	for( size_t i = 0, iCount = m_selectedFeatureIds.size(); i < iCount; ++i )
	{
		CWipFeatureManager::Instance()->SetFeatureSafeMode( m_selectedFeatureIds[i], true );
	}

	FillFeatures();
}

void CWipFeaturesDlg::OnBnClickedButtonNormalmode()
{
	GatherSelectedFeatures( m_selectedFeatureIds );

	for( size_t i = 0, iCount = m_selectedFeatureIds.size(); i < iCount; ++i )
	{
		CWipFeatureManager::Instance()->SetFeatureSafeMode( m_selectedFeatureIds[i], false );
	}

	FillFeatures();
}

void CWipFeaturesDlg::OnBnClickedCancel()
{
	OnCancel();
}

#endif // USE_WIP_FEATURES_MANAGER
