#ifndef __CoordinationActorAssigment_h__
#define __CoordinationActorAssigment_h__

#pragma once


#include "Coordination.h"
#include "CoordinationRole.h"


class CoordinationActorAssignment
{
public:
	CoordinationActorAssignment(const CoordinationActors& actors,
		const CoordinationRoles& roles);
	
	void ReserveRoles(int roleCount);
	void AddRole(int roleId);

	void Process(const CoordinationScriptEnvironment& environment);

	const CoordinationActors& GetAssignees(int roleId) const;

private:
	const CoordinationActors& m_actors;
	const CoordinationRoles& m_roles;

	void ScoreCandidates(const CoordinationScriptEnvironment& environment);
	void InvalidateCandidate(const CoordinationActor& actor);
	void SortRolesByBestCandidate();
	void SortRolesByNeed();

	struct SCandidate
	{
		SCandidate(const CoordinationActor& _actor, float _score)
			: actor(_actor)
			, score(_score)
		{
		}

		CoordinationActor actor;
		float score;

		bool operator<(const SCandidate& other) const
		{
			return score > other.score;
		}
	};

	typedef std::vector<SCandidate> Candidates;

	struct SWorkingRole
	{
		SWorkingRole(int _roleId)
			: roleId(_roleId)
		{
		}

		int roleId;

		Candidates candidates;
		CoordinationActors assignees;

		bool operator<(const SWorkingRole& other) const
		{
			if (candidates.empty())
				return false;

			if (other.candidates.empty())
				return true;

			return candidates.front().score > other.candidates.front().score;
		}
	};

	struct sort_by_need
	{
		sort_by_need(const CoordinationRoles& _roles)
			: roles(_roles)
		{
		}

		bool operator()(const SWorkingRole& lhs, const SWorkingRole& rhs) const
		{
			const CoordinationRole& l = roles[lhs.roleId];
			const CoordinationRole& r = roles[rhs.roleId];

			uint32 needL = l.GetMaxActors()-lhs.assignees.size();
			uint32 needR = r.GetMaxActors()-rhs.assignees.size();

			if (needL != needR)
				return (needL > needR);
			else
				return lhs.operator < (rhs);
		}

		const CoordinationRoles& roles;
	};

	typedef std::vector<SWorkingRole> WorkingRoles;

	WorkingRoles m_workingRoles;
};


#endif //__CoordinationActorAssigment_h__