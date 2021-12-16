//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXEXPORTSOURCE_H__
#define __MAXEXPORTSOURCE_H__

#include "IExportSource.h"
#include "IExportSourceInfo.h"
#include "NodeUtils.h"
#include "NameList.h"
#include <map>

class Interface;
class MaxSkeleton;
class MaxObjectArray;
class IErrorReporter;
class MaxBreakablePhysicsInfo;
class MaxSceneProperties;
class MaxMaterial;
class MaxPhysicsNode;
class MaxExportFlags;
class JointParams;

class MaxExportSource : public IExportSource, public IExportSourceInfo
{
public:
	MaxExportSource(IErrorReporter* pErrorReporter, Interface* pMaxInterface, const std::vector<INode*>& nodes, const std::vector<INode*>& rootBones, NodeUtils::ValidChildrenListKeepDummySetting eKeepDummies, NodeUtils::ValidChildrenListSortSetting eSort, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, int nGranularity, bool bShouldMergeObjects, bool bShouldWriteWeights, bool bShouldWriteVertexColours, bool bShouldAllowMultipleUVs, float fMorphMinOffset);
	virtual ~MaxExportSource();

	// IExportSource
	virtual IExportSourceInfo* GetInfo();
	virtual ISkeleton* GetSkeleton();
	virtual ISourceObjectArray* GetObjects();
	virtual IBreakablePhysicsInfo* GetBreakablePhysicsInfo();
	virtual ISceneProperties* GetSceneProperties();
	virtual IExportFlags* GetExportFlags();

	// IExportSourceInfo
	virtual std::time_t GetExportTime();
	virtual std::string GetFileName();

private:
	void CreateSkeleton(IErrorReporter* pErrorReporter, const std::vector<INode*>& rootBones, NodeUtils::ValidChildrenListKeepDummySetting eKeepDummies, NodeUtils::ValidChildrenListSortSetting eSort, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, float fMorphMinOffset);
	void CreateObjectArray(IErrorReporter* pErrorReporter, const std::vector<INode*>& nodes, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, float fMorphMinOffset);
	
	INode* FindPhysicsNode(INode* pNode);
	INode* FindPhysicsFrameNode(INode* pNode);
	MaxPhysicsNode* CreatePhysicsNode(INode* pNode);
	JointParams* GetJointParamsFromNode(INode* pNode);

	Interface* pMaxInterface;
	MaxSkeleton* pMaxSkeleton;
	MaxObjectArray* pObjects;
	MaxBreakablePhysicsInfo* pBreakablePhysicsInfo;
	MaxSceneProperties* pSceneProperties;
	MaxExportFlags* pExportFlags;
	std::map<Mtl*, MaxMaterial*> m_materialMap;
};

#endif //__MAXEXPORTSOURCE_H__
