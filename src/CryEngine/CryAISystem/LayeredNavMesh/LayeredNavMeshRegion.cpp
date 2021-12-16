
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
#include "Cry_GeoIntersect.h"
#include "Cry_Color.h"
#include "IRenderAuxGeom.h"
#include "IRenderer.h"
#include "IEntity.h"
#include "ITimer.h"
#include "StdAfx.h"
#include <map>
#endif // USE_STDAFX


#include "LayeredNavMeshRegion.h"
#include "../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/ExportFormat.h"
#include "../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/LnmExportFormat.h"
#include "../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/PickingAccelerator.h"
//#include "../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/DebugDraw.h"
#include "PolygonSetOps.h"

#include "DebugDrawContext.h"

#include "CalculationStopper.h"

//#pragma optimize ("", off)
//#pragma inline_depth (0)

namespace ExportFormat = LayeredNavMesh::ExportFormat;
namespace LnmExportFormat = LayeredNavMesh::LnmExportFormat;

// ATTN Dec 11, 2008: <pvl> obsolete, see comment near NavGraphGenerator
class GraphNodeIndexListChunkReader : public LayeredNavMesh::ExportFormat::VersionedChunkReader
{
	std::vector <unsigned int> & m_graphNodeIndices;
public:
	GraphNodeIndexListChunkReader (std::vector <unsigned int> & graphNodeIndices);
	virtual bool ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & );
};

GraphNodeIndexListChunkReader::GraphNodeIndexListChunkReader (std::vector<unsigned int> & graphNodeIndices) :
		m_graphNodeIndices (graphNodeIndices)
{
}

bool GraphNodeIndexListChunkReader::ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & file)
{
	LnmExportFormat::NavMeshGraphNodeIndexListChunk::Header hdr (file);

	m_graphNodeIndices.resize (hdr.m_numIndices);

	// TODO Jul 18, 2008: <pvl> check if is this OK with respect to various platforms?
	file.ReadRaw ( & m_graphNodeIndices[0], hdr.m_numIndices * sizeof (unsigned int));

	return true;
}


class SceneBoundsChunkReader : public LayeredNavMesh::ExportFormat::VersionedChunkReader
{
	Vec3 & m_sceneMin;
	Vec3 & m_sceneMax;
public:
	SceneBoundsChunkReader (Vec3 & sceneMin, Vec3 & sceneMax);
	virtual bool ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & );
};

SceneBoundsChunkReader::SceneBoundsChunkReader (Vec3 & sceneMin, Vec3 & sceneMax) :
		m_sceneMin (sceneMin), m_sceneMax (sceneMax)
{
}

bool SceneBoundsChunkReader::ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & file)
{
	bool ok = true;

	ok = ok && (0 != file.ReadType ( & m_sceneMin.x) );
	ok = ok && (0 != file.ReadType ( & m_sceneMin.y) );
	ok = ok && (0 != file.ReadType ( & m_sceneMin.z) );

	ok = ok && (0 != file.ReadType ( & m_sceneMax.x) );
	ok = ok && (0 != file.ReadType ( & m_sceneMax.y) );
	ok = ok && (0 != file.ReadType ( & m_sceneMax.z) );

	return ok;
}


class ModifierInfoChunkReader : public LayeredNavMesh::ExportFormat::VersionedChunkReader
{
	string & m_name;
	unsigned & m_id;
public:
	ModifierInfoChunkReader (string & name, unsigned & id);
	virtual bool ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & );
};

ModifierInfoChunkReader::ModifierInfoChunkReader (string & name, unsigned & id) :
		m_name (name), m_id (id)
{
}

bool ModifierInfoChunkReader::ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & file)
{
	bool ok = true;

	unsigned nameLen = 0;
	ok = ok && (0 != file.ReadType ( & nameLen) );
	// NOTE Feb 18, 2009: <pvl> arbitrary constant to check against - last defense
	// against reading crap from a file and bringing allocator to its knees ;-)
	assert (nameLen < 255);

	char * buff = new char [nameLen+1];
	ok = ok && (0 != file.ReadRaw (buff, nameLen));
	buff[nameLen] = '\0';
	m_name.assign (buff);
	delete [] buff;

	ok = ok && (0 != file.ReadType ( & m_id) );

	return ok;
}


// NOTE Aug 7, 2009: <pvl> extend the v1 reader by handling agent type related stuff
class ModifierInfoChunkReader_v2 : public ModifierInfoChunkReader
{
	int & m_agentTypeId;
	string & m_agentTypeName;
public:
	ModifierInfoChunkReader_v2 (string & name, unsigned & id,
			int & agentTypeId, string & agentTypeName
	);
	virtual bool ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & );
};

ModifierInfoChunkReader_v2::ModifierInfoChunkReader_v2 (string & name, unsigned & id,
			int & agentTypeId, string & agentTypeName) :
		ModifierInfoChunkReader (name, id), m_agentTypeId (agentTypeId), m_agentTypeName (agentTypeName)
{
}

bool ModifierInfoChunkReader_v2::ReadChunk (const ExportFormat::VersionChunkHeader & versionHdr, CCryFile & file)
{
	bool ok = ModifierInfoChunkReader::ReadChunk (versionHdr, file);

	ok = ok && (0 != file.ReadType ( & m_agentTypeId) );

	unsigned agentTypeNameLen = 0;
	ok = ok && (0 != file.ReadType ( & agentTypeNameLen) );
	// NOTE Feb 18, 2009: <pvl> arbitrary constant to check against - last defense
	// against reading crap from a file and bringing allocator to its knees ;-)
	assert (agentTypeNameLen < 255);

	char * buff = new char [agentTypeNameLen+1];
	ok = ok && (0 != file.ReadRaw (buff, agentTypeNameLen));
	buff[agentTypeNameLen] = '\0';
	m_agentTypeName.assign (buff);
	delete [] buff;

	return ok;
}


// ---

#include <set>
#include <algorithm>
#include <iterator>
#include "DynamicVertex.h"

using LayeredNavMesh::PolygonCont;

/**
 * This class encapsulates getting the two vertices that two neighboring
 * nav mesh polygons have in common.
 *
 * This implementation uses std::sets to compute common vertices in runtime.
 * Should this prove suboptimal it will be enough to implement a class that
 * has a way of passing it two polygons and supports Count() and operator[]()
 * members.
 *
 * (Two things that come to mind would be to either have common vertices
 * precomputed or still compute them at runtime but avoid std::sets.)
 *
 * UPDATE Dec 2, 2009: <pvl> the latter :-)
 */
class CommonPolyVertices {
	unsigned m_common[2];
	unsigned m_numCommon;

	void HandleDynamicPolys (const PolygonCont::Polygon & poly0, const PolygonCont::Polygon & poly1);
public:
	CommonPolyVertices (const PolygonCont::Polygon & poly0, const PolygonCont::Polygon & poly1);

	int Count () const { return m_numCommon; }

	unsigned operator[] (unsigned i) const { assert (i < 2); return m_common[i]; }
};

void CommonPolyVertices::HandleDynamicPolys (
			const PolygonCont::Polygon & poly0,
			const PolygonCont::Polygon & poly1)
{
	assert (poly0.IsDynamic () && poly1.IsDynamic ());

	const unsigned numVtxs0 = poly0.NumVertices ();
	const unsigned numVtxs1 = poly1.NumVertices ();

	for (unsigned int p0=0; p0 < numVtxs0; ++p0)
		for (unsigned int p1=0; p1 < numVtxs1; ++p1)
			if ( *poly0.GetVtxAttribs (p0) == *poly1.GetVtxAttribs (p1) )
			{
				// NOTE Nov 6, 2008: <pvl> using poly0 and p0 or poly1 and p1 should
				// now be guaranteed to give the same vertex in terms of position
				m_common[m_numCommon++] = poly0.GetVtxIndex (p0);
				if (m_numCommon == 2)
					return;
			}
}

CommonPolyVertices::CommonPolyVertices (
			const PolygonCont::Polygon & poly0,
			const PolygonCont::Polygon & poly1) :
		m_numCommon (0)
{
//	FRAME_PROFILER ("$2   CommonPolyVertices", gEnv->pSystem, PROFILE_AI)

	if (poly0.IsDynamic () && poly1.IsDynamic ())
	{
		HandleDynamicPolys (poly0, poly1);
		return;
	}

	PolygonCont::Polygon::vertex_index_iterator it0 = poly0.index_begin ();
	const PolygonCont::Polygon::vertex_index_iterator end0 = poly0.index_end ();
	for ( ; it0 != end0; ++it0)
	{
		PolygonCont::Polygon::vertex_index_iterator it1 = poly1.index_begin ();
		const PolygonCont::Polygon::vertex_index_iterator end1 = poly1.index_end ();

		for ( ; it1 != end1; ++it1)
		{
			if (*it0 != *it1) continue;

			m_common[m_numCommon++] = *it0;
			if (m_numCommon == 2)
				return;
		}
	}
}

// ---

class PathBeautifier {
public:

	virtual ~PathBeautifier () { }

	typedef VectorConstNodeIndices::const_iterator PathIter;

	/**
	 * Takes a path expressed as an interval into a sequence of graph node
	 * indices and returns a beautified path as a sequence of path points.
	 * Use the swap trick to avoid copying.
	 *
	 * Any other data that a particular implementation of this interface might
	 * need for Beautify() to run are expected to be passed through ctor of
	 * that subtype.
	 */
	virtual TPathPoints Beautify (PathIter pathBegin, PathIter pathEnd) const = 0;

	virtual void DebugDraw () const { }
};

bool Left (const Vec3 & v0, const Vec3 & v1, const Vec3 & test)
{
	Vec3 seg0 = v1 - v0;
	Vec3 seg1 = test - v0;

	return seg0.x * seg1.y - seg0.y * seg1.x > 0.0f;
}

class Angle {
	Vec3 m_vertex;
	Vec3 m_leftRay;
	Vec3 m_rightRay;
public:
	Angle (const Vec3 & vertex, const Vec3 & leftRay, const Vec3 & rightRay);
	const Vec3 & LeftRay () const { return m_leftRay; }
	const Vec3 & RigthRay () const { return m_rightRay; }
	const Vec3 & Vertex () const { return m_vertex; }
	bool Degenerate () const { return m_vertex == m_leftRay || m_vertex == m_rightRay; }
};

Angle::Angle (const Vec3 & vertex, const Vec3 & leftRay, const Vec3 & rightRay) :
		m_vertex (vertex), m_leftRay (leftRay), m_rightRay (rightRay)
{
}

enum PositionWrtAngle { OUTSIDE_ON_THE_LEFT, INSIDE, OUTSIDE_ON_THE_RIGHT };
PositionWrtAngle Classify (const Angle & angle, const Vec3 & pt)
{
	// NOTE Aug 12, 2008: <pvl> this way, the calling code will just advance its
	// last vis iterator, proceeding to the next portal which is what we want
	if (angle.Degenerate ()) return INSIDE;

	if (Left (angle.Vertex (), angle.LeftRay (), pt))
		return OUTSIDE_ON_THE_LEFT;
	else if (Left (angle.Vertex (), angle.RigthRay (), pt))
		return INSIDE;
	return OUTSIDE_ON_THE_RIGHT;
}

/**
 * Essentially an implementation of the algorithm presented in David Miles'
 * GDC06 talk "Crowds in a Polygon Soup".
 */
class SimplePathBeautifier : public PathBeautifier {

	// NOTE Aug 12, 2008: <pvl> a "portal" is the edge shared by neigboring
	// nav mesh polygons along a path (see the talk referenced above)
	struct Portal {
		Vec3 m_left, m_right;

		// NOTE Aug 13, 2008: <pvl> store indices as well, they're good for testing
		// vertex equality and also debugging
		unsigned int m_leftIndex, m_rightIndex;

		// TODO Aug 13, 2008: <pvl> this should probably be defined somewhere else,
		// maybe in PolygonCont?  sizeof(Portal) doesn't matter, it's a tmp struct
		// used in very moderate amounts.
		static const unsigned int s_invalidIndex /*= std::numeric_limits<unsigned>::max()*/;

		Portal () { }						// NOTE Aug 8, 2008: <pvl> std::vector needs this
		Portal (const Vec3 & left, const Vec3 & right) :
				m_left (left), m_right (right), m_leftIndex (s_invalidIndex), m_rightIndex (s_invalidIndex)
		{ }
		Portal (const Vec3 & left, const Vec3 & right, unsigned leftIndex, unsigned rightIndex) :
				m_left (left), m_right (right), m_leftIndex (leftIndex), m_rightIndex (rightIndex)
		{ }
	};
	typedef std::vector <Portal> Portals;
	/// Series of portals representing the path being beautified.
	mutable Portals m_portals;

	// TODO Aug 8, 2008: <pvl> should we return portals as a return value and
	// get rid of 'm_portals'?
	// TODO Aug 8, 2008: <pvl> should this function be a member at all?
	void GetPortals (PathIter pathBegin, PathIter pathEnd, const Vec3 & startPos) const;

	// ---

	const PolygonCont & m_polygons;

	const Vec3 m_startPos;
	const Vec3 m_endPos;

public:
	// FIXME Aug 8, 2008: <pvl> division between what's passed through ctor args
	// vs. Beautify() args looks a bit arbitrary
	SimplePathBeautifier (const Vec3 & startPos, const Vec3 & endPos, const PolygonCont & );

	virtual TPathPoints Beautify (PathIter pathBegin, PathIter pathEnd) const;

	virtual void DebugDraw () const;
};

const unsigned int SimplePathBeautifier::Portal::s_invalidIndex = std::numeric_limits<unsigned>::max();

SimplePathBeautifier::SimplePathBeautifier (
			const Vec3 & startPos, const Vec3 & endPos, const PolygonCont & polys) :
		m_polygons (polys), m_startPos (startPos), m_endPos (endPos)
{
}

void SimplePathBeautifier::GetPortals (PathIter pathBegin, PathIter pathEnd, const Vec3 & startPos) const
{
	m_portals.reserve (pathEnd - pathBegin);

	Vec3 currPos = startPos;

	PathIter thisNodeIt = pathBegin;
	PathIter nextNodeIt = pathBegin + 1;
	const PathIter endNodeIt = pathEnd;
	for ( ; nextNodeIt != endNodeIt; ++thisNodeIt, ++nextNodeIt)
	{
		// FIXME Okt 14, 2008: <pvl> should use its LayeredNavMeshRegion instance's 
		// graph!  (Or even get it from somewhere else if no one-to-one correspondence
		// between graphs and nav region instances is maintained.)
		const GraphNode * node0 = gAIEnv.pGraph->GetNodeManager().GetNode (*thisNodeIt);
		const GraphNode * node1 = gAIEnv.pGraph->GetNodeManager().GetNode (*nextNodeIt);
		const unsigned polyIndex0 = node0->GetLayeredMeshNavData()->polygonIndex;
		const unsigned polyIndex1 = node1->GetLayeredMeshNavData()->polygonIndex;
		const PolygonCont::Polygon poly0 = m_polygons[polyIndex0];
		const PolygonCont::Polygon poly1 = m_polygons[polyIndex1];

		CommonPolyVertices commonVerts (poly0, poly1);
		assert (commonVerts.Count () == 2);
		Vec3 endPt0 = m_polygons.GetVertex (commonVerts[0]);
		Vec3 endPt1 = m_polygons.GetVertex (commonVerts[1]);

		if (Left (currPos, endPt0, endPt1))
			m_portals.push_back (Portal (endPt1, endPt0, commonVerts[1], commonVerts[0]));
		else
			m_portals.push_back (Portal (endPt0, endPt1, commonVerts[0], commonVerts[1]));

		// NOTE Aug 12, 2008: <pvl> use the centerpoint of this portal as the point
		// of view from where we'll decide which of the next portal's endpoints
		// is the right or left one
		currPos = (endPt0 + endPt1) / 2.0f;
	}

	// NOTE Aug 12, 2008: <pvl> put the endpos in as the last portal (of zero
	// width).  This little trick simplifies the beautification algorithm
	// implementation a bit.
	m_portals.push_back (Portal (m_endPos, m_endPos));
}

template <typename It>
inline bool Finished (const It & it, const It & end)
{
	It next (it);
	if (++next == end)
		return true;
	return false;
}
TPathPoints SimplePathBeautifier::Beautify (PathIter pathBegin, PathIter pathEnd) const
{
	TPathPoints beautifiedPath;

	GetPortals (pathBegin, pathEnd, m_startPos);
	beautifiedPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, m_startPos));

	Vec3 currPos (m_startPos);

	Portals::const_iterator portalIt = m_portals.begin ();
	Portals::const_iterator lastVisLeftIt = portalIt;
	Portals::const_iterator lastVisRightIt = portalIt;
	++portalIt;

	Portals::const_iterator endPortals = m_portals.end ();

	for ( ; portalIt != endPortals; ++portalIt)
	{
		// NOTE Nov 19, 2008: <pvl> at this point, 'currPos' should either be the
		// start position or an endpoint of a portal.  'lastVisLeftIt' and
		// 'lastVisRightIt' should both point to the next portal and 'portalIt'
		// to the portal after that.
		// Initially, this can always be achieved since we have always at least 2
		// portals (we don't get even called for single-node paths so there's at
		// least one portal, plus the endpoint is added as a portal as well).
		// As the algorithm runs, code that advances the iterators needs to make
		// sure this invariant isn't broken.

		// NOTE Nov 7, 2008: <pvl> this algorithm can be sent into an infinite loop
		// by faulty rubbish data.  This check is to guard against it.  It shouldn't
		// ever trigger for good data, only when there's a problem elsewhere.
		if (beautifiedPath.size () > 200/*more or less random threshold*/)
			return beautifiedPath;

		switch (Classify (Angle (currPos, lastVisLeftIt->m_left, lastVisRightIt->m_right), portalIt->m_left))
		{
		case OUTSIDE_ON_THE_LEFT:
			break;
		case INSIDE:
			if (Finished (portalIt, endPortals))
			{
				beautifiedPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, portalIt->m_left));
				// NOTE Aug 12, 2008: <pvl> ends the loop
				continue;
			}
			else
				lastVisLeftIt = portalIt;
			break;
		case OUTSIDE_ON_THE_RIGHT:
			currPos = lastVisRightIt->m_right;
			beautifiedPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, currPos));
			portalIt = lastVisLeftIt = ++lastVisRightIt;
			if (Finished (portalIt, endPortals))
			{
				beautifiedPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, portalIt->m_left));
				// NOTE Aug 12, 2008: <pvl> ends the loop
				continue;
			}
			break;
		}

		switch (Classify (Angle (currPos, lastVisLeftIt->m_left, lastVisRightIt->m_right), portalIt->m_right))
		{
		case OUTSIDE_ON_THE_LEFT:
			currPos = lastVisLeftIt->m_left;
			beautifiedPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, currPos));
			portalIt = lastVisRightIt = ++lastVisLeftIt;
			if (Finished (portalIt, endPortals))
			{
				beautifiedPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, portalIt->m_left));
				// NOTE Aug 12, 2008: <pvl> ends the loop
				continue;
			}
			break;
		case INSIDE:
			if (Finished (portalIt, endPortals))
			{
				beautifiedPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, portalIt->m_right));
				// NOTE Aug 12, 2008: <pvl> ends the loop
				continue;
			}
			else
				lastVisRightIt = portalIt;
			break;
		case OUTSIDE_ON_THE_RIGHT:
			break;
		}
	}
	return beautifiedPath;
}

void SimplePathBeautifier::DebugDraw () const
{
	CDebugDrawContext dc;
	// draw portals
	const float ballRadius = 0.3f;
	const ColorB leftCol (0, 0, 64, 255);
	const ColorB rightCol (160, 160, 255, 255);
	const Vec3 verticalOffset (0.0f, 0.0f, 0.7f);

	Portals::const_iterator portalIt = m_portals.begin ();
	Portals::const_iterator portalEnd = m_portals.end ();
	for ( ; portalIt != portalEnd; ++portalIt)
	{
		const Vec3 left (portalIt->m_left + verticalOffset);
		const Vec3 right (portalIt->m_right + verticalOffset);
		
		dc->DrawLine (left, leftCol, right, rightCol);
		dc->DrawSphere (left, ballRadius, leftCol);
		dc->DrawSphere (right, ballRadius, rightCol);
	}
}

// ---

#include "PolygonSetOpsUtil.h"

using PSO::Allocator;
using PSO::PoolAllocator;
using PSO::StdAllocator;
using PSO::Polygon;

using PSO::VtxUserData;
using PSO::CrossVtxUserData;
using PSO::OrigVtxUserData;

using PSO::PolygonSetOp;

// NOTE Okt 8, 2008: <pvl> user data that can optionally be attached to polygon
// vertices.  This should be seen as a slightly more civilized version of
// the traditional void*.
struct VtxHeritage {
	char m_type;

	VtxHeritage (char type) : m_type (type) { }

	// NOTE Okt 2, 2008: <pvl> no (virtual) dtor since these aren't to be explicitly deleted

	virtual VtxHeritage * Clone (Allocator * ) const = 0;
	virtual void DebugDraw (const Vec3 & pos) const = 0;
};

// NOTE Okt 8, 2008: <pvl> the purpose of NavPolyVtxHeritage and SShapeVtxHeritage 
// is to maintain information about where a vertex came from and make sure it
// gets carried over properly to resulting polygon vertices.  This info is of no
// consequence to the polygon set ops code but can be important to the caller
// (think LNM's dynamic obstacle handling).
//
// Basically, vertices of a polygon that's the result of a boolean operation on
// two other polygons can come from either operand or can be cross vertices that
// get created whenever two edges intersect.  In case of LNM dynamics, one of the
// operands will always be a polygon representing an obstacle and the other one
// either a static nav mesh polygon or the result of a previous operation.
struct NavPolyVtxHeritage : public VtxHeritage {
	unsigned int m_vtxIndex;

	NavPolyVtxHeritage (unsigned index) :
			VtxHeritage (USER_DATA_NAV_POLY), m_vtxIndex (index)
	{ }

	static void * operator new (size_t size, Allocator * alloc)
	{
		return alloc->Allocate (size);
	}
	static void operator delete (void * , Allocator * )
	{
		assert (0);
	}

	virtual NavPolyVtxHeritage * Clone (Allocator * allocator) const
	{
		return new (allocator) NavPolyVtxHeritage (*this);
	}
	virtual void DebugDraw (const Vec3 & pos) const
	{
		CDebugDrawContext dc;
		dc->Draw3dLabelEx (pos, 2.5f, ColorB(255, 255, 255), false, true, "static:%d", m_vtxIndex);
	}
	bool operator< (const NavPolyVtxHeritage & rhs) const
	{
		if (m_vtxIndex < rhs.m_vtxIndex)
			return true;
		return false;
	}
	bool operator== (const NavPolyVtxHeritage & rhs) const
	{
		if (m_vtxIndex == rhs.m_vtxIndex)
			return true;
		return false;
	}
};

struct SShapeVtxHeritage : public VtxHeritage {
	const void * m_id;
	unsigned int m_vtxNumber;

	SShapeVtxHeritage (const void * id, unsigned num) :
			VtxHeritage (USER_DATA_OBSTACLE_SSHAPE), m_id (id), m_vtxNumber (num)
	{ }
	static void * operator new (size_t size, Allocator * alloc)
	{
		return alloc->Allocate (size);
	}
	static void operator delete (void * , Allocator * )
	{
		assert (0);
	}
	virtual SShapeVtxHeritage * Clone (Allocator * allocator) const
	{
		return new (allocator) SShapeVtxHeritage (*this);
	}
	virtual void DebugDraw (const Vec3 & pos) const
	{
		CDebugDrawContext dc;
		dc->Draw3dLabelEx (pos, 2.5f, ColorB(255, 255, 255), false, true,
                                "SShape:%p[%d]", m_id, m_vtxNumber);
	}
	bool operator< (const SShapeVtxHeritage & rhs) const
	{
		if (m_id < rhs.m_id) return true;
		else if (m_id > rhs.m_id) return false;

		if (m_vtxNumber < rhs.m_vtxNumber)
			return true;
		return false;
	}
	bool operator== (const SShapeVtxHeritage & rhs) const
	{
		if (m_id != rhs.m_id)
			return false;

		if (m_vtxNumber == rhs.m_vtxNumber)
			return true;
		return false;
	}
};

bool operator== (const VtxHeritage & lhs, const VtxHeritage & rhs)
{
	if (lhs.m_type != rhs.m_type)
		return false;

	if (lhs.m_type == USER_DATA_NAV_POLY)
		return static_cast <const NavPolyVtxHeritage &> (lhs) == static_cast <const NavPolyVtxHeritage &> (rhs);
	else if (lhs.m_type == USER_DATA_OBSTACLE_SSHAPE)
		return static_cast <const SShapeVtxHeritage &> (lhs) == static_cast <const SShapeVtxHeritage &> (rhs);
	// NOTE Jan 26, 2009: <pvl> comparison shouldn't ever need to be called on cross vertices
	assert (0);
	return false;
}

class UserDataCompare {
public:
	bool operator() (const VtxHeritage * lhs, const VtxHeritage * rhs) const
	{
		if (lhs->m_type < rhs->m_type)
			return true;
		else if (lhs->m_type > rhs->m_type)
			return false;
		else
		{
			// NOTE Okt 10, 2008: <pvl> both types are the same, delegate to subtype comparison
			if (lhs->m_type == USER_DATA_NAV_POLY)
				return *(NavPolyVtxHeritage * )lhs < *(NavPolyVtxHeritage * )rhs;
			else
				return *(SShapeVtxHeritage * )lhs < *(SShapeVtxHeritage * )rhs;
			// NOTE Okt 10, 2008: <pvl> only static or obstacle vertices need to be compared
//			assert (0);
		}
	}
};

struct CrossVtxHeritage : public VtxHeritage {
	typedef std::pair <NavPolyVtxHeritage*,NavPolyVtxHeritage*> StaticEdge;

	// NOTE Okt 6, 2008: <pvl> user data of all four vertices (two edges) that
	// generated this cross vertex
	union {
		VtxHeritage * m_vtx[4];
		struct {
			// NOTE Okt 23, 2008: <pvl> lower numbered static vertex
			VtxHeritage * m_staticVtx0;
			// NOTE Okt 23, 2008: <pvl> higher numbered static vertex
			VtxHeritage * m_staticVtx1;
			// NOTE Okt 23, 2008: <pvl> lower numbered obstacle vertex
			VtxHeritage * m_obstacleVtx0;
			// NOTE Okt 23, 2008: <pvl> higher numbered obstacle vertex
			VtxHeritage * m_obstacleVtx1;
		};
	};

