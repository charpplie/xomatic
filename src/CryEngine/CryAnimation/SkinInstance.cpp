//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:CSkinInstance.cpp
//  Implementation of CSkinInstance class
//
//	History:
//	September 23, 2004: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include <I3DEngine.h>
#include <Cry_Camera.h>
#include <CryAnimationScriptCommands.h>


#include "ModelMesh.h"
#include "CharacterInstance.h"
#include "CharacterManager.h"
#include "CryCharAnimationParams.h"
#include "CryCharMorphParams.h"
#include "IRenderAuxGeom.h"
#include "FacialAnimation/FacialInstance.h"
#include "GeomQuery.h"


// this is the count of model states created so far
uint32 g_nCharInstanceLoaded = 0;
uint32 g_nSkinInstanceLoaded = 0;
uint32 g_nCharInstanceDeleted = 0;
uint32 g_nSkinInstanceDeleted = 0;

bool CSkinInstance::m_bForceLOD = false;
int CSkinInstance::m_ForcedLODNum = 0;

CSkinInstance::CSkinInstance ( const string &strFileName, CCharacterModel* pModel, uint32 IsSkinAtt, CAttachment* pMasterAttachment )
{
	assert(IsSkinAtt==0xDeadBeef || IsSkinAtt==0xaaaabbbb);
  m_IsSkinAtt = IsSkinAtt;
  //
  m_fUniformScale = 1.0f;
  //
	if(IsSkinAtt==0xDeadBeef)
		g_nSkinInstanceLoaded++;

  CCharInstance* pIMasterInstance = (CCharInstance*)this;
  if (IsSkinAtt == 0xDeadBeef)
    pIMasterInstance = pMasterAttachment->m_pAttachmentManager->m_pSkelInstance;

	m_nRefCounter = 0;

	m_nStillNeedsMorph = 0;
	m_bHaveEntityAttachments = false;
	m_nInstanceUpdateCounter=0;

  Init(strFileName, pModel, pIMasterInstance);
}

//-------------------------------------------------------------------------
void CSkinInstance::Init(const string& strFileName, CCharacterModel* pModel, CCharInstance* pIMasterInstance)
{
	m_useDecals=0;

	m_AttachmentManager.m_pSkinInstance=0;
	m_AttachmentManager.m_pSkelInstance=0;
	m_pSkinAttachment=0; //we set this at run-time

	m_strFilePath=strFileName;
	pModel->AddRef();
	m_pModel=pModel;
	pModel->RegisterInstance(this);
	// Copy render mesh pointers from model to instance.
	uint32 nLOD = (uint32)pModel->m_arrModelMeshes.size();
	for(uint32 nLod=0; nLod<nLOD; nLod++)
		m_pRenderMeshs[nLod] = pModel->m_pRenderMeshs[nLod];


	m_nAnimationLOD=0;
	m_nRenderLOD=0;
	m_nLastRenderedFrameID=0xffaaffaa;
	m_RenderPass=0x55aa55aa;
	m_LastRenderedFrameID=0;
	m_nMorphTargetLod = -1;
	m_UpdatedMorphTargetVBThisFrame = false;
	m_uInstanceFlags= (FLAG_DEFAULT_FLAGS);
	m_UpdateAttachmentMesh=0;

	m_bFacialAnimationEnabled = true;

	m_Morphing.InitMorphing(pModel);

	m_pFacialInstance = 0;
	CFacialModel* pFacialModel = m_pModel->GetFacialModel();
	if (pFacialModel)
	{
		if (m_IsSkinAtt==0xDeadBeef)
		{
			m_pFacialInstance = new CFacialInstance(m_pModel->GetFacialModel(),this,pIMasterInstance);
			m_pFacialInstance->AddRef();
		}
	}
}

//-------------------------------------------------------------------------
void CSkinInstance::ReplaceModel(ICharacterModel* pNewModel)
{
  m_Morphing.StopAllMorphs();

  string filePath = m_pModel->GetFilePath();
  m_pModel->UnregisterInstance(this);
  m_pModel->Release();

  CCharInstance* pIMasterInstance = (CCharInstance*)this;
  if (m_IsSkinAtt == 0xDeadBeef && m_pFacialInstance)
    pIMasterInstance = m_pFacialInstance->GetMasterCharacter();
  SAFE_RELEASE(m_pFacialInstance);

  Init(filePath, (CCharacterModel*)pNewModel, pIMasterInstance);
}






//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
SPU_NO_INLINE void CSkinInstance::UpdateAttachedObjects(const QuatT& rPhysLocationNext, IAttachmentObject::EType *pOnlyThisType, float fZoomAdjustedDistanceFromCamera, uint32 OnRender  )
{
	DEFINE_PROFILER_FUNCTION();

#ifdef VTUNE_PROFILE 
	g_pISystem->VTuneResume();
#endif

	//for this we need the absolute joints 
	m_AttachmentManager.UpdateLocationAttachments( rPhysLocationNext,pOnlyThisType );
	uint32 numAttachmnets = m_AttachmentManager.m_arrAttachments.size();
	for (uint32 i=0; i<numAttachmnets; i++) 
	{
		CAttachment* pCAttachment = m_AttachmentManager.m_arrAttachments[i];
		IAttachmentObject* pIAttachmentObject = pCAttachment->m_pIAttachmentObject;
		if (pIAttachmentObject) 
		{
			if (pOnlyThisType && *pOnlyThisType != pIAttachmentObject->GetAttachmentType())
				continue;

			uint32 type = pCAttachment->GetType();
			if (type==CA_SKIN || type==CA_PART) 
      {
        if( pIAttachmentObject->GetICharacterInstance() )
        {
				  pIAttachmentObject->UpdateAttachment( pCAttachment, rPhysLocationNext, fZoomAdjustedDistanceFromCamera, OnRender);
        }
      }
			else 
      {
				pIAttachmentObject->UpdateAttachment( pCAttachment, pCAttachment->GetAttWorldAbsolute(), fZoomAdjustedDistanceFromCamera, OnRender);
      }
		}
	}
#ifdef VTUNE_PROFILE 
	g_pISystem->VTunePause();
#endif
}

//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
void CSkinInstance::UpdateAttachedObjectsFast(const QuatT& rPhysLocationNext, float fZoomAdjustedDistanceFromCamera, uint32 OnRender )
{
	DEFINE_PROFILER_FUNCTION();

	if (m_bHaveEntityAttachments==0)
		return;

	//const char* mname = GetFilePath();
	//float fC1[4] = {1,0,0,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC1, false,"FastAttachmentUpdate  Model: %s ",mname );	
	//g_YLine+=16.0f;
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC1, false,"pos: %f %f %f",rPhysLocationNext.t.x,rPhysLocationNext.t.y,rPhysLocationNext.t.z );	
	//g_YLine+=16.0f;

	m_AttachmentManager.UpdateLocationAttachmentsFast(rPhysLocationNext); 

	uint32 numAttachmnets = m_AttachmentManager.m_arrAttachments.size();
	for (uint32 i=0; i<numAttachmnets; i++) 
	{
		CAttachment* pCAttachment = m_AttachmentManager.m_arrAttachments[i];
		IAttachmentObject* pIAttachmentObject = pCAttachment->m_pIAttachmentObject;
		if (pIAttachmentObject) 
		{
			uint32 type = pCAttachment->GetType();
			if (type!=CA_SKIN && type!=CA_PART) 
      {
        //if( pIAttachmentObject->GetICharacterInstance() )
        {
          pIAttachmentObject->UpdateAttachment( pCAttachment, pCAttachment->GetAttWorldAbsolute(), fZoomAdjustedDistanceFromCamera, OnRender);
        }
      }
		}
	}
}

// Returns true if this character was created from the file the path refers to.
// If this is true, then there's no need to reload the character if you need to change its model to this one.
bool CSkinInstance::IsModelFileEqual (const char* szFileName)
{
	stack_string strPath = szFileName;
	CryStringUtils::UnifyFilePath(strPath);
	return !stricmp(strPath.c_str(), m_pModel->GetModelFilePath());
}





