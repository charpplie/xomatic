#include "StdAfx.h"
#include "BrushCommonInterface.h"
#include "Objects/DesignerBrushObject.h"
#include "BrushDesigner.h"
#include "Objects/AreaSolidObject.h"
#include "Objects/ClipVolumeObject.h"
#include "BaseBrushCreator.h"

bool CBrushCommonInterface::UpdateStatObjWithoutBackFaces( CBaseObject* pObj )
{
	CBrushDesigner* pDesigner = NULL;
	CBaseBrush* pBrush = NULL;

	if( GetDesigner(pObj, pDesigner) && GetBrush(pObj, pBrush) )
	{
		bool bDisplayBackFace = pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_DisplayBackFace);
		if( bDisplayBackFace )
		{
			pDesigner->SetModeFlag(pDesigner->GetModeFlag()&(~CBrushDesigner::eDesignerMode_DisplayBackFace));
			pBrush->Update(pObj,pDesigner);
			pDesigner->SetModeFlag(pDesigner->GetModeFlag()|CBrushDesigner::eDesignerMode_DisplayBackFace);
		}
		return true;
	}
	return false;
}

bool CBrushCommonInterface::UpdateStatObj( CBaseObject* pObj )
{
	CBrushDesigner* pDesigner = NULL;
	CBaseBrush* pBrush = NULL;

	if( GetDesigner(pObj, pDesigner) && GetBrush(pObj, pBrush) )
	{
		pBrush->Update(pObj, pDesigner);
		return true;
	}
	return false;
}

bool CBrushCommonInterface::UpdateGameResource( CBaseObject* pObj )
{
	if( pObj == NULL )
		return false;

	if( pObj->IsKindOf(RUNTIME_CLASS(CAreaSolid)) )
	{
		CAreaSolid* pArea = (CAreaSolid*)pObj;
		pArea->UpdateGameResource();
		return true;
	}
	else if( pObj->IsKindOf(RUNTIME_CLASS(CClipVolumeObject)) )
	{
		CClipVolumeObject* pVolume = (CClipVolumeObject*)pObj;
		pVolume->UpdateGameResource();
		return true;
	}

	return false;
}

bool CBrushCommonInterface::GetIStatObj( CBaseObject* pObj, _smart_ptr<IStatObj>* pOutStatObj )
{
	if( pObj == NULL )
		return false;

	CBaseBrush* pBrush = NULL;
	if( GetBrush(pObj, pBrush ) )
	{
		pBrush->GetIStatObj(pOutStatObj);
		return true;
	}

	return false;
}

bool CBrushCommonInterface::GenerateGameFilename( CBaseObject* pObj, CString& outFileName )
{
	if( pObj == NULL )
		return false;

	if( pObj->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
	{
		CDesignerBrushObject* pDesigner = (CDesignerBrushObject*)pObj;
		outFileName = pDesigner->GenerateGameFilename();
		return true;
	}
	else if( pObj->IsKindOf(RUNTIME_CLASS(CAreaSolid)) )
	{
		CAreaSolid* pArea = (CAreaSolid*)pObj;
		outFileName = pArea->GenerateGameFilename();
		return true;
	}
	else if( pObj->IsKindOf(RUNTIME_CLASS(CClipVolumeObject)) )
	{
		CClipVolumeObject* pVolume = (CClipVolumeObject*)pObj;
		outFileName = pVolume->GenerateGameFilename();
		return true;
	}
	
	return false;
}

bool CBrushCommonInterface::GetBrushCreator( CBaseObject* pObj, CBaseBrushCreator*& pOutBrushCreator )
{
	if( pObj == NULL )
		return false;

	CBrushDesigner* pDesigner = NULL;
	CBaseBrush* pBrush = NULL;

	if( GetDesigner(pObj, pDesigner) && GetBrush(pObj, pBrush) )
	{
		pOutBrushCreator = new CBaseBrushCreator( pObj, pBrush, pDesigner );
		return true;
	}

	return false;
}

bool CBrushCommonInterface::GetRenderFlag( CBaseObject* pObj, int& outRenderFlag )
{
	if( pObj == NULL )
		return false;

	CBaseBrush* pBrush = NULL;
	if( GetBrush(pObj, pBrush) )
	{
		outRenderFlag = pBrush->GetRenderFlags();
		return true;
	}

	return false;
}

bool CBrushCommonInterface::GetBrush( CBaseObject* pObj, CBaseBrush*& pBrush )
{
	if( pObj == NULL )
		return false;

	pBrush = NULL;

	if( pObj->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
	{
		CDesignerBrushObject* pDesigner = (CDesignerBrushObject*)pObj;
		pBrush = pDesigner->GetBrush();
	}
	else if( pObj->IsKindOf( RUNTIME_CLASS(CAreaSolid)) )
	{
		CAreaSolid* pArea = (CAreaSolid*)pObj;
		pBrush = pArea->GetBrush();
	}
	else if( pObj->IsKindOf( RUNTIME_CLASS(CClipVolumeObject)) )
	{
		CClipVolumeObject* pVolume = (CClipVolumeObject*)pObj;
		pBrush = pVolume->GetBrush();
	}

	return pBrush ? true : false;
}

bool CBrushCommonInterface::GetDesigner( CBaseObject* pObj, CBrushDesigner*& pDesigner )
{
	if( pObj == NULL )
		return false;

	pDesigner = NULL;

	if( pObj->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
	{
		CDesignerBrushObject* pDesignerObj = (CDesignerBrushObject*)pObj;
		pDesigner = pDesignerObj->GetDesigner();
	}
	else if( pObj->IsKindOf( RUNTIME_CLASS(CAreaSolid)) )
	{
		CAreaSolid* pArea = (CAreaSolid*)pObj;
		pDesigner = pArea->GetDesigner();
	}
	else if( pObj->IsKindOf( RUNTIME_CLASS(CClipVolumeObject)) )
	{
		CClipVolumeObject* pVolume = (CClipVolumeObject*)pObj;
		pDesigner = pVolume->GetDesigner();
	}

	return pDesigner != NULL;
}