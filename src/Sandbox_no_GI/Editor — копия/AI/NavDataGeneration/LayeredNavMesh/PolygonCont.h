#ifndef LNM_POLYGON_CONTAINER_H
#define LNM_POLYGON_CONTAINER_H

#include "NavMesh.h"
#include "ExportFormat.h"
#include "../../../../../CryEngine/CryAISystem/LayeredNavMesh/DynamicVertex.h"

#include <limits>

#include "Cry_Geo.h"

// TODO Okt 16, 2008: <pvl> is it right to have this symbol outside the LNM namespace?
struct DynamicOverlay;

namespace LayeredNavMesh {

// NOTE Okt 20, 2008: <pvl> this class is a thin wrapper around std::vector to
// allow having unsigned integers of any width stored in a single container type,
// tightly packed.  The idea is to communicate stored values through API that
// only uses the widest natively supported unsigned integral but store them
// internally so that memory isn't wasted.  The actual width of the stored values
// is determined dynamically and stored in 'm_stride'.
//
// The aim is to enable outside code to manipulate collections of integers (like
// index arrays) uniformly without having to use polymorphism at such a low
// level.  The price to pay for this is a small hit while adding and retrieving
// values which is considered acceptable with respect to the intended usage patterns.
//
// As a side effect, this class is designed to be rather restrictive about its
// memory management.  It's supposed to be used where it's const once filled and
// it's designed to expose reallocations hidden by std::vector by effectively
// forcing the client code to manage memory explictly, and thus exposing what
// would otherwise happen unnoticed.
//
// NOTE Okt 20, 2008: <pvl> if the above mentioned hit while dereferencing isn't
// acceptable, and if client code is willing to branch on 'm_stride' before
// starting looping, it's possible to add another iterator templated on the
// integral type to be retrieved from the container.
class StridedArray {
	unsigned int m_stride;
	std::vector <unsigned char> m_memory;

	template<typename valuetype>
	void push_back_impl (valuetype val)
	{
		assert(m_stride == sizeof(valuetype));

		unsigned char *ptr = reinterpret_cast<unsigned char*>(&val);
		m_memory.insert (m_memory.end (), ptr, ptr + sizeof(valuetype));
	}

public:
	StridedArray (unsigned int stride) :
			m_stride (stride)
	{ }
	StridedArray (unsigned int stride, unsigned int numElem) :
			m_stride (stride)
	{
		m_memory.reserve (numElem * stride);
	}

	// NOTE Oct 30, 2008: <pvl> it's "Const" because it returns value,
	// not reference from its operator*() so the container can't be 
	// changed through it
	class ConstIterator {
		const StridedArray & m_array;
		unsigned m_index;
	public: 
		ConstIterator (const StridedArray & arr, unsigned index = 0) :
					m_array (arr), m_index (index)
		{
		}
		unsigned int operator* () const
		{
			return m_array [m_index];
		}
		ConstIterator & operator++ () { ++m_index; return *this; }
		bool operator== (const ConstIterator & rhs) const
		{
			return & m_array == & rhs.m_array && m_index == rhs.m_index;            
		}
		bool operator!= (const ConstIterator & rhs) const
		{       
			return ! operator== (rhs);
		}
	};

	unsigned int operator[] (unsigned int index) const
	{
		unsigned int result = 0;

#if (defined(PS3) || defined(XENON))
		void *pDestination = reinterpret_cast<unsigned char*>(&result) + sizeof(unsigned int) - m_stride;
#else
		void *pDestination = &result;
#endif

		memcpy ( pDestination, &m_memory[index * m_stride], m_stride);

		return result;
	}

	// NOTE Okt 21, 2008: <pvl> it's only possible to set stride on a container
	// with no elements - otherwise this function does nothing
	void SetStride (unsigned new_stride)
	{
		assert (m_memory.empty ());
		m_stride = new_stride;
	}

