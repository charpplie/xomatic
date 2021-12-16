////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: LayerNodeAnimator.h
//  Version:   v1.00
//  Created:   22-03-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __LAYERNODEANIMATOR_H__
#define __LAYERNODEANIMATOR_H__

#pragma once

class CLayerNodeAnimator : public IAnimNodeAnimator
{
public:
	//-----------------------------------------------------------------------------
	//!
	CLayerNodeAnimator();

	//-----------------------------------------------------------------------------
	//!
	void Animate( IAnimNode *pNode , const SAnimContext& ac );
	
protected:
	//-----------------------------------------------------------------------------
	//!
	virtual ~CLayerNodeAnimator() {}
};



#endif//__LAYERNODEANIMATOR_H__