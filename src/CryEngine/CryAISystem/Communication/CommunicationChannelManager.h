#ifndef __CommunicationChannelManager_h__
#define __CommunicationChannelManager_h__

#pragma once


#include "Communication.h"
#include "CommunicationChannel.h"


class CommunicationChannelManager
{
public:
	bool LoadChannel(const XmlNodeRef& channelNode, const CommunicationChannelID& parentID = 0);

	void Clear();
	void Reset();
	void Update(float updateTime);

	CommunicationChannelID GetChannelID(const char* name) const;
	CommunicationChannel::Ptr GetChannel(const CommunicationChannelID& channelID, EntityId sourceId) const;
	CommunicationChannel::Ptr GetChannel(const CommunicationChannelID& channelID, EntityId sourceId);

private:
	typedef std::map<CommunicationChannelID, SCommunicationChannelParams> ChannelParams;
	ChannelParams m_params;


	typedef std::map<CommunicationChannelID, CommunicationChannel::Ptr> Channels;
	Channels m_globalChannels;

	typedef std::map<int, Channels> GroupChannels;
	GroupChannels m_groupChannels;

	typedef std::map<EntityId, Channels> PersonalChannels;
	PersonalChannels m_personalChannels;
};

#endif //__CommunicationChannelManager_h__