/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   Navigation.h
$Id$
Description: interface for the CGraph class.

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009   : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "INavigation.h"
#include "BuildingIDManager.h"
#include "CTriangulator.h"

struct SpecialArea
{
	// (MATT) Note that this must correspond exactly to the enum in the Editor's Navigation.h {2009/06/17}
	enum EType {
		TYPE_WAYPOINT_HUMAN,
		TYPE_VOLUME,
		TYPE_FLIGHT,
		TYPE_WATER,
		TYPE_WAYPOINT_3DSURFACE,
		TYPE_FREE_2D,
		TYPE_TRIANGULATION,
		TYPE_LAYERED_NAV_MESH
	};

	void SetPolygon(const ListPositions &polygon) {lstPolygon = polygon; CalcAABB();}
	const ListPositions &GetPolygon() const {return lstPolygon;}

	const AABB &GetAABB() const {return aabb;}

	float fMinZ,fMaxZ;
	float fHeight;
	float fNodeAutoConnectDistance;
	int m_iWaypointType;
	int bVehiclesInHumanNav;
	int	nBuildingID;
	SpecialArea::EType type;
	bool  bCalculate3DNav;
	bool  bCalculateLNM;
	float f3DNavVolumeRadius;
	EAILightLevel lightLevel;

	EWaypointConnections	waypointConnections;
	bool	bAltered;		// making links unpassible

	SpecialArea() : nBuildingID(-1), type(SpecialArea::TYPE_WAYPOINT_HUMAN),
		bCalculate3DNav(true), bCalculateLNM(true),
		f3DNavVolumeRadius(10.0f), waypointConnections(WPCON_DESIGNER_NONE),
		bVehiclesInHumanNav(false), bAltered(false), lightLevel(AILL_NONE)
	{
		fMinZ = 9999.f;
		fMaxZ = -9999.f;
		fHeight = 0;
		aabb.Reset();
	}

private:
	ListPositions	lstPolygon;
	AABB aabb;

	void CalcAABB()
	{
		aabb.Reset();
		for (ListPositions::const_iterator it = lstPolygon.begin() ; it != lstPolygon.end() ; ++it)
			aabb.Add(*it);
	}
};

typedef std::map<string, SpecialArea> SpecialAreaMap;
// used for faster GetSpecialArea() lookup in game mode
typedef std::vector<SpecialArea*> SpecialAreaVector;

struct SExtraLinkCostShape
{
	SExtraLinkCostShape(const ListPositions& shape, const AABB& aabb, float costFactor) : shape(shape), aabb(aabb), costFactor(costFactor), origCostFactor(costFactor) {}
	SExtraLinkCostShape(const ListPositions& shape, float costFactor) : shape(shape), costFactor(costFactor), origCostFactor(costFactor)
	{
		aabb.Reset();
		for (ListPositions::const_iterator it = shape.begin() ; it != shape.end() ; ++it)
			aabb.Add(*it);
	}
	ListPositions shape;
	AABB aabb;
	// the cost factor can get modified at run-time - it will get reset at the same time
	// as graph links get reset etc
	float costFactor;
	float origCostFactor;
};

typedef std::map<string, SExtraLinkCostShape> ExtraLinkCostShapeMap;


struct CutEdgeIdx
{
	int idx1;
	int idx2;

	CutEdgeIdx( int i1, int i2 ) 
	{
		idx1 = i1;
		idx2 = i2;
	}

	// default ctor to allow std::vector::resize(0)
	CutEdgeIdx() {}
};

typedef std::vector<CutEdgeIdx> NewCutsVector;


// FIXME Jan 30, 2008: <pvl> these should probably be members of their respective
// classes they save, they need to be visible both in Navigation.cpp
// CAISystem.cpp so let's put them here until refactored further.
void ReadArea(CCryFile & file, int version, string & name, SpecialArea & sa);
bool ReadForbiddenArea(CCryFile & file, int version, CAIShape* shape);
bool ReadPolygonArea(CCryFile & file, int version, string & name, ListPositions & pts);
void ReadExtraLinkCostArea(CCryFile & file, int version, string & name, SExtraLinkCostShape &shape);

class CNavRegion;
class CTriangularNavRegion;
class CWaypointHumanNavRegion;
class CWaypoint3DSurfaceNavRegion;
class CFlightNavRegion;
class CVolumeNavRegion;
class CRoadNavRegion;
class CFree2DNavRegion;
class CSmartObjectNavRegion;
class CCompositeLayeredNavMeshRegion;
class CCustomNavRegion;

class CNavigation : public INavigation
{
public:
	CNavigation(CAISystem * , ISystem * );
	~CNavigation();

