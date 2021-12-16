////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   cry3dengine.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Defines the DLL entry point, implements access to other modules
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

// Must be included only once in DLL module.
#include <platform_impl.h>

#include "3dEngine.h"
#include "VoxTerrain.h"

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#define MAX_ERROR_STRING MAX_WARNING_LENGTH

//////////////////////////////////////////////////////////////////////

struct CSystemEventListner_3DEngine : public ISystemEventListener
{
public:
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
	{
		switch (event)
		{
		case ESYSTEM_EVENT_RANDOM_SEED:
			g_random_generator.seed((uint32)wparam);
			break;
		case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
			{
				STLALLOCATOR_CLEANUP;
				break;
			}
		case ESYSTEM_EVENT_LEVEL_LOAD_END:
			{
				if (Cry3DEngineBase::m_p3DEngine)
					Cry3DEngineBase::m_p3DEngine->ClearDebugFPSInfo();
				if (Cry3DEngineBase::GetObjManager())
					Cry3DEngineBase::GetObjManager()->FreeNotUsedCGFs();
				break;
			}
		}
	}
};
static CSystemEventListner_3DEngine g_system_event_listener_engine;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CEngineModule_Cry3DEngine : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_Cry3DEngine, "EngineModule_Cry3DEngine", 0x2d38f12a521d43cf, 0xba18fd1fa7ea5020)

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() { return "Cry3DEngine"; };
	virtual const char *GetCategory() { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams )
	{
		ISystem* pSystem = env.pSystem;

		ModuleInitISystem(pSystem,"Cry3DEngine");
		pSystem->GetISystemEventDispatcher()->RegisterListener( &g_system_event_listener_engine );

		C3DEngine * p3DEngine = new C3DEngine(pSystem);
		env.p3DEngine = p3DEngine;
		return true;
	}
};

CRYREGISTER_CLASS(CEngineModule_Cry3DEngine)

CEngineModule_Cry3DEngine::CEngineModule_Cry3DEngine()
{
};

CEngineModule_Cry3DEngine::~CEngineModule_Cry3DEngine()
{
};


//////////////////////////////////////////////////////////////////////////
void Cry3DEngineBase::PrintComment(const char *szText,...)
{
	if(!szText)
		return;
	
	va_list args;
	va_start(args, szText);
	GetLog()->LogV(IMiniLog::eComment,szText,args);
	va_end(args);
}

void Cry3DEngineBase::PrintMessage(const char *szText,...)
{
	if(!szText)
		return;

	va_list args;
	va_start(args, szText);
	GetLog()->LogV(IMiniLog::eMessage,szText,args);
	va_end(args);

//	GetLog()->UpdateLoadingScreen(0);
}

void Cry3DEngineBase::PrintMessagePlus(const char *szText,...)
{
	if(!szText)
		return;

	va_list	arglist;
	char buf[MAX_ERROR_STRING];
	va_start(arglist, szText);
	int count = vsnprintf(buf,sizeof(buf),szText, arglist);
	if ( count == -1 || count>= sizeof(buf) )
		buf[sizeof(buf)-1] = '\0';
	va_end(arglist);	
	GetLog()->LogPlus(buf);

//	GetLog()->UpdateLoadingScreen(0);
}

float Cry3DEngineBase::GetCurTimeSec() 
{ return (gEnv->pTimer->GetCurrTime()); }

float Cry3DEngineBase::GetCurAsyncTimeSec() 
{ return (gEnv->pTimer->GetAsyncTime().GetSeconds()); }

//////////////////////////////////////////////////////////////////////////
void Cry3DEngineBase::Warning( const char *format,... )
{
	if (!format)
		return;

	va_list	args;
	va_start(args, format);
	// Call to validating warning of system.
	m_pSystem->WarningV( VALIDATOR_MODULE_3DENGINE,VALIDATOR_WARNING,0,0,format,args );
	va_end(args);

//  GetLog()->UpdateLoadingScreen(0);
}

//////////////////////////////////////////////////////////////////////////
void Cry3DEngineBase::Error( const char *format,... )
{
//  assert(!"Cry3DEngineBase::Error");

	va_list	args;
	va_start(args, format);
	// Call to validating warning of system.
	m_pSystem->WarningV( VALIDATOR_MODULE_3DENGINE,VALIDATOR_ERROR,0,0,format,args );
	va_end(args);

//	GetLog()->UpdateLoadingScreen(0);
}

