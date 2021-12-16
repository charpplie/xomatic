#ifndef _RENDERAUXGEOM_H_
#define _RENDERAUXGEOM_H_

#if defined(ENABLE_PROFILERS) || !(defined(_RELEASE) && (defined(XENON) || defined(PS3)))
#	define ENABLE_RENDER_AUX_GEOM
#endif

#if defined(ENABLE_RENDER_AUX_GEOM)

#include "IRenderAuxGeom.h"
#include "VertexFormats.h"
#include <CrySizer.h>

class ICrySizer;
class CAuxGeomCB;
struct SAuxGeomCBRawDataPackaged;


struct IRenderAuxGeomImpl
{
public:
	virtual void Flush(const SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end) = 0;
	virtual void RT_Flush(const SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end) = 0;
};


class CAuxGeomCB : public IRenderAuxGeom
{
public:
	// interface
	virtual void SetRenderFlags( const SAuxGeomRenderFlags& renderFlags );
	virtual SAuxGeomRenderFlags GetRenderFlags();

	virtual void DrawPoint( const Vec3& v, const ColorB& col, uint8 size = 1 );
	virtual void DrawPoints( const Vec3* v, uint32 numPoints, const ColorB& col, uint8 size = 1 );
	virtual void DrawPoints( const Vec3* v, uint32 numPoints, const ColorB* col, uint8 size = 1 );

	virtual void DrawLine( const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, float thickness = 1.0f );
	virtual void DrawLines( const Vec3* v, uint32 numPoints, const ColorB& col, float thickness = 1.0f );
	virtual void DrawLines( const Vec3* v, uint32 numPoints, const ColorB* col, float thickness = 1.0f );
	virtual void DrawLines( const Vec3* v, uint32 numPoints, const uint16* ind, uint32 numIndices, const ColorB& col, float thickness = 1.0f );
	virtual void DrawLines( const Vec3* v, uint32 numPoints, const uint16* ind, uint32 numIndices, const ColorB* col, float thickness = 1.0f );
	virtual void DrawPolyline( const Vec3* v, uint32 numPoints, bool closed, const ColorB& col, float thickness = 1.0f );
	virtual void DrawPolyline( const Vec3* v, uint32 numPoints, bool closed, const ColorB* col, float thickness = 1.0f );

	virtual void DrawTriangle( const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, const Vec3& v2, const ColorB& colV2 );
	virtual void DrawTriangles( const Vec3* v, uint32 numPoints, const ColorB& col );
	virtual void DrawTriangles( const Vec3* v, uint32 numPoints, const ColorB* col );
	virtual void DrawTriangles( const Vec3* v, uint32 numPoints, const uint16* ind, uint32 numIndices, const ColorB& col );
	virtual void DrawTriangles( const Vec3* v, uint32 numPoints, const uint16* ind, uint32 numIndices, const ColorB* col );

	virtual void DrawAABB( const AABB& aabb, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle );
	virtual void DrawAABB( const AABB& aabb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle );

	virtual void DrawOBB( const OBB& obb, const Vec3& pos, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle );
	virtual void DrawOBB( const OBB& obb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle );

	virtual void DrawSphere( const Vec3& pos, float radius, const ColorB& col, bool drawShaded = true );
	virtual void DrawCone( const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded = true );
	virtual void DrawCylinder( const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded = true );

	virtual void DrawBone( const QuatT& rParent, const QuatT& rBone, ColorB col );
	virtual void DrawBone( const Matrix34& rParent, const Matrix34& rBone, ColorB col );

	virtual void Flush();

public:
	enum EPrimType
	{
		e_PtList,
		e_LineList,
		e_LineListInd,
		e_TriList,
		e_TriListInd,
		e_Obj,

		e_NumPrimTypes,

		e_PrimTypeInvalid
	};

	enum EAuxDrawObjType
	{
		eDOT_Sphere,
		eDOT_Cone,
		eDOT_Cylinder
	};

	struct SAuxDrawObjParams
	{
		SAuxDrawObjParams()
		{
		}

