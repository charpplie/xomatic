////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	12/01/2005 - Created by Ivo Herzeg <ivo@crytek.de>
//
//  Contains:
//  start rendering 
/////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <CryHeaders.h>
#include <CryAnimationScriptCommands.h>
#include <Cry_Camera.h>
#include "ModelMesh.h"
#include <I3DEngine.h>
#include <IShader.h>
#include <CryArray.h>
#include "CharacterInstance.h"
#include "CharacterManager.h"
#include "CryCharAnimationParams.h"
#include "CryCharMorphParams.h"
#include "IRenderAuxGeom.h"


//! Render object ( register render elements into renderer )
void CCharInstance::Render(const struct SRendParams& RendParams, const QuatTS& Offset, Matrix34 *pFinalPhysLocation, bool* updated)
{

  if( GetFlags() & CS_FLAG_COMPOUND_BASE )
  {
    if ( Console::GetInst().ca_DrawCC==0)
      return;
  }

  if (m_pModel && m_pModel->m_ObjectType==CGA)
  {
    if ( Console::GetInst().ca_DrawCGA==0)
      return;
  }

  if (!(rp.m_nFlags & CS_FLAG_DRAW_MODEL))
		return;
	assert(RendParams.pMatrix);

	Matrix34 RenderMat34 = (*RendParams.pMatrix);
	
	
	if (m_HadUpdate && (m_SkeletonAnim.m_IsAnimPlaying&1) && m_SkeletonAnim.m_AnimationDrivenMotion)
		RenderMat34=Matrix34(m_PhysEntityLocation);
	m_HadUpdate=0;

	//f32 axisX = RenderMat34.GetColumn0().GetLength();
	//f32 axisY = RenderMat34.GetColumn1().GetLength();
	//f32 axisZ = RenderMat34.GetColumn2().GetLength();
	//f32 fScaling = 0.333333333f*(axisX+axisY+axisZ);
	//RenderMat34.OrthonormalizeFast();
	if (m_SkeletonAnim.m_AnimationDrivenMotion==0)
	{
		RenderMat34.m03+=Offset.t.x;
		RenderMat34.m13+=Offset.t.y;
		RenderMat34.m23+=Offset.t.z;
	}

	Matrix34 PrevRenderMat34 = RenderMat34; 
	if (RendParams.pPrevMatrix)	
    PrevRenderMat34 = (*RendParams.pPrevMatrix);

  bool bUsePrevRenderMat34 = RendParams.pPrevMatrix && ( (m_fFPWeapon!=0) == m_bPrevFrameFPWeapon);

#ifdef ALLOW_CAMERA_SPACE
  if (Console::GetInst().ca_FPWeaponInCamSpace && m_fFPWeapon)
	{
		Matrix34 CamMat = g_pISystem->GetViewCamera().GetMatrix();
		Vec3 WPos = RenderMat34.GetTranslation(); //position of the object in world-space
		Vec3 CPos = CamMat.GetTranslation();
		Vec3 Dif	=	WPos-CPos;
		Matrix33 c33 = Matrix33(CamMat);
		SmoothCD(m_vDifSmooth, m_vDifSmoothRate, m_fOriginalDeltaTime, Dif*c33, m_fFPWeapon);

		//seems to be a fp-weapon
	//	float fColor[4] = {1,0,1,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"m_fFPWeapon: %f    Model: %s ",m_fFPWeapon,  m_pModel->GetFilePath().c_str() );	
	//	g_YLine+=16.0f;

		////overwrite the old world-matrix
		SRendParams* pRendParams = (SRendParams*)&RendParams;
		//pRendParams->pMatrix = &CamMat;								//we take the rotation of the camera
		pRendParams->pMatrix = &RenderMat34;						//Beni - Use weapon matrix, not camera one ;0
		pRendParams->pMatrix->SetTranslation(c33*m_vDifSmooth);   //put the position to ZERO, because we use this object in camera space
		pRendParams->dwFObjFlags |= FOB_CAMERA_SPACE;
		RenderMat34 = (*RendParams.pMatrix);
	}
#endif

	uint32 nFrameID=g_pCharacterManager->m_nUpdateCounter; //g_pIRenderer->GetFrameID(false);
	if (m_LastRenderedFrameID != nFrameID)
	{
		m_LastRenderedFrameID=nFrameID;
		if (rp.m_nFlags & CS_FLAG_UPDATE_ONRENDER)
		{
			//Usually we update characters by the entity-system. If this is not the case, then we need an UpdateOnRender.
			//To make sure we don't update the same object twice, we compare the FrameID with the last UpdateID.
			//if it is bigger then 3 then the object received no update by the entity-system and an UpdateOnRender is required. 
			uint32 dif = m_LastRenderedFrameID-m_LastUpdateFrameID_Pre;
			uint32 bNeedUpdateOnRender = (dif>3);
			//	float fColor[4] = {0,1,0,1};
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"NeedUpdateOnRender: %d    dif: %d    Model: %s",bNeedUpdateOnRender, dif, GetModelFilePath());	g_YLine+=16.0f;
			if (bNeedUpdateOnRender)
			{
				m_pCamera = &(g_pISystem->GetViewCamera());
				Matrix34 ORenderMat34 = RenderMat34; 
				ORenderMat34.OrthonormalizeFast();
				QuatT renderLocation = QuatT(ORenderMat34);
				QuatTS animLocation = renderLocation;// * Offset;
				animLocation.s = Offset.s;

		//		float fColor[4] = {0,1,0,1};
		//		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"fBoidScaling: (%f %f %f)    %f",Offset.t.x,Offset.t.y,Offset.t.z,  Offset.s);	g_YLine+=16.0f;
				SkeletonPreProcess(renderLocation, animLocation, *m_pCamera,1);
				SetPostProcessParameter(renderLocation, animLocation, 0, RendParams.fDistance, 1);
				FinishAnimationComputations();
				if(updated)
					*updated = true;
			}
		}
	}

	g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );


	//-----------------------------------------------------------------------------
	//---              map logical LOD to render LOD                            ---
	//-----------------------------------------------------------------------------


//	const f32 fZoomFactor = 0.5f + 0.5f*(RAD2DEG(g_pIRenderer->GetCamera().GetFov())/60.f);
//	f32 fLodLevel = RendParams.fDistance *fZoomFactor *(fLodRatioNorm*fLodRatioNorm)/(fRadiusRated);
//	f32 fLodLevel = RendParams.npRenderNode->m_pRNTmpData->userData.nLod;
	uint32 nLodLevel = RendParams.nLod;
	if(gEnv->IsEditor() && CSkinInstance::m_bForceLOD)
		nLodLevel = CSkinInstance::m_ForcedLODNum;
  if(!gEnv->p3DEngine->GetRenderIntoShadowmap())
	  m_nAnimationLOD = nLodLevel;

	m_nRenderLOD = nLodLevel;
	static ICVar* p_e_lod_min = gEnv->pConsole->GetCVar("e_LodMin");
	if (p_e_lod_min)
		m_nRenderLOD += p_e_lod_min->GetIVal();

	int32 numLODs = m_pModel->m_arrModelMeshes.size();
	if(m_nRenderLOD<m_pModel->m_nBaseLOD)
		m_nRenderLOD=m_pModel->m_nBaseLOD;
	if(m_nRenderLOD>=numLODs && numLODs > 0)
		m_nRenderLOD = max(0,numLODs-1);

	m_nRenderLOD = clamp_tpl(m_nRenderLOD,(int32)0,(int32)g_nMaxGeomLodLevels-1);

	if (!g_pI3DEngine->GetRenderIntoShadowmap())
		m_nMorphTargetLod = m_nRenderLOD;

	//------------------------------------------------------------------------

