/********************************************************************
	StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2009.
 -------------------------------------------------------------------------
  File name:   CAISystem.cpp
	$Id$
  Description: 
  
 -------------------------------------------------------------------------
  History:
	- 2001				: Created by Petar Kotevski
	- 2003				: Taken over by Kirill Bulatsev
	- 2 Mar 2009	: Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include "StdAfx.h"

#include "CAISystem.h"
#include "AILog.h"

#include "ScriptBind_AI.h"
#include "Scriptbind_AI_G02.h"
#include "Scriptbind_AI_G04.h"
#include "ScriptBind_AI2.h"

#include "AIVehicle.h"
#include "AIPlayer.h"
#include "ObjectTracker.h"
#include "ObjectContainer.h"
#include "CodeCoverageManager.h"
#include "CodeCoverageGUI.h"
#include "IVisionMap.h"
#include "HidespotQueryContext.h"
#include "AISystemListener.h"
#include "PerceptionManager.h"
#include "SmartObjects.h"
#include "CalculationStopper.h"
#include "AIRadialOcclusion.h"
#include "AIActions.h"
#include "GraphNodeManager.h"
#include "FireCommand.h"
#include "CentralInterestManager.h"
#include "TacticalPointSystem/TacticalPointSystem.h"
#include "TargetSelection/TargetTrackManager.h"
#include "EmotionalSystem/EmotionalSystem.h"
#include "BehaviorTree/ProfileDictionary.h"
#include "GroupSystem/GroupSystem.h"
#include "Readability/CoopReadabilitiesSystem.h"
#include "CoordinationSystem/CoordinationManager.h"
#include "Communication/CommunicationManager.h"
#include "Cover/CoverSystem.h"
#include "SelectionTree/SelectionTreeManager.h"
#include "NewBehaviourTree/TreeProfileManager.h"
#include "DebugDrawContext.h"

#define		TREE_PROFILES_BASEDIR  "Scripts/AI/NewBehaviourTree"

// used to determine if a forbidden area should be rasterised





const size_t AILogMaxIdLen	=32;
static const char *sCodeCoverageContextFile = "ccContext.txt";

#define BroadcastToListeners(call) \
	SystemListenerSet::iterator listenerIt = m_setSystemListeners.begin(); \
	SystemListenerSet::iterator listenerEnd = m_setSystemListeners.end(); \
	for ( ; listenerIt != listenerEnd; ++listenerIt) \
{ \
	(*listenerIt)->call; \
}

// Little helper template
// T should be in practice a CWeakRef<CAIObject> , CCountedRef<CAIObject>, etc.
template < template <typename> class T > struct ObjMapIter
{
	typedef typename std::multimap < short , T < CAIObject> >::iterator Type;
};


//====================================================================
// SAIObjectMapIter
// Specialized next for iterating over the whole container.
//====================================================================
template < template <typename> class T > struct SAIObjectMapIter : public IAIObjectIter
{
	typedef typename ObjMapIter<T>::Type Iter;

	SAIObjectMapIter( Iter first, Iter end) :
		m_it(first), m_end(end), m_ai(0)
	{
	}

	virtual IAIObject* GetObject()
	{
		if (!m_ai && m_it != m_end)
			Next();
		return m_ai;
	}

  virtual void Release() 
	{
		// (MATT) Call my own destructor before I push to the pool - avoids tripping up the STLP debugging {2008/12/04})
		this->~SAIObjectMapIter();
		pool.push_back(this);
	}

	// (MATT) (was) Update the pointer from the current iterator if it's valid. 
	//        (now) That, and keep advancing if the ref is not valid {2009/03/27}
	virtual void	Next()
	{
		for ( m_ai = NULL; !m_ai && m_it != m_end; ++m_it)
			m_ai = m_it->second.GetAIObject();
	}

  static SAIObjectMapIter* Allocate(Iter first, Iter end)
	{
    if (pool.empty())
    {
      return new SAIObjectMapIter(first, end);
    }
    else
    {
      SAIObjectMapIter *res = pool.back(); pool.pop_back();
      return new(res) SAIObjectMapIter(first, end);
    }
  }

	IAIObject*	m_ai;
	Iter	m_it;
	Iter	m_end;
  static std::vector<SAIObjectMapIter*> pool;
};

//====================================================================
// SAIObjectMapIterOfType
// Iterator base for iterating over 'AIObjects' container.
// Returns only objects of specified type.
//====================================================================
template < template <typename> class T > struct SAIObjectMapIterOfType : public SAIObjectMapIter <T>
{
	typedef typename SAIObjectMapIter<T>::Iter Iter;
	
	using SAIObjectMapIter<T>::m_ai;
	using SAIObjectMapIter<T>::m_it;
	using SAIObjectMapIter<T>::m_end;

	SAIObjectMapIterOfType(short n, Iter first, Iter end) :
		m_n(n), SAIObjectMapIter<T>(first, end) {}

  virtual void Release() 
	{
		// (MATT) Call my own destructor before I push to the pool - avoids tripping up the STLP debugging {2008/12/04})
		this->~SAIObjectMapIterOfType();
		pool.push_back(this);
	}

  static SAIObjectMapIterOfType* Allocate(short n, Iter first, Iter end)
  {
    if (pool.empty())
    {
      return new SAIObjectMapIterOfType(n, first, end);
    }
    else
    {
      SAIObjectMapIterOfType *res = pool.back(); pool.pop_back();
      return new(res) SAIObjectMapIterOfType(n, first, end);
    }
  }

	virtual void	Next()
	{
		for ( m_ai = NULL; !m_ai && m_it != m_end; ++m_it)
		{
		// Constraint to type
			if (m_it->first != m_n)
			{
			m_it = m_end;
				break;
			}
			else
			{
				m_ai = m_it->second.GetAIObject();
			}
		}
	}

	short	m_n;
  static std::vector<SAIObjectMapIterOfType*> pool;
};

//====================================================================
// SAIObjectMapIterInRange
// Iterator base for iterating over 'AIObjects' container.
// Returns only objects which are enclosed by the specified sphere.
//====================================================================
template < template <typename> class T > struct SAIObjectMapIterInRange : public SAIObjectMapIter <T>
{
	typedef typename ObjMapIter<T>::Type Iter;
	
	using SAIObjectMapIter<T>::m_ai;
	using SAIObjectMapIter<T>::m_it;
	using SAIObjectMapIter<T>::m_end;


	SAIObjectMapIterInRange(Iter first, Iter end, const Vec3& center, float rad, bool check2D) :
		m_center(center), m_rad(rad), m_check2D(check2D), SAIObjectMapIter<T>(first, end) {}

  virtual void Release() 
	{
		// (MATT) Call my own destructor before I push to the pool - avoids tripping up the STLP debugging {2008/12/04})
		this->~SAIObjectMapIterInRange();
		pool.push_back(this);
	}

  static SAIObjectMapIterInRange* Allocate(Iter first, Iter end, const Vec3& center, float rad, bool check2D)
  {
    if (pool.empty())
    {
      return new SAIObjectMapIterInRange(first, end, center, rad, check2D);
    }
    else
    {
      SAIObjectMapIterInRange *res = pool.back(); pool.pop_back();
      return new(res) SAIObjectMapIterInRange(first, end, center, rad, check2D);
    }
  }

  virtual void	Next()
	{
		for ( m_ai = NULL; !m_ai && m_it != m_end; ++m_it)
		{
			CAIObject *pObj = m_it->second.GetAIObject();
			if (!pObj)
				continue;

			// Constraint to sphere
			if(m_check2D)
			{
				if(Distance::Point_Point2DSq(m_center, pObj->GetPos()) < sqr(pObj->GetRadius() + m_rad)) 
					m_ai = pObj;
			}
			else
			{
				if(Distance::Point_PointSq(m_center, pObj->GetPos()) < sqr(pObj->GetRadius() + m_rad))
					m_ai = pObj;
			}
		}

	}

	Vec3	m_center;
	float	m_rad;
	bool	m_check2D;
  static std::vector<SAIObjectMapIterInRange*> pool;
};

//====================================================================
// SAIObjectMapIterOfTypeInRange
// Specialized next for iterating over the whole container.
// Returns only objects which are enclosed by the specified sphere and of specified type.
//====================================================================
template < template <typename> class T > struct SAIObjectMapIterOfTypeInRange : public SAIObjectMapIter <T>
{
	typedef typename SAIObjectMapIter<T>::Iter Iter;

	using SAIObjectMapIter<T>::m_ai;
	using SAIObjectMapIter<T>::m_it;
	using SAIObjectMapIter<T>::m_end;


	SAIObjectMapIterOfTypeInRange(short n, Iter first, Iter end, const Vec3& center, float rad, bool check2D) :
		m_n(n), m_center(center), m_rad(rad), m_check2D(check2D), SAIObjectMapIter<T>(first, end) {}

  virtual void Release() 
	{
		// (MATT) Call my own destructor before I push to the pool - avoids tripping up the STLP debugging {2008/12/04})
		this->~SAIObjectMapIterOfTypeInRange();
		pool.push_back(this);
	}

  static SAIObjectMapIterOfTypeInRange* Allocate(short n, Iter first, Iter end, const Vec3& center, float rad, bool check2D)
  {
    if (pool.empty())
    {
      return new SAIObjectMapIterOfTypeInRange(n, first, end, center, rad, check2D);
    }
    else
    {
      SAIObjectMapIterOfTypeInRange *res = pool.back(); pool.pop_back();
      return new(res) SAIObjectMapIterOfTypeInRange(n, first, end, center, rad, check2D);
    }
  }

  virtual void	Next()
	{
		for ( m_ai = NULL; !m_ai && m_it != m_end; ++m_it)
		{
			// Constraint to type
			if(m_it->first != m_n)
			{
				m_it = m_end;
				break;
			}

			CAIObject *pObj = m_it->second.GetAIObject();
			if (!pObj)
				continue;

			// Constraint to sphere
			if(m_check2D)
			{
				if(Distance::Point_Point2DSq(m_center, pObj->GetPos()) < sqr(pObj->GetRadius() + m_rad)) 
					m_ai = pObj;
			}
			else
			{
				if(Distance::Point_PointSq(m_center, pObj->GetPos()) < sqr(pObj->GetRadius() + m_rad))
					m_ai = pObj;
			}
		}
	}

	short	m_n;
	Vec3	m_center;
	float	m_rad;
	bool	m_check2D;
  static std::vector<SAIObjectMapIterOfTypeInRange*> pool;
};

//====================================================================
// SAIObjectMapIterInShape
// Iterator base for iterating over 'AIObjects' container.
// Returns only objects which are enclosed by the specified shape.
//====================================================================
template < template <typename> class T > struct SAIObjectMapIterInShape : public SAIObjectMapIter <T>
{
	typedef typename SAIObjectMapIter<T>::Iter Iter;

	using SAIObjectMapIter<T>::m_ai;
	using SAIObjectMapIter<T>::m_it;
	using SAIObjectMapIter<T>::m_end;
	
	SAIObjectMapIterInShape(Iter first, Iter end, const SShape& shape, bool checkHeight) :
		m_shape(shape), m_checkHeight(checkHeight), SAIObjectMapIter<T>(first, end) {}

  virtual void Release() 
	{
		// (MATT) Call my own destructor before I push to the pool - avoids tripping up the STLP debugging {2008/12/04})
		this->~SAIObjectMapIterInShape();
		pool.push_back(this);
	}

  static SAIObjectMapIterInShape* Allocate(Iter first, Iter end, const SShape& shape, bool checkHeight)
  {
    if (pool.empty())
    {
      return new SAIObjectMapIterInShape(first, end, shape, checkHeight);
    }
    else
    {
      SAIObjectMapIterInShape *res = pool.back(); pool.pop_back();
      return new(res) SAIObjectMapIterInShape(first, end, shape, checkHeight);
    }
  }

  virtual void	Next()
	{
		for ( m_ai = NULL; !m_ai && m_it != m_end; ++m_it)
		{
			CAIObject *pObj = m_it->second.GetAIObject();
			if (!pObj)
				continue;

			// Constraint to shape
			if(m_shape.IsPointInsideShape(pObj->GetPos(), m_checkHeight))
				m_ai = pObj;
		}
	}

	const SShape& m_shape;
	bool	m_checkHeight;
  static std::vector<SAIObjectMapIterInShape*> pool;
};

//====================================================================
// SAIObjectMapIterOfTypeInRange
// Specialized next for iterating over the whole container.
// Returns only objects which are enclosed by the specified shape and of specified type.
//====================================================================
template < template <typename> class T > struct SAIObjectMapIterOfTypeInShape : public SAIObjectMapIter <T>
{
	typedef typename SAIObjectMapIter<T>::Iter Iter;

	using SAIObjectMapIter<T>::m_ai;
	using SAIObjectMapIter<T>::m_it;
	using SAIObjectMapIter<T>::m_end;


	SAIObjectMapIterOfTypeInShape(short n, Iter first, Iter end, const SShape& shape, bool checkHeight) :
		m_n(n), m_shape(shape), m_checkHeight(checkHeight), SAIObjectMapIter<T>(first, end) {}

  virtual void Release() 
	{
		// (MATT) Call my own destructor before I push to the pool - avoids tripping up the STLP debugging {2008/12/04})
		this->~SAIObjectMapIterOfTypeInShape();
		pool.push_back(this);
	}

  static SAIObjectMapIterOfTypeInShape* Allocate(short n, Iter first, Iter end, const SShape& shape, bool checkHeight)
  {
    if (pool.empty())
    {
      return new SAIObjectMapIterOfTypeInShape(n, first, end, shape, checkHeight);
    }
    else
    {
      SAIObjectMapIterOfTypeInShape *res = pool.back(); pool.pop_back();
      return new(res) SAIObjectMapIterOfTypeInShape(n, first, end, shape, checkHeight);
    }
  }

  virtual void	Next()
	{
		for ( m_ai = NULL; !m_ai && m_it != m_end; ++m_it)
		{
			// Constraint to type
			if(m_it->first != m_n)
			{
				m_it = m_end;
				break;
			}

			CAIObject *pObj = m_it->second.GetAIObject();
			if (!pObj)
				continue;

			// Constraint to shape
			if(m_shape.IsPointInsideShape(pObj->GetPos(), m_checkHeight))
				m_ai = pObj;
			}
	}

	short	m_n;
	const SShape& m_shape;
	bool	m_checkHeight;
  static std::vector<SAIObjectMapIterOfTypeInShape*> pool;
};


template<> std::vector < SAIObjectMapIter <CWeakRef> * > SAIObjectMapIter <CWeakRef>::pool = std::vector<SAIObjectMapIter <CWeakRef> *>();
template<> std::vector < SAIObjectMapIterOfType <CWeakRef> * > SAIObjectMapIterOfType <CWeakRef>::pool = std::vector<SAIObjectMapIterOfType <CWeakRef>*>();
template<> std::vector < SAIObjectMapIterInRange <CWeakRef> * > SAIObjectMapIterInRange <CWeakRef>::pool = std::vector<SAIObjectMapIterInRange <CWeakRef>*>();
template<> std::vector < SAIObjectMapIterOfTypeInRange <CWeakRef> * > SAIObjectMapIterOfTypeInRange <CWeakRef>::pool = std::vector<SAIObjectMapIterOfTypeInRange <CWeakRef>*>();
template<> std::vector < SAIObjectMapIterInShape <CWeakRef> * > SAIObjectMapIterInShape <CWeakRef>::pool = std::vector<SAIObjectMapIterInShape <CWeakRef>*>();
template<> std::vector < SAIObjectMapIterOfTypeInShape <CWeakRef> * > SAIObjectMapIterOfTypeInShape <CWeakRef>::pool = std::vector<SAIObjectMapIterOfTypeInShape <CWeakRef>*>();
template<> std::vector < SAIObjectMapIter <CCountedRef> * > SAIObjectMapIter <CCountedRef>::pool = std::vector<SAIObjectMapIter <CCountedRef>*>();
template<> std::vector < SAIObjectMapIterOfType <CCountedRef> * > SAIObjectMapIterOfType <CCountedRef>::pool = std::vector<SAIObjectMapIterOfType <CCountedRef>*>();
template<> std::vector < SAIObjectMapIterInRange <CCountedRef> * > SAIObjectMapIterInRange <CCountedRef>::pool = std::vector<SAIObjectMapIterInRange <CCountedRef>*>();
template<> std::vector < SAIObjectMapIterOfTypeInRange <CCountedRef> * > SAIObjectMapIterOfTypeInRange <CCountedRef>::pool = std::vector<SAIObjectMapIterOfTypeInRange <CCountedRef>*>();
template<> std::vector < SAIObjectMapIterInShape <CCountedRef> * > SAIObjectMapIterInShape <CCountedRef>::pool = std::vector<SAIObjectMapIterInShape <CCountedRef>*>();
template<> std::vector < SAIObjectMapIterOfTypeInShape <CCountedRef> * > SAIObjectMapIterOfTypeInShape <CCountedRef>::pool = std::vector<SAIObjectMapIterOfTypeInShape <CCountedRef>*>();


// (MATT) Iterators now have their destructors called before they enter the pool - so we only need to free the memory here {2008/12/04}
template < template <typename> class T> void DeleteAIObjectMapIter(SAIObjectMapIter<T> *ptr) { operator delete(ptr); }

//===================================================================
// ClearAIObjectIteratorPools
//===================================================================
void ClearAIObjectIteratorPools()
{
	// (MATT) Iterators now have their destructors called before they enter the pool - so we only need to free the memory here {2008/12/04}
  std::for_each(SAIObjectMapIter<CWeakRef>::pool.begin(), SAIObjectMapIter<CWeakRef>::pool.end(), DeleteAIObjectMapIter<CWeakRef>);
  std::for_each(SAIObjectMapIterOfType<CWeakRef>::pool.begin(), SAIObjectMapIterOfType<CWeakRef>::pool.end(), DeleteAIObjectMapIter<CWeakRef>);
  std::for_each(SAIObjectMapIterInRange<CWeakRef>::pool.begin(), SAIObjectMapIterInRange<CWeakRef>::pool.end(), DeleteAIObjectMapIter<CWeakRef>);
  std::for_each(SAIObjectMapIterOfTypeInRange<CWeakRef>::pool.begin(), SAIObjectMapIterOfTypeInRange<CWeakRef>::pool.end(), DeleteAIObjectMapIter<CWeakRef>);
  std::for_each(SAIObjectMapIterInShape<CWeakRef>::pool.begin(), SAIObjectMapIterInShape<CWeakRef>::pool.end(), DeleteAIObjectMapIter<CWeakRef>);
  std::for_each(SAIObjectMapIterOfTypeInShape<CWeakRef>::pool.begin(), SAIObjectMapIterOfTypeInShape<CWeakRef>::pool.end(), DeleteAIObjectMapIter<CWeakRef>);
  SAIObjectMapIter<CWeakRef>::pool.clear();
  SAIObjectMapIterOfType<CWeakRef>::pool.clear();
  SAIObjectMapIterInRange<CWeakRef>::pool.clear();
  SAIObjectMapIterOfTypeInRange<CWeakRef>::pool.clear();
  SAIObjectMapIterInShape<CWeakRef>::pool.clear();
  SAIObjectMapIterOfTypeInShape<CWeakRef>::pool.clear();
}

//====================================================================
// CAISystem
//====================================================================
CAISystem::CAISystem(ISystem *pSystem)
: 
m_pScriptAI(0),
m_pScriptAI_G02(0),
m_pScriptAI_G04(0),
m_nNumPuppets(0),
m_nTickCount(0),
m_pGraph(0),
m_pPathfinder(0),
m_pNavigation(0),
m_bInitialized(false),
m_pSmartObjectManager(NULL),
m_bUpdateSmartObjects(false),
m_pAIActionManager(NULL),
m_BehaviourTreeProfileManager(0),
m_lastAmbientFireUpdateTime(-10.0f),
m_lastVisBroadPhaseTime(-10.0f),
m_lastExpensiveAccessoryUpdateTime(-10.0f),
m_lastGroupUpdateTime(-10.0f),
m_DEBUG_screenFlash(0),
m_IsEnabled(true),
m_enabledPuppetsUpdateError(0),
m_enabledPuppetsUpdateHead(0),
m_totalPuppetsUpdateCount(0),
m_disabledPuppetsUpdateError(0),
m_disabledPuppetsHead(0),
m_pBTProfileDictionary(NULL),
m_pEmotionalSystem(NULL),
m_pScriptBind(NULL),
m_bCodeCoverageFailed(false)
{
	CPerceptionManager::GetInstance();//needed here now?

	IConsole *pConsole = gEnv->pConsole;

	//TODO see if we can init the actual environment yet?
	gAIEnv.CVars.Init(); // CVars need to be init before any logging takes place
	gAIEnv.SignalCRCs.Init(pSystem->GetCrc32Gen());
	pConsole->AddCommand("ai_reload", (ConsoleCommandFunc)ReloadConsoleCommand, VF_CHEAT, "Reload AI system scripts and data");
	pConsole->AddCommand("ai_start_coordination", (ConsoleCommandFunc)StartCoordinationConsoleCommand, VF_CHEAT, "Starts the specified coordination.\nUsage: ai_start_coordination groupId coordinationName");
	pConsole->AddCommand("ai_CheckGoalpipes", (ConsoleCommandFunc)CheckGoalpipes, VF_CHEAT, "Checks goalpipes and dumps report to console.");
	pConsole->AddCommand("ai_dumpCheckpoints", (ConsoleCommandFunc)DumpCodeCoverageCheckpoints, VF_CHEAT, "Dump CodeCoverage checkpoints to file");
	pConsole->AddCommand("ai_Recorder_Start", (ConsoleCommandFunc)StartAIRecorder, VF_CHEAT, "Reset and start the AI Recorder on demand");
	pConsole->AddCommand("ai_Recorder_Stop", (ConsoleCommandFunc)StopAIRecorder, VF_CHEAT, "Stop the AI Recorder. If logging in memory, saves it to disk.");

	gAIEnv.pGraph = m_pGraph = new CGraph(this);
	gAIEnv.pNavigation = m_pNavigation = new CNavigation (this, pSystem);
	gAIEnv.pPathfinder = m_pPathfinder = new CPathfinder (this);

#ifdef CRYAISYSTEM_DEBUG
	m_Recorder.Init();
#endif //CRYAISYSTEM_DEBUG

	// Don't Init() here! It will be called later after creation of the Entity System
//	Init();
}
//
//-----------------------------------------------------------------------------------------------------------
CAISystem::~CAISystem()
{
	if (GetISystem() && GetISystem()->GetISystemEventDispatcher())
		GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

	Reset(IAISystem::RESET_INTERNAL);

	m_visionMap.Destroy();

	SAFE_DELETE(m_pScriptBind);
	SAFE_DELETE( m_pBTProfileDictionary );
	SAFE_DELETE( m_pEmotionalSystem );

	m_PipeManager.ClearAllGoalPipes();

	// (MATT) Note that we need to later trigger the object tracker to release all the objects {2009/03/25} 
	m_Objects.clear();

	m_mapSpecies.clear();
	m_mapGroups.clear();
	m_mapDummyObjects.clear();
	m_mapBeacons.clear();

	for(AIGroupMap::iterator it = m_mapAIGroups.begin(); it != m_mapAIGroups.end(); ++it)
		delete it->second;
	m_mapAIGroups.clear();

	m_pNavigation->ShutDown();

	if (gAIEnv.pTargetTrackManager)
		gAIEnv.pTargetTrackManager->Shutdown();

	delete m_pSmartObjectManager;
	delete m_pAIActionManager;

	// Release fire command factory.
	for(std::vector<IFireCommandDesc*>::iterator it = m_firecommandDescriptors.begin(); it != m_firecommandDescriptors.end(); ++it)
		(*it)->Release();
	m_firecommandDescriptors.clear();
	
	SAFE_RELEASE(m_pScriptAI);
	SAFE_RELEASE(m_pScriptAI_G02);
	SAFE_RELEASE(m_pScriptAI_G04);

	// (MATT) Flush all the objects that have been deregistered.
	// Really, this should delete all AI objects regardless {2009/03/27}
	gAIEnv.pObjectContainer->ReleaseDeregisteredObjects();

	delete m_pPathfinder;
	m_pPathfinder = 0;

	delete m_pNavigation;
	m_pNavigation = 0;

	delete m_pGraph;
	m_pGraph = 0;

	delete gAIEnv.pCoordinationManager;
	delete gAIEnv.pCommunicationManager;
	delete gAIEnv.pCoverSystem;
	delete gAIEnv.pSelectionTreeManager;
	delete gAIEnv.pRayCaster;
	delete gAIEnv.pIntersectionTester;

	delete m_BehaviourTreeProfileManager;

	CPerceptionManager::DestroyInstance();
}

//-----------------------------------------------------------------------------------------------------------

bool CAISystem::Init()
{
	AILogProgress("[AISYSTEM] Initialization started.");

	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

	SetupAIEnvironment();

	// (MATT) This has to go here because the ScriptBind_AI code causes an actual Lua script
	//        reload - but we should change that ASAP {2008/05/19}
	m_pScriptBind = new CScriptBind_AI2();

	if (!m_pScriptAI)
		m_pScriptAI = new CScriptBind_AI();
	
	// These should depend on the GameDLL used, and then they should move into GameDLL entirely!
	if (!m_pScriptAI_G02)
		m_pScriptAI_G02 = new CScriptBind_AI_G02();
	if (!m_pScriptAI_G04)
		m_pScriptAI_G04 = new CScriptBind_AI_G04();

	gAIEnv.pGroupSystem->Init();

	Reset(IAISystem::RESET_INTERNAL);

	m_pNavigation->Init();

	m_nTickCount = 0;
	gAIEnv.pWorld = gEnv->pPhysicalWorld;
	
	m_fFrameStartTime = gEnv->pTimer->GetFrameStartTime();
	m_fLastPuppetUpdateTime = m_fFrameStartTime;
	m_fFrameDeltaTime = 0.0f;

	// Register fire command factories.
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("instant", CFireCommandInstant));
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("instant_single", CFireCommandInstantSingle));
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("projectile_slow", CFireCommandProjectileSlow));
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("projectile_fast", CFireCommandProjectileFast));
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("strafing", CFireCommandStrafing));
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("hurricane", CFireCommandHurricane));
	// TODO: move this to game.dll
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("fast_light_moar", CFireCommandFastLightMOAR));
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("hunter_moar", CFireCommandHunterMOAR));
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("hunter_sweep_moar", CFireCommandHunterSweepMOAR));
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("hunter_singularity_cannon", CFireCommandHunterSingularityCannon));
	RegisterFirecommandHandler(CREATE_FIRECOMMAND_DESC("grenade", CFireCommandGrenade));

	m_bInitialized = true;

	// This should become an optional module in the gAIEnvironment
	// If it fails to load initialisation data, delete it again, obviously not needed
	m_pEmotionalSystem = new CEmotionalSystem;
	if (!m_pEmotionalSystem->Init())
		SAFE_DELETE(m_pEmotionalSystem);

	m_pBTProfileDictionary = new CProfileDictionary;
	m_pBTProfileDictionary->LoadData();

	m_pCoopReadabilitiesSystem = CCoopReadabilitiesSystem::GetInstance();	  
	m_pCoopReadabilitiesSystem->Init();
	m_pCoopReadabilitiesSystem->LoadData();

	if (gAIEnv.pTargetTrackManager)
		gAIEnv.pTargetTrackManager->Init();

	gAIEnv.pCoordinationManager->ScanFolder("Scripts/AI/Coordination");
	gAIEnv.pSelectionTreeManager->ScanFolder("Scripts/AI/SelectionTrees");

	// This does not work because at this point, the FlowSystem is not created yet
	//InitSmartObjects();//Try it out here! ->seems to work; TODO remove other calls and just put function content here

	m_visionMap.Init();

	AILogProgress("[AISYSTEM] Initialization finished.");

	m_BehaviourTreeProfileManager = new NewBehaviourTree::TreeProfileManager();
	m_BehaviourTreeProfileManager->LoadProfiles(TREE_PROFILES_BASEDIR);

	return true;
}

void CAISystem::SetAIworkaroundsConfiguration()
{
	/////////////////////////////////////////////////////////////
	//TODO This is workaround support and should go away at some point!
	// Set the compatibility mode for feature/setting emulation
	const char* sValue = gAIEnv.CVars.CompatibilityMode;
	EConfigCompatabilityMode eCompatMode = ECCM_NONE;
	if (strcmpi("crysis",sValue) == 0) 
		eCompatMode = ECCM_CRYSIS;
	if (strcmpi("game04",sValue) == 0) 
		eCompatMode = ECCM_GAME04;
	if (strcmpi("warface",sValue) == 0)
		eCompatMode = ECCM_WARFACE;
	if (strcmpi("crysis2",sValue) == 0) 
		eCompatMode = ECCM_CRYSIS2;
	gAIEnv.configuration.eCompatibilityMode = eCompatMode;
	/////////////////////////////////////////////////////////////
}

#include "GoalOpFactory.h"	//TODO move this to top!
#include "StatsManager.h"	//TODO move this to top!
#include "GameSpecific/GoalOp_G02.h"		//TODO move these out of AISystem
#include "GameSpecific/GoalOp_G04.h" 		//TODO move these out of AISystem
#include "GameSpecific/GoalOp_Crysis2.h" 	//TODO move these out of AISystem
void CAISystem::SetupAIEnvironment()
{
	SetAIworkaroundsConfiguration();

	//TODO make these members of CAISystem that are pointed to, NOT allocated!

	gAIEnv.pPipeManager = &m_PipeManager;

	// GoalOpFactory
	if (!gAIEnv.pGoalOpFactory) 
	{
		CGoalOpFactoryOrdering * pFactoryOrdering = new CGoalOpFactoryOrdering();
		gAIEnv.pGoalOpFactory = pFactoryOrdering;

		//TODO move these out of AISystem, to be added thru interface function from game side
		pFactoryOrdering->AddFactory( new CGoalOpFactoryCrysis2 );
		pFactoryOrdering->AddFactory( new CGoalOpFactoryG04 );
		pFactoryOrdering->AddFactory( new CGoalOpFactoryG02 );
	}

	// ObjectContainer
	if (!gAIEnv.pObjectContainer)
		gAIEnv.pObjectContainer = new CObjectContainer;

	// CodeCoverage
	if (!gAIEnv.pCodeCoverageTracker)
		gAIEnv.pCodeCoverageTracker = new CCodeCoverageTracker;
	if (!gAIEnv.pCodeCoverageManager)
		gAIEnv.pCodeCoverageManager = new CCodeCoverageManager;
	if (!gAIEnv.pCodeCoverageGUI)
		gAIEnv.pCodeCoverageGUI = new CCodeCoverageGUI;

	// StatsManager
	if (!gAIEnv.pStatsManager)
		gAIEnv.pStatsManager = new CStatsManager;

	if (!gAIEnv.pTacticalPointSystem)
		gAIEnv.pTacticalPointSystem = new CTacticalPointSystem();

	if (!gAIEnv.pTargetTrackManager)
		gAIEnv.pTargetTrackManager = new CTargetTrackManager();

	// GroupSystem
	if (!gAIEnv.pGroupSystem)
		gAIEnv.pGroupSystem = new CGroupSystem;

	gAIEnv.pAIActionManager = m_pAIActionManager = new CAIActionManager();
	gAIEnv.pSmartObjectManager = m_pSmartObjectManager = new CSmartObjectManager();

	gAIEnv.pCoordinationManager = new CCoordinationManager();
	gAIEnv.pCommunicationManager = new CCommunicationManager();
	gAIEnv.pCoverSystem = new CCoverSystem();
	gAIEnv.pSelectionTreeManager = new CSelectionTreeManager();

	gAIEnv.pRayCaster = new SAIEnvironment::GlobalRayCaster;
	gAIEnv.pRayCaster->SetQuota(32);

	gAIEnv.pIntersectionTester = new SAIEnvironment::GlobalIntersectionTester;
	gAIEnv.pIntersectionTester->SetQuota(64);
}

//
//-----------------------------------------------------------------------------------------------------------
bool CAISystem::InitSmartObjects()
{
	AILogProgress("[AISYSTEM] Initializing AI Actions.");
	m_pAIActionManager->LoadLibrary( AI_ACTIONS_PATH );

	AILogProgress("[AISYSTEM] Initializing Smart Objects.");
	gEnv->pEntitySystem->AddSink( m_pSmartObjectManager );
	m_pSmartObjectManager->LoadSmartObjectsLibrary();

	return true;
}

//====================================================================
// Reload
//====================================================================
void CAISystem::Reload()
{
	// Clear out any state that would otherwise persist over a reload (e.g. goalpipes)
	ClearForReload();

	gAIEnv.pCoordinationManager->Reload();
	gAIEnv.pCommunicationManager->Reload();
	gAIEnv.pSelectionTreeManager->Reload();

	// Reload the root of the AI system scripts, forcing reload of this and all dependencies
	IScriptSystem *pSS = gEnv->pScriptSystem;
	CRY_ASSERT(pSS);
	if (pSS->ExecuteFile("Scripts/AI/aiconfig.lua", true, true))
	{
		// Reset blackboards etc.
		pSS->BeginCall("AIReset");
		pSS->EndCall();

		// Reload goal pipes, queries etc.
		pSS->BeginCall("AIReload");
		pSS->EndCall();
	}

	if (gAIEnv.pTargetTrackManager)
		gAIEnv.pTargetTrackManager->ReloadConfig();
}

//====================================================================
// SetLevelPath
//====================================================================
void CAISystem::SetLevelPath(const char *sPath)
{
	m_sWorkingFolder = sPath;
}

//====================================================================
// RegisterListener
//====================================================================
bool CAISystem::RegisterListener(IAISystemListener* pListener)
{
	bool result;
	result = m_setSystemListeners.insert(pListener).second;
	return result;
}

//====================================================================
// UnregisterListener
//====================================================================
bool CAISystem::UnregisterListener(IAISystemListener* pListener)
{
	int nErased;
	nErased = m_setSystemListeners.erase(pListener);
	return (nErased == 1);
}

//====================================================================
// OnAgentDeath
//====================================================================
void CAISystem::OnAgentDeath(EntityId deadEntityID)
{
	BroadcastToListeners(OnAgentDeath(deadEntityID));
}

//====================================================================
// GetUpdateInterval
//====================================================================
float CAISystem::GetUpdateInterval() const 
{
	if (gAIEnv.CVars.AIUpdateInterval)
		return gAIEnv.CVars.AIUpdateInterval;
	else
		return 0.1f;
}

//====================================================================
// InvalidatePathsThroughArea
//====================================================================
void CAISystem::InvalidatePathsThroughArea(const ListPositions& areaShape)
{
	for (AIObjectOwners::iterator it = m_Objects.begin() ; it != m_Objects.end() ; ++it)
	{
		CAIObject* pObject = it->second.GetAIObject();
		CPipeUser* pPiper = pObject->CastToCPipeUser();
		if (pPiper)
		{
			const CNavPath & path = pPiper->m_Path;
			const TPathPoints & listPath = path.GetPath();
			Vec3 pos = pPiper->GetPhysicsPos();
			for (TPathPoints::const_iterator pathIt = listPath.begin() ; pathIt != listPath.end() ; ++pathIt)
			{
				Lineseg pathSeg(pos, pathIt->vPos);
				pos = pathIt->vPos;
				if (Overlap::Lineseg_Polygon2D(pathSeg, areaShape))
				{
					pPiper->PathIsInvalid();
					break;
				}
			}
		}
	} // loop over objects
}


struct SSortedPuppetA
{
	SSortedPuppetA(CPuppet* o, float w) : obj(o), weight(w) {}
	bool	operator<(const SSortedPuppetA& rhs) const { return weight < rhs.weight;	}
	bool	operator>(const SSortedPuppetA& rhs) const	{ return weight > rhs.weight;	}

	float			weight;
	CPuppet*	obj;
};

//
//-----------------------------------------------------------------------------------------------------------
int CAISystem::GetAlertness() const
{
	int maxAlertness = 0;
	for (int i = NUM_ALERTNESS_COUNTERS-1; i; i--)
		if (m_AlertnessCounters[i])
		{
			maxAlertness = i;
			break;
		}
	return	maxAlertness;
}

//
//-----------------------------------------------------------------------------------------------------------
int CAISystem::GetAlertness(const IAIAlertnessPredicate& alertnessPredicate)
{
	int maxAlertness = 0;

	AutoAIObjectIter pIter = GetFirstAIObject(OBJFILTER_TYPE, AIOBJECT_PUPPET);
	while (IAIObject *pObject = pIter->GetObject())
	{
		IAIActorProxy *pProxy = pObject->GetProxy();
		if (pProxy && alertnessPredicate.ConsiderAIObject(pObject))
		{
			maxAlertness = max(maxAlertness, pProxy->GetAlertnessState());
		}

		pIter->Next();
	}

	return maxAlertness;
}

//
//-----------------------------------------------------------------------------------------------------------
IAIObject *CAISystem::CreateAIObject(unsigned short type, IAIObject *pAssociation, EntityId entityID)
{
	CCCPOINT(CreateAIObject);

	if(!IsEnabled())
		return 0;

	if (!m_bInitialized)
	{
		AIError("CAISystem::CreateAIObject called on an uninitialized AI system [Code bug]");
		return 0;
	}

	CAIObject *pObject;
	CLeader *pLeader;

	switch (type)
	{
	case AIOBJECT_PUPPET:
		pObject = new CPuppet;
		break;
	case AIOBJECT_2D_FLY:
		type = AIOBJECT_PUPPET;
		pObject = new CPuppet;
		pObject->SetSubType(CAIObject::STP_2D_FLY);
		break;
	case AIOBJECT_LEADER:
		{
			int iGroup = pAssociation ? static_cast< CPuppet* >(pAssociation)->GetGroupId() : -1;
			pLeader = new CLeader(iGroup);
			pObject = pLeader;
		}
		break;
	case AIOBJECT_BOAT:
		type = AIOBJECT_VEHICLE;
		pObject = new CAIVehicle;
		pObject->SetSubType(CAIObject::STP_BOAT);
		//				pObject->m_bNeedsPathOutdoor = false;
		break;
	case AIOBJECT_CAR:
		type = AIOBJECT_VEHICLE;
		pObject = new CAIVehicle;
		pObject->SetSubType(CAIObject::STP_CAR);
		break;
	case AIOBJECT_HELICOPTER:
		type = AIOBJECT_VEHICLE;
		pObject = new CAIVehicle;
		pObject->SetSubType(CAIObject::STP_HELI);
		break;
	case AIOBJECT_PLAYER:
			pObject = new CAIPlayer; // just a dummy for the player	
			pObject->Event(AIEVENT_ENABLE, NULL);
		break;
	case AIOBJECT_RPG:
	case AIOBJECT_GRENADE:
		pObject = new CAIObject;
		break;
	case AIOBJECT_WAYPOINT:
	case AIOBJECT_HIDEPOINT:
	case AIOBJECT_SNDSUPRESSOR:
		pObject = new CAIObject;
		break;
	default:
		// try to create an object of user defined type
		pObject = new CAIObject;
		break;
	}
	
	// Register the object
	CStrongRef< CAIObject > object;
	gAIEnv.pObjectContainer->RegisterObject( pObject, object );

	// insert object into map under key type
	// this is a multimap
	m_Objects.insert(AIObjectOwners::iterator::value_type(type,object));
	unsigned int nNumPuppets = m_Objects.count(AIOBJECT_PUPPET);
	nNumPuppets += m_Objects.count(AIOBJECT_VEHICLE);
	m_nNumPuppets = nNumPuppets;

	// Reset the object after registration, so other systems can reference back to it if needed
	pObject->SetType(type);
	pObject->SetEntityID(entityID);
	pObject->SetAssociation(GetWeakRef((CAIObject*)pAssociation));
	pObject->Reset(AIOBJRESET_INIT);

	// Non-puppets and players need to be updated at least once for delayed initialization (Reset sets this to false!)
	if ( type != AIOBJECT_PUPPET && type!=AIOBJECT_PLAYER) //&& type != AIOBJECT_VEHICLE )
		pObject->m_bUpdatedOnce = true;

	if ((type == AIOBJECT_LEADER) && pAssociation && pLeader)
		AddToGroup(pLeader);

	AILogComment("CAISystem::CreateAIObject %p of type %d", pObject, type);

	return pObject;
}

// Get an AI object by it's AI object ID
IAIObject *CAISystem::GetAIObject(tAIObjectID aiObjectID)
{
	CWeakRef<CAIObject> ref = gAIEnv.pObjectContainer->GetWeakRef(aiObjectID);
	return ref.GetAIObject();
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::ClearForReload(void)
{
	m_PipeManager.ClearAllGoalPipes();
	gAIEnv.pTacticalPointSystem->DestroyAllQueries();
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::ReloadConsoleCommand( IConsoleCmdArgs * pArgs )
{
	// Needs to be static function to be registered as console command
	// The arguments are not used
	GetAISystem()->Reload();
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::StartCoordinationConsoleCommand( IConsoleCmdArgs *pArgs )
{
	if (pArgs->GetArgCount() < 2)
		return;

	const char* group = pArgs->GetArg(1);
	const char* coordName = pArgs->GetArg(2);

	int groupId = atoi(group);
	if (groupId < 1)
		return;

	CoordinationInternalStartParams params;

	int groupCount = GetAISystem()->GetGroupCount(groupId, 0, 0); 

	for (int i = 0; i < groupCount; ++i)
	{
		if (IAIObject* pAIObject = GetAISystem()->GetGroupMember(groupId, i, 0, 0))
			params.actors.push_back(CoordinationActor(pAIObject));
	}

	gAIEnv.pCoordinationManager->StartCoordination(gAIEnv.pCoordinationManager->GetCoordinationSetupID(coordName), params);
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::StartAIRecorder( IConsoleCmdArgs * pArgs )
{
#ifdef CRYAISYSTEM_DEBUG

	if (pArgs->GetArgCount() < 2)
	{
		gEnv->pAISystem->Warning("<StartAIRecorder> ", "Expecting record mode as argument (1 == Memory, 2 == Disk)");
		return;
	}

	char const* szMode = pArgs->GetArg(1);
	char const* szFile = pArgs->GetArgCount() > 2 ? pArgs->GetArg(2) : NULL;

	EAIRecorderMode mode = eAIRM_Off;
	if (strcmp(szMode,"1") == 0)
		mode = eAIRM_Memory;
	else if (strcmp(szMode,"2") == 0)
		mode = eAIRM_Disk;

	string sRealFile;
	if (szFile)
		sRealFile = PathUtil::Make("", szFile, "rcd");

	if (mode > eAIRM_Off)
	{
		CAISystem *pAISystem = GetAISystem();
		pAISystem->m_Recorder.Reset();
		pAISystem->m_Recorder.Start(mode, sRealFile.c_str());
	}

#endif //CRYAISYSTEM_DEBUG
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::StopAIRecorder( IConsoleCmdArgs * pArgs )
{
#ifdef CRYAISYSTEM_DEBUG

	char const* szFile = pArgs->GetArgCount() > 1 ? pArgs->GetArg(1) : NULL;

	string sRealFile;
	if (szFile)
		sRealFile = PathUtil::Make("", szFile, "rcd");

	GetAISystem()->m_Recorder.Stop(sRealFile.c_str());

#endif //CRYAISYSTEM_DEBUG
}

//====================================================================
// CheckUnusedGoalpipes
//====================================================================
void CAISystem::CheckGoalpipes(IConsoleCmdArgs *)
{
	GetAISystem()->m_PipeManager.CheckGoalpipes();
}

void CAISystem::DumpCodeCoverageCheckpoints(IConsoleCmdArgs *pArgs)
{
	if (gAIEnv.pCodeCoverageManager)
	{
		// Any second argument triggers dump to file rather than log
		bool bLogToFile = (pArgs->GetArgCount() > 1);
		gAIEnv.pCodeCoverageManager->DumpCheckpoints(bLogToFile);
	}
}

//
//-----------------------------------------------------------------------------------------------------------
CAIObject * CAISystem::GetPlayer() const
{
	AIObjectOwners::const_iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PLAYER)) != m_Objects.end())
		return ai->second.GetAIObject();
	return 0;
}

//
//-----------------------------------------------------------------------------------------------------------
IAIObject * CAISystem::GetAIObjectByName(unsigned short type, const char *pName) const
{
	AIObjectOwners::const_iterator ai;

	if (m_Objects.empty()) return 0;

	if (!type) 
		return (IAIObject *) GetAIObjectByName(pName);

  if ((ai = m_Objects.find(type)) != m_Objects.end())
  {
    for (;ai!=m_Objects.end();)
    {
      if (ai->first != type)
        break;
      CAIObject *pObject = ai->second.GetAIObject();

      if (strcmp(pName, pObject->GetName()) == 0)
        return (IAIObject *) pObject;
      ++ai;
    }
  }
	return 0;

}


//
//-----------------------------------------------------------------------------------------------------------
CAIObject * CAISystem::GetAIObjectByName(const char *pName) const
{
	AIObjectOwners::const_iterator ai = m_Objects.begin();
	while (ai!=m_Objects.end())
	{
		CAIObject *pObject = ai->second.GetAIObject();

		if (pObject && (strcmp(pName, pObject->GetName()) == 0))
			return pObject;
		++ai;
	}

	// Try dummy object map as well
	AIObjects::const_iterator aiDummy = m_mapDummyObjects.begin();
	while (aiDummy!=m_mapDummyObjects.end())
	{
		CAIObject *pObject = aiDummy->second.GetAIObject();

		if (pObject && (strcmp(pName, pObject->GetName()) == 0))
			return pObject;
		++aiDummy;
	}

	return 0;
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::CreateDummyObject(CStrongRef<CAIObject>& ref, const string &name, CAIObject::ESubTypes type)
{
	CCCPOINT(CreateDummyObject);

	CAIObject *pObject = new CAIObject;
	gAIEnv.pObjectContainer->RegisterObject(pObject, ref);

	pObject->SetType(AIOBJECT_DUMMY);
	pObject->SetSubType(type);

	pObject->SetAssociation(NILREF);
	if(!name.empty() && name.length() != 0)
		pObject->SetName(name);

	// check whether it was added before
	AIObjects::iterator itEntry = m_mapDummyObjects.find(type);
	while ((itEntry != m_mapDummyObjects.end()) && (itEntry->first == type))
	{
		if (itEntry->second == ref)
			return;
		++itEntry;
	}

	// make sure it is not in with another types already
	itEntry = m_mapDummyObjects.begin();
	for (; itEntry != m_mapDummyObjects.end(); ++itEntry)
	{
		if (itEntry->second == ref)
		{
			m_mapDummyObjects.erase(itEntry);
			break;
		}
	}

	// insert object into map under key type
	m_mapDummyObjects.insert(AIObjects::iterator::value_type(type,ref));

	AILogComment("CAISystem::CreateDummyObject %s (%p)", pObject->GetName(), pObject);
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::CreateDummyObject(CCountedRef<CAIObject>& ref, const string &name, CAIObject::ESubTypes type)
{
	CStrongRef<CAIObject> refTemp;
	CreateDummyObject(refTemp, name, type);
	ref = refTemp;
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::RemoveObject(CAIObject *pObject)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI );
	CCCPOINT(RemoveObject);

	
	if (!pObject)
		return;

	RemoveObjectFromAllOfType(AIOBJECT_PUPPET,pObject);
	RemoveObjectFromAllOfType(AIOBJECT_VEHICLE,pObject);
	RemoveObjectFromAllOfType(AIOBJECT_ATTRIBUTE,pObject);
	RemoveObjectFromAllOfType(AIOBJECT_LEADER,pObject);
				 
	// (MATT) Remove from player - especially as attention target {2009/02/05}
	CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
	if(pPlayer)
		pPlayer->OnObjectRemoved(pObject);

	for(AIGroupMap::iterator it = m_mapAIGroups.begin(); it != m_mapAIGroups.end(); ++it)
		it->second->OnObjectRemoved(pObject);

	AIObjects::iterator	oi;
	for (oi=m_mapSpecies.begin();oi!=m_mapSpecies.end();++oi)
	{
		if (oi->second == pObject)
		{
			m_mapSpecies.erase(oi);
			break;
		}
	}

	for (oi=m_mapGroups.begin();oi!=m_mapGroups.end();++oi)
	{
		if (oi->second == pObject)
		{
			m_mapGroups.erase(oi);
			break;
		}
	}

	for (oi=m_mapDummyObjects.begin();oi!=m_mapDummyObjects.end();++oi)
	{
		if (oi->second == pObject)
		{
			m_mapDummyObjects.erase(oi);
			break;
		}
	}

	CLeader* pLeader = pObject->CastToCLeader();
	if(pLeader)
		pLeader->GetAIGroup()->SetLeader(0);

	FormationMap::iterator fi;
	for (fi=m_mapActiveFormations.begin();fi!=m_mapActiveFormations.end();++fi)
		fi->second->OnObjectRemoved(pObject);

	//remove this object from any pending paths generated for him
	CPuppet* pPuppet = pObject->CastToCPuppet();
	if (pPuppet)
		m_pPathfinder->CancelAnyPathsFor(pPuppet, true);	// Remove from actor and raw path requests.


	// (MATT) Try to do implicitly {2009/02/05}
	/*
	// check if this object owned any beacons and remove them if so
	if (!m_mapBeacons.empty())
	{
		BeaconMap::iterator bi,biend = m_mapBeacons.end();
		for (bi=m_mapBeacons.begin();bi!=biend;)
		{
			if ((bi->second).pOwner == pObject)
			{
				BeaconMap::iterator eraseme = bi;
				++bi;
				RemoveObject((eraseme->second).pBeacon);
				m_mapBeacons.erase(bi);
			}
			else
				++bi;
		}
	}
	*/

	m_lightManager.OnObjectRemoved(pObject);

	CPerceptionManager::GetInstance()->OnObjectRemoved(pObject);

	if (gAIEnv.pGroupSystem)
		gAIEnv.pGroupSystem->OnObjectRemoved(pObject);

	if (gAIEnv.pTargetTrackManager)
		gAIEnv.pTargetTrackManager->OnObjectRemoved(pObject);

	if (pPuppet)
	{
		for (unsigned i = 0; i < m_delayedExpAccessoryUpdates.size(); )
		{
			if (m_delayedExpAccessoryUpdates[i].pPuppet = pPuppet)
			{
				m_delayedExpAccessoryUpdates[i] = m_delayedExpAccessoryUpdates.back();
				m_delayedExpAccessoryUpdates.pop_back();
			}
			else
				++i;
		}
	}

	// Refresh number of puppets
	m_nNumPuppets = m_Objects.count(AIOBJECT_PUPPET);
	m_nNumPuppets += m_Objects.count(AIOBJECT_VEHICLE);
}

