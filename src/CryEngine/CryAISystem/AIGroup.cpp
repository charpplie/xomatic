/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   AIGroup.cpp
$Id$
Description: 

-------------------------------------------------------------------------
History:
- 2006				: Created by Mikko Mononen
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/


#include "StdAfx.h"
#include "IAISystem.h"
#include "CAISystem.h"
#include "AIPlayer.h"
#include "AIGroup.h"
#include "Puppet.h"
#include "AIVehicle.h"
#include <float.h>
#include "AIGroupTactic.h"
#include "DebugDrawContext.h"



// Serialises a container of AI references
// Perhaps this is too heavy on the templating and it could be virtualised
template < class T >
bool SerializeWeakRefVector(TSerialize ser, const char* containerName, std::vector < CWeakRef <T> > & container)
{
	ser.BeginGroup(containerName);
	unsigned numItems = container.size();
	ser.Value("numItems", numItems);
	if (ser.IsReading())
		container.resize(numItems);

	for( typename std::vector< CWeakRef <T> >::iterator itr(container.begin()); itr!=container.end(); ++itr )
	{
		ser.BeginGroup("i");
		itr->Serialize(ser, "v");
		ser.EndGroup();
	}
	ser.EndGroup();
	return true;
}

//====================================================================
// CAIGroup
//====================================================================
CAIGroup::CAIGroup(int groupID) :
	m_groupID(groupID),
	m_pLeader(0),
	m_bUpdateEnemyStats(true),
	m_vEnemyPositionKnown(0,0,0),
	m_vAverageEnemyPosition(0,0,0),
	m_vAverageLiveEnemyPosition(0,0,0),
	m_lastUpdateTime(-1.0f),
	m_countMax(0),
	m_countEnabledMax(0),
	m_reinforcementSpotsAcquired(false),
	m_reinforcementState(REINF_NONE),
	m_countDead(0),
	m_countCheckedDead(0)
{
	CCCPOINT(CAIGroup_CAIGroup)
}

//====================================================================
// ~CAIGroup
//====================================================================
CAIGroup::~CAIGroup()
{
	CCCPOINT(CAIGroup_Destructor)
	for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		delete it->second;
	m_tactics.clear();
}

//====================================================================
// SetLeader
//====================================================================
void CAIGroup::SetLeader(CLeader* pLeader)
{
	CCCPOINT(CAIGroup_SetLeader)
	m_pLeader = pLeader;
	if(pLeader)
		pLeader->SetAIGroup(this);
}

//====================================================================
// GetLeader
//====================================================================
CLeader* CAIGroup::GetLeader()
{
	return m_pLeader;
}

//====================================================================
// AddMember
//====================================================================
void	CAIGroup::AddMember(CAIActor* pMember)
{
	CCCPOINT(CAIGroup_AddMember)

	for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		it->second->OnMemberAdded(pMember);

	// everywhere assumes that pMember is non-null
	AIAssert(pMember);
	// check if it's already a member of team
	if(GetUnit(pMember) != m_Units.end())
		return;
	CUnitImg	newUnit;
	newUnit.m_refUnit = GetWeakRef(pMember);
	SetUnitClass(newUnit);

	IEntity* pEntity = pMember->GetEntity();
	if(pEntity)
	{
		IPhysicalEntity* pPhysEntity = pEntity->GetPhysics();
		if(pPhysEntity)
		{
			CCCPOINT(CAIGroup_AddMember_Phys)

			float radius;
			pe_status_pos status;
			pPhysEntity->GetStatus( &status );
			radius = fabsf(status.BBox[1].x - status.BBox[0].x);
			float radius2 = fabsf(status.BBox[1].y - status.BBox[0].y);
			if(radius < radius2)
				radius = radius2;
			float myHeight = status.BBox[1].z - status.BBox[0].z;
			newUnit.SetWidth(radius);
			newUnit.SetHeight(myHeight);
		}
	}
	m_Units.push_back(newUnit);

	SetUnitRank((IAIObject*)pMember);

	if(m_pLeader)
	{
		m_pLeader->AddUnitNotify(pMember);
	}
}

//====================================================================
// RemoveMember
//====================================================================
bool	CAIGroup::RemoveMember(CAIActor* pMember)
{
	if (!pMember)
		return false;

	CCCPOINT(CAIGroup_RemoveMember);

	// (MATT) This will need cleanup {2009/02/12}
	if(m_refReinfPendingUnit == pMember)
		NotifyReinfDone( pMember, false );

	for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		it->second->OnMemberRemoved(pMember);

	if (pMember->GetProxy() && pMember->GetProxy()->GetActorHealth() <= 0)
		m_countDead++;

	TUnitList::iterator theUnit = std::find(m_Units.begin(), m_Units.end(), pMember);

	if(m_pLeader)
	{
		CAIActor* pMemberActor = pMember->CastToCAIActor();
		if(m_pLeader->GetFormationOwner() == pMember && pMemberActor->GetGroupId() == m_pLeader->GetGroupId())
			m_pLeader->ReleaseFormation();

		if(m_pLeader->GetAssociation() == pMember)
			m_pLeader->SetAssociation(NILREF);
	}

	RemoveUnitFromRankList(pMember);

	if(theUnit != m_Units.end())
	{
		m_Units.erase(theUnit);
		if(m_pLeader)
		{
			// if empty group - remove the leader; nobody owns it, will hang there forever
// ok, don't remove leader here - all serialization issues are taken care of; make sure chain-loading is fine
//			if(m_Units.empty())
//			{
//				if(deleting)
//				{
//					GetAISystem()->RemoveObject(m_pLeader);
//					m_pLeader = 0;
//				}
//			}
//			else
			if(!m_Units.empty())
			{
				m_pLeader->DeadUnitNotify(pMember);
			}
		}
		m_bUpdateEnemyStats = true;

		return true;
	}
	return false;
}

//====================================================================
// NotifyBodyChecked
//====================================================================
void CAIGroup::NotifyBodyChecked(const CAIObject* pObject)
{
	m_countCheckedDead++;
}

//====================================================================
// GetUnit
//====================================================================
TUnitList::iterator CAIGroup::GetUnit(const CAIActor * pAIObject) 
{
	CCCPOINT(CAIGroup_GetUnit);
	return std::find(m_Units.begin(),m_Units.end(), pAIObject);
}


//====================================================================
// SetUnitClass
//====================================================================
void CAIGroup::SetUnitClass(CUnitImg& UnitImg) const
{
	CAIObject * const pObject = UnitImg.m_refUnit.GetAIObject();
	IEntity * const pEntity = (pObject ? pObject->GetEntity() : NULL);
	if(pEntity)
	{
		IScriptTable* pTable = pEntity->GetScriptTable();
		SmartScriptTable pEntityProperties;
		if(pTable && pTable->GetValue("Properties",pEntityProperties))
		{
			const char* sCharacter=NULL;
			pEntityProperties->GetValue("aicharacter_character",sCharacter);
			IScriptSystem * pSS = pTable->GetScriptSystem();
			if(pSS)
			{
				SmartScriptTable pGlobalCharacterTable;
				if(pSS->GetGlobalValue("AICharacter",pGlobalCharacterTable))
				{
					SmartScriptTable pMyCharacterTable;
					if(pGlobalCharacterTable->GetValue(sCharacter,pMyCharacterTable))
					{
						int soldierClass;
						if(pMyCharacterTable->GetValue("Class", soldierClass))
						{
							CCCPOINT(CAIGroup_SetUnitClass);

							UnitImg.SetClass(static_cast<int>(soldierClass));
							return;
						}
					}
				}
			}
		}
	}
	UnitImg.SetClass(UNIT_CLASS_UNDEFINED);
}


//====================================================================
// IsMember
//====================================================================
bool CAIGroup::IsMember(const IAIObject* pMember ) const
{
	const CAIActor* pActor = pMember->CastToCAIActor();
	for(TUnitList::const_iterator itUnit = m_Units.begin(); itUnit != m_Units.end(); ++itUnit)
		if ((*itUnit)==pActor)
			return true;
	return false;
}

