#ifndef __CREPARTICLE_H__
#define __CREPARTICLE_H__

#include "CryThread.h"

typedef SVF_P3F_C4B_I4B_PS4F		SVertexParticle;

// forward declarations
class CREParticle;
struct CRenderListData;





























struct SRenderVertices
{
	FixedDynArray<SVertexParticle>	aVertices;
	FixedDynArray<uint16>						aIndices;
	int															nBaseVertexIndex;
	FixedDynArray<uint8>						aVertCounts;
	float														fMaxPixels;
	float														fPixels;

	SRenderVertices()
		{ memset(this, 0, sizeof(*this)); }

	inline void CopyVertices( Array<SVertexParticle>& aSrc )
	{
		int nVerts = min(aSrc.size(), aVertices.available());
		aVertices.push_back(aSrc(0, nVerts));
		aSrc.erase_front(nVerts);
	}

	inline void ExpandQuadVertices()
	{
		SVertexParticle* aV = aVertices.grow(3)-1;

		aV[3] = aV[2] = aV[1] = aV[0];

		aV[1].info.tex_x = 255;
		aV[2].info.tex_y = 255;
		aV[3].info.tex_x = 255;
		aV[3].info.tex_y = 255;
	}

	inline void SetQuadIndices(int nVertAdvance = 4)
	{
		uint16* pIndices = aIndices.grow(6);

		pIndices[0] = 0 + nBaseVertexIndex;
		pIndices[1] = 1 + nBaseVertexIndex;
		pIndices[2] = 2 + nBaseVertexIndex;

		pIndices[3] = 3 + nBaseVertexIndex;
		pIndices[4] = 2 + nBaseVertexIndex;
		pIndices[5] = 1 + nBaseVertexIndex;

		nBaseVertexIndex += nVertAdvance;
	}

	inline void SetQuadsIndices()
	{
		assert((aVertices.size() & 3) == 0);
		int nQuads = aVertices.size() >> 2;
		assert(aIndices.available() >= nQuads*6);
		while (nQuads-- > 0)
			SetQuadIndices();
	}

	inline void SetPolyIndices( int nVerts )
	{
		nVerts >>= 1;
		while (--nVerts > 0)
			SetQuadIndices(2);

		// Final quad.
		nBaseVertexIndex += 2;
	}

	void SetPoliesIndices( Array<SVertexParticle>& aSrcVerts, Array<uint8>& aSrcVertCounts )
	{
		int nAvailVerts = aVertices.available();
		int nVerts = 0;
		int nPolygon = 0;
		for (; nPolygon < aSrcVertCounts.size(); nPolygon++)
		{
			int nPolyVerts = aSrcVertCounts[nPolygon];
			if (nVerts + nPolyVerts > nAvailVerts)
				break;
			nVerts += nPolyVerts;
			SetPolyIndices(nPolyVerts);
		}
		aSrcVertCounts.erase_front(nPolygon);

		aVertices.push_back(aSrcVerts(0, nVerts));
		aSrcVerts.erase_front(nVerts);
	}
};

struct IAllocRender: SRenderVertices
{
	bool		bDirect;
	bool		bGeomShader;

	IAllocRender()
		: bDirect(false), bGeomShader(false)
	{}

	// Render existing SVertices, alloc new ones.
	virtual void Alloc( int nAllocVerts, int nAllocInds = 0, int nAllocVertCounts = 0 ) = 0; 
	virtual CREParticle* RenderElement() const { return 0; }
};

struct SParticleRenderContext
{
	Vec3		m_vCamPos;
	Vec3		m_vCamDir;
	float		m_fAngularRes;						// Pixels per radian
};

struct IParticleVertexCreator
{
	// Create the vertices for the particle emitter.
	virtual void ComputeVertices( const SParticleRenderContext& context, IAllocRender& alloc, bool bIsParticleThread = false ) = 0;
	virtual float GetDistSquared( const Vec3& vPos ) const = 0;
	virtual uint32 GetRenderOrder() const = 0;

	// Reference counting.
	virtual void AddRef() = 0;
	virtual void Release() = 0;
};

class CREParticle : public CRendElementBase
{
public:
	CREParticle( IParticleVertexCreator* pVC, const SParticleRenderContext& context );

	// Custom copy constructor required to avoid m_Lock copy.
	CREParticle( const CREParticle& in )
	: m_pVertexCreator(in.m_pVertexCreator)
	, m_ParticleComputed(in.m_ParticleComputed)
	, m_Context(in.m_Context)
	, m_fPixels(0.f)
	, m_nRenderOrder(0)
	{
	}

	virtual void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		//pSizer->AddObject(this, sizeof(*this)); // allocated in own allocator
	}
	// CRendElement implementation.
	static CREParticle* Create( IParticleVertexCreator* pVC, const SParticleRenderContext& context );
	static void ClearSPUQueue();

	virtual CRendElementBase* mfCopyConstruct()
	{
		return new CREParticle(*this);
	}
  virtual ~CREParticle();
  virtual int Size()
	{
		return sizeof(*this);
	}

	virtual void mfPrepare();
	virtual float mfDistanceToCameraSquared( Matrix34& matInst );

	virtual bool mfPreDraw( SShaderPass *sl );
	virtual bool mfDraw( CShader *ef, SShaderPass *sl );

	// Additional methods.
	void StoreVertices( bool bWait, bool bParticleThread = false );
	void TransferVertices() const;

	void SetVertices( Array<SVertexParticle> aVerts, Array<uint8> aVertCounts, float fPixels )
	{
		m_aVerts = aVerts;
		m_aVertCounts = aVertCounts;
		assert(m_aVerts.empty() || !m_aVertCounts.empty());
		m_fPixels = fPixels;
	}

	bool operator< (const CREParticle& r) const
	{
		return m_nRenderOrder < r.m_nRenderOrder;
	}







	
private:
	CryReadModifyLock										m_Lock;							// Serialises access to vertex creator and verts.
	_smart_ptr<IParticleVertexCreator>	m_pVertexCreator;		// Particle object which computes vertices.
	volatile bool												m_ParticleComputed;
	SParticleRenderContext							m_Context;					// Camera position and resolution.
	Array<SVertexParticle>							m_aVerts;						// Computed particle vertices.
	Array<uint8>												m_aVertCounts;			// Verts in each particle (multi-seg particles only).
	float																m_fPixels;					// Total pixels rendered.
	uint32															m_nRenderOrder;			// Copied from VertexCreator upon assignment.





	bool Lock(const bool bWait)
	{



		return m_Lock.LockModify(!bWait);

	}

	void Unlock()
	{
#if !defined(__SPU__)
		m_Lock.UnlockModify();
#endif // !SPU
	}
};

#endif  // __CREPARTICLE_H__
