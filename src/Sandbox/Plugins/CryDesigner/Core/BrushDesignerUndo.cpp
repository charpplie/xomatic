#include "StdAfx.h"
#include "BrushDesignerUndo.h"
#include "BrushRegion.h"
#include "BrushCommonInterface.h"
#include "Objects/AreaSolidObject.h"
#include "Objects/DesignerBrushObject.h"
#include "Tools/BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

bool CUndoDesigner::IsAKindOfDesignerTool( CEditTool * pEditTool )
{
	return pEditTool && pEditTool->IsKindOf(RUNTIME_CLASS(CBrushDesignerEditTool));
}

CBrushDesignerEditTool* CUndoDesigner::GetEditTool()
{
	CEditTool* pEditTool = GetIEditor()->GetEditTool();
	if( IsAKindOfDesignerTool(pEditTool) )
		return (CBrushDesignerEditTool*)pEditTool;
	return NULL;
}

CUndoDesigner::CUndoDesigner( CBaseObject* pObj, const CBrushDesigner* pDesigner, const char *undoDescription ) :
m_ObjGUID(pObj->GetId())
{
	DESIGNER_ASSERT(pDesigner);
	if( pDesigner )
	{
		if (undoDescription)
			m_undoDescription = undoDescription;
		else 
			m_undoDescription = "CryDesignerUndo";

		StoreEditorTool();

		m_UndoWorldTM = pObj->GetWorldTM();
		m_undo = new CBrushDesigner(*pDesigner);
	}
}

void CUndoDesigner::StoreEditorTool()
{
	CEditTool* pEditTool = GetIEditor()->GetEditTool();
	if( IsAKindOfDesignerTool(pEditTool) )
		m_DesignerMode = ((CBrushDesignerEditTool*)pEditTool)->GetDesignerMode();
	else
		m_DesignerMode = BUtil::eDesigner_Max;
}

void CUndoDesigner::Undo( bool bUndo )
{
	CBrushDesigner* pDesigner(GetDesigner());
	if( !pDesigner )
		return;

	CBaseObject* pObj(GetBaseObject(m_ObjGUID));

	if( bUndo )
	{
		m_RedoWorldTM = pObj->GetWorldTM();
		m_redo = new CBrushDesigner(*pDesigner);
		RestoreEditTool(pDesigner);
	}

	if( m_undo )
	{
		*pDesigner = *m_undo;
		UpdateBrush();
		pObj->SetWorldTM(m_UndoWorldTM);
		pDesigner->ResetDB(BUtil::eDBRF_ALL);
		Sync();
		CBrushCommonInterface::UpdateGameResource(pObj);
		m_undo = NULL;
	}
}

void CUndoDesigner::Redo()
{
	CBrushDesigner* pDesigner(GetDesigner());
	DESIGNER_ASSERT(pDesigner);
	if( !pDesigner )
		return;

	CBaseObject* pObj(GetBaseObject(m_ObjGUID));

	m_UndoWorldTM = pObj->GetWorldTM();
	m_undo = new CBrushDesigner(*pDesigner);	

	RestoreEditTool(pDesigner);

	if( m_redo )
	{
		*pDesigner = *m_redo;
		UpdateBrush();
		CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(m_ObjGUID);
		DESIGNER_ASSERT(pObj);
		if( pObj )
		{
			pObj->SetWorldTM(m_RedoWorldTM);
			CBrushCommonInterface::UpdateGameResource(pObj);
		}
		pDesigner->ResetDB(BUtil::eDBRF_ALL);
		Sync();
		m_redo = NULL;
	}
}

CBaseObject* CUndoDesigner::GetBaseObject( const GUID& objGUID )
{
	CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(objGUID);
	if( pObj == NULL )
		return NULL;
	return pObj;
}

CBrushDesigner* CUndoDesigner::GetDesigner( const GUID& objGUID )
{
	CBaseObject* pObj = GetBaseObject(objGUID);
	if( pObj == NULL )
		return NULL;

	CBrushDesigner* pDesigner = NULL;
	CBrushCommonInterface::GetDesigner(pObj, pDesigner);

	return pDesigner;
}

void CUndoDesigner::Sync()
{
	CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(m_ObjGUID);
	DESIGNER_ASSERT(pObj);
	if( pObj == NULL )
		return;

	pObj->UpdateGroup();
}

