
#include "StdAfx.h"

// NOTE Jun 22, 2009: <pvl> the following #include's are enough to get rid of
// the PCH at the time of this writing.  If you wish to do so comment out the line
// '#include "StdAfx.h"' above, don't #define USE_STDAFX and set the file to
// not use PCH in properties.  (This can be useful e.g. whenever this file needs
// to be compiled with settings not matching those of the PCH.)
#define USE_STDAFX 1
#ifndef USE_STDAFX
#include "Cry_Math.h"
#include "Cry_Vector2.h"
#include "Cry_Vector3.h"
#include "Cry_Geo.h"
#include "Cry_Color.h"
#include "IRenderAuxGeom.h"
#include "IRenderer.h"
#include <map>
#endif // USE_STDAFX

#include "PolygonSetOps.h"
// TODO Aug 26, 2008: <pvl> if we end up using this header in the final version
// of this code consider moving it to a more "general" location
// TODO Aug 26, 2008: <pvl> also if that's the case it might be better to
// templatize it so that it doesn't use doubles (which it does ATM I believe)
#include "../../../CryEngine/CryAISystem/PolygonSetOps/LineSeg.h"

#include "../../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/env.h"	// for Stopwatch

//#include "../../../CryEngine/CryAISystem/DebugDrawContext.h"

// NOTE Aug 15, 2008: <pvl> an implementation of "A Closed Set of Algorithms
// for Performing Set Operations on Polygonal Regions in the Plane" by
// Leonov and Nikitin.  See also "An Edge Labeling Approach to Concave
// Polygon Clipping" by Schutte.

//#pragma optimize ("", off)
//#pragma inline_depth (0)

// ---

namespace PSO {

Polygon::Contour::Contour (Allocator * allocator) :
		m_label (NOT_LABELED), m_allocator (allocator), m_vertices (allocator),
		m_edges (allocator)
{
}

Polygon::Contour::~Contour ()
{
	EdgeLoopIt edgeIt = m_edges.CreateLoopIt ();
	for ( ; edgeIt; ++edgeIt)
		Edge::Destroy ( *edgeIt, m_allocator );

	VertexLoopIt vtxIt = m_vertices.CreateLoopIt ();
	for ( ; vtxIt; ++vtxIt)
		Vertex::Destroy ( *vtxIt, m_allocator );
}

Polygon::Contour::Winding Polygon::Contour::ComputeWinding () const
{
	float total = 0.0f;

	VertexLoopIt vtxIt = CreateVertexLoopIt ();

	const Vec3 & offPos = vtxIt->Pos ();
	for ( ; vtxIt; ++vtxIt)
	{
		const Vec3 &thisPos = vtxIt->Pos ();
		const Vec3 &nextPos = vtxIt.GetNext()->Pos ();
		total += (thisPos.x - offPos.x) * (nextPos.y - offPos.y) - (nextPos.x - offPos.x) * (thisPos.y - offPos.y);
	}
	return total > 0.0f ? CCW : CW;
}

// ATTN Aug 26, 2008: <pvl> this should only be used during the initial building
// of a contour from a properly ordered and wound sequence of polygon vertices
Polygon::Contour::VertexIt Polygon::Contour::AddVertex (const Vertex & e)
{
	return m_vertices.Add (new (m_allocator) Vertex (e));
}

void Polygon::Contour::BuildEdges ()
{
	// NOTE Sep 24, 2008: <pvl> check if edges have been built already
	// UPDATE Mar 23, 2009: <pvl> not necessarily very robust (however robustness
	// hasn't been required here)
	if (m_edges.Size () == m_vertices.Size ()) return;

	m_edges.Clear ();

	VertexLoopIt vtxIt = CreateVertexLoopIt ();
	for ( ; vtxIt; ++vtxIt)
	{
		EdgeIt newEdgeIt = m_edges.Add (new (m_allocator) Edge (vtxIt, vtxIt.GetNext ()));
		vtxIt->SetNextEdge (newEdgeIt);
		vtxIt.GetNext()->SetPrevEdge (newEdgeIt);
	}
}

Polygon::Polygon () : m_allocator (PolygonSetOp::GetCurAllocator())
{
}

Polygon::Polygon (Allocator * allocator) : m_allocator (allocator)
{
}

Polygon::~Polygon ()
{
	std::vector <Contour*>::iterator it = m_contours.begin();
	std::vector <Contour*>::iterator end = m_contours.end();
	for ( ; it != end; ++it)
		delete *it;
}

void Polygon::AddClone (const Contour * orig, TraversalDirection dir)
{
	Contour * newContour = new (m_allocator) Contour (m_allocator);

	Contour::VertexLoopIt it = orig->CreateVertexLoopIt (dir);
	for ( ; it; ++it)
		newContour->AddVertex (**it);

	AddContour (newContour);
}

void Polygon::DelContour (unsigned int contourIdx)
{
	assert (contourIdx < m_contours.size ());
	m_contours.erase (m_contours.begin () + contourIdx);
}

// ---

const string & GetString (Polygon::Edge::Label label)
{
	static const string strings[] = { "NOT LABELLED!!!", "SHARED SAME DIR", "SHARED OPPOSITE DIR", "INSIDE", "OUTSIDE" };
	return strings [label];
}

void DebugDrawSourceContour (Polygon::Contour * contour)
{




















}

void DebugDrawContour (Polygon::Contour * contour, const ColorB & col, float vertOffset)
{

















}

void DebugDrawResultContour (Polygon::Contour * contour)
{
	DebugDrawContour (contour, ColorB (255, 64, 64, 128), 15.0f);
}

void DebugDrawTesselatedContour (Polygon::Contour * contour)
{
	DebugDrawContour (contour, ColorB (64, 255, 64, 128), 10.0f);
}

void DebugDrawSourcePolygon (const Polygon * polygon)
{
	for (int c=0, numContours = polygon->NumContours(); c < numContours; ++c)
		DebugDrawSourceContour (polygon->GetContour (c));
}

void DebugDrawResultPolygon (const Polygon * polygon)
{
	for (int c=0, numContours = polygon->NumContours(); c < numContours; ++c)
		DebugDrawResultContour (polygon->GetContour (c));
}

void DebugDrawTesselatedPolygon (const Polygon * polygon)
{
	for (int c=0, numContours = polygon->NumContours(); c < numContours; ++c)
		DebugDrawTesselatedContour (polygon->GetContour (c));
}

// ---


// an intersection point along an edge
// NOTE Sep 24, 2008: <pvl> 'm_vertex' and 'm_next' form an edge of the original
// polygon outline
// NOTE Mar 19, 2009: <pvl> this class' purpose and implementation mutated enough
// with time that review and refactor is warranted I guess.  At least the name
// seems to be too general considering that it's only used to store edge intersections.
struct PointOnEdge {
	// TODO Sep 10, 2008: <pvl> consider converting these just to positions -
	// the relation to the original vertices is unimportant and even dubious
	// during polygon augmentation by cross-vertices
	const Polygon::Contour::VertexIt m_vertex;

	// NOTE Sep 24, 2008: <pvl> we need to store what m_vertex's next vertex was
	// at the time the parameter ('m_t') was computed to protect the meaning of 'm_t'.
	// Any vertex's next vertex can change while cross vertices are being added.
	// Without storing 'm_next', in case of edges intersected multiple times
	// obviously just the first cross vertex's position would be computed correctly.
	// See also Compare.
	const Polygon::Contour::VertexIt m_next;

	const Polygon::Contour::VertexIt m_otherVertex;
	const Polygon::Contour::VertexIt m_otherNext;

	const float m_t;								// parameter of the intersection point (<0.0, 1.0>)

	// NOTE Mar 19, 2009: <pvl> this is a bit convoluted.  We store the geom
	// location now as well since PointOnEdge actually only ever stores edge
	// intersections.  There's always a sibling PointOnEdge instance somewhere
	// that describes the same intersection point, just from the other edge
	// point of view (as an 'm_t' into the other edge).  With floating point
	// being what it is using a 't' into one edge doesn't seem to guarantee
	// the same result as using a corresponding 't' into the other edge even
	// though the t's were computed for the intersection in the first place.
	// We need to compute the location of the intersection point just once and
	// store it in both sibling PointOnEdge instances to make sure they match.
	//
	// BTW it might seem that having an "Intersection" structure that held
	// the location and pointed to two location-less PointOnEdges would solve
	// the problem more elegantly.  It used to be like that but it turned out
	// impractical since during augmenting edges with intersections the intersections
	// need to be sorted along the edges at which point the sibling PointOnEdges
	// would become separated from each other and their parent "Intersection" instance.
	const Vec3 m_geomLocation;

