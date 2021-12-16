#include "StdAfx.h"
#include "MementoMemoryManager.h"
#include "Network.h"
//#include <IRenderer.h>
#include "ITextModeConsole.h"

#define ENABLE_MMM_DEBUG	ENABLE_DEBUG_KIT

CMementoMemoryManager* CMementoMemoryRegion::m_pMMM = 0;
CMementoMemoryManager::CMementoMemoryManagerAllocator* CMementoMemoryManager::CMementoMemoryManagerAllocator::m_allocator = NULL;
int CMementoMemoryManager::CMementoMemoryManagerAllocator::m_numCMementoMemoryManagers = 0;
CryLockT<CRYLOCK_RECURSIVE> CMementoMemoryManager::CMementoMemoryManagerAllocator::m_mutex;

#if ENABLE_DEBUG_KIT
CMementoMemoryManager::TManagers * CMementoMemoryManager::m_pManagers;
#endif

static void DrawDebugLine( int x, int y, const char * fmt, ... )
{
	char buffer[512];

	va_list args;
	va_start( args, fmt );
	vsprintf_s( buffer, fmt, args );
	va_end( args );

	float white[] = {1,1,1,1};

	gEnv->pRenderer->Draw2dLabel( (float)(x*12 + 12), (float)(y*12 + 12), 1.2f, white, false, "%s", buffer );

	if (ITextModeConsole * pC = gEnv->pSystem->GetITextModeConsole())
	{
		pC->PutText( x, y, buffer );
	}
}

#if ENABLE_MMM_DEBUG
// pool sizes are 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096
static const int MMMDEBUG_FIRST_POOL = 3; // == 8bytes
static const int MMMDEBUG_LAST_POOL = 12; // == 4096bytes
static const int MMMDEBUG_NPOOLS = MMMDEBUG_LAST_POOL - MMMDEBUG_FIRST_POOL + 1;
static const int MMMDEBUG_NUMCALLSTRACK = 64;
struct MMMDebugData
{
	int	m_numAllocs[MMMDEBUG_NPOOLS];
	int m_numFrees[MMMDEBUG_NPOOLS];
	int	m_totalNumAllocs;
	int m_totalNumFrees;

	int m_currentNumAllocs;
};

static MMMDebugData s_MMMDebug;

static void MMMDebugClearFrameData()
{
	memset(s_MMMDebug.m_numAllocs, 0, sizeof(s_MMMDebug.m_numAllocs));
	memset(s_MMMDebug.m_numFrees, 0, sizeof(s_MMMDebug.m_numFrees));
	s_MMMDebug.m_totalNumAllocs = 0;
	s_MMMDebug.m_totalNumFrees = 0;
}

static void MMMDebugInit()
{
	MMMDebugClearFrameData();

	s_MMMDebug.m_currentNumAllocs = 0;
}

static void MMMDebugAddAlloc(void* p, size_t sz)
{
	int pool = std::max( (int)IntegerLog2_RoundUp(sz) - MMMDEBUG_FIRST_POOL, 0 );

	if (pool < MMMDEBUG_NPOOLS)
	{
		s_MMMDebug.m_numAllocs[pool]++;
	}

	s_MMMDebug.m_totalNumAllocs++;
	s_MMMDebug.m_currentNumAllocs++;
}

static void MMMDebugRemoveAlloc(void* p, size_t sz)
{
	int pool = std::max( (int)IntegerLog2_RoundUp(sz) - MMMDEBUG_FIRST_POOL, 0 );

	if (pool < MMMDEBUG_NPOOLS)
	{
		s_MMMDebug.m_numFrees[pool]++;
	}

	s_MMMDebug.m_totalNumFrees++;
	s_MMMDebug.m_currentNumAllocs--;
}

