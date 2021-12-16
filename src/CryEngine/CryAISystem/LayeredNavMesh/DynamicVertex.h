
#ifndef LNM_DYNAMIC_VERTEX_H
#define LNM_DYNAMIC_VERTEX_H

//#include "../../../Sandbox/Editor/AI/NavDataGeneration/LayeredNavMesh/DebugDraw.h"
#define DEBUG_DRAW

// ATTN Okt 9, 2008: <pvl> these numbers may be used for sorting, if you change
// them try not to reorder types
static const char USER_DATA_NAV_POLY = 0;
static const char USER_DATA_CROSS_VTX = 1;
static const char USER_DATA_OBSTACLE_SSHAPE = 2;
static const char USER_DATA_OBSTACLE_CROSS_VTX = 3;

struct SShape;

// NOTE Okt 9, 2008: <pvl> static vertex - comes from the static nav mesh
struct StaticVtxData {
	char m_type;
	unsigned int m_vtxIndex;
	StaticVtxData (unsigned index) : m_type(USER_DATA_NAV_POLY), m_vtxIndex(index)
	{ }
#ifdef DEBUG_DRAW
	void DebugDraw (const ::Vec3 & pos) const;
#endif
};

// NOTE Okt 9, 2008: <pvl> an obstacle vertex
struct SShapeVtxData {
	char m_type;
	const void * m_id;
	// NOTE Okt 9, 2008: <pvl> vertex number within the shape
	unsigned int m_vtxNumber;
	SShapeVtxData (const void * shape, unsigned vtx) :
			m_type(USER_DATA_OBSTACLE_SSHAPE), m_id(shape), m_vtxNumber(vtx)
	{ }
#ifdef DEBUG_DRAW
	void DebugDraw (const ::Vec3 & pos) const;
#endif
};

// NOTE Okt 9, 2008: <pvl> cross vertex data
struct CrossVtxData {
	char m_type;
	// NOTE Okt 9, 2008: <pvl> lower numbered static vertex
	StaticVtxData m_static0;
	// NOTE Okt 9, 2008: <pvl> higher numbered static vertex
	StaticVtxData m_static1;
	// NOTE Okt 9, 2008: <pvl> lower numbered obstacle vertex
	SShapeVtxData m_sshape0;
	// NOTE Okt 9, 2008: <pvl> higher numbered obstacle vertex
	SShapeVtxData m_sshape1;
	CrossVtxData (const StaticVtxData & static0, const StaticVtxData & static1,
				const SShapeVtxData & sshape0, const SShapeVtxData & sshape1) :
			m_type(USER_DATA_CROSS_VTX),
			m_static0(static0), m_static1(static1), m_sshape0(sshape0), m_sshape1(sshape1)
	{ }
#ifdef DEBUG_DRAW
	void DebugDraw (const ::Vec3 & pos) const;
#endif
};

// NOTE Okt 9, 2008: <pvl> cross vertex data
struct ObstacleCrossVtxData {
	char m_type;
	// NOTE Okt 9, 2008: <pvl> lower numbered static vertex
	SShapeVtxData m_sshape00;
	// NOTE Okt 9, 2008: <pvl> higher numbered static vertex
	SShapeVtxData m_sshape01;
	// NOTE Okt 9, 2008: <pvl> lower numbered obstacle vertex
	SShapeVtxData m_sshape10;
	// NOTE Okt 9, 2008: <pvl> higher numbered obstacle vertex
	SShapeVtxData m_sshape11;
	ObstacleCrossVtxData (const SShapeVtxData & sshape00, const SShapeVtxData & sshape01,
				const SShapeVtxData & sshape10, const SShapeVtxData & sshape11) :
			m_type(USER_DATA_OBSTACLE_CROSS_VTX),
			m_sshape00(sshape00), m_sshape01(sshape01), m_sshape10(sshape10), m_sshape11(sshape11)
	{ }
#ifdef DEBUG_DRAW
	void DebugDraw (const ::Vec3 & pos) const;
#endif
};

inline bool operator== (const StaticVtxData & rhs, const StaticVtxData & lhs)
{
	return rhs.m_vtxIndex == lhs.m_vtxIndex;
}

inline bool operator== (const SShapeVtxData & rhs, const SShapeVtxData & lhs)
{
	return rhs.m_id == lhs.m_id && rhs.m_vtxNumber == lhs.m_vtxNumber;
}

inline bool operator== (const CrossVtxData & rhs, const CrossVtxData & lhs)
{
	return rhs.m_sshape0 == lhs.m_sshape0 &&
		rhs.m_sshape1 == lhs.m_sshape1 &&
		rhs.m_static0 == lhs.m_static0 &&
		rhs.m_static1 == lhs.m_static1;
}


// ---


class VtxAttribs {
public:
	struct Attrib {
		char m_type;
#ifdef DEBUG_DRAW
		void DebugDraw (const ::Vec3 & pos) const;
#endif
	};
private:
	// NOTE Okt 22, 2008: <pvl> indices into m_attribs
	std::vector <unsigned int> m_ptrs;
	std::vector <unsigned char> m_attribs;
public:
	// ATTN Okt 22, 2008: <pvl> if you use the default ctor make sure you Swap()
	// some data in later.  Otherwise first PushBack() will assert().
	VtxAttribs ();
	VtxAttribs (unsigned numVtxs, unsigned attribsMemorySize);
	Attrib * operator[] (unsigned index) const;

	template <typename T>
	void PushBack (const T & );

	void Swap (VtxAttribs & );
};

inline VtxAttribs::VtxAttribs ()
{
}

inline VtxAttribs::VtxAttribs (unsigned numVtxs, unsigned attribsMemorySize)
{
	m_ptrs.reserve (numVtxs);
	m_attribs.reserve (attribsMemorySize);
}

inline VtxAttribs::Attrib * VtxAttribs::operator[] (unsigned index) const
{
	assert (index < m_ptrs.size());
	return (Attrib * ) & m_attribs [m_ptrs[index]];
}

template <typename T>
inline void VtxAttribs::PushBack (const T & t)
{
	if (m_attribs.capacity() - m_attribs.size() < sizeof(T))
	{
		// FIXxME Okt 21, 2008: <pvl> implement adding memory?
		// UPDATE Okt 22, 2008: <pvl> does not seem necessary
		assert (0);
		return;
	}

	unsigned int firstFree = m_attribs.size();
	m_attribs.resize (firstFree + sizeof (T));
	new ( & m_attribs [firstFree] ) T (t);
	m_ptrs.push_back (firstFree);
}

inline void VtxAttribs::Swap (VtxAttribs & other)
{
	other.m_ptrs.swap (m_ptrs);
	other.m_attribs.swap (m_attribs);
}


inline bool operator== (const VtxAttribs::Attrib & rhs, const VtxAttribs::Attrib & lhs)
{
	if (rhs.m_type != lhs.m_type)
		return false;

	switch (rhs.m_type)
	{
	case USER_DATA_NAV_POLY:
		return *(StaticVtxData * )&rhs == *(StaticVtxData * )&lhs;
		break;
	case USER_DATA_OBSTACLE_SSHAPE:
		return *(SShapeVtxData * )&rhs == *(SShapeVtxData * )&lhs;
		break;
	case USER_DATA_CROSS_VTX:
		return *(CrossVtxData * )&rhs == *(CrossVtxData * )&lhs;
		break;
	default:
		assert (0 && "unknown type of vertex user data for dynamic vertex");
		return false;		// NOTE Okt 14, 2008: <pvl> placate the compiler
	}
}


#endif // LNM_DYNAMIC_VERTEX_H
