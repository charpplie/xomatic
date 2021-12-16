/*=============================================================================
	CREClientPoly.cpp : implementation of 3D Client polygons RE.
	Copyright (c) 2001-2005 Crytek Studios. All Rights Reserved.

	Revision history:
		* Created by Honitch Andrey

=============================================================================*/

#include "StdAfx.h"


//===============================================================


TArrayHolder<CREClientPoly *> CREClientPoly::m_PolysStorage;

CRendElementBase *CREClientPoly::mfCopyConstruct(void)
{
  CREClientPoly *cp = new CREClientPoly;
  *cp = *this;
  return cp;
}

void CREClientPoly::mfPrepare(void)
{
  CRenderer *rd = gRenDev;
  CShader *ef = rd->m_RP.m_pShader;
  int i, n;

  if (rd->m_RP.m_CurVFormat == eVF_P3S_C4B_T2S)
    rd->m_RP.m_CurVFormat = eVF_P3F_C4B_T2F;

  rd->FX_StartMerging();
  CREClientPoly::mRS.NumRendPolys++;

  int savev = rd->m_RP.m_RendNumVerts;
  int savei = rd->m_RP.m_RendNumIndices;

  int nThreadID = rd->m_RP.m_nProcessThreadID;

  int nVerts, nInds;
  rd->FX_CheckOverflow(m_sNumVerts, m_sNumIndices, this, &nVerts, &nInds);
  
  uint16 *pSrcInds = &rd->m_RP.m_SysIndexPool[nThreadID][m_nOffsInd];
  n = rd->m_RP.m_RendNumVerts;
  uint16 *dinds = &rd->m_RP.m_RendIndices[gRenDev->m_RP.m_RendNumIndices];
  for (i=0; i<nInds; i++, dinds++, pSrcInds++)
  {
    *dinds = *pSrcInds+n;
  }
  rd->m_RP.m_RendNumIndices += i;

  UPipeVertex ptr = rd->m_RP.m_NextPtr;
  byte *OffsT, *OffsD;
  SVF_P3F_C4B_T2F *pSrc = (SVF_P3F_C4B_T2F *)&rd->m_RP.m_SysVertexPool[nThreadID][m_nOffsVert];
  switch(rd->m_RP.m_CurVFormat)
  {
    case eVF_P3F_C4B_T2F:
#ifndef FP16_MESH
    case 3:
#endif
      OffsT = rd->m_RP.m_OffsT + ptr.PtrB;
      OffsD = rd->m_RP.m_OffsD + ptr.PtrB;
      for (i=0; i<nVerts; i++, ptr.PtrB+=rd->m_RP.m_Stride, OffsT+=rd->m_RP.m_Stride, OffsD+=rd->m_RP.m_Stride)
      {
        *(float *)(ptr.PtrB+0) = pSrc[i].xyz[0];
        *(float *)(ptr.PtrB+4) = pSrc[i].xyz[1];
        *(float *)(ptr.PtrB+8) = pSrc[i].xyz[2];
        *(float *)(OffsT) = pSrc[i].st[0];
        *(float *)(OffsT+4) = pSrc[i].st[1];
        *(uint32 *)OffsD = pSrc[i].color.dcolor;
      }
    	break;
    default:
      assert(false);
      break;
  }
  rd->m_RP.m_NextPtr = ptr;

  if (m_nOffsTang >= 0)
  {
    UPipeVertex ptrTang = rd->m_RP.m_NextPtrTang;
    SPipTangents *pTangents = (SPipTangents *)&rd->m_RP.m_SysVertexPool[nThreadID][m_nOffsTang];
    for (i=0; i<nVerts; i++, ptrTang.PtrB+=sizeof(SPipTangents))
    {
      *(SPipTangents *)(ptrTang.PtrB) = pTangents[i];
    }
    rd->m_RP.m_NextPtrTang = ptrTang;
  }

  rd->m_RP.m_RendNumVerts += nVerts;

  CREClientPoly::mRS.NumVerts += rd->m_RP.m_RendNumVerts - savev; 
  CREClientPoly::mRS.NumIndices += rd->m_RP.m_RendNumIndices - savei; 
}


//=======================================================================

SClientPolyStat CREClientPoly::mRS;

void CREClientPoly::mfPrintStat()
{
/*  char str[1024];

  *gpCurPrX = 4;
  sprintf(str, "Num Indices: %i\n", mRS.NumIndices);
  gRenDev->mfPrintString (str, PS_TRANSPARENT | PS_UP);

  *gpCurPrX = 4;
  sprintf(str, "Num Verts: %i\n", mRS.NumVerts);
  gRenDev->mfPrintString (str, PS_TRANSPARENT | PS_UP);

  *gpCurPrX = 4;
  sprintf(str, "Num Render Client Polys: %i\n", mRS.NumRendPolys);
  gRenDev->mfPrintString (str, PS_TRANSPARENT | PS_UP);

  *gpCurPrX = 4;
  sprintf(str, "Num Occluded Client Polys: %i\n", mRS.NumOccPolys);
  gRenDev->mfPrintString (str, PS_TRANSPARENT | PS_UP);

  *gpCurPrX = 4;
  gRenDev->mfPrintString ("\nClient Polygons status info:\n", PS_TRANSPARENT | PS_UP);*/
}