//====================================================================
// SetUnitProperties
//====================================================================
void CAIGroup::SetUnitProperties( const IAIObject* obj, uint32 properties )
{
	const CAIActor* pActor = obj->CastToCAIActor();
	TUnitList::iterator senderUnit = std::find(m_Units.begin(), m_Units.end(), pActor);

	if (senderUnit != m_Units.end())
		senderUnit->SetProperties(properties);
}

//====================================================================
// GetAveragePosition
//====================================================================
Vec3 CAIGroup::GetAveragePosition(IAIGroup::eAvPositionMode mode, uint32 unitClassMask) const
{
	// (MATT) Restructured this {2009/02/13}

	int count = 0;
	Vec3 average(ZERO);
	TUnitList::const_iterator it, itEnd = m_Units.end();
	for (it = m_Units.begin(); it != itEnd; ++it)
	{
		CAIObject * const pUnit = it->m_refUnit.GetAIObject();
		if (!pUnit->IsEnabled())
			continue;
		bool bInclude = false;

		switch(mode)
		{
		case AVMODE_ANY:
			bInclude = true;
			break;
		case AVMODE_CLASS:
			bInclude = (it->GetClass() & unitClassMask) != 0;
			break;
		case AVMODE_PROPERTIES:
			bInclude = (it->GetProperties() & unitClassMask) != 0;
			break;
		default:
			break;
		}

		CCCPOINT(CAIGroup_GetAveragePosition);

		if (bInclude)
		{
			++count;
			average += pUnit->GetPos();
		}
	}
	return (count? average / float(count): average);
}

//====================================================================
// GetUnitCount
//====================================================================
int CAIGroup::GetUnitCount(uint32 unitPropMask) const
{

	int count = 0;
	TUnitList::const_iterator it, itEnd = m_Units.end();
	for (it = m_Units.begin(); it != itEnd; ++it)
		if (it->m_refUnit.GetAIObject()->IsEnabled() && (it->GetProperties() & unitPropMask))
		{
			CCCPOINT(CAIGroup_GetUnitCount);
			++count;
		}
	return count;
}

//====================================================================
// GetAttentionTarget
//====================================================================
CWeakRef<CAIObject> CAIGroup::GetAttentionTarget(bool bHostileOnly, bool bLiveOnly, const CWeakRef<CAIObject> refSkipTarget) const
{
	CCCPOINT(CAIGroup_GetAttentionTarget);

	// to do: give more priority to closer targets?
	CWeakRef<CAIObject> refSelectedTarget;
	bool bHostileFound = false;

	TUnitList::const_iterator it, itEnd = m_Units.end();
	for (it = m_Units.begin(); it != itEnd; ++it)
	{
		CPipeUser* pPipeUser = CastToCPipeUserSafe(it->m_refUnit.GetAIObject());
		if (pPipeUser)
		{
			CAIObject* attentionTarget = static_cast<CAIObject*>(pPipeUser->GetAttentionTarget());
			if (attentionTarget && (attentionTarget->IsEnabled() || attentionTarget->GetAIType()==AIOBJECT_VEHICLE) && refSkipTarget!=attentionTarget)
			{	
				CWeakRef<CAIObject> refAttentionTarget = GetWeakRef(attentionTarget);
				bool bLive = attentionTarget->IsAgent();
				if(pPipeUser->IsHostile(attentionTarget)) 
				{
					if(bLive)
						return refAttentionTarget;//give priority to live hostile targets
					else if(!bLiveOnly)
					{
						bHostileFound = true;
						refSelectedTarget = refAttentionTarget; // then to hostile non live targets
						CCCPOINT(CAIGroup_GetAttentionTarget_A);
					}
				}
				else if(!bHostileFound && !bHostileOnly && (bLive ||!bLiveOnly))
					refSelectedTarget = refAttentionTarget; // then to non hostile targets
			}
		}
	}
	if(m_pLeader && m_pLeader->IsPlayer())
	{
		CAIPlayer* pPlayer = CastToCAIPlayerSafe(m_pLeader->GetAssociation().GetAIObject());
		if(pPlayer)
		{
			CWeakRef<CAIObject> refPlayerAttTarget = pPlayer->GetAttentionTarget();
			CAIObject* pPlayerAttTarget = refPlayerAttTarget.GetAIObject();
			if(pPlayerAttTarget && pPlayerAttTarget->IsEnabled() && refPlayerAttTarget!=refSkipTarget) // nothing else to check, assuming that player has only live enemy targets
			{	
				bool bLive = pPlayerAttTarget->IsAgent();
				if(pPlayer->IsHostile(pPlayerAttTarget))
				{
					if(bLive)
						return refPlayerAttTarget;//give priority to live hostile targets
					else if(!bLiveOnly)
						refSelectedTarget = refPlayerAttTarget; // then to hostile non live targets
				}
				else if(!bHostileFound && !bHostileOnly && (bLive ||!bLiveOnly))
					refSelectedTarget = refPlayerAttTarget; // then to non hostile targets

				CCCPOINT(CAIGroup_GetAttentionTarget_B);
			}
		}
	}

	return refSelectedTarget;
}

//====================================================================
// GetTargetCount
//====================================================================
int CAIGroup::GetTargetCount(bool bHostileOnly, bool bLiveOnly) const
{
	CCCPOINT(CAIGroup_GetTargetCount);
	int n = 0;

	TUnitList::const_iterator it, itEnd = m_Units.end();
	for (it = m_Units.begin(); it != itEnd; ++it)
	{
		CPipeUser* pPipeUser = CastToCPipeUserSafe(it->m_refUnit.GetAIObject());
		if (pPipeUser)
		{
			CAIObject* attentionTarget = static_cast<CAIObject*>(pPipeUser->GetAttentionTarget());
			if (attentionTarget && attentionTarget->IsEnabled())
			{	
				bool bLive = attentionTarget->IsAgent();
				bool bHostile = pPipeUser->IsHostile(attentionTarget);
				if(!(bLiveOnly && !bLive || bHostileOnly && !bHostile))
					n++;
			}
		}
	}
	if(m_pLeader && m_pLeader->IsPlayer())
	{
		CAIPlayer* pPlayer = (CAIPlayer*)m_pLeader->GetAssociation().GetAIObject();;
		if(pPlayer)
		{
			CWeakRef<CAIObject> refAttentionTarget = pPlayer->GetAttentionTarget();
			CAIObject* pAttentionTarget = refAttentionTarget.GetAIObject();
			if(pAttentionTarget && pAttentionTarget->IsEnabled()) // nothing else to check, assuming that player has only live enemy targets
			{	
				bool bLive = pAttentionTarget->IsAgent();
				bool bHostile = pPlayer->IsHostile(pAttentionTarget);
				if(!(bLiveOnly && !bLive || bHostileOnly && !bHostile))
					n++;
			}
		}
	}

	return n;
}


//====================================================================
// SetUnitRank
//====================================================================
void	CAIGroup::SetUnitRank(const IAIObject* pMember, int rank)
{
	if(!pMember)
		return;
	const CAIActor* pActor = pMember->CastToCAIActor();
	TUnitList::iterator itUnit2 = GetUnit(pActor);
	if(itUnit2!=m_Units.end())
	{
		if(rank==0)
			rank = pActor->GetParameters().m_nRank;
		RemoveUnitFromRankList(pMember);
		m_RankedUnits.insert(std::pair < int, CWeakRef<CAIObject> > (rank, GetWeakRef( (CAIObject*) pMember)) );
	}
}

//====================================================================
// GetUnitInRank
//====================================================================
IAIObject* CAIGroup::GetUnitInRank(int rank) const
{
	TUnitMap::const_iterator itUnit;
	if(rank<=0)
		itUnit=m_RankedUnits.begin();
	else
		itUnit = m_RankedUnits.find(rank);

	return(itUnit!=m_RankedUnits.end()? itUnit->second.GetIAIObject() : NULL);
}

