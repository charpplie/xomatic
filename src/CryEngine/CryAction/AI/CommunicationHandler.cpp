#include "StdAfx.h"
#include "CommunicationHandler.h"
#include "CommunicationVoiceLibrary.h"
#include "AIProxy.h"


CommunicationHandler::CommunicationHandler(CAIProxy& proxy, IEntity& entity)
: m_proxy(proxy)
, m_entity(entity)
, m_agState(0)
, m_currentQueryID(0)
, m_currentPlaying(0)
, m_signalInputID(0)
, m_actionInputID(0)
{
}

CommunicationHandler::~CommunicationHandler()
{
	if (m_agState)
		m_agState->RemoveListener(this);
}

void CommunicationHandler::Reset()
{
	//TODO(márcio): We might have to send cancelled events to everyone still listening
	m_playingAnimations.clear();
	m_playingSounds.clear();

	m_voiceLibraryID = CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()
		->GetVoiceLibraryID(m_proxy.GetVoiceLibraryName());
}

tSoundID CommunicationHandler::PlaySound(const char* name, IEventListener* listener)
{
	return PlaySound(name, Sound, listener);
}

void CommunicationHandler::StopSound(tSoundID soundID)
{
	IEntitySoundProxy* soundProxy = static_cast<IEntitySoundProxy*>(m_entity.GetProxy(ENTITY_PROXY_SOUND));
	if (!soundProxy)
		return;

	soundProxy->StopSound(soundID);

	PlayingSounds::iterator it = m_playingSounds.find(soundID);
	if (it != m_playingSounds.end())
		m_playingSounds.erase(it);
}

tSoundID CommunicationHandler::PlayVoice(const char* name, IEventListener* listener)
{
	const char* path = 0;
	const char* fileName = 0;
	
	if (!CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoice(m_voiceLibraryID, name, path, fileName))
	{
		if (listener)
			listener->OnCommunicationHandlerEvent(VoiceFailed, (void*)name, m_entity.GetId());

		return INVALID_SOUNDID;
	}

	stack_string completeName(path);
	completeName.append(fileName);

	return PlaySound(completeName.c_str(), Voice, listener);
}

void CommunicationHandler::StopVoice(tSoundID soundID)
{
	StopSound(soundID);
}

void CommunicationHandler::PlayAnimation(const char* name, EAnimationMethod method, IEventListener* listener)
{
	bool ok = false;

	if (GetAGState())
	{
		IAnimationGraph::InputID inputID = method == AnimationMethodAction ? m_actionInputID : m_signalInputID;
		if (!listener)
			ok = m_agState->SetInput(inputID, name);
		else
		{
			if (ok = m_agState->SetInput(inputID, name, &m_currentQueryID))
			{
				std::pair<PlayingAnimations::iterator, bool> iresult = m_playingAnimations.insert(
					PlayingAnimations::value_type(m_currentQueryID, PlayingAnimation()));

				PlayingAnimation& playingAnimation = iresult.first->second;
				playingAnimation.listener = listener;
				playingAnimation.name = name;
				playingAnimation.method = method;
				playingAnimation.playing = m_currentPlaying;
			}
			m_currentPlaying = false;
			m_currentQueryID = 0;
		}
	}

	if (!ok && listener)
		listener->OnCommunicationHandlerEvent(
		(method == AnimationMethodAction) ? ActionFailed : SignalFailed, (void*)name, m_entity.GetId());
}

void CommunicationHandler::StopAnimation(const char* name, EAnimationMethod method)
{
	if (method == AnimationMethodAction)
	{
		if (GetAGState())
			m_agState->SetInput(m_actionInputID, "idle");
	}

	PlayingAnimations::iterator it = m_playingAnimations.begin();
	PlayingAnimations::iterator end = m_playingAnimations.end();

	for ( ; it != end; ++it)
	{
		PlayingAnimation& playing = it->second;
		
		if (playing.name == name)
			m_playingAnimations.erase(it);

		return;
	}
}

void CommunicationHandler::OnSoundEvent(ESoundCallbackEvent event, ISound* sound)
{
	switch (event)
	{
	case SOUND_EVENT_ON_LOAD_FAILED:
	case SOUND_EVENT_ON_START:
	case SOUND_EVENT_ON_STOP:
		{
			PlayingSounds::iterator it = m_playingSounds.find(sound->GetId());
			if (it == m_playingSounds.end())
				return;

			PlayingSound& playing = it->second;
			if (playing.listener)
			{

				ECommunicationHandlerEvent outEvent;
				switch (event)
				{
				case SOUND_EVENT_ON_LOAD_FAILED:
					outEvent = (playing.type == Sound) ? SoundFailed : VoiceFailed;
					break;
				case SOUND_EVENT_ON_START:
					outEvent = (playing.type == Sound) ? SoundStarted : VoiceStarted;
					break;
				case SOUND_EVENT_ON_STOP:
					outEvent = (playing.type == Sound) ? SoundFinished : VoiceFinished;
					break;
				default:
					break;
				}

				playing.listener->OnCommunicationHandlerEvent(outEvent, (void*)sound->GetId(), m_entity.GetId());
			}
		}
		break;
	default:
		break;
	};
}

