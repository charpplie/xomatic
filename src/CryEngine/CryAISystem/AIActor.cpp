/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2004.
-------------------------------------------------------------------------
File name:   AIActor.cpp
$Id$
Description: implementation of the CAIActor class.

-------------------------------------------------------------------------
History:
14:12:2006 -  Created by Kirill Bulatsev


*********************************************************************/
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AIActor.h"
#include "ObjectTracker.h"
#include "GoalOp.h"
#include "IPerceptionHandlerModifier.h"
#include "EmotionalSystem/EmotionalSystem.h"
#include "GroupSystem/GroupSystem.h"
#include "SelectionTree/SelectionTreeManager.h"
#include "crc32.h"

// (MATT) Needed for fetching proxy pointer {2009/04/03}
#include <IGame.h>
#include <IGameFramework.h>

#define GET_READY_TO_CHANGE_BEHAVIOR_SIGNAL	"OnBehaviorChangeRequest"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define _ser_value_(val) ser.Value(#val, val)


#pragma warning (disable : 4355)
CAIActor::CAIActor() :
	m_pProxy(NULL),
	m_healthHistory(0),
	m_lightLevel(AILL_LIGHT),
	m_usingCombatLight(false),
	m_perceptionDisabled(0),
	m_cachedWaterOcclusionValue(0.0f),
	m_vLastFullUpdatePos(ZERO),
	m_uBSSProfileUserId(g_uBTUserId_Invalid)
{
	_fastcast_CAIActor = true;

	AILogComment("CAIActor (%p)", this);
}
#pragma warning (default : 4355)

CAIActor::~CAIActor()
{
	AILogComment("~CAIActor  %s (%p)", GetName(), this);

	// (MATT) Must invalidate proxy before groups try to query it, as (CryAction) actor is gone at this stage {2009/01/28}
	if (GetProxy())
		GetProxy()->Release();

	CAIGroup* pGroup = GetAISystem()->GetAIGroup(GetGroupId());
	if(pGroup)
		pGroup->RemoveMember(this);

	delete m_healthHistory;

	IBSSProfileManager *pProfileManager = GetAISystem()->GetBSSProfileManager();
	if( pProfileManager && m_uBSSProfileUserId != g_uBTUserId_Invalid )
	{
		pProfileManager->RemoveUser(m_uBSSProfileUserId);
	}
}

SelectionTree* CAIActor::GetBehaviorSelectionTree() const
{
	return m_behaviorSelectionTree.get();
}

SelectionVariables* CAIActor::GetBehaviorSelectionVariables() const
{
	return m_behaviorSelectionVariables.get();
}

void CAIActor::ResetBehaviorSelectionTree()
{
	m_behaviorSelectionTree.reset();
	m_behaviorSelectionVariables.reset();

	IAIActorProxy *pProxy = GetProxy();
	assert(pProxy);
	if (pProxy)
	{
		const char* behaviorSelectionTreeName = pProxy->GetBehaviorSelectionTreeName();

		SelectionTreeTemplateID templateID = gAIEnv.pSelectionTreeManager->GetTreeTemplateID(behaviorSelectionTreeName);

		if (gAIEnv.pSelectionTreeManager->HasTreeTemplate(templateID))
		{
			const SelectionTreeTemplate& treeTemplate = gAIEnv.pSelectionTreeManager->GetTreeTemplate(templateID);
			if (treeTemplate.Valid())
			{
				m_behaviorSelectionTree.reset(new SelectionTree(treeTemplate.GetSelectionTree()));
				m_behaviorSelectionVariables.reset(new SelectionVariables(treeTemplate.GetVariableDeclarations().GetDefaults()));
				m_behaviorSelectionVariables->ResetChanged(true);
			}
		}
	}
}

bool CAIActor::ProcessBehaviorSelectionTreeSignal(const char* signalName, uint32 signalCRC)
{
	FUNCTION_PROFILER( gEnv->pSystem, PROFILE_AI );

	if (m_behaviorSelectionVariables.get())
	{
		const SelectionTreeTemplate& treeTemplate = m_behaviorSelectionTree->GetTemplate();
		if (treeTemplate.Valid())
			return treeTemplate.GetSignalVariables().ProcessSignal(signalName, signalCRC, *m_behaviorSelectionVariables);
	}

	return false;
}

bool CAIActor::UpdateBehaviorSelectionTree()
{
	FUNCTION_PROFILER( gEnv->pSystem, PROFILE_AI );

	if (m_behaviorSelectionTree.get())
	{
		if (m_behaviorSelectionVariables.get() && m_behaviorSelectionVariables->Changed())
		{
			const char* behaviorName = "";

			SelectionNodeID currentNodeID = m_behaviorSelectionTree->GetCurrentNodeID();
			if (SelectionNodeID selectedNodeID = m_behaviorSelectionTree->Evaluate(*m_behaviorSelectionVariables.get()))
			{
				m_behaviorSelectionVariables->ResetChanged();

				if (currentNodeID == selectedNodeID)
					return false;

				const SelectionTreeNode& node = m_behaviorSelectionTree->GetNode(selectedNodeID);
				behaviorName = node.GetName();

				const SelectionTreeTemplate& treeTemplate = m_behaviorSelectionTree->GetTemplate();
				if (const char* translatedName = treeTemplate.GetTranslator().GetTranslation(selectedNodeID))
					behaviorName = translatedName;
			}

			IAIActorProxy *pProxy = GetProxy();
			assert(pProxy);
			if (pProxy)
				pProxy->SetBehaviour(behaviorName);
			return true;
		}
	}

	return false;
}

