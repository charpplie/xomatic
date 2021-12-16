#include "StdAfx.h"
#include "AnimationGraphStateEditor_2.h"
#include "AnimationGraphDialog_2.h"
#include "resource.h"
#include "StringDlg.h"
#include "Controls/PropertyItem.h"

namespace
{

	int ShowQuickPopup( const std::vector<CString>& elems, CWnd * pWnd )
	{
		CMenu menu;
		menu.CreatePopupMenu();
		for (std::vector<CString>::const_iterator iter = elems.begin(); iter != elems.end(); ++iter)
			menu.AppendMenu( MF_STRING, iter - elems.begin() + 1, *iter );

		CPoint p;
		::GetCursorPos(&p);
		return menu.TrackPopupMenuEx( TPM_RETURNCMD|TPM_LEFTBUTTON|TPM_TOPALIGN|TPM_LEFTALIGN, p.x, p.y, pWnd, NULL ) - 1;
	}

}


//IMPLEMENT_DYNAMIC(CFullSizePropertyCtrl, CPropertyCtrl)

BEGIN_MESSAGE_MAP(CFullSizePropertyCtrl2, CPropertyCtrl)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL CFullSizePropertyCtrl2::OnEraseBkgnd(CDC* pDC)
{
	CPropertyCtrl::OnEraseBkgnd(pDC);

	CRect rcWnd, rcClient;
	GetWindowRect( rcWnd );
	GetClientRect( rcClient );
	int nPage = rcClient.Height();
	int h = GetVisibleHeight();

	if ( nPage != h )
	{
		//CPropertyCtrl::OnEraseBkgnd(pDC);

		CXTPTaskPanel* panel = (CXTPTaskPanel*) GetParent();
		if ( m_pGroup && m_pGroup->IsExpanded() )
		{
			panel->ExpandGroup( m_pGroup, FALSE );
			SetWindowPos( NULL, 0, 0, rcWnd.Width(), rcWnd.Height()+h-nPage, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOREDRAW );
			m_pItem->SetControlHandle( GetSafeHwnd() );
			panel->Reposition(FALSE);
			panel->ExpandGroup( m_pGroup, TRUE );
			Invalidate();
		}
	}

	return TRUE;
}

/*
 * CPropGroup
 */

CAnimationGraphStateEditor2::CPropGroup::CPropGroup( CAnimationGraphStateEditor2 * pParent, const CString& name, UINT id, const TAGPropMap& props, const Verb * pVerbs )
{
	CommonInit( pParent, name, id, pVerbs );

	// create the property editor
	CRect rc(0,0,220,500);
	m_pProps = new CFullSizePropertyCtrl2();
	m_pProps->Create( WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS, rc, &m_pParent->m_panel );
	m_pProps->ModifyStyle( 0, WS_CLIPCHILDREN|WS_CLIPSIBLINGS );
	m_pProps->SetItemHeight(16);
	m_pProps->ExpandAll();

	m_pItem = m_pGroup->AddControlItem( m_pProps->GetSafeHwnd() );
	m_pItem->SetCaption("Properties");

	m_pProps->m_pGroup = m_pGroup;
	m_pProps->m_pItem = m_pItem;

	Reset(props);
}

CAnimationGraphStateEditor2::CPropGroup::CPropGroup( CAnimationGraphStateEditor2 * pParent, const CString& name, UINT id, const std::vector<CString>& props, const Verb * pVerbs )
{
	CommonInit( pParent, name, id, pVerbs );

	CRect rc(0,0,220,500);
	m_pList = new CListBox();
	m_pList->Create( WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VSCROLL|WS_HSCROLL, rc, &m_pParent->m_panel, 400 );
	m_pList->SetFont( CFont::FromHandle((HFONT)gSettings.gui.hSystemFont) );

	m_pItem = m_pGroup->AddControlItem( m_pList->GetSafeHwnd() );
	m_pItem->SetCaption("Properties");

	Reset(props);
}

