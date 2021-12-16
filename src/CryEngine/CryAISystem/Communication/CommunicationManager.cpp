#include "StdAfx.h"
#include "CommunicationManager.h"

#include <IGame.h>
#include <IGameFramework.h>
#include <ICommunicationVoiceLibrary.h>
#include <StringUtils.h>


CCommunicationManager::CCommunicationManager()
: m_playGenID(0)
{
	m_randomPool.reserve(MaxVariationCount);
}

uint32 CCommunicationManager::GetConfigCount() const
{
	return m_configs.size();
}

const char* CCommunicationManager::GetConfigName(uint32 index) const
{
	assert(index < m_configs.size());

	Configs::const_iterator it = m_configs.begin();
	std::advance(it, index);

	return it->second.name.c_str();
}

void CCommunicationManager::ScanFolder(const char* folderName, bool recursing)
{
	string folder(PathUtil::MakeGamePath(folderName));
	folder += "/";

	string searchString(folder + "*.xml");

	_finddata_t fd;
	intptr_t handle = 0;

	ICryPak *pPak = gEnv->pCryPak;
	handle = pPak->FindFirst(searchString.c_str(), &fd);

	if (handle > -1)
	{
		do
		{
			if (!strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
				continue;

			if (fd.attrib & _A_SUBDIR)
				ScanFolder(folder + fd.name, true);
			else
				LoadCommunicationSettingsXML(folder + fd.name);

		} while (pPak->FindNext(handle, &fd) >= 0);

		pPak->FindClose(handle);
	}

	if (!recursing)
		stl::push_back_unique(m_folderNames, folderName);
}

bool CCommunicationManager::LoadCommunicationSettingsXML(const char* fileName)
{
	XmlNodeRef rootNode = GetISystem()->LoadXmlFile(fileName);
	if (!rootNode)
	{
		AIWarning("Failed to open XML file '%s'...", fileName);

		return false;
	}

	const char* tagName = rootNode->getTag();

	if (!stricmp(tagName, "Communications"))
	{
		int childCount = rootNode->getChildCount();

		for (int i = 0; i < childCount; ++i)
		{
			XmlNodeRef childNode = rootNode->getChild(i);

			if (!stricmp(childNode->getTag(), "ChannelConfig"))
			{
				int channelCount = childNode->getChildCount();

				for (int c = 0; c < channelCount; ++c )
				{
					XmlNodeRef channelNode = childNode->getChild(c);
					if (!m_channels.LoadChannel(channelNode))
						return false;
				}
			}
			else if (!stricmp(childNode->getTag(), "Config"))
			{
				if (!childNode->haveAttr("name"))
				{
					AIWarning("Missing 'name' attribute for 'Config' tag at line %d...", 
						childNode->getLine());

					return false;
				}

				const char* name;
				childNode->getAttr("name", &name);

				std::pair<Configs::iterator, bool> iresult = m_configs.insert(
					Configs::value_type(GetConfigID(name), CommunicationsConfig()));

				if (!iresult.second)
				{
					if (iresult.first->second.name == name)
						AIWarning("Config '%s' redefinition at line %d...",	name, childNode->getLine());
					else
						AIWarning("Config name '%s' hash collision!",	name);

					return false;
				}

				CommunicationsConfig& config = iresult.first->second;
				config.name = name;
				
				int commCount = childNode->getChildCount();

				for (int c = 0; c < commCount; ++c)
				{
					XmlNodeRef commNode = childNode->getChild(c);

					if (!stricmp(commNode->getTag(), "Communication"))
					{
						SCommunication comm;
						if (!LoadCommunication(commNode, comm))
							return false;

						std::pair<Communications::iterator, bool> iresult = config.comms.insert(
							Communications::value_type(GetCommunicationID(comm.name.c_str()), comm));

						if (!iresult.second)
						{
							if (iresult.first->second.name == comm.name)
								AIWarning("Communication '%s' redefinition at line %d while parsing Communication XML '%s'...",	
									name, commNode->getLine(), fileName);
							else
								AIWarning("Communication name '%s' hash collision!",	name);
	
							return false;
						}
					}
					else if (!stricmp(commNode->getTag(), "AutoGenerateCommunication"))
					{
						std::vector<SCommunication> comms;
						if (!AutoGenerateCommunication(commNode, comms))
							return false;

						std::vector<SCommunication>::iterator it = comms.begin();
						std::vector<SCommunication>::iterator end = comms.end();

						for ( ; it != end; ++it)
						{
							SCommunication& comm = *it;

							std::pair<Communications::iterator, bool> iresult = config.comms.insert(
								Communications::value_type(GetCommunicationID(comm.name.c_str()), comm));

							if (!iresult.second)
							{
								if (iresult.first->second.name == comm.name)
									AIWarning("Communication '%s' redefinition at line %d while parsing Communication XML '%s'...",	
									name, commNode->getLine(), fileName);
								else
									AIWarning("Communication name '%s' hash collision!",	name);

								return false;
							}
						}
					}
					else
					{
						AIWarning(
							"Unexpected tag '%s' found at line %d while parsing Communication XML '%s'...",
							commNode->getTag(), commNode->getLine(), fileName);
					}
				}
			}
		}

		return true;
	}
	else
	{
		AIWarning(
			"Unexpected tag '%s' found at line %d while parsing Communication XML '%s'...",
			rootNode->getTag(), rootNode->getLine(), fileName);
	}

	return false;
}


bool FlagAttr(const XmlNodeRef& node, const char* attribute, uint32& flags, uint32 flag)
{
	XMLUtils::BoolType flagValue = XMLUtils::GetBoolType(node, attribute, (flags & flag) ? XMLUtils::True : XMLUtils::False);

	if (flagValue == XMLUtils::Invalid)
	{
		AIWarning("Invalid value for attribute '%s' tag '%s' at line %d...", 
			attribute, node->getTag(), node->getLine());

		return false;
	}

	flags &= ~flag;
	if (flagValue == XMLUtils::True)
		flags |= flag;

	return true;
}

bool FinishMethod(const XmlNodeRef& node, const char* attribute, uint32& flags)
{
	if (node->haveAttr(attribute))
	{
		const char* finishMethod;
		node->getAttr(attribute, &finishMethod);

		flags &= ~SCommunication::FinishAll;

		stack_string methodList(finishMethod);
		methodList.MakeLower();

		int start = 0;
		stack_string method = methodList.Tokenize(",", start);

		while (!method.empty())
		{
			method.Trim();

			if (method == "animation")
				flags |= SCommunication::FinishAnimation;
			else if (method == "sound")
				flags |= SCommunication::FinishSound;
			else if (method == "timeout")
				flags |= SCommunication::FinishTimeout;
			else if (method == "all")
				flags |= SCommunication::FinishAll;
			else
			{
				AIWarning("Invalid value '%s' for attribute '%s' tag '%s' at line %d...", 
					method.c_str(), attribute, node->getTag(), node->getLine());

				return false;
			}

			method = methodList.Tokenize(",", start);
		}
	}

	return true;
}

bool Blocking(const XmlNodeRef& node, const char* attribute, uint32& flags)
{
	if (node->haveAttr(attribute))
	{
		const char* blocking;
		node->getAttr(attribute, &blocking);

		flags &= ~SCommunication::BlockAll;

		stack_string methodList(blocking);
		methodList.MakeLower();

		int start = 0;
		stack_string method = methodList.Tokenize(",", start);

		while (!method.empty())
		{
			method.Trim();

			if (method == "movement")
				flags |= SCommunication::BlockMovement;
			else if (method == "fire")
				flags |= SCommunication::BlockFire;
			else if (method == "all")
				flags |= SCommunication::BlockAll;
			else if (method == "none")
			{
				flags &= ~SCommunication::BlockAll;
			}
			else
			{
				AIWarning("Invalid value '%s' for attribute '%s' tag '%s' at line %d...", 
					method.c_str(), attribute, node->getTag(), node->getLine());

				return false;
			}

			method = methodList.Tokenize(",", start);
		}
	}

	return true;
}

bool ChoiceMethod(const XmlNodeRef& node, const char* attribute, SCommunication::EVariationChoiceMethod& method)
{
	const char* choiceMethod;

	if (node->haveAttr(attribute))
	{
		node->getAttr(attribute, &choiceMethod);

		if (!stricmp(choiceMethod, "random"))
			method = SCommunication::Random;
		else if (!stricmp(choiceMethod, "sequence"))
			method = SCommunication::Sequence;
		else if (!stricmp(choiceMethod, "randomsequence"))
			method = SCommunication::RandomSequence;
		else if (!stricmp(choiceMethod, "match"))
			method = SCommunication::Match;
		else
		{
			AIWarning("Invalid value for attribute '%s' tag '%s' at line %d...", 
				attribute, node->getTag(), node->getLine());

			return false;
		}
	}
	return true;
}

bool AnimationType(const XmlNodeRef& node, const char* attribute, uint32& flags)
{
	if (node->haveAttr(attribute))
	{
		const char* animationType;
		node->getAttr(attribute, &animationType);

		flags &= ~SCommunication::AnimationAction;

		if (!stricmp(animationType, "action"))
			flags |= SCommunication::AnimationAction;
		else if (stricmp(animationType, "signal"))
		{
			AIWarning("Invalid value for attribute '%s' tag '%s' at line %d...", 
				attribute, node->getTag(), node->getLine());

			return false;
		}
	}

	return true;
}

bool CCommunicationManager::LoadCommunication(const XmlNodeRef& commNode, SCommunication& comm)
{
	if (!commNode->haveAttr("name"))
	{
		AIWarning("Missing 'name' attribute for 'Communication' tag at line %d...", 
			commNode->getLine());

		return false;
	}

	const char* name = 0;
	commNode->getAttr("name", &name);

	comm.name = name;
	comm.choiceMethod = SCommunication::RandomSequence;
	comm.responseChoiceMethod = SCommunication::RandomSequence;

	if (!ChoiceMethod(commNode, "choiceMethod", comm.choiceMethod))
		return false;

	if (commNode->haveAttr("responseName"))
	{
		const char* responseName;
		commNode->getAttr("responseName", &responseName);
		comm.responseID = GetCommunicationID(responseName);
	}
	else
	{
		comm.responseID = 0;
	}

	if (!ChoiceMethod(commNode, "responseChoiceMethod", comm.responseChoiceMethod))
		return false;

	SCommunicationVariation deflt;
	deflt.flags = SCommunication::BlockAll | SCommunication::FinishAll;
	
	if (!LoadVariation(commNode, deflt))
		return false;

	int variationCount = commNode->getChildCount();
	
	for (int i = 0; i < variationCount; ++i)
	{
		XmlNodeRef varNode = commNode->getChild(i);

		if (stricmp(varNode->getTag(), "Variation"))
		{
			AIWarning("Unexpected tag '%s' found at line %d...", varNode->getTag(), varNode->getLine());

			return false;
		}

		SCommunicationVariation variation(deflt);
		if (!LoadVariation(varNode, variation))
			return false;

		comm.variations.push_back(variation);

		if (comm.variations.size() == MaxVariationCount)
		{
			if (variationCount > MaxVariationCount)
			{
				AIWarning("Maximum number of variations reached for '%s' found at line %d...", commNode->getTag(),
					commNode->getLine());
			}

			break;
		}
	}

	return true;
}

bool CCommunicationManager::LoadVariation(const XmlNodeRef& varNode, SCommunicationVariation& variation)
{
	if (varNode->haveAttr("animationName"))
	{
		const char* animationName;
		varNode->getAttr("animationName", &animationName);
		variation.animationName = animationName;
	}

	if (varNode->haveAttr("soundName"))
	{
		const char* soundName;
		varNode->getAttr("soundName", &soundName);
		variation.soundName = soundName;
	}

	if (varNode->haveAttr("voiceName"))
	{
		const char* voiceName;
		varNode->getAttr("voiceName", &voiceName);
		variation.voiceName = voiceName;
	}

	if (!FlagAttr(varNode, "lookAtTarget", variation.flags, SCommunication::LookAtTarget))
		return false;
	
	if (!FinishMethod(varNode, "finishMethod", variation.flags))
		return false;

	if (!Blocking(varNode, "blocking", variation.flags))
		return false;

	if (!AnimationType(varNode, "animationType", variation.flags))
		return false;

	if (varNode->haveAttr("timeout"))
	{
		float timeout = 0.0f;
		if (varNode->getAttr("timeout", timeout))
			variation.timeout = timeout;
	}

	return true;
}

void CCommunicationManager::Reload()
{
	Reset();

	m_configs.clear();

	m_channels.Clear();
	m_player.Clear();

	for (uint32 i = 0; i < m_folderNames.size(); ++i)
		ScanFolder(m_folderNames[i].c_str(), false);
}

void CCommunicationManager::Reset()
{
	m_channels.Reset();
	m_player.Reset();

	ResetHistory();

	m_orderedQueue.clear();
	m_unorderedQueue.clear();
}

void CCommunicationManager::ResetHistory()
{
	Configs::iterator it = m_configs.begin();
	Configs::iterator end = m_configs.end();

	for ( ; it != end; ++it)
	{
		CommunicationsConfig&  config = it->second;

		Communications::iterator cit = config.comms.begin();
		Communications::iterator cend = config.comms.end();

		for ( ; cit != cend; ++cit)
		{
			SCommunication& comm = cit->second;
			comm.history = SCommunication::History();
		}
	}
}

void CCommunicationManager::Update(float updateTime)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	m_channels.Update(updateTime);
	m_player.Update(updateTime);

	UpdateQueue(m_orderedQueue, updateTime);
	UpdateQueue(m_unorderedQueue, updateTime);

	ProcessQueues();
}