//====================================================================
// RemoveUnitFromRankList
//====================================================================
void	CAIGroup::RemoveUnitFromRankList(const IAIObject* pMember)
{
	TUnitMap::iterator itEnd = m_RankedUnits.end();
	TUnitMap::iterator itUnit= m_RankedUnits.begin();
	for(; itUnit!=itEnd;++itUnit)
		if(itUnit->second == pMember)
			break;
	if(itUnit!=itEnd)
		m_RankedUnits.erase(itUnit);
}

//====================================================================
// GetEnemyPosition
//====================================================================
Vec3 CAIGroup::GetEnemyPosition(const CAIObject* pUnit) const
{
	Vec3 enemyPos(0,0,0);
	const CPipeUser* pPipeUser = CastToCPipeUserSafe(pUnit);
	if (pPipeUser)
	{
		IAIObject* pAttTarget = pPipeUser->GetAttentionTarget();
		if (pAttTarget)
		{
			CCCPOINT(CAIGroup_GetEnemyPosition_A);

			switch (((CAIObject*)pAttTarget)->GetType())
			{
			case AIOBJECT_NONE:
				break;
			case AIOBJECT_DUMMY:
			case AIOBJECT_PLAYER:
			case AIOBJECT_PUPPET:
			case AIOBJECT_WAYPOINT: // Actually this should be Beacon
			default:
				return pAttTarget->GetPos();
			}
		}
		else 
			return m_vAverageEnemyPosition;
	}
	else 
	{
		Vec3 averagePos = GetAveragePosition();

		int memory = 0;
		bool realTarget = false;
		float distance = 10000.f;

		TUnitList::const_iterator it, itEnd = m_Units.end();
		for (it = m_Units.begin(); it != itEnd; ++it)
		{
			CPipeUser * const pPipeUser = CastToCPipeUserSafe( it->m_refUnit.GetAIObject());
			if (!pPipeUser || !pPipeUser->IsEnabled())
				continue;

			CAIObject * const pTarget = (CAIObject*) pPipeUser->GetAttentionTarget();
			if (!pTarget || !pPipeUser->IsHostile(pTarget))
			{
				SOBJECTSTATE* state = pPipeUser->GetState();

				if (!realTarget)
				{
					if (state->eTargetType == AITARGET_MEMORY)
					{
						enemyPos += pTarget->GetPos();
						++memory;
					}
					else
					{
						realTarget = true;
						enemyPos = pTarget->GetPos();
						distance = (enemyPos - averagePos).GetLengthSquared();
					}
				}
				else if (state->eTargetType != AITARGET_MEMORY)
				{
					float thisDistance = (pPipeUser->GetPos() - averagePos).GetLengthSquared();
					if (distance > thisDistance)
					{
						enemyPos = pTarget->GetPos();
						distance = thisDistance;
					}
				}
			}
		}

		if(!realTarget && m_pLeader && m_pLeader->IsPlayer())
		{
			CAIPlayer* pPlayer = CastToCAIPlayerSafe(m_pLeader->GetAssociation().GetAIObject());
			if(pPlayer)
			{
				CAIObject* pTarget = pPlayer->GetAttentionTarget().GetAIObject();
				if(pTarget && pTarget->IsEnabled()) // nothing else to check, assuming that player has only live enemy targets
				{
					realTarget = true;
					enemyPos = pTarget->GetPos();
				}
			}
		}

		if (!realTarget && memory)
			enemyPos /= (float) memory;
	}

	CCCPOINT(CAIGroup_GetEnemyPosition_B);

	return enemyPos;
}


//====================================================================
// GetEnemyAverageDirection
//====================================================================
Vec3 CAIGroup::GetEnemyAverageDirection(bool bLiveTargetsOnly, bool bSmart) const 
{
	Vec3 avgDir(0,0,0);
	//	TVecTargets FoundTargets;
	//	TVecTargets::iterator itf;
	TSetAIObjects::const_iterator it, itEnd = m_Targets.end();
	int count=0;
	for (it = m_Targets.begin(); it != itEnd; ++it)
	{
		const CAIObject* pTarget = it->GetAIObject();
		bool bLiveTarget = pTarget->IsAgent();
		if(pTarget && (!bLiveTargetsOnly || bLiveTarget) && !pTarget->GetMoveDir().IsZero())
		{	
			// add only not null (significative) directions
			avgDir+=pTarget->GetMoveDir();
			//			FoundTargets.push_back(pTarget);
			count++;
		}

	}	
	if(count)
	{
		avgDir /= float(count);
		/*if(bSmart)// clear the average if it's small enough 
		//(means that the direction vectors are spread in many opposite directions)
		{

		// compute variance
		float fVariance = 0;
		for (itf = FoundTargets.begin(); itf != FoundTargets.end(); ++itf)
		{	CAIObject* pTarget = *itf;
		fVariance+= (pTarget->GetMoveDir() - avgDir).GetLengthSquared();
		}

		if(fVariance>0.7)
		avgDir.Set(0,0,0);

		}*/
		// either normalize or clear
		float fAvgSize = avgDir.GetLength();
		if(fAvgSize> (bSmart ? 0.4f :0.1f))
			avgDir /= fAvgSize;
		else
			avgDir.Set(0,0,0);
	}
	return avgDir;
}

//====================================================================
// UpdateEnemyStats
//====================================================================
void CAIGroup::UpdateEnemyStats()
{
FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );
	Vec3 vOldAverageLiveEnemyPosition = m_vAverageLiveEnemyPosition;
	m_vAverageEnemyPosition.Set(0,0,0);
	m_vAverageLiveEnemyPosition.Set(0,0,0);

	Vec3	averageEnemyDir( 0, 0, 0 );

	//	m_vEnemySize.Set(0,0,0);
	int count = 0, liveCount = 0;
	if(m_bUpdateEnemyStats)
	{
		TUnitList::iterator it, itEnd = m_Units.end();
		m_Targets.resize(0);
		std::multimap<CAIObject*,CAIObject*> DummyOwnersMap;

		for (it = m_Units.begin(); it != itEnd; ++it)
		{
			CPuppet* pUnitPuppet = CastToCPuppetSafe(it->m_refUnit.GetAIObject());
			if (pUnitPuppet)
			{	
				PotentialTargetMap targetMap;
				IPerceptionHandler *pPerceptionHandler = pUnitPuppet->GetPerceptionHandler();
				if (pPerceptionHandler)
					pPerceptionHandler->GetPotentialTargets(targetMap);

				PotentialTargetMap::iterator itEventEnd = targetMap.end();
				bool bAttTargetChecked = false;
				for(PotentialTargetMap::iterator itEvent = targetMap.begin();itEvent!=itEventEnd || !bAttTargetChecked; )
				{
					CAIObject* pTarget= NULL;
					if(!bAttTargetChecked && itEvent!=itEventEnd )
					{
						pTarget = itEvent->second.refDummyRepresentation.GetAIObject();
						++itEvent;
					}
					else
					{
						pTarget = (CAIObject*)pUnitPuppet->GetAttentionTarget();
						bAttTargetChecked = true;
					}

					if(pTarget && pTarget->IsEnabled() && 	pUnitPuppet->IsHostile(pTarget))
					{
						if(std::find(m_Targets.begin(),m_Targets.end(), pTarget)==m_Targets.end())
						{
							if(pTarget->IsAgent())
							{
								m_vAverageEnemyPosition += pTarget->GetPos();
								m_vAverageLiveEnemyPosition += pTarget->GetPos();
								averageEnemyDir += pTarget->GetMoveDir();
								m_Targets.push_back( GetWeakRef(pTarget) );
								++count;
								++liveCount;
							}
							else if (pTarget->GetSubType()==CAIObject::STP_MEMORY || pTarget->GetSubType()==CAIObject::STP_SOUND) // dummy target
							{	// avoid adding dummy objects (memory, sound etc) with the same owner more than once 
								// (unless they're far enough from each other)
								bool bAdd = true;
								CAIObject* pTargetOwner = static_cast<CAIObject*>(pUnitPuppet->GetEventOwner(itEvent->second.refDummyRepresentation.GetWeakRef()));
								std::multimap<CAIObject*,CAIObject*>::iterator itMO = DummyOwnersMap.find(pTargetOwner);
								if(itMO!=DummyOwnersMap.end())
								{
									for(; itMO!=DummyOwnersMap.end();++itMO)
									{
										if(itMO->first != pTargetOwner)
											break;
										else
										{// a target owned by the same current owner has already been added
											CAIObject* pTarget2 = itMO->second;
											if((pTarget->GetPos() - pTarget2->GetPos()).GetLengthSquared() <0.2 )
											{
												bAdd = false;
												break;
											}
										}
									}
								}
								if(bAdd)
								{
									averageEnemyDir += pTarget->GetMoveDir();
									m_vAverageEnemyPosition += pTarget->GetPos();
									m_Targets.push_back( GetWeakRef(pTarget) );
									DummyOwnersMap.insert(std::make_pair(pTargetOwner,pTarget));
									++count;
								}
							}
						}
					}
				}
			}
		}
		if( m_pLeader && m_pLeader->IsPlayer())
		{
			CAIPlayer* pPlayer = (CAIPlayer*)(m_pLeader->GetAssociation().GetAIObject());
			if(pPlayer)
			{
				CWeakRef<CAIObject> refAttentionTarget = pPlayer->GetAttentionTarget();
				CAIObject* pTarget = refAttentionTarget.GetAIObject();
				if(pTarget && pTarget->IsEnabled() && pTarget->IsHostile(pPlayer))
        {
					if(std::find(m_Targets.begin(),m_Targets.end(), refAttentionTarget)==m_Targets.end())
          {
						m_Targets.push_back( refAttentionTarget );
					  m_vAverageEnemyPosition +=pTarget->GetPos();
					  m_vAverageLiveEnemyPosition +=pTarget->GetPos();
					  averageEnemyDir += pTarget->GetMoveDir();
					  count++;
					  ++liveCount;
          }
				}
			}
		}
	}
	else //m_bUpdateEnemyStats
	{
		// update only average positions, using current target list
		TSetAIObjects::iterator it = m_Targets.begin(), itEnd = m_Targets.end();
		for(;it!=itEnd;++it)
		{
			const CAIObject* pTarget = it->GetAIObject();
			if(pTarget)
			{
				m_vAverageEnemyPosition += pTarget->GetPos();
				count++;
				if(pTarget->IsEnabled() && pTarget->IsAgent())
				{
					m_vAverageLiveEnemyPosition +=pTarget->GetPos();
					averageEnemyDir += pTarget->GetMoveDir();
					liveCount++;
				}
			}
		}
	}

	if(count>1)// avoid division by 1 for precision sake	
		m_vAverageEnemyPosition /= (float) count;

	if(liveCount>1)
		m_vAverageLiveEnemyPosition/= float(liveCount);


	m_bUpdateEnemyStats	= false;

	if(m_pLeader)
		m_pLeader->OnEnemyStatsUpdated(m_vAverageLiveEnemyPosition, vOldAverageLiveEnemyPosition);
}