	void SortVertices ()
	{
		std::sort ( & m_vtx[0], & m_vtx[4], UserDataCompare());
	}

	// NOTE Jan 27, 2009: <pvl> checks to see if any of the endpoints is a cross
	// vertex.  If so it's replaced by the "real" vertex (that is, a vertex belonging
	// to one of the input polygons) behind it according to the following rules:
	// - if the other one isn't a cross vertex then it has to come up among the
	//   the four vertices in the cross vertex data.  The cross vertex is then
	//   replaced by the other side of the same edge.
	// - if both of them are cross vertices then one of the edges that produced
	//   them must be the same.  They are then replaced by endpoints of that edge.
	void CanonicaliseEdge (VtxHeritage ** vtx0, VtxHeritage ** vtx1)
	{
		if ( (*vtx0)->m_type != USER_DATA_CROSS_VTX	&& (*vtx1)->m_type != USER_DATA_CROSS_VTX)
		{
			return;
		}
		else if ( (*vtx0)->m_type == USER_DATA_CROSS_VTX && (*vtx1)->m_type == USER_DATA_CROSS_VTX)
		{
			CrossVtxHeritage ** crossVtx0 = reinterpret_cast <CrossVtxHeritage**> (vtx0);
			CrossVtxHeritage ** crossVtx1 = reinterpret_cast <CrossVtxHeritage**> (vtx1);
			CanonicaliseEdge (crossVtx0, crossVtx1);
		}
		else
		{
			// NOTE Jan 26, 2009: <pvl> by now, one of them is a cross vtx, the other isn't
			if ( (*vtx0)->m_type == USER_DATA_CROSS_VTX )
				CanonicaliseEdge (reinterpret_cast <CrossVtxHeritage**> (vtx0), vtx1);
			else
				CanonicaliseEdge (reinterpret_cast <CrossVtxHeritage**> (vtx1), vtx0);
		}
	}

	void CanonicaliseEdge (CrossVtxHeritage ** crossVtx0, CrossVtxHeritage ** crossVtx1)
	{
		for (int i=0; i<2; ++i)
			for (int j=0; j<2; ++j)
			{
				if ( *(*crossVtx0)->m_vtx[2*i] == *(*crossVtx1)->m_vtx[2*j]
						&& *(*crossVtx0)->m_vtx[2*i+1] == *(*crossVtx1)->m_vtx[2*j+1])
				{
					*reinterpret_cast <VtxHeritage**> (crossVtx0) = (*crossVtx0)->m_vtx[2*i];
					*reinterpret_cast <VtxHeritage**> (crossVtx1) = (*crossVtx1)->m_vtx[2*j+1];
					return;
				}
			}

		assert (0);
	}

	void CanonicaliseEdge (CrossVtxHeritage ** crossVtx, VtxHeritage ** nonCrossVtx)
	{
		// NOTE Jan 27, 2009: <pvl> find the non-cross vtx in the cross vtx descriptor
		int i=0;
		for (; i<4; ++i)
			if ( *(*crossVtx)->m_vtx[i] == **nonCrossVtx)
				break;
		assert (i<4);
		// NOTE Jan 27, 2009: <pvl> get the other side of the same edge
		i ^= 0x1;

		// NOTE Jan 27, 2009: <pvl> replace the cross vtx
		*reinterpret_cast <VtxHeritage**> (crossVtx) = (*crossVtx)->m_vtx[i];
	}

	CrossVtxHeritage (Allocator * allocator, VtxHeritage * vtx00, VtxHeritage * vtx01,
				VtxHeritage * vtx10, VtxHeritage * vtx11) :
			VtxHeritage(USER_DATA_CROSS_VTX),
			// NOTE Okt 23, 2008: <pvl> vertices can be passed to the constructor in
			// any order so at this point, the assignment might be wrong but it will
			// have been fixed by sorting by the time this constructor exits
			m_staticVtx0 (vtx00->Clone (allocator)), m_staticVtx1 (vtx01->Clone (allocator)),
			m_obstacleVtx0 (vtx10->Clone (allocator)), m_obstacleVtx1 (vtx11->Clone (allocator))
	{
		CanonicaliseEdge ( & m_staticVtx0, & m_staticVtx1);
		CanonicaliseEdge ( & m_obstacleVtx0, & m_obstacleVtx1);

		for (int i=0; i<4; ++i)
			assert ( m_vtx[i]->m_type != USER_DATA_CROSS_VTX );

		SortVertices ();
	}

	CrossVtxHeritage (Allocator * allocator, const CrossVtxHeritage & rhs) :
			VtxHeritage(USER_DATA_CROSS_VTX),
			m_staticVtx0 (rhs.m_staticVtx0->Clone (allocator)),
			m_staticVtx1 (rhs.m_staticVtx1->Clone (allocator)),
			m_obstacleVtx0 (rhs.m_obstacleVtx0->Clone (allocator)),
			m_obstacleVtx1 (rhs.m_obstacleVtx1->Clone (allocator))
	{
	}

	// NOTE Okt 2, 2008: <pvl> support for sanity check asserts
	unsigned int NumObstacleVertices () const
	{
		int result=0;
		for (int i=0; i<4; ++i)
			if (m_vtx[i]->m_type != USER_DATA_NAV_POLY && m_vtx[i]->m_type != USER_DATA_CROSS_VTX)
				++result;
		return result;
	}

	StaticEdge GetStaticEdge () const
	{
		StaticEdge staticEdge (0,0);
		for (int i=0; i<4; ++i)
			if (m_vtx[i]->m_type == USER_DATA_NAV_POLY)
				if (staticEdge.first == 0)
					staticEdge.first = (NavPolyVtxHeritage * )m_vtx[i];
				else
					staticEdge.second = (NavPolyVtxHeritage * )m_vtx[i];

		assert (staticEdge.first && staticEdge.second);

		// NOTE Okt 6, 2008: <pvl> the convention is to have the lower-indexed
		// vertex go first so that each (static) edge is unambiguously identified
		// by its vertices
		if (staticEdge.first->m_vtxIndex > staticEdge.second->m_vtxIndex)
			std::swap (staticEdge.first, staticEdge.second);
		return staticEdge;
	}

	bool HasStaticEdge () const
	{
		// NOTE Jan 28, 2009: <pvl> if it does then (thanks to sorting) it has
		// to be the first one
		return m_vtx[0]->m_type == USER_DATA_NAV_POLY;
	}

	static void * operator new (size_t size, Allocator * alloc)
	{
		return alloc->Allocate (size);
	}
	static void operator delete (void * , Allocator * )
	{
		assert (0);
	}

	virtual CrossVtxHeritage * Clone (Allocator * allocator) const
	{
		return new (allocator) CrossVtxHeritage (allocator, *this);
	}
	virtual void DebugDraw (const Vec3 & pos) const
	{
		const Vec3 spacing (0.0f, 0.0f, -0.35f);
		for (int i=0; i<4; ++i)
			m_vtx[i]->DebugDraw (pos + static_cast<float>(i) * spacing);
	}
};


// ---

#include "PolygonSetOps.h"

void PostprocessCrossVtxHeritage (Polygon * polygon, Allocator * alloc)
{
	for (int c=0, numContours=polygon->NumContours(); c < numContours; ++c)
	{
		Polygon::Contour * contour = polygon->GetContour (c);
		Polygon::Contour::VertexLoopIt vtxIt = contour->CreateVertexLoopIt ();
		for ( ; vtxIt; ++vtxIt)
		{
			VtxUserData * userData = vtxIt->GetUserData ();
			if (userData->GetType () == VtxUserData::CROSS)
			{
				CrossVtxUserData * crossVtx = static_cast <CrossVtxUserData*> (userData);
				CrossVtxHeritage * crossVtxHrtg = new (alloc) CrossVtxHeritage (
						alloc,
						reinterpret_cast <VtxHeritage*> ( (reinterpret_cast <OrigVtxUserData*> (crossVtx->GetData (0)))->GetData () ),
						reinterpret_cast <VtxHeritage*> ( (reinterpret_cast <OrigVtxUserData*> (crossVtx->GetData (1)))->GetData () ),
						reinterpret_cast <VtxHeritage*> ( (reinterpret_cast <OrigVtxUserData*> (crossVtx->GetData (2)))->GetData () ),
						reinterpret_cast <VtxHeritage*> ( (reinterpret_cast <OrigVtxUserData*> (crossVtx->GetData (3)))->GetData () )
				);
				vtxIt->ResetUserData (new (alloc) OrigVtxUserData (crossVtxHrtg));
			}
		}
	}
}

// NOTE Sep 24, 2008: <pvl> can't use Contour::Clone() since that copies
// all vertex data including those generated by preprocessing
Polygon * Convert (unsigned operandNum, const Polygon * shape, Allocator * alloc, bool tagVtxs)
{
	Polygon * newPoly = new (alloc) Polygon (alloc);

	for (int c=0, numContours=shape->NumContours(); c < numContours; ++c)
	{
		Polygon::Contour * newContour = newPoly->NewContour ();
		newPoly->AddContour (newContour);

		Polygon::Contour * origContour = shape->GetContour (c);
		Polygon::Contour::VertexLoopIt vtxIt = origContour->CreateVertexLoopIt ();
		for (unsigned vtxNum=1000*c; vtxIt; ++vtxIt, ++vtxNum)
		{
			Polygon::Contour::VertexIt newVtxIt =
					newContour->AddVertex (Polygon::Vertex (operandNum, vtxIt->Pos()));

			if (tagVtxs)
			{
				// NOTE Mar 27, 2009: <pvl> if there's user data already just carry
				// it over.  If not tag this polygon as an obstacle.  (This is just
				// an opportunistic piece of code, don't look for anything profound
				// behind this - it just happens to suit what the caller expects ATM.)
				VtxUserData * userData = vtxIt->GetUserData ();
				if (userData)
				{
					// NOTE Mar 25, 2009: <pvl> if CROSS PostprocessCrossVtxHeritage()
					// probably hasn't been called on this polygon
					assert (userData->GetType () == VtxUserData::ORIG);

					OrigVtxUserData * origVtx = static_cast <OrigVtxUserData*> (userData);
					VtxHeritage * vtxHrtg = reinterpret_cast<VtxHeritage*> (origVtx->GetData());
					newVtxIt->SetUserData (new (alloc) OrigVtxUserData (vtxHrtg->Clone (alloc)));
				}
				else
				{
					SShapeVtxHeritage * vtxHrtg = new (alloc) SShapeVtxHeritage ( & shape, vtxNum);
					newVtxIt->SetUserData (new (alloc) OrigVtxUserData (vtxHrtg));
				}
			}
		}
	}

	return newPoly;
}

Polygon * Convert (unsigned operandNum, const SShape & shape, Allocator * alloc, bool tagVtxs)
{
	Polygon * newPoly = new (alloc) Polygon (alloc);

	Polygon::Contour * contour = newPoly->NewContour ();
	newPoly->AddContour (contour);

	ListPositions::const_iterator it = shape.shape.begin();
	const ListPositions::const_iterator end = shape.shape.end();
	for (int vtxNum=0; it != end; ++it, ++vtxNum)
	{
		Polygon::Contour::VertexIt newVtxIt = contour->AddVertex (Polygon::Vertex (operandNum, *it));

		if (tagVtxs)
		{
			SShapeVtxHeritage * userData = new (alloc) SShapeVtxHeritage ( & shape, vtxNum);
			newVtxIt->SetUserData (new (alloc) OrigVtxUserData (userData));
		}
	}

	return newPoly;
}

#include "../../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/PolygonCont.h"
Polygon * Convert (unsigned operandNum, const LayeredNavMesh::PolygonCont::Polygon & srcPoly, Allocator * alloc, bool tagVtxs)
{
	Polygon * newPoly = new (alloc) Polygon (alloc);

	Polygon::Contour * contour = newPoly->NewContour ();
	newPoly->AddContour (contour);

	LayeredNavMesh::PolygonCont::Polygon::vertex_iterator it = srcPoly.begin();
	LayeredNavMesh::PolygonCont::Polygon::vertex_index_iterator indexIt = srcPoly.index_begin();
	LayeredNavMesh::PolygonCont::Polygon::vertex_iterator end = srcPoly.end();
	for ( ; it != end; ++it, ++indexIt)
	{
		Polygon::Contour::VertexIt newVtxIt = contour->AddVertex (Polygon::Vertex (operandNum, *it));

		if (tagVtxs)
		{
			NavPolyVtxHeritage * userData = new (alloc) NavPolyVtxHeritage (*indexIt);
			newVtxIt->SetUserData (new (alloc) OrigVtxUserData (userData));
		}
	}

	return newPoly;
}

Polygon * Convert (unsigned operandNum, const std::vector<Vec3> & shape, Allocator * alloc, bool tagVtxs)
{
	Polygon * newPoly = new (alloc) Polygon (alloc);

	Polygon::Contour * contour = newPoly->NewContour ();
	newPoly->AddContour (contour);

	std::vector<Vec3>::const_iterator it = shape.begin();
	const std::vector<Vec3>::const_iterator end = shape.end();
	for (int vtxNum=0; it != end; ++it, ++vtxNum)
	{
		Polygon::Contour::VertexIt newVtxIt = contour->AddVertex (Polygon::Vertex (operandNum, *it));

		if (tagVtxs)
		{
			SShapeVtxHeritage * userData = new (alloc) SShapeVtxHeritage ( & shape, vtxNum);
			newVtxIt->SetUserData (new (alloc) OrigVtxUserData (userData));
		}
	}

	return newPoly;
}

// NOTE Mar 31, 2009: <pvl> VtxHeritage data (if any) can't be freed as part of
// Polygon destruction since they're just void* as far as Polygon is concerned.
// We need to get rid of them here in a separate step.
void FreeHeritageTags (Polygon * polygon, Allocator * allocator)
{
	unsigned numContours = polygon->NumContours ();
	for (unsigned ci=0; ci < numContours; ++ci)
	{
		const Polygon::Contour * const c = polygon->GetContour(ci);
		Polygon::Contour::VertexLoopIt vtxIt = c->CreateVertexLoopIt();
		for ( ; vtxIt; ++vtxIt)
		{
			VtxUserData * userData = vtxIt->GetUserData ();
			if (userData == 0) continue;
			//NOTE Mar 25, 2009: <pvl> if CROSS, have you run PostprocessCrossVtxHeritage()?
			assert (userData->GetType () != VtxUserData::CROSS);
			OrigVtxUserData * origUserData = reinterpret_cast<OrigVtxUserData*> (userData);
			VtxHeritage * vtxHrtg = reinterpret_cast<VtxHeritage*> (origUserData->GetData ());
			assert (vtxHrtg);

			switch (vtxHrtg->m_type)
			{
			case USER_DATA_NAV_POLY:
			{
				StaticVtxData * staticData = reinterpret_cast<StaticVtxData*> (vtxHrtg);
				staticData->~StaticVtxData ();
				allocator->Deallocate (staticData);
				break;
			}
			case USER_DATA_OBSTACLE_SSHAPE:
			{
				SShapeVtxData * obstacleData = reinterpret_cast<SShapeVtxData*> (vtxHrtg);
				obstacleData->~SShapeVtxData ();
				allocator->Deallocate (obstacleData);
				break;
			}
			case USER_DATA_CROSS_VTX:
			{
				CrossVtxHeritage * xVtxHeritage = reinterpret_cast<CrossVtxHeritage*> (vtxHrtg);
				xVtxHeritage->~CrossVtxHeritage ();
				allocator->Deallocate (xVtxHeritage);
				break;
			}
			default:
				assert (0 && "unknown type of vertex user data for dynamic vertex");
			}
		}
	}
}


inline unsigned GetAttribsSize (const VtxHeritage * userData)
{
	switch (userData->m_type)
	{
	case USER_DATA_NAV_POLY:
		return sizeof (StaticVtxData);
		break;
	case USER_DATA_OBSTACLE_SSHAPE:
		return sizeof (SShapeVtxData);
		break;
	case USER_DATA_CROSS_VTX:
		{
			CrossVtxHeritage * xVtxHeritage = (CrossVtxHeritage * )userData;
			if (xVtxHeritage->HasStaticEdge ())
				return sizeof (CrossVtxData);
			else
				return sizeof (ObstacleCrossVtxData);
		}
		break;
	default:
		assert (0 && "unknown type of vertex user data for dynamic vertex");
		return 0;
		break;
	}
}

template <typename T>
T ConvertTo (const VtxHeritage * );

template <>
StaticVtxData ConvertTo (const VtxHeritage * userData)
{
	assert (userData->m_type == USER_DATA_NAV_POLY);
	return StaticVtxData (((NavPolyVtxHeritage * )userData)->m_vtxIndex);
}

template <>
SShapeVtxData ConvertTo (const VtxHeritage * userData)
{
	assert (userData->m_type == USER_DATA_OBSTACLE_SSHAPE);
	SShapeVtxHeritage * sshapeVtxHeritage = (SShapeVtxHeritage * )userData;
	return SShapeVtxData (sshapeVtxHeritage->m_id, sshapeVtxHeritage->m_vtxNumber);
}

template <>
CrossVtxData ConvertTo (const VtxHeritage * userData)
{
	assert (userData->m_type == USER_DATA_CROSS_VTX);
	CrossVtxHeritage * xVtxHeritage = (CrossVtxHeritage * )userData;
	// FIXME Okt 22, 2008: <pvl> fix names in the union in CrossVtxHeritage and
	// use them here for better clarity!
	return CrossVtxData (
				ConvertTo <StaticVtxData> (xVtxHeritage->m_staticVtx0),
				ConvertTo <StaticVtxData> (xVtxHeritage->m_staticVtx1),
				ConvertTo <SShapeVtxData> (xVtxHeritage->m_obstacleVtx0),
				ConvertTo <SShapeVtxData> (xVtxHeritage->m_obstacleVtx1)
	);
}

template <>
ObstacleCrossVtxData ConvertTo (const VtxHeritage * userData)
{
	assert (userData->m_type == USER_DATA_CROSS_VTX);
	CrossVtxHeritage * xVtxHeritage = (CrossVtxHeritage * )userData;
	// FIXME Okt 22, 2008: <pvl> fix names in the union in CrossVtxHeritage and
	// use them here for better clarity!
	return ObstacleCrossVtxData (
				ConvertTo <SShapeVtxData> (xVtxHeritage->m_staticVtx0),
				ConvertTo <SShapeVtxData> (xVtxHeritage->m_staticVtx1),
				ConvertTo <SShapeVtxData> (xVtxHeritage->m_obstacleVtx0),
				ConvertTo <SShapeVtxData> (xVtxHeritage->m_obstacleVtx1)
	);
}

//VtxAttribs PolygonRefCounted::GetVtxAttribs () const
VtxAttribs GetVtxAttribs (const Polygon * polygon)
{
	const unsigned numContours = polygon->NumContours();

	// NOTE Okt 17, 2008: <pvl> get the count of vertices
	unsigned numVertices = 0;
	for (unsigned ci=0; ci < numContours; ++ci)
		numVertices += polygon->GetContour(ci)->NumVertices();

	// NOTE Okt 17, 2008: <pvl> next, see how much memory attributes will need
	unsigned attribsSize = 0;
	for (unsigned ci=0; ci < numContours; ++ci)
	{
		const Polygon::Contour * const c = polygon->GetContour(ci);
		Polygon::Contour::VertexLoopIt vtxIt = c->CreateVertexLoopIt();
		for ( ; vtxIt; ++vtxIt)
		{
			VtxUserData * userData = vtxIt->GetUserData ();

			// TODO Apr 3, 2009: <pvl> if a vertex has userData==0 chances are
			// that the whole polygon is untagged - exit early?  Report error
			// since someone is trying to extract attribs without first putting
			// some in?
			if (userData == 0) continue;

			//NOTE Mar 25, 2009: <pvl> if CROSS, have you run PostprocessCrossVtxHeritage()?
			assert (userData->GetType () != VtxUserData::CROSS);
			OrigVtxUserData * origUserData = reinterpret_cast<OrigVtxUserData*> (userData);
			VtxHeritage * vtxHrtg = reinterpret_cast<VtxHeritage*> (origUserData->GetData ());
			assert (vtxHrtg);

			attribsSize += GetAttribsSize (vtxHrtg);
		}
	}

	// NOTE Okt 17, 2008: <pvl> now, loop over vertices once more (should be fast
	// enough thanks to pooled memory) and collect the actual data

	VtxAttribs attribs (numVertices, attribsSize);

	for (unsigned ci=0; ci < numContours; ++ci)
	{
		const Polygon::Contour * const c = polygon->GetContour(ci);
		Polygon::Contour::VertexLoopIt vtxIt = c->CreateVertexLoopIt();
		for ( ; vtxIt; ++vtxIt)
		{
			VtxUserData * userData = vtxIt->GetUserData ();

			if (userData == 0) continue;

			//NOTE Mar 25, 2009: <pvl> if CROSS, have you run PostprocessCrossVtxHeritage()?
			assert (userData->GetType () != VtxUserData::CROSS);
			OrigVtxUserData * origUserData = reinterpret_cast<OrigVtxUserData*> (userData);
			VtxHeritage * vtxHrtg = reinterpret_cast<VtxHeritage*> (origUserData->GetData ());
			assert (vtxHrtg);

			switch (vtxHrtg->m_type)
			{
			case USER_DATA_NAV_POLY:
				attribs.PushBack (ConvertTo <StaticVtxData> (vtxHrtg));
				break;
			case USER_DATA_OBSTACLE_SSHAPE:
				attribs.PushBack (ConvertTo <SShapeVtxData> (vtxHrtg));
				break;
			case USER_DATA_CROSS_VTX:
			{
				CrossVtxHeritage * xVtxHeritage = (CrossVtxHeritage * )vtxHrtg;
				if (xVtxHeritage->HasStaticEdge ())
					attribs.PushBack (ConvertTo <CrossVtxData> (vtxHrtg));
				else
					attribs.PushBack (ConvertTo <ObstacleCrossVtxData> (vtxHrtg));
				break;
			}
			default:
				assert (0 && "unknown type of vertex user data for dynamic vertex");
			}
		}
	}
	return attribs;
}

std::vector <std::vector <float> > GetVertCoordStreams (const Polygon * polygon)
{
	const unsigned numContours = polygon->NumContours();

	std::vector <std::vector <float> > streams (numContours);

	for (unsigned ci=0; ci < numContours; ++ci)
	{
		streams[ci].reserve (polygon->GetContour(ci)->NumVertices());

		const Polygon::Contour * const c = polygon->GetContour(ci);
		Polygon::Contour::VertexLoopIt vtxIt = c->CreateVertexLoopIt();
		for ( ; vtxIt; ++vtxIt)
		{
			const Vec3 & vtxPos = vtxIt->Pos();
			streams[ci].push_back (vtxPos.x);

			streams[ci].push_back (vtxPos.y);
			streams[ci].push_back (vtxPos.z);
		}
	}
	return streams;
}

std::vector < ::Vec3> GetVtxPositions (const Polygon * polygon)
{
	const unsigned numContours = polygon->NumContours();

	unsigned numVertices = 0;
	for (unsigned ci=0; ci < numContours; ++ci)
		numVertices += polygon->GetContour(ci)->NumVertices();

	std::vector < ::Vec3> vtxs;
	vtxs.reserve (numVertices);

	for (unsigned ci=0; ci < numContours; ++ci)
	{
		const Polygon::Contour * const c = polygon->GetContour(ci);
		Polygon::Contour::VertexLoopIt vtxIt = c->CreateVertexLoopIt();
		for ( ; vtxIt; ++vtxIt)
		{
			vtxs.push_back (vtxIt->Pos());
		}
	}
	return vtxs;
}

// ---

class Vec3LexLess {
public:
	bool operator () (const Vec3 & lhs, const Vec3 & rhs) const
	{
		if (lhs.x < rhs.x) return true;
		if (lhs.x > rhs.x) return false;
		if (lhs.y < rhs.y) return true;
		return false;
	}
};


#include "DynamicObstacles.h"

// NOTE Jan 21, 2009: <pvl> this is basically just a thinly veiled std::vector
// with an AABB attached.  It's implicitly and inexpensively convertible
// to std::vector<Vec3> since its polygon vertices are currently supplied
// directly by ConvexHull2D() to avoid excessive allocations and copying and
// std::vector<Vec3> is what ConvexHull2D() expects.  This also explains where
// ObstaclePolygon's data come from.
// ObstaclePolygon is also presented to PolygonSetOps as a std::vector<Vec3>
// to keep PolygonSetOps away from depending on an application-specific type.
class ObstaclePolygon {
	std::vector<Vec3> m_polygon;

	// NOTE Mar 3, 2009: <pvl> the name of the entity this obstacle corresponds to
	string m_name;

	bool m_aabbUptodate;
	mutable AABB m_aabb;

	// NOTE Jan 20, 2009: <pvl> const since this only modifies a mutable member
	void ComputeAABB () const;
public:
	ObstaclePolygon (const string & name = string (""));

	const AABB & GetAABB () const;
	Vec3 GetCentroid () const;
	float GetArea () const;
	void Flush ();
	bool Empty () const;
	operator const std::vector<Vec3> & () const;
	operator std::vector<Vec3> & ();

	void DebugDraw () const;
	void DumpIntoFile (FILE * ) const;
};

ObstaclePolygon::ObstaclePolygon (const string & name) :
		m_name (name), m_aabbUptodate (false), m_aabb (AABB::RESET)
{
}

void ObstaclePolygon::ComputeAABB () const
{
	m_aabb = AABB ( & m_polygon[0], m_polygon.size ());
}

const AABB & ObstaclePolygon::GetAABB () const
{
	if ( ! m_aabbUptodate)
		ComputeAABB ();

	return m_aabb;
}

// FIXME Feb 20, 2009: <pvl> make this generic and put it somewhere where everybody
// can find it
Vec3 ObstaclePolygon::GetCentroid () const
{
	::Vec3 centroid (0.0, 0.0, 0.0);

	const unsigned numVertices = m_polygon.size ();
	std::vector <Vec3>::const_iterator it = m_polygon.begin ();
	const std::vector <Vec3>::const_iterator end = m_polygon.end ();
	// NOTE Okt 28, 2008: <pvl> less likely to overflow but involves multiple divisions
	for ( ; it != end; ++it)
		centroid += *it / static_cast <float> (numVertices);

	return centroid;
}

