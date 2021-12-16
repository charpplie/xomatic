#include "stdafx.h"
#include "MemoryGraphDataSources.h"

#include "ReplaySDK/FrameUsageTracker.h"


size_t AllocUsageMemoryGraphMarkerSource::GetMarkerCount() const
{
	return 
		(m_tracker->AllocEvFrameOffsetsEnd() - m_tracker->AllocEvFrameOffsetsBegin()) +
		std::distance(m_tracker->LabelsBegin(), m_tracker->LabelsEnd());
}

IMemoryGraphMarkerSource::MarkerInfo AllocUsageMemoryGraphMarkerSource::GetMarkerInfo(size_t idx) const
{
	size_t numFrameOffsets = std::distance(m_tracker->AllocEvFrameOffsetsBegin(), m_tracker->AllocEvFrameOffsetsEnd());

	if (idx < numFrameOffsets)
	{
		MarkerInfo mi = {0};
		mi.colour = RGB(0xff, 0xff, 0xff);
		mi.name = "";
		mi.offset = *(m_tracker->AllocEvFrameOffsetsBegin() + idx) / static_cast<double>(m_tracker->GetAllocEventSplit());
		return mi;
	}
	else
	{
		const FrameUsageTracker::Label& label = *(m_tracker->LabelsBegin() + (idx - numFrameOffsets));

		MarkerInfo mi = {0};
		mi.colour = RGB(0xff, 0x00, 0x00);
		mi.name = label.label.c_str();
		mi.offset = static_cast<double>(label.allocEv);

		return mi;
	}
}

size_t FrameUsageMemoryGraphMarkerSource::GetMarkerCount() const
{
	return std::distance(m_tracker->LabelsBegin(), m_tracker->LabelsEnd());
}

IMemoryGraphMarkerSource::MarkerInfo FrameUsageMemoryGraphMarkerSource::GetMarkerInfo(size_t idx) const
{
	const FrameUsageTracker::Label& label = *(m_tracker->LabelsBegin() + idx);

	MarkerInfo mi = {0};
	mi.colour = RGB(0xff, 0x00, 0x00);
	mi.name = label.label.c_str();
	mi.offset = static_cast<double>(label.frame);

	return mi;
}

GenericMemoryGraphStreamSource::GenericMemoryGraphStreamSource()
	: m_samplePointCount(0)
{
}

GenericMemoryGraphStreamSource::GenericMemoryGraphStreamSource(u64 samplePointCount)
	: m_samplePointCount(samplePointCount)
{
}

void GenericMemoryGraphStreamSource::AddStream(const StreamInfo& info, std::vector<int>& sizes)
{
	m_streams.push_back(Stream());

	Stream& str = m_streams.back();
	str.info = info;
	str.sizes.swap(sizes);
	str.visible = true;

	UpdateRemap();
}

void GenericMemoryGraphStreamSource::SetVisible(int stream, bool visible)
{
	m_streams[stream].visible = visible;
	UpdateRemap();
}

u64 GenericMemoryGraphStreamSource::GetSamplePointCount() const
{
	return m_samplePointCount;
}

int GenericMemoryGraphStreamSource::GetStreamCount() const
{
	return m_streamRemap.size();
}

IMemoryGraphStreamSource::StreamInfo GenericMemoryGraphStreamSource::GetStreamInfo(int index) const
{
	return m_streams[m_streamRemap[index]].info;
}

void GenericMemoryGraphStreamSource::SampleAt(int* out, s64 t, int mip) const
{
	size_t offset = static_cast<size_t>(t << mip);

	for (size_t idx = 0, count = m_streamRemap.size(); idx != count; ++ idx)
	{
		const Stream& stream = m_streams[m_streamRemap[idx]];
		*out ++ = stream.sizes[Clamp(0U, stream.sizes.size() - 1, offset)];
	}
}

int GenericMemoryGraphStreamSource::SampleAt(int stream, s64 t, int mip) const
{
	size_t offset = static_cast<size_t>(t << mip);

	const Stream& streamDef = m_streams[m_streamRemap[stream]];
	return streamDef.sizes[Clamp(0U, streamDef.sizes.size() - 1, offset)];
}

void GenericMemoryGraphStreamSource::UpdateRemap()
{
	m_streamRemap.clear();

	size_t idx = 0;
	for (std::vector<Stream>::iterator it = m_streams.begin(), itEnd = m_streams.end(); it != itEnd; ++ it, ++ idx)
	{
		if (it->visible)
			m_streamRemap.push_back(idx);
	}
}