void CAIActor::DebugDrawBehaviorSelectionTree()
{
	if (m_behaviorSelectionVariables.get())
	{
		const SelectionTreeTemplate& treeTemplate = m_behaviorSelectionTree->GetTemplate();
		m_behaviorSelectionVariables->DebugDraw(true, treeTemplate.GetVariableDeclarations());
	}

	if (m_behaviorSelectionTree.get())
		m_behaviorSelectionTree->DebugDraw();
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::Reset(EObjectResetType type)
{
	CAIObject::Reset(type);

	int i = m_State.vSignals.size();
	while ( i-- )
	{
		IAISignalExtraData* pData = m_State.vSignals[i].pEData;
		if ( pData )
			delete (AISignalExtraData*) pData;
	}
	m_State.FullReset();

	// TODO : (MATT) workaround - move into the method {2007/10/30:21:01:16}
	m_State.eLookStyle = LOOKSTYLE_DEFAULT;

	ReleaseFormation();

	if (IAIActorProxy* pProxy = GetProxy())
		pProxy->Reset(type);

	m_bEnabled = true;
	m_bUpdatedOnce = false;

	if(m_healthHistory) 
		m_healthHistory->Reset();

	IBSSProfileManager *pProfileManager = GetAISystem()->GetBSSProfileManager();

	if( pProfileManager && m_uBSSProfileUserId != g_uBTUserId_Invalid )
	{
		pProfileManager->ResetUser(m_uBSSProfileUserId);
	}

	// synch self with owner entity if there is one
	IEntity* pEntity(GetEntity());
	if(pEntity)
	{
		m_bEnabled = pEntity->IsActive();
		SetPos(pEntity->GetPos());
	}

	m_lightLevel = AILL_LIGHT;
	m_usingCombatLight = false;
	m_perceptionDisabled = 0;

	m_cachedWaterOcclusionValue = 0.0f;

	m_vLastFullUpdatePos.zero();

	m_detectionMax.Reset();
	m_detectionSnapshot.Reset();

	m_priorityTargets.clear();
	m_probableTargets.clear();

	m_blackBoard.Clear();
	m_blackBoard.GetForScript()->SetValue("Owner", this->GetName() );

	m_perceptionHandlerModifiers.clear();

	// make sure personal BT is enabled only when actor is active
	if( pProfileManager && m_uBSSProfileUserId != g_uBTUserId_Invalid )
	{
		pProfileManager->EnableUser(m_uBSSProfileUserId, m_bEnabled);
	}

	ResetBehaviorSelectionTree();
}

void CAIActor::EnablePerception(bool enable)
{
	m_perceptionDisabled += enable ? -1 : 1;

	assert(m_perceptionDisabled >= 0);	// Below zero? More disables then enables!
	assert(m_perceptionDisabled < 16); // Just a little sanity check
}

bool CAIActor::IsPerceptionEnabled() const
{
	return m_perceptionDisabled <= 0;
}

void CAIActor::ResetPerception()
{
	m_priorityTargets.clear();
	m_probableTargets.clear();
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::ParseParameters(const AIObjectParameters &params)
{
	SetGroupId(params.m_sParamStruct.m_nGroup);
	SetSpecies(params.m_sParamStruct.m_nSpecies);
	m_movementAbility = params.m_moveAbility;
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIActor::SetBehaviorTreeProfile(const char* szProfile)
{
	CRY_ASSERT(szProfile && szProfile[0]);

	if (m_behaviorSelectionTree.get())
	{
		AIWarning("Trying to set a Behavior Tree Profile while there is already a Behavior Selection Tree running!");
	
		return true;
	}

	bool bResult = false;

	IBSSProfileManager *pProfileManager = GetAISystem()->GetBSSProfileManager();
	if (pProfileManager)
	{
		if (m_uBSSProfileUserId != g_uBTUserId_Invalid)
		{
			bResult = pProfileManager->SwitchUserProfile(m_uBSSProfileUserId, szProfile);
		}
		else
		{
			bResult = pProfileManager->InitNewUser(this, szProfile, m_uBSSProfileUserId);
		}
	}

	return bResult;
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIActor::GetBTUserName(string &sOut) const
{
	sOut = GetName();
	return true;
}

//
//------------------------------------------------------------------------------------------------------------------------
IScriptTable* CAIActor::GetBTUserTable() const
{
	IScriptTable *pTable = NULL;
	
	IEntity *pEntity = GetEntity();
	if (pEntity)
	{
		pTable = pEntity->GetScriptTable();
	}

	return pTable;
}

//
//------------------------------------------------------------------------------------------------------------------------
IBlackBoard* CAIActor::GetBTUserBlackBoard() const
{
	return const_cast<IBlackBoard*>(static_cast<const IBlackBoard*>(&m_blackBoard));
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIActor::CanBTUserTreeRun() const
{
	bool bResult = false;

	IAIActorProxy *pProxy = GetProxy();
	if (pProxy)
	{
		// Behavior tree must run if the proxy is enabled or has a positive health!
		// (Kevin) When CAIProxy::IsDead() is patched up, call it here instead of GetActorHealth {2008/10/28}
		bResult = (pProxy->IsEnabled() && pProxy->GetActorHealth() > 0);
	}

	return bResult;
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::OnBTUserEvent(EBTUserEvents event, IPersonalBehaviorTree *pTree, const string& sSignal, const ScriptAnyValue* pArgValue)
{
	CAISystem *pAISystem = GetAISystem();
	CRY_ASSERT(pAISystem);

	switch (event)
	{
		// All events are handled via sending signals
		case eBTUE_OnNodeSignal:
		case eBTUE_OnNewLeafNode:
		case eBTUE_OnDecisionMade:
		{
			IAISignalExtraData *pData = NULL;
			if (pArgValue && pArgValue->GetVarType() == svtObject)
			{
				pData = pAISystem->CreateSignalExtraData();
				CRY_ASSERT(pData);

				SmartScriptTable dataTbl;
				pArgValue->CopyTo(dataTbl);
				pData->FromScriptTable(dataTbl);
			}

			// Get correct signal to send
			string sSendSignal;
			if (event == eBTUE_OnNewLeafNode)
			{
				sSendSignal = GET_READY_TO_CHANGE_BEHAVIOR_SIGNAL;
			}
			else
			{
				sSendSignal = sSignal;
			}

			pAISystem->SendSignal(SIGNALFILTER_SENDER, AISIGNAL_INCLUDE_DISABLED, sSendSignal.c_str(), this, pData);
		}
		break;

		default:
			CRY_ASSERT_MESSAGE(false, "CAIActor::OnProfileUserEvent Unhandled user event received!");
	}
}

//====================================================================
// CAIObject::OnObjectRemoved
//====================================================================
void CAIActor::OnObjectRemoved(CAIObject *pObject )
{
	CAIObject::OnObjectRemoved(pObject);

	// make sure no pending signal left from removed AIObjects
	if (!m_State.vSignals.empty())
	{
		if (EntityId removedEntityID = pObject->GetEntityID())
		{
			DynArray<AISIGNAL>::iterator it = m_State.vSignals.begin();
			DynArray<AISIGNAL>::iterator end = m_State.vSignals.end();

			for ( ; it != end; )
			{
				AISIGNAL& curSignal = *it;
				
				if(curSignal.senderID == removedEntityID)
				{
					delete static_cast<AISignalExtraData*>(curSignal.pEData);
					it = m_State.vSignals.erase(it);
					end = m_State.vSignals.end();
				}
				else
					++it;
			}
		}
	}

	for (unsigned i = 0; i < m_probableTargets.size(); )
	{
		if (m_probableTargets[i].pTarget == pObject)
		{
			m_probableTargets[i] = m_probableTargets.back();
			m_probableTargets.pop_back();
		}
		else
			++i;
	}

	for (unsigned i = 0; i < m_priorityTargets.size(); )
	{
		if (m_priorityTargets[i].pTarget == pObject)
		{
			m_priorityTargets[i] = m_priorityTargets.back();
			m_priorityTargets.pop_back();
		}
		else
			++i;
	}
}



//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::Update(EObjectUpdate type)
{
	UpdateBehaviorSelectionTree();
	UpdateCloakScale();

	// Determine if position has changed
	const Vec3& pos = GetPos();
	if (type == AIUPDATE_FULL && !IsEquivalent(m_vLastFullUpdatePos, pos, 1.0f))
	{
		// Recalculate the water occlusion at the new point
		m_cachedWaterOcclusionValue = GetAISystem()->GetWaterOcclusionValue(pos);

		m_vLastFullUpdatePos = pos;
	}

	if (gAIEnv.CVars.DebugDraw > 0)
	{
		if(gAIEnv.CVars.DebugDrawDamageControl > 0)
			UpdateHealthHistory();

		if (type == AIUPDATE_FULL)
			m_lightLevel = GetAISystem()->GetLightManager()->GetLightLevelAt(GetPos(), this, &m_usingCombatLight);

		if (!stricmp(gAIEnv.CVars.DrawPerceptionHandlerModifiers,GetName()))
			DebugDrawPerceptionHandlerModifiers();
	}
}


//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::UpdateCloakScale()
{
	float	delta(m_Parameters.m_fCloakScaleTarget - m_Parameters.m_fCloakScale);
		if( fabsf(delta)>.05f ) 
		{
			float	scaleStep(GetAISystem()->GetFrameDeltaTime()*.5f);
			if(delta>0.f)
				m_Parameters.m_fCloakScale = scaleStep<delta ? (m_Parameters.m_fCloakScale + scaleStep) : m_Parameters.m_fCloakScaleTarget;
			else
				m_Parameters.m_fCloakScale = scaleStep<-delta ? (m_Parameters.m_fCloakScale - scaleStep) : m_Parameters.m_fCloakScaleTarget;
		}
		else
			m_Parameters.m_fCloakScale = m_Parameters.m_fCloakScaleTarget;
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::UpdateDisabled(EObjectUpdate type)
{
FUNCTION_PROFILER( GetISystem(),PROFILE_AI );

	// (MATT) I'm assuming that AIActor should always have a proxy, or this could be bad for performance {2009/04/03}
	IAIActorProxy *pProxy = GetProxy();
	if (pProxy)
		pProxy->CheckUpdateStatus();
}

//===================================================================
// SetNavNodes
//===================================================================
void CAIActor::UpdateDamageParts(DamagePartVector& parts)
{
	if(!GetProxy())
		return;

	IPhysicalEntity*	phys = GetProxy()->GetPhysics(true);
	if(!phys)
		return;

	bool	queryDamageValues = true;

	static ISurfaceTypeManager* pSurfaceMan = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();

	pe_status_nparts statusNParts;
	int nParts = phys->GetStatus(&statusNParts);

	if((int)parts.size() != nParts)
	{
		parts.resize(nParts);
		queryDamageValues = true;
	}

	// The global damage table
	SmartScriptTable	pSinglePlayerTable;
	SmartScriptTable	pDamageTable;
	if(queryDamageValues)
	{
		gEnv->pScriptSystem->GetGlobalValue("SinglePlayer", pSinglePlayerTable);
		if(pSinglePlayerTable.GetPtr())
		{
			if(GetType() == AIOBJECT_PLAYER)
				pSinglePlayerTable->GetValue("DamageAIToPlayer", pDamageTable);
			else
				pSinglePlayerTable->GetValue("DamageAIToAI", pDamageTable);
		}
		if(!pDamageTable.GetPtr())
			queryDamageValues = false;
	}

	pe_status_pos statusPos;
	pe_params_part paramsPart;
	for (statusPos.ipart = 0, paramsPart.ipart = 0 ; statusPos.ipart < nParts ; ++statusPos.ipart, ++paramsPart.ipart)
	{
		if(!phys->GetParams(&paramsPart) || !phys->GetStatus(&statusPos))
			continue;

		primitives::box box;
		statusPos.pGeomProxy->GetBBox(&box);

		box.center *= statusPos.scale;
		box.size *= statusPos.scale;

		parts[statusPos.ipart].pos = statusPos.pos + statusPos.q * box.center;
		parts[statusPos.ipart].volume = (box.size.x * 2) * (box.size.y * 2) * (box.size.z * 2);

		if(queryDamageValues)
		{
			const	char*	matName = 0;
			phys_geometry* pGeom = paramsPart.pPhysGeomProxy ? paramsPart.pPhysGeomProxy : paramsPart.pPhysGeom;
			if (pGeom->surface_idx >= 0 &&  pGeom->surface_idx < paramsPart.nMats)
			{
				matName = pSurfaceMan->GetSurfaceType(pGeom->pMatMapping[pGeom->surface_idx])->GetName();      
				parts[statusPos.ipart].surfaceIdx = pGeom->surface_idx;
			}
			else
			{
				parts[statusPos.ipart].surfaceIdx = -1;
			}
			float	damage = 0.0f;
			// The this is a bit dodgy, but the keys in the damage table exclude the 'mat_' part of the material name.
			if(pDamageTable.GetPtr() && matName && strlen(matName) > 4)
				pDamageTable->GetValue(matName + 4, damage);
			parts[statusPos.ipart].damageMult = damage;
		}
	}
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::NotifySignalReceived( const char* szText, IAISignalExtraData* pData, uint32 crcCode)
{
	ListWaitGoalOps::iterator it = m_listWaitGoalOps.begin();
	while ( it != m_listWaitGoalOps.end() )
	{
		COPWaitSignal* pGoalOp = *it;
		if ( pGoalOp->NotifySignalReceived(this, szText, NULL) )
			it = m_listWaitGoalOps.erase( it );
		else
			++it;
	}

	// always process signals sent only to notify wait goal operation
	if (!crcCode)
		crcCode = gAIEnv.SignalCRCs.m_crcGen->GetCRC32(szText);

	if (!ProcessBehaviorSelectionTreeSignal(szText, crcCode))
	{
		// Behavior Tree stuff ---------------------------------
		IBSSProfileManager *pProfileManager = GetAISystem()->GetBSSProfileManager();
		if( pProfileManager && m_uBSSProfileUserId != g_uBTUserId_Invalid )
		{
			ScriptAnyValue argValue = NULL_BSS_SIGNAL_DATA;
			if (pData)
			{
				SmartScriptTable dataTbl(gEnv->pScriptSystem);
				pData->ToScriptTable(dataTbl);
				argValue = dataTbl;
			}
			pProfileManager->SendUserInput(m_uBSSProfileUserId, szText, argValue);
		}
	}

	// Emotional System stuff -------------------------------
	// TODO: Create an observer pattern here for other systems that wants to hear for signals (BT, emo, ...)
	if( GetAISystem()->GetEmotionalSystem() != NULL )
	{
		GetAISystem()->GetEmotionalSystem()->ReceiveEvent(GetEntityID(), szText, (pData ? pData->iValue : -1));
	}
}

// nSignalID = 10 allow duplicating signals
// nSignalID = 9 use the signal only to notify wait goal operations
//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::SetSignal(int nSignalID, const char * szText, IEntity *pSender, IAISignalExtraData* pData, uint32 crcCode)
{
	CCCPOINT(SetSignal);

	// Ensure we delete the pData object if we early out
	struct DeleteBeforeReturning
	{
		IAISignalExtraData** _p;
		DeleteBeforeReturning( IAISignalExtraData** p ) : _p(p) {}
		~DeleteBeforeReturning()
		{ 
			if ( *_p )
				GetAISystem()->FreeSignalExtraData( (AISignalExtraData*)*_p );
		}
	} autoDelete( &pData );

#ifdef _DEBUG
	if(strlen(szText)>=AISIGNAL::SIGNAL_STRING_LENGTH-1)
	{
		AIWarning("####>CAIObject::SetSignal SIGNAL STRING IS TOO LONG for <%s> :: %s  sz-> %d", GetName(),szText,strlen(szText));
		//		AILogEvent("####>CAIObject::SetSignal <%s> :: %s  sz-> %d",m_sName.c_str(),szText,strlen(szText));
	}
#endif // _DEBUG

	// always process signals sent only to notify wait goal operation
	if (!crcCode)
		crcCode = gAIEnv.SignalCRCs.m_crcGen->GetCRC32(szText);

	// (MATT) This is the only place that the CRCs are used and their implementation is very clumsy {2008/08/09}
	if ( nSignalID != AISIGNAL_NOTIFY_ONLY )
	{
		if( nSignalID != AISIGNAL_ALLOW_DUPLICATES)
		{
			DynArray<AISIGNAL>::iterator ai;
			for (ai=m_State.vSignals.begin();ai!=m_State.vSignals.end();++ai)
			{
				//	if ((*ai).strText == szText)
				//if (!stricmp((*ai).strText,szText))

#ifdef _DEBUG
				if (	!stricmp((*ai).strText,szText) &&	!(*ai).Compare(crcCode) )
				{
					AIWarning("Hash values are different, but strings are identical! %s - %s ",(*ai).strText, szText);
					crcCode = gAIEnv.SignalCRCs.m_crcGen->GetCRC32((*ai).strText);
					return;
				}

				if(stricmp((*ai).strText,szText) && (*ai).Compare(crcCode))
				{
					AIWarning("Please report to alexey@crytek.de! Hash values are identical, but strings are different! %s - %s ",(*ai).strText, szText);
				}
#endif // _DEBUG

				if ((*ai).Compare(crcCode))
					return;
			}
		}

		if (!m_bEnabled && nSignalID != AISIGNAL_INCLUDE_DISABLED)
		{
			// (Kevin) This seems like an odd assumption to be making. INCLUDE_DISABLED needs to be a bit or a separate passed-in value.
			//	WarFace compatibility cannot have duplicate signals sent to disabled AI. (08/14/2009)
			if (gAIEnv.configuration.eCompatibilityMode == ECCM_WARFACE || nSignalID != AISIGNAL_ALLOW_DUPLICATES)
			{
				AILogComment("AIActor %x %s dropped signal \'%s\' due to being disabled",(unsigned)(this),GetName(),szText);
				return;
			}
		}
	}

	//	if((nSignalID >= 0) && !m_bCanReceiveSignals)
	//		return;

	AISIGNAL signal;
	signal.nSignal = nSignalID;

	//signal.strText = szText;
	strncpy(signal.strText,szText,AISIGNAL::SIGNAL_STRING_LENGTH);
	//	strcpy(signal.strText,szText);
	signal.m_nCrcText = crcCode;
	signal.senderID = pSender ? pSender->GetId() : 0;
	signal.pEData = pData;

	IAIRecordable::RecorderEventData recorderEventData(szText);
	if(nSignalID != AISIGNAL_RECEIVED_PREV_UPDATE)// received last update 
	{
		RecordEvent(IAIRecordable::E_SIGNALRECIEVED, &recorderEventData);
		GetAISystem()->Record(this, IAIRecordable::E_SIGNALRECIEVED, szText);
		
		// (MATT) I suspect that the data part is not used at all {2008/08/09}
		NotifySignalReceived( szText, signal.pEData, crcCode );
	}

	// don't let notify signals enter the queue
	if ( nSignalID == AISIGNAL_NOTIFY_ONLY )
		return;

	// If all our early-outs passed and it wasn't just a "notify" then actually enter the signal into the stack!
	pData = NULL; // set to NULL to prevent autodeletion of pData on return

		// need to make sure constructor signal is always at the back - to be processed first
		if(!m_State.vSignals.empty())
		{
			AISIGNAL backSignal(m_State.vSignals.back());
			if (!stricmp("Constructor", backSignal.strText) )
			{
				m_State.vSignals.pop_back();
				m_State.vSignals.push_back(signal);
				m_State.vSignals.push_back(backSignal);
			}
			else
				m_State.vSignals.push_back(signal);
		}
		else
		m_State.vSignals.push_back(signal);
	}

//====================================================================
// IsHostile
//====================================================================
bool CAIActor::IsHostile(const IAIObject* pOther, bool bUsingAIIgnorePlayer) const
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );
	
	if (!pOther)
		return false;

	const CAIObject* pOtherAI = static_cast<const CAIObject*>(pOther);
	
	CAIObject *pAssociation = pOtherAI->GetAssociation().GetAIObject();
	if (pOtherAI->GetType() == AIOBJECT_ATTRIBUTE && pAssociation)
	{
		pOtherAI = pAssociation;
	}

	unsigned short nType = pOtherAI->GetType();
	if(	nType == AIOBJECT_GRENADE )//|| nType == AIOBJECT_RPG )
			return true;

	if (bUsingAIIgnorePlayer && nType == AIOBJECT_PLAYER && gAIEnv.CVars.IgnorePlayer > 0)
		return false;

	const CAIActor* pOtherActor = pOtherAI->CastToCAIActor();

// FIXME- handle dummy/memory targets properly
	if (!pOtherActor)
	{
		return (pOtherAI->GetType() == AIOBJECT_TARGET) && 
			(m_Parameters.m_nSpecies != pOtherAI->GetSpecies()) &&
			(pOtherAI->GetSpecies() >= 0);
	}

	if (bUsingAIIgnorePlayer && (m_Parameters.m_bAiIgnoreFgNode || pOtherActor->GetParameters().m_bAiIgnoreFgNode))
		return false;

	CCCPOINT(CAIActor_IsHostile);

	return (
		m_Parameters.m_bSpeciesHostility && pOtherActor->GetParameters().m_bSpeciesHostility 
		&& m_Parameters.m_nSpecies != pOtherActor->GetParameters().m_nSpecies 
		&& pOtherActor->GetParameters().m_nSpecies>=0 
//		&& GetParameters().m_nSpecies>=0
		);
}


//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::Event(unsigned short eType, SAIEVENT *pEvent)
{
	bool	wasEnabled = m_bEnabled;

	CAIObject::Event(eType, pEvent);

	// Update the group status
	if (wasEnabled != m_bEnabled)
	{
		GetAISystem()->UpdateGroupStatus(GetGroupId());

		IBSSProfileManager *pProfileManager = GetAISystem()->GetBSSProfileManager();
		if( pProfileManager && m_uBSSProfileUserId != g_uBTUserId_Invalid )
		{
			pProfileManager->EnableUser(m_uBSSProfileUserId, m_bEnabled);
		}
	}
}

//====================================================================
// 
//====================================================================
bool CAIActor::CanAcquireTarget(IAIObject* pOther) const
{
	if (!pOther)
		return false;

	CCCPOINT(CAIActor_CanAcquireTarget);

	CAIObject* pOtherAI = (CAIObject*)pOther;
	if (pOtherAI->GetType() == AIOBJECT_ATTRIBUTE && pOtherAI->GetAssociation().IsValid())
		pOtherAI = (CAIObject*)pOtherAI->GetAssociation().GetAIObject();

	CAIActor* pOtherActor = pOtherAI->CastToCAIActor();
	if (!pOtherActor)
		return (pOtherAI->GetType() == AIOBJECT_TARGET);
	
	if (GetAISystem()->GetCombatClassScale(m_Parameters.m_CombatClass, pOtherActor->GetParameters().m_CombatClass )>0)
		return true;
	return false;
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::SetGroupId(int id)
{
	if (id != GetGroupId())
	{
		GetAISystem()->RemoveFromGroup(GetGroupId(), this);
		CAIObject::SetGroupId(id);
		GetAISystem()->AddToGroup(this);

		CAIObject* pBeacon = (CAIObject*)GetAISystem()->GetBeacon(id);
		if (pBeacon)
			GetAISystem()->UpdateBeacon(id, pBeacon->GetPos(), this);

		m_Parameters.m_nGroup = id;
	}
}

void CAIActor::RegisterBehaviorListener(IActorBehaviorListener* listener)
{
	m_behaviorListeners.insert(listener);
}

void CAIActor::UnregisterBehaviorListener(IActorBehaviorListener* listener)
{
	m_behaviorListeners.erase(listener);
}

void CAIActor::BehaviorEvent(EBehaviorEvent event)
{
	BehaviorListeners::iterator it = m_behaviorListeners.begin();
	BehaviorListeners::iterator end = m_behaviorListeners.end();

	for ( ; it != end; )
	{
		BehaviorListeners::iterator next = it;
		++next;

		IActorBehaviorListener* listener = *it;
		listener->BehaviorEvent(this, event);

		it = next;
	}
}

void CAIActor::BehaviorChanged(const char* current, const char* previous)
{
	BehaviorListeners::iterator it = m_behaviorListeners.begin();
	BehaviorListeners::iterator end = m_behaviorListeners.end();

	for ( ; it != end; )
	{
		BehaviorListeners::iterator next = it;
		++next;

		IActorBehaviorListener* listener = *it;
		listener->BehaviorChanged(this, current, previous);

		it = next;
	}
}


//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::SetParameters(AgentParameters & sParams)
{
	SetGroupId(sParams.m_nGroup);
	SetSpecies(sParams.m_nSpecies);
	m_Parameters = sParams;
}

void CAIActor::UpdateHealthHistory()
{
	if(!GetProxy()) return;
	if(!m_healthHistory)
		m_healthHistory = new CValueHistory<float>(100, 0.1f);
	//better add float functions here
	float health = (float)(GetProxy()->GetActorHealth() + GetProxy()->GetActorArmor());
	float maxHealth = (float)GetProxy()->GetActorMaxHealth();

	m_healthHistory->Sample(health / maxHealth, GetAISystem()->GetFrameDeltaTime());
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIActor::AddPerceptionHandlerModifier(IPerceptionHandlerModifier *pModifier)
{
	return stl::push_back_unique(m_perceptionHandlerModifiers, pModifier);
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIActor::RemovePerceptionHandlerModifier(IPerceptionHandlerModifier *pModifier)
{
	return stl::find_and_erase(m_perceptionHandlerModifiers, pModifier);
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIActor::GetPerceptionHandlerModifiers(TPerceptionHandlerModifiersVector &outModifiers)
{
	outModifiers = m_perceptionHandlerModifiers;
	return ( !m_perceptionHandlerModifiers.empty() );
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::DebugDrawPerceptionHandlerModifiers()
{
	TPerceptionHandlerModifiersVector::iterator itModifier = m_perceptionHandlerModifiers.begin();
	TPerceptionHandlerModifiersVector::iterator itModifierEnd = m_perceptionHandlerModifiers.end();
	EntityId entityId = GetEntityID();
	float fY = 30.0f;

	for (; itModifier != itModifierEnd; ++itModifier)
	{
		(*itModifier)->DebugDraw(entityId, fY);
	}
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIActor::Serialize( TSerialize ser, CObjectTracker& objectTracker )
{
	m_State.Serialize(ser, objectTracker);
	m_Parameters.Serialize(ser);
	SerializeMovementAbility(ser);

	const bool bNeedsPostSerialize = CAIObject::Serialize(ser, objectTracker);

	// (MATT) We need to know our entityid to fetch our proxy, hence the superclass must be serialised first.
	// We currently work with the assumption that an AI entity will have a proxy objects which always persists, and will be ready for us to use {2009/04/20}
	if(GetProxy())
		GetProxy()->Serialize(ser);

	if(ser.IsReading())
	{
		IEntity *pActorEntity = GetEntity();
		assert(pActorEntity);
		if(pActorEntity)
		{
			//not good : this is resetting scripts after serialization!
			IEntityProxy *pProxy = pActorEntity->GetProxy(ENTITY_PROXY_SCRIPT);
			SEntityEvent resetEvent;
			resetEvent.event = ENTITY_EVENT_RESET;
			resetEvent.nParam[0] = 0;
			pProxy->ProcessEvent(resetEvent);
		}

		// only alive puppets or leaders should be added to groups
		bool addToGroup = GetProxy()!=NULL ? GetProxy()->GetActorHealth()>0 : CastToCLeader()!=NULL;
		if(addToGroup)	
			GetAISystem()->AddToGroup(this);
		GetAISystem()->AddToSpecies(this, m_Parameters.m_nSpecies);
		m_probableTargets.clear();
		m_priorityTargets.clear();
		m_usingCombatLight = false;
		m_lightLevel = AILL_LIGHT;
	}

	ser.Value("maxPuppetExposure", m_detectionMax.puppetExposure);
	ser.Value("maxPuppetThreat", m_detectionMax.puppetThreat);
	ser.Value("maxVehicleExposure", m_detectionMax.vehicleExposure);
	ser.Value("maxVehicleThreat", m_detectionMax.vehicleThreat);

	ser.Value("snapshotPuppetExposure", m_detectionSnapshot.puppetExposure);
	ser.Value("snapshotPuppetThreat", m_detectionSnapshot.puppetThreat);
	ser.Value("snapshotVehicleExposure", m_detectionSnapshot.vehicleExposure);
	ser.Value("snapshotVehicleThreat", m_detectionSnapshot.vehicleThreat);

	ser.Value("m_cachedWaterOcclusionValue", m_cachedWaterOcclusionValue);

	ser.Value("m_vLastFullUpdatePos", m_vLastFullUpdatePos);

	return bNeedsPostSerialize;
}


void CAIActor::SerializeMovementAbility(TSerialize ser)
{
	ser.BeginGroup("AgentMovementAbility");
	AgentMovementAbility &moveAbil = m_movementAbility;

	ser.Value("b3DMove",moveAbil.b3DMove);
	ser.Value("bUsePathfinder",moveAbil.bUsePathfinder);
	ser.Value("usePredictiveFollowing",moveAbil.usePredictiveFollowing);
	ser.Value("allowEntityClampingByAnimation",moveAbil.allowEntityClampingByAnimation);
	ser.Value("maxAccel",moveAbil.maxAccel);
	ser.Value("maxDecel",moveAbil.maxDecel);
	ser.Value("minTurnRadius",moveAbil.minTurnRadius);
	ser.Value("maxTurnRadius",moveAbil.maxTurnRadius);
	ser.Value("avoidanceRadius",moveAbil.avoidanceRadius);
	ser.Value("pathLookAhead",moveAbil.pathLookAhead);
	ser.Value("pathRadius",moveAbil.pathRadius);
	ser.Value("pathSpeedLookAheadPerSpeed",moveAbil.pathSpeedLookAheadPerSpeed);
	ser.Value("cornerSlowDown",moveAbil.cornerSlowDown);
	ser.Value("slopeSlowDown",moveAbil.slopeSlowDown);
	ser.Value("optimalFlightHeight",moveAbil.optimalFlightHeight);
	ser.Value("minFlightHeight",moveAbil.minFlightHeight);
	ser.Value("maxFlightHeight",moveAbil.maxFlightHeight);
	ser.Value("maneuverTrh",moveAbil.maneuverTrh);
	ser.Value("velDecay",moveAbil.velDecay);
	ser.Value("pathFindPrediction",moveAbil.pathFindPrediction);
	ser.Value("pathRegenIntervalDuringTrace",moveAbil.pathRegenIntervalDuringTrace);
	ser.Value("teleportEnabled",moveAbil.teleportEnabled);
	ser.Value("lightAffectsSpeed",moveAbil.lightAffectsSpeed);
	ser.Value("resolveStickingInTrace",moveAbil.resolveStickingInTrace);
	ser.Value("directionalScaleRefSpeedMin",moveAbil.directionalScaleRefSpeedMin);
	ser.Value("directionalScaleRefSpeedMax",moveAbil.directionalScaleRefSpeedMax);
	ser.Value("avoidanceAbilities",moveAbil.avoidanceAbilities);
	ser.Value("pushableObstacleWeakAvoidance",moveAbil.pushableObstacleWeakAvoidance);
	ser.Value("pushableObstacleAvoidanceRadius",moveAbil.pushableObstacleAvoidanceRadius);


	ser.BeginGroup("AgentMovementSpeeds");

	AgentMovementSpeeds &moveSpeeds = m_movementAbility.movementSpeeds;
	for (int i=0; i<AgentMovementSpeeds::AMU_NUM_VALUES; i++)
		for (int j=0; j<AgentMovementSpeeds::AMS_NUM_VALUES; j++)
		{
			ser.BeginGroup("range");
			AgentMovementSpeeds::SSpeedRange &range = moveSpeeds.GetRange(j,i);
			ser.Value("def", range.def);
			ser.Value("min", range.min);
			ser.Value("max", range.max);
			ser.EndGroup();
		}

	ser.EndGroup();

	ser.BeginGroup("AgentPathfindingProperties");
	AgentPathfindingProperties &pfProp = m_movementAbility.pathfindingProperties;
	
	pfProp.navCapMask.Serialize (ser);
	ser.Value("triangularResistanceFactor",pfProp.triangularResistanceFactor);
	ser.Value("waypointResistanceFactor",pfProp.waypointResistanceFactor);
	ser.Value("flightResistanceFactor",pfProp.flightResistanceFactor);
	ser.Value("volumeResistanceFactor",pfProp.volumeResistanceFactor);
	ser.Value("roadResistanceFactor",pfProp.roadResistanceFactor);

	ser.Value("waterResistanceFactor",pfProp.waterResistanceFactor);
	ser.Value("maxWaterDepth",pfProp.maxWaterDepth);
	ser.Value("minWaterDepth",pfProp.minWaterDepth);
	ser.Value("exposureFactor",pfProp.exposureFactor);
	ser.Value("dangerCost",pfProp.dangerCost);
	ser.Value("zScale",pfProp.zScale);

	ser.EndGroup();

	ser.EndGroup();
}


//
//------------------------------------------------------------------------------------------------------------------------
float CAIActor::AdjustTargetVisibleRange(const CAIActor& observer, float fVisibleRange) const
{
	return fVisibleRange;
}

//
//------------------------------------------------------------------------------------------------------------------------
float CAIActor::GetMaxTargetVisibleRange(const CAIObject* pTarget) const
{
	const AgentParameters &parameters = GetParameters();

	float fRange = parameters.m_PerceptionParams.sightRange;

	if (pTarget)
	{
		// Use correct range for vehicles
		if (pTarget->GetAIType() == AIOBJECT_VEHICLE && parameters.m_PerceptionParams.sightRangeVehicle > FLT_EPSILON)
		{
			fRange = parameters.m_PerceptionParams.sightRangeVehicle;
		}

		// Allow target to adjust the visible range as needed. This allows effects like
		//	underwater occlusion or light levels to take effect.
		const CAIActor *pTargetActor = CastToCAIActorSafe(pTarget);
		if (pTargetActor && fRange > FLT_EPSILON)
		{
			fRange = pTargetActor->AdjustTargetVisibleRange(*this, fRange);
		}
	}

	return fRange;
}


//
//------------------------------------------------------------------------------------------------------------------------
float CAIActor::GetCloakMaxDist( ) const 
{
	// Marcio: Reintroduced cloakScale in the max distance calculation
	float cloakMaxD( gAIEnv.CVars.CloakMaxDist );
	cloakMaxD = cloakMaxD + (m_Parameters.m_PerceptionParams.sightRange-cloakMaxD)*(1.f-m_Parameters.m_fCloakScale);
	return cloakMaxD;
}


//
//------------------------------------------------------------------------------------------------------------------------
float CAIActor::GetCloakMinDist( ) const
{
	float cloakMinD( gAIEnv.CVars.CloakMinDist );
	//	cloakMinD = cloakMinD + (m_Parameters.m_PerceptionParams.sightRange-cloakMinD)*(1.f-m_Parameters.m_fCloakScale);
	return cloakMinD;
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIActor::IsCloakEffective() const
{
	return !IsUsingCombatLight() && m_Parameters.m_fCloakScale > 0.f && !GetGrabbedEntity();
}


//
//------------------------------------------------------------------------------------------------------------------------
bool CAIActor::IsInvisibleFrom(const Vec3& pos) const 
{
	if (m_Parameters.m_bInvisible)
		return true;
	//is not fully cloaked - can possibly be seen beyond cloakMax distance
	if (!IsCloakEffective())
		return false;
	const float cloakMaxDist = GetCloakMaxDist();
	return Distance::Point_PointSq(GetPos(), pos) > sqr(cloakMaxDist);
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::NotifyDeath()
{
	IBSSProfileManager *pProfileManager = GetAISystem()->GetBSSProfileManager();
	if( pProfileManager && m_uBSSProfileUserId != g_uBTUserId_Invalid )
	{
		pProfileManager->EnableUser(m_uBSSProfileUserId, false);
	}

	gAIEnv.pGroupSystem->DestroyGroupMember( GetEntityID() );
}

//
//------------------------------------------------------------------------------------------------------------------------
static void CheckAndAddPhysEntity(std::vector<IPhysicalEntity*>& skips, IPhysicalEntity* pPhys)
{
	if (!pPhys) return;
	pe_status_pos	stat;
	if ( pPhys->GetStatus(&stat)!=0 && (((1 << stat.iSimClass) & COVER_OBJECT_TYPES) != 0) )
		skips.push_back(pPhys);
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::GetPhysicsEntitiesToSkip(std::vector<IPhysicalEntity*>& skips) const
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	if (!GetProxy())
		return;

	if (IPhysicalEntity* pPhys = GetProxy()->GetPhysics(false))
		skips.push_back(pPhys);

	if (IPhysicalEntity* pPhys = GetProxy()->GetPhysics(true))
		skips.push_back(pPhys);

	// if holding something in hands - skip it for the vis check
	IEntity *pGrabbedEntity(GetGrabbedEntity());
	if(pGrabbedEntity)
		CheckAndAddPhysEntity(skips, pGrabbedEntity->GetPhysics());
	
	SAIBodyInfo bi;
	GetProxy()->QueryBodyInfo(bi);
	if (bi.linkedVehicleEntity)
	{
		CheckAndAddPhysEntity(skips, bi.linkedVehicleEntity->GetPhysics());
		for (int i = 0, ni = bi.linkedVehicleEntity->GetChildCount(); i < ni; ++i)
		{
			IEntity* pChild = bi.linkedVehicleEntity->GetChild(i);
			if (pChild)
				CheckAndAddPhysEntity(skips, pChild->GetPhysics());
		}
	}

	if (IEntity* pEnt = GetEntity())
	{
		for (int i = 0, ni = pEnt->GetChildCount(); i < ni; ++i)
		{
			IEntity* pChild = pEnt->GetChild(i);
			if (pChild)
				CheckAndAddPhysEntity(skips, pChild->GetPhysics());
		}
	}

}

void CAIActor::GetLocalBounds(AABB& bbox) const
{
	bbox.min.Set(0, 0, 0);
	bbox.max.Set(0, 0, 0);

	IEntity* pEntity = GetEntity();
	IPhysicalEntity* pPhysicalEntity = pEntity->GetPhysics();
	if (pPhysicalEntity)
	{
		pe_status_pos pstate;
		if (pPhysicalEntity && pPhysicalEntity->GetStatus(&pstate))
		{
			bbox.min = pstate.BBox[0] / pstate.scale;
			bbox.max = pstate.BBox[1] / pstate.scale;
		}
	}
	else
	{
		return pEntity->GetLocalBounds(bbox);
	}
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIActor::CacheProxy() const
{
	assert(!m_pProxy);  // So far, this should be unset if it is worth updating;

	// This isn't nice to be trawling through, but it should suffice pending a full and proper refactor of CAIProxy usage
	if ( GetEntityID() )
		m_pProxy = gEnv->pGame->GetIGameFramework()->GetAIActorProxy( GetEntityID() );
	else
		m_pProxy = NULL;
}



IEntity *CAIActor::GetPathAgentEntity() const
{
	return GetEntity();
}


const char *CAIActor::GetPathAgentName() const
{
	return GetName();
}


unsigned short CAIActor::GetPathAgentType() const
{
	return GetType();
}

float CAIActor::GetPathAgentPassRadius() const
{
	return GetParameters().m_fPassRadius;
}


Vec3 CAIActor::GetPathAgentPos() const
{
	return GetPhysicsPos();
}


Vec3 CAIActor::GetPathAgentVelocity() const
{
	return GetVelocity();
}

void CAIActor::GetPathAgentNavigationBlockers(NavigationBlockers &navigationBlockers, const struct PathfindRequest *pRequest)
{
	return GetPathAgentNavigationBlockers(navigationBlockers,pRequest);
}


Vec3 CAIActor::GetForcedStartPos() const
{
	return ZERO;
}

const AgentMovementAbility &CAIActor::GetPathAgentMovementAbility() const
{
	return m_movementAbility;
}

void CAIActor::PathEvent(SAIEVENT *pEvent)
{
	Event(AIEVENT_ONPATHDECISION, pEvent);
}

unsigned int CAIActor::GetPathAgentLastNavNode() const
{
	return m_lastNavNodeIndex;
}

void CAIActor::SetPathAgentLastNavNode(unsigned int lastNavNode)
{
	m_lastNavNodeIndex=lastNavNode;
}

void CAIActor::SetPathToFollow( const char *pathName )
{

}

void CAIActor::SetPathAttributeToFollow( bool bSpline )
{

}

void CAIActor::SetPFBlockerRadius( int blockerType, float radius )
{

}

ETriState CAIActor::CanTargetPointBeReached( CTargetPointRequest &request )
{
	request.SetResult(eTS_false); return eTS_false;
}

void CAIActor::SetPointListToFollow( const std::list<Vec3>& pointList,IAISystem::ENavigationType navType,bool bSpline )
{

}

bool CAIActor::UseTargetPointRequest( const CTargetPointRequest &request )
{
	return false;
}

bool CAIActor::GetValidPositionNearby( const Vec3 &proposedPosition, Vec3 &adjustedPosition ) const
{
	return false;
}

bool CAIActor::GetTeleportPosition( Vec3 &teleportPos ) const
{
	return false;
}
