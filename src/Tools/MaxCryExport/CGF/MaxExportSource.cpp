//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxExportSource.h"
#include "MaxSkeleton.h"
#include "NodeUtils.h"
#include "MaxObject.h"
#include "MaxBreakablePhysicsInfo.h"
#include "MaxSceneProperties.h"
#include "MaxPhysicsNode.h"
#include "MaxJointParameters.h"
#include "MaxPhysicsFrame.h"
#include "MaxExportFlags.h"
#include "interpik.h"
#include <ctime>
#include <sstream>

MaxExportSource::MaxExportSource(IErrorReporter* pErrorReporter, Interface* pMaxInterface, const std::vector<INode*>& nodes, const std::vector<INode*>& rootBones, NodeUtils::ValidChildrenListKeepDummySetting eKeepDummies, NodeUtils::ValidChildrenListSortSetting eSort, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, int nGranularity, bool bShouldMergeObjects, bool bShouldWriteWeights, bool bShouldWriteVertexColours, bool bShouldAllowMultipleUVs, float fMorphMinOffset)
:	pMaxInterface(pMaxInterface),
	pMaxSkeleton(0),
	pBreakablePhysicsInfo(0),
	pSceneProperties(0),
	pExportFlags(0)
{
	this->CreateSkeleton(pErrorReporter, rootBones, eKeepDummies, eSort, bGenerateDefaultUVs, bAllowBlending, BoneList, fMorphMinOffset);
	this->CreateObjectArray(pErrorReporter, nodes, bGenerateDefaultUVs, bAllowBlending, BoneList, fMorphMinOffset);
	this->pBreakablePhysicsInfo = new MaxBreakablePhysicsInfo(nGranularity);
	this->pSceneProperties = new MaxSceneProperties(this->pMaxInterface);
	this->pExportFlags = new MaxExportFlags(bShouldMergeObjects, bShouldWriteWeights, bShouldWriteVertexColours, bShouldAllowMultipleUVs);
}

MaxExportSource::~MaxExportSource()
{
	delete this->pBreakablePhysicsInfo;
	delete this->pSceneProperties;
}

IExportSourceInfo* MaxExportSource::GetInfo()
{
	return this;
}

ISkeleton* MaxExportSource::GetSkeleton()
{
	return this->pMaxSkeleton;
}

ISourceObjectArray* MaxExportSource::GetObjects()
{
	return this->pObjects;
}

IBreakablePhysicsInfo* MaxExportSource::GetBreakablePhysicsInfo()
{
	return this->pBreakablePhysicsInfo;
}

ISceneProperties* MaxExportSource::GetSceneProperties()
{
	return this->pSceneProperties;
}

IExportFlags* MaxExportSource::GetExportFlags()
{
	return this->pExportFlags;
}

std::time_t MaxExportSource::GetExportTime()
{
	return std::time(0);
}

std::string MaxExportSource::GetFileName()
{
	return this->pMaxInterface->GetCurFilePath().data();
}

