#include "StdAfx.h"
#include "BrushDesignerPivot2BottomTool.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerEditTool.h"

void CBrushDesignerPivot2BottomTool::Enter()
{
	CSelectionGroup* pSelectionGroup = GetIEditor()->GetSelection();
	CUndo undo("Designer : Pivot Tool"); 

	for( int i = 0, iCount(pSelectionGroup->GetCount()); i < iCount; ++i )
	{
		CBaseObject* pBaseObject = pSelectionGroup->GetObject(i);
		if( !pBaseObject->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
			continue;

		CDesignerBrushObject* pDesignerObj = (CDesignerBrushObject*)pBaseObject;
		CBrushDesigner* pDesigner = pDesignerObj->GetDesigner();
		CBaseBrush* pBrush = pDesignerObj->GetBrush();
		if( !pDesigner || !pBrush )
			continue;

		if( !pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		{		
			pDesigner->RecordUndo("Designer : Pivot",GetBaseObject());

			pBrush->PivotToCenter(pBaseObject,pDesigner);
			pBrush->Update(pBaseObject,pDesigner);

			pBaseObject->UpdateGroup();
			
			if( pBaseObject == GetBaseObject() )
				UpdateGameResource(GetBaseObject());
		}
		else if( iCount == 1 )
		{
			AfxMessageBox("This tool can't be used in Mirror mode",MB_OK);
		}
	}

	GetEditTool()->SetDesignerMode(BUtil::eDesigner_ObjectMode);
}