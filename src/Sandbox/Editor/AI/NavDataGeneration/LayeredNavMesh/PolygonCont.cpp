
#include "StdAfx.h"

#include "PolygonCont.h"
#include "LnmExportFormat.h"
#include "DebugDraw.h"
#include "../../../../../CryEngine/CryAISystem/LayeredNavMesh/DynamicObstacles.h"

// NOTE Okt 17, 2008: <pvl> without this it doesn't compile in Editor
#include "IRenderAuxGeom.h"
#include <limits>

namespace LayeredNavMesh {

VertexListChunkReader::VertexListChunkReader (std::vector < ::Vec3> & vertices) :
		m_vertices (vertices)
{
}

bool VertexListChunkReader::ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & file)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "LNM - Vertex");

	LnmExportFormat::NavMeshVertexListChunk::Header hdr (file);

	if (hdr.m_numVertices == 0)
		return true;

	m_vertices.reserve (hdr.m_numVertices);

	for (unsigned v=0; v < hdr.m_numVertices; ++v)
	{
		::Vec3 vtx;
		file.ReadType ( & vtx);
		m_vertices.push_back (vtx);
	}

	return true;
}

// ---

PolygonListChunkReader::PolygonListChunkReader (
			StridedArray /*std::vector <uint16>*/ & indexArray,
			std::vector <PolygonCont::InternalPolygon> & polys) :
		m_vertexIndexArray (indexArray), m_polygons (polys)
{
}

bool PolygonListChunkReader::ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & file)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "LNM - Polygon");

	LnmExportFormat::NavMeshPolygonListChunk::Header hdr (file);

	m_vertexIndexArray.reserve (hdr.m_numIndices);

	for (uint32 i=0; i < hdr.m_numPolygons; ++i)
	{
		LnmExportFormat::NavMeshPolygonListChunk::Polygon::RegionType region;
		LnmExportFormat::NavMeshPolygonListChunk::Polygon::NumIndicesType numIndices;
		file.ReadType ( & region);
		file.ReadType ( & numIndices);

		int startingIndexIndex = m_vertexIndexArray.size ();

		m_polygons.push_back (LayeredNavMesh::PolygonCont::InternalPolygon (region, startingIndexIndex, numIndices));

		for (int j=0; j < numIndices; ++j)
		{
			uint16 index;
			file.ReadType ( & index);
			m_vertexIndexArray.push_back (index);
		}
	}
	return true;
}

// ---

class PolygonCont::DynamicOverlayMgr {

	// NOTE Okt 28, 2008: <pvl> we don't own this
	IndexRangeMgr * m_indexRangeMgr;

	std::vector <DynamicOverlay*> m_overlays;
public:
	// NOTE Okt 16, 2008: <pvl> 'numStaticPolygons' is used to estimate how many
	// dynamic overlays there can possibly be.  Currently it's assumed that no more
	// than one overlay a static polygon will be needed.
	DynamicOverlayMgr (unsigned numStaticPolygons);

	void SetIndexRangeMgr (IndexRangeMgr * );

	void AddOverlay (DynamicOverlay * );
	void RemoveOverlay (DynamicOverlay * );

	const PolygonCont::Polygon operator[] (unsigned int polygonIndex) const;
	const ::Vec3 & GetVertex (unsigned i) const;

	bool IsIndexDynamic (unsigned int index) const;

#ifdef DEBUG_DRAW
	void DebugDraw () const;
#endif // DEBUG_DRAW
};

IndexRangeMgr::IndexRangeAllocator::IndexRangeAllocator (unsigned maxNumOverlays) :
		m_allocationUsageBitmap (maxNumOverlays / 8 + 1, 0)
{
}

// NOTE Okt 16, 2008: <pvl> returns the number of the first unset bit in the bitmap
IndexRangeMgr::RangeId
IndexRangeMgr::IndexRangeAllocator::GetUnusedRange () const
{
	const unsigned allocBitmapSize = m_allocationUsageBitmap.size ();
	unsigned i = 0;

	// NOTE Okt 16, 2008: <pvl> find first that's not completely full
	for ( ; i < allocBitmapSize; ++i)
		if (m_allocationUsageBitmap[i] < 0xff)
			break;

	// NOTE Okt 16, 2008: <pvl> if not we're full, and that's a bug at a higher level
	assert (i < allocBitmapSize);

	// NOTE Okt 16, 2008: <pvl> now find first free bit in that byte
	unsigned char c = m_allocationUsageBitmap[i];
	unsigned j = 0;
	for ( ; j < 8; ++j, c >>= 1)
		if ((c & 0x1) == 0)
			break;

	// NOTE Okt 16, 2008: <pvl> we ensured before that there's at least one unset bit
	assert (j < 8);

	return 8*i + j;
}

