
#ifndef LNM_PICKING_ACCELERATOR_H
#define LNM_PICKING_ACCELERATOR_H

#include "NavMesh.h"	// NOTE Aug 1, 2008: <pvl> for DEBUG_DRAW #define

// NOTE Jan 21, 2010: <pvl> a stripped-down version of engine AABB.  Surprisingly
// enough there doesn't seem to be a 2D AABB class in the engine.
// TODO Jan 21, 2010: <pvl> templatise the engine AABB so that it can be instantiated
// as either 3D or 2d?  That would require adding operations to Vec2 that are in Vec3
// but not in Vec3 and removing all explicit references to 'z' in AABB code (probably
// moving them to the Vec[23] classes?).
struct AABB2 {

	Vec2 min;
	Vec2 max;

	void Reset () { min = Vec2(1e35f,1e35f); max = Vec2(-1e35f,-1e35f); }

	AABB2 (AABB::type_reset) { Reset(); }

	AABB2 (const Vec2 & vmin, const Vec2 & vmax) : min(vmin), max(vmax) { }

	AABB2 (const AABB & b) : min(b.min), max(b.max) { }

	void Add (const Vec2 & v)
	{
		min.x = std::min (v.x,min.x);
		min.y = std::min (v.y,min.y);

		max.x = std::max (v.x,max.x);
		max.y = std::max (v.y,max.y);
	}

	bool IsReset () const { return min.x > max.x; }

	Vec2 GetSize() const { return IsReset() ? Vec2(ZERO) : max-min; }

	bool IsContainPoint (const Vec2 & pos) const
	{
		assert (min.IsValid());
		assert (max.IsValid());
		assert (pos.IsValid());
		if (pos.x < min.x) return false;
		if (pos.y < min.y) return false;
		if (pos.x > max.x) return false;
		if (pos.y > max.y) return false;
		return true;
	}

	bool IsContainPoint (const Vec3 & pos) const
	{
		Vec2 pos2d (pos);
		return IsContainPoint (pos2d);
	}

	inline bool	IsIntersectBox (const AABB2 & b) const	
	{
		assert( min.IsValid() );
		assert( max.IsValid() );
		assert( b.min.IsValid() );
		assert( b.max.IsValid() );
		if ((min.x > b.max.x)||(b.min.x > max.x)) return false;
		if ((min.y > b.max.y)||(b.min.y > max.y)) return false;
		return true;
	}

	bool ContainsBox (const AABB2 & b) const
	{
		assert( min.IsValid() );
		assert( max.IsValid() );
		assert( b.min.IsValid() );
		assert( b.max.IsValid() );
		return min.x <= b.min.x && min.y <= b.min.y
				&& max.x >= b.max.x && max.y >= b.max.y;
	}
};


class PickingAccelerator
{
protected:

	virtual AABB2 GetPolyAABB (unsigned int poly) const = 0;
	virtual bool PointInConvexPolygon (unsigned int poly, const ::Vec3 & pos) const = 0;

public:
	virtual ~PickingAccelerator () { }

	// NOTE Aug 1, 2008: <pvl> would work for standalone polygons (i.e. those
	// which don't require any context, such as a polygon container)
//	virtual void Add (const void * ) = 0;
	
	// adding a polygon by index - it is assumed that subtype will know the context
	// NOTE Aug 1, 2008: <pvl> it's probably advisable for all polygons added
	// to the/ same accelerator to belong to the same polygon container.  The user
	// should make sure it's the case since the accelerator has no way of telling.
	virtual void Add (unsigned int ) = 0;

	virtual void Remove (unsigned int ) = 0;

	// use the swap trick to get the returned data without much copying
	// ??? virtual std::vector<Polygon> Pick (const SomeAreaPrimitive & ) const = 0;

//	virtual std::vector<const void * > Pick (const Vec3 & ) const = 0;
	// returning polygons by index - it is assumed that caller will know the context
	virtual std::vector<unsigned int> Pick (const Vec3 & ) const = 0;
	virtual std::vector<unsigned int> Pick (const AABB & ) const = 0;

#ifdef DEBUG_DRAW
	virtual void DebugDraw () const = 0;
#endif
};