float ObstaclePolygon::GetArea () const
{
	float area = 0.0f;
	const Vec3 & off = m_polygon[0];
	for (unsigned i=0, numVert=m_polygon.size (); i < numVert; ++i)
	{
		unsigned next_i = (i+1) % numVert;
		const Vec3 & vert = m_polygon[i] - off;
		const Vec3 & nextVert = m_polygon[next_i] - off;
		area += vert.x * nextVert.y - nextVert.x * vert.y;
	}

	assert (area >= 0.0f);
	return 0.5f * area;
}

void ObstaclePolygon::Flush ()
{
	m_polygon.resize (0);
}

bool ObstaclePolygon::Empty () const
{
	return m_polygon.empty ();
}

ObstaclePolygon::operator const std::vector<Vec3> & () const
{
	return m_polygon;
}

ObstaclePolygon::operator std::vector<Vec3> & ()
{
	return m_polygon;
}

void ObstaclePolygon::DebugDraw () const
{
	const ColorB col (200,200,100);
	for (unsigned vtx=0, numVtxs = m_polygon.size(); vtx < numVtxs; ++vtx)
	{
		unsigned nextVtx = vtx+1 == numVtxs ? 0 : vtx+1;
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine (m_polygon[vtx], col, m_polygon[nextVtx], col, 2.0f);
	}
}

void ObstaclePolygon::DumpIntoFile (FILE * f) const
{
	std::vector <Vec3>::const_iterator it = m_polygon.begin ();
	const std::vector <Vec3>::const_iterator end = m_polygon.end ();
	for ( ; it != end; ++it)
	{
		const Vec3 & vtxPos = *it;
		fprintf (f, "%16.8f %16.8f\n", vtxPos.x, vtxPos.y);
	}
}



class SubTessellator {

	// NOTE Okt 13, 2008: <pvl> the overlay nav mesh that gets built by SubTesselator.
	// We don't own this, someone is supposed to pick this up once it's built.
	DynamicOverlay * m_overlay;

	unsigned int GetVtxIndex (const Vec3 & pos) const;

	void BuildAdjacency ();
	void Tessellate (const Polygon * poly);
public:
	// NOTE Okt 23, 2008: <pvl> tessellate 'poly' and place generated data in 'overlay'
	SubTessellator (const Polygon * poly, DynamicOverlay * overlay);

	DynamicOverlay * GetOverlay () const { return m_overlay; }
};

SubTessellator::SubTessellator (const Polygon * poly, DynamicOverlay * overlay) :
		m_overlay (overlay)
{
	GetVtxPositions (poly).swap (m_overlay->m_vertices);
	GetVtxAttribs (poly).Swap (m_overlay->m_vtxAttribs);

	unsigned numVertices = m_overlay->m_vertices.size ();
	if (numVertices < (1<<8))
		m_overlay->m_indices.SetStride (1);
	else if (numVertices < (1<<16))
		m_overlay->m_indices.SetStride (2);
	else if (numVertices < (1<<24))
		m_overlay->m_indices.SetStride (3);
	else
		m_overlay->m_indices.SetStride (4);

	// NOTE Okt 24, 2008: <pvl> if we don't have enough vertices for even a single
	// triangle there's not much point in tessellation.  This shouldn't happen
	// under normal operation but can be useful while testing fringe case handling
	// with degenerated data sets.
	if (numVertices < 3) return;

	// NOTE Okt 21, 2008: <pvl> if the tesselation turns out to be a triangulation
	// really, we'll need that many indices.  We'll need less for a convex
	// polygonal subdivision.
	// FIXME Jan 28, 2009: <pvl> this is not always true (with holes, for instance)
	//m_overlay->m_indices.reserve (3 * (numVertices - 2));
	//
	// UPDATE Mar 3, 2009: <pvl> let's just reserve twice as much, we ShrinkToFit()
	// in the end anyway
	m_overlay->m_indices.reserve (3 * (numVertices - 2) * 2);

	Tessellate (poly);
	BuildAdjacency ();
} 

unsigned int SubTessellator::GetVtxIndex (const Vec3 & pos) const
{
	// FIXME Okt 10, 2008: <pvl> tmp linear search!
	for (uint32 i=0; i < m_overlay->m_vertices.size (); ++i)
	{
		// FIXxME Okt 24, 2008: <pvl> I'm afraid we'll need to check here if
		// m_overlay->m_vertices[i] is a static vertex and if so, return the static
		// vertex index instead.  That way the identity of the vertex in the system
		// is preserved and doesn't break things like extraction of the common edge
		// shared by neighbouring polygons.
		// UPDATE Nov 3, 2008: <pvl> this is not a good place to deal with that problem.
		// It's been solved by adding optional VtxAttribs* to PolygonCont::Polygon
		// that can be used to translate static vertices in dynamic overlays to their
		// canonical static indices.
		if (pos == m_overlay->m_vertices[i])
			return i;
	}
	assert (0 && "vertex not found in SubTesselator");
	// NOTE Nov 18, 2008: <pvl> shouldn't ever happen, just to placate compiler
	return DynamicOverlay::INVALID_VTX_INDEX;
}

#include "../../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/Tess/Tess.h"
void SubTessellator::Tessellate (const Polygon * poly)
{
	Tess tess(TESS_WINDING_ODD, TESS_PROCESS_CONVEX_POLYGONS | TESS_PROCESS_NO_INTERSECTIONS, 512);

	std::vector <std::vector <float> > vertCoordStreams;
	GetVertCoordStreams(poly).swap (vertCoordStreams);

	std::vector <std::vector <float> >::const_iterator streamIt = vertCoordStreams.begin ();
	std::vector <std::vector <float> >::const_iterator streamEnd = vertCoordStreams.end ();
	for ( ; streamIt != streamEnd; ++streamIt)
		tess.AddContour ( & (*streamIt)[0], streamIt->size() / 3);

	if (!tess.Process())
	{
		printf("process failed!\n");
	}

	// Collect polys
	TessMesh* mesh = tess.GetMesh();
	TessFace* fHead = mesh->GetFHead();

	for (TessFace* f = fHead->next; f != fHead; f = f->next)
	{
		if (!f->inside)
			continue;

		m_overlay->m_polygons.push_back (DynamicOverlay::Polygon());
		DynamicOverlay::Polygon * newPoly = & m_overlay->m_polygons.back ();
		newPoly->m_startIndex = m_overlay->m_indices.size ();

		TessHalfEdge* e = f->anEdge;
		do
		{
			unsigned int vtxIndex = GetVtxIndex (Vec3 (e->Org()->x, e->Org()->y, e->Org()->z));
			// NOTE Nov 18, 2008: <pvl> shouldn't ever happen but we need to handle it anyway
			if (vtxIndex != DynamicOverlay::INVALID_VTX_INDEX)
				m_overlay->m_indices.push_back (vtxIndex);
			e = e->LNext();
		}
		while (e != f->anEdge);

		newPoly->m_numIndices = m_overlay->m_indices.size () - newPoly->m_startIndex;
	}

	// TODO Okt 21, 2008: <pvl> would it be worth it to test first how much memory
	// is actually wasted?
	m_overlay->m_indices.ShrinkToFit ();

	m_overlay->m_neighbours.insert (
			m_overlay->m_neighbours.begin (), m_overlay->m_indices.size (),
			DynamicOverlay::INVALID_NEIGHBOR
	);
}

#include "../../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/env.h"	// for Stopwatch
void SubTessellator::BuildAdjacency ()
{
	struct Edge
	{
		unsigned short vert[2];
		unsigned short polyEdge[2];
		unsigned short poly[2];
	};

	// TODO Okt 10, 2008: <pvl> very similar code also based on
	// http://www.terathon.com/code/edges.php
	// is in NavPolygonGenerator::BuildAdjacency() already - can we make it
	// generic and share it?

	unsigned maxEdgeCount = 0;

	std::vector<DynamicOverlay::Polygon>::const_iterator it = m_overlay->m_polygons.begin ();
	std::vector<DynamicOverlay::Polygon>::const_iterator end = m_overlay->m_polygons.end ();
	for ( ; it != end; ++it)
		maxEdgeCount += it->m_numIndices;

	const unsigned vertCount = m_overlay->m_vertices.size();
	unsigned short * const firstEdge = new unsigned short[vertCount + maxEdgeCount];
	unsigned short * const nextEdge = firstEdge + vertCount;

	Edge * const edges = new Edge[maxEdgeCount];

	for (unsigned a = 0; a < vertCount; a++) firstEdge[a] = 0xffff;

	// First pass over all triangles. This finds all the edges satisfying the
	// condition that the first vertex index is less than the second vertex index
	// when the direction from the first vertex to the second vertex represents
	// a counterclockwise winding around the triangle to which the edge belongs.
	// For each edge found, the edge index is stored in a linked list of edges
	// belonging to the lower-numbered vertex index i. This allows us to quickly
	// find an edge in the second pass whose higher-numbered vertex index is i.

	unsigned edgeCount = 0;

	for (unsigned i = 0, ni = m_overlay->m_polygons.size(); i < ni; ++i)
	{
		DynamicOverlay::Polygon * poly = & m_overlay->m_polygons[i];
		const unsigned numPolyVertices = poly->m_numIndices;
		unsigned i1 = m_overlay->m_indices [poly->m_startIndex];
		for (unsigned b = 0; b < numPolyVertices; b++)
		{
			unsigned i2 = m_overlay->m_indices [poly->m_startIndex + (b+1)%numPolyVertices];
			if (i1 < i2)
			{
				Edge *edge = &edges[edgeCount];

				edge->vert[0] = (unsigned short)i1;
				edge->vert[1] = (unsigned short)i2;
				edge->poly[0] = (unsigned short)i;
				edge->polyEdge[0] = (unsigned short)b;
				edge->poly[1] = (unsigned short)i;
				edge->polyEdge[1] = 0;

				unsigned edgeIndex = firstEdge[i1];
				if (edgeIndex == 0xffff)
				{
					firstEdge[i1] = edgeCount;
				}
				else
				{
					for (;;)
					{
						long index = nextEdge[edgeIndex];
						if (index == 0xffff)
						{
							nextEdge[edgeIndex] = edgeCount;
							break;
						}
						edgeIndex = index;
					}
				}

				nextEdge[edgeCount] = 0xffff;
				edgeCount++;
			}

			i1 = i2;
		}
	}

	// Second pass over all triangles. This finds all the edges satisfying the
	// condition that the first vertex index is greater than the second vertex index
	// when the direction from the first vertex to the second vertex represents
	// a counterclockwise winding around the triangle to which the edge belongs.
	// For each of these edges, the same edge should have already been found in
	// the first pass for a different triangle. So we search the list of edges
	// for the higher-numbered vertex index for the matching edge and fill in the
	// second triangle index. The maximum number of comparisons in this search for
	// any vertex is the number of edges having that vertex as an endpoint.

	for (unsigned i = 0, ni = m_overlay->m_polygons.size(); i < ni; ++i)
	{
		DynamicOverlay::Polygon * poly = & m_overlay->m_polygons[i];
		const unsigned numPolyVertices = poly->m_numIndices;
		long i1 = m_overlay->m_indices [poly->m_startIndex];
		for (unsigned b = 0; b < numPolyVertices; b++)
		{
			long i2 = m_overlay->m_indices [poly->m_startIndex + (b+1)%numPolyVertices];;
			if (i1 > i2)
			{
				for (unsigned edgeIndex = firstEdge[i2]; edgeIndex != 0xffff; edgeIndex = nextEdge[edgeIndex])
				{
					Edge* edge = &edges[edgeIndex];
					if (edge->vert[1] == i1 && edge->poly[0] == edge->poly[1])
					{
						edge->poly[1] = (unsigned short)i;
						edge->polyEdge[1] = (unsigned short)b;
						break;
					}
				}
			}
			i1 = i2;
		}
	}

	// TODO Okt 10, 2008: <pvl> would it be better to have already created new
	// graphs nodes corresponding to the tessellated polys and connect them here
	// directly?
	// Store adjacency
	for (unsigned i = 0; i < edgeCount; ++i)
	{
		const Edge& e = edges[i];
		if (e.poly[0] != e.poly[1])
		{
			m_overlay->m_neighbours [ m_overlay->m_polygons[e.poly[0]].m_startIndex + e.polyEdge[0] ] = e.poly[1] + m_overlay->GetBaseIndex ();
			m_overlay->m_neighbours [ m_overlay->m_polygons[e.poly[1]].m_startIndex + e.polyEdge[1] ] = e.poly[0] + m_overlay->GetBaseIndex ();
		}
	}

	delete [] firstEdge;
	delete [] edges;
}

class OverlaidNavPoly;

class DynamicObstacle {
	// NOTE Sep 23, 2008: <pvl> top-down polygonal representation (think convex hull)
	const ObstaclePolygon * m_polygon;
	const float m_area;

	// NOTE Sep 23, 2008: <pvl> list of nav polys this obstacle intersects
	typedef std::vector <OverlaidNavPoly*> Polygons;
	Polygons m_polys;

public:
	DynamicObstacle (const ObstaclePolygon * poly);

	const ObstaclePolygon * GetPoly () const { return m_polygon; }

	void AddIntersectedPoly (OverlaidNavPoly * poly);

	// NOTE Okt 30, 2008: <pvl> only detaches the obstacle from the polygons
	// overlapped by it, doesn't actually clear 'm_polys'.  That's useful for
	// checking subsequently (after Detach() finishes) which of the polygons
	// aren't overlapped by an obstacle anymore and need to have their overlays
	// removed.
	void Detach () const;

	// NOTE Okt 30, 2008: <pvl> iteration over polygons intersected by this obstacle
	typedef Polygons::const_iterator PolygonIter;
	PolygonIter PolysBegin () const;
	PolygonIter PolysEnd () const;

	static bool PointerCompare (const DynamicObstacle * lhs, const DynamicObstacle * rhs);
};

class OverlaidNavPoly {
	PolygonCont * m_polyCont;
	unsigned int m_polyIndex;

	// NOTE Sep 23, 2008: <pvl> list of DynamicObstacles that intersect the nav
	// mesh polygon represented by this instance.  When this becomes empty this
	// whole instance should be destroyed (physically or morally, as in deactivated
	// and cached for the next use) since the whole raison d'etre of OverlaidNavPoly
	// is the fact that the polygon is intersected by an obstacle.
	typedef std::vector <const DynamicObstacle*> DynamicObstacles;
	DynamicObstacles m_obstacles;

	// FIXME Apr 1, 2009: <pvl> made public for easier prototyping - fix later!
public:
	// NOTE Mar 31, 2009: <pvl> experimental - data related to "obstacle union first"
	// approach to producing a dynamic overlay.  If this pans out get rid of the
	// struct and make its members directly members of OverlaidNavPoly.
	struct UnionFirst {
		Polygon * m_obstaclesSubtracted;
		typedef std::multimap < std::pair<unsigned,unsigned>, Polygon::Contour::VertexIt > StaticToDynamicEdgeMap;
		StaticToDynamicEdgeMap m_edgeMap;

		UnionFirst () : m_obstaclesSubtracted (0) { }
		void MapStaticEdgesToDynamic ();
		bool EnsureNoOverlappingVertices () const;
	private:
		bool IsPieceOfStatic (Polygon::Contour::VertexIt prev, Polygon::Contour::VertexIt next);
		std::pair <unsigned,unsigned>
		GetOriginalStaticEdge (Polygon::Contour::VertexIt prev, Polygon::Contour::VertexIt next);
	} m_unionFirst;
private:

	// ATTN Nov 4, 2008: <pvl> DynamicOverlay instance is created by this class
	// but deleting it is none of its business.  The reason is that while processing
	// obstacles, even if this polygon gets an updated overlay the original one
	// is still needed to finish the process of baking obstacles into nav mesh.
	// It's the responsibility of an upper layer to cache the original overlay
	// as long as they need and then to dispose of it properly.
	DynamicOverlay * m_subdivision;

	void SubdivideNonIteratively ();
	void SubdivideNonIteratively_ObstacleUnionFirst ();
	void ClampDynamicVtxHeights ();
	void AddOverlay ();
	void RemoveOverlay ();

	void DumpPolyAndObstacles () const;
public:
	OverlaidNavPoly (PolygonCont * , unsigned int polyIndex);
	~OverlaidNavPoly ();

	unsigned PolyIndex () const { return m_polyIndex; }

	void AddObstacle (const DynamicObstacle * );
	void RemoveObstacle (const DynamicObstacle * );
	unsigned NumObstacles () const;

	void Subdivide ();

	DynamicOverlay * GetOverlay () const;
};


DynamicObstacle::DynamicObstacle (const ObstaclePolygon * poly) :
		m_polygon (poly), m_area (poly->GetArea ())
{
	// NOTE Sep 23, 2008: <pvl> no good reason for this number in particular,
	// in these early implementation stages I just feel there shouldn't be many
	// obstacles intersecting more than 16 nav polygons.
	m_polys.reserve (16);
}

void DynamicObstacle::AddIntersectedPoly (OverlaidNavPoly * poly)
{
	m_polys.push_back (poly);
}

void DynamicObstacle::Detach () const
{
	Polygons::const_iterator polyIt = m_polys.begin ();
	Polygons::const_iterator polyEnd = m_polys.end ();
	for ( ; polyIt != polyEnd; ++polyIt)
		(*polyIt)->RemoveObstacle (this);
}

DynamicObstacle::Polygons::const_iterator DynamicObstacle::PolysBegin () const
{
	return m_polys.begin ();
}

DynamicObstacle::Polygons::const_iterator DynamicObstacle::PolysEnd () const
{
	return m_polys.end ();
}

bool DynamicObstacle::PointerCompare (const DynamicObstacle * lhs, const DynamicObstacle * rhs)
{
	// NOTE Jun 2, 2009: <pvl> bigger obstacles are considered *less* than
	// smaller ones so that big obstacles are sorted to the beginning of an obstacle list
	return lhs->m_area > rhs->m_area;
}


// ---

OverlaidNavPoly::~OverlaidNavPoly ()
{
	if (m_unionFirst.m_obstaclesSubtracted != 0)
	{
		Allocator * alloc = m_unionFirst.m_obstaclesSubtracted->GetAllocator ();
		FreeHeritageTags (m_unionFirst.m_obstaclesSubtracted, alloc);

		Polygon::Destroy (m_unionFirst.m_obstaclesSubtracted);
		delete alloc;
	}

	RemoveOverlay ();
}

// NOTE Sep 23, 2008: <pvl> this function implements the non-iterative approach
// to subdivision where on any change to 'm_obstacles' complete subdivision
// is re-done from scratch
void OverlaidNavPoly::SubdivideNonIteratively ()
{
	Stopwatch watch;
	watch.Start ();

	// NOTE Okt 23, 2008: <pvl> all obstacles are gone now, no more dynamic overlay
	if (m_obstacles.empty ())
	{
		m_subdivision = 0;
		return;
	}

	AddOverlay ();

	// NOTE Sep 23, 2008: <pvl> now for the heavy lifting - take the original
	// static nav polygon shape and subtract all dynamic obstacles from it.  Then
	// tesselate the (quite likely concave) result so that it's a bunch of convex
	// polygons again and use those as new nav polygons.

	const unsigned memPoolSize = 500 * 1024;
	_smart_ptr<PoolAllocator> alloc[2];
	alloc[0] = new PoolAllocator (memPoolSize);
	alloc[1] = new PoolAllocator (memPoolSize);
	int curAlloc = 0;

	DynamicObstacles::const_iterator obstIt = m_obstacles.begin ();
	DynamicObstacles::const_iterator obstEnd = m_obstacles.end ();

	Polygon * p0 = Convert (0, (*m_polyCont)[m_polyIndex], alloc[curAlloc], true);
	Polygon * p1 = Convert (1, *(*obstIt++)->GetPoly (), alloc[curAlloc], true);

	Polygon * subtracted = 0;
	{
		PolygonSetOp op (p0, p1, alloc[curAlloc]);

		subtracted = op.Difference ();
		PostprocessCrossVtxHeritage (subtracted, alloc[curAlloc]);
	}

	for ( ; obstIt != obstEnd; ++obstIt)
	{
		curAlloc ^= 1;
		alloc[curAlloc]->Clear ();

		p0 = Convert (0, subtracted, alloc[curAlloc], true);
		p1 = Convert (1, *(*obstIt)->GetPoly (), alloc[curAlloc], true);

		PolygonSetOp nextOp (p0, p1, alloc[curAlloc]);

		subtracted = nextOp.Difference();
		PostprocessCrossVtxHeritage (subtracted, alloc[curAlloc]);
	}

	SubTessellator tess (subtracted, m_subdivision);

	watch.Stop ();
//	printf("  Subdivision: %.3fms\n", 1000.0 * watch.GetElapsed ());
}

void OverlaidNavPoly::SubdivideNonIteratively_ObstacleUnionFirst ()
{
	Stopwatch watch;
	watch.Start ();

	// NOTE Okt 23, 2008: <pvl> all obstacles are gone now, no more dynamic overlay
	if (m_obstacles.empty ())
	{
		m_subdivision = 0;
		return;
	}

	AddOverlay ();

	const unsigned memPoolSize = 500 * 1024;
	_smart_ptr<PoolAllocator> alloc[2];
	alloc[0] = new PoolAllocator (memPoolSize);
	alloc[1] = new PoolAllocator (memPoolSize);
	int curAlloc = 0;

	DynamicObstacles::const_iterator obstIt = m_obstacles.begin ();
	DynamicObstacles::const_iterator obstEnd = m_obstacles.end ();

	Polygon * obstacleUnion = Convert (0, *(*obstIt++)->GetPoly (), alloc[curAlloc], false);

	// NOTE Apr 8, 2009: <pvl> just for debugging
	unsigned obstIdx = 0;

	for ( ; obstIt != obstEnd; ++obstIt, ++obstIdx)
	{
		curAlloc ^= 1;
		alloc[curAlloc]->Clear ();

		Polygon * p0 = Convert (0, obstacleUnion, alloc[curAlloc], false);
		Polygon * p1 = Convert (1, *(*obstIt)->GetPoly (), alloc[curAlloc], false);

		PolygonSetOp op (p0, p1, alloc[curAlloc]);

		obstacleUnion = op.Disjunction ();

		if (0)
		{
			DumpPolyAndObstacles ();

			const int obstIdxWidth = (int )log10 ((float )m_obstacles.size ()) + 1;
			char fname[32];
			_snprintf (fname, 32, "union-%d-after-%.*d", m_polyIndex, obstIdxWidth, obstIdx);
			DumpPolygonForGnuplot (obstacleUnion, fname);
		}

		// NOTE May 29, 2009: <pvl> enable the following lines to have each
		// intermediate result tesselated (and the result thrown away).  This was
		// useful in checking which stage a self-intersection (as detected by the
		// tesselator) was introduced.
#if defined (_WIN32) && defined (USER_pavel)
try {
#endif
//		DynamicOverlay overlay;
//		SubTessellator tess (obstacleUnion, & overlay);
#if defined (_WIN32) && defined (USER_pavel)
} catch (float * where) {
	volatile unsigned polyIndex = m_polyIndex;
	volatile unsigned obst = obstIdx;
	DumpPolyAndObstacles ();
	char fname[32];
	_snprintf (fname, 32, "static-%d-final", m_polyIndex);
	DumpPolygonForGnuplot (obstacleUnion, fname);
}
#endif
	}

	// ---
	curAlloc ^= 1;
	alloc[curAlloc]->Clear ();

	Polygon * p0 = Convert (0, (*m_polyCont)[m_polyIndex], alloc[curAlloc], true);
	Polygon * p1 = Convert (1, obstacleUnion, alloc[curAlloc], true);

	PolygonSetOp op (p0, p1, alloc[curAlloc]);

	Polygon * subtracted = op.Difference ();
	PostprocessCrossVtxHeritage (subtracted, alloc[curAlloc]);

	StdAllocator * stdAlloc = new StdAllocator;
	m_unionFirst.m_obstaclesSubtracted = Convert (0, subtracted, stdAlloc, true);

#if defined (_WIN32) && defined (USER_pavel)
try {
#endif
	m_unionFirst.MapStaticEdgesToDynamic ();
#if defined (_WIN32) && defined (USER_pavel)
} catch (...) {
	assert (0);
}
#endif

	Stopwatch tessWatch;
	tessWatch.Start ();
#if defined (_WIN32) && defined (USER_pavel)
try {
#endif
	SubTessellator tess (subtracted, m_subdivision);
#if defined (_WIN32) && defined (USER_pavel)
} catch (float * where) {
	DumpPolyAndObstacles ();
	char fname[32];
	_snprintf (fname, 32, "static-%d-final", m_polyIndex);
	DumpPolygonForGnuplot (subtracted, fname);
}
#endif
	tessWatch.Stop ();

	watch.Stop ();
//	printf("  Subdivision: %d obstacles, %.3fms (out of which re-tesselation: %.3fms)\n", m_obstacles.size (), 1000.0 * watch.GetElapsed (), 1000.0 * tessWatch.GetElapsed ());

	assert (m_unionFirst.EnsureNoOverlappingVertices ());
//	if ( ! m_unionFirst.EnsureNoHorizontalDuplicates ())
//		__asm { int 3 }
}

VtxHeritage * GetHeritage (Polygon::Contour::VertexIt vtxIt)
{
	VtxUserData * data = vtxIt->GetUserData ();
	assert (data);
	assert (data->GetType () == VtxUserData::ORIG);
	OrigVtxUserData * dataOrig = static_cast <OrigVtxUserData*> (data);
	VtxHeritage * hrtg = reinterpret_cast <VtxHeritage*> (dataOrig->GetData ());
	assert (hrtg);
	return hrtg;
}