void CAnimationGraphStateEditor2::CPropGroup::CommonInit( CAnimationGraphStateEditor2 * pParent, const CString& name, UINT id, const Verb * pVerbs )
{
	m_pParent = pParent;
	m_id = id;

	m_pItem = 0;
	m_pGroup = 0;
	m_pProps = 0;
	m_pList = 0;

	// create a group for us
	m_pGroup = m_pParent->m_panel.AddGroup(id);
	m_pGroup->SetCaption(name);
	m_pGroup->SetItemLayout(xtpTaskItemLayoutDefault);

	if (pVerbs)
	{
		for (int i=0; !pVerbs[i].first.IsEmpty(); i++)
		{
			CXTPTaskPanelGroupItem * pGI = m_pGroup->AddLinkItem( 100*id + i );
			pGI->SetType( xtpTaskItemTypeLink );
			pGI->SetCaption( pVerbs[i].first );
			m_callbacks.push_back( pVerbs[i].second );
		}
	}
}

void CAnimationGraphStateEditor2::CPropGroup::Reset( const TAGPropMap& props )
{
	UpdateLock updateLock(m_pParent);

	CString selectedName;
	CPropertyItem* pSelectedItem = m_pProps->GetSelectedItem();
	if ( pSelectedItem )
		selectedName = pSelectedItem->GetName();

	m_pProps->RemoveAllItems();
	for (TAGPropMap::const_iterator iter = props.begin(); iter != props.end(); ++iter)
	{
		if (iter->first == "")
			m_pProps->AddVarBlock( iter->second );
		else
		{
			int pos = 0;
			CString category = iter->first.Tokenize( "/", pos );
			CString name = iter->first.Tokenize( NULL, pos );
			if ( name.IsEmpty() )
			{
				m_pProps->AddVarBlock( iter->second, category );
			}
			else
			{
				CPropertyItem* pRoot = m_pProps->GetRootItem();
				pRoot = pRoot->FindItemByFullName( "::" + category );
				if ( !pRoot )
					pRoot = m_pProps->GetRootItem();
				m_pProps->AddVarBlockAt( iter->second, name, pRoot, true )->SetExpanded( true );
			}
		}
	}
	m_pProps->ExpandAll();
	CPropertyItem* pUnused = m_pProps->GetRootItem() ? m_pProps->GetRootItem()->FindItemByFullName("::Unused") : NULL;
	if ( pUnused )
		pUnused->SetExpanded( false );

	// get things positioned correctly
	CRect rc;
	m_pParent->m_panel.ExpandGroup( m_pGroup, FALSE );
	m_pParent->m_panel.GetClientRect(rc);
	int h = m_pProps->GetVisibleHeight();
	m_pProps->SetWindowPos( NULL, 0, 0, rc.right, h+2, SWP_NOMOVE );
	m_pItem->SetControlHandle( m_pProps->GetSafeHwnd() );
	m_pParent->m_panel.Reposition(FALSE);
	m_pParent->m_panel.ExpandGroup( m_pGroup, TRUE );
	m_pProps->Invalidate();
}

void CAnimationGraphStateEditor2::CPropGroup::Reset( const std::vector<CString>& props )
{
	UpdateLock updateLock(m_pParent);

	m_pList->ResetContent();
	for (int i=0; i<props.size(); i++)
	{
		m_pList->AddString( props[i] );
	}

	// get things positioned correctly
	CRect rc;
	m_pParent->m_panel.ExpandGroup( m_pGroup, FALSE );
	m_pParent->m_panel.GetClientRect(rc);
	m_pList->SetWindowPos( NULL, 0, 0, rc.right, 500, SWP_NOMOVE );
	m_pItem->SetControlHandle( m_pList->GetSafeHwnd() );
	m_pParent->m_panel.Reposition(FALSE);
	m_pParent->m_panel.ExpandGroup( m_pGroup, TRUE );
}

void CAnimationGraphStateEditor2::CPropGroup::OnCommand( UINT id )
{
	if (id > m_callbacks.size())
		return;
	m_pParent->StateEvent(m_callbacks[id]);
}

void CAnimationGraphStateEditor2::CPropGroup::Disable()
{
	if ( m_pProps )
	{
		m_pProps->ClearSelection();
		m_pProps->SetFlags( m_pProps->GetFlags() | CPropertyCtrl::F_READ_ONLY );
	}

	int i = m_callbacks.size();
	while ( i-- )
		m_pGroup->FindItem( 100*m_id+i )->SetEnabled( FALSE );
}

CAnimationGraphStateEditor2::CPropGroup::~CPropGroup()
{
	m_pItem->Remove();
	m_pGroup->Remove();
	delete m_pProps;
}

/*
 * CPropEditorImpl<CAGStatePtr>
 */

