////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   statobjconstr.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: loading
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "StatObj.h"
#include "IndexedMesh.h"
#include "../RenderDll/Common/Shadow_Renderer.h"
#include <IRenderer.h>
#include <CrySizer.h>

#include "CGF/CGFLoader.h"
#include "CGF/CGFSaver.h"
#include "CGF/ReadOnlyChunkFile.h"

#define GEOM_INFO_FILE_EXT "ginfo"
#define MESH_NAME_FOR_MAIN "main"

extern const char* stristr(const char* szString, const char* szSubstring);

extern void TransformMesh( CMesh &mesh,Matrix34 tm );

void CStatObj::StreamAsyncOnComplete(IReadStream* pStream, unsigned nError)
{
  FUNCTION_PROFILER_3DENGINE;

	pStream->SetUserData((DWORD_PTR)NULL);

	if(pStream->IsError())
	{ // file was not loaded successfully
    m_eStreamingStatus = ecss_NotLoaded;
    Error("CStatObj::StreamAsyncOnComplete: Error loading CGF: %s Error# %d", m_szFileName.c_str(), nError);
		return;
	}

	ObjMeshPairs* pObjMeshPairs = new ObjMeshPairs;

  if(!LoadCGF_FromMemBlock( pObjMeshPairs, pStream->GetBuffer(), pStream->GetBytesRead(), m_pLod0!=0, false ))
  {
    Error("CStatObj::StreamOnComplete_LoadCGF_FromMemBlock, filename=%s", m_szFileName.c_str());
  }

	pStream->SetUserData((DWORD_PTR)pObjMeshPairs);
}

