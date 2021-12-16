
#include "StdAfx.h"

#include "DynamicObstacles.h"
#include "DynamicVertex.h"
#include <limits>

#include "IAIDebugRenderer.h"

const unsigned DynamicOverlay::INVALID_NEIGHBOR = std::numeric_limits<unsigned>::max();
const unsigned short DynamicOverlay::INVALID_VTX_INDEX = std::numeric_limits<unsigned short>::max();


// ATTN Okt 14, 2008: <pvl> this is a linear search - for small number of vertices
// it's OK but watch out for degenerate cases with tons of vertices (don't know
// if they're even possible but just be careful)
unsigned short DynamicOverlay::GetVtxIndex (const VtxAttribs::Attrib * vtx) const
{
	unsigned numVtxs = m_vertices.size ();
	for (unsigned int i=0; i < numVtxs; ++i)
	{
		if ( *m_vtxAttribs[i] == *vtx )
			return i;
	}
	assert (0);
	return INVALID_VTX_INDEX;
}

unsigned short DynamicOverlay::GetVtxIndex (const Vec3 & pos) const
{
	unsigned numVtxs = m_vertices.size ();
	for (int i=0; i < numVtxs; ++i)
	{
		if (Vec2 (m_vertices[i]) == Vec2 (pos))
			return i;
	}
	assert (0);
	return INVALID_VTX_INDEX;
}

bool IsSameEdge (unsigned short e0pt0, unsigned short e0pt1, unsigned short e1pt0, unsigned short e1pt1)
{
	if (e0pt0 > e0pt1)
		std::swap (e0pt0, e0pt1);
	if (e1pt0 > e1pt1)
		std::swap (e1pt0, e1pt1);

	if (e0pt0 == e1pt0 && e0pt1 == e1pt1)
		return true;

	return false;
}

unsigned DynamicOverlay::GetPolyFromOutsideEdge (
		unsigned short endPt0Index, unsigned short endPt1Index) const
{
	// TODO Okt 14, 2008: <pvl> this is linear - it might be possible to build
	// up an index of some sort to speed this up - at the cost of some memory of course

	const unsigned numPolys = m_polygons.size ();
	for (unsigned int p=0; p < numPolys; ++p)
	{
		const DynamicOverlay::Polygon & poly = m_polygons[p];
		const unsigned numEdges = poly.m_numIndices;
		for (unsigned i=0; i < numEdges; ++i)
		{
			if (IsSameEdge (endPt0Index, endPt1Index,
				m_indices [poly.m_startIndex + i], m_indices [NextPolygonVtxIndex (p, i)]))
				return p + m_baseIndex;
		}
	}
	return INVALID_NEIGHBOR;
}

unsigned DynamicOverlay::GetPolyFromOutsideEdge (
		const VtxAttribs::Attrib * endPt0, const VtxAttribs::Attrib * endPt1) const
{
	unsigned short endPt0Index = GetVtxIndex (endPt0);
	assert (endPt0Index != INVALID_VTX_INDEX);
	unsigned short endPt1Index = GetVtxIndex (endPt1);
	assert (endPt1Index != INVALID_VTX_INDEX);

	return GetPolyFromOutsideEdge (endPt0Index, endPt1Index);
}

unsigned DynamicOverlay::GetPolyFromOutsideEdge (
		const Vec3 & endPt0, const Vec3 & endPt1) const
{
	unsigned short endPt0Index = GetVtxIndex (endPt0);
	assert (endPt0Index != INVALID_VTX_INDEX);
	unsigned short endPt1Index = GetVtxIndex (endPt1);
	assert (endPt1Index != INVALID_VTX_INDEX);

	return GetPolyFromOutsideEdge (endPt0Index, endPt1Index);
}

void DynamicOverlay::DebugDraw () const
{
	IAIDebugRenderer* pRenderer = gEnv->pAISystem->GetAIDebugRenderer();
	
	const ColorB col (64, 255, 64, 128);
	const ColorB unconnCol (16, 64, 16, 255);
	const Vec3 offset (0.0f, 0.0f, 0.5f);

	const unsigned numPolys = m_polygons.size ();
	for (unsigned int p=0; p < numPolys; ++p)
	{
		const DynamicOverlay::Polygon & poly = m_polygons[p];
		const unsigned numEdges = poly.m_numIndices;
		for (unsigned i=0; i < numEdges; ++i)
		{
			const Vec3 endPt0 = m_vertices [ m_indices [poly.m_startIndex + i] ] + offset;
			const Vec3 endPt1 = m_vertices [ m_indices [NextPolygonVtxIndex (p,i)] ] + offset;
			pRenderer->DrawSphere (endPt0, 0.1f, col);

			bool connected = m_neighbours [poly.m_startIndex + i] != INVALID_NEIGHBOR;
			const ColorB & c = connected ? col : unconnCol;		
			pRenderer->DrawLine (endPt0, c, endPt1, c);
		}
	}

	for (unsigned int v=0; v < m_vertices.size (); ++v)
	{
		m_vtxAttribs[v]->DebugDraw (m_vertices[v] + offset);
	}
}
