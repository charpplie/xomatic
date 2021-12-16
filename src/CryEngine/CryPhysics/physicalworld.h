//////////////////////////////////////////////////////////////////////
//
//	Physical Entity header
//	
//	File: physicalentity.h
//	Description : PhysicalEntity and PhysicalWorld classes declarations
//
//	History:
//	-:Created by Anton Knyazev
//
//////////////////////////////////////////////////////////////////////

#ifndef physicalworld_h
#define physicalworld_h
#pragma once

#ifndef __SPU__
#include "IThreadTask.h"
#endif
#include "physicalentity.h"
#include "geoman.h"

#if defined(PS3) && defined(EXCLUDE_PHYSICS_THREAD)
#define __atomic_align _ALIGN(128)
#else
#define __atomic_align 
#endif

#ifdef __SPU__
	#define ppu_const
#else
	#define ppu_const const
#endif

const int NSURFACETYPES = 512;
const int PLACEHOLDER_CHUNK_SZLG2 = 8;
const int PLACEHOLDER_CHUNK_SZ = 1<<PLACEHOLDER_CHUNK_SZLG2;
const int QUEUE_SLOT_SZ = 8192;

const int PENT_SETPOSED = 1<<16;
const int PENT_QUEUED_BIT = 17;
const int PENT_QUEUED = 1<<PENT_QUEUED_BIT;

class CPhysicalPlaceholder;
class CPhysicalEntity;
class CPhysArea;
struct pe_gridthunk;
enum { pef_step_requested = 0x40000000 };

#define CONTACT_END(pFirstContact) ((entity_contact*)&(pFirstContact))


#ifdef ENTITY_PROFILER_ENABLED
#define PHYS_ENTITY_PROFILER CPhysEntityProfiler ent_profiler(this);
#else
#define PHYS_ENTITY_PROFILER
#endif

#ifdef __SPU__
#define PHYS_FUNC_PROFILER_DISABLED
#endif

#ifndef PHYS_FUNC_PROFILER_DISABLED
#define PHYS_FUNC_PROFILER(name) CPhysFuncProfiler func_profiler((name),this);
#define BLOCK_PROFILER(count) CBlockProfiler block_profiler(count);
#else
#define PHYS_FUNC_PROFILER(name)
#define BLOCK_PROFILER(count)
#endif

struct EventClient {
	int (*OnEvent)(const EventPhys*);
	EventClient *next;
	float priority;
#ifndef PHYS_FUNC_PROFILER_DISABLED
	char tag[32];
	int64 ticks;
#endif
};

const int EVENT_CHUNK_SZ = 8192-sizeof(void*);
struct EventChunk {
	EventChunk *next;
};

struct SExplosionShape {
	int id;
	IGeometry *pGeom;
	float size,rsize;
	int idmat;
	float probability;
	int iFirstByMat,nSameMat;
	int bCreateConstraint;
};

struct SRwiRequest {
	void *pForeignData;
	int iForeignData;
	Vec3 org,dir;
	int objtypes;
	unsigned int flags;
	ray_hit *hits;
	int nMaxHits;
  int idSkipEnts[5];
	ray_hit_cached *phitLast;
	int nSkipEnts;
	int iCaller;
};

#define max_def(a,b) (a) + ((b)-(a) & (a)-(b)>>31)
struct SPwiRequest {
	void *pForeignData;
	int iForeignData;
	int itype;
	char pprim[max_def(max_def(max_def(sizeof(box),sizeof(cylinder)),sizeof(capsule)),sizeof(sphere))];
	Vec3 sweepDir;
	int entTypes;
	int geomFlagsAll;
	int geomFlagsAny;
	int idSkipEnts[4];
	int nSkipEnts;
};

struct SBreakRequest {
	pe_explosion expl;
	geom_world_data gwd[2];
	CPhysicalEntity *pent;
	int partid;
	Vec3 gravity;
};

struct pe_PODcell {
	int inextActive;
	float lifeTime;
	Vec2 zlim;
};

struct SThreadData {
	CPhysicalEntity **pTmpEntList;
	int szList;
	int bGroupInvisible;
	float groupMass;
};

struct SThreadTaskRequest {
	float time_interval;
	int bSkipFlagged;
	int ipass;
	int iter;
	int *pbAllGroupsFinished;
};


#ifdef ENTGRID_2LEVEL
struct pe_entgrid {
	pe_entgrid &operator=(const pe_entgrid &src) { log2Stride=src.log2Stride; gridlod1=src.gridlod1; return *this; }
	pe_entgrid &operator=(int) { gridlod1=0; return *this; }
	operator bool() const { return gridlod1!=0; }

	int operator[](int i) {
		int iy0=i>>log2Stride, ix0=i & (1<<log2Stride)-1;
		int *grid = gridlod1[(iy0>>3)<<log2Stride-3 | ix0>>3], dummy=0;
		INT_PTR mask = -iszero((INT_PTR)grid);
		return ((int*)((INT_PTR)grid+((INT_PTR)&dummy-(INT_PTR)grid & mask)))[(iy0 & 7)*8+(ix0 & 7) & ~(int)mask];
	}
	int &operator[](unsigned int i) {
		int iy0=i>>log2Stride, ix0=i & (1<<log2Stride)-1;
		int *&grid = gridlod1[(iy0>>3)<<log2Stride-3 | ix0>>3];
		if (!grid)
			memset(grid=new int[64], 0, 64*sizeof(int));
		return grid[(iy0 & 7)*8+(ix0 & 7)];
	}

	int log2Stride;
	int **gridlod1;
};

inline void AllocateGrid(pe_entgrid &grid, const Vec2i &size)
{
	memset(grid.gridlod1 = new int*[(size.x*size.y>>6)+1], 0, ((size.x*size.y>>6)+1)*sizeof(int*));
	grid.log2Stride = bitcount(size.x-1);
}
inline void DeallocateGrid(pe_entgrid &grid, const Vec2i &size)
{
	for(int i=size.x*size.y>>6;i>=0;i--) if (grid.gridlod1[i])
		delete[] grid.gridlod1[i];
	delete grid.gridlod1; grid.gridlod1=0;
}
inline int GetGridSize(pe_entgrid &grid, const Vec2i& size)
{
	if (!grid)
		return 0;
	int i,sz=0;
	for(i=size.x*size.y>>6; i>=0; i--) 
		sz += iszero((INT_PTR)grid.gridlod1[i])^1;
	return sz*64*sizeof(int)+((size.x*size.y>>6)+1)*sizeof(int*);
}
#else
#define pe_entgrid int*
inline void AllocateGrid(int *&grid, const Vec2i &size) { memset(grid = new int[size.x*size.y+1], 0, (size.x*size.y+1)*sizeof(int)); }
inline void DeallocateGrid(int *&grid, const Vec2i &size) { delete[] grid; }
inline int GetGridSize(int *&grid, const Vec2i& size) { return (size.x*size.y+1)*sizeof(int); }
#endif


