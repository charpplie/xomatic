#include "StdAfx.h"
#include "BitFiddling.h"

#include <stdio.h>
#include <ISystem.h>
#include <platform.h>

#include <CryMemoryAllocator.h>
#include "MemoryManager.h"

#include "MemReplay.h"

#if CAPTURE_REPLAY_LOG

#ifdef WIN32
#include "DebugCallStack.h"
#include "Psapi.h"
#endif


extern ReplayLogStream g_memReplayStream;
static uint32 g_memReplayFrameCount;
static bool g_memReplayLogInitialised = false;
const int k_maxCallStackDepth=256;

static volatile int s_replayLastWasted = 0;
static volatile int s_replayLastGlobal = 0;
static volatile int s_replayPreTrackSize = 0;
static volatile int s_replayStartingFree = 0;
static int bLock=0;





ReplayCompressor::ReplayCompressor(FILE* fp)
: m_fp(fp)
, m_zSize(0)
{
	memset(&m_compressStream, 0, sizeof(m_compressStream));
	m_compressStream.zalloc = &ReplayCompressor::zAlloc;
	m_compressStream.zfree = &ReplayCompressor::zFree;
	m_compressStream.opaque = this;
	deflateInit(&m_compressStream, 2);//Z_DEFAULT_COMPRESSION);

	uint64 streamLen = 0;
	fwrite(&streamLen, sizeof(streamLen), 1, m_fp);
}

ReplayCompressor::~ReplayCompressor()
{
	deflateEnd(&m_compressStream);
}

size_t ReplayCompressor::GetInternalSize() const
{
	return m_zSize + m_compressTarget.capacity();
}

void ReplayCompressor::Write(const byte* data, size_t len)
{
	if (m_compressTarget.size() < len)
		m_compressTarget.resize(len);

	m_compressStream.next_in = const_cast<byte*>(data);
	m_compressStream.avail_in = len;
	m_compressStream.next_out = &m_compressTarget[0];
	m_compressStream.avail_out = m_compressTarget.size();
	m_compressStream.total_out = 0;

	do 
	{
		int err = deflate(&m_compressStream, Z_SYNC_FLUSH);

		if ((err == Z_OK && m_compressStream.avail_out == 0) || (err == Z_BUF_ERROR && m_compressStream.avail_out == 0))
		{
			int total_out = m_compressStream.total_out;
			fwrite(&m_compressTarget[0], total_out * sizeof(uint8), 1, m_fp);

			m_compressStream.next_out = &m_compressTarget[0];
			m_compressStream.avail_out = m_compressTarget.size();
			m_compressStream.total_out = 0;

			continue;
		}
		else if (m_compressStream.avail_in == 0)
		{
			int total_out = m_compressStream.total_out;
			fwrite(&m_compressTarget[0], total_out * sizeof(uint8), 1, m_fp);
		}
		else
		{
			// Shrug?
		}

		break;
	}
	while (true);

	fflush(m_fp);
}

voidpf ReplayCompressor::zAlloc(voidpf opaque, uInt items, uInt size)
{
	ReplayCompressor* str = reinterpret_cast<ReplayCompressor*>(opaque);
	str->m_zSize += items * size;

	int* ptr = (int*) malloc(sizeof(int) + items * size);
	*ptr = items * size;
	return ptr + 1;
}

void ReplayCompressor::zFree(voidpf opaque, voidpf address)
{
	int* ptr = reinterpret_cast<int*>(address);
	ReplayCompressor* str = reinterpret_cast<ReplayCompressor*>(opaque);
	str->m_zSize -= ptr[-1];
	free(ptr - 1);
}

#if REPLAY_RECORD_THREADED

ReplayRecordThread::ReplayRecordThread(FILE* fp)
: m_compressor(fp)
, m_stop(0)
, m_nextData(NULL)
, m_nextLength(0)
{
}