static void MMMDebugDraw()
{
	int y = 3;
	int x = 40;

	for (int i = 0; i < MMMDEBUG_NPOOLS; i++)
	{
		if (s_MMMDebug.m_numAllocs[i] || s_MMMDebug.m_numFrees[i])
		{
			DrawDebugLine(x, y++, "Pool %d (%d byte blocks): Frame Allocs %d Frame Frees %d",
				i, 1<<(i+MMMDEBUG_FIRST_POOL), s_MMMDebug.m_numAllocs[i], s_MMMDebug.m_numFrees[i]);
		}
	}

	y++;

	DrawDebugLine(x, y++, "Total: Allocs %d Frame Frees %d", s_MMMDebug.m_totalNumAllocs, s_MMMDebug.m_totalNumFrees);
	DrawDebugLine(x, y++, "Current Allocations %d", s_MMMDebug.m_currentNumAllocs);

	MMMDebugClearFrameData();
}
#else
#define MMMDebugInit()
#define MMMDebugRemoveAlloc(p, sz)
#define MMMDebugAddAlloc(p, sz)
#define MMMDebugDraw()
#endif

#if ENABLE_DEBUG_KIT
static float GetWastePercent(size_t used, size_t allocated)
{
	return allocated ? 100.0f*(1.0f - float(used)/float(allocated)) : 0.0f;
}
#endif

CMementoMemoryManager::CMementoMemoryManagerAllocator::CMementoMemoryManagerAllocator()
{
	m_pools[0] = new CPool<8>();
	m_pools[1] = new CPool<16>();
	m_pools[2] = new CPool<32>();
	m_pools[3] = new CPool<64>();
	m_pools[4] = new CPool<128>();
	m_pools[5] = new CPool<256>();
	m_pools[6] = new CPool<512>();
	m_pools[7] = new CPool<1024>();
	m_pools[8] = new CPool<2048>();
	m_pools[9] = new CPool<4096>();
	STATIC_CHECK(NPOOLS == 10, PoolsIncorrectlyInitialized);

	MMMDebugInit();
}

CMementoMemoryManager::CMementoMemoryManagerAllocator::~CMementoMemoryManagerAllocator()
{
	NET_ASSERT(m_handles.size() == m_freeHandles.size());

	for (int i = 0; i < NPOOLS; i++)
	{
		delete m_pools[i];
	}
}

void CMementoMemoryManager::CMementoMemoryManagerAllocator::AddCMementoMemoryManager()
{
	m_mutex.Lock();

	if (m_numCMementoMemoryManagers == 0)
	{
		m_allocator = new CMementoMemoryManagerAllocator();
	}

	m_numCMementoMemoryManagers++;

	m_mutex.Unlock();
}

void CMementoMemoryManager::CMementoMemoryManagerAllocator::RemoveCMementoMemoryManager()
{
	m_mutex.Lock();

	m_numCMementoMemoryManagers--;

	if (m_numCMementoMemoryManagers == 0)
	{
		delete m_allocator;
		m_allocator = NULL;
	}

	m_mutex.Unlock();
}

CMementoMemoryManager::Hdl CMementoMemoryManager::CMementoMemoryManagerAllocator::AllocHdl(size_t sz)
{
	ASSERT_GLOBAL_LOCK;
	Hdl hdl;

	m_mutex.Lock();

	if (m_freeHandles.empty())
	{
		hdl = m_handles.size();
		m_handles.push_back(SHandleData());
	}
	else
	{
		hdl = m_freeHandles.back();
		m_freeHandles.pop_back();
	}

	InitHandleData(m_handles[hdl], sz);

	hdl = ProtectHdl(hdl);

	m_mutex.Unlock();

	return hdl;
}

void CMementoMemoryManager::CMementoMemoryManagerAllocator::ResizeHdl(Hdl hdl, size_t sz)
{
	ASSERT_GLOBAL_LOCK;

	m_mutex.Lock();

	hdl = UnprotectHdl(hdl);

	NET_ASSERT(hdl != InvalidHdl);

	SHandleData& hd = m_handles[hdl];

	if (sz > hd.size) // growing
	{
		if (sz > hd.capacity) // growing and changing pools
		{
			SHandleData hdp;
			MMMDebugRemoveAlloc(hd.p, hd.size);
			m_allocator->InitHandleData( hdp, sz );
			memcpy( hdp.p, hd.p, hd.size );
			hd.pPool->Free(hd.p);
			hd = hdp;
		}
		else
		{
			hd.size = sz;
		}
	}
	else if (sz < hd.size) // shrinking
	{
		if (sz < hd.capacity/2) // shrinking and changing pools
		{
			SHandleData hdp;
			MMMDebugRemoveAlloc(hd.p, hd.size);
			m_allocator->InitHandleData( hdp, sz );
			memcpy( hdp.p, hd.p, sz );
			hd.pPool->Free(hd.p);
			hd = hdp;
		}
		else
		{
			hd.size = sz;
		}
	}

	m_mutex.Unlock();
}