void CStatObj::StreamOnComplete(IReadStream* pStream, unsigned nError)
{
	FUNCTION_PROFILER_3DENGINE;

	ObjMeshPairs* pObjMeshPairs = (ObjMeshPairs*)pStream->GetParams().dwUserData;

	if(pStream->IsError())
	{ // file was not loaded successfully
		if(pObjMeshPairs)
		{
			for(ObjMeshPairs::iterator it = pObjMeshPairs->begin();it != pObjMeshPairs->end();++it)
				delete it->second;
			delete pObjMeshPairs;
			pStream->SetUserData((DWORD_PTR)NULL);
		}
		m_eStreamingStatus = ecss_NotLoaded;
		Error("CStatObj::StreamOnComplete: Error loading CGF: %s Error# %d", m_szFileName.c_str(), nError);
		return;
	}

	if(pObjMeshPairs == NULL)
	{
		m_eStreamingStatus = ecss_NotLoaded;
		assert(0);
		Error("CStatObj::StreamOnComplete: Error postprocessing CGF: %s", m_szFileName.c_str());
		return;
	}

  for(uint32 nGroupId=0; nGroupId<m_pObjManager->m_lstStaticTypes.size(); nGroupId++)
  {
    StatInstGroup & rGroup = m_pObjManager->m_lstStaticTypes[nGroupId];
    if(rGroup.pStatObj == this)
      rGroup.Update(GetCVars(), Get3DEngine()->GetGeomDetailScreenRes());
  }

  m_eStreamingStatus = ecss_Ready;

  m_nStreamingFinishedMainFrameId = GetMainFrameID();

	// setting up render meshes
	for(ObjMeshPairs::iterator it = pObjMeshPairs->begin();it != pObjMeshPairs->end();++it)
	{
		if(it->first)
			it->first->SetRenderMesh(it->second);
	}

	delete pObjMeshPairs;

  m_pReadStream = 0;
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::StartStreaming( bool bFinishNow, IReadStream_AutoPtr* ppStream )
{
  assert(!m_pParentObject);

  assert(m_eStreamingStatus == ecss_NotLoaded);

	if(m_eStreamingStatus != ecss_NotLoaded)
		return;

	// start streaming
	StreamReadParams params;
	params.dwUserData = 0;
	params.nSize = 0;
	params.pBuffer = NULL;
	params.nLoadTime = 10000;
	params.nMaxLoadTime = 10000;
	params.nFlags |= SRP_FLAGS_ASYNC_PROGRESS;

#ifdef _DEBUG
	params.nFlags |= SRP_FLAGS_FORCE_SYNC_CALLBACKS;
#endif

  if(m_szFileName.empty())
  {
    assert(!"CStatObj::StartStreaming: CGF name is empty");
    m_eStreamingStatus = ecss_Ready;
		if(ppStream)*ppStream = NULL;
    return;
  }

	m_pReadStream = GetSystem()->GetStreamEngine()->StartRead(eStreamTaskTypeGeometry, m_szFileName, this, &params);
	if(ppStream)
		(*ppStream) = m_pReadStream;
	if(!bFinishNow)
		m_eStreamingStatus = ecss_InProgress;
	else if(!ppStream)
		m_pReadStream->Wait();
}

bool CStatObj::LoadCGF_FromMemBlock( ObjMeshPairs* ppOutputMeshes, const void * pData, int nDataSize, bool bLod, bool bPhysicalize )
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_CGF, 0, "%s", this->GetFilePath());

  FUNCTION_PROFILER_3DENGINE;
	LOADING_TIME_PROFILE_SECTION(GetSystem());

  if (m_bSubObject) // Never execute this on the sub objects.
    return true;

  CContentCGF *pCGF = NULL;

  //////////////////////////////////////////////////////////////////////////
  // Load CGF.
  //////////////////////////////////////////////////////////////////////////
  class Listener : public ILoaderCGFListener
  {
  public:
    virtual void Warning( const char *format ) {Cry3DEngineBase::Warning("%s", format);}
    virtual void Error( const char *format ) {Cry3DEngineBase::Error("%s", format);}
    virtual bool IsValidationEnabled( ) { return Cry3DEngineBase::GetCVars()->e_StatObjValidate != 0; }
  };

	// Must be in it`s own scope.
	CLoaderCGF cgfLoader;
	CReadOnlyChunkFile chunkFile( false,bLod ); // Chunk file must exist until CGF is completly loaded
	
	if (chunkFile.ReadFromMemBlock( pData,nDataSize ))
	{
		pCGF = cgfLoader.LoadCGF( m_szFileName.c_str(),chunkFile,0,0 );
	}
	if (!pCGF)
	{
		FileWarning( 0,m_szFileName,"CGF Loading from memblock failed: %s",cgfLoader.GetLastError() );
		return false;
	}
  //////////////////////////////////////////////////////////////////////////

  CExportInfoCGF *pExportInfo = pCGF->GetExportInfo();
  CNodeCGF *pFirstMeshNode = NULL;
  m_nSubObjectMeshCount = 0;

	if (!pExportInfo->bCompiledCGF)
	{
		FileWarning( 0,m_szFileName,"CGF is not compiled, use RC" );
		return false;
	}

  bool bHasJoints = false;

  //////////////////////////////////////////////////////////////////////////
  // Find out number of meshes, and get pointer to the first found mesh.
  //////////////////////////////////////////////////////////////////////////
  for (int i = 0; i < pCGF->GetNodeCount(); i++)
  {
    CNodeCGF *pNode = pCGF->GetNode(i);
    if (pNode->pMesh && (pNode->type == CNodeCGF::NODE_MESH))
    {
      if (m_szProperties.empty())
      {
        m_szProperties = pNode->properties; // Take properties from the first mesh node.
        m_szProperties.MakeLower();
      }
      m_nSubObjectMeshCount++;
      if (!pFirstMeshNode)
        pFirstMeshNode = pNode;
    }	
    else if (strncmp(pNode->name,"$joint",6)==0)
      bHasJoints = true;
  }

  bool bIsLod0Merged = false;
  if (bLod && m_pLod0)
  {
    // This is a log object, check if parent was merged or not.
    bIsLod0Merged = m_pLod0->m_nSubObjectMeshCount == 0;
  }

  if (pExportInfo->bMergeAllNodes || (m_nSubObjectMeshCount <= 1 && !bHasJoints && (!bLod || bIsLod0Merged)))
  {
    // If we merging all nodes, ignore sub object meshes.
    m_nSubObjectMeshCount = 0;

    //if (pCGF->GetCommonMaterial())
    //  m_pMaterial = GetMatMan()->LoadCGFMaterial( pCGF->GetCommonMaterial(),m_szFileName );
  }

  // Common of all sub nodes bbox.
  AABB commonBBox;
  commonBBox.Reset();

