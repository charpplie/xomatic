#include "StdAfx.h"
#include "AISignalCRCs.h"
#include <crc32.h>


AISIGNALS_CRC::AISIGNALS_CRC() : m_crcGen(NULL){}


void AISIGNALS_CRC::Init(const Crc32Gen* const crcGen)
{
	assert(crcGen);
	m_crcGen = crcGen;
	m_nOnRequestNewObstacle = crcGen->GetCRC32("OnRequestNewObstacle");
	m_nOnRequestUpdate = crcGen->GetCRC32("OnRequestUpdate"); 
	m_nOnRequestUpdateAlternative = crcGen->GetCRC32("OnRequestUpdateAlternative"); 
	m_nOnRequestUpdateTowards = crcGen->GetCRC32("OnRequestUpdateTowards"); 
	m_nOnClearSpotList = crcGen->GetCRC32("OnClearSpotList"); 
	m_nOnRequestShootSpot = crcGen->GetCRC32("OnRequestShootSpot");
	m_nOnRequestHideSpot = crcGen->GetCRC32("OnRequestHideSpot");
	m_nOnStartTimer = crcGen->GetCRC32("OnStartTimer");
	m_nOnLastKnownPositionApproached = crcGen->GetCRC32("OnLastKnownPositionApproached");
	m_nOnEndApproach = crcGen->GetCRC32("OnEndApproach");
	m_nOnAbortAction = crcGen->GetCRC32("OnAbortAction");
	m_nOnActionCompleted = crcGen->GetCRC32("OnActionCompleted");
	m_nOnNoPathFound = crcGen->GetCRC32("OnNoPathFound");
	m_nOnApproachEnd = crcGen->GetCRC32("OnApproachEnd");
	m_nOnEndFollow = crcGen->GetCRC32("OnEndFollow");
	m_nOnBulletRain = crcGen->GetCRC32("OnBulletRain");
	m_nOnBulletHit = crcGen->GetCRC32("OnBulletHit");
	m_nOnDriverEntered = crcGen->GetCRC32("OnDriverEntered");
	m_nOnCheckDeadTarget = crcGen->GetCRC32("OnCheckDeadTarget");
	m_nOnCheckDeadBody = crcGen->GetCRC32("OnCheckDeadBody");

	m_nAIORD_ATTACK = crcGen->GetCRC32(AIORD_ATTACK);
	m_nAIORD_SEARCH = crcGen->GetCRC32(AIORD_SEARCH);
	m_nAIORD_REPORTDONE = crcGen->GetCRC32(AIORD_REPORTDONE);
	m_nAIORD_REPORTFAIL = crcGen->GetCRC32(AIORD_REPORTFAIL);

	m_nOnChangeStance =  crcGen->GetCRC32("OnChangeStance");
	m_nOnScaleFormation = crcGen->GetCRC32("OnScaleFormation");
	m_nOnUnitDied = crcGen->GetCRC32("OnUnitDied");
	m_nOnUnitDamaged = crcGen->GetCRC32("OnUnitDamaged");
	m_nOnUnitBusy = crcGen->GetCRC32("OnUnitBusy");
	m_nOnUnitSuspended = crcGen->GetCRC32("OnUnitSuspended");
	m_nOnUnitResumed = crcGen->GetCRC32("OnUnitResumed");
	m_nOnSetUnitProperties = crcGen->GetCRC32("OnSetUnitProperties");
	m_nOnJoinTeam = crcGen->GetCRC32("OnJoinTeam");
	m_nRPT_LeaderDead = crcGen->GetCRC32("RPT_LeaderDead"); 

	m_nOnSpotSeeingTarget = crcGen->GetCRC32("OnSpotSeeingTarget");
	m_nOnSpotLosingTarget = crcGen->GetCRC32("OnSpotLosingTarget");

	m_nOnFormationPointReached = crcGen->GetCRC32("OnFormationPointReached");
	m_nOnKeepEnabled = crcGen->GetCRC32("OnKeepEnabled");
	m_nOnActionDone = crcGen->GetCRC32("OnActionDone");
	m_nOnGroupAdvance = crcGen->GetCRC32("OnGroupAdvance");
	m_nOnGroupSeek = crcGen->GetCRC32("OnGroupSeek"); 
	m_nOnLeaderReadabilitySeek = crcGen->GetCRC32("OnLeaderReadabilitySeek");
	m_nAddDangerPoint = crcGen->GetCRC32("AddDangerPoint");
	m_nOnGroupCover = crcGen->GetCRC32("OnGroupCover");
	m_nOnLeaderReadabilityAlarm = crcGen->GetCRC32("OnLeaderReadabilityAlarm");
	m_nOnAdvanceTargetCompromised = crcGen->GetCRC32("OnAdvanceTargetCompromised");
	m_nOnGroupCohesionTest = crcGen->GetCRC32("OnGroupCohesionTest");
	m_nOnGroupTestReadabilityCohesion = crcGen->GetCRC32("OnGroupTestReadabilityCohesion");
	m_nOnGroupTestReadabilityAdvance = crcGen->GetCRC32("OnGroupTestReadabilityAdvance");
	m_nOnGroupAdvanceTest = crcGen->GetCRC32("OnGroupAdvanceTest");
	m_nOnLeaderReadabilityAdvanceLeft = crcGen->GetCRC32("OnLeaderReadabilityAdvanceLeft");
	m_nOnLeaderReadabilityAdvanceRight = crcGen->GetCRC32("OnLeaderReadabilityAdvanceRight");
	m_nOnLeaderReadabilityAdvanceForward = crcGen->GetCRC32("OnLeaderReadabilityAdvanceForward");
	m_nOnGroupTurnAmbient = crcGen->GetCRC32("OnGroupTurnAmbient");
	m_nOnGroupTurnAttack = crcGen->GetCRC32("OnGroupTurnAttack");
	m_nOnShapeEnabled = crcGen->GetCRC32("OnShapeEnabled");
	m_nOnShapeDisabled = crcGen->GetCRC32("OnShapeDisabled");
	m_nOnCloseContact = crcGen->GetCRC32("OnCloseContact");
	m_nOnGroupMemberDiedNearest = crcGen->GetCRC32("OnGroupMemberDiedNearest"); 
	m_nOnTargetDead = crcGen->GetCRC32("OnTargetDead"); 
	m_nOnEndPathOffset = crcGen->GetCRC32("OnEndPathOffset");
	m_nOnPathFindAtStart = crcGen->GetCRC32("OnPathFindAtStart");
	m_nOnPathFound = crcGen->GetCRC32("OnPathFound");
	m_nOnBackOffFailed = crcGen->GetCRC32("OnBackOffFailed");
	m_nOnBadHideSpot = crcGen->GetCRC32("OnBadHideSpot");
	m_nOnHideSpotReached = crcGen->GetCRC32("OnHideSpotReached");
	m_nOnMovingToCover = crcGen->GetCRC32("OnMovingToCover");
	m_nOnEnterCover = crcGen->GetCRC32("OnEnterCover");
	m_nOnLeaveCover = crcGen->GetCRC32("OnLeaveCover");
	m_nOnCoverCompromised = crcGen->GetCRC32("OnCoverCompromised");
	m_nOnMoveInHighCover = crcGen->GetCRC32("OnMoveInHighCover");
	m_nOnMoveInLowCover = crcGen->GetCRC32("OnMoveInLowCover");
	m_nOnTPSDestinationNotFound = crcGen->GetCRC32("OnTPSDestNotFound");
	m_nOnTPSDestinationFound = crcGen->GetCRC32("OnTPSDestFound");
	m_nOnTPSDestinationReached = crcGen->GetCRC32("OnTPSDestReached" );
	m_nOnRightLean = crcGen->GetCRC32("OnRightLean");
	m_nOnLeftLean = crcGen->GetCRC32("OnLeftLean");
	m_nOnLowHideSpot = crcGen->GetCRC32("OnLowHideSpot"); 
	m_nOnChargeStart = crcGen->GetCRC32("OnChargeStart");
	m_nOnChargeHit = crcGen->GetCRC32("OnChargeHit");
	m_nOnChargeMiss = crcGen->GetCRC32("OnChargeMiss");
	m_nOnChargeBailOut = crcGen->GetCRC32("OnChargeBailOut");
	m_nORDER_IDLE= crcGen->GetCRC32("ORDER_IDLE");
	m_nOnNoGroupTarget = crcGen->GetCRC32("OnNoGroupTarget");
	m_nOnLeaderTooFar = crcGen->GetCRC32("OnLeaderTooFar");
	m_nOnEnemySteady = crcGen->GetCRC32("OnEnemySteady");
	m_nOnShootSpotFound= crcGen->GetCRC32("OnShootSpotFound");
	m_nOnShootSpotNotFound= crcGen->GetCRC32("OnShootSpotNotFound"); 
	m_nOnLeaderStop = crcGen->GetCRC32("OnLeaderStop");
	m_nOnUnitMoving = crcGen->GetCRC32("OnUnitMoving");
	m_nOnUnitStop = crcGen->GetCRC32("OnUnitStop");
	m_nORDER_EXIT_VEHICLE = crcGen->GetCRC32("ORDER_EXIT_VEHICLE");
	m_nOnNoFormationPoint = crcGen->GetCRC32("OnNoFormationPoint");
	m_nOnTargetApproaching = crcGen->GetCRC32("OnTargetApproaching");
	m_nOnTargetFleeing = crcGen->GetCRC32("OnTargetFleeing");
	m_nOnNoTargetVisible= crcGen->GetCRC32("OnNoTargetVisible");
	m_nOnNoTargetAwareness = crcGen->GetCRC32("OnNoTargetAwareness");
	m_nOnNoHidingPlace = crcGen->GetCRC32("OnNoHidingPlace");
	m_nOnLowAmmo = crcGen->GetCRC32("OnLowAmmo");
	m_nOnOutOfAmmo = crcGen->GetCRC32("OnOutOfAmmo");
	m_nOnReloaded = crcGen->GetCRC32("OnReloaded");
	m_nOnMeleeExecuted = crcGen->GetCRC32("OnMeleeExecuted");
	m_nOnPlayerLooking = crcGen->GetCRC32("OnPlayerLooking");
	m_nOnPlayerSticking = crcGen->GetCRC32("OnPlayerSticking");
	m_nOnPlayerLookingAway = crcGen->GetCRC32("OnPlayerLookingAway");
	m_nOnPlayerGoingAway = crcGen->GetCRC32("OnPlayerGoingAway");
	m_nOnTargetTooClose = crcGen->GetCRC32("OnTargetTooClose");
	m_nOnTargetTooFar = crcGen->GetCRC32("OnTargetTooFar"); 
	m_nOnFriendInWay = crcGen->GetCRC32("OnFriendInWay");
	m_nOnUseSmartObject = crcGen->GetCRC32("OnUseSmartObject");
	m_nOnAvoidDanger = crcGen->GetCRC32("OnAvoidDanger");
	m_nOnSetPredictionTime = crcGen->GetCRC32("m_nOnSetPredictionTime");
	m_nOnInterestSystemEvent = crcGen->GetCRC32("OnInterestSystemEvent");		
	m_nOnSpecialAction =  crcGen->GetCRC32("OnSpecialAction");		
	m_nOnNewAttentionTarget = crcGen->GetCRC32("OnNewAttentionTarget");
	m_nOnAttentionTargetThreatChanged = crcGen->GetCRC32("OnAttentionTargetThreatChanged");
};