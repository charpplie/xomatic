////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   StatCGFSHCompiler.cpp
//  Version:     v1.00
//  Created:     15/12/2004 by Michael Glueck
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "StatCGFSHCompiler.h"
#include "../../PRT/SHFramework.h"
#include "CGFContent.h"
#include <windows.h>

typedef NSH::CSmartPtr<NSH::NMaterial::ISHMaterial, CSHAllocator<unsigned int> > TSHMaterial;

const bool CStatCFGSHCompiler::Calculate()
{ 
//	MessageBox( NULL,"Attach me","Debug",MB_OK|MB_ICONERROR );
	//if sh coefficients are already computed, return
	if(m_pMesh->m_pSHInfo && m_pMesh->m_pSHInfo->pSHCoeffs && m_pMesh->m_pSHInfo->nDecompressionCount == m_pMesh->m_subsets.size())
		return true;
	//check for valid pointers
	if(!m_pMesh->m_pNorms || !m_pMesh->m_pPositions || !m_pMesh->m_pTangents || !m_pMesh->m_pTexCoord || !m_pMesh->m_pIndices)
	{
		RCLogError("Object has no normals/texture coords/indices or positions set\n");
		return false;
	}
	if (m_pCGF->GetMaterialCount() == 0)
		return true;

	//first check whether there is at least one material requiring sh coefficients
	bool needsSHCoeffs = false;
	for(size_t subMeshCount=0; subMeshCount<m_pMesh->m_subsets.size(); ++subMeshCount)
	{
		const int matID = m_pMesh->m_subsets[subMeshCount].nMatID;
		const CMaterialCGF *cpMatCGF = GetMaterialFromID(m_cpMat, m_pCGF, matID);
		if(cpMatCGF)
		{
			if(cpMatCGF->nFlags & MTL_NAME_CHUNK_DESC::FLAG_SH_COEFFS)
			{
				needsSHCoeffs = true;
				break;
			}
		}
	}
	if(!needsSHCoeffs)
		return true;

	//we have at least one material which needs sh coeffs
	NSH::NTransfer::STransferParameters transferParams;		 
	SetTransferParameters(transferParams);
	CSimpleIndexedMesh shMesh;

	if(!CreateSHMaterials(shMesh, transferParams))
	{
		RCLogError("Failed to create sh materials for SH calculation");
		return false;
	}

	if(!CreateSHMesh(shMesh)) 
	{
		RCLogError("Failed to create mesh for SH calculation");
		return false;
	}
	//now create SH data
	NSH::NFramework::SMeshCompressedCoefficients coefficients;
	RCLog("computing spherical harmonics coefficients:");
//		MessageBox( NULL,"Attach me","Debug",MB_OK|MB_ICONERROR );
	if(!NSH::NFramework::ComputeSingleMeshTransferCompressed(shMesh, transferParams, coefficients))
		RCLogError("ComputeSingleMeshTransferCompressed failed\n");
	if(!ExportSHValues(shMesh, coefficients, transferParams))
	{
		RCLogError("Failed to export sh coefficients and decompression matrices for SH calculation");
		return false;
	}
	RCLog("\n"); 
	return true; //sh coeffs computed 	 
}

const bool CStatCFGSHCompiler::SetTransferParameters(NSH::NTransfer::STransferParameters& rParam)
{
	rParam.bumpGranularity = true; 
//	rParam.backlightingColour = Vec3(85.f/255.f, 255.f/255.f, 97.f/255.f);
	rParam.backlightingColour = Vec3(1.0f, 1.0f, 1.0f);
	rParam.transparencyShadowingFactor = 1.0f;
	rParam.groundPlaneBlockValue = 1.0f;
	rParam.configType = NSH::NTransfer::TRANSFER_CONFIG_VEGETATION;

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	rParam.rayCastingThreads = std::max(systemInfo.dwNumberOfProcessors, (DWORD)1);
	//rParam.rayCastingThreads = 1;

	rParam.sampleCountPerVertex = 2000;
  rParam.rayTracingBias = 0.000001f;

	rParam.supportTransparency = true; 

	NSH::NFramework::SFrameworkParameter frameworkParams;
	frameworkParams.sampleCount = 20000;
	frameworkParams.supportedBands = 3;
	frameworkParams.minSampleCountToRetrieve = rParam.sampleCountPerVertex;
	//init sh framework
	NSH::NFramework::CSHFrameworkManager::Instance(frameworkParams);

	return true;
}

