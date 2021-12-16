#include "stdafx.h"
#include "FrameUsageTracker.h"

namespace
{
	struct GetScreenshotForFrame_Pred
	{
		bool operator () (const std::pair<FrameScreenshotIndex, FrameScreenshot>& a, const std::pair<FrameScreenshotIndex, FrameScreenshot>& b) const
		{
			return a.first.frame < b.first.frame;
		}
	};
	struct GetScreenshotForAllocEv_Pred
	{
		bool operator () (const std::pair<FrameScreenshotIndex, FrameScreenshot>& a, const std::pair<FrameScreenshotIndex, FrameScreenshot>& b) const
		{
			return a.first.allocEv < b.first.allocEv;
		}
	};
}

FrameUsageTracker::FrameUsageTracker()
	: m_foundInfo(false)
	, m_currentAllocEvIdx(0)
	, m_allocEvSplit(AllocEvSplit)
{
}

void FrameUsageTracker::Serialise(ISerialiser& ser)
{
	ser.Write((int) Ser_Version);
	ser.Write((int) MemGroups::Count);

	ser.Write((int) m_allocEvSplit);

	ser.Write(m_frameInfos[MemGroups::Main].size());
	for (int gi = 0; gi != MemGroups::Count; ++ gi)
		ser.Write(m_frameInfos[gi].begin(), m_frameInfos[gi].end());

	ser.Write(m_frameOffsetSpans.size());
	ser.Write(m_frameOffsetSpans.begin(), m_frameOffsetSpans.end());

	ser.Write(m_allocEvInfos[MemGroups::Main].size());
	for (int gi = 0; gi != MemGroups::Count; ++ gi)
		ser.Write(m_allocEvInfos[gi].begin(), m_allocEvInfos[gi].end());

	ser.Write(m_allocEvOffsetSpans.size());
	ser.Write(m_allocEvOffsetSpans.begin(), m_allocEvOffsetSpans.end());

	ser.Write(m_allocEvFrames.size());
	ser.Write(m_allocEvFrames.begin(), m_allocEvFrames.end());

	ser.Write((int) m_labels.size());
	for (std::vector<Label>::const_iterator it = m_labels.begin(), itEnd = m_labels.end(); it != itEnd; ++ it)
	{
		ser.Write((int) it->label.size() + 1);
		ser.Write(it->label.c_str(), it->label.c_str() + it->label.size() + 1);

		ser.Write(it->frame);
		ser.Write(it->allocEv);

		ser.Write(it->streamOffset);
	}

	ser.Write((int) m_screenshots.size());
	for (std::deque<std::pair<FrameScreenshotIndex, FrameScreenshot> >::const_iterator it = m_screenshots.begin(), itEnd = m_screenshots.end();
		it != itEnd;
		++ it)
	{
		ser.Write(it->first.frame);
		ser.Write(it->first.allocEv);
		ser.Write(it->second.bits, it->second.bits + FrameScreenshot::Width * FrameScreenshot::Height * FrameScreenshot::Bpp);
	}
}

