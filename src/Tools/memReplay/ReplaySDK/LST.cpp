#include "stdafx.h"
#include "LST.h"

namespace
{
	struct LayerNodeInfo
	{
		u64 evBegin, evEnd;
		s32 szCnt, szReq, szCon, szGlo;
	};
}

AllocSet::AllocSet()
{
}

void AllocSet::Serialise(u8*& buffer, size_t& bufferLen)
{
	MakeImmutable();
	if (m_immutableAllocs.IsValid() && m_immutableAllocs->empty() == false)
	{
		buffer = reinterpret_cast<u8*>(&m_immutableAllocs->front());
		bufferLen = sizeof(ImmutableAllocSet::value_type) * m_immutableAllocs->size();
	}
	else
	{
		buffer = NULL;
		bufferLen = 0;
	}
}

void AllocSet::Deserialise(const u8* buffer, size_t bufferLen)
{
	m_mutableAllocs = NULL;
	m_immutableAllocs = new ImmutableAllocSet(bufferLen / sizeof(ImmutableAllocSet::value_type));

	memcpy(&m_immutableAllocs->front(), buffer, bufferLen);
}

void AllocSet::Compact()
{
	m_mutableAllocs = NULL;
	m_immutableAllocs = NULL;
}

template <typename InputIteratorT>
static void AddSets(AllocSet::MutableAllocSet& out, InputIteratorT rhs, InputIteratorT rhsEnd)
{
	typedef AllocSet::MutableAllocSet::iterator OutIterator;

	// For each alloc in the set - can be in any of the following states
	// * exists in both:
	//   if alloced in lhs, freed in rhs: erase
	//   if alloced in lhs, alloced in rhs: rhs
	//   if freed in lhs, alloced in rhs: rhs
	//   if freed in both: shouldn't really happen
	// * exists in neither (obviously redundant)
	// * exists in lhs but not rhs - take lhs - is a persistant alloc
	// * exists in rhs but not lhs - take rhs - is a new alloc
	OutIterator lhs = out.begin();

	for (; rhs != rhsEnd;)
	{
		if (lhs != out.end())
		{
			if (lhs->first == rhs->first)
			{
				if (!rhs->second.freed)
				{
					lhs->second = rhs->second;
					++ lhs;
				}
				else
				{
					assert (!lhs->second.freed);

					lhs = out.erase(lhs);
				}

				++ rhs;
			}
			else if (lhs->first < rhs->first)
			{
				// taking lhs
				++ lhs;
			}
			else
			{
				// taking rhs
				lhs = out.insert(*rhs).first;
				++ lhs;
				++ rhs;
			}
		}
		else
		{
			// taking rhs
			lhs = out.insert(*rhs).first;
			++ lhs;
			++ rhs;
		}
	}
}

void AllocSet::Add(const AllocSet& other)
{
	EnsureMutable();

	if (other.m_mutableAllocs.IsValid())
	{
		AddSets(*m_mutableAllocs, other.m_mutableAllocs->begin(), other.m_mutableAllocs->end());
	}
	else if (other.m_immutableAllocs.IsValid())
	{
		AddSets(*m_mutableAllocs, other.m_immutableAllocs->begin(), other.m_immutableAllocs->end());
	}
	else
	{
	}
}
	
void AllocSet::AddAlloc(TAddress ptr, const AllocInfo& ai)
{
	EnsureMutable();

	MutableAllocSet::iterator it = m_mutableAllocs->find(ptr);

	if (it != m_mutableAllocs->end())
		it->second = ai;
	else
		m_mutableAllocs->insert(std::make_pair(ptr, ai));
}

void AllocSet::FreeAlloc(TAddress ptr, const AllocInfo& ai)
{
	EnsureMutable();

	MutableAllocSet::iterator it = m_mutableAllocs->find(ptr);

	if (it != m_mutableAllocs->end())
		m_mutableAllocs->erase(it);
	else
	{
		AllocInfo dai(ai);
		dai.freed = 1;

		m_mutableAllocs->insert(std::make_pair(ptr, dai));
	}
}

void AllocSet::MakeMutable()
{
	m_mutableAllocs = new MutableAllocSet();

	if (m_immutableAllocs.IsValid())
	{
		for (ImmutableAllocSet::const_iterator it = m_immutableAllocs->begin(), itEnd = m_immutableAllocs->end();
			it != itEnd;
			++ it)
		{
			m_mutableAllocs->insert(*it);
		}

		m_immutableAllocs = NULL;
	}
}