void CCommunicationManager::DebugDraw()
{
}

CommunicationChannelID CCommunicationManager::GetChannelID(const char* name) const
{
	return m_channels.GetChannelID(name);
}

CommunicationConfigID CCommunicationManager::GetConfigID(const char* name) const
{
	return CryStringUtils::CalculateHashLowerCase(name);
}

CommunicationID CCommunicationManager::GetCommunicationID(const char* name) const
{
	return CryStringUtils::CalculateHashLowerCase(name);
}

const char* CCommunicationManager::GetCommunicationName(const CommunicationID& communicationID) const
{
	Configs::const_iterator cit =  m_configs.begin();
	Configs::const_iterator cend =  m_configs.end();

	for ( ; cit != cend; ++cit)
	{
		const CommunicationsConfig& config = cit->second;

		Communications::const_iterator commIt = config.comms.find(communicationID);
		if (commIt == config.comms.end())
			continue;

		const SCommunication& comm = commIt->second;
		return comm.name.c_str();
	}

	return 0;
}

bool CCommunicationManager::CanCommunicationPlay(const SCommunicationRequest& request, float* estimatedWaitTime)
{
	if (estimatedWaitTime)
		*estimatedWaitTime = FLT_MAX;

	CommunicationChannel::Ptr channel = m_channels.GetChannel(request.channelID, request.actorID);
	if (!channel)
		return false;

	if (channel->IsFree())
	{
		if (estimatedWaitTime)
			*estimatedWaitTime = 0.0f;

		return true;
	}

	return false;
}

