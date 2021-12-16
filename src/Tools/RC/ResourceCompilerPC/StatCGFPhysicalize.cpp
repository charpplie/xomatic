////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   StatCGFPhysicalize.cpp
//  Version:     v1.00
//  Created:     8/11/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StatCGFPhysicalize.h"
#include "MeshCompiler\MeshCompiler.h"

#define SMALL_MESH_NUM_INDEX 30	

//////////////////////////////////////////////////////////////////////////
CPhysicsInterface::CPhysicsInterface()
{
	m_pPhysicalWorld = m_physLoader.GetWorldPtr();
}

//////////////////////////////////////////////////////////////////////////
CPhysicsInterface::~CPhysicsInterface()
{
}

//////////////////////////////////////////////////////////////////////////
CPhysicsInterface::EPhysicalizeResult CPhysicsInterface::Physicalize( CNodeCGF *pNodeCGF,CContentCGF *pCGF )
{
	if (!pNodeCGF->pMesh)
		return ePR_Empty;

	CPhysicsInterface::EPhysicalizeResult res;

	res = PhysicalizeGeomType( PHYS_GEOM_TYPE_DEFAULT,pNodeCGF,pCGF );
	if(res == ePR_Fail)
	{
		return ePR_Fail;
	}

	res = PhysicalizeGeomType( PHYS_GEOM_TYPE_NO_COLLIDE,pNodeCGF,pCGF );
	if(res == ePR_Fail)
	{
		return ePR_Fail;
	}

	res = PhysicalizeGeomType( PHYS_GEOM_TYPE_OBSTRUCT,pNodeCGF,pCGF );
	if(res == ePR_Fail)
	{
		return ePR_Fail;
	}

	return ePR_Ok;
}

