////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   partman.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: manage particles and sprites
// -------------------------------------------------------------------------
//  History:
//	- 03:2006				 : Modified by Jan Müller (Serialization)
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "partman.h"
#include "ParticleEmitter.h"
#include "3dEngine.h"


#define LIBRARY_PATH			"Libs/"
#define EFFECTS_SUBPATH		LIBRARY_PATH "Particles/"
#define LEVEL_PATH				"Levels/"































































































//////////////////////////////////////////////////////////////////////////
struct CNullPartManager: IParticleManager
{
	virtual IParticleEffect* CreateEffect() { return 0; }
	virtual void DeleteEffect( IParticleEffect* pEffect ) {}
	virtual IParticleEffect* FindEffect( cstr sEffectName, cstr sSource = "", bool bLoadResources = true ) { return 0; }
	virtual IParticleEffect* LoadEffect( cstr sEffectName, XmlNodeRef& effectNode, bool bLoadResources ) { return 0; }
	virtual bool LoadLibrary( cstr sParticlesLibrary, XmlNodeRef& libNode, bool bLoadResources ) { return false; }
	virtual bool LoadLibrary( cstr sParticlesLibrary, cstr sParticlesLibraryFile = NULL, bool bLoadResources = false ) { return false; }

	virtual IParticleEmitter* CreateEmitter( bool bIndependent, Matrix34 const& mLoc, const ParticleParams& Params ) { return 0; }
	virtual void DeleteEmitter( IParticleEmitter * pPartEmitter ) {}

	virtual void Update() {}
	virtual void RenderDebugInfo() {}
	virtual void Reset( bool bIndependentOnly ) {}
	virtual void ClearRenderResources( bool bForceClear ) {}
	virtual void OnStartRuntime() {}
	virtual void OnFrameStart() {}
	virtual void Serialize( TSerialize ser ) {}
	virtual void PostSerialize( bool bReading ) {}

	virtual void GetMemoryUsage( ICrySizer* pSizer ) {}
	virtual void GetCounts( SParticleCounts& counts ) {}

	virtual void AddEventListener(IParticleEffectListener *pListener) {}
	virtual void RemoveEventListener(IParticleEffectListener *pListener) {}
};

//////////////////////////////////////////////////////////////////////////
// Single direct entry point for particle clients.
IParticleManager* CreateParticleManager(bool bEnable)
{
	if (bEnable)
		return new CPartManager();
	else
		return new CNullPartManager();
}

//////////////////////////////////////////////////////////////////////////
AABB CPartManager::m_bbAreasChanged(AABB::RESET);

//////////////////////////////////////////////////////////////////////////
CPartManager::CPartManager()
{
	m_bRuntime = false;
#ifdef bEVENT_TIMINGS
	m_iEventSwitch = 1;
	m_timeThreshold = 0.f;
#endif

	if (GetPhysicalWorld())
	{
		GetPhysicalWorld()->AddEventClient(EventPhysAreaChange::id, &OnPhysAreaChange, 0);

		REGISTER_COMMAND("e_ParticleList", &CmdParticleList, 0, "Writes all effects used and counts to TestResults/particle_list.csv");
		REGISTER_COMMAND("e_ParticleMemory", &CmdParticleMemory, 0, "Displays current particle memory usage");
	}
}

CPartManager::~CPartManager() 
{
	if (Get3DEngine()->GetIVisAreaManager())
		Get3DEngine()->GetIVisAreaManager()->RemoveListener(this);
	GetPhysicalWorld()->RemoveEventClient(EventPhysAreaChange::id, &OnPhysAreaChange, 0);
	Reset(false);
	
	// destroy reference to pPartLightShader
	m_pPartLightShader=NULL;
}

bool CPartManager::IsActive( ParticleParams const& params ) const
{
	if (!params.bEnabled)
		return false;

	int quality = GetCVars()->e_ParticlesQuality;
	if (quality == 0)
		quality = CONFIG_VERYHIGH_SPEC;
	EConfigSpecBrief eSysConfig = EConfigSpecBrief(quality - CONFIG_LOW_SPEC + ConfigSpec_Low);
	if (eSysConfig < params.eConfigMin || eSysConfig > params.eConfigMax)
		return false;
	if (!TrinaryMatch(params.tDX11, GetRenderer()->GetRenderType() == eRT_DX11))
		return false;
	return true;
}

