////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   brush.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <ILMSerializationManager.h>
#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "terrain_sector.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "IndexedMesh.h"
#include "WaterVolumes.h"
#include "Brush.h"
#include "terrain.h"

const char * CBrush::GetEntityClassName() const
{
	return "Brush";
}

Vec3 CBrush::GetPos(bool bWorldOnly) const
{
	assert(bWorldOnly);
	return m_Matrix.GetTranslation();
}

const char *CBrush::GetName() const
{
	if (m_pStatObj)
		return m_pStatObj->GetFilePath();
	return "StatObjNotSet";
}

bool CBrush::HasChanged()
{
	return false;
}

#ifdef WIN64
#pragma warning( push )									//AMD Port
#pragma warning( disable : 4311 )
#endif

void CBrush::Render(const struct SRendParams & _EntDrawParams)
{
	FUNCTION_PROFILER_3DENGINE;

  if (!m_pStatObj || m_dwRndFlags&ERF_HIDDEN)
    return; //false;

	if (m_dwRndFlags & ERF_COLLISION_PROXY || m_dwRndFlags & ERF_RAYCAST_PROXY)
	{
		// Collision proxy is visible in Editor while in editing mode.
		if (!gEnv->IsEditor() || gEnv->bEditorGameMode)
		{
			if (GetCVars()->e_DebugDraw == 0)
				return; //true;
		}
	}

	// some parameters will be modified
	SRendParams rParms = _EntDrawParams;

#ifdef SUPPORT_LM
	// enable lightmaps if allowed
	if(m_dwRndFlags&ERF_USERAMMAPS && HasLightmap(0) && GetCVars()->e_ram_maps)
		rParms.m_pLMData	=	GetLightmapData(0);
  else
    rParms.m_pLMData	=	0;
#endif // SUPPORT_LM

  if(m_nMaterialLayers)
    rParms.nMaterialLayers = m_nMaterialLayers;

	rParms.pMatrix = &m_Matrix;
	rParms.nMotionBlurAmount = 0;
	rParms.pMaterial = m_pMaterial;
  rParms.ppRNTmpData = &m_pRNTmpData;

  float fRadius = GetBBox().GetRadius();

  if(m_dwRndFlags&ERF_MERGE_RESULT)
  { // merged vegetations
    rParms.dwFObjFlags |= FOB_VEGETATION;
    rParms.dwFObjFlags &= ~FOB_TRANS_MASK;   
    if(m_dwRndFlags&ERF_USE_TERRAIN_COLOR && rParms.pTerrainTexInfo)
    {
      rParms.dwFObjFlags |= FOB_BLEND_WITH_TERRAIN_COLOR;
      Vec3 vTerrainNormal = GetTerrain()->GetTerrainSurfaceNormal(GetBBox().GetCenter(), fRadius);
      uint8 ucSunDotTerrain	 = (uint8)(CLAMP((vTerrainNormal.Dot(Get3DEngine()->GetSunDirNormalized()))*255.f, 0, 255));
      rParms.AmbientColor.a *= (1.0f / 255.f * ucSunDotTerrain);
    }
  }

  if( !m_Matrix.m01 && !m_Matrix.m02 && !m_Matrix.m10 && !m_Matrix.m12 && !m_Matrix.m20 && !m_Matrix.m21 )
    rParms.dwFObjFlags &= ~FOB_TRANS_ROTATE;
  else
    rParms.dwFObjFlags |= FOB_TRANS_ROTATE;

  // get statobj for rendering
  IStatObj * pStatObj = GetStatObj();

  // cull lights per triangle for big objects
//  if(GetCVars()->e_brushes_cull_lights_per_triangle_min_obj_radius && pStatObj->GetRenderMesh() &&
  //  fRadius > GetCVars()->e_brushes_cull_lights_per_triangle_min_obj_radius)
//    CObjManager::CullLightsPerTriangle(pStatObj->GetRenderMesh(), m_Matrix, rParms.nDLightMask, m_lightsCache);

	// render
	if(pStatObj)
	{
#ifdef SUPPORT_LM
		if (pStatObj->m_nFlags&STATIC_OBJECT_COMPOUND)
			rParms.m_pLMData	=	(m_SubObjectLightmapData.size()==pStatObj->SubObjectCount())?&m_SubObjectLightmapData[0]:0;
#endif // SUPPORT_LM

		//if(GetCVars()->e_ram_maps && rParms.m_pLMData && rParms.m_pLMData->m_pLMData && rParms.m_pLMData->m_pLMData->GetRAETex())
		//	rParms.dwFObjFlags |= FOB_RAE_GEOMTERM;
		//else
		//	rParms.dwFObjFlags &= ~FOB_RAE_GEOMTERM;

		pStatObj->Render(rParms);
	}
}

