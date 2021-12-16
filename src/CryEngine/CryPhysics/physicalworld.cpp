//////////////////////////////////////////////////////////////////////
//
//	Physical World
//	
//	File: physicalworld.cpp
//	Description : PhysicalWorld class implementation
//
//	History:
//	-:Created by Anton Knyazev
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "bvtree.h"
#include "geometry.h"
#include "overlapchecks.h"
#include "raybv.h"
#include "raygeom.h"
#include "geoman.h"
#include "singleboxtree.h"
#include "boxgeom.h"
#include "cylindergeom.h"
#include "capsulegeom.h"
#include "spheregeom.h"
#include "trimesh.h"
#include "rigidbody.h"
#include "physicalplaceholder.h"
#include "physicalentity.h"
#include "rigidentity.h"
#include "particleentity.h"
#include "livingentity.h"
#include "wheeledvehicleentity.h"
#include "articulatedentity.h"
#include "ropeentity.h"
#include "softentity.h"
#include "tetrlattice.h"
#include "physicalworld.h"
#include "waterman.h"

// x-CR-x
#ifdef PS3
	extern uint32 g_JobAllocSize;
	#undef sys_timer_subusleep_inldb16cyc
	//avoid busy loops
	#define sys_timer_subusleep_inldb16cyc(ms) Sleep(0)
	namespace PhysicsSPUBuffer{ void GetMemoryUsage( ICrySizer *pSizer ); }
#else
	#if !defined(sys_timer_subusleep_inldb16cyc)
		#define sys_timer_subusleep_inldb16cyc(ms)
	#endif
#endif

// forward declarations of memory funcitons to access static data
namespace TriMeshStaticData{ void GetMemoryUsage( ICrySizer *pSizer ); }

CPhysicalWorld::CPhysicalEntityIt::CPhysicalEntityIt(CPhysicalWorld* pWorld)
{
  m_pWorld = pWorld;
  m_refs = 0;

  MoveFirst();
}

bool CPhysicalWorld::CPhysicalEntityIt::IsEnd()
{
  return m_currEntity == m_pWorld->m_nIdsAlloc;
}

IPhysicalEntity* CPhysicalWorld::CPhysicalEntityIt::Next()
{
  if (IsEnd())
    return NULL;

  IPhysicalEntity* pEnt = (IPhysicalEntity*) m_pWorld->m_pEntsById[m_currEntity]->GetEntity();

  ++m_currEntity;
  while (m_currEntity < m_pWorld->m_nIdsAlloc && m_pWorld->m_pEntsById[m_currEntity] == NULL)
    ++m_currEntity;

  return pEnt;
}

IPhysicalEntity* CPhysicalWorld::CPhysicalEntityIt::This()
{
  if (IsEnd())
    return NULL;

  return (IPhysicalEntity*) m_pWorld->m_pEntsById[m_currEntity]->GetEntity();
}

void CPhysicalWorld::CPhysicalEntityIt::MoveFirst()
{
  m_currEntity = 0;
  while (m_currEntity < m_pWorld->m_nIdsAlloc && m_pWorld->m_pEntsById[m_currEntity] == NULL)
    ++m_currEntity;
}

CPhysicalWorld *g_pPhysWorlds[64];
int g_nPhysWorlds;

#if !defined(XENON) && MAX_PHYS_THREADS<=1
int g_physThreadId = -1;
#else
TLS_DEFINE(int*, g_pidxPhysThread);

void MarkAsPhysThread() { 
	static int g_ibufPhysThread[2] = { 1,0 };
	static int *g_lastPtr = g_ibufPhysThread;
	int *ptr = TLS_GET(int*, g_pidxPhysThread);
	if (ptr!=g_lastPtr) {
		ptr = g_ibufPhysThread+(g_lastPtr-g_ibufPhysThread^1);
		*g_lastPtr = MAX_PHYS_THREADS; *ptr = 0;
		TLS_SET(g_pidxPhysThread, g_lastPtr=ptr);
	}
}
void MarkAsPhysWorkerThread(int *pidx) {
	TLS_SET(g_pidxPhysThread, pidx);
}
#endif

#ifdef XENON
int g_idxThisIsPODThread;
struct InitTlsIsPODThread { 
	InitTlsIsPODThread() { g_idxThisIsPODThread = TlsAlloc(); } 
};
static InitTlsIsPODThread now;
#endif


void CreateJobThreads(int nJobs);
int WaitForJobFinish(int &bAllGroupsFinished,int bWait=1);
CMemStream &GetJobAuxStream();


CPhysicalWorld::CPhysicalWorld(ILog *pLog)
{ 
	m_pLog = pLog; 
	Init(); 
	g_pPhysWorlds[g_nPhysWorlds] = this;
	g_nPhysWorlds = min(g_nPhysWorlds+1,sizeof(g_pPhysWorlds)/sizeof(g_pPhysWorlds[0]));
	m_pEntBeingDeleted = 0;
	m_bGridThunksChanged = 0;
	m_bUpdateOnlyFlagged = 0;
	m_lockStep=0;	m_lockQueue=0; m_lockGrid=0; m_lockList=0; m_lockEventsQueue=0;	m_lockEntIdList=0;
	m_lockPODGrid = 0; m_lockFuncProfiler = 0; m_lockEntProfiler = 0;
	m_lockContacts = 0;
	m_idThread = m_idPODThread = -1;
	m_nOnDemandListFailures = 0; m_iLastPODUpdate = 1;
	m_dummyPODcell.zlim[0]=1E10f; m_dummyPODcell.zlim[1]=1E10f;	
	m_lockPotentialPartListUpdate = m_lockPotentialMeshUpdate = 0;
	m_lockNextEntityGroup = 0; m_lockMovedEntsList = 0;
	m_lockPlayerGroups = 0;
	m_lockAuxStepEnt = 0;
	m_nWorkerThreads = 0;
	int i;
	for(i=0;i<=MAX_PHYS_THREADS;i++) {
		m_lockCaller[i] = 0;
		m_threadData[i].szList=0; m_threadData[i].pTmpEntList=0;
	}
	m_nJobRWIsAlloc=0; m_jobRWI=0;
	m_bMassDestruction = 0;
}

CPhysicalWorld::~CPhysicalWorld()
{
	Shutdown();
	int i;
	for(i=0; i<g_nPhysWorlds && g_pPhysWorlds[i]!=this; i++);
	if (i<g_nPhysWorlds)
		g_nPhysWorlds--;
	for(; i<g_nPhysWorlds; i++) g_pPhysWorlds[i] = g_pPhysWorlds[i+1];
	if (!g_nPhysWorlds)
		g_pPhysWorlds[0] = 0;
}


void CPhysicalWorld::Init()
{
	InitGeoman();
	m_pTmpEntList=0; m_pTmpEntList1=0; m_pTmpEntList2=0; m_pGroupMass=0; m_pMassList = 0; m_pGroupIds = 0; m_pGroupNums = 0;
	m_nEnts = 0; m_nEntsAlloc = 0; m_bEntityCountReserved = 0;
	m_pEntGrid = 0;
	m_timePhysics = m_timeSurplus = 0;
	m_timeSnapshot[0]=m_timeSnapshot[1]=m_timeSnapshot[2]=m_timeSnapshot[3] = 0;
	m_iTimeSnapshot[0]=m_iTimeSnapshot[1]=m_iTimeSnapshot[2]=m_iTimeSnapshot[3] = 0;
	m_iTimePhysics = 0;
	int i; for(i=0;i<8;i++) { m_pTypedEnts[i]=m_pTypedEntsPerm[i]=0; m_updateTimes[i]=0; }
	m_pHiddenEnts = 0;
	for(i=0;i<10;i++) m_nTypeEnts[i]=0;
	for(i=0;i<=MAX_PHYS_THREADS;i++) m_pHeightfield[i] = 0;
	m_vars.nMaxSubsteps = 10;
	for(i=0;i<NSURFACETYPES;i++) {
		m_BouncinessTable[i] = 0;
		m_FrictionTable[i] = 1.2f;
		m_DynFrictionTable[i] = 1.2f/1.5f;
		m_SurfaceFlagsTable[i] = 0;
	}
	m_vars.nMaxStackSizeMC = 8;
	m_vars.maxMassRatioMC = 50.0f;
	m_vars.nMaxMCiters = 6000;
	m_vars.nMinMCiters = 4;
	m_vars.nMaxMCitersHopeless = 6000;
	m_vars.accuracyMC = 0.005f;
	m_vars.accuracyLCPCG = 0.005f;
	m_vars.nMaxContacts = 150;
	m_vars.nMaxPlaneContacts = 8;
	m_vars.nMaxPlaneContactsDistress = 4;
	m_vars.nMaxLCPCGsubiters = 120;
	m_vars.nMaxLCPCGsubitersFinal = 250;
	m_vars.nMaxLCPCGmicroiters = 12000;
	m_vars.nMaxLCPCGmicroitersFinal = 25000;
	m_vars.nMaxLCPCGiters = 5;
	m_vars.minLCPCGimprovement = 0.05f;
	m_vars.nMaxLCPCGFruitlessIters = 4;
	m_vars.accuracyLCPCGnoimprovement = 0.05f;
	m_vars.minSeparationSpeed = 0.02f;
	m_vars.maxwCG = 500.0f;
	m_vars.maxvCG = 500.0f;
	m_vars.maxvUnproj = 10.0f;
	m_vars.maxMCMassRatio = 100.0f;
	m_vars.maxMCVel = 15.0f;
	m_vars.maxLCPCGContacts = 100;
	m_vars.bFlyMode = 0;
	m_vars.iCollisionMode = 0;
	m_vars.bSingleStepMode = 0;
	m_vars.bDoStep = 0;
	m_vars.fixedTimestep = 0;
	m_vars.timeGranularity = 0.0001f;
	m_vars.maxWorldStep = 0.2f;
	m_vars.iDrawHelpers = 0;
	m_vars.nMaxSubsteps = 5;
	m_vars.nMaxSurfaces = NSURFACETYPES;
	m_vars.maxContactGap = 0.01f;
	m_vars.maxContactGapPlayer = 0.01f;
	m_vars.bProhibitUnprojection = 1;//2;
	m_vars.bUseDistanceContacts = 0;
	m_vars.unprojVelScale = 10.0f;
	m_vars.maxUnprojVel = 2.5f;
	m_vars.maxUnprojVelRope = 10.0f;
	m_vars.gravity.Set(0,0,-9.8f);
	m_vars.nGroupDamping = 8;
	m_vars.groupDamping = 0.5f;
	m_vars.nMaxSubstepsLargeGroup = 5;
	m_vars.nBodiesLargeGroup = 30;
	m_vars.bEnforceContacts = 1;//1;
	m_vars.bBreakOnValidation = 0;
	m_vars.bLogActiveObjects = 0;
	m_vars.bMultiplayer = 0;
	m_vars.bProfileEntities = 0;
	m_vars.bProfileFunx = 0;
	m_vars.minBounceSpeed = 6;
	m_vars.nGEBMaxCells = 800;
	m_vars.maxVel = 100.0f;
	m_vars.maxVelPlayers = 150.0f;
	m_vars.bSkipRedundantColldet = 1;
	m_vars.penaltyScale = 0.3f;
	m_vars.maxContactGapSimple = 0.03f;
	m_vars.bLimitSimpleSolverEnergy = 1;
	m_vars.nMaxEntityCells = 300000;
	m_vars.nMaxAreaCells = 128;
	m_vars.nMaxEntityContacts = 256;
	m_vars.tickBreakable = 0.1f;
	m_vars.approxCapsLen = 1.2f;
	m_vars.nMaxApproxCaps = 7;
	m_vars.bCGUnprojVel = 0;
	m_vars.bLogLatticeTension = 0;
	m_vars.nMaxLatticeIters = 100000;
	m_vars.bLogStructureChanges = 1;
	m_vars.bPlayersCanBreak = 0;
	m_vars.bMultithreaded = 0;
	m_vars.breakImpulseScale = 1.0f;
	m_vars.jointGravityStep = 1.0f;
	m_vars.massLimitDebris = 1E10f;
	m_vars.maxSplashesPerObj = 0;
	m_vars.splashDist0 = 7.0f; m_vars.minSplashForce0 = 15000.0f;	m_vars.minSplashVel0 = 4.5f;
	m_vars.splashDist1 = 30.0f; m_vars.minSplashForce1 = 150000.0f; m_vars.minSplashVel1 = 10.0f;
	m_vars.lastTimeStep = 0;
	m_vars.numThreads = 1;
	m_vars.numJobs = 4;
	m_vars.helperOffset.zero();
	m_vars.timeScalePlayers = 1.0f;
	MARK_UNUSED m_vars.flagsColliderDebris;
	m_vars.flagsANDDebris = -1;
	m_vars.bDebugExplosions = 0;
	m_vars.ticksPerSecond = 3000000000U;
	m_vars.netMinSnapDist = 0.1f;
	m_vars.netVelSnapMul = 0.1f;
	m_vars.netMinSnapDot = 0.99f;
	m_vars.netAngSnapMul = 0.01f;
	m_vars.netSmoothTime = 5.0f;
	m_zGran = 1.0f/16; m_rzGran = 16;
	m_iNextId = 1;
	m_iNextIdDown=m_lastExtId=m_nExtIds = 0;
	m_pEntsById = 0;
	m_nIdsAlloc = 0;
	m_nOccRes = 0;
	m_nExplVictims = m_nExplVictimsAlloc = 0;
	m_pPlaceholders = 0; m_pPlaceholderMap = 0;
	m_nPlaceholders = m_nPlaceholderChunks = 0;
	m_iLastPlaceholder = -1; 
	m_pPhysicsStreamer = 0;
	m_pEventClient = 0;
	m_nProfiledEnts = 0;
	m_iSubstep = 0;
	m_bWorldStep = 0;
	m_nDynamicEntitiesDeleted = 0;
	m_nQueueSlots = m_nQueueSlotsAlloc = 0;
	m_nQueueSlotSize = QUEUE_SLOT_SZ;
	m_pQueueSlots = 0;
	for(i=0; i<EVENT_TYPES_NUM; i++) {
		m_pFreeEvents[i] = 0;
		m_pEventClients[i][0]=m_pEventClients[i][1] = 0;
		m_nEvents[i] = 0;
	}
	m_pEventFirst = m_pEventLast = 0;
	m_pFirstEventChunk = m_pCurEventChunk = (EventChunk*)(new char[sizeof(EventChunk)+EVENT_CHUNK_SZ]);
	m_szCurEventChunk = 0;
	m_pFirstEventChunk->next = 0;
	m_pGlobalArea = 0; m_nAreas=m_nBigAreas = 0; m_pDeletedAreas = 0;
	m_iLastLogPump = 0;
	m_pFreeContact = m_pLastFreeContact = CONTACT_END(m_pFreeContact);
	m_nFreeContacts = 0;
	m_pExpl = 0;
	m_nExpl = m_nExplAlloc = 0; m_idExpl = 0;
	m_pDeformingEnts = 0; m_nDeformingEnts = m_nDeformingEntsAlloc = 0;
	m_pRenderer = 0;
	m_lockDeformingEntsList = 0;
	m_lockAreas = 0;
	m_matWater = -1; m_bCheckWaterHits = 0;
	g_StaticPhysicalEntity.m_pWorld = this;
	g_StaticPhysicalEntity.m_id = -2;
	g_StaticPhysicalEntity.m_parts[0].pos.zero();
	g_StaticPhysicalEntity.m_parts[0].q.SetIdentity();
	g_StaticPhysicalEntity.m_parts[0].scale = 1.0f;
	memset(g_StaticPhysicalEntity.m_pUsedParts = new int[MAX_PHYS_THREADS+1][16], 0, sizeof(*g_StaticPhysicalEntity.m_pUsedParts));
	m_rwiQueueHead=-1; m_rwiQueueTail=-64; m_rwiQueueSz=m_rwiQueueAlloc = 0;
	m_rwiQueue = 0; m_lockRwiQueue = 0;
	m_pRwiHitsTail = (m_pRwiHitsHead = new ray_hit[256])+255;
	for(i=0;i<255;i++) m_pRwiHitsHead[i].next = m_pRwiHitsHead+i+1;
	m_pRwiHitsHead[i].next = m_pRwiHitsHead; m_rwiPoolEmpty = 1;
	m_rwiHitsPoolSize = 256; m_lockRwiHitsPool = 0; m_lockTPR = 0;
	m_pwiQueueHead=-1; m_pwiQueueTail=0; m_pwiQueueSz=m_pwiQueueAlloc = 0;
	m_pwiQueue = 0; m_lockPwiQueue = 0;
	m_breakQueueHead=-1; m_breakQueueTail=0; m_breakQueueSz=m_breakQueueAlloc = 0;
	m_breakQueue = 0; m_lockBreakQueue = 0;
	m_pWaterMan = 0;
	m_idStep = 0;
	m_nGEA[0]=m_nGEA[1] = 0; m_nEntListAllocs = 0;
	m_curGroupMass = 0;
	m_pPODCells = &(m_pDummyPODcell=&m_dummyPODcell); m_dummyPODcell.lifeTime = 1E10f;
	m_dummyPODcell.zlim.set(1E10f,-1E10f);
	m_log2PODscale = 0;
	m_iActivePODCell0 = -1;	m_bHasPODGrid = 0; m_iLastPODUpdate = 1;
	m_nProfileFunx=m_nProfileFunxAlloc = 0; m_pFuncProfileData = 0;
	m_posViewer.zero();
	InitGThunksPool();
	m_bCurGroupInvisible = 0;
	m_nContactsAlloc = 0;
	for(i=0;i<=MAX_PHYS_THREADS;i++) {
		m_prevGEABBox[i][0].Set(1E10f,1E10f,1E10f); m_prevGEABBox[i][1].zero();
		m_prevGEAobjtypes[i]=m_nprevGEAEnts[i] = 0;
		m_BBoxPlayerGroup[i][0]=m_BBoxPlayerGroup[i][1] = Vec3(1e10f);
	}
}

void CPhysicalWorld::InitGThunksPool()
{
	{
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Physics, 0, "Physical Grid Pool");
		m_gthunks = new pe_gridthunk[m_thunkPoolSz=16384];
	}

	memset(m_gthunks, 0, 16384*sizeof(pe_gridthunk));
	int i; for(i=0;i<m_thunkPoolSz-1;i++) m_gthunks[i].inextOwned = i+1;
	m_gthunks[i].inextOwned = 0;
	m_gthunks[0].inext=m_gthunks[0].iprev = 0;
	m_iFreeGThunk0=1; 
}


