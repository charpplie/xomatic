#include "StdAfx.h"
#include "AnimationGraphState.h"
#include "IConsole.h"
#include "CryAction.h"
#include "ICryAnimation.h"
#include "StateIndex.h"
#include "IActorSystem.h"
#include "AGAnimation.h"
#include "PersistantDebug.h"
#include "ExactPositioning.h"
#include "AnimatedCharacter.h"
#include "IVisualLog.h"

#pragma warning(disable: 4244)

// enable this to check nan's on position updates... useful for debugging some weird crashes
#define ENABLE_NAN_CHECK

#undef CHECKQNAN_FLT
#if defined(ENABLE_NAN_CHECK) && !defined(CHECKQNAN_FLT)
#define CHECKQNAN_FLT(x) \
	CRY_ASSERT(*(unsigned*)&(x) != 0xffffffffu)
#else
#define CHECKQNAN_FLT(x) (void*)0
#endif

#ifndef CHECKQNAN_VEC
#define CHECKQNAN_VEC(v) \
	CHECKQNAN_FLT(v.x); CHECKQNAN_FLT(v.y); CHECKQNAN_FLT(v.z)
#endif

ITimer * CAnimationGraphState::m_pTimer;
EntityId CAnimationGraphState::m_debugEntity;
int CAnimationGraphState::m_debugLayer;
bool CAnimationGraphState::m_debugBreak;
bool CAnimationGraphState::m_testPlanner;
/*
static void DrawArrow( Vec3 pos, Vec3 dir, ColorB clr, float height = 2.0f, float scale = 5.0f, float radius = 0.4f )
{
	if (dir.GetLength() < 0.001f)
		return;
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine( pos+Vec3(0,0,height)-0.5f*scale*dir, clr, pos+Vec3(0,0,height)+0.5f*scale*dir, clr );
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone( pos+Vec3(0,0,height)+0.5f*scale*dir, dir.GetNormalized(), radius, radius, clr );
}
*/
#define AG_SEND_EVENT( func ) \
{ \
  m_callingListeners = m_listeners; \
	for (std::vector<SListener>::reverse_iterator agSendEventIter = m_callingListeners.rbegin(); agSendEventIter != m_callingListeners.rend(); ++agSendEventIter) \
		agSendEventIter->pListener->func; \
}
// insert this above for an idea of how long sending events from the anim graph is taking
//	FRAME_PROFILER("AG_SEND_EVENT:"#func, GetISystem(), PROFILE_ACTION); 

#define CHECK_ENTER_BREAKMODE(cvar, equals) \
	if (m_debugEntity && m_basicState.pEntity && m_debugEntity == m_basicState.pEntity->GetId()) \
		if (CAnimationGraphCVars::Get().cvar->GetString() == equals) \
			CAnimationGraphCVars::Get().m_breakMode = 2;

// this monstrosity is for logging when transition changes get hard to follow
// -- ENABLE_DETAILED_LOGGING can be:
//    0: no logging
//    1: a file per entity
//    2: a single file with entities labeled
#define ENABLE_DETAILED_LOGGING 0

#if ENABLE_DETAILED_LOGGING
# define LOGAGDEBUG(a,b,c) LogAGDebug(a,b,c)
void LogAGDebug( IEntity * pEnt, const char * state, size_t numActiveTransitions )
{
	char filename[256];
	int id = pEnt ? pEnt->GetId() : 0;
	if (pEnt==NULL)
	{
		strcpy(filename, "animation_graph_debug_music.txt");
	}
	else
	{
#if ENABLE_DETAILED_LOGGING == 1
		sprintf( filename, "animation_graph_debug_%s_%.8x.txt", pEnt->GetName(), id );
#else
		strcpy( filename, "animation_graph.txt" );
#endif
	}
	FILE * f = fopen(filename, "at");
	if (!f)
		return;
#if ENABLE_DETAILED_LOGGING == 1
	fprintf(f, "%s %d\n", state, numActiveTransitions);
#else
	fprintf(f, "%.8x %s %s %d\n", id, pEnt ? pEnt->GetName() : "music_graph", state, numActiveTransitions);
#endif
	fclose(f);
}
#else
# define LOGAGDEBUG(a,b,c)
#endif

// handle a change in ag_debug
void CAnimationGraphState::ChangeDebug(const char* pVal)
{
	assert (pVal != 0);
	m_debugEntity = 0;
	if (0 == strcmp(pVal, "1"))
	{
		if (IActor * pActor = CCryAction::GetCryAction()->GetClientActor())
			m_debugEntity = pActor->GetEntityId();
	}
	else if (IEntity * pEntity = gEnv->pEntitySystem->FindEntityByName(pVal))
	{
		m_debugEntity = pEntity->GetId();
	}
	m_debugBreak = false;
	CCryAction::GetCryAction()->PauseGame(false,true);
}

// handle a change in ag_debugLayer
void CAnimationGraphState::ChangeDebugLayer(int iVal)
{
	m_debugLayer = iVal;
}

/*
 * CORE SYSTEM
 */

CAnimationGraphState::CAnimationGraphState( _smart_ptr<CAnimationGraph> pGraph ) : 
	m_token(1000), 
	m_bActivated(true), 
	m_pauseState(0), 
	m_inputLocks(0), 
	m_nextQueryID(70), 
	m_currentStateID(INVALID_STATE), 
	m_catchupFlag(false),
	m_usingTriggerTransition(false),
	m_randomNumber(cry_rand()),
	m_pPointVerifier(NULL),
	m_bHurry(false),
	m_firstPersonMode(false),
  m_blendSpaceWeightFlags(0xFFFF),
	m_pAnimatedCharacter(0),
	m_pParentLayerState(0),
	m_layerIndex(0)
{
	m_pendingLeftStateIDLast = 0;
	m_pendingLeftStateIDsCount = 0;

	m_isStateNodeCacheValid = false;

	if (!m_pTimer)
		m_pTimer = gEnv->pTimer;

	for (size_t i=0; i<CAnimationGraph::MAX_INPUTS; i++)
	{
		m_queryChangedInputIDs[i] = m_inputQueryIDs[i] = 0;
	}

	m_pAnimatedCharacter = NULL;
	m_pGraph = pGraph;
	m_resetLock = false;
	DoReset(true);
}

CAnimationGraphState::~CAnimationGraphState()
{
	AG_SEND_EVENT(DestroyedState(this));
}

void CAnimationGraphState::SendQueryComplete( TAnimationGraphQueryID id, bool success )
{
	AG_SEND_EVENT(QueryComplete(id, success));
}

void CAnimationGraphState::NotifyFinishPoint( const Vec3& position, const Quat& orientation )
{
	if ( m_pPointVerifier )
		m_pPointVerifier->NotifyFinishPoint( position );
}

void CAnimationGraphState::SetTargetPointVerifier( IAnimationGraphTargetPointVerifier * pVerifier )
{
	m_pPointVerifier = pVerifier;
}

void CAnimationGraphState::SetAnimationActivation( bool activated )
{
	m_bActivated = activated;
}

bool CAnimationGraphState::GetAnimationActivation()
{
	return m_bActivated;
}

void CAnimationGraphState::Reset()
{
	DoReset(true);
}

void CAnimationGraphState::ClearOverrides()
{
	for (int i=0; i<SAnimationStateData::MAX_LAYERS; i++)
		m_basicState.overrides[i].ClearOverrides();
}

void CAnimationGraphState::DoReset( bool clearInputs )
{
	CRY_ASSERT(!m_resetLock);

	InvalidateStateNodeCache(true);

	m_token = m_token ^ 0xf0f0;
	for (size_t i=0; i<m_currentStateNodes.size(); i++)
	{
		if (m_currentStateNodes[i])
		{
			m_currentStateNodes[i]->LeaveState( m_basicState );
			m_currentStateNodes[i]->LeftState( m_basicState, m_currentStateWasEntered );
		}
	}
	m_token = m_token ^ 0xf0f0;
	ClearOverrides();

	PublishLeftStates(INVALID_STATE);

	SetBasicStateData(m_basicState);

	m_curSerial = m_pGraph->m_serial;
	m_state = &CAnimationGraphState::Update_InitialState;
	m_inputValueCacheIndex = 0;
	m_queriedStateID = m_currentStateID = m_nextStateID = CAnimationGraph::INVALID_STATE;
	m_queryConsideredInputs = 0xffffffff;
	if (clearInputs)
	{
		for (size_t i=0; i<CAnimationGraph::MAX_INPUTS; i++)
		{
			if (m_inputQueryIDs[i])
			{
				AG_SEND_EVENT(QueryComplete(m_inputQueryIDs[i], false));
			}
			if (m_queryChangedInputIDs[i])
			{
				AG_SEND_EVENT(QueryComplete(m_queryChangedInputIDs[i], false));
			}
			if (m_pGraph->m_inputValues.size() > i)
				m_inputValueCache[0][i] = m_inputValueCache[1][i] = m_pGraph->m_inputValues[i]->defaultValue;
			else
				m_inputValueCache[0][i] = m_inputValueCache[1][i] = CStateIndex::INPUT_VALUE_DONT_CARE;
			m_inputValuesAsFloats[i] = 0.0f;
			m_inputQueryIDs[i] = 0;
			m_queryChangedInputIDs[i] = 0;
		}

		m_variationInputValues.clear();
		m_variationInputValues.resize( m_pGraph->m_variationInputIDs.size() );
	}
	m_nextAllowedTransition = 0.0f;
	m_stickyTransitionCompletion = 0.0f;
	m_activeTransition.Clear();
	for (TForcedStates::const_iterator iter = m_forcedStates.begin(); iter != m_forcedStates.end(); ++iter)
	{
		if (iter->queryID)
			AG_SEND_EVENT(QueryComplete(iter->queryID, false));
	}
	m_forcedStates.resize(0);
	m_nextNeedEnter.resize(0);
	m_currentNeedExit.resize(0);
	m_updateNodes.resize(0);
	m_currentStateNodes.resize(0);
	m_nextStateNodes.resize(0);
	m_currentStateNodeFactories.resize(0);
	m_nextStateNodeFactories.resize(0);
	m_outputs.clear();
	m_queryInvalidated = false;
	m_activeTimeAlignmentMask = 0;
	m_currentStateWasEntered = false;
	m_lastEnteredState = INVALID_STATE;
	m_lastEnteredToken = 0;
	for (int i=0; i<sizeof(m_blendSpaceWeights)/sizeof(float); i++)
		m_blendSpaceWeights[i] = 0;  
	m_bHurry = false;
	m_pExactPositioning.reset();
	m_usingTriggerTransition = false;
	m_oldStates.Clear();
	m_blendSpaceWeightFlags = 0xffff;
	m_catchupFlag = false;

	if (m_basicState.pEntity)
	{
		bool isDebugEntity = m_debugEntity && m_debugEntity == m_basicState.pEntity->GetId();
		if (isDebugEntity)
			m_debugBreak = CAnimationGraphCVars::Get().m_breakMode != 0;
	}

	m_noPhysicalCollider.Reset();
}

void CAnimationGraphState::CallLeftQueries( PendingLeftQueries& qrys )
{
	m_callingPendingLeftQueries = qrys;
	qrys.resize(0);
	for (PendingLeftQueries::const_iterator iter = m_callingPendingLeftQueries.begin(); iter != m_callingPendingLeftQueries.end(); ++iter)
		AG_SEND_EVENT(QueryComplete(*iter, true));
}

void CAnimationGraphState::PublishLeftStates( StateID fromState )
{
	// workaround: [Dejan] fixes QS/QL bug while exiting ladders on top...
	// We have to clear all outputs if there are no state nodes to 'leave'
	// with LeftState at this moment (m_pendingLeftStates is empty) and
	// last entered state is unknown (m_lastEnteredState == INVALID_STATE).
	// Proper solution would probably be to serialize m_pendingLeftStates...
	if ( (m_lastEnteredState == INVALID_STATE) && m_pendingLeftStates.empty() )
		m_outputs.clear();

	for (PendingLeftStates::iterator iter = m_pendingLeftStates.begin(); iter != m_pendingLeftStates.end(); ++iter)
	{
		iter->second->LeftState( m_basicState, true );
	}
	m_pendingLeftStates.clear();

	StateID addPendingStateID = INVALID_STATE;
	if (m_pendingLeftStateIDsCount && m_currentStateID != INVALID_STATE && fromState != INVALID_STATE)
	{
		addPendingStateID = m_currentStateID;
		CRY_ASSERT(m_pendingLeftStateIDLast == addPendingStateID);
		-- m_pendingLeftStateIDsCount;
	}

/*
	for (std::vector<StateID>::iterator iter = m_pendingLeftStateIDs.begin(); iter != m_pendingLeftStateIDs.end(); ++iter)
	{
		const CAnimationGraph::SStateInfo info = m_pGraph->m_states[*iter];
		if (m_noPhysicalCollider.LeaveState( info.noPhysicalCollider ))
		{
			SGameObjectEvent goe( "EnablePhysicalCollider", eGOEF_ToAll );
			m_basicState.pGameObject->SendEvent( goe );
		}
	}
*/

	m_pendingLeftStateIDsCount = 0;
	if (addPendingStateID != INVALID_STATE)
	{
		++ m_pendingLeftStateIDsCount;
		m_pendingLeftStateIDLast = addPendingStateID;
	}

	if (fromState != INVALID_STATE)
	{
		CallLeftQueries( m_pendingLeftQueries );
	}

	CallLeftQueries( m_pendingLeftStateQueries );
}

void CAnimationGraphState::DebugLeftState( StateID state, bool cancelled )
{
	// workaround: should go in its own function, but this is called from everywhere we want to update this
	if (m_oldStates.Full())
		m_oldStates.Pop();
	m_oldStates.Push( SOldState(m_currentStateID, cancelled) );
}

void CAnimationGraphState::QueryLeaveState( TAnimationGraphQueryID * pQueryID )
{
	TAnimationGraphQueryID queryID = m_nextQueryID++;
	*pQueryID = queryID;
	m_pendingLeftQueries.push_back( queryID );
}

void CAnimationGraphState::Pause( bool pause, EAnimationGraphPauser pauser )
{
	bool bWasPaused = m_basicState.isPaused;

	if (pause)
		m_pauseState |= (1<<pauser);
	else
		m_pauseState &= ~(1<<pauser);
	m_basicState.isPaused = (m_pauseState != 0);

	if (bWasPaused != m_basicState.isPaused)
	{
		// If pausing/resuming, make sure current nodes know about it
		for (StateNodeVec::iterator it = m_currentStateNodes.begin(); it != m_currentStateNodes.end(); ++it)
		{
			(*it)->Paused(m_basicState, m_basicState.isPaused);
		}
	}
}

void CAnimationGraphState::Release()
{
	delete this;
}

void CAnimationGraphState::SetBasicStateData( const SAnimationStateData& data )
{
	m_basicState = data;
	m_basicState.pState = this;
}

float CAnimationGraphState::GetInputAsFloat( InputID id )
{
	if (id >= CAnimationGraph::MAX_INPUTS)
		return 0.0f;
	return m_inputValuesAsFloats[id];
}

CAnimationGraphState::InputID CAnimationGraphState::GetInputId( const char *input )
{
	return m_pGraph->LookupInputId(input);
}

CAnimationGraphState::InputID CAnimationGraphState::GetVariationInputId( const char* variationInputName ) const
{
	return m_pGraph->GetVariationInputID(variationInputName);
}