//  bool bHaveMeshNamedMain = false;

  //////////////////////////////////////////////////////////////////////////
  // Create StatObj from Mesh.
  //////////////////////////////////////////////////////////////////////////

  IRenderMesh* pMainMesh = NULL;

  if (pExportInfo->bMergeAllNodes || m_nSubObjectMeshCount == 0)
  {
    CMesh *pMesh = NULL;
   
    if (!pMesh && pFirstMeshNode)
      pMesh = pFirstMeshNode->pMesh;

    // Not support not merged mesh yet.
    if (!pMesh)
    {
      delete pCGF;
      return false;
    }

		// add mesh to sync setup queue
    if (!SetFromMesh( &pMainMesh, pMesh, true ))
    {
      delete pCGF;
      return false;
    }
		ppOutputMeshes->insert(std::make_pair(this, pMainMesh));

    if(bPhysicalize)
    {
			if (pFirstMeshNode)
      {
        PhysicalizeCompiled( pFirstMeshNode );
      }
    }
  }
  //////////////////////////////////////////////////////////////////////////

	std::vector<CNodeCGF*> nodes;
  //////////////////////////////////////////////////////////////////////////
  // Create SubObjects.
  //////////////////////////////////////////////////////////////////////////
  if (pCGF->GetNodeCount() > 1 || m_nSubObjectMeshCount > 0)
  {
    nodes.reserve( pCGF->GetNodeCount() );

    int nNumMeshes = 0;
    int ii;
    for (ii = 0; ii < pCGF->GetNodeCount(); ii++)
    {
      CNodeCGF *pNode = pCGF->GetNode(ii);

			if (pNode->bPhysicsProxy)
				continue;

      SSubObject subObject;
      subObject.pStatObj = 0;
      subObject.bIdentityMatrix = pNode->bIdentityMatrix;
      subObject.bHidden = false;
      subObject.tm = pNode->worldTM;
      subObject.localTM = pNode->localTM;
      subObject.name = pNode->name;
      subObject.properties = pNode->properties;
      subObject.nParent = -1;
      subObject.pWeights = 0;

      if (pNode->type == CNodeCGF::NODE_MESH)
      {
        if (pExportInfo->bMergeAllNodes || m_nSubObjectMeshCount == 0) // Only add helpers, ignore meshes.
          continue;

        nNumMeshes++;
        subObject.nType = STATIC_SUB_OBJECT_MESH;

//        if (stricmp(pNode->name,MESH_NAME_FOR_MAIN) == 0)
//          bHaveMeshNamedMain = true;
      }
      else if (pNode->type == CNodeCGF::NODE_LIGHT)
        subObject.nType = STATIC_SUB_OBJECT_LIGHT;
      else if (pNode->type == CNodeCGF::NODE_HELPER)
      {
        switch (pNode->helperType)
        {
        case HP_POINT:
          subObject.nType = STATIC_SUB_OBJECT_POINT;
          break;
        case HP_DUMMY:
          subObject.nType = STATIC_SUB_OBJECT_DUMMY;
          subObject.helperSize = (pNode->helperSize*0.01f);
          break;
        case HP_XREF:
          subObject.nType = STATIC_SUB_OBJECT_XREF;
          break;
        case HP_CAMERA:
          subObject.nType = STATIC_SUB_OBJECT_CAMERA;
          break;
        case HP_GEOMETRY:
          {
            subObject.nType = STATIC_SUB_OBJECT_HELPER_MESH;
            subObject.bHidden = true; // Helpers are not rendered.

#if !defined(XENON) && !defined(PS3) // occlusion proxies are not used on consoles, we use real depth buffer instead
            if (pNode->pMesh != 0 && pNode->pMesh->GetSubSetCount() != 0 && stristr(pNode->name,"$occlusion") != 0)
            {
              CStatObj *pStatObjOwner = this;
              if (!pExportInfo->bMergeAllNodes && m_nSubObjectMeshCount > 0 && pNode->pParent)
              {
                // We are attached to some object, find it.
				const int numNodes = nodes.size();
                for (int i = 0; i < numNodes; i++)
                  if (nodes[i] == pNode->pParent)
                  {
					  assert(i < m_subObjects.size());
			        if (i < m_subObjects.size())
					{
                      pStatObjOwner = (CStatObj*)m_subObjects[i].pStatObj;
					}
                    break;
                  }
              }
              if (!pStatObjOwner)
                continue;

              if (pStatObjOwner->m_pRenderMeshOcclusion)
              {
                continue;
              }

              if(!pNode->pMesh->GetIndexCount() || !pNode->pMesh->GetVertexCount())
              {
                Warning("Empty occlusion proxy found for object: %s, sub-object name is %s", pStatObjOwner->GetFilePath(), pStatObjOwner->GetGeoName());
                continue;
              }

              m_bHaveOcclusionProxy = true;
              pStatObjOwner->m_bHaveOcclusionProxy = true;
              pStatObjOwner->m_pRenderMeshOcclusion = GetRenderer()->CreateRenderMesh("OcclusionProxy",m_szFileName.c_str());
							// CreateRenderMesh create an object with an refcount of 1, by using a smart-ptr the refcount is incrased to 2, so use Release to correct the refcount
							pStatObjOwner->m_pRenderMeshOcclusion->Release();
              pStatObjOwner->m_pRenderMeshOcclusion->SetMaterial( GetMatMan()->GetDefaultMaterial() );
              TransformMesh( *pNode->pMesh,pNode->localTM );
              pStatObjOwner->m_pRenderMeshOcclusion->SetMesh( *pNode->pMesh );
              continue; // Do not add this sub node.
            }
#endif
          }
          break;
        default:
          assert(0); // unknown type.
        }
      }

      // Only when multiple meshes inside.
      // If only 1 mesh inside, Do not create a separate CStatObj for it.
      if ((m_nSubObjectMeshCount > 0 && pNode->type == CNodeCGF::NODE_MESH && pNode->pMesh != NULL) || 
        (subObject.nType == STATIC_SUB_OBJECT_HELPER_MESH))
      {
        if (!pNode->pSharedMesh) // If shared mesh, then do not create static object.
        {
          // Make inner StatObj.
          CStatObj *pStatObj = 0;//

          for(int nLod = 0; nLod<MAX_STATOBJ_LODS_NUM && !pStatObj; nLod++)
          {
            CStatObj * pLod = (CStatObj*)GetLodObject(nLod);

            if(!pLod)
              continue;

            if(pLod->m_nFileNodeId == pNode->nChunkId)
              if(!strncmp(pLod->m_FileNodeName, pNode->name, sizeof(pLod->m_FileNodeName)))
            {
              pStatObj = pLod;
              break;
            }
          }

          for(int s=0; s<GetSubObjectCount() && !pStatObj; s++)
          {
            SSubObject & sub = *GetSubObject(s);
            if(!sub.pStatObj)
              continue;

            for(int nLod = 0; nLod<MAX_STATOBJ_LODS_NUM && !pStatObj; nLod++)
            {
              CStatObj * pSubLod = (CStatObj*)sub.pStatObj->GetLodObject(nLod);

              if(!pSubLod)
                continue;

              if(pSubLod->m_nFileNodeId == pNode->nChunkId)
                if(!strncmp(pSubLod->m_FileNodeName, pNode->name, sizeof(pSubLod->m_FileNodeName)))
              {
                pStatObj = pSubLod;
                break;
              }
            }
          }         
          
          if(!pStatObj)
          {
            continue;
          }
//            pStatObj = new CStatObj;

          subObject.pStatObj = pStatObj;

          pStatObj->m_szFileName = m_szFileName;
          pStatObj->m_szGeomName = subObject.name;
          pStatObj->m_bSubObject = true;

          if (subObject.nType != STATIC_SUB_OBJECT_HELPER_MESH)
            pStatObj->m_pParentObject = this;
          pStatObj->m_szProperties = subObject.properties;
          pStatObj->m_szProperties.MakeLower();
          if (!pStatObj->m_szProperties.empty())
            pStatObj->ParseProperties();

          //if (pNode->pMaterial)
          //{
          //  pStatObj->m_pMaterial = GetMatMan()->LoadCGFMaterial( pNode->pMaterial,m_szFileName );
          //  if (!m_pMaterial || m_pMaterial->IsDefault())
          //    m_pMaterial = pStatObj->m_pMaterial; // take it as a general stat obj material.
          //}
          if (!pStatObj->m_pMaterial)
            pStatObj->m_pMaterial = m_pMaterial;

          assert(0==strcmp(pStatObj->GetGeoName(),pNode->name));

					// add mesh to sync setup queue
					IRenderMesh* pLodMesh = NULL;
					const bool res = pStatObj->SetFromMesh( &pLodMesh, pNode->pMesh, true );
          if(pLodMesh && res)
  					ppOutputMeshes->insert(std::make_pair(pStatObj, pLodMesh));

          if (pNode->bHasFaceMap)
            memcpy(pStatObj->m_pMapFaceToFace0=new uint16[pNode->pMesh->GetIndexCount()/3], &pNode->mapFaceToFace0[0], 
            (pNode->pMesh->GetIndexCount()/3)*sizeof(uint16));
          
          if(bPhysicalize)
          {
						pStatObj->PhysicalizeCompiled( pNode );
          }

          pStatObj->AnalizeFoliage(pLodMesh, pCGF);
					pStatObj->m_pSkinInfo = (SSkinVtx*)pNode->pSkinInfo; pNode->pSkinInfo=0;

          //////////////////////////////////////////////////////////////////////////
          //@TODO: Optimize this to not keep system memory copy Indexed mesh, for CGF objects.
          /*if (pStatObj->m_bHaveBreakablePhysics)
          {
          pStatObj->m_pIndexedMesh = new CIndexedMesh;
          pStatObj->m_pIndexedMesh->SetMesh( *pNode->pMesh );
          }*/
        }

        // Calc bbox.
        if (pNode->pMesh && pNode->type == CNodeCGF::NODE_MESH)
        {
          AABB box = pNode->pMesh->m_bbox;
          box.SetTransformedAABB( subObject.tm,box );
          commonBBox.Add( box.min );
          commonBBox.Add( box.max );
        }
      }

      //      m_subObjects.push_back(subObject);
      nodes.push_back(pNode);

      if (pNode->bHasFaceMap)
        m_bHasDeformationMorphs = true;
    }

    // Assign SubObject parent pointers.
    /*int nNumCgfNodes = (int)nodes.size();
    if (nNumCgfNodes > 0)
    {
      CNodeCGF **pNodes = &nodes[0];

      //////////////////////////////////////////////////////////////////////////
      // Move meshes to begining, Sort sub-objects so that meshes are first.
      for (i = 0; i < nNumCgfNodes; i++)
      {
        if (pNodes[i]->type != CNodeCGF::NODE_MESH)
        {
          // check if any more meshes exist.
          if (i < nNumMeshes)
          {
            // Try to find next mesh and place it here.
            for (int j = i+1; j < nNumCgfNodes; j++)
            {
              if (pNodes[j]->type == CNodeCGF::NODE_MESH)
              {
                // Swap objects at j to i.
                std::swap( pNodes[i],pNodes[j] );
                std::swap( m_subObjects[i],m_subObjects[j] );
                break;
              }
            }
          }
        }
      }
      //////////////////////////////////////////////////////////////////////////

      // Assign Shared Meshes.
      for (i = 0; i < nNumCgfNodes; i++)
      {
        if (pNodes[i]->pSharedMesh)
        {
          CNodeCGF *pSharedMesh = pNodes[i]->pSharedMesh;
          for (int j = 0; j < nNumCgfNodes; j++)
          {
            if (pNodes[j] == pSharedMesh)
            {
              CStatObj *pStatObj = (CStatObj*)m_subObjects[j].pStatObj;
              m_subObjects[i].pStatObj = pStatObj;
              if (pStatObj == this) {
                int a = 0;
              }
              if (pStatObj)
                pStatObj->m_nUsers++;
              break;
            }
          }
        }
      }

      // Assign Parent nodes.
      for (i = 0; i < nNumCgfNodes; i++)
      {
        CNodeCGF *pParentNode = pNodes[i]->pParent;
        if (pParentNode)
        {
          for (int j = 0; j < nNumCgfNodes; j++)
          {
            if (pNodes[j] == pParentNode)
            {
              m_subObjects[i].nParent = j;
              break;
            }
          }
        }
      }

      //////////////////////////////////////////////////////////////////////////
      // Handle Main/Remain meshes used for Destroyable Objects.
      //////////////////////////////////////////////////////////////////////////
      if (bHaveMeshNamedMain)
      {
        // If have mesh named main, then mark all sub object hidden except the one called "Main".
        for (int i = 0,n = m_subObjects.size(); i < n; i++)
        {
          if (m_subObjects[i].nType == STATIC_SUB_OBJECT_MESH)
          {
            if (stricmp(m_subObjects[i].name,MESH_NAME_FOR_MAIN) == 0)
              m_subObjects[i].bHidden = false;
            else
              m_subObjects[i].bHidden = true;
          }
        }
      }
      //////////////////////////////////////////////////////////////////////////
    }*/
  }

  if (m_nSubObjectMeshCount > 0)
  {
    m_vBoxMin = commonBBox.min;
    m_vBoxMax = commonBBox.max;
    CalcRadiuses();
  }

	if (bPhysicalize)
	{
		//////////////////////////////////////////////////////////////////////////
		// Physicalize physics proxy nodes.
		//////////////////////////////////////////////////////////////////////////
		for (int i = 0,numNodes = pCGF->GetNodeCount(); i < numNodes; i++)
		{
			CNodeCGF *pNode = pCGF->GetNode(i);
			if (pNode->bPhysicsProxy)
			{
				CStatObj *pStatObjParent = this;
				if (pNode->pParent)	for (int k=nodes.size()-1; k>=0; k--) 
					if (nodes[k]==pNode->pParent && m_subObjects[k].pStatObj)
					{
						pStatObjParent = (CStatObj*)m_subObjects[k].pStatObj;
						break;
					}
				pStatObjParent->PhysicalizeCompiled(pNode, 1);
			}
		}
	}

  //////////////////////////////////////////////////////////////////////////
  // Analyze foliage info.
  //////////////////////////////////////////////////////////////////////////
  if (pExportInfo->bMergeAllNodes || m_nSubObjectMeshCount == 0)
  {
    AnalizeFoliage(pMainMesh, pCGF);
  }
  //////////////////////////////////////////////////////////////////////////

  for (int i = 0; i < pCGF->GetNodeCount(); i++) if (strstr(pCGF->GetNode(i)->properties,"deformable"))
    m_nFlags |= STATIC_OBJECT_DEFORMABLE;
  if (m_nSubObjectMeshCount > 0)
    m_nFlags |= STATIC_OBJECT_COMPOUND;

  delete pCGF;

  if (m_bHasDeformationMorphs)
  {
    int i,j;
    for(i=GetSubObjectCount()-1;i>=0;i--) if ((j=SubobjHasDeformMorph(i))>=0)
      GetSubObject(i)->pStatObj->SetDeformationMorphTarget(GetSubObject(j)->pStatObj);
  }

