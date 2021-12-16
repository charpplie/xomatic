
#ifndef __CRECLIENTPOLY_H__
#define __CRECLIENTPOLY_H__

//=============================================================

struct SClientPolyStat
{
  int NumOccPolys;
  int NumRendPolys;
  int NumVerts;
  int NumIndices;
};

class CREClientPoly : public CRendElementBase
{
public:
	enum eFlags
	{
		efAfterWater = 1<<0,
		efShadowGen = 1<<1,
	};
  SShaderItem m_Shader;
  CRenderObject *m_pObject;
  short m_sNumVerts;
  short m_sNumIndices;
  byte m_nCPFlags;
  int m_nOffsVert;
  int m_nOffsTang;
  int m_nOffsInd;

  static SClientPolyStat mRS;
  static void mfPrintStat();

public:
  CREClientPoly()
  {
    mfSetType(eDATA_ClientPoly);
    m_sNumVerts = 0;
		m_nCPFlags = 0;
    mfUpdateFlags(FCEF_TRANSFORM | FCEF_NEEDFILLBUF);
  }

  virtual ~CREClientPoly() {};

  virtual void mfPrepare();
  virtual CRendElementBase *mfCopyConstruct(void);

	virtual void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_PolysStorage);
	}

	static TArrayHolder<CREClientPoly *> m_PolysStorage;
};

#endif  // __CRECLIENTPOLY2D_H__