void CAnimationGraphState::ForceTeleportToQueriedState()
{
	QueryResult qr = PerformQuery(true);
	if (qr.foundResult)
	{
		if ( (m_nextStateID != INVALID_STATE && m_nextStateID != qr.state) || (m_queriedStateID != qr.queriedState) )
		{
			if ( m_layerIndex == 0 )
			{
				ClearOverrides();
				if ( m_basicState.pEntity != NULL )
				{
					if ( ICharacterInstance * pCharacter = m_basicState.pEntity->GetCharacter(0) )
					{
/* 
						if ( m_layerIndex == 0 )
							pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();
						else
							pCharacter->GetISkeletonAnim()->StopAnimationInLayer( m_layerIndex, 0.0f );
*/
						// [*DavidR | 21/Jan/2010] The previous commented block created an snap. Instead, we can remove the 
						// flags that can delay the transition and the blend-in will still happen.
						// Should be in synch with CSkeletonAnim::EvaluateTransitionFlags logic
						const int iNumAnims = pCharacter->GetISkeletonAnim()->GetNumAnimsInFIFO(m_layerIndex);
						if (iNumAnims > 0)
						{
							CAnimation& anim = pCharacter->GetISkeletonAnim()->GetAnimFromFIFO(m_layerIndex, iNumAnims - 1);
							anim.m_AnimParams.m_nFlags &= ~(CA_START_AT_KEYTIME | CA_START_AFTER | CA_IDLE2MOVE | CA_MOVE2IDLE);
						}
					}
				}
			}

			m_activeTransition.Clear();
			if ( m_currentStateID != qr.queriedState && qr.queriedState != INVALID_STATE )
			{
				m_activeTransition.Push( qr.queriedState );
				m_queriedStateID = qr.queriedState;
			}
			else if ( m_currentStateID != qr.state && qr.state != INVALID_STATE )
			{
				m_activeTransition.Push( qr.state );
				m_queriedStateID = qr.state;
			}
			m_queryConsideredInputs = 0xffffffff;

			if ( m_state == &CAnimationGraphState::Update_SteadyState && m_activeTransition.Empty() == false )
				m_state = &CAnimationGraphState::Update_EphemeralState;

			if ( m_activeTransition.Empty() == false )
			{
				InvalidateStateNodeCache(true);
				m_nextStateID = m_activeTransition.Front();
				CRY_ASSERT( m_isStateNodeCacheValid == false );
				BuildStateFactories( m_nextStateID, m_nextStateNodeFactories );
				CRY_ASSERT(m_currentStateID != m_nextStateID);
				TransitionStatesFromFactories();
				CRY_ASSERT(m_currentStateID != m_nextStateID);
				m_isStateNodeCacheValid = true;
				BeginTransitionTo( m_nextStateID, false );
			}
			else
			{
				m_state = &CAnimationGraphState::Update_SteadyState;
			}
		}
	}
	else
	{
		GameWarning("CAnimationGraphState::ForceTeleportToQueriedState: failed to find queried state");
	}
}

void CAnimationGraphState::PushForcedState( const char * state, TAnimationGraphQueryID * pQueryID )
{
	StateID id = stl::find_in_map( m_pGraph->m_stateNameToID, state, INVALID_STATE );
	if (id == INVALID_STATE)
	{
		GameWarning("Unable to force state %s as it does not exist in graph %s", state, m_pGraph->GetName());
		if (pQueryID)
			pQueryID = 0;
		return;
	}
	SForcedState forcedState = id;
	if (pQueryID)
		forcedState.queryID = *pQueryID = m_nextQueryID++;
	
	m_forcedStates.push_back(forcedState);
}

void CAnimationGraphState::ClearForcedStates()
{
	for (TForcedStates::iterator iter = m_forcedStates.begin(); iter != m_forcedStates.end(); ++iter)
		if (iter->queryID)
			AG_SEND_EVENT(QueryComplete(iter->queryID, false));
	m_forcedStates.clear();
}

bool CAnimationGraphState::IsInDebugBreak()
{
	return m_debugEntity && m_debugEntity == m_basicState.pEntity->GetId() && m_debugBreak;
}

void CAnimationGraphState::Debug_SingleStep()
{
	m_debugBreak = false;
	CCryAction::GetCryAction()->PauseGame(false,true);
}

bool CAnimationGraphState::Update()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

	m_randomNumberLock = false;

	if (m_pExactPositioning.get())
	{
		m_pExactPositioning->FrameUpdate();
		if (m_pExactPositioning->CanDestroy())
		{
			m_pExactPositioning.reset();
			m_usingTriggerTransition = false;
		}
	}

	if (m_pGraph->m_serial != m_curSerial)
	{
		DoReset(true);
		return false;
	}

	//
	if (m_basicState.pEntity && m_basicState.pEntity->IsHidden() && !(m_basicState.pEntity->GetFlags()&ENTITY_FLAG_UPDATE_HIDDEN))
	{
		DoReset(false);
		return false;
	}
	//

	ICharacterInstance *pCharacter = NULL;
	if (m_basicState.pEntity)
		pCharacter = m_basicState.pEntity->GetCharacter(0);

	if (pCharacter)
	{
		// Cut-Scene that play animation can invalidate animation graph.
		if (!pCharacter->IsAnimationGraphValid())
		{
			DoReset(false);
			return false;
		}
	}

	CAnimationGraph::CURRENT_ANIMGRAPH_DEBUG = m_pGraph;

	bool isDebugEntity = m_debugEntity && (m_basicState.pEntity != NULL) && (m_debugEntity == m_basicState.pEntity->GetId());
	if (!m_debugEntity && CAnimationGraphCVars::Get().m_debugMusic && this == CCryAction::GetCryAction()->GetMusicGraphState())
		isDebugEntity = true;

	if (!CAnimationGraphCVars::Get().m_breakMode)
		m_debugBreak = false;

	if (m_debugBreak && pCharacter)
	{
		CCryAction::GetCryAction()->PauseGame(true,true);
	}

	if (m_debugBreak)
	{
		CTimeValue frameTime = gEnv->pTimer->GetFrameTime();
		m_stickyTransitionCompletion += frameTime;
		m_nextAllowedTransition += frameTime;
		m_stateStartTime += frameTime;
	}
	else
	{
//		m_hurry -= gEnv->pTimer->GetFrameTime();
		// if it's in hurry, keep staying in it until the current state matches the queried one
		if(m_bHurry)
		{
			if (m_queriedStateID != m_queriedStateIDWhenHurriedSet)
				m_bHurry = (m_queriedStateID != m_currentStateID);
		}
	}

	// perform the update - we keep following states until we find one that cannot be
	// exited (oldState != m_state), or until numChanges becomes to large (prevent infinite loops)
	static const int MAX_CHANGES_PER_FRAME = 5;
	int numChanges = 0;
	StateUpdateFunction oldState = 0;

	while (!m_basicState.isPaused && oldState != m_state && numChanges < MAX_CHANGES_PER_FRAME)
	{
		if (m_currentStateID == INVALID_STATE)
			m_basicState.hurried = false;
		else
			m_basicState.hurried = m_bHurry && m_pGraph->m_states[m_currentStateID].hurryable;

		if (isDebugEntity && m_debugBreak)
			break;

		if (!m_forcedStates.empty() && m_forcedStates.front().stateID == m_currentStateID)
		{
			if (m_forcedStates.front().queryID)
				AG_SEND_EVENT(QueryComplete(m_forcedStates.front().queryID, true));
			m_forcedStates.pop_front();
		}

		// don't update if we don't need to
		if (m_pTimer->GetFrameStartTime() < m_nextAllowedTransition)
		{
			UpdateAt(m_nextAllowedTransition);
			break;
		}
		UpdateTransitionTimes();
		if (m_pTimer->GetFrameStartTime() < m_nextAllowedTransition)
		{
			UpdateAt(m_nextAllowedTransition);
			break;
		}

		if (m_currentStateID == INVALID_STATE && m_state != &CAnimationGraphState::Update_InitialState)
			CRY_ASSERT(false);

		if (m_state != &CAnimationGraphState::Update_TransitionState)
			UpdateSignalling();
		if (m_pExactPositioning.get())
			m_pExactPositioning->Update();

		// the actual update - we run a small FSM to do this, to keep the
		// main update routine from becoming unwieldy
		oldState = m_state;
		(this->*m_state)();

		if (m_currentStateID == INVALID_STATE && m_state != &CAnimationGraphState::Update_InitialState)
			CRY_ASSERT(false);

		++numChanges;

		if (CAnimationGraphCVars::Get().m_breakMode != 0)
			m_debugBreak |= oldState != m_state;
	}

	// update those things that request it
	bool doUpdateLoop = true;
	if (isDebugEntity && m_debugBreak)
		doUpdateLoop = false;
	if (doUpdateLoop)
	{
		FRAME_PROFILER("CAnimationGraphState::Update.child_updates",GetISystem(), PROFILE_ACTION);
		for (StateNodeVec::iterator iter = m_updateNodes.begin(); iter != m_updateNodes.end(); ++iter)
		{
			// TODO: define this better (probably more stuff needs to be filled in)
			//(*iter)->Update( m_basicState ); //Diesel cut
		}
	}

	// display debug information if needed
	if (isDebugEntity)
	{
		DebugDisplay();
	}

	if (pCharacter)
	{    
		// toggle animation system debugging - it's very useful to see this side by side with the anim graph debug
//Diesel cut
		//m_basicState.pEntity->GetCharacter(0)->GetISkeletonAnim()->SetDebugging( m_basicState.pEntity->GetId() == m_debugEntity );
//Diesel cut
		// update blend weights
		//UpdateBlendWeights(pCharacter);
	}


	CAnimationGraph::CURRENT_ANIMGRAPH_DEBUG = NULL;
	
	return true;
}

void CAnimationGraphState::Hurry()
{
	//m_hurry = 0.4f;
	m_bHurry = true;
	m_queriedStateIDWhenHurriedSet = m_queriedStateID;

//	m_basicState.hurried = true;

	/*
	StateID checkStates[] = {m_currentStateID, m_queriedStateID, m_lastEnteredState};
	for (int i=0; i<sizeof(checkStates)/sizeof(*checkStates); i++)
	{
		if (checkStates[i] != INVALID_STATE)
			if(m_pGraph->m_states[checkStates[i]].hurryable && m_basicState.pEntity)
				if (ICharacterInstance * pChar = m_basicState.pEntity->GetCharacter(0))
				{
					pChar->GetISkeleton()->StopAnimationInLayer(0);
					break;
				}
	}
	*/

	// we can't just stop all animations and hope that some other will be played this frame.
	// but, we can at least make sure they get all blended out when needed
	bool canHurry = true;
	if ( ICharacterInstance * pChar = m_basicState.pEntity ? m_basicState.pEntity->GetCharacter(0) : NULL )
	{
		ISkeletonAnim* pSkel = pChar->GetISkeletonAnim();
		int nAnimsInFIFO = pSkel->GetNumAnimsInFIFO( m_layerIndex );
		for ( int i = 0; i < nAnimsInFIFO; ++i )
		{
			CAnimation& anim = pSkel->GetAnimFromFIFO( m_layerIndex, i );
			if ( anim.m_AnimParams.m_nUserToken != m_token || m_lastEnteredState != INVALID_STATE && m_pGraph->m_states[m_lastEnteredState].hurryable )
			{
				anim.m_AnimParams.m_nFlags &= ~CA_START_AFTER;
				if ( (anim.m_AnimParams.m_nFlags & CA_START_AT_KEYTIME) && (anim.m_AnimParams.m_nFlags & CA_TRANSITION_TIMEWARPING) )
					anim.m_AnimParams.m_nFlags &= ~(CA_START_AT_KEYTIME | CA_TRANSITION_TIMEWARPING);
				anim.m_AnimParams.m_fTransTime = min( 0.2f, anim.m_AnimParams.m_fTransTime );
			}
			else
				canHurry = false;
		}
	}

	// instead of stopping all animations we clear overrides to allow next transition
	if ( canHurry && m_layerIndex == 0 )
		ClearOverrides();
}

void CAnimationGraphState::Update_InitialState()
{
	// initial state update - keep querying until we find a state that we can legally enter,
	// and then build the state up and enter steady state

	ANIM_PROFILE_FUNCTION;

	LOGAGDEBUG( m_basicState.pEntity, "initial", m_activeTransition.Size() );

	QueryResult qr = PerformQuery( false );
  if (qr.foundResult && qr.state != INVALID_STATE)
	{
		PublishLeftStates( INVALID_STATE );
		m_activeTransition.Clear();
		DebugTransitionSelection(qr);
		m_currentStateID = qr.state;
		m_queriedStateID = qr.state;
		m_queryConsideredInputs = 0xffffffff;
		CRY_ASSERT(m_currentStateID != INVALID_STATE);
		BuildStateFactories( m_currentStateID, m_currentStateNodeFactories );
		CRY_ASSERT(m_currentStateID != INVALID_STATE);
		BuildStatesFromFactories( m_currentStateID, m_currentStateNodeFactories, m_currentStateNodes );
		CRY_ASSERT(m_currentStateID != INVALID_STATE);
		RefreshUpdateStates();
		CRY_ASSERT(m_currentStateID != INVALID_STATE);
		m_stateStartTime = m_pTimer->GetFrameStartTime();
		CRY_ASSERT(m_currentStateID != INVALID_STATE);
		UpdateTransitionTimes();
		CRY_ASSERT(m_currentStateID != INVALID_STATE);
		m_state = &CAnimationGraphState::Update_SteadyState;

		for (size_t i=0; i<m_currentStateNodes.size(); i++)
		{
			IAnimationStateNode * pNode = m_currentStateNodes[i];
			if (pNode)
				pNode->EnteredState( m_basicState );
		}
		m_currentStateWasEntered = true;
		m_lastEnteredState = m_currentStateID;
		if (m_pExactPositioning.get())
			m_pExactPositioning->InvalidatePositions();
	}
}

void CAnimationGraphState::Update_SteadyState()
{
	// steady state update - we are here if the queried "best" state is the current state,
	// or if we've been forced to follow a link from that best state

	// each frame we query for a new state if the inputs change, and if that returns a
	// different best state, we start a transition

	ANIM_PROFILE_FUNCTION;

	CRY_ASSERT(m_currentStateID!=INVALID_STATE);

	LOGAGDEBUG( m_basicState.pEntity, "steady", m_activeTransition.Size() );

	CRY_ASSERT( m_activeTransition.Empty() );

	QueryResult qr = PerformQuery( true ); // m_pGraph->m_states[m_currentStateID].allowSelect );
	if (MaybeBeginTransition(qr, m_queriedStateID) && !m_activeTransition.Empty())
	{
		BeginNextTransition();
		return;
	}
	else if (!m_activeTransition.Empty())
	{
		m_state = &CAnimationGraphState::Update_EphemeralState;
	}
}

void CAnimationGraphState::Update_EphemeralState()
{
	// ephemeral state update - we're in a state that is part of a transition to another state

	// each frame we query for what the best state should be - if we end up there, then we stop
	// the transition; if the target changes, we re-pathfind, otherwise we wait until we can
	// leave the state, and then enter the next one

	ANIM_PROFILE_FUNCTION;

	LOGAGDEBUG( m_basicState.pEntity, "ephemeral", m_activeTransition.Size() );

	if (m_pExactPositioning.get())
		m_pExactPositioning->CheckEphemeralToken();

	if (m_activeTransition.Empty())
	{
		m_state = &CAnimationGraphState::Update_SteadyState;
		return;
	}
//	CRY_ASSERT( !m_activeTransition.empty() );

	QueryResult qr = PerformQuery( true );
	bool queryChanged = false;
	if (qr.foundResult)
	{
		if (qr.state == m_currentStateID)
		{
			m_activeTransition.Clear();
			m_state = &CAnimationGraphState::Update_SteadyState;
		}
		else
		{
			queryChanged = qr.queriedState != m_queriedStateID;
			MaybeBeginTransition( qr, m_activeTransition.Back() );
		}
	}

	if (m_activeTransition.Empty())
		m_state = &CAnimationGraphState::Update_SteadyState;
	else
	{
		m_basicState.queryChanged = queryChanged;
		if (CanLeaveState( PeekNextAnimState() ))
			BeginNextTransition();
		m_basicState.queryChanged = false;
	}
}

