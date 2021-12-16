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
#include "ConvertContext.h"
#include "iconfig.h"
#include "StatCGFCompiler.h"

#include "ChunkFileHelpers.h"
#include "CGF\CGFLoader.h"
#include "CGF\CGFSaver.h"
#include "CryVersion.h"
#include "StaticObjectCompiler.h"
#include "StatCGFPhysicalize.h"

//////////////////////////////////////////////////////////////////////////
CStatCGFCompiler::CStatCGFCompiler()
{
	m_pPhysicsInterface = NULL;
	//MessageBox(NULL, "Pres OK baton ->", "Ok?", MB_OK);

	m_refCount = 1;
}

//////////////////////////////////////////////////////////////////////////
CStatCGFCompiler::~CStatCGFCompiler()
{
	if (m_pPhysicsInterface)
		delete m_pPhysicsInterface;
}

//////////////////////////////////////////////////////////////////////////
void CStatCGFCompiler::ConstructAndSetOutputFile( ConvertContext &cc )
{
	cc.SetOutputFile(cc.sourceFileFinal);
}

//////////////////////////////////////////////////////////////////////////
void CStatCGFCompiler::GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const
{
	string const filename(cc.getOutputPath());

	if (filenameBuffer && (filename.length() < bufferSize))
	{
		strcpy(filenameBuffer, filename.c_str());		
	}
}

//////////////////////////////////////////////////////////////////////////
int CStatCGFCompiler::GetNumPlatforms() const
{
	return 3;
}

//////////////////////////////////////////////////////////////////////////
void CStatCGFCompiler::Release()
{
	if (--m_refCount <= 0)
		delete this;
}

//////////////////////////////////////////////////////////////////////////
ICompiler* CStatCGFCompiler::CreateCompiler()
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
bool CStatCGFCompiler::SupportsMultithreading() const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
EPlatform CStatCGFCompiler::GetPlatform( int index ) const
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

DWORD CStatCGFCompiler::GetTimestamp() const
{
	return GetTimestampForLoadedLibrary(g_hInst);
}


static string getTextMatrix( const Matrix34& M )
{
	string text = "";

	char buff[200];

	sprintf(buff,  "axisX(%g %g %g)", M.m00,M.m10,M.m20);
	text += buff;
	sprintf(buff, " axisY(%g %g %g)", M.m01,M.m11,M.m21);
	text += buff;
	sprintf(buff, " axisZ(%g %g %g)", M.m02,M.m12,M.m22);
	text += buff;
	sprintf(buff, " trans(%g %g %g)", M.m03,M.m13,M.m23);
	text += buff;

	return text;
}

static const char* getTextNodeType( CNodeCGF::ENodeType const type )
{
	const char* text;

	switch (type)
	{
	case CNodeCGF::NODE_HELPER:
		text = "NODE_HELPER";
		break;
	case CNodeCGF::NODE_LIGHT:
		text = "NODE_LIGHT";
		break;
	case CNodeCGF::NODE_MESH:
		text = "NODE_MESH";
		break;
	default:
		text = "***UNKNOWN***";
		break;
	}

	return text;
}

static const char* getTextHelperType( HelperTypes const type )
{
	const char* text;

	switch (type)
	{
	case HP_POINT:
		text = "HP_POINT";
		break;
	case HP_DUMMY:
		text = "HP_DUMMY";
		break;
	case HP_XREF:
		text = "HP_XREF";
		break;
	case HP_CAMERA:
		text = "HP_CAMERA";
		break;
	case HP_GEOMETRY:
		text = "HP_GEOMETRY";
		break;
	default:
		text = "*** UNKNOWN ***";
		break;
	}

	return text;
}

static string getTextPhysicalizeType( int const type )
{
	string text;

	switch (type)
	{
	case PHYS_GEOM_TYPE_NONE:
		text += "PHYS_GEOM_TYPE_NONE";
		break;
	case PHYS_GEOM_TYPE_DEFAULT:
		text += "PHYS_GEOM_TYPE_DEFAULT";
		break;
	case PHYS_GEOM_TYPE_NO_COLLIDE:
		text += "PHYS_GEOM_TYPE_NO_COLLIDE";
		break;
	case PHYS_GEOM_TYPE_OBSTRUCT:
		text += "PHYS_GEOM_TYPE_OBSTRUCT";
		break;
	case PHYS_GEOM_TYPE_DEFAULT_PROXY:
		text += "PHYS_GEOM_TYPE_DEFAULT_PROXY";
		break;
	default:
		{
			text += "*** UNKNOWN ***";
			char buff[50];
			sprintf(buff, " (0x%x)", type);
			text += buff;
		}
		break;
	}

	return text;
}