void CPhysicalWorld::Shutdown(int bDeleteGeometries)
{
	int i; CPhysicalEntity *pent,*pent_next;
	m_bMassDestruction = 1;
	for(i=0;i<8;i++) {
		for(pent=m_pTypedEnts[i]; pent; pent=pent_next) 
		{ pent_next=pent->m_next; delete pent; }
		m_pTypedEnts[i] = 0; m_pTypedEntsPerm[i] = 0;
	}
	for(pent=m_pHiddenEnts; pent; pent=pent_next) 
	{ pent_next=pent->m_next; delete pent; }
	m_pHiddenEnts = 0;
	m_nEnts = m_nEntsAlloc = 0; m_bEntityCountReserved = 0;
	for(i=0;i<m_nPlaceholderChunks;i++) if (m_pPlaceholders[i])
		delete[] m_pPlaceholders[i];
	if (m_pPlaceholders) delete[] m_pPlaceholders;
	if (m_pPlaceholderMap) delete[] m_pPlaceholderMap;
	m_nPlaceholderChunks = m_nPlaceholders = 0;
	m_iLastPlaceholder = -1;
	if (m_pEntGrid) DeallocateGrid(m_pEntGrid,m_entgrid.size);
	m_pEntGrid=0;
	if (m_gthunks) delete[] m_gthunks;
	m_gthunks=0; m_thunkPoolSz=m_iFreeGThunk0=0;
	if (m_pTmpEntList) delete[] m_pTmpEntList; m_pTmpEntList = 0;
	if (m_pTmpEntList1) delete[] m_pTmpEntList1; m_pTmpEntList1 = 0;
	if (m_pTmpEntList2) delete[] m_pTmpEntList2; m_pTmpEntList2 = 0;
	if (m_pGroupMass) delete[] m_pGroupMass; m_pGroupMass = 0;
	if (m_pMassList) delete[] m_pMassList; m_pMassList = 0;
	if (m_pGroupIds) delete[] m_pGroupIds; m_pGroupIds = 0;
	if (m_pGroupNums) delete[] m_pGroupNums; m_pGroupNums = 0;
	if (m_pEntsById) delete[] m_pEntsById; m_pEntsById = 0;	m_nIdsAlloc = 0;
	if (m_nOccRes) for(i=0;i<6;i++) {
		delete[] m_pGridStat[i]; delete[] m_pGridDyn[i];
	}
	if (m_nExplVictimsAlloc) {
		delete[] m_pExplVictims; delete[] m_pExplVictimsFrac; delete[] m_pExplVictimsImp;
	}

	if (bDeleteGeometries) {
		for(i=0; i<m_nQueueSlots; i++) delete[] m_pQueueSlots[i];
		if (m_nQueueSlots) delete [] m_pQueueSlots;
		m_nQueueSlots = m_nQueueSlotsAlloc = 0;
		m_nQueueSlotSize = QUEUE_SLOT_SZ;
		m_pQueueSlots = 0;
		for(i=0; i<EVENT_TYPES_NUM; i++) {
			EventClient *pClient,*pNextClient;
			for(pClient=m_pEventClients[i][0]; pClient; pClient=pNextClient) {
				pNextClient=pClient->next; delete pClient;
			}
			for(pClient=m_pEventClients[i][1]; pClient; pClient=pNextClient) {
				pNextClient=pClient->next; delete pClient;
			}
			m_pEventClients[i][0]=m_pEventClients[i][1] = 0;
		}
		m_pEventFirst = m_pEventLast = 0;
		EventChunk *pChunk,*pChunkNext;
		for(pChunk=m_pFirstEventChunk;pChunk;pChunk=pChunkNext) {
			pChunkNext=pChunk->next; delete[] pChunk;
		}
		m_szCurEventChunk = 0; m_pFirstEventChunk=m_pCurEventChunk = 0;
		entity_contact *pContact,*pContactNext;
		for(pContact=m_pFreeContact; pContact!=CONTACT_END(m_pFreeContact); pContact=pContact->next) if (!pContact->bChunkStart) {
			pContact->prev->next=pContact->next; pContact->next->prev=pContact->prev;
		}
		for(pContact=m_pFreeContact; pContact!=CONTACT_END(m_pFreeContact); pContact=pContactNext) {
			pContactNext = pContact->next; delete[] pContact;
		}
		m_pFreeContact = m_pLastFreeContact = CONTACT_END(m_pFreeContact);
		m_nContactsAlloc = 0;
		if (m_pExpl) { 
			for(i=0;i<m_nExpl;i++) m_pExpl[i].pGeom->Release();
			delete[] m_pExpl; m_pExpl = 0; 
		}
		m_nExpl = m_nExplAlloc = 0; m_idExpl = 0;
		if (m_rwiQueue) delete[] m_rwiQueue;
		m_rwiQueueHead=-1; m_rwiQueueTail=-64; m_rwiQueueSz=m_rwiQueueAlloc = 0;
		m_rwiQueue = 0; m_lockRwiQueue = 0;
		if (m_pRwiHitsHead) {
			ray_hit *phit=m_pRwiHitsHead,*pchunk=0,*phit_next;
			do { 
				if ((phit_next=phit->next)!=phit+1) {
					if (pchunk) delete[] pchunk; pchunk = phit_next;
			} } while ((phit=phit_next)!=m_pRwiHitsHead);
			delete[] pchunk;
			m_pRwiHitsHead=m_pRwiHitsTail = 0;
			m_rwiHitsPoolSize = 0;
		}
		if (m_pwiQueue) delete[] m_pwiQueue;
		m_pwiQueueHead=-1; m_pwiQueueTail=0; m_pwiQueueSz=m_pwiQueueAlloc = 0;
		m_pwiQueue = 0; m_lockPwiQueue = 0;
		if (m_breakQueue) delete[] m_breakQueue;
		m_breakQueueHead=-1; m_breakQueueTail=0; m_breakQueueSz=m_breakQueueAlloc = 0;
		m_breakQueue = 0; m_lockBreakQueue = 0;
		DestroyWaterManager();

		SetHeightfieldData(0);
		ShutDownGeoman();

		for(i=m_nWorkerThreads-1;i>=0;i--) m_threads[i]->bStop=1,m_threadStart[i].Set(),m_threadDone[i].Wait();
		CreateJobThreads(0);
	}
	m_bMassDestruction = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////


IPhysicalEntity *CPhysicalWorld::SetHeightfieldData(const heightfield *phf, int *pMatMapping,int nMats)
{
	int iCaller;
	if (!phf) {
		for(iCaller=0;iCaller<=MAX_PHYS_THREADS;iCaller++) {
			if (m_pHeightfield[iCaller]) {
				m_pHeightfield[iCaller]->m_parts[0].pPhysGeom->pGeom->Release();
				delete m_pHeightfield[iCaller]->m_parts[0].pPhysGeom;
				m_pHeightfield[iCaller]->m_parts[0].pPhysGeom = 0;
				if (m_pHeightfield[iCaller]->m_parts[0].pMatMapping) 
					delete[] m_pHeightfield[iCaller]->m_parts[0].pMatMapping;
				delete m_pHeightfield[iCaller];
			}
			m_pHeightfield[iCaller] = 0;
		}
		return 0;
	}
	for(iCaller=0; iCaller<=MAX_PHYS_THREADS; iCaller++) {
		CGeometry *pGeom = (CGeometry*)CreatePrimitive(heightfield::type, phf);
		if (m_pHeightfield[iCaller])	{
			m_pHeightfield[iCaller]->m_parts[0].pPhysGeom->pGeom->Release();
			if (m_pHeightfield[iCaller]->m_parts[0].pMatMapping)
				delete[] m_pHeightfield[iCaller]->m_parts[0].pMatMapping;
		} else {
			m_pHeightfield[iCaller] = new CPhysicalEntity(this);
			m_pHeightfield[iCaller]->m_parts[0].pPhysGeom = m_pHeightfield[iCaller]->m_parts[0].pPhysGeomProxy = new phys_geometry;
			memset(m_pHeightfield[iCaller]->m_parts[0].pPhysGeom, 0, sizeof(phys_geometry));
			m_pHeightfield[iCaller]->m_parts[0].id = 0;
			m_pHeightfield[iCaller]->m_parts[0].scale = 1.0;
			m_pHeightfield[iCaller]->m_parts[0].mass = 0;
			m_pHeightfield[iCaller]->m_parts[0].flags = geom_collides;
			m_pHeightfield[iCaller]->m_parts[0].minContactDist = phf->step.x;
			m_pHeightfield[iCaller]->m_parts[0].idmatBreakable = -1;
			m_pHeightfield[iCaller]->m_parts[0].pLattice = 0;
			m_pHeightfield[iCaller]->m_parts[0].nMats = 0;
			m_pHeightfield[iCaller]->m_nParts = 1;
			m_pHeightfield[iCaller]->m_id = -1;
		}
		if (pMatMapping)
			memcpy(m_pHeightfield[iCaller]->m_parts[0].pMatMapping = new int[nMats], pMatMapping, 
				(m_pHeightfield[iCaller]->m_parts[0].nMats=nMats)*sizeof(int));
		else 
			m_pHeightfield[iCaller]->m_parts[0].pMatMapping = 0;
		m_HeightfieldBasis = phf->Basis;
		m_HeightfieldOrigin = phf->origin;
		m_pHeightfield[iCaller]->m_parts[0].pPhysGeom->pGeom = pGeom;
		m_pHeightfield[iCaller]->m_parts[0].pos = phf->origin;
		m_pHeightfield[iCaller]->m_parts[0].q = !quaternionf(phf->Basis);
		m_pHeightfield[iCaller]->m_parts[0].BBox[0].zero();
		m_pHeightfield[iCaller]->m_parts[0].BBox[1].zero();
	}
	return m_pHeightfield[0];
}

IPhysicalEntity *CPhysicalWorld::GetHeightfieldData(heightfield *phf)
{
	if (m_pHeightfield[0]) {
		m_pHeightfield[0]->m_parts[0].pPhysGeom->pGeom->GetPrimitive(0,phf);
		phf->Basis = m_HeightfieldBasis;
		phf->origin = m_HeightfieldOrigin;
	}
	return m_pHeightfield[0];
}


void CPhysicalWorld::SetupEntityGrid(int axisz, Vec3 org, int nx,int ny, float stepx,float stepy, int log2PODscale)
{
	WriteLock lockGrid(m_lockGrid); 
	if (m_pEntGrid) {
		int i;
		if (m_pEntsById) for(i=0;i<m_iNextId;i++) if (m_pEntsById[i]) {
			DetachEntityGridThunks(m_pEntsById[i]);
			if (!m_pEntsById[i]->m_pEntBuddy || m_pEntsById[i]->m_pEntBuddy==m_pEntsById[i]) {
				CPhysicalEntity *pent = (CPhysicalEntity*)m_pEntsById[i];
				for(int j=0;j<pent->m_nParts;j++) if (pent->m_parts[j].pPlaceholder)
					DetachEntityGridThunks(pent->m_parts[j].pPlaceholder);
			}
		}
		for(CPhysArea *pArea=m_pGlobalArea; pArea; pArea=pArea->m_next)
			DetachEntityGridThunks(pArea);
		for	(i=m_entgrid.size.x*m_entgrid.size.y;i>=0;i--) if (m_pEntGrid[i])
			m_gthunks[m_pEntGrid[i]].iprev = 0;
		DeallocateGrid(m_pEntGrid,m_entgrid.size);
		InitGThunksPool();
	}

	//Before changing m_entgrid.size, delete the on demand grid so
	//we don't treat the old grid as valid and write/delete out of bounds.
	DeactivateOnDemandGrid();

	nx = min(1024,nx);
	ny = min(1024,ny);
	m_iEntAxisz = axisz;
	m_entgrid.size.set(nx,ny);
	m_entgrid.stride.set(1,nx);
	m_entgrid.step.set(stepx,stepy);
	m_entgrid.stepr.set(1.0f/stepx,1.0f/stepy);
	m_entgrid.origin = org;
	m_entgrid.Basis.SetIdentity();
	AllocateGrid(m_pEntGrid,m_entgrid.size);
	m_log2PODscale = log2PODscale;
	m_PODstride.set(1,ny>>3+m_log2PODscale);
}

void CPhysicalWorld::DeactivateOnDemandGrid()
{
	if (m_bHasPODGrid) {
		int i;
		for(i=m_entgrid.size.x*m_entgrid.size.y>>(3+m_log2PODscale)*2;i>=0;i--) if (m_pPODCells[i])
			delete[] m_pPODCells[i];
		delete[] m_pPODCells; 
		m_pPODCells = &m_pDummyPODcell; m_dummyPODcell.lifeTime = 1E10f;
		m_iActivePODCell0 = -1;	m_bHasPODGrid = 0;
		for(i=0;i<8;i++) for(CPhysicalEntity *pent=m_pTypedEnts[i]; pent; pent=pent->m_next) {
			pent->m_nRefCount-=pent->m_nRefCountPOD; pent->m_nRefCountPOD=0;
		}
	}
}

void CPhysicalWorld::RegisterBBoxInPODGrid(const Vec3 *BBox)
{
	int i,ix,iy,igx[2],igy[2],imask;
	pe_PODcell *pPODcell;
	WriteLock lockPOD(m_lockPODGrid);
	if (!m_bHasPODGrid) {
		if ((m_entgrid.size.x|m_entgrid.size.y) & 7)
			return;
		i = (m_entgrid.size.x*m_entgrid.size.y>>(3+m_log2PODscale)*2)+1;
		memset(m_pPODCells = new pe_PODcell*[i], 0, i*sizeof(m_pPODCells[0]));
		m_iActivePODCell0 = -1;	m_bHasPODGrid = 1;
	}
	
	for(i=0;i<2;i++) {
		igx[i] = max(-1,min(m_entgrid.size.x,float2int((BBox[i][inc_mod3[m_iEntAxisz]]-m_entgrid.origin[inc_mod3[m_iEntAxisz]])*m_entgrid.stepr.x-0.5f)));
		igy[i] = max(-1,min(m_entgrid.size.y,float2int((BBox[i][dec_mod3[m_iEntAxisz]]-m_entgrid.origin[dec_mod3[m_iEntAxisz]])*m_entgrid.stepr.y-0.5f)));
		igx[i] >>= m_log2PODscale; igy[i] >>= m_log2PODscale;
	}
	for(ix=igx[0];ix<=igx[1];ix++) for(iy=igy[0];iy<=igy[1];iy++) {
		imask = -(inrange(ix,-1,m_entgrid.size.x>>m_log2PODscale) & inrange(iy,-1,m_entgrid.size.y>>m_log2PODscale));
		i = (ix>>3)*m_PODstride.x + (iy>>3)*m_PODstride.y;
		i = i + ((m_entgrid.size.x*m_entgrid.size.y>>(3+m_log2PODscale)*2)-i & ~imask);
		if (!m_pPODCells[i]) {
			memset(m_pPODCells[i] = new pe_PODcell[64], 0, sizeof(pe_PODcell)*64);
			for(int j=0;j<64;j++) m_pPODCells[i][j].zlim.set(1000.0f,-1000.0f);
		}
		pPODcell = m_pPODCells[i] + ((ix&7)+(iy&7)*8 & imask);
		pPODcell->zlim[0] = min(pPODcell->zlim[0], BBox[0][m_iEntAxisz]);
		pPODcell->zlim[1] = max(pPODcell->zlim[1], BBox[1][m_iEntAxisz]);
		if (pPODcell->lifeTime>0) {
			MarkAsPODThread(this);
			Vec3 center,sz; GetPODGridCellBBox(ix<<m_log2PODscale,iy<<m_log2PODscale, center,sz);
			m_pPhysicsStreamer->DestroyPhysicalEntitiesInBox(center-sz,center+sz);
			pPODcell->lifeTime = -1;
			int *picellNext;
			pe_PODcell *pPODcell1;
			for(i=m_iActivePODCell0,picellNext=&m_iActivePODCell0; i>=0; i=pPODcell1->inextActive) 
				if (pPODcell==(pPODcell1=getPODcell(i&0xFFFF,i>>16))) {
					*picellNext = pPODcell->inextActive; break;
				} else 
					picellNext = &pPODcell1->inextActive;
			UnmarkAsPODThread(this);
    }
	}
}

int CPhysicalWorld::AddRefEntInPODGrid(IPhysicalEntity *_pent, const Vec3 *BBox)
{
	CPhysicalEntity *pent = (CPhysicalEntity*)_pent;
	WriteLock lockPOD(m_lockPODGrid);
	int i,ix,iy,nCells=0;
	Vec2i ig[2];
	const Vec3 *pBBox = BBox ? BBox:pent->m_BBox;
	for(i=0;i<2;i++) {
		ig[i].x = max(-1,min(m_entgrid.size.x,float2int((pBBox[i][inc_mod3[m_iEntAxisz]]-m_entgrid.origin[inc_mod3[m_iEntAxisz]])*m_entgrid.stepr.x-0.5f)));
		ig[i].y = max(-1,min(m_entgrid.size.y,float2int((pBBox[i][dec_mod3[m_iEntAxisz]]-m_entgrid.origin[dec_mod3[m_iEntAxisz]])*m_entgrid.stepr.y-0.5f)));
		ig[i].x >>= m_log2PODscale; ig[i].y >>= m_log2PODscale;
	}
	for(ix=ig[0].x;ix<=ig[1].x;ix++) for(iy=ig[0].y;iy<=ig[1].y;iy++)
		if (getPODcell(ix<<m_log2PODscale,iy<<m_log2PODscale)->lifeTime>0)
			++pent->m_nRefCount,++pent->m_nRefCountPOD,++nCells;
	return nCells;
}


void CPhysicalWorld::GetPODGridCellBBox(int ix,int iy, Vec3 &center,Vec3 &size)
{
#if defined(XENON) || defined(PS3)
	enum {X=0, Y=1, SCALE=2};
	volatile int64 v[3] = { ix >> m_log2PODscale, iy >> m_log2PODscale, 1 << m_log2PODscale};	// Mitigate the load-hit-stores, volatile forces the store

	const int xx = inc_mod3[m_iEntAxisz];
	const int yy = dec_mod3[m_iEntAxisz];
	
	pe_PODcell *pPODcell = getPODcell(ix,iy);
	center = m_entgrid.origin;
	center[m_iEntAxisz] = (pPODcell->zlim[1]+pPODcell->zlim[0])*0.5f;
	size[m_iEntAxisz] = (pPODcell->zlim[1]-pPODcell->zlim[0])*0.5f;
	float scale = (float)v[SCALE];
	Vec2 step = m_entgrid.step*scale;
	size[xx] = step.x*0.5f;
	size[yy] = step.y*0.5f;
	center[xx] += ((float)v[X]+0.5f)*step.x;
	center[yy] += ((float)v[Y]+0.5f)*step.y;
#else
	Vec2 step = m_entgrid.step*(1<<m_log2PODscale);
	pe_PODcell *pPODcell = getPODcell(ix,iy);
	center = m_entgrid.origin;
	center[inc_mod3[m_iEntAxisz]] += ((ix>>m_log2PODscale)+0.5f)*step.x;
	center[dec_mod3[m_iEntAxisz]] += ((iy>>m_log2PODscale)+0.5f)*step.y;
	center[m_iEntAxisz] = (pPODcell->zlim[1]+pPODcell->zlim[0])*0.5f;
	size[inc_mod3[m_iEntAxisz]] = step.x*0.5f;
	size[dec_mod3[m_iEntAxisz]] = step.y*0.5f;
	size[m_iEntAxisz] = (pPODcell->zlim[1]-pPODcell->zlim[0])*0.5f;
#endif
}


int CPhysicalWorld::SetSurfaceParameters(int surface_idx, float bounciness,float friction, unsigned int flags)
{
	if ((unsigned int)surface_idx>=(unsigned int)NSURFACETYPES)
		return 0;
	m_BouncinessTable[surface_idx] = bounciness;
	m_FrictionTable[surface_idx] = friction;
	m_DynFrictionTable[surface_idx] = friction*(1.0/1.5);
	m_SurfaceFlagsTable[surface_idx] = flags;
	m_DamageReductionTable[surface_idx] = 0;
	m_RicochetAngleTable[surface_idx] = 0;
	m_RicDamReductionTable[surface_idx] = 0;
	m_RicVelReductionTable[surface_idx] = 0;
	return 1;
}
int CPhysicalWorld::GetSurfaceParameters(int surface_idx, float &bounciness,float &friction, unsigned int &flags)
{
	if ((unsigned int)surface_idx>=(unsigned int)NSURFACETYPES)
		return 0;
	bounciness = m_BouncinessTable[surface_idx];
	friction = m_FrictionTable[surface_idx];
	flags = m_SurfaceFlagsTable[surface_idx];
	return 1;
}

int CPhysicalWorld::SetSurfaceParameters(int surface_idx, float bounciness, float friction,
																				 float damage_reduction, float ric_angle, float ric_dam_reduction,
																				 float ric_vel_reduction, unsigned int flags)
{
	if ((unsigned int)surface_idx>=(unsigned int)NSURFACETYPES)
		return 0;
	SetSurfaceParameters(surface_idx, bounciness, friction, flags);
	m_DamageReductionTable[surface_idx] = damage_reduction;
	m_RicochetAngleTable[surface_idx] = ric_angle;
	m_RicDamReductionTable[surface_idx] = ric_dam_reduction;
	m_RicVelReductionTable[surface_idx] = ric_vel_reduction;
	return 1;
}

int CPhysicalWorld::GetSurfaceParameters(int surface_idx, float &bounciness,float &friction,
																				 float &damage_reduction, float &ric_angle, float &ric_dam_reduction,
																				 float &ric_vel_reduction, unsigned int &flags)
{
	if ((unsigned int)surface_idx>=(unsigned int)NSURFACETYPES)
		return 0;
	GetSurfaceParameters(surface_idx, bounciness, friction, flags);
	damage_reduction = m_DamageReductionTable[surface_idx];
	ric_angle = m_RicochetAngleTable[surface_idx];
	ric_dam_reduction = m_RicDamReductionTable[surface_idx];
	ric_vel_reduction = m_RicVelReductionTable[surface_idx];
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////


IPhysicalEntity* CPhysicalWorld::CreatePhysicalEntity(pe_type type, float lifeTime, pe_params* params, void *pForeignData,int iForeignData, 
																											int id, IPhysicalEntity *pHostPlaceholder)
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );

	CPhysicalEntity *res=0;
	CPhysicalPlaceholder *pEntityHost = (CPhysicalPlaceholder*)pHostPlaceholder;

//#ifdef _DEBUG
//	{ WriteLockCond lock(m_lockStep, pForeignData!=0 || iForeignData!=0x5AFE);	// since in debug memory manager is not thread-safe (but dll-specific)
//#endif

	switch (type) {
		case PE_STATIC: res = new CPhysicalEntity(this); break;
		case PE_RIGID : res = new CRigidEntity(this); break;
		case PE_LIVING: res = new CLivingEntity(this); break;
		case PE_WHEELEDVEHICLE: res = new CWheeledVehicleEntity(this); break;
		case PE_PARTICLE: res = new CParticleEntity(this); break;
		case PE_ARTICULATED: res = new CArticulatedEntity(this); break;
		case PE_ROPE: res = new CRopeEntity(this); break;
		case PE_SOFT: res = new CSoftEntity(this); break;
	}
	m_nTypeEnts[type]++;
	if (!res)
		return 0;

//#ifdef _DEBUG
//	}
//#endif

	if (type!=PE_STATIC)
		m_nDynamicEntitiesDeleted = 0;
	if (pEntityHost && lifeTime>0) {
		res->m_pForeignData = pEntityHost->m_pForeignData;
		res->m_iForeignData = pEntityHost->m_iForeignData;
		res->m_iForeignFlags = pEntityHost->m_iForeignFlags;
		res->m_id = pEntityHost->m_id;
		res->m_pEntBuddy = pEntityHost;
		pEntityHost->m_pEntBuddy = res;
		res->m_maxTimeIdle = lifeTime;
		res->m_bPermanent = 0;
		res->m_iGThunk0 = pEntityHost->m_iGThunk0;
		res->m_ig[0].x=pEntityHost->m_ig[0].x; res->m_ig[1].x=pEntityHost->m_ig[1].x;
		res->m_ig[0].y=pEntityHost->m_ig[0].y; res->m_ig[1].y=pEntityHost->m_ig[1].y;
	} else {
		res->m_bPermanent = 1;
		res->m_pForeignData = pForeignData;
		res->m_iForeignData = iForeignData;
		m_lastExtId = max(m_lastExtId, id);
		m_nExtIds += 1+(id>>31);
		SetPhysicalEntityId(res, id>=0 ? id:GetFreeEntId());
	}
	res->m_flags |= 0x80000000u;
	if (params)
		res->SetParams(params, iForeignData==0x5AFE || get_iCaller()<MAX_PHYS_THREADS);

	if (!m_lockStep && lifeTime==0) {
		WriteLockCond lock1(m_lockCaller[MAX_PHYS_THREADS], !IsPODThread(this) && m_nEnts+1>m_nEntsAlloc-1);
		WriteLock lock(m_lockStep);
		res->m_flags &= ~0x80000000u;
		RepositionEntity(res,2);
		if (++m_nEnts > m_nEntsAlloc-1) {
			int nEntsAllocNew = m_nEntsAlloc+4096; 
			m_nEntListAllocs++; m_bEntityCountReserved = 0;
			ReallocateList(m_pTmpEntList,m_nEnts-1,nEntsAllocNew);
			ReallocateList(m_pTmpEntList1,m_nEnts-1,nEntsAllocNew);
			if (m_threadData[MAX_PHYS_THREADS].szList<nEntsAllocNew)
				ReallocateList(m_pTmpEntList2,m_threadData[MAX_PHYS_THREADS].szList,nEntsAllocNew);
			ReallocateList(m_pGroupMass,m_nEnts-1,nEntsAllocNew);
			ReallocateList(m_pMassList,m_nEnts-1,nEntsAllocNew);
			ReallocateList(m_pGroupIds,m_nEnts-1,nEntsAllocNew);
			ReallocateList(m_pGroupNums,m_nEnts-1,nEntsAllocNew);
			m_nEntsAlloc = nEntsAllocNew;
		}
	} else if (!m_lockQueue || get_iCaller()>=MAX_PHYS_THREADS && iForeignData!=0x5AFE) {
		WriteLock lock(m_lockQueue);
		AllocRequestsQueue(sizeof(int)*3+sizeof(void*));
		QueueData(4);	// RepositionEntity opcode
		QueueData((int)(sizeof(int)*3+sizeof(void*)));	// size
		QueueData(res);
		QueueData(2);	// flags
	} else {
		res->m_timeIdle = -10.0f;
		m_nOnDemandListFailures++;
	}

	return res;
}


IPhysicalEntity *CPhysicalWorld::CreatePhysicalPlaceholder(pe_type type, pe_params* params, void *pForeignData,int iForeignData, int id)
{
	int i,j,iChunk;
	if (m_nPlaceholders*10<m_iLastPlaceholder*7) {
		for(i=m_iLastPlaceholder>>5; i>=0 && m_pPlaceholderMap[i]==-1; i--);
		if (i>=0) {
			for(j=0;j<32 && m_pPlaceholderMap[i]&1<<j;j++);
			i = i<<5|j;
		}
		i = i-(i>>31) | m_iLastPlaceholder+1&i>>31;
	} else
		i = m_iLastPlaceholder+1;

	iChunk = i>>PLACEHOLDER_CHUNK_SZLG2;
	if (iChunk==m_nPlaceholderChunks) {
		m_nPlaceholderChunks++;
		ReallocateList(m_pPlaceholders, m_nPlaceholderChunks-1,m_nPlaceholderChunks,true);
		ReallocateList(m_pPlaceholderMap, (m_iLastPlaceholder>>5)+1,m_nPlaceholderChunks<<PLACEHOLDER_CHUNK_SZLG2-5,true);
	}
	if (!m_pPlaceholders[iChunk])
		m_pPlaceholders[iChunk] = new CPhysicalPlaceholder[PLACEHOLDER_CHUNK_SZ];
	CPhysicalPlaceholder *res = m_pPlaceholders[iChunk]+(i & PLACEHOLDER_CHUNK_SZ-1);
	
	res->m_pForeignData = pForeignData;
	res->m_iForeignData = iForeignData;
	res->m_iForeignFlags = 0;
	res->m_iGThunk0 = 0;
	res->m_ig[0].x=res->m_ig[0].y=res->m_ig[1].x=res->m_ig[1].y = -2;
	res->m_pEntBuddy = 0;
	res->m_id = -1;
	res->m_bProcessed = 0;
	switch (type) {
		case PE_STATIC: res->m_iSimClass = 0; break;
		case PE_RIGID: case PE_WHEELEDVEHICLE: res->m_iSimClass = 1; break;
		case PE_LIVING: res->m_iSimClass = 3; break;
		case PE_PARTICLE: case PE_ROPE: case PE_ARTICULATED: case PE_SOFT: res->m_iSimClass = 4;
	}
	m_pPlaceholderMap[i>>5] |= 1<<(i&31);

	if (id>-2)
		SetPhysicalEntityId(res, id>=0 ? id:GetFreeEntId());
	else 
		res->m_id = id;
	if (params)
		res->SetParams(params);
	m_nPlaceholders++;
	m_iLastPlaceholder = max(m_iLastPlaceholder,i);

	return res;
}


int CPhysicalWorld::DestroyPhysicalEntity(IPhysicalEntity* _pent,int mode,int bThreadSafe)
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );

	int idx;
	CPhysicalPlaceholder *ppc = (CPhysicalPlaceholder*)_pent;
	if (ppc->m_pEntBuddy && IsPlaceholder(ppc->m_pEntBuddy) && mode!=0 || m_nDynamicEntitiesDeleted && ppc->m_iSimClass>0)
		return 0;
	if (!(idx=IsPlaceholder(ppc)))
		if (ppc->m_iSimClass!=5) {
			if (mode & 4 && ((CPhysicalEntity*)ppc)->Release()>0)
				return 0;
			((CPhysicalEntity*)ppc)->m_iDeletionTime = 1;
			if (mode==0) {
				((CPhysicalEntity*)ppc)->m_pForeignData = 0;
				((CPhysicalEntity*)ppc)->m_iForeignData = -1;
			}
		} else
			((CPhysArea*)ppc)->m_bDeleted = 1;
	mode &= 3;

	//if (m_lockStep & (bThreadSafe^1))
	if (m_vars.bMultithreaded & (bThreadSafe^1) && (m_lockStep || m_lockTPR || m_vars.lastTimeStep>0)) {
		WriteLock lock(m_lockQueue);
		AtomicAdd(&ppc->m_bProcessed, PENT_QUEUED);
		AllocRequestsQueue(sizeof(int)*3+sizeof(void*));
		QueueData(5);	// DestroyPhysicalEntity opcode
		QueueData((int)(sizeof(int)*3+sizeof(void*)));	// size
		QueueData(_pent);
		QueueData(mode); 
		return 1;
	}
	WriteLockCond lock(m_lockStep, m_vars.bMultithreaded && !bThreadSafe && !IsPODThread(this));

	if (ppc->m_iSimClass==5) {
		if (mode==0)
			RemoveArea(_pent); 
		return 1;
	}
	
	if (idx) {
		if (mode!=0)
			return 0;
		if (ppc->m_pEntBuddy && ppc->m_pEntBuddy->m_pEntBuddy==ppc)
			DestroyPhysicalEntity(ppc->m_pEntBuddy,mode,1);
		SetPhysicalEntityId(ppc,-1);
		{ WriteLock lockGrid(m_lockGrid); 
			DetachEntityGridThunks(ppc);
		}
		--idx;
		m_pPlaceholderMap[idx>>5] &= ~(1<<(idx&31));
		m_nPlaceholders--;

		int i,j,iChunk = idx>>PLACEHOLDER_CHUNK_SZLG2;
		// if entire iChunk is empty, deallocate it
		for(i=j=0;i<PLACEHOLDER_CHUNK_SZ>>5;i++) j |= m_pPlaceholderMap[(iChunk<<PLACEHOLDER_CHUNK_SZLG2-5)+i];
		if (!j) {
			delete[] m_pPlaceholders[iChunk]; m_pPlaceholders[iChunk] = 0;
		}
		j = m_nPlaceholderChunks;
		// make sure that m_iLastPlaceholder points to the last used placeholder slot
		for(; m_iLastPlaceholder>=0 && !(m_pPlaceholderMap[m_iLastPlaceholder>>5] & 1<<(m_iLastPlaceholder&31)); m_iLastPlaceholder--)
		if ((m_iLastPlaceholder^m_iLastPlaceholder-1)+1>>1 == PLACEHOLDER_CHUNK_SZ) {	
			// if m_iLastPlaceholder points to the 1st chunk element, entire chunk is free and can be deallocated
			iChunk = m_iLastPlaceholder>>PLACEHOLDER_CHUNK_SZLG2;
			if (m_pPlaceholders[iChunk]) {
				delete[] m_pPlaceholders[iChunk]; m_pPlaceholders[iChunk] = 0;
			}
			m_nPlaceholderChunks = iChunk;
		}
		if (m_nPlaceholderChunks<j)
			ReallocateList(m_pPlaceholderMap,j<<PLACEHOLDER_CHUNK_SZLG2-5,m_nPlaceholderChunks<<PLACEHOLDER_CHUNK_SZLG2-5,true);

		return 1;
	}

	CPhysicalEntity *pent = (CPhysicalEntity*)_pent;
	if (pent->m_iSimClass==7)
		return 0;
	pent->m_iDeletionTime = m_iLastLogPump+2;
	for(idx=m_nProfiledEnts-1;idx>=0;idx--) if (m_pEntProfileData[idx].pEntity==pent)
		memmove(m_pEntProfileData+idx, m_pEntProfileData+idx+1, (--m_nProfiledEnts-idx)*sizeof(m_pEntProfileData[0]));
	for(idx=0; idx<=MAX_PHYS_THREADS; idx++)
		m_prevGEAobjtypes[idx] = -1;

	if (mode!=1 && pent->m_iSimClass==-1) {
		if (pent->m_next) pent->m_next->m_prev = pent->m_prev;
		if (pent->m_prev) pent->m_prev->m_next = pent->m_next;
		if (pent==m_pHiddenEnts) m_pHiddenEnts = pent->m_next;
		pent->m_next = pent->m_prev = 0;
	}
	if (mode==2) {
		if (pent->m_iSimClass==-1 && pent->m_iPrevSimClass>=0) {
			pent->m_ig[0].x=pent->m_ig[1].x=pent->m_ig[0].y=pent->m_ig[1].y = -2;
			pent->m_iSimClass = pent->m_iPrevSimClass & 0x0F; pent->m_iPrevSimClass=-1;
			AtomicAdd(&m_lockGrid,-RepositionEntity(pent));
		}
		pent->m_iDeletionTime = 0;
		return 1;
	}

	if (m_pEntBeingDeleted==pent)
		return 1;
	m_pEntBeingDeleted = pent;
	if (mode==0 && !pent->m_bPermanent && m_pPhysicsStreamer)
		m_pPhysicsStreamer->DestroyPhysicalEntity(pent);
	m_pEntBeingDeleted = 0;

	pent->AlertNeighbourhoodND(mode);
	if ((unsigned int)pent->m_iPrevSimClass<8u && pent->m_iSimClass>=0) {
		if (pent->m_next) pent->m_next->m_prev = pent->m_prev;
		(pent->m_prev ? pent->m_prev->m_next : m_pTypedEnts[pent->m_iPrevSimClass]) = pent->m_next;
		if (pent==m_pTypedEntsPerm[pent->m_iPrevSimClass])
			m_pTypedEntsPerm[pent->m_iPrevSimClass] = pent->m_next;
	}
	pent->m_next=pent->m_prev = 0;
/*#ifdef _DEBUG
CPhysicalEntity *ptmp = m_pTypedEnts[1];
for(;ptmp && ptmp!=m_pTypedEntsPerm[1]; ptmp=ptmp->m_next);
if (ptmp!=m_pTypedEntsPerm[1])
DEBUG_BREAK;
#endif*/

	if (!pent->m_pEntBuddy)	{
		{ WriteLock lockGrid(m_lockGrid); 
			DetachEntityGridThunks(pent);
		}
		for(int j=0;j<pent->m_nParts;j++) if (pent->m_parts[j].pPlaceholder)
			DestroyPhysicalEntity(pent->ReleasePartPlaceholder(j),0,1);
		(pent->m_flags &= ~pef_parts_traceable) |= pef_traceable;
		pent->m_nUsedParts = 0;
	}
	pent->m_iGThunk0 = 0;

	if (mode==0) {
		int bWasRegistered = !(pent->m_flags & 0x80000000u);
		pent->m_iPrevSimClass = -1; pent->m_iSimClass = 7;
		pent->m_pForeignData = 0;
		pent->m_iForeignData = -1;
		if (pent->m_next = m_pTypedEnts[7]) 
			pent->m_next->m_prev=pent;
		if (pent->m_pEntBuddy)
			pent->m_pEntBuddy->m_pEntBuddy = 0;
		else {
			if (pent->m_id<=m_lastExtId)
				--m_nExtIds;
			SetPhysicalEntityId(pent,-1);
		}
		m_pTypedEnts[7] = pent;
		if (bWasRegistered)
			m_nTypeEnts[pent->GetType()]--;
		if (bWasRegistered && --m_nEnts < m_nEntsAlloc-8192 && !m_bEntityCountReserved) {
			m_nEntsAlloc -= 8192; m_nEntListAllocs++;
			ReallocateList(m_pTmpEntList,m_nEntsAlloc+8192,m_nEntsAlloc);
			ReallocateList(m_pTmpEntList1,m_nEntsAlloc+8192,m_nEntsAlloc);
			ReallocateList(m_pTmpEntList2,m_nEntsAlloc+8192,m_nEntsAlloc);
			ReallocateList(m_pGroupMass,0,m_nEntsAlloc);
			ReallocateList(m_pMassList,0,m_nEntsAlloc);
			ReallocateList(m_pGroupIds,0,m_nEntsAlloc);
			ReallocateList(m_pGroupNums,0,m_nEntsAlloc);
			m_threadData[0].szList=m_threadData[MAX_PHYS_THREADS].szList = m_nEntsAlloc;
		}
	} else if (pent->m_iSimClass>=0) {
		pe_action_reset reset;
		pent->Action(&reset);
		pent->m_iPrevSimClass = pent->m_iSimClass | 0x100;
		pent->m_iSimClass = -1;
		if (m_pHiddenEnts) m_pHiddenEnts->m_prev = pent;
		pent->m_next = m_pHiddenEnts; m_pHiddenEnts = pent;
		pent->m_prev = 0;
	}
	
	return 1;
}


int CPhysicalWorld::ReserveEntityCount(int nNewEnts)
{
	if (m_nEnts+nNewEnts > m_nEntsAlloc-1) {
		m_nEntsAlloc = (m_nEnts+nNewEnts & ~4095) + 4096;
		m_nEntListAllocs++; m_bEntityCountReserved = 1;
		ReallocateList(m_pTmpEntList,m_nEnts-1,m_nEntsAlloc);
		ReallocateList(m_pTmpEntList1,m_nEnts-1,m_nEntsAlloc);
		ReallocateList(m_pTmpEntList2,m_nEnts-1,m_nEntsAlloc);
		ReallocateList(m_pGroupMass,m_nEnts-1,m_nEntsAlloc);
		ReallocateList(m_pMassList,m_nEnts-1,m_nEntsAlloc);
		ReallocateList(m_pGroupIds,m_nEnts-1,m_nEntsAlloc);
		ReallocateList(m_pGroupNums,m_nEnts-1,m_nEntsAlloc);
	}
	return m_nEntsAlloc;
}


void CPhysicalWorld::CleanseEventsQueue()
{
	WriteLock lock(m_lockEventsQueue);
	EventPhys *pEvent,**ppPrevNext;

	for(pEvent=m_pEventFirst,ppPrevNext=&m_pEventFirst,m_pEventLast=0; pEvent; pEvent=*ppPrevNext) {
		if (pEvent->idval<=EventPhysCollision::id && 
				(((CPhysicalEntity*)((EventPhysStereo*)pEvent)->pEntity[0])->m_iDeletionTime || 
					((CPhysicalEntity*)((EventPhysStereo*)pEvent)->pEntity[1])->m_iDeletionTime) ||
				pEvent->idval>EventPhysCollision::id && ((CPhysicalEntity*)((EventPhysMono*)pEvent)->pEntity)->m_iDeletionTime)
		{
			*ppPrevNext = pEvent->next;
			pEvent->next = m_pFreeEvents[pEvent->idval]; m_pFreeEvents[pEvent->idval] = pEvent;
		} else {
			ppPrevNext = &pEvent->next;
			m_pEventLast = pEvent;
		}
	}
}


int CPhysicalWorld::GetFreeEntId()
{
	int nPhysEnts=m_nEnts-m_nExtIds, nPhysSlots=m_iNextId-m_lastExtId;
	if (nPhysEnts*2 > nPhysSlots)
		return m_iNextId++;
	int nTries;
	for(nTries=100; nTries>0 && m_iNextIdDown>m_lastExtId && m_pEntsById[m_iNextIdDown]; m_iNextIdDown--,nTries--);
	if (nTries<=0)
		return m_iNextId++;
	if (m_iNextIdDown<=m_lastExtId)
		for(m_iNextIdDown=m_iNextId-2,nTries=100; m_iNextIdDown>m_lastExtId && m_pEntsById[m_iNextIdDown]; m_iNextIdDown--);
	if (nTries<=0 || m_iNextIdDown<=m_lastExtId)
		return m_iNextId++;
	return m_iNextIdDown--;
}