/*  {
    FRAME_PROFILER( "CStatObj::LoadCGF_FromMemBlock_GetRenderMeshesMemoryUsage", GetSystem(), PROFILE_3DENGINE );

    ICrySizer * pSizer;

    pSizer = GetSystem()->CreateSizer();
    GetRenderMeshesMemoryUsage(pSizer, false);
    m_arrRenderMeshesMemoryUsage[0] = pSizer->GetTotalSize();
    pSizer->Release();
  
    pSizer = GetSystem()->CreateSizer();
    GetRenderMeshesMemoryUsage(pSizer, true);
    m_arrRenderMeshesMemoryUsage[1] = pSizer->GetTotalSize();
    pSizer->Release();
  }*/

  return true;
}

void CStatObj::ReleaseStreamableContent()
{
  assert(!m_pParentObject);

  bool bLodsAreLoadedFromSeparateFile = m_pLod0 ? m_pLod0->m_bLodsAreLoadedFromSeparateFile : m_bLodsAreLoadedFromSeparateFile;

  if(!bLodsAreLoadedFromSeparateFile)
  {
    for(int nLod = 0; nLod<MAX_STATOBJ_LODS_NUM; nLod++)
    {
      CStatObj * pLod = (CStatObj*)GetLodObject(nLod);

      if(!pLod)
        continue;

      pLod->SetRenderMesh(0);
      pLod->FreeFoliageData();
      pLod->m_eStreamingStatus = ecss_NotLoaded;

      if(pLod->m_pParentObject)
      {
        pLod->m_pParentObject->SetRenderMesh(0);
        pLod->m_pParentObject->FreeFoliageData();
        pLod->m_pParentObject->m_eStreamingStatus = ecss_NotLoaded;
      }
    }
  }

  for(int s=0; s<GetSubObjectCount(); s++)
  {
    SSubObject & sub = *GetSubObject(s);
    if(!sub.pStatObj)
      continue;

    if(bLodsAreLoadedFromSeparateFile)
    {
      CStatObj * pSubLod = (CStatObj*)sub.pStatObj;

      if(!pSubLod)
        continue;

      pSubLod->SetRenderMesh(0);
      pSubLod->FreeFoliageData();
      pSubLod->m_eStreamingStatus = ecss_NotLoaded;

      if(pSubLod->m_pParentObject)
      {
        pSubLod->m_pParentObject->SetRenderMesh(0);
        pSubLod->m_pParentObject->FreeFoliageData();
        pSubLod->m_pParentObject->m_eStreamingStatus = ecss_NotLoaded;
      }
    }
    else
    {
      for(int nLod = 0; nLod<MAX_STATOBJ_LODS_NUM; nLod++)
      {
        CStatObj * pSubLod = (CStatObj*)sub.pStatObj->GetLodObject(nLod);

        if(!pSubLod)
          continue;

        pSubLod->SetRenderMesh(0);
        pSubLod->FreeFoliageData();
        pSubLod->m_eStreamingStatus = ecss_NotLoaded;

        if(pSubLod->m_pParentObject)
        {
          pSubLod->m_pParentObject->SetRenderMesh(0);
          pSubLod->m_pParentObject->FreeFoliageData();
          pSubLod->m_pParentObject->m_eStreamingStatus = ecss_NotLoaded;
        }
      }
    }
  }         

  SetRenderMesh(0);
  FreeFoliageData();

  UnMergeSubObjectsRenderMeshes();

  m_eStreamingStatus = ecss_NotLoaded;
}