void CBrush::Render(SRenderObjectModifier * pROM, int nThreadId) 
{
  FUNCTION_PROFILER_3DENGINE;

  // Collision proxy is visible in Editor while in editing mode.
  if(m_dwRndFlags & ERF_COLLISION_PROXY)
    if (!gEnv->IsEditor() || gEnv->bEditorGameMode)
      if (GetCVars()->e_DebugDraw == 0)
        return;

  if(m_dwRndFlags & ERF_HIDDEN)
    return;

	if(m_dwRndFlags & ERF_SELECTED)
		m_fLastSelectedTime = Get3DEngine()->GetCurTimeSec();

  if(GetIntegrationType() && (!(m_fLastSelectedTime > Get3DEngine()->GetCurTimeSec()-2.f) || !GetCVars()->e_VoxTerOnTheFlyIntegration))
  {
    if( GetCVars()->e_Voxel && GetCVars()->e_VoxTerHideIntegrated==2 )
      return; // hide always
    if( GetCVars()->e_Voxel && GetCVars()->e_VoxTerHideIntegrated && Get3DEngine()->GetIVoxTerrain() )
      return; // hide only if voxel terrain used    
  }

  CRenderObject * & pObj = m_pRNTmpData->userData.pRenderObject;

	//////////////////////////////////////////////////////////////////////////
	// temp fix to update ambient color (Vlad please review!)
	if(m_pOcNode && m_pOcNode->m_pVisArea)
		pObj->m_II.m_AmbColor = m_pOcNode->m_pVisArea->GetFinalAmbientColor();
	else 
		pObj->m_II.m_AmbColor = Get3DEngine()->GetSkyColor();
	//////////////////////////////////////////////////////////////////////////

  if(GetCVars()->e_Dissolve && !m_nRenderStackLevel)
  {
    if (pObj->m_DissolveRef = GetObjManager()->GetDissolveRef(pObj->m_fDistance - m_fWSMaxViewDist, &m_pRNTmpData->userData.dissolveTransitionState))
    {
      pObj->m_ObjFlags |= FOB_DISSOLVE;
      if(pObj->m_DissolveRef == 255)
        return;
    }
    else
      pObj->m_ObjFlags &= ~FOB_DISSOLVE;
  }

  if(CStatObj * pStatObj = (CStatObj *)GetStatObj())
  {
    pStatObj->RenderInternal(pObj, pROM, nThreadId, m_pRNTmpData->userData.nLod);

    if (GetCVars()->e_BBoxes && !IsRenderIntoShadowmap())
      GetObjManager()->RenderObjectDebugInfo((IRenderNode*)this, pObj);
  }
}

#ifdef WIN64
#pragma warning( pop )									//AMD Port
#endif

struct IStatObj * CBrush::GetEntityStatObj( unsigned int nPartId, unsigned int nSubPartId, Matrix34A* pMatrix, bool bReturnOnlyVisible )
{
	if(nPartId != 0)
		return 0;

	if(pMatrix)
		*pMatrix = m_Matrix;

	return m_pStatObj;
}