std::pair <unsigned,unsigned>
OverlaidNavPoly::UnionFirst::GetOriginalStaticEdge (Polygon::Contour::VertexIt prev, Polygon::Contour::VertexIt next)
{
	VtxHeritage * prevHrtg = GetHeritage (prev);
	VtxHeritage * nextHrtg = GetHeritage (next);

	// NOTE Mar 31, 2009: <pvl> these types are only used when obstacles are
	// subtracted from a static poly one by one - they can't and mustn't leak here
	assert (prevHrtg->m_type != USER_DATA_OBSTACLE_CROSS_VTX);
	assert (nextHrtg->m_type != USER_DATA_OBSTACLE_CROSS_VTX);

	// NOTE Mar 31, 2009: <pvl> if any of the endpoints is an obstacle vertex
	// obviously we can't be looking at a piece of a static edge
	if (prevHrtg->m_type == USER_DATA_OBSTACLE_SSHAPE || nextHrtg->m_type == USER_DATA_OBSTACLE_SSHAPE)
		return std::make_pair (PolygonCont::INVALID_VERTEX_INDEX, 0);

	if (prevHrtg->m_type == USER_DATA_NAV_POLY && nextHrtg->m_type == USER_DATA_NAV_POLY)
	{
		NavPolyVtxHeritage * endPtHrtg0 = static_cast<NavPolyVtxHeritage*> (prevHrtg);
		NavPolyVtxHeritage * endPtHrtg1 = static_cast<NavPolyVtxHeritage*> (nextHrtg);
		unsigned vtxIndex0 = endPtHrtg0->m_vtxIndex;
		unsigned vtxIndex1 = endPtHrtg1->m_vtxIndex;
		return std::make_pair (std::min (vtxIndex0, vtxIndex1), std::max (vtxIndex0, vtxIndex1));
	}

	// TODO Mar 31, 2009: <pvl> the remaining two cases are handled in much
	// the same way as CanonicaliseEdge() - could we share the code?
	if (prevHrtg->m_type == USER_DATA_CROSS_VTX && nextHrtg->m_type == USER_DATA_CROSS_VTX)
	{
		CrossVtxHeritage * xVtxPrev = static_cast <CrossVtxHeritage*> (prevHrtg);
		CrossVtxHeritage * xVtxNext = static_cast <CrossVtxHeritage*> (nextHrtg);

		for (int i=0; i<2; ++i)
			for (int j=0; j<2; ++j)
			{
				if ( *xVtxPrev->m_vtx[2*i] == *xVtxNext->m_vtx[2*j]
						&& *xVtxPrev->m_vtx[2*i+1] == *xVtxNext->m_vtx[2*j+1])
				{
					NavPolyVtxHeritage * endPtHrtg0 = static_cast<NavPolyVtxHeritage*> (xVtxPrev->m_vtx[2*i]);
					NavPolyVtxHeritage * endPtHrtg1 = static_cast<NavPolyVtxHeritage*> (xVtxPrev->m_vtx[2*i+1]);
					assert (endPtHrtg0->m_type == endPtHrtg1->m_type);
					if (endPtHrtg0->m_type != USER_DATA_NAV_POLY)
						return std::make_pair (PolygonCont::INVALID_VERTEX_INDEX, 0);
					unsigned vtxIndex0 = endPtHrtg0->m_vtxIndex;
					unsigned vtxIndex1 = endPtHrtg1->m_vtxIndex;
					return std::make_pair (std::min (vtxIndex0, vtxIndex1), std::max (vtxIndex0, vtxIndex1));
				}
			}

		assert (0);
	}

	// NOTE Mar 31, 2009: <pvl> one of them is static and the other cross

	NavPolyVtxHeritage * staticHrtg = static_cast<NavPolyVtxHeritage*> (prevHrtg->m_type == USER_DATA_NAV_POLY ? prevHrtg : nextHrtg);
	CrossVtxHeritage * xHrtg = static_cast<CrossVtxHeritage*> (prevHrtg->m_type == USER_DATA_CROSS_VTX ? prevHrtg : nextHrtg);
	assert (staticHrtg->m_type == USER_DATA_NAV_POLY);
	assert (xHrtg->m_type == USER_DATA_CROSS_VTX);

	// NOTE Jan 27, 2009: <pvl> find the non-cross vtx in the cross vtx descriptor
	int i=0;
	for (; i<4; ++i)
		if ( *xHrtg->m_vtx[i] == *staticHrtg)
			break;
#ifdef _WIN32
	if (i >= 4)
		throw "mrd";
#endif
	assert (i<4);
	// NOTE Jan 27, 2009: <pvl> get the other side of the same edge
	i ^= 0x1;

	NavPolyVtxHeritage * otherSideHrtg = static_cast <NavPolyVtxHeritage*> (xHrtg->m_vtx[i]);
	unsigned vtxIndex0 = staticHrtg->m_vtxIndex;
	unsigned vtxIndex1 = otherSideHrtg->m_vtxIndex;
	return std::make_pair (std::min (vtxIndex0, vtxIndex1), std::max (vtxIndex0, vtxIndex1));
}

// NOTE Jun 15, 2009: <pvl> the 'UnionFirst' version of doing things looks up
// vertices in overlays based on their 2D geometric position.  We'd better make
// sure that there's no ambiguity along the outside edges of an overlay since
// these vertices could be queried by neigbouring overlays.
bool OverlaidNavPoly::UnionFirst::EnsureNoOverlappingVertices () const
{
	std::vector <Vec3> vtxs;

	StaticToDynamicEdgeMap::const_iterator it = m_edgeMap.begin ();
	const StaticToDynamicEdgeMap::const_iterator end = m_edgeMap.end ();
	for ( ; it != end; ++it)
	{
		VtxHeritage * hrtg = GetHeritage (it->second);
		// NOTE Jun 15, 2009: <pvl> only consider cross vertices since static
		// vertices must be seen at the same coords from everywhere and obstacle
		// vertices (or obstacle cross vertices) shouldn't ever lie on boundary
		// shared by neighbouring overlays
		if (hrtg->m_type != USER_DATA_CROSS_VTX)
			continue;
		vtxs.push_back (it->second->Pos ());
	}

	std::sort (vtxs.begin (), vtxs.end (), Vec3LexLess ());
	std::vector<Vec3>::iterator newEnd = std::unique (vtxs.begin (), vtxs.end ());
	return newEnd - vtxs.begin () == vtxs.size ();
}

void OverlaidNavPoly::UnionFirst::MapStaticEdgesToDynamic ()
{
	m_edgeMap.clear ();

	for (unsigned ci=0, numContours = m_obstaclesSubtracted->NumContours (); ci < numContours; ++ci)
	{
		const Polygon::Contour * c = m_obstaclesSubtracted->GetContour (ci);

		Polygon::Contour::VertexLoopIt vtxIt = c->CreateVertexLoopIt ();
		for ( ; vtxIt; ++vtxIt)
		{
			std::pair <unsigned,unsigned> staticEdge = GetOriginalStaticEdge (vtxIt, vtxIt.GetNext ());
			if (staticEdge.first != PolygonCont::INVALID_VERTEX_INDEX)
			{
				assert (staticEdge.first < staticEdge.second);
				m_edgeMap.insert (std::make_pair (staticEdge, vtxIt));
			}
		}
	}
}

void OverlaidNavPoly::ClampDynamicVtxHeights ()
{
	std::pair <float,float> heightRange = (*m_polyCont)[m_polyIndex].GetHeightRange ();

	std::vector<Vec3>::iterator vtxIt = m_subdivision->m_vertices.begin ();
	const std::vector<Vec3>::iterator vtxEnd = m_subdivision->m_vertices.end ();
	for ( ; vtxIt != vtxEnd; ++vtxIt)
		if (vtxIt->z < heightRange.first)
			vtxIt->z = heightRange.first;
		else if (vtxIt->z > heightRange.second)
			vtxIt->z = heightRange.second;
}

OverlaidNavPoly::OverlaidNavPoly (PolygonCont * polyCont, unsigned int polyIndex) :
		m_polyCont (polyCont), m_polyIndex (polyIndex), m_subdivision (0)
{
}

void OverlaidNavPoly::AddObstacle (const DynamicObstacle * obst)
{
	m_obstacles.push_back (obst);
	// TODO Jun 2, 2009: <pvl> if the idea of sorting obstacles by area pans out
	// find out what the most efficient implementation would be.  std::set would
	// be better at sorting but it would also use more memory.
	std::sort (m_obstacles.begin (), m_obstacles.end (), & DynamicObstacle::PointerCompare);
}

// TODO Sep 25, 2008: <pvl> verify running time of this - 'm_obstacles' is
// a vector and as such is well suited for neither std::find() nor erase()
// of a random iterator - unless it's size() is guaranteed to be very small.
void OverlaidNavPoly::RemoveObstacle (const DynamicObstacle * obst)
{
	DynamicObstacles::iterator deadIt = std::find (m_obstacles.begin(), m_obstacles.end(), obst);
	if (deadIt == m_obstacles.end ())
		return;			// NOTE Sep 25, 2008: <pvl> log a warning?

	m_obstacles.erase (deadIt);
}

unsigned OverlaidNavPoly::NumObstacles () const
{
	return m_obstacles.size ();
}

void OverlaidNavPoly::Subdivide ()
{
	//SubdivideNonIteratively ();
	SubdivideNonIteratively_ObstacleUnionFirst ();

	if (m_subdivision)
		ClampDynamicVtxHeights ();
}

inline DynamicOverlay * OverlaidNavPoly::GetOverlay () const
{
	return m_subdivision;
}

inline void OverlaidNavPoly::AddOverlay ()
{
	m_subdivision = new DynamicOverlay;
	m_polyCont->Add (m_subdivision);
}

inline void OverlaidNavPoly::RemoveOverlay ()
{
	if (m_subdivision == 0) return;

	m_polyCont->Remove (m_subdivision);
	delete m_subdivision;
	m_subdivision = 0;
}

void OverlaidNavPoly::DumpPolyAndObstacles () const
{
	const unsigned numObst = NumObstacles();
	const int obstIdxWidth = (int )log10 ((float )numObst) + 1;
	for (unsigned obstIdx=0; obstIdx < numObst; ++obstIdx)
	{
		char buf[16];
		_snprintf (buf, 16, "%.*d", obstIdxWidth, obstIdx);
		FILE * fp = fopen (string ("poly-obstacle_") + string (buf), "w");

		m_obstacles[obstIdx]->GetPoly()->DumpIntoFile (fp);

		fclose (fp);
	}

	char buf[16];
	_snprintf (buf, 16, "%d", m_polyIndex);
	FILE * fp = fopen (string ("poly-static_") + string (buf), "w");

	(*m_polyCont)[m_polyIndex].DumpIntoFile (fp);

	fclose (fp);
}

// ---

// NOTE Okt 24, 2008: <pvl> corresponds to an overlay of a single static polygon
struct DynamicGraphOverlay {

	// NOTE Okt 24, 2008: <pvl> has to be the same as in the corresponding DynamicOverlay!
	unsigned int m_baseIndex;

	unsigned int m_staticPolyIndex;
	// NOTE Okt 30, 2008: <pvl> this is GraphNodeManager index
	unsigned int m_staticNodeIndex;
	// NOTE Okt 27, 2008: <pvl> indices of static polygons that share an edge with
	// this one.  It's necessary to keep this info to be able to reconnect the graph
	// once the overlay is gone.
	std::vector <unsigned int> m_staticNeighbours;

	// ATTN Okt 27, 2008: <pvl> these aren't indices used by dynamics handling,
	// these are the ones usable with GraphNodeManager!
	std::vector <unsigned int> m_dynamicNodes;

	DynamicGraphOverlay (unsigned int baseIndex);
	DynamicGraphOverlay (unsigned int baseIndex, const DynamicGraphOverlay & rhs);
};

// TODO Okt 28, 2008: <pvl> an invalid index to initialize 'm_staticPolyIndex' is needed
DynamicGraphOverlay::DynamicGraphOverlay (unsigned int baseIndex) :
		m_baseIndex (baseIndex), m_staticPolyIndex (std::numeric_limits<unsigned>::max()/*invalid*/),
		m_staticNodeIndex (0)
{
}

DynamicGraphOverlay::DynamicGraphOverlay (unsigned int baseIndex, const DynamicGraphOverlay & rhs) :
		m_baseIndex (baseIndex), m_staticPolyIndex (rhs.m_staticPolyIndex),
		m_staticNodeIndex (rhs.m_staticNodeIndex),
		m_staticNeighbours (rhs.m_staticNeighbours.begin (), rhs.m_staticNeighbours.end ())
{
}

// TODO Okt 24, 2008: <pvl> create a class like PolygonCont::DynamicOverlayMgr
// but for DynamicGraphOverlays.  Will probably need to pull index range allocator
// from there to make it available to graph overlay manager.

class DynamicGraphOverlayMgr {
	// NOTE Okt 24, 2008: <pvl> should only be queried by this class (i.e. no new
	// range allocations made from here)?
	const LayeredNavMesh::IndexRangeMgr * m_indexRangeMgr;

	std::vector <DynamicGraphOverlay*> m_overlays;
public:
	DynamicGraphOverlayMgr (unsigned numStaticPolygons, LayeredNavMesh::IndexRangeMgr * indexRangeMgr);
	~DynamicGraphOverlayMgr ();

	// NOTE Okt 24, 2008: <pvl> by symmetry with PolygonCont::DynamicOverlayMgr
	// NOTE Okt 24, 2008: <pvl> unlike PolygonCont::DynamicOverlayMgr and DynamicOverlays,
	// DynamicGraphOverlay passed over in the argument should already have its
	// base index set (?)
	void AddOverlay (DynamicGraphOverlay * );
	void RemoveOverlay (DynamicGraphOverlay * );

	DynamicGraphOverlay * GetOverlayByStaticPoly (unsigned staticPolyIndex) const;
	DynamicGraphOverlay * GetOverlayByDynamicPoly (unsigned dynamicPolyIndex) const;

	unsigned int operator[] (unsigned int index) const;
};

DynamicGraphOverlayMgr::DynamicGraphOverlayMgr (unsigned numStaticPolygons, LayeredNavMesh::IndexRangeMgr * indexRangeMgr) :
		// FIXME Okt 28, 2008: <pvl> this a duplicate of the way DynamicOverlayMgr
		// initializes its 'm_overlays'
		m_indexRangeMgr (indexRangeMgr), m_overlays (numStaticPolygons/4, 0)
{
}

DynamicGraphOverlayMgr::~DynamicGraphOverlayMgr ()
{
	std::vector <DynamicGraphOverlay*>::const_iterator it = m_overlays.begin ();
	std::vector <DynamicGraphOverlay*>::const_iterator end = m_overlays.end ();
	for ( ; it != end; ++it)
		delete *it;
}

void DynamicGraphOverlayMgr::AddOverlay (DynamicGraphOverlay * newOverlay)
{
	// NOTE Okt 28, 2008: <pvl> 'm_baseIndex' must have been set already
	assert (newOverlay->m_baseIndex != 0);
	LayeredNavMesh::IndexRangeMgr::RangeId rangeNum = m_indexRangeMgr->GetRangeFromIndex (newOverlay->m_baseIndex);
	// TODO Okt 28, 2008: <pvl> maybe assert that this range is used?	

	if (rangeNum >= m_overlays.size ())
		m_overlays.resize (rangeNum + 1);
	m_overlays[rangeNum] = newOverlay;
}

void DynamicGraphOverlayMgr::RemoveOverlay (DynamicGraphOverlay * overlay)
{
	const unsigned baseIndex = overlay->m_baseIndex;
	const LayeredNavMesh::IndexRangeMgr::RangeId rangeNum = m_indexRangeMgr->GetRangeFromIndex (baseIndex);
	assert (rangeNum < m_overlays.size ());
	m_overlays[rangeNum] = 0;
}

DynamicGraphOverlay * DynamicGraphOverlayMgr::GetOverlayByStaticPoly (unsigned staticPolyIndex) const
{
	std::vector <DynamicGraphOverlay*>::const_iterator it = m_overlays.begin ();
	std::vector <DynamicGraphOverlay*>::const_iterator end = m_overlays.end ();
	for ( ; it != end; ++it)
	{
		if ( (*it) == 0 ) continue;
		if ( (*it)->m_staticPolyIndex == staticPolyIndex )
			return *it;
	}
	return 0;
}

DynamicGraphOverlay * DynamicGraphOverlayMgr::GetOverlayByDynamicPoly (unsigned dynamicPolyIndex) const
{
	LayeredNavMesh::IndexRangeMgr::RangeId rangeNum = m_indexRangeMgr->GetRangeFromIndex (dynamicPolyIndex);
	assert (rangeNum < m_overlays.size ());
	return m_overlays[rangeNum];
}

unsigned int DynamicGraphOverlayMgr::operator[] (unsigned int index) const
{
	std::pair<unsigned,unsigned> rangeAndIndex = m_indexRangeMgr->Translate (index);
	const unsigned rangeNum = rangeAndIndex.first;
	const unsigned indexIntoRange = rangeAndIndex.second;
	assert (m_overlays[rangeNum]);
	assert (m_overlays[rangeNum]->m_dynamicNodes.size () > indexIntoRange);
	return m_overlays[rangeNum]->m_dynamicNodes[indexIntoRange];
}



void GraphNodeCont::SetupStaticGraphNodes (
		const CGraph * pGraph, unsigned numNodes, unsigned modifId, int agentTypeId)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "LNM - Graph Nodes (static)");

	m_staticGraphNodes.resize (numNodes);

	CAllNodesContainer::Iterator it (pGraph->GetAllNodes(), IAISystem::NAV_LAYERED_NAV_MESH);

	while (unsigned graphNodeIndex = it.Increment())
	{
		const GraphNode * node = pGraph->GetNodeManager().GetNode (graphNodeIndex);
		const SLayeredMeshNavData * lnmData = node->GetLayeredMeshNavData();
		if (lnmData->navModifIndex != modifId)
			continue;
		// TODO Aug 7, 2009: <pvl> agentTypeId == -1 means either a bug or we've
		// read in an old export file version that doesn't support agent types yet.
		// If the former is the case we shouldn't really have got as far as here,
		// and the old export files should go away at some point.  So some time
		// from now the -1 check should be removed.
		if (agentTypeId != -1 && lnmData->agentType != agentTypeId)
			continue;
		unsigned polygonIndex = lnmData->polygonIndex;
		assert (polygonIndex < numNodes);
		m_staticGraphNodes[polygonIndex] = graphNodeIndex;
	}
}


unsigned GraphNodeCont::operator[] (unsigned index) const
{
	if (m_indexRangeMgr->IsIndexDynamic (index))
	{
		return (*m_overlayMgr)[index];
	}
	else
	{
		assert (index < m_staticGraphNodes.size ());
		return m_staticGraphNodes[index];
	}
}

void GraphNodeCont::Clear ()
{
	m_staticGraphNodes.resize (0);
}

void GraphNodeCont::SetupReadingFromFile (LayeredNavMesh::ExportFormat::FileReader * fileReader)
{
	ExportFormat::VersionSwitchChunkReader * versionSwitch = new ExportFormat::VersionSwitchChunkReader;
	versionSwitch->RegisterReader (1, new GraphNodeIndexListChunkReader (m_staticGraphNodes));
	fileReader->RegisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_GRAPH_NODE_INDICES, versionSwitch);
}

void GraphNodeCont::CleanupReadingFromFile (LayeredNavMesh::ExportFormat::FileReader * fileReader)
{
	delete fileReader->UnregisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_GRAPH_NODE_INDICES);
}

// ---

class DynamicObstacleMgr {
	// NOTE Sep 23, 2008: <pvl> we don't own these
	CGraph * m_graph;
	PolygonCont * m_polygonCont;
	GraphNodeCont * m_graphNodeCont;
	PickingAccelerator * m_pickingAccel;

	// ---
	typedef std::map <const ObstaclePolygon*,DynamicObstacle*> Obstacles;
	Obstacles m_obstacles;

	// ---
	LayeredNavMesh::IndexRangeMgr m_indexRangeMgr;

	DynamicGraphOverlayMgr m_graphOverlayMgr;

	typedef std::map <unsigned /*poly index*/, OverlaidNavPoly*> OverlaidPolys;
	OverlaidPolys m_overlaidPolys;

	typedef std::vector <OverlaidNavPoly*> OverlaidPolyVec;
	OverlaidPolyVec m_dirtyPolys;

	// NOTE Okt 13, 2008: <pvl> creates the poly if it's not there already
	OverlaidNavPoly * GetOverlaidPoly (unsigned int polyIndex);
	// NOTE Okt 14, 2008: <pvl> returns 0 if there's no overlay for 'polyIndex'
	OverlaidNavPoly * FindOverlaidPoly (unsigned int polyIndex) const;
	void RemoveOverlaidPoly (OverlaidNavPoly * );

	// FIXME Nov 5, 2008: <pvl> currently this function only registers overlay's
	// polygons with PickingAccelerator since that's what OverlaidNavPoly can't do
	// (it doesn't have a pointer to the PickingAccelerator).  The rest is done
	// in OverlaidNavPoly.  Having the setup of a new overlay split like this is
	// ugly and error prone.  Considering that OverlaidNavPoly can't even clean up
	// its overlay anymore (that's done by ProcessObstacles()) it might make more
	// sense to just rip the rest of overlay lifetime management out of OverlaidNavPoly
	// and collect it here.
	void AddOverlay (DynamicOverlay * );
	void RemoveOverlay (DynamicOverlay * );

	void ConnectOverlay (OverlaidNavPoly * );
	unsigned int GetStaticNeighbourFromOverlay (unsigned polyIndex, DynamicGraphOverlay * graphOverlay,
			unsigned staticVtx0, unsigned staticVtx1) const;
	unsigned int GetStaticNeighbour (unsigned polyIndex, unsigned staticVtx0, unsigned staticVtx1) const;
	unsigned int GetDynamicNeighbour (unsigned staticNeighIndex, const VtxAttribs::Attrib * endPt0, const VtxAttribs::Attrib * endPt1) const;
	unsigned int GetDynamicNeighbour_ObstacleUnionFirst (OverlaidNavPoly * poly, unsigned sharedEdgeEndPt0, unsigned sharedEdgeEndPt1, unsigned staticNeighIndex, unsigned dynamicEndPt0, unsigned dynamicEndPt1) const;

	void ClearObstacles ();
	void ClearOverlaidNavPolys ();

	// NOTE Okt 27, 2008: <pvl> nav graph manipulation functions
	void DisconnectFromStaticMesh (OverlaidNavPoly * poly, DynamicGraphOverlay * graphOverlay);
	void CreateDynamicNodes (OverlaidNavPoly * poly, DynamicGraphOverlay * graphOverlay);
	void RecordStaticConnectivity (OverlaidNavPoly * poly, DynamicGraphOverlay * graphOverlay);
	void CreateGraphNodes (OverlaidNavPoly * poly);
	void ConnectGraphOverlay (OverlaidNavPoly * poly);
	void RestoreStaticPolygon (OverlaidNavPoly * poly);

public:
	DynamicObstacleMgr (CGraph * , PolygonCont * , GraphNodeCont * , PickingAccelerator * );
	~DynamicObstacleMgr ();
	// TODO Sep 23, 2008: <pvl> take a Shape for testing but with actual obstacles
	// (that would always be entities I assume) obviously something else should be
	// passed in - Entity* comes to mind.
	// NOTE Sep 23, 2008: <pvl> these functions are meant to be called when
	// a dynamic blocker stops moving (AddObstacle()) or starts moving again
	// (RemoveObstacle()).  It's up to the caller to implement hysteresis or
	// something else to prevent entities that move by small amounts all the time
	// from spamming the system.  Remember that both of these calls are expensive,
	// at least potentially.
	void AddObstacle (const ObstaclePolygon * );
	void RemoveObstacle (const ObstaclePolygon * );

	void ProcessObstacles ();

	bool IsOverlaid (unsigned polygonIndex) const;

	unsigned int GetStaticNeighbourFromGraph (unsigned polyIndex, unsigned staticVtx0, unsigned staticVtx1) const;
};

OverlaidNavPoly * DynamicObstacleMgr::GetOverlaidPoly (unsigned int polyIndex)
{
	OverlaidPolys::iterator it = m_overlaidPolys.find (polyIndex);
	if (it == m_overlaidPolys.end ())
	{
		// NOTE Sep 23, 2008: <pvl> if OverlaidPoly starts requiring a non-trivial ctor
		// with arguments that are inconvenient to pass in here just return 0 here
		it = m_overlaidPolys.insert (
				std::make_pair (polyIndex, new OverlaidNavPoly (m_polygonCont, polyIndex))
		).first;
	}

	return it->second;
}

OverlaidNavPoly * DynamicObstacleMgr::FindOverlaidPoly (unsigned int polyIndex) const
{
	OverlaidPolys::const_iterator it = m_overlaidPolys.find (polyIndex);
	if (it == m_overlaidPolys.end ())
		return 0;

	return it->second;
}

void DynamicObstacleMgr::RemoveOverlaidPoly (OverlaidNavPoly * poly)
{
	// NOTE Okt 31, 2008: <pvl> just remove it from bookkeeping but don't delete
	// it yet - it could still be referenced by later phases of ProcessObstacles().
	// Leave it up to ProcessObstacles() epilogue to free the actual memory.
	OverlaidPolys::iterator it = m_overlaidPolys.find (poly->PolyIndex ());
	assert (it != m_overlaidPolys.end ());
	m_overlaidPolys.erase (it);
}

bool DynamicObstacleMgr::IsOverlaid (unsigned polygonIndex) const
{
	if (m_indexRangeMgr.IsIndexDynamic (polygonIndex))
		return false;

	return FindOverlaidPoly (polygonIndex) != 0;
}

void DynamicObstacleMgr::ClearObstacles ()
{
	Obstacles::iterator it = m_obstacles.begin ();
	Obstacles::iterator end = m_obstacles.end ();
	for ( ; it != end; ++it)
		delete it->second;
}

void DynamicObstacleMgr::ClearOverlaidNavPolys ()
{
	OverlaidPolys::iterator it = m_overlaidPolys.begin ();
	OverlaidPolys::iterator end = m_overlaidPolys.end ();
	for ( ; it != end; ++it)
		delete it->second;
}

DynamicObstacleMgr::DynamicObstacleMgr (
			CGraph * graph, PolygonCont * polyCont,
			GraphNodeCont * graphNodeCont, PickingAccelerator * pick) :
		m_graph (graph), m_polygonCont (polyCont),
		m_graphNodeCont (graphNodeCont), m_pickingAccel (pick),
		m_indexRangeMgr (m_polygonCont->GetPolyCount ()),
		m_graphOverlayMgr (m_polygonCont->GetPolyCount(), & m_indexRangeMgr)
{
	m_polygonCont->SetupDynamicObstacleHandling ( & m_indexRangeMgr);
	// TODO Okt 29, 2008: <pvl> the asymetry between GraphNodeCont and PolygonCont
	// is unpleasant, maybe at some point PolygonCont::DynamicOverlayMgr could
	// also live here like DynamicGraphOverlayMgr?
	m_graphNodeCont->SetupDynamicObstacleHandling ( & m_graphOverlayMgr, & m_indexRangeMgr);
}