	// NOTE Okt 21, 2008: <pvl> this operation trims off any excess memory.  It
	// does involve copying of the data so it's still preferable to reserve the
	// right amount of memory if the right amount can be known in advance.
	void ShrinkToFit ()
	{
		if (m_memory.size () < m_memory.capacity ())
			std::vector<unsigned char> (m_memory).swap (m_memory);
	}

	ConstIterator Begin () const { return ConstIterator (*this); }
	ConstIterator End () const { return ConstIterator (*this, m_memory.size() / m_stride); }

	void reserve (unsigned numElem) { m_memory.reserve (numElem * m_stride); }
	void resize ( unsigned numElem) { m_memory.resize (numElem * m_stride); }
	unsigned size () const { return m_memory.size () / m_stride; }

	void push_back (unsigned int val)
	{
		assert (m_memory.capacity() - m_memory.size() >= m_stride);

		switch(m_stride)
		{
		case 1:
			push_back_impl<uint8>(val);
			break;
		case 2:
			push_back_impl<uint16>(val);
			break;
		case 4:
			push_back_impl<uint32>(val);
			break;
		default:
			assert(0 && "bad stride");
		}
	}
};

class VertexListChunkReader;
class PolygonListChunkReader;

class IndexRangeMgr;

class PolygonCont {
public:

	static const unsigned int INVALID_POLYGON_INDEX;
	static const unsigned int INVALID_VERTEX_INDEX;

	// TODO Okt 23, 2008: <pvl> InternalPolygon is now only publicly visible
	// for DynamicOverlay to be able to use it.  Seeing as DynamicOverlay is
	// sort of equivalent to PolygonCont but for dynamic parts of the nav mesh,
	// maybe the analogy could be made explicit in the program design, getting
	// rid of public InternalPolygon in the process?
	struct InternalPolygon {
		// NOTE Jul 18, 2008: <pvl> ordered to promote good packing
		//
		// UPDATE Okt 15, 2008: <pvl> if we can ensure that 2**24 vertices are
		// enough we could pack 'm_startIndex' and 'm_numIndices' in 4 bytes.
		// By itself, it wouldn't probably save memory because of 'm_region' and
		// alignment requirements but it could make sense to move 'm_region' data
		// for all polygons into a separate block of memory.  If we can then keep
		// sizeof (InternalPolygon) at 4 alignment would be flawless.  Of course,
		// that would make accessing region info for a given polygon slower.  It
		// might still be a good tradeoff if that operation isn't needed very often.
		// It could even be possible to sort 'm_polygons' by region and then compress
		// region info into a series of polygon index intervals (or RLE-style).
		unsigned int m_startIndex;
		unsigned short m_region;
		unsigned char m_numIndices;
		InternalPolygon () : m_startIndex(0), m_region(0), m_numIndices(0) { }
		InternalPolygon (unsigned short region, unsigned short startIndex, unsigned char numIndices);
	};

	struct Polygon {
		const InternalPolygon * m_poly;

		// TODO Nov 6, 2008: <pvl> I wonder if it made sense to unify PolygonCont
		// and DynamicOverlay implementations and just store a pointer to it here -
		// instead of all the following data members?
		const StridedArray * m_indices;
		const std::vector< ::Vec3> * m_vertices;

		// NOTE Nov 6, 2008: <pvl> the following only applies to dynamic polygons.
		// 'm_baseIndex' is needed to know fully qualified indices (overlay index +
		// an index into overlay).
		const VtxAttribs * m_attribs;
		const unsigned m_baseIndex;

		Polygon (const InternalPolygon * poly, const StridedArray * indices,
					const std::vector< ::Vec3> * vtxs) :
				m_poly(poly), m_indices(indices), m_vertices(vtxs), m_attribs(0), m_baseIndex(0)
		{ }
		Polygon (const InternalPolygon * poly, const StridedArray * indices,
					const std::vector< ::Vec3> * vtxs, const VtxAttribs * attrs, unsigned baseIndex) :
				m_poly(poly), m_indices(indices), m_vertices(vtxs), m_attribs(attrs), m_baseIndex(baseIndex)
		{ }