void CAnimationGraphState::Update_TransitionState()
{
	// transition state - added in case we ever want to define things that can slow down
	// the transition from one state to another

	// this isn't really used at the moment however

	ANIM_PROFILE_FUNCTION;

	CRY_ASSERT(m_currentStateID != m_nextStateID);

	LOGAGDEBUG( m_basicState.pEntity, "transition", m_activeTransition.Size() );

	int numWaiting = 0;
	int numEntered = 0;
	//if (!m_basicState.hurried)
	{
		for (size_t i=0; !numEntered && i<m_currentStateNodes.size(); i++)
		{
			IAnimationStateNode * pNode = m_currentStateNodes[i];
			if (pNode)
			{
/* //Diesel cut
				switch (pNode->HasEnteredState(m_basicState))
				{
				case eHES_Entered:
					numEntered ++;
					break;
				case eHES_Waiting:
					numWaiting ++;
					break;
				}
*/ //Diesel cut
			}
		}
	}

	if ((numWaiting == 0) || (numEntered != 0))
	{
		StateID fromState = m_currentStateID;
		m_currentStateID = m_nextStateID;

		m_currentStateWasEntered = (numEntered > 0);
		if (m_currentStateWasEntered)
		{
			PublishLeftStates( fromState );

			m_lastEnteredState = m_currentStateID;
			m_lastEnteredToken = m_token;

			for (size_t i=0; i<m_currentStateNodes.size(); i++)
			{
				IAnimationStateNode * pNode = m_currentStateNodes[i];
				if (pNode)
					pNode->EnteredState( m_basicState );
			}
			if (m_pExactPositioning.get())
				m_pExactPositioning->InvalidatePositions();
		}

		if (m_activeTransition.Empty())
		{
			if (!m_randomNumberLock)
			{
				m_randomNumber = cry_rand();
				m_randomNumberLock = true;
			}

			m_state = &CAnimationGraphState::Update_SteadyState;
		}
		else
		{
			m_state = &CAnimationGraphState::Update_EphemeralState;
		}
	}
	else if (m_lastEnteredState != INVALID_STATE && m_lastEnteredState != m_nextStateID && (!m_pExactPositioning.get() || m_pExactPositioning->AllowRollbacks()))
	{
		QueryResult qr = PerformQuery( true );
		if (qr.foundResult && qr.queriedState != m_queriedStateID)
		{
			if (qr.state == m_currentStateID)
			{
				RollbackState();
				m_state = &CAnimationGraphState::Update_SteadyState;
				m_queriedStateID = m_currentStateID;
			}
			else if (qr.state == m_nextStateID)
			{
			}
			else
			{
				PathFind( m_currentStateID, qr );
				if (m_activeTransition.Front() != m_nextStateID)
				{
					RollbackState();
					m_state = &CAnimationGraphState::Update_SteadyState;
				//	m_queriedStateID = m_currentStateID;
				}
				else
				{
				//	PopActiveTransitionState();
				}
			}
		}
	}
}

void CAnimationGraphState::PopActiveTransitionState()
{
	CRY_ASSERT(m_activeTransition.Size());
	if (m_usingTriggerTransition)
	{
		CRY_ASSERT( m_pExactPositioning.get() );
		if ( !m_pExactPositioning->GetTriggerTransition().Empty() )
		{
			CRY_ASSERT( m_pExactPositioning->GetTriggerTransition().Front() == m_activeTransition.Front() );
			m_pExactPositioning->PopFrontTransition();
		}
	}
	m_activeTransition.Pop();
}

void CAnimationGraphState::PathFind( StateID from, const QueryResult& to )
{
	m_activeTransition.Clear();
	//SAnimationMovement movement;
	if (to.useTriggeredTransition)
	{
		GetManager()->Log(ColorF(1,1,1,1), m_basicState.pEntity, "Use triggered transition");
		CRY_ASSERT( m_pExactPositioning.get() );
		m_activeTransition = m_pExactPositioning->GetTriggerTransition();
		m_usingTriggerTransition = true;
	}
	else if (from == to.state)
	{
		GetManager()->Log(ColorF(1,1,1,1), m_basicState.pEntity, "WARNING: Requested pathfinding between the same nodes");
		m_usingTriggerTransition = false;
	}
	else if (from == to.queriedState && from != to.state)
	{
		// simplified handling for this special case
		// path from a null node to a force-followed state is requested
		GetManager()->Log(ColorF(1,1,1,1), m_basicState.pEntity, "Simplified pathfinding to a force-followed state");
		m_activeTransition.Push(to.state); //Diesel cut
		m_usingTriggerTransition = false;
	}
	else if (!m_catchupFlag)
	{
		// CRY_ASSERT( m_lastPathFind != m_pTimer->GetFrameStartTime() );

		CAnimationGraph::SPathFindParams params;
		params.destinationStateID = to.state;
		params.pTransitions = &m_activeTransition;
		//params.pMovement = &movement;
		params.pCurInputValues = &m_inputValueCache[m_inputValueCacheIndex][0];
		params.radius = -1.0f;
		params.time = -1.0f;
		params.pEntity = m_basicState.pEntity;
		params.pGraphState = this;
		params.pStats = &m_pathFindStats;
		params.randomNumber = m_randomNumber;

		ETriState success = BasicPathfind(from, params);
		if (CAnimationGraphCVars::Get().m_log)
		{
			string path;
			for (StateList::SIterator iter = m_activeTransition.Begin(); iter != m_activeTransition.End(); ++iter)
			{
				if (!path.empty())
					path += " -> ";
				path += string(m_pGraph->m_states[*iter].id.c_str());
			}
			GetManager()->Log(ColorF(1,1,1,1), m_basicState.pEntity, "Pathfind: %s [%s]", path.c_str(), (success==eTS_true)? "ok" : "failed/maybe");
		}
		m_lastPathFind = m_pTimer->GetFrameStartTime();
		m_usingTriggerTransition = false;
	}
	else
	{
		GetManager()->Log(ColorF(1,1,1,1), m_basicState.pEntity, "Use catchup mode pathfinding");
		//m_activeTransition.Push(to.state); //Diesel cut
		m_usingTriggerTransition = false;
	}
}

void CAnimationGraphState::RollbackState()
{
	m_currentStateID = m_nextStateID;
	InvalidateStateNodeCache(true); //?
	BeginTransitionTo( m_lastEnteredState, true );
	m_activeTransition.Clear();
	m_currentStateID = m_nextStateID;
	InvalidateStateNodeCache(true); //?
}

void CAnimationGraphState::InvalidateStateNodeCache( bool leaveStates )
{
	if (!m_isStateNodeCacheValid)
		return;
	if (leaveStates)
	{
		// didn't end up needing those newly created states after all
		for (int i=0; i<m_nextStateNodes.size(); i++)
		{
			if (m_nextNeedEnter[i])
				m_nextStateNodes[i]->LeftState(m_basicState, false);
		}
	}
	m_isStateNodeCacheValid = false;
}

bool CAnimationGraphState::CanLeaveState( StateID toState )
{
	if (toState == INVALID_STATE)
		return false;

	if (m_nextStateID != toState)
	{
		InvalidateStateNodeCache(true);
		m_nextStateID = toState;
	}
	if (!m_isStateNodeCacheValid)
	{
		BuildStateFactories( m_nextStateID, m_nextStateNodeFactories );
		CRY_ASSERT(m_currentStateID != m_nextStateID);
		TransitionStatesFromFactories();
		CRY_ASSERT(m_currentStateID != m_nextStateID);
		m_isStateNodeCacheValid = true;
	}

	if (m_basicState.hurried)
		return true;

	if (!m_bActivated)
		return false;

	bool allNodesCanLeave = true;
	for (size_t i=0; allNodesCanLeave && i<m_currentStateNodes.size(); i++)
	{
		if (m_currentNeedExit[i])
		{
			//IAnimationStateNode * pNode = m_currentStateNodes[i]; //Diesel cut
			//if (pNode) allNodesCanLeave &= pNode->CanLeaveState( m_basicState ); //Diesel cut
		}
/* code left to see if anything *could* prevent us from leaving this state; breakpoint on int i=0 to make it useful again
		else
		{
			IAnimationStateNode * pNode = m_currentStateNodes[i];
			if (pNode)
				if (!pNode->CanLeaveState( m_basicState ))
					int i=0;
		}
*/
	}

	return allNodesCanLeave;
}

bool CAnimationGraphState::MaybeBeginTransition( const QueryResult& qr, StateID currentWantId )
{
	ANIM_PROFILE_FUNCTION;

  // no state found, ignore
	if (!qr.foundResult || (qr.state == INVALID_STATE))
	{
		//GameWarning("MaybeBeginTransition called with no state to transition to");
    return false;
	}

	// if we're at the right state, nothing needs to happen
	if (!m_basicState.hurried && (qr.state == currentWantId) && (!m_activeTransition.Empty() || m_currentStateID == currentWantId))
		return false;

	// if we're in a sticky state, then queriedId must be additionally different from m_queriedId
	if (!m_basicState.hurried && (m_pTimer->GetFrameStartTime() < m_stickyTransitionCompletion))
	{
		if (m_queriedStateID == qr.state)
		{
			bool stickyResult = true;
			// but only if there is a link from the queried state to current state
			if (m_queriedStateID != INVALID_STATE)
			{
				if (m_pGraph->m_states[m_queriedStateID].hasForceFollows)
				{
					CAnimationGraph::LinkInfoVec::const_iterator iter = m_pGraph->m_links.begin() + m_pGraph->m_states[m_queriedStateID].linkOffset;
					while (stickyResult && iter != m_pGraph->m_links.end() && iter->from == m_queriedStateID)
					{
						if (iter->to == m_currentStateID)
							stickyResult = false;
						++iter;
					}
				}
			}
			if (!stickyResult)
				return false;
		}
	}

	// in case CanLeaveState fails
	//StateList oldActiveTranstion = m_activeTransition;
	PathFind( m_currentStateID, qr );

	// nodes get a veto right on whether they can leave a state or not
	// this has side effects, so we must always call it here if there is even a chance of returning 'true'
	m_basicState.queryChanged = currentWantId != qr.queriedState;
	bool canLeaveState = CanLeaveState(PeekNextAnimState());
	m_basicState.queryChanged = false;
	if (!m_basicState.hurried && !canLeaveState)
	{
		//m_activeTransition = oldActiveTranstion;
		if (qr.countsAsQueriedState)
		{
			m_queriedStateID = qr.state;
			// m_queryConsideredInputs = 0xffffffff;
		}
		return false;
	}

	// now kick start the transition
	DebugTransitionSelection(qr);

	if (qr.countsAsQueriedState)
	{
		m_queriedStateID = qr.state;
		m_queryConsideredInputs = 0xffffffff;
	}

	return true;
}

void CAnimationGraphState::DebugTransitionSelection( const QueryResult& qr )
{
}

CAnimationGraphState::StateID CAnimationGraphState::PeekNextAnimState() const
{
	if (m_activeTransition.Empty())
		return INVALID_STATE;
	StateList::SConstIterator iterLast = m_activeTransition.End();
	--iterLast;
	for (StateList::SConstIterator iter = m_activeTransition.Begin(); iter != m_activeTransition.End(); ++iter)
	{
		bool pop;
		bool skipTransition;
		CheckTransitionAnimation(*iter, iter == iterLast, pop, skipTransition);
		if (!pop)
			return *iter;
	}
	return INVALID_STATE;
}

void CAnimationGraphState::CheckTransitionAnimation(StateID stateID, bool isLast, bool & pop, bool & skipTransition) const
{
	pop = false;
	skipTransition = false;
	if (stateID == m_currentStateID)
		pop = true;
	else if (m_currentStateID != INVALID_STATE && m_basicState.hurried && !isLast && m_pGraph->m_states[stateID].hurryable)
		pop = true;
	if (m_firstPersonMode && m_pGraph->m_states[stateID].skipFirstPerson)
	{
		pop = true;
		skipTransition = true;
	}
}

void CAnimationGraphState::BeginNextTransition()
{
	ANIM_PROFILE_FUNCTION;

	CRY_ASSERT(PeekNextAnimState() == m_nextStateID);

	//CRY_ASSERT(CanLeaveState());
	while (!m_activeTransition.Empty())
	{
		CRY_ASSERT(PeekNextAnimState() == m_nextStateID);

		bool pop = false;
		bool skipTransition = false;
		CheckTransitionAnimation(m_activeTransition.Front(), m_activeTransition.Size() == 1, pop, skipTransition);

/*
		if (m_activeTransition.Front() == m_currentStateID)
			pop = true;
		else if (m_currentStateID != INVALID_STATE && m_basicState.hurried && m_activeTransition.Size() > 1)
			pop = true;
		if (m_firstPersonMode && m_pGraph->m_states[m_activeTransition.Front()].skipFirstPerson)
		{
			pop = true;
			skipTransition = true;
		}
*/
		if (skipTransition && CAnimationGraphCVars::Get().m_fpAnimPop && m_basicState.pEntity)
		{
			if (ICharacterInstance * pCharacter = m_basicState.pEntity->GetCharacter(0))
			{
				if ( m_layerIndex == 0 )
					pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();
				else
					pCharacter->GetISkeletonAnim()->StopAnimationInLayer( m_layerIndex, 0.0f );
			}
		}
		if (pop)
			PopActiveTransitionState();
		else
			break;

		CRY_ASSERT(PeekNextAnimState() == m_nextStateID);
	}
	if (m_activeTransition.Empty())
	{
		if (m_usingTriggerTransition)
		{
			CRY_ASSERT( m_pExactPositioning.get() );
			CRY_ASSERT( m_pExactPositioning->GetTriggerTransition().Empty() );
		}
		return;
	}
	CRY_ASSERT(PeekNextAnimState() == m_nextStateID);
	CRY_ASSERT(m_activeTransition.Front() == m_nextStateID);
	BeginTransitionTo( m_nextStateID, false );
	PopActiveTransitionState();
}

void CAnimationGraphState::BeginTransitionTo( StateID state, bool fromRollback )
{
	if (state != INVALID_STATE)
		GetManager()->Log(ColorF(0.7f,0.7f,0.7f,1), m_basicState.pEntity, "Entering state %s", m_pGraph->m_states[state].id.c_str());
	CRY_ASSERT(state!=INVALID_STATE);
	CRY_ASSERT(m_currentStateID != state);
	DebugLeftState( m_currentStateID, fromRollback );
	if (fromRollback) // otherwise CanLeaveState deals with this
	{
		if (state != m_nextStateID)
		{
			InvalidateStateNodeCache(true);
			m_nextStateID = state;
		}
		if (!m_isStateNodeCacheValid)
		{
			BuildStateFactories( m_nextStateID, m_nextStateNodeFactories );
			CRY_ASSERT(m_currentStateID != m_nextStateID);
			TransitionStatesFromFactories();
			CRY_ASSERT(m_currentStateID != m_nextStateID);
			m_isStateNodeCacheValid = true;
		}
	}
	CRY_ASSERT(m_nextStateID == state);
	ActivateNextState( fromRollback );
	CRY_ASSERT(m_currentStateID != m_nextStateID);
//	m_lastEnteredState = INVALID_STATE;
	m_state = &CAnimationGraphState::Update_TransitionState;
}