//////////////////////////////////////////////////////////////////////////
void Cry3DEngineBase::FileWarning( int flags,const char *file,const char *format,... )
{
	va_list	args;
	va_start(args, format);
	// Call to validating warning of system.
	m_pSystem->WarningV( VALIDATOR_MODULE_3DENGINE,VALIDATOR_WARNING,flags|VALIDATOR_FLAG_FILE,file,format,args );
	va_end(args);

//	GetLog()->UpdateLoadingScreen(0);
}

IMaterial * Cry3DEngineBase::MakeSystemMaterialFromShader(const char * sShaderName)
{
	IMaterial * pMat = Get3DEngine()->GetMaterialManager()->CreateMaterial( sShaderName );
	pMat->AddRef();
	SShaderItem si;
	si = GetRenderer()->EF_LoadShaderItem(sShaderName, true, EF_SYSTEM);
	pMat->AssignShaderItem(si);
	return pMat;
}

//////////////////////////////////////////////////////////////////////////
bool Cry3DEngineBase::IsValidFile( const char *sFilename )
{
  LOADING_TIME_PROFILE_SECTION(GetSystem());

	CCryFile file;
	if (!file.Open(sFilename,"rb",ICryPak::FOPEN_HINT_QUIET))
	{
		return false;
	}
	if (file.GetLength() <= 0)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool Cry3DEngineBase::IsResourceLocked( const char *sFilename )
{
	IResourceList *pResList = GetPak()->GetRecorderdResourceList(ICryPak::RFOM_NextLevel);
	if (pResList)
	{
		return pResList->IsExist( sFilename );
	}
	return false;
}

void Cry3DEngineBase::DrawBBoxLabeled( const AABB & aabb, const Matrix34 & m34, const ColorB & col, const char * format, ... )
{
  va_list args;
  va_start(args, format);
  char szText[256];
  vsnprintf_s(szText,sizeof(szText),sizeof(szText)-1,format, args);
  float fColor[4] = {col[0]/255.f,col[1]/255.f,col[2]/255.f,col[3]/255.f};
  GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
  GetRenderer()->DrawLabelEx( m34.TransformPoint(aabb.GetCenter()), 1.3f, fColor, true, true, szText );
  GetRenderer()->GetIRenderAuxGeom()->DrawAABB( aabb, m34, false, col, eBBD_Faceted );
  va_end(args);
}

//////////////////////////////////////////////////////////////////////////
void Cry3DEngineBase::DrawBBox( const Vec3 & vMin, const Vec3 & vMax )
{
	GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
	GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vMin, vMax),false,ColorB(255,255,255,255),eBBD_Faceted);
}

void Cry3DEngineBase::DrawBBox( const AABB & box, ColorB col )
{
	GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
	GetRenderer()->GetIRenderAuxGeom()->DrawAABB(box,false,col,eBBD_Faceted);
}

void Cry3DEngineBase::DrawLine( const Vec3 & vMin, const Vec3 & vMax )
{
	GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
	GetRenderer()->GetIRenderAuxGeom()->DrawLine(vMin, ColorB(255,255,255,255), vMax, ColorB(255,255,255,255));
}

void Cry3DEngineBase::DrawSphere( const Vec3 & vPos, float fRadius, ColorB color )
{
	GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
	GetRenderer()->GetIRenderAuxGeom()->DrawSphere(vPos, fRadius, color);
}

// Check if preloading is enabled.
bool Cry3DEngineBase::IsPreloadEnabled()
{
	bool bPreload = false;
	ICVar *pSysPreload = GetConsole()->GetCVar( "sys_preload" );
	if (pSysPreload && pSysPreload->GetIVal() != 0)
		bPreload = true;

	return bPreload;
}

//////////////////////////////////////////////////////////////////////////
bool Cry3DEngineBase::CheckMinSpec( uint32 nMinSpec )
{
	if (nMinSpec == CONFIG_DETAIL_SPEC && GetCVars()->e_ViewDistRatioDetail == 0)
		return false;

	if ((int)nMinSpec != 0 && GetCVars()->e_ObjQuality != 0 && (int)nMinSpec > GetCVars()->e_ObjQuality)
		return false;

	return true;
}

