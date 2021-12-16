#include "stdafx.h"
#include "ReplayLogReader.h"

#include <process.h>

bool ReplayRange::ReadNext(ReplayEventIds::Ids& idOut)
{
	if (m_eventCount == 0)
		return false;

	if (m_current)
	{
		const ReplayEventHeader* header = reinterpret_cast<const ReplayEventHeader*>(m_current);
		m_current += sizeof(ReplayEventHeader) + header->eventLength;

		idOut = static_cast<ReplayEventIds::Ids>(reinterpret_cast<const ReplayEventHeader*>(m_current)->eventId);
	}
	else
	{
		m_current = m_buffer;
		idOut = static_cast<ReplayEventIds::Ids>(reinterpret_cast<const ReplayEventHeader*>(m_current)->eventId);
	}

	-- m_eventCount;

	return true;
}

void CompositeReplayListener::ReplayBegin()
{
	for (std::vector<IReplayListener*>::iterator it = m_listeners.begin(), itEnd = m_listeners.end();
		it != itEnd;
		++ it)
	{
		(*it)->ReplayBegin();
	}
}

void CompositeReplayListener::Replay(ReplayRange range)
{
	for (std::vector<IReplayListener*>::iterator it = m_listeners.begin(), itEnd = m_listeners.end();
		it != itEnd;
		++ it)
	{
		(*it)->Replay(range);
	}
}

void CompositeReplayListener::ReplayEnd(u64 position)
{
	for (std::vector<IReplayListener*>::iterator it = m_listeners.begin(), itEnd = m_listeners.end();
		it != itEnd;
		++ it)
	{
		(*it)->ReplayEnd(position);
	}
}

SharedPtr<ReplayLogReader> ReplayLogReader::FromFile(const char* filename)
{
	SharedPtr<IReplayLogStream> stream;

	const char* extension = strrchr(filename, '.');
	if (!extension)
		stream = new ReplayRawLogStream(filename);
	else if (_stricmp(extension, ".mrl") == 0)
		stream = new ReplayRawLogStream(filename);
	else if (_stricmp(extension, ".zmrl") == 0)
		stream = new ReplayZLibLogStream(filename);
	else 
		stream = new ReplayRawLogStream(filename);

	SharedPtr<ReplayLogReader> reader = new ReplayLogReader(stream);

	if (reader->GetPhysicalLength() == 0)
		return SharedPtr<ReplayLogReader>();

	return reader;
}

ReplayLogReader::~ReplayLogReader()
{
	if (m_buffer)
		VirtualFree(m_buffer, 0, MEM_RELEASE);
}

u64 ReplayLogReader::GetStreamPosition() const
{
	return m_streamPosition + static_cast<ptrdiff_t>(m_current - (m_primaryBuffer + OverlappedBorder));
}

bool ReplayLogReader::ReadNext(ReplayEventIds::Ids& idOut)
{
	assert (m_stream->GetLength());

	m_current += m_nextSize;

	if (!EnsureAvailable(sizeof(ReplayEventHeader)))
		return false;

	FixEndian(*reinterpret_cast<ReplayEventHeader*>(m_current));
	ReplayEventHeader header = *reinterpret_cast<ReplayEventHeader*>(m_current);

	assert(header.eventLength != 0);

	m_nextSize = header.eventLength + sizeof(ReplayEventHeader);
	m_nextEndianFixed = false;

	if (!EnsureAvailable(m_nextSize))
		return false;

	idOut = static_cast<ReplayEventIds::Ids>(header.eventId);

	return true;
}

void ReplayLogReader::Rewind()
{
	if (!m_stream->GetLength())
		return;

	u64 position = 0;

	m_streamPosition = position;

	CompleteRead();

	//  Read the first block synchronously.
	m_stream->Rewind();
	StartRead();
	m_bufferRemaining = OverlappedBorder + CompleteRead();

	m_streamPosition = position;
	m_sequenceCheck = 0;

	m_current = m_primaryBuffer + OverlappedBorder;

	//  Now start the second block.
	StartRead();

	m_nextSize = 0;

	const char* hdrTag = reinterpret_cast<const char*>(m_current);
	if ((hdrTag[0] == 'M') && (hdrTag[1] == 'R') && (hdrTag[2] == 'L') && !hdrTag[3])
	{
		// Little endian.
		m_needsEndianSwap = false;

		m_current += sizeof(u32);
	}
	else
	{
		// Big-endian.
		m_needsEndianSwap = true;
	}
}

