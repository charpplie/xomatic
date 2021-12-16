#include "stdafx.h"
#include "memHier2.h"
#include "CallstackTable.h"

#include "CodeTreeOffsetQuery.h"
#include "CodeTreeContextQuery.h"

#include "ReplayLogReader.h"

namespace
{
	struct OffsetSpanOrderPredicate
	{
		bool operator () (const std::pair<u64, u64>& a, const std::pair<u64, u64>& b) const
		{
			return a.first < b.first;
		}
	};

	template <typename IteratorT>
	IteratorT CollapseSpans(IteratorT first, IteratorT end)
	{
		std::sort(first, end, OffsetSpanOrderPredicate());

		IteratorT out = first;

		for (++ first; first != end; ++ first)
		{
			if (first->first > out->second)
			{
				++ out;
				*out = *first;
			}
			else
			{
				out->second = std::max(out->second, first->second);
			}
		}

		++ out;
		return out;
	}
}

void CopyTypeToMemHierRecurse(GenericTree& tree, GenericTreeStream<SizeInfoGroups>& exclusive, GenericTreeStream<SizeInfoGroups>& inclusive, GenericTreeNode* dst, TypeSymbol* pType)
{
	SizeInfo size(pType->m_count, 0, pType->m_length, pType->m_length, pType->m_length);

	dst->SetKey(pType);

	(*dst)[inclusive].AddToGroup(MemGroups::Main, size);
	if (pType->m_children.empty())
		(*dst)[exclusive].AddToGroup(MemGroups::Main, size);

	u32 total=0;
	for (size_t i = 0, ic = pType->m_children.size(); i < ic; ++ i)
	{
		GenericTreeNode* dstChild = new GenericTreeNode();
		tree.AddChild(dst, dstChild);

		CopyTypeToMemHierRecurse(tree, exclusive, inclusive, dstChild, pType->m_children[i]);

		total += pType->m_children[i]->m_length;
	}

	if (total < pType->m_length && !pType->m_children.empty())
	{
		GenericTreeNode *wasteNode = new GenericTreeNode(0);
		SizeInfo waste(pType->m_count, 0, 0, pType->m_length-total, pType->m_length-total);

		tree.AddChild(dst, wasteNode);
		(*wasteNode)[inclusive].AddToGroup(MemGroups::Main, waste);
	}
}

SharedPtr<GenericTree> MemHierFromTypeInfo(const char* typeName, const SharedPtr<SymbolHelper>& symbolTable)
{
	SharedPtr<GenericTree> mh = new GenericTree();
	mh->AddStream("Inclusive", new GenericTreeStream<SizeInfoGroups>());
	mh->AddStream("Exclusive", new GenericTreeStream<SizeInfoGroups>());

	GenericTreeNode* root = mh->GetRoot();

	TypeSymbol *pType=symbolTable->FindType(typeName);
	if (pType)
		CopyTypeToMemHierRecurse(*mh, *mh->GetStream<SizeInfoGroups>("Exclusive"), *mh->GetStream<SizeInfoGroups>("Inclusive"), root, pType);

	return mh;
}

GenericTreeNode* FindNodeForCallstackId(CallstackNodeMap& csnm, size_t id, CallstackTable& csTable, GenericTree& tree, GenericTreeNode* root)
{
	GenericTreeNode* node = root;

	CallstackNodeMap::iterator itCSNM = csnm.find(id);

	if (itCSNM != csnm.end())
		return itCSNM->second;

	TAddress cs[256];
	size_t csLen = 256;

	if (csTable.TryFindCallstack(id, cs, csLen))
	{
		for (u32 idx = 0; idx != csLen; ++ idx)
		{
			TAddress addr = cs[idx];
			GenericTreeNode* child = NULL;

			for (child = node->children; child; child = child->sibling)
			{
				if (child->GetKey<TAddress>() == addr)
					break;
			}

			if (child == NULL)
			{
				child = new GenericTreeNode(addr);
				tree.AddChild(node, child);
			}

			node = child;
		}

		csnm.insert(std::make_pair((u32)id, node));
	}

	return node;
}

SharedPtr<GenericTree> CreateCodeGenericTree()
{
	SharedPtr<GenericTree> destTree = new GenericTree();
	destTree->AddStream("Inclusive", new GenericTreeStream<SizeInfoGroups>());
	destTree->AddStream("Exclusive", new GenericTreeStream<SizeInfoGroups>());
	return destTree;
}

