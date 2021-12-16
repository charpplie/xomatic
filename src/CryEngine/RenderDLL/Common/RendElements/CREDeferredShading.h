/*
=====================================================================
FILE : CREDeferredShading.h
DESC : Deferred shading processing render element
PROJ : Crytek Engine
CODER: Tiago Sousa

=====================================================================
*/

#ifndef __CREDEFERREDSHADING_H__
#define __CREDEFERREDSHADING_H__


class CREDeferredShading : public CRendElementBase
{
  friend class CD3D9Renderer;
  friend class CGLRenderer;

public:

  // constructor/destructor
  CREDeferredShading();

  virtual ~CREDeferredShading();

  // prepare screen processing
  virtual void mfPrepare();
  // render screen processing
  virtual bool mfDraw(CShader *ef, SShaderPass *sfm);

  // begin screen processing
  virtual void mfActivate(int iProcess);
  // reset 
  virtual void mfReset(void);

	virtual void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif
