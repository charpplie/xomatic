////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   statobjman.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Loading trees, buildings, ragister/unregister entities for rendering
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "terrain_sector.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "IndexedMesh.h"
#include "WaterVolumes.h"
#include "Brush.h"
#include "ICryAnimation.h"
#include "DecalRenderNode.h"
#include "FogVolumeRenderNode.h"

int CObjManager::m_nUpdateStreamingPrioriryRoundId = 1;
int CObjManager::s_nLastStreamingMemoryUsage = 0;


// PS3 Memory peak avoidance - If memory dangerously low, don't stream until memory has 
// recovered 























struct CObjManager_Cmp_Streamable_Priority
{
	
	// returns true if v1 < v2
	ILINE bool operator()( const SStreamAbleObject &v1, const SStreamAbleObject &v2) const
	{
		IStreamable * arrObj[2] = { v1.GetStreamAbleObject(), v2.GetStreamAbleObject() };

		// compare priorities
		if(v1.fCurImportance > v2.fCurImportance)
			return true;
		if(v1.fCurImportance < v2.fCurImportance)
			return false;

		// give low lod's and small meshes higher priority
		int MemUsage0 = v1.GetStreamableContentMemoryUsage();
		int MemUsage1 = v2.GetStreamableContentMemoryUsage();
		if(MemUsage0 < MemUsage1)
			return true;
		if(MemUsage0 > MemUsage1)
			return false;

		// fix sorting consistency
		if(arrObj[0] > arrObj[1])
			return true;
		if(arrObj[0] < arrObj[1])
			return false;

		//[AlexMc|04.02.10] This assert isn't correct if we use quicksort (we might compare the pivot against itself)
		//assert(!"Duplicated object found");

		return false;
	}
};

void CObjManager::RegisterForStreaming(IStreamable*pObj) 
{ 
	SStreamAbleObject streamAbleObject(pObj);
  if(m_arrStreamableObjects.Find(streamAbleObject)<0) 
    m_arrStreamableObjects.Add(streamAbleObject);  
}

void CObjManager::UnregisterForStreaming(IStreamable*pObj) 
{ 
	SStreamAbleObject streamAbleObject(pObj);
  m_arrStreamableObjects.Delete(streamAbleObject);
}

void CObjManager::UpdateObjectsStreamingPriority(bool bSyncLoad)
{
  for(int nPass=0; nPass<(1+2*(int)bSyncLoad); nPass++)
  {
    if(!m_arrUpdateStreamingPrioriryStack.Count())
    {
      FRAME_PROFILER( "UpdateObjectsStreamingPriority_Init", GetSystem(), PROFILE_3DENGINE );

      m_nUpdateStreamingPrioriryRoundId++;

//      if(GetCVars()->e_StreamCgf==2)
  //      PrintMessage("UpdateObjectsStreamingPriority_Restart");

      for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
        if(Get3DEngine()->m_pObjectsTree[nSID])
          m_arrUpdateStreamingPrioriryStack.Add(Get3DEngine()->m_pObjectsTree[nSID]);

      if(CVisArea * pRoot0 = (GetVisAreaManager() ? GetVisAreaManager()->GetCurVisArea() : NULL))
      {
        m_tmpAreas0.Clear();
        pRoot0->AddConnectedAreas(m_tmpAreas0, 5);

        for(int v=0; v<m_tmpAreas0.Count(); v++)
        {
          CVisArea * pN1 = m_tmpAreas0[v];
          assert(m_arrUpdateStreamingPrioriryStack.Find(pN1->m_pObjectsTree)<0);
          if(pN1->m_pObjectsTree)
            m_arrUpdateStreamingPrioriryStack.Add(pN1->m_pObjectsTree);
        }
      }
      else if(GetVisAreaManager())
      {
        // find portals around
        m_tmpAreas0.Clear();
        GetVisAreaManager()->MakeActiveEntransePortalsList(NULL, m_tmpAreas0, NULL);

        // make list of areas for streaming
        m_tmpAreas1.Clear();
        for(int p=0; p<m_tmpAreas0.Count(); p++)
          if(CVisArea * pRoot = m_tmpAreas0[p])
            pRoot->AddConnectedAreas(m_tmpAreas1, 5);

        // fill list of object trees
        for(int v=0; v<m_tmpAreas1.Count(); v++)
        {
          CVisArea * pN1 = m_tmpAreas1[v];
          assert(m_arrUpdateStreamingPrioriryStack.Find(pN1->m_pObjectsTree)<0);
          if(pN1->m_pObjectsTree)
            m_arrUpdateStreamingPrioriryStack.Add(pN1->m_pObjectsTree);
        }
      }
    }

    {
      const float fStartTime = GetTimer()->GetAsyncCurTime();

      int nMaxNodesToVisit = CLAMP((int)(GetTimer()->GetFrameTime()*1000.f), 4, 128);
      for(int i=0; i<nMaxNodesToVisit || (bSyncLoad && m_arrUpdateStreamingPrioriryStack.Count()); i++)
      {    
        FRAME_PROFILER( "UpdateObjectsStreamingPriority_MarkNodes", GetSystem(), PROFILE_3DENGINE );

        if(m_arrUpdateStreamingPrioriryStack.Count())
        {
          COctreeNode * pLast = m_arrUpdateStreamingPrioriryStack.Last();
          m_arrUpdateStreamingPrioriryStack.DeleteLast();
          pLast->UpdateStreamingPrioriry(m_arrUpdateStreamingPrioriryStack);
        }

        if(!bSyncLoad)
        {
          float fTime = GetTimer()->GetAsyncCurTime();
          if( (fTime - fStartTime) > 0.0005f )
            break;
        }
      }
    }
  }
}

static bool AreTexturesStreamed(IMaterial* pMaterial, float fMipFactor)
{
	// check texture streaming distances
	if(pMaterial)
	{
		if(IRenderShaderResources * pRes = pMaterial->GetShaderItem().m_pShaderResources)
		{
			for(int iSlot = 0;iSlot<EFTT_MAX;++iSlot)
			{
				if(SEfResTexture* pResTex = pRes->GetTexture(iSlot))
				{
					if(ITexture * pITex = pResTex->m_Sampler.m_pITex)
					{
						float fCurMipFactor = fMipFactor * pResTex->m_TexModificator.m_Tiling[0] * pResTex->m_TexModificator.m_Tiling[1];

						if(!pITex->IsParticularMipStreamed(fCurMipFactor))
							return false;
					}
				}
			}
		}
	}

	return true;
}

void CObjManager::CheckTextureReadyFlag()
{
  FUNCTION_PROFILER_3DENGINE;

  if(!m_lstStaticTypes.size())
    return;

  static uint32 nGroupId = 0;

  if(nGroupId>=m_lstStaticTypes.size())
    nGroupId=0;

	for (size_t currentGroup = 0; currentGroup < m_lstStaticTypes.size(); currentGroup++)
	{
		StatInstGroup & rGroup = m_lstStaticTypes[currentGroup];

		if(CStatObj * pStatObj = rGroup.GetStatObj())
		{
			for(int j=0;j<FAR_TEX_COUNT;++j)
			{
				SVegetationSpriteLightInfo& rLightInfo = rGroup.m_arrSSpriteLightInfo[j];
				if(rLightInfo.m_pDynTexture && rLightInfo.m_pDynTexture->GetFlags() & IDynTexture::fNeedRegenerate)
				{
					IMaterial* pMaterial = rGroup.pMaterial;
					if(pMaterial == NULL)
						pMaterial = pStatObj->GetMaterial();

					IRenderMesh *pRenderMesh = pStatObj->GetRenderMesh();
					bool texturesStreamedIn = true;

					if (pRenderMesh)
					{
						PodArray<CRenderChunk> *pChunks = &pRenderMesh->GetChunks();

						if (pChunks)
						{
							for (size_t i = 0; i < pChunks->size(); i++)
							{
								CRenderChunk &currentChunk = (*pChunks)[i];
								IMaterial *pCurrentSubMtl = pMaterial->GetSafeSubMtl(currentChunk.m_nMatID);
								float fMipFactor = rLightInfo.m_MipFactor*currentChunk.m_texelAreaDensity;

								assert(pCurrentSubMtl);
								pCurrentSubMtl->PrecacheTextures(fMipFactor,FPR_STARTLOADING,m_nUpdateStreamingPrioriryRoundId);

								if (currentGroup == nGroupId)
								{
									if (!AreTexturesStreamed(pCurrentSubMtl,fMipFactor))
									{
										texturesStreamedIn = false;
									}
								}
							}
						}
					}

					if (currentGroup == nGroupId)
					{
						rGroup.nTexturesAreStreamedIn = texturesStreamedIn;
					}
				}
			}
		}
	}

  nGroupId++;
}

void CObjManager::ProcessObjectsStreaming()
{
  FUNCTION_PROFILER_3DENGINE;

  if(!GetCVars()->e_StreamCgf || !GetVisAreaManager())
    return;

	// this assert is most likely triggered by forgetting to call 
	// 3dEngine::SyncProcessStreamingUpdate at the end of the frame, leading to multiple
	// updates, but not starting and/or stopping streaming tasks
	assert( m_bNeedProcessObjectsStreaming_Finish == false );
	if( m_bNeedProcessObjectsStreaming_Finish == true )
	{
		CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "ProcessObjectsStreaming invoked without a following ProcessObjectsStreaming_Finish, please check your update logic");
	}

  bool bSyncLoad = Get3DEngine()->IsStatObjSyncLoad();
  if(bSyncLoad && Get3DEngine()->IsShadersSyncLoad())
    PrintMessage("======== Pre-caching meshes, shaders and textures ========");
  else if(bSyncLoad)
    PrintMessage("======== Pre-caching meshes ========");

  UpdateObjectsStreamingPriority(bSyncLoad || (GetCVars()->e_StreamCgfDebug!=0) || (GetCVars()->e_StreamCgfDebugHeatMap!=0));

	m_bNeedProcessObjectsStreaming_Finish = true;











	{
		ProcessObjectsStreaming_Impl(bSyncLoad); // this function will be execute on SPU
	}

	// during precache don't run asynchrony and sync directly to ensure the ESYSTEM_EVENT_LEVEL_PRECACHED
	// event is send to activate the renderthread
	// this also applies to the editor, since it calls the render function multiple times and thus invoking the
	// function multiple times without syncing
	if( bSyncLoad || gEnv->IsEditor() )
	{
		ProcessObjectsStreaming_Finish();
	}

}

#if !defined(CRCYG_CM)
SPU_ENTRY(ProcessObjectsStreaming)
#endif
void CObjManager::ProcessObjectsStreaming_Impl(bool bSyncLoad)
{






	ProcessObjectsStreaming_Sort();
	ProcessObjectsStreaming_Release();
	ProcessObjectsStreaming_InitLoad(bSyncLoad);




}

SPU_NO_INLINE void CObjManager::ProcessObjectsStreaming_Sort()
{
	int nNumStreamableObjects = m_arrStreamableObjects.Count();
	if (nNumStreamableObjects > 0)
	{
		FRAME_PROFILER( "ProcessObjectsStreaming_Sort", GetSystem(), PROFILE_3DENGINE );

	  SStreamAbleObject* arrStreamableObjects = SPU_PTR_SELECT( &m_arrStreamableObjects[0], gSPUarrStreamableObjects );
	  assert(arrStreamableObjects);

		for(int i=0; i<nNumStreamableObjects; i++)
		{
			SStreamAbleObject &rObj = arrStreamableObjects[i];

			// use data of previous prediction round since current round is not finished yet
			int nRoundId = CObjManager::m_nUpdateStreamingPrioriryRoundId - 1;

			// compute importance of objects for selected nRoundId
			rObj.fCurImportance = -1000.f;

			for(int nRoundIdx=0; nRoundIdx<2; nRoundIdx++)
			{
				if(rObj.GetStreamAbleObject()->m_arrUpdateStreamingPrioriryRoundInfo[nRoundIdx].nRoundId == nRoundId)
				{
					rObj.fCurImportance = rObj.GetStreamAbleObject()->m_arrUpdateStreamingPrioriryRoundInfo[nRoundIdx].fMaxImportance;
					if(rObj.GetLastDrawMainFrameId() > (C3DEngine::GetMainFrameID() - 2) )
						rObj.fCurImportance += C3DEngine::GetCVars()->e_StreamCgfVisObjPriority;
					break;
				}
			}
		}

		std::sort( SPU_LOCAL_PTR(&arrStreamableObjects[0]), SPU_LOCAL_PTR(&arrStreamableObjects[nNumStreamableObjects]), CObjManager_Cmp_Streamable_Priority() );	
	}
}

