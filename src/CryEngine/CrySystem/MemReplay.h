#ifndef __MemReplay_h__
#define __MemReplay_h__

#define REPLAY_RECORD_FREECS 0
#define REPLAY_RECORD_USAGE_CHANGES 1
#define REPLAY_RECORD_THREADED 1

#if CAPTURE_REPLAY_LOG

#include "ICryPak.h"
#include "HeapAllocator.h"
#include "zlib/zlib.h"
#include "CryThread.h"

namespace MemReplayEventIds
{
	enum Ids
	{
		RE_Alloc,
		RE_Free,
		RE_Callstack,
		RE_FrameStart,
		RE_Label,
		RE_XenonModuleRef,
		RE_AllocVerbose,
		RE_FreeVerbose,
		RE_Info,
		RE_PushContext,
		RE_PopContext,
		RE_Alloc3,
		RE_Free3,
		RE_PushContext2,
		RE_PS3ModuleRef,
		RE_AddressProfile,
		RE_PushContext3,
		RE_Free4,
		RE_AllocUsage,
		RE_Info2,
		RE_Alloc64,
		RE_Free64,
	};
}

#pragma pack(push)
#pragma pack(1)

struct MemReplayEventHeader
{
	MemReplayEventHeader(int id, size_t size)
		: sequenceCheck(0)
		, eventId(static_cast<uint8>(id))
		, eventLength(static_cast<uint16>(size))
	{
	}

	uint8 sequenceCheck;
	uint8 eventId;
	uint16 eventLength;
};

struct MemReplayFrameStartEvent
{
	static const int EventId = MemReplayEventIds::RE_FrameStart;

	uint32 frameId;

	MemReplayFrameStartEvent(uint32 frameId)
		: frameId(frameId)
	{}
};

struct MemReplayLabelEvent
{
	static const int EventId = MemReplayEventIds::RE_Label;

	char label[1];

	MemReplayLabelEvent(const char* label)
	{
		// Assume there is room beyond this instance.
		strcpy(this->label, label); // we're intentionally writing beyond the end of this array, so don't use strcpy_s
	}
};

struct MemReplayPushContextEvent
{
	static const int EventId = MemReplayEventIds::RE_PushContext3;

	uint32 threadId;
	uint32 contextType;
	uint32 flags;

	// This field must be the last in the structure, and enough memory should be allocated
	// for the structure to hold the required name.
	char name[1];

	MemReplayPushContextEvent(uint32 threadId, const char* name, EMemStatContextTypes::Type type, uint32 flags)
	{
		// We're going to assume that there actually is enough space to store the name directly in the struct.

		this->threadId = threadId;
		this->contextType = static_cast<uint32>(type);
		this->flags = flags;
		strcpy(this->name, name); // we're intentionally writing beyond the end of this array, so don't use strcpy_s
	}
};

struct MemReplayPopContextEvent
{
	static const int EventId = MemReplayEventIds::RE_PopContext;

	uint32 threadId;

	explicit MemReplayPopContextEvent(uint32 threadId)
	{
		this->threadId = threadId;
	}
};

struct MemReplayXenonModuleRefEvent
{
	static const int EventId = MemReplayEventIds::RE_XenonModuleRef;

	char name[256];
	char path[256];
	char sig[512];

	MemReplayXenonModuleRefEvent(const char* name, const char* path, const char* sig)
	{
		strncpy_s(this->name, name, sizeof(this->name));
		this->name[sizeof(this->name) - 1] = '\0';

		strncpy_s(this->path, path, sizeof(this->path));
		this->path[sizeof(this->path) - 1] = '\0';

		strncpy_s(this->sig, sig,sizeof(this->sig));
		this->sig[sizeof(this->sig) - 1] = '\0';
	}
};

struct MemReplayPS3ModuleRefEvent
{
	static const int EventId = MemReplayEventIds::RE_PS3ModuleRef;

	char name[256];

	MemReplayPS3ModuleRefEvent(const char* name)
	{
		strncpy_s(this->name, name, sizeof(this->name));
		this->name[sizeof(this->name) - 1] = '\0';
	}
};

struct MemReplayAllocEvent
{
	static const int EventId = MemReplayEventIds::RE_Alloc3;