struct SortParticleCount
{
	bool operator()(CParticleEffect* a, CParticleEffect* b)
	{
		return a->GetCounts().SumParticlesAlloc > b->GetCounts().SumParticlesAlloc;
	}
};

void CPartManager::GetCounts( SParticleCounts& counts )
{
  FUNCTION_PROFILER_SYS(PARTICLE);

	ZeroStruct(counts);
	bool bEffectStats = (GetCVars()->e_ParticlesDebug & AlphaBit('s')) != 0;
	if (bEffectStats)
	{
		for_container (TEffectsList, it, m_Effects)
			it->second->InitCounts();
	}

	for_all_ptrs (const CParticleEmitter, e, m_Emitters)
		// Count stats when debug flags indicate.
		e->GetCounts( counts, bEffectStats );

	if (bEffectStats)
	{
		// Print effect stats.
		DynArray<CParticleEffect*> aSortedEffects;
		for_container (TEffectsList, it, m_Effects)
		{
			it->second->SumCounts();
			aSortedEffects.push_back(it->second);
		}
		std::sort(aSortedEffects.begin(), aSortedEffects.end(), SortParticleCount());

		// Header for CSV-formatted emitter list.
		for_array (i, aSortedEffects)
		{
			aSortedEffects[i]->PrintCounts(i==0);
			aSortedEffects[i]->ClearCounts();
		}
		GetCVars()->e_ParticlesDebug &= ~AlphaBit('s');
	}
}

void CPartManager::GetMemoryUsage( ICrySizer* pSizer )
{
	{
	  SIZER_COMPONENT_NAME(pSizer, "Effects");
		GetContainerMemoryUsage(pSizer, m_Effects);
		GetContainerMemoryUsage(pSizer, m_LoadedLibs);
	}
	{
	  SIZER_COMPONENT_NAME(pSizer, "Emitters");
		pSizer->Add(*this);
		m_Emitters.GetMemoryUsage(pSizer);
	}
}

void CPartManager::PrintParticleMemory()
{
	// Get stats and mem usage for all effects.
	ICrySizer* pSizer = GetSystem()->CreateSizer();
	SEffectCounts eCounts;

	GetContainerMemoryUsage(pSizer, m_LoadedLibs);
	pSizer->AddContainer(m_Effects);

	for_container (TEffectsList, it, m_Effects)
	{
		if (!it->second->IsNull())
		{
			it->second->GetEffectCounts(eCounts);
			it->second->GetMemoryUsage(pSizer);
		}
	}

	CryLogAlways( "Particle effects: %d loaded, %d enabled, %d active, %d KB, %d allocs",
		eCounts.nLoaded, eCounts.nEnabled, eCounts.nActive, pSizer->GetTotalSize()>>10, pSizer->GetObjectCount());

	// Get mem usage for all emitters.
	pSizer->Reset();
	pSizer->Add(*this);
	m_Emitters.GetMemoryUsage(pSizer);

	SParticleCounts pCounts;
	ZeroStruct(pCounts);

	for_all_ptrs (const CParticleEmitter, e, m_Emitters)
		e->GetCounts(pCounts);

	CryLogAlways( "Particle emitters: %.0f emitters, %.0f particles, %d KB, %d allocs",
		pCounts.EmittersAlloc, pCounts.ParticlesAlloc, pSizer->GetTotalSize()>>10, pSizer->GetObjectCount());

	stl::SMemoryUsage mem = ParticleAllocator::GetTotalMemory();
	size_t nPool = ParticleAllocator::GetPoolMemory().nAlloc;
	CryLogAlways( "Particle heap: %d KB used, %d KB free pool, %d KB free heap, %d KB non-pool alloced", 
		mem.nUsed>>10, (nPool - mem.nUsed)>>10, (mem.nAlloc - nPool)>>10, (pSizer->GetTotalSize() - mem.nUsed)>>10);
}

void CPartManager::Reset(bool bIndependentOnly)
{
	m_bRuntime = false;

	for_all_ptrs (CParticleEmitter, e, m_Emitters)
	{
		if (!bIndependentOnly || e->IsIndependent())
		{
			e = EraseEmitter(e);
		}
	}

	if (!m_pPartLightShader)
	{
		m_pPartLightShader = MakeSystemMaterialFromShader("ParticlesNoMat");
	}
}