//====================================================================
// OnObjectRemoved
//====================================================================
void CAIGroup::OnObjectRemoved(CAIObject* pObject)
{
	if(!pObject)
		return;
	CAIActor* pActor = pObject->CastToCAIActor();

	for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		it->second->OnMemberRemoved(pObject);

	TUnitList::iterator it = std::find(m_Units.begin(), m_Units.end(), pActor);
	if(it != m_Units.end())
		RemoveMember(pActor);

	// (MATT) If UpdateEnemyStats touches targets, this must happen after other cleanup has happened... {2009/03/17}
	TSetAIObjects::iterator ittgt = std::find(m_Targets.begin(), m_Targets.end(), pObject);
	if(ittgt != m_Targets.end())
	{
		m_bUpdateEnemyStats = true;
		UpdateEnemyStats();
	}

	for (unsigned i = 0; i < m_reinforcementSpots.size(); )
	{
		if (m_reinforcementSpots[i].pAI == pObject)
		{
			m_reinforcementSpots[i] = m_reinforcementSpots.back();
			m_reinforcementSpots.pop_back();
		}
		else
			++i;
	}
}

//====================================================================
// GetForemostUnit
//====================================================================
CAIObject* CAIGroup::GetForemostUnit(const Vec3& direction, uint32 unitProp) const
{
	Vec3 vFurthestPos(0,0,0);
	float maxLength = 0;
	CAIObject* pForemost=NULL;

	Vec3 vAvgPos = GetAveragePosition(AVMODE_PROPERTIES,unitProp);

	for (TUnitList::const_iterator it = m_Units.begin();it!=m_Units.end(); ++it)
	{
		const CUnitImg& unit = *it;
		CAIObject* pUnit = unit.m_refUnit.GetAIObject();
		if(pUnit->IsEnabled() && ( unit.GetProperties() & unitProp))
		{
			CCCPOINT(CAIGroup_ForemostUnit);

			Vec3 vDisplace = pUnit->GetPos() - vAvgPos;
			float fNorm = vDisplace.GetLength();
			if(fNorm>0.001f)//consider an epsilon
			{
				Vec3 vNorm = vDisplace/fNorm;
				float cosine = vNorm.Dot(direction);
				if(cosine>=0) // exclude negative cosine (opposite directions)
				{
					float fProjection = cosine*fNorm;
					if(maxLength<fProjection)
					{
						maxLength = fProjection;
						pForemost = pUnit;
					}
				}
			}
			else if(!pForemost)
			{
				// the unit is exactly at the average position and no one better has been found yet
				pForemost = pUnit;
			}
		}
	}
	return (pForemost);
}

//====================================================================
// GetForemostEnemyPosition
//====================================================================
Vec3 CAIGroup::GetForemostEnemyPosition(const Vec3& direction, bool bLiveOnly) const
{
	const CAIObject* pForemost = GetForemostEnemy(direction,bLiveOnly);
	return (pForemost ? pForemost->GetPos() : ZERO);
}

//====================================================================
// GetForemostEnemy
//====================================================================
const CAIObject* CAIGroup::GetForemostEnemy(const Vec3& direction, bool bLiveOnly) const
{
	Vec3 vFurthestPos(0,0,0);
	float maxLength = 0;
	const CAIObject* pForemost=NULL;
	Vec3 vDirNorm = direction.GetNormalizedSafe();
	TSetAIObjects::const_iterator itend = m_Targets.end();
	for (TSetAIObjects::const_iterator it = m_Targets.begin();it!= itend; ++it)
	{
		const CAIObject* pTarget = it->GetAIObject();
		if(!bLiveOnly || pTarget->IsAgent())
		{
			Vec3 vDisplace = pTarget->GetPos() - m_vAverageEnemyPosition;
			float fNorm = vDisplace.GetLength();
			if(fNorm>0.001f)//consider an epsilon
			{
				Vec3 vNorm = vDisplace/fNorm;
				float cosine = vNorm.Dot(vDirNorm);
				if(cosine>=0) // exclude negative cosine (opposite directions)
				{
					float fProjection = cosine*fNorm;
					if(maxLength<fProjection)
					{
						maxLength = fProjection;
						pForemost = pTarget;
					}
				}
			}
			else if(!pForemost)
			{
				// the target is exactly at the average position and no one better has been found yet
				pForemost = pTarget;
			}
		}
	}

	CAISystem* pAISystem(GetAISystem());
	if(!pForemost && !bLiveOnly)
		pForemost = static_cast<CAIObject*>(pAISystem->GetBeacon(m_groupID));

	return pForemost;
}