		SAuxDrawObjParams( const Matrix34& matWorld, const uint32& color, float size, bool shaded )
			: m_matWorld( matWorld )
			, m_color( color )
			, m_size( size )
			, m_shaded( shaded )
		{
		}

		Matrix34 m_matWorld;
		uint32 m_color;
		float m_size;
		bool m_shaded;
	};

	struct SAuxPushBufferEntry
	{
		SAuxPushBufferEntry()
		{
		}

		SAuxPushBufferEntry( uint32 numVertices, uint32 numIndices, uint32 vertexOffs, uint32 indexOffs, uint32 transMatrixIdx, const SAuxGeomRenderFlags& renderFlags )
		: m_numVertices( numVertices )
		, m_numIndices( numIndices )
		, m_vertexOffs( vertexOffs )
		, m_indexOffs( indexOffs )
		, m_transMatrixIdx( transMatrixIdx )
		, m_renderFlags( renderFlags )
		{
		}

		SAuxPushBufferEntry( uint32 drawParamOffs, uint32 transMatrixIdx, const SAuxGeomRenderFlags& renderFlags )
		: m_numVertices( 0 )
		, m_numIndices( 0 )
		, m_vertexOffs( drawParamOffs )
		, m_indexOffs( 0 )
		, m_transMatrixIdx( transMatrixIdx )
		, m_renderFlags( renderFlags )
		{
			assert( e_Obj == GetPrimType( m_renderFlags ) );
		}

		bool GetDrawParamOffs( uint32& drawParamOffs ) const
		{
			if( e_Obj == GetPrimType( m_renderFlags ) )
			{
				drawParamOffs = m_vertexOffs;
				return( true );
			}
			return( false );
		}

		void GetMemoryUsage(ICrySizer* pSizer) const
		{
			pSizer->AddObject(this, sizeof(*this));
		}
		uint32 m_numVertices;
		uint32 m_numIndices;
		uint32 m_vertexOffs;
		uint32 m_indexOffs;
		int m_transMatrixIdx;
		SAuxGeomRenderFlags m_renderFlags;
	};

	typedef std::vector< SAuxPushBufferEntry > AuxPushBuffer;
	typedef std::vector< const SAuxPushBufferEntry* > AuxSortedPushBuffer;
	typedef std::vector< SAuxVertex > AuxVertexBuffer;
	typedef std::vector< uint16 > AuxIndexBuffer;
	typedef std::vector< SAuxDrawObjParams > AuxDrawObjParamBuffer;
	typedef DynArray16<Matrix44A> AuxOrthoMatrixBuffer;

	struct SAuxGeomCBRawData
	{
	public:
		void GetSortedPushBuffer(size_t begin, size_t end, AuxSortedPushBuffer& auxSortedPushBuffer) const;
		void GetMemoryUsage(ICrySizer* pSizer) const;

	public:
		AuxPushBuffer m_auxPushBuffer;
		AuxVertexBuffer m_auxVertexBuffer;
		AuxIndexBuffer m_auxIndexBuffer;
		AuxDrawObjParamBuffer m_auxDrawObjParamBuffer;
		AuxOrthoMatrixBuffer m_auxOrthoMatrices;
	};

public:
	// c/dtor
	CAuxGeomCB(IRenderAuxGeomImpl* pRenderAuxGeom);
	~CAuxGeomCB();

	// get methods for private flags
	static EPrimType GetPrimType( const SAuxGeomRenderFlags& renderFlags );
	static bool IsThickLine( const SAuxGeomRenderFlags& renderFlags );
	static EAuxDrawObjType GetAuxObjType( const SAuxGeomRenderFlags& renderFlags );
	static uint8 GetPointSize( const SAuxGeomRenderFlags& renderFlags );

