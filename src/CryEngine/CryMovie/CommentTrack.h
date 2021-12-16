////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: CommentTrack.h
//  Version:   v1.00
//  Created:   24-03-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __COMMENTTRACK_H__
#define __COMMENTTRACK_H__

#pragma once

#include "IMovieSystem.h"
#include "AnimTrack.h"

class CCommentTrack : public TAnimTrack<ICommentKey>
{
public:
	//-----------------------------------------------------------------------------
	//!
	CCommentTrack();

	//-----------------------------------------------------------------------------
	//! IAnimTrack Method Overriding.
	//-----------------------------------------------------------------------------

	virtual EAnimTrackType GetType() { return ATRACK_COMMENT_TEXT; }
	virtual EAnimValue     GetValueType() { return AVALUE_COMMENT_TEXT; }

	virtual void GetKeyInfo( int key,const char* &description,float &duration );
	virtual void SerializeKey( ICommentKey &key,XmlNodeRef &keyNode,bool bLoading );

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const;

	//-----------------------------------------------------------------------------
	//!
	void ValidateKeyOrder() { CheckValid(); }

};

#endif//__COMMENTTRACK_H__