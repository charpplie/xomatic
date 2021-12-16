////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: CommentTrack.cpp
//  Version:   v1.00
//  Created:   24-03-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "StdAfx.h"
#include "CommentTrack.h"

//-----------------------------------------------------------------------------
CCommentTrack::CCommentTrack()
{

}

//-----------------------------------------------------------------------------
void CCommentTrack::GetKeyInfo( int key,const char* &description,float &duration )
{
	static char desc[128];
	assert( key >= 0 && key < (int)m_keys.size() );
	CheckValid();
	description = 0;
	duration = m_keys[key].m_duration;

#ifndef LINUX
	strncpy_s(desc,sizeof(desc),m_keys[key].m_strComment.c_str(),_TRUNCATE);
#else
	strncpy(desc,m_keys[key].m_strComment.c_str(), sizeof(desc));
	desc[sizeof(desc)-1] = NULL;
#endif

	description = desc;

}

//-----------------------------------------------------------------------------
void CCommentTrack::SerializeKey( ICommentKey &key,XmlNodeRef &keyNode,bool bLoading )
{
	if (bLoading)
	{
		// Load only in the editor for loading performance.
		if(gEnv->IsEditor())
		{
			XmlString xmlComment;
			keyNode->getAttr( "comment",xmlComment);
			key.m_strComment = xmlComment;
			keyNode->getAttr( "duration",key.m_duration);
			const char* strFont = keyNode->getAttr( "font");
			strcpy_s(key.m_strFont, strFont);
			keyNode->getAttr( "color", key.m_color);
			keyNode->getAttr( "size", key.m_size);
			int alignment = 0;
			keyNode->getAttr( "align", alignment);
			key.m_align = (ICommentKey::ETextAlign)(alignment);
		}
	}
	else
	{
		XmlString xmlComment(key.m_strComment.c_str());
		keyNode->setAttr( "comment",xmlComment);
		keyNode->setAttr( "duration",key.m_duration);
		keyNode->setAttr( "font", key.m_strFont);
		keyNode->setAttr( "color", key.m_color);
		keyNode->setAttr( "size", key.m_size);
		keyNode->setAttr( "align", (int)key.m_align);
	}
}

void CCommentTrack::GetMemoryUsage(ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
}