DynamicObstacleMgr::~DynamicObstacleMgr ()
{
	ClearObstacles ();
	ClearOverlaidNavPolys ();
}

void DynamicObstacleMgr::AddOverlay (DynamicOverlay * newOverlay)
{
	const unsigned numPolys = newOverlay->m_polygons.size ();
	const unsigned baseIndex = newOverlay->GetBaseIndex ();
	for (unsigned int p = baseIndex; p < baseIndex + numPolys; ++p)
		m_pickingAccel->Add (p);
}

void DynamicObstacleMgr::RemoveOverlay (DynamicOverlay * overlay)
{
	const unsigned numPolys = overlay->m_polygons.size ();
	const unsigned baseIndex = overlay->GetBaseIndex ();
	for (unsigned int p = baseIndex; p < baseIndex + numPolys; ++p)
		m_pickingAccel->Remove (p);
}

void DynamicObstacleMgr::AddObstacle (const ObstaclePolygon * obst)
{
	// TODO Sep 23, 2008: <pvl> should this check be even performed here?  Is it
	// not a sign of a higher-level screw-up when someone calls this function on
	// an LNM nav region instance that isn't properly initialized?
	if (m_pickingAccel == 0)
		return;

#ifdef _DEBUG
	if (m_obstacles.find (obst) != m_obstacles.end ())
	{
		AIWarning ("DynamicObstacleMgr: called to add an obstacle that's been added already");
		return;
	}
#endif
	// NOTE Sep 23, 2008: <pvl> setup internal book-keeping for the new obstacle
	// TODO Sep 23, 2008: <pvl> at some point this will involve computing a convex
	// hull or other expensive operations.  Caching these results might make sense
	// sometimes, e.g. with entities that never rotate (so their hull is always
	// the same) or only rotate about vertical axis (so their hull is constant
	// but might be rotated).
	// TODO Sep 23, 2008: <pvl> consider avoiding lookup e.g. by storing the
	// DynamicObstacle* with the entity itself.
	DynamicObstacle * obstRec = new DynamicObstacle (obst);
	m_obstacles.insert (std::make_pair (obst, obstRec));

	Vec3 obstCentroid = obst->GetCentroid ();

	// FIXME Sep 23, 2008: <pvl> as of now, this test is purely AABB-based
	std::vector <unsigned int> affectedPolys;
	m_pickingAccel->Pick (obst->GetAABB()).swap (affectedPolys);

	// NOTE Sep 23, 2008: <pvl> let the obstacle and the affected polygons know
	// about each other
	std::vector <unsigned int>::const_iterator polyIt = affectedPolys.begin ();
	std::vector <unsigned int>::const_iterator polyEnd = affectedPolys.end ();
	for ( ; polyIt != polyEnd; ++polyIt)
	{
		if (m_indexRangeMgr.IsIndexDynamic (*polyIt))
			continue;

		Vec3 polyCentroid = (*m_polygonCont)[*polyIt].GetCentroid ();

		// NOTE Mar 13, 2009: <pvl> disabling this since I'm not sure if it can
		// work reliably in this simplistic form.  The immediate problem it causes
		// is that if a poly doesn't get an overlay based on this test but its
		// neighbour does then code that connects up overlays fails.  That's because
		// if it sees a cross vertex on the shared edge it assumes there must be
		// an overlay on the other side as well.  (Note that this could be fixed
		// by just removing this assumption but I'm not confident yet that this
		// wouldn't have undesired side-effects.)
//		if ( fabs (polyCentroid.z - obstCentroid.z) > 2*1.8f/*agent height?*/)
//			continue;

		OverlaidNavPoly * poly = GetOverlaidPoly (*polyIt);

		poly->AddObstacle (obstRec);
		obstRec->AddIntersectedPoly (poly);

		m_dirtyPolys.push_back (poly);
	}
}

void DynamicObstacleMgr::RemoveObstacle (const ObstaclePolygon * obst)
{
	Obstacles::iterator obstRecIt = m_obstacles.find (obst);

	if (obstRecIt == m_obstacles.end ())
		return;			// NOTE Sep 25, 2008: <pvl> is this worth a warning?

	DynamicObstacle * obstRec = obstRecIt->second;

	obstRec->Detach ();

	DynamicObstacle::PolygonIter it = obstRec->PolysBegin ();
	DynamicObstacle::PolygonIter end = obstRec->PolysEnd ();
	for ( ; it != end; ++it)
		m_dirtyPolys.push_back (*it);

	delete obstRec;
	m_obstacles.erase (obstRecIt);
}

void DynamicObstacleMgr::ProcessObstacles ()
{
	Stopwatch watch;
	watch.Start ();

	// NOTE Sep 30, 2008: <pvl> when an obstacle is being added each polygon
	// overlapped by it is added to the dirty list, independent of other obstacles
	// if any.  Consequently, if a polygon is intersected by more than one obstacle
	// it will be added more than once.  That's why we need to make them unique
	// before any further processing.
	std::sort (m_dirtyPolys.begin (), m_dirtyPolys.end ());
	m_dirtyPolys.erase (std::unique (m_dirtyPolys.begin (), m_dirtyPolys.end ()), m_dirtyPolys.end ());

	// NOTE Nov 4, 2008: <pvl> overlays of polygons that are in 'm_dirtyPolys'
	// will be removed from these polys (either replaced by updated ones or just
	// dropped if a poly isn't intersected by an obstacle anymore).  However, we
	// need to keep them alive (as in "still registered with overlay manager")
	// for the rest of the reconnecting process to run correctly.
	// That's why we store them in 'overlaysToRemove' and get rid of them just
	// before this function exits.
	std::vector<DynamicOverlay*> overlaysToRemove;
	overlaysToRemove.reserve (m_dirtyPolys.size ());
	OverlaidPolyVec::const_iterator it = m_dirtyPolys.begin ();
	OverlaidPolyVec::const_iterator end = m_dirtyPolys.end ();
	for ( ; it != end; ++it)
		overlaysToRemove.push_back ( (*it)->GetOverlay () );

	it = m_dirtyPolys.begin ();
	for ( ; it != end; ++it)
	{
		// TODO Okt 16, 2008: <pvl> how do you clean up an overlay under a scheme
		// where overlays don't correspond to polygons?  Probably in RemoveObstacle()
		// (since an overlay corresponds to an obstacle in such case)?
		(*it)->Subdivide ();
	}

	// NOTE Okt 14, 2008: <pvl> now that all overlays have been produced
	// it's possible to correctly connect neighbouring overlays

	// NOTE Okt 30, 2008: <pvl> first, unregister polys that aren't going to have
	// an overlay any more
	it = m_dirtyPolys.begin ();
	for ( ; it != end; ++it)
		if ( 0 == (*it)->GetOverlay () )
			RemoveOverlaidPoly (*it);
		
	it = m_dirtyPolys.begin ();
	for ( ; it != end; ++it)
	{
		// NOTE Okt 23, 2008: <pvl> if all obstacles previously overlapping a poly
		// are gone, it won't have any overlay anymore at this point.  If that's the
		// case, remove it from our bookkeeping.
		if ( (*it)->GetOverlay () )
		{
			AddOverlay ( (*it)->GetOverlay () );
			ConnectOverlay (*it);
			CreateGraphNodes (*it);
		}
	}

	// NOTE Okt 27, 2008: <pvl> at this point, everything's connected up at the
	// polygonal mesh level but the actual graph nodes have just been created.
	// Connect them up now using the nav mesh connectivity.
	it = m_dirtyPolys.begin ();
	for ( ; it != end; ++it)
	{
		ConnectGraphOverlay (*it);
	}

	// NOTE Okt 31, 2008: <pvl> done, now just clean up stale overlay data of
	// polygons that are no longer overlaid
	it = m_dirtyPolys.begin ();
	for ( ; it != end; ++it)
		if ( (*it)->GetOverlay () == 0)
			delete *it;

	// NOTE Nov 4, 2008: <pvl> get rid of the now-obsolete overlays
	// FIXME Nov 4, 2008: <pvl> this should really go to a separate function
	std::vector<DynamicOverlay*>::const_iterator overIt = overlaysToRemove.begin ();
	std::vector<DynamicOverlay*>::const_iterator overEnd = overlaysToRemove.end();
	for ( ; overIt != overEnd; ++overIt)
	{
		// NOTE Nov 4, 2008: <pvl> this can happen if a polygon didn't have
		// an overlay previously (and only got one during this update)
		if (*overIt == 0) continue;

		if (DynamicGraphOverlay * graphOverlay = m_graphOverlayMgr.GetOverlayByDynamicPoly ( (*overIt)->GetBaseIndex () ))
		{
			std::vector<unsigned>::const_iterator nodeIt = graphOverlay->m_dynamicNodes.begin ();
			std::vector<unsigned>::const_iterator nodeEnd = graphOverlay->m_dynamicNodes.end ();
			for ( ; nodeIt != nodeEnd; ++nodeIt)
				m_graph->Disconnect (*nodeIt, true);

			m_graphOverlayMgr.RemoveOverlay (graphOverlay);
			delete graphOverlay;
		}
		RemoveOverlay (*overIt);
		m_polygonCont->Remove (*overIt);
		delete *overIt;
	}

	m_dirtyPolys.resize (0);

	watch.Stop ();
	//printf("  ProcessObstacles(): %.3fms\n", 1000.0 * watch.GetElapsed ());
}

unsigned int DynamicObstacleMgr::GetStaticNeighbourFromGraph (unsigned polyIndex, unsigned staticVtx0, unsigned staticVtx1) const
{
	FRAME_PROFILER ("GetStaticNeighbourFromGraph", gEnv->pSystem, PROFILE_AI)

	const CGraph * graph = m_graph;

	const GraphNode * node = graph->GetNodeManager().GetNode ((*m_graphNodeCont)[polyIndex]);

	// NOTE Okt 14, 2008: <pvl> for each link incident to 'node'
	unsigned linkId = node->firstLinkIndex;
	for ( ; linkId; linkId = graph->GetLinkManager().GetNextLink(linkId))
	{
		// NOTE Okt 14, 2008: <pvl> get graph node at the far end of this link
		const unsigned nextNodeIndex = graph->GetLinkManager().GetNextNode(linkId);
		const GraphNode * next = graph->GetNodeManager().GetNode(nextNodeIndex);

		// NOTE Okt 14, 2008: <pvl> get the corresponding static polygon index from it

		// NOTE Nov 12, 2008: <pvl> disabled the assert for now since with the current
		// nav graph implementation it's not guaranteed that we won't ever run into
		// a non-LNM node (at least the "safe first" node (NAV_UNSET) can always come up)
	//	assert (next->navType == IAISystem::NAV_LAYERED_NAV_MESH);
		if (next->navType != IAISystem::NAV_LAYERED_NAV_MESH) continue;
		const unsigned nextPolyIndex = next->GetLayeredMeshNavData()->polygonIndex;

		// NOTE Okt 14, 2008: <pvl> get endpoints of the shared edge
		CommonPolyVertices commonVerts ((*m_polygonCont)[polyIndex], (*m_polygonCont)[nextPolyIndex]);
		assert (2 == commonVerts.Count());
		unsigned sharedEdgeEndPt0 = commonVerts[0];
		unsigned sharedEdgeEndPt1 = commonVerts[1];
		if (sharedEdgeEndPt0 > sharedEdgeEndPt1)
			std::swap (sharedEdgeEndPt0, sharedEdgeEndPt1);

		// NOTE Okt 14, 2008: <pvl> if 'polyIndex' and 'nextPolyIndex' share the
		// edge we're looking for we've found our neighbour
		if (staticVtx0 == sharedEdgeEndPt0 && staticVtx1 == sharedEdgeEndPt1)
		{
			if (m_indexRangeMgr.IsIndexDynamic (nextPolyIndex))
			{
				DynamicGraphOverlay * graphOverlay = m_graphOverlayMgr.GetOverlayByDynamicPoly (nextPolyIndex);
				assert (graphOverlay);
				return graphOverlay->m_staticPolyIndex;
			}
			else
				return nextPolyIndex;
		}
	}

	// NOTE Okt 15, 2008: <pvl> this can happen at the perifery of a nav mesh
	return DynamicOverlay::INVALID_NEIGHBOR;
}

// NOTE Dec 18, 2008: <pvl> DynamicGraphOverlay* is passed in as an argument
// since looking it up is rather expensive (see GetOverlayByStaticPoly()) and it
// needs to be performed by the caller anyway
unsigned int DynamicObstacleMgr::GetStaticNeighbourFromOverlay (
		unsigned polyIndex, DynamicGraphOverlay * graphOverlay,
		unsigned staticVtx0, unsigned staticVtx1) const
{
	std::vector<unsigned>::const_iterator it = graphOverlay->m_staticNeighbours.begin ();
	std::vector<unsigned>::const_iterator end = graphOverlay->m_staticNeighbours.end ();
	for ( ; it != end; ++it)
	{
		CommonPolyVertices commonVerts ((*m_polygonCont)[polyIndex], (*m_polygonCont)[*it]);
		assert (2 == commonVerts.Count());
		unsigned sharedEdgeEndPt0 = commonVerts[0];
		unsigned sharedEdgeEndPt1 = commonVerts[1];
		if (sharedEdgeEndPt0 > sharedEdgeEndPt1)
			std::swap (sharedEdgeEndPt0, sharedEdgeEndPt1);

		// NOTE Okt 14, 2008: <pvl> if 'polyIndex' and 'nextPolyIndex' share the
		// edge we're looking for we've found our neighbour
		if (staticVtx0 == sharedEdgeEndPt0 && staticVtx1 == sharedEdgeEndPt1)
			return *it;
	}
	return DynamicOverlay::INVALID_NEIGHBOR;
}

// FIXME Okt 14, 2008: <pvl> this function extracts the neighbour index directly
// from the nav graph.  The advantage is that no additional storage is used, the
// drawback is that this very likely to be very slow.  The other drawback will
// be that this relies on one-to-one correspondence between nav mesh polys and
// graph nodes which is by no means guaranteed.  Any other scheme will require
// modifications to this function or break it completely.

// NOTE Okt 14, 2008: <pvl> 'staticVtx[01]' form an edge of 'poly'
// TODO Okt 14, 2008: <pvl> look into grouping them in some Edge struct
// TODO Okt 14, 2008: <pvl> does this need to live in DynamicObstacleMgr?
unsigned int DynamicObstacleMgr::GetStaticNeighbour (unsigned polyIndex, unsigned staticVtx0, unsigned staticVtx1) const
{
	if (staticVtx0 > staticVtx1)
		std::swap (staticVtx0, staticVtx1);

	// NOTE Okt 30, 2008: <pvl> if polyIndex isn't overlaid use graph, if it is
	// look in graph overlay
	if (DynamicGraphOverlay * graphOverlay = m_graphOverlayMgr.GetOverlayByStaticPoly (polyIndex))
		return GetStaticNeighbourFromOverlay (polyIndex, graphOverlay, staticVtx0, staticVtx1);
	else
		return GetStaticNeighbourFromGraph (polyIndex, staticVtx0, staticVtx1);
}

unsigned int DynamicObstacleMgr::GetDynamicNeighbour (
			unsigned staticNeighIndex,
			const VtxAttribs::Attrib * endPt0, const VtxAttribs::Attrib * endPt1) const
{
	OverlaidNavPoly * neighPoly = FindOverlaidPoly (staticNeighIndex);
	assert (neighPoly);
	DynamicOverlay * neighOverlay = neighPoly->GetOverlay ();
	assert (neighOverlay);
	unsigned dynamicNeighIndex = neighOverlay->GetPolyFromOutsideEdge (endPt0, endPt1);
	assert (dynamicNeighIndex != DynamicOverlay::INVALID_NEIGHBOR);
	return dynamicNeighIndex;
}


unsigned int DynamicObstacleMgr::GetDynamicNeighbour_ObstacleUnionFirst (
			OverlaidNavPoly * poly, unsigned sharedEdgeEndPt0, unsigned sharedEdgeEndPt1,
			unsigned staticNeighIndex, unsigned dynamicEndPt0, unsigned dynamicEndPt1) const
{
	const OverlaidNavPoly * neighPoly = FindOverlaidPoly (staticNeighIndex);
	assert (neighPoly);

	const std::pair <unsigned,unsigned> sharedEdge (
			std::min (sharedEdgeEndPt0, sharedEdgeEndPt1),
			std::max (sharedEdgeEndPt0, sharedEdgeEndPt1)
	);

	typedef OverlaidNavPoly::UnionFirst::StaticToDynamicEdgeMap::const_iterator EdgeMapIt;
	const std::pair <EdgeMapIt,EdgeMapIt> edgePiecesRange = poly->m_unionFirst.m_edgeMap.equal_range (sharedEdge);
	const std::pair <EdgeMapIt,EdgeMapIt> neighEdgePiecesRange = neighPoly->m_unionFirst.m_edgeMap.equal_range (sharedEdge); 

	std::vector <Vec3> edgePieces;
	EdgeMapIt edgePieceIt = edgePiecesRange.first;
	for ( ; edgePieceIt != edgePiecesRange.second; ++edgePieceIt)
	{
		const Polygon::Contour::VertexIt vtxIt = edgePieceIt->second;
		edgePieces.push_back (vtxIt->Pos ());
		edgePieces.push_back (vtxIt.GetNext()->Pos ());
	}

	std::vector <Vec3> neighEdgePieces;
	EdgeMapIt neighEdgePieceIt = neighEdgePiecesRange.first;
	for ( ; neighEdgePieceIt != neighEdgePiecesRange.second; ++neighEdgePieceIt)
	{
		const Polygon::Contour::VertexIt vtxIt = neighEdgePieceIt->second;
		neighEdgePieces.push_back (vtxIt->Pos ());
		neighEdgePieces.push_back (vtxIt.GetNext()->Pos ());
	}

	assert (edgePieces.size () == neighEdgePieces.size ());

	std::sort (edgePieces.begin (), edgePieces.end (), Vec3LexLess ());
	std::sort (neighEdgePieces.begin (), neighEdgePieces.end (), Vec3LexLess ());

#ifdef _DEBUG
	std::vector <Vec3>::const_iterator vtxIt = edgePieces.begin ();
	std::vector <Vec3>::const_iterator neighVtxIt = neighEdgePieces.begin ();
	const std::vector <Vec3>::const_iterator vtxEnd = edgePieces.end ();
	const std::vector <Vec3>::const_iterator neighVtxEnd = neighEdgePieces.end ();
	for ( ; vtxIt != vtxEnd && neighVtxIt != neighVtxEnd; ++vtxIt, ++neighVtxIt)
	{
		if (Vec2 (*vtxIt) != Vec2 (*neighVtxIt))
		{
			const Vec2 vtx (*vtxIt);
			const Vec2 neighVtx (*neighVtxIt);
			int x_diff = *(unsigned * )&vtx.x - *(unsigned * )&neighVtx.x;
			int y_diff = *(unsigned * )&vtx.y - *(unsigned * )&neighVtx.y;

			assert (abs (x_diff) <= 1 && abs (y_diff) <= 1);
		}
	}
#endif // _DEBUG

	DynamicOverlay * overlay = poly->GetOverlay ();
	const Vec3 sharedEdgeEndPt0Pos = overlay->m_vertices[dynamicEndPt0];
	const Vec3 sharedEdgeEndPt1Pos = overlay->m_vertices[dynamicEndPt1];
	int index0 = -1;
	int index1 = -1;
	for (unsigned i=0, num=edgePieces.size(); i < num; ++i)
	{
		if (Vec2 (edgePieces[i]) == Vec2 (sharedEdgeEndPt0Pos))
			index0 = i;
		if (Vec2 (edgePieces[i]) == Vec2 (sharedEdgeEndPt1Pos))
			index1 = i;
		if (index0 != -1 && index1 != -1)
			break;
	}
	assert (index0 != -1 && index1 != -1);

	DynamicOverlay * neighOverlay = neighPoly->GetOverlay ();

	unsigned dynamicNeighIndex = neighOverlay->GetPolyFromOutsideEdge (neighEdgePieces[index0], neighEdgePieces[index1]);	
	assert (dynamicNeighIndex != DynamicOverlay::INVALID_NEIGHBOR);
	return dynamicNeighIndex;
}

// TODO Okt 13, 2008: <pvl> IMPORTANT: if the adjacency information on the
// polygon level turns out to be only used to rewire the nav graph, it would
// likely speed things up a lot if we build the graph directly - that is,
// allocate new graph nodes directly after re-tessellation and from that point
// on (BuildAdjacency() etc.) work on graph node level exclusively.
void DynamicObstacleMgr::ConnectOverlay (OverlaidNavPoly * staticPoly)
{
	DynamicOverlay * overlay = staticPoly->GetOverlay ();

	// TODO Okt 13, 2008: <pvl> make this more abstract - this code loops over all
	// polygons and all their (half)edges and for each one of them that doesn't
	// have its adjacency information set yet, determine who the neighbour is or
	// that there's no neighbour.

	std::vector<DynamicOverlay::Polygon>::const_iterator polyIt = overlay->m_polygons.begin ();
	std::vector<DynamicOverlay::Polygon>::const_iterator polyEnd = overlay->m_polygons.end ();
	for ( ; polyIt != polyEnd; ++polyIt)
	{
		const DynamicOverlay::Polygon & poly = *polyIt;
		const unsigned numEdges = poly.m_numIndices;
		for (unsigned i=0; i < numEdges; ++i)
		{
			if (overlay->m_neighbours[poly.m_startIndex + i] != DynamicOverlay::INVALID_NEIGHBOR)
				continue;

			// NOTE Okt 14, 2008: <pvl> this edge's endpoints
			unsigned endPt0 = overlay->m_indices [poly.m_startIndex + i];
			unsigned endPt1 = overlay->m_indices [overlay->NextPolygonVtxIndex (poly, i)];

			unsigned char typeVert0 = overlay->m_vtxAttribs[endPt0]->m_type;
			unsigned char typeVert1 = overlay->m_vtxAttribs[endPt1]->m_type;

			// NOTE Okt 13, 2008: <pvl> any edge with at least one endpoint coming from
			// an obstacle isn't adjacent to any walkable polygon
			if (typeVert0 == USER_DATA_OBSTACLE_SSHAPE || typeVert1 == USER_DATA_OBSTACLE_SSHAPE)
				continue;

			// NOTE Jan 28, 2009: <pvl> if either of this edge's endpoints comes
			// from two obstacle edges intersecting each other, there can be no
			// walkable polygon to connect to behind this edge
			if (typeVert0 == USER_DATA_OBSTACLE_CROSS_VTX || typeVert1 == USER_DATA_OBSTACLE_CROSS_VTX)
				continue;

			if (typeVert0 == USER_DATA_CROSS_VTX && typeVert1 == USER_DATA_CROSS_VTX)
			{
				CrossVtxData * crossVert0 = (CrossVtxData * )overlay->m_vtxAttribs[endPt0];
				CrossVtxData * crossVert1 = (CrossVtxData * )overlay->m_vtxAttribs[endPt1];
				if ( ! (crossVert0->m_static0.m_vtxIndex == crossVert1->m_static0.m_vtxIndex
							&& crossVert0->m_static1.m_vtxIndex == crossVert1->m_static1.m_vtxIndex))
				{
					// NOTE Okt 15, 2008: <pvl> both cross vertices refer to different static
					// edges.  That means that the original static nav poly was cut in two
					// by an obstacle edge.  The edge we're looking at here is a part of
					// that obstacle edge and there's no walkable polygon on its other side.
					continue;
				}
			}

			// NOTE Okt 13, 2008: <pvl> if both endpoints are static vertices then
			// just extract adjacency from the nav graph
			if (typeVert0 == USER_DATA_NAV_POLY && typeVert1 == USER_DATA_NAV_POLY)
			{
				unsigned staticVtxIndex0 = ((StaticVtxData * )overlay->m_vtxAttribs[endPt0])->m_vtxIndex;
				unsigned staticVtxIndex1 = ((StaticVtxData * )overlay->m_vtxAttribs[endPt1])->m_vtxIndex;
				if (staticVtxIndex0 > staticVtxIndex1)
					std::swap (staticVtxIndex0, staticVtxIndex1);

				overlay->m_neighbours[poly.m_startIndex + i] = GetStaticNeighbour (staticPoly->PolyIndex(), staticVtxIndex0, staticVtxIndex1);
			}
			else
			{
				// NOTE Okt 13, 2008: <pvl> by now, either one of them is static and the
				// other is cross, or both are cross.

				assert (typeVert0 == USER_DATA_CROSS_VTX || typeVert1 == USER_DATA_CROSS_VTX);
				unsigned crossEndPt = typeVert0 == USER_DATA_CROSS_VTX ? endPt0 : endPt1;

				CrossVtxData * crossVert = (CrossVtxData * )overlay->m_vtxAttribs[crossEndPt];
				unsigned staticVtxIndex0 = crossVert->m_static0.m_vtxIndex;
				unsigned staticVtxIndex1 = crossVert->m_static1.m_vtxIndex;

				unsigned staticNeighIndex = GetStaticNeighbour (staticPoly->PolyIndex(), staticVtxIndex0, staticVtxIndex1);

				// NOTE Jan 22, 2009: <pvl> can legally be INVALID_POLYGON_INDEX if
				// the edge (staticVtxIndex0,staticVxtIndex1) is a border edge of
				// the nav mesh so there's no neighbour polygon on the other side
				if (staticNeighIndex != PolygonCont::INVALID_POLYGON_INDEX)
				{
//					overlay->m_neighbours[poly.m_startIndex + i] = GetDynamicNeighbour (
//							staticNeighIndex, overlay->m_vtxAttribs[endPt0], overlay->m_vtxAttribs[endPt1]
//					);
					overlay->m_neighbours[poly.m_startIndex + i] = GetDynamicNeighbour_ObstacleUnionFirst (
							staticPoly, staticVtxIndex0, staticVtxIndex1, staticNeighIndex, endPt0, endPt1
					);
				}
			}
		}
	}
}

