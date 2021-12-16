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
void CSkinInstance::Render(const struct SRendParams& RendParams, const QuatTS& Offset, Matrix34 *pFinalPhysLocation, bool* updated)
{
	int nList = (int)g_pIRenderer->EF_Query(EFQ_MainThreadList);
	if (m_pSkinAttachment==0)
		return;
	if (m_pSkinAttachment->m_Type!=CA_SKIN && m_pSkinAttachment->m_Type!=CA_PART)
	{
		const char* name = m_pModel->GetModelFilePath();
		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,name,	"CryAnimation: this SkinInstance is no skin attachment");
		return;
	}

	CCharInstance* pMaster	=	m_pSkinAttachment->m_pAttachmentManager->m_pSkelInstance;

	if (!(pMaster->rp.m_nFlags & CS_FLAG_DRAW_MODEL))
		return;
	assert(RendParams.pMatrix);

	Matrix34 RenderMat34 = (*RendParams.pMatrix);
	Matrix34 PrevRenderMat34 = RenderMat34; 
	if (RendParams.pPrevMatrix)	PrevRenderMat34 = (*RendParams.pPrevMatrix);

	g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );



	//-----------------------------------------------------------------------------
	//---              map logical LOD to render LOD                            ---
	//-----------------------------------------------------------------------------
	uint32 nLodLevel = RendParams.nLod;
	int nRenderLOD = nLodLevel;
	static ICVar *p_e_lod_min = gEnv->pConsole->GetCVar("e_LodMin");
	if(p_e_lod_min)
		nRenderLOD += p_e_lod_min->GetIVal();
	if(gEnv->IsEditor() && CSkinInstance::m_bForceLOD)
		nRenderLOD = CSkinInstance::m_ForcedLODNum;
	int32 numLODs = m_pModel->m_arrModelMeshes.size();
	if(nRenderLOD<m_pModel->m_nBaseLOD)
		nRenderLOD=m_pModel->m_nBaseLOD;
	if(nRenderLOD>=numLODs)
		nRenderLOD = numLODs-1;

	//float fColor[4] = {1,0,1,1};
	//extern f32 g_YLine;
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"CSkin:  nLodLevel: %d   nRenderLOD: %d   Model: %s",nLodLevel,nRenderLOD,m_pModel->GetFilePath().c_str() ); g_YLine+=0x10;
	
	if (!g_pI3DEngine->GetRenderIntoShadowmap())
		m_nMorphTargetLod = nRenderLOD;

	assert(m_pSkinAttachment);

	//---------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------
	//	Render2 (RendParams, RenderMat34, rp );

	CRenderObject* pObj = g_pIRenderer->EF_GetObject(true);
	if (!pObj)
		return;

	pObj->m_nLod	=	nRenderLOD;
	pObj->m_fSort	= RendParams.fCustomSortOffset;

	pObj->m_ObjFlags |= FOB_TRANS_MASK;

#ifdef ALLOW_CAMERA_SPACE
	if (RendParams.dwFObjFlags & FOB_CAMERA_SPACE)
	{
		pObj->m_ObjFlags |= FOB_CAMERA_SPACE;
	}
#endif
	//check if it should be drawn close to the player
	if ((RendParams.dwFObjFlags & FOB_NEAREST) || (pMaster->rp.m_nFlags & CS_FLAG_DRAW_NEAR) )	{ 	pObj->m_ObjFlags|=FOB_NEAREST;	}
	else pObj->m_ObjFlags&=~FOB_NEAREST;

	pObj->m_fAlpha = RendParams.fAlpha;
	pObj->m_fDistance =	RendParams.fDistance;

	pObj->m_II.m_AmbColor = RendParams.AmbientColor;

	pObj->m_ObjFlags |= RendParams.dwFObjFlags;
	SRenderObjData *pD = g_pIRenderer->EF_GetObjData(pObj, true);

	pObj->m_II.m_Matrix = RenderMat34;

  if( RendParams.pPrevMatrix && pFinalPhysLocation )
    (*pFinalPhysLocation) = RenderMat34;

  pObj->m_nMotionBlurAmount = 0;
  if( RendParams.pPrevMatrix && (Console::GetInst().m_pMotionBlur->GetIVal()> 1 || (RendParams.nMaterialLayersBlend & MTL_LAYER_BLEND_CLOAK)))
  {
    bool bCheckMotion = pMaster->MotionBlurMotionCheck( pObj->m_ObjFlags );
    if( bCheckMotion || !pObj->m_II.m_Matrix.IsEquivalent(PrevRenderMat34, 0.001f) )
    {
      pD->m_prevMatrix = PrevRenderMat34;
      pObj->m_nMotionBlurAmount = 128;
    }
  }

	pObj->m_nMaterialLayers = RendParams.nMaterialLayersBlend;