CAnimationGraphState::QueryResult CAnimationGraphState::PerformQuery( bool allowForceFollows )
{
	ANIM_PROFILE_FUNCTION;

	bool wasQueryInvalidated = m_queryInvalidated;
	m_queryInvalidated = false;

	QueryResult qr;
	qr.foundResult = false;
	qr.countsAsQueriedState = false;
	qr.useTriggeredTransition = false;
	qr.state = INVALID_STATE;
	qr.queriedState = INVALID_STATE;
	m_debugLastLocomotionState = INVALID_STATE;

	struct LogCleanup
	{
		LogCleanup(_smart_ptr<CAnimationGraph> pG, IEntity * pE, QueryResult * pQR, StateID currentQueried, StateID currentQueried2) : m_commit(true), m_pG(pG), m_pE(pE), m_pQR(pQR), m_currentQueried(currentQueried), m_currentQueried2(currentQueried)
		{
			m_pG->m_pManager->AddLogMarker();
		}
		~LogCleanup()
		{
			m_commit &= (m_currentQueried != m_pQR->state && m_currentQueried2 != m_pQR->state);
			if (!m_pQR->foundResult || !m_pQR->countsAsQueriedState)
				m_commit = false;
			if (!m_pG->m_pManager->GotDataSinceMarker())
				m_commit = false;
			if (m_commit && m_pQR->state != INVALID_STATE)
				m_pG->m_pManager->Log(ColorF(1,1,0,1), m_pE, "New query result: %s", m_pG->m_states[m_pQR->state].id.c_str());
			m_pG->m_pManager->DoneLogMarker( m_commit );
		}
		void DontCommit() { m_commit = false; }
		bool m_commit;
		_smart_ptr<CAnimationGraph> m_pG;
		IEntity * m_pE;
		QueryResult * m_pQR;
		StateID m_currentQueried;
		StateID m_currentQueried2;
	};
	LogCleanup logCleanup(m_pGraph, m_basicState.pEntity, &qr, m_queriedStateID, m_currentStateID);

	// check if we have a triggered animation - EARLY OUT
	if (m_pExactPositioning.get())
	{
		if (m_pExactPositioning->OverrideQuery(qr, m_state == &CAnimationGraphState::Update_SteadyState))
		{
			GetManager()->Log(ColorF(0,1,0,1), m_basicState.pEntity, "Exact positioning decides current animation state");
			CRY_ASSERT(qr.foundResult);
			return qr;
		}
	}
	// check forced states first
	if (!qr.foundResult && !m_forcedStates.empty())
	{
		const SForcedState& forcedState = m_forcedStates.front();
		qr.foundResult = true;
		qr.state = m_forcedStates.front().stateID;
		qr.queriedState = qr.state;
		qr.countsAsQueriedState = true;
	}
	// then selection criteria
	if (!qr.foundResult)
	{
		CStateIndex::InputValue * inputValueData = &m_inputValueCache[m_inputValueCacheIndex][0];
		bool performedQuery = false;
		if (wasQueryInvalidated || m_currentStateID == INVALID_STATE || 0 != memcmp( &m_inputValueCache[m_inputValueCacheIndex][0], &m_inputValueCache[m_inputValueCacheIndex^1][0], CAnimationGraph::MAX_INPUTS ))
		{
			BasicQuery( inputValueData, m_validStates );
			m_inputValueCacheIndex ^= 1;
			if (CAnimationGraphCVars::Get().m_log)
			{
				string changedStuff;
				for (int i=0; i<m_pGraph->m_inputValues.size(); i++)
				{
					if (m_inputValueCache[0][i] != m_inputValueCache[1][i])
					{
						char value[256];
						m_pGraph->m_inputValues[i]->DebugText( value, m_inputValueCache[m_inputValueCacheIndex^1][i], m_inputValuesAsFloats );
						changedStuff += string().Format(" %s(%s)", m_pGraph->m_inputValues[i]->name.c_str(), value);
					}
				}
				GetManager()->Log(ColorF(1,1,0,1), m_basicState.pEntity, "Changes in the following inputs caused a state change:%s", changedStuff.c_str());
				if (m_validStates.empty())
					GetManager()->Log(ColorF(1,0,0,1), m_basicState.pEntity, "NO VALID STATES FOUND");
			}
			memcpy( m_inputValueCache[m_inputValueCacheIndex], m_inputValueCache[m_inputValueCacheIndex^1], sizeof(m_inputValueCache[0]) );
			performedQuery = true;
		}
		if (!m_validStates.empty())
		{
			bool modifiedOrdering = !performedQuery;
			bool stateSelected = false;
			// allow hysteresis - if the current state matches exactly as well as the newly queried state, keep the current state
			if (m_state == &CAnimationGraphState::Update_SteadyState)
			{
				for (CStateIndex::StateIDVec::iterator iter = m_validStates.begin(); iter != m_validStates.end(); ++iter)
				{
					if ( *iter == m_currentStateID )
					{
						if (iter != m_validStates.begin())
							GetManager()->Log(ColorF(1,1,0,1), m_basicState.pEntity, "Use hysteresis to maintain current state");
						std::swap( *m_validStates.begin(), *iter );
						modifiedOrdering = true;
						stateSelected = true;
						break;
					}
				}
			}
			if (!stateSelected &&
				//m_state != &CAnimationGraphState::Update_SteadyState &&
				m_queriedStateID != INVALID_STATE)
			{
				for (CStateIndex::StateIDVec::iterator iter = m_validStates.begin(); iter != m_validStates.end(); ++iter)
				{
					if ( *iter == m_queriedStateID )
					{
						if (iter != m_validStates.begin())
							GetManager()->Log(ColorF(1,1,0,1), m_basicState.pEntity, "Use hysteresis to maintain current state");
						std::swap( *m_validStates.begin(), *iter );
						modifiedOrdering = true;
						break;
					}
				}
			}
			// TODO: modify logic here to maintain coherency for SelectLocomotionState
			if (!modifiedOrdering && m_validStates.size() > 1)
			{
				std::swap( m_validStates[0], m_validStates[m_randomNumber % m_validStates.size()] );
				GetManager()->Log(ColorF(1,1,0,1), m_basicState.pEntity, "Choose a random state from the set of valid states");
			}

			qr.countsAsQueriedState = true;
			qr.foundResult = true;

			// Only to locomotion state selection for fullbody layer.
			// TODO: This should be totally skipped for music graph. 
			// Well, there is only one music graph in the whole game, maybe we don't mind.
			if (m_pGraph->UseCrysis1LocoStateSelectionLogic() && (m_pAnimatedCharacter != NULL) && (m_layerIndex == 0))
			{
				// TODO: move qr.state to the head of m_validStates, and use it to early out in SelectLocomotionState
				qr.state = m_debugLastLocomotionState = m_pAnimatedCharacter->SelectLocomotionState(m_pGraph, m_currentStateID, m_validStates, m_validStates.front());
			}
			else
			{
				qr.state = m_validStates.front();
			}
			qr.queriedState = qr.state;
		}
	}
	// then any outgoing links that force us to follow them
	bool checkForceFollows = false;
	if (allowForceFollows && m_currentStateID != INVALID_STATE && !qr.useTriggeredTransition && gEnv->bClient)
	{
		if (qr.foundResult && qr.countsAsQueriedState && m_currentStateID == qr.state /*&& m_state == &CAnimationGraphState::Update_SteadyState*/ )
			checkForceFollows = true;
		else if (!qr.foundResult)
			checkForceFollows = true;
	}
	if (checkForceFollows)
	{
		logCleanup.DontCommit();
		const CAnimationGraph::SStateInfo& info = m_pGraph->m_states[m_currentStateID];
		if (info.hasForceFollows)
		{
			for (bool evaluateGuards = true; true ; evaluateGuards = false)
			{
				int totalFollows = 0;
				std::vector< std::pair<int,StateID> >& destinations = m_pGraph->m_destinationsCache;
				destinations.resize(0);
				CAnimationGraph::LinkInfoVec::const_iterator iter;
				for (iter = m_pGraph->m_links.begin() + info.linkOffset; iter != m_pGraph->m_links.end() && iter->from == m_currentStateID; ++iter)
				{
					if (!iter->forceFollowChance)
						continue;
					if (m_firstPersonMode && m_pGraph->m_states[iter->to].skipFirstPerson)
					{
						//skip some states in first person
						continue;
					}
					if (evaluateGuards && !m_pGraph->m_states[iter->to].EvaluateGuards( &m_inputValueCache[m_inputValueCacheIndex][0] ))
					{
						//skip states which are not matching the current input values
						continue;
					}
					totalFollows += iter->forceFollowChance;
					destinations.push_back( std::make_pair(iter->forceFollowChance, iter->to) );
				}
				if (totalFollows == 0)
				{
					//no state passed the guard test
					CRY_ASSERT(evaluateGuards);
					continue;
				}
				CRY_ASSERT(totalFollows);

				int randNum = totalFollows > 1 ? m_randomNumber % totalFollows : 0;
				totalFollows = 0;
				std::vector< std::pair<int,StateID> >::iterator it;
				for (it = destinations.begin(); it != destinations.end(); ++it)
				{
					totalFollows += it->first;
					if (totalFollows > randNum)
						break;
				}
				if (it != destinations.end())
				{
					GetManager()->Log(ColorF(1,1,0,1), m_basicState.pEntity, "Following force-follow link");
					qr.foundResult = true;
					qr.state = it->second;
					qr.countsAsQueriedState = false;
				}
				break;
			}
		}
	}

	// set the "replaceme" animation if we have one
	if (!qr.foundResult && m_pGraph->m_replaceMeAnimation != INVALID_STATE)
	{
		GetManager()->Log(ColorF(1,0,0,1), m_basicState.pEntity, "Choosing replace-me animation");
		qr.foundResult = true;
		qr.countsAsQueriedState = true;
		qr.state = m_pGraph->m_replaceMeAnimation;
		qr.queriedState = m_pGraph->m_replaceMeAnimation;
	}

	if (qr.foundResult)
	{
		CRY_ASSERT(qr.state != INVALID_STATE);
		CHECK_ENTER_BREAKMODE( m_pBreakOnQuery, m_pGraph->m_states[qr.state].id );
	}

	return qr;
}

void CAnimationGraphState::BuildStateFactories( StateID state, StateFactoryVec& factories )
{
	// handles inheritance for overridable and non-overridable state node factories
	// most work is in the recursive child function
	factories.resize(0);

	if (state == INVALID_STATE)
	{
		GameWarning("BuildStateFactories with invalid state ID");
		return;
	}

	factories.resize(GetOverrideSlotCount());
	BuildStateFactories_Recurse( state, factories );
}

void CAnimationGraphState::BuildStateFactories_Recurse( StateID state, StateFactoryVec& factories )
{
	// state factory array:
	// overridable slot 0
	// overridable slot 1
	// overridable slot 2
	// overridable slot 3
	// non-overridable slot 0
	// non-overridable slot 1
	// non-overridable slot 2
	// non-overridable slot 3

	const CAnimationGraph::SStateInfo& stateInfo = m_pGraph->m_states[state];
	std::vector<uint8>& factorySlotIndices = m_pGraph->m_factorySlotIndices;
	std::vector<IAnimationStateNodeFactory*>& allFactories = m_pGraph->m_factories;
	int i = 0;
	int overridable = GetOverrideSlotCount();
	CRY_ASSERT(factories.size() == overridable);
	// go through states and set up factories for overridable states defined here
	while (i < stateInfo.factoryLength)
	{
		int slot = factorySlotIndices[i+stateInfo.factoryStart];
		if (slot >= overridable)
			break;
		if (!factories[slot])
			factories[slot] = allFactories[i+stateInfo.factoryStart];
		i++;
	}
	// recurse to our parent state if we have one
	if (stateInfo.parentState == state )
	{
		CryLogAlways( "[Animation Graph Error] Invalid parent for animation graph state %s!", stateInfo.id.c_str() );
		m_pGraph->m_states[state].parentState = CAnimationGraph::INVALID_STATE;
	}
	else if (stateInfo.parentState != CAnimationGraph::INVALID_STATE)
	{
		BuildStateFactories_Recurse( stateInfo.parentState, factories );
	}
	// now append the not-overridden slots - because of our recursion order, we add them
	// in order of parent, child1, child2, most-derived
	while (i < stateInfo.factoryLength)
	{
		int slot = factorySlotIndices[i+stateInfo.factoryStart];
		CRY_ASSERT(slot >= overridable);
		factories.push_back( allFactories[i+stateInfo.factoryStart] );
		i++;
	}























}

void CAnimationGraphState::BuildStatesFromFactories( StateID id, const StateFactoryVec& factories, StateNodeVec& state, bool skipEntering /*=false*/ )
{
	bool oldLock = m_resetLock;
	m_resetLock = true;

	// take the state factory array, and build the state node array
	state.resize(0);
	CRY_ASSERT(m_currentStateID != INVALID_STATE);
	bool logTransition = CAnimationGraphCVars::Get().m_logTransitions != 0;
	if (logTransition)
		CryLogAlways("[ag] '%s' Initial State Transition to '%s' --------------------------------------", m_basicState.pEntity ? m_basicState.pEntity->GetName() : "<unknown>", m_pGraph->m_states[id].id.c_str());
	CRY_ASSERT(m_currentStateID != INVALID_STATE);
	m_basicState.MovementControlMethodH = m_pGraph->m_states[m_currentStateID].MovementControlMethodH;
	m_basicState.MovementControlMethodV = m_pGraph->m_states[m_currentStateID].MovementControlMethodV;
	m_basicState.animationControlledView = m_pGraph->m_states[m_currentStateID].animationControlledView;
	m_basicState.additionalTurnMultiplier = m_pGraph->m_states[m_currentStateID].additionalTurnMultiplier;
	m_basicState.canMix = m_pGraph->m_states[m_currentStateID].canMix;
	for (size_t i=0; i<factories.size(); i++)
	{
		CRY_ASSERT(m_currentStateID != INVALID_STATE);
		if (factories[i])
		{
			CRY_ASSERT(m_currentStateID != INVALID_STATE);
			state.push_back(factories[i]->Create());
			CRY_ASSERT(m_currentStateID != INVALID_STATE);
			if (logTransition)
				CryLogAlways("[ag] EnterState '%s' call node '%s'", m_pGraph->m_states[id].id.c_str(), factories[i]->GetName());
			CRY_ASSERT(m_currentStateID != INVALID_STATE);
			if (!skipEntering)
				state.back()->EnterState( m_basicState, false );
			CRY_ASSERT(m_currentStateID != INVALID_STATE);
		}
	}
	OnEnterState(id);

	m_resetLock = oldLock;
	m_catchupFlag = false;
}

void CAnimationGraphState::OnEnterState( StateID id )
{
	if (id != INVALID_STATE)
	{
		const CAnimationGraph::SStateInfo info = m_pGraph->m_states[id];
/*
		if (m_noPhysicalCollider.EnterState( info.noPhysicalCollider ))
		{
			SGameObjectEvent goe( "DisablePhysicalCollider", eGOEF_ToAll );
			m_basicState.pGameObject->SendEvent( goe );
		}
*/
		++m_pendingLeftStateIDsCount;
		m_pendingLeftStateIDLast = id;
	}
}

void CAnimationGraphState::RefreshUpdateStates()
{
	// rebuild the update list (only those nodes that need updates)
//	m_updateNodes.resize(0); //Diesel cut
	for (size_t i=0; i<m_currentStateNodes.size(); i++)
	{
/*	//Diesel cut	
		if (m_currentStateNodes[i] && (m_currentStateNodes[i]->flags & eASNF_Update))
		{
			m_updateNodes.push_back(m_currentStateNodes[i]);
		}
*/ //Diesel cut
	}
}