void CAISystem::RemoveObject(tAIObjectID objectID)
{
	// Find the element in the owners list and erase it from there
	// This is strong, so will trigger removal/deregister/release in normal fashion
	// (MATT) Really not very efficient because the multimaps aren't very suitable for this. 
	// I think a different container might be better as primary owner. {2009/05/22}
	AIObjectOwners::iterator it = m_Objects.begin();
	AIObjectOwners::iterator itEnd = m_Objects.end();
	for ( ; it != itEnd; it++ )
		if (it->second.GetObjectID() == objectID)
			break;

	// Check we found one
	if (it == itEnd)
	{
		AIError("AI system asked to erase AI object with unknown AIObjectID");
		assert(false);
		return;
	}

	m_Objects.erase(it);

	// Because Action doesn't yet handle a delayed removal of the Proxies, we should perform cleanup immediately.
	// Note that this only happens when triggered externally, when an entity is refreshed/removed
	gAIEnv.pObjectContainer->ReleaseDeregisteredObjects();
}
// Sends a signal using the desired filter to the desired agents
//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::SendSignal(unsigned char cFilter, int nSignalId,const char *szText, IAIObject *pSenderObject, IAISignalExtraData* pData)
{
	// (MATT) This is quite a switch statement. Needs replacing. {2009/02/11}
	CCCPOINT(CAISystem_SendSignal);
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	// This deletes the passed pData parameter if it wasn't set to NULL
	struct DeleteBeforeReturning
	{
		IAISignalExtraData** _p;
		DeleteBeforeReturning( IAISignalExtraData** p ) : _p(p) {}
		~DeleteBeforeReturning()
		{ 
			if ( *_p )
				delete (AISignalExtraData*)*_p;
		}
	} autoDelete( &pData );

	
	// Calling this with no senderObject is an error
	assert(pSenderObject);

	CAIActor* pSender = CastToCAIActorSafe(pSenderObject);
	//filippo: can be that sender is null, for example when you send this singal in multiplayer.
	if(!pSender)
		return;

	float fRange = pSender->GetParameters().m_fCommRange;
	fRange*=pSender->GetParameters().m_fCommRange;
	Vec3 pos = pSender->GetPos();
	IEntity	*pSenderEntity(pSender->GetEntity());

	CWeakRef<CAIObject> refSender = GetWeakRefSafe(pSender);

	switch (cFilter)
	{

		case SIGNALFILTER_SENDER:
		{
			pSender->SetSignal(nSignalId, szText, pSenderEntity, pData);
			pData = NULL;
			break;
		}
		case SIGNALFILTER_READABILITYAT:
		case SIGNALFILTER_READABILITY:
		case SIGNALFILTER_READABILITYRESPONSE:
		{
			// Check if someone in the group is already playing this readability, and prevent it. Skip responses.
			// Priorities over 100 are always played.
			bool highPriority = pData && pData->iValue >= 100;
			if (cFilter != SIGNALFILTER_READABILITYRESPONSE && !highPriority)
			{
				// Get the duration of blocking from the readability manager.
				IAIActorProxy *pAIActorProxy = pSender->GetProxy();
				if(pAIActorProxy)
				{
					int groupId = pSender->GetGroupId();
					IAIGroup* pGroup = GetIAIGroup(groupId);
					float	t;
					int	id;
					pAIActorProxy->GetReadabilityBlockingParams(szText, t, id);
					if(pGroup && !pGroup->RequestReadabilitySound(id, t))
					{
						string	name(szText);
						name += "[blocked]";
						Record(pSender, IAIRecordable::E_SIGNALRECIEVEDAUX, name.c_str());	
						IAIRecordable::RecorderEventData recorderEventData(name.c_str());
						pSender->RecordEvent(IAIRecordable::E_SIGNALRECIEVEDAUX, &recorderEventData);
						return;
					}
				}
			}


			unsigned int groupid = pSender->GetGroupId();
			AuxSignalDesc asd;
			asd.fTimeout = 5.f;
			asd.strMessage = string(szText);
			m_mapAuxSignalsFired.insert(MapSignalStrings::iterator::value_type(groupid,asd));

			pSender->m_State.nAuxSignal = cFilter;
			pSender->m_State.szAuxSignalText = szText;
			if( pData )
			{
				pSender->m_State.nAuxPriority = pData->iValue;
				pSender->m_State.fAuxDelay = pData->fValue;
			}
			else
			{
				pSender->m_State.nAuxPriority = 0;
				pSender->m_State.fAuxDelay = 0;
			}
			Record(pSender, IAIRecordable::E_SIGNALRECIEVEDAUX, szText);	
			IAIRecordable::RecorderEventData recorderEventData(szText);
			pSender->RecordEvent(IAIRecordable::E_SIGNALRECIEVEDAUX, &recorderEventData);

			break;
		}
		case SIGNALFILTER_HALFOFGROUP:
			{
				CCCPOINT(CAISystem_SendSignal_HALFOFGROUP);

				int groupid = pSender->GetGroupId();
				AIObjects::iterator ai;
		
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					int groupmembers=m_mapGroups.count(groupid);
					groupmembers/=2;

					for (;ai!=m_mapGroups.end();)
					{
						if ( (ai->first != groupid) || (!groupmembers--) )
							break;
						
						// Object may have been removed
						CAIActor* pReciever = CastToCAIActorSafe(ai->second.GetAIObject());
						if (pReciever && pReciever != pSenderObject)
						{
							pReciever->SetSignal(nSignalId,szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
						}
						else
							++groupmembers;	// dont take into account sender
						++ai;
					}
					// don't delete pData!!! - it will be deleted at the end of this function
				}
			}
			break;
		case SIGNALFILTER_NEARESTGROUP:
			{
				int groupid = pSender->GetGroupId();
				AIObjects::iterator ai;
				CAIActor *pNearest= 0;
				float mindist = 2000;
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					for (;ai!=m_mapGroups.end(); ++ai)
					{
						CAIObject *pObject = ai->second.GetAIObject();
						if (ai->first != groupid)
							break;
						if (!pObject) 
							continue;
						if (pObject != pSenderObject)
						{
							float dist = (pObject->GetPos() - pSender->GetPos()).GetLength();
							if (dist < mindist)
							{
								pNearest = pObject->CastToCAIActor();
								if(pNearest)
								mindist = (pObject->GetPos() - pSender->GetPos()).GetLength();
							}
						}
					}

					if (pNearest)
					{
						pNearest->SetSignal(nSignalId,szText,pSenderEntity,pData);
						pData = NULL;
					}
				}
			}
			break;
			 
		case SIGNALFILTER_NEARESTINCOMM:
			{
				int groupid = pSender->GetGroupId();
				AIObjects::iterator ai;
				CAIActor *pNearest= 0;
				float mindist = pSender->GetParameters().m_fCommRange;
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					for (;ai!=m_mapGroups.end();)
					{
						CAIObject *pObject = ai->second.GetAIObject();
						if (ai->first != groupid)
							break;
						if (!pObject) 
							continue;
						if (pObject != pSenderObject)
						{
							float dist = (pObject->GetPos() - pSender->GetPos()).GetLength();
							if (dist < mindist)
							{
								pNearest = pObject->CastToCAIActor();
								if(pNearest)
								mindist = (pObject->GetPos() - pSender->GetPos()).GetLength();
							}
						}
						++ai;
					}

					if (pNearest)
					{
						pNearest->SetSignal(nSignalId,szText,pSenderEntity,pData);
						pData = NULL;
					}
				}
			}
			break;
		case SIGNALFILTER_NEARESTINCOMM_LOOKING:
			{
				int groupid = pSender->GetGroupId();
				AIObjects::iterator ai;
				CAIActor *pSenderActor = pSenderObject->CastToCAIActor();
				CAIActor *pNearest = 0;
				float mindist = pSender->GetParameters().m_fCommRange;
				float	closestDist2(std::numeric_limits<float>::max());
				if ( pSenderActor && (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					for (;ai!=m_mapGroups.end();)
					{
						CAIObject *pObject = ai->second.GetAIObject();
						if (ai->first != groupid)
							break;
						if (!pObject) 
							continue;
						if (pObject != pSenderObject)
						{
							CPuppet *pCandidatePuppet=pObject->CastToCPuppet();
							float dist = (pObject->GetPos() - pSender->GetPos()).GetLength();
							if (pCandidatePuppet && dist < mindist)
								if (CheckVisibilityToBody(pCandidatePuppet, pSenderActor, closestDist2))
									pNearest = pCandidatePuppet;
						}
						++ai;
					}
					if (pNearest)
					{
						pNearest->SetSignal(nSignalId,szText,pSenderEntity,pData);
						pData = NULL;
					}
				}
			}
			break;
		case SIGNALFILTER_NEARESTINCOMM_SPECIES:
			{
				int species = pSender->GetParameters().m_nSpecies;
				AIObjects::iterator ai;
				CAIActor *pNearest= 0;
				float mindist = pSender->GetParameters().m_fCommRange;
				if ( (ai=m_mapSpecies.find(species)) != m_mapSpecies.end() )
				{
					for (;ai!=m_mapSpecies.end();)
					{
						CAIObject *pObject = ai->second.GetAIObject();
						if (ai->first != species)
							break;
						if (!pObject) 
							continue;
						if (pObject != pSenderObject && pObject->IsEnabled())
						{
							float dist = (pObject->GetPos() - pSender->GetPos()).GetLength();
							if (dist < mindist)
							{
								pNearest = pObject->CastToCAIActor();
								if(pNearest)
								mindist = (pObject->GetPos() - pSender->GetPos()).GetLength();
							}
						}
						++ai;
					}

					if (pNearest)
					{
						pNearest->SetSignal(nSignalId,szText,pSenderEntity,pData);
						pData = NULL;
					}
				}
			}
			break;
		case SIGNALFILTER_SUPERGROUP:
			{
				int groupid = pSender->GetGroupId();
				AIObjects::iterator ai;
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					for (;ai!=m_mapGroups.end(); ai++)
					{
						CAIActor* pReciever = CastToCAIActorSafe(ai->second.GetAIObject());
						if (ai->first != groupid)
							break;
						if(pReciever)
							pReciever->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
					}
					// don't delete pData!!! - it will be deleted at the end of this function
				}
				// send message also to player
				CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
				if(pPlayer)
				{
					if(pSender->GetParameters().m_nGroup == pPlayer->GetParameters().m_nGroup )
						pPlayer->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
				}
			}
			break;
		case SIGNALFILTER_SUPERSPECIES:
			{
				int speciesid = pSender->GetParameters().m_nSpecies;
				AIObjects::iterator ai;

				if ( (ai=m_mapSpecies.find(speciesid)) != m_mapSpecies.end() )
				{
					for (;ai!=m_mapSpecies.end();)
					{
						CAIActor* pReciever = CastToCAIActorSafe(ai->second.GetAIObject());
						if (ai->first != speciesid)
							break;
						if(pReciever)
							pReciever->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
						++ai;
					}
					// don't delete pData!!! - it will be deleted at the end of this function
				}
				// send message also to player
				CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
				if(pPlayer)
				{
					pPlayer->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
				}
			}
			break;

		case SIGNALFILTER_GROUPONLY:
		case SIGNALFILTER_GROUPONLY_EXCEPT:
			{
				int groupid = pSender->GetGroupId();
				AIObjects::iterator ai;
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					for (;ai!=m_mapGroups.end(); ++ai)
					{
						CAIActor* pReciever = CastToCAIActorSafe(ai->second.GetAIObject());
						if (ai->first != groupid)
							break;
						if (pReciever && Distance::Point_PointSq(pReciever->GetPos(),pos) < fRange && !(cFilter==SIGNALFILTER_GROUPONLY_EXCEPT && pReciever==pSender) )
						{
							pReciever->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
						}
					}
					// don't delete pData!!! - it will be deleted at the end of this function
				}

				// send message also to player
				CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
				if(pPlayer)
				{
					if(pSender->GetParameters().m_nGroup == pPlayer->GetParameters().m_nGroup && Distance::Point_PointSq(pPlayer->GetPos(),pos) < fRange && !(cFilter==SIGNALFILTER_GROUPONLY_EXCEPT && pPlayer==pSender) )
						pPlayer->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
				}
			}
			break;
		case SIGNALFILTER_SPECIESONLY:
			{
				int speciesid = pSender->GetParameters().m_nSpecies;
				AIObjects::iterator ai;

				if ( (ai=m_mapSpecies.find(speciesid)) != m_mapSpecies.end() )
				{
					for (;ai!=m_mapSpecies.end();++ai)
					{
						CAIActor* pReciever = CastToCAIActorSafe(ai->second.GetAIObject());
						if (ai->first != speciesid)
							break;
						if(pReciever)
						{
							Vec3 mypos = pReciever->GetPos();
						mypos -=pos;

						if (mypos.GetLengthSquared() < fRange)
								pReciever->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
						}
					}
					// don't delete pData!!! - it will be deleted at the end of this function
				}
				// send message also to player
				CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
				if(pPlayer)
				{
					if(pSender->GetParameters().m_nSpecies == pPlayer->GetParameters().m_nSpecies && Distance::Point_PointSq(pPlayer->GetPos(),pos) < fRange  )
						pPlayer->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
				}
			}
			break;
		case SIGNALFILTER_ANYONEINCOMM:
		case SIGNALFILTER_ANYONEINCOMM_EXCEPT:
			{
				// send to puppets and aware objects in the communications range of the sender

				// Added the evaluation of AIOBJECT_VEHICLE so that vehicles can get signals.
				// 20/12/05 Tetsuji
				AIObjectOwners::iterator ai;

				std::vector<int> objectKinds;
				objectKinds.push_back(AIOBJECT_PUPPET);
				objectKinds.push_back(AIOBJECT_VEHICLE);

				std::vector<int>::iterator ki,ki2,kiend=objectKinds.end();

				for (ki=objectKinds.begin();ki!=kiend;++ki)
				{
					int objectKind =*ki;
					// first look for all the puppets
					if ( (ai=m_Objects.find(objectKind)) != m_Objects.end() )
					{
						for (;ai!=m_Objects.end();)
						{
							for (ki2=objectKinds.begin();ki2!=kiend;++ki2)
								if (ai->first==*ki)
								{
									CAIActor* pReciever = ai->second->CastToCAIActor();
									if (pReciever && Distance::Point_PointSq(pReciever->GetPos(),pos) < fRange && !(cFilter==SIGNALFILTER_ANYONEINCOMM_EXCEPT && pReciever==pSender) )
										pReciever->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
							}
							++ai;
						}
					}
					// don't delete pData!!! - it will be deleted at the end of this function
				}

				// now look for aware objects
				if ( (ai=m_Objects.find(AIOBJECT_AWARE)) != m_Objects.end() )
				{
					for (;ai!=m_Objects.end();)
					{
						if (ai->first != AIOBJECT_AWARE)
							break;
						CAIActor* pReciever = ai->second->CastToCAIActor();
						if(pReciever)
						{
							Vec3 mypos = pReciever->GetPos();
						mypos -=pos;
						if (mypos.GetLengthSquared() < fRange) 
								pReciever->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
						}
						++ai;
					}
				}
				// send message also to player

				CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
				if(pPlayer)
				{
					if( Distance::Point_PointSq(pPlayer->GetPos(),pos) < fRange && !(cFilter==SIGNALFILTER_ANYONEINCOMM_EXCEPT && pPlayer==pSender) )
					{
						PREFAST_SUPPRESS_WARNING(6011)
						pPlayer->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
					}
				}
				// don't delete pData!!! - it will be deleted at the end of this function
			}
			break;
		case SIGNALFILTER_LEADER:
		case SIGNALFILTER_LEADERENTITY:
			{
				int groupid = pSender->GetGroupId();
				AIObjects::iterator ai;
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					for (;ai!=m_mapGroups.end();)
					{
						CLeader* pLeader = CastToCLeaderSafe(ai->second.GetAIObject());
						if (ai->first != groupid)
							break;
						if(pLeader)
						{
							CAIObject* pRecipientObj = (cFilter ==SIGNALFILTER_LEADER ? pLeader : pLeader->GetAssociation().GetAIObject());
							if(pRecipientObj)
							{
								CAIActor* pRecipient = pRecipientObj->CastToCAIActor();
							if(pRecipient)
								pRecipient->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
							}
							break;
						}
						++ai;
					}
					// don't delete pData!!! - it will be deleted at the end of this function
				}
			}
			break;

	case SIGNALFILTER_FORMATION:
	case SIGNALFILTER_FORMATION_EXCEPT:
		{
			for(FormationMap::iterator fi(m_mapActiveFormations.begin()); fi!=m_mapActiveFormations.end(); ++fi)
			{
				CFormation *pFormation = fi->second;
				if(pFormation && pFormation->GetFormationPoint(refSender))
				{
					CCCPOINT(CAISystem_SendSignal_Formation);
	
					int s = pFormation->GetSize();
					for(int i=0; i<s; i++)
					{
						CAIObject* pMember = pFormation->GetPointOwner(i);
						if(pMember && (cFilter==SIGNALFILTER_FORMATION || pMember != pSender))
						{
							CAIActor* pRecipient = pMember->CastToCAIActor();
							if(pRecipient)
								pRecipient->SetSignal(nSignalId, szText,pSenderEntity, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
						}
					}
					break;
				}
			}
			break;
		}
	}
}

// adds an object to a group
//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::AddToGroup(CAIActor * pObject, int nGroupId)
{
	if(!pObject->CastToCPipeUser() && !pObject->CastToCLeader() && !pObject->CastToCAIPlayer())
		return;

	CWeakRef<CAIObject> refObj = GetWeakRef(pObject);

	if(nGroupId >=0)
		pObject->SetGroupId(nGroupId);
	else
		nGroupId = pObject->GetGroupId();

	AIObjects::iterator gi;
	bool	found = false;
	if((gi=m_mapGroups.find(nGroupId)) != m_mapGroups.end())
	{
		// check whether it was added before
		while ( (gi!=m_mapGroups.end()) &&  (gi->first == nGroupId) )
		{
			if (gi->second == refObj)
			{
				found = true;
				break;
			}
			++gi;
		}
	}

	// in the map - do nothing
	if (found)
	{
		// Update the group count status and create the group object if necessary.
		UpdateGroupStatus(nGroupId);
		return;
	}

		m_mapGroups.insert(AIObjects::iterator::value_type(nGroupId,refObj));

	// Update the group count status and create the group object if necessary.
	UpdateGroupStatus(nGroupId);

	//group leaders related stuff
	AIGroupMap::iterator	it = m_mapAIGroups.find(nGroupId);
	if(it != m_mapAIGroups.end())
	{
		if (CLeader* pLeader = pObject->CastToCLeader())
		{
			//it's a team leader - add him to leaders map
			pLeader->SetAIGroup(it->second);
			it->second->SetLeader(pLeader);
		}
		else if (CPipeUser* pPipeuser = pObject->CastToCPipeUser())
		{
			// it's a soldier - find the leader of hi's team 
			it->second->AddMember(pObject);
		}
	}
}

// removes specified object from group
//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::RemoveFromGroup(int nGroupID, CAIObject * pObject)
{
	AIObjects::iterator gi;

	for (gi = m_mapGroups.find(nGroupID);gi!=m_mapGroups.end();)
	{
		if (gi->first!=nGroupID)
			break;

		if ( gi->second == pObject)
		{
			m_mapGroups.erase(gi);
			break;
		}
		++gi;
	}

	if (m_mapAIGroups.find(nGroupID) == m_mapAIGroups.end())
		return;

	UpdateGroupStatus(nGroupID);

	AIGroupMap::iterator	it = m_mapAIGroups.find(nGroupID);
	if(it != m_mapAIGroups.end())
	{
		if (pObject->CastToCLeader())
			it->second->SetLeader(0);
		else
			it->second->RemoveMember(pObject->CastToCAIActor());
	}
}

void CAISystem::UpdateGroupStatus(int nGroupID)
{
	AIGroupMap::iterator	it = m_mapAIGroups.find(nGroupID);
	if (it == m_mapAIGroups.end())
	{
		CAIGroup*	pNewGroup = new CAIGroup(nGroupID);
		m_mapAIGroups.insert(AIGroupMap::iterator::value_type(nGroupID, pNewGroup));
		it = m_mapAIGroups.find(nGroupID);
	}

	CAIGroup* pGroup = it->second;
	AIAssert(pGroup);
	pGroup->UpdateGroupCountStatus();
}