//====================================================================
// GetEnemyPositionKnown
//====================================================================
Vec3 CAIGroup::GetEnemyPositionKnown( ) const
{
	// (MATT) Only used in one place and candidates for its replacement are noted {2009/02/10}
	CCCPOINT(CAIGroup_GetEnemyPositionKnown);

	CAISystem* pAISystem(GetAISystem());
	Vec3 enemyPos;
	bool realTarget = false;
	TUnitList::const_iterator it, itEnd = m_Units.end();
	for (it = m_Units.begin(); it != itEnd; ++it)
	{
		CPipeUser* pPipeUser = CastToCPipeUserSafe(it->m_refUnit.GetAIObject());
		if(!pPipeUser || !pPipeUser->IsEnabled())
			continue;

		CAIObject * const pTarget = (CAIObject*) pPipeUser->GetAttentionTarget();
		if (!pTarget || !pTarget->IsEnabled() || !pTarget->IsAgent())
			continue;

		if (pPipeUser->GetAttentionTargetType() == AITARGET_MEMORY || !pPipeUser->IsHostile(pTarget))
			continue;
		
		realTarget = true;
		enemyPos = pTarget->GetPos();
		break;
	}

	if(!realTarget)
	{
		CCCPOINT(CAIGroup_GetEnemyPositionKnown_A);

		if(m_pLeader && m_pLeader->IsPlayer())
		{
			CAIPlayer* pPlayer = CastToCAIPlayerSafe(m_pLeader->GetAssociation().GetAIObject());
			if(pPlayer)
			{
				CAIObject* pTarget = pPlayer->GetAttentionTarget().GetAIObject();
				if(pTarget && pTarget->IsEnabled()) // nothing else to check, assuming that player has only live enemy targets
				{
					realTarget = true;
					enemyPos = pTarget->GetPos();
				}
			}
		}
		if(!realTarget)
		{
			if(pAISystem->GetBeacon(m_groupID))
				enemyPos = pAISystem->GetBeacon(m_groupID)->GetPos();
			else
				enemyPos.Set(0,0,0);
		}
	}
	return enemyPos;
}

//====================================================================
// Update
//====================================================================
void CAIGroup::Update()
{
	CCCPOINT(CAIGroup_Update);
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	CAISystem* pAISystem(GetAISystem());

	if(m_lastUpdateTime.GetSeconds() < 0.0f)
		m_lastUpdateTime = pAISystem->GetFrameStartTime();

	float	dt = (pAISystem->GetFrameStartTime() - m_lastUpdateTime).GetSeconds();

	UpdateEnemyStats();

	// (MATT)Could here, run through units, call on objectremoved?  {2009/03/24}

	for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		it->second->Update(dt);

	m_lastUpdateTime = pAISystem->GetFrameStartTime();

	// update readability sounds
	TSoundReadabilityMap::iterator it = m_ReadabilitySounds.begin();
	while ( it != m_ReadabilitySounds.end())
	{
		it->second -= dt;
		if(it->second <=0)
			m_ReadabilitySounds.erase(it++);
		else
			++it;
	}

	UpdateReinforcementLogic(dt);
}

