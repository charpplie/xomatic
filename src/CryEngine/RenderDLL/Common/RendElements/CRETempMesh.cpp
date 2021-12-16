#include "StdAfx.h"
#include "RendElement.h"

void CRETempMesh::mfPrepare()
{
  gRenDev->FX_CheckOverflow(0, 0, this);

  gRenDev->m_RP.m_pRE = this;
  gRenDev->m_RP.m_RendNumIndices = 6;
  gRenDev->m_RP.m_RendNumVerts = 4;
  gRenDev->m_RP.m_FirstVertex = 0;
  gRenDev->m_RP.m_FirstIndex = 0;
}

void *CRETempMesh::mfGetPointer(ESrcPointer ePT, int *Stride, EParamType Type, ESrcPointer Dst, int Flags)
{
  *Stride = sizeof(SVF_P3F_C4B_T2F);
  SVF_P3F_C4B_T2F *pVertices = (SVF_P3F_C4B_T2F *)m_pVBuffer->m_VS.m_pLocalData;
  SBufInfoTable *pOffs = &CRenderMesh2::m_cBufInfoTable[m_pVBuffer->m_eVF];

  switch(ePT) 
  {
    case eSrcPointer_Vert:
      return &pVertices->xyz.x;
    case eSrcPointer_Tex:
      return &pVertices->st;
    case eSrcPointer_Color:
      return &pVertices->color.dcolor;
  }
  return NULL;
}

