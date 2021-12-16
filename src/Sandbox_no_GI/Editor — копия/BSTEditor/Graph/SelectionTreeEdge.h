////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeEdge.h
//  Version:     v1.00
//  Created:     20/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_EDGE__H__
#define __SELECTION_TREE_EDGE__H__

#include "HyperGraph/HyperGraph.h"

class CSelectionTree_BaseNode;

class CSelectionTreeEdge
	: public CHyperEdge
{
public:
	CSelectionTreeEdge();
	virtual ~CSelectionTreeEdge();

	virtual int	GetCustomSelectionMode();

	virtual void DrawSpecial( Gdiplus::Graphics* pGraphics, Gdiplus::PointF point );

	void SetConditionProvider( CSelectionTree_BaseNode* pConditionProvider );

private:
	CSelectionTree_BaseNode* m_pConditionProvider;
};

#endif