const bool CStatCFGSHCompiler::CreateSHMaterials(CSimpleIndexedMesh& rSHMesh, NSH::NTransfer::STransferParameters& rTransferParams)
{
	NSH::NMaterial::CMaterialFactory *pMaterialFactory = NSH::NMaterial::CMaterialFactory::Instance();
	if(!pMaterialFactory)
		return false;
	uint32 matIndex = 0;

	bool useAmbientTerm = false;
	for(size_t subMeshCount=0; subMeshCount<m_pMesh->m_subsets.size(); ++subMeshCount)
	{
		const SMeshSubset& crSubset = m_pMesh->m_subsets[subMeshCount];
		const uint32 cMatID = crSubset.nMatID;
		if(cMatID >= (uint32)m_pCGF->GetMaterialCount())
			return false;
		const CMaterialCGF *cpMatCGF = GetMaterialFromID(m_cpMat, m_pCGF, cMatID);
		if(cpMatCGF)
		{
			const bool cNeedSHCoeffs	= ((cpMatCGF->nFlags & MTL_NAME_CHUNK_DESC::FLAG_SH_COEFFS) != 0);
			const bool cIs2Sided			= ((cpMatCGF->nFlags & MTL_NAME_CHUNK_DESC::FLAG_SH_2SIDED) != 0);
			const float cOpacity			= cpMatCGF->shOpacity;
			if((cpMatCGF->nFlags & MTL_NAME_CHUNK_DESC::FLAG_SH_AMBIENT) != 0)
				useAmbientTerm = true;//if at least one mat wants ambient term, use it for all for consistency
			//now create the non textured materials from that
			//create new material
			//we dont have enough data to do this actually, so assume everything opaque as default material, anything with transparency as alpha mat 
			//and if it has sh flag, set it as backlighting material
			TSHMaterial pNewMat;
			NSH::NMaterial::EMaterialType useMatType = NSH::NMaterial::MATERIAL_TYPE_DEFAULT;
			if(cOpacity < 1.f)
			{
				if(cOpacity > 0.f && cNeedSHCoeffs)
					useMatType = NSH::NMaterial::MATERIAL_TYPE_BACKLIGHTING_DEFAULT;
				else
					useMatType = NSH::NMaterial::MATERIAL_TYPE_ALPHA_DEFAULT;
			}
			//set init data required by factory 
			NSH::NMaterial::SSharedMaterialInitData matInitData;
			matInitData.pMesh = &rSHMesh;
			matInitData.transparencyShadowingFactor = rTransferParams.transparencyShadowingFactor;
			matInitData.diffuseIntensity.a = std::max(0.f, 1.f - cOpacity);//no interreflection but alpha used
			matInitData.backlightingColour = rTransferParams.backlightingColour;
			pNewMat = pMaterialFactory->GetMaterial(useMatType, matInitData);

			rSHMesh.AddMaterial(SAddMaterialProperty(pNewMat, (cOpacity > 0.f), cNeedSHCoeffs, true/*always do that, we need 360 degree here*/));
			m_MatToSHIDindexMap.insert(std::make_pair(cMatID, matIndex++));
		}
	}	
	rTransferParams.minDirectBumpCoeffVisibility = useAmbientTerm?0.15f : 0.f;
	return true;
}