//////////////////////////////////////////////////////////////////////////
void CPartManager::ClearRenderResources( bool bForceClear )
{
	Reset(false);

	if (!GetCVars()->e_ParticlesPreload || bForceClear)
	{
		m_Effects.clear();
		m_LoadedLibs.clear();
		m_Emitters.clear();
	}

	CParticleEffect *pEffect = CParticleEffect::get_intrusive_list_root();
	while (pEffect)
	{
		pEffect->UnloadResources(false);
		pEffect = pEffect->m_next_intrusive;
	}
	
	if (bForceClear)
	{
/*
		//////////////////////////////////////////////////////////////////////////
		// Delete not released yet particle effects,
		//////////////////////////////////////////////////////////////////////////
		CParticleEffect *pEffect = CParticleEffect::get_intrusive_list_root();
		while (pEffect)
		{
			CParticleEffect *pNext = pEffect->m_next_intrusive;
			delete pEffect;
			pEffect = pNext;
		}
		*/
	}
	m_pPartLightShader = 0;
}

//////////////////////////////////////////////////////////////////////////
void CPartManager::AddEventListener(IParticleEffectListener *pListener)
{
	assert(pListener);

	if (pListener)
		stl::push_back_unique(m_ListenersList, pListener);
}

void CPartManager::RemoveEventListener(IParticleEffectListener *pListener)
{
	assert(pListener);

	m_ListenersList.remove(pListener);
}

//////////////////////////////////////////////////////////////////////////
// Particle Effects.
//////////////////////////////////////////////////////////////////////////
IParticleEffect* CPartManager::CreateEffect()
{
	return new CParticleEffect();
}

//////////////////////////////////////////////////////////////////////////
void CPartManager::RenameEffect( CParticleEffect* pEffect, cstr sNewName )
{
	assert( pEffect );

	IParticleEffect* pParent = pEffect->GetParent();
	if (!pParent)
	{
		// Top-level effect. Should be fully qualified with library and group names.
		// Remove and reinsert in set.
		if (stricmp(pEffect->GetName(), sNewName) != 0)
		{
			pEffect->AddRef();

			m_Effects.erase(pEffect->GetName());
			pEffect->Rename(sNewName);
			m_Effects[pEffect->GetName()] = pEffect;
			pEffect->Release();
		}
	}
	else
	{
		// Child effect. Use only final component, and prefix with parent name.
		cstr sBaseName = strrchr(sNewName, '.');
		sBaseName = sBaseName ? sBaseName+1 : sNewName;

		// Ensure unique name.
		stack_string sNewBase;
		for (int i = pParent->GetChildCount()-1; i >= 0; i--)
		{
			IParticleEffect* pSibling = pParent->GetChild(i);
			if (pSibling != pEffect && stricmp(pSibling->GetBaseName(), sBaseName) == 0)
			{
				// Extract and replace number.
				cstr p = sBaseName+strlen(sBaseName); 
				while (p > sBaseName && (p[-1] >= '0' && p[-1] <= '9'))
					p--;
				int nIndex = atoi(p);
				sNewBase.assign(sBaseName, p);
				sNewBase.append(ToString(nIndex+1));
				sBaseName = sNewBase;

				// Reset loop.
				i = pParent->GetChildCount();
			}
		}

		// Qualify name by parents.
		stack_string strFullName = pEffect->GetParent()->GetName();
		if (!strFullName.empty())
			strFullName += ".";
	 	strFullName += sBaseName;

		pEffect->Rename( strFullName );
	}
}

//////////////////////////////////////////////////////////////////////////
void CPartManager::DeleteEffect( IParticleEffect *pEffect )
{
	assert( pEffect );
	if (!pEffect->GetParent())
		m_Effects.erase(pEffect->GetName());
}

