#include <stdafx.h>
#include "FreedSpaceQuery.h"

#include "CallstackTable.h"
#include "memHier2.h"
#include "MemoryMap.h"
#include "ReplayLogDefs.h"
#include "ReplayLogReader.h"
#include "SymbolHelper.h"

namespace
{
	class FragmentationAnalysis : public IReplayListener
	{
	public:
		FragmentationAnalysis(u64 allocEvEnd, const std::vector<int>& limitToBuckets);

		virtual void ReplayBegin();
		virtual void Replay(ReplayRange range);
		virtual void ReplayEnd(u64 position);

		SharedPtr<GenericTree> GetMemHier() const { return m_tree; }

	private:
		u64 m_allocEvEnd;
		std::vector<int> m_limitToBuckets;

		MemAddressProfile m_addressProfile;
		MemoryMap m_map;
		u64 m_allocEv;

		std::vector<BucketRange> m_bucketRanges;

		SharedPtr<GenericTree> m_tree;
	};
}

FreedSpaceQuery::FreedSpaceQuery(u64 allocEvEnd, const std::vector<int>& limitToBuckets)
	: m_allocEvEnd(allocEvEnd)
	, m_limitToBuckets(limitToBuckets)
{
}

void FreedSpaceQuery::RunImpl(ReplayLogReader& reader)
{
	FragmentationAnalysis analysis(m_allocEvEnd, m_limitToBuckets);
	reader.Replay(analysis);
	Complete(analysis.GetMemHier());
}

FragmentationAnalysis::FragmentationAnalysis(u64 allocEvEnd, const std::vector<int>& limitToBuckets)
	: m_allocEvEnd(allocEvEnd)
	, m_limitToBuckets(limitToBuckets)
{
	std::sort(m_limitToBuckets.begin(), m_limitToBuckets.end());
}

void FragmentationAnalysis::ReplayBegin()
{
	m_map.Clear();
	m_allocEv = 0;
}

void FragmentationAnalysis::Replay(ReplayRange range)
{
	using namespace ReplayEventIds;

	bool queryBuckets = !m_limitToBuckets.empty();

	ReplayEventIds::Ids id;

	while (range.ReadNext(id) && (m_allocEv < m_allocEvEnd))
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

		case RE_Alloc3:
			{
				const ReplayAlloc3Event& ev = range.Get<ReplayAlloc3Event>();

				bool accept = true;
				size_t sz = ev.sizeConsumed;

				if (queryBuckets)
				{
					accept = false;

					std::vector<BucketRange>::iterator it = std::lower_bound(m_bucketRanges.begin(), m_bucketRanges.end(), ev.ptr, BucketRangePtrPredicate());
					if (it != m_bucketRanges.begin())
					{
						if (it->base != ev.ptr)
							-- it;
						accept = (ev.ptr >= it->base && (ev.ptr + ev.sizeConsumed) <= it->end);
						sz = (sz + it->alignment - 1) & ~(it->alignment - 1);
					}
				}

				if (accept)
				{
					m_map.BlitAlloc(ev.ptr, sz, ev.callstack, ev.callstackLength);
				}

				++ m_allocEv;
			}
			break;

		case RE_Free3:
			{
				const ReplayFree3Event& ev = range.Get<ReplayFree3Event>();
				m_map.BlitFree(ev.ptr);

				++ m_allocEv;
			}
			break;

		case RE_Free4:
			{
				const ReplayFree4Event& ev = range.Get<ReplayFree4Event>();
				m_map.BlitFree(ev.ptr);

				++ m_allocEv;
			}
			break;

		case ReplayEventIds::RE_BucketMark2:
			{
				if (queryBuckets)
				{
					const ReplayBucketMark2Event& ev = range.Get<ReplayBucketMark2Event>();

					if (std::binary_search(m_limitToBuckets.begin(), m_limitToBuckets.end(), ev.index))
					{
						BucketRange item(ev.ptr, ev.ptr + ev.length, ev.index, ev.alignment);
						m_bucketRanges.insert(
							std::lower_bound(m_bucketRanges.begin(), m_bucketRanges.end(), item),
							item);
					}
				}
			}
			break;
		}
	}
}

static GenericTreeNode& FindNode(const TAddress* callstack, size_t callstackSize, GenericTree& tree)
{
	GenericTreeNode* node = tree.GetRoot();

	for (size_t idx = 0; idx != callstackSize; ++ idx)
	{
		GenericTreeNode* searchNode;

		for (searchNode = node->children;
			searchNode && searchNode->GetKey<TAddress>() != callstack[idx];
			searchNode = searchNode->sibling)
			;

		if (!searchNode)
		{
			searchNode = new GenericTreeNode(callstack[idx]);
			tree.AddChild(node, searchNode);
		}

		node = searchNode;
	}

	return *node;
}

void FragmentationAnalysis::ReplayEnd(u64 position)
{
	if (m_map.IsEmpty())
		return;

	struct FragSizeInfo
	{
		size_t count[MemGroups::Count];
		size_t mem[MemGroups::Count];
		size_t age;

		FragSizeInfo()
		{
			memset(this, 0, sizeof(*this));
		}
	};

	std::map<size_t, FragSizeInfo> callstackFragHits;

	size_t idx = 0;
	std::vector<MemoryMap::AllocInfo>* page = &m_map.PagesBegin()->second;

	for (MemoryMap::PageIterator it = m_map.PagesBegin(), itEnd = m_map.PagesEnd(); it != itEnd; )
	{
		TAddress ptr = it->first | (idx << MemoryMap::AlignmentShift);
		MemoryMap::AllocInfo& alloc = (*page)[idx];

		size_t sz = 0;
		do
		{
			sz += MemoryMap::Alignment;

			++ idx;
			if (idx == page->size())
			{
				idx = 0;
				++ it;
				if (it == itEnd)
					break;
				page = &it->second;
			}
		}
		while ((*page)[idx].id == alloc.id);

		if (alloc.id && !alloc.inUse)
		{
			FragSizeInfo& pr = callstackFragHits[m_map.GetAllocEvInfo(alloc.id).callstackId];
			int memGroup = MemGroups::SelectGroupForAddress(ptr, m_addressProfile);

			++ pr.count[memGroup];
			pr.mem[memGroup] += sz;
			pr.age = std::max(pr.age, alloc.id);//(pr.age + alloc.id) / 2;
		}
	}

	m_tree = CreateCodeGenericTree();
	GenericTreeStream<SizeInfoGroups>& exclusive = *m_tree->GetStream<SizeInfoGroups>("Exclusive");

	CallstackTable& callstacks = *m_map.GetCallstacks();
	
	for (std::map<size_t, FragSizeInfo>::iterator it = callstackFragHits.begin(), itEnd = callstackFragHits.end(); it != itEnd; ++ it)
	{
		TAddress callstack[256];
		size_t callstackSize = 256;

		if (callstacks.TryFindCallstack(it->first, callstack, callstackSize))
		{
			GenericTreeNode& node = FindNode(callstack, callstackSize, *m_tree);
			node[exclusive].AddToGroup(MemGroups::Main, SizeInfo(
				it->second.count[MemGroups::Main], 
				0,
				it->second.mem[MemGroups::Main],
				it->second.mem[MemGroups::Main], 0));
			node[exclusive].AddToGroup(MemGroups::RSX, SizeInfo(
				it->second.count[MemGroups::RSX],
				0,
				it->second.mem[MemGroups::RSX],
				it->second.mem[MemGroups::RSX], 0));
		}
	}

	SumSizes(m_tree->GetRoot(), exclusive, *m_tree->GetStream<SizeInfoGroups>("Inclusive"));
}
