
#include "StdAfx.h"

#include "StringDlg.h"
#include "AnimationGraphDialog_2.h"
#include "AnimationGraph_2.h"

#include "AnimationGraphStateParams_2.h"

//////////////////////////////////////////////////////////////////////////
// CAnimationGraphStateParamCtrl2
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CAnimationGraphStateParamCtrl2,CDialog);

BEGIN_MESSAGE_MAP(CAnimationGraphStateParamCtrl2,CDialog)
	ON_WM_ERASEBKGND()
	ON_LBN_SELCHANGE( IDC_VALUESLIST, OnListBoxSelChange )
	ON_LBN_DBLCLK( IDC_VALUESLIST, OnListBoxDblClk )
	ON_EN_KILLFOCUS( IDC_PARAMNAME, OnParamNameKillFocus )
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


void CAnimationGraphStateParamCtrl2::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_PARAMNAME, m_ParamNameEdit );
	DDX_Control( pDX, IDC_VALUESLIST, m_ValuesList );
}

BOOL CAnimationGraphStateParamCtrl2::OnInitDialog()
{
	__super::OnInitDialog();

	m_ParamNameEdit.SetWindowText( m_name );

	m_ValuesList.AddString(" [All]");
	m_ValuesList.SetCurSel(0);

	m_ItemHeight = float( m_ValuesList.GetItemHeight(0) ) * 1.333f;
	m_ValuesList.SetItemHeight( 0, m_ItemHeight );

	AdjustHeight();
	return FALSE;
}

void CAnimationGraphStateParamCtrl2::AdjustHeight()
{
	int numItems = m_ValuesList.GetCount();
	if ( numItems > 7 )
		numItems = 7;

	CRect rcParent, rcWindow, rcClient;
	m_ValuesList.GetWindowRect( rcWindow );
	m_ValuesList.GetClientRect( rcClient );
	GetWindowRect( rcParent );

	rcParent.bottom -= rcWindow.bottom;
	rcWindow.bottom = rcWindow.top + rcWindow.Height()-rcClient.bottom + numItems*m_ItemHeight;
	m_ValuesList.SetWindowPos( 0, 0, 0, rcWindow.Width(), rcWindow.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER );
	rcParent.bottom += rcWindow.bottom;

	CAnimationGraphStateParamsPanel2* pParent = (CAnimationGraphStateParamsPanel2*) GetParent();
	SetWindowPos( 0, 0, 0, rcParent.Width(), rcParent.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER );
	pParent->ArrangeMiniPanels();
}


void CAnimationGraphStateParamCtrl2::SetParamNameText(const CString& text)
{
	m_ParamNameEdit.SetWindowText( text );
}


BOOL CAnimationGraphStateParamCtrl2::OnEraseBkgnd( CDC* pDC )
{
	CRect rc;
	GetClientRect( rc );
	pDC->FillSolidRect( 0, 1, rc.right, rc.bottom-1, GetSysColor(COLOR_BTNFACE) );
	pDC->FillSolidRect( 0, 0, rc.right, 1, GetSysColor(COLOR_3DSHADOW) );
	return TRUE;
}

void CAnimationGraphStateParamCtrl2::OnParamNameKillFocus()
{
	static bool bIgnore = false;
	if ( bIgnore )
		return;

	CString name;
	m_ParamNameEdit.GetWindowText( name );
	if ( name == m_name )
		return;

	bIgnore = true;
	if ( name.IsEmpty() )
	{
		if ( AfxMessageBox("This will delete selected state parameter!\n\nAre you sure?", MB_YESNO|MB_DEFBUTTON2) == IDYES )
		{
			if ( !((CAnimationGraphStateParamsPanel2*) GetParent())->DeleteParam(this) )
				m_ParamNameEdit.SetWindowText( m_name );
		}
		else
			m_ParamNameEdit.SetWindowText( m_name );
	}
	else
	{
		if ( ((CAnimationGraphStateParamsPanel2*) GetParent())->RenameParam(this, name) )
		{
			m_name = name;
			m_ParamNameEdit.SetWindowText( m_name );
		}
		else
			m_ParamNameEdit.SetWindowText( m_name );
	}
	bIgnore = false;
}

