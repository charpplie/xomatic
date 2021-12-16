#include "StdAfx.h"
#include "BrushDesignerCloneTool.h"
#include "SurfaceInfoPicker.h"
#include "Prefabs/PrefabItem.h"
#include "Prefabs/PrefabManager.h"
#include "Objects/PrefabObject.h"
#include "ViewManager.h"
#include "BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

namespace 
{
	ICloneToolPanel* s_pCloneToolPanel = NULL;
}

void CBrushDesignerCloneTool::BeginEditParams()
{
	if( !s_pCloneToolPanel )
		s_pCloneToolPanel = CreateCloneToolPanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerCloneTool::EndEditParams()
{
	if( s_pCloneToolPanel )
	{
		s_pCloneToolPanel->DestroyPanel();
		s_pCloneToolPanel = NULL;
	}
}

void CBrushDesignerCloneTool::Enter()
{
	__super::Enter();

	m_vStartPos = Vec3(0,0,0);
	m_SelectedObject = (CDesignerBrushObject*)GetBaseObject();

	AABB boundbox;
	m_SelectedObject->GetBoundBox(boundbox);

	m_vStartPos = GetCenterBottom(boundbox);
	m_vPickedPos = m_vStartPos;

	m_fRadius = 0.1f;
	s_pCloneToolPanel->GetNumOfClone();
	s_pCloneToolPanel->GetPlacementType();

	if( !m_bSuspendedUndo )
	{	
		GetIEditor()->SuspendUndo();
		m_bSuspendedUndo = true;
	}

	if( GetDesigner()->IsEmpty(0) )
		GetEditTool()->SetDesignerMode(BUtil::eDesigner_Box);
}

void CBrushDesignerCloneTool::Leave()
{
	__super::Leave();
	DeleteClones();	
	if( m_bSuspendedUndo )
	{
		GetIEditor()->ResumeUndo();
		FreezeClones();	
		m_bSuspendedUndo = false;
	}	
}

void CBrushDesignerCloneTool::DeleteClones()
{
	for( int i = 0, iCount(m_ClonedObjects.size()); i < iCount; ++i )
		GetIEditor()->GetObjectManager()->DeleteObject(m_ClonedObjects[i]);
}

void CBrushDesignerCloneTool::OnLButtonDown( CViewport *view, UINT nFlags, CPoint point )
{
	UpdateCloneList();
	m_vPickedPos = m_vStartPos;
	m_Plane = BrushPlane( m_vStartPos, m_vStartPos+BrushVec3(0,1,0), m_vStartPos+BrushVec3(1,0,0), kDesignerEpsilon );
}

void CBrushDesignerCloneTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch(event)
	{
	case eNotify_OnBeginGameMode:
	case eNotify_OnBeginSceneSave:
		DeleteClones();
		m_ClonedObjects.clear();
		break;
	}
}

void CBrushDesignerCloneTool::OnMouseMove( CViewport *view, UINT nFlags, CPoint point )
{
	if( nFlags & MK_LBUTTON )
	{
		if( m_ArrangeType == BUtil::eArrangeType_Array )
		{
			CSurfaceInfoPicker picker;
			SRayHitInfo hitInfo;
			int nCloneCount = m_ClonedObjects.size();
			DESIGNER_ASSERT(nCloneCount > 0);
			if( nCloneCount > 0 )
			{
				CSurfaceInfoPicker::CExcludedObjects excludedObjList;
				excludedObjList.Add(m_SelectedObject);
				for( int i = 0; i < nCloneCount; ++i )
					excludedObjList.Add(m_ClonedObjects[i]);

				if( picker.Pick( point, hitInfo, &excludedObjList ) )
				{
					m_vPickedPos = hitInfo.vHitPos;
					UpdateClonePositions();
				}
			}
		}
		else if( m_ArrangeType == BUtil::eArrangeType_Circle )
		{
			Vec3 vWorldRaySrc;
			Vec3 vWorldRayDir;
			GetIEditor()->GetActiveView()->ViewToWorldRay( point, vWorldRaySrc, vWorldRayDir );
			BrushVec3 vOut;
			if( m_Plane.HitTest( vWorldRaySrc, vWorldRaySrc+vWorldRayDir, kDesignerEpsilon, NULL, &vOut ) )
			{
				m_vPickedPos = ToVec3(vOut);
				UpdateClonePositions();
			}
		}
	}
}

