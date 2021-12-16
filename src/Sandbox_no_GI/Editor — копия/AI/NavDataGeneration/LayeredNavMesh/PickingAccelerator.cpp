
#include "StdAfx.h"

#include "PickingAccelerator.h"
#include "env.h"

#include "IRenderAuxGeom.h"


inline float & UpDir (::Vec3 & v)
{
	return v.z; 
}

inline float & UpDir (LayeredNavMesh::Vec3 & v)
{
	return v.y;
}

inline Vec2 StripUpDir (const ::Vec3 & v)
{
	return Vec2 (v.x, v.y);
}

inline Vec2 StripUpDir (const LayeredNavMesh::Vec3 & v)
{
	return Vec2 (v.z, v.x);
}

inline ::Vec3 ConvertToEngineVec3 (const ::Vec3 & v)
{
	return v;
}

inline ::Vec3 ConvertToEngineVec3 (const LayeredNavMesh::Vec3 & v)
{
	return ConvertVec3 (v);
}


template <typename PolygonType>
//AABB GetAABB (const NavPolygonGenerator::NavPolyPtr & poly)
AABB2 GetAABB (const PolygonType & poly)
{
	AABB2 aabb (AABB::RESET);
	typename PolygonType::vertex_iterator it = poly.begin ();
	typename PolygonType::vertex_iterator end = poly.end ();
	for ( ; it != end; ++it)
	{
		aabb.Add (Vec2 (ConvertToEngineVec3 (*it)));
	}
	return aabb;
}

inline float Area (const AABB2 & aabb)
{
	return (aabb.max.x - aabb.min.x) * (aabb.max.y - aabb.min.y);
}

// TODO Jul 10, 2008: <pvl> NavPolyPtr might not be the best polygon representation
// for this function to run fast - vertices can be scattered in memory
//
// TODO Jul 10, 2008: <pvl> testing direction is x+ - make sure it doesn't interact
// badly with the fact that GLU tesselator produces primarily triangles in one
// direction!
//
// TODO Jul 22, 2008: <pvl> try implementing the bisection test described in
// in the Ericson collision book!!!
template <typename PolygonType>
bool PointInConvexPoly (const PolygonType & poly, const ::Vec3 & posCE)
{
#if 1
	Vec2 pos (StripUpDir (posCE));

	typename PolygonType::vertex_iterator vertIt = poly.begin ();
	typename PolygonType::vertex_iterator vertEnd = poly.end ();
	typename PolygonType::vertex_iterator nextVertIt = vertIt;

	int numRightCrossings=0, numLeftCrossings=0;

	for (++nextVertIt; vertIt != vertEnd; ++vertIt, ++nextVertIt)
	{
		if (nextVertIt == vertEnd)
			nextVertIt = poly.begin ();

		Vec2 v0 (StripUpDir (*vertIt));
		Vec2 v1 (StripUpDir (*nextVertIt));

		// NOTE Jul 11, 2008: <pvl> see if this edge crosses the x+ line anchored at 'pos'
		bool crossesInPosDir = v0.y <= pos.y && v1.y > pos.y;		// as in "up" in horizontal plane
		bool crossesInNegDir = v0.y > pos.y && v1.y <= pos.y;		// as in "down" in horizontal plane

		if ( ! crossesInPosDir && ! crossesInNegDir)
			continue;

		float edgeX = v1.x - v0.x;
		float edgeY = v1.y - v0.y;
		float toPosX = pos.x - v0.x;
		float toPosY = pos.y - v0.y;
		float edgeCrossToPos = edgeX * toPosY - toPosX * edgeY;

		if ( (edgeCrossToPos > 0.0f && crossesInPosDir) || (edgeCrossToPos <= 0.0f && crossesInNegDir))
			++numRightCrossings;
		else
			++numLeftCrossings;

		// NOTE Jul 11, 2008: <pvl> the poly is convex, it can't have more than 2
		if (numRightCrossings + numLeftCrossings == 2)
		{
			if (numRightCrossings == 1)
				return true;
			else
				return false;
		}
	}
	return false;

#else

	std::vector< ::Vec3> polyCE;
	NavPolygonGenerator::NavPolyPtr::vertex_iterator vertIt = poly.begin ();
	NavPolygonGenerator::NavPolyPtr::vertex_iterator vertEnd = poly.end ();
	for ( ; vertIt != vertEnd; ++vertIt)
	{
		polyCE.push_back (ConvertVec3 (*vertIt));
	}

	return Overlap::Point_Polygon2D (posCE, polyCE);
#endif
}

// ---


NaivePickingAcceleratorBase::NaivePickingAcceleratorBase ()
{
}