CAnimationGraphStateEditor2::CPropEditorImpl<CAGState2Ptr>::CPropEditorImpl( CAnimationGraphStateEditor2 * pSE, CAGState2Ptr pCurState )
: CPropEditor(pSE, ePET_State)
, m_pCurState(pCurState)
{
	m_pCurState->GetGraph()->AddListener(this);
}

CAnimationGraphStateEditor2::CPropEditorImpl<CAGState2Ptr>::~CPropEditorImpl()
{
	m_pCurState->GetGraph()->RemoveListener(this);
}

void CAnimationGraphStateEditor2::CPropEditorImpl<CAGState2Ptr>::OnStateEvent( EAGStateEvent2 evt, CAGState2Ptr pState )
{
	if (pState != m_pCurState)
		return;

	switch (evt)
	{
	case eAGSE_ChangeParent2:
		{
			UpdateLock updateLock(m_pSE);
			OnEvent(eSEE_RebuildCriteria2);
			OnEvent(eSEE_RebuildExtra2);
		}
		break;
	case eAGSE_CanNotChangeName2:
		if ( m_pGeneralPanel )
		{
			CPropertyCtrl* pCtrl = m_pGeneralPanel->GetCtrl();
			CPropertyItem* pItem = pCtrl->GetRootItem();
			pItem = pItem->FindItemByFullName( "::State name" );
			pItem->GetVariable()->Set( pState->GetName() );
		}
		break;
	}
}

void CAnimationGraphStateEditor2::CPropEditorImpl<CAGState2Ptr>::BuildExtras()
{
	UpdateLock updateLock(m_pSE);

	static const Verb verbsExtras[] = 
	{
		Verb("Add", eSEE_AddExtra2),
		Verb("Remove", eSEE_RemoveExtra2),
		Verb()
	};
	//m_pExtrasPanel = m_pSE->AddPropertyGroup("Extra Properties", m_pCurState->GetExtraProperties(), verbsExtras);

	m_pCriteriaPanel = m_pSE->AddPropertyGroup("Selection Criteria", m_pCurState->GetCriteriaProperties(m_pSE));
}

void CAnimationGraphStateEditor2::CPropEditorImpl<CAGState2Ptr>::OnEvent( EStateEditorEvent2 evt )
{
	switch (evt)
	{
	case eSEE_InitContainer2:
		{
			static const Verb verbsGeneral[] =
			{
				Verb("Clone...", eSEE_Clone2),
				Verb("Delete...", eSEE_Delete2),
				Verb()
			};
			m_pGeneralPanel = m_pSE->AddPropertyGroup("General", m_pCurState->GetGeneralProperties(), verbsGeneral);
			BuildExtras();

			// activate the node details (for an animation node?) here
			// This class should hold pointers to all of these detail panes
			// the are actually created and owned by the state editor however
			// they get passed a pointer upon creation
			// the panes need to have a reinitialize function or similar to
			//	set new nodes and their data
			m_pSE->GetAnimNodeDetailsPane()->SetActiveAnimNode(m_pCurState);
		}
		break;

	case eSEE_UpdateNodeDetailsPane:
			// AG 1.5 --> Update the information displayed on the details panel
			m_pSE->GetAnimNodeDetailsPane()->UpdateInformationDisplay();
		break;

	case eSEE_AddExtra2:
		{
			std::vector<CString> items;
			CAnimationGraph2Ptr pGraph = m_pSE->m_pParent->GetAnimationGraph();
			for (CAnimationGraph2::state_factory_iterator iter = pGraph->StateFactoryBegin(); iter != pGraph->StateFactoryEnd(); ++iter)
			{        
				CAGCategory2Ptr pCat = (*iter)->GetCategory();
				if (!pCat)
					continue;
				if (!pCat->IsOverridable() && pCat->IsUsableWithTemplate())
					items.push_back( (*iter)->GetHumanName() );
			}
			std::sort(items.begin(), items.end());
			int item = ShowQuickPopup( items, &m_pSE->m_panel );
			if (item < 0)
				return;
			UpdateLock updateLock(m_pSE);

			m_pCurState->AddExtraState( pGraph->FindStateFactory(items[item]) );
			OnEvent(eSEE_RebuildExtra2);
		}
		break;
	case eSEE_RemoveExtra2:
		{
			std::vector<CString> items;
			m_pCurState->GetExtraStateStrings( items );
			int item = ShowQuickPopup( items, &m_pSE->m_panel );
			if (item < 0)
				return;
			UpdateLock updateLock(m_pSE);

			m_pCurState->RemoveExtraState( item );
			if (m_pExtrasPanel)
				m_pExtrasPanel->Reset( m_pCurState->GetExtraProperties() );
		}
		break;
	case eSEE_RebuildExtra2:
		{
			UpdateLock updateLock(m_pSE);
			if (m_pExtrasPanel)
				m_pExtrasPanel->Reset( m_pCurState->GetExtraProperties() );
		}
		break;

	case eSEE_RebuildCriteria2:
		{
			UpdateLock updateLock(m_pSE);

			if (m_pCriteriaPanel)
				m_pCriteriaPanel->Reset( m_pCurState->GetCriteriaProperties(m_pSE) );
		}
		break;
	case eSEE_Clone2:
		{
			if (IDYES == AfxMessageBox( "Clone " + m_pCurState->GetName() + "?\n", MB_YESNO ))
			{
				m_pCurState->GetGraph()->CloneState( m_pCurState );
			}	
		}
		break;
	case eSEE_Delete2:
		{
			IAGCheckedOperation2Ptr pOp = m_pCurState->CreateDeleteOp();
			if (IDYES == AfxMessageBox( "Delete " + m_pCurState->GetName() + "?\n\n" + pOp->GetExplanation(), MB_YESNO ))
				pOp->Perform();
			m_pSE->ClearActiveItem();
		}
		break;
	case eSEE_RebuildExtraPanels2:
		m_pSE->m_propGroups.resize(1);
		BuildExtras();
		break;
	}
}

