//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ISOURCEOBJECT_H__
#define __ISOURCEOBJECT_H__

#include <string>

class ISourceMaterial;
class ISourceMesh;
class IController;

class ISourceObject
{
public:
	virtual ~ISourceObject() {}

	virtual std::string GetName() = 0;
	virtual ISourceMaterial* GetMaterial() = 0;
	virtual ISourceMesh* GetMesh() = 0;
	virtual IController* GetPositionController() = 0;
	virtual IController* GetRotationController() = 0;
	virtual IController* GetScaleController() = 0;
	virtual void GetTransform(Matrix34& transform) = 0;
	virtual void GetObjectOffsetTransform(Matrix34& transform) = 0;
	virtual ISourceObject* GetParent() = 0;

	// TEMPORARY workaround - REMOVE ONCE CODE IS SUFFICIENTLY REFACTORED.
	virtual INode* GetMaxNode() = 0;
};

#endif //__ISOURCEOBJECT_H__
