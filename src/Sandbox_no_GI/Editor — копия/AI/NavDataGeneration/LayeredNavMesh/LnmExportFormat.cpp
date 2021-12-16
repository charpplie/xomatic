
#include "StdAfx.h"

#include "LnmExportFormat.h"

namespace LayeredNavMesh {
namespace LnmExportFormat {

NavMeshVertexListChunk::Header::Header (CCryFile & file) :
		m_numVertices (0)
{
	// TODO Jul 3, 2008: <pvl> switch to auto types once I figure out how they work?
	//file.ReadType (this);
	file.ReadType ( & m_numVertices);
}

// ---

NavMeshPolygonListChunk::Header::Header (CCryFile & file) : m_numPolygons(0)
{
	// TODO Jul 3, 2008: <pvl> switch to auto types once I figure out how they work?
	//file.ReadType (this);
	file.ReadType ( & m_numPolygons);
	file.ReadType ( & m_numIndices);
}

void NavMeshPolygonListChunk::Header::Write (CCryFile & file) const
{
	file.Write ( & m_numPolygons, sizeof (m_numPolygons));
	file.Write ( & m_numIndices, sizeof (m_numIndices));
}

size_t NavMeshPolygonListChunk::Header::Size ()
{
	// FIXME Jul 17, 2008: <pvl> REALLY bad to hardcode types here!!!!!!!!!!!!!!!!!
	return sizeof (uint32) + sizeof (uint32);
}


void NavMeshPolygonListChunk::Polygon::Write (CCryFile & file) const
{
	file.Write ( & m_region, sizeof (m_region));
	file.Write ( & m_numIndices, sizeof (m_numIndices));
	file.Write ( & m_indices[0], m_indices.size () * sizeof (m_indices[0]));
}

// ---

NavMeshGraphNodeIndexListChunk::Header::Header (CCryFile & file) :
		m_numIndices(0)
{
	// TODO Jul 3, 2008: <pvl> switch to auto types once I figure out how they work?
	//file.ReadType (this);
	file.ReadType ( & m_numIndices);
}

} // namespace LnmExportFormat
} // namespace LayeredNavMesh
