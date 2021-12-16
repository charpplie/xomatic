/*
=====================================================================
FILE : CREDeferredShading.cpp
DESC : Deferred shading processing render element
PROJ : Crytek Engine
CODER: Tiago Sousa

=====================================================================
*/


#include "StdAfx.h"


// constructor/destructor
CREDeferredShading::CREDeferredShading()
{
  // setup screen process renderer type
  mfSetType(eDATA_DeferredShading);
  mfUpdateFlags(FCEF_TRANSFORM);
}

CREDeferredShading::~CREDeferredShading()
{  
};

// prepare screen processing
void CREDeferredShading:: mfPrepare()
{
  gRenDev->FX_CheckOverflow(0, 0, this);

  gRenDev->m_RP.m_pRE = this;
  gRenDev->m_RP.m_FlagsPerFlush |= RBSI_DRAWAS2D;
  gRenDev->m_RP.m_RendNumIndices = 0;
  gRenDev->m_RP.m_RendNumVerts = 0;
}

void CREDeferredShading::mfReset()
{
}

void CREDeferredShading::mfActivate(int iProcess)
{
}