void CAnimationGraphState::TransitionStatesFromFactories()
{
	// tricky... figure out the delta between two state factories
	// to minimize our enter/exit state calls - feeds into ActivateNextState
	m_nextNeedEnter.resize(0);
	m_currentNeedExit.resize(0);
	m_nextStateNodes.resize(0);

	StateNodeVec::const_iterator currStateIter = m_currentStateNodes.begin();
	size_t i;
	for (i=0; i<GetOverrideSlotCount(); i++)
	{
		if (m_currentStateNodeFactories.size() > i && m_currentStateNodeFactories[i] != 0)
		{
			CRY_ASSERT( currStateIter != m_currentStateNodes.end() );
			bool needExit = true;
			if (m_nextStateNodeFactories[i] == m_currentStateNodeFactories[i])
			{
				m_nextStateNodes.push_back(*currStateIter);
				needExit = m_nextStateNodeFactories[i]->GetForceReentering();
				m_nextNeedEnter.push_back(needExit);
			}
			else if (m_nextStateNodeFactories[i])
			{
				m_nextStateNodes.push_back(m_nextStateNodeFactories[i]->Create());
				m_nextNeedEnter.push_back(true);
			}
			m_currentNeedExit.push_back(needExit);
			++currStateIter;
		}
		else if (m_nextStateNodeFactories[i])
		{
			m_nextStateNodes.push_back(m_nextStateNodeFactories[i]->Create());
			m_nextNeedEnter.push_back(true);
		}
	}

	const size_t MaxMergePoint = min( m_nextStateNodeFactories.size(), m_currentStateNodeFactories.size() );
	for (; i<MaxMergePoint && m_currentStateNodeFactories[i] == m_nextStateNodeFactories[i]; i++)
	{
		CRY_ASSERT( currStateIter != m_currentStateNodes.end() );
		m_nextStateNodes.push_back( *currStateIter++ );
		m_currentNeedExit.push_back(false);
		m_nextNeedEnter.push_back(false);
	}
	size_t startPoint = i;
	for (; i<m_currentStateNodeFactories.size(); i++)
	{
		bool needExit = startPoint >= m_nextStateNodeFactories.size() ? true :
			std::find( m_nextStateNodeFactories.begin()+startPoint, m_nextStateNodeFactories.end(), m_currentStateNodeFactories[i] )
			== m_nextStateNodeFactories.end();
		m_currentNeedExit.push_back( needExit );
	}
	for (i=startPoint; i<m_nextStateNodeFactories.size(); i++)
	{
		CRY_ASSERT( m_nextStateNodeFactories[i] );

		StateFactoryVec::iterator it = startPoint >= m_currentStateNodeFactories.size() ? m_currentStateNodeFactories.end() :
			std::find( m_currentStateNodeFactories.begin()+startPoint, m_currentStateNodeFactories.end(), m_nextStateNodeFactories[i] );
		if ( it == m_currentStateNodeFactories.end() )
		{
			m_nextStateNodes.push_back( m_nextStateNodeFactories[i]->Create() );
			m_nextNeedEnter.push_back(true);
		}
		else
		{
			m_nextStateNodes.push_back( *(currStateIter + ((it-m_currentStateNodeFactories.begin())-startPoint)) );
			m_nextNeedEnter.push_back(false);
		}
	}

	CRY_ASSERT( m_nextNeedEnter.size() == m_nextStateNodes.size() );
	CRY_ASSERT( m_currentNeedExit.size() == m_currentStateNodes.size() );
}

void CAnimationGraphState::ActivateNextState( bool fromRollback )
{
	bool oldLock = m_resetLock;
	m_resetLock = true;

	CRY_ASSERT( m_nextNeedEnter.size() == m_nextStateNodes.size() );
	CRY_ASSERT( m_currentNeedExit.size() == m_currentStateNodes.size() );
	CRY_ASSERT( m_nextStateID != INVALID_STATE );

	if ( m_currentStateWasEntered )
		m_basicState.overrideTransitionTime = 0.0f;
	if ( !fromRollback )
	{
		const CAnimationGraph::SLinkInfo* pLink = m_pGraph->FindLink( m_currentStateID, m_nextStateID );
		if ( pLink )
			m_basicState.overrideTransitionTime = max( pLink->overrideTransitionTime, m_basicState.overrideTransitionTime );
	}

	// call needed enter/exits from TransitionStatesFromFactories
	bool logTransition = CAnimationGraphCVars::Get().m_logTransitions != 0;

	if (logTransition)
		CryLogAlways("[ag] '%s' Begin State Transition '%s'->'%s' --------------------------------------", m_basicState.pEntity->GetName(), m_pGraph->m_states[m_currentStateID].id.c_str(), m_pGraph->m_states[m_nextStateID].id.c_str());
	for (size_t i=0; i<m_currentStateNodes.size(); i++)
	{
/* //Diesel cut
		if (m_currentNeedExit[i])
		{

			if (logTransition)
				CryLogAlways("[ag] LeaveState '%s' call node '%s'", m_pGraph->m_states[m_currentStateID].id.c_str(), m_currentStateNodes[i]->GetFactory()->GetName());
			if (m_currentStateNodes[i])
			{
				m_currentStateNodes[i]->LeaveState( m_basicState );
				if (m_currentStateWasEntered)
					m_pendingLeftStates.insert( std::make_pair(m_token, m_currentStateNodes[i]) );
				else
					m_currentStateNodes[i]->LeftState( m_basicState, false );
			}
			else
				CryLogAlways("Warning: should have left state node, but it was NULL")
		}
		else
		{
			if (logTransition)
				CryLogAlways("[ag] DontLeaveState '%s'->'%s' node '%s'", m_pGraph->m_states[m_currentStateID].id.c_str(), m_pGraph->m_states[m_nextStateID].id.c_str(), m_currentStateNodes[i]->GetFactory()->GetName());
		}
*/ //Diesel cut

	}
	if (fromRollback)
	{
		m_token = m_lastEnteredToken;
		m_currentStateWasEntered = true;
	}
	else
	{
		m_token += m_currentStateWasEntered;
		m_currentStateWasEntered = false;
	}
	m_basicState.MovementControlMethodH = m_pGraph->m_states[m_nextStateID].MovementControlMethodH;
	m_basicState.MovementControlMethodV = m_pGraph->m_states[m_nextStateID].MovementControlMethodV;
	m_basicState.animationControlledView = m_pGraph->m_states[m_nextStateID].animationControlledView;
	m_basicState.additionalTurnMultiplier = m_pGraph->m_states[m_nextStateID].additionalTurnMultiplier;
	m_basicState.canMix = m_pGraph->m_states[m_nextStateID].canMix;
	for (size_t i=0; i<m_nextStateNodes.size(); i++)
	{
		if (m_nextNeedEnter[i])
		{
			// test workaround
		//	if (m_firstPersonMode)
		//		m_basicState.params[0].m_nFlags &= ~CA_RETAIN_PELVIS_POS;
			// ~test workaround
			if (logTransition)
				CryLogAlways("[ag] EnterState '%s' call node '%s'", m_pGraph->m_states[m_nextStateID].id.c_str(), m_nextStateNodes[i]->GetFactory()->GetName());
			if (m_nextStateNodes[i])
				m_nextStateNodes[i]->EnterState( m_basicState, fromRollback );
			else
				CryLogAlways("Warning: should have entered state node, but it was NULL");
		}
		else
		{
			if (logTransition)
				CryLogAlways("[ag] DontEnterState '%s'->'%s' node '%s'", m_pGraph->m_states[m_currentStateID].id.c_str(), m_pGraph->m_states[m_nextStateID].id.c_str(), m_nextStateNodes[i]->GetFactory()->GetName());
		}
	}
	OnEnterState(m_nextStateID);

	// update our internal "current" state
	m_currentStateNodes = m_nextStateNodes;
	m_currentStateNodeFactories = m_nextStateNodeFactories;
	CRY_ASSERT(m_nextStateID != INVALID_STATE);
//	m_currentStateID = m_nextStateID;
	m_stateStartTime = m_pTimer->GetFrameStartTime();
	UpdateTransitionTimes();
	RefreshUpdateStates();
	InvalidateStateNodeCache(false);

	m_resetLock = oldLock;
}

const char * CAnimationGraphState::GetCurrentStateName()
{
	if (m_currentStateID == INVALID_STATE)
		return "<no state>";
	else
		return m_pGraph->m_states[m_currentStateID].id.c_str();
}

void CAnimationGraphState::UpdateTransitionTimes()
{
	// figure out any time-based constraints on leaving this state
	m_nextAllowedTransition = 0.0f;
	m_stickyTransitionCompletion = 0.0f;

	for (size_t i=0; i<m_currentStateNodes.size(); i++)
	{
		IAnimationStateNode * pNode = m_currentStateNodes[i];
		if (pNode)
		{
			//CTimeValue hardFinish; //Diesel cut
			//CTimeValue stickyFinish; //Diesel cut
			//pNode->GetCompletionTimes( m_basicState, m_stateStartTime, hardFinish, stickyFinish ); //Diesel cut
			//m_nextAllowedTransition = max( m_nextAllowedTransition, hardFinish ); //Diesel cut
			//m_stickyTransitionCompletion = max( m_stickyTransitionCompletion, stickyFinish ); //Diesel cut
		}
	}
}

class CAnimationGraphState::CStateNameMapper
{
public:
	typedef string ValueType;

	explicit CStateNameMapper( _smart_ptr<CAnimationGraph> pGraph ) : m_pGraph(pGraph)
	{
	}

	string KeyToValue( StateID id ) const
	{
		if (id == INVALID_STATE)
			return "!!invalidstate";
		else
			return m_pGraph->StateIDToName(id).c_str();
	}
	StateID ValueToKey( const string& name ) const
	{
		return m_pGraph->StateNameToID(name.c_str());
	}

private:
	_smart_ptr<CAnimationGraph> m_pGraph;
};

class CAnimationGraphState::CUpdateStateNameMapper
{
public:
	typedef string ValueType;

	string KeyToValue( CAnimationGraphState::StateUpdateFunction id ) const
	{
		if (id == &CAnimationGraphState::Update_EphemeralState)
			return "ephemeral";
		else if (id == &CAnimationGraphState::Update_InitialState)
			return "initial";
		else if (id == &CAnimationGraphState::Update_SteadyState)
			return "steady";
		else if (id == &CAnimationGraphState::Update_TransitionState)
			return "transition";
		else
			return "unknown_default_to_initial";
	}
	CAnimationGraphState::StateUpdateFunction ValueToKey( const string& name ) const
	{
		if (name == "ephemeral")
			return &CAnimationGraphState::Update_EphemeralState;
		else if (name == "initial")
			return &CAnimationGraphState::Update_InitialState;
		else if (name == "steady")
			return &CAnimationGraphState::Update_SteadyState;
		else if (name == "transition")
			return &CAnimationGraphState::Update_TransitionState;
		else
			return &CAnimationGraphState::Update_InitialState;
	}
};

void CAnimationGraphState::Serialize(TSerialize ser)
{
	if (ser.GetSerializationTarget() == eST_SaveGame)
	{
		CStateNameMapper stateNameMapper(m_pGraph);
		CUpdateStateNameMapper updateStateNameMapper;

		if (ser.IsReading())
			DoReset(true);
		ser.BeginGroup("AnimationGraphState");
		ser.MappedValue("currentState", m_currentStateID, stateNameMapper);
		ser.MappedValue("queriedState", m_queriedStateID, stateNameMapper);
		ser.MappedValue("nextState", m_nextStateID, stateNameMapper);
		ser.Value("nextAllowedTransition", m_nextAllowedTransition);
		ser.Value("stickTransitionCompletion", m_stickyTransitionCompletion);
		ser.Value("stateStartTime", m_stateStartTime);
		ser.MappedValue("state", m_state, updateStateNameMapper);
		ser.MappedValue("activeTransition", m_activeTransition, stateNameMapper);
		ser.Value("token", m_token);
		ser.Value("stateWasEntered", m_currentStateWasEntered);
		ser.EndGroup();

		if (ser.IsReading() && m_state != &CAnimationGraphState::Update_InitialState)
		{
			m_lastEnteredState = m_currentStateWasEntered == true ? m_currentStateID : StateID(INVALID_STATE);
			if ( m_state == &CAnimationGraphState::Update_TransitionState )
			{
				BuildStateFactories( m_nextStateID, m_currentStateNodeFactories );
				BuildStatesFromFactories( m_nextStateID, m_currentStateNodeFactories, m_currentStateNodes, true );
			}
			else
			{
				BuildStateFactories( m_currentStateID, m_currentStateNodeFactories );
				BuildStatesFromFactories( m_currentStateID, m_currentStateNodeFactories, m_currentStateNodes, true );
				for ( size_t i = 0; i < m_currentStateNodes.size(); ++i )
				{
					if ( IAnimationStateNode* pNode = m_currentStateNodes[i] )
					{
						if ( m_currentStateWasEntered )
							pNode->EnteredState( m_basicState );
					}
				}
			}
			RefreshUpdateStates();
		}

		for(int i = 0; i < m_pGraph->m_inputValues.size(); ++i)
		{
			ser.BeginGroup("AnimGraphInput");
			m_pGraph->m_inputValues[i]->Serialize(ser, 
				&m_inputValueCache[m_inputValueCacheIndex][i], 
				&m_inputValuesAsFloats[i]);
			ser.EndGroup();
		}

		ser.Value("m_outputs", m_outputs);

		ser.Value("m_variationInputValues", m_variationInputValues);
		if (ser.IsReading())
			m_variationInputValues.resize( m_pGraph->m_variationInputIDs.size() );
	}
}

IAnimationGraphPtr CAnimationGraphState::ChangeGraph( const char *graph )
{
	// graph wants to be changed - try to do so
	IAnimationGraphPtr pNewGraph = CCryAction::GetCryAction()->GetAnimationGraphManager()->LoadGraph( graph );
	if (!pNewGraph)
	{
		GameWarning("Unable to load graph %s; animation graph not changed", graph);
		return &*m_pGraph;
	}
	m_pGraph = (CAnimationGraph *)&(*pNewGraph);
	m_curSerial = m_pGraph->m_serial - 1; // force the serial number to be wrong... to force a reset
	return &*m_pGraph;
}

uint32 CAnimationGraphState::GetCurrentToken()
{
	return m_token;
}

void CAnimationGraphState::QueryChangeInput( InputID id, TAnimationGraphQueryID * pQuery )
{
	CRY_ASSERT(pQuery);
	*pQuery = 0;
	if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pGraph->m_inputValues.size())
		return;
	m_queryChangedInputIDs[id] = *pQuery = m_nextQueryID++;
}

void CAnimationGraphState::SetInput( InputID id, CStateIndex::InputValue inval, float fval, TAnimationGraphQueryID * pQuery )
{
	CHECKQNAN_FLT(fval);

	if (id == InputID(-1))
		return;

	if (CAnimationGraphCVars::Get().m_breakMode == 2)
		return;

	m_queryConsideredInputs &= ~(1u<<id);

	TAnimationGraphQueryID queryID = 0;
	if (pQuery /* && inval != INVALID_STATE */)
	{
		queryID = *pQuery = m_nextQueryID++;
		CRY_ASSERT(queryID);
	}

	if (inval == CStateIndex::INPUT_VALUE_DONT_CARE)
	{
		inval = m_pGraph->m_inputValues[id]->defaultValue;
	}

	bool changed = false;
	if (inval != CStateIndex::INPUT_VALUE_DONT_CARE && 
		m_pGraph->m_inputValues[id]->signalled && 
		m_pGraph->m_stateIndex.StateMatchesInput( m_currentStateID, id, inval, 0 ))
	{
		// if we already match the signal, don't set it again
		// (but inform the requester that there's no need to wait for it)
		if ( queryID != 0 )
		{
			AG_SEND_EVENT(QueryComplete(queryID, true));
			queryID = 0;
			pQuery = 0;
		}
	}
	else if (m_inputValueCache[m_inputValueCacheIndex][id] != inval)
	{
		m_inputValueCache[m_inputValueCacheIndex][id] = inval;
		changed = true;
	}
	if (changed && m_pExactPositioning.get())
		m_pExactPositioning->InvalidatePositions();
	m_inputValuesAsFloats[id] = fval;
	if (changed && m_inputQueryIDs[id])
	{
		TAnimationGraphQueryID temp = m_inputQueryIDs[id];
		m_inputQueryIDs[id] = 0;
		AG_SEND_EVENT(QueryComplete(temp, false));
	}
	if (changed && m_queryChangedInputIDs[id])
	{
		TAnimationGraphQueryID temp = m_queryChangedInputIDs[id];
		m_queryChangedInputIDs[id] = 0;
		AG_SEND_EVENT(QueryComplete(temp, true));
	}
	
	if (!changed && m_inputQueryIDs[id])
	{
		// don't change the m_inputQueryIDs[id]! something is waiting for it.
		// instead change *pQuery since they are both waiting for the same thing
		if (pQuery)
			*pQuery = m_inputQueryIDs[id];
	}
	else if (queryID && !m_pGraph->m_inputValues[id]->signalled && 
		m_pGraph->m_stateIndex.StateMatchesInput( m_currentStateID, id, inval, eSMIF_ConsiderMatchesAny ))
	{
		AG_SEND_EVENT(QueryComplete(queryID, true));
	}
	else
	{
		m_inputQueryIDs[id] = queryID;
	}
}

