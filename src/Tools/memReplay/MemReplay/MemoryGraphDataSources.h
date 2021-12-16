#pragma once

#include "MemoryGraphWnd.h"

class FrameUsageTracker;

class AllocUsageMemoryGraphMarkerSource : public IMemoryGraphMarkerSource
{
public:
	AllocUsageMemoryGraphMarkerSource()
		: m_tracker(NULL)
	{
	}

	AllocUsageMemoryGraphMarkerSource(const FrameUsageTracker& tracker)
		: m_tracker(&tracker)
	{
	}

	size_t GetMarkerCount() const;
	MarkerInfo GetMarkerInfo(size_t idx) const;

private:
	const FrameUsageTracker* m_tracker;
};

class FrameUsageMemoryGraphMarkerSource : public IMemoryGraphMarkerSource
{
public:
	FrameUsageMemoryGraphMarkerSource()
		: m_tracker(NULL)
	{
	}

	FrameUsageMemoryGraphMarkerSource(const FrameUsageTracker& tracker)
		: m_tracker(&tracker)
	{
	}

	size_t GetMarkerCount() const;
	MarkerInfo GetMarkerInfo(size_t idx) const;

private:
	const FrameUsageTracker* m_tracker;
};

class GenericMemoryGraphStreamSource : public IMemoryGraphStreamSource
{
public:
	GenericMemoryGraphStreamSource();
	explicit GenericMemoryGraphStreamSource(u64 samplePointCount);

	// Takes ownership of vector contents
	void AddStream(const StreamInfo& info, std::vector<int>& sizes);

	bool IsVisible(int stream) const { return m_streams[stream].visible; }
	void SetVisible(int stream, bool visible);

public: // IMemoryGraphStreamSource Members
	u64 GetSamplePointCount() const;
	int GetStreamCount() const;
	StreamInfo GetStreamInfo(int index) const;

	void SampleAt(int* out, s64 t, int mip) const;
	int SampleAt(int stream, s64 t, int mip) const;

private:
	struct Stream
	{
		StreamInfo info;
		bool visible;
		std::vector<int> sizes;
	};

private:
	void UpdateRemap();
	
private:
	u64 m_samplePointCount;

	std::vector<int> m_streamRemap;
	std::vector<Stream> m_streams;
};
