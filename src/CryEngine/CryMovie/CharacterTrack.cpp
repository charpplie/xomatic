////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   charactertrack.h.cpp
//  Version:     v1.00
//  Created:     20/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CharacterTrack.h"

//////////////////////////////////////////////////////////////////////////
void CCharacterTrack::SerializeKey( ICharacterKey &key,XmlNodeRef &keyNode,bool bLoading )
{
	if (bLoading)
	{
		const char *str;
		
		str = keyNode->getAttr( "anim" );
		strncpy( key.m_animation,str,sizeof(key.m_animation) );
		key.m_animation[sizeof(key.m_animation)-1] = 0;

		key.m_duration = 0;
		key.m_blendTime = 0;
		key.m_startTime = 0;
		key.m_bLoop = false;
		key.m_bInPlace = false;
		key.m_speed = 1;
		keyNode->getAttr( "length",key.m_duration );
		keyNode->getAttr( "blend",key.m_blendTime );
		keyNode->getAttr( "speed",key.m_speed );
		keyNode->getAttr( "loop",key.m_bLoop );
		keyNode->getAttr( "unload",key.m_bUnload );
		keyNode->getAttr( "inplace",key.m_bInPlace );
		keyNode->getAttr( "start",key.m_startTime );
	}
	else
	{
		if (strlen(key.m_animation) > 0)
			keyNode->setAttr( "anim",key.m_animation );
		if (key.m_duration > 0)
			keyNode->setAttr( "length",key.m_duration );
		if (key.m_blendTime > 0)
			keyNode->setAttr( "blend",key.m_blendTime );
		if (key.m_speed != 1)
			keyNode->setAttr( "speed",key.m_speed );
		if (key.m_bLoop)
			keyNode->setAttr( "loop",key.m_bLoop );
		if (key.m_bUnload)
			keyNode->setAttr( "unload",key.m_bUnload );
		if (key.m_bInPlace)
			keyNode->setAttr( "inplace",key.m_bInPlace );
		if (key.m_startTime != 0)
			keyNode->setAttr( "start",key.m_startTime );
	}
}

void CCharacterTrack::GetKeyInfo( int key,const char* &description,float &duration )
{
	assert( key >= 0 && key < (int)m_keys.size() );
	CheckValid();
	description = 0;
	duration = 0;
	if (strlen(m_keys[key].m_animation) > 0)
	{
		description = m_keys[key].m_animation;
		if (m_keys[key].m_bLoop)
		{
			float lastTime = m_timeRange.end;
			if (key+1 < (int)m_keys.size())
			{
				lastTime = m_keys[key+1].time;
			}
			// duration is unlimited but cannot last past end of track or time of next key on track.
			duration = lastTime - m_keys[key].time;
		}
		else
			duration = (m_keys[key].m_duration - m_keys[key].m_startTime) / m_keys[key].m_speed;
	}
}