//////////////////////////////////////////////////////////////////////////
IParticleEffect* CPartManager::FindEffect( cstr sEffectName, cstr sSource, bool bLoad )
{
	if (!sEffectName || !*sEffectName)
		return NULL;

	TEffectsList::iterator it = m_Effects.find(sEffectName);
	if (it == m_Effects.end())
	{
		// Try to load the lib.
		cstr sDot = strchr(sEffectName,'.');
		if (sDot)
		{
			stack_string sLibraryName(sEffectName, sDot);
			if (m_bRuntime)
			{
				// Do not load whole library, just requested effect.
				// Inefficient, as entire XML lib is temporarily read, 
				// but runtime loads stall anyway, so we choose saving memory.
				XmlNodeRef nodeLib = ReadLibrary(sLibraryName);
				if (nodeLib)
				{
					cstr sBaseName = sDot+1;
					for (int i = nodeLib->getChildCount()-1; i >= 0; i--)
					{
						XmlNodeRef node = nodeLib->getChild(i);
						if (node->isTag("Particles"))
						{
							if (strcmpi(node->getAttr("Name"), sBaseName) == 0)
							{
								CryLog( "Particle effect loaded at runtime: '%s'%s%s",
									sEffectName, *sSource ? " from " : "", sSource );
								return LoadEffect( sEffectName, node, true );
							}
						}
					}
				}
			}
			else
				LoadLibrary(sLibraryName);

			// Try it again.
			it = m_Effects.find(sEffectName);
		}

		if (it == m_Effects.end())
		{
			// Not found. Add an empty effect (small memory use) to avoid duplicate warnings.
			CParticleEffect* newEffect = new CParticleEffect(sEffectName);
			m_Effects[newEffect->GetName()] = newEffect;
			Warning("Particle effect not found: '%s'%s%s", sEffectName, *sSource ? " from " : "", sSource);
			return NULL;
		}
	}

	CParticleEffect* pEffect = it->second;
	assert(pEffect);
	if (!pEffect->IsNull())
	{
		MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_ParticleEffect, 0, "%s", sEffectName);

		pEffect->LoadFromXML();
		if (bLoad && pEffect->LoadResources())
		{
			if (m_bRuntime)
				CryLog( "Particle effect loaded at runtime: '%s'%s%s",
					sEffectName, *sSource ? " from " : "", sSource );
		}
		return pEffect;
	}

	// Empty effect (either disabled params, or cached not found).
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
#ifdef bEVENT_TIMINGS
void CPartManager::LogEvents()
{
	WriteLock lock(m_EventLock);
	if (GetCVars()->e_ParticlesDebug & AlphaBits('ed'))
	{
		if (!m_aEvents.size())
		{
			// Start timing this frame.
			AddEventTiming("*Frame", 0);
		}
		else
		{
			// Finish and log frame.
			m_aEvents[0].timeEnd = max(GetTimer()->GetAsyncCurTime(), m_aEvents[0].timeStart + GetTimer()->GetRealFrameTime());
			float timeFrameStart = m_aEvents[0].timeStart,
						timeFrameTime = m_aEvents[0].timeEnd - m_aEvents[0].timeStart;

			if (timeFrameTime > 0.f)
			{
				static const int nWidth = 50;
				int nSumStart = m_aEvents.size();
				float timeTotal = 0.f;

				// Add to summary entries at end.
				for (int e = 0; e < nSumStart; e++)
				{
					SEventTiming* pEvent = &m_aEvents[e];
					if (pEvent->nContainerId)
					{
						timeTotal += pEvent->timeEnd - pEvent->timeStart;
						int s;
						for (s = nSumStart; s < m_aEvents.size(); s++)
						{
							SEventTiming* pSum = &m_aEvents[s];
							if (pSum->sEvent == pEvent->sEvent && pSum->nThread == pEvent->nThread)
								break;
						}
						if (s == m_aEvents.size())
						{
							// Add to summary.
							SEventTiming* pSum = m_aEvents.push_back();
							*pSum = m_aEvents[e];
							pSum->nContainerId = 0;
						}
					}
				}

				// Check against time threshold.
				if ((GetCVars()->e_ParticlesDebug & AlphaBit('e')) || timeTotal > m_timeThreshold)
				{
					// Increase threshold for next time.
					m_timeThreshold = timeTotal * 1.1f;

					for_array (c, m_aEvents)
					{
						SEventTiming* pEventC = &m_aEvents[c];
						if (!pEventC->sEvent)
							continue;

						// Iterate unique threads.
						for (int t = c; t < m_aEvents.size(); t++)
						{
							SEventTiming* pEventT = &m_aEvents[t];
							if (!pEventT->sEvent)
								continue;
							if (pEventT->nContainerId != pEventC->nContainerId)
								continue;

							cstr sThread = CryThreadGetName(pEventT->nThread);
							if (pEventT == pEventC)			// Main thread.
								GetLog()->LogToFile( "%*s %s(%X) @%s", 
									nWidth, "", pEventC->pEffect ? pEventC->pEffect->GetName() : "", pEventC->nContainerId, sThread );
							else
								GetLog()->LogToFile( "%*s   @%s", nWidth, "", sThread );

							// Log event times.
							for (int e = t; e < m_aEvents.size(); e++)
							{
								SEventTiming* pEvent = &m_aEvents[e];
								if (!pEvent->sEvent)
									continue;
								if (pEvent->nContainerId != pEventT->nContainerId || pEvent->nThread != pEventT->nThread)
									continue;

								// Construct thread timeline.
								char sGraph[nWidth+1];
								memset(sGraph, ' ', nWidth);
								sGraph[nWidth] = 0;

								int start_iter = pEventC->nContainerId ? e : 0;
								int end_iter = pEventC->nContainerId ? e+1 : nSumStart;
								float timeStart = m_aEvents[e].timeStart, 
											timeEnd = m_aEvents[e].timeEnd,
											timeTotal = 0.f;
								for (int i = start_iter; i < end_iter; i++)
								{
									SEventTiming* pEventI = &m_aEvents[i];
									if (pEventI->sEvent != pEvent->sEvent || pEventI->nThread != pEvent->nThread)
										continue;
 
									timeStart = min(timeStart, pEventI->timeStart);
									timeEnd = max(timeEnd, pEventI->timeEnd);
									timeTotal += pEventI->timeEnd - pEventI->timeStart;

									int nEventStart = int_round((pEventI->timeStart - timeFrameStart) * nWidth / timeFrameTime);
									int nEventEnd = int_round((pEventI->timeEnd - timeFrameStart) * nWidth / timeFrameTime);

									nEventStart = min(nEventStart, nWidth-1);
									nEventEnd = min(max(nEventEnd, nEventStart+1), nWidth);

									cstr sEvent = strrchr(pEventI->sEvent, ':');
									char cEvent = sEvent ? sEvent[1] : *pEventI->sEvent;
									memset(sGraph+nEventStart, cEvent, nEventEnd-nEventStart);
								}

								GetLog()->LogToFile( "%s     %.3f-%.3f [%.3f] %s",
									sGraph,
									(timeStart - timeFrameStart)*1000.f, (timeEnd - timeFrameStart)*1000.f, timeTotal*1000.f,
									pEvent->sEvent );

								pEvent->sEvent = 0;
							}
						}
					}
				}
			}

			// Clear log.
			GetCVars()->e_ParticlesDebug &= ~AlphaBit('e');
			m_aEvents.resize(0);
			if (GetCVars()->e_ParticlesDebug & AlphaBit('d'))
				// Keep timing every frame.
				AddEventTiming("*Frame", 0);
		}
	}
	else
	{
		m_aEvents.resize(0);
		m_timeThreshold = 0.f;
	}
	m_iEventSwitch *= -1;
}
#endif