// adds an object to a species
//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::AddToSpecies(CAIObject * pObject, unsigned short nSpecies)
{
	CWeakRef<CAIObject> refObj = GetWeakRef(pObject);
	AIObjects::iterator gi;

	// (MATT) Bit longwinded, isn't it? {2009/04/01}

	if ( (gi=m_mapSpecies.find(nSpecies)) != m_mapSpecies.end())
		// check whether it was added before
		while ((gi!=m_mapSpecies.end()) && (gi->first == nSpecies) )
		{
			if (gi->second == refObj)
				return;
			++gi;
		}

	// make sure it is not in with another species already
	AIObjects::iterator	oi(m_mapSpecies.begin());
	for (;oi!=m_mapSpecies.end();++oi)
		if (oi->second == refObj)
		{
			m_mapSpecies.erase(oi);
			break;
		}

	// it has not been added, add it now
	m_mapSpecies.insert(AIObjects::iterator::value_type(nSpecies,refObj));
}

// creates a formation and associates it with a group of agents
//
//-----------------------------------------------------------------------------------------------------------
CFormation *CAISystem::CreateFormation(CWeakRef<CAIObject> refOwner, const char * szFormationName, Vec3 vTargetPos)
{
	FormationMap::iterator fi;
	fi = m_mapActiveFormations.find(refOwner);
	if (fi==m_mapActiveFormations.end())
	{
		FormationDescriptorMap::iterator di;
		di = m_mapFormationDescriptors.find(szFormationName);
		if (di!=m_mapFormationDescriptors.end())
		{
			CCCPOINT(CAISystem_CreateFormation);

			CFormation *pFormation = new CFormation();
			pFormation->Create(di->second,refOwner,vTargetPos);
			m_mapActiveFormations.insert(FormationMap::iterator::value_type(refOwner,pFormation));
			return pFormation;
		}
	}
	else
		return (fi->second);
	return 0;
}

// change the current formation for the given group
//
//-----------------------------------------------------------------------------------------------------------
bool CAISystem::ChangeFormation(IAIObject* pOwner, const char * szFormationName,float fScale)
{
	if(pOwner)
	{
		CFormation *pFormation = ((CAIObject*)pOwner)->m_pFormation;
		if(pFormation)
		{
			FormationDescriptorMap::iterator di;
			di = m_mapFormationDescriptors.find(szFormationName);
			if (di!=m_mapFormationDescriptors.end())
			{
				CCCPOINT(CAISystem_ChangeFormation);

				pFormation->Change(di->second,fScale);
				return true;
			}
		}
	}
	return false;
}

// scale the current formation for the given group
//
//-----------------------------------------------------------------------------------------------------------
bool CAISystem::ScaleFormation(IAIObject* pOwner, float fScale)
{
	CWeakRef<CAIObject> refOwner = GetWeakRef( static_cast<CAIObject*>(pOwner) );
	FormationMap::iterator fi;
	fi = m_mapActiveFormations.find(refOwner);
	if (fi != m_mapActiveFormations.end())
	{
		CFormation *pFormation = fi->second;
		if(pFormation)
		{
			CCCPOINT(CAISystem_ScaleFormation);

			pFormation->SetScale(fScale);
			return true;
		}
	}
	return false;
}

// scale the current formation for the given group
//
//-----------------------------------------------------------------------------------------------------------
bool CAISystem::SetFormationUpdate(IAIObject* pOwner, bool bUpdate)
{
	CWeakRef<CAIObject> refOwner = GetWeakRef( static_cast<CAIObject*>(pOwner) );
	FormationMap::iterator fi;
	fi = m_mapActiveFormations.find(refOwner);
	if (fi!=m_mapActiveFormations.end())
	{
		CFormation *pFormation = fi->second;
		if(pFormation)
		{
			pFormation->SetUpdate(bUpdate);
			return true;
		}
	}
	return false;
}

// check if puppet and vehicle are in the same formation
//
//-----------------------------------------------------------------------------------------------------------
bool CAISystem::SameFormation(const CPuppet* pHuman, const CAIVehicle* pVehicle)
{
	if(pHuman->IsHostile(pVehicle))
		return false;
	for(FormationMap::iterator fi(m_mapActiveFormations.begin()); fi!=m_mapActiveFormations.end(); ++fi)
	{
		CFormation *pFormation = fi->second;
		if(pFormation->GetFormationPoint( GetWeakRef(pHuman) ))
		{
			CAIObject* pFormationOwner(pFormation->GetPointOwner(0));
			if(!pFormationOwner)
				return false;
			if(pFormationOwner == pVehicle->GetAttentionTarget())
			{
				CCCPOINT(CAISystem_SameFormation);
				return true;
			}
			return false;
		}
	}
	return false;
}

// Resets all agent states to initial
//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::Reset(IAISystem::EResetReason reason)
{
	if(!IsEnabled())
		return;

  AILogEvent("CAISystem::Reset %d", reason);
	m_bUpdateSmartObjects = false;


	// Notify listeners
	for (SystemListenerSet::iterator it = m_setSystemListeners.begin(); it != m_setSystemListeners.end(); it++)
	{
		(*it)->OnEvent(IAISystemListener::eAISE_Reset);
	}

	// (MATT) How many of these things are really editor-only? Unify the codepaths. {2008/09/05}
	if (gEnv->IsEditor())
	{
		switch(reason)
		{
		case RESET_INTERNAL:
			// Signals basic Init call on AI system
		case RESET_INTERNAL_LOAD:
			break;
		case RESET_EXIT_GAME:
			if( m_pEmotionalSystem != NULL ) m_pEmotionalSystem->OnEditorSetGameMode(true);
			break;
		case RESET_ENTER_GAME:
			if( m_pEmotionalSystem != NULL ) m_pEmotionalSystem->OnEditorSetGameMode(false);
			break;
		}
	}

	if (gAIEnv.pTargetTrackManager)
		gAIEnv.pTargetTrackManager->Reset(reason);

	// (Kevin) Following items should be reset in launcher as well. Unify the codepaths! (2008/10/27)
	switch(reason)
	{
	case RESET_INTERNAL:
		// Signals basic Init call on AI system
	case RESET_INTERNAL_LOAD:
		// Called by the AI system itself
		gAIEnv.pGroupSystem->Clear();
		break;
	case RESET_EXIT_GAME:
		m_pBTProfileDictionary->OnEditorSetGameMode(false);
		gAIEnv.pGroupSystem->OnEditorSetGameMode(false);
		break; 
	case RESET_ENTER_GAME:
		// Called on entering game mode
		m_pBTProfileDictionary->OnEditorSetGameMode(true);
		gAIEnv.pGroupSystem->OnEditorSetGameMode(true);
		break;
	}

#ifdef CRYAISYSTEM_DEBUG
	m_Recorder.OnReset(reason);
#endif //CRYAISYSTEM_DEBUG

	// It would be much better to have this at the end of Reset, when state is minimal
	// However there are various returns below that stop it being hit

	SetAIworkaroundsConfiguration();

	// May need to make calls here to get those settings reflected in the system


	if(reason != RESET_ENTER_GAME)
	// Reset Interest System
		GetCentralInterestManager()->Reset();

	m_lightManager.Reset();

	gAIEnv.pCodeCoverageGUI->Reset(reason);

	// reset the groups.
	for(AIGroupMap::iterator it = m_mapAIGroups.begin(); it != m_mapAIGroups.end(); ++it)
		it->second->Reset();

	m_CurrentGlobalAlertness = -1; // to force initial change to 0
	for (int i = 0; i < NUM_ALERTNESS_COUNTERS; i++)
		m_AlertnessCounters[i] = 0;

	if (!m_bInitialized)
		return;

	const char * sStatsTarget = gAIEnv.CVars.StatsTarget;
	if ((*sStatsTarget != '\0') && (stricmp("none",sStatsTarget)!=0))
		Record(NULL, IAIRecordable::E_RESET, NULL);

	if ( m_pSmartObjectManager->IsInitialized() )
	{
		m_pAIActionManager->Reset();

		// If CAISystem::Reset() was called from CAISystem::Shutdown() IEntitySystem is already deleted
		if ( gEnv->pEntitySystem )
		{
			if ( reason == IAISystem::RESET_ENTER_GAME )
			{
				m_pSmartObjectManager->SoftReset();
			}
			if ( reason == IAISystem::RESET_INTERNAL_LOAD )
			{
				m_pSmartObjectManager->ReloadSmartObjectRules();
			}
		}
	}
	else
	{
		// If CAISystem::Reset() was called from CAISystem::Shutdown() IEntitySystem is already deleted
		if ( gEnv->pEntitySystem )
			InitSmartObjects();
	}

	if (m_Objects.empty())
		return;

	//m_mapDEBUGTiming.clear();


	AIObjectOwners::iterator ai;

	EObjectResetType objectResetType = AIOBJRESET_INIT;
	switch (reason)
	{
	case RESET_EXIT_GAME:
		objectResetType = AIOBJRESET_SHUTDOWN;
		break;
	case RESET_INTERNAL:
	case RESET_INTERNAL_LOAD:
	case RESET_ENTER_GAME:
	default:
		objectResetType = AIOBJRESET_INIT;
		break;
	}

	for (ai=m_Objects.begin();ai!=m_Objects.end();++ai)
	{
		// Strong, so always valid
		CAIObject *pObject = ai->second.GetAIObject();
		pObject->Reset(objectResetType);

#ifdef CRYAISYSTEM_DEBUG
		// Reset the AI recordable stuff when entering game mode.
		if(reason == IAISystem::RESET_ENTER_GAME)
		{
			CRecorderUnit *pRecord = static_cast<CRecorderUnit*>(pObject->GetAIDebugRecord());
			if (pRecord)
			{
				pRecord->ResetStreams(GetFrameStartTime());
			}
		}
#endif //CRYAISYSTEM_DEBUG

	}

#ifdef CRYAISYSTEM_DEBUG

	// Reset the recorded trajectory.
	m_lastStatsTargetTrajectoryPoint.Set(0,0,0);
	m_lstStatsTargetTrajectory.clear();

	m_DEBUG_fakeTracers.clear();
	m_DEBUG_fakeHitEffect.clear();
	m_DEBUG_fakeDamageInd.clear();
	m_DEBUG_screenFlash = 0.0f;

#endif //CRYAISYSTEM_DEBUG


	if (!m_mapActiveFormations.empty())
	{
		FormationMap::iterator fi;
		for (fi=m_mapActiveFormations.begin();fi!=m_mapActiveFormations.end();++fi)
		{
			CFormation* pFormation = fi->second;
			if(pFormation)
			{
				delete pFormation;
			}
		}
		m_mapActiveFormations.clear();
	}

	m_deadBodies.clear();

	if (reason == RESET_EXIT_GAME)
	{
		gAIEnv.pTacticalPointSystem->Reset();
		gAIEnv.pCoordinationManager->Reset();
		gAIEnv.pCommunicationManager->Reset();
		gAIEnv.pSelectionTreeManager->Reset();

		m_visionMap.Reset();
	}

  ClearAIObjectIteratorPools();

	m_pPathfinder->Reset(reason);
	m_pNavigation->Reset(reason);

	m_pGraph->ClearMarks();
	m_pGraph->Reset();

  m_isPointInForbiddenRegionQueue.Clear();
  m_isPointOnForbiddenEdgeQueue.Clear();

  CPathObstacles::Reset();

	m_dynHideObjectManager.Reset();

	m_mapAuxSignalsFired.clear();

		m_mapBeacons.clear();

	// Remove temporary shapes and re-enable all shapes.
	for(ShapeMap::iterator it = m_mapGenericShapes.begin(); it != m_mapGenericShapes.end();)
	{
		// Enable
		it->second.enabled = true;
		// Remove temp
		if(it->second.temporary)
			m_mapGenericShapes.erase(it++);
		else
			++it;
	}

  // Init profiles, this will happen only once, no matter how many times we  call this
	m_pBTProfileDictionary->InitProfiles();
	
	m_pCoopReadabilitiesSystem->Reset();

	m_VertexList.Reset();

	m_pSmartObjectManager->ResetBannedSOs();

	m_lastAmbientFireUpdateTime.SetSeconds(-10.0f);
	m_lastExpensiveAccessoryUpdateTime.SetSeconds(-10.0f);
	m_lastVisBroadPhaseTime.SetSeconds(-10.0f);
	m_lastGroupUpdateTime.SetSeconds(-10.0f);

	m_delayedExpAccessoryUpdates.clear();

	CPerceptionManager::GetInstance()->Reset();

	m_enabledPuppetsUpdateError = 0;
	m_enabledPuppetsUpdateHead = 0;
	m_totalPuppetsUpdateCount = 0;
	m_disabledPuppetsUpdateError = 0;
	m_disabledPuppetsHead = 0;

	DebugOutputObjects("End of reset");

#ifdef CALIBRATE_STOPPER
  AILogAlways("Calculation stopper calibration:");
  for (CCalculationStopper::TMapCallRate::const_iterator it = CCalculationStopper::m_mapCallRate.begin() ; it != CCalculationStopper::m_mapCallRate.end() ; ++it)
  {
    const std::pair<unsigned, float> &result = it->second;
    const string &name = it->first;
    float rate = result.second > 0.0f ? result.first / result.second : -1.0f;
    AILogAlways("%s calls = %d time = %6.2f sec call-rate = %7.2f",f
      name.c_str(), result.first, result.second, rate);
  }
#endif
}


//-----------------------------------------------------------------------------------------------------------
int CAISystem::GetGroupCount(int nGroupID, int flags, int type)
{
	if (type == 0)
	{
		AIGroupMap::iterator	it = m_mapAIGroups.find(nGroupID);
		if(it == m_mapAIGroups.end())
			return 0;

		return it->second->GetGroupCount(flags);
	}
	else
	{
		int count = 0;
		int	countEnabled = 0;
		for (AIObjects::iterator it = m_mapGroups.find(nGroupID); it != m_mapGroups.end() && it->first == nGroupID; ++it)
		{
			CAIObject* pObject = it->second.GetAIObject();;
			if (pObject->GetType() == type)
			{
				if (pObject->IsEnabled())
					countEnabled++;
				count++;
			}
		}

		if (flags & IAISystem::GROUP_MAX)
		{
			AIWarning("GetGroupCount does not support specified type and max count to be used at the same time."/*, type*/);
			flags &= ~IAISystem::GROUP_MAX;
		}

		if (flags == 0 || flags == IAISystem::GROUP_ALL)
			return count;
		else if (flags == IAISystem::GROUP_ENABLED)
			return countEnabled;
	}

	return 0;
}

// gets the I-th agent in a specified group
//
//-----------------------------------------------------------------------------------------------------------
IAIObject* CAISystem::GetGroupMember(int nGroupID, int nIndex, int flags, int type)
{
	if (nIndex < 0)
		return NULL;

	bool	bOnlyEnabled = false;
	if (flags & IAISystem::GROUP_ENABLED)
		bOnlyEnabled = true;

	AIObjects::iterator gi = m_mapGroups.find(nGroupID);
	for (; gi!=m_mapGroups.end() && nIndex >= 0; ++gi)
	{
		CAIObject *pObject = gi->second.GetAIObject();
		if ( gi->first!=nGroupID )
			return NULL;
		if (pObject && pObject->GetType() != AIOBJECT_LEADER && // skip leaders
			(!type || pObject->GetType() == type) &&
			(!bOnlyEnabled || pObject->IsEnabled()) )
		{
			if (0 == nIndex)
				return pObject;
			--nIndex;
			// (MATT) Why decrement index? {2009/03/26}
		}
	}
	return NULL;
}

//====================================================================
// GetLeader
//====================================================================
CLeader* CAISystem::GetLeader(int nGroupID)
{
	AIGroupMap::iterator	it = m_mapAIGroups.find(nGroupID);
	if(it != m_mapAIGroups.end())
		return it->second->GetLeader();
	return 0;
}


CLeader* CAISystem::GetLeader(const CAIActor* pSoldier)
{
	return (pSoldier ? GetLeader(pSoldier->GetGroupId()): NULL);
}

//====================================================================
// SetLeader
//====================================================================
void CAISystem::SetLeader(IAIObject* pObject)
{
	// can't make disabled/dead puppet a leader
	if(!pObject->IsEnabled() || pObject->GetProxy()&&pObject->GetProxy()->GetActorHealth()<1 )
		return;
	CAIActor* pActor = pObject->CastToCAIActor();
	if(!pActor)
		return;
	int groupid = pActor->GetGroupId();
	CLeader* pLeader = GetLeader(groupid);
	if(!pLeader)
	{
		pLeader  = (CLeader*) CreateAIObject(AIOBJECT_LEADER, pObject, 0);
		CAIGroup* pGroup = GetAIGroup(groupid);
		if(pGroup)
			pGroup->SetLeader(pLeader);
	}
	else
	{
		pLeader->SetAssociation(GetWeakRef((CAIObject*)pObject));
		CCCPOINT(CAISystem_SetLeader);
	}

	return;
}

//====================================================================
// SetLeader
//====================================================================
CLeader*  CAISystem::CreateLeader(int nGroupID)
{
	CLeader* pLeader = GetLeader(nGroupID);
	if(pLeader)
		return NULL;
	CAIGroup* pGroup = GetAIGroup(nGroupID);
	if(!pGroup)
		return NULL;
	TUnitList &unitsList=pGroup->GetUnits();
	if(unitsList.empty())
		return NULL;

	CAIActor* pLeaderActor=(unitsList.begin())->m_refUnit.GetAIObject();
	pLeader  = (CLeader*) CreateAIObject(AIOBJECT_LEADER, pLeaderActor, 0);
	pGroup->SetLeader(pLeader);
	return pLeader;
}



//====================================================================
// GetAIGroup
//====================================================================
CAIGroup* CAISystem::GetAIGroup(int nGroupID)
{
	AIGroupMap::iterator	it = m_mapAIGroups.find(nGroupID);
	if(it != m_mapAIGroups.end())
		return it->second;
	return 0;
}

IAIGroup* CAISystem::GetIAIGroup(int nGroupID)
{
	return (IAIGroup*)GetAIGroup(nGroupID);
}


//====================================================================
// ReadAreasFromFile
//====================================================================
void CAISystem::ReadAreasFromFile(const char * fileNameAreas)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Navigation, 0, "Areas (%s)", fileNameAreas);

	CCryFile file;
	if( false != file.Open( fileNameAreas, "rb" ) )
	{
		int iNumber;
		file.ReadType(&iNumber);
/* //Diesel cut
		if (iNumber == 18)
		{
			AIWarning("AI area file version 18 found - suitable only for subsequent export");
		}
		else if (iNumber < BAI_AREA_FILE_VERSION_READ)
		{
			AIError("Incompatible AI area file version - found %d, expected range [%d,%d] - regenerate triangulation [Design bug]", iNumber, BAI_AREA_FILE_VERSION_READ, BAI_AREA_FILE_VERSION_WRITE);
			file.Close();
			return;
		}
*/ //Diesel cut
		m_pNavigation->ReadAreasFromFile(file, iNumber);

		FlushAllAreas();

		unsigned numAreas;

		// Read generic shapes
		if(iNumber > 15)
		{
			file.ReadType(&numAreas);
			// vague sanity check
			AIAssert(numAreas < 1000000);
			for (unsigned iArea = 0 ; iArea < numAreas ; ++iArea)
			{
				ListPositions lp;
				string name;
				ReadPolygonArea(file, iNumber, name, lp);

				int	navType, type;
				file.ReadType(&navType);
				file.ReadType(&type);
				float height = 0;
				int lightLevel = 0;
				if (iNumber >= 18)
				{
					file.ReadType(&height);
					file.ReadType(&lightLevel);
				}
//Diesel cut
				//if (m_mapGenericShapes.find(name) != m_mapGenericShapes.end())
				//	AIError("CAISystem::ReadAreasFromFile: Shape '%s' already exists, please rename the path and reexport.", name.c_str());
				//else
//Diesel cut
				m_mapGenericShapes.insert(ShapeMap::iterator::value_type(name, SShape(lp, false, (IAISystem::ENavigationType)navType, type, height, (EAILightLevel)lightLevel)));
			}
		}
		file.Close();
	}
	else
	{
		AIWarning("Unable to open areas file %s", fileNameAreas);
	}
}


// // loads the triangulation for this level and mission
//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::LoadNavigationData(const char * szLevel, const char * szMission, bool demandLoadLNMs)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "AI Navigation");

  LOADING_TIME_PROFILE_SECTION(GetISystem());

	if(!IsEnabled())
		return;
	if (!szLevel || !szMission)
		return;
	if ( m_bInitialized )
		Reset(IAISystem::RESET_INTERNAL_LOAD);

	m_pPathfinder->FlushPathQueue ();

	m_pNavigation->LoadNavigationData(szLevel, szMission, demandLoadLNMs);
}


void CAISystem::LoadCover(const char* szLevel, const char* szMission)
{
	gAIEnv.pCoverSystem->Clear();

	char coverFileName[1024] = { 0 };
	sprintf(coverFileName, "%s/cover%s.bai", szLevel, szMission);
	gAIEnv.pCoverSystem->ReadSurfacesFromFile(coverFileName);
}

bool CAISystem::LoadNavMesh(const char * navModifName, const char * agentTypeName)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "AI Navigation");

	return m_pNavigation->LoadNavMesh (navModifName, agentTypeName);
}

bool CAISystem::UnloadNavMesh(const char * navModifName, const char * agentTypeName)
{
	return m_pNavigation->UnloadNavMesh (navModifName, agentTypeName);
}