//====================================================================
// UpdateReinforcementLogic
//====================================================================
// (MATT) This code looks rather complex - does it really help? {2009/02/12}
void CAIGroup::UpdateReinforcementLogic(float dt)
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	if (m_reinforcementState == REINF_ALERTED_COMBAT_PENDING ||
			m_reinforcementState == REINF_DONE_PENDING )
		return;

	// Get reinforcement points for this group if not acquired yet.
	if (!m_reinforcementSpotsAcquired)
	{
		m_reinforcementSpots.clear();
		
		m_reinforcementSpotsAllAlerted = 0;
		m_reinforcementSpotsCombat = 0;
		m_reinforcementSpotsBodyCount = 0;

		for (AutoAIObjectIter it(GetAISystem()->GetFirstAIObject(IAISystem::OBJFILTER_TYPE, AIANCHOR_REINFORCEMENT_SPOT)); it->GetObject(); it->Next())
		{
			CAIObject* pAI = static_cast<CAIObject*>(it->GetObject());
			if (pAI->GetGroupId() == m_groupID && pAI->GetEntity())
			{
				// Look up the extra properties.
				IScriptTable* pTable = pAI->GetEntity()->GetScriptTable();
				if (pTable)
				{
					CCCPOINT(CAIGroup_UpdateReinforcementLogic_A);

					SmartScriptTable props;
					if (pTable->GetValue("Properties", props))
					{
						m_reinforcementSpots.resize(m_reinforcementSpots.size() + 1);

						int whenAllAlerted = 0;
						int whenInCombat = 0;
						int groupBodyCount = 0;
						float avoidWhenTargetInRadius = 0.0f;
						int type = 0;
						props->GetValue("bWhenAllAlerted", whenAllAlerted);
						props->GetValue("bWhenInCombat", whenInCombat);
						props->GetValue("iGroupBodyCount", groupBodyCount);
						props->GetValue("eiReinforcementType", type);
						props->GetValue("AvoidWhenTargetInRadius", avoidWhenTargetInRadius);

						m_reinforcementSpots.back().whenAllAlerted = whenAllAlerted > 0;
						m_reinforcementSpots.back().whenInCombat = whenInCombat > 0;
						m_reinforcementSpots.back().groupBodyCount = groupBodyCount;
						m_reinforcementSpots.back().avoidWhenTargetInRadius = avoidWhenTargetInRadius;
						m_reinforcementSpots.back().type = type;
						m_reinforcementSpots.back().pAI = pAI;

						if (m_reinforcementSpots.back().whenAllAlerted)
							m_reinforcementSpotsAllAlerted++;
						if (m_reinforcementSpots.back().whenInCombat)
							m_reinforcementSpotsCombat++;
						if (m_reinforcementSpots.back().groupBodyCount > 0)
							m_reinforcementSpotsBodyCount++;

					}
				}
			}
		}

		m_reinforcementSpotsAcquired = true;
	}

	// Check if reinforcements should be called.
	if (!m_reinforcementSpots.empty() && m_reinforcementState != REINF_DONE)
	{
		CCCPOINT(CAIGroup_UpdateReinforcementLogic_B);

		int totalCount = 0;
		int enabledCount = 0;
		int alertedCount = 0;
		int combatCount = 0;
		int liveTargetCount = 0;
		for (TUnitList::const_iterator it = m_Units.begin(), end = m_Units.end(); it != end; ++it)
		{
			CPuppet* pPuppet = CastToCPuppetSafe(it->m_refUnit.GetAIObject());
			if (!pPuppet) continue;

			totalCount++;

			if (!pPuppet->IsEnabled()) continue;
			if (pPuppet->GetProxy()->IsCurrentBehaviorExclusive()) continue;

			if (pPuppet->GetAlertness() >= 1)
				alertedCount++;
			if (pPuppet->GetAlertness() >= 2)
				combatCount++;
			if (pPuppet->GetAttentionTargetType() == AITARGET_VISUAL &&
					pPuppet->GetAttentionTargetThreat() == AITHREAT_AGGRESSIVE)
				liveTargetCount++;
			enabledCount++;
		}

		bool matchWhenAllAlerted = false;
		bool matchWhenInCombat = false;
		bool matchWhenGroupSizeLess = false;

		bool tryToCall = false;

		if (m_reinforcementState == REINF_NONE)
		{
			if (m_reinforcementSpotsAllAlerted)
			{
				if ((enabledCount <= 2 && alertedCount > 0) ||
						(enabledCount > 2 && alertedCount > 1))
				{
					matchWhenAllAlerted = true;
					matchWhenInCombat = false;
					matchWhenGroupSizeLess = false;
					tryToCall = true;
				}
			}

			if (m_reinforcementSpotsCombat)
			{
				if ((enabledCount <= 2 && liveTargetCount > 0 && combatCount > 0) ||
						(enabledCount > 2 && liveTargetCount > 1 && combatCount > 1))
				{
					matchWhenAllAlerted = false;
					matchWhenInCombat = true;
					matchWhenGroupSizeLess = false;
					tryToCall = true;
				}
			}
		}

		if (m_reinforcementState == REINF_ALERTED_COMBAT)
		{
			if (m_reinforcementSpotsBodyCount && m_countDead > 0)
			{
				for (unsigned i = 0, n = m_reinforcementSpots.size(); i < n; ++i)
				{
					if (m_countDead >= m_reinforcementSpots[i].groupBodyCount)
					{
						matchWhenAllAlerted = false;
						matchWhenInCombat = false;
						matchWhenGroupSizeLess = true;
						tryToCall = true;
						break;
					}
				}
			}
		}

		if (tryToCall)
		{
			CCCPOINT(CAIGroup_UpdateReinforcementLogic_C);

			float nearestDist = FLT_MAX;
			SAIReinforcementSpot* nearestSpot = 0;
			CUnitImg* pNearestCallerImg = 0;	

			Vec3 playerPos(0,0,0), playerViewDir(0,0,0);
			CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetAISystem()->GetPlayer());
			if (pPlayer)
			{
				playerPos = pPlayer->GetPos();
				playerViewDir = pPlayer->GetViewDir();
			}

			for (unsigned i = 0, n = m_reinforcementSpots.size(); i < n; ++i)
			{
				if (!m_reinforcementSpots[i].pAI->IsEnabled()) continue;

				if (matchWhenAllAlerted && !m_reinforcementSpots[i].whenAllAlerted) continue;
				if (matchWhenInCombat && !m_reinforcementSpots[i].whenInCombat) continue;
				if (matchWhenGroupSizeLess && m_countDead >= m_reinforcementSpots[i].groupBodyCount) continue;

				const Vec3& reinfSpot = m_reinforcementSpots[i].pAI->GetPos();
				const float reinfRadiusSqr =  sqr(m_reinforcementSpots[i].pAI->GetRadius());
				const float avoidWhenTargetInRadiusSqr = sqr(m_reinforcementSpots[i].avoidWhenTargetInRadius);

				for (TUnitList::iterator it = m_Units.begin(), end = m_Units.end(); it != end; ++it)
				{
					CPuppet* const pPuppet = CastToCPuppetSafe(it->m_refUnit.GetAIObject());
					if (!pPuppet) continue;
					if (!pPuppet->IsEnabled()) continue;
					if ((GetAISystem()->GetFrameStartTime()-it->m_lastReinforcementTime).GetSeconds()<3.f) continue;
					if (pPuppet->GetProxy()->IsCurrentBehaviorExclusive()) continue;
					if (matchWhenAllAlerted && pPuppet->GetAlertness() < 1) continue;
					if (matchWhenInCombat && pPuppet->GetAlertness() < 2) continue;

					bool hasTarget = pPuppet->GetAttentionTarget() != 0;

					// Skip reinforcement spots that are too close the attention target.
					if (hasTarget && (pPuppet->GetAttentionTargetType() == AITARGET_VISUAL || pPuppet->GetAttentionTargetType() == AITARGET_MEMORY) &&
						pPuppet->GetAttentionTargetThreat() == AITHREAT_AGGRESSIVE &&
						Distance::Point_PointSq(reinfSpot, pPuppet->GetAttentionTarget()->GetPos()) < avoidWhenTargetInRadiusSqr)
						continue;

					float distSqr = Distance::Point_PointSq(pPuppet->GetPos(), reinfSpot);

					CCCPOINT(CAIGroup_UpdateReinforcementLogic_D);

					if (distSqr < reinfRadiusSqr)
					{
						float dist = sqrtf(distSqr);

						bool isInVehicle = pPuppet->GetProxy() && pPuppet->GetProxy()->GetLinkedVehicleEntityId();

						if (!hasTarget || isInVehicle)
						{
							dist += 100.0f;
						}
						else
						{
							const float preferredCallDist = 25.0f;
							float bestDist = fabsf(Distance::Point_Point(pPuppet->GetPos(), pPuppet->GetAttentionTarget()->GetPos()) - preferredCallDist);

							if (pPuppet->GetAttentionTargetType() == AITARGET_VISUAL &&
									pPuppet->GetAttentionTargetThreat() == AITHREAT_AGGRESSIVE)
								bestDist *= 0.5f;
							else if (pPuppet->GetAttentionTargetThreat() == AITHREAT_THREATENING)
								bestDist *= 0.75f;

							dist += bestDist;
						}

						// Prefer puppet visible to the player.
						Vec3 dirPlayerToPuppet = pPuppet->GetPos() - playerPos;
						dirPlayerToPuppet.NormalizeSafe();
						float dot = playerViewDir.Dot(dirPlayerToPuppet);
						dist += (1 - sqr((dot + 1)/2)) * 25.0f;

						if (dist < nearestDist)
						{
							nearestDist = dist;
							nearestSpot = &m_reinforcementSpots[i];
//							nearestCaller = pPuppet;
							pNearestCallerImg = &(*it);
						}
					}
				}
			}

			if (pNearestCallerImg && nearestSpot)
			{
				// Change state
				if (m_reinforcementState == REINF_NONE)
					m_reinforcementState = REINF_ALERTED_COMBAT_PENDING;
				else if (m_reinforcementState == REINF_ALERTED_COMBAT)
					m_reinforcementState = REINF_DONE_PENDING;

				// Tell the agent to call reinforcements.
				CAIActor * const pUnit = CastToCAIActorSafe(pNearestCallerImg->m_refUnit.GetAIObject());
				AISignalExtraData* pData = new AISignalExtraData;
				pData->nID = nearestSpot->pAI->GetEntityID();
				pData->iValue = nearestSpot->type;
				pUnit->SetSignal(1, "OnCallReinforcements", pUnit->GetEntity(), pData);
				pNearestCallerImg->m_lastReinforcementTime = GetAISystem()->GetFrameStartTime();

				m_DEBUG_reinforcementCalls.push_back(SAIRefinforcementCallDebug(pUnit->GetPos()+Vec3(0,0,0.3f), nearestSpot->pAI->GetPos()+Vec3(0,0,0.3f), 7.0f, nearestSpot->pAI->GetName()));

				m_refReinfPendingUnit = StaticCast<CPuppet>(pNearestCallerImg->m_refUnit);
				// Remove the spot (when confirmed)
				m_pendingDecSpotsAllAlerted = nearestSpot->whenAllAlerted;
				m_pendingDecSpotsCombat = nearestSpot->whenInCombat;
				m_pendingDecSpotsBodyCount = nearestSpot->groupBodyCount > 0;

				CCCPOINT(CAIGroup_UpdateReinforcementLogic_E);
			}
		}
	}

	for (unsigned i = 0; i < m_DEBUG_reinforcementCalls.size(); )
	{
		m_DEBUG_reinforcementCalls[i].t -= dt;
		if (m_DEBUG_reinforcementCalls[i].t < 0.0f)
		{
			m_DEBUG_reinforcementCalls[i] = m_DEBUG_reinforcementCalls.back();
			m_DEBUG_reinforcementCalls.pop_back();
		}
		else
			++i;
	}
}

//====================================================================
// FindOrCreateGroupTactic
//====================================================================
IAIGroupTactic*	CAIGroup::FindOrCreateGroupTactic(int eval)
{
	TacticMap::iterator ittac = m_tactics.find(eval);
	if(ittac != m_tactics.end())
		return ittac->second;

	IAIGroupTactic*	tactic = 0;

	switch(eval)
	{
	case 0: tactic = new CHumanGroupTactic(this); break;
	case 1: tactic = new CFollowerGroupTactic(this); break;
	case 2: tactic = new CBossGroupTactic(this); break;
	}

	if(tactic)
	{
		// Add current group members to the tactic
		for (TUnitList::const_iterator it = m_Units.begin(), end = m_Units.end(); it != end; ++it)
			tactic->OnMemberAdded(it->m_refUnit.GetAIObject());
		m_tactics.insert(TacticMap::iterator::value_type(eval, tactic));

		CCCPOINT(CAIGroup_FindOrCreateGroupTactic);
	}

	return tactic;
}


//====================================================================
// NotifyGroupTacticState
//====================================================================
void CAIGroup::NotifyGroupTacticState(IAIObject* pRequester, int tac, int type, float param)
{
	IAIGroupTactic*	pTactic = FindOrCreateGroupTactic(tac);
	if(!pTactic)
		return;
	pTactic->NotifyState(pRequester, type, param);
}

//====================================================================
// GetGroupTacticState
//====================================================================
int CAIGroup::GetGroupTacticState(IAIObject* pRequester, int tac, int type, float param)
{
	IAIGroupTactic*	pTactic = FindOrCreateGroupTactic(tac);
	if(!pTactic)
		return 0;
	return pTactic->GetState(pRequester, type, param);
}

