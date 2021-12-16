////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   CVarNode.h
//  Version:     v1.00
//  Created:     10/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CVarNode_h__
#define __CVarNode_h__
#pragma once

#include "AnimNode.h"

class CAnimCVarNode : public CAnimNode
{
public:
	CAnimCVarNode();

	virtual EAnimNodeType GetType() const { return ANODE_CVAR; }

	//////////////////////////////////////////////////////////////////////////
	// Overrides from CAnimNode
	//////////////////////////////////////////////////////////////////////////
	void SetName( const char *name );
	void Animate( SAnimContext &ec );
	void CreateDefaultTracks();
  void Reset();
  void Resume();

	virtual int GetParamCount() const;
	virtual bool GetParamInfo( int nIndex, SParamInfo &info ) const;
	virtual bool GetParamInfoFromId( int paramId, SParamInfo &info ) const;

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		CAnimNode::GetMemoryUsage(pSizer);
	}
private:
	float m_value;
};

#endif // __CVarNode_h__

