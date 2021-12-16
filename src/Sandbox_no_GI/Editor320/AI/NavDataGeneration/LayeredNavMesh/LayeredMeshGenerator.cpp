
#include "StdAfx.h"

#include "LayeredMeshGenerator.h"
#include "NavMesh.h"
#include "DynArray.h"
#include "GeomMesh.h"
#include "GeomInstance.h"
#include "Voxelizer.h"
#include "NavSurfaceGenerator.h"
#include "NavContourGenerator.h"
#include "NavPolygonGenerator.h"
#include "ExportFormat.h"
#include "LnmExportFormat.h"
#include "PickingAccelerator.h"
#include "env.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "../AILog.h"

// TODO Mai 14, 2008: <pvl> for SpecialArea definition - it needs to be moved
// to a separate file I think
#include "../Navigation.h"

#include "../Graph.h"

#include "IIndexedMesh.h"
#include "../../../Terrain/Heightmap.h"
#include "AI/AIManager.h"
#include "GameEngine.h"		// NOTE Feb 19, 2010: <pvl> just for level path - shameful


using LayeredNavMesh::GeomMesh;
using LayeredNavMesh::GeomInstance;
using LayeredNavMesh::Voxelizer;
using LayeredNavMesh::NavSurfaceGenerator;
using LayeredNavMesh::NavContourGenerator;
using LayeredNavMesh::NavPolygonGenerator;
using LayeredNavMesh::BuildSettings;

//#pragma optimize ("", off)
//#pragma inline_depth (0)

namespace LayeredNavMesh {

SceneBounds::AABB::AABB (const Vec3 & min, const Vec3 & max) :
		m_min (min), m_max (max)
{
}

const Vec3 & SceneBounds::AABB::Min () const
{
	return m_min;
}

const Vec3 & SceneBounds::AABB::Max () const
{
	return m_max;
}

// ---

class NavModifSceneBounds {
	// NOTE Feb 20, 2009: <pvl> polygon is stored in engine Vec3's since it's
	// used with engine overlap routines
	std::vector <::Vec3> m_vertices;
	// TODO Feb 20, 2009: <pvl> could be just as well engine AABB?  This class
	// depends on engine types (Vec3) anyway.
	SceneBounds::AABB m_aabb;
public:
	template <typename Iterator>
	NavModifSceneBounds (const Vec3 & aabbMin, const Vec3 & aabbMax, Iterator vtxBegin, Iterator vtxEnd); 

	const SceneBounds::AABB & GetAABB () const;

	bool Overlaps (const Vec3 & pt) const;
	bool Overlaps (const Vec3 & a, const Vec3 & b, const Vec3 & c) const;
	bool Overlaps (const std::vector <::Vec3> & polygon) const;
};

template <typename Iterator>
NavModifSceneBounds::NavModifSceneBounds (
			const Vec3 & aabbMin, const Vec3 & aabbMax,
			Iterator vtxBegin, Iterator vtxEnd) :
		m_aabb (aabbMin, aabbMax), m_vertices (vtxBegin, vtxEnd)
{
}

const SceneBounds::AABB & NavModifSceneBounds::GetAABB () const
{
	return m_aabb;
}

bool NavModifSceneBounds::Overlaps (const Vec3 & pt) const
{
	return (pt.y > m_aabb.Min().y) && (pt.y < m_aabb.Max().y)
				&& Overlap::Point_Polygon2D (ConvertVec3(pt), m_vertices);
}

bool NavModifSceneBounds::Overlaps (const Vec3 & a, const Vec3 & b, const Vec3 & c) const
{
	if ( ! ((a.y > m_aabb.Min().y) && (a.y < m_aabb.Max().y) || 
					(b.y > m_aabb.Min().y) && (b.y < m_aabb.Max().y) || 
					(c.y > m_aabb.Min().y) && (c.y < m_aabb.Max().y)) )
		return false;
					
	return Overlap::Triangle_Polygon2D(ConvertVec3(a), ConvertVec3(b), ConvertVec3(c), m_vertices);
}

bool NavModifSceneBounds::Overlaps (const std::vector <::Vec3> & polygon) const
{
	return Overlap::Polygon_Polygon2D (polygon, m_vertices);
}

// ---

SceneBounds::SceneBounds (const NavModifSceneBounds * impl) : m_impl (impl)
{
}

const SceneBounds::AABB & SceneBounds::GetAABB () const
{
	return m_impl->GetAABB ();
}

bool SceneBounds::Overlaps (const Vec3 & pt) const
{
	return m_impl->Overlaps (pt);
}

bool SceneBounds::Overlaps (const Vec3 & a, const Vec3 & b, const Vec3 & c) const
{
	return m_impl->Overlaps (a, b, c);
}

bool SceneBounds::Overlaps (const std::vector <::Vec3> & polygon) const
{
	return m_impl->Overlaps (polygon);
}


} // namespace LayeredNavMesh


// TODO Mai 14, 2008: <pvl> probably temporary - there's a function for this
// in the 3D engine but it's on Main so I can't access it ATM.  On the other
// hand, the 3D engine version isn't generic enough to be able to support
// LayeredNavMesh::DynArray so using it will require addition data copy.
// Need to find out how much of a slowdown it would mean.
void GenerateMeshFromHeightmap (const LayeredNavMesh::SceneBounds * sceneBounds,
			LayeredNavMesh::DynArray<LayeredNavMesh::Vec3> & arrVerts,
			LayeredNavMesh::DynArray<unsigned int> & arrIndices )
{
	const Vec3 sceneMin = ConvertVec3 (sceneBounds->GetAABB().Min());
	const Vec3 sceneMax = ConvertVec3 (sceneBounds->GetAABB().Max());

	const int nUnitSize = GetIEditor()->Get3DEngine()->GetHeightMapUnitSize();

	const int nMinX = ((int )sceneMin.x / nUnitSize) * nUnitSize;
	const int nMinY = ((int )sceneMin.y / nUnitSize) * nUnitSize;
	const int nMaxX = ((int )sceneMax.x / nUnitSize) * nUnitSize;
	const int nMaxY = ((int )sceneMax.y / nUnitSize) * nUnitSize;

	for(int x = nMinX; x <= nMaxX; x += nUnitSize)
	{
		for(int y = nMinY; y <= nMaxY; y += nUnitSize)
		{
			Vec3 vTmp(x,y,GetIEditor()->Get3DEngine()->GetTerrainElevation(x,y)+0.01f);
			arrVerts.Push(LayeredNavMesh::Vec3 (vTmp.x, vTmp.y, vTmp.z));
		}
	}

	const CHeightmap * terrain = GetIEditor()->GetHeightmap();

	const int nMinXTerrUnits = nMinX/nUnitSize;
	const int nMinYTerrUnits = nMinY/nUnitSize;

	// make indices
	const int dx = (nMaxX-nMinX)/nUnitSize + 1;
	const int dy = (nMaxY-nMinY)/nUnitSize + 1;

	for(int x = 0; x < dx-1; ++x)
	{
		for(int y = 0; y < dy-1; ++y)
		{
			if (terrain->IsHoleAt (nMinYTerrUnits + y, nMinXTerrUnits + x))
				continue;

			int nIdx0 = x*dy + y;
			int nIdx1 = (x+1)*dy + y;
			int nIdx2 = x*dy + y+1;
			int nIdx3 = (x+1)*dy + y+1;

			arrIndices.Push(nIdx0);
			arrIndices.Push(nIdx1);
			arrIndices.Push(nIdx2);

			arrIndices.Push(nIdx1);
			arrIndices.Push(nIdx3);
			arrIndices.Push(nIdx2);
		}
	}
}

// NOTE Mai 21, 2008: <pvl> scene bounds are expected in CE coords (z is up)
static bool ConvertTerrain (
			LayeredNavMesh::DynArray<GeomMesh*> & meshes,
			LayeredNavMesh::DynArray<GeomInstance*> & instances,
			const LayeredNavMesh::SceneBounds * sceneBounds)
{
	LayeredNavMesh::DynArray<LayeredNavMesh::Vec3> vertices;
	LayeredNavMesh::DynArray<unsigned int> indices;
	GenerateMeshFromHeightmap (sceneBounds, vertices, indices);

	const GeomMesh * geomMesh = new GeomMesh (vertices, indices);
	meshes.Push (const_cast <GeomMesh*> (geomMesh));

	// NOTE Mai 16, 2008: <pvl> terrain has no xform in particular, it's just
	// identity shuffled around so that y-axis points up as LNM preprocessing expects
	LayeredNavMesh::Matrix4 xform (
		0, 0, 1, 0,
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	);
	instances.Push (new GeomInstance (geomMesh, xform));

	return true;
}



/**
 * Iterates over parts of physical entities.
 *
 * You feed the ctor a physical entity pointer and then retrieve parts using
 * dereference operator.
 *
 * Parts are represented by their pe_status_pos structures here.  This might
 * not be the best/most generic representation but with physics API being
 * complex as it is I know of nothing better.  In any case, pe_status_pos
 * fits the present purpose nicely.
 */
class PartIterator {
	/// The entity whose parts we iterate over.
	IPhysicalEntity * m_entity;

	/// Number of parts m_entity has.
	int m_nrParts;

	/// Index of the current part, always between zero (incl.) and m_nrParts.
	int m_currPartIndex;

	/// The part corresponding to m_currPartIndex.
	pe_status_pos m_part;

	void GetNextPart ();
public:
	PartIterator (IPhysicalEntity * );

	/// Dereference - returns current part.
	const pe_status_pos & operator* () const;

	PartIterator & operator++ ();

	/// 'true' if there are more parts.
	operator bool () const;
};

PartIterator::PartIterator (IPhysicalEntity * entity) :
		m_entity (entity), m_currPartIndex (0)
{
	pe_status_nparts dummy;
	m_nrParts = m_entity->GetStatus ( & dummy);

	GetNextPart ();
}

/**
 * Gets m_part corresponding to m_currPartIndex.  If there's an error it tries
 * to get higher numbered parts until it succeeds or there's no more parts.
 */
void PartIterator::GetNextPart ()
{
	m_part.ipart = m_currPartIndex;

	for ( ; *this; m_part.ipart = ++m_currPartIndex)
	{
		if ( ! m_entity->GetStatus ( & m_part))
			continue;

		pe_params_part paramsPart;
		paramsPart.ipart = m_currPartIndex;
		if (0 == m_entity->GetParams ( & paramsPart))
			break;
		if ( ! (paramsPart.flagsAND & geom_colltype_player) )
			continue;

		break;
	}
}

const pe_status_pos & PartIterator::operator* () const
{
	return m_part;
}

PartIterator & PartIterator::operator++ ()
{
	++m_currPartIndex;

	GetNextPart ();

	return *this;
}

PartIterator::operator bool () const
{
	return m_entity && m_currPartIndex < m_nrParts;
}


/**
 * Iterates over physical entities with specified flags within a specified box.
 */
class EntityIterator {

	/// Number of physical entities in the box passed over to this class' ctor.
	int m_nrEntities;

	/// Index of the current entity, always between zero (incl.) and m_nrEntities.
	int m_currEntityIndex;

	/// The actual entities (more precisely, an array of entity pointers).
	IPhysicalEntity ** m_entities;

public:
	EntityIterator (const Vec3 & sceneMin, const Vec3 & sceneMax, int entityFlags);

	IPhysicalEntity * operator* () const;
	EntityIterator & operator++ ();
	operator bool () const;
};

EntityIterator::EntityIterator (const Vec3 & sceneMin, const Vec3 & sceneMax, int entityFlags) :
		m_nrEntities(0), m_currEntityIndex (0), m_entities (0)
{
	ISystem * pSystem = GetIEditor()->GetSystem();
	m_nrEntities = pSystem->GetIPhysicalWorld()->GetEntitiesInBox (
			sceneMin, sceneMax, m_entities, entityFlags
	);
}

