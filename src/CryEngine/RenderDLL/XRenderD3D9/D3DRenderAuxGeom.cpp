#include "StdAfx.h"
#include "DriverD3D.h"
#include "D3DRenderAuxGeom.h"

#if defined(ENABLE_RENDER_AUX_GEOM)

const float c_clipThres( 0.1f );

enum EAuxGeomBufferSizes
{
	e_auxGeomVBSize = 32768,
	e_auxGeomIBSize = e_auxGeomVBSize * 2
	//e_auxGeomVBSize = 17,
	//e_auxGeomIBSize = 41
};



struct SAuxObjVertex
{
	SAuxObjVertex()
	{
	}

	SAuxObjVertex( const Vec3& pos, const Vec3& normal )
		: m_pos( pos )
		, m_normal( normal )
	{
	}

	Vec3 m_pos;
	Vec3 m_normal;
};


typedef std::vector< SAuxObjVertex > AuxObjVertexBuffer;
typedef std::vector< uint16 > AuxObjIndexBuffer;


CRenderAuxGeomD3D::CRenderAuxGeomD3D( CD3D9Renderer& renderer )
: m_renderer( renderer )
, m_pAuxGeomVB( 0 )
, m_pAuxGeomIB( 0 )
, m_pCurVB( 0 )
, m_pCurIB( 0 )
, m_auxGeomSBM()
, m_wndXRes( 0 )
, m_wndYRes( 0 )
, m_aspect( 1.0f )
, m_aspectInv( 1.0f )
, m_matrices()
, m_curPrimType( CAuxGeomCB::e_PrimTypeInvalid )
, m_curPointSize( 1 )
, m_curTransMatrixIdx( -1 )
, m_pAuxGeomShader( 0 )
, m_curDrawInFrontMode( e_DrawInFrontOff )
, m_auxSortedPushBuffer()
, m_pCurCBRawData(0)
, m_auxGeomCBCol()
, CV_r_auxGeom(1)
{
	gEnv->pConsole->Register("r_auxGeom", &CV_r_auxGeom, 1);
}


CRenderAuxGeomD3D::~CRenderAuxGeomD3D()
{
	SAFE_RELEASE(m_pAuxGeomShader);
	// m_pAuxGeomShader released by CShaderMan ???
	//delete m_pAuxGeomShader;
	//m_pAuxGeomShader = 0;
}


void CRenderAuxGeomD3D::GetMemoryUsage(ICrySizer* pSizer) const
{
	pSizer->AddObject( this, sizeof(*this)); // adjust for new operator
	pSizer->AddObject( m_auxSortedPushBuffer );	
	pSizer->AddObject(m_auxGeomCBCol);
}


void CRenderAuxGeomD3D::ReleaseDeviceObjects()
{
	SAFE_RELEASE( m_pAuxGeomVB );
	SAFE_RELEASE( m_pAuxGeomIB );

	for( uint32 i( 0 ); i < e_auxObjNumLOD; ++i )
	{
		m_sphereObj[ i ].Release();
		m_coneObj[ i ].Release();
		m_cylinderObj[ i ].Release();
	}
}

int CRenderAuxGeomD3D::GetDeviceDataSize()
{
  int nSize = 0;

  nSize += _VertBufferSize(m_pAuxGeomVB);
  nSize += _IndexBufferSize(m_pAuxGeomIB);

  for(uint32 i=0; i<e_auxObjNumLOD; ++i)
  {
    nSize += m_sphereObj[i].GetDeviceDataSize();
    nSize += m_coneObj[i].GetDeviceDataSize();
    nSize += m_cylinderObj[i].GetDeviceDataSize();
  }
  return nSize;
}


// function to generate a sphere mesh
static void CreateSphere( AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib, float radius, unsigned int rings, unsigned int sections )
{
	// calc required number of vertices/indices/triangles to build a sphere for the given parameters
	uint32 numVertices( ( rings - 1 ) * ( sections + 1 ) + 2 );
	uint32 numTriangles( ( rings - 2 ) * sections * 2  + 2 * sections );
	uint32 numIndices( numTriangles * 3 );

	// setup buffers
	vb.clear();
	vb.reserve( numVertices );

	ib.clear();
	ib.reserve( numIndices );

	// 1st pole vertex
	vb.push_back( SAuxObjVertex( Vec3( 0.0f, 0.0f, radius ), Vec3( 0.0f, 0.0f, 1.0f ) ) );

	// calculate "inner" vertices
	float sectionSlice( DEG2RAD( 360.0f / (float) sections ) );
	float ringSlice( DEG2RAD( 180.0f / (float) rings ) );

	for( uint32 a( 1 ); a < rings; ++a )
	{
		float w( sinf( a * ringSlice ) );
		for( uint32 i( 0 ); i <= sections; ++i )
		{
			Vec3 v;
			v.x = radius * cosf( i * sectionSlice ) * w;
			v.y = radius * sinf( i * sectionSlice ) * w;
			v.z = radius * cosf( a * ringSlice );
			vb.push_back( SAuxObjVertex( v, v.GetNormalized() ) );
		}
	}
	
	// 2nd vertex of pole (for end cap)
	vb.push_back( SAuxObjVertex( Vec3( 0.0f, 0.0f, -radius ), Vec3( 0.0f, 0.0f, 1.0f ) ) );

	// build "inner" faces
	for( uint32 a( 0 ); a < rings - 2; ++a )
	{
		for( uint32 i( 0 ); i < sections; ++i )
		{
			ib.push_back( (uint16) ( 1 + a * ( sections + 1 ) + i + 1 ) );
			ib.push_back( (uint16) ( 1 + a * ( sections + 1 ) + i ) );
			ib.push_back( (uint16) ( 1 + ( a + 1 ) * ( sections + 1 ) + i + 1 ) );

			ib.push_back( (uint16) ( 1 + ( a + 1 ) * ( sections + 1 ) + i ) );
			ib.push_back( (uint16) ( 1 + ( a + 1 ) * ( sections + 1 ) + i + 1 ) );
			ib.push_back( (uint16) ( 1 + a * ( sections + 1 ) + i ) );
		}
	}

	// build faces for end caps (to connect "inner" vertices with poles)
	for( uint32 i( 0 ); i < sections; ++i )
	{
		ib.push_back( (uint16) ( 1 + ( 0 ) * ( sections + 1 ) + i ) );
		ib.push_back( (uint16) ( 1 + ( 0 ) * ( sections + 1 ) + i + 1 ) );
		ib.push_back( (uint16) 0 );
	}

	for( uint32 i( 0 ); i < sections; ++i )
	{
		ib.push_back( (uint16) ( 1 + ( rings - 2 ) * ( sections + 1 ) + i + 1 ) );
		ib.push_back( (uint16) ( 1 + ( rings - 2 ) * ( sections + 1 ) + i ) );
		ib.push_back( (uint16) ( ( rings - 1 ) * ( sections + 1 ) + 1 ) );
	}
}



// function to generate a cone mesh
static void CreateCone( AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib, float radius, float height, unsigned int sections )
{
	// calc required number of vertices/indices/triangles to build a cone for the given parameters
	uint32 numVertices( 2 * ( sections + 1 ) + 2 );
	uint32 numTriangles( 2 * sections );
	uint32 numIndices( numTriangles * 3 );

	// setup buffers
	vb.clear();
	vb.reserve( numVertices );

	ib.clear();
	ib.reserve( numIndices );

	// center vertex
	vb.push_back( SAuxObjVertex( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ) ) );

	// create circle around it
	float sectionSlice( DEG2RAD( 360.0f / (float) sections ) );
	for( uint32 i( 0 ); i <= sections; ++i )
	{
		Vec3 v;
		v.x = radius * cosf( i * sectionSlice );
		v.y = 0.0f;
		v.z = radius * sinf( i * sectionSlice );
		vb.push_back( SAuxObjVertex( v, Vec3( 0.0f, -1.0f, 0.0f ) ) );
	}

	// build faces for end cap 
	for( uint16 i( 0 ); i < sections; ++i )
	{
		ib.push_back( 0 );
		ib.push_back( 1 + i );
		ib.push_back( 1 + i + 1 );
	}

	// top
	vb.push_back( SAuxObjVertex( Vec3( 0.0f, height, 0.0f ), Vec3( 0.0f, 1.0f, 0.0f ) ) );

	for( uint32 i( 0 ); i <= sections; ++i )
	{
		Vec3 v;
		v.x = radius * cosf( i * sectionSlice );
		v.y = 0.0f;
		v.z = radius * sinf( i * sectionSlice );

		Vec3 v1;
		v1.x = radius * cosf( i * sectionSlice + 0.01f );
		v1.y = 0.0f;
		v1.z = radius * sinf( i * sectionSlice + 0.01f );

		Vec3 d( v1 - v );
		Vec3 d1( Vec3( 0.0, height, 0.0f ) - v );

		Vec3 n( ( d1.Cross( d ) ).normalized() );
		vb.push_back( SAuxObjVertex( v, n ) );
	}

	// build faces
	for( uint16 i( 0 ); i < sections; ++i )
	{
		ib.push_back( sections + 2 );
		ib.push_back( sections + 3 + i + 1 );
		ib.push_back( sections + 3 + i );
	}
}