void CAnimationGraphStateParamCtrl2::OnListBoxSelChange()
{
	int sel = m_ValuesList.GetCurSel();
	if ( sel > 0 )
		m_ValuesList.GetText( sel, m_SelectedValue );
	else
		m_SelectedValue.Empty();
	((CAnimationGraphStateParamsPanel2*) GetParent())->OnSelChanged();
}

void CAnimationGraphStateParamCtrl2::OnListBoxDblClk()
{
	int sel = m_ValuesList.GetCurSel();
	if ( sel <= 0 )
		return;

	CStringDlg dlg;
	CString oldValue;
	m_ValuesList.GetText( sel, oldValue );
	dlg.m_strString = oldValue;
	if ( dlg.DoModal() == IDOK )
	{
		CAnimationGraphStateParamsPanel2* pParent = (CAnimationGraphStateParamsPanel2*) GetParent();
		if ( pParent->RenameParamValue(m_name, oldValue, dlg.m_strString) )
		{
			m_ValuesList.DeleteString( sel );
			m_ValuesList.SetCurSel( m_ValuesList.AddString(dlg.m_strString) );
			OnListBoxSelChange();
		}
	}
}

void CAnimationGraphStateParamCtrl2::OnContextMenu( CWnd* pWnd, CPoint pos )
{
	__super::OnContextMenu( pWnd, pos );

	if ( pos.x == -1 && pos.y == -1 )
	{
		pos.SetPoint( 5, 5 );
		m_ValuesList.ClientToScreen( &pos );
	}

	int selection = m_ValuesList.GetCurSel();

	CMenu menu;
	VERIFY( menu.CreatePopupMenu() );

	menu.AppendMenu( MF_STRING, ID_AG_PARAMVALUE_ADD, _T("Add...") );
	menu.AppendMenu( MF_STRING | ( selection > 0 ? 0 : MF_DISABLED ), ID_AG_PARAMVALUE_RENAME, _T("Rename...") );
	menu.AppendMenu( MF_STRING | ( selection > 0 ? 0 : MF_DISABLED ), ID_AG_PARAMVALUE_DELETE, _T("Delete...") );

	//menu.AppendMenu( MF_SEPARATOR );

	//CSmartObjectsEditorDialog* pDlg = (CSmartObjectsEditorDialog*) GetParent();

	// track menu	
	int nMenuResult = CXTPCommandBars::TrackPopupMenu( &menu, TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, pos.x, pos.y, this, NULL );

	CStringDlg dlg;
	int sel = m_ValuesList.GetCurSel();
	CAnimationGraphStateParamsPanel2* pParent = (CAnimationGraphStateParamsPanel2*) GetParent();
	switch ( nMenuResult )
	{
	case ID_AG_PARAMVALUE_ADD:
		if ( dlg.DoModal() == IDOK )
		{
			if ( pParent->AddParamValue(m_name, dlg.m_strString) )
			{
				m_ValuesList.SetCurSel( m_ValuesList.AddString(dlg.m_strString) );
				OnListBoxSelChange();
			}
		}
		break;

	case ID_AG_PARAMVALUE_RENAME:
		if ( sel <= 0 )
			return;
		OnListBoxDblClk();
		break;

	case ID_AG_PARAMVALUE_DELETE:
		if ( sel > 0 )
		{
			CString msg, value;
			m_ValuesList.GetText( sel, value );
			msg.Format( "Delete value '%s'?", value );
			if ( AfxMessageBox(msg, MB_YESNO) == IDYES )
			{
				if ( pParent->DeleteParamValue(m_name, value) )
				{
					m_ValuesList.DeleteString( sel );
					m_ValuesList.SetCurSel( 0 );
					OnListBoxSelChange();
				}
			}
		}
		break;
	}

	AdjustHeight();
	m_ValuesList.SetFocus();
}




//////////////////////////////////////////////////////////////////////////
// CAnimationGraphStateParamsDlg2
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAnimationGraphStateParamsDlg2,CDialog);

