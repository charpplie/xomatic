////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   VisualBudgetTool.h
//  Created:     6/10/2009 by Paulo Zaffari.
//  Description: This class implements the tool which gathers the position
//               of the terrain around which we want to analyze the budget.
//							 One of the uses for this is selecting which region we want
//							 to analyze in terms of memory by having a detailed report.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <InitGuid.h>
#include "VisualBudgetTool.h"
#include "..\Viewport.h"
#include "..\ViewManager.h"
#include "..\Terrain\Heightmap.h"
#include "..\GameEngine.h"

#include "..\Visual Budget System/VisualBudgetSystem.h"
#include "..\Asset Browser/AssetBrowserDialog.h"

//////////////////////////////////////////////////////////////////////////
typedef CVisualBudgetMode::CVisualBudgetModeClassDesc CVisualBudgetModeClassDesc;

IMPLEMENT_DYNCREATE(CVisualBudgetMode,CEditTool)
REGISTER_CLASS_DESC(CVisualBudgetModeClassDesc);

//////////////////////////////////////////////////////////////////////////
CVisualBudgetMode::CVisualBudgetMode()
{
	m_pClassDesc = GetIEditor()->GetClassFactory()->FindClass(VISUAL_BUDGET_MODE_GUID);
	SetStatusText( _T("Visual Budget System: Selection") );

	m_openContext = false;
	m_commandMode = NothingMode;

	m_boAABBSelectionStart=false;
	m_boAABBDragSelection=false;
	m_stSelectedAABB.Reset();
}

//////////////////////////////////////////////////////////////////////////
CVisualBudgetMode::~CVisualBudgetMode()
{
	// Restore the original full contents of the asset browser.
	CAssetBrowserDialog * pDlg = CVisualBudgetSystem::GetVisualBudgetSystem().GetAssetBrowserDialog();
	if (pDlg)
	{
		//pDlg->HideAllDatabases();
		//pDlg->ViewAllDatabases();
	}
}

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetMode::Display( struct DisplayContext &dc )
{
}