//Diesel cut
  if( RendParams.nVisionParams && !g_pI3DEngine->GetRenderIntoShadowmap() && !(pObj->m_ObjFlags & FOB_VEGETATION))
  {
    pObj->m_nVisionParams = RendParams.nVisionParams;
  }

//Diesel new
	//if( RendParams.nVisionParams && !g_pI3DEngine->GetRenderIntoShadowmap() && !(pObj->m_ObjFlags & FOB_VEGETATION))
    //pObj->m_nVisionParams = RendParams.nVisionParams;
	
//Diesel cut
  pObj->m_ObjFlags |= (RendParams.dwFObjFlags & FOB_SHADOW_DISSOLVE);
	
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

	pObj->m_DynLMMask[nList] = RendParams.nDLightMask;

	pObj->m_nSort = fastround_positive(RendParams.fDistance * 2.0f);

	//	AddCurrentRenderData (pObj, RendParams);
	IRenderMesh* pRenderMesh = m_pRenderMeshs[nRenderLOD];
	if (pRenderMesh==0)
		AnimWarning("model '%s' has no render mesh", m_pModel->GetModelFilePath() );

	if (pRenderMesh)
	{       
		// MichaelS - use the instance's material if there is one, and if no override material has already been specified.
		IMaterial* pMaterial = RendParams.pMaterial;
		if (!pMaterial && this->CSkinInstance::GetMaterialOverride())
			pMaterial = this->CSkinInstance::GetMaterialOverride();

    if(!pMaterial)
      pMaterial = m_pModel->GetMaterial();

		CModelMesh* pModelMesh = m_pModel->GetModelMesh(nRenderLOD);
		static ICVar *p_e_debug_draw = gEnv->pConsole->GetCVar("e_DebugDraw");
		if(p_e_debug_draw && p_e_debug_draw->GetIVal() > 0)
			pModelMesh->DrawDebugInfo(pMaster, nRenderLOD, RenderMat34, p_e_debug_draw->GetIVal(), pMaterial, pObj, RendParams );

		/*float fColor[4] = {1,0,1,1};
		extern f32 g_YLine;
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"pObj->m_nLod: %d",pObj->m_nLod  ); g_YLine+=0x10;*/
			
		pRenderMesh->Render(RendParams,pObj,pMaterial);
		if (!g_pI3DEngine->GetRenderIntoShadowmap())
			AddDecalsToRenderer(pMaster,m_pSkinAttachment,RenderMat34);
	}

	// draw weapon and binded objects
	m_AttachmentManager.DrawAttachments(RendParams,RenderMat34);
}




void CSkinInstance::DrawDecalsBBoxes( CCharInstance* pMaster2, CAttachment* pAttachment, const Matrix34& rRenderMat34)
{
  int nList = (int)g_pIRenderer->EF_Query(EFQ_MainThreadList);

	QuatTS arrNewSkinQuat[MAX_JOINT_AMOUNT+1];	//bone quaternions for skinning (entire skeleton)
//	uint32 iActiveFrame		=	pMaster2->m_iActiveFrame;
	if(pAttachment)
	{
		uint32 numRemapJoints	= pAttachment->m_arrRemapTable.size();
		for (uint32 i=0; i<numRemapJoints; i++)
			arrNewSkinQuat[i]=pMaster2->m_arrNewSkinQuat[nList][pAttachment->m_arrRemapTable[i]];
	}
	else
	{
		uint32 numJoints	= pMaster2->m_SkeletonPose.GetJointCount();
		cryMemcpy(arrNewSkinQuat, &pMaster2->m_arrNewSkinQuat[nList][0],	sizeof(QuatTS)*numJoints);
	}

	OBB obb;
	QuatT WorldQuat		=	QuatT( rRenderMat34 );

	uint32 count = m_pModel->m_arrCollisions.size();
	for (uint32 i=1; i<count; ++i)
	{
		int32 id	=	m_pModel->m_arrCollisions[i].m_iBoneId;
		AABB aabb	=	m_pModel->m_arrCollisions[i].m_aABB;
		const QuatD& qd=(const QuatD&)arrNewSkinQuat[id].q;
		QuatTS wjoint=WorldQuat*QuatT(qd);
		obb.SetOBBfromAABB(wjoint.q,aabb);
		g_pAuxGeom->DrawOBB(obb,wjoint.t,0,RGBA8(0x0f,0x3f,0x3f,0x0f),eBBD_Extremes_Color_Encoded);
	}

}