CommunicationPlayID CCommunicationManager::PlayCommunication(const SCommunicationRequest& request)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	Configs::iterator cit = m_configs.find(request.configID);
	if (cit == m_configs.end())
		return CommunicationPlayID(0);

	CommunicationsConfig& config = cit->second;
	Communications::iterator commIt = config.comms.find(request.commID);
	if (commIt == config.comms.end())
		return CommunicationPlayID(0);

	SCommunication& comm = commIt->second;

	CommunicationChannel::Ptr channel = m_channels.GetChannel(request.channelID, request.actorID);
	if (!channel)
		return CommunicationPlayID(0);

	++m_playGenID;
	if (!m_playGenID)
		++m_playGenID;
	
	if (channel && channel->IsFree())
	{
		if (Play(m_playGenID, request, channel, comm))
			return m_playGenID;
	}

	if (request.ordering == SCommunicationRequest::Ordered)
		Queue(m_orderedQueue, m_playGenID, request);
	else
		Queue(m_unorderedQueue, m_playGenID, request);

	return m_playGenID;
}

void CCommunicationManager::StopCommunication(const CommunicationPlayID& playID)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	if (m_player.IsPlaying(playID))
	{
		PlayingCommunications::iterator it = m_playing.find(playID);
		assert(it != m_playing.end());
		
		PlayingCommunication& playing = it->second;
		
		if (playing.eventListener)
			playing.eventListener->OnCommunicationEvent(CommunicationCancelled, playing.actorID, playID);

		m_playing.erase(it); // erase before stopping - to simplify handling in the OnCommunicationFinished
		
		m_player.Stop(playID);
	}

	if (RemoveFromQueue(m_orderedQueue, playID))
		return;

	if (RemoveFromQueue(m_unorderedQueue, playID))
		return;
}