	uint32 threadId;
	uint32 ptr;
	uint32 sizeRequested;
	uint32 sizeConsumed;
	int32 sizeGlobal;			//  Inferred from changes in global memory status

	uint32 callstackLength;
	uint32 callstack[1];	// Must be last.

	MemReplayAllocEvent(uint32 threadId, uint32 ptr, uint32 sizeReq, uint32 sizeCon, int32 sizeGlobal)
		: threadId(threadId)
		, ptr(ptr)
		, sizeRequested(sizeReq)
		, sizeConsumed(sizeCon)
		, sizeGlobal(sizeGlobal)
		, callstackLength(0)
	{
	}
};

struct MemReplayAllocEvent64
{
	static const int EventId = MemReplayEventIds::RE_Alloc64;

	uint32 threadId;
	uint64 ptr;
	uint32 sizeRequested;
	uint32 sizeConsumed;
	int32 sizeGlobal;			//  Inferred from changes in global memory status

	uint32 callstackLength;
	uint64 callstack[1];	// Must be last.

	MemReplayAllocEvent64(uint32 threadId, uint64 ptr, uint32 sizeReq, uint32 sizeCon, int32 sizeGlobal)
		: threadId(threadId)
		, ptr(ptr)
		, sizeRequested(sizeReq)
		, sizeConsumed(sizeCon)
		, sizeGlobal(sizeGlobal)
		, callstackLength(0)
	{
	}
};

struct MemReplayFreeEvent
{
	static int const EventId = MemReplayEventIds::RE_Free3;

	uint32 threadId;
	uint32 ptr;
	int32 sizeGlobal;

	MemReplayFreeEvent(uint32 threadId, uint32 ptr, int32 sizeGlobal)
		: threadId(threadId)
		, ptr(ptr)
		, sizeGlobal(sizeGlobal)
	{
	}
};

struct MemReplayFreeCSEvent
{
	static const int EventId = MemReplayEventIds::RE_Free4;

	uint32 threadId;
	uint32 ptr;
	int32 sizeGlobal;			//  Inferred from changes in global memory status

	uint32 callstackLength;
	uint32 callstack[1];	// Must be last.

	MemReplayFreeCSEvent(uint32 threadId, uint32 ptr, int32 sizeGlobal)
		: threadId(threadId)
		, ptr(ptr)
		, sizeGlobal(sizeGlobal)
		, callstackLength(0)
	{
	}
};

struct MemReplayFreeEvent64
{
	static int const EventId = MemReplayEventIds::RE_Free64;

	uint32 threadId;
	uint64 ptr;
	int32 sizeGlobal;

	MemReplayFreeEvent64(uint32 threadId, uint64 ptr, int32 sizeGlobal)
		: threadId(threadId)
		, ptr(ptr)
		, sizeGlobal(sizeGlobal)
	{
	}
};

struct MemReplayInfoEvent
{
	static const int EventId = MemReplayEventIds::RE_Info;

	uint32 preTrackedSize;
	uint32 trackingSize;

	MemReplayInfoEvent(uint32 preTrackedSize, uint32 trackingSize)
		: preTrackedSize(preTrackedSize)
		, trackingSize(trackingSize)
	{
	}
};

struct MemReplayInfoEvent2
{
	static const int EventId = MemReplayEventIds::RE_Info2;

	uint32 preTrackedSize;
	uint32 trackingSize;
	uint32 bucketsFree;

	MemReplayInfoEvent2(uint32 preTrackedSize, uint32 trackingSize, uint32 bucketsFree)
		: preTrackedSize(preTrackedSize)
		, trackingSize(trackingSize)
		, bucketsFree(bucketsFree)
	{
	}
};

struct MemReplayAddressProfileEvent
{
	static const int EventId = MemReplayEventIds::RE_AddressProfile;

	uint32 rsxStart;

	MemReplayAddressProfileEvent(uint32 rsxStart)
	{
		this->rsxStart = rsxStart;
	}
};

struct MemReplayAllocUsageEvent
{
	static const int EventId = MemReplayEventIds::RE_AllocUsage;

	uint32 address;
	uint32 used;

	MemReplayAllocUsageEvent(void* address, uint32 used)
		: address((uint32) address)
		, used(used)
	{
	}
};

#pragma pack(pop)