void NaivePickingAcceleratorBase::Add (unsigned int poly)
{
	PolygonRec polyRec (poly, GetPolyAABB (poly));
	m_polys.push_back (polyRec);
}

std::vector<unsigned int>
NaivePickingAcceleratorBase::Pick (const ::Vec3 & posOrig) const
{
	::Vec3 pos (posOrig);
	pos.z = 0.0f;		// NOTE Jul 16, 2008: <pvl> we're 2D

	std::vector<unsigned int> matches;
	matches.reserve (16);

	std::vector<PolygonRec>::const_iterator it = m_polys.begin ();
	std::vector<PolygonRec>::const_iterator end = m_polys.end ();
	for ( ; it != end; ++it)
	{
		if (it->m_aabb.IsContainPoint (pos))
		{
			if (PointInConvexPolygon (it->m_poly, pos))
				matches.push_back (it->m_poly);
		}
	}

	return matches;
}

// ---

QuadtreePickingAcceleratorBase::Node::Node (const ::Vec3 & min, const ::Vec3 & max) :
		m_aabb (Vec2 (min), Vec2 (max))
{
	for (int i=0; i<4; ++i)
		m_children[i] = 0;
}

QuadtreePickingAcceleratorBase::Node::~Node ()
{
	for (int i=0; i<4; ++i)
		delete m_children[i];
}

void QuadtreePickingAcceleratorBase::Node::AllocateChild (Quadrant q)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "LNM - Picker - Nodes");
	// TODO Jul 15, 2008: <pvl> precompute any of these?
	Vec2 nodeMin (m_aabb.min.x, m_aabb.min.y);
	Vec2 nodeMax (m_aabb.max.x, m_aabb.max.y);
	Vec2 nodeMid ((m_aabb.max.x + m_aabb.min.x) / 2, (m_aabb.max.y + m_aabb.min.y) / 2);
	if (q == LL)
		m_children[q] = new Node (::Vec3 (nodeMin.x, nodeMin.y, 0), ::Vec3 (nodeMid.x, nodeMid.y, 0));
	else if (q == LR)
		m_children[q] = new Node (::Vec3 (nodeMid.x, nodeMin.y, 0), ::Vec3 (nodeMax.x, nodeMid.y, 0));
	else if (q == UL)
		m_children[q] = new Node (::Vec3 (nodeMin.x, nodeMid.y, 0), ::Vec3 (nodeMid.x, nodeMax.y, 0));
	else if (q == UR)
		m_children[q] = new Node (::Vec3 (nodeMid.x, nodeMid.y, 0), ::Vec3 (nodeMax.x, nodeMax.y, 0));
}

void QuadtreePickingAcceleratorBase::Node::DeallocateChild (Quadrant q)
{
	delete m_children[q];
	m_children[q] = 0;
}

inline bool QuadtreePickingAcceleratorBase::Node::QuadrantsAreNeighbors (Quadrant q0, Quadrant q1) const
{
	if (q0 == UL && q1 != LR)
		return true;
	else if (q0 == UR && q1 != LL)
		return true;
	else if (q0 == LR && q1 != UL)
		return true;
	else if (q0 == LL && q1 != UR)
		return true;
	return false;
}

QuadtreePickingAcceleratorBase::Node::Quadrant
QuadtreePickingAcceleratorBase::Node::GetQuadrant (const ::Vec3 & pos) const
{
	// FIXME Jul 15, 2008: <pvl> slow, precompute this?
	const float vertBoundary = (m_aabb.max.x + m_aabb.min.x) / 2;
	const float horizBoundary = (m_aabb.max.y + m_aabb.min.y) / 2;

	if (pos.x <= vertBoundary)
	{
		if (pos.y <= horizBoundary)
			return LL;
		else
			return UL;
	}
	else
	{
		if (pos.y <= horizBoundary)
			return LR;
		else
			return UR;
	}
}

inline bool QuadtreePickingAcceleratorBase::Node::NodeSubdivideable (float leafNodeSizeSqr) const
{
	return m_aabb.GetSize().GetLength2() > leafNodeSizeSqr;
}

inline bool QuadtreePickingAcceleratorBase::Node::Empty () const
{
	for (int i=0; i < 4; ++i)
		if (m_children[i])
			return false;

	return m_polys.empty ();
}


void QuadtreePickingAcceleratorBase::Node::AddToChild (
		Quadrant childQ, unsigned int poly, const AABB2 & polyAABB, float leafNodeSizeSqr)
{
	if (m_children[childQ] == 0)
		AllocateChild (childQ);
	m_children[childQ]->Add (poly, polyAABB, leafNodeSizeSqr);
}