void CBrush::SetMatrix( const Matrix34* pMatrix )
{
	Get3DEngine()->UnRegisterEntity(this);

	//	InvalidateShadow();

	if(pMatrix)
	{
		if(!IsMatrixValid(*pMatrix))
		{
			Warning( "Error: IRenderNode::SetMatrix: Invalid matrix passed from the editor - ignored, reset to identity: %s", GetName());
			m_Matrix.SetIdentity();
		}
		else
			m_Matrix = *pMatrix;

		CalcBBox();
	}

	Get3DEngine()->RegisterEntity(this);
	if (!m_pPhysEnt)
		Physicalize();
	else
	{
		// Just move physics.
		pe_status_placeholder spc;
		if (m_pPhysEnt->GetStatus(&spc) && !spc.pFullEntity)
		{
			pe_params_bbox pbb;
			pbb.BBox[0]=m_WSBBox.min; pbb.BBox[1]=m_WSBBox.max;
			m_pPhysEnt->SetParams(&pbb);
		} else
		{
			pe_params_pos par_pos;
			par_pos.pMtx3x4 = &m_Matrix;
			m_pPhysEnt->SetParams(&par_pos);
		}

		//////////////////////////////////////////////////////////////////////////
		// Update physical flags.
		//////////////////////////////////////////////////////////////////////////
		pe_params_foreign_data  foreignData;
		m_pPhysEnt->GetParams(&foreignData);
		if (m_dwRndFlags & ERF_HIDABLE)
			foreignData.iForeignFlags |= PFF_HIDABLE;
		else
			foreignData.iForeignFlags &= ~PFF_HIDABLE;
		if (m_dwRndFlags & ERF_HIDABLE_SECONDARY)
			foreignData.iForeignFlags |= PFF_HIDABLE_SECONDARY;
		else
			foreignData.iForeignFlags &= ~PFF_HIDABLE_SECONDARY;
		// flag to exclude from AI triangulation
		if (m_dwRndFlags & ERF_EXCLUDE_FROM_TRIANGULATION)
			foreignData.iForeignFlags |= PFF_EXCLUDE_FROM_STATIC;
		else
			foreignData.iForeignFlags &= ~PFF_EXCLUDE_FROM_STATIC;
		m_pPhysEnt->SetParams(&foreignData);
	}

	if(GetCVars()->e_VoxTer && GetIntegrationType() && m_bEditor)
		PreCopyResources();
}

void CBrush::CalcBBox()
{
	m_WSBBox.min = SetMaxBB();
	m_WSBBox.max = SetMinBB();

	if (!m_pStatObj)
		return;

	m_WSBBox.min = m_pStatObj->GetBoxMin();
	m_WSBBox.max = m_pStatObj->GetBoxMax();
	m_WSBBox.SetTransformedAABB(m_Matrix,m_WSBBox);
  m_fMatrixScale = m_Matrix.GetColumn0().GetLength();
}

CBrush::CBrush()
{
	m_WSBBox.min=m_WSBBox.max=Vec3(ZERO);
	m_dwRndFlags=0;
	m_Matrix.SetIdentity();
	m_pPhysEnt=0;
	m_Matrix.SetIdentity();
	m_pMaterial = 0;
	m_nMergeGroupId = 0;
	m_pStatObj = NULL;
  m_pMaterial = NULL;
	m_bVehicleOnlyPhysics = false;
  m_vWindBending = Vec2(0,0);
  m_bMerged = 0;
  m_fMatrixScale = 1.f;
	m_fLastSelectedTime = 0;

	GetInstCount(GetRenderNodeType())++;
}

void CBrush::DeleteLMTC()
{
#ifdef SUPPORT_LM
	for(int nLod=0;nLod<MAX_BRUSH_LODS_NUM;nLod++)
		if(m_arrLMData[nLod].m_pLMTCBuffer)
		{
			gEnv->pRenderer->DeleteRenderMesh(m_arrLMData[nLod].m_pLMTCBuffer);
			m_arrLMData[nLod].m_pLMTCBuffer = NULL;
		}
		for(int a=0;a<m_SubObjectLightmapData.size();a++)
			if(m_SubObjectLightmapData[a].m_pLMTCBuffer)
			{
				gEnv->pRenderer->DeleteRenderMesh(m_SubObjectLightmapData[a].m_pLMTCBuffer);
				m_SubObjectLightmapData[a].m_pLMTCBuffer = NULL;
			}
#endif // SUPPORT_LM
}

CBrush::~CBrush()
{  
	Dephysicalize( );
//	Get3DEngine()->UnRegisterEntity(this);
  Get3DEngine()->FreeRenderNodeState(this);
  
	//m_pLMData = NULL;
/*
	if(GetRndFlags()&ERF_MERGED_NEW && m_nObjectTypeID>=0)
	{
		CStatObj * pBody = (CStatObj*)GetObjManager()->m_lstBrushTypes[m_nObjectTypeID];
		assert(pBody->m_nUsers==1);
		delete pBody;
		GetObjManager()->m_lstBrushTypes[m_nObjectTypeID] = NULL;
		m_nObjectTypeID=-1;
	}
*/
  DeleteLMTC();
	
  m_pStatObj = NULL;

	if(m_pRNTmpData)
		Get3DEngine()->FreeRNTmpData(&m_pRNTmpData);
	assert(!m_pRNTmpData);

  GetInstCount(GetRenderNodeType())--;
}

