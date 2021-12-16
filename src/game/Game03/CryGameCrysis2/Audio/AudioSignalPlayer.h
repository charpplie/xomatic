#pragma once

#ifndef AUDIO_SIGNAL_PLAYER
#define AUDIO_SIGNAL_PLAYER

#include "GameAudio.h"


class CAudioSignalPlayer
{
public:
	CAudioSignalPlayer()
	: m_audioSignalId( INVALID_AUDIOSIGNAL_ID )
	{}
	
	// TODO: only Play should need the entityId param.

	void SetSignal( const char* pName );
	void SetSignal( TAudioSignalID signalID );
	void Play( EntityId entityID = 0, const char* pParam = NULL, float param = 0 );
	void Play( const Vec3& pos );
	void Stop( EntityId entityID = 0 );
	bool IsPlaying( EntityId entityID = 0 ) const;
	void SetVolume( EntityId entityID, float vol );
	void SetParam( EntityId entityID, const char* paramName, float paramValue );
	bool HasValidSignal() const { return m_audioSignalId!=INVALID_AUDIOSIGNAL_ID; }
	
	static void JustPlay( TAudioSignalID signalID, EntityId entityID = 0 );
	static void JustPlay( const char* signal, EntityId entityID = 0 );
	static void JustPlay( TAudioSignalID signalID, const Vec3& pos );
	static void JustPlay( const char* signal, const Vec3& pos );

	static float GetSignalLength(TAudioSignalID signalID);
	#ifndef _RELEASE
	const char* GetSignalName();  // for debug purposes
	#endif

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(m_playingSoundIDs);
	}
private:	
	static tSoundID PlaySound( const string& name, const ESoundSemantic semantic, EntityId entityID, const char* pParam, float param, uint32 flags = FLAG_SOUND_EVENT );
	static tSoundID PlaySound( const string& name, const ESoundSemantic semantic, const Vec3& pos , uint32 flags = FLAG_SOUND_EVENT );
	void StopSound( const tSoundID soundID, EntityId entityID );
	bool IsSoundLooped( const tSoundID soundID, EntityId entityID );
	IEntitySoundProxy* GetEntitySoundProxy( EntityId entityID ) const;
	ISound* GetSoundInterface( IEntitySoundProxy* pProxy, tSoundID soundID ) const;	
	static void PlayMoods( EntityId entityID, const CGameAudio::CAudioSignal* pAudioSignal );
	
private:
	TAudioSignalID m_audioSignalId;
	std::vector<tSoundID> m_playingSoundIDs;
};

#endif
