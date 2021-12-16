#include "StdAfx.h"
#include "MTSafeAllocator.h"
#include <IConsole.h>

int CMTSafeHeap::m_numBigAllocationSize = 24*1024*1024;
int CMTSafeHeap::m_numBigAllocationStartSize = 1*512;

extern CMTSafeHeap* g_pPakHeap;













CMTSafeHeap::CMTSafeHeap() 





{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "CryPak Heap");

	//IConsole* pConsole( gEnv->pConsole );
	//if( 0 != pConsole )
	//{
	//	REGISTER_CVAR2( "sys_mtsafeheap_size", &m_numBigAllocationSize, m_numBigAllocationSize, VF_DUMPTODISK, "Sets the CMTSafeHeap big allocation size" );
	//	REGISTER_CVAR2( "sys_mtsafeheap_startsize", &m_numBigAllocationStartSize, m_numBigAllocationStartSize, VF_DUMPTODISK, "Sets the CMTSafeHeap start size for big allocation usage" );
	//}

#if !defined(XENON) && !defined(PS3) && !defined(LINUX)
	m_iBigPoolSize[0] = 32 * 1024;
	m_iBigPoolSize[1] = 32 * 1024;
	m_iBigPoolSize[2] = 32 * 1024;
	m_iBigPoolSize[3] = 32 * 1024;

	m_iBigPoolSize[4] = 128 * 1024;
	m_iBigPoolSize[5] = 128 * 1024;

	m_iBigPoolSize[6] = 256 * 1024;
	m_iBigPoolSize[7] = 256 * 1024;
	m_iBigPoolSize[8] = 256 * 1024;
	m_iBigPoolSize[9] = 512 * 1024;
	m_iBigPoolSize[10] = 512 * 1024;
	m_iBigPoolSize[11] = 512 * 1024;
	m_iBigPoolSize[12] = 1024 * 1024;
	m_iBigPoolSize[13] = 2048 * 1024;
	m_iBigPoolSize[14] = 5 * 1024 * 1024;
	m_iBigPoolSize[15] = 14 * 1024 * 1024;

#else




















  m_iBigPoolSize[0] = 1;//0 * 1024;
  m_iBigPoolSize[1] = 1;//0 * 1024;
  m_iBigPoolSize[2] = 1;//0 * 1024;
  m_iBigPoolSize[3] = 1;//0 * 1024;
  m_iBigPoolSize[4] = 1;//0 * 1024 * 1024;
  m_iBigPoolSize[5] = 1;//0 * 1024 * 1024;
	m_iBigPoolSize[6] = 1;//0 * 1024 * 1024;
  m_iBigPoolSize[7] = 1;//0 * 1024 * 1024;
  m_iBigPoolSize[8] = 1;//0 * 1024 * 1024;
	m_iBigPoolSize[9] = 1;//0 * 1024 * 1024;
	m_iBigPoolSize[10] = 1;//0 * 1024 * 1024;
	m_iBigPoolSize[11] = 1;//0 * 1024 * 1024;
	m_iBigPoolSize[12] = 1;//0 * 1024 * 1024;
	m_iBigPoolSize[13] = 1;//0 * 1024 * 1024;
	m_iBigPoolSize[14] = 1;//0 * 1024 * 1024;
	m_iBigPoolSize[15] = 1;//0 * 1024 * 1024;


#endif
//	m_numBigAllocationSize >> (NUM_POOLS - i - 1);

	for (int i = 0; i < NUM_POOLS; ++i)
	{
		
		m_pBigPool[i] = 0;
		if (m_iBigPoolSize[i])
			m_pBigPool[i] = static_cast<void*>(new char[m_iBigPoolSize[i]]);

		m_iBigPoolUsed[i] = 0;
		m_sBigPoolDescription[i] = "";
		if (!m_pBigPool[i])
		{
	//		CryFatalError("Failed CMTSafeHeap::m_pBigPool allocation");
#if defined(PS3) || defined(LINUX)
			CRY_ASSERT_MESSAGE(0, "Failed CMTSafeHeap::m_pBigPool allocation");
#else

#endif
		}
	}

	m_numAllocations = 0;






















};

CMTSafeHeap::~CMTSafeHeap() 
{
	for (int i = 0; i < NUM_POOLS; ++i)
		delete [] (uint8*)m_pBigPool[i];















};

void* CMTSafeHeap::Alloc (size_t nSize, const char* szDbgSource,bool bUsePools)
{
	m_numAllocations++;





#if !defined(XENON) && !defined(PS3)
	if (bUsePools && (int)nSize >= m_numBigAllocationStartSize && (int)nSize <= m_numBigAllocationSize)
	{
		for (int i = 0; i < NUM_POOLS; ++i)
		{
			if (!m_iBigPoolUsed[i] && (int)nSize < m_iBigPoolSize[i])
			{
				m_sBigPoolDescription[i] = szDbgSource;
				if (CryInterlockedCompareExchange(&m_iBigPoolUsed[i], 1, 0) == 0)
					return m_pBigPool[i];
			}
		}

		//CryWarning(VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"CMTSafeHeap::m_pBigPool is already used. %s uses malloc instead", szDbgSource );
	}
#endif

	return malloc(nSize);


};


void CMTSafeHeap::Free (void*p)
{
	m_numAllocations--;




	for (int i = 0; i < NUM_POOLS; ++i)
	{
		if (p == m_pBigPool[i])
		{
			m_sBigPoolDescription[i] = "";
			m_iBigPoolUsed[i] = 0;
			return;
		}
	}

	free(p);

};

//////////////////////////////////////////////////////////////////////////
void CMTSafeHeap::FreePools()
{
	// Potentially unsafe operation!!
	for (int i = 0; i < NUM_POOLS; ++i)
	{
		if (m_iBigPoolUsed[i] == 0 && m_pBigPool[i])
		{
			m_iBigPoolSize[i] = 0;
			delete [] (uint8*)m_pBigPool[i];
			m_pBigPool[i] = 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void* CMTSafeHeap::StaticAlloc (void* pOpaque, unsigned nItems, unsigned nSize)
{
	return g_pPakHeap->Alloc(nItems*nSize,"StaticAlloc");
}

//////////////////////////////////////////////////////////////////////////
void CMTSafeHeap::StaticFree (void* pOpaque, void* pAddress)
{
	g_pPakHeap->Free(pAddress);
}

//////////////////////////////////////////////////////////////////////////
void CMTSafeHeap::GetMemoryUsage( ICrySizer *pSizer )
{
	SIZER_COMPONENT_NAME(pSizer, "FileSystem Pools");
	for (int i = 0; i < NUM_POOLS; ++i)
	{
		if (m_pBigPool[i])
		{
			pSizer->AddObject( m_pBigPool[i],m_iBigPoolSize[i] );
		}
	}
}




























































































































































