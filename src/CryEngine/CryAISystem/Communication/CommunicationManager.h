#ifndef __CommunicationManager_h__
#define __CommunicationManager_h__

#pragma once


#include "Communication.h"
#include "CommunicationChannelManager.h"
#include "CommunicationPlayer.h"

#include <ICommunicationManager.h>


class CCommunicationManager
	: public CommunicationPlayer::ICommunicationFinishedListener
	, public ICommunicationManager
{
public:
	enum
	{
		MaxVariationCount = 32,
	};

public:
	CCommunicationManager();

	// ICommunicationManager
	virtual uint32 GetConfigCount() const;
	virtual const char* GetConfigName(uint32 index) const;
	//~ICommunicationManager

	void ScanFolder(const char* folderName, bool recursing = false);
	bool LoadCommunicationSettingsXML(const char* fileName);
	bool LoadCommunication(const XmlNodeRef& commNode, SCommunication& comm);
	bool LoadVariation(const XmlNodeRef& varNode, SCommunicationVariation& variation);

	void Reload();

	void Reset();
	void ResetHistory();
	void Update(float updateTime);

	void DebugDraw();

	CommunicationChannelID GetChannelID(const char* name) const;
	CommunicationConfigID GetConfigID(const char* name) const;
	CommunicationID GetCommunicationID(const char* name) const;
	const char* GetCommunicationName(const CommunicationID& communicationID) const;

	bool CanCommunicationPlay(const SCommunicationRequest& request, float* estimatedWaitTime = 0);
	CommunicationPlayID PlayCommunication(const SCommunicationRequest& request);
	void StopCommunication(const CommunicationPlayID& playID);
	bool IsPlaying(const CommunicationPlayID& playID, float* timeRemaining = 0) const;
	bool IsQueued(const CommunicationPlayID& playID, float* estimatedWaitTime = 0) const;

	// ICommunicationFinishedListener
	virtual void OnCommunicationFinished(const CommunicationPlayID& playID);
	//~ICommunicationFinishedListener

private:
	bool AutoGenerateCommunication(const XmlNodeRef& commNode, std::vector<SCommunication>& comms);
	uint32 ChooseVariation(const SCommunication& comm, SCommunication::EVariationChoiceMethod method, uint32 index) const;
	void UpdateHistory(SCommunication& comm, uint32 variationPlayed);

	bool Play(const CommunicationPlayID& playID, const SCommunicationRequest& request, 
		const CommunicationChannel::Ptr& channel, SCommunication& comm);

	struct QueuedCommunication
	{
		CommunicationPlayID playID;
		SCommunicationRequest request;
		float age;
	};
	typedef std::list<QueuedCommunication> QueuedCommunications;

	void Queue(QueuedCommunications& queue, const CommunicationPlayID& playID, const SCommunicationRequest& request);
	bool RemoveFromQueue(QueuedCommunications& queue, const CommunicationPlayID& playID);
	void UpdateQueue(QueuedCommunications& queue, float updateTime);
	void ProcessQueues();

	typedef std::vector<string> FolderNames;
	FolderNames m_folderNames;

	typedef std::map<CommunicationID, SCommunication> Communications;
	struct CommunicationsConfig
	{
		string name;
		Communications comms;
	};

	typedef std::map<CommunicationConfigID, CommunicationsConfig> Configs;
	Configs m_configs;

	struct PlayingCommunication
	{
		EntityId actorID;
		CommunicationChannel::Ptr channel;
		ICommunicationManager::IEventListener* eventListener;
	};

	typedef std::map<CommunicationPlayID, PlayingCommunication> PlayingCommunications;
	PlayingCommunications m_playing;
	
	CommunicationChannelManager m_channels;
	CommunicationPlayer m_player;

	QueuedCommunications m_orderedQueue;
	QueuedCommunications m_unorderedQueue;

	CommunicationPlayID m_playGenID;
	mutable std::vector<uint8> m_randomPool;
};


#endif // __CommunicationManager_h__