#ifndef __SPU__
struct SPhysTask : public IThreadTask {
	SPhysTask(CPhysicalWorld *pWorld,int idx) { m_pWorld=pWorld; m_idx=idx; bStop=0; }
	virtual void OnUpdate();
	virtual void Stop();
	virtual SThreadTaskInfo* GetTaskInfo() { return &m_TaskInfo; }
	class CPhysicalWorld *m_pWorld;
	int m_idx;
	volatile int bStop;
	SThreadTaskInfo m_TaskInfo;
};

#ifndef MAIN_THREAD_NONWORKER
#define THREAD_TASK(a,b) \
	m_rq.ipass=a; \
	for(int ithread=0;ithread<m_nWorkerThreads;ithread++)	m_threadStart[ithread].Set();	\
	b; \
	for(int ithread=0;ithread<m_nWorkerThreads;ithread++)	m_threadDone[ithread].Wait();
#else
#define THREAD_TASK(a,b) \
	m_rq.ipass=a; \
	for(int ithread=0;ithread<m_nWorkerThreads;ithread++)	m_threadStart[ithread].Set();	\
	for(int ithread=0;ithread<m_nWorkerThreads;ithread++)	m_threadDone[ithread].Wait();
#endif

class CPhysicalWorld : public IPhysicalWorld, public IPhysUtils, public CGeomManager {

  class CPhysicalEntityIt : public IPhysicalEntityIt {
  public:
    CPhysicalEntityIt(CPhysicalWorld* pWorld);

	  virtual bool IsEnd();
	  virtual IPhysicalEntity* Next();
	  virtual IPhysicalEntity* This();
	  virtual void MoveFirst();

	  virtual void AddRef()	{	++m_refs;	}
		virtual void Release() { if (--m_refs<=0)	delete this; }
	private:
		CPhysicalWorld* m_pWorld;
		int m_currEntity;
		int m_refs;
	};

public:
	CPhysicalWorld(ILog *pLog);
	~CPhysicalWorld();

	virtual void Init();
	virtual void Shutdown(int bDeleteEntities = 1);
	virtual void Release() { delete this; }
	virtual IGeomManager* GetGeomManager() { return this; }
	virtual IPhysUtils* GetPhysUtils() { return this; }

	virtual void SetupEntityGrid(int axisz, Vec3 org, int nx,int ny, float stepx,float stepy, int log2PODscale=0);
	virtual void DeactivateOnDemandGrid();
	virtual void RegisterBBoxInPODGrid(const Vec3 *BBox);
	virtual int AddRefEntInPODGrid(IPhysicalEntity *pent, const Vec3 *BBox=0);
	virtual IPhysicalEntity *SetHeightfieldData(const heightfield *phf,int *pMatMapping=0,int nMats=0);
	virtual IPhysicalEntity *GetHeightfieldData(heightfield *phf);
	virtual int SetSurfaceParameters(int surface_idx, float bounciness,float friction, unsigned int flags=0);
	virtual int GetSurfaceParameters(int surface_idx, float &bounciness,float &friction, unsigned int &flags);
	virtual int SetSurfaceParameters(int surface_idx, float bounciness,float friction,
		float damage_reduction, float ric_angle, float ric_dam_reduction,
		float ric_vel_reduction, unsigned int flags=0);
	virtual int GetSurfaceParameters(int surface_idx, float &bounciness,float &friction,
		float &damage_reduction, float &ric_angle, float &ric_dam_reduction,
		float &ric_vel_reduction, unsigned int &flags);
	virtual PhysicsVars *GetPhysVars() { return &m_vars; }

	void GetPODGridCellBBox(int ix,int iy, Vec3 &center,Vec3 &size);
	pe_PODcell *getPODcell(int ix,int iy)	{
		int i, imask = ~negmask(ix) & ~negmask(iy) & negmask(ix - m_entgrid.size.x) & negmask(iy - m_entgrid.size.y);   // (x>=0 && x<m_entgrid.size.x) ? 0xffffffff : 0;
		ix >>= m_log2PODscale; iy >>= m_log2PODscale;
		i = (ix>>3)*m_PODstride.x + (iy>>3)*m_PODstride.y;
		i = i + ((m_entgrid.size.x*m_entgrid.size.y>>(3+m_log2PODscale)*2)-i & ~imask) & -m_bHasPODGrid;
		INT_PTR pmask = -iszero((INT_PTR)m_pPODCells[i]);
		pe_PODcell *pcell0 = (pe_PODcell*)(((INT_PTR)m_pPODCells[i] & ~pmask) + ((INT_PTR)m_pDummyPODcell & pmask));
		imask &= -m_bHasPODGrid & ~pmask;
		return pcell0 + ((ix&7)+(iy&7)*8 & imask);
	}
	void InitGThunksPool();

	virtual IPhysicalEntity* CreatePhysicalEntity(pe_type type, pe_params* params=0, void *pForeignData=0,int iForeignData=0, int id=-1)
	{ return CreatePhysicalEntity(type,0.0f,params,pForeignData,iForeignData,id); }
	virtual IPhysicalEntity* CreatePhysicalEntity(pe_type type, float lifeTime, pe_params* params=0, void *pForeignData=0,int iForeignData=0, 
		int id=-1,IPhysicalEntity *pHostPlaceholder=0);
	virtual IPhysicalEntity *CreatePhysicalPlaceholder(pe_type type, pe_params* params=0, void *pForeignData=0,int iForeignData=0, int id=-1);
	virtual int DestroyPhysicalEntity(IPhysicalEntity *pent, int mode=0, int bThreadSafe=0);
	virtual int SetPhysicalEntityId(IPhysicalEntity *pent, int id, int bReplace=1, int bThreadSafe=0);
	virtual int GetPhysicalEntityId(IPhysicalEntity *pent);
	int GetFreeEntId();
	virtual IPhysicalEntity* GetPhysicalEntityById(int id);
	int IsPlaceholder(const CPhysicalPlaceholder *pent) {
		if (!pent) return 0;
		int iChunk; for(iChunk=0; iChunk<m_nPlaceholderChunks && (unsigned int)(pent-m_pPlaceholders[iChunk])>=(unsigned int)PLACEHOLDER_CHUNK_SZ; iChunk++);
		return iChunk<m_nPlaceholderChunks ? (iChunk<<PLACEHOLDER_CHUNK_SZLG2 | pent-m_pPlaceholders[iChunk])+1 : 0;
	}

