#include "StdAfx.h"
#include "CREParticle.h"
#include "FrameProfiler.h"
#include "HeapContainer.h"

#define PARTICLE_THREAD_NAME "ParticleCompute"





struct CryWaitEvent
{
	CryWaitEvent()
		: m_bWaiting(false)
	{}

	void Wait()
	{
		m_bWaiting = true;
		m_Event.Wait();
		m_bWaiting = false;
	}

	void Wake(bool bAlways = false)
	{
		if (bAlways || m_bWaiting)
		{
			FUNCTION_PROFILER_SYS(PARTICLE);
			m_Event.Set();
		}
	}

protected:
	CryEvent			m_Event;
	volatile bool m_bWaiting;
};


typedef HeapPriorityQueue<CREParticle,std::less<CREParticle>,stl::PSyncMultiThread> 		
	CParticleQueue;

//////////////////////////////////////////////////////////////////////////
//
// Data needed for each render list in multi-threaded rendering.
//

struct CParticleSharedData
{
	CParticleSharedData() 
		: m_fTotalPixels(0.f), m_fMaxPixels(0.f), m_Lock(0)
	{
		m_afPixels.reserve(2048);
	}

	// Tracks pixel coverage distribution for each frame, for fill-rate limiting.
	float GetMaxPixels() const
	{
		return m_fMaxPixels;
	}

	void AddPixelCount( float fPixels )
	{
		if (fPixels > 0.f)
		{
			WriteLock lock(m_Lock);
			m_afPixels.push_back( fPixels );
			m_fTotalPixels += fPixels;
		}
	}

	void ComputeMaxPixels()
	{
		FUNCTION_PROFILER_SYS(PARTICLE);

		// Find per-container maximum which will not exceed total.		
		static ICVar* pVar = gEnv->pConsole->GetCVar("e_ParticlesMaxScreenFill");
		if (!pVar)
			return;
		float fMaxTotalPixels = pVar->GetFVal() * gRenDev->GetWidth() * gRenDev->GetHeight();
		float fNewMax = fMaxTotalPixels;

		WriteLock lock(m_Lock);
		if (m_fTotalPixels > fMaxTotalPixels)
		{
			// Compute max pixels we can have per emitter before total exceeded,
			// from previous frame's data.
			std::sort( m_afPixels.begin(), m_afPixels.end() );
			float fUnclampedTotal = 0.f;
			for_array (i, m_afPixels)
			{
				float fTotal = fUnclampedTotal + (m_afPixels.size() - i) * m_afPixels[i];
				if (fTotal > fMaxTotalPixels)
				{
					fNewMax = (fMaxTotalPixels - fUnclampedTotal) / (m_afPixels.size() - i);
					break;
				}
				fUnclampedTotal += m_afPixels[i];
			}
		}

		// Update current value gradually.
		float fMaxChange = fNewMax * 0.5f;
		m_fMaxPixels = clamp_tpl(fNewMax, m_fMaxPixels - fMaxChange, m_fMaxPixels + fMaxChange);

		m_afPixels.resize(0);
		m_fTotalPixels = 0.f;
	}

protected:
	DynArray<float>					m_afPixels;
	float										m_fTotalPixels;
	float										m_fMaxPixels;
	volatile int						m_Lock;
};

static CParticleSharedData s_SharedData;

//////////////////////////////////////////////////////////////////////////
//
// Data needed for each render list in multi-threaded rendering.
//

struct CRenderListData
{
	CParticleQueue														m_Queue;
	stl::HeapAllocator<stl::PSyncMultiThread>	m_VertHeap;

	void Clear()
	{
		m_Queue.clear();
		m_VertHeap.Reset();
	}

	void FlushQueue()
	{
		FUNCTION_PROFILER_SYS(PARTICLE);

		while (CREParticle* pRE = m_Queue.pop_front())
		{
			pRE->StoreVertices(false);
		}
	}
};

static CRenderListData		g_RenderListData[2];			// One for each render list.

//////////////////////////////////////////////////////////////////////////
//
// Separate particle computation thread
//

