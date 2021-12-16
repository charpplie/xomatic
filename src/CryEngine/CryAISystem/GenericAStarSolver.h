#ifndef GENERICASTARSOLVER_H
#define GENERICASTARSOLVER_H

#include <vector>
#include <list>
#include <map>
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////
//DISCLAIMER/////////////////////////////////////////////////////////////////
//This is not really THAT generic of an A*.  Is it missing your needed feature?
//	=>Likely!!
//Please feel encouraged as reader/user of this file to make it more generic.
//
//Only Requests:
//KEEP IT CLEAN!
//	(And make sure it stays backwards compatible for whoever is using)
//
//DISCLAIMER/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//TODO's:////////////////////////////////////////////////////////////////////
//beautifier
//test custom traversals cost
//closed list => soft closed based on G //=>template param Heuristic admisable
//closed list could be bit array;
//	=>parent node stored in pool of AStarNodes only pointed at by open std::list
//TODO's:////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//AStar OpenList Node////////////////////////////////////////////////////////
struct AStarNode
{
	float G, F;
	int graphNodeIndex;

	AStarNode(float theG, float theF, int theGraphNodeIndex)
		:G(theG), F(theF), graphNodeIndex(theGraphNodeIndex)
	{}

	bool operator<(const AStarNode& other)const
	{
		return F > other.F;		//backwards cause this is priority
	}

	bool operator<=(const AStarNode& other)const
	{
		return F >= other.F;	//backwards cause this is priority
	}
};
//AStar OpenList Node////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//Static polymorphic interfaces://///////////////////////////////////////////
template<typename YourGraph, typename YourReferencePoint>
class GraphTraits
{
public:
	int GetBestNodeIndex(YourReferencePoint& referencePoint)const;
	YourReferencePoint GetNodeReferencePoint(int nodeIndex)const;

	int GetNumLinks(int nodeIndex)const;
	int GetNextNodeIndex(int graphNodeIndex, int linkIndex)const;
};

template<typename YourGraph>
class HeuristicTraits
{
public:
	float H(const YourGraph* graph, int currentNodeIndex, int endNodeIndex) const;
};

template<typename YourGraph>
class TraversalCostCalculatorTraits
{
public:
	float GetCost(const YourGraph* graph, int currentNodeIndex, int nextNodeIndex) const;
};

template<typename YourGraph>
class OpenListTraits
{
public:
	void insert(const AStarNode& node);
	AStarNode pop_front();
	void clear();

	bool empty()const;
};

template<typename YourGraph>
class ClosedListTraits
{
public:
	void insert(int nodeIndex, int previousNodeIndex);
	int find(int nodeIndex);

	int StartNodePreviousIndex()const;
	int end()const;

	void clear();
};
//Static polymorphic interfaces://///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//Optional OpenList Imp: Heap////////////////////////////////////////////////
class OpenList_Heap
{
	std::vector<AStarNode> openList;

public:

	void insert(const AStarNode& node)
	{
		openList.push_back(node);
		std::push_heap(openList.begin(), openList.end());
	}

	AStarNode pop_front()
	{
		std::pop_heap(openList.begin(), openList.end());
		AStarNode node = openList.back();
		openList.pop_back();

		return node;
	}

	void clear()
	{
		openList.clear();
	}

	bool empty()const
	{
		return openList.empty();
	}

	//For users debug benefit
	void print()const
	{
		printf("Open List:\n");
		for(size_t i = 0; i < openList.size(); ++i)
		{
			const AStarNode& node = openList[i];
			printf("<GraphNode: %i; Cost So Far: %f; Est cost: %f>\n"
				,node.graphNodeIndex
				,node.G
				,node.F
				);
		}
	}
};
//Optional OpenList Imp: Heap////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//Optional OpenList Imp: List////////////////////////////////////////////////
class OpenList_List
{
	std::list<AStarNode> openList;

public:

	void insert(const AStarNode& node)
	{
		std::list<AStarNode>::iterator iter = openList.begin();
		while(iter != openList.end())
		{
			if(*iter <= node)
			{
				break;
			}
			++iter;
		}
		openList.insert(iter, node);
	}

	AStarNode pop_front()
	{
		AStarNode node = openList.front();
		openList.pop_front();
		return node;
	}

	void clear()
	{
		openList.clear();
	}

	size_t empty()const
	{
		return openList.empty();
	}

	//For users debug benefit
	void print()const
	{
		printf("Open List:\n");
		for(std::list<AStarNode>::const_iterator iter = openList.begin(); iter != openList.end(); ++iter)
		{
			const AStarNode& node = *iter;
			printf("<GraphNode: %i; Cost So Far: %f; Est cost: %f>\n"
				,node.graphNodeIndex
				,node.G
				,node.F
				);
		}
	}
};
//Optional OpenList Imp: List////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//DefaultClosedList//////////////////////////////////////////////////////////
class DefaultClosedList
{
	std::map<int, int> closedList;
public:

	void insert(int currentNodeIndex, int lastNodeIndex)
	{
		closedList.insert(std::pair<int, int>(currentNodeIndex, lastNodeIndex));
	}

	int find(int nodeIndex)const
	{
		std::map<int, int>::const_iterator iter = closedList.find(nodeIndex);
		if(iter != closedList.end())
			return iter->second;
		return -1;
	}

	void clear()
	{
		closedList.clear();
	}

	int end()const
	{
		return -1;
	}

	int StartNodePreviousIndex()const
	{
		return -2;
	}

	//For users debug benefit
	void print()const
	{
		printf("Closed List:\n");
		for(std::map<int, int>::const_iterator iter = closedList.begin(); iter != closedList.end(); ++iter)
		{
			if(iter->second == StartNodePreviousIndex())
				printf("Node Index: %i; Previous Node: None\n", iter->first);
			else
				printf("Node Index: %i; Previous Node: %i\n", iter->first, iter->second);
		}
	}
};
//DefaultClosedList//////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//DefaultOpenList////////////////////////////////////////////////////////////
//For many cases std::list priority queue inserted at front will be fastest
typedef OpenList_List /*OpenList_Heap*/ DefaultOpenList;
//DefaultOpenList////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//GenericAStarSolver/////////////////////////////////////////////////////////
template<
	typename Graph
	,typename Heuristic
	,typename TraversalCostCalculator
	,typename ReferencePoint
	,typename ClosedList = DefaultClosedList
	,typename OpenList = DefaultOpenList>
class GenericAStarSolver
{
	const Graph* graph;
	OpenList openList;
	ClosedList closedList;

	std::vector<int> finalPathNodeIndices;
	int goalNodeIndex;
	//ReferencePoint end; ?TODO apend to final path?
	//TODO Graph should chose index values for invalid, and start?
	//testing for closed std::list faster with bit array
	
	TraversalCostCalculator costCalculator;		//TODO make pointers so can use same solver with interface member set on start
	const Heuristic* heuristic;						//TODO make pointers so can use same solver with interface member set on start

public:

	void StartPathFind(const Graph* theGraph, ReferencePoint theStart, ReferencePoint theEnd, const Heuristic* theHeuristic)
	{
		graph = theGraph;
		heuristic = theHeuristic;

		openList.clear();
		closedList.clear();
		finalPathNodeIndices.clear();

		int theStartNodeIndex = theGraph->GetBestNodeIndex(theStart);
		goalNodeIndex = theGraph->GetBestNodeIndex(theEnd);

		const float G = 0;
		float F = G + heuristic->H(graph, theStartNodeIndex, goalNodeIndex);
		openList.insert(AStarNode(G, F, theStartNodeIndex));
		closedList.insert(theStartNodeIndex, closedList.StartNodePreviousIndex());
	}

	int Update(int theMaxNumNodesToProcess)
	{
		for(; 0 < theMaxNumNodesToProcess; --theMaxNumNodesToProcess)
		{
			if(openList.empty())
			{
				break;
			}

			AStarNode currentAStarNode = openList.pop_front();

			if(currentAStarNode.graphNodeIndex != goalNodeIndex)
			{
				int numLinks = graph->GetNumLinks(currentAStarNode.graphNodeIndex);
				for(int linkIndex = 0; linkIndex < numLinks; ++linkIndex)
				{
					int nextNodeIndex = graph->GetNextNodeIndex(currentAStarNode.graphNodeIndex, linkIndex);
					if(closedList.find(nextNodeIndex) != closedList.end())
					{
						//this next node is already marked, ignore it.
						continue;
					}

					float H = heuristic->H(graph, nextNodeIndex, goalNodeIndex);
					float G = currentAStarNode.G + costCalculator.GetCost(graph, currentAStarNode.graphNodeIndex, nextNodeIndex);
					float F = G + H;

					closedList.insert(nextNodeIndex, currentAStarNode.graphNodeIndex);
					openList.insert(AStarNode(G, F, nextNodeIndex));
				}
			}
			else
			{
				//Goal found, save the path and quit:
				int nodeIndex = currentAStarNode.graphNodeIndex;
				while(nodeIndex != closedList.StartNodePreviousIndex())
				{
					finalPathNodeIndices.push_back(nodeIndex);
					nodeIndex = closedList.find(nodeIndex);
				}
				break;
			}
		}
		return theMaxNumNodesToProcess;
	}

	void GetPath(std::vector<ReferencePoint>& pathReferencePoints)
	{
		//TODO (maybe?): not destroy path here?
		while(!finalPathNodeIndices.empty())
		{
			int index = finalPathNodeIndices.back();
			finalPathNodeIndices.pop_back();

			pathReferencePoints.push_back(graph->GetNodeReferencePoint(index));
		}
	}

	//For users debug benefit
	const OpenList& GetOpenList()
	{
		return openList;
	}

	//For users debug benefit
	const ClosedList& GetClosedList()
	{
		return closedList;
	}
};
//GenericAStarSolver/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


#endif //GENERICASTARSOLVER_H