int CPhysicalWorld::SetPhysicalEntityId(IPhysicalEntity *_pent, int id, int bReplace, int bThreadSafe)
{
	WriteLockCond lock(m_lockEntIdList,bThreadSafe^1);
	CPhysicalPlaceholder *pent = (CPhysicalPlaceholder*)_pent;
	unsigned int previd = (unsigned int)pent->m_id;
	if (previd<(unsigned int)m_nIdsAlloc) {
		m_pEntsById[previd] = 0;
		if (previd==m_iNextId-1)
			for(;m_iNextId>0 && m_pEntsById[m_iNextId-1]==0;m_iNextId--);
		if (previd==m_lastExtId)
			for(--m_lastExtId; m_lastExtId>0 && m_pEntsById[m_lastExtId]; m_lastExtId--);
	}
	m_iNextId = max(m_iNextId,id+1);

	if (id>=0) { 
		if (id>=m_nIdsAlloc) {
			int nAllocPrev = m_nIdsAlloc;
			ReallocateList(m_pEntsById, nAllocPrev,m_nIdsAlloc=(id&~32767)+32768, true);
		}
		if (m_pEntsById[id]) {
			if (bReplace)
				SetPhysicalEntityId(m_pEntsById[id],GetFreeEntId(),1,1);
			else 
				return 0;
		}
		if (IsPlaceholder(pent->m_pEntBuddy))
			pent = pent->m_pEntBuddy;
		(m_pEntsById[id] = pent)->m_id = id;
		if (pent->m_pEntBuddy)
			pent->m_pEntBuddy->m_id = id;
		return 1;
	}
	return 0;
}

int CPhysicalWorld::GetPhysicalEntityId(IPhysicalEntity *pent)
{
	return pent ? ((CPhysicalEntity*)pent)->m_id : -1;
}

IPhysicalEntity* CPhysicalWorld::GetPhysicalEntityById(int id)
{
	ReadLock lock(m_lockEntIdList);
	int bNoExpand = id>>30; id &= ~(1<<30);
	if ((unsigned int)id<(unsigned int)m_nIdsAlloc)
		return m_pEntsById[id] ? (!bNoExpand ? m_pEntsById[id]->GetEntity():m_pEntsById[id]->GetEntityFast()) : 0;
	else if (id==-1)
		return m_pHeightfield[0];
	else if (id==-2)
		return &g_StaticPhysicalEntity;
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////


static inline void swap(CPhysicalEntity **pentlist,float *pmass,int *pids, int i1,int i2) {	
	CPhysicalEntity *pent = pentlist[i1]; pentlist[i1] = pentlist[i2]; pentlist[i2] = pent;
	float m = pmass[i1]; pmass[i1] = pmass[i2]; pmass[i2] = m;
	if (pids) {
		int id = pids[i1]; pids[i1] = pids[i2]; pids[i2] = id;
	}
}
static void qsort(CPhysicalEntity **pentlist,float *pmass,int *pids, int ileft,int iright)
{
	if (ileft>=iright) return;
	int i,ilast; 
	float diff = 0.0f;
	swap(pentlist,pmass,pids, ileft,ileft+iright>>1);
	for(ilast=ileft,i=ileft+1; i<=iright; i++) {
		diff += fabs_tpl(pmass[i]-pmass[ileft]);
		if (pmass[i] > pmass[ileft])
			swap(pentlist,pmass,pids, ++ilast,i);
	}
	swap(pentlist,pmass,pids, ileft,ilast);

	if (diff>0) {
		qsort(pentlist,pmass,pids, ileft,ilast-1);
		qsort(pentlist,pmass,pids, ilast+1,iright);
	}
}

int CPhysicalWorld::ReallocTmpEntList(CPhysicalEntity **&pEntList, int iCaller, int szNew)
{
	assert(iCaller<=MAX_PHYS_THREADS);
	ReallocateList(m_threadData[iCaller].pTmpEntList, m_threadData[iCaller].szList, szNew);
	pEntList = m_threadData[iCaller].pTmpEntList;
	if (iCaller==0)
		m_pTmpEntList = m_threadData[iCaller].pTmpEntList;
	else if (iCaller==MAX_PHYS_THREADS)
		m_pTmpEntList2 = m_threadData[iCaller].pTmpEntList;
	return m_threadData[iCaller].szList = szNew;
}


inline bool AABB_overlap2d(const Vec2& min0,const Vec2& max0, const Vec2 &min1,const Vec2& max1) {
	return max(fabs_tpl(min0.x+max0.x-min1.x-max1.x) - (max0.x-min0.x)-(max1.x-min1.x),
						 fabs_tpl(min0.y+max0.y-min1.y-max1.y) - (max0.y-min0.y)-(max1.y-min1.y))<0;
}

int CPhysicalWorld::GetEntitiesAround(const Vec3 &ptmin,const Vec3 &ptmax, CPhysicalEntity **&pList, int objtypes, 
																			CPhysicalEntity *pPetitioner, int szListPrealloc, int iCaller)
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );
	INT_PTR mask = (INT_PTR)pPetitioner;
	mask = mask>>sizeof(mask)*8-1 ^ (mask-1)>>sizeof(mask)*8-1;
	PHYS_FUNC_PROFILER((const char*)((INT_PTR)"GetEntitiesAround(Physics)"&~mask | (INT_PTR)"GetEntitiesAround(External)"&mask));

	if (!m_pEntGrid || !m_pTmpEntList) return 0;
	//WriteLock lock(m_lockCaller[iCaller]);
	CPhysicalEntity **pTmpEntList;//, **pTmpEntLists[2] = { m_pTmpEntList,m_pTmpEntList2 };
	int i,igx[2],igy[2],ix,iy,nout=0,itype,/*bSortRequired=0,*/bContact,nGridEnts=0,nEntsChecked=0,bProcessed,bAreasOnly,szList;
	float zrange,gx[2],gy[2],gz[2];
	Vec3 bbox[2];
	int ithunk,ithunk_next; 
	pe_PODcell *pPODcell;
	EventPhysBBoxOverlap event;
	//pTmpEntList = pTmpEntLists[iCaller];
	szList = GetTmpEntList(pTmpEntList, iCaller);
	if ((szListPrealloc | (objtypes & ent_allocate_list))==0)
		pList = pTmpEntList;
	if (pPetitioner) {
		itype = 1<<pPetitioner->m_iSimClass & -iszero((int)pPetitioner->m_flags&pef_never_affect_triggers);
		event.pEntity[0]=pPetitioner; event.pForeignData[0]=pPetitioner->m_pForeignData; event.iForeignData[0]=pPetitioner->m_iForeignData;
	} else
		itype = 0;
	bAreasOnly = iszero(objtypes-ent_areas);
	m_nGEA[iCaller]++;
	if (1-iszero((ptmin-m_prevGEABBox[iCaller][0]).len2()+(ptmax-m_prevGEABBox[iCaller][1]).len2())+sqr(objtypes-m_prevGEAobjtypes[iCaller])+
			1-(iCaller-MAX_PHYS_THREADS>>31) == 0) 
	{
		pList = pTmpEntList; return m_nprevGEAEnts[iCaller];
	}


	bbox[0]=ptmin; bbox[1]=ptmax;
	for(i=0;i<2;i++) {
		gx[i] = (bbox[i][inc_mod3[m_iEntAxisz]]-m_entgrid.origin[inc_mod3[m_iEntAxisz]])*m_entgrid.stepr.x;
		igx[i] = max(-1,min(m_entgrid.size.x,float2int(gx[i]-0.5f)));
		gy[i] = (bbox[i][dec_mod3[m_iEntAxisz]]-m_entgrid.origin[dec_mod3[m_iEntAxisz]])*m_entgrid.stepr.y;
		igy[i] = max(-1,min(m_entgrid.size.y,float2int(gy[i]-0.5f)));
		gz[i] = (bbox[i][m_iEntAxisz]-m_entgrid.origin[inc_mod3[m_iEntAxisz]])*m_rzGran;
	}

	if ((igx[1]-igx[0]+1)*(igy[1]-igy[0]+1)>m_vars.nGEBMaxCells) {
		if (m_pLog)
			m_pLog->Log("GetEntitiesInBox: too many cells requested by %s (%d, (%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f))",
			pPetitioner && m_pRenderer ? 
			m_pRenderer->GetForeignName(pPetitioner->m_pForeignData,pPetitioner->m_iForeignData,pPetitioner->m_iForeignFlags):"Game",
			(igx[1]-igx[0]+1)*(igy[1]-igy[0]+1), bbox[0].x,bbox[0].y,bbox[0].z, bbox[1].x,bbox[1].y,bbox[1].z);
		if (m_vars.bBreakOnValidation) DoBreak
	}
	
	{ ReadLock lock0(m_lockGrid);
		for(ix=igx[0];ix<=igx[1];ix++) for(iy=igy[0];iy<=igy[1];iy++) {
			if ((objtypes & (ent_static|ent_no_ondemand_activation))==ent_static) {
				pPODcell = getPODcell(ix,iy);
				zrange = pPODcell->zlim[1]-pPODcell->zlim[0];
				if (fabs_tpl((ptmax[m_iEntAxisz]+ptmin[m_iEntAxisz]-pPODcell->zlim[1]-pPODcell->zlim[0])*zrange) < 
										(ptmax[m_iEntAxisz]-ptmin[m_iEntAxisz]+zrange)*zrange)
				{ CryInterlockedAdd(&m_lockGrid,-1);
					ReadLock lockPOD(m_lockPODGrid);	
					if (pPODcell->lifeTime<=0) { 
						CryInterlockedAdd(&m_lockPODGrid,-1);
						{ WriteLock lockPODw(m_lockPODGrid);	
							if (pPODcell->lifeTime<=0) { 
								MarkAsPODThread(this); 
								Vec3 center,size;
								GetPODGridCellBBox(ix,iy,center,size);
								m_nOnDemandListFailures=0; ++m_iLastPODUpdate;
								if (m_pPhysicsStreamer->CreatePhysicalEntitiesInBox(center-size,center+size)) {	
									pPODcell->lifeTime = m_nOnDemandListFailures ? 1E10f:8.0f;
									pPODcell->inextActive = m_iActivePODCell0;
									m_iActivePODCell0 = iy<<16|ix;
									szList = max(szList, GetTmpEntList(pTmpEntList, iCaller));
								}
							}
						} ReadLockCond lockPODr(m_lockPODGrid,1); lockPODr.SetActive(0); 
						m_nOnDemandListFailures=0;
					}	else
						pPODcell->lifeTime = max(8.0f,pPODcell->lifeTime);
					UnmarkAsPODThread(this);
					ReadLockCond relock(m_lockGrid,1); relock.SetActive(0);
				}
			}

			for(ithunk=m_pEntGrid[m_entgrid.getcell_safe(ix,iy)]; ithunk && ((objtypes>>m_gthunks[ithunk].iSimClass)&1)|bAreasOnly^1; 
				ithunk=ithunk_next,nGridEnts++) 
			{	ithunk_next = m_gthunks[ithunk].inext;
				if ((objtypes >> m_gthunks[ithunk].iSimClass)&1 &&
					  (!m_entgrid.inrange(ix,iy) ||
							AABB_overlap(Vec3(gx[0],gy[0],gz[0]), Vec3(gx[1],gy[1],gz[1]),
							Vec3(ix+m_gthunks[ithunk].BBox[0]*(1.0f/256), iy+m_gthunks[ithunk].BBox[1]*(1.0f/256), m_gthunks[ithunk].BBoxZ0),
							Vec3(ix+(m_gthunks[ithunk].BBox[2]+1)*(1.0f/256), iy+(m_gthunks[ithunk].BBox[3]+1)*(1.0f/256), m_gthunks[ithunk].BBoxZ1))) &&
						!(m_gthunks[ithunk].pent->m_bProcessed>>iCaller & 1)) 
				{
					CPhysicalPlaceholder *pGridEnt = m_gthunks[ithunk].pent;
					{ ReadLockPlatf0 lock1(pGridEnt->m_lockUpdate);
						bContact = AABB_overlap(bbox,pGridEnt->m_BBox);
					}

					if (bContact) {
						if (nout>=szList)
							szList = ReallocTmpEntList(pTmpEntList,iCaller,szList+1024);
						if ((unsigned int)(pGridEnt->m_iSimClass-5)>1u) {
							m_bGridThunksChanged = 0;
							CPhysicalEntity *pent = pGridEnt->GetEntity();
							if (m_bGridThunksChanged)
								ithunk_next = m_pEntGrid[m_entgrid.getcell_safe(ix,iy)];
							m_bGridThunksChanged = 0;
							if (!pGridEnt->m_pEntBuddy || pent->m_pEntBuddy==pGridEnt) {
								if (objtypes & ent_ignore_noncolliding) {
									for(i=0;i<pent->m_nParts && !(pent->m_parts[i].flags & geom_colltype_solid);i++);
									if (i==pent->m_nParts) continue;
								}
								pTmpEntList[nout] = pent;
								bProcessed = iszero(m_bUpdateOnlyFlagged & ((int)pent->m_flags^pef_update)) | iszero(pent->m_iSimClass);
								nout += bProcessed; AtomicAdd(&pGridEnt->m_bProcessed,bProcessed<<iCaller);
							} else if ((m_bUpdateOnlyFlagged & ((int)pent->m_flags^pef_update) & -pent->m_iSimClass>>31)==0) {
								bProcessed = pent->m_bProcessed>>iCaller & 1;
								AtomicAdd(&pent->m_lockUpdate, bProcessed^1);
								volatile char *pw=(volatile char*)&pent->m_lockUpdate+(1+eBigEndian); for(;*pw;); // ReadLock(m_lockUpdate)
								AtomicAdd(&pent->m_bProcessed, (bProcessed^1)<<iCaller);
								AtomicAdd(&pent->m_nUsedParts, (pent->m_nUsedParts & (15<<iCaller*4))*(bProcessed-1));
								int nUsedParts = pent->m_nUsedParts>>iCaller*4 & 15;
								int notFull = nUsedParts+1>>4 ^ 1;
								notFull &= 1-iszero((INT_PTR)pGridEnt->m_pEntBuddy);
								nUsedParts += notFull;
								AtomicAdd(&pent->m_nUsedParts, notFull<<iCaller*4);
								pent->m_pUsedParts[iCaller][nUsedParts-1] = -2-pGridEnt->m_id;
								if (!bProcessed)
									pTmpEntList[nout++] = pent;
								AtomicAdd(&pGridEnt->m_bProcessed, 1<<iCaller);
							}
							//bSortRequired += pent->m_pOuterEntity!=0;
						} else if (pGridEnt->m_iSimClass==5) {
							if (!((CPhysArea*)pGridEnt)->m_bDeleted) {
								pTmpEntList[nout++] = (CPhysicalEntity*)pGridEnt;
								AtomicAdd(&pGridEnt->m_bProcessed, 1<<iCaller);
							}
						} else if (pGridEnt->m_iForeignFlags & itype) {
							event.pEntity[1]=pGridEnt; event.pForeignData[1]=pGridEnt->m_pForeignData; event.iForeignData[1]=pGridEnt->m_iForeignData;
							OnEvent(pPetitioner->m_flags, &event);
							//m_pEventClient->OnBBoxOverlap(pGridEnt,pGridEnt->m_pForeignData,pGridEnt->m_iForeignData,
							//	pPetitioner,pPetitioner->m_pForeignData,pPetitioner->m_iForeignData);
						}
					}
					nEntsChecked++;
				}
			}
		}
		//listfull:;
	}
	for(i=0;i<nout;i++)	{
		AtomicAdd(&(pTmpEntList[i]->m_pEntBuddy ? pTmpEntList[i]->m_pEntBuddy:pTmpEntList[i])->m_bProcessed,-(1<<iCaller));
		int j,nUsedParts = pTmpEntList[i]->m_nUsedParts>>iCaller*4 & 15;
		nUsedParts &= ~-iszero(pTmpEntList[i]->m_iSimClass-5);
		if (nUsedParts==15)	for(j=0;j<pTmpEntList[i]->m_nParts;j++) 
			AtomicAdd(&pTmpEntList[i]->m_parts[j].pPlaceholder->m_bProcessed,-(int)(pTmpEntList[i]->m_parts[j].pPlaceholder->m_bProcessed & 1<<iCaller));
		else for(j=0;j<nUsedParts;j++)
			AtomicAdd(&pTmpEntList[i]->m_parts[pTmpEntList[i]->m_pUsedParts[iCaller][j]].pPlaceholder->m_bProcessed,-(1<<iCaller));
		AtomicAdd(&pTmpEntList[i]->m_lockUpdate, -nUsedParts>>31);
	}

	/*if (bSortRequired) {
		CPhysicalEntity *pent,*pents,*pstart;
		for(i=0;i<nout;i++) pTmpEntList[i]->m_bProcessed_aux = 1;
		for(i=0,pent=0;i<nout-1;i++) {
			pTmpEntList[i]->m_prev_aux = pent;
			pTmpEntList[i]->m_next_aux = pTmpEntList[i+1];
			pent = pTmpEntList[i];
		}
		pstart = pTmpEntList[0];
		pTmpEntList[nout-1]->m_prev_aux = pent;
		pTmpEntList[nout-1]->m_next_aux = 0;
		for(i=0;i<nout;i++) {
			if ((pent=pTmpEntList[i])->m_pOuterEntity && pent->m_pOuterEntity->m_bProcessed_aux>0) {
				// if entity has an outer entity, move it together with its children right before this outer entity
				for(pents=pent,j=pent->m_bProcessed_aux-1; j>0; pents=pents->m_prev_aux);	// count back the number of pent children
				(pents->m_prev_aux ? pent->m_prev_aux->m_next_aux : pstart) = pent->m_next_aux;	// cut pents-pent stripe from list ...
				if (pent->m_next_aux) pent->m_next_aux->m_prev_aux = pents->m_prev_aux;
				pent->m_next_aux = pent->m_pOuterEntity; // ... and insert if before pent
				pents->m_prev_aux = pent->m_pOuterEntity->m_prev_aux;
				(pent->m_pOuterEntity->m_prev_aux ? pent->m_pOuterEntity->m_prev_aux->m_next_aux : pstart) = pents;
				pent->m_pOuterEntity->m_prev_aux = pent;
				pent->m_pOuterEntity->m_bProcessed_aux += pent->m_bProcessed_aux;
			}
		}
		Vec3 ptc = (ptmin+ptmax)*0.5f;
		for(i=0;i<nout;i++) pTmpEntList[i]->m_bProcessed_aux = 0;
		for(pent=pstart,nout=0; pent; pent=pent->m_next_aux) if (!pent->m_bProcessed_aux) {
			pTmpEntList[nout] = pent;
			if (pent->m_pOuterEntity && pent->IsPointInside(ptc))
				for(pent=pent->m_pOuterEntity; pent; pent=pent->m_pOuterEntity) pent->m_bProcessed_aux=-1;
			pent = pTmpEntList[nout++];
		}
	}*/

	if (m_pHeightfield[iCaller] && objtypes & ent_terrain) {
		if (nout>=szList)
			szList = ReallocTmpEntList(pTmpEntList,iCaller,szList+1024);
		pTmpEntList[nout++] = m_pHeightfield[iCaller];
	}

	if (objtypes & ent_sort_by_mass) {
		for(i=0;i<nout;i++) m_pMassList[i] = pTmpEntList[i]->GetMassInv();
		// manually put all static (0-massinv) object to the end of the list, since qsort doesn't
		// perform very well on lists of same numbers
		int ilast;
		for(i=ilast=nout-1; i>0; i--) if (m_pMassList[i]==0) {
			if (i!=ilast) 
				swap(pTmpEntList,m_pMassList,0, i,ilast);
			--ilast;
		}
		qsort(pTmpEntList,m_pMassList,0, 0,ilast);
	}

	if (objtypes&1<<5) {
		ReadLock lock1(m_lockAreas);
		if (m_pGlobalArea) for(CPhysArea* pArea=m_pGlobalArea->m_nextBig; pArea; pArea=pArea->m_nextBig) 
			if (!pArea->m_bDeleted && AABB_overlap(bbox,pArea->m_BBox) && nout<szList)
				pTmpEntList[nout++] = (CPhysicalEntity*)pArea;
	}
	
	if (szListPrealloc<nout) {
		if (!(objtypes & ent_allocate_list))
			pList = pTmpEntList;
		else if (nout>0) {	//  don't allocate 0-elements arrays
			pList = new CPhysicalEntity*[nout];	
			for(i=0;i<nout;i++) pList[i] = pTmpEntList[i];
		}	
	}	else
		for(i=0;i<nout;i++) pList[i] = pTmpEntList[i];

	m_prevGEABBox[iCaller][0]=ptmin; m_prevGEABBox[iCaller][1]=ptmax;
	m_prevGEAobjtypes[iCaller]=objtypes; m_nprevGEAEnts[iCaller]=nout;
	return nout;
}


void CPhysicalWorld::ScheduleForStep(CPhysicalEntity *pent, float time_interval)
{
	WriteLock lock(m_lockAuxStepEnt);
	if (!(pent->m_flags & pef_step_requested)) {
		pent->m_flags |= pef_step_requested;
		pent->m_next_coll2 = m_pAuxStepEnt;
		pent->m_timeIdle = time_interval;
		m_pAuxStepEnt = pent;
	}	else
		pent->m_timeIdle = min(pent->m_timeIdle, time_interval);
}


void CPhysicalWorld::UpdateDeformingEntities(float time_interval)
{
	WriteLockPlatf1 lockp0(m_lockPotentialPartListUpdate),lockp1(m_lockPotentialMeshUpdate);
	WriteLock lock3(m_lockDeformingEntsList);
	int i,j;
	int iCaller = get_iCaller_int();
	if (time_interval>=0) {
		for(i=j=0; i<m_nDeformingEnts; i++) if (m_pDeformingEnts[i]->m_iSimClass!=7 && m_pDeformingEnts[i]->UpdateStructure(time_interval,0,iCaller))
			m_pDeformingEnts[j++] = m_pDeformingEnts[i];
		else 
			m_pDeformingEnts[i]->m_flags &= ~pef_deforming;
		m_nDeformingEnts = j;
	} else {
		for(i=0;i<m_nDeformingEnts;i++)
			m_pDeformingEnts[i]->m_flags &= ~pef_deforming;
		m_nDeformingEnts = 0;
	}
}


void SPhysTask::OnUpdate() { m_pWorld->ThreadProc(m_idx,this); }
void SPhysTask::Stop() { bStop=1; m_pWorld->m_threadStart[m_idx].Set(); }
int __cursubstep=10;

void CPhysicalWorld::ProcessIslandSolverResults(int i, int iter, float groupTimeStep,float Ebefore, int nEnts,float fixedDamping, int &bAllGroupsFinished,
																								entity_contact **pContacts,int nContacts,int nBodies, int iCaller,int iticks0)
{
	int i1,j,bGroupFinished,nBrokenParts,nParts0,idCurGroup=m_pGroupIds[i];
	float Eafter,damping;
	CPhysicalEntity *pent;
	iter += m_rq.iter-iter | iter>>31;

	for(j=0;j<nContacts;j++) if ((pContacts[j]->ipart[0]|pContacts[j]->ipart[1])>=0) {
		if (pContacts[j]->pent[0]->m_parts[pContacts[j]->ipart[0]].flags & geom_monitor_contacts)
			pContacts[j]->pent[0]->OnContactResolved(pContacts[j],0,idCurGroup);
		if (pContacts[j]->pent[1]->m_parts[pContacts[j]->ipart[1]].flags & geom_monitor_contacts)
			pContacts[j]->pent[1]->OnContactResolved(pContacts[j],1,idCurGroup);
	}

	#ifdef ENTITY_PROFILER_ENABLED
	i1 = CryGetTicks()-iticks0;
	if (m_vars.bProfileEntities) {
		for(pent=m_pTmpEntList1[i],j=0; pent; pent=pent->m_next_coll)
			j += -pent->m_iSimClass>>31 & 1;
		i1 /= max(1,j);
		for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll) if (pent->m_iSimClass>0)
			AddEntityProfileInfo(pent,i1);
	}
	#endif

	damping = 1.0f-groupTimeStep*m_vars.groupDamping*isneg(m_vars.nGroupDamping-1-nEnts);
	for(pent=m_pTmpEntList1[i],bGroupFinished=1,Eafter=0.0f; pent; pent=pent->m_next_coll) {
		Eafter += pent->CalcEnergy(0);	
		if (!(pent->m_flags & pef_fixed_damping))
			damping = min(damping,pent->GetDamping(groupTimeStep));
		else {
			damping = pent->GetDamping(groupTimeStep);
			break;
		}
	}
	//Ebefore *= isneg(-nAnimatedObjects)+1; // increase energy growth limit if we have animated bodies involved
	if (Eafter>Ebefore*(1.0f+0.1f*isneg(nBodies-15)))
		damping = min(damping, sqrt_tpl(Ebefore/Eafter));
	if (fixedDamping>-0.5f)
		damping = fixedDamping;
	for(pent=m_pTmpEntList1[i],bGroupFinished=1; pent; pent=pent->m_next_coll)
		bGroupFinished &= pent->Update(groupTimeStep, damping);
	bGroupFinished |= isneg(m_vars.nMaxSubstepsLargeGroup-iter-2 & m_vars.nBodiesLargeGroup-nBodies-1);
	bGroupFinished |= isneg(m_vars.nMaxSubsteps-iter-2);
	if (!bGroupFinished) for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll)
		pent->m_bMoved = 0;
	else {
		WriteLock lock1(m_lockMovedEntsList);
		for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll) {
			pent->m_bMoved = 3;
			if (pent->m_iSimClass<3 && !pent->m_next_coll2) {
				pent->m_next_coll2=(CPhysicalEntity*)m_pMovedEnts; m_pMovedEnts=pent;
			}
		}
	}
	bAllGroupsFinished &= bGroupFinished;

	// process deforming (breaking) enities of this group
	{ WriteLockPlatf1 lockp0(m_lockPotentialPartListUpdate),lockp1(m_lockPotentialMeshUpdate);
		WriteLock lock3(m_lockDeformingEntsList);
		for(i1=j=nBrokenParts=0; i1<m_nDeformingEnts; i1++) if (m_pDeformingEnts[i1]->m_iGroup==idCurGroup) {
			if ((nParts0=m_pDeformingEnts[i1]->m_nParts) && m_pDeformingEnts[i1]->m_iSimClass!=7) {
				if (m_pDeformingEnts[i1]->UpdateStructure(max(groupTimeStep,0.01f),0,iCaller))
					m_pDeformingEnts[j++] = m_pDeformingEnts[i1];
				else
					m_pDeformingEnts[i1]->m_flags &= ~pef_deforming;
				nBrokenParts += -iszero((int)m_pDeformingEnts[i1]->m_flags & aef_recorded_physics) & nParts0-m_pDeformingEnts[i1]->m_nParts;
			} else 
				m_pDeformingEnts[i1]->m_flags &= ~pef_deforming;
			m_pDeformingEnts[i1]->m_iGroup = -1;
		}	else
			m_pDeformingEnts[j++] = m_pDeformingEnts[i1];
		m_nDeformingEnts = j;
	}
	// if some entities broke, step back the velocities, but don't re-execute the step immediately
	if (nBrokenParts) for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll)
		pent->StepBack(0);
}

int CPhysicalWorld::ReadDelayedSolverResults(CMemStream &stm, float &dt,float &Ebefore,int &nEnts,float &fixedDamping, 
																						 entity_contact **pContacts,RigidBody **pBodies)
{
	int iCaller = get_iCaller_int();
	int iGroup,nContacts,nBodies;
	stm.Read(iGroup); stm.Read(dt); stm.Read(Ebefore); stm.Read(nEnts);	stm.Read(fixedDamping); 
	stm.Read(m_threadData[iCaller].bGroupInvisible);
	stm.Read(nContacts); stm.ReadRaw(pContacts, sizeof(void*)*nContacts);
	stm.Read(nBodies); stm.ReadRaw(pBodies, sizeof(void*)*nBodies);
	return iGroup;
}

