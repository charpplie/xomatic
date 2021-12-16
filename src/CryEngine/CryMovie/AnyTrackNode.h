////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   AnyTrackNode.h
//  Version:     v1.00
//  Created:     13/5/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AnyTrackNode_h__
#define __AnyTrackNode_h__
#pragma once

#include "AnimNode.h"

class CAnimAnyTrackNode : public CAnimNode
{
public:
	CAnimAnyTrackNode();

	virtual EAnimNodeType GetType() const { return ANODE_MATERIAL; }

	//////////////////////////////////////////////////////////////////////////
	// Overrides from CAnimNode
	//////////////////////////////////////////////////////////////////////////
	void Animate( SAnimContext &ec );

	//////////////////////////////////////////////////////////////////////////
	// Supported tracks description.
	//////////////////////////////////////////////////////////////////////////
	virtual int GetParamCount() const;
	virtual bool GetParamInfo( int nIndex, SParamInfo &info ) const;
	virtual bool GetParamInfoFromId( int paramId, SParamInfo &info ) const;
};

#endif // __AnyTrackNode_h__