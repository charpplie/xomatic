#include "StdAfx.h"
#include "..\StringDlg.h"
#include "AnimationGraphDialog_2.h"
#include "IViewPane.h"
#include "resource.h"
#include "AnimationGraph_2.h"
#include "IslandDetection_2.h"
#include "SpeedReport_2.h"
#include "ICryAnimation.h"
#include "TransitionReport_2.h"
#include "..\CharacterEditor\CharPanel_Animation.h"
#include "..\CharacterEditor\ModelViewportCE.h"
#include "..\PropertiesPanel.h"
#include "AnimationGraph2_Modifier.h"

#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list

#define ANIMATIONGRAPH_DIALOGFRAME_CLASSNAME "AnimationGraphDialog2"
//#define ANIMATIONGRAPH_FILE_FILTER "Animation Graph 2.0 Files (*.ag2xml)|*.ag2xml; |Animation Graph 1.0 Files (*.xml)|*.xml"
#define ANIMATIONGRAPH_FILE_FILTER "Animation Graph Files (*.xml)|*.xml"
#define CHARACTER_FILE_FILTER "Character Files (*.cdf)|*.cdf"

#define IDC_ANIMGRAPH_STATES 1
#define IDC_ANIMGRAPH_STATEPARAMS 2
#define IDC_ANIMGRAPH_VIEWS 3
#define IDC_ANIMGRAPH_INPUTS 4
#define IDC_ANIMGRAPH_MODIFIERS 5

#define IDW_ANIMGRAPH_STATES_PANE AFX_IDW_CONTROLBAR_FIRST + 20
#define IDW_ANIMGRAPH_STATE_EDITOR_PANE AFX_IDW_CONTROLBAR_FIRST + 21
#define IDW_ANIMGRAPH_TEST_PANE AFX_IDW_CONTROLBAR_FIRST + 22
#define IDW_ANIMGRAPH_PREVIEW_PANE AFX_IDW_CONTROLBAR_FIRST + 23
#define IDW_ANIMGRAPH_PREVIEW_OPTIONS_PANE AFX_IDW_CONTROLBAR_FIRST + 24
#define IDW_ANIMGRAPH_STATE_PARAMS_PANE AFX_IDW_CONTROLBAR_FIRST + 25
#define IDW_ANIMGRAPH_STATE_QUERY_PANE AFX_IDW_CONTROLBAR_FIRST + 26
#define IDW_ANIMGRAPH_VIEWS_PANE AFX_IDW_CONTROLBAR_FIRST + 27
#define IDW_ANIMGRAPH_INPUTS_PANE AFX_IDW_CONTROLBAR_FIRST + 28

// new animation graph 2.0 panels
#define IDW_ANIMGRAPH_MODIFIER_PANE AFX_IDW_CONTROLBAR_FIRST + 29
#define IDW_ANIMGRAPH_DETAILS_PANE AFX_IDW_CONTROLBAR_FIRST + 30

//#define ANIMATION_GRAPH_PANE_COUNT 11
#define ANIMATION_GRAPH_PANE_COUNT IDW_ANIMGRAPH_DETAILS_PANE - IDW_ANIMGRAPH_STATES_PANE + 1

namespace AnimationGraphDialogHelpers2
{
	const char* GetDefaultCharacterName()
	{
		const char* szEditModelFallback = "Objects\\Characters\\Human\\Asian\\NK_Soldier\\nk_soldier_camp_flanker_light_01.cdf";

		ICVar* charEditModelCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar( "ca_CharEditModel" );
		if ( !charEditModelCVar )
			return szEditModelFallback;

		const char* szCharEditModel = charEditModelCVar->GetString();

		if ( szCharEditModel && szCharEditModel[0] ) 
			return szCharEditModel;
		else
			return szEditModelFallback;
	}
}

class CAnimationGraphViewClass2 : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {196C52AA-35FD-41E6-B76A-C401903DCC01}
		static const GUID guid = { 0x196c52aa, 0x35fd, 0x41e6, { 0xb7, 0x6a, 0xc4, 0x1, 0x90, 0x3d, 0xcc, 0x1 } };
		return guid;
	}
	virtual const char* ClassName() { return "Animation Graph"; };
	virtual const char* Category() { return "Animation Graph"; };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CAnimationGraphDialog2); };
	virtual const char* GetPaneTitle() { return _T("Animation Graph"); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(200,200,600,500); };
	virtual bool SinglePane() { return false; };
	virtual bool WantIdleUpdate() { return false; };
};


IMPLEMENT_DYNAMIC(CAnimationGraphStateParamCtrl2,CDialog);

BEGIN_MESSAGE_MAP(CAnimationGraphStateParamCtrl2,CDialog)
	ON_WM_ERASEBKGND()
	ON_LBN_SELCHANGE( IDC_VALUESLIST, OnListBoxSelChange )
	ON_LBN_DBLCLK( IDC_VALUESLIST, OnListBoxDblClk )
	ON_EN_KILLFOCUS( IDC_PARAMNAME, OnParamNameKillFocus )
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

BOOL CAnimationGraphStateParamCtrl2::DestroyWindow()
{
	return __super::DestroyWindow();
}

void CAnimationGraphStateParamCtrl2::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_PARAMNAME, m_ParamNameEdit );
	DDX_Control( pDX, IDC_VALUESLIST, m_ValuesList );
}

BOOL CAnimationGraphStateParamCtrl2::OnInitDialog()
{
	__super::OnInitDialog();

	m_ParamNameEdit.SetWindowText( m_Name );

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
	if ( name == m_Name )
		return;

	bIgnore = true;
	if ( name.IsEmpty() )
	{
		if ( AfxMessageBox("This will delete selected state parameter!\n\nAre you sure?", MB_YESNO|MB_DEFBUTTON2) == IDYES )
		{
			if ( !((CAnimationGraphStateParamsPanel2*) GetParent())->DeleteParam(this) )
				m_ParamNameEdit.SetWindowText( m_Name );
		}
		else
			m_ParamNameEdit.SetWindowText( m_Name );
	}
	else
	{
		if ( ((CAnimationGraphStateParamsPanel2*) GetParent())->RenameParam(this, name) )
		{
			m_Name = name;
			m_ParamNameEdit.SetWindowText( m_Name );
		}
		else
			m_ParamNameEdit.SetWindowText( m_Name );
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
		if ( pParent->RenameParamValue(m_Name, oldValue, dlg.m_strString) )
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
			if ( pParent->AddParamValue(m_Name, dlg.m_strString) )
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
				if ( pParent->DeleteParamValue(m_Name, value) )
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

	LockWindowUpdate();

	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	GetScrollInfo( SB_VERT, &si );
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
	
	m_StateParamsDlg.ShowWindow( m_pParamsDeclaration ? SW_NORMAL : SW_HIDE );
	if ( m_pParamsDeclaration )
	{
		CParamsDeclaration2::iterator it, itEnd = m_pParamsDeclaration->end();
		for ( it = m_pParamsDeclaration->begin(); it != itEnd; ++it )
		{
			CAnimationGraphStateParamCtrl2* pParam = new CAnimationGraphStateParamCtrl2( it->first );
			m_ParamCtrls.push_back( pParam );

			pParam->Create( CAnimationGraphStateParamCtrl2::IDD, this );
			pParam->ShowWindow( SW_NORMAL );

			pParam->m_ParamNameEdit.SetWindowText( it->first );

			TSetOfCString& values = it->second;
			TSetOfCString::iterator itValues, itValuesEnd = values.end();
			for ( itValues = values.begin(); itValues != itValuesEnd; ++itValues )
				pParam->m_ValuesList.AddString( *itValues );
			pParam->AdjustHeight();
		}
	}

	ArrangeMiniPanels();
	OnSelChanged();

	UnlockWindowUpdate();
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
		(*it)->m_ValuesList.SetCurSel(0);
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
	if ( !((CAnimationGraphDialog2*)GetOwner())->DeleteParameter(pParamCtrl->m_Name) )
		return false;

	pParamCtrl->DestroyWindow();
	m_ParamCtrls.remove( pParamCtrl );
	ArrangeMiniPanels();
	OnSelChanged();
	return true;
}

bool CAnimationGraphStateParamsPanel2::RenameParam( CAnimationGraphStateParamCtrl2* pParamCtrl, const CString& newName )
{
	if ( !((CAnimationGraphDialog2*)GetOwner())->RenameParameter(pParamCtrl->m_Name, newName) )
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
		m_CurrentSelection[ pParamCtrl->m_Name ] = pParamCtrl->m_SelectedValue;
		specialized = specialized || !pParamCtrl->m_SelectedValue.IsEmpty();
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


void CAnimationGraphDialog2::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CAnimationGraphViewClass2 );
}

IMPLEMENT_DYNCREATE(CAnimationGraphDialog2, CXTPFrameWnd);