void CPhysicalWorld::ProcessNextEntityIsland(float time_interval, int ipass, int iter, int &bAllGroupsFinished, int iCaller)
{
	int i,j,n,i1,nEnts,nAnimatedObjects,nBodies,bStepValid,bGroupInvisible;
	float Ebefore,groupTimeStep,fixedDamping;
	CPhysicalEntity *pent,*pent_next,*phead,**pentlist;

	do {
		{ WriteLock lock(m_lockNextEntityGroup);
			if (m_iCurGroup>=m_nGroups)
				break;
			i = m_iCurGroup++;
		}
		m_threadData[iCaller].groupMass=m_curGroupMass = m_pGroupMass[i]-m_maxGroupMass*isneg(m_maxGroupMass-m_pGroupMass[i]);
		m_threadData[iCaller].bGroupInvisible=m_bCurGroupInvisible = 0;
		groupTimeStep = time_interval*(ipass^1); nAnimatedObjects = 0; fixedDamping = -1.0f;
		Ebefore = 0.0f; 
		for(phead=m_pTmpEntList1[i],bGroupInvisible=pef_invisible; phead; phead=phead->m_next_coll)	{
			ReadLock lockcol(phead->m_lockColliders);
			bGroupInvisible &= phead->m_flags;
			for(j=0,n=phead->GetColliders(pentlist); j<n; j++) if (pentlist[j]->m_iSimClass>1 && pentlist[j]->GetMassInv()<=0) {
				if (ipass) {
					if (pentlist[j]->m_flags & pef_fixed_damping)
						fixedDamping = max(fixedDamping, pentlist[j]->GetDamping(pentlist[j]->GetMaxTimeStep(time_interval)));
					groupTimeStep = max(groupTimeStep, pentlist[j]->GetLastTimeStep(time_interval));
					RigidBody *pbody = pentlist[j]->GetRigidBody();
					Vec3 sz = pentlist[j]->m_BBox[1]-pentlist[j]->m_BBox[0];
					Ebefore += m_curGroupMass*(pbody->v.len2()+pbody->w.len2()*sqr(max(max(sz.x,sz.y),sz.z)));
				} else
					groupTimeStep = min(groupTimeStep, pentlist[j]->GetMaxTimeStep(time_interval));
				nAnimatedObjects++;
				bGroupInvisible &= pentlist[j]->m_flags | ~-iszero(pentlist[j]->m_iSimClass-2);
			}
		}
		m_threadData[iCaller].bGroupInvisible=m_bCurGroupInvisible = -(-bGroupInvisible>>31);

		if (ipass==0) {
			ReadLockPlatf1 lockp1(m_lockPotentialMeshUpdate);
			for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll)
				groupTimeStep = min(groupTimeStep, pent->GetMaxTimeStep(time_interval));
			for(pent=m_pTmpEntList1[i],bStepValid=1,phead=0; pent; pent=pent_next) {
				pent_next=pent->m_next_coll;
				if (pent->m_iSimClass<3)
					bStepValid &= (phead=pent)->Step(groupTimeStep); 
				pent->m_bMoved = 1;
			}
			if (!bStepValid) {
				for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll)
					pent->StepBack(groupTimeStep);
			}
			for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll) pent->m_bMoved = 2;
		} else if (time_interval>0) {
			for(pent=m_pTmpEntList1[i],groupTimeStep=0; pent; pent=pent->m_next_coll) if (pent->m_iSimClass>1)
				groupTimeStep = max(groupTimeStep, pent->GetLastTimeStep(time_interval));
			if (groupTimeStep==0)
				groupTimeStep = time_interval;
			InitContactSolver(groupTimeStep);

			if (m_vars.nMaxPlaneContactsDistress!=m_vars.nMaxPlaneContacts) {
				for(pent=m_pTmpEntList1[i],j=nEnts=0; pent; pent=pent->m_next_coll,nEnts++)	{
					j += pent->GetContactCount(m_vars.nMaxPlaneContacts);
					Ebefore += pent->CalcEnergy(groupTimeStep);
				}
				n = j>m_vars.nMaxContacts ? m_vars.nMaxPlaneContactsDistress : m_vars.nMaxPlaneContacts;
				for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll)
					pent->RegisterContacts(groupTimeStep,n);
			} else for(pent=m_pTmpEntList1[i],nEnts=0; pent; pent=pent->m_next_coll,nEnts++) {
				pent->RegisterContacts(groupTimeStep, m_vars.nMaxPlaneContacts);
				Ebefore += pent->CalcEnergy(groupTimeStep);
			}

			Ebefore = max(m_pGroupMass[i]*sqr(0.005f),Ebefore);

			#ifdef ENTITY_PROFILER_ENABLED
			i1 = CryGetTicks();
			#endif

			entity_contact **pContacts;
			RigidBody **pBodies;
			int nContacts=0;
			nBodies = InvokeContactSolver(groupTimeStep, &m_vars, Ebefore, pContacts,nContacts,pBodies);

#ifdef USE_PHYS_JOBS
			if (nBodies<0) {
        // prevents gcc from incorrecly assuming the presence of a sign extension when compiled with -O3
        const unsigned int uintBodies = (unsigned int) (nBodies*-1);
				CMemStream &stm = GetJobAuxStream();
				stm.Write(i); stm.Write(groupTimeStep); stm.Write(Ebefore); stm.Write(nEnts);	stm.Write(fixedDamping); 
				stm.Write(m_threadData[iCaller].bGroupInvisible);
				stm.Write(nContacts); stm.Write(pContacts, sizeof(void*)*nContacts);
				stm.Write(uintBodies); stm.Write(pBodies, sizeof(void*)*uintBodies);
				continue;
			}
#endif

			ProcessIslandSolverResults(i,iter,groupTimeStep,Ebefore,nEnts,fixedDamping,bAllGroupsFinished,pContacts,nContacts,nBodies,iCaller,i1);
		}
	} while(true);
}

void CPhysicalWorld::ProcessNextEngagedIndependentEntity(int iCaller)
{
	CPhysicalEntity *pent,*pentEnd,*pentNext;
	do {
		{ WriteLock lock(m_lockNextEntityGroup);
			if (!m_pCurEnt)
				break;
			pent=pentEnd=(CPhysicalEntity*)m_pCurEnt; m_pCurEnt=m_pCurEnt->m_next_coll2;
			if (pent->m_pOuterEntity || pent->m_next_coll2 && pent->m_next_coll2->m_pOuterEntity==pent) {
				if (!pent->m_pOuterEntity)
					pentEnd = pent->m_next_coll2;
				for(; pentEnd->m_next_coll2 && pentEnd->m_next_coll2->m_pOuterEntity==pentEnd->m_pOuterEntity; pentEnd=pentEnd->m_next_coll2);
				m_pCurEnt = pentEnd->m_next_coll2;
			}
		}
		do {
			pentNext = pent->m_next_coll2; pent->m_next_coll2 = 0;
			m_threadData[iCaller].bGroupInvisible = isneg(-((int)pent->m_flags & pef_invisible));
			pent->m_flags &= ~pef_step_requested;
			pent->Step(pent->m_timeIdle);
			pent->m_bMoved = 0;	
			if (pent==pentEnd)
				break;
		} while(pent=pentNext);
	} while(true);
}

void CPhysicalWorld::ProcessNextLivingEntity(float time_interval, int bSkipFlagged, int iCaller)
{
	CPhysicalEntity *pent,*pentEnd;
	Vec3 BBox[2],BBoxNew[2],velAbs;
	do {
		{ WriteLock lock(m_lockNextEntityGroup);
			if (!m_pCurEnt)
				break;
			pent=pentEnd = (CPhysicalEntity*)m_pCurEnt; 
			velAbs =((CLivingEntity*)pent)->m_vel.abs();
			BBox[0] = pent->m_BBox[0]-velAbs; BBox[1] = pent->m_BBox[1]+velAbs;
			while(pentEnd->m_next) {
				velAbs = ((CLivingEntity*)pentEnd->m_next)->m_vel.abs();
				BBoxNew[0] = pentEnd->m_next->m_BBox[0]-velAbs; BBoxNew[1] = pentEnd->m_next->m_BBox[1]+velAbs;
				if (AABB_overlap(BBox,BBoxNew)) {
					BBox[0] = min(BBox[0], BBoxNew[0]);
					BBox[1] = max(BBox[1], BBoxNew[1]);
					pentEnd = pentEnd->m_next;
				}	else
					break;
			}
			m_pCurEnt = pentEnd->m_next;
		}
		if (m_nWorkerThreads>0) {
			assert(m_nWorkerThreads+FIRST_WORKER_THREAD<=MAX_PHYS_THREADS);
			int i;
			do {
				do { 
					ReadLock lockr(m_lockPlayerGroups);
					for(i=0; i<m_nWorkerThreads+FIRST_WORKER_THREAD && (i==iCaller || !AABB_overlap(m_BBoxPlayerGroup[i],BBox)); i++);
					if (i>=m_nWorkerThreads+FIRST_WORKER_THREAD)
						break;
				} while(true);
				{ WriteLock lockw(m_lockPlayerGroups);
					for(i=0; i<m_nWorkerThreads+FIRST_WORKER_THREAD && (i==iCaller || !AABB_overlap(m_BBoxPlayerGroup[i],BBox)); i++);
					if (i>=m_nWorkerThreads+FIRST_WORKER_THREAD) {
						m_BBoxPlayerGroup[iCaller][0] = BBox[0];
						m_BBoxPlayerGroup[iCaller][1] = BBox[1];
						break;
					}
				}
			} while(true);
		}
		do {
			if (!(m_bUpdateOnlyFlagged&(pent->m_flags^pef_update) | bSkipFlagged&pent->m_flags))
				pent->Step(pent->GetMaxTimeStep(time_interval*m_vars.timeScalePlayers));
			if (pent==pentEnd)
				break;
		}	while (pent=pent->m_next);
		if (m_nWorkerThreads>0) { 
			WriteLock lock(m_lockPlayerGroups);
			m_BBoxPlayerGroup[iCaller][0]=m_BBoxPlayerGroup[iCaller][1] = Vec3(1e10f);
		}
	} while(true);
}

void CPhysicalWorld::ProcessNextIndependentEntity(float time_interval, int bSkipFlagged, int iCaller)
{
	CPhysicalEntity *pent,*pentEnd;
	int iter;
	do {
		{ WriteLock lock(m_lockNextEntityGroup);
			if (!m_pCurEnt)
				break;
			pent=pentEnd=(CPhysicalEntity*)m_pCurEnt; m_pCurEnt=m_pCurEnt->m_next;
			if (pent->m_pOuterEntity || pent->m_next && pent->m_next->m_pOuterEntity==pent) {
				if (!pent->m_pOuterEntity)
					pentEnd = pent->m_next;
				for(; pentEnd->m_next && pentEnd->m_next->m_pOuterEntity==pentEnd->m_pOuterEntity; pentEnd=pentEnd->m_next);
				m_pCurEnt = pentEnd->m_next;
			}
		}
		do {
			if (!(m_bUpdateOnlyFlagged&(pent->m_flags^pef_update) | bSkipFlagged&pent->m_flags)) {
				m_threadData[iCaller].bGroupInvisible = isneg(-((int)pent->m_flags & pef_invisible));
				for(iter=0; !pent->Step(pent->GetMaxTimeStep(time_interval)) && ++iter<m_vars.nMaxSubsteps; );
			}
			if (pent==pentEnd)
				break;
		} while(pent=pent->m_next);
	} while(true);
}

void CPhysicalWorld::ProcessBreakingEntities(float time_interval)
{
	WriteLockPlatf1 lockp0(m_lockPotentialPartListUpdate),lockp1(m_lockPotentialMeshUpdate);
	WriteLock lock3(m_lockDeformingEntsList);
	int i,j;
	int iCaller = get_iCaller_int();
	for(i=j=0; i<m_nDeformingEnts; i++) if (m_pDeformingEnts[i]->m_iSimClass!=7 && m_pDeformingEnts[i]->UpdateStructure(time_interval,0,iCaller))
		m_pDeformingEnts[j++] = m_pDeformingEnts[i];
	else 
		m_pDeformingEnts[i]->m_flags &= ~pef_deforming;
	m_nDeformingEnts = j;
}

void CPhysicalWorld::ThreadProc(int ithread, SPhysTask *pTask)
{
	if (pTask->bStop) {
		GetISystem()->GetIThreadTaskManager()->UnregisterTask(pTask);
		delete pTask;
		return;
	}
	AtomicAdd(&m_nWorkerThreads,1);
	MarkAsPhysWorkerThread(&ithread);
	static const char *tname[] = { "Physics0","Physics1","Physics2","Physics3" };
	if (ithread-FIRST_WORKER_THREAD<4)
		GetISystem()->GetIThreadTaskManager()->MarkThisThreadForDebugging(tname[ithread-FIRST_WORKER_THREAD],true);

	while(true) {
		m_threadStart[ithread-FIRST_WORKER_THREAD].Wait();
		if (pTask->bStop)	{
			m_threadDone[ithread-FIRST_WORKER_THREAD].Set();
			break;
		}
		switch(m_rq.ipass) {
			case 0: 
			case 1: ProcessNextEntityIsland(m_rq.time_interval, m_rq.ipass, m_rq.iter, *m_rq.pbAllGroupsFinished,ithread); break;
			case 2: ProcessNextEngagedIndependentEntity(ithread); break;
			case 3: ProcessNextLivingEntity(m_rq.time_interval, m_rq.bSkipFlagged, ithread); break;
			case 4: ProcessNextIndependentEntity(m_rq.time_interval, m_rq.bSkipFlagged, ithread); break;
			case 5: ProcessBreakingEntities(m_rq.time_interval); break;
		}
		m_threadDone[ithread-FIRST_WORKER_THREAD].Set();
	}
	if (ithread-FIRST_WORKER_THREAD<4)
		GetISystem()->GetIThreadTaskManager()->MarkThisThreadForDebugging(tname[ithread-FIRST_WORKER_THREAD],false);
	AtomicAdd(&m_nWorkerThreads,-1);
	//delete pTask;
}


int __curstep = 0; // debug


void CPhysicalWorld::TimeStep(float time_interval, int flags)
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );

	float m,/*m_groupTimeStep,*/time_interval_org = time_interval;
	CPhysicalEntity *pent,*phead,*ptail,**pentlist,*pent_next,*pent1,*pentmax;
	int i,i1,j,n,iter,ipass,nGroups,bHeadAdded,bAllGroupsFinished,bSkipFlagged;

	if (time_interval<0)
		return;
	//if (m_vars.bMultithreaded)
	//	m_pLog = 0;

	m_vars.numThreads = min(m_vars.numThreads,MAX_PHYS_THREADS);
	if (m_vars.numThreads!=m_nWorkerThreads+FIRST_WORKER_THREAD) {
		for(i=m_nWorkerThreads-1;i>=0;i--) m_threads[i]->bStop=1,m_threadStart[i].Set(),m_threadDone[i].Wait();
		SThreadTaskParams ttp;
		ttp.name = "PhysicsWorkerThread";
		ttp.nFlags = THREAD_TASK_BLOCKING;
		for(i=0;i<m_vars.numThreads-FIRST_WORKER_THREAD;i++) {
#if defined (XENON)
			ttp.nPreferedThread = 5-m_vars.numJobs+i; 
#endif
			GetISystem()->GetIThreadTaskManager()->RegisterTask(m_threads[i]=new SPhysTask(this,i+FIRST_WORKER_THREAD), ttp);
		}
		for(; m_nWorkerThreads!=m_vars.numThreads-FIRST_WORKER_THREAD; );
	}
#ifdef PS3
	CreateJobThreads(m_vars.numJobs);
#else
	CreateJobThreads(min(m_vars.numJobs,MAX_PHYS_THREADS-1));
#endif

	{ WriteLock lock1(m_lockCaller[MAX_PHYS_THREADS]),lock2(m_lockStep),lock3(m_lockQueue);
		WriteLockPlatf1 lockp0(m_lockPotentialPartListUpdate);
		if (time_interval>0)
			MarkAsPhysThread();
		phys_geometry *pgeom;
		for(i=0; i<m_nQueueSlots; i++) for(j=0; (iter=*(int*)(m_pQueueSlots[i]+j))!=-1; j+=*(int*)(m_pQueueSlots[i]+j+sizeof(int))) {
			if (iter<0)
				continue;
			pent = *(CPhysicalEntity**)(m_pQueueSlots[i]+j+sizeof(int)*2);
			if (!(pent->m_iSimClass==7 || pent->m_iSimClass==5 && pent->m_iDeletionTime==2))	switch (iter) {
				case 0: pent->SetParams((pe_params*)(m_pQueueSlots[i]+j+sizeof(int)*2+sizeof(void*)),1); break;
				case 1: pent->Action((pe_action*)(m_pQueueSlots[i]+j+sizeof(int)*2+sizeof(void*)),1); break;
				case 2: pent->AddGeometry(pgeom=*(phys_geometry**)(m_pQueueSlots[i]+j+sizeof(int)*2+sizeof(void*)),
																	(pe_geomparams*)(m_pQueueSlots[i]+j+sizeof(int)*3+sizeof(void*)*2), 
																	*(int*)(m_pQueueSlots[i]+j+sizeof(int)*2+sizeof(void*)*2),1); 
								AtomicAdd(&pgeom->nRefCount, -1);	break;
				case 3: pent->RemoveGeometry(*(int*)(m_pQueueSlots[i]+j+sizeof(int)*2+sizeof(void*)*2),1); break;
				case 4: 
					pent->m_flags &= ~0x80000000u;
					AtomicAdd(&m_lockGrid,-RepositionEntity(pent,*(int*)(m_pQueueSlots[i]+j+sizeof(int)*2+sizeof(void*)),0,1));	
					if (++m_nEnts > m_nEntsAlloc-1) {
						m_nEntsAlloc += 4096; m_nEntListAllocs++; m_bEntityCountReserved = 0;
						ReallocateList(m_pTmpEntList,m_nEnts-1,m_nEntsAlloc);
						ReallocateList(m_pTmpEntList1,m_nEnts-1,m_nEntsAlloc);
						ReallocateList(m_pTmpEntList2,m_nEnts-1,m_nEntsAlloc);
						ReallocateList(m_pGroupMass,m_nEnts-1,m_nEntsAlloc);
						ReallocateList(m_pMassList,m_nEnts-1,m_nEntsAlloc);
						ReallocateList(m_pGroupIds,m_nEnts-1,m_nEntsAlloc);
						ReallocateList(m_pGroupNums,m_nEnts-1,m_nEntsAlloc);
					}	break;
				case 5: 
					DestroyPhysicalEntity((IPhysicalEntity*)pent, *(int*)(m_pQueueSlots[i]+j+sizeof(int)*2+sizeof(void*)),1);	
			}
			if (iter!=4)
				AtomicAdd(&pent->m_bProcessed,-PENT_QUEUED);
		}
		if (m_nQueueSlots) {
			m_nQueueSlots=1; m_nQueueSlotSize=0; *(int*)m_pQueueSlots[0]=-1;
		}
	}
	WriteLock lock(m_lockStep);
	if (time_interval>0 && !(flags & ent_flagged_only))
		MarkAsPhysThread();

	if (time_interval > m_vars.maxWorldStep)
		time_interval = time_interval_org = m_vars.maxWorldStep;
	
	if (m_vars.timeGranularity>0) {
		i = float2int(time_interval_org*(m_vars.rtimeGranularity=1.0f/m_vars.timeGranularity));
		time_interval_org = time_interval = i*m_vars.timeGranularity;
		m_iTimePhysics += i;
		m_timePhysics = m_iTimePhysics*m_vars.timeGranularity;
	}	else
		m_timePhysics += time_interval;
	if (m_vars.fixedTimestep>0 && time_interval>0)
		time_interval = m_vars.fixedTimestep;
	m_bUpdateOnlyFlagged = flags & ent_flagged_only;
	bSkipFlagged = flags>>1 & pef_update;
	m_bWorldStep = 1;
	m_vars.bMultiplayer = gEnv->bMultiplayer;
	m_vars.bUseDistanceContacts &= m_vars.bMultiplayer^1;
	m_vars.lastTimeStep = m_lastTimeInterval = time_interval;
	if (m_pGlobalArea && !is_unused(m_pGlobalArea->m_gravity))
		m_pGlobalArea->m_gravity = m_vars.gravity;
	m_rq.time_interval=time_interval; m_rq.bSkipFlagged=bSkipFlagged;	m_rq.pbAllGroupsFinished=&bAllGroupsFinished;
	m_pMovedEnts = 0;

	if (m_vars.bDoStep==2) {
		m_vars.bDoStep = 0;
		SerializeWorld("D:\\worldents.txt",1);
		SerializeGeometries("D:\\worldgeoms.txt",1);
		m_vars.bDoStep = 1;
	}

	if (flags & ent_living) {
		for(pent=m_pTypedEnts[3]; pent; pent=pent->m_next) if (!(m_bUpdateOnlyFlagged&(pent->m_flags^pef_update) | bSkipFlagged&pent->m_flags))
			pent->StartStep(time_interval_org*m_vars.timeScalePlayers); // prepare to advance living entities
	}

	if (!m_vars.bSingleStepMode || m_vars.bDoStep) {
		{ SBreakRequest curreq;
			do {
				{ ReadLock lockbq(m_lockBreakQueue);
					if (m_breakQueueSz==0)
						break;
					curreq = m_breakQueue[m_breakQueueTail];
					m_breakQueueTail = m_breakQueueTail+1 - (m_breakQueueAlloc & m_breakQueueAlloc-2-m_breakQueueTail>>31);
					m_breakQueueSz--; 
				}
				if (curreq.pent->m_iSimClass!=7) {
					int ipart;
					for(ipart=curreq.pent->m_nParts-1; ipart>=0 && curreq.pent->m_parts[ipart].id!=curreq.partid; ipart--);
					if (ipart>=0 && DeformEntityPart(curreq.pent,ipart, &curreq.expl, curreq.gwd,curreq.gwd+1) && 
							curreq.pent->UpdateStructure(0.01f,&curreq.expl,-1,curreq.gravity))
						MarkEntityAsDeforming(curreq.pent);
				}
				curreq.pent->Release();
			} while(true);
		}

		iter = 0;	__curstep++;
		if (!(__curstep & 7))
			memset(g_idata[0].UsedNodesMap, 0, sizeof(g_idata[0].UsedNodesMap));
		if (flags & ent_independent) {
			for(pent=m_pTypedEnts[4]; pent; pent=pent->m_next) if (!(m_bUpdateOnlyFlagged&(pent->m_flags^pef_update) | bSkipFlagged&pent->m_flags))
				pent->StartStep(time_interval);
		}

		if (flags & ent_rigid && time_interval>0) {
			if (m_pTypedEnts[2]) do { // make as many substeps as required
				bAllGroupsFinished = 1;	m_pAuxStepEnt = 0;
				m_pGroupNums[m_nEntsAlloc-1] = -1; // special group for rigid bodies w/ infinite mass
				m_threadData[0].bGroupInvisible=m_bCurGroupInvisible = 0;
				m_iSubstep++;

				for(ipass=0; ipass<2; ipass++) {
					// build lists of intercolliding groups of entities
					for(pent=m_pTypedEnts[2],nGroups=0; pent; pent=pent_next) {
						pent_next = pent->m_next; 
						if (!(pent->m_bMoved | m_bUpdateOnlyFlagged&(pent->m_flags^pef_update) | bSkipFlagged&pent->m_flags)) {
							if (pent->GetMassInv()<=0) { 
								if ((iter|ipass)==0) { // just make isolated step for rigids with infinite mass
									pent->StartStep(time_interval); 
									pent->m_iGroup = -1;//m_nEntsAlloc-1;
								}
								if (ipass==0)	{
									pent->Step(/*m_groupTimeStep = */pent->GetMaxTimeStep(time_interval));
									bAllGroupsFinished &= pent->Update(time_interval,1);
								}
							} else {
								pent->m_iGroup = nGroups; pent->m_bMoved = 1;	m_pGroupIds[nGroups] = 0;
								m_pGroupMass[nGroups] = 1.0f/pent->GetMassInv();
								if ((iter | ipass)==0) pent->StartStep(time_interval);
								pent->m_next_coll1 = pent->m_next_coll = 0;
								m_pTmpEntList1[nGroups] = 0;
								// initially m_pTmpEntList1 points to group entities that collide with statics (sorted by mass) - linked via m_next_coll
								// m_next_coll1 maintains a queue of current intercolliding objects

								for(phead=ptail=pentmax=pent; phead; phead=phead->m_next_coll1) {
									for(i=bHeadAdded=0,n=phead->GetColliders(pentlist); i<n; i++) if (pentlist[i]->GetMassInv()<=0) {
										if (!bHeadAdded) {
											for(pent1=m_pTmpEntList1[nGroups]; pent1 && pent1->m_next_coll && pent1->m_next_coll->GetMassInv()<=phead->GetMassInv(); 
													pent1=pent1->m_next_coll);
											if (!pent1 || pent1->GetMassInv()>phead->GetMassInv()) {
												phead->m_next_coll = pent1; m_pTmpEntList1[nGroups] = phead;
											} else {
												phead->m_next_coll = pent1->m_next_coll; pent1->m_next_coll = phead;
											}
											bHeadAdded = 1;
										}
										m_pGroupIds[nGroups] = 1; // tells that group has static entities
									} else if (!(pentlist[i]->m_bMoved | m_bUpdateOnlyFlagged & (pentlist[i]->m_flags^pef_update))) {
										pentlist[i]->m_flags &= ~bSkipFlagged;
										ptail->m_next_coll1 = pentlist[i]; ptail = pentlist[i]; ptail->m_next_coll1 = 0;
										ptail->m_next_coll = 0;
										ptail->m_iGroup = nGroups; ptail->m_bMoved = 1;
										if ((iter | ipass)==0) ptail->StartStep(time_interval);
										m_pGroupMass[nGroups] += 1.0f/(m=ptail->GetMassInv());
										if (pentmax->GetMassInv()>m)
											pentmax = ptail;
									}
								}
								if (!m_pTmpEntList1[nGroups])
									m_pTmpEntList1[nGroups] = pentmax;
								nGroups++;
							}
						}
					}

					// add maximum group mass to all groups that contain static entities
					for(i=1,m=m_pGroupMass[0]; i<nGroups; i++) m = max(m,m_pGroupMass[i]);
					for(m*=1.01f,i=0; i<nGroups; i++) m_pGroupMass[i] += m*m_pGroupIds[i];
					for(i=0;i<nGroups;i++) m_pGroupIds[i] = i;

					// sort groups by decsending group mass
					qsort(m_pTmpEntList1,m_pGroupMass,m_pGroupIds, 0,nGroups-1);
					for(i=0;i<nGroups;i++) m_pGroupNums[m_pGroupIds[i]] = i;

					for(i=0;i<nGroups;i++) {
						for(ptail=m_pTmpEntList1[i]; ptail->m_next_coll; ptail=ptail->m_next_coll) 
							ptail->m_bMoved=0; ptail->m_bMoved=0; 
						for(phead=m_pTmpEntList1[i]; phead; phead=phead->m_next_coll)
							for(j=0,n=phead->GetColliders(pentlist); j<n; j++) if (pentlist[j]->GetMassInv()>0) {
								if (pentlist[j]->m_bMoved==1 && !(m_bUpdateOnlyFlagged & (pentlist[j]->m_flags^pef_update))) {
									ptail->m_next_coll = pentlist[j]; ptail = pentlist[j]; ptail->m_next_coll = 0; 
									ptail->m_bMoved = 0; 
								} 
							}
					}

					m_nGroups=nGroups; m_maxGroupMass=m;
					m_rq.iter=iter;	m_iCurGroup=0;
					THREAD_TASK(ipass, ProcessNextEntityIsland(time_interval,ipass,iter,bAllGroupsFinished,0));
          while(WaitForJobFinish(bAllGroupsFinished,1)>=0)
            sys_timer_subusleep_inldb16cyc(16);//gives other hardware thread all instr.fetch cycles

					if (ipass==0) {
						for(i=0;i<m_nGroups;i++)
							for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll) 
								pent->m_bMoved=0, pent->m_iGroup=-1;
						ReadLockPlatf1 lockp1(m_lockPotentialMeshUpdate);
						m_bWorldStep = 2;
						m_pCurEnt=m_pAuxStepEnt; m_pAuxStepEnt=0;
						THREAD_TASK(2, ProcessNextEngagedIndependentEntity(0))
						while(WaitForJobFinish(bAllGroupsFinished,1)>=0)
							sys_timer_subusleep_inldb16cyc(16);
						m_bWorldStep = 1;
					}

					/*for(i=0;i<nGroups;i++) {
						m_iCurGroup = m_pGroupIds[i];
						m_curGroupMass = m_pGroupMass[i]-m*isneg(m-m_pGroupMass[i]);
						m_groupTimeStep = time_interval*(ipass^1); nAnimatedObjects = 0;
						for(ptail=m_pTmpEntList1[i]; ptail->m_next_coll; ptail=ptail->m_next_coll) ptail->m_bMoved = 0;
						ptail->m_bMoved = 0;
						m_bCurGroupInvisible = m_pTmpEntList1[i]->m_flags & pef_invisible;
						fixedDamping = -1.0f;
						for(phead=m_pTmpEntList1[i]; phead; phead=phead->m_next_coll)
							for(j=0,n=phead->GetColliders(pentlist); j<n; j++) if (pentlist[j]->GetMassInv()>0) {
								if (!(pentlist[j]->m_bMoved^1 | m_bUpdateOnlyFlagged & (pentlist[j]->m_flags^pef_update))) {
									ptail->m_next_coll = pentlist[j]; ptail = pentlist[j]; ptail->m_next_coll = 0; ptail->m_bMoved = 0;
								} 
								m_bCurGroupInvisible &= pentlist[j]->m_flags;
							} else if (pentlist[j]->m_iSimClass>1) {
								if (ipass) {
									if (pentlist[j]->m_flags & pef_fixed_damping)
										fixedDamping = max(fixedDamping, pentlist[j]->GetDamping(pentlist[j]->GetMaxTimeStep(time_interval)));
									m_groupTimeStep = max(m_groupTimeStep, pentlist[j]->GetLastTimeStep(time_interval));
								} else
									m_groupTimeStep = min(m_groupTimeStep, pentlist[j]->GetMaxTimeStep(time_interval));
								nAnimatedObjects++;
								m_bCurGroupInvisible &= pentlist[j]->m_flags | ~-iszero(pentlist[j]->m_iSimClass-2);
							}
						m_bCurGroupInvisible = -(-m_bCurGroupInvisible>>31);

						if (ipass==0) {
							ReadLockPlatf1 lockp1(m_lockPotentialMeshUpdate);
							for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll)
								m_groupTimeStep = min(m_groupTimeStep, pent->GetMaxTimeStep(time_interval));
							for(pent=m_pTmpEntList1[i],bStepValid=1,phead=0; pent; pent=pent_next) {
								pent_next=pent->m_next_coll;
								if (pent->m_iSimClass<3)
									bStepValid &= (phead=pent)->Step(m_groupTimeStep); 
								pent->m_bMoved = 1;
							}
							if (!bStepValid) {
								for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll)
									pent->StepBack(m_groupTimeStep);
								//for(pent=m_pAuxStepEnt1; pent; pent=pent->m_next_coll) 
								//	pent->m_flags &= ~pef_step_requested;
							}
							for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll) pent->m_bMoved = 0;
						} else if (time_interval>0) {
							for(pent=m_pTmpEntList1[i],m_groupTimeStep=0; pent; pent=pent->m_next_coll) if (pent->m_iSimClass>1)
								m_groupTimeStep = max(m_groupTimeStep, pent->GetLastTimeStep(time_interval));
							if (m_groupTimeStep==0)
								m_groupTimeStep = time_interval;
							InitContactSolver(m_groupTimeStep);
							Ebefore = Eafter = 0.0f; 

							if (m_vars.nMaxPlaneContactsDistress!=m_vars.nMaxPlaneContacts) {
								for(pent=m_pTmpEntList1[i],j=nEnts=0; pent; pent=pent->m_next_coll,nEnts++)	{
									j += pent->GetContactCount(m_vars.nMaxPlaneContacts);
									Ebefore += pent->CalcEnergy(m_groupTimeStep);
								}
								n = j>m_vars.nMaxContacts ? m_vars.nMaxPlaneContactsDistress : m_vars.nMaxPlaneContacts;
								for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll)
									pent->RegisterContacts(m_groupTimeStep,n);
							} else for(pent=m_pTmpEntList1[i],nEnts=0; pent; pent=pent->m_next_coll,nEnts++) {
								pent->RegisterContacts(m_groupTimeStep, m_vars.nMaxPlaneContacts);
								Ebefore += pent->CalcEnergy(m_groupTimeStep);
							}

							Ebefore = max(m_pGroupMass[i]*sqr(0.005f),Ebefore);

							#ifdef ENTITY_PROFILER_ENABLED
							i1 = CryGetTicks();
							#endif

							InvokeContactSolver(m_groupTimeStep, &m_vars, Ebefore);
							for(j=0;j<g_nContacts;j++) if ((g_pContacts[j]->ipart[0]|g_pContacts[j]->ipart[1])>=0) {
								if (g_pContacts[j]->pent[0]->m_parts[g_pContacts[j]->ipart[0]].flags & geom_monitor_contacts)
									g_pContacts[j]->pent[0]->OnContactResolved(g_pContacts[j],0,m_iCurGroup);
								if (g_pContacts[j]->pent[1]->m_parts[g_pContacts[j]->ipart[1]].flags & geom_monitor_contacts)
									g_pContacts[j]->pent[1]->OnContactResolved(g_pContacts[j],1,m_iCurGroup);
							}

							#ifdef ENTITY_PROFILER_ENABLED
							i1 = CryGetTicks()-i1;
							if (m_vars.bProfileEntities) {
								for(pent=m_pTmpEntList1[i],j=0; pent; pent=pent->m_next_coll)
									j += -pent->m_iSimClass>>31 & 1;
								i1 /= max(1,j);
								for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll) if (pent->m_iSimClass>0)
									AddEntityProfileInfo(pent,i1);
							}
							#endif

							//if (nAnimatedObjects==0) 
							damping = 1.0f-m_groupTimeStep*m_vars.groupDamping*isneg(m_vars.nGroupDamping-1-nEnts);//max(nEnts,g_nBodies));
							for(pent=m_pTmpEntList1[i],bGroupFinished=1; pent; pent=pent->m_next_coll) {
								Eafter += pent->CalcEnergy(0);	
								if (!(pent->m_flags & pef_fixed_damping))
									damping = min(damping,pent->GetDamping(m_groupTimeStep));
								else {
									damping = pent->GetDamping(m_groupTimeStep);
									break;
								}
							}
							Ebefore *= isneg(-nAnimatedObjects)+1; // increase energy growth limit if we have animated bodies involved
							if (Eafter>Ebefore*(1.0f+0.1f*isneg(g_nBodies-15)))
								damping = min(damping, sqrt_tpl(Ebefore/Eafter));
							if (fixedDamping>-0.5f)
								damping = fixedDamping;
							for(pent=m_pTmpEntList1[i],bGroupFinished=1; pent; pent=pent->m_next_coll)
								bGroupFinished &= pent->Update(m_groupTimeStep, damping);
							bGroupFinished |= isneg(m_vars.nMaxSubstepsLargeGroup-iter-2 & m_vars.nBodiesLargeGroup-g_nBodies-1);
							bGroupFinished |= isneg(m_vars.nMaxSubsteps-iter-2);
							if (!bGroupFinished) for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll)
								pent->m_bMoved = 0;
							else for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll) {
								pent->m_bMoved = 1;
								if (pent->m_iSimClass<3 && !pent->m_next_coll2) {
									pent->m_next_coll2=pent_bMoved; pent_bMoved=pent;
								}
							}
							bAllGroupsFinished &= bGroupFinished;

							// process deforming (breaking) enities of this group
							{ WriteLock lock3(m_lockDeformingEntsList);
								WriteLockPlatf1 lockp0(m_lockPotentialPartListUpdate),lockp1(m_lockPotentialMeshUpdate);
								for(i1=j=nBrokenParts=0; i1<m_nDeformingEnts; i1++) if (m_pDeformingEnts[i1]->m_iGroup==m_iCurGroup) {
									if ((nParts0=m_pDeformingEnts[i1]->m_nParts) && m_pDeformingEnts[i1]->m_iSimClass!=7) {
										if (m_pDeformingEnts[i1]->UpdateStructure(max(m_groupTimeStep,0.01f),0))
											m_pDeformingEnts[j++] = m_pDeformingEnts[i1];
										else
											m_pDeformingEnts[i1]->m_flags &= ~pef_deforming;
										nBrokenParts += -iszero((int)m_pDeformingEnts[i1]->m_flags & aef_recorded_physics) & nParts0-m_pDeformingEnts[i1]->m_nParts;
									} else 
										m_pDeformingEnts[i1]->m_flags &= ~pef_deforming;
									m_pDeformingEnts[i1]->m_iGroup = -1;
								}	else
									m_pDeformingEnts[j++] = m_pDeformingEnts[i1];
								m_nDeformingEnts = j;
							}
							// if some entities broke, step back the velocities, but don't re-execute the step immediately
							if (nBrokenParts) for(pent=m_pTmpEntList1[i]; pent; pent=pent->m_next_coll)
								pent->StepBack(0);
						}
					}

					if (ipass==0) {
						ReadLockPlatf1 lockp1(m_lockPotentialMeshUpdate);
						m_bWorldStep = 2;
						for(pent=m_pAuxStepEnt; pent; pent=pent_next) {
							pent_next = pent->m_next_coll2;
							pent->m_flags &= ~pef_step_requested;
							pent->Step(pent->GetMaxTimeStep(pent->m_timeIdle));
							pent->m_bMoved = 0;	pent->m_next_coll2 = 0;
						}
						m_bWorldStep = 1;
						m_pAuxStepEnt = 0;
					}*/
				}
			} while (!bAllGroupsFinished && ++iter<m_vars.nMaxSubsteps);

			for(pent=(CPhysicalEntity*)m_pMovedEnts; pent; pent=pent_next) {
				pent_next = pent->m_next_coll2;
				pent->m_bMoved=0, pent->m_iGroup=-1; pent->m_next_coll2=0;
			}
			for(pent=m_pTypedEnts[4]; pent; pent=pent->m_next) pent->m_bMoved=0,pent->m_iGroup=-1;
			m_updateTimes[1] = m_updateTimes[2] = m_timePhysics;
		}

		if (m_pWaterMan)
			m_pWaterMan->TimeStep(time_interval);
		for(i=0;i<m_nProfiledEnts;i++)
			m_pEntProfileData[i].nTicksStep &= -m_pEntProfileData[i].nTicks>>31;
	}
	m_iSubstep++;

	if (flags & ent_living) {
		ReadLockPlatf1 lockp1(m_lockPotentialMeshUpdate);
		m_pCurEnt = m_pTypedEnts[3];
		THREAD_TASK(3, ProcessNextLivingEntity(time_interval,bSkipFlagged,0))
		/*for(pent=m_pTypedEnts[3]; pent; pent=pent_next) {
			pent_next = pent->m_next;
			if (!(m_bUpdateOnlyFlagged&(pent->m_flags^pef_update) | bSkipFlagged&pent->m_flags))
				pent->Step(pent->GetMaxTimeStep(time_interval_org*m_vars.timeScalePlayers)); // advance living entities
		}*/
		m_updateTimes[3] = m_timePhysics;
	}

	if (!m_vars.bSingleStepMode || m_vars.bDoStep) {
		if (flags & ent_independent) {
			ReadLockPlatf1 lockp1(m_lockPotentialMeshUpdate);
			m_pCurEnt = m_pTypedEnts[4];
			THREAD_TASK(4, ProcessNextIndependentEntity(time_interval,bSkipFlagged,0));
      while(WaitForJobFinish(bAllGroupsFinished,1)>=0) {
        sys_timer_subusleep_inldb16cyc(16);//gives other hardware thread all instr.fetch cycles
      }
			m_updateTimes[4] = m_timePhysics;
		}
	}

	if (flags & ent_deleted) {
		if (!m_vars.bSingleStepMode || m_vars.bDoStep) {
			// process deforming (breaking) enities
			{ WriteLockPlatf1 lockp0(m_lockPotentialPartListUpdate),lockp1(m_lockPotentialMeshUpdate);
				WriteLock lock3(m_lockDeformingEntsList);
				for(i=j=0; i<m_nDeformingEnts; i++) if (m_pDeformingEnts[i]->m_iSimClass!=7 && m_pDeformingEnts[i]->UpdateStructure(time_interval,0))
					m_pDeformingEnts[j++] = m_pDeformingEnts[i];
				else 
					m_pDeformingEnts[i]->m_flags &= ~pef_deforming;
				m_nDeformingEnts = j;
				m_updateTimes[0] = m_timePhysics;
			}

			CleanseEventsQueue(); // remove events that reference deleted entities

			for(pent=m_pTypedEnts[7]; pent; pent=pent_next) { // purge deletion requests
				pent_next = pent->m_next; 
				if (m_iLastLogPump>=pent->m_iDeletionTime && pent->m_nRefCount<=0)	{
					if (pent->m_next) pent->m_next->m_prev = pent->m_prev;
					(pent->m_prev ? pent->m_prev->m_next : m_pTypedEnts[7]) = pent->m_next;
					delete pent; 
				}
			}
			//m_pTypedEnts[7] = 0;
		}

		// flush timeouted sectors for cell-based physics-on-demand
		{ WriteLock lockPOD(m_lockPODGrid);
			MarkAsPODThread(this);
			pe_PODcell *pPODcell;
			int *picellNext;
			for(i=m_iActivePODCell0,picellNext=&m_iActivePODCell0; i>=0; i=pPODcell->inextActive) 
				if (((pPODcell=getPODcell(i&0xFFFF,i>>16))->lifeTime-=time_interval_org)<=0 || pPODcell->lifeTime>1E9f) {
					Vec3 center,sz;	++m_iLastPODUpdate;
					GetPODGridCellBBox(i&0xFFFF,i>>16, center,sz);
					m_pPhysicsStreamer->DestroyPhysicalEntitiesInBox(center-sz,center+sz);
					*picellNext = pPODcell->inextActive;
				} else picellNext = &pPODcell->inextActive;
			UnmarkAsPODThread(this);
		}

		// flush static and sleeping physical objects that have timeouted
		for(i=0;i<2;i++) for(pent=m_pTypedEnts[i]; pent!=m_pTypedEntsPerm[i]; pent=pent_next) {
			pent_next = pent->m_next;
			{ WriteLock lockEnt(pent->m_lockUpdate);
				for(j=0;j<pent->m_nParts && !(pent->m_parts[j].flags & geom_can_modify);j++);
				if (j<pent->m_nParts || pent->m_pStructure && pent->m_pStructure->bModified)
					j=-1;
			}
			if (j==-1) {
				j -= pent_next==m_pTypedEntsPerm[i];
				pent->m_bPermanent = 1;
				ChangeEntitySimClass(pent);
				if (pent->m_pEntBuddy) {
					CPhysicalPlaceholder *ppc = pent->m_pEntBuddy;
					ppc->m_pEntBuddy=0;	
					ppc->m_iGThunk0=0; 
					SetPhysicalEntityId(ppc,-1,1,1); ppc->m_id=-1; pent->m_pEntBuddy = 0;
					SetPhysicalEntityId(pent,pent->m_id,1,1);
					for(i1=pent->m_iGThunk0;i1;i1=m_gthunks[i1].inextOwned) m_gthunks[i1].pent=pent;
					DestroyPhysicalEntity(ppc,0,1);
				}
				if (j==-2)
					break;
			}	else if (pent->m_nRefCount==0 && ((pent->m_timeIdle+=time_interval_org)>pent->m_maxTimeIdle || pent->m_timeIdle<0))
				DestroyPhysicalEntity(pent,0,1);
		}
/* //Diesel cut
		for(pent=m_pTypedEnts[2]; pent!=m_pTypedEntsPerm[2]; pent=pent->m_next)
			pent->m_timeIdle = 0;	// reset idle count for active physical entities 

		for(pent=m_pTypedEnts[4]; pent!=m_pTypedEntsPerm[4]; pent=pent_next) {
			assert(pent);
			pent_next = pent->m_next;
			if (pent->IsAwake())
				pent->m_timeIdle = 0;	// reset idle count for active detached entities 
			else if (pent->m_nRefCount==0 && (pent->m_timeIdle+=time_interval_org)>pent->m_maxTimeIdle)
				DestroyPhysicalEntity(pent);
		}
*/ //Diesel cut
		// flush deleted areas 
		{ WriteLock lockAreas(m_lockAreas);
			CPhysArea *pArea,**ppNextArea=&m_pDeletedAreas;
			for(pArea=m_pDeletedAreas; pArea; pArea=*ppNextArea) if (pArea->m_lockRef==0) {
				*ppNextArea=pArea->m_next; delete pArea;
			}	else 
				ppNextArea = &pArea->m_next;
		}

		m_updateTimes[7] = m_timePhysics;
		m_vars.bDoStep = 0;
	}
	m_bUpdateOnlyFlagged = 0;
	m_bWorldStep = 0;
	if (time_interval>0)
		++m_idStep;
}


