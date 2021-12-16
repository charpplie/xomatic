#include "StdAfx.h"
#include "CoordinationTaskAction.h"


CoordinationTaskAction::RunningAction::~RunningAction()
{
	ActorStates::iterator ait = actors.begin();
	ActorStates::iterator aend = actors.end();

	for ( ; ait != aend; ++ait)
	{
		const CoordinationActor& actor = ait->first;
		ActorState& state = ait->second;

		if (state.state != CoordinationRunning)
			actor.UnregisterBehaviorListener(this);
	}
}

void CoordinationTaskAction::RunningAction::BehaviorEvent(IAIObject* actor, EBehaviorEvent event)
{
	if (event != BehaviorFinished)
		return;

	CoordinationActor participant(actor);

	ActorStates::iterator it = actors.find(participant);
	assert(it != actors.end());
	
	ActorState& state = it->second;
	state.state = CoordinationWaiting;
	participant.UnregisterBehaviorListener(this);

	--activeCount;
}

void CoordinationTaskAction::RunningAction::BehaviorChanged(IAIObject* actor, const char* current, const char* previous)
{
}

const char* CoordinationTaskAction::GetName() const
{
	return "Action";
}

const char* CoordinationTaskAction::GetDescription() const
{
	return m_actionName;
}

CoordinationTaskAction::CoordinationTaskAction(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor)
: m_timeout(0.0f)
, m_sync(setup, taskMonitor)
, m_setup(setup)
, m_taskMonitor(taskMonitor)
{
}

bool CoordinationTaskAction::LoadFromXML(int roleID, const XmlNodeRef& rootNode)
{
	const char* name;
	if (!rootNode->getAttr("name", &name))
	{
		AIWarning("Missing 'name' attribute for 'Action' tag at line %d...", rootNode->getLine());

		return false;
	}

	m_actionName = name;

	if (rootNode->haveAttr("stopName"))
	{
		const char* stopName;
		rootNode->getAttr("stopName", &stopName);
		
		m_stopName = stopName;
	}

	if (rootNode->haveAttr("timeout"))
		rootNode->getAttr("timeout", m_timeout);

	if (!m_sync.LoadFromXML(roleID, rootNode))
		return false;

	return true;
}

ECoordinationState CoordinationTaskAction::GetActorState(const CoordinationID& coordinationID, 
																												 const CoordinationActor& actor) const
{
	RunningActions::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningAction& running = it->second;

		ActorStates::const_iterator ait = running.actors.find(actor);

		if (ait != running.actors.end())
			return ait->second.state;
	}

	return CoordinationNotRunning;
}

ECoordinationState CoordinationTaskAction::GetState(const CoordinationID& coordinationID) const
{
	RunningActions::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningAction& running = it->second;

		return running.state;
	}

	return CoordinationNotRunning;
}

bool CoordinationTaskAction::CanStart(const CoordinationID& coordinationID, const CoordinationActors& assignees) const
{
	return true;
}

void CoordinationTaskAction::Start(const CoordinationID& coordinationID,
																	 const CoordinationActors& assignees)
{
	std::pair<RunningActions::iterator, bool> result = m_running.insert(
		RunningActions::value_type(coordinationID, RunningAction()));

	RunningAction& running = result.first->second;

	CoordinationActors::const_iterator ait = assignees.begin();
	CoordinationActors::const_iterator aend = assignees.end();

	for ( ; ait != aend; ++ait)
	{
		ActorState& actorState = stl::map_insert_or_get(running.actors, *ait, ActorState());
		actorState.state = CoordinationRunning;
		actorState.timeout = m_timeout;

		ait->Action(m_actionName.c_str());
		ait->RegisterBehaviorListener(&running);
	}

	running.activeCount += assignees.size();

	running.state = CoordinationRunning;
	if (result.second)
		m_sync.Start(coordinationID);
}

void CoordinationTaskAction::Stop(const CoordinationID& coordinationID)
{
	RunningActions::iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	RunningAction& running = it->second;

	ActorStates::iterator ait = running.actors.begin();
	ActorStates::iterator aend = running.actors.end();

	for ( ; ait != aend; ++ait)
	{
		ActorState& actorState = ait->second;
		if (actorState.state != CoordinationNotRunning)
			Stop(coordinationID, ait->first);	// TODO(márcio) fix the 2 extra-lookups
	}

	m_running.erase(it);
	m_sync.Stop(coordinationID);
}

void CoordinationTaskAction::Stop(const CoordinationID& coordinationID, const CoordinationActor& actor)
{
	RunningActions::iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	RunningAction& running = it->second;
	ActorStates::iterator ait = running.actors.find(actor);
	assert(ait != running.actors.end());

	ActorState& actorState = ait->second;

	assert(actorState.state != CoordinationNotRunning);

	if (actorState.state == CoordinationRunning)
	{
		actor.UnregisterBehaviorListener(&running);
		--running.activeCount;
	}

	if (actorState.state != CoordinationFinished)
		actor.Action(m_stopName.c_str());

	actorState.state = CoordinationNotRunning;
}

void CoordinationTaskAction::Update(float updateTime)
{
	if (!m_running.empty())
	{
		m_sync.Update();

		RunningActions::iterator it = m_running.begin();
		RunningActions::iterator end = m_running.end();

		for ( ; it != end; ++it)
		{
			const CoordinationID& coordinationID = it->first;
			RunningAction& running = it->second;

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

							actor.UnregisterBehaviorListener(&running);
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