bool FrameUsageTracker::Deserialise(IDeserialiser& ser)
{
	if (ser.Read<int>() != Ser_Version)
		return false;
	if (ser.Read<int>() != MemGroups::Count)
		return false;

	m_allocEvSplit = ser.Read<int>();

	int count = ser.Read<int>();
	for (int group = 0; group != MemGroups::Count; ++ group)
	{
		m_frameInfos[group].clear();
		m_frameInfos[group].reserve(count);
		ser.Read(std::back_inserter(m_frameInfos[group]), count);
	}

	count = ser.Read<int>();
	m_frameOffsetSpans.clear();
	m_frameOffsetSpans.reserve(count);
	ser.Read(std::back_inserter(m_frameOffsetSpans), count);

	count = ser.Read<int>();
	for (int group = 0; group != MemGroups::Count; ++ group)
	{
		m_allocEvInfos[group].clear();
		m_allocEvInfos[group].reserve(count);
		ser.Read(std::back_inserter(m_allocEvInfos[group]), count);
	}

	count = ser.Read<int>();
	m_allocEvOffsetSpans.clear();
	m_allocEvOffsetSpans.reserve(count);
	ser.Read(std::back_inserter(m_allocEvOffsetSpans), count);

	count = ser.Read<int>();
	m_allocEvFrames.clear();
	m_allocEvFrames.reserve(count);
	ser.Read(std::back_inserter(m_allocEvFrames), count);

	std::vector<char> labelTemp(256);

	int labelCount = ser.Read<int>();
	m_labels.reserve(labelCount);

	for (size_t labelIx = 0; labelIx != labelCount; ++ labelIx)
	{
		int strLen = ser.Read<int>();

		if (static_cast<int>(labelTemp.size()) < strLen)
			labelTemp.resize(strLen);
		ser.Read(labelTemp.begin(), strLen);

		int frame = ser.Read<int>();
		u64 allocEv = ser.Read<u64>();
		u64 streamOffset = ser.Read<u64>();

		m_labels.push_back(Label(std::string(&labelTemp[0]), frame, allocEv, streamOffset));
	}

	size_t ssCount = (size_t) ser.Read<int>();

	for (size_t i = 0; i != ssCount; ++ i)
	{
		FrameScreenshotIndex idx;
		FrameScreenshot ss;
		idx.frame = ser.Read<int>();
		idx.allocEv = ser.Read<u64>();
		ser.Read(ss.bits, FrameScreenshot::Width * FrameScreenshot::Height * FrameScreenshot::Bpp);
		m_screenshots.push_back(std::make_pair(idx, ss));
	}

	return true;
}

void FrameUsageTracker::ReplayBegin()
{
	using std::swap;

	AllocMap tmpAM;
	swap(tmpAM, m_allocs);

	for (int group = 0; group != MemGroups::Count; ++ group)
	{
		m_frameInfos[group].clear();
		m_frameInfos[group].push_back(SizeInfo());
		m_frameInfos[group].push_back(SizeInfo());
	}

	m_frameOffsetSpans.clear();
	m_frameOffsetSpans.push_back(std::pair<u64, u64>(0, 0));

	for (int group = 0; group != MemGroups::Count; ++ group)
	{
		m_allocEvInfos[group].clear();
		m_allocEvInfos[group].push_back(SizeInfo());
		m_allocEvInfos[group].push_back(SizeInfo());
	}

	m_allocEvOffsetSpans.clear();
	m_allocEvOffsetSpans.push_back(std::pair<u64, u64>(0, 0));

	{
		std::vector<Label> tmpLabels;
		swap(tmpLabels, m_labels);
	}

	m_naiveTotal = SizeInfo();

	m_currentAllocEvIdx = 0;
}