	// memory usage
	void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->Add(*this);
		m_data.GetMemoryUsage(pSizer);
	}

	// internal buffer toggle
	void Toggle(bool resetOnly)
	{
		m_data.Toggle(resetOnly);
	}

	// setting orthogonal projection
	void SetOrthoMode(bool enable, Matrix44A* pMatrix = 0)
	{
		m_data.SetOrthoMode(enable, pMatrix);
	}

	// sets new thread ID (mostly needed for debugging when switching MT rendering off and back on)
	void SetThreadID(uint32 newThreadID)
	{
		m_threadID = newThreadID;
	}

private:
	enum EAuxGeomPrivateRenderflagBitMasks
	{
		// public field starts at bit 22

		e_PrimTypeShift			= 19,
		e_PrimTypeMask			= 0x7 << e_PrimTypeShift,

		e_PrivateRenderflagsMask = ( 1 << 19 ) - 1
	};

	enum EAuxGeomPrivateRenderflags
	{
		// for non-indexed triangles
		e_TriListParam_ProcessThickLines = 0x00000001,

		// for triangles

		// for lines

		// for points

		// for objects
	};

private:
	uint32 CreatePointRenderFlags( uint8 size );
	uint32 CreateLineRenderFlags( bool indexed );
	uint32 CreateTriangleRenderFlags( bool indexed );
	uint32 CreateObjectRenderFlags( const EAuxDrawObjType& objType );
	
	void DrawThickLine( const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, float thickness );
	
	void AddPushBufferEntry( uint32 numVertices, uint32 numIndices, const SAuxGeomRenderFlags& renderFlags );
	void AddPrimitive( SAuxVertex*& pVertices, uint32 numVertices, const SAuxGeomRenderFlags& renderFlags );
	void AddIndexedPrimitive( SAuxVertex*& pVertices, uint32 numVertices, uint16*& pIndices, uint32 numIndices, const SAuxGeomRenderFlags& renderFlags );
	void AddObject( SAuxDrawObjParams*& pDrawParams, const SAuxGeomRenderFlags& renderFlags );

	int GetTransMatrixIndex()
	{
		return m_data.GetTransMatIdx();
	}

	SAuxGeomCBRawData& AccessData()
	{
		return m_data.Access();
	}

private:
	struct PushBufferSortFunc
	{
		bool operator() (const SAuxPushBufferEntry* lhs, const SAuxPushBufferEntry* rhs) const
		{
			if (lhs->m_renderFlags.m_renderFlags != rhs->m_renderFlags.m_renderFlags)
				return lhs->m_renderFlags.m_renderFlags < rhs->m_renderFlags.m_renderFlags;

			return lhs->m_transMatrixIdx < rhs->m_transMatrixIdx;
		}
	};

	class CRawCBDataContainer
	{
	public:
		CRawCBDataContainer()
		: m_curTransMatIdx(-1)
		, m_curIdx(0)
		, m_lastFlushPos(0)
		{
		}

		SAuxGeomCBRawData& Access()
		{
			return m_cbData[m_curIdx];
		}

		const SAuxGeomCBRawData& Access() const
		{
			return m_cbData[m_curIdx];
		}

		size_t GetLastFlushPos() const
		{
			return m_lastFlushPos;
		}

		size_t GetCurFlushPos() const
		{
			return Access().m_auxPushBuffer.size();
		}

		void UpdateLastFlushPos()
		{
			m_lastFlushPos = GetCurFlushPos();
		}

		void Toggle(bool resetOnly)
		{
			assert(m_curTransMatIdx == -1);
			m_curTransMatIdx = -1;
			if (!resetOnly)
				m_curIdx = (m_curIdx + 1) & 1;
			m_lastFlushPos = 0;
			
			m_cbData[m_curIdx].m_auxPushBuffer.resize(0);
			m_cbData[m_curIdx].m_auxVertexBuffer.resize(0);
			m_cbData[m_curIdx].m_auxIndexBuffer.resize(0);
			m_cbData[m_curIdx].m_auxDrawObjParamBuffer.resize(0);
			m_cbData[m_curIdx].m_auxOrthoMatrices.resize(0);
		}

		void GetMemoryUsage(ICrySizer* pSizer) const
		{
			for (int i(0); i<2; ++i)
				m_cbData[i].GetMemoryUsage(pSizer);
		}

		int GetTransMatIdx() const
		{
			return m_curTransMatIdx;
		}

		void SetOrthoMode(bool enable, Matrix44A* pMatrix = 0)
		{
			if (enable)
			{
				assert(pMatrix);
				m_curTransMatIdx = Access().m_auxOrthoMatrices.size();
				Access().m_auxOrthoMatrices.push_back(*pMatrix);
			}
			else
				m_curTransMatIdx = -1;
		}

	private:
		int m_curTransMatIdx;
		int m_curIdx;
		size_t m_lastFlushPos;
		SAuxGeomCBRawData m_cbData[2];
	};