void MaxExportSource::CreateSkeleton(IErrorReporter* pErrorReporter, const std::vector<INode*>& rootBones, NodeUtils::ValidChildrenListKeepDummySetting eKeepDummies, NodeUtils::ValidChildrenListSortSetting eSort, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, float fMorphMinOffset)
{
	if (!rootBones.empty())
	{
		// Allocate the new skeleton.
		this->pMaxSkeleton = new MaxSkeleton();

		// Add all the bones to the skeleton.
		std::vector<INode*> bones;
		for (std::vector<INode*>::const_iterator itBone = rootBones.begin(); itBone != rootBones.end(); ++itBone)
		{
			bones.push_back(*itBone);
			NodeUtils::GetValidChildrenList(*itBone, bones, NodeUtils::ValidChildrenListRecurse, eKeepDummies, eSort);
		}

		std::map<INode*, MaxSkeleton::Bone*> boneMap;
		for (std::vector<INode*>::iterator itBone = bones.begin(); itBone != bones.end(); ++itBone)
		{
			std::string sName = BoneList.arrNames[BoneList.GetID(*itBone)];
			MaxSkeleton::Bone* pBone = new MaxSkeleton::Bone(pErrorReporter, sName, *itBone, bGenerateDefaultUVs, bAllowBlending, BoneList, m_materialMap, this->CreatePhysicsNode(*itBone), fMorphMinOffset);
			this->pMaxSkeleton->AddBone(pBone);

			// Add the bone to the bone map so we can find it again later.
			boneMap.insert(std::make_pair(*itBone, pBone));
		}

		// Loop through all the bones we created, adding them as children to their parents.
		for (std::vector<INode*>::iterator itBone = bones.begin(); itBone != bones.end(); ++itBone)
		{
			// Find the bone in the bone map.
			MaxSkeleton::Bone* pBone = (*boneMap.find(*itBone)).second;

			// If the bone has a parent, look for it in the bone map and register the bone as a child.
			if ((*itBone)->GetParentNode())
			{
				std::map<INode*, MaxSkeleton::Bone*>::iterator itBoneMapEntry = boneMap.find((*itBone)->GetParentNode());
				if (itBoneMapEntry != boneMap.end())
				{
					(*itBoneMapEntry).second->AddChild(pBone);
					pBone->SetParent((*itBoneMapEntry).second);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void MaxExportSource::CreateObjectArray(IErrorReporter* pErrorReporter, const std::vector<INode*>& nodes, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, float fMorphMinOffset)
{
	// Create an array with all the objects, keeping a map of nodes to objects.
	std::map<INode*, MaxObject*> objectMap;
	this->pObjects = new MaxObjectArray();
	for (std::vector<INode*>::const_iterator itNode = nodes.begin(); itNode != nodes.end(); ++itNode)
	{
		this->pObjects->AddObject(pErrorReporter, *itNode, bGenerateDefaultUVs, bAllowBlending, BoneList, m_materialMap, fMorphMinOffset,true);
	}

	// Insert all objects into the map.
	int i = 0;
	for (i = 0; i < pObjects->Count(); i++)
	{
		MaxObject *pObject = (MaxObject*)pObjects->Get(i);
		objectMap.insert(std::make_pair(pObject->GetMaxNode(), pObject));
	}

	// Loop through all the objects we created, link each one to its parent.
	for (std::vector<INode*>::const_iterator itNode = nodes.begin(); itNode != nodes.end(); ++itNode)
	{
		// Find the object in the object map.
		MaxObject* pObject = (*objectMap.find(*itNode)).second;

		// If the object has a parent, look for it in the object map and store the parent pointer.
		if ((*itNode)->GetParentNode())
		{
			std::map<INode*, MaxObject*>::iterator itObjectMapEntry = objectMap.find((*itNode)->GetParentNode());
			if (itObjectMapEntry != objectMap.end())
			{
				pObject->SetParent((*itObjectMapEntry).second);
			}
		}
	}
}

INode* MaxExportSource::FindPhysicsNode(INode* pNode)
{
	std::ostringstream physicsNodeName;
	physicsNodeName << pNode->GetName() << " Phys";
	return GetCOREInterface()->GetINodeByName(physicsNodeName.str().c_str());
}

INode* MaxExportSource::FindPhysicsFrameNode(INode* pNode)
{
	std::ostringstream physicsNodeName;
	physicsNodeName << pNode->GetName() << " Phys ParentFrame";
	return GetCOREInterface()->GetINodeByName(physicsNodeName.str().c_str());
}

MaxPhysicsNode* MaxExportSource::CreatePhysicsNode(INode* pNode)
{
	// Find the physics node that the physics information is drawn from.
	INode* pPhysNode = this->FindPhysicsNode(pNode);

	// Changes made for Anton - if the bone is a rope bone (ie name starts with "rope") *and*
	// has a user property of "active_phys" then we are allowed to fall back to exporting
	// the main bone ik properties as the physical joint properties, even if there is no
	// " Phys" node.
	//
	// Edit: the *and* above used to say "or", which is apparently incorrect. Logic below changed
	// to compensate.
	{
		static const char ropeName[] = "rope";
		bool isRopeBone = (strlen(pNode->GetName()) >= strlen(ropeName) && _strnicmp(ropeName, pNode->GetName(), strlen(ropeName)));
		TSTR userProps;
		pNode->GetUserPropBuffer(userProps);
		bool isActivePhys = (strstr(userProps, "active_phys") != 0);

		if (!pPhysNode && (!isRopeBone || !isActivePhys))
			return 0;
	}

	// Find information about the joint, if there is any.
	JointParams* pJointParams = (pPhysNode ? this->GetJointParamsFromNode(pPhysNode) : 0);
	if (pJointParams == 0)
		pJointParams = this->GetJointParamsFromNode(pNode);
	MaxJointParameters* pJointParametersObject = pJointParams ? new MaxJointParameters(pJointParams) : 0;

	// Find information about the frame.
	MaxPhysicsFrame* pPhysicsFrame = 0;
	INode* pFrameNode = FindPhysicsFrameNode(pNode);
	INode* pParentFrameNode = 0;
	if (pFrameNode != 0)
		pParentFrameNode = pFrameNode->GetParentNode();
	if (pFrameNode && pParentFrameNode)
		pPhysicsFrame = new MaxPhysicsFrame(pFrameNode, pParentFrameNode);

	std::string sProperty;
	if (pPhysNode)
	{
		TSTR property;
		pPhysNode->GetUserPropBuffer(property);
		sProperty = property;
	}

	// Create the physics node.
	MaxPhysicsNode* pPhysicsNode = new MaxPhysicsNode(sProperty, pJointParametersObject, pPhysicsFrame);

	return pPhysicsNode;
}

JointParams* MaxExportSource::GetJointParamsFromNode(INode* pNode)
{
	if (pNode->GetTMController() != 0 && pNode->GetTMController()->GetRotationController())
		return static_cast<JointParams*>(pNode->GetTMController()->GetRotationController()->GetProperty(PROPID_JOINTPARAMS));
	return 0;
}
