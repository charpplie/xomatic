#include "StdAfx.h"
#include "CoordinationStep.h"
#include "CoordinationPlan.h"
#include "CoordinationActorAssignment.h"
#include "CoordinationSetup.h"

#include "DebugDrawContext.h"


CoordinationStep::CoordinationStep(const CoordinationSetup* setup)
: m_setup(setup)
{
}

bool CoordinationStep::LoadFromXML(uint32 elementId, const XmlNodeRef& rootNode)
{
	if (rootNode->haveAttr("name"))
	{
		const char* name;
		rootNode->getAttr("name", &name);
		m_name = name;
	}
	else
		m_name.Format("Step:%d", elementId);

	const char* binder = "";
	if (rootNode->haveAttr("binder"))
		rootNode->getAttr("binder", &binder);

	m_binderName = binder;

	int childCount = rootNode->getChildCount();

	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef childNode = rootNode->getChild(i);

		const char* tagName = childNode->getTag();

		if (!stricmp(tagName, "Sequence"))
		{
			const char* roleName = 0;
			if (!childNode->getAttr("role", &roleName))
			{
				AIWarning("Missing 'role' attribute for 'Sequence' tag at line %d...", childNode->getLine());

				return false;
			}

			int roleID = -1;

			const CoordinationRoles& roles = m_setup->GetRoles();
			
			CoordinationRoles::const_iterator it = roles.begin();
			CoordinationRoles::const_iterator end = roles.end();

			for (int i = 0; it != end; ++it, ++i)
			{
				if (!stricmp(it->GetName(), roleName))
				{
					roleID = i;
					break;
				}
			}

			if (roleID < 0)
			{
				AIWarning("Unknown Role '%s' for 'Sequence' tag at line %d...", 
					roleName, childNode->getLine());

				return false;
			}

			CoordinationRoleSequence roleSequence(m_setup, this);
			if (!roleSequence.LoadFromXML(m_roleSequences.size(), roleID, childNode))
				return false;

			m_roleSequences.push_back(roleSequence);
		}
	}

	return true;
}

uint32 CoordinationStep::GetRunningCount() const
{
	return m_running.size();
}

const CoordinationID& CoordinationStep::GetRunningCoordinationID(uint32 index) const
{
	assert(index >= 0 && index < m_running.size());
	RunningSteps::const_iterator it = m_running.begin();
	std::advance(it, index);

	return it->first;
}

uint32 CoordinationStep::GetSequenceCount() const
{
	return m_roleSequences.size();
}

const CoordinationRoleSequence& CoordinationStep::GetSequence(uint32 index) const
{
	assert(index >= 0 && index < m_roleSequences.size());

	return m_roleSequences[index];
}

void CoordinationStep::DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const
{
	context->Draw2dLabel(context.x + context.ident, context.y, context.fontSize, Col_SlateBlue, false, "%s", 
		GetName());
	context.y += context.lineHeight;

	uint32 sequenceCount = GetSequenceCount();

	context.ident += context.identWidth;

	float x = context.x;
	float y = context.y;
	float highestY = y;

	for (uint32 s = 0; s < sequenceCount; ++s)
	{
		const CoordinationRoleSequence& sequence = GetSequence(s);

		sequence.DebugDrawStatus(coordinationID, context);

		if (context.y > highestY)
			highestY = context.y;

		context.x += 152.0f;
		context.y = y;
	}

	context.x = x;
	context.y = highestY;

	context.ident -= context.identWidth;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

ECoordinationState CoordinationStep::GetState(const CoordinationID& coordinationID) const
{
	RunningSteps::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningStep& step = it->second;

		return step.state;
	}

	return CoordinationNotRunning;
}

bool CoordinationStep::CanStart(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params) const
{
	return true;
}