	virtual void TimeStep(float time_interval, int flags=ent_all|ent_deleted);
	virtual float GetPhysicsTime() { return m_timePhysics; }
	virtual int GetiPhysicsTime() { return m_iTimePhysics; }
	virtual void SetPhysicsTime(float time) { 
		m_timePhysics = time; 
		if (m_vars.timeGranularity>0) 
			m_iTimePhysics = (int)(m_timePhysics/m_vars.timeGranularity+0.5f); 
	}
	virtual void SetiPhysicsTime(int itime) { m_timePhysics = (m_iTimePhysics=itime)*m_vars.timeGranularity; }
	virtual void SetSnapshotTime(float time_snapshot,int iType=0) { 
		m_timeSnapshot[iType] = time_snapshot; 
		if (m_vars.timeGranularity>0) 
			m_iTimeSnapshot[iType] = (int)(time_snapshot/m_vars.timeGranularity+0.5f); 
	}
	virtual void SetiSnapshotTime(int itime_snapshot,int iType=0) { 
		m_iTimeSnapshot[iType] = itime_snapshot; m_timeSnapshot[iType] = itime_snapshot*m_vars.timeGranularity; 
	}

	// *important* if request RWIs queued iForeignData should be a EPhysicsForeignIds
	virtual int RayWorldIntersection(Vec3 org,Vec3 dir, int objtypes, unsigned int flags, ray_hit *hits,int nmaxhits, 
		IPhysicalEntity **pSkipEnts=0,int nSkipEnts=0, void *pForeignData=0,int iForeignData=0, 
		const char *pNameTag="RayWorldIntersection(Physics)", ray_hit_cached *phitLast=0, int iCaller=get_iCaller_int());
	virtual int TracePendingRays(int bDoTracing=1);

	void RayHeightfield(const Vec3 &org,Vec3 &dir, ray_hit *hits, int flags, int iCaller);
	void RayWater(const Vec3 &org,const Vec3 &dir, struct entity_grid_checker &egc, int flags,int nMaxHits, ray_hit *hits);

	virtual void SimulateExplosion(pe_explosion *pexpl, IPhysicalEntity **pSkipEnts=0,int nSkipEnts=0,
		int iTypes=ent_rigid|ent_sleeping_rigid|ent_living|ent_independent, int iCaller=get_iCaller_int());
	virtual float IsAffectedByExplosion(IPhysicalEntity *pent, Vec3 *impulse=0);
	virtual float CalculateExplosionExposure(pe_explosion *pexpl, IPhysicalEntity *pient);
	virtual void ResetDynamicEntities();
	virtual void DestroyDynamicEntities();
	virtual void PurgeDeletedEntities();
	virtual int DeformPhysicalEntity(IPhysicalEntity *pent, const Vec3 &ptHit,const Vec3 &dirHit,float r, int flags=0);
	virtual void UpdateDeformingEntities(float time_interval);
	virtual int GetEntityCount(int iEntType) { return m_nTypeEnts[iEntType]; }
	virtual int ReserveEntityCount(int nNewEnts);

  virtual IPhysicalEntityIt* GetEntitiesIterator();

	virtual void DrawPhysicsHelperInformation(IPhysRenderer *pRenderer,int iCaller=0);

	virtual void GetMemoryStatistics(ICrySizer *pSizer);

	virtual int CollideEntityWithBeam(IPhysicalEntity *_pent, Vec3 org,Vec3 dir,float r, ray_hit *phit);
	virtual int RayTraceEntity(IPhysicalEntity *pient, Vec3 origin,Vec3 dir, ray_hit *pHit, pe_params_pos *pp=0,
		unsigned int geomFlagsAny = geom_colltype0|geom_colltype_player);
	virtual int CollideEntityWithPrimitive(IPhysicalEntity *_pent, int itype, primitive *pprim, Vec3 dir, ray_hit *phit);

	virtual float PrimitiveWorldIntersection(int itype, const primitive *pprim, const Vec3 &sweepDir=Vec3(ZERO), int entTypes=ent_all, 
		geom_contact **ppcontact=0, int geomFlagsAll=0,int geomFlagsAny=geom_colltype0|geom_colltype_player, intersection_params *pip=0,
		void *pForeignData=0, int iForeignData=0, IPhysicalEntity **pSkipEnts=0,int nSkipEnts=0, const char *pNameTag="PrimitiveWorldIntersection");

	virtual int GetEntitiesInBox(Vec3 ptmin,Vec3 ptmax, IPhysicalEntity **&pList, int objtypes, int szListPrealloc) {
		WriteLock lock(m_lockCaller[MAX_PHYS_THREADS]);
		return GetEntitiesAround(ptmin,ptmax, (CPhysicalEntity**&)pList, objtypes, 0, szListPrealloc, MAX_PHYS_THREADS);
	}
	int GetEntitiesAround(const Vec3 &ptmin,const Vec3 &ptmax, CPhysicalEntity **&pList, int objtypes, CPhysicalEntity *pPetitioner=0, 
		int szListPrealoc=0, int iCaller=get_iCaller_int());
	void ChangeEntitySimClass(CPhysicalEntity *pent);
	int RepositionEntity(CPhysicalPlaceholder *pobj, int flags=3, Vec3 *BBox=0, int bQueued=0);
	void DetachEntityGridThunks(CPhysicalPlaceholder *pobj);
	void ScheduleForStep(CPhysicalEntity *pent, float time_interval);
	CPhysicalEntity *CheckColliderListsIntegrity();

