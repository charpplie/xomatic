#include "StdAfx.h"
#include "CoordinationTaskSignal.h"


CoordinationTaskSignal::CoordinationTaskSignal(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor)
: m_sync(setup, taskMonitor)
, m_setup(setup)
, m_taskMonitor(taskMonitor)
{
}

const char* CoordinationTaskSignal::GetName() const
{
	return "Signal";
}

const char* CoordinationTaskSignal::GetDescription() const
{
	return m_signalName.c_str();
}

bool CoordinationTaskSignal::LoadFromXML(int roleID, const XmlNodeRef& rootNode)
{
	const char* name;
	if (!rootNode->getAttr("name", &name))
	{
		AIWarning("Missing 'name' attribute for 'Signal' tag at line %d...", rootNode->getLine());

		return false;
	}

	m_signalName = name;

	if (!m_sync.LoadFromXML(roleID, rootNode))
		return false;

	return true;
}

ECoordinationState CoordinationTaskSignal::GetActorState(const CoordinationID& coordinationID,
																												 const CoordinationActor& actor) const
{
	RunningSignals::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningSignal& signal = it->second;

		return signal.state;
	}

	return CoordinationNotRunning;
}

ECoordinationState CoordinationTaskSignal::GetState(const CoordinationID& coordinationID) const
{
	RunningSignals::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningSignal& signal = it->second;

		return signal.state;
	}

	return CoordinationNotRunning;
}

bool CoordinationTaskSignal::CanStart(const CoordinationID& coordinationID, const CoordinationActors& assignees) const
{
	return true;
}

void CoordinationTaskSignal::Start(const CoordinationID& coordinationID,
																	 const CoordinationActors& assignees)
{
	RunningSignals::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
		return;

	std::pair<RunningSignals::iterator, bool> result = m_running.insert(RunningSignals::value_type(coordinationID, RunningSignal()));
	RunningSignal& running = result.first->second;

	CoordinationActors actors = assignees;
	
	CoordinationActors::iterator ait = actors.end();
	CoordinationActors::iterator aend = actors.end();

	for ( ; ait != aend; ++ait)
	{
		CoordinationActor& actor = *ait;

		actor.Signal(m_signalName.c_str());
	}

	running.state = CoordinationRunning;
	
	m_sync.Start(coordinationID);
}

void CoordinationTaskSignal::Stop(const CoordinationID& coordinationID)
{
	RunningSignals::iterator it = m_running.find(coordinationID);

	if (it != m_running.end())
	{
		RunningSignal& running = it->second;

		m_running.erase(it);
	}

	m_sync.Stop(coordinationID);
}

void CoordinationTaskSignal::Stop(const CoordinationID& coordinationID, const CoordinationActor& actor)
{
}

void CoordinationTaskSignal::Update(float frameTime)
{
	if (!m_running.empty())
	{
		m_sync.Update();

		RunningSignals::iterator ait = m_running.begin();
		RunningSignals::iterator aend = m_running.end();

		for ( ; ait != aend; ++ait)
		{
			const CoordinationID& coordinationID = ait->first;
			RunningSignal& signal = ait->second;

			bool finished = true;
			if (finished)
			{
				if (m_sync.GetState(coordinationID) == CoordinationFinished)
					signal.state = CoordinationFinished;
				else
					signal.state = CoordinationWaiting;
			}
		}
	}
}