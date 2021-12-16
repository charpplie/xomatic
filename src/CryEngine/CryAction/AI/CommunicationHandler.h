#ifndef __CommunicationHandler_h__
#define __CommunicationHandler_h__


#pragma once


#include <IAgent.h>
#include "CommunicationVoiceLibrary.h"


class CommunicationHandler
	: public IAICommunicationHandler
	, public ISoundEventListener
	, public IAnimationGraphStateListener
{
public:
	CommunicationHandler(CAIProxy& proxy, IEntity& entity);
	virtual ~CommunicationHandler();

	// IAICommunicationHandler
	virtual tSoundID PlaySound(const char* name, IEventListener* listener = 0);
	virtual void StopSound(tSoundID soundID);

	virtual tSoundID PlayVoice(const char* name, IEventListener* listener = 0);
	virtual void StopVoice(tSoundID soundID);

	virtual void PlayAnimation(const char* name, EAnimationMethod method, IEventListener* listener = 0);
	virtual void StopAnimation(const char* name, EAnimationMethod method);
	//~IAICommunicationHandler

	// ISoundEventListener
	virtual void OnSoundEvent(ESoundCallbackEvent event, ISound* sound);
	//~ISoundEventListener

	// IAnimationgGraphStateListener
	virtual void SetOutput(const char* output, const char* value) {};
	virtual void QueryComplete( TAnimationGraphQueryID queryID, bool succeeded );
	virtual void DestroyedState(IAnimationGraphState*);
	//~IAnimationgGraphStateListener

	void Reset();

private:
	enum ESoundType
	{
		Sound = 0,
		Voice,
	};

	tSoundID PlaySound(const char* name, ESoundType type, IEventListener* listener = 0);
	IAnimationGraphState* GetAGState();

	CAIProxy& m_proxy;
	IEntity& m_entity;
	IAnimationGraphState* m_agState;

	struct PlayingSound
	{
		PlayingSound()
			: listener(0)
			, type(Sound)
		{
		}

		ESoundType type;
		IAICommunicationHandler::IEventListener* listener;
	};

	struct PlayingAnimation
	{
		PlayingAnimation()
			: listener(0)
			, playing(false)
		{
		}

		IAICommunicationHandler::IEventListener* listener;
		string name;

		EAnimationMethod method;
		bool playing;
	};

	typedef std::map<TAnimationGraphQueryID, PlayingAnimation> PlayingAnimations;
	PlayingAnimations m_playingAnimations;
	TAnimationGraphQueryID m_currentQueryID;	// because animation graph can send query result during SetInput, 
	bool m_currentPlaying;										// before we had chance to insert in the map

	typedef std::map<tSoundID, PlayingSound> PlayingSounds;
	PlayingSounds m_playingSounds;

	VoiceLibraryID m_voiceLibraryID;

	IAnimationGraph::InputID m_signalInputID;
	IAnimationGraph::InputID m_actionInputID;
};

#endif