void ReplayRecordThread::Write(const byte* data, size_t len)
{
	Lock();

	while (m_nextData)
	{
		Unlock();
		Sleep(0);
		Lock();
	}

	m_nextData = data;
	m_nextLength = len;
	NotifySingle();

	Unlock();
}

size_t ReplayRecordThread::GetInternalSize() const
{
	return m_compressor.GetInternalSize();
}

void ReplayRecordThread::Run()
{
	CryMemStatIgnoreThread(CryGetCurrentThreadId());

	while (true)
	{
		Lock();

		while (!m_stop && m_nextData == NULL)
			Wait();

		if (m_stop)
		{
			Unlock();
			break;
		}

		size_t length = m_nextLength;
		m_nextLength = 0;

		const byte* data = m_nextData;
		m_nextData = NULL;

		m_compressor.Write(data, length);

		Unlock();
	}
}

void ReplayRecordThread::Cancel()
{
	Lock();

	CryLog("Cancelling replay record thread.");

	m_stop = 1;
	NotifySingle();

	Unlock();
}

#endif

ReplayLogStream::ReplayLogStream()
: m_isOpen(0)
#if REPLAY_RECORD_THREADED
, m_recordThread(NULL)
#else
, m_compressor(NULL)
#endif
{
}

bool ReplayLogStream::Open()
{
	m_bufferA.resize(1 * 1024 * 1024);

#if REPLAY_RECORD_THREADED
	m_bufferB.resize(1 * 1024 * 1024);
#endif

	m_buffer = &m_bufferA;

	m_fp = NULL;

	{
		time_t curTime;
		time(&curTime);
		struct tm* lt = localtime(&curTime);

		strftime(m_filename, sizeof(m_filename) / sizeof(m_filename[0]), "memlog-%Y%m%d-%H%M%S.zmrl", lt);

		m_bufferEnd = &(*m_buffer)[0];
		
		/*
		// Write Stream description.
		memcpy(&(*m_buffer)[0], "MRL", 4);
		m_bufferEnd += 4;
		*/

		CryInterlockedIncrement(&m_isOpen);
	}

	return true;
}

void ReplayLogStream::CleanupOldLogs(const char* ext)
{

























}

void ReplayLogStream::Flush()
{
	if (m_fp == NULL)
	{
		CleanupOldLogs("mrl");
		CleanupOldLogs("zmrl");

		m_fp = fxopen(m_filename, "wb");
		if (m_fp == NULL)
		{
			fprintf(stdout, "Failed to open \"%s\"\n", m_filename);
		}
		else
		{
#if REPLAY_RECORD_THREADED
			m_recordThread = new ReplayRecordThread(m_fp);
			m_recordThread->Start();
#else
			m_compressor = new ReplayCompressor(m_fp);
#endif
		}
	}

	if (m_fp != NULL)
	{
		m_uncompressedLen += m_bufferEnd - &(*m_buffer)[0];

#if REPLAY_RECORD_THREADED
		m_recordThread->Write(&(*m_buffer)[0], m_bufferEnd - &(*m_buffer)[0]);
		m_buffer = (m_buffer == &m_bufferA) ? &m_bufferB : &m_bufferA;
#else
		m_compressor->Write(&(*m_buffer)[0], m_bufferEnd - &(*m_buffer)[0]);
#endif
	}

	m_bufferEnd = &(*m_buffer)[0];
}

void ReplayLogStream::FullFlush()
{
	Flush();
	fclose(m_fp);
	m_fp = fxopen(m_filename, "r+b");
	if (m_fp)
		fseek(m_fp, 0, SEEK_END);
}

size_t ReplayLogStream::GetSize() const
{
	return m_bufferA.capacity() + sizeof(*this)
#if REPLAY_RECORD_THREADED
		+ m_bufferB.capacity()
		+ (m_recordThread ? m_recordThread->GetInternalSize() + sizeof(*m_recordThread) : 0)
#else
		+ (m_compressor ? m_compressor->GetInternalSize() + sizeof(*m_compressor) : 0)
#endif
		;
}

