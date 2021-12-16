////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: ScreenFaderTrack.cpp
//  Version:   v1.00
//  Created:   28-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "StdAfx.h"

#include "ScreenFaderTrack.h"
#include <IRenderer.h>

//-----------------------------------------------------------------------------
CScreenFaderTrack::CScreenFaderTrack()
{
}

//-----------------------------------------------------------------------------
CScreenFaderTrack::~CScreenFaderTrack()
{
	ReleasePreloadedTextures();
}

//-----------------------------------------------------------------------------
void CScreenFaderTrack::GetKeyInfo( int key,const char* &description,float &duration )
{
	static char desc[32];
	assert( key >= 0 && key < (int)m_keys.size() );
	CheckValid();
	description = 0;
	duration = m_keys[key].m_fadeTime;
	strcpy_s(desc,m_keys[key].m_fadeType == IScreenFaderKey::eFT_FadeIn ? "In" : "Out");

	description = desc;
}

//-----------------------------------------------------------------------------
void CScreenFaderTrack::SerializeKey( IScreenFaderKey &key,XmlNodeRef &keyNode,bool bLoading )
{
	if (bLoading)
	{
		keyNode->getAttr("fadeTime",key.m_fadeTime);
		Vec3 color(0,0,0);
		keyNode->getAttr("fadeColor",color);
		key.m_fadeColor = Vec4(color, 1.f);
		int fadeType = 0;
		keyNode->getAttr("fadeType",fadeType);
		key.m_fadeType = IScreenFaderKey::EFadeType(fadeType);
		int fadeChangeType(0);
		if( keyNode->getAttr("fadeChangeType",fadeChangeType) )
			key.m_fadeChangeType = IScreenFaderKey::EFadeChangeType(fadeChangeType);
		else
			key.m_fadeChangeType = IScreenFaderKey::eFCT_Linear;
		const char *str;
		str = keyNode->getAttr( "texture" );
		strncpy( key.m_strTexture,str,sizeof(key.m_strTexture) );
		key.m_strTexture[sizeof(key.m_strTexture)-1] = 0;
		keyNode->getAttr("useCurColor",key.m_bUseCurColor);
	}
	else
	{
		keyNode->setAttr( "fadeTime",key.m_fadeTime);
		Vec3 color(key.m_fadeColor.x,key.m_fadeColor.y,key.m_fadeColor.z);
		keyNode->setAttr( "fadeColor",color);
		keyNode->setAttr( "fadeType", (int)key.m_fadeType);
		keyNode->setAttr( "fadeChangeType", (int)key.m_fadeChangeType);
		keyNode->setAttr( "texture", key.m_strTexture);
		keyNode->setAttr( "useCurColor",key.m_bUseCurColor);
	}
}

//-----------------------------------------------------------------------------
void CScreenFaderTrack::GetMemoryUsage(ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
}

//-----------------------------------------------------------------------------
void CScreenFaderTrack::PreloadTextures()
{
	if (!m_preloadedTextures.empty())
		ReleasePreloadedTextures();

	const int nKeysCount = GetNumKeys();
	if (nKeysCount > 0)
	{
		m_preloadedTextures.reserve(nKeysCount);
		for (int nKeyIndex = 0; nKeyIndex < nKeysCount; ++nKeyIndex)
		{
			IScreenFaderKey key;
			GetKey(nKeyIndex, &key);
			if (key.m_strTexture[0])
			{
				ITexture* pTexture = gEnv->pRenderer->EF_LoadTexture(key.m_strTexture, FT_DONT_STREAM|FT_DONT_RESIZE|FT_STATE_CLAMP);
				if (pTexture)
				{
					pTexture->SetClamp(true);
					m_preloadedTextures.push_back(pTexture);
				}
			}
			else
			{
				m_preloadedTextures.push_back(NULL);
			}
		}
	}
}

//-----------------------------------------------------------------------------
ITexture* CScreenFaderTrack::GetTexture(int index) const
{
	return (index >= 0 && (size_t) index < m_preloadedTextures.size()) ? m_preloadedTextures[index] : 0;
}

//-----------------------------------------------------------------------------
void CScreenFaderTrack::ReleasePreloadedTextures()
{
	const size_t size = m_preloadedTextures.size();
	for (size_t i=0; i<size; ++i)
	{
		SAFE_RELEASE(m_preloadedTextures[i]);
	}
	m_preloadedTextures.resize(0);
}