BEGIN_MESSAGE_MAP(CAnimationGraphDialog2, CXTPFrameWnd)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CONTEXTMENU()

	ON_COMMAND( ID_FILE_NEW,OnFileNew )
	ON_COMMAND( ID_FILE_OPEN,OnFileOpen )
	ON_COMMAND( ID_FILE_SAVE,OnFileSave )
	ON_COMMAND( ID_FILE_SAVE_AS,OnFileSaveAs )
	ON_COMMAND( ID_GRAPH_ADDSTATE,OnGraphAddState )
	ON_COMMAND( ID_GRAPH_ADDVIEW,OnGraphAddView )
	ON_COMMAND( ID_GRAPH_ADDINPUT,OnGraphAddInput )
	ON_COMMAND_RANGE( ID_GRAPH_ADDMODIFIER_FIRST, ID_GRAPH_ADDMODIFIER_LAST, OnGraphAddModifier )

	ON_COMMAND( ID_GRAPH_CREATEANIMATION,OnGraphCreateAnimation )
	ON_COMMAND( ID_GRAPH_TRIAL,OnGraphTrial )
	ON_COMMAND( ID_GRAPH_ISLANDREPORT,OnGraphIslandReport )
	ON_COMMAND( ID_GRAPH_SPEEDREPORT,OnGraphSpeedReport )
	ON_COMMAND( ID_GRAPH_DEADINPUTREPORT,OnGraphDeadInputReport )
	ON_COMMAND( ID_GRAPH_BADCALFILEREPORT,OnGraphBadCALReport )
	ON_COMMAND( ID_GRAPH_TRANSITIONLENGTHREPORT,OnGraphTransitionLengthReport )
	ON_COMMAND( ID_GRAPH_MATCHMOVEMENTTEMPLATESPEEDSTOANIMATIONSPEEDS,OnGraphMatchSpeeds )
	ON_COMMAND( ID_GRAPH_BADNULLNODEREPORT, OnGraphBadNullNodeReport )
	ON_COMMAND( ID_GRAPH_ORPHANNODEREPORT, OnGraphOrphanNodesReport )
	ON_COMMAND( ID_SETTINGS_EDITDEFAULTSETTINGS, OnGraphDefaultSettingsDialog )
	ON_COMMAND( ID_AGVERSION_INFO, OnGraphVersionInfo )

	ON_COMMAND_RANGE( ID_VIEW_STATES, ID_VIEW_STATEPARAMS, OnViewPane )
	ON_COMMAND_RANGE( ID_VIEW_VIEWS, ID_VIEW_INPUTS, OnViewPane )
	ON_COMMAND_RANGE( ID_VIEW_STATES, ID_VIEW_DETAILS, OnViewPane )
	ON_COMMAND_RANGE( ID_VIEW_MODIFIERS, ID_VIEW_MODIFIERS, OnViewPane )
	ON_COMMAND( ID_VIEW_STATEQUERY, OnViewPaneStateQuery )
	ON_COMMAND( ID_AGLINK_MAPPINGCHANGED, OnAGLinkMappingChanged )

	ON_XTP_EXECUTE( ID_ANIMATION_GRAPH_ICON_SIZE, OnIconSizeChanged )
	ON_UPDATE_COMMAND_UI( ID_ANIMATION_GRAPH_ICON_SIZE, OnUpdateIconSizeUI )
	ON_XTP_EXECUTE( ID_ANIMATION_GRAPH_ICONS, OnDisplayIconsChanged )
	ON_UPDATE_COMMAND_UI( ID_ANIMATION_GRAPH_ICONS, OnUpdateDisplayIconsUI )
	ON_XTP_EXECUTE( ID_ANIMATION_GRAPH_PREVIEW, OnDisplayPreviewChanged )
	ON_UPDATE_COMMAND_UI( ID_ANIMATION_GRAPH_PREVIEW, OnUpdateDisplayPreviewUI )

	ON_NOTIFY( TVN_SELCHANGED, IDC_ANIMGRAPH_STATES, OnStateListSelChanged )
	ON_NOTIFY( TVN_BEGINDRAG, IDC_ANIMGRAPH_STATES, OnBeginDrag )

	ON_NOTIFY( TVN_SELCHANGED, IDC_ANIMGRAPH_MODIFIERS, OnModifierListSelChanged )
	ON_NOTIFY( TVN_BEGINDRAG, IDC_ANIMGRAPH_MODIFIERS, OnModifierListBeginDrag )

	ON_NOTIFY( TVN_SELCHANGED, IDC_ANIMGRAPH_VIEWS, OnViewListSelChanged )
	ON_NOTIFY( TVN_SELCHANGED, IDC_ANIMGRAPH_INPUTS, OnInputListSelChanged )

	//////////////////////////////////////////////////////////////////////////
	// XT Commands.
	ON_MESSAGE(XTPWM_DOCKINGPANE_NOTIFY, OnDockingPaneNotify)
	ON_MESSAGE( XTPWM_TASKPANEL_NOTIFY, OnTaskPanelNotify )
END_MESSAGE_MAP()

CAnimationGraphDialog2::CAnimationGraphDialog2()
	: m_stateListCtrl(this)
	, m_viewListCtrl(this)
	, m_inputListCtrl(this)
	, m_modifierListCtrl(this)
	, m_pDragImage(NULL)
	, m_nBlockSelChanged(0)
	, m_previewDialog(AnimationGraphDialogHelpers2::GetDefaultCharacterName())
	, m_pView(NULL)
	, m_pNodeDetailsPane(NULL)
{
	RegisterConsoleVariables();

	//m_enableIcons->Set(true);

	m_iconSizes.push_back(std::make_pair(64, 64));
	m_iconSizes.push_back(std::make_pair(96, 96));
	m_iconSizes.push_back(std::make_pair(128, 128));
	m_iconSizes.push_back(std::make_pair(192, 192));
	m_iconSizes.push_back(std::make_pair(256, 256));
	m_iconSizes.push_back(std::make_pair(512, 512));

	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();
	if (!(::GetClassInfo(hInst, ANIMATIONGRAPH_DIALOGFRAME_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style            = 0 /*CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW*/;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = NULL; // (HBRUSH) (COLOR_3DFACE + 1);
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = ANIMATIONGRAPH_DIALOGFRAME_CLASSNAME;
		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
		}
	}
	CRect rc(0,0,0,0);
	BOOL bRes = Create( WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,rc,AfxGetMainWnd() );
	if (!bRes)
		return;
	ASSERT( bRes );

	OnInitDialog();
}

CAnimationGraphDialog2::~CAnimationGraphDialog2()
{
	m_stateEditor.ClearActiveItem();
}

void
CAnimationGraphDialog2::OnDestroy()
{
	if (m_pGraph)
	{
		m_pGraph->RemoveListener(this);
		m_pGraph = 0;
	}
	if (m_pViewHypergraph)
	{
		m_pViewHypergraph->RemoveListener(this);
		m_pViewHypergraph = 0;
	}

	CXTPDockingPaneLayout layout( GetDockingPaneManager() );
	GetDockingPaneManager()->GetLayout( &layout );
	layout.Save( _T("AnimationGraph1_5Layout") );

	__super::OnDestroy();
}

BOOL CAnimationGraphDialog2::Create( DWORD dwStyle,const RECT& rect,CWnd* pParentWnd )
{
	BOOL bReturn = __super::Create( ANIMATIONGRAPH_DIALOGFRAME_CLASSNAME,NULL,dwStyle,rect,pParentWnd );

	m_pCAnimationImageManager = new CAnimationImageManager2(this, AnimationGraphDialogHelpers2::GetDefaultCharacterName(), std::make_pair(64, 64));

	return bReturn;
}

void CAnimationGraphDialog2::OnCharacterChanged()
{
	// update the graph and mark it as modified
	if ( m_pGraph )
		m_pGraph->SetCharacterName( m_previewDialog.GetModelViewport()->GetLoadedFileName() );

	// recreate the icons
	m_pCAnimationImageManager->ChangeModelAssetName( m_previewDialog.GetModelViewport()->GetLoadedFileName() );

	// invalidate the entire view.
	SetActiveView( m_pView );
}

BOOL CAnimationGraphDialog2::OnInitDialog()
{
	try
	{
		//////////////////////////////////////////////////////////////////////////
		// Initialize the command bars
		if (!InitCommandBars())
			return -1;

	}	catch (CResourceException *e)
	{
		e->Delete();
		return -1;
	}

	ModifyStyleEx( WS_EX_CLIENTEDGE, 0 );

	// Get a pointer to the command bars object.
	CXTPCommandBars* pCommandBars = GetCommandBars();
	if(pCommandBars == NULL)
	{
		TRACE0("Failed to create command bars object.\n");
		return -1;      // fail to create
	}

	m_defaultsDialog.SetAnimationGraph(GetAnimationGraph());

	// Add the menu bar
	CXTPCommandBar* pMenuBar = pCommandBars->SetMenu( _T("Menu Bar"), IDR_ANIMATIONGRAPH_2 );

	ASSERT(pMenuBar);

	pMenuBar->SetFlags(xtpFlagStretched);
	pMenuBar->EnableCustomization(FALSE);
	
	//////////////////////////////////////////////////////////////////////////
	GetDockingPaneManager()->InstallDockingPanes(this);
	GetDockingPaneManager()->SetTheme(xtpPaneThemeOffice2003);
	//GetDockingPaneManager()->SetTheme(CMainFrame::GetDockingPaneTheme());
	GetDockingPaneManager()->SetThemedFloatingFrames(TRUE);
	if (CMainFrame::GetDockingHelpers())
	{
		GetDockingPaneManager()->SetAlphaDockingContext(TRUE);
		GetDockingPaneManager()->SetShowDockingContextStickers(TRUE);
	}	
	//////////////////////////////////////////////////////////////////////////

	/* WARNING: If you add or remove panes, make sure you update the ANIMATION_GRAPH_PANE_COUNT macro
	to the new number of panes or things won't work correctly! */

	// States
	CRect rectStateListCtrl(0,0,220,500);
	m_stateListCtrl.Create( /*WS_VISIBLE|*/WS_CHILD|WS_TABSTOP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,rectStateListCtrl,this,IDC_ANIMGRAPH_STATES );
	m_stateListCtrl.ReloadStates();

//Diesel cut	
	//CXTPDockingPane* pStatesPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_STATES_PANE,rectStateListCtrl,dockLeftOf );
//Diesel new	
	CXTPDockingPane* pStatesPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_STATES_PANE,rectStateListCtrl,xtpPaneDockLeft );
	
	
	pStatesPane->SetTitle( _T("States") );

	// Inputs
	//CRect rectInputListCtrl(0,0,220,150);
	m_inputListCtrl.Create( /*WS_VISIBLE|*/WS_CHILD|WS_TABSTOP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,rectStateListCtrl,this,IDC_ANIMGRAPH_INPUTS );
	m_inputListCtrl.ReloadInputs();

//Diesel cut
	//CXTPDockingPane* pInputsPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_INPUTS_PANE,rectStateListCtrl,dockTopOf,pStatesPane );
//Diesel new	
	CXTPDockingPane* pInputsPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_INPUTS_PANE,rectStateListCtrl,xtpPaneDockTop,pStatesPane );
	
	
	pInputsPane->SetTitle( _T("Inputs") );
	GetDockingPaneManager()->AttachPane(pInputsPane, pStatesPane);
	//pStatesPane->Select();

	// Modifiers
	m_modifierListCtrl.Create( /*WS_VISIBLE|*/WS_CHILD|WS_TABSTOP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN, rectStateListCtrl, this, IDC_ANIMGRAPH_MODIFIERS );
	m_modifierListCtrl.ReloadModifiers();

//Diesel cut	
	//CXTPDockingPane* pModifierPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_MODIFIER_PANE, rectStateListCtrl, dockTopOf, pStatesPane );
//Diesel new	
	CXTPDockingPane* pModifierPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_MODIFIER_PANE, rectStateListCtrl, xtpPaneDockTop, pStatesPane );
	
	
	pModifierPane->SetTitle( _T("Modifiers") );
	GetDockingPaneManager()->AttachPane(pModifierPane, pStatesPane);
	pStatesPane->Select();

	// Views
	CRect rectViewListCtrl(0,0,220,200);
	m_viewListCtrl.Create( /*WS_VISIBLE|*/WS_CHILD|WS_TABSTOP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,rectViewListCtrl,this,IDC_ANIMGRAPH_VIEWS );
	m_viewListCtrl.ReloadViews();

//Diesel cut
	//CXTPDockingPane* pViewsPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_VIEWS_PANE,rectViewListCtrl,dockBottomOf,pStatesPane );
//Diesel new	
	CXTPDockingPane* pViewsPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_VIEWS_PANE,rectViewListCtrl,xtpPaneDockBottom,pStatesPane );
	
	
	pViewsPane->SetTitle( _T("Views") );

	// State Params
	m_stateParamsCtrl.Create( m_stateParamsCtrl.IDD );
	m_stateParamsCtrl.ModifyStyle( 0, WS_VSCROLL );
	m_stateParamsCtrl.ModifyStyleEx( 0, WS_EX_STATICEDGE );
	m_stateParamsCtrl.Init();

	CRect rectStateParamsCtrl( 0, 0, 500, 180);
	
//Diesel cut	
	//CXTPDockingPane* pStateParamsPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_STATE_PARAMS_PANE,rectStateParamsCtrl,dockBottomOf );