void DynamicObstacleMgr::RecordStaticConnectivity (OverlaidNavPoly * poly, DynamicGraphOverlay * graphOverlay)
{
	unsigned nodeIndex = (*m_graphNodeCont)[poly->PolyIndex ()];
	GraphNode * node = m_graph->GetNodeManager().GetNode (nodeIndex);
	graphOverlay->m_staticPolyIndex = poly->PolyIndex ();
	graphOverlay->m_staticNodeIndex = nodeIndex;

	// NOTE Okt 24, 2008: <pvl> for each link incident to 'node'
	unsigned linkId = node->firstLinkIndex;
	for ( ; linkId; linkId = m_graph->GetLinkManager().GetNextLink(linkId))
	{
		// NOTE Okt 24, 2008: <pvl> get graph node at the far end of this link
		const unsigned nextNodeIndex = m_graph->GetLinkManager().GetNextNode(linkId);
		const GraphNode * next = m_graph->GetNodeManager().GetNode(nextNodeIndex);

		// NOTE Okt 24, 2008: <pvl> get the corresponding polygon index from it

		// NOTE Nov 12, 2008: <pvl> disabled the assert for now since with the current
		// nav graph implementation it's not guaranteed that we won't ever run into
		// a non-LNM node (at least the "safe first" node (NAV_UNSET) can always come up)
	//	assert (next->navType == IAISystem::NAV_LAYERED_NAV_MESH);
		if (next->navType != IAISystem::NAV_LAYERED_NAV_MESH) continue;
		unsigned nextPolyIndex = next->GetLayeredMeshNavData()->polygonIndex;

		// NOTE Okt 24, 2008: <pvl> if it's a static poly we're done, otherwise we
		// need to query dynamic overlays to get the underlying static one
		if (m_indexRangeMgr.IsIndexDynamic (nextPolyIndex))
		{
			DynamicGraphOverlay * neighGraphOverlay = m_graphOverlayMgr.GetOverlayByDynamicPoly (nextPolyIndex);
			assert (neighGraphOverlay);
			nextPolyIndex = neighGraphOverlay->m_staticPolyIndex;
		}

		graphOverlay->m_staticNeighbours.push_back (nextPolyIndex);
	}
}

// NOTE Okt 27, 2008: <pvl> called while a new graph overlay is being added to the system
void DynamicObstacleMgr::DisconnectFromStaticMesh (OverlaidNavPoly * poly, DynamicGraphOverlay * graphOverlay)
{
	m_graph->Disconnect ((*m_graphNodeCont)[poly->PolyIndex ()], false);
}

// NOTE Okt 27, 2008: <pvl> just create nodes, one for each poly in this overlay,
// but don't try to connect them up just yet.  Their neighbours might be polygons
// in a neighbour overlay that hasn't have its nodes even created yet.
// UPDATE Nov 5, 2008: <pvl> is that really a problem?  Could we not leave the
// links unresolved whenever 'm_graphNodeCont' doesn't have a node (yet) for
// a particular polygon index and then just create the links while the overlays
// containing the missing nodes are being generated?
void DynamicObstacleMgr::CreateDynamicNodes (OverlaidNavPoly * poly, DynamicGraphOverlay * graphOverlay)
{
	const DynamicOverlay * overlay = poly->GetOverlay ();
	assert (overlay);

	unsigned numPolys = overlay->m_polygons.size ();
	for (unsigned p=0; p < numPolys; ++p)
	{
		const ::Vec3 graphNodePos = (*overlay)[p].GetCentroid ();

		unsigned nodeIndex = m_graph->CreateNewNode (
				IAISystem::NAV_LAYERED_NAV_MESH, graphNodePos
		);

		GraphNode * node = m_graph->GetNodeManager().GetNode (nodeIndex);
		node->GetLayeredMeshNavData()->polygonIndex = p + overlay->m_baseIndex;

		graphOverlay->m_dynamicNodes.push_back (nodeIndex);
	}
}

void DynamicObstacleMgr::CreateGraphNodes (OverlaidNavPoly * poly)
{
	DynamicGraphOverlay * graphOverlay = m_graphOverlayMgr.GetOverlayByStaticPoly (poly->PolyIndex ());
	if (graphOverlay == 0)
	{
		graphOverlay = new DynamicGraphOverlay (poly->GetOverlay()->GetBaseIndex());
		RecordStaticConnectivity (poly, graphOverlay);
	}
	else
	{
		// NOTE Nov 4, 2008: <pvl> if there is an existing overlay we need to keep
		// it intact since a subsequent phase of ProcessObstacles() might need it.
		// So we create a new one using a "pseudo-cctor" that only transfers static
		// connectivity data and leaves anything dynamic empty.  Also base index
		// is changed to correspond to that of this graph overlay's sibling navmesh
		// overlay.
		graphOverlay = new DynamicGraphOverlay (poly->GetOverlay()->GetBaseIndex (), *graphOverlay);
	}
	m_graphOverlayMgr.AddOverlay (graphOverlay);

	CreateDynamicNodes (poly, graphOverlay);
}

void DynamicObstacleMgr::ConnectGraphOverlay (OverlaidNavPoly * polyRec)
{
	const DynamicOverlay * overlay = polyRec->GetOverlay ();
	if (0 == overlay)
	{
		RestoreStaticPolygon (polyRec);
		return;
	}

	DynamicGraphOverlay * graphOverlay = m_graphOverlayMgr.GetOverlayByDynamicPoly (overlay->GetBaseIndex());

	DisconnectFromStaticMesh (polyRec, graphOverlay);

	unsigned numPolys = overlay->m_polygons.size ();
	for (unsigned p=0; p < numPolys; ++p)
	{
		const PolygonCont::InternalPolygon & poly = overlay->m_polygons[p];

		const unsigned thisNodeIndex = graphOverlay->m_dynamicNodes[p];

		unsigned firstIndex = poly.m_startIndex;
		unsigned lastIndex = firstIndex + poly.m_numIndices;
		for (unsigned vtxIndex = firstIndex; vtxIndex < lastIndex; ++vtxIndex)
		{
			const unsigned neighPolyIndex = overlay->m_neighbours[vtxIndex];
			if (neighPolyIndex == DynamicOverlay::INVALID_NEIGHBOR)
				continue;

			unsigned neighNodeIndex = (*m_graphNodeCont)[neighPolyIndex];

			// NOTE Okt 27, 2008: <pvl> now, link them!! :-)
			int linkIndex = m_graph->GetNodeManager().GetNode(thisNodeIndex)->GetLinkIndex (
					m_graph->GetNodeManager(),
					m_graph->GetLinkManager(),
					m_graph->GetNodeManager().GetNode (neighNodeIndex)
			);
			if (-1 == linkIndex)
				m_graph->Connect (thisNodeIndex, neighNodeIndex);
		}
	}
}

void DynamicObstacleMgr::RestoreStaticPolygon (OverlaidNavPoly * poly)
{
	// TODO Okt 30, 2008: <pvl> now that poly's original DynamicOverlay is not
	// accessible through OverlaidNavPoly* anymore we don't have its base index
	// handy here.  If we did we could use it for fast lookup here and didn't
	// need to fall back to this slow way.
	DynamicGraphOverlay * graphOverlay = m_graphOverlayMgr.GetOverlayByStaticPoly (poly->PolyIndex ());

	// NOTE Jan 21, 2009: <pvl> this can happen if an obstacle was added and
	// then removed during the same frame.  This seems pathological but it does
	// happen with vehicles driven by the player.  Physics doesn't seem to
	// know about the player and puts the vehicle to sleep after a while.  The
	// vehicle seems to be then awoken immediately by something game code does.
	if (graphOverlay == 0)
		return;

	// NOTE Okt 30, 2008: <pvl> loop over static neighbours.  Connect to those
	// not overlaid by any obstacles, query those who are for their subdivision
	// polygon to connect to.
	std::vector<unsigned>::const_iterator neighNodeIt = graphOverlay->m_staticNeighbours.begin();
	std::vector<unsigned>::const_iterator neighNodeEnd = graphOverlay->m_staticNeighbours.end();
	for ( ; neighNodeIt != neighNodeEnd; ++neighNodeIt)
	{
		unsigned neighGraphNode = 0;

		// TODO Okt 30, 2008: <pvl> would be cleaner to factor out both branches
		// into separate functions
		if (OverlaidNavPoly * neighPoly = FindOverlaidPoly (*neighNodeIt))
		{
			CommonPolyVertices common ((*m_polygonCont)[poly->PolyIndex()], (*m_polygonCont)[neighPoly->PolyIndex()]);
			assert (common.Count());
			StaticVtxData sharedVtx0 (common[0]);
			StaticVtxData sharedVtx1 (common[1]);
			// TODO Okt 30, 2008: <pvl> would it make sense to derive vtx data structs
			// from Attrib to avoid casting in cases like this?
			unsigned dynamicNeigh = neighPoly->GetOverlay()->GetPolyFromOutsideEdge (
					reinterpret_cast<VtxAttribs::Attrib*> (& sharedVtx0),
					reinterpret_cast<VtxAttribs::Attrib*> (& sharedVtx1)
			);
			neighGraphNode = (*m_graphNodeCont)[dynamicNeigh];
		}
		else
		{
			neighGraphNode = (*m_graphNodeCont)[*neighNodeIt];
		}

		assert (neighGraphNode);

		int linkIndex = m_graph->GetNodeManager().GetNode(graphOverlay->m_staticNodeIndex)->GetLinkIndex (
					m_graph->GetNodeManager(),
					m_graph->GetLinkManager(),
					m_graph->GetNodeManager().GetNode(neighGraphNode)
		);
		if (-1 == linkIndex)
			m_graph->Connect (graphOverlay->m_staticNodeIndex, neighGraphNode);
	}
}

// ---
/*
static std::vector<Vec3> ConvertEntityMeshToPointSet (pe_status_pos& statusPos, pe_params_part& paramsPart)
{
	IGeometry* p_geometry = paramsPart.pPhysGeomProxy->pGeom;
	assert((p_geometry->GetType() == GEOM_TRIMESH) || (p_geometry->GetType() == GEOM_VOXELGRID));
	const mesh_data* p_mesh_data = static_cast<const mesh_data*>(p_geometry->GetData());

	std::vector<Vec3> pts;

	const strided_pointer<Vec3>& pVertices = p_mesh_data->pVertices;
	for (int i = 0; i < p_mesh_data->nVertices; i++)
	{
		Vec3 vec3 = statusPos.scale * pVertices[i];
		Vec3 worldC = statusPos.pos + statusPos.q * vec3;
		pts.push_back(worldC);
	}
	return pts;
}
*/
static std::vector<Vec3> ConvertEntityBBoxToPointSet (pe_status_pos& statusPos)
{
	primitives::box box;
	statusPos.pGeomProxy->GetBBox(&box);

	// Scale the bounding box appropriately
	box.center *= statusPos.scale;
	box.size   *= statusPos.scale;

	Vec3 worldC = statusPos.pos + statusPos.q * box.center;
	Matrix33 worldM = Matrix33(statusPos.q) * box.Basis.GetTransposed();

	std::vector<Vec3> pts;

	pts.push_back(worldC + worldM * Vec3(box.size.x, box.size.y, box.size.z));
	pts.push_back(worldC + worldM * Vec3(box.size.x, box.size.y, -box.size.z));
	pts.push_back(worldC + worldM * Vec3(box.size.x, -box.size.y, box.size.z));
	pts.push_back(worldC + worldM * Vec3(box.size.x, -box.size.y, -box.size.z));
	pts.push_back(worldC + worldM * Vec3(-box.size.x, box.size.y, box.size.z));
	pts.push_back(worldC + worldM * Vec3(-box.size.x, box.size.y, -box.size.z));
	pts.push_back(worldC + worldM * Vec3(-box.size.x, -box.size.y, box.size.z));
	pts.push_back(worldC + worldM * Vec3(-box.size.x, -box.size.y, -box.size.z));

	return pts;
}

// NOTE Jan 22, 2009: <pvl> a bunch of function objects for STL algorithms
bool CompareZ (const Vec3 & v0, const Vec3 & v1)
{
	if (v0.z < v1.z)
		return true;
	return false;
} 

class SetZ {
	float m_z;
public:
	SetZ (float z) : m_z (z) { }

	void operator() (Vec3 & v) const
	{
		v.z = m_z;
	} 
};

// NOTE Mar 19, 2009: <pvl> perturbs Vec3's horizontal position by a random amount
class Perturb {
	static CMTRand_int32 s_rand;

	float m_bound;
public:
	Perturb (float bound) : m_bound (bound) { }

	void operator() (Vec3 & v) const
	{
		v.x += 2*(s_rand.GenerateFloat() - 0.5f) * m_bound;
		v.y += 2*(s_rand.GenerateFloat() - 0.5f) * m_bound;
	} 
};
CMTRand_int32 Perturb::s_rand;

string GetEntityName (const IPhysicalEntity * entity)
{
	if (entity->GetiForeignData () != 2)
		return string ("");

	IEntity * e = (IEntity * )entity->GetForeignData (2);
	return e->GetName ();
}

class EntityToPolygonConvertor {
	ObstaclePolygon m_polygon;
public:
	EntityToPolygonConvertor (const IPhysicalEntity * );
	const ObstaclePolygon & GetPoly () const { return m_polygon; }
};

EntityToPolygonConvertor::EntityToPolygonConvertor (const IPhysicalEntity * entity) :
		m_polygon (GetEntityName (entity))
{
	// NOTE Jan 22, 2009: <pvl> relevant vertices representing an entity
	// over all of its parts will be accumulated here
	std::vector<Vec3> points;

	// Get the number of parts of the entity
	pe_status_nparts statusNParts;
	int nParts = entity->GetStatus(&statusNParts);

	pe_params_part paramsPart;
	pe_status_pos statusPos;
	for (statusPos.ipart = 0, paramsPart.ipart = 0 ; statusPos.ipart < nParts ; ++statusPos.ipart, ++paramsPart.ipart)
	{
		// Consider only those parts which can collide with the player
		if (0 == entity->GetParams(&paramsPart))
			continue;
		if (!(paramsPart.flagsAND & geom_colltype_player))
			continue;

		// Get the position of part #ipart
		if (0 == entity->GetStatus(&statusPos))
			continue;

		// Consider only those parts which have some associated geometry
		IGeometry* p_geometry = statusPos.pGeomProxy;
		if (!p_geometry)
			continue;

/*
		int geometry_type = p_geometry->GetType();
		if ((geometry_type == GEOM_TRIMESH) || (geometry_type == GEOM_VOXELGRID))
		{
			ConvertEntityMeshToPointSet (statusPos, paramsPart).swap (points);
		}
		else
		{
			ConvertEntityBBoxToPointSet (statusPos).swap (points);
		}

		polygon.Flush ();
*/
		const std::vector<Vec3> & partPts = ConvertEntityBBoxToPointSet (statusPos);
		points.insert (points.end(), partPts.begin(), partPts.end());
	}
	if (points.empty ())
		return;

	// NOTE Jan 22, 2009: <pvl> put all points to the height level of the lowest
	// one.  This approximates the notion of "projecting to the ground" rather well.
	std::vector<Vec3>::const_iterator minZIt = std::min_element (points.begin (), points.end (), CompareZ);
	std::for_each (points.begin (), points.end (), SetZ (minZIt->z));
	// NOTE Mar 19, 2009: <pvl> an attempt to get rid of incident vertices/edges
	//std::for_each (points.begin (), points.end (), Perturb (0.05f));

	ConvexHull2D (m_polygon, points);

	// NOTE Mar 3, 2009: <pvl> 'points' can contain duplicate vertices and so can
	// the hull - get rid of those
	std::vector<Vec3> & hull (m_polygon);
	hull.erase (std::unique (hull.begin(), hull.end()), hull.end());
	// NOTE Mar 3, 2009: <pvl> if first and last are the same std::unique() doesn't
	// take care of this
	if (hull.front() == hull.back())
		hull.pop_back ();

	EnsureShapeIsWoundAnticlockwise <std::vector<Vec3>, float> (hull);
}

class PathBlockingEntityMgr {
	DynamicObstacleMgr * m_obstacleMgr;

	typedef std::multimap <const IPhysicalEntity*, ObstaclePolygon> PathBlockerCache;
	PathBlockerCache mCache;

	static int PathBlockerStartedOrStoppedMoving (const EventPhys * );

	void AddToCache (const IPhysicalEntity * );
	void RemoveFromCache (const IPhysicalEntity * entity);

	void AddObstacles (const IPhysicalEntity * );
	void RemoveObstacles (const IPhysicalEntity * );
public:
	PathBlockingEntityMgr (DynamicObstacleMgr * );
	~PathBlockingEntityMgr ();

	void AddPathBlocker (const IPhysicalEntity * );
	void RemovePathBlocker (const IPhysicalEntity * );

	void DebugDraw () const;
};

// NOTE Jan 20, 2009: <pvl> physics doesn't allow registering of client data
// with an event callback.  In order for the callback function (which has no
// 'this') to be able to access an PathBlockingEntityMgr instance we need to
// make it static.
// ATTN Jan 20, 2009: <pvl> this could cause trouble when starting using multiple
// instances of LayeredNavMeshRegion.  I think in principle PathBlockingEntityMgr
// could be a singleton serving all of LNM nav region instances but I'm not
// sure about this yet.
static PathBlockingEntityMgr * s_pathBlockerMgr = 0;

PathBlockingEntityMgr::PathBlockingEntityMgr (DynamicObstacleMgr * dynObstMgr) :
		m_obstacleMgr (dynObstMgr)
{
	gEnv->pPhysicalWorld->AddEventClient(EventPhysStateChange::id, PathBlockerStartedOrStoppedMoving, 1);
	s_pathBlockerMgr = this;
}

PathBlockingEntityMgr::~PathBlockingEntityMgr ()
{
	gEnv->pPhysicalWorld->RemoveEventClient (EventPhysStateChange::id, PathBlockerStartedOrStoppedMoving, 1);
}

void PathBlockingEntityMgr::AddPathBlocker (const IPhysicalEntity * entity)
{
	assert (s_pathBlockerMgr->mCache.find (entity) == s_pathBlockerMgr->mCache.end ());
	if (s_pathBlockerMgr->mCache.find (entity) != s_pathBlockerMgr->mCache.end ())
		return;

	s_pathBlockerMgr->AddToCache (entity);
	s_pathBlockerMgr->AddObstacles (entity);
}

void PathBlockingEntityMgr::RemovePathBlocker (const IPhysicalEntity * entity)
{
	assert (s_pathBlockerMgr->mCache.find (entity) != s_pathBlockerMgr->mCache.end ());
	if (s_pathBlockerMgr->mCache.find (entity) == s_pathBlockerMgr->mCache.end ())
		return;

	s_pathBlockerMgr->RemoveObstacles (entity);
	s_pathBlockerMgr->RemoveFromCache (entity);
}

int PathBlockingEntityMgr::PathBlockerStartedOrStoppedMoving (const EventPhys * event)
{
	// TODO Aug 5, 2009: <pvl> can we avoid querying the cvar all the time?
	// UPDATE Aug 7, 2009: <pvl> Matt tells me that it's possible to make cvar
	// use our piece of memory for storage - look into that.
	if (gAIEnv.CVars.lnmDynamics == 0)
		return 1;

	EventPhysStateChange * stateChangeEvent = (EventPhysStateChange * )event;
	int newSymClass = stateChangeEvent->iSimClass[1];
	int oldSymClass = stateChangeEvent->iSimClass[0];

	if (stateChangeEvent->iForeignData != 2)
		return 1;

	IPhysicalEntity * entity = stateChangeEvent->pEntity;

	assert (stateChangeEvent->iForeignData == 2);


	//IEntity * e = (IEntity * )stateChangeEvent->pForeignData;
	//if (0 != strcmp (e->GetName (), "Asian_tank1"))
	//	return 1;


	if (oldSymClass == SC_SLEEPING_RIGID && newSymClass == SC_ACTIVE_RIGID)
	{
		// NOTE Jan 19, 2009: <pvl> has woken up
		//AIWarning ("%p waking up (frame %d)\n", entity,	gEnv->pRenderer->GetFrameID ());
		s_pathBlockerMgr->RemovePathBlocker (entity);
	}

	if (oldSymClass == SC_ACTIVE_RIGID && newSymClass == SC_SLEEPING_RIGID)
	{
		// NOTE Jan 19, 2009: <pvl> goes to sleep
		//AIWarning ("%p going to sleep (frame %d)\n", entity, gEnv->pRenderer->GetFrameID ());
		s_pathBlockerMgr->AddPathBlocker (entity);
	}

	return 1;
}

void PathBlockingEntityMgr::AddToCache (const IPhysicalEntity * entity)
{
	assert (mCache.find (entity) == mCache.end ());
	if (mCache.find (entity) != mCache.end ())
		return;

	EntityToPolygonConvertor convertor (entity);
	const ObstaclePolygon & polygon = convertor.GetPoly ();
	if (polygon.Empty ())
		return;

	mCache.insert (std::make_pair (entity, polygon));
}

void PathBlockingEntityMgr::RemoveFromCache (const IPhysicalEntity * entity)
{
	std::pair <PathBlockerCache::iterator, PathBlockerCache::iterator>
	range = mCache.equal_range (entity);

	mCache.erase (range.first, range.second);
}

void PathBlockingEntityMgr::AddObstacles (const IPhysicalEntity * entity)
{
	std::pair <PathBlockerCache::const_iterator, PathBlockerCache::const_iterator>
	range = mCache.equal_range (entity);

	PathBlockerCache::const_iterator obstPolyIt = range.first;
	const PathBlockerCache::const_iterator obstPolyEnd = range.second;
	for ( ; obstPolyIt != obstPolyEnd; ++obstPolyIt)
	{
		m_obstacleMgr->AddObstacle ( & obstPolyIt->second);
	}
}

void PathBlockingEntityMgr::RemoveObstacles (const IPhysicalEntity * entity)
{
	std::pair <PathBlockerCache::const_iterator, PathBlockerCache::const_iterator>
	range = mCache.equal_range (entity);

	PathBlockerCache::const_iterator obstPolyIt = range.first;
	const PathBlockerCache::const_iterator obstPolyEnd = range.second;
	for ( ; obstPolyIt != obstPolyEnd; ++obstPolyIt)
	{
		m_obstacleMgr->RemoveObstacle ( & obstPolyIt->second);
	}
}

void PathBlockingEntityMgr::DebugDraw () const
{
	PathBlockerCache::const_iterator obstPolyIt = mCache.begin ();
	const PathBlockerCache::const_iterator obstPolyEnd = mCache.end ();
	for ( ; obstPolyIt != obstPolyEnd; ++obstPolyIt)
	{
		const ObstaclePolygon & obstPoly = obstPolyIt->second;
		obstPoly.DebugDraw ();
	}
}



#ifdef _DEBUG
std::vector<Vec3> ConvertToVector (const SShape * shape)
{
	return std::vector<Vec3> (shape->shape.begin (), shape->shape.end ());
}

// NOTE Jan 23, 2009: <pvl> a small class to track obstacles that don't correspond
// to path blocking entities but are added by debugging code as SShapes.
class DbgPathBlockingEntityMgr {
	DynamicObstacleMgr * m_obstacleMgr;

	typedef std::map <const SShape*, ObstaclePolygon> PathBlockerCache;
	PathBlockerCache m_cache;
public:
	DbgPathBlockingEntityMgr (DynamicObstacleMgr * );

	void AddPathBlocker (const SShape * );
	void RemovePathBlocker (const SShape * );
};

DbgPathBlockingEntityMgr::DbgPathBlockingEntityMgr (DynamicObstacleMgr * obstMgr) :
		m_obstacleMgr (obstMgr)
{
}

void DbgPathBlockingEntityMgr::AddPathBlocker (const SShape * shape)
{
	//assert (m_cache.find (shape) == m_cache.end ());
	if (m_cache.find (shape) != m_cache.end ())
		return;

	// NOTE Jan 23, 2009: <pvl> will insert the entry
	ObstaclePolygon & polygon = m_cache[shape];
	ConvertToVector (shape).swap (polygon);

	m_obstacleMgr->AddObstacle ( & polygon);
}

void DbgPathBlockingEntityMgr::RemovePathBlocker (const SShape * shape)
{
	PathBlockerCache::iterator it = m_cache.find (shape);

	//assert (it != m_cache.end ());
	if (it == m_cache.end ())
		return;

	m_obstacleMgr->RemoveObstacle ( & it->second);
	m_cache.erase (it);
}
#endif // _DEBUG

// ---

// NOTE Dec 2, 2009: <pvl> keep in mind this class assumes that the polygon is
// convex and that nav mesh vertices are only on the edges of navigable areas
// (never within them).  Unfortunately, if runtime AI will ever needs to support
// sectorised loading the latter assumtion will most likely become invalid.
// TODO Dec 2, 2009: <pvl> on the other hand, *until* it becomes invalid, handling
// of the case where we hit a vertex could probably be further simplified by just
// picking up any of the two incident edges.  If we hit a vertex then we hit an
// obstacle (since vertices always mean obstacle contours) so the raycasting ends
// here.  The only thing that's left is computation of the location of the hit.
// Both incident edges are, in general, equally good for this purpose.
class RayConvexPolyIntersection2D {

	unsigned m_edgeHit;
	unsigned m_numEdgesHit;

	enum RelativePosition { LEFT, ON, RIGHT };

	RelativePosition GetRelativePosition (const Vec3 & pt, const Vec3 & rayOrig, const Vec3 & rayDir) const;
public:
	template <typename VtxIt>
	RayConvexPolyIntersection2D (VtxIt vtxBegin, VtxIt vtxEnd, const Vec3 & rayOrig, const Vec3 & rayDir);

	operator bool () const;
	unsigned GetIntersectedEdge () const;
	bool HitVertex () const;
};

RayConvexPolyIntersection2D::RelativePosition
RayConvexPolyIntersection2D::GetRelativePosition (const Vec3 & pt, const Vec3 & rayOrig, const Vec3 & rayDir) const
{
	const Vec3 toVtx (pt - rayOrig);
	const float cross = rayDir.x * toVtx.y - toVtx.x * rayDir.y;
	return cross > 0.0f ? LEFT : cross == 0.0f ? ON : RIGHT;
}

