////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   ColladaCompiler.cpp
//  Version:     v1.00
//  Created:     3/4/2006 by Michael Smith
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ColladaCompiler.h"
#include "ColladaLoader.h"
#include "CharacterCompiler.h"
#include "CryVersion.h"
#include "CGF\CGFSaver.h"
#include "CGF\ANMSaver.h"
#include "CGF\CAFSaver.h"
#include "StaticObjectCompiler.h"
#include "StatCGFPhysicalize.h"
#include "cgf/CGFNodeMerger.h"
#include "Iconfig.h"

#include "IXml.h"
#include "IXMLSerializer.h"
#include "Xml/xml.h"

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

void GenerateDefaultUVs(CMesh& mesh);

ColladaCompiler::ColladaCompiler(ICryXML* pCryXML, IPakSystem* pPakSystem)
:	pCryXML(pCryXML),
	pPakSystem(pPakSystem),
	m_refCount(1)
{
	this->pCryXML->AddRef();
	this->pPhysicsInterface = 0;
}

ColladaCompiler::~ColladaCompiler()
{
	delete this->pPhysicsInterface;
	this->pCryXML->Release();
}

void ColladaCompiler::Release()
{
	if (--m_refCount <= 0)
		delete this;
}

class ColladaLoaderListener : public IColladaLoaderListener
{
public:
	ColladaLoaderListener(ConvertContext &cc)
		:	cc(cc)
	{
	}

	virtual void OnColladaLoaderMessage(MessageType type, const char* szMessage)
	{
		switch (type)
		{
			case MESSAGE_INFO:
				RCLog("%s", szMessage);
				break;
			case MESSAGE_WARNING:
				RCLogWarning("%s", szMessage);
				break;
			case MESSAGE_ERROR:
				RCLogError("%s", szMessage);
				break;
		}
	}

private:
	ConvertContext &cc;
};

// -----------------------------------------------------------------------------------------------------------
// Compile collada file to Crytek Geometry File (CGF) --------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------
bool ColladaCompiler::CompileToCGF(ConvertContext &cc, ColladaAuthorTool authorTool, CContentCGF* pCGF, CInternalSkinningInfo* pSkinninginfo, const string& exportFileName)
{
	if (!pCGF)
		return false;

	if (!this->PrepareCGF(pCGF))
		return false;

	// Flip the uvs in all the meshes for compiled format
	for (int i=0; i<pCGF->GetNodeCount(); i++)
	{
		CNodeCGF* node = pCGF->GetNode(i);

		if (!node || !node->pMesh)
			continue;

		CMesh* mesh = node->pMesh;
		if (!mesh->m_pTexCoord)
			continue;

		// flip the 'v' coordinate
		for (int v=0; v<mesh->GetTexCoordsCount(); v++)
			mesh->m_pTexCoord[v].t = 1.0f - mesh->m_pTexCoord[v].t;
	}

	// Make an object to compile the nodes.
	if (this->pPhysicsInterface == 0)
		this->pPhysicsInterface = new CPhysicsInterface;
	CStaticObjectCompiler compiler(this->pPhysicsInterface);

	// Compile the cgf.
	CContentCGF* pCompiledCGF = compiler.MakeCompiledCGF(pCGF);

	if (!pCompiledCGF)
	{
		SAFE_DELETE(pCompiledCGF);
		return false;
	}
	
	// Set export data.
	SFileVersion fv = cc.pRC->GetFileVersion();
	pCompiledCGF->GetExportInfo()->rc_version[0] = fv.v[0];
	pCompiledCGF->GetExportInfo()->rc_version[1] = fv.v[1];
	pCompiledCGF->GetExportInfo()->rc_version[2] = fv.v[2];
	pCompiledCGF->GetExportInfo()->rc_version[3] = fv.v[3];
	sprintf(pCompiledCGF->GetExportInfo()->rc_version_string," RCVer:%d.%d ",fv.v[2],fv.v[1]);
	
	switch (authorTool)
	{
		case TOOL_SOFTIMAGE_XSI:
			pCompiledCGF->GetExportInfo()->bFromColladaXSI = true;
			break;
		case TOOL_AUTODESK_MAX:
			pCompiledCGF->GetExportInfo()->bFromColladaMAX = true;
			break;
	}

	// Save the processed data to a chunkfile.
	CChunkFile chunkFile;
	CSaverCGF cgfSaver(exportFileName, chunkFile);

	cgfSaver.SetContent(pCompiledCGF);

	// Only store 
  bool bNeedEndianSwap = false, bNeedCompressVertices = false, bUseQuaternions = false;

	if (!cc.config->Get("qtangents",bUseQuaternions))
		bUseQuaternions = false;

	cgfSaver.SaveExportFlags();
	if (pSkinninginfo)
		cgfSaver.SaveTiming(pSkinninginfo);
	cgfSaver.SaveMaterials(bNeedEndianSwap);
	cgfSaver.SaveNodes(bNeedEndianSwap, bNeedCompressVertices, bUseQuaternions, pSkinninginfo );
	cgfSaver.SaveBreakablePhysics(bNeedEndianSwap);
	cgfSaver.SaveFoliage();

	// Force remove of the read only flag.
	SetFileAttributes(exportFileName, FILE_ATTRIBUTE_ARCHIVE);
	chunkFile.Write(exportFileName);

	SAFE_DELETE(pCompiledCGF);

	return true;
}