BEGIN_MESSAGE_MAP(CAnimationGraphStateParamsDlg2,CDialog)
	ON_COMMAND(IDC_ADDPARAM, OnAddParam)
	ON_BN_CLICKED(IDC_EXCLUDEFROMGRAPH, OnCheckBoxChanged)
END_MESSAGE_MAP()

void CAnimationGraphStateParamsDlg2::OnAddParam()
{
	CAnimationGraphStateParamsPanel2* pParent = (CAnimationGraphStateParamsPanel2*) GetParent();
	CStringDlg dlg;
	if ( dlg.DoModal() == IDOK )
	{
		pParent->OnAddParam( dlg.m_strString );
	}
}

void CAnimationGraphStateParamsDlg2::OnCheckBoxChanged()
{
	CAnimationGraphStateParamsPanel2* pParent = (CAnimationGraphStateParamsPanel2*) GetParent();
	CAnimationGraphDialog2* pOwner = (CAnimationGraphDialog2*) pParent->GetOwner();
	CButton* pCheckBox = (CButton*) GetDlgItem( IDC_EXCLUDEFROMGRAPH );
	pOwner->SetExcludeFromGraph( pCheckBox->GetCheck() == BST_CHECKED );
}




//////////////////////////////////////////////////////////////////////////
// CAnimationGraphStateParamsPanel2
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAnimationGraphStateParamsPanel2,CDialog);

BEGIN_MESSAGE_MAP(CAnimationGraphStateParamsPanel2,CDialog)
	ON_WM_SIZE()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()


void CAnimationGraphStateParamsPanel2::Init()
{
	m_StateParamsDlg.Create( m_StateParamsDlg.IDD, this );
	m_StateParamsDlg.ShowWindow( SW_HIDE );

	CRect rc;
	m_StateParamsDlg.GetWindowRect( rc );
	m_MiniPanelWidth = rc.Width();
	m_StateParamsDlgHeight = rc.Height();
}

void CAnimationGraphStateParamsPanel2::SetParamsDeclaration( CParamsDeclaration2* pParamsDcl )
{
	if ( pParamsDcl == m_pParamsDeclaration || !m_hWnd )
		return;
	m_pParamsDeclaration = pParamsDcl;

	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	GetScrollInfo( SB_VERT, &si );
	if( si.nPos>0 )
		ScrollWindow( 0, -si.nPos );
	si.nPos = 0;
	si.fMask = SIF_DISABLENOSCROLL | SIF_POS;
	SetScrollInfo( SB_VERT, &si );

	TListParamCtrls::iterator it, itEnd = m_ParamCtrls.end();
	for ( it = m_ParamCtrls.begin(); it != itEnd; ++it )
	{
		CAnimationGraphStateParamCtrl2* pCtrl = *it;
		pCtrl->DestroyWindow();
	}
	m_ParamCtrls.clear();
	
	m_StateParamsDlg.ShowWindow( m_pParamsDeclaration ? SW_SHOW : SW_HIDE );
	if ( m_pParamsDeclaration )
	{
		CParamsDeclaration2::iterator it, itEnd = m_pParamsDeclaration->end();
		for ( it = m_pParamsDeclaration->begin(); it != itEnd; ++it )
		{
			CAnimationGraphStateParamCtrl2* pParam = new CAnimationGraphStateParamCtrl2( it->first );
			m_ParamCtrls.push_back( pParam );

			pParam->Create( CAnimationGraphStateParamCtrl2::IDD, this );
			pParam->ShowWindow( SW_NORMAL );

			pParam->SetParamNameText( it->first );

			TSetOfCString& values = it->second;
			TSetOfCString::iterator itValues, itValuesEnd = values.end();
			for ( itValues = values.begin(); itValues != itValuesEnd; ++itValues )
				pParam->GetValueList()->AddString( *itValues );
			pParam->AdjustHeight();
		}
	}

	ArrangeMiniPanels();
	OnSelChanged();
}