int CStatObj::GetStreamableContentMemoryUsage()
{
  assert(!m_pParentObject);

  bool bLodsAreLoadedFromSeparateFile = m_pLod0 ? m_pLod0->m_bLodsAreLoadedFromSeparateFile : m_bLodsAreLoadedFromSeparateFile;
  bool bCountLods = !bLodsAreLoadedFromSeparateFile;

  if(m_arrRenderMeshesPotentialMemoryUsage[bCountLods]<0)
  {
    int nCount=0;

    if(bCountLods)
    {
      for(int nLod = 1; nLod<MAX_STATOBJ_LODS_NUM; nLod++)
      {
        CStatObj * pLod = (CStatObj*)m_arrpLowLODs[nLod];

        if(!pLod)
          continue;

        nCount += pLod->m_nRenderMeshMemoryUsage;
      }
    }

    nCount += m_nRenderMeshMemoryUsage;

    for(int s=0; s<GetSubObjectCount(); s++)
    {
      SSubObject & sub = *GetSubObject(s);
      
      if(!sub.pStatObj)
        continue;

      if(bCountLods)
      {
        for(int nLod = 1; nLod<MAX_STATOBJ_LODS_NUM; nLod++)
        {
          CStatObj * pSubLod = ((CStatObj*)sub.pStatObj)->m_arrpLowLODs[nLod];

          if(!pSubLod)
            continue;

          nCount += pSubLod->m_nRenderMeshMemoryUsage;
        }
      }

      nCount += ((CStatObj*)sub.pStatObj)->m_nRenderMeshMemoryUsage;
    }         

    m_arrRenderMeshesPotentialMemoryUsage[bCountLods] = nCount;
  }

  if(m_pMergedObject)
  {
    if( IRenderMesh * pRM = m_pMergedObject->GetRenderMesh() )
      m_nMergedMemoryUsage = pRM->GetVerticesCount()*(sizeof(SPipTangents) + sizeof(SVF_P3S_C4B_T2S)) + pRM->GetIndicesCount()*sizeof(uint16);
    else
      m_nMergedMemoryUsage = 0;
  }
  else
  if(!GetCVars()->e_StatObjMerge)
  {
    m_nMergedMemoryUsage = 0;
  }

  return m_nMergedMemoryUsage + m_arrRenderMeshesPotentialMemoryUsage[bCountLods];
}

