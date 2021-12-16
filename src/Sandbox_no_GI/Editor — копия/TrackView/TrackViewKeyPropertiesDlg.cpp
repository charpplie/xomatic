////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TrackViewKeyPropertiesDlg.cpp
//  Created:     7/4/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "TcbKeyDialog.h"
#include <ISplines.h>

IMPLEMENT_DYNAMIC(CTrackViewKeyUIControls,CObject)

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyUIControls::OnInternalVariableChange( IVariable *pVar )
{
	SelectedKeys keys;
	CTrackViewUtils::GetSelectedKeys( m_pKeyPropertiesDlg->GetSequence(),keys );

	OnUIChange( pVar,keys );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyUIControls::RefreshTrackViewKeys()
{
	GetIEditor()->Notify(eNotify_OnUpdateTrackViewKeys);
}

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CTrackViewKeyPropertiesDlg, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyPropertiesDlg::CTrackViewKeyPropertiesDlg()
{
	m_pVarBlock = new CVarBlock;

	// Add key UI classes
	std::vector<IClassDesc*> classes;
	GetIEditor()->GetClassFactory()->GetClassesBySystemID( ESYSTEM_CLASS_TRACKVIEW_KEYUI,classes );
	for (int i = 0; i < (int)classes.size(); i++)
	{
		CObject *pObj = classes[i]->GetRuntimeClass()->CreateObject();
		if (pObj->IsKindOf(RUNTIME_CLASS(CTrackViewKeyUIControls)))
		{
			m_keyControls.push_back( (CTrackViewKeyUIControls*)pObj );
		}
	}

	CreateAllVars();
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewKeyPropertiesDlg::OnInitDialog()
{
	BOOL bRes = __super::OnInitDialog();	

	if (bRes)
	{
		CRect rc;
		GetClientRect(rc);
		m_wndProps.Create( WS_CHILD|WS_VISIBLE,rc,this );
		m_wndProps.ModifyStyleEx( 0,WS_EX_CLIENTEDGE );

		m_wndTrackProps.Create( CTrackViewTrackPropsDlg::IDD,this );

		{
			CBrush m_bkgBrush;
			m_bkgBrush.CreateSolidBrush(RGB(0xE0,0xE0,0xE0));
			WNDCLASS wndcls;
			ZeroStruct(wndcls);
			//you can specify your own window procedure
			wndcls.lpfnWndProc = ::DefWindowProc;
			wndcls.hInstance = AfxGetInstanceHandle();
			wndcls.hbrBackground = (HBRUSH) (COLOR_3DFACE + 1);
			wndcls.lpszClassName = _T("TrackViewTcbPreviewClass");
			AfxRegisterClass(&wndcls);
			m_wndTcbPreview.Create( _T("TrackViewTcbPreviewClass"),NULL,WS_CHILD|WS_VISIBLE,CRect(0,0,60,60),this,0 );
		}
	}

	return bRes;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyPropertiesDlg::OnSize( UINT nType, int cx, int cy )
{
	if (m_wndProps.m_hWnd)
	{
		CRect rcClient;
		GetClientRect(rcClient);

		CRect rcTrackWnd;
		m_wndTrackProps.GetWindowRect(rcTrackWnd);
		CRect rcTrack( rcClient.left,rcClient.top,rcClient.right,rcClient.top+rcTrackWnd.Height() );
		m_wndTrackProps.MoveWindow( rcTrack );

		int tcbPreview = 0;

		CRect rcProps( 2,rcTrack.bottom+4,rcClient.right-2,rcClient.bottom - 4 - tcbPreview );
		m_wndProps.MoveWindow( rcProps );

		CRect rcPreview( 2,rcProps.bottom+4,rcClient.left + tcbPreview + 20,rcClient.bottom - 4 );
		m_wndTcbPreview.MoveWindow( rcPreview );
	}
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewKeyPropertiesDlg::PreTranslateMessage( MSG* pMsg )
{
	if (pMsg->message == WM_KEYDOWN)
	{
		// In case of arrow keys, pass the message to keys window so that it can handle
		// a key selection change by arrow keys.
		int nVirtKey = (int) pMsg->wParam;
		if (nVirtKey == VK_UP || nVirtKey == VK_DOWN || nVirtKey == VK_RIGHT || nVirtKey == VK_LEFT)
		{
			if (m_keysCtrl)
				m_keysCtrl->SendMessage(WM_KEYDOWN, pMsg->wParam, pMsg->lParam);	
		}
	}
	return __super::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyPropertiesDlg::OnVarChange( IVariable *pVar )
{
	
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyPropertiesDlg::CreateAllVars()
{
	for (int i = 0; i < (int)m_keyControls.size(); i++)
	{
		m_keyControls[i]->SetKeyPropertiesDlg(this);
		m_keyControls[i]->OnCreateVars();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyPropertiesDlg::PopulateVariables()
{
	//SetVarBlock( m_pVarBlock,functor(*this,&CTrackViewKeyPropertiesDlg::OnVarChange) );

	// Must first clear any selection in properties window.
	m_wndProps.ClearSelection();
	m_wndProps.DeleteAllItems();
	m_wndProps.AddVarBlock( m_pVarBlock );

	m_wndProps.SetUpdateCallback( functor(*this,&CTrackViewKeyPropertiesDlg::OnVarChange) );
	//m_wndProps.ExpandAll();


	ReloadValues();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyPropertiesDlg::OnKeySelectionChange()
{
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys( GetSequence(),selectedKeys );

	if (m_wndTrackProps.m_hWnd)
		m_wndTrackProps.OnKeySelectionChange(selectedKeys);

	m_pVarBlock->Clear();
	
	bool bAssigned = false;
	if (selectedKeys.keys.size() > 0 && selectedKeys.bAllOfSameType)
	{
		for (int i = 0; i < (int)m_keyControls.size(); i++)
		{
			if (m_keyControls[i]->SupportTrackType(selectedKeys.trackType))
			{
				AddVars( m_keyControls[i] );
				if (m_keyControls[i]->OnKeySelectionChange(selectedKeys))
					bAssigned = true;
			}
		}
		m_wndProps.EnableWindow(TRUE);
	}
	else
	{
		m_wndProps.EnableWindow(FALSE);
	}

	PopulateVariables();

	if (selectedKeys.keys.size() > 1 || !bAssigned)
		m_wndProps.SetDisplayOnlyModified(true);
	else
		m_wndProps.SetDisplayOnlyModified(false);

	if (selectedKeys.keys.size() == 1 && selectedKeys.bAllOfSameType && 
			(selectedKeys.trackType == ATRACK_TCB_FLOAT 
			|| selectedKeys.trackType == ATRACK_TCB_VECTOR 
			|| selectedKeys.trackType == ATRACK_TCB_QUAT ) )
	{
		ITcbKey key;
		selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );
		m_wndTcbPreview.SetTcb( key.tens,key.cont,key.bias,key.easeto,key.easefrom );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyPropertiesDlg::AddVars( CTrackViewKeyUIControls *pUI )
{
	CVarBlock *pVB = pUI->GetVarBlock();
	for (int i = 0,num = pVB->GetVarsCount(); i < num; i++)
	{
		IVariable *pVar = pVB->GetVariable(i);
		m_pVarBlock->AddVariable(pVar);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyPropertiesDlg::ReloadValues()
{
	if (m_wndProps.m_hWnd)
	{
		m_wndProps.ReloadValues();
	}
}

void CTrackViewKeyPropertiesDlg::SetSequence( IAnimSequence *pSequence )
{
	m_pSequence = pSequence;
	m_wndTrackProps.SetSequence(pSequence);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CTrackViewTrackPropsDlg::CTrackViewTrackPropsDlg()
{
	m_track = 0;
	m_key = -1;
}

void CTrackViewTrackPropsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PREVNEXT, m_keySpinBtn);
	DDX_Control(pDX, IDC_KEYNUM, m_keynum);
}


BEGIN_MESSAGE_MAP(CTrackViewTrackPropsDlg, CDialog)
	ON_NOTIFY(UDN_DELTAPOS, IDC_PREVNEXT, OnDeltaposPrevnext)
	ON_EN_UPDATE(IDC_TIME, OnUpdateTime)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewTrackPropsDlg::OnInitDialog()
{
	BOOL bRes = CDialog::OnInitDialog();

	m_time.Create( this,IDC_TIME );
	m_time.SetRange( 0,0 );

	m_keySpinBtn.SetPos(0);
	m_keySpinBtn.SetBuddy( &m_keynum );
	m_keySpinBtn.SetRange( -10000,10000 );

	return bRes;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewTrackPropsDlg::SetSequence( IAnimSequence *pSequence )
{
	if (pSequence)
	{
		Range range = pSequence->GetTimeRange();
		m_time.SetRange( range.start,range.end );
	}
}


//////////////////////////////////////////////////////////////////////////
bool CTrackViewTrackPropsDlg::OnKeySelectionChange( CTrackViewUtils::SelectedKeys &selectedKeys )
{
	if (!IsWindow(m_hWnd))
		return false;

	m_track = 0;
	m_key = 0;

	if (selectedKeys.keys.size() == 1)
	{
		m_track = selectedKeys.keys[0].pTrack;
		m_key = selectedKeys.keys[0].nKey;
	}

	if (m_track != NULL)
	{
		m_time.SetValue( m_track->GetKeyTime(m_key) );
		m_keySpinBtn.SetRange( 1,m_track->GetNumKeys() );
		m_keySpinBtn.SetPos( m_key+1 );

		m_keySpinBtn.EnableWindow(TRUE);
		m_time.EnableWindow(TRUE);
	}
	else
	{
		m_keySpinBtn.EnableWindow(FALSE);
		m_time.EnableWindow(FALSE);
	}
	return true;
}

void CTrackViewTrackPropsDlg::OnDeltaposPrevnext(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	if (!m_track)
		return;

	int nkey = m_key + pNMUpDown->iDelta;
	if (nkey < 0)
		nkey = m_track->GetNumKeys()-1;
	if (nkey > m_track->GetNumKeys()-1)
		nkey = 0;

	SetCurrKey( nkey );

	*pResult = 1;
}

void CTrackViewTrackPropsDlg::OnUpdateTime()
{
	if (!m_track)
		return;

	if (m_key < 0 || m_key >= m_track->GetNumKeys())
		return;

	float time = m_time.GetValue();
	m_track->SetKeyTime( m_key,time );
	m_track->SortKeys();

	int k = m_track->FindKey( time );
	if (k != m_key)
	{
		SetCurrKey( k );
	}
	GetIEditor()->GetAnimation()->ForceAnimation();
}

void CTrackViewTrackPropsDlg::SetCurrKey( int nkey )
{
	if (m_key >= 0 && m_key < m_track->GetNumKeys())
	{
		m_track->SelectKey(m_key, false);
	}
	if (nkey >= 0 && nkey < m_track->GetNumKeys())
	{
		m_track->SelectKey(nkey, true);
	}
	GetIEditor()->Notify(eNotify_OnUpdateTVKeySelection);
}