	virtual int CoverPolygonWithCircles(strided_pointer<vector2df> pt,int npt,bool bConsecutive, const vector2df &center, 
		vector2df *&centers,float *&radii, float minCircleRadius) 
	{ return ::CoverPolygonWithCircles(pt,npt,bConsecutive, center, centers,radii, minCircleRadius); }
	virtual void DeletePointer(void *pdata) { if (pdata) delete[] (char*)pdata; }
	virtual int qhull(strided_pointer<Vec3> pts, int npts, index_t*& pTris, qhullmalloc qmalloc = 0) { return ::qhull(pts,npts,pTris, qmalloc); }
	virtual void SetPhysicsStreamer(IPhysicsStreamer *pStreamer) { m_pPhysicsStreamer=pStreamer; }
	virtual void SetPhysicsEventClient(IPhysicsEventClient *pEventClient) { m_pEventClient=pEventClient; }
	virtual float GetLastEntityUpdateTime(IPhysicalEntity *pent) { return m_updateTimes[((CPhysicalPlaceholder*)pent)->m_iSimClass & 7]; }
	virtual volatile int *GetInternalLock(int idx) {
		switch (idx) {
			case PLOCK_WORLD_STEP: return &m_lockStep; 
			case PLOCK_QUEUE: return &m_lockQueue;
			case PLOCK_AREAS: return &m_lockAreas;
			default: 
				if ((unsigned int)(idx-PLOCK_CALLER0)<=(unsigned int)MAX_PHYS_THREADS)
					return m_lockCaller+(idx-PLOCK_CALLER0);
		}
		return 0;
	}

	void AddEntityProfileInfo(CPhysicalEntity *pent,int nTicks);
	virtual int GetEntityProfileInfo(phys_profile_info *&pList)	{	pList=m_pEntProfileData; return m_nProfiledEnts; }
	void AddFuncProfileInfo(const char *name,int nTicks);
	virtual int GetFuncProfileInfo(phys_profile_info *&pList)	{	pList=m_pFuncProfileData; return m_nProfileFunx; }

	// *important* if provided callback function return 0, other registered listeners are not called anymore.
	virtual void AddEventClient(int type, int (*func)(const EventPhys*), int bLogged, float priority=1.0f);
	virtual int RemoveEventClient(int type, int (*func)(const EventPhys*), int bLogged);
	virtual void PumpLoggedEvents();
	virtual void ClearLoggedEvents();
	void CleanseEventsQueue();
	EventPhys *AllocEvent(int id,int sz);
	template<class Etype> int OnEvent(unsigned int flags, Etype *pEvent, Etype **pEventLogged=0) {
		int res = 0;
		if ((flags & Etype::flagsCall)==Etype::flagsCall)
			res = SignalEvent(pEvent, 0);
		if ((flags & Etype::flagsLog)==Etype::flagsLog) {
			WriteLock lock(m_lockEventsQueue);
			Etype *pDst = (Etype*)AllocEvent(Etype::id,sizeof(Etype));
			*pDst = *pEvent; pDst->next = 0;
			(m_pEventLast ? m_pEventLast->next : m_pEventFirst) = pDst;
			m_pEventLast = pDst;
			if (pEventLogged)
				*pEventLogged = pDst;
			if (Etype::id==(const int)EventPhysPostStep::id) {
				CPhysicalPlaceholder *ppc = (CPhysicalPlaceholder*)((EventPhysPostStep*)pEvent)->pEntity;
				if (ppc->m_bProcessed & PENT_SETPOSED)
					AtomicAdd(&ppc->m_bProcessed, -PENT_SETPOSED);
			}
		}
		return res;
	}
	template<class Etype> int SignalEvent(Etype *pEvent, int bLogged) {
		int nres = 0;
		EventClient *pClient;
		for(pClient = m_pEventClients[Etype::id][bLogged]; pClient; pClient=pClient->next)
			nres += pClient->OnEvent(pEvent);
		return nres;
	}

	virtual int SerializeWorld(const char *fname, int bSave);
	virtual int SerializeGeometries(const char *fname, int bSave);

	virtual IPhysicalEntity *AddGlobalArea();
	virtual IPhysicalEntity *AddArea(Vec3 *pt,int npt, float zmin,float zmax, const Vec3 &pos=Vec3(0,0,0), const quaternionf &q=quaternionf(), 
		float scale=1.0f, const Vec3& normal=Vec3(ZERO), int *pTessIdx=0,int nTessTris=0,Vec3 *pFlows=0);
	virtual IPhysicalEntity *AddArea(IGeometry *pGeom, const Vec3& pos,const quaternionf &q,float scale);
	virtual IPhysicalEntity *AddArea(Vec3 *pt,int npt, float r, const Vec3 &pos,const quaternionf &q,float scale);
	virtual void RemoveArea(IPhysicalEntity *pArea);
	virtual int CheckAreas(const Vec3 &ptc, Vec3 &gravity, pe_params_buoyancy *pb, int nMaxBuoys=1, const Vec3 &vel=Vec3(ZERO), 
		IPhysicalEntity *pent=0, int iCaller=get_iCaller_int());
	int CheckAreas(CPhysicalEntity *pent, Vec3 &gravity, pe_params_buoyancy *pb,int nMaxBuoys=1, const Vec3 &vel=Vec3(ZERO), int iCaller=get_iCaller_int()) {
		if (!m_pGlobalArea || pent->m_flags & pef_ignore_areas)
			return 0;
		return CheckAreas((pent->m_BBox[0]+pent->m_BBox[1])*0.5f, gravity, pb, nMaxBuoys, vel, pent, iCaller);
	}
	void RepositionArea(CPhysArea *pArea);
	virtual IPhysicalEntity *GetNextArea(IPhysicalEntity *pPrevArea=0);

	virtual void SetWaterMat(int imat);
	virtual int GetWaterMat() { return m_matWater; }
	virtual int SetWaterManagerParams(pe_params *params);
	virtual int GetWaterManagerParams(pe_params *params);
	virtual int GetWatermanStatus(pe_status *status);
	virtual void DestroyWaterManager();

	virtual int AddExplosionShape(IGeometry *pGeom, float size,int idmat, float probability=1.0f);
	virtual void RemoveExplosionShape(int id);
	virtual void RemoveAllExplosionShapes(void (*OnRemoveGeom)(IGeometry *pGeom)=0) {
		for(int i=0;i<m_nExpl;i++) {
			if (OnRemoveGeom) OnRemoveGeom(m_pExpl[i].pGeom);
			m_pExpl[i].pGeom->Release();
		}	m_nExpl=m_idExpl = 0;
	}
	IGeometry *GetExplosionShape(float size,int idmat, float &scale, int &bCreateConstraint);
	int DeformEntityPart(CPhysicalEntity *pent,int i, pe_explosion *pexpl, geom_world_data *gwd,geom_world_data *gwd1, int iSource=0);
	void MarkEntityAsDeforming(CPhysicalEntity *pent);
	void UnmarkEntityAsDeforming(CPhysicalEntity *pent);
	void ClonePhysGeomInEntity(CPhysicalEntity *pent,int i,IGeometry *pNewGeom);