/*
	if ( Console::GetInst().ca_DrawPositionPost ) 	
	{
		Vec3 wpos = RenderMat34.GetTranslation();
		g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
		static Ang3 angle(0,0,0); 
		angle += Ang3(0.01f,0.02f,0.03f);
		AABB aabb = AABB(Vec3(-0.055f,-0.055f,-0.055f),Vec3(+0.055f,+0.055f,+0.055f));
		OBB obb=OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle),aabb );
		g_pAuxGeom->DrawOBB(obb,wpos,0,RGBA8(0x00,0xff,0x00,0xff), eBBD_Extremes_Color_Encoded);

		Vec3 axisX = RenderMat34.GetColumn0();
		Vec3 axisY = RenderMat34.GetColumn1();
		Vec3 axisZ = RenderMat34.GetColumn2();
		g_pAuxGeom->DrawLine(wpos,RGBA8(0x7f,0x00,0x00,0x00), wpos+axisX,RGBA8(0xff,0x00,0x00,0x00) );
		g_pAuxGeom->DrawLine(wpos,RGBA8(0x00,0x7f,0x00,0x00), wpos+axisY,RGBA8(0x00,0xff,0x00,0x00) );
		g_pAuxGeom->DrawLine(wpos,RGBA8(0x00,0x00,0x7f,0x00), wpos+axisZ,RGBA8(0x00,0x00,0xff,0x00) );
	}*/

	if (m_pModel->m_ObjectType==CGA)
  {
    Matrix34 mRendMat34 = RenderMat34*Matrix34(Offset);

    if (RendParams.pPrevMatrix && pFinalPhysLocation)	
      (*pFinalPhysLocation) = mRendMat34;

    if( !bUsePrevRenderMat34 )
      PrevRenderMat34 = mRendMat34;

    RenderCGA (RendParams, mRendMat34, PrevRenderMat34);
  }
	else
	{
		pe_params_flags pf;

		if (m_pSkinAttachment)
		{
			//this is a skin-instance. We need to access the skeleton of the master 
			//	CAttachment* pAttachment	= (CAttachment*)m_pIMasterAttachment;
			//	CCharInstance* pIMasterInstance	=	pAttachment->m_pAttachmentManager->m_pCharInstance;

			//	IPhysicalEntity* pCharPhys = pIMasterInstance->m_SkeletonPose.GetCharacterPhysics();
			//	if (pCharPhys && pCharPhys->GetType()==PE_ARTICULATED && pCharPhys->GetParams(&pf) && pf.flags & aef_recorded_physics)
			//		RenderMat34 = RenderMat34*Matrix34(Offset);
      if (RendParams.pPrevMatrix && pFinalPhysLocation)	
        (*pFinalPhysLocation) = RenderMat34;

      if( !bUsePrevRenderMat34 )
        PrevRenderMat34 = RenderMat34;
      // at min for decals
			int nList = (int)g_pIRenderer->EF_Query(EFQ_MainThreadList);
      uint32 numRemapJoints	= m_pSkinAttachment->m_arrRemapTable.size();
//      uint32 iActiveThis = this->m_iActiveFrame;
  //    uint32 iActiveFrameMaster	=	m_pSkinAttachment->m_pAttachmentManager->m_pSkelInstance->m_iActiveFrame;
      for (uint32 i=0; i<numRemapJoints; i++)
      {
        int index = m_pSkinAttachment->m_arrRemapTable[i];
        this->m_arrNewSkinQuat[nList][i] = m_pSkinAttachment->m_pAttachmentManager->m_pSkelInstance->m_arrNewSkinQuat[nList][index];
      }
      //
			RenderCHR (RendParams, RenderMat34, PrevRenderMat34);
		} 
		else  //Diesel cut
		{
			IPhysicalEntity* pCharPhys = m_SkeletonPose.GetCharacterPhysics();
			if (pCharPhys && pCharPhys->GetType()==PE_ARTICULATED && pCharPhys->GetParams(&pf) && pf.flags & aef_recorded_physics)
				RenderMat34 = RenderMat34*Matrix34(Offset);

			//	float fColor[4] = {0,1,0,1};
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"Offset: (%f %f %f)",Offset.t.x,Offset.t.y,Offset.t.z);	
			//	g_YLine+=16.0f;

      if (RendParams.pPrevMatrix && pFinalPhysLocation)	
        (*pFinalPhysLocation) = RenderMat34;

      if( !bUsePrevRenderMat34 )
        PrevRenderMat34 = RenderMat34;

			RenderCHR (RendParams, RenderMat34, PrevRenderMat34);
		} //*/ //Diesel cut
	}

	// draw weapon and binded objects
//Diesel cut	
	m_AttachmentManager.DrawAttachments(RendParams,RenderMat34);

  if ( Console::GetInst().ca_FPWeaponInCamSpace && m_fFPWeapon )
    m_bPrevFrameFPWeapon = true;
  else
    m_bPrevFrameFPWeapon = false;
}



void CCharInstance::RenderCGA(const struct SRendParams& RendParams, const Matrix34& RenderMat34, const Matrix34& PrevRenderMat34 )
{
  if( Console::GetInst().ca_DrawCGAAsSkin==1 && m_pRenderMeshs[0] )
  {
		int nList = (int)g_pIRenderer->EF_Query(EFQ_MainThreadList);
    SRendParams nodeRP = RendParams;
    if( nodeRP.pMaterial==NULL )
    {
      nodeRP.pMaterial = m_pModel->GetMaterial();
    }
    uint32 numJoints = m_SkeletonPose.GetPoseData().GetJointCount();
    assert(numJoints);
    for (uint32 i=0; i<numJoints; i++)
    {
      //SJoint* joint = &m_SkeletonPose.m_arrJoints[i];
      if (m_SkeletonPose.m_arrCGAJoints.size() &&  m_SkeletonPose.m_arrCGAJoints[i].m_CGAObjectInstance)
      {      
        const QuatT& skinTr = m_SkeletonPose.GetPoseData().m_jointsAbsolute[i];
        m_arrNewSkinQuat[nList][i] = skinTr;
      }
    }
    RenderCHR(nodeRP,RenderMat34,PrevRenderMat34);
    return;
  }
	/*
	const char* mname = GetModelFilePath();
	if ( strcmp(mname,"objects/library/architecture/multiplayer/small_vehicle_factory/small_factory_gates.cga")==0 )
	{
	uint32 sdsd=0;
	float fColor[4] = {0,1,0,1};
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"RenderMat34: %f %f %f %f",RenderMat34.m00,RenderMat34.m01,RenderMat34.m02,RenderMat34.m03 );	
	g_YLine+=10.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"RenderMat34: %f %f %f %f",RenderMat34.m10,RenderMat34.m11,RenderMat34.m12,RenderMat34.m13 );	
	g_YLine+=10.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"RenderMat34: %f %f %f %f",RenderMat34.m20,RenderMat34.m21,RenderMat34.m22,RenderMat34.m23 );	
	g_YLine+=10.0f;
	}*/

  if(!m_arrNewSkinQuat[0].size())
    return;

	SRendParams nodeRP = RendParams;
	f32 fScale = m_arrNewSkinQuat[0][0].s;

	Matrix34 ortho_tm34 = RenderMat34;
//	ortho_tm34.OrthonormalizeFast();
	Matrix34 ortho_tm34prev = PrevRenderMat34;
//	ortho_tm34prev.OrthonormalizeFast();

//	f32 axisX = RenderMat34.GetColumn0().GetLength();
//	f32 axisY = RenderMat34.GetColumn1().GetLength();
//	f32 axisZ = RenderMat34.GetColumn2().GetLength();
//	f32 fScaling = 0.333333333f*(axisX+axisY+axisZ);
//	float fColor[4] = {0,1,0,1};
//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"fScaling: %f    fScale: %f",fScaling,  fScale);	g_YLine+=16.0f;


	IMaterial* pMaterial = nodeRP.pMaterial;
	uint32 numJoints = m_SkeletonPose.GetPoseData().GetJointCount();
	assert(numJoints);
	if ( Console::GetInst().ca_DrawBaseMesh)
	{
		for (uint32 i=0; i<numJoints; i++)
		{
			//SJoint* joint = &m_SkeletonPose.m_arrJoints[i];
			if (m_SkeletonPose.m_arrCGAJoints.size() &&  m_SkeletonPose.m_arrCGAJoints[i].m_CGAObjectInstance)
			{      
				Matrix34 tm34 = ortho_tm34*Matrix34(m_SkeletonPose.GetPoseData().m_jointsAbsolute[i]);
				//	tm34.m00*=fScale;	tm34.m01*=fScale;	tm34.m02*=fScale;
				//	tm34.m10*=fScale;	tm34.m11*=fScale;	tm34.m12*=fScale;
				//	tm34.m20*=fScale;	tm34.m21*=fScale;	tm34.m22*=fScale;
				nodeRP.pMatrix = &tm34;

				Matrix34 tm34prev;      
				if( nodeRP.pPrevMatrix )
				{
					tm34prev = ortho_tm34prev*Matrix34(m_SkeletonPose.GetPoseData().m_jointsAbsolute[i]);
				//	tm34prev.m00*=fScale;	tm34prev.m01*=fScale;	tm34prev.m02*=fScale;
				//	tm34prev.m10*=fScale;	tm34prev.m11*=fScale;	tm34prev.m12*=fScale;
				//	tm34prev.m20*=fScale;	tm34prev.m21*=fScale;	tm34prev.m22*=fScale;
					nodeRP.pPrevMatrix = &tm34prev;
				}      
				nodeRP.dwFObjFlags |= FOB_TRANS_MASK;
				nodeRP.ppRNTmpData = &m_SkeletonPose.m_arrCGAJoints[i].m_pRNTmpData;      
      
				// apply custom joint material, if set
				nodeRP.pMaterial = m_SkeletonPose.m_arrCGAJoints[i].m_pMaterial ? m_SkeletonPose.m_arrCGAJoints[i].m_pMaterial.get() : pMaterial;
      
        if(tm34.IsValid())
				  m_SkeletonPose.m_arrCGAJoints[i].m_CGAObjectInstance->Render(nodeRP);
        else
          AnimWarning("CCharInstance::RenderCGA: object has invalid matrix: %s", m_pModel->GetModelFilePath() );
			}
		}
	}

	if (Console::GetInst().ca_DrawSkeleton)
		m_SkeletonPose.DrawSkeleton( RenderMat34 );

	if ( Console::GetInst().ca_DrawBBox ) 
		m_SkeletonPose.DrawBBox( RenderMat34 );
}



