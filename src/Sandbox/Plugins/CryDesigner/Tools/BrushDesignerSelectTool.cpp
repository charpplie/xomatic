#include "StdAfx.h"
#include "BrushDesignerSelectTool.h"
#include "Viewport.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerUndo.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerRemoveTool.h"
#include "IBaseToolPanel.h"
#include "Core/BrushDesignerPolygonDecomposer.h"

void CBrushDesignerSelectTool::Enter()
{
	__super::Enter();
	if( GetDesigner() )
		GetDesigner()->ResetDB(BUtil::eDBRF_ALL);

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->RemoveInvalidElements();

	UpdateSelectionMeshFromSelectedElementList(GetMainContext());

	if( GetIEditor()->GetEditMode() == eEditModeRotateCircle )
	{
		GetIEditor()->SetEditMode(eEditModeRotate);
		UpdateTMManipulatorBasedOnElements(pSelected);
	}
}

void CBrushDesignerSelectTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	BrushVec3 localRaySrc, localRayDir;
	BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir );
	int nRegionIndex(0);

	GetIEditor()->BeginUndo();
	GetEditTool()->StoreSelectionUndo();

	bool bOnlyIncludeCube = GetKeyState(VK_SPACE) & (1<<15);
	CBrushDesignerElementManager pickedElements;
	if( !pickedElements.Pick(GetBaseObject(),GetDesigner(),view,point,m_nPickFlag,bOnlyIncludeCube,&m_PickedPosAsLMBDown) )
		GetIEditor()->ShowTransformManipulator(false);

	bool bPicked = pickedElements.IsEmpty() ? false : true;
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	if( nFlags & MK_CONTROL )
	{
		m_InitialSelectionElementsInRectangleSel = *pSelected;

		if( !bPicked )
		{
			m_SelectionType = eST_RectangleSelection;
			view->SetSelectionRectangle(point,point);
			m_MouseDownPos = point;
		}
		else
		{
			m_SelectionType = eST_NormalSelection;
			if( !pSelected->Erase(pickedElements) )
				pSelected->Add(pickedElements);
		}
	}
	else 
	{
		pSelected->Clear();
		if( !bPicked )
		{
			m_InitialSelectionElementsInRectangleSel.Clear();	
			m_SelectionType = eST_RectangleSelection;
			view->SetSelectionRectangle(point,point);
		}
		else
		{
			m_SelectionType = eST_LikelyToMoveSelection;
		}
		m_MouseDownPos = point;
		pSelected->Add(pickedElements);
	}

	UpdateTMManipulatorBasedOnElements(GetEditTool()->GetSelectedElements());
	UpdateSelectionMeshFromSelectedElementList(GetMainContext());
	ResetDesignerRejectedEdgeList(GetMainContext());
}

void CBrushDesignerSelectTool::OnLButtonUp( CViewport *view,UINT nFlags,CPoint point )
{
	GetIEditor()->AcceptUndo("Designer : Selection");

	if( m_SelectionType == eST_RectangleSelection )
	{
		view->SetSelectionRectangle(CPoint(0,0),CPoint(0,0));
		UpdateTMManipulatorBasedOnElements(GetEditTool()->GetSelectedElements());
	}
	m_SelectionType = eST_Nothing;
}

void CBrushDesignerSelectTool::UpdateSelectionMeshFromSelectedElementList( BUtil::SMainContext& mc )
{
	if( !mc.pDesigner || !mc.pBrush )
		return;

	std::vector<CBrushRegion::RegionPtr> regionlist;

	CBrushDesignerElementManager* pSelectedElement = GetEditTool()->GetSelectedElements();
	for( int k = 0, iElementSize(pSelectedElement->GetSize()); k < iElementSize; ++k )
	{
		if( pSelectedElement->Get(k).IsFace() )
			regionlist.push_back(pSelectedElement->Get(k).m_pRegion);
	}

	if( m_pSelectionMesh == NULL )
		m_pSelectionMesh = new CBrushRegionMesh;

	int renderFlag = mc.pBrush->GetRenderFlags();
	int viewDist = mc.pBrush->GetViewDistRatio();
	uint32 minSpec = mc.pObject->GetMinSpec();
	uint32 materialLayerMask = mc.pObject->GetMaterialLayersMask();
	BrushMatrix34 worldTM = mc.pObject->GetWorldTM();

	m_pSelectionMesh->SetRegions(regionlist, false, worldTM, renderFlag, viewDist, minSpec, materialLayerMask );
}

void CBrushDesignerSelectTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	bool bOnlyIncludeCube = GetKeyState(VK_SPACE) & (1<<15);
	CBrushDesignerElementManager pickedElements;
	bool bPicked = pickedElements.Pick(GetBaseObject(), GetDesigner(), view, point, m_nPickFlag, bOnlyIncludeCube, NULL);

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	if( !m_bHitGizmo )
		UpdateCursor(view,bPicked);

	if( m_SelectionType == eST_RectangleSelection )
	{
		CRect rc(m_MouseDownPos, point);
		view->SetSelectionRectangle(rc.TopLeft(),rc.BottomRight());	
		if( std::abs(rc.Width()) < 2 || std::abs(rc.Height()) < 2 )
			return;		

		GetDesigner()->ClearExcludedEdgesInDrawing();
		*pSelected = m_InitialSelectionElementsInRectangleSel;

		CRect selectionRect = view->GetSelectionRectangle();
		CBrushRegion::RegionPtr pRectRegion = CBrushRegion::MakeRegionFromRectangle(selectionRect);

		if( selectionRect.top != selectionRect.bottom && selectionRect.left != selectionRect.right )
		{
			CBrushDesignerElementManager selectionList;

			if( m_nPickFlag & BUtil::ePF_Vertex )
			{
				CBrushDesignerDB::QueryResult qResult;
				GetDesigner()->GetDB()->QueryAsRectangle(view,GetWorldTM(),selectionRect,qResult);
				for( int i = 0, iCount(qResult.size()); i < iCount; ++i )
					selectionList.Add(SDesignerElement(GetBaseObject(),qResult[i].m_Pos));
			}

			if( m_nPickFlag & BUtil::ePF_Edge )
			{
				std::vector< std::pair<BrushEdge3D,BrushVec3> > edges;
				GetDesigner()->QueryIntersectionEdgesWith2DRect(view,GetWorldTM(),pRectRegion,false,edges);
				for( int i = 0, iCount(edges.size()); i < iCount; ++i )
				{
					selectionList.Add(SDesignerElement(GetBaseObject(),edges[i].first));
					GetDesigner()->AddExcludedEdgeInDrawing(edges[i].first);
				}
			}

			if( m_nPickFlag & BUtil::ePF_Face )
			{
				std::vector<CBrushRegion::RegionPtr> regionList;
				if( !bOnlyIncludeCube )
					GetDesigner()->QueryIntersectionRegionsWith2DRect(view,GetWorldTM(),pRectRegion,false,regionList);

				if( gSettings.bDesignerHighlightElements )
				{
					BrushVec3 vBoxSize = BUtil::GetElementBoxSize(view,view->GetType()!=ET_ViewportCamera,GetWorldTM().GetTranslation());

					for( int i = 0, iRegionCount(GetDesigner()->GetRegionSize()); i < iRegionCount; ++i )
					{
						CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(i);
						BrushVec3 vRepresentativePos = pRegion->GetRepresentativePosition();
						AABB aabb(ToVec3(vRepresentativePos-vBoxSize),ToVec3(vRepresentativePos+vBoxSize));
						CPoint center2D = view->WorldToView(GetWorldTM().TransformPoint(aabb.GetCenter()));
						if( selectionRect.PtInRect(center2D) && !pRegion->CheckFlags(CBrushRegion::eRF_Hidden) )
							regionList.push_back(pRegion);
					}
				}

				for( int i = 0, iRegionCount(regionList.size()); i < iRegionCount; ++i )
					selectionList.Add(SDesignerElement(GetBaseObject(),regionList[i]));
			}

			pSelected->Add(selectionList);
		}

		UpdateSelectionMeshFromSelectedElementList(GetMainContext());
	}
	else if( (nFlags&MK_CONTROL) && (nFlags&MK_LBUTTON) )
	{
		if( bPicked )
		{
			pSelected->Add(pickedElements);
			UpdateSelectionMeshFromSelectedElementList(GetMainContext());
			ResetDesignerRejectedEdgeList(GetMainContext());
		}
	}
}

bool CBrushDesignerSelectTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( nChar == VK_ESCAPE )
	{
		if( !pSelected->IsEmpty() )
		{
			GetIEditor()->ShowTransformManipulator(false);
			pSelected->Clear();
			m_pSelectionMesh = NULL;
			ResetDesignerRejectedEdgeList(GetMainContext());
		}
		else
		{
			GetDesigner()->ClearExcludedEdgesInDrawing();			
			GetEditTool()->SetDesignerMode(BUtil::eDesigner_ObjectMode);
		}
	}
	else if( nChar == VK_DELETE )
	{
		GetEditTool()->SetDesignerMode(BUtil::eDesigner_Remove);
	}
#ifdef DEBUG
	else if( nChar == VK_RETURN )
	{
		IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
		CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();		
		int nCount = 0;
		for( int i = 0, iCount(pSelected->GetSize()); i < iCount; ++i )
		{
			if( (*pSelected)[i].m_pRegion )
			{
				CString buff;
				buff.Format("R #%d", ++nCount);
				dlg->AddRegion((*pSelected)[i].m_pRegion.get(),buff);
			}
		}
		dlg->Open();

		for( int i = 0, iCount(pSelected->GetSize()); i < iCount; ++i )
		{
			if( !(*pSelected)[i].m_pRegion )
				continue;

			std::vector<BrushVec3> vertexList;
			std::vector<BrushVec3> normalList;
			std::vector<SMeshFace> faceList;
			CBrushDesignerPolygonDecomposer decomposer;
			decomposer.TriangulateRegion((*pSelected)[i].m_pRegion, vertexList, normalList, faceList);
		}
	}
#endif
	return true;
}