//====================================================================
// OnMissionLoaded
//====================================================================
void CAISystem::OnMissionLoaded()
{
	m_pNavigation->OnMissionLoaded();
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::ReleaseFormation(CWeakRef<CAIObject> refOwner, bool bDelete)
{
	CCCPOINT(CAISystem_ReleaseFormation);

	FormationMap::iterator fi;
	fi = m_mapActiveFormations.find(refOwner);
	if (fi!=m_mapActiveFormations.end())
	{
		CFormation* pFormation = fi->second;
		if(bDelete && pFormation)
		{
			//pFormation->ReleasePoints();
			delete pFormation;
		}
		m_mapActiveFormations.erase(fi);
}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::FreeFormationPoint(CWeakRef<CAIObject> refOwner)
{
	FormationMap::iterator fi;
	fi = m_mapActiveFormations.find(refOwner);
	if (fi!=m_mapActiveFormations.end())
		(fi->second)->FreeFormationPoint(refOwner);
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::FlushSystem(void)
{
  AILogEvent("CAISystem::FlushSystem");
	FlushSystemNavigation();
	// remove all the leaders first;
	m_Objects.erase(AIOBJECT_LEADER);;
	m_lightManager.Reset();

  m_isPointInForbiddenRegionQueue.Clear();
  m_isPointOnForbiddenEdgeQueue.Clear();

	m_pSmartObjectManager->ResetBannedSOs();
	m_dynHideObjectManager.Reset();

	m_pPathfinder->FlushPathQueue();

	m_mapSpecies.clear();
	m_mapGroups.clear();
	m_mapDummyObjects.clear();
    m_mapBeacons.clear();

	FormationMap::iterator iFormation=m_mapActiveFormations.begin(), iEnd = m_mapActiveFormations.end();
	for( ;iFormation != iEnd; ++iFormation)
		delete iFormation->second;
	m_mapActiveFormations.clear();

	GetCentralInterestManager()->Reset();

	// Remove all the objects themselves
	m_Objects.clear();

	// (MATT) Flush all the objects that may have been deregistered on leaving a previous level
	// Really, this should delete all AI objects regardless {2009/03/27}
	gAIEnv.pObjectContainer->ReleaseDeregisteredObjects();

	int numRegistered = gAIEnv.pObjectContainer->GetNumRegistered();
	if (numRegistered != 0)
	{
		gAIEnv.pObjectContainer->DumpRegistered();
	}
	CRY_ASSERT_MESSAGE(numRegistered == 0, "Something has leaked AI objects! Check the log for details");
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::FlushSystemNavigation(void)
{
  AILogEvent("CAISystem::FlushSystemNavigation");

	// clear pathfinder
	m_pPathfinder->CancelCurrentRequest();
	m_pPathfinder->FlushPathQueue();

	// clear any paths in the puppets
	AIObjectOwners::iterator ai;
	for (ai = m_Objects.find(AIOBJECT_PUPPET);ai!=m_Objects.end();++ai)
	{
		// Strong, so always valid
		CPuppet* pPuppet = ai->second.GetAIObject()->CastToCPuppet();
		if (ai->first != AIOBJECT_PUPPET) 
			break;
		if (pPuppet)
			pPuppet->Reset(AIOBJRESET_INIT);
	} 

	for (ai = m_Objects.begin();ai!=m_Objects.end();++ai)
	{
		CAIObject *pObject = ai->second.GetAIObject();
		pObject->Reset(AIOBJRESET_INIT);
	}

	m_pNavigation->FlushSystemNavigation();
}



void CAISystem::GetDetectionLevels(IAIObject *pObject, SAIDetectionLevels& levels)
{
	levels.Reset();

	if (!pObject)
		pObject = GetPlayer();
	if (!pObject)
		return;
	
	if (CAIPlayer* pPlayer = pObject->CastToCAIPlayer())
	{
		levels = pPlayer->GetDetectionLevels();
	}
	else if (CPuppet* pPuppet = pObject->CastToCPuppet())
	{
		levels = pPuppet->GetDetectionLevels();
	}
}


//
//-----------------------------------------------------------------------------------------------------------
int CAISystem::GetAITickCount(void)
{
	return m_nTickCount;
}


//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::SendAnonymousSignal(int nSignalID, const char * szText, const Vec3 & vPos, float fRadius, IAIObject *pObject, IAISignalExtraData* pData)
{
	CCCPOINT(CAISystem_SendAnonymousSignal);

	AIObjects::iterator ai;
	fRadius*=fRadius;

	// find sending puppet (we need this to know which group to skip when we send the signal)
/*	int groupid = -1;
	if (pObject)
	{
		CPuppet* pPuppet = pObject->CastToCPuppet();
		if (pPuppet)
			groupid = pPuppet->GetGroupId();
	}
*/
	// go trough all the puppets and vehicles in the surrounding area.
	// still makes precise sq radius check inside because the grid might return
	// objects in a bigger radius 
	SEntityProximityQuery query;	
	query.nEntityFlags = ENTITY_FLAG_HAS_AI;
	query.pEntityClass = NULL;
	float dim = fRadius;
	query.box = AABB(Vec3(vPos.x-dim,vPos.y-dim,vPos.z-dim), Vec3(vPos.x+dim,vPos.y+dim,vPos.z+dim));
	gEnv->pEntitySystem->QueryProximity(query);
	
	for(int i=0; i<query.nCount; ++i)
	{
		IEntity* pEntity = query.pEntities[i];
		if (pEntity)
		{
			IAIObject *pAI=pEntity->GetAI();
			if (!pAI)
				continue;			

			CPuppet* pPuppet = pAI->CastToCPuppet();
		if (pPuppet)
			{
				if( pPuppet->GetParameters().m_PerceptionParams.perceptionScale.visual>.01f &&
					pPuppet->GetParameters().m_PerceptionParams.perceptionScale.audio>.01f &&
					Distance::Point_PointSq(pPuppet->GetPos(),vPos) < fRadius )
				{
					if (pObject)
						// is sender is not actor - probably will not have entity or scriptTable, so use pPuppet
						pPuppet->SetSignal(nSignalID, szText, pObject->CastToCAIActor() ? pObject->GetEntity() : pPuppet->GetEntity(),
							pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
					else
						pPuppet->SetSignal(nSignalID, szText, NULL, pData ? new AISignalExtraData(*(AISignalExtraData*)pData) : NULL );
	}
			}
		}
	} //i


	// finally delete pData since all recipients have their own copies
	if ( pData )
		delete (AISignalExtraData*)pData;
}

void CAISystem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	if (event == ESYSTEM_EVENT_GAME_POST_INIT)
	{
		gAIEnv.pCommunicationManager->ScanFolder("Scripts/AI/Communication");
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::ReleaseFormationPoint(CAIObject * pReserved)
{
	if (m_mapActiveFormations.empty())
		return;

	CCCPOINT(CAISystem_ReleaseFormationPoint);

	CWeakRef<CAIObject> refReserved = GetWeakRef(pReserved);

	FormationMap::iterator fi;
	CAIActor* pActor = pReserved->CastToCAIActor();
	CLeader* pLeader = GetLeader(pActor);
	if(pLeader)
		pLeader->FreeFormationPoint(refReserved);
	else
		for (fi=m_mapActiveFormations.begin();fi!=m_mapActiveFormations.end();++fi)
			(fi->second)->FreeFormationPoint(refReserved);
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::Update(CTimeValue frameStartTime, float frameDeltaTime)
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	static CTimeValue lastFrameStartTime;
	if ( frameStartTime == lastFrameStartTime )
		return;
	lastFrameStartTime = frameStartTime;


	if (!m_bInitialized || !IsEnabled())
		return;

	if (!gAIEnv.CVars.AiSystem) 
		return;

	CCCPOINT(CAISystem_Update);

	// used for debug
	if( m_pEmotionalSystem != NULL ) 
	{
		m_pEmotionalSystem->SetDebug( gAIEnv.CVars.DebugEmotionalSystem );
		m_pEmotionalSystem->UpdateDraw();
		m_pEmotionalSystem->Update( frameDeltaTime );
	}

	m_pBTProfileDictionary->UpdateDraw();
	m_pBTProfileDictionary->Update( frameDeltaTime );
	m_pBTProfileDictionary->SetDebugTree( gAIEnv.CVars.DebugBehaviorTree );
	m_pBTProfileDictionary->SetDebugTactics( gAIEnv.CVars.DebugTactics ? true : false );
	m_pBTProfileDictionary->SetDebugActivationConditions( gAIEnv.CVars.DebugActivationConditions );

	gAIEnv.pGroupSystem->Update( frameDeltaTime );

	//	for (AIGroupMap::iterator it = m_mapAIGroups.begin(); it != m_mapAIGroups.end(); ++it)
	//		it->second->Validate();

	{
		FRAME_PROFILER("AIUpdate 1", gEnv->pSystem, PROFILE_AI)

		if ( !m_pSmartObjectManager->IsInitialized() )
			InitSmartObjects();

		if ( m_pAIActionManager )
			m_pAIActionManager->Update();

		ClearBadFloorRecords();

		m_fFrameStartTime = frameStartTime;
		m_fFrameDeltaTime = frameDeltaTime;

		CAIRadialOcclusionRaycast::UpdateActiveCount();

		m_lightManager.Update(false);

		if (m_pNavigation->GetNavDataState() == CNavigation::NDS_BAD)
		{
			static CTimeValue lastTime = frameStartTime;
			if ((frameStartTime - lastTime).GetSeconds() > 5.0f)
			{
				AIWarning("*** AI navigation is bad. Please regenerate. AI SYSTEM IS NOT UPDATED");
				lastTime = frameStartTime;
			}
			return;
		}
		else if (m_pNavigation->GetNavDataState() != CNavigation::NDS_OK)
		{
			CPerceptionManager::GetInstance()->Update(m_fFrameDeltaTime);
			return;
		}

		IConsole *pConsole = gEnv->pConsole;
		CCalculationStopper::m_useCounter = gAIEnv.CVars.UseCalculationStopperCounter != 0;

		m_pNavigation->Update (frameStartTime, frameDeltaTime);

		// Update all the dead bodies, remove expired ones
		for (unsigned i = 0; i < m_deadBodies.size(); )
		{
			// decrease timeout
			m_deadBodies[i].t -= m_fFrameDeltaTime;
			if (m_deadBodies[i].t > 0.0f) 
			{
				++i;
				continue;
			}
			m_deadBodies[i] = m_deadBodies.back();
			m_deadBodies.pop_back();
		}

		m_pSmartObjectManager->UpdateBannedSOs(m_fFrameDeltaTime);
	}

	CheckVisibilityBodies();
	UpdateAmbientFire();
	UpdateExpensiveAccessoryQuota();

	CPerceptionManager::GetInstance()->Update(m_fFrameDeltaTime);

	gAIEnv.pCoordinationManager->Update(frameDeltaTime);
	gAIEnv.pCommunicationManager->Update(frameDeltaTime);
	m_visionMap.Update(frameDeltaTime);

	{
		FRAME_PROFILER("AIUpdate 2", gEnv->pSystem, PROFILE_AI)

			// Marcio: Update all players.
			AIObjectOwners::const_iterator ai=m_Objects.find(AIOBJECT_PLAYER);
		for (;ai!=m_Objects.end() && ai->first==AIOBJECT_PLAYER; ++ai)
		{
			CAIPlayer* pPlayer = CastToCAIPlayerSafe(ai->second.GetAIObject());
			if (pPlayer)
				pPlayer->Update(AIUPDATE_FULL);
		}

		// trace any paths that were requested
		m_pPathfinder->UpdatePathFinder();

		{
			FRAME_PROFILER("AIUpdate groups", gEnv->pSystem, PROFILE_AI)
				const float dt = (frameStartTime - m_lastGroupUpdateTime).GetSeconds();
			if (dt > gAIEnv.CVars.AIUpdateInterval)
			{
				for (AIGroupMap::iterator it = m_mapAIGroups.begin(); it != m_mapAIGroups.end(); ++it)
					it->second->Update();
				m_lastGroupUpdateTime = frameStartTime;
			}
		}
		UpdateAuxSignalsMap();
	}

	{
		FRAME_PROFILER("AIUpdate 3", gEnv->pSystem, PROFILE_AI)

			// (MATT) We could be more subtle about this, but this is a simple way to erase invalidated puppets {2009/04/08}
			for (PuppetSet::iterator it = m_enabledPuppetsSet.begin(); it != m_enabledPuppetsSet.end(); )
			{
				const CPuppet *pPuppet = it->GetAIObject();
				if (!pPuppet)
					it = m_enabledPuppetsSet.erase(it);
				else
					++it;
			}

			unsigned activePuppetCount = m_enabledPuppetsSet.size();
			gAIEnv.pStatsManager->SetStat( eStat_ActivePuppets, static_cast<float>(activePuppetCount) );

			if (activePuppetCount > 0)
			{
				const float updateInterval = max(gAIEnv.CVars.AIUpdateInterval,0.0001f);
				const float updatesPerSecond = (activePuppetCount / updateInterval) + m_enabledPuppetsUpdateError;
				unsigned puppetUpdateCount = (unsigned)floorf(updatesPerSecond * m_fFrameDeltaTime);
				if (m_fFrameDeltaTime > 0.0f)
					m_enabledPuppetsUpdateError = updatesPerSecond - puppetUpdateCount / m_fFrameDeltaTime;

				// Collect list of enabled priority targets (players, grenades, projectiles, etc).
				static std::vector<CAIObject*> priorityTargets;
				priorityTargets.resize(0);
				for (unsigned i = 0, ni = m_priorityObjectTypes.size(); i < ni; ++i)
				{
					short type = m_priorityObjectTypes[i];
					AIObjectOwners::iterator ai = m_Objects.find(type);
					for ( ; ai != m_Objects.end() && ai->first == type; ++ai)
					{
						CAIObject* pTarget = ai->second.GetAIObject();
						if (!pTarget->IsEnabled())
							continue;
						priorityTargets.push_back(pTarget);
					}
				}

				int64 fst(m_fFrameStartTime.GetMilliSecondsAsInt64());
				bool bRunOutOfRays = false;
				unsigned fullUpdates = 0;
				m_enabledPuppetsUpdateHead %= activePuppetCount;
				int idx = m_enabledPuppetsUpdateHead;

				// Note: cannot cache the set size, since a puppet may be removed from the list as part of the update.
				for (unsigned i = 0; i < m_enabledPuppetsSet.size(); ++i)
				{
					if (idx >= (int)m_enabledPuppetsSet.size())
						idx = 0;

					CPuppet *pPuppet = m_enabledPuppetsSet[idx].GetAIObject();

					idx++;

					if (!pPuppet)
						continue;  // (MATT) Shouldn't actually be required I suspect, invalid ones should already be removed above {2009/04/08}

					if (fullUpdates < puppetUpdateCount && !bRunOutOfRays)
					{
						pPuppet->SetUpdatePriority(CalcPuppetUpdatePriority(pPuppet));

						// Full update
						bRunOutOfRays = CPerceptionManager::GetInstance()->UpdatePerception(pPuppet, priorityTargets);

						// TODO/NOTE: This should be inside the SingleFullUpdate, but since there
						// are 2 versions currently, it is here for the time being. [Mikko]
						ITimer *pTimer = gEnv->pTimer;
						CTimeValue timeLast;
						if (gAIEnv.CVars.AllTime)
							timeLast = pTimer->GetAsyncTime();

						pPuppet->Update(AIUPDATE_FULL);

						BroadcastToListeners(OnAgentUpdate(pPuppet->GetEntityID()));

						if (gAIEnv.CVars.AllTime)
						{
							FRAME_PROFILER( "AISFU:TimeMeasuringBlock", gEnv->pSystem,PROFILE_AI);
							CTimeValue timeCurr = pTimer->GetAsyncTime();
							float fTime = (timeCurr-timeLast).GetSeconds();
							timeLast=timeCurr;

							//m_mapDEBUGTiming[pPuppet->GetAIObjectID()] = fTime;
						}

						if (!pPuppet->m_bUpdatedOnce && m_bUpdateSmartObjects && pPuppet->IsEnabled() &&
							(!pPuppet->GetCurrentGoalPipe() || !pPuppet->GetCurrentGoalPipe()->GetSubpipe()))
							pPuppet->m_bUpdatedOnce = true;

						m_totalPuppetsUpdateCount++;

						if (m_totalPuppetsUpdateCount >= (int)activePuppetCount)
						{
							// full update cycle finished on all ai objects
							// now allow updating smart objects
							m_bUpdateSmartObjects = true;
							// Take snapshot of the perception values when all puppets have been updated.
							TakeDetectionSnapshot();
							m_totalPuppetsUpdateCount = 0;
						}

						fullUpdates++;
					}
					else
					{
						// Dry update
						SingleDryUpdate(pPuppet);
					}
				}

				gAIEnv.pStatsManager->SetStat( eStat_FullUpdates, static_cast<float>(fullUpdates) );

				// Advance update head.
				m_enabledPuppetsUpdateHead += fullUpdates;

			}
			else
			{
				// No active puppets, allow updating smart objects
				m_bUpdateSmartObjects = true;
				// No active puppets, make sure the detection gets still updated.
				TakeDetectionSnapshot();
			}

			// Update disabled

			// (MATT) We could be more subtle about this, but this is a simple way to erase invalidated puppets {2009/04/08}
			for (PuppetSet::iterator it = m_disabledPuppetsSet.begin(); it != m_disabledPuppetsSet.end(); )
			{
				const CPuppet *pPuppet = it->GetAIObject();
				if (!pPuppet)
					it = m_disabledPuppetsSet.erase(it);
				else
					++it;
			}


			if (!m_disabledPuppetsSet.empty())
			{
				unsigned inactivePuppetCount = m_disabledPuppetsSet.size();
				const float updateInterval = 0.3f;
				const float updatesPerSecond = (inactivePuppetCount / updateInterval) + m_disabledPuppetsUpdateError;
				unsigned puppetDisabledUpdateCount = (unsigned)floorf(updatesPerSecond * m_fFrameDeltaTime);
				if (m_fFrameDeltaTime > 0.0f)
					m_disabledPuppetsUpdateError = updatesPerSecond - puppetDisabledUpdateCount / m_fFrameDeltaTime;

				m_disabledPuppetsHead %= inactivePuppetCount;
				int idx = m_disabledPuppetsHead;

				for (unsigned i = 0; i < puppetDisabledUpdateCount; ++i)
				{
					CPuppet *pPuppet = m_disabledPuppetsSet[idx].GetAIObject();

					if (pPuppet) // (MATT) Shouldn't actually be required I suspect, invalid ones should already be removed above {2009/04/08}
					{
						pPuppet->UpdateDisabled(AIUPDATE_FULL);
					}

					// [AlexMcC|28.09.09] UpdateDisabled might remove the puppet from the disabled set, so the size might change
					inactivePuppetCount = m_disabledPuppetsSet.size();
					if(inactivePuppetCount == 0)
					{
						break;
					}

					idx++;
					if (idx >= (int)inactivePuppetCount)
						idx = 0;
				}

				// Advance update head.
				m_disabledPuppetsHead += puppetDisabledUpdateCount;
			}

			//
			//	update all leaders here (should be not every update (full update only))
			const static float leaderUpdateRate(.2f);
			static float leaderNoUpdatedTime(.0f);
			leaderNoUpdatedTime += m_fFrameDeltaTime;
			if(leaderNoUpdatedTime > leaderUpdateRate)
			{
				leaderNoUpdatedTime = .0f;
				AIObjectOwners::iterator aio = m_Objects.find(AIOBJECT_LEADER);
				for (;aio!=m_Objects.end();++aio)
				{
					if (aio->first != AIOBJECT_LEADER)
						break;
					CLeader* pLeader = aio->second.GetAIObject()->CastToCLeader();
					if (pLeader)
						pLeader->Update(AIUPDATE_FULL);
				}
			}
			// leaders update over
			if ( m_bUpdateSmartObjects )
				m_pSmartObjectManager->Update();

			//	fLastUpdateTime = currTime;
			++m_nTickCount;

#ifdef CRYAISYSTEM_DEBUG
			UpdateDebugStuff();
#endif //CRYAISYSTEM_DEBUG

			// Update interest system
			ICentralInterestManager * pInterestManager = CCentralInterestManager::GetInstance();
			if(pInterestManager->Enable( gAIEnv.CVars.InterestSystem!=0 ))
			{
				pInterestManager->Update(frameDeltaTime);
			}
	}

	{
		CDebugDrawContext dc;
		
		{
			FRAME_PROFILER("GlobalRayCaster", gEnv->pSystem, PROFILE_AI)

			gAIEnv.pRayCaster->Update(frameDeltaTime);
		}

		{
			FRAME_PROFILER("GlobalIntersectionTester", gEnv->pSystem, PROFILE_AI)

			gAIEnv.pIntersectionTester->Update(frameDeltaTime);
		}

		if (gAIEnv.CVars.DebugDrawPhysicsAccess)
		{
			SAIEnvironment::GlobalRayCaster::ContentionStats rstats = gAIEnv.pRayCaster->GetContentionStats();
			stack_string text;

			text.Format(
				"RayCaster\n"
				"---------\n"
				"Quota: %d\n"
				"Queue Size: %d\n"
				"Max Queue Size: %d\n"
				"Immediate Count: %d\n"
				"Immediate Average: %.1f\n"
				"Deferred Count: %d\n"
				"Deferred Average: %.1f", 
				rstats.quota,
				rstats.queueSize,
				rstats.maxQueueSize,
				rstats.immediateCount,
				rstats.immediateAverage,
				rstats.deferredCount,
				rstats.deferredAverage);

			bool warning = (rstats.immediateCount + rstats.deferredCount) > rstats.quota;
			warning = warning || (rstats.immediateAverage + rstats.deferredAverage) > rstats.quota;
			warning = warning || rstats.queueSize > (3 * rstats.quota);

			dc->Draw2dLabel(400.0, 745.0f, 1.25f, warning ? Col_Red : Col_DarkTurquoise, false, "%s", text.c_str());

			SAIEnvironment::GlobalIntersectionTester::ContentionStats istats = gAIEnv.pIntersectionTester->GetContentionStats();
			text.Format(
				"IntersectionTester\n"
				"------------------\n"
				"Quota: %d\n"
				"Queue Size: %d\n"
				"Max Queue Size: %d\n"
				"Immediate Count: %d\n"
				"Immediate Average: %.1f\n"
				"Deferred Count: %d\n"
				"Deferred Average: %.1f", 
				istats.quota,
				istats.queueSize,
				istats.maxQueueSize,
				istats.immediateCount,
				istats.immediateAverage,
				istats.deferredCount,
				istats.deferredAverage);

			warning = (istats.immediateCount + istats.deferredCount) > istats.quota;
			warning = warning || (istats.immediateAverage + istats.deferredAverage) > istats.quota;
			warning = warning || istats.queueSize > (3 * istats.quota);

			dc->Draw2dLabel(600.0, 745.0f, 1.25f, warning ? Col_Red : Col_DarkTurquoise, false, "%s", text.c_str());
		}
	}

	// Monitor code coverage
	int nCodeCoverageMode = gAIEnv.CVars.CodeCoverage;

	// First, try to load context if required
	CCodeCoverageManager *pCCManager = gAIEnv.pCodeCoverageManager;
	if (nCodeCoverageMode && !m_bCodeCoverageFailed && !pCCManager->IsContextValid())
	{
		string filePath = PathUtil::Make( m_sWorkingFolder, sCodeCoverageContextFile );
		FILE * ccFile = gEnv->pCryPak->FOpen(filePath.c_str(), "r");
		bool bOk = false;
		if (ccFile)
		{
			bOk = pCCManager->ReadCodeCoverageContext( ccFile );
			gEnv->pCryPak->FClose(ccFile);

			if (bOk)
				AILogAlways("CodeCoverageManager: Successfully loaded code coverage context file \"%s\"", filePath.c_str());
			else
				AILogAlways("CodeCoverageManager: Failed during read of code coverage context file \"%s\"", filePath.c_str());
		}
		else
		{
			AILogAlways("CodeCoverageManager: Failed to find code coverage context file \"%s\"", filePath.c_str());
		}

		m_bCodeCoverageFailed = !bOk;
	}

	if (gAIEnv.pCodeCoverageManager && gAIEnv.pCodeCoverageManager->IsContextValid())
		gAIEnv.pCodeCoverageManager->Update();
	if (nCodeCoverageMode > 0)
		gAIEnv.pCodeCoverageGUI->Update(frameStartTime, frameDeltaTime);

	// Update asynchronous TPS processing
	float fMaxTime = gAIEnv.CVars.TacticalPointUpdateTime;
	gAIEnv.pTacticalPointSystem->Update(fMaxTime);

	// Housekeeping
	gAIEnv.pObjectContainer->ReleaseDeregisteredObjects();
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::UpdateBeacon(unsigned short nGroupID, const Vec3 & vPos, IAIObject *pOwner)
{
	Vec3 pos = vPos;
	ray_hit hit;

	CAIActor* pOwnerActor = CastToCAIActorSafe(pOwner);
//FIXME: this should never happen! 
	if(!pOwner)
	{
		AIAssert(false);
		return;
	}

	BeaconMap::iterator bi;
	bi = m_mapBeacons.find(nGroupID);

	if ( bi== m_mapBeacons.end())
	{
		CCCPOINT(CAISystem_UpdateBeacon_New)

		BeaconStruct bs;
    char name[32];
    sprintf(name,"BEACON_%d", nGroupID);
		CStrongRef<CAIObject> refBeacon;
		CreateDummyObject(refBeacon, name);
		CAIObject *pObject = refBeacon.GetAIObject();

		pObject->SetPos(pos);
    pObject->SetMoveDir(pOwner->GetMoveDir());
    pObject->SetBodyDir(pOwner->GetBodyDir());
		pObject->SetViewDir(pOwner->GetViewDir());
		pObject->SetType(AIOBJECT_WAYPOINT);
		pObject->SetSubType(IAIObject::STP_BEACON);
		pObject->SetGroupId(nGroupID);

		bs.refBeacon = refBeacon;
		m_mapBeacons.insert(BeaconMap::iterator::value_type(nGroupID,bs));
	}
	else
	{
		CCCPOINT(CAISystem_UpdateBeacon_Reuse)
		// (MATT) It's possible that the owner is now outdated, but we reset that anyway. {2009/02/16}

		// beacon found, update its position
		(bi->second).refBeacon->SetPos(pos);
    (bi->second).refBeacon->SetBodyDir(pOwner->GetBodyDir());
    (bi->second).refBeacon->SetMoveDir(pOwner->GetMoveDir());
		(bi->second).refBeacon->SetViewDir(pOwner->GetViewDir());
		(bi->second).refOwner.Reset();
	}
}

//
//-----------------------------------------------------------------------------------------------------------
IAIObject * CAISystem::GetBeacon(unsigned short nGroupID)
{
	BeaconMap::iterator bi;
	bi = m_mapBeacons.find(nGroupID);
	if ( bi== m_mapBeacons.end())
		return NULL;

	CCCPOINT(CAISystem_GetBeacon)

	return (bi->second).refBeacon.GetAIObject();
}

//
//-----------------------------------------------------------------------------------------------------------
int CAISystem::GetBeaconGroupId(CAIObject* pBeacon)
{
	BeaconMap::iterator bi, biEnd = m_mapBeacons.end();
	
	for(bi = m_mapBeacons.begin(); bi != biEnd; ++bi)
		if ( bi->second.refBeacon == pBeacon)
		{
			CCCPOINT(CAISystem_GetBeaconGroupId);

			return bi->first;
		}
	return -1;
}



//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::SetAssesmentMultiplier(unsigned short type, float fMultiplier)
{
	if (fMultiplier<=0.f)
		fMultiplier = 0.01f;
	MapMultipliers::iterator mi;
	
	mi = m_mapMultipliers.find(type);
	if (mi== m_mapMultipliers.end())
		m_mapMultipliers.insert(MapMultipliers::iterator::value_type(type,fMultiplier));
	else
		mi->second = fMultiplier;

	if (std::find(m_priorityObjectTypes.begin(), m_priorityObjectTypes.end(), type) == m_priorityObjectTypes.end())
		m_priorityObjectTypes.push_back(type);
}




//
//-----------------------------------------------------------------------------------------------------------
IAIObject * CAISystem::GetNearestToObjectInRange(IAIObject * pRef, unsigned short nType, float fRadiusMin, 
																								 float fRadiusMax, float inCone, bool bFaceAttTarget, bool bSeesAttTarget, bool bDevalue )
{
	AIObjectOwners::iterator ai;
	ai = m_Objects.find(nType);

	float	fRadiusMinSQR = fRadiusMin*fRadiusMin;
	float	fRadiusMaxSQR = fRadiusMax*fRadiusMax;

	if (ai == m_Objects.end()) 
		return NULL;

	CPipeUser* pPipeUser = pRef->CastToCPipeUser();
	CAIObject *pAttTarget = 0;
	if (pPipeUser)
		pAttTarget = (CAIObject*)pPipeUser->GetAttentionTarget();
	
	IAIObject *pRet = 0;
	CPuppet* pPuppet = pRef->CastToCPuppet();
	float	eyeOffset = 0.f;
	if(pPuppet && bSeesAttTarget)
	{
		SAIBodyInfo bi;
		pPuppet->GetProxy()->QueryBodyInfo(SAIBodyInfoQuery(STANCE_CROUCH, 0.0f, 0.0f, true), bi);
		eyeOffset = bi.vEyePos.z - pPuppet->GetEntity()->GetWorldPos().z;
	}
	float maxdist = std::numeric_limits<float>::max();
	Vec3 pos = pRef->GetPos();
	for (;ai!=m_Objects.end();++ai)
	{
		CAIObject * const pObject = ai->second.GetAIObject();
		if (ai->first != nType)
			break;
		if(!pObject || !pObject->IsEnabled())
			continue;
		Vec3 ob_pos = pObject->GetPos();
		Vec3 ob_dir = ob_pos-pos;
		float f = ob_dir.GetLengthSquared();

		// only consider objects in cone
		if(inCone>0.0f)
		{
			ob_dir.Normalize();
			float	dot = pRef->GetMoveDir().Dot(ob_dir);
			if( dot < inCone )
				continue;
			// Favor points which have tighter hide spot.
			float	a = 0.5f + 0.5f * (1.0f - max(0.0f, dot));
			f *= a * a;
		}

		if( bFaceAttTarget && pAttTarget )
		{
			Vec3	dirToAtt = pAttTarget->GetPos() - ob_pos;
			Vec3	coverDir = pObject->GetMoveDir();
			dirToAtt.NormalizeSafe();
			if( dirToAtt.Dot( coverDir ) < 0.3f )	// let's make it 70 degree threshold
				continue;
		}

		// (MATT) Crysis surely depends on this but it's not a nice behaviour {2008/11/11}
		if (pPuppet && (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS || gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2))
		{
			if (pPuppet->IsDevalued(pObject))
				continue;
		}

		//use anchor radius ---------------------
		float	fCurRadiusMaxSQR( (ai->second)->GetRadius()<0.01f ? fRadiusMaxSQR : (fRadiusMax+(ai->second)->GetRadius())*(fRadiusMax+(ai->second)->GetRadius()));
		if ( f>fRadiusMinSQR && f<fCurRadiusMaxSQR)
		{
			if ((f < maxdist))
			{
				if(bSeesAttTarget && pAttTarget)
				{
					int rwiResult(0);
					ray_hit hit;
					float terrainZ = gEnv->p3DEngine->GetTerrainElevation(ob_pos.x, ob_pos.y);
					Vec3	startTracePos( ob_pos.x, ob_pos.y, terrainZ+eyeOffset );
					Vec3	ob_at_dir = pAttTarget->GetPos() - startTracePos;
					std::vector<IPhysicalEntity*> skipList;
					pPuppet->GetPhysicsEntitiesToSkip(skipList);
					if(pAttTarget && pAttTarget->CastToCAIActor())
						pAttTarget->CastToCAIActor()->GetPhysicsEntitiesToSkip(skipList);
					rwiResult = gAIEnv.pWorld->RayWorldIntersection( startTracePos, ob_at_dir, ent_static|ent_terrain|ent_sleeping_rigid, rwi_stop_at_pierceable, &hit, 1,
																												skipList.empty() ? 0 : &skipList[0], skipList.size());
					if(rwiResult)
						continue;
				}
				pRet = pObject;
				maxdist = f;
			}
		}
	}
	if (pRet)
	{
		if(!bDevalue )
			Devalue( pRef, pRet, true, .05f );	// no devalue - just make sure it's not used in the same update again
		else
		Devalue( pRef, pRet, true );
	}
	return pRet; 
} 


//
//-----------------------------------------------------------------------------------------------------------
IAIObject * CAISystem::GetRandomObjectInRange(IAIObject * pRef, unsigned short nType, float fRadiusMin, float fRadiusMax, bool bFaceAttTarget)
{
	// Make sure there is at least one object of type present.
	AIObjectOwners::iterator ai;
	ai = m_Objects.find(nType);
	if (ai == m_Objects.end()) 
		return NULL;

	CPipeUser* pPipeUser = pRef->CastToCPipeUser();
	IAIObject *pAttTarget = 0;
	if (bFaceAttTarget && pPipeUser)
			pAttTarget = pPipeUser->GetAttentionTarget();

	// Collect all the points within the given range.
	const float	fRadiusMinSQR = fRadiusMin*fRadiusMin;
	const float	fRadiusMaxSQR = fRadiusMax*fRadiusMax;
	std::list<IAIObject*>	lstObjectsInRange;
	Vec3 pos = pRef->GetPos();
	CPuppet* pPuppet = pRef->CastToCPuppet();
	for (;ai!=m_Objects.end();++ai)
	{
		// Skip objects of wrong type.
		if (ai->first != nType)
			break;

		// Strong so always valid
		CAIObject *pObject = ai->second.GetAIObject();

		// Skip disable objects.
		if(!pObject->IsEnabled())
			continue;
		
		// Skip devalued objects.
		// (MATT) Crysis surely depends on this but it's not a nice behaviour {2008/11/11}
		if (pPuppet && (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS || gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2))
		{
			if (pPuppet->IsDevalued(pObject))
				continue;
		}

		// check if facing target
		if(pAttTarget)
		{
			Vec3 candidate2Target(pAttTarget->GetPos()-pObject->GetPos());
			float	dotS(pObject->GetMoveDir()*candidate2Target);
			if(dotS<0.f)
				continue;
		}
		// Skip objects out of range.
		Vec3 ob_pos = pObject->GetPos();
		Vec3 ob_dir = ob_pos-pos;
		float f = ob_dir.GetLengthSquared();
		//use anchor radius ---------------------
		float	fCurRadiusMaxSQR( pObject->GetRadius()<0.01f ? fRadiusMaxSQR : (fRadiusMax+pObject->GetRadius())*(fRadiusMax+pObject->GetRadius()));
		if ( f>fRadiusMinSQR && f<fCurRadiusMaxSQR )
			lstObjectsInRange.push_back( pObject );
	}

	// Choose random object.
	IAIObject *pRet = 0;
	if( !lstObjectsInRange.empty() )
	{
		int	choice = ai_rand()%lstObjectsInRange.size();
		std::list<IAIObject*>::iterator randIt = lstObjectsInRange.begin();
		std::advance( randIt, choice );
		if( randIt != lstObjectsInRange.end() )
			pRet = (*randIt);
	}

	if (pRet && pPuppet)
		pPuppet->Devalue( pRet, false, 2.f);

	return pRet; 
} 

//-----------------------------------------------------------------------------------------------------------
IAIObject * CAISystem::GetBehindObjectInRange(IAIObject * pRef, unsigned short nType, float fRadiusMin, float fRadiusMax)
{
	// Find an Object to escape from a target. 04/11/05 tetsuji

	// Make sure there is at least one object of type present.
	AIObjectOwners::iterator ai;
	ai = m_Objects.find(nType);
	if (ai == m_Objects.end()) 
		return NULL;

	CPipeUser* pPipeUser = pRef->CastToCPipeUser();
	assert(pPipeUser);

	CAIObject *pAttTarget = 0;
	if (pPipeUser)
		pAttTarget = (CAIObject*)pPipeUser->GetAttentionTarget();

	// Collect all the points within the given range.
	const float	fRadiusMinSQR = fRadiusMin*fRadiusMin;
	const float	fRadiusMaxSQR = fRadiusMax*fRadiusMax;
	std::list<IAIObject*>	lstObjectsInRange;

	Vec3 pos = pRef->GetPos();
	Vec3 vForward = pRef->GetMoveDir();

	vForward.SetLength(10.0);

	// If no att target, I assume the target is the front point of the object.
	// 20/12/05 Tetsuji
	Vec3 vTargetPos = pAttTarget ? pAttTarget->GetPos() : pos + vForward;
	Vec3 vTargetToPos = pos - vTargetPos;

	if(!pPipeUser->GetMovementAbility().b3DMove)
		vTargetToPos.z = 0.f;
	vTargetToPos.NormalizeSafe();

	for (;ai!=m_Objects.end();++ai)
	{
		// Skip objects of wrong type.
		if (ai->first != nType)
			break;

		// Strong so always valid
		CAIObject *pObject = ai->second.GetAIObject();

		// Skip disable objects.
		if(!pObject->IsEnabled())
			continue;

		// Skip devalued objects.
		// (MATT) Crysis surely depends on this but it's not a nice behaviour {2008/11/11}
		CPuppet* pPuppet = pRef->CastToCPuppet();
		if (pPuppet && (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS || gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2))
		{
			if (pPuppet->IsDevalued(pObject))
				continue;
		}

		// Skip objects out of range.
		Vec3 ob_pos = pObject->GetPos();
		Vec3 ob_dir = ob_pos-pos;
		float f = ob_dir.GetLengthSquared();
		//use anchor radius ---------------------
		float	fCurRadiusMaxSQR( pObject->GetRadius()<0.01f ? fRadiusMaxSQR : (fRadiusMax+pObject->GetRadius())*(fRadiusMax+(pObject->GetRadius())));
		if ( f>fRadiusMinSQR && f<fCurRadiusMaxSQR )
			lstObjectsInRange.push_back( pObject );
	}

	// Choose object. 
	IAIObject *pRet = 0;
	float maxdot = -10.0f;

	if( !lstObjectsInRange.empty() )
	{
		std::list<IAIObject*>::iterator choiceIt = lstObjectsInRange.begin();
		std::list<IAIObject*>::iterator choiceItEnd = lstObjectsInRange.end();
		for (;choiceIt!=choiceItEnd;++choiceIt)
		{
			IAIObject *pObj =*choiceIt;
			Vec3 vPosToObj =pObj->GetPos() - pos;
			CPuppet* pPuppet = pRef->CastToCPuppet();
			if(pPuppet && !pPuppet->GetMovementAbility().b3DMove)
				vPosToObj.z = 0.f;

			if (pPuppet && !pPuppet->CheckFriendsInLineOfFire(vPosToObj, true))
					continue;

			float dot =vPosToObj.Dot(vTargetToPos);
			if(dot < .2f)
				continue;
			if(dot>maxdot)
			{
				maxdot =dot;
				pRet =pObj;
			}
		}
	}

	if (pRet)
		Devalue( pRef, pRet, true );

	return pRet; 
} 


//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::Devalue( IAIObject* pRef, IAIObject* pObject, bool group, float fDevalueTime )
{
	if( !pRef || !pObject )
		return;

	CPipeUser* pPipeUser = pRef->CastToCPipeUser();
	if (!pPipeUser)
		return;

	if( group )
	{
		// Devalue the object for whole group.
		int myGroup = pPipeUser->GetGroupId();
		AIObjects::iterator gri = m_mapGroups.find(myGroup);
		for( ; gri != m_mapGroups.end(); ++gri )
		{
			if (gri->first != myGroup)
				break;
			CPuppet* pGroupMember = CastToCPuppetSafe(gri->second.GetAIObject());
			if (pGroupMember)
				pGroupMember->Devalue((CAIObject*)pObject, false, fDevalueTime);
		}
	}
	else
	{
		// Devalue the object for one puppet
		if ( CPuppet *pPuppet = pRef->CastToCPuppet() )
			pPuppet->Devalue((CAIObject*)pObject, false, fDevalueTime);
	}
}

//
//-----------------------------------------------------------------------------------------------------------
IAIObject * CAISystem::GetNearestObjectOfTypeInRange(const Vec3 &pos, unsigned int nTypeID, float fRadiusMin, float fRadiusMax, IAIObject* pSkip, int nOption)
{
	IAIObject *pRet = 0;
	float mindist = 100000000;
	const float fRadiusMinSQR = fRadiusMin*fRadiusMin;
	const float fRadiusMaxSQR = fRadiusMax*fRadiusMax;

	AIObjectOwners::iterator ai = m_Objects.find(nTypeID), aiEnd = m_Objects.end();
	for ( ;ai != aiEnd && ai->first == nTypeID; ++ai )
	{
		CAIObject* object = ai->second.GetAIObject();
		if( pSkip == object || !(nOption & AIFAF_INCLUDE_DISABLED) && !object->IsEnabled() )
			continue;

		float f = (object->GetPos()-pos).GetLengthSquared();
		//use anchor radius ---------------------
		float	fCurRadiusMaxSQR( object->GetRadius()<0.01f ? fRadiusMaxSQR : (fRadiusMax+object->GetRadius())*(fRadiusMax+object->GetRadius()));
		if ( f < mindist && f > fRadiusMinSQR && f < fCurRadiusMaxSQR )
		{
			pRet = object;
			mindist = f;
		}
	}

	return pRet;
}



//
//-----------------------------------------------------------------------------------------------------------
IAIObject * CAISystem::GetNearestObjectOfTypeInRange(IAIObject * pRequester, unsigned int nTypeID, float fRadiusMin, float fRadiusMax, int nOption)
{
	CCCPOINT(GetNearestObjectOfTypeInRange);

	// (MATT) This method seems to try to work with non-puppets, but it's fishy {2008/11/12}
	CPuppet* pPuppet = CastToCPuppetSafe(pRequester);
	if (!pPuppet) AIWarning("GetNearestObjectOfTypeInRange passed a non-puppet");

	Vec3 reqPos = pRequester->GetPos();
	if( nOption & AIFAF_USE_REFPOINT_POS )
	{
		if( !pPuppet || !pPuppet->GetRefPoint() )
			return 0;
		reqPos = pPuppet->GetRefPoint()->GetPos();
	}

	IAIObject* pRet = 0;
	float mindist = 100000000;
	const float fRadiusMinSQR = fRadiusMin*fRadiusMin;
	const float fRadiusMaxSQR = fRadiusMax*fRadiusMax;

	AIObjectOwners::iterator ai = m_Objects.find(nTypeID), aiEnd = m_Objects.end();
	for ( ;ai != aiEnd && ai->first == nTypeID; ++ai )
	{
		CAIObject* pObj = ai->second.GetAIObject();
		if ( pObj == pRequester || !(nOption & AIFAF_INCLUDE_DISABLED) && !pObj->IsEnabled() )
			continue;
		if (nOption & AIFAF_SAME_GROUP_ID) 
		{
			CAIActor* pActor = pObj->CastToCAIActor();
			if (pActor && pActor->GetGroupId() != pPuppet->GetGroupId())
			continue;
		}

		const Vec3& objPos = pObj->GetPos();
		float f = (objPos - reqPos).GetLengthSquared();
		if ( f < mindist && f > fRadiusMinSQR && f < fRadiusMaxSQR )
		{
			// (MATT) Crysis surely depends on this but it's not a nice behaviour {2008/11/11}
			if ( (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS || gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2) ||
						(nOption & AIFAF_INCLUDE_DEVALUED) ||
						( pPuppet && !pPuppet->IsDevalued(pObj)) )
			 {
				 if (nOption & AIFAF_HAS_FORMATION && !pObj->m_pFormation )
				 {
					 continue;
				 }

				if (nOption & AIFAF_VISIBLE_FROM_REQUESTER)
				{
					ray_hit rh;
					int colliders(0);
					colliders = gAIEnv.pWorld->RayWorldIntersection(objPos, reqPos - objPos, COVER_OBJECT_TYPES, HIT_COVER|HIT_SOFT_COVER, &rh, 1);
					if (colliders)
						continue;
				}
				if( nOption & (AIFAF_LEFT_FROM_REFPOINT | AIFAF_RIGHT_FROM_REFPOINT) )
				{
					if( pPuppet && pPuppet->GetRefPoint() )
					{
						const Vec3	one = pPuppet->GetRefPoint()->GetPos() - reqPos;
						const Vec3	two = objPos - reqPos;

						float zcross = one.x * two.y - one.y * two.x;

						if( nOption & AIFAF_LEFT_FROM_REFPOINT )
						{
							if( zcross < 0 )
								continue;
						}
						else
						{
							if( zcross > 0 )
								continue;
						}
					}
				}

				if (nOption & AIFAF_INFRONT_OF_REQUESTER)
				{
					const Vec3	toTargetDir((objPos - reqPos).GetNormalizedSafe());
					float diffCosine(toTargetDir*pPuppet->m_State.vMoveDir);

					if(diffCosine<.7f)
						continue;
				}

				pRet = pObj;
				mindist = f;
			}
		}
	}

	if (pRet && !(nOption & AIFAF_DONT_DEVALUE))
		Devalue( pRequester, pRet, true );

	return pRet;
}

//
//-----------------------------------------------------------------------------------------------------------
bool CAISystem::DoesNavigationShapeExists(const char * szName, EnumAreaType areaType, bool road)
{
	if (m_pNavigation->DoesNavigationShapeExists(szName,areaType,road))
		return true;

	if (areaType == AREATYPE_OCCLUSION_PLANE)
	{
		return m_mapOcclusionPlanes.find(szName) != m_mapOcclusionPlanes.end();
	}
	else if (areaType == AREATYPE_GENERIC)
	{
		return m_mapGenericShapes.find(szName) != m_mapGenericShapes.end();
	}

	return false;
}

//
//-----------------------------------------------------------------------------------------------------------
bool CAISystem::CreateNavigationShape(const SNavigationShapeParams &params)
{
	// need at least one point in a path. Some paths need more than one (areas need 3)
	if (params.nPoints == 0)
		return true; // Do not report too few points as errors.

	// TODO Jan 31, 2008: <pvl> 'true' returned here can mean either area type
	// wasn't matched (in which case we should go on matching here) or that
	// there a shape was actually created already (meaning we should return
	// immediately).  In the latter case we have a bit of inefficiency here.
	if (m_pNavigation->CreateNavigationShape(params) == false)
		return false;

	std::vector<Vec3> vecPts(params.points, params.points + params.nPoints);

	if (params.areaType==AREATYPE_PATH && params.pathIsRoad == false )
	{
		//designer path need to preserve directions
	}
	else
	{
  if (params.closed)
    EnsureShapeIsWoundAnticlockwise<std::vector<Vec3>, float>(vecPts);
	}

  ListPositions listPts(vecPts.begin(), vecPts.end());

	if (params.areaType == AREATYPE_OCCLUSION_PLANE)
	{
		if (listPts.size() < 3)
			return true; // Do not report too few points as errors.

		if (m_mapOcclusionPlanes.find(params.szPathName) != m_mapOcclusionPlanes.end())
		{
			AIError("CAISystem::CreateNavigationShape: Occlusion plane '%s' already exists, please rename the shape.", params.szPathName);
			return false;
		}

			m_mapOcclusionPlanes.insert(ShapeMap::iterator::value_type(params.szPathName,SShape(listPts)));
	}
	else if (params.areaType == AREATYPE_PERCEPTION_MODIFIER)
	{
		if (listPts.size() < 2)
			return false;

		PerceptionModifierShapeMap::iterator di;
		di = m_mapPerceptionModifiers.find(params.szPathName);

		if (di== m_mapPerceptionModifiers.end())
		{
			SPerceptionModifierShape pms(listPts, params.fReductionPerMetre, params.fReductionMax, params.fHeight, params.closed);
			m_mapPerceptionModifiers.insert(PerceptionModifierShapeMap::iterator::value_type(params.szPathName,pms));
		}
		else
			return false;
	}
	else if (params.areaType == AREATYPE_GENERIC)
	{
		if (listPts.size() < 3)
			return true; // Do not report too few points as errors.

		if (m_mapGenericShapes.find(params.szPathName) != m_mapGenericShapes.end())
		{
			AIError("CAISystem::CreateNavigationShape: Shape '%s' already exists, please rename the shape.", params.szPathName);
			return false;
		}

			m_mapGenericShapes.insert(ShapeMap::iterator::value_type(params.szPathName,
				SShape(listPts, false, IAISystem::NAV_UNSET, params.nAuxType, params.fHeight, params.lightLevel)));
	}

	return true;
}

// deletes designer created path
//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DeleteNavigationShape(const char * szName)
{
	ShapeMap::iterator di;

	m_pNavigation->DeleteNavigationShape(szName);

	di = m_mapGenericShapes.find(szName);
	if (di!= m_mapGenericShapes.end())
	{
		m_mapGenericShapes.erase(di);
		// Make sure there is no dangling pointers left.
		AIObjectOwners::iterator ai = m_Objects.find(AIOBJECT_PUPPET);
		for(; ai != m_Objects.end(); ++ai)
		{
			if(ai->first != AIOBJECT_PUPPET) break;
			CPuppet *pPuppet = ai->second.GetAIObject()->CastToCPuppet();
			if(pPuppet)
			{
				if(strcmp(pPuppet->GetRefShapeName(), szName) == 0)
					pPuppet->SetRefShapeName(0);
				if(strcmp(pPuppet->GetTerritoryShapeName(), szName) == 0)
					pPuppet->SetTerritoryShapeName(0);
			}
		}
	}

	di = m_mapOcclusionPlanes.find(szName);
	if (di!= m_mapOcclusionPlanes.end())
		m_mapOcclusionPlanes.erase(di);

	PerceptionModifierShapeMap::iterator pmsi = m_mapPerceptionModifiers.find(szName);
	if (pmsi != m_mapPerceptionModifiers.end())
		m_mapPerceptionModifiers.erase(pmsi);

	// Light manager might have pointers to shapes and areas, update it.
	m_lightManager.Update(true);
}

//====================================================================
// GetEnclosingGenericShapeOfType
//====================================================================
const char*	CAISystem::GetEnclosingGenericShapeOfType(const Vec3& reqPos, int type, bool checkHeight)
{
	ShapeMap::iterator end = m_mapGenericShapes.end();
	ShapeMap::iterator nearest = end;
	float	nearestDist = FLT_MAX;
	for(ShapeMap::iterator it = m_mapGenericShapes.begin(); it != end; ++it)
	{
		const SShape&	shape = it->second;
		if(!shape.enabled) continue;
		if(shape.type != type) continue;
		if(shape.IsPointInsideShape(reqPos, (type == AIANCHOR_COMBAT_TERRITORY) ? shape.height >= 0.01f : checkHeight))
		{
			float dist = 0;
			Vec3	pt;
			shape.NearestPointOnPath(reqPos, dist, pt);
			if(dist < nearestDist)
			{
				nearest = it;
				nearestDist = dist;
			}
		}
	}

	if(nearest != end)
		return nearest->first.c_str();

	return 0;
}

//====================================================================
// DistanceToGenericShape
//====================================================================
float CAISystem::DistanceToGenericShape(const Vec3& reqPos, const char* shapeName, bool checkHeight)
{
	if(!shapeName || shapeName[0] == 0)
		return 0.0f;

	ShapeMap::iterator	it = m_mapGenericShapes.find(shapeName);
	if(it == m_mapGenericShapes.end())
	{
		AIWarning("CAISystem::DistanceToGenericShape Unable to find generic shape called %s", shapeName);
		return 0.0f;
	}
	const SShape&	shape = it->second;
	float	dist;
	Vec3	nearestPt;
	shape.NearestPointOnPath(reqPos, dist, nearestPt);
	if(checkHeight)
		return dist;
	return Distance::Point_Point2D(reqPos, nearestPt);
}

//====================================================================
// IsPointInsideGenericShape
//====================================================================
bool CAISystem::IsPointInsideGenericShape(const Vec3& reqPos, const char* shapeName, bool checkHeight)
{
	if(!shapeName || shapeName[0] == 0)
		return false;

	ShapeMap::iterator	it = m_mapGenericShapes.find(shapeName);
	if(it == m_mapGenericShapes.end())
	{
		AIWarning("CAISystem::IsPointInsideGenericShape Unable to find generic shape called %s", shapeName);
		return false;
	}
	const SShape&	shape = it->second;
	return shape.IsPointInsideShape(reqPos, checkHeight);
}

//====================================================================
// ConstrainInsideGenericShape
//====================================================================
bool CAISystem::ConstrainInsideGenericShape(Vec3& pos, const char* shapeName, bool checkHeight)
{
	if(!shapeName || shapeName[0] == 0)
		return false;

	ShapeMap::iterator	it = m_mapGenericShapes.find(shapeName);
	if(it == m_mapGenericShapes.end())
	{
		AIWarning("CAISystem::ConstrainInsideGenericShape Unable to find generic shape called %s", shapeName);
		return false;
	}
	const SShape&	shape = it->second;
	return shape.ConstrainPointInsideShape(pos, checkHeight);
}

//====================================================================
// GetGenericShapeOfName
//====================================================================
SShape* CAISystem::GetGenericShapeOfName(const char* shapeName)
{
	if(!shapeName || shapeName[0] == 0)
		return 0;

	ShapeMap::iterator	it = m_mapGenericShapes.find(shapeName);
	if(it == m_mapGenericShapes.end())
		return 0;
	return &it->second;
}

//====================================================================
// CreateTemporaryGenericShape
//====================================================================
const char*	CAISystem::CreateTemporaryGenericShape(Vec3* points, int npts, float height, int type)
{
	if(npts < 2)
		return 0;

	// Make sure the shape is wound clockwise.
	std::vector<Vec3> vecPts(points, points + npts);
	EnsureShapeIsWoundAnticlockwise<std::vector<Vec3>, float>(vecPts);
	ListPositions listPts(vecPts.begin(), vecPts.end());

	// Create random name for the shape.
	char	name[16];
	ShapeMap::iterator di;
	do 
	{
		_snprintf(name, 16, "Temp%08x", ai_rand());
		di = m_mapGenericShapes.find(name);
	}
	while(di != m_mapGenericShapes.end());

	std::pair<ShapeMap::iterator, bool > pr;

	pr = m_mapGenericShapes.insert(ShapeMap::iterator::value_type(name, SShape(listPts, false, IAISystem::NAV_UNSET, type, height, AILL_NONE, true)));
	if(pr.second == true)
		return (pr.first)->first.c_str();
	return 0;
}

//====================================================================
// EnableGenericShape
//====================================================================
void CAISystem::EnableGenericShape(const char* shapeName, bool state)
{
	if(!shapeName || strlen(shapeName) < 1)
		return;
	ShapeMap::iterator	it = m_mapGenericShapes.find(shapeName);
	if(it == m_mapGenericShapes.end())
	{
		AIWarning("CAISystem::EnableGenericShape Unable to find generic shape called %s", shapeName);
		return;
	}

	SShape&	shape = it->second;

	// If the new state is the same, no need to inform the users.
	if(shape.enabled == state)
		return;

	// Change the state of the shape
	shape.enabled = state;

	// Notify the puppets that are using the shape that the shape state has changed.
	AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_PUPPET);
	for(; ai != m_Objects.end(); ++ai)
	{
		if (ai->first != AIOBJECT_PUPPET)
			break;

		CAIObject* obj = ai->second.GetAIObject();
		CPuppet* puppet = obj->CastToCPuppet();
		if(!puppet)
			continue;

		if(state)
		{
			// Shape enabled
			if(shape.IsPointInsideShape(puppet->GetPos(), true))
			{
				IAISignalExtraData* pData = CreateSignalExtraData();
				pData->SetObjectName(shapeName);
				pData->iValue = shape.type;
				puppet->SetSignal(1, "OnShapeEnabled", puppet->GetEntity(), pData, gAIEnv.SignalCRCs.m_nOnShapeEnabled);
			}
		}
		else
		{
			// Shape disabled
			int	val = 0;
			if(puppet->GetRefShapeName() && strcmp(puppet->GetRefShapeName(), shapeName) == 0)
				val |= 1;
			if(puppet->GetTerritoryShapeName() && strcmp(puppet->GetTerritoryShapeName(), shapeName) == 0)
				val |= 2;
			if(val)
			{
				IAISignalExtraData* pData = CreateSignalExtraData();
				pData->iValue = val;
				puppet->SetSignal(1, "OnShapeDisabled", puppet->GetEntity(), pData, gAIEnv.SignalCRCs.m_nOnShapeDisabled);
			}
		}
	}
}

void CAISystem::FlushAllAreas()
{
	// Flushes all non-navigation areas
	m_mapGenericShapes.clear();
	m_mapPerceptionModifiers.clear();
}



//====================================================================
// GetOccupiedHideObjectPositions
//====================================================================
void CAISystem::GetOccupiedHideObjectPositions(const CPipeUser* pRequester, std::vector<Vec3>& hideObjectPositions)
{
	CCCPOINT(GetOccupiedHideObjectPositions);

	// Iterate over the list of active puppets and collect positions of the valid hidespots in use.
	hideObjectPositions.clear();
	
	PuppetSet::const_iterator it = m_enabledPuppetsSet.begin();
	PuppetSet::const_iterator end = m_enabledPuppetsSet.end();

	for ( ; it != end; ++it)
	{
		CPipeUser* pOther = it->GetAIObject()->CastToCPipeUser();
		if (!pOther || (pRequester && (pRequester == pOther)))
			continue;

		// Include the position of each enemy regardless of the validity of the hidepoint itself - seems to get better results
		hideObjectPositions.push_back(pOther->GetPos());

		if (!pOther->m_CurrentHideObject.IsValid())
			continue;

		// One reason why it can be important to also add the hidepoint itself is because the AI may not have reached it yet
		hideObjectPositions.push_back(pOther->m_CurrentHideObject.GetObjectPos());
	}

	OccupiedHideSpotPositions::const_iterator oit = m_occupiedHideSpotPositions.begin();
	OccupiedHideSpotPositions::const_iterator oend = m_occupiedHideSpotPositions.end();
	
	for ( ; oit != oend; ++oit)
		hideObjectPositions.push_back(*oit);
}

void CAISystem::SetHideSpotOccupied(const Vec3& pos, bool occupied)
{
	if (occupied)
		m_occupiedHideSpotPositions.push_back(pos);
	else
	{
		OccupiedHideSpotPositions::iterator it = m_occupiedHideSpotPositions.begin();

		while(it != m_occupiedHideSpotPositions.end())
		{
			if ((*it - pos).len2() < 0.25f * 0.25f)
				it = m_occupiedHideSpotPositions.erase(it);
			else
				++it;
		}
	}
}

// Márcio: Seriously, we need to get some kind of ID system for HideSpots.
// God kills a kitten each time he looks at this code.
bool CAISystem::IsHideSpotOccupied(CPipeUser* pRequester, const Vec3& pos) const
{
	PuppetSet::const_iterator it = m_enabledPuppetsSet.begin();
	PuppetSet::const_iterator end = m_enabledPuppetsSet.end();

	for ( ; it != end; ++it)
	{
		CPipeUser* pOther = it->GetAIObject()->CastToCPipeUser();
		if (!pOther || (pRequester && (pRequester == pOther)))
			continue;

		if ((pOther->GetPos() - pos).len2() < 0.5*0.5f)
			return true;

		if (!pOther->m_CurrentHideObject.IsValid())
			continue;

		if ((pOther->m_CurrentHideObject.GetObjectPos() - pos).len2() < 0.5*0.5f)
			return true;
	}

	OccupiedHideSpotPositions::const_iterator oit = m_occupiedHideSpotPositions.begin();
	OccupiedHideSpotPositions::const_iterator oend = m_occupiedHideSpotPositions.end();

	for ( ; oit != oend; ++oit)
	{
		if (((*oit) - pos).len2() < 0.5f*0.5f)
			return true;
	}

	return false;
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::NotifyEnableState(CPuppet* pPuppet, bool state)
{
	// (MATT)  
	// The puppet should, ideally, be both erased and then inserted
	// In many cases the method is called when the state is unchanged and so the puppet is neither erased or inserted
	// On creation it will not be found for erase, but should be added anyway - should really be handled separately
	// If this happens later, it may be a bug - e.g. re-adding during puppet Release()
	// {2008/11/10}

	assert(pPuppet);
	CWeakRef<CPuppet> refPuppet = GetWeakRef(pPuppet);

	if (state)
	{
		bool bErased = (m_disabledPuppetsSet.erase(refPuppet) > 0);
		bool bInserted = m_enabledPuppetsSet.insert(refPuppet).second;
		if (bErased != bInserted)
		{
			if (bInserted) 
				AILogComment("Puppet %x %s added to enable list",(unsigned)(pPuppet),pPuppet->GetName());
			else 
				AIWarning("Puppet %x %s removed from disable, but already present in enable list",(unsigned)(pPuppet),pPuppet->GetName());
		}
		AILogComment("Puppet %x %s moved from disabled to enabled set A/B:1/1 %d %d",(unsigned)(pPuppet),pPuppet->GetName(),bErased,bInserted);
	}
	else
	{
		pPuppet->ClearProbableTargets();
		CPerceptionManager::GetInstance()->RemovePuppetLOSQueries(pPuppet);

		bool bErased = (m_enabledPuppetsSet.erase(refPuppet) > 0);
		bool bInserted = m_disabledPuppetsSet.insert(refPuppet).second;
		if (bErased != bInserted)
		{
			if (bInserted) 
				AILogComment("Puppet %x %s added to disable list",(unsigned)(pPuppet),pPuppet->GetName());
			else 
				AIWarning("Puppet %x %s removed from enable, but already present in disable list",(unsigned)(pPuppet),pPuppet->GetName());
		}
		AILogComment("Puppet %x %s moved from enabled to disabled set A/B:1/1 %d %d",(unsigned)(pPuppet),pPuppet->GetName(),bErased,bInserted);
	}
}

//
//-----------------------------------------------------------------------------------------------------------
const ObstacleData CAISystem::GetObstacle(int nIndex)
{
	return m_VertexList.GetVertex(nIndex);
}

// it removes all references to this object from all objects of the specified type
//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::RemoveObjectFromAllOfType(int nType, CAIObject* pRemovedObject)
{
	AIObjectOwners::iterator ai = m_Objects.lower_bound(nType);
	AIObjectOwners::iterator end = m_Objects.end();
	for (; ai != end && ai->first == nType; ++ai)
			ai->second.GetAIObject()->OnObjectRemoved(pRemovedObject);
		}

//TODO: find better solution - not to send a signal from there but notify AIProxy - make AIHAndler send the signal, same way OnEnemySeen works
//
// sand signal to all objects of type nType, which have the pDeadObject as attention targer
//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::NotifyTargetDead(CAIObject* pDeadObject)
{
	if (!pDeadObject)
		return;

	AIObjectOwners::iterator ai = m_Objects.find(AIOBJECT_PUPPET);
	AIObjectOwners::iterator end = m_Objects.end();
	// tell all objects of nType that this object is considered invalid
	for ( ; ai != end && ai->first == AIOBJECT_PUPPET; ++ai)
		{
		CPipeUser *pPipeUser = ai->second.GetAIObject()->CastToCPipeUser();
			if (pPipeUser)
			{
				if(pPipeUser->GetAttentionTarget() == pDeadObject)
					pPipeUser->SetSignal(0, "OnTargetDead", NULL, 0, gAIEnv.SignalCRCs.m_nOnTargetDead);
			}
		}

	// Check if the death of the actor was part of any player stunt.
	CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
	CAIActor* pDeadActor = pDeadObject->CastToCAIActor();
	if (pPlayer && pDeadActor)
	{
		if (pPlayer->IsPlayerStuntAffectingTheDeathOf(pDeadActor))
		{
			// Skip the nearest thrown entity, since it is potentially blocking the view to the corpse.
			EntityId nearestThrownEntId = pPlayer->GetNearestThrownEntity(pDeadActor->GetPos());
			IEntity* pNearestThrownEnt = nearestThrownEntId ? gEnv->pEntitySystem->GetEntity(nearestThrownEntId) : 0;
			IPhysicalEntity* pNearestThrownEntPhys = pNearestThrownEnt ? pNearestThrownEnt->GetPhysics() : 0;

			short gid = (short)pDeadObject->GetGroupId();
			AIObjects::iterator aiIt = m_mapGroups.find(gid);
			AIObjects::iterator endIt = m_mapGroups.end();
			for ( ; aiIt != endIt && aiIt->first == gid; ++aiIt)
			{
				CPuppet* pPuppet = CastToCPuppetSafe(aiIt->second.GetAIObject());
				if (!pPuppet) 
					continue;
				if (pPuppet->GetEntityID() == pDeadActor->GetEntityID()) 
					continue;
				float dist = FLT_MAX;
				if (!CheckVisibilityToBody(pPuppet, pDeadActor, dist, pNearestThrownEntPhys))
					continue;
				pPuppet->SetSignal(1, "OnGroupMemberMutilated", pDeadObject->GetEntity(), 0);
			}
		}
	}

}


//===================================================================
// ExitNodeImpossible
//===================================================================
bool CAISystem::ExitNodeImpossible(CGraphLinkManager& linkManager, const GraphNode * pNode, float fRadius) const
{
	for (unsigned gl = pNode->firstLinkIndex; gl; gl = linkManager.GetNextLink(gl))
{
		if (linkManager.GetRadius(gl) >= fRadius)
			return false;
	}
	return true;
}

//===================================================================
// EnterNodeImpossible
//===================================================================
bool CAISystem::EnterNodeImpossible(CGraphNodeManager& nodeManager, CGraphLinkManager& linkManager, const GraphNode * pNode, float fRadius) const
{
	for (unsigned link = pNode->firstLinkIndex; link; link = linkManager.GetNextLink(link))
	{
		unsigned nextNodeIndex = linkManager.GetNextNode(link);
		GraphNode* nextNode = nodeManager.GetNode(nextNodeIndex);
		unsigned incomingLink = nextNode->GetLinkTo(nodeManager, linkManager, pNode);
		if (incomingLink && linkManager.GetRadius(incomingLink) >= fRadius)
			return false;
	}
	return true;
}

//===================================================================
// SetSpeciesThreatMultiplier
//===================================================================
void CAISystem::SetSpeciesThreatMultiplier(int nSpeciesID, float fMultiplier)
{
	if (fMultiplier>1.f)
		fMultiplier = 1.f;

	if (fMultiplier< 0.f) // Modified from <= 0 -> = 0.01
		fMultiplier = 0.0f;

	// will use this multiplier any time a puppet perceives a target of these species
	MapMultipliers::iterator mi;
	
	mi = m_mapSpeciesThreatMultipliers.find(nSpeciesID);
	if (mi== m_mapSpeciesThreatMultipliers.end())
		m_mapSpeciesThreatMultipliers.insert(MapMultipliers::iterator::value_type(nSpeciesID,fMultiplier));
	else
		mi->second = fMultiplier;

}

//===================================================================
// IsAIInDevMode
//===================================================================
bool CAISystem::IsAIInDevMode()
{
  return gAIEnv.configuration.IsDevMode;
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DumpStateOf(IAIObject * pObject)
{
	AILogAlways("AIName: %s",pObject->GetName());
	CPuppet *pPuppet = CastToCPuppetSafe(pObject);
	if (pPuppet)
	{
		CGoalPipe *pPipe = pPuppet->GetCurrentGoalPipe();
		if (pPipe)
		{
			AILogAlways("Current pipes: %s", pPipe->GetName());
			while (pPipe->IsInSubpipe())
			{
				pPipe = pPipe->GetSubpipe();
				AILogAlways("   subpipe: %s", pPipe->GetName());
			}
		}
	}
}


//
//-----------------------------------------------------------------------------------------------------------
float CAISystem::GetRayPerceptionModifier(const Vec3 &start,const Vec3 &end, const char *actorName)
{
	const Lineseg ray(start,end);
	const float rayLen((end - start).len());
	const int max_intersects = 8;
	float intersects[max_intersects];
	float fPerception = 1.0f;
	Vec3 hit;

	int icvDrawPerceptionDebugging = gAIEnv.CVars.DrawPerceptionDebugging;

	PerceptionModifierShapeMap::iterator pmsi = m_mapPerceptionModifiers.begin(),pmsiEnd = m_mapPerceptionModifiers.end();
	for (;pmsi!=pmsiEnd;++pmsi)
	{
		SPerceptionModifierShape &shape = pmsi->second;
		if (shape.shape.empty()) continue;
		
		// Quick bounding box check, skip if no intersection and both points aren't inside AABB
		if (Intersect::Lineseg_AABB( ray, shape.aabb, hit) == 0x00 &&
				!(shape.aabb.IsContainPoint(start) && shape.aabb.IsContainPoint(end))) continue;

		// Proper test

		//char text[64]; //Diesel cut

		bool bStartInsideShape(shape.IsPointInsideShape(start, true));
		bool bEndInsideShape(shape.IsPointInsideShape(end, true));
		if (bStartInsideShape && bEndInsideShape)
		{
			float fMod = rayLen * shape.fReductionPerMetre; 
			fMod = clamp(fMod, 0.0f, shape.fReductionMax); 
			fPerception -= fMod;

#ifdef CRYAISYSTEM_DEBUG
			if (icvDrawPerceptionDebugging != 0)
			{
				_snprintf(text, sizeof(text), "%s-0", actorName?actorName:"");
				AddPerceptionDebugLine(text, start+Vec3(0.f,0.f,-0.1f), end+Vec3(0.f,0.f,-0.1f), 50, 255, 50, 1.f, 3.f);
			}
#endif //CRYAISYSTEM_DEBUG

		}
		else
		{
			int nIntersects = shape.GetIntersectionDistances(start, end, intersects, max_intersects, true, true);	
			if (nIntersects > 0)
			{
				// Special handling for open shapes 
				if (!shape.closed)
				{
					fPerception -= shape.fReductionMax * nIntersects;
					if( fPerception < 0.0f )
						fPerception = 0.0f;

#ifdef CRYAISYSTEM_DEBUG
					if (icvDrawPerceptionDebugging != 0)
					{						
						Vec3 intersectionPos;
						for (int i = 0; i < nIntersects; ++i )
						{
							_snprintf(text, sizeof(text), "%s-0-%d", actorName?actorName:"", i+1);
							intersectionPos = start + (intersects[i]/rayLen) * (end - start) + Vec3(0.f,0.f,-0.1f);
							AddPerceptionDebugLine(text, 
								Vec3(intersectionPos.x, intersectionPos.y, shape.aabb.min.z), 
								Vec3(intersectionPos.x, intersectionPos.y, shape.aabb.max.z), 
								50, 255, 50, 1.f, 3.f);
						}
					}
#endif //CRYAISYSTEM_DEBUG

					continue;
				}

				float fMod(0.f);
				int in, out;
				float inPos, outPos;
				if (bStartInsideShape)
				{
					in = -1; inPos = 0.f;
					out = 0; outPos = intersects[0];
				}
				else if (nIntersects > 1)
				{
					in = 0; inPos = intersects[0];
					out = 1; outPos = intersects[1];
				}
				else
				{
					out = nIntersects;
				}

				while (out < nIntersects)
				{
					fMod = (outPos - inPos) * shape.fReductionPerMetre; 
					fMod = clamp(fMod, 0.0f, shape.fReductionMax); 
					fPerception -= fMod;

#ifdef CRYAISYSTEM_DEBUG
					if (icvDrawPerceptionDebugging != 0)
					{
						Vec3 startPos( start + (inPos/rayLen) * (end - start) + Vec3(0.f,0.f,-0.1f) );
						Vec3 endPos( start + (outPos/rayLen) * (end - start) + Vec3(0.f,0.f,-0.1f) );
						_snprintf(text, sizeof(text), "%s-%d-1", actorName?actorName:"", out);
						AddPerceptionDebugLine(text, startPos, endPos, 50, 255, 50, 1.f, 3.f);
						_snprintf(text, sizeof(text), "%s-%d-2", actorName?actorName:"", out);
						AddPerceptionDebugLine(text, 
							Vec3(startPos.x, startPos.y, shape.aabb.min.z), 
							Vec3(startPos.x, startPos.y, shape.aabb.max.z), 
							50, 255, 50, 1.f, 3.f);
						_snprintf(text, sizeof(text), "%s-%d-3", actorName?actorName:"", out);
						AddPerceptionDebugLine(text, 
							Vec3(endPos.x, endPos.y, shape.aabb.min.z), 
							Vec3(endPos.x, endPos.y, shape.aabb.max.z), 
							50, 255, 50, 1.f, 3.f);
					}
#endif //CRYAISYSTEM_DEBUG

					in += 2; inPos = intersects[in];
					out += 2; outPos = intersects[out];
				}

				if (bEndInsideShape)
				{
					fMod = (rayLen - intersects[nIntersects-1]) * shape.fReductionPerMetre; 
					fMod = clamp(fMod, 0.0f, shape.fReductionMax); 
					fPerception -= fMod;

#ifdef CRYAISYSTEM_DEBUG
					if (icvDrawPerceptionDebugging != 0)
					{
						_snprintf(text, sizeof(text), "%s-%d-1", actorName?actorName:"", nIntersects);
						Vec3 startPos( start + (intersects[nIntersects-1]/rayLen) * (end - start) + Vec3(0.f,0.f,-0.1f) );
						AddPerceptionDebugLine(text, startPos, end+Vec3(0.f,0.f,-0.1f), 50, 255, 50, 1.f, 3.f);
					
						_snprintf(text, sizeof(text), "%s-%d-2", actorName?actorName:"", nIntersects);
						AddPerceptionDebugLine(text, 
							Vec3(startPos.x, startPos.y, shape.aabb.min.z), 
							Vec3(startPos.x, startPos.y, shape.aabb.max.z), 
							50, 255, 50, 1.f, 3.f);
					}
#endif //CRYAISYSTEM_DEBUG

				}
			}
		}
	}

	return fPerception;
}

IAIObjectIter* CAISystem::GetFirstAIObject(EGetFirstFilter filter, short n)
{
	if(filter == OBJFILTER_GROUP)
	{
    return SAIObjectMapIterOfType<CWeakRef>::Allocate(n, m_mapGroups.find(n), m_mapGroups.end());
	}
	else if(filter == OBJFILTER_SPECIES)
	{
    return SAIObjectMapIterOfType<CWeakRef>::Allocate(n, m_mapSpecies.find(n), m_mapSpecies.end());
	}
	else if (filter == OBJFILTER_DUMMYOBJECTS)
	{
	return SAIObjectMapIterOfType<CWeakRef>::Allocate(n, m_mapDummyObjects.find(n), m_mapDummyObjects.end());
	}
	else
	{
		if(n == 0)
      return SAIObjectMapIter<CCountedRef>::Allocate(m_Objects.begin(), m_Objects.end());
		else
      return SAIObjectMapIterOfType<CCountedRef>::Allocate(n, m_Objects.find(n), m_Objects.end());
	}
}

IAIObjectIter* CAISystem::GetFirstAIObjectInRange(EGetFirstFilter filter, short n, const Vec3& pos, float rad, bool check2D)
{
	if(filter == OBJFILTER_GROUP)
	{
    return SAIObjectMapIterOfTypeInRange<CWeakRef>::Allocate(n, m_mapGroups.find(n), m_mapGroups.end(), pos, rad, check2D);
	}
	else if(filter == OBJFILTER_SPECIES)
	{
    return SAIObjectMapIterOfTypeInRange<CWeakRef>::Allocate(n, m_mapSpecies.find(n), m_mapSpecies.end(), pos, rad, check2D);
	}
	else if (filter == OBJFILTER_DUMMYOBJECTS)
	{
    return SAIObjectMapIterOfTypeInRange<CWeakRef>::Allocate(n, m_mapDummyObjects.find(n), m_mapDummyObjects.end(), pos, rad, check2D);
	}
	else //	if(filter == OBJFILTER_TYPE)
	{
		if(n == 0)
      return SAIObjectMapIterInRange<CCountedRef>::Allocate(m_Objects.begin(), m_Objects.end(), pos, rad, check2D);
		else
      return SAIObjectMapIterOfTypeInRange<CCountedRef>::Allocate(n, m_Objects.find(n), m_Objects.end(), pos, rad, check2D);
	}
}

IAIObjectIter* CAISystem::GetFirstAIObjectInShape(EGetFirstFilter filter, short n, const char* shapeName, bool checkHeight)
{
	ShapeMap::iterator	it = m_mapGenericShapes.find(shapeName);

	// Return dummy iterator.
	if(it == m_mapGenericShapes.end())
    return SAIObjectMapIter<CCountedRef>::Allocate(m_Objects.end(), m_Objects.end());

	SShape&	shape = it->second;

	if(filter == OBJFILTER_GROUP)
	{
    return SAIObjectMapIterOfTypeInShape<CWeakRef>::Allocate(n, m_mapGroups.find(n), m_mapGroups.end(), shape, checkHeight);
	}
	else if(filter == OBJFILTER_SPECIES)
	{
    return SAIObjectMapIterOfTypeInShape<CWeakRef>::Allocate(n, m_mapSpecies.find(n), m_mapSpecies.end(), shape, checkHeight);
	}
	else if (filter == OBJFILTER_DUMMYOBJECTS)
	{
    return SAIObjectMapIterOfTypeInShape<CWeakRef>::Allocate(n, m_mapDummyObjects.find(n), m_mapDummyObjects.end(), shape, checkHeight);
	}
	else
	{
		if(n == 0)
      return SAIObjectMapIterInShape<CCountedRef>::Allocate(m_Objects.begin(), m_Objects.end(), shape, checkHeight);
		else
      return SAIObjectMapIterOfTypeInShape<CCountedRef>::Allocate(n, m_Objects.find(n), m_Objects.end(), shape, checkHeight);
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::Event( int event, const char *name)
{
	CCCPOINT(CAISystem_Event);

	switch(event)
	{
		case AISYSEVENT_DISABLEMODIFYER:
		{
			CCCPOINT(CAISystem_Event_DM);
			m_pNavigation->DisableModifier(name);
				m_pPathfinder->RescheduleCurrentPathfindRequest();
			break;
		}
		default:
			break;
	}
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::CreateFormationDescriptor(const char *name)
{
	FormationDescriptorMap::iterator itD = m_mapFormationDescriptors.find(name);
	if(itD!=m_mapFormationDescriptors.end())
		m_mapFormationDescriptors.erase(itD);
	CFormationDescriptor fdesc;
	fdesc.m_sName = name;
	m_mapFormationDescriptors.insert(FormationDescriptorMap::iterator::value_type(fdesc.m_sName, fdesc));
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::EnumerateFormationNames(unsigned int maxNames, const char** names, unsigned int* nameCount) const
{
	CRY_ASSERT(names);
	CRY_ASSERT(nameCount);

	*nameCount = 0;

	FormationDescriptorMap::const_iterator it = m_mapFormationDescriptors.begin();
	FormationDescriptorMap::const_iterator end = m_mapFormationDescriptors.end();

	for ( ; it != end; ++it)
	{
		if (*nameCount == maxNames)
		{
			gEnv->pLog->LogError("CAISystem::EnumerateFormationNames: Can't fit more formation names. Number of names = %d, maximum number of names = %d.", m_mapFormationDescriptors.size(), maxNames);
			return;
		}

		names[(*nameCount)++] = it->first.c_str();
	}
}

//-----------------------------------------------------------------------------------------------------------
bool CAISystem::IsFormationDescriptorExistent(const char *name)
{
	return m_mapFormationDescriptors.find(name)!=m_mapFormationDescriptors.end();
}
//-----------------------------------------------------------------------------------------------------------

void CAISystem::AddFormationPoint(const char *name, const FormationNode& nodeDescriptor)
{
	FormationDescriptorMap::iterator fdit = m_mapFormationDescriptors.find(name);
	if(fdit!=m_mapFormationDescriptors.end())
	{
		fdit->second.AddNode(nodeDescriptor);
	}
}

//-----------------------------------------------------------------------------------------------------------

IAIObject* CAISystem::GetLeaderAIObject(int iGroupID)
{
	CLeader* pLeader = GetLeader(iGroupID);
	return pLeader ? pLeader->GetAssociation().GetAIObject() : NULL;
}

IAIObject* CAISystem::GetLeaderAIObject(IAIObject* pObject)
{
	CAIActor* pActor = pObject->CastToCAIActor();
	CLeader* pLeader = GetLeader(pActor);
	return pLeader ? pLeader->GetAssociation().GetAIObject() : NULL;
}

IAIObject* CAISystem::GetFormationPoint(IAIObject* pObject)
{
	CAIActor* pActor = pObject->CastToCAIActor();
	CLeader* pLeader = GetLeader(pActor);
	if(pLeader) 
	{
		CCCPOINT(CAISystem_GetFormationPoint);
		return pLeader->GetFormationPoint(GetWeakRef((CAIObject*)pObject)).GetAIObject();
	}
	return NULL;
}


int CAISystem::GetFormationPointClass(const char* descriptorName, int position) 
{
	FormationDescriptorMap::iterator di;
	di = m_mapFormationDescriptors.find(descriptorName);
	if (di!=m_mapFormationDescriptors.end())
	{
		return di->second.GetNodeClass(position);
	}
	return -1;
}

//====================================================================
// Warning
//====================================================================
void CAISystem::Warning(const char * id, const char * format, ...)const
{
	if (!AIGetWarningErrorsEnabled() || !AICheckLogVerbosity(AI_LOG_WARNING))
		return;

	char outputBuffer[MAX_WARNING_LENGTH + AILogMaxIdLen ]; // extra for the id/prefix
	size_t idLen = strlen(id);
	idLen = min(idLen,AILogMaxIdLen);
	strncpy(outputBuffer, id, AILogMaxIdLen);
	va_list args;
	va_start(args, format);
	int count = vsnprintf(outputBuffer + idLen, sizeof(outputBuffer)-idLen, format, args);
	if ( count == -1 || count >=sizeof(outputBuffer) )
		outputBuffer[sizeof(outputBuffer)-1] = '\0';

	va_end(args);
	AIWarning(outputBuffer);
}

//====================================================================
// Error
//====================================================================
void CAISystem::Error(const char * id, const char * format, ...)
{
	if (!AIGetWarningErrorsEnabled() || !AICheckLogVerbosity(AI_LOG_ERROR))
		return;

	char outputBuffer[MAX_WARNING_LENGTH + AILogMaxIdLen]; // extra for the id/prefix
	size_t idLen = strlen(id);
	idLen = min(idLen,AILogMaxIdLen);
	strncpy(outputBuffer, id, AILogMaxIdLen);
	va_list args;
	va_start(args, format);
	int count = vsnprintf(outputBuffer + idLen, sizeof(outputBuffer)-idLen, format, args);
	if ( count == -1 || count >=sizeof(outputBuffer) )
		outputBuffer[sizeof(outputBuffer)-1] = '\0';
	va_end(args);
	AIError(outputBuffer);
}

//====================================================================
// LogProgress
//====================================================================
void CAISystem::LogProgress(const char * id, const char * format, ...)
{
	if (!AICheckLogVerbosity(AI_LOG_PROGRESS))
		return;

	char outputBuffer[MAX_WARNING_LENGTH + AILogMaxIdLen]; // extra for the id/prefix
	size_t idLen = strlen(id);
	idLen = min(idLen,AILogMaxIdLen);
	strncpy(outputBuffer, id, AILogMaxIdLen);
	va_list args;
	va_start(args, format);
	int count = vsnprintf(outputBuffer + idLen, sizeof(outputBuffer)-idLen, format, args);
	if ( count == -1 || count >=sizeof(outputBuffer) )
		outputBuffer[sizeof(outputBuffer)-1] = '\0';
	va_end(args);
	AILogProgress(outputBuffer);
}

//====================================================================
// LogEvent
//====================================================================
void CAISystem::LogEvent(const char * id, const char * format, ...)
{
	if (!AICheckLogVerbosity(AI_LOG_EVENT))
		return;

	char outputBuffer[MAX_WARNING_LENGTH + AILogMaxIdLen]; // extra for the id/prefix
	size_t idLen = strlen(id);
	idLen = min(idLen,AILogMaxIdLen);
	strncpy(outputBuffer, id, AILogMaxIdLen);
	va_list args;
	va_start(args, format);
	int count = vsnprintf(outputBuffer + idLen, sizeof(outputBuffer)-idLen, format, args);
	if ( count == -1 || count >=sizeof(outputBuffer) )
		outputBuffer[sizeof(outputBuffer)-1] = '\0';
	va_end(args);
	AILogEvent(outputBuffer);
}

//====================================================================
// LogComment
//====================================================================
void CAISystem::LogComment(const char * id, const char * format, ...)
{
	if (!AICheckLogVerbosity(AI_LOG_COMMENT))
		return;

	char outputBuffer[MAX_WARNING_LENGTH + AILogMaxIdLen]; // extra for the id/prefix
	size_t idLen = strlen(id);
	idLen = min(idLen,AILogMaxIdLen);
	strncpy(outputBuffer, id, AILogMaxIdLen);
	va_list args;
	va_start(args, format);
	int count = vsnprintf(outputBuffer + idLen, sizeof(outputBuffer)-idLen,format, args);
	if ( count == -1 || count >=sizeof(outputBuffer) )
		outputBuffer[sizeof(outputBuffer)-1] = '\0';
	va_end(args);
	AILogComment(outputBuffer);
}


// (MATT) Handy {2009/03/25}
//bool bNeedsToBeCreated(entityID==0 && type!=AIOBJECT_LEADER);


//====================================================================
// PopulateObjectTracker
//====================================================================
void CAISystem::PopulateObjectTracker(CObjectTracker& objectTracker)
{
	objectTracker.AddObject(m_pGraph, false);

	m_pPathfinder->PopulateObjectTracker(objectTracker);

	// take care of formations
	for (FormationMap::iterator it(m_mapActiveFormations.begin()); it!=m_mapActiveFormations.end(); ++it)
		objectTracker.AddObject(it->second, false);

	// take care of unitActions
	// (MATT) Which are objects within leader which must be noted by the tracker... Perhaps there is more like this. {2009/03/26}
	for (AIObjectOwners::iterator it(m_Objects.find(AIOBJECT_LEADER)); it!=m_Objects.end(); ++it)
	{
		if (it->first != AIOBJECT_LEADER)
			break;
		CLeader* pLeader = it->second.GetAIObject()->CastToCLeader();
		if (pLeader)
			pLeader->PopulateObjectTracker(objectTracker);
	}
}

//====================================================================
// CAIObjectCreator 
//====================================================================
struct CAIObjectCreator : public CObjectTracker::CObjectCreator
{
	CAIObjectCreator() {}
	virtual void* CreateObject(CObjectTracker::TID ID, CObjectTracker::TID externalID, const char* objectType) const;
};


//====================================================================
// CreateObject
// Objects just get created here - serialisation happens later
//====================================================================
void* CAIObjectCreator::CreateObject(CObjectTracker::TID ID, CObjectTracker::TID externalID, const char* objectType) const
{
	AILogEvent("Creating object of type %s for ID %d", objectType, ID);
	if (externalID != 0)
	{
		AIWarning("Being asked to create an object of type %s even though it has an external ID %d",
			objectType, externalID);
		return 0;
	}

		AIWarning("Unhandled request to create an object of type %s", objectType);

	return NULL;
}


//====================================================================
// InstantiateFromObjectTracker
//====================================================================
void CAISystem::InstantiateFromObjectTracker(CObjectTracker& objectTracker)
{
	AIAssert(m_pGraph);
	objectTracker.CreateObjects(CAIObjectCreator());
}

//====================================================================
// DebugOutputObjects
//====================================================================
void CAISystem::DebugOutputObjects(const char *txt) const
{
  AILogComment("================= DebugOutputObjects: %s =================", txt);

  AILogComment("m_Objects");
	bool bNoEntityWarning = (AIGetLogConsoleVerbosity()>=3 || AIGetLogFileVerbosity()>=3);

  for (AIObjectOwners::const_iterator it = m_Objects.begin() ; it != m_Objects.end() ; ++it)
  {
    short ID = it->first;
    const CAIObject *pObject = it->second.GetAIObject();
    const char *name = pObject->GetName();
    unsigned entID = pObject->GetEntityID();
    unsigned short type = pObject->GetType();
  
    AILogComment("ID %d, entID = %d, type = %d, ptr = %p, name = %s", ID, entID, type, pObject, name);

		if(bNoEntityWarning && entID>0 && !gEnv->pEntitySystem->GetEntity(pObject->GetEntityID()) 
			&& pObject->GetAIType() !=AIOBJECT_LEADER)
			AIWarning("No entity for AIObject %s",pObject->GetName());
  }

  AILogComment("================= DebugOutputObjects End =================");
}

//====================================================================
// Serialize
//====================================================================
void CAISystem::Serialize( TSerialize ser )
{
  DebugOutputObjects("Start of serialize");

  if ( m_pAIActionManager && ser.IsReading() )
	  m_pAIActionManager->Reset();

	// simplify serialising pathfinder
	m_pPathfinder->RescheduleCurrentPathfindRequest();

	CObjectTracker objectTracker;
	if (ser.IsWriting())
	{
		PopulateObjectTracker(objectTracker);
		m_pGraph->PopulateObjectTracker(objectTracker);
    DebugOutputObjects("Writing - after populating obj tracker");
	}

	if (ser.IsReading())
	{
      m_mapBeacons.clear();
    }

	// get all objects sorted and pointers-to-existing objects set up
	if (ser.IsReading())
	{
		objectTracker.Serialize(ser);
		InstantiateFromObjectTracker(objectTracker);
    DebugOutputObjects("After instantiating");
	}

	// start serialising
  ser.BeginGroup( "AISystemStatus" );
	{
		// our stuff - do we need to split this up into two stages - 1st being to walk through all objects and
		// set up pointers in the object tracker - the other to actually serialise? However, that might not
		// be possible if, for example, we have an array of pointers - then you need to serialise properly
		// to get the array size...
		ser.BeginGroup("AI_Pointers");
		{
			objectTracker.SerializeObjectPointer(ser, "m_pGraph", m_pGraph, true);
			m_pPathfinder->SerializePointers (ser, objectTracker);

      // at this point there may be more objects+dummies than when saved because some entity objects (e.g. CPipeUser
      // always contain an internal non-entity object - when they sync up the numbers should match.

			// All the non-entity objects in order
			// (MATT) Was here, now gone {2009/03/24}

			// It makes sense to clear the root objects, which should deregister all objects, before loading new ones in via objectContainer
			// Without it, it was crashing trying to deregister old objects as it read in old ones, in CCountedRef
			// That's because the standard container serialisation code clears it first, triggering deletes, even if the serialisation methods themselves don't.
			if (ser.IsReading())
			{
				m_Objects.clear();
			}
			
			// (MATT) Around here is where we should probably call the object tracker {2009/03/26}
			gAIEnv.pObjectContainer->Serialize(ser, objectTracker);
		}
		ser.EndGroup();

		ser.Value("ObjectOwners", m_Objects);
		ser.Value("MapGroups", m_mapGroups);
		ser.Value("MapSpecies", m_mapSpecies);
		ser.Value("MapDummyObjects", m_mapDummyObjects);
		ser.Value("EnabledPuppetsSet",m_enabledPuppetsSet);
		ser.Value("DisabledPuppetsSet",m_disabledPuppetsSet);


		//====================================================================
		// Object serialisation - pointers should all be set up by now
		//====================================================================
    Serialize(ser, objectTracker);

		if (ser.IsReading())
		{
			objectTracker.ValidateOwnership();
			DebugOutputObjects("After serialising");
		}
		ser.Value("m_lastAmbientFireUpdateTime", m_lastAmbientFireUpdateTime);
		ser.Value("m_lastExpensiveAccessoryUpdateTime", m_lastExpensiveAccessoryUpdateTime);
		ser.Value("m_lastVisBroadPhaseTime", m_lastVisBroadPhaseTime);
		ser.Value("m_lastGroupUpdateTime", m_lastGroupUpdateTime);
	}
	ser.EndGroup();

	// write the object tracker database at the end in case objects got added during serialisation
	if (ser.IsWriting())
		objectTracker.Serialize(ser);

	if ( m_pAIActionManager )
		m_pAIActionManager->Serialize( ser );

	if( gAIEnv.pGroupSystem )
		gAIEnv.pGroupSystem->Serialize( ser );

	if (gAIEnv.pTargetTrackManager)
		gAIEnv.pTargetTrackManager->Serialize(ser);

	CPerceptionManager::GetInstance()->Serialize(ser);

	//serialize AI lua globals
	IScriptSystem* pSS = gEnv->pScriptSystem;
	if(pSS)
	{
		SmartScriptTable pScriptAI;
		if(pSS->GetGlobalValue("AI",pScriptAI)) 
			if(pScriptAI->HaveValue("OnSave") && pScriptAI->HaveValue("OnLoad"))
			{
				ser.BeginGroup("ScriptBindAI");
				SmartScriptTable persistTable(pSS);
				if (ser.IsWriting())
					Script::CallMethod(pScriptAI, "OnSave", persistTable);
				ser.Value( "ScriptData", persistTable.GetPtr() );
			  if (ser.IsReading())
					Script::CallMethod(pScriptAI, "OnLoad", persistTable);
				ser.EndGroup();
			}
  }
}

//===================================================================
// Serialize
//===================================================================
void PathfindRequest::Serialize(TSerialize ser, CObjectTracker& objectTracker)
{
	ser.BeginGroup("PathFindRequest");
		gAIEnv.pGraph->SerializeNodePointer(ser, "pStart", startIndex);
		gAIEnv.pGraph->SerializeNodePointer(ser, "pEnd", endIndex);
  ser.Value("startPos", startPos);
  ser.Value("startDir", startDir);
  ser.Value("endPos", endPos);
  ser.Value("endDir", endDir);
		ser.Value("bSuccess", bSuccess);
		
		// (MATT) Locally convert to weak ref just here - this may or may not work in this case. {2009/02/16}
		CWeakRef<CAIActor> refRequester;
		if (ser.IsReading())
		{
			refRequester.Serialize(ser, "refRequester");
			pRequester = refRequester.GetAIObject(); // Just use the IAIPathAgent
		}
		else
		{
			refRequester = GetWeakRef( CastToCAIActorSafe( (CAIObject*)pRequester ));
			refRequester.Serialize(ser, "refRequester");
		}
		
		ser.Value("bSuccess", bSuccess);
		ser.Value("nForceTargetBuildingId", nForceTargetBuildingId);
		ser.Value("allowDangerousDestination", allowDangerousDestination);
		ser.Value("endTol", endTol);
		ser.Value("endDistance", endDistance);
		ser.Value("isDirectional", isDirectional);
		ser.Value("bPathEndIsAsRequested", bPathEndIsAsRequested);
		ser.Value("id", id);
		ser.Value("navCapMask", navCapMask);
		ser.Value("passRadius", passRadius);
		ser.EnumValue("type", type, TYPE_ACTOR, TYPE_RAW);
	ser.EndGroup();
}


//====================================================================
// Serialize
//====================================================================
void CAISystem::Serialize( TSerialize ser, CObjectTracker& objectTracker )
{
	// WIP Notes:
	// Leaders seem special. Can't serialise them like the other m_Objects.
	// m_mapAIGroups might well be interfering with other group list
	

  ser.BeginGroup("CAISystem");
  {

	m_pPathfinder->Serialize (ser, objectTracker);

  ser.BeginGroup("Beacons");
  {
    int count = m_mapBeacons.size();
    ser.Value("count", count);
    char name[32];
    if (ser.IsWriting())
    {
      int i = 0;
      for (BeaconMap::iterator it = m_mapBeacons.begin() ; it != m_mapBeacons.end() ; ++it, ++i)
      {
        sprintf(name, "beacon-%d", i);
        ser.BeginGroup(name);
        {
          unsigned short id = it->first;
          BeaconStruct &bs = it->second;

          ser.Value("id", id);
					bs.refBeacon.Serialize(ser, "refBeacon");
					bs.refBeacon.Serialize(ser, "refOwner");
        }
        ser.EndGroup();
      }
    }
    else
    {
      m_mapBeacons.clear();
      for (int i = 0 ; i < count ; ++i)
      {
          sprintf(name, "beacon-%d", i);
        ser.BeginGroup(name);
        {
          unsigned short id;
          BeaconStruct bs;
          ser.Value("id", id);
					bs.refBeacon.Serialize(ser, "refBeacon");
					bs.refBeacon.Serialize(ser, "refOwner");
          m_mapBeacons[id] = bs;
        }
        ser.EndGroup();
      }
    }
  }
  ser.EndGroup();

		m_lightManager.Serialize(ser, objectTracker);

    // Active formations
    ser.BeginGroup("ActiveFormations");
    {
      int count = m_mapActiveFormations.size();
      ser.Value("numObjects", count);

      if(ser.IsReading())
      {
        FormationMap::iterator iFormation=m_mapActiveFormations.begin(), iEnd = m_mapActiveFormations.end();
        for( ;iFormation != iEnd; ++iFormation)
          delete iFormation->second;
        m_mapActiveFormations.clear();
      }

      char formationName[32];
      while(--count>=0)
      {
        CFormation* pFormationObject(0);
        CWeakRef<CAIObject> refOwner;

        if(ser.IsWriting())
        {
          FormationMap::iterator iFormation=m_mapActiveFormations.begin();
          std::advance(iFormation, count);
          refOwner = iFormation->first;
          pFormationObject = iFormation->second;
        }
        else
          pFormationObject = new CFormation();
        sprintf(formationName, "Formation_%d", count);
        ser.BeginGroup(formationName);
        {
          {
            refOwner.Serialize(ser,"formationOwner");
            pFormationObject->Serialize(ser, objectTracker);
          }
          objectTracker.SerializeObjectPointer(ser, "formation", pFormationObject, true);
        }
        ser.EndGroup();
        if(ser.IsReading())
          m_mapActiveFormations.insert(FormationMap::iterator::value_type(refOwner,pFormationObject));
      }
    }
    ser.EndGroup();


	m_PipeManager.Serialize(ser);

    // Serialize temporary shapes.
    ser.BeginGroup("TempGenericShapes");
    if(ser.IsWriting())
    {
      int	nTempShapes = 0;
      for(ShapeMap::iterator it = m_mapGenericShapes.begin(); it != m_mapGenericShapes.end(); ++it)
        if(it->second.temporary)
          nTempShapes++;
      ser.Value("nTempShapes", nTempShapes);
      for(ShapeMap::iterator it = m_mapGenericShapes.begin(); it != m_mapGenericShapes.end(); ++it)
      {
        SShape&	shape = it->second;
        if(shape.temporary)
        {
          ser.BeginGroup("Shape");
          string	name(it->first);
          ser.Value("name", name);
          ser.Value("aabbMin", shape.aabb.min);
          ser.Value("aabbMax", shape.aabb.max);
          ser.Value("type", shape.type);
          ser.Value("shape", shape.shape);
          ser.EndGroup();
        }
      }
    }
    else
    {
      // Remove temporary shapes.
      for(ShapeMap::iterator it = m_mapGenericShapes.begin(); it != m_mapGenericShapes.end();)
      {
        // Enable
        it->second.enabled = true;
        // Remove temp
        if(it->second.temporary)
          m_mapGenericShapes.erase(it++);
        else
          ++it;
      }
      // Create new temp shapes.
      int	nTempShapes = 0;
      ser.Value("nTempShapes", nTempShapes);
      for(int i = 0; i < nTempShapes; ++i)
      {
        string	name;
        SShape	shape;
        ser.BeginGroup("Shape");
        ser.Value("name", name);
        ser.Value("aabbMin", shape.aabb.min);
        ser.Value("aabbMax", shape.aabb.max);
        ser.Value("type", shape.type);
        ser.Value("shape", shape.shape);
        ser.EndGroup();
        shape.temporary = true;
        m_mapGenericShapes.insert(ShapeMap::iterator::value_type(name, shape));
      }
    }
    ser.EndGroup();

    ser.BeginGroup("DisabledGenericShapes");
    if(ser.IsWriting())
    {
      int	nDisabledShapes = 0;
      for(ShapeMap::iterator it = m_mapGenericShapes.begin(); it != m_mapGenericShapes.end(); ++it)
        if(!it->second.enabled)
          nDisabledShapes++;
      ser.Value("nDisabledShapes", nDisabledShapes);

      for(ShapeMap::iterator it = m_mapGenericShapes.begin(); it != m_mapGenericShapes.end(); ++it)
      {
        if(it->second.enabled) continue;
				ser.BeginGroup("Shape");
        ser.Value("name", it->first);
				ser.EndGroup();
      }
    }
    else
    {
      int	nDisabledShapes = 0;
      ser.Value("nDisabledShapes", nDisabledShapes);
      string	name;
      for(int i = 0; i < nDisabledShapes; ++i)
      {
				ser.BeginGroup("Shape");
        ser.Value("name", name);
				ser.EndGroup();
        ShapeMap::iterator di = m_mapGenericShapes.find(name);
        if(di != m_mapGenericShapes.end())
          di->second.enabled = false;
        else
          AIWarning("CAISystem::Serialize Unable to find generic shape called %s", name.c_str());
      }
    }
    ser.EndGroup();

    ser.BeginGroup("AI_Objects");
    {
			/*M
      // First all non-entity objects - these will have been loaded/saved in the same order
      char AIObjectSaveName[256];
      for (unsigned i = 0 ; i < nonEntityObjects.size() ; ++i)
      {
        sprintf(AIObjectSaveName, "Non-entity_AIObject-%d", i);
        ser.BeginGroup(AIObjectSaveName);
        {
          IAIObject* pObject = nonEntityObjects[i];
          AIAssert(pObject->GetEntityID() == 0);
          pObject->Serialize(ser, objectTracker);
        }
        ser.EndGroup();
      }*/

			/*M

      // Now all entity objects - these will find themselves based on their entity ID
			int i=0;//DEBUG
      for (AIObjects::iterator it = m_Objects.begin() ; it != m_Objects.end() ; ++it)
      {
        CAIObject* pObject = it->second;
        if (pObject->GetEntityID() != 0 && it->first!=AIOBJECT_LEADER)
        {
          sprintf(AIObjectSaveName, "Entity_AIObject-%d", pObject->GetEntityID());
					AILogEvent("Serializing AIObject(%d) %s",i++,pObject->GetName());//DEBUG
          if( ser.BeginOptionalGroup(AIObjectSaveName, true))
          {
            pObject->Serialize(ser, objectTracker);
            ser.EndGroup();
          }
        }
      }*/

			/*M
      //serialize AILeaders after other Entity AIObjects since they contain reference to other AI Objects
      ser.BeginGroup("AILeaders");
      {
				int numLeaders(0);
				if(ser.IsReading())
				{
					// remove all the leaders first;
					m_Objects.erase(AIOBJECT_LEADER);;
					ser.Value("numLeaders", numLeaders);
					for(int j=0; j<numLeaders; j++)
					{
						sprintf(AIObjectSaveName, "AILeaderN-%d", j);
						ser.BeginGroup(AIObjectSaveName);
						{
							int groupId;
							ser.Value("groupId", groupId);
							CLeader* pLeader = CreateLeader(groupId);
							if(!pLeader)
							{
								AIWarning("CAISystem::Serialization : can't create AILeader %s for group %d", AIObjectSaveName, groupId);
								ser.EndGroup();
								continue;
							}
							pLeader->Serialize(ser, objectTracker);
						}
						ser.EndGroup();
					}
				}
				else
				{
					for (AIObjects::iterator it = m_Objects.find(AIOBJECT_LEADER), itEnd = m_Objects.end();  
						it != itEnd && it->first == AIOBJECT_LEADER; ++it)
						++numLeaders;
					ser.Value("numLeaders", numLeaders);
					int counter(0);
					for (AIObjects::iterator it = m_Objects.find(AIOBJECT_LEADER), itEnd = m_Objects.end();  
						it != itEnd && it->first == AIOBJECT_LEADER; ++it)
					{
						sprintf(AIObjectSaveName, "AILeaderN-%d", counter++);
						ser.BeginGroup(AIObjectSaveName);
						{
							CLeader* pLeader = it->second->CastToCLeader();
							int groupId(pLeader->GetGroupId());
							ser.Value("groupId", groupId);
							pLeader->Serialize(ser, objectTracker);
						}
						ser.EndGroup();
					}
				}
      }
      ser.EndGroup();
			*/
    }
    ser.EndGroup();

		if (m_pGraph)
			m_pGraph->Serialize(ser, objectTracker, "AINavGraph");

		m_pNavigation->Serialize(ser,objectTracker);

    ser.Value("m_deadBodies", m_deadBodies);
    ser.Value("m_priorityObjectTypes", m_priorityObjectTypes);

    ser.Value("m_fFrameStartTime", m_fFrameStartTime);
    ser.Value("m_fFrameDeltaTime", m_fFrameDeltaTime);
    ser.Value("m_fLastPuppetUpdateTime", m_fLastPuppetUpdateTime);
    if (ser.IsReading())
    {
      // Danny: physics doesn't serialise its time (it doesn't really use it) so we can
      // set it here.
      GetISystem()->GetIPhysicalWorld()->SetPhysicsTime(m_fFrameStartTime.GetSeconds());
    }

    float fst = m_fFrameStartTime.GetSeconds();

    AIObjectOwners::iterator itobjend = m_Objects.end();


    ser.BeginGroup("AI_Groups");
    {
      //regenerate the whole groups map when reading
      if(ser.IsReading())
      {
        for(AIGroupMap::iterator it = m_mapAIGroups.begin(); !m_mapAIGroups.empty(); )
        {
          delete it->second;
          m_mapAIGroups.erase(it++);
        }

        for (AIObjectOwners::iterator itobj = m_Objects.begin() ; itobj != itobjend ; ++itobj)
        {
          CAIObject* pObject = itobj->second.GetAIObject();
          if(pObject->CastToCPuppet() || pObject->CastToCAIPlayer() || pObject->CastToCLeader())
          {
						CAIActor* pActor = pObject->CastToCAIActor();
            int groupid = pActor->GetGroupId();
            if(m_mapAIGroups.find(groupid)==m_mapAIGroups.end())
            {
              CAIGroup* pGroup = new CAIGroup(groupid);
              m_mapAIGroups.insert(std::make_pair(groupid,pGroup));

            }
          }
        }
      }
      for(AIGroupMap::iterator it = m_mapAIGroups.begin(); it!=m_mapAIGroups.end(); ++it)
      {
        CAIGroup* pGroup = it->second;
        pGroup->Serialize(ser, objectTracker);
        if(ser.IsReading())
        {

          // assign group to leader and viceversa
          int groupid = it->first;
          CLeader* pLeader = NULL;
          bool found = false;

          AIObjectOwners::const_iterator itl = m_Objects.find(AIOBJECT_LEADER);

          while(!found && itl!=itobjend && itl->first == AIOBJECT_LEADER)
          {
            pLeader = (CLeader*)itl->second.GetAIObject();
            found = (pLeader->GetGroupId() == groupid);
            ++itl;
          }

          if(found && pLeader)
            pGroup->SetLeader(pLeader);
          }
        }
      }
    ser.EndGroup();


    m_pSmartObjectManager->Serialize( ser );

    char name[32];
    ser.BeginGroup("AI_Alertness");
    ser.Value("m_CurrentGlobalAlertness",m_CurrentGlobalAlertness);
    for(int i=0;i<4;i++)
    {
      sprintf(name,"AlertnessCounter%d",i);
      ser.Value(name,m_AlertnessCounters[i]);
    }
    ser.EndGroup();

    ser.Value("m_nTickCount",m_nTickCount);
    ser.Value("m_bUpdateSmartObjects",m_bUpdateSmartObjects);
    objectTracker.SerializeValueContainer(ser,"m_deadBodies",m_deadBodies);
    objectTracker.SerializeValueObjectContainer(ser,"m_mapAuxSignalsFired",m_mapAuxSignalsFired);

    m_isPointOnForbiddenEdgeQueue.Clear();
    m_isPointInForbiddenRegionQueue.Clear();
  }
  ser.EndGroup();
}

// notifies that entity has changed its position, which is important for smart objects
void CAISystem::NotifyAIObjectMoved( IEntity* pEntity, SEntityEvent event )
{
	if(!IsEnabled())
		return;
	AIAssert( m_pSmartObjectManager );
	((IEntitySystemSink*)m_pSmartObjectManager)->OnEvent( pEntity, event );
	}

//====================================================================
// RegisterFirecommandHandler
//====================================================================
void CAISystem::RegisterFirecommandHandler(IFireCommandDesc* desc)
{
	for(std::vector<IFireCommandDesc*>::iterator it = m_firecommandDescriptors.begin(); it != m_firecommandDescriptors.end(); ++it)
	{
		if((*it) == desc)
		{
			desc->Release();
			return;
		}
	}
	m_firecommandDescriptors.push_back(desc);
}

//====================================================================
// CreateFirecommandHandler
//====================================================================
IFireCommandHandler* CAISystem::CreateFirecommandHandler(const char* name, IAIActor *pShooter)
{
	for(std::vector<IFireCommandDesc*>::iterator it = m_firecommandDescriptors.begin(); it != m_firecommandDescriptors.end(); ++it)
	{
		if(stricmp((*it)->GetName(), name) == 0)
			return (*it)->Create(pShooter);
	}
	AIWarning("CAISystem::CreateFirecommandHandler: Could not find firecommand handler '%s'", name);
	return 0;
}

//====================================================================
// CombatClasses - add new
//====================================================================
void CAISystem::AddCombatClass(int combatClass, float* pScalesVector, int size, const char* szCustomSignal)
{
	if(size==0)
	{
		m_CombatClasses.clear();
		return;
	}

	// Sanity check
	AIAssert(combatClass >= 0 && combatClass < 100);

	if (combatClass >= (int)m_CombatClasses.size())
		m_CombatClasses.resize(combatClass+1);

	SCombatClassDesc& desc = m_CombatClasses[combatClass];
	if (szCustomSignal && strlen(szCustomSignal) > 1)
		desc.customSignal = szCustomSignal;
	else
		desc.customSignal.clear();

	desc.mods.resize(size);
	for (int i = 0; i < size; ++i)
		desc.mods[i] = pScalesVector[i];
}

//====================================================================
// AIHandler calls this function to replace the OnEnemySeen signal
//====================================================================
const char* CAISystem::GetCustomOnSeenSignal(int combatClass)
{
	if (combatClass < 0 || combatClass >= (int)m_CombatClasses.size())
		return 0;
	if (m_CombatClasses[combatClass].customSignal.empty())
		return 0;
	return m_CombatClasses[combatClass].customSignal.c_str();
}

//====================================================================
// GetCombatClassScale - retrieves scale for given target/shooter
//====================================================================
float	CAISystem::GetCombatClassScale(int shooterClass, int targetClass)
{
	if (targetClass < 0 || shooterClass < 0 || shooterClass >= (int)m_CombatClasses.size())
		return 1.0f;
	SCombatClassDesc& desc = m_CombatClasses[shooterClass];
	if (targetClass >= (int)desc.mods.size())
		return 1.0f;
	return desc.mods[targetClass];
}

//===================================================================
// GetDangerSpots
//===================================================================
unsigned int CAISystem::GetDangerSpots(const IAIObject* requester, float range, Vec3* positions, unsigned int* types,
																			 unsigned int n, unsigned int flags)
{
	float	rangeSq = sqr(range);

	const Vec3&	reqPos = requester->GetPos();

	const CAIActor* pActor = requester->CastToCAIActor();
	unsigned int species = pActor->GetParameters().m_nSpecies;
	unsigned int i = 0;

	if(flags & DANGER_EXPLOSIVE)
	{
		// Collect all grenades and merge close ones.
		std::list<Vec3>	grenades;
		AIObjectOwners::const_iterator aio = m_Objects.find(AIOBJECT_GRENADE);
		for(; aio != m_Objects.end(); ++aio)
		{
			// TODO: add other explosives here too (maybe check if the requester knows about them also too)
			if (aio->first != AIOBJECT_GRENADE)
				break;

			// Skip explosives which are too far away.
			const Vec3&	pos = aio->second.GetAIObject()->GetPos();
			if(Distance::Point_PointSq(pos, reqPos) > rangeSq)
				continue;

			// Merge with current greanades if possible.
			bool	merged = false;
			for(std::list<Vec3>::iterator it = grenades.begin(); it != grenades.end(); ++it)
			{
				if(Distance::Point_Point((*it), pos) < 3.0f)
				{
					merged = true;
					break;
				}
			}

			// If cannot be merged, add new.
			if(!merged)
				grenades.push_back(pos);
		}

		for(std::list<Vec3>::iterator it = grenades.begin(); it != grenades.end(); ++it)
		{
			// Limit the number of points to output.
			if(i >= n) break;
			// Output point
			positions[i] = (*it);
			if(types) types[i] = DANGER_EXPLOSIVE;
			i++;
		}
	}

	if(flags & DANGER_EXPLOSION_SPOT)
	{
		// The explosions positions are merged already when they are added to the list.
/*		for (unsigned j = 0, nj = m_explosionSpots.size(); j < nj; ++j)
		{
			// Limit the number of points to output.
			if(i >= n) break;

			// Skip far away explosions.
			if(Distance::Point_PointSq(m_explosionSpots[j].pos, reqPos) > rangeSq)
				continue;
			// Output the point.
			positions[i] = m_explosionSpots[j].pos;
			if(types) types[i] = DANGER_EXPLOSION_SPOT;
			i++;
		}*/
	}

	if(flags & DANGER_DEADBODY)
	{
		// The dead body positions are merged already when they are added to the list.
		for (unsigned j = 0, nj = m_deadBodies.size(); j < nj; ++j)
		{
			// Limit the number of points to output.
			if(i >= n) break;

			// Skip dead bodies of different species.
			if (m_deadBodies[j].species != species) continue;

			// Skip far away bodies.
			const Vec3& p = m_deadBodies[j].pos;
			if(Distance::Point_PointSq(p, reqPos) > rangeSq)
				continue;
			// Output the point.
			positions[i] = p;
			if(types) types[i] = DANGER_DEADBODY;
			i++;
		}
	}

	// Returns number of points.
	return i;
}

//===================================================================
// RegisterDeadBody
//===================================================================
void CAISystem::RegisterDeadBody(unsigned int species, const Vec3& pos)
{
	// Check if the position is close to existing body
//	for(TDeadBodyList::iterator it = m_lstDeadBodies.begin(); it != m_lstDeadBodies.end(); ++it)
	for (unsigned i = 0, ni = m_deadBodies.size(); i < ni; ++i)
	{
		unsigned int s = m_deadBodies[i].species;
		Vec3& p = m_deadBodies[i].pos;
		if(s == species && Distance::Point_PointSq(p, pos) < sqr(2.0f))
		{
			// Merge the points
			p = (p + pos) * 0.5f;
			return;
		}
	}
	// Add the new body to the list.
	// Limit the number of bodies in the list.
	if (m_deadBodies.size() >= 20)
	{
		unsigned oldest = 0;
		float oldestTime = FLT_MAX;
		for (unsigned i = 0, ni = m_deadBodies.size(); i < ni; ++i)
		{
			if (m_deadBodies[i].t < oldestTime)
			{
				oldest = i;
				oldestTime = m_deadBodies[i].t;
			}
		}
		m_deadBodies[oldest].Set(species, pos);
	}
	else
	{
		m_deadBodies.push_back(SAIDeadBody(species, pos));
	}
}

//===================================================================
// DynOmniLightEvent
//===================================================================
void CAISystem::DynOmniLightEvent(const Vec3& pos, float radius, EAILightEventType type, EntityId shooterId, float time)
	{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	// Do not handle events while serializing.
	if (gEnv->pSystem->IsSerializingFile())
		return;
	if(!IsEnabled())
		return;

	IEntity* pShooterEnt = gEnv->pEntitySystem->GetEntity(shooterId);
	if (!pShooterEnt) return;
	CAIActor* pActor = CastToCAIActorSafe(pShooterEnt->GetAI());
	if (pActor)
		m_lightManager.DynOmniLightEvent(pos, radius, type, pActor, time);
	}

//===================================================================
// DynSpotLightEvent
//===================================================================
void CAISystem::DynSpotLightEvent(const Vec3& pos, const Vec3& dir, float radius, float fov, EAILightEventType type, EntityId shooterId, float time)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	// Do not handle events while serializing.
	if (gEnv->pSystem->IsSerializingFile())
		return;
	if(!IsEnabled())
		return;

	IEntity* pShooterEnt = gEnv->pEntitySystem->GetEntity(shooterId);
	if (!pShooterEnt) return;
	CAIActor* pActor = CastToCAIActorSafe(pShooterEnt->GetAI());
	if (pActor)
		m_lightManager.DynSpotLightEvent(pos, dir, radius, fov, type, pActor, time);
}

//===================================================================
// RegisterAIEventListener
//===================================================================
void CAISystem::RegisterAIEventListener(IAIEventListener* pListener, const Vec3& pos, float rad, int flags)
{
	CPerceptionManager::GetInstance()->RegisterAIEventListener(pListener, pos, rad, flags);
}

//===================================================================
// UnregisterAIEventListener
//===================================================================
void CAISystem::UnregisterAIEventListener(IAIEventListener* pListener)
{
	CPerceptionManager::GetInstance()->UnregisterAIEventListener(pListener);
}


//===================================================================
// RegisterStimulus
//===================================================================
void CAISystem::RegisterStimulus(const SAIStimulus& stim)
{
	// Do not handle events while serializing.
	if (gEnv->pSystem->IsSerializingFile())
		return;
	if (!IsEnabled())
		return;

	CPerceptionManager::GetInstance()->RegisterStimulus(stim);
}

//===================================================================
// IgnoreStimulusFrom
//===================================================================
void CAISystem::IgnoreStimulusFrom(EntityId sourceId, EAIStimulusType type, float time)
{
	// Do not handle events while serializing.
	if (gEnv->pSystem->IsSerializingFile())
		return;
	if (!IsEnabled())
		return;

	CPerceptionManager::GetInstance()->IgnoreStimulusFrom(sourceId, type, time);
}


//===================================================================
// CheckPointsInsidePathObstacles
//===================================================================
unsigned int CAISystem::CheckPointsInsidePathObstacles(IAIObject* requester, const Vec3* points, bool* results, unsigned int n)
{
	CPuppet* puppet = CastToCPuppetSafe(requester);
	if(!puppet)
		return 0;

	const CPathObstacles& obstacles = puppet->GetPathAdjustmentObstacles();

	unsigned int ninside = 0;
	for(unsigned int i = 0; i < n; i++)
	{
		results[i] = obstacles.IsPointInsideObstacles(points[i]);
		if(results[i])
			ninside++;
	}
	return ninside;
}

//===================================================================
// RegisterDamageRegion
//===================================================================
void CAISystem::RegisterDamageRegion(const void *pID, const Sphere &sphere)
{
  if (sphere.radius > 0.0f)
    m_damageRegions[pID] = sphere;
  else
    m_damageRegions.erase(pID);
}

//===================================================================
// GetObstaclePositionsInRadius
//===================================================================
void CAISystem::GetObstaclePositionsInRadius(const Vec3& pos, float radius, float minHeight, int flags,
																						 std::vector<std::pair<Vec3, float> >& positions)
{
	bool covers = (flags & OBSTACLES_COVER) != 0;
	bool softCovers = (flags & OBSTACLES_SOFT_COVER) != 0;

	std::vector<std::pair<float, unsigned> > verts;
	m_VertexList.GetVerticesInRange(verts, pos, radius, OBSTACLE_HIDEABLE);

	for (unsigned i = 0, ni = verts.size(); i < ni; ++i)
  {
		const ObstacleData& od =  m_VertexList.GetVertex(verts[i].second);
		if ((softCovers && !od.IsCollidable()) || (covers && od.IsCollidable()))
				if (od.GetApproxHeight() > minHeight && od.fApproxRadius > 0.0f)
					positions.push_back(std::make_pair(od.vPos, od.fApproxRadius));
	}  
  }

//===================================================================
// GetHidespotsInRange
//===================================================================
MultimapRangeHideSpots &CAISystem::GetHideSpotsInRange(MultimapRangeHideSpots &hidespots, 
                                                       MapConstNodesDistance &traversedNodes,
                                                       const Vec3 & startPos, float maxDist, 
                                                       IAISystem::tNavCapMask navCapMask, float passRadius, bool skipNavigationTest,
                                                       IEntity* pSmartObjectUserEntity,
																											 unsigned lastNavNodeIndex,
                                                       const class CAIObject *pRequester)
{
	const GraphNode *pLastNavNode = m_pGraph->GetNodeManager().GetNode(lastNavNodeIndex);

  FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );
  hidespots.clear();

  float maxDistSq = square(maxDist);

  if (!skipNavigationTest)
  {
		m_pGraph->GetNodesInRange(traversedNodes, startPos, maxDist, navCapMask, passRadius, lastNavNodeIndex, pRequester);
    if (traversedNodes.empty())
      return hidespots;
  }

  // waypoint
  if (navCapMask & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE))
  {
    if (skipNavigationTest)
    {
			// (MATT) Commented out the warning for milestone.
			// Ideally, we would not skip the test anyway, and gain performance by avoiding the hash spacelookup of graph hidespots {2009/12/11}
			//AIWarning("Not implemented skipNavigationTest for waypoint");
    }
    else
    {
      const MapConstNodesDistance::const_iterator itEnd = traversedNodes.end();
      for (MapConstNodesDistance::const_iterator it = traversedNodes.begin() ; it != itEnd ; ++it)
      {
        const GraphNode *pNode = it->first;
        if (pNode->navType & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE))
        {
          float distance = it->second;
          const SWaypointNavData *pData = pNode->GetWaypointNavData();
          if (pData->type != WNT_HIDE)
            continue;
          MultimapRangeHideSpots::iterator resultIt = hidespots.insert(MultimapRangeHideSpots::value_type(distance, 
            SHideSpot(SHideSpotInfo::eHST_WAYPOINT, pNode->GetPos(), pData->dir)));
          resultIt->second.pNavNode = pNode;
        }
      }
    }
  }  

  // triangular
  if (navCapMask & IAISystem::NAV_TRIANGULAR)
  {
		std::vector<std::pair<float, unsigned> > verts;
		m_VertexList.GetVerticesInRange(verts, startPos, maxDist + 1, OBSTACLE_HIDEABLE);

		for (unsigned i = 0, ni = verts.size(); i < ni; ++i)
    {
			const ObstacleData& od = m_VertexList.GetVertex(verts[i].second);

			float distanceSq = Distance::Point_Point2DSq(od.vPos, startPos);
			if (distanceSq > maxDistSq)
            continue;

          const std::vector<const GraphNode *> &navNodes = od.GetNavNodes();
          const std::vector<const GraphNode *>::const_iterator nodeEnd = navNodes.end();
          for (std::vector<const GraphNode *>::const_iterator nodeIt = navNodes.begin() ; nodeIt != nodeEnd ; ++nodeIt)
          {
            const GraphNode *pNode = *nodeIt;

				float distance;
				MapConstNodesDistance::const_iterator it;
				if (!skipNavigationTest)
				{
					it = traversedNodes.find(pNode);
					if (it == traversedNodes.end())
              continue;
					distance = it->second;
				}
				else
				{
					distance = sqrtf(distanceSq);
				}

            MultimapRangeHideSpots::iterator resultIt = hidespots.insert(MultimapRangeHideSpots::value_type(distance,
              SHideSpot(SHideSpotInfo::eHST_TRIANGULAR, od.vPos, od.vDir)));
            resultIt->second.pNavNodes = &od.GetNavNodes();
            resultIt->second.pObstacle = &od;
            break;
          }
        }
      }

  // hide anchors
  {
    int anchorTypes[2] = {AIANCHOR_COMBAT_HIDESPOT, AIANCHOR_COMBAT_HIDESPOT_SECONDARY};
    for (int at = 0 ; at < 2 ; ++at)
    {
      const AIObjectOwners::const_iterator oiEnd = m_Objects.end();
	    for (AIObjectOwners::const_iterator oi = m_Objects.find(anchorTypes[at]) ; oi != oiEnd ; ++oi)
      {
		    CAIObject* object = oi->second.GetAIObject();
		    if (object->GetType() != anchorTypes[at])
			    break;
		    if (!object->IsEnabled())
			    continue;
		    float distanceSq = Distance::Point_PointSq(object->GetPos(), startPos);
		    if (distanceSq > maxDistSq)
          continue;
        const GraphNode *pNode = object->GetAnchorNavNode();
        if (!pNode)
          continue;

				float distance;
        MapConstNodesDistance::const_iterator it;
        if (!skipNavigationTest)
				{
          it = traversedNodes.find(pNode);
					if (it == traversedNodes.end())
						continue;
					distance = it->second;
				}
				else
        {
					distance = sqrtf(distanceSq);
				}

          MultimapRangeHideSpots::iterator resultIt = hidespots.insert(MultimapRangeHideSpots::value_type(distance,
            SHideSpot(SHideSpotInfo::eHST_ANCHOR, object->GetPos(), object->GetMoveDir())));
          resultIt->second.pNavNode = pNode;
          resultIt->second.pAnchorObject = object;
        }
	    }
    }

	// Dynamic hidespots.
	if (gAIEnv.CVars.DynamicHidespotsEnabled != 0)
	{
		typedef std::vector<SDynamicObjectHideSpot> TDynamicHideSpots;
		static TDynamicHideSpots dynamicHideSpots;
		m_dynHideObjectManager.GetHidePositionsWithinRange(dynamicHideSpots, startPos, maxDist, navCapMask, passRadius, lastNavNodeIndex);

		const TDynamicHideSpots::const_iterator diEnd = dynamicHideSpots.end();
		for (TDynamicHideSpots::const_iterator di = dynamicHideSpots.begin() ; di != diEnd ; ++di)
		{
			const SDynamicObjectHideSpot& hs = *di;
			GraphNode *pNode = m_pGraph->GetNodeManager().GetNode(hs.nodeIndex);
			if (!pNode)
				continue;
			float distance;
			MapConstNodesDistance::const_iterator it;
			if (!skipNavigationTest)
			{
				it = traversedNodes.find(pNode);
				if (it == traversedNodes.end())
					continue;
				distance = it->second;
			}
			else
			{
				distance = Distance::Point_Point(hs.pos, startPos);
			}

			MultimapRangeHideSpots::iterator resultIt = hidespots.insert(MultimapRangeHideSpots::value_type(distance,
				SHideSpot(SHideSpotInfo::eHST_DYNAMIC, hs.pos, hs.dir)));
			resultIt->second.pNavNode = pNode;
			resultIt->second.entityId = hs.entityId;
		}
	}

  // smart objects
  if (pSmartObjectUserEntity)
  {
	  QueryEventMap query;
    IEntity* pObjectEntity = NULL;
	  m_pSmartObjectManager->TriggerEvent( "Hide", pSmartObjectUserEntity, pObjectEntity, &query );

  	const QueryEventMap::const_iterator itEnd = query.end();
    for (QueryEventMap::const_iterator it = query.begin() ; it != itEnd ; ++it)
    {
  		const CQueryEvent* pQueryEvent = &it->second;

      Vec3 pos;
		  if (pQueryEvent->pRule->pObjectHelper)
			  pos = pQueryEvent->pObject->GetHelperPos(pQueryEvent->pRule->pObjectHelper);
		  else
			  pos = pQueryEvent->pObject->GetPos();
      Vec3 dir = pQueryEvent->pObject->GetOrientation(pQueryEvent->pRule->pObjectHelper);

			unsigned nodeIndices[2];
			unsigned nNavNodes = pQueryEvent->pObject->GetNavNodes(nodeIndices, pQueryEvent->pRule->pObjectHelper);
      for (unsigned iNavNode = 0 ; iNavNode < nNavNodes ; ++iNavNode)
      {
				const GraphNode *pNavNode = m_pGraph->GetNodeManager().GetNode(nodeIndices[iNavNode]);
        AIAssert(pNavNode);
        MapConstNodesDistance::const_iterator dit(traversedNodes.end());
        if (!skipNavigationTest )
          dit = traversedNodes.find(pNavNode);
        if (skipNavigationTest || dit != traversedNodes.end())
        {
          float distance = skipNavigationTest ? Distance::Point_Point(pos, startPos) : dit->second;
          MultimapRangeHideSpots::iterator resultIt = hidespots.insert(MultimapRangeHideSpots::value_type(distance,
            SHideSpot(SHideSpotInfo::eHST_SMARTOBJECT, pos, dir)));
          resultIt->second.pNavNode = pNavNode;
          resultIt->second.SOQueryEvent = *pQueryEvent;
        }
      }
    }
  }

  // Volume hidespots
  if (navCapMask & IAISystem::NAV_VOLUME)
  {
    // well.. it could be done but it's not used?
/*
    static std::vector<SVolumeHideSpot> hidePositions;
    hidePositions.resize(0);
    m_pVolumeNavRegion->GetHideSpotsWithinRadius(startPos, maxDist, SVolumeHideFunctor(hidePositions));
*/
  }

  return hidespots;
}

//===================================================================
// AdjustDirectionalCoverPosition
//===================================================================
// (MATT) Raycasts. No new code uses this, just seekcover and hide {2009/07/10}
void CAISystem::AdjustDirectionalCoverPosition(Vec3& pos, const Vec3& dir, float agentRadius, float testHeight)
{
	Vec3	floorPos(pos);
	// Add fudge to the initial position in case the point is very near to ground.
	GetFloorPos(floorPos, pos + Vec3(0.0f, 0.0f, max(0.5f, testHeight * 0.5f)), walkabilityFloorUpDist,
		walkabilityFloorDownDist, walkabilityDownRadius, AICE_ALL);
	floorPos.z += testHeight;
	Vec3 hitPos;
	const float	distToWall = AGENT_COVER_CLEARANCE + agentRadius;
	float	hitDist = distToWall;
	if(IntersectSweptSphere(&hitPos, hitDist, Lineseg(floorPos, floorPos + dir * distToWall), 0.15f, AICE_ALL))
		pos = floorPos + dir * (hitDist - distToWall);
	else
		pos = floorPos;
}

//===================================================================
// AdjustOmniDirectionalCoverPosition
// if hideBehind false - move point to left/right side of cover, to locate spot by cover, but not hiden
//===================================================================
void CAISystem::AdjustOmniDirectionalCoverPosition(Vec3& pos, Vec3& dir, float hideRadius, float agentRadius, const Vec3& hideFrom, const bool hideBehind)
{
	dir = hideFrom - pos;
	dir.z = 0;
	dir.NormalizeSafe();

	if(!hideBehind)
	{
		// dir is left/right offset when not hiding behind cover
		float tmp(dir.x);
		dir.x = -dir.y;
		dir.y = tmp;
		if(ai_rand()%100 < 50)
			dir = -dir;
	}
	else
	{

		if (gAIEnv.configuration.eCompatibilityMode == ECCM_GAME04)
		{
			// MERGE : (MATT) The maths wasn't quite right here, and this might not suit Crysis anyway {2008/01/04:12:59:04}
			// If hide object has a radius significantly bigger than the AI's, get a position near the edge so we can lean out
			if (hideRadius > agentRadius * 1.1f)
			{
				// Move the position off-centre
				Vec3 adjustDir = dir.Cross( Vec3(0,0,1) );
				if(ai_rand() & 1)
					adjustDir = -adjustDir;
				float fAdjustDist = hideRadius - agentRadius;
				adjustDir.SetLength( fAdjustDist );
				pos += adjustDir;

				// Adjust length of dir, since we're now less than a radius from the centre of the obstacle
				dir *= cosf( gf_PI * 0.5f * fAdjustDist / hideRadius );
			}
		}

	}
	
	pos -= dir * (max(hideRadius, 0.0f) + agentRadius + AGENT_COVER_CLEARANCE);
}

//===================================================================
// TempHideSpot
// Structure to hold hide points.
// Includes distance to sort the points and both the found hide pos and the object pos/dir.
//===================================================================
struct TempHideSpot
{
	TempHideSpot(const Vec3& pos, const Vec3& objPos, const Vec3& objDir, float rad, float dist, bool collidable, bool directional) :
		pos(pos), objDir(objDir), rad(rad), dist(dist), collidable(collidable), directional(directional) {}
	bool	operator<(const TempHideSpot& rhs) const { return dist < rhs.dist; }
	Vec3	pos, objDir;
	float	rad;
	float	dist;
	bool	collidable;
	bool	directional;
};

//===================================================================
// GetHideSpotsInRange
//===================================================================
unsigned int CAISystem::GetHideSpotsInRange(IAIObject* requester, const Vec3& reqPos,
																				 const Vec3& hideFrom, float minRange, float maxRange, bool collidableOnly, bool validatedOnly,
																				 unsigned int maxPts, Vec3* coverPos, Vec3* coverObjPos, Vec3* coverObjDir, float* coverRad, bool* coverCollidable)
{
	CPuppet* puppet = CastToCPuppetSafe(requester);
//	if (!puppet)
//		return 0;

	IAISystem::tNavCapMask navCapMask = IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_SMARTOBJECT | IAISystem::NAV_LAYERED_NAV_MESH ;
	float	passRadius = 0.5f;
	IEntity* pRequesterEnt = 0;
	unsigned lastNavNodeIndex = 0;

	if (puppet)
	{
		navCapMask = puppet->GetMovementAbility().pathfindingProperties.navCapMask;
		passRadius = puppet->GetParameters().m_fPassRadius;
		pRequesterEnt = puppet->GetEntity();
		lastNavNodeIndex = puppet->m_lastNavNodeIndex;
	}

	MultimapRangeHideSpots hidespots;
	MapConstNodesDistance traversedNodes;

	GetHideSpotsInRange(hidespots, traversedNodes, reqPos, maxRange, 
		navCapMask, passRadius, false, pRequesterEnt, lastNavNodeIndex, (CAIObject*)requester);

	// Only update the obstacles if validation is requested.
	const CPathObstacles* obstacles = 0;
	
	if (puppet)
		obstacles = &puppet->GetPathAdjustmentObstacles(validatedOnly);

	float	minRangeSq = sqr(minRange);
	float	maxRangeSq = sqr(maxRange);

	std::vector<TempHideSpot>	spots;
	spots.reserve(hidespots.size());

	const MultimapRangeHideSpots::iterator liEnd = hidespots.end();
	for (MultimapRangeHideSpots::iterator li = hidespots.begin(); li != liEnd; ++li)
	{
		const SHideSpot &hs = li->second;

		if (hs.info.type == SHideSpotInfo::eHST_SMARTOBJECT)
			continue;

		bool	collidable(true);
		float	rad = 0.0f;

		if(hs.pObstacle && !hs.pObstacle->IsCollidable())
			collidable = false;
		if (hs.pAnchorObject && hs.pAnchorObject->GetType() == AIANCHOR_COMBAT_HIDESPOT_SECONDARY)
		{
			collidable = false;
			rad = 0.2f;
		}

		if(collidableOnly && !collidable)
			continue;

		if(hs.pObstacle)
			rad = hs.pObstacle->fApproxRadius;

		float dist = li->first;
		float distSq = sqr(dist);

		Vec3 pos = hs.info.pos;
		Vec3 dir = hs.info.dir;
		const Vec3& objPos(hs.info.pos); // AlexMcC: should this be referencing pos or objPos?

		if(hs.pObstacle || (hs.pAnchorObject && hs.pAnchorObject->GetType() == AIANCHOR_COMBAT_HIDESPOT_SECONDARY))
			AdjustOmniDirectionalCoverPosition(pos, dir, max(rad, 0.0f), passRadius, hideFrom);

		float d = Distance::Point_PointSq(pos, reqPos);

		if(d >= minRangeSq && d <= maxRangeSq)
			spots.push_back(TempHideSpot(pos, objPos, dir, rad, d, collidable,
				hs.info.type == SHideSpotInfo::eHST_ANCHOR || hs.info.type == SHideSpotInfo::eHST_WAYPOINT || hs.info.type == SHideSpotInfo::eHST_DYNAMIC));
	}

	// Output best points.
	std::sort(spots.begin(), spots.end());

	unsigned int npts = 0;

	for (unsigned i = 0, n = spots.size(); i < n; ++i)
	{
		TempHideSpot&	spot = spots[i];
		// Check if the point is inside another cover object and skip if it is.
		bool embedded = false;
		for (unsigned j = 0, m = spots.size(); j < m; ++j)
		{
			TempHideSpot&	spotj = spots[j];
			if (i == j || !spotj.collidable || spotj.rad < 0.01f) continue;
			if (Distance::Point_Point2DSq(spot.pos, spotj.pos) < sqr(spotj.rad + passRadius))
		{
				embedded = true;
				break;
		}
		}
		if (embedded)
			continue;

		// Discard points which do not offer cover.
		if (validatedOnly)
		{
			if (spot.directional)
			{
				Vec3 dirHideToEnemy = hideFrom - spot.pos;
				dirHideToEnemy.NormalizeSafe();
				if (dirHideToEnemy.Dot(spot.objDir) < HIDESPOT_COVERAGE_ANGLE_COS)
					continue;
			}

			int nBuildingID;
			IVisArea* pArea;
			IAISystem::ENavigationType navType = m_pNavigation->CheckNavigationType(spot.pos, nBuildingID, pArea, navCapMask);
			if (navType == IAISystem::NAV_TRIANGULAR)
		{
				// Discard points inside forbidden areas
				if (m_pNavigation->IsPointForbidden(spot.pos, passRadius, 0, 0))
					continue;
				// Discard points inside dynamic obstacles
				if (obstacles && obstacles->IsPointInsideObstacles(spot.pos))
			continue;
		}
		}

		if(coverPos) coverPos[npts] = spot.pos;
		if(coverObjPos) coverObjPos[npts] = spot.pos;
		if(coverObjDir) coverObjDir[npts] = spot.objDir;
		if(coverRad) coverRad[npts] = spot.rad;
		if(coverCollidable) coverCollidable[npts] = spot.collidable;
		++npts;

		if (npts >= maxPts) break;
	}

	return npts;
}

unsigned int CAISystem::GetHideSpots(HidespotQueryContext& queryContext) const
{
	//CPuppet* puppet = CastToCPuppetSafe(requester);

	IAISystem::tNavCapMask navCapMask = IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_SMARTOBJECT | IAISystem::NAV_LAYERED_NAV_MESH ;
	float	passRadius = 0.5f;
	IEntity* pRequesterEnt = 0;
	unsigned lastNavNodeIndex = 0;

	// TODO: Refactor so it's using direct references
	const Vec3& reqPos = queryContext.centerOfQuery;
	const Vec3& hideFrom = queryContext.hideFromPos;
	const float minRange = queryContext.minRange;
	const float maxRange = queryContext.maxRange;
	const bool collidableOnly = queryContext.onlyCollidable;
	const bool validatedOnly = queryContext.onlyThoseThatGiveCover;
	assert(queryContext.maxPoints > 0);
	unsigned int maxPts = queryContext.maxPoints;
	Vec3* coverPos = queryContext.pCoverPos;
	Vec3* coverObjPos = queryContext.pCoverObjPos;
	Vec3* coverObjDir = queryContext.pCoverObjDir;
	float* coverRad = NULL;
	bool* coverCollidable = NULL;

// 	if (puppet)
// 	{
// 		navCapMask = puppet->GetMovementAbility().pathfindingProperties.navCapMask;
// 		passRadius = puppet->GetParameters().m_fPassRadius;
// 		pRequesterEnt = puppet->GetEntity();
// 		lastNavNodeIndex = puppet->m_lastNavNodeIndex;
// 	}

	MultimapRangeHideSpots hidespots;
	MapConstNodesDistance traversedNodes;

	// TODO: Correct constness in callee
	CAISystem* pThisWithoutConst = const_cast<CAISystem*>(this);

	pThisWithoutConst->GetHideSpotsInRange(hidespots, traversedNodes, reqPos, maxRange, 
		navCapMask, passRadius, false, pRequesterEnt, lastNavNodeIndex, NULL /*(CAIObject*)requester*/);

	// Only update the obstacles if validation is requested.
	const CPathObstacles* obstacles = 0;

// 	if (puppet)
// 		obstacles = &puppet->GetPathAdjustmentObstacles(validatedOnly);

	float	minRangeSq = sqr(minRange);
	float	maxRangeSq = sqr(maxRange);

	std::vector<TempHideSpot>	spots;
	spots.reserve(hidespots.size());

	const MultimapRangeHideSpots::iterator liEnd = hidespots.end();
	for (MultimapRangeHideSpots::iterator li = hidespots.begin(); li != liEnd; ++li)
	{
		const SHideSpot &hs = li->second;

		if (hs.info.type == SHideSpotInfo::eHST_SMARTOBJECT)
			continue;

		bool	collidable(true);
		float	rad = 0.0f;

		if(hs.pObstacle && !hs.pObstacle->IsCollidable())
			collidable = false;
		if (hs.pAnchorObject && hs.pAnchorObject->GetType() == AIANCHOR_COMBAT_HIDESPOT_SECONDARY)
		{
			collidable = false;
			rad = 0.2f;
		}

		if(collidableOnly && !collidable)
			continue;

		if(hs.pObstacle)
			rad = hs.pObstacle->fApproxRadius;

		float dist = li->first;
		float distSq = sqr(dist);

		Vec3 pos = hs.info.pos;
		Vec3 dir = hs.info.dir;
		const Vec3& objPos(hs.info.pos); // AlexMcC: should this be referencing pos or objPos?

		if(hs.pObstacle || (hs.pAnchorObject && hs.pAnchorObject->GetType() == AIANCHOR_COMBAT_HIDESPOT_SECONDARY))
			pThisWithoutConst->AdjustOmniDirectionalCoverPosition(pos, dir, max(rad, 0.0f), passRadius, hideFrom);

		float d = Distance::Point_PointSq(pos, reqPos);

		if(d >= minRangeSq && d <= maxRangeSq)
			spots.push_back(TempHideSpot(pos, objPos, dir, rad, d, collidable,
			hs.info.type == SHideSpotInfo::eHST_ANCHOR || hs.info.type == SHideSpotInfo::eHST_WAYPOINT || hs.info.type == SHideSpotInfo::eHST_DYNAMIC));
	}

	// Output best points.
	std::sort(spots.begin(), spots.end());

	unsigned int npts = 0;

	for (unsigned i = 0, n = spots.size(); i < n; ++i)
	{
		TempHideSpot&	spot = spots[i];
		// Check if the point is inside another cover object and skip if it is.
		bool embedded = false;
		for (unsigned j = 0, m = spots.size(); j < m; ++j)
		{
			TempHideSpot&	spotj = spots[j];
			if (i == j || !spotj.collidable || spotj.rad < 0.01f) continue;
			if (Distance::Point_Point2DSq(spot.pos, spotj.pos) < sqr(spotj.rad + passRadius))
			{
				embedded = true;
				break;
			}
		}
		if (embedded)
			continue;

		// Discard points which do not offer cover.
		if (validatedOnly)
		{
			if (spot.directional)
			{
				Vec3 dirHideToEnemy = hideFrom - spot.pos;
				dirHideToEnemy.NormalizeSafe();
				if (dirHideToEnemy.Dot(spot.objDir) < HIDESPOT_COVERAGE_ANGLE_COS)
					continue;
			}

			int nBuildingID;
			IVisArea* pArea;
			IAISystem::ENavigationType navType = m_pNavigation->CheckNavigationType(spot.pos, nBuildingID, pArea, navCapMask);
			if (navType == IAISystem::NAV_TRIANGULAR)
			{
				// Discard points inside forbidden areas
				if (m_pNavigation->IsPointForbidden(spot.pos, passRadius, 0, 0))
					continue;
				// Discard points inside dynamic obstacles
				if (obstacles && obstacles->IsPointInsideObstacles(spot.pos))
					continue;
			}
		}

		if(coverPos) coverPos[npts] = spot.pos;
		if(coverObjPos) coverObjPos[npts] = spot.pos;
		if(coverObjDir) coverObjDir[npts] = spot.objDir;
		if(coverRad) coverRad[npts] = spot.rad;
		if(coverCollidable) coverCollidable[npts] = spot.collidable;
		++npts;

		if (npts >= maxPts) break;
	}

	return npts;
}

//===================================================================
// GetVisPerceptionDistScale
//===================================================================
void CAISystem::SetPerceptionDistLookUp( float* pLookUpTable, int tableSize )
{
	AILinearLUT& lut(m_VisDistLookUp);
	lut.Set(pLookUpTable, tableSize);
}


//===================================================================
// GetVisPerceptionDistScale
//===================================================================
float CAISystem::GetVisPerceptionDistScale( float distRatio )
{
  AILinearLUT& lut(m_VisDistLookUp);

  // Make sure the dist ratio is in required range [0..1]
  distRatio = clamp(distRatio, 0.0f, 1.0f);

  // If no table is setup, use simple quadratic fall off function.
  if(lut.GetSize() < 2)
    return sqr(1.0f - distRatio);
  return lut.GetValue(distRatio) / 100.0f;
}

//===================================================================
// DebugReportHitDamage
//===================================================================
void CAISystem::DebugReportHitDamage(IEntity* pVictim, IEntity* pShooter, float damage, const char* material)
{
#ifdef CRYAISYSTEM_DEBUG
	if (!pVictim)
		return;

	if (GetPlayer() && pVictim->GetId() == GetPlayer()->GetEntityID())
	{
		if (pShooter)
			m_DEBUG_playerShots.push_back(SDebugPlayerDamage(DBG_PLAYER_HIT, pShooter->GetName(), damage, material, GetFrameStartTime().GetSeconds()));
		else
			m_DEBUG_playerShots.push_back(SDebugPlayerDamage(DBG_PLAYER_HIT, "<Unknown>", damage, material, GetFrameStartTime().GetSeconds()));
	}

	CAIActor *pVictimAI = CastToCAIActorSafe(pVictim->GetAI());
	if (pVictimAI && IsRecording(pVictimAI, IAIRecordable::E_HIT_DAMAGE))
	{
		const unsigned short type = pVictimAI->GetAIType();
		if (type == AIOBJECT_PLAYER || type == AIOBJECT_PUPPET)
		{
			char msg[64];
			_snprintf(msg, 64, "%.1f (%s)", damage, material);
			IAIRecordable::RecorderEventData recorderEventData(msg);

			pVictimAI->RecordEvent(IAIRecordable::E_HIT_DAMAGE, &recorderEventData);
		}
	}
#endif //CRYAISYSTEM_DEBUG
}

//===================================================================
// DebugReportDeath
//===================================================================
void CAISystem::DebugReportDeath(IAIObject* pVictim)
{
#ifdef CRYAISYSTEM_DEBUG
	if (!pVictim) return;

	if(CPuppet* pPuppet = pVictim->CastToCPuppet())
	{
		IAIRecordable::RecorderEventData recorderEventData("Death");
		pPuppet->RecordEvent(IAIRecordable::E_DEATH, &recorderEventData);
	}
	else if(CAIPlayer* pPlayer = pVictim->CastToCAIPlayer())
	{
		if (IsRecording(pPlayer, IAIRecordable::E_DEATH))
		{
			pPlayer->IncDeathCount();
			char	msg[32];
			_snprintf(msg, 32, "Death %d", pPlayer->GetDeathCount());
			IAIRecordable::RecorderEventData recorderEventData(msg);
			pPlayer->RecordEvent(IAIRecordable::E_DEATH, &recorderEventData);
		}
	}
#endif //CRYAISYSTEM_DEBUG
}

//===================================================================
// NotifyDeath
//===================================================================
void CAISystem::NotifyDeath(IAIObject* pVictim)
{
  assert(pVictim);

  if ( pVictim != NULL )
  {
    CAIActor* pActor = pVictim->CastToCAIActor(); 
    if( pActor != NULL )
    {
      pActor->NotifyDeath();
    }
  }
}

//===================================================================
// WouldHumanBeVisible
//===================================================================
bool CAISystem::WouldHumanBeVisible(const Vec3 &footPos, bool fullCheck) const
{
  int ignore = gAIEnv.CVars.IgnoreVisibilityChecks;
  if (ignore)
    return false;

  static float radius = 0.5f;
  static float height = 1.8f;

  const CCamera& cam = GetISystem()->GetViewCamera();
  AABB aabb(AABB::RESET);
  aabb.Add(footPos + Vec3(0, 0, radius), radius);
  aabb.Add(footPos + Vec3(0, 0, height - radius), radius);

  if (!cam.IsAABBVisible_F(aabb))
    return false;

  // todo Danny do fullCheck
  return true;
}

// Static formation point
struct SAIStatFormPt
{
	enum ETested { NOT_TESTED, VALID, INVALID };
	SAIStatFormPt(const Vec3& p, float w, IAISystem::ENavigationType navType) : pos(p), w(w), d(0), vis(0), test(NOT_TESTED), navType(navType) {}
	Vec3	pos;
	float	d;
	float	w;
	unsigned char vis;
	ETested test;
	IAISystem::ENavigationType navType;
};

// Static formation histogram island
struct SAIStatFormIsland
{
	SAIStatFormIsland(int a, int b, float v) : a(a), b(b), v(v) {}
	inline bool operator<(const SAIStatFormIsland& rhs) const { return v < rhs.v; }
	inline bool operator>(const SAIStatFormIsland& rhs) const { return v > rhs.v; }
	int a, b;
	float v;
};

// Static formation unit
struct SAIStatFormUnit
{
	SAIStatFormUnit(const Vec3& pos, unsigned i) : pos(pos), i(i), targetPos(0,0,0), d(0) {}
	inline bool operator<(const SAIStatFormUnit& rhs) const { return d < rhs.d; }
	inline bool operator>(const SAIStatFormUnit& rhs) const { return d > rhs.d; }
	Vec3	pos;
	Vec3	targetPos;
	float	d;
	unsigned i;
};

//===================================================================
// GetStaticFormation
//===================================================================
bool CAISystem::CreateStaticFormation(SAIStaticFormation::EType type, SAIStaticFormation::ELocation location,
																	const std::vector<CPuppet*>& group, const Vec3& targetPos, const Vec3* pPivotPos,
																	float offsetTowardsTarget, float minDistanceToTarget, float searchRange, float spacing,
																	std::vector<std::pair<Vec3, float> >& otherGroups, CAIRadialOcclusionRaycast* pOcclusion,
																	SAIStaticFormation& outFormation)
{
	FUNCTION_PROFILER(gEnv->pSystem,PROFILE_AI);

	if (group.empty())
	{
		outFormation.error = SAIStaticFormation::ERR_GROUP_EMPTY;
		return false;
	}
	
	Vec3	groupPos(0,0,0);
	std::vector<SAIStatFormUnit>	units;
	IAISystem::tNavCapMask navCapMask = NAVMASK_ALL;
	for (unsigned i = 0; i < group.size(); ++i)
	{
		units.push_back(SAIStatFormUnit(group[i]->GetPos(), i));
		const Vec3& pos = group[i]->GetPos();
		groupPos += pos;
		navCapMask &= group[i]->GetMovementAbility().pathfindingProperties.navCapMask;
	}
	groupPos /= (float)units.size();

	if (pPivotPos)
		groupPos = *pPivotPos;


	Vec3	formationForw = targetPos - groupPos;
	formationForw.z = 0;
	formationForw.NormalizeSafe();
	Vec3	formationRight(formationForw.y, -formationForw.x, 0);


	CPuppet* mostFront = 0;
	float mostFrontDist = -FLT_MAX;
	for (unsigned i = 0; i < group.size(); ++i)
	{
		float d = formationForw.Dot(group[i]->GetPos() - groupPos);
		if (d > mostFrontDist)
		{
			mostFrontDist = d;
			mostFront = group[i];
		}
	}
	if (!mostFront)
	{
		outFormation.error = SAIStaticFormation::ERR_GROUP_EMPTY;
		return false;
	}

//	groupPos += mostFrontDist * formationForw;

	outFormation.debugForw = formationForw;
	outFormation.debugRight = formationRight;

	std::vector<SAIStatFormPt>	points;

	MultimapRangeHideSpots hidespots;
	MapConstNodesDistance traversedNodes;

	GetHideSpotsInRange(hidespots, traversedNodes, mostFront->GetPos(), searchRange, navCapMask, 0.0f, false);
	outFormation.debugSearchPos = mostFront->GetPos();

	if(hidespots.empty() && traversedNodes.empty())
	{
		outFormation.error = SAIStaticFormation::ERR_NO_HIDESPOTS;
		return false;
	}

	CAIRadialOcclusion softOcclusion;
	softOcclusion.Reset(targetPos, Distance::Point_Point(targetPos, mostFront->GetPos()) + searchRange*2);

	const float offsetFromCover = 2.0f;

	// Build list of obstacle cover segments.
	for(MultimapRangeHideSpots::iterator it = hidespots.begin(); it != hidespots.end(); ++it)
	{
		float distance = it->first;
		const SHideSpot &hs = it->second;


		IAISystem::ENavigationType navType = IAISystem::NAV_UNSET;
		if (hs.info.type == SHideSpotInfo::eHST_TRIANGULAR)
			navType = IAISystem::NAV_TRIANGULAR;
		else if (hs.info.type == SHideSpotInfo::eHST_WAYPOINT)
			navType = IAISystem::NAV_WAYPOINT_HUMAN;

		if(hs.pObstacle && hs.pObstacle->fApproxRadius > 0.01f)
		{
			Vec3	dir;
			Vec3	pos = hs.info.pos;

			AdjustOmniDirectionalCoverPosition(pos, dir, max(hs.pObstacle->fApproxRadius, 0.0f), 0.4f, targetPos);
			points.push_back(SAIStatFormPt(pos, hs.IsSecondary() ? 0.75f : 1.0f, navType));

			if(hs.pObstacle && !hs.pObstacle->IsCollidable())
			{
				// On soft cover, try points at the side of the object, choose the visible one.
				//			odDir = enemyPos - hs.pObstacle->vPos;
				// Use sides for soft cover
				Vec3	norm(dir.y, -dir.x, 0.0f);
				norm.NormalizeSafe();

				float	rad = max(0.1f, hs.pObstacle->fApproxRadius);

				pos = hs.info.pos + norm * (rad + 1.0f);
				points.push_back(SAIStatFormPt(pos, 0.75f, navType));

				pos = hs.info.pos - norm * (rad + 1.0f);
				points.push_back(SAIStatFormPt(pos, 0.75f, navType));
			}

		}
		else
		{
			points.push_back(SAIStatFormPt(hs.info.pos, hs.IsSecondary() ? 0.75f : 1.0f, navType));
		}

		if(hs.pNavNodes)
		{
			for(std::vector<const GraphNode *>::const_iterator itn = hs.pNavNodes->begin(); itn != hs.pNavNodes->end(); ++itn)
			{
				const GraphNode* node = *itn;
				if(!node) continue;

				Vec3 dir = node->GetPos() - hs.info.pos;
				float	len = dir.GetLengthSquared();
				if(len > sqr(offsetFromCover))
				{
					Vec3	pos = hs.info.pos + dir * offsetFromCover/sqrtf(len);
					points.push_back(SAIStatFormPt(pos, 0.75f, navType));
				}
			}
		}
		if(hs.pNavNode)
		{
			Vec3 dir = hs.pNavNode->GetPos() - hs.info.pos;
			float	len = dir.GetLength();
			if(len > offsetFromCover)
			{
				Vec3	pos = hs.info.pos + dir * offsetFromCover/len;
				points.push_back(SAIStatFormPt(pos, 0.75f, navType));
			}
		}
	}

	VectorSet<int> touchedVertices;

	for(MapConstNodesDistance::iterator it = traversedNodes.begin(); it != traversedNodes.end(); ++it)
	{
		// Rasterize soft vegetation.
		const GraphNode* node = it->first;
		if (node->navType == IAISystem::NAV_TRIANGULAR)
		{
			const STriangularNavData* pTriData = node->GetTriangularNavData();
			for (uint32 i = 0; i < (uint32)pTriData->vertices.size(); ++i)
				touchedVertices.insert(pTriData->vertices[i]);
		}

		const Vec3& p = node->GetPos();
		points.push_back(SAIStatFormPt(p, 0.25f, node->navType));
		for (unsigned link = node->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
		{
			Vec3 dir = m_pGraph->GetLinkManager().GetEdgeCenter(link) - p;
			float	len = dir.GetLengthSquared();
			if(len > sqr(offsetFromCover))
			{
				Vec3	pos = p + dir * offsetFromCover/sqrtf(len);
				points.push_back(SAIStatFormPt(pos, 0.25f, node->navType));
			}
		}
	}

	for (unsigned i = 0, ni = touchedVertices.size(); i < ni; ++i)
	{
		const ObstacleData& obstacle = m_VertexList.GetVertex(touchedVertices[i]);
		if (!obstacle.IsCollidable() && obstacle.fApproxRadius > 0.01f)
		{
			softOcclusion.RasterizeCircle(obstacle.vPos, max(0.1f, obstacle.fApproxRadius));
			outFormation.debugOcclusionObjectCount++;
		}
	}

	outFormation.debugOcclusion = softOcclusion;

	// Calc vis for points
	for (unsigned i = 0, ni = points.size(); i < ni; ++i)
	{
		Vec3 pos = points[i].pos;

		points[i].vis = 0;
		if (!softOcclusion.IsVisible(pos))
		{
			points[i].vis |= 1;
			}

		pos.z += 1.0f;
		if (pOcclusion && !pOcclusion->IsVisible(pos))
		{
			points[i].vis |= 2;
		}
	}


	// Copy the debug points.
	outFormation.debugPoints.resize(points.size());
	for (unsigned i = 0, n = points.size(); i < n; ++i)
		outFormation.debugPoints[i].Set(points[i].pos, points[i].w);

	// find max distance
	float minPtDist = FLT_MAX;
	float maxPtDist = -FLT_MAX;
	for (unsigned i = 0, n = points.size(); i < n; ++i)
	{
		Vec3	p = points[i].pos - groupPos;
		float	d = formationForw.Dot(p);
		minPtDist = min(minPtDist, d);
		maxPtDist = max(maxPtDist, d);
	}

	if (!pPivotPos)
		offsetTowardsTarget += mostFrontDist;

	float distToTarget = Distance::Point_Point(targetPos, groupPos);
	if (offsetTowardsTarget > distToTarget/2)
		offsetTowardsTarget = distToTarget/2;

	if (offsetTowardsTarget > max(0.0f, distToTarget - minDistanceToTarget))
		offsetTowardsTarget = max(0.0f, distToTarget - minDistanceToTarget);

	if (offsetTowardsTarget < minPtDist)
		offsetTowardsTarget = minPtDist;
	if(offsetTowardsTarget > maxPtDist)
		offsetTowardsTarget = maxPtDist;


	Vec3	advanceSpot = groupPos + formationForw * offsetTowardsTarget;


	for (unsigned i = 0, n = units.size(); i < n; ++i)
		units[i].d = formationRight.Dot(units[i].pos - groupPos);
	std::sort(units.begin(), units.end());


	for (unsigned i = 0, n = group.size(); i < n; ++i)
	{
		if (group[i]->GetTerritoryShape())
			group[i]->GetTerritoryShape()->ConstrainPointInsideShape(advanceSpot, true);
	}

	outFormation.debugAdvanceSpot = advanceSpot;

	const int histSamples = 30;
	float	histogram[histSamples];
	const float	histMinDist = -searchRange/2;
	const float	histMaxDist = searchRange/2;
	for (int i = 0; i < histSamples; ++i)
		histogram[i] = 0;
	const float histScale = 1.0f / (histMaxDist - histMinDist);

	const float distThreshold = searchRange; //offsetTowardsTarget * 1.5f;

	for (unsigned i = 0, n = points.size(); i < n; ++i)
	{
		Vec3	p = points[i].pos - advanceSpot;
		points[i].d = formationRight.Dot(p);
		float	dd = formationForw.Dot(p);

		if (fabsf(dd) < distThreshold)
		{
			float a = clamp(points[i].d - histMinDist, 0.0f, histMaxDist - histMinDist) * histScale;
			int	h = (int)(a * (histSamples-1));
			float v = (1.0f / (1+fabsf(dd))) * points[i].w;
			// gently bias the stuff in the center
			v *= 0.8f + sqr(sinf(a * gf_PI/2)) * 0.2f;
			histogram[h] += v;
		}
		else
		{
			points[i].test = SAIStatFormPt::INVALID;
		}
	}

	outFormation.debugHistogram.resize(histSamples);
	for (int i = 0; i < histSamples; ++i)
		outFormation.debugHistogram[i] = histogram[i];


	float	histMinVal, histMaxVal;
	histMinVal = histMaxVal = histogram[0];

	for(int i = 1; i < histSamples-1; ++i)
	{
		histMinVal = min(histMinVal, histogram[i]);
		histMaxVal = max(histMaxVal, histogram[i]);
	}

	std::vector<SAIStatFormIsland>	islands;
	{
		float thr = histMinVal + (histMaxVal - histMinVal) / 3.0f;
		int start = -1;
		float accVal = 0;
		for(int i = 0; i < histSamples; ++i)
		{
			if(start == -1)
			{
				if(histogram[i] > thr)
				{
					accVal = histogram[i];
					start = i;
				}
			}
			else 
			{
				if(histogram[i] < thr)
				{
					islands.push_back(SAIStatFormIsland(start, i, -(i - start) * accVal));
					start = -1;
				}
				else
				{
					accVal += histogram[i];
				}
			}
		}
		if(start != -1)
		{
			islands.push_back(SAIStatFormIsland(start, histSamples, -(histSamples - start) * accVal));
		}
	}

//	std::sort(islands.begin(), islands.end());

	float	bestIsland = FLT_MAX;
	Vec3 bestIslandPos(0,0,0);

	const unsigned formationSize = units.size();
	float	formationWidth = spacing * (formationSize - 1);

	float groupD = formationRight.Dot(groupPos - advanceSpot);

	for (unsigned i = 0, ni = islands.size(); i < ni; ++i)
		{
			float a = (islands[i].a + 0.5f) / (float)histSamples;
			float	b = a + (islands[i].b - islands[i].a) / (float)histSamples;
			float d = (histMinDist + (a+b)/2 * (histMaxDist - histMinDist));

			if(d < histMinDist + formationWidth/2)
				d = histMinDist + formationWidth/2;
			if(d > histMaxDist - formationWidth/2)
				d = histMaxDist - formationWidth/2;

		outFormation.debugIslands.push_back(SAIStaticFormation::SDebugIsland(histMinDist + a * (histMaxDist - histMinDist),
			histMinDist + b * (histMaxDist - histMinDist), d, islands[i].v));

		float groupAddition = 0.0f;

		for (unsigned j = 0, nj = otherGroups.size(); j < nj; ++j)
		{
			std::pair<Vec3, float>& other = otherGroups[j];
			float distToLine = formationForw.Dot(other.first - advanceSpot);
			if (fabsf(distToLine) < other.second)
			{
				float distAlongLine = formationRight.Dot(other.first - advanceSpot);
				if (distAlongLine > (a - other.second) && distAlongLine < (b + other.second))
				{
					groupAddition += searchRange/3;
				}
			}
		}

		if (location == SAIStaticFormation::AISF_NEAREST || location == SAIStaticFormation::AISF_BEST)
		{
			float absd = fabsf(d - groupD) + groupAddition; // * scale;
			if (absd < bestIsland)
			{
				bestIslandPos = advanceSpot + formationRight * d;
				bestIsland = absd;
			}
		}
		else if (location == SAIStaticFormation::AISF_NEAREST_LEFT)
		{
			float absd = fabsf(d - (groupD - searchRange/2)) + groupAddition; // * scale;
			if (absd < bestIsland)
			{
				bestIslandPos = advanceSpot + formationRight * d;
				bestIsland = absd;
			}
		}
		else if (location == SAIStaticFormation::AISF_NEAREST_RIGHT)
		{
			float absd = fabsf(d - (groupD + searchRange/2)) + groupAddition; // * scale;
			if (absd < bestIsland)
			{
				bestIslandPos = advanceSpot + formationRight * d;
				bestIsland = absd;
			}
		}
	}

	if (!bestIslandPos.IsZero())
	{
		const Vec3& forw = formationForw;
		const Vec3& right = formationRight;

		outFormation.formationPoints.resize(units.size());
		outFormation.desiredFormationPoints.resize(units.size());
		outFormation.order.resize(units.size());

		float	avgUnitD = 0;
		float	avgTargetD = 0;

		for(unsigned i = 0; i < formationSize; ++i)
		{
			CPuppet* pPuppet = group[units[i].i];
			IAISystem::tNavCapMask puppetNavCapMask = pPuppet->GetMovementAbility().pathfindingProperties.navCapMask;
			const CPathObstacles &pathAdjustmentObstacles = pPuppet->GetPathAdjustmentObstacles();
			SShape* pTerrShape = pPuppet->GetTerritoryShape();
			float puppetRadius = pPuppet->GetParameters().m_fPassRadius;

			Vec3	desiredFormationPoint;
			if (type == SAIStaticFormation::AISF_ROW)
			{
				// row
				float a = (i - 0.5f * (formationSize-1)) * spacing;
				desiredFormationPoint = bestIslandPos + right*a;
				desiredFormationPoint.z += 0.5f;
			}
			else
			{
				// zig-zag line
				const float s = 1.0f / sqrtf(2.0f);
			float a = (i - 0.5f * (formationSize-1)) * spacing;
				float b = (i & 1) * 2.0f - 1.0f;
				desiredFormationPoint = bestIslandPos + forw*a*s + right*b*s;
				desiredFormationPoint.z += 0.5f;
			}

			float minDist = FLT_MAX;
			Vec3	pos;
			bool found = false;
			for (unsigned j = 0, m = points.size(); j < m; ++j)
			{
				if(points[j].test == SAIStatFormPt::INVALID) continue;

				// Check if the current points is too close to the target points of the already found units.
				bool skip = false;
				for(unsigned k = 0; k < i; ++k)
				{
					if (Distance::Point_PointSq(points[j].pos, units[k].targetPos) < sqr(spacing*0.9f))
					{
						skip = true;
						break;
					}
				}
				if(skip) continue;

				const Vec3& curPos = points[j].pos;

				Vec3	ab = curPos - desiredFormationPoint;
				float dx = right.Dot(ab) * 1.3f;
				float dy = forw.Dot(ab);
				float dz = ab.z;

				float vis = 0;
				if (points[j].vis & 1)
					vis += spacing*2.0f;
				if (points[j].vis & 2)
					vis += spacing*6.0f;

				float	d =  sqrtf(dx*dx + dy*dy + dz*dz) * (1.5f - points[j].w*0.3f) + vis;

				if(d < minDist)
				{
					if(points[j].test == SAIStatFormPt::NOT_TESTED)
					{
						if (pTerrShape && !pTerrShape->IsPointInsideShape(curPos, true))
						{
							points[j].test = SAIStatFormPt::INVALID;
							outFormation.debugPointsOutsideTerritory++;
							continue;
						}

						// Discard the point if it is inside another vegetation obstacle.
						bool	insideObstacle(false);
						for (unsigned k = 0, n = touchedVertices.size(); k < n; ++k)
						{
							const ObstacleData& obstacle = m_VertexList.GetVertex(touchedVertices[k]);
							if (obstacle.IsCollidable() && obstacle.fApproxRadius > 0.01f)
							{
								if (Distance::Point_Point2DSq(curPos, obstacle.vPos) < sqr(max(obstacle.fApproxRadius, 0.0f)))
							{
								insideObstacle = true;
								break;
							}
						}
						}
						if(insideObstacle)
						{
							points[j].test = SAIStatFormPt::INVALID;
							outFormation.debugPointsInsideObstacle++;
							continue;
						}

						int nBuildingID;
						IVisArea* pArea;
						IAISystem::ENavigationType navType = points[j].navType != IAISystem::NAV_UNSET ? points[j].navType :
							m_pNavigation->CheckNavigationType(curPos, nBuildingID, pArea, puppetNavCapMask);

						if (navType == IAISystem::NAV_TRIANGULAR)
						{
							if (m_pNavigation->IsPointForbidden(points[j].pos, puppetRadius, 0, 0))
							{
								points[j].test = SAIStatFormPt::INVALID;
								outFormation.debugPointsInsideForbiddenArea++;
								continue;
							}
							if (pathAdjustmentObstacles.IsPointInsideObstacles(points[j].pos))
							{
								points[j].test = SAIStatFormPt::INVALID;
								outFormation.debugPointsInsidePathObstacles++;
								continue;
							}
						}
					}

					minDist = d;
					pos = points[j].pos;
					found = true;
				}
			}

			if (!found)
			{
				outFormation.error = SAIStaticFormation::ERR_FORMATION_POINT_NOT_FOUND;
				return false;
			}

			avgUnitD += units[i].d;
			avgTargetD += formationRight.Dot(pos - groupPos);

			units[i].targetPos = pos;
			outFormation.formationPoints[units[i].i] = units[i].targetPos;
			outFormation.desiredFormationPoints[units[i].i] = desiredFormationPoint;
			outFormation.order[i] = units[i].i;
		}

		if(avgTargetD - avgUnitD > 0)
			std::reverse(outFormation.order.begin(), outFormation.order.end());

		return true;
	}

	outFormation.error = SAIStaticFormation::ERR_ISLANDS_NOT_FOUND;

	return false;
}

//===================================================================
// ProcessBalancedDamage
//===================================================================
float CAISystem::ProcessBalancedDamage(IEntity* pShooterEntity, IEntity* pTargetEntity, float damage, const char* damageType)
{
 /* //Diesel cut

	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	if (!pShooterEntity || !pTargetEntity || !pShooterEntity->GetAI() || !pTargetEntity->GetAI())
		return damage;

//	AILogEvent("CAISystem::ProcessBalancedDamage: %s -> %s Damage:%f type:%s\n", pShooterEntity->GetName(), pTargetEntity->GetName(), damage, damageType);

	CAIActor* pShooterActor = pShooterEntity->GetAI()->CastToCAIActor();
	if (pShooterActor->GetType() == AIOBJECT_PLAYER)
		return damage;

	if (pShooterActor && pShooterActor->GetProxy() && pShooterActor->GetProxy()->IsDriver() )
	{
		 EntityId vehicleId = pShooterActor->GetProxy()->GetLinkedVehicleEntityId();
		 if (vehicleId)
		 {
			IEntity* pVehicleEnt = gEnv->pEntitySystem->GetEntity(vehicleId);
			if (pVehicleEnt && pVehicleEnt->GetAI())
			{
				if ( pVehicleEnt->GetAI()->CastToCAIActor() )
				{
					pShooterEntity = pVehicleEnt;
					pShooterActor = pShooterEntity->GetAI()->CastToCAIActor();
				}
			}
		 }
	}


	if (!pShooterActor->IsHostile(pTargetEntity->GetAI()))
	{
		// Skip friendly bullets.
		// but only if this is not grabbed AI (human shield.)
		if ( GetPlayer()->GetProxy()->GetGrabbedEntity()!=pTargetEntity && strcmp(damageType, "bullet") == 0 )
			return 0.0f;
	}

		if (strcmp(damageType, "bullet") != 0)
		return damage;

		CPuppet* pShooterPuppet = pShooterEntity->GetAI()->CastToCPuppet();
		CAIActor* pTargetActor = pTargetEntity->GetAI()->CastToCAIActor();
		if (!pShooterPuppet || !pTargetActor)
			return 0.0f;

		if(pShooterPuppet->CanDamageTarget())
		{
			if (gAIEnv.configuration.eCompatibilityMode != ECCM_CRYSIS2)
			{
				if (pTargetActor->GetType() == AIOBJECT_PLAYER)
				{
					if (pTargetActor->GetProxy())
					{
						Vec3 dirToTarget = pShooterPuppet->GetPos() - pTargetActor->GetPos();
						float distToTarget = dirToTarget.NormalizeSafe();

						float dirMod = (1.0f + dirToTarget.Dot(pTargetActor->GetViewDir()))/2;
						float distMod = 1.0f - sqr(1.0f - distToTarget/pShooterPuppet->GetParameters().m_fAttackRange);

						const float maxDirDamageMod = 0.6f;
						const float maxDistDamageMod = 0.7f;

						dirMod = maxDirDamageMod + (1 - maxDirDamageMod) * sqr(dirMod);
						distMod = maxDistDamageMod + (1 - maxDistDamageMod) * distMod;

						damage *= dirMod * distMod;
						damage *= (0.6f + ai_frand() * 0.4f);

#ifdef CRYAISYSTEM_DEBUG

						float maxHealth = (float)pTargetActor->GetProxy()->GetActorMaxHealth();

						DEBUG_AddFakeDamageIndicator(pShooterPuppet, (damage / maxHealth) * 5.0f);

						m_DEBUG_screenFlash += damage / maxHealth;
						if (m_DEBUG_screenFlash > 2.0f)
							m_DEBUG_screenFlash = 2.0f;

#endif //CRYAISYSTEM_DEBUG

					}
				}
			}

			return damage;
		}

	 return 0.0f;
*/ //Diesel cut end

return damage; //Diesel damage wmesto 0
}



//===================================================================
// Interest System
//===================================================================
ICentralInterestManager* CAISystem::GetCentralInterestManager(void)
{
	return CCentralInterestManager::GetInstance();	
}

ICentralInterestManager const* CAISystem::GetCentralInterestManager(void) const
{
	return CCentralInterestManager::GetInstance();	
}

//===================================================================
struct SAIPunchableObject
{
	SAIPunchableObject(const AABB& bounds, float dist, IPhysicalEntity* pPhys) :
		bounds(bounds), pPhys(pPhys), dist(dist) {}

	inline bool operator<(const SAIPunchableObject& rhs) const { return dist < rhs.dist; }

	AABB bounds;
	float dist;
	IPhysicalEntity* pPhys;
};

//===================================================================
// GetNearestPunchableObjectPosition
//===================================================================
bool CAISystem::GetNearestPunchableObjectPosition(IAIObject* pRef, const Vec3& searchPos, float searchRad, const Vec3& targetPos,
																									float minSize, float maxSize, float minMass, float maxMass,
																									Vec3& posOut, Vec3& dirOut, IEntity** objEntOut)
{
	CPuppet* pPuppet = CastToCPuppetSafe(pRef);
	if (!pPuppet)
		return false;

	const float agentRadius = 0.6f;
	const float distToObject = -0.1f;

	std::vector<SAIPunchableObject> objects;

	float searchRadSq = sqr(searchRad);

	AABB aabb(searchPos - Vec3(searchRad, searchRad, searchRad/2), searchPos + Vec3(searchRad, searchRad, searchRad/2));
	PhysicalEntityListAutoPtr entities;
	unsigned nEntities = GetEntitiesFromAABB(entities, aabb, AICE_DYNAMIC);

	const CPathObstacles& pathAdjustmentObstacles = pPuppet->GetPathAdjustmentObstacles();
	IAISystem::tNavCapMask navCapMask = pPuppet->GetMovementAbility().pathfindingProperties.navCapMask;
	const float passRadius = pPuppet->GetParameters().m_fPassRadius;

	Vec3 dirToTarget = targetPos - searchPos;
	dirToTarget.Normalize();

	for (unsigned i = 0 ;i < nEntities ; ++i)
	{
		IPhysicalEntity* pPhysEntity = entities[i];

		// Skip moving objects
		pe_status_dynamics status;
		pPhysEntity->GetStatus(&status);
		if (status.v.GetLengthSquared() > sqr(0.1f))
			continue;
		if (status.w.GetLengthSquared() > sqr(0.1f))
			continue;

		pe_status_pos statusPos;
		pPhysEntity->GetStatus(&statusPos);


		AABB	bounds(statusPos.BBox[0] + statusPos.pos, statusPos.BBox[1] + statusPos.pos);
		Vec3 midBounds = bounds.GetCenter();

		float dist = Distance::Point_Point(statusPos.pos, searchPos);
		if (dist > searchRad)
			continue;

		float rad = bounds.GetRadius();
		if (rad < minSize || rad > maxSize)
			continue;
		float height = bounds.max.z - bounds.min.z;
		if (height < minSize || height > maxSize)
			continue;

		pe_status_dynamics	dyn;
		pPhysEntity->GetStatus(&dyn);
		float mass = dyn.mass;
		if (mass < minMass || mass > maxMass)
			continue;

		// Weight towards target.
		Vec3 dirToObject = statusPos.pos - searchPos;
		dirToObject.NormalizeFast();
		float dot = dirToTarget.Dot(dirToObject);
		float w = 0.25f + 0.75f * sqr((1-dot+1)/2);

		objects.push_back(SAIPunchableObject(bounds, dist * w, pPhysEntity));
	}

	std::sort(objects.begin(), objects.end());

	for (unsigned i = 0; i < objects.size(); ++i)
	{
		// check the altitude
		IPhysicalEntity* pPhysEntity = objects[i].pPhys;

		IEntity * pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhysEntity);
		if (!pEntity)
			continue;

		const AABB& bounds = objects[i].bounds;
		Vec3 center = bounds.GetCenter();

		Vec3 groundPos(center.x, center.y, center.z - 4.0f);
		Vec3 delta = groundPos - center;
		ray_hit hit;
		if (!gEnv->pPhysicalWorld->RayWorldIntersection(center, delta, AICE_ALL, 
			rwi_ignore_noncolliding | rwi_stop_at_pierceable, &hit, 1, &pPhysEntity, 1))
			continue;
		groundPos = hit.pt;

		Vec3 dirObjectToTarget = targetPos - center;
		dirObjectToTarget.z = 0;
		dirObjectToTarget.Normalize();

		float radius = bounds.GetRadius();

		Vec3	posBehindObject = center - dirObjectToTarget * (agentRadius + distToObject + radius);

		// Check if the point is reachable.
		delta = -dirObjectToTarget * (agentRadius + distToObject + radius);
		if (gEnv->pPhysicalWorld->RayWorldIntersection(center, delta, AICE_ALL, 
			rwi_ignore_noncolliding | rwi_stop_at_pierceable, &hit, 1, &pPhysEntity, 1))
		{
			continue;
		}

		float height = bounds.max.z - bounds.min.z;

		// Raycast to find ground pos.
		if (!gEnv->pPhysicalWorld->RayWorldIntersection(posBehindObject, Vec3(0,0,-(height/2 + 1)), AICE_ALL, 
			rwi_ignore_noncolliding | rwi_stop_at_pierceable, &hit, 1, &pPhysEntity, 1))
			continue;
		posBehindObject = hit.pt;
		posBehindObject.z += 0.2f;

		// Check if it possible to stand at the object position.			
		if (OverlapCapsule(Lineseg(Vec3(posBehindObject.x, posBehindObject.y, posBehindObject.z+agentRadius+0.3f),
			Vec3(posBehindObject.x, posBehindObject.y, posBehindObject.z+2.0f - agentRadius)), agentRadius, AICE_ALL))
			continue;

		// Check stuff in front of the object.
		delta = dirObjectToTarget*3.0f;
		Vec3 target = center + delta;
		if (gEnv->pPhysicalWorld->RayWorldIntersection(center, delta, AICE_ALL, 
			rwi_ignore_noncolliding | rwi_stop_at_pierceable, &hit, 1, &pPhysEntity, 1))
		{
			continue;
		}

		// Make sure the location can be reached.
		int nBuildingID;
		IVisArea* pArea;
		IAISystem::ENavigationType navType = m_pNavigation->CheckNavigationType(posBehindObject, nBuildingID, pArea, navCapMask);
		if (navType == IAISystem::NAV_TRIANGULAR)
		{
			if (m_pNavigation->IsPointForbidden(posBehindObject, passRadius, 0, 0))
				continue;
			if (pathAdjustmentObstacles.IsPointInsideObstacles(posBehindObject))
				continue;
		}

		// The object is valid.
		posOut = posBehindObject;
		dirOut = dirObjectToTarget;
		*objEntOut = pEntity;

		return true;
	}

	return false;
}

// NOTE Mai 21, 2007: <pvl> I put this function here because there's no AllNodesContainer.cpp
// and creating it for a single debugging function that might go away any time seems overkill.
// It's also too complex in terms of dependencies to go into AllNodesContainer.h .
// ATTN Mai 21, 2007: <pvl> if something seems strange in this function, it might well be
// a bug.  This is a piece of debugging code that was written too fast.
// Update: <mikko> Changed the validation to reflect the changes in hash space code.
bool CAllNodesContainer::ValidateHashSpace() const
{
	std::auto_ptr <Iterator> nodeIt (new Iterator(*this, IAISystem::NAV_WAYPOINT_HUMAN));
	int numWayptNodes = 0;

	AILogProgress(">>> Validating HashSpace");
	while (unsigned int nodeIndex = nodeIt->GetNode())
	{
		GraphNode * node = gAIEnv.pGraph->GetNodeManager().GetNode(nodeIndex);

		++numWayptNodes;

		typedef std::vector< std::pair<float, unsigned> > TNodes;
		static TNodes nodes;
		nodes.resize(0);

		GetAllNodesWithinRange(nodes, node->GetPos(), 0.05f, IAISystem::NAV_WAYPOINT_HUMAN);

		if (nodes.size() > 1)
			AIWarning ("More than one node within 5cm from each other.");

		bool validated = false;

		TNodes::const_iterator it = nodes.begin();
		TNodes::const_iterator end = nodes.end();
		for ( ; it != end; ++it)
		{
			if (it->second == nodeIndex)
			{
				validated = true;
				break;
			}
		}
		if (!validated)
		{
			AIWarning ("HashSpace probably corrupt - a node in a wrong cell!");
			return false;
		}

		nodeIt->Increment();
	}
	AILogProgress (">>> HashSpace validation: %d waypts in the level", numWayptNodes);

	for (unsigned i = 0, ni = m_hashSpace.GetBucketCount(); i < ni; ++i)
	{
		for (unsigned j = 0, nj = m_hashSpace.GetObjectCountInBucket(i); j < nj; ++j)
		{
			const SGraphNodeRecord& nodeRec = m_hashSpace.GetObjectInBucket(j, i);

			if (!DoesNodeExist(nodeRec.nodeIndex))
			{
				Vec3 nodePos = nodeRec.GetPos(gAIEnv.pGraph->GetNodeManager());
				AIWarning ("HashSpace probably corrupt - contains a node that's not in AllNodesContainer!");
				AIWarning ("  pos = (%5.2f, %5.2f, %5.2f), index = %d", nodePos.x, nodePos.y, nodePos.z, nodeRec.nodeIndex);
				return false;
			}

			int ii, jj, kk;
			m_hashSpace.GetIJKFromPosition(nodeRec.GetPos(gAIEnv.pGraph->GetNodeManager()), ii, jj, kk);
			unsigned bucket = m_hashSpace.GetHashBucketIndex(ii,jj,kk);
			if (i != bucket)
				AIWarning ("HashSpace probably corrupt - a node incorrectly assigned to cell!");
		}
	}

	return true;
}

//====================================================================
// AllocGoalPipeId
//====================================================================
int CAISystem::AllocGoalPipeId() const
{
	CCCPOINT(CAISystem_AllocGoalPipeId);
	static int g_iLastActionId = 0;
	g_iLastActionId++;
	if ( g_iLastActionId < 0 )
		g_iLastActionId = 1;
	return g_iLastActionId;
}

//====================================================================
// CreateSignalExtraData
//====================================================================
IAISignalExtraData* CAISystem::CreateSignalExtraData() const
{
	return new AISignalExtraData;
}

//====================================================================
// FreeSignalExtraData
//====================================================================
void CAISystem::FreeSignalExtraData( IAISignalExtraData* pData ) const
{
	if ( pData )
		delete (AISignalExtraData*) pData;
}
//===================================================================
// Behavior Tree Dict
//===================================================================
CProfileDictionary* CAISystem::GetBTProfileDictionary()
{
  return( m_pBTProfileDictionary );
}

//===================================================================
// Behavior Tree Dict
//===================================================================
CProfileDictionary const* CAISystem::GetBTProfileDictionary() const
{
  return( m_pBTProfileDictionary );
}

//===================================================================
// BSS Profile Manager
//===================================================================
IBSSProfileManager* CAISystem::GetBSSProfileManager()
{
	return((IBSSProfileManager*) m_pBTProfileDictionary);
}

//===================================================================
// Get the tactical point system
//===================================================================
ITacticalPointSystem* CAISystem::GetTacticalPointSystem(void)
{
  return gAIEnv.pTacticalPointSystem;
}

//===================================================================
// Emotional System
//===================================================================
IEmotionalSystem* CAISystem::GetEmotionalSystem()
{
	return( m_pEmotionalSystem );
}

//===================================================================
// Emotional System
//===================================================================
IEmotionalSystem const* CAISystem::GetEmotionalSystem() const
{
	return( m_pEmotionalSystem );
}

ICoordinationManager* CAISystem::GetCoordinationManager() const
{
	return gAIEnv.pCoordinationManager;
}

ICommunicationManager* CAISystem::GetCommunicationManager() const
{
	return gAIEnv.pCommunicationManager;
}

ICoverSystem* CAISystem::GetCoverSystem() const
{
	return gAIEnv.pCoverSystem;
}

ISelectionTreeManager* CAISystem::GetSelectionTreeManager() const
{
	return gAIEnv.pSelectionTreeManager;
}

ITargetTrackManager* CAISystem::GetTargetTrackManager() const
{
	return gAIEnv.pTargetTrackManager;
}


//===================================================================
// GetCooperativeReadabilitiesSystem
//===================================================================
CCoopReadabilitiesSystem* CAISystem::GetCooperativeReadabilitiesSystem() 
{
	return( m_pCoopReadabilitiesSystem );
}

//===================================================================
// GetCooperativeReadabilitiesSystem
//===================================================================
CCoopReadabilitiesSystem const* CAISystem::GetCooperativeReadabilitiesSystem() const
{
	return( m_pCoopReadabilitiesSystem );
}

//===================================================================
// AssignPFPropertiesToPathType
//===================================================================
void CAISystem::AssignPFPropertiesToPathType(const string& sPathType, const AgentPathfindingProperties& properties)
{
	mapPFProperties.insert(PFPropertiesMap::value_type(sPathType, properties));
}

//===================================================================
// GetPFPropertiesOfPathType
//===================================================================
const AgentPathfindingProperties* CAISystem::GetPFPropertiesOfPathType(const string& sPathType)
{
	PFPropertiesMap::iterator iterPFProperties = mapPFProperties.find(sPathType);
	return (iterPFProperties != mapPFProperties.end()) ? &iterPFProperties->second : 0;
}

//===================================================================
// GetRegisteredPathTypes
//===================================================================
string CAISystem::GetPathTypeNames()
{
	string pathNames;
	for (PFPropertiesMap::iterator iter = mapPFProperties.begin(); iter != mapPFProperties.end(); ++iter)
		pathNames += iter->first + ' ';
	return pathNames;
}

//===================================================================
// Parse AI description tables
//===================================================================
bool CAISystem::ParseTables(int firstTable, bool parseMovementAbility, IFunctionHandler* pH, AIObjectParameters& aiParams, bool& updateAlways)
{
	return m_pScriptAI->ParseTables(firstTable, parseMovementAbility, pH, aiParams, updateAlways);
}

void IAISystem::tNavCapMask::Serialize (TSerialize ser)
{
	ser.Value("navCapMask", m_navCaps);
}

void CAISystem::Release()
{
	delete this;
}

void CAISystem::Enable( bool enable/*=true*/ )
{
	m_IsEnabled=enable;
}

CTimeValue CAISystem::GetFrameStartTime() const
{
	return m_fFrameStartTime;
}

float CAISystem::GetFrameDeltaTime() const
{
	return m_fFrameDeltaTime;
}

const ShapeMap & CAISystem::GetGenericShapes() const
{
	return m_mapGenericShapes;
}

const ShapeMap& CAISystem::GetOcclusionPlanes() const
{
	return m_mapOcclusionPlanes;
}

const CAISystem::TDamageRegions& CAISystem::GetDamageRegions() const
{
	return m_damageRegions;
}


CPerceptionManager* CAISystem::GetPerceptionManager()
{
	return CPerceptionManager::GetInstance();
}

CAILightManager* CAISystem::GetLightManager()
{
	return &m_lightManager;
}

CAIDynHideObjectManager* CAISystem::GetDynHideObjectManager()
{
	return &m_dynHideObjectManager;
}

bool CAISystem::IsRecording( const IAIObject* pTarget, IAIRecordable::e_AIDbgEvent event ) const
{
#ifdef CRYAISYSTEM_DEBUG
	return m_DbgRecorder.IsRecording( pTarget, event );
#else
	return false;
#endif //CRYAISYSTEM_DEBUG
}

void CAISystem::Record( const IAIObject* pTarget, IAIRecordable::e_AIDbgEvent event, const char* pString ) const
{
#ifdef CRYAISYSTEM_DEBUG
	m_DbgRecorder.Record( pTarget, event, pString );
#endif //CRYAISYSTEM_DEBUG
}

void CAISystem::GetRecorderDebugContext( SAIRecorderDebugContext* &pContext )
{
	pContext = &m_recorderDebugContext;
}

IAIRecorder * CAISystem::GetIAIRecorder()
{
#ifdef CRYAISYSTEM_DEBUG
	return &m_Recorder;
#else
	return NULL;
#endif //CRYAISYSTEM_DEBUG
}

INavigation * CAISystem::GetINavigation()
{
	return m_pNavigation;
}

IAIPathFinder * CAISystem::GetIAIPathFinder()
{
	return m_pPathfinder;
}

IAIDebugRenderer* CAISystem::GetAIDebugRenderer()
{
	return gAIEnv.GetDebugRenderer();
}

IAIDebugRenderer* CAISystem::GetAINetworkDebugRenderer()
{
	return gAIEnv.GetNetworkDebugRenderer();
}

void CAISystem::SetAINetworkDebugRenderer( IAIDebugRenderer* pAINetworkDebugRenderer )
{
	gAIEnv.SetNetworkDebugRenderer(pAINetworkDebugRenderer);
}

void CAISystem::SetAIDebugRenderer( IAIDebugRenderer* pAIDebugRenderer )
{
	gAIEnv.SetDebugRenderer				(pAIDebugRenderer);
}

const CAISystem::PuppetSet& CAISystem::GetEnabledPuppetSet() const
{
	return m_enabledPuppetsSet;
}

const bool CAISystem::IsEnabled() const
	{
	if(!m_IsEnabled)
		Warning(__FUNCTION__, "Something is trying to access CAISystem before it is enabled!");
	return m_IsEnabled;
	}

IAIActionManager* CAISystem::GetAIActionManager()
{
	return m_pAIActionManager;
}

ISmartObjectManager* CAISystem::GetSmartObjectManager()
{
	return m_pSmartObjectManager;
}

IAIObjectManager* CAISystem::GetAIObjectManager()
{
	return this;//TEMP
}
#include UNIQUE_VIRTUAL_WRAPPER(IAISystem)