void AllocSet::MakeImmutable()
{
	if (m_immutableAllocs.IsValid())
		return;
	if (!m_mutableAllocs.IsValid())
		return;

	m_immutableAllocs = new ImmutableAllocSet();
	m_immutableAllocs->reserve(m_mutableAllocs->size());

	std::copy(m_mutableAllocs->begin(), m_mutableAllocs->end(), std::back_inserter(*m_immutableAllocs));

	m_mutableAllocs = NULL;
}


AllocSetLST::AllocSetLST(const TCHAR* filename)
	: m_filename(filename)
	//, m_zip(NULL)
	, m_currentAllocEvIdx(0)
{
}

AllocSetLST::~AllocSetLST()
{
	/*
	if (m_zip)
		CloseZip(m_zip);
		*/
}

bool AllocSetLST::Restore()
{






























































































	return true;
}

void AllocSetLST::ReplayBegin()
{

















}

void AllocSetLST::Replay(ReplayRange range)
{
	using namespace ReplayEventIds;

	Ids id;

	AllocSetHeader* activeSet = &m_tree.front()->back();

	while (range.ReadNext(id))
	{
		switch (id)
		{
		case RE_Alloc3:
			{
				const ReplayAlloc3Event& ev = range.Get<ReplayAlloc3Event>();

				SizeInfo sz(1, 0, ev.sizeRequested, ev.sizeConsumed, ev.sizeGlobal);

				size_t csId = m_callstackTable->AddCallstack(ev);

				AllocInfo ai(ev.threadId, csId, sz.requested, sz.consumed, sz.global);
				activeSet->set.AddAlloc(ev.ptr, ai);

				activeSet->totalSize += sz;

				m_allocs.insert(std::make_pair(ev.ptr, ai));

				++ m_currentAllocEvIdx;
				if ((m_currentAllocEvIdx & (SplitAlignment - 1)) == 0)
				{
					FinaliseTree();
					activeSet = &m_tree.front()->back();
				}
			}
			break;

		case RE_Free3:
			{
				const ReplayFree3Event& ev = range.Get<ReplayFree3Event>();
				ApplyFree(activeSet, ev.ptr, ev.sizeGlobal);
			}
			break;

		case RE_Free4:
			{
				const ReplayFree4Event& ev = range.Get<ReplayFree4Event>();
				ApplyFree(activeSet, ev.ptr, ev.sizeGlobal);
			}
			break;
		}
	}
}

void AllocSetLST::ApplyFree(AllocSetHeader*& activeSet, TAddress ptr, ptrdiff_t sizeGlobal)
{
	AllocIndex::iterator it = m_allocs.find(ptr);

	if (it != m_allocs.end())
	{
		activeSet->set.FreeAlloc(ptr, it->second);

		SizeInfo sz(0, 1, -it->second.sizeRequested, -it->second.sizeConsumed, sizeGlobal);
		activeSet->totalSize += sz;

		m_allocs.erase(it);
	}
	else
	{
		SizeInfo sz(0, 1, 0, 0, sizeGlobal);
		activeSet->totalSize += sz;
	}

	++ m_currentAllocEvIdx;
	if ((m_currentAllocEvIdx & (SplitAlignment - 1)) == 0)
	{
		FinaliseTree();
		activeSet = &m_tree.front()->back();
	}
}

void AllocSetLST::ReplayEnd(u64 position)
{


























































}

void AllocSetLST::Add(AllocSet& set, u64 begin, u64 end)
{
















































}

void AllocSetLST::FinaliseTree()
{
	size_t layerId = 0;

	while ((m_tree[layerId]->size() & 0x1) == 0)
	{
		// Need to propogate the tree down the pyramid.
		if (m_tree.size() == layerId + 1)
		{
			m_tree.push_back(new std::vector<AllocSetHeader>());
		}

		std::vector<AllocSetHeader>& destLayer = *m_tree[layerId + 1];
		std::vector<AllocSetHeader>& sourceLayer = *m_tree[layerId];

		AllocSetHeader& lhs = *(sourceLayer.end() - 2);
		AllocSetHeader& rhs = *(sourceLayer.end() - 1);

		AllocSetHeader foldedHeader(lhs.allocEvBegin, rhs.allocEvEnd);
		foldedHeader.totalSize = lhs.totalSize + rhs.totalSize;

		foldedHeader.set.Add(lhs.set);
		foldedHeader.set.Add(rhs.set);

		Serialise(layerId, &lhs - &*sourceLayer.begin());
		Serialise(layerId, &rhs - &*sourceLayer.begin());

		destLayer.push_back(foldedHeader);

		++ layerId;
	}

	m_tree.front()->push_back(AllocSetHeader(
		m_tree.front()->back().allocEvEnd,
		m_tree.front()->back().allocEvEnd + SplitAlignment));
}

void AllocSetLST::Serialise(size_t layer, size_t index)
{














}