void CAnimationGraphState::SetInputValue( CStateIndex::InputID id, CStateIndex::InputValue encValue, float fltValue )
{
	SetInput(id, encValue, fltValue, NULL);
}

bool CAnimationGraphState::SetInput( InputID id, float val, TAnimationGraphQueryID * pQuery )
{
	CHECKQNAN_FLT(val);

	if (pQuery)
		*pQuery = 0;
	if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pGraph->m_inputValues.size())
		return false;
	if (m_inputLocks & (1<<id))
		return false;
	CStateIndex::InputValue encValue = m_pGraph->m_inputValues[id]->EncodeInput( val );
	if ( encValue == CStateIndex::INPUT_VALUE_DONT_CARE )
	{
		SetInput( id, encValue, val, NULL );
		return false;
	}
	SetInput( id, encValue, val, pQuery );
	return encValue != CStateIndex::INPUT_VALUE_DONT_CARE;
}

bool CAnimationGraphState::SetInput( InputID id, int val, TAnimationGraphQueryID * pQuery )
{
	if (pQuery)
		*pQuery = 0;
	if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pGraph->m_inputValues.size())
		return false;
	if (m_inputLocks & (1<<id))
		return false;
	CStateIndex::InputValue encValue = m_pGraph->m_inputValues[id]->EncodeInput( val );
	if ( encValue == CStateIndex::INPUT_VALUE_DONT_CARE )
	{
		SetInput( id, encValue, val, NULL );
		return false;
	}
	SetInput( id, encValue, val, pQuery );
	return encValue != CStateIndex::INPUT_VALUE_DONT_CARE;
}

bool CAnimationGraphState::SetInput( InputID id, const char * val, TAnimationGraphQueryID * pQuery )
{
	if (pQuery)
		*pQuery = 0;
	if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pGraph->m_inputValues.size())
		return false;
	if (m_inputLocks & (1<<id))
		return false;
	CStateIndex::InputValue encValue = m_pGraph->m_inputValues[id]->EncodeInput( val );
	if ( encValue == CStateIndex::INPUT_VALUE_DONT_CARE )
	{
		SetInput( id, encValue, 0.0f, NULL );
		return false;
	}
	SetInput( id, encValue, 0.0f, pQuery );
	return encValue != CStateIndex::INPUT_VALUE_DONT_CARE;
}

bool CAnimationGraphState::SetInputOptional( InputID id, const char * val, TAnimationGraphQueryID * pQuery )
{
	if (pQuery)
		*pQuery = 0;
	if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pGraph->m_inputValues.size())
		return false;
	if (m_inputLocks & (1<<id))
		return false;
	CStateIndex::InputValue encValue = m_pGraph->m_inputValues[id]->EncodeInput( val );
	if ( encValue == CStateIndex::INPUT_VALUE_DONT_CARE )
	{
		return false;
	}
	SetInput( id, encValue, 0.0f, pQuery );
	return encValue != CStateIndex::INPUT_VALUE_DONT_CARE;
}

void CAnimationGraphState::ClearInput( InputID id )
{
	if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pGraph->m_inputValues.size())
		return;
	if (m_inputLocks & (1u<<id))
		return;
	m_inputValueCache[m_inputValueCacheIndex][id] = CStateIndex::INPUT_VALUE_DONT_CARE;
	m_inputValuesAsFloats[id] = 0.0f;
}

void CAnimationGraphState::LockInput( InputID id, bool lock )
{
	if (id >= CAnimationGraph::MAX_INPUTS)
		return;
	if (lock)
		m_inputLocks |= (1<<id);
	else
		m_inputLocks &= ~(1<<id);
}

void CAnimationGraphState::GetInput( InputID id, char * val ) const
{
	if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pGraph->m_inputValues.size())
	{
		*val = 0;
		return;
	}
	CStateIndex::InputValue inputValue = m_inputValueCache[m_inputValueCacheIndex][id];
	m_pGraph->m_inputValues[id]->DecodeInput( val, inputValue );
}

void CAnimationGraphState::GetInput( InputID id, char * val, int layerIndex ) const
{
	if (layerIndex != m_layerIndex)
	{
		*val = 0;
		return;
	}
	GetInput(id, val);
}

bool CAnimationGraphState::IsDefaultInputValue( InputID id ) const
{
	if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pGraph->m_inputValues.size())
		return true;
	CStateIndex::InputValue defaultValue = m_pGraph->m_inputValues[id]->defaultValue;
	CStateIndex::InputValue inputValue = m_inputValueCache[m_inputValueCacheIndex][id];
	return defaultValue == inputValue;
}

bool CAnimationGraphState::IsMixingAllowedForCurrentState() const
{
	if (m_currentStateID == INVALID_STATE)
		return false;
	return m_pGraph->m_states[m_currentStateID].canMix;
}

bool CAnimationGraphState::DoesParentLayerAllowMixing() const
{
	return m_pParentLayerState ? m_pParentLayerState->IsMixingAllowedForCurrentState() : true;
}

void CAnimationGraphState::UpdateSignalling()
{
	if (m_currentStateID == INVALID_STATE)
		return;
	// only selectable states cause querycompletes/signalling changes
	if (!m_pGraph->m_states[m_currentStateID].allowSelect)
		return;

	for (InputID id = 0; id < m_pGraph->m_inputValues.size(); id++)
	{
		if (m_inputValueCache[m_inputValueCacheIndex][id] != CStateIndex::INPUT_VALUE_DONT_CARE)
		{
			bool needToCheck = true;
			bool stateMatchesInput = false;
			// this is a little bit dirty to avoid double checking whether a state matches a particular input (as that can be slow)
			// note that we set needToCheck to false and pass a fals parameter as the last parameter to StateMatchesInput, and
			// cache the value in stateMatchesInput
      // note that the false that needToSet is set to is 0, which is exactly the flags we need to pass
#define AG_CHECK_MATCH() (needToCheck? stateMatchesInput = m_pGraph->m_stateIndex.StateMatchesInput( m_currentStateID, id, m_inputValueCache[m_inputValueCacheIndex][id], needToCheck = false ) : stateMatchesInput)
			if (m_inputQueryIDs[id])
			{
				if (AG_CHECK_MATCH())
				{
					AG_SEND_EVENT(QueryComplete(m_inputQueryIDs[id], true));
					m_inputQueryIDs[id] = 0;
				}
				else
				{
					if (m_queryConsideredInputs & (1u<<id))
					{
						if (!m_pGraph->m_stateIndex.StateMatchesInput(m_queriedStateID, id, m_inputValueCache[m_inputValueCacheIndex][id], eSMIF_EnforceMatchesInput))
						{
							AG_SEND_EVENT(QueryComplete(m_inputQueryIDs[id], false));
							m_inputQueryIDs[id] = 0;
						}
					}
				}
			}
			if (m_inputValueCache[m_inputValueCacheIndex][id] != m_pGraph->m_inputValues[id]->defaultValue && m_pGraph->m_inputValues[id]->signalled)
			{
				if (AG_CHECK_MATCH())
					m_inputValueCache[m_inputValueCacheIndex][id] = m_pGraph->m_inputValues[id]->defaultValue;
			}
#undef AG_CHECK_MATCH
		}
	}
}

void CAnimationGraphState::ForceLeaveCurrentState()
{
	if (m_currentStateID == INVALID_STATE)
		return;

	if (m_state != &CAnimationGraphState::Update_SteadyState)
		return;
	
	const CAnimationGraph::SStateInfo& info = m_pGraph->m_states[m_currentStateID];

	if (!info.allowSelect)
		return;

	// we just choose a random link to follow (hopefully there's only one...)
	StateID forceID = INVALID_STATE;

	CStateIndex::StateIDVec intersectionLinkedValidStates;

	int inputFilteredStateCount = m_validStates.size();
	for (CAnimationGraph::LinkInfoVec::const_iterator link = m_pGraph->m_links.begin() + info.linkOffset; 
		(link != m_pGraph->m_links.end()) && (link->from == m_currentStateID); 
		++link)
	{
		for (int i = 0; i < inputFilteredStateCount; ++i)
		{
			if (m_validStates[i] == link->to)
				intersectionLinkedValidStates.push_back(m_validStates[i]);
		}
	}

	if (intersectionLinkedValidStates.empty())
	{
		GameWarning("[AnimationGraph] Forced to leave current state '%s', but no linked state is valid for current inputs.", m_pGraph->m_states[m_currentStateID].id.c_str());
	}
	else
	{
		// TODO: Here we should select a random state or the nullest state (sum of factory lengths, including all parents).
		forceID = intersectionLinkedValidStates[0];
	}

	if (forceID != INVALID_STATE)
	{
		if (m_forcedStates.empty() || m_forcedStates.front().stateID != forceID)
			m_forcedStates.push_front(forceID);
	}
}

void CAnimationGraphState::InvalidateQueriedState()
{
	m_queryInvalidated = true;
}

void CAnimationGraphState::SetOutput( int id )
{
	std::pair<string, string> output = m_pGraph->m_outputs[id];
	m_outputs[output.first] = output.second;
	AG_SEND_EVENT(SetOutput(output.first.c_str(), output.second.c_str()));
}

void CAnimationGraphState::ClearOutput( int id )
{
	std::pair<string, string> output = m_pGraph->m_outputs[id];
	m_outputs[output.first] = string();
	AG_SEND_EVENT(SetOutput(output.first.c_str(), ""));
}

const char * CAnimationGraphState::QueryOutput( const char * name )
{
	std::map<string, string>::const_iterator iter = m_outputs.find(CONST_TEMP_STRING(name));
	if (iter == m_outputs.end())
		return NULL;
	else
		return iter->second.c_str();
}

TimeAlignmentMask CAnimationGraphState::GetCurrentTimeAlignmentMask() const
{
	return m_activeTimeAlignmentMask;
}

void CAnimationGraphState::SetCurrentTimeAlignmentMask( TimeAlignmentMask timeAlignmentMask )
{
	m_activeTimeAlignmentMask = timeAlignmentMask;
}

void CAnimationGraphState::AddListener( const char * name, IAnimationGraphStateListener * pListener )
{
	SListener listener;
	strncpy(listener.name, name, sizeof(listener.name));
	listener.name[sizeof(listener.name)-1] = 0;
	listener.pListener = pListener;
	stl::push_back_unique( m_listeners, listener );
}

void CAnimationGraphState::RemoveListener( IAnimationGraphStateListener * pListener )
{
	SListener listener;
	listener.name[0] = 0;
	listener.pListener = pListener;
	stl::find_and_erase( m_listeners, listener );
}

void CAnimationGraphState::UpdateBlendWeights( ICharacterInstance * pCharacter )
{
	// compatibility with existing code
	if (m_pGraph->m_blendWeightInputValues.empty())
		return;

	// but the rest of this code should be agnostic to that
	static const size_t MaxWeights = sizeof(m_blendSpaceWeights) / sizeof(float);
	size_t numWeights = min(MaxWeights, m_pGraph->m_blendWeightInputValues.size());

	//one day, this will be a global solution
	f32 AvrgFrameTime = pCharacter->GetAverageFrameTime();

	for (int i=0; i<MaxWeights; i++)
	{
		if (numWeights > i)
		{
			CAnimationGraph::IInputValue* pInputValue = m_pGraph->m_blendWeightInputValues[i];
			if (pInputValue)
				m_blendSpaceWeights[i] = clamp(m_inputValuesAsFloats[pInputValue->id], -1.0f, 1.0f);
			else
				m_blendSpaceWeights[i] = 0;
		}
		else
		{
			m_blendSpaceWeights[i] = 0;
		}
	}


	ISkeletonAnim* pISkeletonAnim = pCharacter->GetISkeletonAnim();
//	pISkeleton->SetLayerUpdateMultiplier(0,1);	//no scaling
//	pISkeleton->SetDesiredSpeedMSec(0,0,0);		//disable desired speed
  
	/*
	IRenderer* pIRenderer = gEnv->pRenderer;
	IRenderAuxGeom*	g_pAuxGeom = pIRenderer->GetIRenderAuxGeom();
		extern f32 g_YLine2;
	float fColor2[4] = {1,0,0,1};
	pIRenderer->Draw2dLabel( 1,g_YLine2, 1.3f, fColor2, false,"m_blendSpaceWeights: %f %f",m_blendSpaceWeights[1],m_blendSpaceWeights[2] );	
	g_YLine2+=16.0f;
*/

  //strafing
//  if (m_blendSpaceWeightFlags & eBSP_SetBlendSpaceControl0)
//    pISkeleton->SetBlendSpaceControl0( 0, AnimParams(m_blendSpaceWeights[0],   m_blendSpaceWeights[1],m_blendSpaceWeights[2],   -m_blendSpaceWeights[1], m_blendSpaceWeights[3]) );

	/*
  //interpolation strafing-leaning
  if (m_blendSpaceWeightFlags & eBSP_SetIWeights)
    pISkeleton->SetIWeight( 0, m_blendSpaceWeights[8] );
  //run+leaning
  if (m_blendSpaceWeightFlags & eBSP_SetBlendSpaceControl1)
    pISkeleton->SetBlendSpaceControl1( 0, AnimParams(m_blendSpaceWeights[4],m_blendSpaceWeights[5],m_blendSpaceWeights[6],      -m_blendSpaceWeights[5], m_blendSpaceWeights[7]) );  
*/

	//interpolation strafing-leaning
	//run+leaning
//	if (m_blendSpaceWeightFlags & eBSP_SetBlendSpaceControl1)
//		pISkeleton->SetBlendSpaceControl1( 0, AnimParams(0,0,0,0,0) );  

}

Vec2 CAnimationGraphState::GetQueriedStateMinMaxSpeed()
{
	if (m_basicState.pEntity && m_queriedStateID != INVALID_STATE)
		return m_pGraph->GetStateMinMaxSpeed( m_basicState.pEntity, m_queriedStateID );
	return Vec2(0,0);
}

ETriState CAnimationGraphState::BasicPathfind(StateID fromStateID, CAnimationGraph::SPathFindParams& params)
{
	m_lastPathFind = m_pTimer->GetFrameStartTime();
	if (params.pStats == NULL)
		params.pStats = &m_pathFindStats;
	return m_pGraph->PathFindBetweenStates(fromStateID, params);
}

/*
 * existence queries
 */

class CAnimationGraphExistanceQuery : public IAnimationGraphExistanceQuery
{
public:
	//------------------  IAnimationGraphExistanceQuery
	CAnimationGraphExistanceQuery( CAnimationGraphState * pState ) : m_pState(pState), m_checkMatchingInputs(0)
	{
		InitInputs();
	}