void CCharInstance::RenderCHR(const SRendParams& RendParams, const Matrix34& rRenderMat34, const Matrix34& PrevRenderMat34)
{
	CRenderObject* pObj = g_pIRenderer->EF_GetObject(true);
	if (!pObj)
		return;

	pObj->m_nLod	=	m_nRenderLOD;

	/*float fColor[4] = {1,0,1,1};
	extern f32 g_YLine;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"attachment LOD: %d", pObj->m_nLod  ); g_YLine+=0x10;*/

	pObj->m_fSort	= RendParams.fCustomSortOffset;
	pObj->m_ObjFlags |= FOB_TRANS_MASK;
#ifdef ALLOW_CAMERA_SPACE
	if (RendParams.dwFObjFlags & FOB_CAMERA_SPACE)
	{
		pObj->m_ObjFlags |= FOB_CAMERA_SPACE;
	}
#endif
	//check if it should be drawn close to the player
	if ((RendParams.dwFObjFlags & FOB_NEAREST) || (rp.m_nFlags & CS_FLAG_DRAW_NEAR) )	{ 	pObj->m_ObjFlags|=FOB_NEAREST;	}
	else pObj->m_ObjFlags&=~FOB_NEAREST;

	pObj->m_fAlpha = RendParams.fAlpha;
	pObj->m_fDistance =	RendParams.fDistance;

	pObj->m_II.m_AmbColor = RendParams.AmbientColor;


	pObj->m_ObjFlags |= RendParams.dwFObjFlags;
	SRenderObjData *pD = g_pIRenderer->EF_GetObjData(pObj, true);

	pObj->m_II.m_Matrix = rRenderMat34;    
  pObj->m_nMotionBlurAmount = 0;
  
	if( RendParams.pPrevMatrix && (Console::GetInst().m_pMotionBlur->GetIVal()> 1 || (RendParams.nMaterialLayersBlend & MTL_LAYER_BLEND_CLOAK)))
  {
    bool bCheckMotion = MotionBlurMotionCheck( pObj->m_ObjFlags );
    if( bCheckMotion || !pObj->m_II.m_Matrix.IsEquivalent(PrevRenderMat34, 0.001f) )
    {
      pD->m_prevMatrix = PrevRenderMat34;
      pObj->m_nMotionBlurAmount = 128;
    }
  }

  if (RendParams.nVisionParams && !g_pI3DEngine->GetRenderIntoShadowmap() && !(pObj->m_ObjFlags & FOB_VEGETATION))
  {
    pObj->m_nVisionParams = RendParams.nVisionParams;
  }
  
 //Diesel cut
  pObj->m_ObjFlags |= (RendParams.dwFObjFlags & FOB_SHADOW_DISSOLVE);
//Diesel new
 //pObj->m_ObjFlags |= (RendParams.dwFObjFlags);

	pObj->m_nMaterialLayers = RendParams.nMaterialLayersBlend;

	if(RendParams.pTerrainTexInfo && (RendParams.dwFObjFlags & (FOB_BLEND_WITH_TERRAIN_COLOR | FOB_AMBIENT_OCCLUSION)))
	{
		pObj->m_nTextureID = RendParams.pTerrainTexInfo->nTex0;
		//pObj->m_nTextureID1 = RendParams.pTerrainTexInfo->nTex1;
		pD->m_fTempVars[0] = RendParams.pTerrainTexInfo->fTexOffsetX;
		pD->m_fTempVars[1] = RendParams.pTerrainTexInfo->fTexOffsetY;
		pD->m_fTempVars[2] = RendParams.pTerrainTexInfo->fTexScale;
		pD->m_fTempVars[3] = RendParams.pTerrainTexInfo->fTerrainMinZ;
		pD->m_fTempVars[4] = RendParams.pTerrainTexInfo->fTerrainMaxZ;
	}

  pObj->m_DissolveRef = RendParams.nDissolveRef;
	pObj->m_pShadowCasters = RendParams.pShadowMapCasters;

	pD->m_pCharInstance = this;
	pObj->m_ObjFlags |= FOB_CHARACTER | FOB_INSHADOW;

  int nList = (int)g_pIRenderer->EF_Query(EFQ_MainThreadList);
	pObj->m_DynLMMask[nList] = RendParams.nDLightMask;

	pObj->m_nSort = fastround_positive(RendParams.fDistance * 2.0f);

	if (m_HideMaster==0) 
	{
		//	AddCurrentRenderData (pObj, RendParams);
		IRenderMesh* pRenderMesh = m_pRenderMeshs[m_nRenderLOD];
		if (pRenderMesh==0)
			AnimWarning("model '%s' has no render mesh", m_pModel->GetModelFilePath() );

		if (pRenderMesh)
		{       
			// MichaelS - use the instance's material if there is one, and if no override material has
			// already been specified.
			IMaterial* pMaterial = RendParams.pMaterial;
			if (!pMaterial && this->GetMaterialOverride())
				pMaterial = this->GetMaterialOverride();

      if(!pMaterial)
        pMaterial = m_pModel->GetMaterial();

			CModelMesh* pModelMesh = m_pModel->GetModelMesh(m_nRenderLOD);
			static ICVar *p_e_debug_draw = gEnv->pConsole->GetCVar("e_DebugDraw");
			if(p_e_debug_draw && p_e_debug_draw->GetIVal() > 0)
				pModelMesh->DrawDebugInfo(this, m_nRenderLOD, rRenderMat34, p_e_debug_draw->GetIVal(), pMaterial, pObj, RendParams );

			//	float fColor[4] = {1,0,1,1};
			//	extern f32 g_YLine;
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"m_nRenderLOD: %d  %d",RendParams.nLodLevel, pObj->m_nLod  ); g_YLine+=0x10;

//Diesel cut
			if ( Console::GetInst().ca_DrawBaseMesh)
			{
				pRenderMesh->Render(RendParams,pObj,pMaterial);				
				if (!g_pI3DEngine->GetRenderIntoShadowmap())	AddDecalsToRenderer(this,0,rRenderMat34);
			}

			g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
			if (Console::GetInst().ca_DrawMotionBlurTest)
				MotionBlurTest(0,rRenderMat34);
			if (Console::GetInst().ca_DrawDecalsBBoxes)
				DrawDecalsBBoxes(this,0, rRenderMat34 );
			if (Console::GetInst().ca_DrawSkeleton)
				m_SkeletonPose.DrawSkeleton( rRenderMat34 );
			if ( Console::GetInst().ca_DrawBBox ) 
				m_SkeletonPose.DrawBBox( rRenderMat34 );
		}
	}

}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