void CPartManager::OnFrameStart()
{
#ifdef bEVENT_TIMINGS
	LogEvents();
#endif
}

void CPartManager::Update()
{
	FUNCTION_PROFILER_CONTAINER(0);

	// If any render tasks from previous frame are still outstanding (due to multi-threaded renderer),
	// compute and store vertices, before progressing to next frame.
  gEnv->pRenderer->EF_ComputeQueuedParticles();




















	if (GetSystem()->IsPaused() || (GetCVars()->e_ParticlesDebug & AlphaBit('z')))
		return;

	if (!m_bRuntime && !m_pSystem->IsEditorMode())
		OnStartRuntime();

	m_bRuntime = !m_pSystem->IsEditorMode();
	m_tLastUpdate = GetTimer()->GetFrameStartTime();

	if (Get3DEngine()->GetIVisAreaManager())
		Get3DEngine()->GetIVisAreaManager()->AddListener(this);

	// Initial pass to update particle-generated forces.
	for_all_ptrs (CParticleEmitter, e, m_Emitters)
		e->UpdateForce();

	// Main pass to update state.
	for_all_ptrs (CParticleEmitter, e, m_Emitters)
	{
		// Evolve emitters (and particles of dynamic-bounds emitters).
		EEmitterState eState = e->UpdateEmitter();

		// Cull dead emitters.
		switch (eState)
		{
			case eEmitter_Active:
			case eEmitter_Particles:
				e->Register(true);



				break;
			case eEmitter_Dormant:
				e->Register(false);
				break;
			case eEmitter_Dead:
				e = EraseEmitter(e);
				break;
			default:
				assert("!Unhandled EEmitterState");
		}
	}

	m_bbAreasChanged.Reset();
}