	PointOnEdge (
				const Polygon::Contour::VertexIt & e,
				const Polygon::Contour::VertexIt & n,
				const Polygon::Contour::VertexIt & othere,
				const Polygon::Contour::VertexIt & othern,
				float t, const Vec3 & loc) :
			m_vertex(e), m_next(n), m_otherVertex(othere), m_otherNext(othern), m_t(t),
			m_geomLocation (loc)
	{ }

	Vec3 Pt () const { return m_geomLocation; }

	// NOTE Sep 24, 2008: <pvl> this is used while augmenting the original polygon
	// contours with cross vertices.  The idea is that we don't care which order
	// edges are processed in but within one edge we want to add vertices from
	// the end towards the beginning since that allows us add vertices conveniently
	// using 'm_vertex' as the previous vertex.
	struct Compare {
		bool operator() (const PointOnEdge * lhs, const PointOnEdge * rhs)
		{
			if (*lhs->m_vertex < *rhs->m_vertex) return true;
			if (*lhs->m_vertex > *rhs->m_vertex) return false;
			// NOTE Aug 28, 2008: <pvl> that's right, we want instances with *bigger*
			// 'm_t' to compare as *smaller*
			return lhs->m_t > rhs->m_t;
		}
	};

	struct Equal {
		bool operator() (const PointOnEdge * lhs, const PointOnEdge * rhs)
		{
			// TODO Sep 8, 2008: <pvl> better tolerance!
			if (*lhs->m_vertex == *rhs->m_vertex
						&& *lhs->m_otherVertex == *rhs->m_otherVertex
						&& fabs (lhs->m_t - rhs->m_t) < 0.0000001f)
				return true;
			return false;
		}
	};

	static void * operator new (size_t size)
	{
		return PolygonSetOp::GetCurAllocator()->Allocate (size);
	}
};


typedef float FltType;

// ATTN Jun 2, 2009: <pvl> there's no explicit guarding against negative return value
float GetEpsilon (float value)
{
	float next (value);
	++*(unsigned * )&next;
	return next - value;
}

bool PtOnLine (const LineSeg<Vec2> & lineSeg, const Vec2 & pt)
{
	const Vec2_tpl<FltType> lineSegDir (lineSeg.Dir ());
	const Vec2_tpl<FltType> ptToSegStartDir (lineSeg.Start() - pt);

	const FltType cross = lineSegDir.Cross (ptToSegStartDir);
	const FltType distSqrd = cross*cross / lineSegDir.GetLength2 ();

	const FltType errX = GetEpsilon (pt.x);
	const FltType errY = GetEpsilon (pt.y);
	const bool onLine = distSqrd < (errX*errX + errY*errY)/4.0;

	return onLine;
}

class Intersection {
	std::pair <FltType,FltType> m_intersections[2];
	unsigned m_numIntersections;

	void ProcessCollinear (const LineSeg<Vec2> & seg0, const LineSeg<Vec2> & seg1);
public:
	Intersection (const LineSeg<Vec2> & seg0, const LineSeg<Vec2> & seg1);
	operator bool () const { return m_numIntersections > 0; }
	unsigned Count () const { return m_numIntersections; }
	const std::pair<FltType,FltType> & operator[] (unsigned index) const;
};

void Intersection::ProcessCollinear (const LineSeg<Vec2> & seg0, const LineSeg<Vec2> & seg1)
{
	FltType t00 = seg0.GetParam (seg1.Start ());
	if (t00 >= 0.0f && t00 <= 1.0f)
	{
		m_intersections[m_numIntersections].first = t00;
		m_intersections[m_numIntersections].second = 0.0f;
		++m_numIntersections;
	}
	FltType t01 = seg0.GetParam (seg1.End ());
	if (t01 >= 0.0f && t01 <= 1.0f)
	{
		m_intersections[m_numIntersections].first = t01;
		m_intersections[m_numIntersections].second = 1.0f;
		++m_numIntersections;
	}

	// NOTE Dez 2, 2008: <pvl> note the strict inequalities here.  If 't10' or
	// 't11' come out 0.0 or 1.0 then the segments share an endpoint.  That
	// case has been already handled above though (if segments share an endpoint
	// then either 't00' or 't01' must have been 0.0 or 1.0 and the non-strict
	// inequalities above must have caught that) so we don't want to handle
	// it here again as that would result in duplicate intersections.
	FltType t10 = seg1.GetParam (seg0.Start ());
	if (t10 > 0.0f && t10 < 1.0f)
	{
		assert (m_numIntersections < 2);
		m_intersections[m_numIntersections].first = 0.0f;
		m_intersections[m_numIntersections].second = t10;
		++m_numIntersections;
	}
	FltType t11 = seg1.GetParam (seg0.End ());
	if (t11 > 0.0f && t11 < 1.0f)
	{
		assert (m_numIntersections < 2);
		m_intersections[m_numIntersections].first = 1.0f;
		m_intersections[m_numIntersections].second = t11;
		++m_numIntersections;
	}
}

Intersection::Intersection (const LineSeg<Vec2> & seg0, const LineSeg<Vec2> & seg1) :
		m_numIntersections(0)
{
	// NOTE Jun 2, 2009: <pvl> bbox check
	const float seg0_minX = std::min (seg0.Start().x, seg0.End().x);
	const float seg0_maxX = std::max (seg0.Start().x, seg0.End().x);
	const float seg0_minY = std::min (seg0.Start().y, seg0.End().y);
	const float seg0_maxY = std::max (seg0.Start().y, seg0.End().y);
	const float seg1_minX = std::min (seg1.Start().x, seg1.End().x);
	const float seg1_maxX = std::max (seg1.Start().x, seg1.End().x);
	const float seg1_minY = std::min (seg1.Start().y, seg1.End().y);
	const float seg1_maxY = std::max (seg1.Start().y, seg1.End().y);
	if (seg0_minX > seg1_maxX || seg0_maxX < seg1_minX || seg0_minY > seg1_maxY || seg0_maxY < seg1_minY)
		return;

	typedef Vec2_tpl <FltType> Vec2;

	const Vec2 delta = seg0.Start() - seg1.Start();
	const FltType cross0 = seg0.Dir().Cross (seg1.Dir ());

	const FltType cross1 = seg0.Dir().Cross (delta);
	const FltType cross2 = seg1.Dir().Cross (-delta);

	// TODO May 29, 2009: <pvl> it might be a good idea to check for any endpoints
	// lying on the other segment's endpoints as a special case before going
	// into anything else.  The code that follows could handle these cases but
	// I'm not sure if it's guaranteed to give clean 0.0's and 1.0's for both
	// segments when their endpoints overlap.

	const bool seg1startCoincides = PtOnLine (seg0, seg1.Start());
	const bool seg1endCoincides = PtOnLine (seg0, seg1.End());
	const bool seg0startCoincides = PtOnLine (seg1, seg0.Start());
	const bool seg0endCoincides = PtOnLine (seg1, seg0.End());

	if ( (seg1startCoincides && seg1endCoincides) || (seg0startCoincides && seg0endCoincides) )
	{
		ProcessCollinear (seg0, seg1);
		return;
	}
	else if (seg1startCoincides)
	{
		FltType t = seg0.GetParam (seg1.Start ());
		// TODO Apr 24, 2009: <pvl> sharp inequalities or not?
		if (t > 0.0f && t < 1.0f)
		{
			m_numIntersections = 1;
			m_intersections[0].first = t;
			m_intersections[0].second = 0.0f;
			return;
		}
	} else if (seg1endCoincides)
	{
		FltType t = seg0.GetParam (seg1.End ());
		if (t > 0.0f && t < 1.0f)
		{
			m_numIntersections = 1;
			m_intersections[0].first = t;
			m_intersections[0].second = 1.0f;
			return;
		}
	} else if (seg0startCoincides)
	{
		FltType t = seg1.GetParam (seg0.Start ());
		if (t > 0.0f && t < 1.0f)
		{
			m_numIntersections = 1;
			m_intersections[0].first = 0.0f;
			m_intersections[0].second = t;
			return;
		}
	} else if (seg0endCoincides)
	{
		FltType t = seg1.GetParam (seg0.End ());
		if (t > 0.0f && t < 1.0f)
		{
			m_numIntersections = 1;
			m_intersections[0].first = 1.0f;
			m_intersections[0].second = t;
			return;
		}
	}

	if (cross0 != 0.0f)
	{
		const FltType t0 = cross1 / cross0;
		const FltType t1 = cross2 / -cross0;
		if (t0 >= 0.0f && t0 <= 1.0f && t1 >= 0.0f && t1 <= 1.0f)
		{
			m_numIntersections = 1;
			m_intersections[0].first = t1;
			m_intersections[0].second = t0;
		}
	}
	else
	{
		// NOTE Aug 26, 2008: <pvl> the segments are parallel, check if also collinear

		// NOTE Aug 26, 2008: <pvl> in principle using cross2 here should work too
		const FltType cross1norm = cross1 / (seg0.Dir().GetLength() * delta.GetLength ());	// normalize cross1
		if (fabs (cross1norm) < 0.0000001f)
		{
			// NOTE Aug 26, 2008: <pvl> they're collinear
			ProcessCollinear (seg0, seg1);
		}
	}
}

const std::pair<FltType,FltType> & Intersection::operator[] (unsigned index) const
{
	assert (m_numIntersections <= 2);
	assert (index < m_numIntersections);
	return m_intersections[index];
}

// ---

typedef FixedSizeArray<PointOnEdge*> EdgeIntersectionList;
// NOTE Sep 8, 2008: <pvl> basically just a convenience replacement of std::pair
struct EdgeIntersectionLists {
	// FIXME Mar 26, 2009: <pvl> change the following names, they don't make
	// any sense anymore!
	EdgeIntersectionList m_poly0;
	EdgeIntersectionList m_poly1;
	EdgeIntersectionLists (unsigned reserve) :
			m_poly0 (reserve, PolygonSetOp::GetCurAllocator ()),
			m_poly1 (reserve, PolygonSetOp::GetCurAllocator ())
	{ }
};

// NOTE Aug 15, 2008: <pvl> our polygons are expected to have very low vertex
// counts, I suspect brute force with its zero overhead will be faster here
// than any of the more sophisticated space partitioning or space ordering
// algorithms ... (see also Further Comparison of Algorithms for Geometric
// Intersection Problems by Andrews et al.)
EdgeIntersectionLists FindEdgeIntersections (const Polygon::Contour * cntr0, const Polygon::Contour * cntr1)
{
	EdgeIntersectionLists isectLists (cntr0->NumVertices() * cntr1->NumVertices());

	Polygon::Contour::VertexLoopIt vtxIt0 = cntr0->CreateVertexLoopIt();

	for ( ; vtxIt0; ++vtxIt0)
	{
		Polygon::Contour::VertexLoopIt vtxIt1 = cntr1->CreateVertexLoopIt();
		for ( ; vtxIt1; ++vtxIt1)
		{
			Intersection isect (
						LineSeg<Vec2> (Vec2 (vtxIt0->Pos()), Vec2 (vtxIt0.GetNext()->Pos())),
						LineSeg<Vec2> (Vec2 (vtxIt1->Pos()), Vec2 (vtxIt1.GetNext()->Pos()))
			);

			if (isect)
			{
				for (int i=0, numIsect=isect.Count(); i < numIsect; ++i)
				{
					// NOTE May 27, 2009: <pvl> if the intersection coincides with
					// one of the endpoints use that endpoint's location.  This is
					// required because of the way Augment() works.  Bad things could
					// happen in cases where an endpoint of one segment lies in the
					// interior of the other. For that segment, the existing endpoint
					// would just be turned into a cross-vertex but for the other,
					// a new cross-vertex would be added.  If that new vertex
					// location was computed the way it's computed for proper
					// intersections it wouldn't necessarily match the other
					// cross-vertex location, putting things out of synch and
					// breaking edge labelling.
					Vec3 loc;
					if (isect[i].first == 0.0f)
						loc = vtxIt0->Pos();
					else if (isect[i].first == 1.0f)
						loc = vtxIt0.GetNext()->Pos();
					else if (isect[i].second == 0.0f)
						loc = vtxIt1->Pos();
					else if (isect[i].second == 1.0f)
						loc = vtxIt1.GetNext()->Pos();
					else
					{
						const Vec3 loc0 (LineSeg<Vec3> (vtxIt0->Pos(), vtxIt0.GetNext()->Pos()).Pt (isect[i].first));
						const Vec3 loc1 (LineSeg<Vec3> (vtxIt1->Pos(), vtxIt1.GetNext()->Pos()).Pt (isect[i].second));
						loc = loc0 / 2.0f + loc1 / 2.0f;
					}

					isectLists.m_poly0.PushBack (
								new PointOnEdge (vtxIt0, vtxIt0.GetNext(), vtxIt1, vtxIt1.GetNext(), isect[i].first, loc)
					);
					isectLists.m_poly1.PushBack (
								new PointOnEdge (vtxIt1, vtxIt1.GetNext(), vtxIt0, vtxIt0.GetNext(), isect[i].second, loc)
					);
				}
			}
		}
	}
	return isectLists;
}

// ---

class CrossVtxDesc {
	Polygon::Contour::VertexIt m_vertex;
	Polygon::Contour::EdgeIt m_edge;

