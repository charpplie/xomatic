#ifndef __CommunicationPlayer_h__
#define __CommunicationPlayer_h__

#pragma once


#include "Communication.h"


class CommunicationPlayer
{
public:
	struct ICommunicationFinishedListener
	{
		virtual void OnCommunicationFinished(const CommunicationPlayID& playID) = 0;
	};

public:
	void Clear();
	void Reset();

	bool Play(const CommunicationPlayID& playID, const SCommunicationRequest& request,
		SCommunication& comm, uint32 variationIdx, ICommunicationFinishedListener* listener, void* param);
	void Stop(const CommunicationPlayID& playID);
	void Update(float updateTime);

	bool IsPlaying(const CommunicationPlayID& playID, float* remainingTime = 0) const;

private:
	struct PlayingCommunication
		: public IAICommunicationHandler::IEventListener
	{
		enum EFinishedFlags
		{
			FinishedAnimation	= 1 << 0,
			FinishedSound			= 1 << 1,
			FinishedVoice			= 1 << 2,
			FinishedTimeout		= 1 << 3,
			FinishedAll				= FinishedAnimation | FinishedSound | FinishedVoice | FinishedTimeout,
		};

		PlayingCommunication(const SCommunicationRequest& request, const SCommunicationVariation& variation,
			ICommunicationFinishedListener* _listener = 0, void* _param = 0)
			: actorID(request.actorID)
			, soundID(INVALID_SOUNDID)
			, voiceID(INVALID_SOUNDID)
			, timeout(variation.timeout)
			, target(request.target)
			, targetID(request.targetID)
			, flags(variation.flags)
			, finishedFlags(0)
			, listener(_listener)
			, param(_param)
		{
		}

		virtual void OnCommunicationHandlerEvent(
			IAICommunicationHandler::ECommunicationHandlerEvent type, void* idOrName, EntityId actorID);

		EntityId actorID;

		tSoundID soundID;
		tSoundID voiceID;

		string animationName;

		float timeout;

		Vec3 target;
		EntityId targetID;

		uint32 flags;
		uint8 finishedFlags;

		ICommunicationFinishedListener* listener;
		void* param;
	};

	bool UpdatePlaying(PlayingCommunication& playing, float updateTime);
	void BlockingSettings(IAIObject* aiObject, PlayingCommunication& playing, bool set);

	typedef std::map<CommunicationPlayID, PlayingCommunication> PlayingCommunications;
	PlayingCommunications m_playing;
};

#endif