void ReplayLogReader::Replay(IReplayListener& listener)
{
	Rewind();

	listener.ReplayBegin();

	// Endian correct the primary block.
	size_t count;
	u8* current = m_current;

	for (;;)
	{
		u64 position = GetStreamPosition();

		count = EndianCorrectBlock();
		if (count == 0)
			break;

		listener.Replay(ReplayRange(position, current, count));
		ReadBlock();

		current = m_current;
	}

	listener.ReplayEnd(GetStreamPosition());
}

void ReplayLogReader::ReadBlock()
{
	size_t len = (size_t) (m_primaryBuffer + m_bufferRemaining - m_current);

	assert (len <= OverlappedBorder);

	size_t numRead = CompleteRead();

	memcpy(m_primaryBuffer + OverlappedBorder - len, m_secondaryBuffer + m_bufferRemaining - len, len);
	m_current = &m_primaryBuffer[OverlappedBorder - len];

	m_bufferRemaining = numRead + OverlappedBorder;

	StartRead();
}

size_t ReplayLogReader::EndianCorrectBlock()
{
	size_t eventCount = 0;

	u8* buffer = m_current;

	const u8* const blockBase = m_primaryBuffer + OverlappedBorder;

	for (; ;)
	{
		if ((ptrdiff_t) (buffer - blockBase) + (ptrdiff_t) sizeof(ReplayEventHeader) > (ptrdiff_t) (m_bufferRemaining - OverlappedBorder))
			break;

		ReplayEventHeader* header = reinterpret_cast<ReplayEventHeader*>(buffer);
		FixEndian(*header);

		u8* block = reinterpret_cast<u8*>(header + 1);

		if (static_cast<ptrdiff_t>(block - blockBase) + header->eventLength > (ptrdiff_t) (m_bufferRemaining - OverlappedBorder))
		{
			// Need to undo the endian fix.
			FixEndian(*header);
			break;
		}

		assert ((header->sequenceCheck == m_sequenceCheck) || (header->sequenceCheck == 0));

		using namespace ReplayEventIds;

		switch (header->eventId)
		{
		case RE_Alloc: FixEndian(*reinterpret_cast<ReplayAllocEvent*>(block)); break;
		case RE_Free: FixEndian(*reinterpret_cast<ReplayFreeEvent*>(block)); break;
		case RE_Callstack: FixEndian(*reinterpret_cast<ReplayCallstackEvent*>(block)); break;
		case RE_FrameStart: FixEndian(*reinterpret_cast<ReplayFrameStartEvent*>(block)); break;
		case RE_Label: FixEndian(*reinterpret_cast<ReplayLabelEvent*>(block)); break;
		case RE_XenonModuleRef: FixEndian(*reinterpret_cast<ReplayXenonModuleRefEvent*>(block)); break;
		case RE_AllocVerbose: FixEndian(*reinterpret_cast<ReplayAllocVerboseEvent*>(block)); break;
		case RE_FreeVerbose: FixEndian(*reinterpret_cast<ReplayFreeVerboseEvent*>(block)); break;
		case RE_Info: FixEndian(*reinterpret_cast<ReplayInfoEvent*>(block)); break;
		case RE_PushContext: FixEndian(*reinterpret_cast<ReplayPushContextEvent*>(block)); break;
		case RE_PopContext: FixEndian(*reinterpret_cast<ReplayPopContextEvent*>(block)); break;
		case RE_Alloc3: FixEndian(*reinterpret_cast<ReplayAlloc3Event*>(block)); break;
		case RE_Free3: FixEndian(*reinterpret_cast<ReplayFree3Event*>(block)); break;
		case RE_PushContext2: FixEndian(*reinterpret_cast<ReplayPushContext2Event*>(block)); break;
		case RE_PS3ModuleRef: FixEndian(*reinterpret_cast<ReplayPS3ModuleRefEvent*>(block)); break;
		case RE_AddressProfile: FixEndian(*reinterpret_cast<ReplayAddressProfileEvent*>(block)); break;
		case RE_PushContext3: FixEndian(*reinterpret_cast<ReplayPushContext3Event*>(block)); break;
		case RE_Free4: FixEndian(*reinterpret_cast<ReplayFree4Event*>(block)); break;
		case RE_AllocUsage: FixEndian(*reinterpret_cast<ReplayAllocUsageEvent*>(block)); break;
		case RE_Info2: FixEndian(*reinterpret_cast<ReplayInfo2Event*>(block)); break;
		case RE_Screenshot: FixEndian(*reinterpret_cast<ReplayScreenshotEvent*>(block)); break;
		case RE_SizerPush: FixEndian(*reinterpret_cast<ReplaySizerPushEvent*>(block)); break;
		case RE_SizerPop: FixEndian(*reinterpret_cast<ReplaySizerPopEvent*>(block)); break;
		case RE_SizerAddRange: FixEndian(*reinterpret_cast<ReplaySizerAddRangeEvent*>(block)); break;
		case RE_AddressProfile2: FixEndian(*reinterpret_cast<ReplayAddressProfile2Event*>(block)); break;
		case RE_BucketMark: FixEndian(*reinterpret_cast<ReplayBucketMarkEvent*>(block)); break;
		case RE_BucketMark2: FixEndian(*reinterpret_cast<ReplayBucketMark2Event*>(block)); break;
		default: assert("Unknown event type" && 0);// throw std::runtime_error("Unknown event type."); break;
		}

		buffer += sizeof(ReplayEventHeader) + header->eventLength;
		++ eventCount;
		++ m_sequenceCheck;
	}

	m_current = buffer;

	return eventCount;
}