bool CAnimationGraphStateEditor2::CPropEditorImpl<CAGState2Ptr>::CheckSameEditor( CPropEditor* pEditor )
{
	return m_pCurState == static_cast<CPropEditorImpl*>(pEditor)->m_pCurState;
}

void CAnimationGraphStateEditor2::CPropEditorImpl<CAGState2Ptr>::ActivateParameterization( const TParameterizationId2* pParamId, const CString& viewName )
{
	m_pCurState->ActivateParameterization( pParamId, viewName );
}

/*
* CPropEditorImpl< std::vector< CAGStatePtr > >
*/

CAnimationGraphStateEditor2::CPropEditorImpl< std::vector< CAGState2Ptr > >::CPropEditorImpl( CAnimationGraphStateEditor2 * pSE, std::vector< CAGState2Ptr >& vCurStates )
: CPropEditor( pSE, ePET_States )
{
	m_vCurStates = vCurStates;
}

CAnimationGraphStateEditor2::CPropEditorImpl< std::vector< CAGState2Ptr > >::~CPropEditorImpl()
{
}

void CAnimationGraphStateEditor2::CPropEditorImpl< std::vector< CAGState2Ptr > >::OnEvent( EStateEditorEvent2 evt )
{
	switch (evt)
	{
	case eSEE_InitContainer2:
		{
			static const Verb verbsGeneral[] =
			{
				Verb("Delete All...", eSEE_Delete2),
				Verb()
			};
			TAGPropMap props;
			m_pGeneralPanel = m_pSE->AddPropertyGroup("General", props, verbsGeneral);
//			BuildExtras();
		}
		break;
/*
		case eSEE_AddExtra2:
		{
			std::vector<CString> items;
			CAnimationGraphPtr pGraph = m_pSE->m_pParent->GetAnimationGraph();
			for (CAnimationGraph::state_factory_iterator iter = pGraph->StateFactoryBegin(); iter != pGraph->StateFactoryEnd(); ++iter)
			{        
				CAGCategoryPtr pCat = (*iter)->GetCategory();
				if (!pCat)
					continue;
				if (!pCat->IsOverridable() && (!m_pCurState->IsTemplateState() || pCat->IsUsableWithTemplate())) 
					items.push_back( (*iter)->GetHumanName() );
			}
			std::sort(items.begin(), items.end());
			int item = ShowQuickPopup( items, &m_pSE->m_panel );
			if (item < 0)
				return;
			UpdateLock updateLock(m_pSE);

			m_pCurState->AddExtraState( pGraph->FindStateFactory(items[item]) );
			OnEvent(eSEE_RebuildExtra);
		}
		break;
	case eSEE_RemoveExtra2:
		{
			std::vector<CString> items;
			m_pCurState->GetExtraStateStrings( items );
			int item = ShowQuickPopup( items, &m_pSE->m_panel );
			if (item < 0)
				return;
			UpdateLock updateLock(m_pSE);

			m_pCurState->RemoveExtraState( item );
			if (m_pExtrasPanel)
				m_pExtrasPanel->Reset( m_pCurState->GetExtraProperties() );
		}
		break;
	case eSEE_RebuildExtra2:
		{
			UpdateLock updateLock(m_pSE);
			if (m_pExtrasPanel)
				m_pExtrasPanel->Reset( m_pCurState->GetExtraProperties() );
		}
		break;
	case eSEE_RebuildCriteria2:
		{
			UpdateLock updateLock(m_pSE);

			if (m_pCriteriaPanel)
				m_pCriteriaPanel->Reset( m_pCurState->GetCriteriaProperties(m_pSE) );
		}
		break;

	case eSEE_RebuildOverridable2:
		{
			UpdateLock updateLock(m_pSE);

			if (m_pOverridablePanel)
				m_pOverridablePanel->Reset( m_pCurState->GetOverridableProperties(m_pSE) );
		}
		break;
	case eSEE_RebuildTemplate2:
		{
			UpdateLock updateLock(m_pSE);

			if (m_pTemplatePanel)
				m_pTemplatePanel->Reset( m_pCurState->GetTemplateProperties(m_pSE) );
			OnEvent(eSEE_RebuildCriteria);
		}
		break;

	case eSEE_RebuildExtraPanels2:
		m_pSE->m_propGroups.resize(1);
		BuildExtras();
		break;
		*/
	case eSEE_Delete2:
		{
			CString explanation;
			explanation.Format( "Delete %d states?\n", m_vCurStates.size() );
			std::vector< CAGState2Ptr >::iterator it, itEnd = m_vCurStates.end();
			std::vector< IAGCheckedOperation2Ptr > operations;
			int i = 0;
			for ( it = m_vCurStates.begin(); it != itEnd; ++it )
			{
				CAGState2* pState = *it;
				IAGCheckedOperation2Ptr pOp = pState->CreateDeleteOp();
				operations.push_back( pOp );
				if ( ++i == 11 )
				{
					explanation += "\n<...>";
				}
				else if ( i < 11 )
				{
					explanation += '\n';
					explanation += pState->GetName();
					explanation += ":\n";
					explanation += pOp->GetExplanation();
					explanation += '\n';
				}
			}
			if (IDYES == AfxMessageBox( explanation, MB_YESNO ))
			{
				for ( int i = 0; i < operations.size(); ++i )
					operations[i]->Perform();
			}
			m_pSE->ClearActiveItem();
		}
		break;
	}
}

