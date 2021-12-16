#ifndef __Communication_h__
#define __Communication_h__

#pragma once


#include <ICommunicationManager.h>


struct SCommunicationChannelParams
{
	enum ECommunicationChannelType
	{
		Global = 0,
		Group,
		Personal,
	};

	float minSilence;

	string name;
	CommunicationChannelID parentID;
	ECommunicationChannelType type;
};


struct SCommunicationVariation
{
	SCommunicationVariation()
		: timeout(0.0f)
		, flags(0)
	{
	}

	string animationName;
	string soundName;
	string voiceName;

	float timeout;

	uint32 flags;
};


struct SCommunication
{
	enum EVariationChoiceMethod
	{
		Random = 0,
		Sequence,
		RandomSequence,
		Match, // only valid for responses
	};

	enum ECommunicationFlags
	{
		LookAtTarget		= 1 << 0,

		FinishAnimation	= 1 << 8,
		FinishSound			= 1 << 9,
		FinishVoice			= 1 << 10,
		FinishTimeout		= 1 << 11,
		FinishAll				= FinishAnimation | FinishSound | FinishVoice | FinishTimeout,

		BlockMovement		= 1 << 12,
		BlockFire				= 1 << 13,
		BlockAll				= BlockMovement | BlockFire,

		AnimationAction	= 1 << 16,
	};

	string name;
	CommunicationID responseID;

	EVariationChoiceMethod choiceMethod;
	EVariationChoiceMethod responseChoiceMethod;

	struct History
	{
		History()
			: played(0)
		{
		}

		uint32 played;
	} history;

	std::vector<SCommunicationVariation> variations;
};


#endif __Communication_h__