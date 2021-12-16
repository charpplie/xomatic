#ifndef __CREOCLEAF_H__
#define __CREOCLEAF_H__

//=============================================================

#define FCEF_CALCCENTER 0x10000
#define FCEF_DYNAMIC    0x20000

class CREMesh : public CRendElementBase
{
  friend class CRenderer;
public:
  // Copy of Chunk to avoid indirections
  int m_nFirstIndexId;
  int m_nNumIndices;
  uint16 m_nFirstVertId;
  uint16 m_nNumVerts;
  uint16 m_nPrimetiveType;
  struct CRenderChunk *m_pChunk;
  class  CRenderMesh2 *m_pRenderMesh;

  CREMesh()
  {
    mfSetType(eDATA_Mesh);
    mfUpdateFlags(FCEF_TRANSFORM);
    m_pChunk = NULL;
    m_pRenderMesh = NULL;
    m_nPrimetiveType = ~0;
    m_nFirstIndexId = -1;
    m_nNumIndices = -1;
    m_nFirstVertId = 0;
    m_nNumVerts = 0;
  }

  virtual ~CREMesh()
  {
  }
  virtual struct CRenderChunk *mfGetMatInfo();
  virtual PodArray<struct CRenderChunk> *mfGetMatInfoList();
  virtual int mfGetMatId();
  virtual bool mfPreDraw(SShaderPass *sl);
	virtual bool mfIsHWSkinned()
	{
		return m_pChunk?!m_pChunk->m_arrChunkBoneIDs.empty() : false;
	}
  virtual void mfGetPlane(Plane& pl);
  virtual void mfPrepare();
  virtual void mfReset();
  virtual bool mfCullByClipPlane(CRenderObject *pObj);
  virtual void mfCenter(Vec3& Pos, CRenderObject*pObj);
  virtual bool mfDraw(CShader *ef, SShaderPass *sfm);
  virtual void *mfGetPointer(ESrcPointer ePT, int *Stride, EParamType Type, ESrcPointer Dst, int Flags);
  virtual float mfDistanceToCameraSquared(Matrix34& matInst);
  virtual bool mfCheckUpdate(EVertexFormat eVertFormat, int Flags);
  virtual void mfGetBBox(Vec3& vMins, Vec3& vMaxs);
  virtual void mfPrecache(const SShaderItem& SH);
  virtual int Size()
  {
    int nSize = sizeof(*this);
    return nSize;
  }
	virtual void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif  // __CREOCLEAF_H__
