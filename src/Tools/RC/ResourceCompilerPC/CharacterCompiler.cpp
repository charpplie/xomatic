////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   StatCGFCompiler.cpp
//  Version:     v1.00
//  Created:     5/11/2002 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Cry_Color.h"
#include "ConvertContext.h"
#include "iconfig.h"
#include "CharacterCompiler.h"
#include "ChunkFileHelpers.h"
#include "CGF\CGFLoader.h"
#include "CGF\CGFSaver.h"
#include "StatCGFPhysicalize.h"
#include "CryVersion.h"
#include "ICryXML.h"
#include "IXMLSerializer.h"

//////////////////////////////////////////////////////////////////////////
CharacterCompiler::CharacterCompiler(ICryXML * pXml)
{
	m_pXML = pXml;
	m_refCount = 1;
	m_pPhysicsInterface = NULL;
}

//////////////////////////////////////////////////////////////////////////
CharacterCompiler::~CharacterCompiler()
{
	if (m_pPhysicsInterface)
		delete m_pPhysicsInterface;
}

////////////////////////////////////////////////////////////
void CharacterCompiler::ConstructAndSetOutputFile( ConvertContext &cc )
{
	cc.SetOutputFile(cc.sourceFileFinal);
}

////////////////////////////////////////////////////////////
void CharacterCompiler::GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const
{
	string const filename(cc.getOutputPath());

	if (filenameBuffer && (filename.length() < bufferSize))
	{
		strcpy(filenameBuffer, filename.c_str());		
	}
}

//////////////////////////////////////////////////////////////////////////
void CharacterCompiler::Release()
{
	if (--m_refCount <= 0)
		delete this;
}

//////////////////////////////////////////////////////////////////////////
ICompiler* CharacterCompiler::CreateCompiler()
{
	// Only ever return one compiler, since we don't support multithreading. Since
	// the compiler is just this object, we can tell whether we have already returned
	// a compiler by checking the ref count.
	if (m_refCount >= 2)
		return 0;

	// Until we support multithreading for this convertor, the compiler and the
	// convertor may as well just be the same object.
	++m_refCount;
	return this;
}

//////////////////////////////////////////////////////////////////////////
bool CharacterCompiler::SupportsMultithreading() const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
int CharacterCompiler::GetNumPlatforms() const
{
	return 3;
}

//////////////////////////////////////////////////////////////////////////
EPlatform CharacterCompiler::GetPlatform( int index ) const
{
	switch (index)
	{
	case 0:	return ePlatform_PC;
	case 1:	return ePlatform_X360;
	case 2:	return ePlatform_PS3;
	//case 3:	return ePlatform_GAMECUBE;
	};
	//assert(0);
	return ePlatform_UNKNOWN;
}

DWORD CharacterCompiler::GetTimestamp() const
{
	return GetTimestampForLoadedLibrary(g_hInst);
}

struct IntMeshCollisionInfo
{
	int m_iBoneId;
	AABB m_aABB;
	std::vector<short int> m_arrIndexes;

	IntMeshCollisionInfo()
	{
		// This didn't help much.
		// The BBs are reset to opposite infinites, 
		// but never clamped/grown by any member points.
		m_aABB.min.zero();
		m_aABB.max.zero();
	}
};

string BuildCCCPath(string str)
{

	str.MakeLower();

	std::replace(str.begin(), str.end(), '/', '\\');
	size_t pos = str.find("\\objects\\");
	string cccpath = "\\objects\\CrysisCharacterConversion.ccc";
	if (pos != -1) {
		cccpath = str.Left(pos) + cccpath;
	}

	return cccpath;
}

