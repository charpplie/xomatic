//
//  NavPolygonGenerator.h - Generates polygonized navmesh from nav contours.
//
//  Copyright (C) 2007-2008 Mikko Mononen
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
//  Mikko Mononen memon@inside.org
//

#ifndef NAVPOLYGONGENERATOR_H
#define NAVPOLYGONGENERATOR_H

#include "Vec3.h"
#include "NavMesh.h"
#include "NavContourGenerator.h"
#include "NavSurfaceGenerator.h"
#include "DynArray.h"

namespace LayeredNavMesh {

struct NavEdge
{
	static const unsigned short INVALID = 0xffff;
	unsigned int v;	// edge start vertex
	unsigned short p; // adjacent polygon.
};

struct NavPoly
{
	inline void Free() { delete [] edges; }
	unsigned region;
	unsigned short numEdges;
	NavEdge* edges;
};

class BitArray;

class NavPolygonGenerator
{
public:
	class NavPolyPtr {
		const NavPoly * m_poly;
		const NavPolygonGenerator * m_owner;

		friend class NavPolygonGenerator;
	public:
		// NOTE Jul 10, 2008: <pvl> originally private, meant to only be used by
		// NavPolygonGenerator.  This class mutates into something it wasn't meant for ...
		NavPolyPtr (const NavPoly * , const NavPolygonGenerator * );

		const Vec3 & GetVertex (int vertexIndex) const;
		Vec3 GetCentroid () const;
		int GetNumEdges () const;
		int GetNeighbourIndex (int edgeNumber) const;

		class vertex_iterator {
			const NavPolyPtr * m_poly;
			int m_currVertexIndex;
		public:
			vertex_iterator (const NavPolyPtr * , int vertexIndex = 0);
			vertex_iterator & operator++ ();
			bool operator== (const vertex_iterator & ) const;
			bool operator!= (const vertex_iterator & ) const;
			const Vec3 & operator* () const;
		};
		vertex_iterator begin () const;
		vertex_iterator end () const;

		// NOTE Jul 4, 2008: <pvl> this was added to support exporting of "raw"
		// data, prereferably don't use this function when using instances of this
		// class the expected way, i.e. to represent a polygon.
		// NOTE Jul 9, 2008: <pvl> now it's also used by (preliminary) implementation
		// of picking accelerator
		const NavPoly * GetInternalPoly () const { return m_poly; }
		// NOTE Jul 9, 2008: <pvl> for picking accelerator - is there a cleaner way?
		const NavPolygonGenerator * GetOwner () const { return m_owner; }
	};

	NavPolygonGenerator(const BuildSettings& settings);
	~NavPolygonGenerator();
	
	void BuildPolygons(NavContourGenerator* pContour);

	inline unsigned GetPolyCount() const { return m_polys.Size(); }
	inline const NavPolyPtr GetPoly(unsigned i) const { return NavPolyPtr (m_polys[i], this); }
	inline unsigned GetVertCount() const { return m_verts.Size(); }
	inline const Vec3& GetVert(unsigned i) const { return m_verts[i]; }

	// NOTE Jan 28, 2010: <pvl> we need to pass this in from the outside since
	// Graph complete with Smart Object nodes and links is necessary to compute it.
	void SetPolygonReachability (const BitArray * reachability);
	bool IsPolyReachable (unsigned polyIdx) const;

#ifdef DEBUG_DRAW
	void DebugDraw(const BuildSettings& settings);
	static const unsigned DEBUG_ITEM_COUNT = 3;
	DebugItem m_debugItems[DEBUG_ITEM_COUNT];
#endif

private:

	DynArray<NavPoly*> m_polys;
	DynArray<Vec3> m_verts;
	DynArray<int> m_vertNext;
	int* m_vertBuckets;

	const BitArray * m_reachable;

	void TessellateContours(const DynArray<const Contour*>& contours, unsigned region);
	void AddPolygon(const Vec3* pts, unsigned npts, unsigned region);
	unsigned AddVertex(const Vec3& v);
	void BuildAdjacency();

	const BuildSettings m_settings;
};



inline NavPolygonGenerator::NavPolyPtr::NavPolyPtr (
	const NavPoly * poly, const LayeredNavMesh::NavPolygonGenerator * owner) :
		m_poly(poly), m_owner(owner)
{
}

inline const Vec3 & NavPolygonGenerator::NavPolyPtr::GetVertex (int vertexIndex) const
{
	assert (vertexIndex < m_poly->numEdges);
	return m_owner->m_verts [m_poly->edges[vertexIndex].v];
}

inline int NavPolygonGenerator::NavPolyPtr::GetNumEdges () const
{
	return m_poly->numEdges;
}

inline int NavPolygonGenerator::NavPolyPtr::GetNeighbourIndex (int edgeNumber) const
{
	return m_poly->edges[edgeNumber].p;
}


inline NavPolygonGenerator::NavPolyPtr::vertex_iterator::vertex_iterator (const NavPolyPtr * poly, int vertexIndex) :
		m_poly (poly), m_currVertexIndex (vertexIndex)
{
}

inline NavPolygonGenerator::NavPolyPtr::vertex_iterator & NavPolygonGenerator::NavPolyPtr::vertex_iterator::operator++ ()
{
	++m_currVertexIndex;
	return *this;
}

inline bool NavPolygonGenerator::NavPolyPtr::vertex_iterator::operator== (const vertex_iterator & rhs) const
{
	return m_poly == rhs.m_poly && m_currVertexIndex == rhs.m_currVertexIndex;
}

inline bool NavPolygonGenerator::NavPolyPtr::vertex_iterator::operator!= (const vertex_iterator & rhs) const
{
	return ! ((*this) == rhs);
}

inline const Vec3 & NavPolygonGenerator::NavPolyPtr::vertex_iterator::operator* () const
{
	return m_poly->GetVertex (m_currVertexIndex);
}

inline NavPolygonGenerator::NavPolyPtr::vertex_iterator NavPolygonGenerator::NavPolyPtr::begin () const
{
	return vertex_iterator (this);
}

inline NavPolygonGenerator::NavPolyPtr::vertex_iterator NavPolygonGenerator::NavPolyPtr::end () const
{
	return vertex_iterator (this, m_poly->numEdges);
}

} // namespace LayeredNavMesh

#endif
