
#ifndef LAYERED_NAV_MESH_REGION_H
#define LAYERED_NAV_MESH_REGION_H

#include "NavRegion.h"

#include <memory>





#include "../../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/PolygonCont.h"
#include "../../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/NavMesh.h"	// for DEBUG_DRAW


// ---
// TODO Okt 29, 2008: <pvl> does this warrant a separate .h/.cpp similar to PolygonCont?

class DynamicGraphOverlayMgr;
namespace LayeredNavMesh {
	class IndexRangeMgr;
	namespace ExportFormat {
		class FileReader;
	}
}

// NOTE Okt 29, 2008: <pvl> similar in spirit to PolygonCont - its main mission
// is to translate LNM indices in a way that multiplexes among static nodes and
// dynamically overlaid ones
class GraphNodeCont {
	std::vector <unsigned int> m_staticGraphNodes;

	// NOTE Okt 29, 2008: <pvl> we don't own these pointers
	const LayeredNavMesh::IndexRangeMgr * m_indexRangeMgr;
	const DynamicGraphOverlayMgr * m_overlayMgr;
public:
	GraphNodeCont ();

	// NOTE Okt 29, 2008: <pvl> until this is called only static nodes are handled
	void SetupDynamicObstacleHandling (const DynamicGraphOverlayMgr * , const LayeredNavMesh::IndexRangeMgr * );

	// NOTE Okt 29, 2008: <pvl> takes an "LNM dynamic index" and returns CGraph node index
	unsigned operator[] (unsigned index) const;

	void Clear ();

	void SetupStaticGraphNodes (const CGraph * , unsigned numNodes, unsigned modifId, int agentTypeId);

	void SetupReadingFromFile (LayeredNavMesh::ExportFormat::FileReader * );
	void CleanupReadingFromFile (LayeredNavMesh::ExportFormat::FileReader * );
};

inline GraphNodeCont::GraphNodeCont () : m_indexRangeMgr (0), m_overlayMgr (0)
{
}

inline void GraphNodeCont::SetupDynamicObstacleHandling (
			const DynamicGraphOverlayMgr * overlayMgr,
			const LayeredNavMesh::IndexRangeMgr * indexRangeMgr)
{
	m_indexRangeMgr = indexRangeMgr;
	m_overlayMgr = overlayMgr;
}

// ---

class PickingAccelerator;
class DynamicObstacleMgr;
class PathBlockingEntityMgr;
class DbgPathBlockingEntityMgr;
class ObstaclePolygon;

class CLayeredNavMeshRegion : public CNavRegion
{
public:
	CLayeredNavMeshRegion (CGraph * );
	~CLayeredNavMeshRegion ();

	virtual void BeautifyPath(
		const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
		const Vec3& startPos, const Vec3& startDir, 
		const Vec3& endPos, const Vec3 & endDir,
		float radius,
		const AgentMovementAbility & movementAbility,
		const NavigationBlockers& navigationBlockers);

	// NOTE Aug 8, 2008: <pvl> probably temporary
	void BeautifyPath (const VectorConstNodeIndices& inPath, TPathPoints& outPath,
			const Vec3& startPos, const Vec3& endPos);

	virtual void UglifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
		const Vec3& startPos, const Vec3& startDir, 
		const Vec3& endPos, const Vec3 & endDir)
	{ }

	virtual bool CheckPassability(const Vec3& from, const Vec3& to, float radius, const NavigationBlockers& navigationBlockers, IAISystem::tNavCapMask ) const;

	virtual unsigned GetEnclosing(const Vec3 &pos, float passRadius = 0.0f, unsigned startIndex = 0,
		float range = -1.0f, Vec3 * closestValid = 0, bool returnSuspect = false, const char *requesterName = "");

	virtual void Clear();

	virtual void Reset (IAISystem::EResetReason reason);

	virtual void Serialize(TSerialize ser, CObjectTracker& objectTracker) { }
	
	virtual size_t MemStats() { return 0; }

	// Reads navigation from binary file, returns true on success.
	bool ReadFromFile (const char * fname);

	// ATTN Jan 23, 2009: <pvl> at the time of this writing, these functions aren't
	// actually used since real obstacles (entity-based, as opposed to SShape-based
	// debug ones) are driven by physics events that go to 'm_pathBlockerMgr' where
	// entities are converted to polygons and added directly to 'm_obstacleMgr'.
	// I'll keep them around for a while though before I maybe get rid of them.
	void AddDynamicObstacle (const ObstaclePolygon * );
	void RemoveDynamicObstacle (const ObstaclePolygon * );
