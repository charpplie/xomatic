//////////////////////////////////////////////////////////////////////////
// this is the unique IReadStream* interface implementation,
// that refers to the actual read stream (hence the name proxy)
// The proxy is the per-client unique object that is acquired by the Streaming
// Engine clients via StartRead() API. Several Proxies can refer to the same
// Stream (file) but perform their operations independently.
#ifndef _CRY_SYSTEM_READ_STREAM_PROXY_HDR_
#define _CRY_SYSTEM_READ_STREAM_PROXY_HDR_

#include "IStreamEngine.h"

class CRefReadStreamProxy: public IReadStream
{
public:
	// max number of retries - if the file doesn't start reading after this number of retries, unrecoverable error is returned
	enum {g_numMaxRetries = 4};
	// this is the length of the block that's read at once.
	// big requests are splitted into smaller blocks of this size
	enum {g_nBlockLength = 32 * 1024 * 1024}; // 128 k is the max size of the DMA transfer request

	CRefReadStreamProxy (const StreamTaskType tSource, class CRefReadStream* pStream, IStreamCallback* pCallback, StreamReadParams* pParams);
	virtual ~CRefReadStreamProxy ();

	VIRTUAL DWORD_PTR GetUserData() {return m_Params.dwUserData;}

	// set user defined data into stream's params
	VIRTUAL void SetUserData(DWORD_PTR dwUserData);

	// returns true if the file read was not successful.
	VIRTUAL bool IsError();

	// returns true if the file read was completed (successfully or unsuccessfully)
	// check IsError to check if the whole requested file (piece) was read
	VIRTUAL bool IsFinished();

	// returns the number of bytes read so far (the whole buffer size if IsFinished())
	VIRTUAL unsigned int GetBytesRead (bool bWait);

	// returns the buffer into which the data has been or will be read
	// at least GetBytesRead() bytes in this buffer are guaranteed to be already read
	VIRTUAL const void* GetBuffer ();

	// tries to stop reading the stream; this is advisory and may have no effect
	// but the callback	will not be called after this. If you just destructing object,
	// dereference this object and it will automatically abort and release all associated resources.
	VIRTUAL void Abort();

	// tries to raise the priority of the read; this is advisory and may have no effect
	VIRTUAL void RaisePriority (unsigned int nPriority);

	// unconditionally waits until the callback is called
	// i.e. if the stream hasn't yet finish, it's guaranteed that the user-supplied callback
	// is called before return from this function (unless no callback was specified)
	VIRTUAL void Wait();

	// the interface for the actual stream
	// returns true, if the read start finished (with error or success)
	// or false if it needs to be restarted again
  VIRTUAL bool StartRead (unsigned nMemQuota = 0x7FFFFFFF);

	// decompression of zip-compressed files with default behavior
	VIRTUAL void Decompress();

	// by default, decompression of zip-compressed files should happen in the separate thread
	VIRTUAL void DecompressTo(void* pDestination, size_t nBufferSize);

	VIRTUAL uint64 GetPriority()const;

	// this returns true after the main IO job has been executed (either in worker or in main thread)
	VIRTUAL bool IsIOExecuted();

	// this gets called upon the IO has been executed to call the callbacks
	VIRTUAL void FinalizeIO();

	VIRTUAL const StreamReadParams& GetParams() const {return m_Params;}

	// finalizes the read operation, forces callback and erases it (so that it doesn't get called twice)
	VIRTUAL void OnFinishRead(unsigned nError);

	VIRTUAL string Dump();

	// returns the size of allocated memory for this object and all subobjects if any
	VIRTUAL size_t GetSize();

	//	 Returns the size of the file to read. In case of file absence 
	VIRTUAL size_t GetFileSize();

	// return pointer to callback routine(can be NULL)
	VIRTUAL IStreamCallback* GetCallback() const;

	// return IO error #
	VIRTUAL unsigned GetError() const; 

	// return stream name
	VIRTUAL const string& GetName() const;

	// returns the sync primitive for callback execution
	VIRTUAL CryCriticalSection& GetCallbackLock();

	// call the async callback 
	VIRTUAL void ExecuteAsyncCallback();

	// call the sync callback 
	VIRTUAL void ExecuteSyncCallback();

protected:
	static const string		s_sClassName;
	// the number of times the StartRead was retried; after too many retries unrecoverable error is returned
	unsigned m_numRetries;
	static unsigned g_numPendingOperations;
	virtual void OnIOComplete(unsigned nError, unsigned numBytesRead);
	// on the platforms that support overlapped IO, calls ReadFileEx.
	// on other platforms merely reads the file, calling OnIOComplete()
  DWORD CallReadFileEx ();

	bool IsDecompressed() const;









	static VOID CALLBACK FileIOCompletionRoutine(
		DWORD dwErrorCode,                // completion code
		DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
		LPOVERLAPPED lpOverlapped         // I/O information buffer
		);


	// Stream task priority
	uint64											m_nPriority;

	// the actual stream
	_smart_ptr<CRefReadStream>	m_pStream;
	// the type of the task
	const StreamTaskType				m_Type;
	// the initial data from the user
	StreamReadParams						m_Params;
	// the callback; may be NULL
	IStreamCallback*						m_pCallback;
	bool												m_bIsAsyncCallbackExecuted;
	bool												m_bIsSyncCallbackExecuted;
	// lock primitive for callback execution
	CryCriticalSection m_CallbackLock;
	// the actual buffer to read to
	void* m_pBuffer;
	// the number of bytes read so far
	unsigned m_numBytesRead;

	// the portion of data (offset, length) currently queued for reading.
	// this portion offset is RELATIVELY to the supplied by the client
	// offset within the file; so, the offset within the file is m_nPieceOffset + m_Params.nOffset
	// This is only used during m_bPending is true
	unsigned m_nPieceOffset, m_nPieceLength;


	// is locally decompressed
	bool		m_bIsDecompressed;

	// the structure for asynchronous callback
	OVERLAPPED m_Overlapped;



	void SetOverlappedOffset(DWORD offset) { m_Overlapped.Offset = offset; }


	volatile bool m_bError, m_bFinished;
	bool m_bFreeBuffer, m_bPending;
	unsigned m_nIOError;
};

TYPEDEF_AUTOPTR(CRefReadStreamProxy);

#endif