//////////////////////////////////////////////////////////////////////////
IParticleEmitter* CPartManager::CreateEmitter( bool bIndependent, Matrix34 const& mLoc, const IParticleEffect* pEffect, const ParticleParams* pParams )
{
	assert(pEffect || pParams);
	FUNCTION_PROFILER_SYS(PARTICLE);

	CParticleEmitter *pEmitter = m_Emitters.push_back(bIndependent);
	if (pEmitter)
	{
		pEmitter->AddRef();
		pEmitter->SetMatrix(mLoc);
		pEmitter->SetEffect(static_cast<const CParticleEffect*>(pEffect), pParams);
		for_container (TListenersList, itListener, m_ListenersList)
		{
			(*itListener)->OnCreateEmitter(pEmitter, bIndependent, mLoc, pEffect, pParams);
		}
	}
	return pEmitter;
}

//////////////////////////////////////////////////////////////////////////
bool CPartManager::KillEmitter( CParticleEmitter *pEmitter )
{
	if (pEmitter)
	{
		if (pEmitter->Kill())
		{
			for_container (TListenersList, itListener, m_ListenersList)
			{
				(*itListener)->OnDeleteEmitter(pEmitter);
			}
		}

		if (pEmitter->GetRefCount() == 1)
			// Caller should delete emitter immediately.
			return true;
	}
	return false;
}

void CPartManager::DeleteEmitter( IParticleEmitter *pEmitter )
{
	CParticleEmitter* pE = static_cast<CParticleEmitter*>(pEmitter);
	EraseEmitter(pE);
}

void CPartManager::UpdateEmitters( IParticleEffect *pEffect )
{
	// Update all emitters with this effect tree.
	for_all_ptrs (CParticleEmitter, e, m_Emitters)
	{
		for (IParticleEffect* pTest = pEffect; pTest; pTest = pTest->GetParent())
			if (e->GetEffect() == pTest)
			{
				e->SetEffect(pTest);
				break;
			}
	}
}

int CPartManager::OnPhysAreaChange(const EventPhys *pEvent)
{
	EventPhysAreaChange const& epac = (EventPhysAreaChange const&)*pEvent;
	m_bbAreasChanged.Add( AABB(epac.boxAffected[0], epac.boxAffected[1]) );
	return 0;
}

void CPartManager::OnVisAreaDeleted( IVisArea* pVisArea )
{
	for_all_ptrs (CParticleEmitter, e, m_Emitters)
		e->OnVisAreaDeleted(pVisArea);
}

CParticleEmitter* CPartManager::EraseEmitter( CParticleEmitter *pEmitter )
{
	CRY_ASSERT(pEmitter);
	if (pEmitter && KillEmitter(pEmitter))
	{
		return m_Emitters.erase(pEmitter);
	}

	return pEmitter;
}

//////////////////////////////////////////////////////////////////////////
class CLibPathIterator
{
public:

	CLibPathIterator(cstr sLevelPath = "")
		: sPath(*sLevelPath ? sLevelPath : LEVEL_PATH), bDone(false)
		{}
	operator bool() const
		{ return !bDone; }
	const string& operator *() const
		{ return sPath; }

	void operator++()
	{
		if (sPath.empty())
			bDone = true;
		else
			sPath = PathUtil::GetParentDirectory(sPath);
	}

protected:
	string sPath;
	bool bDone;
};