uint32 CSkinInstance::GetSkeletonPose( int nLOD, int nList, const Matrix34& rRenderMat34, QuatTS*& pBoneQuatsL, QuatTS*& pBoneQuatsS, QuatTS*& pMBBoneQuatsL, QuatTS*& pMBBoneQuatsS, Vec4 shapeDeformationData[], uint32 &HWSkinningFlags, uint8*& pRemapTable )
{
#ifdef DEFINE_PROFILER_FUNCTION
	DEFINE_PROFILER_FUNCTION();
#endif

	//lets be optimistic and assume this is the Master-Character
//	uint32 iActiveFrame=0;
	
	CCharInstance* pMaster=0;
	uint32 numSkinQuats=0;
	pRemapTable=0;
	CAttachment* pSkinAttachment=m_pSkinAttachment;

	const Console& rConsole = Console::GetInst();

	if (pSkinAttachment)
	{
		//This is a slave character! take bones of master character	
		//Get the instance of the master-character
		pMaster	=	pSkinAttachment->m_pAttachmentManager->m_pSkelInstance;

//		iActiveFrame=pMaster->m_iActiveFrame;
		numSkinQuats = pSkinAttachment->m_arrRemapTable.size();
		pRemapTable = &pSkinAttachment->m_arrRemapTable[0];

		if (pSkinAttachment->m_Type!=CA_SKIN && pSkinAttachment->m_Type!=CA_PART)
		{
			AnimWarning("CryAnimation: SkinInstance is no skin attachment");
			CryFatalError("CryAnimation: SkinInstance is no skin attachment");
		}
	} 
	else 
	{
		pMaster=(CCharInstance*)this;
//		iActiveFrame=pMaster->m_iActiveFrame;
		numSkinQuats = pMaster->m_arrNewSkinQuat[nList].size();//
	}

	bool bUseMotionBlur = (HWSkinningFlags&eHWS_MotionBlured)>0;
	uint32 nCloseEnough = (HWSkinningFlags&eHWS_MorphTarget);
	HWSkinningFlags = NULL;
#if !defined(SHAPE_DEFORMATION_DISABLED)
	HWSkinningFlags = eHWS_ShapeDeform;
#endif
	if (pMaster==0)
		CryFatalError("CryAnimation: pMaster is zero");

	int nCurrentFrameID = g_pCharacterManager->m_nUpdateCounter; //g_pIRenderer->GetFrameID(false);
	if (m_nLastRenderedFrameID!=nCurrentFrameID) 
	{
		m_nLastRenderedFrameID=nCurrentFrameID; 
		m_RenderPass=0;
		m_UpdatedMorphTargetVBThisFrame = false;

		if (m_nStillNeedsMorph > 0)
			m_nStillNeedsMorph--;
	} 
	else 
	{
		m_RenderPass++;
	}
	assert(m_RenderPass!=0x55aa55aa);


	pBoneQuatsS		=	0;
	pBoneQuatsL		=	0; 
	pMBBoneQuatsL	=	0;
	pMBBoneQuatsS	=	0;

	if (rConsole.ca_SphericalSkinning) 
		pBoneQuatsS = &pMaster->m_arrNewSkinQuat[nList][0];
	else
		pBoneQuatsL = &pMaster->m_arrNewSkinQuat[nList][0]; 


	if (bUseMotionBlur) 
	{
		pMaster->UpdatePreviousFrameBones(0);
#ifdef MOTIONBLUR_QUATS
		if (pMaster->m_arrMBSkinQuat[0].size()==0)
			CryFatalError("CryAnimation: m_arrMBSkinQuat is zero");

		if (rConsole.ca_SphericalSkinning) 
			pMBBoneQuatsS	=	&pMaster->m_arrMBSkinQuat[nList][0];
		else
			pMBBoneQuatsL	=	&pMaster->m_arrMBSkinQuat[nList][0];
#else
		if (rConsole.ca_SphericalSkinning) 
			pMBBoneQuatsS	=	&pMaster->m_arrNewSkinQuat[nList][0];
		else
			pMBBoneQuatsL	=	&pMaster->m_arrNewSkinQuat[nList][0];
#endif
	}

	AddMorphTargetsToVertexBuffer(nLOD,nCloseEnough,HWSkinningFlags);

	//-----------------------------------------------------------------------
	//----    Create 8 Vec4 vectors that contain the blending values     ----
	//---                    to blend between 3 models                    ---
	//-----------------------------------------------------------------------
	uint32 rm=pMaster->GetResetMode();
	f32 MorphArray[8] = { 0,0,0,0,0,0,0,0};
	if (rm==0) 
	{ 
		f32* pMorphValues = pMaster->GetShapeDeformArray();
		MorphArray[0]=pMorphValues[0];
		MorphArray[1]=pMorphValues[1];
		MorphArray[2]=pMorphValues[2];
		MorphArray[3]=pMorphValues[3];
		MorphArray[4]=pMorphValues[4];
		MorphArray[5]=pMorphValues[5];
		MorphArray[6]=pMorphValues[6];
		MorphArray[7]=pMorphValues[7];
	}

#ifdef XENON_INTRINSICS

#define UPDATE_SHAPE_DEFORM { \
	const float morph = MorphArray[i]; \
	float sign = 	(float)__fsel(morph, 0.0f, 1.0f); \
	shapeDeformationData[i].x = -sign * morph;	\
	shapeDeformationData[i].y = 1.0f + (sign * 2.0f - 1.0f) * morph;	\
	shapeDeformationData[i++].z = (1.0f - sign) * morph;	\
	}

	{
		int i = 0;
		UPDATE_SHAPE_DEFORM;
		UPDATE_SHAPE_DEFORM;
		UPDATE_SHAPE_DEFORM;
		UPDATE_SHAPE_DEFORM;
		UPDATE_SHAPE_DEFORM;
		UPDATE_SHAPE_DEFORM;
		UPDATE_SHAPE_DEFORM;
		UPDATE_SHAPE_DEFORM;
	}
