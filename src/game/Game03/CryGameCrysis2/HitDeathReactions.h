/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
Description: 

-------------------------------------------------------------------------
History:
- 6:11:2009	16:35 : Created by David Ramos
*************************************************************************/
#pragma once
#ifndef __HIT_DEATH_REACTIONS_H
#define __HIT_DEATH_REACTIONS_H

#include "HitDeathReactionsDefs.h"
#include <IDeferredRaycastManager.h>

class CActor;
struct HitInfo;
struct ExplosionInfo;
struct EventPhysCollision;

//////////////////////////////////////////////////////////////////////////
class CHitDeathReactions : public IDeferredRaycastReceiver
{
private:
	// Private typedefs
	struct SCustomAnim
	{
		SCustomAnim() : iLayer(0) {}

		void Invalidate() { sAnimName.clear(); }  
		bool IsValid() const { return !sAnimName.empty(); }

		CryCharAnimationParams	animParams;
		string									sAnimName;
		int											iLayer;
	};

	enum EReactionType
	{
		eRT_None = -1,

		eRT_Hit,
		eRT_Death,
	};

	enum EExecutionType
	{
		eET_None = -1,

		eET_ReactionAnim,
		eET_AnimationGraphAnim,

		eET_Custom,
	};

	enum EAIPausedState
	{
		eAIP_NotPaused,
		eAIP_ExecutingCustomSignal,		// Current reaction has sent a custom signal to the AI for behavior processing
		eAIP_PipeUserPaused,			// Pipe user is paused as a result of the current reaction
	};

public:
	// Public methods
	CHitDeathReactions(CActor& actor);

	void Update(float fFrameTime);

	// Events
	bool OnKill(const HitInfo& hitInfo);
	bool OnHit(const HitInfo& hitInfo);
	bool OnExplosion(const ExplosionInfo& explosionInfo);
	bool OnAnimationEvent(const AnimEventInstance &event, const uint32 eventNameCRC);
	bool HandleEvent(const SGameObjectEvent& event);
	void OnCollision(const EventPhysCollision& collision);
	void OnRevive();

	void Reload();

	bool StartHitReaction(const HitInfo& hitInfo, const SReactionParams& reactionParams);
	bool StartDeathReaction(const HitInfo& hitInfo, const SReactionParams& reactionParams);

	bool IsValidReaction(const HitInfo& hitInfo, const SReactionParams& reactionParams) const;
	bool ExecuteHitReaction(const SReactionParams& reactionParams);
	bool ExecuteDeathReaction(const SReactionParams& reactionParams);

	bool IsValidReaction(const HitInfo& hitInfo, const ScriptTablePtr pScriptTable) const;
	bool ExecuteHitReaction(const ScriptTablePtr pScriptTable);
	bool ExecuteDeathReaction(const ScriptTablePtr pScriptTable);

	bool EndCurrentReaction(bool bForceRagdollOnHit = false);

	bool IsInDeathReaction() const { return m_currentReaction == eRT_Death; }
	bool IsInHitReaction() const { return m_currentReaction == eRT_Hit; }
	bool IsInReaction() const { return (IsInDeathReaction() || IsInHitReaction()); }

	// FIXME: Currently used to prevent AI from moving while in a hit reaction - To be replaced by better method.
	bool CanActorMove() const { return (eAIP_NotPaused == m_AIPausedState); }

	// "Reaction Anim" utils. This methods allows playing animations directly, without the need of a dedicated 
	// animation graph state setup. Designed (and enforced) to be used _only_ during hit or death reactions
	bool StartReactionAnim(const string& sAnimName, bool bLoop = false, float fBlendTime = 0.2f, int iSlot = 0, int iLayer = 0, uint32 animFlags = (DEFAULT_REACTION_ANIM_FLAGS), float fAdditiveWeight = 0.0f, float fAniSpeed = 1.0f);
	bool StartReactionAnimByID(int animID, bool bLoop = false, float fBlendTime = 0.2f, int iSlot = 0, int iLayer = 0, uint32 animFlags = (DEFAULT_REACTION_ANIM_FLAGS), float fAdditiveWeight = 0.0f, float fAniSpeed = 1.0f);
	void EndReactionAnim();
	bool IsPlayingReactionAnim() const;