	void SetInput( InputID id, int value )
	{
		if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pState->m_pGraph->m_inputValues.size())
			return;
		m_inputs[id] = m_pState->m_pGraph->m_inputValues[id]->EncodeInput( value );
		m_checkMatchingInputs |= (1 << id);
	}
	void SetInput( InputID id, const char * value )
	{
		if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pState->m_pGraph->m_inputValues.size())
			return;
		m_inputs[id] = m_pState->m_pGraph->m_inputValues[id]->EncodeInput( value );
		m_checkMatchingInputs |= (1 << id);
	}
	void SetInput( InputID id, float value )
	{
		if (id >= CAnimationGraph::MAX_INPUTS || id >= m_pState->m_pGraph->m_inputValues.size())
			return;
		m_inputs[id] = m_pState->m_pGraph->m_inputValues[id]->EncodeInput( value );
		m_checkMatchingInputs |= (1 << id);
	}

	bool Complete()
	{
		CStateIndex::StateIDVec results;
		m_pState->m_lastQuery = m_pState->m_pTimer->GetFrameStartTime();
		m_pState->m_pGraph->m_stateIndex.Query( m_inputs, results, m_pState->m_stateIndexQueryStats );

		for (size_t i=0; i<results.size(); i++)
		{
			bool ok = true;
			for (int j=0; j<m_pState->m_pGraph->m_numInputIDs; j++)
			{
				if ((m_checkMatchingInputs & (1u<<j)) == 0)
					continue;
				if (!m_pState->m_pGraph->m_stateIndex.StateMatchesInput(results[i], j, m_inputs[j], eSMIF_EnforceMatchesInput))
					ok = false;
			}
			if (ok)
			{
				m_animationLength = m_pState->m_pGraph->m_states[results[i]].animDesc.movement.duration;
				return true;
			}
		}

		return false;
	}

	CTimeValue GetAnimationLength() const
	{
		return m_animationLength;
	}

	void Release() 
	{ 
		delete this; 
	}

	void Reset()
	{
		InitInputs();
	}

	IAnimationGraphState * GetState()
	{
	return m_pState;
	}
	//------------------  ~IAnimationGraphExistanceQuery

private:
	void InitInputs()
	{
		for (int i=0; i<CAnimationGraph::MAX_INPUTS; ++i)
			m_inputs[i] = m_pState->m_inputValueCache[m_pState->m_inputValueCacheIndex][i];

		m_checkMatchingInputs = 0;
	}

	CAnimationGraphState * m_pState;
	CStateIndex::InputValue m_inputs[CAnimationGraph::MAX_INPUTS];
	uint32 m_checkMatchingInputs;
	CTimeValue m_animationLength;
};

IAnimationGraphExistanceQuery * CAnimationGraphState::CreateExistanceQuery()
{
	return new CAnimationGraphExistanceQuery(this);
}

IAnimationGraphExistanceQuery * CAnimationGraphState::CreateExistanceQuery(int layerIndex)
{
	if (layerIndex == m_layerIndex)
		return CreateExistanceQuery();
	else
		return NULL;
}

void CAnimationGraphState::BasicQuery( const CStateIndex::InputID * inputs, CStateIndex::StateIDVec& validStates )
{
	ANIM_PROFILE_FUNCTION;

	m_lastQuery = m_pTimer->GetFrameStartTime();
	m_pGraph->m_stateIndex.Query( inputs, validStates, m_stateIndexQueryStats );

	// if in first person remove selectable states flagged as 'skip in first person'
	if (m_firstPersonMode)
	{
		for (CStateIndex::StateIDVec::reverse_iterator it = validStates.rbegin(); it != validStates.rend(); ++it)
		{
			if (m_pGraph->m_states[*it].skipFirstPerson)
			{
				*it = validStates.back();
				validStates.pop_back();
			}
		}
	}
}

// must be called after one query and before the next if rankings are needed
int CAnimationGraphState::GetRankings( const CStateIndex::InputValue* query, std::vector<uint16>& rankings )
{
	return m_pGraph->m_stateIndex.GetRankings( query, rankings );
}

void CAnimationGraphState::SetKeepLowValueRankings( bool bKeep )
{
	m_pGraph->m_stateIndex.SetKeepLowValueRankings( bKeep );
}

bool CAnimationGraphState::GetKeepLowValueRankings()
{
	return m_pGraph->m_stateIndex.GetKeepLowValueRankings();
}

bool CAnimationGraphState::IsUpdateReallyNecessary()
{
	// Dejan: This is needed always, for various reasons. If we don't get an update things will break down.
	return true;
/*
	bool necessary = false;
	if (m_pExactPositioning.get() && m_pExactPositioning->IsUpdateReallyNecessary())
		necessary = true;
	return necessary;
*/
}

/*
 * TRIGGERING
 */

IAnimationSpacialTrigger * CAnimationGraphState::SetTrigger( const SAnimationTargetRequest& req, EAnimationGraphTriggerUser user, TAnimationGraphQueryID* pQryStart, TAnimationGraphQueryID* pQryEnd )
{
	if (!m_pExactPositioning.get())
		m_pExactPositioning.reset(new CExactPositioning(this));
	if (m_pExactPositioning->SetTrigger(req, user, pQryStart, pQryEnd))
		return m_pExactPositioning.get();
	else
		return NULL;
}

ETriState CAnimationGraphState::CheckTargetMovement( const SAnimationMovement& movement, float radius, CTargetPointRequest& targetPointRequest)
{
	CRY_ASSERT(m_pExactPositioning.get());
	return m_pExactPositioning->CheckTargetMovement( movement, radius, targetPointRequest );
}

void CAnimationGraphState::ClearTrigger( EAnimationGraphTriggerUser user )
{
	if (m_pExactPositioning.get())
		m_pExactPositioning->ClearTrigger(user);
}

const SAnimationTarget * CAnimationGraphState::GetAnimationTarget()
{
	if (!m_pExactPositioning.get())
		return 0;
	return m_pExactPositioning->GetAnimationTarget();
}

bool CAnimationGraphState::HasAnimationTarget() const
{
	if (!m_pExactPositioning.get())
		return 0;
	return m_pExactPositioning->HasTargetRequest() || m_pExactPositioning->GetAnimationTarget() != NULL;
}

CTimeValue CAnimationGraphState::GetAnimationLength() const
{
	return  m_pGraph->m_states[m_currentStateID].animDesc.movement.duration;
}

void CAnimationGraphState::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->AddContainer(m_validStates);
	s->AddContainer(m_currentStateNodes);
	s->AddContainer(m_currentStateNodeFactories);
	s->AddContainer(m_nextStateNodes);
	s->AddContainer(m_nextStateNodeFactories);
	s->AddContainer(m_updateNodes);
//	s->AddContainer(m_nextNeedEnter);
//	s->AddContainer(m_currentNeedExit);
	s->AddContainer(m_forcedStates);
	s->AddContainer(m_listeners);
	s->AddContainer(m_callingListeners);
	s->AddContainer(m_pendingLeftStates);
	s->AddContainer(m_pendingLeftQueries);
	s->AddContainer(m_pendingLeftStateQueries);

	for (size_t i=0; i<m_currentStateNodes.size(); i++)
		if (m_currentStateNodes[i])
			m_currentStateNodes[i]->GetStateMemoryStatistics(s);
	for (size_t i=0; i<m_nextStateNodes.size(); i++)
		if (m_nextStateNodes[i])
			m_nextStateNodes[i]->GetStateMemoryStatistics(s);
}

/*
 * DEBUG
 */