IPhysicalEntity * EntityIterator::operator* () const
{
	return m_entities [m_currEntityIndex];
}

EntityIterator & EntityIterator::operator++ ()
{
	++m_currEntityIndex;
	return *this;
}

EntityIterator::operator bool () const
{
	return m_currEntityIndex < m_nrEntities;
}




/**
 * Gets a physical entity (or its part) and converts its geometry to LNM format.
 *
 * The part is represented as pe_status_pos and passed over to the ctor.
 * The constructor also receives a IPhysicalEntity* which is only needed for
 * solids and potentially other stuff for which there's no mesh_data.
 * Results (GeomMesh and GeomInstance) can then be retrieved by calling
 * GetGeomMesh() and GetGeomInstance().
 *
 * Implements a simple caching scheme to avoid converting the same mesh
 * repeatedly in case it has multiple instances.  Caching simply associates
 * source mesh_data* with destination GeomMesh* so its necessary to flush
 * the cache by calling Init() whenever there's a danger that a mesh_data*
 * from an upcoming batch could have the same value as one of the already
 * cached ones but be in fact different.
 */
class MeshDataConvertor {
	GeomMesh * m_mesh;
	GeomInstance * m_instance;

	bool m_conversionOK;

	void ComputeMesh (IPhysicalEntity * , const pe_status_pos & );
	/// Converts anything whose IGeometry is of type GEOM_TRIMESH.
	void ComputeMesh (const mesh_data * );
	/// Converts stuff that doesn't boil down to mesh_data (like solids).
	void ComputeMesh (IIndexedMesh * );
	/// Converts primitive boxes.
	void ComputeBoxMesh (const primitives::box & box);
	// NOTE Feb 25, 2010: <pvl> the scale factor passed into sphere and cylinder
	// functions is NOT used to actually transform vertex positions (that needs
	// to happen only later when the transform stored in status_pos is applied).  It's
	// only used to give the functions an idea of the eventual size of the primitive
	// in order to figure out an appropriate mesh resolution.
	/// Converts primitive spheres.
	void ComputeSphereMesh (const Vec3 & centre, float radius, float scale, float tol);
	/// Converts primitive cylinders.
	void ComputeCylinderMesh (const primitives::cylinder & , float scale, float tol);

	void ComputeInstance (const pe_status_pos & );

	class GeomMeshCache {
		std::map <const mesh_data*, const GeomMesh*> m_meshes;
	public:
		void Flush ();
		void Insert (const mesh_data * , const GeomMesh * );
		GeomMesh * Retrieve (const mesh_data * ) const;
	};

	static GeomMeshCache s_cache;
public:
	/// Flushes the cache, needs to be called before converting a batch of meshes.
	static void Init ();

	MeshDataConvertor (IPhysicalEntity * , const pe_status_pos & );

	GeomMesh * GetGeomMesh () const;
	GeomInstance * GetGeomInstance () const;

	bool ConversionSuccessful () const;
};

MeshDataConvertor::GeomMeshCache MeshDataConvertor::s_cache;


inline void MeshDataConvertor::GeomMeshCache::Flush ()
{
	m_meshes.erase (m_meshes.begin (), m_meshes.end ());
}

inline void MeshDataConvertor::GeomMeshCache::Insert (
			const mesh_data * meshData, const GeomMesh * geomMesh)
{
	m_meshes.insert (std::make_pair (meshData, geomMesh));
}

inline GeomMesh * MeshDataConvertor::GeomMeshCache::Retrieve (const mesh_data * meshData) const
{
	std::map <const mesh_data*, const GeomMesh*>::const_iterator geomMeshIt = m_meshes.find (meshData);
	if (geomMeshIt == m_meshes.end ())
		return 0;
	return const_cast <GeomMesh*> (geomMeshIt->second);
}

void MeshDataConvertor::Init ()
{
	s_cache.Flush ();
}

MeshDataConvertor::MeshDataConvertor (IPhysicalEntity * entity, const pe_status_pos & statusPos) :
		m_mesh (0), m_instance (0), m_conversionOK (true)
{
	ComputeMesh (entity, statusPos);
	ComputeInstance (statusPos);
}

/// Converts the triangles and caches the result if not found in the cache.
void MeshDataConvertor::ComputeMesh (IPhysicalEntity * entity, const pe_status_pos & statusPos)
{
	IGeometry * geom = statusPos.pGeom;

	IRenderNode * renderNode = (IRenderNode*)entity->GetForeignData (PHYS_FOREIGN_ID_STATIC);

	// NOTE Jan 27, 2010: <pvl> overload the old "exclude from triangulation"
	// flag to exclude stuff from the LNM as well
	if (renderNode && renderNode->GetRndFlags() & ERF_EXCLUDE_FROM_TRIANGULATION)
	{
		m_conversionOK = false;
		return;
	}

	if (geom->GetType () == GEOM_TRIMESH)
	{
		const mesh_data * meshData = static_cast<const mesh_data *> (geom->GetData());
		ComputeMesh (meshData);
	}
	else if (geom->GetType () == GEOM_BOX)
	{
		using primitives::box;
		const box * b = static_cast<const box *> (geom->GetData());
		ComputeBoxMesh (*b);
	}
	else if (geom->GetType () == GEOM_SPHERE)
	{
		using primitives::sphere;
		const sphere * ball = static_cast<const sphere *> (geom->GetData());
		ComputeSphereMesh (ball->center, ball->r, statusPos.scale, 0.01f);
	}
	else if (geom->GetType () == GEOM_CYLINDER)
	{
		using primitives::cylinder;
		const cylinder * cyl = static_cast<const cylinder *> (geom->GetData());
		ComputeCylinderMesh (*cyl, statusPos.scale, 0.01f);
	}
	else
	{
		printf ("WARNING: unhandled physical proxy type %d\n", geom->GetType ());

		IStatObj * statObj = renderNode->GetEntityStatObj (statusPos.partid);
		if (statObj == 0)
		{
			m_conversionOK = false;
			return;
		}

		IIndexedMesh * indexedMesh = statObj->GetIndexedMesh ();
		if (indexedMesh == 0)
		{
			m_conversionOK = false;
			return;
		}
		else
		{
			ComputeMesh (indexedMesh);
		}
	}









}

void MeshDataConvertor::ComputeMesh (const mesh_data * meshData)
{
	m_mesh = s_cache.Retrieve (meshData);
	if (m_mesh == 0)
	{
		LayeredNavMesh::DynArray<LayeredNavMesh::Vec3> vertices;
		for (int i=0; i < meshData->nVertices; ++i)
			vertices.Push (LayeredNavMesh::Vec3 (
					meshData->pVertices[i].x, meshData->pVertices[i].y, meshData->pVertices[i].z)
			); 

		LayeredNavMesh::DynArray<unsigned int> indices;
		for (int i=0; i < 3 * meshData->nTris; ++i)
			indices.Push (meshData->pIndices[i]);

		m_mesh = new GeomMesh (vertices, indices);
		s_cache.Insert (meshData, m_mesh);
	}
}

/**
 * Used for converting non-GEOM_TRIMESH stuff (mostly solids AFAIK).
 *
 * Doesn't cache the result since solids don't (usually?) have multiple
 * instances.
 */
void MeshDataConvertor::ComputeMesh (IIndexedMesh * mesh)
{
	IIndexedMesh::SMeshDesc meshDesc;
	mesh->GetMesh (meshDesc);

	LayeredNavMesh::DynArray<LayeredNavMesh::Vec3> vertices;
	for (int i=0; i < meshDesc.m_nVertCount; ++i)
		vertices.Push (LayeredNavMesh::Vec3 (
				meshDesc.m_pVerts[i].x, meshDesc.m_pVerts[i].y, meshDesc.m_pVerts[i].z)
		); 

	LayeredNavMesh::DynArray<unsigned int> indices;
	for (int i=0; i < meshDesc.m_nIndexCount; ++i)
		indices.Push (meshDesc.m_pIndices[i]);

	m_mesh = new GeomMesh (vertices, indices);
}

void MeshDataConvertor::ComputeBoxMesh (const primitives::box & box)
{
	std::vector<Vec3> localVtxs;

	localVtxs.push_back (Vec3 (-box.size.x, -box.size.y, -box.size.z));
	localVtxs.push_back (Vec3 ( box.size.x, -box.size.y, -box.size.z));
	localVtxs.push_back (Vec3 ( box.size.x,  box.size.y, -box.size.z));
	localVtxs.push_back (Vec3 (-box.size.x,  box.size.y, -box.size.z));

	localVtxs.push_back (Vec3 (-box.size.x, -box.size.y,  box.size.z));
	localVtxs.push_back (Vec3 ( box.size.x, -box.size.y,  box.size.z));
	localVtxs.push_back (Vec3 ( box.size.x,  box.size.y,  box.size.z));
	localVtxs.push_back (Vec3 (-box.size.x,  box.size.y,  box.size.z));

	const size_t nVertices = localVtxs.size();


	//	Face vertices have the following indices:
	//
	//       7-----6
	//      /|    /|
	//     4-+---5 |
	//     | 3---+-2
	//     |/    |/
	//     0-----1


	const unsigned int idxs[] = {
		2, 1, 0, 0, 3, 2,		// bottom
		3, 0, 7, 0, 4, 7,		// left
		0, 1, 5, 0, 5, 4,		// front
		1, 2, 5, 6, 5, 2,		// right
		7, 2, 3, 7, 6, 2,		// rear,
		7, 4, 5, 7, 5, 6		// top
	};

	LayeredNavMesh::DynArray<unsigned int> indices;
	indices.Append (idxs, sizeof (idxs) / sizeof (idxs[0]));
	
	const Vec3 localTranslation = box.center;
	const Matrix33 localRotation = box.Basis.GetTransposed();

	LayeredNavMesh::DynArray<LayeredNavMesh::Vec3> vertices;
	for (int i = 0; i < nVertices; ++i)
	{
		Vec3 v = localVtxs[i];
		if (box.bOriented)
			v = localRotation * v;
		v += localTranslation;

		vertices.Push (LayeredNavMesh::Vec3 (v.x, v.y, v.z)); 
	}

	m_mesh = new GeomMesh (vertices, indices);
}

