
#ifndef LNM_DYNAMIC_OBSTACLE_HANDLING_H
#define LNM_DYNAMIC_OBSTACLE_HANDLING_H

#include "../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/PolygonCont.h"
#include "DynamicVertex.h"

// NOTE Okt 16, 2008: <pvl> just DynamicOverlay lives here at the moment
// (it needs to be accessible from multiple .cpp's) but it might be a good idea
// to move most or all of dynamics handling from LNM nav region over here.

// NOTE Sep 23, 2008: <pvl> one instance per OverlaidNavPoly, might be better
// nested in it
// UPDATE Okt 13, 2008: <pvl> come to think of it, it might not as doing so would
// probably tie both of them together too tightly

// NOTE Okt 13, 2008: <pvl> similar to PolygonCont but just for subdivisions
// FIXME Okt 16, 2008: <pvl> this will need to be a class to encapsulate index
// translation if not for anything else

// FIXME Okt 24, 2008: <pvl> rename this to DynamicNavMeshOverlay
// or something to differentiate it from DynamicGraphOverlay
struct DynamicOverlay {

	// FIXME Okt 10, 2008: <pvl> account for region as well

	typedef LayeredNavMesh::PolygonCont::InternalPolygon Polygon;

	static const unsigned INVALID_NEIGHBOR;
	static const unsigned short INVALID_VTX_INDEX;

	// NOTE Okt 17, 2008: <pvl> (most) externally observable indices (both polygon
	// and vertex ones) need to be offset by 'm_baseIndex' so that they can be
	// later recognized as belonging to a particular DynamicOverlay instance
	unsigned int m_baseIndex;

	// FIXME Okt 10, 2008: <pvl> optimize memory usage of this class (by getting
	// rid of std::vectors?)

	std::vector< ::Vec3> m_vertices;
	VtxAttribs m_vtxAttribs;

	LayeredNavMesh::StridedArray m_indices;
	// TODO Okt 13, 2008: <pvl> consider freeing up this memory if it turns out
	// unused once nav graph has been rewired
	std::vector<unsigned int> m_neighbours;
	std::vector<Polygon> m_polygons;

	DynamicOverlay ();

	unsigned short GetVtxIndex (const VtxAttribs::Attrib * vtx) const;
	unsigned short GetVtxIndex (const Vec3 & pos) const;

	unsigned short NextPolygonVtxIndex (const Polygon & poly, unsigned short vtx) const;
	unsigned short NextPolygonVtxIndex (unsigned polyIndex, unsigned short vtx) const;

	// FIXME May 26, 2009: <pvl> make the first one private (that's the workhorse function)
	unsigned GetPolyFromOutsideEdge (unsigned short endPt0Index, unsigned short endPt1Index) const;
	unsigned GetPolyFromOutsideEdge (const Vec3 & endPt0, const Vec3 & endPt1) const;
	unsigned GetPolyFromOutsideEdge (const VtxAttribs::Attrib * endPt0, const VtxAttribs::Attrib * endPt1) const;

	unsigned GetBaseIndex () const { return m_baseIndex; }
	void SetBaseIndex (unsigned baseIndex) { m_baseIndex = baseIndex; }

	// NOTE Okt 16, 2008: <pvl> to mimic PolygonCont interface
	LayeredNavMesh::PolygonCont::Polygon operator[] (unsigned int polygonIndex) const;
	const Vec3 & GetVertex (unsigned vtxIndex) const { return m_vertices[vtxIndex]; }

	void DebugDraw () const;
};

// NOTE Okt 21, 2008: <pvl> setting stride of 'm_indices' to 1 since I figure
// most dynamically retessellated meshes should have less than 256 vertices
// but it's really up to a higher layer to set this before entering the actual data.
inline DynamicOverlay::DynamicOverlay () : m_baseIndex (0), m_indices (1)
{
}

inline unsigned short DynamicOverlay::NextPolygonVtxIndex (const Polygon & poly, unsigned short vtx) const
{
	return (vtx+1) % poly.m_numIndices + poly.m_startIndex;
}

inline unsigned short DynamicOverlay::NextPolygonVtxIndex (unsigned polyIndex, unsigned short vtx) const
{
	const Polygon & poly = m_polygons[polyIndex];
	return NextPolygonVtxIndex (poly, vtx);
}

inline LayeredNavMesh::PolygonCont::Polygon
DynamicOverlay::operator[] (unsigned int polygonIndex) const
{
	return LayeredNavMesh::PolygonCont::Polygon ( & m_polygons[polygonIndex], & m_indices, & m_vertices, & m_vtxAttribs, m_baseIndex);
}

#endif // LNM_DYNAMIC_OBSTACLE_HANDLING_H