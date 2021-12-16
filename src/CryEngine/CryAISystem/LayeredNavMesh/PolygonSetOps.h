
#ifndef POLYGON_SET_OPS_H
#define POLYGON_SET_OPS_H

#include <vector>

namespace PSO {

// NOTE Jul 16, 2009: <pvl> in Editor.dll, these seem to be #define'd by some
// M$ crap header
#undef CONJUNCTION
#undef DISJUNCTION
#undef DIFFERENCE
#undef SYMMETRICAL_DIFFERENCE

enum PolygonOperation { CONJUNCTION, DISJUNCTION, DIFFERENCE, SYMMETRICAL_DIFFERENCE };

class Allocator;
class Polygon;
class CrossVtxListMgr;

class PolygonSetOp {
	static Allocator * s_allocator;
	Allocator * m_allocator;

	Polygon * m_poly0;
	Polygon * m_poly1;

	Polygon * m_result;

	CrossVtxListMgr * m_xVtxListMgr;

	void Preprocess ();
	void AddCrossVertexDescriptors (Polygon * );
	void ExtractResult (PolygonOperation );
	void ExtractResult (PolygonOperation , Polygon * operand, int operandNum);

public:
	PolygonSetOp (Polygon * , Polygon * , Allocator * );

	~PolygonSetOp ();

	Polygon * Conjunction ();
	Polygon * Disjunction ();
	Polygon * Difference ();
	Polygon * SymmetricalDifference ();

	// FIXME Sep 24, 2008: <pvl> should ideally only be visible to implementation code
	static Allocator * GetCurAllocator ();
};


void DebugDrawSourcePolygon (const Polygon * );
void DebugDrawResultPolygon (const Polygon * );
void DebugDrawTesselatedPolygon (const Polygon * );

} // namespace PSO



// ---



#include "PolygonSetOpsUtil.h"

namespace PSO {

class CrossVtxDesc;
class VtxUserData;
typedef CircularList<CrossVtxDesc> PointConnectivityList;
typedef PointConnectivityList::iterator PointConnectivityIt;
typedef PointConnectivityList::loop_iterator PointConnectivityLoopIt;

#ifdef IN
#undef IN
#endif
#ifdef OUT
#undef OUT
#endif

enum Direction { IN = 0, OUT = 1 };
inline Direction Opposite (Direction dir) { if (dir == IN) return OUT; return IN; }

class Polygon {
public:
	class Vertex;
	class Edge;

	class Contour {
	public:
		// ATTN Nov 4, 2008: <pvl> labels are ordered by their relevance, don't
		// reorder them unless you know what you're doing
		enum Label { NOT_LABELED, OUTSIDE, INSIDE, INTERSECTED };
		typedef CircularList<Vertex>::iterator VertexIt;
		typedef CircularList<Vertex>::loop_iterator VertexLoopIt;
		typedef CircularList<Edge>::iterator EdgeIt;
		typedef CircularList<Edge>::loop_iterator EdgeLoopIt;
	private:
		CircularList<Vertex> m_vertices;
		CircularList<Edge> m_edges;
		Label m_label;

		Allocator * m_allocator;
	public:
		Contour (Allocator * allocator);
		~Contour ();

		VertexIt AddVertex (const Vertex & new_vertex);
		template <typename Iterator>
		VertexIt InsertVertex (const Iterator & previous, const Vertex & new_vertex);
		template <typename Iterator>
		void EraseVertex (Iterator & );
		unsigned NumVertices () const { return m_vertices.Size (); }

		void BuildEdges ();

		Label GetLabel () const { return m_label; }
		void SetLabel (Label new_label) { if (new_label > m_label) m_label = new_label; }
		void ResetLabel (Label new_label) { m_label = new_label; }

		enum Winding { CW, CCW };
		Winding ComputeWinding () const;

		VertexLoopIt CreateVertexLoopIt (TraversalDirection dir = FORWARD) const { return m_vertices.CreateLoopIt (dir); }

		EdgeLoopIt CreateEdgeLoopIt (TraversalDirection dir = FORWARD) const { return m_edges.CreateLoopIt (dir); }

		Allocator * GetAllocator () const { return m_allocator; }

		static void * operator new (size_t size, Allocator * allocator)
		{
			return allocator->Allocate (size);
		}
		static void operator delete (void * , Allocator * )
		{
			assert (0);
		}
		static void operator delete (void * )
		{
		}
	};