SharedPtr<GenericTree> QueryCode(
																 ReplayLogReader& stream,
																 const std::vector<std::pair<u64, u64> >& offsetSpans,
																 bool includeAllocs,
																 bool includeFrees,
																 const std::vector<int>& limitToBuckets,
																 SharedPtr<SymbolHelper> symbols)
{
	SharedPtr<GenericTree> mh = CreateCodeGenericTree();

	GenericTreeStream<SizeInfoGroups>& exclusive = *mh->GetStream<SizeInfoGroups>("Exclusive");
	GenericTreeStream<SizeInfoGroups>& inclusive = *mh->GetStream<SizeInfoGroups>("Inclusive");

	GenericTreeNode* root = mh->GetRoot();

	if (offsetSpans.empty())
		return mh;

	std::vector<std::pair<u64, u64> > spans(offsetSpans);
	spans.erase(CollapseSpans(spans.begin(), spans.end()), spans.end());
	std::reverse(spans.begin(), spans.end());

	bool queryBuckets = !limitToBuckets.empty();
	std::vector<BucketRange> bucketRanges;

	stream.Rewind();

	struct AllocInfo
	{
		AllocInfo(
			size_t csId,
			GenericTreeNode* node,
			SizeInfo sz)
			: csId(csId)
			, node(node)
			, sz(sz)
		{}

		size_t csId;
		GenericTreeNode* node;
		SizeInfo sz;
	};

	typedef std::map<
		TAddress,
		AllocInfo,
		std::less<const TAddress>,
		STLPoolAllocator<std::pair<const TAddress, AllocInfo> > > AllocsMap;

	typedef AllocsMap::iterator AllocIterator;

	AllocsMap allocs;

	CallstackTable csTable(symbols);
	CallstackNodeMap csnm;

	ReplayEventIds::Ids id;
	MemAddressProfile addressProfile;

	while (stream.ReadNext(id))
	{
		u64 streamPosition = stream.GetStreamPosition();
		while (!spans.empty() && streamPosition >= spans.back().second)
			spans.pop_back();

		if (spans.empty())
		{
			SumSizes(mh->GetRoot(), exclusive, inclusive);
			return mh;
		}

		switch (id)
		{
		case ReplayEventIds::RE_AddressProfile:
			{
				const ReplayAddressProfileEvent& ev = stream.Get<ReplayAddressProfileEvent>();
				addressProfile = MemAddressProfile(ev.rsxStart, 0xffffffff);
			}
			break;

		case ReplayEventIds::RE_AddressProfile2:
			{
				const ReplayAddressProfile2Event& ev = stream.Get<ReplayAddressProfile2Event>();
				addressProfile = MemAddressProfile(ev.rsxStart, ev.rsxStart + ev.rsxLength);
			}
			break;

		case ReplayEventIds::RE_Alloc3:
			{
				const ReplayAlloc3Event& ev = stream.Get<ReplayAlloc3Event>();

				ptrdiff_t csId = (ptrdiff_t) csTable.AddCallstack(ev);

				GenericTreeNode* node = NULL;

				bool accept = true;

				if (queryBuckets)
				{
					accept = false;

					std::vector<BucketRange>::iterator it = std::lower_bound(bucketRanges.begin(), bucketRanges.end(), ev.ptr, BucketRangePtrPredicate());
					if (it != bucketRanges.begin())
					{
						if (it->base != ev.ptr)
							-- it;
						accept = (ev.ptr >= it->base && ev.ptr < it->end);
					}
				}

				if (accept)
				{
					if (includeAllocs && (streamPosition >= spans.back().first))
					{
						node = FindNodeForCallstackId(csnm, csId, csTable, *mh, root);
						MemGroups::Group group = MemGroups::SelectGroupForAddress(ev.ptr, addressProfile);

						(*node)[exclusive].AddToGroup(group, SizeInfo(1, 0, ev.sizeRequested, ev.sizeConsumed, ev.sizeGlobal));
					}

					allocs.insert(std::make_pair(
						static_cast<TAddress>(ev.ptr),
						AllocInfo(csId, node, SizeInfo(1, 0, ev.sizeRequested, ev.sizeConsumed, ev.sizeGlobal))));
				}
			}
			break;

		case ReplayEventIds::RE_AllocUsage:
			{
				const ReplayAllocUsageEvent& ev = stream.Get<ReplayAllocUsageEvent>();

				AllocsMap::iterator it = allocs.find(ev.ptr);
				if (it != allocs.end())
				{
					ptrdiff_t diff = ev.used - it->second.sz.requested;
					it->second.sz.requested += diff;

					if (includeAllocs && it->second.node)
						exclusive[it->second.node->id].AddToGroup(MemGroups::SelectGroupForAddress(ev.ptr, addressProfile), SizeInfo(0, 0, diff, 0, 0));
				}
			}
			break;
			
		case ReplayEventIds::RE_Free3:
			{
				const ReplayFree3Event& ev = stream.Get<ReplayFree3Event>();

				AllocIterator it = allocs.find(ev.ptr);
				if (it != allocs.end())
				{
					if (includeFrees && (streamPosition >= spans.back().first))
					{
						GenericTreeNode* node = FindNodeForCallstackId(csnm, it->second.csId, csTable, *mh, root);
						MemGroups::Group group = MemGroups::SelectGroupForAddress(ev.ptr, addressProfile);

						SizeInfo sz(0, 1, -it->second.sz.requested, -it->second.sz.consumed, ev.sizeGlobal);
						(*node)[exclusive].AddToGroup(group, sz);
					}

					allocs.erase(it);
				}
			}
			break;

		case ReplayEventIds::RE_Free4:
			{
				const ReplayFree4Event& ev = stream.Get<ReplayFree4Event>();

				AllocIterator it = allocs.find(ev.ptr);
				if (it != allocs.end())
				{
					if (includeFrees && (streamPosition >= spans.back().first))
					{
						ptrdiff_t csId = (it->second.csId >= 0) ? it->second.csId : csTable.AddCallstack(ev);

						GenericTreeNode* node = FindNodeForCallstackId(csnm, csId, csTable, *mh, root);
						MemGroups::Group group = MemGroups::SelectGroupForAddress(ev.ptr, addressProfile);

						SizeInfo sz(0, 1, -it->second.sz.requested, -it->second.sz.consumed, ev.sizeGlobal);
						exclusive[node->id].AddToGroup(group, sz);
					}

					allocs.erase(it);
				}
			}
			break;

		case ReplayEventIds::RE_BucketMark2:
			{
				if (queryBuckets)
				{
					const ReplayBucketMark2Event& ev = stream.Get<ReplayBucketMark2Event>();

					if (std::binary_search(limitToBuckets.begin(), limitToBuckets.end(), ev.index))
					{
						BucketRange item(ev.ptr, ev.ptr + ev.length, ev.index, ev.alignment);
						bucketRanges.insert(
							std::lower_bound(bucketRanges.begin(), bucketRanges.end(), item),
							item);
					}
				}
			}
			break;
		}
	}

	SumSizes(mh->GetRoot(), exclusive, inclusive);

	return mh;
}

