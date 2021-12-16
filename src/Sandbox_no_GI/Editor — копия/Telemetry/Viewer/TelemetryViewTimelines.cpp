////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   TelemetryViewTimelines.cpp
//  Version:     v1.00
//  Created:     13/05/11 by Steve Humphreys
//  Description: Tree-view of all telemetry timelines
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TelemetryViewTimelines.h"
#include "TelemetryEventsView.h"
#include "StringDlg.h"
#include "NumberDlg.h"
#include "TelemetryViewDialog.h"
#include "TimelineEventFilter.h"

#include "Clipboard.h"

#include "CustomColorDialog.h"

#include "Objects\EntityObject.h"
#include "ViewManager.h"
#include "RenderViewport.h"

// The 'MI' represents a Menu Item.
#define MI_TIMELINE_COLOUR 603
#define MI_TIMELINE_FILTER 604
#define MI_TIMELINE_CLEAR_FILTER 605
// #define MI_COPY_SELECTED_Timelines 602
// #define MI_REMOVE_SELECTED 10
// #define MI_EXPAND_ALL 650
// #define MI_COLLAPSE_ALL 659
// #define MI_EXPAND_FOLDERS 660
// #define MI_COLLAPSE_FOLDERS 661
// #define MI_EXPAND_ENTITIES 651
// #define MI_COLLAPSE_ENTITIES 652
// #define MI_EXPAND_CAMERAS 653
// #define MI_COLLAPSE_CAMERAS 654
// #define MI_EXPAND_MATERIALS 655
// #define MI_COLLAPSE_MATERIALS 656
// #define MI_EXPAND_EVENTS 657
// #define MI_COLLAPSE_EVENTS 658
// #define MI_RENAME 11
// #define MI_CREATE_FOLDER 610
// #define MI_ADD_SELECTED_ENTITY 500
// #define MI_ADD_DIRECTOR_NODE 501
// #define MI_ADD_CONSOLE_VARIABLE 502
// #define MI_ADD_SCRIPT_VARIABLE 503
// #define MI_ADD_MATERIAL 504
// #define MI_ADD_EVENT 505
// #define MI_ADD_CURRENT_LAYER 506
// #define MI_ADD_COMMENT_NODE 507
// #define MI_ADD_RADIALBLUR 508
// #define MI_ADD_COLLORCORRECTION 509
// #define MI_ADD_DOF 510
// #define MI_ADD_SCREENFADER_NODE 511
// #define MI_ADD_HDR_SETUP 512
// #define MI_ADD_SHADOW_SETUP 513
// #define MI_EDIT_EVENTS 550
// #define MI_PASTE_Timelines 604
// #define MI_DETACH_FROM_GROUP 611
// #define MI_COPY_SELECTED_KEYS 600
// #define MI_PASTE_KEYS 601
// #define MI_SAVE_TO_COLLADA 12
// #define MI_LOAD_FROM_COLLADA 14
// #define MI_SET_AS_VIEW_CAMERA 13
// #define MI_SET_AS_ACTIVE_DIRECTOR 15
// #define MI_TRANSFORM_BONE_LINK 16
// #define MI_ADD_TRACK_BASE 1000
// #define MI_REMOVE_TRACK 299
// #define MI_SHOW_HIDE_BASE 100


#define EDIT_DISABLE_GRAY_COLOR RGB(180,180,180)

