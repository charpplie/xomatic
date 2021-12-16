////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   HyperNodePainter_QuickSearch.h
//  Version:     v1.00
//  Created:     19/2/2011 by Sascha Hoba.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __HYPERNODEPAINTER_QUICKSEARCH_H__
#define __HYPERNODEPAINTER_QUICKSEARCH_H__

#pragma once

#include "IHyperNodePainter.h"

class CHyperNodePainter_QuickSearch : public IHyperNodePainter
{
public:
	virtual void Paint( CHyperNode * pNode, CDisplayList * pList );
};

#endif