SharedPtr<GenericTree> QueryCodeContext(ReplayLogReader& stream, const std::vector<ContextStreamOffsetSpan>& streamOffsets, bool includeAllocs, bool includeFrees, SharedPtr<SymbolHelper> symbols)
{
	SharedPtr<GenericTree> mh = CreateCodeGenericTree();

	GenericTreeStream<SizeInfoGroups>& exclusive = *mh->GetStream<SizeInfoGroups>("Exclusive");
	GenericTreeStream<SizeInfoGroups>& inclusive = *mh->GetStream<SizeInfoGroups>("Inclusive");

	GenericTreeNode* root = mh->GetRoot();

	std::vector<ContextStreamOffsetSpan> localOffsets(streamOffsets);
	std::sort(localOffsets.rbegin(), localOffsets.rend());

	typedef std::map<TThreadId,
		std::vector<u64>,
		std::less<const TThreadId>,
		STLPoolAllocator<std::pair<const TThreadId, std::vector<u64> > > > ThreadOffsetSpanMap;

	ThreadOffsetSpanMap threadSpans;
	CallstackNodeMap csnm;

	// Sort the interleaved offsets into thread grouped offsets.
	for (std::vector<ContextStreamOffsetSpan>::const_iterator it = localOffsets.begin(), itEnd = localOffsets.end(); it != itEnd; ++ it)
	{
		threadSpans[it->threadId].push_back(it->offset);
	}

	if (localOffsets.empty())
		return mh;

	stream.Rewind();

	typedef std::map<TAddress,
		std::pair<GenericTreeNode*, SizeInfo>,
		std::less<const TAddress>,
		STLPoolAllocator<std::pair<const TAddress, std::pair<const GenericTreeNode*, SizeInfo> > > > AllocsMap;
	typedef AllocsMap::iterator AllocIterator;

	AllocsMap allocs;

	CallstackTable csTable(symbols);

	ReplayEventIds::Ids id;

	MemAddressProfile addressProfile;

	while (stream.ReadNext(id))
	{
		switch (id)
		{
		case ReplayEventIds::RE_AddressProfile:
			{
				const ReplayAddressProfileEvent& ev = stream.Get<ReplayAddressProfileEvent>();
				addressProfile = MemAddressProfile(ev.rsxStart, 0xffffffff);
			}
			break;

		case ReplayEventIds::RE_AddressProfile2:
			{
				const ReplayAddressProfile2Event& ev = stream.Get<ReplayAddressProfile2Event>();
				addressProfile = MemAddressProfile(ev.rsxStart, ev.rsxStart + ev.rsxLength);
			}
			break;

		case ReplayEventIds::RE_Alloc3:
			{
				const ReplayAlloc3Event& ev = stream.Get<ReplayAlloc3Event>();

				GenericTreeNode* node = NULL;

				std::vector<u64>& spanList = threadSpans[ev.threadId];

				if (includeAllocs && !spanList.empty() && (stream.GetStreamPosition() >= spanList.back()))
				{
					node = FindNodeForCallstackId(csnm, csTable.AddCallstack(ev), csTable, *mh, root);
					MemGroups::Group group = MemGroups::SelectGroupForAddress(ev.ptr, addressProfile);

					(*node)[exclusive].AddToGroup(group, SizeInfo(1, 0, ev.sizeRequested, ev.sizeConsumed, ev.sizeGlobal));
				}

				allocs.insert(std::make_pair(static_cast<TAddress>(ev.ptr), std::make_pair(node, SizeInfo(1, 0, ev.sizeRequested, ev.sizeConsumed, ev.sizeGlobal))));
			}
			break;

		case ReplayEventIds::RE_AllocUsage:
			{
				const ReplayAllocUsageEvent& ev = stream.Get<ReplayAllocUsageEvent>();

				AllocsMap::iterator it = allocs.find(ev.ptr);
				if (it != allocs.end())
				{
					ptrdiff_t diff = ev.used - it->second.second.requested;
					it->second.second.requested += diff;
					if (includeAllocs && it->second.first)
						(*it->second.first)[exclusive].AddToGroup(MemGroups::SelectGroupForAddress(ev.ptr, addressProfile), SizeInfo(0, 0, diff, 0, 0));
				}
			}
			break;

		case ReplayEventIds::RE_Free3:
			{
				const ReplayFree3Event& ev = stream.Get<ReplayFree3Event>();

				AllocIterator it = allocs.find(ev.ptr);
				if (it != allocs.end())
				{
					if (includeFrees && it->second.first)
					{
						MemGroups::Group group = MemGroups::SelectGroupForAddress(ev.ptr, addressProfile);
						SizeInfo szInfo(0, 1, -it->second.second.requested, -it->second.second.consumed, ev.sizeGlobal);

						(*it->second.first)[exclusive].AddToGroup(group, szInfo);
					}

					allocs.erase(it);
				}
			}
			break;

		case ReplayEventIds::RE_PushContext:
			{
				const ReplayPushContextEvent& ev = stream.Get<ReplayPushContextEvent>();

				std::vector<u64>& spanList = threadSpans[ev.threadId];

				while (!spanList.empty() && (stream.GetStreamPosition() > spanList.back()))
					spanList.pop_back();
			}
			break;

		case ReplayEventIds::RE_PushContext2:
			{
				const ReplayPushContext2Event& ev = stream.Get<ReplayPushContext2Event>();

				std::vector<u64>& spanList = threadSpans[ev.threadId];

				while (!spanList.empty() && (stream.GetStreamPosition() > spanList.back()))
					spanList.pop_back();
			}
			break;

		case ReplayEventIds::RE_PushContext3:
			{
				const ReplayPushContext3Event& ev = stream.Get<ReplayPushContext3Event>();

				std::vector<u64>& spanList = threadSpans[ev.threadId];

				while (!spanList.empty() && (stream.GetStreamPosition() > spanList.back()))
					spanList.pop_back();
			}
			break;

		case ReplayEventIds::RE_PopContext:
			{
				const ReplayPopContextEvent& ev = stream.Get<ReplayPopContextEvent>();

				std::vector<u64>& spanList = threadSpans[ev.threadId];

				while (!spanList.empty() && (stream.GetStreamPosition() > spanList.back()))
					spanList.pop_back();
			}
			break;
		}
	}

	SumSizes(root, exclusive, inclusive);

	return mh;
}

void CodeTreeContextQuery::RunImpl(ReplayLogReader& reader)
{
	Complete(QueryCodeContext(reader, m_spans, m_includeAllocs, m_includeFrees, m_symbols));
}

void CodeTreeOffsetQuery::RunImpl(ReplayLogReader& reader)
{
	Complete(QueryCode(reader, m_spans, m_includeAllocs, m_includeFrees, m_limitToBuckets, m_symbols));
}