// -----------------------------------------------------------------------------------------------------------
// Compile collada file to Crytek Character File (CHR) -------------------------------------------------------
// This is uncompiled format of CHR! Should run the resource compiler again for this chr file ----------------
// -----------------------------------------------------------------------------------------------------------
bool ColladaCompiler::CompileToCHR(ConvertContext &cc, ColladaAuthorTool authorTool, CContentCGF* pCGF, const string& exportFileName)
{
	if (!pCGF)
		return false;

	CSkinningInfo* pSkinningInfo = pCGF->GetSkinningInfo();

	if (pSkinningInfo->m_arrBonesDesc.size() == 0)
		return false;

	CChunkFile chunkFile;
	CSaverCGF cgfSaver(exportFileName, chunkFile);

	SFileVersion fv = cc.pRC->GetFileVersion();
	pCGF->GetExportInfo()->rc_version[0] = fv.v[0];
	pCGF->GetExportInfo()->rc_version[1] = fv.v[1];
	pCGF->GetExportInfo()->rc_version[2] = fv.v[2];
	pCGF->GetExportInfo()->rc_version[3] = fv.v[3];
	sprintf(pCGF->GetExportInfo()->rc_version_string," RCVer:%d.%d ",fv.v[2],fv.v[1]);

	switch (authorTool)
	{
		case TOOL_SOFTIMAGE_XSI:
			pCGF->GetExportInfo()->bFromColladaXSI = true;
			break;
		case TOOL_AUTODESK_MAX:
			pCGF->GetExportInfo()->bFromColladaMAX = true;
			break;
	}

	cgfSaver.SetContent(pCGF);
	
	// Only store 
	cgfSaver.SaveExportFlags();
	cgfSaver.SaveMaterials(cc.platform == ePlatform_X360 || cc.platform == ePlatform_PS3);
	cgfSaver.SaveUncompiledNodes();				// the nodes of character will be compile later
	cgfSaver.SaveUncompiledMorphTargets();
	
	// get number of bones
	int numBones = pSkinningInfo->m_arrBonesDesc.size();
	
	//---------------------------------------------------------------------
	//---  write bone meshes to chunk
	//---------------------------------------------------------------------
	std::vector<int> boneMeshMap(pSkinningInfo->m_arrPhyBoneMeshes.size());
	for (int phys = 0, physCount = int(pSkinningInfo->m_arrPhyBoneMeshes.size()); phys < physCount; ++phys)
		boneMeshMap[phys] = cgfSaver.SaveBoneMesh(pSkinningInfo->m_arrPhyBoneMeshes[phys]);

	//---------------------------------------------------------------------
	//---  write bones to chunk
	//---------------------------------------------------------------------
	assert(pSkinningInfo->m_arrBoneEntities.size() == pSkinningInfo->m_arrBonesDesc.size());

	DynArray<BONE_ENTITY> tempBoneEntities = pSkinningInfo->m_arrBoneEntities;
	for (int i = 0, count = int(tempBoneEntities.size()); i < count; ++i)
		tempBoneEntities[i].phys.nPhysGeom = (tempBoneEntities[i].phys.nPhysGeom >= 0 ? boneMeshMap[tempBoneEntities[i].phys.nPhysGeom] : -1);
	BONE_ENTITY* pPhysicalBone = &tempBoneEntities[0];
	cgfSaver.SaveBones(pPhysicalBone,numBones,numBones*sizeof(BONE_ENTITY));

	//---------------------------------------------------------------------
	//---  write bone names to chunk
	//---------------------------------------------------------------------
	char* boneNames = new char[65536];
	char* p = boneNames;
	int allChars = 0;
	SBoneInitPosMatrix* boneMatrices = new SBoneInitPosMatrix[numBones];
	for (int BoneID=0; BoneID<numBones; BoneID++)
	{
		int entIdx = 0;
		for (; entIdx<numBones; entIdx++)
			if (BoneID == pSkinningInfo->m_arrBoneEntities[entIdx].BoneID)
				break;
		assert(entIdx < numBones);
		int descIdx = 0;
		for (; descIdx<numBones; descIdx++)
			if (pSkinningInfo->m_arrBonesDesc[descIdx].m_nControllerID == pSkinningInfo->m_arrBoneEntities[entIdx].ControllerID)
				break;
		assert(descIdx < numBones);

		// get bone name --------------
		char* name = pSkinningInfo->m_arrBonesDesc[BoneID].m_arrBoneName;//pSkinningInfo->m_arrBonesDesc[descIdx].m_arrBoneName;

		int len = strlen(name) + 1;

		char *pos = strstr(name, "%");
		if (pos)
		{
			pos[0] = 0;
			len = pos-name+1;
		}
		//for (int i = 0; i < len; i++)
		//{
		//	if (name[i] == '_')
		//		name[i] = ' ';
		//}

		strncpy(p,name,len);
		p += len;
		allChars += len;
		// ----------------------------

		// get bone init matrix -------
		for (int x=0; x<4; x++)
		{
			//TODO: is this OK? (get data from m_DefaultB2W) (and the last rows of matrices, contains 100x scale? - this maybe causes a problem)
//			Vec3 column = pSkinningInfo->m_arrBonesDesc[BoneID].m_DefaultB2W.GetColumn(x);//pSkinningInfo->m_arrBonesDesc[descIdx].m_DefaultB2W.GetColumn(x);

			Matrix34 m = Matrix34::CreateScale(Vec3(100,100,100))*pSkinningInfo->m_arrBonesDesc[BoneID].m_DefaultB2W;
			Vec3 column = m.GetColumn(x);//pSkinningInfo->m_arrBonesDesc[descIdx].m_DefaultB2W.GetColumn(x);
			for (int y=0; y<3; y++)
				boneMatrices[BoneID][x][y] = column[y];
		}
		// ----------------------------
	}
	*p = 0;
	allChars++;
	
	//---------------------------------------------------------------------
	//---  write bone names and initial matrices to chunk (in BoneID order)
	//---------------------------------------------------------------------
	cgfSaver.SaveBoneNames(boneNames,numBones,allChars);
	cgfSaver.SaveBoneInitialMatrices(boneMatrices,numBones,sizeof(SBoneInitPosMatrix)*numBones);
	
	delete boneNames;
	delete boneMatrices;

	// Force remove of the read only flag.
	SetFileAttributes(exportFileName, FILE_ATTRIBUTE_ARCHIVE);
	chunkFile.Write(exportFileName);

	return true;
}