uint64 ReplayLogStream::GetUncompressedLength() const
{
	return m_uncompressedLen;
}

void ReplayLogStream::Close()
{
	using std::swap;

	CryLog("Closing memory log.");

	// Flag it as closed here, to prevent recursion into the memory logging as it is closing.
	CryInterlockedDecrement(&m_isOpen);

	Flush();

#if REPLAY_RECORD_THREADED
	m_recordThread->Write(NULL, 0);
	m_recordThread->Cancel();
	m_recordThread->WaitForThread();
	delete m_recordThread;
	m_recordThread = 0;

	std::vector<byte>().swap(m_bufferB);
#endif

	CryLog("Flushed memory log.");

	fseek(m_fp, 0, SEEK_SET);

	fwrite(&m_uncompressedLen, sizeof(m_uncompressedLen), 1, m_fp);

	CryLog("Written uncompressed length of %llu", m_uncompressedLen);

	fclose(m_fp);

	CryLog("Closed memory log.");

	std::vector<byte>().swap(m_bufferA);
	m_buffer = NULL;
	m_bufferEnd = 0;

#if !REPLAY_RECORD_THREADED
	delete m_compressor;
	m_compressor = 0;
#endif
}

void MemReplayCaptureCallstack(void** callstack, uint32& callstackLength)
{
	memset(callstack, 0, sizeof(void*) * callstackLength);

	uint32 callstackCapacity = callstackLength;

	callstackLength = 0;

	int nNumStackFramesToSkip = 3;
























#if defined(WIN32)
	if (callstackCapacity > 40)
		callstackCapacity = 40;
	callstackLength = RtlCaptureStackBackTrace( nNumStackFramesToSkip,callstackCapacity,callstack,NULL );
#endif
/*
	static int aaa = 0;
	aaa++;

	if ((aaa & 0xF)  == 0)
	{
		CryLogAlways( "RtlCaptureStackBackTrace = (%d)",callstackLength );
		for (int i=0; i<callstackLength; i++)
		{
			CryLogAlways( "   [%d] = (%X)",i,callstack[i] );
		}
	}

	callstackLength = IDebugCallStack::instance()->CollectCallStackFrames( callstack,callstackCapacity );
	if ((aaa & 0xF)  == 0)
	{
		CryLogAlways( "StackWalk64 = (%d)",callstackLength );
		for (int i=0; i<callstackLength; i++)
		{
			CryLogAlways( "   [%d] = (%X)",i,callstack[i] );
		}
	}
	*/

	if (callstackLength > 0)
	{
		// for some inexplicable reason, std::reverse crashes where this implementation doesn't...
//		std::reverse(callstack, callstack + callstackLength);
	

		for (int i=0; i<callstackLength/2; i++)
		{
			void *tmp = callstack[i];
			callstack[i] = callstack[callstackLength-i-1];
			callstack[callstackLength-i-1] = tmp;
		}
	}
}

void MemReplayRecordAlloc(void* p, size_t sizeRequested, size_t sizeConsumed, ptrdiff_t sizeGlobal)
{
	uint8 tmpBuffer[k_maxCallStackDepth*sizeof(void*) + sizeof(MemReplayAllocEvent) - sizeof(reinterpret_cast<MemReplayAllocEvent*>(0)->callstack)];

	MemReplayAllocEvent* ev = reinterpret_cast<MemReplayAllocEvent*>(tmpBuffer);
	new (ev) MemReplayAllocEvent(
		CryGetCurrentThreadId(),
		reinterpret_cast<uint32>(p),
		static_cast<uint32>(sizeRequested),
		static_cast<uint32>(sizeConsumed),
		static_cast<int32>(sizeGlobal));

	assert (sizeof(void*) == sizeof(uint32));

	ev->callstackLength = k_maxCallStackDepth;
	MemReplayCaptureCallstack(reinterpret_cast<void**>(ev->callstack), ev->callstackLength);

	if (ev->callstackLength > 1)
	{
		g_memReplayStream.WriteEvent(ev, sizeof(MemReplayAllocEvent) - sizeof(ev->callstack) + sizeof(ev->callstack[0]) * ev->callstackLength);
	}
}