	// INavigation
	VIRTUAL uint32 GetPath(const char *szPathName, Vec3 *points, uint32 maxpoints) const;
	VIRTUAL	float GetNearestPointOnPath(const char *szPathName, const Vec3& vPos, Vec3& vResult, bool& bLoopPath, float& totalLength) const;
	VIRTUAL void GetPointOnPathBySegNo(const char *szPathName, Vec3& vResult, float segNo) const;
	VIRTUAL bool IsSegmentValid(IAISystem::tNavCapMask navCap, float rad, const Vec3& posFrom, Vec3& posTo, IAISystem::ENavigationType& navTypeFrom) const;
	//~INavigation

	const SpecialAreaMap &GetSpecialAreas() const {return m_mapSpecialAreas;}
	const SpecialArea *GetSpecialArea(const Vec3 &pos, SpecialArea::EType areaType);
	// Gets the special area with a particular building ID - may return 0 if it cannot be found
	const SpecialArea * GetSpecialArea(int buildingID) const;
	const SpecialArea *GetSpecialAreaNearestPos(const Vec3 &pos, SpecialArea::EType areaType);

	typedef std::vector< std::pair<string, const SpecialArea*> > VolumeRegions;
	/// Fills in the container of special areas that relate to 3D volumes
	void GetVolumeRegions(VolumeRegions& volumeRegions) const;
	const ShapeMap& GetDesignerPaths() const { return m_mapDesignerPaths; }

	// copies a designer path into provided list if a path of such name is found
	bool GetDesignerPath(const char * szName, SShape &path) const;

	bool Init();
	void Reset(IAISystem::EResetReason reason);
	void ShutDown();
	void FlushSystemNavigation();
	// Gets called after loading the mission
	void OnMissionLoaded();
	// // loads the triangulation for this level and mission
	void LoadNavigationData(const char * szLevel, const char * szMission, bool demandLoadLNMs = false);
	// NOTE Oct 9, 2009: <pvl> demand-loading layered navigation meshes
	bool LoadNavMesh(const char * navModifName, const char * agentTypeName);
	bool UnloadNavMesh(const char * navModifName, const char * agentTypeName);

	void Serialize( TSerialize ser, CObjectTracker& objectTracker );

	// reads special areas from file. clears the existing special areas
	void ReadAreasFromFile(CCryFile & , int fileVersion);

	void Update(CTimeValue currentTime, float frameTime);
	void UpdateNavRegions();

	enum ENavDataState {NDS_UNSET, NDS_OK, NDS_BAD};
	ENavDataState GetNavDataState () const { return m_navDataState; }

	void FlushAllAreas();
	void FlushSpecialAreas();

	void InsertSpecialArea(const string & name, SpecialArea & sa);
	void EraseSpecialArea(const string & name);
	/// rebuilding the QuadTrees will not be super-quick so they simply get
	/// cleared when they are invalidated.
	void ClearForbiddenQuadTrees();
	/// QuadTrees will be built on (a) triangulation (b) loading a level
	void RebuildForbiddenQuadTrees();
	/// This is just for debugging
	const char *GetNavigationShapeName(int nBuildingID) const;
	/// Checks if navigation shape exists - called by editor
	bool DoesNavigationShapeExists(const char * szName, EnumAreaType areaType, bool road = false);
	bool CreateNavigationShape(const SNavigationShapeParams &params);
	/// Deletes designer created path/shape - called by editor
	void DeleteNavigationShape(const char * szName);
	void DisableModifier (const char * name);