//Diesel new	
	CXTPDockingPane* pStateParamsPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_STATE_PARAMS_PANE,rectStateParamsCtrl,xtpPaneDockBottom );
	

	pStateParamsPane->SetTitle( "State Params" );
	pStateParamsPane->Hide();

	// Generic Pane for Node Details
	m_genericNodeDetails.Create( m_genericNodeDetails.IDD );
	m_genericNodeDetails.Init();
	CRect rectNodeDetailsCtrl( 0, 0, 500, 300);
	
//Diesel cut	
	//CXTPDockingPane* m_pNodeDetailsPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_DETAILS_PANE, rectNodeDetailsCtrl, dockBottomOf );
//Diesel new	
	CXTPDockingPane* m_pNodeDetailsPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_DETAILS_PANE, rectNodeDetailsCtrl, xtpPaneDockBottom );
	
	
	m_pNodeDetailsPane->SetTitle( _T("Node Details") );

	// State Editor
	CXTPDockingPane* pStateEditorPane = m_stateEditor.Init(this, IDW_ANIMGRAPH_STATE_EDITOR_PANE);
	m_stateEditor.GetPanel()->SetOwner(this);

	// Attach all node's details panes here and hide them
	//m_genericNodeDetails.Attach(m_stateEditor.GetAnimNodeDetailsPane()->GetSafeHwnd());
	//m_pNodeDetailsPane->ShowWindow( SW_HIDE );

	// Preview Options
	CRect rectPreviewOptionsCtrl(0,0,220,300);
	
//Diesel cut
	//CXTPDockingPane* pPreviewOptionsPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_PREVIEW_OPTIONS_PANE,rectPreviewOptionsCtrl,dockRightOf, pStateEditorPane);
//Diesel new	
	CXTPDockingPane* pPreviewOptionsPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_PREVIEW_OPTIONS_PANE,rectPreviewOptionsCtrl,xtpPaneDockRight, pStateEditorPane);
	

	pPreviewOptionsPane->SetTitle( _T("Preview Options") );
	//pPreviewOptionsPane->Hide();
	pPreviewOptionsPane->Close();

	// Preview
	CRect rectPreviewCtrl(0,0,220,300);
	m_previewDialog.Create( CAnimationGraphPreviewDialog2::IDD,this );
	m_previewManager.SetViewport( m_previewDialog.GetModelViewport() );
	m_previewDialog.GetModelViewport()->SetCharacterChangeListener( this );
	
//Diesel cut	
	//CXTPDockingPane* pPreviewPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_PREVIEW_PANE,rectPreviewCtrl,dockTopOf, pStateEditorPane);
//Diesel new	
	CXTPDockingPane* pPreviewPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_PREVIEW_PANE,rectPreviewCtrl,xtpPaneDockTop, pStateEditorPane);
	
	
	pPreviewPane->SetTitle( _T("Preview") );

	m_rollupCtrl.Create(/*WS_VISIBLE|*/WS_CHILD,CRect(4, 4, 187, 362),this, NULL);
	CharPanel_Animation* pAnimationPanel = new CharPanel_Animation(m_previewDialog.GetModelViewport(), NULL);
	m_previewDialog.GetModelViewport()->SetModelPanelA(pAnimationPanel);
	//m_previewDialog.GetModelViewport()->UpdateAnimationList(); // Xiaomao: don't need to update animation list in animation graph
	/*int animationControlRollupID = */m_rollupCtrl.InsertPage("Character Animation", pAnimationPanel);

	CPropertiesPanel* s_varsPanel;
	s_varsPanel = new CPropertiesPanel(this);
	s_varsPanel->AddVars( m_previewDialog.GetModelViewport()->GetVarObject()->GetVarBlock() );
	/*int propertiesRollupID = */m_rollupCtrl.InsertPage("Debug Options", s_varsPanel);

	// Testing
	m_tester.Init( this );
	
//Diesel cut	
	//CXTPDockingPane* pTestPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_TEST_PANE, rectStateListCtrl, dockRightOf );
//Diesel new	
	CXTPDockingPane* pTestPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_TEST_PANE, rectStateListCtrl, xtpPaneDockRight );
	
	
	pTestPane->SetTitle( _T("Testing") );
	pTestPane->Close();
//	pTestPane->Hide();

	// State Query
	m_stateQuery.Init( this );
	
//Diesel cut	
	//CXTPDockingPane* pStateQueryPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_STATE_QUERY_PANE, rectStateListCtrl, dockRightOf/*, pStateEditorPane*/ );
//Diesel new	
	CXTPDockingPane* pStateQueryPane = GetDockingPaneManager()->CreatePane( IDW_ANIMGRAPH_STATE_QUERY_PANE, rectStateListCtrl, xtpPaneDockRight/*, pStateEditorPane*/ );
	
	
	pStateQueryPane->SetTitle( _T("State Query") );
	//pStateQueryPane->Hide();
	pStateQueryPane->Close();

	// Main view
	CRect rc;
	GetClientRect(&rc);
	m_view.Create( WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_VISIBLE, rc, this, AFX_IDW_PANE_FIRST);
	m_view.ModifyStyleEx( 0, WS_EX_STATICEDGE );

	// Status bar
	m_statusBar.Create(this);
	static UINT indicators[] = 
	{
		ID_SEPARATOR,
	};
	m_statusBar.SetIndicators( indicators, sizeof(indicators)/sizeof(*indicators) );
	UpdateTitle();

	// Toolbar
	VERIFY(m_wndToolBar.CreateToolBar(WS_VISIBLE|WS_CHILD|CBRS_TOOLTIPS|CBRS_GRIPPER, this, AFX_IDW_TOOLBAR));
	VERIFY(m_wndToolBar.LoadToolBar(IDR_ANIMATION_GRAPH));
	m_wndToolBar.SetFlags(xtpFlagStretched);
	m_wndToolBar.EnableCustomization(FALSE);

	// Create the resolution combo box.
	{
		CXTPControl *pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_ANIMATION_GRAPH_ICON_SIZE, TRUE, FALSE);
		if (pCtrl)
		{
			int nIndex = pCtrl->GetIndex();
			CXTPControlComboBox *pCombo = (CXTPControlComboBox*)m_wndToolBar.GetControls()->SetControlType(nIndex,xtpControlComboBox);
			pCombo->SetWidth(80);

			int index = 0;
			for (IconSizeList::iterator itSize = m_iconSizes.begin(); itSize != m_iconSizes.end(); ++itSize)
			{
				int width = (*itSize).first;
				int height = (*itSize).second;
				string text;
				text.Format("%d x %d", width, height);
				pCombo->InsertString(index, text.c_str());
				++index;
			}
			pCombo->SetCurSel(m_displayedIconSize->GetIVal());
			m_pCAnimationImageManager->SetImageSize(m_iconSizes[m_displayedIconSize->GetIVal()]);
		}

		ReadDisplayedIconSize();
	}

	// Create the enable/disable icons button.
	{
		CXTPControl *pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_ANIMATION_GRAPH_ICONS, TRUE, FALSE);
		if (pCtrl)
		{
			int nIndex = pCtrl->GetIndex();
			CXTPControlButton *pPopup = (CXTPControlButton*)m_wndToolBar.GetControls()->SetControlType(nIndex,xtpControlButton);
			pPopup->SetCaption("Display Icons");
			pPopup->SetStyle(xtpButtonCaption);
		}
	}

	// Create the enable/disable preview button.
	{
		CXTPControl *pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_ANIMATION_GRAPH_PREVIEW, TRUE, FALSE);
		if (pCtrl)
		{
			int nIndex = pCtrl->GetIndex();
			CXTPControlButton *pPopup = (CXTPControlButton*)m_wndToolBar.GetControls()->SetControlType(nIndex,xtpControlButton);
			pPopup->SetCaption("Preview Animations");
			pPopup->SetStyle(xtpButtonCaption);
			m_previewManager.EnablePreview( m_preview->GetIVal() );
		}
	}

	CXTPDockingPaneLayout layout( GetDockingPaneManager() );
	if ( layout.Load( _T("AnimationGraph1_5Layout") ) && layout.GetPaneList().GetCount() >= ANIMATION_GRAPH_PANE_COUNT )
		GetDockingPaneManager()->SetLayout( &layout );

	return TRUE;
}

void CAnimationGraphDialog2::OnFileNew()
{
	CAnimationGraph2Ptr pGraph = new CAnimationGraph2(m_pCAnimationImageManager, this);
	pGraph->CreateMinimumGraph();
	SetGraph( pGraph );
}

void CAnimationGraphDialog2::OnFileOpen()
{
	CAnimationGraph2Ptr pGraph = new CAnimationGraph2(m_pCAnimationImageManager, this);
	CString filename;
	if (CFileUtil::SelectSingleFile( EFILE_TYPE_ANY,filename,ANIMATIONGRAPH_FILE_FILTER ))
	{
		CWaitCursor wait;
		if (pGraph->Load( filename ))
			SetGraph( pGraph );
	}
}

void CAnimationGraphDialog2::OnFileSave()
{
	if (!m_pGraph)
		return;

	m_view.ClearSelection();
	m_stateEditor.ClearActiveItem();

	if (m_pGraph->GetFilename().IsEmpty())
		OnFileSaveAs();
	else
	{
		CWaitCursor wait;
		m_pGraph->Save();

		m_stateQuery.RunUnitTests( true, false );
		m_stateQuery.Reload( false );
	}
}

void CAnimationGraphDialog2::OnFileSaveAs()
{
	CString filename = m_pGraph->GetFilename();
	CString dir = Path::GetPath(filename);
	if (CFileUtil::SelectSaveFile( ANIMATIONGRAPH_FILE_FILTER, "xml", dir, filename ))
	{
		m_view.ClearSelection();
		m_stateEditor.ClearActiveItem();

		CWaitCursor wait;
		m_pGraph->SetFilename( filename );
		m_pGraph->Save();

		m_stateQuery.Reload( false );
	}
}

void CAnimationGraphDialog2::SetGraph( CAnimationGraph2Ptr pGraph )
{
	m_pView = NULL;
	m_vStates.clear();

	m_view.SetGraph( NULL );
	m_pViewHypergraph = NULL;
	m_stateEditor.ClearActiveItem();
	if (m_pGraph)
		m_pGraph->RemoveListener(this);
	m_pGraph = pGraph;
	if (m_pGraph)
		m_pGraph->AddListener(this);
	m_defaultsDialog.SetAnimationGraph(pGraph);
	m_stateListCtrl.ReloadStates();
	m_viewListCtrl.ReloadViews();
	m_inputListCtrl.ReloadInputs();
	m_modifierListCtrl.ReloadModifiers();
	m_tester.Reload();
	m_stateQuery.Reload( false );
	m_previewDialog.SetCharacter( pGraph->GetCharacterName().IsEmpty() ? AnimationGraphDialogHelpers2::GetDefaultCharacterName() : pGraph->GetCharacterName() );
	pGraph->ShowIcons(m_enableIcons->GetIVal());

	// Attach the Modifier Manager here
	//m_pNodeDetailsPane->Attach(m_pGraph->GetModifierManager());

	UpdateTitle();

	UpdateGraphMenu();
}