void MeshDataConvertor::ComputeSphereMesh (const Vec3 & centre, float radius, float scale, float tol)
{
	// TODO Feb 23, 2010: <pvl> the attempt at adapting the subdivision granularity
	// to the actual size of the primitive will fail if the transform is stored
	// in status_pos which we don't get to see here

	float dAngle = 2 * acosf (1 - tol/(scale*radius));
	dAngle = M_PI / ceilf (M_PI / dAngle);

	unsigned nLat = M_PI / dAngle + 0.5f + 1;
	unsigned nLong = 2*M_PI / dAngle + 0.5f;

	if (nLat < 3) nLat = 3;
	if (nLong < 3) nLong = 3;

	LayeredNavMesh::DynArray<LayeredNavMesh::Vec3> vertices;

	// NOTE Feb 23, 2010: <pvl> ignore the zeroth and last latitude which are
	// trivial and consist only of south and north pole, respectively
	nLat -= 2;

	for (unsigned lat=0; lat < nLat; ++lat)
	{
		const float latAngle = -M_PI/2.0f + (lat+1)*dAngle;
		// NOTE Feb 23, 2010: <pvl> also could pre-multiply by 'radius' here but
		// I'm not sure whether it's worth the loss of clarity
		const float cosLat = cosf (latAngle);
		const float sinLat = sinf (latAngle);
		for (unsigned lng=0; lng < nLong; ++lng)
		{
			const float longAngle = lng*dAngle;
			LayeredNavMesh::Vec3 v (
					radius * cosLat * cosf (longAngle) + centre.x,
					radius * cosLat * sinf (longAngle) + centre.y,
					radius * sinLat + centre.z
			);
			vertices.Push (v);
		}
	}

	// NOTE Feb 22, 2010: <pvl> poles
	vertices.Push (LayeredNavMesh::Vec3 (0.0f, 0.0f, -1.0f));
	vertices.Push (LayeredNavMesh::Vec3 (0.0f, 0.0f, 1.0f));

	LayeredNavMesh::DynArray<unsigned int> indices;
	// NOTE Feb 22, 2010: <pvl> deal with quad faces
	for (unsigned lat=0; lat < nLat-1; ++lat)
	{
		for (unsigned lng=0, prevLng=nLong-1; lng < nLong; prevLng=lng, ++lng)
		{
			indices.Push (lat*nLong + prevLng);
			indices.Push (lat*nLong + lng);
			indices.Push ((lat+1)*nLong + lng);

			indices.Push (lat*nLong + prevLng);
			indices.Push ((lat+1)*nLong + lng);
			indices.Push ((lat+1)*nLong + prevLng);
		}
	}

	// NOTE Feb 23, 2010: <pvl> triangular faces surrounding poles
	const unsigned southPoleIdx = vertices.Size()-2;
	for (unsigned lng=0, prevLng=nLong-1; lng < nLong; prevLng=lng, ++lng)
	{
		indices.Push (lng);
		indices.Push (prevLng);
		indices.Push (southPoleIdx);
	}

	const unsigned northPoleIdx = vertices.Size()-1;
	for (unsigned lng=0, prevLng=nLong-1; lng < nLong; prevLng=lng, ++lng)
	{
		indices.Push ((nLat-1)*nLong + prevLng);
		indices.Push ((nLat-1)*nLong + lng);
		indices.Push (northPoleIdx);
	}

	m_mesh = new GeomMesh (vertices, indices);
}

void MeshDataConvertor::ComputeCylinderMesh (const primitives::cylinder & cyl, float scale, float tol)
{
	// TODO Feb 23, 2010: <pvl> the attempt at adapting the subdivision granularity
	// to the actual size of the primitive will fail if the transform is stored
	// in status_pos which we don't get to see here

	float dAzimuth = 2 * acosf (1 - tol/(scale*cyl.r));
	dAzimuth = M_PI / ceilf (M_PI / dAzimuth);

	unsigned nSections = 2*M_PI / dAzimuth + 0.5f;
	if (nSections < 6) nSections = 6;

	const Vec3 rotAxis (Vec3 (0.0f, 0.0f, 1.0f).Cross (cyl.axis));
	Matrix33 rot;
	rot.SetRotationAA (acosf (Vec3 (0.0f, 0.0f, 1.0f) * cyl.axis) , rotAxis);

	LayeredNavMesh::DynArray<LayeredNavMesh::Vec3> vertices;
	for (unsigned s=0; s < nSections; ++s)
	{
		const float azimuth = s * dAzimuth;
		const float x = cyl.r*cosf (azimuth) + cyl.center.x;
		const float y = cyl.r*sinf (azimuth) + cyl.center.y;
		const Vec3 upperVtx (rot * Vec3 (x, y, cyl.center.z - cyl.hh));
		vertices.Push (LayeredNavMesh::Vec3 (upperVtx.x, upperVtx.y, upperVtx.z));
		const Vec3 lowerVtx (rot * Vec3 (x, y, cyl.center.z + cyl.hh));
		vertices.Push (LayeredNavMesh::Vec3 (lowerVtx.x, lowerVtx.y, lowerVtx.z));
	}

	LayeredNavMesh::DynArray<unsigned int> indices;
	for (unsigned s=0, prevSect=nSections-1; s < nSections; prevSect=s, ++s)
	{
		indices.Push (2*prevSect);
		indices.Push (2*s);
		indices.Push (2*prevSect+1);

		indices.Push (2*s);
		indices.Push (2*s+1);
		indices.Push (2*prevSect+1);
	}

	// NOTE Feb 23, 2010: <pvl> top and bottom parts
	for (int s=0; s < nSections-1; ++s)
	{
		indices.Push (2*(s+1));
		indices.Push (2*s);
		indices.Push (0);

		indices.Push (2*s + 1);
		indices.Push (2*(s+1) + 1);
		indices.Push (0 + 1);
	}

	m_mesh = new GeomMesh (vertices, indices);
}


/// Mainly just converts an instance's transformation.
void MeshDataConvertor::ComputeInstance (const pe_status_pos & statusPos)
{
	// NOTE Aug 4, 2008: <pvl> if conversion hasn't gone well already (meaning
	// probably that we weren't even able to convert the mesh) there's no point
	// in trying to compute the instance transformation
	if ( ! m_conversionOK)
		return;

	Vec3 partPos = statusPos.pos;
	Matrix33 partMat = Matrix33(statusPos.q);
	partMat *= statusPos.scale;

	// NOTE Mai 8, 2008: <pvl> LNM preprocessor expects y to be the vertical
	// direction - rotate base vectors (x,y,z)->(y,z,x)
	LayeredNavMesh::Matrix4 xform (
		partMat.m10, partMat.m20, partMat.m00, 0,
		partMat.m11, partMat.m21, partMat.m01, 0,
		partMat.m12, partMat.m22, partMat.m02, 0,
		partPos.y, partPos.z, partPos.x, 1
	);

	// NOTE Mai 15, 2008: <pvl> m_mesh must have already been taken care of by
	// calling ComputeMesh() previously - assert that
	assert (m_mesh);

	m_instance = new GeomInstance (m_mesh, xform);
}

inline GeomMesh * MeshDataConvertor::GetGeomMesh () const
{
	return m_mesh;
}	

inline GeomInstance * MeshDataConvertor::GetGeomInstance () const
{
	return m_instance;
}

inline bool MeshDataConvertor::ConversionSuccessful () const
{
	return m_conversionOK;
}


static bool Overlaps (const LayeredNavMesh::SceneBounds * sceneBounds, const pe_status_pos & statusPos)
{
	// NOTE Feb 20, 2009: <pvl> construct bounding box from 'statusPos'
	primitives::box bbox;
	statusPos.pGeomProxy->GetBBox(&bbox);

	bbox.center *= statusPos.scale;
	bbox.size   *= statusPos.scale;

	::Vec3 worldC = statusPos.pos + statusPos.q * bbox.center;
	Matrix33 worldM = Matrix33(statusPos.q) * bbox.Basis.GetTransposed();

	std::vector<::Vec3> world_bbox;

	// NOTE Feb 20, 2009: <pvl> is this guaranteed to work if 'world_bbox' is
	// used as a polygon?  If we're not lucky and 'bbox' doesn't have it xform
	// reset then 'world_box' could end up with something with odd connectivity,
	// bad winding, holes, self-intersections etc.?
	world_bbox.push_back(worldC + worldM * ::Vec3(-bbox.size.x, -bbox.size.y, -bbox.size.z));
	world_bbox.push_back(worldC + worldM * ::Vec3( bbox.size.x, -bbox.size.y, -bbox.size.z));
	world_bbox.push_back(worldC + worldM * ::Vec3( bbox.size.x,  bbox.size.y, -bbox.size.z));
	world_bbox.push_back(worldC + worldM * ::Vec3(-bbox.size.x,  bbox.size.y, -bbox.size.z));

	world_bbox.push_back(worldC + worldM * ::Vec3(-bbox.size.x, -bbox.size.y,  bbox.size.z));
	world_bbox.push_back(worldC + worldM * ::Vec3( bbox.size.x, -bbox.size.y,  bbox.size.z));
	world_bbox.push_back(worldC + worldM * ::Vec3( bbox.size.x,  bbox.size.y,  bbox.size.z));
	world_bbox.push_back(worldC + worldM * ::Vec3(-bbox.size.x,  bbox.size.y,  bbox.size.z));

	// (evgeny 23 Jan 2009) Check if the Z-axis projection of the bounding box
	// is within the Z-axis projection of the Scene Bounds
	// UPDATE Feb 20, 2009: <pvl> simply, check vertical dimension
	
	float minZ = world_bbox[0].z;
	float maxZ = minZ;

	for (unsigned int i = 0, n = world_bbox.size(); i < n; i++)
	{
		if (world_bbox[i].z > maxZ)
			maxZ = world_bbox[i].z;
		if (world_bbox[i].z < minZ)
			minZ = world_bbox[i].z;
	}

	// (evgeny 23 Jan 2009) Note the mix of engine Vec3 and LNM Vec3 coordinates
	if ((minZ > sceneBounds->GetAABB().Max().y)  || (maxZ < sceneBounds->GetAABB().Min().y))
		return false;

	return sceneBounds->Overlaps (world_bbox);
}

#include "../../../Objects/ShapeObject.h"

static bool ConvertGeometry (
			 LayeredNavMesh::DynArray<GeomMesh*> & meshes,
			 LayeredNavMesh::DynArray<GeomInstance*> & instances,
			 const LayeredNavMesh::SceneBounds * sceneBounds)
{
	Stopwatch watch;
	watch.Start ();

	// flush the mesh cache
	MeshDataConvertor::Init ();

	// iterate over all parts of all suitable entities within a box and convert
	// trimeshes to LNM format
	EntityIterator entityIt (
			ConvertVec3 (sceneBounds->GetAABB().Min()),
			ConvertVec3 (sceneBounds->GetAABB().Max()),
			ent_static|ent_ignore_noncolliding
	);
	for ( ; entityIt; ++entityIt)
	{
		PartIterator partIt (*entityIt);
		for ( ; partIt; ++partIt)
		{
			const pe_status_pos & statusPos = *partIt;
			IGeometry * geom = statusPos.pGeom;

			if ( ! Overlaps (sceneBounds, statusPos))
				continue;

			MeshDataConvertor convertor ( *entityIt, statusPos);
			if (convertor.ConversionSuccessful ())
			{
				meshes.Push (convertor.GetGeomMesh ());
				instances.Push (convertor.GetGeomInstance ());
			}
		}
	}
	watch.Stop ();
	AILogAlways ("Geometry conversion to LNM format: %.3fms", 1000.0 * watch.GetElapsed ());

#if defined (USER_pavel)
	// NOTE Jul 28, 2009: <pvl> debugging only - make it possible to voxelize
	// and preprocess a special-named shape (i.e. treat a shape as geometry)
	CBaseObject * obj = GetIEditor()->GetObjectManager()->FindObject ("chrst");
	if (obj)
	{
		ObjectType type = obj->GetType ();
		assert (type == OBJTYPE_SHAPE);
		CShapeObject * shapeObj = (CShapeObject * )obj;

		const Matrix34 tm = obj->GetWorldTM ();
		
		LayeredNavMesh::DynArray<LayeredNavMesh::Vec3> vertices;
		LayeredNavMesh::DynArray<unsigned int> indices;
		for (int vtx=0, numVtxs=shapeObj->GetPointCount(); vtx < numVtxs; ++vtx)
		{
			const ::Vec3 xformd = tm.TransformPoint (shapeObj->GetPoint (vtx));
			vertices.Push (LayeredNavMesh::Vec3 (xformd.x, xformd.y, xformd.z)); 
			indices.Push (vtx);
		}
		GeomMesh * geomMesh = new GeomMesh (vertices, indices);
		meshes.Push (geomMesh);

		LayeredNavMesh::Matrix4 xform (
			0, 0, 1, 0,
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 0, 1
		);
		instances.Push (new GeomInstance (geomMesh, xform));
	}
#endif // defined (USER_pavel)

	return true;
}


