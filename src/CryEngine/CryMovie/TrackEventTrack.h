////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TrackEventTrack.h
//  Version:     v1.00
//  Created:     4/4/2008 by Kevin.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TRACKEVENTTRACK_H__
#define __TRACKEVENTTRACK_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "IMovieSystem.h"
#include "AnimTrack.h"
#include "AnimKey.h"

class CTrackEventTrack : public TAnimTrack<IEventKey>
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Overrides of IAnimTrack.
	//////////////////////////////////////////////////////////////////////////
	EAnimTrackType GetType() { return ATRACK_TRACKEVENT; };
	EAnimValue GetValueType() { return AVALUE_TRACKEVENT; };

	void GetKeyInfo( int key,const char* &description,float &duration );
	void SerializeKey( IEventKey &key,XmlNodeRef &keyNode,bool bLoading );

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif // __TRACKEVENTTRACK_H__
