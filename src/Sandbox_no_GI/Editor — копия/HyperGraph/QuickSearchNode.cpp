////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   QuickSearchNode.cpp
//  Version:     v1.00
//  Created:     19/2/2011 by Sascha Hoba.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "QuickSearchNode.h"

#include "HyperNodePainter_QuickSearch.h"

static CHyperNodePainter_QuickSearch painter;

CQuickSearchNode::CQuickSearchNode(void):m_iSearchResultCount(1),m_iIndex(1)
{
	SetClass(GetClassType());
	m_pPainter = &painter;
	m_name = "Misc:Start";
}

CQuickSearchNode::~CQuickSearchNode(void)
{
}

void CQuickSearchNode::Init()
{
}

void CQuickSearchNode::Done()
{
}

CHyperNode* CQuickSearchNode::Clone()
{
	CQuickSearchNode* pNode = new CQuickSearchNode();
	pNode->CopyFrom(*this);
	return pNode;
}