bool CAnimationGraphStateParamsPanel2::AddParamValue( const char* param, const char* value )
{
	return ((CAnimationGraphDialog2*)GetOwner())->AddParamValue( param, value );
}

bool CAnimationGraphStateParamsPanel2::DeleteParamValue( const char* param, const char* value )
{
	return ((CAnimationGraphDialog2*)GetOwner())->DeleteParamValue( param, value );
}

bool CAnimationGraphStateParamsPanel2::RenameParamValue( const char* param, const char* oldValue, const char* newValue )
{
	return ((CAnimationGraphDialog2*)GetOwner())->RenameParamValue( param, oldValue, newValue );
}

bool CAnimationGraphStateParamsPanel2::OnAddParam( const char* name )
{
	if ( m_pParamsDeclaration->find( name ) != m_pParamsDeclaration->end() )
	{
		AfxMessageBox( "The entered parameter name already exists for selected state!", MB_OK|MB_ICONERROR );
		return false;
	}

	TListParamCtrls::iterator it, itEnd = m_ParamCtrls.end();
	for ( it = m_ParamCtrls.begin(); it != itEnd; ++it )
	{
		(*it)->GetValueList()->SetCurSel(0);
		(*it)->OnListBoxSelChange();
	}

	if ( !((CAnimationGraphDialog2*)GetOwner())->AddParameter( name ) )
		return false;

	CAnimationGraphStateParamCtrl2* pParam = new CAnimationGraphStateParamCtrl2( name );
	m_ParamCtrls.push_back( pParam );

	pParam->Create( CAnimationGraphStateParamCtrl2::IDD, this );
	pParam->ShowWindow( SW_NORMAL );

	ArrangeMiniPanels();
	OnSelChanged();

	return true;
}

bool CAnimationGraphStateParamsPanel2::DeleteParam( CAnimationGraphStateParamCtrl2* pParamCtrl )
{
	if ( !((CAnimationGraphDialog2*)GetOwner())->DeleteParameter( pParamCtrl->GetName() ) )
		return false;

	pParamCtrl->DestroyWindow();
	m_ParamCtrls.remove( pParamCtrl );
	ArrangeMiniPanels();
	OnSelChanged();
	return true;
}

bool CAnimationGraphStateParamsPanel2::RenameParam( CAnimationGraphStateParamCtrl2* pParamCtrl, const CString& newName )
{
	if ( !((CAnimationGraphDialog2*)GetOwner())->RenameParameter(pParamCtrl->GetName(), newName) )
		return false;

	OnSelChanged();
	return true;
}

void CAnimationGraphStateParamsPanel2::OnSelChanged()
{
	bool specialized = false;

	m_CurrentSelection.clear();
	TListParamCtrls::iterator it, itEnd = m_ParamCtrls.end();
	for ( it = m_ParamCtrls.begin(); it != itEnd; ++it )
	{
		CAnimationGraphStateParamCtrl2* pParamCtrl = *it;
		m_CurrentSelection[ pParamCtrl->GetName() ] = pParamCtrl->GetSelectedValue();
		specialized = specialized || !pParamCtrl->GetSelectedValue().IsEmpty();
	}
	if ( !specialized )
		m_CurrentSelection.clear();

	CAnimationGraphDialog2* pOwner = (CAnimationGraphDialog2*) GetOwner();
	pOwner->OnStateParamSelChanged();

	CButton* pCheckBox = (CButton*) m_StateParamsDlg.GetDlgItem( IDC_EXCLUDEFROMGRAPH );
	pCheckBox->EnableWindow( specialized );
	if ( specialized )
	{
		switch ( pOwner->GetExcludeFromGraph() )
		{
		case -1:
			pCheckBox->SetCheck( BST_INDETERMINATE );
			break;
		case 0:
			pCheckBox->SetCheck( BST_UNCHECKED );
			break;
		case 1:
			pCheckBox->SetCheck( BST_CHECKED );
			break;
		}
	}
	else
		pCheckBox->SetCheck( BST_UNCHECKED );
}