struct CParticleThread: CrySimpleThread<>, CryWaitEvent
{
	static int nThread()
	{
#if !defined(PS3) // disabled particle thread on PS3 since spus are used there
		if (gEnv->pi.numCoresAvailableToProcess > 1)
		{
			if (gEnv->pConsole)
			{
				ICVar* pVar = gEnv->pConsole->GetCVar("e_ParticlesThread");
				if (pVar)
					return pVar->GetIVal();
			}
		}
#endif
		return 0;
	}

	CParticleThread(int nThread = 0)
	{
		Start(-1, PARTICLE_THREAD_NAME, THREAD_PRIORITY_NORMAL, 64*1024);




	}

	virtual void Terminate()
	{
		assert(!IsRunning());
		delete this;
	}

	void Stop()
	{
		CrySimpleThread<>::Stop();
		Wake(true);
	}

protected:

	virtual void Run()
	{
		CryThreadSetName(-1, PARTICLE_THREAD_NAME);
		gEnv->pSystem->GetIThreadTaskManager()->MarkThisThreadForDebugging("Particles",true);

		while (IsStarted())
		{
			// Wait until there is work to do.
			Wait();

      if(!gRenDev->m_pRT)
        break;

			CRenderListData& Data = g_RenderListData[ gRenDev->m_pRT->CurThreadFill() ];
			CREParticle* pRE;
			while (IsStarted())
			{
				pRE = Data.m_Queue.pop_largest();
				if (!pRE)
					break;
				pRE->StoreVertices(false, true);
			}
		}
		gEnv->pSystem->GetIThreadTaskManager()->MarkThisThreadForDebugging("Particles",false);
	}
};

static CParticleThread* g_ParticleThread = 0;

static bool IsMultiThreading()
{ 
	// Check all conditions which mean that particles are computed multithreaded
	if (g_ParticleThread || !gRenDev->m_pRT->IsRenderThread())
		return true;



	return false;

}

//////////////////////////////////////////////////////////////////////////
//
// IAllocRender implementations.
//

struct CAllocRenderStore: IAllocRender
{
	CREParticle*	m_pRE;

	CAllocRenderStore(CREParticle* pRE)
		: m_pRE(pRE)
	{
		fMaxPixels = s_SharedData.GetMaxPixels();
	}

	// Set existing SVertices to RE, alloc new ones.
	virtual void Alloc( int nAllocVerts, int nAllocInds = 0, int nAllocVertCounts = 0 )
	{
		assert(nAllocInds == 0);
		assert(aIndices.empty());

		m_pRE->SetVertices( aVertices(), aVertCounts(), fPixels );

		CRenderListData& Data = g_RenderListData[ gRenDev->m_pRT->GetThreadList() ];

		if (nAllocVerts > aVertices.available())
			aVertices.set( ArrayT( Data.m_VertHeap.NewArray<SVertexParticle>(nAllocVerts, 128), nAllocVerts ) );
		if (nAllocVertCounts > aVertCounts.available())
			aVertCounts.set(ArrayT( Data.m_VertHeap.NewArray<uint8>(nAllocVertCounts, 1), nAllocVertCounts ));
	}

	virtual CREParticle* RenderElement() const 
	{ 
		return m_pRE; 
	}
};

struct CAllocRenderDirect: IAllocRender
{
	CAllocRenderDirect()
	{
		fMaxPixels = s_SharedData.GetMaxPixels();
		SShaderTechnique* pTech = gRenDev->m_RP.GetStartTechnique();
		bDirect = true;
		bGeomShader = pTech && (pTech->m_Flags & FHF_USE_GEOMETRY_SHADER);
	}

