#include "StdAfx.h"
#include "CentralInterestManager.h"
#include "PersonalInterestManager.h"
#include "AIActor.h"
#include "Puppet.h"

// For persistent debugging
#include "IGameFramework.h"

#define CIM_RAYBUDGET 1			// Max number of rays the interest system may fire per update; 2 would be extravagant
#define CIM_UPDATE_TIME 0.1f
#define CIM_MAX_PIMS_PER_UPDATE 2
#define CIM_MIN_DIST 40.0f*40.0f
#define CIM_INITIAL_PIMS 32
#define CIM_INITIAL_INTERESTING 128

//------------------------------------------------------------------------------------------------------------------------

// Zero singleton pointer
CCentralInterestManager * CCentralInterestManager::m_pCIMInstance = NULL;

//------------------------------------------------------------------------------------------------------------------------

CCentralInterestManager::CCentralInterestManager(void)
{
	m_bEnabled = false;
	m_pPersistantDebug = NULL;
	m_bEntityEventListenerInstalled = false;
}

//------------------------------------------------------------------------------------------------------------------------

void CCentralInterestManager::Init(void)
{
	m_cvDebugInterest = gEnv->pConsole->GetCVar("ai_DebugInterestSystem");

	// Init vectors to stored sizes
	m_PIMs.reserve(CIM_INITIAL_PIMS);
	m_InterestingEntities.reserve(CIM_INITIAL_INTERESTING);

	m_itLastUpdated = m_PIMs.begin();

	if (!m_bEntityEventListenerInstalled)
	{
		// Start listening to all moving entities
		gEnv->pEntitySystem->AddSink( this );		
		m_bEntityEventListenerInstalled = true;
	}
}

//------------------------------------------------------------------------------------------------------------------------

CCentralInterestManager::~CCentralInterestManager(void)
{
	// Stop listening to all entities
	if (m_bEntityEventListenerInstalled)
	{
		gEnv->pEntitySystem->RemoveSink( this );	
		m_bEntityEventListenerInstalled = false;
	}
}

//------------------------------------------------------------------------------------------------------------------------

CCentralInterestManager * CCentralInterestManager::GetInstance(void)
{
	// Return singleton pointer
	// Create from scratch if need be
	if (!m_pCIMInstance)
	{
		m_pCIMInstance = new CCentralInterestManager();
		m_pCIMInstance->Init();
	}
	return m_pCIMInstance;
}

//------------------------------------------------------------------------------------------------------------------------

void CCentralInterestManager::Reset(void)
{
	// Clear tracking vectors
	for (TVecInteresting::iterator itI = m_InterestingEntities.begin(); itI != m_InterestingEntities.end(); ++itI)
		(*itI).Invalidate();

	for (TVecPIMs::iterator itP = m_PIMs.begin(); itP != m_PIMs.end(); ++itP)
		(*itP).Assign(NILREF);

	m_itLastUpdated = m_PIMs.begin();
}

//------------------------------------------------------------------------------------------------------------------------

bool CCentralInterestManager::Enable(bool bEnable)
{
	if (m_bEnabled != bEnable)
	{
		m_bEnabled = bEnable;

		if (m_bEnabled)
		{
			if(!m_pPersistantDebug) m_pPersistantDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
		}
	}

	return m_bEnabled;
}

//------------------------------------------------------------------------------------------------------------------------