//workaround!!
void workaround_GetCGFInfo(CContentCGF* pCGF)
{
	printf("\n\n");

	int numNodes = pCGF->GetNodeCount();

	for (int i=0; i<numNodes; i++)
	{
		CNodeCGF* node = pCGF->GetNode(i);

		string nodeType;
		switch (node->type)
		{
			case 0:
				nodeType = "NODE_MESH";
				break;
			case 1:
				nodeType = "NODE_LIGHT";
				break;
			case 2:
				nodeType = "NODE_HELPER";
				break;
			default:
				nodeType = "!!UNKNOWN_NODE!!";
				break;
		}
		printf("\n  ----------------------\n\n");
		printf("  Node %d = '%s' (%s)\n",i,node->name,nodeType.c_str());
		if (node->pParent)
			printf("  Parent = '%s'\n",node->pParent->name);
		else
			printf("  Parent = NULL\n");

		printf("  pos = (%2.2f, %2.2f, %2.2f)\n",node->pos.x,node->pos.y,node->pos.z);
		printf("  rot = (%2.2f, %2.2f, %2.2f, %2.2f)\n",node->rot.v.x,node->rot.v.y,node->rot.v.z,node->rot.w);
		printf("  scl = (%2.2f, %2.2f, %2.2f)\n",node->scl.x,node->scl.y,node->scl.z);

		if (node->type == 0 && node->pMesh)
		{
			printf("  %d vertices, %d faces\n",node->pMesh->GetVertexCount(),node->pMesh->GetFacesCount());
			
			if (node->pMesh->m_pBoneMapping)
			{
				int minBoneIdx = 65536;
				int maxBoneIdx = 0;
				for (int i=0; i<node->pMesh->GetVertexCount(); i++)
				{
					for (int j=0; j<4; j++)
					{
						if (node->pMesh->m_pBoneMapping[i].boneIDs[j] < minBoneIdx)
							minBoneIdx = node->pMesh->m_pBoneMapping[i].boneIDs[j];
						if (node->pMesh->m_pBoneMapping[i].boneIDs[j] > maxBoneIdx)
							maxBoneIdx = node->pMesh->m_pBoneMapping[i].boneIDs[j];
					}
				}

				printf("  min. bone index = %d\n", minBoneIdx);
				printf("  max. bone index = %d\n", maxBoneIdx);
			}
		}
	}

	printf("\n");
	printf("  m_arrBonesDesc.size() = %d\n",pCGF->GetSkinningInfo()->m_arrBonesDesc.size());
	printf("  ");
	for (int bone=0; bone<pCGF->GetSkinningInfo()->m_arrBonesDesc.size(); bone++)
	{
		printf("%s   ",pCGF->GetSkinningInfo()->m_arrBonesDesc[bone].m_arrBoneName);
	}
	printf("\n");

	printf("\n");
	printf("  m_arrBoneEntities.size() = %d\n",pCGF->GetSkinningInfo()->m_arrBoneEntities.size());
	int minID = 65536;
	int maxID = 0;
	bool differentidx = false;
	for (int bone=0; bone<pCGF->GetSkinningInfo()->m_arrBoneEntities.size(); bone++)
	{
		if (bone != pCGF->GetSkinningInfo()->m_arrBoneEntities[bone].BoneID)
			differentidx = true;

		if (pCGF->GetSkinningInfo()->m_arrBoneEntities[bone].BoneID < minID)
			minID = pCGF->GetSkinningInfo()->m_arrBoneEntities[bone].BoneID;
		if (pCGF->GetSkinningInfo()->m_arrBoneEntities[bone].BoneID > maxID)
			maxID = pCGF->GetSkinningInfo()->m_arrBoneEntities[bone].BoneID;
	}
	printf("  minID = %d, maxID = %d  (%s)\n", minID, maxID, differentidx ? "BoneID's and the array indices are different" : "Each BoneID match the array indices");

	printf("\n");
	printf("  m_arrPhyBoneMeshes.size() = %d\n",pCGF->GetSkinningInfo()->m_arrPhyBoneMeshes.size());
	printf("  m_arrCollisions.size() = %d\n",pCGF->GetSkinningInfo()->m_arrCollisions.size());
	printf("  m_arrIntVertices.size() = %d\n",pCGF->GetSkinningInfo()->m_arrIntVertices.size());
	printf("  m_arrIntFaces.size() = %d\n",pCGF->GetSkinningInfo()->m_arrIntFaces.size());

	printf("\n\n");
	return;
}

template<class T>
void WriteData( std::vector<uint8>& dest, T* data, size_t count, bool bSwapEndian )
{
	size_t total = count * sizeof(T);
	dest.resize(dest.size() + total);
	T* newdata = (T*)(&dest.back() + 1 - total);
	memcpy(newdata, data, total);
	SwapEndian(newdata, count, bSwapEndian);
}

template<class A>
void WriteArray( std::vector<uint8>& dest, A& array, bool bSwapEndian )
{
	WriteData(dest, array.begin(), array.size(), bSwapEndian);
}

