/*=============================================================================
  NULL_TexturesStreaming.cpp : NULL device specific texture streaming technology.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honitch Andrey

=============================================================================*/

#include "StdAfx.h"
#include "NULL_Renderer.h"

//===============================================================================

STexPoolItem::~STexPoolItem()
{

}

int CTexture::StreamSetLod(int nToMip, bool bUnload)
{
	return 0;
}

// Just remove item from the texture object and keep Item in Pool list for future use
// This function doesn't release API texture
void CTexture::StreamRemoveFromPool()
{
}

void CTexture::StreamCopyMips(int nStartMip, int nEndMip, bool bToDevice)
{
}

STexPoolItem *CTexture::StreamGetPoolItem(int nStartMip, int nMips)
{
  return NULL;
}
void CTexture::StreamAssignPoolItem(STexPoolItem *pItem, int nMinMip)
{
}

STexPool *CTexture::StreamCreatePool(int nWidth, int nHeight, int nMips, D3DFormat eTF, ETEX_Type eTT)
{
  return NULL;
}

void CTexture::StreamUnloadOldTextures(int nCurTexPoolSize)
{
}

void CTexture::StreamCheckTexLimits()
{
}