void QuadtreePickingAcceleratorBase::Node::AddToThis (
		unsigned int poly, const AABB2 & polyAABB)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "LNM - Picker - PolygonRec");
	m_polys.push_back (PolygonRec (poly, polyAABB));
}

void QuadtreePickingAcceleratorBase::Node::RemoveFromChild (
		Quadrant childQ, unsigned int poly, const AABB2 & polyAABB)
{
	if (m_children[childQ] == 0) return;

	m_children[childQ]->Remove (poly, polyAABB);
	if (m_children[childQ]->Empty ())
		DeallocateChild (childQ);
}

void QuadtreePickingAcceleratorBase::Node::RemoveFromThis (unsigned int poly)
{
	// TODO Nov 6, 2008: <pvl> don't know yet if this could be a problem in
	// practice but this is obviously slow for higher values of m_polys.size()
	std::vector<PolygonRec>::iterator toBeRemoved = std::find_if (m_polys.begin (), m_polys.end (), PolygonRecCmp (poly));
	assert (toBeRemoved != m_polys.end ());
	m_polys.erase (toBeRemoved);
}

// FIXME Jul 22, 2008: <pvl> the following algorithm is based purely on rough
// AABB-AABB tests - add a final proper test to see if the *polygon* itself
// (not just its AABB) intersects a cell!
void QuadtreePickingAcceleratorBase::Node::Add (
		unsigned int poly, const AABB2 & polyAABB, float leafNodeSizeSqr)
{
	Quadrant minQ = GetQuadrant (polyAABB.min);
	Quadrant maxQ = GetQuadrant (polyAABB.max);
	if (minQ != maxQ)
	{
		// NOTE Jul 15, 2008: <pvl> straddles a boundary separating children

		// NOTE Jul 15, 2008: <pvl> if 'polyAABB' only overlaps 2 children (they
		// are necessarily neighbors then) pass it to these children for further
		// classification.  Otherwise there's no point, store it in this node.
		if (QuadrantsAreNeighbors (minQ, maxQ) && NodeSubdivideable (leafNodeSizeSqr))
		{
			AddToChild (minQ, poly, polyAABB, leafNodeSizeSqr);
			AddToChild (maxQ, poly, polyAABB, leafNodeSizeSqr);
		}
		else
		{
			// NOTE Jul 16, 2008: <pvl> 'polyAABB' overlaps all four children.  If it's
			// "big enough" store it in this node.  This should make sense since even
			// if it means that each query traversing through this node will need to
			// test against this polygon, the polygon is big so there's a good chance
			// it will be a hit.
			// If the poly is small push it down to all 4 children to avoid small stuff
			// stuck high up in the tree hierarchy.
			if (Area (polyAABB) > 0.25f * Area (m_aabb) || ! NodeSubdivideable (leafNodeSizeSqr))
			{
				AddToThis (poly, polyAABB);
			}
			else
			{
				for (int i=0; i < 4; ++i)
					AddToChild (static_cast<Quadrant>(i), poly, polyAABB, leafNodeSizeSqr);
			}
		}
	}
	else
	{
		// FIXME Jul 15, 2008: <pvl> add a check against max allowed tree depth
		// so that very small objects don't cause the depth grow excessively
		// TODO Jul 16, 2008: <pvl> does the above comment still apply?  Does the
		// algorithm not have stopping criteria in it already?
		if (m_aabb.ContainsBox (polyAABB) && NodeSubdivideable (leafNodeSizeSqr))
			AddToChild (minQ, poly, polyAABB, leafNodeSizeSqr);
		else
			AddToThis (poly, polyAABB);
	}
}