class ReplayCompressor
{
public:
	ReplayCompressor(FILE* fp);
	~ReplayCompressor();

	size_t GetInternalSize() const;
	void Write(const byte* data, size_t len);

private:
	static voidpf zAlloc (voidpf opaque, uInt items, uInt size);
	static void   zFree  (voidpf opaque, voidpf address);

private:
	ReplayCompressor(const ReplayCompressor&);
	ReplayCompressor& operator = (const ReplayCompressor&);

private:
	FILE* m_fp;

	std::vector<byte> m_compressTarget;
	z_stream m_compressStream;
	int m_zSize;
};

#if REPLAY_RECORD_THREADED

void CryMemStatIgnoreThread(DWORD threadId);

class ReplayRecordThread : public CryThread<>
{
public:
	ReplayRecordThread(FILE* fp);

	void Write(const byte* data, size_t len);
	size_t GetInternalSize() const;

public:
	void Run();
	void Cancel();

private:
	volatile int m_stop;
	const byte* volatile m_nextData;
	long volatile m_nextLength;

	ReplayCompressor m_compressor;
};
#endif

class ReplayLogStream
{
public:
	ReplayLogStream();

	bool Open();

	void CleanupOldLogs(const char* ext);

	template <typename T>
	void WriteEvent(const T* ev, size_t evLength)
	{
		uint32 size = sizeof(MemReplayEventHeader) + evLength;

		if (((&m_buffer->back() + 1) - m_bufferEnd) < size)
			Flush();

		MemReplayEventHeader* header = reinterpret_cast<MemReplayEventHeader*>(m_bufferEnd);
		new (header) MemReplayEventHeader(T::EventId, evLength);

		memcpy(m_bufferEnd + sizeof(MemReplayEventHeader), ev, evLength);
		m_bufferEnd += size;
	}

	template <typename T>
	void WriteEvent(const T& ev)
	{
		WriteEvent(&ev, sizeof(T));
	}

	void Flush();
	void FullFlush();

	size_t GetSize() const;
	uint64 GetUncompressedLength() const;

	void Close();

	bool IsOpen() const { return m_isOpen != 0; }
	const char* const GetFilename() { return m_filename; }

private:
	volatile int m_isOpen;

	char m_filename[128];
	std::vector<uint8>* m_buffer;
	uint8* m_bufferEnd;

	uint64 m_uncompressedLen;

#if REPLAY_RECORD_THREADED
	std::vector<uint8> m_bufferA;
	std::vector<uint8> m_bufferB;
	ReplayRecordThread* m_recordThread;
#else
	std::vector<uint8> m_bufferA;
	ReplayCompressor* m_compressor;
#endif

	FILE* m_fp;
};

void MemReplayCaptureCallstack(void** callstack, uint32& callstackLength);
void MemReplayRecordAlloc(void* p, size_t sizeRequested, size_t sizeConsumed, ptrdiff_t sizeGlobal);
void MemReplayRecordFreeCS(void* p, ptrdiff_t sizeGlobal);
void MemReplayRecordFree(void* p, ptrdiff_t sizeGlobal);
void MemReplayRecordModules();

extern int GetPageBucketAlloc_wasted_in_allocation();
extern int GetPageBucketAlloc_get_free();

//////////////////////////////////////////////////////////////////////////
class CMemReplay : public IMemReplay
{
public:
	//////////////////////////////////////////////////////////////////////////
	// IMemReplay interface implementation
	//////////////////////////////////////////////////////////////////////////
	void DumpStats(void);
	void ReplayDumpSymbols(void);
	void ReplayStart(bool bPaused);
	void ReplayStop(void);
	void MemStatAlloc(void *p, uint32 size);
	void MemStatFree(void *p);
	void MemStatAddLabel(const char* label);
	void MemStatAddFrameStart();
	int  MemStatLock();
	void MemStatUnlock();
	void ReplayGetInfo(CryReplayInfo& infoOut);
	void MemStatAllocUsage(void* p, uint32 used);
	void MemAddContextV(int type, uint32 flags, const char* format, va_list args);
	void MemRemoveContext();
	//////////////////////////////////////////////////////////////////////////

	static CMemReplay* GetInstance();
};
//////////////////////////////////////////////////////////////////////////

#endif

#endif