SPU_NO_INLINE void CObjManager::ProcessObjectsStreaming_Release()
{
	FRAME_PROFILER( "ProcessObjectsStreaming_Release", GetSystem(), PROFILE_3DENGINE ); 
	int nMemoryUsage = 0;

	int nNumStreamableObjects = m_arrStreamableObjects.Count();
	SStreamAbleObject* arrStreamableObjects = SPU_PTR_SELECT( nNumStreamableObjects ? &m_arrStreamableObjects[0] :NULL, gSPUarrStreamableObjects );
	
	for(int nObjId=0; nObjId<nNumStreamableObjects; nObjId++)
	{
		const SStreamAbleObject &rObj = arrStreamableObjects[nObjId];

		nMemoryUsage += rObj.GetStreamableContentMemoryUsage();

		if(nMemoryUsage >= GetCVars()->e_StreamCgfPoolSize*1024*1024)
		{
			if(rObj.GetStreamAbleObject()->m_eStreamingStatus == ecss_Ready)
			{
				m_arrStreamableToRelease.push(rObj.GetStreamAbleObject());
			}  

			// remove from list if not active for long time
			if(rObj.GetStreamAbleObject()->m_eStreamingStatus == ecss_NotLoaded)
			{
				if(rObj.GetStreamAbleObject()->m_arrUpdateStreamingPrioriryRoundInfo[0].nRoundId < (CObjManager::m_nUpdateStreamingPrioriryRoundId-8))
				{
					rObj.GetStreamAbleObject()->m_arrUpdateStreamingPrioriryRoundInfo[0].nRoundId = 0;
					m_arrStreamableToDelete.push(rObj.GetStreamAbleObject());
				}
			}
		}
	}
	s_nLastStreamingMemoryUsage = nMemoryUsage;
}

SPU_NO_INLINE void CObjManager::ProcessObjectsStreaming_InitLoad(bool bSyncLoad)
{
	FRAME_PROFILER( "ProcessObjectsStreaming_InitLoad", GetSystem(), PROFILE_3DENGINE );
	int nInProgress=0;

	int nNumStreamableObjects = m_arrStreamableObjects.Count();
	SStreamAbleObject* arrStreamableObjects = SPU_PTR_SELECT( nNumStreamableObjects ? &m_arrStreamableObjects[0] :NULL, gSPUarrStreamableObjects );

	for(int nObjId = 0; nObjId<nNumStreamableObjects; nObjId++)
	{
		const SStreamAbleObject &rObj = arrStreamableObjects[nObjId];
		if(rObj.GetStreamAbleObject()->m_eStreamingStatus == ecss_InProgress)
			nInProgress++;
	}

	if((nInProgress < GetCVars()->e_StreamCgfMaxTasksInProgress)|| bSyncLoad)
	{
		int nMemoryUsage = 0;

		for(int nObjId = 0; nObjId<nNumStreamableObjects; nObjId++)
		{
			const SStreamAbleObject &rObj = arrStreamableObjects[nObjId];

			int size = rObj.GetStreamableContentMemoryUsage();
			nMemoryUsage += size; 
			if(nMemoryUsage >= GetCVars()->e_StreamCgfPoolSize*1024*1024)
				break;

			if(rObj.GetStreamAbleObject()->m_eStreamingStatus == ecss_NotLoaded)
			{
				m_arrStreamableToLoad.push( rObj.GetStreamAbleObject() );				
	
				if(!bSyncLoad)
					break;
			}
		}
	}
}

