#include "StdAfx.h"
#include "BrushDesignerGlobalSettings.h"
#include "RenderViewport.h"
#include "TimeOfDayDialog.h"
#include "Tools/BrushDesignerEditTool.h"
#include "Objects/DesignerBrushObject.h"
#include "Objects/AreaSolidObject.h"
#include "Mission.h"
#include "CryEditDoc.h"
#include "Objects/EnvironmentProbeObject.h"
#include "MainFrm.h"

class CUndoExclusiveMode : public IUndoObject
{
public:
	CUndoExclusiveMode( const char *undoDescription = NULL )
	{
	}

	int GetSize() { return sizeof(*this); }
	const char* GetDescription() { return "Undo for Designer Exclusive Mode"; }

	void Undo( bool bUndo=true ) override
	{
	}

	void Redo() override
	{
	}
};

CBrushDesignerEditTool* GetDesignerEditTool()
{
	CEditTool* pEditTool = GetIEditor()->GetEditTool();
	if( pEditTool && pEditTool->IsKindOf(RUNTIME_CLASS(CBrushDesignerEditTool)) )
		return (CBrushDesignerEditTool*)pEditTool;
	return NULL;
}

CRenderViewport* GetRenderViewport()
{
	CViewport* pViewport = GetIEditor()->GetActiveView();
	if( pViewport->IsKindOf(RUNTIME_CLASS(CRenderViewport)) )
		return (CRenderViewport*)pViewport;
	return NULL;
}

void SDesignerEnvironmentInfo::Save()
{
}

void SDesignerEnvironmentInfo::Load()
{
}

void SDesignerEnvironmentInfo::CenterCameraForExclusiveMode()
{
	CSelectionGroup* pSelection = GetIEditor()->GetSelection();
	CRenderViewport* pView = GetRenderViewport();
	if( pSelection->GetCount() > 0 && pView )
	{
		AABB selBoundBox = pSelection->GetBounds();
		Vec3 vCenterPos = pSelection->GetCenter();
		vCenterPos -= pView->GetCamera().GetViewdir()*(selBoundBox.GetRadius()+10.0f);
		Matrix34 cameraTM = pView->GetViewTM();
		cameraTM.SetTranslation(vCenterPos);
		pView->SetViewTM(cameraTM);
	}
}