void CMementoMemoryManager::CMementoMemoryManagerAllocator::FreeHdl(Hdl hdl)
{
	ASSERT_GLOBAL_LOCK;

	m_mutex.Lock();

	hdl = UnprotectHdl(hdl);

	if (hdl != InvalidHdl)
	{
		SHandleData& hd = m_handles[hdl];
		MMMDebugRemoveAlloc(hd.p, hd.size);
		hd.pPool->Free(hd.p);
		m_freeHandles.push_back(hdl);
	}

	m_mutex.Unlock();
}

void* CMementoMemoryManager::CMementoMemoryManagerAllocator::AllocPtr(size_t sz)
{
	ASSERT_GLOBAL_LOCK;
	SHandleData hd;

	m_mutex.Lock();

	InitHandleData(hd, sz);

	m_mutex.Unlock();

	return hd.p;
}

void CMementoMemoryManager::CMementoMemoryManagerAllocator::FreePtr( void * p, size_t sz )
{
	ASSERT_GLOBAL_LOCK;

	m_mutex.Lock();

	int pool = std::max( (int)IntegerLog2_RoundUp(sz) - FIRST_POOL, 0 );

	MMMDebugRemoveAlloc(p, sz);

	if (pool < NPOOLS)
	{
		m_pools[pool]->Free(p);
	}
	else
	{
		m_largeObjectPool.Free(p);
	}

	m_mutex.Unlock();
}

void CMementoMemoryManager::CMementoMemoryManagerAllocator::InitHandleData( SHandleData& hd, size_t sz )
{
	ASSERT_GLOBAL_LOCK;

	size_t szP2 = IntegerLog2_RoundUp(sz);
	int pool = std::max( (int)szP2 - FIRST_POOL, 0 );

	hd.size = sz;

	if (pool < NPOOLS)
	{
		hd.pPool = m_pools[pool];
		hd.p = hd.pPool->Allocate( hd.capacity );
	}
	else
	{
		hd.pPool = &m_largeObjectPool;
		hd.p = m_largeObjectPool.AllocateSized( sz, hd.capacity );
	}

	MMMDebugAddAlloc(hd.p, hd.size);
	NET_ASSERT(hd.capacity >= sz);
}

void CMementoMemoryManager::CMementoMemoryManagerAllocator::DebugDraw(int& y, size_t& totalPoolAllocations, size_t& totalPoolSize)
{
#if ENABLE_DEBUG_KIT
	DrawDebugLine( 0, y++, "Memento allocator memory" );

	for (int i = 0; i < NPOOLS; i++)
	{
		SPoolStats stats = m_pools[i]->GetPoolStats();

		if (stats.allocated)
		{
			DrawDebugLine(2, y++, "Pool %d (%d byte blocks): alloc=%d, used=%d, waste=%.1f%%", i, 1<<(i+FIRST_POOL), stats.allocated, stats.used, stats.GetWastePercent());

			totalPoolAllocations += stats.used;
			totalPoolSize += stats.allocated;
		}
	}

	DrawDebugLine(2, y++, "Total: used=%d, alloc=%d, waste=%.1f%%", totalPoolAllocations, totalPoolSize, GetWastePercent(totalPoolAllocations, totalPoolSize));
#endif
}