//////////////////////////////////////////////////////////////////////////
bool CharacterCompiler::Process( ConvertContext &cc )
{
	string sourceFile = cc.getSourcePath();
	string outputFile = cc.getOutputPath();
	std::replace(sourceFile.begin(), sourceFile.end(), '/', '\\');
	std::replace(outputFile.begin(), outputFile.end(), '/', '\\');

	bool ok = false;

	try
	{
		if (cc.config->HasKey("RealignChunks"))
		{
			ok = ChunkFileHelpers::RealignChunks(sourceFile, outputFile);
			return ok;
		}

		CChunkFile chunkFile;
		
		CLoaderCGF cgfLoader;
		class Listener : public ILoaderCGFListener
		{
		public:
			Listener(ConvertContext& cc): cc(cc) {}
			virtual void Warning( const char *format ) {RCLogWarning("%s", format);}
			virtual void Error( const char *format ) {RCLogError("%s", format);}

		private:
			ConvertContext& cc;
		};
		Listener listener(cc);
		CContentCGF *pCGF = cgfLoader.LoadCGF( sourceFile,chunkFile,&listener );
		if (!pCGF)
		{
			RCLogError( "Failed to load geometry file %s - %s",sourceFile.c_str(),cgfLoader.GetLastError() );
			return false;
		}

		{
			bool validate = false;
			if (cc.config->HasKey("debugvalidate"))
			{
				if (!cc.config->Get("debugvalidate",validate))
				{
					// No any value specified. Assume 'true'.
					validate = true;
				}
			}

			if (validate)
			{
				bool debugValidateCGF(CContentCGF *pCGF, const char* a_filename);
				debugValidateCGF(pCGF, sourceFile.c_str());
				return true;
			}
		}
		
		const bool bNeedEndianSwap = (cc.platform == ePlatform_X360 || cc.platform == ePlatform_X360);

		bool bUseQuaternions = false;
		if (!cc.config->Get("qtangents",bUseQuaternions))
		{
			bUseQuaternions = false;
		}

		const bool bRotate = true;

		// Delete Node and Mesh chunks from CGF chunk file.
		DeleteOldChunks( pCGF,chunkFile );
		CSkinningInfo* pSkinningInfo = pCGF->GetSkinningInfo();

		string name(sourceFile);
		name.MakeLower();


		string name_woLOD(name);
		int pos = name_woLOD.find("_lod");
		if (pos != string::npos) {
			name_woLOD = name_woLOD.erase(pos, name_woLOD.length() - pos) + ".chr";
		}

		if (bRotate && pSkinningInfo->m_arrBonesDesc.size())
		{
			const char* RootName =  pSkinningInfo->m_arrBonesDesc.size() ? pSkinningInfo->m_arrBonesDesc[0].m_arrBoneName : "NOTHING";
			uint32 IsBiped=0;
			if (RootName[0]=='B' && RootName[1]=='i' && RootName[2]=='p' && RootName[3]=='0' && RootName[4]=='1')
				IsBiped=1;

			Matrix33 RootMat33 = Matrix33(pSkinningInfo->m_arrBonesDesc[0].m_DefaultB2W);
			Vec3 RootPos = pSkinningInfo->m_arrBonesDesc[0].m_DefaultB2W.GetTranslation();
			Vec3 ForwardRoot		= pSkinningInfo->m_arrBonesDesc[0].m_DefaultB2W.GetColumn0();

			Matrix33 PelvisMat33(IDENTITY);
			Vec3 ForwardPelvis(ZERO); 
			uint32 numBones;
			if (numBones = pSkinningInfo->m_arrBonesDesc.size())	
			{
				PelvisMat33	= Matrix33(pSkinningInfo->m_arrBonesDesc[1].m_DefaultB2W);
				pSkinningInfo->m_arrBonesDesc[1].m_DefaultB2W.GetColumn1();
			}


			if (IsBiped)
			{
				//this is a correct BIP-model, facing Y-forward. All we need to do is adjust the root.
				uint32 xforward = ForwardRoot.IsEquivalent( Vec3(0,1,0),0.1f );		//x has to point y-forward
				uint32 yforward = ForwardPelvis.IsEquivalent( Vec3(0,1,0),0.1f );	//y has to point y-forward
				if (xforward && yforward)
				{
					Vec3 trans = pSkinningInfo->m_arrBonesDesc[0].m_DefaultB2W.GetTranslation();
					pSkinningInfo->m_arrBonesDesc[0].m_DefaultB2W.SetIdentity();
					pSkinningInfo->m_arrBonesDesc[0].m_DefaultB2W.SetTranslation(trans);
				}
			}
		}

		// Save modified content to the same chunk file.
		CSaverCGF cgfSaver( sourceFile,chunkFile );

		//---------------------------------------------------------------------
		//---  write compiled bones to chunk
		//---------------------------------------------------------------------

		int version;

		uint32 numAnimBones = pSkinningInfo->m_arrBonesDesc.size();
		if (numAnimBones) 
		{
#if SIZEOF_PTR!=4
#error CryBoneDescData must be converted to CryBoneDescData_Comp for saving
#endif
			CryBoneDescData_Comp* pCryBoneDescData = (CryBoneDescData_Comp*)&pSkinningInfo->m_arrBonesDesc[0];
			// If we need endian swap - do it
			version = COMPILED_BONE_CHUNK_DESC_0800::VERSION;
			if (bNeedEndianSwap) {
				version |= CONSOLE_VERSION_MASK;
				SwapEndian(pCryBoneDescData, numAnimBones, true);
			}

			int q = cgfSaver.SaveCompiledBones( pCryBoneDescData, numAnimBones*sizeof(CryBoneDescData), version );
		}

		//---------------------------------------------------------------------
		//---  write compiled physical bones to chunk
		//---------------------------------------------------------------------
		BONE_ENTITY* pPhysicalBone = 0;
		uint32 numPhyBones = pSkinningInfo->m_arrBonesDesc.size();
		if(numPhyBones > 0)
		{
			pPhysicalBone = &pSkinningInfo->m_arrBoneEntities[0];
		}

		version = COMPILED_PHYSICALBONE_CHUNK_DESC_0800::VERSION;
		if (bNeedEndianSwap) 
		{
			version |= CONSOLE_VERSION_MASK;
			SwapEndian(pPhysicalBone, numPhyBones, true);
		}

		int n = cgfSaver.SaveCompiledPhysicalBones( pPhysicalBone, numPhyBones*sizeof(BONE_ENTITY), version );

		//---------------------------------------------------------------------
		//--- create compiled physical proxi mesh and write it to chunk     ---
		//---------------------------------------------------------------------
		std::vector<uint8> pmt;
		uint32 numPhysicalProxies = pSkinningInfo->m_arrPhyBoneMeshes.size();
 		uint32 numMorphtargets = pSkinningInfo->m_arrMorphTargets.size();
		for(uint32 i=0; i<numPhysicalProxies; i++) 
		{
			SMeshPhysicalProxyHeader header;
			header.ChunkID			= pSkinningInfo->m_arrPhyBoneMeshes[i].ChunkID;
			header.numPoints		= pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrPoints.size();
			header.numIndices		= pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrIndices.size();
			header.numMaterials	= pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrMaterials.size();

			version = COMPILED_PHYSICALPROXY_CHUNK_DESC_0800::VERSION;
			if (bNeedEndianSwap)
				version |= CONSOLE_VERSION_MASK;

			WriteData(pmt, &header, 1, bNeedEndianSwap);

			WriteArray(pmt, pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrPoints, bNeedEndianSwap);

			WriteArray(pmt, pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrIndices, bNeedEndianSwap);

			WriteArray(pmt, pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrMaterials, bNeedEndianSwap);
		}

		cgfSaver.SaveCompiledPhysicalProxis( &pmt[0], pmt.size(), numPhysicalProxies, version );

		//---------------------------------------------------------------------
		//--- create compiled internal morph-targets and write them to chnk ---
		//---------------------------------------------------------------------
		std::vector<uint8> cmt;

		for(uint32 i=0; i<numMorphtargets; i++) 
		{
			SMeshMorphTargetHeader header;
			//store the mesh ID
			header.MeshID = pSkinningInfo->m_arrMorphTargets[i]->MeshID;
			//store the name of morph-target
			header.NameLength = pSkinningInfo->m_arrMorphTargets[i]->m_strName.size()+1;
			//store the vertices&indices of morph-target
			header.numIntVertices = pSkinningInfo->m_arrMorphTargets[i]->m_arrIntMorph.size();
			//store the vertices&indices of morph-target
			header.numExtVertices = pSkinningInfo->m_arrMorphTargets[i]->m_arrExtMorph.size();


			version = COMPILED_MORPHTARGETS_CHUNK_DESC_0800::VERSION1;
			if (bNeedEndianSwap)
				version |= CONSOLE_VERSION_MASK;

			WriteData(cmt, &header, 1, bNeedEndianSwap);

			WriteArray(cmt, pSkinningInfo->m_arrMorphTargets[i]->m_strName, bNeedEndianSwap);
			// Add ending null to name.
			cmt.push_back(0);

			WriteArray(cmt, pSkinningInfo->m_arrMorphTargets[i]->m_arrIntMorph, bNeedEndianSwap);

			WriteArray(cmt, pSkinningInfo->m_arrMorphTargets[i]->m_arrExtMorph, bNeedEndianSwap);
		}
		cgfSaver.SaveCompiledMorphTargets( &cmt[0], cmt.size(), numMorphtargets, version );


		std::set<int> usedBoneslist;
		std::vector<int> useRemap(pSkinningInfo->m_arrBonesDesc.size());

		for (uint32 e=0; e<pSkinningInfo->m_arrIntVertices.size()/*m_arrExt2IntMap.size()*/; e++)
		{
			//uint32 i=pSkinningInfo->m_arrExt2IntMap[e];

			uint8 idx0 = (uint8)pSkinningInfo->m_arrIntVertices[e].boneIDs[0];
			uint8 idx1 = (uint8)pSkinningInfo->m_arrIntVertices[e].boneIDs[1];
			uint8 idx2 = (uint8)pSkinningInfo->m_arrIntVertices[e].boneIDs[2];
			uint8 idx3 = (uint8)pSkinningInfo->m_arrIntVertices[e].boneIDs[3];

			usedBoneslist.insert(idx0);
			usedBoneslist.insert(idx1);
			usedBoneslist.insert(idx2);
			usedBoneslist.insert(idx3);
		}

		int remap = 0;
		std::vector<IntMeshCollisionInfo> arrCollisions;
		for (std::set<int>::iterator it = usedBoneslist.begin(), end = usedBoneslist.end(); it != end ; ++it, ++remap)
		{
			IntMeshCollisionInfo meshCollision;
			meshCollision.m_aABB = AABB(Vec3(VMAX),Vec3(VMIN));
			meshCollision.m_iBoneId = *it;
			arrCollisions.push_back(meshCollision);
			useRemap[*it] = remap;
		}

		for (uint32 e=0; e<pSkinningInfo->m_arrExt2IntMap.size(); e++)
		{
			uint32 i=pSkinningInfo->m_arrExt2IntMap[e];

			//uint32 i= std::distance(pSkinningInfo->m_arrExt2IntMap.begin(), (std::find(pSkinningInfo->m_arrExt2IntMap.begin(), pSkinningInfo->m_arrExt2IntMap.end(), e)));

			uint8 idx0 = (uint8)pSkinningInfo->m_arrIntVertices[i].boneIDs[0];
			uint8 idx1 = (uint8)pSkinningInfo->m_arrIntVertices[i].boneIDs[1];
			uint8 idx2 = (uint8)pSkinningInfo->m_arrIntVertices[i].boneIDs[2];
			uint8 idx3 = (uint8)pSkinningInfo->m_arrIntVertices[i].boneIDs[3];

			arrCollisions[useRemap[idx0]].m_aABB.Add(pSkinningInfo->m_arrIntVertices[i].wpos1);
			arrCollisions[useRemap[idx1]].m_aABB.Add(pSkinningInfo->m_arrIntVertices[i].wpos1);
			arrCollisions[useRemap[idx2]].m_aABB.Add(pSkinningInfo->m_arrIntVertices[i].wpos1);
			arrCollisions[useRemap[idx3]].m_aABB.Add(pSkinningInfo->m_arrIntVertices[i].wpos1);

			arrCollisions[useRemap[idx0]].m_arrIndexes.push_back(e);
			arrCollisions[useRemap[idx1]].m_arrIndexes.push_back(e);
			arrCollisions[useRemap[idx2]].m_arrIndexes.push_back(e);
			arrCollisions[useRemap[idx3]].m_arrIndexes.push_back(e);

			/*
			Vec3 binormal	= Vec3( tPackB2F(pMesh->m_pTangents[e].Binormal.x),tPackB2F(pMesh->m_pTangents[e].Binormal.y),tPackB2F(pMesh->m_pTangents[e].Binormal.z) );
			Vec3 b = z180*binormal;
			pMesh->m_pTangents[e].Binormal = Vec4sf(tPackF2B(b.x),tPackF2B(b.y),tPackF2B(b.z), pMesh->m_pTangents[e].Binormal.w);

			Vec3 tangent	= Vec3( tPackB2F(pMesh->m_pTangents[e].Tangent.x),tPackB2F(pMesh->m_pTangents[e].Tangent.y),tPackB2F(pMesh->m_pTangents[e].Tangent.z) );
			Vec3 t = z180*tangent;
			pMesh->m_pTangents[e].Tangent = Vec4sf(tPackF2B(t.x),tPackF2B(t.y),tPackF2B(t.z), pMesh->m_pTangents[e].Tangent.w);
			*/
		}

		//---------------------------------------------------------------------
		//---  write internal skinning vertices to chunk
		//---------------------------------------------------------------------
		IntSkinVertex* pIntSkinVertex = 0;
		uint32 numIntVertices = pSkinningInfo->m_arrIntVertices.size();
		if(numIntVertices>0)
		{
			pIntSkinVertex = &pSkinningInfo->m_arrIntVertices[0];
		}

		version = COMPILED_INTSKINVERTICES_CHUNK_DESC_0800::VERSION;;
		if (bNeedEndianSwap)
		{
			version |= CONSOLE_VERSION_MASK;
			SwapEndian(pIntSkinVertex, numIntVertices, bNeedEndianSwap);
		}
		int m = cgfSaver.SaveCompiledIntSkinVertices( pIntSkinVertex,numIntVertices*sizeof(IntSkinVertex), version );

		//---------------------------------------------------------------------
		//---  write internal faces to chunk
		//---------------------------------------------------------------------
		TFace* pIntFaces = 0; 
		uint32 numIntFaces = pSkinningInfo->m_arrIntFaces.size();
		if(numIntFaces > 0)
		{
			pIntFaces = &pSkinningInfo->m_arrIntFaces[0];
		}

		version = COMPILED_INTFACES_CHUNK_DESC_0800::VERSION;
		if (bNeedEndianSwap)
		{
			version |= CONSOLE_VERSION_MASK;
			SwapEndian(pIntFaces, numIntFaces, bNeedEndianSwap);
		}

		cgfSaver.SaveCompiledIntFaces( pIntFaces, numIntFaces*sizeof(TFace), version );

		//---------------------------------------------------------------------
		//---  write Ext2IntMap to chunk
		//---------------------------------------------------------------------
		uint16* pExt2IntMap = 0;
		uint32 numExt2Int = pSkinningInfo->m_arrExt2IntMap.size();
    
		if(numExt2Int > 0)
		{
			pExt2IntMap = &pSkinningInfo->m_arrExt2IntMap[0];
		}

		version = COMPILED_EXT2INTMAP_CHUNK_DESC_0800::VERSION;
    
		if (bNeedEndianSwap) 
		{      
			version |= CONSOLE_VERSION_MASK;
			SwapEndian(pExt2IntMap, numExt2Int, bNeedEndianSwap);
		}

		cgfSaver.SaveCompiledExt2IntMap( pExt2IntMap, numExt2Int*sizeof(uint16), version );


		//-----------------------------------------------------------------------------------------------

		SFileVersion fv = cc.pRC->GetFileVersion();
		CContentCGF* pCContentCGF=cgfLoader.GetCContentCGF();

		if (pCContentCGF==0) 
		{
			pCContentCGF = MakeCompiledCGF( pCGF );
			if(!pCContentCGF)
			{
				return false;
			}
		}

		//------------------------------------------------------------------
/*		uint32 numNodes = pCGF->GetNodeCount();;
		CNodeCGF* pGFXNode=0;
		for(uint32 n=0; n<numNodes; n++) 
		{
			if (pCGF->GetNode(n)->type==CNodeCGF::NODE_MESH)	
			{	
				pGFXNode = pCGF->GetNode(n);	break; 
			}
		}
		assert(pGFXNode);
		CMesh* pMesh = pGFXNode->pMesh;
		assert ( pMesh->m_pBoneMapping);
		SMeshBoneMapping* pBone = pMesh->m_pBoneMapping;
		SMeshShapeDeformation* pShape = pMesh->m_pShapeDeformation;*/

		//------------------------------------------------------------------

		int nWrongModelFormat=0;
		if (pCContentCGF && nWrongModelFormat)
		{
			uint32 numNodes = pCGF->GetNodeCount();;
			CNodeCGF* pGFXNode=0;
			for(uint32 n=0; n<numNodes; n++) 
			{
				if (pCGF->GetNode(n)->type==CNodeCGF::NODE_MESH)	
				{	
					pGFXNode = pCGF->GetNode(n);	
					for (uint32 e = 0; e < pGFXNode->pMesh->m_numVertices; ++e) {
						pGFXNode->pMesh->m_pPositions[e] = pGFXNode->pMesh->m_pPositions[e];
						//pGFXNode->pMesh->m_pShapeDeformation[e].thin = z180 * pGFXNode->pMesh->m_pShapeDeformation[e].thin;
						//pGFXNode->pMesh->m_pShapeDeformation[e].fat = z180 * pGFXNode->pMesh->m_pShapeDeformation[e].fat;
//						pGFXNode->pMesh->

						Vec3 binormal	= Vec3( tPackB2F(pGFXNode->pMesh->m_pTangents[e].Binormal.x),tPackB2F(pGFXNode->pMesh->m_pTangents[e].Binormal.y),tPackB2F(pGFXNode->pMesh->m_pTangents[e].Binormal.z) );
						Vec3 b = binormal;
						pGFXNode->pMesh->m_pTangents[e].Binormal = Vec4sf(tPackF2B(b.x),tPackF2B(b.y),tPackF2B(b.z), pGFXNode->pMesh->m_pTangents[e].Binormal.w);

						Vec3 tangent	= Vec3( tPackB2F(pGFXNode->pMesh->m_pTangents[e].Tangent.x),tPackB2F(pGFXNode->pMesh->m_pTangents[e].Tangent.y),tPackB2F(pGFXNode->pMesh->m_pTangents[e].Tangent.z) );
						Vec3 t = tangent;
						pGFXNode->pMesh->m_pTangents[e].Tangent = Vec4sf(tPackF2B(t.x),tPackF2B(t.y),tPackF2B(t.z), pGFXNode->pMesh->m_pTangents[e].Tangent.w);

						/*
						pSkinningInfo->m_arrIntVertices[e].wpos0 = z180*pSkinningInfo->m_arrIntVertices[e].wpos0;
						pSkinningInfo->m_arrIntVertices[e].wpos1 = z180*pSkinningInfo->m_arrIntVertices[e].wpos1;
						pSkinningInfo->m_arrIntVertices[e].wpos2 = z180*pSkinningInfo->m_arrIntVertices[e].wpos2;
						*/
						/*
						Vec3 binormal	= Vec3( tPackB2F(pMesh->m_pTangents[e].Binormal.x),tPackB2F(pMesh->m_pTangents[e].Binormal.y),tPackB2F(pMesh->m_pTangents[e].Binormal.z) );
						Vec3 b = z180*binormal;
						pMesh->m_pTangents[e].Binormal = Vec4sf(tPackF2B(b.x),tPackF2B(b.y),tPackF2B(b.z), pMesh->m_pTangents[e].Binormal.w);

						Vec3 tangent	= Vec3( tPackB2F(pMesh->m_pTangents[e].Tangent.x),tPackB2F(pMesh->m_pTangents[e].Tangent.y),tPackB2F(pMesh->m_pTangents[e].Tangent.z) );
						Vec3 t = z180*tangent;
						pMesh->m_pTangents[e].Tangent = Vec4sf(tPackF2B(t.x),tPackF2B(t.y),tPackF2B(t.z), pMesh->m_pTangents[e].Tangent.w);
						*/

					}

					std::vector<TFace> arrExtFaces;
					arrExtFaces.resize(pSkinningInfo->m_arrIntFaces.size());

					for (uint32 f=0; f<pSkinningInfo->m_arrIntFaces.size(); f++)
					{
						arrExtFaces[f].i0 = pGFXNode->pMesh->m_pIndices[f*3 +0];//std::distance(pSkinningInfo->m_arrExt2IntMap.begin(), std::find(pSkinningInfo->m_arrExt2IntMap.begin(), pSkinningInfo->m_arrExt2IntMap.end(), pSkinningInfo->m_arrIntFaces[f].i0));//[f*3+0];
						arrExtFaces[f].i1 = pGFXNode->pMesh->m_pIndices[f*3 +1];//std::distance(pSkinningInfo->m_arrExt2IntMap.begin(),std::find(pSkinningInfo->m_arrExt2IntMap.begin(), pSkinningInfo->m_arrExt2IntMap.end(), pSkinningInfo->m_arrIntFaces[f].i1));//[f*3+1];
						arrExtFaces[f].i2 = pGFXNode->pMesh->m_pIndices[f*3 +2];//std::distance(pSkinningInfo->m_arrExt2IntMap.begin(),std::find(pSkinningInfo->m_arrExt2IntMap.begin(), pSkinningInfo->m_arrExt2IntMap.end(), pSkinningInfo->m_arrIntFaces[f].i2));////[f*3+2];
					}


					for (int i = 0; i < arrCollisions.size(); ++i)
					{
						std::set<int> unique;

						for (int j = 0; j < arrCollisions[i].m_arrIndexes.size(); ++j)
						{
							unique.insert(arrCollisions[i].m_arrIndexes[j]);
						}

						std::vector<short int> tmp;
						tmp.swap(arrCollisions[i].m_arrIndexes);
						arrCollisions[i].m_arrIndexes.reserve(unique.size());

						for (uint32 j = 0; j < arrExtFaces.size(); ++j)
						{
							if ((unique.find(arrExtFaces[j].i0) != unique.end()) || (unique.find(arrExtFaces[j].i1) != unique.end()) || (unique.find(arrExtFaces[j].i2) != unique.end()))
							{
								arrCollisions[i].m_arrIndexes.push_back(j);
							}
						}
					}

					std::vector<short int> tmp;
					tmp.swap(arrCollisions[0].m_arrIndexes);

				}
			}
		}
		version = COMPILED_BONEBOXES_CHUNK_DESC_0800::VERSION1;
		if (bNeedEndianSwap)
			version |= CONSOLE_VERSION_MASK;


		for (uint32 c = 0, endc = arrCollisions.size(); c < endc; ++c) 
		{
			std::vector<char> data;

			data.resize(sizeof(int) + sizeof(AABB) + sizeof(int) + arrCollisions[c].m_arrIndexes.size() * sizeof(short int));
			char * pData = &data[0];
			int size = arrCollisions[c].m_arrIndexes.size();
			if (bNeedEndianSwap) {
				SwapEndian(arrCollisions[c].m_iBoneId, true);
				SwapEndian(size, true);
				SwapEndian(arrCollisions[c].m_aABB, true);
			}
			memcpy(pData, &arrCollisions[c].m_iBoneId, sizeof(int));
			pData += sizeof(int);
			memcpy(pData, &arrCollisions[c].m_aABB, sizeof(AABB));
			pData += sizeof(AABB);

			memcpy(pData, &size, sizeof(int));
			pData += sizeof(int);
			if (size)
				memcpy(pData, &arrCollisions[c].m_arrIndexes[0], arrCollisions[c].m_arrIndexes.size() * sizeof(short int));

			cgfSaver.SaveCompiledBoneBox(&data[0], data.size(), version);
		}

		if (pCContentCGF) 
		{

			pCContentCGF->GetExportInfo()->rc_version[0] = fv.v[0];
			pCContentCGF->GetExportInfo()->rc_version[1] = fv.v[1];
			pCContentCGF->GetExportInfo()->rc_version[2] = fv.v[2];
			pCContentCGF->GetExportInfo()->rc_version[3] = fv.v[3];
			sprintf( pCContentCGF->GetExportInfo()->rc_version_string," RCVer:%d.%d ",fv.v[2],fv.v[1] );
			cgfSaver.SetContent( pCContentCGF );
			// Only store 
			cgfSaver.SaveExportFlags();
		}

		bool bNeedCompressVertices = false;

		cgfSaver.SaveNodes(bNeedEndianSwap, bNeedCompressVertices, bUseQuaternions);

		// Force remove of the read only flag.
		SetFileAttributes( outputFile, FILE_ATTRIBUTE_ARCHIVE );

		{
			// We write temporary file to prevent external file readers from reading half-written file

			string tmpFilename = outputFile;
			tmpFilename += ".$tmp$";

			ok = chunkFile.Write(tmpFilename.c_str());

			remove(outputFile.c_str());

			ok = ok && (rename(tmpFilename.c_str(), outputFile.c_str()) == 0);
		}

		delete pCGF;
		delete cgfLoader.GetCContentCGF();
	}
	catch(char*)
	{
		Beep(1000,1000);
		ok = false;
	}

	return ok;
}