		// NOTE Nov 6, 2008: <pvl> 'vtxIndex' is between 0 and NumVertices()
		const VtxAttribs::Attrib * GetVtxAttribs (unsigned vtxIndex) const;
		unsigned GetVtxIndex (unsigned vtx) const;

		const ::Vec3 & operator[] (unsigned vtxIndex) const;

		unsigned int NumVertices () const { return m_poly->m_numIndices; }

		::Vec3 GetCentroid () const;
		AABB GetAABB () const;
		// NOTE Jun 8, 2009: <pvl> retval.first is minimum, retval.second is maximum
		std::pair<float,float> GetHeightRange () const;

		bool IsDynamic () const { return m_attribs != 0; }

#ifdef DEBUG_DRAW
		void DebugDraw (const ColorB & , float verticalOffset = 0.0f) const;
#endif // DEBUG_DRAW
		void DumpIntoFile (FILE * ) const;

		struct iterator_base {

			const Polygon * m_polygon;
			int m_currVertex;

			iterator_base (const Polygon * , int startingVertex = 0);
			iterator_base & operator++ ();
			iterator_base & operator-- ();
			bool operator== (const iterator_base & ) const;
			bool operator!= (const iterator_base & ) const;
		};

		class vertex_iterator {
			iterator_base m_base;
		public:
			vertex_iterator (const Polygon * poly, int startingVertex = 0) :
					m_base (poly, startingVertex)
			{ }
			vertex_iterator & operator++ () { ++m_base; return *this; }
			vertex_iterator & operator-- () { --m_base; return *this; }
			bool operator== (const vertex_iterator & rhs) const { return m_base == rhs.m_base; }
			bool operator!= (const vertex_iterator & rhs) const { return m_base != rhs.m_base; }
			const ::Vec3 & operator* () const;
			const ::Vec3 * operator-> () const;
		};

		// TODO Nov 23, 2009: <pvl> for compatibility with templates that expect
		// STL-style stuff - probably requires more thought and some clean-up
		typedef vertex_iterator iterator;
		typedef vertex_iterator const_iterator;

		vertex_iterator begin () const { return vertex_iterator (this); }
		vertex_iterator end () const { return vertex_iterator (this, m_poly->m_numIndices); }

		class vertex_index_iterator {
			iterator_base m_base;
		public:
			vertex_index_iterator (
						const Polygon * poly, int startingVertex = 0) :
					m_base (poly, startingVertex)
			{ }
			vertex_index_iterator & operator++ () { ++m_base; return *this; }
			vertex_index_iterator & operator-- () { --m_base; return *this; }
			bool operator== (const vertex_index_iterator & rhs) const { return m_base == rhs.m_base; }
			bool operator!= (const vertex_index_iterator & rhs) const { return m_base != rhs.m_base; }
			unsigned int operator* () const;
		};

		vertex_index_iterator index_begin () const { return vertex_index_iterator (this); }
		vertex_index_iterator index_end () const { return vertex_index_iterator (this, m_poly->m_numIndices); }
	};

private:
	// NOTE Jul 22, 2008: <pvl> the readers are conceptually part of PolygonCont
	// so it's OK to make them friends
	friend class VertexListChunkReader;
	friend class PolygonListChunkReader;

	// TODO Aug 8, 2008: <pvl> we're dealing mostly with x and y only, could we
	// somehow avoid storing full z and save some memory in the process?
	std::vector < ::Vec3> m_vertices;
	StridedArray m_vertexIndexArray;
	std::vector <PolygonCont::InternalPolygon> m_polygons;

	// FIXxME Okt 17, 2008: <pvl> compiler warns about deleting an incomplete type
	// DynamicOverlayMgr, no dtor is supposedly called - investigate
	// UPDATE Okt 27, 2008: <pvl> went back to not using auto_ptr - vc++ seems to
	// be instantiating the whole auto_ptr template including dtor when it encounters
	// it here and not even having ~PolygonCont() declared seems to stop the warning
	// from being issued (which seems to be a compiler bug - at any rate, gcc
	// doesn't seem to do this)
	class DynamicOverlayMgr;
	DynamicOverlayMgr * m_dynOverlayMgr;

public:
	PolygonCont ();
	~PolygonCont ();