CMementoMemoryManager::CMementoMemoryManager( const string& name ) : m_name(name)
{
	ASSERT_GLOBAL_LOCK;

	CMementoMemoryManagerAllocator::AddCMementoMemoryManager();

#if ENABLE_DEBUG_KIT
	if (!m_pManagers)
	{
		m_pManagers = new TManagers;
	}

	m_pManagers->push_back(this);
#endif

	arith_zeroSizeHdl = InvalidHdl;

	for (int i=0; i<sizeof(pThings)/sizeof(*pThings); i++)
	{
		pThings[i] = 0;
	}

	m_totalAllocations = 0;
}

CMementoMemoryManager::~CMementoMemoryManager()
{
	SCOPED_GLOBAL_LOCK;

	FreeHdl(arith_zeroSizeHdl);

	MMM_REGION(this);

	for (int i=0; i<sizeof(pThings)/sizeof(*pThings); i++)
	{
		SAFE_RELEASE(pThings[i]);
	}

	NET_ASSERT(m_totalAllocations == 0);

#if ENABLE_DEBUG_KIT
	stl::find_and_erase(*m_pManagers, this);
#endif

	CMementoMemoryManagerAllocator::RemoveCMementoMemoryManager();
}

CMementoMemoryManager::Hdl CMementoMemoryManager::AllocHdl( size_t sz, void * callerOverride )
{
	ASSERT_GLOBAL_LOCK;

	Hdl hdl = CMementoMemoryManagerAllocator::GetAllocator()->AllocHdl(sz);

	m_totalAllocations += sz;

#if MMM_CHECK_LEAKS
	void * caller = callerOverride? callerOverride : UP_STACK_PTR;
	m_hdlToAlloc[hdl] = caller;
	m_allocAmt[caller] += sz;
#endif

	return hdl;
}

CMementoMemoryManager::Hdl CMementoMemoryManager::CloneHdl( Hdl hdl )
{
	ASSERT_GLOBAL_LOCK;

	Hdl out = AllocHdl( GetHdlSize(hdl), UP_STACK_PTR );

	memcpy( PinHdl(out), PinHdl(hdl), GetHdlSize(hdl) );

	return out;
}

void CMementoMemoryManager::ResizeHdl( Hdl hdl, size_t sz )
{
	ASSERT_GLOBAL_LOCK;

	size_t oldSize = GetHdlSize(hdl);
	
	CMementoMemoryManagerAllocator::GetAllocator()->ResizeHdl(hdl, sz);

	m_totalAllocations -= oldSize;
	m_totalAllocations += sz;

#if MMM_CHECK_LEAKS
	void * caller = m_hdlToAlloc[hdl];
	uint32& callerAlloc = m_allocAmt[caller];
	callerAlloc -= oldSize;
	callerAlloc += sz;
#endif
}

void CMementoMemoryManager::FreeHdl( Hdl hdl )
{
	ASSERT_GLOBAL_LOCK;

	size_t size = GetHdlSize(hdl);

	m_totalAllocations -= size;

#if MMM_CHECK_LEAKS
	void * caller = m_hdlToAlloc[hdl];

	m_hdlToAlloc.erase(hdl);

	if (!(m_allocAmt[caller] -= size))
	{
		m_allocAmt.erase(caller);
	}
#endif

	CMementoMemoryManagerAllocator::GetAllocator()->FreeHdl(hdl);
}

void * CMementoMemoryManager::AllocPtr( size_t sz, void * callerOverride )
{
	ASSERT_GLOBAL_LOCK;

	m_totalAllocations += sz;

	void* ret = CMementoMemoryManagerAllocator::GetAllocator()->AllocPtr(sz);

#if MMM_CHECK_LEAKS
	void * caller = callerOverride? callerOverride : UP_STACK_PTR;
	m_ptrToAlloc[ret] = caller;
	m_allocAmt[caller] += sz;
#endif

	return ret;
}

void CMementoMemoryManager::FreePtr( void * p, size_t sz )
{
	CMementoMemoryManagerAllocator::GetAllocator()->FreePtr(p, sz);

	m_totalAllocations -= sz;

#if MMM_CHECK_LEAKS
	void * caller = m_ptrToAlloc[p];

	m_ptrToAlloc.erase(p);

	if (!(m_allocAmt[caller] -= sz))
	{
		m_allocAmt.erase(caller);
	}
#endif
}