	void AllocRequestsQueue(int sz) {
		if (m_nQueueSlotSize+sz+1 > QUEUE_SLOT_SZ) {
			if (m_nQueueSlots==m_nQueueSlotsAlloc)
				ReallocateList(m_pQueueSlots, m_nQueueSlots,m_nQueueSlotsAlloc+=8, true);
			if (!m_pQueueSlots[m_nQueueSlots])
				m_pQueueSlots[m_nQueueSlots] = new char[max(sz+1,QUEUE_SLOT_SZ)];
			++m_nQueueSlots; m_nQueueSlotSize = 0;
		}
	}
	SPU_NO_INLINE void *QueueData(void *ptr, int sz) {
		void *storage = m_pQueueSlots[m_nQueueSlots-1]+m_nQueueSlotSize;
		memcpy(storage, ptr, sz);	m_nQueueSlotSize += sz;
		*(int*)(m_pQueueSlots[m_nQueueSlots-1]+m_nQueueSlotSize) = -1;
		return storage;
	}
	template<class T> SPU_NO_INLINE T* QueueData(const T& data) {		
		T &storage = *(T*)(m_pQueueSlots[m_nQueueSlots-1]+m_nQueueSlotSize);
		storage = data;	
		m_nQueueSlotSize += sizeof(data);
		*(int*)(m_pQueueSlots[m_nQueueSlots-1]+m_nQueueSlotSize) = -1;
		return &storage;
	}

	entity_contact *AllocContact();
	void FreeContact(entity_contact *pContact);

	float GetFriction(int imat0,int imat1,int bDynamic=0) {
		float *pTable = (float*)((intptr_t)m_FrictionTable&~(intptr_t)-bDynamic | (intptr_t)m_DynFrictionTable&(intptr_t)-bDynamic);
		return max(0.0f,pTable[imat0 & NSURFACETYPES-1]+pTable[imat1 & NSURFACETYPES-1])*0.5f;
	}
	float GetBounciness(int imat0,int imat1) {
		return (m_BouncinessTable[imat0 & NSURFACETYPES-1]+m_BouncinessTable[imat1 & NSURFACETYPES-1])*0.5f;
	}

	virtual void SavePhysicalEntityPtr(TSerialize ser, IPhysicalEntity* pIEnt);
	virtual IPhysicalEntity* LoadPhysicalEntityPtr(TSerialize ser);
  virtual void GetEntityMassAndCom(IPhysicalEntity* pIEnt, float& mass, Vec3& com);

	virtual void SetDynPoolSize(int size) { ::SetDynPoolSize(size); }
	virtual int FlushDynPool() { return ::FlushDynPool(); }

	IPhysicalWorld *GetIWorld() { return this; }

	virtual void SerializeGarbageTypedSnapshot( TSerialize ser, int iSnapshotType, int flags );

	int GetTmpEntList(CPhysicalEntity **&pEntList, int iCaller)	{
		INT_PTR plist = (INT_PTR)m_threadData[iCaller].pTmpEntList; 
		int is0=iCaller-1>>31, isN=MAX_PHYS_THREADS-iCaller-1>>31;
		plist += (INT_PTR)m_pTmpEntList -plist & is0;
		plist += (INT_PTR)m_pTmpEntList2-plist & isN;
		m_threadData[iCaller].pTmpEntList = (CPhysicalEntity**)plist;
		m_threadData[iCaller].szList += m_nEntsAlloc-m_threadData[iCaller].szList & (is0|isN);
		pEntList = m_threadData[iCaller].pTmpEntList;
		return m_threadData[iCaller].szList;
	}
	int ReallocTmpEntList(CPhysicalEntity **&pEntList, int iCaller, int szNew);

	void ProcessNextEntityIsland(float time_interval, int ipass, int iter, int &bAllGroupsFinished, int iCaller);
	void ProcessIslandSolverResults(int i, int iter, float groupTimeStep,float Ebefore, int nEnts,float fixedDamping, int &bAllGroupsFinished,
		entity_contact **pContacts,int nContacts,int nBodies, int iCaller,int iticks0);
	int ReadDelayedSolverResults(CMemStream &stm, float &dt,float &Ebefore,int &nEnts,float &fixedDamping, entity_contact **pContacts,RigidBody **pBodies);
	void ProcessNextEngagedIndependentEntity(int iCaller);
	void ProcessNextLivingEntity(float time_interval, int bSkipFlagged, int iCaller);
	void ProcessNextIndependentEntity(float time_interval, int bSkipFlagged, int iCaller);
	void ProcessBreakingEntities(float time_interval);
	void ThreadProc(int ithread, SPhysTask *pTask);

	template<class T> void ReallocQueue(T *&pqueue, int sz,int &szAlloc, int &head,int &tail, int nGrow) {
		if (sz==szAlloc) {
			T *pqueueOld = pqueue;
			pqueue = new T[szAlloc+nGrow];
			memcpy(pqueue, pqueueOld, (head+1)*sizeof(T));
			memcpy(pqueue+head+nGrow+1, pqueueOld+head+1, (sz-head-1)*sizeof(T));
			if (tail) tail += nGrow;
			szAlloc += nGrow; 
			if (pqueueOld) delete[] pqueueOld;
		}
		head = head+1 - (szAlloc & szAlloc-2-head>>31);
	}

	PhysicsVars m_vars;
	ILog *m_pLog;
	IPhysicsStreamer *m_pPhysicsStreamer;
	IPhysicsEventClient *m_pEventClient;
	IPhysRenderer *m_pRenderer;