void CBrush::Physicalize(bool bInstant)
{
	float fScaleX = m_Matrix.GetColumn(0).len();
	float fScaleY = m_Matrix.GetColumn(1).len();
	float fScaleZ = m_Matrix.GetColumn(2).len();

	if( fabs(fScaleX - fScaleY)>0.01f || fabs(fScaleX - fScaleZ)>0.01f || !m_pStatObj || !m_pStatObj->IsPhysicsExist() )
	{ // scip non uniform scaled object and object without physics
		
		// Check if we are compound object.
		if (m_pStatObj && !m_pStatObj->IsPhysicsExist() && (m_pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND))
		{
			// Try to physicalize compound object.
		}
		else
		{
			Dephysicalize();
			return;
		}
	}

	/*if (!GetCVars()->e_OnDemandPhysics || pBody->GetFlags() & STATIC_OBJECT_COMPOUND)
		bInstant = true;

	if (!bInstant)
	{
		pe_status_placeholder spc;
		if (m_pPhysEnt && m_pPhysEnt->GetStatus(&spc) && spc.pFullEntity)
			GetSystem()->GetIPhysicalWorld()->DestroyPhysicalEntity(spc.pFullEntity);

		pe_params_bbox pbb;
		pbb.BBox[0] = m_WSBBox.min;
		pbb.BBox[1] = m_WSBBox.max;
		pe_params_foreign_data pfd;
		pfd.pForeignData = (IRenderNode*)this;
		pfd.iForeignData = PHYS_FOREIGN_ID_STATIC;
		pfd.iForeignFlags = PFF_BRUSH;

		if (!m_pPhysEnt)
			m_pPhysEnt = GetSystem()->GetIPhysicalWorld()->CreatePhysicalPlaceholder(PE_STATIC,&pbb);
		else
			m_pPhysEnt->SetParams(&pbb);
		m_pPhysEnt->SetParams(&pfd);
		return;
	}*/

//  CStatObj * pBody = (CStatObj*)GetObjManager()->m_lstBrushTypes[m_nObjectTypeID];
//  m_pStatObj->CheckLoaded();
//  if(!(pBody && (pBody->m_arrPhysGeomInfo[0] || pBody->m_arrPhysGeomInfo[1])))
  //  return;

	//Dephysicalize( );
	// create new
//	pe_params_pos par_pos;
	//pe_status_placeholder spc;
	//int bOnDemandCallback = 0;
	if (!m_pPhysEnt)
	{
		m_pPhysEnt = GetSystem()->GetIPhysicalWorld()->CreatePhysicalEntity(PE_STATIC,NULL,(IRenderNode*)this,PHYS_FOREIGN_ID_STATIC);
		if(!m_pPhysEnt)
			return;
	}
	/*else if (bOnDemandCallback = m_pPhysEnt->GetStatus(&spc) && spc.pFullEntity==0) 
		GetSystem()->GetIPhysicalWorld()->CreatePhysicalEntity(PE_STATIC,5.0f,0,0,0,-1,m_pPhysEnt);*/

	pe_action_remove_all_parts remove_all;
	m_pPhysEnt->Action(&remove_all);//,bOnDemandCallback);

  pe_geomparams params;	  
	if(m_pStatObj->GetPhysGeom(PHYS_GEOM_TYPE_DEFAULT))
  {
		/*if (m_pStatObj->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_NO_COLLIDE])
			params.flags &= ~geom_colltype_ray;*/
		if (m_bVehicleOnlyPhysics || (m_pStatObj->GetVehicleOnlyPhysics()!=0))
				params.flags = geom_colltype6;
		if (GetCVars()->e_ObjQuality != CONFIG_LOW_SPEC) 
		{
			params.idmatBreakable = m_pStatObj->GetIDMatBreakable();
			if (m_pStatObj->GetBreakableByGame())
				params.flags |= geom_manually_breakable;
		}	else
			params.idmatBreakable = -1;

		if (GetRndFlags() & ERF_COLLISION_PROXY)
		{
			// Collision proxy only collides with players and vehicles.
			params.flags |= geom_colltype_player | geom_colltype_vehicle;
		}
		if (GetRndFlags() & ERF_RAYCAST_PROXY)
		{
			// Collision proxy only collides with players and vehicles.
			params.flags |= geom_colltype_ray;
		}
  }
	m_pStatObj->Physicalize(m_pPhysEnt, &params);

  if(m_dwRndFlags & (ERF_HIDABLE|ERF_HIDABLE_SECONDARY|ERF_EXCLUDE_FROM_TRIANGULATION))
  {
    pe_params_foreign_data  foreignData;
    m_pPhysEnt->GetParams(&foreignData);
		if (m_dwRndFlags & ERF_HIDABLE)
			foreignData.iForeignFlags |= PFF_HIDABLE;
		if (m_dwRndFlags & ERF_HIDABLE_SECONDARY)
			foreignData.iForeignFlags |= PFF_HIDABLE_SECONDARY;
		//[PETAR] new flag to exclude from triangulation
		if (m_dwRndFlags & ERF_EXCLUDE_FROM_TRIANGULATION)
			foreignData.iForeignFlags |= PFF_EXCLUDE_FROM_STATIC;
    m_pPhysEnt->SetParams(&foreignData);//,bOnDemandCallback);
  }

	pe_params_flags par_flags;
	par_flags.flagsOR = pef_never_affect_triggers;
	m_pPhysEnt->SetParams(&par_flags);//,bOnDemandCallback);

  pe_params_pos par_pos;
	par_pos.pMtx3x4 = &m_Matrix;
	m_pPhysEnt->SetParams(&par_pos);//,bOnDemandCallback);

	if (m_pMaterial)
		UpdatePhysicalMaterials( );//bOnDemandCallback );
}