	/// Returns true if a point is in a forbidden region. When two forbidden regions
	/// are nested then it is just the region between them that is forbidden. This
	/// only checks the forbidden areas, not the boundaries.
	bool IsPointInForbiddenRegion(const Vec3 & pos, bool checkAutoGenRegions = true) const;
	/// Optionally returns the forbidden region the point is in (if it is in a forbidden region)
	bool IsPointInForbiddenRegion(const Vec3 & pos, const CAIShape** ppShape, bool checkAutoGenRegions) const; // internal use
	/// Returns true if the point is inside the nav modifiers marked out as containing water
	bool IsPointInWaterAreas(const Vec3 &pt) const;
	/// Indicates if a point is in a special area, checking the height too
	static bool IsPointInSpecialArea(const Vec3 &pos, const SpecialArea &sa);
	/// returns true if pos is inside a TRIANGULATION nav modifier, or if there are no
	/// such modifiers.
	/// NOTE this must only be called in editor (since it shouldn't be needed in game) - will assert that
	/// this is the case!
	bool IsPointInTriangulationAreas(const Vec3 &pos) const;
	/// if there's intersection vClosestPoint indicates the intersection point, and the edge normal
	/// is optionally returned. If bForceNormalOutwards is set then in the case of forbidden
	/// boundaries this normal is chosen to point (partly) towards vStart.
	/// nameToSkip can optionally point to a string indicating a forbidden area area to not check
	/// mode indicates if areas and/or boundaries should be checked
	VIRTUAL bool IntersectsForbidden(const Vec3 & vStart, const Vec3 & vEnd, Vec3 & vClosestPoint, const string * nameToSkip = 0,Vec3* pNormal = NULL, 
		INavigation::EIFMode mode = INavigation::IF_AREASBOUNDARIES, bool bForceNormalOutwards=false) const;
	/// Returns true if a point is on/close to a forbidden boundary/area edge
	bool IsPointOnForbiddenEdge(const Vec3& pos, float tol = 0.0001f, Vec3* pNormal = 0, const CAIShape** ppPolygon = 0, bool checkAutoGenRegions = true) const;
	/// Returns true if a point is within a forbidden boundary. This
	/// only checks the forbidden boundaries, not the areas.
	bool IsPointInForbiddenBoundary(const Vec3 & pos, const CAIShape** ppShape = 0) const;
	/// Returns true if it is impossible (assuming path finding is OK) to get from start 
	/// to end without crossing a forbidden boundary (except for moving out of a 
	/// forbidden region). 
	VIRTUAL bool IsPathForbidden(const Vec3 & start, const Vec3 & end) const;
	/// Returns true if a point is inside forbidden boundary/area edge or close to its edge
	bool IsPointForbidden(const Vec3 & pos, float tol, const CAIShape** ppShape, Vec3* pNormal = 0) const;
	VIRTUAL bool IsPointForbidden(const Vec3 & pos, float tol, Vec3* pNormal = 0) const;
	/// Get the best point outside any forbidden region given the input point, 
	/// and optionally a start position to stay close to
	VIRTUAL Vec3 GetPointOutsideForbidden(Vec3& pos, float distance, const Vec3* startPos = 0) const;

	bool GetBuildingInfo(int nBuildingID, IAISystem::SBuildingInfo& info) const;
	bool IsPointInBuilding(const Vec3& pos, int nBuildingID) const;

	/// Returns nearest designer created path/shape.
	/// The devalue parameter specifies how long the path will be unusable by others after the query.
	/// If useStartNode is true the start point of the path is used to select nearest path instead of the nearest point on path.
	VIRTUAL const char*	GetNearestPathOfTypeInRange(IAIObject* requester, const Vec3& pos, float range, int type, float devalue, bool useStartNode);

	/// indicates if a linesegment would intersect the extra cost areas
	bool OverlapExtraCostAreas(const Lineseg & lineseg) const;
	/// Returns the extra link cost associated with the link by intersecting against the appropriate
	/// shapes. 0.0 means no shapes were found.
	float GetExtraLinkCost(const Vec3 &pos1, const Vec3 &pos2) const;
	/// Internal helper - returns the extra cost associated with traversing a single shape. Initial AABB is 
	/// assumed to have already been done and passed
	float GetExtraLinkCost(const Vec3 &pos1, const Vec3 &pos2, const AABB &linkAABB, const SExtraLinkCostShape &shape) const;

	void DisableNavigationInBrokenRegion(std::list<Vec3> & outline);

	VIRTUAL void ModifyNavCostFactor(const char *navModifierName, float factor);

	/// returns the names of the region files generated during volume generation
	VIRTUAL void GetVolumeRegionFiles(const char * szLevel, const char * szMission, DynArray<CryStringT<char> > & filenames) const;

	/// Returns the base-class nav region - needs the graph only if type is waypoint (so you can
	/// pass in 0 if you know it's not... but be careful!)
	CNavRegion *GetNavRegion(IAISystem::ENavigationType type, const CGraph *pGraph) const;

	/// Triangular nav region may be associated with more than one graph
	CTriangularNavRegion* GetTriangularNavRegion() {return m_pTriangularNavRegion;}
	CWaypointHumanNavRegion* GetWaypointHumanNavRegion() {return m_pWaypointHumanNavRegion;}
	CWaypoint3DSurfaceNavRegion* GetWaypoint3DSurfaceNavRegion() {return m_pWaypoint3DSurfaceNavRegion;}
	CFlightNavRegion* GetFlightNavRegion() {return m_pFlightNavRegion;}
	CVolumeNavRegion* GetVolumeNavRegion() {return m_pVolumeNavRegion;}
	CRoadNavRegion* GetRoadNavRegion() {return m_pRoadNavRegion;}
	CFree2DNavRegion* GetFree2DNavRegion() {return m_pFree2DNavRegion;}
	CSmartObjectNavRegion* GetSmartObjectsNavRegion() {return m_pSmartObjectNavRegion;}
	// FIXME Feb 17, 2009: <pvl> with multiple LNM regions just "get LNM region" is
	// not the right question to ask.  Figure out how to deal with it.
	CCompositeLayeredNavMeshRegion* GetLayeredNavMeshRegion() { return m_pLayeredNavMeshRegion;}

