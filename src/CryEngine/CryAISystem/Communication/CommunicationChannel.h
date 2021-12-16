#ifndef __CommunicationChannel_h__
#define __CommunicationChannel_h__

#pragma once


#include "Communication.h"


class CommunicationChannel
	: public _reference_target_t
{
public:
	typedef _smart_ptr<CommunicationChannel> Ptr;

public:
	CommunicationChannel(const CommunicationChannel::Ptr& parent, const SCommunicationChannelParams& params);

	void Update(float updateTime);
	void Occupy(bool occupy);

	bool IsFree() const;
	void Clear();

	CommunicationChannelID GetID();

private:
	CommunicationChannel();

	CommunicationChannelID m_id;
	CommunicationChannel::Ptr m_parent;

	const float m_minSilence;

	float m_silence;
	uint8 m_occupied;
};

#endif //__CommunicationChannel_h__