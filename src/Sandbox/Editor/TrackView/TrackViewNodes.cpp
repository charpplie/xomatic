// TrackViewNodes.cpp : implementation file
//

#include "stdafx.h"
#include "TrackViewNodes.h"
#include "TrackViewKeyList.h"
#include "TrackViewUndo.h"
#include "..\StringDlg.h"
#include "..\NumberDlg.h"
#include "TVEventsDialog.h"
#include "TrackViewDialog.h"

#include "..\Clipboard.h"

#include "IMovieSystem.h"

#include "..\Objects\ObjectManager.h"
#include "..\Objects\Entity.h"
#include "..\ViewManager.h"

#include "ReportControl/XTPReportControl.h"


// The 'MI' represents a Menu Item.
#define MI_COPY_SELECTED_NODES 602
#define MI_REMOVE_SELECTED 10
#define MI_EXPAND_ALL 650
#define MI_COLLAPSE_ALL 659
#define MI_EXPAND_FOLDERS 660
#define MI_COLLAPSE_FOLDERS 661
#define MI_EXPAND_ENTITIES 651
#define MI_COLLAPSE_ENTITIES 652
#define MI_EXPAND_CAMERAS 653
#define MI_COLLAPSE_CAMERAS 654
#define MI_EXPAND_MATERIALS 655
#define MI_COLLAPSE_MATERIALS 656
#define MI_EXPAND_EVENTS 657
#define MI_COLLAPSE_EVENTS 658
#define MI_RENAME 11
#define MI_CREATE_FOLDER 610
#define MI_ADD_SELECTED_ENTITY 500
#define MI_ADD_DIRECTOR_NODE 501
#define MI_ADD_CONSOLE_VARIABLE 502
#define MI_ADD_SCRIPT_VARIABLE 503
#define MI_ADD_MATERIAL 504
#define MI_ADD_EVENT 505
#define MI_EDIT_EVENTS 550
#define MI_PASTE_NODES 604
#define MI_DETACH_FROM_GROUP 611
#define MI_COPY_SELECTED_KEYS 600
#define MI_PASTE_KEYS 601
#define MI_SAVE_TO_COLLADA 12
#define MI_LOAD_FROM_COLLADA 14
#define MI_SET_AS_VIEW_CAMERA 13
#define MI_ADD_TRACK_BASE 1000
#define MI_REMOVE_TRACK 299
#define MI_SHOW_HIDE_BASE 100

//////////////////////////////////////////////////////////////////////////
CTrackViewNodesCtrl::CAnimSequenceUndo::CAnimSequenceUndo( IAnimSequence *pSeq,const char *sName )
: CUndo( sName )
{
	CUndo::Record( new CUndoAnimSequenceObject(pSeq) );
}

