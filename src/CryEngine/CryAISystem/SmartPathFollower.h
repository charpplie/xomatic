#ifndef SMARTPATHFOLLOWER_H
#define SMARTPATHFOLLOWER_H


#include <IPathfinder.h>

// TEMP: Based on old structure, will be removed when path data is no longer duplicated.
struct SPathControlPoint2
{
	SPathControlPoint2()
		: navType(IAISystem::NAV_UNSET), pos(ZERO), distance(ZERO), customId(0) {}
	SPathControlPoint2(IAISystem::ENavigationType navType, const Vec3 &pos, uint16 customId = 0)
		: navType(navType), pos(pos), customId(customId) {}

	Vec3 pos;
	IAISystem::ENavigationType navType;
	float distance;		// Distance along path
	uint16 customId;
};

/**
Represents a continuous navigation path with support for fractional (i.e. non-integer) indexing
and automatic interpolation of values that lay between control points.

TODO: Merge with NavPath and avoid duplication of path data.
*/
class InterpolatedPath
{
public:
	InterpolatedPath() : m_totalDistance() {}

	/// Search for a segment index based on the distance along the path, startIndex is optional to allow search optimization
	inline float FindSegmentIndexAtDistance(float distance, size_t startIndex = 0) const;

	/// Returns the closest segment found between the two distances
	inline float FindClosestSegmentIndex(const Vec3& testPoint, float startDistance = 0.0f, float endDistance = FLT_MAX) const;

	/// True if the path section between the start and endIndex deviates from the line by no more than maxDeviation.
	inline bool IsParrallelTo(const Lineseg& line, float startIndex, float endIndex, float maxDeviation) const;

	/// Returns the interpolated position for a given index
	inline Vec3 GetPositionAtSegmentIndex(float index) const;

	/// Returns the interpolated distance for a given index
	inline float GetDistanceAtSegmentIndex(float index) const;

	/// Returns the line segment bounded by the two segment indices
	inline void GetLineSegment(float startIndex, float endIndex, Lineseg& segment) const;

	/// Returns the index of the first navigation type transition after index or ~0
	inline size_t FindNextNavTypeSectionIndexAfter(size_t index) const;

	/// Returns the next index on the path that deviates from a straight line by the deviation specified.
	inline float FindNextInflectionIndex(float startIndex, float maxDeviation = 0.1f) const;

	/// Shortens the path to the specified endIndex
	inline void ShortenToIndex(float endIndex);

	/// Returns the total distance of the path along all segments
	inline float TotalDistance() const { return m_totalDistance; }

	/// std::vector style methods for back compatibility with old code
	inline void clear() { m_points.clear(); m_totalDistance = 0.0f; }
	inline void push_back(const SPathControlPoint2& point);
	inline size_t size() const { return m_points.size(); }
	inline bool empty() const { return m_points.empty(); }
	inline const SPathControlPoint2& front() const { return m_points.front(); }
	inline SPathControlPoint2& back() { return m_points.back(); }
	inline const SPathControlPoint2& operator[](size_t index) const { return m_points[index]; }

private:
	typedef std::vector<SPathControlPoint2> TPoints;

	TPoints m_points;					// Points that make up the path
	float		m_totalDistance;	// Distance to end of path
};

/**
 PathFollower implementation based on tracking the furthest reachable position on the path.
*/
class CSmartPathFollower : public IPathFollower
{
	/// The copied version of the original path
	InterpolatedPath m_path;

	/// Parameters describing how we follow the path, modified directly (!)
	PathFollowerParams m_params;

	/// The original agent navigation path
	INavPath *m_pNavPath;	// TODO: Use this directly by merging InterpolatedPath with INavPath.

	/// Path obstacles to avoid when reaching the destination target
	const CPathObstacles* m_pPathObstacles;

	/// The path version - used to detect changes to the source path
	int m_pathVersion;

	/// Current position of the user agent
	Vec3 m_curPos;

	/// Position used as start position when calculating the most recent follow target.
	Vec3 m_validatedStartPos;

	/// Current segment index of the follow target along the path.
	float m_followTargetIndex;

	/// Current inflection point (point beyond follow target that deviates from straight path)
	float m_inflectionIndex;

	/// Used to limit the maximum reach tests performed per update (time-slicing)
	//mutable uint32 m_reachTestCount;

	/// True when the FT has reached an optimal line-of-access (allows a form of time slicing)
	//bool m_safePathOptimal;

	/// Attempts to find a reachable target between startIndex (exclusive) & endIndex (inclusive).
	bool FindReachableTarget(float startIndex, float endIndex, float& reachableIndex) const;

	/// True if the test position can be reached by the agent from the start position.
	bool CanReachTarget(const Vec3 startPos, float testIndex) const;

	/// calculates distance between two points, depending on m_params.use_2D
	float DistancePointPoint(const Vec3 pt1, const Vec3 pt2) const;
	/// calculates distance squared between two points, depending on m_params.use_2D
	float DistancePointPointSq(const Vec3 pt1, const Vec3 pt2) const;

	/// Used internally to create our list of path control points
	void ProcessPath();

	// Hiding this virtual method here, so they are only accessible through the IPathFollower interface
	// (i.e. Outside the AISystem module)
	virtual void Release() { delete this; }

public:
  CSmartPathFollower(const PathFollowerParams &params = PathFollowerParams(), const CPathObstacles* pPathObstacles = NULL);
  ~CSmartPathFollower();

	virtual void Reset();

  /// This attaches us to a particular path (pass 0 to detach)
  virtual void AttachToPath(INavPath *pNavPath);

	virtual void SetParams(const PathFollowerParams &params) {m_params=params;}

  /// The params may be modified as the path is followed - bear in mind that doing so
  /// will invalidate any predictions
  virtual PathFollowerParams &GetParams() {return m_params;}
  /// Just view the params
  virtual const PathFollowerParams &GetParams() const {return m_params;}

	// Attempts to advance the follow target along the path as far as possible while ensuring the follow 
	// target remains reachable. Returns true if the follow target is reachable, false otherwise.
  virtual bool Update(PathFollowResult &result, const Vec3 &curPos, const Vec3 &curVel, float dt);

  /// Advances the current state in terms of position - effectively pretending that the follower
  /// has gone further than it has.
	virtual void Advance(float distance) {}		// TODO: Remove, but needed by old implementation for now...

  /// Returns the distance from the lookahead to the end, plus the distance from the position passed in
  /// to the LA if pCurPos != 0
  virtual float GetDistToEnd(const Vec3 *pCurPos) const;

  /// Returns the distance along the path from the current look-ahead position to the 
  /// first smart object path segment. If there's no path, or no smart objects on the 
  /// path, then std::numeric_limits<float>::max() will be returned
  virtual float GetDistToSmartObject() const;
  virtual float GetDistToNavType(IAISystem::ENavigationType navType) const;
	virtual float GetDistToCustomNav(const InterpolatedPath &controlPoints, uint32 curLASegmentIndex, const Vec3 &curLAPos) const;
	/// Returns a point on the path some distance ahead. actualDist is set according to 
  /// how far we looked - may be less than dist if we'd reach the end
	virtual Vec3 GetPathPointAhead(float dist, float &actualDist) const;

	virtual void Draw(const Vec3& drawOffset=ZERO) const;

	virtual void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
};

#endif