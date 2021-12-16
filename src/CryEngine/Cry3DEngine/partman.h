////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   partman.h
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//	- 03:2006				 : Modified by Jan Müller (Serialization)
//
////////////////////////////////////////////////////////////////////////////

#ifndef PART_MANAGER
#define PART_MANAGER

#include "ParticleEffect.h"
#include "BitFiddling.h"
#include "CryList.h"
#include "PoolAllocator.h"

class CParticleEmitter;
class CParticleContainer;

struct CSpinLock
{
	CSpinLock()
		: m_Lock(0)
	{}
	operator volatile int&()
		{ return m_Lock; }
protected:
	volatile int	m_Lock;
};

//////////////////////////////////////////////////////////////////////////
// Specify particle heap allocator.
// Specify heap block size, and for now, allow multiple blocks.
static const size_t nPARTICLE_HEAP_SIZE = 1<<20;
static const bool bMEMORY_LIMIT = false;

typedef stl::HeapAllocator<stl::PSyncNone, !bMEMORY_LIMIT> CParticleHeapBase;
struct CParticleHeap: CParticleHeapBase
{
	CParticleHeap()
		: CParticleHeapBase(nPARTICLE_HEAP_SIZE) {}
};

typedef stl::PoolCommonAllocator<CParticleHeap> ParticleAllocator;

//////////////////////////////////////////////////////////////////////////
// Top class of particle system
class CPartManager : public Cry3DEngineBase, public IParticleManager, public IVisAreaCallback
{
public:
  CPartManager();
  ~CPartManager();

	// Provide access to extended CPartManager functions only to particle classes.
	static CPartManager* GetManager()
	{
		return static_cast<CPartManager*>(m_pPartManager);
	}

	//////////////////////////////////////////////////////////////////////////
	// IParticleManager interface
	IParticleEffect* CreateEffect();
	void DeleteEffect( IParticleEffect* pEffect );
	IParticleEffect* FindEffect( cstr sEffectName, cstr sSource = "", bool bLoad = true );

	IParticleEffect* LoadEffect( cstr sEffectName, XmlNodeRef& effectNode, bool bLoadResources )
	{
		return LoadEffect( sEffectName, effectNode, true, bLoadResources);
	}
	bool LoadLibrary( cstr sParticlesLibrary, XmlNodeRef& libNode, bool bLoadResources );
	bool LoadLibrary( cstr sParticlesLibrary, cstr sParticlesLibraryFile = NULL, bool bLoadResources = false );
	void ClearCachedLibraries();

	IParticleEmitter* CreateEmitter( bool bIndependent, Matrix34 const& mLoc, const ParticleParams& Params )
	{
		return CreateEmitter( bIndependent, mLoc, NULL, &Params );
	}
	void DeleteEmitter( IParticleEmitter *pEmitter );

	// Processing
	void Update();
	void RenderDebugInfo();
	void RenderSpuUsage() const;
	void OnStartRuntime();
	void OnFrameStart();
	void Reset( bool bIndependentOnly );
	void ClearRenderResources( bool bForceClear );
	void Serialize(TSerialize ser);
	void PostSerialize( bool bReading );

	// Stats
	void GetMemoryUsage( ICrySizer* pSizer );
	void GetCounts( SParticleCounts& counts );
	void PrintParticleList();
	void PrintParticleMemory();

	// Summary:
	//	 Registers new particle events listener.
	virtual void AddEventListener(IParticleEffectListener *pListener);
	virtual void RemoveEventListener(IParticleEffectListener *pListener);

	//////////////////////////////////////////////////////////////////////////
	// Particle effects.
	//////////////////////////////////////////////////////////////////////////
	void RenameEffect( CParticleEffect* pEffect, cstr sNewName );
	XmlNodeRef ReadLibrary( cstr sParticlesLibrary );

	// Whether params are selected for current config.
	bool IsActive( ParticleParams const& params ) const;

	//////////////////////////////////////////////////////////////////////////
	// Emitters.
	//////////////////////////////////////////////////////////////////////////
	IParticleEmitter* CreateEmitter( bool bIndependent, Matrix34 const& mLoc, const IParticleEffect* pEffect, const ParticleParams* pParams = NULL );
	void UpdateEmitters( IParticleEffect *pEffect );

	//////////////////////////////////////////////////////////////////////////
	// Other methods
	IMaterial* GetLightShader() const
	{
		return m_pPartLightShader;
	}

	static AABB const& GetUpdatedAreaBB()
	{
		return m_bbAreasChanged;
	}

	IParticleEffect* LoadEffect( cstr sEffectName, XmlNodeRef& effectNode, bool bLoadXML, bool bLoadResources );

