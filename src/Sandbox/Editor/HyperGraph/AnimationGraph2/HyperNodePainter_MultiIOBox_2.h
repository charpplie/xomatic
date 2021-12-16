#ifndef __HYPERNODEPAINTER_MULTIIOBOX2_H__
#define __HYPERNODEPAINTER_MULTIIOBOX2_H__

#pragma once

#include "../IHyperNodePainter.h"

class CHyperNodePainter_MultiIOBox2 : public IHyperNodePainter
{
public:
	virtual void Paint( CHyperNode * pNode, CDisplayList * pList );
};

class CHyperNodePainter_Image2 : public IHyperNodePainter
{
public:
	virtual void Paint( CHyperNode * pNode, CDisplayList * pList );
};

#endif
