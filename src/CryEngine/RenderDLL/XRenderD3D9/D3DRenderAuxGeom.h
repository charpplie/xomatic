#ifndef D3D_RENDER_AUX_GEOM_H
#define D3D_RENDER_AUX_GEOM_H


#include "../Common/RenderAuxGeom.h"

#if defined(ENABLE_RENDER_AUX_GEOM)

class CD3D9Renderer;
class ICrySizer;


class CRenderAuxGeomD3D : public IRenderAuxGeomImpl
{
public:
	VIRTUAL void Flush(const SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end);
	VIRTUAL void RT_Flush(const SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end);

public:
	static CRenderAuxGeomD3D* Create( CD3D9Renderer& renderer )
	{
		return( new CRenderAuxGeomD3D( renderer ) );
	}

public:
	~CRenderAuxGeomD3D();

	CAuxGeomCB* GetRenderAuxGeom();
  int GetDeviceDataSize();
	void ReleaseDeviceObjects();
	HRESULT RestoreDeviceObjects();
	void SetOrthoMode(bool enable, Matrix44A* pMatrix = 0);
	void GetMemoryUsage(ICrySizer* pSizer) const;

	void* operator new(size_t s)
	{
		uint8* p = (uint8*) malloc(s + 16 + 8);
		memset(p, 0, s + 16 + 8);
		uint8* pRet = (uint8*) ((size_t) (p + 16 + 8) & ~0xF);
		((uint8**) pRet)[-1] = p;
		return pRet;
	}

	void operator delete(void* p)
	{
		free(((uint8**)p)[-1]);
	}

	CAuxGeomCB* GetMainThreadAuxGeomCB()
	{
		return m_auxGeomCBCol.Get(CAuxGeomCBCollector::eTCB_Main, this);
	}

	CAuxGeomCB* GetRenderThreadAuxGeomCB()
	{
		return m_auxGeomCBCol.Get(CAuxGeomCBCollector::eTCB_Renderer, this);
	}

private:
	struct SStreamBufferManager
	{
	public:
		SStreamBufferManager();
		void Reset();
		void DiscardVB();
		void DiscardIB();

	public:
		bool m_discardVB;
		uint32 m_curVBIndex;
		bool m_discardIB;
		uint32 m_curIBIndex;
	};

	struct SDrawObjMesh
	{
		SDrawObjMesh()
		: m_numVertices( 0 )
		, m_numFaces( 0 )
		, m_pVB( 0 )
		, m_pIB( 0 )
		{
		}

		~SDrawObjMesh()
		{
			Release();
		}

		void Release()
		{
			SAFE_RELEASE( m_pVB );
			SAFE_RELEASE( m_pIB );

			m_numVertices = 0;
			m_numFaces = 0;
		}

    int GetDeviceDataSize()
    {
      int nSize = 0;
      nSize += _VertBufferSize(m_pVB);
      nSize += _IndexBufferSize(m_pIB);

      return nSize;
    }

		uint32 m_numVertices;
		uint32 m_numFaces;
		D3DVertexBuffer* m_pVB;
		D3DIndexBuffer* m_pIB;
	};

	enum EAuxObjNumLOD
	{
		e_auxObjNumLOD = 5
	};

	struct SMatrices
	{
		SMatrices()
		: m_pCurTransMat(0)
		{
			m_matView.SetIdentity();
			m_matViewInv.SetIdentity();
			m_matProj.SetIdentity();
			m_matTrans3D.SetIdentity();

			m_matTrans2D = Matrix44A( 2,  0, 0, 0,
																0, -2, 0, 0,
																0,  0, 0, 0,
															 -1,  1, 0, 1);
		}

		void UpdateMatrices(CD3D9Renderer& renderer);

		Matrix44A m_matView;
		Matrix44A m_matViewInv;
		Matrix44A m_matProj;
		Matrix44A m_matTrans3D;
		Matrix44A m_matTrans2D;
		const Matrix44A* m_pCurTransMat;
	};

	class CAuxGeomCBCollector
	{
	public:
		CAuxGeomCBCollector()
		{
			for (size_t i(0); i<eTCB_NumThreadCBs; ++i)
				m_pCBs[i] = 0;
		}
		~CAuxGeomCBCollector()
		{
			for (size_t i(0); i<eTCB_NumThreadCBs; ++i)
				SAFE_DELETE(m_pCBs[i]);
		}