// -----------------------------------------------------------------------------------------------------------
// Compile collada file to Crytek Animation File (ANM) -------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------
bool ColladaCompiler::CompileToANM(ConvertContext &cc, ColladaAuthorTool authorTool, CContentCGF* pCGF, CInternalSkinningInfo* pSkinninginfo, const string& exportFileName)
{
	if (!pCGF || !pSkinninginfo)
		return false;

	// Set export data.
	SFileVersion fv = cc.pRC->GetFileVersion();
	pCGF->GetExportInfo()->rc_version[0] = fv.v[0];
	pCGF->GetExportInfo()->rc_version[1] = fv.v[1];
	pCGF->GetExportInfo()->rc_version[2] = fv.v[2];
	pCGF->GetExportInfo()->rc_version[3] = fv.v[3];
	sprintf(pCGF->GetExportInfo()->rc_version_string," RCVer:%d.%d ",fv.v[2],fv.v[1]);

	switch (authorTool)
	{
		case TOOL_SOFTIMAGE_XSI:
			pCGF->GetExportInfo()->bFromColladaXSI = true;
			break;
		case TOOL_AUTODESK_MAX:
			pCGF->GetExportInfo()->bFromColladaMAX = true;
			break;
	}

	// Save the processed data to a chunkfile.
	CChunkFile chunkFile;	//TODO: set FileType, Version (if needs)
	CSaverANM anmSaver(exportFileName, chunkFile);

	anmSaver.Save(pCGF,pSkinninginfo);

	SetFileAttributes(exportFileName, FILE_ATTRIBUTE_ARCHIVE);
	chunkFile.Write(exportFileName);

	return true;
}