	float m_edgeAngle;								// angle between the edge and (1,0)
	Direction m_dir;

	PointConnectivityList * m_connListId;
public:
	CrossVtxDesc (const Polygon::Contour::EdgeIt & edgeIt, Direction dir, PointConnectivityList * listId) :
			m_vertex (0), m_edge (edgeIt), m_edgeAngle (0.0f), m_dir (dir), m_connListId (listId)
	{
		if (m_dir == OUT)
		{
			const Vec2 edgeVec (m_edge->GetNextVtx()->Pos() - m_edge->GetPrevVtx()->Pos());
			m_edgeAngle = edgeVec.atan2 ();
			m_vertex = m_edge->GetPrevVtx();
		}
		else
		{
			const Vec2 edgeVec (m_edge->GetPrevVtx()->Pos() - m_edge->GetNextVtx()->Pos());
			m_edgeAngle = edgeVec.atan2 ();
			m_vertex = m_edge->GetNextVtx();
		}
		if (m_edgeAngle < 0.0f) m_edgeAngle = 2*gf_PI + m_edgeAngle;
	}

	Polygon::Contour::VertexIt Vtx () const { return m_vertex; }
	Polygon::Contour::EdgeIt Edge () const { return m_edge; }

	Direction Dir () const { return m_dir; }

	typedef void * LocationId;
	LocationId GeomLocation () const { return m_connListId; }
	PointConnectivityList * GetList () const { return m_connListId; }
	unsigned char OwnerPolygon () const { return m_vertex->OwnerPolygon (); }

	bool FarEndIsCross () const
	{
		return (m_dir == OUT ? m_edge->GetNextVtx() : m_edge->GetPrevVtx())->IsCross();
	}
	PointConnectivityIt FarEndCrossVtxDesc () const
	{
		Polygon::Contour::VertexIt vtxIt = m_dir == OUT ? m_edge->GetNextVtx () : m_edge->GetPrevVtx ();
		assert (vtxIt->IsCross ());
		if ( ! vtxIt->IsCross ()) return 0;
		return vtxIt->GetCrossVtxDesc (Opposite (m_dir));
	}

	static void * operator new (size_t size)
	{
		return PolygonSetOp::GetCurAllocator()->Allocate (size);
	}

	struct Compare {
		bool operator() (const CrossVtxDesc * desc0, const CrossVtxDesc * desc1) const
		{
			return desc0->m_edgeAngle < desc1->m_edgeAngle;
		}
	};
};


class CrossVtxListMgr {

	struct CrossVtxListRec {
		Vec2 m_geomLocation;
		PointConnectivityList m_list;
		CrossVtxListRec (const Vec2 & loc, Allocator * allocator) :
				m_geomLocation (loc), m_list (allocator) { }
	};

	typedef FixedSizeArray <CrossVtxListRec> CrossVtxLists;
	CrossVtxLists m_lists;

	Allocator * m_allocator;

	PointConnectivityList * GetList (const Polygon::Contour::VertexIt & vertexIt)
	{
		for (unsigned i=0, numLists = m_lists.Size(); i < numLists; ++i)
		{
			// FIXME Sep 1, 2008: <pvl> come up with a better tolerance number!
			//if ( (m_lists[i].m_geomLocation - Vec2 (vertexIt->Pos())).GetLength2() < /*0.000001f*/1e-10)
			// UPDATE Mar 19, 2009: <pvl> in fact, let's only consider the same
			// those special cases where there's a perfect match
			if (m_lists[i].m_geomLocation == Vec2 (vertexIt->Pos()))
				return & m_lists[i].m_list;
		}

		// NOTE Sep 1, 2008: <pvl> create a new list at this vertex's position
		m_lists.PushBack (CrossVtxListRec (Vec2 (vertexIt->Pos()), m_allocator));
		return & m_lists[m_lists.Size()-1].m_list;
	}

