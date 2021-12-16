#pragma once

#include "ReplayLogReader.h"
#include "SizeInfo.h"

struct FrameScreenshot
{
	enum
	{
		Width = 160,
		Height = 90,
		Bpp = 3
	};
	u8 bits[Width * Height * Bpp];
};

struct FrameScreenshotIndex
{
	u32 frame;
	u64 allocEv;
};

class FrameUsageTracker : public IReplayListener
{
public:
	struct Label
	{
		Label()
			: frame(0)
			, streamOffset(0)
		{
		}

		Label(const std::string& label, int frame, u64 allocEv, u64 frameOffset)
			: label(label)
			, frame(frame)
			, allocEv(allocEv)
			, streamOffset(streamOffset)
		{
		}

		std::string label;

		int frame;
		u64 allocEv;

		u64 streamOffset;
	};

	typedef std::vector<SizeInfo>::const_iterator ConstIterator;
	typedef std::vector<std::pair<u64, u64> >::const_iterator OffsetSpanIterator;
	typedef std::vector<u64>::const_iterator AllocEvFrameOffsetIterator;
	typedef std::vector<Label>::const_iterator LabelConstIterator;

public:
	FrameUsageTracker();

	void Serialise(ISerialiser& ser);
	bool Deserialise(IDeserialiser& ser);

	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

	ConstIterator FrameInfoBegin(int memGroup) const { return m_frameInfos[memGroup].begin(); }
	ConstIterator FrameInfoEnd(int memGroup) const { return m_frameInfos[memGroup].end(); }

	OffsetSpanIterator FrameOffsetsBegin() const { return m_frameOffsetSpans.begin(); }
	OffsetSpanIterator FrameOffsetsEnd() const { return m_frameOffsetSpans.end(); }

	ConstIterator AllocEvInfoBegin(int memGroup) const { return m_allocEvInfos[memGroup].begin(); }
	ConstIterator AllocEvInfoEnd(int memGroup) const { return m_allocEvInfos[memGroup].end(); }

	OffsetSpanIterator AllocEvOffsetsBegin() const { return m_allocEvOffsetSpans.begin(); }
	OffsetSpanIterator AllocEvOffsetsEnd() const { return m_allocEvOffsetSpans.end(); }

	AllocEvFrameOffsetIterator AllocEvFrameOffsetsBegin() const { return m_allocEvFrames.begin(); }
	AllocEvFrameOffsetIterator AllocEvFrameOffsetsEnd() const { return m_allocEvFrames.end(); }

	LabelConstIterator LabelsBegin() const { return m_labels.begin(); }
	LabelConstIterator LabelsEnd() const { return m_labels.end(); }

	const FrameScreenshot* GetScreenshotForFrame(int frameIdx) const;
	const FrameScreenshot* GetScreenshotForAllocEv(u64 allocEv) const;

	int FindFrameForAllocEv(u64 allocEv) const;
	u64 FindAllocEvForFrame(int frame) const;

	u32 GetAllocEventSplit() const { return m_allocEvSplit; }

private:
	typedef std::map<TAddress, SizeInfo, std::less<const TAddress>, STLPoolAllocator<std::pair<const TAddress, SizeInfo> > > AllocMap;

	enum
	{
		Ser_Version = 7,
		AllocEvSplit = 256,
	};

private:
	void ApplyFree(TAddress ptr, ptrdiff_t sizeGlobal, u64 streamPosition);

private:
	int m_allocEvSplit;

	std::vector<SizeInfo> m_frameInfos[MemGroups::Count];
	std::vector<std::pair<u64, u64> > m_frameOffsetSpans;

	std::vector<SizeInfo> m_allocEvInfos[MemGroups::Count];
	std::vector<std::pair<u64, u64> > m_allocEvOffsetSpans;
	std::vector<u64> m_allocEvFrames;

	std::vector<Label> m_labels;
	std::deque<std::pair<FrameScreenshotIndex, FrameScreenshot> > m_screenshots;
	
	MemAddressProfile m_addressProfile;
	AllocMap m_allocs;

	SizeInfo m_naiveTotal;

	u64 m_currentAllocEvIdx;

	bool m_foundInfo;
};