BEGIN_MESSAGE_MAP(CTrackViewNodesCtrl, CTreeCtrlReport)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnNMRclick)
	ON_WM_SIZE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CTrackViewNodesCtrl::CTrackViewNodesCtrl()
{
	m_sequence = 0;
	m_keysCtrl = 0;

	CMFCUtils::LoadTrueColorImageList( m_imageList,IDB_TRACKVIEW_NODES,16,RGB(255,0,255) );
	SetImageList( &m_imageList );

	CXTPReportColumn *pNodeCol   
		= AddColumn(new CXTPReportColumn(0, _T("Node"), 150, TRUE, XTP_REPORT_NOICON, TRUE, TRUE));
	pNodeCol->SetTreeColumn(true);
	pNodeCol->SetSortable(FALSE);

	ShowHeader(FALSE);

	SetMultipleSelection(TRUE);

	SetGroupRowsBold(TRUE);
	//SetTreeIndent(30);

	GetReportHeader()->AllowColumnRemove(FALSE);
	GetReportHeader()->AllowColumnSort(FALSE);

	AllowEdit(FALSE);
};

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::SetSequence( IAnimSequence *seq )
{
	m_pTrackViewDialog = (CTrackViewDialog*)GetOwner();

	m_itemInfos.clear();
	DeleteAllItems();
	m_sequence = seq;

	Reload();
	m_keysCtrl->Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::Reload()
{
	__super::Reload();
	SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::SetKeyListCtrl( CTrackViewKeys *keysCtrl )
{
	m_keysCtrl = keysCtrl;
	//SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::FillTracks( CRecord *pNodeRecord,IAnimNode *node )
{
	IAnimNode::SParamInfo paramInfo;
	for (int i = 0; i < node->GetTrackCount(); i++)
	{
		IAnimTrack *track = node->GetTrackByIndex(i);
		if (track->GetFlags() & IAnimTrack::ATRACK_HIDDEN) 
			continue;

		int type = track->GetParameterType();

		if (!node->GetParamInfoFromId(type,paramInfo))
			continue;

		int nImage = GetIconIndexForParam(type);
		CRecord *pTrackRecord = new CRecord( false,paramInfo.name );
		pTrackRecord->SetItemHeight( 18 );

		CXTPReportRecordItem *pTrackItem = new CXTPReportRecordItemText(paramInfo.name);
		pTrackRecord->AddItem(pTrackItem);
		pTrackItem->SetIconIndex(nImage);
		pTrackItem->SetTextColor(::GetSysColor(COLOR_HIGHLIGHT));

		pTrackRecord->node = node;
		pTrackRecord->baseObj = pNodeRecord->baseObj;
		pTrackRecord->track = track;
		pTrackRecord->paramId = type;

		m_itemInfos.push_back(pTrackRecord);

		pNodeRecord->GetChilds()->Add( pTrackRecord );

		if (track->GetSubTrackCount() > 0)
		{
			// This track s compound
			for (int nSubtrack = 0; nSubtrack < track->GetSubTrackCount(); nSubtrack++)
			{
				IAnimTrack *pSubTrack = track->GetSubTrack(nSubtrack);

				const char *name = track->GetSubTrackName(nSubtrack);

				int nSubImage = GetIconIndexForParam(type);
				CRecord *pSubTrackRecord = new CRecord( false,name );
				pSubTrackRecord->SetItemHeight( 18 );

				CXTPReportRecordItem *pSubTrackItem = new CXTPReportRecordItemText(name);
				pSubTrackRecord->AddItem(pSubTrackItem);
				pSubTrackItem->SetIconIndex(nSubImage);
				pSubTrackItem->SetTextColor(::GetSysColor(COLOR_HIGHLIGHT));

				pSubTrackRecord->node = node;
				pSubTrackRecord->baseObj = pNodeRecord->baseObj;
				pSubTrackRecord->track = pSubTrack;
				pSubTrackRecord->paramId = pSubTrack->GetParameterType();

				m_itemInfos.push_back(pSubTrackRecord);

				pTrackRecord->GetChilds()->Add( pSubTrackRecord );	
			}
			if (track->GetFlags() & IAnimTrack::ATRACK_EXPANDED)
				pTrackRecord->SetExpanded(TRUE);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewNodesCtrl::GetIconIndexForParam( int type )
{
	int nImage = 13; // Default
	if (type == APARAM_FOV)
		nImage = 2;
	if (type == APARAM_POS)
		nImage = 3;
	if (type == APARAM_ROT)
		nImage = 4;
	if (type == APARAM_SCL)
		nImage = 5;
	if (type == APARAM_EVENT || type == APARAM_TRACKEVENT)
		nImage = 6;
	if (type == APARAM_VISIBLE)
		nImage = 7;
	if (type == APARAM_CAMERA)
		nImage = 8;
	if ((type >= APARAM_SOUND1) && (type <= APARAM_SOUND3))
		nImage = 9;
	if ((type >= APARAM_CHARACTER1) && (type <= APARAM_CHARACTER3))
		nImage = 10;
	if (type == APARAM_SEQUENCE)
		nImage = 11;
	if ((type >= APARAM_EXPRESSION1) && (type <= APARAM_EXPRESSION3))
		nImage = 12;
	if (type == APARAM_FLOAT_1)
		nImage = 13;
	if (type == APARAM_CAPTURE)
		nImage = 11;

	return nImage;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::FillNodes(CRecord *pRecord, IAnimNode *pParent)
{
	IObjectManager *pObjMgr = GetIEditor()->GetObjectManager();
	IAnimSequence *seq = m_sequence;

	for (int i = 0; i < seq->GetNodeCount(); i++)
	{
		IAnimNode *node = seq->GetNode(i);
		if (node->GetParent() != pParent)
			continue;

		if (node->GetType() == ANODE_GROUP)
		{
			CRecord *pGroupRecord = new CRecord(true,node->GetName());
			CXTPReportRecordItem *pItem = new CXTPReportRecordItemText(node->GetName());
			pItem->SetBold(TRUE);
			pItem->SetBackgroundColor(RGB(220,255,220));
			pGroupRecord->AddItem(pItem);
			if (node->GetFlags()&ANODE_FLAG_EXPANDED)
			{
				pGroupRecord->SetExpanded(TRUE);
			}
			pGroupRecord->SetItemHeight(20);
			pGroupRecord->node = node;

			pRecord->GetChilds()->Add( pGroupRecord );

			FillNodes(pGroupRecord, node);
		}
		else
		{
			int nNodeImage = 1;
			EAnimNodeType nodeType = node->GetType();
			switch (nodeType)
			{
			case ANODE_ENTITY:	   nNodeImage = 1; break;
			case ANODE_CAMERA:	   nNodeImage = 8; break;
			case ANODE_SCRIPTVAR:	 nNodeImage = 14; break;
			case ANODE_CVAR:       nNodeImage = 15; break;
			case ANODE_MATERIAL:   nNodeImage = 16; break;
			case ANODE_EVENT:       nNodeImage = 17; break;
			}

			CRecord *pNodeRecord = new CRecord( false,node->GetName() );
			CXTPReportRecordItem *pNodeItem = new CXTPReportRecordItemText(node->GetName());
			pNodeRecord->AddItem(pNodeItem);
			pNodeItem->SetIconIndex(nNodeImage);
			pNodeItem->SetBold(TRUE);
			pNodeItem->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));

			pNodeRecord->node = node;
			pNodeRecord->baseObj = pObjMgr->FindAnimNodeOwner(node);
			pNodeRecord->track = 0;
			pNodeRecord->paramId = 0;

			m_itemInfos.push_back(pNodeRecord);

			if (node->GetFlags()&ANODE_FLAG_EXPANDED)
			{
				pNodeRecord->SetExpanded(TRUE);
			}

			pRecord->GetChilds()->Add( pNodeRecord );

			FillTracks(pNodeRecord, node);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnFillItems()
{
	m_itemInfos.clear();

	IAnimSequence *seq = m_sequence;
	if (!seq)
		return;

	CRecord *pRootGroupRec = new CRecord(true,seq->GetName());
	CXTPReportRecordItem *pRootItem = new CXTPReportRecordItemText(seq->GetName());
	pRootItem->SetBold(TRUE);
	pRootGroupRec->SetItemHeight(24);
	pRootGroupRec->AddItem(pRootItem);
	pRootGroupRec->SetExpanded(TRUE);

	AddRecord( pRootGroupRec );

	FillNodes(pRootGroupRec);

	// Additional empty record like space for scrollbar in key control
	CRecord *pGroupRec = new CRecord();
	pGroupRec->SetItemHeight(18);
	AddRecord( pGroupRec );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnItemExpanded( CXTPReportRow *pRow,bool bExpanded )
{
	CRecord *pRecord = (CRecord*)pRow->GetRecord();
	if (pRecord && pRecord->track)
	{
		IAnimTrack *track = pRecord->track;
		if (bExpanded)
		{
			// Mark track expanded
			track->SetFlags( track->GetFlags()|IAnimTrack::ATRACK_EXPANDED );
		}
		else
		{
			// Mark track collapsed.
			track->SetFlags( track->GetFlags()&(~IAnimTrack::ATRACK_EXPANDED) );
		}
	}
	else if (pRecord && pRecord->node)
	{
		IAnimNode *node = pRecord->node;
		if (bExpanded)
		{
			// Mark node expanded
			node->SetFlags( node->GetFlags()|ANODE_FLAG_EXPANDED );
		}
		else
		{
			// Mark node collapsed.
			node->SetFlags( node->GetFlags()&(~ANODE_FLAG_EXPANDED) );
		}
	}

	SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnSelectionChanged()
{
	m_keysCtrl->ClearSelection();
	
	if (!m_sequence)
		return;

	// Clear track selections.
	for (int i = 0; i < m_sequence->GetNodeCount(); i++)
	{
		IAnimNode *node = m_sequence->GetNode(i);
		for (int t = 0; t < node->GetTrackCount(); t++)
		{
			node->GetTrackByIndex(t)->SetSelected(false);
		}
	}


	int nCount = GetSelectedRows()->GetCount();
	for (int i = 0; i < nCount; i++)
	{
		CRecord *pRecord = (CRecord*)GetSelectedRows()->GetAt(i)->GetRecord();
		if (!pRecord->track)
			continue;

		for (int i = 0; i < m_keysCtrl->GetCount(); i++)
		{
			if (pRecord->track)
				pRecord->track->SetSelected(true);
			if (m_keysCtrl->GetTrack(i) == pRecord->track)
			{
				m_keysCtrl->SelectItem(i);
				break;
			}
		}
	}

	GetIEditor()->Notify( eNotify_OnUpdateTVKeySelection );
}



//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::SyncKeyCtrl()
{

CXTPReportRows m_arrScreenRows; //Diesel new

	if (!m_keysCtrl)
		return;

	m_keysCtrl->ResetContent();

	if (!m_sequence)
		return;

	// Forces ctrl to be drawn first.
	UpdateWindow();

	int nStartRow = 0;

	CXTPReportRows *pRows = GetRows();
	int nNumRows = m_arrScreenRows.GetCount();
	for (int i = 0; i < nNumRows; i++)
	{
		CXTPReportRow *pRow = m_arrScreenRows.GetAt(i);
		if (!pRow->IsItemsVisible())
			break;

		CRecord *pRecord = (CRecord*)pRow->GetRecord();

		CRect rc = pRecord->GetRect();
		rc = pRow->GetRect();

		int nItemHeight = rc.bottom-rc.top;

		CTrackViewKeyList::Item item;

		if (pRecord != NULL && pRecord->track != NULL && pRecord->node != NULL)
		{
			item = CTrackViewKeyList::Item(nItemHeight, pRecord->node,pRecord->paramId,pRecord->track);
		}
		else
		{
			item = CTrackViewKeyList::Item(nItemHeight,pRecord->node);
		}
		item.nHeight = nItemHeight;
		item.bSelected = false;
		if (pRecord->track)
		{
			item.bSelected = (pRecord->track->GetFlags() & IAnimTrack::ATRACK_SELECTED) != 0;
		}
		m_keysCtrl->AddItem( item );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::ExpandNode( IAnimNode *node )
{
	CXTPReportRows *pRows = GetRows();
	for (int i = 0,nNumRows = pRows->GetCount(); i < nNumRows; i++)
	{
		CXTPReportRow *pRow = pRows->GetAt(i);
		CRecord *pRecord = (CRecord*)pRow->GetRecord();
		if (pRecord && pRecord->node == node)
		{
			pRow->SetExpanded(TRUE);
			break;
		}
	}
	SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::DeleteSelectedNodes()
{
	char *description = NULL;

	if (GetSelectedCount() > 1)
	{
		description = "Remove Anim Nodes/Tracks";
	}
	else
	{
		description = "Remove Anim Node";
	}

	CUndo undo(description);
	CUndo::Record( new CUndoAnimSequenceObject(m_sequence) );

	bool nodeDeleted = false;
	bool trackDeleted = false;

	Records records;
	GetSelectedRecords( records );

	// First remove all selected tracks
	for (int currentNode = 0; currentNode < records.size(); currentNode++)
	{
		CRecord *pItemInfo = (CRecord*)records[currentNode];
		if (pItemInfo != NULL && pItemInfo->node != NULL && pItemInfo->track != NULL)
		{
			pItemInfo->node->RemoveTrack(pItemInfo->track);
			trackDeleted = true;
		}
	}

	// Then remove all selected nodes
	for (int currentNode = 0; currentNode < records.size(); currentNode++)
	{
		CRecord *pItemInfo = (CRecord*)records[currentNode];
		if (pItemInfo != NULL && pItemInfo->node != NULL && pItemInfo->track == NULL)
		{
			// Unbind the entity from the AnimNode before removing it.
			CEntity *pEntity = (CEntity*)GetIEditor()->GetObjectManager()->FindAnimNodeOwner(pItemInfo->node);
			if (pEntity)
			{
				pItemInfo->node->SetNodeOwner(0);
				pEntity->SetAnimNode(0);
			}

			m_sequence->RemoveNode(pItemInfo->node);
			nodeDeleted = true;
		}
	}

	if (nodeDeleted == true || trackDeleted == true)
	{
		InvalidateNodes();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::SelectNode( const char *sName )
{
	CXTPReportRows *pRows = GetRows();
	for (int i = 0,nNumRows = pRows->GetCount(); i < nNumRows; i++)
	{
		CXTPReportRow *pRow = pRows->GetAt(i);
		CRecord *pRecord = (CRecord*)pRow->GetRecord();
		if (pRecord && pRecord->node != NULL && stricmp(pRecord->node->GetName(),sName) == 0)
		{
			pRow->SetSelected(TRUE);
			break;
		}
	}
	SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewNodesCtrl::CopySelectedNodes()
{
	XmlNodeRef animNodesRoot = CreateXmlNode("CopyAnimNodesRoot");

	AnimNodes records;
	GetSelectedNodes( records );

	if(records.empty() == false)
		m_sequence->CopyNodes(animNodesRoot, &(records[0]), (uint32)records.size());

	CClipboard clipboard;
	clipboard.Put(animNodesRoot,"Track view entity node");

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewNodesCtrl::PasteNodes()
{
	CClipboard clipboard;
	if (clipboard.IsEmpty())
	{
		return false;
	}

	XmlNodeRef animNodesRoot = clipboard.Get();
	if (animNodesRoot == NULL || strcmp(animNodesRoot->getTag(),"CopyAnimNodesRoot"))
	{
		return false;
	}

	bool bSaveUndo = !CUndo::IsRecording();

	if (bSaveUndo)
	{
		GetIEditor()->BeginUndo();
	}

	CUndo::Record(new CUndoAnimSequenceObject(m_sequence));

	m_sequence->PasteNodes(animNodesRoot);

	InvalidateNodes();

	if (bSaveUndo)
	{
		GetIEditor()->AcceptUndo("Paste Anim Node(s)");
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnVerticalScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint point;

	SItemInfo *pItemInfo = 0;

	if (!m_sequence)
		return;

	// Find node under mouse.
	GetCursorPos( &point );
	ScreenToClient( &point );
	// Select the item that is at the point myPoint.

	CXTPReportRow* pRow = HitTest(point);
	if (pRow)
	{
		pItemInfo = (SItemInfo*)pRow->GetRecord();
	}

	IAnimNode *pGroupNode = 0;
	if (pItemInfo && pItemInfo->node && pItemInfo->node->GetType() == ANODE_GROUP)
	{
		pGroupNode = pItemInfo->node;
	}

	int cmd = ShowPopupMenu(point, pItemInfo);

	if (cmd == MI_SAVE_TO_COLLADA)
	{
		SaveNodeToCollada();
	}
	else if (cmd == MI_LOAD_FROM_COLLADA)
	{
		LoadNodeFromCollada();
	}
	else if (cmd == MI_SET_AS_VIEW_CAMERA)
	{
		SetAsViewCamera();
	}
	else if (cmd == MI_REMOVE_SELECTED)
	{
		DeleteSelectedNodes();
	}
	else if (cmd == MI_ADD_SELECTED_ENTITY)
	{
		m_pTrackViewDialog->AddSelectedNodes(pGroupNode);
	}
	else if (cmd == MI_ADD_DIRECTOR_NODE)
	{
		m_pTrackViewDialog->CreateAnimNode( ANODE_SCENE,"Director",pGroupNode );
	}
	else if (cmd == MI_ADD_CONSOLE_VARIABLE)
	{
		CStringDlg dlg( _T("Console Variable Name") );
		if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
		{
			m_pTrackViewDialog->CreateAnimNode( ANODE_CVAR,dlg.GetString(),pGroupNode );
		}
	}
	else if (cmd == MI_ADD_SCRIPT_VARIABLE)
	{
		CStringDlg dlg( _T("Script Variable Name") );
		if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
		{
			m_pTrackViewDialog->CreateAnimNode( ANODE_SCRIPTVAR,dlg.GetString(),pGroupNode );
		}
	}
	else if (cmd == MI_ADD_MATERIAL)
	{
		CStringDlg dlg( _T("Material Name") );
		if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
		{
			m_pTrackViewDialog->CreateAnimNode( ANODE_MATERIAL,dlg.GetString(),pGroupNode );
		}
	}
	else if (cmd == MI_ADD_EVENT)
	{
		CStringDlg dlg( _T("Track Event Name") );
		if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
		{
			m_pTrackViewDialog->CreateAnimNode( ANODE_EVENT,dlg.GetString(),pGroupNode );
		}
	}
	else if (cmd == MI_EDIT_EVENTS)
	{
		EditEvents();
	}
	else if (cmd == MI_RENAME && pItemInfo != NULL && pItemInfo->node != NULL)
	{
		RenameNode(pItemInfo);

	}
	else if (cmd == MI_CREATE_FOLDER)
	{
		CreateFolder(pGroupNode);
	}
	else if (cmd == MI_DETACH_FROM_GROUP)
	{
		// Detach from group
		if (pItemInfo && pItemInfo->node)
		{
			pItemInfo->node->SetParent(0);
			InvalidateNodes();
			SelectNode(pItemInfo->node->GetName());
		}
	}
	else if (cmd >= MI_ADD_TRACK_BASE && cmd < MI_ADD_TRACK_BASE+1000)
	{
		if (pItemInfo)
		{
			IAnimNode *node = pItemInfo->node;
			if (node)
			{
				AddTrack(cmd-MI_ADD_TRACK_BASE, node);
			}
		}
	}
	else if (cmd == MI_REMOVE_TRACK)
	{
		if (pItemInfo)
		{
			IAnimNode *node = pItemInfo->node;
			if (node)
			{
				RemoveTrack( pItemInfo);
			}
		}
	}
	else if (cmd >= MI_SHOW_HIDE_BASE && cmd < MI_SHOW_HIDE_BASE+100)
	{
		if (pItemInfo)
		{
			IAnimNode *node = pItemInfo->node;
			if (node)
			{
				ShowHideTrack(node, cmd-MI_SHOW_HIDE_BASE);
			}
		}
	}
	else if(cmd==MI_COPY_SELECTED_KEYS)
	{
		m_keysCtrl->CopyKeys();
	}
	else if(cmd==MI_PASTE_KEYS)
	{
		SItemInfo * pSii = GetSelectedNode();
		m_keysCtrl->PasteKeys(pSii?pSii->node:0,0);
	}
	else if(cmd==MI_COPY_SELECTED_NODES)
	{
		CopySelectedNodes();
	}
	else if(cmd==MI_PASTE_NODES)
	{
		PasteNodes();
	}
	else if(cmd==MI_EXPAND_ALL)
		ExpandNodes();
	else if(cmd==MI_COLLAPSE_ALL)
		CollapseNodes();
	else if(cmd==MI_EXPAND_FOLDERS)
		ExpandNodes(ANODE_GROUP);
	else if(cmd==MI_COLLAPSE_FOLDERS)
		CollapseNodes(ANODE_GROUP);
	else if(cmd==MI_EXPAND_ENTITIES)
		ExpandNodes(ANODE_ENTITY);
	else if(cmd==MI_COLLAPSE_ENTITIES)
		CollapseNodes(ANODE_ENTITY);
	else if(cmd==MI_EXPAND_CAMERAS)
		ExpandNodes(ANODE_CAMERA);
	else if(cmd==MI_COLLAPSE_CAMERAS)
		CollapseNodes(ANODE_CAMERA);
	else if(cmd==MI_EXPAND_MATERIALS)
		ExpandNodes(ANODE_MATERIAL);
	else if(cmd==MI_COLLAPSE_MATERIALS)
		CollapseNodes(ANODE_MATERIAL);
	else if(cmd==MI_EXPAND_EVENTS)
		ExpandNodes(ANODE_EVENT);
	else if(cmd==MI_COLLAPSE_EVENTS)
		CollapseNodes(ANODE_EVENT);

	// processed
	*pResult = 1;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewNodesCtrl::CRecord* CTrackViewNodesCtrl::GetSelectedNode()
{
	if (GetSelectedCount() == 1)
	{
		Records records;
		GetSelectedRecords( records );
		return (CRecord*)records[0];
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewNodesCtrl::OnBeginDragAndDrop( CXTPReportRow *pRow, CPoint pt )
{
	CRecord* pRec = static_cast<CRecord*> (pRow->GetRecord());
	if (!pRec || !pRec->node)
		return false;
	if (pRec->node && pRec->track && !pRec->IsGroup())
		return false;
	return __super::OnBeginDragAndDrop(pRow,pt);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnDragAndDrop( CXTPReportRow *pRow, CPoint absoluteCursorPos )
{
	CWnd *wnd = CWnd::WindowFromPoint( absoluteCursorPos );
	//if (wnd != this)
		//return;

	CPoint p = absoluteCursorPos;
	ScreenToClient(&p);

	CRect reportArea = GetReportRectangle();
	if (reportArea.PtInRect(p))
	{
		CXTPReportRow* pTargetRow = HitTest( p );
		if (pTargetRow && pTargetRow != pRow) 
		{
			CRecord *pRecordSrc = (CRecord*)pRow->GetRecord();
			CRecord *pRecordTrg = (CRecord*)pTargetRow->GetRecord();
			if (pRecordSrc && pRecordTrg && pRecordSrc->node && pRecordTrg->node && (pRecordSrc->node != pRecordTrg->node))
			{
				IAnimNode *pTargetNode = pRecordTrg->node;
				if (pTargetNode->GetType() == ANODE_GROUP)
				{
					CAnimSequenceUndo undo(m_sequence,"Attach Anim Node to Group");
					AnimNodes nodes;
					if (GetSelectedNodes( nodes ))
					{
						for (int i = 0; i < (int)nodes.size(); i++)
						{
							if (nodes[i] != pTargetNode)
								nodes[i]->SetParent(pTargetNode);
						}
					}
					InvalidateNodes();
				}
				else
				{
					CAnimSequenceUndo undo(m_sequence,"Reorder Node");

					IAnimNode *pTargetNode = pRecordTrg->node;
					// Drop to another node.
					if (pTargetNode->GetParent())
					{
						AnimNodes nodes;
						if (GetSelectedNodes( nodes ))
						{
							for (int i = 0; i < (int)nodes.size(); i++)
							{
								if (nodes[i] != pTargetNode)
									nodes[i]->SetParent( pTargetNode->GetParent() );
							}
						}

						// Attach to same group as target
						pRecordSrc->node->SetParent( pTargetNode->GetParent() );
					}
					m_sequence->ReorderNode( pRecordSrc->node,pTargetNode );
					SelectNode( pTargetNode->GetName() );
					InvalidateNodes();
				}
			}
			else
			{
				if (pRecordTrg && pRecordTrg == GetRows()->GetAt(0)->GetRecord() && pRecordSrc && pRecordSrc->node)
				{
					// Droping at root
					CAnimSequenceUndo undo(m_sequence,"Detach Anim Node from Group");
					pRecordSrc->node->SetParent(0);
					InvalidateNodes();
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnItemDblClick( CXTPReportRow *pRow )
{
	CRecord *pRecord = (CRecord*)pRow->GetRecord();
	if (pRecord && pRecord->node != NULL)
	{
		// Double Clicked on node or track.
		// Make this object selected in Editor.
		CBaseObject *obj = GetIEditor()->GetObjectManager()->FindAnimNodeOwner(pRecord->node);
		if (obj)
		{
			CUndo undo( "Select Object(s)" );
			GetIEditor()->ClearSelection();
			GetIEditor()->SelectObject(obj);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::InvalidateNodes()
{
	GetIEditor()->Notify( eNotify_OnUpdateTrackView );
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewNodesCtrl::GetSelectedNodes( AnimNodes &nodes )
{
	int nCount = GetSelectedRows()->GetCount();
	for (int i = 0; i < nCount; i++)
	{
		CRecord *pRecord = (CRecord*)GetSelectedRows()->GetAt(i)->GetRecord();
		if (pRecord && pRecord->node)
			nodes.push_back(pRecord->node);
	}
	return !nodes.empty();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewNodesCtrl::HasDirectorNode() const
{
	IAnimNode *pNode = NULL;
	// Check the new name.
	pNode = m_sequence->FindNodeByName("Director");
	if (pNode && pNode->GetType() == ANODE_SCENE)
		return true;
	// Check the deprecated name.
	pNode = m_sequence->FindNodeByName("Scene");
	if (pNode && pNode->GetType() == ANODE_SCENE)
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::ExpandNodes(EAnimNodeType nodeType)
{
	for (int i = 0,num = m_sequence->GetNodeCount(); i < num; i++)
	{
		IAnimNode *node = m_sequence->GetNode(i);
		if (nodeType == ANODE_ANYTRACK || node->GetType() == nodeType)
			node->SetFlags( node->GetFlags() | ANODE_FLAG_EXPANDED );
	}
	InvalidateNodes();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::CollapseNodes(EAnimNodeType nodeType)
{
	for (int i = 0,num = m_sequence->GetNodeCount(); i < num; i++)
	{
		IAnimNode *node = m_sequence->GetNode(i);
		if (nodeType == ANODE_ANYTRACK || node->GetType() == nodeType)
		 node->SetFlags( node->GetFlags() & ~ANODE_FLAG_EXPANDED );
	}
	InvalidateNodes();
}

void CTrackViewNodesCtrl::SaveNodeToCollada()
{
	SItemInfo * pSii = GetSelectedNode();
	if (pSii && pSii->node)
	{		
		char szFilters[] = "Collada Files (*.dae)|*.dae|";

		CString path = Path::GetGameFolder() + CString("\\");
		CString file = CString(pSii->node->GetName()) + CString(".dae");

		CString finalPath = file;
		uint32 uSave = CFileUtil::SelectSaveFile( szFilters,"dae", path, finalPath );
		if (uSave)
		{
			float fps = 30.0f;
			CNumberDlg cDialog( m_pTrackViewDialog,fps,"Set a baking FPS" );
			cDialog.SetRange(1.0f, 120.0f);

			if (cDialog.DoModal() == IDOK)
			{
				fps = cDialog.GetValue();
				XmlNodeRef colladaNode = pSii->node->SaveToColladaInFixedFPS(fps);
				colladaNode->saveToFile(finalPath.GetString());
			}
		}
	}
}

void CTrackViewNodesCtrl::LoadNodeFromCollada()
{
	SItemInfo * pSii = GetSelectedNode();
	if (pSii && pSii->node)
	{		
		char szFilters[] = "Collada Files (*.dae)|*.dae|";

		CString path = Path::GetGameFolder() + CString("\\");

		CString filePath;
		if (CFileUtil::SelectFile(szFilters, path, filePath))
		{
			XmlNodeRef colladaNode = GetISystem()->LoadXmlFile(filePath.GetString());
			if(!colladaNode)
				AfxMessageBox("Cannot load the specified COLLADA file.", MB_ICONEXCLAMATION | MB_OK);
			else
			{
				bool ok = pSii->node->LoadFromCollada(colladaNode);
				if (ok == false)
					AfxMessageBox("Cannot load the specified COLLADA file.", MB_ICONEXCLAMATION | MB_OK);
			}
		}
	}
}

void CTrackViewNodesCtrl::SetAsViewCamera()
{
	SItemInfo * pSii = GetSelectedNode();
	if (pSii && pSii->node && pSii->node->GetType() == ANODE_CAMERA)
	{
		CBaseObject *pCameraObject = GetIEditor()->GetObjectManager()->FindAnimNodeOwner(pSii->node);
		if (pCameraObject)
			static_cast<CRenderViewport*>(GetIEditor()->GetViewManager()->GetGameViewport())->SetCameraObject(pCameraObject);
	}
}

void CTrackViewNodesCtrl::EditEvents()
{
	CTVEventsDialog dlg(m_sequence);
	if (dlg.DoModal() == IDOK)
	{
		// Copy over track events
		m_sequence->ClearTrackEvents();
		TrackEvents::iterator itEvent = dlg.m_events.begin();
		TrackEvents::iterator itEventEnd = dlg.m_events.end();
		for (; itEvent != itEventEnd; ++itEvent)
		{
			m_sequence->AddTrackEvent(itEvent->c_str());
		}
	}
}

void CTrackViewNodesCtrl::RenameNode( SItemInfo * pItemInfo )
{
	// Rename node under cursor.
	CStringDlg dlg( _T("Rename Node") );
	dlg.SetString(pItemInfo->node->GetName());
	if (dlg.DoModal() == IDOK)
	{
		pItemInfo->node->SetName( dlg.GetString() );
		InvalidateNodes();
	}
}

void CTrackViewNodesCtrl::CreateFolder( IAnimNode * pGroupNode )
{
	// Change Group of the node.
	CStringDlg dlg( _T("Enter Folder Name") );
	//dlg.SetString(pItemInfo->node->GetGroupName());
	if (dlg.DoModal() == IDOK)
	{
		if(m_sequence->FindNodeByName(dlg.GetString()))
			AfxMessageBox("The name already exists. Use another.", MB_ICONEXCLAMATION | MB_OK);
		else
		{
			CAnimSequenceUndo undo(m_sequence,"Create Group");
			m_pTrackViewDialog->CreateAnimNode( ANODE_GROUP,dlg.GetString(),pGroupNode );
			InvalidateNodes();
		}
	}
}

void CTrackViewNodesCtrl::AddTrack( int paramId, IAnimNode * node )
{
	CUndo undo("Create Anim Track");
	CUndo::Record( new CUndoAnimSequenceObject(m_sequence) );

	node->CreateTrack( paramId );
	InvalidateNodes();
	ExpandNode(node);
}

void CTrackViewNodesCtrl::RemoveTrack( SItemInfo * pItemInfo )
{
	if (AfxMessageBox("Are you sure you want to delete this track ? Undo will not be available !", 
										MB_ICONQUESTION | MB_YESNO)==IDYES)
	{
		CUndo undo("Remove Anim Track");
		CUndo::Record( new CUndoAnimSequenceObject(m_sequence) );

		pItemInfo->node->RemoveTrack(pItemInfo->track);
		InvalidateNodes();
	}
}

void CTrackViewNodesCtrl::ShowHideTrack( IAnimNode * node, int trackIndex )
{
	CUndo undo("Modify Sequence");
	CUndo::Record( new CUndoAnimSequenceObject(m_sequence) );

	IAnimTrack *track = node->GetTrackByIndex(trackIndex);

	// change hidden flag for this track.
	if (track->GetFlags() & IAnimTrack::ATRACK_HIDDEN)
		track->SetFlags( track->GetFlags() & ~IAnimTrack::ATRACK_HIDDEN );
	else
		track->SetFlags( track->GetFlags() | IAnimTrack::ATRACK_HIDDEN );

	InvalidateNodes();
}

int CTrackViewNodesCtrl::ShowPopupMenu( CPoint point, const SItemInfo *pItemInfo )
{
	// Create pop up menu.
	CMenu menu;
	CMenu menuAddTrack;

	CMenu menuExpand;
	CMenu menuCollapse;

	menu.CreatePopupMenu();

	IAnimNode *pGroupNode = 0;
	if (pItemInfo && pItemInfo->node && pItemInfo->node->GetType() == ANODE_GROUP)
	{
		pGroupNode = pItemInfo->node;
	}

	if (GetSelectedCount() > 1)
	{
		Records records;
		GetSelectedRecords( records );
		bool nodeSelected = false;
		for (int currentNode = 0; currentNode < GetSelectedCount(); currentNode++)
		{
			SItemInfo *pItemInfo = (SItemInfo*)records[currentNode];
			if (pItemInfo != NULL && pItemInfo->node != NULL && pItemInfo->track == NULL)
			{
				nodeSelected = true;
				break;
			}
		}

		if (nodeSelected)
		{
			menu.AppendMenu( MF_STRING,MI_COPY_SELECTED_NODES,"Copy Selected Nodes" );
		}

		menu.AppendMenu( MF_STRING,MI_REMOVE_SELECTED,"Remove Selected Nodes/Tracks" );
	}
	else
	{
		bool notOnValidItem = !pItemInfo || !pItemInfo->node;
		bool onValidItem = !notOnValidItem;
		bool onGroupNode = pGroupNode != NULL;
		bool onNode = onValidItem && pItemInfo->track == NULL;
		bool onTrack = onValidItem && pItemInfo->track != NULL;
		if (notOnValidItem || onGroupNode)
		{
			menuExpand.CreatePopupMenu();
			menuCollapse.CreatePopupMenu();

			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_ALL,_T("Expand all") );
			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_ALL,_T("Collapse all") );
			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_FOLDERS,_T("Expand Folders") );
			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_FOLDERS,_T("Collapse Folders") );
			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_ENTITIES,_T("Expand Entities") );
			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_ENTITIES,_T("Collapse Entities") );
			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_CAMERAS,_T("Expand Cameras") );
			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_CAMERAS,_T("Collapse Cameras") );
			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_MATERIALS,_T("Expand Materials") );
			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_MATERIALS,_T("Collapse Materials") );
			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_EVENTS,_T("Expand Events") );
			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_EVENTS,_T("Collapse Events") );

			menu.AppendMenu(MF_POPUP,(UINT_PTR)menuExpand.m_hMenu,"Expand");
			menu.AppendMenu(MF_POPUP,(UINT_PTR)menuCollapse.m_hMenu,"Collapse");

			menu.AppendMenu( MF_SEPARATOR,0,"" );
			if (onGroupNode)
			{
				menu.AppendMenu( MF_STRING,MI_RENAME,"Rename" );
				menu.AppendMenu( MF_STRING,MI_REMOVE_SELECTED,"Delete" );
			}
			menu.AppendMenu( MF_STRING,MI_CREATE_FOLDER,"Create Folder" );
			menu.AppendMenu( MF_SEPARATOR,0,"" );
			menu.AppendMenu( MF_STRING,MI_ADD_SELECTED_ENTITY,_T("Add Selected Entity(s)") );
			menu.AppendMenu( MF_SEPARATOR,0,"" );
			if (HasDirectorNode() == false)	// There can be only one director node per sequence.
				menu.AppendMenu( MF_STRING,MI_ADD_DIRECTOR_NODE,_T("Add Director(Scene) Node") );
			menu.AppendMenu( MF_STRING,MI_ADD_CONSOLE_VARIABLE,_T("Add Console Variable") );
			menu.AppendMenu( MF_STRING,MI_ADD_SCRIPT_VARIABLE,_T("Add Script Variable") );
			menu.AppendMenu( MF_STRING,MI_ADD_MATERIAL,_T("Add Material") );
			menu.AppendMenu( MF_STRING,MI_ADD_EVENT,_T("Add Event") );
			menu.AppendMenu( MF_SEPARATOR,0,"" );
			menu.AppendMenu( MF_STRING,MI_EDIT_EVENTS,_T("Edit Events") );
			menu.AppendMenu( MF_SEPARATOR,0,"" );
			menu.AppendMenu( MF_STRING,MI_PASTE_NODES,_T("Paste Node(s)") );
		}
		else	// On a node other than group node or on a track
		{
			if (onNode)
			{
				menu.AppendMenu( MF_STRING,MI_CREATE_FOLDER,"Create Folder" );
				if (pItemInfo->node->GetParent())
					menu.AppendMenu( MF_STRING,MI_DETACH_FROM_GROUP,"Detach from Group" );

				menu.AppendMenu( MF_SEPARATOR,0,"" );
				menu.AppendMenu( MF_STRING,MI_COPY_SELECTED_NODES,"Copy" );
				menu.AppendMenu( MF_STRING,MI_COPY_SELECTED_KEYS,"Copy Selected Keys");
				menu.AppendMenu( MF_SEPARATOR,0,"" );
				menu.AppendMenu( MF_STRING,MI_REMOVE_SELECTED,"Delete" );
			}
			else	// On a track
			{
				menu.AppendMenu( MF_STRING,MI_COPY_SELECTED_KEYS,"Copy Keys");
			}
			if (onValidItem)
			{
				if (pItemInfo->node->GetFlags() & ANODE_FLAG_CAN_CHANGE_NAME)
				{
					menu.AppendMenu( MF_STRING,MI_RENAME,"Rename" );
				}
			}

			menu.AppendMenu( MF_STRING,MI_PASTE_KEYS,"Paste Keys");
			menu.AppendMenu( MF_SEPARATOR,0,"" );

			if (onNode)
			{
				menu.AppendMenu( MF_STRING,MI_SAVE_TO_COLLADA,_T("Save to a COLLADA file...") );
				menu.AppendMenu( MF_STRING,MI_LOAD_FROM_COLLADA,_T("Load from a COLLADA file...") );
				menu.AppendMenu( MF_SEPARATOR,0,"" );
				if (pItemInfo->node->GetType() == ANODE_CAMERA)
				{
					menu.AppendMenu( MF_STRING,MI_SET_AS_VIEW_CAMERA,_T("Set As View Camera") );
					menu.AppendMenu( MF_SEPARATOR,0,"" );
				}
			}
		}

		// add tracks menu
		menuAddTrack.CreatePopupMenu();
		bool bTracksToAdd=false;
		if (onValidItem)
		{
			IAnimNode::SParamInfo paramInfo;
			// List`s which tracks can be added to animation node.
			for (int i = 0; i < pItemInfo->node->GetParamCount(); i++)
			{
				if (!pItemInfo->node->GetParamInfo( i,paramInfo ))
					continue;

				int flags = 0;
				IAnimTrack *track = pItemInfo->node->GetTrackForParameter( paramInfo.paramId );
				if (track && !(paramInfo.flags&IAnimNode::PARAM_MULTIPLE_TRACKS))
				{
					continue;
					//flags |= MF_CHECKED;
				}

				menuAddTrack.AppendMenu( MF_STRING|flags,MI_ADD_TRACK_BASE+paramInfo.paramId,paramInfo.name );
				bTracksToAdd=true;
			}
		}
		if (bTracksToAdd)
			menu.AppendMenu(MF_POPUP,(UINT_PTR)menuAddTrack.m_hMenu,"Add Track");

		// delete track menu
		if (onTrack)
		{
			menu.AppendMenu(MF_STRING, MI_REMOVE_TRACK, "Remove Track");
		}

		if (bTracksToAdd || onTrack)
			menu.AppendMenu( MF_SEPARATOR,0,"" );

		if (onValidItem)
		{
			CString str;
			str.Format( "%s Tracks",pItemInfo->node->GetName() );
			menu.AppendMenu( MF_STRING|MF_DISABLED,0,str );

			// Show tracks in anim node.
			{
				IAnimNode::SParamInfo paramInfo;
				for (int i = 0; i < pItemInfo->node->GetTrackCount(); i++)
				{
					IAnimTrack *track = pItemInfo->node->GetTrackByIndex(i);

					if (!pItemInfo->node->GetParamInfoFromId( track->GetParameterType(),paramInfo ))
						continue;

					// change hidden flag for this track.
					int checked = MF_CHECKED;
					if (track->GetFlags() & IAnimTrack::ATRACK_HIDDEN)
					{
						checked = MF_UNCHECKED;
					}
					menu.AppendMenu( MF_STRING|checked,MI_SHOW_HIDE_BASE+i,CString( "  " ) + paramInfo.name );
				}
			}
		}
	}

	GetCursorPos( &point );
	// track menu
	return CXTPCommandBars::TrackPopupMenu(&menu, TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, 
																					point.x,point.y, this, NULL);
}


void CTrackViewNodesCtrl::OnSize(UINT nType, int cx, int cy) 
{
	__super::OnSize(nType, cx, cy);

	SyncKeyCtrl();
}