void CSkinInstance::GetMemoryUsage2(ICrySizer* pSizer) const
{
#if ENABLE_GET_MEMORY_USAGE
	SIZER_SUBCOMPONENT_NAME(pSizer, "Characters");
	pSizer->Add(*this);
	pSizer->AddObject(m_AttachmentManager.m_arrAttachments);
#endif

	uint32 nLOD = m_pModel->m_arrModelMeshes.size();
	for(uint32 dwLod=0; dwLod<nLOD; dwLod++)
	{
		if(!m_pRenderMeshs[dwLod])
			continue;

		if(m_pModel->GetMaterial()==NULL)
			continue;

		PodArray<CRenderChunk>& Mats = m_pRenderMeshs[dwLod]->GetChunks();

		for (uint32 j=0; j<Mats.Count(); j++)
		{
			CRenderChunk * pChunk = &Mats[j];

			if(!pChunk)
				continue;

			SShaderItem shaderItem = m_pModel->GetMaterial()->GetShaderItem(pChunk->m_nMatID);
			if (!shaderItem.m_pShaderResources)
				continue;

			IRenderShaderResources *pRes = shaderItem.m_pShaderResources;

			for (int32 i=0; i<EFTT_MAX; i++)
			{
				if (!pRes->GetTexture(i))
					continue;

				ITexture *pTexture = pRes->GetTexture(i)->m_Sampler.m_pITex;

				uint32 dwSize=0xffffffff;

				if(pTexture)
				{
					dwSize = pTexture->GetDeviceDataSize();

					pSizer->GetResourceCollector().AddResource(pTexture->GetName(),dwSize);		// used texture
				}
			}
		}
	}
}








void CSkinInstance::DeleteDecals()
{
	m_DecalManager.clear();
}









//////////////////////////////////////////////////////////////////////////
SPU_INDIRECT(CommandBufferExecute(M))
SPU_NO_INLINE IFacialInstance* CSkinInstance::GetFacialInstance()
{
	if (m_pFacialInstance)
		return m_pFacialInstance;

	// Check all skin attachments if they have facial animation.
	uint32 numAttachmnets = m_AttachmentManager.m_arrAttachments.size();
	for (uint32 i=0; i<numAttachmnets; i++) 
	{
		CAttachment* pAttachment = m_AttachmentManager.m_arrAttachments[i];
		IAttachmentObject* pIAttachmentObject = pAttachment->m_pIAttachmentObject;
		if (pIAttachmentObject) 
		{
			ICharacterInstance *pAttachedCharacter = pIAttachmentObject->GetICharacterInstance();
			if (pAttachedCharacter)
			{
				IFacialInstance *pFacialInstance = pAttachedCharacter->GetFacialInstance();
				if (pFacialInstance)
					return pFacialInstance;
			}
		}
	}
	return NULL;
};

//////////////////////////////////////////////////////////////////////////
void CSkinInstance::LipSyncWithSound( uint32 nSoundId, bool bStop )
{
	IFacialInstance *pFacialInstance = GetFacialInstance();
	if (pFacialInstance)
	{
		pFacialInstance->LipSyncWithSound( nSoundId, bStop );
	}
}


