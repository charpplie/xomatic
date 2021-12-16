#include "stdafx.h"
#include "BucketUsageQuery.h"

#include "ReplayVisitor.h"

BucketUsageQuery::BucketUsageQuery()
{
}

void BucketUsageQuery::ReplayBegin()
{
	m_allocEv = 0;

	m_result = new BucketUsageResult();
	m_result->allocEvSplit = AllocEvSplit;

	{
		BucketUsageResult::UsageStream& stream = m_result->buckets[-1];

		stream.push_back(std::vector<BucketUsageResult::UsageInfo>());
		stream.back().reserve(16384);
		stream.back().push_back(BucketUsageResult::UsageInfo());
	}
}

void BucketUsageQuery::Replay(ReplayRange range)
{
	ReplayVisitor<BucketUsageQuery> visitor(*this);
	visitor.Replay(range);
}

void BucketUsageQuery::ReplayEnd(u64 position)
{
}

void BucketUsageQuery::RunImpl(ReplayLogReader& reader)
{
	reader.Replay(*this);
	Complete(m_result);
}

void BucketUsageQuery::ReplayEvent(const ReplayAlloc3Event& ev)
{
	std::vector<BucketRange>::iterator it = std::lower_bound(m_bucketRanges.begin(), m_bucketRanges.end(), ev.ptr, BucketRangePtrPredicate());
	if (it != m_bucketRanges.begin())
	{
		if (it->base != ev.ptr)
			-- it;

		if (ev.ptr >= it->base && (ev.ptr + ev.sizeConsumed) <= it->end)
		{
			if (m_allocs.find(ev.ptr) == m_allocs.end())
			{
				size_t sz = (ev.sizeConsumed + it->alignment - 1) & ~(it->alignment - 1);
				BucketUsageResult::UsageInfo& info = m_result->buckets[it->bucket].back().back();
				info.requested += ev.sizeRequested;
				info.consumed += sz;
				++ info.count;

				m_allocs.insert(std::make_pair(ev.ptr, std::pair<AllocSize, int>(std::pair<u16, u16>(ev.sizeRequested, sz), it->bucket)));
			}
		}
	}

	++ m_allocEv;
	if (!(m_allocEv & AllocEvSplit - 1))
		ExtendBuckets();
}

void BucketUsageQuery::ReplayEvent(const ReplayFree3Event& ev)
{
	AllocMap::iterator it = m_allocs.find(ev.ptr);
	if (it != m_allocs.end())
	{
		BucketUsageResult::UsageInfo& info = m_result->buckets[it->second.second].back().back();
		info.requested -= it->second.first.first;
		info.consumed -= it->second.first.second;
		-- info.count;

		m_allocs.erase(it);
	}

	++ m_allocEv;
	if (!(m_allocEv & (AllocEvSplit - 1)))
		ExtendBuckets();
}

void BucketUsageQuery::ReplayEvent(const ReplayFree4Event& ev)
{
	AllocMap::iterator it = m_allocs.find(ev.ptr);
	if (it != m_allocs.end())
	{
		BucketUsageResult::UsageInfo& info = m_result->buckets[it->second.second].back().back();
		info.requested -= it->second.first.first;
		info.consumed -= it->second.first.second;
		-- info.count;

		m_allocs.erase(it);
	}

	++ m_allocEv;
	if (!(m_allocEv & (AllocEvSplit - 1)))
		ExtendBuckets();
}

void BucketUsageQuery::ReplayEvent(const ReplayBucketMark2Event& ev)
{
	BucketRange item(ev.ptr, ev.ptr + ev.length, ev.index, ev.alignment);
	m_bucketRanges.insert(
		std::lower_bound(m_bucketRanges.begin(), m_bucketRanges.end(), item),
		item);

	BucketUsageResult::BucketMap::iterator it = m_result->buckets.find(ev.index);

	if (it == m_result->buckets.end())
	{
		it = m_result->buckets.insert(std::make_pair(ev.index, BucketUsageResult::UsageStream())).first;
		PadStream(it->second);
	}

	it->second.back().back().capacity += ev.length;
}

void BucketUsageQuery::ExtendBuckets()
{
	for (BucketUsageResult::BucketMap::iterator bucketIt = m_result->buckets.begin(), bucketItEnd = m_result->buckets.end();
		bucketIt != bucketItEnd;
		++ bucketIt)
	{
		BucketUsageResult::UsageStream& stream = bucketIt->second;

		BucketUsageResult::UsageInfo carry = stream.back().back();

		if (stream.back().size() == 16384)
		{
			stream.push_back(std::vector<BucketUsageResult::UsageInfo>());
			stream.back().reserve(16384);
		}
		
		stream.back().push_back(carry);
	}
}

void BucketUsageQuery::PadStream(BucketUsageResult::UsageStream& stream)
{
	u64 sz = m_allocEv / AllocEvSplit;

	size_t blocks = static_cast<size_t>(sz / 16384);
	size_t wrap = static_cast<size_t>(sz % 16384);

	stream.resize(blocks, std::vector<BucketUsageResult::UsageInfo>(16384, BucketUsageResult::UsageInfo()));
	stream.push_back(std::vector<BucketUsageResult::UsageInfo>(wrap + 1, BucketUsageResult::UsageInfo()));
}