static string getTextPhysicalizeFlags( int const flags )
{
	string text;

	switch (flags)
	{
	case 0:
		text += "none";
		break;
	case CNodeCGF::ePhysicsalizeFlag_MeshNotNeeded:
		text += "MeshNotNeeded";
		break;
	default:
		{
			text += "*** UNKNOWN ***";
			char buff[50];
			sprintf(buff, " (0x%x)", flags);
			text += buff;
		}
		break;
	}

	return text;
}

static string getTextMaterialFlags( int nFlags )
{
	string text = "{";

	if ( nFlags & MTL_NAME_CHUNK_DESC::FLAG_MULTI_MATERIAL )
	{
		nFlags &= ~MTL_NAME_CHUNK_DESC::FLAG_MULTI_MATERIAL;
		text += " MULTI_MATERIAL";		
	}

	if ( nFlags & MTL_NAME_CHUNK_DESC::FLAG_SUB_MATERIAL )
	{
		nFlags &= ~MTL_NAME_CHUNK_DESC::FLAG_SUB_MATERIAL;
		text += " SUB_MATERIAL";
	}

	if ( nFlags & MTL_NAME_CHUNK_DESC::FLAG_SH_COEFFS )
	{
		nFlags &= ~MTL_NAME_CHUNK_DESC::FLAG_SH_COEFFS;
		text += " SH_COEFFS";
	}

	if ( nFlags & MTL_NAME_CHUNK_DESC::FLAG_SH_2SIDED )
	{
		nFlags &= ~MTL_NAME_CHUNK_DESC::FLAG_SH_2SIDED;
		text += " SH_2SIDED";
	}

	if ( nFlags & MTL_NAME_CHUNK_DESC::FLAG_SH_AMBIENT )
	{
		nFlags &= ~MTL_NAME_CHUNK_DESC::FLAG_SH_AMBIENT;
		text += " SH_AMBIENT";
	}

	if ( nFlags )
	{
		text += " ***UNKNOWN***";
	}

	text += " }";

	return text;
};