bool CCommunicationManager::IsPlaying(const CommunicationPlayID& playID, float* timeRemaining) const
{
	return m_player.IsPlaying(playID, timeRemaining);
}

bool CCommunicationManager::IsQueued(const CommunicationPlayID& playID, float* estimatedWaitTime) const
{
	QueuedCommunications::const_iterator oit = m_orderedQueue.begin();
	QueuedCommunications::const_iterator oend = m_orderedQueue.end();

	for ( ; oit != oend; ++oit)
	{
		if (oit->playID == playID)
			return true;
	}

	QueuedCommunications::const_iterator uit = m_unorderedQueue.begin();
	QueuedCommunications::const_iterator uend = m_unorderedQueue.end();

	for ( ; uit != uend; ++uit)
	{
		if (uit->playID == playID)
			return true;
	}

	return false;
}

void CCommunicationManager::OnCommunicationFinished(const CommunicationPlayID& playID)
{
	PlayingCommunications::iterator it = m_playing.find(playID);
	if (it != m_playing.end())
	{
		PlayingCommunication& playing = it->second;

		assert(!playing.channel->IsFree());
		playing.channel->Occupy(false);

		if (playing.eventListener)
			playing.eventListener->OnCommunicationEvent(CommunicationFinished, playing.actorID, playID);
	}
}