// ---

namespace LayeredNavMesh {

class SpanBufferChunkWriter : public ExportFormat::ChunkWriter {
	const SpanBuffer * m_spanBuffer;

	virtual bool WriteChunkData (CCryFile & ) const;
public:
	SpanBufferChunkWriter (const SpanBuffer * spanBuffer) :
			ExportFormat::ChunkWriter (LnmExportFormat::LnmChunkType::NAV_MESH_SPAN_BUFFER_ASCII),
			m_spanBuffer (spanBuffer)
	{ }
};

bool SpanBufferChunkWriter::WriteChunkData (CCryFile & file) const
{
	string line;
	line.reserve (1024);

	bool ok = true;

	for (unsigned x=0, maxx = m_spanBuffer->GetWidth (); x < maxx; ++x)
		for (unsigned z=0, maxz = m_spanBuffer->GetHeight (); z < maxz; ++z)
		{
			const SpanBuffer::Span * span = m_spanBuffer->GetSpans (x, z);
			if (span == 0) continue;

			// NOTE Aug 26, 2009: <pvl> keep in mind if anything goes wrong in
			// Format() we won't ever find out
			line.resize (0);
			line.Format ("(%d,%d):", x, z);

			for ( ; span; span = span->next)
				line += string().Format (" %#x %d-%d", span->flags, span->smin, span->smax);

			line += string().Format ("\n");

			bool lineWrittenOk = (0 != file.Write (line.c_str (), line.length ()));
			ok = ok && lineWrittenOk;
		}

	return ok;
}

class BinSpanBufferChunkWriter : public ExportFormat::VersionedChunkWriter {
	const SpanBuffer * m_spanBuffer;

	virtual bool WriteChunkData (CCryFile & ) const;
public:
	BinSpanBufferChunkWriter (const SpanBuffer * spanBuffer, uint16 version) :
			ExportFormat::VersionedChunkWriter (LnmExportFormat::LnmChunkType::NAV_MESH_SPAN_BUFFER_BIN, version),
			m_spanBuffer (spanBuffer)
	{ }
};

bool BinSpanBufferChunkWriter::WriteChunkData (CCryFile & file) const
{
	const size_t spanCnt =  m_spanBuffer->GetSpanCount();
	file.Write ( & spanCnt, sizeof (spanCnt) );

	unsigned numOccupiedCells = 0;
	for (unsigned x=0, maxx = m_spanBuffer->GetWidth (); x < maxx; ++x)
		for (unsigned z=0, maxz = m_spanBuffer->GetHeight (); z < maxz; ++z)
		{
			const SpanBuffer::Span * span = m_spanBuffer->GetSpans (x, z);
			if (span == 0) continue;

			++numOccupiedCells;

			for ( ; span; span = span->next)
			{
				file.Write ( & span->flags, sizeof (span->flags) );
				file.Write ( & span->smin, sizeof (span->smin) );
				file.Write ( & span->smax, sizeof (span->smax) );
			}
		}

	file.Write ( & numOccupiedCells, sizeof (numOccupiedCells) );

	for (unsigned x=0, maxx = m_spanBuffer->GetWidth (); x < maxx; ++x)
		for (unsigned z=0, maxz = m_spanBuffer->GetHeight (); z < maxz; ++z)
		{
			const SpanBuffer::Span * span = m_spanBuffer->GetSpans (x, z);
			if (span == 0) continue;

			unsigned short numSpans = 0;
			for (const SpanBuffer::Span * s = span; s; s=s->next, ++numSpans)
				;

			file.Write ( & x, sizeof (x) );
			file.Write ( & z, sizeof (z) );
			file.Write ( & numSpans, sizeof (numSpans) );

			--numOccupiedCells;
		}

	assert (numOccupiedCells == 0);

	const int spanMin = m_spanBuffer->GetSpanMin ();
	file.Write ( & spanMin, sizeof (spanMin) );
	const int spanMax = m_spanBuffer->GetSpanMax ();
	file.Write ( & spanMax, sizeof (spanMax) );

	// NOTE Jan 29, 2010: <pvl> this is just a debug dump, don't disrupt the whole
	// file writing operation even if this fails
	return true;
}

class BinSpanBufferChunkReader : public ExportFormat::VersionedChunkReader {
	SpanBuffer * m_spanBuffer;

public:
	BinSpanBufferChunkReader (SpanBuffer * spanBuffer) :
			m_spanBuffer (spanBuffer)
	{ }
	bool ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & );
};

bool BinSpanBufferChunkReader::ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & file)
{
	if (file.IsEof ())
		return false;

	size_t spanCnt;
	file.ReadType ( & spanCnt );

#pragma pack (push, 1)
	struct CellHdr {
		unsigned x, z;
		unsigned short numSpans;
	};
	struct SpanRec {
		unsigned char flags;
		int smin, smax;
	};
#pragma pack (pop)

	SpanRec * spns (new SpanRec[spanCnt]);
	file.ReadRaw (spns, spanCnt * sizeof (SpanRec));

	unsigned cellCnt;
	file.ReadType ( & cellCnt );

	CellHdr * hdrs (new CellHdr[cellCnt]);
	file.ReadRaw (hdrs, cellCnt * sizeof (CellHdr));

	unsigned spanIdx = 0;
	for (unsigned ss=0; ss < cellCnt; ++ss)
	{
		SpanBuffer::Span * lowest = 0;
		SpanBuffer::Span * prev = 0;
		for (unsigned s=0; s < hdrs[ss].numSpans; ++s, ++spanIdx)
		{
			SpanBuffer::Span * span = m_spanBuffer->AllocSpan ();
			if (s==0) lowest = span;

			span->flags = spns[spanIdx].flags;
			span->smin = spns[spanIdx].smin;
			span->smax = spns[spanIdx].smax;

			span->next = 0;

			if (prev)
				prev->next = span;
			prev = span;
		}
		assert (lowest);
		m_spanBuffer->AddSpanChain (hdrs[ss].x, hdrs[ss].z, lowest);
	}

	delete [] spns;
	delete [] hdrs;

	int spanMin;
	file.ReadType ( & spanMin );
	m_spanBuffer->SetSpanMin (spanMin);

	int spanMax;
	file.ReadType ( & spanMax );
	m_spanBuffer->SetSpanMax (spanMax);

	// TODO Jan 29, 2010: <pvl> ????
	return true;
}


class BinNavSurfaceElementChunkWriter : public ExportFormat::VersionedChunkWriter {
	const NavSurfaceGenerator * m_surfaceGen;

	virtual bool WriteChunkData (CCryFile & ) const;
public:
	BinNavSurfaceElementChunkWriter (const NavSurfaceGenerator * surfaceGen, uint16 version) :
			ExportFormat::VersionedChunkWriter (LnmExportFormat::LnmChunkType::NAV_MESH_NAV_SURF_ELEMENTS_BIN, version),
			m_surfaceGen (surfaceGen)
	{ }
};

bool BinNavSurfaceElementChunkWriter::WriteChunkData (CCryFile & file) const
{
	string line;
	line.reserve (1024);

	bool ok = true;

	// NOTE Jan 29, 2010: <pvl> cache this as an optimisation
	const LayeredNavMesh::Cell * baseCell = & m_surfaceGen->GetCell (0);
	const unsigned invalidCellIdx = std::numeric_limits<unsigned>::max();

	const unsigned cellCount = m_surfaceGen->GetCellCount();
	file.Write ( & cellCount, sizeof (cellCount) );

	for (unsigned c=0, nc=cellCount; c < nc; ++c)
	{
		const LayeredNavMesh::Cell & cell = m_surfaceGen->GetCell (c);

		for (unsigned n=0; n<4; ++n)
		{
			const unsigned con = cell.con[n] ? cell.con[n] - baseCell : invalidCellIdx;
			file.Write ( & con, sizeof (con) );
		}

		file.Write ( & cell.region, sizeof (cell.region) );

		file.Write ( & cell.x, sizeof (cell.x) );
		file.Write ( & cell.y, sizeof (cell.y) );
		file.Write ( & cell.z, sizeof (cell.z) );

		file.Write ( & cell.flags, sizeof (cell.flags) );
		file.Write ( & cell.data, sizeof (cell.data) );
		file.Write ( & cell.userData, sizeof (cell.userData) );
	}

	return ok;
}

class BinNavSurfaceElementChunkReader : public ExportFormat::VersionedChunkReader {
	NavSurfaceGenerator * m_surfaceGen;

public:
	BinNavSurfaceElementChunkReader (NavSurfaceGenerator * surfaceGen) :
			m_surfaceGen (surfaceGen)
	{ }
	bool ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & );
};

bool BinNavSurfaceElementChunkReader::ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & file)
{
	if (file.IsEof ())
		return false;

	unsigned cellCount;
	file.ReadType ( & cellCount );

	LayeredNavMesh::DynArray<LayeredNavMesh::Cell> cells;
	cells.Resize (cellCount);

	LayeredNavMesh::Cell * baseCell = & cells[0];
	const unsigned invalidCellIdx = std::numeric_limits<unsigned>::max();

#pragma pack (push, 1)
	struct CellRec {
		// NOTE Feb 18, 2010: <pvl> would be great from maintenance POV if
		// we could use the Cell struct directly here, however it's not packed
		// and the packing pragma here obviously can't affect it
		//LayeredNavMesh::Cell m_cell;
		unsigned con[4];
		unsigned int region;
		short x, y, z;
		unsigned char flags;
		unsigned char data;
		unsigned char userData;
	};
#pragma pack (pop)

	CellRec * cellRecs (new CellRec[cellCount]);
	file.ReadRaw (cellRecs, cellCount * sizeof (CellRec) );

	for (unsigned c=0; c < cellCount; ++c)
	{
		Cell & cell = cells[c];

		for (unsigned n=0; n<4; ++n)
		{
			unsigned con = cellRecs[c].con[n];
			cell.con[n] = con == invalidCellIdx ? 0 : baseCell + con;
		}

		cell.x = cellRecs[c].x;
		cell.y = cellRecs[c].y;
		cell.z = cellRecs[c].z;

		cell.region = cellRecs[c].region;

		cell.flags = cellRecs[c].flags;
		cell.data = cellRecs[c].data;
		cell.userData = cellRecs[c].userData;
	}

	delete [] cellRecs;

	m_surfaceGen->SetCells (cells);

	// TODO Feb 17, 2010: <pvl> ????
	return true;
}



class NavSurfaceElementChunkWriter : public ExportFormat::ChunkWriter {
	const NavSurfaceGenerator * m_surfaceGen;

	virtual bool WriteChunkData (CCryFile & ) const;
public:
	NavSurfaceElementChunkWriter (const NavSurfaceGenerator * surfaceGen) :
			ExportFormat::ChunkWriter (LnmExportFormat::LnmChunkType::NAV_MESH_NAV_SURF_ELEMENTS_ASCII),
			m_surfaceGen (surfaceGen)
	{ }
};

