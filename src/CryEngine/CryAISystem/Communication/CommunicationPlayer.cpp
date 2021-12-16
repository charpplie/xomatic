#include "StdAfx.h"
#include "CommunicationPlayer.h"
#include "Puppet.h"



void CommunicationPlayer::PlayingCommunication::OnCommunicationHandlerEvent(
	IAICommunicationHandler::ECommunicationHandlerEvent event, void* idOrName, EntityId actorID)
{
	switch (event)
	{
	case IAICommunicationHandler::ActionStarted:
	case IAICommunicationHandler::SignalStarted:
	case IAICommunicationHandler::SoundStarted:
	case IAICommunicationHandler::VoiceStarted:
	//case IAICommunicationHandler::ActionCancelled:
		break;
	case IAICommunicationHandler::ActionFailed:
	case IAICommunicationHandler::SignalFailed:
	case IAICommunicationHandler::SignalCancelled:
	case IAICommunicationHandler::SignalFinished:
		finishedFlags |= FinishedAnimation;
		break;
	case IAICommunicationHandler::SoundFailed:
	case IAICommunicationHandler::SoundFinished:
		finishedFlags |= FinishedSound;
		break;
	case IAICommunicationHandler::VoiceFailed:
	case IAICommunicationHandler::VoiceFinished:
		finishedFlags |= FinishedVoice;
		break;
	default:
		break;
	}
}

void CommunicationPlayer::Reset()
{
	PlayingCommunications::iterator it = m_playing.begin();
	PlayingCommunications::iterator end = m_playing.end();

	for ( ; it != end; ++it)
	{
		PlayingCommunication& playing = it->second;

		IEntity* entity = gEnv->pEntitySystem->GetEntity(playing.actorID);
		if (!entity)
			continue;

		IAIObject* aiObject = entity->GetAI();
		if (!aiObject)
			continue;

		IAIActorProxy* aiProxy = aiObject->GetProxy();
		if (!aiProxy)
			continue;

		IAICommunicationHandler* commHandler = aiProxy->GetCommunicationHandler();

		if (!playing.animationName.empty())
		{
			IAICommunicationHandler::EAnimationMethod method = (playing.flags & SCommunication::AnimationAction) 
				? IAICommunicationHandler::AnimationMethodAction : IAICommunicationHandler::AnimationMethodSignal;

			commHandler->StopAnimation(playing.animationName.c_str(), method);
		}
		
		if (playing.soundID != INVALID_SOUNDID)
			commHandler->StopSound(playing.soundID);

		if (playing.voiceID != INVALID_SOUNDID)
			commHandler->StopVoice(playing.voiceID);
	}

	m_playing.clear();
}

void CommunicationPlayer::Clear()
{
	Reset();
}

void CommunicationPlayer::BlockingSettings(IAIObject* aiObject, PlayingCommunication& playing, bool set)
{
	if (playing.flags & SCommunication::BlockAll)
	{
		if (playing.flags & SCommunication::BlockMovement)
			if (CPipeUser* pipeUser = aiObject->CastToCPipeUser())
				pipeUser->Pause(set);

		if (playing.flags & SCommunication::BlockFire)
			if (CPuppet* puppet = aiObject->CastToCPuppet())
				puppet->EnableFire(!set);
	}
}