// -----------------------------------------------------------------------------------------------------------
// Compile collada file to Crytek Bone Animation File (CAF) --------------------------------------------------
// -----------------------------------------------------------------------------------------------------------
bool ColladaCompiler::CompileToCAF(ConvertContext &cc, ColladaAuthorTool authorTool, CContentCGF* pCGF, CInternalSkinningInfo* pSkinninginfo, const string& exportFileName)
{
	if (!pCGF || !pSkinninginfo)
		return false;

	// Set export data.
	SFileVersion fv = cc.pRC->GetFileVersion();
	pCGF->GetExportInfo()->rc_version[0] = fv.v[0];
	pCGF->GetExportInfo()->rc_version[1] = fv.v[1];
	pCGF->GetExportInfo()->rc_version[2] = fv.v[2];
	pCGF->GetExportInfo()->rc_version[3] = fv.v[3];
	sprintf(pCGF->GetExportInfo()->rc_version_string," RCVer:%d.%d ",fv.v[2],fv.v[1]);

	switch (authorTool)
	{
		case TOOL_SOFTIMAGE_XSI:
			pCGF->GetExportInfo()->bFromColladaXSI = true;
			break;
		case TOOL_AUTODESK_MAX:
			pCGF->GetExportInfo()->bFromColladaMAX = true;
			break;
	}

	// Save the processed data to a chunkfile.
	CChunkFile chunkFile;	//TODO: set FileType, Version (if needs)
	CSaverCAF cafSaver(exportFileName, chunkFile);

	cafSaver.Save(pCGF,pSkinninginfo);

	SetFileAttributes(exportFileName, FILE_ATTRIBUTE_ARCHIVE);
	chunkFile.Write(exportFileName);

	return true;
}