void CPhysicalWorld::DetachEntityGridThunks(CPhysicalPlaceholder *pobj)
{
	if (pobj->m_iGThunk0) {
		int ithunk,ithunk_next,ithunk_last,icell,iprev,inext;
		for(ithunk=pobj->m_iGThunk0; ithunk; ithunk=ithunk_next) {
			ithunk_next = m_gthunks[ithunk].inextOwned;
			iprev=m_gthunks[ithunk].iprev; inext=m_gthunks[ithunk].inext;
			m_gthunks[ithunk].inext=m_gthunks[ithunk].iprev = -1;
			m_gthunks[inext].iprev = iprev & -(int)inext>>31;
			m_gthunks[inext].bFirstInCell = m_gthunks[ithunk].bFirstInCell;
			if (m_gthunks[ithunk].bFirstInCell) {
				icell = m_entgrid.size.x*m_entgrid.size.y;
				if (m_pEntGrid[icell]!=ithunk)
					icell = Vec2i(iprev&1023,iprev>>10&1023)*m_entgrid.stride;
				m_pEntGrid[(unsigned int)icell] = inext;
			}	else
				m_gthunks[iprev].inext = inext;
			m_gthunks[ithunk].pent = 0;
			ithunk_last = ithunk;
		}
		m_gthunks[ithunk_last].inextOwned = m_iFreeGThunk0;
		m_iFreeGThunk0 = pobj->m_iGThunk0;
		pobj->m_iGThunk0 = 0;
	}
}


void CPhysicalWorld::ChangeEntitySimClass(CPhysicalEntity *pent)
{
	WriteLock lock(m_lockList);
	if ((unsigned int)pent->m_iPrevSimClass<8u) {
		if (pent->m_next) pent->m_next->m_prev = pent->m_prev;
		(pent->m_prev ? pent->m_prev->m_next : m_pTypedEnts[pent->m_iPrevSimClass]) = pent->m_next;
		if (pent==m_pTypedEntsPerm[pent->m_iPrevSimClass])
			m_pTypedEntsPerm[pent->m_iPrevSimClass] = pent->m_next;
	}

	if (!pent->m_bPermanent) {
		pent->m_next = m_pTypedEnts[pent->m_iSimClass]; 
		pent->m_prev = 0;
		if (pent->m_next) pent->m_next->m_prev = pent;
		m_pTypedEnts[pent->m_iSimClass] = pent;
	} else {
		pent->m_next = m_pTypedEntsPerm[pent->m_iSimClass];
		if (m_pTypedEntsPerm[pent->m_iSimClass]) {
			if (pent->m_prev = m_pTypedEntsPerm[pent->m_iSimClass]->m_prev)
				pent->m_prev->m_next = pent;
			pent->m_next->m_prev = pent;
		} else if (m_pTypedEnts[pent->m_iSimClass]) {
			for(pent->m_prev=m_pTypedEnts[pent->m_iSimClass]; pent->m_prev && pent->m_prev->m_next; 
				pent->m_prev=pent->m_prev->m_next);
			assert(pent->m_prev);
			pent->m_prev->m_next = pent;
		} else
			pent->m_prev = 0;
		if (m_pTypedEntsPerm[pent->m_iSimClass]==m_pTypedEnts[pent->m_iSimClass])
			m_pTypedEnts[pent->m_iSimClass] = pent;
		m_pTypedEntsPerm[pent->m_iSimClass] = pent;
		if (pent->m_iSimClass>0 && m_pTypedEntsPerm[pent->m_iSimClass]!=m_pTypedEnts[pent->m_iSimClass])
			CryLog("Warning: entity %s @ %.2f,%.2f,%.2f is non-static and non-permanent!", 
				m_pRenderer ? m_pRenderer->GetForeignName(pent->m_pForeignData,pent->m_iForeignData,pent->m_iForeignFlags):"", pent->m_pos.x,pent->m_pos.y,pent->m_pos.z);
	}

	for(int ithunk=pent->m_iGThunk0; ithunk; ithunk=m_gthunks[ithunk].inextOwned) 
		m_gthunks[ithunk].iSimClass = pent->m_iSimClass;
}


int CPhysicalWorld::RepositionEntity(CPhysicalPlaceholder *pobj, int flags, Vec3 *BBox, int bQueued)
{
	int i,j,igx[2],igy[2],igxInner[2],igyInner[2],igz[2],ix,iy,ithunk,ithunk0;
	unsigned int n;
	if ((unsigned int)pobj->m_iSimClass>=7u) return 0; // entity is frozen
	int bGridLocked = 0;

	if (flags&1 && m_pEntGrid) {
		i = -iszero((INT_PTR)BBox);
		Vec3 *pBBox = (Vec3*)((INT_PTR)pobj->m_BBox & (INT_PTR)i | (INT_PTR)BBox & ~(INT_PTR)i);
		for(i=0;i<2;i++) {
			float x = (pBBox[i][inc_mod3[m_iEntAxisz]] - m_entgrid.origin[inc_mod3[m_iEntAxisz]])*m_entgrid.stepr.x;
			igx[i] = max(-1,min(m_entgrid.size.x, float2int(x-0.5f)));
			igxInner[i] = max(0,min(255,float2int((x-igx[i])*256.0f-0.5f)));
			x = (pBBox[i][dec_mod3[m_iEntAxisz]] - m_entgrid.origin[dec_mod3[m_iEntAxisz]])*m_entgrid.stepr.y;
			igy[i] = max(-1,min(m_entgrid.size.y, float2int(x-0.5f)));
			igyInner[i] = max(0,min(255,float2int((x-igy[i])*256.0f-0.5f)));
			igz[i] = (int)((pBBox[i][m_iEntAxisz]-m_entgrid.origin[m_iEntAxisz])*m_rzGran)+i;
		}
		if (pobj->m_ig[0].x!=-3) // if m_igx[0] is -3, the entity should not be registered in grid at all
			if (igx[0]-pobj->m_ig[0].x | igy[0]-pobj->m_ig[0].y | igx[1]-pobj->m_ig[1].x | igy[1]-pobj->m_ig[1].y) {
				CPhysicalPlaceholder *pcurobj = pobj;
				if (IsPlaceholder(pobj->m_pEntBuddy))
					goto skiprepos; //pcurobj = pobj->m_pEntBuddy;
				SpinLock(&m_lockGrid,0,bGridLocked = WRITE_LOCK_VAL);
				m_bGridThunksChanged = 1;
				DetachEntityGridThunks(pobj);
				n = (igx[1]-igx[0]+1)*(igy[1]-igy[0]+1);
				if (pobj->m_iSimClass!=5) {
					if (n==0 || n>(unsigned int)m_vars.nMaxEntityCells) {
						Vec3 pos = (pcurobj->m_BBox[0]+pcurobj->m_BBox[1])*0.5f;
						char buf[256]; sprintf(buf,"Error: %s @ %.1f,%.1f,%.1f is too large or invalid", !m_pRenderer ? "entity" : 
							m_pRenderer->GetForeignName(pcurobj->m_pForeignData,pcurobj->m_iForeignData,pcurobj->m_iForeignFlags), pos.x,pos.y,pos.z);
						VALIDATOR_LOG(m_pLog,buf);
						if (m_vars.bBreakOnValidation) DoBreak
						pobj->m_ig[0].x=pobj->m_ig[1].x=pobj->m_ig[0].y=pobj->m_ig[1].y = -2;
						goto skiprepos;
					}
				} else if (n>(unsigned int)m_vars.nMaxAreaCells)
					return -1;
				for(ix=igx[0];ix<=igx[1];ix++) for(iy=igy[0];iy<=igy[1];iy++) {
					j = m_entgrid.getcell_safe(ix,iy);
					if (!m_iFreeGThunk0) {
						if (m_thunkPoolSz>=1<<20) {
							static bool g_bSpammed = false;
							if (!g_bSpammed)
								VALIDATOR_LOG(m_pLog, "Error: too many entity grid thunks created, further repositions ignored");
							g_bSpammed = true;
							goto skiprepos;
						}
						pe_gridthunk *prevthunks = m_gthunks;
						int szNew = max(1024, m_thunkPoolSz*2);
						memcpy(m_gthunks = new pe_gridthunk[szNew], prevthunks, m_thunkPoolSz*sizeof(pe_gridthunk));
						memset(m_gthunks+m_thunkPoolSz, 0, (szNew-m_thunkPoolSz)*sizeof(pe_gridthunk));
						for(ithunk=m_thunkPoolSz; ithunk<szNew-1; ithunk++)
							m_gthunks[ithunk].inextOwned = ithunk+1;
						m_gthunks[ithunk].inextOwned = 0;
						if (prevthunks)
							delete[] prevthunks; 
						m_iFreeGThunk0=m_thunkPoolSz; m_thunkPoolSz=szNew;
					}
					ithunk = m_iFreeGThunk0; m_iFreeGThunk0 = m_gthunks[m_iFreeGThunk0].inextOwned;
					m_gthunks[ithunk].inextOwned = pcurobj->m_iGThunk0;	pcurobj->m_iGThunk0 = ithunk;
					int ithunkGrid = m_pEntGrid[j];

					if (!ithunkGrid || m_gthunks[ithunkGrid].iSimClass!=5) {
						m_gthunks[ithunk].bFirstInCell = 1;
						m_gthunks[ithunk].iprev = iy<<10|ix;
						m_gthunks[ithunk].inext = m_pEntGrid[j];
						m_gthunks[ithunkGrid].iprev = ithunk & -ithunkGrid>>31;
						m_gthunks[ithunkGrid].bFirstInCell = 0;
						m_pEntGrid[(unsigned int)j] = ithunk;
					}	else {
						for(ithunk0=ithunkGrid; m_gthunks[m_gthunks[ithunk0].inext].iSimClass==5; ithunk0=m_gthunks[ithunk0].inext);
						m_gthunks[ithunk].bFirstInCell = 0;
						m_gthunks[ithunk].inext = m_gthunks[ithunk0].inext;
						m_gthunks[ithunk].iprev = ithunk0;
						m_gthunks[m_gthunks[ithunk0].inext].iprev = ithunk & -(int)m_gthunks[ithunk0].inext>>31;
						m_gthunks[ithunk0].inext = ithunk;
					}

					m_gthunks[ithunk].iSimClass = pcurobj->m_iSimClass;
					m_gthunks[ithunk].BBox[0] = igxInner[0] & ~(igx[0]-ix>>31);
					m_gthunks[ithunk].BBox[1] = igyInner[0] & ~(igy[0]-iy>>31);
					m_gthunks[ithunk].BBox[2] = igxInner[1] + (255-igxInner[1] & ix-igx[1]>>31);
					m_gthunks[ithunk].BBox[3] = igyInner[1] + (255-igyInner[1] & iy-igy[1]>>31);
					m_gthunks[ithunk].BBoxZ0  = igz[0];
					m_gthunks[ithunk].BBoxZ1  = igz[1];
					m_gthunks[ithunk].pent = pcurobj;
				}
				pcurobj->m_ig[0].x=igx[0]; pcurobj->m_ig[1].x=igx[1]; 
				pcurobj->m_ig[0].y=igy[0]; pcurobj->m_ig[1].y=igy[1];
				if (pcurobj->m_pEntBuddy && pcurobj->m_pEntBuddy->m_pEntBuddy==pcurobj) {
					pcurobj->m_pEntBuddy->m_iGThunk0 = pcurobj->m_iGThunk0;
					pcurobj->m_pEntBuddy->m_ig[0].x=igx[0]; pcurobj->m_pEntBuddy->m_ig[1].x=igx[1];
					pcurobj->m_pEntBuddy->m_ig[0].y=igy[0]; pcurobj->m_pEntBuddy->m_ig[1].y=igy[1];
				}
				skiprepos:;
			} else for(ix=igx[1],ithunk=pobj->m_iGThunk0;ix>=igx[0];ix--) for(iy=igy[1];iy>=igy[0];iy--,ithunk=m_gthunks[ithunk].inextOwned) {
				m_gthunks[ithunk].BBox[0] = igxInner[0] & ~(igx[0]-ix>>31);
				m_gthunks[ithunk].BBox[1] = igyInner[0] & ~(igy[0]-iy>>31);
				m_gthunks[ithunk].BBox[2] = igxInner[1] + (255-igxInner[1] & ix-igx[1]>>31);
				m_gthunks[ithunk].BBox[3] = igyInner[1] + (255-igyInner[1] & iy-igy[1]>>31);
				m_gthunks[ithunk].BBoxZ0  = igz[0];
				m_gthunks[ithunk].BBoxZ1  = igz[1];
			}
	}

	if (flags&2) {
		CPhysicalEntity *pent = (CPhysicalEntity*)pobj;
		if (pent->m_iPrevSimClass!=pent->m_iSimClass && pent->m_bPermanent+bQueued) {
			ChangeEntitySimClass(pent);			
			i = pent->m_iPrevSimClass;
			pent->m_iPrevSimClass = pent->m_iSimClass;

			if (pent->m_flags & (pef_monitor_state_changes | pef_log_state_changes)) {
				EventPhysStateChange event;
				event.pEntity=pent; event.pForeignData=pent->m_pForeignData; event.iForeignData=pent->m_iForeignData;
				event.iSimClass[0] = i; event.iSimClass[1] = pent->m_iSimClass;
				event.timeIdle = pent->m_timeIdle;
				OnEvent(pent->m_flags,&event);
			}

/*#ifdef _DEBUG
CPhysicalEntity *ptmp = m_pTypedEnts[1];
for(;ptmp && ptmp!=m_pTypedEntsPerm[1]; ptmp=ptmp->m_next);
if (ptmp!=m_pTypedEntsPerm[1])
DEBUG_BREAK;
#endif*/
		}
	}

	return bGridLocked;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////


float CPhysicalWorld::IsAffectedByExplosion(IPhysicalEntity *pobj, Vec3 *impulse)
{
	int i;
	CPhysicalEntity *pent = ((CPhysicalPlaceholder*)pobj)->GetEntityFast();
	for(i=0;i<m_nExplVictims && m_pExplVictims[i]!=pent;i++);
	if (i<m_nExplVictims) {
		if (impulse)
			*impulse = m_pExplVictimsImp[i];
		return m_pExplVictimsFrac[i];
	}
	if (impulse)
		impulse->zero();
	return 0.0f;
}

int CPhysicalWorld::DeformPhysicalEntity(IPhysicalEntity *pient, const Vec3 &ptHit,const Vec3 &dirHit,float r, int flags)
{
	// craig - experimental fix: i think the random number in GetExplosionShape() is upsetting things in MP
	if (m_vars.bMultiplayer)
		g_random_generator.seed(1234567);

	int i,bEntChanged,bPartChanged;
	CPhysicalEntity *pent = (CPhysicalEntity*)pient;
	pe_explosion expl;
	geom_world_data gwd,gwd1;
	box bbox;
	Vec3 zaxWorld(0,0,1),zaxObj,zax;
	gwd1.offset = ptHit;
	(zaxWorld -= dirHit*(zaxWorld*dirHit)).normalize();
	expl.epicenter=expl.epicenterImp = ptHit;
	expl.impulsivePressureAtR = 0;
	expl.r=expl.rmin=expl.holeSize = r;
	if (flags & 2) { // special values for explosion
		expl.explDir = dirHit;
		expl.impulsivePressureAtR = -1;
	}
	expl.iholeType = 0;

	{ WriteLock lock(pent->m_lockUpdate);
		for(i=bEntChanged=0; i<pent->m_nParts; i++) 
		if (pent->m_parts[i].flags & geom_colltype_explosion && pent->m_parts[i].idmatBreakable>=0) {
			pent->m_parts[i].pPhysGeomProxy->pGeom->GetBBox(&bbox);
			zaxObj = pent->m_qrot*(pent->m_parts[i].q*bbox.Basis.GetRow(idxmax3(bbox.size)));
			if (pent->m_iSimClass>0 || fabs_tpl(zaxObj*zaxWorld)<0.7f)
				(zax = zaxObj-dirHit*(zaxObj*dirHit)).normalize();
			else zax = zaxWorld;
			gwd1.R.SetColumn(0,dirHit^zax); gwd1.R.SetColumn(1,dirHit); gwd1.R.SetColumn(2,zax);
			gwd.R = Matrix33(pent->m_qrot*pent->m_parts[i].q);
			gwd.offset = pent->m_pos + pent->m_qrot*pent->m_parts[i].pos;
			gwd.scale = pent->m_parts[i].scale;
			bEntChanged += (bPartChanged=DeformEntityPart(pent,i, &expl, &gwd,&gwd1, 1));
			if (bPartChanged)
				pent->m_parts[i].flags &= ~(flags>>16 & 0xFFFF);
		}
	}
	if (bEntChanged && pent->UpdateStructure(0.01f,&expl,MAX_PHYS_THREADS))
		MarkEntityAsDeforming(pent);

	return bEntChanged;
}


void CPhysicalWorld::ClonePhysGeomInEntity(CPhysicalEntity *pent,int i,IGeometry *pNewGeom)
{
	phys_geometry *pgeom;
	if (pNewGeom->GetType()==GEOM_TRIMESH && pent->m_parts[i].pLattice)
		(pent->m_parts[i].pLattice = new CTetrLattice(pent->m_parts[i].pLattice,1))->SetMesh((CTriMesh*)pNewGeom);
	{ WriteLock lock(m_lockGeoman);
		*(pgeom = GetFreeGeomSlot()) = *pent->m_parts[i].pPhysGeomProxy;
		pgeom->pGeom = pNewGeom;
		pgeom->nRefCount = 1;
		pgeom->surface_idx = 0;
		if (pgeom->pMatMapping)
			memcpy(pgeom->pMatMapping=new int[pgeom->nMats], pent->m_parts[i].pPhysGeomProxy->pMatMapping, pgeom->nMats*sizeof(int));
	}
	if (pent->m_parts[i].pPhysGeom->pMatMapping==pent->m_parts[i].pMatMapping)
		pent->m_parts[i].pMatMapping = pgeom->pMatMapping;
	UnregisterGeometry(pent->m_parts[i].pPhysGeom);
	if (pent->m_parts[i].pPhysGeomProxy!=pent->m_parts[i].pPhysGeom)
		UnregisterGeometry(pent->m_parts[i].pPhysGeomProxy);
	pent->m_parts[i].pPhysGeomProxy = pent->m_parts[i].pPhysGeom = pgeom;
	pent->m_parts[i].flags |= geom_can_modify;
}


int CPhysicalWorld::DeformEntityPart(CPhysicalEntity *pent,int i, pe_explosion *pexpl, geom_world_data *gwd,geom_world_data *gwd1, int iSource)
{
	IGeometry *pGeom,*pHole;
	CTriMesh *pNewGeom=0;
	EventPhysUpdateMesh epum;
	int bCreateConstraint=0;
	epum.pEntity = pent;
	epum.pForeignData = pent->m_pForeignData;
	epum.iForeignData = pent->m_iForeignData;
	epum.partid = pent->m_parts[i].id;
	epum.iReason = iSource ? EventPhysUpdateMesh::ReasonRequest : EventPhysUpdateMesh::ReasonExplosion;
	epum.bInvalid = 0;
	if ((pent->m_parts[i].idmatBreakable>>7 | pexpl->iholeType)!=0 && !(pent->m_parts[i].idmatBreakable & 128<<pexpl->iholeType))
		return 0;
	for(int j=0;j<min(pent->m_nParts,5);j++) 
		if (i!=j && pent->m_parts[j].flags & geom_log_interactions && pent->m_parts[j].pPhysGeom->pGeom->PointInsideStatus(
			((pexpl->epicenter-pent->m_pos)*pent->m_qrot-pent->m_parts[j].pos)*pent->m_parts[j].q*(
			pent->m_parts[j].scale==1.0f ? 1.0f:1.0f/pent->m_parts[j].scale)))
			return 0;

	if (pHole = GetExplosionShape(pexpl->holeSize,pent->m_parts[i].idmatBreakable,gwd1->scale,bCreateConstraint)) {
		pGeom = pent->m_parts[i].pPhysGeomProxy->pGeom;
		if (!(pent->m_parts[i].flags & geom_can_modify)) {
			if (!(pNewGeom = (CTriMesh*)((CGeometry*)pGeom)->GetTriMesh()))
				return 0;
			pGeom = pNewGeom;
		}	else {
			box bbox,bbox1; pGeom->GetBBox(&bbox); pHole->GetBBox(&bbox1);
			float szmin0 = min(min(bbox.size.x,bbox.size.y),bbox.size.z);
			if (szmin0 > max(max(bbox.size.x,bbox.size.y),bbox.size.z)*0.4f &&
					szmin0 < max(max(bbox1.size.x,bbox1.size.y),bbox1.size.z)*1.5f &&
					++((CTriMesh*)pGeom)->m_nMessyCutCount>=5)
				return 0;
		}
#ifdef _DEBUG1
		static CTriMesh *g_pPrevGeom = 0;
		if (m_vars.iDrawHelpers & 0x4000)
			pent->m_parts[i].pPhysGeomProxy->pGeom = pGeom = g_pPrevGeom;
		else if (g_pPrevGeom) 
			g_pPrevGeom->Release();
		(g_pPrevGeom = new CTriMesh())->Clone((CTriMesh*)pGeom,0);
		g_pPrevGeom->RebuildBVTree(((CTriMesh*)pGeom)->m_pTree);
#endif
		WriteLockPlatf1 lockp1(m_lockPotentialMeshUpdate);
		if (pGeom->Subtract(pHole, gwd,gwd1)) {
			if (pNewGeom)	
				ClonePhysGeomInEntity(pent,i,pNewGeom);
			if (pent->m_parts[i].pLattice)
				pent->m_parts[i].pLattice->Subtract(pHole, gwd,gwd1);
			(epum.pMesh = pGeom)->Lock();
			epum.pLastUpdate = (bop_meshupdate*)epum.pMesh->GetForeignData(DATA_MESHUPDATE);
			for(; epum.pLastUpdate && epum.pLastUpdate->next; epum.pLastUpdate=epum.pLastUpdate->next);
			epum.pMesh->Unlock();
			OnEvent(m_vars.bLogStructureChanges+1,&epum);
			pent->m_parts[i].flags |= geom_structure_changes|(geom_constraint_on_break & -bCreateConstraint); 
			return 1;
		}	else if (pNewGeom)
			pNewGeom->Release();
	}
	return 0;
}


void CPhysicalWorld::MarkEntityAsDeforming(CPhysicalEntity *pent)
{
	if (!(pent->m_flags & pef_deforming)) {
		WriteLock lock(m_lockDeformingEntsList);
		pent->m_flags |= pef_deforming;
		if (m_nDeformingEnts==m_nDeformingEntsAlloc)
			ReallocateList(m_pDeformingEnts, m_nDeformingEnts,m_nDeformingEntsAlloc+=16);
		m_pDeformingEnts[m_nDeformingEnts++] = pent;
	}
}

void CPhysicalWorld::UnmarkEntityAsDeforming(CPhysicalEntity *pent)
{
	if (pent->m_flags & pef_deforming) {
		WriteLock lock(m_lockDeformingEntsList);
		pent->m_flags &= ~pef_deforming;
		int i; for(i=m_nDeformingEnts-1; i>=0 && m_pDeformingEnts[i]!=pent;i--);
		if (i>=0)
			m_pDeformingEnts[i] = m_pDeformingEnts[--m_nDeformingEnts];
	}
}


void CPhysicalWorld::SimulateExplosion(pe_explosion *pexpl, IPhysicalEntity **pSkipEnts,int nSkipEnts, int iTypes, int iCaller)
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );
	
	CPhysicalEntity **pents;
	int nents,nents1,i,j,bBreak,bEntChanged;
	RigidBody *pbody;
	float kr=pexpl->impulsivePressureAtR*sqr(pexpl->r),maxspeed=15,E,frac=1.0f,sumFrac,sumV,Minv;
	Vec3 gravity;
	pe_params_buoyancy pb;
	geom_world_data gwd,gwd1;
	box bboxPart,bbox;
	sphere sphExpl;
	CPhysicalPlaceholder **pSkipPcs = (CPhysicalPlaceholder**)pSkipEnts;
	pe_action_impulse shockwave;
	shockwave.iApplyTime = 2;
	shockwave.iSource = 2;
	EventPhysCollision epc;
	epc.pEntity[0]=&g_StaticPhysicalEntity; epc.pForeignData[0]=0; epc.iForeignData[0]=0;
	epc.vloc[1].zero(); epc.mass[0] = 1E10f;
	epc.partid[0]=0; epc.idmat[0]=0; epc.penetration=epc.radius=0;
	if (!CheckAreas(pexpl->epicenter,gravity,&pb,1,Vec3(ZERO),0,iCaller) || is_unused(gravity))
		gravity = m_vars.gravity;	
	WriteLock lock(m_lockCaller[iCaller]);
	bboxPart.bOriented = 0;
	bboxPart.Basis.SetIdentity();
	sphExpl.center = pexpl->epicenter;
	sphExpl.r = pexpl->rmax;
	if(pexpl->rmin < FLT_EPSILON)
		pexpl->rmin = 0.1f;
	if (m_vars.bDebugExplosions)
		m_vars.bSingleStepMode = 1;

	for(i=0;i<nSkipEnts;i++)
		AtomicAdd(&((!pSkipPcs[i]->m_pEntBuddy || IsPlaceholder(pSkipPcs[i])) ? pSkipPcs[i] : pSkipPcs[i]->m_pEntBuddy)->m_bProcessed,1<<iCaller);