//////////////////////////////////////////////////////////////////////////
CPhysicsInterface::EPhysicalizeResult CPhysicsInterface::PhysicalizeGeomType( int nGeomType,CNodeCGF *pNodeCGF,CContentCGF *pCGF )
{
	CMesh &mesh = *pNodeCGF->pMesh;

	CPhysicalizeInfoCGF &physInfo = *pCGF->GetPhysiclizeInfo();

	std::vector<uint16> indices;
	std::vector<uint16> indicesRemap;
	std::vector<char> faceMaterials;
	std::vector<int> originalFaces;
	indices.reserve( mesh.GetIndexCount() );
	indicesRemap.reserve( mesh.GetIndexCount() );
	originalFaces.reserve( mesh.GetIndexCount()/3 );

	int *pForeignIndices = 0;
	bool bPhysicalizeProxy = (pNodeCGF->type == CNodeCGF::NODE_HELPER) || pNodeCGF->bPhysicsProxy;
	bool bHasRenderPhysicsGeometry = false;

	for (int m = 0; m < mesh.GetSubSetCount(); m++)
	{
		SMeshSubset &subset = mesh.m_subsets[m];
		
		if (subset.nPhysicalizeType == PHYS_GEOM_TYPE_NONE)
			continue;
		if ((subset.nPhysicalizeType != nGeomType) && !(subset.nPhysicalizeType==PHYS_GEOM_TYPE_DEFAULT_PROXY && nGeomType==PHYS_GEOM_TYPE_DEFAULT))
			continue;
		if (subset.nNumIndices <= 0 || subset.nNumVerts <= 0)
			continue;

		// Everything not PHYS_GEOM_TYPE_DEFAULT is a physical proxy and don't need foreign ids.
		if (subset.nPhysicalizeType == PHYS_GEOM_TYPE_DEFAULT && !bPhysicalizeProxy)
		{
			bHasRenderPhysicsGeometry = true;
		}
		else
		{
			bPhysicalizeProxy = true;
		}

		if ( bHasRenderPhysicsGeometry && bPhysicalizeProxy && pNodeCGF->type != CNodeCGF::NODE_HELPER && !pNodeCGF->bPhysicsProxy)
		{
			RCLogError("Error in Calculating Physics Mesh, Node '%s' in file '%s' contains both physicalised render geometry and a physics proxy. This will cause corruption to the physics mesh in game. To fix this you can do either of the following:\n\t 1) Create a physics proxy for all of your mesh and do not have physicalised render geometry.\n\t 2) Remove your physics proxies and only have physicalised render geometry.\n\t 3) Have the physicalised render geometry and the physics proxies in seperate nodes and do not merge them on export.",pNodeCGF->name, pCGF->GetFilename());
		}

		if ( bHasRenderPhysicsGeometry && bPhysicalizeProxy && pNodeCGF->type == CNodeCGF::NODE_HELPER && !pNodeCGF->bPhysicsProxy)
		{
			RCLog("Node '%s' in file '%s' contains physicalised render geometry and is also a helper node. This could be a lod, are you sure the intention is to have physics on this helper?",pNodeCGF->name, pCGF->GetFilename());
		}

		for (int idx = subset.nFirstIndexId; idx < subset.nFirstIndexId+subset.nNumIndices; idx += 3)
		{
			indices.push_back( mesh.m_pIndices[idx] );
			indices.push_back( mesh.m_pIndices[idx+1] );
			indices.push_back( mesh.m_pIndices[idx+2] );
			faceMaterials.push_back( subset.nMatID );
			int nOriginalFace = idx/3;
			originalFaces.push_back( nOriginalFace );
		}
	}

	if (originalFaces.size() > 0 && !bPhysicalizeProxy)
	{
		pForeignIndices = &originalFaces[0];
	}

	if (indices.empty())
		return ePR_Empty;

	Vec3 *pVertices = pVertices = mesh.m_pPositions;
	int nVerts = mesh.GetVertexCount();

	// We have to check if the input data are ok (3D MAX produces "not a number" coordinates sometimes)
	{
		for (int i = 0; i < nVerts; i++)
		{
			if(!mesh.m_pPositions[i].IsValid())
			{
				RCLog("Mesh of the Node '%s' in file '%s' contains invalid vertex position at index %d",pNodeCGF->name, pCGF->GetFilename(),i );
				return ePR_Fail;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if (pNodeCGF->type == CNodeCGF::NODE_HELPER && pCGF->GetExportInfo()->bMergeAllNodes)
	{
		// Transform physics proxy nodes into the world space when everything must be merged.
		pVertices = new Vec3[nVerts];
		for (int i = 0; i < nVerts; i++)
		{
			pVertices[i] = pNodeCGF->worldTM.TransformPoint(mesh.m_pPositions[i]);
		}
	}
	else if (pNodeCGF->type == CNodeCGF::NODE_HELPER && !pCGF->GetExportInfo()->bMergeAllNodes)
	{
		// Transform physics proxy nodes into the local space for not merged objects.
		pVertices = new Vec3[nVerts];
		for (int i = 0; i < nVerts; i++)
		{
			pVertices[i] = pNodeCGF->localTM.TransformPoint(mesh.m_pPositions[i]);
		}
	}

	if (nVerts > 2)
	{
		IGeomManager *pGeoman = GetPhysicalWorld()->GetGeomManager();
		Vec3 ptmin = mesh.m_bbox.min;
		Vec3 ptmax = mesh.m_bbox.max;

		int flags = mesh_multicontact0;
		float tol = 0.05f;
		flags |= indices.size() <= SMALL_MESH_NUM_INDEX ? mesh_SingleBB : mesh_OBB|mesh_AABB|mesh_AABB_rotated;
		flags |= mesh_approx_box | mesh_approx_sphere | mesh_approx_cylinder | mesh_approx_capsule;

		if (pForeignIndices)
		{
			// When no foreign indices there is no need to save vtxmap as well.
			flags |= mesh_keep_vtxmap_for_saving;
		}
		flags |= mesh_full_serialization;

		//////////////////////////////////////////////////////////////////////////
		// Get flags from properties string.
		//////////////////////////////////////////////////////////////////////////
		string props = pNodeCGF->properties;
		if (!props.empty())
		{
			int curPos = 0;
			string token = props.Tokenize("\n",curPos);
			while (token != "")
			{
				token = token.Trim();

				if (strstr(token,"wheel") || strstr(token,"cylinder"))
				{
					flags &= ~(mesh_approx_box | mesh_approx_sphere | mesh_approx_capsule);
					flags |= mesh_approx_cylinder;
					tol = 1.0f;
				} else if (strstr(token,"explosion") || strstr(token,"crack")) // temporary solution
				{
					flags = flags & ~mesh_multicontact1 | mesh_multicontact2;	// temporary solution
					flags &= ~(mesh_approx_box | mesh_approx_sphere | mesh_approx_cylinder);
				}	else if (strstr(token,"box"))
				{
					flags &= ~(mesh_approx_cylinder | mesh_approx_sphere | mesh_approx_capsule);
					flags |= mesh_approx_box;
					tol = 1.0f;	
				}	else if (strstr(token,"sphere"))
				{
					flags &= ~(mesh_approx_cylinder | mesh_approx_box | mesh_approx_capsule);
					flags |= mesh_approx_sphere;
					tol = 1.0f;	
				} else if (strstr(token,"capsule"))
				{
					flags &= ~(mesh_approx_cylinder | mesh_approx_box | mesh_approx_sphere);
					flags |= mesh_approx_capsule;
					tol = 1.0f;	
				}	else if (strstr(token,"notaprim"))
				{
					flags &= ~(mesh_approx_capsule | mesh_approx_cylinder | mesh_approx_box | mesh_approx_sphere);
					tol = 0.0f;
					break;
				}

				token = props.Tokenize("\n",curPos);
			};
		}

		int nMinTrisPerNode = 2;
		int nMaxTrisPerNode = 4;
		Vec3 size = ptmax - ptmin;
		if (indices.size() < 600 && max(max(size.x,size.y),size.z) > 6) // make more dense OBBs for large (wrt terrain grid) objects
			nMinTrisPerNode = nMaxTrisPerNode = 1;

		//////////////////////////////////////////////////////////////////////////
		// Create physical mesh.
		//////////////////////////////////////////////////////////////////////////
		IGeometry *pGeom = pGeoman->CreateMesh(
			pVertices,
			&indices[0],
			&faceMaterials[0],
			pForeignIndices,
			indices.size()/3,
			flags, tol, nMinTrisPerNode,nMaxTrisPerNode, 2.5f );

		if(pGeom)
		{
			if (pGeom->GetErrorCount())
			{
				if ( bPhysicalizeProxy )
				{
					RCLog("%d error(s) in physics proxy (%s), one or more physics proxies are not manifold.", pGeom->GetErrorCount(), pNodeCGF->name); 
				}
				else
				{
					RCLog("%d artifacts in physicalised render geometry (%s), one or more meshes are not manifold, try converting the physicalised render geometry to a manifold physics proxy if you experience problems in game.", pGeom->GetErrorCount(), pNodeCGF->name); 
				}
			}

			CMemStream stm(false);
			phys_geometry *pPhysGeom = pGeoman->RegisterGeometry( pGeom,faceMaterials[0] );
			pGeoman->SavePhysGeometry( stm,pPhysGeom );

			// Add physicalized data to the node.
			pNodeCGF->physicalGeomData[nGeomType-PHYS_GEOM_TYPE_DEFAULT].resize(stm.GetUsedSize());
			memcpy( &pNodeCGF->physicalGeomData[nGeomType-PHYS_GEOM_TYPE_DEFAULT][0],stm.GetBuf(),stm.GetUsedSize() );

			if (flags & mesh_full_serialization)
			{
				// We saved complete physics mesh.
				pNodeCGF->nPhysicalizeFlags |= CNodeCGF::ePhysicsalizeFlag_MeshNotNeeded;
			}

			pGeoman->UnregisterGeometry(pPhysGeom);
			pGeom->Release();
		}
	}

	// Free temporary verts array.
	if (pVertices != mesh.m_pPositions)
		delete [] pVertices;

	return ePR_Ok;
}

namespace {

	int SaveStlSurface(const char *fname, Vec3 *pt,uint16 *pidx,int nTris)
	{
		int i,j;
		Vec3 n;
		FILE *f = fopen(fname,"wt");
		if (!f)
			return 0;

		fprintf(f, "solid tmpobj\n");
		for(i=0;i<nTris;i++) {
			n = (pt[pidx[i*3+1]]-pt[pidx[i*3]] ^ pt[pidx[i*3+2]]-pt[pidx[i*3]]).normalized();
			fprintf(f, "  facet normal %.8g %.8g %.8g\n", n.x,n.y,n.z);
			fprintf(f, "    outer loop\n");
			for(j=0;j<3;j++)
				fprintf(f, "      vertex %.8g %.8g %.8g\n", pt[pidx[i*3+j]].x,pt[pidx[i*3+j]].y,pt[pidx[i*3+j]].z);
			fprintf(f, "    endloop\n");
			fprintf(f, "  endfacet\n");
		}
		fprintf(f, "endsolid tmpobj\n");
		fclose(f);

		return 1;
	}

	int LoadNetgenTetrahedrization(const char *fname, Vec3 *&pVtx,int &nVtx, int *&pTets,int &nTets)
	{
		int i,j,nj;
		FILE *f = fopen(fname,"rt");
		char buf[65536],*pbuf=buf,*str;

		pVtx=0; pTets=0;
		while ((!pVtx || !pTets) && (str=fgets(pbuf,65536, f))) {
			if (!strncmp(str,"volumeelements",14)) {
				fscanf(f, "%d",&nTets); pTets = new int[nTets*4];
				for(i=0;i<nTets;i++) {
					fscanf(f, " %d %d %d %d %d %d", &j,&nj, pTets+i*4,pTets+i*4+1,pTets+i*4+2,pTets+i*4+3);
					for(j=0;j<nj;j++) pTets[i*4+j]--;
				}
			}     else if (!strncmp(str,"points",6)) {
				fscanf(f, "%d",&nVtx); pVtx = new Vec3[nVtx];
				for(i=0;i<nVtx;i++) 
					fscanf(f, " %f %f %f", &pVtx[i].x,&pVtx[i].y,&pVtx[i].z);
			}
		}
		return pVtx && pTets;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicsInterface::ProcessBreakablePhysics( CContentCGF *pCompiledCGF,CContentCGF *pSrcCGF )
{
	if(!pSrcCGF)
		return;
	
	CPhysicalizeInfoCGF* pPi = pSrcCGF->GetPhysiclizeInfo();

	if(pPi->nMode==-1 || pPi->nGranularity==-1)
		return;

	char path[1024];
	GetModuleFileName(GetModuleHandle(NULL),path, 1024);
	char * ch = strrchr(path, '\\');
	if(!ch)
		return;
	strcpy(ch, "\\netgen\\");

	char filepath[4096];
	sprintf(filepath, "%sng.exe", path);

	char stlFile[1024];
	sprintf(stlFile, "%stempin.stl", path);

	char volFile[1024];
	sprintf(volFile, "%stempout.vol", path);

	{
		// Remove read only attribute of ng.ini
		char ngIni[1024];
		sprintf(ngIni, "%sng.ini", path);
		SetFileAttributes( ngIni,FILE_ATTRIBUTE_ARCHIVE );
	}

	CNodeCGF * pNode = 0;
	int i;
	for(i=0; i<pSrcCGF->GetNodeCount(); i++)
	{
		pNode = pSrcCGF->GetNode(i);
		if(!strcmp(pNode->name, "$tet"))
			break;
		pNode = 0;
	}
	if(!pNode)
		return;
	CMesh * pMesh = pNode->pMesh;
	if(!pMesh)
		return;

	Vec3 *pVtx = new Vec3[pMesh->m_numVertices];
	for(i=0; i<pMesh->m_numVertices; i++)
		pVtx[i] = pNode->worldTM*pMesh->m_pPositions[i];

	if(!pMesh->m_pIndices)
	{
		uint16 * pInds = new uint16[pMesh->m_numFaces * 3];

		for(i=0; i<pMesh->m_numFaces; i++)
		{
			pInds[i*3+0] = pMesh->m_pFaces[i].v[0];
			pInds[i*3+1] = pMesh->m_pFaces[i].v[1];
			pInds[i*3+2] = pMesh->m_pFaces[i].v[2];
		}

		SaveStlSurface(stlFile, pVtx, pInds, pMesh->m_numFaces);

		delete[] pInds;
	}
	else
		SaveStlSurface(stlFile, pVtx, pMesh->m_pIndices, pMesh->m_nIndexCount/3);
	delete[] pVtx;

	const char * pGr = "";

	switch(pPi->nGranularity)
	{
	case 0:
		pGr = "-verycoarse";
		break;
	case 1:
		pGr = "-coarse";
		break;
	case 2:
		pGr = "-moderate";
		break;
	case 3:
		pGr = "-fine";
		break;
	case 4:
		pGr = "-veryfine";
		break;
	}

	char cmd[4096];
	sprintf(cmd, "%s -geofile=tempin.stl -meshfile=tempout.vol -batchmode %s", filepath, pGr);

	SetEnvironmentVariable("TCL_LIBRARY", ".\\tcl8.3");
	SetEnvironmentVariable("TIX_LIBRARY", ".\\tix8.2");

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset( &si,0,sizeof(si) );
	si.cb = sizeof(si);
	if (!CreateProcess( NULL,cmd,NULL,NULL,FALSE,0,NULL,path,&si,&pi ))
	{
		RCLogError("Error executing netgen. To solve this you can Either:\n\n\tensure that the NETGEN executables are present in the RC folder.\n\tRemove the asset\n\tRemove the breakable flag from the asset.", pNode->name);
		return;
	}
	// Wait until child process exits.
	WaitForSingleObject( pi.hProcess, INFINITE );

	DWORD lpExitCode = 0;
	GetExitCodeProcess( pi.hProcess,&lpExitCode );
	// Close process and thread handles.
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	if (lpExitCode != EXIT_SUCCESS)
		return;

	CPhysicalizeInfoCGF* pConPi = pCompiledCGF->GetPhysiclizeInfo();

	LoadNetgenTetrahedrization(volFile, pConPi->pRetVtx, pConPi->nRetVtx, pConPi->pRetTets, pConPi->nRetTets);


	remove(stlFile);
	remove(volFile);
}

//////////////////////////////////////////////////////////////////////////
bool CPhysicsInterface::DeletePhysicalProxySubsets( CNodeCGF *pNodeCGF, bool const bHasFaceMap )
{
	CMesh &mesh = *pNodeCGF->pMesh;

	const bool bPreValidation = mesh.Validate(0);

	// If we have a non-empty physical proxy subset and face mapping
	// then we cannot perform deleting because it will 
	// invalidate the face mapping stored in a physics chunk.
	for (size_t i = 0; i < mesh.m_subsets.size(); ++i)
	{
		const SMeshSubset& subset = mesh.m_subsets[i];

		const bool bPhysicalProxy = 
			(subset.nPhysicalizeType != PHYS_GEOM_TYPE_NONE) && 
			(subset.nPhysicalizeType != PHYS_GEOM_TYPE_DEFAULT);

		const bool bEmpty = ((subset.nNumIndices <= 0) && (subset.nNumVerts <= 0));

		if (bPhysicalProxy && bHasFaceMap && !bEmpty)
		{
			return false;
		}
	}

	// Sometimes subsets are empty or broken (due to errors in the past 
	// versions of the RC), so we delete them before further processing.
	for (size_t i = 0; i < mesh.m_subsets.size(); ++i)
	{
		const SMeshSubset& subset = mesh.m_subsets[i];

		const bool bEmpty = ((subset.nNumIndices <= 0) && (subset.nNumVerts <= 0));
		const bool bBroken = ((subset.nNumIndices <= 0) != (subset.nNumVerts <= 0));

		if (bEmpty || bBroken)
		{
			mesh.m_subsets.erase(i);
			--i;
		}
	}

	// Check if we still have physical proxies to delete.
	{
		bool bHasPhysicalProxy = false;
		
		for (size_t i = 0; i < mesh.m_subsets.size(); ++i)
		{
			const SMeshSubset& subset = mesh.m_subsets[i];
	
			const bool bPhysicalProxy = 
				(subset.nPhysicalizeType != PHYS_GEOM_TYPE_NONE) && 
				(subset.nPhysicalizeType != PHYS_GEOM_TYPE_DEFAULT);

			if (bPhysicalProxy)
			{
				bHasPhysicalProxy = true;
				break;
			}
		}

		if (!bHasPhysicalProxy)
		{
			return true;
		}

		if (mesh.m_numFaces > 0)
		{
			RCLogError("Uncompiled mesh passed to DeletePhysicalProxySubsets(). Contact an RC programmer.");
			return false;		
		}
	}

	// Remove physical proxies
	for (int i = (int)mesh.m_subsets.size()-1; i >= 0; --i)
	{
		SMeshSubset& subset = mesh.m_subsets[i];

		const bool bPhysicalProxy = 
			(subset.nPhysicalizeType != PHYS_GEOM_TYPE_NONE) && 
			(subset.nPhysicalizeType != PHYS_GEOM_TYPE_DEFAULT);

		if (bPhysicalProxy)
		{
			const int nFirstIndexId = subset.nFirstIndexId;
			const int nNumIndices = subset.nNumIndices;
			const int nFirstVertexId = subset.nFirstVertId;
			const int nNumVertices = subset.nNumVerts;

			// Collapse indices.
			mesh.RemoveRangeFromStream( CMesh::INDICES, nFirstIndexId, nNumIndices );

			// Collapse vertices.
			mesh.RemoveRangeFromStream( CMesh::POSITIONS,        nFirstVertexId, nNumVertices );
			mesh.RemoveRangeFromStream( CMesh::NORMALS,          nFirstVertexId, nNumVertices );
			mesh.RemoveRangeFromStream( CMesh::TEXCOORDS,        nFirstVertexId, nNumVertices );
			mesh.RemoveRangeFromStream( CMesh::COLORS_0,         nFirstVertexId, nNumVertices );
			mesh.RemoveRangeFromStream( CMesh::COLORS_1,         nFirstVertexId, nNumVertices );
			mesh.RemoveRangeFromStream( CMesh::TANGENTS,         nFirstVertexId, nNumVertices );
			mesh.RemoveRangeFromStream( CMesh::SHCOEFFS,         nFirstVertexId, nNumVertices );
			mesh.RemoveRangeFromStream( CMesh::SHAPEDEFORMATION, nFirstVertexId, nNumVertices );
			mesh.RemoveRangeFromStream( CMesh::BONEMAPPING,      nFirstVertexId, nNumVertices );
			mesh.RemoveRangeFromStream( CMesh::VERT_MATS,        nFirstVertexId, nNumVertices );
			mesh.RemoveRangeFromStream( CMesh::QTANGENTS,        nFirstVertexId, nNumVertices );

			// Erase subset.
			mesh.m_subsets.erase(i);
			--i;

			// Fix references to vertices and indices in remaining subsets.
			for (size_t j = 0; j < mesh.m_subsets.size(); ++j)
			{
				SMeshSubset& subset = mesh.m_subsets[j];

				if (subset.nFirstVertId >= nFirstVertexId)
				{
					subset.nFirstVertId -= nNumVertices;
				}

				if (subset.nFirstIndexId >= nFirstIndexId)
				{
					subset.nFirstIndexId -= nNumIndices;
				}
			}

			// Fix indices.
			for (int j = 0; j < mesh.m_nIndexCount; ++j)
			{
				if (mesh.m_pIndices[j] >= nFirstVertexId)
				{
					mesh.m_pIndices[j] -= nNumVertices;					
				}
			}
		}
	}

	{
		const char* err = "unknown problem";
		const bool bPostValidation = mesh.Validate(&err);

		if (bPreValidation && !bPostValidation)
		{
			RCLogError("Mesh is damaged by DeletePhysicalProxySubsets(): '%s'. Contact an RC programmer.", err);
			return false;
		}
	}

	// Check if mesh is now empty so the whole node is just a physics proxy.
	bool bEmptyMesh = true;
	for (size_t i = 0; i < mesh.m_subsets.size(); ++i)
	{
		const SMeshSubset& subset = mesh.m_subsets[i];
		if (subset.nNumIndices > 0 && subset.nNumVerts > 0)
		{
			bEmptyMesh = false;
		}
	}

	if (bEmptyMesh)
	{
		// Force this node to be physics proxy.
		pNodeCGF->bPhysicsProxy = true;
		pNodeCGF->type = CNodeCGF::NODE_HELPER;
		pNodeCGF->helperType = HP_GEOMETRY;
		
		string newName = string("$physics_proxy_") + pNodeCGF->name;
		strncpy_s( pNodeCGF->name,newName.c_str(),_TRUNCATE );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CPhysicsInterface::RephysicalizeNode( CNodeCGF *pNode,CContentCGF *pCGF )
{
	if (pNode->nPhysicalizeFlags & CNodeCGF::ePhysicsalizeFlag_MeshNotNeeded)
	{
		// Not needed, this is fully physicalized mesh.
		return;
	}

	bool bHavePhysicsData = false;
	for (int nPhysGeomType = 0; nPhysGeomType < 4; nPhysGeomType++)
	{
		if (!pNode->physicalGeomData[nPhysGeomType].empty())
		{
			bHavePhysicsData = true;
			break;
		}
	}
	if (!bHavePhysicsData)
		return;

	std::vector<char> faceMaterials;
	if (pNode->pMesh)
	{
		CMesh &mesh = *pNode->pMesh;
		// Fill faces material array.
		faceMaterials.resize( mesh.GetIndexCount()/3 );
		if (!faceMaterials.empty())
		{
			for (int m = 0; m < mesh.GetSubSetCount(); m++)
			{
				char *pFaceMats = &faceMaterials[0];
				SMeshSubset &subset = mesh.m_subsets[m];
				for (int nFaceIndex = subset.nFirstIndexId/3, last = subset.nFirstIndexId/3 + subset.nNumIndices/3; nFaceIndex < last; nFaceIndex++)
				{
					pFaceMats[nFaceIndex] = subset.nMatID;
				}
			}
		}
	}

	for (int nPhysGeomType = 0; nPhysGeomType < 4; nPhysGeomType++)
	{
		DynArray<char> &physData = pNode->physicalGeomData[nPhysGeomType];
		if (!physData.empty())
		{
			char *pFaceMats = 0;
			if (!faceMaterials.empty())
				pFaceMats = &faceMaterials[0];

			phys_geometry *pPhysGeom = 0;

			{
				// We have physical geometry for this node.
				CMemStream stmIn( &physData.front(), physData.size(), true );
				if (pNode->pMesh)
				{
					CMesh &mesh = *pNode->pMesh;
					// Load using mesh information.
					pPhysGeom = GetPhysicalWorld()->GetGeomManager()->LoadPhysGeometry(stmIn,mesh.m_pPositions,mesh.m_pIndices,pFaceMats);
				}
				else
				{
					// Load only using physics saved data.
					pPhysGeom = GetPhysicalWorld()->GetGeomManager()->LoadPhysGeometry(stmIn,0,0,0);
				}
			}

			if (!pPhysGeom)
			{
				RCLogError( "Bad physical data in CGF node, Physicalization Failed, (%s)" );
				return;
			}

			if (pPhysGeom->pGeom->GetType() == GEOM_TRIMESH)
			{
				mesh_data *pmd = (mesh_data*)pPhysGeom->pGeom->GetData();
				if (pmd->flags & mesh_full_serialization)
				{
					// Already fully serialized.
					GetPhysicalWorld()->GetGeomManager()->UnregisterGeometry(pPhysGeom);
					return;
				}

				pmd->flags |= mesh_full_serialization;

				// Save it again, back to the node.
				CMemStream stm(false);
				GetPhysicalWorld()->GetGeomManager()->SavePhysGeometry( stm,pPhysGeom );

				int nGeomType = nPhysGeomType;
				// Add physicalized data to the node.
				physData.resize(stm.GetUsedSize());
				memcpy( &(physData[0]),stm.GetBuf(),stm.GetUsedSize() );
				
				// We saved complete physics mesh.
				pNode->nPhysicalizeFlags |= CNodeCGF::ePhysicsalizeFlag_MeshNotNeeded;
			}

			GetPhysicalWorld()->GetGeomManager()->UnregisterGeometry(pPhysGeom);
		}
	}
}