void CObjManager::ProcessObjectsStreaming_Finish()
{
	if( m_bNeedProcessObjectsStreaming_Finish == false )
		return;
	
	m_bNeedProcessObjectsStreaming_Finish = false;
	
	FRAME_PROFILER( "ProcessObjectsStreaming_Finish", GetSystem(), PROFILE_3DENGINE );
	bool bSyncLoad = Get3DEngine()->IsStatObjSyncLoad();	
	
	{
		LOADING_TIME_PROFILE_SECTION(GetSystem());

	// make sure asynchron execution has finished







		

	bool bUnloaded = false;
	// now unload the stat object
	while( !m_arrStreamableToRelease.empty() )
	{
		bUnloaded = true;
		IStreamable *pStatObj = m_arrStreamableToRelease.pop();
		pStatObj->ReleaseStreamableContent();		

		if(GetCVars()->e_StreamCgfDebug==2)
		{
			string sName;
			pStatObj->GetStreamableName(sName);
			PrintMessage("Unloaded: %s", sName.c_str());
		}
	}
	
	// start streaming of stat objects
	while( !m_arrStreamableToLoad.empty() )
	{
		IStreamable *pStatObj = m_arrStreamableToLoad.pop();






		
		if(GetCVars()->e_StreamCgfDebug==2)
		{
			string sName;
			pStatObj->GetStreamableName(sName);
			PrintMessage("Loading: %s", sName.c_str());
		}

		IReadStreamPtr stream;
		pStatObj->StartStreaming( bSyncLoad, bSyncLoad?&stream:NULL );
		// temporary disabled for PS3 (magic crash)
		if(bSyncLoad && stream)
			stream->Wait();
	}
	
	// remove no longer needed objects from list
	while( !m_arrStreamableToDelete.empty() )
	{
		IStreamable *pStatObj = m_arrStreamableToDelete.pop();
		SStreamAbleObject stramAbleObject(pStatObj);
		m_arrStreamableObjects.Delete(stramAbleObject);
	}
	}

  if (bSyncLoad) 
  {
    PrintMessage("======== done pre-cacheing meshes ========");

		{
			LOADING_TIME_PROFILE_SECTION(GetSystem());
			gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_PRECACHED, NULL, NULL);
		}
	}
		gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_PRECACHE_END, NULL, NULL);


  // Peak memory avoidance: 
  // Send event that the engine has finished streaming initial content as soon as nothing is in flight anymore
  // Very, very workaround - and should be removed after crysis2 ms1 





















    
}

void CObjManager::GetObjectsStreamingStatus(I3DEngine::SObjectsStreamingStatus &outStatus)
{
	outStatus.nReady = outStatus.nInProgress = outStatus.nTotal = outStatus.nAllocatedBytes = outStatus.nMemRequired = 0;

  for (LoadedObjects::iterator it = m_lstLoadedObjects.begin(); it != m_lstLoadedObjects.end(); ++it)
  {
    CStatObj *pStatObj = *it;
    if(pStatObj->m_bSubObject)
      continue;

    if(pStatObj->m_pLod0)
      continue;

    for(int l=0; l<MAX_STATOBJ_LODS_NUM; l++)
      if(CStatObj * pLod = (CStatObj *)pStatObj->GetLodObject(l))
        outStatus.nTotal++;
  }

  outStatus.nActive = m_arrStreamableObjects.Count();

  for(int nObjId = 0; nObjId<m_arrStreamableObjects.Count(); nObjId++)
  {
    const SStreamAbleObject &rStreamAbleObject = m_arrStreamableObjects[nObjId];

    if(rStreamAbleObject.GetStreamAbleObject()->m_eStreamingStatus == ecss_Ready)
      outStatus.nReady++;
    
    if(rStreamAbleObject.GetStreamAbleObject()->m_eStreamingStatus == ecss_InProgress)
      outStatus.nInProgress++;

    if(rStreamAbleObject.GetStreamAbleObject()->m_eStreamingStatus == ecss_Ready)
      outStatus.nAllocatedBytes += rStreamAbleObject.GetStreamableContentMemoryUsage();

    if(rStreamAbleObject.GetStreamAbleObject()->m_arrUpdateStreamingPrioriryRoundInfo[0].nRoundId >= (CObjManager::m_nUpdateStreamingPrioriryRoundId-4))
      outStatus.nMemRequired += rStreamAbleObject.GetStreamableContentMemoryUsage();
  }
}