	CPhysicalEntity *m_pTypedEnts[8],*m_pTypedEntsPerm[8];
	CPhysicalEntity **m_pTmpEntList,**m_pTmpEntList1,**m_pTmpEntList2;
	CPhysicalEntity *m_pHiddenEnts;
	float *m_pGroupMass,*m_pMassList;
	int *m_pGroupIds,*m_pGroupNums;
	grid m_entgrid;
	int m_iEntAxisz;
	float m_zGran,m_rzGran;
	pe_entgrid m_pEntGrid;
	pe_gridthunk *m_gthunks;
	int m_thunkPoolSz,m_iFreeGThunk0;
	pe_PODcell **m_pPODCells,m_dummyPODcell,*m_pDummyPODcell;
	vector2di m_PODstride;
	int m_log2PODscale;
	int m_bHasPODGrid,m_iActivePODCell0;
	int m_nEnts,m_nEntsAlloc;
	int m_nDynamicEntitiesDeleted;
	CPhysicalPlaceholder **m_pEntsById;
	int m_nIdsAlloc, m_iNextId;
	int m_iNextIdDown,m_lastExtId,m_nExtIds;
	int m_bGridThunksChanged;
	int m_bUpdateOnlyFlagged;
	int m_nTypeEnts[10];
	int m_bEntityCountReserved;
	volatile int m_nGEA[MAX_PHYS_THREADS+1];
	volatile int m_nEntListAllocs;
	int m_nOnDemandListFailures;
	int m_iLastPODUpdate;
	Vec3 m_prevGEABBox[MAX_PHYS_THREADS+1][2];
	int m_prevGEAobjtypes[MAX_PHYS_THREADS+1];
	int m_nprevGEAEnts[MAX_PHYS_THREADS+1];

	int m_nPlaceholders,m_nPlaceholderChunks,m_iLastPlaceholder;
	CPhysicalPlaceholder **m_pPlaceholders;
	int *m_pPlaceholderMap;
	CPhysicalEntity *m_pEntBeingDeleted;

	int *m_pGridStat[6],*m_pGridDyn[6];
	int m_nOccRes;
	Vec3 m_lastEpicenter,m_lastEpicenterImp,m_lastExplDir;
	float m_lastRmax;
	CPhysicalEntity **m_pExplVictims;
	float *m_pExplVictimsFrac;
	Vec3 *m_pExplVictimsImp;
	int m_nExplVictims,m_nExplVictimsAlloc;

	CPhysicalEntity *m_pHeightfield[MAX_PHYS_THREADS+2];
	Matrix33 m_HeightfieldBasis;
	Vec3 m_HeightfieldOrigin;

	float m_timePhysics,m_timeSurplus,m_timeSnapshot[4];
	int m_iTimePhysics,m_iTimeSnapshot[4];
	float m_updateTimes[8];
	int m_iSubstep,m_bWorldStep;
	int m_bCurGroupInvisible;
	float m_curGroupMass;
	CPhysicalEntity *m_pAuxStepEnt;
	phys_profile_info m_pEntProfileData[16];
	int m_nProfiledEnts;
	phys_profile_info *m_pFuncProfileData;
	int m_nProfileFunx,m_nProfileFunxAlloc;
	volatile int m_lockEntProfiler,m_lockFuncProfiler;
	float m_lastTimeInterval;
	volatile int m_idThread;
	volatile int m_idPODThread;
	int m_bMassDestruction;

	float m_BouncinessTable[NSURFACETYPES];
	float m_FrictionTable[NSURFACETYPES];
	float m_DynFrictionTable[NSURFACETYPES];
	float m_DamageReductionTable[NSURFACETYPES];
	float m_RicochetAngleTable[NSURFACETYPES];
	float m_RicDamReductionTable[NSURFACETYPES];
	float m_RicVelReductionTable[NSURFACETYPES];
	unsigned int m_SurfaceFlagsTable[NSURFACETYPES];
	int m_matWater,m_bCheckWaterHits;
	class CWaterMan *m_pWaterMan;
	Vec3 m_posViewer;

	char **m_pQueueSlots;
	int m_nQueueSlots,m_nQueueSlotsAlloc;
	int m_nQueueSlotSize;

	CPhysArea *m_pGlobalArea;
	int m_nAreas,m_nBigAreas;
	CPhysArea *m_pDeletedAreas;

	EventPhys *m_pEventFirst,*m_pEventLast,*m_pFreeEvents[EVENT_TYPES_NUM];
	EventClient *m_pEventClients[EVENT_TYPES_NUM][2];
	EventChunk *m_pFirstEventChunk,*m_pCurEventChunk;
	int m_szCurEventChunk;
	int m_nEvents[EVENT_TYPES_NUM];
	volatile int m_idStep;

	entity_contact *m_pFreeContact,*m_pLastFreeContact;
	int m_nFreeContacts;
	int m_nContactsAlloc;

	SExplosionShape *m_pExpl;
	int m_nExpl,m_nExplAlloc,m_idExpl;
	CPhysicalEntity **m_pDeformingEnts;
	int m_nDeformingEnts,m_nDeformingEntsAlloc;
	SBreakRequest *m_breakQueue;
	int m_breakQueueHead,m_breakQueueTail;
	int m_breakQueueSz,m_breakQueueAlloc;

	SRwiRequest *m_rwiQueue;
	int m_rwiQueueHead,m_rwiQueueTail;
	int m_rwiQueueSz,m_rwiQueueAlloc;
	ray_hit *m_pRwiHitsHead,*m_pRwiHitsTail;
	int m_rwiHitsPoolSize;
	int m_rwiPoolEmpty;

	SPwiRequest *m_pwiQueue;
	int m_pwiQueueHead,m_pwiQueueTail;
	int m_pwiQueueSz,m_pwiQueueAlloc;

	SThreadTaskRequest m_rq;
	CryEvent m_threadStart[MAX_PHYS_THREADS],m_threadDone[MAX_PHYS_THREADS];
	SThreadData m_threadData[MAX_PHYS_THREADS+1];
	SPhysTask *m_threads[MAX_PHYS_THREADS];
	Vec3 m_BBoxPlayerGroup[MAX_PHYS_THREADS+1][2];
	int m_nGroups;
	float m_maxGroupMass;
	volatile int __atomic_align m_nWorkerThreads;
	volatile int __atomic_align m_iCurGroup;
	volatile __atomic_align CPhysicalEntity *m_pCurEnt;
	volatile __atomic_align CPhysicalEntity *m_pMovedEnts;
	volatile int __atomic_align m_lockNextEntityGroup;
	volatile int __atomic_align m_lockMovedEntsList;
	volatile int __atomic_align m_lockPlayerGroups;