void SDesignerEnvironmentInfo::EnableExclusiveMode( bool bEnable )
{
	if( m_bEnableExclusiveMode == bEnable )
		return;

	if( bEnable )
	{
		CSelectionGroup* pGroup = GetIEditor()->GetSelection();
		if( pGroup->GetCount() == 0 )
			return;
		for( int i = 0, iObjectCount(pGroup->GetCount()); i < iObjectCount; ++i )
		{
			CBaseObject* pObject = pGroup->GetObject(i);
			if( !pObject->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
				return;
		}
	}

	GetIEditor()->SuspendUndo();

	m_bEnableExclusiveMode = bEnable;

	CRenderViewport* pView = GetRenderViewport();
	if( pView == NULL )
	{
		GetIEditor()->ResumeUndo();
		return;
	}

	if( m_bEnableExclusiveMode )
	{
		CBaseObject* pCameraObj = GetIEditor()->GetObjectManager()->FindObject(BUtil::BrushDesignerCameraName);
		if( pCameraObj )
			GetIEditor()->GetObjectManager()->DeleteObject(pCameraObj);

		char workingDirectory[_MAX_PATH] = {0,};
		GetCurrentDirectory(_MAX_PATH,workingDirectory);
		CString designerCameraPath = CString(workingDirectory) + "\\Editor\\Objects\\CryDesigner_Camera.grp";

		XmlNodeRef cameraNode = XmlHelpers::LoadXmlFromFile(designerCameraPath);
		if( cameraNode )
		{
			CObjectArchive cameraArchive(GetIEditor()->GetObjectManager(),cameraNode,true);
			GetIEditor()->GetObjectManager()->LoadObjects(cameraArchive, false);
			pCameraObj = GetIEditor()->GetObjectManager()->FindObject(BUtil::BrushDesignerCameraName);
		}

		if( !pCameraObj )
		{
			GetIEditor()->ResumeUndo();
			return;
		}

		pCameraObj->SetFlags(OBJFLAG_HIDE_HELPERS);
		DynArray< _smart_ptr<CBaseObject> > allLightsUnderCamera;
		pCameraObj->GetAllChildren(allLightsUnderCamera);
		for( int i = 0, iCount(allLightsUnderCamera.size()); i < iCount; ++i )
			allLightsUnderCamera[i]->SetFlags(OBJFLAG_HIDE_HELPERS);

		pView->EnableCameraObjectMove(true);
		pView->LockCameraMovement(false);

		m_OldCameraTM = pView->GetViewTM();
		pView->SetCameraObject(pCameraObj);
		pView->SetViewTM(m_OldCameraTM);

		m_OldObjectHideMask = gSettings.objectHideMask;
		gSettings.objectHideMask = 0;

		SetCVForExclusiveMode();
		//SetTimeOfDayForExclusiveMode();
		SetObjectsFlagForExclusiveMode();
		CenterCameraForExclusiveMode();
	}
	else if( pView->GetCameraObject() && pView->GetCameraObject()->GetName() == BUtil::BrushDesignerCameraName )
	{
		Matrix34 cameraTM = pView->GetCamera().GetMatrix();

		pView->SetCameraObject(NULL);
		pView->EnableCameraObjectMove(false);
		pView->LockCameraMovement(true);

		_smart_ptr<CBaseObject> pCameraObj = GetIEditor()->GetObjectManager()->FindObject(BUtil::BrushDesignerCameraName);
		if( pCameraObj )
		{
			DynArray< _smart_ptr<CBaseObject> > allLightsUnderCamera;
			pCameraObj->GetAllChildren(allLightsUnderCamera);
			for( int i = 0, iCount(allLightsUnderCamera.size()); i < iCount; ++i )
				GetIEditor()->GetObjectManager()->DeleteObject(allLightsUnderCamera[i]);
			GetIEditor()->GetObjectManager()->DeleteObject(pCameraObj);
			pView->SetViewTM(m_OldCameraTM);
		}

		gSettings.objectHideMask = m_OldObjectHideMask;
		m_OldObjectHideMask = 0;

		RestoreCV();
		//RestoreTimeOfDay();
		RestoreObjectsFlag();
	}

	GetIEditor()->ResumeUndo();
}

void SDesignerEnvironmentInfo::SetCVForExclusiveMode()
{
	IConsole* pConsole = GetIEditor()->GetSystem()->GetIConsole();
	if( !pConsole )
		return;

	ICVar* pDisplayInfo = pConsole->GetCVar("r_DisplayInfo");
	if( pDisplayInfo )
	{
		m_OldConsoleVars.r_DisplayInfo = pDisplayInfo->GetIVal();
		pDisplayInfo->Set(0);
	}

	ICVar* pHDRRendering = pConsole->GetCVar("r_HDRRendering");
	if( pHDRRendering )
	{
		m_OldConsoleVars.r_HDRRendering = pHDRRendering->GetIVal();
		pHDRRendering->Set(0);
	}

// 	ICVar* pPostProcessEffect = pConsole->GetCVar("r_PostProcessEffects");
// 	if( pPostProcessEffect )
// 	{
// 		m_OldConsoleVars.r_PostProcessEffects = pPostProcessEffect->GetIVal();
// 		pPostProcessEffect->Set(0);
// 	}

	ICVar* pRenderVegetation = pConsole->GetCVar("e_Vegetation");
	if( pRenderVegetation )
	{
		m_OldConsoleVars.e_Vegetation = pRenderVegetation->GetIVal();
		pRenderVegetation->Set(0);
	}

	ICVar* pWaterOcean = pConsole->GetCVar("e_WaterOcean");
	if( pWaterOcean )
	{
		m_OldConsoleVars.e_WaterOcean = pWaterOcean->GetIVal();
		pWaterOcean->Set(0);
	}

	ICVar* pWaterVolume = pConsole->GetCVar("e_WaterVolumes");
	if( pWaterVolume )
	{
		m_OldConsoleVars.e_WaterVolumes = pWaterVolume->GetIVal();
		pWaterVolume->Set(0);
	}

	ICVar* pTerrain = pConsole->GetCVar("e_Terrain");
	if( pTerrain )
	{
		m_OldConsoleVars.e_Terrain = pTerrain->GetIVal();
		pTerrain->Set(0);
	}

	ICVar* pShadow = pConsole->GetCVar("e_Shadows");
	if( pShadow )
	{
		m_OldConsoleVars.e_Shadows = pShadow->GetIVal();
		pShadow->Set(0);
	}

	ICVar* pParticle = pConsole->GetCVar("e_Particles");
	if( pParticle )
	{
		m_OldConsoleVars.e_Particles = pParticle->GetIVal();
		pParticle->Set(0);
	}

	ICVar* pClouds = pConsole->GetCVar("e_Clouds");
	if( pClouds )
	{
		m_OldConsoleVars.e_Clouds = pClouds->GetIVal();
		pClouds->Set(0);
	}

	ICVar* pBeams = pConsole->GetCVar("r_Beams");
	if( pBeams )
	{
		m_OldConsoleVars.r_Beams = pBeams->GetIVal();
		pBeams->Set(0);
	}

	ICVar* pSkybox = pConsole->GetCVar("e_SkyBox");
	if( pSkybox )
	{
		m_OldConsoleVars.e_SkyBox = pSkybox->GetIVal();
		pSkybox->Set(1);
	}
}

void SDesignerEnvironmentInfo::SetObjectsFlagForExclusiveMode()
{
	DynArray<CBaseObject*> objects;
	GetIEditor()->GetObjectManager()->GetObjects(objects);

	m_ObjectHiddenFlagMap.clear();

	CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();

	for( int i = 0, iObjectCount(objects.size()); i < iObjectCount; ++i )
	{
		m_ObjectHiddenFlagMap[objects[i]] = objects[i]->IsHidden();
		bool bSelected = false;
		for( int k = 0, iSelectionCount(pSelection->GetCount()); k < iSelectionCount; ++k )
		{
			if( pSelection->GetObject(k) == objects[i] )
			{
				bSelected = true;
				break;
			}
		}
		if( bSelected )
			continue;
		if( objects[i]->GetName() != BUtil::BrushDesignerCameraName && ( !objects[i]->GetParent() || objects[i]->GetParent()->GetName() != BUtil::BrushDesignerCameraName ) )
			objects[i]->SetHidden(true);
	}
}

void SDesignerEnvironmentInfo::SetTime( ITimeOfDay* pTOD, float fTime )
{
	if( pTOD == NULL )
		return;
	pTOD->SetTime(fTime,true);
	GetIEditor()->SetConsoleVar("e_TimeOfDay",fTime);

	CWnd* pWnd = GetIEditor()->FindView( "Time Of Day" );
	if( pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CTimeOfDayDialog)) )
	{
		((CTimeOfDayDialog*)pWnd)->UpdateValues();
	}
	else
	{
		pTOD->Update(false,true);
		GetIEditor()->GetDocument()->GetCurrentMission()->SetTime(fTime);
		GetIEditor()->Notify( eNotify_OnTimeOfDayChange );
	}
}