// function to generate a cylinder mesh
static void CreateCylinder( AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib, float radius, float height, unsigned int sections )
{
	// calc required number of vertices/indices/triangles to build a cylinder for the given parameters
	uint32 numVertices( 4 * ( sections + 1 ) + 2 );
	uint32 numTriangles( 4 * sections );
	uint32 numIndices( numTriangles * 3 );

	// setup buffers
	vb.clear();
	vb.reserve( numVertices );

	ib.clear();
	ib.reserve( numIndices );

	float sectionSlice( DEG2RAD( 360.0f / (float) sections ) );

	// bottom cap
	{
		// center bottom vertex
		vb.push_back( SAuxObjVertex( Vec3( 0.0f, -0.5f * height, 0.0f ), Vec3( 0.0f, -1.0f, 0.0f ) ) );

		// create circle around it
		for( uint32 i( 0 ); i <= sections; ++i )
		{
			Vec3 v;
			v.x = radius * cosf( i * sectionSlice );
			v.y = -0.5f * height;
			v.z = radius * sinf( i * sectionSlice );
			vb.push_back( SAuxObjVertex( v, Vec3( 0.0f, -1.0f, 0.0f ) ) );
		}

		// build faces
		for( uint16 i( 0 ); i < sections; ++i )
		{
			ib.push_back( 0 );
			ib.push_back( 1 + i );
			ib.push_back( 1 + i + 1 );
		}
	}

	// side
	{
		int vIdx( vb.size() );

		for( uint32 i( 0 ); i <= sections; ++i )
		{
			Vec3 v;
			v.x = radius * cosf( i * sectionSlice );
			v.y = -0.5f * height;
			v.z = radius * sinf( i * sectionSlice );

			Vec3 n( v.normalized() );
			vb.push_back( SAuxObjVertex( v, n ) );
			vb.push_back( SAuxObjVertex( Vec3( v.x, -v.y, v.z ), n ) );
		}

		// build faces
		for( uint16 i( 0 ); i < sections; ++i, vIdx += 2 )
		{
			ib.push_back( vIdx );
			ib.push_back( vIdx + 1 );
			ib.push_back( vIdx + 2 );

			ib.push_back( vIdx + 1 );
			ib.push_back( vIdx + 3 );
			ib.push_back( vIdx + 2 );

		}
	}

	// top cap
	{
		int vIdx( vb.size() );

		// center top vertex
		vb.push_back( SAuxObjVertex( Vec3( 0.0f, 0.5f * height, 0.0f ), Vec3( 0.0f, 1.0f, 0.0f ) ) );

		// create circle around it
		for( uint32 i( 0 ); i <= sections; ++i )
		{
			Vec3 v;
			v.x = radius * cosf( i * sectionSlice );
			v.y = 0.5f * height;
			v.z = radius * sinf( i * sectionSlice );
			vb.push_back( SAuxObjVertex( v, Vec3( 0.0f, 1.0f, 0.0f ) ) );
		}

		// build faces
		for( uint16 i( 0 ); i < sections; ++i )
		{
			ib.push_back( vIdx );
			ib.push_back( vIdx + 1 + i + 1 );
			ib.push_back( vIdx + 1 + i );
		}
	}
}


// Functor to generate a sphere mesh. To be used with generalized CreateMesh function.
struct SSphereMeshCreateFunc
{
	SSphereMeshCreateFunc( float radius, unsigned int rings, unsigned int sections )
		: m_radius( radius )
		, m_rings( rings )
		, m_sections( sections )
	{
	}

	void CreateMesh( AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib )
	{
		CreateSphere( vb, ib, m_radius, m_rings, m_sections );
	}

	float m_radius;
	unsigned int m_rings;
	unsigned int m_sections;
};


// Functor to generate a cone mesh. To be used with generalized CreateMesh function.
struct SConeMeshCreateFunc
{
	SConeMeshCreateFunc( float radius, float height, unsigned int sections )
		: m_radius( radius )
		, m_height( height )
		, m_sections( sections )
	{
	}

	void CreateMesh( AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib )
	{
		CreateCone( vb, ib, m_radius, m_height, m_sections );
	}

	float m_radius;
	float m_height;
	unsigned int m_sections;
};


// Functor to generate a cylinder mesh. To be used with generalized CreateMesh function.
struct SCylinderMeshCreateFunc
{
	SCylinderMeshCreateFunc( float radius, float height, unsigned int sections )
		: m_radius( radius )
		, m_height( height )
		, m_sections( sections )
	{
	}

	void CreateMesh( AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib )
	{
		CreateCylinder( vb, ib, m_radius, m_height, m_sections );
	}

	float m_radius;
	float m_height;
	unsigned int m_sections;
};


// Generalized CreateMesh function to create any mesh via passed-in functor.
// The functor needs to provide a CreateMesh function which accepts an 
// AuxObjVertexBuffer and AuxObjIndexBuffer to stored the resulting mesh.
template< typename TMeshFunc >
HRESULT CRenderAuxGeomD3D::CreateMesh( SDrawObjMesh& mesh, TMeshFunc meshFunc )
{
	// create mesh
	std::vector< SAuxObjVertex > vb;
	std::vector< uint16 > ib;
	meshFunc.CreateMesh( vb, ib );

	// create vertex buffer and copy data
	HRESULT hr( S_OK );
#if defined (DIRECT3D9) || defined(OPENGL)
	if( FAILED( hr = m_renderer.m_pd3dDevice->CreateVertexBuffer( vb.size() * sizeof( SAuxObjVertex ), 0, 0, D3DPOOL_MANAGED, &mesh.m_pVB, 0 ) ) )
	{
		return( hr );
	}
	SAuxObjVertex* pVertices( 0 );
	if( FAILED( hr = mesh.m_pVB->Lock( 0, 0, (void **) &pVertices, 0 ) ) )
	{
		return( hr );
	}

	memcpy( pVertices, &vb[ 0 ], vb.size() * sizeof( SAuxObjVertex ) );
	hr = mesh.m_pVB->Unlock();
#elif defined (DIRECT3D10)
	D3D11_BUFFER_DESC BufDescV;
	ZeroStruct(BufDescV);
	BufDescV.ByteWidth = vb.size() * sizeof( SAuxObjVertex );
	BufDescV.Usage = D3D11_USAGE_DEFAULT;
	BufDescV.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufDescV.CPUAccessFlags = 0;
	BufDescV.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &vb[0];
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;
	if( FAILED( hr = m_renderer.m_pd3dDevice->CreateBuffer( &BufDescV, &InitData, &mesh.m_pVB ) ) )
	{
		assert(SUCCEEDED(hr));
		return( hr );
	}
#endif

#if defined (DIRECT3D9) || defined(OPENGL)
	// create index buffer and copy data
	if( FAILED( hr = m_renderer.m_pd3dDevice->CreateIndexBuffer( ib.size() * sizeof( uint16 ), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &mesh.m_pIB, 0 ) ) )
	{
		return( hr );
	}
	uint16* pIndices( 0 );
	if( FAILED( hr = mesh.m_pIB->Lock( 0, 0, (void **) &pIndices, 0 ) ) )
	{
		return( hr );
	}

	memcpy( pIndices, &ib[ 0 ], ib.size() * sizeof( uint16 ) );
	hr = mesh.m_pIB->Unlock();
#elif defined (DIRECT3D10)
	D3D11_BUFFER_DESC BufDescI;
	ZeroStruct(BufDescI);
	BufDescI.ByteWidth = ib.size() * sizeof( uint16 );
	BufDescI.Usage = D3D11_USAGE_DEFAULT;
	BufDescI.BindFlags = D3D11_BIND_INDEX_BUFFER;
	BufDescI.CPUAccessFlags = 0;
	BufDescI.MiscFlags = 0;

	InitData.pSysMem = &ib[0];
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;
	if( FAILED( hr = m_renderer.m_pd3dDevice->CreateBuffer(&BufDescI, &InitData, &mesh.m_pIB ) ) )
	{
		assert(SUCCEEDED(hr));
		return( hr );
	}
#endif

	// write mesh info
	mesh.m_numVertices = vb.size();
	mesh.m_numFaces = ib.size() / 3;

	return( hr );
}


