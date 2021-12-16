////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTree_BaseNode_Painter.h
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE__BASE_NODE__PAINTER__H__
#define __SELECTION_TREE__BASE_NODE__PAINTER__H__

 
#include "HyperGraph/IHyperNodePainter.h"

class CSelectionTree_BaseNode_Painter
	: public IHyperNodePainter
{
public:
	virtual void Paint( CHyperNode* pNode, CDisplayList* pList );
};


#endif