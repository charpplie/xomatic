////////////////////////////////////////////////////////////////////////////
//
//  StalTech Engine Source File.
//  Copyright (C), StalTech Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   AIEnvironment.h
//  Created:     29/02/2008 by Matthew
//  Description: Simple global struct for accessing major module pointers
//  Notes:       auto_ptrs to handle init/delete would be great,
//               but exposing autoptrs to the rest of the code needs thought
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AIENVIRONMENT
#define __AIENVIRONMENT

#pragma once

#include "Configuration.h"
#include "AIConsoleVariables.h"
#include "AISignalCRCs.h"
#include "RayCastQueue.h"
#include "IntersectionTestQueue.h"

//////////////////////////////////////////////////////////////////////////
// AI system environment.
// For internal use only!
// Contain pointers to commonly needed modules for simple, efficient lookup
// Need a clear point in execution when these will all be valid
//////////////////////////////////////////////////////////////////////////
struct IGoalOpFactory;
class CObjectContainer;
class CCodeCoverageTracker;
class CCodeCoverageManager;
class CCodeCoverageGUI;
class CStatsManager;
class CTacticalPointSystem;
class CGroupSystem;
class CTargetTrackManager;
struct IAIDebugRenderer;
class CPipeManager;
class CGraph;
class CPathfinder;
class CNavigation;
class CAIActionManager;
class CSmartObjectManager;
class CCoordinationManager;
class CCommunicationManager;
class CCoverSystem;
class CSelectionTreeManager;



struct SAIEnvironment
{
	AIConsoleVars CVars;
	AISIGNALS_CRC SignalCRCs;

	SConfiguration configuration;


	IGoalOpFactory* pGoalOpFactory;
	CObjectContainer* pObjectContainer;
	CCodeCoverageTracker* pCodeCoverageTracker;
	CCodeCoverageManager* pCodeCoverageManager;
	CCodeCoverageGUI* pCodeCoverageGUI;
	CStatsManager* pStatsManager;
	CTacticalPointSystem* pTacticalPointSystem;
	CTargetTrackManager* pTargetTrackManager;
	CGroupSystem* pGroupSystem;

	CPipeManager* pPipeManager;
	CGraph* pGraph;
	CPathfinder* pPathfinder;
	CNavigation* pNavigation;
	CAIActionManager* pAIActionManager;
	CSmartObjectManager* pSmartObjectManager;

	CCoordinationManager* pCoordinationManager;
	CCommunicationManager* pCommunicationManager;
	CCoverSystem* pCoverSystem;
	CSelectionTreeManager* pSelectionTreeManager;

	typedef RayCastQueue<41> GlobalRayCaster;
	GlobalRayCaster* pRayCaster;
	
	typedef IntersectionTestQueue<43> GlobalIntersectionTester;
	GlobalIntersectionTester* pIntersectionTester;

	//more cache friendly
	IPhysicalWorld* pWorld;//TODO use this more, or eliminate it.
	

	SAIEnvironment();
	~SAIEnvironment();
	

	IAIDebugRenderer* GetDebugRenderer();
	IAIDebugRenderer* GetNetworkDebugRenderer();
	void SetDebugRenderer(IAIDebugRenderer* pAIDebugRenderer);
	void SetNetworkDebugRenderer(IAIDebugRenderer* pAIDebugRenderer);

private:
	IAIDebugRenderer* pDebugRenderer;
	IAIDebugRenderer* pNetworkDebugRenderer;
};

extern SAIEnvironment gAIEnv;

#endif // __AIENVIRONMENT