static bool debugDumpCGF(const char* a_filename)
{
	RCLog( "Dumping geometry file %s...", a_filename );

	class Listener : public ILoaderCGFListener
	{
	public:
		Listener() {}
		virtual void Warning( const char *format ) {RCLogWarning("%s", format);}
		virtual void Error( const char *format ) {RCLogError("%s", format);}
	};
	CChunkFile chunkFile;
	Listener listener;
	CLoaderCGF cgfLoader;
	CContentCGF *pCGF = cgfLoader.LoadCGF( a_filename, chunkFile, &listener );

	if (!pCGF)
	{
		RCLogError( "Dump: Failed to load geometry file %s - %s",a_filename,cgfLoader.GetLastError() );
		return false;
	}

	if (pCGF->GetConsoleFormat()) 
	{
		RCLogError( "Dump: Cannot dump geometry file %s because it's in console format.",a_filename );
		delete pCGF;
		return false;
	}

	string dumpFilename = a_filename;
	dumpFilename += ".dump";

	FILE* f = fopen(dumpFilename.c_str(), "wt");

	if (f == 0)
	{
		RCLogError( "Dump: Cannot create dump file %s.", dumpFilename.c_str() );
		delete pCGF;
		return false;
	}

	
	fprintf(f, "<<< Dump of '%s' >>>\n", a_filename);
	fprintf(f, "\n");

	fprintf(f, "---------------------------------------------\n");
	fprintf(f, "material#: %d\n", pCGF->GetMaterialCount());
	fprintf(f, "\n");

	std::vector<CMesh*> meshes;

	for (int materialIdx=0; materialIdx<pCGF->GetMaterialCount(); ++materialIdx)
	{
		fprintf(f, "material[%d]:\n", materialIdx);
		const CMaterialCGF* pMat = pCGF->GetMaterial(materialIdx);

		if (pMat == 0)
		{
			fprintf(f, "\t" "***NULL MATERIAL***");
			continue;
		}

		fprintf(f, "\t" "name: '%s'\n", pMat->name);
		fprintf(f, "\t" "nFlags: %s\n", getTextMaterialFlags(pMat->nFlags).c_str());
		fprintf(f, "\t" "nPhysicalizeType: %s\n", getTextPhysicalizeType(pMat->nPhysicalizeType).c_str() );
		fprintf(f, "\t" "bOldMaterial: %s\n", (pMat->bOldMaterial ? "true" : "false"));
		fprintf(f, "\t" "shOpacity: %.g\n", pMat->shOpacity);
		fprintf(f, "\t" "oldMaterialEntity: %s\n", (pMat->pMatEntity ? "(some pointer)" : "NULL"));

		fprintf(f, "\t" "subMaterials[]: size=%d, capacity=%d\n", pMat->subMaterials.size(), pMat->subMaterials.capacity());
		for (int i=0; i<pMat->subMaterials.size(); ++i)
		{
			CMaterialCGF* const pSubMat = pMat->subMaterials[i];

			fprintf(f, "\t\t" "subMaterials[%d]: ", i);
			if (pSubMat == 0)
			{
				fprintf(f, "NULL");
			}
			else
			{
				// Trying to find pSubMat in our materials
				bool found = false;
				for (int j=0; j<pCGF->GetMaterialCount(); ++j)
				{
					if (pSubMat == pCGF->GetMaterial(j))
					{
						found = true;
						fprintf(f, "material[%d] '%s'", j, pSubMat->name);
						break;
					}
				}
				if (!found)
				{
					fprintf(f, "*** not in material[] *** '%s'", pSubMat->name);
				}
			}
			fprintf(f, "\n");
		}

		fprintf(f, "\t" "internals:\n");
		fprintf(f, "\t\t" "nChunkId: %d\n", pMat->nChunkId);

		fprintf(f, "\n");
	}
	fprintf(f, "---------------------------------------------\n");

	{
		CMaterialCGF* pMat = pCGF->GetCommonMaterial();

		fprintf(f, "\t" "pCommonMaterial: ");
		if (pMat == 0)
		{
			fprintf(f, "NULL");
		}
		else
		{
			// Trying to find commonmaterial in our materials
			bool found = false;
			for (int j=0; j<pCGF->GetMaterialCount(); ++j)
			{
				if (pMat == pCGF->GetMaterial(j))
				{
					found = true;
					fprintf(f, "material[%d] '%s'", j, pMat->name);
					break;
				}
			}
			if (!found)
			{
				fprintf(f, "*** not in material[] *** '%s'", pMat->name);
			}
		}
		fprintf(f, "\n");
	}

	{
		CPhysicalizeInfoCGF* pPhys = pCGF->GetPhysiclizeInfo();
		fprintf(f, "\t" "PhysicalizeInfo: (not printed yet)\n");
	}

	{
		CExportInfoCGF* pExport = pCGF->GetExportInfo();
		fprintf(f, "\t" "ExportInfo: (not printed yet)\n");
	}

	{
		CSkinningInfo* pSkin = pCGF->GetSkinningInfo();
		fprintf(f, "\t" "SkinningInfo: (not printed yet)\n");
	}

	{
		SFoliageInfoCGF* pSkin = pCGF->GetFoliageInfo();
		fprintf(f, "\t" "FoliageInfo: (not printed yet)\n");
	}

	fprintf(f, "---------------------------------------------\n");

	fprintf(f, "node#: %d\n", pCGF->GetNodeCount());
	fprintf(f, "\n");

	for (int nodeIdx=0; nodeIdx<pCGF->GetNodeCount(); ++nodeIdx)
	{
		fprintf(f, "node[%d]:\n", nodeIdx);
		const CNodeCGF* pNode = pCGF->GetNode(nodeIdx);

		if (pNode == 0)
		{
			fprintf(f, "\t" "***NULL NODE***");
			continue;
		}

		fprintf(f, "\t" "type: %s\n", getTextNodeType(pNode->type));
		fprintf(f, "\t" "name: '%s'\n", pNode->name);
		fprintf(f, "\t" "properties: '%s'\n", pNode->properties.c_str());
		fprintf(f, "\t" "localTM: %s\n", getTextMatrix(pNode->localTM).c_str());
		fprintf(f, "\t" "worldTM: %s\n", getTextMatrix(pNode->worldTM).c_str());
		fprintf(f, "\t" "pos: %g %g %g\n", pNode->pos.x, pNode->pos.y, pNode->pos.z);
		fprintf(f, "\t" "rot: %g %g %g %g\n", pNode->rot.v.x, pNode->rot.v.y, pNode->rot.v.z, pNode->rot.w);
		fprintf(f, "\t" "scl: %g %g %g\n", pNode->scl.x, pNode->scl.y, pNode->scl.z);

		fprintf(f, "\t" "pParent: ");
		if (pNode->pParent == 0)
		{
			fprintf(f, "NULL");
		}
		else
		{
			// Trying to find pParent node in our nodes
			bool found = false;
			for (int i=0; i<pCGF->GetNodeCount(); ++i)
			{
				if (pNode->pParent == pCGF->GetNode(i))
				{
					found = true;
					fprintf(f, "node[%d] '%s'", i, pNode->pParent->name);
					break;
				}
			}
			if (!found)
			{
				fprintf(f, "*** not in node[] *** '%s'", pNode->pParent->name);
			}
		}
		fprintf(f, "\n");

		fprintf(f, "\t" "pSharedMesh: ");
		if (pNode->pSharedMesh == 0)
		{
			fprintf(f, "NULL");
		}
		else
		{
			// Trying to find pSharedMesh node in our nodes
			bool found = false;
			for (int i=0; i<pCGF->GetNodeCount(); ++i)
			{
				if (pNode->pSharedMesh == pCGF->GetNode(i))
				{
					found = true;
					fprintf(f, "node[%d] '%s'", i, pNode->pSharedMesh->name);
					break;
				}
			}
			if (!found)
			{
				fprintf(f, "*** not in node[] *** '%s'", pNode->pSharedMesh->name);
			}
		}
		fprintf(f, "\n");

		//if (pNode->type == CNodeCGF::NODE_MESH)
		{
			fprintf(f, "\t" "pMesh: ");
			CMesh* const pMesh = pNode->pMesh;
			if (pMesh == 0)
			{
				fprintf(f, "NULL");
			}
			else
			{
				unsigned int i;
				for (i=0; i<meshes.size(); ++i)
				{
					if(meshes[i] == pMesh)
					{
						break;
					}
				}
				if (i >= meshes.size())
				{
					meshes.push_back(pMesh);
					i = meshes.size()-1;
				}
				fprintf(f, "\t" "mesh[%d]", i);
			}
			fprintf(f, "\n");
		}

		if (pNode->type == CNodeCGF::NODE_HELPER)
		{
			fprintf(f, "\t" "helperType: %s\n", getTextHelperType(pNode->helperType));
			fprintf(f, "\t" "helperSize: %g %g %g\n", pNode->helperSize.x, pNode->helperSize.y, pNode->helperSize.z);
		}

		fprintf(f, "\t" "pMaterial: " );
		if (pNode->pMaterial == 0)
		{
			fprintf(f, "NULL");
		}
		else
		{
			// Trying to find pMaterial in our materials
			bool found = false;
			for (int i=0; i<pCGF->GetMaterialCount(); ++i)
			{
				if (pNode->pMaterial == pCGF->GetMaterial(i))
				{
					found = true;
					fprintf(f, "material[%d] '%s'", i, pNode->pMaterial->name);
					break;
				}
			}
			if (!found)
			{
				fprintf(f, "*** not in material[] *** '%s'", pNode->pMaterial->name);
			}
		}
		fprintf(f, "\n");

		fprintf(f, "\t" "nPhysicalizeFlags: %s\n", getTextPhysicalizeFlags(pNode->nPhysicalizeFlags).c_str());

		{
			bool bIsEmpty = true;

			for (int i=0; i<4; ++i)
			{
				const bool bSlotIsEmpty = (pNode->physicalGeomData[i].size() == 0) && (pNode->physicalGeomData[i].capacity() == 0);
				if (!bSlotIsEmpty)
				{
					bIsEmpty = false;
					break;
				}
			}

			if (bIsEmpty)
			{
				fprintf(f, "\t" "physicalGeomData[]: empty\n");
			}
			else
			{
				for (int i=0; i<4; ++i)
				{
					const bool bSlotIsEmpty = (pNode->physicalGeomData[i].size() == 0) && (pNode->physicalGeomData[i].capacity() == 0);
					if (!bSlotIsEmpty)
					{
						fprintf(f, "\t" "physicalGeomData[%d]: size=%d, capacity=%d\n", i, pNode->physicalGeomData[i].size(), pNode->physicalGeomData[i].capacity());
					}
				}
			}
		}

		fprintf(f, "\t" "internals:\n");

		fprintf(f, "\t\t" "nChunkId:       %d\n", pNode->nChunkId);
		fprintf(f, "\t\t" "nParentChunkId: %d\n", pNode->nParentChunkId);
		fprintf(f, "\t\t" "nObjectChunkId: %d\n", pNode->nObjectChunkId);
		fprintf(f, "\t\t" "pos_controller_id:    %d\n", pNode->pos_cont_id);
		fprintf(f, "\t\t" "rot_controller_id:    %d\n", pNode->rot_cont_id);
		fprintf(f, "\t\t" "scl_controller_id:    %d\n", pNode->scl_cont_id);

		fprintf(f, "\t" "bIdentityMatrix: %s\n", (pNode->bIdentityMatrix ? "true" : "false"));
		fprintf(f, "\t" "bPhysicsProxy:   %s\n", (pNode->bPhysicsProxy   ? "true" : "false"));
		fprintf(f, "\t" "bHasFaceMap:     %s\n", (pNode->bHasFaceMap     ? "true" : "false"));
		fprintf(f, "\t" "mapFaceToFace0[]: size=%d, capacity=%d\n", pNode->mapFaceToFace0.size(), pNode->mapFaceToFace0.capacity());
		//for (int i=0, count=pNode->mapFaceToFace0.size(); i<count; ++i)
		//{
		//	const int val = pNode->mapFaceToFace0[i];
		//	fprintf(f, "\t\t" "%d", val);
		//}
		fprintf(f, "\n");
	}
	fprintf(f, "---------------------------------------------\n");

	bool hasError = false;

	fprintf(f, "mesh#: %d\n", meshes.size());
	fprintf(f, "\n", meshes.size());

	for (int meshIdx=0; meshIdx<meshes.size(); ++meshIdx)
	{
		fprintf(f, "mesh[%d]:\n", meshIdx);

		CMesh* const pMesh = meshes[meshIdx];

		const char* pErrorDescription = 0;
		bool const bMeshValid = pMesh->Validate(&pErrorDescription);
		if (!bMeshValid)
		{
			hasError = true;
			fprintf(f, "\t\t" "** ERROR **: mesh is invalid (%s)\n", pErrorDescription);
		}

		fprintf(f, "\t" "numFaces:    %d\n", pMesh->m_numFaces);
		fprintf(f, "\t" "numVertices: %d\n", pMesh->m_numVertices);
		fprintf(f, "\t" "nCoorCount:  %d\n", pMesh->m_nCoorCount);
		fprintf(f, "\t" "nIndexCount: %d\n", pMesh->m_nIndexCount);

		fprintf(f, 
			"\t" "bbox: min(%g %g %g) max(%g %g %g)\n", 
			pMesh->m_bbox.min.x, pMesh->m_bbox.min.y, pMesh->m_bbox.min.z,
			pMesh->m_bbox.max.x, pMesh->m_bbox.max.y, pMesh->m_bbox.max.z );
	
		fprintf(f, "\t" "subsets[]: size=%d, capacity=%d\n", pMesh->m_subsets.size(), pMesh->m_subsets.capacity());
		for (int i=0; i<pMesh->m_subsets.size(); ++i)
		{
			SMeshSubset& subset = pMesh->m_subsets[i];

			fprintf(f, "\t" "subset[%d]:\n", i);

			fprintf(f, "\t\t" "center: %g %g %g\n", subset.vCenter.x, subset.vCenter.y, subset.vCenter.z);
			fprintf(f, "\t\t" "radius: %g\n", subset.fRadius);

			fprintf(f, "\t\t" "nFirstIndexId: %d\n", subset.nFirstIndexId);
			fprintf(f, "\t\t" "nNumIndices: %d\n", subset.nNumIndices);

			fprintf(f, "\t\t" "nFirstVertId: %d\n", subset.nFirstVertId);
			fprintf(f, "\t\t" "nNumVerts: %d\n", subset.nNumVerts);

			fprintf(f, "\t\t" "nMatID: %d\n", subset.nMatID);
			fprintf(f, "\t\t" "nMatFlags: 0x%08x\n", subset.nMatFlags);
			fprintf(f, "\t\t" "nPhysicalizeType: %s\n", getTextPhysicalizeType(subset.nPhysicalizeType).c_str());

			fprintf(f, "\t\t" "arrGlobalBonesPerSubset[]: size=%d, capacity=%d\n", subset.m_arrGlobalBonesPerSubset.size(), subset.m_arrGlobalBonesPerSubset.capacity());

			if (!bMeshValid)
			{
				// Check indices
				if (subset.nNumIndices > 0)
				{
					const uint16* const pIndices = pMesh->m_pIndices;
					if (pIndices == 0)
					{
						fprintf(f, "\t\t" "pIndices: NULL   ** ERROR **\n");
						break;
					}
					if (subset.nNumIndices % 3)
					{
						fprintf(f, "\t\t" "nNumIndices is not multiplication of 3   ** ERROR **\n");
						break;
					}
					if (subset.nFirstIndexId < 0)
					{
						fprintf(f, "\t\t" "nFirstIndexId < 0  ** ERROR **\n");
						break;
					}
					if (subset.nFirstIndexId + subset.nNumIndices > pMesh->m_nIndexCount)
					{
						fprintf(f, "\t\t" "nIndices + numIndices > mesh's nIndexCount   ** ERROR **\n");
						break;
					}
					if (subset.nNumVerts <= 0)
					{
						fprintf(f, "\t\t" "nNumVerts <= 0  ** ERROR **\n");
						break;
					}
					if (subset.nFirstVertId < 0)
					{
						fprintf(f, "\t\t" "nFirstVertId < 0  ** ERROR **\n");
						break;
					}
					if (subset.nFirstVertId + subset.nNumVerts > pMesh->m_numVertices)
					{
						fprintf(f, "\t\t" "nFirstVertId + nNumVerts > mesh's numVertices   ** ERROR **\n");
						break;
					}
					for (int i=0; i<subset.nNumIndices; ++i)
					{
						const int vertexIndex = pIndices[subset.nFirstIndexId + i];
						const int nLastVertId = subset.nFirstVertId + subset.nNumVerts - 1;
						if ((vertexIndex < subset.nFirstVertId) || (vertexIndex > nLastVertId))
						{
							fprintf(f, "\t\t" "%ith index refers to vertex %i (outside of [%i;%i])   ** ERROR **\n", i, vertexIndex, subset.nFirstIndexId, nLastVertId);
							break;
						}
					}
				}
			}
		}

		fprintf(f, "\n");
	}

	if (hasError)
	{
		fprintf(f, "** ATTENTION! At least one mesh has fatal errors in geometry. Search for '** ERROR **' above\n");
	}

	fprintf(f, "<<< End of dump of '%s' >>>\n", a_filename);

	fclose(f);
	f = 0;

	delete pCGF;
	pCGF = 0;

	RCLog( "Finished dumping geometry file %s.", a_filename );

	return true;
}