//////////////////////////////////////////////////////////////////////////
void CBrush::UpdatePhysicalMaterials(int bThreadSafe)
{
	if (!m_pPhysEnt)
		return;

	if ((GetRndFlags() & ERF_COLLISION_PROXY) && m_pPhysEnt)
	{
		pe_params_part ppart;
		ppart.flagsAND = 0;
		ppart.flagsOR = geom_colltype_player | geom_colltype_vehicle;
		m_pPhysEnt->SetParams( &ppart );
	}

	if ((GetRndFlags() & ERF_RAYCAST_PROXY) && m_pPhysEnt)
	{
		pe_params_part ppart;
		ppart.flagsAND = 0;
		ppart.flagsOR = geom_colltype_ray;
		m_pPhysEnt->SetParams( &ppart );
	}
	
	if (m_pMaterial)
	{
		// Assign custom material to physics.
		int surfaceTypesId[MAX_SUB_MATERIALS];
		memset( surfaceTypesId,0,sizeof(surfaceTypesId) );
		int i,numIds = m_pMaterial->FillSurfaceTypeIds(surfaceTypesId);
		ISurfaceTypeManager *pSurfaceTypeManager = Get3DEngine()->GetMaterialManager()->GetSurfaceTypeManager();
		ISurfaceType *pMat;
		bool bBreakable = false;

		for(i=0,m_bVehicleOnlyPhysics=false; i<numIds; i++)
			if (pMat=pSurfaceTypeManager->GetSurfaceType(surfaceTypesId[i]))
			{
				if (pMat->GetFlags() & SURFACE_TYPE_VEHICLE_ONLY_COLLISION)
					m_bVehicleOnlyPhysics = true;
				if (pMat->GetBreakability())
					bBreakable = true;
			}
	
		if (bBreakable && m_pStatObj)
		{
			// mark the rendermesh as KepSysMesh so that it is kept in system memory
			if( m_pStatObj->GetRenderMesh() )
				m_pStatObj->GetRenderMesh()->KeepSysMesh(true);
			
			m_pStatObj->SetFlags( m_pStatObj->GetFlags() | STATIC_OBJECT_DYNAMIC);
		}
		pe_params_part ppart;
		ppart.nMats = numIds;
		ppart.pMatMapping = surfaceTypesId;
		if (m_bVehicleOnlyPhysics)
			ppart.flagsAND = geom_colltype6;
		m_pPhysEnt->SetParams( &ppart, bThreadSafe );
	}	else if (m_bVehicleOnlyPhysics)
	{
		m_bVehicleOnlyPhysics = false;
		if (!m_pStatObj->GetVehicleOnlyPhysics())
		{
			pe_params_part ppart;
			ppart.flagsOR = geom_colltype_solid|geom_colltype_ray|geom_floats|geom_colltype_explosion;
			m_pPhysEnt->SetParams( &ppart, bThreadSafe );
		}
	}
}

void CBrush::Dephysicalize( bool bKeepIfReferenced )
{
	// delete old physics
	if(m_pPhysEnt && 0 != GetSystem()->GetIPhysicalWorld()->DestroyPhysicalEntity(m_pPhysEnt, ((int)bKeepIfReferenced)*4))
		m_pPhysEnt=0;
}