template <typename VtxIt>
RayConvexPolyIntersection2D::RayConvexPolyIntersection2D (VtxIt vtxBegin, VtxIt vtxEnd, const Vec3 & rayOrig, const Vec3 & rayDir) :
		m_numEdgesHit (0)
{
	// NOTE Nov 25, 2009: <pvl> compute which side of the ray the polygon vertices
	// lie.  Do that just once for each vertex to ensure consistency in presence
	// of finite precision arithmetics.

	VtxIt lastIt (vtxEnd);
	--lastIt;
	// NOTE Dec 1, 2009: <pvl> if <vtxBegin,vtxEnd) is non-empty 'lastIt' is valid
	int prevRelPos = GetRelativePosition (*lastIt, rayOrig, rayDir);

	VtxIt vtxIt (vtxBegin);
	for (unsigned edgeIdx=0; vtxIt != vtxEnd; ++vtxIt, ++edgeIdx)
	{
		const int thisRelPos = GetRelativePosition (*vtxIt, rayOrig, rayDir);

		if (thisRelPos == LEFT && prevRelPos == RIGHT)
		{
			// NOTE Dec 1, 2009: <pvl> next polygon to be checked is the one
			// behind the edge starting at previous vertex and ending at this one

			m_edgeHit = edgeIdx;
			m_numEdgesHit = 1;
			return;
		}
		else if (thisRelPos == ON && prevRelPos == RIGHT)
		{
			// NOTE Dec 1, 2009: <pvl> check next vertex - it has to be on the LEFT
			// if this is to be a valid exit from this polygon.
			//
			// That's because if it's on the RIGHT we have a sequence of
			// RIGHT-ON-RIGHT, obviously just touching the polygon at this vertex.
			// We don't need to check any further because the polygon is convex.
			//
			// If it's ON the ray then the sequence is RIGHT-<one or more ON's>-RIGHT.
			// The last one has to be on the RIGHT because if it were on the LEFT
			// it would mean the last ON vertex is reflex and the polygon is not
			// convex.  So again, if next is ON there's no proper exit and we
			// don't need to look any further.
			VtxIt nextIt (vtxIt);
			++nextIt;
			if (nextIt == vtxEnd)
				nextIt = vtxBegin;

			const int nextRelPos = GetRelativePosition (*nextIt, rayOrig, rayDir);
			if (nextRelPos == LEFT)
			{
				// NOTE Dec 1, 2009: <pvl> check polygons behind both edges
				// incident with 'vtxIt'

				m_edgeHit = edgeIdx;
				m_numEdgesHit = 2;
				return;
			}
			else
			{
				return;
			}
		}
		prevRelPos = thisRelPos;
	}
}

RayConvexPolyIntersection2D::operator bool () const
{
	return m_numEdgesHit != 0;
}

unsigned RayConvexPolyIntersection2D::GetIntersectedEdge () const
{
	assert (m_numEdgesHit > 0);
	return m_edgeHit;
}

bool RayConvexPolyIntersection2D::HitVertex () const
{
	return m_numEdgesHit == 2;
}



unsigned int CLayeredNavMeshRegion::SelectEnclosing (const Vec3 & pos, const std::vector <unsigned> & candidatePolys) const
{
	unsigned int enclosing = PolygonCont::INVALID_POLYGON_INDEX;

	unsigned int closest = PolygonCont::INVALID_POLYGON_INDEX;
	float closestDistSqr = std::numeric_limits<float>::max ();

	std::vector <unsigned int>::const_iterator hitIt = candidatePolys.begin ();
	std::vector <unsigned int>::const_iterator hitEnd = candidatePolys.end ();
	for ( ; hitIt != hitEnd; ++hitIt)
	{
			if (m_obstacleMgr->IsOverlaid (*hitIt)) continue;

			AABB polyAABB = m_polygons[*hitIt].GetAABB ();
			if (polyAABB.IsContainPoint (pos))
			{
				if (enclosing == PolygonCont::INVALID_POLYGON_INDEX)
					enclosing = *hitIt;
				else
				{
					// TODO Nov 18, 2008: <pvl> assert() just for testing to see if this
					// branch is hit frequently
					printf ("MULTIPLE MATCHES!");
					//assert (0);
				}
			}
			else
			{
				float distSqr = fabs (polyAABB.GetCenter ().z - pos.z);
				if (distSqr < closestDistSqr)
				{
					closestDistSqr = distSqr;
					closest = *hitIt;
				}
			}
	}
	// NOTE Nov 5, 2008: <pvl> can happen if 'pos' is over an obstacle - in that
	// case, there's no dynamic polygon obviously so the underlying static one
	// is returned
	if (enclosing == PolygonCont::INVALID_POLYGON_INDEX)
	{
		if (closest == PolygonCont::INVALID_POLYGON_INDEX)
			return candidatePolys.front();
		else
			return closest;
	}
	return enclosing;
}

// NOTE Nov 23, 2009: <pvl> use Newell's method to allow for nonplanar polygons
// which are commonplace in the LNM
template <typename VtxIt>
inline bool ComputeBestFitPlane (VtxIt vtxBegin, VtxIt vtxEnd, Vec3 & planeNormal, float & planeDisp)
{
	planeNormal = Vec3 (0.0f, 0.0f, 0.0f);
	VtxIt vtxIt = vtxBegin;
	VtxIt vtxNext = vtxIt;
	++vtxNext;

	// NOTE Nov 23, 2009: <pvl> single vertex is no polygon
	if (vtxNext == vtxEnd) return false;

	Vec3 centroid (0.0f, 0.0f, 0.0f);
	unsigned numVtxs = 0;

	for ( ; vtxIt != vtxEnd; ++vtxIt, ++vtxNext, ++numVtxs)
	{
		if (vtxNext == vtxEnd)
			vtxNext = vtxBegin;

		planeNormal.x += (vtxIt->y - vtxNext->y) * (vtxIt->z + vtxNext->z);
		planeNormal.y += (vtxIt->z - vtxNext->z) * (vtxIt->x + vtxNext->x);
		planeNormal.z += (vtxIt->x - vtxNext->x) * (vtxIt->y + vtxNext->y);

		// ATTN Nov 23, 2009: <pvl> more accurate computation would sum one-nths
		// of polygon vertices but a) we don't have the n until we finish the loop,
		// and b) it would be slower.  As this is a sum (as opposed to multiplication)
		// it's unlikely to get into trouble very fast and this could be a good trade-off.
		centroid += *vtxIt;
	}

	centroid /= float (numVtxs);

	planeDisp = -centroid * planeNormal;

	return true;
}

template <typename PolygonT>
inline float DistancePoint_NonPlanarPolygon3DSq (const Vec3 & pos, const PolygonT & poly)
{
	Vec3 planeNormal (0.0f, 0.0f, 0.0f);
	float planeDisp (0.0f);

	if ( ! ComputeBestFitPlane (poly.begin(), poly.end(), planeNormal, planeDisp))
		return std::numeric_limits<float>::max();

	// NOTE Nov 23, 2009: <pvl> project 'pos' to the plane of the polygon
	Vec3 posProj = pos - ( (pos*planeNormal + planeDisp) / (planeNormal*planeNormal) ) * planeNormal;

	// NOTE Nov 23, 2009: <pvl> now project everything top-down and apply 2D algo.
	// The polygon here is a navigation one so it's pretty much guaranteed that
	// z is the best axis to project along - or at least that it's not too bad.
	Vec3 closestPtInPoly;
	Distance::Point_Polygon2DSq (posProj, poly, closestPtInPoly);

	// NOTE Nov 23, 2009: <pvl> bring 'closestPtInPoly' back to the plane of the polygon
	closestPtInPoly.z = (-planeDisp - planeNormal.x*closestPtInPoly.x - planeNormal.y*closestPtInPoly.y) / planeNormal.z;

	return (pos - closestPtInPoly).len2();
}


// ---

CLayeredNavMeshRegion::CLayeredNavMeshRegion (CGraph * graph) :
		m_pGraph (graph), m_id (std::numeric_limits<unsigned>::max()), m_obstacleMgr (0),
		m_agentTypeId (-1), m_servicedVolume (AABB::RESET)
#ifdef DEBUG_DRAW
// NOTE Nov 18, 2008: <pvl> gets rid of the "'this' : used in base member
// initializer list" warning.  It's pointless here since 'DebugDraw' does nothing
// with the pointer in its constructor.
#pragma warning (disable : 4355)
		, DebugDraw(this)
#pragma warning (default : 4355)
#endif // DEBUG_DRAW
{
}

CLayeredNavMeshRegion::~CLayeredNavMeshRegion ()
{
	Clear ();
}

unsigned CLayeredNavMeshRegion::GetEnclosingPolygon (const Vec3 &pos) const
{
	FRAME_PROFILER ("GetEnclosingPolygon", gEnv->pSystem, PROFILE_AI)

	// TODO Aug 27, 2008: <pvl> should this check be even performed here?  Is it
	// not a sign of a higher-level screw-up when someone calls this function on
	// an LNM nav region instance that isn't properly initialized?
	if (m_pickingAccel.get () == 0)
		return PolygonCont::INVALID_POLYGON_INDEX;

	std::vector <unsigned int> hits = m_pickingAccel->Pick (pos);
	if (hits.empty ())
		return PolygonCont::INVALID_POLYGON_INDEX;

	return SelectEnclosing (pos, hits);
}

unsigned CLayeredNavMeshRegion::GetNearestPolygon (const Vec3 & pos, float range) const
{
	FRAME_PROFILER ("GetNearestPolygon", gEnv->pSystem, PROFILE_AI)

	const AABB rangeAABB (pos - Vec3 (range,range,0.0f), pos + Vec3 (range,range,0.0f));
	std::vector <unsigned> candidatePolys;

	m_pickingAccel->Pick (rangeAABB).swap (candidatePolys);

	if (candidatePolys.empty ())
		return PolygonCont::INVALID_POLYGON_INDEX;

	unsigned nearestPoly = PolygonCont::INVALID_POLYGON_INDEX;
	float nearestDistSqrd = std::numeric_limits<float>::max();

	std::vector<unsigned>::const_iterator candIt = candidatePolys.begin ();
	const std::vector<unsigned>::const_iterator candEnd = candidatePolys.end ();
	for ( ; candIt != candEnd; ++candIt)
	{
		LayeredNavMesh::PolygonCont::Polygon candPoly = m_polygons[*candIt];
		float distSqrd = DistancePoint_NonPlanarPolygon3DSq (pos, candPoly);

		if (distSqrd < nearestDistSqrd)
		{
			nearestDistSqrd = distSqrd;
			nearestPoly = *candIt;
		} 
	}
	return nearestPoly;
}

// Range is provided to allow compatibility with old code if (range < 0).
unsigned CLayeredNavMeshRegion::GetEnclosingGraphNode (const Vec3 &pos, float range) const
{
	unsigned polygonIndex = GetEnclosingPolygon (pos);

	// If no true enclosing polygon found and range is non-zero
	if (polygonIndex == PolygonCont::INVALID_POLYGON_INDEX && range != 0.0f)
	{
		const float range = gAIEnv.CVars.lnmGetEnclosingExtraRange;
		polygonIndex = range == 0.0f ? PolygonCont::INVALID_POLYGON_INDEX : GetNearestPolygon (pos, range);
	}
	
	return polygonIndex != PolygonCont::INVALID_POLYGON_INDEX ? m_graphNodes [polygonIndex] : 0;
}

unsigned CLayeredNavMeshRegion::GetEnclosing (
			const Vec3 &pos, float passRadius, unsigned startIndex, float range,
			Vec3 * closestValid, bool returnSuspect, const char *requesterName)
{
	return GetEnclosingGraphNode (pos, range);
}

float CLayeredNavMeshRegion::DistanceToObstacle (const Vec3 & pt, const Vec3 & dir) const
{
	FRAME_PROFILER ("DistanceToObstacle", gEnv->pSystem, PROFILE_AI)

	// ATTN Nov 25, 2009: <pvl> make sure to use the function here that doesn't
	// return the nearest!!
	const unsigned enclosing = GetEnclosingPolygon (pt);
	if (enclosing == PolygonCont::INVALID_POLYGON_INDEX)
		return 0.0f;		// NOTE Nov 25, 2009: <pvl> we are within an obstacle

	std::vector <unsigned> polysToCheck;
	polysToCheck.reserve (8);
	polysToCheck.push_back (enclosing);

	while ( ! polysToCheck.empty ())
	{
		const unsigned polyIdx = polysToCheck.back ();
		polysToCheck.pop_back ();

		const LayeredNavMesh::PolygonCont::Polygon poly = m_polygons[polyIdx];

		RayConvexPolyIntersection2D intersection (poly.begin(), poly.end(), pt, dir);
		if (intersection)
		{
			const unsigned numPolyVtxs = poly.NumVertices();
			unsigned numEdgesToProcess = 0;
			unsigned edgesToProcess[2];

			const unsigned isectdEdgeEndVtx = intersection.GetIntersectedEdge();
			// NOTE Dec 2, 2009: <pvl> edge is identified by its start (source) vertex
			edgesToProcess[numEdgesToProcess++] = (isectdEdgeEndVtx - 1 + numPolyVtxs) % numPolyVtxs;
			if (intersection.HitVertex ())
			{
				// NOTE Dec 2, 2009: <pvl> check the following edge as well
				edgesToProcess[numEdgesToProcess++] = isectdEdgeEndVtx;
			}

			for (unsigned e=0; e < numEdgesToProcess; ++e)
			{
				const unsigned edge = edgesToProcess[e];
				const unsigned edgeStartVtx = edge;
				const unsigned edgeEndVtx = (edge + 1) % numPolyVtxs;

				// NOTE Dec 2, 2009: <pvl> so far we've been referring to edges and
				// vertices by their local index, "local" meaning "within this polygon"
				// (e.g. for a polygon of 6 vertices this index runs from 0 to 5).
				// To get the neighbour polygon however we need to translate those
				// to mesh-wide unique vertex indices.
				unsigned endPt[2];
				endPt[0] = poly.GetVtxIndex (edgeStartVtx);
				endPt[1] = poly.GetVtxIndex (edgeEndVtx);
				if (endPt[0] > endPt[1]) std::swap (endPt[0], endPt[1]);
				const unsigned neighPoly = m_obstacleMgr->GetStaticNeighbourFromGraph (polyIdx, endPt[0], endPt[1]);

				if (neighPoly != DynamicOverlay::INVALID_NEIGHBOR)
				{
					polysToCheck.push_back (neighPoly);
				}
				else
				{
					// NOTE Dec 1, 2009: <pvl> hard edge - get intersection and be done with it
					const Lineseg ray (pt, pt+dir);
					const Lineseg edge (poly[edgeStartVtx], poly[edgeEndVtx]);
					float ray_t, edge_t;
					// NOTE Dec 1, 2009: <pvl> ignore the return value - one of the
					// linesegs is really a ray so the function could get confused.
					// Also, intersection should be ensured by RayConvexPolyIntersection2D.
					Intersect::Lineseg_Lineseg2D (ray, edge, ray_t, edge_t);
			//		assert (edge_t > 0.0f && edge_t < 1.0f);
			//		assert (ray_t >= 0.0f);
					return ray_t;
				}
			}
		}
	}

	// NOTE Dec 1, 2009: <pvl> obviously this isn't really expected to happen
	return std::numeric_limits<float>::max();
}


void CLayeredNavMeshRegion::BeautifyPath(
					const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
					const Vec3& startPos, const Vec3& startDir, 
					const Vec3& endPos, const Vec3 & endDir,
					float radius,
					const AgentMovementAbility & movementAbility,
					const NavigationBlockers& navigationBlockers)
{
	if (inPath.size () < 2)
	{
		outPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, startPos));
		outPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, endPos));
		return;
	}

	SimplePathBeautifier beautifier (startPos, endPos, m_polygons);

	// NOTE Jan 11, 2010: <pvl> for comments, see the corresponding parts of the other overload
	unsigned startGraphNodeIdx = GetEnclosing (startPos);
	VectorConstNodeIndices::const_iterator realBeginIt = std::find(inPath.begin(), inPath.end(), startGraphNodeIdx);

	// If we found the start position graph node to use for beautification
	if (realBeginIt != inPath.end())
	{
		beautifier.Beautify(realBeginIt, inPath.end()).swap(outPath);
	}
	else	// Start position is no longer on the calculated path
		{
		// TODO: Handle gracefully...
		}
}
void CLayeredNavMeshRegion::BeautifyPath (
			const VectorConstNodeIndices& inPath, TPathPoints& outPath,
			const Vec3& startPos, const Vec3& endPos)
{
	if (inPath.size () < 2)
	{
		outPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, startPos));
		outPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, endPos));
		return;
	}

	// NOTE Dec 8, 2009: <pvl> if pathfind request was issued for an agent that's
	// already moving it can happen that by the time the request processing gets
	// here the agent has already left the node it was in at the time it issued the request.
	// In that case, we look along the computed path for the node the agent is in now.
	// If we find it we beautify just the part of the path that's still relevant.
	// Otherwise, we do nothing and we are in a bit of a pickle since I don't see
	// a good way of letting the caller know via the inherited BeautifyPath() signature
	// that no beautification took place.
	//
	// TODO Dec 8, 2009: <pvl> this costs an additional GetEnclosing().  Caller
	// might be in a position to avoid this by making sure 'startPos' and the first
	// node of 'inPath' match.
	unsigned startGraphNodeIdx = GetEnclosing (startPos);
	VectorConstNodeIndices::const_iterator it = inPath.begin ();
	const VectorConstNodeIndices::const_iterator end = inPath.end ();
	for ( ; it != end; ++it)
		if (*it == startGraphNodeIdx)
			break;

	if (it == end) return;

	while (it != end)
	{
		// NOTE Dec 8, 2009: <pvl> skip non-LNM
		for ( ; it != end; ++it)
		{
			GraphNode * node = m_pGraph->GetNodeManager().GetNode(*it);
			if (node->navType == IAISystem::NAV_LAYERED_NAV_MESH)
				break;
		}

		if (it == end) return;

		VectorConstNodeIndices::const_iterator lnmEnd = it;
		for ( ; lnmEnd != end; ++lnmEnd)
		{
			GraphNode * node = m_pGraph->GetNodeManager().GetNode(*lnmEnd);
			if (node->navType != IAISystem::NAV_LAYERED_NAV_MESH)
				break;
		}

		TPathPoints beautified;
		Vec3 segmentStartPos (startPos);
		if ( ! outPath.empty())
		{
			// NOTE Jan 12, 2010: <pvl> the only known way ATM for an LNM path
			// to contain non-LNM nodes is that the non-LNM nodes are smart object
			// nodes.  We use the last non-LNM (read: smart object) node as the
			// start pos for this path segment under the assumption that, first,
			// this is where the smart object leaves the character, and second,
			// that position is within the LNM node immediately following it along
			// the path.
			VectorConstNodeIndices::const_iterator lastNonLnm (it);
			--lastNonLnm;
			segmentStartPos = m_pGraph->GetNodeManager().GetNode(*lastNonLnm)->GetPos();
		}

		Vec3 segmentEndPos (endPos);
		if (lnmEnd != end)
			segmentEndPos = m_pGraph->GetNodeManager().GetNode(*lnmEnd)->GetPos();

		// NOTE Jan 12, 2010: <pvl> as it makes no sense to invoke the beautifier
		// for paths where both start and end lie within the same node, the beautifier
		// expects that the path passed to it has at least two nodes
		if (lnmEnd - it < 2)
		{
			outPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, segmentStartPos));
			outPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, segmentEndPos));
		}
		else
		{
			SimplePathBeautifier beautifier (segmentStartPos, segmentEndPos, m_polygons);
			beautifier.Beautify (it,lnmEnd).swap (beautified);
			outPath.insert (outPath.end(), beautified.begin(), beautified.end());
			if (lnmEnd != end)
				outPath.push_back (PathPointDescriptor (IAISystem::NAV_LAYERED_NAV_MESH, m_pGraph->GetNodeManager().GetNode(*lnmEnd)->GetPos()));

			beautifier.DebugDraw ();
		}

		it = lnmEnd;
	}
}

bool CLayeredNavMeshRegion::CheckPassability(const Vec3& from, const Vec3& to, float radius, const NavigationBlockers& navigationBlockers, IAISystem::tNavCapMask ) const
{
	// (MATT) This is obviously a dumb implementation, but it should work fine for now {2009/06/19}
	ListPositions boundary;
	return CheckWalkability(from, to, 0.0f, false, boundary, AICE_ALL);
}

void CLayeredNavMeshRegion::AddDynamicObstacle (const ObstaclePolygon * s)
{
	if (m_obstacleMgr.get () == 0) return;

	m_obstacleMgr->AddObstacle (s);
}

void CLayeredNavMeshRegion::RemoveDynamicObstacle (const ObstaclePolygon * s)
{
	if (m_obstacleMgr.get () == 0) return;

	m_obstacleMgr->RemoveObstacle (s);
}

#ifdef _DEBUG
void CLayeredNavMeshRegion::AddDynamicObstacle (const SShape * s)
{
	if (m_obstacleMgr.get () == 0) return;

	m_dbgPathBlockerMgr->AddPathBlocker (s);
}

void CLayeredNavMeshRegion::RemoveDynamicObstacle (const SShape * s)
{
	if (m_obstacleMgr.get () == 0) return;

	m_dbgPathBlockerMgr->RemovePathBlocker (s);
}
#endif // _DEBUG

void CLayeredNavMeshRegion::ProcessDynamicObstacles ()
{
	if (m_obstacleMgr.get () == 0) return;

	m_obstacleMgr->ProcessObstacles ();
}

void CLayeredNavMeshRegion::ResetDynamicObstacleHandling ()
{
	m_obstacleMgr.reset (new DynamicObstacleMgr (m_pGraph, & m_polygons, & m_graphNodes, m_pickingAccel.get ()));
}

bool CLayeredNavMeshRegion::ContainsPos (const Vec3 & pos) const
{
	return m_servicedVolume.IsContainPoint (pos);
}


void CLayeredNavMeshRegion::Clear ()
{
	// NOTE Aug 5, 2008: <pvl> flush our nodes from the graph
	// UPDATE Sep 25, 2008: <pvl> disabling this since LNM file is getting out
	// of sync with graph .bai file way too easily and often.  When that happens
	// 'm_pGraph' is called to Disconnect() a node that's not there the program
	// crashes.  Clearing via Graph::Clear() looks more appealing and makes more
	// sense even - at least until LNM gets a new graph structure let the current
	// Graph take care of its nodes.








	IAISystem::tNavCapMask ourType (IAISystem::NAV_LAYERED_NAV_MESH);
	ourType.SetLnmCaps (ModifierId());
	m_pGraph->Clear (ourType);

	m_polygons.Clear ();
	m_graphNodes.Clear ();
	delete m_pickingAccel.release();
}

void CLayeredNavMeshRegion::Reset (IAISystem::EResetReason reason)
{
	// NOTE Dec 11, 2008: <pvl> I'm not quite sure what the desired semantics
	// of Reset(RESET_INTERNAL_LOAD) is but this seems to do the job
	if (reason == IAISystem::RESET_INTERNAL_LOAD)
		Clear ();
}

bool CLayeredNavMeshRegion::ReadFromFile (const char * fname)
{
	ExportFormat::FileReader reader;

	// NOTE Feb 16, 2009: <pvl> not storing scene bounds in members since they
	// aren't actually needed ATM after picking accelerator has been set up
	Vec3 sceneMin, sceneMax;
	ExportFormat::VersionSwitchChunkReader * sceneBoundsVersionSwitch = new ExportFormat::VersionSwitchChunkReader;
	sceneBoundsVersionSwitch->RegisterReader (1, new SceneBoundsChunkReader (sceneMin, sceneMax));
	reader.RegisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_SCENE_BOUNDS, sceneBoundsVersionSwitch);

	ExportFormat::VersionSwitchChunkReader * modifInfoVersionSwitch = new ExportFormat::VersionSwitchChunkReader;
	modifInfoVersionSwitch->RegisterReader (1, new ModifierInfoChunkReader (m_navModifName, m_id));
	modifInfoVersionSwitch->RegisterReader (2, new ModifierInfoChunkReader_v2 (m_navModifName, m_id, m_agentTypeId, m_agentTypeName));
	reader.RegisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_MODIFIER_INFO, modifInfoVersionSwitch);

	m_polygons.SetupReadingFromFile ( & reader);

	bool success = reader.Read (fname);

	// ---

	m_polygons.CleanupReadingFromFile ( & reader);

	delete reader.UnregisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_SCENE_BOUNDS);
	delete reader.UnregisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_MODIFIER_INFO);

	if ( ! success)
		return false;

	m_graphNodes.SetupStaticGraphNodes (m_pGraph, m_polygons.GetPolyCount (), m_id, m_agentTypeId);

	m_servicedVolume.Add (sceneMin);
	m_servicedVolume.Add (sceneMax);

	SetupPickingAccelerator (sceneMin, sceneMax);
	// NOTE Sep 24, 2008: <pvl> now we have our picking accelerator, we can construct
	// our dynamic obstacle manager as well
	m_obstacleMgr.reset (new DynamicObstacleMgr (m_pGraph, & m_polygons, & m_graphNodes, m_pickingAccel.get ()));
	m_pathBlockerMgr.reset (new PathBlockingEntityMgr (m_obstacleMgr.get ()));
#ifdef _DEBUG
	m_dbgPathBlockerMgr.reset (new DbgPathBlockingEntityMgr (m_obstacleMgr.get ()));
#endif // _DEBUG

	return success;
}

void CLayeredNavMeshRegion::SetupPickingAccelerator (const Vec3 & sceneMin, const Vec3 & sceneMax)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "LNM - Picking Accelerator");

	// FIXME Jan 28, 2010: <pvl> make the node size a cvar
	QuadtreePickingAccelerator<RuntimePolygonContext> * acc = new QuadtreePickingAccelerator<RuntimePolygonContext> (
			sceneMin, sceneMax, 3.0f, RuntimePolygonContext ( & m_polygons)
	);
	m_pickingAccel.reset (acc);

	unsigned int polyCount = m_polygons.GetPolyCount ();

	for (unsigned int i=0; i < polyCount; ++i)
	{
		m_pickingAccel->Add (i);
	}

	acc->FreeUnusedMemory ();

	QuadtreePickingAccelerator<RuntimePolygonContext>::Stats stats = acc->CollectStats ();
	stats.Print ();
}

#ifdef DEBUG_DRAW
void CLayeredNavMeshRegion::DebugDrawMgr::operator() () const
{
	m_owner->m_polygons.DebugDraw ();
	if (m_owner->m_pickingAccel.get ())
		m_owner->m_pickingAccel->DebugDraw ();
	if (m_owner->m_pathBlockerMgr.get ())
		m_owner->m_pathBlockerMgr->DebugDraw ();
}

class PolylineLengthAccumulator {
	Vec3 m_prev;
	float m_lenSoFar;
public:
	PolylineLengthAccumulator (const Vec3 init_vtx) :
			m_prev (init_vtx), m_lenSoFar (0.0f)
	{ }
	void AddVertex (const Vec3 & vertex)
	{
		m_lenSoFar += (vertex - m_prev).len ();
		m_prev = vertex;
	}
	float LengthSoFar () const { return m_lenSoFar; }
};