bool NavSurfaceElementChunkWriter::WriteChunkData (CCryFile & file) const
{
	string line;
	line.reserve (1024);

	bool ok = true;

	// NOTE Jan 29, 2010: <pvl> cache this as an optimisation
	const LayeredNavMesh::Cell * baseCell = & m_surfaceGen->GetCell (0);

	for (unsigned c=0, nc=m_surfaceGen->GetCellCount(); c < nc; ++c)
	{
		const LayeredNavMesh::Cell & cell = m_surfaceGen->GetCell (c);

		// NOTE Aug 26, 2009: <pvl> keep in mind if anything goes wrong in
		// Format() we won't ever find out
		line.resize (0);

		line.Format (" neighbours: ( ");
		for (unsigned n=0; n<4; ++n)
			line += string().Format ("%d ", cell.con[n] ? cell.con[n] - baseCell : 0);
		line += string().Format (")");

		line += string().Format (" next_in_hash: %d", cell.next ? cell.next - baseCell : 0);

		line += string().Format (" (%d %d %d)", cell.x, cell.y, cell.z);

		line += string().Format (" reg: %d", cell.region);

		line += string().Format (" %#x %d %d", cell.flags, cell.data, cell.userData);

		line += string().Format ("\n");

		bool lineWrittenOk = (0 != file.Write (line.c_str (), line.length ()));
		ok = ok && lineWrittenOk;
	}

	return ok;
}

class ContourListChunkWriter : public ExportFormat::ChunkWriter {
	const NavContourGenerator * m_contours;

	virtual bool WriteChunkData (CCryFile & ) const;
public:
	ContourListChunkWriter (const NavContourGenerator * contours) :
			ExportFormat::ChunkWriter (LnmExportFormat::LnmChunkType::NAV_MESH_CONTOURS_ASCII),
			m_contours (contours)
	{ }
};

bool ContourListChunkWriter::WriteChunkData (CCryFile & file) const
{
	string line;
	line.reserve (1024);

	bool ok = true;

	for (unsigned c=0, numContours = m_contours->GetContourCount(); c < numContours; ++c)
	{
		const Contour * contour = m_contours->GetContour (c);

		// NOTE Aug 26, 2009: <pvl> keep in mind if anything goes wrong in
		// Format() we won't ever find out
		line.resize (0);

		for (unsigned pt = 0, numPts = contour->points.Size(); pt < numPts; ++pt)
		{
			if (pt % 5 == 0)
				line += string().Format ("\n\t");

			const Pointi& p = contour->points[pt];
			line += string().Format (" (%4d %4d %4d)", p.x, p.y, p.z);
		}

		line += string().Format ("\n");

		bool lineWrittenOk = (0 != file.Write (line.c_str (), line.length ()));
		ok = ok && lineWrittenOk;
	}

	return ok;
}

} // namespace LayeredNavMesh


// ---

// FIXxME Jun 9, 2008: <pvl> to be moved to a separate file, most likely
// ATTN Dec 11, 2008: <pvl> this class is obsolete since it assumes that graph
// node indices survive loading and saving which they don't.  The mapping
// between polygons and their corresponding graph nodes is now computed
// as a post-load step in GraphNodeCont::SetupStaticGraphNodes().
// Not to mention that having 1-to-1 correspondence between polys and nodes
// turned out to be a bad idea altogether, leading to severely suboptimal
// paths coming out of A*.
// UPDATE Jan 28, 2010: <pvl> the "export" part of this is still dead but make
// sure we run this nevertheless as graph nodes need to be inserted (if for
// no other reason then at least for unreachable polygon pruning).
class NavGraphGenerator {

	CGraph * m_pGraph;		// NOTE Jun 9, 2008: <pvl> we don't own the memory
	const LayeredNavMesh::NavPolygonGenerator * m_pPolygons;

	std::vector <unsigned int> m_graphNodeIndices;

	unsigned int CreateGraphNode (unsigned navModifIndex, unsigned polygonIndex, int agentTypeId);
public:
	NavGraphGenerator (CGraph * graph);
	~NavGraphGenerator ();

	void Generate (
			const LayeredNavMesh::NavPolygonGenerator * polygons,
			unsigned navModifIndex, int agentTypeId
	);

	// FIXME Jul 25, 2008: <pvl> ugly - would probably be better to have an Export()
	// function and save private stuff within it?
	const std::vector <unsigned int> & GetGraphNodeIndices () const { return m_graphNodeIndices; }
};

NavGraphGenerator::NavGraphGenerator (CGraph * graph) :
		m_pGraph(graph), m_pPolygons(0)
{
}

NavGraphGenerator::~NavGraphGenerator ()
{
	// NOTE Aug 5, 2008: <pvl> flush our nodes from the graph
	std::vector<unsigned>::const_iterator it = m_graphNodeIndices.begin ();
	std::vector<unsigned>::const_iterator end = m_graphNodeIndices.end ();
	for ( ; it != end; ++it)
	{
		m_pGraph->Disconnect (*it);
	}
}

// TODO Jun 9, 2008: <pvl> how about passing in NavPolyPtr directly?  That way
// we wouldn't have to access NavPolyGenerator in this function and we might
// even be able to avoid having to store it as member data.
unsigned int NavGraphGenerator::CreateGraphNode (unsigned navModifIndex, unsigned polygonIndex, int agentTypeId)
{
	const NavPolygonGenerator::NavPolyPtr poly = m_pPolygons->GetPoly (polygonIndex);
	const LayeredNavMesh::Vec3 graphNodePos = poly.GetCentroid ();

	unsigned nodeIndex = m_pGraph->CreateNewNode (
			IAISystem::NAV_LAYERED_NAV_MESH, ConvertVec3 (graphNodePos)/*Vec3 (graphNodePos.z, graphNodePos.x, graphNodePos.y)*/
	);

	GraphNode * node = m_pGraph->GetNodeManager().GetNode (nodeIndex);
	node->GetLayeredMeshNavData()->polygonIndex = polygonIndex;
	node->GetLayeredMeshNavData()->navModifIndex = navModifIndex;
	node->GetLayeredMeshNavData()->agentType = agentTypeId;

	return nodeIndex;
}

void NavGraphGenerator::Generate (
		const LayeredNavMesh::NavPolygonGenerator * polygons,
		unsigned navModifIndex, int agentTypeId)
{
	m_pPolygons = polygons;
	m_graphNodeIndices.resize (0);

	int polyCount = polygons->GetPolyCount();

	m_graphNodeIndices.resize (polyCount);

	for (int i=0; i < polyCount; ++i)
	{
		const NavPolygonGenerator::NavPolyPtr poly = polygons->GetPoly (i);

		// if there's no graph node yet for this poly create it now
		if (m_graphNodeIndices[i] == 0)
		{
			m_graphNodeIndices[i] = CreateGraphNode (navModifIndex, i, agentTypeId);
		}
		unsigned int thisNodeIndex = m_graphNodeIndices[i];

		// iterate over all neighbours and create links in case they don't exist yet
		for (unsigned j = 0, nj = poly.GetNumEdges(); j < nj; ++j)
		{
			int neighborPolyIndex = poly.GetNeighbourIndex (j);
			if (neighborPolyIndex == LayeredNavMesh::NavEdge::INVALID)
				continue;

			if (m_graphNodeIndices [neighborPolyIndex] == 0)
			{
				m_graphNodeIndices[neighborPolyIndex] = CreateGraphNode (navModifIndex, neighborPolyIndex, agentTypeId);
			}
			unsigned int neighborNodeIndex = m_graphNodeIndices[neighborPolyIndex];

			// now we've got indices of both neighbouring polys/nodes - link them if necessary
			int linkIndex = m_pGraph->GetNodeManager().GetNode(thisNodeIndex)->GetLinkIndex (
						m_pGraph->GetNodeManager(),
						m_pGraph->GetLinkManager(),
						m_pGraph->GetNodeManager().GetNode(neighborNodeIndex)
			);
			if (-1 == linkIndex)
				m_pGraph->Connect (thisNodeIndex, neighborNodeIndex);
		}
	}
}

// ---

#include "BitArray.h"
using LayeredNavMesh::BitArray;

// NOTE Jan 28, 2010: <pvl> unlike runtime GetEnclosing() we have no picking
// accelerator here.  We could construct one but it seems wasteful to do so just
// to run a single query (or a couple of them at most).  We test all polys instead.
unsigned GetEnclosingPolygon (const LayeredNavMesh::NavPolygonGenerator * polygons, const ::Vec3 & pos)
{
	std::vector< ::Vec3> polygon;
	std::vector<unsigned> candidates;

	for (unsigned p=0, polyCount=polygons->GetPolyCount(); p < polyCount; ++p)
	{
		polygon.resize (0);

		LayeredNavMesh::NavPolygonGenerator::NavPolyPtr poly = polygons->GetPoly (p);
		LayeredNavMesh::NavPolygonGenerator::NavPolyPtr::vertex_iterator it = poly.begin();
		const LayeredNavMesh::NavPolygonGenerator::NavPolyPtr::vertex_iterator end = poly.end();
		for ( ; it != end; ++it)
			polygon.push_back (ConvertVec3 (*it));

		// TODO Jan 28, 2010: <pvl> this could be sped up if we construct an AABB
		// while filling 'polygon' and pass it to the following call?
		if (Overlap::Point_Polygon2D (pos, polygon))
			candidates.push_back (p);
	}

	unsigned enclosing = std::numeric_limits<unsigned>::max();
	float closestDistSqr = std::numeric_limits<float>::max ();
	for (unsigned c=0, nc=candidates.size(); c < nc; ++c)
	{
		AABB polyAABB (AABB::RESET);

		LayeredNavMesh::NavPolygonGenerator::NavPolyPtr poly = polygons->GetPoly (candidates[c]);
		LayeredNavMesh::NavPolygonGenerator::NavPolyPtr::vertex_iterator it = poly.begin();
		const LayeredNavMesh::NavPolygonGenerator::NavPolyPtr::vertex_iterator end = poly.end();
		for ( ; it != end; ++it)
			polyAABB.Add (ConvertVec3 (*it));

		if (polyAABB.IsContainPoint (pos))
		{
			enclosing = candidates[c];
			break;
		}
		else
		{
			float distSqr = fabs (polyAABB.GetCenter ().z - pos.z);
			if (distSqr < closestDistSqr)
			{
				closestDistSqr = distSqr;
				enclosing = candidates[c];
			}
		}
	}
	return enclosing;
}

unsigned GetNodeByPolygon (CGraph * graph, unsigned polygonIndex, unsigned navModifIndex, int agentTypeId)
{
	CAllNodesContainer::Iterator it (graph->GetAllNodes(), IAISystem::NAV_LAYERED_NAV_MESH);

	while (unsigned nodeIndex = it.Increment())
	{
		const GraphNode * node = graph->GetNodeManager().GetNode (nodeIndex);
		const SLayeredMeshNavData * lnmData = node->GetLayeredMeshNavData();
		if (lnmData->navModifIndex == navModifIndex && lnmData->agentType == agentTypeId
				&& lnmData->polygonIndex == polygonIndex)
			return nodeIndex;
	}
	return 0;
}

