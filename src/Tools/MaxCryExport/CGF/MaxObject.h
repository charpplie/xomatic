//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXOBJECT_H__
#define __MAXOBJECT_H__

#include "ISourceObject.h"
#include "ISourceObjectArray.h"
#include "NameList.h"
#include "MaxController.h"
#include <vector>
#include <map>

class MaxMesh;
class MaxMaterial;
class IErrorReporter;

class MaxObject : public ISourceObject
{
public:
	explicit MaxObject(IErrorReporter* pErrorReporter, INode* pMaxNode, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, std::map<Mtl*, MaxMaterial*>& materialMap, float fMorphMinOffset);
	virtual ~MaxObject();

	virtual std::string GetName();
	virtual ISourceMaterial* GetMaterial();
	virtual ISourceMesh* GetMesh();
	virtual INode* GetMaxNode();
	virtual IController* GetPositionController();
	virtual IController* GetRotationController();
	virtual IController* GetScaleController();
	virtual void GetTransform(Matrix34& transform);
	virtual void GetObjectOffsetTransform(Matrix34& transform);
	virtual ISourceObject* GetParent();

	void SetParent(MaxObject* pObject);

private:
	void CreateMaterial(std::map<Mtl*, MaxMaterial*>& materialMap);
	void CreateControllers();

	INode* pMaxNode;
	MaxMaterial* pMaterial;
	MaxMesh* pMesh;
	MaxController* pPositionController;
	MaxController* pRotationController;
	MaxController* pScaleController;
	MaxObject* pParent;
};

class MaxObjectArray : public ISourceObjectArray
{
public:
	MaxObjectArray();
	virtual ~MaxObjectArray();

	virtual int Count();
	virtual ISourceObject* Get(int i);

	MaxObject* AddObject(IErrorReporter* pErrorReporter, INode* pMaxNode, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, std::map<Mtl*, MaxMaterial*>& materialMap, float fMorphMinOffset,bool bRecursive);
	MaxObject* FindObject( INode *pMaxNode );

private:
	std::vector<MaxObject*> objects;
};

#endif //__MAXOBJECT_H__