#else
	for (uint32 i=0; i<8; i++) 
	{
		float sign = 	(float)__fsel(MorphArray[i], 0.0f, 1.0f);
		shapeDeformationData[i] = Vec4( -sign*MorphArray[i],1.0f+(sign*2.0f-1.0f)*MorphArray[i], (1.0f-sign)*MorphArray[i], 0);
	}
#endif



	//------------------------------------------------------------------
	//---       render debug-output                                  ---
	//------------------------------------------------------------------
#if !defined(_RELEASE)
	if (m_RenderPass==0) 
	{
		if (rConsole.ca_DrawDecalsBBoxes)
			DrawDecalsBBoxes(pMaster,pSkinAttachment, rRenderMat34 );

		if (rConsole.ca_DrawWireframe) 
		{
			CModelMesh* pModelMesh = m_pModel->GetModelMesh(nLOD);
			if (pSkinAttachment)
			{
				QuatTS arrNewSkinQuat[MAX_JOINT_AMOUNT+1];	//bone quaternions for skinning (entire skeleton)
				uint32 numRemapJoints	= pSkinAttachment->m_arrRemapTable.size();
				for (uint32 i=0; i<numRemapJoints; i++)	arrNewSkinQuat[i]=pMaster->m_arrNewSkinQuat[nList][pSkinAttachment->m_arrRemapTable[i]];
				pModelMesh->DrawWireframe(&m_Morphing,arrNewSkinQuat,pMaster->GetShapeDeformArray(),pMaster->GetResetMode(),nLOD,rRenderMat34);
			}
			else
			{
				pModelMesh->DrawWireframe(&m_Morphing,&pMaster->m_arrNewSkinQuat[nList][0],GetShapeDeformArray(),GetResetMode(), nLOD,rRenderMat34);
			}
		}


		uint32 tang = rConsole.ca_DrawTangents;
		uint32 binorm = rConsole.ca_DrawBinormals;
		uint32 norm = rConsole.ca_DrawNormals;
		if (tang || binorm || norm) 
		{
			CModelMesh* pModelMesh = m_pModel->GetModelMesh(nLOD);
			pModelMesh->SkinningExtSW(pMaster,pSkinAttachment,nLOD, nList);

			if (tang) 
				pModelMesh->DrawTangents(rRenderMat34);
			if (binorm) 
				pModelMesh->DrawBinormals(rRenderMat34);
			if (norm) 
				pModelMesh->DrawNormals(rRenderMat34);
		}
	}
#endif

	return numSkinQuats;
}







