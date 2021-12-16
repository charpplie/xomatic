#ifndef __RECORDINGBUFFER_H__
#define __RECORDINGBUFFER_H__

enum ERecordingBufferPacketType
{
	eRBPT_Invalid = 0,
	eRBPT_FrameData,
	eRBPT_Custom,
};

struct SRecording_Packet
{
	SRecording_Packet()
		: size(0)
		, type(eRBPT_Invalid)
	{}
	uint16 size;
	uint8 type;
};

struct SRecording_FrameData : SRecording_Packet
{
	SRecording_FrameData()
		: frametime(0)
	{
		size = sizeof(SRecording_FrameData);
		type = eRBPT_FrameData;
	}
	float frametime;
};

class CRecordingBuffer
{
public:
	typedef void PacketDiscardCallback(SRecording_Packet *pDiscardedPacket, float recordedTime);

	CRecordingBuffer(size_t size);
	CRecordingBuffer(size_t size, unsigned char *buffer);

	~CRecordingBuffer();

	void Reset();
	//   - clear the buffer memory
	//   - reset the packet lists

	void Update();
	//   - add an entry to the list that stores the list of frame/packet things

	void AddPacket(const SRecording_Packet& packet);
	//   - adds a packet to the buffer

	size_t GetData(uint8 *pBuffer, size_t bufferSize) const;
	
	const uint8* at(size_t offset) const;

	size_t size() const { return m_usedSize; }

	void SetPacketDiscardCallback(PacketDiscardCallback *pdc) {m_pDiscardCallback = pdc;}

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_pBuffer,m_actualBufferSize);		
	}
private:
	uint8* GetEnd()
	{
		uint8* pEnd = m_pStart + m_usedSize;
		if (pEnd >= m_pBuffer + m_dynamicBufferSize)
		{
			pEnd -= m_dynamicBufferSize;
		}
		return pEnd;
	}

	void RemoveFromStart(size_t size);
	void RemoveFrame();
	void RemovePacket(float recordedTime);
	void EnsureFreeSpace(size_t size);

	bool m_allocatedBuffer;

	uint8* m_pBuffer;
	size_t m_dynamicBufferSize;		// The size of the physical buffer minus any wasted space at the end (to avoid splitting packets)
	size_t m_actualBufferSize;		// The actual size of the physical buffer
	
	uint8* m_pStart;
	size_t m_usedSize;

	PacketDiscardCallback *m_pDiscardCallback;
};

#endif // __RECORDINGBUFFER_H__