#ifdef _DEBUG
	if (m_vars.iDrawHelpers & 0x4000)	{
pexpl->epicenter = m_lastEpicenter;
pexpl->epicenterImp = m_lastEpicenterImp;
pexpl->explDir = m_lastExplDir;	}
#endif
	if (pexpl->holeSize>0) {
		gwd1.R.SetRotationV0V1(Vec3(0,1,0),pexpl->explDir);
		gwd1.offset = pexpl->epicenter;
	}

	if (pexpl->nOccRes>0) {
		if (pexpl->nOccRes>m_nOccRes) for(i=0;i<6;i++) {
			if (m_nOccRes) {
				delete[] m_pGridStat[i]; delete[] m_pGridDyn[i];
			}
			m_pGridStat[i] = new int[sqr(pexpl->nOccRes)];
			m_pGridDyn[i] = new int[sqr(pexpl->nOccRes)];
		}
		for(i=0;i<6;i++) for(j=sqr(pexpl->nOccRes)-1;j>=0;j--) 
			m_pGridStat[i][j] = (1u<<31)-1;
		m_lastEpicenter = pexpl->epicenter;
		m_lastEpicenterImp = pexpl->epicenterImp;
		m_lastRmax = pexpl->rmax;
		m_lastExplDir = pexpl->explDir;
	}
	
	if (pexpl->nOccRes>0 || pexpl->holeSize>0)
		for(nents=GetEntitiesAround(pexpl->epicenter-Vec3(1,1,1)*pexpl->rmax,pexpl->epicenter+Vec3(1,1,1)*pexpl->rmax,pents,
				ent_terrain|ent_static|ent_rigid,0,0,iCaller)-1; nents>=0; nents--)
		if (pents[nents]->m_iSimClass<1 || pents[nents]->GetMassInv()<=0)
		{ { WriteLock lock0(pents[nents]->m_lockUpdate);
				for(i=bEntChanged=0; i<pents[nents]->m_nParts; i++) 
				if (pents[nents]->m_parts[i].flags & geom_colltype_explosion && 
						(pents[nents]->m_nParts<=1 || 
						 (bboxPart.center=(pents[nents]->m_parts[i].BBox[1]+pents[nents]->m_parts[i].BBox[0])*0.5f,
						  bboxPart.size  =(pents[nents]->m_parts[i].BBox[1]-pents[nents]->m_parts[i].BBox[0])*0.5f,
						  box_sphere_overlap_check(&bboxPart,&sphExpl)))) 
				{
					bBreak = (m_vars.breakImpulseScale || pents[nents]->m_flags & pef_override_impulse_scale && !m_vars.bMultiplayer || pexpl->forceDeformEntities)
						&& iTypes & 1<<pents[nents]->m_iSimClass && 
						pexpl->holeSize>0 && pents[nents]->m_parts[i].idmatBreakable>=0 && !(pents[nents]->m_parts[i].flags & geom_manually_breakable);
					if (pexpl->nOccRes<=0 && !bBreak)
						continue;
					gwd.R = Matrix33(pents[nents]->m_qrot*pents[nents]->m_parts[i].q);
					gwd.offset = pents[nents]->m_pos + pents[nents]->m_qrot*pents[nents]->m_parts[i].pos - pexpl->epicenter;
					gwd.scale = pents[nents]->m_parts[i].scale;
					if (pexpl->nOccRes>0 &&
							(!(pents[nents]->m_parts[i].flags & geom_manually_breakable) || 
							 (pents[nents]->m_parts[i].pPhysGeomProxy->pGeom->GetBBox(&bbox), min(min(bbox.size.x,bbox.size.y),bbox.size.z)>pexpl->rminOcc)))
						pents[nents]->m_parts[i].pPhysGeomProxy->pGeom->BuildOcclusionCubemap(&gwd, 0, m_pGridStat,m_pGridDyn, 
							pexpl->nOccRes, pexpl->rminOcc,pexpl->rmax,pexpl->nGrow);
					if (bBreak) {
						gwd.offset += pexpl->epicenter;
						if (!(iTypes & ent_delayed_deformations)) 
							bEntChanged += DeformEntityPart(pents[nents],i, pexpl, &gwd,&gwd1);
						else {
							WriteLock lockbq(m_lockBreakQueue);
							ReallocQueue(m_breakQueue, m_breakQueueSz,m_breakQueueAlloc, m_breakQueueHead,m_breakQueueTail, 4);
							(m_breakQueue[m_breakQueueHead].pent = pents[nents])->AddRef();
							m_breakQueue[m_breakQueueHead].partid = pents[nents]->m_parts[i].id;
							m_breakQueue[m_breakQueueHead].expl = *pexpl;
							m_breakQueue[m_breakQueueHead].gwd[0] = gwd;
							m_breakQueue[m_breakQueueHead].gwd[1] = gwd1;
							m_breakQueue[m_breakQueueHead].gravity = gravity;
							m_breakQueueSz++;
						}
					}
				}
			}
			if (bEntChanged && pents[nents]->UpdateStructure(0.01f,pexpl,-1,gravity))
				MarkEntityAsDeforming(pents[nents]);
		}
	if (pexpl->nOccRes>=0)
		m_nOccRes = pexpl->nOccRes;

	nents = GetEntitiesAround(pexpl->epicenter-Vec3(1,1,1)*pexpl->rmax,pexpl->epicenter+Vec3(1,1,1)*pexpl->rmax,pents, iTypes, 0,0,iCaller);
	if (pexpl->nOccRes<0 && m_nOccRes>=0) {
		// special case: reuse the previous m_pGridStat and process only entities that were not affected by the previous call
		for(i=nents1=0;i<nents;i++) {
			for(j=0;j<m_nExplVictims && m_pExplVictims[j]!=pents[i];j++);
			if (j==m_nExplVictims)
				pents[nents1++] = pents[i];
		}
		pexpl->nOccRes=m_nOccRes; nents=nents1;
	}
	if (m_nExplVictimsAlloc<nents) {
		if (m_nExplVictimsAlloc) {
			delete[] m_pExplVictims; delete[] m_pExplVictimsFrac; delete[] m_pExplVictimsImp;
		}
		m_pExplVictims = new CPhysicalEntity*[m_nExplVictimsAlloc=nents];
		m_pExplVictimsFrac = new float[m_nExplVictimsAlloc];
		m_pExplVictimsImp = new Vec3[m_nExplVictimsAlloc];
	}

	ReadLockPlatf1 lockp0(m_lockPotentialPartListUpdate);
	for(nents--,m_nExplVictims=0; nents>=0; nents--) {
		{ ReadLock lock0(pents[nents]->m_lockUpdate);
			m_pExplVictimsImp[m_nExplVictims].zero();
			for(i=bEntChanged=0,sumFrac=sumV=0.0f; i<pents[nents]->m_nParts; i++) 
			if ((pents[nents]->m_parts[i].flags & geom_colltype_explosion || pents[nents]->m_flags & pef_use_geom_callbacks) &&
					(pents[nents]->m_nParts<=1 || 
					 (bboxPart.center=(pents[nents]->m_parts[i].BBox[1]+pents[nents]->m_parts[i].BBox[0])*0.5f,
					  bboxPart.size  =(pents[nents]->m_parts[i].BBox[1]-pents[nents]->m_parts[i].BBox[0])*0.5f,
					  box_sphere_overlap_check(&bboxPart,&sphExpl)))) 
			{
				bBreak = pents[nents]->GetMassInv()>0;//m_iSimClass>0 || pents[nents]->m_parts[i].flags & geom_monitor_contacts) && 
								 //!(pents[nents]->m_parts[i].flags & geom_manually_breakable);

				if (bBreak || pents[nents]->m_parts[i].flags & (geom_monitor_contacts|geom_manually_breakable)) {
					gwd.R = Matrix33(pents[nents]->m_qrot*pents[nents]->m_parts[i].q);
					gwd.offset = pents[nents]->m_pos + pents[nents]->m_qrot*pents[nents]->m_parts[i].pos;
					gwd.scale = pents[nents]->m_parts[i].scale;

					if (pexpl->nOccRes>0) {
						gwd.offset -= pexpl->epicenter;
						frac = pents[nents]->m_parts[i].pPhysGeomProxy->pGeom->BuildOcclusionCubemap(&gwd, 1, m_pGridStat,m_pGridDyn,pexpl->nOccRes, 
							pexpl->rminOcc,m_lastRmax,pexpl->nGrow);
						gwd.offset += pexpl->epicenter;
						sumFrac += pents[nents]->m_parts[i].pPhysGeomProxy->V*frac;
						sumV += pents[nents]->m_parts[i].pPhysGeomProxy->V;
					}

					if (bBreak)
						if (pexpl->holeSize>0 && pents[nents]->m_parts[i].idmatBreakable>=0)
							bEntChanged += DeformEntityPart(pents[nents],i, pexpl, &gwd,&gwd1);

					if (kr>0) {
						if (!(pents[nents]->m_flags & pef_use_geom_callbacks)) {
							shockwave.impulse.zero(); shockwave.angImpulse.zero();
							pbody = pents[nents]->GetRigidBody(i);
							Minv = pents[nents]->GetMassInv();
							pents[nents]->m_parts[i].pPhysGeomProxy->pGeom->CalcVolumetricPressure(&gwd, pexpl->epicenterImp,kr,pexpl->rmin, pbody->pos, 
								shockwave.impulse,shockwave.angImpulse);
							shockwave.impulse *= frac; shockwave.angImpulse *= frac;
							shockwave.ipart = i;
							if ((E=shockwave.impulse.len2()*sqr(Minv))>sqr(maxspeed))
								shockwave.impulse *= sqrt_tpl(sqr(maxspeed)/E);
							if ((E=shockwave.angImpulse*(pbody->Iinv*shockwave.angImpulse)*Minv)>sqr(maxspeed))
								shockwave.angImpulse *= sqrt_tpl(sqr(maxspeed)/E);
							if (pents[nents]->m_pStructure && pents[nents]->m_pStructure->defparts && pents[nents]->m_pStructure->defparts[i].pSkelEnt) {
								pe_params_pos ppos; 
								ppos.pos = pents[nents]->m_pos; ppos.q = pents[nents]->m_qrot;
								pents[nents]->m_pStructure->defparts[i].pSkelEnt->SetParams(&ppos); 
								pents[nents]->m_pStructure->defparts[i].pSkelEnt->ApplyVolumetricPressure(pexpl->epicenterImp,kr*frac,pexpl->rmin);
								MarkEntityAsDeforming(pents[nents]);
							}
							pents[nents]->Action(&shockwave);
							m_pExplVictimsImp[m_nExplVictims] += shockwave.impulse;
						} else
							pents[nents]->ApplyVolumetricPressure(pexpl->epicenterImp,kr*frac,pexpl->rmin);
					}
				} else if (pents[nents]->m_flags & pef_use_geom_callbacks)
					pents[nents]->ApplyVolumetricPressure(pexpl->epicenterImp,kr*frac,pexpl->rmin);

				if ((pents[nents]->m_parts[i].flags & (geom_manually_breakable|geom_structure_changes))==geom_manually_breakable && 
						(pexpl->nOccRes==0 || sumFrac>0))	
				{
					int iprim,ifeat,ncont,bMultipart;
					Vec3 ptdst[2];
					CBoxGeom boxGeom;
					intersection_params ip;
					geom_contact *pcontacts;
					float rscale;
					mesh_data *pmd=0;
					pents[nents]->m_parts[i].pPhysGeomProxy->pGeom->GetBBox(&bbox);

					if (pents[nents]->m_parts[i].idmatBreakable>=0 || pents[nents]->m_parts[i].flags & geom_break_approximation ||
							pents[nents]->m_parts[i].pPhysGeomProxy->pGeom->GetPrimitiveCount()<=1 || 
							!(pmd=(mesh_data*)pents[nents]->m_parts[i].pPhysGeomProxy->pGeom->GetData()) || pmd->nIslands<=1)
					{
						if (!pmd || !pmd->pMats)
							epc.idmat[1] = pents[nents]->GetMatId(-1,i);
						else for(iprim=0; iprim<pmd->nTris && 
							!(m_SurfaceFlagsTable[epc.idmat[1]=pents[nents]->GetMatId(pmd->pMats[iprim],i)] & sf_manually_breakable); iprim++);
						epc.n.zero(); bMultipart=0;
						goto single_island;
					} else {
						for(j=0;j<pmd->nIslands;j++) if (((ptdst[0]=gwd.R*pmd->pIslands[j].center*gwd.scale+gwd.offset)-sphExpl.center).len2()<sqr(sphExpl.r*2)) {
							epc.n.zero(); bMultipart=1;
							if (!pmd || !pmd->pMats)
								epc.idmat[1] = pents[nents]->GetMatId(-1,i);
							else {
								Vec3 BBox[2] = { pmd->pIslands[j].center, pmd->pIslands[j].center };
								for(iprim=pmd->pIslands[j].itri,epc.idmat[1]=-1; iprim<pmd->nTris && pmd->pMats[iprim]>=0; iprim=pmd->pTri2Island[iprim].inext) {
									int idmat = pents[nents]->GetMatId(pmd->pMats[iprim],i);
									epc.idmat[1] += idmat-epc.idmat[1] & -((int)m_SurfaceFlagsTable[idmat] & sf_manually_breakable)>>31;
									for(int ivtx=0;ivtx<3;ivtx++) {
										Vec3 vtx = pmd->pVertices[pmd->pIndices[iprim*3+ivtx]];
										BBox[0]=min(BBox[0],vtx); BBox[1]=max(BBox[1],vtx);
									}
								}
								if (iprim<pmd->nTris || epc.idmat[1]<0)
									continue;
								epc.n[idxmin3(bbox.size = BBox[1]-BBox[0])] = 1.0f;
								epc.n = gwd.R*epc.n;
								epc.vloc[0] = -(epc.n *= sgnnz(epc.n*(sphExpl.center-ptdst[0])));
							}
							goto post_event;

					single_island:
					boxGeom.CreateBox(&bbox);
					if (boxGeom.FindClosestPoint(&gwd, iprim,ifeat, pexpl->epicenter,pexpl->epicenter, ptdst, 1)<0 ||
							(pexpl->epicenter-ptdst[0])*(ptdst[0]-gwd.offset-gwd.R*bbox.center*gwd.scale)<0)
						ptdst[0] = pexpl->epicenter;
					if (pents[nents]->m_parts[i].idmatBreakable>=0) {
						if ((ptdst[0]-ptdst[1]).len2() > sqr(pexpl->rmin*0.7f+pexpl->rmax*0.3f))
							goto next_part;
						j = idxmax3(bbox.size);
						rscale = gwd.scale==1.0f ? 1.0f:1.0f/gwd.scale;
						ptdst[1].z = (bbox.Basis.GetRow(j)*((ptdst[0]-gwd.offset)*gwd.R-bbox.center))*rscale;
						ptdst[1].z = max(-bbox.size[j]*0.8f, min(bbox.size[j]*0.8f, ptdst[1].z));
						bbox.center += bbox.Basis.GetRow(j)*ptdst[1].z;
						bbox.size[inc_mod3[j]]*=1.01f; bbox.size[dec_mod3[j]]*=1.01f; bbox.size[j]*=0.002f;
						boxGeom.CreateBox(&bbox);
						if (ncont=pents[nents]->m_parts[i].pPhysGeomProxy->pGeom->Intersect(&boxGeom, 0,0,&ip, pcontacts)) {
							WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive();
							ptdst[0].Set(1E10f,1E10f,1E10f);
							ptdst[1] = ((pexpl->epicenter-gwd.offset)*gwd.R)*rscale;
							float dist,mindist=1E10f;
							for(ncont--;ncont>=0;ncont--) for(j=0;j<pcontacts[ncont].nborderpt;j++) {
								Vec3 vtx0 = pcontacts[ncont].ptborder[j], 
										 vtx1 = pcontacts[ncont].ptborder[j+1-(pcontacts[ncont].nborderpt & pcontacts[ncont].nborderpt-j-2<<31)];
								if ((dist=(vtx0-ptdst[1]).len2()) < mindist)
									epc.n = pents[nents]->m_parts[i].pPhysGeomProxy->pGeom->GetNormal(pcontacts[ncont].idxborder[j][0] & IDXMASK,	
									ptdst[0]=vtx0), mindist=dist;
								float proj=(ptdst[1]-vtx0)*(vtx1-vtx0), edgelen=(vtx1-vtx0).len2();
								if (inrange(proj,0.0f,edgelen) && (dist=(ptdst[1]-vtx0^vtx1-vtx0).len2())<mindist*edgelen)	{
									mindist = dist*(edgelen = 1.0f/edgelen);
									ptdst[0] = vtx0+(vtx1-vtx0)*(proj*edgelen);
									epc.n = pents[nents]->m_parts[i].pPhysGeomProxy->pGeom->GetNormal(pcontacts[ncont].idxborder[j][0] & IDXMASK,	ptdst[0]);
								}
							}
							ptdst[0] = gwd.R*ptdst[0]*gwd.scale+gwd.offset;
							epc.vloc[0] = -(epc.n = gwd.R*epc.n);
						}
					}
					post_event:
					epc.pt = ptdst[0];
					epc.pEntity[1]=pents[nents]; epc.pForeignData[1]=pents[nents]->m_pForeignData; epc.iForeignData[1]=pents[nents]->m_iForeignData;
					if ((epc.pt-pexpl->epicenter).len2()<sqr(pexpl->holeSize*0.2f)*(1-bMultipart)) {
						epc.pt = pexpl->epicenter;
						if (!epc.n.len2())
							epc.n = -(epc.vloc[0] = pexpl->explDir);
					}	else if (!epc.n.len2())
						epc.vloc[0] = -(epc.n = (pexpl->epicenter-epc.pt).normalized());
					epc.mass[0] = bbox.size.x*bbox.size.y+bbox.size.x*bbox.size.z+bbox.size.y*bbox.size.z;
					epc.vloc[0] *= kr/max(sqr(pexpl->rmin), (pexpl->epicenter-epc.pt).len2());
					epc.vloc[0] *= epc.mass[0]*100.0f;
					epc.mass[0] = 0.01f;
					epc.mass[1] = pents[nents]->GetMass(i);
					epc.partid[1] = pents[nents]->m_parts[i].id;
					epc.idmat[0] = -1;
					epc.normImpulse=epc.penetration = 0;
					epc.radius = pexpl->rmax;
					pents[nents]->m_parts[i].flags |= geom_will_be_destroyed;
					
					OnEvent(pef_log_collisions, &epc);
					if (!bMultipart)
						break;
					}} // multi-island loop end
					next_part:;
				}
			}
		}
		if (pents[nents]->m_nParts==0)
			pents[nents]->ApplyVolumetricPressure(pexpl->epicenterImp,kr,pexpl->rmin);

		m_pExplVictims[m_nExplVictims] = pents[nents];
		m_pExplVictimsFrac[m_nExplVictims++] = sumV>0 ? sumFrac/sumV : 1.0f;
		if (bEntChanged && pents[nents]->UpdateStructure(0.01f,pexpl,-1,gravity))
			MarkEntityAsDeforming(pents[nents]);
	}
	pexpl->pAffectedEnts = (IPhysicalEntity**)m_pExplVictims;
	pexpl->pAffectedEntsExposure = m_pExplVictimsFrac;
	pexpl->nAffectedEnts = m_nExplVictims;

	for(i=0;i<nSkipEnts;i++)
		AtomicAdd(&((!pSkipPcs[i]->m_pEntBuddy || IsPlaceholder(pSkipPcs[i])) ? pSkipPcs[i] : pSkipPcs[i]->m_pEntBuddy)->m_bProcessed,-(1<<iCaller));
}


float CPhysicalWorld::CalculateExplosionExposure(pe_explosion *pexpl, IPhysicalEntity *pient)
{
	if (pexpl->nOccRes<=0)
		return 1.0f;
	if (pient->GetType()==PE_AREA)
		return 0.0f;

	CPhysicalEntity *pent = (CPhysicalEntity*)pient;
	ReadLock lock(pent->m_lockUpdate);
	int i;
	float sumV,sumFrac,frac;
	geom_world_data gwd;

	for(i=0,sumFrac=sumV=0.0f; i<pent->m_nParts; i++) if (pent->m_parts[i].flags & geom_colltype_explosion) {
		gwd.R = Matrix33(pent->m_qrot*pent->m_parts[i].q);
		gwd.offset = pent->m_pos + pent->m_qrot*pent->m_parts[i].pos - pexpl->epicenter;
		gwd.scale = pent->m_parts[i].scale;
		frac = pent->m_parts[i].pPhysGeomProxy->pGeom->BuildOcclusionCubemap(&gwd, 1, m_pGridStat,m_pGridDyn,pexpl->nOccRes, 
			pexpl->rminOcc,m_lastRmax,pexpl->nGrow);
		sumFrac += pent->m_parts[i].pPhysGeomProxy->V*frac;
		sumV += pent->m_parts[i].pPhysGeomProxy->V;
	}

	return sumV>0 ? sumFrac/sumV : 1.0f;
}


void CPhysicalWorld::ResetDynamicEntities()
{
	int i; CPhysicalEntity *pent;
	WriteLock lock(m_lockStep);
	pe_action_reset reset;
	for(i=1;i<=4;i++) for(pent=m_pTypedEnts[i]; pent; pent=pent->m_next)
		pent->Action(&reset);
}