	void AddVertexToList (PointConnectivityList * list, const Polygon::Contour::VertexIt & vertexIt)
	{
		CrossVtxDesc * in = new CrossVtxDesc (vertexIt->GetPrevEdge(), IN, list);
		CrossVtxDesc * out = new CrossVtxDesc (vertexIt->GetNextEdge(), OUT, list);
		PointConnectivityIt inIt (list->Insert (in, CrossVtxDesc::Compare()));
		PointConnectivityIt outIt (list->Insert (out, CrossVtxDesc::Compare()));
		vertexIt->AddCrossVtxDescs (inIt, outIt);
	}

	void AddCrossVertexDescriptors (const Polygon::Contour::VertexIt & vertexIt)
	{
		PointConnectivityList * descList = GetList (vertexIt);
		AddVertexToList (descList, vertexIt);
	}
public:
	CrossVtxListMgr (unsigned numLists, Allocator * allocator) :
			m_lists	(numLists, allocator), m_allocator (allocator)
	{
	}

	void AddCrossVertexDescriptors (const Polygon::Contour * contour)
	{
		Polygon::Contour::VertexLoopIt vertexIt = contour->CreateVertexLoopIt();
		for ( ; vertexIt; ++vertexIt)
		{
			if (vertexIt->IsCross ())
				AddCrossVertexDescriptors (vertexIt);
		}
	}

	void DebugDraw ()
	{









	}

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

	static void Destroy (CrossVtxListMgr * mgr)
	{
		if (mgr == 0) return;

		Allocator * allocator = mgr->m_allocator;
		mgr->~CrossVtxListMgr ();
		allocator->Deallocate (mgr);
	}
};

// ---

void LabelNonIsected (Polygon::Contour::EdgeLoopIt edgeIt)
{
	edgeIt->SetLabel (edgeIt.GetPrev ()->GetLabel ());
}

bool LabelIsectedIfShared (Polygon::Contour::EdgeLoopIt edgeIt)
{
	// NOTE Sep 3, 2008: <pvl> check for a shared edge - loop over edgeIt's
	// connectivity list, looking for cross vertex descriptors belonging to
	// the other polygon.  For each of them, check the far end of that edge
	// and see if it's where our far end is, geometrically.  If so then it's
	// a shared edge with the same direction in case its descriptor at edgeIt
	// location is of type 'out' and the opposite direction otherwise.

	// NOTE Sep 11, 2008: <pvl> we've already checked that both of our endpoints
	// are cross vertices so this is safe
	CrossVtxDesc::LocationId thisEdgeFarEndLoc = edgeIt->GetNextCrossVtxDesc()->GeomLocation();
	PointConnectivityLoopIt xVtxEdgeIt = edgeIt->GetPrevCrossVtxDesc();
	for ( ; xVtxEdgeIt; ++xVtxEdgeIt)
	{
		if (xVtxEdgeIt->OwnerPolygon () == edgeIt->OwnerPolygon()) continue;

		if (xVtxEdgeIt->FarEndIsCross ()
			&& xVtxEdgeIt->FarEndCrossVtxDesc()->GeomLocation() == thisEdgeFarEndLoc)
		{
			if (xVtxEdgeIt->Dir() == OUT)
				edgeIt->SetLabel (Polygon::Edge::SHARED_SAME_DIR);
			else
				edgeIt->SetLabel (Polygon::Edge::SHARED_OPPOSITE_DIR);
			return true;
		}
	}
	return false;
}

static bool check = false;

void LabelIsected (Polygon::Contour::EdgeLoopIt edgeIt)
{
	// NOTE Sep 3, 2008: <pvl> now we know we're not SHARED_* - set the label
	// to OUTSIDE by default and then see if we can prove otherwise

	edgeIt->SetLabel (Polygon::Edge::OUTSIDE);

	PointConnectivityLoopIt crossVtxDescIt = edgeIt->GetPrevVtx()->IsCross()
			? edgeIt->GetPrevVtx()->GetCrossVtxDesc (OUT)
			: edgeIt->GetNextVtx()->GetCrossVtxDesc (IN);

	// NOTE Sep 15, 2008: <pvl> a list of vertices whose corner we're outside of
	// NOTE Sep 15, 2008: <pvl> divide by 2 since there's twice less vertices
	// than descriptors (each vertex has 2 descriptors, 'in' and 'out')
	FixedSizeArray <Polygon::Contour::VertexIt> outVtcs (crossVtxDescIt->GetList()->Size() / 2, PolygonSetOp::GetCurAllocator ());

	for ( ; crossVtxDescIt; ++crossVtxDescIt)
	{
		if (crossVtxDescIt->OwnerPolygon() == edgeIt->OwnerPolygon()) continue;

if (check)
{
		Polygon::Contour::EdgeIt edge = crossVtxDescIt->Edge ();
		if ( (Vec2 (edge->GetPrevVtx()->Pos()) - Vec2 (edge->GetNextVtx()->Pos())).GetLength() < 0.0001f )
		{
			outVtcs.PushBack (crossVtxDescIt->Vtx());
			continue;
		}
}

		if (crossVtxDescIt->Dir() == OUT)
		{
			outVtcs.PushBack (crossVtxDescIt->Vtx());
		}
		else
		{
			if ( ! outVtcs.IsOnList (crossVtxDescIt->Vtx ()))
			{
				edgeIt->SetLabel (Polygon::Edge::INSIDE);
				break;
			}
		}
	}
}

void LabelEdges (Polygon::Contour * c)
{
	// NOTE Sep 2, 2008: <pvl> find a cross-vertex
	// TODO Sep 2, 2008: <pvl> could be moved into Contour class?
	Polygon::Contour::VertexLoopIt crossIt = c->CreateVertexLoopIt ();
	for ( ; crossIt; ++crossIt)
		if (crossIt->IsCross ())
			break;

	if ( ! crossIt)
	{
		// TODO Sep 2, 2008: <pvl> no cross vertices, the whole contour is either
		// inside or outside - find out and mark the edges accordingly
		return;
	}

	Polygon::Contour::EdgeLoopIt edgeIt (crossIt->GetNextEdge());
	for ( ; edgeIt; ++edgeIt)
	{
		if ( ! edgeIt->GetPrevVtx()->IsCross () && ! edgeIt->GetNextVtx()->IsCross ())
		{
			// NOTE Sep 3, 2008: <pvl> since we deliberately start with a cross-vertex,
			// this will never trigger first time around and thus the previous edge
			// will always have been labeled already.  This saves the trouble of doing
			// a point-in-polygon test if we started with a normal (non-cross) vertex.
			LabelNonIsected (edgeIt);
			continue;
		}
		else if (edgeIt->GetPrevVtx()->IsCross () && edgeIt->GetNextVtx()->IsCross ())
		{
			if (LabelIsectedIfShared (edgeIt))
				continue;
		}

		LabelIsected (edgeIt);
	}
}

// ---

class EdgeInclusionRule {
	bool m_include;
	TraversalDirection m_dir;

	void Conjunction (const Polygon::Contour::EdgeIt & );
	void Disjunction (const Polygon::Contour::EdgeIt & );
	void Difference (const Polygon::Contour::EdgeIt & );
	void SymmetricalDifference (const Polygon::Contour::EdgeIt & );
public:
	EdgeInclusionRule (const Polygon::Contour::EdgeIt & edgeIt, PolygonOperation );