//#ifdef MOTION_BLUR_TEST
void CCharInstance::MotionBlurTest(int nLOD, const Matrix34& rRenderMat34 )
{
/*
  int nFrameTimeScale = Console::GetInst().m_pMotionBlurFrameTimeScale->GetIVal();
  float fShutterSpeed = Console::GetInst().m_pMotionBlurShutterSpeed->GetFVal();

  float fExposureTime = fShutterSpeed * 0.4f;
  float fAlpha = fExposureTime / gEnv->pTimer->GetFrameTime();  
  if( nFrameTimeScale )
  {
    float fAlphaScale = min(1.0f, (1.0f / gEnv->pTimer->GetFrameTime()) / ( 32.0f)); // attenuate motion blur for lower frame rates
    fAlpha *= fAlphaScale;
  }

  uint32 numBones=m_arrMBSkinQuat[0].size();
  for (uint32 i=0; i<numBones; i++)
   m_arrMBSkinQuat[0][i].SetNLerp(m_arrNewSkinQuat[0][i],m_arrNewSkinQuat[0][m_iBackFrame][i], fAlpha);

	CModelMesh* pModelMesh = m_pModel->GetModelMesh(nLOD);

	assert(pModelMesh->m_nLOD==nLOD);
	pModelMesh->InitSkinningExtSW(&this->m_Morphing,0,m_ResetMode,nLOD);
  
	uint32 numExtIndices	= pModelMesh->m_pModel->GetRenderMesh(0)->GetIndicesCount();//pModelMesh->m_arrExtFaces.size()*3;
	uint32 numExtVertices	=	pModelMesh->m_pModel->GetRenderMesh(0)->GetVerticesCount();
	assert(numExtVertices);

	static std::vector<Vec3> arrExtSkinnedStream;
	uint32 vsize = arrExtSkinnedStream.size();
	if (vsize<numExtVertices) arrExtSkinnedStream.resize( numExtVertices );	


	pModelMesh->LockFullRenderMesh(pModelMesh->m_pModel->m_nBaseLOD);
	if (Console::GetInst().ca_SphericalSkinning==0) 
	{
		for(uint32 e=0; e<numExtVertices; e++) 
		{	
			//create the final vertex for skinning (blend between 3 characters)
			ExtSkinVertex vertex = pModelMesh->GetSkinVertex(e);
			uint8 idx	=	vertex.color.a; 
			Vec3 v		= vertex.wpos0*pModelMesh->VertexRegs[idx].x + vertex.wpos1*pModelMesh->VertexRegs[idx].y + vertex.wpos2*pModelMesh->VertexRegs[idx].z;

			//get indices for bones (always 4 indices per vertex)
			uint32 id0 = vertex.boneIDs[0];
			uint32 id1 = vertex.boneIDs[1];
			uint32 id2 = vertex.boneIDs[2];
			uint32 id3 = vertex.boneIDs[3];
			f32 w0 = vertex.weights[0]/255.0f;
			f32 w1 = vertex.weights[1]/255.0f;
			f32 w2 = vertex.weights[2]/255.0f;
			f32 w3 = vertex.weights[3]/255.0f;

			Vec3 v0=m_arrMBSkinQuat[0][id0]*v*w0;
			Vec3 v1=m_arrMBSkinQuat[0][id1]*v*w1;
			Vec3 v2=m_arrMBSkinQuat[0][id2]*v*w2;
			Vec3 v3=m_arrMBSkinQuat[0][id3]*v*w3;
			Vec3 final = (v0+v1+v2+v3);
			arrExtSkinnedStream[e] = rRenderMat34*final;
		}

		SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
		renderFlags.SetFillMode( e_FillModeWireframe );
		renderFlags.SetDrawInFrontMode( e_DrawInFrontOn );
		g_pAuxGeom->SetRenderFlags( renderFlags );
		g_pAuxGeom->DrawTriangles(&arrExtSkinnedStream[0],numExtVertices, pModelMesh->m_pIndices,numExtIndices, RGBA8(0x00,0x70,0x00,0x00) );		

		//-------------------------------------------------------------------------------------------
		//-------------------------------------------------------------------------------------------
		//-------------------------------------------------------------------------------------------

		for(uint32 e=0; e<numExtVertices; e++) 
		{	
			//create the final vertex for skinning (blend between 3 characters)
			ExtSkinVertex vertex = pModelMesh->GetSkinVertex(e);
			uint8 idx	=	vertex.color.a; 
			Vec3 v		= vertex.wpos0*pModelMesh->VertexRegs[idx].x + vertex.wpos1*pModelMesh->VertexRegs[idx].y + vertex.wpos2*pModelMesh->VertexRegs[idx].z;

			//get indices for bones (always 4 indices per vertex)
			uint32 id0 = vertex.boneIDs[0];
			uint32 id1 = vertex.boneIDs[1];
			uint32 id2 = vertex.boneIDs[2];
			uint32 id3 = vertex.boneIDs[3];

			f32 w0 = vertex.weights[0]/255.0f;
			f32 w1 = vertex.weights[1]/255.0f;
			f32 w2 = vertex.weights[2]/255.0f;
			f32 w3 = vertex.weights[3]/255.0f;

			Vec3 v0=m_arrNewSkinQuat[0][id0]*v*w0;
			Vec3 v1=m_arrNewSkinQuat[0][id1]*v*w1;
			Vec3 v2=m_arrNewSkinQuat[0][id2]*v*w2;
			Vec3 v3=m_arrNewSkinQuat[0][id3]*v*w3;
			Vec3 final = (v0+v1+v2+v3);
			arrExtSkinnedStream[e] = rRenderMat34*final;
		}

		renderFlags.SetFillMode( e_FillModeWireframe );
		renderFlags.SetDrawInFrontMode( e_DrawInFrontOn );
		g_pAuxGeom->SetRenderFlags( renderFlags );
		g_pAuxGeom->DrawTriangles(&arrExtSkinnedStream[0],numExtVertices, pModelMesh->m_pIndices,numExtIndices, RGBA8(0xff,0x00,0x00,0x00) );		
	}
	else
	{
		for(uint32 e=0; e<numExtVertices; e++) 
		{	
			//create the final vertex for skinning (blend between 3 characters)
			ExtSkinVertex vertex = pModelMesh->GetSkinVertex(e);
			uint8 idx	=	vertex.color.a; 
			Vec3 v		= vertex.wpos0*pModelMesh->VertexRegs[idx].x + vertex.wpos1*pModelMesh->VertexRegs[idx].y + vertex.wpos2*pModelMesh->VertexRegs[idx].z;

			//get indices for bones (always 4 indices per vertex)
			uint32 id0 = vertex.boneIDs[0];
			uint32 id1 = vertex.boneIDs[1];
			uint32 id2 = vertex.boneIDs[2];
			uint32 id3 = vertex.boneIDs[3];

			f32 w0 = vertex.weights[0]/255.0f;
			f32 w1 = vertex.weights[1]/255.0f;
			f32 w2 = vertex.weights[2]/255.0f;
			f32 w3 = vertex.weights[3]/255.0f;

			Quat wquat	=	(m_arrMBSkinQuat[0][id0].q*w0 + 
				m_arrMBSkinQuat[0][id1].q*w1 + 
				m_arrMBSkinQuat[0][id2].q*w2 + 
				m_arrMBSkinQuat[0][id3].q*w3).GetNormalized();
			Vec3 bv			=	v;
			Vec3 sv			= wquat*bv + m_arrMBSkinQuat[0][id0].t;
			arrExtSkinnedStream[e] = rRenderMat34*sv;
		}
		SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
		renderFlags.SetFillMode( e_FillModeWireframe );
		renderFlags.SetDrawInFrontMode( e_DrawInFrontOn );
		g_pAuxGeom->SetRenderFlags( renderFlags );
		g_pAuxGeom->DrawTriangles(&arrExtSkinnedStream[0],numExtVertices, pModelMesh->m_pIndices,numExtIndices, RGBA8(0x00,0x70,0x00,0x00) );		

		//-------------------------------------------------------------------------------------------
		for(uint32 e=0; e<numExtVertices; e++) 
		{	
			//create the final vertex for skinning (blend between 3 characters)
			ExtSkinVertex vertex = pModelMesh->GetSkinVertex(e);
			uint8 idx	=	vertex.color.a; 
			Vec3 v		= vertex.wpos0*pModelMesh->VertexRegs[idx].x + vertex.wpos1*pModelMesh->VertexRegs[idx].y + vertex.wpos2*pModelMesh->VertexRegs[idx].z;

			//get indices for bones (always 4 indices per vertex)
			uint32 id0 = vertex.boneIDs[0];
			uint32 id1 = vertex.boneIDs[1];
			uint32 id2 = vertex.boneIDs[2];
			uint32 id3 = vertex.boneIDs[3];

			f32 w0 = vertex.weights[0]/255.0f;
			f32 w1 = vertex.weights[1]/255.0f;
			f32 w2 = vertex.weights[2]/255.0f;
			f32 w3 = vertex.weights[3]/255.0f;

			Quat wquat = (
				m_arrNewSkinQuat[0][id0].q*w0 + 
				m_arrNewSkinQuat[0][id1].q*w1 + 
				m_arrNewSkinQuat[0][id2].q*w2 + 
				m_arrNewSkinQuat[0][id3].q*w3).GetNormalized();
			Vec3 bv =	v;
			Vec3 sv = wquat*bv + m_arrNewSkinQuat[0][id0].t;
			arrExtSkinnedStream[e] = rRenderMat34*sv;
		}
		pModelMesh->UnlockFullRenderMesh(pModelMesh->m_pModel->m_nBaseLOD);
		renderFlags.SetFillMode( e_FillModeWireframe );
		renderFlags.SetDrawInFrontMode( e_DrawInFrontOn );
		g_pAuxGeom->SetRenderFlags( renderFlags );
		g_pAuxGeom->DrawTriangles(&arrExtSkinnedStream[0],numExtVertices, pModelMesh->m_pIndices,numExtIndices, RGBA8(0xff,0x00,0x00,0x00) );		
	}
*/
}
//#endif