void CoordinationStep::Start(const CoordinationID& coordinationID)
{
	const CoordinationInternalStartParams& params = m_setup->GetParams(coordinationID);
	const CoordinationRoles& roles = m_setup->GetRoles();

	const CoordinationScriptEnvironment& scriptEnvironment = m_setup->GetScriptEnvironment(coordinationID);
	scriptEnvironment.Apply();

	if (!m_binderName.empty())
	{
		const CoordinationEnvironmentBinders& binders = m_setup->GetContextBinders(coordinationID);

		CoordinationEnvironmentBinders::const_iterator it = binders.find(m_binderName);
		if (it != binders.end())
			it->second.Execute();
	}

	uint32 roleCount = m_roleSequences.size(); 

	CoordinationActorAssignment roleAssignments(params.actors, roles);
	roleAssignments.ReserveRoles(roleCount);

	for (uint32 i = 0; i < roleCount; ++i)
		roleAssignments.AddRole(m_roleSequences[i].GetRoleID());

	roleAssignments.Process(scriptEnvironment);

	std::pair<RunningSteps::iterator, bool> result = 
		m_running.insert(RunningSteps::value_type(coordinationID, RunningStep()));
	
	if (!result.second)
	{
		assert(0);
		return;
	}

	RunningStep& running = result.first->second;

	RoleSequences::iterator rit = m_roleSequences.begin();
	RoleSequences::iterator rend = m_roleSequences.end();

	for ( ; rit != rend; ++rit)
	{
		CoordinationRoleSequence& roleSequence = *rit;
		const CoordinationRole& role = roles[roleSequence.GetRoleID()];

		if (role.IsRequired())
		{
			const CoordinationActors& assignees = roleAssignments.GetAssignees(roleSequence.GetRoleID());

			if (assignees.size() < role.GetMinActors())
			{
				running.state = CoordinationFailed;
				return;
			}
		}
	}

	rit = m_roleSequences.begin();

	for ( ; rit != rend; ++rit)
	{
		CoordinationRoleSequence& roleSequence = *rit;

		roleSequence.Start(coordinationID, roleAssignments.GetAssignees(roleSequence.GetRoleID()));
	}
}

void CoordinationStep::Stop(const CoordinationID& coordinationID)
{
	RunningSteps::iterator it = m_running.find(coordinationID);

	if (it != m_running.end())
	{
		RunningStep& running = it->second;

		RoleSequences::iterator rit = m_roleSequences.begin();
		RoleSequences::iterator rend = m_roleSequences.end();

		for ( ; rit != rend; ++rit)
		{
			CoordinationRoleSequence& roleSequence = *rit;
			roleSequence.Stop(coordinationID);
		}

		m_running.erase(it);
	}
}

void CoordinationStep::Update(float updateTime)
{
	if (!m_running.empty())
	{
		RoleSequences::iterator it = m_roleSequences.begin();
		RoleSequences::iterator end = m_roleSequences.end();

		for ( ; it != end; ++it)
			it->Update(updateTime);

		RunningSteps::iterator sit = m_running.begin();
		RunningSteps::iterator send = m_running.end();

		for ( ; sit != send; ++sit)
		{
			RunningStep& running = sit->second;
			if (running.state != CoordinationRunning)
				continue;

			const CoordinationID& coordinationID = sit->first;

			uint32 finishedCount = 0;
			uint32 failedCount = 0;

			RoleSequences::iterator it = m_roleSequences.begin();
			RoleSequences::iterator end = m_roleSequences.end();

			for ( ; it != end; ++it)
			{
				ECoordinationState state = it->GetState(coordinationID);
				
				finishedCount += ((state == CoordinationFinished) || (state == CoordinationWaiting)) ? 1 : 0;
				failedCount += (state == CoordinationFailed) ? 1 : 0;
			}

			/*if (failedCount)	// TODO: 1 failed means all failed?
			{

				running.state = CoordinationFailed;
				continue;
			}*/

			if ((finishedCount + failedCount) == m_roleSequences.size())
			{
				running.state = CoordinationFinished;
				
				continue;
			}
		}
	}
}

ECoordinationState CoordinationStep::GetTaskState(const CoordinationID& coordinationID, int roleID, int taskID) const
{
	RoleSequences::const_iterator it = m_roleSequences.begin();
	RoleSequences::const_iterator end = m_roleSequences.end();

	for ( ; it != end; ++it)
	{
		if (it->GetRoleID() != roleID)
			continue;

		return it->GetTaskState(coordinationID, taskID);
	}

	assert(0);

	return CoordinationNotRunning;
}

ECoordinationState CoordinationStep::GetRoleState(const CoordinationID& coordinationID, int roleID) const
{
	RoleSequences::const_iterator it = m_roleSequences.begin();
	RoleSequences::const_iterator end = m_roleSequences.end();

	for ( ; it != end; ++it)
	{
		if (it->GetRoleID() != roleID)
			continue;

		return it->GetState(coordinationID);
	}

	assert(0);

	return CoordinationNotRunning;
}

int CoordinationStep::GetCurrentTask(const CoordinationID& coordinationID, int roleID) const
{
	RoleSequences::const_iterator it = m_roleSequences.begin();
	RoleSequences::const_iterator end = m_roleSequences.end();

	for ( ; it != end; ++it)
	{
		if (it->GetRoleID() != roleID)
			continue;

		return it->GetCurrentTask(coordinationID);
	}

	return -1;
}

const char* CoordinationStep::GetTaskName(int roleID, int taskID) const
{
	RoleSequences::const_iterator it = m_roleSequences.begin();
	RoleSequences::const_iterator end = m_roleSequences.end();

	for ( ; it != end; ++it)
	{
		if (it->GetRoleID() != roleID)
			continue;

		return it->GetTaskName(taskID);
	}

	assert(0);

	return 0;
}