		enum EThreadCB
		{
			eTCB_Main,
			eTCB_Renderer,

			eTCB_NumThreadCBs
		};

		CAuxGeomCB* Get(EThreadCB id, IRenderAuxGeomImpl* pRenderAuxGeomImpl)
		{
#ifdef STRIP_RENDER_THREAD
			id = eTCB_Renderer; // force id to always point to render thread data
#endif
			assert(id < eTCB_NumThreadCBs);
			if (!m_pCBs[id])
				m_pCBs[id] = new CAuxGeomCB(pRenderAuxGeomImpl);
			return m_pCBs[id];
		}

		void GetMemoryUsage(ICrySizer* pSizer) const
		{
			for (size_t i(0); i<eTCB_NumThreadCBs; ++i)
			{
				if (m_pCBs[i])
					m_pCBs[i]->GetMemoryUsage(pSizer);
			}
		}

	private:
		CAuxGeomCB* m_pCBs[eTCB_NumThreadCBs];
	};

private:
	CRenderAuxGeomD3D( CD3D9Renderer& renderer );
#if defined (DIRECT3D9) || defined(OPENGL)
	void DetermineAuxPrimitveFlags( uint32& d3dNumPrimDivider, D3DPRIMITIVETYPE& d3dPrim, CAuxGeomCB::EPrimType primType ) const;
#elif defined (DIRECT3D10)
	void DetermineAuxPrimitveFlags( uint32& d3dNumPrimDivider, D3D11_PRIMITIVE_TOPOLOGY& d3dPrim, CAuxGeomCB::EPrimType primType ) const;
#endif
	void DrawAuxPrimitives( CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd, const CAuxGeomCB::EPrimType& primType );
	void DrawAuxIndexedPrimitives( CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd, const CAuxGeomCB::EPrimType& primType );
	void DrawAuxObjects( CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd );

	void PrepareThickLines2D( CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd );
	void PrepareThickLines3D( CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd );

	void PrepareRendering();
	void SetShader( const SAuxGeomRenderFlags& renderFlags );
	void AdjustRenderStates( const SAuxGeomRenderFlags& renderFlags );
#if defined (DIRECT3D9) || defined(OPENGL)
	bool BindStreams( EVertexFormat newVertexFormat, IDirect3DVertexBuffer9* pNewVB, IDirect3DIndexBuffer9* pNewIB );
#elif defined (DIRECT3D10)
	bool BindStreams( EVertexFormat newVertexFormat, ID3D11Buffer* pNewVB, ID3D11Buffer* pNewIB );
#endif

	template< typename TMeshFunc >
		HRESULT CreateMesh( SDrawObjMesh& mesh, TMeshFunc meshFunc );

	const Matrix44A& GetCurrentView() const;
	const Matrix44A& GetCurrentViewInv() const;
	const Matrix44A& GetCurrentProj() const;
	const Matrix44A& GetCurrentTrans3D() const;
	const Matrix44A& GetCurrentTrans2D() const;

	bool IsOrthoMode() const;

	const CAuxGeomCB::AuxVertexBuffer& GetAuxVertexBuffer() const;
	const CAuxGeomCB::AuxIndexBuffer& GetAuxIndexBuffer() const;
	const CAuxGeomCB::AuxDrawObjParamBuffer& GetAuxDrawObjParamBuffer() const;
	const Matrix44A& GetAuxOrthoMatrix(int idx) const;

private:
	CD3D9Renderer& m_renderer;

#if defined (DIRECT3D9) || defined(OPENGL)
	IDirect3DVertexBuffer9* m_pAuxGeomVB;
	IDirect3DIndexBuffer9* m_pAuxGeomIB;

	IDirect3DVertexBuffer9* m_pCurVB;
	IDirect3DIndexBuffer9* m_pCurIB;
#elif defined (DIRECT3D10)
	ID3D11Buffer* m_pAuxGeomVB;
	ID3D11Buffer* m_pAuxGeomIB;

	ID3D11Buffer* m_pCurVB;
	ID3D11Buffer* m_pCurIB;
#endif

	SStreamBufferManager m_auxGeomSBM;