//////////////////////////////////////////////////////////////////////////
void CharacterCompiler::DeleteOldChunks( CContentCGF *pCGF,CChunkFile &chunkFile )
{


	for (int i = 0; i < pCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pNode = pCGF->GetNode(i);
		if (pNode->type == CNodeCGF::NODE_MESH && pNode->nChunkId)
		{
			chunkFile.DeleteChunkId(pNode->nChunkId); // Delete chunk of node.
			if (pNode->nObjectChunkId)
				chunkFile.DeleteChunkId(pNode->nObjectChunkId); // Delete chunk of mesh.
		}
	}


	// Delete all mesh chunks.
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_Mesh);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}


	// Delete all mesh subsets.
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_MeshSubsets);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	// Delete all data streams.
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_DataStream);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	// Delete all precompiled physics data streams.
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_MeshPhysicsData);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_ExportFlags);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	//---------------------------------------------------

	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_MeshMorphTarget);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	// Delete all bone-namelists.
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_BoneNameList);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	// Delete all bone-positions.
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_BoneInitialPos);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	// Delete all bones.
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_BoneAnim);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_BoneMesh);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}


	//-----------------------------------------------------------------------------------------------
	//delete all compiled chunks
	//-----------------------------------------------------------------------------------------------
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledBones);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledPhysicalBones);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledPhysicalProxies);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledMorphTargets);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledIntFaces);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledIntSkinVertices);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledExt2IntMap);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}
	while (true) {
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_BonesBoxes);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}

}