	inline unsigned int GetPolyCount() const { return m_polygons.size(); }

	Polygon operator[] (unsigned int polygonIndex) const;

	::Vec3 GetVertex (unsigned i) const;

	void Add (DynamicOverlay * );
	void Remove (DynamicOverlay * );
#ifdef DEBUG_DRAW
	void DebugDraw () const;
	/**
	 * Allows the caller to specify vertical offset to prevent z-fighting when
	 * the same poly is drawn multiple times (e.g. basic and highlighted variant).
	 */
	void DebugDrawPolygon (unsigned int polyIndex, const ColorB & , float verticalOffset = 0.0f) const;
#endif // DEBUG_DRAW

	void Clear ();

	void SetupReadingFromFile (ExportFormat::FileReader * );
	void CleanupReadingFromFile (ExportFormat::FileReader * );

	// TODO Okt 28, 2008: <pvl> the (current) reason for having to setup dynamics
	// handling explicitly is to pass IndexRangeMgr* to the PolygonCont (its
	// DynamicOverlayMgr really).  Ideally, dynamics handling should be setup
	// automatically as soon as nav mesh data is read in (in CleanupReadingFromFile())
	// but not having the IndexRangeMgr* available at that point prevents us from
	// doing so. :-(  Hopefully a better arrangement emerges later on.
	void SetupDynamicObstacleHandling (IndexRangeMgr * );
};


inline PolygonCont::Polygon::iterator_base::iterator_base (
			const Polygon * poly, int startingVertex) :
		m_polygon (poly), m_currVertex (startingVertex)
{
}

// FIXME Okt 21, 2008: <pvl> stride of 'm_vertexIndexArray' should really only
// be decided once the count of vertices is known.  If there's less than 256
// we could even use just a single byte to store an index.
inline PolygonCont::PolygonCont () : m_dynOverlayMgr (0), m_vertexIndexArray(2)
{
}

inline const VtxAttribs::Attrib * PolygonCont::Polygon::GetVtxAttribs (unsigned vtxIndex) const
{
	// NOTE Nov 6, 2008: <pvl> only dynamic polygons need attributes, in static
	// polygons all vertices are obviously inherently static :-)
	assert (IsDynamic ());
	return (*m_attribs)[(*m_indices)[m_poly->m_startIndex + vtxIndex]];
}

inline unsigned PolygonCont::Polygon::GetVtxIndex (unsigned vtx) const
{
	return m_baseIndex + (*m_indices)[m_poly->m_startIndex + vtx];
}

inline const ::Vec3 & PolygonCont::Polygon::operator[] (unsigned vtxIndex) const
{
	assert (vtxIndex < NumVertices ());
	return (*m_vertices)[GetVtxIndex(vtxIndex)];
}

inline ::Vec3 PolygonCont::Polygon::GetCentroid () const
{
	::Vec3 centroid (0.0, 0.0, 0.0);

	const unsigned numVertices = NumVertices ();
	vertex_iterator it = begin ();
	vertex_iterator e = end ();
	// NOTE Okt 28, 2008: <pvl> less likely to overflow but involves multiple divisions
	for ( ; it != e; ++it)
		centroid += *it / static_cast <float> (numVertices);

	return centroid;
}

inline AABB PolygonCont::Polygon::GetAABB () const
{
	AABB aabb (AABB::RESET);

	vertex_iterator it = begin ();
	vertex_iterator e = end ();
	for ( ; it != e; ++it)
		aabb.Add (*it);

	return aabb;
}

inline std::pair<float,float> PolygonCont::Polygon::GetHeightRange () const
{
	std::pair <float,float> retval (std::numeric_limits<float>::max(), std::numeric_limits<float>::min());

	vertex_iterator it = begin ();
	vertex_iterator e = end ();
	for ( ; it != e; ++it)
		if ((*it).z < retval.first)
			retval.first = (*it).z;
		else if ((*it).z > retval.second)
			retval.second = (*it).z;

	return retval;
}

inline PolygonCont::Polygon::iterator_base &
PolygonCont::Polygon::iterator_base::operator++ ()
{
	++m_currVertex;
	return *this;
}

inline PolygonCont::Polygon::iterator_base &
PolygonCont::Polygon::iterator_base::operator-- ()
{
	--m_currVertex;
	return *this;
}

inline bool PolygonCont::Polygon::iterator_base::operator== (const iterator_base & rhs) const
{
	assert (m_polygon == rhs.m_polygon);
	return m_currVertex == rhs.m_currVertex;
}

inline bool PolygonCont::Polygon::iterator_base::operator!= (const iterator_base & rhs) const
{
	return ! (*this == rhs);
}

inline const ::Vec3 & PolygonCont::Polygon::vertex_iterator::operator* () const
{
	return (*m_base.m_polygon->m_vertices) [ (*m_base.m_polygon->m_indices) [m_base.m_polygon->m_poly->m_startIndex + m_base.m_currVertex]];
}

inline const ::Vec3 * PolygonCont::Polygon::vertex_iterator::operator-> () const
{
	return & operator*();
}

// ---

class VertexListChunkReader : public ExportFormat::VersionedChunkReader
{
	std::vector < ::Vec3> & m_vertices;
public:
	VertexListChunkReader (std::vector < ::Vec3> & vertices);
	virtual bool ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & );
};

