#ifndef __MEMENTOMEMORYMANAGER_H__
#define __MEMENTOMEMORYMANAGER_H__

#pragma once

#include "STLPoolAllocator.h"
#include "Config.h"

struct IMementoManagedThing
{
	virtual void Release() = 0;
};

// handle based memory manager that can repack data to save fragmentation
// TODO: re-introduce repacking for handles (but not pointers)
class CMementoMemoryManager : public CMultiThreadRefCount
{
	friend class CMementoStreamAllocator;

public:
	static const size_t ALIGNMENT = 4; // alignment for mementos

	// workaround for arithmetic alphabet stuff
	uint32 arith_zeroSizeHdl;
	IMementoManagedThing * pThings[64];

	// who's using pThings:
	//   0 - arith row sym cache
	//   1 - arith row low cache

	CMementoMemoryManager( const string& name );
	~CMementoMemoryManager();

	typedef uint32 Hdl;
	static const Hdl InvalidHdl = ~Hdl(0);

	void * AllocPtr( size_t sz, void * callerOverride = 0 );
	void FreePtr( void * p, size_t sz );
	Hdl AllocHdl( size_t sz, void * callerOverride = 0 );
	Hdl CloneHdl( Hdl hdl );
	void ResizeHdl( Hdl hdl, size_t sz );
	void FreeHdl( Hdl hdl );
	void AddHdlToSizer( Hdl hdl, ICrySizer * pSizer );
	ILINE void*	 PinHdl( Hdl hdl ) const											{ return CMementoMemoryManagerAllocator::GetAllocator()->PinHdl(hdl); }
	ILINE size_t GetHdlSize( Hdl hdl ) const									{ return CMementoMemoryManagerAllocator::GetAllocator()->GetHdlSize(hdl); }

	void GetMemoryStatistics(ICrySizer* pSizer, bool countingThis = false);

	static void DebugDraw();

private:
	class CMementoMemoryManagerAllocator
	{
		struct SPoolStats
		{
			SPoolStats() : allocated(0), used(0) {}
			size_t allocated;
			size_t used;

			float GetWastePercent() const
			{
				return allocated ? 100.0f*(1.0f - float(used)/float(allocated)) : 0.0f;
			}
		};

		struct IPool
		{
			virtual void * Allocate( size_t& alloced ) = 0;
			virtual void Free( void * p ) = 0;
			virtual SPoolStats GetPoolStats() { return SPoolStats(); }
			virtual ~IPool() {}
		};

		template <size_t SZ>
		class CPool : public IPool
		{
		public:
			virtual void* Allocate( size_t& alloced )
			{
				alloced = SZ;
				return m_impl.Allocate();
			}
			virtual void Free( void * p )
			{
				m_impl.Deallocate( p );
			}
			virtual SPoolStats GetPoolStats()
			{
				SPoolStats stats;
				stats.allocated = m_impl.GetTotalAllocatedMemory();
				stats.used = m_impl.GetTotalAllocatedNodeSize();
				return stats;
			}

		private:
			stl::PoolAllocator<SZ, stl::PoolAllocatorSynchronizationSinglethreaded> m_impl;
		};

		class CLargeObjectPool : public IPool
		{
		public:
			virtual void * Allocate( size_t& ) { CryFatalError("Cannot call Allocate() on this object."); return 0; }
			virtual void Free( void * p ) { CryModuleFree(p); }
			ILINE void * AllocateSized( size_t sz, size_t& alloced ) { alloced = sz; return CryModuleMalloc(sz); }
		};

		struct SHandleData
		{
			void*  p;
			size_t size;
			size_t capacity;
			IPool* pPool;
		};

	public:
		CMementoMemoryManagerAllocator();
		~CMementoMemoryManagerAllocator();

		static CMementoMemoryManagerAllocator* GetAllocator()	{ return m_allocator; }
		static void AddCMementoMemoryManager();
		static void RemoveCMementoMemoryManager();

		Hdl AllocHdl(size_t sz);
		void FreeHdl(Hdl hdl);
		void* AllocPtr(size_t sz);
		void FreePtr(void* p, size_t sz);
		void ResizeHdl(Hdl hdl, size_t sz);
		void InitHandleData(SHandleData& hd, size_t sz);
		void DebugDraw(int& y, size_t& totalPoolAllocations, size_t& totalPoolSize);
		ILINE void* PinHdl(Hdl hdl) const										
		{ 
			hdl = UnprotectHdl(hdl); 
			return (hdl != InvalidHdl) ? m_handles[hdl].p : NULL; 
		}
		ILINE size_t GetHdlSize( Hdl hdl ) const						
		{ 
			hdl = UnprotectHdl(hdl);
			return (hdl != InvalidHdl) ? m_handles[hdl].size : 0; 
		}

	private:
		ILINE Hdl ProtectHdl( Hdl x ) const
		{
#if CRYNETWORK_RELEASEBUILD
			if (x!=InvalidHdl)
			{	
				return (x<<1) ^ (uint32(this)+1);			// ensures 0xFFFFFFFF cannot be a valid result (this will always be at least 4 byte aligned)
			}
#endif
			return x;
		}

		ILINE Hdl UnprotectHdl( Hdl x ) const
		{
#if CRYNETWORK_RELEASEBUILD
			if (x!=InvalidHdl)
			{
				return (x ^ (uint32(this)+1))>>1;
			}
#endif
			return x;
		}

		// pool sizes are 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096
		static const int FIRST_POOL = 3; // == 8bytes
		static const int LAST_POOL = 12; // == 4096bytes
		static const int NPOOLS = LAST_POOL - FIRST_POOL + 1;

		IPool* m_pools[NPOOLS];
		CLargeObjectPool m_largeObjectPool;
		std::vector<SHandleData> m_handles;
		std::vector<uint32> m_freeHandles;

		static CMementoMemoryManagerAllocator* m_allocator;
		static int m_numCMementoMemoryManagers;
		static CryLockT<CRYLOCK_RECURSIVE> m_mutex;
	};

	size_t m_totalAllocations;
	string m_name;

#if MMM_CHECK_LEAKS
	std::map<void*, void*> m_ptrToAlloc;
	std::map<uint32, void*> m_hdlToAlloc;
	std::map<void*, uint32> m_allocAmt;
#endif

#if ENABLE_DEBUG_KIT
	typedef std::list<CMementoMemoryManager*> TManagers;
	static TManagers * m_pManagers;
#endif
};

typedef CMementoMemoryManager::Hdl TMemHdl;
const TMemHdl TMemInvalidHdl = CMementoMemoryManager::InvalidHdl;

typedef _smart_ptr<CMementoMemoryManager> CMementoMemoryManagerPtr;

class CMementoStreamAllocator : public IStreamAllocator
{
public:
	CMementoStreamAllocator(const CMementoMemoryManagerPtr& mmm);

	void * Alloc( size_t sz, void * callerOverride );
	void * Realloc( void * old, size_t sz );
	void Free( void *old ); // WARNING: no-op (calling code uses GetHdl() to grab this...)

	TMemHdl GetHdl() const { return m_hdl; }

private:
	TMemHdl m_hdl;
	void * m_pPin;
	CMementoMemoryManagerPtr m_mmm;
};


#endif