//////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CTelemetryViewTimelinesCtrl, CTreeCtrlReport)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnNMRclick)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CTelemetryViewTimelinesCtrl::CTelemetryViewTimelinesCtrl()
{
	m_eventsCtrl = 0;

	CMFCUtils::LoadTrueColorImageList( m_imageList,IDB_TRACKVIEW_NODES,16,RGB(255,0,255) );
	SetImageList( &m_imageList );

	CXTPReportColumn *pNodeCol   
		= AddColumn(new CXTPReportColumn(0, _T("Timeline"), 150, TRUE, XTP_REPORT_NOICON, TRUE, TRUE));
	pNodeCol->SetTreeColumn(true);
	pNodeCol->SetSortable(FALSE);

	ShowHeader(FALSE);

	SetMultipleSelection(TRUE);

	SetGroupRowsBold(TRUE);
	//SetTreeIndent(30);

	GetReportHeader()->AllowColumnRemove(FALSE);
	GetReportHeader()->AllowColumnSort(FALSE);

	AllowEdit(FALSE);

	m_pTelemetryViewDialog = NULL;
};

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::Reload()
{
	__super::Reload();
	SyncEventsCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::SetEventsCtrl( CTelemetryEventsView *eventsCtrl )
{
	m_eventsCtrl = eventsCtrl;
	m_eventsCtrl->SetTimelinesCtrl(this);
	SyncEventsCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::FillTimelines( Telemetry::CTimelineRecord *pNodeRecord )
{
	if(!m_pTelemetryViewDialog)
		return;

	const Telemetry::CTelemetryRepository* pRepo = m_pTelemetryViewDialog->GetRepository();

	uint count = pRepo->GetTimelineCount();

	// first pass: collect owners
	std::vector<string> owners;
	for(uint i=0; i<count; ++i)
	{
		Telemetry::CTelemetryTimeline* pTimeLine = pRepo->GetTimeline(i);

		Telemetry::TTelemNodePtr pOwner = pTimeLine->GetOwner();

		Telemetry::STelemetryNode::TStates states = pOwner->GetStates();
		string name = states["name"];

		stl::push_back_unique(owners, name);
	}

	// create node for each owner, add subTimelines for tracks belonging to that owner
	for(std::vector<string>::const_iterator it = owners.begin(), end = owners.end(); it != end; ++it)
	{
		Telemetry::CTimelineRecord *pTrackRecord = new Telemetry::CTimelineRecord(it->c_str(), true );
		pTrackRecord->SetItemHeight( TELEMETRY_TIMELINE_ITEM_HEIGHT );
		pTrackRecord->SetExpanded(TRUE);
		m_itemInfos.push_back(pTrackRecord);

		CXTPReportRecordItem *pTrackItem = new CXTPReportRecordItemText(it->c_str());
		pTrackRecord->AddItem(pTrackItem);
		pTrackItem->SetTextColor(::GetSysColor(COLOR_HIGHLIGHT));
		pTrackItem->HasCheckbox(true);
		pTrackItem->SetChecked(true);
		pNodeRecord->GetChilds()->Add( pTrackRecord );

		for(uint i = 0; i < count; ++i)
		{
			Telemetry::CTelemetryTimeline* pTimeLine = pRepo->GetTimeline(i);

			Telemetry::TTelemNodePtr pOwner = pTimeLine->GetOwner();
			Telemetry::STelemetryNode::TStates states = pOwner->GetStates();
			string name = states["name"];

			if(it->compare(name) == 0)
			{
				Telemetry::CTimelineRecord* pSubTrackRecord = NULL;

				// for the owner, find out if there is already a timeline with the same name;
				//	if so, add this timeline to the same record
				for(int child = 0, childCount = pTrackRecord->GetChildCount(); child != childCount; ++child)
				{
					Telemetry::CTimelineRecord* pChildRecord = reinterpret_cast<Telemetry::CTimelineRecord*>(pTrackRecord->GetChild(child));

					for(Telemetry::TTimelines::const_iterator it = pChildRecord->m_timelines.begin(), end = pChildRecord->m_timelines.end(); it != end; ++it)
					{
						if(!strcmp((*it)->GetName(), pTimeLine->GetName()))
						{
							pSubTrackRecord = pChildRecord;
							pChildRecord->m_timelines.push_back(pTimeLine);
							break;
						}
					}

					if(pSubTrackRecord != NULL)
						break;
				}

				// was found: skip to next timeline
				if(pSubTrackRecord != NULL)
					continue;

				// not already present: create a new record and add it

				string timelineName = pTimeLine->GetName();
				if(pTimeLine->IsFiltered())
				{
					timelineName.append(" (*)");
				}
				pSubTrackRecord = new Telemetry::CTimelineRecord( timelineName.c_str(), false );
				pSubTrackRecord->SetItemHeight( TELEMETRY_TIMELINE_ITEM_HEIGHT );
				pSubTrackRecord->SetExpanded(true);
				pSubTrackRecord->m_timelines.push_back(pTimeLine);
				m_itemInfos.push_back(pSubTrackRecord);

				CXTPReportRecordItem *pSubTrackItem = new CXTPReportRecordItemText(timelineName);
				pSubTrackRecord->AddItem(pSubTrackItem);
				pSubTrackRecord->SetExpanded(true);
				pSubTrackItem->HasCheckbox(true);
				pSubTrackItem->SetChecked(true);
				pSubTrackItem->SetTextColor(::GetSysColor(COLOR_HIGHLIGHT));

				pTrackRecord->GetChilds()->Add( pSubTrackRecord );	
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
int CTelemetryViewTimelinesCtrl::GetIconIndexForParam( int type )
{
	// todo: provide images for each timeline type

 	int nImage = 13; // Default
// 	if (type == APARAM_FOV)
// 		nImage = 2;
// 	if (type == APARAM_POS)
// 		nImage = 3;
// 	if (type == APARAM_ROT)
// 		nImage = 4;
// 	if (type == APARAM_SCL)
// 		nImage = 5;
// 	if (type == APARAM_EVENT || type == APARAM_TRACKEVENT)
// 		nImage = 6;
// 	if (type == APARAM_VISIBLE)
// 		nImage = 7;
// 	if (type == APARAM_CAMERA)
// 		nImage = 8;
// 	if (type == APARAM_SOUND)
// 		nImage = 9;
// 	if (type == APARAM_ANIMATION)
// 		nImage = 10;
// 	if (type == APARAM_SEQUENCE)
// 		nImage = 11;
// 	if (type == APARAM_EXPRESSION)
// 		nImage = 12;
// 	if (type == APARAM_FLOAT_1)
// 		nImage = 13;
// 	if (type == APARAM_CAPTURE)
// 		nImage = 11;

	return nImage;
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::OnFillItems()
{
	BeginUpdate();

	m_itemInfos.clear();

	ResetContent(true);

	string root = "Session";
	Telemetry::CTimelineRecord *pRootGroupRec = new Telemetry::CTimelineRecord(root.c_str(), true);
	CXTPReportRecordItem *pRootItem = new CXTPReportRecordItemText(root.c_str());
	pRootItem->SetBold(TRUE);
	pRootItem->HasCheckbox(true);
	pRootItem->SetChecked(true);

	pRootGroupRec->SetItemHeight(TELEMETRY_TIMELINE_GROUP_HEIGHT);
	pRootGroupRec->AddItem(pRootItem);
	pRootGroupRec->SetExpanded(TRUE);
	pRootGroupRec->SetVisible(true);

	m_itemInfos.push_back(pRootGroupRec);

	AddRecord( pRootGroupRec );

	FillTimelines(pRootGroupRec);

	EnsureItemVisible(pRootGroupRec);
	RedrawControl();

 	// Additional empty record like space for scrollbar in key control
 	Telemetry::CTimelineRecord *pGroupRec = new Telemetry::CTimelineRecord();
 	pGroupRec->SetItemHeight(TELEMETRY_TIMELINE_ITEM_HEIGHT);
 	AddRecord( pGroupRec );
	m_itemInfos.push_back(pGroupRec);

	EndUpdate();

	Populate();

	// sync that should only happen on new data received.
	if(m_pTelemetryViewDialog)
	{
		const Telemetry::CTelemetryRepository* pRepo = m_pTelemetryViewDialog->GetRepository();

		uint64 start = pRepo->GetStartTime();
		uint64 end = pRepo->GetEndTime();

		if(start != std::numeric_limits<uint64>::max() && end != 0)
			m_eventsCtrl->SetTimeRange(start / 1000.0f, end/1000.0f);

		m_eventsCtrl->SetCurrTime(start/1000.0f);

		//m_eventsCtrl->SetTimeScale(1.0f, start/1000.0f);
		m_eventsCtrl->SetScrollPos( SB_HORZ, m_eventsCtrl->TimeToClient(start / 1000.0f) );
	}

	SyncEventsCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::OnItemExpanded( CXTPReportRow *pRow,bool bExpanded )
{
// 	CRecord *pRecord = (CRecord*)pRow->GetRecord();
// 	if (pRecord && pRecord->track)
// 	{
// 		IAnimTrack *track = pRecord->track;
// 		if (bExpanded)
// 		{
// 			// Mark track expanded
// 			track->SetFlags( track->GetFlags()|IAnimTrack::ATRACK_EXPANDED );
// 		}
// 		else
// 		{
// 			// Mark track collapsed.
// 			track->SetFlags( track->GetFlags()&(~IAnimTrack::ATRACK_EXPANDED) );
// 		}
// 	}
// 	else if (pRecord && pRecord->node)
// 	{
// 		IAnimNode *node = pRecord->node;
// 		if (bExpanded)
// 		{
// 			// Mark node expanded
// 			node->SetFlags( node->GetFlags()|ANODE_FLAG_EXPANDED );
// 		}
// 		else
// 		{
// 			// Mark node collapsed.
// 			node->SetFlags( node->GetFlags()&(~ANODE_FLAG_EXPANDED) );
// 		}
// 	}

	SyncEventsCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::OnSelectionChanged()
{
	m_eventsCtrl->ClearSelection();
	
// 	if (!m_sequence)
// 		return;
// 
// 	// Clear track selections.
// 	for (int i = 0; i < m_sequence->GetNodeCount(); i++)
// 	{
// 		IAnimNode *node = m_sequence->GetNode(i);
// 		for (int t = 0; t < node->GetTrackCount(); t++)
// 		{
// 			node->GetTrackByIndex(t)->SetSelected(false);
// 		}
// 	}
// 
// 
// 	int nCount = GetSelectedRows()->GetCount();
// 	for (int i = 0; i < nCount; i++)
// 	{
// 		CRecord *pRecord = (CRecord*)GetSelectedRows()->GetAt(i)->GetRecord();
// 		if (!pRecord->track)
// 			continue;
// 
// 		for (int i = 0; i < m_eventsCtrl->GetCount(); i++)
// 		{
// 			if (pRecord->track)
// 				pRecord->track->SetSelected(true);
// 			if (m_eventsCtrl->GetTrack(i) == pRecord->track)
// 			{
// 				m_eventsCtrl->SelectItem(i);
// 				break;
// 			}
// 		}
// 	}
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::SyncEventsCtrl()
{
	if (!m_eventsCtrl)
		return;
 
 	// Forces ctrl to be drawn first.
 	UpdateWindow();

	m_eventsCtrl->SetVerticalScroll(GetTopRowIndex() * TELEMETRY_TIMELINE_ITEM_HEIGHT);
	m_eventsCtrl->Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::SelectNode( const char *sName )
{
// 	CXTPReportRows *pRows = GetRows();
// 	for (int i = 0,nNumRows = pRows->GetCount(); i < nNumRows; i++)
// 	{
// 		CXTPReportRow *pRow = pRows->GetAt(i);
// 		CRecord *pRecord = (CRecord*)pRow->GetRecord();
// 		pRow->SetSelected(FALSE);
// 		if (pRecord && pRecord->node != NULL && stricmp(pRecord->node->GetName(),sName) == 0)
// 		{
// 			pRow->SetSelected(TRUE);
// 			break;
// 		}
// 	}
	SyncEventsCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::SelectInViewPort()
{
// 	AnimTimelines records;
// 	GetSelectedNodes( records );
// 	int selectedCount = records.size();
// 
// 	IAnimNode *aNode = NULL;
// 	CBaseObject *selectedObject = NULL;
// 
// 	if (selectedCount>0)
// 	{
// 		GetIEditor()->BeginUndo();
// 		GetIEditor()->ClearSelection();
// 	}
// 
// 	for (int i=0 ; i < selectedCount ; ++i)
// 	{
// 		aNode = records[i];
// 		CBaseObject *obj = GetIEditor()->GetObjectManager()->FindAnimNodeOwner(aNode);
// 		if (obj)
// 		{
// 			GetIEditor()->SelectObject(obj);
// 		}
// 	}
// 
// 	if (selectedCount>0)
// 	{
// 		GetIEditor()->AcceptUndo("Select Object(s)");
// 	}

}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::OnVerticalScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	SyncEventsCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewTimelinesCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint point;

	Telemetry::CTimelineRecord *pItemInfo = 0;

 	// Find node under mouse.
 	GetCursorPos( &point );
 	ScreenToClient( &point );

	// Select the item that is at the point myPoint.
 	CXTPReportRow* pRow = HitTest(point);
 	if (pRow)
 	{
 		pItemInfo = (Telemetry::CTimelineRecord*)pRow->GetRecord();
 	}
 
 	int cmd = ShowPopupMenu(point, pItemInfo);

	switch(cmd)
	{
		case MI_TIMELINE_COLOUR:
			{
				ColorB col = pItemInfo->m_timelines[0]->GetDebugColor();

				CCustomColorDialog dlg(col.pack_bgr888());
				if (IDOK == dlg.DoModal())
				{
					COLORREF result = dlg.GetColor();
					col.r = GetRValue(result);
					col.g = GetGValue(result);
					col.b = GetBValue(result);
					col.a = 255;

					pItemInfo->GetItem(0)->SetTextColor(result);

					for(uint i=0; i<pItemInfo->m_timelines.size(); ++i)
						pItemInfo->m_timelines[i]->SetDebugColor(col);
				}
			}
			break;

		case MI_TIMELINE_FILTER:
			{
				FilterTimeline(pItemInfo);
			}
			break;

		case MI_TIMELINE_CLEAR_FILTER:
			{
				Telemetry::STimelineSimpleFilter nullFilter; 

				for(uint i=0; i<pItemInfo->m_timelines.size(); ++i)
					pItemInfo->m_timelines[i]->Filter(nullFilter);

				m_pTelemetryViewDialog->InvalidateTrackList();
			}
	}
 
// 	float scrollPos = SaveVerticalScrollPos();
// 
// 	if (cmd == MI_SAVE_TO_COLLADA)
// 	{
// 		SaveNodeToCollada();
// 	}
// 	else if (cmd == MI_LOAD_FROM_COLLADA)
// 	{
// 		LoadNodeFromCollada();
// 	}
// 	else if (cmd == MI_SET_AS_VIEW_CAMERA)
// 	{
// 		SetAsViewCamera();
// 	}
// 	else if (cmd == MI_REMOVE_SELECTED)
// 	{
// 		DeleteSelectedNodes();
// 	}
// 	else if (cmd == MI_ADD_SELECTED_ENTITY)
// 	{
// 		m_pTelemetryViewDialog->AddSelectedNodes(pGroupNode);
// 	}
// 	else if (cmd == MI_ADD_CURRENT_LAYER)
// 	{
// 		m_pTelemetryViewDialog->AddCurrentLayer(pGroupNode);
// 	}
// 	else if (cmd == MI_ADD_SCREENFADER_NODE)
// 	{
// 		m_pTelemetryViewDialog->CreateAnimNode( ANODE_SCREENFADER, "ScreenFader", pGroupNode);
// 	}
// 	else if (cmd == MI_ADD_COMMENT_NODE)
// 	{
// 		m_pTelemetryViewDialog->CreateAnimNode( ANODE_COMMENT, "Comment",pGroupNode );
// 	}
// 	else if (cmd == MI_ADD_RADIALBLUR)
// 	{
// 		m_pTelemetryViewDialog->CreateAnimNode( ANODE_RADIALBLUR, "RadialBlur",pGroupNode);
// 	}
// 	else if (cmd == MI_ADD_COLLORCORRECTION)
// 	{
// 		m_pTelemetryViewDialog->CreateAnimNode( ANODE_COLORCORRECTION, "ColorCorrection",pGroupNode);
// 	}
// 	else if (cmd == MI_ADD_DOF)
// 	{
// 		m_pTelemetryViewDialog->CreateAnimNode( ANODE_DOF, "DepthOfField",pGroupNode);
// 	}
// 	else if (cmd == MI_ADD_HDR_SETUP)
// 	{
// 		m_pTelemetryViewDialog->CreateAnimNode( ANODE_HDRSETUP, "HdrSetup",pGroupNode);
// 	}
// 	else if (cmd == MI_ADD_SHADOW_SETUP)
// 	{
// 		m_pTelemetryViewDialog->CreateAnimNode( ANODE_SHADOWSETUP, "ShadowsSetup",pGroupNode);
// 	}
// 	else if (cmd == MI_ADD_DIRECTOR_NODE)
// 	{
// 		m_pTelemetryViewDialog->CreateAnimNode( ANODE_SCENE,"Director",pGroupNode );
// 	}
// 	else if (cmd == MI_ADD_CONSOLE_VARIABLE)
// 	{
// 		CStringDlg dlg( _T("Console Variable Name") );
// 		if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
// 		{
// 			m_pTelemetryViewDialog->CreateAnimNode( ANODE_CVAR,dlg.GetString(),pGroupNode );
// 		}
// 	}
// 	else if (cmd == MI_ADD_SCRIPT_VARIABLE)
// 	{
// 		CStringDlg dlg( _T("Script Variable Name") );
// 		if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
// 		{
// 			m_pTelemetryViewDialog->CreateAnimNode( ANODE_SCRIPTVAR,dlg.GetString(),pGroupNode );
// 		}
// 	}
// 	else if (cmd == MI_ADD_MATERIAL)
// 	{
// 		CStringDlg dlg( _T("Material Name") );
// 		if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
// 		{
// 			m_pTelemetryViewDialog->CreateAnimNode( ANODE_MATERIAL,dlg.GetString(),pGroupNode );
// 		}
// 	}
// 	else if (cmd == MI_ADD_EVENT)
// 	{
// 		CStringDlg dlg( _T("Track Event Name") );
// 		if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
// 		{
// 			m_pTelemetryViewDialog->CreateAnimNode( ANODE_EVENT,dlg.GetString(),pGroupNode );
// 		}
// 	}
// 	else if (cmd == MI_EDIT_EVENTS)
// 	{
// 		EditEvents();
// 	}
// 	else if (cmd == MI_RENAME && pItemInfo != NULL && pItemInfo->node != NULL)
// 	{
// 		RenameNode(pItemInfo);
// 
// 	}
// 	else if (cmd == MI_CREATE_FOLDER)
// 	{
// 		CreateFolder(pGroupNode);
// 	}
// 	else if (cmd == MI_DETACH_FROM_GROUP)
// 	{
// 		// Detach from group
// 		if (pItemInfo && pItemInfo->node)
// 		{
// 			IAnimNode *pNode = pItemInfo->node;
// 			pNode->SetParent(0);
// 			InvalidateNodes();
// 			SelectNode(pNode->GetName());
// 		}
// 	}
// 	else if (cmd == MI_SET_AS_ACTIVE_DIRECTOR)
// 	{
// 		if (pItemInfo && pItemInfo->node)
// 		{
// 			assert(pItemInfo->node->GetType() == ANODE_SCENE);
// 			m_sequence->SetActiveDirector(pItemInfo->node);
// 			InvalidateNodes();
// 		}
// 	}
// 	else if (cmd == MI_TRANSFORM_BONE_LINK)
// 	{
// 		if (pItemInfo && pItemInfo->node)
// 		{
// 			assert(pItemInfo->node->GetType() == ANODE_ENTITY
// 					|| pItemInfo->node->GetType() == ANODE_CAMERA);
// 			if (pItemInfo->node->IsBoneLinkTransformEnabled())
// 				pItemInfo->node->EnableBoneLinkTransform(false);
// 			else
// 				pItemInfo->node->EnableBoneLinkTransform(true);
// 			InvalidateNodes();
// 		}
// 	}
// 	else if (cmd >= MI_ADD_TRACK_BASE && cmd < MI_ADD_TRACK_BASE+1000)
// 	{
// 		if (pItemInfo)
// 		{
// 			IAnimNode *node = pItemInfo->node;
// 			if (node)
// 			{
// 				AddTrack(cmd-MI_ADD_TRACK_BASE, node);
// 			}
// 		}
// 	}
// 	else if (cmd == MI_REMOVE_TRACK)
// 	{
// 		if (pItemInfo)
// 		{
// 			IAnimNode *node = pItemInfo->node;
// 			if (node)
// 			{
// 				RemoveTrack( pItemInfo);
// 			}
// 		}
// 	}
// 	else if (cmd >= MI_SHOW_HIDE_BASE && cmd < MI_SHOW_HIDE_BASE+100)
// 	{
// 		if (pItemInfo)
// 		{
// 			IAnimNode *node = pItemInfo->node;
// 			if (node)
// 			{
// 				ShowHideTrack(node, cmd-MI_SHOW_HIDE_BASE);
// 			}
// 		}
// 	}
// 	else if(cmd==MI_COPY_SELECTED_KEYS)
// 	{
// 		m_eventsCtrl->CopyKeys();
// 	}
// 	else if(cmd==MI_PASTE_KEYS)
// 	{
// 		SItemInfo * pSii = GetSelectedNode();
// 		if(pSii)
// 			m_eventsCtrl->PasteKeys(pSii->node,pSii->track,0);
// 		else
// 			m_eventsCtrl->PasteKeys(0,0,0);
// 	}
// 	else if(cmd==MI_COPY_SELECTED_NODES)
// 	{
// 		CopySelectedNodes();
// 	}
// 	else if(cmd==MI_PASTE_NODES)
// 	{
// 		PasteNodes(pGroupNode);
// 	}
// 	else if(cmd==MI_EXPAND_ALL)
// 		ExpandNodes();
// 	else if(cmd==MI_COLLAPSE_ALL)
// 		CollapseNodes();
// 	else if(cmd==MI_EXPAND_FOLDERS)
// 		ExpandNodes(ANODE_GROUP);
// 	else if(cmd==MI_COLLAPSE_FOLDERS)
// 		CollapseNodes(ANODE_GROUP);
// 	else if(cmd==MI_EXPAND_ENTITIES)
// 		ExpandNodes(ANODE_ENTITY);
// 	else if(cmd==MI_COLLAPSE_ENTITIES)
// 		CollapseNodes(ANODE_ENTITY);
// 	else if(cmd==MI_EXPAND_CAMERAS)
// 		ExpandNodes(ANODE_CAMERA);
// 	else if(cmd==MI_COLLAPSE_CAMERAS)
// 		CollapseNodes(ANODE_CAMERA);
// 	else if(cmd==MI_EXPAND_MATERIALS)
// 		ExpandNodes(ANODE_MATERIAL);
// 	else if(cmd==MI_COLLAPSE_MATERIALS)
// 		CollapseNodes(ANODE_MATERIAL);
// 	else if(cmd==MI_EXPAND_EVENTS)
// 		ExpandNodes(ANODE_EVENT);
// 	else if(cmd==MI_COLLAPSE_EVENTS)
// 		CollapseNodes(ANODE_EVENT);
// 	else if (cmd= MI_SELECT_IN_VIEWPORT )
// 	{
// 		SelectInViewPort();
// 	}
// 
// 	if(cmd)
// 	{
// 		RestoreVerticalScrollPos(scrollPos);
// 		SyncKeyCtrl();
// 	}

	// processed
	*pResult = 1;
}

//////////////////////////////////////////////////////////////////////////
bool CTelemetryViewTimelinesCtrl::IsSelected(Telemetry::CTimelineRecord* pRecord)
{
	Records records;
	GetSelectedRecords( records );

	return stl::find(records, pRecord);
}

//////////////////////////////////////////////////////////////////////////
bool CTelemetryViewTimelinesCtrl::HasTimeline(const char *name) const
{
	if(name == NULL)
		return false;

	IAnimNode *pNode = NULL;
	//pNode = m_sequence->FindNodeByName(name);
	if (pNode)
		return true;
	else
		return false;
}

int CTelemetryViewTimelinesCtrl::ShowPopupMenu( CPoint point, const Telemetry::CTimelineRecord *pItemInfo )
{
	// Create pop up menu.
 	CMenu menu;
 
	if(pItemInfo && !pItemInfo->m_timelines.empty())
	{
 		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, MI_TIMELINE_COLOUR,"Set Color" ); 
		menu.AppendMenu(MF_STRING, MI_TIMELINE_FILTER, "Filter");
		menu.AppendMenu(MF_STRING, MI_TIMELINE_CLEAR_FILTER, "Clear Filter");
	}
 
// 	if (GetSelectedCount() > 1)
// 	{
// 		Records records;
// 		GetSelectedRecords( records );
// 		bool Timelineselected = false;
// 		for (int currentNode = 0; currentNode < GetSelectedCount(); currentNode++)
// 		{
// 			SItemInfo *pItemInfo = (SItemInfo*)records[currentNode];
// 			if (pItemInfo != NULL && pItemInfo->node != NULL && pItemInfo->track == NULL)
// 			{
// 				nodeSelected = true;
// 				break;
// 			}
// 		}
// 
// 		if (nodeSelected)
// 		{
// 			menu.AppendMenu( MF_STRING,MI_COPY_SELECTED_NODES,"Copy Selected Nodes" );
// 		}
// 
// 		menu.AppendMenu( MF_STRING,MI_REMOVE_SELECTED,"Remove Selected Nodes/Tracks" );
// 
// 		if (nodeSelected)
// 		{
// 			menu.AppendMenu( MF_SEPARATOR,0,"" );
// 			menu.AppendMenu( MF_STRING, MI_SELECT_IN_VIEWPORT ,"Select In Viewport" );
// 		}
// 
// 	}
// 	else
// 	{
// 		bool notOnValidItem = !pItemInfo || !pItemInfo->node;
// 		bool onValidItem = !notOnValidItem;
// 		bool onGroupNode = pGroupNode != NULL;
// 		bool onNode = onValidItem && pItemInfo->track == NULL;
// 		bool onTrack = onValidItem && pItemInfo->track != NULL;
// 		if (notOnValidItem || onGroupNode)
// 		{
// 			menuExpand.CreatePopupMenu();
// 			menuCollapse.CreatePopupMenu();
// 
// 			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_ALL,_T("Expand all") );
// 			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_ALL,_T("Collapse all") );
// 			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_FOLDERS,_T("Expand Folders") );
// 			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_FOLDERS,_T("Collapse Folders") );
// 			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_ENTITIES,_T("Expand Entities") );
// 			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_ENTITIES,_T("Collapse Entities") );
// 			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_CAMERAS,_T("Expand Cameras") );
// 			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_CAMERAS,_T("Collapse Cameras") );
// 			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_MATERIALS,_T("Expand Materials") );
// 			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_MATERIALS,_T("Collapse Materials") );
// 			menuExpand.AppendMenu( MF_STRING,MI_EXPAND_EVENTS,_T("Expand Events") );
// 			menuCollapse.AppendMenu( MF_STRING,MI_COLLAPSE_EVENTS,_T("Collapse Events") );
// 
// 			menu.AppendMenu(MF_POPUP,(UINT_PTR)menuExpand.m_hMenu,"Expand");
// 			menu.AppendMenu(MF_POPUP,(UINT_PTR)menuCollapse.m_hMenu,"Collapse");
// 
// 			menu.AppendMenu( MF_SEPARATOR,0,"" );
// 			if (onGroupNode)
// 			{
// 				// In case that it's a director node instead of a normal group node,
// 				if (pItemInfo->node->GetType() == ANODE_SCENE)
// 				{
// 					menu.AppendMenu( MF_STRING,MI_SET_AS_ACTIVE_DIRECTOR,"Set as Active Director" );
// 					menu.AppendMenu( MF_SEPARATOR,0,"" );
// 					if (pItemInfo->node->GetParent())
// 						menu.AppendMenu( MF_STRING,MI_DETACH_FROM_GROUP,"Detach from Group" );
// 					menu.AppendMenu( MF_STRING,MI_COPY_SELECTED_KEYS,"Copy Selected Keys");
// 					menu.AppendMenu( MF_STRING,MI_PASTE_KEYS,"Paste Keys");
// 				}
// 				menu.AppendMenu( MF_STRING,MI_COPY_SELECTED_NODES,"Copy" );
// 				menu.AppendMenu( MF_STRING,MI_RENAME,"Rename" );
// 				menu.AppendMenu( MF_STRING,MI_REMOVE_SELECTED,"Delete" );
// 			}
// 			menu.AppendMenu( MF_STRING,MI_CREATE_FOLDER,"Create Folder" );
// 			menu.AppendMenu( MF_SEPARATOR,0,"" );
// 			menu.AppendMenu( MF_STRING,MI_ADD_SELECTED_ENTITY,_T("Add Selected Entity(s)") );
// 
// 			if( m_pTelemetryViewDialog && !m_pTelemetryViewDialog->IsCurrentLayerInSequence())
// 				menu.AppendMenu( MF_STRING,MI_ADD_CURRENT_LAYER,_T("Add Current Layer") );
// 
// 			//! 
// 			menu.AppendMenu( MF_SEPARATOR,0,"" );
// 
// 			if(m_pTelemetryViewDialog && !m_pTelemetryViewDialog->IsAnimNodeInCurrentSequence(ANODE_RADIALBLUR))
// 				menu.AppendMenu( MF_STRING, MI_ADD_RADIALBLUR,_T("Add Radial Blur Node"));
// 
// 			if(m_pTelemetryViewDialog && !m_pTelemetryViewDialog->IsAnimNodeInCurrentSequence(ANODE_COLORCORRECTION))
// 				menu.AppendMenu( MF_STRING, MI_ADD_COLLORCORRECTION,_T("Add Color Correction Node"));
// 
// 			if(m_pTelemetryViewDialog && !m_pTelemetryViewDialog->IsAnimNodeInCurrentSequence(ANODE_DOF))
// 				menu.AppendMenu( MF_STRING, MI_ADD_DOF,_T("Add Depth of Field Node"));
// 
// 			if(m_pTelemetryViewDialog && !m_pTelemetryViewDialog->IsAnimNodeInCurrentSequence(ANODE_SCREENFADER))
// 				menu.AppendMenu( MF_STRING,MI_ADD_SCREENFADER_NODE,_T("Add Screen Fader") );
// 
// 			if(m_pTelemetryViewDialog && !m_pTelemetryViewDialog->IsAnimNodeInCurrentSequence(ANODE_HDRSETUP))
// 				menu.AppendMenu( MF_STRING, MI_ADD_HDR_SETUP,_T("Add HDR Setup Node"));
// 
// 			if(m_pTelemetryViewDialog && !m_pTelemetryViewDialog->IsAnimNodeInCurrentSequence(ANODE_SHADOWSETUP))
// 				menu.AppendMenu( MF_STRING, MI_ADD_SHADOW_SETUP,_T("Add Shadows Setup Node"));
// 
// 			menu.AppendMenu( MF_SEPARATOR,0,"" );
// 			// A director node cannot have another director node as a child.
// 			if (notOnValidItem || pItemInfo->node->GetType() != ANODE_SCENE)
// 				menu.AppendMenu( MF_STRING,MI_ADD_DIRECTOR_NODE,_T("Add Director(Scene) Node") );
// 			menu.AppendMenu( MF_STRING,MI_ADD_COMMENT_NODE,_T("Add Comment Node") );
// 			menu.AppendMenu( MF_STRING,MI_ADD_CONSOLE_VARIABLE,_T("Add Console Variable") );
// 			menu.AppendMenu( MF_STRING,MI_ADD_SCRIPT_VARIABLE,_T("Add Script Variable") );
// 			menu.AppendMenu( MF_STRING,MI_ADD_MATERIAL,_T("Add Material") );
// 			menu.AppendMenu( MF_STRING,MI_ADD_EVENT,_T("Add Event") );
// 			menu.AppendMenu( MF_SEPARATOR,0,"" );
// 			menu.AppendMenu( MF_STRING,MI_EDIT_EVENTS,_T("Edit Events") );
// 			menu.AppendMenu( MF_SEPARATOR,0,"" );
// 			menu.AppendMenu( MF_STRING,MI_PASTE_NODES,_T("Paste Node(s)") );
// 		}
// 		else	// On a node other than group node or on a track
// 		{
// 			if (onNode)
// 			{
// 				menu.AppendMenu( MF_STRING,MI_CREATE_FOLDER,"Create Folder" );
// 				if (pItemInfo->node->GetParent())
// 					menu.AppendMenu( MF_STRING,MI_DETACH_FROM_GROUP,"Detach from Group" );
// 
// 				menu.AppendMenu( MF_SEPARATOR,0,"" );
// 				menu.AppendMenu( MF_STRING,MI_COPY_SELECTED_NODES,"Copy" );
// 				menu.AppendMenu( MF_STRING,MI_COPY_SELECTED_KEYS,"Copy Selected Keys");
// 				menu.AppendMenu( MF_SEPARATOR,0,"" );
// 				menu.AppendMenu( MF_STRING,MI_REMOVE_SELECTED,"Delete" );
// 			}
// 			else	// On a track
// 			{
// 				menu.AppendMenu( MF_STRING,MI_COPY_SELECTED_KEYS,"Copy Keys");
// 			}
// 			if (onValidItem)
// 			{
// 				if (pItemInfo->node->GetFlags() & ANODE_FLAG_CAN_CHANGE_NAME)
// 				{
// 					menu.AppendMenu( MF_STRING,MI_RENAME,"Rename" );
// 				}
// 			}
// 
// 			menu.AppendMenu( MF_STRING,MI_PASTE_KEYS,"Paste Keys");
// 
// 			if (onNode)
// 			{
// 				menu.AppendMenu( MF_SEPARATOR,0,"" );
// 				menu.AppendMenu( MF_STRING,MI_SELECT_IN_VIEWPORT,"Select In Viewport" );
// 			}
// 
// 			if (onNode)
// 			{
// 				menu.AppendMenu( MF_SEPARATOR,0,"" );
// 				bool bEntityOrCameraNode = pItemInfo->node->GetType() == ANODE_ENTITY 
// 																|| pItemInfo->node->GetType() == ANODE_CAMERA;
// 				UINT flag = bEntityOrCameraNode?MF_STRING:(MF_STRING|MF_DISABLED);
// 				menu.AppendMenu( flag,MI_SAVE_TO_COLLADA,_T("Save to a COLLADA file...") );
// 				menu.AppendMenu( flag,MI_LOAD_FROM_COLLADA,_T("Load from a COLLADA file...") );
// 
// 				if (bEntityOrCameraNode)
// 				{
// 					bool bHasBoneLink = false;
// 					CBaseObject *pObject = GetIEditor()->GetObjectManager()->FindAnimNodeOwner(pItemInfo->node);
// 					if (pObject)
// 					{
// 						if (static_cast<CEntityObject*>(pObject)->GetBoneAttachLinkIndex() >= 0)
// 							bHasBoneLink = true;
// 					}
// 					flag = bHasBoneLink?MF_STRING:(MF_STRING|MF_DISABLED);
// 					bool bBoneLinkXformEnabled = pItemInfo->node->IsBoneLinkTransformEnabled();
// 					menu.AppendMenu( flag,MI_TRANSFORM_BONE_LINK,bBoneLinkXformEnabled?	_T("Transform Entity") :
// 																																							_T("Transform Bone Link") );
// 				}
// 
// 				menu.AppendMenu( MF_SEPARATOR,0,"" );
// 				if (pItemInfo->node->GetType() == ANODE_CAMERA)
// 				{
// 					menu.AppendMenu( MF_STRING,MI_SET_AS_VIEW_CAMERA,_T("Set As View Camera") );
// 				}
// 			}
// 		}
// 
// 		// add tracks menu
// 		menuAddTrack.CreatePopupMenu();
// 		bool bTracksToAdd=false;
// 		if (onValidItem)
// 		{
// 			menu.AppendMenu( MF_SEPARATOR,0,"" );
// 			IAnimNode::SParamInfo paramInfo;
// 			// List`s which tracks can be added to animation node.
// 			for (int i = 0; i < pItemInfo->node->GetParamCount(); i++)
// 			{
// 				if (!pItemInfo->node->GetParamInfo( i,paramInfo ))
// 					continue;
// 
// 				int flags = 0;
// 				IAnimTrack *track = pItemInfo->node->GetTrackForParameter( paramInfo.paramId );
// 				if (track && !(paramInfo.flags&IAnimNode::PARAM_MULTIPLE_TRACKS))
// 				{
// 					continue;
// 					//flags |= MF_CHECKED;
// 				}
// 
// 				menuAddTrack.AppendMenu( MF_STRING|flags,MI_ADD_TRACK_BASE+paramInfo.paramId,paramInfo.name );
// 				bTracksToAdd=true;
// 			}
// 		}
// 		if (bTracksToAdd)
// 			menu.AppendMenu(MF_POPUP,(UINT_PTR)menuAddTrack.m_hMenu,"Add Track");
// 
// 		// delete track menu
// 		if (onTrack)
// 		{
// 			menu.AppendMenu(MF_STRING, MI_REMOVE_TRACK, "Remove Track");
// 		}
// 
// 		if (bTracksToAdd || onTrack)
// 			menu.AppendMenu( MF_SEPARATOR,0,"" );
// 
// 		if (onValidItem)
// 		{
// 			CString str;
// 			str.Format( "%s Tracks",pItemInfo->node->GetName() );
// 			menu.AppendMenu( MF_STRING|MF_DISABLED,0,str );
// 
// 			// Show tracks in anim node.
// 			{
// 				IAnimNode::SParamInfo paramInfo;
// 				for (int i = 0; i < pItemInfo->node->GetTrackCount(); i++)
// 				{
// 					IAnimTrack *track = pItemInfo->node->GetTrackByIndex(i);
// 
// 					if (!pItemInfo->node->GetParamInfoFromId( track->GetParameterType(),paramInfo ))
// 						continue;
// 
// 					// change hidden flag for this track.
// 					int checked = MF_CHECKED;
// 					if (track->GetFlags() & IAnimTrack::ATRACK_HIDDEN)
// 					{
// 						checked = MF_UNCHECKED;
// 					}
// 					menu.AppendMenu( MF_STRING|checked,MI_SHOW_HIDE_BASE+i,CString( "  " ) + paramInfo.name );
// 				}
// 			}
// 		}
// 	}
// 

 	GetCursorPos( &point );

 	// track menu
 	return CXTPCommandBars::TrackPopupMenu(&menu, TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, 
 																					point.x,point.y, this, NULL);
}

//-----------------------------------------------------------------------------
void CTelemetryViewTimelinesCtrl::SetPopupMenuLock(CMenu* menu)
{
	if(!menu)
		return;

	UINT count = menu->GetMenuItemCount();
	for(UINT i=0; i<count;++i)
	{
		CString menuString;
		menu->GetMenuString(i,menuString, MF_BYPOSITION);

		if(menuString!="Expand" && menuString!="Collapse")
			menu->EnableMenuItem(i,MF_DISABLED | MF_GRAYED | MF_BYPOSITION );
	}
}


void CTelemetryViewTimelinesCtrl::OnSize(UINT nType, int cx, int cy) 
{
	__super::OnSize(nType, cx, cy);

	SyncEventsCtrl();
}

//-----------------------------------------------------------------------------
BOOL CTelemetryViewTimelinesCtrl::OnEraseBkgnd(CDC* pDC)
{
	CXTPReportPaintManager* pPaintManager = GetPaintManager();
	if(pPaintManager)
	{
		COLORREF backGround(COLORREF_NULL);

		pPaintManager->m_clrControlBack.SetCustomValue(backGround);
	}
	return TRUE;
}

float CTelemetryViewTimelinesCtrl::SaveVerticalScrollPos() const
{
	int sbMin=0, sbMax=0;
	GetScrollRange(SB_VERT, &sbMin, &sbMax);
	return float(GetTopRowIndex()) / std::max(float(sbMax - sbMin), 1.0f);
}

void CTelemetryViewTimelinesCtrl::RestoreVerticalScrollPos(float fScrollPos)
{
	int sbMin=0, sbMax=0;
	GetScrollRange(SB_VERT, &sbMin, &sbMax);
	int newScrollPos = FloatToIntRet(fScrollPos * (sbMax - sbMin) + sbMin);
	SetTopRow(newScrollPos);
}

////////////////////////////////////////////////////////////////////////
// bool CTelemetryViewTimelinesCtrl::ShouldRenderTimeline( const char *name )
// {
//  	CXTPReportRows *pRows = GetRows();
//  	for (int i = 0,nNumRows = pRows->GetCount(); i < nNumRows; i++)
//  	{
//  		CXTPReportRow *pRow = pRows->GetAt(i);
//  		CRecord *pRecord = (CRecord*)pRow->GetRecord();
//  		if (pRecord && stricmp(pRecord->GetName(),name) == 0)
//  		{
//  			return pRecord->GetItem(0)->IsChecked();
//  		}
//  	}
// 
// 	return false;
// }

void CTelemetryViewTimelinesCtrl::UpdateRenderStatus()
{
	uint count = m_itemInfos.size();
	Records selectedRecords;
	GetSelectedRecords( selectedRecords );

	for(uint i=0; i<count; ++i)
	{
		Telemetry::CTimelineRecord* pRecord = m_itemInfos[i];

		bool render = pRecord->GetItem(0) ? pRecord->GetItem(0)->IsChecked() : false;

		if(render != pRecord->m_bWasChecked)
		{
			// mark all children for the changed state
			for(uint j = 0, childCount = pRecord->GetChildCount(); j < childCount; ++j)
			{
				Telemetry::CTimelineRecord* pChild = reinterpret_cast<Telemetry::CTimelineRecord*>(pRecord->GetChild(j));
				pChild->GetItem(0)->SetChecked(render);
			}

			pRecord->m_bWasChecked = render;
		}

		if(render)
		{
			for(Telemetry::TTimelines::const_iterator it = pRecord->m_timelines.begin(), end = pRecord->m_timelines.end(); it != end; ++it)
			{
				Telemetry::CTelemetryTimeline* pTimeline = *it;
				if(pTimeline)
				{
					bool selected = stl::find(selectedRecords, pRecord);

					pTimeline->SetRendering(render, selected);
				}
			}

		}
	}
}

void CTelemetryViewTimelinesCtrl::FilterTimeline(Telemetry::CTimelineRecord* pItemInfo)
{
	CTimelineEventsFilterDialog dlg(pItemInfo);
	if(dlg.DoModal() == IDOK)
	{

	}
}
