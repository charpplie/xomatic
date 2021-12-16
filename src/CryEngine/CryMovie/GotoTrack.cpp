////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   booltrack.cpp
//  Version:     v1.00
//  Created:     29/10/2009 by Paulo Zaffari.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GotoTrack.h"

#define MIN_TIME_PRECISION 0.01f

//////////////////////////////////////////////////////////////////////////
CGototTrack::CGototTrack()
{
	m_flags = 0;
	m_DefaultValue=-1.0f;
}
////////////////////////////////////////////////////////////////////////
void CGototTrack::GetValue( float time,float &value )
{
	size_t nTotalKeys(m_keys.size());

	value = m_DefaultValue;

	if(nTotalKeys < 1)
	{		
		return;
	}

	CheckValid();

	size_t nKey(0);
	for (nKey=0;nKey<nTotalKeys;++nKey)
	{
		if (time>=m_keys[nKey].time)
		{
			value = m_keys[nKey].m_fValue;
		}
		else
		{
			break;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CGototTrack::SetValue( float time,const float &value,bool bDefault)
{
	if (!bDefault)
	{
		IDiscreteFloatKey oKey;

		oKey.SetValue(value);
		SetKeyAtTime( time,&oKey );
	}
	else
	{
		m_DefaultValue = value;
	}
}
////////////////////////////////////////////////////////////////////////
void CGototTrack::SerializeKey( IDiscreteFloatKey &key,XmlNodeRef &keyNode,bool bLoading)
{
	if (bLoading)
	{
		keyNode->getAttr( "time",key.time );
		keyNode->getAttr( "value",key.m_fValue );
		//assert(key.time == key.m_fValue);

		keyNode->getAttr( "flags",key.flags );
	}
	else
	{
		keyNode->setAttr( "time",key.time );
		keyNode->setAttr( "value",key.m_fValue);

		int flags = key.flags;
		if(flags != 0)
			keyNode->setAttr( "flags",flags );
	}
}
//////////////////////////////////////////////////////////////////////////
void CGototTrack::GetKeyInfo( int index,const char* &description,float &duration )
{
	static char str[64];
	description = str;
	assert( index >= 0 && index < GetNumKeys() );
	float &k = m_keys[index].m_fValue;
	sprintf_s( str,"%.2f",k);
}
//////////////////////////////////////////////////////////////////////////
void CGototTrack::SetKeyAtTime( float time,IKey *key )
{
	assert( key != 0 );

	key->time = time;

	bool found = false;
	// Find key with given time.
	for (int i = 0; i < m_keys.size(); i++)
	{
		float keyt = m_keys[i].time;
		if (fabs(keyt-time) < MIN_TIME_PRECISION)
		{
			key->flags = m_keys[i].flags;				// Reserve the flag value.
			SetKey( i,key );
			found = true;
			break;
		}
		//if (keyt > time)
		//break;
	}
	if (!found)
	{
		// Key with this time not found.
		// Create a new one.
		int keyIndex = CreateKey(time);
		// Reserve the flag value.
		key->flags = m_keys[keyIndex].flags;		// Reserve the flag value.
		SetKey( keyIndex,key );
	}
}
//////////////////////////////////////////////////////////////////////////