void CAnimationGraphStateParamsPanel2::ArrangeMiniPanels()
{
	if ( !IsWindow(m_StateParamsDlg) )
		return;

	CRect rc;
	GetClientRect( rc );

	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS;
	GetScrollInfo( SB_VERT, &si );

	int numColumns = 1;
	if ( m_MiniPanelWidth && rc.right > m_MiniPanelWidth )
		numColumns = rc.right / m_MiniPanelWidth;

	std::vector< int > yPerColumn;
	std::vector< int > widthPerColumn;
	for ( int i = 0; i < numColumns; ++i )
	{
		yPerColumn.push_back( -si.nPos - (i>0) );
		widthPerColumn.push_back( m_MiniPanelWidth + (i == numColumns-1 ? rc.right % m_MiniPanelWidth : 0) );
	}

	m_StateParamsDlg.MoveWindow( 0, -si.nPos, widthPerColumn[0], m_StateParamsDlgHeight );
	yPerColumn[0] += m_StateParamsDlgHeight;

	TListParamCtrls::iterator it, itEnd = m_ParamCtrls.end();
	for ( it = m_ParamCtrls.begin(); it != itEnd; ++it )
	{
		int currentColumn = 0;
		int minY = yPerColumn[0];
		for ( int i = 1; i < numColumns; ++i )
			if ( yPerColumn[i] < minY )
			{
				minY = yPerColumn[i];
				currentColumn = i;
			}

		CRect rcParam;
		CAnimationGraphStateParamCtrl2* pParamCtrl = *it;
		pParamCtrl->GetWindowRect( rcParam );
		pParamCtrl->MoveWindow( currentColumn*m_MiniPanelWidth, minY, widthPerColumn[currentColumn], rcParam.Height() );
		yPerColumn[currentColumn] += rcParam.Height();
	}

	int maxY = yPerColumn[0];
	for ( int i = 1; i < numColumns; ++i )
		if ( yPerColumn[i] > maxY )
			maxY = yPerColumn[i];

	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_RANGE;
	si.nMin = 0;
	si.nMax = maxY + si.nPos; // since we started at -si.nPos
	si.nPage = rc.bottom;
	m_bIgnoreVScroll = true;
	SetScrollInfo( SB_VERT, &si );
	m_bIgnoreVScroll = false;
}

void CAnimationGraphStateParamsPanel2::OnSize(UINT nType, int cx, int cy)
{
	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS;
	GetScrollInfo( SB_VERT, &si );
	int offset = si.nPos;

	__super::OnSize(nType, cx, cy);
	ArrangeMiniPanels();
	
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS;
	GetScrollInfo( SB_VERT, &si );
	offset -= si.nPos;
	ScrollWindow( 0, offset );
}

void CAnimationGraphStateParamsPanel2::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	__super::OnVScroll(nSBCode, nPos, pScrollBar);
	if ( m_bIgnoreVScroll )
		return;

	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_ALL;
	GetScrollInfo( SB_VERT, &si );
	int offset = si.nPos;

	switch (nSBCode)
	{
	case SB_BOTTOM: // Scroll to bottom
		si.nPos = si.nMax;
		break;
	case SB_ENDSCROLL: // End scroll
		break;
	case SB_LINEDOWN: // Scroll one line down
		si.nPos += 8;
		break;
	case SB_LINEUP: // Scroll one line up
		si.nPos -= 8;
		break;
	case SB_PAGEDOWN: // Scroll one page down
		si.nPos += si.nPage;
		break;
	case SB_PAGEUP: // Scroll one page up
		si.nPos -= si.nPage;
		break;
	case SB_THUMBPOSITION: // Scroll to the absolute position. The current position is provided in nPos
	case SB_THUMBTRACK: // Drag scroll box to specified position. The current position is provided in nPos
		si.nPos = nPos;
		break;
	case SB_TOP: // Scroll to top
		si.nPos = 0;
		break;
	}

	si.fMask = SIF_POS;
	si.nPos = max(min(si.nPos, si.nMax-(int)si.nPage),0);
	SetScrollInfo( SB_VERT, &si );

	offset -= si.nPos;
	ScrollWindow( 0, offset );
}