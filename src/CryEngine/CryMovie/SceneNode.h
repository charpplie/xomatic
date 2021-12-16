////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   scenenode.h
//  Version:     v1.00
//  Created:     23/4/2002 by Lennert.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __scenenode_h__
#define __scenenode_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "AnimNode.h"
#include "SoundTrack.h"

#define SCENE_SOUNDTRACKS 3

struct ISound;
class CGototTrack;

class CAnimSceneNode : public CAnimNode
{
public:
	CAnimSceneNode();
	~CAnimSceneNode();

	virtual EAnimNodeType GetType() const { return ANODE_SCENE; }

	//////////////////////////////////////////////////////////////////////////
	// Overrides from CAnimNode
	//////////////////////////////////////////////////////////////////////////
	void Animate( SAnimContext &ec );
	void CreateDefaultTracks();

	virtual void Activate( bool bActivate );

	// ovverided from IAnimNode
	void Reset();
	void Pause();

	//////////////////////////////////////////////////////////////////////////
	virtual int GetParamCount() const;
	virtual bool GetParamInfo( int nIndex, SParamInfo &info ) const;
	virtual bool GetParamInfoFromId( int paramId, SParamInfo &info ) const;

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		for(int i = 0; i < SCENE_SOUNDTRACKS ; ++i )
			pSizer->AddObject(m_SoundInfo[i]);

		CAnimNode::GetMemoryUsage(pSizer);
	}
private:
	void ReleaseSounds();
	void ApplyCameraKey( ISelectKey &key,SAnimContext &ec );
	void ApplyEventKey(IEventKey &key, SAnimContext &ec);
	void ApplyConsoleKey(IConsoleKey &key, SAnimContext &ec);
	void ApplySoundKey( IAnimTrack *pTrack,int nCurrKey,int nLayer, ISoundKey &key, SAnimContext &ec);
	void ApplySequenceKey( IAnimTrack *pTrack,int nPrevKey,int nCurrKey,ISequenceKey &key,SAnimContext &ec );
	void ApplyMusicKey(IMusicKey &key, SAnimContext &ec);

	void ApplyGotoKey(CGototTrack*	poGotoTrack,SAnimContext &ec);

	// Cached parameters of node at given time.
	float m_time;

	IMovieSystem *m_pMovie;

	bool m_bActive;

	bool m_bMusicMoodSet;

	//! Last animated key in track.
	int m_lastCameraKey;
	int m_lastEventKey;
	int m_lastConsoleKey;
	int m_lastMusicKey;
	int m_lastSequenceKey;
	int m_nLastGotoKey;
	int m_lastCaptureKey;
	bool m_bLastCapturingEnded;

	EntityId m_currentCameraEntityId;

	SSoundInfo m_SoundInfo[SCENE_SOUNDTRACKS];
};

#endif // __entitynode_h__