bool CCommunicationManager::Play(const CommunicationPlayID& playID, const SCommunicationRequest& request, 
																 const CommunicationChannel::Ptr& channel, SCommunication& comm)
{
	uint32 variation = ChooseVariation(comm, comm.choiceMethod, 0);
	
	if (m_player.Play(m_playGenID, request, comm, variation, this, (void*)request.channelID))
	{
		UpdateHistory(comm, variation);

		channel->Occupy(true);

		PlayingCommunication& playing = stl::map_insert_or_get(m_playing, playID, PlayingCommunication());
		playing.actorID = request.actorID;
		playing.channel = channel;
		playing.eventListener = request.eventListener;
		
		if (request.eventListener)
			request.eventListener->OnCommunicationEvent(CommunicationStarted, request.actorID, playID);

		return true;
	}

	return false;
}

void CCommunicationManager::Queue(QueuedCommunications& queue, const CommunicationPlayID& playID,
																	const SCommunicationRequest& request)
{
	assert(!IsQueued(playID, 0));

	queue.push_back(QueuedCommunication());
	QueuedCommunication& back = queue.back();

	back.playID = playID;
	back.age = 0.0f;
	back.request = request;

	if (request.eventListener)
		request.eventListener->OnCommunicationEvent(CommunicationQueued, request.actorID, playID);

	assert((m_unorderedQueue.size() + m_orderedQueue.size()) < 32);
}

bool CCommunicationManager::RemoveFromQueue(QueuedCommunications& queue, const CommunicationPlayID& playID)
{
	QueuedCommunications::iterator it = queue.begin();
	QueuedCommunications::iterator end = queue.end();

	for ( ; it != end; ++it)
	{
		QueuedCommunication& queued = *it;

		if (queued.playID == playID)
		{
			if (queued.request.eventListener)
				queued.request.eventListener->OnCommunicationEvent(CommunicationCancelled, queued.request.actorID, queued.playID);

			queue.erase(it);
			return true;
		}
	}

	return false;
}

void CCommunicationManager::UpdateQueue(QueuedCommunications& queue, float updateTime)
{
	QueuedCommunications::iterator it = queue.begin();
	QueuedCommunications::iterator end = queue.end();

	for ( ; it != end; )
	{
		QueuedCommunication& queued = *it;
		queued.age += updateTime;

		if (queued.age <= queued.request.contextExpirity)
			++it;
		else
		{
			if (queued.request.eventListener)
				queued.request.eventListener->OnCommunicationEvent(CommunicationExpired, queued.request.actorID, queued.playID);

			QueuedCommunications::iterator erased = it++;
			queue.erase(erased);
		}
	}
}