	CCustomNavRegion* GetCustomNavRegion() { return m_pCustomNavRegion; }	
	const CCustomNavRegion* GetCustomNavRegion() const { return m_pCustomNavRegion; }

	const std::vector<float> & Get3DPassRadii() const {return m_3DPassRadii;}

	Vec3 IsFlightSpaceVoid(const Vec3 &vPos, const Vec3 &vFwd, const Vec3 &vWng, const Vec3 &vUp ) const;
	Vec3 IsFlightSpaceVoidByRadius(const Vec3 &vPos, const Vec3 &vFwd, float radius ) const;

	IAISystem::ENavigationType CheckNavigationType(const Vec3 & pos, int & nBuildingID, IVisArea *&pArea, IAISystem::tNavCapMask navCapMask) const;

	void GetMemoryStatistics(ICrySizer *pSizer);

	void DebugDraw() const;
	void DebugDrawForbidden() const;
	void DebugDrawNavModifiers() const;
	void DebugDrawLayeredNavMesh () const;

private:
	/// Indicates if the navigation data is sufficiently valid after loading that
	/// we should continue
	ENavDataState m_navDataState;

	// <NAV REGION STUFF>
	CTriangularNavRegion *m_pTriangularNavRegion;
	CWaypointHumanNavRegion *m_pWaypointHumanNavRegion;
	CWaypoint3DSurfaceNavRegion *m_pWaypoint3DSurfaceNavRegion;
	CVolumeNavRegion *m_pVolumeNavRegion;	
	CFlightNavRegion *m_pFlightNavRegion;	
	CRoadNavRegion *m_pRoadNavRegion;	
	CFree2DNavRegion *m_pFree2DNavRegion;
	CSmartObjectNavRegion* m_pSmartObjectNavRegion;	
	CCompositeLayeredNavMeshRegion * m_pLayeredNavMeshRegion;
	CCustomNavRegion *m_pCustomNavRegion;

	/// used during 3D nav generation - the pass radii of the entities that will use the navigation
	std::vector<float> m_3DPassRadii;
	// </NAV REGION STUFF>

	// <AI SHAPE STUFF>
	ShapeMap m_mapDesignerPaths;

	CAIShapeContainer m_forbiddenAreas;
	CAIShapeContainer m_designerForbiddenAreas;
	CAIShapeContainer m_forbiddenBoundaries;
	// </AI SHAPE STUFF>

	// <NAV MODIFIER STUFF>
	ExtraLinkCostShapeMap m_mapExtraLinkCosts;

	SpecialAreaMap	m_mapSpecialAreas;	// define where to disable automatic AI processing.
	SpecialAreaVector m_vectorSpecialAreas;	// to speed up GetSpecialArea() in game mode

	unsigned int m_nNumBuildings;
	CBuildingIDManager	m_BuildingIDManager;
	// </NAV MODIFIER STUFF>

	struct SValidationErrorMarker
	{
		SValidationErrorMarker(const string& msg, const Vec3& pos, const OBB& obb, ColorB col) : msg(msg), pos(pos), obb(obb), col(col) {}
		SValidationErrorMarker(const string& msg, const Vec3& pos, ColorB col) : msg(msg), pos(pos), obb(obb), col(col)
		{
			obb.SetOBBfromAABB(Matrix33(IDENTITY), AABB(Vec3 (-0.1f, -0.1f, -0.1f), Vec3 (0.1f, 0.1f, 0.1f)));
		}
		Vec3 pos;
		OBB obb;
		string msg;
		ColorB col;
	};

	std::vector<SValidationErrorMarker>	m_validationErrorMarkers;


	// NOTE Oct 27, 2009: <pvl> unless demand-loading of individual layered nav
	// meshes is in use this shouldn't be needed.  For demand-loading we need to
	// know even after initialization which file we load (parts of) graph from.
	string m_graphFileName;

	//====================================================================
	// Used during graph generation - should subsequently be empty
	//====================================================================
	CTriangulator *m_pTriangulator;
	std::vector<Tri*>	m_vTriangles;
	VARRAY  m_vVertices;
};

#endif // NAVIGATION_H
