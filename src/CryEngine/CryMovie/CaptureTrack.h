////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   CaptureTrack.h
//  Version:     v1.00
//  Created:     19/11/2009 by JJ
//  Compilers:   Visual Studio 2008
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __capturetrack_h__
#define __capturetrack_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "IMovieSystem.h"
#include "AnimTrack.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/** A track for capturing a movie from the engine rendering.
*/
class CCaptureTrack : public TAnimTrack<ICaptureKey>
{
public:
	virtual EAnimTrackType GetType() { return ATRACK_CAPTURE; };
	virtual EAnimValue GetValueType() { return AVALUE_CAPTURE; };

	void SerializeKey( ICaptureKey &key,XmlNodeRef &keyNode,bool bLoading );
	void GetKeyInfo( int key,const char* &description,float &duration );

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif // __capturetrack_h__
