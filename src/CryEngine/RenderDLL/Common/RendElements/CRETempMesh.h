
#ifndef __CRETEMPMESH_H__
#define __CRETEMPMESH_H__

//=============================================================


class CRETempMesh : public CRendElementBase
{
public:
  int m_nVertices;
  CVertexBuffer *m_pVBuffer;
  int m_nIndices;
  CIndexBuffer *m_pIBuffer;

public:
  CRETempMesh()
  {
    m_nVertices = 0;
    m_nIndices = 0;
    m_pVBuffer = NULL;
    m_pIBuffer = NULL;
    mfSetType(eDATA_TempMesh);
    mfUpdateFlags(FCEF_TRANSFORM);
  }

  virtual ~CRETempMesh()
  {
  }

  virtual void mfPrepare();
  virtual bool mfDraw(CShader *ef, SShaderPass *sfm);
  virtual void *mfGetPointer(ESrcPointer ePT, int *Stride, EParamType Type, ESrcPointer Dst, int Flags);
  virtual bool mfPreDraw(SShaderPass *sl);
  virtual void mfReset();
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

#endif  // __CRETEMPMESH_H__