//====================================================================
// GetGroupTacticPoint
//====================================================================
Vec3 CAIGroup::GetGroupTacticPoint(IAIObject* pRequester, int eval, int type, float param)
{
	IAIGroupTactic*	pTactic = FindOrCreateGroupTactic(eval);
	if(!pTactic)
		return ZERO;
	return pTactic->GetPoint(pRequester, type, param);
}

//====================================================================
// UpdateGroupCountStatus
//====================================================================
void CAIGroup::UpdateGroupCountStatus()
{
	CAISystem* pAISystem(GetAISystem());
	// Count number of active and non active members.
	int count = 0;
	int	countEnabled = 0;
	for (CAISystem::AIObjects::iterator it = pAISystem->m_mapGroups.find(m_groupID); it != pAISystem->m_mapGroups.end() && it->first == m_groupID; )
	{
		CAIObject* pObject = it->second.GetAIObject();

		// (MATT) These are weak refs so objects may have been removed {2009/03/25}
		if (!pObject)
		{
			pAISystem->m_mapGroups.erase(it++);
			continue;
		}

		int type = pObject->GetType();
		if (type == AIOBJECT_PUPPET || type == AIOBJECT_VEHICLE)
		{
			if (pObject->IsEnabled())
				countEnabled++;
			count++;
		}
		else if (type == AIOBJECT_VEHICLE)
		{
			CAIVehicle* pVehicle = pObject->CastToCAIVehicle();
			if (pVehicle->IsEnabled() && pVehicle->IsDriverInside())
				countEnabled++;
			count++;
		}

		++it;
	}

	// Store the current state
	m_count = count;
	m_countEnabled = countEnabled;
	// Keep track on the maximum 
	m_countMax = max(m_countMax, count);
	m_countEnabledMax = max(m_countEnabledMax, countEnabled);

	// add smart objects state "GroupHalved" to alive group members
	// (MATT) Rather specific :/ Does anyone use it?  {2009/03/25}
	if ( m_countEnabled == m_countEnabledMax / 2 )
	{
		CAISystem::AIObjects::iterator it, itEnd = pAISystem->m_mapGroups.end();
		for ( it = pAISystem->m_mapGroups.lower_bound( m_groupID ); it != itEnd && it->first == m_groupID; ++it )
		{
			// Objects will now already be validated or removed
			CAIObject* pObject = it->second.GetAIObject();
			if ( pObject->IsEnabled() && pObject->GetType() != AIOBJECT_LEADER )
			{
				if ( IEntity* pEntity = pObject->GetEntity() )
					gAIEnv.pSmartObjectManager->AddSmartObjectState( pEntity, "GroupHalved" );
			}
		}
	}
}

//====================================================================
// RequestReadabilitySound
//====================================================================
bool CAIGroup::RequestReadabilitySound(int id, float duration)
{
	if(m_ReadabilitySounds.find(id) != m_ReadabilitySounds.end())
		return false;
	m_ReadabilitySounds.insert(std::make_pair(id, duration));
	return true;
}

//====================================================================
// GetGroupCount
//====================================================================
int CAIGroup::GetGroupCount(int flags)
{
	if (flags == 0 || flags == IAISystem::GROUP_ALL)
		return m_count;
	else if (flags == IAISystem::GROUP_ENABLED)
		return m_countEnabled;
	else if (flags == IAISystem::GROUP_MAX)
		return m_countMax;
	else if (flags == (IAISystem::GROUP_ENABLED|IAISystem::GROUP_MAX))
		return m_countEnabledMax;
	return 0;
}

//====================================================================
// Reset
//====================================================================
void CAIGroup::Reset()
{
	CCCPOINT(CAIGroup_Reset);

	m_lastUpdateTime.SetSeconds(-1.0f);
	m_count = 0;
	m_countMax = 0;
	m_countEnabled = 0;
	m_countEnabledMax = 0;
	m_countDead = 0;
	m_countCheckedDead = 0;
	m_bUpdateEnemyStats = true;

	for(TUnitList::iterator itrUnit(m_Units.begin()); itrUnit!=m_Units.end(); ++itrUnit )
		itrUnit->Reset();

	for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		it->second->Reset();

	m_ReadabilitySounds.clear();

	m_reinforcementSpots.clear();
	m_reinforcementSpotsAcquired = false;
	m_reinforcementState = REINF_NONE;
	m_refReinfPendingUnit.Reset();

	m_DEBUG_reinforcementCalls.clear();
}

//====================================================================
// Serialize
//====================================================================
void CAIGroup::Serialize(TSerialize ser, CObjectTracker& objectTracker)
{
	CCCPOINT(CAIGroup_Serialize);

	// TODO: serialize and create tactical evals
	char Name[256];
	sprintf(Name, "AIGroup-%d", m_groupID);

	ser.BeginGroup(Name);

	ser.Value("AIgroup_GroupID", m_groupID);

	m_bUpdateEnemyStats = true;

	ser.Value("m_lastUpdateTime", m_lastUpdateTime);

	ser.Value("m_count", m_count);
	ser.Value("m_countMax", m_countMax);
	ser.Value("m_countEnabled", m_countEnabled);
	ser.Value("m_countEnabledMax", m_countEnabledMax);
	ser.Value("m_countDead", m_countDead);
	ser.Value("m_countCheckedDead", m_countCheckedDead);

	//	TSetAIObjects	m_Targets;
	ser.Value("m_vEnemyPositionKnown", m_vEnemyPositionKnown);
	ser.Value("m_vAverageEnemyPosition", m_vAverageEnemyPosition);
	ser.Value("m_vAverageLiveEnemyPosition", m_vAverageLiveEnemyPosition);

	//see below for m_pLeader Serialization

	objectTracker.SerializeObjectContainer(ser, "m_Units", m_Units);

	// reset ranks - m_RankedUnits is not getting serialized 
	if(ser.IsReading())
	{
		m_RankedUnits.clear();
		for(TUnitList::iterator itrUnit(m_Units.begin()); itrUnit!=m_Units.end(); ++itrUnit )
			SetUnitRank(itrUnit->m_refUnit.GetAIObject());
		// Clear reinforcement spots.
		m_reinforcementSpotsAcquired = false;
		m_reinforcementSpots.clear();
	}
	objectTracker.SerializeValueValueContainer(ser, "m_ReadabilitySounds", m_ReadabilitySounds);
	
	SerializeWeakRefVector<const CAIObject>(ser, "m_Targets", m_Targets);
	
	ser.EnumValue("m_reinforcementState", m_reinforcementState, REINF_NONE, LAST_REINF);

	ser.Value("m_pendingDecSpotsAllAlerted", m_pendingDecSpotsAllAlerted);
	ser.Value("m_pendingDecSpotsCombat", m_pendingDecSpotsCombat);
	ser.Value("m_pendingDecSpotsBodyCount", m_pendingDecSpotsBodyCount);
	m_refReinfPendingUnit.Serialize(ser, "m_refReinfPendingUnit");

	// Serialize group tactics.
	ser.BeginGroup("AIGroupTactics");
	int numTactics = m_tactics.size();
	ser.Value("numTactics", numTactics);

	if(ser.IsWriting())
	{
		int i = 0;
		for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		{
			ser.BeginGroup("Tactic");
			int	type = it->first;
			ser.Value("type", type);
			it->second->Serialize(ser, objectTracker);
			ser.EndGroup();
			++i;
		}
	}
	else
	{
		for(int i = 0; i < numTactics; ++i)
		{
			ser.BeginGroup("Tactic");
			int	type = 0;
			ser.Value("type", type);
			IAIGroupTactic*	tactic = FindOrCreateGroupTactic(type);
			if(tactic)
				tactic->Serialize(ser, objectTracker);
			ser.EndGroup();
		}
	}

	ser.EndGroup();	// AIGroupTactics

	ser.EndGroup();	// AIGroup
}

void CAIGroup::OnUnitAttentionTargetChanged()
{
	m_bUpdateEnemyStats = true;
	for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		it->second->OnUnitAttentionTargetChanged();
}

