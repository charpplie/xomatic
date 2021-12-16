//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __SELECTIONUTILS_H__
#define __SELECTIONUTILS_H__

#include <string>

#include <string>

namespace SelectionUtils
{
	enum Result
	{
		Result_Success,
		Result_CannotFindMesh,
		Result_CannotAddMeshWrapper
	};
	Result SelectFaces(Object* pObject, const std::set<int>& faceIndices, std::string* psError = 0);
	Result CreateMeshSelectWrapper(INode* pNode, std::string* psError = 0);
}

#endif //__SELECTIONUTILS_H__
