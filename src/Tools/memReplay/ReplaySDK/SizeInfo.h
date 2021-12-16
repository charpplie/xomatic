#ifndef SIZEINFO_H
#define SIZEINFO_H

struct MemAddressProfile
{
	MemAddressProfile()
		: rsxStart(0xffffffff)
		, rsxEnd(0xffffffff)
	{}

	MemAddressProfile(TAddress rsxStart, TAddress rsxEnd)
		: rsxStart(rsxStart)
		, rsxEnd(rsxEnd)
	{}


	TAddress rsxStart;
	TAddress rsxEnd;
};

namespace MemGroups
{
	enum Group
	{
		Main = 0,
		RSX,

		Count
	};

	inline MemGroups::Group SelectGroupForAddress(TAddress ptr, const MemAddressProfile& profile = MemAddressProfile())
	{
		return ((ptr >= profile.rsxStart) && (ptr <= profile.rsxEnd)) ? RSX : Main;
	}
}

struct SizeInfo
{
	SizeInfo()
		: allocCount(0)
		, freeCount(0)
		, requested(0)
		, consumed(0)
		, global(0)
	{
	}

	SizeInfo(ptrdiff_t allocCount, ptrdiff_t freeCount, ptrdiff_t requested, ptrdiff_t consumed, ptrdiff_t global)
		: allocCount(allocCount)
		, freeCount(freeCount)
		, requested(requested)
		, consumed(consumed)
		, global(global)
	{
	}

	ptrdiff_t allocCount;
	ptrdiff_t freeCount;
	ptrdiff_t requested;
	ptrdiff_t consumed;
	ptrdiff_t global;

	SizeInfo& operator += (const SizeInfo& b)
	{
		allocCount += b.allocCount;
		freeCount += b.freeCount;
		requested += b.requested;
		consumed += b.consumed;
		global += b.global;
		return *this;
	}

	SizeInfo& operator *= (const int s)
	{
		allocCount *= s;
		freeCount *= s;
		requested *= s;
		consumed *= s;
		global *= s;
		return *this;
	}

	friend SizeInfo operator * (const SizeInfo& a, int b)
	{
		return SizeInfo(a.allocCount * b, a.freeCount * b, a.requested * b, a.consumed * b, a.global * b);
	}

	friend SizeInfo operator + (const SizeInfo& a, const SizeInfo& b)
	{
		return SizeInfo(a.allocCount + b.allocCount, a.allocCount + b.freeCount, a.requested + b.requested, a.consumed + b.consumed, a.global + b.global);
	}
};

class SizeInfoGroups
{
public:
	const SizeInfo& GetTotal() const { return m_total; }
	const SizeInfo& GetGroup(MemGroups::Group group) const { return m_groups[group]; }

	void AddToGroup(MemGroups::Group group, const SizeInfo& sz)
	{
		m_groups[group] += sz;
		m_total += sz;
	}

	SizeInfoGroups& operator += (const SizeInfoGroups& other)
	{
		for (int i = 0; i != MemGroups::Count; ++ i)
			this->m_groups[i] += other.m_groups[i];
		this->m_total += other.m_total;

		return *this;
	}

	SizeInfoGroups& operator *= (const int s)
	{
		for (int i = 0; i != MemGroups::Count; ++ i)
			this->m_groups[i] *= s;
		this->m_total *= s;

		return *this;
	}

	void Serialise(ISerialiser& ser) const;
	void Deserialise(IDeserialiser& ser);

private:
	SizeInfo m_groups[MemGroups::Count];
	SizeInfo m_total;
};

struct ContextStreamOffsetSpan
{
	ContextStreamOffsetSpan()
		: offset(0)
		, threadId(0)
	{}
	ContextStreamOffsetSpan(u64 offset, TThreadId threadId)
		: offset(offset)
		, threadId(threadId)
	{}

	friend bool operator < (const ContextStreamOffsetSpan& a, const ContextStreamOffsetSpan& b)
	{
		return a.offset < b.offset;
	}

	friend bool operator == (const ContextStreamOffsetSpan& a, const ContextStreamOffsetSpan& b)
	{
		return (a.offset == b.offset) && (a.threadId == b.threadId);
	}

	friend bool operator != (const ContextStreamOffsetSpan& a, const ContextStreamOffsetSpan& b)
	{
		return (a.offset != b.offset) || (a.threadId != b.threadId);
	}

	u64 offset;
	TThreadId threadId;
};

typedef std::vector<SizeInfoGroups> SizeInfoGroupVector;

struct SizeInfoUsageResult
{
	SizeInfoGroupVector byFrame;

	int allocEvSplit;
	SizeInfoGroupVector byAllocEv;
};

struct BucketUsageResult
{
	struct UsageInfo
	{
		size_t requested;
		size_t consumed;
		size_t capacity;
		size_t count;

		UsageInfo()
			: requested(0)
			, consumed(0)
			, capacity(0)
			, count(0)
		{
		}

		UsageInfo(size_t requested, size_t consumed, size_t capacity, size_t count)
			: requested(requested)
			, consumed(consumed)
			, capacity(capacity)
			, count(count)
		{
		}
	};

	typedef std::deque<std::vector<UsageInfo> >  UsageStream;
	typedef std::map<int, UsageStream> BucketMap;

	int allocEvSplit;
	BucketMap buckets;
};

struct BucketRange
{
	TAddress base;
	TAddress end;
	s16 bucket;
	u16 alignment;

	BucketRange(TAddress base, TAddress end, s16 bucket, u16 alignment)
		: base(base)
		, end(end)
		, bucket(bucket)
		, alignment(alignment)
	{
	}

	BucketRange()
		: base(0)
		, end(0)
		, bucket(0)
		, alignment(0)
	{
	}

	friend bool operator < (const BucketRange& a, const BucketRange& b)
	{
		return a.base < b.base;
	}
};

struct BucketRangePtrPredicate
{
	bool operator () (const BucketRange& a, TAddress ptr) const
	{
		return a.base < ptr;
	}

	bool operator () (TAddress ptr, const BucketRange& a) const
	{
		return ptr < a.base;
	}

	bool operator () (const BucketRange& a, const BucketRange& b) const
	{
		return a.base < b.base;
	}
};

#endif