void MemReplayRecordAlloc64(void* p, size_t sizeRequested, size_t sizeConsumed, ptrdiff_t sizeGlobal)
{
	uint8 tmpBuffer[k_maxCallStackDepth*sizeof(void*) + sizeof(MemReplayAllocEvent64) - sizeof(reinterpret_cast<MemReplayAllocEvent64*>(0)->callstack)];

	MemReplayAllocEvent64* ev = reinterpret_cast<MemReplayAllocEvent64*>(tmpBuffer);
	new (ev) MemReplayAllocEvent64(
		CryGetCurrentThreadId(),
		reinterpret_cast<uint64>(p),
		static_cast<uint32>(sizeRequested),
		static_cast<uint32>(sizeConsumed),
		static_cast<int32>(sizeGlobal));

	assert (sizeof(void*) == sizeof(uint64));

	ev->callstackLength = k_maxCallStackDepth;
	MemReplayCaptureCallstack(reinterpret_cast<void**>(ev->callstack), ev->callstackLength);

	if (ev->callstackLength > 1)
	{
		g_memReplayStream.WriteEvent(ev, sizeof(MemReplayAllocEvent64) - sizeof(ev->callstack) + sizeof(ev->callstack[0]) * ev->callstackLength);
	}
}

void MemReplayRecordFreeCS(void* p, ptrdiff_t sizeGlobal)
{
	uint8 tmpBuffer[k_maxCallStackDepth * sizeof(uint32) + sizeof(MemReplayFreeCSEvent) - sizeof(reinterpret_cast<MemReplayFreeCSEvent*>(0)->callstack)];

	MemReplayFreeCSEvent* ev = reinterpret_cast<MemReplayFreeCSEvent*>(tmpBuffer);
	new (ev) MemReplayFreeCSEvent(
		CryGetCurrentThreadId(),
		reinterpret_cast<uint32>(p),
		static_cast<int32>(sizeGlobal));

	assert (sizeof(void*) == sizeof(uint32));

	ev->callstackLength = k_maxCallStackDepth;
	MemReplayCaptureCallstack(reinterpret_cast<void**>(ev->callstack), ev->callstackLength);

	if (ev->callstackLength > 1)
	{
		g_memReplayStream.WriteEvent(ev, sizeof(MemReplayFreeCSEvent) - sizeof(ev->callstack) + sizeof(ev->callstack[0]) * ev->callstackLength);
	}
}

void MemReplayRecordFree(void* p, ptrdiff_t sizeGlobal)
{
	g_memReplayStream.WriteEvent(MemReplayFreeEvent(
		CryGetCurrentThreadId(),
		reinterpret_cast<uint32>(p),
		static_cast<int32>(sizeGlobal)));
}

void MemReplayRecordFree64(void* p, ptrdiff_t sizeGlobal)
{
	g_memReplayStream.WriteEvent(MemReplayFreeEvent64(
		CryGetCurrentThreadId(),
		reinterpret_cast<uint64>(p),
		static_cast<int32>(sizeGlobal)));
}

