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

#include "CGF/CGFLoader.h"
#include "CGF/CGFSaver.h"
#include "CGF/ReadOnlyChunkFile.h"

#include "CryMemoryManager.h"

#define GEOM_INFO_FILE_EXT "ginfo"
#define MESH_NAME_FOR_MAIN "main"

PodArray<CStatObj*> CStatObj::m_arrStatObjForRenderMeshDelete;

//////////////////////////////////////////////////////////////////////////
/*inline bool IsDigitChar2( char c )
{
	return (isdigit(c)||c=='.'||c=='-'||c=='+');
}

//////////////////////////////////////////////////////////////////////////
inline float ExtractFloatKeyFromString( const char *key, const char *props )
{
	const char *ptr;
	char *ptr1,buf[64];
	if (ptr=strstr(props,key)) 
	{
		for(ptr+=4;*ptr && !IsDigitChar2(*ptr);ptr++);
		for(ptr1=buf; *ptr && ptr1-buf<sizeof(buf)-1 && (IsDigitChar2(*ptr)||*ptr=='E'||*ptr=='e'); *ptr++,*ptr1++) *ptr1=*ptr;
		*ptr1 = 0;
		return (float)atof(buf);
	}
	return -1;
}*/

void CStatObj::Refresh(int nFlags)
{
	if(nFlags & FRO_GEOMETRY)
	{
//		bool bSpritesWasCreated = IsSpritesCreated();
		ShutDown();
		Init();
		bool bRes = LoadCGF( m_szFileName );

		LoadLowLODs(false,0);

		if(!bRes)
		{ // load default in case of error
			ShutDown();
			Init();
			LoadCGF("objects/default.cgf");
			m_bDefaultObject = true;
		}

		return;
	}

	if (nFlags & (FRO_TEXTURES | FRO_SHADERS))
	{
		//if (m_pMaterial)
			//m_pMaterial->Reload();

		/*
		IRenderMesh *pRM = m_pRenderMesh;

		for (int i=0; i<pRM->GetChunks()->Count(); i++)
		{
			IShader *e = (*pRM->GetChunks())[i].GetDefaultMaterial()->GetShaderItem().m_pShader;
			if (e && (*pRM->GetChunks())[i].pRE && (*pRM->GetChunks())[i].nNumIndices)
				e->Reload(nFlags);
		}
		*/
	}
}

