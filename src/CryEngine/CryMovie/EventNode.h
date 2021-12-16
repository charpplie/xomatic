////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   EventNode.h
//  Version:     v1.00
//  Created:     4/4/2008 by Kevin.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __EVENTNODE_H__
#define __EVENTNODE_H__

#include "AnimNode.h"

class CAnimEventNode : public CAnimNode
{
public:
	CAnimEventNode();

	virtual EAnimNodeType GetType() const { return ANODE_EVENT; }

	//////////////////////////////////////////////////////////////////////////
	// Overrides from CAnimNode
	//////////////////////////////////////////////////////////////////////////
	virtual void Animate(SAnimContext &ec);
	virtual void CreateDefaultTracks();
	virtual void Reset();

	//////////////////////////////////////////////////////////////////////////
	// Supported tracks description.
	//////////////////////////////////////////////////////////////////////////
	virtual int GetParamCount() const;
	virtual bool GetParamInfo(int nIndex, SParamInfo &info) const;
	virtual bool GetParamInfoFromId(int paramId, SParamInfo &info) const;

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		CAnimNode::GetMemoryUsage(pSizer);
	}
private:
	void SetScriptValue();

private:
	//! Last animated key in track.
	int m_lastEventKey;
};

#endif //__EVENTNODE_H__