void ColladaCompiler::WriteMaterialLibrary(sMaterialLibrary& materialLibrary, const string& matFileName)
{
	IXMLSerializer* pSerializer = pCryXML->GetXMLSerializer();

	XmlNodeRef rootNode = pSerializer->CreateNode("Material");
	XmlNodeRef submatNode = pSerializer->CreateNode("SubMaterials");

	rootNode->setAttr("MtlFlags", materialLibrary.flag);
	rootNode->addChild(submatNode);

	for (int i=0;i<materialLibrary.materials.size();i++)
	{
		sMaterial& material = materialLibrary.materials[i];

		Vec3 vecDiffuse(material.diffuse.r,material.diffuse.g,material.diffuse.b);
		Vec3 vecSpecular(material.specular.r,material.specular.g,material.specular.b);
		Vec3 vecEmissive(material.emissive.r,material.emissive.g,material.emissive.b);

		XmlNodeRef matNode = pSerializer->CreateNode("Material");	

		matNode->setAttr("Name", material.name.c_str());
		matNode->setAttr("MtlFlags", material.flag);
		//matNode->setAttr("Shader", material.shader.c_str());		//TODO: select proper shader
		matNode->setAttr("SurfaceType", material.surfacetype.c_str());
		matNode->setAttr("Diffuse", vecDiffuse);
		matNode->setAttr("Specular", vecSpecular);
		matNode->setAttr("Emissive", vecEmissive);
		matNode->setAttr("Shininess", material.shininess);
		matNode->setAttr("Opacity", material.opacity);

		XmlNodeRef texturesNode = pSerializer->CreateNode("Textures");
		matNode->addChild(texturesNode);

		for (int t=0;t<material.textures.size();t++)
		{
			sTexture& texture = material.textures[t];
			
			XmlNodeRef textureNode = pSerializer->CreateNode("Texture");
			textureNode->setAttr("Map", texture.type.c_str());
			textureNode->setAttr("File", texture.filename.c_str());
			
			texturesNode->addChild(textureNode);
		}
		
		submatNode->addChild(matNode);
	}

	pSerializer->Write(rootNode, matFileName.c_str());
}