void FrameUsageTracker::Replay(ReplayRange range)
{
	using namespace ReplayEventIds;

	Ids id;
	while (range.ReadNext(id))
	{
		switch (id)
		{
		case RE_AddressProfile:
			{
				const ReplayAddressProfileEvent& ev = range.Get<ReplayAddressProfileEvent>();
				m_addressProfile = MemAddressProfile(ev.rsxStart, 0xffffffff);
			}
			break;

		case RE_AddressProfile2:
			{
				const ReplayAddressProfile2Event& ev = range.Get<ReplayAddressProfile2Event>();
				m_addressProfile = MemAddressProfile(ev.rsxStart, ev.rsxStart + ev.rsxLength);
			}
			break;

		case RE_FrameStart:
			{
				m_frameOffsetSpans.back().second = range.GetPosition();
				m_frameOffsetSpans.push_back(std::pair<u64, u64>(range.GetPosition(), range.GetPosition()));

				m_allocEvFrames.push_back(m_currentAllocEvIdx);

				for (int group = 0; group != MemGroups::Count; ++ group)
					m_frameInfos[group].push_back(m_frameInfos[group].back());
			}
			break;

		case RE_Screenshot:
			{
				const ReplayScreenshotEvent& ev = range.Get<ReplayScreenshotEvent>();

				FrameScreenshotIndex idx;
				idx.frame = m_frameInfos->size() - 1;
				idx.allocEv = m_currentAllocEvIdx;

				FrameScreenshot ss;
				memcpy(ss.bits, ev.bmp, ss.Width * ss.Height * ss.Bpp);
				m_screenshots.push_back(std::make_pair(idx, ss));
			}
			break;

		case RE_Label:
			{
				const ReplayLabelEvent& ev = range.Get<ReplayLabelEvent>();

				m_labels.push_back(
					Label(
					std::string(ev.label), 
					m_frameInfos[0].size(),
					m_allocEvInfos[0].size(),
					range.GetPosition()));
			}
			break;

		case RE_Alloc3:
			{
				const ReplayAlloc3Event& ev = range.Get<ReplayAlloc3Event>();

				SizeInfo sz(1, 0, ev.sizeRequested, ev.sizeConsumed, 0);

				int group = MemGroups::SelectGroupForAddress(ev.ptr, m_addressProfile);

				if (!m_allocs.insert(std::make_pair(ev.ptr, sz)).second)
					sz = SizeInfo(0, 0, 0, 0, 0);

				m_frameInfos[group].back() += sz;
				m_allocEvInfos[group].back() += sz;

				m_frameInfos[MemGroups::Main].back() += SizeInfo(0, 0, 0, 0, ev.sizeGlobal);
				m_allocEvInfos[MemGroups::Main].back() += SizeInfo(0, 0, 0, 0, ev.sizeGlobal);

				m_naiveTotal += sz;

				++ m_currentAllocEvIdx;
				if ((m_currentAllocEvIdx & (m_allocEvSplit - 1)) == 0)
				{
					m_allocEvOffsetSpans.back().second = range.GetPosition();
					m_allocEvOffsetSpans.push_back(std::pair<u64, u64>(range.GetPosition(), range.GetPosition()));

					for (int gi = 0; gi != MemGroups::Count; ++ gi)
						m_allocEvInfos[gi].push_back(m_allocEvInfos[gi].back());
				}
			}
			break;

		case RE_Free3:
			{
				const ReplayFree3Event& ev = range.Get<ReplayFree3Event>();
				ApplyFree(ev.ptr, ev.sizeGlobal, range.GetPosition());
			}
			break;

		case RE_Free4:
			{
				const ReplayFree4Event& ev = range.Get<ReplayFree4Event>();
				ApplyFree(ev.ptr, ev.sizeGlobal, range.GetPosition());
			}
			break;

		case RE_Info:
			{
				const ReplayInfoEvent& ev = range.Get<ReplayInfoEvent>();

				if (!m_foundInfo)
				{
					SizeInfo sz(0, 0, ev.preTrackSize, ev.preTrackSize, ev.preTrackSize);

					for (std::vector<SizeInfo>::iterator it = m_frameInfos[MemGroups::Main].begin(), itEnd = m_frameInfos[MemGroups::Main].end();
						it != itEnd;
						++ it)
					{
						(*it) += sz;
					}

					for (std::vector<SizeInfo>::iterator it = m_allocEvInfos[MemGroups::Main].begin(), itEnd = m_allocEvInfos[MemGroups::Main].end();
						it != itEnd;
						++ it)
					{
						(*it) += sz;
					}

					m_naiveTotal += sz;

					m_foundInfo = true;
				}
			}
			break;
		
		case RE_Info2:
			{
				const ReplayInfo2Event& ev = range.Get<ReplayInfo2Event>();

				if (!m_foundInfo)
				{
					SizeInfo sz(0, 0, ev.preTrackSize - ev.bucketsFree, ev.preTrackSize - ev.bucketsFree, ev.preTrackSize);

					for (std::vector<SizeInfo>::iterator it = m_frameInfos[MemGroups::Main].begin(), itEnd = m_frameInfos[MemGroups::Main].end();
						it != itEnd;
						++ it)
					{
						(*it) += sz;
					}

					for (std::vector<SizeInfo>::iterator it = m_allocEvInfos[MemGroups::Main].begin(), itEnd = m_allocEvInfos[MemGroups::Main].end();
						it != itEnd;
						++ it)
					{
						(*it) += sz;
					}

					m_naiveTotal += sz;

					m_foundInfo = true;
				}
			}
			break;
		}
	}
}