	bool ShouldBeIncluded () const { return m_include; }
	TraversalDirection WhichDirection () const { assert (m_include); return m_dir; }
};

void EdgeInclusionRule::Conjunction (const Polygon::Contour::EdgeIt & edgeIt)
{
	Polygon::Edge::Label label = edgeIt->GetLabel ();

	if (label == Polygon::Edge::INSIDE || label == Polygon::Edge::SHARED_SAME_DIR)
	{
		m_include = true;
		m_dir = FORWARD;
	}
}

void EdgeInclusionRule::Disjunction (const Polygon::Contour::EdgeIt & edgeIt)
{
	Polygon::Edge::Label label = edgeIt->GetLabel ();

	if (label == Polygon::Edge::OUTSIDE || label == Polygon::Edge::SHARED_SAME_DIR)
	{	
		m_include = true;
		m_dir = FORWARD;
	}
}

void EdgeInclusionRule::Difference (const Polygon::Contour::EdgeIt & edgeIt)
{
	Polygon::Edge::Label label = edgeIt->GetLabel ();

	if (label == Polygon::Edge::OUTSIDE || label == Polygon::Edge::SHARED_OPPOSITE_DIR)
	{
		if (edgeIt->OwnerPolygon () == 0)
		{
			m_include = true;
			m_dir = FORWARD;
		}
	}
	else if (label == Polygon::Edge::INSIDE || label == Polygon::Edge::SHARED_OPPOSITE_DIR)
	{
		if (edgeIt->OwnerPolygon () == 1)
		{
			m_include = true;
			m_dir = BACKWARD;
		}
	}
}

void EdgeInclusionRule::SymmetricalDifference (const Polygon::Contour::EdgeIt & edgeIt)
{
	Polygon::Edge::Label label = edgeIt->GetLabel ();

	if (label == Polygon::Edge::OUTSIDE)
	{
		m_include = true;
		m_dir = FORWARD;
	}
	else if (label == Polygon::Edge::INSIDE)
	{
		m_include = true;
		m_dir = BACKWARD;
	}
}

EdgeInclusionRule::EdgeInclusionRule (const Polygon::Contour::EdgeIt & edgeIt, PolygonOperation op) :
		m_include (false)
{
	switch (op)
	{
	case CONJUNCTION:
		Conjunction (edgeIt);
		break;
	case DISJUNCTION:
		Disjunction (edgeIt);
		break;
	case DIFFERENCE:
		Difference (edgeIt);
		break;
	case SYMMETRICAL_DIFFERENCE:
		SymmetricalDifference (edgeIt);
		break;
	default:
		assert (0);
		break;
	}
}

// ---

class ContourCollector {
	/// The contour being collected.
	Polygon::Contour * m_contour;
public:
	ContourCollector (PolygonOperation op, Polygon::Contour * newContour,
			const Polygon::Contour::EdgeLoopIt & initialEdgeIt, TraversalDirection initDir);

	Polygon::Contour * GetContour ();
};


class ContourCollectionIt {

	Polygon::Contour::EdgeIt m_edgeIt;

	PolygonOperation m_op;
	/// The direction we're traversing edges at the moment (fwd or back).
	TraversalDirection m_dir;

	Polygon::Contour::EdgeIt SelectNextEdgeAtCrossVtx (Polygon::Contour::EdgeIt edgeIt);

public:
	ContourCollectionIt (PolygonOperation op, const Polygon::Contour::EdgeLoopIt & initialEdgeIt, TraversalDirection initDir) :
			m_edgeIt (initialEdgeIt), m_op (op), m_dir (initDir)
	{ }

	operator bool () const { return m_edgeIt; }
	ContourCollectionIt & operator++ ();
	const Polygon::Contour::EdgeIt & operator-> () const { return m_edgeIt; }

	Polygon::Contour::VertexIt PrevVertex () const { return m_dir == FORWARD ? m_edgeIt->GetPrevVtx() : m_edgeIt->GetNextVtx(); }
	Polygon::Contour::VertexIt NextVertex () const { return m_dir == FORWARD ? m_edgeIt->GetNextVtx() : m_edgeIt->GetPrevVtx(); }
};

ContourCollectionIt & ContourCollectionIt::operator++ ()
{
	const Polygon::Contour::VertexIt & nextVtx = NextVertex();

	if ( ! nextVtx->IsCross ())
		m_dir == FORWARD ?	++m_edgeIt : --m_edgeIt;
	else
		m_edgeIt = SelectNextEdgeAtCrossVtx (m_edgeIt);
	return *this;
}

Polygon::Contour::EdgeIt ContourCollectionIt::SelectNextEdgeAtCrossVtx (Polygon::Contour::EdgeIt edgeIt)
{
	PointConnectivityLoopIt xVtxDescIt (
			NextVertex()->GetCrossVtxDesc (m_dir == FORWARD ? IN : OUT), BACKWARD
	);
	++xVtxDescIt;

	for ( ; xVtxDescIt; ++xVtxDescIt)
	{
		if (xVtxDescIt->Edge()->IsIncludedInResult ()) continue;

		EdgeInclusionRule rule (xVtxDescIt->Edge(), m_op);
		if ( ! rule.ShouldBeIncluded()) continue;

		if (rule.WhichDirection() == FORWARD && xVtxDescIt->Dir() == OUT
				|| rule.WhichDirection() == BACKWARD && xVtxDescIt->Dir() == IN)
		{
			m_dir = rule.WhichDirection ();
			return Polygon::Contour::EdgeIt (xVtxDescIt->Edge());
		}
	}
	return Polygon::Contour::EdgeIt (0);
}

ContourCollector::ContourCollector (PolygonOperation op,
			Polygon::Contour * newContour, 
			const Polygon::Contour::EdgeLoopIt & initialEdgeIt,
			TraversalDirection initDir) :
		m_contour (newContour)
{
	ContourCollectionIt edgeIt (op, initialEdgeIt, initDir);

	for ( ; edgeIt && ! edgeIt->IsIncludedInResult (); ++edgeIt)
	{
		// NOTE Sep 10, 2008: <pvl> include it in the result
		// FIXME Sep 12, 2008: <pvl> can we make this more transparent??
		m_contour->AddVertex ( ** edgeIt.PrevVertex ());

		// NOTE Sep 10, 2008: <pvl> mark it as such
		edgeIt->MarkAsIncludedInResult ();

		Polygon::Edge::Label label = edgeIt->GetLabel ();
		if (label == Polygon::Edge::SHARED_SAME_DIR || label == Polygon::Edge::SHARED_OPPOSITE_DIR)
		{
			// NOTE Apr 9, 2009: <pvl> mark the corresponding shared edges as
			// included in result as well
			CrossVtxDesc::LocationId thisEdgeFarEndLoc = edgeIt->GetNextCrossVtxDesc()->GeomLocation();
			PointConnectivityLoopIt xVtxEdgeIt = edgeIt->GetPrevCrossVtxDesc();
			for ( ; xVtxEdgeIt; ++xVtxEdgeIt)
			{
				if (xVtxEdgeIt->OwnerPolygon () == edgeIt->OwnerPolygon()) continue;

				if (xVtxEdgeIt->FarEndIsCross ()
					&& xVtxEdgeIt->FarEndCrossVtxDesc()->GeomLocation() == thisEdgeFarEndLoc)
				{
					if (label == Polygon::Edge::SHARED_SAME_DIR && xVtxEdgeIt->Dir() == edgeIt->GetPrevCrossVtxDesc ()->Dir ())
						xVtxEdgeIt->Edge()->MarkAsIncludedInResult ();
					else if (label == Polygon::Edge::SHARED_OPPOSITE_DIR && xVtxEdgeIt->Dir() != edgeIt->GetPrevCrossVtxDesc ()->Dir ())
						xVtxEdgeIt->Edge()->MarkAsIncludedInResult ();
				}
			}
		}
	}
}

Polygon::Contour * ContourCollector::GetContour ()
{
	return m_contour;
}

// ---

class ResultingContourIt {
	PolygonOperation m_op;
	Polygon::Contour::EdgeLoopIt m_edgeIt;

	Polygon * m_polygon;
public:
	ResultingContourIt (const Polygon::Contour * contour, Polygon * newPoly, PolygonOperation op) :
			m_op (op), m_edgeIt (contour->CreateEdgeLoopIt()), m_polygon (newPoly)
	{
		++(*this);
	}

	ResultingContourIt & operator++ ()
	{
		for ( ; m_edgeIt; ++m_edgeIt)
		{
			if (m_edgeIt->IsIncludedInResult ()) continue;

			EdgeInclusionRule rule (m_edgeIt, m_op);
			if ( ! rule.ShouldBeIncluded()) continue;

			Polygon::Contour::EdgeLoopIt startingEdge (m_edgeIt);
			if (rule.WhichDirection() == BACKWARD)
			{
				startingEdge = startingEdge.GetReset (true);
			}

			Polygon::Contour * newContour = m_polygon->NewContour ();
			m_polygon->AddContour (newContour);
			ContourCollector result (m_op, newContour, startingEdge, rule.WhichDirection ());
			++m_edgeIt;
			break;
		}
		return *this;
	}