void CAnimationGraphState::DebugDisplay()
{
	if ( m_layerIndex != m_debugLayer )
		return;

	const char * state = CAnimationGraphCVars::Get().m_pQueue->GetString();
	if (state && state[0])
	{
		PushForcedState(state, NULL);
		CAnimationGraphCVars::Get().m_pQueue->Set("");
	}
	state = CAnimationGraphCVars::Get().m_pSignal->GetString();
	if (state && state[0])
	{
		SetInput(GetInputId("Signal"), state, NULL);
		CAnimationGraphCVars::Get().m_pSignal->Set("");
	}
	state = CAnimationGraphCVars::Get().m_pAction->GetString();
	if (state && state[0])
	{
		LockInput( GetInputId("Action"), false );
		SetInput(GetInputId("Action"), state, NULL);
		LockInput( GetInputId("Action"), true );
//		CAnimationGraphCVars::Get().m_pAction->Set("");
	}
	else
	{
		LockInput( GetInputId("Action"), false );
	}
	state = CAnimationGraphCVars::Get().m_pItem->GetString();
	if (state && state[0])
	{
		LockInput( GetInputId("Item"), false );
		SetInput(GetInputId("Item"), state, NULL);
		LockInput( GetInputId("Item"), true );
		//		CAnimationGraphCVars::Get().m_pAction->Set("");
	}
	else
	{
		LockInput( GetInputId("Item"), false );
	}
	state = CAnimationGraphCVars::Get().m_pStance->GetString();
	if (state && state[0])
	{
		LockInput( GetInputId("Stance"), false );
		SetInput(GetInputId("Stance"), state, NULL);
		LockInput( GetInputId("Stance"), true );
		//		CAnimationGraphCVars::Get().m_pAction->Set("");
	}
	else
	{
		LockInput( GetInputId("Stance"), false );
	}
	if (m_testPlanner)
	{
//		IAnimationSpacialTrigger * pTrigger = SetTrigger( Vec3(144,149,106), Quat::CreateIdentity(), 5.0f, 0.25f );
//		pTrigger->SetInput( "Signal", "callReinforcement" );
//		pTrigger->SetInput("Stance", "prone");
//		pTrigger->SetInput( "Action", "none" );

/*
		AnimationPlan_Begin();
//		AnimationPlan_AddAnimationAtPoint( stl::find_in_map(m_pGraph->m_stateNameToID, "combat_callReinforcements_nw_01", INVALID_STATE), Vec3(144,149,106), 2.0f );
		AnimationPlan_AddAnimationAtTime( stl::find_in_map(m_pGraph->m_stateNameToID, "combat_callReinforcements_nw_01", INVALID_STATE), m_pTimer->GetFrameStartTime() + 5.0f );
		AnimationPlan_End();
*/
		m_testPlanner = false;
	}

	IVisualLog *pVLog = gEnv->pVisualLog;
	CRY_ASSERT( pVLog != NULL );
	ColorF cWhite = ColorF(1,1,1,1);
	ColorF cRed = ColorF(1,0,0,1);
	ColorF cYellow = ColorF(1,1,0,1);
	ColorF cGreen = ColorF(0,1,0,1);
	ColorF cWhiteHalf = ColorF(1,1,1,0.5f);

	IRenderer * pRend = gEnv->pRenderer;
	IRenderAuxGeom * pAux = pRend->GetIRenderAuxGeom();
	float white[4] = {1,1,1,1};
	float red[4] = {1,0,0,1};
	float yellow[4] = {1,1,0,1};
	float green[4] = {0,1,0,1};
	static const size_t BUFSZ = 512;
	char buf[BUFSZ];

	int y = 200, x = 20;
	static const int YSPACE = 12;
	static const int XSPACE = 220;
	const char* szCurrentState = (m_currentStateID != INVALID_STATE) ? m_pGraph->m_states[m_currentStateID].id.c_str() : "Invalid State";
	if (m_state == &CAnimationGraphState::Update_InitialState)
		sprintf(buf, "initial state");
	else if (m_state == &CAnimationGraphState::Update_SteadyState)
		sprintf(buf, "steady state %s", szCurrentState);
	else if (m_state == &CAnimationGraphState::Update_EphemeralState)
		sprintf(buf, "ephemeral state %s", szCurrentState);
	else if (m_state == &CAnimationGraphState::Update_TransitionState)
	{
		const char* szNextState = (m_nextStateID != INVALID_STATE) ? m_pGraph->m_states[m_nextStateID].id.c_str() : "Invalid State";
		sprintf(buf, "transitioning states %s -> %s", szCurrentState, szNextState);
	}
	else
		sprintf(buf, "<<unknown state>>");
	pRend->Draw2dLabel( x, y, 2, white, false, buf );
	pVLog->Log( SVisualLogParams( cWhite, 2, 1, true ), buf );
	y += YSPACE*2;

	if (m_queriedStateID != INVALID_STATE)
		sprintf(buf, "queried %s", m_pGraph->m_states[m_queriedStateID].id.c_str());
	else
		strcpy(buf, "queried <<no state>>");
	if (m_debugBreak)
	{
		pRend->Draw2dLabel( x, y, 2, red, false, "BREAK: %s", buf );
		pVLog->Log( SVisualLogParams( cRed, 2, 1, false ), "BREAK: %s", buf );
	}
	else
	{
		pRend->Draw2dLabel( x, y, 2, white, false, buf );
		pVLog->Log( SVisualLogParams( cWhite, 2, 1, false ), buf );
	}
	y += YSPACE*2;

	pRend->Draw2dLabel( x, y, 2, white, false, buf );
	pVLog->Log( SVisualLogParams( cWhite, 2, 1, false ), buf );
	y += YSPACE*2;

	int mainTopY = y;

	pRend->Draw2dLabel( x, y, 1, white, false, "Anim. time align mask: %d", m_activeTimeAlignmentMask );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, true ), "Anim. time align: %d", m_activeTimeAlignmentMask );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Current Random Number: %d", m_randomNumber );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Current Random Number: %d", m_randomNumber );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "No physical collider: %d [%d]", m_noPhysicalCollider.Enabled()? 1 : 0, m_noPhysicalCollider.Debug() );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "No physical collider: %d [%d]", m_noPhysicalCollider.Enabled()? 1 : 0, m_noPhysicalCollider.Debug() );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Current Token: %d", m_token );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Current Token: %d", m_token );
	y += YSPACE;
	//pRend->Draw2dLabel( x, y, 1, white, false, "Hurried: %s (%f)", m_basicState.hurried? "YES" : "NO", m_hurry );
	pRend->Draw2dLabel( x, y, 1, white, false, "Hurried: %s", m_basicState.hurried? "YES" : "NO" );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Hurried: %s", m_basicState.hurried? "YES" : "NO" );
	y += YSPACE;
	if (m_basicState.pEntity)
	{
		if (ICharacterInstance * pChar = m_basicState.pEntity->GetCharacter(0))
		{
			pRend->Draw2dLabel( x, y, 1, white, false, "Hor. Movement: %s", g_szMCMString[(int)(pChar->GetISkeletonAnim()->GetUserData(eAGUD_MovementControlMethodH))] );
			pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Hor. Movement: %s", g_szMCMString[(int)(pChar->GetISkeletonAnim()->GetUserData(eAGUD_MovementControlMethodH))] );
			y += YSPACE;
			pRend->Draw2dLabel( x, y, 1, white, false, "Ver. Movement: %s", g_szMCMString[(int)(pChar->GetISkeletonAnim()->GetUserData(eAGUD_MovementControlMethodV))] );
			pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Ver. Movement: %s", g_szMCMString[(int)(pChar->GetISkeletonAnim()->GetUserData(eAGUD_MovementControlMethodV))] );
			y += YSPACE;
			pRend->Draw2dLabel( x, y, 1, white, false, "+ turn mul: %f", pChar->GetISkeletonAnim()->GetUserData(eAGUD_AdditionalTurnMultiplier) );
			pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "+ turn mul: %f", pChar->GetISkeletonAnim()->GetUserData(eAGUD_AdditionalTurnMultiplier) );
			y += YSPACE;
		}
	}
	pRend->Draw2dLabel( x, y, 1, white, false, "Pending left states: %d", m_pendingLeftStates.size() );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Pending left states: %d", m_pendingLeftStates.size() );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Pause state: [%s] %.8x", m_basicState.isPaused? "PAUSED" : "RUNNING", m_pauseState );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Pause state: [%s] %.8x", m_basicState.isPaused? "PAUSED" : "RUNNING", m_pauseState );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Input Locks: %.8x", m_inputLocks );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Input Locks: %.8x", m_inputLocks );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Num sub-updates per-frame: %d", m_updateNodes.size() );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Num sub-updates per-frame: %d", m_updateNodes.size() );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Next allowed transition: %f", (m_nextAllowedTransition - m_pTimer->GetFrameStartTime()).GetSeconds() );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Next allowed transition: %f", (m_nextAllowedTransition - m_pTimer->GetFrameStartTime()).GetSeconds() );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Sticky state duration: %f", (m_stickyTransitionCompletion - m_pTimer->GetFrameStartTime()).GetSeconds() );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Sticky state duration: %f", (m_stickyTransitionCompletion - m_pTimer->GetFrameStartTime()).GetSeconds() );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Query age: %f", (m_pTimer->GetFrameStartTime() - m_lastQuery).GetSeconds() );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Query age: %f", (m_pTimer->GetFrameStartTime() - m_lastQuery).GetSeconds() );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  num inputs considered: %d", m_stateIndexQueryStats.nInputsConsidered );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  num inputs considered: %d", m_stateIndexQueryStats.nInputsConsidered );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  total queryable states: %d", m_stateIndexQueryStats.nQueryableStates );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  total queryable states: %d", m_stateIndexQueryStats.nQueryableStates );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  intersection values touched: %d", m_stateIndexQueryStats.nIntersectionValuesTouched );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  intersection values touched: %d", m_stateIndexQueryStats.nIntersectionValuesTouched );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  ranking values touched: %d", m_stateIndexQueryStats.nRankingValuesTouched );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  ranking values touched: %d", m_stateIndexQueryStats.nRankingValuesTouched );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  database size: %d", m_stateIndexQueryStats.databaseSize );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  database size: %d", m_stateIndexQueryStats.databaseSize );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  input data size: %d", m_stateIndexQueryStats.inputDataSize );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  input data size: %d", m_stateIndexQueryStats.inputDataSize );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Path find age: %f", (m_pTimer->GetFrameStartTime() - m_lastPathFind).GetSeconds() );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "Path find age: %f", (m_pTimer->GetFrameStartTime() - m_lastPathFind).GetSeconds() );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  maximum cost: %d", m_pathFindStats.maxCost );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  maximum cost: %d", m_pathFindStats.maxCost );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  final cost: %d", m_pathFindStats.finalCost );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  final cost: %d", m_pathFindStats.finalCost );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  largest open queue: %d", m_pathFindStats.largestOpenQueue );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  largest open queue: %d", m_pathFindStats.largestOpenQueue );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  nodes touched: %d", m_pathFindStats.nodesTouched );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  nodes touched: %d", m_pathFindStats.nodesTouched );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  replacements made: %d", m_pathFindStats.replacementsMade );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  replacements made: %d", m_pathFindStats.replacementsMade );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  guard evaluations: %d", m_pathFindStats.guardEvaluationsPerformed );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  guard evaluations: %d", m_pathFindStats.guardEvaluationsPerformed );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  slow guard evals: %d", m_pathFindStats.expensiveGuardEvaluationsPerformed );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  slow guard evals: %d", m_pathFindStats.expensiveGuardEvaluationsPerformed );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  guard fails: %d", m_pathFindStats.guardEvaluationsFailed );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  guard fails: %d", m_pathFindStats.guardEvaluationsFailed );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  links followed: %d", m_pathFindStats.linksFollowed );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  links followed: %d", m_pathFindStats.linksFollowed );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  cumulative links in queue: %d", m_pathFindStats.cumLinksInQueue );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  cumulative links in queue: %d", m_pathFindStats.cumLinksInQueue );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  ai walkability queries: %d", m_pathFindStats.aiWalkabilityQueriesPerformed );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  ai walkability queries: %d", m_pathFindStats.aiWalkabilityQueriesPerformed );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "  animation movement queries: %d", m_pathFindStats.animationMovementQueriesPerformed );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "  animation movement queries: %d", m_pathFindStats.animationMovementQueriesPerformed );
	y += YSPACE;

	for (std::map<string,string>::const_iterator iter = m_outputs.begin(); iter != m_outputs.end(); ++iter)
	{
		pRend->Draw2dLabel( x, y, 1, green, false, "OUTPUT: %s = %s", iter->first.c_str(), iter->second.c_str() );
		pVLog->Log( SVisualLogParams( cGreen, 1, 1, false ), "OUTPUT: %s = %s", iter->first.c_str(), iter->second.c_str() );
		y += YSPACE;
	}

	int mainBottomY = y;
	y = mainTopY;
	x += XSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Current inputs" );
	pVLog->Log( SVisualLogParams( cWhite, 1, 2, false ), "Current inputs" );
	y += YSPACE;

	std::vector< std::pair<uint8, size_t> > orderInputs;
	for (size_t i=0; i<m_pGraph->m_inputValues.size(); i++)
		orderInputs.push_back( std::make_pair( m_pGraph->m_inputValues[i]->priority, i ) );
	std::sort( orderInputs.begin(), orderInputs.end(), std::greater< std::pair<uint8, size_t> >() );

	std::vector<uint16> rankings;
	int numInputsUsed = m_pGraph->m_stateIndex.GetRankings( &m_inputValueCache[m_inputValueCacheIndex][0], rankings );

	for (size_t i=0; i<m_pGraph->m_inputValues.size(); i++)
	{
		size_t id = orderInputs[i].second;

		m_pGraph->m_inputValues[id]->DebugText( buf, m_inputValueCache[m_inputValueCacheIndex][id], m_inputValuesAsFloats );
		int correctness = 0;
		if (m_currentStateID != INVALID_STATE)
			correctness |= (int) m_pGraph->m_stateIndex.StateMatchesInput( m_currentStateID, id, m_inputValueCache[m_inputValueCacheIndex][id], eSMIF_ConsiderMatchesAny );
		if (m_queriedStateID != INVALID_STATE)
			correctness |= 2 * m_pGraph->m_stateIndex.StateMatchesInput( m_queriedStateID, id, m_inputValueCache[m_inputValueCacheIndex][id], eSMIF_ConsiderMatchesAny );

		float * clr;
		ColorF * cClr;
		switch (correctness)
		{
		default:
			clr = red;
			cClr = &cRed;
			break;
		case 1:
			clr = yellow;
			cClr = &cYellow;
			break;
		case 2:
			clr = green;
			cClr = &cGreen;
			break;
		case 3:
			clr = white;
			cClr = &cWhite;
			break;
		}

		if (i>=numInputsUsed)
		{
			clr[3] = 0.5f;
			cClr->a = 0.5f;
		}

		char queryBuffer[512];
		char * pQueryBuffer = queryBuffer;
		pQueryBuffer += sprintf(pQueryBuffer, "  %.20s: [%d] %s", m_pGraph->m_inputValues[id]->name.c_str(), m_inputValueCache[m_inputValueCacheIndex][id], buf);
		if (m_inputQueryIDs[id])
			pQueryBuffer += sprintf(pQueryBuffer, " [query %d]", (int)m_inputQueryIDs);
		if (m_queryChangedInputIDs[id])
			pQueryBuffer += sprintf(pQueryBuffer, " [onchange %d]", m_queryChangedInputIDs[id]);
		pRend->Draw2dLabel( x, y, 1, clr, false, queryBuffer );
		pVLog->Log( SVisualLogParams( *cClr, 1, 2, false ), queryBuffer );
		y += YSPACE;

		clr[3] = 1.0f;
		cClr->a = 1.0f;
	}
	mainBottomY = max(y, mainBottomY);
	y = mainTopY;
	x += XSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "Past states" );
	pVLog->Log( SVisualLogParams( cWhite, 1, 3, false ), "Past states" );
	y += YSPACE;
	for (OldStateQueue::SIterator iter = m_oldStates.Begin(); iter != m_oldStates.End(); ++iter)
	{
		if (iter->cancelled)
		{
			white[3] = 0.5f;
			cWhite.a = 0.5f;
		}
		if (iter->state == INVALID_STATE)
		{
			pRend->Draw2dLabel( x, y, 1, white, false, "INVALID_STATE" );
			pVLog->Log( SVisualLogParams( cWhite, 1, 3, false ), "INVALID_STATE" );
		}
		else
		{
			pRend->Draw2dLabel( x, y, 1, white, false, "%s", m_pGraph->m_states[iter->state].id.c_str() );
			pVLog->Log( SVisualLogParams( cWhite, 1, 3, false ), "%s", m_pGraph->m_states[iter->state].id.c_str() );
		}
		white[3] = 1.0f;
		cWhite.a = 1.0f;
		y += YSPACE;
	}

	mainBottomY = max(y, mainBottomY);
	x += 2*XSPACE;
	y = mainTopY;
	pRend->Draw2dLabel( x, y, 1, white, false, "Node status" );
	pVLog->Log( SVisualLogParams( cWhite, 1, 5, false ), "Node status" );
	y += YSPACE;
	for (size_t i=0; i<m_currentStateNodes.size(); i++)
	{
		if (IAnimationStateNode * pStateNode = m_currentStateNodes[i])
		{
			EHasEnteredState hasEntered = pStateNode->HasEnteredState(m_basicState);
			const char * enteredText = "<error>";
			switch (hasEntered)
			{
			case eHES_Entered:
				enteredText = "entered";
				break;
			case eHES_Instant:
				enteredText = "instant";
				break;
			case eHES_Waiting:
				enteredText = "waiting";
				break;
			}

			const char * canLeave = " [blocking]";
			if (pStateNode->CanLeaveState(m_basicState))
				canLeave = "";

			const char * updated = " [updated]";
			if ((pStateNode->flags & eASNF_Update) == 0)
				updated = "";

			const char * category = pStateNode->GetFactory()->GetCategory();
			const char * name = pStateNode->GetFactory()->GetName();

			pRend->Draw2dLabel(x, y, 1, white, false, "%s.%s[%.8x]: %s%s%s", category, name, (uint32)pStateNode->GetFactory(), enteredText, canLeave, updated);
			pVLog->Log( SVisualLogParams( cWhite, 1, 5, false ), "%s.%s[%.8x]: %s%s%s", category, name, (uint32)pStateNode->GetFactory(), enteredText, canLeave, updated );
			y += YSPACE;

			pStateNode->DebugDraw( m_basicState, pRend, x+10, y, YSPACE );
		}
	}

	if (m_pExactPositioning.get())
		m_pExactPositioning->DebugDraw_Status( pRend, x, y, YSPACE );

	// Dummy 6th column text to make Visual Log output look better
	pVLog->Log( SVisualLogParams( cWhite, 1, 6, false ), " " );

	x -= 4*XSPACE;
	mainBottomY = max(y, mainBottomY);

	y = mainBottomY;
	int stateTopY = y;
	pRend->Draw2dLabel( x, y, 1, white, false, "Available states" );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, true ), "Available states" );
	y += YSPACE;
	pRend->Draw2dLabel( x, y, 1, white, false, "[  ID] [Desc] Name" );
	pVLog->Log( SVisualLogParams( cWhite, 1, 1, true ), "[  ID] [Desc] Name" );
	y += YSPACE;

	for (CStateIndex::StateIDVec::const_iterator iter = m_validStates.begin(); iter != m_validStates.end(); ++iter)
	{
		const char * szName = "<invalid>";
		const char * szAnimdesc = " ?? ";
		if (*iter != INVALID_STATE)
		{
			const SAnimationDesc* animDesc = m_pGraph->GetAnimationDesc(GetEntity(), *iter, this);
			if (animDesc == NULL)
			{
				szAnimdesc = "NONE";
			}
			else
			{
				const SAnimationSelectionProperties* animProps = animDesc->properties;
				if (animProps != NULL)
				{
					szAnimdesc = "FULL";
				}
				else 
				{
					szAnimdesc = "HALF ";
				}
			}

			szName = m_pGraph->m_states[*iter].id.c_str();
		}

		if (*iter == m_debugLastLocomotionState)
		{
			pRend->Draw2dLabel( x, y, 1, green, false, "[%4d] [%s] %s", (uint16)*iter, szAnimdesc, szName );
			pVLog->Log( SVisualLogParams( cGreen, 1, 1, false ), "[%4d] [%s] %s", (uint16)*iter, szAnimdesc, szName );
		}
		else
		{
			pRend->Draw2dLabel( x, y, 1, white, false, "[%4d] [%s] %s", (uint16)*iter, szAnimdesc, szName );
			pVLog->Log( SVisualLogParams( cWhite, 1, 1, false ), "[%4d] [%s] %s", (uint16)*iter, szAnimdesc, szName );
		}
		y += YSPACE;
	}

	int stateBottomY = y;

	x += 2*XSPACE;
	y = stateTopY;
	pRend->Draw2dLabel( x, y, 1, white, false, "Active transition (%d)", m_activeTransition.Size() );
	pVLog->Log( SVisualLogParams( cWhite, 1, 3, false ), "Active transition (%d)", m_activeTransition.Size() );
	y += YSPACE;
	for (StateList::SIterator iter = m_activeTransition.Begin(); iter != m_activeTransition.End(); ++iter)
	{
		const char * id = "<invalid>";
		if (*iter != INVALID_STATE)
			id = m_pGraph->m_states[*iter].id.c_str();
		pRend->Draw2dLabel( x, y, 1, white, false, id );
		pVLog->Log( SVisualLogParams( cWhite, 1, 3, false ), id );
		y += YSPACE;
	}

	x += XSPACE;
	y = stateTopY;
	if (m_pExactPositioning.get())
		m_pExactPositioning->DebugDraw_Transition( pRend, x, y, YSPACE );

	x += XSPACE;
	y = stateTopY;
	pRend->Draw2dLabel( x, y, 1, white, false, "Forced state queue" );
	pVLog->Log( SVisualLogParams( cWhite, 1, 5, false ), "Forced state queue" );
	y += YSPACE;
	for (TForcedStates::const_iterator iter = m_forcedStates.begin(); iter != m_forcedStates.end(); ++iter)
	{
		const char * id = "<invalid>";
		if (iter->stateID != INVALID_STATE)
			id = m_pGraph->m_states[iter->stateID].id.c_str();
		int pos = 0;
		pos += sprintf( buf+pos, "%s", id );
		if (iter->queryID)
		{
			pos += sprintf( buf+pos, "[%d]", iter->queryID );
		}

		pRend->Draw2dLabel( x, y, 1, white, false, "%s", buf );
		pVLog->Log( SVisualLogParams( cWhite, 1, 5, false ), "%s", buf );
		y += YSPACE;
	}
	stateBottomY = max(stateBottomY, y);
}

bool CAnimationGraphState::IsSignalledInput( InputID inputId ) const
{
	CRY_ASSERT( inputId < m_pGraph->m_inputValues.size() );
	CRY_ASSERT( inputId == m_pGraph->m_inputValues[ inputId ]->id );
	return m_pGraph->m_inputValues[ inputId ]->signalled;
}

const char* CAnimationGraphState::GetInputName( InputID inputId ) const
{
	if ( inputId >= m_pGraph->m_inputValues.size() )
		return NULL;
	CRY_ASSERT( inputId == m_pGraph->m_inputValues[ inputId ]->id );
	return m_pGraph->m_inputValues[ inputId ]->name.c_str();
}

const char* CAnimationGraphState::GetVariationInputName( InputID inputId ) const
{
	if ( inputId >= m_pGraph->m_variationInputIDs.size() )
		return NULL;

	for (CAnimationGraph::MapVariationInputIDs::iterator iter = m_pGraph->m_variationInputIDs.begin(); (iter != m_pGraph->m_variationInputIDs.end()); iter++)
	{
		 if (iter->second == inputId)
		 {
			 return iter->first.c_str();
		 }
	}
	
	//--- Should never reach here due to the size validation above, but just in case.
	CRY_ASSERT(0);
	return NULL;
}

bool CAnimationGraphState::SetVariationInput( const char* name, const char* value )
{
	return SetVariationInput(m_pGraph->GetVariationInputID(name), value);
}

bool CAnimationGraphState::SetVariationInput( InputID inputID, const char* value )
{
	if (inputID == InputID(-1))
		return false;

	CRY_ASSERT( inputID < m_variationInputValues.size() );

	m_variationInputValues[inputID] = value;
	return true;
}

string CAnimationGraphState::ExpandVariationInputs( const char* animationName ) const
{
	string result;
	while ( const char* i = strchr(animationName,'%') )
	{
		result.append( animationName, i-animationName );
		char id = i[1] - 128;
		CRY_ASSERT( id >= 0 && id < m_variationInputValues.size() );
		result += m_variationInputValues[id];
		animationName = i + 2;
	}
	result += animationName;
	return result;
}

#include UNIQUE_VIRTUAL_WRAPPER(IAnimationGraphExistanceQuery)