bool CAnimationGraphStateEditor2::CPropEditorImpl< std::vector< CAGState2Ptr > >::CheckSameEditor( CPropEditor* pEditor )
{
	return m_vCurStates == static_cast<CPropEditorImpl*>(pEditor)->m_vCurStates;
}

/*
 * CPropEditorImpl<CAGViewPtr>
 */

CAnimationGraphStateEditor2::CPropEditorImpl<CAGView2Ptr>::CPropEditorImpl( CAnimationGraphStateEditor2 * pSE, CAGView2Ptr pCurView )
: CPropEditor(pSE, ePET_View)
, m_pCurView(pCurView)
{
}

CAnimationGraphStateEditor2::CPropEditorImpl<CAGView2Ptr>::~CPropEditorImpl()
{
}

void CAnimationGraphStateEditor2::CPropEditorImpl<CAGView2Ptr>::OnEvent( EStateEditorEvent2 evt )
{
	switch (evt)
	{
	case eSEE_InitContainer2:
		{
			static const Verb verbsGeneral[] =
			{
				Verb("Delete...", eSEE_Delete2),
				Verb()
			};
			m_pSE->AddPropertyGroup("General", m_pCurView->GetGeneralProperties(), verbsGeneral);
		}
		break;
	case eSEE_Delete2:
		{
			if ( IDYES == AfxMessageBox( "Delete " + m_pCurView->GetName() + "?", MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION ))
				m_pCurView->GetGraph()->RemoveView( m_pCurView );
		}
		m_pSE->ClearActiveItem();
		break;
	}
}