BitArray * ComputePolygonReachability (const LayeredNavMesh::NavPolygonGenerator * polygons, unsigned navModifIndex, int agentTypeId, CGraph * graph)
{
	// TODO Jan 27, 2010: <pvl> get all pertinent markers - see CVolumeNavRegion::GetNavigableSpaceEntities()
	// for an example how to retrieve more than one
	IEntity * ent = gEnv->pSystem->GetIEntitySystem()->FindEntityByName ("Reachable");

	if (0 == ent)
		return 0;

	const ::Vec3 pos = ent->GetWorldPos();

	const unsigned enclosing = GetEnclosingPolygon (polygons, pos);
	if (enclosing == std::numeric_limits<unsigned>::max())
		return 0;		// NOTE Jan 28, 2010: <pvl> all are reachable if there's no valid seed

	const unsigned enclosingNodeIndex = GetNodeByPolygon (graph, enclosing, navModifIndex, agentTypeId);
	// NOTE Jan 28, 2010: <pvl> if this triggers, maybe NavGraphGenerator wasn't run?
	assert (enclosingNodeIndex);

	// NOTE Jan 27, 2010: <pvl> now for the flood-fill
	BitArray * reachablePolys = new BitArray (polygons->GetPolyCount ());

	std::set <unsigned> nodesVisited;
	std::vector <unsigned> nodesToCheck;

	nodesToCheck.push_back (enclosingNodeIndex);
	nodesVisited.insert (enclosingNodeIndex);

	while ( ! nodesToCheck.empty ())
	{
		unsigned nodeIdx = nodesToCheck.back();
		nodesToCheck.pop_back();

		const GraphNode * node = graph->GetNodeManager().GetNode (nodeIdx);

		if (node->navType == IAISystem::NAV_LAYERED_NAV_MESH)
		{
			const SLayeredMeshNavData * lnmData = node->GetLayeredMeshNavData();
			assert (lnmData->navModifIndex == navModifIndex && lnmData->agentType == agentTypeId);
			reachablePolys->Mark (lnmData->polygonIndex);
		}

		// NOTE Jan 27, 2010: <pvl> for each link incident to 'node'
		unsigned linkId = node->firstLinkIndex;
		for ( ; linkId; linkId = graph->GetLinkManager().GetNextLink(linkId))
		{
			// NOTE Jan 27, 2010: <pvl> get graph node at the far end of this link
			const unsigned nextNodeIndex = graph->GetLinkManager().GetNextNode(linkId);

			if (nodesVisited.find (nextNodeIndex) != nodesVisited.end())
				continue;

			const GraphNode * next = graph->GetNodeManager().GetNode(nextNodeIndex);
			if (next->navType == IAISystem::NAV_SMARTOBJECT || next->navType == IAISystem::NAV_LAYERED_NAV_MESH)
				nodesToCheck.push_back (nextNodeIndex);

			nodesVisited.insert (nextNodeIndex);
		}
	}
	return reachablePolys;
}

// ---

// NOTE Aug 26, 2009: <pvl> severely limited Alias/Wavefront OBJ exporter - only
// exports vertices and faces
void ExportToObj (const LayeredNavMesh::DynArray<GeomInstance*> & instances, const string & fname)
{
	using namespace LayeredNavMesh;

	FILE * fp = fopen (fname, "w");
	if (0 == fp) return;

	unsigned baseVtxIdx = 0;

	for (unsigned i = 0, ni = instances.Size(); i < ni; ++i)
	{
		GeomInstance* instance = instances[i];

		const Matrix4 & tm = instance->GetTM();
		const GeomMesh * mesh = instance->GetMesh();

		const LayeredNavMesh::DynArray<LayeredNavMesh::Vec3>& verts = mesh->GetVerts();

		for (LayeredNavMesh::DynArray<LayeredNavMesh::Vec3>::ConstIter v = verts.Begin(); v != verts.End(); ++v)
		{
			LayeredNavMesh::Vec3 tv = tm.TransformPoint(*v);
			fprintf (fp, "v %f %f %f\n", tv.x, tv.y, tv.z);
		}

		const LayeredNavMesh::DynArray<GeomMesh::Face>& faces = mesh->GetFaces();

		for (LayeredNavMesh::DynArray<GeomMesh::Face>::ConstIter f = faces.Begin(); f != faces.End(); ++f)
		{
			// NOTE Aug 25, 2009: <pvl> in OBJ vertex indices seem to be 1-based
			fprintf (fp, "f %d %d %d\n",
						baseVtxIdx + f->va + 1, baseVtxIdx + f->vb + 1, baseVtxIdx + f->vc + 1);
		}

		baseVtxIdx += verts.Size ();
	}
	fclose (fp);
}

void ExportToObj (
		const LayeredNavMesh::NavPolygonGenerator * polygons,
		unsigned region, const string & fname)
{
	FILE * fp = fopen (fname, "w");
	if (0 == fp) return;

	const unsigned vtxCount = polygons->GetVertCount();
	char * const vtxUsed = new char [vtxCount];
	memset ( & vtxUsed[0], 0, vtxCount );

	// NOTE Jan 12, 2010: <pvl> see which vertices are used by the region
	for (int p=0, np=polygons->GetPolyCount(); p < np; ++p)
	{
		const LayeredNavMesh::NavPoly * poly = polygons->GetPoly(p).GetInternalPoly ();
		assert (poly);
		if (poly->region != region)
			continue;

		for (unsigned v=0, nv = poly->numEdges; v < nv; ++v)
			vtxUsed[poly->edges[v].v] = 1;
	}

	// NOTE Jan 12, 2010: <pvl> write vertices and create vertex index translation table
	unsigned short * const vtxIdxTranslationTbl = new unsigned short[vtxCount];
	memset ( & vtxIdxTranslationTbl[0], 0xff, vtxCount * sizeof (unsigned short));
	unsigned numUsedVtxs = 0;
	for (unsigned v=0; v < vtxCount; ++v)
	{
		if (vtxUsed[v])
		{
			const LayeredNavMesh::Vec3 & vtx = polygons->GetVert (v);
			fprintf (fp, "v %f %f\n", vtx.x, vtx.z);
			vtxIdxTranslationTbl[v] = numUsedVtxs++;
		}
	}

	// NOTE Jan 12, 2010: <pvl> now run through faces again and write them out
	// using the vertex index translation computed above
	for (int p=0, np=polygons->GetPolyCount(); p < np; ++p)
	{
		const LayeredNavMesh::NavPoly * poly = polygons->GetPoly(p).GetInternalPoly ();
		assert (poly);
		if (poly->region != region)
			continue;

		fprintf (fp, "f");
		for (unsigned v=0, nv = poly->numEdges; v < nv; ++v)
		{
			assert (vtxIdxTranslationTbl[poly->edges[v].v] != 0xffff);
			fprintf (fp, " %d", vtxIdxTranslationTbl[poly->edges[v].v] + 1);
		}
		fprintf (fp, "\n");
	}

	delete [] vtxIdxTranslationTbl;
	delete [] vtxUsed;

	fclose (fp);
}


// ---



struct CLayeredMeshGenerator::CVarMgr {
	ICVar * m_cvDefaultAgentPFPropertiesName;
	ICVar * m_cvDrawVoxels;
	ICVar * m_cvDrawSurfaces;
	ICVar * m_cvDrawCountours;
	ICVar * m_cvDrawPolygons;
	ICVar * m_cvDrawTerrain;
	ICVar * m_cvIgnoreTerrain;
	ICVar * m_cvIgnoreBrushes;
	ICVar * m_cvVoxelSize;
	ICVar * m_cvVoxelSizeVert;
	ICVar * m_cvAgentClimbCapTangentOverride;
	ICVar * m_cvAgentClimbCap;
	ICVar * m_cvContourOptimTolNumer;
	ICVar * m_cvContourOptimTolDenom;
	ICVar * m_cvExportInputGeomToObj;
	ICVar * m_cvDrawInputGeom;
	ICVar * m_cvIntermediatePreprocessedDataDumps;

	CVarMgr ();
	void Init ();
};

CLayeredMeshGenerator::CVarMgr::CVarMgr ()
{
	// NOTE Mai 28, 2008: <pvl> I don't normally do this but with this struct,
	// the assumption is that all data members will be ICVar*'s so zeroing them
	// is a proper initialization.  Moreover it eliminates the danger of someone
	// adding a new pointer and forgetting to add corresponding initialization as well.
	memset (this, 0, sizeof (*this));

	Init ();
}

void CLayeredMeshGenerator::CVarMgr::Init ()
{
	m_cvDefaultAgentPFPropertiesName =
		REGISTER_STRING("ai_lnmDefaultAgentPFPropertiesName","AIPATH_DEFAULT",VF_CHEAT|VF_DUMPTODISK,
		"The name of Agent Pathfinding Properties assigned to Layered Nav Mesh modifier by default");
	m_cvDrawVoxels = REGISTER_INT("ai_lnmDrawVoxels",0,VF_CHEAT|VF_DUMPTODISK,
		"Debug draw of Layered Nav Mesh voxelization");
	m_cvDrawSurfaces = REGISTER_INT("ai_lnmDrawSurfaces",0,VF_CHEAT|VF_DUMPTODISK,
		"Debug draw of Layered Nav Mesh walkable surfaces as extracted from voxels");
	m_cvDrawCountours = REGISTER_INT("ai_lnmDrawContours",1,VF_CHEAT|VF_DUMPTODISK,
		"Debug draw of Layered Nav Mesh walkable surfaces contours");
	m_cvDrawPolygons = REGISTER_INT("ai_lnmDrawPolygons",1,VF_CHEAT|VF_DUMPTODISK,
		"Debug draw of Layered Nav Mesh walkable surfaces polygon subdivision");
	m_cvDrawTerrain = REGISTER_INT("ai_lnmDrawTerrain",0,VF_CHEAT|VF_DUMPTODISK,
		"Debug draw of terrain heighmap as seen by the LNM preprocessor");
	m_cvIgnoreTerrain = REGISTER_INT("ai_lnmIgnoreTerrain",0,VF_CHEAT|VF_DUMPTODISK,
		"Excludes terrain from Layered Nav Mesh preprocessing");
	m_cvIgnoreBrushes = REGISTER_INT("ai_lnmIgnoreBrushes",0,VF_CHEAT|VF_DUMPTODISK,
		"Excludes brushes (and voxel terrain) from Layered Nav Mesh preprocessing");
	m_cvVoxelSize = REGISTER_FLOAT("ai_lnmVoxelSizeOverride",-1.0f,VF_CHEAT|VF_DUMPTODISK,
		"Horizontal voxel size for voxelization stage of LNM preprocessing.  This "
		"is normally computed from agent radius but if this var has a positive "
		"value it overrides that computation.");
	m_cvVoxelSizeVert = REGISTER_FLOAT("ai_lnmVerticalVoxelSizeOverride",-1.0f,VF_CHEAT|VF_DUMPTODISK,
		"Vertical resolution of the voxel grid used by LNM preprocessor, expressed "
		"as a multiple of ai_lnmVoxelSize.  Normally, this will be automatically "
		"derived from ai_lnmAgentClimbCap but if you set this var to be >=0.0f you "
		"can override it.  Use only if you know what know what you're doing.");
	m_cvAgentClimbCap = REGISTER_INT("ai_lnmAgentClimbCap",45,VF_CHEAT|VF_DUMPTODISK,
		"Maximum slope the agent can climb, given in degrees (0 - 60)");
	m_cvContourOptimTolNumer = REGISTER_INT("ai_lnmContourOptimTolNumer",1,VF_CHEAT|VF_DUMPTODISK,
		"Numerator of the fraction that controls tolerance during contour optimisation");
	m_cvContourOptimTolDenom = REGISTER_INT("ai_lnmContourOptimTolDenom",1,VF_CHEAT|VF_DUMPTODISK,
		"Denominator of the fraction that controls tolerance during contour optimisation");
	m_cvExportInputGeomToObj = REGISTER_STRING("ai_lnmExportInputGeomToObj","none",VF_CHEAT|VF_DUMPTODISK,
		"Set this to a file name where input geometry as acquired from physics should be stored. "
		"Set to \"none\" to turn it off.");
	m_cvDrawInputGeom = REGISTER_INT("ai_lnmDrawInputGeom",0,VF_CHEAT|VF_DUMPTODISK,
		"Set to non-zero to draw input geometry as acquired from physics and seen by the pre-processor");
	m_cvIntermediatePreprocessedDataDumps = REGISTER_INT("ai_lnmIntermediatePreprocessedDataDumps",0,VF_CHEAT|VF_DUMPTODISK,
		"Set to non-zero to have pre-processor read voxels and surfaces from files is they are present, "
		"or dump generated data into files if they aren't.  KNOW WHAT YOU'RE DOING IF YOU USE THIS.");
}

