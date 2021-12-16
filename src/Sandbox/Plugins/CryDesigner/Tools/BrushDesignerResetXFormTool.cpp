#include "StdAfx.h"
#include "BrushDesignerResetXFormTool.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

namespace 
{
	IBaseToolPanel* g_pResetXFormToolPanel = NULL;
}

void CBrushDesignerResetXFormTool::BeginEditParams()
{
	if( !g_pResetXFormToolPanel )
		g_pResetXFormToolPanel = CreateResetXFormToolPanel(this);
}

void CBrushDesignerResetXFormTool::EndEditParams()
{
	if( g_pResetXFormToolPanel )
	{
		g_pResetXFormToolPanel->DestroyPanel();
		g_pResetXFormToolPanel = NULL;
	}
}

void CBrushDesignerResetXFormTool::FreezeXForm( int nResetFlag )
{
	FreezeXForm( GetDesigner(), GetBrush(), GetBaseObject(), nResetFlag );
	UpdateGameResource(GetBaseObject());
}

void CBrushDesignerResetXFormTool::FreezeXForm( CBrushDesigner* pDesigner, CBaseBrush* pBrush, CBaseObject* pObj, int nResetFlag )
{
	pDesigner->RecordUndo("Designer : Pivot",pObj);

	pBrush->ResetXForm(pObj,pDesigner,nResetFlag);
	pBrush->Update(pObj,pDesigner);

	pObj->UpdateGroup();
}