void SDesignerEnvironmentInfo::SetTimeOfDayForExclusiveMode()
{
	char workingDirectory[_MAX_PATH] = {0,};
	GetCurrentDirectory(_MAX_PATH,workingDirectory);
	CString designerTimeOfDay = CString(workingDirectory) + "\\Editor\\CryDesigner_TimeOfDay.xml";

	XmlNodeRef root = GetIEditor()->GetSystem()->LoadXmlFromFile(designerTimeOfDay);
	if (root)
	{
		ITimeOfDay *pTimeOfDay = GetIEditor()->Get3DEngine()->GetTimeOfDay();

		m_OldTimeOfDay = GetIEditor()->GetSystem()->CreateXmlNode();
		m_OldTimeOfTOD = GetIEditor()->GetConsoleVar("e_TimeOfDay");
		pTimeOfDay->Serialize(m_OldTimeOfDay,false);

		pTimeOfDay->Serialize(root,true);
		SetTime(pTimeOfDay,0);
	}
}

void SDesignerEnvironmentInfo::RestoreTimeOfDay()
{
	if( m_OldTimeOfDay )
	{
		ITimeOfDay *pTimeOfDay = GetIEditor()->Get3DEngine()->GetTimeOfDay();
		pTimeOfDay->Serialize(m_OldTimeOfDay,true);
		SetTime(pTimeOfDay,m_OldTimeOfTOD);
		m_OldTimeOfDay = NULL;
	}
}