#ifdef _DEBUG
	// NOTE Jan 21, 2009: <pvl> to support simulating obstacles using SShapes for debugging
	void AddDynamicObstacle (const SShape * );
	void RemoveDynamicObstacle (const SShape * );
#endif // _DEBUG
	void ProcessDynamicObstacles ();
	// NOTE Okt 24, 2008: <pvl> for mem leak debugging atm but could be useful later
	void ResetDynamicObstacleHandling ();

	unsigned ModifierId () const { return m_id; }
	const string & ModifierName () const { return m_navModifName; }
	int AgentTypeId () const { return m_agentTypeId; }
	const string & AgentTypeName () const { return m_agentTypeName; }
	bool ContainsPos (const Vec3 & ) const;

	float DistanceToObstacle (const Vec3 & pt, const Vec3 & dir) const;

	Lineseg GetSharedEdge (unsigned poly0, unsigned poly1) const;

private:
	void SetupPickingAccelerator (const Vec3 & sceneMin, const Vec3 & sceneMax);
	unsigned GetEnclosingPolygon (const Vec3 &pos) const;
	unsigned GetNearestPolygon (const Vec3 & pos, float range) const;
	unsigned GetEnclosingGraphNode (const Vec3 &pos, float range) const;
	unsigned int SelectEnclosing (const Vec3 & pos, const std::vector <unsigned> & candidatePolys) const;


	CGraph * m_pGraph;

	// NOTE Feb 18, 2009: <pvl> uniquely identifies this instance among others.
	// Useful for telling if a graph node belongs to this instance.
	unsigned m_id;
	string m_navModifName;

	int m_agentTypeId;
	string m_agentTypeName;

	// TODO Aug 10, 2009: <pvl> just AABB now, should probably become polygonal
	// prism at some point?
	AABB m_servicedVolume;

	LayeredNavMesh::PolygonCont m_polygons;
	GraphNodeCont m_graphNodes;

	// NOTE Sep 23, 2008: <pvl> should never be modified by this class once
	// initialized.  It's passed over to a dynamic obstacle manager though,
	// which can modify the nav mesh and the accelerator.  Would be nice if
	// m_pickingAccel could be const.
	std::auto_ptr <PickingAccelerator> m_pickingAccel;

	std::auto_ptr <DynamicObstacleMgr> m_obstacleMgr;
	std::auto_ptr <PathBlockingEntityMgr> m_pathBlockerMgr;
#ifdef _DEBUG
	std::auto_ptr <DbgPathBlockingEntityMgr> m_dbgPathBlockerMgr;
#endif // _DEBUG

#ifdef DEBUG_DRAW
	class DebugDrawMgr {
		const CLayeredNavMeshRegion * m_owner;

		void PolygonAlongPath (unsigned int polyIndex, unsigned numberAlongPath) const;
	public:
		DebugDrawMgr (const CLayeredNavMeshRegion * owner) : m_owner(owner) { }

		void operator() () const;
		void operator() (const CAStarSolver::tPathNodes & ) const;

		void GetEnclosingTest (const Vec3 & pos) const;
		void AreaOverlapTest (const AABB & area) const;

		void MeshCuttingTest (const SShape * shape) const;

		void RaycastTest (const Vec3 & pos) const;
	};
	friend class DebugDrawMgr;
public:
	DebugDrawMgr DebugDraw;
#endif // DEBUG_DRAW
};

// TODO Mar 23, 2009: <pvl> there's no good place for these ATM
namespace PSO {
class Polygon;
class Allocator;
}
PSO::Polygon * Convert (unsigned operandNum, const PSO::Polygon * shape, PSO::Allocator * , bool tagVtxs = false);
PSO::Polygon * Convert (unsigned operandNum, const SShape & shape, PSO::Allocator * , bool tagVtxs = false);
PSO::Polygon * Convert (unsigned operandNum, const LayeredNavMesh::PolygonCont::Polygon & srcPoly, PSO::Allocator * , bool tagVtxs = false);
PSO::Polygon * Convert (unsigned operandNum, const std::vector<Vec3> & shape, PSO::Allocator * , bool tagVtxs = false);


// ---

// NOTE Aug 7, 2009: <pvl> purpose of this class is to multiplex among a collection
// of ordinary LayeredNavMeshRegions
class CCompositeLayeredNavMeshRegion : public CNavRegion
{
public:
	CCompositeLayeredNavMeshRegion (CGraph * );
	~CCompositeLayeredNavMeshRegion ();

