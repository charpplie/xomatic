#include "StdAfx.h"
#include "CoordinationActorAssignment.h"
#include "CoordinationScriptEnvironment.h"


CoordinationActorAssignment::CoordinationActorAssignment(
	const CoordinationActors& actors, const CoordinationRoles& roles)
	: m_actors(actors)
	, m_roles(roles)
{
}

void CoordinationActorAssignment::ReserveRoles(int roleCount)
{
	m_workingRoles.reserve(roleCount);
}

void CoordinationActorAssignment::AddRole(int roleId)
{
	m_workingRoles.push_back(SWorkingRole(roleId));
}

void CoordinationActorAssignment::Process(const CoordinationScriptEnvironment& environment)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	CTimeValue start = gEnv->pTimer->GetAsyncTime();

	ScoreCandidates(environment);
	SortRolesByBestCandidate();

	uint32 assignedCount = 0;
	uint32 availableCount = m_actors.size();
	uint32 roleCount = m_workingRoles.size();
	uint32 roleIdx = 0;

	// First iteration only fulfil minimum requirements
	while ((assignedCount < availableCount) && (roleIdx < roleCount))
	{
		SWorkingRole& workingRole = m_workingRoles[roleIdx++];
		const CoordinationRole& role = m_roles[workingRole.roleId];

		if (!workingRole.candidates.empty() 
			&& (workingRole.assignees.size() < role.GetMinActors()))
		{
			roleIdx = 0;
			++assignedCount;

			CoordinationActor candidate = workingRole.candidates.front().actor;
			workingRole.assignees.push_back(candidate);

			InvalidateCandidate(candidate);

			SortRolesByBestCandidate();
		}
	}

	roleIdx = 0;
	SortRolesByNeed();

	// Second iteration will distribute remaining actors by need
	uint32 loopAssigned = 0;
	while (assignedCount < availableCount)
	{
		SWorkingRole& workingRole = m_workingRoles[roleIdx++];
		const CoordinationRole& role = m_roles[workingRole.roleId];

		if (!workingRole.candidates.empty() 
			&& (workingRole.assignees.size() < role.GetMaxActors()))
		{
			++assignedCount;
			++loopAssigned;

			CoordinationActor candidate = workingRole.candidates.front().actor;
			workingRole.assignees.push_back(candidate);

			InvalidateCandidate(candidate);
		}

		if (roleIdx >= roleCount)
		{
			if (!loopAssigned)
				break;

			loopAssigned = 0;
			roleIdx = 0;
		}
	}

	CTimeValue duration = gEnv->pTimer->GetAsyncTime() - start;

	static bool showBreakdown = true;
	if (showBreakdown)
	{
		CryLogAlways("Role Assignment Breakdown");
		CryLogAlways("---------------------------------");

		uint32 roleCount = m_workingRoles.size();
		if (roleCount > 0)
		{
			uint32 actorCount = m_actors.size();
			uint32 assignedCount = 0;

			for (int i = 0; i < roleCount; ++i)
			{
				SWorkingRole& workingRole = m_workingRoles[i];
				const CoordinationRole& role = m_roles[workingRole.roleId];

				uint32 assigneeCount = workingRole.assignees.size();
				assignedCount += assigneeCount;

				string assignees;

				for (uint i = 0; i < workingRole.assignees.size(); ++i)
				{
					assignees += workingRole.assignees[i].GetName();
					if (i < workingRole.assignees.size()-1)
						assignees += ", ";
				}

				CryLogAlways("%s (%d/%d) Assigned: %d - %s", role.GetName(), role.GetMinActors(), 
					role.GetMaxActors() > 88 ? 88 : role.GetMaxActors(), workingRole.assignees.size(), assignees.c_str());
			}
		}

		CryLogAlways("Available: %d // Assigned: %d // Unassigned: %d", m_actors.size(), assignedCount,
			m_actors.size()-assignedCount);

		CryLogAlways("Assigning %d actors to %d roles took %.3fms...", m_actors.size(), roleCount, duration.GetMilliSeconds());
	}
}

const CoordinationActors& CoordinationActorAssignment::GetAssignees(int roleId) const
{
	uint32 roleCount = m_workingRoles.size();

	for (int i = 0; i < roleCount; ++i)
	{
		const SWorkingRole& workingRole = m_workingRoles[i];
		if (workingRole.roleId == roleId)
			return workingRole.assignees;
	}

	assert(0);

	static CoordinationActors empty;

	return empty;
}

void CoordinationActorAssignment::ScoreCandidates(const CoordinationScriptEnvironment& environment)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	uint32 roleCount = m_workingRoles.size();
	uint32 actorCount = m_actors.size();

	for (uint32 a = 0; a < actorCount; ++a)
	{
		const CoordinationActor& actor = m_actors[a];

		environment.SetActor(actor);
		environment.Apply();

		for (int i = 0; i < roleCount; ++i)
		{
			SWorkingRole& workingRole = m_workingRoles[i];
			const CoordinationRole& role = m_roles[workingRole.roleId];

			float score = role.GetActorScore(actor);
			workingRole.candidates.push_back(SCandidate(actor, score));
		}
	}

	for (int i = 0; i < roleCount; ++i)
	{
		SWorkingRole& workingRole = m_workingRoles[i];

		std::sort(workingRole.candidates.begin(), workingRole.candidates.end());
	}
}

void CoordinationActorAssignment::InvalidateCandidate(const CoordinationActor& actor)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	uint32 roleCount = m_workingRoles.size();

	for (int i = 0; i < roleCount; ++i)
	{
		SWorkingRole& workingRole = m_workingRoles[i];

		Candidates::iterator it = workingRole.candidates.begin();
		Candidates::iterator end = workingRole.candidates.end();
		
		for ( ; it != end; ++it)
		{
			if (it->actor == actor)
			{
				workingRole.candidates.erase(it);
				break;
			}
		}
	}
}

void CoordinationActorAssignment::SortRolesByBestCandidate()
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	std::sort(m_workingRoles.begin(), m_workingRoles.end());
}

void CoordinationActorAssignment::SortRolesByNeed()
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	std::sort(m_workingRoles.begin(), m_workingRoles.end(), sort_by_need(m_roles));
}