bool CAnimationGraphStateEditor2::CPropEditorImpl<CAGView2Ptr>::CheckSameEditor( CPropEditor* pEditor )
{
	return m_pCurView == static_cast<CPropEditorImpl*>(pEditor)->m_pCurView;
}

/*
 * CPropEditorImpl<CAGInputPtr>
 */

CAnimationGraphStateEditor2::CPropEditorImpl<CAGInput2Ptr>::CPropEditorImpl( CAnimationGraphStateEditor2 * pSE, CAGInput2Ptr pCurInput )
: CPropEditor(pSE, ePET_Input)
, m_pCurInput(pCurInput)
{
	m_pCurInput->GetGraph()->AddListener( this );
}

CAnimationGraphStateEditor2::CPropEditorImpl<CAGInput2Ptr>::~CPropEditorImpl()
{
	m_pCurInput->GetGraph()->RemoveListener( this );
}

void CAnimationGraphStateEditor2::CPropEditorImpl<CAGInput2Ptr>::OnEvent( EStateEditorEvent2 evt )
{
	switch (evt)
	{
	case eSEE_InitContainer2:
		{
			static const Verb verbsGeneral[] =
			{
				Verb("Delete", eSEE_Delete2),
				Verb()
			};
			m_pSE->AddPropertyGroup("General", m_pCurInput->GetGeneralProperties(), verbsGeneral);
			OnEvent( eSEE_RebuildInputTypedProperties2 );
		}
		break;
	case eSEE_RebuildInputTypedProperties2:
		{
			UpdateLock updateLock( m_pSE );
			if ( m_pSE->m_propGroups.size() >= 2 )
				m_pSE->m_propGroups.erase( m_pSE->m_propGroups.begin()+1 );

			static const Verb verbs[] = 
			{
				Verb("Add", eSEE_AddValue2),
				Verb("Remove", eSEE_RemoveValue2),
				Verb("Rename", eSEE_RenameValue2),
				Verb()
			};

			switch (m_pCurInput->GetType())
			{
			case eAGIT_Float:
				m_pSE->AddPropertyGroup("Float Properties", m_pCurInput->GetFloatProperties());
				break;
			case eAGIT_Integer:
				m_pSE->AddPropertyGroup("Integer Properties", m_pCurInput->GetIntegerProperties());
				break;
			case eAGIT_String:
				m_pSE->AddPropertyGroup("Key Properties", m_pCurInput->GetKeyProperties(), verbs);
				break;
			default:
				assert(false);
			}
		}
		break;
	case eSEE_RebuildInputPanel2:
		{
			UpdateLock updateLock( m_pSE );
			m_pSE->m_propGroups.clear();
			OnEvent(eSEE_InitContainer2);
		}
		break;
	case eSEE_AddValue2:
		{
			CString name;
			for (int i=0; ; i++)
			{
				name.Format("input%d", i);
				if (!m_pCurInput->HasKey(name))
					break;
			}
			CStringDlg dlg;
			dlg.m_strString = name;
			if (dlg.DoModal() == IDOK)
				m_pCurInput->AddKey(dlg.m_strString);
		}
		OnEvent(eSEE_RebuildInputPanel2);
		break;
	case eSEE_RemoveValue2:
		{
			CListBox * pListBox = m_pSE->m_propGroups.back()->GetList();
			int sel = pListBox->GetCurSel();
			if (sel != LB_ERR)
			{
				CString value;
				pListBox->GetText( sel, value );
				m_pCurInput->RemoveKey( value );
			}
		}
		OnEvent(eSEE_RebuildInputPanel2);
		break;
	case eSEE_Delete2:
		{
			m_pCurInput->GetGraph()->RemoveInput(m_pCurInput);
			m_pSE->ClearActiveItem();
		}
		break;
	case eSEE_RenameValue2:
		{
			CListBox * pListBox = m_pSE->m_propGroups.back()->GetList();
			int sel = pListBox->GetCurSel();
			if (sel != LB_ERR)
			{
				CString value;
				pListBox->GetText( sel, value );
				CStringDlg dlg;
				dlg.m_strString = value;
				if (dlg.DoModal() == IDOK)
				{
					m_pCurInput->RemoveKey(value);
					m_pCurInput->AddKey(dlg.m_strString);
				}
			}
		}
		OnEvent(eSEE_RebuildInputPanel2);
		break;
	}
}