void CAnimationGraphDialog2::UpdateTitle()
{
	CString titleFormatter;
	if (m_pGraph)
	{
		if (m_pGraph->IsModified())
			titleFormatter += "modified ";
		titleFormatter += m_pGraph->GetFilename();
	}
	else
	{
		titleFormatter += "No file open.";
	}
	m_statusBar.SetPaneText(0, titleFormatter );
}

CAnimationGraph2Ptr CAnimationGraphDialog2::GetAnimationGraph()
{
	return m_pGraph;
}

bool CAnimationGraphDialog2::ResetParameterization()
{
	if ( !m_stateEditor.IsParameterized() )
		return true;

	if ( AfxMessageBox("The selected item has custom parameterized data which will be lost by this action! \n\nProceed?", MB_YESNO|MB_DEFBUTTON2) == IDNO )
		return false;

	m_stateEditor.ResetParameterization();
	return true;
}

void CAnimationGraphDialog2::OnStateListSelChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	CWaitCursor wait;

	*pResult = TRUE;
	if (!m_pGraph)
		return;
	if (m_nBlockSelChanged)
		return;
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM treeitem = pNMTreeView->itemNew.hItem;
	if (treeitem != NULL)
	{
		m_inputListCtrl.SelectItem(NULL);
		m_modifierListCtrl.SelectItem(NULL);
	}
	CString item = m_stateListCtrl.GetItemText(treeitem);
	m_previewManager.SetState(CAGState2Ptr(0));
	m_vStates.clear();
	m_view.ClearSelection();
	SetActiveState( m_pGraph->FindState(item) );
}

void CAnimationGraphDialog2::OnViewListSelChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	CWaitCursor wait;

	*pResult = TRUE;
	if (!m_pGraph)
		return;
	if (m_nBlockSelChanged)
		return;
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM treeitem = pNMTreeView->itemNew.hItem;
	if (treeitem != NULL)
	{
		m_stateListCtrl.SelectItem(NULL);
		m_inputListCtrl.SelectItem(NULL);
	}
	CString item = m_viewListCtrl.GetItemText(treeitem);
	m_previewManager.SetState(CAGState2Ptr(0));
	m_vStates.clear();
	SetActiveView( m_pGraph->FindView(item) );
	m_stateEditor.EditPropertiesOf( m_pGraph->FindView(item) );
}

void CAnimationGraphDialog2::OnInputListSelChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	CWaitCursor wait;

	*pResult = TRUE;
	if (!m_pGraph)
		return;
	if (m_nBlockSelChanged)
		return;
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM treeitem = pNMTreeView->itemNew.hItem;
	if (treeitem != NULL)
	{
		m_stateListCtrl.SelectItem(NULL);
		m_modifierListCtrl.SelectItem(NULL);
	}
	CString item = m_inputListCtrl.GetItemText(treeitem);
	m_previewManager.SetState(CAGState2Ptr(0));
	m_vStates.clear();
	m_view.ClearSelection();
	m_stateEditor.EditPropertiesOf( m_pGraph->FindInput(item) );
}

void CAnimationGraphDialog2::OnGraphModified()
{
	UpdateTitle();
	m_stateQuery.Reload( true );
}

void CAnimationGraphDialog2::SetActiveState( CAGState2Ptr pState )
{
	m_vStates.clear();
	if ( pState )
	{
		m_vStates.push_back( pState );
		m_stateEditor.EditPropertiesOf( pState );
		m_previewManager.SetState( pState );
		if ( m_pView )
		{
			CHyperNode* pNode = m_pView->GetNodeForState( pState );
			if ( pNode && !pNode->IsSelected() )
				m_view.ShowAndSelectNode( pNode, true );
		}
		m_pGraph->GetModifierManager()->SetSelectedModifier(NULL);
	}
	else
	{
		if (m_stateEditor.GetActiveItem())
		{
			m_stateEditor.ClearActiveItem();
		}
		m_previewManager.SetState( CAGState2Ptr(0) );
	}
}

void CAnimationGraphDialog2::SetActiveStates( std::vector< CAGState2Ptr > vStates )
{
	if ( vStates.empty() )
		SetActiveState( NULL );
	else if ( vStates.size() == 1 )
		SetActiveState( vStates[0] );
	else
	{
		m_stateEditor.EditPropertiesOf( vStates );
		m_previewManager.SetState( CAGState2Ptr(0) );
		if ( m_pView )
		{
			std::vector< CAGState2Ptr >::iterator it, itEnd = vStates.end();
			for ( it = vStates.begin(); it != itEnd; ++it )
			{
				CAGState2* pState = *it;
				CHyperNode* pNode = m_pView->GetNodeForState( pState );
				if ( pNode && !pNode->IsSelected() )
					m_view.ShowAndSelectNode( pNode, true );
			}
		}
	}
}

void CAnimationGraphDialog2::SetActiveView( CAGView2Ptr pView )
{
	if (m_pViewHypergraph)
	{
		m_pViewHypergraph->RemoveListener(this);
		m_pViewHypergraph = NULL;
		m_view.SetGraph(NULL);
	}
	m_pView = pView;
	if (m_pView)
	{
		m_pViewHypergraph = pView->GetHyperGraph();
		m_pViewHypergraph->AddListener(this);
		m_view.SetGraph( m_pViewHypergraph );
	}
}

void CAnimationGraphDialog2::OnStateEvent( EAGStateEvent2 evt, CAGState2Ptr pState )
{
	switch (evt)
	{
	case eAGSE_ChangeName2:
		m_stateListCtrl.ReloadStates();
		SetActiveView( m_pView );
		break;
	case eAGSE_Cloned2:
	case eAGSE_Removed2:
		++m_nBlockSelChanged;
		m_stateListCtrl.ReloadStates();
		m_tester.Reload();
		m_stateQuery.Reload( true );
		SetActiveView( m_pView );
		if (evt != eAGSE_Removed2)
		{
			SetActiveState( pState );
			m_stateListCtrl.SelectItemByName( pState->GetName() );
		}
		--m_nBlockSelChanged;
		break;
	case eAGSE_ChangeUseTemplate2:
		m_stateQuery.Reload( true );
		break;
	case eAGSE_ChangeParent2:
		m_stateQuery.Reload( true );
		break;
	case eAGSE_ChangeIconSnapshotTime2:
		{
			m_pCAnimationImageManager->InvalidateImage(pState);

			class InvalidateNodeTreeRecursive
			{
			public:
				static void Perform(CHyperGraph* pViewHypergraph, CHyperNode* pNode)
				{
					if (pNode)
					{
						pViewHypergraph->InvalidateNode(pNode);
						pNode->Invalidate(true);
						IHyperGraphEnumerator* pEnum = pNode->GetChildrenEnumerator();
						if (pEnum)
						{
							for (IHyperNode* pChild = pEnum->GetFirst(); pChild; pChild = pEnum->GetNext())
								Perform(pViewHypergraph, static_cast<CHyperNode*>(pChild));
							pEnum->Release();
						}
					}
				}
			};

			InvalidateNodeTreeRecursive::Perform(m_pViewHypergraph, m_pView->GetNodeForState(pState));
		}
		break;
	}
}

void CAnimationGraphDialog2::OnViewEvent( EAGViewEvent2 evt, CAGView2Ptr pView )
{
	switch (evt)
	{
	case eAGVE_ChangeName2:
		++m_nBlockSelChanged;
		m_viewListCtrl.ReloadViews();
		m_tester.Reload();
		SetActiveView( m_pView );
		m_viewListCtrl.SelectItemByName( m_pView->GetName() );
		--m_nBlockSelChanged;
		break;
	case eAGVE_Removed2:
		++m_nBlockSelChanged;
		m_stateEditor.ClearActiveItem();
		m_viewListCtrl.ReloadViews();
		SetActiveView( NULL );
		--m_nBlockSelChanged;
		break;
	}
}

void CAnimationGraphDialog2::OnInputEvent( EAGInputEvent2 evt, CAGInput2Ptr pInput )
{
	switch (evt)
	{
	case eAGIE_ChangeName2:
	case eAGIE_ChangeType2:
	case eAGIE_Removed:
		++m_nBlockSelChanged;
		m_inputListCtrl.ReloadInputs();
		m_tester.Reload();
		m_stateQuery.Reload( true );
		SetActiveView( m_pView );
		m_inputListCtrl.SelectItemByName( pInput->GetName() );
		--m_nBlockSelChanged;
		break;
	}
}

LRESULT CAnimationGraphDialog2::OnTaskPanelNotify(WPARAM wParam, LPARAM lParam)
{
	switch(wParam) 
	{
	case XTP_TPN_CLICK:
		{
			CXTPTaskPanelGroupItem* pItem = (CXTPTaskPanelGroupItem*)lParam;
			UINT nCmdID = pItem->GetID();
			if (nCmdID < 60)
				m_tester.OnCommand(nCmdID);
			else if (nCmdID < 100)
				m_stateQuery.OnCommand(nCmdID);
			else
				m_stateEditor.OnCommand(nCmdID);
		}
		break;

	case XTP_TPN_RCLICK:
		break;
	}

	return 0;
}

void CAnimationGraphDialog2::OnHyperGraphEvent( IHyperNode * pNode, EHyperGraphEvent event )
{
	CAGNodeBase2* pBaseNode = (CAGNodeBase2*)pNode;
	if (!pBaseNode)
		return;

	if (pBaseNode->GetAGNodeType() == AG_NODE_MODIFIER)
	{
		OnModifierNodeEvent(pBaseNode, event);
	}
	else
	{
		OnStateNodeEvent(event);
	}


}

void CAnimationGraphDialog2::OnLinkEdit( CHyperEdge * pEdge )
{
	CAGLink2Ptr pLink = ((CAGEdge2*)pEdge)->pLink;
	m_stateEditor.EditPropertiesOf( pLink );
}

void CAnimationGraphDialog2::OnBeginDrag( NMHDR* pNMHDR, LRESULT* pResult )
{
	NM_TREEVIEW * pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	*pResult = TRUE;
	bool doDrag = false;

	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	m_dragNodeClass = m_stateListCtrl.GetItemText(hItem);
	doDrag = !!m_pView && m_pView->CanAddState( m_pGraph->FindState(m_dragNodeClass) );

	if (doDrag)
	{
		m_pDragImage = m_stateListCtrl.CreateDragImage( hItem );
		if (m_pDragImage)
		{
			m_pDragImage->BeginDrag(0, CPoint(-10, -10));
			CRect rc;
			GetWindowRect(rc);
			CPoint p = pNMTreeView->ptDrag;
			ClientToScreen( &p );
			p.x -= rc.left;
			p.y -= rc.top;
			m_pDragImage->DragEnter( this, p );
			SetCapture();

			*pResult = FALSE;
		}
	}
}

void CAnimationGraphDialog2::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_pDragImage)
	{
		CRect rc;
		GetWindowRect(rc);
		CPoint p = point;
		ClientToScreen(&p);
		p.x -= rc.left;
		p.y -= rc.top;
		m_pDragImage->DragMove(p);
		return;
	}
	__super::OnMouseMove(nFlags, point);
}