//////////////////////////////////////////////////////////////////////////
CContentCGF* CharacterCompiler::MakeCompiledCGF( CContentCGF *pCGF )
{
	CContentCGF *pCompiledCGF = new CContentCGF( pCGF->GetFilename() );
	*pCompiledCGF->GetExportInfo() = *pCGF->GetExportInfo(); // Copy export info.

	{
		// Compile meshes in nodes.
		for (int i = 0, num = pCGF->GetNodeCount(); i < num; ++i)
		{
			CNodeCGF *pNodeCGF = pCGF->GetNode(i);
			if (!pNodeCGF->pMesh || pNodeCGF->bPhysicsProxy)
				continue;

//			strcpy( g_sCurrentNode,pNodeCGF->name.c_str() );

			mesh_compiler::CMeshCompiler meshCompiler;
			if (!meshCompiler.Compile( *pNodeCGF->pMesh ))
			{
				RCLogError( "Failed to compile geometry file %s - %s",pCGF->GetFilename(),meshCompiler.GetLastError() );
				delete pCompiledCGF;
				return 0;
			}
		}

		for (int i = 0, num = pCGF->GetNodeCount(); i < num; ++i)
		{
			CNodeCGF *pNodeCGF = pCGF->GetNode(i);
			if (pNodeCGF->pMesh && (pNodeCGF->type == CNodeCGF::NODE_MESH))
			{
				pCompiledCGF->AddNode( pNodeCGF );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Compile physics proxy nodes.
	//////////////////////////////////////////////////////////////////////////
	if (pCGF->GetExportInfo()->bHavePhysicsProxy)
	{
		for (int i = 0, num = pCGF->GetNodeCount(); i < num; ++i)
		{
			CNodeCGF *pNodeCGF = pCGF->GetNode(i);
			if (pNodeCGF->pMesh && pNodeCGF->bPhysicsProxy)
			{
//				strcpy( g_sCurrentNode,pNodeCGF->name.c_str() );

				// Compile physics proxy mesh.
				mesh_compiler::CMeshCompiler meshCompiler;
				if (!meshCompiler.Compile( *pNodeCGF->pMesh,mesh_compiler::MESH_COMPILE_OPTIMIZE ))
				{
					RCLogError( "Failed to compile geometry in node %s in file %s - %s",pNodeCGF->name,pCGF->GetFilename(),meshCompiler.GetLastError() );
					delete pCompiledCGF;
					return 0;
				}
			}
			pCompiledCGF->AddNode(pNodeCGF);
		}
	}
	//////////////////////////////////////////////////////////////////////////

	const bool ok = Physicalize( pCompiledCGF );
	if (!ok)
	{
		RCLogError( "Failed to physicalize geometry in file %s (bad vertex coordinates in source mesh, probably)",pCGF->GetFilename() );
		delete pCompiledCGF;
		return 0;
	}

	return pCompiledCGF;
}

//////////////////////////////////////////////////////////////////////////
bool CharacterCompiler::Physicalize( CContentCGF *pCGF )
{
	if (!m_pPhysicsInterface)
		m_pPhysicsInterface = new CPhysicsInterface;

	for (int i = 0; i < pCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pNode = pCGF->GetNode(i);
		if (!pNode->pMesh)
			continue;

		CPhysicsInterface::EPhysicalizeResult res = m_pPhysicsInterface->Physicalize( pNode,pCGF );
		if(res == CPhysicsInterface::ePR_Fail)
		{
			return false;
		}
	}

	return true;
}

