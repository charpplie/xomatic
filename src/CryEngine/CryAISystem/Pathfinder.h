#ifndef PATHFINDER_H
#define PATHFINDER_H


#include "IAISystem.h"
#include "NavPath.h"
#include "PathFollower.h"


typedef std::list<PathfindRequest *> PathQueue;


struct IAIPathFinderListerner;

class CPathfinder: public IAIPathFinder
{
public:
	CPathfinder (CAISystem * pAISystem);
	~CPathfinder ();

	INavPath *CreateEmptyPath() const;
	IPathFollower *CreatePathFollower(const PathFollowerParams &params) const;

	void RequestPathTo(const Vec3 &start, const Vec3 &end, const Vec3 &endDir, IAIPathAgent *pRequester, 
		bool allowDangerousDestination, int forceTargetBuildingId, float endTol, float endDistance);
	void RequestPathTo(uint32 startIndex, uint32 endIndex, const Vec3 &endDir, IAIPathAgent *pRequester, 
		bool allowDangerousDestination, int forceTargetBuildingId, float endTol, float endDistance);
	int RequestRawPathTo(const Vec3 &start, const Vec3 &end, float passRadius, IAISystem::tNavCapMask navCapMask,
		unsigned &lastNavNode, bool allowDangerousDestination, float endTol, const PathfindingExtraConstraints &constraints, IAIPathAgent *pReference = 0);
	void RequestPathInDirection(const Vec3 &start, const Vec3 &pos, float maxDist,
		IAIPathAgent *pRequester, float endDistance);
	void CancelAnyPathsFor(IAIPathAgent* pRequester, bool actorRemoved = false);
	void CancelCurrentRequest ();
	// stops the pathfinder and reschedules the original request - used if the graph
	// changes
	void RescheduleCurrentPathfindRequest();
	bool IsFindingPathFor(const IAIPathAgent *pRequester) const;
	/// Attempts to generate an almost straight path between the current request
	/// start and end up to a maxCost. 
	/// If unsuccessful -1 will be returned. If successful 
	/// straightPath will be non-empty and the approximate
	/// cost will be returned. The straightPath will contain the start position, but no 
	/// points behind the requester's current position (as in BeautifyPath) which is
	/// specified by curPos. If returnPartialPath is set then success will be returned and 
	/// the path will go as far as it can towards the destination
	float AttemptStraightPath(TPathPoints &straightPath, const PathfindRequest &request,
		const Vec3 &curPos, const CHeuristic* pHeuristic, float maxCost,
		const NavigationBlockers& navigationBlockers, bool returnPartialPath);

	/// Does a search for a partial path from startPos up to a maximum cost as calculated by the
	/// heuristic and returns the position on the path closest (in terms of cost, determined by the
	/// heuristic) to the end position. This uses a brute-force approach, so is only meant for 
	/// fairly small values of maxCost (maybe up to a few tens of m). 
	/// If there's a problem (e.g. start position is bad) then the position returned is startPos.
	/// The maxCost value is only approximate - if possible this function will return endPos, even 
	/// if the actual cost exceeds maxCost. Currently, if endPos is too far away, only graph node
	/// locations get returned.
	Vec3 GetBestPosition(const CHeuristic& heuristic, float maxCost, 
		const Vec3& startPos, const Vec3& endPos, unsigned startHintIndex, IAISystem::tNavCapMask navCapMask);

	Vec3 GetBestPosition(const 	PathfindingHeuristicProperties &heuristic, float maxCost, 
		const Vec3 &startPos, const Vec3 &endPos, unsigned startHintIndex, IAISystem::tNavCapMask navCapMask);

	void UpdatePathFinder();

	/// Walks through over object in the system that needs to be serialised, or is referenced
	/// by serialisation, and adds it to the object tracker (to generate/store an ID for it)
	void PopulateObjectTracker(class CObjectTracker& objectTracker);

	const INavPath *GetCurrentPath() const { return &m_navPath; };
	const CNavPath &GetNavPath() const { return m_navPath; };
	const PathfindRequest * GetPathfindCurrentRequest() const { return m_pCurrentRequest; };

	void FlushPathQueue ();
	bool IsPathQueueEmpty () const;

	void Reset(IAISystem::EResetReason reason);
	void Serialize( TSerialize ser, CObjectTracker& objectTracker );
	void SerializePointers( TSerialize ser, CObjectTracker& objectTracker );

	void GetMemoryStatistics(ICrySizer *pSizer);

	void RegisterPathFinderListener(IAIPathFinderListerner* pListener);
	void UnregisterPathFinderListener(IAIPathFinderListerner* pListener);

	const AStarSearchNodeVector& GetTaggedNodesVector();

private:
	bool CheckForAndHandleShortPath(const PathfindRequest &request);
	void BeautifyPath(const VectorConstNodeIndices & pathNodes);
	void QueuePathfindRequest(const PathfindRequest &request);
	void CancelPath(int id);
	void NotifyPathFinderListeners(int id, const std::vector<unsigned>* pathNodes);
	/// Returns the set of navigation blockers last used in A* (may be a little out of date)
	const NavigationBlockers& GetNavigationBlockers() const {return m_navigationBlockers;}
	/// Populates a collection of navigation blockers based on the currently active 
	/// puppers (etc) in relation to the requester
	void GetNavigationBlockers(NavigationBlockers& navigationBlockers, IAIPathAgent *pRequester, const PathfindRequest *pfr);
	/// Internal helper - check/handle a position. pAIObject can be zero (if so, assume
	/// it is the player)
	void GetNavigationBlocker(NavigationBlockers& navigationBlockers, 
		const Vec3 &requesterPos, const Vec3 &requesterVel,
		Vec3 pos, const Vec3& blockerVel, CAIActor* pAIActor);
	/// Attempts to generate an almost straight (and simple) path in a certain direction, up to length
	/// maxDist. Returns the actual length of the path (-1 if there were no path points added)
	float AttemptPathInDir(TPathPoints &path, const Vec3 &curPos, const Vec3 &dir, const PathfindRequest &request, 
		const CHeuristic* pHeuristic, float maxCost, const NavigationBlockers& navigationBlockers);

	// ---

	class CAStarSolver * m_pAStarSolver;

	NavigationBlockers m_navigationBlockers;

	/// when we generate a path we store it here - the requester can get it from us when
	/// we send it AIEVENT_ONPATHDECISION. 
	CNavPath m_navPath;
	PathQueue	m_lstPathQueue;
	PathfindRequest *m_pCurrentRequest;
	EPathfinderResult m_nPathfinderResult;

	int m_pathFindIdGen;

	VectorSet<IAIPathFinderListerner*> m_pathFindListeners;

	CTimeValue	m_fLastPathfindTimeStart;
	
	// Time consumed by path finding (in seconds)
	float m_timeConsumption;
};

#endif // PATHFINDER_H