void CCommunicationManager::ProcessQueues()
{
	// Ordered queue has priority since it's usually more important
	while (!m_orderedQueue.empty())
	{
		QueuedCommunication& queued = m_orderedQueue.front();

		// sanity checks are performed before inserting in the queue
		Configs::iterator cit = m_configs.find(queued.request.configID);
		CommunicationsConfig& config = cit->second;

		Communications::iterator commIt = config.comms.find(queued.request.commID);
		SCommunication& comm = commIt->second;

		CommunicationChannel::Ptr channel = m_channels.GetChannel(queued.request.channelID, queued.request.actorID);
		if (!channel->IsFree())
			break;
		
		if (!Play(queued.playID, queued.request, channel, comm))
			break;

		m_orderedQueue.erase(m_orderedQueue.begin());
	}

	while (!m_unorderedQueue.empty())
	{
		QueuedCommunication& queued = m_unorderedQueue.front();

		// sanity checks are performed before inserting in the queue
		Configs::iterator cit = m_configs.find(queued.request.configID);
		CommunicationsConfig& config = cit->second;

		Communications::iterator commIt = config.comms.find(queued.request.commID);
		SCommunication& comm = commIt->second;

		CommunicationChannel::Ptr channel = m_channels.GetChannel(queued.request.channelID, queued.request.actorID);
		if (!channel->IsFree())
			break;

		if (!Play(queued.playID, queued.request, channel, comm))
			break;

		m_unorderedQueue.erase(m_unorderedQueue.begin());
	}
}

bool CCommunicationManager::AutoGenerateCommunication(const XmlNodeRef& commNode, std::vector<SCommunication>& comms)
{
	if (!commNode->haveAttr("voiceLib"))
	{
		AIWarning("Missing 'voiceLib' attribute for 'AutoGenerateCommunication' tag at line %d...", 
			commNode->getLine());

		return false;
	}

	const char* voiceLibName = 0;
	commNode->getAttr("voiceLib", &voiceLibName);

	SCommunicationVariation defaultVariation;
	defaultVariation.flags = SCommunication::BlockAll | SCommunication::FinishAll;

 	if (!LoadVariation(commNode, defaultVariation))
 		return false;

	ICommunicationVoiceLibrary* commVoiceLib = gEnv->pGame->GetIGameFramework()->GetICommunicationVoiceLibrary();
	assert(commVoiceLib);
	if (!commVoiceLib)
		return true;

	const char* groupNames[1024];
	uint32 groupCount = commVoiceLib->GetGroupNames(voiceLibName, 1024, groupNames);
	for (uint32 groupIndex = 0; groupIndex < groupCount; ++groupIndex)
	{
		const char* groupName = groupNames[groupIndex];

		SCommunication comm;
		comm.name = groupName;
		comm.choiceMethod = SCommunication::RandomSequence;
		comm.responseChoiceMethod = SCommunication::RandomSequence;
		comm.responseID = 0;

		SCommunicationVariation variation(defaultVariation);
		variation.voiceName = groupName;
		comm.variations.push_back(variation);

		comms.push_back(comm);
	}

	return true;
}

uint32 CCommunicationManager::ChooseVariation(const SCommunication& comm, SCommunication::EVariationChoiceMethod method,
																						uint32 index) const
{
	uint32 variationCount = comm.variations.size();

	switch(method)
	{
	case SCommunication::RandomSequence:
		{
			m_randomPool.resize(0);
			
			for (uint32 i = 0; i < variationCount; ++i)
			{
				if ((comm.history.played & (1 << i)) == 0)
					m_randomPool.push_back(i);
			}

			if (!m_randomPool.empty())
				return m_randomPool[Random(variationCount)];
			return 0;
		}
		break;
	case SCommunication::Sequence:
		{
			uint32 played = comm.history.played;
			uint32 next = 0;

			while (played >>= 1)
				++next;

			return next;
		}
		break;
	case SCommunication::Match:
		{
			if (index < variationCount)
				return index;
			else
				return index % variationCount;
		}
		break;
	default:
	case SCommunication::Random:
		return Random(variationCount);
	}
}

void CCommunicationManager::UpdateHistory(SCommunication& comm, uint32 variation)
{
	uint32 variationCount = comm.variations.size();
	assert(variation < variationCount);

	comm.history.played |= 1 << variation;

	// Reset everything except the one currently played,
	// to prevent it from being picked again next play
	if (variationCount > 1)
	{
		if ((comm.history.played & ((1 << variationCount) - 1)) == comm.history.played)
			comm.history.played = 1 << variation;
	}
}