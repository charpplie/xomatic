#include "StdAfx.h"
#include "BrushDesignerSeparateTool.h"
#include "BrushDesignerSelectTool.h"
#include "Core/BrushDesigner.h"
#include "Objects/DesignerBrushObject.h"
#include "BrushDesignerEditTool.h"

void CBrushDesignerSeparateTool::Enter()
{
	CUndo undo("Designer : Separate a Part");
	GetDesigner()->RecordUndo("Designer : Separate a Part",GetBaseObject());
	CDesignerBrushObject* pNewObj = Separate(GetMainContext());
	if( pNewObj )
	{
		GetEditTool()->SetBaseObject(pNewObj);
		GetIEditor()->ShowTransformManipulator(false);
	}
	GetEditTool()->SetDesignerMode(BUtil::eDesigner_ObjectMode);
}

CDesignerBrushObject* CBrushDesignerSeparateTool::Separate( BUtil::SMainContext& mc )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	CDesignerBrushObject* pNewObj = (CDesignerBrushObject*)GetIEditor()->NewObject("Designer","");
	DESIGNER_ASSERT(pNewObj);
	if( !pNewObj )
		return NULL;

	pNewObj->SetWorldTM(mc.pObject->GetWorldTM());

	_smart_ptr<CBrushDesigner> pNewDesigner = pNewObj->GetDesigner();
	pNewDesigner->SetModeFlag(mc.pDesigner->GetModeFlag());
	pNewDesigner->SetSubdivisionLevel(mc.pDesigner->GetSubdivisionLevel());

	DESIGNER_SHELF_RECONSTRUCTOR_POSTFIX(pNewDesigner,0);
	DESIGNER_SHELF_RECONSTRUCTOR_POSTFIX(mc.pDesigner,1);

	pNewDesigner->SetShelf(0);
	mc.pDesigner->SetShelf(0);	

	for( int i = 0, iElementCount(pSelected->GetSize()); i < iElementCount; ++i )
	{
		const SDesignerElement& elementInfo = (*pSelected)[i];
		if( !elementInfo.IsFace() )
			continue;

		DESIGNER_ASSERT(elementInfo.m_pRegion);
		if( !elementInfo.m_pRegion )
			continue;

		mc.pDesigner->RemoveRegion(mc.pDesigner->QueryEquivalentRegion(elementInfo.m_pRegion));
		pNewDesigner->AddRegion(elementInfo.m_pRegion->Clone(),CBrushDesigner::eOpType_Add);
	}

	mc.pBrush->Update(mc.pObject,mc.pDesigner);

	pNewObj->PivotToCenter();
	pNewObj->UpdateBrush();
	pNewObj->SetMaterial(mc.pObject->GetMaterial());

	GetIEditor()->SelectObject(pNewObj);
	GetIEditor()->GetObjectManager()->UnselectObject(mc.pObject);

	return pNewObj;
}