CLayeredMeshGenerator::CVarMgr * CLayeredMeshGenerator::s_cvarMgr = 0;

CLayeredMeshGenerator::CLayeredMeshGenerator (CGraph * graph, unsigned modifIndex) :
		m_pVoxelizer(0), m_pSurfaceGen(0), m_pContourGen(0), m_pPolygonGen(0),
		m_pGraphGen(0), m_pGraph(graph), m_navModifIndex (modifIndex), m_terrain (0),
		m_meshes(0), m_instances(0)
{
}

void CLayeredMeshGenerator::InitConsoleVars ()
{
	if (s_cvarMgr == 0)
		s_cvarMgr = new CVarMgr;
}

CLayeredMeshGenerator::~CLayeredMeshGenerator ()
{
	Reset ();
}

void CLayeredMeshGenerator::Reset ()
{
	SAFE_DELETE (m_pVoxelizer);
	SAFE_DELETE (m_pSurfaceGen);
	SAFE_DELETE (m_pContourGen);
	SAFE_DELETE (m_pPolygonGen);
	SAFE_DELETE (m_pGraphGen);
	SAFE_DELETE (m_settings.sceneBounds);

	using namespace LayeredNavMesh;

	std::vector <const SceneBounds *>::const_iterator exclIt = m_settings.exclusionVolumes.begin ();
	const std::vector <const SceneBounds *>::const_iterator exclEnd = m_settings.exclusionVolumes.end ();
	for ( ; exclIt != exclEnd; ++exclIt)
		delete *exclIt;
	m_settings.exclusionVolumes.resize (0);

	m_terrain = 0;

	SAFE_DELETE (m_instances);
	SAFE_DELETE (m_meshes);
}

bool ReadVoxels (Voxelizer * voxelizer, const string & fname)
{
	using namespace LayeredNavMesh::ExportFormat;
	FileReader reader;

	VersionSwitchChunkReader * spanBufSwitch = new VersionSwitchChunkReader;
	spanBufSwitch->RegisterReader (1, new LayeredNavMesh::BinSpanBufferChunkReader (const_cast <LayeredNavMesh::SpanBuffer*> (voxelizer->GetSpans())));
	reader.RegisterChunkReader (LayeredNavMesh::LnmExportFormat::LnmChunkType::NAV_MESH_SPAN_BUFFER_BIN, spanBufSwitch);

	Stopwatch readVoxels;
	readVoxels.Start ();
	bool success = reader.Read (fname.c_str());
	if (success)
	{
		readVoxels.Stop ();
		printf("Voxels read in from file in %.3fms\n", 1000.0 * readVoxels.GetElapsed ());
	}
	return success;
}

void WriteVoxels (Voxelizer * voxelizer, const string & fname)
{
	CCryFile file;
	file.Open (fname.c_str(), "wb");
	LayeredNavMesh::BinSpanBufferChunkWriter spanBufWriter (voxelizer->GetSpans(), 1);
	spanBufWriter.WriteChunk (file);
	file.Close();
}

bool ReadNavSurfaceElements (NavSurfaceGenerator * surfaceGen, const string & fname)
{
	using namespace LayeredNavMesh::ExportFormat;
	FileReader reader;

	VersionSwitchChunkReader * navSurfSwitch = new VersionSwitchChunkReader;
	navSurfSwitch->RegisterReader (1, new LayeredNavMesh::BinNavSurfaceElementChunkReader (surfaceGen));
	reader.RegisterChunkReader (LayeredNavMesh::LnmExportFormat::LnmChunkType::NAV_MESH_NAV_SURF_ELEMENTS_BIN, navSurfSwitch);

	Stopwatch readSurfs;
	readSurfs.Start ();
	bool success = reader.Read (fname.c_str());
	if (success)
	{
		readSurfs.Stop ();
		printf("Nav surface elements read in from file in %.3fms\n", 1000.0 * readSurfs.GetElapsed ());
	}
	return success;
}

void WriteNavSurfaceElements (NavSurfaceGenerator * surfaceGen, const string & fname)
{
	CCryFile file;
	file.Open (fname.c_str(), "wb");
	LayeredNavMesh::BinNavSurfaceElementChunkWriter navSurfWriter (surfaceGen, 1);
	navSurfWriter.WriteChunk (file);
	file.Close();
}

void CLayeredMeshGenerator::DoGenerate ()
{
	Stopwatch total;

	printf("Starting generation of LNM data...\n");
	total.Start ();

	m_meshes = new LayeredNavMesh::DynArray<GeomMesh*>;
	m_instances = new LayeredNavMesh::DynArray<GeomInstance*>;

	// Populate meshes and instances...
	if (s_cvarMgr->m_cvIgnoreBrushes->GetIVal () == 0)
	{
		if ( ! ConvertGeometry (*m_meshes, *m_instances, m_settings.sceneBounds))
		{
			AIError ("LayeredMeshGenerator::Generate(): couldn't get geometry\n");
			return;
		}
	}

	if (s_cvarMgr->m_cvIgnoreTerrain->GetIVal () == 0)
	{
		if ( ! ConvertTerrain (*m_meshes, *m_instances, m_settings.sceneBounds))
		{
			AIError ("LayeredMeshGenerator::Generate(): couldn't get terrain\n");
			return;
		}
		m_terrain = m_meshes->Last ();
	}

	string objFileName = s_cvarMgr->m_cvExportInputGeomToObj->GetString();
	if (objFileName != "none")
		ExportToObj (*m_instances, objFileName);

	printf("Agent: h=%d  r=%d  c=%d  voxel size: horizontal=%.4f vertical=%.4f\n",
			m_settings.agentHeight, m_settings.agentRadius, m_settings.agentClimb,
			m_settings.voxelSizeHorizontal, m_settings.voxelSizeVertical);

	LayeredNavMesh::Vec3 dim = m_settings.sceneBounds->GetAABB().Max() - m_settings.sceneBounds->GetAABB().Min();
	printf("Dim: %.1f x %.1f x %.1f\n", dim.x, dim.y, dim.z);

	bool readFiles = s_cvarMgr->m_cvIntermediatePreprocessedDataDumps->GetIVal();
	const bool writeFiles = readFiles;

	m_pVoxelizer = new Voxelizer (m_settings);
	m_pVoxelizer->InitSpanBuffer ();

	const string levelPath (Path::AddBackslash (GetIEditor()->GetGameEngine()->GetLevelPath()));
	const string voxelsFname = levelPath + m_navModifName + "-" + m_agentType + "_voxels-bin.dbg";
	if (readFiles)
		readFiles = ReadVoxels (m_pVoxelizer, voxelsFname);
	if ( ! readFiles )
	{
		m_pVoxelizer->VoxelizeSpanBuffer(*m_instances);

		if (writeFiles)
			WriteVoxels (m_pVoxelizer, voxelsFname);
	}

	m_pSurfaceGen = new NavSurfaceGenerator (m_settings);

	const string navSurfFname = levelPath + m_navModifName + "-" + m_agentType + "_navsurfs-bin.dbg";
	if (readFiles)
		readFiles = ReadNavSurfaceElements (m_pSurfaceGen, navSurfFname);
	if ( ! readFiles )
	{
		m_pSurfaceGen->BuildSurfaceFromSpanBuffer(m_pVoxelizer->GetSpans());

		if (writeFiles)
			WriteNavSurfaceElements (m_pSurfaceGen, navSurfFname);
	}

	m_pContourGen = new NavContourGenerator (m_settings, s_cvarMgr->m_cvContourOptimTolNumer->GetIVal(), s_cvarMgr->m_cvContourOptimTolDenom->GetIVal());
	m_pContourGen->BuildContours (m_pSurfaceGen);

	m_pPolygonGen = new NavPolygonGenerator (m_settings);
	m_pPolygonGen->BuildPolygons(m_pContourGen);

	//ExportToObj (m_pPolygonGen, 1, "region-1.obj");

	// FIXME Aug 7, 2009: <pvl> fix the case of generating with no modifier and
	// therefore no explicit properties
	const AgentPathfindingProperties * pfProp = GetIEditor()->GetAI()->GetPFPropertiesOfPathType (m_agentType);
	const int agentTypeId = pfProp ? pfProp->id : 0;

	m_pGraphGen = new NavGraphGenerator (m_pGraph);
	m_pGraphGen->Generate (m_pPolygonGen, m_navModifIndex, agentTypeId);

	m_pPolygonGen->SetPolygonReachability (ComputePolygonReachability (m_pPolygonGen, m_navModifIndex, agentTypeId, m_pGraph));

	total.Stop ();
	printf("Total time for LNM data generation: %.3fms\n\n", 1000.0 * total.GetElapsed ());
}

void CLayeredMeshGenerator::FillInSettings (float agentHeight, float agentRadius, int maxSlopeDeg)
{
	const float slopeTangent = tan (maxSlopeDeg / 180.0f * M_PI);
	const float vertVoxelResolutionOverride = s_cvarMgr->m_cvVoxelSizeVert->GetFVal ();
	const float vertVoxelResolution = vertVoxelResolutionOverride >= 0.0f ? vertVoxelResolutionOverride : slopeTangent;

	float voxelSizeOverride = s_cvarMgr->m_cvVoxelSize->GetFVal ();
	const float voxelSize = voxelSizeOverride > 0.0f ? voxelSizeOverride : agentRadius / 3;
	m_settings.voxelSizeHorizontal = voxelSize;
	m_settings.voxelSizeVertical = voxelSize * vertVoxelResolution;
	m_settings.agentHeight = int (agentHeight / m_settings.voxelSizeVertical);
	m_settings.agentRadius = RoundFloatToInt (agentRadius / m_settings.voxelSizeHorizontal);
	m_settings.agentClimb = 1;
}

void CLayeredMeshGenerator::Generate ()
{
	Reset ();

	ISystem * pSystem = GetIEditor()->GetSystem();

	float fTSize = (float) pSystem->GetI3DEngine()->GetTerrainSize();
	Vec3 sceneMin (0,0,-50);
	Vec3 sceneMax (fTSize,fTSize,500.0f);

	std::vector<::Vec3> poly;
	poly.push_back (Vec3 (sceneMin.x, sceneMin.y, 0));
	poly.push_back (Vec3 (sceneMax.x, sceneMin.y, 0));
	poly.push_back (Vec3 (sceneMax.x, sceneMax.y, 0));
	poly.push_back (Vec3 (sceneMin.x, sceneMax.y, 0));

	m_settings.sceneBounds =
			new LayeredNavMesh::SceneBounds (
					new LayeredNavMesh::NavModifSceneBounds (
							ConvertVec3 (sceneMin), ConvertVec3 (sceneMax),
							poly.begin(), poly.end()
					)
			);

	FillInSettings (1.8f, 0.4f, s_cvarMgr->m_cvAgentClimbCap->GetIVal());

	DoGenerate ();
}

static LayeredNavMesh::SceneBounds * Convert (const SpecialArea * sa)
{
	const AABB modifierAABB = sa->GetAABB ();
	const ListPositions & navModifPoly = sa->GetPolygon ();
	const Vec3 sceneMin = modifierAABB.min;
	const Vec3 sceneMax = modifierAABB.max + Vec3 (0, 0, sa->fHeight);

	return new LayeredNavMesh::SceneBounds (
					new LayeredNavMesh::NavModifSceneBounds (
							ConvertVec3 (sceneMin), ConvertVec3 (sceneMax),
							navModifPoly.begin(), navModifPoly.end()
					)
			);
}