class PolygonListChunkReader : public ExportFormat::VersionedChunkReader
{
	StridedArray & m_vertexIndexArray;
	std::vector <PolygonCont::InternalPolygon> & m_polygons;
public:
	PolygonListChunkReader (StridedArray & indexArray, std::vector <PolygonCont::InternalPolygon> & polys);
	virtual bool ReadChunk (const ExportFormat::VersionChunkHeader & , CCryFile & );
};

// ---

class IndexRangeMgr {

public:
	typedef unsigned int RangeId;

private:
	// NOTE Okt 16, 2008: <pvl> dynamic vertices have the highest bit of their index set
	// UPDATE Feb 19, 2009: <pvl> switched to 3-byte indices due to the need to
	// store also modifier ID in SLayeredMeshNavData, meaning that polygon index alone
	// can't use full 32 bits anymore.
	// UPDATE Aug 7, 2009: <pvl> another four bits taken away from polygon index
	// for agent type in SLayeredMeshNavData - 20 bits left
	static const unsigned int INDEX_BASE = 1 << (20 - 1);
	// NOTE Okt 16, 2008: <pvl> this many vertex indices are allocated to every overlay
	static const unsigned short INDEX_RANGE_SIZE = (1 << 10);

	class IndexRangeAllocator {
		std::vector <unsigned char> m_allocationUsageBitmap;

		RangeId GetUnusedRange () const;
		void MarkRangeUsed (RangeId );
		void MarkRangeUnused (RangeId );
	public:
		IndexRangeAllocator (unsigned maxNumOverlays);

		unsigned AllocateIndexRange ();
		void FreeIndexRange (RangeId );
	};

	IndexRangeAllocator m_indexAllocator;

public:
	IndexRangeMgr (unsigned maxNumOverlays);

	unsigned AllocateRange ();
	void FreeRange (unsigned rangeNum);

	unsigned GetBaseIndexFromRange (RangeId rangeNum) const;
	RangeId GetRangeFromIndex (unsigned index) const;

	// NOTE Okt 27, 2008: <pvl> takes a compound index and returns a range num
	// and a simple index into that range
	std::pair<unsigned,unsigned> Translate (unsigned compoundIndex) const;

	bool IsIndexDynamic (unsigned int index) const;
};

} // namespace LayeredNavMesh

// NOTE Nov 18, 2008: <pvl> cannot be defined within LayeredNavMesh namespace
namespace std {
template <>
struct iterator_traits <LayeredNavMesh::PolygonCont::Polygon::vertex_index_iterator>
{
	typedef std::bidirectional_iterator_tag iterator_category;
};
} // namespace std



#endif // LNM_POLYGON_CONTAINER_H