void CPhysicalWorld::DestroyDynamicEntities()
{
	int i; CPhysicalEntity *pent,*pent_next;

	m_nDynamicEntitiesDeleted = 0;
	for(i=1;i<=4;i++) {
		for(pent=m_pTypedEnts[i]; pent; pent=pent_next) {
			pent_next = pent->m_next;
			if (pent->m_pEntBuddy) {
				pent->m_pEntBuddy->m_pEntBuddy = 0;
				DestroyPhysicalEntity(pent->m_pEntBuddy);
			}	else
				SetPhysicalEntityId(pent,-1);
			DetachEntityGridThunks(pent);
			for(int j=0;j<pent->m_nParts;j++) if (pent->m_parts[j].pPlaceholder)
				DetachEntityGridThunks(pent->m_parts[j].pPlaceholder);
			if (pent->m_next = m_pTypedEnts[7]) 
				pent->m_next->m_prev = pent;
			m_pTypedEnts[7] = pent;	
			pent->m_iPrevSimClass = -1; pent->m_iSimClass = 7;
			m_nDynamicEntitiesDeleted++;
		}
		m_pTypedEnts[i] = m_pTypedEntsPerm[i] = 0;
	}

	m_nEnts -= m_nDynamicEntitiesDeleted;
	if (m_nEnts < m_nEntsAlloc-8192 && !m_bEntityCountReserved) {
		int nEntsAlloc = m_nEntsAlloc;
		m_nEntsAlloc = (m_nEnts-1&~8191)+8192; m_nEntListAllocs++;
		ReallocateList(m_pTmpEntList,nEntsAlloc,m_nEntsAlloc);
		ReallocateList(m_pTmpEntList1,nEntsAlloc,m_nEntsAlloc);
		ReallocateList(m_pTmpEntList2,nEntsAlloc,m_nEntsAlloc);
		ReallocateList(m_pGroupMass,0,m_nEntsAlloc);
		ReallocateList(m_pMassList,0,m_nEntsAlloc);
		ReallocateList(m_pGroupIds,0,m_nEntsAlloc);
		ReallocateList(m_pGroupNums,0,m_nEntsAlloc);
	}
}

void CPhysicalWorld::PurgeDeletedEntities()
{
	int i,j;
	{ WriteLock lock1(m_lockQueue);
		for(i=0; i<m_nQueueSlots; i++) for(j=0; *(int*)(m_pQueueSlots[i]+j)!=-1; j+=*(int*)(m_pQueueSlots[i]+j+sizeof(int)))
			if (*(int*)(m_pQueueSlots[i]+j)!=-2 && (*(CPhysicalEntity**)(m_pQueueSlots[i]+j+sizeof(int)*2))->m_iSimClass==7)
				*(int*)(m_pQueueSlots[i]+j) = -2;
	}

	{ WriteLock lock3(m_lockDeformingEntsList);
		for(i=j=0; i<m_nDeformingEnts; i++) if (m_pDeformingEnts[i]->m_iSimClass!=7)
			m_pDeformingEnts[j++] = m_pDeformingEnts[i];
		else 
			m_pDeformingEnts[i]->m_flags &= ~pef_deforming;
		m_nDeformingEnts = j;
	}

	TracePendingRays(0);

	WriteLock lock(m_lockStep);
	CleanseEventsQueue();
	CPhysicalEntity *pent,*pent_next;
	/*for(pent=m_pTypedEnts[7]; pent; pent=pent_next) { // purge deletion requests
		pent_next = pent->m_next; delete pent;
	}
	m_pTypedEnts[7] = 0;*/
	for(pent=m_pTypedEnts[7]; pent; pent=pent_next) { // purge deletion requests
		pent_next = pent->m_next; 
		if (pent->m_nRefCount<=0)	{
			if (pent->m_next) pent->m_next->m_prev = pent->m_prev;
			(pent->m_prev ? pent->m_prev->m_next : m_pTypedEnts[7]) = pent->m_next;
			delete pent; 
		}
	}
}


void CPhysicalWorld::DrawPhysicsHelperInformation(IPhysRenderer *pRenderer, int iCaller)
{
	int entype; CPhysicalEntity *pent=0;
	(m_pRenderer = pRenderer)->SetOffset(m_vars.helperOffset);
	
	if (m_vars.iDrawHelpers) {
		assert(iCaller<=MAX_PHYS_THREADS);
		int i,n=0,nEntListAllocs,nGEA;
		CPhysicalEntity **pEntList;
		{ WriteLock lock0(m_lockCaller[iCaller]);
			if (m_pHeightfield[iCaller] && m_vars.iDrawHelpers & 128)
				pRenderer->DrawGeometry(m_pHeightfield[iCaller]->m_parts[0].pPhysGeom->pGeom,0,0);
			nEntListAllocs=m_nEntListAllocs; nGEA=m_nGEA[iCaller];

			pEntList = iCaller ? m_pTmpEntList2:m_pTmpEntList;
			{ ReadLock lock(m_lockList);
				for(entype=0;entype<=6;entype++) if (m_vars.iDrawHelpers & 0x100<<entype)
				for(pent=m_pTypedEnts[entype]; pent && nEntListAllocs==m_nEntListAllocs && nGEA==m_nGEA[iCaller]; pent=pent->m_next)
					pEntList[n++] = (CPhysicalEntity*)pent->m_id;
				if (pent)
					return;
			}
		}
		for(i=0;i<n;i++) {
			int id = *(int*)(pEntList+i);
			if (nEntListAllocs!=m_nEntListAllocs || nGEA!=m_nGEA[iCaller])
				break;
			if ((pent=(CPhysicalEntity*)GetPhysicalEntityById(id|1<<30)) && m_vars.iDrawHelpers & 0x80<<pent->m_iSimClass+1)
				pent->DrawHelperInformation(pRenderer, m_vars.iDrawHelpers);
		}
	}

	if (m_vars.iDrawHelpers & 8192 && m_nOccRes) {
		float zscale,xscale,/*xoffs,*/z;
		int i,ix,iy,cx,cy,cz;
		Vec3 pt0,pt1;
		zscale = m_lastRmax*(1.0/65535.0f);
		xscale = 2.0f/m_nOccRes;
//		xoffs = 1.0f-xscale;
		for(i=0;i<6;i++) {
			cz=i>>1; cx=inc_mod3[cz]; cy=dec_mod3[cz];
			for(iy=0;iy<m_nOccRes;iy++) for(ix=0;ix<m_nOccRes;ix++) if (m_pGridStat[i][iy*m_nOccRes+ix]<(1u<<31)-1) {
				pt0[cz] = (z=m_pGridStat[i][iy*m_nOccRes+ix]*zscale)*((i&1)*2-1);
				pt0[cx] = ((ix+0.5f)*xscale-1.0f)*z;
				pt0[cy] = ((iy+0.5f)*xscale-1.0f)*z;
				pt0 += m_lastEpicenter;
				pRenderer->DrawLine(m_lastEpicenter,pt0,7);
				pt0[cx] -= z*xscale*0.5f; pt0[cy] -= z*xscale*0.5f;
				pt1=pt0; pt1[cx] += z*xscale; pt1[cy] += z*xscale;
				pRenderer->DrawLine(pt0,pt1,7);
				pt0[cy] += z*xscale; pt1[cy] -= z*xscale;
				pRenderer->DrawLine(pt0,pt1,7);
			}
		}
	}

	if (m_vars.iDrawHelpers & 32)	{
		ReadLock lock(m_lockAreas);
		for(CPhysArea *pArea=m_pGlobalArea; pArea; pArea=pArea->m_next)
			pArea->DrawHelperInformation(pRenderer, m_vars.iDrawHelpers);
	}

	if (m_vars.iDrawHelpers & 32768 && m_pWaterMan)
		m_pWaterMan->DrawHelpers(pRenderer);

	if (m_vars.bLogActiveObjects) {
		ReadLock lock(m_lockList);
		m_vars.bLogActiveObjects = 0;
		int i,nPrims,nCount=0;
		RigidBody *pbody;
		for(pent=m_pTypedEnts[2]; pent; pent=pent->m_next) if (pent->GetMassInv()>0) {
			for(i=nPrims=0;i<pent->m_nParts;i++) if (pent->m_parts[i].flags & geom_colltype0)
				nPrims += ((CGeometry*)pent->m_parts[i].pPhysGeomProxy->pGeom)->GetPrimitiveCount();
			pbody = pent->GetRigidBody();	++nCount;
			CryLogAlways("%s @ %7.2f,%7.2f,%7.2f, mass %.2f, v %.1f, w %.1f, #polies %d, id %d",
				m_pRenderer ? m_pRenderer->GetForeignName(pent->m_pForeignData,pent->m_iForeignData,pent->m_iForeignFlags):"",
				pent->m_pos.x,pent->m_pos.y,pent->m_pos.z, pbody->M,pbody->v.len(),pbody->w.len(),nPrims,pent->m_id);
		}
		CryLogAlways("%d active object(s)",nCount);
	}
}


int CPhysicalWorld::CollideEntityWithBeam(IPhysicalEntity *_pent, Vec3 org,Vec3 dir,float r, ray_hit *phit)
{
	if (!_pent)
		return 0;
	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );

	CPhysicalEntity *pent = (CPhysicalEntity*)_pent;
	ReadLock lock(pent->m_lockUpdate);
	CSphereGeom SweptSph;
	geom_contact *pcontacts;
	geom_world_data gwd[2];
	sphere asph;
	asph.r = r;
	asph.center.zero();
	SweptSph.CreateSphere(&asph);
	intersection_params ip;
	ip.bSweepTest = dir.len2()>0;
	gwd[0].R.SetIdentity();
	gwd[0].offset = org;
	gwd[0].v = dir;
	ip.time_interval = 1.0f;
	phit->dist = 1E10;

	for(int i=0;i<pent->m_nParts;i++) if (pent->m_parts[i].flags & geom_collides) {
		gwd[1].offset = pent->m_pos + pent->m_qrot*pent->m_parts[i].pos;
		gwd[1].R = Matrix33(pent->m_qrot*pent->m_parts[i].q);
		gwd[1].scale = pent->m_parts[i].scale;
		if (SweptSph.Intersect(pent->m_parts[i].pPhysGeom->pGeom, gwd,gwd+1, &ip, pcontacts)) {
			WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive();
			if (pcontacts->t<phit->dist) {
				phit->dist = pcontacts->t;
				phit->pCollider = pent;
				phit->partid = pent->m_parts[phit->ipart=i].id;
				phit->surface_idx = pent->GetMatId(pcontacts->id[1],i);
				phit->idmatOrg = pcontacts->id[1] + (pent->m_parts[i].surface_idx+1 & pcontacts->id[1]>>31);
				phit->foreignIdx = pent->m_parts[i].pPhysGeom->pGeom->GetForeignIdx(pcontacts->iPrim[1]);
				phit->pt = pcontacts->pt;
				phit->n = -pcontacts->n;
			}
		}
	}

	return isneg(phit->dist-1E9f);
}

int CPhysicalWorld::CollideEntityWithPrimitive(IPhysicalEntity *_pent, int itype, primitive *pprim, Vec3 dir, ray_hit *phit)
{
	if (!_pent || ((CPhysicalPlaceholder*)_pent)->m_iSimClass==5)
		return 0;
	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );

	CPhysicalEntity *pent = (CPhysicalEntity*)_pent;
	ReadLock lock(pent->m_lockUpdate);
	geom_contact *pcontacts;
	geom_world_data gwd[2];
	CBoxGeom gbox;
	CCylinderGeom gcyl;
	CCapsuleGeom gcaps;
	CSphereGeom gsph;
	CGeometry *pgeom;
	intersection_params ip;
	ip.bSweepTest = dir.len2()>0;
	gwd[0].R.SetIdentity();
	gwd[0].offset.zero();
	gwd[0].v = dir;
	ip.time_interval = 1.0f;
	phit->dist = 1E10;

	switch (itype) {
		case box::type:			 gwd[0].offset=((box*)pprim)->center; ((box*)pprim)->center.zero(); gbox.CreateBox((box*)pprim); 
												 pgeom=&gbox; ((box*)pprim)->center=gwd[0].offset; break;
		case cylinder::type: gwd[0].offset=((cylinder*)pprim)->center; ((cylinder*)pprim)->center.zero(); gcyl.CreateCylinder((cylinder*)pprim); 
												 pgeom=&gcyl; ((cylinder*)pprim)->center=gwd[0].offset; break;
		case capsule::type:  gwd[0].offset=((capsule*)pprim)->center; ((capsule*)pprim)->center.zero(); gcaps.CreateCapsule((capsule*)pprim); 
												 pgeom=&gcaps; ((capsule*)pprim)->center=gwd[0].offset; break;
		case sphere::type:	 gwd[0].offset=((sphere*)pprim)->center; ((sphere*)pprim)->center.zero(); gsph.CreateSphere((sphere*)pprim); 
												 pgeom=&gsph; ((sphere*)pprim)->center=gwd[0].offset; break;
		default: return 0;
	}

	for(int i=0;i<pent->m_nParts;i++) if (pent->m_parts[i].flags & geom_collides) {
		gwd[1].offset = pent->m_pos + pent->m_qrot*pent->m_parts[i].pos;
		gwd[1].R = Matrix33(pent->m_qrot*pent->m_parts[i].q);
		gwd[1].scale = pent->m_parts[i].scale;
		if (pgeom->Intersect(pent->m_parts[i].pPhysGeom->pGeom, gwd,gwd+1, &ip, pcontacts)) {
			WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive();
			if (pcontacts->t<phit->dist) {
				phit->dist = pcontacts->t;
				phit->pCollider = pent;
				phit->partid = pent->m_parts[phit->ipart=i].id;
				phit->surface_idx = pent->GetMatId(pcontacts->id[1],i);
				phit->idmatOrg = pcontacts->id[1] + (pent->m_parts[i].surface_idx+1 & pcontacts->id[1]>>31);
				phit->foreignIdx = pent->m_parts[i].pPhysGeom->pGeom->GetForeignIdx(pcontacts->iPrim[1]);
				phit->pt = pcontacts->pt;
				phit->n = -pcontacts->n;
			}
		}
	}

	return isneg(phit->dist-1E9f);
}


static inline void swap(CPhysicalEntity **pentlist, int i1,int i2) {	
	CPhysicalEntity *pent = pentlist[i1]; pentlist[i1] = pentlist[i2]; pentlist[i2] = pent;
}
static void qsort(CPhysicalEntity **pentlist, const Vec3 &mask0,const Vec3 &mask1, int ileft,int iright)
{
	if (ileft>=iright) return;
	int i,ilast; 
	swap(pentlist, ileft,ileft+iright>>1);
	for(ilast=ileft,i=ileft+1; i<=iright; i++) 
		if (pentlist[i]->m_BBox[0]*mask0+pentlist[i]->m_BBox[1]*mask1 < pentlist[ileft]->m_BBox[0]*mask0+pentlist[ileft]->m_BBox[1]*mask1)
			swap(pentlist, ++ilast,i);
	swap(pentlist, ileft,ilast);
	qsort(pentlist, mask0,mask1, ileft,ilast-1);
	qsort(pentlist, mask0,mask1, ilast+1,iright);
}

float CPhysicalWorld::PrimitiveWorldIntersection(int itype, const primitive *pprim, const Vec3 &sweepDir, int entTypes, geom_contact **ppcontact,
																								 int geomFlagsAll,int geomFlagsAny, intersection_params *pip, void *pForeignData,int iForeignData,
																								 IPhysicalEntity **pSkipEnts,int nSkipEnts, const char *pNameTag)
{
	int i,j,j1,ncont,nents,iActive=0;
	int iCaller = get_iCaller();
	Vec3 BBox[2],sz,mask[2]={ Vec3(ZERO),Vec3(ZERO) };
	box bbox;
	CPhysicalEntity **pents;
	CBoxGeom gbox;
	CCylinderGeom gcyl;
	CCapsuleGeom gcaps;
	CSphereGeom gsph;
	CGeometry *pgeom;
	intersection_params ip;
	geom_world_data gwd[2];
	geom_contact *pcontacts;
	static geom_contact contactBest;
	contactBest.t = 0; contactBest.pt.zero();

	if (entTypes & rwi_queue) {
		WriteLock lockQ(m_lockPwiQueue);
		if (ppcontact || pip)
			return 0;

		ReallocQueue(m_pwiQueue, m_pwiQueueSz,m_pwiQueueAlloc, m_pwiQueueHead,m_pwiQueueTail, 64);
		switch (m_pwiQueue[m_pwiQueueHead].itype = itype) {
			case box::type: *(box*)m_pwiQueue[m_pwiQueueHead].pprim = *(box*)pprim; break;
			case cylinder::type: *(cylinder*)m_pwiQueue[m_pwiQueueHead].pprim = *(cylinder*)pprim; break;
			case capsule::type: *(capsule*)m_pwiQueue[m_pwiQueueHead].pprim = *(capsule*)pprim; break;
			case sphere::type: *(sphere*)m_pwiQueue[m_pwiQueueHead].pprim = *(sphere*)pprim; break;
			default: return 0;
		}
		m_pwiQueue[m_pwiQueueHead].sweepDir = sweepDir;
		m_pwiQueue[m_pwiQueueHead].entTypes = entTypes & ~rwi_queue;
		m_pwiQueue[m_pwiQueueHead].geomFlagsAll = geomFlagsAll;
		m_pwiQueue[m_pwiQueueHead].geomFlagsAny = geomFlagsAny;
		m_pwiQueue[m_pwiQueueHead].pForeignData = pForeignData;
		m_pwiQueue[m_pwiQueueHead].iForeignData = iForeignData;
		m_pwiQueue[m_pwiQueueHead].nSkipEnts = min(sizeof(m_pwiQueue[0].idSkipEnts)/sizeof(m_pwiQueue[0].idSkipEnts[0]),nSkipEnts);
		for(i=0;i<m_pwiQueue[m_pwiQueueHead].nSkipEnts;i++)
			m_pwiQueue[m_pwiQueueHead].idSkipEnts[i] = pSkipEnts[i] ? GetPhysicalEntityId(pSkipEnts[i]):-3;
		m_pwiQueueSz++;
		return 1;
	}

	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );
	PHYS_FUNC_PROFILER( pNameTag );
	WriteLock lock(m_lockCaller[iCaller]);

	if (pip) {
		ip = *pip;
		if (!ip.bThreadSafe)
			SpinLock(pip->plock=ip.plock=&g_lockIntersect,0,iActive=WRITE_LOCK_VAL);
		ip.bThreadSafe = true;
	} else if (sweepDir.len2()>0)	{
		ip.bSweepTest = true;
		gwd[0].v = sweepDir;
		ip.time_interval = 1.0f;
		contactBest.t = 1E10f;
	} else {
		ip.bStopAtFirstTri = true;
		ip.bNoBorder = true;
		ip.bNoAreaContacts = true;
	}
	if (ppcontact)
		*ppcontact = 0;

	switch (itype) {
		case box::type:			 gwd[0].offset=((box*)pprim)->center; ((box*)pprim)->center.zero(); gbox.CreateBox((box*)pprim); 
												 pgeom=&gbox; ((box*)pprim)->center=gwd[0].offset; break;
		case cylinder::type: gwd[0].offset=((cylinder*)pprim)->center; ((cylinder*)pprim)->center.zero(); gcyl.CreateCylinder((cylinder*)pprim); 
												 pgeom=&gcyl; ((cylinder*)pprim)->center=gwd[0].offset; break;
		case capsule::type:  gwd[0].offset=((capsule*)pprim)->center; ((capsule*)pprim)->center.zero(); gcaps.CreateCapsule((capsule*)pprim); 
												 pgeom=&gcaps; ((capsule*)pprim)->center=gwd[0].offset; break;
		case sphere::type:	 gwd[0].offset=((sphere*)pprim)->center; ((sphere*)pprim)->center.zero(); gsph.CreateSphere((sphere*)pprim); 
												 pgeom=&gsph; ((sphere*)pprim)->center=gwd[0].offset; break;
		default: return 0;
	}
	pgeom->GetBBox(&bbox);
	sz = bbox.size*bbox.Basis.Fabs();
	BBox[0] = gwd[0].offset+bbox.center-sz; BBox[1] = gwd[0].offset+bbox.center+sz;
	for(i=0;i<3;i++)
		BBox[0][i]+=min(0.0f,sweepDir[i]), BBox[1][i]+=max(0.0f,sweepDir[i]);

	for(i=0;i<nSkipEnts;i++) if (pSkipEnts[i]) {
		if (!(((CPhysicalPlaceholder**)pSkipEnts)[i]->m_bProcessed>>iCaller & 1))
			AtomicAdd(&((CPhysicalPlaceholder**)pSkipEnts)[i]->m_bProcessed,1<<iCaller);
		if (((CPhysicalPlaceholder**)pSkipEnts)[i]->m_pEntBuddy && !(((CPhysicalPlaceholder**)pSkipEnts)[i]->m_pEntBuddy->m_bProcessed>>iCaller&1))
			AtomicAdd(&((CPhysicalPlaceholder**)pSkipEnts)[i]->m_pEntBuddy->m_bProcessed,1<<iCaller);
	}

	nents = GetEntitiesAround(BBox[0],BBox[1],pents,entTypes,0,0,iCaller);
	if (ip.bSweepTest && nents>0) {
		i = idxmax3(sweepDir.abs());
		j = isneg(sweepDir[i]);
		mask[j][i] = 1-j*2;
		qsort(pents,mask[0],mask[1],0,nents-1);
		contactBest.pt = pents[nents-1]->m_BBox[j];
	}
	for(i=0;i<nents;i++) 
		if (sz.x=(pents[i]->m_BBox[1]-pents[i]->m_BBox[0]).len2(), 
				sz.x*(pents[i]->m_BBox[0]*mask[0]+pents[i]->m_BBox[1]*mask[1]) <= sz.x*(contactBest.pt*(mask[0]+mask[1]))) 
	{	ReadLock lockEnt(pents[i]->m_lockUpdate);
		for(j1=0;j1<pents[i]->GetUsedPartsCount(iCaller);j1++) 
			if ((pents[i]->m_parts[j=pents[i]->GetUsedPart(iCaller,j1)].flags & geomFlagsAll)==geomFlagsAll && 
					(pents[i]->m_parts[j].flags & geomFlagsAny) && 
					((pents[i]->m_parts[j].BBox[1]-pents[i]->m_parts[j].BBox[0]).len2()==0 || AABB_overlap(pents[i]->m_parts[j].BBox,BBox)))
			{
				gwd[1].offset = pents[i]->m_pos + pents[i]->m_qrot*pents[i]->m_parts[j].pos;
				gwd[1].R = Matrix33(pents[i]->m_qrot*pents[i]->m_parts[j].q);
				gwd[1].scale = pents[i]->m_parts[j].scale;
				if (ncont = pgeom->Intersect(pents[i]->m_parts[j].pPhysGeom->pGeom,gwd,gwd+1,&ip,pcontacts)) {
					WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive(ip.bThreadSafe^1);
					for(int ic=0;ic<ncont;ic++) {
						pcontacts[ic].iPrim[0]=pents[i]->m_id; pcontacts[ic].iPrim[1]=pents[i]->m_parts[j].id;
						pcontacts[ic].id[1] = pents[i]->GetMatId(pcontacts[ic].id[1], j);
					}
					if (ip.bStopAtFirstTri) {
						if (ppcontact) *ppcontact = ip.pGlobalContacts;
						return 1;
					}
					if (ip.bSweepTest) {
						if (pcontacts[0].t<contactBest.t)
							contactBest = pcontacts[0];
					} else
						ip.bKeepPrevContacts=true, contactBest.t+=ncont;
				}
			}
	}
	if (ppcontact)
		*ppcontact = ip.bSweepTest ? &contactBest : ip.pGlobalContacts;
	if (pip && contactBest.t==0)
		CryInterlockedAdd(&g_lockIntersect,-iActive);

	if (m_vars.iDrawHelpers & 64 && m_pRenderer) {
		if (!ip.bSweepTest)
			m_pRenderer->DrawGeometry(pgeom,gwd,7,1);
		else if (contactBest.t<1E10f)	{
			m_pRenderer->DrawGeometry(pgeom,gwd,7,1,sz=gwd[0].v.normalized()*contactBest.t);
			gwd[0].offset+=sz; m_pRenderer->DrawGeometry(pgeom,gwd,7,1,gwd[0].v-sz);
		}	else
			m_pRenderer->DrawGeometry(pgeom,gwd,7,1,gwd[0].v);
	}

	for(i=0;i<nSkipEnts;i++) if (pSkipEnts[i]) {
		if (((CPhysicalPlaceholder**)pSkipEnts)[i]->m_bProcessed>>iCaller & 1)
			AtomicAdd(&((CPhysicalPlaceholder**)pSkipEnts)[i]->m_bProcessed,-(1<<iCaller));
		if (((CPhysicalPlaceholder**)pSkipEnts)[i]->m_pEntBuddy && (((CPhysicalPlaceholder**)pSkipEnts)[i]->m_pEntBuddy->m_bProcessed>>iCaller&1))
			AtomicAdd(&((CPhysicalPlaceholder**)pSkipEnts)[i]->m_pEntBuddy->m_bProcessed,-(1<<iCaller));
	}

	return contactBest.t<1E10f ? contactBest.t:0;
}


int CPhysicalWorld::RayTraceEntity(IPhysicalEntity *pient, Vec3 origin,Vec3 dir, ray_hit *pHit, pe_params_pos *pp,
								   unsigned int geomFlagsAny /*=geom_colltype0|geom_colltype_player*/)
{
	if (!(dir.len2()>0 && origin.len2()>=0))
		return 0;

	int i,ncont;
	Vec3 pos;
	quaternionf qrot;
	float scale = 1.0f;
	CRayGeom aray(origin,dir);
	geom_world_data gwd;
	geom_contact *pcontacts;
	intersection_params ip;
	pHit->dist = 1E10;

	if (((CPhysicalPlaceholder*)pient)->m_iSimClass!=5) {
		CPhysicalEntity *pent = ((CPhysicalPlaceholder*)pient)->GetEntity();
		if (pp) {
			pos = pp->pos; qrot = pp->q; 
			if (!is_unused(pp->scale)) scale = pp->scale;
			get_xqs_from_matrices(pp->pMtx3x4,pp->pMtx3x3, pos,qrot,scale);
		}	else {
			pos = pent->m_pos; qrot = pent->m_qrot;
		}
		for(i=0;i<pent->m_nParts;i++) if (pent->m_parts[i].flags & geomFlagsAny) {
			//(pent->m_qrot*pent->m_parts[i].q).getmatrix(gwd.R);	//Q2M_IVO 
			gwd.R = Matrix33(qrot*pent->m_parts[i].q);
			gwd.offset = pos + qrot*pent->m_parts[i].pos;
			gwd.scale = scale*pent->m_parts[i].scale;
			ncont = pent->m_parts[i].pPhysGeom->pGeom->Intersect(&aray,&gwd,0,&ip,pcontacts);
			WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive(isneg(-ncont));
			for(; ncont>0 && pcontacts[ncont-1].t<pHit->dist && pcontacts[ncont-1].n*dir>0; ncont--);
			if (ncont>0) {
				pHit->dist = pcontacts[ncont-1].t;
				pHit->pCollider = pent; pHit->partid = pent->m_parts[pHit->ipart=i].id;
				pHit->surface_idx = pent->GetMatId(pcontacts[ncont-1].id[0],i);
				pHit->idmatOrg = pcontacts[ncont-1].id[0] + (pent->m_parts[i].surface_idx+1 & pcontacts[ncont-1].id[0]>>31);
				pHit->foreignIdx = pent->m_parts[i].pPhysGeom->pGeom->GetForeignIdx(pcontacts[ncont-1].iPrim[0]);
				pHit->pt = pcontacts[ncont-1].pt; 
				pHit->n = pcontacts[ncont-1].n;
			}
		}
	}	else 
		return ((CPhysArea*)pient)->RayTrace(origin,dir,pHit,pp);

	return isneg(pHit->dist-1E9);
}


CPhysicalEntity *CPhysicalWorld::CheckColliderListsIntegrity()
{
	int i,j,k;
	CPhysicalEntity *pent;
	for(i=1;i<=2;i++) for(pent=m_pTypedEnts[i];pent;pent=pent->m_next)
		for(j=0;j<pent->m_nColliders;j++) if (pent->m_pColliders[j]->m_iSimClass>0) {
			for(k=0;k<pent->m_pColliders[j]->m_nColliders && pent->m_pColliders[j]->m_pColliders[k]!=pent;k++);
			if (k==pent->m_pColliders[j]->m_nColliders)
				return pent;
		}
	return 0;
}