HRESULT CRenderAuxGeomD3D::RestoreDeviceObjects()
{
	HRESULT hr( S_OK );
#if defined (DIRECT3D9) || defined(OPENGL)
	LPDIRECT3DDEVICE9 pd3dDevice( m_renderer.m_pd3dDevice );

	// recreate vertex buffer 
	SAFE_RELEASE( m_pAuxGeomVB );
	if( FAILED( hr = pd3dDevice->CreateVertexBuffer( e_auxGeomVBSize * sizeof( SAuxVertex ), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 
		0, D3DPOOL_DEFAULT, &m_pAuxGeomVB, NULL ) ) )
	{
		return( hr );
	}

	// recreate index buffer
	SAFE_RELEASE( m_pAuxGeomIB );
	if( FAILED( hr = pd3dDevice->CreateIndexBuffer( e_auxGeomIBSize * sizeof( uint16 ), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 
		D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pAuxGeomIB, NULL ) ) )
	{
		return( hr );
	}
#elif defined (DIRECT3D10)
	D3DDevice* pd3dDevice( m_renderer.m_pd3dDevice );

	// recreate vertex buffer 
	SAFE_RELEASE( m_pAuxGeomVB );

	D3D11_BUFFER_DESC BufDescV;
	ZeroStruct(BufDescV);
	BufDescV.ByteWidth = e_auxGeomVBSize * sizeof( SAuxVertex );
	BufDescV.Usage = D3D11_USAGE_DYNAMIC;
	BufDescV.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufDescV.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufDescV.MiscFlags = 0;

	if( FAILED( hr = pd3dDevice->CreateBuffer( &BufDescV , 0, &m_pAuxGeomVB ) ) )
	{
		assert(0);
		return( hr );
	}

	// recreate index buffer
	SAFE_RELEASE( m_pAuxGeomIB );
	D3D11_BUFFER_DESC BufDescI;
	ZeroStruct(BufDescI);
	BufDescI.ByteWidth =  e_auxGeomIBSize * sizeof( uint16 );
	BufDescI.Usage = D3D11_USAGE_DYNAMIC;
	BufDescI.BindFlags = D3D11_BIND_INDEX_BUFFER;
	BufDescI.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufDescI.MiscFlags = 0;
	if( FAILED( hr = pd3dDevice->CreateBuffer( &BufDescI, 0, &m_pAuxGeomIB ) ) )
	{
		assert(0);
		return( hr );
	}
#endif

	// recreate aux objects
	for( uint32 i( 0 ); i < e_auxObjNumLOD; ++i )
	{
		m_sphereObj[ i ].Release();
		if( FAILED( hr = CreateMesh( m_sphereObj[ i ], SSphereMeshCreateFunc( 1.0f, 9 + 4 * i, 9 + 4 * i ) ) ) )
		{
			return( hr );
		}

		m_coneObj[ i ].Release();
		if( FAILED( hr = CreateMesh( m_coneObj[ i ], SConeMeshCreateFunc( 1.0f, 1.0f, 10 + i * 6 ) ) ) )
		{
			return( hr );
		}

		m_cylinderObj[ i ].Release();
		if( FAILED( hr = CreateMesh( m_cylinderObj[ i ], SCylinderMeshCreateFunc( 1.0f, 1.0f, 10 + i * 6 ) ) ) )
		{
			return( hr );
		}
	}
	return( hr );
}


void CRenderAuxGeomD3D::DrawAuxPrimitives( CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd, const CAuxGeomCB::EPrimType& primType )
{
	assert( CAuxGeomCB::e_PtList == primType || CAuxGeomCB::e_LineList == primType || CAuxGeomCB::e_TriList == primType );

	HRESULT hr( S_OK );

	bool streamsBound = false;
#if !defined(XENON)
	// bind vertex and index streams and set vertex declaration
	streamsBound = BindStreams( eVF_P3F_C4B_T2F, m_pAuxGeomVB, m_pAuxGeomIB );
#endif

	// get aux vertex buffer
	const CAuxGeomCB::AuxVertexBuffer& auxVertexBuffer( GetAuxVertexBuffer() );

	// determine flags for prim type
	uint32 d3dNumPrimDivider;
#if defined (DIRECT3D9) || defined(OPENGL)
	D3DPRIMITIVETYPE d3dPrim;
#elif defined (DIRECT3D10)
	D3D11_PRIMITIVE_TOPOLOGY d3dPrim;
#endif
	DetermineAuxPrimitveFlags( d3dNumPrimDivider, d3dPrim, primType );

	// helpers for DP call
	uint32 initialVBLockOffset( m_auxGeomSBM.m_curVBIndex );
	uint32 numVerticesWrittenToVB( 0 );

	m_renderer.FX_Commit();

	// process each entry
	for( CAuxGeomCB::AuxSortedPushBuffer::const_iterator it( itBegin ); it != itEnd; ++it )
	{
		// get current push buffer entry
		const CAuxGeomCB::SAuxPushBufferEntry* curPBEntry( *it );

		// number of vertices to copy
		uint32 verticesToCopy( curPBEntry->m_numVertices );
		uint32 verticesCopied( 0 );

		// stream vertex data
		while( verticesToCopy > 0 )
		{
			// number of vertices which fit into current vb
			uint32 maxVerticesInThisBatch( e_auxGeomVBSize - m_auxGeomSBM.m_curVBIndex );

			// round down to previous multiple of "d3dNumPrimDivider"
			maxVerticesInThisBatch -= maxVerticesInThisBatch % d3dNumPrimDivider;

			// still enough space to feed data in the current vb
			if( maxVerticesInThisBatch > 0 )
			{
				// compute amount of vertices to move in this batch
				uint32 toCopy( verticesToCopy > maxVerticesInThisBatch ? maxVerticesInThisBatch : verticesToCopy );

				// get pointer to vertex buffer
				SAuxVertex* pVertices( 0 );
#if defined (DIRECT3D9) || defined(OPENGL)
				// determine lock flags
				uint32 lockFlags( D3DLOCK_NOOVERWRITE );
				if( false != m_auxGeomSBM.m_discardVB )
				{
					m_auxGeomSBM.m_discardVB = false;
					lockFlags = D3DLOCK_DISCARD;
				}

#	if !defined(XENON)
				if( FAILED( hr = m_pAuxGeomVB->Lock( m_auxGeomSBM.m_curVBIndex * sizeof( SAuxVertex ), toCopy * sizeof( SAuxVertex ), (void**) &pVertices, lockFlags ) ) )
				{
					assert( 0 );
					iLog->Log( "ERROR: CD3DRenderAuxGeom::DrawAuxPrimitives() - Vertex buffer could not be locked!" );
					return;
				}




#	endif
#elif defined (DIRECT3D10)
				// determine lock flags
				D3D11_MAP mapFlags( D3D11_MAP_WRITE_NO_OVERWRITE );
				if( false != m_auxGeomSBM.m_discardVB )
				{
					m_auxGeomSBM.m_discardVB = false;
					mapFlags = D3D11_MAP_WRITE_DISCARD;
				}

				D3D11_MAPPED_SUBRESOURCE mappedResource;
				if( FAILED( hr = m_renderer.m_pd3dDeviceContext->Map(m_pAuxGeomVB, 0, mapFlags, 0, &mappedResource ) ) )
				{
					assert( 0 );
					iLog->Log( "ERROR: CD3DRenderAuxGeom::DrawAuxPrimitives() - Vertex buffer could not be locked!" );
					return;
				}
				pVertices = (SAuxVertex*)mappedResource.pData;
				pVertices += m_auxGeomSBM.m_curVBIndex;
#endif

				// move vertex data
				memcpy( pVertices, &auxVertexBuffer[ curPBEntry->m_vertexOffs + verticesCopied ], toCopy * sizeof( SAuxVertex ) );

				// unlock vb
#if defined (DIRECT3D9) || defined(OPENGL)
				hr = m_pAuxGeomVB->Unlock();
#elif defined (DIRECT3D10)
				m_renderer.GetDeviceContext()->Unmap(m_pAuxGeomVB, 0);
#endif
				// update accumulators and buffer indices
				verticesCopied += toCopy;
				verticesToCopy -= toCopy;

				m_auxGeomSBM.m_curVBIndex += toCopy;
				numVerticesWrittenToVB += toCopy;
			}
			else
			{
				// not enough space in vb for (remainings of) current push buffer entry
				if( numVerticesWrittenToVB > 0 )
				{
					// commit batch 
					assert( 0 == numVerticesWrittenToVB % d3dNumPrimDivider );
#if defined (DIRECT3D9) || defined(OPENGL)



					if (streamsBound)
						hr = m_renderer.m_pd3dDevice->DrawPrimitive( d3dPrim, initialVBLockOffset, numVerticesWrittenToVB / d3dNumPrimDivider );
#elif defined (DIRECT3D10)
					m_renderer.SetPrimitiveTopology(d3dPrim);
					if (streamsBound)
						m_renderer.m_pd3dDeviceContext->Draw(numVerticesWrittenToVB, initialVBLockOffset);
#endif
				}

				// request a DISCARD lock of vb in the next run
				m_auxGeomSBM.DiscardVB();
				initialVBLockOffset = m_auxGeomSBM.m_curVBIndex;
				numVerticesWrittenToVB = 0;
			}
		}
	}

	if( numVerticesWrittenToVB > 0 )
	{
		// commit batch
		assert( 0 == numVerticesWrittenToVB % d3dNumPrimDivider );
#if defined (DIRECT3D9) || defined(OPENGL)



		if (streamsBound)
			hr = m_renderer.m_pd3dDevice->DrawPrimitive( d3dPrim, initialVBLockOffset, numVerticesWrittenToVB / d3dNumPrimDivider );
#elif defined (DIRECT3D10)
		m_renderer.SetPrimitiveTopology(d3dPrim);
		if (streamsBound)
			m_renderer.m_pd3dDeviceContext->Draw(numVerticesWrittenToVB, initialVBLockOffset);
#endif
	}
}