	operator bool () const { return m_edgeIt; }
};

// TODO Sep 18, 2008: <pvl> can get rid of the 'poly' argument (e.g. by teaching
// contours which poly they belong to)?
void CollectNonIsectedContour (const Polygon::Contour * c, unsigned poly, Polygon * newPoly, PolygonOperation op)
{
	Polygon::Contour::Label label = c->GetLabel ();
	assert (label != Polygon::Contour::NOT_LABELED);

	if (op == CONJUNCTION && label == Polygon::Contour::INSIDE)
		newPoly->AddClone (c, FORWARD);
	else if (op == DISJUNCTION && label == Polygon::Contour::OUTSIDE)
		newPoly->AddClone (c, FORWARD);
	else if (op == DIFFERENCE)
	{
		if (poly == 0 && label == Polygon::Contour::OUTSIDE)
			newPoly->AddClone (c, FORWARD);
		else if (poly == 1 && label == Polygon::Contour::INSIDE)
			newPoly->AddClone (c, BACKWARD);
	}
	else if (op == SYMMETRICAL_DIFFERENCE)
	{
		if (label == Polygon::Contour::OUTSIDE)
			newPoly->AddClone (c, FORWARD);
		else if (label == Polygon::Contour::INSIDE)
			newPoly->AddClone (c, BACKWARD);
	}
}

// ---

bool Inside (const Polygon::Contour * c0, const Polygon::Contour * c1)
{
	Vec3 pos (c0->CreateVertexLoopIt()->Pos());

	std::vector< ::Vec3> poly;
	Polygon::Contour::VertexLoopIt vtxIt = c1->CreateVertexLoopIt ();
	for ( ; vtxIt; ++vtxIt)
	{
		poly.push_back (vtxIt->Pos ());
	}

	return Overlap::Point_Polygon2D (pos, poly);
}

void LabelNonIsectedContour (Polygon::Contour * c, const Polygon * p)
{
	bool inside = false;

	for (unsigned int pci=0, numContours = p->NumContours(); pci < numContours; ++pci)
	{
		Polygon::Contour * pc = p->GetContour (pci);
		if (Inside (c, pc))
			inside = ! inside;
	}

	assert (c->GetLabel () == Polygon::Contour::NOT_LABELED);
	if (inside)
		c->SetLabel (Polygon::Contour::INSIDE);
	else
		c->SetLabel (Polygon::Contour::OUTSIDE);
}

// ---

Allocator * PolygonSetOp::s_allocator = 0;

static const int s_memPoolSize = 500 * 1024;

PolygonSetOp::PolygonSetOp (Polygon * s0, Polygon * s1, Allocator * alloc) :
		m_allocator (alloc), m_poly0 (s0), m_poly1 (s1),
		m_result(0), m_xVtxListMgr (0)
{
	s_allocator = m_allocator;

	Preprocess ();
}



PolygonSetOp::~PolygonSetOp ()
{
	// NOTE May 29, 2009: <pvl> don't Destroy() the polygons anymore - now client
	// has the responsibility to manage their lifetime
//	Polygon::Destroy (m_poly0);
//	Polygon::Destroy (m_poly1);
	CrossVtxListMgr::Destroy (m_xVtxListMgr);
	s_allocator = 0;
//	delete m_allocator;
}

void PolygonSetOp::AddCrossVertexDescriptors (Polygon * p)
{
	for (unsigned int ci=0, numContours = p->NumContours(); ci < numContours; ++ci)
	{
		Polygon::Contour * c = p->GetContour (ci);

		c->BuildEdges ();

		// NOTE Sep 1, 2008: <pvl> build cross vertex descriptor lists
		m_xVtxListMgr->AddCrossVertexDescriptors (c);
	}
}

void LabelEdges (Polygon * p)
{
	for (unsigned int ci=0, numContours = p->NumContours(); ci < numContours; ++ci)
	{
		Polygon::Contour * c = p->GetContour (ci);

		assert (c->GetLabel () != Polygon::Contour::NOT_LABELED);

		if (c->GetLabel () == Polygon::Contour::INTERSECTED)
			LabelEdges (c);
		//DebugDrawSourceContour (c);
	}
}


void DumpContour (const Polygon::Contour * c, FILE * fp)
{
	fprintf (fp, "contour:\n");

	Polygon::Contour::VertexLoopIt vtxIt = c->CreateVertexLoopIt();
	for ( ; vtxIt; ++vtxIt)
	{
		Vec3 vtxPos = vtxIt->Pos ();
		fprintf (fp, "  %8.4f %8.4f %8.4f%s\n", vtxPos.x, vtxPos.y, vtxPos.z, vtxIt->IsCross () ? " cross" : "");
	}
}

void DumpPolygon (const Polygon * p, FILE * fp)
{
	for (unsigned int ci=0, numContours = p->NumContours(); ci < numContours; ++ci)
	{
		Polygon::Contour * c = p->GetContour (ci);
		DumpContour (c, fp);
	}
	fprintf (fp, "\n\n");
}

void DumpContourForGnuplot (const Polygon::Contour * c, FILE * fp)
{
	Polygon::Contour::VertexLoopIt vtxIt = c->CreateVertexLoopIt();
	for ( ; vtxIt; ++vtxIt)
	{
		Vec3 vtxPos = vtxIt->Pos ();
		fprintf (fp, "%8.4f %8.4f\n", vtxPos.x, vtxPos.y);
	}
	// NOTE Mar 5, 2009: <pvl> dump the first one once more to make the shape closed
	vtxIt = c->CreateVertexLoopIt();
	Vec3 vtxPos = vtxIt->Pos ();
	fprintf (fp, "%8.4f %8.4f\n", vtxPos.x, vtxPos.y);
}

void DumpPolygonForGnuplot (const Polygon * p, const string & polyName)
{
	const int contourIdxWidth = (int )log10 ((float )p->NumContours()) + 1;
	for (unsigned int ci=0, numContours = p->NumContours(); ci < numContours; ++ci)
	{
		char buf[16];
		_snprintf (buf, 16, "%.*d", contourIdxWidth, ci);
		FILE * fp = fopen (polyName + string ("-contour_") + string (buf), "w");

		Polygon::Contour * c = p->GetContour (ci);
		DumpContourForGnuplot (c, fp);

		fclose (fp);
	}
}

//---

class Contains {
public:
	bool operator() (const Polygon::Contour * lhs, const Polygon::Contour * rhs) const;
};

bool Contains::operator() (const Polygon::Contour * lhs, const Polygon::Contour * rhs) const
{
	return Inside (rhs, lhs);
}

// ---

template <typename T>
class PosetNode {
	T mValue;

	PosetNode * mParent;
	std::vector<PosetNode*> mChildren;
public:
	PosetNode (T );

	T GetValue () const;

	void AddChild (PosetNode * );
	void DelChild (PosetNode * );
	void SetParent (PosetNode * );
	PosetNode * GetParent () const;
	bool IsAncestor (PosetNode * ) const;

	bool IsLeafNode () const;
	bool IsRootNode () const;

	void Print (const string & indent = "") const;

	// ---
	typedef bool (PosetNode<T>::*PredFnT) () const;

	template <PredFnT PredFn>
	class Pred {
	public:
		bool operator() (PosetNode * node) const { return (node->*PredFn) (); }
	};

	typedef Pred < & PosetNode::IsLeafNode > LeafNodePred;
	typedef Pred < & PosetNode::IsRootNode > RootNodePred;

};

template <typename T>
inline PosetNode<T>::PosetNode (T val) : mValue (val), mParent (0)
{
}

template <typename T>
inline T PosetNode<T>::GetValue () const
{
	return mValue;
}

template <typename T>
inline void PosetNode<T>::AddChild (PosetNode * new_child)
{
	new_child->SetParent (this);
	mChildren.push_back (new_child);
}

template <typename T>
inline void PosetNode<T>::DelChild (PosetNode * child)
{
	child->SetParent (0);

	typename std::vector<PosetNode*>::iterator it = mChildren.begin ();
	const typename std::vector<PosetNode*>::iterator end = mChildren.end ();
	for ( ; it != end; ++it)
		if (*it == child) {
			mChildren.erase (it);
			return;
		}
	assert (0);
}

template <typename T>
inline void PosetNode<T>::SetParent (PosetNode * new_parent)
{
	mParent = new_parent;
}

template <typename T>
inline PosetNode<T> * PosetNode<T>::GetParent () const
{
	return mParent;
}

template <typename T>
inline bool PosetNode<T>::IsAncestor (PosetNode * node) const
{
	PosetNode * ancestor = mParent;
	for ( ; ancestor; ancestor = ancestor->mParent)
		if (node == ancestor)
			return true;
	return false;
}

template <typename T>
inline bool PosetNode<T>::IsLeafNode () const
{
	return mChildren.empty ();
}

template <typename T>
inline bool PosetNode<T>::IsRootNode () const
{
	return mParent == 0;
}

template <typename T>
inline void PosetNode<T>::Print (const string & indent) const
{










}

// ---

template <typename T>
class NodeRegistry {
	typedef std::map <T, PosetNode<T>*> Registry;
	Registry mRegistry;
public:
	~NodeRegistry ();