void FrameUsageTracker::ReplayEnd(u64 position)
{
	using std::swap;

	m_frameOffsetSpans.back().second = position;
	m_allocEvOffsetSpans.back().second = position;

	m_frameOffsetSpans.push_back(std::make_pair(position, position));
	m_allocEvOffsetSpans.push_back(std::make_pair(position, position));
	
	AllocMap tmpAllocs;
	swap(m_allocs, tmpAllocs);
}

const FrameScreenshot* FrameUsageTracker::GetScreenshotForFrame(int frameIdx) const
{
	FrameScreenshotIndex idx;
	idx.frame = (size_t) frameIdx;

	FrameScreenshot ss;

	std::deque<std::pair<FrameScreenshotIndex, FrameScreenshot> >::const_iterator it =
		std::lower_bound(m_screenshots.begin(), m_screenshots.end(), std::make_pair(idx, ss), GetScreenshotForFrame_Pred());
	if (it == m_screenshots.end())
		return NULL;
	return &it->second;
}

const FrameScreenshot* FrameUsageTracker::GetScreenshotForAllocEv(u64 allocEv) const
{
	FrameScreenshotIndex idx;
	idx.allocEv = allocEv;

	FrameScreenshot ss;

	std::deque<std::pair<FrameScreenshotIndex, FrameScreenshot> >::const_iterator it =
		std::lower_bound(m_screenshots.begin(), m_screenshots.end(), std::make_pair(idx, ss), GetScreenshotForAllocEv_Pred());
	if (it == m_screenshots.end())
		return NULL;
	return &it->second;
}

int FrameUsageTracker::FindFrameForAllocEv(u64 allocEv) const
{
	std::vector<u64>::const_iterator it = std::lower_bound(m_allocEvFrames.begin(), m_allocEvFrames.end(), allocEv);
	if (it != m_allocEvFrames.end())
	{
		if (*it != allocEv)
		{
			-- it;
		}

		return it - m_allocEvFrames.begin();
	}

	return 0;
}

u64 FrameUsageTracker::FindAllocEvForFrame(int frame) const
{
	if (m_allocEvFrames.empty())
		return 0;

	frame = Clamp(0, (int) m_allocEvFrames.size() - 1, frame);
	return m_allocEvFrames[frame];
}

void FrameUsageTracker::ApplyFree(TAddress ptr, ptrdiff_t sizeGlobal, u64 streamPosition)
{
	int group = MemGroups::SelectGroupForAddress(ptr, m_addressProfile);

	AllocMap::iterator it = m_allocs.find(ptr);

	if (it != m_allocs.end())
	{
		SizeInfo sz(0, 1, -it->second.requested, -it->second.consumed, 0);
		m_frameInfos[group].back() += sz;
		m_allocEvInfos[group].back() += sz;

		m_naiveTotal += sz;

		m_allocs.erase(it);
	}

	m_frameInfos[MemGroups::Main].back() += SizeInfo(0, 0, 0, 0, sizeGlobal);
	m_allocEvInfos[MemGroups::Main].back() += SizeInfo(0, 0, 0, 0, sizeGlobal);

	++ m_currentAllocEvIdx;
	if ((m_currentAllocEvIdx & (m_allocEvSplit - 1)) == 0)
	{
		m_allocEvOffsetSpans.back().second = streamPosition;
		m_allocEvOffsetSpans.push_back(std::pair<u64, u64>(streamPosition, streamPosition));

		for (int gi = 0; gi != MemGroups::Count; ++ gi)
			m_allocEvInfos[gi].push_back(m_allocEvInfos[gi].back());
	}
}
