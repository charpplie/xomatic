#include "StdAfx.h"
#include "CoordinationTaskCoordination.h"
#include "CoordinationManager.h"


CoordinationTaskCoordination::CoordinationTaskCoordination(const CoordinationSetup* setup, 
																													 const CoordinationTaskMonitor* taskMonitor)
: m_timeout(0.0f)
, m_coordSetupID(0)
, m_sync(setup, taskMonitor)
, m_setup(setup)
, m_taskMonitor(taskMonitor)
{
}

bool CoordinationTaskCoordination::LoadFromXML(int roleID, const XmlNodeRef& rootNode)
{
	const char* name;
	if (!rootNode->getAttr("name", &name))
	{
		AIWarning("Missing 'name' attribute for 'Coordination' tag at line %d...", rootNode->getLine());

		return false;
	}

	m_coordSetupID = gAIEnv.pCoordinationManager->GetCoordinationSetupID(name);

	if (rootNode->haveAttr("timeout"))
		rootNode->getAttr("timeout", m_timeout);

	if (!m_sync.LoadFromXML(roleID, rootNode))
		return false;

	return true;
}

const char* CoordinationTaskCoordination::GetName() const
{
	return "Coord";
}

const char* CoordinationTaskCoordination::GetDescription() const
{
	return gAIEnv.pCoordinationManager->GetCoordinationSetupName(m_coordSetupID);
}

ECoordinationState CoordinationTaskCoordination::GetActorState(const CoordinationID& coordinationID, 
																															 const CoordinationActor& actor) const
{
	RunningCoordinations::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningCoordination& running = it->second;

		return running.state;
	}

	return CoordinationNotRunning;
}

ECoordinationState CoordinationTaskCoordination::GetState(const CoordinationID& coordinationID) const
{
	RunningCoordinations::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningCoordination& running = it->second;

		return running.state;
	}

	return CoordinationNotRunning;
}

bool CoordinationTaskCoordination::CanStart(const CoordinationID& coordinationID,
																						const CoordinationActors& assignees) const
{
	CoordinationInternalStartParams subparams;
	subparams.actors = assignees;

	return gAIEnv.pCoordinationManager->CanStart(m_coordSetupID, subparams);
}

void CoordinationTaskCoordination::Start(const CoordinationID& coordinationID,
																	 const CoordinationActors& assignees)
{
	RunningCoordinations::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
		return;

	std::pair<RunningCoordinations::iterator, bool> result = m_running.insert(
		RunningCoordinations::value_type(coordinationID, RunningCoordination()));

	RunningCoordination& running = result.first->second;

	CoordinationInternalStartParams params;
	params.actors = assignees;

	running.coordID = gAIEnv.pCoordinationManager->StartCoordination(m_coordSetupID, params);
	running.timeout = m_timeout;

	m_sync.Start(coordinationID);
}

void CoordinationTaskCoordination::Stop(const CoordinationID& coordinationID)
{
	RunningCoordinations::iterator it = m_running.find(coordinationID);

	if (it != m_running.end())
	{
		RunningCoordination& running = it->second;
		
		gAIEnv.pCoordinationManager->StopCoordination(running.coordID);

		m_running.erase(it);
	}

	m_sync.Stop(coordinationID);
}

void CoordinationTaskCoordination::Stop(const CoordinationID& coordinationID, const CoordinationActor& actor)
{
}

void CoordinationTaskCoordination::Update(float frameTime)
{
	if (!m_running.empty())
	{
		m_sync.Update();

		RunningCoordinations::iterator ait = m_running.begin();
		RunningCoordinations::iterator aend = m_running.end();

		for ( ; ait != aend; ++ait)
		{
			const CoordinationID& coordinationID = ait->first;
			RunningCoordination& running = ait->second;

			if (!running.finished)
			{
				ECoordinationState coordState = gAIEnv.pCoordinationManager->GetCoordinationState(running.coordID);
				if (coordState == CoordinationFinished)
					running.finished = true;

				if (!running.finished && (m_timeout > 0.0f))
				{
					running.timeout -= frameTime;

					if (running.timeout <= 0.0f)
					{
						gAIEnv.pCoordinationManager->StopCoordination(running.coordID);
						running.finished = true;
					}
				}
			}

			if (running.finished)
			{
				if (m_sync.GetState(coordinationID) == CoordinationFinished)
					running.state = CoordinationFinished;
				else
					running.state = CoordinationWaiting;
			}
		}
	}
}