// NOTE Nov 7, 2008: <pvl> reflects the algorithm implemented in Add() - any
// change to Add() have to be checked to see if Remove() needs a similar one
void QuadtreePickingAcceleratorBase::Node::Remove (
	unsigned int poly, const AABB2 & polyAABB)
{
	Quadrant minQ = GetQuadrant (polyAABB.min);
	Quadrant maxQ = GetQuadrant (polyAABB.max);
	if (minQ != maxQ)
	{
		// NOTE Jul 15, 2008: <pvl> straddles a boundary separating children

		// NOTE Jul 15, 2008: <pvl> 'polyAABB' only overlaps 2 children (they
		// are necessarily neighbors then)
		assert(minQ < 4);
		if (QuadrantsAreNeighbors (minQ, maxQ) && m_children[minQ])
		{
			// NOTE Nov 6, 2008: <pvl> from the way Add() works I guess we should
			// have either both children or none of them
			assert (m_children[minQ] && m_children[maxQ]);
			RemoveFromChild (minQ, poly, polyAABB);
			RemoveFromChild (maxQ, poly, polyAABB);
		}
		else
		{
			// NOTE Jul 16, 2008: <pvl> 'polyAABB' overlaps all four children.  If it's
			// "big enough" it's stored in this node.
			if (Area (polyAABB) > 0.25f * Area (m_aabb) || (!m_children[0] && !m_children[1] && !m_children[2] && !m_children[3]))
			{
				RemoveFromThis (poly);
			}
			else
			{
				for (int i=0; i < 4; ++i)
					RemoveFromChild (static_cast<Quadrant>(i), poly, polyAABB);
			}
		}
	}
	else
	{
		assert( (int)minQ < 4 );
		if (m_aabb.ContainsBox (polyAABB) && m_children[minQ])
			RemoveFromChild (minQ, poly, polyAABB);
		else
			RemoveFromThis (poly);
	}
}

void QuadtreePickingAcceleratorBase::Node::FreeUnusedMemory ()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "LNM - Picker - PolygonRec");
	std::vector <PolygonRec> (m_polys).swap (m_polys);

	for (int i=0; i < 4; ++i)
	{
		Node * childNode = m_children[i];
		if (childNode)
			childNode->FreeUnusedMemory ();
	}
}

// TODO Jul 15, 2008: <pvl> depending on how deep the trees will be getting
// with real data it might make sense to get rid of recursion!
//
/// Assumes the caller already checked that 'pos' is inside this Node's AABB.
void QuadtreePickingAcceleratorBase::Node::Pick (
		const ::Vec3 & pos,
		std::vector<unsigned int> & matches,
		const QuadtreePickingAcceleratorBase * tree) const
{
	// NOTE Jul 15, 2008: <pvl> test against each polygon in this node
	std::vector <PolygonRec>::const_iterator it = m_polys.begin ();
	std::vector <PolygonRec>::const_iterator end = m_polys.end ();
	for ( ; it != end; ++it)
	{
		if ( it->m_aabb.IsContainPoint (pos) )
		{
			if (tree->PointInConvexPolygon (it->m_poly, pos))
				matches.push_back (it->m_poly);
		}
	}

	Node * childNode = m_children[GetQuadrant(pos)];
	if (childNode && childNode->GetAABB().IsContainPoint (pos))
		childNode->Pick (pos, matches, tree);
}

void QuadtreePickingAcceleratorBase::Node::Pick (
		const AABB2 & area,
		std::vector<unsigned int> & matches,
		const QuadtreePickingAcceleratorBase * tree) const
{
	// NOTE Jul 15, 2008: <pvl> test against each polygon in this node
	std::vector <PolygonRec>::const_iterator it = m_polys.begin ();
	std::vector <PolygonRec>::const_iterator end = m_polys.end ();
	for ( ; it != end; ++it)
	{
		if ( it->m_aabb.IsIntersectBox (area) )
		{
			// TODO Sep 18, 2008: <pvl> only uses AABB for testing, not the polygon itself
			matches.push_back (it->m_poly);
		}
	}

	for (int i=0; i<4; ++i)
	{
		Node * childNode = m_children[i];
		if (childNode && childNode->GetAABB().IsIntersectBox (area))
			childNode->Pick (area, matches, tree);
	}
}

void QuadtreePickingAcceleratorBase::Node::CollectStats (Stats * stats) const
{
	++stats->m_numNodes;

	unsigned numChildren=0;
	for (unsigned i=0; i < 4; ++i)
		if (m_children[i])
			++numChildren;

	++stats->m_numNodesWithNumChildren[numChildren];

	unsigned polyHistogramBand = m_polys.size();
	if (polyHistogramBand > 10 - 1)
		polyHistogramBand = 10 - 1;

	++stats->m_numPolygonsInNode[polyHistogramBand];

	for (int i=0; i < 4; ++i)
	{
		Node * childNode = m_children[i];
		if (childNode)
			childNode->CollectStats (stats);
	}
}

QuadtreePickingAcceleratorBase::QuadtreePickingAcceleratorBase (
			const ::Vec3 & min, const ::Vec3 & max, float leafNodeSize) :
		m_root (new Node (min, max)), m_leafNodeSizeSqr (leafNodeSize*leafNodeSize)
{
}

QuadtreePickingAcceleratorBase::~QuadtreePickingAcceleratorBase ()
{
	delete (m_root);
}