void IndexRangeMgr::IndexRangeAllocator::MarkRangeUsed (RangeId rangeNum)
{
	assert (rangeNum/8 < m_allocationUsageBitmap.size ());
	m_allocationUsageBitmap [rangeNum/8] |= 1 << (rangeNum%8);
}

void IndexRangeMgr::IndexRangeAllocator::MarkRangeUnused (RangeId rangeNum)
{
	assert (rangeNum/8 < m_allocationUsageBitmap.size ());
	m_allocationUsageBitmap [rangeNum/8] &= ~(1 << (rangeNum%8));
}

// NOTE Okt 16, 2008: <pvl> returns base index of a newly allocated range
IndexRangeMgr::RangeId
IndexRangeMgr::IndexRangeAllocator::AllocateIndexRange ()
{
	const RangeId rangeNum = GetUnusedRange ();
	MarkRangeUsed (rangeNum);
	return rangeNum;
}

// FIXME Okt 16, 2008: <pvl> 'index' can be any index from the range to be freed
void IndexRangeMgr::IndexRangeAllocator::FreeIndexRange (RangeId rangeNum)
{
	assert (rangeNum/8 < m_allocationUsageBitmap.size ());
	MarkRangeUnused (rangeNum);
}

IndexRangeMgr::IndexRangeMgr (unsigned maxNumOverlays) :
		m_indexAllocator (maxNumOverlays)
{
}

unsigned IndexRangeMgr::AllocateRange ()
{
	return m_indexAllocator.AllocateIndexRange ();
}

void IndexRangeMgr::FreeRange (unsigned rangeNum)
{
	m_indexAllocator.FreeIndexRange (rangeNum);
}

unsigned IndexRangeMgr::GetBaseIndexFromRange (RangeId rangeNum) const
{
	return INDEX_BASE + rangeNum * INDEX_RANGE_SIZE;
}

IndexRangeMgr::RangeId
IndexRangeMgr::GetRangeFromIndex (unsigned index) const
{
	return (index - INDEX_BASE) / INDEX_RANGE_SIZE;
}

// NOTE Okt 27, 2008: <pvl> takes a compound index and returns a range num
// and a simple index into that range
std::pair<unsigned,unsigned>
IndexRangeMgr::Translate (unsigned compoundIndex) const
{
	compoundIndex -= INDEX_BASE;
	return std::pair<unsigned,unsigned> (compoundIndex / INDEX_RANGE_SIZE, compoundIndex % INDEX_RANGE_SIZE);
}

bool IndexRangeMgr::IsIndexDynamic (unsigned int index) const
{
	return index >= INDEX_BASE;
}

// NOTE Okt 16, 2008: <pvl> the constant in 'm_overlays' initialization is just
// an uninformed guess.  The idea is obviously to avoid gradual reallocations
// during system startup but how much the actual average/peak usage might be
// has to be deduced only after the system gets some use.
// If the number of dynamic overlays is found to be varying widely it might also
// be necessary to have a deallocation scheme to shrink the memory.
PolygonCont::DynamicOverlayMgr::DynamicOverlayMgr (unsigned numStaticPolygons) :
		// TODO Okt 28, 2008: <pvl> 'm_indexRangeMgr' has to be filled in later
		// by the upper layer.  It's awkward since every member function needs to
		// check this.  Unfortunately, I don't see a good way of passing
		// it in to the constructor.
		// UPDATE Okt 28, 2008: <pvl> maybe this DynamicOverlayMgr should live
		// directly in DynamicObstacleMgr in the LNM nav region?
		m_indexRangeMgr (0), m_overlays (numStaticPolygons/4, 0)
{
}

void PolygonCont::DynamicOverlayMgr::SetIndexRangeMgr (IndexRangeMgr * indexRangeMgr)
{
	m_indexRangeMgr = indexRangeMgr;
}

void PolygonCont::DynamicOverlayMgr::AddOverlay (DynamicOverlay * newOverlay)
{
	assert (m_indexRangeMgr && "you need to pass an IndexRangeMgr* in to be able to handle dynamic obstacles");
	const IndexRangeMgr::RangeId rangeNum = m_indexRangeMgr->AllocateRange ();

	if (rangeNum >= m_overlays.size ())
		m_overlays.resize (rangeNum + 1);
	m_overlays[rangeNum] = newOverlay;

	newOverlay->SetBaseIndex (m_indexRangeMgr->GetBaseIndexFromRange (rangeNum));
}

void PolygonCont::DynamicOverlayMgr::RemoveOverlay (DynamicOverlay * overlay)
{
	assert (m_indexRangeMgr && "you need to pass an IndexRangeMgr* in to be able to handle dynamic obstacles");
	const unsigned baseIndex = overlay->GetBaseIndex ();
	const IndexRangeMgr::RangeId rangeNum = m_indexRangeMgr->GetRangeFromIndex (baseIndex);
	assert (rangeNum < m_overlays.size ());
	m_overlays[rangeNum] = 0;
	m_indexRangeMgr->FreeRange (rangeNum);
}

