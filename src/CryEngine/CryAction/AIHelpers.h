/********************************************************************
CryGame Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   AIHelpers.h
Version:     v1.00
Description: 

-------------------------------------------------------------------------
History:
- 31:07:2009 Created by Alex McCarthy

*********************************************************************/


#ifndef __AIHelpers_H__
#define __AIHelpers_H__

#include <IAISystem.h>
#include <IAgent.h>
#include <IEntitySystem.h>

enum EAIAlertLevel
{
	// Note: Numbers matter! Matches up with internal CryAISystem usage
	eAIAlert_Invalid	= -1,
	eAIAlert_Green		= 0,
	eAIAlert_Yellow		= 1,
	eAIAlert_Red		= 2,

	eAIAlert_COUNT,
};

namespace AIHelpers
{
	static bool GetAITarget(IAIObject *pAI, EntityId &targetId)
	{
		CRY_ASSERT(pAI);
		targetId = 0;

		if (pAI)
		{
			IPipeUser *pAIPipeUser = pAI->CastToIPipeUser();
			IPuppet *pAIPuppet = pAI->CastToIPuppet();

			IAIObject *pAITarget = (pAIPipeUser ? pAIPipeUser->GetAttentionTarget() : NULL);

			// Use event owner if possible
			IAIObject *pAITargetOwner = (pAIPuppet ? pAIPuppet->GetEventOwner(pAITarget) : NULL);
			if (pAITargetOwner)
			{
				targetId = pAITargetOwner->GetEntityID();
			}

			// Try using the association (for sound target firing, etc)
			if (!targetId && pAIPipeUser)
			{
				IAIObject *pAITargetAssociation = pAIPipeUser->GetAttentionTargetAssociation();
				if (pAITargetAssociation)
				{
					targetId = pAITargetAssociation->GetEntityID();
				}
			}

			// Finally, try using the target itself
			if (!targetId && pAITarget)
			{
				targetId = pAITarget->GetEntityID();
			}
		}

		return (targetId > 0);
	}

	struct SAreaAlert : public IAIAlertnessPredicate
	{
		SAreaAlert(const Vec3& vPos, float fRadius) : m_vPos(vPos), m_fRadiusSqr(fRadius * fRadius)
		{
			CRY_ASSERT(vPos.IsValid());
			CRY_ASSERT(fRadius > FLT_EPSILON);
			CRY_ASSERT(m_fRadiusSqr > FLT_EPSILON);
		}

		virtual bool ConsiderAIObject(IAIObject* pAIObject) const
		{
			return (m_fRadiusSqr > FLT_EPSILON)
				&& pAIObject->IsEnabled()
				&& (pAIObject->GetPos().GetSquaredDistance(m_vPos) <= m_fRadiusSqr);
		}

		const Vec3& m_vPos;
		float m_fRadiusSqr;
	};

	struct STargetAlert : public IAIAlertnessPredicate
	{
		STargetAlert(EntityId entityId) : m_entityId(entityId)
		{
			CRY_ASSERT(entityId > 0);
		}

		virtual bool ConsiderAIObject(IAIObject* pAIObject) const
		{
			EntityId targetId = 0;
			return pAIObject->IsEnabled() && GetAITarget(pAIObject, targetId) && targetId == m_entityId;
		}

		EntityId m_entityId;
	};



	static EAIAlertLevel GetAreaAlertLevel(const Vec3& vPos, float fRadius)
	{
		CRY_ASSERT(vPos.IsValid());
		CRY_ASSERT(fRadius > FLT_EPSILON);
		SAreaAlert areaAlert(vPos, fRadius);
		return static_cast<EAIAlertLevel>(gEnv->pAISystem->GetAlertness(areaAlert));
	}

	static EAIAlertLevel GetAreaAlertLevel(IAIObject *pAI, float fRadius)
	{
		CRY_ASSERT(pAI);
		CRY_ASSERT(fRadius > FLT_EPSILON);

		return (pAI ? GetAreaAlertLevel(pAI->GetPos(), fRadius) : eAIAlert_Invalid);
	}

	static EAIAlertLevel GetAreaAlertLevel(IEntity *pEntity, float fRadius)
	{
		CRY_ASSERT(pEntity);
		CRY_ASSERT(fRadius > FLT_EPSILON);

		return (pEntity ? GetAreaAlertLevel(pEntity->GetWorldPos(), fRadius) : eAIAlert_Invalid);
	}

	static EAIAlertLevel GetAreaAlertLevel(EntityId entityId, float fRadius)
	{
		CRY_ASSERT(entityId > 0);
		CRY_ASSERT(fRadius > FLT_EPSILON);

		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entityId);
		return (pEntity ? GetAreaAlertLevel(pEntity, fRadius) : eAIAlert_Invalid);
	}




	static EAIAlertLevel GetTargetAlertLevel(EntityId entityId)
	{
		CRY_ASSERT(entityId > 0);
		STargetAlert targetAlert(entityId);
		return static_cast<EAIAlertLevel>(gEnv->pAISystem->GetAlertness(targetAlert));
	}

	static EAIAlertLevel GetTargetAlertLevel(IAIObject *pAI)
	{
		CRY_ASSERT(pAI);

		return (pAI ? GetTargetAlertLevel(pAI->GetEntityID()) : eAIAlert_Invalid);
	}

	static EAIAlertLevel GetTargetAlertLevel(IEntity *pEntity)
	{
		CRY_ASSERT(pEntity);

		return (pEntity ? GetTargetAlertLevel(pEntity->GetId()) : eAIAlert_Invalid);
	}

}

#endif // __AIHelpers_H__