void CAnimationGraphDialog2::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_pDragImage)
	{
		CPoint p;
		GetCursorPos( &p );

		m_pDragImage->DragLeave( this );
		m_pDragImage->EndDrag();
		delete m_pDragImage;
		m_pDragImage = 0;
		ReleaseCapture();

		CWnd *wnd = CWnd::WindowFromPoint( p );
		if (wnd == &m_view)
		{
			m_view.ScreenToClient(&p);

			m_view.CreateNode( m_dragNodeClass, p );
			m_dragNodeClass.Empty();
		}

		return;
	}
	__super::OnLButtonUp(nFlags, point);
}

void CAGHyperGraphView2::ShowContextMenu( CPoint point, CHyperNode* pNode )
{
	OnLButtonDown( 0, point );
	OnLButtonUp( 0, point );
	if ( pNode )
	{
		ClientToScreen( &point );
		( (CAnimationGraphDialog2*) GetParent() )->OnContextMenu( this, point );
	}
}

void CAnimationGraphDialog2::OnContextMenu( CWnd* pWnd, CPoint pos )
{
	if ( pos.x == -1 && pos.y == -1 )
	{
		__super::OnContextMenu( pWnd, pos );
		return;
	}

	CWnd* wnd = WindowFromPoint( pos );
	if ( !wnd || wnd != &m_stateListCtrl && wnd != &m_modifierListCtrl /*&& wnd != &m_viewListCtrl && wnd != &m_inputListCtrl*/ && wnd != &m_view )
	{
		__super::OnContextMenu( pWnd, pos );
		return;
	}

	if ( wnd == &m_stateListCtrl )
	{
		TVHITTESTINFO hit;
		hit.pt = pos;
		m_stateListCtrl.ScreenToClient( &hit.pt );
		m_stateListCtrl.HitTest( &hit );
		if ( !hit.hItem )
		{
			__super::OnContextMenu( pWnd, pos );
			return;
		}
		m_stateListCtrl.SetFocus();
		if ( m_vStates.size() != 1 && m_stateListCtrl.GetSelectedItem() == hit.hItem )
			m_stateListCtrl.SelectItem( 0 );
		m_stateListCtrl.SelectItem( hit.hItem );
	}

	// Add in some handling of a click in the modifier list ctrl (non-existing yet)
	bool isModifier = false;
	CAG2ModifierBasePtr pModifier = NULL;
	if ( wnd == &m_modifierListCtrl )
	{
		TVHITTESTINFO hit;
		hit.pt = pos;
		m_modifierListCtrl.ScreenToClient( &hit.pt );
		m_modifierListCtrl.HitTest( &hit );
		if ( !hit.hItem )
		{
			__super::OnContextMenu( pWnd, pos );
			return;
		}
		m_modifierListCtrl.SetFocus();
		if ( m_modifierListCtrl.GetSelectedItem() == hit.hItem )
			m_modifierListCtrl.SelectItem( 0 );

		m_modifierListCtrl.SelectItem( hit.hItem );
		pModifier = m_pGraph->GetModifier(m_modifierListCtrl.GetItemData(hit.hItem));
		if (pModifier)
			isModifier = true;
	}

	CAGState2* pState = m_vStates.size() == 1 ? m_vStates[0] : NULL;
	if ( wnd == &m_view )
	{
		CPoint local( pos );
		m_view.ScreenToClient( &local );
		// Make a difference between regular states and modifier states
		CAGNodeBase2* pBaseNode = (CAGNodeBase2*) m_view.GetNodeAtPoint( local );
		if (pBaseNode->GetAGNodeType() == AG_NODE_MODIFIER)
		{
			isModifier = true;
			pModifier = ((CAG2ModifierNode*)pBaseNode)->GetModifier();
		}
		else
		{
			CAGNode2* pNode = (CAGNode2*) pBaseNode;
			if ( pNode )
				pState = pNode->GetState();
		}
	}

	if (isModifier)
	{
		// modifier PopUp menu
		ModifierStatePopupMenu(pModifier, wnd, pos);
	}
	else
	{
		// Animation State PopUp menu
		AnimationStatePopupMenu(pState, wnd, pos);
	}

}

void CAnimationGraphDialog2::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	if (::IsWindow(m_rollupCtrl.m_hWnd))
		m_rollupCtrl.Invalidate();
}

void CAnimationGraphDialog2::OnIconSizeChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMXTPCONTROL* tagNMCONTROL = (NMXTPCONTROL*)pNMHDR;
	CXTPControlComboBox* pControl = (CXTPControlComboBox*)tagNMCONTROL->pControl;
	if (pControl->GetType() == xtpControlComboBox)
	{
		int sel = pControl->GetCurSel();
		if (sel != CB_ERR)
		{
			const IconSize& iconSize = m_iconSizes[sel];
			m_pCAnimationImageManager->SetImageSize(iconSize);

			// Invalidate the entire view.
			SetActiveView(m_pView);
		}
	}
	*pResult = 1;
}

void CAnimationGraphDialog2::OnUpdateIconSizeUI(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_enableIcons->GetIVal());

	if (m_iconSizes[m_displayedIconSize->GetIVal()] != m_pCAnimationImageManager->GetImageSize())
	{
		CXTPControlComboBox *pControl = (CXTPControlComboBox*)m_wndToolBar.GetControls()->FindControl(xtpControlComboBox, ID_ANIMATION_GRAPH_ICON_SIZE, TRUE, FALSE);
		if (!pControl || pControl->GetType() != xtpControlComboBox)
			return;
		int sel = CB_ERR;
		IconSize iconSize = m_pCAnimationImageManager->GetImageSize();
		for (int index = 0; index < int(m_iconSizes.size()); ++index)
		{
			if (iconSize == m_iconSizes[index])
				sel = index;
		}

		if (sel != CB_ERR && sel != pControl->GetCurSel())
			pControl->SetCurSel(sel);

		ReadDisplayedIconSize();
	}
}

void CAnimationGraphDialog2::ReadDisplayedIconSize()
{
	CXTPControlComboBox *pControl = (CXTPControlComboBox*)m_wndToolBar.GetControls()->FindControl(xtpControlComboBox, ID_ANIMATION_GRAPH_ICON_SIZE, TRUE, FALSE);
	if (!pControl || pControl->GetType() != xtpControlComboBox)
		return;

	if (pControl->GetCurSel() != CB_ERR)
		m_displayedIconSize->Set(pControl->GetCurSel());
	else
		m_displayedIconSize->Set(0);
}

void CAnimationGraphDialog2::OnDisplayIconsChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMXTPCONTROL* tagNMCONTROL = (NMXTPCONTROL*)pNMHDR;
	CXTPControlButton* pControl = (CXTPControlButton*)tagNMCONTROL->pControl;
	if (pControl->GetType() == xtpControlButton)
	{
		pControl->SetChecked(!pControl->GetChecked());
		EnableIcons(pControl->GetChecked());
	}
	*pResult = 1;
}

void CAnimationGraphDialog2::OnUpdateDisplayIconsUI(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);

	CXTPControlButton *pControl = (CXTPControlButton*)m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_ANIMATION_GRAPH_ICONS, TRUE, FALSE);
	if (!pControl || pControl->GetType() != xtpControlButton)
		return;

	pControl->SetChecked(m_enableIcons->GetIVal());
}

void CAnimationGraphDialog2::OnDisplayPreviewChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMXTPCONTROL* tagNMCONTROL = (NMXTPCONTROL*)pNMHDR;
	CXTPControlButton* pControl = (CXTPControlButton*)tagNMCONTROL->pControl;
	if (pControl->GetType() == xtpControlButton)
	{
		pControl->SetChecked(!pControl->GetChecked());
		EnablePreview(pControl->GetChecked());
	}
	*pResult = 1;
}

void CAnimationGraphDialog2::OnUpdateDisplayPreviewUI(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);

	CXTPControlButton *pControl = (CXTPControlButton*)m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_ANIMATION_GRAPH_PREVIEW, TRUE, FALSE);
	if (!pControl || pControl->GetType() != xtpControlButton)
		return;

	pControl->SetChecked(m_preview->GetIVal());
}

//////////////////////////////////////////////////////////////////////////
LRESULT CAnimationGraphDialog2::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		// get a pointer to the docking pane being shown.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
		if (!pwndDockWindow->IsValid())
		{
			switch (pwndDockWindow->GetID())
			{
			case IDW_ANIMGRAPH_STATES_PANE:
				pwndDockWindow->Attach(&m_stateListCtrl);
				m_stateListCtrl.SetOwner( this );
				break;
			case IDW_ANIMGRAPH_VIEWS_PANE:
				pwndDockWindow->Attach(&m_viewListCtrl);
				m_viewListCtrl.SetOwner( this );
				break;
			case IDW_ANIMGRAPH_INPUTS_PANE:
				pwndDockWindow->Attach(&m_inputListCtrl);
				m_inputListCtrl.SetOwner( this );
				break;
			case IDW_ANIMGRAPH_STATE_EDITOR_PANE:
				pwndDockWindow->Attach(m_stateEditor.GetPanel());
				m_stateEditor.GetPanel()->SetOwner( this );
				break;
			case IDW_ANIMGRAPH_STATE_PARAMS_PANE:
				pwndDockWindow->Attach(&m_stateParamsCtrl);
				m_stateParamsCtrl.SetOwner( this );
				break;
			case IDW_ANIMGRAPH_TEST_PANE:
				//pwndDockWindow->Attach(&m_tester);
				//m_tester.SetOwner( this );
				break;
			case IDW_ANIMGRAPH_STATE_QUERY_PANE:
				pwndDockWindow->Attach(&m_stateQuery);
				m_stateQuery.SetOwner( this );
				break;
			case IDW_ANIMGRAPH_PREVIEW_PANE:
				pwndDockWindow->Attach(&m_previewDialog);
				m_previewDialog.SetOwner( this );
				break;
			case IDW_ANIMGRAPH_PREVIEW_OPTIONS_PANE:
				pwndDockWindow->Attach(&m_rollupCtrl);
				m_rollupCtrl.SetOwner( this );
				break;

				// Animation Graph 2.0
			case IDW_ANIMGRAPH_MODIFIER_PANE:
				pwndDockWindow->Attach(&m_modifierListCtrl);
				m_modifierListCtrl.SetOwner( this );
				break;
			case IDW_ANIMGRAPH_DETAILS_PANE:
				pwndDockWindow->Attach(&m_genericNodeDetails);
				m_genericNodeDetails.SetOwner( this );
				break;

			default:
				return FALSE;
			}
		}
		return TRUE;
	}
	else if (wParam == XTP_DPN_CLOSEPANE)
	{
		// get a pointer to the docking pane being shown.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
		if (pwndDockWindow->IsValid())
		{
			switch (pwndDockWindow->GetID())
			{
			case IDW_ANIMGRAPH_STATES_PANE:
				break;
			}
		}
	}

	return FALSE;
}

void CAnimationGraphDialog2::OnAGLinkMappingChanged()
{
	m_stateEditor.StateEvent( eSEE_InitContainer2 );
}