void CMementoMemoryManager::AddHdlToSizer( Hdl hdl, ICrySizer * pSizer )
{
	if (hdl != InvalidHdl)
	{
		pSizer->AddObject( PinHdl(hdl), GetHdlSize(hdl) );
	}
}

void CMementoMemoryManager::GetMemoryStatistics(ICrySizer* pSizer, bool countingThis /* = false */)
{
	SIZER_COMPONENT_NAME(pSizer, "CMementoMemoryManager");

	if (countingThis)
	{
		pSizer->Add(*this);
	}
}

void CMementoMemoryManager::DebugDraw()
{
#if ENABLE_DEBUG_KIT
	if (m_pManagers && CMementoMemoryManagerAllocator::GetAllocator() && CVARS.MemInfo&eDMM_Mementos)
	{
		static size_t maxTotalPoolSize = 0;
		static size_t maxTotalPoolAllocations = 0;
		static size_t maxTotalAllocations = 0;
		size_t totalPoolSize = 0;
		size_t totalPoolAllocations = 0;
		size_t totalAllocations = 0;
		int y = 2;

		CMementoMemoryManagerAllocator::GetAllocator()->DebugDraw(y, totalPoolAllocations, totalPoolSize);
		y++;

		for (TManagers::iterator it = m_pManagers->begin(); it != m_pManagers->end(); ++it)
		{
			DrawDebugLine( 0, y++, "Memento memory for %s: live=%d", (*it)->m_name.c_str(), (*it)->m_totalAllocations );
			totalAllocations += (*it)->m_totalAllocations;
		}

		y++;

		if (totalAllocations)
		{
			bool changed = false;

			if (totalPoolSize > maxTotalPoolSize)
			{
				maxTotalPoolSize = totalPoolSize;
				changed = true;
			}

			if (totalPoolAllocations > maxTotalPoolAllocations)
			{
				maxTotalPoolAllocations = totalPoolAllocations;
				changed = true;
			}

			if (totalAllocations > maxTotalAllocations)
			{
				maxTotalAllocations = totalAllocations;
				changed = true;
			}

			DrawDebugLine( 0, y++, "Total MMM: live=%d, used=%d, alloc=%d, live-waste=%.1f%%",
				totalAllocations, totalPoolAllocations, totalPoolSize, GetWastePercent(totalAllocations, totalPoolSize));

			DrawDebugLine( 0, y++, "Max Total MMM: live=%d, used=%d, alloc=%d, live-waste=%.1f%%",
				maxTotalAllocations, maxTotalPoolAllocations, maxTotalPoolSize, GetWastePercent(maxTotalAllocations, maxTotalPoolSize));

			if (changed)
			{
				CryLogAlways( "[net mmm] Max Total MMM: live=%d, used=%d, alloc=%d, live-waste=%.1f%%",
					maxTotalAllocations, maxTotalPoolAllocations, maxTotalPoolSize, GetWastePercent(maxTotalAllocations, maxTotalPoolSize));
			}
		}

		MMMDebugDraw();
	}
#endif
}

/*
 * CMementoStreamAllocator
 */

CMementoStreamAllocator::CMementoStreamAllocator( const CMementoMemoryManagerPtr& mmm ) : m_hdl(CMementoMemoryManager::InvalidHdl), m_pPin(0), m_mmm(mmm)
{
}

void * CMementoStreamAllocator::Alloc( size_t sz, void * callerOverride )
{
	NET_ASSERT( m_hdl == CMementoMemoryManager::InvalidHdl );

	m_hdl = m_mmm->AllocHdl(sz, callerOverride? callerOverride : UP_STACK_PTR);

	return m_pPin = m_mmm->PinHdl(m_hdl);
}

void * CMementoStreamAllocator::Realloc( void * old, size_t sz )
{
	NET_ASSERT(m_pPin == old && m_pPin);
	m_mmm->ResizeHdl( m_hdl, sz );

	return m_pPin = m_mmm->PinHdl(m_hdl);
}

void CMementoStreamAllocator::Free( void *old )
{
}