void CBrushDesignerCloneTool::Display( struct DisplayContext& dc )
{
	dc.SetColor(ColorB(100,200,100,255));

	Matrix34 tm = dc.GetMatrix();
	dc.PopMatrix();

	if( m_ArrangeType == BUtil::eArrangeType_Circle )
	{
		dc.DrawCircle(m_vStartPos, m_fRadius);
		dc.DrawLine(m_vStartPos,Vec3(m_vPickedPos.x,m_vPickedPos.y,m_vStartPos.z));
	}
	else
	{
		dc.DrawLine(m_vStartPos,m_vPickedPos);
	}

	dc.PushMatrix(tm);
}

void CBrushDesignerCloneTool::SetPivotToObject( CBaseObject* pObj, const Vec3& pos )
{
	AABB localBB;
	pObj->GetLocalBounds(localBB);
	Vec3 vLocalCenterBottom = GetCenterBottom(localBB);
	pObj->SetPos(pos-vLocalCenterBottom);
}

Vec3 CBrushDesignerCloneTool::GetCenterBottom( const AABB& aabb ) const
{
	Vec3 vCenterBottom;
	vCenterBottom.x = (aabb.min.x+aabb.max.x)*0.5f;
	vCenterBottom.y = (aabb.min.y+aabb.max.y)*0.5f;
	vCenterBottom.z = aabb.min.z;
	return vCenterBottom;
}

void CBrushDesignerCloneTool::UpdateCloneList()
{
	if( !m_SelectedObject || !m_SelectedObject->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject))  )
		return;

	int nRealNumberOfClone = s_pCloneToolPanel->GetNumOfClone()-1;
	if( m_ClonedObjects.size() == nRealNumberOfClone )
		return;

	for( int i = 0, iCount(m_ClonedObjects.size()); i < iCount; ++i )
		GetIEditor()->GetObjectManager()->DeleteObject(m_ClonedObjects[i]);	

	m_ClonedObjects.clear();
	m_ClonedObjects.reserve(nRealNumberOfClone);

	for( int i = 0; i < nRealNumberOfClone; ++i )
	{
		CBaseObject* pClonedObj = GetIEditor()->GetObjectManager()->CloneObject(m_SelectedObject);
		m_ClonedObjects.push_back((CDesignerBrushObject*)pClonedObj);
	}

	GetIEditor()->GetViewManager()->UpdateViews();
}

void CBrushDesignerCloneTool::UpdateClonePositionsAlongLine()
{
	int nCloneCount = m_ClonedObjects.size();
	DESIGNER_ASSERT(nCloneCount > 0);
	if( nCloneCount <= 0 )
		return;
	Vec3 vDelta = s_pCloneToolPanel->GetPlacementType() == BUtil::ePlacementType_Divide ? (m_vPickedPos-m_vStartPos)/nCloneCount : (m_vPickedPos-m_vStartPos);
	for( int i = 1; i <= nCloneCount; ++i )
		SetPivotToObject(m_ClonedObjects[i-1],m_vStartPos+vDelta*i);
	SetPivotToObject(m_SelectedObject,m_vStartPos);
}

void CBrushDesignerCloneTool::UpdateClonePositionsAlongCircle()
{
	int nCloneCount = m_ClonedObjects.size();
	DESIGNER_ASSERT(nCloneCount>0);
	if( nCloneCount <= 0 )
		return;
	m_fRadius = (Vec2(m_vStartPos)-Vec2(m_vPickedPos)).GetLength();
	if( m_fRadius < 0.1f )
		m_fRadius = 0.1f;

	BrushVec2 vCenterOnPlane = m_Plane.W2P(ToBrushVec3(m_vStartPos));
	BrushVec2 vPickedPosOnPlane = m_Plane.W2P(ToBrushVec3(m_vPickedPos));
	float fAngle = BUtil::ComputeAnglePointedByPos( vCenterOnPlane, vPickedPosOnPlane );

	int nCloneCountInPanel = s_pCloneToolPanel->GetNumOfClone();
	std::vector<BrushVec2> vertices2D;
	BUtil::MakeSectorOfCircle( m_fRadius, m_Plane.W2P(ToBrushVec3(m_vStartPos)), fAngle, BUtil::PI2, nCloneCountInPanel+1, vertices2D );

	for( int i = 0; i < nCloneCountInPanel; ++i )
	{
		if( i == 0 )
			SetPivotToObject(m_SelectedObject,ToVec3(m_Plane.P2W(vertices2D[i])));
		else
			SetPivotToObject(m_ClonedObjects[i-1],ToVec3(m_Plane.P2W(vertices2D[i])));
	}
}