void CUndoDesigner::RestoreEditTool( CBrushDesigner* pDesigner, REFGUID objGUID, BUtil::EDesignerMode designerMode )
{
	CBaseObject* pSelectedObj = GetBaseObject(objGUID);
	if( !pSelectedObj )
		return;

	bool bSelected(pSelectedObj->IsSelected());
	if( !bSelected )
	{
		GetIEditor()->GetObjectManager()->ClearSelection();
		GetIEditor()->GetObjectManager()->SelectObject(pSelectedObj);
	} 

	CEditTool* pEditTool = GetIEditor()->GetEditTool();
	if( pEditTool && IsAKindOfDesignerTool(pEditTool) )
	{
		if( designerMode == BUtil::eDesigner_Merge || 
			designerMode == BUtil::eDesigner_Weld ||
			designerMode == BUtil::eDesigner_Pivot ||
			designerMode == BUtil::eDesigner_Magnet )
		{
			designerMode = BUtil::eDesigner_ObjectMode;
		}

		CBrushDesignerEditTool* pDesignerEditTool = (CBrushDesignerEditTool*)pEditTool;
		pDesignerEditTool->SetBaseObject(pSelectedObj);

		if( pSelectedObj->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
		{
			if( !CDesignerBrushObject::GetMenuPanel() )
			{
				CDesignerBrushObject::CreateDesignerPanels((CDesignerBrushObject*)pSelectedObj,false);
				CDesignerBrushObject::GetMenuPanel()->SetEditTool(pDesignerEditTool);
			}
		}
		pDesignerEditTool->SetDesignerMode(designerMode);
	}
}

void CUndoDesigner::UpdateBrush()
{
	CBaseObject* pObject = GetBaseObject(m_ObjGUID);
	if( pObject == NULL )
		return;

	CBrushDesigner* pDesigner = GetDesigner();
	if( pDesigner == NULL )
		return;

	CBaseBrush* pBrush = NULL;
	if(CBrushCommonInterface::GetBrush(pObject, pBrush))
		pBrush->Update( pObject, pDesigner );
}

CUndoDesigneSelection::CUndoDesigneSelection( CBrushDesignerElementManager& selectionContext, CBaseObject* pObj, const char *undoDescription )
{
	SetObjGUID(pObj->GetId());	

	if( undoDescription )
		SetDescription(undoDescription);
	else
		SetDescription(selectionContext.GetElementsInfoText());

	CopyElements(selectionContext,m_SelectionContextForUndo);

	int nDesignerMode = 0;
	for( int i = 0, iSelectionCount(selectionContext.GetSize()); i < iSelectionCount; ++i )
	{
		if( selectionContext[i].IsVertex() )
			nDesignerMode |= BUtil::eDesigner_Select_Vertex;
		else if( selectionContext[i].IsEdge() )
			nDesignerMode |= BUtil::eDesigner_Select_Edge;
		else if( selectionContext[i].IsFace() )
			nDesignerMode |= BUtil::eDesigner_Select_Face;
	}
	SetDesignerMode((BUtil::EDesignerMode)nDesignerMode);
}

CBrushDesignerSelectTool* CUndoDesigneSelection::GetSelectToolHandler()
{
	CEditTool* pEditTool = GetIEditor()->GetEditTool();
	if( !IsAKindOfDesignerTool(pEditTool) )
		return NULL;

	CBrushDesignerEditTool* pDesignerEditTool = (CBrushDesignerEditTool*)pEditTool;
	BUtil::EDesignerMode dm = pDesignerEditTool->GetDesignerMode();
	if( !BUtil::IsSelectElementMode(dm) )
		return NULL;

	return (CBrushDesignerSelectTool*)(pDesignerEditTool->GetCurrentTool());
}

void CUndoDesigneSelection::Undo( bool bUndo )
{
	CBrushDesigner* pDesigner(GetDesigner());
	RestoreEditTool(pDesigner);

	CBrushDesignerSelectTool* pHandler = GetSelectToolHandler();
	if( pHandler == NULL )
		return;

	CBrushDesignerElementManager* pSelected = GetEditTool() ? GetEditTool()->GetSelectedElements() : NULL;
	if( bUndo )
	{		
		if( pSelected )
			CopyElements(*pSelected,m_SelectionContextForRedo);
	}

	pHandler->ClearRegionSelections();
	ReplaceRegionsWithExistingRegionsInDesigner(m_SelectionContextForUndo);
	if( pSelected )
		pSelected->Set(m_SelectionContextForUndo);
	pHandler->UpdateSelectionMeshFromSelectedElementList(pHandler->GetMainContext());
	pHandler->ResetDesignerRejectedEdgeList(pHandler->GetMainContext());
}

void CUndoDesigneSelection::Redo()
{
	CBrushDesigner* pDesigner(GetDesigner());
	RestoreEditTool(pDesigner);

	CBrushDesignerSelectTool* pHandler = GetSelectToolHandler();
	if( pHandler == NULL )
		return;

	CBrushDesignerElementManager* pSelected = GetEditTool() ? GetEditTool()->GetSelectedElements() : NULL;

	pHandler->ClearRegionSelections();
	ReplaceRegionsWithExistingRegionsInDesigner(m_SelectionContextForRedo);
	if( pSelected )
		pSelected->Set(m_SelectionContextForRedo);
	pHandler->UpdateSelectionMeshFromSelectedElementList(pHandler->GetMainContext());
	pHandler->ResetDesignerRejectedEdgeList(pHandler->GetMainContext());
}

void CUndoDesigneSelection::CopyElements( CBrushDesignerElementManager& sourceElements, CBrushDesignerElementManager& destElements )
{
	destElements = sourceElements;
	for( int i = 0, iElementSize(destElements.GetSize()); i < iElementSize; ++i )
	{
		if( !destElements[i].IsFace() )
			continue;
		destElements[i].m_pRegion = destElements[i].m_pRegion->Clone();
	}
}

void CUndoDesigneSelection::ReplaceRegionsWithExistingRegionsInDesigner( CBrushDesignerElementManager& elements )
{
	std::vector<SDesignerElement> removedElements;

	for( int i = 0, iCount(elements.GetSize()); i < iCount; ++i )
	{
		if( !elements[i].IsFace() )
			continue;

		if( elements[i].m_pRegion == NULL )
			continue;

		CBrushRegion::RegionPtr pEquivalentRegion = GetDesigner()->QueryEquivalentRegion(elements[i].m_pRegion);
		if( pEquivalentRegion == NULL )
		{
			removedElements.push_back(elements[i]);
			continue;
		}

		elements[i].m_pRegion = pEquivalentRegion;
	}

	for( int i = 0, iRemovedCount(removedElements.size()); i < iRemovedCount; ++i )
		elements.Erase(removedElements[i]);
}

void CUndoDesignerTextureMapping::Undo( bool bUndo )
{
	CBrushDesigner* pDesigner = CUndoDesigner::GetDesigner(m_ObjGUID);
	if( pDesigner == NULL )
		return;
	if( bUndo )
		SaveDesignerTexInfoContext(m_RedoContext);
	RestoreTexInfo(m_UndoContext);
	CUndoDesigner::RestoreEditTool(pDesigner,m_ObjGUID,BUtil::eDesigner_Mapping);
}

void CUndoDesignerTextureMapping::Redo()
{
	CBrushDesigner* pDesigner = CUndoDesigner::GetDesigner(m_ObjGUID);
	if( pDesigner == NULL )
		return;
	SaveDesignerTexInfoContext(m_UndoContext);
	RestoreTexInfo(m_RedoContext);
	CUndoDesigner::RestoreEditTool(pDesigner,m_ObjGUID,BUtil::eDesigner_Mapping);
}


void CUndoDesignerTextureMapping::RestoreTexInfo( const std::vector<SContextInfo>& contextList )
{
	CBrushDesigner* pDesigner = CUndoDesigner::GetDesigner(m_ObjGUID);
	if( pDesigner == NULL )
		return;

	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);

	for( int i = 0, iContextCount(contextList.size()); i < iContextCount; ++i )
	{
		const SContextInfo& context = contextList[i];
		CBrushRegion::RegionPtr pRegion = pDesigner->QueryRegion(context.m_RegionGUID);
		if( pRegion == NULL )
			continue;
		pRegion->SetTexInfo(context.m_TexInfo);
		pRegion->SetMaterialID(context.m_MatID);
	}

	CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(m_ObjGUID);
	if( pObj )
	{
		pObj->UpdateGroup();
		if( pObj->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
			((CDesignerBrushObject*)pObj)->UpdateBrush();
	}
}

void CUndoDesignerTextureMapping::SaveDesignerTexInfoContext( std::vector<SContextInfo>& contextList )
{
	CBrushDesigner* pDesigner(CUndoDesigner::GetDesigner(m_ObjGUID));
	if( !pDesigner )
		return;

	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);

	contextList.clear();

	for( int k = 0; k < BUtil::kMaxShelfCount; ++k )
	{
		pDesigner->SetShelf(k);
		for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
			if( pRegion == NULL )
				continue;
			SContextInfo context;
			context.m_RegionGUID = pRegion->GetGUID();
			context.m_MatID = pRegion->GetMaterialID();
			context.m_TexInfo = pRegion->GetTexInfo();
			contextList.push_back(context);
		}
	}
}