void CStatObj::LoadLowLODs(bool bUseStreaming,unsigned long nLoadingFlags=0)
{
	if (m_nLoadedLodsNum > 1)
		return;

	m_nLoadedLodsNum = 1;

	if(!GetCVars()->e_Lods)
		return;

	if (m_bSubObject) // Never do this for sub objects.
		return;

	int nLoadedLods = 1;
	CStatObj* loadedLods[MAX_STATOBJ_LODS_NUM];
	for(int nLodLevel=0; nLodLevel<MAX_STATOBJ_LODS_NUM; nLodLevel++)
		loadedLods[nLodLevel] = 0;

	const char *sFileExt = PathUtil::GetExt(m_szFileName);

	for(int nLodLevel=1; nLodLevel<MAX_STATOBJ_LODS_NUM; nLodLevel++)
	{
		// make lod file name
		char sLodFileName[512];
		char sLodNum[8];
		strcpy_s(sLodFileName,m_szFileName);
		char *sPointSeparator = strchr(sLodFileName,'.');
		if (sPointSeparator)
			*sPointSeparator = '\0'; // Terminate at the dot
	  strcat_s(sLodFileName,"_lod");
		ltoa(nLodLevel,sLodNum,10);
		strcat_s(sLodFileName,sLodNum);
		strcat_s(sLodFileName, "." );
		strcat_s(sLodFileName, sFileExt );

		CStatObj *pLodStatObj = m_arrpLowLODs[nLodLevel];

		// try to load
		bool bRes = false;

    {
      if(pLodStatObj = stl::find_in_map( m_pObjManager->m_nameToObjectMap,CONST_TEMP_STRING(sLodFileName),NULL ))
      {
        pLodStatObj->m_bForInternalUse = true;
        pLodStatObj->m_pLod0 = this; // Must be here.
        bRes = true;

        typedef std::set<CStatObj*> LoadedObjects;
        LoadedObjects::iterator it = m_pObjManager->m_lstLoadedObjects.find(pLodStatObj);
        if (it != m_pObjManager->m_lstLoadedObjects.end())
        {
          m_pObjManager->m_lstLoadedObjects.erase(it);
          m_pObjManager->m_nameToObjectMap.erase( CONST_TEMP_STRING(sLodFileName) );
        }
      }
      else if(IsValidFile(sLodFileName))
      {
        if (!pLodStatObj)
        {
          pLodStatObj = new CStatObj();
          pLodStatObj->m_bForInternalUse = true;
          pLodStatObj->m_pLod0 = this; // Must be here.
        }

        if(bUseStreaming && GetCVars()->e_StreamCgf)
          pLodStatObj->m_bUseStreaming = true;

        bRes = pLodStatObj->LoadCGF(sLodFileName,true,nLoadingFlags);
      }
		}

		if(!bRes)
		{
			if (m_arrpLowLODs[nLodLevel] != pLodStatObj)
			{
				SAFE_RELEASE(pLodStatObj);
			}
			SetLodObject( nLodLevel,0 );
			break;
		}
		else
			nLoadedLods++;

		loadedLods[nLodLevel] = pLodStatObj;

		bool bLodCompound = (pLodStatObj->GetFlags()&STATIC_OBJECT_COMPOUND) != 0;
		bool bLod0Compund = (GetFlags()&STATIC_OBJECT_COMPOUND) != 0;

		if (!bLodCompound)
		{
			SetLodObject( nLodLevel,pLodStatObj );
		}
		if (bLodCompound != bLod0Compund)
		{
			// LOD0 and LOD differ.
			FileWarning( 0,sLodFileName,"Invalid LOD%d, LOD%d have different merging property from LOD0",nLodLevel,nLodLevel );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Put LODs into the sub objects.
	//////////////////////////////////////////////////////////////////////////
	if (nLoadedLods > 1)
	{
    m_bLodsAreLoadedFromSeparateFile = true;

		for (int i = 0; i < m_subObjects.size(); i++)
		{
			SSubObject *pSubObject = &m_subObjects[i];
			if (!pSubObject->pStatObj || pSubObject->nType != STATIC_SUB_OBJECT_MESH)
				continue;

			CStatObj *pSubStatObj = (CStatObj*)pSubObject->pStatObj;

//			int nLoadedTrisCount = ((CStatObj*)pSubObject->pStatObj)->m_nLoadedTrisCount;

			for (int nLodLevel = 1; nLodLevel < nLoadedLods; nLodLevel++)
			{
				if (loadedLods[nLodLevel] != 0 && loadedLods[nLodLevel]->m_nSubObjectMeshCount > 0)
				{
					SSubObject *pLodSubObject = loadedLods[nLodLevel]->FindSubObject(pSubObject->name);
					if (pLodSubObject && pLodSubObject->pStatObj && pLodSubObject->nType == STATIC_SUB_OBJECT_MESH)
					{
						pSubStatObj->SetLodObject( nLodLevel,(CStatObj*)pLodSubObject->pStatObj );
					}
				}
			}
			if (pSubStatObj)
				pSubStatObj->CleanUnusedLods();
		}
	}

	CleanUnusedLods();

  for(int nLodLevel=0; nLodLevel<MAX_STATOBJ_LODS_NUM; nLodLevel++)
  {
    if (loadedLods[nLodLevel])
      GetObjManager()->CheckForGarbage(loadedLods[nLodLevel], true);
  }
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::CleanUnusedLods()
{
	//////////////////////////////////////////////////////////////////////////
	// Free render resources for unused upper LODs.
	//////////////////////////////////////////////////////////////////////////
	if (m_nLoadedLodsNum > 1)
	{
		int nMinLod = GetMinUsableLod();
		nMinLod = clamp_tpl(nMinLod,0,(int)m_nLoadedLodsNum-1);
		for (int i = 0; i < nMinLod; i++)
		{
			CStatObj *pStatObj = (CStatObj*)GetLodObject(i);
			if (!pStatObj)
				continue;

			if (pStatObj->m_pRenderMesh)
			{
				pStatObj->SetRenderMesh(0);
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
}

void TransformMesh( CMesh &mesh,Matrix34 tm )
{
	if (!mesh.m_pPositions)
		return;

	int nVerts = mesh.GetVertexCount();
	for (int i = 0; i < nVerts; i++)
	{
		mesh.m_pPositions[i] = tm.TransformPoint(mesh.m_pPositions[i]);
	}
}

#if INCLUDE_MEMSTAT_CONTEXTS
static string FindCGFSourceFilename(const char* filename)
{
  CChunkFile infoChunkFile;
	if (!infoChunkFile.Read(filename))
		return string();

	for (int ci = 0, cc = infoChunkFile.NumChunks(); ci != cc; ++ ci)
	{
		if (infoChunkFile.GetChunkHeader(ci).ChunkType == ChunkType_SourceInfo)
		{
			IChunkFile::ChunkDesc* chunkDesc = infoChunkFile.GetChunk(ci);

			return ((const char*) (chunkDesc->data)) + sizeof(CHUNK_HEADER);
		}
	}

	return string();
}
#endif //INCLUDE_MEMSTAT_CONTEXTS

//////////////////////////////////////////////////////////////////////////
bool CStatObj::LoadCGF( const char *filename,bool bLod,unsigned long nLoadingFlags )
{
  FUNCTION_PROFILER_3DENGINE;

	if (m_bSubObject) // Never execute this on the sub objects.
		return true;

#if INCLUDE_MEMSTAT_CONTEXTS
	string sourceFilename;

	{
		CryReplayInfo info;
		CryGetIMemReplay()->ReplayGetInfo(info);
		if (info.filename)
			sourceFilename = FindCGFSourceFilename(filename);
	}

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_MAX, 0, "%s", sourceFilename.c_str());
	MEMSTAT_CONTEXT_NAMED_FMT(CGF, EMemStatContextTypes::MSC_CGF, 0, "%s", filename);
#endif

	PrintComment("Loading %s", filename);
	if (!bLod)
		GetConsole()->TickProgressBar();

	m_nLoadedTrisCount = 0;
	m_nLoadedVertexCount = 0;
	m_szFileName = filename;
	m_szFileName.replace( '\\','/' );

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

	Listener listener;
	CLoaderCGF cgfLoader;
	CReadOnlyChunkFile chunkFile( false,bLod ); // Chunk file must exist until CGF is completely loaded, and if loading from file do not make a copy of it.

	if(m_nDataSize)
	{
		if (chunkFile.ReadFromMemBlock(m_pData,m_nDataSize))
			pCGF = cgfLoader.LoadCGF( filename,chunkFile, &listener, nLoadingFlags );
	}
	else
		pCGF = cgfLoader.LoadCGF( filename,chunkFile, &listener, nLoadingFlags );
	if (!pCGF)
	{
		FileWarning( 0,filename,"CGF Loading Failed: %s",cgfLoader.GetLastError() );
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
	if (nLoadingFlags & ELoadingFlagsForceBreakable)
		m_nFlags |= STATIC_OBJECT_DYNAMIC;

	//////////////////////////////////////////////////////////////////////////
	// Find out number of meshes, and get pointer to the first found mesh.
	//////////////////////////////////////////////////////////////////////////
	for (int i = 0; i < pCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pNode = pCGF->GetNode(i);
		if (pNode->type == CNodeCGF::NODE_MESH)
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

		if (pCGF->GetCommonMaterial())
		{
			if (nLoadingFlags&ELoadingFlagsPreviewMode)
			{
				m_pMaterial = GetMatMan()->GetDefaultMaterial();
				m_pMaterial->AddRef();
			}
			else 
			{
				m_pMaterial = GetMatMan()->LoadCGFMaterial( pCGF->GetCommonMaterial(),m_szFileName,nLoadingFlags );
			}
		}
	}

  { 
    // Fail if mesh was not complied by RC
    CMesh *pTestMesh = NULL;
    if (!pTestMesh && pFirstMeshNode)
      pTestMesh = pFirstMeshNode->pMesh;

    if (pTestMesh && pTestMesh->GetFacesCount() > 0)
    {
      FileWarning( 0,filename,"CGF is not compiled" );
      delete pCGF;
      return false;
    }

    if(GetCVars()->e_StatObjValidate)
    {
      const char* pErrorDescription = 0;
/* //Diesel cut
      if (pTestMesh && (!pTestMesh->Validate(&pErrorDescription)))
      {
	      FileWarning( 0,filename,"CGF has invalid merged mesh (%s)", pErrorDescription );
	      assert(!"CGF has invalid merged mesh");
	      delete pCGF;
	      return false;
      }

      if (!pCGF->ValidateMeshes(&pErrorDescription))
      {
	      FileWarning( 0,filename,"CGF has invalid meshes (%s)", pErrorDescription );
	      assert(!"CGF has invalid meshes");
	      delete pCGF;
	      return false;
      }
*/ //Diesel cut
    }
  }

  // Common of all sub nodes bbox.
  AABB commonBBox;
  commonBBox.Reset();

	bool bHaveMeshNamedMain = false;

	//////////////////////////////////////////////////////////////////////////
	// Create StatObj from Mesh.
	//////////////////////////////////////////////////////////////////////////
	if (pExportInfo->bMergeAllNodes || m_nSubObjectMeshCount == 0)
	{
		CMesh *pMesh = NULL;
		if (!pMesh && pFirstMeshNode)
			pMesh = pFirstMeshNode->pMesh;

		if (pFirstMeshNode)
		{
			m_vBoxMin = pFirstMeshNode->meshInfo.bboxMin;
			m_vBoxMax = pFirstMeshNode->meshInfo.bboxMax;
			m_nLoadedTrisCount = pFirstMeshNode->meshInfo.nIndices / 3;
			m_nLoadedVertexCount = pFirstMeshNode->meshInfo.nVerts;
		}

		if (pMesh)
		{
			// Assign mesh to this static object.
			if (!SetFromMesh( &m_pRenderMesh, pMesh, !m_bUseStreaming ))
			{
				delete pCGF;
				return false;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Physicalize merged geometry.
		//////////////////////////////////////////////////////////////////////////
		if (pFirstMeshNode)
		{
			PhysicalizeCompiled( pFirstMeshNode );
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

		std::vector< std::pair<CNodeCGF*,CStatObj*> > meshToObject;
		meshToObject.reserve( pCGF->GetNodeCount() );

		int nNumMeshes = 0;
		for (int ii = 0; ii < pCGF->GetNodeCount(); ii++)
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
      subObject.helperSize.Set(0,0,0);

			if (pNode->type == CNodeCGF::NODE_MESH)
			{
				if (pExportInfo->bMergeAllNodes || m_nSubObjectMeshCount == 0) // Only add helpers, ignore meshes.
					continue;

				nNumMeshes++;
				subObject.nType = STATIC_SUB_OBJECT_MESH;

				if (stricmp(pNode->name,MESH_NAME_FOR_MAIN) == 0)
					bHaveMeshNamedMain = true;
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
								for (int i = 0,num = nodes.size(); i < num; i++)
									if (nodes[i] == pNode->pParent)
									{
										pStatObjOwner = (CStatObj*)m_subObjects[i].pStatObj;
										break;
									}
							}
							if (!pStatObjOwner)
								continue;

							if (pStatObjOwner->m_pRenderMeshOcclusion)
							{
								pStatObjOwner->m_pRenderMeshOcclusion = 0;
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
				if (pNode->pSharedMesh)
				{
					// Try to find already create StatObj for a shred mesh node
					for (int k = 0,num = (int)meshToObject.size(); k < num; k++)
					{
						if (pNode->pSharedMesh == meshToObject[k].first)
						{
							subObject.pStatObj = meshToObject[k].second;
							break;
						}
					}
				}
				if (!subObject.pStatObj)
				{
					// Create a StatObj from the CGF node.
					subObject.pStatObj = MakeStatObjFromCgfNode(pCGF,pNode,nLoadingFlags,commonBBox);
					if (pNode->pSharedMesh)
						meshToObject.push_back( std::make_pair(pNode->pSharedMesh,static_cast<CStatObj*>(subObject.pStatObj)) );
					else
						meshToObject.push_back( std::make_pair(pNode,static_cast<CStatObj*>(subObject.pStatObj)) );
				}
			}

			//////////////////////////////////////////////////////////////////////////
			// Check if helper object is a LOD, name must be $lod1, $lod2 etc...
			//////////////////////////////////////////////////////////////////////////
			if (subObject.nType == STATIC_SUB_OBJECT_HELPER_MESH && (stristr(pNode->name,"$lod") != 0))
			{
				// Check if helper object is a LOD, name must be $lod1, $lod2 etc...
				if (!subObject.pStatObj)
					continue;

				CStatObj *pLodStatObj = (CStatObj*)subObject.pStatObj;
				CStatObj *pStatObjParent = this;
				if (!pExportInfo->bMergeAllNodes && m_nSubObjectMeshCount > 0 && pNode->pParent)
				{
					// We are attached to some object, find it.
					for (int i = 0,num = nodes.size(); i < num; i++)
					{
						if (nodes[i] == pNode->pParent)
						{
							pStatObjParent = (CStatObj*)m_subObjects[i].pStatObj;
							break;
						}
					}
				}
				if (!pStatObjParent)
				{
					continue;
				}

				int nLodLevel = clamp_tpl( atoi(pNode->name+4),1,MAX_STATOBJ_LODS_NUM-1 );
				if (!pStatObjParent->m_arrpLowLODs[nLodLevel])
				{
					pStatObjParent->SetLodObject( nLodLevel,pLodStatObj );
					continue;
				}
				else
				{
					FileWarning(0, m_szFileName.c_str(),	"Duplicate LOD helper %s (%s)",pNode->name,m_szFileName.c_str() );
					continue;
				}
			}
			//////////////////////////////////////////////////////////////////////////

			if (subObject.pStatObj)
			{
				subObject.pStatObj->AddRef();
			}

			m_subObjects.push_back(subObject);
			nodes.push_back(pNode);
			
			if (pNode->bHasFaceMap)
				m_bHasDeformationMorphs = true;
		}

		// Delete not assigned stat objects.
		for (int k = 0,num = (int)meshToObject.size(); k < num; k++)
		{
			if (meshToObject[k].second->m_nUsers == 0)
			{
				delete meshToObject[k].second;
			}
		}

		// Assign SubObject parent pointers.
		int nNumCgfNodes = (int)nodes.size();
		if (nNumCgfNodes > 0)
		{
			CNodeCGF **pNodes = &nodes[0];

			//////////////////////////////////////////////////////////////////////////
			// Move meshes to begining, Sort sub-objects so that meshes are first.
			for (int i = 0; i < nNumCgfNodes; i++)
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

			// Assign Parent nodes.
			for (int i = 0; i < nNumCgfNodes; i++)
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
		}
	}
	
	if (m_nSubObjectMeshCount > 0)
	{
		m_vBoxMin = commonBBox.min;
		m_vBoxMax = commonBBox.max;
		CalcRadiuses();
	}

	//////////////////////////////////////////////////////////////////////////
	// Physicalize physics proxy nodes.
	//////////////////////////////////////////////////////////////////////////
	for (int i = 0,numNodes = pCGF->GetNodeCount(); i < numNodes; i++)
	{
		CNodeCGF *pNode = pCGF->GetNode(i);
		if (pNode->bPhysicsProxy)
		{
			CStatObj *pStatObjParent = this;
			if (pNode->pParent)	for (int j=nodes.size()-1; j>=0; j--) 
				if (nodes[j]==pNode->pParent && m_subObjects[j].pStatObj)
				{
					pStatObjParent = (CStatObj*)m_subObjects[j].pStatObj;
					break;
				}
			pStatObjParent->PhysicalizeCompiled(pNode, 1);
		}
	}

	
	//////////////////////////////////////////////////////////////////////////
	// Analyze foliage info.
	//////////////////////////////////////////////////////////////////////////
	if (pExportInfo->bMergeAllNodes || m_nSubObjectMeshCount == 0)
	{
		AnalizeFoliage(m_pRenderMesh, pCGF);
	}
	//////////////////////////////////////////////////////////////////////////

  for (int i = 0; i < pCGF->GetNodeCount(); i++) if (strstr(pCGF->GetNode(i)->properties,"deformable"))
	  m_nFlags |= STATIC_OBJECT_DEFORMABLE;

	if (m_nSubObjectMeshCount > 0)
		m_nFlags |= STATIC_OBJECT_COMPOUND;

/*	
  if (!bLod)
	{
		// Try load info file.
		string szInfoFile = PathUtil::ReplaceExtension(m_szFileName,GEOM_INFO_FILE_EXT);
		LoadCGF_Info(szInfoFile);
	}
*/	
	if (!m_szProperties.empty())
		ParseProperties();

	CPhysicalizeInfoCGF* pPi = pCGF->GetPhysiclizeInfo();
	if (pPi->nRetTets)
		m_pLattice = GetPhysicalWorld()->GetGeomManager()->CreateTetrLattice(pPi->pRetVtx,pPi->nRetVtx, pPi->pRetTets,pPi->nRetTets);

	delete pCGF;

	if (m_bHasDeformationMorphs)
	{
		int i,j;
		for(i=GetSubObjectCount()-1;i>=0;i--) if ((j=SubobjHasDeformMorph(i))>=0)
			GetSubObject(i)->pStatObj->SetDeformationMorphTarget(GetSubObject(j)->pStatObj);
		m_bUnmergable = 1;
	}

  if(!m_bUseStreaming)
    m_eStreamingStatus = ecss_Ready;

	return true;
}

//////////////////////////////////////////////////////////////////////////
CStatObj* CStatObj::MakeStatObjFromCgfNode( CContentCGF *pCGF,CNodeCGF *pNode,int nLoadingFlags,AABB &commonBBox )
{
	CNodeCGF *pTMNode = pNode;
	if (pNode->pSharedMesh)
	{
		pNode = pNode->pSharedMesh;
	}

	// Calc bbox.
	if (pNode->pMesh && pNode->type == CNodeCGF::NODE_MESH)
	{
		AABB box = pNode->pMesh->m_bbox;
		box.SetTransformedAABB( pTMNode->worldTM,box );
		commonBBox.Add( box.min );
		commonBBox.Add( box.max );
	}

	CStatObj *pStatObj = new CStatObj;

	pStatObj->m_szFileName = m_szFileName;
	pStatObj->m_szGeomName = pNode->name;
	pStatObj->m_bSubObject = true;

	if (pNode->type == CNodeCGF::NODE_MESH)
	{
		pStatObj->m_pParentObject = this;
	}

	pStatObj->m_szProperties = pNode->properties;
	pStatObj->m_szProperties.MakeLower();
	if (!pStatObj->m_szProperties.empty())
		pStatObj->ParseProperties();

	if (pNode->pMaterial)
	{
		if (nLoadingFlags&ELoadingFlagsPreviewMode)
		{
			pStatObj->m_pMaterial = GetMatMan()->GetDefaultMaterial();
			pStatObj->m_pMaterial->AddRef();
		}
		else 
		{
			pStatObj->m_pMaterial = GetMatMan()->LoadCGFMaterial( pNode->pMaterial,m_szFileName,nLoadingFlags );
		}
		if (!m_pMaterial || m_pMaterial->IsDefault())
			m_pMaterial = pStatObj->m_pMaterial; // take it as a general stat obj material.
	}
	if (!pStatObj->m_pMaterial)
		pStatObj->m_pMaterial = m_pMaterial;

	pStatObj->m_vBoxMin = pNode->meshInfo.bboxMin;
	pStatObj->m_vBoxMax = pNode->meshInfo.bboxMax;
	pStatObj->m_nLoadedTrisCount = pNode->meshInfo.nIndices / 3;
	pStatObj->m_nLoadedVertexCount = pNode->meshInfo.nVerts;
	if (nLoadingFlags & ELoadingFlagsForceBreakable)
		pStatObj->m_nFlags |= STATIC_OBJECT_DYNAMIC;

	if (pNode->pMesh)
	{
		pStatObj->SetFromMesh( &pStatObj->m_pRenderMesh, pNode->pMesh, !m_bUseStreaming );
	}
	pStatObj->m_nFileNodeId = pNode->nChunkId;

	strncpy(pStatObj->m_FileNodeName, pNode->name, sizeof(pStatObj->m_FileNodeName));
	if (pNode->bHasFaceMap && pNode->pMesh)
	{
		memcpy(pStatObj->m_pMapFaceToFace0=new uint16[pNode->pMesh->GetIndexCount()/3], &pNode->mapFaceToFace0[0], 	(pNode->pMesh->GetIndexCount()/3)*sizeof(uint16));
	}
	pStatObj->PhysicalizeCompiled( pNode );
	pStatObj->AnalizeFoliage( pStatObj->m_pRenderMesh,pCGF );
	pStatObj->m_pSkinInfo = (SSkinVtx*)pNode->pSkinInfo;
	pNode->pSkinInfo=0;

	return pStatObj;
}

//////////////////////////////////////////////////////////////////////////
bool CStatObj::SetFromMesh( IRenderMesh** ppOutputMesh, CMesh *pMesh, bool bDoRenderMesh )
{
  FUNCTION_PROFILER_3DENGINE;

	// If mesh contain faces, it must be compiled.
	if (pMesh->GetFacesCount() > 0)
	{
    Warning( "CStatObj::SetFromMesh: Stripifying geometry at loading time %s (Use resource compiler)",m_szFileName.c_str() );

		mesh_compiler::CMeshCompiler meshCompiler;
		meshCompiler.Compile( *pMesh );
	}

/*
	if(pMesh->m_bbox.GetRadius()>1.f) // tessellate
	{
		mesh_compiler::CMeshCompiler meshCompiler;
		meshCompiler.Tesselate(*pMesh, 0.25f);
	}
*/
	m_vBoxMin = pMesh->m_bbox.min;
	m_vBoxMax = pMesh->m_bbox.max;

	CalcRadiuses();

	m_nLoadedTrisCount = pMesh->GetIndexCount() / 3;
	m_nLoadedVertexCount = pMesh->GetVertexCount();
	if (!m_nLoadedTrisCount)
		return 0;
	
	m_nRenderTrisCount = 0;
	m_nRenderMatIds = 0;
	//////////////////////////////////////////////////////////////////////////
	// Initialize Mesh subset material flags.
	//////////////////////////////////////////////////////////////////////////
	for (int i = 0; i < pMesh->GetSubSetCount(); i++)
	{
		SMeshSubset &subset = pMesh->m_subsets[i];
		IMaterial *pMtl = m_pMaterial->GetSafeSubMtl(subset.nMatID);
		subset.nMatFlags = pMtl->GetFlags();
		if (subset.nPhysicalizeType==PHYS_GEOM_TYPE_NONE)
			subset.nMatFlags |= MTL_FLAG_NOPHYSICALIZE;
		if (!(subset.nMatFlags&MTL_FLAG_NODRAW) && (subset.nNumIndices > 0))
		{
			m_nRenderMatIds++;
			m_nRenderTrisCount += subset.nNumIndices/3;
		}
	}
	//////////////////////////////////////////////////////////////////////////

	if (!m_nRenderTrisCount)
  {
    //Warning( "CStatObj::SetFromMesh: 0 visible triangles found in file %s - skipped", m_szFileName.c_str() );
		return 0;
  }

	// Create renderable mesh.
	if (!GetSystem()->IsDedicated())
	{
		if (*ppOutputMesh)
		{
			GetRenderer()->DeleteRenderMesh(*ppOutputMesh);
			assert(!m_bUseStreaming);
      (*ppOutputMesh) = NULL;
		}
		
		if (!pMesh)
			return false;
		if (pMesh->GetSubSetCount() == 0)
			return true;

    if(bDoRenderMesh)
    {
      (*ppOutputMesh) = GetRenderer()->CreateRenderMesh("StatObj", m_szFileName.c_str());
			if(m_idmatBreakable >= 0 || m_bBreakableByGame)
			{
				// need to keep mesh data in system memory for breakable meshes
				(*ppOutputMesh)->KeepSysMesh(true);
			}
			
      (*ppOutputMesh)->SetMaterial( m_pMaterial );
			// we cannot use FSM_CREATE_DEVICE_MESH flag since we can have an async call to the renderer!
      (*ppOutputMesh)->SetMesh( *pMesh, 0, GetCVars()->e_StreamCgf?0:FSM_CREATE_DEVICE_MESH );
/*
      ICrySizer * pSizer = GetSystem()->CreateSizer();
      (*pOutputMesh)->GetMemoryUsage(pSizer, IRenderMesh::MEM_USAGE_COMBINED);
      m_nRenderMeshMemoryUsage = pSizer->GetTotalSize();
      pSizer->Release();

      if(!bDoRenderMesh)
        m_arrStatObjForRenderMeshDelete.Add(this);
*/
    }

		m_nRenderMeshMemoryUsage = pMesh->EstimateRenderMeshMemoryUsage();
  }

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Save statobj to the CGF file.
bool CStatObj::SaveToCGF( const char *sFilename,IChunkFile** pOutChunkFile )
{
	CContentCGF *pCGF = new CContentCGF(sFilename);

	pCGF->GetExportInfo()->bCompiledCGF = true;
	pCGF->GetExportInfo()->bMergeAllNodes = true;
	strcpy_s( pCGF->GetExportInfo()->rc_version_string,"From Sandbox" );

	CChunkFile *pChunkFile = new CChunkFile();
	if (pOutChunkFile)
		*pOutChunkFile = pChunkFile;

	CMaterialCGF *pMaterialCGF = new CMaterialCGF;
	strcpy_s( pMaterialCGF->name,m_pMaterial->GetName() );
	pMaterialCGF->nPhysicalizeType = PHYS_GEOM_TYPE_DEFAULT;
	pMaterialCGF->bOldMaterial = false;
	pMaterialCGF->pMatEntity = NULL;
	pMaterialCGF->nChunkId = 0;

	// Array of sub materials.
	//std::vector<CMaterialCGF*> subMaterials;

	// Add single node for merged mesh.
	CNodeCGF *pNode = new CNodeCGF;
	pNode->type = CNodeCGF::NODE_MESH;
	strcpy_s( pNode->name,"Merged" );
	pNode->localTM.SetIdentity();
	pNode->worldTM.SetIdentity();
	pNode->pos.Set(0,0,0);
	pNode->rot.SetIdentity();
	pNode->scl.Set(1,1,1);
	pNode->bIdentityMatrix = true;
	pNode->pMesh = new CMesh;
	pNode->pMesh->Copy( *GetIndexedMesh(true)->GetMesh() );
	pNode->pParent = 0;
	pNode->pMaterial = pMaterialCGF;
	pNode->nPhysicalizeFlags = 0;
	SavePhysicalizeData( pNode );
	// Add node to CGF contents.
	pCGF->AddNode( pNode );

	CSaverCGF cgfSaver( sFilename,*pChunkFile );
	cgfSaver.SaveContent( pCGF );

	bool bResult = true;
	if (!pOutChunkFile)
	{
		bResult = pChunkFile->Write( sFilename );
		pChunkFile->Release();
	}

	delete pCGF;

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
/*bool CStatObj::LoadCGF_Info( const char *filename )
{
	CCryFile file;
	if (file.Open(filename,"rb",ICryPak::FOPEN_HINT_QUIET))
	{
		int nLen = file.GetLength();

		m_szProperties.resize(nLen);
		char *sAllText = (char*)&m_szProperties[0];
		file.ReadRaw( sAllText,nLen );

		// Convert properties to low case.
		m_szProperties.MakeLower();

		//m_szProperties.reserve( 2+nLen );
		//m_szProperties += '\n';
		//m_szProperties += sAllText;

		return true;
	}
	return false;
}*/

//////////////////////////////////////////////////////////////////////////
inline char* trim_whitespaces( char *str,char *strEnd )
{
	char *first = str;
	while (first < strEnd && (*first == ' '||*first == '\t'))
		first++;
	char *s = strEnd-1;
	while (s >= first && (*s == ' '||*s == '\t'))
		*s-- = 0;
	return first;
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::ParseProperties()
{
  FUNCTION_PROFILER_3DENGINE;

	//m_phys_mass = ExtractFloatKeyFromString("mass",sProperties);
	//m_phys_density = ExtractFloatKeyFromString("density",sProperties);

//	int nLen = *sP
	//static DynArray<char> propertiesString;
	//propertiesString.resize(sP)

	int nLen = m_szProperties.size();
	if (nLen >= 4090)
	{
		Warning("CGF '%s' have longer then 4K geometry info file", m_szFileName.c_str());
		nLen = 4090;
	}

	char properties[4096];
	memcpy( properties,m_szProperties.c_str(),nLen );
	properties[nLen] = 0;

	char *str = properties;
	char *strEnd = str + nLen;
	while (str < strEnd)
	{
		char *line = str;
		while (str<strEnd && *str != '\n' && *str != '\r')
			str++;
		char *lineEnd = str;
		*lineEnd = 0;
		str++;
		while (str<strEnd && (*str == '\n' || *str == '\r')) // Skip all \r\n at end.
			str++;

		if (*line == '/' || *line == '#') // skip comments
		{
			continue;
		}

		if (line < lineEnd)
		{
			// Parse line.
			char *l = line;
			while (l < lineEnd && *l != '=')
				l++;
			if (l < lineEnd)
			{
				*l = 0;
				char *left = line;
				char *right = l+1;

				// remove white spaces from left and right.
				left = trim_whitespaces(left,l);
				right = trim_whitespaces(right,lineEnd);

				//////////////////////////////////////////////////////////////////////////
				if (0 == strcmp(left,"mass"))
				{
					m_phys_mass = (float)atof(right);
				}
				else if (0 == strcmp(left,"density"))
				{
					m_phys_density = (float)atof(right);
				}
				//////////////////////////////////////////////////////////////////////////

			}
			else
			{
				// There`s no = on the line, must be a flag.
				//////////////////////////////////////////////////////////////////////////
				if (0 == strcmp(line,"entity"))
				{
					// pickable
					m_nFlags |= STATIC_OBJECT_SPAWN_ENTITY;
				}
				else if (0 == strcmp(line,"pickable"))
				{
					m_nFlags |= STATIC_OBJECT_PICKABLE;
				}
        else if (0 == strcmp(line,"no_auto_hidepoints"))
        {
          m_nFlags |= STATIC_OBJECT_NO_AUTO_HIDEPOINTS;
        }
				else if (0 == strcmp(line,"dynamic"))
        {
          m_nFlags |= STATIC_OBJECT_DYNAMIC;
        }
				else if (0 == strcmp(line,"no_hit_refinement"))
        {
          m_bNoHitRefinement = true;
					for(int i=m_arrPhysGeomInfo.GetGeomCount()-1; i>=0; i--)
						m_arrPhysGeomInfo[i]->pGeom->SetForeignData(0,0);
        } else if (0 == strcmp(line,"no_explosion_occlusion"))
        {
          m_bDontOccludeExplosions = true;
				}
				//////////////////////////////////////////////////////////////////////////
			}
		}
	}
}
