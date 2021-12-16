////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   FBXExporter.h
//  Version:     v1.00
//  Created:     24 June 2011 by Sergiy Shaykin.
//  Compilers:   Visual Studio 2008
//  Description: Export geometry to FBX file format
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Include/IExportManager.h>
#include "fbxsdk.h"

struct SFBXSettings
{
	bool bCopyTextures; 
	bool bEmbedded;
	bool bAsciiFormat;
};

class CFBXExporter : public IExporter
{
public:
	CFBXExporter();

	virtual const char* GetExtension() const;
	virtual const char* GetShortDescription() const;
	virtual bool ExportToFile(const char* filename, const Export::IData* pData);
	virtual bool ImportFromFile(const char* filename, Export::IData* pData);
	virtual void Release();

private:
	FbxMesh* CreateFBXMesh(const Export::Object* pObj);
	FbxFileTexture* CreateFBXTexture(const char* pTypeName, const char* pName);
	FbxSurfaceMaterial* CreateFBXMaterial(const std::string& name, const Export::Mesh* pMesh);
	FbxNode* CreateFBXNode(const Export::Object* pObj);
	FbxNode* CreateFBXAnimNode(FbxScene *pScene, FbxAnimLayer* pCameraAnimBaseLayer, const Export::Object* pObj);
	void FillAnimationData(Export::Object *pObject,FbxAnimLayer *pAnimLayer, FbxAnimCurve* pCurve, Export::EAnimParamType paramType);

	FbxManager* m_pFBXManager;
	SFBXSettings m_settings;
	FbxScene* m_pFBXScene;
	std::string m_path;
	std::vector<FbxNode*> m_nodes;
	std::map<std::string, FbxSurfaceMaterial*> m_materials;
	std::map<const Export::Mesh*, int> m_meshMaterialIndices;
};