const PolygonCont::Polygon
PolygonCont::DynamicOverlayMgr::operator[] (unsigned int polygonIndex) const
{
	assert (m_indexRangeMgr && "you need to pass an IndexRangeMgr* in to be able to handle dynamic obstacles");
	std::pair<unsigned,unsigned> rangeAndIndex = m_indexRangeMgr->Translate (polygonIndex);

	// NOTE Okt 28, 2008: <pvl> just for convenience, to give things proper names
	const unsigned rangeNum = rangeAndIndex.first;
	const unsigned indexIntoRange = rangeAndIndex.second;

	DynamicOverlay * overlay = m_overlays [rangeNum];
	return (*overlay) [indexIntoRange];
}

const ::Vec3 & PolygonCont::DynamicOverlayMgr::GetVertex (unsigned vtxIndex) const
{
	assert (m_indexRangeMgr && "you need to pass an IndexRangeMgr* in to be able to handle dynamic obstacles");
	std::pair<unsigned,unsigned> rangeAndIndex = m_indexRangeMgr->Translate (vtxIndex);
	const unsigned rangeNum = rangeAndIndex.first;
	const unsigned indexIntoRange = rangeAndIndex.second;

	return m_overlays[rangeNum]->GetVertex (indexIntoRange);
}

// TODO Okt 28, 2008: <pvl> would it be possible to have the caller of this
// function call the IndexRangeMgr directly and get rid of this forwarder?
// UPDATE Okt 29, 2008: <pvl> PolygonCont uses this, it would need to see the
// range manager directly to avoid calling this.  Or it would take a more fundamental
// reassignment of responsibilities among various classes, that might be the way to go.
bool PolygonCont::DynamicOverlayMgr::IsIndexDynamic (unsigned int index) const
{
	if (m_indexRangeMgr == 0) return false;

	return m_indexRangeMgr->IsIndexDynamic (index);
}

#ifdef DEBUG_DRAW
void PolygonCont::DynamicOverlayMgr::DebugDraw () const
{
	std::vector<DynamicOverlay*>::const_iterator it = m_overlays.begin ();
	std::vector<DynamicOverlay*>::const_iterator end = m_overlays.end ();
	for ( ; it != end; ++it)
	{
		DynamicOverlay * overlay = *it;
		if (overlay == 0) continue;
		unsigned numPolys = overlay->m_polygons.size ();
		for (uint32 p=0; p < numPolys; ++p)
			(*overlay)[p].DebugDraw (IntToCol (p + overlay->GetBaseIndex (), 255, true), 0.2f/*vertical offset*/);

		// NOTE Nov 12, 2008: <pvl> disabled (temporarily) - it adds a lot of noise.
		// It might be useful to let the caller pass a parameter saying if overlay
		// debug drawing is desired or not.
		//overlay->DebugDraw ();
	}
}
#endif // DEBUG_DRAW

// ---

const unsigned int PolygonCont::INVALID_POLYGON_INDEX = std::numeric_limits <unsigned>::max ();
const unsigned int PolygonCont::INVALID_VERTEX_INDEX = std::numeric_limits <unsigned>::max ();

LayeredNavMesh::PolygonCont::InternalPolygon::InternalPolygon (
			unsigned short region, unsigned short startIndex, unsigned char numIndices) :
		m_startIndex (startIndex), m_region (region), m_numIndices (numIndices)
{
}

#include "../../../../../CryEngine/CryAISystem/LayeredNavMesh/DynamicVertex.h"
unsigned int PolygonCont::Polygon::vertex_index_iterator::operator* () const
{
	unsigned indexIndex = m_base.m_polygon->m_poly->m_startIndex + m_base.m_currVertex;
	const VtxAttribs * attribs = m_base.m_polygon->m_attribs;
	unsigned vertexIndex = (*m_base.m_polygon->m_indices)[indexIndex];
	if (attribs && (*attribs)[vertexIndex]->m_type == USER_DATA_NAV_POLY)
	{
		StaticVtxData * vtxData = reinterpret_cast <StaticVtxData*> ((*attribs)[vertexIndex]);
		return vtxData->m_vtxIndex;
	}
	else
		return (*m_base.m_polygon->m_indices) [indexIndex];
}


PolygonCont::~PolygonCont ()
{
	delete m_dynOverlayMgr;
}

void PolygonCont::Add (::DynamicOverlay * overlay)
{
	m_dynOverlayMgr->AddOverlay (overlay);
}

void PolygonCont::Remove (::DynamicOverlay * overlay)
{
	m_dynOverlayMgr->RemoveOverlay (overlay);
}

