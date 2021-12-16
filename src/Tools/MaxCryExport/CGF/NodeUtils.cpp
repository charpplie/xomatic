//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "NodeUtils.h"
#include "ModifierUtils.h"

namespace NodeUtils
{
	class NodeNameLess : public std::less<INode*>
	{
	public:
		bool operator()(INode* left, INode* right)
		{
			return stricmp(left->GetName(), right->GetName()) < 0;
		}
	};
}

bool NodeUtils::GetValidChildrenList(INode *node, std::vector<INode*>& children, ValidChildrenListRecurseSetting eRecurse, ValidChildrenListKeepDummySetting eKeepDummy, ValidChildrenListSortSetting eSort)
{
	Interval ivalid;
	TimeValue time=0;//GetTime();

	std::vector<INode*> nodeChildren;
	for(int i=0;i<node->NumChildren();i++)
	{
		INode *child = node->GetChildNode(i);
		if (child)
			nodeChildren.push_back(child);
	}

	if (eSort == ValidChildrenListSort)
		std::sort(nodeChildren.begin(), nodeChildren.end(), NodeNameLess());

	for (int i = 0, count = nodeChildren.size(); i < count; ++i)
	{
		INode *child = nodeChildren[i];
		if (!child)
			continue;

		char* szName = child->GetName();
		OutputDebugString (szName);
		OutputDebugString("\n");

		//do not export footprint node
		if(IsFootPrint(child)) continue;

		//don't export deformed mesh as a bone
		if(FindPhysiqueModifier(child->GetObjectRef())) continue;

		if(FindSkinModifier(child->GetObjectRef())) continue;

		//Ignore dummy objects if the checkbox is set
		if (eKeepDummy == ValidChildrenListIgnoreDummy)
		{
			ObjectState os		=child->EvalWorldState(time);
			Class_ID	cid		=os.obj->ClassID();
			if(cid == Class_ID(DUMMY_CLASS_ID,0))
			{
				continue;
			}
		}

		// Ignore bones with name that start with underscore.
		if (*szName == '_')
		{
			continue;
		}

		children.push_back(child);

		if(eRecurse == ValidChildrenListRecurse)
			if (GetValidChildrenList(child, children, eRecurse, eKeepDummy, eSort))
				return true;
	}
	return false;
}

bool NodeUtils::IsFootPrint(INode *node)
{
	if(!node) return false;
	Control *pc=node->GetTMController();
	return (pc && (pc->ClassID() == FOOTPRINT_CLASS_ID));
}
