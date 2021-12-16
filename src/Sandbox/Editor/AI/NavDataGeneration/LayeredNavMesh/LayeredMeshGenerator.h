
#ifndef LAYERED_MESH_GENERATOR_H
#define LAYERED_MESH_GENERATOR_H

#include "NavMesh.h"		// for BuildSettings

struct ICVar;
struct SpecialArea;
struct AgentPathfindingProperties;

namespace LayeredNavMesh {

class Voxelizer;
class NavSurfaceGenerator;
class NavContourGenerator;
class NavPolygonGenerator;
class GeomMesh;
class GeomInstance;
template <typename > class DynArray;

} // namespace LayeredNavMesh

class CGraph;
class NavGraphGenerator;

class CLayeredMeshGenerator {
public:
	CLayeredMeshGenerator (CGraph * graph, unsigned modifIndex);
	~CLayeredMeshGenerator ();

	/// Generate nav mesh for area enclosed by a specific nav modifier.
	void Generate (
			const string & navModifName, const SpecialArea & , const string & agentType,
			const std::vector <const SpecialArea *> & exclusionVols
	);

	/// Generate nav mesh for the whole map.
	void Generate ();

	/// Writes out data needed in-game in a format suitable for fast loading.
	void Export (const char * filename) const;

	/// Meant for debug draws, there shouldn't be anything else to update here.
	void Update () const;

	static void InitConsoleVars ();

private:
	void DoGenerate ();
	void Reset ();
	void FillInSettings (float agentHeight, float agentRadius, int maxSlopeDeg);

	LayeredNavMesh::BuildSettings m_settings;

	LayeredNavMesh::Voxelizer * m_pVoxelizer;
	LayeredNavMesh::NavSurfaceGenerator * m_pSurfaceGen;
	LayeredNavMesh::NavContourGenerator * m_pContourGen;
	LayeredNavMesh::NavPolygonGenerator * m_pPolygonGen;
	NavGraphGenerator * m_pGraphGen;

	struct CVarMgr;
	static CVarMgr * s_cvarMgr;


	CGraph * m_pGraph;		// NOTE Jun 9, 2008: <pvl> we don't own the memory

	// NOTE Feb 18, 2009: <pvl> uniquely identifies this instance among others
	unsigned m_navModifIndex;
	string m_navModifName;	// NOTE Feb 17, 2009: <pvl> just for export

	// NOTE Aug 7, 2009: <pvl> the type of agent this nav mesh is computed for
	string m_agentType;

	// NOTE Aug 3, 2009: <pvl> terrain mesh as computed by ConvertTerrain(),
	// only used for debug-drawing
	LayeredNavMesh::GeomMesh * m_terrain;

	LayeredNavMesh::DynArray<LayeredNavMesh::GeomMesh*> * m_meshes;
	LayeredNavMesh::DynArray<LayeredNavMesh::GeomInstance*> * m_instances;
};

#endif // LAYERED_MESH_GENERATOR_H
