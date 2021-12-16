/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id: GameRealtimeRemoveUpdate.cpp,v 1.1 23/09/2009 by Johnmichael Quinlan 
	Description:  This is the source file for the game module Realtime remote 
								update. The purpose of this module is to allow data update to happen 
								remotely so that you can, for example, edit the terrain and see the changes
								in the console.
-------------------------------------------------------------------------
History:
- 23:09:2009 :  Created by Johnmichael Quinlan
 ************************************************************************/

#include "StdAfx.h"
#include "GameRealtimeRemoteUpdate.h"
#include "LivePreview/RealtimeRemoteUpdate.h"
#include <IGameFramework.h>
#include <IViewSystem.h>
#include <IEntitySystem.h>
#include "../GameCore/Game.h"
#include <IGameRulesSystem.h>
#include <ILevelSystem.h>


//0 no fly, 1 fly mode, 2 fly mode + noclip
enum eCameraModes
{
	eCameraModes_no_fly	= 0,
	eCameraModes_fly_mode,
	eCameraModes_fly_mode_noclip,
	eCameraModes_count
};

//////////////////////////////////////////////////////////////////////////
CGameRealtimeRemoteUpdateListener&	CGameRealtimeRemoteUpdateListener::GetGameRealtimeRemoteUpdateListener()
{
	LOG_CODE_COVERAGE();
	static CGameRealtimeRemoteUpdateListener oRealtimeUpdateListener;
	return oRealtimeUpdateListener;
}

CGameRealtimeRemoteUpdateListener::CGameRealtimeRemoteUpdateListener():
	m_nPreviousFlyMode(eCameraModes_no_fly),
	m_bCameraSync(false),
	m_Position(0.0f,0.0f,0.0f),
	m_ViewDirection(0.0f,0.0f,0.0f),
	m_CameraUpdateInfo(NULL)
{	
	LOG_CODE_COVERAGE();
}

CGameRealtimeRemoteUpdateListener::~CGameRealtimeRemoteUpdateListener()
{
	LOG_CODE_COVERAGE();
}

bool CGameRealtimeRemoteUpdateListener::Enable(bool boEnable)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(g_pGame);
	CRY_SAFE_RETURN(!g_pGame, false);

	IGameFramework* pGameFramework = g_pGame->GetIGameFramework();
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, false);

// 	IRealtimeRemoteUpdate* pLiveCreate = pGameFramework->GetIRealTimeRemoteUpdate();
// 	CRY_ASSERT(pLiveCreate);
// 	CRY_SAFE_RETURN(!pLiveCreate, false);
// 	
// 	if ( boEnable )
// 	{	
// 		pLiveCreate->Enable(true);
// 		pLiveCreate->AddGameHandler(this);
// 	}
// 	else
// 	{
// 		pLiveCreate->Enable(false);
// 		pLiveCreate->RemoveGameHandler(this);
// 	}

	return boEnable;
}

bool CGameRealtimeRemoteUpdateListener::Update()
{
	LOG_CODE_COVERAGE();

	if (m_CameraUpdateInfo)
	{
		UpdateCamera(m_CameraUpdateInfo);
		m_CameraUpdateInfo=NULL;
	}

	return true;
}

bool CGameRealtimeRemoteUpdateListener::UpdateGameData(XmlNodeRef oXmlNode, unsigned char * auchBinaryData)
{
	LOG_CODE_COVERAGE();

	string oSyncType = oXmlNode->getAttr("Type");

	if ( oSyncType.compare("ChangeLevel")==0 )
	{
		CloseLevel();
	}
	else if ( oSyncType.compare("Camera")==0 )
	{
		m_CameraUpdateInfo=oXmlNode;
		return false;
		m_CameraUpdateInfo=oXmlNode->clone();
	}
	return true;
}

void CGameRealtimeRemoteUpdateListener::UpdateCamera(XmlNodeRef oXmlNode)
{
	LOG_CODE_COVERAGE();

	int	nStartSync(0);

	if (!oXmlNode->getAttr("Position",m_Position))
		return;

	if (!oXmlNode->getAttr("Direction",m_ViewDirection))
		return;

	if (oXmlNode->getAttr("StartSync",nStartSync))
	{
		if (nStartSync)
		{				
			CameraSync();
		}
		else
		{
			EndCameraSync();
		}
	}
}