void CObjManager::PrecacheStatObjMaterial( IMaterial* pMaterial, const float fEntDistance, IStatObj *pStatObj )
{
  FUNCTION_PROFILER_3DENGINE;

	if (pStatObj)
	{
		for (int i = 0; i < pStatObj->GetSubObjectCount(); i++)
		{
			PrecacheStatObjMaterial(pMaterial,fEntDistance,pStatObj->GetSubObject(i)->pStatObj);
		}

		PrecacheMaterial(pMaterial,fEntDistance,pStatObj->GetRenderMesh());
	}
}

void CObjManager::PrecacheMaterial( IMaterial* pMaterial, const float fEntDistance, IRenderMesh *pRenderMesh )
{
  FUNCTION_PROFILER_3DENGINE;
	LOADING_TIME_PROFILE_SECTION(GetSystem());


	// updating texture streaming distances
	if (pRenderMesh)
	{
		if (!pMaterial)
		{
			pMaterial = pRenderMesh->GetMaterial();
		}

		if (pMaterial)
		{
			PodArray<CRenderChunk> *pChunks = &pRenderMesh->GetChunks();

			if (pChunks != NULL)
			{
				for (uint32 i = 0; i < pChunks->Size(); i++)
				{
					//Diesel cut
					//pMaterial->PrecacheChunkTextures(fEntDistance,0,m_nUpdateStreamingPrioriryRoundId,&(*pChunks)[i]);
				//Diesel new
					//pMaterial->PrecacheChunkTextures(fEntDistance*400,NULL,m_nUpdateStreamingPrioriryRoundId,&(*pChunks)[i]);
				
				}
			}

			pChunks = pRenderMesh->GetChunksSkinned();

			if (pChunks != NULL)
			{
				for (uint32 i = 0; i < pChunks->Size(); i++)
				{
//Diesel cut
					pMaterial->PrecacheChunkTextures(fEntDistance,0,m_nUpdateStreamingPrioriryRoundId,&(*pChunks)[i]);
//Diesel new				
				//pMaterial->PrecacheChunkTextures(fEntDistance*400,NULL,m_nUpdateStreamingPrioriryRoundId,&(*pChunks)[i]);
				
				}
			}
		}
	}
	else
	{
		if (pMaterial)
		{
//Diesel cut
			pMaterial->PrecacheChunkTextures(fEntDistance,0,m_nUpdateStreamingPrioriryRoundId,NULL);
//Diesel new		
		//pMaterial->PrecacheChunkTextures(fEntDistance*400,NULL,m_nUpdateStreamingPrioriryRoundId,NULL);
		
		}
	}
}