// ---


class NaivePickingAcceleratorBase : public PickingAccelerator
{
	struct PolygonRec {
		// NOTE Jul 9, 2008: <pvl> NavPoly instances hold just indices into arrays
		// stored in their parent NavPolygonGenerator so they aren't stand-alone.
		// See also m_polygonOwner.
		unsigned int m_poly;
		AABB2 m_aabb;
		PolygonRec (unsigned int poly, const AABB2 & aabb) :
				m_poly(poly), m_aabb(aabb)
		{ }
	};

	std::vector <PolygonRec> m_polys;

public:
	NaivePickingAcceleratorBase ();

	virtual void Add (unsigned int poly);
	virtual std::vector<unsigned int> Pick (const ::Vec3 & ) const;

	//TODO: (from Markus for Pavel) This was just added to get rid of a PS3 compiler warning. But it should be added anyway
	virtual std::vector<unsigned int> Pick (const AABB & ) const { return std::vector<unsigned int>(); }

#ifdef DEBUG_DRAW
	// NOTE Jul 16, 2008: <pvl> nothing to draw here really
	virtual void DebugDraw () const { }
#endif
};

template <typename PolygonContext>
class NaivePickingAccelerator : public NaivePickingAcceleratorBase
{
	PolygonContext m_polygonContext;

	virtual AABB2 GetPolyAABB (unsigned int poly) const
	{
		return m_polygonContext.GetPolyAABB (poly);
	}
	virtual bool PointInConvexPolygon (unsigned int poly, const ::Vec3 & pos) const
	{
		return m_polygonContext.PointInConvexPolygon (poly, pos);
	}
public:
	NaivePickingAccelerator (const PolygonContext & polyCtx) :
			m_polygonContext (polyCtx)
	{	}
};

// ---

class QuadtreePickingAcceleratorBase : public PickingAccelerator {
public:
	struct Stats {
		unsigned m_numNodes;
		unsigned m_numNodesWithNumChildren[4];
		unsigned m_numPolygonsInNode[10];
		Stats ();
		void Print () const;
	};

private:
	class Node {
		// FIXME Jul 15, 2008: <pvl> much the same as in the naive accelerator
		struct PolygonRec {
			unsigned int m_poly;
			AABB2 m_aabb;
			PolygonRec (unsigned int poly, const AABB2 & aabb) :
					m_poly(poly), m_aabb(aabb)
			{ }
		};

		class PolygonRecCmp {
			unsigned int m_poly;
		public:
			PolygonRecCmp (unsigned int poly) : m_poly(poly) { }
			bool operator() (const PolygonRec & polyRec) { return polyRec.m_poly == m_poly; }
		};

		AABB2 m_aabb;
		Node * m_children[4];
		std::vector <PolygonRec> m_polys;

		enum Quadrant { UL, UR, LL, LR };		// upper-left, lower-right etc. in maths convention
		Quadrant GetQuadrant (const ::Vec3 & pos) const;
		bool QuadrantsAreNeighbors (Quadrant , Quadrant ) const;
		void AllocateChild (Quadrant );
		void DeallocateChild (Quadrant );
		void AddToChild (Quadrant childQ, unsigned int poly, const AABB2 & polyAABB, float leafNodeSizeSqr);
		void AddToThis (unsigned int poly, const AABB2 & polyAABB);
		void RemoveFromChild (Quadrant childQ, unsigned int poly, const AABB2 & polyAABB);
		void RemoveFromThis (unsigned int poly);
		bool NodeSubdivideable (float leafNodeSizeSqr) const;
		bool Empty () const;
	public:
		Node (const ::Vec3 & min, const ::Vec3 & max);
		~Node ();

		const AABB2 & GetAABB () const { return m_aabb; }

