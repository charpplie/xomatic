//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __NODEUTILS_H__
#define __NODEUTILS_H__

#include <vector>

namespace NodeUtils
{
	enum ValidChildrenListKeepDummySetting
	{
		ValidChildrenListKeepDummy,
		ValidChildrenListIgnoreDummy
	};
	enum ValidChildrenListRecurseSetting
	{
		ValidChildrenListRecurse,
		ValidChildrenListNoRecurse
	};
	enum ValidChildrenListSortSetting
	{
		ValidChildrenListSort,
		ValidChildrenListNoSort
	};
	bool GetValidChildrenList(INode *node, std::vector<INode*>& children, ValidChildrenListRecurseSetting eRecurse, ValidChildrenListKeepDummySetting eKeepDummy, ValidChildrenListSortSetting eSort);

	bool IsFootPrint(INode *node);
}

#endif //__NODEUTILS_H__
