////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   QuickSearchNode.h
//  Version:     v1.00
//  Created:     19/2/2011 by Sascha Hoba.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __QUICKSEARCHNODE_H__
#define __QUICKSEARCHNODE_H__

#include "HyperGraphNode.h"

class CQuickSearchNode : public CHyperNode
{
public:
	static const char* GetClassType() {
		return "QuickSearch";
	}
	CQuickSearchNode(void);
	virtual ~CQuickSearchNode(void);

	// CHyperNode overwrites
	void Init();
	void Done();
	CHyperNode * Clone();
	virtual bool IsEditorSpecialNode() { return true; }
	virtual bool IsFlowNode() { return false; }
	void SetSearchResultCount(int count){m_iSearchResultCount=count;};
	int GetSearchResultCount(){return m_iSearchResultCount;};

	void SetIndex(int index){m_iIndex=index;};
	int GetIndex(){return m_iIndex;};

private:
	int m_iSearchResultCount;
	int m_iIndex;
};

#endif