	uint32 m_wndXRes;
	uint32 m_wndYRes;
	float m_aspect;
	float m_aspectInv;

	SMatrices m_matrices;

	CAuxGeomCB::EPrimType m_curPrimType;

	uint8 m_curPointSize;

	int m_curTransMatrixIdx;

	CShader* m_pAuxGeomShader;
	EAuxGeomPublicRenderflags_DrawInFrontMode m_curDrawInFrontMode;

	CAuxGeomCB::AuxSortedPushBuffer m_auxSortedPushBuffer;
	const CAuxGeomCB::SAuxGeomCBRawData* m_pCurCBRawData;
	CAuxGeomCBCollector m_auxGeomCBCol;

	int CV_r_auxGeom;

	SDrawObjMesh m_sphereObj[ e_auxObjNumLOD ];
	SDrawObjMesh m_coneObj[ e_auxObjNumLOD ];
	SDrawObjMesh m_cylinderObj[ e_auxObjNumLOD ];
};


inline
CRenderAuxGeomD3D::SStreamBufferManager::SStreamBufferManager()
: m_discardVB( true )
, m_curVBIndex( 0 )
, m_discardIB( true )
, m_curIBIndex( 0 )
{
}


inline void
CRenderAuxGeomD3D::SStreamBufferManager::Reset()
{
	m_discardVB = true;
	m_curVBIndex = 0;
	m_discardIB = true;
	m_curIBIndex = 0;
}


inline void 
CRenderAuxGeomD3D::SStreamBufferManager::DiscardVB()
{
	m_discardVB = true;
	m_curVBIndex = 0;
}


inline void 
CRenderAuxGeomD3D::SStreamBufferManager::DiscardIB()
{
	m_discardIB = true;
	m_curIBIndex = 0;
}


#if defined (DIRECT3D9) || defined(OPENGL)
inline void CRenderAuxGeomD3D::DetermineAuxPrimitveFlags( uint32& d3dNumPrimDivider, D3DPRIMITIVETYPE& d3dPrim, CAuxGeomCB::EPrimType primType ) const
#elif defined (DIRECT3D10)
inline void CRenderAuxGeomD3D::DetermineAuxPrimitveFlags( uint32& d3dNumPrimDivider, D3D11_PRIMITIVE_TOPOLOGY& d3dPrim, CAuxGeomCB::EPrimType primType ) const
#endif
{
#if defined (DIRECT3D9) || defined(OPENGL)
	switch( primType )
	{
	case CAuxGeomCB::e_PtList:
		{
			d3dNumPrimDivider = 1;
			d3dPrim = D3DPT_POINTLIST;
			break;
		}
	case CAuxGeomCB::e_LineList:
		{
			d3dNumPrimDivider = 2;
			d3dPrim = D3DPT_LINELIST;
			break;
		}
	case CAuxGeomCB::e_LineListInd:
		{
			d3dNumPrimDivider = 2;
			d3dPrim = D3DPT_LINELIST;
			break;
		}
	case CAuxGeomCB::e_TriList:
		{
			d3dNumPrimDivider = 3;
			d3dPrim = D3DPT_TRIANGLELIST;
			break;
		}
	case CAuxGeomCB::e_TriListInd:
	default:
		{
			d3dNumPrimDivider = 3;
			d3dPrim = D3DPT_TRIANGLELIST;
			break;
		}
	}
#elif defined (DIRECT3D10)
	switch( primType )
	{
	case CAuxGeomCB::e_PtList:
		{
			d3dNumPrimDivider = 1;
			d3dPrim = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		}
	case CAuxGeomCB::e_LineList:
		{
			d3dNumPrimDivider = 2;
			d3dPrim = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		}
	case CAuxGeomCB::e_LineListInd:
		{
			d3dNumPrimDivider = 2;
			d3dPrim = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		}
	case CAuxGeomCB::e_TriList:
		{
			d3dNumPrimDivider = 3;
			d3dPrim = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		}
	case CAuxGeomCB::e_TriListInd:
	default:
		{
			d3dNumPrimDivider = 3;
			d3dPrim = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		}
	}
#endif
}

#endif // #if defined(ENABLE_RENDER_AUX_GEOM)

#endif D3D_RENDER_AUX_GEOM_H