void CRenderAuxGeomD3D::DrawAuxIndexedPrimitives( CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd, const CAuxGeomCB::EPrimType& primType )
{
	assert( CAuxGeomCB::e_LineListInd == primType || CAuxGeomCB::e_TriListInd == primType );

	HRESULT hr(S_OK);

	bool streamsBound = false;
#if !defined(XENON)
	// bind vertex and index streams and set vertex declaration
	streamsBound = BindStreams( eVF_P3F_C4B_T2F, m_pAuxGeomVB, m_pAuxGeomIB );
#endif

	// get aux vertex and index buffer
	const CAuxGeomCB::AuxVertexBuffer& auxVertexBuffer( GetAuxVertexBuffer() );
	const CAuxGeomCB::AuxIndexBuffer& auxIndexBuffer( GetAuxIndexBuffer() );

	// determine flags for prim type
	uint32 d3dNumPrimDivider;
#if defined (DIRECT3D9) || defined(OPENGL)
	D3DPRIMITIVETYPE d3dPrim;
#elif defined (DIRECT3D10)
  D3D11_PRIMITIVE_TOPOLOGY d3dPrim;
#endif
	DetermineAuxPrimitveFlags( d3dNumPrimDivider, d3dPrim, primType );

	// helpers for DP call
	uint32 initialVBLockOffset( m_auxGeomSBM.m_curVBIndex );
	uint32 numVerticesWrittenToVB( 0 );
	uint32 initialIBLockOffset( m_auxGeomSBM.m_curIBIndex );
	uint32 numIndicesWrittenToIB( 0 );

	m_renderer.FX_Commit();

	// process each entry
	for( CAuxGeomCB::AuxSortedPushBuffer::const_iterator it( itBegin ); it != itEnd; )
	{
		// get current push buffer entry
		const CAuxGeomCB::SAuxPushBufferEntry* curPBEntry( *it );

		// process a push buffer entry if it can fit at all (otherwise silently skip it)
		if( e_auxGeomVBSize >= curPBEntry->m_numVertices && e_auxGeomIBSize >= curPBEntry->m_numIndices )
		{
			// check if push buffer still fits into current buffer
			if( e_auxGeomVBSize >= m_auxGeomSBM.m_curVBIndex + curPBEntry->m_numVertices && e_auxGeomIBSize >= m_auxGeomSBM.m_curIBIndex + curPBEntry->m_numIndices )
			{
				// determine lock vb flags

				// get pointer to vertex buffer
				SAuxVertex* pVertices( 0 );
#if defined (DIRECT3D9) || defined(OPENGL)
				uint32 lockFlags( D3DLOCK_NOOVERWRITE );
				if( false != m_auxGeomSBM.m_discardVB )
				{
					m_auxGeomSBM.m_discardVB = false;
					lockFlags = D3DLOCK_DISCARD;
				}
#	if !defined(XENON)
				if( FAILED( hr = m_pAuxGeomVB->Lock( m_auxGeomSBM.m_curVBIndex * sizeof( SAuxVertex ), curPBEntry->m_numVertices * sizeof( SAuxVertex ), (void**) &pVertices, lockFlags ) ) )
				{
					assert( 0 );
					iLog->Log( "ERROR: CD3DRenderAuxGeom::DrawAuxIndexedPrimitives() - Vertex buffer could not be locked!" );
					return;
				}




#	endif
#elif defined (DIRECT3D10)
				D3D11_MAP mp (D3D11_MAP_WRITE_NO_OVERWRITE);
				if( false != m_auxGeomSBM.m_discardVB )
				{
					m_auxGeomSBM.m_discardVB = false;
					mp = D3D11_MAP_WRITE_DISCARD;
				}
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				if( FAILED( hr = m_renderer.m_pd3dDeviceContext->Map(m_pAuxGeomVB, 0, mp, 0, &mappedResource) ) )
				{
					assert( 0 );
					iLog->Log( "ERROR: CD3DRenderAuxGeom::DrawAuxIndexedPrimitives() - Vertex buffer could not be locked!" );
					return;
				}
				pVertices = (SAuxVertex*)mappedResource.pData;
				pVertices += m_auxGeomSBM.m_curVBIndex;
#endif

				// move vertex data of this entry
				memcpy( pVertices, &auxVertexBuffer[ curPBEntry->m_vertexOffs ], curPBEntry->m_numVertices * sizeof( SAuxVertex ) );

				// unlock vb
#if defined (DIRECT3D9) || defined(OPENGL)
				hr = m_pAuxGeomVB->Unlock();
#elif defined (DIRECT3D10)
				m_renderer.GetDeviceContext()->Unmap(m_pAuxGeomVB, 0);
#endif

				// get pointer to index buffer
				uint16* pIndices( 0 );
#if defined (DIRECT3D9) || defined(OPENGL)
				// determine lock ib flags
				lockFlags = D3DLOCK_NOOVERWRITE;
				if( false != m_auxGeomSBM.m_discardIB )
				{
					m_auxGeomSBM.m_discardIB = false;
					lockFlags = D3DLOCK_DISCARD;
				}
#	if !defined(XENON)
				if( FAILED( hr = m_pAuxGeomIB->Lock( m_auxGeomSBM.m_curIBIndex * sizeof( uint16 ), curPBEntry->m_numIndices * sizeof( uint16 ), (void**) &pIndices, lockFlags ) ) )
				{
					assert( 0 );
					iLog->Log( "ERROR: CD3DRenderAuxGeom::DrawAuxIndexedPrimitives() - Index buffer could not be locked!" );

					m_pAuxGeomVB->Unlock();
					return;
				}









#	endif
#elif defined (DIRECT3D10)
				mp = D3D11_MAP_WRITE_NO_OVERWRITE;
				if( false != m_auxGeomSBM.m_discardIB )
				{
					m_auxGeomSBM.m_discardIB = false;
					mp = D3D11_MAP_WRITE_DISCARD;
				}

				if( FAILED( hr = m_renderer.m_pd3dDeviceContext->Map(m_pAuxGeomIB, 0, mp, 0, &mappedResource) ) )
				{
					assert( 0 );
					iLog->Log( "ERROR: CD3DRenderAuxGeom::DrawAuxIndexedPrimitives() - Index buffer could not be locked!" );

					m_renderer.GetDeviceContext()->Unmap(m_pAuxGeomVB, 0);
					return;
				}
				pIndices = (uint16*)mappedResource.pData;
				pIndices += m_auxGeomSBM.m_curIBIndex;
#endif

				// move index data of this entry (modify indices to match VB insert location)
				for( uint32 i( 0 ); i < curPBEntry->m_numIndices; ++i )
				{
					pIndices[ i ] = numVerticesWrittenToVB + auxIndexBuffer[ curPBEntry->m_indexOffs + i ];
				}

				// unlock ib
#if defined (DIRECT3D9) || defined(OPENGL)
				hr = m_pAuxGeomIB->Unlock();
#elif defined (DIRECT3D10)
				m_renderer.GetDeviceContext()->Unmap(m_pAuxGeomIB, 0);
#endif

				// update buffer indices
				m_auxGeomSBM.m_curVBIndex += curPBEntry->m_numVertices;
				m_auxGeomSBM.m_curIBIndex += curPBEntry->m_numIndices;

				numVerticesWrittenToVB += curPBEntry->m_numVertices;
				numIndicesWrittenToIB += curPBEntry->m_numIndices;

				// advance to next push puffer entry
				++it;
			}
			else
			{
				// push buffer entry currently doesn't fit, will be processed in the next iteration when buffers got flushed
				if( numVerticesWrittenToVB > 0 && numIndicesWrittenToIB > 0 )
				{
					// commit batch 
					assert( 0 == numIndicesWrittenToIB % d3dNumPrimDivider );
#if defined (DIRECT3D9) || defined(OPENGL)



					if (streamsBound)
						hr = m_renderer.m_pd3dDevice->DrawIndexedPrimitive( d3dPrim, initialVBLockOffset, 0, numVerticesWrittenToVB, initialIBLockOffset, numIndicesWrittenToIB / d3dNumPrimDivider );
#elif defined (DIRECT3D10)
					m_renderer.SetPrimitiveTopology(d3dPrim);
					if (streamsBound)
						m_renderer.m_pd3dDeviceContext->DrawIndexed(numIndicesWrittenToIB, initialIBLockOffset, initialVBLockOffset);
#endif
				}

				// request a DISCARD lock / don't advance iterator!
				m_auxGeomSBM.DiscardVB();
				initialVBLockOffset = m_auxGeomSBM.m_curVBIndex;
				numVerticesWrittenToVB = 0;

				m_auxGeomSBM.DiscardIB();
				initialIBLockOffset = m_auxGeomSBM.m_curIBIndex;
				numIndicesWrittenToIB = 0;
			}
		}
		else
		{
			// push buffer entry too big for dedicated vb/ib buffer
			// advance to next push puffer entry
			assert( 0 );
			iLog->Log( "ERROR: CD3DRenderAuxGeom::DrawAuxIndexedPrimitives() - Auxiliary geometry too big to render!" );
			++it;
		}
	}

	if( numVerticesWrittenToVB > 0 && numIndicesWrittenToIB > 0 )
	{
		// commit batch 
		assert( 0 == numIndicesWrittenToIB % d3dNumPrimDivider );
#if defined (DIRECT3D9) || defined(OPENGL)



		if (streamsBound)
			hr = m_renderer.m_pd3dDevice->DrawIndexedPrimitive( d3dPrim, initialVBLockOffset, 0, numVerticesWrittenToVB, initialIBLockOffset, numIndicesWrittenToIB / d3dNumPrimDivider );
#elif defined (DIRECT3D10)
		m_renderer.SetPrimitiveTopology(d3dPrim);
		if (streamsBound)
			m_renderer.m_pd3dDeviceContext->DrawIndexed(numIndicesWrittenToIB, initialIBLockOffset, initialVBLockOffset);
#endif
	}
}