bool CommunicationPlayer::Play(const CommunicationPlayID& playID, const SCommunicationRequest& request,
															 SCommunication& comm, uint32 variationIdx, ICommunicationFinishedListener* listener,
															 void* param)
{
	// Only return false when playing could not happen due to congestion
	// If case of unrecoverable error return true to avoid the queue getting full

	bool failed = false;

	IEntity* entity = gEnv->pEntitySystem->GetEntity(request.actorID);
	if (!entity)
		failed = true;

	IAIObject* aiObject = entity->GetAI();
	if (!aiObject)
		failed = true;

	IAIActorProxy* aiProxy = aiObject->GetProxy();
	if (!aiProxy)
		failed = true;

	assert(variationIdx < comm.variations.size());
	SCommunicationVariation& variation = comm.variations[variationIdx];

	PlayingCommunication& playing = stl::map_insert_or_get(m_playing, playID, 
		PlayingCommunication(request, variation, listener, param));

	if (failed)
	{
		playing.finishedFlags = PlayingCommunication::FinishedAll;

		return true;
	}

	assert(playing.soundID == INVALID_SOUNDID);
	assert(playing.voiceID == INVALID_SOUNDID);
	assert(playing.animationName.empty());

	IAICommunicationHandler* commHandler = aiProxy->GetCommunicationHandler();

	if (!variation.soundName.empty())
		playing.soundID = commHandler->PlaySound(variation.soundName.c_str(),
			(variation.flags & SCommunication::FinishSound) ? &playing : 0);
	else
		playing.finishedFlags |= PlayingCommunication::FinishedSound;

	if (!variation.voiceName.empty())
		playing.voiceID = commHandler->PlayVoice(variation.voiceName.c_str(),
			(variation.flags & SCommunication::FinishVoice) ? &playing : 0);
	else
		playing.finishedFlags |= PlayingCommunication::FinishedVoice;

	if (!variation.animationName.empty())
	{
		playing.animationName = variation.animationName;

		IAICommunicationHandler::EAnimationMethod method = (variation.flags & SCommunication::AnimationAction)
			? IAICommunicationHandler::AnimationMethodAction : IAICommunicationHandler::AnimationMethodSignal;

		IAICommunicationHandler::IEventListener* listener = (playing.flags & SCommunication::FinishAnimation) ? &playing : 0;

		commHandler->PlayAnimation(variation.animationName.c_str(), method, listener);
	}
	else
		playing.finishedFlags |= PlayingCommunication::FinishedAnimation;

	if (playing.timeout <= 0.000001f)
		playing.finishedFlags |= PlayingCommunication::FinishedTimeout;

	BlockingSettings(aiObject, playing, true);

	return true;
}

void CommunicationPlayer::Update(float updateTime)
{
	PlayingCommunications::iterator it = m_playing.begin();
	PlayingCommunications::iterator end = m_playing.end();

	for ( ; it != end; )
	{
		PlayingCommunication& playing = it->second;
		
		if (!UpdatePlaying(playing, updateTime))
		{
			if (playing.listener)
				playing.listener->OnCommunicationFinished(it->first);

			PlayingCommunications::iterator erased = it++;
			m_playing.erase(erased);

			continue;
		}
		++it;
	}
}

bool CommunicationPlayer::UpdatePlaying(PlayingCommunication& playing, float updateTime)
{
	// Only return false when playing could not happen due to congestion
	// If case of unrecoverable error return true to avoid the queue getting full
	IEntity* entity = gEnv->pEntitySystem->GetEntity(playing.actorID);
	if (!entity)
		return false;

	IAIObject* aiObject = entity->GetAI();
	if (!aiObject)
		return false;

	if (playing.flags & SCommunication::LookAtTarget)
	{
		if (playing.targetID)
		{
			if (IEntity* targetEntity = gEnv->pEntitySystem->GetEntity(playing.targetID))
				playing.target = targetEntity->GetWorldPos();
		}

		if (!playing.target.IsZeroFast())
		{
			if (CPipeUser* pipeUser = aiObject->CastToCPipeUser())
			{
				pipeUser->SetLookStyle(LOOKSTYLE_HARD_NOLOWER);
				pipeUser->SetLookAtPointPos(playing.target, true);
			}
		}
	}

	if (playing.timeout > 0.0f)
	{
		playing.timeout -= updateTime;
		if (playing.timeout <= 0.0f)
			playing.finishedFlags |= PlayingCommunication::FinishedTimeout;
	}

	bool finished = ((playing.flags & SCommunication::FinishAnimation) == 0) 
		|| (playing.finishedFlags & PlayingCommunication::FinishedAnimation);

	finished = finished && (((playing.flags & SCommunication::FinishSound) == 0) 
		|| (playing.finishedFlags & PlayingCommunication::FinishedSound));

	finished = finished && (((playing.flags & SCommunication::FinishVoice) == 0) 
		|| (playing.finishedFlags & PlayingCommunication::FinishedVoice));

	finished = finished && (((playing.flags & SCommunication::FinishTimeout) == 0) 
		|| (playing.finishedFlags & PlayingCommunication::FinishedTimeout));

	if (finished)
	{
		if ((playing.flags & SCommunication::AnimationAction) && ((playing.finishedFlags & PlayingCommunication::FinishedAnimation) == 0))
		{
			IAIActorProxy* aiProxy = aiObject->GetProxy();
			if (!aiProxy)
				return false;

			IAICommunicationHandler* commHandler = aiProxy->GetCommunicationHandler();

			IAICommunicationHandler::EAnimationMethod method = (playing.flags & SCommunication::AnimationAction)
				? IAICommunicationHandler::AnimationMethodAction : IAICommunicationHandler::AnimationMethodSignal;

			commHandler->StopAnimation(playing.animationName.c_str(), method);

			playing.finishedFlags |= PlayingCommunication::FinishedAnimation;
		}

		BlockingSettings(aiObject, playing, false);

		return false;
	}

	return true;
}

