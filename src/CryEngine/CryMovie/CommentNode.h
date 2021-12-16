////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: CommentNode.h
//  Version:   v1.00
//  Created:   08-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __COMMENTNODE_H__
#define __COMMENTNODE_H__

#pragma once

#include "AnimNode.h"

class CCommentContext;

class CCommentNode : public CAnimNode
{
public:
	CCommentNode();
	static void Initialize();

	//-----------------------------------------------------------------------------
	//!
	virtual EAnimNodeType GetType() const { return ANODE_COMMENT; }

	//-----------------------------------------------------------------------------
	//! Overrides from CAnimNode
	virtual void Animate(SAnimContext &ac);

	virtual void CreateDefaultTracks();

	virtual void Reset();

	virtual void Activate( bool bActivate );

	virtual void Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks );

	//-----------------------------------------------------------------------------
	//! Overrides from IAnimNode
	virtual int GetParamCount() const;

	virtual bool GetParamInfo( int nIndex, SParamInfo &info ) const;

	virtual bool GetParamInfoFromId( int paramId, SParamInfo &info ) const;
};

#endif//__COMMENTNODE_H__