void CLayeredNavMeshRegion::DebugDrawMgr::operator() (const CAStarSolver::tPathNodes & pathNodes) const
{
	CDebugDrawContext dc;
	const ColorB col (255,64,0);
	const Vec3 vertOffset (0.0f, 0.0f, 0.7f);

	GraphNode * node = gAIEnv.pGraph->GetNodeManager().GetNode (pathNodes.front());
	Vec3 pos = node->GetPos ();

	PolygonAlongPath (node->GetLayeredMeshNavData()->polygonIndex, 0);
	dc->DrawSphere (pos + vertOffset, 0.5f, col);

	PolylineLengthAccumulator distFromStart (pos);
	unsigned numNodes = pathNodes.size ();
	for (unsigned i=1; i < numNodes; ++i)
	{
		node = gAIEnv.pGraph->GetNodeManager().GetNode (pathNodes[i]);
		dc->DrawLine (pos + vertOffset, col, node->GetPos() + vertOffset, col);
		pos = node->GetPos();

		if (node->navType == IAISystem::NAV_LAYERED_NAV_MESH)
			PolygonAlongPath (node->GetLayeredMeshNavData()->polygonIndex, i);

		dc->DrawSphere (pos + vertOffset, 0.5f, col);

		distFromStart.AddVertex (pos);
		dc->Draw3dLabelEx (pos + vertOffset, 2.5f, ColorB(0, 0, 0), false, true,
				"%.1f", distFromStart.LengthSoFar ()
		);
	}
}

void CLayeredNavMeshRegion::DebugDrawMgr::PolygonAlongPath (unsigned int polyIndex, unsigned numberAlongPath) const
{
	// NOTE Aug 6, 2008: <pvl> multiply 'numberAlongPath' by 2 so that neighboring
	// polys don't get neighboring (e.g. too close to each other) colors
	unsigned char g = 128 + 15*((numberAlongPath*2)%8);
	m_owner->m_polygons.DebugDrawPolygon (polyIndex, ColorB (255, g, 96, 255), 0.6f);
}

void CLayeredNavMeshRegion::DebugDrawMgr::GetEnclosingTest (const Vec3 & pos) const
{
	unsigned int mrdPolyIndex = m_owner->GetEnclosingPolygon (pos);

	if (mrdPolyIndex == PolygonCont::INVALID_POLYGON_INDEX)
	{
		const float range = gAIEnv.CVars.lnmGetEnclosingExtraRange;
		mrdPolyIndex = range == 0.0f ? PolygonCont::INVALID_POLYGON_INDEX : m_owner->GetNearestPolygon (pos, range);
	}

	if (mrdPolyIndex == PolygonCont::INVALID_POLYGON_INDEX) return;

	// draw it highlighted
	ColorB col (255,255,255,224);
	m_owner->m_polygons.DebugDrawPolygon (mrdPolyIndex, col, 0.8f);

	// draw its polygon and/or graph node index
	CDebugDrawContext dc;
	dc->Draw3dLabelEx (pos, 1.5f, ColorB(0, 0, 0), true, true,
			"Polygon: %d\nNode: %d", mrdPolyIndex, m_owner->m_graphNodes[mrdPolyIndex]
	);

	// draw vertex indices
	PolygonCont::Polygon poly = m_owner->m_polygons[mrdPolyIndex];

	PolygonCont::Polygon::vertex_index_iterator vertIndexIt = poly.index_begin ();
	PolygonCont::Polygon::vertex_index_iterator vertIndexEnd = poly.index_end ();
	for ( ; vertIndexIt != vertIndexEnd; ++vertIndexIt)
	{
		Vec3 vtxPos = m_owner->m_polygons.GetVertex (*vertIndexIt);
		dc->Draw3dLabelEx (vtxPos, 1.5f, ColorB(0, 0, 0), true, true,
				"V:%d\n", *vertIndexIt
		);
	}
}

void CLayeredNavMeshRegion::DebugDrawMgr::AreaOverlapTest (const AABB & area) const
{
	// TODO Sep 18, 2008: <pvl> should this check be even performed here?  Is it
	// not a sign of a higher-level screw-up when someone calls this function on
	// an LNM nav region instance that isn't properly initialized?
	if (m_owner->m_pickingAccel.get () == 0)
		return;

	std::vector <unsigned int> hits = m_owner->m_pickingAccel->Pick (area);

	const ColorB col (255,255,255,128);
	std::vector <unsigned int>::const_iterator polyIt = hits.begin ();
	std::vector <unsigned int>::const_iterator polyEnd = hits.end ();
	for ( ; polyIt != polyEnd; ++polyIt)
	{
		m_owner->m_polygons.DebugDrawPolygon (*polyIt, col, 0.3f);
	}
}

void CLayeredNavMeshRegion::DebugDrawMgr::MeshCuttingTest (const SShape * shape) const
{
	// TODO Sep 18, 2008: <pvl> should this check be even performed here?  Is it
	// not a sign of a higher-level screw-up when someone calls this function on
	// an LNM nav region instance that isn't properly initialized?
	if (m_owner->m_pickingAccel.get () == 0)
		return;

	const unsigned memPoolSize = 10 * 1024;
	_smart_ptr<PoolAllocator> alloc ( new PoolAllocator (memPoolSize) );

	std::vector <unsigned int> hits = m_owner->m_pickingAccel->Pick (shape->aabb);
	std::vector <unsigned int>::const_iterator polyIt = hits.begin ();
	std::vector <unsigned int>::const_iterator polyEnd = hits.end ();
	for ( ; polyIt != polyEnd; ++polyIt)
	{
		Polygon * p0 = Convert (0, m_owner->m_polygons[*polyIt], alloc);
		Polygon * p1 = Convert (1, *shape, alloc);

		PolygonSetOp op (p0, p1, alloc);
		DebugDrawResultPolygon (op.Difference ());

		alloc->Clear ();
	}
}

void CLayeredNavMeshRegion::DebugDrawMgr::RaycastTest (const Vec3 & pos) const
{
	const unsigned numRays = 16;

	for (unsigned r=0; r < numRays; ++r)
	{
		const float rayAngle = r * 2*gf_PI / numRays;
		const Vec3 rayDir (cry_cosf (rayAngle), cry_sinf (rayAngle), 0.0f);
		const float dist = m_owner->DistanceToObstacle (pos, rayDir);
		assert (dist < std::numeric_limits<float>::max());

		CDebugDrawContext dc;
		dc->DrawLine (pos, ColorB(192,255,192), pos + dist * rayDir, ColorB(192,255,192), 5.0f);
	}
}


#endif // DEBUG_DRAW

// TODO Jul 25, 2008: <pvl> consider refactoring so that it takes IRenderer*
// as a parameter - then the code could be the same for both editor & runtime
/**
* Draw AABB's of tree nodes.
*/
#ifdef DEBUG_DRAW
void QuadtreePickingAcceleratorBase::Node::DebugDraw (int level) const
{
	ColorB col (level*32, level*32, level*32);

	// NOTE Jul 16, 2008: <pvl> basically, draw node AABB but put the root below
	// the scene (even below zero - there still might be something drawn at 0)
	// and offset each level a bit up from the previous one (at least to prevent
	// z-fighting).  Also give the boxes some thickness so that they don't
	// disappear when looked at from the side.
	AABB aabb (Vec3 (m_aabb.min), Vec3 (m_aabb.max));
	aabb.Move (Vec3 (0.0f, 0.0f, -30.0f + level*1.5f));
	aabb.max.z += 0.1f;

	CDebugDrawContext dc;
	dc->DrawAABB (aabb, true, col, eBBD_Faceted);

	for (int i=0; i<4; ++i)
		if (m_children[i])
			m_children[i]->DebugDraw (level+1);
}
#endif


Lineseg CLayeredNavMeshRegion::GetSharedEdge (unsigned poly0, unsigned poly1) const
{
	CommonPolyVertices commonVerts (m_polygons[poly0], m_polygons[poly1]);
	assert (commonVerts.Count() == 2);

	return Lineseg (m_polygons.GetVertex (commonVerts[0]), m_polygons.GetVertex (commonVerts[1]));
}



// ---

CCompositeLayeredNavMeshRegion::CCompositeLayeredNavMeshRegion (CGraph * graph) :
		m_graph (graph)
{
}

CCompositeLayeredNavMeshRegion::~CCompositeLayeredNavMeshRegion ()
{
	for (int i=0, ni=m_regions.size (); i < ni; ++i)
		delete m_regions[i];
}

// ATTN Aug 10, 2009: <pvl> gets the first one of matching agent type that claims
// to be able to service 'pos'.  At this moment, if there's more than one
// no attempt is made at arbitrating.
// NOTE Oct 26, 2009: <pvl> GetModifier() functions are const conceptually but
// have to return an ordinary iterator as having both const and non-const versions
// would lead to duplication and general ugliness.  Thus the const_cast.
std::vector<CLayeredNavMeshRegion*>::iterator CCompositeLayeredNavMeshRegion::GetModifier (const Vec3 & pos, unsigned agentType) const
{
	CCompositeLayeredNavMeshRegion * self = const_cast<CCompositeLayeredNavMeshRegion*> (this);
	std::vector <CLayeredNavMeshRegion*>::iterator it = self->m_regions.begin ();
	const std::vector <CLayeredNavMeshRegion*>::iterator end = self->m_regions.end ();
	for ( ; it != end; ++it)
	{
		if ((*it)->AgentTypeId () != agentType)
			continue;
		if ((*it)->ContainsPos (pos))
			return it;
	}
	return end;
}

// NOTE Oct 9, 2009: <pvl> in case you're wondering, the following overloads are
// linear searches because
// a) they're not expected to be executed often (essentially just while loading)
// b) the context they should be executed in (loading) involves delays orders of
//    magnitude bigger
std::vector<CLayeredNavMeshRegion*>::iterator CCompositeLayeredNavMeshRegion::GetModifier (unsigned modifIndex, unsigned agentType) const
{
	CCompositeLayeredNavMeshRegion * self = const_cast<CCompositeLayeredNavMeshRegion*> (this);
	std::vector <CLayeredNavMeshRegion*>::iterator it = self->m_regions.begin ();
	const std::vector <CLayeredNavMeshRegion*>::iterator end = self->m_regions.end ();
	for ( ; it != end; ++it)
	{
		if ((*it)->ModifierId () == modifIndex && (*it)->AgentTypeId () == agentType)
			return it;
	}
	return end;
}

std::vector<CLayeredNavMeshRegion*>::iterator CCompositeLayeredNavMeshRegion::GetModifier (const char * navModifName, const char * agentTypeName) const
{
	CCompositeLayeredNavMeshRegion * self = const_cast<CCompositeLayeredNavMeshRegion*> (this);
	std::vector <CLayeredNavMeshRegion*>::iterator it = self->m_regions.begin ();
	const std::vector <CLayeredNavMeshRegion*>::iterator end = self->m_regions.end ();
	for ( ; it != end; ++it)
	{
		if ((*it)->ModifierName () == navModifName && (*it)->AgentTypeName () == agentTypeName)
			return it;
	}
	return end;
}

CLayeredNavMeshRegion * CCompositeLayeredNavMeshRegion::Check (std::vector <CLayeredNavMeshRegion*>::iterator it) const
{
	if (it == m_regions.end ())
		return 0;
	return *it;
}



void CCompositeLayeredNavMeshRegion::BeautifyPath(
	const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
	const Vec3& startPos, const Vec3& startDir, 
	const Vec3& endPos, const Vec3 & endDir,
	float radius,
	const AgentMovementAbility & movementAbility,
	const NavigationBlockers& navigationBlockers)
{
	if (inPath.empty())
		return;

	GraphNode * node = m_graph->GetNodeManager().GetNode (inPath[0]);
	// NOTE Jan 11, 2010: <pvl> this function assumes that 'inPath' consist
	// solely of nodes of the LNM type, see comment in the header
	assert (node->navType == IAISystem::NAV_LAYERED_NAV_MESH);

	const unsigned modifIndex = node->GetLayeredMeshNavData()->navModifIndex;
	const unsigned agentType = node->GetLayeredMeshNavData()->agentType;

	CLayeredNavMeshRegion * reg = Check (GetModifier (modifIndex, agentType));

	assert (reg);
	if (reg == 0)
	{
		// NOTE Aug 10, 2009: <pvl> warn?
		return;
	}

	reg->BeautifyPath (inPath, outPath, startPos, startDir, endPos, endDir, radius, movementAbility, navigationBlockers);
}

void CCompositeLayeredNavMeshRegion::BeautifyPath (const VectorConstNodeIndices& inPath, TPathPoints& outPath,
		const Vec3& startPos, const Vec3& endPos)
{
	if (inPath.empty())
		return;

	GraphNode * node = m_graph->GetNodeManager().GetNode (inPath[0]);
	if (node->navType != IAISystem::NAV_LAYERED_NAV_MESH)
	{
		// TODO Aug 10, 2009: <pvl> log error?  Or can this happen legally?
		return;
	}

	const unsigned modifIndex = node->GetLayeredMeshNavData()->navModifIndex;
	const unsigned agentType = node->GetLayeredMeshNavData()->agentType;

	CLayeredNavMeshRegion * reg = Check (GetModifier (modifIndex, agentType));

	assert (reg);
	if (reg == 0)
	{
		// NOTE Aug 10, 2009: <pvl> warn?
		return;
	}

	reg->BeautifyPath (inPath, outPath, startPos, endPos);
}

bool CCompositeLayeredNavMeshRegion::CheckPassability(const Vec3& from, const Vec3& to, float radius, const NavigationBlockers& navigationBlockers, IAISystem::tNavCapMask navCapMask) const
{
	unsigned agentType = navCapMask.GetLnmCaps ();

	CLayeredNavMeshRegion * reg = Check (GetModifier (from, agentType));

	// NOTE Aug 10, 2009: <pvl> different modifiers don't know about each other
	if (reg == 0 || reg != Check (GetModifier (to, agentType)))
		return false;

	return reg->CheckPassability (from, to, radius, navigationBlockers, navCapMask);
}

unsigned CCompositeLayeredNavMeshRegion::GetEnclosing (const Vec3 & pos, int agentType, float range)
{
	CLayeredNavMeshRegion * reg = Check (GetModifier (pos, agentType));

	if (reg == 0)
		return 0;

	return reg->GetEnclosing (pos, 0.0f, 0, range);
}

unsigned CCompositeLayeredNavMeshRegion::GetEnclosing(const Vec3 &pos, float passRadius, unsigned startIndex,
	float range, Vec3 * closestValid, bool returnSuspect, const char *requesterName)
{
	assert (0);
	return 0;
}

void CCompositeLayeredNavMeshRegion::Clear()
{
	// NOTE Feb 17, 2009: <pvl> it's impossible to make sure what Clear() is
	// supposed to do but anyway, with LNM just getting rid of the current
	// instances and loading new ones seems to be the right thing to do
	for (int i=0, ni=m_regions.size (); i < ni; ++i)
		delete m_regions[i];
	m_regions.resize (0);









}

void CCompositeLayeredNavMeshRegion::Reset (IAISystem::EResetReason reason)
{
	std::vector <CLayeredNavMeshRegion*>::iterator it = m_regions.begin ();
	const std::vector <CLayeredNavMeshRegion*>::iterator end = m_regions.end ();
	for ( ; it != end; ++it)
	{
		(*it)->Reset (reason);
	}
}

// Reads navigation from binary file, returns true on success.
bool CCompositeLayeredNavMeshRegion::ReadFromFile (const char * fname, bool demandLoading)
{
	if (demandLoading)
		return InitDemandLoading (fname);

	bool lnmOK = false;
	for (int i=0; i < 10000/*arbitrary*/; ++i)
	{
		char fileNameLNM[1024];
		_snprintf (fileNameLNM, 1024-1, "%s%d.bai", fname, i);

		// NOTE Feb 17, 2009: <pvl> this is just to check if there's a file of
		// that name.  Shame we don't seem to have a more light-weight test.
		CCryFile file;
		if ( ! file.Open (fileNameLNM, "rb") )
			break;

		// TODO Feb 17, 2009: <pvl> each LNM modifier is expected to use its own
		// graph soon so never mind this GetGraph() call
		CLayeredNavMeshRegion * lnmReg = new CLayeredNavMeshRegion (gAIEnv.pGraph);
		if (lnmReg->ReadFromFile (fileNameLNM))
			lnmOK = true;
		// NOTE Feb 17, 2009: <pvl> push even if it failed to load, helps with debugging
		m_regions.push_back (lnmReg);
	}
	return lnmOK;
}

bool ReadModifInfo (const char * fname, string & navModifName, string & agentTypeName, unsigned & modifIndex)
{
	// NOTE Oct 8, 2009: <pvl> now set up a reader that only understands the
	// modifier info chunk
	ExportFormat::FileReader reader;

	int dummy;

	ExportFormat::VersionSwitchChunkReader * modifInfoVersionSwitch = new ExportFormat::VersionSwitchChunkReader;
	modifInfoVersionSwitch->RegisterReader (1, new ModifierInfoChunkReader (navModifName, modifIndex));
	modifInfoVersionSwitch->RegisterReader (2, new ModifierInfoChunkReader_v2 (navModifName, modifIndex, dummy, agentTypeName));
	reader.RegisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_MODIFIER_INFO, modifInfoVersionSwitch);

	const bool success = reader.Read (fname);

	delete reader.UnregisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_MODIFIER_INFO);

	return success;
}

CLayeredNavMeshRegion * CCompositeLayeredNavMeshRegion::GetModifierByIndexAndType (unsigned modifIndex, unsigned agentType) const
{
	return Check (GetModifier (modifIndex, agentType));
}


// NOTE Oct 8, 2009: <pvl> return success if anything went well
bool CCompositeLayeredNavMeshRegion::InitDemandLoading (const char * fname)
{
	bool lnmOK = false;
	for (int i=0; i < 10000/*arbitrary*/; ++i)
	{
		char fileNameLNM[1024];
		_snprintf (fileNameLNM, 1024-1, "%s%d.bai", fname, i);

		// NOTE Feb 17, 2009: <pvl> this is just to check if there's a file of
		// that name.  Shame we don't seem to have a more light-weight test.
		CCryFile file;
		if ( ! file.Open (fileNameLNM, "rb") )
			break;

		string navModifName, agentTypeName;
		unsigned navModifIndex;
		const bool success = ReadModifInfo (fileNameLNM, navModifName, agentTypeName, navModifIndex);
		if ( ! success)
		{
			AIWarning ("Failed to extract info from navmesh file %s", fileNameLNM);
			continue;
		}

		lnmOK = true;

		m_meshToFname.insert (std::make_pair (navModifName+agentTypeName, MeshInfo (fileNameLNM, navModifIndex)));
	}
	return lnmOK;
}

const char * CCompositeLayeredNavMeshRegion::GetFilename (const char * navModifName, const char * agentTypeName) const
{
	MeshInfoMap::const_iterator it = m_meshToFname.find (string (navModifName) + string (agentTypeName));
	if (it == m_meshToFname.end ())
		return 0;
	return it->second.m_fileName.c_str ();
}

bool CCompositeLayeredNavMeshRegion::LoadNavMesh (const char * navModifName, const char * agentTypeName)
{
	CLayeredNavMeshRegion * existing = Check (GetModifier (navModifName, agentTypeName));
	if (existing)
		return true;

	const char * fname = GetFilename (navModifName, agentTypeName);
	if (fname == 0)
		return false;

	bool success = true;
	CLayeredNavMeshRegion * region = new CLayeredNavMeshRegion (gAIEnv.pGraph);
	if ( ! region->ReadFromFile (fname))
		success = false;

	// NOTE Oct 9, 2009: <pvl> push even if it failed to load, helps with debugging
	m_regions.push_back (region);
	return success;
}

bool CCompositeLayeredNavMeshRegion::UnloadNavMesh (const char * navModifName, const char * agentTypeName)
{
	std::vector<CLayeredNavMeshRegion*>::iterator regIt = GetModifier (navModifName, agentTypeName);
	if (Check (regIt))
	{
		delete *regIt;
		m_regions.erase (regIt);
		return true;
	}
	else
	{
		AIWarning ("Can't unload mesh (%s,%s) because it's not loaded", navModifName, agentTypeName);
		return false;
	}
}

bool CCompositeLayeredNavMeshRegion::IsNavMeshLoaded (const char * navModifName, const char * agentTypeName) const
{
	CLayeredNavMeshRegion * existing = Check (GetModifier (navModifName, agentTypeName));
	if (existing)
		return true;
	return false;
}

unsigned CCompositeLayeredNavMeshRegion::GetModifierId (const char * navModifName, const char * agentTypeName) const
{
	MeshInfoMap::const_iterator it = m_meshToFname.find (string (navModifName) + string (agentTypeName));
	if (it == m_meshToFname.end ())
		return 0;
	return it->second.m_modifIndex;
}

//
//-----------------------------------------------------------------------------------------------------------
void DebugDrawLayeredNavMeshRegion (CLayeredNavMeshRegion * lnmRegion)
{
#ifdef DEBUG_DRAW
	CDebugDrawContext dc;
	lnmRegion->DebugDraw ();

	{
		string kokot0 (string ("PathStart-") + lnmRegion->AgentTypeName ());
		string kokot1 (string ("PathEnd-") + lnmRegion->AgentTypeName ());

		IEntity* ent0 = gEnv->pEntitySystem->FindEntityByName(kokot0);
		IEntity* ent1 = gEnv->pEntitySystem->FindEntityByName(kokot1);
		if (ent0 && ent1)
		{
			Vec3 p0 = ent0->GetWorldPos();
			Vec3 p1 = ent1->GetWorldPos();

			unsigned int startIndex = lnmRegion->GetEnclosing (p0);
			unsigned int endIndex = lnmRegion->GetEnclosing (p1);
			if (startIndex && endIndex)
			{
				const CGraph * graph = gAIEnv.pGraph;
				CCalculationStopper stopper ("kokot", 1000000, 1000000);
				NavigationBlockers dummyBlockers;
				CStandardHeuristic heuristic;
				heuristic.SetStartEndData (
						graph->GetNodeManager().GetNode(startIndex), p0,
						graph->GetNodeManager().GetNode(endIndex), p1,
						PathfindingExtraConstraints()
				);

				CAStarSolver astarSolver (gAIEnv.pGraph->GetNodeManager());
				EAStarResult result = astarSolver.SetupAStar(
					stopper, gAIEnv.pGraph, & heuristic,
					startIndex, endIndex, dummyBlockers, false
				);
				while (result == ASTAR_STILLSOLVING)
				{
					result = astarSolver.ContinueAStar (stopper);
				}
				if (result == ASTAR_PATHFOUND)
				{
					CAStarSolver::tPathNodes pathNodes = astarSolver.GetPathNodes ();
					assert (pathNodes.front () == startIndex && pathNodes.back () == endIndex);
					lnmRegion->DebugDraw (pathNodes);

					TPathPoints beautifiedPath;
					lnmRegion->BeautifyPath (pathNodes, beautifiedPath, p0, p1);
					if (beautifiedPath.size () > 1)
					{
						const ColorB col (128, 255, 128, 255);
						const Vec3 verticalOffset (0.0f, 0.0f, 0.8f);
						TPathPoints::const_iterator pointIt = beautifiedPath.begin ();
						TPathPoints::const_iterator pointEnd = beautifiedPath.end ();
						dc->DrawSphere (pointIt->vPos + verticalOffset, 0.5f, col);
						Vec3 prevPos = pointIt->vPos + verticalOffset;
						for ( ; pointIt != pointEnd; ++pointIt)
						{
							dc->DrawLine (prevPos, col, pointIt->vPos + verticalOffset, col, 5.0f);
							dc->DrawSphere (pointIt->vPos + verticalOffset, 0.5f, col);
							prevPos = pointIt->vPos + verticalOffset;
						}
					}
				}
			}
		}
	}
	{
		string mrd (string ("Enclosing-") + lnmRegion->AgentTypeName ());

		IEntity * ent = gEnv->pSystem->GetIEntitySystem()->FindEntityByName (mrd);
		if (ent)
		{
			Vec3 pos = ent->GetWorldPos();
			lnmRegion->DebugDraw.GetEnclosingTest (pos);
		}
	}
	{
		string mrd (string ("Raycast-") + lnmRegion->AgentTypeName ());

		IEntity * ent = gEnv->pSystem->GetIEntitySystem()->FindEntityByName (mrd);
		if (ent)
		{
			Vec3 pos = ent->GetWorldPos();
			lnmRegion->DebugDraw.RaycastTest (pos);
		}
	}
	{
		SShape * poly = GetAISystem()->GetGenericShapeOfName ("LnmOverlapTest");
		if (poly)
		{
			lnmRegion->DebugDraw.AreaOverlapTest (poly->aabb);
			lnmRegion->DebugDraw.MeshCuttingTest (poly);
		}
	}

	static bool testDynamics = false;
	if (testDynamics) {
		static int frameCounter = 0;





















































































		//lnmRegion->ResetDynamicObstacleHandling ();
		lnmRegion->ProcessDynamicObstacles ();

		++frameCounter;
	}
#endif // DEBUG_DRAW
}

void CCompositeLayeredNavMeshRegion::DebugDraw () const
{
#ifdef DEBUG_DRAW
	// TODO Aug 20, 2009: <pvl> the way things are ATM, this variable is unlikely
	// to be available in pure game as it's created by the editor.  Not sure
	// what the clean solution is as AISystem shouldn't be required to run editor
	// either meaning that moving the var into AISystem doesn't fundamentally
	// solve the problem.
	static ICVar * agentTypeToDraw = gEnv->pConsole->GetCVar ("ai_lnmDrawAgentType");

	const bool doDynamicsUpdate = gAIEnv.CVars.lnmDynamics != 0;

	std::vector <CLayeredNavMeshRegion*>::const_iterator regIt = m_regions.begin ();
	const std::vector <CLayeredNavMeshRegion*>::const_iterator regEnd = m_regions.end ();
	for ( ; regIt != regEnd; ++regIt)
	{
		if ( ! agentTypeToDraw || agentTypeToDraw->GetString () == (*regIt)->AgentTypeName ())
			DebugDrawLayeredNavMeshRegion (*regIt);

		if (doDynamicsUpdate)
			(*regIt)->ProcessDynamicObstacles ();
	}
#endif // DEBUG_DRAW
}