PolygonCont::Polygon PolygonCont::operator[] (unsigned int polygonIndex) const
{
	if (m_dynOverlayMgr->IsIndexDynamic (polygonIndex))
		return (*m_dynOverlayMgr)[polygonIndex];

	return Polygon ( & m_polygons[polygonIndex], & m_vertexIndexArray, & m_vertices);
}

::Vec3 PolygonCont::GetVertex (unsigned i) const
{
	if (m_dynOverlayMgr->IsIndexDynamic (i))
		return m_dynOverlayMgr->GetVertex (i);

	assert (i < m_vertices.size ());
	return m_vertices[i];
}

void LayeredNavMesh::PolygonCont::Clear ()
{
	m_vertices.resize (0);
	m_vertexIndexArray.resize (0);
	m_polygons.resize (0);
}

void LayeredNavMesh::PolygonCont::SetupReadingFromFile (ExportFormat::FileReader * fileReader)
{
	ExportFormat::VersionSwitchChunkReader * vertexListVersionSwitch = new ExportFormat::VersionSwitchChunkReader;
	vertexListVersionSwitch->RegisterReader (1, new VertexListChunkReader (m_vertices));
	fileReader->RegisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_VERTICES, vertexListVersionSwitch);

	ExportFormat::VersionSwitchChunkReader * polygonListVersionSwitch = new ExportFormat::VersionSwitchChunkReader;
	polygonListVersionSwitch->RegisterReader (1, new PolygonListChunkReader (m_vertexIndexArray, m_polygons));
	fileReader->RegisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_POLYGONS, polygonListVersionSwitch);
}

void LayeredNavMesh::PolygonCont::CleanupReadingFromFile (ExportFormat::FileReader * fileReader)
{
	delete fileReader->UnregisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_VERTICES);
	delete fileReader->UnregisterChunkReader (LnmExportFormat::LnmChunkType::NAV_MESH_POLYGONS);

	// NOTE Okt 16, 2008: <pvl> now data should be read in, it's time to set up
	// dynamics handling as well
	m_dynOverlayMgr = new DynamicOverlayMgr (m_polygons.size ());
	// ATTN Okt 28, 2008: <pvl> 'm_dynOverlayMgr' will not be functional until
	// we pass IndexRangeMgr to it!
}

void LayeredNavMesh::PolygonCont::SetupDynamicObstacleHandling (IndexRangeMgr * indexRangeMgr)
{
	m_dynOverlayMgr->SetIndexRangeMgr (indexRangeMgr);
}

// ---

#ifdef DEBUG_DRAW
void LayeredNavMesh::PolygonCont::DebugDraw () const
{
	unsigned int polyCount = GetPolyCount ();
	for (unsigned int i=0; i < polyCount; ++i)
	{
		ColorB col (255,255,255,160);

		// FIXME Aug 5, 2008: <pvl> add a cvar
		if (true /*colorPolys*/)
		{
			col = IntToCol (i, 160);
		}
		else
		{
			const InternalPolygon * poly = (*this)[i].m_poly;
			col = IntToCol (poly->m_region, 255);
		}

		DebugDrawPolygon (i, col);
	}

	if (m_dynOverlayMgr)
		m_dynOverlayMgr->DebugDraw ();
}

void LayeredNavMesh::PolygonCont::Polygon::DebugDraw (const ColorB & col, float verticalOffset) const
{
	// TODO Aug 5, 2008: <pvl> could be checked directly in the loader
	assert (NumVertices () >= 3);

	::Vec3 offsetVec (0.0f, 0.0f, verticalOffset);

	Polygon::vertex_iterator first = begin ();
	Polygon::vertex_iterator it = ++begin ();
	Polygon::vertex_iterator e = --end ();
	for ( ; it != e; ++it)
	{
		Polygon::vertex_iterator next (it);
		++next;

		::Vec3 v0 = *first + offsetVec;
		::Vec3 v1 = *it + offsetVec;
		::Vec3 v2 = *next + offsetVec;

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle (v0, col, v1, col, v2, col);
	}
}

void LayeredNavMesh::PolygonCont::DebugDrawPolygon (unsigned int polyIndex, const ColorB & col, float verticalOffset) const
{
	const Polygon & poly = (*this)[polyIndex];
	poly.DebugDraw (col, verticalOffset);
}

#endif // DEBUG_DRAW

void LayeredNavMesh::PolygonCont::Polygon::DumpIntoFile (FILE * f) const
{
	Polygon::vertex_iterator it = begin ();
	Polygon::vertex_iterator e = end ();
	for ( ; it != e; ++it)
	{
		const ::Vec3 & vtxPos = *it;
		fprintf (f, "%16.8f %16.8f\n", vtxPos.x, vtxPos.y);
	}
}


} // namespace LayeredNavMesh