ReplayLogReader::ReplayLogReader(const SharedPtr<IReplayLogStream>& stream)
	: m_stream(stream)
	, m_bufferRemaining(0)
	, m_nextSize(0)
	, m_needsEndianSwap(true)
	, m_nextEndianFixed(false)
{
	m_buffer = (u8*) VirtualAlloc(NULL, BufferSize * 2 + OverlappedBorder * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	m_primaryBuffer = m_buffer;
	m_secondaryBuffer = m_buffer + BufferSize + OverlappedBorder;

	if (m_stream->GetLength())
	{
		m_stream->Rewind();
		StartRead();

		m_bufferRemaining = OverlappedBorder + CompleteRead();
		StartRead();

		m_streamPosition = 0;

		m_current = m_primaryBuffer + OverlappedBorder;
	}
}

void ReplayLogReader::StartRead()
{
	m_stream->StartRead(m_secondaryBuffer + OverlappedBorder, BufferSize);
}

size_t ReplayLogReader::CompleteRead()
{
	using std::swap;

	size_t len = m_stream->CompleteRead();

	swap(m_primaryBuffer, m_secondaryBuffer);

	m_streamPosition += m_bufferRemaining - OverlappedBorder;
	return len;
}


ReplayRawLogStream::ReplayRawLogStream(const char *filename)
	: m_fp(NULL)
	, m_streamPosition(0)
	, m_streamLength(0)
{
	m_overlappedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	DWORD flags = FILE_ATTRIBUTE_NORMAL;

	flags |= FILE_FLAG_OVERLAPPED;
	m_fp = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, flags, NULL);

	if (m_fp)
	{
		LARGE_INTEGER fileSize;
		GetFileSizeEx(m_fp, &fileSize);

		m_streamLength = (static_cast<u64>(fileSize.HighPart) << 32) | static_cast<u64>(fileSize.LowPart);
	}
}

ReplayRawLogStream::~ReplayRawLogStream()
{
	if (m_fp)
		CloseHandle(m_fp);

	if (m_overlappedEvent)
		CloseHandle(m_overlappedEvent);
}

void ReplayRawLogStream::Rewind()
{
	m_streamPosition = 0;
}

void ReplayRawLogStream::StartRead(u8* data, size_t dataCapacity)
{
	// Start the next block async.
	memset(&m_pendingRequest, 0, sizeof(m_pendingRequest));

	if (m_streamPosition < m_streamLength)
	{
		m_pendingRequest.Offset = m_streamPosition & 0xffffffff;
		m_pendingRequest.OffsetHigh = m_streamPosition >> 32;
		m_pendingRequest.hEvent = m_overlappedEvent;

		DWORD toRead = std::min(static_cast<DWORD>(dataCapacity), static_cast<DWORD>(m_streamLength - m_streamPosition));
		if (!ReadFile(m_fp, data, toRead, NULL, &m_pendingRequest))
		{
			switch (GetLastError())
			{
			case ERROR_IO_PENDING:
				return;
			default:
				memset(&m_pendingRequest, 0, sizeof(m_pendingRequest));
				break;
			}
		}
	}
}

size_t ReplayRawLogStream::CompleteRead()
{
	using std::swap;

	DWORD bytesRead = 0;
	if (m_pendingRequest.hEvent != NULL)
	{
		GetOverlappedResult(m_fp, &m_pendingRequest, &bytesRead, TRUE);

		m_streamPosition += bytesRead;
	}

	return bytesRead;
}

