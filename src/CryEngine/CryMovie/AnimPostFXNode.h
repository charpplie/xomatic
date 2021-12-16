////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: AnimPostFXNode.h
//  Version:   v1.00
//  Created:   15-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 


#ifndef __ANIMPOSTFXNODE_H__
#define __ANIMPOSTFXNODE_H__

#pragma once

#include "AnimNode.h"

class CFXNodeDescription;

//////////////////////////////////////////////////////////////////////////
class CAnimPostFXNode : public CAnimNode
{
public:
	//-----------------------------------------------------------------------------
	//!
	static void Initialize();

	//-----------------------------------------------------------------------------
	//!
	static CAnimNode* CreateNode(EAnimNodeType nodeType);

	//-----------------------------------------------------------------------------
	//!
	CAnimPostFXNode(CFXNodeDescription* pDesc);

	//-----------------------------------------------------------------------------
	//!
	virtual EAnimNodeType GetType() const;

	//-----------------------------------------------------------------------------
	//!
	virtual int GetParamCount() const;

	//-----------------------------------------------------------------------------
	//!
	virtual bool GetParamInfo( int nIndex, SParamInfo &info ) const;

	//-----------------------------------------------------------------------------
	//!
	virtual bool GetParamInfoFromId( int paramId, SParamInfo &info ) const;

	//-----------------------------------------------------------------------------
	//!
	virtual void CreateDefaultTracks();

	virtual void Reset();

	//-----------------------------------------------------------------------------
	//!
	virtual void Animate( SAnimContext &ac );

protected:
	typedef std::map< EAnimNodeType, _smart_ptr<CFXNodeDescription> > FxNodeDescriptionMap;
	static FxNodeDescriptionMap s_fxNodeDescriptions;
	static bool s_initialized;

	CFXNodeDescription* m_pDescription;
};

#endif//__ANIMPOSTFXNODE_H__