bool CCharInstance::MotionBlurMotionCheck( uint32 nObjFlags )
{
/*
  int nList = (int)g_pIRenderer->EF_Query(EFQ_MainThreadList);
  if( m_pSkinAttachment )
  {
    CCharInstance* pMaster	=	m_pSkinAttachment->m_pAttachmentManager->m_pSkelInstance;
    if( pMaster )
      return pMaster->MotionBlurMotionCheck( 0 );

    return false;
  }

  uint32 numBones = m_arrNewSkinQuat[nList][m_iActiveFrame].size();
  if (!numBones || m_iActiveFrame == m_iBackFrame ) // no updates
    return false;

  float fMovementAmount = 0.0f;
  for (uint32 i=0; i<numBones; ++i)
  {
		assert(nList>=0 && nList<2);
		assert(m_iActiveFrame>=0 && m_iActiveFrame<MAX_MOTIONBLUR_FRAMES);
		assert(m_iBackFrame>=0 && m_iBackFrame<MAX_MOTIONBLUR_FRAMES);
		QuatTS q0 = m_arrNewSkinQuat[nList][i];
    QuatTS q1 = m_arrNewSkinQuat[nList][m_iBackFrame][i];
    fMovementAmount += 1.0f - max(0.0f, q0.q.v.x * q1.q.v.x + q0.q.v.y * q1.q.v.y + q0.q.v.z * q1.q.v.z + q0.q.w * q1.q.w);
  }

  const float fMinMovementThreshold = 0.001f;
  if( fMovementAmount > fMinMovementThreshold )
      return true;
*/
  return false;
}

void CCharInstance::UpdatePreviousFrameBones(int nLOD)
{
  int nFrameTimeScale = Console::GetInst().m_pMotionBlurFrameTimeScale->GetIVal();
  float fShutterSpeed = Console::GetInst().m_pMotionBlurShutterSpeed->GetFVal();

  float fExposureTime = fShutterSpeed * 0.8f;
	const float dt = gEnv->pTimer->GetFrameTime();
	float fAlpha = iszero(dt) ? 0.0f : fExposureTime / dt;  
  if( nFrameTimeScale )
  {
    float fAlphaScale = iszero(dt) ? 1.0f : min(1.0f, (1.0f / dt) / ( 32.0f)); // attenuate motion blur for lower frame rates
    fAlpha *= fAlphaScale;
  }
  int nList = (int)g_pIRenderer->EF_Query(EFQ_MainThreadList);
#ifdef MOTIONBLUR_QUATS
  uint32 numBones = m_arrMBSkinQuat[nList].size();
	if (!numBones)
		m_arrMBSkinQuat[nList].resize(m_arrNewSkinQuat[nList].size());
#endif
/*  if( m_iActiveFrame != m_iBackFrame )
  {
		assert(nList>=0 && nList<2);
		assert(m_iActiveFrame>=0 && m_iActiveFrame<MAX_MOTIONBLUR_FRAMES);
		assert(m_iBackFrame>=0 && m_iBackFrame<MAX_MOTIONBLUR_FRAMES);
    for (uint32 i=0; i<numBones; ++i)
      m_arrMBSkinQuat[nList][i].SetNLerp(m_arrNewSkinQuat[nList][m_iActiveFrame][i],m_arrNewSkinQuat[nList][m_iBackFrame][i], fAlpha);
  }
  else
*/  {
    // if skeleton not being updated use only current frame 
		assert(nList>=0 && nList<2);
//		assert(m_iActiveFrame>=0 && m_iActiveFrame<MAX_MOTIONBLUR_FRAMES);
#ifdef MOTIONBLUR_QUATS
    for (uint32 i=0; i<numBones; ++i)
      m_arrMBSkinQuat[nList][i] = m_arrNewSkinQuat[nList][i];
#endif
  }
}





void CCharInstance::SetColor(float fR, float fG, float fB, float fA)
{
	rp.m_Color.r = fR;
	rp.m_Color.g = fG;
	rp.m_Color.b = fB;
	rp.m_Color.a = fA;
}