void CRenderAuxGeomD3D::DrawAuxObjects( CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd )
{
	CAuxGeomCB::EAuxDrawObjType objType( CAuxGeomCB::GetAuxObjType( (*itBegin)->m_renderFlags ) );

	// get draw params buffer
	const CAuxGeomCB::AuxDrawObjParamBuffer& auxDrawObjParamBuffer( GetAuxDrawObjParamBuffer() );

	// process each entry
	for( CAuxGeomCB::AuxSortedPushBuffer::const_iterator it( itBegin ); it != itEnd; ++it )
	{
		// get current push buffer entry
		const CAuxGeomCB::SAuxPushBufferEntry* curPBEntry( *it );

		// assert than all objects in this batch are of same type
		assert( CAuxGeomCB::GetAuxObjType( curPBEntry->m_renderFlags ) == objType );

		uint32 drawParamOffs( 0 );
		if( curPBEntry->GetDrawParamOffs( drawParamOffs ) )
		{
			// get draw params
			const CAuxGeomCB::SAuxDrawObjParams& drawParams( auxDrawObjParamBuffer[ drawParamOffs ] );

			// Prepare d3d world space matrix in draw param structure 
			// Attention: in d3d terms matWorld is actually matWorld^T
			Matrix44A matWorld;
			matWorld.SetIdentity();
			memcpy( &matWorld, &drawParams.m_matWorld, sizeof( drawParams.m_matWorld ) );

			// set transformation matrices
			static CCryName matWorldViewProjName("matWorldViewProj");
			if( m_curDrawInFrontMode == e_DrawInFrontOn )
			{
				Matrix44A matScale(Matrix44A(Matrix34::CreateScale(Vec3(0.999f, 0.999f, 0.999f))));

				Matrix44A matWorldViewScaleProjT;
				matWorldViewScaleProjT.Multiply(GetCurrentView(), matScale);
				matWorldViewScaleProjT = matWorldViewScaleProjT * GetCurrentProj();

				matWorldViewScaleProjT.Transpose(matWorldViewScaleProjT);
				matWorldViewScaleProjT = matWorldViewScaleProjT * matWorld;

				m_pAuxGeomShader->FXSetVSFloat( matWorldViewProjName, (Vec4*) &matWorldViewScaleProjT, 4 );
			}
			else
			{
				Matrix44A matWorldViewProjT;
				matWorldViewProjT.Transpose(*m_matrices.m_pCurTransMat);
				matWorldViewProjT = matWorldViewProjT * matWorld;
				m_pAuxGeomShader->FXSetVSFloat( matWorldViewProjName, (Vec4*) &matWorldViewProjT, 4 );
			}

			// set color
			ColorF col(drawParams.m_color);
			Vec4 colVec( col.b, col.g, col.r, col.a ); // need to flip r/b as drawParams.m_color was originally argb
			static CCryName auxGeomObjColorName("auxGeomObjColor");
			m_pAuxGeomShader->FXSetVSFloat( auxGeomObjColorName, &colVec, 1 );

			// set shading flag
			Vec4 shadingVec( drawParams.m_shaded ? 0.4f : 0, drawParams.m_shaded ? 0.6f : 1, 0, 0 );
			static CCryName auxGeomObjShadingName("auxGeomObjShading");
			m_pAuxGeomShader->FXSetVSFloat( auxGeomObjShadingName, &shadingVec, 1 );

			// set light vector (rotate back into local space)
			Matrix33 matWorldInv( drawParams.m_matWorld.GetInverted() );
			Vec3 lightLocalSpace( matWorldInv * Vec3( 0.5773f, 0.5773f, 0.5773f ) );
			
			// normalize light vector (matWorld could contain non-uniform scaling)
			lightLocalSpace.Normalize();
			Vec4 lightVec( lightLocalSpace.x, lightLocalSpace.y, lightLocalSpace.z, 0.0f );
			static CCryName globalLightLocalName("globalLightLocal");
			m_pAuxGeomShader->FXSetVSFloat( globalLightLocalName, &lightVec, 1 );
			
			// LOD calculation
			Matrix44A matWorldT;
			matWorldT.Transpose(matWorld);

			D3DXVECTOR4 objCenterWorld;
			D3DXVECTOR3 nullVec( 0.0f, 0.0f, 0.0f );
			mathVec3TransformF( &objCenterWorld, &nullVec, &matWorldT );			
			D3DXVECTOR4 objOuterRightWorld( objCenterWorld + (D3DXVECTOR4( GetCurrentView().m00, GetCurrentView().m10, GetCurrentView().m20, 0.0f ) * drawParams.m_size) );

			D3DXVECTOR4 v0, v1;

			D3DXVECTOR3 objCenterWorldVec( objCenterWorld.x, objCenterWorld.y, objCenterWorld.z );
			D3DXVECTOR3 objOuterRightWorldVec( objOuterRightWorld.x, objOuterRightWorld.y, objOuterRightWorld.z );
			mathVec3TransformF( &v0, &objCenterWorldVec, m_matrices.m_pCurTransMat );
			mathVec3TransformF( &v1, &objOuterRightWorldVec, m_matrices.m_pCurTransMat );

			float scale;
			assert( fabs(v0.w - v0.w ) < 1e-4 );
			if( fabs( v0.w ) < 1e-2 ) 
			{
				scale = 0.5f;
			}
			else
			{
				scale = ( ( v1.x - v0.x ) / v0.w ) * (float) max( m_wndXRes, m_wndYRes ) / 500.0f;
			}

			// map scale to detail level
			uint32 lodLevel( (uint32) ( ( scale / 0.5f ) * ( e_auxObjNumLOD - 1 ) ) );
			if( lodLevel >= e_auxObjNumLOD )
			{
				lodLevel = e_auxObjNumLOD - 1;
			}

			// get appropriate mesh
			assert( lodLevel >= 0 && lodLevel < e_auxObjNumLOD );
			SDrawObjMesh* pMesh( 0 );
			switch( objType )
			{
			case CAuxGeomCB::eDOT_Sphere:
			default:
				{
					pMesh = &m_sphereObj[ lodLevel ];
					break;
				}
			case CAuxGeomCB::eDOT_Cone:
				{
					pMesh = &m_coneObj[ lodLevel ];
					break;
				}
			case CAuxGeomCB::eDOT_Cylinder:
				{
					pMesh = &m_cylinderObj[ lodLevel ];
					break;
				}
			}
			assert( 0 != pMesh );

			// bind vertex and index streams and set vertex declaration
			if (BindStreams(eVF_P3F_T3F, pMesh->m_pVB, pMesh->m_pIB))
			{
				m_renderer.FX_Commit();

				// draw mesh
#if defined (DIRECT3D9) || defined(OPENGL)
				HRESULT hr = m_renderer.m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, pMesh->m_numVertices, 0, pMesh->m_numFaces );
#elif defined (DIRECT3D10)
				m_renderer.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				m_renderer.m_pd3dDeviceContext->DrawIndexed(pMesh->m_numFaces * 3, 0, 0);
#endif

				//// debug code to visualize the LOD scale value
				//// to use it remove the multiplications of matWorld in the "set transformation matrices" code above
				// 
				//SAuxObjVertex line[ 2 ] =
				//{
				//	SAuxObjVertex( Vec3( objCenterWorld.x, objCenterWorld.y, objCenterWorld.z ), Vec3( 0, 0, 0 ) ),
				//	SAuxObjVertex( Vec3( objOuterRightWorld.x, objOuterRightWorld.y, objOuterRightWorld.z ), Vec3( 0, 0, 0 ) )
				//};
				//hr = pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, &line, sizeof( SAuxObjVertex ) );
			}
		}
		else
		{
			assert( 0 ); // GetDrawParamOffs( ... ) failed -- corrupt data in push buffer?
		}
	}
}


static inline D3DXVECTOR3 IntersectLinePlane( const D3DXVECTOR3& o, const D3DXVECTOR3& d, const D3DXPLANE& p, float& t )
{
	t = -( D3DXPlaneDotNormal( &p, &o ) + ( p.d + c_clipThres ) ) / D3DXPlaneDotNormal( &p, &d );
	return( o + d * t);
}


static inline DWORD ClipColor( const DWORD& c0, const DWORD& c1, float t )
{
	D3DXCOLOR v0( c0 ), v1( c1 );
	D3DXCOLOR vRes( v0 + ( v1 - v0 ) * t );
	return( D3DCOLOR_COLORVALUE( vRes.r, vRes.g, vRes.b, vRes.a ) );
}


static bool ClipLine( D3DXVECTOR3* v, DWORD* c )
{
	// get near plane to perform clipping	
	const Plane& np( *gRenDev->GetCamera().GetFrustumPlane( FR_PLANE_NEAR ) );
	D3DXPLANE nearPlane( np.n.x, np.n.y, np.n.z, np.d );

	// get clipping flags
	bool bV0Behind( -( D3DXPlaneDotNormal( &nearPlane, &v[ 0 ] ) + nearPlane.d ) < c_clipThres );
	bool bV1Behind( -( D3DXPlaneDotNormal( &nearPlane, &v[ 1 ] ) + nearPlane.d ) < c_clipThres );

	// proceed only if both are not behind near clipping plane
	if( false == bV0Behind || false == bV1Behind )
	{
		if( false == bV0Behind && false == bV1Behind )
		{
			// no clipping needed
			return( true );
		}

		// define line to be clipped
		D3DXVECTOR3 p( v[ 0 ] );
		D3DXVECTOR3 d( v[ 1 ] - v[ 0 ] );

		// get clipped position
		float t;
		v[ 0 ] = ( false == bV0Behind ) ? v[ 0 ] : IntersectLinePlane( p, d, nearPlane, t );
		v[ 1 ] = ( false == bV1Behind ) ? v[ 1 ] : IntersectLinePlane( p, d, nearPlane, t );

		// get clipped colors
		c[ 0 ] = ( false == bV0Behind ) ? c[ 0 ] : ClipColor( c[ 0 ], c[ 1 ], t );
		c[ 1 ] = ( false == bV1Behind ) ? c[ 1 ] : ClipColor( c[ 0 ], c[ 1 ], t );

		return( true );
	}
	else
	{
		return( false );
	}
}


