////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeGraphManager.h
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_GRAPH_MANAGER__H__
#define __SELECTION_TREE_GRAPH_MANAGER__H__

#include "HyperGraph/HyperGraphManager.h"

class CSelectionTree_BaseNode;

class CSelectionTreeGraphManager
	: public CHyperGraphManager
{
public:
	CSelectionTreeGraphManager();
	virtual ~CSelectionTreeGraphManager();

	virtual void Init();
	virtual void ReloadClasses();

	virtual CHyperNode* CreateNode( CHyperGraph* pGraph, const char* nodeClass, HyperNodeID nodeId, const Gdiplus::PointF& position, CBaseObject* pObj = NULL );
	virtual CHyperGraph* CreateGraph();

protected:
	void ClearPrototypes();

private:
	void AddNodePrototype( _smart_ptr< CSelectionTree_BaseNode > prototypeNode );
};

#endif