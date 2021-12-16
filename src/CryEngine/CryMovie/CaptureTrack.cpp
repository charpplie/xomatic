////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   CaptureTrack.cpp
//  Version:     v1.00
//  Created:     19/11/2009 by JJ
//  Compilers:   Visual Studio 2008
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CaptureTrack.h"

//////////////////////////////////////////////////////////////////////////
void CCaptureTrack::SerializeKey( ICaptureKey &key,XmlNodeRef &keyNode,bool bLoading )
{
	if (bLoading)
	{
		const char *desc;

		keyNode->getAttr( "time", key.time);
		keyNode->getAttr( "flags", key.flags);
		keyNode->getAttr( "duration", key.duration);
		keyNode->getAttr( "timeStep", key.timeStep);
		desc = keyNode->getAttr( "format" );
		if (strcmp(desc, "jpg") == 0)
			key.FormatJPG();
		else if (strcmp(desc, "bmp") == 0)
			key.FormatBMP();
		else if (strcmp(desc, "hdr") == 0)
			key.FormatHDR();
		else
			key.FormatTGA();
		desc = keyNode->getAttr( "folder" );
		strcpy_s(key.folder, desc);
		keyNode->getAttr( "once", key.once);
		desc = keyNode->getAttr( "prefix" );
		if (desc)
			strcpy_s(key.prefix, desc);
		keyNode->getAttr( "bufferToCapture", key.bufferToCapture );
	}
	else
	{
		keyNode->setAttr( "time", key.time);
		keyNode->setAttr( "flags", key.flags);
		keyNode->setAttr( "duration", key.duration);
		keyNode->setAttr( "timeStep", key.timeStep);
		keyNode->setAttr( "format", key.GetFormat());
		keyNode->setAttr( "folder", key.folder);
		keyNode->setAttr( "once", key.once);
		keyNode->setAttr( "prefix", key.prefix);
		keyNode->setAttr( "bufferToCapture", key.bufferToCapture);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCaptureTrack::GetKeyInfo( int key,const char* &description,float &duration )
{
	assert( key >= 0 && key < (int)m_keys.size() );
	static char buffer[256];
	CheckValid();
	duration = m_keys[key].once?0:m_keys[key].duration;
	char prefix[64] = "Frame";
	if(strlen(m_keys[key].prefix) > 0)
		strcpy_s(prefix, m_keys[key].prefix);
	description = buffer;
	if(strlen(m_keys[key].folder) > 0)
		sprintf_s(buffer, "[%s] %s, %.3f, %s", prefix, m_keys[key].GetFormat(), m_keys[key].timeStep, m_keys[key].folder);
	else
		sprintf_s(buffer, "[%s] %s, %.3f", prefix, m_keys[key].GetFormat(), m_keys[key].timeStep);
}