void CSkinInstance::AddCurrentRenderData(CRenderObject *pObj, const SRendParams &pParams)
{
	int nSort = pParams.nRenderList;

	// prevent from render list changing
	// todo: use define here
	int nLod = m_nRenderLOD;
	pObj->m_nLod = nLod;

	IRenderMesh* pRenderMesh = m_pRenderMeshs[nLod];

//	pRenderMesh->SetBBox(m_AABB.min,m_AABB.max);
	pRenderMesh->SetBBox(Vec3(-0.3f,-0.3f,-0.3f),Vec3(+0.3f,+0.3f,+0.3f) );
	IMaterial *pMaterial = pParams.pMaterial;
	if (!pMaterial)
  {
		pMaterial = m_pModel->GetMaterial();
  }
    
  //////////////////////////////////////////////////////////////////////////////////////////////////

	assert(pMaterial && "Material must always be valid here");

	CModelMesh* pSkinning = &m_pModel->m_arrModelMeshes[nLod];
    
  pObj->m_pCurrMaterial = pMaterial;
  pObj->m_nMaterialLayers = pParams.nMaterialLayersBlend;

	if(pParams.pTerrainTexInfo && (pParams.dwFObjFlags & (FOB_BLEND_WITH_TERRAIN_COLOR | FOB_AMBIENT_OCCLUSION)))
	{
		pObj->m_nTextureID = pParams.pTerrainTexInfo->nTex0;
		//pObj->m_nTextureID1 = pParams.pTerrainTexInfo->nTex1;
    SRenderObjData *pD = g_pIRenderer->EF_GetObjData(pObj, true);
		pD->m_fTempVars[0] = pParams.pTerrainTexInfo->fTexOffsetX;
		pD->m_fTempVars[1] = pParams.pTerrainTexInfo->fTexOffsetY;
		pD->m_fTempVars[2] = pParams.pTerrainTexInfo->fTexScale;
		pD->m_fTempVars[3] = pParams.pTerrainTexInfo->fTerrainMinZ;
		pD->m_fTempVars[4] = pParams.pTerrainTexInfo->fTerrainMaxZ;
	}

  PodArray<CRenderChunk> &Chnks = pRenderMesh->GetChunks();

	for(uint32 i=0; i<Chnks.size(); i++)    
	{ 
		CRenderChunk *pChunk = &pRenderMesh->GetChunks()[i];
		SShaderItem si;

		// Override object material.
		si = pMaterial->GetShaderItem(pChunk->m_nMatID);

		CREMesh * pREOcLeaf  = pChunk->pRE;
	  if (si.m_pShader && pREOcLeaf)
		{
      //// Get material layer manager presets from material resources
      //IMaterial *pCurrMtl = ( pMaterial->GetSubMtlCount() )? pMaterial->GetSubMtl( pChunk->m_nMatID ) : pMaterial;
      //if(pCurrMtl)
      //{
      //  si.m_pShaderResources->m_pMaterialLayerManager = pCurrMtl->GetMaterialLayerManager();          
      //} 

			g_pIRenderer->EF_AddEf(pREOcLeaf, si, pObj, nSort);
		}
	}

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void CSkinInstance::AddDecalsToRenderer( CCharInstance* pMaster, CAttachment* pAttachment, const Matrix34& RenderMat34 )
{
	if (m_nRenderLOD == 0)
	{
		/*QuatTS arrNewSkinQuat[MAX_JOINT_AMOUNT+1];	//bone quaternions for skinning (entire skeleton)
		if(m_pSkinAttachment)
		{
			IAttachmentObject* obj = m_pSkinAttachment->GetIAttachmentObject();
			CCHRAttachment* pobj = static_cast<CCHRAttachment*>(obj);
			if(iActiveFrame >=MAX_MOTIONBLUR_FRAMES)
				CryError("AddDecalsToRenderer crash %s, master: %s, iActiveFrame", pobj->GetICharacterInstance()->GetFilePath(), pMaster->GetFilePath());
			uint32 numRemapJoints	= m_pSkinAttachment->m_arrRemapTable.size();
			if(numRemapJoints > MAX_JOINT_AMOUNT)
				CryError("AddDecalsToRenderer crash %s, master: %s numRemapJoints", pobj->GetICharacterInstance()->GetFilePath(), pMaster->GetFilePath());
			uint32 m_size = pMaster->m_arrNewSkinQuat[iActiveFrame].size();
			for (uint32 i=0; i<numRemapJoints; i++)
			{
				uint32 id = m_pSkinAttachment->m_arrRemapTable[i];
				if( id >= m_size)
					CryError("AddDecalsToRenderer crash %s, master: %s, remapId", pobj->GetICharacterInstance()->GetFilePath(), pMaster->GetFilePath());
				arrNewSkinQuat[i] = pMaster->m_arrNewSkinQuat[iActiveFrame][id];
			}
		}*/

		// TODO: optimize?
		m_DecalManager.RemoveObsoleteDecals();
		size_t numDecals = m_DecalManager.m_arrDecals.size();
		if (!numDecals)
			return;

//		uint32 iActiveFrame		=	pMaster->m_iActiveFrame;
		//IMaterialManager* pMatMan = g_pI3DEngine->GetMaterialManager();
		//IMaterial* pDefaultDecalMaterial( pMatMan->LoadMaterial( "Materials/Decals/CharacterDecal" ) );
		IMaterial* pMaterial;// = pDefaultDecalMaterial;
		//if( pDefaultDecalMaterial )
		//	pDefaultDecalMaterial = g_pI3DEngine->GetMaterialManager()->GetDefaultMaterial();
		//assert( pDefaultDecalMaterial );

		for( size_t d = 0 ; d < numDecals; d++ )
		{
			const CAnimDecal& rDecal = m_DecalManager.m_arrDecals[d];			
			if (rDecal.m_pMaterial)
				pMaterial = rDecal.m_pMaterial;			

			size_t numVertices =rDecal.m_arrVertices.size();
			size_t numFaces	= rDecal.m_arrFaces.size();

		//	float fColor[4] = {0,1,0,1};
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"DecalNum: %x  DecalVertices: %x",d,numVertices );	g_YLine+=16.0f;

			if( numFaces > 0 && numVertices > 0 )
			{
				// add decals to renderer
				CRenderObject* pObj( g_pIRenderer->EF_GetObject( true ) );
        if (!pObj)
          return;
				pObj->m_ObjFlags |= FOB_TRANS_MASK | FOB_DECAL | FOB_NO_Z_PASS | FOB_INSHADOW;
				pObj->m_II.m_Matrix = RenderMat34; // = rRenderMat34;

				SVF_P3F_C4B_T2F* pDecalVertices( 0 );
				SPipTangents* pDecalTangents( 0 );
				uint16* pDecalIndices( 0 );
				g_pIRenderer->EF_AddPolygonToScene( pMaterial->GetShaderItem(), pObj, numVertices, numFaces * 3, pDecalVertices, pDecalTangents, pDecalIndices, true, false );

        int nList = (int)g_pIRenderer->EF_Query(EFQ_MainThreadList);


				if(m_pSkinAttachment)
				{
					QuatTS arrNewSkinQuat[MAX_JOINT_AMOUNT+1];	//bone quaternions for skinning (entire skeleton)
					uint32 numRemapJoints	= m_pSkinAttachment->m_arrRemapTable.size();
					for (uint32 i=0; i<numRemapJoints; i++)	arrNewSkinQuat[i]=pMaster->m_arrNewSkinQuat[nList][m_pSkinAttachment->m_arrRemapTable[i]];
					FillUpDecalBuffer( pMaster,arrNewSkinQuat, pDecalVertices, pDecalTangents, pDecalIndices, d );
				}
				else
				{
					FillUpDecalBuffer( pMaster,&pMaster->m_arrNewSkinQuat[nList][0], pDecalVertices, pDecalTangents, pDecalIndices, d );
				}



				//draw rDecal surrounding-lines
				if (Console::GetInst().ca_UseDecals==2) 
				{
					if (numVertices) 
					{
						static std::vector<Vec3> arrVertices;
						arrVertices.resize(numVertices);
						for (uint32 i=0; i<numVertices; i++)
							arrVertices[i] = RenderMat34*pDecalVertices[i].xyz;
						static std::vector<TFace> arrFaces;
						arrFaces.resize(numFaces);
						for (uint32 i=0; i<numFaces; i++)
						{
							arrFaces[i].i0 = pDecalIndices[i*3+0];
							arrFaces[i].i1 = pDecalIndices[i*3+1];
							arrFaces[i].i2 = pDecalIndices[i*3+2];
						}
						SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
						renderFlags.SetFillMode( e_FillModeWireframe );
						renderFlags.SetDrawInFrontMode( e_DrawInFrontOn );
						g_pAuxGeom->SetRenderFlags( renderFlags );
						g_pAuxGeom->DrawTriangles( &arrVertices[0],numVertices,  &arrFaces[0].i0,numFaces*3, RGBA8(0x00,0xff,0x00,0x00) );		
					}
				}
			}
		}
	}
}



//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
void CSkinInstance::FillUpDecalBuffer( CCharInstance* pMaster, QuatTS* parrNewSkinQuat, SVF_P3F_C4B_T2F* pVertices, SPipTangents* pTangents, uint16* pIndices, size_t decalIdx )
{

	CModelMesh* pMesh = m_pModel->GetModelMesh(0);
	
	if( decalIdx == 0 )
		pMesh->InitSkinningExtSW(&m_Morphing,pMaster->m_arrShapeDeformValues,0,0); // only init for first decal of this instance, all other can reuse prepared data

	const CAnimDecal& rDecal = m_DecalManager.m_arrDecals[decalIdx];

	size_t numDecalVertices = rDecal.m_arrVertices.size();
	size_t numDecalFaces = rDecal.m_arrFaces.size();


	//float fColor[4] = {1,1,0,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"numDecalVertices: %d  numDecalFaces: %d",numDecalVertices,numDecalFaces );	g_YLine+=16.0f;


	// fill vertices
	pMesh->LockFullRenderMesh(0);
	for (size_t i=0; i<numDecalVertices; i++)
	{
		// this is the index of the untransformed vertex

		//FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!
		//FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!
		uint32 idx = rDecal.m_arrVertices[i].nVertex;
		assert(idx<0x4000);
		pVertices[i].xyz		=	pMesh->GetSkinnedExtVertex2(parrNewSkinQuat,idx);
		pVertices[i].st	=	Vec2(rDecal.m_arrVertices[i].u, rDecal.m_arrVertices[i].v);
		pVertices[i].color.dcolor	=	~0;

		// TODO: fill proper tangents
		pTangents[i].Tangent	=	Vec4sf( tPackF2B( 1.0f ), tPackF2B( 0.0f ), tPackF2B( 0.0f ), tPackF2B( 1 ) );
		pTangents[i].Binormal = Vec4sf( tPackF2B( 0.0f ), tPackF2B( 1.0f ), tPackF2B( 0.0f ), tPackF2B( 1 ) );
	}

	pMesh->UnlockFullRenderMesh(0);
	// fill indices
	for (size_t f=0, g=0; f<numDecalFaces; f++)
	{
		const TFace& face = rDecal.m_arrFaces[f];
		pIndices[g] = face.i0; 
		g++;
		pIndices[g] = face.i1; 
		g++;
		pIndices[g] = face.i2; 
		g++;
	}
}