void CStatObj::UpdateStreamingPrioriryInternal(const Matrix34A & objMatrix, float fImportance)
{
  int nRoundId = GetObjManager()->m_nUpdateStreamingPrioriryRoundId;

  if(m_pParentObject && m_bSubObject) 
  { // stream parent for sub-objects
    m_pParentObject->UpdateStreamingPrioriryInternal(objMatrix, fImportance);
  }
  else if(!m_bSubObject)
  { // stream object itself
    if(m_bUseStreaming)
    {
      if(UpdateStreamingPrioriryLowLevel(fImportance, nRoundId))
        GetObjManager()->RegisterForStreaming(this);
    }
  }
  else if(m_pLod0)
  { // sub-object lod without parent
    m_pLod0->UpdateStreamingPrioriryInternal(objMatrix, fImportance);
    assert(!m_pLod0->m_bLodsAreLoadedFromSeparateFile);
  }
  else if(m_bUseStreaming)
    assert(!"Invalid CGF hierarchy");
}

void CStatObj::UpdateSubObjectsMerging()
{
  if (m_nFlags & STATIC_OBJECT_HIDDEN)
    return;

  if(m_eStreamingStatus != ecss_Ready)
    return;

  // sub meshes merging
  if (GetCVars()->e_StatObjMerge)
  {
    if (!m_bMerged && !m_bUnmergable)
      MergeSubObjectsRenderMeshes();
  }
  else
  {
    if(m_bMerged)
      UnMergeSubObjectsRenderMeshes();
  }
}