void CGameRealtimeRemoteUpdateListener::CameraSync()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	IGame* pGame = gEnv->pGame;
	CRY_ASSERT(pGame);
	CRY_SAFE_RETURN(!pGame, CRY_NO_RETURN_VALUE);

	IGameFramework* pGameFramework = pGame->GetIGameFramework();								
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, CRY_NO_RETURN_VALUE);

	IViewSystem* pViewSystem = pGameFramework->GetIViewSystem();	
	CRY_ASSERT(pViewSystem);
	CRY_SAFE_RETURN(!pViewSystem, CRY_NO_RETURN_VALUE);

	IView* pView = pViewSystem->GetActiveView();
	CRY_ASSERT(pView);
	CRY_SAFE_RETURN(!pView, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pEntitySystem);
	CRY_SAFE_RETURN(!gEnv->pEntitySystem, CRY_NO_RETURN_VALUE);

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(pView->GetLinkedId());
	CRY_ASSERT(pEntity);
	CRY_SAFE_RETURN(!pEntity, CRY_NO_RETURN_VALUE);

// 	CPlayer* pPlayer = static_cast<CPlayer *>(pGameFramework->GetClientActor());
// 	CRY_ASSERT(pPlayer);
//	CRY_SAFE_RETURN(!pPlayer, CRY_NO_RETURN_VALUE);
// 
// 	IEntity* pPlayerEntity = pPlayer->GetEntity();
// 	CRY_ASSERT(pPlayerEntity);
//	CRY_SAFE_RETURN(!pPlayerEntity, CRY_NO_RETURN_VALUE);
// 
// 	IPhysicalEntity* piPlayerPhysics = pPlayerEntity->GetPhysics();
// 	CRY_ASSERT(piPlayerPhysics);
//	CRY_SAFE_RETURN(!piPlayerPhysics, CRY_NO_RETURN_VALUE);

// 	pe_player_dimensions dim;
// 	piPlayerPhysics->GetParams( &dim );
//	m_Position.z = m_Position.z - dim.heightHead;

	pEntity->SetPos(m_Position);
	pViewSystem->SetOverrideCameraRotation(true,Quat::CreateRotationVDir(m_ViewDirection));

// 	if ( m_bCameraSync == false && m_nPreviousFlyMode != eCameraModes_fly_mode_noclip ) 
// 	{
// 		m_nPreviousFlyMode=pPlayer->GetFlyMode();
// 		pPlayer->SetFlyMode(eCameraModes_fly_mode_noclip);
// 	}

//	pPlayerEntity->Hide(true);

	m_bCameraSync = true;
}

void CGameRealtimeRemoteUpdateListener::EndCameraSync()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	IGame* pGame = gEnv->pGame;
	CRY_ASSERT(pGame);
	CRY_SAFE_RETURN(!pGame, CRY_NO_RETURN_VALUE);

	IGameFramework* pGameFramework = pGame->GetIGameFramework();								
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, CRY_NO_RETURN_VALUE);

	IViewSystem* pViewSystem = pGameFramework->GetIViewSystem();	
	CRY_ASSERT(pViewSystem);
	CRY_SAFE_RETURN(!pViewSystem, CRY_NO_RETURN_VALUE);

	IView* pView = pViewSystem->GetActiveView();
	CRY_ASSERT(pView);
	CRY_SAFE_RETURN(!pView, CRY_NO_RETURN_VALUE);


	CRY_ASSERT(gEnv->pEntitySystem);
	CRY_SAFE_RETURN(!gEnv->pEntitySystem, CRY_NO_RETURN_VALUE);

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(pView->GetLinkedId());
	CRY_ASSERT(pEntity);
	CRY_SAFE_RETURN(!pEntity, CRY_NO_RETURN_VALUE);

// 	CPlayer* pPlayer = static_cast<CPlayer *>(pGameFramework->GetClientActor());
// 	CRY_ASSERT(pPlayer);
//	CRY_SAFE_RETURN(!pPlayer, CRY_NO_RETURN_VALUE);
// 
// 	IEntity* pPlayerEntity = pPlayer->GetEntity();
// 	CRY_ASSERT(pPlayerEntity);
//	CRY_SAFE_RETURN(!pPlayerEntity, CRY_NO_RETURN_VALUE);
// 	
// 	pPlayer->SetFlyMode(m_nPreviousFlyMode);
// 	pPlayerEntity->Hide(false);

	pViewSystem->SetOverrideCameraRotation(false,Quat::CreateRotationVDir(m_ViewDirection));
	pEntity->SetRotation(Quat::CreateRotationVDir(m_ViewDirection));
	m_bCameraSync = false;
}

void CGameRealtimeRemoteUpdateListener::CloseLevel()
{
	LOG_CODE_COVERAGE();

	if ( m_bCameraSync )
		EndCameraSync();

	char packPath[256];
	g_pGame->GetIGameFramework()->EndGameContext();
	
	if ( !g_pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel() )
		return;

	const char * fullPath = g_pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel()->GetLevelInfo()->GetPath();

	_snprintf(packPath, sizeof(packPath), "%s\\*.pak", fullPath);
	gEnv->pCryPak->ClosePacks(packPath,0);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