void CCentralInterestManager::RefreshInterestingEntityData(SEntityInterest& InterestingEntity)
{
	IEntity* pEntity = InterestingEntity.GetEntity();

	SmartScriptTable InstanceTable;
	bool bFound = pEntity->GetScriptTable()->GetValue("PropertiesInstance", InstanceTable);
	if(bFound == false)
	{
		bFound = pEntity->GetScriptTable()->GetValue("Properties", InstanceTable);
	}

	if(bFound == true)
	{
		SmartScriptTable InterestInstanceTable;
		if ( InstanceTable->GetValue("Interest", InterestInstanceTable) == true )
		{
			bool bOverrideArchetype = true;
			bool bExist = InterestInstanceTable->GetValue("bOverrideArchetype", bOverrideArchetype);

			if( bExist == false || bOverrideArchetype	== true )
			{
				RegisterFromTable(pEntity, InstanceTable);
			} 
			else
			{
				RegisterFromTable(pEntity, pEntity->GetArchetype()->GetProperties());
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------

void CCentralInterestManager::Update( float fDelta)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (!m_bEnabled) return;

	m_fUpdateTime -= fDelta;
	if( m_fUpdateTime > 0.0f ) return;

	m_fUpdateTime = CIM_UPDATE_TIME;

	// Make sure that properties are updated all the time while in the editor
	if(gEnv->IsEditor() == true && IsDebuggingEnabled() == true)
	{
		TVecInteresting::iterator itI = m_InterestingEntities.begin();
		TVecInteresting::iterator itEnd = m_InterestingEntities.end();
		for (; itI != itEnd; ++itI)
		{
			if ((*itI).IsValid() == true)
			{
				RefreshInterestingEntityData(*itI);
			}
		}
	}

	// Cache camera position and view direction
	const CCamera cam = gEnv->pSystem->GetViewCamera();

	// Update all the active PIMs
	TVecPIMs::iterator itEnd = m_PIMs.end();
	for (uint uCount = 0; m_itLastUpdated != itEnd && uCount < CIM_MAX_PIMS_PER_UPDATE; ++m_itLastUpdated)
	{
		if((*m_itLastUpdated).IsReset() == false) 
		{
			// Sanity check to only update active non-combat AI
			CPuppet * const pPuppet = (*m_itLastUpdated).GetAssigned()->CastToCPuppet();
			if(pPuppet && !pPuppet->IsAlarmed() && pPuppet->IsActive() && pPuppet->GetProxy() && pPuppet->GetProxy()->GetActorHealth()> 0 )
			{
				bool bCloseToCam = (*m_itLastUpdated).GetAssigned()->GetPos().GetSquaredDistance2D(cam.GetPosition()) < CIM_MIN_DIST;
				if( (*m_itLastUpdated).Update(bCloseToCam) )
					++uCount;
			}
		}
	}

	if( m_itLastUpdated == itEnd )
	{
		m_itLastUpdated = m_PIMs.begin();
	}
}

//------------------------------------------------------------------------------------------------------------------------

bool CCentralInterestManager::RegisterInterestingEntity( IEntity *pEntity, float fRadius, float fBaseInterest, const char* actionName )
{
	SEntityInterest const * pInterest = NULL;
	bool bOnlyUpdate = false;
	bool bSomethingChanged = false;

	TVecInteresting::iterator itI = m_InterestingEntities.begin();
	TVecInteresting::iterator itEnd = m_InterestingEntities.end();
	for (; itI != itEnd; ++itI)
	{
		if ((*itI).entityId == pEntity->GetId())
		{
			pInterest = &(*itI);
			bSomethingChanged = (*itI).Set(pEntity->GetId(), fRadius, fBaseInterest, actionName);
			break;
		}
	}

	if(pInterest == NULL)
	{
		// Find a spare PIM
		itI = m_InterestingEntities.begin();
		for (; itI != itEnd; ++itI)
		{
			if((*itI).IsValid() == false)
				break;
		}	

		if (itI == itEnd) 
		{
			m_InterestingEntities.push_back(SEntityInterest(pEntity->GetId(), fRadius, fBaseInterest, actionName));
			pInterest = &(m_InterestingEntities.back());
		}
		else
		{
			(*itI).Set(pEntity->GetId(), fRadius, fBaseInterest, actionName);
			pInterest = &(*itI);
		}

		bSomethingChanged = true;
	}

	if(IsDebuggingEnabled() && pInterest != NULL && bSomethingChanged == true)
	{
		string sText;
		sText.Format("Interest: %0.1f Radius: %0.1f Action: %s", pInterest->fInterest, pInterest->fRadius, pInterest->GetAction());
		AddDebugTag(pInterest->entityId, sText.c_str());
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------------------

void CCentralInterestManager::DeregisterInterestingEntity( IEntity *pEntity )
{
	TVecInteresting::iterator itI = m_InterestingEntities.begin();
	TVecInteresting::iterator itEnd = m_InterestingEntities.end();
	for (; itI != itEnd; ++itI)
	{
		if ((*itI).entityId == pEntity->GetId())
		{
			(*itI).Invalidate();
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------

bool CCentralInterestManager::RegisterInterestedAIActor( IEntity *pEntity, bool bEnablePIM, float fInterestFilter, float fAngle )
{
	// Try to get an AI object
	IAIObject *pAIObj = pEntity->GetAI();
	if (!pAIObj) return false;
	
	// Try to cast to actor
	CAIActor* pActor = CastToCAIActorSafe(pAIObj);
	if (!pActor) return false;

	if(CPersonalInterestManager* pPIM = FindPIM(pEntity))
	{
		pPIM->SetSettings(bEnablePIM,fInterestFilter,fAngle);
		return true;
	}

	// Find a spare PIM
	TVecPIMs::iterator it = m_PIMs.begin();	
	TVecPIMs::iterator itEnd = m_PIMs.end();	
	for (; it != itEnd; ++it)
	{
		CPersonalInterestManager* p0 = &(*it);
		if ((*it).IsReset() == true) 
		{
			(*it).Assign(pActor);
			(*it).SetSettings(bEnablePIM,fInterestFilter,fAngle);
			break;
		}
	}

	if (it == itEnd) 
	{
		m_PIMs.push_back(CPersonalInterestManager(pActor));
		m_itLastUpdated = m_PIMs.begin();

		if(CPersonalInterestManager* pPIM = FindPIM(pEntity))
		{
			pPIM->SetSettings(bEnablePIM,fInterestFilter,fAngle);
		}
		else
		{
			assert(false && "Cannot find a just recently created PersonalInterestManager!");
		}
	}

	// Mark this to say we successfully registered it
	return true;
}

//------------------------------------------------------------------------------------------------------------------------

CPersonalInterestManager* CCentralInterestManager::FindPIM(IEntity* pEntity)
{
	assert(pEntity != NULL);

	CPersonalInterestManager* pRet = NULL; 
	TVecPIMs::iterator it = m_PIMs.begin();	
	TVecPIMs::iterator itEnd = m_PIMs.end();	

	for (; it != itEnd; ++it)
	{
		CPersonalInterestManager& pit = *it;
		CAIActor* actor = pit.GetAssigned();
		
		if (actor && (actor->GetEntityID() == pEntity->GetId()))
			return &pit;
	}

	return 0;
}

//------------------------------------------------------------------------------------------------------------------------

bool CCentralInterestManager::DeregisterInterestedAIActor( IEntity *pEntity )
{
	// Try to get an AI object
	IAIObject *pAIObj = pEntity->GetAI();
	if (!pAIObj) return false;

	// Try to cast to actor
	CAIActor* pActor = CastToCAIActorSafe(pAIObj);
	if (!pActor) return false;

	if(CPersonalInterestManager* pPIM = FindPIM(pEntity))
	{
		pPIM->Assign(NILREF);
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------------------

void CCentralInterestManager::AddDebugTag( EntityId id, const char* pStr, float fTime )
{
	if (IsDebuggingEnabled())	
	{
		string text;
		text.Format("[%s]: %s",gEnv->pEntitySystem->GetEntity(id)->GetName(), pStr);

		if(!m_pPersistantDebug) m_pPersistantDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();

		if (fTime < 0.0f) m_pPersistantDebug->AddEntityTag( SEntityTagParams( id, pStr ) );
		else m_pPersistantDebug->AddEntityTag( SEntityTagParams( id, pStr, 1.5f, ColorF(1, 1, 1, 1), fTime ) );
	}
}

//------------------------------------------------------------------------------------------------------------------------

void CCentralInterestManager::DeregisterObject(IEntity * pEntity)
{
	// Find this entity
	TVecInteresting::iterator it = m_InterestingEntities.begin();
	TVecInteresting::iterator itEnd = m_InterestingEntities.end();
	for (; it != itEnd; ++it)
	{
		if ((*it).entityId == pEntity->GetId()) break;
	}
	if (it != itEnd) 
	{
		(*it).Invalidate();
	}
		
	// Debugging
	AddDebugTag(pEntity->GetId(), "Deregistered");
}

//------------------------------------------------------------------------------------------------------------------------
void CCentralInterestManager::OnEntityEvent( IEntity *pEntity, SEntityEvent &event )
{
	assert(pEntity != NULL);

	if (event.event == ENTITY_EVENT_START_LEVEL || (event.event == ENTITY_EVENT_RESET && event.nParam[0] == 1) )
	{
		SmartScriptTable InstanceTable;
		bool bFound = pEntity->GetScriptTable()->GetValue("PropertiesInstance", InstanceTable);
		if(bFound == false)
		{
			bFound = pEntity->GetScriptTable()->GetValue("Properties", InstanceTable);
		}

		if(bFound == true)
		{
			SmartScriptTable InterestInstanceTable;
			if ( InstanceTable->GetValue("Interest", InterestInstanceTable) == true )
			{
				bool bOverrideArchetype = true;
				bool bExist = InterestInstanceTable->GetValue("bOverrideArchetype", bOverrideArchetype);

				if( bExist == false || bOverrideArchetype	== true )
				{
					RegisterFromTable(pEntity, InstanceTable);
				} 
				else if(pEntity->GetArchetype() != NULL)
				{
					RegisterFromTable(pEntity, pEntity->GetArchetype()->GetProperties());
				}
			}
		}
	} 
	else
	{
		DeregisterObject(pEntity);

		// Is this a potential actor?
		if (CastToIAIActorSafe(pEntity->GetAI()) != NULL)
		{
			DeregisterInterestedAIActor(pEntity);
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------
void CCentralInterestManager::RegisterFromTable( IEntity *pEntity, const SmartScriptTable& Table )
{
	assert(pEntity != NULL);

	SmartScriptTable InterestTable;

	if(Table->GetValue("Interest", InterestTable) == true )
	{
		if (CastToIAIActorSafe(pEntity->GetAI()) != NULL) // Is this a potential actor?
		{
			RegisterActorFromTable(pEntity, InterestTable);
		}
		else
		{
			RegisterEntityFromTable(pEntity, InterestTable);
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------
void CCentralInterestManager::RegisterEntityFromTable( IEntity *pEntity, const SmartScriptTable& Table )
{
	assert(pEntity != NULL);

	bool bInteresting = false;
	const char *sAction = NULL;
	float fInterestLevel = 0.0f;
	float fRadius = 0.0f;
	int iCategory = 0;

	Table->GetValue("bInteresting", bInteresting);
	Table->GetValue("soaction_Action", sAction);
	Table->GetValue("InterestLevel", fInterestLevel);
	Table->GetValue("Radius", fRadius);

	if (bInteresting == true)
	{
		RegisterInterestingEntity(pEntity, fRadius, fInterestLevel, sAction);
	}
}

//------------------------------------------------------------------------------------------------------------------------
void CCentralInterestManager::RegisterActorFromTable( IEntity *pEntity, const SmartScriptTable& Table )
{
	assert(pEntity != NULL);

	bool bInterested = false;
	float fFilter = 0.0f;
	float fAngle = 0.0f;

	Table->GetValue("bInterested", bInterested);
	Table->GetValue("MinInterestLevel", fFilter);
	Table->GetValue("Angle", fAngle);

	if (bInterested == true)
	{
		RegisterInterestedAIActor(pEntity, true, fFilter, fAngle);
	}
}

//------------------------------------------------------------------------------------------------------------------------
void CCentralInterestManager::OnSpawn( IEntity *pEntity,SEntitySpawnParams &params ) 
{
	assert(pEntity != NULL);

	if(pEntity->GetScriptTable() != NULL)
	{
		SmartScriptTable InstanceTable;
		bool bFound = pEntity->GetScriptTable()->GetValue("PropertiesInstance", InstanceTable);
		if(bFound == false)
		{
			bFound = pEntity->GetScriptTable()->GetValue("Properties", InstanceTable);
		}

		if(bFound == true)
		{
			SmartScriptTable InterestInstanceTable;
			if ( InstanceTable->GetValue("Interest", InterestInstanceTable) == true )
			{

				gEnv->pEntitySystem->AddEntityEventListener(pEntity->GetId(), ENTITY_EVENT_START_LEVEL, this);
				gEnv->pEntitySystem->AddEntityEventListener(pEntity->GetId(), ENTITY_EVENT_DONE, this);
				gEnv->pEntitySystem->AddEntityEventListener(pEntity->GetId(), ENTITY_EVENT_RESET, this);
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------

bool CCentralInterestManager::OnRemove( IEntity *pEntity )
{
	assert(pEntity != NULL);

	DeregisterObject(pEntity);

	// Is this a potential actor?
	if (IAIObject *pObj = pEntity->GetAI())
	{
		if (IAIActor * pAIActor = CastToIAIActorSafe(pObj))	DeregisterInterestedAIActor(pEntity);
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------------------

void CCentralInterestManager::RegisterListener( IInterestListener* pClass, EntityId idInterestingEntity )
{
	assert(pClass != NULL);
	assert(idInterestingEntity != 0);

	m_Listeners.insert(TMapListeners::value_type(idInterestingEntity, pClass));
}

//------------------------------------------------------------------------------------------------------------------------

void CCentralInterestManager::UnRegisterListener( IInterestListener* pClass, EntityId idInterestingEntity )
{
	assert(pClass != NULL);
	assert(idInterestingEntity != 0);

	std::pair<TMapListeners::iterator, TMapListeners::iterator> range;
	range = m_Listeners.equal_range(idInterestingEntity);

	TMapListeners::iterator it = range.first;
	for(; it!=range.second; ++it)
	{
		m_Listeners.erase(it);
	}
}

//------------------------------------------------------------------------------------------------------------------------

void CCentralInterestManager::OnEntityInteresting( EntityId idActor, EntityId idInterestingEntity, bool bInteresting )
{
	assert(idActor != 0);
	assert(idInterestingEntity != 0);

	std::pair<TMapListeners::iterator, TMapListeners::iterator> range;
	range = m_Listeners.equal_range(idInterestingEntity);

	TMapListeners::iterator it = range.first;
	for(; it!=range.second; ++it)
	{
		if(bInteresting)
		{
			(it->second)->OnInterested(idActor, idInterestingEntity);
		}
		else
		{
			(it->second)->OnLoseInterest(idActor, idInterestingEntity);
		}
	}
}

//-----------------------------------------------------------------------------------------------------
#include UNIQUE_VIRTUAL_WRAPPER(ICentralInterestManager)

bool SEntityInterest::Set(EntityId eid, float radius, float interest, const char* action)
{
	bool bChange = false;
	if(eid != entityId)
	{
		entityId = eid;
		bChange = true;
	}
	if(radius >= 0.0f && radius != fRadius) 
	{
		fRadius = radius;
		bChange = true;
	}
	if(interest >= 0.0f && interest != fInterest) 
	{
		fInterest = interest;
		bChange = true;
	}

	if(action != NULL && strlen(action) > 0 && actionName.compare(action) != 0 ) 
	{
		actionName = action;
		bChange = true;
	}

	if( bChange )
	{
		gAIEnv.pSmartObjectManager->AddSmartObjectState(GetEntity() , "Registered" );
	}

	return bChange;
}
