////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   DiscreteFloatTrack.h
//  Version:     v1.00
//  Created:     29/10/2009 by Paulo Zaffari.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef DiscreteFloatTrack_h__
#define DiscreteFloatTrack_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "IMovieSystem.h"
#include "AnimTrack.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/** Goto track, every key on this track negates boolean value.
*/
class CGototTrack : public TAnimTrack<IDiscreteFloatKey>
{
public:
	CGototTrack();

	virtual EAnimTrackType GetType() { return ATRACK_GOTO; };
	virtual EAnimValue GetValueType() { return AVALUE_DISCRETE_FLOAT; };

	virtual void GetValue( float time,float &value );
	virtual void SetValue( float time,const float &value,bool bDefault=false );

	void SerializeKey( IDiscreteFloatKey &key,XmlNodeRef &keyNode,bool bLoading );
	void GetKeyInfo( int key,const char* &description,float &duration );

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
protected:
	void SetKeyAtTime( float time,IKey *key );

	float m_DefaultValue;
};

#endif // DiscreteFloatTrack_h__