//////////////////////////////////////////////////////////////////////////
bool CPartManager::LoadLibrary( cstr sParticlesLibrary, cstr sParticlesLibraryFile, bool bLoadResources )
{
	if (strchr(sParticlesLibrary, '*'))
	{
		// Wildcard load.
		if (!sParticlesLibraryFile)
		{
			// Load libs from level-local and global paths.
			int nCount = 0;
			for (CLibPathIterator path(Get3DEngine()->GetLevelFilePath("")); path; ++path)
				nCount += LoadLibrary(sParticlesLibrary, PathUtil::Make(*path, EFFECTS_SUBPATH), bLoadResources);
			return nCount > 0;
		}
		else
		{
			// Load from specified path.
			string sLibPath = PathUtil::Make( sParticlesLibraryFile, sParticlesLibrary, "xml" );
			ICryPak *pack = gEnv->pCryPak;
			_finddata_t fd;
			intptr_t handle = pack->FindFirst( sLibPath, &fd );
			int nCount = 0;
			if (handle >= 0)
			{
				do {
					if (LoadLibrary( PathUtil::GetFileName(fd.name), PathUtil::Make( sParticlesLibraryFile, fd.name), bLoadResources ))
						nCount++;
				} while (pack->FindNext( handle, &fd ) >= 0);
				pack->FindClose(handle);
			}
			return nCount > 0;
		}
	}
	else if (sParticlesLibrary[0] == '@')
	{
		int nCount = 0;
		string sFilename = PathUtil::Make( EFFECTS_SUBPATH, sParticlesLibrary+1, "txt" );
		CCryFile file;
		if (file.Open( sFilename, "r" ))
		{
			int nLen = file.GetLength();
			string sAllText;
			sAllText.assign(nLen, '\n');
			file.ReadRaw( sAllText.begin(), nLen );

			nLen = 0;
			string sLine;
			while (!(sLine = sAllText.Tokenize("\r\n", nLen)).empty())
			{
				if (LoadLibrary( sLine, NULL, bLoadResources ))
					nCount++;
			}
		}
		return nCount > 0;
	}

	if (m_LoadedLibs[sParticlesLibrary])
	{
		// Already loaded.
		if (bLoadResources)
		{
			// Iterate all fx in lib and load their resources.
			stack_string sPrefix = sParticlesLibrary;
			sPrefix += ".";
			for_container (TEffectsList, it, m_Effects)
			{
				CParticleEffect* pEffect = it->second;
				if (pEffect && strnicmp(pEffect->GetName(), sPrefix, sPrefix.size()) == 0)
				{
					pEffect->LoadFromXML();
					pEffect->LoadResources(true);
				}
			}
		}
		return true;
	}

	XmlNodeRef libNode;
	if (sParticlesLibraryFile)
		// Load from specified location.
		libNode = GetISystem()->LoadXmlFile( sParticlesLibraryFile );
	else
		libNode = ReadLibrary( sParticlesLibrary );

	if (!libNode)
		return false;

	LoadLibrary(sParticlesLibrary, libNode, bLoadResources);

	return true;
}

XmlNodeRef CPartManager::ReadLibrary( cstr sParticlesLibrary )
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_ParticleLibrary, 0, "Particle lib (%s)", sParticlesLibrary);

	// Look for library in level-specific, then general locations.
	string sLibSubPath = PathUtil::Make( EFFECTS_SUBPATH, sParticlesLibrary, "xml" );
	for (CLibPathIterator path(Get3DEngine()->GetLevelFilePath("")); path; ++path)
	{
		XmlNodeRef libNode = GetISystem()->LoadXmlFile( PathUtil::Make(*path, sLibSubPath) );
		if (libNode)
			return libNode;
	}
	return NULL;
}

bool CPartManager::LoadLibrary(cstr sParticlesLibrary, XmlNodeRef& libNode, bool bLoadResources )
{
	if (m_bRuntime)
		Warning( "Particle library loaded at runtime: %s", sParticlesLibrary );

	gEnv->pLog->LogToFile( "Loading Particle Library: %s", sParticlesLibrary );
//	gEnv->pLog->UpdateLoadingScreen(0);

	m_LoadedLibs[sParticlesLibrary] = libNode;

	// Load all nodes from the particle libaray.
	for (int i = 0, count = libNode->getChildCount(); i < count; i++)
	{
		XmlNodeRef effectNode = libNode->getChild(i);
		if (effectNode->isTag("Particles"))
		{
			string sEffectName = sParticlesLibrary;
			sEffectName += ".";
			sEffectName += effectNode->getAttr("Name");

			// Load the full effect.
			LoadEffect( sEffectName, effectNode, bLoadResources, bLoadResources );
		}
	}
	return true;
}

IParticleEffect* CPartManager::LoadEffect(cstr sEffectName, XmlNodeRef& effectNode, bool bLoadXML, bool bLoadResources)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_ParticleEffect, 0, "%s", sEffectName);

	CParticleEffect* pEffect;
	TEffectsList::iterator it = m_Effects.find(sEffectName);
	if (it != m_Effects.end())
		pEffect = it->second;
	else
	{
		pEffect = new CParticleEffect(sEffectName);
		m_Effects[pEffect->GetName()] = pEffect;
	}

	if (bLoadXML)
	{
		// Load effect params and all assets.
		pEffect->Serialize( effectNode, true, true );
		if (bLoadResources)
			pEffect->LoadResources();
	}
	else
	{
		// Cache XML node during level load.
		pEffect->SetXMLSource( effectNode );
	}

	return pEffect;
}