void MemReplayRecordModules()
{































#if defined(WIN32)
	CryLog("Win");

	HMODULE hMods[1024];
	HANDLE hProcess;
	DWORD cbNeeded;
	unsigned int i;

	// Get a list of all the modules in this process.
	hProcess = OpenProcess( PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE, GetCurrentProcessId() );
	if (NULL != hProcess)
	{
		if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
		{
			for ( i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
			{
				TCHAR szModName[MAX_PATH];
				// Get the full path to the module's file.
				if ( GetModuleFileNameEx( hProcess, hMods[i], szModName,sizeof(szModName) / sizeof(TCHAR)))
				{
					CryLog( "%s\n", szModName );
					//CryLog( "%s\n", signature.Path );

					stack_string pdbPath = PathUtil::ReplaceExtension( stack_string(szModName),"pdb" );

					char sig[512];
					strcpy(sig,"");
					/*
					sprintf( sig, "%p, %08X, %08X, {%08X-%04X-%04X-%02X %02X-%02X %02X %02X %02X %02X %02X}, %d\n",
						modLoad.BaseAddress, modLoad.Size, modLoad.TimeStamp,
						signature.Guid.Data1, signature.Guid.Data2, signature.Guid.Data3,
						signature.Guid.Data4[0], signature.Guid.Data4[1],
						signature.Guid.Data4[2], signature.Guid.Data4[3],
						signature.Guid.Data4[4], signature.Guid.Data4[5],
						signature.Guid.Data4[6], signature.Guid.Data4[7],
						signature.Age );
						*/

					g_memReplayStream.WriteEvent(MemReplayXenonModuleRefEvent(szModName, pdbPath.c_str(), sig));

				}
			}
		}
		CloseHandle( hProcess );
	}
#endif //WIN32

	g_memReplayStream.FullFlush();
}



#include "CryThread.h"
typedef CryLockT<CRYLOCK_RECURSIVE> MemFastMutex;
MemFastMutex memmutex;
static volatile DWORD s_ignoreThreadId = (DWORD)-1;

void CryMemStatIgnoreThread(DWORD threadId)
{
	s_ignoreThreadId = threadId;
}


extern volatile bool g_bMemStatsProcessed;
ReplayLogStream g_memReplayStream;
static bool g_memReplayPaused = false;

//////////////////////////////////////////////////////////////////////////
// CMemReplay class implementation.
//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	static int GetCurrentSysAlloced()
	{
		int trackingUsage = g_memReplayStream.GetSize();
#ifdef WIN32
		IMemoryManager::SProcessMemInfo mi;
		CCryMemoryManager::GetInstance()->GetProcessMemInfo(mi);
		return (uint32)(mi.PagefileUsage - trackingUsage);
#else
		MEMORYSTATUS mem;
		GlobalMemoryStatus(&mem);
		return (mem.dwTotalPhys - mem.dwAvailPhys) - trackingUsage;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	static int GetCurrentSysFree()
	{
		int trackingUsage = g_memReplayStream.GetSize();
#ifdef WIN32
		MEMORYSTATUS mem;
		GlobalMemoryStatus(&mem);

		IMemoryManager::SProcessMemInfo mi;
		CCryMemoryManager::GetInstance()->GetProcessMemInfo(mi);
		return (uint32)(mem.dwAvailVirtual) + trackingUsage;
#else
		MEMORYSTATUS mem;
		GlobalMemoryStatus(&mem);
		return mem.dwAvailPhys + trackingUsage;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::DumpStats(void)
	{
		if (g_memReplayStream.IsOpen())
		{
			CryAutoLock<CryCriticalSection> lock(memmutex);
			if (g_memReplayStream.IsOpen())
			{
				g_bMemStatsProcessed = true;

				int trackingUsage = g_memReplayStream.GetSize();
				g_memReplayStream.WriteEvent(MemReplayInfoEvent2(s_replayPreTrackSize, trackingUsage, s_replayStartingFree));

				MemReplayRecordModules();

				g_bMemStatsProcessed = false;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::ReplayDumpSymbols(void)
	{
		if (g_memReplayStream.IsOpen())
		{
			CryAutoLock<CryCriticalSection> lock(memmutex);
			if (g_memReplayStream.IsOpen())
			{
				g_bMemStatsProcessed = true;

				int trackingUsage = g_memReplayStream.GetSize();
				g_memReplayStream.WriteEvent(MemReplayInfoEvent2(s_replayPreTrackSize, trackingUsage, s_replayStartingFree));

				MemReplayRecordModules();

				g_bMemStatsProcessed = false;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::ReplayStart(bool bPaused)
	{
		CryAutoLock<CryCriticalSection> lock(memmutex);
		g_memReplayPaused = bPaused;

		if (!g_memReplayStream.IsOpen())
		{
			g_bMemStatsProcessed = true;

			g_memReplayStream.Open();

			g_bMemStatsProcessed = false;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::ReplayStop(void)
	{
		CryAutoLock<CryCriticalSection> lock(memmutex);
		g_bMemStatsProcessed = true;

		if (g_memReplayStream.IsOpen())
			g_memReplayStream.Close();

		g_bMemStatsProcessed = false;
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::MemStatAlloc(void *p, uint32 size)
	{
		if (p && CryGetCurrentThreadId() != s_ignoreThreadId && g_memReplayStream.IsOpen() && !g_memReplayPaused)
		{
			CryAutoLock<CryCriticalSection> lock(memmutex);
			if (g_memReplayStream.IsOpen() && !g_bMemStatsProcessed && !bLock)
			{
				g_bMemStatsProcessed = true;

				if (!g_memReplayLogInitialised)
				{
					CryInterlockedAdd(&s_replayPreTrackSize, GetCurrentSysAlloced());
					CryInterlockedAdd(&s_replayLastGlobal, GetCurrentSysFree());




					s_replayLastWasted = GetPageBucketAlloc_wasted_in_allocation();
					CryInterlockedAdd(&s_replayStartingFree, GetPageBucketAlloc_get_free());





					g_memReplayStream.WriteEvent(MemReplayAddressProfileEvent(0xffffffff));


					g_memReplayStream.WriteEvent(MemReplayFrameStartEvent(g_memReplayFrameCount ++));

					g_memReplayLogInitialised = true;
				}

				int changeGlobal = 0;
				while (1)
				{
					int sysFree = GetCurrentSysFree();
					int lastFree = s_replayLastGlobal;
					changeGlobal = lastFree - sysFree;
					if (CryInterlockedCompareExchange((volatile long*) &s_replayLastGlobal, sysFree, lastFree) == lastFree)
						break;
				} 




				size_t currentWasted = GetPageBucketAlloc_wasted_in_allocation();


#ifndef WIN64
				MemReplayRecordAlloc(p, size, size + (currentWasted - s_replayLastWasted), changeGlobal);
#else
				MemReplayRecordAlloc64(p, size, size + (currentWasted - s_replayLastWasted), changeGlobal);
#endif

				s_replayLastWasted = currentWasted;

				g_bMemStatsProcessed = false;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::MemStatFree(void *p)
	{
		if (p && CryGetCurrentThreadId() != s_ignoreThreadId && g_memReplayStream.IsOpen() && !g_memReplayPaused)
		{
			CryAutoLock<CryCriticalSection> lock(memmutex);
			if (g_memReplayStream.IsOpen() && !g_bMemStatsProcessed && !bLock)
			{
				g_bMemStatsProcessed=true;

				int changeGlobal = 0;
				while (1)
				{
					int sysFree = GetCurrentSysFree();
					int lastFree = s_replayLastGlobal;
					changeGlobal = lastFree - sysFree;
					if (CryInterlockedCompareExchange((volatile long*) &s_replayLastGlobal, sysFree, lastFree) == lastFree)
						break;
				} 

#ifndef WIN64
#if REPLAY_RECORD_FREECS
				MemReplayRecordFreeCS(p, changeGlobal);
#else //REPLAY_RECORD_FREECS
				MemReplayRecordFree(p, changeGlobal);
#endif //REPLAY_RECORD_FREECS

#else //WIN64
				MemReplayRecordFree64(p, changeGlobal);
#endif //WIN64

				g_bMemStatsProcessed=false;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::MemStatAddLabel(const char* label)
	{
		if (g_memReplayStream.IsOpen())
		{
			CryAutoLock<CryCriticalSection> lock(memmutex);

			if (g_memReplayStream.IsOpen())
			{
				g_bMemStatsProcessed = true;

				size_t evLen = sizeof(MemReplayLabelEvent) + strlen(label) + 1;
				MemReplayLabelEvent* ev = new (alloca(evLen)) MemReplayLabelEvent(label);
				g_memReplayStream.WriteEvent(ev, evLen);

				g_bMemStatsProcessed = false;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::MemStatAddFrameStart()
	{
		if (g_memReplayStream.IsOpen())
		{
			static bool bSymbolsDumped = false;
			if (!bSymbolsDumped)
			{
				ReplayDumpSymbols();
				bSymbolsDumped = true;
			}
	
			CryAutoLock<CryCriticalSection> lock(memmutex);

			if (g_memReplayStream.IsOpen())
			{
				g_bMemStatsProcessed = true;

				g_memReplayStream.WriteEvent(MemReplayFrameStartEvent(g_memReplayFrameCount ++));

				g_bMemStatsProcessed = false;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	int CMemReplay::MemStatLock()
	{
		if (g_memReplayStream.IsOpen())
		{
			memmutex.Lock();
			bLock++;
			return MSL_NeedsUnlock | (bLock == 1 ? MSL_Owner : 0);
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::MemStatUnlock()
	{
		if (g_memReplayStream.IsOpen())
		{
			bLock--;
			memmutex.Unlock();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::ReplayGetInfo(CryReplayInfo& infoOut)
	{
		memset(&infoOut, 0, sizeof(CryReplayInfo));

		if (g_memReplayStream.IsOpen())
		{
			infoOut.uncompressedLength = g_memReplayStream.GetUncompressedLength();
			infoOut.trackingSize = g_memReplayStream.GetSize();
			infoOut.filename = g_memReplayStream.GetFilename();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::MemStatAllocUsage(void* ptr, uint32 used)
	{
#if REPLAY_RECORD_USAGE_CHANGES
		if (!ptr)
			return;

		if (g_memReplayStream.IsOpen())
		{
			CryAutoLock<CryCriticalSection> lock(memmutex);
			if (!g_bMemStatsProcessed)
			{
				g_bMemStatsProcessed = true;
				if (g_memReplayStream.IsOpen())
				{
					g_memReplayStream.WriteEvent(MemReplayAllocUsageEvent(ptr, used));
				}
				g_bMemStatsProcessed = false;
			}
		}
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::MemAddContextV(int type, uint32 flags, const char* format, va_list args)
	{
		DWORD threadId = CryGetCurrentThreadId();
		if (g_memReplayStream.IsOpen() && threadId != s_ignoreThreadId && !g_memReplayPaused)
		{
			char input[512];
			vsnprintf(input, sizeof(input), format, args);
			input[sizeof(input) - 1] = 0;

			CryAutoLock<CryCriticalSection> lock(memmutex);

			if (g_memReplayStream.IsOpen())
			{
				g_bMemStatsProcessed = true;

				{
					size_t cap = Align(sizeof(MemReplayPushContextEvent) + strlen(input) + 1, 4);
					MemReplayPushContextEvent* ev = new (alloca(cap)) MemReplayPushContextEvent(threadId, input, (EMemStatContextTypes::Type) type, flags);

					g_memReplayStream.WriteEvent(ev, cap);
				}

				g_bMemStatsProcessed = false;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	void CMemReplay::MemRemoveContext()
	{
		DWORD threadId = CryGetCurrentThreadId();

		if (g_memReplayStream.IsOpen() && threadId != s_ignoreThreadId && !g_memReplayPaused)
		{
			CryAutoLock<CryCriticalSection> lock(memmutex);

			if (g_memReplayStream.IsOpen())
			{
				g_bMemStatsProcessed = true;

				g_memReplayStream.WriteEvent(MemReplayPopContextEvent(CryGetCurrentThreadId()));

				g_bMemStatsProcessed = false;
			}
		}
	}

	CMemReplay* CMemReplay::GetInstance()
	{
		static CMemReplay s_instance;
		return &s_instance;
	}




























































#else // CAPTURE_REPLAY_LOG

#endif //CAPTURE_REPLAY_LOG