bool ColladaCompiler::Process(ConvertContext &cc)
{
	string filenameForUptodateCheck;
	{
		char buff[MAX_PATH];
		buff[0] = 0;
		GetFilenameForUpToDateCheck(cc, buff, sizeof(buff));
		filenameForUptodateCheck = buff;
	}

	SetFileAttributes(filenameForUptodateCheck.c_str(), FILE_ATTRIBUTE_ARCHIVE);  // force remove of the read only flag
	remove(filenameForUptodateCheck.c_str());

	string const outputFolder = PathHelpers::GetDirectory(cc.getOutputPath());

	ColladaLoaderListener listener(cc);	
	std::vector<ExportFile> exportFileList;
	std::vector<sMaterialLibrary> materialLibraryList;
	IPakSystem* pPakSystem = (cc.pRC ? cc.pRC->GetPakSystem() : 0);
	if (!ColladaLoader().Load(exportFileList, materialLibraryList, cc.getSourcePath(), this->pCryXML, pPakSystem, &listener))
	{
		return false;
	}

	// Write material libraries ------------------------------------------------------
	{
		bool bCreateMaterialLibrary = false;
		{
			string stringValue;
			if (cc.config->Get("createmtl",stringValue)
				&& (stringValue.empty() || (atoi(stringValue.c_str()) != 0)))
			{
				bCreateMaterialLibrary = true;
			}
		}

		if (bCreateMaterialLibrary)
		{
			for (int i=0;i<materialLibraryList.size();i++)
			{
				sMaterialLibrary& materialLibrary = materialLibraryList[i];
				string const matFileName = PathHelpers::Join(outputFolder, materialLibrary.library.c_str()) + ".mtl";
				WriteMaterialLibrary(materialLibrary, matFileName);
			}
		}
	}

	// Export model and animation track files (CGF,CGA,CHR,ANM,CAF) ------------------
	bool success = true;
	for (int i=0; i<exportFileList.size(); i++)
	{
		ExportFile& exportfile = exportFileList[i];

		uint32 numNodes = exportfile.pCGF->GetNodeCount();
		for (int j=0; j<numNodes; j++)
		{
			CNodeCGF *pNode = exportfile.pCGF->GetNode(j);
			string newNodeName;
			size_t pos = string(pNode->name).find('%');
			if (pos == string::npos || pos > 0)
			{
				newNodeName = string(pNode->name).substr(0,pos);
				strncpy(pNode->name,newNodeName,sizeof(pNode->name));
				pNode->name[sizeof(pNode->name)-1] = 0;
			}
		}

		switch (exportfile.type)
		{
			// Export to CGF
			case EXPORT_CGF:
				{
					string exportFileName = PathHelpers::Join(outputFolder, exportfile.name.c_str());
					exportFileName = PathHelpers::ReplaceExtension(exportFileName,"cgf");
					if (!CompileToCGF(cc,exportfile.authorTool,exportfile.pCGF,NULL,exportFileName))
						success = false;
				}
				break;
			
			// Export to CGA
			case EXPORT_CGA:
				{
					string exportFileName = PathHelpers::Join(outputFolder, exportfile.name.c_str());
					exportFileName = PathHelpers::ReplaceExtension(exportFileName,"cga");
					if (!CompileToCGF(cc,exportfile.authorTool,exportfile.pCGF,exportfile.pCtrlSkinningInfo,exportFileName))
						success = false;
				}
				break;

			// Export to CHR
			case EXPORT_CHR:
				{
					string exportFileName = PathHelpers::Join(outputFolder, exportfile.name.c_str());
					exportFileName = PathHelpers::ReplaceExtension(exportFileName,"chr");
					if (!CompileToCHR(cc,exportfile.authorTool,exportfile.pCGF,exportFileName))
						success = false;
				}
				break;
			
			// Export to ANM
			case EXPORT_ANM:
				{
					string exportFileName = PathHelpers::Join(outputFolder, exportfile.name.c_str());
					exportFileName = PathHelpers::ReplaceExtension(exportFileName,"anm");
					if (!CompileToANM(cc,exportfile.authorTool,exportfile.pCGF,exportfile.pCtrlSkinningInfo,exportFileName))
						success = false;
				}
				break;

			// Export to CAF
			case EXPORT_CAF:
				{
					string exportFileName = PathHelpers::Join(outputFolder, exportfile.name.c_str());
					exportFileName = PathHelpers::ReplaceExtension(exportFileName,"caf");
					if (!CompileToCAF(cc,exportfile.authorTool,exportfile.pCGF,exportfile.pCtrlSkinningInfo,exportFileName))
						success = false;
				}
				break;
		}
	}

	if (success)
	{
		FILE* const f = fopen(filenameForUptodateCheck.c_str(),"wb");
		if (f)
		{
			fwrite("v", 1, 1, f);
			fclose(f);
		}
	}

	return success;
}

void ColladaCompiler::ConstructAndSetOutputFile(ConvertContext &cc)
{
	cc.SetOutputFile("*unused*");
}

void ColladaCompiler::GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const
{
	string const file = PathHelpers::GetFilename(cc.getSourcePath()) + ".rcdone";
	string const folder = PathHelpers::GetDirectory(cc.getOutputPath());
	string const path(PathHelpers::Join(folder, file));

	if (filenameBuffer && (path.length() < bufferSize))
	{
		strcpy(filenameBuffer, path.c_str());		
	}
}

ICompiler* ColladaCompiler::CreateCompiler()
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

bool ColladaCompiler::SupportsMultithreading() const
{
	return false;
}

int ColladaCompiler::GetNumPlatforms() const
{
	return 1;
}

EPlatform ColladaCompiler::GetPlatform(int index) const
{
	switch (index)
	{
	case 0:	return ePlatform_PC;
	//case 1:	return ePlatform_X360;
	//case 2:	return ePlatform_PS2;
	//case 3:	return ePlatform_GAMECUBE;
	};
	return ePlatform_UNKNOWN;
}