void CObjManager::PrecacheCharacter(IRenderNode * pObj, const float fImportance,  ICharacterInstance * pCharacter, IMaterial* pSlotMat, const Matrix34& matParent, const float fEntDistance, const float fScale, int nMaxDepth )
{
  if(!nMaxDepth || !pCharacter)
    return;
  nMaxDepth--;

	if(IAttachmentManager * pAttMan = pCharacter->GetIAttachmentManager())
  {
		if(ICharacterInstance *pCharInstance = pAttMan->GetSkinInstance())
		{
			if(pCharInstance != pCharacter)
			{
				PrecacheCharacter(pObj, fImportance, pCharInstance, pSlotMat, matParent, fEntDistance, fScale, nMaxDepth);
			}
		}
		if(ICharacterInstance *pCharInstance = pAttMan->GetSkelInstance())
		{
			if(pCharInstance != pCharacter)
			{
				PrecacheCharacter(pObj, fImportance, pCharInstance, pSlotMat, matParent, fEntDistance, fScale, nMaxDepth);
			}
		}

    int nCount = pAttMan->GetAttachmentCount();
    for(int i=0; i<nCount; i++)
    {
      if(IAttachment * pAtt = pAttMan->GetInterfaceByIndex(i))
      {
        if(IAttachmentObject * pAttObj = pAtt->GetIAttachmentObject())
        {
					IMaterial* pAttMatOverride = pAttObj->GetMaterialOverride();
					IMaterial* pAttMat = pSlotMat ? pSlotMat : (pAttMatOverride ? pAttMatOverride : pAttObj->GetMaterial());
          if(ICharacterInstance *pCharInstance = pAttObj->GetICharacterInstance())
						PrecacheCharacter(pObj, fImportance, pCharInstance, pAttMat, matParent, fEntDistance, fScale, nMaxDepth);

          if(CStatObj * pStatObj = (CStatObj *)pAttObj->GetIStatObj())
          {
            if(!pStatObj || pStatObj->GetFlags()&STATIC_OBJECT_HIDDEN)
              continue;

            const QuatT q = pAtt->GetAttAbsoluteDefault();

            Matrix34A tm34 = matParent*Matrix34(q);
            pStatObj->UpdateStreamableComponents(fImportance, tm34, pObj, fEntDistance);

						IMaterial* pStatObjMat = pStatObj->GetMaterial();
						PrecacheStatObjMaterial(pAttMat ? pAttMat : pStatObjMat, fEntDistance / fScale, pStatObj);
          }
        }
      }
    }

		IMaterial* pCharObjMat = pCharacter->GetMaterial();

		IRenderMesh *pRenderMesh = pCharacter->GetRenderMesh();
		if (pRenderMesh)
		{
			assert( !pRenderMesh->IsEmpty() );
			PrecacheMaterial(pSlotMat ? pSlotMat : pCharObjMat, fEntDistance / fScale, pRenderMesh);
		}
  }

	// joints
  if(ISkeletonPose * pSkeletonPose = pCharacter->GetISkeletonPose())
	{
		uint32 numJoints = pSkeletonPose->GetJointCount();

		// check StatObj attachments
		for(uint32 i=0; i<numJoints; i++)
		{
			CStatObj * pStatObj = (CStatObj *)pSkeletonPose->GetStatObjOnJoint(i);
			if(!pStatObj || pStatObj->GetFlags()&STATIC_OBJECT_HIDDEN)
				continue;

			Matrix34A tm34 = matParent*Matrix34(pSkeletonPose->GetAbsJointByID(i));
			pStatObj->UpdateStreamableComponents(fImportance, tm34, pObj, fEntDistance);

			IMaterial* pStatObjMat = pStatObj->GetMaterial();
			PrecacheStatObjMaterial(pSlotMat ? pSlotMat : pStatObjMat, fEntDistance / fScale, pStatObj);
		}
	}
}

