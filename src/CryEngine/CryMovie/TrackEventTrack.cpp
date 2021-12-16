////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TrackEventTrack.cpp
//  Version:     v1.00
//  Created:     4/4/2008 by Kevin.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TrackEventTrack.h"

//////////////////////////////////////////////////////////////////////////
void CTrackEventTrack::SerializeKey( IEventKey &key,XmlNodeRef &keyNode,bool bLoading )
{
	if (bLoading)
	{
		const char *str;
		str = keyNode->getAttr( "event" );
		strncpy( key.event,str,sizeof(key.event) );
		key.event[sizeof(key.event)-1] = 0;

		str = keyNode->getAttr( "eventValue" );
		strncpy( key.eventValue,str,sizeof(key.eventValue) );
		key.eventValue[sizeof(key.eventValue)-1] = 0;
	}
	else
	{
		if (strlen(key.event) > 0)
			keyNode->setAttr( "event",key.event );
		if (strlen(key.eventValue) > 0)
			keyNode->setAttr( "eventValue",key.eventValue );
	}
}

void CTrackEventTrack::GetKeyInfo( int key,const char* &description,float &duration )
{
	static char desc[128];

	assert( key >= 0 && key < (int)m_keys.size() );
	CheckValid();
	description = 0;
	duration = 0;
	if (strlen(m_keys[key].event) > 0)
		strcpy_s(desc, m_keys[key].event);
	else
		strcpy_s(desc, "");
	if (strlen(m_keys[key].eventValue) > 0)
	{
		strcat_s(desc, ", ");
		strcat_s(desc, m_keys[key].eventValue);
	}

	description = desc;
}