void SDesignerEnvironmentInfo::RestoreCV()
{
	IConsole* pConsole = GetIEditor()->GetSystem()->GetIConsole();
	if( !pConsole )
		return;

	ICVar* pDisplayInfo = pConsole->GetCVar("r_DisplayInfo");
	if( pDisplayInfo )
		pDisplayInfo->Set(m_OldConsoleVars.r_DisplayInfo);

	ICVar* pHDRRendering = pConsole->GetCVar("r_HDRRendering");
	if( pHDRRendering )
		pHDRRendering->Set(m_OldConsoleVars.r_HDRRendering);

// 	ICVar* pPostProcessEffect = pConsole->GetCVar("r_PostProcessEffects");
// 	if( pPostProcessEffect )
// 		pPostProcessEffect->Set(m_OldConsoleVars.r_PostProcessEffects);

	ICVar* pRenderVegetation = pConsole->GetCVar("e_Vegetation");
	if( pRenderVegetation )
		pRenderVegetation->Set(m_OldConsoleVars.e_Vegetation);

	ICVar* pWaterOcean = pConsole->GetCVar("e_WaterOcean");
	if( pWaterOcean )
		pWaterOcean->Set(m_OldConsoleVars.e_WaterOcean);

	ICVar* pWaterVolume = pConsole->GetCVar("e_WaterVolumes");
	if( pWaterVolume )
		pWaterVolume->Set(m_OldConsoleVars.e_WaterVolumes);

	ICVar* pTerrain = pConsole->GetCVar("e_Terrain");
	if( pTerrain )
		pTerrain->Set(m_OldConsoleVars.e_Terrain);

	ICVar* pShadow = pConsole->GetCVar("e_Shadows");
	if( pShadow )
		pShadow->Set(m_OldConsoleVars.e_Shadows);

	ICVar* pParticle = pConsole->GetCVar("e_Particles");
	if( pParticle )
		pParticle->Set(m_OldConsoleVars.e_Particles);

	ICVar* pClouds = pConsole->GetCVar("e_Clouds");
	if( pClouds )
		pClouds->Set(m_OldConsoleVars.e_Clouds);

	ICVar* pBeams = pConsole->GetCVar("r_Beams");
	if( pBeams )
		pBeams->Set(m_OldConsoleVars.r_Beams);

	ICVar* pSkybox = pConsole->GetCVar("e_SkyBox");
	if( pSkybox )
		pSkybox->Set(m_OldConsoleVars.e_SkyBox);
}

void SDesignerEnvironmentInfo::RestoreObjectsFlag()
{
	DynArray<CBaseObject*> objects;
	GetIEditor()->GetObjectManager()->GetObjects(objects);

	for( int i = 0, iObjectCount(objects.size()); i < iObjectCount; ++i )
	{
		if( m_ObjectHiddenFlagMap.find(objects[i]) != m_ObjectHiddenFlagMap.end() )
			objects[i]->SetHidden(m_ObjectHiddenFlagMap[objects[i]]);
	}

	GetIEditor()->SetConsoleVar("e_Vegetation",1.0f);
}