void CommunicationPlayer::Stop(const CommunicationPlayID& playID)
{
	PlayingCommunications::iterator it = m_playing.find(playID);
	if (it != m_playing.end())
	{
		PlayingCommunication& playing = it->second;

		bool finished = ((playing.flags & SCommunication::FinishAnimation) == 0) 
			|| (playing.finishedFlags & PlayingCommunication::FinishedAnimation);
		finished = finished && (((playing.flags & SCommunication::FinishSound) == 0)
			|| (playing.finishedFlags & PlayingCommunication::FinishedSound));
		finished = finished && (((playing.flags & SCommunication::FinishVoice) == 0) 
			|| (playing.finishedFlags & PlayingCommunication::FinishedVoice));
		finished = finished && (((playing.flags & SCommunication::FinishTimeout) == 0) 
			|| (playing.finishedFlags & PlayingCommunication::FinishedTimeout));

		if (!finished)
		{
			IEntity* entity = gEnv->pEntitySystem->GetEntity(playing.actorID);
			IAIObject* aiObject = entity ? entity->GetAI() : 0;
			
			if (IAIActorProxy* aiProxy = aiObject ? aiObject->GetProxy() : 0)
			{
				IAICommunicationHandler* commHandler = aiProxy->GetCommunicationHandler();

				if ((playing.soundID != INVALID_SOUNDID) && ((playing.finishedFlags & PlayingCommunication::FinishedSound) == 0))
					commHandler->StopSound(playing.soundID);

				if ((playing.voiceID != INVALID_SOUNDID) && ((playing.finishedFlags & PlayingCommunication::FinishedVoice) == 0))
					commHandler->StopSound(playing.voiceID);

				if ((playing.flags & SCommunication::AnimationAction) && ((playing.finishedFlags & PlayingCommunication::FinishedAnimation) == 0))
				{
					IAICommunicationHandler::EAnimationMethod method = (playing.flags & SCommunication::AnimationAction) 
						? IAICommunicationHandler::AnimationMethodAction : IAICommunicationHandler::AnimationMethodSignal;

					commHandler->StopAnimation(playing.animationName.c_str(), method);
				}
			}

			BlockingSettings(aiObject, playing, false);
		}

		if (playing.listener)
			playing.listener->OnCommunicationFinished(playID);

		m_playing.erase(it);
	}
}

bool CommunicationPlayer::IsPlaying(const CommunicationPlayID& playID, float* remainingTime) const
{
	PlayingCommunications::const_iterator it = m_playing.find(playID);
	if (it != m_playing.end())
		return true;

	return false;
}