private:
	SAuxGeomRenderFlags m_curRenderFlags;
	CRawCBDataContainer m_data;
	uint32 m_threadID;
	IRenderAuxGeomImpl* m_pRenderAuxGeom;
};


// package CAuxGeomCB::SAuxGeomCBRawData ptr via seperate struct as nested types cannot be forward declared
struct SAuxGeomCBRawDataPackaged
{
	SAuxGeomCBRawDataPackaged(const CAuxGeomCB::SAuxGeomCBRawData* pData)
	: m_pData(pData)
	{
		assert(m_pData);
	}

	const CAuxGeomCB::SAuxGeomCBRawData* m_pData;
};


inline uint32 CAuxGeomCB::CreatePointRenderFlags( uint8 size )
{
	return( m_curRenderFlags.m_renderFlags | ( e_PtList << e_PrimTypeShift ) | size );
}


inline uint32 CAuxGeomCB::CreateLineRenderFlags( bool indexed )
{
	if( false != indexed )
	{
		return( m_curRenderFlags.m_renderFlags | ( e_LineListInd << e_PrimTypeShift ) );
	}
	else
	{
		return( m_curRenderFlags.m_renderFlags | ( e_LineList << e_PrimTypeShift ) );
	}
}


inline uint32 CAuxGeomCB::CreateTriangleRenderFlags( bool indexed )
{
	if( false != indexed )
	{
		return( m_curRenderFlags.m_renderFlags | ( e_TriListInd << e_PrimTypeShift ) );
	}
	else
	{
		return( m_curRenderFlags.m_renderFlags | ( e_TriList << e_PrimTypeShift ) );
	}
}


inline uint32 CAuxGeomCB::CreateObjectRenderFlags( const EAuxDrawObjType& objType )
{
	return( m_curRenderFlags.m_renderFlags | ( e_Obj << e_PrimTypeShift ) | objType );
}


inline CAuxGeomCB::EPrimType CAuxGeomCB::GetPrimType( const SAuxGeomRenderFlags& renderFlags )
{ 
	uint32 primType( ( renderFlags.m_renderFlags & e_PrimTypeMask ) >> e_PrimTypeShift );
	switch( primType )
	{
	case e_PtList: 
		{
			return( e_PtList );
		}
	case e_LineList: 
		{
			return( e_LineList );
		}
	case e_LineListInd: 
		{
			return( e_LineListInd );
		}
	case e_TriList: 
		{
			return( e_TriList );
		}
	case e_TriListInd: 
		{
			return( e_TriListInd );
		}
	case e_Obj: 
	default:
		{
			assert( e_Obj == primType );
			return( e_Obj );
		}
	}
}


inline bool CAuxGeomCB::IsThickLine( const SAuxGeomRenderFlags& renderFlags )
{
	EPrimType primType( GetPrimType( renderFlags ) );
	assert( e_TriList == primType );
	
	if( e_TriList == primType )
	{
		return( 0 != ( renderFlags.m_renderFlags & e_TriListParam_ProcessThickLines ) );
	}
	else
	{
		return( false );
	}
}


