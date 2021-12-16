/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: Allows game modules system to talk to IGameStatistics system

	-------------------------------------------------------------------------
	History:
	- 20:10:2009  : Created by Mark Tully

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesStatsRecording.h"
#include "IXml.h"
#include "GameRules.h"
#include "IGameStatistics.h"
#include "ICryAnimation.h"
#include "StatsRecordingMgr.h"
#include "StatHelpers.h"

void CGameRulesStatsRecording::Init(
	XmlNodeRef			inXML)
{
	m_gameStatistics=g_pGame->GetIGameFramework()->GetIGameStatistics();
	m_actorSystem=g_pGame->GetIGameFramework()->GetIActorSystem();
	m_statsMgr=g_pGame->GetStatsRecorder();
	assert(m_gameStatistics);
	assert(m_actorSystem);
	assert(m_statsMgr);
}

void CGameRulesStatsRecording::OnInGameBegin()
{
	m_statsMgr->BeginRound();

	// add all players that are already spawned into the stats tracking system

	// AI aren't in the gamerules player list,
	//	so add all actors here instead
	IActorSystem *pActorSystem = g_pGame->GetIGameFramework()->GetIActorSystem();
	IActorIteratorPtr it = pActorSystem->CreateActorIterator();
	while (IActor *pActor = it->Next())
	{
		OnPlayerRevived(pActor);
	}
}

void CGameRulesStatsRecording::OnPostGameBegin()
{
	m_statsMgr->StopTrackingAllPlayerStats();
	m_statsMgr->EndRound();
}

void CGameRulesStatsRecording::OnGameEnd()
{
	m_statsMgr->EndRound();
}

void CGameRulesStatsRecording::OnPlayerRevived(
	IActor			*inActor)
{
	m_statsMgr->StartTrackingStats(inActor);
}

void CGameRulesStatsRecording::OnPlayerKilled(
	IActor			*inActor)
{
	m_statsMgr->StopTrackingStats(inActor);
}