void CBrushDesignerCloneTool::FreezeClones()
{
	if( !m_SelectedObject || m_ClonedObjects.empty() )
		return;

	CPrefabManager *pPrefabManager = GetIEditor()->GetPrefabManager();
	if( !pPrefabManager )
		return;

	IDataBaseLibrary* pLibrary = pPrefabManager->FindLibrary("Level");
	if( !pLibrary )
		return;

	CUndo undo("Clone Designer Object");

	CSelectionGroup selectionGroup;
	selectionGroup.AddObject(m_SelectedObject);

	CPrefabItem* pPrefabItem = (CPrefabItem*)GetIEditor()->GetPrefabManager()->CreateItem(pLibrary);
	pPrefabItem->MakeFromSelection(selectionGroup);

	std::vector< _smart_ptr<CPrefabObject> > objectsInGroup;

	_smart_ptr<CBaseObject> pPrevObject = NULL;
	_smart_ptr<CBaseObject> pCurrObject = NULL;

	CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
	if( pSelection && pSelection->GetCount() == 1 && pSelection->GetObject(0)->GetChildCount() == 1 )
	{
		CBaseObject* pSelect = pSelection->GetObject(0);
		if( pSelect->IsKindOf(RUNTIME_CLASS(CPrefabObject)) )
		{
			((CPrefabObject*)pSelect)->Open();
			CDesignerBrushObject* pChild = (CDesignerBrushObject*)pSelect->GetChild(0);
			GetEditTool()->SetBaseObject(pChild);
			objectsInGroup.push_back((CPrefabObject*)pSelect);
			pPrevObject = pSelect;
			pCurrObject = pChild;
		}
		else
		{
			return;
		}
	}

	for( int i = 0, iCloneCount(m_ClonedObjects.size()); i < iCloneCount; ++i )
	{
		CPrefabObject* pDesignerPrefabObj = (CPrefabObject*)GetIEditor()->GetObjectManager()->NewObject(PREFAB_OBJECT_CLASS_NAME);
		pDesignerPrefabObj->SetPrefab(pPrefabItem,true);

		const Vec3 vChildLocalPos = pDesignerPrefabObj->GetChildCount() >= 1 ? pDesignerPrefabObj->GetChild(0)->GetPos() : Vec3(0,0,0);
		pDesignerPrefabObj->SetWorldPos(m_ClonedObjects[i]->GetWorldPos()-vChildLocalPos);
		pDesignerPrefabObj->Open();

		objectsInGroup.push_back(pDesignerPrefabObj);
		GetIEditor()->DeleteObject(m_ClonedObjects[i]);
	}

	CGroup* pGroup = (CGroup*)GetIEditor()->GetObjectManager()->NewObject("Group");
	pGroup->Open();
	
	pGroup->SetWorldPos(Vec3(0,0,0));
	for( int i = 0, iObjectCount(objectsInGroup.size()); i < iObjectCount; ++i )
		pGroup->AddMember(objectsInGroup[i]);

	AABB aabb;
	pGroup->GetBoundBox(aabb);
	pGroup->UpdatePivot(Vec3((aabb.min.x+aabb.max.x)*0.5f,(aabb.min.y+aabb.max.y)*0.5f,aabb.min.z));

	if( pPrevObject )
		GetIEditor()->GetObjectManager()->UnselectObject(pPrevObject);
	if( pCurrObject )
		GetIEditor()->GetObjectManager()->SelectObject(pCurrObject);
}