void CBrush::Dematerialize( )
{
  if(m_pMaterial)
    m_pMaterial = 0;
}

IPhysicalEntity* CBrush::GetPhysics() const
{
	return m_pPhysEnt;
}

//////////////////////////////////////////////////////////////////////////
void CBrush::SetPhysics( IPhysicalEntity* pPhys )
{
	m_pPhysEnt = pPhys;
}

//////////////////////////////////////////////////////////////////////////
bool CBrush::IsMatrixValid(const Matrix34 & mat)
{
	Vec3 vScaleTest = mat.TransformVector(Vec3(0,0,1));
  float fDist = mat.GetTranslation().GetDistance(Vec3(0,0,0));

	if( vScaleTest.GetLength()>1000.f || vScaleTest.GetLength()<0.01f || fDist > 256000 ||
		!_finite(vScaleTest.x) || !_finite(vScaleTest.y) || !_finite(vScaleTest.z) )
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CBrush::SetMaterial( IMaterial *pMat )
{
  m_pMaterial = pMat;
	
	bool bCollisionProxy = false;
	bool bRaycastProxy = false;

	if (pMat)
	{
		if (pMat->GetFlags() & MTL_FLAG_COLLISION_PROXY)
			bCollisionProxy = true;

		if (pMat->GetFlags() & MTL_FLAG_RAYCAST_PROXY)
		{
			bRaycastProxy = true;
		}
	}
	SetRndFlags( ERF_COLLISION_PROXY,bCollisionProxy );
	SetRndFlags( ERF_RAYCAST_PROXY, bRaycastProxy);

	UpdatePhysicalMaterials();

  if (pMat != NULL &&  pMat->IsSubSurfScatterCaster())
  {
    m_dwRndFlags |= ERF_SUBSURFSCATTER;
    //force creating depth maps
    m_dwRndFlags |= ERF_CASTSHADOWMAPS;

		if(m_pOcNode)
			((COctreeNode*)m_pOcNode)->MarkAsUncompiled();
  }

	// register and get brush material id
	m_pMaterial = pMat;

	if(GetCVars()->e_VoxTer && GetIntegrationType() && m_bEditor)
		PreCopyResources();
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CBrush::GetMaterial(Vec3 * pHitPos)
{
	if (m_pMaterial)
		return m_pMaterial;

	if(m_pStatObj)
		return m_pStatObj->GetMaterial();

	return NULL;
}

void CBrush::CheckPhysicalized()
{
//  if(GetEntityStatObj(0))
  //  ((CStatObj*)GetEntityStatObj(0))->CheckLoaded(true);

  if(!m_pPhysEnt)
    Physicalize();
}

float CBrush::GetMaxViewDist()
{
	if (GetMinSpecFromRenderNodeFlags(m_dwRndFlags) == CONFIG_DETAIL_SPEC)
		return max(GetCVars()->e_ViewDistMin,min(GetCVars()->e_ViewDistCompMaxSize, GetBBox().GetRadius())*GetCVars()->e_ViewDistRatioDetail*GetViewDistRatioNormilized());

  return max(GetCVars()->e_ViewDistMin,min(GetCVars()->e_ViewDistCompMaxSize, GetBBox().GetRadius())*GetCVars()->e_ViewDistRatio*GetViewDistRatioNormilized());
}
/*
void CBrush::Serialize(bool bSave, ICryPak * pPak, FILE * f)
{
#if 0

  if(bSave)
  {
    pPak->FWrite(this,sizeof(*this),1,f);

    if(0 && m_pLMTCBuffer)
    {
		*/
  /*    int nPos=0;
      m_pLMTCBuffer->Serialize(nPos,0,true,0,0);
      uint8 * pLMLBuffer = new uint8[nPos];
      pPak->FWrite(&nPos,sizeof(nPos),1,f);
      nPos=0;
      m_pLMTCBuffer->Serialize(nPos,pLMLBuffer,true,0,0);*/
/*
      // Make RenderMesh and fill it with texture coordinates
//      m_pLMTCBuffer = GetRenderer()->CreateRenderMeshInitialized(
  //      &vTexCoord3[0], pRenderMesh->GetSysVertCount(), VERTEX_FORMAT_P3F, 
    //    pRenderMesh->GetIndices(0), pRenderMesh->GetIndicesCount(), R_PRIMV_TRIANGLES, "LMapTexCoords", eRMT_Static);

      pPak->FWrite(&m_pLMTCBuffer->GetSysVertCount(),sizeof(m_pLMTCBuffer->GetSysVertCount()),1,f);
      byte * pData = (byte *)m_pLMTCBuffer->GetSysVertBuffer()->m_VS[VSF_GENERAL].m_VData;
      assert(m_VertexSize[m_pLMTCBuffer->GetSysVertBuffer()->m_vertexformat] == 12);
      int nSize = m_VertexSize[m_pLMTCBuffer->GetSysVertBuffer()->m_vertexformat]*m_pLMTCBuffer->GetSysVertCount();
      pPak->FWrite(&m_pLMTCBuffer->GetSysVertCount(),sizeof(m_pLMTCBuffer->GetSysVertCount()),1,f);
    }
    else
    {
      int nPos=0;
      pPak->FWrite(&nPos,sizeof(nPos),1,f);
    }
  }
  else
  {
    pPak->FRead(this,sizeof(*this),1,f);

    if(m_nMaterialId>=0)
    { // restore material
      const char * sMtlName = CBrush::m_lstSExportedBrushMaterials[m_nMaterialId].material;
			m_pMaterial = Get3DEngine()->LoadMaterial( sMtlName );
    }
    else
      m_pMaterial = 0;

    int nSize=0;
    pPak->FRead(&nSize,sizeof(nSize),1,f);

    if(nSize)
    {
      assert(0);
      int nVertCount = 0;
      pPak->FRead(&nVertCount,sizeof(nVertCount),1,f);
      byte * pData = new byte[nVertCount*12];
      pPak->FRead(pData,nVertCount*12,1,f);

      // Make RenderMesh and fill it with texture coordinates
      m_pLMTCBuffer = GetRenderer()->CreateRenderMeshInitialized(
        pData, nVertCount, VERTEX_FORMAT_P3F, 
        0, 0, R_PRIMV_TRIANGLES, "LMapTexCoordsStreamed", eRMT_Static);

      assert(m_VertexSize[m_pLMTCBuffer->GetSysVertBuffer()->m_vertexformat] == 12);
    }
    else
    {
      m_pLMTCBuffer = 0;
      m_pLMData = NULL;
    }
  }

#endif
}*/

void CBrush::GetMemoryUsage(ICrySizer * pSizer)
{
	SIZER_COMPONENT_NAME(pSizer, "Brush");
	pSizer->AddObject(this, sizeof(*this));
}

void CBrush::SetEntityStatObj( unsigned int nSlot, IStatObj * pStatObj, const Matrix34A * pMatrix )
{
  //assert(pStatObj);

	IStatObj *pPrevStatObj = GetStatObj();

  m_pStatObj = (CStatObj*)pStatObj;

  // If object differ we must re-physicalize.
	if (pStatObj != pPrevStatObj)
		if (!pPrevStatObj || !pStatObj || (pStatObj->GetCloneSourceObject()!=pPrevStatObj))// ||
				//(pStatObj->GetFlags() & (STATIC_OBJECT_GENERATED|STATIC_OBJECT_CLONE))==STATIC_OBJECT_CLONE)
			Physicalize();

  if(pMatrix)
    SetMatrix((Matrix34 *)pMatrix);

	if(m_pRNTmpData)
		Get3DEngine()->FreeRNTmpData(&m_pRNTmpData);
	assert(!m_pRNTmpData);

	m_nInternalFlags |= UPDATE_DECALS;
}

IRenderMesh * CBrush::GetRenderMesh(int nLod)
{ 
	IStatObj * pStatObj = GetStatObj() ? GetStatObj()->GetLodObject(nLod) : NULL; 
	return pStatObj ? pStatObj->GetRenderMesh() : NULL;
}

/*
float CBrush::GetLodForDistance(float fDistance)
{
	assert(m_pStatObj!=0);

	float fLodLevel = (float)CObjManager::GetObjectLOD(fDistance, GetLodRatioNormalized(), GetBBox().GetRadius());

	if(fLodLevel > m_pStatObj->m_nLoadedLodsNum-1)
		fLodLevel = (float)m_pStatObj->m_nLoadedLodsNum-1;

	return fLodLevel;
}
*/
void CBrush::OnRenderNodeBecomeVisible()
{
  assert(m_pRNTmpData);
  CRNTmpData::SRNUserData & userData = m_pRNTmpData->userData;

  userData.objMat = m_Matrix;
  userData.fRadius = GetBBox().GetRadius();

  userData.objMatFlags = 0;

  // init pRenderObject

  CRenderObject * & pObj = userData.pRenderObject;

  assert(!pObj);

  pObj = GetRenderer()->EF_GetObject(false);

  pObj->m_pRenderNode = this;
  SRenderObjData *pOD = NULL;
/*  if (m_HMAIndex)
  {
    pOD = GetRenderer()->EF_GetObjData(pObj, true);
    pOD->m_HMAData	=	m_HMAIndex;
  }*/
  pObj->m_II.m_Matrix = userData.objMat;  
  pObj->m_nMotionBlurAmount = 0;
  if(m_pOcNode && m_pOcNode->m_pVisArea)
    pObj->m_II.m_AmbColor = m_pOcNode->m_pVisArea->GetFinalAmbientColor();
  else 
    pObj->m_II.m_AmbColor = Get3DEngine()->GetSkyColor();
  pObj->m_fAlpha = 1.f;
  pObj->m_ObjFlags |= FOB_INSHADOW | FOB_TRANS_MASK;

  if( !userData.objMat.m01 && !userData.objMat.m02 && !userData.objMat.m10 && !userData.objMat.m12 && !userData.objMat.m20 && !userData.objMat.m21 )
    pObj->m_ObjFlags &= ~FOB_TRANS_ROTATE;
  else
    pObj->m_ObjFlags |= FOB_TRANS_ROTATE;

  const Vec3 vCamPos = GetCamera().GetPosition();
  float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,GetBBox()))*m_fZoomFactor;

  pObj->m_nSort = fastround_positive(fEntDistance*2.0f);

  if(GetTerrain()->IsAmbientOcclusionEnabled())
    pObj->m_ObjFlags |= FOB_AMBIENT_OCCLUSION;

  pObj->m_pCurrMaterial = GetMaterial();
  if( uint8 nMaterialLayers = GetMaterialLayers() ) 
  {
    uint8 nFrozenLayer = ( nMaterialLayers&(1<<0) )? 255: 0;
    uint8 nWetLayer = ( nMaterialLayers&(1<<1) )? 255: 0;
    pObj->m_nMaterialLayers = (uint32) (nFrozenLayer<< 24) | (nWetLayer<<16);
  }
  else
    pObj->m_nMaterialLayers = 0;

  userData.nLod = CObjManager::GetObjectLOD(fEntDistance, GetLodRatioNormalized(), userData.fRadius);
}