tSoundID CommunicationHandler::PlaySound(const char* name, ESoundType type, IEventListener* listener)
{
	if (ISound *sound = gEnv->pSoundSystem->CreateSound(name, 
		FLAG_SOUND_DEFAULT_3D | ((type == Voice) ? FLAG_SOUND_VOICE : 0) | (listener ? FLAG_SOUND_EVENT : 0)))
	{
		IEntitySoundProxy* soundProxy = static_cast<IEntitySoundProxy*>(m_entity.GetProxy(ENTITY_PROXY_SOUND));
		if (!soundProxy)
		{
			m_entity.CreateProxy(ENTITY_PROXY_SOUND);
			soundProxy = static_cast<IEntitySoundProxy*>(m_entity.GetProxy(ENTITY_PROXY_SOUND));
		}

		sound->AddEventListener(this, "CommunicationHandler");
		sound->SetSemantic(eSoundSemantic_AI_Readability);

		soundProxy->PlaySound(sound, ZERO, FORWARD_DIRECTION);

		tSoundID soundID = sound->GetId();

		if (listener)
		{
			std::pair<PlayingSounds::iterator, bool> iresult = m_playingSounds.insert(
				PlayingSounds::value_type(soundID, PlayingSound()));

			PlayingSound& playingSound = iresult.first->second;
			playingSound.listener = listener;
			playingSound.type = type;
		}

		return soundID;
	}
	
	if (listener)
	{
		ECommunicationHandlerEvent event = (type == Sound) ? SoundFailed : VoiceFailed;
		listener->OnCommunicationHandlerEvent(event, 0, m_entity.GetId());
	}

	return INVALID_SOUNDID;
}

IAnimationGraphState* CommunicationHandler::GetAGState()
{
	if (m_agState)
		return m_agState;

	if (IActor* actor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(m_entity.GetId()))
	{
		if (m_agState = actor->GetAnimationGraphState())
		{
			m_agState->AddListener("AICommunicationHandler", this);
			m_signalInputID = m_agState->GetInputId("Signal");
			m_actionInputID = m_agState->GetInputId("Action");
		}
	}	

	return m_agState;
}

void CommunicationHandler::QueryComplete(TAnimationGraphQueryID queryID, bool succeeded)
{
	if (queryID == m_currentQueryID) // this call happened during SetInput
	{
		m_currentPlaying = true;
		m_agState->QueryLeaveState(&m_currentQueryID);

		return;
	}

	PlayingAnimations::iterator animationIt = m_playingAnimations.find(queryID);
	if (animationIt != m_playingAnimations.end())
	{
		PlayingAnimation& playingAnimation = animationIt->second;
		if (!playingAnimation.playing)
		{
			ECommunicationHandlerEvent event;

			if (playingAnimation.method == AnimationMethodAction)
				event = succeeded ? ActionStarted : ActionFailed;
			else
				event = succeeded ? SignalStarted : SignalFailed;

			if (playingAnimation.listener)
				playingAnimation.listener->OnCommunicationHandlerEvent(event, (void*)playingAnimation.name.c_str(), m_entity.GetId());

			if (succeeded)
			{
				playingAnimation.playing = true;

				TAnimationGraphQueryID leaveQueryID;
				m_agState->QueryLeaveState(&leaveQueryID);

				m_playingAnimations.insert(PlayingAnimations::value_type(leaveQueryID, playingAnimation));
			}
		}
		else
		{
			ECommunicationHandlerEvent event;

			if (playingAnimation.method == AnimationMethodAction)
				event = ActionCancelled;
			else
				event = succeeded ? SignalFinished : SignalCancelled;

			if (playingAnimation.listener)
				playingAnimation.listener->OnCommunicationHandlerEvent(event, (void*)playingAnimation.name.c_str(), m_entity.GetId());
		}

		m_playingAnimations.erase(animationIt);
	}
}

void CommunicationHandler::DestroyedState(IAnimationGraphState* agState)
{
	if (agState == m_agState)
		m_agState = 0;
}