	PosetNode<T> * GetNode (T );

	template <typename Pred>
	class NodeIt {
		typename Registry::iterator mCur;
		typename Registry::iterator mEnd;
		Pred mPred;
		void MoveToNextSuitable ();
	public:
		NodeIt (Registry & , const Pred & );
		PosetNode<T> * operator* () const;
		operator bool () const;
		NodeIt & operator++ ();
	};

	typedef NodeIt <typename PosetNode<T>::LeafNodePred> LeafNodeIt;
	typedef NodeIt <typename PosetNode<T>::RootNodePred> RootNodeIt;

	LeafNodeIt CreateLeafIt ();
	RootNodeIt CreateRootIt ();

	void Print () const;
};

template <typename T>
template <typename Pred>
inline NodeRegistry<T>::NodeIt<Pred>::NodeIt (Registry & reg, const Pred & pred) :
		mCur (reg.begin()), mEnd (reg.end()), mPred (pred)
{
	MoveToNextSuitable ();
}

template <typename T>
template <typename Pred>
inline void NodeRegistry<T>::NodeIt<Pred>::MoveToNextSuitable ()
{
	while ( *this && ! mPred (mCur->second) )
		++mCur;
}

template <typename T>
template <typename Pred>
inline PosetNode<T> * NodeRegistry<T>::NodeIt<Pred>::operator* () const
{
	return mCur->second;
}

template <typename T>
template <typename Pred>
inline NodeRegistry<T>::NodeIt<Pred>::operator bool () const
{
	return mCur != mEnd;
}

// NOTE Jun 26, 2009: <pvl> vc++ 9 can't compile the syntax required by g++, hence
// the #if.  BTW vc++ 8 could compile it just fine.
template <typename T>
template <typename Pred>
#if defined (_MSC_VER) && _MSC_VER >= 1500
inline typename NodeRegistry<T>::NodeIt<Pred> &
#else
inline typename NodeRegistry<T>::template NodeIt<Pred> &
#endif
NodeRegistry<T>::NodeIt<Pred>::operator++ ()
{
	++mCur;
	MoveToNextSuitable ();
	return *this;
}

template <typename T>
inline typename NodeRegistry<T>::LeafNodeIt NodeRegistry<T>::CreateLeafIt ()
{
	return LeafNodeIt (mRegistry, typename PosetNode<T>::LeafNodePred());
}

template <typename T>
inline typename NodeRegistry<T>::RootNodeIt NodeRegistry<T>::CreateRootIt ()
{
	return RootNodeIt (mRegistry, typename PosetNode<T>::RootNodePred());
}

template <typename T>
inline NodeRegistry<T>::~NodeRegistry ()
{
	typename Registry::iterator it = mRegistry.begin ();
	const typename Registry::iterator end = mRegistry.end ();
	for ( ; it != end; ++it)
		delete it->second;
}

template <typename T>
inline PosetNode<T> * NodeRegistry<T>::GetNode (T element)
{
	PosetNode<T> * node = 0;
	typename Registry::iterator it = mRegistry.find (element);
	if (it != mRegistry.end ())
		node = it->second;
	else {
		node = new PosetNode<T> (element);
		mRegistry.insert (std::make_pair (element, node));
	}
	return node;
}

template <typename T>
inline void NodeRegistry<T>::Print () const
{
	typename Registry::const_iterator it = mRegistry.begin ();
	const typename Registry::const_iterator end = mRegistry.end ();
	for ( ; it != end; ++it)
		if (it->second->GetParent () == 0)
			it->second->Print ();
}

// ---

bool SanityCheckWinding (const Polygon * p)
{
	// NOTE May 27, 2009: <pvl> contours mustn't intersect each other so it's
	// possible to view a "contains" relationship defined on the set of contours
	// as a partial ordering.  The ordering for a well-formed polygon is a forest
	// (a set of trees) where the following must be true for each individual tree:
	// - the root has to be CCW
	// - on the path from each leaf node to the root the winding directions must
	//   alternate

	// NOTE May 27, 2009: <pvl> construct the partial order
    NodeRegistry<Polygon::Contour*> node_registry;

	Contains contains;

	for (unsigned int ci=0, numContours = p->NumContours(); ci < numContours; ++ci)
	{
		Polygon::Contour * c = p->GetContour (ci);
		for (unsigned int cj=0; cj < numContours; ++cj)
		{
			if (cj == ci) continue;

			Polygon::Contour * otherCntr = p->GetContour (cj);

            PosetNode<Polygon::Contour*> * cNode = node_registry.GetNode (c);
            PosetNode<Polygon::Contour*> * otherNode = node_registry.GetNode (otherCntr);

			if (contains (c, otherCntr))
			{
				if ( ! otherNode->IsAncestor (cNode)) {
					PosetNode<Polygon::Contour*> * otherNodeParent = otherNode->GetParent ();
					if (otherNodeParent && ! cNode->IsAncestor (otherNodeParent)) {
					} else {
						if (otherNodeParent)
							otherNodeParent->DelChild (otherNode);
						cNode->AddChild (otherNode);
					}
				}
			}
		}
	}

	// NOTE May 27, 2009: <pvl> check the winding
	NodeRegistry<Polygon::Contour*>::LeafNodeIt leaf_it = node_registry.CreateLeafIt ();
	for ( ; leaf_it; ++leaf_it) {
		PosetNode<Polygon::Contour*> * nodeAlongPathToRoot = *leaf_it;

		bool ccw = nodeAlongPathToRoot->GetValue()->ComputeWinding () == Polygon::Contour::CCW;
		nodeAlongPathToRoot = nodeAlongPathToRoot->GetParent ();

		for ( ; nodeAlongPathToRoot; nodeAlongPathToRoot = nodeAlongPathToRoot->GetParent ())
		{
			bool nextCcw = nodeAlongPathToRoot->GetValue()->ComputeWinding () == Polygon::Contour::CCW;
			if (nextCcw != ! ccw)
				return false;

			ccw = nextCcw;
		}
		// NOTE Apr 8, 2009: <pvl> the outermost contour must be CCW
		if ( ! ccw)
			return false;
	}
	return true;
}

bool SanityCheckResult (const Polygon * p)
{
	for (unsigned int ci=0, numContours = p->NumContours(); ci < numContours; ++ci)
	{
		Polygon::Contour * c = p->GetContour (ci);
		//if (c->GetLabel () != Polygon::Contour::INTERSECTED) continue;

		if (c->NumVertices () < 3)
			return false;
	}
	return true;
}

bool SanityCheckInput (const Polygon * p)
{
	for (unsigned int ci=0, numContours = p->NumContours(); ci < numContours; ++ci)
	{
		Polygon::Contour * c = p->GetContour (ci);

		if (c->NumVertices () < 3)
			return false;

		Polygon::Contour::VertexLoopIt vtxIt = c->CreateVertexLoopIt();
		for ( ; vtxIt; ++vtxIt)
		{
			if (vtxIt->IsCross ())
				return false;
		}
	}
	return true;
}

void SanitizeResult (Polygon * p)
{
	for (unsigned int ci=0; ci < p->NumContours(); )
	{
		Polygon::Contour * c = p->GetContour (ci);

		if (c->NumVertices () < 3)
			p->DelContour (ci);
		else
			++ci;
	}
}

void SetupCrossVtx (Polygon::Contour::VertexIt vtxIt, const PointOnEdge * isect, Allocator * alloc)
{
	vtxIt->SetCross ();

	if (isect->m_vertex->GetUserData () && isect->m_otherVertex->GetUserData ())
	{
		// NOTE Mar 26, 2009: <pvl> this is a crude replacement for asking
		// "are both polygons labelled?"
		CrossVtxUserData * userData = new (alloc) CrossVtxUserData (
				CrossVtxUserData::Edge (isect->m_vertex->GetUserData(), isect->m_next->GetUserData()),
				CrossVtxUserData::Edge (isect->m_otherVertex->GetUserData(), isect->m_otherNext->GetUserData())
		);
		vtxIt->ResetUserData (userData);
	}
}

void Augment (Polygon::Contour * contour, std::vector <PointOnEdge*> & isects)
{
	std::sort (isects.begin (), isects.end (), PointOnEdge::Compare());
	isects.erase ( std::unique (isects.begin(), isects.end (), PointOnEdge::Equal()), isects.end () );

	contour->SetLabel (Polygon::Contour::INTERSECTED);

	std::vector <PointOnEdge*>::const_iterator isectIt = isects.begin ();
	const std::vector <PointOnEdge*>::const_iterator isectEnd = isects.end ();
	for ( ; isectIt != isectEnd; ++isectIt)
	{
		if ( (*isectIt)->m_t == 0.0f)
		{
			if ( ! (*isectIt)->m_vertex->IsCross ())
			{
				SetupCrossVtx ((*isectIt)->m_vertex, *isectIt, contour->GetAllocator());
			}
		} else if ( (*isectIt)->m_t < 1.0f)
		{
			// NOTE May 29, 2009: <pvl> even if 'm_t' isn't a clean 0.0 or 1.0,
			// due to finite precision of FP the computed 'pos' might end up
			// precisely where an endpoint is (i.e. as if 'm_t' were 0.0 or 1.0).
			// In these cases we need to avoid creating a new cross vertex.
			const Vec3 & pos = (*isectIt)->Pt ();
			if ( Vec2 (pos) == Vec2 ((*isectIt)->m_vertex->Pos()) )
			{
				if ( ! (*isectIt)->m_vertex->IsCross ())
					SetupCrossVtx ((*isectIt)->m_vertex, *isectIt, contour->GetAllocator());
				continue;
			}
			// NOTE Aug 3, 2009: <pvl> in this case we actually want to check
			// 'm_vertex.GetNext()', not 'm_next'.  This is to guard against
			// 'm_t's that are so close to each other that they map to the same
			// point along their lineseg.  When this happens, the cross vertex
			// corresponding to the (slightly) bigger 'm_t' will have been inserted
			// by now and 'm_vertex.GetNext()' refers to it.  If the 'm_t'
			// in question is very close to 1.0 it might happen that the other vertex
			// this cross would overlap with is the far end of the lineseg - which
			// is handled by this code as well.
			if ( Vec2 (pos) == Vec2 ((*isectIt)->m_vertex.GetNext()->Pos()) )
			{
				if ( ! (*isectIt)->m_vertex.GetNext()->IsCross ())
					SetupCrossVtx ((*isectIt)->m_vertex.GetNext(), *isectIt, contour->GetAllocator ());
				continue;
			}

			Polygon::Contour::VertexIt newVtxIt =
					contour->InsertVertex ( (*isectIt)->m_vertex, Polygon::Vertex ( (*isectIt)->m_vertex->OwnerPolygon (), (*isectIt)->Pt ()/*, true*/));

			SetupCrossVtx (newVtxIt, *isectIt, contour->GetAllocator ());
		}
		else if ( (*isectIt)->m_t == 1.0f )
		{
			if ( ! (*isectIt)->m_next->IsCross ())
			{
				SetupCrossVtx ((*isectIt)->m_next, *isectIt, contour->GetAllocator ());
			}
		}
	}
}

void LabelContours (Polygon * p0, Polygon * p1)
{
	for (unsigned int ci0=0, numContours0 = p0->NumContours(); ci0 < numContours0; ++ci0)
	{
		Polygon::Contour * c0 = p0->GetContour (ci0);
		if (c0->GetLabel () != Polygon::Contour::NOT_LABELED)
			continue;

		LabelNonIsectedContour (c0, p1);
	}
}

void PolygonSetOp::Preprocess ()
{
	assert (SanityCheckInput (m_poly0));
	assert (SanityCheckInput (m_poly1));

	Stopwatch watch;
	watch.Start ();

	// FIXME Mar 27, 2009: <pvl> these maps and vectors live in unpooled memory
	std::map <Polygon::Contour* ,std::vector <PointOnEdge*> > allIsects;
	unsigned numIsects = 0;

	Stopwatch edgeIsectWatch;
	edgeIsectWatch.Start ();
	for (unsigned int ci0=0, numContours0 = m_poly0->NumContours(); ci0 < numContours0; ++ci0)
		for (unsigned int ci1=0, numContours1 = m_poly1->NumContours(); ci1 < numContours1; ++ci1)
		{
			Polygon::Contour * c0 = m_poly0->GetContour (ci0);
			Polygon::Contour * c1 = m_poly1->GetContour (ci1);
			EdgeIntersectionLists isects = FindEdgeIntersections (c0, c1);

			assert (isects.m_poly0.Size() == isects.m_poly1.Size());
			numIsects += isects.m_poly0.Size();
			if (isects.m_poly0.Size () == 0)
				continue;

			allIsects[c0].insert ( allIsects[c0].end (), isects.m_poly0.Begin (), isects.m_poly0.End ());
			allIsects[c1].insert ( allIsects[c1].end (), isects.m_poly1.Begin (), isects.m_poly1.End ());
		}
	edgeIsectWatch.Stop ();

	Stopwatch augmentWatch;
	augmentWatch.Start ();
	std::map <Polygon::Contour* ,std::vector <PointOnEdge*> >::iterator it = allIsects.begin ();
	const std::map <Polygon::Contour* ,std::vector <PointOnEdge*> >::iterator end = allIsects.end ();
	for ( ; it != end; ++it)
		Augment (it->first, it->second);
	augmentWatch.Stop ();

	Stopwatch labelContoursWatch;
	labelContoursWatch.Start ();
	LabelContours (m_poly0, m_poly1);
	LabelContours (m_poly1, m_poly0);
	labelContoursWatch.Stop ();

	m_xVtxListMgr = new (m_allocator) CrossVtxListMgr (numIsects, m_allocator);

	AddCrossVertexDescriptors (m_poly0);
	AddCrossVertexDescriptors (m_poly1);

	Stopwatch labelEdgesWatch;
	labelEdgesWatch.Start ();
	LabelEdges (m_poly0);
	LabelEdges (m_poly1);
	labelEdgesWatch.Stop ();

	watch.Stop ();
	//printf("  Preprocessing of the polygon: %.3fms (edge isect %.3fms, augment %.3fms, label cntrs %.3fms, label edges %.3fms)\n", 1000.0 * watch.GetElapsed (),
	//		1000.0 * edgeIsectWatch.GetElapsed (), 1000.0 * augmentWatch.GetElapsed (),
	//		1000.0 * labelContoursWatch.GetElapsed (), 1000.0 * labelEdgesWatch.GetElapsed ());
}

void PolygonSetOp::ExtractResult (PolygonOperation operation, Polygon * operand, int operandNum)
{
	Stopwatch watch;
	watch.Start ();

	for (unsigned int ci=0, numContours = operand->NumContours(); ci < numContours; ++ci)
	{
		Polygon::Contour * c = operand->GetContour (ci);
		if (c->GetLabel() == Polygon::Contour::INTERSECTED)
		{
			// FIXME Nov 4, 2008: <pvl> a CollectIsectedContour() call would look better here
			ResultingContourIt contourIt (c, m_result, operation);
			for ( ; contourIt; ++contourIt)
				;
		}
		else
		{
			CollectNonIsectedContour (c, operandNum, m_result, operation);
		}
	}
}

void PolygonSetOp::ExtractResult (PolygonOperation operation)
{
	Stopwatch watch;
	watch.Start ();

	m_result = new (GetCurAllocator ()) Polygon (GetCurAllocator ());

	ExtractResult (operation, m_poly0, 0);
	ExtractResult (operation, m_poly1, 1);

#ifdef _DEBUG
	watch.Pause ();

	SanitizeResult (m_result);
	assert (SanityCheckWinding (m_result));

	if ( ! SanityCheckResult (m_result))
	{

		FILE * fp = fopen ("polysetops-errdump.txt", "w");
		DumpPolygon (m_poly0, fp);
		DumpPolygon (m_poly1, fp);
		DumpPolygon (m_result, fp);

		fclose (fp);
		assert (0);
	}
	watch.Resume ();
#endif // _DEBUG

	watch.Stop ();
	//printf("  Extracting polygon op result: %.3fms\n", 1000.0 * watch.GetElapsed ());
}

Polygon * PolygonSetOp::Conjunction ()
{
	ExtractResult (CONJUNCTION);
	return m_result;
}

Polygon * PolygonSetOp::Disjunction ()
{
	ExtractResult (DISJUNCTION);
	return m_result;
}

Polygon * PolygonSetOp::Difference ()
{
	ExtractResult (DIFFERENCE);
	return m_result;
}

Polygon * PolygonSetOp::SymmetricalDifference ()
{
	ExtractResult (SYMMETRICAL_DIFFERENCE);
	return m_result;
}

Allocator * PolygonSetOp::GetCurAllocator ()
{
	return s_allocator;
}

} // namespace PSO