void CBrush::PreCopyResources()
{
	if(GetCVars()->e_VoxTer && GetIntegrationType())
	{ 
		// force creation of texture system copy
		if(GetMaterial())
		{
			SRayHitInfo hitInfo;
			hitInfo.vHitTangent.x = hitInfo.vHitTangent.w = hitInfo.vHitBinormal.y = 1.f;
			ColorF colSumm = Col_Green;
			Vec3 vSummNorm(0.f,0.f,0.5f);

			CIndexedMesh::ProcessMaterial(GetMaterial(), hitInfo.nHitMatID, hitInfo.vHitColor, hitInfo.vHitTC, 0, colSumm, vSummNorm, hitInfo.vHitTangent, hitInfo.vHitBinormal, 0);
			for(int s=0; s<GetMaterial()->GetSubMtlCount(); s++)
        if(GetMaterial()->GetSubMtl(s))
				  CIndexedMesh::ProcessMaterial(GetMaterial()->GetSubMtl(s), hitInfo.nHitMatID, hitInfo.vHitColor, hitInfo.vHitTC, 0, colSumm, vSummNorm, hitInfo.vHitTangent, hitInfo.vHitBinormal, 0);
		}

		// force render mesh indexing 
		if(IStatObj * pStatObj = GetStatObj() ? GetStatObj()->GetLodObject(0) : NULL)
		{
			if(GetRenderMesh(0))
				GetRenderMesh(0)->GetTrisForPosition(Vec3(0,0,0));

			for(int s=0; s<pStatObj->GetSubObjectCount(); s++)
			{
				IStatObj::SSubObject * pSubObj = pStatObj->GetSubObject(s);

				if (pSubObj->pStatObj && pSubObj->nType == STATIC_SUB_OBJECT_MESH && !pSubObj->bHidden)
				{
					if(pSubObj->pStatObj->GetRenderMesh())
						pSubObj->pStatObj->GetRenderMesh()->GetTrisForPosition(Vec3(0,0,0));
				}
			}
		}
	}
}