void QuadtreePickingAcceleratorBase::Add (unsigned int poly)
{
	AABB2 polyAABB (GetPolyAABB (poly));
	const AABB2 & rootAABB (m_root->GetAABB ());

	// NOTE Jul 15, 2008: <pvl> sanity check - if we're asked to add an AABB that's
	// not fully covered by the tree root there's quite possibly something wrong
	if ( ! rootAABB.ContainsBox (polyAABB))
	{
		AIWarning ("Asked to add a poly straddling the boundary of the tree!");
		return;
	}

	m_root->Add (poly, polyAABB, m_leafNodeSizeSqr);
}

void QuadtreePickingAcceleratorBase::Remove (unsigned int poly)
{
	// TODO Nov 6, 2008: <pvl> add similar sanity checks as in Add()
	AABB2 polyAABB (GetPolyAABB (poly));
	m_root->Remove (poly, polyAABB);
}

void QuadtreePickingAcceleratorBase::FreeUnusedMemory ()
{
	m_root->FreeUnusedMemory ();
}

std::vector<unsigned int>
QuadtreePickingAcceleratorBase::Pick (const ::Vec3 & posOrig) const
{
	::Vec3 pos (posOrig);
	pos.z = 0.0f;		// NOTE Jul 16, 2008: <pvl> we're 2D

	std::vector<unsigned int> matches;
	matches.reserve (16);

	if (m_root->GetAABB ().IsContainPoint (pos))
		m_root->Pick (pos, matches, this);

	return matches;
}

std::vector<unsigned int>
QuadtreePickingAcceleratorBase::Pick (const AABB & areaOrig) const
{
	AABB2 area (areaOrig);

	std::vector<unsigned int> matches;
	matches.reserve (16);

	if (m_root->GetAABB ().IsIntersectBox (area))
		m_root->Pick (area, matches, this);

	// NOTE Sep 25, 2008: <pvl> in this tree, a single polygon can end up being
	// referenced by multiple tree cells (see how Node::Add() works).  It means
	// that an AABB query can have duplicate entries in its 'matches' vector.
	// That can happen if a polygon lives in multiple cells and a testing area
	// overlaps it in more than one of them.
	// TODO Sep 25, 2008: <pvl> could this be done faster/more elegantly?
	std::sort (matches.begin (), matches.end ());
	std::vector <unsigned int>::iterator newEnd = std::unique (matches.begin (), matches.end ());
	matches.erase (newEnd, matches.end ());

	return matches;
}

QuadtreePickingAcceleratorBase::Stats QuadtreePickingAcceleratorBase::CollectStats () const
{
	Stats stats;
	m_root->CollectStats ( & stats );
	return stats;
}

#ifdef DEBUG_DRAW
void QuadtreePickingAcceleratorBase::DebugDraw () const
{
	m_root->DebugDraw (0);
}
#endif


void QuadtreePickingAcceleratorBase::Stats::Print () const
{
	AILogAlways ("Picking accelerator: node count = %d\n", m_numNodes);

	AILogAlways ("break-down by number of child nodes:\n");
	for (unsigned i=0; i < 4; ++i)
		AILogAlways ("  * nodes with %d %8s: %d\n", i, i==1 ? "child" : "children", m_numNodesWithNumChildren[i]);

	AILogAlways ("break-down by number of polygons stored in a node:\n");
	for (unsigned i=0; i < 10; ++i)
		AILogAlways ("  * nodes with %d %8s: %d\n", i, i==1 ? "polygon" : "polygons", m_numPolygonsInNode[i]);
}

// ---

#include "NavPolygonGenerator.h"
using LayeredNavMesh::NavPolygonGenerator;

AABB2 EditorPolygonContext::GetPolyAABB (unsigned int poly) const
{
	return ::GetAABB (
			NavPolygonGenerator::NavPolyPtr (
					m_polyGen->GetPoly(poly).GetInternalPoly(),	m_polyGen
			)
	);
}

bool EditorPolygonContext::PointInConvexPolygon (unsigned int poly, const ::Vec3 & pos) const
{
	return ::PointInConvexPoly (
			NavPolygonGenerator::NavPolyPtr (
					m_polyGen->GetPoly(poly).GetInternalPoly(), m_polyGen
			),
			pos
	);
}

#include "PolygonCont.h"
using LayeredNavMesh::PolygonCont;

AABB2 RuntimePolygonContext::GetPolyAABB (unsigned int poly) const
{
	return ::GetAABB ( (*m_polyCont)[poly] );
}

bool RuntimePolygonContext::PointInConvexPolygon (unsigned int poly, const ::Vec3 & pos) const
{
	return ::PointInConvexPoly ( (*m_polyCont)[poly], pos );
}