bool CAnimationGraphStateEditor2::CPropEditorImpl<CAGInput2Ptr>::CheckSameEditor( CPropEditor* pEditor )
{
	return m_pCurInput == static_cast<CPropEditorImpl*>(pEditor)->m_pCurInput;
}

void CAnimationGraphStateEditor2::CPropEditorImpl<CAGInput2Ptr>::OnInputEvent( EAGInputEvent2 event, CAGInput2Ptr pInput )
{
	if (m_pCurInput != pInput)
		return;

	switch (event)
	{
	case eAGIE_ChangeType2:
		OnEvent( eSEE_RebuildInputTypedProperties2 );
		break;
	}
}

/*
 * CPropEditorImpl<CAGLinkPtr>
 */

CAnimationGraphStateEditor2::CPropEditorImpl<CAGLink2Ptr>::CPropEditorImpl( CAnimationGraphStateEditor2 * pSE, CAGLink2Ptr pCurLink )
: CPropEditor(pSE, ePET_Link)
, m_pCurLink(pCurLink)
{
	m_pCurLink->GetGraph()->AddListener( this );
}

CAnimationGraphStateEditor2::CPropEditorImpl<CAGLink2Ptr>::~CPropEditorImpl()
{
	m_pCurLink->GetGraph()->RemoveListener( this );
}

void CAnimationGraphStateEditor2::CPropEditorImpl<CAGLink2Ptr>::OnLinkEvent( EAGLinkEvent2 event, CAGLink2Ptr pLink )
{
	if (m_pCurLink != pLink)
		return;

	switch (event)
	{
	case eAGLE_ChangeMapping2:
		m_pSE->m_pParent->PostMessage( WM_COMMAND, ID_AGLINK_MAPPINGCHANGED, 0 );
		break;
	}
}

void CAnimationGraphStateEditor2::CPropEditorImpl<CAGLink2Ptr>::OnEvent( EStateEditorEvent2 evt )
{
	switch (evt)
	{
	case eSEE_InitContainer2:
		{
			UpdateLock updateLock( m_pSE );

			m_pSE->m_propGroups.resize(0);

			static const Verb verbsGeneral[] =
			{
				Verb("Delete", eSEE_Delete2),
				Verb()
			};
			m_pSE->AddPropertyGroup("General", m_pCurLink->GetGeneralProperties(), verbsGeneral);

			CAGState2Ptr from, to;
			m_pCurLink->GetLinkedStates( from, to );
			assert( from && to );
			if ( from->GetParamsDeclaration()->empty() && to->GetParamsDeclaration()->empty() )
				break;

			CParamsDeclaration2* fromParams = from->GetParamsDeclaration();
			CParamsDeclaration2* toParams = to->GetParamsDeclaration();

			m_pSE->AddPropertyGroup("Parameters Mapping", m_pCurLink->GetMappingProperties());
		}
		break;
	}
}

bool CAnimationGraphStateEditor2::CPropEditorImpl<CAGLink2Ptr>::CheckSameEditor( CPropEditor* pEditor )
{
	return m_pCurLink == static_cast<CPropEditorImpl*>(pEditor)->m_pCurLink;
}

/*
 * class proper
 */

CXTPDockingPane * CAnimationGraphStateEditor2::Init( CAnimationGraphDialog2 * pParent, UINT id )
{
	m_pParent = pParent;

	CRect rc(0,0,250,500);
	CXTPDockingPane * pDockPane = pParent->GetDockingPaneManager()->CreatePane( id, rc, dockRightOf );
	pDockPane->SetTitle( _T("Properties") );

	m_panel.Create( WS_CHILD|/*WS_VISIBLE|*/WS_CLIPSIBLINGS|WS_CLIPCHILDREN, rc, m_pParent, 0 );
	m_panel.SetBehaviour(xtpTaskPanelBehaviourExplorer);
	m_panel.SetTheme(xtpTaskPanelThemeNativeWinXP);
	m_panel.SetSelectItemOnFocus(TRUE);
	m_panel.AllowDrag(FALSE);
	m_panel.SetAnimation( xtpTaskPanelAnimationNo );
	m_panel.GetPaintManager()->m_rcGroupOuterMargins.SetRect(0,0,0,0);
	m_panel.GetPaintManager()->m_rcGroupInnerMargins.SetRect(0,0,0,0);
	m_panel.GetPaintManager()->m_rcItemOuterMargins.SetRect(0,0,0,0);
	m_panel.GetPaintManager()->m_rcItemInnerMargins.SetRect(1,1,1,1);
	m_panel.GetPaintManager()->m_rcControlMargins.SetRect(2,0,2,0);
	m_panel.GetPaintManager()->m_nGroupSpacing = 0;

	m_AnimationNodeDetails.Create( m_AnimationNodeDetails.IDD, 	m_pParent->GetNodeDetailsParent() );
	//m_AnimationNodeDetails.ModifyStyle( 0, WS_VSCROLL );
	//m_AnimationNodeDetails.ModifyStyleEx( 0, WS_EX_STATICEDGE );
	m_AnimationNodeDetails.Init();
	m_AnimationNodeDetails.SetOwner(m_pParent);

	return pDockPane;
}