		// TODO Jan 22, 2010: <pvl> it's kind of ugly to have to pass 'leafNodeSizeSqr'
		// around like this.  The API's could be made cleaner if the Add() operation
		// is not recursive but driven by an explicit loop in the quad tree class
		// where the subdivision criterion is directly available.
		void Add (unsigned int poly, const AABB2 & polyAABB, float leafNodeSizeSqr);
		void Remove (unsigned int poly, const AABB2 & polyAABB);

		void FreeUnusedMemory ();

		// FIXME Jul 25, 2008: <pvl> the third arg is UGLY!!!  Is there another way?
		void Pick (const ::Vec3 & pos, std::vector<unsigned int> & matches, const QuadtreePickingAcceleratorBase * ) const;
		void Pick (const AABB2 & area, std::vector<unsigned int> & matches, const QuadtreePickingAcceleratorBase * ) const;

		void CollectStats (Stats * ) const;
#ifdef DEBUG_DRAW
		void DebugDraw (int level) const;
#endif
	};

	Node * m_root;

	// NOTE Jan 22, 2010: <pvl> if a node's diagonal is less than the square root
	// of this, the node can't be further subdivided
	const float m_leafNodeSizeSqr;

public:
	QuadtreePickingAcceleratorBase (const ::Vec3 & min, const ::Vec3 & max, float leafNodeSize);
	virtual ~QuadtreePickingAcceleratorBase ();

	virtual void Add (unsigned int poly);
	virtual void Remove (unsigned int poly);
	virtual std::vector<unsigned int> Pick (const ::Vec3 & ) const;
	virtual std::vector<unsigned int> Pick (const AABB & ) const;

	void FreeUnusedMemory ();

	Stats CollectStats () const;

#ifdef DEBUG_DRAW
	virtual void DebugDraw () const;
#endif
};

inline QuadtreePickingAcceleratorBase::Stats::Stats () : m_numNodes(0)
{
	for (unsigned i=0; i<4; ++i)
		m_numNodesWithNumChildren[i] = 0;
	for (unsigned i=0; i<10; ++i)
		m_numPolygonsInNode[i] = 0;
}

template <typename PolygonContext>
class QuadtreePickingAccelerator : public QuadtreePickingAcceleratorBase
{
	PolygonContext m_polygonContext;

	virtual AABB2 GetPolyAABB (unsigned int poly) const
	{
		return m_polygonContext.GetPolyAABB (poly);
	}
	virtual bool PointInConvexPolygon (unsigned int poly, const ::Vec3 & pos) const
	{
		return m_polygonContext.PointInConvexPolygon (poly, pos);
	}
public:
	QuadtreePickingAccelerator (const ::Vec3 & min, const ::Vec3 & max, float leafNodeSize, const PolygonContext & polyCtx) :
			QuadtreePickingAcceleratorBase (min,max,leafNodeSize), m_polygonContext (polyCtx)
	{	}
};


// ---

namespace LayeredNavMesh { class NavPolygonGenerator; }

struct EditorPolygonContext {
	const LayeredNavMesh::NavPolygonGenerator * m_polyGen;

	AABB2 GetPolyAABB (unsigned int poly) const;
	bool PointInConvexPolygon (unsigned int poly, const ::Vec3 & pos) const;

	EditorPolygonContext (const LayeredNavMesh::NavPolygonGenerator * polyGen) :
			m_polyGen(polyGen)
	{ }
};


namespace LayeredNavMesh { class PolygonCont; }

struct RuntimePolygonContext {
	const LayeredNavMesh::PolygonCont * m_polyCont;

	AABB2 GetPolyAABB (unsigned int poly) const;
	bool PointInConvexPolygon (unsigned int poly, const ::Vec3 & pos) const;

	RuntimePolygonContext (const LayeredNavMesh::PolygonCont * polyCont) :
			m_polyCont(polyCont)
	{ }
};


#endif // LNM_PICKING_ACCELERATOR_H