void CBrushDesignerSelectTool::UpdateCursor( CViewport* view, bool bPickingElements )
{
	if( !bPickingElements )
	{
		view->SetCurrentCursor(STD_CURSOR_DEFAULT,"");
		return;
	}
	else if( GetIEditor()->GetEditMode() == eEditModeMove )
		view->SetCurrentCursor(STD_CURSOR_MOVE,"");
	else if( GetIEditor()->GetEditMode() == eEditModeRotateCircle || GetIEditor()->GetEditMode() == eEditModeRotate )
		view->SetCurrentCursor(STD_CURSOR_ROTATE,"");
	else if( GetIEditor()->GetEditMode() == eEditModeScale )
		view->SetCurrentCursor(STD_CURSOR_SCALE,"");
}

void CBrushDesignerSelectTool::Display( DisplayContext &dc )
{
	__super::Display(dc);

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	if( gSettings.bDesignerHighlightElements )
		pSelected->DisplayHighlightElements(GetBaseObject(),GetDesigner(),dc,m_nPickFlag);

	pSelected->Display(GetBaseObject(),dc);
}

void CBrushDesignerSelectTool::ResetDesignerRejectedEdgeList( BUtil::SMainContext& mc )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	mc.pDesigner->ClearExcludedEdgesInDrawing();

	for( int i = 0, iElementSize(pSelected->GetSize()); i < iElementSize; ++i )
	{
		const SDesignerElement& element = pSelected->Get(i);
		if( element.IsEdge() )
		{
			DESIGNER_ASSERT(element.m_Vertices.size() == 2);
			mc.pDesigner->AddExcludedEdgeInDrawing(BrushEdge3D(element.m_Vertices[0],element.m_Vertices[1]));
		}
		else if( element.IsFace() && element.m_pRegion->IsOpen() )
		{
			for( int a = 0, iEdgeCount(element.m_pRegion->GetEdgeSize()); a < iEdgeCount; ++a )
			{
				BrushEdge3D e = element.m_pRegion->GetEdge(a);
				mc.pDesigner->AddExcludedEdgeInDrawing(e);
			}
		}
	}
}

void CBrushDesignerSelectTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	__super::OnEditorNotifyEvent(event);

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	switch(event)
	{
	case eNotify_OnEndUndoRedo:
		for( int i = 0, iElementSize(pSelected->GetSize()); i < iElementSize; ++i )
		{
			SDesignerElement element = pSelected->Get(i);

			if( !element.IsFace() )
				continue;

			CBrushRegion::RegionPtr pEquivalentRegion = GetDesigner()->QueryEquivalentRegion(element.m_pRegion);
			if( pEquivalentRegion )
			{
				element.m_pRegion = pEquivalentRegion;
				pSelected->Set(i,element);
			}
		}
		UpdateSelectionMeshFromSelectedElementList(GetMainContext());
		break;
	}
}

CBrushDesignerSelectTool::OrganizedQueryResults CBrushDesignerSelectTool::CreateOrganizedResultsAroundRegionFromQueryResults( const CBrushDesignerDB::QueryResult& queryResult )
{
	CBrushDesignerSelectTool::OrganizedQueryResults organizedQueryResults;
	int iQueryResultSize(queryResult.size());
	for( int i = 0; i < iQueryResultSize; ++i )
	{
		const CBrushDesignerDB::Vertex& v = queryResult[i];
		for( int k = 0, iMarkListSize(v.m_MarkList.size()); k < iMarkListSize; ++k )
		{
			CBrushRegion::RegionPtr pRegion = v.m_MarkList[k].m_pRegion;
			DESIGNER_ASSERT(pRegion);
			if( !pRegion )
				continue;

			if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) )
				continue;

			organizedQueryResults[v.m_MarkList[k].m_pRegion].push_back(QueryInput(i,k));
		}
	}
	return organizedQueryResults;
}

void CBrushDesignerSelectTool::SelectAllElements()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	bool bSelectedElementsExist = !pSelected->IsEmpty();
	pSelected->Clear();

	if( !bSelectedElementsExist )
	{
		CBrushDesignerElementManager elements;
		for( int k = 0, iRegionCount(GetDesigner()->GetRegionSize()); k < iRegionCount; ++k )
		{
			CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(k);
			SDesignerElement element;
			for( int i = 0, iVertexCount(pRegion->GetVertexListSize()); i < iVertexCount; ++i )
				element.m_Vertices.push_back(pRegion->GetVertex(i));
			element.m_pRegion = pRegion;
			element.m_pObject = GetBaseObject();
			elements.Add(element);
		}
		pSelected->Add(elements);
	}

	UpdateSelectionMeshFromSelectedElementList(GetMainContext());
	ResetDesignerRejectedEdgeList(GetMainContext());
	UpdateTMManipulatorBasedOnElements(pSelected);
}

CString CBrushDesignerSelectTool::GetStatusText() const
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->IsEmpty() )
		return CBrushDesignerBaseTool::GetStatusText();

	CString str = pSelected->GetElementsInfoText();
	str += " Selected.";

	return str;
}