bool Cry3DEngineBase::IsEscapePressed()
{
#ifdef WIN32
  if(Cry3DEngineBase::m_bEditor && (CryGetAsyncKeyState(0x03) & 1)) // Ctrl+Break
  {
    Get3DEngine()->PrintMessage("*** Ctrl-Break was pressed - operation aborted ***");
    if(Get3DEngine()->m_pVoxTerrain)
      Get3DEngine()->m_pVoxTerrain->m_bAllowEditing = false;
    return true;
  }
#endif  
  return false;
}

#include <CrtDebugStats.h>

// TypeInfo implementations for Cry3DEngine
#ifndef _LIB
	#include "Common_TypeInfo.h"
#endif

#include "TypeInfo_impl.h"

// Common types
#include "IIndexedMesh_info.h"
#include "IEntityRenderState_info.h"
#include "CGFContent_info.h"

// 3DEngine types
#include "SkyLightNishita_info.h"
#include "terrain_sector_info.h"
#include "VoxMan_info.h"

//#include "IsoOctree/Octree.h"

// isotree
STRUCT_INFO_BEGIN(SIsoTreeInfo)
	VAR_INFO(aabbTerrain)
	VAR_INFO(fOceanWaterLevel)
	VAR_INFO(maxDepth)
	VAR_INFO(nSVoxValueSize)
	VAR_INFO(nModId)
STRUCT_INFO_END(SIsoTreeInfo)

STRUCT_INFO_BEGIN(SIsoTreeChunkHeader)
	VAR_INFO(nVersion)
	VAR_INFO(nChunkSize)
	VAR_INFO(TerrainInfo)
STRUCT_INFO_END(SIsoTreeChunkHeader)

// OctNode
STRUCT_INFO_BEGIN(IsoTreeNodeData)
	VAR_INFO(mcIndex)
STRUCT_INFO_END(IsoTreeNodeData)

STRUCT_INFO_BEGIN(OctNodeChunk)
	VAR_INFO(nHasChilds)
	VAR_INFO(nodeData)
STRUCT_INFO_END(OctNodeChunk)

STRUCT_INFO_BEGIN(SSurfTypeInfo)
  VAR_INFO(we)
  VAR_INFO(ty)
STRUCT_INFO_END(SSurfTypeInfo)

STRUCT_INFO_BEGIN(SVoxValue)
	VAR_INFO(val)
	VAR_INFO(surf_type)
	VAR_INFO(color)
  VAR_INFO(normal)
STRUCT_INFO_END(SVoxValue)

STRUCT_INFO_BEGIN(SVoxValueOld)
VAR_INFO(val)
VAR_INFO(surf_type)
VAR_INFO(color)
STRUCT_INFO_END(SVoxValueOld)

STRUCT_INFO_BEGIN(SVoxValueOld2)
  VAR_INFO(val)
  VAR_INFO(surf_type)
  VAR_INFO(color)
STRUCT_INFO_END(SVoxValueOld2)

STRUCT_INFO_BEGIN(SFlatnessItem)
VAR_INFO(first)
VAR_INFO(second)
STRUCT_INFO_END(SFlatnessItem)

STRUCT_INFO_BEGIN(SImageSubInfo)
	VAR_INFO(nDummy)
	VAR_INFO(nDim)
	VAR_INFO(fTilingIn)
	VAR_INFO(fTiling)
	VAR_INFO(fSpecularAmount)
	VAR_INFO(nSortOrder)
STRUCT_INFO_END(SImageSubInfo)

STRUCT_INFO_BEGIN(SImageInfo)
	VAR_INFO(baseInfo)
	VAR_INFO(detailInfo)
	VAR_INFO(szDetMatName)
  VAR_INFO(arrTextureId)
  VAR_INFO(nPhysSurfaceType)
	VAR_INFO(szBaseTexName)
	VAR_INFO(fUseRemeshing)
	VAR_INFO(layerFilterColor)
	VAR_INFO(nLayerId)
	VAR_INFO(fBr)
STRUCT_INFO_END(SImageInfo)

STRUCT_INFO_BEGIN(SVoxTexHeader)
  VAR_INFO(w)
  VAR_INFO(h)
  VAR_INFO(eFormat)
  VAR_INFO(nPitch)
STRUCT_INFO_END(SVoxTexHeader)