	void SetDestructiblesLastEvent(unsigned int lastEventCRC32) { m_lastEventCRC32 = lastEventCRC32;}

	// IDeferredRaycastReceiver interface
	//async raycasts results callback
	virtual void OnDataReceived(const EventPhysRWIResult *pRWIResult);

	//async primitive casts results callback
	virtual void OnDataReceived(const EventPhysPWIResult *pPWIResult);

	//reset data callback
	virtual void OnDRWReset();
	// ~IDeferredRaycastReceiver interface

	void GetMemoryUsage(ICrySizer *pSizer ) const;
private:
	// Private predicates
	struct SPredFindValidReaction;

	// Private methods
	void										ClearState();

	bool 										CanPlayHitReaction() const;
	bool 										CanPlayDeathReaction() const;
	bool										IsAIPlayingTargetPhase() const;

	void 										SetInReaction(EReactionType reactionType);
	void 										SetExecutionType(EExecutionType executionType) { m_currentExecutionType = executionType; }
	bool 										IsExecutionType(EExecutionType executionType) const { return (m_currentExecutionType == executionType); }

	void 										SetVariations(const SReactionParams::SAnimGraphReaction::VariationsContainer& variations) const;
	bool 										ExecuteCommonReaction(const SReactionParams& reactionParams);
	void 										PausePipeUser(bool bPause);
	bool										CanAIEnterPoint(const Vec3 &vDest) const;

	void 										LoadData(SmartScriptTable pSelfTable, bool bForceReload);

	void 										OnCustomAnimFinished();

	void										DoCollisionCheck();
	void 										StartCollisionReaction(const Vec3& vNormal, const Vec3& vPoint);

	ECardinalDirection			GetRelativeCardinalDirection2D(const Vec2& vDir1, const Vec2& vDir2) const;
	bool										CheckCardinalDirection2D(ECardinalDirection direction, const Vec2& vDir1, const Vec2& vDir2) const;
	bool 										CustomAnimHasFinished() const;
	void 										MakePreviousAnimInterruptible(int iSlot, int iLayer);
	void 										StopHigherLayers(int iSlot, int iLayer, float fBlendOut);

	uint32									GetSynchedSeed(bool bKillReaction) const;
	float										GetRandomProbability() const;

/*
	void 										SetHitReactGoalPipe(IPipeUser* pPipeUser) const;
	void 										ClearHitReactGoalPipe(IPipeUser* pPipeUser) const;
*/
	bool										IsAI() const;
	IPipeUser*							GetAIPipeUser() const;
	void										HandleAIStartHitReaction(const HitInfo& hitInfo, const SReactionParams& reactionParams);
	void										HandleAIEndHitReaction();
	void										HandleAIHitReactionInterrupted();
	bool										SendAISignal(const char* szSignal, IAISignalExtraData *pData = NULL, bool bWaitOpOnly = false) const;

	void 										DrawDebugInfo();

	bool										IsValidReactionId(ReactionId reactionId) const;
	const SReactionParams&	GetReactionParamsById(ReactionId reactionId) const;

	// Private attributes
	IScriptSystem*							m_pScriptSystem;

	ReactionsContainerConstPtr	m_pDeathReactions;
	ReactionsContainerConstPtr	m_pHitReactions;
	ReactionsContainerConstPtr	m_pCollisionReactions;

	ScriptTablePtr							m_pSelfTable;

	CActor&											m_actor;

	mutable CMTRand_int32				m_pseudoRandom;

	float												m_fReactionCounter; // holds time since last reaction triggering. Sanity check purpose
	float												m_fReactionEndTime; // time to switch to ragdoll
	int													m_iGoalPipeId; // holds the Id of the goalpipe used by the actor
	EAIPausedState							m_AIPausedState;
	unsigned int								m_lastEventCRC32;

	EExecutionType							m_currentExecutionType;
	SCustomAnim									m_currentCustomAnim;

	EReactionType								m_currentReaction;

	unsigned char								m_reactionOnCollision;

	const HitInfo*							m_pHitInfo;

	CDeferredRaycastHelper			m_raycastHelper;
};

#endif // __HIT_DEATH_REACTIONS
