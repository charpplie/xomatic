////////////////////////////////////////////////////////////////////////////
//
//  StalTech Engine Source File.
//  Copyright (C), StalTech Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   AIEnvironment.cpp
//  Created:     29/02/2008 by Matthew
//  Description: Simple global struct for accessing major module pointers
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Environment.h"



#include "ObjectContainer.h"

#include "CodeCoverageManager.h"
#include "CodeCoverageGUI.h"
#include "GoalOpFactory.h"
#include "StatsManager.h"
#include "TacticalPointSystem/TacticalPointSystem.h"
#include "TargetSelection/TargetTrackManager.h"
#include "GroupSystem/GroupSystem.h"
#include "NullAIDebugRenderer.h"


static CNullAIDebugRenderer nullAIRenderer;


SAIEnvironment::SAIEnvironment()
: pGoalOpFactory(NULL)
, pObjectContainer(NULL)
, pCodeCoverageTracker(NULL)
, pCodeCoverageManager(NULL)
, pCodeCoverageGUI(NULL)
, pTacticalPointSystem(NULL)
, pTargetTrackManager(NULL)
, pStatsManager(NULL)
, pGroupSystem(NULL)
, pPipeManager(NULL)
, pAIActionManager(NULL)
, pSmartObjectManager(NULL)
, pCoordinationManager(NULL)
, pCommunicationManager(NULL)
, pCoverSystem(NULL)
, pSelectionTreeManager(NULL)
, pRayCaster(NULL)
, pIntersectionTester(NULL)
{
	SetDebugRenderer(0);
	SetNetworkDebugRenderer(0);
}

SAIEnvironment::~SAIEnvironment()
{
	SAFE_DELETE (pGoalOpFactory);
	SAFE_DELETE (pCodeCoverageTracker);
	SAFE_DELETE (pCodeCoverageManager);
	SAFE_DELETE (pCodeCoverageGUI);
	SAFE_DELETE (pStatsManager);
	SAFE_DELETE (pTacticalPointSystem);
	SAFE_DELETE (pTargetTrackManager);
	SAFE_DELETE (pGroupSystem);
	SAFE_DELETE (pObjectContainer);
}

IAIDebugRenderer* SAIEnvironment::GetDebugRenderer()
{
	return pDebugRenderer;
}

IAIDebugRenderer* SAIEnvironment::GetNetworkDebugRenderer()
{
	return pNetworkDebugRenderer;
}

void SAIEnvironment::SetDebugRenderer(IAIDebugRenderer* pAIDebugRenderer)
{
	pDebugRenderer = pAIDebugRenderer ? pAIDebugRenderer : &nullAIRenderer;
}

void SAIEnvironment::SetNetworkDebugRenderer(IAIDebugRenderer* pAINetworkDebugRenderer)
{
	pNetworkDebugRenderer = pAINetworkDebugRenderer ? pAINetworkDebugRenderer : &nullAIRenderer;
}



SAIEnvironment gAIEnv;