ReplayZLibLogStream::ReplayZLibLogStream(const char* filename)
	: m_lastRead(0)
	, m_decompressThread(0)
	, m_position(0)
{
	memset(&m_zStr, 0, sizeof(m_zStr));

	m_fp = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (m_fp != INVALID_HANDLE_VALUE)
	{
		m_position = 0;

		LARGE_INTEGER fileSize;
		GetFileSizeEx(m_fp, &fileSize);

		m_compressedLength = (static_cast<u64>(fileSize.HighPart) << 32) | static_cast<u64>(fileSize.LowPart);

		SetFilePointer(m_fp, 0, 0, FILE_BEGIN);

		DWORD num;
		Header hdr;
		ReadFile(m_fp, &hdr, sizeof(Header), &num, NULL);
		m_position += num;

		SwapEndian(hdr);

		if (hdr.uncompressedLen)
		{
			m_uncompressedLength = hdr.uncompressedLen;
		}
		else
		{
			m_uncompressedLength = 1;
		}

		m_compressedLog.resize(StreamBufSize);

		ReadFile(m_fp, &m_compressedLog[0], sizeof(u8) * StreamBufSize, &num, NULL);
		m_position += num;

		m_zStr.avail_in = num;
		m_zStr.next_in = &m_compressedLog[0];

		inflateInit(&m_zStr);

		m_decompressThread = _beginthreadex(NULL, 0, DecompressThreadProxy, this, 0, NULL);
		m_msgEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_resultEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	}
}

ReplayZLibLogStream::~ReplayZLibLogStream()
{
	if (m_decompressThread)
	{
		WaitForSingleObject(m_resultEvent, INFINITE);

		m_msg = Msg_Quit;
		ResetEvent(m_resultEvent);
		SetEvent(m_msgEvent);

		WaitForSingleObject(m_resultEvent, INFINITE);

		CloseHandle(m_msgEvent);
		CloseHandle(m_resultEvent);
	}

	if (m_fp)
	{
		CloseHandle(m_fp);
		inflateEnd(&m_zStr);
	}
}

u64 ReplayZLibLogStream::GetPosition() const
{
	return m_position;
}

u64 ReplayZLibLogStream::GetLength() const
{
	return m_compressedLength;
}

void ReplayZLibLogStream::Rewind()
{
	WaitForSingleObject(m_resultEvent, INFINITE);

	m_msg = Msg_Rewind;
	ResetEvent(m_resultEvent);
	SetEvent(m_msgEvent);
}

void ReplayZLibLogStream::StartRead(u8* data, size_t dataCapacity)
{
	WaitForSingleObject(m_resultEvent, INFINITE);

	m_data = data;
	m_dataCapacity = dataCapacity;
	m_msg = Msg_StartDecompress;

	ResetEvent(m_resultEvent);
	SetEvent(m_msgEvent);
}

size_t ReplayZLibLogStream::CompleteRead()
{
	WaitForSingleObject(m_resultEvent, INFINITE);

	return m_lastRead;
}

unsigned int __stdcall ReplayZLibLogStream::DecompressThreadProxy(void* self)
{
	return reinterpret_cast<ReplayZLibLogStream*>(self)->DecompressThread();
}

int ReplayZLibLogStream::DecompressThread()
{
	do
	{
		SetEvent(m_resultEvent);
		WaitForSingleObject(m_msgEvent, INFINITE);

		switch (m_msg)
		{
		case Msg_Quit:
			SetEvent(m_resultEvent);
			return 0;

		case Msg_Rewind:
			Thread_Rewind();
			break;

		case Msg_StartDecompress:
			Thread_Decompress();
			break;
		}

		m_msg = Msg_None;
	}
	while (true);

	return 0;
}

void ReplayZLibLogStream::Thread_Rewind()
{
	assert (m_fp);

	inflateEnd(&m_zStr);

	SetFilePointer(m_fp, sizeof(Header), 0, FILE_BEGIN);

	DWORD len;
	ReadFile(m_fp, &m_compressedLog[0], sizeof(u8) * StreamBufSize, &len, NULL);
	m_position = len;

	memset(&m_zStr, 0, sizeof(m_zStr));
	m_zStr.avail_in = len;
	m_zStr.next_in = &m_compressedLog[0];

	inflateInit(&m_zStr);
}

void ReplayZLibLogStream::Thread_Decompress()
{
	assert (m_fp);

	m_lastRead = 0;

	m_zStr.next_out = m_data;
	m_zStr.avail_out = m_dataCapacity;
	m_zStr.total_out = 0;

	do 
	{
		int err = inflate(&m_zStr, Z_SYNC_FLUSH);

		if (err == Z_OK)
		{
			if (m_zStr.avail_in == 0)
			{
				DWORD len;
				ReadFile(m_fp, &m_compressedLog[0], sizeof(u8) * StreamBufSize, &len, NULL);
				m_position += len;

				m_zStr.avail_in = len;
				m_zStr.next_in = &m_compressedLog[0];

				continue;
			}
		}

		break;
	} 
	while (true);

	m_lastRead = m_zStr.total_out;
}