	bool KillEmitter( CParticleEmitter *pEmitter );

	//
	// Debugging and stats.
	//
	void ListEmitters( cstr sDesc );

#ifdef bEVENT_TIMINGS
	struct SEventTiming
	{
		const CParticleEffect* pEffect;
		uint32 nContainerId;
		int nThread;
		cstr sEvent;
		float	timeStart, timeEnd;
	};
	int AddEventTiming( cstr sEvent, const CParticleContainer* pCont );

	inline int StartEventTiming( cstr sEvent, const CParticleContainer* pCont )
	{
		if (pCont && (GetCVars()->e_ParticlesDebug & AlphaBits('ed')))
		{
			WriteLock lock(m_EventLock);
			return AddEventTiming( sEvent, pCont ) * m_iEventSwitch;
		}
		else
			return 0;
	}
	inline void EndEventTiming( int iEvent )
	{
		WriteLock lock(m_EventLock);
		iEvent *= m_iEventSwitch;
		if (iEvent > 0)
			m_aEvents[iEvent].timeEnd = GetTimer()->GetAsyncCurTime();
	}
#endif

	//
	// Fill rate limiting.
	//
	float GetMaxContainerPixels() const
	{
		return m_fMaxContainerPixels;
	}

	CTimeValue GetLastUpdateTime() const
	{
		return m_tLastUpdate;
	}

private:

	struct CCompCStr
	{
		bool operator()( cstr a, cstr b ) const
		{
			return stricmp(a, b) < 0;
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Map of particle effect case-insensitive name to interface pointer.
	// The name key points to the name string in the actual effect,
	// so there is no string duplication.
	typedef std::map< cstr, _smart_ptr<CParticleEffect>, CCompCStr > TEffectsList;
	typedef std::list<IParticleEffectListener*> TListenersList;
	TEffectsList								m_Effects;
	TListenersList							m_ListenersList;

	//////////////////////////////////////////////////////////////////////////
	// Loaded particle libs.
	std::map< string, XmlNodeRef, stl::less_stricmp<string> > m_LoadedLibs;

	//////////////////////////////////////////////////////////////////////////
	// Particle effects emitters, top-level only.
	//////////////////////////////////////////////////////////////////////////
	List<CParticleEmitter, ParticleAllocator>
															m_Emitters;

	bool												m_bRuntime;
	CTimeValue									m_tLastUpdate;
	_smart_ptr<IMaterial>			  m_pPartLightShader;
	float												m_fMaxContainerPixels;			// Per-frame computed fill-rate limit per container.

	// Listener for physics events. Static data will be fine, there's just one physics system.
	static int OnPhysAreaChange(const EventPhys *pEvent);
	static AABB m_bbAreasChanged;

	// Listener for vis area events.
	virtual void OnVisAreaDeleted( IVisArea* pVisArea );

	CParticleEmitter* EraseEmitter( CParticleEmitter *pEmitter );

	// Console command interface.
	static void CmdParticleList( IConsoleCmdArgs* )
	{
		// Trigger stat gathering for next frame.
		GetCVars()->e_ParticlesDebug |= AlphaBit('s');
	}
	static void CmdParticleMemory( IConsoleCmdArgs* )
	{ 
		((CPartManager*)m_pPartManager)->PrintParticleMemory(); 
	}

#ifdef bEVENT_TIMINGS
	DynArray<SEventTiming>		m_aEvents;
	int												m_iEventSwitch;
	CSpinLock									m_EventLock;
	float											m_timeThreshold;

	void LogEvents();
#endif
};

#ifdef bEVENT_TIMINGS

class CEventProfilerSection: public CFrameProfilerSection, public Cry3DEngineBase
{
public:
	CEventProfilerSection( CFrameProfiler *pProfiler, const CParticleContainer* pCont = 0 )
	: CFrameProfilerSection(pProfiler)
	{
		m_iEvent = m_pPartManager->StartEventTiming(pProfiler->m_name, pCont);
	}
	~CEventProfilerSection()
	{
		m_pPartManager->EndEventTiming(m_iEvent);
	}
protected:
	int m_iEvent;
};

	#define FUNCTION_PROFILER_CONTAINER(pCont) \
		static CFrameProfiler staticFrameProfiler( gEnv->pSystem, __FUNC__, PROFILE_PARTICLE ); \
		CEventProfilerSection eventProfilerSection( &staticFrameProfiler, pCont );

#else

	#define FUNCTION_PROFILER_CONTAINER(pCont)	FUNCTION_PROFILER_SYS(PARTICLE)

#endif

#endif // PART_MANAGER
	