//====================================================================
// DebugDraw
//====================================================================
void CAIGroup::DebugDraw()
{
	for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		it->second->DebugDraw();

	// Debug draw reinforcement logic.
	if (gAIEnv.CVars.DebugDrawReinforcements == m_groupID)
	{

		int totalCount = 0;
		int enabledCount = 0;
		int alertedCount = 0;
		int combatCount = 0;
		int liveTargetCount = 0;
		for (TUnitList::const_iterator it = m_Units.begin(), end = m_Units.end(); it != end; ++it)
		{
			CPuppet* pPuppet = CastToCPuppetSafe( it->m_refUnit.GetAIObject() );
			if (!pPuppet) continue;
			totalCount++;
			if (!pPuppet->IsEnabled()) continue;
			if (pPuppet->GetProxy()->IsCurrentBehaviorExclusive()) continue;

			if (pPuppet->GetAlertness() >= 1)
				alertedCount++;
			if (pPuppet->GetAlertness() >= 2)
				combatCount++;
			if (pPuppet->GetAttentionTargetType() == AITARGET_VISUAL &&
					pPuppet->GetAttentionTargetThreat() == AITHREAT_AGGRESSIVE)
				liveTargetCount++;
			enabledCount++;
		}

		CDebugDrawContext dc;

		for (unsigned i = 0, n = m_reinforcementSpots.size(); i < n; ++i)
		{
			if (m_reinforcementSpots[i].pAI->IsEnabled())
			{
				string text;
				char msg[64];

				if (m_reinforcementState == REINF_NONE)
				{
					if (m_reinforcementSpots[i].whenAllAlerted)
					{
						_snprintf(msg, 64, "ALERTED: Alerted >= Enabled | %d >= %d", alertedCount, enabledCount);
						text += msg;
						if (alertedCount >= enabledCount)
							text += " Active!\n";
						else
							text += "\n";
					}

					if (m_reinforcementSpots[i].whenInCombat)
					{
						if (enabledCount > 1)
							_snprintf(msg, 64, "COMBAT: InCombat > 0 && LiveTarget > 1 | %d > 0 && %d > 1", combatCount, liveTargetCount);
						else
							_snprintf(msg, 64, "COMBAT: InCombat > 0 && LiveTarget > 0 | %d > 0 && %d > 0", combatCount, liveTargetCount);
						text += msg;

						if (combatCount > 0 && ((enabledCount > 1 && liveTargetCount > 1) || (enabledCount == 1 && liveTargetCount > 0)))
							text += " Active!\n";
						else
							text += "\n";
					}
				}

				if (m_reinforcementState == REINF_NONE || m_reinforcementState == REINF_ALERTED_COMBAT)
				{
					if (m_reinforcementSpots[i].groupBodyCount > 0)
					{
						_snprintf(msg, 64, "SIZE: Bodies < Count | %d < %d",
							m_reinforcementSpots[i].groupBodyCount, m_countDead);
						text += msg;

						if (m_countDead >= m_reinforcementSpots[i].groupBodyCount)
							text += " >>>\n";
						else
							text += "\n";
					}
				}
				dc->Draw3dLabel(m_reinforcementSpots[i].pAI->GetPos() + Vec3(0, 0, 1), 1.5f, text.c_str());
			}
			else
			{
				dc->Draw3dLabel(m_reinforcementSpots[i].pAI->GetPos() + Vec3(0, 0, 1), 1.0f, "Disabled");
			}

			ColorB	color, colorTrans;
			int c = m_reinforcementSpots[i].type % 3;
			if (c == 0)
				color.Set(255, 64, 64, 255);
			else if (c == 1)
				color.Set(64, 255, 64, 255);
			else if (c == 2)
				color.Set(64, 64, 255, 255);
			colorTrans = color;
			colorTrans.a = 64;

			if (!m_reinforcementSpots[i].pAI->IsEnabled())
			{
				color.a /= 4;
				colorTrans.a /= 4;
			}

			dc->DrawCylinder(m_reinforcementSpots[i].pAI->GetPos() + Vec3(0, 0, 0.5f), Vec3(0, 0, 1), 0.2f, 1.0f, color);
			dc->DrawRangeCircle(m_reinforcementSpots[i].pAI->GetPos(), m_reinforcementSpots[i].pAI->GetRadius(), 1.0f, colorTrans, color, true);

			if (m_reinforcementSpots[i].pAI->IsEnabled())
			{
				for (TUnitList::const_iterator it = m_Units.begin(), end = m_Units.end(); it != end; ++it)
				{
					CPuppet* pPuppet = CastToCPuppetSafe(it->m_refUnit.GetAIObject());
					if (!pPuppet) continue;
					totalCount++;
					if (!pPuppet->IsEnabled()) continue;
					if (pPuppet->GetProxy()->IsCurrentBehaviorExclusive()) continue;

					if (Distance::Point_PointSq(pPuppet->GetPos(), m_reinforcementSpots[i].pAI->GetPos()) < sqr(m_reinforcementSpots[i].pAI->GetRadius()))
						dc->DrawLine(pPuppet->GetPos(), color + Vec3(0, 0, 0.5f), m_reinforcementSpots[i].pAI->GetPos() + Vec3(0, 0, 0.5f), color);
				}
			}
		}

		for (unsigned i = 0, n = m_DEBUG_reinforcementCalls.size(); i < n; ++i)
		{
			const Vec3& pos = m_DEBUG_reinforcementCalls[i].from;
			Vec3 dir = m_DEBUG_reinforcementCalls[i].to - m_DEBUG_reinforcementCalls[i].from;
			dc->DrawArrow(pos, dir, 0.5f, ColorB(255, 255, 255));
			
			float len = dir.NormalizeSafe();
			dir *= min(2.0f, len);

			dc->Draw3dLabel(pos + dir, 1.0f, m_DEBUG_reinforcementCalls[i].performer.c_str());
		}

	}
}

void CAIGroup::Validate()
{
	for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		((CHumanGroupTactic*)(it->second))->Validate();
}

void CAIGroup::Dump()
{
	AILogAlways(">>----------------------------");
	for(TacticMap::iterator it = m_tactics.begin(); it != m_tactics.end(); ++it)
		((CHumanGroupTactic*)(it->second))->Dump();
}

//====================================================================
// NotifyGroupTacticState
//====================================================================
void CAIGroup::NotifyReinfDone(const IAIObject* obj, bool isDone)
{
	CAIObject *pReinfPendingUnit = m_refReinfPendingUnit.GetAIObject();
	if(!pReinfPendingUnit)
		return;
	if(pReinfPendingUnit!=obj)
	{
		AIWarning("CAIGroup::NotifyReinfDone - pending unit missMatch (internal %s ; passed %s)", pReinfPendingUnit->GetName(), obj->GetName());
		AIAssert(0);
		return;
	}
	if (m_reinforcementState != REINF_ALERTED_COMBAT_PENDING &&
			m_reinforcementState != REINF_DONE_PENDING )
	{
		AIWarning("CAIGroup::NotifyReinfDone - not pending state (Puppet %s ; state %d)", obj->GetName(), m_reinforcementState);
		AIAssert(0);
		return;
	}
	if(isDone)
	{
		// Change state
		if (m_reinforcementState == REINF_ALERTED_COMBAT_PENDING)
			m_reinforcementState = REINF_ALERTED_COMBAT;
		else if (m_reinforcementState == REINF_DONE_PENDING)
			m_reinforcementState = REINF_DONE;

		// Remove the spot
		if (m_pendingDecSpotsAllAlerted)
			m_reinforcementSpotsAllAlerted--;
		if (m_pendingDecSpotsCombat)
			m_reinforcementSpotsCombat--;
		if (m_pendingDecSpotsBodyCount)
			m_reinforcementSpotsBodyCount--;
	}
	else
	{
		// Change state
		if (m_reinforcementState == REINF_ALERTED_COMBAT_PENDING)
			m_reinforcementState = REINF_NONE;
		else if (m_reinforcementState == REINF_DONE_PENDING)
			m_reinforcementState = REINF_ALERTED_COMBAT;
	}
	m_refReinfPendingUnit.Reset();
}

