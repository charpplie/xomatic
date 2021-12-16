#pragma once

#include "ReplayQuery.h"

class CodeTreeOffsetQuery : public ReplayQuery<SharedPtr<GenericTree> >
{
public:
	CodeTreeOffsetQuery(const std::vector<std::pair<u64, u64> >& spans, bool includeAllocs, bool includeFrees, const std::vector<int>& limitToBuckets, SharedPtr<SymbolHelper> symbols)
		: m_spans(spans)
		, m_includeAllocs(includeAllocs)
		, m_includeFrees(includeFrees)
		, m_limitToBuckets(limitToBuckets)
		, m_symbols(symbols)
	{
	}

	void RunImpl(ReplayLogReader& reader);

private:
	std::vector<std::pair<u64, u64> > m_spans;
	bool m_includeAllocs;
	bool m_includeFrees;
	std::vector<int> m_limitToBuckets;
	SharedPtr<SymbolHelper> m_symbols;
};