	// Render existing SVertices, alloc new ones.
	virtual void Alloc( int nAllocVerts, int nAllocInds = 0, int nAllocVertCounts = 0 )
	{
		assert(nAllocVertCounts == 0);
		assert(aVertCounts.empty());

		SRenderPipeline& rp = gRenDev->m_RP;

		// Update pipeline verts based on how many used.
		rp.m_NextPtr.VBPtr_8 += aVertices.size();
		rp.m_RendNumVerts += aVertices.size();
		rp.m_RendNumIndices += aIndices.size();

		int nAllocedVerts = 0, nAllocedInds = 0;
		if (nAllocVerts)
		{
			// Flush and alloc more.
			gRenDev->FX_CheckOverflow( nAllocVerts, nAllocInds, rp.m_pRE, &nAllocedVerts, &nAllocedInds );
			if (nAllocedInds < nAllocInds)
				// Limit vert count when index allocation also truncated.
				nAllocedVerts = min(nAllocedVerts, int((int64)nAllocedInds * nAllocVerts/nAllocInds));
		}
		aVertices.set(ArrayT( rp.m_NextPtr.VBPtr_8, nAllocedVerts ));
		aIndices.set(ArrayT( rp.m_RendIndices + rp.m_RendNumIndices, nAllocedInds ));
		nBaseVertexIndex = rp.m_RendNumVerts;
	}
};


//////////////////////////////////////////////////////////////////////////
//
// CREParticle implementation.
//

CREParticle::CREParticle( IParticleVertexCreator* pVC, const SParticleRenderContext& context )
: m_pVertexCreator(pVC)
, m_ParticleComputed(false)
, m_Context(context)
, m_fPixels(0.f)
, m_nRenderOrder(pVC ? pVC->GetRenderOrder() : 0)
{
	mfSetType(eDATA_Particle);
}

CREParticle* 
CREParticle::Create( IParticleVertexCreator* pVC, const SParticleRenderContext& context )
{
	FUNCTION_PROFILER_SYS(PARTICLE);

	assert(pVC);

	CRenderListData& Data = g_RenderListData[ gRenDev->m_pRT->GetThreadList() ];
	CREParticle* pRE = Data.m_Queue.push_back(pVC, context);









	// Queue for threaded processing.
	if (g_ParticleThread)
	{
		g_ParticleThread->Wake();
	}

	return pRE;
}

CREParticle::~CREParticle()
{
}



void
CREParticle::ClearSPUQueue() 
{
	CRenderListData& Data = g_RenderListData[ gRenDev->m_pRT->CurThreadFill() ];	
	Data.m_Queue.clear();
}

float 
CREParticle::mfDistanceToCameraSquared( Matrix34& matInst )
{
	// This should only be called when we still have a container reference.
	IParticleVertexCreator* pVC = m_pVertexCreator;
	assert(pVC);
	if (!pVC)
		return 0.f;
	return pVC->GetDistSquared( m_Context.m_vCamPos );
}

void 
CREParticle::mfPrepare()
{
	gRenDev->FX_StartMerging();

	bool bComputedLocally = false;








	{
		CRenderListData& Data = g_RenderListData[gRenDev->m_pRT->CurThreadFill()];
		if(Lock(true))
		{
			if(!m_ParticleComputed)
			{
				_smart_ptr<IParticleVertexCreator> pVC = m_pVertexCreator;
				if (pVC)
				{
					// Compute verts directly into render buffer.
					CAllocRenderDirect alloc;
					pVC->ComputeVertices( m_Context, alloc );
					s_SharedData.AddPixelCount( alloc.fPixels );
					m_ParticleComputed = true;
					bComputedLocally = true;
				}
			}
			Unlock();
		}
	}

	if(!bComputedLocally)
	{
		// Transfer verts that were stored during the particle thread's update
		TransferVertices();
	}

#if defined (DIRECT3D10) && !defined(PS3)
	if (gRenDev->m_RP.m_RendNumVerts)
	{
		gRenDev->m_RP.m_pRE = this; 
	}
#endif
}

void 
CREParticle::StoreVertices( bool bWait, bool bIsParticleThread )
{
	FUNCTION_PROFILER_SYS(PARTICLE);

	if(Lock(bWait))
	{
		if(!m_ParticleComputed)
		{
			_smart_ptr<IParticleVertexCreator> pVC = m_pVertexCreator;
			if(pVC)
			{
				// Compute and save vertices into vert buffer.
				assert(m_aVerts.empty());
				CAllocRenderStore alloc(this);
				pVC->ComputeVertices(m_Context, alloc, bIsParticleThread);
				m_ParticleComputed = true;
			}
		}
		Unlock();
	}
}