	class Edge {
	public:
		enum Label { NOT_LABELED, SHARED_SAME_DIR, SHARED_OPPOSITE_DIR, INSIDE, OUTSIDE };
	private:
		Contour::VertexIt m_prevVertex;
		Contour::VertexIt m_nextVertex;

		Label m_label;
		bool m_includedInResult;

	public:
		Edge (Contour::VertexIt prev, Contour::VertexIt next) :
				m_prevVertex (prev), m_nextVertex (next), m_label (NOT_LABELED),
				m_includedInResult (false)
		{ }

		bool IsIncludedInResult () const { return m_includedInResult; }
		void MarkAsIncludedInResult () { m_includedInResult = true; }

		Contour::VertexIt GetPrevVtx () const { return m_prevVertex; }
		Contour::VertexIt GetNextVtx () const { return m_nextVertex; }

		PointConnectivityIt GetPrevCrossVtxDesc () const
		{
			assert (m_prevVertex->IsCross());
			if ( ! m_prevVertex->IsCross ()) return 0;
			return m_prevVertex->GetCrossVtxDesc (OUT);
		}
		PointConnectivityIt GetNextCrossVtxDesc () const
		{
			assert (m_nextVertex->IsCross());
			if ( ! m_nextVertex->IsCross ()) return 0;
			return m_nextVertex->GetCrossVtxDesc (IN);
		}

		unsigned OwnerPolygon () const
		{
			assert (m_prevVertex->OwnerPolygon() == m_nextVertex->OwnerPolygon());
			return m_prevVertex->OwnerPolygon();
		}

		Label GetLabel () const { return m_label; }
		void SetLabel (Label new_label) { m_label = new_label; }

		static void * operator new (size_t size, Allocator * alloc)
		{
			return alloc->Allocate (size);
		}
		static void operator delete (void * , Allocator * )
		{
			assert (0);
		}
		static void Destroy (Edge * edge, Allocator * allocator)
		{
			if (edge == 0) return;

			edge->~Edge ();
			allocator->Deallocate (edge);
		}
	};

	class Vertex {

		const Vec3 m_pos;
		bool m_isCrossVtx;
		const unsigned m_ownerPoly;

		Contour::EdgeIt m_prevEdge;
		Contour::EdgeIt m_nextEdge;

		struct CrossVtxDescs {
			PointConnectivityIt m_inDesc;
			PointConnectivityIt m_outDesc;
			CrossVtxDescs (const PointConnectivityIt & in, const PointConnectivityIt & out) :
					m_inDesc (in), m_outDesc (out)
			{ }
			static void * operator new (size_t size)
			{
				return PolygonSetOp::GetCurAllocator()->Allocate (size);
			}
		};

		CrossVtxDescs * m_crossVtxDescs;

		VtxUserData * m_userData;

	public:
		Vertex (unsigned poly, Vec3 pos) :
				m_ownerPoly (poly), m_pos (pos), m_isCrossVtx (false),
				m_crossVtxDescs (0), m_prevEdge (0), m_nextEdge (0), m_userData (0)
		{ }

		Vec3 Pos () const { return m_pos; }
		void SetPrevEdge (Contour::EdgeIt prevEdge) { m_prevEdge = prevEdge; }
		void SetNextEdge (Contour::EdgeIt nextEdge) { m_nextEdge = nextEdge; }
		Contour::EdgeIt GetPrevEdge () const { return m_prevEdge; }
		Contour::EdgeIt GetNextEdge () const { return m_nextEdge; }

		bool IsCross () const { return m_isCrossVtx; }
		void SetCross () { m_isCrossVtx = true; }
		void AddCrossVtxDescs (const PointConnectivityIt & in, const PointConnectivityIt & out)
		{
			m_crossVtxDescs = new CrossVtxDescs (in, out);
		}

		PointConnectivityIt GetCrossVtxDesc (Direction dir) const
		{
			assert (m_crossVtxDescs);
			return dir == IN ? m_crossVtxDescs->m_inDesc : m_crossVtxDescs->m_outDesc;
		}

		unsigned OwnerPolygon () const { return m_ownerPoly; }

		void SetUserData (VtxUserData * userData) { assert (m_userData == 0); m_userData = userData; }
		void ResetUserData (VtxUserData * userData) { m_userData = userData; }
		VtxUserData * GetUserData () const { return m_userData; }