//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	switch (event) 
	{
		case eMouseLDown:
		{
			return OnLButtonDown(view,flags,point);
		}
		break;

		case eMouseLUp:
		{
			return OnLButtonUp(view,flags,point);
		}
		break;

		case eMouseLDblClick:
		{
			return OnLButtonDblClk(view,flags,point);
		}
		break;

		case eMouseRDown:
		{
			return OnRButtonDown(view,flags,point);
		}
		break;

		case eMouseRUp:
		{
			return OnRButtonUp(view,flags,point);
		}
		break;

		case eMouseMove:
		{
			return OnMouseMove(view,flags,point);
		}
		break;

		case eMouseMDown:
		{
			return OnMButtonDown(view,flags,point);
		}
		break;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE)
	{
		CVisualBudgetSystem::GetVisualBudgetSystem().SetDetailedAnalysisStatus(false);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::OnKeyUp( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	return false;
}
//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::OnLButtonDown( CViewport *view,int nFlags, CPoint point) 
{
	// CPointF ptMarker;
	CPoint ptCoord;
	int iCurSel = -1;

	if (GetIEditor()->IsInGameMode())
	{
		// Ignore clicks while in game.
		return false;
	}

	// Save the mouse down position
	m_cMouseDownPos = point;
	m_cCurrentMousePos=m_cMouseDownPos;

	view->ResetSelectionRegion();

	m_boAABBDragSelection = CheckVirtualKey(VK_SHIFT);

	m_boAABBSelectionStart=false;

	Vec3 pos;
	if (CalculateHeightmapPointFromDisplayPoint(m_cCurrentMousePos,pos))
	{
		m_boAABBSelectionStart=CVisualBudgetSystem::GetVisualBudgetSystem().GetSectorBoundingBoxFromPoint(pos,m_stSelectedAABB);
		if (m_boAABBSelectionStart)
		{
			m_regionsBackup = CVisualBudgetSystem::GetVisualBudgetSystem().GetDetailedAnalysisRegions();
			if (CheckVirtualKey(VK_CONTROL) == false)
				CVisualBudgetSystem::GetVisualBudgetSystem().ClearDetailedAnalysisRegions();
			CVisualBudgetSystem::GetVisualBudgetSystem().AddDetailedAnalysisRegion(m_stSelectedAABB);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	view->CaptureMouse();
	//////////////////////////////////////////////////////////////////////////

	UpdateStatusText();

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::OnLButtonUp( CViewport *view,int nFlags, CPoint point) 
{
	if (GetIEditor()->IsInGameMode())
	{
		// Ignore clicks while in game.
		return true;
	}

	// Reset the status bar caption
	GetIEditor()->SetStatusText("Ready");

	if (m_boAABBSelectionStart)
	{
		bool cancelled = true;
		CAssetBrowserDialog * pDlg = CVisualBudgetSystem::GetVisualBudgetSystem().GetAssetBrowserDialog();
		if (m_boAABBDragSelection)
		{
			if (pDlg)
				pDlg->UpdateAssetDatabaseForVisualBudgetSystem(CVisualBudgetSystem::GetVisualBudgetSystem().GetDetailedAnalysisRegions());
			cancelled = false;
		}
		else
		{
			Vec3 pos;
			if (CalculateHeightmapPointFromDisplayPoint(point,pos))
			{
				AABB stCurrentAABB;
				if (CVisualBudgetSystem::GetVisualBudgetSystem().GetSectorBoundingBoxFromPoint(pos,stCurrentAABB))
				{
					if (IsEquivalent(stCurrentAABB,m_stSelectedAABB,0.0f))
					{
						if(pDlg)
							pDlg->UpdateAssetDatabaseForVisualBudgetSystem(CVisualBudgetSystem::GetVisualBudgetSystem().GetDetailedAnalysisRegions());
						cancelled = false;
					}
				}
			}
		}
		if (cancelled)
		{
			// Restore the original analysis regions.
			CVisualBudgetSystem::GetVisualBudgetSystem().ClearDetailedAnalysisRegions();
			for (size_t i(0); i<m_regionsBackup.size(); ++i)
				CVisualBudgetSystem::GetVisualBudgetSystem().AddDetailedAnalysisRegion(m_regionsBackup[i]);
		}
		m_regionsBackup.clear();
		m_stSelectedAABB.Reset();
		m_boAABBSelectionStart=false;
		m_boAABBDragSelection=false;
	}

	//////////////////////////////////////////////////////////////////////////
	//if (view->IsUndoRecording())
	//{
	//	if (GetCommandMode() == MoveMode)
	//	{
	//		view->AcceptUndo( "Move Selection" );
	//	}
	//	else if (GetCommandMode() == RotateMode)
	//	{
	//		view->AcceptUndo( "Rotate Selection" );
	//	}
	//	else if (GetCommandMode() == ScaleMode)
	//	{
	//		view->AcceptUndo( "Scale Selection" );
	//	}
	//	else
	//	{
	//		view->CancelUndo();
	//	}
	//}
	//////////////////////////////////////////////////////////////////////////

	if (GetCommandMode() == SelectMode && (!GetIEditor()->IsSelectionLocked()))
	{
		bool bUnselect = CheckVirtualKey(VK_MENU);
		CRect selectRect = view->GetSelectionRectangle();
		if (!selectRect.IsRectEmpty())
		{
			// Ignore too small rectangles.
			if (selectRect.Width() > 5 && selectRect.Height() > 5)
			{
				GetIEditor()->GetObjectManager()->SelectObjectsInRect( view,selectRect,!bUnselect );
			}
		}

	//	if (GetIEditor()->GetEditMode() == eEditModeSelectArea)
	//	{
	//		BBox box;
	//		GetIEditor()->GetSelectedRegion( box );

	//		//////////////////////////////////////////////////////////////////////////
	//		GetIEditor()->ClearSelection();

	//		SEntityProximityQuery query;
	//		query.box = box;
	//		gEnv->pEntitySystem->QueryProximity( query );
	//		for (int i = 0; i < query.nCount; i++)
	//		{
	//			IEntity *pIEntity = query.pEntities[i];
	//			CEntity *pEntity = CEntity::FindFromEntityId( pIEntity->GetId() );
	//			if (pEntity)
	//			{
	//				GetIEditor()->GetObjectManager()->SelectObject( pEntity );
	//			}
	//		}
	//		//////////////////////////////////////////////////////////////////////////
	//		/*

	//		if (fabs(box.min.x-box.max.x) > 0.5f && fabs(box.min.y-box.max.y) > 0.5f)
	//		{
	//		//@FIXME: restore it later.
	//		//Timur[1/14/2003]
	//		//SelectRectangle( box,!bUnselect );
	//		//SelectObjectsInRect( m_selectedRect,!bUnselect );
	//		GetIEditor()->GetObjectManager()->SelectObjects( box,bUnselect );
	//		GetIEditor()->UpdateViews(eUpdateObjects);
	//		}
	//		*/
	//	}
	}
	// Release the restriction of the cursor
	view->ReleaseMouse();

	//if (GetIEditor()->GetEditMode() != eEditModeSelectArea)
	//{
	//	view->ResetSelectionRegion();
	//}
	// Reset selected rectangle.
	view->SetSelectionRectangle( CPoint(0,0),CPoint(0,0) );

	//// Restore default editor axis constrain.
	//if (GetIEditor()->GetAxisConstrains() != view->GetAxisConstrain())
	//{
	//	view->SetAxisConstrain( GetIEditor()->GetAxisConstrains() );
	//}

	SetCommandMode( NothingMode );

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::OnLButtonDblClk( CViewport *view,int nFlags, CPoint point)
{
	// If shift clicked, Move the camera to this place.
	if (nFlags & MK_SHIFT)
	{
		// Get the heightmap coordinates for the click position
		Vec3 v = view->ViewToWorld( point );
		if (!(v.x == 0 && v.y == 0 && v.z == 0))
		{
			Matrix34 tm = view->GetViewTM();
			Vec3 p = tm.GetTranslation();
			float height = p.z - GetIEditor()->GetTerrainElevation(p.x,p.y);
			if (height < 1) height = 1;
			p.x = v.x;
			p.y = v.y;
			p.z = GetIEditor()->GetTerrainElevation( p.x,p.y ) + height;
			tm.SetTranslation(p);
		}
	}
	else
	{
		// Check if double clicked on object.
		HitContext hitInfo;
		view->HitTest( point,hitInfo );

		CBaseObject *hitObj = hitInfo.object;
		if (hitObj)
		{
			// Fire double click event on hitted object.
			hitObj->OnEvent( EVENT_DBLCLICK );
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::OnRButtonDown( CViewport *view,int nFlags, CPoint point) 
{
	// Save the mouse down position
	m_openContext = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::OnRButtonUp( CViewport *view,int nFlags, CPoint point) 
{
	if (m_openContext)
	{
		//// Check if double clicked on object.
		//HitContext hitInfo;
		//view->HitTest( point,hitInfo );

		//CBaseObject *hitObj = hitInfo.object;
		//if (hitObj && hitObj->IsKindOf(RUNTIME_CLASS(CEntity)))
		//{
			//CEntity *pEntity = (CEntity*)hitObj;

			//std::vector<CFlowGraph*> flowgraphs;
			//CFlowGraph* entityFG = 0;
			//FlowGraphHelpers::FindGraphsForEntity(pEntity, flowgraphs, entityFG);

			unsigned int id=1;
			CMenu menu;
			menu.CreatePopupMenu();

			unsigned int createFGId = 0;
			//if (NULL == pEntity->GetFlowGraph())
			//{
			//	createFGId = id;
			//	menu.AppendMenu(MF_STRING, createFGId, "Create Flow Graph");
			//	menu.AppendMenu(MF_SEPARATOR);
			//	++id;
			//}

			//unsigned int baseFGId = id;
			//if (flowgraphs.size() > 0)
			//{
			//	CMenu fgMenu;
			//	fgMenu.CreatePopupMenu();

			//	std::vector<CFlowGraph*>::const_iterator iter (flowgraphs.begin());
			//	while (iter != flowgraphs.end()) 
			//	{
			//		CString name;
			//		FlowGraphHelpers::GetHumanName(*iter, name);
			//		if (*iter == entityFG) {
			//			name+=" <GraphEntity>";
			//			fgMenu.AppendMenu(MF_STRING, id, name);
			//			if (flowgraphs.size() > 1) fgMenu.AppendMenu(MF_SEPARATOR);
			//		} else {
			//			fgMenu.AppendMenu(MF_STRING, id, name);
			//		}
			//		++id;
			//		++iter;
			//	}
			//	menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(fgMenu.GetSafeHmenu()), "Flow Graphs");
			//	menu.AppendMenu(MF_SEPARATOR);
			//}

			//// TrackView sequences
			//unsigned int sequenceId = id;
			//IAnimNode *pAnimNode = pEntity->HasOwnedAnimNode();
			//if(pAnimNode)
			//{
			//	IAnimSequence *pSequence = pAnimNode->GetSequence();

			//	if (pSequence)
			//	{
			//		CMenu sequenceMenu;
			//		sequenceMenu.CreatePopupMenu();
			//		sequenceMenu.AppendMenu(MF_STRING, id++, pSequence->GetName());

			//		menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(sequenceMenu.GetSafeHmenu()), "Track Views");
			//		menu.AppendMenu(MF_SEPARATOR);
			//	}
			//}

			//// events
			//unsigned int baseEventId = id;
			//CEntityScript *pScript = pEntity->GetScript();

			//if (pScript && pScript->GetEventCount()>0)
			//{
			//	CMenu eventMenu;
			//	eventMenu.CreatePopupMenu();
			//	for (int i=0; i<pScript->GetEventCount(); ++i)
			//	{
			//		CString sourceEvent = pScript->GetEvent(i);
			//		eventMenu.AppendMenu(MF_STRING, id++, sourceEvent);
			//	}

			//	menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(eventMenu.GetSafeHmenu()), "Events");
			//	menu.AppendMenu(MF_SEPARATOR);
			//}

			//// add reload script
			//unsigned int reloadScriptId = id;
			//menu.AppendMenu(MF_STRING, reloadScriptId, "Reload Script");
			//++id;

			CPoint p;
			::GetCursorPos(&p);
			unsigned int chosen = menu.TrackPopupMenuEx( TPM_RETURNCMD|TPM_LEFTBUTTON|TPM_TOPALIGN|TPM_LEFTALIGN, p.x, p.y, view, NULL );
			if (chosen > 0)
			{
			//	if (chosen == reloadScriptId)
			//	{
			//		CEntityScript *script = pEntity->GetScript();
			//		if (script)
			//		{
			//			script->Reload();
			//		}
			//		pEntity->Reload(true);
			//	}
			//	else if (chosen == createFGId)
			//	{
			//		pEntity->CreateFlowGraphWithGroupDialog();
			//		pEntity->OpenFlowGraph("");
			//	}
			//	else if ((chosen-baseFGId) < flowgraphs.size())
			//	{
			//		GetIEditor()->GetFlowGraphManager()->OpenView( flowgraphs[chosen-baseFGId] );
			//		CWnd* pWnd = GetIEditor()->FindView( "Flow Graph" );
			//		if ( pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CHyperGraphDialog)) )
			//		{
			//			CHyperGraphDialog* pHGDlg = (CHyperGraphDialog*) pWnd;
			//			CFlowGraphSearchCtrl* pSC = pHGDlg->GetSearchControl();
			//			if (pSC)
			//			{
			//				CFlowGraphSearchOptions* pOpts = CFlowGraphSearchOptions::GetSearchOptions();
			//				pOpts->m_bIncludeEntities = true;
			//				pOpts->m_findSpecial = CFlowGraphSearchOptions::eFLS_None;
			//				pOpts->m_LookinIndex = CFlowGraphSearchOptions::eFL_Current;
			//				pSC->Find(pEntity->GetName(), false, true, true);
			//			}
			//		}
			//	}
			//	else if (pAnimNode && pAnimNode->GetSequence()
			//		&& chosen == sequenceId)
			//	{
			//		if(GetIEditor()->FindView("Track View") == NULL)
			//			GetIEditor()->OpenView("Track View");
			//		CWnd* pWnd = GetIEditor()->FindView("Track View");
			//		if ( pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CTrackViewDialog)) )
			//		{
			//			CTrackViewDialog* pTVDlg = (CTrackViewDialog*) pWnd;
			//			pTVDlg->SetCurrentSequence(pAnimNode->GetSequence());
			//			pTVDlg->SelectNode(pAnimNode->GetName());
			//		}				
			//	}
			//	else
			//	{
			//		CEntityScript *pScript = pEntity->GetScript();
			//		if (pScript && pEntity->GetIEntity())
			//			pScript->SendEvent( pEntity->GetIEntity(), pScript->GetEvent(chosen - baseEventId));
			//	}
			//}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::OnMButtonDown( CViewport *view,int nFlags, CPoint point) 
{
	if (GetIEditor()->GetGameEngine()->GetSimulationMode())
	{
		// Get control key status.
		bool bAltClick = CheckVirtualKey(VK_MENU);
		bool bCtrlClick = CheckVirtualKey(VK_CONTROL);
		bool bShiftClick = CheckVirtualKey(VK_SHIFT);

		if (bCtrlClick)
		{
			// In simulation mode awake objects under the cursor when Ctrl+MButton pressed.
			//AwakeObjectAtPoint(view,point);
			return true;
		}
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::CheckVirtualKey( int virtualKey )
{
	GetAsyncKeyState(virtualKey);
	if (GetAsyncKeyState(virtualKey))
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetMode::MoveSelectionToPos( CViewport *view,Vec3 &pos )
{
	view->BeginUndo();
	// Find center of selection.
	Vec3 center = GetIEditor()->GetSelection()->GetCenter();
	GetIEditor()->GetSelection()->Move( pos-center,false,true );
	view->AcceptUndo( "Move Selection" );
}

//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::OnMouseMove( CViewport *view,int nFlags, CPoint point)
{
	if (GetIEditor()->IsInGameMode())
	{
		// Ignore while in game.
		return true;
	}

	m_openContext = false;
	//SetObjectCursor(view,0);

	m_cCurrentMousePos=point;
	Vec3 pos = view->SnapToGrid( view->ViewToWorld( point ) );

	// get world/local coordinate system setting.
	int coordSys = GetIEditor()->GetReferenceCoordSys();

	//// get current axis constrains.
	//if (GetCommandMode() == MoveMode)
	//{
	//	GetIEditor()->RestoreUndo();

	//	Vec3 v;
	//	//m_cMouseDownPos = point;
	//	bool followTerrain = false;
	//	if (view->GetAxisConstrain() == AXIS_TERRAIN)
	//	{
	//		followTerrain = true;
	//		Vec3 p1 = view->SnapToGrid(view->ViewToWorld( m_cMouseDownPos ));
	//		Vec3 p2 = view->SnapToGrid(view->ViewToWorld( point ));
	//		v = p2 - p1;
	//		v.z = 0;
	//	}
	//	else
	//	{
	//		Vec3 p1 = view->MapViewToCP(m_cMouseDownPos);
	//		Vec3 p2 = view->MapViewToCP(point);
	//		if (p1.IsZero() || p2.IsZero())
	//			return true;
	//		v = view->GetCPVector(p1,p2);

	//		//Matrix invParent = m_parentConstructionMatrix;
	//		//invParent.Invert();
	//		//p1 = invParent.TransformVector(p1);
	//		//p2 = invParent.TransformVector(p2);
	//		//v = p2 - p1;
	//	}

	//	GetIEditor()->GetSelection()->Move( v,followTerrain,coordSys );

	//	return true;
	//}
	//else if (GetCommandMode() == RotateMode)
	//{
	//	GetIEditor()->RestoreUndo();

	//	Ang3 ang(0,0,0);
	//	float ax = point.x - m_cMouseDownPos.x;
	//	float ay = point.y - m_cMouseDownPos.y;
	//	switch (view->GetAxisConstrain())
	//	{
	//	case AXIS_X: ang.x = ay; break;
	//	case AXIS_Y: ang.y = ay; break;
	//	case AXIS_Z: ang.z = ay; break;
	//	case AXIS_XY: ang(ax,ay,0); break;
	//	case AXIS_XZ: ang(ax,0,ay); break;
	//	case AXIS_YZ: ang(0,ay,ax); break;
	//	case AXIS_TERRAIN: ang(ax,ay,0); break;
	//	};

	//	ang = view->GetViewManager()->GetGrid()->SnapAngle(ang);

	//	//m_cMouseDownPos = point;
	//	GetIEditor()->GetSelection()->Rotate( ang,coordSys );
	//	return true;
	//}
	//else if (GetCommandMode() == ScaleMode)
	//{
	//	GetIEditor()->RestoreUndo();

	//	Vec3 scl(0,0,0);
	//	float ay = 1.0f - 0.01f*(point.y - m_cMouseDownPos.y);
	//	if (ay < 0.01f) ay = 0.01f;
	//	scl(ay,ay,ay);
	//	switch (view->GetAxisConstrain())
	//	{
	//	case AXIS_X: scl(ay,1,1); break;
	//	case AXIS_Y: scl(1,ay,1); break;
	//	case AXIS_Z: scl(1,1,ay); break;
	//	case AXIS_XY: scl(ay,ay,ay); break;
	//	case AXIS_XZ: scl(ay,ay,ay); break;
	//	case AXIS_YZ: scl(ay,ay,ay); break;
	//	case AXIS_XYZ: scl(ay,ay,ay); break;
	//	case AXIS_TERRAIN: scl(ay,ay,ay); break;
	//	};
	//	GetIEditor()->GetSelection()->Scale( scl,coordSys );
	//	return true;
	//}
	//else if (GetCommandMode() == SelectMode)
	//{
	//	// Ignore select when selection locked.
	//	if (GetIEditor()->IsSelectionLocked())
	//		return true;

	//	CRect rc( m_cMouseDownPos,point );
	//	if (GetIEditor()->GetEditMode() == eEditModeSelectArea)
	//		view->OnDragSelectRectangle( CPoint(rc.left,rc.top),CPoint(rc.right,rc.bottom),false );
	//	else
	//	{
	//		view->SetSelectionRectangle( rc.TopLeft(),rc.BottomRight() );
	//	}
	//	//else
	//	//OnDragSelectRectangle( CPoint(rc.left,rc.top),CPoint(rc.right,rc.bottom),true );
	//}

	if (!(nFlags & MK_RBUTTON))
	{
		//// Track mouse movements.
		//HitContext hitInfo;
		//if (view->HitTest( point,hitInfo ))
		//{
		//	SetObjectCursor(view,hitInfo.object);
		//}
	}

	if ((nFlags & MK_MBUTTON) && GetIEditor()->GetGameEngine()->GetSimulationMode())
	{
		// Get control key status.
		bool bAltClick = CheckVirtualKey(VK_MENU);
		bool bCtrlClick = CheckVirtualKey(VK_CONTROL);
		bool bShiftClick = CheckVirtualKey(VK_SHIFT);
		if (bCtrlClick)
		{
			// In simulation mode awake objects under the cursor when Ctrl+MButton pressed.
			//AwakeObjectAtPoint(view,point);
		}
	}

	UpdateStatusText();

	Vec3 p;
	if (CalculateHeightmapPointFromDisplayPoint(m_cCurrentMousePos ,p))
	{
		AABB aabb;
		if (CVisualBudgetSystem::GetVisualBudgetSystem().GetSectorBoundingBoxFromPoint(p,aabb))
		{
			if (m_boAABBSelectionStart)
			{
				if (m_boAABBDragSelection)
					CVisualBudgetSystem::GetVisualBudgetSystem().AddDetailedAnalysisRegion(aabb);
			}
			CVisualBudgetSystem::GetVisualBudgetSystem().SetCurrentRegion(aabb);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetMode::RegisterTool( CRegistrationContext &rc )
{
	rc.pClassFactory->RegisterClass( new CVisualBudgetMode::CVisualBudgetModeClassDesc );
}

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetMode::UpdateStatusText()
{
	CString str;
	int nCount = GetIEditor()->GetSelection()->GetCount();
	if (nCount > 0)
	{
		str.Format( "%d Object(s) Selected",nCount );
	}
	else
		str.Format( "No Selection",nCount );
	SetStatusText( str );
}
//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetMode::CalculateHeightmapPointFromDisplayPoint(const CPoint& rstDisplayPoint,Vec3& rstHeightmapPoint)
{
	CViewport	*poViewport=GetIEditor()->GetActiveView();
	//CViewport	*poViewport=GetIEditor()->GetViewManager()->GetViewport("Perspective");
	if (poViewport)
	{
		Vec3 v = poViewport->ViewToWorld( rstDisplayPoint );
		if (!(v.x == 0 && v.y == 0 && v.z == 0))
		{
			Matrix34 tm = poViewport->GetViewTM();
			Vec3 p = tm.GetTranslation();
			float height = p.z - GetIEditor()->GetTerrainElevation(p.x,p.y);
			if (height < 1) height = 1;
			p.x = v.x;
			p.y = v.y;
			p.z = GetIEditor()->GetTerrainElevation( p.x,p.y );
			rstHeightmapPoint=p;

			return true;
		}
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
