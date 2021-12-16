#include <StdAfx.h>
#include "AnimationGraph_2.h"
#include <queue>
#include "TransitionReport_2.h"

typedef std::map<CAGState2Ptr, size_t> CostMap;

static bool IsNullState( CAGState2Ptr pState )
{
	return (pState->GetTemplateType() == "Default") || (pState->GetFactoryCount() == 0);
}

static void DijkstraAnimGraph( CAGState2Ptr fromNode, CostMap& costs )
{
	costs.clear();

	CAnimationGraph2Ptr pGraph = fromNode->GetGraph();
	for (CAnimationGraph2::state_iterator iter = pGraph->StateBegin(); iter != pGraph->StateEnd(); ++iter)
		costs[*iter] = ~size_t(0);
	costs[fromNode] = 0;

	typedef std::queue<CAGState2Ptr> StateQueue;

	StateQueue q;
	q.push(fromNode);

	std::vector<CAGState2Ptr> links;
	while (!q.empty())
	{
		CAGState2Ptr pState = q.front();
		q.pop();
		pGraph->StatesLinkedFrom( links, pState );
		size_t baseCost = costs[pState];
		for (std::vector<CAGState2Ptr>::const_iterator iter = links.begin(); iter != links.end(); ++iter)
		{
			CAGState2Ptr pToState = *iter;
			if (!pToState->IncludeInGame())
				continue;
			CostMap::iterator iterCost = costs.find(pToState);
			int cost = baseCost;
			int extraCost = !IsNullState(pToState);
			cost = baseCost + extraCost;
			if (cost < iterCost->second)
			{
				iterCost->second = cost;
				q.push(*iter);
			}
		}
	}
}

struct SLongTransition
{
	CString from;
	CString to;
	size_t length;

	bool operator<( const SLongTransition& rhs ) const
	{
		if (length > rhs.length)
			return true;
		else if (length < rhs.length)
			return false;
		else if (from < rhs.from)
			return true;
		else if (from > rhs.from)
			return false;
		else if (to < rhs.to)
			return true;
		else if (to > rhs.to)
			return false;
		return false;
	}
};

void FindLongTransitions( CString& out, CAnimationGraph2Ptr pGraph, size_t minCost )
{
	CostMap costs;
	std::vector<SLongTransition> longTrans;
	for (CAnimationGraph2::state_iterator iter = pGraph->StateBegin(); iter != pGraph->StateEnd(); ++iter)
	{
		if (IsNullState(*iter))
			continue;

		costs.clear();
		DijkstraAnimGraph( *iter, costs );
		for (CostMap::const_iterator iterC = costs.begin(); iterC != costs.end(); ++iterC)
		{
			if (IsNullState(iterC->first))
				continue;
			if (iterC->second >= minCost && iterC->second != ~size_t(0))
			{
				SLongTransition t;
				t.from = (*iter)->GetName();
				t.to = iterC->first->GetName();
				t.length = iterC->second;
				longTrans.push_back(t);
			}
		}
	}
	char temp[32];
	std::sort( longTrans.begin(), longTrans.end() );
	for (size_t i=0; i<longTrans.size(); i++)
	{
		sprintf( temp,"%d",(int)longTrans[i].length );
		out += longTrans[i].from + " -> " + longTrans[i].to + "  @ " + temp + "\n";
	}
}