//////////////////////////////////////////////////////////////////////////
void CSkinInstance::EnableFacialAnimation( bool bEnable )
{
	m_bFacialAnimationEnabled = bEnable;

	for (int attachmentIndex = 0, end = int(m_AttachmentManager.m_arrAttachments.size()); attachmentIndex < end; ++attachmentIndex)
	{
		CAttachment* pAttachment = m_AttachmentManager.m_arrAttachments[attachmentIndex];
		IAttachmentObject* pAttachmentObject = (pAttachment ? pAttachment->GetIAttachmentObject() : 0);
		ICharacterInstance* pAttachmentCharacter = (pAttachmentObject ? pAttachmentObject->GetICharacterInstance() : 0);
		if (pAttachmentCharacter)
			pAttachmentCharacter->EnableFacialAnimation(bEnable);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSkinInstance::EnableProceduralFacialAnimation( bool bEnable )
{
	IFacialInstance *pInst = GetFacialInstance();
	if (pInst)
		pInst->EnableProceduralFacialAnimation( bEnable );
}


 

void CSkinInstance::ReleaseTemporaryResources()
{
	if (m_Morphing.m_morphTargetsState.capacity())
	{
		DynArray<SVF_P3F> dummy;
		m_Morphing.m_morphTargetsState.swap(dummy);
	}

	int end = int(m_AttachmentManager.m_arrAttachments.size());
	for (int attachmentIndex = 0; attachmentIndex < end; ++attachmentIndex)
	{
		CAttachment* pAttachment = m_AttachmentManager.m_arrAttachments[attachmentIndex];
		IAttachmentObject* pAttachmentObject = (pAttachment ? pAttachment->GetIAttachmentObject() : 0);
		CSkinInstance* pAttachedCharacter = (CSkinInstance*)(pAttachmentObject ? pAttachmentObject->GetICharacterInstance() : 0);
		if (pAttachedCharacter)
			pAttachedCharacter->ReleaseTemporaryResources();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSkinInstance::ShutDown()
{
	const char* pFilePath = GetFilePath();
	g_pCharacterManager->ReleaseCDF(pFilePath);
}

//////////////////////////////////////////////////////////////////////////
void CSkinInstance::Init()
{
}


//////////////////////////////////////////////////////////////////////////
void CSkinInstance::DeleteThis()
{
	delete this;
}

void CSkinInstance::Release()
{
	if (--m_nRefCounter == 0)
	{
		ShutDown();
		g_pCharacterManager->AddToEraseList(this);
	}
	else if (m_nRefCounter < 0)
	{
		// Should never happens, someone tries to release CharacterInstance pointer twice.
		assert( 0 && "CSkinInstance::Release" );
		CryFatalError( "CSkinInstance::Release" );
	}
}

//////////////////////////////////////////////////////////////////////////
CSkinInstance::~CSkinInstance()
{ 
	SAFE_RELEASE(m_pFacialInstance);

	//////////////////////////////////////////////////////////////////////////
	// Check if character body locked for the next level.
	//////////////////////////////////////////////////////////////////////////
	if (IsResourceLocked(m_pModel->GetModelFilePath()))
	{
		g_pCharacterManager->LockModel( m_pModel );
	}

	assert(m_nRefCounter==0);

	if(m_pSkinAttachment)
	{
		g_nSkinInstanceLoaded--;
		g_nSkinInstanceDeleted++;
	}
	// the submeshes that are not the default model submeshes lock their corresponding
	// CCharacterModels. The default model can't lock its parent CCharacterModel, because it will
	// be circular dependency. Owner ship is as follows: States->CCharacterModel->Default State
	m_AttachmentManager.RemoveAllAttachments();
	m_pModel->UnregisterInstance(this);
	m_pModel->Release();
}





//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------

void CSkinInstance::ProcessSkinAttachment(const QuatT &rPhysLocationNext,const QuatTS &rAnimLocationNext, IAttachment* pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 OnRender )
{

	//this is a skin attachment
	assert(pIAttachment); 
	assert(pIAttachment->GetType()==CA_SKIN || pIAttachment->GetType()==CA_PART); 

#if !defined(_RELEASE)
	if (pIAttachment==0)
	{
		const char* name = m_pModel->GetModelFilePath();
		CryFatalError("expecting pointer to skin-attachment for model: '%s'", name );
	}
#endif

  m_pSkinAttachment=(CAttachment*)pIAttachment;

	//Get the instance of the master-character
	CCharInstance* pMaster	=	m_pSkinAttachment->m_pAttachmentManager->m_pSkelInstance;
	const f32 fDeltaTime = pMaster->m_fDeltaTime;
	if (m_bFacialAnimationEnabled)
		m_Morphing.UpdateMorphEffectors(m_pFacialInstance, fDeltaTime, rAnimLocationNext, fZoomAdjustedDistanceFromCamera);
	else
		m_Morphing.UpdateMorphEffectors(0,fDeltaTime, rAnimLocationNext, fZoomAdjustedDistanceFromCamera);

	uint32 numAttachmnets=m_AttachmentManager.GetAttachmentCount();
	if (numAttachmnets)
		UpdateAttachedObjects(rPhysLocationNext,0,fZoomAdjustedDistanceFromCamera,OnRender);

}



AABB CSkinInstance::GetAABB() 
{ 
	if (m_pSkinAttachment==0)
		return AABB(ZERO,ZERO);

	CCharInstance* pMaster	=	m_pSkinAttachment->m_pAttachmentManager->m_pSkelInstance;
	return pMaster->m_SkeletonPose.GetAABB();	
}








//////////////////////////////////////////////////////////////////////////
size_t CSkinInstance::SizeOfCharInstance(ICrySizer * pSizer)
{

	size_t nSizeOfSkinInstance	= 0;
	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "CAttachmentManager");
		size_t size = sizeof(CAttachmentManager)+m_AttachmentManager.SizeOfThis();
		pSizer->AddObject(&m_AttachmentManager, size);
		nSizeOfSkinInstance += size;
	}

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "DecalManager");
		size_t size		= sizeof(CAnimDecalManager)+m_DecalManager.SizeOfThis();
		pSizer->AddObject(&m_DecalManager, size);
		nSizeOfSkinInstance += size;
	}

	{
		SIZER_COMPONENT_NAME(pSizer, "Morphing");
		size_t size	= sizeof(CMorphing);
		size +=	sizeof(Vec3)*m_Morphing.m_morphTargetsState.capacity();
		size += sizeof(CryModEffMorph)*m_Morphing.m_arrMorphEffectors.capacity(); 
		pSizer->AddObject(&m_Morphing, size);
		nSizeOfSkinInstance += size;
	}

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "FacialInstance");
		size_t size		= 0;
		if (m_pFacialInstance)
			size = m_pFacialInstance->SizeOfThis();
		pSizer->AddObject(m_pFacialInstance, size);
		nSizeOfSkinInstance += size;
	}



	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "CSkinInstance");
		size_t size		=	sizeof(CSkinInstance)-sizeof(CAttachmentManager)-sizeof(CAnimDecalManager)-sizeof(CMorphing);
		uint32 stringlength = m_strFilePath.capacity();
		size += stringlength;
		pSizer->AddObject(&m_uInstanceFlags, size);
		nSizeOfSkinInstance += size;
	}


	return nSizeOfSkinInstance;
};