		static void * operator new (size_t size, Allocator * alloc)
		{
			return alloc->Allocate (size);
		}
		static void Destroy (Vertex * vtx, Allocator * allocator)
		{
			if (vtx == 0) return;

			if (vtx->m_userData)
				allocator->Deallocate (vtx->m_userData);

			if (vtx->m_crossVtxDescs)
			{
				allocator->Deallocate ( * vtx->m_crossVtxDescs->m_inDesc);
				allocator->Deallocate ( * vtx->m_crossVtxDescs->m_outDesc);
			}
			vtx->~Vertex ();
			allocator->Deallocate (vtx);
		}
	};

	// NOTE Sep 30, 2008: <pvl> use PolygonSetOp::GetCurAllocator() for allocator
	Polygon ();
	// NOTE Sep 30, 2008: <pvl> use the allocator passed in the argument
	Polygon (Allocator * );
	~Polygon ();

	unsigned NumContours () const { return m_contours.size (); }
	Contour * GetContour (unsigned num) const { assert (num < NumContours()); return m_contours[num]; }
	Contour * NewContour () const { return new (m_allocator) Contour (m_allocator); }
	void AddContour (Contour * new_contour) { m_contours.push_back (new_contour); }
	void AddClone (const Contour * new_contour, TraversalDirection );
	void DelContour (unsigned int contourIdx);

	Allocator * GetAllocator () const { return m_allocator; }

	// NOTE Sep 30, 2008: <pvl> allocate from the passed in allocator
	static void * operator new (size_t size, Allocator * allocator)
	{
		return allocator->Allocate (size);
	}
	static void operator delete (void * mem, Allocator * allocator)
	{
		allocator->Deallocate (mem);
	}
	static void Destroy (Polygon * polygon)
	{
		if (polygon == 0) return;

		Allocator * allocator = polygon->GetAllocator ();
		polygon->~Polygon ();
		allocator->Deallocate (polygon);
	}

private:
	// ATTN Sep 26, 2008: <pvl> 'm_allocator' has to come first in data member list.
	// It's possible and likely that this Polygon instance lives in its memory.
	// If this Polygon instance also holds the last reference to the allocator
	// the memory will be deallocated in this Polygon's destructor.  Obviously it's
	// important that this is that last thing to happen in ~Polygon().
	Allocator * m_allocator;

	std::vector <Contour*> m_contours;
};


template <typename Iterator>
inline Polygon::Contour::VertexIt Polygon::Contour::InsertVertex (const Iterator & previous, const Vertex & new_vertex)
{
	return m_vertices.Insert (previous, new (m_allocator) Vertex (new_vertex));
}

template <typename Iterator>
inline void Polygon::Contour::EraseVertex (Iterator & doomed)
{
	m_vertices.Erase (doomed);
}


// ---


class VtxUserData {
public:
	enum Type { ORIG, CROSS };
private:
	Type m_type;
protected:
	VtxUserData (Type type) : m_type (type) { }

public:
	Type GetType () const { return m_type; }
};

class OrigVtxUserData : public VtxUserData {
	void * m_userData;
public:
	OrigVtxUserData (void * userData) : VtxUserData (ORIG), m_userData (userData) { }

	void * GetData () const { return m_userData; }

	static void * operator new (size_t size, Allocator * alloc)
	{
		return alloc->Allocate (size);
	}
	static void operator delete (void * , Allocator * )
	{
		assert (0);
	}
};

class CrossVtxUserData : public VtxUserData {
public:
	struct Edge {
		VtxUserData * m_endPt[2];
		Edge (VtxUserData * endpt0, VtxUserData * endpt1)
		{
			m_endPt[0] = endpt0;
			m_endPt[1] = endpt1;
		}
	};

private:
	VtxUserData * m_vtx[4];

public:
	CrossVtxUserData (const Edge & e0, const Edge & e1) : VtxUserData (CROSS)
	{
		m_vtx[0] = e0.m_endPt[0];
		m_vtx[1] = e0.m_endPt[1];
		m_vtx[2] = e1.m_endPt[0];
		m_vtx[3] = e1.m_endPt[1];
	}

	void * GetData (unsigned index) const { return m_vtx[index]; }

	static void * operator new (size_t size, Allocator * alloc)
	{
		return alloc->Allocate (size);
	}
	static void operator delete (void * , Allocator * )
	{
		assert (0);
	}
};


void DumpPolygonForGnuplot (const Polygon * p, const string & polyName);

} // namespace PSO

#endif // POLYGON_SET_OPS_H