bool CStatObj::UpdateStreamableComponents(float fImportance, Matrix34A & objMatrix, IRenderNode * pRenderNode, float fEntDistance)
{
  FUNCTION_PROFILER_3DENGINE;

  if(m_nFlags&STATIC_OBJECT_HIDDEN)
    return false;

  if ((m_nFlags & STATIC_OBJECT_COMPOUND) && SubObjectCount())
  {
    for(int s=0,num = SubObjectCount(); s<num; s++)
    {
      IStatObj::SSubObject &subObj = SubObject(s);

      if (subObj.pStatObj && !subObj.bHidden && subObj.nType == STATIC_SUB_OBJECT_MESH)
      {
        Matrix34 subObjMatrix = objMatrix * subObj.tm;

        CStatObj *pSubStatObj = ((CStatObj*)subObj.pStatObj);

        if(pSubStatObj->m_nRenderTrisCount<3)
          continue;

        int nNewLod = pSubStatObj->GetLod(subObjMatrix, fEntDistance, pRenderNode);

        for(int l=nNewLod; l<=(nNewLod+1) && l<MAX_STATOBJ_LODS_NUM; l++)
          if(CStatObj * pLod = (CStatObj *)pSubStatObj->GetLodObject(l))
            pLod->UpdateStreamingPrioriryInternal(objMatrix, fImportance);
      }
    }
  }
  else if(m_nRenderTrisCount>=3)
  {
    Matrix34 objMatrixNA = objMatrix;

    int nNewLod = GetLod(objMatrixNA, fEntDistance, pRenderNode);

    for(int l=nNewLod; l<=(nNewLod+1) && l<MAX_STATOBJ_LODS_NUM; l++)
      if(CStatObj * pLod = (CStatObj *)GetLodObject(l))
        pLod->UpdateStreamingPrioriryInternal(objMatrix, fImportance);
  }

  return true;
}

int CStatObj::FindNearesLoadedLOD(int nLodIn, const SRendParams & rParams)
{
  // make sure requested lod is loaded
/*  if(CStatObj * pObjForStreamIn = nLodIn ? m_arrpLowLODs[nLodIn] : this)
  {
    bool bRenderNodeValid(rParams.pRenderNode && ((int)(void*)(rParams.pRenderNode)>0) && rParams.pRenderNode->m_fWSMaxViewDist);
    float fImportance = bRenderNodeValid ? (1.f - (rParams.fDistance / rParams.pRenderNode->m_fWSMaxViewDist)) : 0.5f;
    pObjForStreamIn->UpdateStreamingPrioriryInternal(fImportance);
  }*/

  // if requested lod is not ready - find nearest ready one
  int nLod = nLodIn;

  if(nLod==0 && !GetRenderMesh())
    nLod++;

  while(nLod && nLod<MAX_STATOBJ_LODS_NUM && (!m_arrpLowLODs[nLod] || !m_arrpLowLODs[nLod]->GetRenderMesh()))
    nLod++;

  if(nLod>=MAX_STATOBJ_LODS_NUM)
    nLod = nLodIn;

  return nLod;
}