void CObjManager::UpdateRenderNodeStreamingPrioriry(IRenderNode * pObj, float fEntDistance)
{
//  FUNCTION_PROFILER_3DENGINE;

  if(pObj->m_fWSMaxViewDist<0.01f)
    return;

  if( pObj->GetIntegrationType() && GetCVars()->e_VoxTerHideIntegrated && Get3DEngine()->GetIVoxTerrain() )
    return; // baked into terrain

  // check cvars
  switch(pObj->GetRenderNodeType())
  {
  case eERType_Brush:
    if(!GetCVars()->e_Brushes) return;
    break;
  case eERType_Vegetation:
    if(!GetCVars()->e_Vegetation) return;
    break;
  case eERType_ParticleEmitter:
    if(!GetCVars()->e_Particles) return;
    break;
  case eERType_Decal:
    if(!GetCVars()->e_Decals) return;
    break;
  case eERType_VoxelObject:
    if(!GetCVars()->e_Voxel) return;
    break;
  case eERType_WaterWave:
    if(!GetCVars()->e_WaterWaves) return;
    break;
  case eERType_Road:
    if(!GetCVars()->e_Roads) return;
    break;
  default:
    if(!GetCVars()->e_Entities) return;
    break;
  }

  float fImportance = 1.f - (fEntDistance / ((pObj->m_fWSMaxViewDist + GetCVars()->e_StreamCgfPredicitionDistance)));

  if(fImportance<0 || pObj->m_dwRndFlags&ERF_HIDDEN)
    return;

  float fObjScale = 1.f;
	AABB objBox(pObj->GetBBox());

  if(pObj->GetRenderNodeType() == eERType_Decal)
  {
    CDecalRenderNode * pDecal = (CDecalRenderNode *)pObj;
    fObjScale = max(0.001f,pDecal->GetMatrix().GetColumn0().GetLength());
  }
  else if(pObj->GetRenderNodeType() == eERType_Vegetation)
  {
    CVegetation * pVeg = (CVegetation *)pObj;
    fObjScale = max(0.001f,pVeg->GetScale());
    fObjScale /= max(0.01f,pVeg->GetLodRatioNormalized());
  }
	else if(pObj->GetRenderNodeType() == eERType_Brush)
	{
		fObjScale = max(0.001f,((CBrush *)pObj)->GetMatrix().GetColumn0().GetLength());
	}
	else if(pObj->GetRenderNodeType() == eERType_FogVolume)
	{
		fObjScale = max(0.001f,((CFogVolumeRenderNode *)pObj)->GetMatrix().GetColumn0().GetLength());
	}

	float fInvObjScale = 1.0f / fObjScale;

	int lod = CObjManager::GetObjectLOD(fEntDistance, pObj->GetLodRatioNormalized(), objBox.GetRadius());

	IMaterial* pRenderNodeMat = pObj->GetMaterialOverride();
	{
		IStatObj *pStatObj = pObj->GetEntityStatObj(lod);
		if (pStatObj != NULL)
		{
			PrecacheStatObjMaterial(pRenderNodeMat, fEntDistance * fInvObjScale, pStatObj);
		}
		else
		{
			PrecacheMaterial(pRenderNodeMat, fEntDistance * fInvObjScale, pObj->GetRenderMesh(lod));
		}
	}

  if(pObj->GetRenderNodeType() == eERType_ParticleEmitter)
  {
    IParticleEmitter* pEmitter = (IParticleEmitter*)pObj;
    Matrix34A tm34A; tm34A.SetIdentity();
    tm34A.SetTranslation(pEmitter->GetPos());
    pEmitter->UpdateStreamableComponents(fImportance, tm34A, pObj, fEntDistance);
    return;
  }

  if(pObj->m_pRNTmpData)
  {
    pObj->m_pRNTmpData->userData.nLod = lod;

    if(pObj->GetRndFlags()&ERF_OUTDOORONLY && pObj->m_pRNTmpData->userData.pRenderObject)
    {
      ColorF ambColor = Get3DEngine()->GetSkyColor();
      pObj->m_pRNTmpData->userData.pRenderObject->m_II.m_AmbColor.r = ambColor.r;
      pObj->m_pRNTmpData->userData.pRenderObject->m_II.m_AmbColor.g = ambColor.g;
      pObj->m_pRNTmpData->userData.pRenderObject->m_II.m_AmbColor.b = ambColor.b;
    }
/*
    if(pObj->GetRenderNodeType() == eERType_Vegetation)
    {
      CVegetation*pVeg = (CVegetation*)pObj;
      pVeg->UpdateBending();
    }    
    */
  }

  const int nSlotCount = pObj->GetSlotCount();

  for(int nEntSlot = 0; nEntSlot<nSlotCount; nEntSlot++)
  {
		IMaterial* pSlotMat = pObj->GetEntitySlotMaterial(nEntSlot);
		if(!pSlotMat)
			pSlotMat = pRenderNodeMat;

    Matrix34A matParent;

    CStatObj * pStatObj = (CStatObj *)pObj->GetEntityStatObj(nEntSlot, 0, &matParent, true);
    if (pStatObj)
    {
      pStatObj->UpdateStreamableComponents(fImportance, matParent, pObj, fEntDistance);
      pStatObj->UpdateSubObjectsMerging();

			IMaterial* pStatObjMat = pStatObj->GetMaterial();
			PrecacheStatObjMaterial(pSlotMat ? pSlotMat : pStatObjMat, fEntDistance * fInvObjScale, pStatObj);
    }

    if(ICharacterInstance * pChar = pObj->GetEntityCharacter(nEntSlot, &matParent, true))
    {
      PrecacheCharacter(pObj, fImportance, pChar, pSlotMat, matParent, fEntDistance, fObjScale, 2);
    }
  }
}

#undef USE_SPU
