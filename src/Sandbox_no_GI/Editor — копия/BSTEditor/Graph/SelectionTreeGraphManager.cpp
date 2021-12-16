////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeGraphManager.cpp
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTreeGraphManager.h"

#include "SelectionTreeGraph.h"

#include "Nodes/SelectionTree_TreeNode.h"
#include "Nodes/SelectionTree_RefNode.h"
#include "Nodes/SelectionTree_ConditionNode.h"

CSelectionTreeGraphManager::CSelectionTreeGraphManager()
{
	ReloadClasses();
}


CSelectionTreeGraphManager::~CSelectionTreeGraphManager()
{

}


void CSelectionTreeGraphManager::Init()
{
	__super::Init();

	ReloadClasses();
}


void CSelectionTreeGraphManager::ReloadClasses()
{
	ClearPrototypes();

	AddNodePrototype( new CSelectionTree_TreeNode() );
	AddNodePrototype( new CSelectionTree_RefNode() );
 	AddNodePrototype( new CSelectionTree_ConditionNode() );
}


CHyperNode* CSelectionTreeGraphManager::CreateNode( CHyperGraph* pGraph, const char* nodeClass, HyperNodeID nodeId, const Gdiplus::PointF& position, CBaseObject* pObj )
{
	CHyperNode* pNewNode = __super::CreateNode( pGraph, nodeClass, nodeId, position, pObj );
	return pNewNode;
}

CHyperGraph* CSelectionTreeGraphManager::CreateGraph()
{
	CSelectionTreeGraph* pSelectionTreeGraph = new CSelectionTreeGraph( this );
	return pSelectionTreeGraph;
}

void CSelectionTreeGraphManager::ClearPrototypes()
{
	m_prototypes.clear();
}

void CSelectionTreeGraphManager::AddNodePrototype( _smart_ptr< CSelectionTree_BaseNode > pPrototypeNode )
{
	assert( pPrototypeNode.get() != NULL );
	if ( pPrototypeNode.get() == NULL )
	{
		return;
	}

	const char* nodeClassName = pPrototypeNode->GetClassName();
	assert( nodeClassName != NULL );
	if ( nodeClassName == NULL )
	{
		return;
	}
	string prototypeName( nodeClassName );

	m_prototypes[ prototypeName ] = pPrototypeNode;
}