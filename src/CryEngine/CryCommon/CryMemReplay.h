////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2001-2010.
// -------------------------------------------------------------------------
//  File name:   CryMemReplay.h
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CryMemReplay_h__
#define __CryMemReplay_h__
#pragma once

#if CAPTURE_REPLAY_LOG
// Memory replay interface, access it with CryGetMemReplay call
struct IMemReplay
{
	virtual void DumpStats(void) = 0;
	virtual void ReplayDumpSymbols(void) = 0;
	virtual void ReplayStart(bool bPaused=false) = 0;
	virtual void ReplayStop(void) = 0;
	virtual void MemStatAlloc(void *p, uint32 size) = 0;
	virtual void MemStatFree(void *p) = 0;
	virtual void MemStatAddLabel(const char* label) = 0;
	virtual void MemStatAddFrameStart() = 0;
	virtual int  MemStatLock() = 0;
	virtual void MemStatUnlock() = 0;
	virtual void ReplayGetInfo(CryReplayInfo& infoOut) = 0;
	virtual void MemStatAllocUsage(void* p, uint32 used) = 0;
	virtual void MemAddContextV(int type, uint32 flags, const char* format, va_list args) = 0;
	virtual void MemRemoveContext() = 0;
};
#else //CAPTURE_REPLAY_LOG
struct IMemReplay
{
	void DumpStats(void) {};
	void ReplayDumpSymbols(void) {};
	void ReplayStart(bool bPaused=false) {};
	void ReplayStop(void) {};
	void MemStatAlloc(void *p, uint32 size) {};
	void MemStatFree(void *p) {};
	void MemStatAddLabel(const char* label) {};
	void MemStatAddFrameStart() {};
	int  MemStatLock() { return 0; };
	void MemStatUnlock() {};
	void ReplayGetInfo(CryReplayInfo& infoOut) {};
	void MemStatAllocUsage(void* p, uint32 used) {};
	void MemAddContextV(int type, uint32 flags, const char* format, va_list args) {};
	void MemRemoveContext() {};
};
#endif //CAPTURE_REPLAY_LOG

inline IMemReplay* CryGetIMemReplay()
{
	static IMemReplay *s_pMemReplay = 0;
	if (!s_pMemReplay)
	{
		// get it from System		
		s_pMemReplay = CryGetIMemoryManager()->GetIMemReplay();
	}
	return s_pMemReplay;
}

#if defined(__cplusplus) && CAPTURE_REPLAY_LOG
namespace EMemStatContextTypes
{
	// Add new types at the end, do not modify existing values.
	enum Type
	{
		MSC_MAX = 0,
		MSC_CGF = 1,
		MSC_MTL = 2,
		MSC_DBA = 3,
		MSC_CHR = 4,
		MSC_LMG = 5,
		MSC_AG = 6,
		MSC_Texture = 7,
		MSC_ParticleLibrary = 8,

		MSC_Physics = 9,
		MSC_Terrain = 10,
		MSC_Shader = 11,
		MSC_Other = 12,
		MSC_RenderMesh = 13,
		MSC_Entity = 14,
		MSC_Navigation = 15,
		MSC_ScriptCall = 16,

		MSC_CDF = 17,

		MSC_RenderMeshType = 18,

		MSC_ANM = 19,
		MSC_CGA = 20,
		MSC_CAF = 21,
		MSC_ArchetypeLib = 22,

		MSC_SoundProject = 23,
		MSC_EntityArchetype=24,

		MSC_LUA = 25,
		MSC_D3D = 26,
		MSC_ParticleEffect = 27,
		MSC_SoundBuffer = 28,
		MSC_FSB = 29,  // Sound bank data
	};
}

namespace EMemStatContextFlags
{
	enum Flags
	{
		MSF_None = 0,
		MSF_Instance = 1,
	};
}

class CMemStatContext
{
public:
	CMemStatContext(EMemStatContextTypes::Type type, uint32 flags, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		CryGetIMemReplay()->MemAddContextV(type, flags, format, args);
		va_end(args);
	}
	~CMemStatContext()
	{
		CryGetIMemReplay()->MemRemoveContext();
	}
private:
	CMemStatContext(const CMemStatContext&);
	CMemStatContext& operator = (const CMemStatContext&);
};
#endif // CAPTURE_REPLAY_LOG

#if CAPTURE_REPLAY_LOG
#define INCLUDE_MEMSTAT_CONTEXTS 1
#define INCLUDE_MEMSTAT_ALLOC_USAGES 1
#else
#define INCLUDE_MEMSTAT_CONTEXTS 0
#define INCLUDE_MEMSTAT_ALLOC_USAGES 0
#endif


#if INCLUDE_MEMSTAT_CONTEXTS
#define MEMSTAT_CONTEXT(type, id, str) CMemStatContext _memctx(type, id, str);
#define MEMSTAT_CONTEXT_NAMED(name, type, id, str) CMemStatContext _##name (type, id, str);
#define MEMSTAT_CONTEXT_FMT(type, id, format, ...) CMemStatContext _memctx(type, id, format, __VA_ARGS__);
#define MEMSTAT_CONTEXT_NAMED_FMT(name, type, id, format, ...) CMemStatContext _##name (type, id, format, __VA_ARGS__);
#else
#define MEMSTAT_CONTEXT(...)
#define MEMSTAT_CONTEXT_NAMED(...)
#define MEMSTAT_CONTEXT_FMT(...)
#define MEMSTAT_CONTEXT_NAMED_FMT(...)
#endif

#if INCLUDE_MEMSTAT_ALLOC_USAGES
#define MEMSTAT_USAGE(ptr, size) (CryGetIMemReplay()->MemStatAllocUsage(ptr, size));
#else
#define MEMSTAT_USAGE(ptr, size)
#endif

#if CAPTURE_REPLAY_LOG
class CMemStatAllocLock
{
public:
	CMemStatAllocLock()
		: m_lockState(CryGetIMemReplay()->MemStatLock())
	{}

	~CMemStatAllocLock()
	{
		if (m_lockState & MSL_NeedsUnlock)
			CryGetIMemReplay()->MemStatUnlock();
	}

	void Stat(void* p, size_t sz)
	{
		if (m_lockState & MSL_NeedsUnlock)
			CryGetIMemReplay()->MemStatUnlock();
		if (m_lockState & MSL_Owner)
			CryGetIMemReplay()->MemStatAlloc(p, sz);
		m_lockState = 0;
	}

private:
	CMemStatAllocLock(const CMemStatAllocLock&);
	CMemStatAllocLock& operator = (const CMemStatAllocLock&);

private:
	int m_lockState;
};

class CMemStatFreeLock
{
public:
	CMemStatFreeLock(void* p)
		: m_lockState(CryGetIMemReplay()->MemStatLock())
		, m_p(p)
	{}

	~CMemStatFreeLock()
	{
		if (m_lockState & MSL_NeedsUnlock)
			CryGetIMemReplay()->MemStatUnlock();
		if (m_lockState & MSL_Owner)
			CryGetIMemReplay()->MemStatFree(m_p);
	}

private:
	CMemStatFreeLock(const CMemStatFreeLock&);
	CMemStatFreeLock& operator = (const CMemStatFreeLock&);

private:
	int m_lockState;
	void* m_p;
};
#endif

#endif  //__CryMemReplay_h__