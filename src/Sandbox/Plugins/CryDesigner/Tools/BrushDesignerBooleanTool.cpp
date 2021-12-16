#include "StdAfx.h"
#include "BrushDesignerBooleanTool.h"
#include "BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

namespace
{
	IBaseToolPanel* s_pBooleanToolPanel = NULL;
}

void CBrushDesignerBooleanTool::BeginEditParams()
{
	if( !s_pBooleanToolPanel )
		s_pBooleanToolPanel = CreateBoolaenToolPanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerBooleanTool::EndEditParams()
{
	if( s_pBooleanToolPanel )
	{
		s_pBooleanToolPanel->DestroyPanel();
		s_pBooleanToolPanel = NULL;
	}
}

void CBrushDesignerBooleanTool::Enter()
{
	__super::Enter();

	CSelectionGroup* pGroup = GetIEditor()->GetSelection();
	int nDesignerObjectCount = 0;
	int nOtherObjectCount = 0;
	for( int i = 0, iObjectCount(pGroup->GetCount()); i < iObjectCount; ++i )
	{
		CBaseObject* pObject = pGroup->GetObject(i);
		if( pObject->GetType() == OBJTYPE_SOLID )
			++nDesignerObjectCount;
		else
			++nOtherObjectCount;
	}

	if( nOtherObjectCount > 0 || nDesignerObjectCount < 2 )
	{
		if( nOtherObjectCount > 0 )
			AfxMessageBox("Only designer objects should be selected to use this tool",MB_OK);
		else if( nDesignerObjectCount < 2 )
			AfxMessageBox("At least two designer objects should be selected to use this tool",MB_OK);
		GetEditTool()->SetDesignerMode(BUtil::eDesigner_ObjectMode);
	}
}

void CBrushDesignerBooleanTool::BooleanOperation( BUtil::EBooleanOperationEnum booleanType )
{
	CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();

	std::vector< _smart_ptr<CDesignerBrushObject> > designerObjList;
	for( int i = 0, iSelectionCount(pSelection->GetCount()); i < iSelectionCount; ++i )
	{
		CBaseObject* pObj = pSelection->GetObject(iSelectionCount-i-1);
		if( !pObj->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
			continue;
		designerObjList.push_back((CDesignerBrushObject*)pObj);
	}

	if( designerObjList.size() < 2 )
	{
		AfxMessageBox( "More than 2 designer objects should be selected to be operated.", MB_OK );
		return;
	}

	CString undoMsg("Designer : Boolean.");

	if( booleanType == BUtil::eBOE_Union )	undoMsg += "Union";
	else if( booleanType == BUtil::eBOE_Intersection )	undoMsg += "Intersection";
	else if( booleanType == BUtil::eBOE_Difference )	undoMsg += "Difference";

	CUndo undo(undoMsg);
	designerObjList[0]->StoreUndo(undoMsg);
	designerObjList[0]->GetBrush()->ResetXForm(designerObjList[0],designerObjList[0]->GetDesigner());

	for( int i = 1, iDesignerObjCount(designerObjList.size()); i < iDesignerObjCount; ++i )
	{
		BrushVec3 offset = designerObjList[i]->GetWorldTM().GetTranslation() - designerObjList[0]->GetWorldTM().GetTranslation();
		Matrix34 targetTM(designerObjList[i]->GetWorldTM());
		targetTM.SetTranslation(offset);
		designerObjList[i]->StoreUndo(undoMsg);
		designerObjList[i]->GetDesigner()->Transform(targetTM);

		if( booleanType == BUtil::eBOE_Union )
			designerObjList[0]->GetDesigner()->Union(designerObjList[i]->GetDesigner());
		else if( booleanType == BUtil::eBOE_Difference )
			designerObjList[0]->GetDesigner()->Subtract(designerObjList[i]->GetDesigner());
		else if( booleanType == BUtil::eBOE_Intersection )
			designerObjList[0]->GetDesigner()->Intersect(designerObjList[i]->GetDesigner());

		GetIEditor()->DeleteObject(designerObjList[i]);
	}

	if( designerObjList[0]->GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		CBrushDesignerBaseTool::CreateMirroredRegions(designerObjList[0]->GetDesigner());

	designerObjList[0]->PivotToCenter();
	designerObjList[0]->GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
	designerObjList[0]->UpdateBrush();

	designerObjList[0]->SwitchToDesignerEditTool();
}