	virtual void BeautifyPath(
		const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
		const Vec3& startPos, const Vec3& startDir, 
		const Vec3& endPos, const Vec3 & endDir,
		float radius,
		const AgentMovementAbility & movementAbility,
		const NavigationBlockers& navigationBlockers);

	// NOTE Aug 8, 2008: <pvl> probably temporary
	// UPDATE Jan 11, 2010: <pvl> if the two BeautifyPath() entry points are ever
	// consolidated, keep in mind they differ in functionality slightly.  The inherited
	// one pretty much assumes that the caller is Pathfinder::BeautifyPath().  That
	// function pre-processes the path returned by A* to chop it up to chunks of
	// the same navtype.  This means that the BeautifyPath() in the appropriate
	// NavRegion instance never has to worry about seeing a path node of a different
	// navtype.
	// The non-inherited, simpler signature is currently called just from debug
	// code which passes in the path as it comes out of the pathfinder.  It means
	// that it needs to take care of skipping foreign path nodes itself.
	void BeautifyPath (const VectorConstNodeIndices& inPath, TPathPoints& outPath,
			const Vec3& startPos, const Vec3& endPos);

	virtual void UglifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
		const Vec3& startPos, const Vec3& startDir, 
		const Vec3& endPos, const Vec3 & endDir)
	{ }

	virtual bool CheckPassability(const Vec3& from, const Vec3& to, float radius, const NavigationBlockers& navigationBlockers, IAISystem::tNavCapMask navCapMask) const;

	// NOTE Aug 14, 2009: <pvl> inherited from NavRegion.  This version should
	// never be called as its signature misses agent type information, making it
	// impossible to implement.
	virtual unsigned GetEnclosing(const Vec3 &pos, float passRadius = 0.0f, unsigned startIndex = 0,
		float range = -1.0f, Vec3 * closestValid = 0, bool returnSuspect = false, const char *requesterName = "");

	// NOTE Aug 14, 2009: <pvl> this is the GetEnclosing() overload that's actually
	// useful - the downside is, it doesn't conform to the NavRegion interface
	unsigned GetEnclosing (const Vec3 & pos, int agentType, float range);

	virtual void Clear();

	virtual void Reset (IAISystem::EResetReason reason);

	virtual void Serialize(TSerialize ser, CObjectTracker& objectTracker) { }
	
	virtual size_t MemStats() { return 0; }

	// Reads navigation from binary file, returns true on success.
	bool ReadFromFile (const char * fname, bool demandLoading = false);

	bool LoadNavMesh (const char * navModifName, const char * agentTypeName);
	bool UnloadNavMesh (const char * navModifName, const char * agentTypeName);
	bool IsNavMeshLoaded (const char * navModifName, const char * agentTypeName) const;
	bool IsAnyNavMeshLoaded () const { return !m_regions.empty(); }
	unsigned GetModifierId (const char * navModifName, const char * agentTypeName) const;
	CLayeredNavMeshRegion * GetModifierByIndexAndType (unsigned modifIndex, unsigned agentType) const;

	void DebugDraw () const;

private:

	bool InitDemandLoading (const char * fname);

	CLayeredNavMeshRegion * Check (std::vector <CLayeredNavMeshRegion*>::iterator it) const;
	std::vector<CLayeredNavMeshRegion*>::iterator GetModifier (const Vec3 & pos, unsigned agentType) const;
	std::vector<CLayeredNavMeshRegion*>::iterator GetModifier (unsigned modifIndex, unsigned agentType) const;
	std::vector<CLayeredNavMeshRegion*>::iterator GetModifier (const char * navModifName, const char * agentTypeName) const;

	CGraph * m_graph;
	std::vector<CLayeredNavMeshRegion*> m_regions;

	// ---
	// NOTE Oct 8, 2009: <pvl> demand-loading related stuff follows

	struct MeshInfo {
		string m_fileName;
		unsigned m_modifIndex;
		MeshInfo (const string & fname, unsigned modifIndex) :
				m_fileName (fname), m_modifIndex (modifIndex)
		{ }
	};
	// NOTE Oct 8, 2009: <pvl> mesh is identified here by concatenation of
	// modifier and agent type name
	typedef std::map <string, MeshInfo> MeshInfoMap;
	MeshInfoMap m_meshToFname;

	const char * GetFilename (const char * navModifName, const char * agentTypeName) const;
};


#endif // LAYERED_NAV_MESH_REGION_H