void CAnimationGraphStateEditor2::ClearActiveItem()
{
	UpdateLock updateLock(this);
	if ( m_pCurEdit )
		m_pCurEdit->ActivateParameterization( NULL, "" );
	m_pCurEdit = NULL;
	m_propGroups.resize(0);
	m_pParent->SetParamsDeclaration( NULL );

	// remove everything that could be on the details pane
	if (m_AnimationNodeDetails)
		m_AnimationNodeDetails.ShowWindow( SW_HIDE );

}

void CAnimationGraphStateEditor2::SetEditor( _smart_ptr<CPropEditor> pImpl )
{
	assert( !!pImpl );
	if (m_pCurEdit)
	{
		if (pImpl->IsSameEditor(m_pCurEdit))
			return;
		m_pCurEdit->ActivateParameterization( NULL, "" );
	}
	UpdateLock updateLock(this);
	m_pCurEdit = pImpl;
	m_propGroups.resize(0);
	m_pCurEdit->OnEvent( eSEE_InitContainer2 );
//	m_panel.Invalidate();

	m_pParent->SetParamsDeclaration( m_pCurEdit->GetParamsDeclaration() );
}

CAnimationGraphStateEditor2::CPropGroupPtr CAnimationGraphStateEditor2::AddPropertyGroup( const CString& name, const TAGPropMap& props, const Verb * pVerbs)
{
	UpdateLock updateLock(this);

	CPropGroupPtr pPropGroup = new CPropGroup( this, name, m_propGroups.size()+1, props, pVerbs );
	m_propGroups.push_back(pPropGroup);
	return pPropGroup;
}

CAnimationGraphStateEditor2::CPropGroupPtr CAnimationGraphStateEditor2::AddPropertyGroup( const CString& name, const std::vector<CString>& props, const Verb * pVerbs)
{
	UpdateLock updateLock(this);

	CPropGroupPtr pPropGroup = new CPropGroup( this, name, m_propGroups.size()+1, props, pVerbs );
	m_propGroups.push_back(pPropGroup);
	return pPropGroup;
}

void CAnimationGraphStateEditor2::OnCommand( UINT id )
{
	UINT grp = id / 100;
	if (!grp || grp > m_propGroups.size())
		return;
	m_propGroups[grp-1]->OnCommand( id - grp*100 );
}

void CAnimationGraphStateEditor2::StateEvent( EStateEditorEvent2 event )
{
	if (m_pCurEdit)
		m_pCurEdit->OnEvent( event );
}

void CAnimationGraphStateEditor2::OnStateParamSelChanged( const TParameterizationId2* pParamsId, const CString& viewName )
{
	if ( m_pCurEdit )
	{
		m_pCurEdit->ActivateParameterization( pParamsId, viewName );
		
		UpdateLock updateLock(this);

		// rebuild panels
		if ( !pParamsId )
		{
			m_propGroups.resize(0);
			m_pCurEdit->OnEvent( eSEE_InitContainer2 );
		}
		else
		{
			m_pCurEdit->OnEvent( eSEE_RebuildCriteria2 );
			m_pCurEdit->OnEvent( eSEE_UpdateNodeDetailsPane );
			if ( m_propGroups.size() >= 1 && m_propGroups[0] )
				m_propGroups[0]->Disable();
// 			if ( m_propGroups.size() >= 2 && m_propGroups[1] )
// 				m_propGroups[1]->Disable();
		}
	}
}

/*
CXTPDockingPane * CAnimationGraphStateEditor2::GetDetailsDockingPane()
{
	assert(m_pParent); 
	return m_pParent->GetNodeDetailsPane();
}
*/