	volatile int __atomic_align m_lockPODGrid;
	volatile int __atomic_align m_lockEntIdList;
	volatile int __atomic_align m_lockStep,m_lockGrid __atomic_align, m_lockCaller[MAX_PHYS_THREADS+1] __atomic_align ,m_lockQueue __atomic_align,m_lockList __atomic_align;
	volatile int __atomic_align m_lockAreas;
	volatile int __atomic_align m_lockEventsQueue,m_iLastLogPump;
	volatile int __atomic_align m_lockDeformingEntsList;
	volatile int __atomic_align m_lockRwiQueue;
	volatile int __atomic_align m_lockRwiHitsPool;
	volatile int __atomic_align m_lockTPR;
	volatile int __atomic_align m_lockPwiQueue;
	volatile int __atomic_align m_lockContacts;
	volatile int __atomic_align m_lockBreakQueue;
	volatile int __atomic_align m_lockAuxStepEnt;
	volatile int __atomic_align m_lockPotentialMeshUpdate;			// ps3-only
	volatile int __atomic_align m_lockPotentialPartListUpdate;	// ps3-only

	int StartRWIJob();
	int EndRWIJob();
	void RWIJob();
	SRwiRequest *m_jobRWI;
	int m_nJobRWIs,m_nJobRWIsDone,m_nJobRWIsAlloc;
};
#endif//__SPU__

class CPhysArea : public CPhysicalPlaceholder {
public:
	CPhysArea(CPhysicalWorld *pWorld);
	~CPhysArea();
	virtual pe_type GetType() const { return PE_AREA; }
	virtual CPhysicalEntity *GetEntity();
	virtual CPhysicalEntity *GetEntityFast() { return GetEntity(); }
	virtual int AddRef() { CryInterlockedAdd(&m_lockRef, 1); return m_lockRef; }
	virtual int Release() { CryInterlockedAdd(&m_lockRef, -1); return m_lockRef; }

	virtual int SetParams(pe_params* params,int bThreadSafe=1);
	virtual int GetParams(pe_params* params) const;
	virtual int GetStatus(pe_status* status) const;
	virtual IPhysicalWorld *GetWorld() const { return (IPhysicalWorld*)m_pWorld; }
	SPU_NO_INLINE
	int CheckPoint(const Vec3& pttest) ppu_const;
	int ApplyParams(const Vec3& pt, Vec3& gravity,const Vec3 &vel, pe_params_buoyancy *pbdst,int nBuoys,int nMaxBuoys,int &iMedium0, IPhysicalEntity *pent) const;
	SPU_NO_INLINE
	float FindSplineClosestPt(const Vec3 &ptloc, int &iClosestSeg,float &tClosest) ppu_const;
	SPU_NO_INLINE
	int FindSplineClosestPt(const Vec3 &org, const Vec3 &dir, Vec3 &ptray, Vec3 &ptspline) ppu_const;
	virtual void DrawHelperInformation(IPhysRenderer *pRenderer, int flags);
	int RayTraceInternal(const Vec3 &org, const Vec3 &dir, ray_hit *pHit, pe_params_pos *pp);
	int RayTrace(const Vec3 &org, const Vec3 &dir, ray_hit *pHit, pe_params_pos *pp=SPU_LOCAL_PTR((pe_params_pos*)0))
	{
#ifdef __SPU__
		//avoid calls to main memory phys areas -> memcpy back and forth
		char localBuf[sizeof(CPhysArea)] _ALIGN(128);
		memcpy(localBuf, this, sizeof(*this));
		SPU_DOMAIN_LOCAL CPhysArea * pLocalArea = (CPhysArea*)localBuf;
		int retVal = pLocalArea->RayTraceInternal(org, dir, pHit, pp);
		memcpy(this, localBuf, sizeof(*this));
		return retVal;
#else
		return RayTraceInternal(org, dir, pHit, pp);
#endif
	}

	virtual float ComputeExtent(GeomQuery& geo, EGeomForm eForm) const;
	virtual void GetRandomPos(RandomPos& ran, GeomQuery& geo, EGeomForm eForm) const;

	virtual void GetMemoryStatistics(ICrySizer *pSizer) const;

	int m_bDeleted;
	CPhysArea *m_next,*m_nextBig;
	CPhysicalWorld *m_pWorld;

	Vec3 m_offset;
	Matrix33 m_R;
	float m_scale,m_rscale;

	Vec3 m_offset0;
	Matrix33 m_R0;

	IGeometry *m_pGeom;
	Vec2 *m_pt;
	int m_npt;
	float m_zlim[2];
	int *m_idxSort[2];
	Vec3 *m_pFlows;
	unsigned int *m_pMask;
	Vec3 m_size0;
	Vec3 *m_ptSpline;
	mutable Vec3 m_ptLastCheck;
	mutable int m_iClosestSeg;
	mutable float m_tClosest,m_mindist;
	mutable indexed_triangle m_trihit;

	pe_params_buoyancy m_pb;
	Vec3 m_gravity;
	Vec3 m_size,m_rsize;
	int m_bUniform;
	float m_falloff0,m_rfalloff0;
	float m_damping;
	int m_bUseCallback;
	mutable volatile int __atomic_align m_lockRef;
} _ALIGN(128);

#ifndef __SPU__
extern int g_nPhysWorlds;
extern CPhysicalWorld *g_pPhysWorlds[];

#ifndef XENON
inline int IsPODThread(CPhysicalWorld *pWorld) { return iszero((int)CryGetCurrentThreadId()-pWorld->m_idPODThread); }
inline void MarkAsPODThread(CPhysicalWorld *pWorld) { pWorld->m_idPODThread = CryGetCurrentThreadId(); }
inline void UnmarkAsPODThread(CPhysicalWorld *pWorld) { pWorld->m_idPODThread = -1; }
#else
extern int g_idxThisIsPODThread; 
inline int IsPODThread(CPhysicalWorld*) { return (int)TlsGetValue(g_idxThisIsPODThread); }
inline void MarkAsPODThread(CPhysicalWorld*) { TlsSetValue(g_idxThisIsPODThread,(void*)1); }
inline void UnmarkAsPODThread(CPhysicalWorld*) { TlsSetValue(g_idxThisIsPODThread,(void*)0); }
#endif

struct CPhysEntityProfiler {
	int64 m_iStartTime;
	CPhysicalEntity *m_pEntity;