void CPartManager::OnStartRuntime()
{
	ClearCachedLibraries();
}

void CPartManager::ClearCachedLibraries()
{
	// Print stats for loaded vs requested particle effects.
	for (TEffectsList::iterator it = m_Effects.begin(); it != m_Effects.end(); )
	{
		// Purge all cached XML, and unused effects.
		it->second->SetXMLSource(0);
		if (it->second->IsNull())
			m_Effects.erase(it++);
		else
			++it;
	}
	m_LoadedLibs.clear();

	PrintParticleMemory();
}

void CPartManager::RenderDebugInfo()
{
  if ((GetCVars()->e_ParticlesDebug & AlphaBit('b')) || GetSystem()->IsEditorMode())
  {
		// Debug particle BBs.
		for_all_ptrs (CParticleEmitter, e, m_Emitters)
			e->RenderDebugInfo();
	}
}


void CPartManager::RenderSpuUsage() const
{		




















}

#ifdef bEVENT_TIMINGS

int CPartManager::AddEventTiming( cstr sEvent, const CParticleContainer* pCont )
{
	if (!m_aEvents.size() && *sEvent != '*')
		return -1;

	SEventTiming* pEvent = m_aEvents.push_back();
	pEvent->nContainerId = (uint32)pCont;
	pEvent->pEffect = pCont ? pCont->GetEffect() : 0;
	pEvent->nThread = CryGetCurrentThreadId();
	pEvent->sEvent = sEvent;
	pEvent->timeStart = GetTimer()->GetAsyncCurTime();

	return m_aEvents.size()-1;
}

#endif

//////////////////////////////////////////////////////////////////////////
void CPartManager::Serialize(TSerialize ser)
{
	ser.BeginGroup("ParticleEmitters");

	if (ser.IsWriting())
	{
		ListEmitters("before save");

		int nCount = 0;
		for_all_ptrs (CParticleEmitter, e, m_Emitters)
		{
			if (e->NeedSerialize())
				nCount++;
		}
		ser.Value("Emitters", nCount);

		for_all_ptrs (CParticleEmitter, e, m_Emitters)
		{
			if (e->NeedSerialize())
				e->Serialize(ser);
		}
	}
	else
	{
		ListEmitters("before load");
		m_bRuntime = false;

		// Clean up existing emitters.
		for_all_ptrs (CParticleEmitter, e, m_Emitters)
		{
			if (e->IsIndependent())
				// Would be serialized.
				e = EraseEmitter(e);
			else if (e->GetAge() < 0.f ||
			e->GetAge() > e->GetStopAge())
				// No longer exists at new time.
				e = EraseEmitter(e);
			else
				// Restart at age 0, in case expired.
				e->Activate(eAct_Reactivate);
		}

		int nCount = 0;
		ser.Value("Emitters", nCount);
		while (nCount-- > 0)
		{
			CParticleEmitter *pEmitter = m_Emitters.push_back(true);
			pEmitter->Serialize(ser);
			if (pEmitter->GetEffect())
				pEmitter->AddRef();
			else
				EraseEmitter(pEmitter);
		}
	}
	ser.EndGroup();
}

void CPartManager::PostSerialize( bool bReading )
{
	if (bReading)
		ListEmitters("after load");
}

void CPartManager::ListEmitters( cstr sDesc )
{
	if (GetCVars()->e_ParticlesDebug & AlphaBit('l'))
	{
		// Count emitters.
		int anEmitters[eEmitter_Active+1] = {0};
		for_all_ptrs (const CParticleEmitter, e, m_Emitters)
			anEmitters[min(e->GetState(), eEmitter_Active)]++;

		// Log summary, and state of each emitter.
		CryLog( "Emitters %s: time %.3f (prev %.3f), %d active, %d inactive, %d dormant, %d dead", 
			sDesc, GetTimer()->GetFrameStartTime().GetSeconds(), m_tLastUpdate.GetSeconds(), 
			anEmitters[eEmitter_Active], anEmitters[eEmitter_Particles], anEmitters[eEmitter_Dormant], anEmitters[eEmitter_Dead] );
		for_all_ptrs (const CParticleEmitter, e, m_Emitters)
			CryLog( " Effect %s", e->GetDebugString('s').c_str() );
	}
}

#undef USE_SPU