void CAnimationGraphDialog2::OnViewPane( UINT cmdID )
{
	int pane = 0;
	switch ( cmdID )
	{
	case ID_VIEW_STATEQUERY:
			OnViewPaneStateQuery();
			return;
			break;
	case ID_VIEW_STATES:
		pane = IDW_ANIMGRAPH_STATES_PANE;
		break;
	case ID_VIEW_VIEWS:
		pane = IDW_ANIMGRAPH_VIEWS_PANE;
		break;
	case ID_VIEW_INPUTS:
		pane = IDW_ANIMGRAPH_INPUTS_PANE;
		break;
	case ID_VIEW_TESTING:
		//pane = IDW_ANIMGRAPH_TEST_PANE;
		return;
		break;
	case ID_VIEW_PREVIEW:
		pane = IDW_ANIMGRAPH_PREVIEW_PANE;
		break;
	case ID_VIEW_PREVIEWOPTIONS:
		pane = IDW_ANIMGRAPH_PREVIEW_OPTIONS_PANE;
		break;
	case ID_VIEW_STATEEDITOR:
		pane = IDW_ANIMGRAPH_STATE_EDITOR_PANE;
		break;
	case ID_VIEW_STATEPARAMS:
		pane = IDW_ANIMGRAPH_STATE_PARAMS_PANE;
		break;

		// Animation Graph 2.0
	case ID_VIEW_MODIFIERS:
		pane = IDW_ANIMGRAPH_MODIFIER_PANE;
		break;
	case ID_VIEW_DETAILS:
		pane = IDW_ANIMGRAPH_DETAILS_PANE;
		break;

	default:
		assert( !"invalid command id" );
		return;
	}
	m_paneManager.ShowPane( pane, TRUE );
}

void CAnimationGraphDialog2::OnViewPaneStateQuery()
{
	m_paneManager.ShowPane( IDW_ANIMGRAPH_STATE_QUERY_PANE, TRUE );
}

void CAnimationGraphDialog2::OnGraphAddState()
{
	if ( m_pGraph == NULL )
	{
		return;
	}

	CString name = GenerateName(m_pGraph, &CAnimationGraph2::FindState, "state");
	m_pGraph->AddState( new CAGState2(m_pGraph, name) );
	m_stateListCtrl.ReloadStates();
	m_tester.Reload();
	m_stateQuery.Reload( true );
	m_stateListCtrl.SelectItemByName(name);
}

void CAnimationGraphDialog2::OnGraphAddView()
{
	if ( m_pGraph == NULL )
	{
		return;
	}

	CString name = GenerateName(m_pGraph, &CAnimationGraph2::FindView, "view");
	m_pGraph->AddView( new CAGView2(m_pGraph, name) );
	m_viewListCtrl.ReloadViews();
	m_tester.Reload();
	m_viewListCtrl.SelectItemByName(name);
}

void CAnimationGraphDialog2::OnGraphAddInput()
{
	if ( m_pGraph == NULL )
	{
		return;
	}

	CString name = GenerateName(m_pGraph, &CAnimationGraph2::FindInput, "input");
	m_pGraph->AddInput( new CAGInput2(m_pGraph, name) );
	m_inputListCtrl.ReloadInputs();
	m_tester.Reload();
	m_stateQuery.Reload( true );
	m_inputListCtrl.SelectItemByName(name);
}

void CAnimationGraphDialog2::OnGraphCreateAnimation()
{
	AfxMessageBox("Not yet implemented, sorry.");
}

void CAnimationGraphDialog2::OnGraphIslandReport()
{
	if (m_pGraph)
	{
		ViewReport( GenerateIslandReport(m_pGraph) );
	}
}

void CAnimationGraphDialog2::OnGraphBadNullNodeReport()
{
	if (m_pGraph)
	{
		ViewReport( GenerateNullNodesWithNoForceLeaveReport2(m_pGraph) );
	}
}

void CAnimationGraphDialog2::OnGraphOrphanNodesReport()
{
	if (m_pGraph)
	{
		ViewReport( GenerateOrphanNodesReport2(m_pGraph) );
	}
}

void CAnimationGraphDialog2::OnGraphSpeedReport()
{
	CString filename;
	if (CFileUtil::SelectSingleFile( EFILE_TYPE_ANY,filename,CHARACTER_FILE_FILTER ))
	{
		if (ICharacterInstance * pInstance = GetISystem()->GetIAnimationSystem()->CreateInstance(filename))
		{
			ViewReport( GenerateSpeedReport2(pInstance->GetIAnimationSet()) );
			pInstance->Release();
		}
		else
			AfxMessageBox("Couldn't create character " + filename);
	}
}

void CAnimationGraphDialog2::OnGraphBadCALReport()
{
	if ( m_pGraph == NULL )
	{
		return;
	}

	CString filename;
	if (CFileUtil::SelectSingleFile( EFILE_TYPE_ANY,filename,CHARACTER_FILE_FILTER ))
	{
		if (ICharacterInstance * pInstance = GetISystem()->GetIAnimationSystem()->CreateInstance(filename))
		{
			ViewReport( GenerateBadCALReport2(pInstance->GetIAnimationSet(), m_pGraph) );
			pInstance->Release();
		}
		else
			AfxMessageBox("Couldn't create character " + filename);
	}
}

void CAnimationGraphDialog2::OnGraphMatchSpeeds()
{
	if ( m_pGraph == NULL )
	{
		return;
	}

	CString filename;
	if (CFileUtil::SelectSingleFile( EFILE_TYPE_ANY,filename,CHARACTER_FILE_FILTER ))
	{
		if (ICharacterInstance * pInstance = GetISystem()->GetIAnimationSystem()->CreateInstance(filename))
		{
			ViewReport( MatchMovementSpeedsToAnimations2(pInstance->GetIAnimationSet(), m_pGraph) );
			pInstance->Release();
		}
		else
			AfxMessageBox("Couldn't create character " + filename);
	}
}

void CAnimationGraphDialog2::OnGraphDeadInputReport()
{
	if ( m_pGraph == NULL )
	{
		return;
	}

	ViewReport( GenerateDeadInputsReport2( m_pGraph ) );
}

void CAnimationGraphDialog2::OnGraphTransitionLengthReport()
{
	if ( m_pGraph == NULL )
	{
		return;
	}

	CString out;
	FindLongTransitions( out, m_pGraph, 2 );
	ViewReport( out );
}

void CAnimationGraphDialog2::ViewReport( const CString& report )
{
	FILE * f = fopen("animation_graph_report.txt", "wt");
	if (f)
	{
		fwrite( (const char *)report, report.GetLength(), 1, f );
		fclose(f);
		ShellExecute( GetSafeHwnd(), "open", "animation_graph_report.txt", NULL, ".", SW_SHOW );
	}
}

void CAnimationGraphDialog2::OnGraphTrial()
{
	CString errorString;

	if (m_pGraph)
	{
		XmlNodeRef graph = m_pGraph->ToXml();
		if (graph)
		{
			CString filename = m_pGraph->GetFilename();
			filename = PathUtil::GetFile( (const char*)filename );
			if (!GetISystem()->GetIAnimationGraphSystem()->TrialAnimationGraph( filename, graph ))
				errorString = "Unable to load animation graph into game engine";
		}
		else
		{
			errorString = "Unable to convert graph to XML";
		}
	}

	if (!errorString.IsEmpty())
		AfxMessageBox( errorString );
}

/*
 * CAnimationGraphListCtrl
 */

BOOL CAnimationGraphListCtrl2::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	BOOL ret = __super::Create( dwStyle|WS_BORDER|TVS_HASBUTTONS|/*TVS_LINESATROOT|*/TVS_HASLINES|TVS_SHOWSELALWAYS,rect,pParentWnd,nID );

	CMFCUtils::LoadTrueColorImageList( m_imageList, IDB_ANIMATIONGRAPH2_COMPONENTS /*IDB_HYPERGRAPH_COMPONENTS*/,20 /*16*/,RGB(255,0,255) );
	SetImageList( &m_imageList,TVSIL_NORMAL );
	SetItemHeight(18 /*14*/);

	return ret;
}

inline bool CompareNames( const CAnimationGraphListCtrl2::TNameIcon& ni1, const CAnimationGraphListCtrl2::TNameIcon& ni2 )
{
	return ni1.first < ni2.first;
}

void CAnimationGraphListCtrl2::AddList( const CString& root, std::vector< TNameIcon >& items)
{
	std::sort( items.begin(), items.end(), CompareNames );
	for (std::vector< TNameIcon >::const_iterator iter = items.begin(); iter != items.end(); ++iter )
	{
		InsertItem( iter->first, iter->second, iter->second );
	}
}

void CAnimationGraphListCtrl2::ReloadStates()
{
	SetRedraw(FALSE);
	DeleteAllItems();

	if (CAnimationGraph2Ptr pGraph = m_pParent->GetAnimationGraph())
	{
		std::set< CString > parents;

		std::vector< TNameIcon > items;
		for (CAnimationGraph2::state_iterator iter = pGraph->StateBegin(); iter != pGraph->StateEnd(); ++iter)
		{
			CAGState2* pState = *iter;
			if ( CAGState2* pParent = pState->GetParent() )
				parents.insert( pParent->GetName() );
			int icon = !pState->IncludeInGame() ? 1 : pState->AllowSelect() ? 3 : 2;
			if ( !pState->IsNullState() )
				icon += 3;
			items.push_back( TNameIcon(pState->GetName(), icon) );
		}
		AddList( "States", items );

		// Look at all of the state items and make the parent states bold
		TCHAR szText[1024];
		TVITEM item;
		item.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_STATE;
		item.pszText = szText;
		item.cchTextMax = 1024;
		for ( item.hItem = GetRootItem(); item.hItem; item.hItem = GetNextItem(item.hItem, TVGN_NEXT) )
		{
			if ( GetItem(&item) )
			{
				if ( parents.find(item.pszText) == parents.end() )
					continue;
				item.state = TVIS_BOLD;
				item.stateMask = TVIS_BOLD;
				SetItem(&item);
				item.stateMask = 0;
			}
		}
	}

	SetRedraw(TRUE);
}

void CAnimationGraphListCtrl2::ReloadModifiers()
{
	SetRedraw(FALSE);
	DeleteAllItems();

	if (CAnimationGraph2Ptr pGraph = m_pParent->GetAnimationGraph())
	{
		// Go through all existing modifiers and add them in here
//		std::vector< TNameIcon > items;
		CAnimationGraph2::modifier_iterator iterEnd = pGraph->ModifiersEnd();
		for (CAnimationGraph2::modifier_iterator iter = pGraph->ModifiersBegin(); iter != iterEnd; ++iter)
		{
			HTREEITEM newItem = InsertItem( iter->second->GetListDisplayString(), 11, 11 );
			SetItemData(newItem, iter->second->GetId());
			//items.push_back( TNameIcon(iter->second->GetListDisplayString(), 11) );
		}
		//AddList( "Modifiers", items );
	}

	SetRedraw(TRUE);
}