void CPhysicalWorld::GetMemoryStatistics(ICrySizer *pSizer)
{
	static char *entnames[] = { "static entities", "physical entities", "physical entities", "living entities", "detached entities", 
		"areas","triggers", "deleted entities" };
	int i,j,n;
	CPhysicalEntity *pent;

#ifndef _LIB // Only when compiling as dynamic library
	{
		//SIZER_COMPONENT_NAME(pSizer,"Strings");
		//pSizer->AddObject( (this+1),string::_usedMemory(0) );
	}
	{
#ifndef NOT_USE_CRY_MEMORY_MANAGER
		SIZER_COMPONENT_NAME(pSizer,"STL Allocator Waste");
		CryModuleMemoryInfo meminfo;
		ZeroStruct(meminfo);
		CryGetMemoryInfoForModule( &meminfo );
		pSizer->AddObject( (this+2),meminfo.STL_wasted );
#endif
	}
#endif

/*#ifdef WIN32
	static char *sec_ids[] = { ".text",".textbss",".data",".idata" };
	static char *sec_names[] = { "code section","code section","data section","data section" };
	_IMAGE_DOS_HEADER *pMZ = (_IMAGE_DOS_HEADER*)GetModuleHandle("CryPhysics.dll");
	_IMAGE_NT_HEADERS *pPE = (_IMAGE_NT_HEADERS*)((char*)pMZ+pMZ->e_lfanew);
	IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(pPE);
	for(i=0;i<pPE->FileHeader.NumberOfSections;i++) for(j=0;j<sizeof(sec_ids)/sizeof(sec_ids[0]);j++)
	if (!strncmp((char*)sections[i].Name, sec_ids[j], min(8,strlen(sec_ids[j])+1))) {
		SIZER_COMPONENT_NAME(pSizer, sec_names[j]);
		pSizer->AddObject((void*)sections[i].VirtualAddress, sections[i].Misc.VirtualSize);
	}
#endif*/

	{ SIZER_COMPONENT_NAME(pSizer,"world structures");
		pSizer->AddObject(this, sizeof(CPhysicalWorld));
		pSizer->AddObject(m_pTmpEntList, m_nEntsAlloc*sizeof(m_pTmpEntList[0]));
		pSizer->AddObject(m_pTmpEntList1, m_nEntsAlloc*sizeof(m_pTmpEntList1[0]));
		pSizer->AddObject(m_pTmpEntList2, m_nEntsAlloc*sizeof(m_pTmpEntList2[0]));
		pSizer->AddObject(m_pGroupMass, m_nEntsAlloc*sizeof(m_pGroupMass[0]));
		pSizer->AddObject(m_pMassList, m_nEntsAlloc*sizeof(m_pMassList[0]));
		pSizer->AddObject(m_pGroupIds, m_nEntsAlloc*sizeof(m_pGroupIds[0]));
		pSizer->AddObject(m_pGroupNums, m_nEntsAlloc*sizeof(m_pGroupNums[0]));
		pSizer->AddObject(m_pEntsById, m_nIdsAlloc*sizeof(m_pEntsById[0]));
		pSizer->AddObject(&m_pEntGrid, GetGridSize(m_pEntGrid, m_entgrid.size));
		pSizer->AddObject(m_gthunks, m_thunkPoolSz*sizeof(m_gthunks[0]));
		pSizer->AddObject(m_pGridStat, m_nOccRes*6*2*sizeof(m_pGridStat[0][0]));
		pSizer->AddObject(m_pExplVictims, m_nExplVictimsAlloc*(sizeof(m_pExplVictims[0])+sizeof(m_pExplVictimsFrac[0])+sizeof(m_pExplVictimsImp[0])));
		pSizer->AddObject(m_pFreeContact, m_nContactsAlloc*sizeof(m_pFreeContact[0]));
		if (m_bHasPODGrid) {
			for(i=j=0; i<m_entgrid.size.x*m_entgrid.size.y>>(3+m_log2PODscale)*2; j+=m_pPODCells[i++]!=0);
			pSizer->AddObject(m_pPODCells, j*sizeof(pe_PODcell)*64+(m_entgrid.size.x*m_entgrid.size.y*sizeof(m_pPODCells[0])>>(3+m_log2PODscale)*2));
		}
	}
			
	{ SIZER_COMPONENT_NAME(pSizer,"world queues");
		EventChunk *pChunk;
		for(n=0,pChunk=m_pFirstEventChunk;pChunk;pChunk=pChunk->next,n++);
		pSizer->AddObject(pChunk, n*EVENT_CHUNK_SZ);
		pSizer->AddObject(m_pQueueSlots, m_nQueueSlots*(sizeof(int*)+QUEUE_SLOT_SZ));
		pSizer->AddObject(m_rwiQueue, m_rwiQueueAlloc*sizeof(m_rwiQueue[0]));
		pSizer->AddObject(m_pwiQueue, m_pwiQueueAlloc*sizeof(m_pwiQueue[0]));
		pSizer->AddObject(m_pRwiHitsHead, m_rwiHitsPoolSize*sizeof(ray_hit));
	}

	{	SIZER_COMPONENT_NAME(pSizer,"placeholders");
		pSizer->AddObject(m_pPlaceholders, m_nPlaceholderChunks*(sizeof(CPhysicalPlaceholder)*PLACEHOLDER_CHUNK_SZ+sizeof(CPhysicalPlaceholder*)));
		pSizer->AddObject(m_pPlaceholderMap, (m_nPlaceholderChunks<<PLACEHOLDER_CHUNK_SZLG2-5)*sizeof(int));
	}

	{ SIZER_COMPONENT_NAME(pSizer,"entities");
		for(i=0;i<=7;i++) {
			SIZER_COMPONENT_NAME(pSizer,entnames[i]);
			for(pent=m_pTypedEnts[i]; pent; pent=pent->m_next)
				pent->GetMemoryStatistics(pSizer);
		}
		{	SIZER_COMPONENT_NAME(pSizer,"hidden entities");
			for(pent=m_pHiddenEnts; pent; pent=pent->m_next)
				pent->GetMemoryStatistics(pSizer);
		}
	}

	{ SIZER_COMPONENT_NAME(pSizer,"geometries");
		if (m_pHeightfield[0]) m_pHeightfield[0]->m_parts[0].pPhysGeom->pGeom->GetMemoryStatistics(pSizer);
		if (m_pHeightfield[1]) m_pHeightfield[1]->m_parts[0].pPhysGeom->pGeom->GetMemoryStatistics(pSizer);
		pSizer->AddObject(m_pGeoms, m_nGeomChunks*sizeof(m_pGeoms[0]));
		for(i=0;i<m_nGeomChunks;i++) {
			n = GEOM_CHUNK_SZ&i-m_nGeomChunks+1>>31 | m_nGeomsInLastChunk&m_nGeomChunks-2-i>>31;
			pSizer->AddObject(m_pGeoms[i], n*sizeof(m_pGeoms[i][0]));
			for(j=0;j<n;j++) if (m_pGeoms[i][j].pGeom)
				m_pGeoms[i][j].pGeom->GetMemoryStatistics(pSizer);
		}
	}

	{ SIZER_COMPONENT_NAME(pSizer,"external geometries");
		pSizer->AddObject(&m_sizeExtGeoms, m_sizeExtGeoms);
	}

#ifdef PS3
	{ SIZER_COMPONENT_NAME(pSizer,"spu job buffers");
		PhysicsSPUBuffer::GetMemoryUsage(pSizer);
	}
#endif

#ifdef USE_DYN_POOL
	if (DynAllocsTracked()) { 
		SIZER_COMPONENT_NAME(pSizer,"Unused breakables pool");
		pSizer->AddObject(0, GetFreeDynSize());
	}
#endif

	{ SIZER_COMPONENT_NAME(pSizer,"Static TriMesh Data");
		TriMeshStaticData::GetMemoryUsage(pSizer);
	}
}


void CPhysicalWorld::AddEntityProfileInfo(CPhysicalEntity *pent,int nTicks)
{
	if (m_nProfiledEnts==sizeof(m_pEntProfileData)/sizeof(m_pEntProfileData[0]) && 
			nTicks<=m_pEntProfileData[m_nProfiledEnts-1].nTicksStep || m_vars.bSingleStepMode)
		return;

	int i;
	WriteLock lock(m_lockEntProfiler);
	phys_profile_info ppi;
	for(i=0;i<m_nProfiledEnts && m_pEntProfileData[i].pEntity!=pent;i++);
	if (i==m_nProfiledEnts) {
		ppi.pEntity = pent;
		ppi.nTicks=ppi.nTicksStep=nTicks; ppi.nCalls=1;
		ppi.nTicksPeak=ppi.nCallsPeak=ppi.nTicksAvg=ppi.peakAge=0; ppi.nCallsAvg=0;
		ppi.id = pent->m_id;
		ppi.pName = m_pRenderer ? m_pRenderer->GetForeignName(pent->m_pForeignData,
				pent->m_iForeignData,pent->m_iForeignFlags) : "noname";
	}	else {
		ppi = m_pEntProfileData[i];
		ppi.nTicksStep &= -ppi.nTicks>>31;
		ppi.nTicks += nTicks; ppi.nCalls++;
		nTicks = (ppi.nTicksStep = max(ppi.nTicksStep,nTicks));
		memmove(m_pEntProfileData+i,m_pEntProfileData+i+1, (--m_nProfiledEnts-i)*sizeof(m_pEntProfileData[0]));
	}
	
	int iBound[2]={ -1,m_nProfiledEnts };
	do {
		i = iBound[0]+iBound[1]>>1;
		iBound[isneg(m_pEntProfileData[i].nTicksStep-nTicks)] = i;
	} while(iBound[1]>iBound[0]+1);
	m_nProfiledEnts = min(m_nProfiledEnts+1, sizeof(m_pEntProfileData)/sizeof(m_pEntProfileData[0]));
	if ((i=iBound[0]+1)<m_nProfiledEnts) {
		memmove(m_pEntProfileData+i+1,m_pEntProfileData+i, (m_nProfiledEnts-1-i)*sizeof(m_pEntProfileData[0]));
		m_pEntProfileData[i] = ppi;
	}
}


void CPhysicalWorld::AddFuncProfileInfo(const char *name, int nTicks)
{
	WriteLock lock(m_lockFuncProfiler);
	int i,iBound[2]={ -1,m_nProfileFunx };
	if (m_nProfileFunx) do {
		i = iBound[0]+iBound[1]>>1;
		iBound[isneg((int)(name-m_pFuncProfileData[i].pName))] = i;
	}	while(iBound[1]>iBound[0]+1);
	if ((i=iBound[0])<0 || m_pFuncProfileData[i].pName!=name) {
		++i;
		if (m_nProfileFunx==m_nProfileFunxAlloc) {
			if (m_nProfileFunx>=64)
				return;
			ReallocateList(m_pFuncProfileData, m_nProfileFunx,m_nProfileFunxAlloc+=16);
		}
		memmove(m_pFuncProfileData+i+1, m_pFuncProfileData+i, sizeof(phys_profile_info)*(m_nProfileFunx-i));
		m_pFuncProfileData[i].nTicks=m_pFuncProfileData[i].nTicksPeak=m_pFuncProfileData[i].peakAge = 0;
		m_pFuncProfileData[i].nCalls=m_pFuncProfileData[i].nCallsPeak = 0;
		m_pFuncProfileData[i].pName = name;
		m_nProfileFunx++;
	}
	m_pFuncProfileData[i].nTicks += nTicks;
	m_pFuncProfileData[i].nCalls++;
	m_pFuncProfileData[i].id = 0;
}


void CPhysicalWorld::AddEventClient(int type, int (*func)(const EventPhys*), int bLogged, float priority)
{
	RemoveEventClient(type,func,bLogged);
	EventClient *pSlot=new EventClient, *pCurSlot, *pSlot0=m_pEventClients[type][bLogged];
	memset(pSlot,0,sizeof(EventClient));
	pSlot->priority = priority;
	pSlot->OnEvent = func;
	if (pSlot0 && pSlot0->priority>priority) {
		for(pCurSlot=m_pEventClients[type][bLogged]; pCurSlot->next && pCurSlot->next->priority>priority; pCurSlot=pCurSlot->next);
		pSlot->next = pCurSlot->next; pCurSlot->next = pSlot;
	} else
		(m_pEventClients[type][bLogged] = pSlot)->next = pSlot0;
}

int CPhysicalWorld::RemoveEventClient(int type, int (*func)(const EventPhys*), int bLogged)
{
	EventClient *pSlot=m_pEventClients[type][bLogged];
	if (!pSlot)
		return 0;
	if (pSlot->OnEvent==func) {
		m_pEventClients[type][bLogged] = pSlot->next;
		delete pSlot;	return 1;
	}
	for(; pSlot->next && pSlot->next->OnEvent!=func; pSlot=pSlot->next);
	if (pSlot->next) {
		EventClient *pDelSlot = pSlot->next;
		pSlot->next = pSlot->next->next;
		delete pDelSlot; return 1;
	}
	return 0;
}


EventPhys *CPhysicalWorld::AllocEvent(int id,int sz)
{
	if (m_pFreeEvents[id]==0) {
		if (m_szCurEventChunk+sz > EVENT_CHUNK_SZ) {
			EventChunk *pNewChunk = (EventChunk*)(new char[sizeof(EventChunk)+max(sz,EVENT_CHUNK_SZ)]);
			pNewChunk->next = 0; m_pCurEventChunk->next = pNewChunk;
			m_pCurEventChunk = pNewChunk;
			m_szCurEventChunk = 0;
		}
		m_pFreeEvents[id] = (EventPhys*)((char*)(m_pCurEventChunk+1)+m_szCurEventChunk);
		m_szCurEventChunk += sz;
		m_pFreeEvents[id]->idval = id;
		m_pFreeEvents[id]->next = 0;
	}
	EventPhys *pSlot = m_pFreeEvents[id];
	m_pFreeEvents[id] = m_pFreeEvents[id]->next;
	m_nEvents[id]++;
	return pSlot;
}

void CPhysicalWorld::PumpLoggedEvents()
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );

	EventPhys *pEventFirst,*pEvent,*pEventLast,*pEvent_next;
	ray_hit *pLastPoolHit=0;
	int lastPoolHitBlockSize;
	{ WriteLock lock(m_lockEventsQueue);
		pEventFirst = m_pEventFirst;
		m_pEventFirst = m_pEventLast = 0; 
		for(int i=0; i<EVENT_TYPES_NUM; i++) m_nEvents[i] = 0;
	}
	m_iLastLogPump++;

	EventClient *pClient;
	for(pEvent=pEventFirst; pEvent; pEvent=pEvent->next)
		if (!(pEvent->idval<=EventPhysCollision::id && (((CPhysicalEntity*)((EventPhysStereo*)pEvent)->pEntity[0])->m_iDeletionTime | 
																									((CPhysicalEntity*)((EventPhysStereo*)pEvent)->pEntity[1])->m_iDeletionTime) ||
					pEvent->idval>EventPhysCollision::id && ((CPhysicalEntity*)((EventPhysMono*)pEvent)->pEntity)->m_iDeletionTime))
		{
			if (pEvent->idval==EventPhysPostStep::id) {
				EventPhysPostStep* pepps = (EventPhysPostStep*)pEvent;
				if (pepps->idStep==m_idStep)
					break;
				else if (pepps->idStep<m_idStep-1 && ((CPhysicalPlaceholder*)pepps->pEntity)->m_iSimClass>1+m_vars.bSingleStepMode*8 || 
								((CPhysicalPlaceholder*)pepps->pEntity)->m_bProcessed & PENT_SETPOSED ||
								((CPhysicalEntity*)pepps->pEntity)->m_iDeletionTime)
					continue;
			}
			for(pClient=m_pEventClients[pEvent->idval][1]; pClient; pClient=pClient->next) {
				BLOCK_PROFILER(pClient->ticks);
				int bContinue = pClient->OnEvent(pEvent);
				bContinue += iszero(pEvent->idval-EventPhysRWIResult::id)+iszero(pEvent->idval-EventPhysPWIResult::id);
				if (!bContinue)
					break;
			}
				
			if (pEvent->idval==EventPhysRWIResult::id && ((EventPhysRWIResult*)pEvent)->bHitsFromPool)
				pLastPoolHit = ((EventPhysRWIResult*)pEvent)->pHits+(lastPoolHitBlockSize=((EventPhysRWIResult*)pEvent)->nMaxHits)-1;
		}
	pEventLast = pEvent;

#ifndef PHYS_FUNC_PROFILER_DISABLED
	int iEvent,iClient;
	for(iEvent=0;iEvent<EVENT_TYPES_NUM;iEvent++) 
		for(pClient=m_pEventClients[iEvent][1],iClient=0; pClient; pClient=pClient->next,iClient++) {
			if (pClient->ticks*1000 > m_vars.ticksPerSecond) {
				sprintf(pClient->tag, "PhysEventHandler(%d,%d)", iEvent,iClient);
				AddFuncProfileInfo(pClient->tag,pClient->ticks);
			}
			pClient->ticks = 0;
		}
#endif


	{ WriteLock lock(m_lockEventsQueue);
		for(pEvent=pEventFirst; pEvent!=pEventLast; pEvent=pEvent_next) {
			pEvent_next = pEvent->next;
			pEvent->next = m_pFreeEvents[pEvent->idval]; m_pFreeEvents[pEvent->idval] = pEvent;
		}
		if (pEventLast) {
			CPhysicalEntity *pent;
			for(pEvent=pEventLast; pEvent; pEvent=pEvent->next)
				if (pEvent->idval>EventPhysCollision::id) {
					pent = (CPhysicalEntity*)((EventPhysMono*)pEvent)->pEntity;
					pent->m_iDeletionTime += min(1,pent->m_iDeletionTime);
				}	else {
					pent = (CPhysicalEntity*)((EventPhysStereo*)pEvent)->pEntity[0];
					pent->m_iDeletionTime += min(1,pent->m_iDeletionTime);
					pent = (CPhysicalEntity*)((EventPhysStereo*)pEvent)->pEntity[1];
					pent->m_iDeletionTime += min(1,pent->m_iDeletionTime);
				}
			for(pEvent=pEventLast; pEvent->next; pEvent=pEvent->next);
			pEvent->next = m_pEventFirst; 
			m_pEventFirst = pEventLast;
			if (!m_pEventLast)
				m_pEventLast = pEvent;
			//(m_pEventLast ? m_pEventLast->next : m_pEventFirst) = pEventLast;
			//for(m_pEventLast=pEventLast; m_pEventLast->next; m_pEventLast=m_pEventLast->next);
		}
	}
	if (pLastPoolHit) { 
		WriteLock lockH(m_lockRwiHitsPool);
		int i; for(i=1; i<lastPoolHitBlockSize && pLastPoolHit[-i].next==pLastPoolHit-i+1; i++);
		if (i<lastPoolHitBlockSize)// || pLastPoolHit->next->next!=pLastPoolHit->next+1)
			CryLog("Error: queued RWI hits pool corrupted");
		else {
			m_pRwiHitsHead = pLastPoolHit->next;
			m_rwiPoolEmpty = m_pRwiHitsTail->next==m_pRwiHitsHead;
		}
	}

	{ WriteLock lockfp(m_lockFuncProfiler);
		int i,j;
		for(i=j=0;i<m_nProfileFunx;i++) if (++m_pFuncProfileData[i].id<20) {
			if (i!=j) 
				m_pFuncProfileData[j] = m_pFuncProfileData[i];
			++j;
		}
		m_nProfileFunx = j;
	}
}

void CPhysicalWorld::ClearLoggedEvents()
{
	EventPhys *pEvent,*pEvent_next;
	WriteLock lock(m_lockEventsQueue);
	for(pEvent=m_pEventFirst; pEvent; pEvent=pEvent_next) {
		pEvent_next = pEvent->next;
		pEvent->next = m_pFreeEvents[pEvent->idval]; m_pFreeEvents[pEvent->idval] = pEvent;
	}
	m_pEventFirst = m_pEventLast = 0;
	m_iLastLogPump = -1;
}


entity_contact *CPhysicalWorld::AllocContact()
{
	WriteLock lock(m_lockContacts);
	if (m_pFreeContact->next==m_pFreeContact) {
		entity_contact *pChunk = new entity_contact[64];
		for(int i=0;i<64;i++) {
			pChunk[i].next = pChunk+i+1; pChunk[i].prev = pChunk+i-1;
			pChunk[i].bChunkStart = 0;
		}
		pChunk[0].prev = pChunk[63].next = CONTACT_END(m_pFreeContact);
		(m_pFreeContact = pChunk)->bChunkStart = 1;
		m_nFreeContacts += 64; m_nContactsAlloc += 64;
	}
	entity_contact *pContact = m_pFreeContact;
	m_pFreeContact->next->prev = m_pFreeContact->prev;
	m_pFreeContact->prev->next = m_pFreeContact->next;
	pContact->next=pContact->prev = pContact;
	m_nFreeContacts--;
	return pContact;
}


void CPhysicalWorld::FreeContact(entity_contact *pContact)
{
	WriteLock lock(m_lockContacts);
	pContact->prev = m_pFreeContact->prev; pContact->next = m_pFreeContact;
	m_pFreeContact->prev = pContact; m_pFreeContact = pContact;
	m_nFreeContacts++;
}


#ifdef PHYSWORLD_SERIALIZATION
void SerializeGeometries(CPhysicalWorld *pWorld, const char *fname,int bSave);
void SerializeWorld(CPhysicalWorld *pWorld, const char *fname,int bSave);

int CPhysicalWorld::SerializeWorld(const char *fname, int bSave) 
{
	::SerializeWorld(this,fname,bSave);
	return 1;
}
int CPhysicalWorld::SerializeGeometries(const char *fname, int bSave)
{
	::SerializeGeometries(this,fname,bSave);
	return 1;
}
#else
int CPhysicalWorld::SerializeWorld(const char *fname, int bSave) { return 0; }
int CPhysicalWorld::SerializeGeometries(const char *fname, int bSave) { return 0; }
#endif


int CPhysicalWorld::AddExplosionShape(IGeometry *pGeom, float size,int idmat, float probability)
{
	int i,j,bCreateConstraint=idmat>>16 & 1;
	idmat &= 0xFFFF;
	for(i=0;i<m_nExpl;i++) if (m_pExpl[i].pGeom==pGeom && m_pExpl[i].idmat==idmat)
		return -1;
	if (m_nExpl==m_nExplAlloc)
		ReallocateList(m_pExpl, m_nExpl,m_nExplAlloc+=16);

	for(i=0;i<m_nExpl && m_pExpl[i].idmat<=idmat;i++);
	memmove(m_pExpl+i+1, m_pExpl+i, (m_nExpl-i)*sizeof(m_pExpl[0]));
	if (i>0 && m_pExpl[i-1].idmat==idmat) {
		m_pExpl[i].iFirstByMat = m_pExpl[i-1].iFirstByMat;
		m_pExpl[i].nSameMat = m_pExpl[i-1].nSameMat+1;
		for(j=m_pExpl[i].iFirstByMat; j<m_pExpl[i].iFirstByMat+m_pExpl[i-1].nSameMat; j++) 
			m_pExpl[j].nSameMat++;
	}	else {
		m_pExpl[i].iFirstByMat = i;
		m_pExpl[i].nSameMat = 1;
	}
	if (pGeom->GetType()==GEOM_TRIMESH) {
		mesh_data *pmd = (mesh_data*)pGeom->GetData();
		memset(pmd->pMats, 100, pmd->nTris);
	}

	m_pExpl[i].id = m_idExpl++;
	(m_pExpl[i].pGeom = pGeom)->AddRef();
	m_pExpl[i].size = size;
	m_pExpl[i].rsize = 1/size;
	m_pExpl[i].idmat = idmat;
	m_pExpl[i].probability = probability;
	m_pExpl[i].bCreateConstraint = bCreateConstraint;
	m_nExpl++;

	return m_pExpl[i].id;
}

void CPhysicalWorld::RemoveExplosionShape(int id)
{
	int i,j;
	for(i=0;i<m_nExpl && m_pExpl[i].id!=id;i++);
	if (i==m_nExpl)
		return;
	m_pExpl[i].pGeom->Release();
	for(j=m_pExpl[i].iFirstByMat; j<m_pExpl[i].iFirstByMat+m_pExpl[i].nSameMat; j++)
		m_pExpl[j].nSameMat--;
	for(; j<m_nExpl; j++)
		m_pExpl[j].iFirstByMat--;
	memmove(m_pExpl+i, m_pExpl+i+1, (m_nExpl-1-i)*sizeof(m_pExpl[0]));
	m_nExpl--;
}


IGeometry *CPhysicalWorld::GetExplosionShape(float size,int idmat, float &scale, int &bCreateConstraint)
{
	int i,j,mask,ibound[2]={ -1,m_nExpl };
	float sum,probabilitySum,f;

	if (!m_nExpl || size<=0)
		return 0;
	idmat &= 127;
	do {
		i = ibound[0]+ibound[1]>>1;
		ibound[isneg(idmat-m_pExpl[i].idmat)] = i;
	} while(ibound[1] > ibound[0]+1);
	if (ibound[0]<0 || m_pExpl[ibound[0]].idmat!=idmat)
		return 0;

	ibound[1] = m_pExpl[ibound[0]].iFirstByMat+m_pExpl[ibound[0]].nSameMat;
	j = ibound[0] = m_pExpl[ibound[0]].iFirstByMat;
	for(i=j+1; i<ibound[1]; i++)	{
		mask = -isneg(fabs_tpl(m_pExpl[i].size-size) - fabs_tpl(m_pExpl[j].size-size));
		j = i&mask | j&~mask;
	}

	for(i=ibound[0],probabilitySum=0; i<ibound[1]; i++)
		probabilitySum += m_pExpl[i].probability*iszero(m_pExpl[i].size-m_pExpl[j].size);
	f = (physics_rand()+1)*probabilitySum*(1.0f/(RAND_MAX+1));
	for(i=ibound[0],sum=0; i<ibound[1] && sum<f; i++)
		sum += m_pExpl[i].probability*iszero(m_pExpl[i].size-m_pExpl[j].size);

	scale = size*m_pExpl[i-1].rsize;
	bCreateConstraint = m_pExpl[i-1].bCreateConstraint;
	return m_pExpl[i-1].pGeom;
}


int CPhysicalWorld::SetWaterManagerParams(pe_params *params) 
{ 
	if (params->type==pe_params_waterman::type_id && !is_unused(((pe_params_waterman*)params)->posViewer))
		m_posViewer = ((pe_params_waterman*)params)->posViewer;
	if (!m_pWaterMan) {
		if ((params->type!=pe_params_waterman::type_id) || is_unused(((pe_params_waterman*)params)->nExtraTiles))
			return 0;
		m_pWaterMan = new CWaterMan(this);
	}
	return m_pWaterMan->SetParams(params);
}
int CPhysicalWorld::GetWaterManagerParams(pe_params *params) 
{ 
	return m_pWaterMan ? m_pWaterMan->GetParams(params):0; 
}
int CPhysicalWorld::GetWatermanStatus(pe_status *status)
{
	return m_pWaterMan ? m_pWaterMan->GetStatus(status):0;
}
void CPhysicalWorld::DestroyWaterManager() 
{ 
	if (m_pWaterMan) { 
		delete m_pWaterMan; m_pWaterMan=0; 
	} 
}


void CPhysicalWorld::GetEntityMassAndCom(IPhysicalEntity *pIEnt, float &mass, Vec3 &com)
{
  CPhysicalEntity* pent = (CPhysicalEntity*) pIEnt; // CRAP: need to check this is CPhysicalEntity

	int i,j;
	for(j=i=0,mass=0; i<pent->m_nParts; i++) {
		j += i-j & -isneg(pent->m_parts[i].mass-pent->m_parts[j].mass);
		mass += pent->m_parts[i].mass;
	}
	//if (pent->m_nParts)
	//	com = pent->m_pos+pent->m_qrot*(pent->m_parts[j].pos+pent->m_parts[j].q*pent->m_parts[j].pPhysGeomProxy->origin*pent->m_parts[j].scale);
	//else com.zero();
  com = pent->m_BBox[0] + (pent->m_BBox[1] - pent->m_BBox[0]) / 2.0f;
}

void CPhysicalWorld::SavePhysicalEntityPtr(TSerialize ser, IPhysicalEntity* pIEnt)
{
  CPhysicalEntity* pent = (CPhysicalEntity*) pIEnt; // CRAP: need to check this is CPhysicalEntity

	ser.BeginGroup("entity_ptr");
	int i;
	float mass; Vec3 com;
	ser.Value("id", i=pent->m_id);
	ser.Value("simclass", i=pent->m_iSimClass);
	GetEntityMassAndCom(pent, mass,com);
	ser.Value("com", com);
	ser.Value("mass", mass);
	ser.EndGroup();
}

IPhysicalEntity* CPhysicalWorld::LoadPhysicalEntityPtr(TSerialize ser)
{
	int i,iSimClass,iSimClass1;
	CPhysicalEntity *pent,**pents;
	float mass,mass1;
	Vec3 com,com1;
	ser.BeginGroup("entity_ptr");
	ser.Value("id", i);
	ser.Value("simclass", iSimClass);
	ser.Value("com", com);
	ser.Value("mass", mass);
	ser.EndGroup();
	iSimClass1 = (unsigned int)(iSimClass-1)<2u ? (iSimClass^3):iSimClass;

	if (pent = (CPhysicalEntity*)GetPhysicalEntityById(i)) {
		GetEntityMassAndCom(pent, mass1,com1);
		if ((pent->m_iSimClass==iSimClass || pent->m_iSimClass==iSimClass1) && 
				fabs_tpl(mass-mass1)<=fabs_tpl(min(mass,mass1))*0.01f && (com-com1).len2()<sqr(0.01f))
			return pent;
	}

	for(i=GetEntitiesAround(com-Vec3(0.1f),com+Vec3(0.1f),pents,1<<iSimClass|1<<iSimClass1)-1;i>=0;i--) {
		GetEntityMassAndCom(pents[i],mass1,com1);
		if (fabs_tpl(mass-mass1)<=fabs_tpl(min(mass,mass1))*0.01f && (com-com1).len2()<sqr(0.01f))
			return pents[i];
	}
	return 0;
}


int CPhysicalEntity::SetStateFromTypedSnapshot(TSerialize ser, int iSnapshotType, int flags) 
{ 
	static CPhysicalEntity g_entStatic(0);
	static CRigidEntity g_entRigid(0);
	static CWheeledVehicleEntity g_entWheeled(0);
	static CLivingEntity g_entLiving(0);
	static CParticleEntity g_entParticle(0);
	static CArticulatedEntity g_entArticulated(0);
	static CRopeEntity g_entRope(0);
	static CSoftEntity g_entSoft(0);
	static CPhysicalEntity *g_pTypedEnts[] = { &g_entStatic,&g_entStatic,&g_entRigid,&g_entWheeled,
		&g_entLiving,&g_entParticle,&g_entArticulated,&g_entRope,&g_entSoft };

	if (GetType()==iSnapshotType)
		return SetStateFromSnapshot(ser,flags);
	return g_pTypedEnts[min(sizeof(g_pTypedEnts)/sizeof(g_pTypedEnts[0]),max(0,iSnapshotType))]->
		SetStateFromSnapshot(ser,flags); 
}

void CPhysicalWorld::SerializeGarbageTypedSnapshot( TSerialize ser, int iSnapshotType, int flags )
{
	static CPhysicalEntity g_entStatic(0);
	static CRigidEntity g_entRigid(0);
	static CWheeledVehicleEntity g_entWheeled(0);
	static CLivingEntity g_entLiving(0);
	static CParticleEntity g_entParticle(0);
	static CArticulatedEntity g_entArticulated(0);
	static CRopeEntity g_entRope(0);
	static CSoftEntity g_entSoft(0);
	static CPhysicalEntity *g_pTypedEnts[] = { &g_entStatic,&g_entStatic,&g_entRigid,&g_entWheeled,
		&g_entLiving,&g_entParticle,&g_entArticulated,&g_entRope,&g_entSoft };

	g_pTypedEnts[min(sizeof(g_pTypedEnts)/sizeof(g_pTypedEnts[0]),max(0,iSnapshotType))]->
		GetStateSnapshot(ser,flags);
}

IPhysicalEntityIt* CPhysicalWorld::GetEntitiesIterator()
{
  return new CPhysicalEntityIt(this);
}
