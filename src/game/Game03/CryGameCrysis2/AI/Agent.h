#pragma once

#ifndef Agent_h
#define Agent_h

#include <IAgent.h>
#include <IEntity.h>
#include <IAIActor.h>
#include <IAIObject.h>
#include <Cry_Vector3.h>

struct VisionID;

// Serves as a unified interface for an agent
class Agent
{
public:
	Agent(IAIObject* pAIObject)
	{
		if (pAIObject)
		{
			SetFrom(pAIObject->GetEntityID());
		}
	}

	Agent(EntityId entityID)
	{
		SetFrom(entityID);
	}

	void SetFrom(EntityId entityID)
	{
		if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityID))
		{
			if (IAIObject* pAIObject = pEntity->GetAI())
			{
				if (IPuppet* pPuppet = pAIObject->CastToIPuppet())
				{
					if (IAIActor* pAIActor = pAIObject->CastToIAIActor())
					{
						if (IPipeUser* pPipeUser = pAIObject->CastToIPipeUser())
						{
							m_pAIObject = pAIObject;
							m_pPipeUser = pPipeUser;
							m_pAIActor = pAIActor;
							m_pPuppet = pPuppet;
							return;
						}
					}
				}
			}
		}

		m_pAIObject = NULL;
		m_pPipeUser = NULL;
		m_pAIActor = NULL;
		m_pPuppet = NULL;
	}

	const Vec3& GetPos() const
	{
		return  m_pAIObject->GetPos();
	}

	const Vec3& GetViewDir() const
	{
		return m_pAIObject->GetViewDir();
	}

	int GetGroupID() const
	{
		return m_pAIObject->GetGroupId();
	}

	//const VisionID& GetVisionID() const //Diesel cut
	//{ //Diesel cut
	//	//return m_pAIObject->GetVisionID(); //Diesel cut
	//} //Diesel cut

	bool CanSee(const VisionID& otherVisionID) const
	{
		//return m_pAIActor->CanSee(otherVisionID); //Diesel cut
		return 0; //Diesel new
	}

	bool CanSee(const Agent& otherAgent) const
	{
		//return m_pAIActor->CanSee(otherAgent.GetVisionID()); //Diesel cut
		return 0; //Diesel new
	}

 	EntityId GetEntityID() const
 	{
 		return m_pAIObject->GetEntityID();
 	}

	bool IsValid() const
	{
		return m_pPuppet && m_pAIObject && m_pAIActor && m_pPipeUser;
	}

	const char* GetName() const
	{
		return m_pAIObject->GetName();
	}

	int GetSpecies() const
	{
		return m_pAIObject->GetSpecies();
	}

	EAITargetThreat GetTargetThreat() const
	{
		const SOBJECTSTATE* pObjectState = m_pAIActor->GetState();
		assert(pObjectState);
		return pObjectState->eTargetThreat;
	}

	IAIObject* GetAttentionTarget() const
	{
		return m_pPipeUser->GetAttentionTarget();
	}

	void SetSignal(int signalID, const char* pText)
	{
		// This doesn't pass in sender, CRC and data for now.
		m_pAIActor->SetSignal(signalID, pText);
	}

	operator bool () const
	{
		return IsValid();
	}

	const Agent& operator = (const Agent& rhs)
	{
		m_pAIObject = rhs.m_pAIObject;
		m_pPipeUser = rhs.m_pPipeUser;
		m_pAIActor = rhs.m_pAIActor;
		m_pPuppet = rhs.m_pPuppet;
		return *this;
	}

private:
	Agent() {}

	IAIObject* m_pAIObject;
	IPipeUser* m_pPipeUser;
	IAIActor* m_pAIActor;
	IPuppet* m_pPuppet;
};

inline float SquaredDistance(const Agent& agentA, const Agent& agentB)
{
	return agentA.GetPos().GetSquaredDistance(agentB.GetPos());
}

#endif // Agent_h