void 
CREParticle::TransferVertices() const
{
	FUNCTION_PROFILER_SYS(PARTICLE);

	CAllocRenderDirect alloc;

	// Copy pre-created vertices.
	// Track progress in Array reference objects.
	Array<SVertexParticle> aVerts = m_aVerts;
	Array<uint8> aVertCounts = m_aVertCounts;

	while (!aVerts.empty())
	{
		assert(!aVertCounts.empty());
		if (aVertCounts[0] > 4)
		{
			// Variable vertex count.
			assert(!alloc.bGeomShader);
			alloc.Alloc(aVerts.size(), aVerts.size() * 3);
			alloc.SetPoliesIndices(aVerts, aVertCounts);
		}
		else if (aVertCounts[0] == 1)
		{
			// Point sprites, 1 vertex per particle.
			assert(aVertCounts.size() == 1);
			if (alloc.bGeomShader)
			{
				// Just copy vertices for point sprites.
				alloc.Alloc(aVerts.size());
				alloc.CopyVertices(aVerts);
			}
			else
			{
				// Expand to 4 vertices each.
				alloc.Alloc(aVerts.size() * 4, aVerts.size() * 6);
				int nVerts = min(aVerts.size(), alloc.aVertices.available() >> 2);
				for (int n = 0; n < nVerts; n++)
				{
					alloc.aVertices.push_back(aVerts[n]);
					alloc.ExpandQuadVertices();
				}
				aVerts.erase_front(nVerts);
				alloc.SetQuadsIndices();
			}
		}
		else
		{
			// 4 vertices per sprite.
 			assert(aVertCounts[0] == 4);
			assert(aVertCounts.size() == 1);
			assert(!alloc.bGeomShader);
			alloc.Alloc(aVerts.size(), aVerts.size() * 3/2);
			alloc.CopyVertices(aVerts);
			alloc.SetQuadsIndices();
		}
	}

	s_SharedData.AddPixelCount( m_fPixels );
	alloc.Alloc(0);
}

//////////////////////////////////////////////////////////////////////////
//
// CRenderer particle functions implementation.
//

void 
CRenderer::EF_ComputeQueuedParticles()
{
	if (IsMultiThreading())
	{
#ifndef STRIP_RENDER_THREAD
		g_RenderListData[gRenDev->m_pRT->m_nCurThreadProcess].FlushQueue();
#endif
		g_RenderListData[gRenDev->m_pRT->CurThreadFill()].FlushQueue();
	}
}


void 
CRenderer::EF_RemoveParticlesFromScene()
{
	FUNCTION_PROFILER_SYS(PARTICLE);

	s_SharedData.ComputeMaxPixels();

	CRenderListData& Data = g_RenderListData[ gRenDev->m_pRT->GetThreadList() ];
	Data.Clear();

	// Manage threading.
	if (int nThread = CParticleThread::nThread())
	{
		if (!g_ParticleThread)
			g_ParticleThread = new CParticleThread(nThread);
	}
	else if (g_ParticleThread)
	{
		g_ParticleThread->Stop();
		g_ParticleThread = 0;
	}
}


void 
CRenderer::SafeReleaseParticleREs()
{
	if (g_ParticleThread)
	{
		g_ParticleThread->Stop();
		g_ParticleThread = 0;
	}
	for (int i = 0; i < 2; ++i)
	{
		g_RenderListData[i].m_Queue.clear();
		g_RenderListData[i].m_VertHeap.Clear();
	}
}


void 
CRenderer::GetMemoryUsageParticleREs( ICrySizer * pSizer )
{
	for (int i = 0; i < 2; ++i)
	{
		pSizer->AddObject( g_RenderListData[i].m_VertHeap );
		pSizer->AddObject( g_RenderListData[i].m_Queue );
	}
}

#undef USE_SPU