const bool CStatCFGSHCompiler::CreateSHMesh(CSimpleIndexedMesh& rSHMesh)
{
	Vec3 minExt(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()), 
		maxExt(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

	rSHMesh.SetMeshName(m_pCGF->GetFilename());
	rSHMesh.AllocateNormals(m_pMesh->GetVertexCount());
	rSHMesh.AllocateVertices(m_pMesh->GetVertexCount());
	rSHMesh.AllocateTexCoords(m_pMesh->GetTexCoordsCount());

	for(uint32 i=0; i<(uint32)rSHMesh.GetVertexCount(); ++i)
	{
		const Vec3& crVertex = m_pMesh->m_pPositions[i];

		rSHMesh.GetVertex(i)			= crVertex;
		rSHMesh.GetWSNormal(i)		= rSHMesh.GetNormal(i) = m_pMesh->m_pNorms[i];

		rSHMesh.GetBiNormal(i).x	= tPackB2F(m_pMesh->m_pTangents[i].Binormal[0]);
		rSHMesh.GetBiNormal(i).y	= tPackB2F(m_pMesh->m_pTangents[i].Binormal[1]);
		rSHMesh.GetBiNormal(i).z	= tPackB2F(m_pMesh->m_pTangents[i].Binormal[2]);

		rSHMesh.GetTNormal(i).x	= tPackB2F(m_pMesh->m_pTangents[i].Tangent[0]);
		rSHMesh.GetTNormal(i).y	= tPackB2F(m_pMesh->m_pTangents[i].Tangent[1]);
		rSHMesh.GetTNormal(i).z	= tPackB2F(m_pMesh->m_pTangents[i].Tangent[2]);  

		minExt.x = (minExt.x>crVertex.x)?crVertex.x:minExt.x;
		minExt.y = (minExt.y>crVertex.y)?crVertex.y:minExt.y;
		minExt.z = (minExt.z>crVertex.z)?crVertex.z:minExt.z;

		maxExt.x = (maxExt.x<crVertex.x)?crVertex.x:maxExt.x;
		maxExt.y = (maxExt.y<crVertex.y)?crVertex.y:maxExt.y;
		maxExt.z = (maxExt.z<crVertex.z)?crVertex.z:maxExt.z;
	}
	for(uint32 i=0; i<(uint32)rSHMesh.GetTexCoordCount(); ++i)
	{	
		rSHMesh.GetTexCoord(i).s = m_pMesh->m_pTexCoord[i].s;
		rSHMesh.GetTexCoord(i).t = m_pMesh->m_pTexCoord[i].t;
	}
	rSHMesh.SetMaxExt(maxExt);
	rSHMesh.SetMinExt(minExt);

	const uint32 cFaceCount = m_pMesh->m_nIndexCount/3;
	rSHMesh.AllocateFaces(cFaceCount);
	//build from index data
	int counter = 0;
	//assert(rSHMesh.GetTexCoordCount() == rSHMesh.GetVertexCount());//not working here yet
	for(size_t subMeshCount=0; subMeshCount<m_pMesh->m_subsets.size(); ++subMeshCount)
	{
		const SMeshSubset& crSubset = m_pMesh->m_subsets[subMeshCount];
		const uint32 cMatID = crSubset.nMatID;
		int v=crSubset.nFirstIndexId;
		for(; v<crSubset.nFirstIndexId+crSubset.nNumIndices; v+=3)
		{
			assert(counter < (int)rSHMesh.GetFaceCount());
			rSHMesh.GetObjFace(counter).t[0]	= rSHMesh.GetObjFace(counter).n[0]	= rSHMesh.GetObjFace(counter).v[0]	= m_pMesh->m_pIndices[v];
			rSHMesh.GetObjFace(counter).t[1]	= rSHMesh.GetObjFace(counter).n[1]	= rSHMesh.GetObjFace(counter).v[1]	= m_pMesh->m_pIndices[v+1];
			rSHMesh.GetObjFace(counter).t[2]	= rSHMesh.GetObjFace(counter).n[2]	= rSHMesh.GetObjFace(counter).v[2]	= m_pMesh->m_pIndices[v+2];
			std::map<uint32, uint32>::const_iterator cMapIter = m_MatToSHIDindexMap.find(cMatID);
			if(cMapIter == m_MatToSHIDindexMap.end())
				return false;
			rSHMesh.GetObjFace(counter).shaderID = cMapIter->second;//get mapping into sh material vector
			++counter;				
		}
		assert(v == crSubset.nFirstIndexId+crSubset.nNumIndices);
	}
	return true;
}

const bool CStatCFGSHCompiler::ExportSHValues
(
	CSimpleIndexedMesh& rSHMesh, 
	const NSH::NFramework::SMeshCompressedCoefficients& crCoefficients, 
	const NSH::NTransfer::STransferParameters& crTransferParams
)
{
	//now store compressed matrices into Mesh
	CMesh& rMesh = *m_pMesh;
	if(!rMesh.m_pSHInfo)
	{
		rMesh.m_pSHInfo = new SSHInfo;
		const SCoefficientExportPolicy& crExportPolicy = rSHMesh.GetExportPolicy();
		rMesh.m_pSHInfo->pDecompressions = new SSHDecompressionMat[m_pMesh->m_subsets.size()];	assert(rMesh.m_pSHInfo->pDecompressions);
		rMesh.m_pSHInfo->nDecompressionCount = m_pMesh->m_subsets.size();
		for(size_t subMeshCount=0; subMeshCount<m_pMesh->m_subsets.size(); ++subMeshCount)
		{
			//store decompression matrix for each subset
			const SMeshSubset& crSubset = m_pMesh->m_subsets[subMeshCount];
			const uint32 cMatID = crSubset.nMatID;
			std::map<uint32, uint32>::const_iterator cMapIter = m_MatToSHIDindexMap.find(cMatID);
			if(cMapIter == m_MatToSHIDindexMap.end())
				return false;
			assert(cMapIter->second < crExportPolicy.materialCompressionInfo.size());
			const NSH::SCompressionInfo& crDecompressionInfo = (crExportPolicy.materialCompressionInfo)[cMapIter->second];
			SSHDecompressionMat& rSubsetDecompressionMatrix = rMesh.m_pSHInfo->pDecompressions[subMeshCount];//dest
			if(!crDecompressionInfo.isCompressed)
			{
				memset(&rSubsetDecompressionMatrix, 0, sizeof(SSHDecompressionMat));
				continue;
			}
			const NSH::TFloatPairVec& crDecompressionValues = crDecompressionInfo.compressionValue;	//source
			assert(crDecompressionValues.size() == 8);//8 coefficients
			for(int i=0; i<4; ++i)
			{//invert values to gain decompression values from compression values
				rSubsetDecompressionMatrix.offset0[i] = -crDecompressionValues[i].second;//offset for first 4 coefficients
				rSubsetDecompressionMatrix.offset1[i] = -crDecompressionValues[i+4].second;//offset for second 4 coefficients
				rSubsetDecompressionMatrix.scale0[i]	= 1.f/crDecompressionValues[i].first;//scale for first 4 coefficients
				rSubsetDecompressionMatrix.scale1[i]	= 1.f/crDecompressionValues[i+4].first;//scale for second 4 coefficients
			}
		} 
	}
	//store coefficients
	const size_t cCoeffSize = sizeof(SMeshSHCoeffs) * rSHMesh.GetVertexCount();
	assert(cCoeffSize == crCoefficients.size);
	rMesh.m_pSHInfo->pSHCoeffs = (SMeshSHCoeffs*)malloc(cCoeffSize);
	assert(rMesh.m_pSHInfo->pSHCoeffs);
	memcpy(rMesh.m_pSHInfo->pSHCoeffs, &crCoefficients.pDirectCoeffs[0], cCoeffSize);
	return true;
}

const CMaterialCGF* CStatCFGSHCompiler::GetMaterialFromID(const CMaterialCGF *cpParentMat, CContentCGF *cpCGF, const uint32 cMatID)
{
	if(cpParentMat)
	{
		if(!cpParentMat->subMaterials.empty())
		{
			if (cMatID < cpParentMat->subMaterials.size())
				return cpParentMat->subMaterials[cMatID];
		}
		else
			return cpParentMat;
	}
	else
	{
		if (cMatID < (uint32)cpCGF->GetMaterialCount())
			return cpCGF->GetMaterial(cMatID);
	}
	return 0;
}
