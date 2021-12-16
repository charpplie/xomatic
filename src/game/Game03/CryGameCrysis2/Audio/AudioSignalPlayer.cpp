#include "StdAfx.h"
#include "AudioSignalPlayer.h"
#include "Game.h"


//////////////////////////////////////////////////////////////////////////

void CAudioSignalPlayer::Play( EntityId entityID, const char* pParam, float param )
{
	const CGameAudio::CAudioSignal* pAudioSignal = g_pGame->GetGameAudio()->GetAudioSignal( m_audioSignalId );
		
	if (!pAudioSignal)
		return;

	//  stops looped sounds
	int numPlayingSounds = m_playingSoundIDs.size();
	for (uint32 i = 0; i < numPlayingSounds; ++i)
	{
		tSoundID soundID = m_playingSoundIDs[i];
		if (soundID != INVALID_SOUNDID && IsSoundLooped(soundID, entityID))
			StopSound( soundID, entityID );
	}

	// 
	m_playingSoundIDs.clear();
	m_playingSoundIDs.reserve( pAudioSignal->m_sounds.size() );

	// plays new sounds
	const uint32 soundCount = pAudioSignal->m_sounds.size();
	for (uint32 soundIndex = 0; soundIndex < soundCount; ++soundIndex)
	{
		const CGameAudio::CSound& sound = pAudioSignal->m_sounds[soundIndex];
		tSoundID soundID = PlaySound( sound.m_name, sound.m_semantic, entityID, pParam, param, sound.m_flags );
		m_playingSoundIDs.push_back( soundID );
	}
	
	PlayMoods( entityID, pAudioSignal );
}


//////////////////////////////////////////////////////////////////////////

void CAudioSignalPlayer::PlayMoods( EntityId entityID, const CGameAudio::CAudioSignal* pAudioSignal )
{
	if (pAudioSignal->m_moodCommand.IsACommand())
	{
		if (entityID==0 || g_pGame->GetIGameFramework()->GetClientActorId()==entityID)
			pAudioSignal->m_moodCommand.Execute();
	}
}


//////////////////////////////////////////////////////////////////////////

void CAudioSignalPlayer::Stop( EntityId entityID )
{
	const uint32 numPlayingSounds = m_playingSoundIDs.size();
	for (uint32 soundIndex = 0; soundIndex < numPlayingSounds; ++soundIndex)
	{
		if (m_playingSoundIDs[soundIndex] != INVALID_SOUNDID)
		{
			StopSound( m_playingSoundIDs[soundIndex], entityID );
			m_playingSoundIDs[soundIndex] = INVALID_SOUNDID;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CAudioSignalPlayer::SetSignal( const char* pName )
{
	m_audioSignalId = g_pGame->GetGameAudio()->GetSignalID( pName );
	m_playingSoundIDs.clear();
}


//////////////////////////////////////////////////////////////////////////

void CAudioSignalPlayer::SetSignal( TAudioSignalID signalID )
{
	m_audioSignalId = signalID;
	m_playingSoundIDs.clear();
}


//////////////////////////////////////////////////////////////////////////

bool CAudioSignalPlayer::IsPlaying( EntityId entityID ) const
{
	IEntitySoundProxy* pProxy = GetEntitySoundProxy( entityID );
		
	const uint32 numSounds = m_playingSoundIDs.size();
	for (int s=0; s<numSounds; ++s)
	{
		ISound* pSound = GetSoundInterface( pProxy, m_playingSoundIDs[s] );
		if (pSound)
			return true;
	}
	
	return false;
}


//////////////////////////////////////////////////////////////////////////

void CAudioSignalPlayer::SetVolume( EntityId entityID, float vol )
{
	IEntitySoundProxy* pProxy = GetEntitySoundProxy( entityID );

	const uint32 numSounds = m_playingSoundIDs.size();
	for (int s=0; s<numSounds; ++s)
	{
		ISound* pSound = GetSoundInterface( pProxy, m_playingSoundIDs[s] );
		if (pSound)
			pSound->GetInterfaceExtended()->SetVolume( vol );
	}
}


//////////////////////////////////////////////////////////////////////////

void CAudioSignalPlayer::SetParam( EntityId entityID, const char* paramName, float paramValue )
{
	IEntitySoundProxy* pProxy = GetEntitySoundProxy( entityID );
	
	const uint32 numSounds = m_playingSoundIDs.size();
	for (int s=0; s<numSounds; ++s)
	{
		ISound* pSound = GetSoundInterface( pProxy, m_playingSoundIDs[s] );
		if (pSound)
			pSound->SetParam( paramName, paramValue );
	}
}


//////////////////////////////////////////////////////////////////////////

tSoundID CAudioSignalPlayer::PlaySound( const string& name, const ESoundSemantic semantic, EntityId entityID, const char* pParam, float param, uint32 flags )
{
	ISound* pSound = gEnv->pSoundSystem->CreateSound(name.c_str(), flags);

	if(!pSound)
	{
#ifndef _RELEASE
		if(!gEnv->pSystem->IsDedicated())
		{
			CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "AudioSignalPlayer: Unable to create sound '%s'", name.c_str());
		}
#endif
		return INVALID_SOUNDID;
	}

	pSound->SetSemantic(semantic);
	if(pParam)
		pSound->SetParam(pParam, param);

	if(entityID)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityID);
		CRY_ASSERT(pEntity);
		if(pEntity)
		{
			IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*)pEntity->CreateProxy(ENTITY_PROXY_SOUND);
			CRY_ASSERT(pSoundProxy);
			if(pSoundProxy)
				pSoundProxy->PlaySound(pSound);
		}
	}
	else
	{
		pSound->Play();
	}

	return pSound->GetId();
}


//////////////////////////////////////////////////////////////////////////

