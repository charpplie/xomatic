//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXSKELETON_H__
#define __MAXSKELETON_H__

#include "ISkeleton.h"
#include "IBone.h"
#include "IBoneArray.h"
//#include "IMaterial.h"
#include "NameList.h"

#include <vector>

class INode;
class MaxMaterial;
class MaxMesh;
class IErrorReporter;
class MaxPhysicsNode;

class MaxSkeleton : public ISkeleton
{
public:
	class BoneArray;

	class Bone : public IBone
	{
	public:
		explicit Bone(IErrorReporter* pErrorReporter, const std::string& sName, INode* pMaxNode, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, std::map<Mtl*, MaxMaterial*>& materialMap, MaxPhysicsNode* pPhysicsNode, float fMorphMinOffset);
		virtual ~Bone();

		// IBone
		virtual ISourceMaterial* GetMaterial();
		virtual ISourceMesh* GetMesh();
		virtual IPhysicsNode* GetPhysicsNode();
		virtual bool MarkedNonPhysical();
		virtual std::string GetUserProperty();
		virtual IBoneArray* GetChildren();
		virtual std::string GetBoneName();
		virtual IBone* GetParent();
		virtual void GetTransform(Matrix34& matrix);

		void AddChild(Bone* pBone);
		INode* GetMaxNode();
		void SetParent(Bone* pParent);

	private:
		void CreateMaterial(std::map<Mtl*, MaxMaterial*>& materialMap);

		INode* pMaxNode;
		MaxMaterial* pMaterial;
		MaxMesh* pMesh;
		MaxPhysicsNode* pPhysicsNode;
		BoneArray* pChildren;
		std::string sName;
		Bone* pParent;
	};

	class BoneArray : public IBoneArray
	{
	public:
		BoneArray();
		virtual ~BoneArray();

		virtual int Count();
		virtual IBone* Get(int nIndex);

		void AddBone(Bone* pBone);
		void DeleteChildren();

	private:
		std::vector<Bone*> bones;
	};

	MaxSkeleton();
	virtual ~MaxSkeleton();

	virtual IBoneArray* GetBones();

	void AddBone(Bone* pBone);

private:
	BoneArray* pBoneArray;
};

#endif //__MAXSKELETON_H__
