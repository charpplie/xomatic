/*=============================================================================
  NULL_DevBuffer.cpp : NULL renderer device buffer management.
  Copyright (c) 2001-2009 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "NULL_Renderer.h"

//===============================================================================

SDevBuffer::~SDevBuffer()
{

}

int32 CDevBufferMan::CreateVB(int32 nSize)
{
  int32 nVB = -1;
  return nVB;
}

int32 CDevBufferMan::CreateIB(int32 nSize)
{
  int32 nIB = -1;
  return nIB;
}

bool CDevBufferMan::ReleaseVB(int32 nVB)
{
  return true;
}

bool CDevBufferMan::ReleaseIB(int32 nIB)
{
  return true;
}

void* CDevBufferMan::UpdateVB(int32 nVB, const void *pData, int32 nSize, int32 nOffset)
{
  return NULL;
}
void* CDevBufferMan::UpdateIB(int32 nIB, const void *pData, int32 nSize, int32 nOffset)
{
  return NULL;
}

void *CDevBufferMan::LockVB(int32 nVB, int32 nOffset, int32 nSize, uint32 nFlags)
{
  return NULL;
}

void CDevBufferMan::UnlockVB(int32 nVB)
{
}

void *CDevBufferMan::LockIB(int32 nIB, int32 nOffset, int32 nSize, uint32 nFlags)
{
  return NULL;
}

void CDevBufferMan::UnlockIB(int32 nIB)
{
}

//===============================================================================================================

void CDevBufferMan::ReleaseVBuffer(CVertexBuffer* pVB)
{
  SAFE_DELETE(pVB);
}

void CDevBufferMan::ReleaseIBuffer(CIndexBuffer* pIB)
{
  SAFE_DELETE(pIB);
}

CVertexBuffer *CDevBufferMan::CreateVBuffer(int32 nVerts, EVertexFormat eVF, const char *szName)
{
  CVertexBuffer *pVB = new CVertexBuffer(NULL, eVF);
  pVB->m_nVerts = nVerts;
  pVB->m_VS.m_nDevID = -1;

  return pVB;
}

CIndexBuffer *CDevBufferMan::CreateIBuffer(int32 nInds, const char *szName)
{
  CIndexBuffer *pIB = new CIndexBuffer(NULL);
  pIB->m_nInds = nInds;
  pIB->m_VS.m_nDevID = -1;

  return pIB;
}

void* CDevBufferMan::UpdateVBuffer(CVertexBuffer *pVB, void *pVerts, int32 nVerts)
{
  return NULL;
}

void* CDevBufferMan::UpdateIBuffer(CIndexBuffer *pIB, void *pInds, int32 nInds)
{
  return NULL;
}

#ifdef _RENDERER

CVertexBuffer::~CVertexBuffer()
{
}

CIndexBuffer::~CIndexBuffer()
{
}

#endif