bool debugValidateCGF(CContentCGF *pCGF, const char* a_filename)
{
	RCLog( "Validating geometry file %s...", a_filename );

	if (!pCGF)
	{
		RCLogError( "Validate: Failed to load geometry file %s",a_filename );
		return false;
	}

	if (pCGF->GetConsoleFormat()) 
	{
		RCLogError( "Validate: Cannot validate geometry file %s because it's in console format.",a_filename );
		return false;
	}

	const char* pErrorDescription = 0;
	if (!pCGF->ValidateMeshes(&pErrorDescription)) 
	{
		RCLogError( "Validate: Geometry file %s is damaged (%s).",a_filename, pErrorDescription );
		return false;
	}

	RCLog( "Validate: Geometry file %s is ok.",a_filename );
	return true;
}


#include <conio.h>
//////////////////////////////////////////////////////////////////////////
bool CStatCGFCompiler::Process( ConvertContext &cc )
{
//	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_CHECK_CRT_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_REPORT_FLAG);

	const string sourceFile = cc.getSourcePath();
	const string outputFile = cc.getOutputPath();

	try
	{
		{
			bool dump = false;
			if (cc.config->HasKey("debugdump"))
			{
				if (!cc.config->Get("debugdump",dump))
				{
					// No any value specified. Assume 'true'.
					dump = true;
				}
			}

			if (dump)
			{
				debugDumpCGF(sourceFile.c_str());
				return true;
			}
		}

		if (cc.config->HasKey("RealignChunks"))
		{
			const bool ok = ChunkFileHelpers::RealignChunks(sourceFile, outputFile);
			return ok;
		}

		const bool bStripMeshData = cc.config->HasKey("StripMesh");
		const bool bSplitLods = cc.config->HasKey("SplitLODs");

		/*
		if (bStripMeshData && IsLodFile(sourceFile))
		{
			// Ignore _lod files for striped meshes.
			return true;
		}
		*/

		if (cc.config->HasKey("SkipMissing"))
		{
			// Skip missing source files.
			const DWORD dwFileSpecAttr = GetFileAttributes(sourceFile.c_str());
			if (dwFileSpecAttr == 0xFFFFFFFF)
			{
				// Skip missing file.
				return true;
			}
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
			RCLog( "Failed to load geometry file %s - %s",sourceFile.c_str(),cgfLoader.GetLastError() );
			return false;
		}

		//////////////////////////////////////////////////////////////////////////
		bool bNeedEndianSwap = false, bNeedCompressVertices = false;

		if (cc.platform == ePlatform_PS3 || cc.platform == ePlatform_X360)
			bNeedEndianSwap = true;
		bool bUseQuaternions = false;
		if (!cc.config->Get("qtangents",bUseQuaternions))
			bUseQuaternions = false;

		//////////////////////////////////////////////////////////////////////////
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
				debugValidateCGF(pCGF, sourceFile.c_str());
				delete pCGF;
				return true;
			}
		}

		// Delete Node and Mesh chunks from CGF chunk file.
		DeleteOldChunks( pCGF,chunkFile );

		// Compile contents.
		if (m_pPhysicsInterface == 0)
		{
			m_pPhysicsInterface = new CPhysicsInterface;
		}

		CStaticObjectCompiler statCgfCompiler(m_pPhysicsInterface);
		statCgfCompiler.SetSplitLods(bSplitLods);
		CContentCGF* pCompiledCGF = statCgfCompiler.MakeCompiledCGF(pCGF);

		if (!pCompiledCGF)
		{
			RCLogError( "Failed to process geometry file %s. Try to re-export the file. If it not helps - contact an RC programmer.",sourceFile.c_str() );
			delete pCGF;
			return false;
		}

		// Save modified content to the same chunk file.
		CSaverCGF cgfSaver( outputFile,chunkFile );

		SFileVersion fv = cc.pRC->GetFileVersion();
		pCompiledCGF->GetExportInfo()->rc_version[0] = fv.v[0];
		pCompiledCGF->GetExportInfo()->rc_version[1] = fv.v[1];
		pCompiledCGF->GetExportInfo()->rc_version[2] = fv.v[2];
		pCompiledCGF->GetExportInfo()->rc_version[3] = fv.v[3];
		sprintf( pCompiledCGF->GetExportInfo()->rc_version_string," RCVer:%d.%d ",fv.v[2],fv.v[1] );

		const char* pErrorDescription = 0;
		if(!pCompiledCGF->ValidateMeshes(&pErrorDescription))
		{
			RCLogError( 
				"Failed to process geometry file %s (%s). Try to re-export the file. If it not helps - contact an RC programmer.",
				sourceFile.c_str(),
				pErrorDescription );
			return false;
		}

		if (bStripMeshData)
		{
			cgfSaver.SetMeshDataSaving(false);
		}
    
		cgfSaver.SaveContent(pCompiledCGF,bNeedEndianSwap, bNeedCompressVertices, bUseQuaternions);
		
		// Force remove of the read only flag and after write restore previous file flags.
		SetFileAttributes( outputFile,FILE_ATTRIBUTE_ARCHIVE );

		//if (strstr(outputFile,"_lod") == 0)
		chunkFile.Write( outputFile );

		if (bSplitLods)
		{
			// Save split LODs
			for (int i = 1; i < MAX_NUM_LODS; i++)
			{
				const int nLodNum = i;
				CContentCGF *pLodCgf = statCgfCompiler.m_pLODs[i];
				if (pLodCgf)
				{
					CChunkFile lodChunkFile;

					string lodFilename = outputFile;
					lodFilename = PathHelpers::RemoveExtension(lodFilename);
					lodFilename += "_lod";
					lodFilename += '0' + nLodNum;
					lodFilename = PathHelpers::ReplaceExtension( lodFilename,PathHelpers::FindExtension(outputFile) );

					{
						const char* pErrorDescription = 0;
						if(!pLodCgf->ValidateMeshes(&pErrorDescription))
						{
							RCLogError( 
								"Failed to process geometry file %s (%s). Try to re-export the file. If it not helps - contact an RC programmer.",
								lodFilename.c_str(),
								pErrorDescription );
							return false;
						}
					}

					CSaverCGF cgfSaver( lodFilename,lodChunkFile );
					cgfSaver.SaveContent(pLodCgf,bNeedEndianSwap, bNeedCompressVertices, bUseQuaternions);
					SetFileAttributes( lodFilename,FILE_ATTRIBUTE_ARCHIVE );
					lodChunkFile.Write( lodFilename );
				}
			}
		}

		delete pCGF;
	}
	catch(char*)
	{
		Beep(1000,1000);
		RCLogError("Unexpected failure in processing %s to %s - contact an RC programmer.", sourceFile.c_str(), outputFile.c_str());
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CStatCGFCompiler::DeleteChunkByType(CChunkFile &chunkFile,int nChunkType)
{
	while (true)
	{
		CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(nChunkType);
		if (cd)
			chunkFile.DeleteChunkId( cd->hdr.ChunkID );
		else
			break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CStatCGFCompiler::DeleteOldChunks( CContentCGF *pCGF,CChunkFile &chunkFile )
{
	for (int i = 0; i < pCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pNode = pCGF->GetNode(i);
		if (pNode->nChunkId)
		{
			chunkFile.DeleteChunkId(pNode->nChunkId); // Delete chunk of node.
			// Check if light node, for light nodes we not change light object.
			if (pNode->nObjectChunkId && pNode->type != CNodeCGF::NODE_LIGHT)
				chunkFile.DeleteChunkId(pNode->nObjectChunkId); // Delete chunk of mesh.
			pNode->nObjectChunkId = 0;
		}
	}
	// Delete all material chunks
	DeleteChunkByType(chunkFile,ChunkType_MtlName);
	// Delete all mesh chunks.
	DeleteChunkByType(chunkFile,ChunkType_Mesh);
	// Delete all mesh subsets.
	DeleteChunkByType(chunkFile,ChunkType_MeshSubsets);
	// Delete all data streams.
	DeleteChunkByType(chunkFile,ChunkType_DataStream);
	// Delete all pre-compiled physics data streams.
	DeleteChunkByType(chunkFile,ChunkType_MeshPhysicsData);
	// Delete all mesh chunks.
	DeleteChunkByType(chunkFile,ChunkType_ExportFlags);
	// Delete breakable physics chunk.
	DeleteChunkByType(chunkFile,ChunkType_BreakablePhysics);
	// Delete foliage info.
	DeleteChunkByType(chunkFile,ChunkType_FoliageInfo);
	// Delete facemap.
	DeleteChunkByType(chunkFile,ChunkType_FaceMap);
}

//////////////////////////////////////////////////////////////////////////
bool CStatCGFCompiler::IsLodFile( const string &filename ) const
{
	string file = filename;
	file.MakeLower();
	const char *s = strstr(file.c_str(),"_lod");
	if (s && s[4] >= '0' && s[4] <= '9' && s[5] == '.')
	{
		// This is filename of the lod.
		return true;
	}
	return false;
}