void CLayeredMeshGenerator::Generate (
			const string & navModifName, const SpecialArea & navModifier,
			const string & agentType, const std::vector <const SpecialArea *> & exclusionVols)
{
	Reset ();

	m_settings.sceneBounds = Convert ( & navModifier);

	// NOTE Aug 18, 2009: <pvl> convert exlusion volumes
	m_settings.exclusionVolumes.reserve (exclusionVols.size ());
	std::vector <const SpecialArea *>::const_iterator exclIt = exclusionVols.begin ();
	const std::vector <const SpecialArea *>::const_iterator exclEnd = exclusionVols.end ();
	for ( ; exclIt != exclEnd; ++exclIt)
		m_settings.exclusionVolumes.push_back (Convert (*exclIt));

	m_navModifName = navModifName;

	m_agentType = agentType;
	const AgentPathfindingProperties * pfProp = GetIEditor()->GetAI()->GetPFPropertiesOfPathType (agentType);

	FillInSettings (pfProp->height, pfProp->radius, pfProp->maxSlope);

	DoGenerate ();
}

namespace LayeredNavMesh {

class VertexListChunkWriter : public ExportFormat::VersionedChunkWriter {
	const NavPolygonGenerator * m_polygons;

	virtual bool WriteChunkData (CCryFile & ) const;
public:
	VertexListChunkWriter (uint16 version, const NavPolygonGenerator * polys) :
			ExportFormat::VersionedChunkWriter (LnmExportFormat::LnmChunkType::NAV_MESH_VERTICES, version),
			m_polygons (polys)
	{ }
};

bool VertexListChunkWriter::WriteChunkData (CCryFile & file) const
{
	unsigned int numVertices = m_polygons->GetVertCount ();
	LnmExportFormat::NavMeshVertexListChunk::Header hdr (numVertices);

	file.Write ( & hdr, sizeof (hdr));

	for (int i=0; i < numVertices; ++i)
	{
		::Vec3 vertex = ConvertVec3 (m_polygons->GetVert (i));
		file.Write ( & vertex.x, sizeof (float));
		file.Write ( & vertex.y, sizeof (float));
		file.Write ( & vertex.z, sizeof (float));
	}
	return true;
}


class PolygonListChunkWriter : public ExportFormat::VersionedChunkWriter {
	const NavPolygonGenerator * m_polygons;

	virtual bool WriteChunkData (CCryFile & ) const;
public:
	PolygonListChunkWriter (uint16 version, const NavPolygonGenerator * polys) :
			ExportFormat::VersionedChunkWriter (LnmExportFormat::LnmChunkType::NAV_MESH_POLYGONS, version),
			m_polygons (polys)
	{ }
};

bool PolygonListChunkWriter::WriteChunkData (CCryFile & file) const
{
	// NOTE Okt 20, 2008: <pvl> take note where this chunk begins
	unsigned chunkBeginPos = file.GetPosition ();
	// NOTE Okt 20, 2008: <pvl> skip enough space for the header (to be filled in later)
	file.Seek (LnmExportFormat::NavMeshPolygonListChunk::Header::Size (), SEEK_CUR);

	unsigned indexCount = 0;

	unsigned int polyCount = m_polygons->GetPolyCount();
	for (int i=0; i < polyCount; ++i)
	{
		const LayeredNavMesh::NavPoly * poly = m_polygons->GetPoly (i).GetInternalPoly ();

		indexCount += poly->numEdges;

		LnmExportFormat::NavMeshPolygonListChunk::Polygon filePoly;
		filePoly.m_region = poly->region;
		filePoly.m_numIndices = poly->numEdges;
		filePoly.m_indices.reserve (filePoly.m_numIndices);

		for (unsigned edgeIndex=0, numEdges=poly->numEdges; edgeIndex < numEdges; ++edgeIndex)
		{
			filePoly.m_indices.push_back (poly->edges[edgeIndex].v);
		}
		filePoly.Write (file);
	}

	// NOTE Okt 20, 2008: <pvl> go back to the chunk header and fill in the number of indices
	LnmExportFormat::NavMeshPolygonListChunk::Header hdr (0, polyCount, indexCount);

	file.Seek (chunkBeginPos, SEEK_SET);
	hdr.Write (file);
	file.Seek (0, SEEK_END);

	return true;
}


// ATTN Dec 11, 2008: <pvl> obsolete, see comment near NavGraphGenerator
class GraphNodeIndexListChunkWriter : public ExportFormat::VersionedChunkWriter {
	const std::vector<unsigned> & m_graphNodeIndices;

	virtual bool WriteChunkData (CCryFile & ) const;
public:
	GraphNodeIndexListChunkWriter (uint16 version, const std::vector<unsigned> & nodeIndices) :
			ExportFormat::VersionedChunkWriter (LnmExportFormat::LnmChunkType::NAV_MESH_GRAPH_NODE_INDICES, version),
			m_graphNodeIndices (nodeIndices)
	{ }
};

bool GraphNodeIndexListChunkWriter::WriteChunkData (CCryFile & file) const
{
	unsigned int numIndices = m_graphNodeIndices.size ();
	LnmExportFormat::NavMeshGraphNodeIndexListChunk::Header hdr (numIndices);

	file.Write ( & hdr, sizeof (hdr));

	for (int i=0; i < numIndices; ++i)
	{
		file.Write ( & m_graphNodeIndices[i], sizeof (unsigned int));
	}
	return true;
}


class SceneBoundsChunkWriter : public ExportFormat::VersionedChunkWriter {
	::Vec3 m_sceneMin;
	::Vec3 m_sceneMax;

	virtual bool WriteChunkData (CCryFile & ) const;
public:
	SceneBoundsChunkWriter (uint16 version, const ::Vec3 & sceneMin, const ::Vec3 & sceneMax) :
			ExportFormat::VersionedChunkWriter (LnmExportFormat::LnmChunkType::NAV_MESH_SCENE_BOUNDS, version),
			m_sceneMin (sceneMin), m_sceneMax (sceneMax)
	{ }
};

bool SceneBoundsChunkWriter::WriteChunkData (CCryFile & file) const
{
	bool ok = true;

	ok = ok && (0 != file.Write ( & m_sceneMin.x, sizeof (float)) );
	ok = ok && (0 != file.Write ( & m_sceneMin.y, sizeof (float)) );
	ok = ok && (0 != file.Write ( & m_sceneMin.z, sizeof (float)) );

	ok = ok && (0 != file.Write ( & m_sceneMax.x, sizeof (float)) );
	ok = ok && (0 != file.Write ( & m_sceneMax.y, sizeof (float)) );
	ok = ok && (0 != file.Write ( & m_sceneMax.z, sizeof (float)) );

	return ok;
}


class ModifierInfoChunkWriter : public ExportFormat::VersionedChunkWriter {

	string m_name;
	unsigned int m_id;

	int m_agentTypeId;
	// NOTE Aug 7, 2009: <pvl> this is written out just for debugging, just to
	// be able to check which agent type name the agent type ID corresponded
	// at the time the file was written.  Readers and runtime AI should make
	// decisions solely based on ID though.
	string m_agentTypeName;

	virtual bool WriteChunkData (CCryFile & ) const;
public:
	ModifierInfoChunkWriter (uint16 version, const string & name, unsigned id,
				int agentTypeId, const string & agentTypeName) :
			ExportFormat::VersionedChunkWriter (LnmExportFormat::LnmChunkType::NAV_MESH_MODIFIER_INFO, version),
			m_name (name.empty() ? "*WholeMap*" : name), m_id (id),
			m_agentTypeId (agentTypeId), m_agentTypeName (agentTypeName)
	{ }
};

bool ModifierInfoChunkWriter::WriteChunkData (CCryFile & file) const
{
	bool ok = true;

	unsigned nameLen = m_name.length ();
	ok = ok && (0 != file.Write ( & nameLen, sizeof (nameLen)) );
	ok = ok && (0 != file.Write ( m_name.c_str (), nameLen) );

	ok = ok && (0 != file.Write ( & m_id, sizeof (m_id)) );

	ok = ok && (0 != file.Write ( & m_agentTypeId, sizeof (m_agentTypeId)) );

	unsigned agentTypeNameLen = m_agentTypeName.length ();
	ok = ok && (0 != file.Write ( & agentTypeNameLen, sizeof (agentTypeNameLen)) );
	ok = ok && (0 != file.Write ( m_agentTypeName.c_str (), agentTypeNameLen) );

	return ok;
}



} // namespace LayeredNavMesh


void CLayeredMeshGenerator::Export (const char * filename) const
{
	if (0 == m_pPolygonGen)
		return;

	const ::Vec3 sceneMin = ConvertVec3 (m_settings.sceneBounds->GetAABB().Min());
	const ::Vec3 sceneMax = ConvertVec3 (m_settings.sceneBounds->GetAABB().Max());

	// FIXME Aug 7, 2009: <pvl> fix the case of generating with no modifier and
	// therefore no explicit properties
	const AgentPathfindingProperties * pfProp = GetIEditor()->GetAI()->GetPFPropertiesOfPathType (m_agentType);
	const int agentTypeId = pfProp ? pfProp->id : 0;

	LayeredNavMesh::ExportFormat::FileWriter writer;
	writer.RegisterChunkWriter (new LayeredNavMesh::SceneBoundsChunkWriter (1, sceneMin, sceneMax));
	writer.RegisterChunkWriter (new LayeredNavMesh::ModifierInfoChunkWriter (2, m_navModifName, m_navModifIndex, agentTypeId, m_agentType));
	writer.RegisterChunkWriter (new LayeredNavMesh::VertexListChunkWriter (1, m_pPolygonGen));
	writer.RegisterChunkWriter (new LayeredNavMesh::PolygonListChunkWriter (1, m_pPolygonGen));
	// NOTE Aug 26, 2009: <pvl> enable for debugging
	//if (m_pVoxelizer)
	//	writer.RegisterChunkWriter (new LayeredNavMesh::SpanBufferChunkWriter (m_pVoxelizer->GetSpans ()));
	//if (m_pContourGen)
	//	writer.RegisterChunkWriter (new LayeredNavMesh::ContourListChunkWriter (m_pContourGen));

	writer.Write (filename);
}

void CLayeredMeshGenerator::Update () const
{
	static ICVar * lnmDrawAgentType = gEnv->pConsole->GetCVar ("ai_lnmDrawAgentType");
	assert (lnmDrawAgentType);
	const string agentTypeToBeDrawn = lnmDrawAgentType->GetString ();

	if (agentTypeToBeDrawn != m_agentType && agentTypeToBeDrawn != "all")
		return;

	if (m_pVoxelizer && s_cvarMgr->m_cvDrawVoxels->GetIVal())
		m_pVoxelizer->DebugDraw (m_settings);	

	if (m_pSurfaceGen && s_cvarMgr->m_cvDrawSurfaces->GetIVal())
		m_pSurfaceGen->DebugDraw (m_settings);

	if (m_pContourGen && s_cvarMgr->m_cvDrawCountours->GetIVal())
		m_pContourGen->DebugDraw (m_settings);

	if (m_pPolygonGen && s_cvarMgr->m_cvDrawPolygons->GetIVal())
		m_pPolygonGen->DebugDraw (m_settings);

	if (m_terrain && s_cvarMgr->m_cvDrawTerrain->GetIVal() && ! s_cvarMgr->m_cvDrawInputGeom->GetIVal())
		m_terrain->DebugDraw (
					LayeredNavMesh::Matrix4 (
						0, 0, 1, 0,
						1, 0, 0, 0,
						0, 1, 0, 0,
						0, 0, 0, 1
					)
		);

	if (s_cvarMgr->m_cvDrawInputGeom->GetIVal())
	{
		for (unsigned i = 0, ni = m_instances->Size(); i < ni; ++i)
		{
			GeomInstance* pInst = (*m_instances)[i];
			pInst->GetMesh()->DebugDraw (pInst->GetTM());
		}
	}
}