tSoundID CAudioSignalPlayer::PlaySound( const string& name, const ESoundSemantic semantic, const Vec3& pos, uint32 flags )
{
	ISound* pSound = gEnv->pSoundSystem->CreateSound( name.c_str(), flags );

	if (pSound == 0)
	{
#ifndef _RELEASE
		if(!gEnv->pSystem->IsDedicated())
		{
			CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "AudioSignalPlayer: Unable to create sound '%s'", name.c_str());
		}
#endif
		return INVALID_SOUNDID;
	}

	pSound->SetSemantic( semantic );
	pSound->SetPosition( pos );
	pSound->Play();

	return pSound->GetId();
}



//////////////////////////////////////////////////////////////////////////

void CAudioSignalPlayer::StopSound( const tSoundID soundID, EntityId entityID )
{
	IEntitySoundProxy* pProxy = GetEntitySoundProxy( entityID );
	if (pProxy)
		pProxy->StopSound(soundID);
	else
	{
		ISound* pSound = gEnv->pSoundSystem->GetSound( soundID );
		if (pSound)
			pSound->Stop();
	}
}


//////////////////////////////////////////////////////////////////////////

bool CAudioSignalPlayer::IsSoundLooped( const tSoundID soundID, EntityId entityID )
{
	IEntitySoundProxy* pProxy = GetEntitySoundProxy( entityID );
	ISound* pSound = GetSoundInterface( pProxy, soundID );
	
	return (pSound && pSound->GetFlags() & FLAG_SOUND_LOOP);
}


//////////////////////////////////////////////////////////////////////////

IEntitySoundProxy* CAudioSignalPlayer::GetEntitySoundProxy( EntityId entityID ) const
{
	IEntitySoundProxy* pProxy = NULL;
	if (entityID != 0)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityID);
		if (pEntity)
			pProxy = static_cast<IEntitySoundProxy*>(pEntity->GetProxy(ENTITY_PROXY_SOUND));
	}
	
	return pProxy;
}


//////////////////////////////////////////////////////////////////////////

ISound* CAudioSignalPlayer::GetSoundInterface( IEntitySoundProxy* pProxy, tSoundID soundID ) const
{
	if (soundID != INVALID_SOUNDID)
	{
		if (pProxy)
			return pProxy->GetSound( soundID );
		else
			return gEnv->pSoundSystem->GetSound( soundID );
	}

	return NULL;
}



//////////////////////////////////////////////////////////////////////////
// static function, provides a way to play signals using just IDs. the limitation is that signals can only be played, not stopped.
// for signals that need the posibility to be stoped, need to use a real CAudioSignalPlayer object

void CAudioSignalPlayer::JustPlay( const char* signal, EntityId entityID )
{
	JustPlay(g_pGame->GetGameAudio()->GetSignalID(signal), entityID);
}

void CAudioSignalPlayer::JustPlay( TAudioSignalID signalID, EntityId entityID )
{
	const CGameAudio::CAudioSignal* pAudioSignal = g_pGame->GetGameAudio()->GetAudioSignal( signalID );

	if (!pAudioSignal)
		return;

	const uint32 soundCount = pAudioSignal->m_sounds.size();
	for (uint32 soundIndex = 0; soundIndex < soundCount; ++soundIndex)
	{
		const CGameAudio::CSound& sound = pAudioSignal->m_sounds[soundIndex];
		PlaySound( sound.m_name, sound.m_semantic, entityID, NULL, 0, sound.m_flags );
	}

	PlayMoods( entityID, pAudioSignal );
}

//////////////////////////////////////////////////////////////////////////
// when need to play the signal at a 3D pos, but cant use an entity proxy for some reason


void CAudioSignalPlayer::JustPlay( const char* signal, const Vec3& pos )
{
	JustPlay(g_pGame->GetGameAudio()->GetSignalID(signal), pos);
}

void CAudioSignalPlayer::JustPlay( TAudioSignalID signalID, const Vec3& pos )
{
	const CGameAudio::CAudioSignal* pAudioSignal = g_pGame->GetGameAudio()->GetAudioSignal( signalID );

	if (!pAudioSignal)
		return;

	const uint32 soundCount = pAudioSignal->m_sounds.size();
	for (uint32 soundIndex = 0; soundIndex < soundCount; ++soundIndex)
	{
		const CGameAudio::CSound& sound = pAudioSignal->m_sounds[soundIndex];
		PlaySound( sound.m_name, sound.m_semantic, pos, sound.m_flags );
	}

	PlayMoods( 0, pAudioSignal );
}

float CAudioSignalPlayer::GetSignalLength(TAudioSignalID signalID)
{
	float length = 0.0f;

	const CGameAudio::CAudioSignal* pAudioSignal = g_pGame->GetGameAudio()->GetAudioSignal( signalID );
	if(pAudioSignal)
	{
		const int soundsSize = pAudioSignal->m_sounds.size();
		for(int i = 0; i < soundsSize; i++)
		{
			const CGameAudio::CSound& sound = pAudioSignal->m_sounds[i];
			ISound* pSound = gEnv->pSoundSystem->CreateSound( sound.m_name.c_str(), sound.m_flags );
			if(pSound)
			{
				length = max(pSound->GetLengthMs()/1000.0f, length);
			}
		}
	}

	return length;
}

#ifndef _RELEASE
//////////////////////////////////////////////////////////////////////////
// for debug purposes
const char* CAudioSignalPlayer::GetSignalName()
{
	const CGameAudio::CAudioSignal* pAudioSignal = g_pGame->GetGameAudio()->GetAudioSignal( m_audioSignalId );

	if (!pAudioSignal)
		return NULL;
		
	return pAudioSignal->m_signalName.c_str();
}
#endif