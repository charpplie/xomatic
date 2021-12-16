#pragma once

#include "CallstackTable.h"

class MemoryMap
{
public:
	enum
	{
		PageSize = 256 * 1024,
		Alignment = 4,
		AlignmentShift = 2
	};

	typedef TAddress AllocId;

	struct AllocInfo
	{
		union
		{
			struct
			{
				AllocId id : 31;
				unsigned int inUse : 1;
			};
			u32 v;
		};

		AllocInfo()
			: v(0)
		{
		}
	};

	struct AllocEvInfo
	{
		size_t callstackId;

		explicit AllocEvInfo(size_t callstackId)
			: callstackId(callstackId)
		{}
	};

	typedef std::map<TAddress, std::vector<AllocInfo> >::iterator PageIterator;

public:
	MemoryMap()
	{
		Clear();
	}

	bool IsEmpty() const { return m_pages.size() == 0; }

	void Clear()
	{
		m_callstacks = new CallstackTable(NULL);
		m_pages.clear();

		m_allocs.clear();
		m_allocs.push_back(std::vector<AllocEvInfo>());
		m_allocs.back().reserve(16384);
	}

	std::vector<AllocInfo>& GetPage(TAddress pageBase)
	{
		std::map<TAddress, std::vector<AllocInfo> >::iterator it = m_pages.find(pageBase);
		if (it == m_pages.end())
		{
			it = m_pages.insert(std::make_pair(pageBase, std::vector<AllocInfo>())).first;
			it->second.resize(PageSize / Alignment);
		}
		return it->second;
	}

	PageIterator PagesBegin() { return m_pages.begin(); }
	PageIterator PagesEnd() { return m_pages.end(); }

	const AllocEvInfo& GetAllocEvInfo(AllocId id ) const
	{
		assert (id > 0);

		-- id;
		return m_allocs[id >> 14][id & 16383];
	}

	SharedPtr<CallstackTable> GetCallstacks() const { return m_callstacks; }

	size_t GetNumAllocs() const
	{
		return ((m_allocs.size() - 1) * 16384 + m_allocs.back().size());
	}

	void BlitAlloc(TAddress ptr, size_t sz, const TAddress* cs, size_t csLen)
	{
		if (!ptr)
			return;

		TAddress pageBase = ptr & ~(PageSize - 1);
		std::vector<AllocInfo>* page = &GetPage(pageBase);

		AllocId id = GetNumAllocs() + 1;
		size_t callstackId = m_callstacks->AddCallstack(cs, csLen);

		PushBackAlloc(AllocEvInfo(callstackId));

		size_t nBlocks = (sz + (Alignment - 1)) >> AlignmentShift;

		for (TAddress pageEntryMin = (ptr & (PageSize - 1)) >> AlignmentShift; nBlocks; -- nBlocks)
		{
			(*page)[pageEntryMin].id = id;
			(*page)[pageEntryMin].inUse = 1;

			Advance(page, pageBase, pageEntryMin);
		}
	}

	void BlitFree(TAddress ptr)
	{
		TAddress pageBase = ptr & ~(PageSize - 1);
		TAddress pageEntryMin = (ptr & (PageSize - 1)) >> AlignmentShift;

		std::map<TAddress, std::vector<AllocInfo> >::iterator it = m_pages.find(pageBase);
		if (it == m_pages.end())
			return;

		std::vector<AllocInfo>* page = &it->second;

		for (TAddress id = (*page)[pageEntryMin].id; id && ((*page)[pageEntryMin].id == id); )
		{
			(*page)[pageEntryMin].inUse = 0;

			Advance(page, pageBase, pageEntryMin);
		}
	}

	size_t FindLengthToEnd(TAddress ptr)
	{
		TAddress pageBase = ptr & ~(PageSize - 1);
		TAddress pageEntryMin = (ptr & (PageSize - 1)) >> AlignmentShift;

		std::map<TAddress, std::vector<AllocInfo> >::iterator it = m_pages.find(pageBase);
		if (it == m_pages.end())
			return 0;

		std::vector<AllocInfo>* page = &it->second;

		AllocId last = 0;

		TAddress ptrEnd = pageBase | (pageEntryMin << AlignmentShift);
		for (TAddress id = (*page)[pageEntryMin].id; id && ((*page)[pageEntryMin].id == id); ptrEnd += Alignment)
		{
			Advance(page, pageBase, pageEntryMin);
		}

		return ptrEnd - ptr;
	}

	template <typename IteratorT>
	void FindUniqueAllocIdsInRange(IteratorT outIt, TAddress ptr, u32 sz)
	{
		TAddress pageBase = ptr & ~(PageSize - 1);
		TAddress pageEntryMin = (ptr & (PageSize - 1)) >> AlignmentShift;
		sz = (sz + (Alignment - 1)) & ~(Alignment - 1);
		sz >>= AlignmentShift;

		std::map<TAddress, std::vector<AllocInfo> >::iterator it = m_pages.find(pageBase);
		if (it == m_pages.end())
			return;

		std::vector<AllocInfo>* page = &it->second;

		AllocId last = 0;

		for (; sz; -- sz)
		{
			AllocInfo& info = (*page)[pageEntryMin];
			if (info.id != last)
			{
				last = info.id;
				*outIt ++ = last;
			}

			Advance(page, pageBase, pageEntryMin);
		}
	}

private:
	void Advance(std::vector<AllocInfo>*& page, TAddress& pageBase, TAddress& pageEntry)
	{
		++ pageEntry;
		if (pageEntry & (PageSize >> AlignmentShift))
		{
			pageBase += PageSize;
			page = &GetPage(pageBase);
			pageEntry = 0;
		}
	}

	void PushBackAlloc(const AllocEvInfo& ev)
	{
		m_allocs.back().push_back(ev);
		if (m_allocs.back().size() == 16384)
		{
			m_allocs.push_back(std::vector<AllocEvInfo>());
			m_allocs.back().reserve(16384);
		}
	}

private:
	std::map<TAddress, std::vector<AllocInfo> > m_pages;
	std::deque<std::vector<AllocEvInfo> > m_allocs;
	SharedPtr<CallstackTable> m_callstacks;
};
