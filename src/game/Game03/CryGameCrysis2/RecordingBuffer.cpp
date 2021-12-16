#include "StdAfx.h"
#include "RecordingBuffer.h"

CRecordingBuffer::CRecordingBuffer(size_t size)
{
	m_pBuffer = (uint8*) new char[size];
	m_dynamicBufferSize = size;
	m_actualBufferSize = size;

	Reset();

	m_allocatedBuffer = true;
	m_pDiscardCallback = NULL;
}

CRecordingBuffer::CRecordingBuffer(size_t size, unsigned char *buffer)
{
	m_pBuffer = (uint8*)buffer;
	m_dynamicBufferSize = size;
	m_actualBufferSize = size;

	Reset();

	m_allocatedBuffer = false;
	m_pDiscardCallback = NULL;
}

CRecordingBuffer::~CRecordingBuffer()
{
	if (m_pBuffer && m_allocatedBuffer)
	{
		delete [] (char*) m_pBuffer;
	}
	m_pBuffer = NULL;
}

void CRecordingBuffer::Reset()
{
#ifndef _RELEASE
	memset(m_pBuffer, 0xFE, m_actualBufferSize);
#endif

	m_pStart = m_pBuffer;
	m_usedSize = 0;
	m_dynamicBufferSize = m_actualBufferSize;
}

void CRecordingBuffer::Update()
{
	SRecording_FrameData packet;
	packet.frametime = gEnv->pTimer->GetAsyncTime().GetSeconds();
	AddPacket(packet);
}

void CRecordingBuffer::RemoveFrame()
{
	CRY_ASSERT_MESSAGE(m_usedSize > 0, "Unable to remove frame, no packets found");
	SRecording_Packet* pPacket = (SRecording_Packet*)m_pStart;
	if (pPacket->type == eRBPT_FrameData)
	{
		SRecording_FrameData* pFrameData = (SRecording_FrameData*)pPacket;
		float recordedTime = pFrameData->frametime;
		// If there is frame data then remove packets until we reach the next frame data packet
		do 
		{
			RemovePacket(recordedTime);
			pPacket = (SRecording_Packet*)m_pStart;
		} while (m_usedSize > 0 && pPacket->type != eRBPT_FrameData);
	}
	else
	{
		// If there is no frame data then just remove a single packet
		RemovePacket(0);
	}
}

void CRecordingBuffer::RemovePacket(float recordedTime)
{
	CRY_ASSERT_MESSAGE(m_usedSize > 0, "Unable to remove packet, no packets found");
	SRecording_Packet* pPacket = (SRecording_Packet*)m_pStart;
	CRY_ASSERT_MESSAGE(pPacket->size <= m_usedSize, "Unable to remove more than we are using");
	if (m_pDiscardCallback)
	{
		m_pDiscardCallback(pPacket, recordedTime);
	}
	m_pStart += pPacket->size;
	if (m_pStart >= m_pBuffer + m_dynamicBufferSize)
	{
		CRY_ASSERT_MESSAGE(m_pStart == m_pBuffer + m_dynamicBufferSize, "The packet size has overrun the size of the dynamic buffer");
		m_pStart = m_pBuffer;
		m_dynamicBufferSize = m_actualBufferSize;
	}
	m_usedSize -= pPacket->size;
	if (m_usedSize == 0)
	{
		Reset();
	}
}

void CRecordingBuffer::EnsureFreeSpace(size_t size)
{
	CRY_ASSERT_MESSAGE(size <= m_actualBufferSize, "Unable to clear enough space");
	while (true)
	{
		uint8* pEnd = GetEnd();
		size_t freeSpaceLeft = 0;
		if (m_pStart > pEnd)
		{
			freeSpaceLeft = m_pStart - pEnd;
		}
		else if (m_pStart == pEnd)
		{
			if (m_usedSize == 0)
			{
				// We have cleared the entire buffer by this point
				break;
			}
			else
			{
				// No free space at all, the buffer is entirely full
			}
		}
		else
		{
			// We need a continuous block of memory, so take the greatest of either
			// the end bit or the start bit
			size_t endSpace = m_pBuffer + m_actualBufferSize - pEnd;
			size_t startSpace = m_pStart - m_pBuffer;
			freeSpaceLeft = max(endSpace, startSpace);
		}
		if (freeSpaceLeft >= size)
		{
			// That's enough space, don't need to clear any more
			break;
		}
		RemoveFrame();
	};
}

void CRecordingBuffer::AddPacket(const SRecording_Packet& packet)
{
	CRY_ASSERT_MESSAGE(packet.size != 0, "The size of the packet must not be 0.");
	CRY_ASSERT_MESSAGE(packet.type != eRBPT_Invalid, "The type of the packet must not be eRBT_Invalid. Has it been initialised properly?");

	EnsureFreeSpace(packet.size);

	uint8* pEnd = GetEnd();
	size_t distToEnd = (m_pBuffer + m_dynamicBufferSize) - pEnd;
	if (distToEnd >= packet.size)
	{
		// We have enough space at the end of the buffer so add it to the end
		memcpy(pEnd, &packet, packet.size);
	}
	else
	{
		// There is no space left at the end, so wrap round to the start again
		memcpy(m_pBuffer, &packet, packet.size);
		// The dynamic buffer size is reduced to account for the fact that we are not using the last few bytes of the buffer
		m_dynamicBufferSize = m_actualBufferSize - distToEnd;
	}

	m_usedSize += packet.size;

	CRY_ASSERT_MESSAGE(m_usedSize <= m_dynamicBufferSize, "Can't use more memory than we have in the buffer, something has gone wrong here");
}

size_t CRecordingBuffer::GetData(uint8 *pBuffer, size_t bufferSize) const
{
	size_t copiedSize = min(bufferSize, m_usedSize);
	if (m_pStart + copiedSize <= m_pBuffer + m_dynamicBufferSize)
	{
		// All the data is in one continuous block
		memcpy(pBuffer, m_pStart, copiedSize);
	}
	else
	{
		// The data is split into two blocks
		size_t firstPartSize = (m_pBuffer + m_dynamicBufferSize) - m_pStart;
		size_t secondPartSize = copiedSize - firstPartSize;
		memcpy(pBuffer, m_pStart, firstPartSize);
		memcpy(pBuffer + firstPartSize, m_pBuffer, secondPartSize);
	}
	return copiedSize;
}

const uint8* CRecordingBuffer::at(size_t offset) const
{
	CRY_ASSERT_MESSAGE(offset < m_usedSize, "Start offset is too large");
	uint8* pStart = m_pStart + offset;
	if (pStart >= m_pBuffer + m_dynamicBufferSize)
	{
		pStart -= m_dynamicBufferSize;
	}
	return pStart;
}