static float ComputeConstantScale( const D3DXVECTOR3& v, const Matrix44A& matView, const Matrix44A& matProj, const uint32 wndXRes )
{
	D3DXVECTOR4 vCam0; 
	mathVec3TransformF( &vCam0, &v, &matView );

	D3DXVECTOR4 vCam1( vCam0 );
	vCam1.x += 1.0f;

	float c0( ( vCam0.x * matProj.m00 + 
		vCam0.y * matProj.m10 + 
		vCam0.z * matProj.m20 + 
		matProj.m30 ) / ( vCam0.x * matProj.m03 + 
		vCam0.y * matProj.m13 + 
		vCam0.z * matProj.m23 + 
		matProj.m33 ) );

	float c1( ( vCam1.x * matProj.m00 + 
		vCam1.y * matProj.m10 + 
		vCam1.z * matProj.m20 + 
		matProj.m30 ) / ( vCam1.x * matProj.m03 + 
		vCam1.y * matProj.m13 + 
		vCam1.z * matProj.m23 + 
		matProj.m33 ) );

	return( 1.0f / ( (float) wndXRes * ( c1 - c0 ) ) );
}


void CRenderAuxGeomD3D::PrepareThickLines3D( CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd )
{
	const CAuxGeomCB::AuxVertexBuffer& auxVertexBuffer( GetAuxVertexBuffer() );

	// process each entry
	for( CAuxGeomCB::AuxSortedPushBuffer::const_iterator it( itBegin ); it != itEnd; ++it )
	{
		// get current push buffer entry
		const CAuxGeomCB::SAuxPushBufferEntry* curPBEntry( *it );

		uint32 offset( curPBEntry->m_vertexOffs );
		for( uint32 i( 0 ); i < curPBEntry->m_numVertices / 6; ++i, offset += 6 )
		{
			// get line vertices and thickness parameter
			D3DXVECTOR3 v[ 2 ] = 
			{
				D3DXVECTOR3( (const float*) &(auxVertexBuffer[ offset + 0 ].xyz.x) ),
					D3DXVECTOR3( (const float*) &(auxVertexBuffer[ offset + 1 ].xyz.x) )
			};
			DWORD col[ 2 ] =
			{
				auxVertexBuffer[ offset + 0 ].color.dcolor,
					auxVertexBuffer[ offset + 1 ].color.dcolor
			};
			float thickness( auxVertexBuffer[ offset + 2 ].xyz.x );

			bool skipLine( false );
			D3DXVECTOR4 vf[ 4 ];

			if( false == IsOrthoMode() ) // regular, 3d projected geometry
			{
				skipLine = !ClipLine( v, col );
				if( false == skipLine )
				{
					// compute depth corrected thickness of line end points
					float thicknessV0( 0.5f * thickness * ComputeConstantScale( v[ 0 ], GetCurrentView(), GetCurrentProj(), m_wndXRes ) );
					float thicknessV1( 0.5f * thickness * ComputeConstantScale( v[ 1 ], GetCurrentView(), GetCurrentProj(), m_wndXRes ) );

					// compute camera space line delta
					D3DXVECTOR4 vt[ 2 ];
					mathVec3TransformF( &vt[ 0 ], &v[ 0 ], &GetCurrentView() );
					mathVec3TransformF( &vt[ 1 ], &v[ 1 ], &GetCurrentView() );
					D3DXVECTOR2 delta( vt[ 1 ] / vt[ 1 ].z - vt[ 0 ] / vt[ 0 ].z );

					// create screen space normal of line delta
					D3DXVECTOR2 normal( -delta.y , delta.x );
					mathVec2NormalizeF( &normal, &normal );

					D3DXVECTOR2 n[ 2 ];
					n[ 0 ] = normal * thicknessV0;
					n[ 1 ] = normal * thicknessV1;

					// compute final world space vertices of thick line
					D3DXVECTOR4 vertices[4] = 
						{	
							D3DXVECTOR4( vt[ 0 ].x + n[ 0 ].x, vt[ 0 ].y + n[ 0 ].y, vt[ 0 ].z, vt[ 0 ].w ),
							D3DXVECTOR4( vt[ 1 ].x + n[ 1 ].x, vt[ 1 ].y + n[ 1 ].y, vt[ 1 ].z, vt[ 1 ].w ),
							D3DXVECTOR4( vt[ 1 ].x - n[ 1 ].x, vt[ 1 ].y - n[ 1 ].y, vt[ 1 ].z, vt[ 1 ].w ),
							D3DXVECTOR4( vt[ 0 ].x - n[ 0 ].x, vt[ 0 ].y - n[ 0 ].y, vt[ 0 ].z, vt[ 0 ].w )
						};
					mathVec4TransformF( &vf[ 0 ], &vertices[0], &GetCurrentViewInv() );
					mathVec4TransformF( &vf[ 1 ], &vertices[1], &GetCurrentViewInv() );
					mathVec4TransformF( &vf[ 2 ], &vertices[2], &GetCurrentViewInv() );
					mathVec4TransformF( &vf[ 3 ], &vertices[3], &GetCurrentViewInv() );
				}
			}
			else // orthogonal projected geometry
			{
				// compute depth corrected thickness of line end points
				float thicknessV0( 0.5f * thickness * ComputeConstantScale( v[ 0 ], GetCurrentView(), GetCurrentProj(), m_wndXRes ) );
				float thicknessV1( 0.5f * thickness * ComputeConstantScale( v[ 1 ], GetCurrentView(), GetCurrentProj(), m_wndXRes ) );

				// compute line delta
				D3DXVECTOR2 delta( v[ 1 ] - v[ 0 ] );

				// create normal of line delta
				D3DXVECTOR2 normal( -delta.y , delta.x );
				mathVec2NormalizeF( &normal, &normal );

				D3DXVECTOR2 n[ 2 ];
				n[ 0 ] = normal * thicknessV0 * 2.0f;
				n[ 1 ] = normal * thicknessV1 * 2.0f;

				// compute final world space vertices of thick line
				vf[ 0 ] = D3DXVECTOR4( v[ 0 ].x + n[ 0 ].x, v[ 0 ].y + n[ 0 ].y, v[ 0 ].z, 1.0f );
				vf[ 1 ] = D3DXVECTOR4( v[ 1 ].x + n[ 1 ].x, v[ 1 ].y + n[ 1 ].y, v[ 1 ].z, 1.0f );
				vf[ 2 ] = D3DXVECTOR4( v[ 1 ].x - n[ 1 ].x, v[ 1 ].y - n[ 1 ].y, v[ 1 ].z, 1.0f );
				vf[ 3 ] = D3DXVECTOR4( v[ 0 ].x - n[ 0 ].x, v[ 0 ].y - n[ 0 ].y, v[ 0 ].z, 1.0f );
			}

			SAuxVertex* pVertices( const_cast<SAuxVertex*>( &auxVertexBuffer[ offset ] ) );
			if( false == skipLine )
			{
				// copy data to vertex buffer
				pVertices[ 0 ].xyz = Vec3( vf[ 0 ].x, vf[ 0 ].y, vf[ 0 ].z );
				pVertices[ 0 ].color.dcolor = col[ 0 ];
				pVertices[ 1 ].xyz = Vec3( vf[ 1 ].x, vf[ 1 ].y, vf[ 1 ].z );
				pVertices[ 1 ].color.dcolor = col[ 1 ];
				pVertices[ 2 ].xyz = Vec3( vf[ 2 ].x, vf[ 2 ].y, vf[ 2 ].z );
				pVertices[ 2 ].color.dcolor = col[ 1 ];
				pVertices[ 3 ].xyz = Vec3( vf[ 0 ].x, vf[ 0 ].y, vf[ 0 ].z );
				pVertices[ 3 ].color.dcolor = col[ 0 ];
				pVertices[ 4 ].xyz = Vec3( vf[ 2 ].x, vf[ 2 ].y, vf[ 2 ].z );
				pVertices[ 4 ].color.dcolor = col[ 1 ];
				pVertices[ 5 ].xyz = Vec3( vf[ 3 ].x, vf[ 3 ].y, vf[ 3 ].z );
				pVertices[ 5 ].color.dcolor = col[ 0 ];
			}
			else
			{
				// invalidate parameter data of thick line stored in vertex buffer 
				// (generates two black degenerated triangles at (0,0,0))
				memset( pVertices, 0, sizeof( SAuxVertex ) * 6 );
			}
		}
	}
}


