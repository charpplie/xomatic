#ifndef REPLAYLOGREADER_H
#define REPLAYLOGREADER_H

#include "ReplayLogDefs.h"
#include "zlib/zlib.h"

class ReplayRange
{
public:
	ReplayRange(u64 bufferPhysicalOffset, const u8* buffer, size_t eventCount)
		: m_bufferPhysicalOffset(bufferPhysicalOffset)
		, m_buffer(buffer)
		, m_current(NULL)
		, m_eventCount(eventCount)
	{
	}

	bool ReadNext(ReplayEventIds::Ids& idOut);

	template <typename T> const T& Get() const
	{
		const ReplayEventHeader* header = reinterpret_cast<const ReplayEventHeader*>(m_current);

		assert(T::EventId == header->eventId);

		return *reinterpret_cast<const T*>(reinterpret_cast<const ReplayEventHeader*>(m_current) + 1);
	}

	u64 GetPosition() const
	{
		return m_bufferPhysicalOffset + (m_current - m_buffer);
	}

private:
	u64 m_bufferPhysicalOffset;
	const u8* m_buffer;
	const u8* m_current;
	size_t m_eventCount;
};

class IReplayListener
{
public:
	virtual ~IReplayListener() {}

	virtual void ReplayBegin() = 0;
	virtual void Replay(ReplayRange range) = 0;
	virtual void ReplayEnd(u64 position) = 0;
};

class CompositeReplayListener : public IReplayListener
{
public:
	void AddListener(IReplayListener& listener)
	{
		m_listeners.push_back(&listener);
	}

	size_t ListenerCount() const { return m_listeners.size(); }

	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

private:
	std::vector<IReplayListener*> m_listeners;
};

class IReplayLogStream
{
public:
	virtual ~IReplayLogStream() {}

	virtual u64 GetPosition() const = 0;
	virtual u64 GetLength() const = 0;

	virtual void Rewind() = 0;
	virtual void StartRead(u8* data, size_t dataCapacity) = 0;
	virtual size_t CompleteRead() = 0;
};

class ReplayRawLogStream : public IReplayLogStream
{
public:
	ReplayRawLogStream(const char* filename);
	~ReplayRawLogStream();

	u64 GetPosition() const { return m_streamPosition; }
	u64 GetLength() const { return m_streamLength; }

	void Rewind();
	void StartRead(u8* data, size_t dataCapacity);
	size_t CompleteRead();

private:
	ReplayRawLogStream(const ReplayRawLogStream&);
	ReplayRawLogStream& operator = (const ReplayRawLogStream&);

private:
	HANDLE m_fp;
	OVERLAPPED m_pendingRequest;
	HANDLE m_overlappedEvent;

	u64 m_streamPosition;
	u64 m_streamLength;
};

class ReplayZLibLogStream : public IReplayLogStream
{
public:
	ReplayZLibLogStream(const char* filename);
	~ReplayZLibLogStream();

	u64 GetPosition() const;
	u64 GetLength() const;

	void Rewind();
	void StartRead(u8* data, size_t dataCapacity);
	size_t CompleteRead();

	struct Header
	{
		u64 uncompressedLen;

		friend void SwapEndian(Header& hdr)
		{
			SwapEndian(hdr.uncompressedLen);
		}
	};

	enum
	{
		StreamBufSize = 1024 * 1024
	};

private:
	enum Msg
	{
		Msg_None,
		Msg_Rewind,
		Msg_StartDecompress,
		Msg_Quit
	};

private:
	static unsigned int __stdcall DecompressThreadProxy(void* self);

private:
	ReplayZLibLogStream(const ReplayZLibLogStream&);
	ReplayZLibLogStream& operator = (const ReplayZLibLogStream&);

	int DecompressThread();
	void Thread_Rewind();
	void Thread_Decompress();

private:
	u64 m_compressedLength;
	u64 m_uncompressedLength;

	HANDLE m_fp;
	u64 m_position;

	uintptr_t m_decompressThread;

	HANDLE m_msgEvent;
	HANDLE m_resultEvent;
	Msg m_msg;
	u8* m_data;
	size_t m_dataCapacity;

	std::vector<u8> m_compressedLog;

	z_stream m_zStr;

	size_t m_lastRead;
};

class ReplayLogReader
{
public:
	static SharedPtr<ReplayLogReader> FromFile(const char* filename);

public:
	~ReplayLogReader();

	u64 GetStreamPosition() const;

	u64 GetPhysicalPosition() const { return m_stream->GetPosition(); }
	u64 GetPhysicalLength() const { return m_stream->GetLength(); }

	bool ReadNext(ReplayEventIds::Ids& idOut);

	template <typename T> const T& Get() const
	{
		ReplayEventHeader* header = reinterpret_cast<ReplayEventHeader*>(m_current);

		assert(T::EventId == header->eventId);

		T* block = reinterpret_cast<T*>(header + 1);

		if (!m_nextEndianFixed)
		{
			FixEndian(*block);
			m_nextEndianFixed = true;
		}

		return *block;
	}

	void Rewind();

	void Replay(IReplayListener& listener);

	u64 GetChunkSize() const { return BufferSize; }

private:
	enum { BufferSize = 16 * 1024 * 1024, Alignment = 4096, OverlappedBorder = 1 * 1024 * 1024 };

private:
	explicit ReplayLogReader(const SharedPtr<IReplayLogStream>& stream);

	ReplayLogReader(const ReplayLogReader&);
	ReplayLogReader& operator = (const ReplayLogReader&);

private:
	bool EnsureAvailable(size_t availLen)
	{
		if ((m_current + availLen) <= (m_primaryBuffer + m_bufferRemaining))
			return true;

		ReadBlock();
		return ((m_current + availLen) <= (m_primaryBuffer + m_bufferRemaining));
	}

	void ReadBlock();
	size_t EndianCorrectBlock();

	void StartRead();
	size_t CompleteRead();

	template <typename T>
	void FixEndian(T& v) const
	{
		if (m_needsEndianSwap)
			SwapEndian(v);
	}

private:
	SharedPtr<IReplayLogStream> m_stream;

	u64 m_streamPosition;

	mutable u8* m_primaryBuffer;
	mutable u8* m_secondaryBuffer;

	mutable u8* m_buffer;
	size_t m_bufferRemaining;

	u8* m_current;

	size_t m_nextSize;
	bool m_needsEndianSwap;
	mutable bool m_nextEndianFixed;

	u8 m_sequenceCheck;
};


#endif