void CSkinInstance::GetMemoryUsage(ICrySizer * pSizer) const
{

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "CAttachmentManager");
		pSizer->AddObject( m_AttachmentManager );
	}

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "DecalManager");
		pSizer->AddObject( m_DecalManager );
	}

	{
		SIZER_COMPONENT_NAME(pSizer, "Morphing");
		pSizer->AddObject( m_Morphing );
	}

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "FacialInstance");
		pSizer->AddObject( m_pFacialInstance );		
	}


	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "CSkinInstance");		
		pSizer->AddObject( m_strFilePath );		
	}

};

//////////////////////////////////////////////////////////////////////////
int CSkinInstance::FindAffectedVertices(const Vec3 &vPos,float fRadius,std::vector<int> &lstIDs)
{
	// first set all morphs to default (0)
	
	// vertices not included in the list here means they are never moved by morphs so
	// we dont care about them

	lstIDs.clear();
	
	int nLOD=0;

	CModelMesh* pModelMesh = m_pModel->GetModelMesh(nLOD);

	uint32 numMorphs =m_Morphing.m_arrMorphEffectors.size();

	uint32 numVertices = pModelMesh->m_numExtVertices;


	// code from modelmesh drawwireframe - initialize skinning ext
//	f32 arrShapeDeformValues[8] = {0,0,0,0, 0,0,0,0};
	f32 MorphArray[8] = { 0,0,0,0,0,0,0,0};
	Vec4 VertexRegs[8];
	for (uint32 i=0; i<8; i++) 
	{
		if (MorphArray[i]<0.0f) 
		{
			VertexRegs[i].x	=	1-(MorphArray[i]+1);
			VertexRegs[i].y	=	MorphArray[i]+1;
			VertexRegs[i].z	=	0;
		} 
		else 
		{
			VertexRegs[i].x	=	0;
			VertexRegs[i].y	=	1-MorphArray[i];
			VertexRegs[i].z	=	MorphArray[i];
		}
	}

	// calc offset

	DynArray<Vec3> morphs2;
	morphs2.resize( numVertices );
	Vec3* pTrgMorphData = &morphs2[0];
	memset(pTrgMorphData,0,numVertices*sizeof(Vec3));

	static float g_Test=0;
	g_Test+=gEnv->pTimer->GetFrameTime();
	if (g_Test>1.0f)
		g_Test=0;

	// TODO: remove all effectors sent by game, editor etc.

	numMorphs=m_pModel->GetNumMorphs(nLOD);

	//for (uint32 nMorphEffector=0; nMorphEffector<numMorphs; ++nMorphEffector)
	//{
	//	const CryModEffMorph& rMorphEffector = m_Morphing.m_arrMorphEffectors[nMorphEffector];
	//	int nMorphTargetId = rMorphEffector.getMorphTargetId ();
	//	if (nMorphTargetId < 0)
	//		continue;

	for (uint32 nMorphTargetId=0; nMorphTargetId<numMorphs; ++nMorphTargetId)
	{
		//CMorphTarget* pMorphSkin = m_pModel->getMorphSkin(nLOD, nMorphTargetId);
		//if (!pMorphSkin)
		//	continue;		

		CMorphTarget* pMorphSkin = m_pModel->getMorphSkin(nLOD, nMorphTargetId);
		if (!pMorphSkin)
			continue;		

		//float fBalance = rMorphEffector.m_Params.m_fBalance;
		//float fBalanceBase = 1.0f - fabs(fBalance);

		float fBlending = g_Test; //rMorphEffector.getBlending();

		uint32 numVerts = pMorphSkin->m_vertices.size();

		Vec3 UnpackedVertex;
		f32 ex = pMorphSkin->m_MTExtensions.x/255.0f;
		f32 ey = pMorphSkin->m_MTExtensions.y/255.0f;
		f32 ez = pMorphSkin->m_MTExtensions.z/255.0f;
		for(uint32 i=0; i<numVerts; i++) 
		{
			uint32 idx = pMorphSkin->m_vertices[i].m_nVertexId;

			float fVertexBlend = fBlending;

			// add all morph-values to the stream to use subsequently in skinning
			f32 x=f32(pMorphSkin->m_vertices[i].m_MTVertexX);
			f32 y=f32(pMorphSkin->m_vertices[i].m_MTVertexY);
			f32 z=f32(pMorphSkin->m_vertices[i].m_MTVertexZ);
			UnpackedVertex.x = x*ex+pMorphSkin->m_MTNegBasis.x;
			UnpackedVertex.y = y*ey+pMorphSkin->m_MTNegBasis.y;
			UnpackedVertex.z = z*ez+pMorphSkin->m_MTNegBasis.z;
			pTrgMorphData[idx] +=UnpackedVertex * fVertexBlend;

		} //i
	} // nmoprhs


	//uint32 numExtIndices	= m_pModel->GetRenderMesh(nLOD)->GetSysIndicesCount();
	uint32 numExtVertices	=	m_pModel->GetRenderMesh(nLOD)->GetVerticesCount();

	//uint32 numExtIndices	= m_pModel->GetModelMesh(nLOD)->GetSysIndicesCount();
	//uint32 numExtVertices	=	m_pModel->GetModelMesh(nLOD)->GetVertextCount();

	CModelMesh* pMesh = m_pModel->GetModelMesh(nLOD);
	if (!pMesh)
		return (0);

	DynArray<Vec3> arrExtSkinnedStream;	
	arrExtSkinnedStream.resize( numExtVertices );	

	const Vec3 vOrigin=vPos;

	pMesh->LockFullRenderMesh(nLOD);

	IRenderAuxGeom* g_pAuxGeom				= gEnv->pRenderer->GetIRenderAuxGeom();
	g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );

	for(uint32 e=0; e<numExtVertices; e++) 
	{	
		//create the final vertex for skinning (blend between 3 characters)
		ExtSkinVertex vertex = pMesh->GetSkinVertexNoInd(e);
		//arrExtVColors[e]=vertex.color;
		uint8 idx	=	vertex.color.a; 
		Vec3 v		= vertex.wpos0*VertexRegs[idx].x + vertex.wpos1*VertexRegs[idx].y + vertex.wpos2*VertexRegs[idx].z;

		v+=(vOrigin+pTrgMorphData[e]);					
		arrExtSkinnedStream[e] = v;

		if (pTrgMorphData[e].len2()>0)
			g_pAuxGeom->DrawSphere(v,0.005f,ColorB(255,255,0));
		else
			g_pAuxGeom->DrawSphere(v,0.005f,ColorB(0,255,0));

		//if (g_arrExtMorphStream.size())
		//	v+=g_arrExtMorphStream[e];
		//arrExtSkinnedStream[e] = m34*v;
	}
	pMesh->UnlockFullRenderMesh(nLOD);

	SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
	//renderFlags.SetFillMode( e_FillModeWireframe );
	//renderFlags.SetDrawInFrontMode( e_DrawInFrontOn );
	//renderFlags.SetAlphaBlendMode(e_AlphaAdditive);
	//g_pAuxGeom->SetRenderFlags( renderFlags );
	//g_pAuxGeom->DrawTriangles(&arrExtSkinnedStream[0],numExtVertices, pMesh->m_pIndices,numExtIndices,RGBA8(0x00,0xff,0x00,0x00));		

	return (lstIDs.size());
}

SPU_INDIRECT(CommandBufferExecute(M))
ICharacterModel* CSkinInstance::GetICharacterModel()
{ 
	return m_pModel; 
};

