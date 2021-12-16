// This is the prototypes of interfaces that will be used for asynchronous
// I/O (streaming).
// THIS IS NOT FINAL AND IS SUBJECT TO CHANGE WITHOUT NOTICE

// Some excerpts explaining basic ideas behind streaming design here:

/*
 * The idea is that the data loaded is ready for usage and ideally doesn't need further transformation,
 * therefore the client allocates the buffer (to avoid extra copy). All the data transformations should take place in the Resource Compiler. If you have to allocate a lot of small memory objects, you should revise this strategy in favor of one big allocation (again, that will be read directly from the compiled file).
 * Anyway, we can negotiate that the streaming engine allocates this memory.
 * In the end, it could make use of a memory pool, and copying data is not the bottleneck in our engine
 *
 * The client should take care of all fast operations. Looking up file size should be fast on the virtual
 * file system in a pak file, because the directory should be preloaded in memory
 */
#include DEVIRTUALIZE_HEADER_FIX(IStreamEngine.h)

#ifndef _CRY_COMMON_STREAM_ENGINE_HDR_
#define _CRY_COMMON_STREAM_ENGINE_HDR_

#include <list>
#include "smartptr.h"
#include "IThreadTask.h"
#include "CryThread.h"

#ifndef MAKEFOURCC
#define NEED_UNDEF_MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
				((uint32)(uint8)(ch0) | ((uint32)(uint8)(ch1) << 8) |       \
				((uint32)(uint8)(ch2) << 16) | ((uint32)(uint8)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

struct StreamEngineParams;
class IStreamCallback;
class ICrySizer;

enum
{
	ERROR_UNKNOWN_ERROR          = 0xF0000000,
	ERROR_UNEXPECTED_DESTRUCTION = 0xF0000001,
	ERROR_INVALID_CALL           = 0xF0000002,
	ERROR_CANT_OPEN_FILE         = 0xF0000003,
	ERROR_REFSTREAM_ERROR        = 0xF0000004,
	ERROR_OFFSET_OUT_OF_RANGE    = 0xF0000005,
	ERROR_REGION_OUT_OF_RANGE    = 0xF0000006,
	ERROR_SIZE_OUT_OF_RANGE      = 0xF0000007,
	ERROR_CANT_START_READING     = 0xF0000008,
	ERROR_OUT_OF_MEMORY          = 0xF0000009,
	ERROR_ABORTED_ON_SHUTDOWN    = 0xF000000A,
	ERROR_OUT_OF_MEMORY_QUOTA    = 0xF000000B,
	ERROR_ZIP_CACHE_FAILURE      = 0xF000000C,
	ERROR_USER_ABORT             = 0xF000000D
};

// Summary:
//	 Types of streaming tasks
//	 Affects priority directly (in order of declaration)
enum StreamTaskType
{
	eStreamTaskTypeMusic = 0,
	eStreamTaskTypeAnimation,
	eStreamTaskTypeGeometry,
	eStreamTaskTypeSound,
	eStreamTaskTypeTexture,
};

// Summary:
//	 Flags for the StreamReadParams structure.
// See also:
//	 StreamReadParams
enum StreamReadParamsFlagEnum
{
	// Description:
	//	 If this flag is set, the callback can be called from within stream engine's worker thread.
	// Notes: 
	//	 WARNING: Use with care.
	SRP_FLAGS_ASYNC_CALLBACK = 1,

	// Description:
	//	 If this flag is set, the file will be read synchronously.
	// Notes: 
	//	 Not implemented yet.
//	SRP_FLAGS_SYNC_READ = 1 << 1,

	// Description:
	//	 If this flag is set, the stream will be treated as "permanent" and the file handle will
	//	 be cached. This is needed for files which are accessed frequently, e.g. Resource files.
	SRP_FLAGS_MAKE_PERMANENT = 1<<2,

	// Description:
	//	 If this flag is set and the stream was made permanent before (either explicitly because of
	//	 the SRP_FLAGS_MAKE_PERMANENT flag, or implicitly because of the policy of the StreamEngine),
	//	 the stream will be removed as soon as the last proxy will be released.
	SRP_FLAGS_MAKE_TRANSIENT = 1 << 3,

	// Description:
	//	 With this flag, the progress routine will be called asynchronously.
	SRP_FLAGS_ASYNC_PROGRESS = 1 << 4,

	// Description:
	//	 This means that the path passed to StartRead is real path, and shouldn't undergo
	//	 adjustments through mod searching mechanics.
	SRP_FLAGS_PATH_REAL      = 1 << 5,

	// Description:
	//	 If this is set, it is advised that Update(0) be called during startread, which
	//	 can effectively call the callback before StartRead returns
	//	 SRP_IMMEDIATE_UPDATE and SRP_QUICK_RETURN are mutually exclusive.
//	SRP_IMMEDIATE_UPDATE     = 1 << 6,

	// Description:
	//	 If this is set, it is advised that Update(0) not be called during StartRead,
	//	 which yields quicker response.
	//	 SRP_IMMEDIATE_UPDATE and SRP_QUICK_RETURN are mutually exclusive.
	SRP_QUICK_STARTREAD      = 1 << 7,

	// Description:
	//	 If this flag is set, the second async callback is forced to be called in the main thread just before the main callback
	//	 it's only for debug purposes.
	SRP_FLAGS_FORCE_SYNC_CALLBACKS = 1 << 8,

	// Description:
	//	 This flag means that user takes control on the decompression 
	//	 inside one of the callbacks.
	SRP_FLAGS_USER_DECOMPRESSION_IN_CALLBACKS = 1 << 9,
};

class IReadStream;

// Description:
//	 This is used as parameter to the asynchronous read function
//	 all the unnecessary parameters go here, because there are many of them.
struct StreamReadParams
{
public:
	StreamReadParams (
		DWORD_PTR _dwUserData = 0,
		int _nPriority = 0,
		unsigned _nLoadTime = 0,
		unsigned _nMaxLoadTime = 0,
		unsigned _nOffset = 0,
		unsigned _nSize = 0,
		void* _pBuffer = NULL,
		unsigned _nFlags = 0
	):
		dwUserData (_dwUserData),
		nPriority(_nPriority),
		nLoadTime(_nLoadTime),
		nMaxLoadTime(_nMaxLoadTime),
		pBuffer (_pBuffer),
		nOffset (_nOffset),
		nSize (_nSize),
		nFlags (_nFlags)
	{
	}

	// Summary:
	//	 File name.
	//const char* szFile;

	// Summary:
	//	 The callback.
	//IStreamCallback* pAsyncCallback;

    // Summary:
	//	 The user data that'll be used to call the callback.
	DWORD_PTR dwUserData;

	// The priority of this read; INT_MIN is the idle, INT_MAX is the highest, 0 is the average.
	int nPriority;

	// Description:
	//	 The desirable loading time, in milliseconds, from the time of call
	//	 0 means as fast as possible (desirably in this frame).
	unsigned nLoadTime;

	// Description:
	//	 The maximum load time, in milliseconds. 0 means forever. If the read lasts longer, it can be discarded.
	//	 WARNING: avoid too small max times, like 1-10 ms, because many loads will be discarded in this case.
	unsigned nMaxLoadTime;

	// Description:
	//	 The buffer into which to read the file or the file piece
	//	 if this is NULL, the streaming engine will supply the buffer.
	// Notes:
	//	 DO NOT USE THIS BUFFER during read operation! DO NOT READ from it, it can lead to memory corruption!
	void* pBuffer;

	// Description:
	//	 Offset in the file to read; if this is not 0, then the file read
	//	 occurs beginning with the specified offset in bytes.
	//	 The callback interface receives the size of already read data as nSize
	//	 and generally behaves as if the piece of file would be a file of its own.
	unsigned nOffset;

	// Description:
	//	 Number of bytes to read; if this is 0, then the whole file is read,
	//	 if nSize == 0 && nOffset != 0, then the file from the offset to the end is read.
	//	 If nSize != 0, then the file piece from nOffset is read, at most nSize bytes
	//	 (if less, an error is reported). So, from nOffset byte to nOffset + nSize - 1 byte in the file.
	unsigned nSize;

	// Description:
	//	 The combination of one or several flags from StreamReadParamsFlagEnum.
	// See also:
	//	 StreamReadParamsFlagEnum
	unsigned nFlags;
};

// typedef IReadStream_AutoPtr auto ptr wrapper
TYPEDEF_AUTOPTR(IReadStream);
typedef IReadStream_AutoPtr IReadStreamPtr;

// Description:
//	 The highest level. There is only one StreamingEngine in the application
//	 and it controls all I/O streams.
UNIQUE_IFACE struct IStreamEngine
{
public:

	struct SStatistics
	{
		int									m_nBytesRead;
		float								m_fDeltaTime;
		struct SAsset
		{
			string	m_sName;
			int			m_nSize;
			const bool operator<(const SAsset& a) const { return m_nSize > a.m_nSize; }
			SAsset() {}
			SAsset(const string& sName, const int nSize):m_sName(sName), m_nSize(nSize) { }
		};
		std::vector<SAsset>	vecHeavyAssets;
	};

	enum EJobType
	{
		ejtStarted = 1<<0,
		ejtPending = 1<<1,
		ejtFinished = 1<<2,
	};

	// Summary:
	//	 General purpose flags.
	enum
	{
		// Description:
		//	 If this is set in the call to Update, the time quota for callbacks is ignored,
		//	 and all the callbacks for which the data is available are called.
		FLAGS_DISABLE_CALLBACK_TIME_QUOTA = 1 << 0,

		// Description:
		//	 If this is set in the call to Wait function will not reset m_hIOExecuted event
		//	 that mean that if you have already executed some job, you will return Wait() method immediately
		//	 useful for many parallel streams to force finalization.
		FLAGS_SILENT_WAIT = 1 << 1,
	};

	// Description:
	//	 Starts asynchronous read from the specified file (the file may be on a
	//	 virtual file system, in pak or zip file or wherever).
	//	 Reads the file contents into the given buffer, up to the given size.
	//	 Upon success, calls success callback. If the file is truncated or for other 
	//	 reason can not be read, calls error callback. The callback can be NULL (in this case, the client should poll
	//	 the returned IReadStream object; the returned object must be locked for that)
	// NOTE: the error/success/ progress callbacks can also be called from INSIDE this function.
	// Arguments:
	//	 szSource	-
	//	 szFile		-
	//	 pCallback	-
	//	 pParams	- PLACEHOLDER for the future additional parameters (like priority), or really
	//				  a pointer to a structure that will hold the parameters if there are too many of them.
	// Return Value:
	//	 IReadStream is reference-counted and will be automatically deleted if you don't refer to it;
	//	 if you don't store it immediately in an auto-pointer, it may be deleted as soon as on the next line of code,
	//	 because the read operation may complete immediately inside StartRead() and the object is self-disposed
	//	 as soon as the callback is called.
	// Remarks:
	//	 In some implementations disposal of the old pointers happen synchronously
	//	 (in the main thread) outside StartRead() (it happens in the entity update),
	//	 so you're guaranteed that it won't trash inside the calling function. However, this may change in the future
	//	 and you'll be required to assign it to IReadStream immediately (StartRead will return IReadStream_AutoPtr then).
	// See also:
	//	 IReadStream,IReadStream_AutoPtr
	virtual IReadStreamPtr StartRead (const StreamTaskType tSource, const char* szFile, IStreamCallback* pCallback = NULL, StreamReadParams* pParams = NULL) = 0;

	// Summary:
	//	 Returns the size of the file; 
	//	Return Value:
	//	 The size of the file or 0 if there's no such file.
	// Arguments:
	//	 szFile		  -
	//	 nCryPakFlags - the flag set as in ICryPak
	// See also:
	//	 ICryPak
	virtual unsigned GetFileSize (const char* szFile, unsigned nCryPakFlags = 0) = 0;

	// Description:
	//	 Signals to stream engine that this job is finished reading routines.
	virtual void OnIOJobFinishedRead (IReadStream_AutoPtr pJobStream) = 0;

	// Description:
	//	 Signals to stream engine that this job is finished all routines
	//	 including both reading and callback execution.
	virtual void OnIOJobExecuted			(IReadStream_AutoPtr pJobStream) = 0;

	// Summary:
	//	 Returns number of jobs with type EJobType
	virtual size_t GetNumJobs(uint32 jobType) = 0;

	// Summary:
	//	 Aborts the job execution.
	virtual void AbortJob(IReadStreamPtr pStream) = 0;

	// Description:
	//	 Waits at most the specified number of milliseconds, or until at least one pending operation is completed.
	// Arguments:
	//	 nFlags -  may have the following flag set: 
	//			   FLAGS_DISABLE_CALLBACK_TIME_QUOTA
	virtual void Update (unsigned nFlags = 0) = 0;

	// Description:
	//	 Waits at most the specified time for the IO jobs to be completed.
	// Return Value:
	//	 The number of jobs that actually were completed (finalized) during the call.
	// Notes:
	//	 It may be different from the number of executed jobs.
	virtual unsigned Wait(unsigned nMilliseconds, unsigned nFlags = 0) = 0;

	// Description:
	//	 Puts the memory statistics into the given sizer object.
	//	 According to the specifications in interface ICrySizer.
	// See also:
	//	 ICrySizer
	virtual void GetMemoryStatistics(ICrySizer *pSizer) = 0;

	// Description:
	//	 Threads pool to assign async callback tasks to.
	virtual ThreadPoolHandle GetThreadsPool() const = 0;

	// Description:
	//	 Enables or disables callback time quota per frame.
	virtual void SuspendCallbackTimeQuota(){}
	virtual void ResumeCallbackTimeQuota(){}

	// Description:
	//	 Returns the streaming statistics collected from the previous call.
	virtual void GetStreamingStatistics(SStatistics& stats) = 0;

	virtual ~IStreamEngine() {}
};


class AutoSuspendTimeQuota
{
public:
	AutoSuspendTimeQuota(IStreamEngine* pStreamEngine)
	{
		m_pStreamEngine = pStreamEngine;
		pStreamEngine->SuspendCallbackTimeQuota();
	}

	~AutoSuspendTimeQuota()
	{
		m_pStreamEngine->ResumeCallbackTimeQuota();
	}
protected:
	IStreamEngine* m_pStreamEngine;
};


// Description:
//	 This is the file "handle" that can be used to query the status
//	 of the asynchronous operation on the file. The same object may be returned
//	 for the same file to multiple clients.
// Notes:
//	 It will actually represent the asynchronous object in memory, and will be
//	 thread-safe reference-counted (both AddRef() and Release() will be virtual
//	 and thread-safe, just like the others)
// Example:
//	 USE:
//		 IReadStream_AutoPtr pReadStream = pStreamEngine->StartRead ("bla.xxx", this);
//	 OR:
//		 pStreamEngine->StartRead ("MusicSystem","bla.xxx", this);
UNIQUE_IFACE class IReadStream : public CMultiThreadRefCount
{
public:
	// Summary:
	//	 This class sets the priority order for the proxes.
	struct Order
	{
		bool operator ()(const IReadStream* pLeft, const IReadStream* pRight)const 
		{
			return pLeft->GetPriority() > pRight->GetPriority();
		}
	};
	// Summary:
	//	 Returns true if the file read was not successful.
	virtual bool IsError() = 0;
	// Return Value:
	//	 True if the file read was completed successfully.
	// Summary:
	//	 Checks IsError to check if the whole requested file (piece) was read.
	virtual bool IsFinished() = 0;
	// Description:
	//	 Returns the number of bytes read so far (the whole buffer size if IsFinished())
	// Arguments:
	//	 bWait - if == true, then waits until the pending I/O operation completes.
	// Return Value:
	//	 The total number of bytes read (if it completes successfully, returns the size of block being read)
	virtual unsigned int GetBytesRead(bool bWait=false) = 0;
	// Description:
	//	 Returns the buffer into which the data has been or will be read
	//	 at least GetBytesRead() bytes in this buffer are guaranteed to be already read.
	// Notes:
	//	 DO NOT USE THIS BUFFER during read operation! DO NOT READ from it, it can lead to memory corruption!
	virtual const void* GetBuffer () = 0;

	// Description:
	//	 Tries to raise the priority of the read; this is advisory and may have no effect.
	virtual void RaisePriority (unsigned int nPriority) = 0;

	// Summary:
	//	 Returns the priority of the read.
	virtual uint64 GetPriority()const = 0;

	// Description:
	//	 Returns the transparent DWORD that was passed in the StreamReadParams::dwUserData field
	//	 of the structure passed in the call to IStreamEngine::StartRead.
	// See also:
	//	 StreamReadParams::dwUserData,IStreamEngine::StartRead
	virtual DWORD_PTR GetUserData() = 0;

	// Summary:
	//	 Set user defined data into stream's params.
	virtual void SetUserData(DWORD_PTR dwUserData) = 0;

	// Summary:
	//	 The interface for the actual stream
	// Return Value:
	//	 True, if the read start finished (with error or success)
	//	 or false if it needs to be restarted again.
	virtual bool StartRead (unsigned nMemQuota = 0x7FFFFFFF) = 0;

	// Summary:
	//	 Decompression of zip-compressed files with default behavior.
	virtual void Decompress() = 0;

	// Description:
	//	 By default, decompression of zip-compressed files should happen in the separate thread.
	virtual void DecompressTo(void* pDestination, size_t nBufferSize) = 0;

	// Description:
	//	 Tries to stop reading the stream; this is advisory and may have no effect
	//	 but the callback will not be called after this. If you just destructing object,
	//	 dereference this object and it will automatically abort and release all associated resources.
	virtual void Abort() = 0;

	// Summary:
	//	 Unconditionally waits until the callback is called.
	// Example:
	//	 If the stream hasn't yet finish, it's guaranteed that the user-supplied callback
	//	 is called before return from this function (unless no callback was specified).
	virtual void Wait() = 0;

	// Description:
	//	 This returns true after the main IO job has been executed (either in worker or in main thread).
	virtual bool IsIOExecuted() = 0;

	// Description:
	//	 This gets called upon the IO has been executed to call the callbacks.
	virtual void FinalizeIO() = 0;

	// Summary:
	//	 Returns stream params.
	virtual const StreamReadParams& GetParams() const = 0;

	// Description:
	//	 Finalizes the read operation, forces callback and erases it (so that it doesn't get called twice).
	virtual void OnFinishRead(unsigned nError) = 0;

	// Description:
	//	 Dumps information about stream to log(necessary for debug purposes).
	virtual string Dump() = 0;

	// Description:
	//	 Returns the size of allocated memory for this object and all sub-objects if any.
	virtual size_t GetSize() = 0;

	// Description:
	//	 Returns the size of the file to read. In case of file absence 
	virtual size_t GetFileSize() = 0;

	// Summary:
	//	 Returns pointer to callback routine(can be NULL).
	virtual IStreamCallback* GetCallback() const = 0;

	// Summary:
	//	 Returns IO error #.
	virtual unsigned GetError() const = 0; 

	// Summary:
	//	 Returns stream name.
	virtual const string& GetName() const = 0;

	// Summary:
	//	 Returns the sync primitive for callback execution.
	virtual CryCriticalSection& GetCallbackLock() = 0;

	// Summary:
	//	 Calls the async callback. 
	virtual void ExecuteAsyncCallback() = 0;

	// Summary:
	//	 Call the async callback.
	virtual void ExecuteSyncCallback() = 0;

protected:
	// Summary:
	//	 The clients are not allowed to destroy this object directly; only via Release().
	virtual ~IReadStream() {}
};

TYPEDEF_AUTOPTR(IReadStream);

// Description:
//   CryPak supports asynchronous reading through this interface. The callback 
//   is called from the main thread in the frame update loop.
//
//   The callback receives packets through StreamOnComplete() and 
//   StreamOnProgress(). The second one can be used to update the asset based 
//   on the partial data that arrived. the callback that will be called by the 
//   streaming engine must be implemented by all clients that want to use 
//   StreamingEngine services
// Remarks:
//   the pStream interface is guaranteed to be locked (have reference count > 0)
//   while inside the function, but can vanish any time outside the function.
//   If you need it, keep it from the beginning (after call to StartRead())
//   some or all callbacks MAY be called from inside IStreamEngine::StartRead()
// 
// Example:
//   <code>
//   IStreamEngine *pStreamEngine = g_pISystem->GetStreamEngine();	 // get streaming engine
//   IStreamCallback *pAsyncCallback = &MyClass;									// user
// 
//   StreamReadParams params;
// 
//   params.dwUserData = 0;
//   params.nSize = 0;
//   params.pBuffer = NULL;
//   params.nLoadTime = 10000;
//   params.nMaxLoadTime = 10000;
//   params.nFlags |= SRP_FLAGS_ASYNC_PROGRESS;
// 
//   pStreamEngine->StartRead(  .. pAsyncCallback .. params .. );			 // registers callback
//   </code>
class IStreamCallback
{
public:
	// Description:
	//	 For some applications, even partially loaded data can be useful.
	//	 To have this callback possibly called, specify the corresponding flag in IStreamEngine::StartRead()
	//	 This callback signals about finish of reading of nSize bytes to the specified buffer pData
	//	 so the client can read that much data inside this callback.
	// Notes:
	//   The default implementation is {} so the clients need not implement it if they don't care
	//   This callback is not guaranteed to be called.
	//   If this callback is called, it doesn't mean the data load will finish successfully;
	//   it still may not finish (then the OnError is called)
	//   nSize is always LESS than the requested data size; when it's equal, StreamOnComplete is called instead.
	virtual void StreamOnProgress (IReadStream* pStream) {}

	// Description:
	//	 Signals that reading the requested data has completed (with or without error).
	//	 This callback is always called, whether an error occurs or not.
	//	 pStream will signal either IsFinished() or IsError() and will hold the (perhaps partially) read data until this interface is released.
	//	 GetBytesRead() will return the size of the file (the completely read buffer) in case of successful operation end
	//	 or the size of partially read data in case of error (0 if nothing was read).
	//	 Pending status is true during this callback, because the callback itself is the part of IO operation.
	//	 nError == 0 : Success
	//	 nError != 0 : Error code
	virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError) {}

	// Description:
	//	 Signals that reading the requested data has completed (with or without error).
	//	 This callback is always called, whether an error occurs or not.
	//	 pStream will signal either IsFinished() or IsError() and will hold the (perhaps partially) read data until this interface is released.
	//	 GetBytesRead() will return the size of the file (the completely read buffer) in case of successful operation end
	//	 or the size of partially read data in case of error (0 if nothing was read).
	//	 Pending status is true during this callback, because the callback itself is the part of IO operation.
	//	 nError == 0 : Success
	//	 nError != 0 : Error code
	virtual void StreamOnComplete (IReadStream* pStream, unsigned nError) = 0;
};



enum StreamingStrategy
{
	// First in-first out, the strategy which ignores priorities.
	SS_FIFO,
	// Random order, ignoring priorities, but the sequence is chosen to read as fast as possible.
	SS_FAST,
	// Inside the same priority class, first in - first out. 
	// The highest priorities get serviced first.
	SS_PRIORITIZED,
	// Attempts to make read smooth, taking priorities into account.
	SS_MULTISTREAM
};


// Description:
//	 This is the approximate parameter block for the streaming engine 
//	 set up.
struct StreamEngineParams
{
	// Summary:
	//	 The strategy to use when queuing clients.
	StreamingStrategy nStrategy;
	// Description:
	//	 The stream capacity to use, at most N bytes per second
	//	 if read requests come faster than that, delay loading.
	unsigned nMaxBytesPerSecond;
	// Description:
	//	 The maximum number of ticks (TICK is yet to be defined - 
	//	 CPU clock, or mcs, or ms, or whatever) to spend inside
	//	 the callbacks per SECOND. If callbacks spend more than that, 
	//	 delay callback execution until the next frame.
	//	 The actual limit is per-frame, the streaming engine uses estimated
	//	 FPS to calculate the per-frame max callback time.
	unsigned nMaxCallbackTicksPerSecond;
	// Summary:
	//	 The maximum allowable simultaneously open streams.
	unsigned nMaxStreams;
};

#ifdef NEED_UNDEF_MAKEFOURCC
	#undef NEED_UNDEF_MAKEFOURCC
	#undef MAKEFOURCC
#endif // NEED_UNDEF_MAKEFOURCC

#endif //_CRY_COMMON_STREAM_ENGINE_HDR_