void CAnimationGraphListCtrl2::ReloadViews()
{
	SetRedraw(FALSE);
	DeleteAllItems();

	if (CAnimationGraph2Ptr pGraph = m_pParent->GetAnimationGraph())
	{
		std::vector< TNameIcon > items;
		for (CAnimationGraph2::view_iterator iter = pGraph->ViewBegin(); iter != pGraph->ViewEnd(); ++iter)
			items.push_back( TNameIcon((*iter)->GetName(),7) );
		AddList( "Views", items );
	}

	SetRedraw(TRUE);
}

void CAnimationGraphListCtrl2::ReloadInputs()
{
	SetRedraw(FALSE);
	DeleteAllItems();

	if (CAnimationGraph2Ptr pGraph = m_pParent->GetAnimationGraph())
	{
		std::vector< TNameIcon > items;
		for (CAnimationGraph2::input_iterator iter = pGraph->InputBegin(); iter != pGraph->InputEnd(); ++iter)
		{
			CAGInput2* pInput = *iter;
			EAnimationGraphInputType iType = pInput->GetType();
			items.push_back( TNameIcon(pInput->GetName(), iType==eAGIT_Float?8:iType==eAGIT_Integer?9:10) );
		}
		AddList( "Inputs", items );
	}

	SetRedraw(TRUE);
}

void CAnimationGraphListCtrl2::SelectItemByName( const CString& state )
{
	for (HTREEITEM item = GetRootItem(); item; item = GetNextSiblingItem(item))
	{
		if (state == GetItemText(item))
		{
			SelectItem( item );
			return;
		}
	}
	this->SelectItem(NULL);
}


//////////////////////////////////////////////////////////////////////////
BOOL CAnimationGraphDialog2::PreTranslateMessage(MSG* pMsg)
{
	bool bFramePreTranslate = true;
	if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
	{
		CWnd* pWnd = CWnd::GetFocus();
		if (pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CEdit)))
			bFramePreTranslate = false;
	}

	if (bFramePreTranslate)
	{
		// allow tooltip messages to be filtered
		if (__super::PreTranslateMessage(pMsg))
			return TRUE;
	}

	if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
	{
		// All keypresses are translated by this frame window

		::TranslateMessage(pMsg);
		::DispatchMessage(pMsg);

		return TRUE;
	}

	return FALSE;
}

void CAnimationGraphDialog2::EnableIcons(bool enableIcons)
{
	if (m_enableIcons->GetIVal() != int(enableIcons))
	{
		m_enableIcons->Set(enableIcons);
		if (m_pGraph)
			m_pGraph->ShowIcons(enableIcons);

		// Invalidate the entire view.
		SetActiveView(m_pView);
	}
}

void CAnimationGraphDialog2::EnablePreview(bool enablePreview)
{
	if (m_preview->GetIVal() != int(enablePreview))
	{
		m_preview->Set(enablePreview);
		m_previewManager.EnablePreview(enablePreview);
	}
}

void CAnimationGraphDialog2::RegisterConsoleVariables()
{
	m_displayedIconSize = gEnv->pConsole->GetCVar("ag_icon_size");
	if (!m_displayedIconSize)
	{
		m_displayedIconSize = REGISTER_INT("ag_icon_size",0,0,
			"Size of icons on the animation graph (0-4).\n"
			"Default is 0.");
	}

	m_enableIcons = gEnv->pConsole->GetCVar("ag_display_icons");
	if (!m_enableIcons)
	{
		m_enableIcons = REGISTER_INT("ag_display_icons",1,0,
			"Flag indicating whether icons are displayed on the animation graph.\n"
			"Default is 1.");
	}

	m_preview = gEnv->pConsole->GetCVar("ag_preview");
	if (!m_preview)
	{
		m_preview = REGISTER_INT("ag_preview", 1, 0,
			"Flag indicating whether animations are previewed in the preview window of the animation graph.\n"
			"Default is 1.");
	}
}

void CAnimationGraphDialog2::OnStateParamSelChanged()
{
	if ( ::IsWindow(m_stateParamsCtrl) )
	{
		m_stateEditor.OnStateParamSelChanged(
			m_stateParamsCtrl.m_CurrentSelection.empty() ? NULL : &m_stateParamsCtrl.m_CurrentSelection,
			m_pView ? m_pView->GetName() : "" );

		TParameterizationId2::const_iterator it;
		TParameterizationId2::const_iterator itEnd = m_stateParamsCtrl.m_CurrentSelection.end();
		
		for ( it = m_stateParamsCtrl.m_CurrentSelection.begin(); it != itEnd; ++it )
			if ( !it->second.IsEmpty() )
				m_previewManager.SetParameter( it->first, it->second );
	}
}

void CAnimationGraphDialog2::OnGraphDefaultSettingsDialog()
{
	// start the dialog window with all the different
	// node default settings
	m_defaultsDialog.DoModal();

	if (!m_pGraph)
		return;

	// maybe reload character if it has changed
	CString newCharName = m_pGraph->GetCharacterName();
	const CString loadedCharName = m_previewDialog.GetCharacterName();
	if (newCharName != loadedCharName)
	{
		m_previewDialog.SetCharacter( newCharName.IsEmpty() ? AnimationGraphDialogHelpers2::GetDefaultCharacterName() : newCharName );
	}
}

void CAnimationGraphDialog2::OnGraphVersionInfo()
{
	AfxMessageBox("AnimationGraph Version 1.5\n\n\
Compatibility Issues when Converting from 1.0 Graph to 1.5:\n\
- Transition Times and Playback Speed in parameterized states that are overridden\n\
  in sub-parameterizations are not converted. Only one transition time and playback\n\
  speed per state is supported.\n\
- Animations on multiple layers within one state are no longer supported for\n\
  better flow control. Warning message will appear, use Additive Modifier to\n\
  start animations on additional layers\n\
- Custom Template	Nodes added that were not shipped with AnimationGraph 1.0\n\
  will not be automatically converted.\n\
  Create a custom Modifier to re-integrate them into AG 1.5\n\
- WaitForKeyTime (CA_START_AT_KEYFRAME flag) parameter has been removed\n\
  (unused in Crysis 1 and SDK graph, so no compatibility issues)\n\
- Custom additional	Template Nodes defined in Parent states are not inherited by the child states\n\
- ForceStartImmediately is no longer available in the graph, because the flags affected\n\
  by it are obsolete and are no longer in use.\n\
- Events and other template nodes/extra parameters that read in custom template\n\
  parameter values (such as $name) will not work. Conversion will create dummy\n\
  Modifiers for these case. This is valid with the exception of Outputs, which\n\
  support custom template parameters\n\
- Custom Template: Animation Names using unusual template parameter names might\n\
  not be found by the conversion code.\n");
}

void CAnimationGraphDialog2::AnimationStatePopupMenu( CAGState2* pState, CWnd* wnd, CPoint &pos )
{
	if ( !pState )
	{
		__super::OnContextMenu( wnd, pos );
		return;
	}

	CMenu menu;
	CMenu viewMenu, fromMenu, toMenu, childMenu, activeParamMenu, excludedParamMenu, modifiersMenu;
	VERIFY( menu.CreatePopupMenu() );
	VERIFY( viewMenu.CreatePopupMenu() );
	VERIFY( fromMenu.CreatePopupMenu() );
	VERIFY( toMenu.CreatePopupMenu() );
	VERIFY( childMenu.CreatePopupMenu() );
	VERIFY( activeParamMenu.CreatePopupMenu() );
	VERIFY( excludedParamMenu.CreatePopupMenu() );
	VERIFY( modifiersMenu.CreatePopupMenu() );

	int id = 15000;
	for ( CAnimationGraph2::view_iterator it = m_pGraph->ViewBegin(); it != m_pGraph->ViewEnd(); ++it )
	{
		CAGView2Ptr pView = *it;
		if ( pView->HasState(pState) )
			viewMenu.AppendMenu( MF_STRING | ( m_pView == pView ? MF_CHECKED : 0 ), id++, pView->GetName() );
	}
	int idStates = id;
	std::vector< CAGState2Ptr > states;
	m_pGraph->StatesLinkedTo( states, pState );
	std::vector< CAGState2Ptr >::iterator it, itEnd = states.end();
	for ( it = states.begin(); it != itEnd; ++it )
	{
		CAGState2* pStateIt = *it;
		fromMenu.AppendMenu( MF_STRING, id++, pStateIt->GetName() );
	}
	states.clear();
	m_pGraph->StatesLinkedFrom( states, pState );
	itEnd = states.end();
	for ( it = states.begin(); it != itEnd; ++it )
	{
		CAGState2Ptr pStateIt = *it;
		toMenu.AppendMenu( MF_STRING, id++, pStateIt->GetName() );
	}
	states.clear();
	m_pGraph->ChildStates( states, pState );
	itEnd = states.end();
	for ( it = states.begin(); it != itEnd; ++it )
	{
		CAGState2Ptr pStateIt = *it;
		childMenu.AppendMenu( MF_STRING, id++, pStateIt->GetName() );
	}

	// Create active and excluded parameters menu lists
	int idStateParams = id;
	std::vector< TParameterizationId2 > paramIds;
	pState->GetParameterizations( paramIds );
	std::vector< TParameterizationId2 >::const_iterator itIds;
	for ( itIds = paramIds.begin(); itIds != paramIds.end(); ++itIds )
	{
		if ( pState->IsParameterizationExcluded( &(*itIds) ) )
		{
			excludedParamMenu.AppendMenu( MF_STRING, id++, itIds->AsString() );
		}
		else
		{
			activeParamMenu.AppendMenu( MF_STRING, id++, itIds->AsString() );
		}
	}

	int idModifiers = id;
	for ( int i = 0; i < pState->GetLinkedModifierCount(); ++i )
	{
		int modifierId = pState->GetLinkedModifier( i );
		CAG2ModifierBasePtr pModifier = m_pGraph->GetModifier( modifierId );
		CString modifierName = pModifier->GetListDisplayString();
		modifiersMenu.AppendMenu( MF_STRING, id++, modifierName );
	}


	menu.AppendMenu( MF_POPUP, (UINT) viewMenu.m_hMenu, _T("Find in View") );
	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_POPUP, (UINT) fromMenu.m_hMenu, _T("Linked From") );
	menu.AppendMenu( MF_POPUP, (UINT) toMenu.m_hMenu, _T("Linked To") );
	menu.AppendMenu( MF_POPUP, (UINT) modifiersMenu.m_hMenu, _T("Modifiers") );
	menu.AppendMenu( MF_POPUP, (UINT) childMenu.m_hMenu, _T("Child States") );
	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_POPUP, (UINT) activeParamMenu.m_hMenu, _T("Active Param Combinations") );
	menu.AppendMenu( MF_POPUP, (UINT) excludedParamMenu.m_hMenu, _T("Excluded Param Combinations") );

	if ( wnd == &m_view )
	{
		menu.AppendMenu( MF_SEPARATOR );
		menu.AppendMenu( MF_STRING, 32767, _T("Remove from View") );
		menu.AppendMenu( MF_STRING, 32766, _T("Remove and Disconnect") );
	}

	// track menu	
	int nMenuResult = CXTPCommandBars::TrackPopupMenu( &menu, TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, pos.x, pos.y, this, NULL );

	if ( nMenuResult == 32766 )
	{
		// Remove and Disconnect
		m_view.SendMessage( WM_COMMAND, ID_EDIT_DELETE, 0 );
	}
	else if ( nMenuResult == 32767 )
	{
		// Remove from View
		m_view.SendMessage( WM_COMMAND, ID_EDIT_DELETE_KEEP_LINKS, 0 );
	}
	if ( nMenuResult >= 15000 && nMenuResult < idStates )
	{
		CWaitCursor wait;
		CString view;
		viewMenu.GetMenuString( nMenuResult, view, MF_BYCOMMAND );
		CAGView2Ptr pView = m_pGraph->FindView( view );
		if ( pView != m_pView )
		{
			SetActiveView( pView );
			m_viewListCtrl.SelectItemByName( view );
		}
		CHyperNode* pNode = m_pView->GetNodeForState( pState );
		if ( pNode )
			m_view.ShowAndSelectNode( pNode, true );
		m_stateListCtrl.SelectItemByName( pState->GetName() );
		m_view.SetFocus();
	}
	else if ( nMenuResult >= idStates && nMenuResult < idStateParams )
	{
		CWaitCursor wait;
		CString state;
		if (!fromMenu.GetMenuString( nMenuResult, state, MF_BYCOMMAND ))
			if (!toMenu.GetMenuString( nMenuResult, state, MF_BYCOMMAND ))
				childMenu.GetMenuString( nMenuResult, state, MF_BYCOMMAND );

		HTREEITEM hState = m_stateListCtrl.GetRootItem();
		while ( hState && m_stateListCtrl.GetItemText(hState) != state )
			hState = m_stateListCtrl.GetNextItem( hState, TVGN_NEXT );
		if ( hState )
		{
			m_stateListCtrl.EnsureVisible( hState );
			m_stateListCtrl.SelectItem( hState );
			SetActiveState( m_pGraph->FindState(state) );
			if ( m_pView )
			{
				CHyperNode* pNode = m_pView->GetNodeForState( pState );
				if ( pNode )
					m_view.ShowAndSelectNode( pNode, true );
			}
		}
	}
	else if ( nMenuResult >= idStateParams && nMenuResult < idModifiers )
	{
		// TODO: Handle state params here...
	}
	else if ( nMenuResult >= idModifiers )
	{
		int linkedModifierIndex = nMenuResult - idModifiers;
		if ( pState->GetLinkedModifierCount() <= linkedModifierIndex )
		{
			return;
		}

		int modifierId = pState->GetLinkedModifier( linkedModifierIndex );
		CAG2ModifierBasePtr pModifier = m_pGraph->GetModifier( modifierId );

		SetActiveState( CAGState2Ptr( NULL ) );
		m_pGraph->GetModifierManager()->SetSelectedModifier( pModifier );

		HTREEITEM hState = m_modifierListCtrl.GetRootItem();
		while ( hState && m_modifierListCtrl.GetItemData( hState ) != modifierId )
		{
			hState = m_modifierListCtrl.GetNextItem( hState, TVGN_NEXT );
		}

		if ( ! hState )
		{
			return;
		}

		m_modifierListCtrl.EnsureVisible( hState );
		m_modifierListCtrl.SelectItem( hState );
	}
}

