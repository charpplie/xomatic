//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IBONE_H__
#define __IBONE_H__

#include <string>

class ISourceMaterial;
class ISourceMesh;
class IPhysicsNode;
class IBoneArray;

class IBone
{
public:
	virtual ~IBone() {}

	virtual ISourceMaterial* GetMaterial() = 0;
	virtual ISourceMesh* GetMesh() = 0;
	virtual IPhysicsNode* GetPhysicsNode() = 0;
	virtual bool MarkedNonPhysical() = 0;
	virtual std::string GetUserProperty() = 0;
	virtual IBoneArray* GetChildren() = 0;
	virtual std::string GetBoneName() = 0;
	virtual IBone* GetParent() = 0;
	virtual void GetTransform(Matrix34& matrix) = 0;
};

#endif //__IBONE_H__
