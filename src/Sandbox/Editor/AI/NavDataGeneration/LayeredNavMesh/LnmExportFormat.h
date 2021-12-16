
#ifndef LNM_EXPORT_FORMAT_H
#define LNM_EXPORT_FORMAT_H

#include "ExportFormat.h"

class CCryFile;

namespace LayeredNavMesh {
namespace LnmExportFormat {

namespace LnmChunkType {
	static const uint16 NAV_MESH_VERTICES = ExportFormat::ChunkType::FIRST_AVAILABLE;
	static const uint16 NAV_MESH_POLYGONS =	NAV_MESH_VERTICES + 1;
	static const uint16 NAV_MESH_GRAPH_NODE_INDICES = NAV_MESH_VERTICES + 2;
	static const uint16 NAV_MESH_SCENE_BOUNDS = NAV_MESH_VERTICES + 3;
	static const uint16 NAV_MESH_MODIFIER_INFO = NAV_MESH_VERTICES + 4;
	static const uint16 NAV_MESH_SPAN_BUFFER_ASCII = NAV_MESH_VERTICES + 5;
	static const uint16 NAV_MESH_CONTOURS_ASCII = NAV_MESH_VERTICES + 6;
	static const uint16 NAV_MESH_NAV_SURF_ELEMENTS_ASCII = NAV_MESH_VERTICES + 7;
	static const uint16 NAV_MESH_SPAN_BUFFER_BIN = NAV_MESH_VERTICES + 8;
	static const uint16 NAV_MESH_NAV_SURF_ELEMENTS_BIN = NAV_MESH_VERTICES + 9;
}


namespace NavMeshVertexListChunk {

	struct Header {
		uint32 m_numVertices;

		Header (uint32 numVertices) :	m_numVertices (numVertices) { }
		Header (CCryFile & );
	};

};

namespace NavMeshPolygonListChunk {

	struct Header {
		uint32 m_numPolygons;
		// NOTE Okt 20, 2008: <pvl> not strictly necessary but added to enable
		// the reader to reserve the right amount of memory for all indices in advance
		uint32 m_numIndices;

		Header (unsigned short version, unsigned short numPolygons, unsigned int numIndices) :
				m_numPolygons (numPolygons), m_numIndices (numIndices)
		{ }
		Header (CCryFile & );

		void Write (CCryFile & ) const;

		static size_t Size ();
	};

	// TODO Jul 30, 2008: <pvl> helper struct, should move closer to where
	// the actual writer is
	struct Polygon {
		typedef uint16 RegionType;
		typedef uint8 NumIndicesType;

		RegionType m_region;
		NumIndicesType m_numIndices;
		std::vector <uint16> m_indices;

		void Write (CCryFile & ) const;
	};
};

namespace NavMeshGraphNodeIndexListChunk {

	// TODO Jul 25, 2008: <pvl> practically the same as NavMeshVertexListChunk::Header -
	// would it be possible to reuse stuff here?
	struct Header {
		uint16 m_numIndices;

		Header (unsigned short numIndices) :	m_numIndices (numIndices) { }
		Header (CCryFile & );
	};

};

} // namespace LnmExportFormat
} // namespace LayeredNavMesh

#endif // LNM_EXPORT_FORMAT_H