void CRenderAuxGeomD3D::PrepareThickLines2D( CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd )
{
	const CAuxGeomCB::AuxVertexBuffer& auxVertexBuffer( GetAuxVertexBuffer() );

	// process each entry
	for( CAuxGeomCB::AuxSortedPushBuffer::const_iterator it( itBegin ); it != itEnd; ++it )
	{
		// get current push buffer entry
		const CAuxGeomCB::SAuxPushBufferEntry* curPBEntry( *it );

		uint32 offset( curPBEntry->m_vertexOffs );
		for( uint32 i( 0 ); i < curPBEntry->m_numVertices / 6; ++i, offset += 6 )
		{
			// get line vertices and thickness parameter
			const D3DXVECTOR3 v[ 2 ] = 
			{
				D3DXVECTOR3( (const float*) &(auxVertexBuffer[ offset + 0 ].xyz.x) ),
					D3DXVECTOR3( (const float*) &(auxVertexBuffer[ offset + 1 ].xyz.x) )
			};
			const DWORD col[ 2 ] =
			{
				auxVertexBuffer[ offset + 0 ].color.dcolor,
					auxVertexBuffer[ offset + 1 ].color.dcolor
			};
			float thickness( auxVertexBuffer[ offset + 2 ].xyz.x );

			// get line delta and aspect ratio corrected normal
			D3DXVECTOR3 delta( v[ 1 ] - v[ 0 ] );		
			D3DXVECTOR3 normal( -delta.y * m_aspectInv , delta.x * m_aspect, 0.0f );

			// normalize and scale to line thickness
			mathVec3NormalizeF( &normal, &normal );
			normal *= thickness * 0.001f;

			// compute final 2D vertices of thick line in normalized device space
			D3DXVECTOR3 vf[ 4 ];
			vf[ 0 ] = v[ 0 ] + normal;
			vf[ 1 ] = v[ 1 ] + normal;
			vf[ 2 ] = v[ 1 ] - normal;
			vf[ 3 ] = v[ 0 ] - normal;

			// copy data to vertex buffer
			SAuxVertex* pVertices( const_cast<SAuxVertex*>( &auxVertexBuffer[ offset ] ) );
			pVertices[ 0 ].xyz = Vec3( vf[ 0 ].x, vf[ 0 ].y, vf[ 0 ].z );
			pVertices[ 0 ].color.dcolor = col[ 0 ];
			pVertices[ 1 ].xyz = Vec3( vf[ 1 ].x, vf[ 1 ].y, vf[ 1 ].z );
			pVertices[ 1 ].color.dcolor = col[ 1 ];
			pVertices[ 2 ].xyz = Vec3( vf[ 2 ].x, vf[ 2 ].y, vf[ 2 ].z );
			pVertices[ 2 ].color.dcolor = col[ 1 ];
			pVertices[ 3 ].xyz = Vec3( vf[ 0 ].x, vf[ 0 ].y, vf[ 0 ].z );
			pVertices[ 3 ].color.dcolor = col[ 0 ];
			pVertices[ 4 ].xyz = Vec3( vf[ 2 ].x, vf[ 2 ].y, vf[ 2 ].z );
			pVertices[ 4 ].color.dcolor = col[ 1 ];
			pVertices[ 5 ].xyz = Vec3( vf[ 3 ].x, vf[ 3 ].y, vf[ 3 ].z );
			pVertices[ 5 ].color.dcolor = col[ 0 ];
		}
	}
}


void CRenderAuxGeomD3D::PrepareRendering()
{
	// update transformation matrices
	m_matrices.UpdateMatrices(m_renderer);

	// get current window resultion and update aspect ratios
	m_wndXRes = m_renderer.GetWidth();
	m_wndYRes = m_renderer.GetHeight();

	m_aspect = 1.0f;
	m_aspectInv = 1.0f;
	if( m_wndXRes > 0 && m_wndYRes > 0 )
	{
		m_aspect = (float) m_wndXRes / (float) m_wndYRes;
		m_aspectInv = 1.0f / m_aspect;
	}

	// reset DrawInFront mode
	m_curDrawInFrontMode = e_DrawInFrontOff;

	// reset stream buffer manager
	m_auxGeomSBM.Reset();

	// reset current VB/IB
	m_pCurVB = 0;
	m_pCurIB = 0;

	// reset current prim type
	m_curPrimType = CAuxGeomCB::e_PrimTypeInvalid;
}


bool CRenderAuxGeomD3D::BindStreams( EVertexFormat newVertexFormat, D3DVertexBuffer* pNewVB, D3DIndexBuffer* pNewIB )
{
	// set vertex declaration
	if (FAILED(m_renderer.FX_SetVertexDeclaration(0, newVertexFormat)))
		return false;

	// bind streams
	HRESULT hr = S_OK;
	if( m_pCurVB != pNewVB )
	{
		hr = m_renderer.FX_SetVStream(0, pNewVB, 0, CRenderMesh2::m_cSizeVF[newVertexFormat]);
		m_pCurVB = pNewVB;
	}
	if( m_pCurIB != pNewIB )
	{
		hr = m_renderer.FX_SetIStream(pNewIB);
		m_pCurIB = pNewIB;
	}

	return SUCCEEDED(hr);
}


void CRenderAuxGeomD3D::SetShader( const SAuxGeomRenderFlags& renderFlags )
{
	if( 0 == m_pAuxGeomShader )
	{
		m_pAuxGeomShader = m_renderer.m_cEF.mfForName( "AuxGeom", EF_SYSTEM );
		assert( 0 != m_pAuxGeomShader );
	}

	if( 0 != m_pAuxGeomShader )
	{
		bool dirty( 0 != ( m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_PersFlags & ( RBPF_FP_DIRTY | RBPF_FP_MATRIXDIRTY ) ) );
		if( false != dirty )
		{
			// NOTE: dirty flags are either set when marking matrices as dirty or setting EF_ColorOp in AdjustRenderStates
			m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_FP_DIRTY | RBPF_FP_MATRIXDIRTY;
			m_renderer.m_RP.m_ObjFlags &= ~FOB_TRANS_MASK;
			m_renderer.m_RP.m_pCurObject = m_renderer.m_RP.m_TempObjects[0][0];
			m_renderer.m_RP.m_pCurInstanceInfo = &m_renderer.m_RP.m_pCurObject->m_II;
			m_renderer.m_RP.m_FlagsShader_LT = m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_eCurColorOp[ 0 ] | ( m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_eCurAlphaOp[ 0 ] << 8 ) | 
				( m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_eCurColorArg[ 0 ] << 16 ) | ( m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_eCurAlphaArg[ 0 ] << 24 );
		}

		EAuxGeomPublicRenderflags_DrawInFrontMode newDrawInFrontMode( renderFlags.GetDrawInFrontMode() );
		CAuxGeomCB::EPrimType newPrimType( CAuxGeomCB::GetPrimType( renderFlags ) );

		if( false != dirty || m_pAuxGeomShader != m_renderer.m_RP.m_pShader || m_curDrawInFrontMode != newDrawInFrontMode || m_curPrimType != newPrimType )
		{
			if( CAuxGeomCB::e_Obj != newPrimType )
			{
				static CCryName techName("AuxGeometry");
				m_pAuxGeomShader->FXSetTechnique( techName );
				m_pAuxGeomShader->FXBegin( &m_renderer.m_RP.m_nNumRendPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES );
				m_pAuxGeomShader->FXBeginPass( 0 );

				static CCryName matViewProjName("matViewProj");
				if( e_DrawInFrontOn == renderFlags.GetDrawInFrontMode() && e_Mode3D == renderFlags.GetMode2D3DFlag() )
				{
					Matrix44A matScale(Matrix44(Matrix34::CreateScale(Vec3(0.999f, 0.999f, 0.999f))));

					Matrix44A matViewScaleProjT;
					matViewScaleProjT.Multiply(GetCurrentView(), matScale);
					matViewScaleProjT = matViewScaleProjT * GetCurrentProj();
					matViewScaleProjT.Transpose(matViewScaleProjT);

					m_pAuxGeomShader->FXSetVSFloat( matViewProjName, (Vec4*) &matViewScaleProjT, 4 );
					m_curDrawInFrontMode = e_DrawInFrontOn;
				}
				else
				{
					Matrix44A matViewProjT;
					matViewProjT.Transpose(*m_matrices.m_pCurTransMat);
					m_pAuxGeomShader->FXSetVSFloat( matViewProjName, (Vec4*) &matViewProjT, 4 );
					m_curDrawInFrontMode = e_DrawInFrontOff;
				}
			}
			else
			{
				static CCryName techName("AuxGeometryObj");
				m_pAuxGeomShader->FXSetTechnique( techName );
				m_pAuxGeomShader->FXBegin( &m_renderer.m_RP.m_nNumRendPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES );
				m_pAuxGeomShader->FXBeginPass( 0 );

				if( e_DrawInFrontOn == renderFlags.GetDrawInFrontMode() && e_Mode3D == renderFlags.GetMode2D3DFlag() )
				{
					m_curDrawInFrontMode = e_DrawInFrontOn;
				}
				else
				{
					m_curDrawInFrontMode = e_DrawInFrontOff;
				}
			}
			m_curPrimType = newPrimType;
		}
	}
	else
	{
		m_renderer.FX_SetFPMode();
	}
}


