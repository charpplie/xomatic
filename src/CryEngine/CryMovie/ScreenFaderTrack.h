////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: ScreenFaderTrack.h
//  Version:   v1.00
//  Created:   28-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __SCREENFADERTRACK_H__
#define __SCREENFADERTRACK_H__

#pragma once

#include "IMovieSystem.h"
#include "AnimTrack.h"

class CScreenFaderTrack : public TAnimTrack<IScreenFaderKey>
{
public:
	//-----------------------------------------------------------------------------
	//!
	CScreenFaderTrack();
	~CScreenFaderTrack();

	//-----------------------------------------------------------------------------
	//! IAnimTrack Method Overriding.
	//-----------------------------------------------------------------------------
	virtual EAnimTrackType GetType() { return ATRACK_SCREENFADER; }
	virtual EAnimValue     GetValueType() { return AVALUE_SCREENFADER; }

	virtual void GetKeyInfo( int key,const char* &description,float &duration );
	virtual void SerializeKey( IScreenFaderKey &key,XmlNodeRef &keyNode,bool bLoading );

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const;

public:
	void PreloadTextures();
	ITexture* GetTexture(int index) const;

private:
	void ReleasePreloadedTextures();

private:
	std::vector<ITexture*> m_preloadedTextures;
};

#endif//__SCREENFADERTRACK_H__