int ColladaCompiler::GetNumExt() const
{
	return 2;
}

const char* ColladaCompiler::GetExt(int index) const
{
	return (index == 0)  ? "dae" : "dae.zip";
}

DWORD ColladaCompiler::GetTimestamp() const
{
	return GetTimestampForLoadedLibrary(g_hInst);
}

bool ColladaCompiler::PrepareCGF(CContentCGF* pCGF)
{
	// Loop through all the meshes.
	for (int i = 0; i < pCGF->GetNodeCount(); i++) 
	{
		CNodeCGF *pNodeCGF = pCGF->GetNode(i);
		CMesh* mesh = pNodeCGF->pMesh;
		if (mesh)
		{
			// If the mesh has no UVs, generate them here.
			if (mesh->m_nCoorCount == 0)
				GenerateDefaultUVs(*mesh);
		}
	}

	return true;
}

void ColladaCompiler::FindUsedMaterialIDs(std::vector<int>& usedMaterialIDs, CContentCGF* pCGF)
{
	std::set<int> materialIDSet;
	for (int nodeIndex = 0; nodeIndex < pCGF->GetNodeCount(); ++nodeIndex)
	{
		CNodeCGF* node = pCGF->GetNode(nodeIndex);
		CMesh* mesh = 0;
		if (node && node->pMesh && !node->bPhysicsProxy && node->type == CNodeCGF::NODE_MESH)
			mesh = node->pMesh;
		if (mesh)
		{
			for (int subsetIndex = 0; subsetIndex < mesh->m_subsets.size(); ++subsetIndex)
			{
				SMeshSubset& subset = mesh->m_subsets[subsetIndex];
				materialIDSet.insert(subset.nMatID);
			}
		}
	}

	usedMaterialIDs.clear();
	std::copy(materialIDSet.begin(), materialIDSet.end(), back_inserter(usedMaterialIDs));
}

// Copied from MaxCryExport
void GenerateDefaultUVs(CMesh& mesh)
{
	unsigned numVerts = mesh.m_numVertices;
	mesh.SetTexCoordsCount(numVerts);

	// make up the cubic coordinates
	for (unsigned nVert = 0; nVert < numVerts; ++nVert)
	{
		Vec3& v = mesh.m_pPositions[nVert];
		Vec3 r(fabs(v.x), fabs(v.y), fabs(v.z));
		SMeshTexCoord uv = {0, 0};
		if (r.x > 1e-3 || r.y > 1e-3 || r.z > 1e-3)
		{
			if (r.x > r.y)
			{
				if (r.x > r.z)
				{
					// X rules
					uv.s = v.y/r.x;
					uv.t = v.z/r.x;
				}
				else
				{
					// Z rules
					uv.s = v.x / r.z;
					uv.t = v.y / r.z;
				}
			}
			else
			{
				// r.x < r.y
				if (r.y > r.z)
				{
					// Y rules
					uv.s = v.x / r.y;
					uv.t = v.z / r.y;
				}
				else
				{
					// Z rules
					uv.s = v.x / r.z;
					uv.t = v.y / r.z;
				}
			}
		}
		// now the texture coordinates are in the range [-1,1]
		// we want normalized to 0..1 texture coordinates
		uv.s = (uv.s + 1)/2;
		uv.t = (uv.t + 1)/2;

		mesh.m_pTexCoord[nVert] = uv;
	}

	// Store the texture vertex indices in the face.
	for (int faceIndex = 0; faceIndex < mesh.m_numFaces; ++faceIndex)
	{
		mesh.m_pFaces[faceIndex].t[0] = mesh.m_pFaces[faceIndex].v[0];
		mesh.m_pFaces[faceIndex].t[1] = mesh.m_pFaces[faceIndex].v[1];
		mesh.m_pFaces[faceIndex].t[2] = mesh.m_pFaces[faceIndex].v[2];
	}
}