void CAnimationGraphDialog2::ModifierStatePopupMenu( CAG2ModifierBasePtr pModifier, CWnd* wnd, CPoint &pos )
{
	if ( !pModifier )
	{
		__super::OnContextMenu( wnd, pos );
		return;
	}

	CMenu menu;
	CMenu modifierMenu;
	VERIFY( menu.CreatePopupMenu() );
	VERIFY( modifierMenu.CreatePopupMenu() );

	if ( wnd == &m_view )
	{
		menu.AppendMenu( MF_SEPARATOR );
		menu.AppendMenu( MF_STRING, 32767, _T("Remove from View") );
	}
	menu.AppendMenu( MF_STRING, 32766, _T("Delete Completely") );

	// track menu	
	int nMenuResult = CXTPCommandBars::TrackPopupMenu( &menu, TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, pos.x, pos.y, this, NULL );

	if ( nMenuResult == 32766 )
	{
		// Delete Completely
		m_view.SendMessage( WM_COMMAND, ID_EDIT_DELETE, 0 );
		m_pGraph->GetModifierManager()->SetSelectedModifier(NULL);
		m_pGraph->RemoveModifier(pModifier);
		if (wnd == &m_modifierListCtrl)
		{
			CAGView2Ptr pView = m_pView;
			SetActiveView(NULL);
			SetActiveView(pView);
		}
	}
	else if ( nMenuResult == 32767 )
	{
		// Remove from View
		m_view.SendMessage( WM_COMMAND, ID_EDIT_DELETE_KEEP_LINKS, 0 );
	}

}

void CAnimationGraphDialog2::UpdateGraphMenu()
{
	CXTPCommandBar* pAddModifierBar = GetCommandBars()->GetMenuBar()->GetControl(2)->GetCommandBar()->GetControl(3)->GetCommandBar();

	if (pAddModifierBar)
	{
		CMenu* subMenu = new CMenu();
		subMenu->CreatePopupMenu();
		CModifierManager* pModManager = m_pGraph->GetModifierManager();
		int modCount = pModManager->GetRegisteredModifierCount();
		for (int i = 0; i < modCount; ++i)
		{
			subMenu->AppendMenu( MF_STRING, ID_GRAPH_ADDMODIFIER_FIRST + i, pModManager->GetModifierHumanReadableName(i));
		}

		pAddModifierBar->LoadMenu(subMenu);

		for (int i = 0; i < modCount; ++i)
		{
			// add the className as additional data and use the human readable name here instead
			CXTPControl* menuEntry = pAddModifierBar->GetControl(i);
			menuEntry->SetDescription(pModManager->GetModifierClassName(i));
		}
	}
}

void CAnimationGraphDialog2::OnGraphAddModifier( UINT nID )
{
	// Menu Call:
	// Graph Menu-->Add Modifier-->[dynamic list of available modifier classes]
	if ( m_pGraph == NULL )
		return;

	CXTPCommandBar* pAddModifierBar = GetCommandBars()->GetMenuBar()->GetControl(2)->GetCommandBar()->GetControl(3)->GetCommandBar();

	CRY_ASSERT_MESSAGE(pAddModifierBar, "Could not retrieve Modifier Menu!");

	CXTPControl* menuEntry = pAddModifierBar->GetControl(nID - ID_GRAPH_ADDMODIFIER_FIRST);
	CString caption = menuEntry->GetDescription();
	//CString caption = menuEntry->GetCaption();
	CAG2ModifierBasePtr newMod = m_pGraph->AddNewModifier(caption);
	CRY_ASSERT_MESSAGE(newMod, "No Modifier Created!");

	m_modifierListCtrl.ReloadModifiers();
	m_modifierListCtrl.SelectItemByName(newMod->GetListDisplayString());
	m_modifierListCtrl.SetActiveWindow();
}

void CAnimationGraphDialog2::OnModifierListSelChanged( NMHDR* pNMHDR, LRESULT* pResult )
{
	*pResult = TRUE;
	if (!m_pGraph)
		return;

	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM treeitem = pNMTreeView->itemNew.hItem;
	if (treeitem != NULL)
	{
		m_inputListCtrl.SelectItem(NULL);
		m_stateListCtrl.SelectItem(NULL);
		SetActiveState( NULL );
		m_vStates.clear();
		m_view.ClearSelection();
		m_previewManager.SetState(CAGState2Ptr(0));

		// set the active modifier so it's details appear
		int modifierId = m_modifierListCtrl.GetItemData(treeitem);
		m_pGraph->GetModifierManager()->SetSelectedModifier( m_pGraph->GetModifier(modifierId) );
	}
}

void CAnimationGraphDialog2::OnModifierEvent( EAG2ModifierEvent evt, CAG2ModifierBasePtr pModifier )
{
	switch (evt)
	{
	case eAG2_Modifier_Delete:
		m_modifierListCtrl.ReloadModifiers();
		//SetActiveView( m_pView );
		break;
	}
}

void CAnimationGraphDialog2::OnModifierListBeginDrag( NMHDR* pNMHDR, LRESULT* pResult )
{
	NM_TREEVIEW * pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	*pResult = TRUE;
	bool doDrag = false;

	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	m_dragModifierId = m_modifierListCtrl.GetItemData(hItem);
	CString nodeName;
	nodeName.Format("$modifier %i", m_dragModifierId);
	m_dragNodeClass = nodeName;

	doDrag = !!m_pView && m_pView->CanAddModifier( m_pGraph->GetModifier(m_dragModifierId) );

	if (doDrag)
	{
		m_pDragImage = m_stateListCtrl.CreateDragImage( hItem );
		if (m_pDragImage)
		{
			m_pDragImage->BeginDrag(0, CPoint(-10, -10));
			CRect rc;
			GetWindowRect(rc);
			CPoint p = pNMTreeView->ptDrag;
			ClientToScreen( &p );
			p.x -= rc.left;
			p.y -= rc.top;
			m_pDragImage->DragEnter( this, p );
			SetCapture();

			*pResult = FALSE;
		}
	}
}

void CAnimationGraphDialog2::OnStateNodeEvent( EHyperGraphEvent event )
{
	switch (event)
	{
	case EHG_NODE_CHANGE:
		{
			std::vector< CHyperNode* > nodes;
			m_view.GetSelectedNodes( nodes, CHyperGraphView::SELECTION_SET_ONLY_PARENTS );
			if ( nodes.size() == 1 )
			{
				SetActiveState( ((CAGNode2*)nodes[0])->GetState() );
			}
			else if ( nodes.size() > 1 )
			{
				if ( m_pView )
				{
					std::vector< CAGState2Ptr > states;
					std::vector< CHyperNode* >::iterator it, itEnd = nodes.end();
					for ( it = nodes.begin(); it != itEnd; ++it )
						if ( CAGState2* pState = ((CAGNode2*)*it)->GetState() )
							states.push_back( pState );
					SetActiveStates( states );
				}
			}
			else
			{
				SetActiveState( NULL );
				if ( m_pView && !nodes.size() )
					m_view.ClearSelection();
			}
		}
		break;
	}
}

void CAnimationGraphDialog2::OnModifierNodeEvent( CAGNodeBase2* pBaseNode, EHyperGraphEvent event )
{
	switch (event)
	{
	case EHG_NODE_CHANGE:
		{
			std::vector< CHyperNode* > nodes;
			m_view.GetSelectedNodes( nodes, CHyperGraphView::SELECTION_SET_ONLY_PARENTS );
			if ( nodes.size() == 1 )
			{
				m_pGraph->GetModifierManager()->SetSelectedModifier(((CAG2ModifierNode*) pBaseNode)->GetModifier());
			}
			else
			{
				// if nothing was selected
				SetActiveState( NULL );
				m_pGraph->GetModifierManager()->SetSelectedModifier( NULL );
				m_view.ClearSelection();
			}
			break;

		}
	}

}