//////////////////////////////////////////////////////////////////////////
void CSkinInstance::SetMaterial( IMaterial *pMaterial )
{
	m_pInstanceMaterial = pMaterial;
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CSkinInstance::GetMaterial()
{
	if (m_pInstanceMaterial)
		return m_pInstanceMaterial;
	return m_pModel->GetMaterial();
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CSkinInstance::GetMaterialOverride()
{
	return m_pInstanceMaterial;
}




// notifies the renderer that the character will soon be rendered
void CSkinInstance::PreloadResources ( float fDistance, float fTime, int nFlags )
{
	FUNCTION_PROFILER( g_pISystem,PROFILE_3DENGINE );
	PreloadResources2(fDistance, fTime, nFlags,m_pModel->GetMaterial());
}

void CSkinInstance::PreloadResources2(float fDistance, float fTime, int nFlags,IMaterial*pMat)
{
	uint32 numLODs = m_pModel->m_arrModelMeshes.size();
	for (uint32 i=0; i<numLODs; ++i)
		if (m_pRenderMeshs[i])
//Diesel cut
			//g_pIRenderer->EF_PrecacheResource(m_pRenderMeshs[i], pMat, fDistance*fDistance, fTime, nFlags, -1);
//Diesel new
		g_pIRenderer->EF_PrecacheResource(m_pRenderMeshs[i], fDistance*fDistance, fTime, nFlags, -1);

}


IRenderMesh* CSkinInstance::GetRenderMesh ()
{
	assert( m_nRenderLOD >= 0 && m_nRenderLOD < g_nMaxGeomLodLevels);
	return m_pRenderMeshs[m_nRenderLOD];
}

// this is the array that's returned from the RenderMesh
const PodArray<CRenderChunk>* CSkinInstance::getRenderMeshMaterials()
{
	if (m_pModel->m_ObjectType==CGA)
	{
		return 0;
	}
	else
	{
		if (m_pRenderMeshs[m_pModel->m_nBaseLOD])
			return &m_pRenderMeshs[m_pModel->m_nBaseLOD]->GetChunks();
		else
			return NULL;
	}
}



//-----------------------------------------------------------------------
//----        add all morph-targets into a dynamic-stream            ----
//-----------------------------------------------------------------------
void CSkinInstance::AddMorphTargetsToVertexBuffer( int nLOD,uint32 nCloseEnough, uint32 &HWSkinningFlags )
{
	// [MichaelS - 4/8/2007] There is a problem with this method of updating the VB only once per
	// frame. The problem is that the character is often rendered twice - the first time for shadow
	// casting. Sometimes the shadow casting uses a lower lod than the main rendering, so the morph
	// targets are calculated for the wrong lod, leading to incorrect morph playback when the mesh
	// is rendered. Therefore, when Render() is called, we check whether the rendering is for shadow
	// casting, and if not we set the m_nPendingMorphTargetLod to the appropriate lod. This might
	// mean that the shadow casting is done using the morphs from the previous frame. On the other
	// hand it might mean that the shadow casting is done using morphs from an incorrect lod, but
	// this causes fewer problems than rendering visible geometry with the wrong lod.

	if (nLOD != m_nMorphTargetLod)
	{
		// We are updating the wrong LOD - this is probably because this is a shadow-casting pass and we
		// are using a lower lod than for the visible geometry. In this case, render the shadow model
		// without morphs. The 'correct' alternative is to update the morphs twice per frame, but this
		// is not cheap.
		HWSkinningFlags &= ~eHWS_MorphTarget;
		return;
	}

	CModelMesh* pModelMesh = m_pModel->GetModelMesh(nLOD);

	if (m_Morphing.m_LinearMorphSequence<0)
	{
		//	uint32 needMorph = (nLOD==0 && (NeedMorph() || m_nStillNeedsMorph > 0) && Console::GetInst().ca_UseMorph);
		uint32 needMorph = ( nCloseEnough && (m_Morphing.NeedMorph() || m_nStillNeedsMorph > 0) && Console::GetInst().ca_UseMorph);
		HWSkinningFlags |=  needMorph ? eHWS_MorphTarget : 0;

		if (HWSkinningFlags & eHWS_MorphTarget)
		{
			DEFINE_PROFILER_SECTION("CSkinInstance::MorphTargetCopy");
			//memset(pMorphData,0,numExtVertices*sizeof(Vec3));
			uint32 numVertices = pModelMesh->m_numExtVertices;
			assert(numVertices);


			if (!m_UpdatedMorphTargetVBThisFrame)
			{
				m_UpdatedMorphTargetVBThisFrame = true;

				if (m_Morphing.m_morphTargetsState.size() != numVertices)
				{
					// Make sure num of morph targets vertices are same as in vertex buffer.
					m_Morphing.m_morphTargetsState.resize(numVertices);
					memset( &m_Morphing.m_morphTargetsState[0],0,numVertices*sizeof(m_Morphing.m_morphTargetsState[0]) );
				}


				uint32 numMorphs =m_Morphing.m_arrMorphEffectors.size();
		//		float fColor[4] = {0,1,0,1};
		//		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"numMorphs: %d",numMorphs );	
		//		g_YLine+=16.0f;

				float s = 10.0f * g_pITimer->GetFrameTime()*Console::GetInst().ca_lipsync_vertex_drag;//*m_fMorphVertexDrag;
				//float s = 0.1f;
				if (s > 1.0f)
					s = 1.0f;
				if (s < 0.1f)
					s = 0.1f;

				if (numMorphs > 0)
				{


					static std::vector<Vec3> morphs;
					morphs.resize( numVertices );
					Vec3* pTrgMorphData = &morphs[0];
					memset(pTrgMorphData,0,numVertices*sizeof(Vec3));


					m_nStillNeedsMorph = 30; // Continue morph targets for at least 100 times after all morph targets gone.
					for (uint32 nMorphEffector=0; nMorphEffector<numMorphs; ++nMorphEffector)
					{

						const CryModEffMorph& rMorphEffector = m_Morphing.m_arrMorphEffectors[nMorphEffector];
						int nMorphTargetId = rMorphEffector.getMorphTargetId ();
						if (nMorphTargetId < 0)
							continue;

						CMorphTarget* pMorphSkin = m_pModel->getMorphSkin(nLOD, nMorphTargetId);
						if (pMorphSkin)
						{
							//const char* morphname = pMorphSkin->m_name;
							//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"morphname: %s",morphname );	
							//g_YLine+=16.0f;

							float fBalance = rMorphEffector.m_Params.m_fBalance;
							float fBalanceL = 0;
							float fBalanceR = 0;
							if (fBalance < 0)
								fBalanceL = fabs(fBalance);
							else
								fBalanceR = fabs(fBalance);
							float fBalanceBase = 1.0f - fabs(fBalance);
							if (pMorphSkin)
							{
								float fBlending = rMorphEffector.getBlending();

#ifdef XENON_INTRINSICS
               XMVECTOR fBal = {fBlending*fBalanceL, fBlending*fBalanceR, fBlending * fBalanceBase, 0 };
               XMVECTOR ext = XMLoadFloat3((XMFLOAT3 *)&pMorphSkin->m_MTExtensions.x);
               static const XMVECTOR div255 = {1.0f/255.0f, 1.0f/255.0f, 1.0f/255.0f, 1.0f/255.0f};
               static const XMVECTOR div15 = {1.0f/15.0f, 1.0f/15.0f, 1.0f/15.0f, 0.0f/15.0f};
               ext *= div255;
#else
                float _fBase = fBlending * fBalanceBase;
                float _fBalL = fBlending*fBalanceL;
                float _fBalR = fBlending*fBalanceR;
#endif
								uint32 numVerts = pMorphSkin->m_vertices.size();

								Vec3 vUnpackedVertex;
#if !defined(XENON_INTRINSICS)
								f32 ex = pMorphSkin->m_MTExtensions.x/255.0f;
								f32 ey = pMorphSkin->m_MTExtensions.y/255.0f;
								f32 ez = pMorphSkin->m_MTExtensions.z/255.0f;
#endif
                Vec3 res1, res2, src;
								for(uint32 i=0; i<numVerts; i++) 
								{
                  uint32 idx = pMorphSkin->m_vertices[i].m_nVertexId;
#if defined(XENON_INTRINSICS)
                  PrefetchLine(&pTrgMorphData[idx], 0);
                  //PrefetchLine(&pMorphSkin->m_vertices[i+1], 0);
                  PrefetchLine(&pMorphSkin->m_vertices[i+2], 0);

                  src = pTrgMorphData[idx];
                 // {

                  XMVECTOR target = XMLoadFloat3((XMFLOAT3 *)&pTrgMorphData[idx].x);
                  XMVECTOR negBasis = XMLoadFloat3((XMFLOAT3 *)&pMorphSkin->m_MTNegBasis.x);
                  XMUBYTE4 lr;
                  lr.x =  pMorphSkin->m_vertices[i].m_fBalanceLR&0x0f;
                  lr.y =  pMorphSkin->m_vertices[i].m_fBalanceLR >> 0x4;
                  lr.z = 15;
#ifdef XENON
                  XMVECTOR vertDataSrc;
									vertDataSrc.v[0] =pMorphSkin->m_vertices[i].m_MTVertexX;
									vertDataSrc.v[1] = pMorphSkin->m_vertices[i].m_MTVertexY;
									vertDataSrc.v[2] = pMorphSkin->m_vertices[i].m_MTVertexZ;//__lvrx(&pMorphSkin->m_vertices[i].) =;// XMLoadUByte4((const XMUBYTE4 *)&pMorphSkin->m_vertices[i].m_Data[0]);//__lvrx(&pMorphSkin->m_vertices[i].m_Data[0], 4);
#else
									XMVECTOR vertDataSrc = XMLoadUByte4((const XMUBYTE4*)&pMorphSkin->m_vertices[i].m_MTVertexX);
#endif

                  XMVECTOR lrVec = XMLoadUByte4(&lr);
                  lrVec *= div15;
                  XMVECTOR vertData = __vmaddfp(vertDataSrc, ext, negBasis);
                  XMVECTOR vertexBlend = __vspltw(__vmsum3fp(lrVec, fBal), 0);

                  XMVECTOR res = __vmaddfp(vertData, vertexBlend, target);

                  XMStoreVector3(&pTrgMorphData[idx].x, res);
                  res1 = pTrgMorphData[idx];
                 // }
  


   /*               //{
                    Vec3 tmp = src;//pTrgMorphData[idx];
                    f32 l=f32(pMorphSkin->m_vertices[i].m_fBalanceLR&0x0f)/15.0f;
                    f32 r=f32(pMorphSkin->m_vertices[i].m_fBalanceLR>>0x4)/15.0f; 
                    float fVertexBlend = _fBase +  _fBalL*l +  _fBalR*r;

                    f32 x=f32(pMorphSkin->m_vertices[i].m_MTVertexX);
                    f32 y=f32(pMorphSkin->m_vertices[i].m_MTVertexY);
                    f32 z=f32(pMorphSkin->m_vertices[i].m_MTVertexZ);
                    vUnpackedVertex.x = x*ex+pMorphSkin->m_MTNegBasis.x;
                    vUnpackedVertex.y = y*ey+pMorphSkin->m_MTNegBasis.y;
                    vUnpackedVertex.z = z*ez+pMorphSkin->m_MTNegBasis.z;
                    tmp += vUnpackedVertex * fVertexBlend;
                    pTrgMorphData[idx] = tmp;
                    res2 = pTrgMorphData[idx];

                  //}

									Vec3 dist = res1-res2;
									if (dist.GetLength() > 0.001f) {
										int a = 0;
									}*/

#else
                  Vec3 tmp = pTrgMorphData[idx];
									f32 l=f32(pMorphSkin->m_vertices[i].m_fBalanceLR&0x0f)/15.0f;
									f32 r=f32(pMorphSkin->m_vertices[i].m_fBalanceLR>>0x4)/15.0f; 
									float fVertexBlend = _fBase +  _fBalL*l +  _fBalR*r;

									f32 x=f32(pMorphSkin->m_vertices[i].m_MTVertexX);
									f32 y=f32(pMorphSkin->m_vertices[i].m_MTVertexY);
									f32 z=f32(pMorphSkin->m_vertices[i].m_MTVertexZ);
									vUnpackedVertex.x = x*ex+pMorphSkin->m_MTNegBasis.x;
									vUnpackedVertex.y = y*ey+pMorphSkin->m_MTNegBasis.y;
									vUnpackedVertex.z = z*ez+pMorphSkin->m_MTNegBasis.z;
									tmp += vUnpackedVertex * fVertexBlend;
                  pTrgMorphData[idx] = tmp;
#endif
								}
							}
						}
					}
					// Slowly scale previous morph target state into the new one.
          if( m_Morphing.m_bInstantMorphsApply )
          {
            s = 1.0f;
          }
					for (uint32 i = 0; i < numVertices; i++)
					{
						Vec3 vOrig = m_Morphing.m_morphTargetsState[i].xyz;
						Vec3 diff = pTrgMorphData[i] - vOrig;
						vOrig += diff*s;
						m_Morphing.m_morphTargetsState[i].xyz = vOrig;
					}

				} 
        else 
        {
          // Slowly scale previous morph target state into the new one.
          if( m_Morphing.m_bInstantMorphsApply )
          {
            s = 1.0f;
          }
  				// Slowly scale previous morph target state into the new one.
					for (uint32 i = 0; i < numVertices; i++)
					{
						Vec3 vOrig = m_Morphing.m_morphTargetsState[i].xyz;
						Vec3 diff = /*pTrgMorphData[i]*/ - vOrig;
						vOrig += diff*s;
						m_Morphing.m_morphTargetsState[i].xyz = vOrig;
					}
				}
			} //if (m_RenderPass==0)

			// For every pass just copy morph target state.
			if (numVertices > 0)
			{
        int32 nStride;
				SVF_P3F* pMorphData = (SVF_P3F*)m_pRenderMeshs[ nLOD ]->GetMorphTargetPtr(nStride, FSL_SYSTEM_CREATE);
				assert(pMorphData);

				// Copy morph targets state into the per pass data.
				cryMemcpy( pMorphData,&m_Morphing.m_morphTargetsState[0],numVertices*sizeof(SVF_P3F) );
			}
		}
	}
	else
	{
		//linear playback of morphs
		DEFINE_PROFILER_SECTION("CSkinInstance::MorphTargetCopy");
    int32 nStride;
    Vec3* pMorphData = (Vec3 *)m_pRenderMeshs[ nLOD ]->GetMorphTargetPtr(nStride, FSL_SYSTEM_CREATE);
    assert(pMorphData);
		uint32 numVertices = pModelMesh->m_numExtVertices;
		assert(numVertices);

		if (!m_UpdatedMorphTargetVBThisFrame)
		{
			m_UpdatedMorphTargetVBThisFrame = true;
			if (m_Morphing.m_morphTargetsState.size() != numVertices)
				m_Morphing.m_morphTargetsState.resize(numVertices);
			memset( &m_Morphing.m_morphTargetsState[0],0,numVertices*sizeof(Vec3) );

			uint32 numMorphs=pModelMesh->m_morphTargets.size();
			if (numMorphs>1)
			{
				f32 t=m_Morphing.m_LinearMorphSequence;
				if (t>=1)	t=0.9999f;

				Vec3 UnpackedVertex;

				f32 blend  = t*(numMorphs-1);
				uint32 Morph0 = uint32(blend);
				uint32 Morph1 = Morph0+1;

				assert(Morph0!=Morph1);
				CMorphTarget* pMorphTarget0 = pModelMesh->m_morphTargets[Morph0];
				f32 t0=1.0f-(blend-Morph0);
				uint32 numMorphVertices0 = pMorphTarget0->m_vertices.size();
				for (uint32 i=0; i<numMorphVertices0; i++)
				{
					f32 x=f32(pMorphTarget0->m_vertices[i].m_MTVertexX)/255.0f;
					f32 y=f32(pMorphTarget0->m_vertices[i].m_MTVertexY)/255.0f;
					f32 z=f32(pMorphTarget0->m_vertices[i].m_MTVertexZ)/255.0f;
					UnpackedVertex.x = x*pMorphTarget0->m_MTExtensions.x+pMorphTarget0->m_MTNegBasis.x;
					UnpackedVertex.y = y*pMorphTarget0->m_MTExtensions.y+pMorphTarget0->m_MTNegBasis.y;
					UnpackedVertex.z = z*pMorphTarget0->m_MTExtensions.z+pMorphTarget0->m_MTNegBasis.z;
					m_Morphing.m_morphTargetsState[pMorphTarget0->m_vertices[i].m_nVertexId].xyz = UnpackedVertex*t0;
				}
				f32 t1=(blend-Morph0);
				CMorphTarget* pMorphTarget1 = pModelMesh->m_morphTargets[Morph1];
				uint32 numMorphVertices1 = pMorphTarget1->m_vertices.size();
				for (uint32 i=0; i<numMorphVertices1; i++)
				{
					f32 x=f32(pMorphTarget1->m_vertices[i].m_MTVertexX)/255.0f;
					f32 y=f32(pMorphTarget1->m_vertices[i].m_MTVertexY)/255.0f;
					f32 z=f32(pMorphTarget1->m_vertices[i].m_MTVertexZ)/255.0f;
					UnpackedVertex.x = x*pMorphTarget1->m_MTExtensions.x+pMorphTarget1->m_MTNegBasis.x;
					UnpackedVertex.y = y*pMorphTarget1->m_MTExtensions.y+pMorphTarget1->m_MTNegBasis.y;
					UnpackedVertex.z = z*pMorphTarget1->m_MTExtensions.z+pMorphTarget1->m_MTNegBasis.z;
					m_Morphing.m_morphTargetsState[pMorphTarget1->m_vertices[i].m_nVertexId].xyz = m_Morphing.m_morphTargetsState[pMorphTarget1->m_vertices[i].m_nVertexId].xyz + UnpackedVertex*t1;
				}
				//float fColor[4] = {0,1,0,1};
				//const char* morphname = pMorphTarget0->m_name;
				//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"t0: %f    t1: %f ",t0,t1 );	g_YLine+=16.0f;
				//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"numMorphs: %d %d %f %d  morphname: %s  numVertices:%d numMorphVertices:%d",numMorphs, Morph0,blend,Morph1,morphname,numVertices,numMorphVertices0 );	g_YLine+=16.0f;
			}
		}	//if(m_RenderPass==0)

		// Copy morph targets state into the per pass data.
		HWSkinningFlags |=  eHWS_MorphTarget;
		cryMemcpy( pMorphData,&m_Morphing.m_morphTargetsState[0],numVertices*sizeof(Vec3) );
	}
}