void CRenderAuxGeomD3D::AdjustRenderStates( const SAuxGeomRenderFlags& renderFlags )
{
	// init current render states mask
	uint32 curRenderStates( 0 ) ;

	// mode 2D/3D -- set new transformation matrix
	const Matrix44A* pNewTransMat( &GetCurrentTrans3D() );
	if( e_Mode2D == renderFlags.GetMode2D3DFlag() )
	{
		pNewTransMat = &GetCurrentTrans2D();
	}
	if( m_matrices.m_pCurTransMat != pNewTransMat )
	{
		m_matrices.m_pCurTransMat = pNewTransMat;

		m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matView->LoadIdentity();
		m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matProj->LoadMatrix( pNewTransMat );
		m_renderer.EF_DirtyMatrix();
	}

	// set alpha blending mode
	switch( renderFlags.GetAlphaBlendMode() )
	{
	case e_AlphaAdditive:
		{
			curRenderStates |= GS_BLSRC_ONE | GS_BLDST_ONE;
			break;
		}
	case e_AlphaBlended:
		{
			curRenderStates |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
			break;
		}
	case e_AlphaNone:
	default:
		{
			break;
		}
	}

	// set fill mode
	switch( renderFlags.GetFillMode() )
	{
	case e_FillModeWireframe:
		{
			curRenderStates |= GS_WIREFRAME;
			break;
		}
	case e_FillModeSolid:
	default:
		{
			break;
		}
	}

	// set cull mode
	switch( renderFlags.GetCullMode() )
	{
	case e_CullModeNone:
		{
			m_renderer.SetCullMode( R_CULL_NONE );
			break;
		}
	case e_CullModeFront:
		{
			m_renderer.SetCullMode( R_CULL_FRONT );
			break;
		}
	case e_CullModeBack:
	default:
		{
			m_renderer.SetCullMode( R_CULL_BACK );
			break;
		}
	}

	// set depth write mode
	switch( renderFlags.GetDepthWriteFlag() )
	{
	case e_DepthWriteOff:
		{
			break;
		}
	case e_DepthWriteOn:
	default:
		{
			curRenderStates |= GS_DEPTHWRITE;
			break;
		}
	}

	// set depth test mode
	switch( renderFlags.GetDepthTestFlag() )
	{
	case e_DepthTestOff:
		{
			curRenderStates |= GS_NODEPTHTEST;
			break;
		}
	case e_DepthTestOn:
	default:
		{
			break;
		}
	}

	// set point size
	uint8 newPointSize( m_curPointSize );
	if( CAuxGeomCB::e_PtList == CAuxGeomCB::GetPrimType( renderFlags ) )
	{
		newPointSize = CAuxGeomCB::GetPointSize( renderFlags );
	}
	else
	{
		newPointSize = 1;
	}

	if( newPointSize != m_curPointSize )
	{
		assert( newPointSize > 0 );
		float pointSize( (float) newPointSize );
#if defined (DIRECT3D9) || defined(OPENGL)
		m_renderer.m_pd3dDevice->SetRenderState( D3DRS_POINTSIZE, *( (uint32*) &pointSize ) );
#elif defined (DIRECT3D10)
		assert(0);
#endif
		m_curPointSize = newPointSize;
	}

	// apply states 
	m_renderer.EF_SetState( curRenderStates );

	// set color operations
	m_renderer.EF_SetColorOp( eCO_REPLACE, eCO_REPLACE, ( eCA_Diffuse | ( eCA_Diffuse << 3 ) ), ( eCA_Diffuse | ( eCA_Diffuse << 3 ) ) );
}


void CRenderAuxGeomD3D::RT_Flush(const SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end)
{
	if (!CV_r_auxGeom)
		return;

	PROFILE_LABEL_PUSH("AuxGeom");

	// should only be called from render thread
	assert(m_renderer.m_pRT->IsRenderThread());

	assert(data.m_pData);
	m_pCurCBRawData = data.m_pData;

	m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matProj->Push();
	m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matView->Push();

	if( false == m_renderer.IsDeviceLost() )
	{
		// prepare rendering
		PrepareRendering();

		// get push buffer to process all submitted auxiliary geometries
		m_pCurCBRawData->GetSortedPushBuffer(begin, end, m_auxSortedPushBuffer);

		// process push buffer
		for( CAuxGeomCB::AuxSortedPushBuffer::const_iterator it( m_auxSortedPushBuffer.begin() ), itEnd( m_auxSortedPushBuffer.end() ); it != itEnd; )
		{
			// mark current push buffer position
			CAuxGeomCB::AuxSortedPushBuffer::const_iterator itCur( it );

			// get current render flags
			const SAuxGeomRenderFlags& curRenderFlags( (*itCur)->m_renderFlags );
			m_curTransMatrixIdx = (*itCur)->m_transMatrixIdx;

			// get prim type
			CAuxGeomCB::EPrimType primType( CAuxGeomCB::GetPrimType( curRenderFlags ) );

			// find all entries sharing the same render flags
			while( true )
			{
				++it;
				if( ( it == itEnd ) || ( (*it)->m_renderFlags != curRenderFlags ) || ( (*it)->m_transMatrixIdx != m_curTransMatrixIdx ) )
				{
					break;
				}
			}

			// adjust render states based on current render flags
			AdjustRenderStates( curRenderFlags );

			// prepare thick lines
			if( CAuxGeomCB::e_TriList == primType && false != CAuxGeomCB::IsThickLine( curRenderFlags ) )
			{
				if( e_Mode3D == curRenderFlags.GetMode2D3DFlag() )
				{
					PrepareThickLines3D( itCur, it );
				}
				else
				{
					PrepareThickLines2D( itCur, it );
				}
			}

			// set appropriate shader
			SetShader( curRenderFlags );

			// draw push buffer entries
			switch( primType )
			{
			case CAuxGeomCB::e_PtList:
			case CAuxGeomCB::e_LineList:
			case CAuxGeomCB::e_TriList:
				{
					DrawAuxPrimitives( itCur, it, primType );
					break;
				}
			case CAuxGeomCB::e_LineListInd:
			case CAuxGeomCB::e_TriListInd:
				{
					DrawAuxIndexedPrimitives( itCur, it, primType );
					break;
				}
			case CAuxGeomCB::e_Obj:
			default:
				{
					DrawAuxObjects( itCur, it );
					break;
				}
			}
		}
	}

	m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matProj->Pop();
	m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matView->Pop();
	m_renderer.EF_DirtyMatrix();

	m_pCurCBRawData = 0;
	m_curTransMatrixIdx = 0;

	PROFILE_LABEL_POP("AuxGeom");
}


void CRenderAuxGeomD3D::Flush(const SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end)
{
	m_renderer.m_pRT->RC_AuxFlush(this, data, begin, end);
}


void CRenderAuxGeomD3D::SetOrthoMode(bool enable, Matrix44A* pMatrix)
{
	GetRenderAuxGeom()->SetOrthoMode(enable, pMatrix);
}


const Matrix44A& CRenderAuxGeomD3D::GetCurrentView() const
{
	return IsOrthoMode() ? gRenDev->m_IdentityMatrix : m_matrices.m_matView;
}


const Matrix44A& CRenderAuxGeomD3D::GetCurrentViewInv() const
{
	return IsOrthoMode() ? gRenDev->m_IdentityMatrix : m_matrices.m_matViewInv;
}


const Matrix44A& CRenderAuxGeomD3D::GetCurrentProj() const
{
	return IsOrthoMode() ? GetAuxOrthoMatrix(m_curTransMatrixIdx) : m_matrices.m_matProj;
}


const Matrix44A& CRenderAuxGeomD3D::GetCurrentTrans3D() const
{
	return IsOrthoMode() ? GetAuxOrthoMatrix(m_curTransMatrixIdx) : m_matrices.m_matTrans3D;
}


const Matrix44A& CRenderAuxGeomD3D::GetCurrentTrans2D() const
{
	return m_matrices.m_matTrans2D;
}


bool CRenderAuxGeomD3D::IsOrthoMode() const
{
	return m_curTransMatrixIdx != -1;
}


void CRenderAuxGeomD3D::SMatrices::UpdateMatrices(CD3D9Renderer& renderer)
{
	renderer.GetModelViewMatrix(&m_matView.m00);
	renderer.GetProjectionMatrix(&m_matProj.m00);

	m_matViewInv.Invert(m_matView);
	m_matTrans3D.Multiply(m_matView, m_matProj);

	m_pCurTransMat = 0;
}


CAuxGeomCB* CRenderAuxGeomD3D::GetRenderAuxGeom()
{
	if (m_renderer.m_pRT->IsMainThread())
		return m_auxGeomCBCol.Get(CAuxGeomCBCollector::eTCB_Main, this);
	else if (m_renderer.m_pRT->IsRenderThread())
		return m_auxGeomCBCol.Get(CAuxGeomCBCollector::eTCB_Renderer, this);
	else
	{
		assert(!"Rendering aux geometry from outside main or render thread is not supported!");
		return 0;
	}
}


inline const CAuxGeomCB::AuxVertexBuffer& CRenderAuxGeomD3D::GetAuxVertexBuffer() const
{
	assert(m_pCurCBRawData);
	return m_pCurCBRawData->m_auxVertexBuffer;
}


inline const CAuxGeomCB::AuxIndexBuffer& CRenderAuxGeomD3D::GetAuxIndexBuffer() const
{
	assert(m_pCurCBRawData);
	return m_pCurCBRawData->m_auxIndexBuffer;
}


inline const CAuxGeomCB::AuxDrawObjParamBuffer& CRenderAuxGeomD3D::GetAuxDrawObjParamBuffer() const
{
	assert(m_pCurCBRawData);
	return m_pCurCBRawData->m_auxDrawObjParamBuffer;
}


inline const Matrix44A& CRenderAuxGeomD3D::GetAuxOrthoMatrix(int idx) const
{
	assert(m_pCurCBRawData && idx >= 0 && idx < m_pCurCBRawData->m_auxOrthoMatrices.size());
	return m_pCurCBRawData->m_auxOrthoMatrices[idx];
}

#endif // #if defined(ENABLE_RENDER_AUX_GEOM)