	CPhysEntityProfiler(CPhysicalEntity *pent) {
		m_pEntity = pent;
		m_iStartTime = CryGetTicks();
	}
	~CPhysEntityProfiler() {
		if (m_pEntity->m_pWorld->m_vars.bProfileEntities)
			m_pEntity->m_pWorld->AddEntityProfileInfo(m_pEntity,(int)(CryGetTicks()-m_iStartTime));
	}
};

struct CPhysFuncProfiler {
	int64 m_iStartTime;
	const char *m_name;
	CPhysicalWorld *m_pWorld;

	CPhysFuncProfiler(const char *name, CPhysicalWorld *pWorld) {
		m_name = name; m_pWorld = pWorld;
		m_iStartTime = CryGetTicks();
	}
	~CPhysFuncProfiler() {
		if (m_pWorld->m_vars.bProfileFunx)
			m_pWorld->AddFuncProfileInfo(m_name, (int)(CryGetTicks()-m_iStartTime));
	}
};

struct CBlockProfiler {
	int64 &m_time;
	CBlockProfiler(int64 &time) : m_time(time) { time=CryGetTicks(); }
	~CBlockProfiler() { m_time = CryGetTicks()-m_time; }
};

template<class T> inline bool StructChangesPos(T*) { return false; }
inline bool StructChangesPos(pe_params *params) { 
	pe_params_pos *pp = (pe_params_pos*)params;
	return params->type==pe_params_pos::type_id && (!is_unused(pp->pos) || !is_unused(pp->pMtx3x4) && pp->pMtx3x4) && !(pp->bRecalcBounds & 16); 
}

template<class T> inline void OnStructQueued(T *params, CPhysicalWorld *pWorld, void *ptrAux,int iAux) {}
inline void OnStructQueued(pe_geomparams *params, CPhysicalWorld *pWorld, void *ptrAux,int iAux) { pWorld->AddRefGeometry((phys_geometry*)ptrAux); }

template<class T> struct ChangeRequest {
	CPhysicalWorld *m_pWorld;
	int m_bQueued,m_bLocked,m_bLockedCaller;
	T *m_pQueued;

	SPU_NO_INLINE ChangeRequest(CPhysicalPlaceholder *pent, CPhysicalWorld *pWorld, T *params, int bInactive, void *ptrAux=0,int iAux=0) : 
	m_pWorld(pWorld),m_bQueued(0),m_bLocked(0),m_bLockedCaller(0) 
	{
		if (bInactive<=0 && pent->m_iSimClass!=7) {
			if (m_pWorld->m_lockStep || m_pWorld->m_lockTPR || pent->m_bProcessed>=PENT_QUEUED || bInactive<0) {
				subref *psubref;
				int szSubref,szTot;
				WriteLock lock(m_pWorld->m_lockQueue);
				AtomicAdd(&pent->m_bProcessed, PENT_QUEUED);
				for(psubref=GetSubref(params),szSubref=0; psubref; psubref=psubref->next)
					if (*(char**)((char*)params+psubref->iPtrOffs) && !is_unused(*(char**)((char*)params+psubref->iPtrOffs)))
						szSubref += ((*(int*)((char*)params+max(0,psubref->iSzOffs)) & -psubref->iSzOffs>>31) + psubref->iSzFixed)*psubref->iSzUnit;
				szTot = sizeof(int)*2+sizeof(void*)+GetStructSize(params)+szSubref;
				if (StructUsesAuxData(params))
					szTot += sizeof(void*)+sizeof(int);
				m_pWorld->AllocRequestsQueue(szTot);
				m_pWorld->QueueData(GetStructId(params));
				m_pWorld->QueueData(szTot);
				m_pWorld->QueueData(pent);
				if (StructUsesAuxData(params)) {
					m_pWorld->QueueData(ptrAux);
					m_pWorld->QueueData(iAux);
				}
				m_pQueued = (T*)m_pWorld->QueueData(params, GetStructSize(params));
				for(psubref=GetSubref(params); psubref; psubref=psubref->next) {
					szSubref = ((*(int*)((char*)params+max(0,psubref->iSzOffs)) & -psubref->iSzOffs>>31) + psubref->iSzFixed)*psubref->iSzUnit;
					char *pParams = *(char**)((char*)params+psubref->iPtrOffs);
					if (pParams && !is_unused(pParams) && szSubref>=0)
						*(void**)((char*)m_pQueued+psubref->iPtrOffs) = m_pWorld->QueueData(pParams, szSubref);
				}
				OnStructQueued(params,pWorld,ptrAux,iAux);
				m_bQueued = 1;
			}	else {
				if (!IsPODThread(m_pWorld))
					SpinLock(&m_pWorld->m_lockCaller[MAX_PHYS_THREADS], 0, m_bLockedCaller=WRITE_LOCK_VAL);
				SpinLock(&m_pWorld->m_lockStep, 0, m_bLocked=WRITE_LOCK_VAL);
			}
		} 
		if (StructChangesPos(params) && !(pent->m_bProcessed & PENT_SETPOSED))
			AtomicAdd(&pent->m_bProcessed, PENT_SETPOSED);
	}
	~ChangeRequest() { AtomicAdd(&m_pWorld->m_lockStep,-m_bLocked);AtomicAdd(&m_pWorld->m_lockCaller[MAX_PHYS_THREADS],-m_bLockedCaller); }
	int IsQueued() { return m_bQueued; }
	T *GetQueuedStruct() { return m_pQueued; }
};

#endif//__SPU__

class CRayGeom;
struct geom_contact;
//struct passing through virtual RayTrace, spu local store references to pointers currently impossible with crycg
//do not process lock
struct SRayTraceRes
{
	SPU_DOMAIN_LOCAL const CRayGeom *pRay;
	SPU_DOMAIN_LOCAL geom_contact *pcontacts;
#ifdef __SPU__
	ILINE SRayTraceRes(const CRayGeom *const ray,geom_contact *contacts,volatile int*&)
		: pRay(ray),pcontacts(contacts){}
	ILINE SRayTraceRes(const CRayGeom *const ray,geom_contact *contacts,int*&)
		: pRay(ray),pcontacts(contacts){}
	ILINE void SetLock(volatile int*){}
#else
	SPU_DOMAIN_LOCAL volatile int *&plock;
	ILINE SRayTraceRes(const CRayGeom *const ray,geom_contact *contacts,volatile int *&lock)
		: pRay(ray),pcontacts(contacts),plock(lock){}
	ILINE void SetLock(volatile int* lock){plock = lock;}
#endif
};
#endif