inline CAuxGeomCB::EAuxDrawObjType CAuxGeomCB::GetAuxObjType( const SAuxGeomRenderFlags& renderFlags )
{
	EPrimType primType( GetPrimType( renderFlags ) );
	assert( e_Obj == primType );

	uint32 objType( ( renderFlags.m_renderFlags & e_PrivateRenderflagsMask ) );
	switch( objType )
	{
	case eDOT_Sphere: 
	default:
		{
			assert( eDOT_Sphere == objType );
			return( eDOT_Sphere );
		}
	case eDOT_Cone: 
		{
			assert( eDOT_Cone == objType );
			return( eDOT_Cone );
		}
	case eDOT_Cylinder: 
		{
			assert( eDOT_Cylinder == objType );
			return( eDOT_Cylinder );
		}
	}
}


inline uint8 CAuxGeomCB::GetPointSize( const SAuxGeomRenderFlags& renderFlags )
{
	EPrimType primType( GetPrimType( renderFlags ) );
	assert( e_PtList == primType );

	if( e_PtList == primType )
	{
		return( renderFlags.m_renderFlags & e_PrivateRenderflagsMask );
	}
	else
	{
		return( 0 );
	}
}

#else // #if defined(ENABLE_RENDER_AUX_GEOM)

class CAuxGeomCB_Null : public IRenderAuxGeom
{
public:
	// interface
	virtual void SetRenderFlags(const SAuxGeomRenderFlags& renderFlags) {}
	virtual SAuxGeomRenderFlags GetRenderFlags() {return SAuxGeomRenderFlags();}

	virtual void DrawPoint(const Vec3& v, const ColorB& col, uint8 size = 1) {}
	virtual void DrawPoints(const Vec3* v, uint32 numPoints, const ColorB& col, uint8 size = 1) {}
	virtual void DrawPoints(const Vec3* v, uint32 numPoints, const ColorB* col, uint8 size = 1) {}

	virtual void DrawLine(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, float thickness = 1.0f) {}
	virtual void DrawLines(const Vec3* v, uint32 numPoints, const ColorB& col, float thickness = 1.0f) {}
	virtual void DrawLines(const Vec3* v, uint32 numPoints, const ColorB* col, float thickness = 1.0f) {}
	virtual void DrawLines(const Vec3* v, uint32 numPoints, const uint16* ind, uint32 numIndices, const ColorB& col, float thickness = 1.0f) {}
	virtual void DrawLines(const Vec3* v, uint32 numPoints, const uint16* ind, uint32 numIndices, const ColorB* col, float thickness = 1.0f) {}
	virtual void DrawPolyline(const Vec3* v, uint32 numPoints, bool closed, const ColorB& col, float thickness = 1.0f) {}
	virtual void DrawPolyline(const Vec3* v, uint32 numPoints, bool closed, const ColorB* col, float thickness = 1.0f) {}

	virtual void DrawTriangle(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, const Vec3& v2, const ColorB& colV2) {}
	virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const ColorB& col) {}
	virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const ColorB* col) {}
	virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const uint16* ind, uint32 numIndices, const ColorB& col) {}
	virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const uint16* ind, uint32 numIndices, const ColorB* col) {}

	virtual void DrawAABB(const AABB& aabb, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle) {}
	virtual void DrawAABB(const AABB& aabb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle) {}

	virtual void DrawOBB(const OBB& obb, const Vec3& pos, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle) {}
	virtual void DrawOBB(const OBB& obb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle) {}

	virtual void DrawSphere(const Vec3& pos, float radius, const ColorB& col, bool drawShaded = true) {}
	virtual void DrawCone(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded = true) {}
	virtual void DrawCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded = true) {}

	virtual void DrawBone(const QuatT& rParent, const QuatT& rBone, ColorB col) {}
	virtual void DrawBone(const Matrix34& rParent, const Matrix34& rBone, ColorB col) {}

	virtual void Flush() {}

public:
	CAuxGeomCB_Null() {}
	~CAuxGeomCB_Null() {}
};

#endif // #if defined(ENABLE_RENDER_AUX_GEOM)

#endif // #ifndef _RENDERAUXGEOM_H_
