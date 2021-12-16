#include "StdAfx.h"
#include "CoordinationTaskCommunication.h"
#include "Communication/CommunicationManager.h"



void CoordinationTaskCommunication::RunningCommunication::OnCommunicationEvent(
	ICommunicationManager::ECommunicationEvent event, EntityId actorID, const CommunicationPlayID& playID)
{
	switch (event)
	{
	case ICommunicationManager::CommunicationCancelled:
	case ICommunicationManager::CommunicationFinished:
	case ICommunicationManager::CommunicationExpired:
		{
			CoordinationActor actor(actorID);
			ActorStates::iterator it = actors.find(actor);
			assert(it != actors.end());

			ActorState& actorState = it->second;
			actorState.state = CoordinationFinished;
			--activeCount;
		}
		break;
	default:
		break;
	}
}

CoordinationTaskCommunication::CoordinationTaskCommunication(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor)
: m_timeout(0.0f)
, m_commID(0)
, m_channelID(0)
, m_expirity(0.0f)
, m_ordering(SCommunicationRequest::Ordered)
, m_sync(setup, taskMonitor)
, m_setup(setup)
, m_taskMonitor(taskMonitor)
{
}

CoordinationTaskCommunication::~CoordinationTaskCommunication()
{
}

const char* CoordinationTaskCommunication::GetName() const
{
	return "Comm";
}

const char* CoordinationTaskCommunication::GetDescription() const
{
	return gAIEnv.pCommunicationManager->GetCommunicationName(m_commID);
}

bool CoordinationTaskCommunication::LoadFromXML(int roleID, const XmlNodeRef& rootNode)
{
	const char* commName;
	if (!rootNode->getAttr("name", &commName))
	{
		AIWarning("Missing 'name' attribute for 'Communication' tag at line %d...", rootNode->getLine());

		return false;
	}

	m_commID = gAIEnv.pCommunicationManager->GetCommunicationID(commName);
	if (!m_commID)
	{
		AIWarning("Unknown communication name '%s' at line %d...", commName, rootNode->getLine());

		return false;
	}

	const char* channelName;
	if (!rootNode->getAttr("channel", &channelName))
	{
		AIWarning("Missing 'channel' attribute for 'Communication' tag at line %d...", rootNode->getLine());

		return false;
	}

	m_channelID = gAIEnv.pCommunicationManager->GetChannelID(channelName);
	if (!m_channelID)
	{
		AIWarning("Unknown channel name '%s' at line %d...", channelName, rootNode->getLine());

		return false;
	}

	if (rootNode->haveAttr("timeout"))
		rootNode->getAttr("timeout", m_timeout);

	if (rootNode->haveAttr("expirity"))
		rootNode->getAttr("expirity", m_expirity);

	if (rootNode->haveAttr("ordering"))
	{
		const char* ordering = 0;
		rootNode->getAttr("ordering", &ordering);

		if (!stricmp(ordering, "ordered"))
			m_ordering = SCommunicationRequest::Ordered;
		else if (!stricmp(ordering, "ordered"))
			m_ordering = SCommunicationRequest::Unordered;
		else
		{
			AIWarning("Invalid ordering '%s' for 'Communication' tag at line %d...", ordering, rootNode->getLine());

			return false;
		}
	}

	if (!m_sync.LoadFromXML(roleID, rootNode))
		return false;

	return true;
}

ECoordinationState CoordinationTaskCommunication::GetActorState(const CoordinationID& coordinationID,
																													 const CoordinationActor& actor) const
{
	RunningCommunications::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningCommunication& running = it->second;

		ActorStates::const_iterator ait = running.actors.find(actor);

		if (ait != running.actors.end())
			return ait->second.state;
	}

	return CoordinationNotRunning;
}

ECoordinationState CoordinationTaskCommunication::GetState(const CoordinationID& coordinationID) const
{
	RunningCommunications::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningCommunication& running = it->second;

		return running.state;
	}

	return CoordinationNotRunning;
}

bool CoordinationTaskCommunication::CanStart(const CoordinationID& coordinationID, const CoordinationActors& assignees) const
{
	return true;
}

void CoordinationTaskCommunication::Start(const CoordinationID& coordinationID,
																					const CoordinationActors& assignees)
{
	std::pair<RunningCommunications::iterator, bool> result = m_running.insert(
		RunningCommunications::value_type(coordinationID, RunningCommunication()));
	
	RunningCommunication& running = result.first->second;
	
	SCommunicationRequest request;
	request.channelID = m_channelID;
	request.commID = m_commID;
	request.contextExpirity = m_expirity;
	request.ordering = m_ordering;
	request.eventListener = &running;

	CoordinationActors::const_iterator ait = assignees.begin();
	CoordinationActors::const_iterator aend = assignees.end();

	for ( ; ait != aend; ++ait)
	{
		request.actorID = ait->GetEntityID();
		request.configID = ait->GetCommunicationConfigID();

		ActorState& actorState = stl::map_insert_or_get(running.actors, *ait, ActorState());
		actorState.state = CoordinationRunning;
		actorState.timeout = m_timeout;

		if (CommunicationPlayID playID = gAIEnv.pCommunicationManager->PlayCommunication(request))
		{
			++running.activeCount;
			actorState.playID = playID;
		}
		else
			actorState.state = CoordinationFinished;
	}

	running.state = CoordinationRunning;
	if (result.second)
		m_sync.Start(coordinationID);
}

void CoordinationTaskCommunication::Stop(const CoordinationID& coordinationID)
{
	RunningCommunications::iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	RunningCommunication& running = it->second;

	ActorStates::iterator ait = running.actors.begin();
	ActorStates::iterator aend = running.actors.end();

	for ( ; ait != aend; ++ait)
		Stop(coordinationID, ait->first); // TODO(márcio) fix the 2 extra-lookups

	m_running.erase(it);
	m_sync.Stop(coordinationID);
}

void CoordinationTaskCommunication::Stop(const CoordinationID& coordinationID, const CoordinationActor& actor)
{
	RunningCommunications::iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	RunningCommunication& running = it->second;
	ActorStates::iterator ait = running.actors.find(actor);
	assert(ait != running.actors.end());

	ActorState& actorState = ait->second;

	if (actorState.state == CoordinationRunning)
	{
		gAIEnv.pCommunicationManager->StopCommunication(actorState.playID);
		--running.activeCount;
	}

	actorState.state = CoordinationFinished;
}

void CoordinationTaskCommunication::Update(float updateTime)
{
	if (!m_running.empty())
	{
		m_sync.Update();

		RunningCommunications::iterator ait = m_running.begin();
		RunningCommunications::iterator aend = m_running.end();

		for ( ; ait != aend; ++ait)
		{
			const CoordinationID& coordinationID = ait->first;
			RunningCommunication& running = ait->second;

			if (m_timeout > 0.0f)
			{
				ActorStates::iterator ait = running.actors.begin();
				ActorStates::iterator aend = running.actors.end();

				for ( ; ait != aend; ++ait)
				{
					ActorState& actorState = ait->second;

					if (actorState.state == CoordinationRunning)
					{
						actorState.timeout -= updateTime;

						if (actorState.timeout <= 0.0f)
						{
							const CoordinationActor& actor = ait->first;

							gAIEnv.pCommunicationManager->StopCommunication(actorState.playID);
							actorState.state = CoordinationWaiting;
							--running.activeCount;
						}
					}
				}
			}

			if (!running.activeCount)
			{
				if (m_sync.GetState(coordinationID) == CoordinationFinished)
					running.state = CoordinationFinished;
				else
					running.state = CoordinationWaiting;
			}
		}
	}
}