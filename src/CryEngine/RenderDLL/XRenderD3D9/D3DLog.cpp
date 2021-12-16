#include "StdAfx.h"
#include "DriverD3D.h"

#ifdef DO_RENDERLOG

#define ecase(e) case e: return #e

static char sLogStr[1024];
#define edefault(e) default: sprintf(sLogStr, "0x%x", e); return sLogStr;

#define eflag(e)  \
  if (Flag & e)   \
{               \
    if (sLogStr[0])  \
      strcat (sLogStr, " | "); \
    strcat(sLogStr, #e);       \
}               \

static char *sHex (DWORD Value)
{
  sprintf(sLogStr, "0x%x", (uint32)Value);
  return sLogStr;
}
static char *sInt (DWORD Value)
{
  sprintf(sLogStr, "%d", (uint32)Value);
  return sLogStr;
}
static char *sDWFloat (DWORD Value)
{
  float fVal = *((float*)(&Value));
  sprintf(sLogStr, "%.3f", fVal);
  return sLogStr;
}

static char *sBool (DWORD Value)
{
  switch (Value)
  {
    ecase (FALSE);
    ecase (TRUE);
    edefault ((uint32)Value);
  }
}















#if defined (DIRECT3D9) || defined (OPENGL)

static char *sD3DZB (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DZB_FALSE);
    ecase (D3DZB_TRUE);
    ecase (D3DZB_USEW);
    edefault (Value);
  }
}

static char *sD3DFILL (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DFILL_POINT);
    ecase (D3DFILL_WIREFRAME);
    ecase (D3DFILL_SOLID);
    edefault (Value);
  }
}

static char *sD3DSHADE (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DSHADE_FLAT);
    ecase (D3DSHADE_GOURAUD);
    ecase (D3DSHADE_PHONG);
    edefault (Value);
  }
}

static char *sD3DBLEND (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DBLEND_ZERO);
    ecase (D3DBLEND_ONE);
    ecase (D3DBLEND_SRCCOLOR);
    ecase (D3DBLEND_INVSRCCOLOR);
    ecase (D3DBLEND_SRCALPHA);
    ecase (D3DBLEND_INVSRCALPHA);
    ecase (D3DBLEND_DESTALPHA);
    ecase (D3DBLEND_INVDESTALPHA);
    ecase (D3DBLEND_DESTCOLOR);
    ecase (D3DBLEND_INVDESTCOLOR);
    ecase (D3DBLEND_SRCALPHASAT);  
#ifndef _XBOX
		ecase (D3DBLEND_BOTHSRCALPHA);
    ecase (D3DBLEND_BOTHINVSRCALPHA);



#endif
	   ecase (D3DBLEND_BLENDFACTOR);
    ecase (D3DBLEND_INVBLENDFACTOR);
    edefault (Value);
  }
}

static char *sD3DCULL (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DCULL_NONE);
    ecase (D3DCULL_CW);
    ecase (D3DCULL_CCW);
    edefault (Value);
  }
}

static char *sD3DCMP (DWORD Value)
{
  switch(Value)
  {
    ecase (D3DCMP_NEVER);
    ecase (D3DCMP_LESS);
    ecase (D3DCMP_EQUAL);
    ecase (D3DCMP_LESSEQUAL);
    ecase (D3DCMP_GREATER);
    ecase (D3DCMP_NOTEQUAL);
    ecase (D3DCMP_GREATEREQUAL);
    ecase (D3DCMP_ALWAYS);
    edefault (Value);
  }
}

static char *sD3DFOG (DWORD Value)
{
  switch(Value)
  {
    ecase (D3DFOG_NONE);
    ecase (D3DFOG_EXP);
    ecase (D3DFOG_EXP2);
    ecase (D3DFOG_LINEAR);
    edefault (Value);
  }
}

static char *sD3DSTENCILOP (DWORD Value)
{
  switch(Value)
  {
    ecase (D3DSTENCILOP_KEEP);
    ecase (D3DSTENCILOP_ZERO);
    ecase (D3DSTENCILOP_REPLACE);
    ecase (D3DSTENCILOP_INCRSAT);
    ecase (D3DSTENCILOP_DECRSAT);
    ecase (D3DSTENCILOP_INVERT);
    ecase (D3DSTENCILOP_INCR);
    ecase (D3DSTENCILOP_DECR);
    edefault (Value);
  }
}

static char *sD3DMCS (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DMCS_MATERIAL);
    ecase (D3DMCS_COLOR1);
    ecase (D3DMCS_COLOR2);
    edefault (Value);
  }
}

static char *sD3DVBF (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DVBF_DISABLE);
    ecase (D3DVBF_1WEIGHTS);
    ecase (D3DVBF_2WEIGHTS);
    ecase (D3DVBF_3WEIGHTS);
    ecase (D3DVBF_TWEENING);
    ecase (D3DVBF_0WEIGHTS);
    edefault (Value);
  }
}

static char *sD3DPATCHEDGE (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DPATCHEDGE_DISCRETE);
    ecase (D3DPATCHEDGE_CONTINUOUS);
    edefault (Value);
  }
}

static char *sD3DDMT (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DDMT_ENABLE);
    ecase (D3DDMT_DISABLE);
    edefault (Value);
  }
}

static char *sD3DBLENDOP (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DBLENDOP_ADD);
    ecase (D3DBLENDOP_SUBTRACT);
    ecase (D3DBLENDOP_REVSUBTRACT);
    ecase (D3DBLENDOP_MIN);
    ecase (D3DBLENDOP_MAX);
    edefault (Value);
  }
}

static char *sD3DDEGREE (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DDEGREE_LINEAR);
    ecase (D3DDEGREE_QUADRATIC);
    ecase (D3DDEGREE_CUBIC);
    ecase (D3DDEGREE_QUINTIC);
    edefault (Value);
  }
}

static char *sD3DTOP (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DTOP_DISABLE);
    ecase (D3DTOP_SELECTARG1);
    ecase (D3DTOP_SELECTARG2);
    ecase (D3DTOP_MODULATE);
    ecase (D3DTOP_MODULATE2X);
    ecase (D3DTOP_MODULATE4X);
    ecase (D3DTOP_ADD);
    ecase (D3DTOP_ADDSIGNED);
    ecase (D3DTOP_ADDSIGNED2X);
    ecase (D3DTOP_SUBTRACT);
    ecase (D3DTOP_ADDSMOOTH);
    ecase (D3DTOP_BLENDDIFFUSEALPHA);
    ecase (D3DTOP_BLENDTEXTUREALPHA);
    ecase (D3DTOP_BLENDFACTORALPHA);
    ecase (D3DTOP_BLENDTEXTUREALPHAPM);
    ecase (D3DTOP_BLENDCURRENTALPHA);
    ecase (D3DTOP_PREMODULATE);
    ecase (D3DTOP_MODULATEALPHA_ADDCOLOR);
    ecase (D3DTOP_MODULATECOLOR_ADDALPHA);
    ecase (D3DTOP_MODULATEINVALPHA_ADDCOLOR);
    ecase (D3DTOP_MODULATEINVCOLOR_ADDALPHA);
    ecase (D3DTOP_BUMPENVMAP);
    ecase (D3DTOP_BUMPENVMAPLUMINANCE);
    ecase (D3DTOP_DOTPRODUCT3);
    ecase (D3DTOP_MULTIPLYADD);
    ecase (D3DTOP_LERP);
    edefault (Value);
  }
}

static char *sD3DPT (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DPT_POINTLIST);
    ecase (D3DPT_LINELIST);
    ecase (D3DPT_LINESTRIP);
    ecase (D3DPT_TRIANGLELIST);
    ecase (D3DPT_TRIANGLESTRIP);
    ecase (D3DPT_TRIANGLEFAN);
    edefault (Value);
  }
}

static char *sD3DTA (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DTA_CURRENT);
    ecase (D3DTA_DIFFUSE);
    ecase (D3DTA_SELECTMASK);
    ecase (D3DTA_SPECULAR);
    ecase (D3DTA_TEMP);
    ecase (D3DTA_TEXTURE);
    ecase (D3DTA_TFACTOR);
    edefault (Value);
  }
}

static char *sTEXCOORDINDEX (DWORD Value)
{
  sprintf(sLogStr, "%d", Value & 0xf);
  if (Value & D3DTSS_TCI_CAMERASPACENORMAL)
    strcat (sLogStr, " | D3DTSS_TCI_CAMERASPACENORMAL");
  else
  if (Value & D3DTSS_TCI_CAMERASPACEPOSITION)
    strcat (sLogStr, " | D3DTSS_TCI_CAMERASPACEPOSITION");
  else
  if (Value & D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR)
    strcat (sLogStr, " | D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR");
  else
  if (Value & D3DTSS_TCI_SPHEREMAP)
    strcat (sLogStr, " | D3DTSS_TCI_SPHEREMAP");
  else
    strcat (sLogStr, " | D3DTSS_TCI_PASSTHRU");

  return sLogStr;
}

static char *sD3DTTFF (DWORD Value)
{
  switch(Value & 0xf)
  {
    case D3DTTFF_DISABLE:
      strcpy(sLogStr, "D3DTTFF_DISABLE");
  	  break;
    case D3DTTFF_COUNT1:
      strcpy(sLogStr, "D3DTTFF_COUNT1");
  	  break;
    case D3DTTFF_COUNT2:
      strcpy(sLogStr, "D3DTTFF_COUNT2");
      break;
    case D3DTTFF_COUNT3:
      strcpy(sLogStr, "D3DTTFF_COUNT3");
      break;
    case D3DTTFF_COUNT4:
      strcpy(sLogStr, "D3DTTFF_COUNT4");
      break;
    default:
      sprintf(sLogStr, "0x%x", Value & 0xf);
  }
  if (Value & D3DTTFF_PROJECTED)
    strcat (sLogStr, " | D3DTTFF_PROJECTED");

  return sLogStr;
}

static char *sD3DTADDRESS (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DTADDRESS_WRAP);
    ecase (D3DTADDRESS_MIRROR);
    ecase (D3DTADDRESS_CLAMP);
    ecase (D3DTADDRESS_BORDER);
    ecase (D3DTADDRESS_MIRRORONCE);
    edefault (Value);
  }
}

static char *sD3DTEXF (DWORD Value)
{
  switch (Value)
  {
    ecase (D3DTEXF_NONE);
    ecase (D3DTEXF_POINT);
    ecase (D3DTEXF_LINEAR);
    ecase (D3DTEXF_ANISOTROPIC);
    ecase (D3DTEXF_PYRAMIDALQUAD);
    ecase (D3DTEXF_GAUSSIANQUAD);
    edefault (Value);
  }
}

static char *sD3DTS (DWORD Value)
{
  switch(Value)
  {
    ecase (D3DTS_VIEW);
    ecase (D3DTS_PROJECTION);
    ecase (D3DTS_TEXTURE0);
    ecase (D3DTS_TEXTURE1);
    ecase (D3DTS_TEXTURE2);
    ecase (D3DTS_TEXTURE3);
    ecase (D3DTS_TEXTURE4);
    ecase (D3DTS_TEXTURE5);
    ecase (D3DTS_TEXTURE6);
    ecase (D3DTS_TEXTURE7);
    edefault (Value);
  }
}

static char *sD3DCLEARFLAGS (DWORD Value)
{
  if (!(Value & (D3DCLEAR_TARGET | D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER)))
    sprintf(sLogStr, "0x%x", Value);
  else
  {
    sLogStr[0] = 0;
    if (Value & D3DCLEAR_TARGET)
      strcat(sLogStr, "D3DCLEAR_TARGET");
    if (Value & D3DCLEAR_ZBUFFER)
    {
      if (sLogStr[0])
        strcat (sLogStr, " | ");
      strcat(sLogStr, "D3DCLEAR_ZBUFFER");
    }
    if (Value & D3DCLEAR_STENCIL)
    {
      if (sLogStr[0])
        strcat (sLogStr, " | ");
      strcat(sLogStr, "D3DCLEAR_STENCIL");
    }
  }

  return sLogStr;
}

static char *sFVF (DWORD Flag)
{
  sLogStr[0] = 0;
  eflag(D3DFVF_XYZ);
  eflag(D3DFVF_XYZRHW);
  eflag(D3DFVF_NORMAL);
  eflag(D3DFVF_PSIZE);
  eflag(D3DFVF_DIFFUSE);
  eflag(D3DFVF_SPECULAR);
  char ss[128];
  if(Flag & 0xf00)
  {
    sprintf(ss, "D3DFVF_TEX%d", (Flag >> 8)&0xf);
    if (sLogStr[0])
      strcat (sLogStr, " | ");
    strcat(sLogStr, ss);
  }

  return sLogStr;
}

static char *sD3DFMT (D3DFORMAT Value)
{
  switch (Value)
  {
    ecase(D3DFMT_X8R8G8B8);
    ecase(D3DFMT_A8R8G8B8);
    ecase(D3DFMT_R16F);
    ecase(D3DFMT_G16R16F);
    ecase(D3DFMT_R32F);
    ecase(D3DFMT_A16B16G16R16F);
    edefault (Value);
  }
}

//=========================================================================================

#ifndef OPENGL
HRESULT CMyDirect3DDevice9::QueryInterface(REFIID riid, void** ppvObj)
{



  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::QueryInterface");
  return gcpRendD3D->m_pActuald3dDevice->QueryInterface(riid, ppvObj);

}
ULONG CMyDirect3DDevice9::AddRef()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::AddRef");
  return gcpRendD3D->m_pActuald3dDevice->AddRef();
}
ULONG CMyDirect3DDevice9::Release()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::Release");
  return gcpRendD3D->m_pActuald3dDevice->Release();
}
#endif

  /*** IDirect3DDevice9 methods ***/
HRESULT CMyDirect3DDevice9::TestCooperativeLevel()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::TestCooperativeLevel");
  return gcpRendD3D->m_pActuald3dDevice->TestCooperativeLevel();
}


UINT CMyDirect3DDevice9::GetAvailableTextureMem()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetAvailableTextureMem");
  return gcpRendD3D->m_pActuald3dDevice->GetAvailableTextureMem();
}


HRESULT CMyDirect3DDevice9::EvictManagedResources()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::EvictManagedResources");
  return gcpRendD3D->m_pActuald3dDevice->EvictManagedResources();
}
HRESULT CMyDirect3DDevice9::GetDirect3D(IDirect3D9** ppD3D9)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetDirect3D");
  return gcpRendD3D->m_pActuald3dDevice->GetDirect3D(ppD3D9);
}
HRESULT CMyDirect3DDevice9::GetDeviceCaps(D3DCAPS9* pCaps)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetDeviceCaps");
  return gcpRendD3D->m_pActuald3dDevice->GetDeviceCaps(pCaps);
}
HRESULT CMyDirect3DDevice9::GetDisplayMode(UINT iSwapChain,D3DDISPLAYMODE* pMode)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetDisplayMode");
  return gcpRendD3D->m_pActuald3dDevice->GetDisplayMode(iSwapChain, pMode);
}
HRESULT CMyDirect3DDevice9::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetCreationParameters");
  return gcpRendD3D->m_pActuald3dDevice->GetCreationParameters(pParameters);
}
HRESULT CMyDirect3DDevice9::SetCursorProperties(UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetCursorProperties");
  return gcpRendD3D->m_pActuald3dDevice->SetCursorProperties(XHotSpot,YHotSpot,pCursorBitmap);
}

#if !defined(XENON) && !defined(PS3)
void CMyDirect3DDevice9::SetCursorPosition(int X,int Y,DWORD Flags)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetCursorPosition");
  return gcpRendD3D->m_pActuald3dDevice->SetCursorPosition(X,Y,Flags);
#endif
}
#endif

BOOL CMyDirect3DDevice9::ShowCursor(BOOL bShow)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::ShowCursor");
  return gcpRendD3D->m_pActuald3dDevice->ShowCursor(bShow);
#else
  return true;
#endif
}


#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateAdditionalSwapChain");
  return gcpRendD3D->m_pActuald3dDevice->CreateAdditionalSwapChain( pPresentationParameters, pSwapChain);
#else
  return S_OK;
#endif
}
#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::GetSwapChain(UINT iSwapChain,IDirect3DSwapChain9** pSwapChain)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetSwapChain");
  return gcpRendD3D->m_pActuald3dDevice->GetSwapChain(iSwapChain,pSwapChain);
#else
  return S_OK;
#endif
}
#endif

#if !defined(XENON) && !defined(PS3)
UINT CMyDirect3DDevice9::GetNumberOfSwapChains()
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetNumberOfSwapChains");
  return gcpRendD3D->m_pActuald3dDevice->GetNumberOfSwapChains();
#else
  return 1;
#endif
}    
#endif








HRESULT CMyDirect3DDevice9::Present(CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::Present");
  return gcpRendD3D->m_pActuald3dDevice->Present(pSourceRect,pDestRect,hDestWindowOverride,pDirtyRegion);
}


HRESULT CMyDirect3DDevice9::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::Reset");
  return gcpRendD3D->m_pActuald3dDevice->Reset(pPresentationParameters);
}

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::GetBackBuffer(UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetBackBuffer");
  return gcpRendD3D->m_pActuald3dDevice->GetBackBuffer(iSwapChain,iBackBuffer,Type,ppBackBuffer);
}






#endif

HRESULT CMyDirect3DDevice9::GetRasterStatus(UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetRasterStatus");
  return gcpRendD3D->m_pActuald3dDevice->GetRasterStatus(iSwapChain,pRasterStatus);
}

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::SetDialogBoxMode(BOOL bEnableDialogs)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetDialogBoxMode");
  return gcpRendD3D->m_pActuald3dDevice->SetDialogBoxMode(bEnableDialogs);
#else
  return S_OK;
#endif
}
#endif























void CMyDirect3DDevice9::SetGammaRamp(UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetGammaRamp");
  return gcpRendD3D->m_pActuald3dDevice->SetGammaRamp(iSwapChain,Flags,pRamp);
}

void CMyDirect3DDevice9::GetGammaRamp(UINT iSwapChain,D3DGAMMARAMP* pRamp)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetGammaRamp");
  return gcpRendD3D->m_pActuald3dDevice->GetGammaRamp(iSwapChain,pRamp);
}

HRESULT CMyDirect3DDevice9::CreateTexture(UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateTexture");
  return gcpRendD3D->m_pActuald3dDevice->CreateTexture(Width,Height,Levels,Usage,Format,Pool,ppTexture,pSharedHandle);
}
HRESULT CMyDirect3DDevice9::CreateVolumeTexture(UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateVolumeTexture");
  return gcpRendD3D->m_pActuald3dDevice->CreateVolumeTexture(Width,Height,Depth,Levels,Usage,Format,Pool,ppVolumeTexture,pSharedHandle);
}
HRESULT CMyDirect3DDevice9::CreateCubeTexture(UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateCubeTexture");
  return gcpRendD3D->m_pActuald3dDevice->CreateCubeTexture(EdgeLength,Levels,Usage,Format,Pool,ppCubeTexture,pSharedHandle);
}
HRESULT CMyDirect3DDevice9::CreateVertexBuffer(UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateVertexBuffer");
  return gcpRendD3D->m_pActuald3dDevice->CreateVertexBuffer(Length,Usage,FVF,Pool, ppVertexBuffer, pSharedHandle);
}
HRESULT CMyDirect3DDevice9::CreateIndexBuffer(UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateIndexBuffer");
  return gcpRendD3D->m_pActuald3dDevice->CreateIndexBuffer(Length,Usage,Format,Pool, ppIndexBuffer,pSharedHandle);
}

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::CreateRenderTarget(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateRenderTarget");
	return gcpRendD3D->m_pActuald3dDevice->CreateRenderTarget(Width,Height,Format,MultiSample,MultisampleQuality,Lockable, ppSurface,
#if defined(__GNUC__)
	    reinterpret_cast<void **>(pSharedHandle)
#else
	    pSharedHandle
#endif
	    );
}






#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::CreateDepthStencilSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateDepthStencilSurface");
	return gcpRendD3D->m_pActuald3dDevice->CreateDepthStencilSurface(Width,Height,Format,MultiSample,MultisampleQuality,Discard, ppSurface,
#if defined(__GNUC__)
	    reinterpret_cast<void **>(pSharedHandle)
#else
	    pSharedHandle
#endif
	    );
}






#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::UpdateSurface(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::UpdateSurface");
  return gcpRendD3D->m_pActuald3dDevice->UpdateSurface(pSourceSurface,pSourceRect,pDestinationSurface, pDestPoint);
}
#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::UpdateTexture(IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::UpdateTexture");
  return gcpRendD3D->m_pActuald3dDevice->UpdateTexture(pSourceTexture,pDestinationTexture);
}
#endif









HRESULT CMyDirect3DDevice9::GetRenderTargetData(IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetRenderTargetData");
  return gcpRendD3D->m_pActuald3dDevice->GetRenderTargetData(pRenderTarget,pDestSurface);
}
HRESULT CMyDirect3DDevice9::GetFrontBufferData(UINT iSwapChain,IDirect3DSurface9* pDestSurface)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetFrontBufferData");
  return gcpRendD3D->m_pActuald3dDevice->GetFrontBufferData(iSwapChain,pDestSurface);
}
HRESULT CMyDirect3DDevice9::StretchRect(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::StretchRect");
  return gcpRendD3D->m_pActuald3dDevice->StretchRect(pSourceSurface,pSourceRect,pDestSurface,pDestRect,Filter);
}
HRESULT CMyDirect3DDevice9::ColorFill(IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (0x%x, 0x%x, 0x%x)\n", "D3DDevice::ColorFill", pSurface, pRect, color);
  return gcpRendD3D->m_pActuald3dDevice->ColorFill(pSurface,pRect,color);
}
HRESULT CMyDirect3DDevice9::CreateOffscreenPlainSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateOffscreenPlainSurface");
  return gcpRendD3D->m_pActuald3dDevice->CreateOffscreenPlainSurface(Width,Height,Format,Pool, ppSurface, pSharedHandle);
}

UINT WINAPI GetD3D9ColorBits( D3DFORMAT fmt );
HRESULT CMyDirect3DDevice9::SetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget)
{
  D3DSURFACE_DESC dtdsdRT;
  if (pRenderTarget)
  {
    pRenderTarget->GetDesc(&dtdsdRT);
    int nBPP = GetD3D9ColorBits(dtdsdRT.Format) / 8;
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, 0x%x) (%dx%d x %s: %.3fMb)\n", "D3DDevice::SetRenderTarget", RenderTargetIndex, pRenderTarget, dtdsdRT.Width, dtdsdRT.Height, sD3DFMT(dtdsdRT.Format), (float)(dtdsdRT.Width*dtdsdRT.Height*nBPP) / 1024.0f / 1024.0f);
  }
  else
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, 0x%x)\n", "D3DDevice::SetRenderTarget", RenderTargetIndex, pRenderTarget);
  return gcpRendD3D->m_pActuald3dDevice->SetRenderTarget(RenderTargetIndex,pRenderTarget);
}
HRESULT CMyDirect3DDevice9::GetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetRenderTarget");
  return gcpRendD3D->m_pActuald3dDevice->GetRenderTarget(RenderTargetIndex, ppRenderTarget);
}
HRESULT CMyDirect3DDevice9::SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (0x%x)\n", "D3DDevice::SetDepthStencilSurface", pNewZStencil);
  return gcpRendD3D->m_pActuald3dDevice->SetDepthStencilSurface(pNewZStencil);
}
HRESULT CMyDirect3DDevice9::GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetDepthStencilSurface");
  return gcpRendD3D->m_pActuald3dDevice->GetDepthStencilSurface(ppZStencilSurface);
}
HRESULT CMyDirect3DDevice9::BeginScene()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::BeginScene");
  return gcpRendD3D->m_pActuald3dDevice->BeginScene();
}
HRESULT CMyDirect3DDevice9::EndScene()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::EndScene");
  return gcpRendD3D->m_pActuald3dDevice->EndScene();
}
HRESULT CMyDirect3DDevice9::Clear(DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, 0x%x, %s, 0x%x, %.3f, 0x%x)\n", "D3DDevice::Clear", Count, pRects, sD3DCLEARFLAGS(Flags), Color, Z, Stencil);
  return gcpRendD3D->m_pActuald3dDevice->Clear(Count,pRects,Flags,Color,Z,Stencil);
}
HRESULT CMyDirect3DDevice9::SetTransform(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix)
{
  char *state = sD3DTS(State);
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%s, 0x%x)\n", "D3DDevice::SetTransform", state, pMatrix);
  return gcpRendD3D->m_pActuald3dDevice->SetTransform(State,pMatrix);
}

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::GetTransform(D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetTransform");
  return gcpRendD3D->m_pActuald3dDevice->GetTransform(State,pMatrix);
#else
  return S_OK;
#endif
}
#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::MultiplyTransform(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::MultiplyTransform");
  return gcpRendD3D->m_pActuald3dDevice->MultiplyTransform(State,pMatrix);
#else
  return S_OK;
#endif
}
#endif

HRESULT CMyDirect3DDevice9::SetViewport(CONST D3DVIEWPORT9* pViewport)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, %d, %d, %d, %.3f, %.3f)\n", "D3DDevice::SetViewport", pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height, pViewport->MinZ, pViewport->MaxZ);
  return gcpRendD3D->m_pActuald3dDevice->SetViewport(pViewport);
}
HRESULT CMyDirect3DDevice9::GetViewport(D3DVIEWPORT9* pViewport)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetViewport");
  return gcpRendD3D->m_pActuald3dDevice->GetViewport(pViewport);
}

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::SetMaterial(CONST D3DMATERIAL9* pMaterial)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetMaterial");
  return gcpRendD3D->m_pActuald3dDevice->SetMaterial(pMaterial);
#else
  return S_OK;
#endif
}
#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::GetMaterial(D3DMATERIAL9* pMaterial)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetMaterial");
  return gcpRendD3D->m_pActuald3dDevice->GetMaterial(pMaterial);
#else
  return S_OK;
#endif
}
#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::SetLight(DWORD Index,CONST D3DLIGHT9* pLight)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, 0x%x)\n", "D3DDevice::SetLight", Index, pLight);
  return gcpRendD3D->m_pActuald3dDevice->SetLight(Index,pLight);
#else
  return S_OK;
#endif
}
#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::GetLight(DWORD Index,D3DLIGHT9* pLight)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetLight");
  return gcpRendD3D->m_pActuald3dDevice->GetLight(Index,pLight);
#else
  return S_OK;
#endif
}
#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::LightEnable(DWORD Index,BOOL Enable)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, %s)\n", "D3DDevice::LightEnable", Index, sBool(Enable));
  return gcpRendD3D->m_pActuald3dDevice->LightEnable(Index,Enable);
#else
  return S_OK;
#endif
}
#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::GetLightEnable(DWORD Index,BOOL* pEnable)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetLightEnable");
  return gcpRendD3D->m_pActuald3dDevice->GetLightEnable(Index,pEnable);
#else
  return S_OK;
#endif
}
#endif

HRESULT CMyDirect3DDevice9::SetClipPlane(DWORD Index,CONST float* pPlane)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetClipPlane");
  return gcpRendD3D->m_pActuald3dDevice->SetClipPlane(Index,pPlane);
}
HRESULT CMyDirect3DDevice9::GetClipPlane(DWORD Index,float* pPlane)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetClipPlane");
  return gcpRendD3D->m_pActuald3dDevice->GetClipPlane(Index,pPlane);
}
HRESULT CMyDirect3DDevice9::SetRenderState(D3DRENDERSTATETYPE State,DWORD Value)
{
#define ECASE(e) case e: state = #e
  char Str[1024];
#define EDEFAULT(e) default: sprintf(Str, "0x%x", e); state = Str;
  char *state;
  char *val = NULL;
  switch(State)
  {








    ECASE (D3DRS_ZENABLE);
    val = sD3DZB(Value);
    break;
    ECASE (D3DRS_FILLMODE);
    val = sD3DFILL(Value);
    break;
#if !defined(XENON) && !defined(PS3)
    ECASE (D3DRS_SHADEMODE);
    val = sD3DSHADE(Value);
    break;
    ECASE (D3DRS_DITHERENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_FOGENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_SPECULARENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_FOGCOLOR);
    val = sHex(Value);
    break;
    ECASE (D3DRS_FOGTABLEMODE);
    val = sD3DFOG(Value);
    break;
    ECASE (D3DRS_FOGSTART);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_FOGEND);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_FOGDENSITY);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_RANGEFOGENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_TEXTUREFACTOR);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP0);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP1);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP2);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP3);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP4);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP5);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP6);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP7);
    val = sHex(Value);
    break;
    ECASE (D3DRS_CLIPPING);
    val = sBool(Value);
    break;
    ECASE (D3DRS_LIGHTING);
    val = sBool(Value);
    break;
    ECASE (D3DRS_AMBIENT);
    val = sHex(Value);
    break;
    ECASE (D3DRS_FOGVERTEXMODE);
    val = sD3DFOG(Value);
    break;
    ECASE (D3DRS_COLORVERTEX);
    val = sBool(Value);
    break;
    ECASE (D3DRS_LOCALVIEWER);
    val = sBool(Value);
    break;
    ECASE (D3DRS_NORMALIZENORMALS);
    val = sBool(Value);
    break;
    ECASE (D3DRS_DIFFUSEMATERIALSOURCE);
    val = sD3DMCS(Value);
    break;
    ECASE (D3DRS_SPECULARMATERIALSOURCE);
    val = sD3DMCS(Value);
    break;
    ECASE (D3DRS_AMBIENTMATERIALSOURCE);
    val = sD3DMCS(Value);
    break;
    ECASE (D3DRS_EMISSIVEMATERIALSOURCE);
    val = sD3DMCS(Value);
    break;
    ECASE (D3DRS_POINTSCALE_A);
    break;
    ECASE (D3DRS_POINTSCALE_B);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_POINTSCALE_C);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_DEBUGMONITORTOKEN);
    val = sD3DDMT(Value);
    break;
    ECASE (D3DRS_TWEENFACTOR);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_PATCHEDGESTYLE);
    val = sD3DPATCHEDGE(Value);
    break;
    ECASE (D3DRS_POSITIONDEGREE);
    val = sD3DDEGREE(Value);
    break;
    ECASE (D3DRS_NORMALDEGREE);
    val = sD3DDEGREE(Value);
    break;
    ECASE (D3DRS_ANTIALIASEDLINEENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_ADAPTIVETESS_X);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_ADAPTIVETESS_Y);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_ADAPTIVETESS_Z);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_ADAPTIVETESS_W);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_ENABLEADAPTIVETESSELLATION);
    val = sBool(Value);
    break;
    ECASE (D3DRS_SRGBWRITEENABLE);
    val = sBool(Value);
    break;
#endif
    ECASE (D3DRS_ZWRITEENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_ALPHATESTENABLE);
    val = sBool(Value);
    if (Value && (gcpRendD3D->m_RTStack[0][gcpRendD3D->m_nRTStackLevel[0]].m_pTex))
    {
      CTexture *pTex = gcpRendD3D->m_RTStack[0][gcpRendD3D->m_nRTStackLevel[0]].m_pTex;
      if (!(gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_ATOC))
      {
        if (pTex->GetDstFormat() == eTF_G16R16F || pTex->GetDstFormat() == eTF_R32F || pTex->GetDstFormat() == eTF_R16F)
        {
          assert(0);
        }
      }
    }
    break;
    ECASE (D3DRS_SRCBLEND);
    val = sD3DBLEND(Value);
    break;
    ECASE (D3DRS_DESTBLEND);
    val = sD3DBLEND(Value);
    break;
    ECASE (D3DRS_CULLMODE);
    val = sD3DCULL(Value);
    break;
    ECASE (D3DRS_ZFUNC);
    val = sD3DCMP(Value);
    break;
    ECASE (D3DRS_ALPHAREF);
    val = sInt(Value);
    break;
    ECASE (D3DRS_ALPHAFUNC);
    val = sD3DCMP(Value);
    break;
    ECASE (D3DRS_ALPHABLENDENABLE);
    val = sBool(Value);
    if (Value && (gcpRendD3D->m_RTStack[0][gcpRendD3D->m_nRTStackLevel[0]].m_pTex))
    {
      CTexture *pTex = gcpRendD3D->m_RTStack[0][gcpRendD3D->m_nRTStackLevel[0]].m_pTex;
      if (pTex->GetDstFormat() == eTF_G16R16F || pTex->GetDstFormat() == eTF_R32F || pTex->GetDstFormat() == eTF_R16F)
      {
        assert(0);
      }
    }
    break;
    ECASE (D3DRS_STENCILENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_STENCILFAIL);
    val = sD3DSTENCILOP(Value);
    break;
    ECASE (D3DRS_STENCILZFAIL);
    val = sD3DSTENCILOP(Value);
    break;
    ECASE (D3DRS_STENCILPASS);
    val = sD3DSTENCILOP(Value);
    break;
    ECASE (D3DRS_STENCILFUNC);
    val = sD3DCMP(Value);
    break;
    ECASE (D3DRS_STENCILREF);
    val = sInt(Value);
    break;
    ECASE (D3DRS_STENCILMASK);
    val = sHex(Value);
    break;
    ECASE (D3DRS_STENCILWRITEMASK);
    val = sHex(Value);
    break;
    ECASE (D3DRS_VERTEXBLEND);
    val = sD3DVBF(Value);
    break;
    ECASE (D3DRS_CLIPPLANEENABLE);
    val = sHex(Value);
    break;
    ECASE (D3DRS_POINTSIZE);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_POINTSIZE_MIN);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_POINTSPRITEENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_POINTSCALEENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_MULTISAMPLEANTIALIAS);
    val = sBool(Value);
    break;
    ECASE (D3DRS_MULTISAMPLEMASK);
    val = sHex(Value);
    break;
    ECASE (D3DRS_POINTSIZE_MAX);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_INDEXEDVERTEXBLENDENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_COLORWRITEENABLE);
    val = sHex(Value);
    break;
    ECASE (D3DRS_BLENDOP);
    val = sD3DBLENDOP(Value);
    break;
    ECASE (D3DRS_SCISSORTESTENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_SLOPESCALEDEPTHBIAS);
    val = sHex(Value);
    break;
    ECASE (D3DRS_MINTESSELLATIONLEVEL);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_MAXTESSELLATIONLEVEL);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_TWOSIDEDSTENCILMODE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_CCW_STENCILFAIL);
    val = sD3DSTENCILOP(Value);
    break;
    ECASE (D3DRS_CCW_STENCILZFAIL);
    val = sD3DSTENCILOP(Value);
    break;
    ECASE (D3DRS_CCW_STENCILPASS);
    val = sD3DSTENCILOP(Value);
    break;
    ECASE (D3DRS_CCW_STENCILFUNC);
    val = sD3DCMP(Value);
    break;
    ECASE (D3DRS_COLORWRITEENABLE1);
    val = sHex(Value);
    break;
    ECASE (D3DRS_COLORWRITEENABLE2);
    val = sHex(Value);
    break;
    ECASE (D3DRS_COLORWRITEENABLE3);
    val = sHex(Value);
    break;
    ECASE (D3DRS_BLENDFACTOR);
    val = sHex(Value);
    break;
    ECASE (D3DRS_DEPTHBIAS);
    val = sDWFloat(Value);
    break;
    ECASE (D3DRS_WRAP8);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP9);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP10);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP11);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP12);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP13);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP14);
    val = sHex(Value);
    break;
    ECASE (D3DRS_WRAP15);
    val = sHex(Value);
    break;
    ECASE (D3DRS_SEPARATEALPHABLENDENABLE);
    val = sBool(Value);
    break;
    ECASE (D3DRS_SRCBLENDALPHA);
    val = sD3DBLEND(Value);
    break;
    ECASE (D3DRS_DESTBLENDALPHA);
    val = sD3DBLEND(Value);
    break;
    ECASE (D3DRS_BLENDOPALPHA);
    val = sD3DBLENDOP(Value);
    break;
    EDEFAULT (State);
  }
  if (!val)
    val = sHex(Value);
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%s, %s)\n", "D3DDevice::SetRenderState", state, val);
  return gcpRendD3D->m_pActuald3dDevice->SetRenderState(State,Value);
#undef ECASE
#undef EDEFAULT
}
HRESULT CMyDirect3DDevice9::GetRenderState(D3DRENDERSTATETYPE State,DWORD* pValue)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetRenderState");
  return gcpRendD3D->m_pActuald3dDevice->GetRenderState(State,pValue);
}
HRESULT CMyDirect3DDevice9::CreateStateBlock(D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateStateBlock");
  return gcpRendD3D->m_pActuald3dDevice->CreateStateBlock(Type,ppSB);
#else
  return S_OK;
#endif
}

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::BeginStateBlock()
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::BeginStateBlock");
  return gcpRendD3D->m_pActuald3dDevice->BeginStateBlock();
#else
  return S_OK;
#endif
}
HRESULT CMyDirect3DDevice9::EndStateBlock(IDirect3DStateBlock9** ppSB)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::EndStateBlock");
  return gcpRendD3D->m_pActuald3dDevice->EndStateBlock(ppSB);
#else
  return S_OK;
#endif
}
#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetClipStatus");
  return gcpRendD3D->m_pActuald3dDevice->SetClipStatus(pClipStatus);
#else
  return S_OK;
#endif
}
HRESULT CMyDirect3DDevice9::GetClipStatus(D3DCLIPSTATUS9* pClipStatus)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetClipStatus");
  return gcpRendD3D->m_pActuald3dDevice->GetClipStatus(pClipStatus);
#else
  return S_OK;
#endif
}
#endif

HRESULT CMyDirect3DDevice9::GetTexture(DWORD Stage,IDirect3DBaseTexture9** ppTexture)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetTexture");
  return gcpRendD3D->m_pActuald3dDevice->GetTexture(Stage, ppTexture);
}
HRESULT CMyDirect3DDevice9::SetTexture(DWORD Stage,IDirect3DBaseTexture9* pTexture)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, 0x%x)\n", "D3DDevice::SetTexture", Stage, pTexture);
  return gcpRendD3D->m_pActuald3dDevice->SetTexture(Stage,pTexture);
}

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::GetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetTextureStageState");
  return gcpRendD3D->m_pActuald3dDevice->GetTextureStageState(Stage,Type,pValue);
#else
  return S_OK;
#endif
}
#endif

HRESULT CMyDirect3DDevice9::SetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value)
{
#define ECASE(e) case e: type = #e
  char Str[1024];
#define EDEFAULT(e) default: sprintf(Str, "0x%x", e); type = Str;
  char *type;
  char *val = NULL;
  switch (Type)
  {
    ECASE (D3DTSS_COLOROP);
    val = sD3DTOP(Value);
    break;
    ECASE (D3DTSS_COLORARG1);
    val = sD3DTA(Value);
    break;
    ECASE (D3DTSS_COLORARG2);
    val = sD3DTA(Value);
    break;
    ECASE (D3DTSS_ALPHAOP);
    val = sD3DTOP(Value);
    break;
    ECASE (D3DTSS_ALPHAARG1);
    val = sD3DTA(Value);
    break;
    ECASE (D3DTSS_ALPHAARG2);
    val = sD3DTA(Value);
    break;
    ECASE (D3DTSS_BUMPENVMAT00);
    val = sDWFloat(Value);
    break;
    ECASE (D3DTSS_BUMPENVMAT01);
    val = sDWFloat(Value);
    break;
    ECASE (D3DTSS_BUMPENVMAT10);
    val = sDWFloat(Value);
    break;
    ECASE (D3DTSS_BUMPENVMAT11);
    val = sDWFloat(Value);
    break;
    ECASE (D3DTSS_TEXCOORDINDEX);
    val = sTEXCOORDINDEX(Value);
    break;
    ECASE (D3DTSS_BUMPENVLSCALE);
    val = sDWFloat(Value);
    break;
    ECASE (D3DTSS_BUMPENVLOFFSET);
    val = sDWFloat(Value);
    break;
    ECASE (D3DTSS_TEXTURETRANSFORMFLAGS);
    val = sD3DTTFF(Value);
    break;
    ECASE (D3DTSS_COLORARG0);
    val = sD3DTA(Value);
    break;
    ECASE (D3DTSS_ALPHAARG0);
    val = sD3DTA(Value);
    break;
    ECASE (D3DTSS_RESULTARG);
    val = sD3DTA(Value);
    break;
    ECASE (D3DTSS_CONSTANT);
    val = sHex(Value);
    break;
    EDEFAULT (Type);
  }
  if (!val)
    val = sHex(Value);
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, %s, %s)\n", "D3DDevice::SetTextureStageState", Stage, type, val);
  return gcpRendD3D->m_pActuald3dDevice->SetTextureStageState(Stage,Type,Value);
#undef ECASE
#undef EDEFAULT
}
HRESULT CMyDirect3DDevice9::GetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetSamplerState");
  return gcpRendD3D->m_pActuald3dDevice->GetSamplerState(Sampler,Type, pValue);
}
HRESULT CMyDirect3DDevice9::SetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value)
{
#define ECASE(e) case e: type = #e
  char Str[1024];
#define EDEFAULT(e) default: sprintf(Str, "0x%x", e); type = Str;
  char *type;
  char *val = NULL;
  switch(Type)
  {
    ECASE (D3DSAMP_ADDRESSU);
    val = sD3DTADDRESS(Value);
    break;
    ECASE (D3DSAMP_ADDRESSV);
    val = sD3DTADDRESS(Value);
    break;
    ECASE (D3DSAMP_ADDRESSW);
    val = sD3DTADDRESS(Value);
    break;
    ECASE (D3DSAMP_BORDERCOLOR);
    val = sHex(Value);
    break;
    ECASE (D3DSAMP_MAGFILTER);
    val = sD3DTEXF(Value);
    break;
    ECASE (D3DSAMP_MINFILTER);
    val = sD3DTEXF(Value);
    break;
    ECASE (D3DSAMP_MIPFILTER);
    val = sD3DTEXF(Value);
    break;
    ECASE (D3DSAMP_MIPMAPLODBIAS);
    val = sDWFloat(Value);
    break;
    ECASE (D3DSAMP_MAXMIPLEVEL);
    val = sInt(Value);
    break;
    ECASE (D3DSAMP_MAXANISOTROPY);
    val = sInt(Value);
    break;
#if !defined(XENON) && !defined(PS3)
    ECASE (D3DSAMP_SRGBTEXTURE);
    val = sBool(Value);
    break;
    ECASE (D3DSAMP_ELEMENTINDEX);
    val = sInt(Value);
    break;
    ECASE (D3DSAMP_DMAPOFFSET);
    val = sInt(Value);
    break;
#endif
    EDEFAULT (Type);
  }
  if (!val)
    val = sHex(Value);
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, %s, %s)\n", "D3DDevice::SetSamplerState", Sampler, type, val);
  return gcpRendD3D->m_pActuald3dDevice->SetSamplerState(Sampler,Type,Value);
#undef ECASE
#undef EDEFAULT
}

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::ValidateDevice(DWORD* pNumPasses)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::ValidateDevice");
  return gcpRendD3D->m_pActuald3dDevice->ValidateDevice(pNumPasses);
#else
  return S_OK;
#endif
}
HRESULT CMyDirect3DDevice9::SetPaletteEntries(UINT PaletteNumber,CONST PALETTEENTRY* pEntries)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetPaletteEntries");
  return gcpRendD3D->m_pActuald3dDevice->SetPaletteEntries(PaletteNumber,pEntries);
#else
  return S_OK;
#endif
}
HRESULT CMyDirect3DDevice9::GetPaletteEntries(UINT PaletteNumber,PALETTEENTRY* pEntries)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetPaletteEntries");
  return gcpRendD3D->m_pActuald3dDevice->GetPaletteEntries(PaletteNumber,pEntries);
#else
  return S_OK;
#endif
}
HRESULT CMyDirect3DDevice9::SetCurrentTexturePalette(UINT PaletteNumber)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetCurrentTexturePalette");
  return gcpRendD3D->m_pActuald3dDevice->SetCurrentTexturePalette(PaletteNumber);
#else
  return S_OK;
#endif
}
HRESULT CMyDirect3DDevice9::GetCurrentTexturePalette(UINT *PaletteNumber)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetCurrentTexturePalette");
  return gcpRendD3D->m_pActuald3dDevice->GetCurrentTexturePalette(PaletteNumber);
#else
  return S_OK;
#endif
}
#endif

HRESULT CMyDirect3DDevice9::SetScissorRect(CONST RECT* pRect)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, %d, %d)\n", "D3DDevice::SetScissorRect", pRect->left, pRect->right, pRect->top, pRect->bottom);
  return gcpRendD3D->m_pActuald3dDevice->SetScissorRect(pRect);
}
HRESULT CMyDirect3DDevice9::GetScissorRect(RECT* pRect)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetScissorRect");
  return gcpRendD3D->m_pActuald3dDevice->GetScissorRect(pRect);
}

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::SetSoftwareVertexProcessing(BOOL bSoftware)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetSoftwareVertexProcessing");
  return gcpRendD3D->m_pActuald3dDevice->SetSoftwareVertexProcessing(bSoftware);
#else
  return S_OK;
#endif
}
BOOL CMyDirect3DDevice9::GetSoftwareVertexProcessing()
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetSoftwareVertexProcessing");
  return gcpRendD3D->m_pActuald3dDevice->GetSoftwareVertexProcessing();
#else
  return S_OK;
#endif
}
#endif

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::SetNPatchMode(float nSegments)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetNPatchMode");
  return gcpRendD3D->m_pActuald3dDevice->SetNPatchMode(nSegments);
}
float CMyDirect3DDevice9::GetNPatchMode()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetNPatchMode");
  return gcpRendD3D->m_pActuald3dDevice->GetNPatchMode();
}
#endif
HRESULT CMyDirect3DDevice9::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s  (%s, %d, %d)\n", "D3DDevice::DrawPrimitive", sD3DPT(PrimitiveType), StartVertex, PrimitiveCount);
  return gcpRendD3D->m_pActuald3dDevice->DrawPrimitive(PrimitiveType,StartVertex,PrimitiveCount);
}
HRESULT CMyDirect3DDevice9::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%s, %d, %d, %d, %d, %d)\n", "D3DDevice::DrawIndexedPrimitive", sD3DPT(PrimitiveType), BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
  return gcpRendD3D->m_pActuald3dDevice->DrawIndexedPrimitive(PrimitiveType,BaseVertexIndex,MinVertexIndex,NumVertices,startIndex,primCount);
}
HRESULT CMyDirect3DDevice9::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
{
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%s, %d, 0x%x, %d)\n", "D3DDevice::DrawPrimitiveUP", sD3DPT(PrimitiveType), PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
  return gcpRendD3D->m_pActuald3dDevice->DrawPrimitiveUP(PrimitiveType,PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride);
}
HRESULT CMyDirect3DDevice9::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::DrawIndexedPrimitiveUP");
  return gcpRendD3D->m_pActuald3dDevice->DrawIndexedPrimitiveUP(PrimitiveType,MinVertexIndex,NumVertices,PrimitiveCount, pIndexData,IndexDataFormat,pVertexStreamZeroData,VertexStreamZeroStride);
}

#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::ProcessVertices(UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags)
{
#ifndef OPENGL
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::ProcessVertices");
  return gcpRendD3D->m_pActuald3dDevice->ProcessVertices(SrcStartIndex,DestIndex,VertexCount,pDestBuffer,pVertexDecl,Flags);
#else
  return S_OK;
#endif
}
#endif

HRESULT CMyDirect3DDevice9::CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateVertexDeclaration");
  return gcpRendD3D->m_pActuald3dDevice->CreateVertexDeclaration(pVertexElements,ppDecl);
}
HRESULT CMyDirect3DDevice9::SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (0x%x)\n", "D3DDevice::SetVertexDeclaration", pDecl);
  return gcpRendD3D->m_pActuald3dDevice->SetVertexDeclaration(pDecl);
}
HRESULT CMyDirect3DDevice9::GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetVertexDeclaration");
  return gcpRendD3D->m_pActuald3dDevice->GetVertexDeclaration( ppDecl);
}
HRESULT CMyDirect3DDevice9::SetFVF(DWORD FVF)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%s)\n", "D3DDevice::SetFVF", sFVF(FVF));
  return gcpRendD3D->m_pActuald3dDevice->SetFVF(FVF);
}
HRESULT CMyDirect3DDevice9::GetFVF(DWORD* pFVF)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetFVF");
  return gcpRendD3D->m_pActuald3dDevice->GetFVF( pFVF);
}
HRESULT CMyDirect3DDevice9::CreateVertexShader(CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateVertexShader");
  return gcpRendD3D->m_pActuald3dDevice->CreateVertexShader(pFunction, ppShader);
}
HRESULT CMyDirect3DDevice9::SetVertexShader(IDirect3DVertexShader9* pShader)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (0x%x)\n", "D3DDevice::SetVertexShader", pShader);
  return gcpRendD3D->m_pActuald3dDevice->SetVertexShader( pShader);
}
HRESULT CMyDirect3DDevice9::GetVertexShader(IDirect3DVertexShader9** ppShader)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetVertexShader");
  return gcpRendD3D->m_pActuald3dDevice->GetVertexShader( ppShader);
}
HRESULT CMyDirect3DDevice9::SetVertexShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d", "D3DDevice::SetVertexShaderConstantF", StartRegister);
  const float *v = pConstantData;
  for (int i=0; i<(int)Vector4fCount; i++)
  {
    gcpRendD3D->Logv(0, ", [%.3f, %.3f, %.3f, %.3f]", v[0], v[1], v[2], v[3]);
    v += 4;
  }
  gcpRendD3D->Logv(0, ", %d)\n", Vector4fCount);
  return gcpRendD3D->m_pActuald3dDevice->SetVertexShaderConstantF(StartRegister,pConstantData,Vector4fCount);
}
HRESULT CMyDirect3DDevice9::GetVertexShaderConstantF(UINT StartRegister,float* pConstantData,UINT Vector4fCount)
{
  //gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetVertexShaderConstantF");
  return gcpRendD3D->m_pActuald3dDevice->GetVertexShaderConstantF(StartRegister,pConstantData,Vector4fCount);
}
HRESULT CMyDirect3DDevice9::SetVertexShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetVertexShaderConstantI");
  return gcpRendD3D->m_pActuald3dDevice->SetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount);
}
HRESULT CMyDirect3DDevice9::GetVertexShaderConstantI(UINT StartRegister,int* pConstantData,UINT Vector4iCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetVertexShaderConstantI");
  return gcpRendD3D->m_pActuald3dDevice->GetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount);
}
HRESULT CMyDirect3DDevice9::SetVertexShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetVertexShaderConstantB");
  return gcpRendD3D->m_pActuald3dDevice->SetVertexShaderConstantB(StartRegister,pConstantData,BoolCount);
}
HRESULT CMyDirect3DDevice9::GetVertexShaderConstantB(UINT StartRegister,BOOL* pConstantData,UINT BoolCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetVertexShaderConstantB");
  return gcpRendD3D->m_pActuald3dDevice->GetVertexShaderConstantB(StartRegister,pConstantData,BoolCount);
}
HRESULT CMyDirect3DDevice9::SetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, 0x%x, %d, %d)\n", "D3DDevice::SetStreamSource", StreamNumber, pStreamData, OffsetInBytes, Stride);
  return gcpRendD3D->m_pActuald3dDevice->SetStreamSource(StreamNumber,pStreamData,OffsetInBytes,Stride);
}
HRESULT CMyDirect3DDevice9::GetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* OffsetInBytes,UINT* pStride)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetStreamSource");
  return gcpRendD3D->m_pActuald3dDevice->GetStreamSource(StreamNumber, ppStreamData, OffsetInBytes, pStride);
}
HRESULT CMyDirect3DDevice9::SetStreamSourceFreq(UINT StreamNumber,UINT Divider)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d, 0x%x)\n", "D3DDevice::SetStreamSourceFreq", StreamNumber, Divider);
  return gcpRendD3D->m_pActuald3dDevice->SetStreamSourceFreq(StreamNumber,Divider);
}
HRESULT CMyDirect3DDevice9::GetStreamSourceFreq(UINT StreamNumber,UINT* Divider)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetStreamSourceFreq");
  return gcpRendD3D->m_pActuald3dDevice->GetStreamSourceFreq(StreamNumber,Divider);
}
HRESULT CMyDirect3DDevice9::SetIndices(IDirect3DIndexBuffer9* pIndexData)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (0x%x)\n", "D3DDevice::SetIndices", pIndexData);
  return gcpRendD3D->m_pActuald3dDevice->SetIndices( pIndexData);
}
HRESULT CMyDirect3DDevice9::GetIndices(IDirect3DIndexBuffer9** ppIndexData)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetIndices");
  return gcpRendD3D->m_pActuald3dDevice->GetIndices( ppIndexData);
}
HRESULT CMyDirect3DDevice9::CreatePixelShader(CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreatePixelShader");
  return gcpRendD3D->m_pActuald3dDevice->CreatePixelShader(pFunction,ppShader);
}
HRESULT CMyDirect3DDevice9::SetPixelShader(IDirect3DPixelShader9* pShader)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (0x%x)\n", "D3DDevice::SetPixelShader", pShader);
  return gcpRendD3D->m_pActuald3dDevice->SetPixelShader( pShader);
}
HRESULT CMyDirect3DDevice9::GetPixelShader(IDirect3DPixelShader9** ppShader)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (0x%x)\n", "D3DDevice::GetPixelShader", ppShader);
  return gcpRendD3D->m_pActuald3dDevice->GetPixelShader( ppShader);
}
HRESULT CMyDirect3DDevice9::SetPixelShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d", "D3DDevice::SetPixelShaderConstantF", StartRegister);
  const float *v = pConstantData;
  for (int i=0; i<(int)Vector4fCount; i++)
  {
    gcpRendD3D->Logv(0, ", [%.3f, %.3f, %.3f, %.3f]", v[0], v[1], v[2], v[3]);
    v += 4;
  }
  gcpRendD3D->Logv(0, ", %d)\n", Vector4fCount);
  return gcpRendD3D->m_pActuald3dDevice->SetPixelShaderConstantF(StartRegister,pConstantData,Vector4fCount);
}
HRESULT CMyDirect3DDevice9::GetPixelShaderConstantF(UINT StartRegister,float* pConstantData,UINT Vector4fCount)
{
  //gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetPixelShaderConstantF");
  return gcpRendD3D->m_pActuald3dDevice->GetPixelShaderConstantF(StartRegister,pConstantData,Vector4fCount);
}
HRESULT CMyDirect3DDevice9::SetPixelShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s (%d", "D3DDevice::SetPixelShaderConstantI", StartRegister);
  const int *v = pConstantData;
  for (int i=0; i<(int)Vector4iCount; i++)
  {
    gcpRendD3D->Logv(0, ", [%d, %d, %d, %d]", v[0], v[1], v[2], v[3]);
    v += 4;
  }
  gcpRendD3D->Logv(0, ", %d)\n", Vector4iCount);
  return gcpRendD3D->m_pActuald3dDevice->SetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount);
}
HRESULT CMyDirect3DDevice9::GetPixelShaderConstantI(UINT StartRegister,int* pConstantData,UINT Vector4iCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetPixelShaderConstantI");
  return gcpRendD3D->m_pActuald3dDevice->GetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount);
}
HRESULT CMyDirect3DDevice9::SetPixelShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::SetPixelShaderConstantB");
  return gcpRendD3D->m_pActuald3dDevice->SetPixelShaderConstantB(StartRegister,pConstantData,BoolCount);
}
HRESULT CMyDirect3DDevice9::GetPixelShaderConstantB(UINT StartRegister,BOOL* pConstantData,UINT BoolCount)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetPixelShaderConstantB");
  return gcpRendD3D->m_pActuald3dDevice->GetPixelShaderConstantB(StartRegister,pConstantData,BoolCount);
}
#if !defined(XENON) && !defined(PS3)
HRESULT CMyDirect3DDevice9::DrawRectPatch(UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::DrawRectPatch");
  return gcpRendD3D->m_pActuald3dDevice->DrawRectPatch(Handle,pNumSegs,pRectPatchInfo);
}
HRESULT CMyDirect3DDevice9::DrawTriPatch(UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::DrawTriPatch");
  return gcpRendD3D->m_pActuald3dDevice->DrawTriPatch(Handle,pNumSegs,pTriPatchInfo);
}
HRESULT CMyDirect3DDevice9::DeletePatch(UINT Handle)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::DeletePatch");
  return gcpRendD3D->m_pActuald3dDevice->DeletePatch(Handle);
}
#endif
HRESULT CMyDirect3DDevice9::CreateQuery(D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::CreateQuery");
  return gcpRendD3D->m_pActuald3dDevice->CreateQuery(Type, ppQuery);
}

//==============================================================================================

//void glSetLogFuncs(bool bSet);
#elif defined (DIRECT3D10)

static char *sD3DFMT (DXGI_FORMAT Value)
{
  switch (Value)
  {
    ecase(DXGI_FORMAT_R32G32B32A32_TYPELESS);
    ecase(DXGI_FORMAT_R32G32B32A32_FLOAT);
    ecase(DXGI_FORMAT_R32G32B32A32_UINT);
    ecase(DXGI_FORMAT_R32G32B32A32_SINT);
    ecase(DXGI_FORMAT_R32G32B32_TYPELESS);
    ecase(DXGI_FORMAT_R32G32B32_FLOAT);
    ecase(DXGI_FORMAT_R32G32B32_UINT);
    ecase(DXGI_FORMAT_R32G32B32_SINT);
    ecase(DXGI_FORMAT_R16G16B16A16_TYPELESS);
    ecase(DXGI_FORMAT_R16G16B16A16_FLOAT);
    ecase(DXGI_FORMAT_R16G16B16A16_UNORM);
    ecase(DXGI_FORMAT_R16G16B16A16_UINT);
    ecase(DXGI_FORMAT_R16G16B16A16_SNORM);
    ecase(DXGI_FORMAT_R16G16B16A16_SINT);
    ecase(DXGI_FORMAT_R32G32_TYPELESS);
    ecase(DXGI_FORMAT_R32G32_FLOAT);
    ecase(DXGI_FORMAT_R32G32_UINT);
    ecase(DXGI_FORMAT_R32G32_SINT);
    ecase(DXGI_FORMAT_R32G8X24_TYPELESS);
    ecase(DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
    ecase(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
    ecase(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
    ecase(DXGI_FORMAT_R10G10B10A2_TYPELESS);
    ecase(DXGI_FORMAT_R10G10B10A2_UNORM);
    ecase(DXGI_FORMAT_R10G10B10A2_UINT);
    ecase(DXGI_FORMAT_R11G11B10_FLOAT);
    ecase(DXGI_FORMAT_R8G8B8A8_TYPELESS);
    ecase(DXGI_FORMAT_R8G8B8A8_UNORM);
    ecase(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    ecase(DXGI_FORMAT_R8G8B8A8_UINT);
    ecase(DXGI_FORMAT_R8G8B8A8_SNORM);
    ecase(DXGI_FORMAT_R8G8B8A8_SINT);
    ecase(DXGI_FORMAT_R16G16_TYPELESS);
    ecase(DXGI_FORMAT_R16G16_FLOAT);
    ecase(DXGI_FORMAT_R16G16_UNORM);
    ecase(DXGI_FORMAT_R16G16_UINT);
    ecase(DXGI_FORMAT_R16G16_SNORM);
    ecase(DXGI_FORMAT_R16G16_SINT);
    ecase(DXGI_FORMAT_R32_TYPELESS);
    ecase(DXGI_FORMAT_D32_FLOAT);
    ecase(DXGI_FORMAT_R32_FLOAT);
    ecase(DXGI_FORMAT_R32_UINT);
    ecase(DXGI_FORMAT_R32_SINT);
    ecase(DXGI_FORMAT_R24G8_TYPELESS);
    ecase(DXGI_FORMAT_D24_UNORM_S8_UINT);
    ecase(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
    ecase(DXGI_FORMAT_X24_TYPELESS_G8_UINT);
    ecase(DXGI_FORMAT_R8G8_TYPELESS);
    ecase(DXGI_FORMAT_R8G8_UNORM);
    ecase(DXGI_FORMAT_R8G8_UINT);
    ecase(DXGI_FORMAT_R8G8_SNORM);
    ecase(DXGI_FORMAT_R8G8_SINT);
    ecase(DXGI_FORMAT_R16_TYPELESS);
    ecase(DXGI_FORMAT_R16_FLOAT);
    ecase(DXGI_FORMAT_D16_UNORM);
    ecase(DXGI_FORMAT_R16_UNORM);
    ecase(DXGI_FORMAT_R16_UINT);
    ecase(DXGI_FORMAT_R16_SNORM);
    ecase(DXGI_FORMAT_R16_SINT);
    ecase(DXGI_FORMAT_R8_TYPELESS);
    ecase(DXGI_FORMAT_R8_UNORM);
    ecase(DXGI_FORMAT_R8_UINT);
    ecase(DXGI_FORMAT_R8_SNORM);
    ecase(DXGI_FORMAT_R8_SINT);
    ecase(DXGI_FORMAT_A8_UNORM);
    ecase(DXGI_FORMAT_R1_UNORM);
    ecase(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
    ecase(DXGI_FORMAT_R8G8_B8G8_UNORM);
    ecase(DXGI_FORMAT_G8R8_G8B8_UNORM);
    ecase(DXGI_FORMAT_BC1_TYPELESS);
    ecase(DXGI_FORMAT_BC1_UNORM);
    ecase(DXGI_FORMAT_BC1_UNORM_SRGB);
    ecase(DXGI_FORMAT_BC2_TYPELESS);
    ecase(DXGI_FORMAT_BC2_UNORM);
    ecase(DXGI_FORMAT_BC2_UNORM_SRGB);
    ecase(DXGI_FORMAT_BC3_TYPELESS);
    ecase(DXGI_FORMAT_BC3_UNORM);
    ecase(DXGI_FORMAT_BC3_UNORM_SRGB);
    ecase(DXGI_FORMAT_BC4_TYPELESS);
    ecase(DXGI_FORMAT_BC4_UNORM);
    ecase(DXGI_FORMAT_BC4_SNORM);
    ecase(DXGI_FORMAT_BC5_TYPELESS);
    ecase(DXGI_FORMAT_BC5_UNORM);
    ecase(DXGI_FORMAT_BC5_SNORM);
    ecase(DXGI_FORMAT_B5G6R5_UNORM);
    ecase(DXGI_FORMAT_B5G5R5A1_UNORM);
    ecase(DXGI_FORMAT_B8G8R8A8_UNORM);
    ecase(DXGI_FORMAT_B8G8R8X8_UNORM);
    edefault (Value);
  }
}

static char *sD3DPRIM_TOPLOGY(D3D11_PRIMITIVE_TOPOLOGY Topology)
{
  switch (Topology)
  {
    ecase(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    ecase(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    ecase(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    ecase(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ecase(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ecase(D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
    ecase(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ);
    ecase(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);
    ecase(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ);
    edefault (Topology);
  }
}

static char *sD3DBlendOP (DWORD Value)
{
  switch (Value)
  {
    ecase (D3D11_BLEND_OP_ADD);
    ecase (D3D11_BLEND_OP_SUBTRACT);
    ecase (D3D11_BLEND_OP_REV_SUBTRACT);
    ecase (D3D11_BLEND_OP_MIN);
    ecase (D3D11_BLEND_OP_MAX);
    edefault ((uint32)Value);
  }
}

static char *sD3DBLEND (DWORD Value)
{
  switch (Value)
  {
    ecase (D3D11_BLEND_ZERO);
    ecase (D3D11_BLEND_ONE);
    ecase (D3D11_BLEND_SRC_COLOR);
    ecase (D3D11_BLEND_DEST_COLOR);
    ecase (D3D11_BLEND_SRC_ALPHA);
    ecase (D3D11_BLEND_SRC_ALPHA_SAT);
    ecase (D3D11_BLEND_DEST_ALPHA);
    ecase (D3D11_BLEND_INV_SRC_COLOR);
    ecase (D3D11_BLEND_INV_SRC_ALPHA);
    ecase (D3D11_BLEND_INV_DEST_ALPHA);
    ecase (D3D11_BLEND_INV_DEST_COLOR);
    ecase (D3D11_BLEND_BLEND_FACTOR);
    ecase (D3D11_BLEND_INV_BLEND_FACTOR);
    ecase (D3D11_BLEND_SRC1_COLOR);
    ecase (D3D11_BLEND_INV_SRC1_COLOR);
    ecase (D3D11_BLEND_SRC1_ALPHA);
    ecase (D3D11_BLEND_INV_SRC1_ALPHA);
    edefault ((uint32)Value);
  }
}

static char *sD3DCompareFunc (DWORD Value)
{
  switch (Value)
  {
    ecase (D3D11_COMPARISON_NEVER);
    ecase (D3D11_COMPARISON_LESS);
    ecase (D3D11_COMPARISON_EQUAL);
    ecase (D3D11_COMPARISON_LESS_EQUAL);
    ecase (D3D11_COMPARISON_GREATER);
    ecase (D3D11_COMPARISON_NOT_EQUAL);
    ecase (D3D11_COMPARISON_GREATER_EQUAL);
    ecase (D3D11_COMPARISON_ALWAYS);
    edefault ((uint32)Value);
  }
}

static char *sD3DStencilOp (DWORD Value)
{
  switch (Value)
  {
    ecase (D3D11_STENCIL_OP_KEEP);
    ecase (D3D11_STENCIL_OP_ZERO);
    ecase (D3D11_STENCIL_OP_REPLACE);
    ecase (D3D11_STENCIL_OP_INCR_SAT);
    ecase (D3D11_STENCIL_OP_DECR_SAT);
    ecase (D3D11_STENCIL_OP_INVERT);
    ecase (D3D11_STENCIL_OP_INCR);
    ecase (D3D11_STENCIL_OP_DECR);
    edefault ((uint32)Value);
  }
}


static char *sD3DCull (DWORD Value)
{
  switch (Value)
  {
    ecase (D3D11_CULL_BACK);
    ecase (D3D11_CULL_FRONT);
    ecase (D3D11_CULL_NONE);
    edefault ((uint32)Value);
  }
}

static char *sD3DTAddress (D3D11_TEXTURE_ADDRESS_MODE Value)
{
  switch (Value)
  {
    ecase (D3D11_TEXTURE_ADDRESS_CLAMP);
    ecase (D3D11_TEXTURE_ADDRESS_WRAP);
    ecase (D3D11_TEXTURE_ADDRESS_MIRROR);
    ecase (D3D11_TEXTURE_ADDRESS_BORDER);
    ecase (D3D11_TEXTURE_ADDRESS_MIRROR_ONCE);
    edefault (Value);
  }
}

static char *sD3DTFilter (D3D11_FILTER Value)
{
  switch (Value)
  {
    ecase (D3D11_FILTER_MIN_MAG_MIP_POINT);
    ecase (D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR);
    ecase (D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT);
    ecase (D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR);
    ecase (D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT);
    ecase (D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
    ecase (D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT);
    ecase (D3D11_FILTER_MIN_MAG_MIP_LINEAR);
    ecase (D3D11_FILTER_ANISOTROPIC);
    ecase (D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT);
    ecase (D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR);
    ecase (D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT);
    ecase (D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR);
    ecase (D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT);
    ecase (D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
    ecase (D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT);
    ecase (D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
    ecase (D3D11_FILTER_COMPARISON_ANISOTROPIC);
    edefault (Value);
  }
}

HRESULT CMyDirect3DDevice11::QueryInterface(REFIID riid, void** ppvObj)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::QueryInterface");
  return gcpRendD3D->m_pActuald3dDevice->QueryInterface(riid, ppvObj);
}
ULONG CMyDirect3DDevice11::AddRef()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::AddRef");
  return gcpRendD3D->m_pActuald3dDevice->AddRef();
}
ULONG CMyDirect3DDevice11::Release()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::Release");
  return gcpRendD3D->m_pActuald3dDevice->Release();
}

HRESULT CMyDirect3DDeviceContext11::QueryInterface(REFIID riid, void** ppvObj)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDeviceContext::QueryInterface");
	return gcpRendD3D->m_pActuald3dDeviceContext->QueryInterface(riid, ppvObj);
}
ULONG CMyDirect3DDeviceContext11::AddRef()
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDeviceContext::AddRef");
	return gcpRendD3D->m_pActuald3dDeviceContext->AddRef();
}
ULONG CMyDirect3DDeviceContext11::Release()
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDeviceContext::Release");
	return gcpRendD3D->m_pActuald3dDeviceContext->Release();
}
void CMyDirect3DDeviceContext11::VSSetConstantBuffers( 
  /* [in] */ UINT StartConstantSlot,
  /* [in] */ UINT NumBuffers,
  /* [size_is][in] */ ID3D11Buffer *const *ppConstantBuffers)
{
  int nBytes = 0;
  if (ppConstantBuffers && *ppConstantBuffers)
  {
    D3D11_BUFFER_DESC Desc;
    (*ppConstantBuffers)->GetDesc(&Desc);
    nBytes = Desc.ByteWidth;
  }

  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x) (%d bytes)\n", "D3DDeviceContext::VSSetConstantBuffers", StartConstantSlot, NumBuffers, *ppConstantBuffers, nBytes);

  /*
  void *pConstants = NULL;
  (*ppConstantBuffers)->Map(D3D11_MAP_READ, 0 , &pConstants);
  if (pConstants!=NULL)
  {
  for(int i=0; i<(nBytes/4); i++)
  {
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel, ".5f", ((float*)pConstants)[i]);
  }
  }
  (*ppConstantBuffers)->Unmap();
  */

  gcpRendD3D->m_pActuald3dDeviceContext->VSSetConstantBuffers(StartConstantSlot, NumBuffers, ppConstantBuffers);
}

void CMyDirect3DDeviceContext11::PSSetShaderResources( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumViews,
  /* [size_is][in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::PSSetShaderResources", Offset, NumViews, *ppShaderResourceViews);
  gcpRendD3D->m_pActuald3dDeviceContext->PSSetShaderResources(Offset, NumViews, ppShaderResourceViews);
}

void CMyDirect3DDeviceContext11::PSSetShader( 
	/* [in] */ ID3D11PixelShader *pPixelShader,
	/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
	/* [in] */ UINT NumClassInstances)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::PSSetShader", pPixelShader);
  gcpRendD3D->m_pActuald3dDeviceContext->PSSetShader(pPixelShader, ppClassInstances, NumClassInstances);
}

void CMyDirect3DDeviceContext11::PSSetSamplers( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumSamplers,
  /* [size_is][in] */ ID3D11SamplerState *const *ppSamplers)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::PSSetSamplers", Offset, NumSamplers, *ppSamplers);
  D3D11_SAMPLER_DESC Desc;
  if (ppSamplers && *ppSamplers)
  {
    (*ppSamplers)->GetDesc(&Desc);
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  AddressU: %s\n", sD3DTAddress(Desc.AddressU));
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  AddressV: %s\n", sD3DTAddress(Desc.AddressV));
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  AddressW: %s\n", sD3DTAddress(Desc.AddressW));
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  Filter: %s\n", sD3DTFilter(Desc.Filter));
    if (Desc.Filter == D3D11_FILTER_ANISOTROPIC || Desc.Filter == D3D11_FILTER_COMPARISON_ANISOTROPIC)
      gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  MaxAnisotropy: %d\n", Desc.MaxAnisotropy);
  }

  gcpRendD3D->m_pActuald3dDeviceContext->PSSetSamplers(Offset, NumSamplers, ppSamplers);
}

void CMyDirect3DDeviceContext11::VSSetShader( 
	/* [in] */ ID3D11VertexShader *pVertexShader,
	/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
	/* [in] */ UINT NumClassInstances)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::VSSetShader", pVertexShader);
  gcpRendD3D->m_pActuald3dDeviceContext->VSSetShader(pVertexShader, ppClassInstances, NumClassInstances);
}

void CMyDirect3DDeviceContext11::DrawIndexed( 
  /* [in] */ UINT IndexCount,
  /* [in] */ UINT StartIndexLocation,
  /* [in] */ INT BaseVertexLocation)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, %d)\n", "D3DDeviceContext::DrawIndexed", IndexCount, StartIndexLocation, BaseVertexLocation);
  gcpRendD3D->m_pActuald3dDeviceContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}


void CMyDirect3DDeviceContext11::Draw( 
                                    /* [in] */ UINT VertexCount,
                                    /* [in] */ UINT StartVertexLocation)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::Draw", VertexCount, StartVertexLocation);
  gcpRendD3D->m_pActuald3dDeviceContext->Draw(VertexCount, StartVertexLocation);
}


void CMyDirect3DDeviceContext11::PSSetConstantBuffers( 
  /* [in] */ UINT StartConstantSlot,
  /* [in] */ UINT NumBuffers,
  /* [size_is][in] */ ID3D11Buffer *const *ppConstantBuffers)
{
  int nBytes = 0;
  if (ppConstantBuffers && *ppConstantBuffers)
  {
    D3D11_BUFFER_DESC Desc;
    (*ppConstantBuffers)->GetDesc(&Desc);
    nBytes = Desc.ByteWidth;
  }
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x) (%d bytes)\n", "D3DDeviceContext::PSSetConstantBuffers", StartConstantSlot, NumBuffers, *ppConstantBuffers, nBytes);
  gcpRendD3D->m_pActuald3dDeviceContext->PSSetConstantBuffers(StartConstantSlot, NumBuffers, ppConstantBuffers);
}

void CMyDirect3DDeviceContext11::IASetInputLayout( 
  /* [in] */ ID3D11InputLayout *pInputLayout)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::IASetInputLayout", pInputLayout);
  gcpRendD3D->m_pActuald3dDeviceContext->IASetInputLayout(pInputLayout);
}

void CMyDirect3DDeviceContext11::IASetVertexBuffers( 
  /* [in] */ UINT StartSlot,
  /* [in] */ UINT NumBuffers,
  /* [size_is][in] */ ID3D11Buffer *const *ppVertexBuffers,
  /* [size_is][in] */ const UINT *pStrides,
  /* [size_is][in] */ const UINT *pOffsets)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x, %d, %d)\n", "D3DDeviceContext::IASetVertexBuffers", StartSlot, NumBuffers, *ppVertexBuffers, *pStrides, *pOffsets);
  gcpRendD3D->m_pActuald3dDeviceContext->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}

void CMyDirect3DDeviceContext11::IASetIndexBuffer( 
  /* [in] */ ID3D11Buffer *pIndexBuffer,
  /* [in] */ DXGI_FORMAT Format,
  /* [in] */ UINT Offset)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %s, %d)\n", "D3DDeviceContext::IASetIndexBuffer", pIndexBuffer, sD3DFMT(Format), Offset);
  gcpRendD3D->m_pActuald3dDeviceContext->IASetIndexBuffer(pIndexBuffer, Format, Offset);
}

void CMyDirect3DDeviceContext11::DrawIndexedInstanced( 
  /* [in] */ UINT IndexCountPerInstance,
  /* [in] */ UINT InstanceCount,
  /* [in] */ UINT StartIndexLocation,
  /* [in] */ INT BaseVertexLocation,
  /* [in] */ UINT StartInstanceLocation)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, %d, %d, %d)\n", "D3DDeviceContext::DrawIndexedInstanced", IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
  gcpRendD3D->m_pActuald3dDeviceContext->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void CMyDirect3DDeviceContext11::DrawInstanced( 
  /* [in] */ UINT VertexCountPerInstance,
  /* [in] */ UINT InstanceCount,
  /* [in] */ UINT StartVertexLocation,
  /* [in] */ UINT StartInstanceLocation)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, %d, %d)\n", "D3DDeviceContext::DrawInstanced", VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
  gcpRendD3D->m_pActuald3dDeviceContext->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void CMyDirect3DDeviceContext11::GSSetConstantBuffers( 
  /* [in] */ UINT StartConstantSlot,
  /* [in] */ UINT NumBuffers,
  /* [size_is][in] */ ID3D11Buffer *const *ppConstantBuffers)
{
  int nBytes = 0;
  if (ppConstantBuffers && *ppConstantBuffers)
  {
    D3D11_BUFFER_DESC Desc;
    (*ppConstantBuffers)->GetDesc(&Desc);
    nBytes = Desc.ByteWidth;
  }
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x) (%d bytes)\n", "D3DDeviceContext::GSSetConstantBuffers", StartConstantSlot, NumBuffers, *ppConstantBuffers, nBytes);
  gcpRendD3D->m_pActuald3dDeviceContext->GSSetConstantBuffers(StartConstantSlot, NumBuffers, ppConstantBuffers);
}

void CMyDirect3DDeviceContext11::GSSetShader( 
	/* [in] */ ID3D11GeometryShader *pShader,
	/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
	/* [in] */ UINT NumClassInstances)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::GSSetShader", pShader);
  gcpRendD3D->m_pActuald3dDeviceContext->GSSetShader(pShader, ppClassInstances, NumClassInstances);
}

void CMyDirect3DDeviceContext11::IASetPrimitiveTopology( 
  /* [in] */ D3D11_PRIMITIVE_TOPOLOGY Topology)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%s)\n", "D3DDeviceContext::IASetPrimitiveTopology", sD3DPRIM_TOPLOGY(Topology));
  gcpRendD3D->m_pActuald3dDeviceContext->IASetPrimitiveTopology(Topology);
}

void CMyDirect3DDeviceContext11::VSSetShaderResources( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumViews,
  /* [size_is][in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::VSSetShaderResources", Offset, NumViews, *ppShaderResourceViews);
  gcpRendD3D->m_pActuald3dDeviceContext->VSSetShaderResources(Offset, NumViews, ppShaderResourceViews);
}

void CMyDirect3DDeviceContext11::VSSetSamplers( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumSamplers,
  /* [size_is][in] */ ID3D11SamplerState *const *ppSamplers)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::VSSetSamplers", Offset, NumSamplers, *ppSamplers);
  gcpRendD3D->m_pActuald3dDeviceContext->VSSetSamplers(Offset, NumSamplers, ppSamplers);
}

void CMyDirect3DDeviceContext11::SetPredication( 
  /* [in] */ ID3D11Predicate *pPredicate,
  /* [in] */ BOOL PredicateValue)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d)\n", "D3DDeviceContext::SetPredication", pPredicate, PredicateValue);
  gcpRendD3D->m_pActuald3dDeviceContext->SetPredication(pPredicate, PredicateValue);
}

void CMyDirect3DDeviceContext11::GSSetShaderResources( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumViews,
  /* [size_is][in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::GSSetShaderResources", Offset, NumViews, *ppShaderResourceViews);
  gcpRendD3D->m_pActuald3dDeviceContext->GSSetShaderResources(Offset, NumViews, ppShaderResourceViews);
}

void CMyDirect3DDeviceContext11::GSSetSamplers( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumSamplers,
  /* [size_is][in] */ ID3D11SamplerState *const *ppSamplers)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::GSSetSamplers", Offset, NumSamplers, *ppSamplers);
  gcpRendD3D->m_pActuald3dDeviceContext->GSSetSamplers(Offset, NumSamplers, ppSamplers);
}

void CMyDirect3DDeviceContext11::OMSetRenderTargets( 
  /* [in] */ UINT NumViews,
  /* [size_is][in] */ ID3D11RenderTargetView *const *ppRenderTargetViews,
  /* [in] */ ID3D11DepthStencilView *pDepthStencilView)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::OMSetRenderTargets", NumViews, *ppRenderTargetViews, pDepthStencilView);
  gcpRendD3D->m_pActuald3dDeviceContext->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
}

void CMyDirect3DDeviceContext11::OMSetBlendState( 
  /* [in] */ ID3D11BlendState *pBlendState,
  /* [in] */ const FLOAT BlendFactor[ 4 ],
  /* [in] */ UINT SampleMask)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::OMSetBlendState", pBlendState);
  D3D11_BLEND_DESC Desc;
  pBlendState->GetDesc(&Desc);

  if (!Desc.RenderTarget[0].BlendEnable)
     gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  Blending: Disabled\n");
  else
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  Blending: (%s, %s, BlendOp:%s, BlendOpAlpha:%s)\n", sD3DBLEND(Desc.RenderTarget[0].SrcBlend), sD3DBLEND(Desc.RenderTarget[0].DestBlend), sD3DBlendOP(Desc.RenderTarget[0].BlendOp), sD3DBlendOP(Desc.RenderTarget[0].BlendOpAlpha));

  for (int i=1;i<8;i++)
  {
    if (Desc.RenderTarget[i].BlendEnable)
			gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "RT:  Blending %d: (%s, %s, BlendOp:%s, BlendOpAlpha:%s)\n", i, sD3DBLEND(Desc.RenderTarget[0].SrcBlend), sD3DBLEND(Desc.RenderTarget[0].DestBlend), sD3DBlendOP(Desc.RenderTarget[0].BlendOp),  sD3DBlendOP(Desc.RenderTarget[0].BlendOpAlpha));
  }

  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  WriteMask: %d\n", Desc.RenderTarget[0].RenderTargetWriteMask);
  gcpRendD3D->m_pActuald3dDeviceContext->OMSetBlendState(pBlendState, BlendFactor, SampleMask);
}

void CMyDirect3DDeviceContext11::OMSetDepthStencilState( 
  /* [in] */ ID3D11DepthStencilState *pDepthStencilState,
  /* [in] */ UINT StencilRef)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::OMSetDepthStencilState", pDepthStencilState);
  D3D11_DEPTH_STENCIL_DESC Desc;
  pDepthStencilState->GetDesc(&Desc);
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  DepthTest: %s\n", Desc.DepthEnable ? "TRUE" : "FALSE");
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  DepthWrite: %s\n", Desc.DepthWriteMask ? "TRUE" : "FALSE");
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  DepthFunc: %s\n", sD3DCompareFunc(Desc.DepthFunc));

  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  StencilEnable: %s\n", Desc.StencilEnable ? "TRUE" : "FALSE");

  if (Desc.StencilEnable)
  {
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  StencilRef: 0x%x\n", StencilRef );
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  StencilReadMask: 0x%x\n", Desc.StencilReadMask);
    gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  StencilWriteMask: 0x%x\n", Desc.StencilWriteMask );


    gcpRendD3D->Logv( SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  FrontFace Stencil: \n");
    gcpRendD3D->Logv( SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "    StencilFailOp: %s\n", sD3DStencilOp(Desc.FrontFace.StencilFailOp) );
    gcpRendD3D->Logv( SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "    StencilDepthFailOp: %s\n", sD3DStencilOp(Desc.FrontFace.StencilDepthFailOp) );
    gcpRendD3D->Logv( SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "    StencilPassOp: %s\n", sD3DStencilOp(Desc.FrontFace.StencilPassOp) );
    gcpRendD3D->Logv( SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "    StencilFunc: %s\n", sD3DCompareFunc(Desc.FrontFace.StencilFunc) );

    gcpRendD3D->Logv( SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "  BackFace Stencil: \n");
    gcpRendD3D->Logv( SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "    StencilFailOp: %s\n", sD3DStencilOp(Desc.BackFace.StencilFailOp) );
    gcpRendD3D->Logv( SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "    StencilDepthFailOp: %s\n", sD3DStencilOp(Desc.BackFace.StencilDepthFailOp) );
    gcpRendD3D->Logv( SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "    StencilPassOp: %s\n", sD3DStencilOp(Desc.BackFace.StencilPassOp) );
    gcpRendD3D->Logv( SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "    StencilFunc: %s\n", sD3DCompareFunc(Desc.BackFace.StencilFunc) );
  }

  gcpRendD3D->m_pActuald3dDeviceContext->OMSetDepthStencilState(pDepthStencilState, StencilRef);
}

void CMyDirect3DDeviceContext11::SOSetTargets( 
  /* [in] */ UINT NumBuffers,
  /* [size_is][in] */ ID3D11Buffer *const *ppSOTargets,
  /* [size_is][in] */ const UINT *pOffsets)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, 0x%x, %d)\n", "D3DDeviceContext::SOSetTargets", NumBuffers, *ppSOTargets, *pOffsets);
  gcpRendD3D->m_pActuald3dDeviceContext->SOSetTargets(NumBuffers, ppSOTargets, (UINT*)pOffsets);
}

void CMyDirect3DDeviceContext11::DrawAuto( void)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::DrawAuto");
  gcpRendD3D->m_pActuald3dDeviceContext->DrawAuto();
}

void CMyDirect3DDeviceContext11::RSSetState( 
  /* [in] */ ID3D11RasterizerState *pRasterizerState)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::RSSetState", pRasterizerState);
  D3D11_RASTERIZER_DESC Desc;
  pRasterizerState->GetDesc(&Desc);
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "   CullMode: %s\n", sD3DCull(Desc.CullMode));
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "   Scissor: %s\n", Desc.ScissorEnable ? "TRUE" : "FALSE");
  gcpRendD3D->m_pActuald3dDeviceContext->RSSetState(pRasterizerState);
}

void CMyDirect3DDeviceContext11::RSSetViewports( 
  /* [in] */ UINT NumViewports,
  /* [size_is][in] */ const D3D11_VIEWPORT *pViewports)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, 0x%x) (%d, %d, %d, %d [%.2f, %.2f])\n", "D3DDeviceContext::RSSetViewports", NumViewports, pViewports, pViewports->TopLeftX, pViewports->TopLeftY, pViewports->Width, pViewports->Height, pViewports->MinDepth, pViewports->MaxDepth);
  gcpRendD3D->m_pActuald3dDeviceContext->RSSetViewports(NumViewports, pViewports);
}

void CMyDirect3DDeviceContext11::RSSetScissorRects( 
  /* [in] */ UINT NumRects,
  /* [size_is][in] */ const D3D11_RECT *pRects)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, 0x%x)\n", "D3DDeviceContext::RSSetScissorRects", NumRects, pRects);
  gcpRendD3D->m_pActuald3dDeviceContext->RSSetScissorRects(NumRects, pRects);
}

void CMyDirect3DDeviceContext11::ClearRenderTargetView( 
  /* [in] */ ID3D11RenderTargetView *pRenderTargetView,
  /* [in] */ const FLOAT ColorRGBA[ 4 ])
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x (%f, %f, %f, %f))\n", "D3DDeviceContext::ClearRenderTargetView", pRenderTargetView, ColorRGBA[0], ColorRGBA[1], ColorRGBA[2], ColorRGBA[3]);
  gcpRendD3D->m_pActuald3dDeviceContext->ClearRenderTargetView(pRenderTargetView, ColorRGBA);
}

void CMyDirect3DDeviceContext11::ClearDepthStencilView( 
  /* [in] */ ID3D11DepthStencilView *pDepthStencilView,
  /* [in] */ UINT Flags,
  /* [in] */ FLOAT Depth,
  /* [in] */ UINT8 Stencil)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x, %f, %d)\n", "D3DDeviceContext::ClearDepthStencilView", pDepthStencilView, Flags, Depth, Stencil);
  gcpRendD3D->m_pActuald3dDeviceContext->ClearDepthStencilView(pDepthStencilView, Flags, Depth, Stencil);
}

void CMyDirect3DDeviceContext11::GenerateMips( 
  /* [in] */ ID3D11ShaderResourceView *pShaderResourceView)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::GenerateMips", pShaderResourceView);
  gcpRendD3D->m_pActuald3dDeviceContext->GenerateMips(pShaderResourceView);
}

void CMyDirect3DDeviceContext11::VSGetConstantBuffers( 
  /* [in] */ UINT StartConstantSlot,
  /* [in] */ UINT NumBuffers,
  /* [size_is][out] */ ID3D11Buffer **ppConstantBuffers)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::VSGetConstantBuffers", StartConstantSlot, NumBuffers);
  gcpRendD3D->m_pActuald3dDeviceContext->VSGetConstantBuffers(StartConstantSlot, NumBuffers, ppConstantBuffers);
}

void CMyDirect3DDeviceContext11::PSGetShaderResources( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumViews,
  /* [size_is][out] */ ID3D11ShaderResourceView **ppShaderResourceViews)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::PSGetShaderResources", Offset, NumViews);
  gcpRendD3D->m_pActuald3dDeviceContext->PSGetShaderResources(Offset, NumViews, ppShaderResourceViews);
}

void CMyDirect3DDeviceContext11::PSGetShader( 
	/* [out][in] */ ID3D11PixelShader **ppPixelShader,
	/* [in] */ ID3D11ClassInstance **ppClassInstances,
	/* [in] */ UINT* pNumClassInstances)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::PSGetShader");
  gcpRendD3D->m_pActuald3dDeviceContext->PSGetShader(ppPixelShader, ppClassInstances, pNumClassInstances);
}

void CMyDirect3DDeviceContext11::PSGetSamplers( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumSamplers,
  /* [size_is][out] */ ID3D11SamplerState **ppSamplers)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::PSGetSamplers", Offset, NumSamplers);
  gcpRendD3D->m_pActuald3dDeviceContext->PSGetSamplers(Offset, NumSamplers, ppSamplers);
}

void CMyDirect3DDeviceContext11::VSGetShader( 
	/* [out][in] */ ID3D11VertexShader **ppVertexShader,
	/* [in] */ ID3D11ClassInstance **ppClassInstances,
	/* [in] */ UINT* pNumClassInstances)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::VSGetShader");
  gcpRendD3D->m_pActuald3dDeviceContext->VSGetShader(ppVertexShader, ppClassInstances, pNumClassInstances);
}

void CMyDirect3DDeviceContext11::PSGetConstantBuffers( 
  /* [in] */ UINT StartConstantSlot,
  /* [in] */ UINT NumBuffers,
  /* [size_is][out] */ ID3D11Buffer **ppConstantBuffers)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::PSGetConstantBuffers", StartConstantSlot, NumBuffers);
  gcpRendD3D->m_pActuald3dDeviceContext->PSGetConstantBuffers(StartConstantSlot, NumBuffers, ppConstantBuffers);
}

void CMyDirect3DDeviceContext11::IAGetInputLayout( 
  /* [out][in] */ ID3D11InputLayout **ppInputLayout)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::IAGetInputLayout");
  gcpRendD3D->m_pActuald3dDeviceContext->IAGetInputLayout(ppInputLayout);
}

void CMyDirect3DDeviceContext11::IAGetVertexBuffers( 
  /* [in] */ UINT StartSlot,
  /* [in] */ UINT NumBuffers,
  /* [size_is][out] */ ID3D11Buffer **ppVertexBuffers,
  /* [size_is][out] */ UINT *pStrides,
  /* [size_is][out] */ UINT *pOffsets)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::IAGetVertexBuffers", StartSlot, NumBuffers);
  gcpRendD3D->m_pActuald3dDeviceContext->IAGetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}

void CMyDirect3DDeviceContext11::IAGetIndexBuffer( 
  /* [out] */ ID3D11Buffer **pIndexBuffer,
  /* [out] */ DXGI_FORMAT *Format,
  /* [out] */ UINT *Offset)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::IAGetIndexBuffer");
  gcpRendD3D->m_pActuald3dDeviceContext->IAGetIndexBuffer(pIndexBuffer, Format, Offset);
}

void CMyDirect3DDeviceContext11::GSGetConstantBuffers( 
  /* [in] */ UINT StartConstantSlot,
  /* [in] */ UINT NumBuffers,
  /* [size_is][out] */ ID3D11Buffer **ppConstantBuffers)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::GSGetConstantBuffers", StartConstantSlot, NumBuffers);
  gcpRendD3D->m_pActuald3dDeviceContext->GSGetConstantBuffers(StartConstantSlot, NumBuffers, ppConstantBuffers);
}

void CMyDirect3DDeviceContext11::GSGetShader( 
	/* [out] */ ID3D11GeometryShader **ppGeometryShader,
	/* [in] */ ID3D11ClassInstance **ppClassInstances,
	/* [in] */ UINT* pNumClassInstances)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::GSGetShader");
  gcpRendD3D->m_pActuald3dDeviceContext->GSGetShader(ppGeometryShader, ppClassInstances, pNumClassInstances);
}

void CMyDirect3DDeviceContext11::IAGetPrimitiveTopology( 
  /* [out] */ D3D11_PRIMITIVE_TOPOLOGY *pTopology)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::IAGetPrimitiveTopology");
  gcpRendD3D->m_pActuald3dDeviceContext->IAGetPrimitiveTopology(pTopology);
}

void CMyDirect3DDeviceContext11::VSGetShaderResources( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumViews,
  /* [size_is][out] */ ID3D11ShaderResourceView **ppShaderResourceViews)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::VSGetShaderResources", Offset, NumViews);
  gcpRendD3D->m_pActuald3dDeviceContext->VSGetShaderResources(Offset, NumViews, ppShaderResourceViews);
}

void CMyDirect3DDeviceContext11::VSGetSamplers( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumSamplers,
  /* [size_is][out] */ ID3D11SamplerState **ppSamplers)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::VSGetSamplers", Offset, NumSamplers);
  gcpRendD3D->m_pActuald3dDeviceContext->VSGetSamplers(Offset, NumSamplers, ppSamplers);
}

void CMyDirect3DDeviceContext11::GetPredication( 
  /* [out] */ ID3D11Predicate **ppPredicate,
  /* [out] */ BOOL *pPredicateValue)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::GetPredication");
  gcpRendD3D->m_pActuald3dDeviceContext->GetPredication(ppPredicate, pPredicateValue);
}

void CMyDirect3DDeviceContext11::GSGetShaderResources( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumViews,
  /* [size_is][out] */ ID3D11ShaderResourceView **ppShaderResourceViews)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::GSGetShaderResources", Offset, NumViews);
  gcpRendD3D->m_pActuald3dDeviceContext->GSGetShaderResources(Offset, NumViews, ppShaderResourceViews);
}

void CMyDirect3DDeviceContext11::GSGetSamplers( 
  /* [in] */ UINT Offset,
  /* [in] */ UINT NumSamplers,
  /* [size_is][out] */ ID3D11SamplerState **ppSamplers)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d)\n", "D3DDeviceContext::GSGetSamplers", Offset, NumSamplers);
  gcpRendD3D->m_pActuald3dDeviceContext->GSGetSamplers(Offset, NumSamplers, ppSamplers);
}

void CMyDirect3DDeviceContext11::OMGetRenderTargets( 
  /* [in] */ UINT NumViews,
  /* [size_is][out] */ ID3D11RenderTargetView **ppRenderTargetViews,
  /* [out] */ ID3D11DepthStencilView **ppDepthStencilView)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d)\n", "D3DDeviceContext::OMGetRenderTargets", NumViews);
  gcpRendD3D->m_pActuald3dDeviceContext->OMGetRenderTargets(NumViews, ppRenderTargetViews, ppDepthStencilView);
}

void CMyDirect3DDeviceContext11::OMGetBlendState( 
  /* [out] */ ID3D11BlendState **ppBlendState,
  /* [out] */ FLOAT BlendFactor[ 4 ],
  /* [out] */ UINT *pSampleMask)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::OMGetBlendState");
  gcpRendD3D->m_pActuald3dDeviceContext->OMGetBlendState(ppBlendState, BlendFactor, pSampleMask);
}

void CMyDirect3DDeviceContext11::OMGetDepthStencilState( 
  /* [out] */ ID3D11DepthStencilState **ppDepthStencilState,
  /* [out] */ UINT *pStencilRef)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::OMGetDepthStencilState");
  gcpRendD3D->m_pActuald3dDeviceContext->OMGetDepthStencilState(ppDepthStencilState, pStencilRef);
}

void CMyDirect3DDeviceContext11::SOGetTargets( 
  /* [in] */ UINT NumBuffers,
  /* [size_is][out] */ ID3D11Buffer **ppSOTargets)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d)\n", "D3DDeviceContext::SOGetTargets", NumBuffers);
  gcpRendD3D->m_pActuald3dDeviceContext->SOGetTargets(NumBuffers, ppSOTargets);
}

void CMyDirect3DDeviceContext11::RSGetState( 
  /* [out] */ ID3D11RasterizerState **ppRasterizerState)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::RSGetState");
  gcpRendD3D->m_pActuald3dDeviceContext->RSGetState(ppRasterizerState);
}

void CMyDirect3DDeviceContext11::RSGetViewports( 
  /* [out][in] */ UINT *NumViewports,
  /* [size_is][out] */ D3D11_VIEWPORT *pViewports)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::RSGetViewports");
  gcpRendD3D->m_pActuald3dDeviceContext->RSGetViewports(NumViewports, pViewports);
}

void CMyDirect3DDeviceContext11::RSGetScissorRects( 
  /* [out][in] */ UINT *NumRects,
  /* [size_is][out] */ D3D11_RECT *pRects)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::RSGetScissorRects");
  gcpRendD3D->m_pActuald3dDeviceContext->RSGetScissorRects(NumRects, pRects);
}

void CMyDirect3DDeviceContext11::Flush( void)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::Flush");
	gcpRendD3D->m_pActuald3dDeviceContext->Flush();
}

HRESULT CMyDirect3DDevice11::GetDeviceRemovedReason( void)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDevice::GetDeviceRemovedReason");
  return gcpRendD3D->m_pActuald3dDevice->GetDeviceRemovedReason();
}

HRESULT CMyDirect3DDevice11::SetExceptionMode( 
  UINT RaiseFlags)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::SetExceptionMode", RaiseFlags);
  return gcpRendD3D->m_pActuald3dDevice->SetExceptionMode(RaiseFlags);
}

UINT CMyDirect3DDevice11::GetExceptionMode( void)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDevice::GetExceptionMode");
  return gcpRendD3D->m_pActuald3dDevice->GetExceptionMode();
}

HRESULT CMyDirect3DDevice11::GetPrivateData( 
  /* [in] */ REFGUID guid,
  /* [out][in] */ UINT *pDataSize,
  /* [size_is][out] */ void *pData)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDevice::GetPrivateData");
  return gcpRendD3D->m_pActuald3dDevice->GetPrivateData(guid, pDataSize, pData);
}

HRESULT CMyDirect3DDevice11::SetPrivateData( 
  /* [in] */ REFGUID guid,
  /* [in] */ UINT DataSize,
  /* [size_is][in] */ const void *pData)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDevice::SetPrivateData");
  return gcpRendD3D->m_pActuald3dDevice->SetPrivateData(guid, DataSize, pData);
}

HRESULT CMyDirect3DDevice11::SetPrivateDataInterface( 
  /* [in] */ REFGUID guid,
  /* [in] */ const IUnknown *pData)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDevice::SetPrivateDataInterface");
  return gcpRendD3D->m_pActuald3dDevice->SetPrivateDataInterface(guid, pData);
}

HRESULT CMyDirect3DDevice11::CreateBuffer( 
  /* [in] */ const D3D11_BUFFER_DESC *pDesc,
  /* [in] */ const D3D11_SUBRESOURCE_DATA *pInitialData,
  /* [out] */ ID3D11Buffer **ppBuffer)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x)\n", "D3DDevice::CreateBuffer", pDesc, pInitialData);
  return gcpRendD3D->m_pActuald3dDevice->CreateBuffer(pDesc, pInitialData, ppBuffer);
}

HRESULT CMyDirect3DDevice11::CreateTexture1D( 
  /* [in] */ const D3D11_TEXTURE1D_DESC *pDesc,
  /* [in] */ const D3D11_SUBRESOURCE_DATA *pInitialData,
  /* [out] */ ID3D11Texture1D **ppTexture1D)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x)\n", "D3DDevice::CreateTexture1D", pDesc, pInitialData);
  return gcpRendD3D->m_pActuald3dDevice->CreateTexture1D(pDesc, pInitialData, ppTexture1D);
}

HRESULT CMyDirect3DDevice11::CreateTexture2D( 
  /* [in] */ const D3D11_TEXTURE2D_DESC *pDesc,
  /* [in] */ const D3D11_SUBRESOURCE_DATA *pInitialData,
  /* [out] */ ID3D11Texture2D **ppTexture2D)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x)\n", "D3DDevice::CreateTexture2D", pDesc, pInitialData);
  return gcpRendD3D->m_pActuald3dDevice->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
}

HRESULT CMyDirect3DDevice11::CreateTexture3D( 
  /* [in] */ const D3D11_TEXTURE3D_DESC *pDesc,
  /* [in] */ const D3D11_SUBRESOURCE_DATA *pInitialData,
  /* [out] */ ID3D11Texture3D **ppTexture3D)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x)\n", "D3DDevice::CreateTexture3D", pDesc, pInitialData);
  return gcpRendD3D->m_pActuald3dDevice->CreateTexture3D(pDesc, pInitialData, ppTexture3D);
}

HRESULT CMyDirect3DDevice11::CreateShaderResourceView( 
  /* [in] */ ID3D11Resource *pResource,
  /* [in] */ const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
  /* [out] */ ID3D11ShaderResourceView **ppSRView)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x)\n", "D3DDevice::CreateShaderResourceView", pResource, pDesc);
  return gcpRendD3D->m_pActuald3dDevice->CreateShaderResourceView(pResource, pDesc, ppSRView);
}

HRESULT CMyDirect3DDevice11::CreateRenderTargetView( 
  /* [in] */ ID3D11Resource *pResource,
  /* [in] */ const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
  /* [out] */ ID3D11RenderTargetView **ppRTView)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x)\n", "D3DDevice::CreateRenderTargetView", pResource, pDesc);
  return gcpRendD3D->m_pActuald3dDevice->CreateRenderTargetView(pResource, pDesc, ppRTView);
}

HRESULT CMyDirect3DDevice11::CreateDepthStencilView( 
  /* [in] */ ID3D11Resource *pResource,
  /* [in] */ const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
  /* [out] */ ID3D11DepthStencilView **ppDepthStencilView)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x)\n", "D3DDevice::CreateDepthStencilView", pResource, pDesc);
  return gcpRendD3D->m_pActuald3dDevice->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView);
}

HRESULT CMyDirect3DDevice11::CreateInputLayout( 
  /* [size_is][in] */ const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
  /* [in] */ UINT NumElements,
  /* [in] */ const void *pShaderBytecodeWithInputSignature,
	/* [in] */ SIZE_T BytecodeLength,
  /* [out] */ ID3D11InputLayout **ppInputLayout)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d, 0x%x, %d)\n", "D3DDevice::CreateInputLayout", pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength);
  return gcpRendD3D->m_pActuald3dDevice->CreateInputLayout(pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout);
}

HRESULT CMyDirect3DDevice11::CreateVertexShader( 
  /* [in] */ const void *pShaderBytecode,
	/* [in] */ SIZE_T BytecodeLength,
	/* [in] */ ID3D11ClassLinkage *pClassLinkage,
	/* [out] */ ID3D11VertexShader **ppVertexShader)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d)\n", "D3DDevice::CreateVertexShader", pShaderBytecode, BytecodeLength);
  return gcpRendD3D->m_pActuald3dDevice->CreateVertexShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
}

HRESULT CMyDirect3DDevice11::CreateGeometryShader( 
  /* [in] */ const void *pShaderBytecode,
	/* [in] */ SIZE_T BytecodeLength,
	/* [in] */ ID3D11ClassLinkage *pClassLinkage,
	/* [out] */ ID3D11GeometryShader **ppGeometryShader)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d)\n", "D3DDevice::CreateGeometryShader", pShaderBytecode, BytecodeLength);
  return gcpRendD3D->m_pActuald3dDevice->CreateGeometryShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppGeometryShader);
}

HRESULT CMyDirect3DDevice11::CreateGeometryShaderWithStreamOutput( 
	/* [in] */ const void *pShaderBytecode,
	/* [in] */ SIZE_T BytecodeLength,
	/* [size_is][in] */ const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
	/* [in] */ UINT NumEntries,
	/* [in] */ const UINT *pBufferStrides,
	/* [in] */ UINT NumStrides,
	/* [in] */ UINT RasterizedStream,
	/* [in] */ ID3D11ClassLinkage *pClassLinkage,
	/* [out] */ ID3D11GeometryShader **ppGeometryShader)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d, 0x%x, %d, %d)\n", "D3DDevice::CreateGeometryShader", pShaderBytecode, BytecodeLength, pSODeclaration, NumEntries, RasterizedStream);
  return gcpRendD3D->m_pActuald3dDevice->CreateGeometryShaderWithStreamOutput(pShaderBytecode, BytecodeLength, pSODeclaration, NumEntries, pBufferStrides, NumStrides, RasterizedStream, pClassLinkage, ppGeometryShader);
}

HRESULT CMyDirect3DDevice11::CreatePixelShader( 
  /* [in] */ const void *pShaderBytecode,
	/* [in] */ SIZE_T BytecodeLength,
	/* [in] */ ID3D11ClassLinkage *pClassLinkage,
	/* [out] */ ID3D11PixelShader **ppPixelShader)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d)\n", "D3DDevice::CreatePixelShader", pShaderBytecode, BytecodeLength);
  return gcpRendD3D->m_pActuald3dDevice->CreatePixelShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
}

HRESULT CMyDirect3DDevice11::CreateBlendState( 
  /* [in] */ const D3D11_BLEND_DESC *pBlendStateDesc,
  /* [out] */ ID3D11BlendState **ppBlendState)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateBlendState", pBlendStateDesc);
  return gcpRendD3D->m_pActuald3dDevice->CreateBlendState(pBlendStateDesc, ppBlendState);
}

HRESULT CMyDirect3DDevice11::CreateDepthStencilState( 
  /* [in] */ const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
  /* [out] */ ID3D11DepthStencilState **ppDepthStencilState)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateDepthStencilState", pDepthStencilDesc);
  return gcpRendD3D->m_pActuald3dDevice->CreateDepthStencilState(pDepthStencilDesc, ppDepthStencilState);
}

HRESULT CMyDirect3DDevice11::CreateRasterizerState( 
  /* [in] */ const D3D11_RASTERIZER_DESC *pRasterizerDesc,
  /* [out] */ ID3D11RasterizerState **ppRasterizerState)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateRasterizerState", pRasterizerDesc);
  return gcpRendD3D->m_pActuald3dDevice->CreateRasterizerState(pRasterizerDesc, ppRasterizerState);
}

HRESULT CMyDirect3DDevice11::CreateSamplerState( 
  /* [in] */ const D3D11_SAMPLER_DESC *pSamplerDesc,
  /* [out] */ ID3D11SamplerState **ppSamplerState)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateSamplerState", pSamplerDesc);
  return gcpRendD3D->m_pActuald3dDevice->CreateSamplerState(pSamplerDesc, ppSamplerState);
}

HRESULT CMyDirect3DDevice11::CreateQuery( 
  /* [in] */ const D3D11_QUERY_DESC *pQueryDesc,
  /* [out] */ ID3D11Query **ppQuery)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateQuery", pQueryDesc);
  return gcpRendD3D->m_pActuald3dDevice->CreateQuery(pQueryDesc, ppQuery);
}
#ifndef PS3
HRESULT CMyDirect3DDevice11::CreatePredicate( 
  /* [in] */ const D3D11_QUERY_DESC *pPredicateDesc,
  /* [out] */ ID3D11Predicate **ppPredicate)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreatePredicate", pPredicateDesc);
  return gcpRendD3D->m_pActuald3dDevice->CreatePredicate(pPredicateDesc, ppPredicate);
}

HRESULT CMyDirect3DDevice11::CreateUnorderedAccessView( 
														/* [in] */ ID3D11Resource *pResource, 
														/* [in] */ const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, 
														/* [out] */ ID3D11UnorderedAccessView **ppUAView )
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateUnorderedAccessView", pDesc);
	return gcpRendD3D->m_pActuald3dDevice->CreateUnorderedAccessView(pResource, pDesc, ppUAView);
}

HRESULT CMyDirect3DDevice11::CreateHullShader(
	/* [in] */ const void *pShaderBytecode,
	/* [in] */ SIZE_T BytecodeLength,
	/* [in] */ ID3D11ClassLinkage *pClassLinkage,
	/* [out] */ ID3D11HullShader **ppHullShader
	)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateHullShader", ppHullShader);
	return gcpRendD3D->m_pActuald3dDevice->CreateHullShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppHullShader);
}

HRESULT CMyDirect3DDevice11::CreateDomainShader(
	/* [in] */ const void *pShaderBytecode,
	/* [in] */ SIZE_T BytecodeLength,
	/* [in] */ ID3D11ClassLinkage *pClassLinkage,
	/* [out] */ ID3D11DomainShader **ppDomainShader
	)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateDomainShader", ppDomainShader);
	return gcpRendD3D->m_pActuald3dDevice->CreateDomainShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppDomainShader);
}

HRESULT CMyDirect3DDevice11::CreateComputeShader(
	/* [in] */ const void *pShaderBytecode,
	/* [in] */ SIZE_T BytecodeLength,
	/* [in] */ ID3D11ClassLinkage *pClassLinkage,
	/* [out] */ ID3D11ComputeShader **ppComputeShader
	)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateComputeShader", ppComputeShader);
	return gcpRendD3D->m_pActuald3dDevice->CreateComputeShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppComputeShader);
}

HRESULT CMyDirect3DDevice11::CreateClassLinkage(
	/* [out] */ ID3D11ClassLinkage **ppLinkage
	)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateClassLinkage", ppLinkage);
	return gcpRendD3D->m_pActuald3dDevice->CreateClassLinkage(ppLinkage);
}

HRESULT CMyDirect3DDevice11::CreateDeferredContext(
	/* [in] */ UINT ContextFlags,
	/* [out] */ ID3D11DeviceContext **ppDeferredContext
	)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateDeferredContext", ppDeferredContext);
	return gcpRendD3D->m_pActuald3dDevice->CreateDeferredContext(ContextFlags, ppDeferredContext);
}

HRESULT CMyDirect3DDevice11::CheckFeatureSupport(
	/* [in] */ D3D11_FEATURE Feature,
	/* [in] */ void *pFeatureSupportData,
	/* [in] */ UINT FeatureSupportDataSize
	)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CheckFeatureSupport", Feature);
	return gcpRendD3D->m_pActuald3dDevice->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
}

D3D_FEATURE_LEVEL CMyDirect3DDevice11::GetFeatureLevel()
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", "D3DDevice::GetFeatureLevel");
	return gcpRendD3D->m_pActuald3dDevice->GetFeatureLevel();
}

void CMyDirect3DDevice11::GetImmediateContext(
	/* [out] */ ID3D11DeviceContext **ppImmediateContext
	)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::GetImmediateContext", ppImmediateContext);
	return gcpRendD3D->m_pActuald3dDevice->GetImmediateContext(ppImmediateContext);
}

HRESULT CMyDirect3DDevice11::CreateCounter( 
  /* [in] */ const D3D11_COUNTER_DESC *pCounterDesc,
  /* [out] */ ID3D11Counter **ppCounter)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CreateCounter", pCounterDesc);
  return gcpRendD3D->m_pActuald3dDevice->CreateCounter(pCounterDesc, ppCounter);
}
#endif
HRESULT CMyDirect3DDevice11::CheckFormatSupport( 
  /* [in] */ DXGI_FORMAT Format,
  /* [retval][out] */ UINT *pFormatSupport)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%s)\n", "D3DDevice::CheckFormatSupport", sD3DFMT(Format));
  return gcpRendD3D->m_pActuald3dDevice->CheckFormatSupport(Format, pFormatSupport);
}

HRESULT CMyDirect3DDevice11::CheckMultisampleQualityLevels( 
  DXGI_FORMAT Format,
  /* [in] */ UINT SampleCount,
  /* [retval][out] */ UINT *pNumQualityLevels)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%s, %d)\n", "D3DDevice::CheckMultisampleQualityLevels", sD3DFMT(Format), SampleCount);
  return gcpRendD3D->m_pActuald3dDevice->CheckMultisampleQualityLevels(Format, SampleCount, pNumQualityLevels);
}

void CMyDirect3DDevice11::CheckCounterInfo( 
  /* [retval][out] */ D3D11_COUNTER_INFO *pCounterInfo)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CheckCounterInfo", pCounterInfo);
  gcpRendD3D->m_pActuald3dDevice->CheckCounterInfo(pCounterInfo);
}

HRESULT CMyDirect3DDevice11::CheckCounter( 
  /*  */ 
  __in  const D3D11_COUNTER_DESC *pDesc,
  /*  */ 
  __out  D3D11_COUNTER_TYPE *pType,
  /*  */ 
  __out  UINT *pActiveCounters,
  /*  */ 
  __out_ecount_opt(*pNameLength)  LPSTR wszName,
  /*  */ 
  __inout_opt  UINT *pNameLength,
  /*  */ 
  __out_ecount_opt(*pUnitsLength)  LPSTR wszUnits,
  /*  */ 
  __inout_opt  UINT *pUnitsLength,
  /*  */ 
  __out_ecount_opt(*pDescriptionLength)  LPSTR wszDescription,
  /*  */ 
  __inout_opt  UINT *pDescriptionLength)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDevice::CheckCounter", pDesc);
  return gcpRendD3D->m_pActuald3dDevice->CheckCounter(pDesc, pType, pActiveCounters, wszName, pNameLength, wszUnits, pUnitsLength, wszDescription, pDescriptionLength);
}

void CMyDirect3DDeviceContext11::CopySubresourceRegion(
                                   ID3D11Resource * pDstResource,
                                   UINT DstSubresource,
                                   UINT DstX,
                                   UINT DstY,
                                   UINT DstZ,
                                   ID3D11Resource * pSrcResource,
                                   UINT SrcSubresource,
                                   const D3D11_BOX * pSrcBox
                                   )
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::CopySubresourceRegion");
  gcpRendD3D->m_pActuald3dDeviceContext->CopySubresourceRegion(pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);
}

void CMyDirect3DDeviceContext11::UpdateSubresource(
  ID3D11Resource * pDstResource,
  UINT DstSubresource,
  const D3D11_BOX * pDstBox,
  const void *pSrcData,
  UINT SrcRowPitch,
  UINT SrcDepthPitch
  )
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::UpdateSubresource");
  gcpRendD3D->m_pActuald3dDeviceContext->UpdateSubresource(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}

void CMyDirect3DDeviceContext11::ResolveSubresource(
  ID3D11Resource * pDstResource,
  UINT DstSubresource,
  ID3D11Resource * pSrcResource,
  UINT SrcSubresource,
  DXGI_FORMAT Format
  )
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::ResolveSubresource");
  gcpRendD3D->m_pActuald3dDeviceContext->ResolveSubresource(pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
}

void CMyDirect3DDeviceContext11::CopyResource(ID3D11Resource * pDstResource, ID3D11Resource * pSrcResource)
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::CopyResource");
  gcpRendD3D->m_pActuald3dDeviceContext->CopyResource(pDstResource, pSrcResource);
}

void CMyDirect3DDeviceContext11::ClearState()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::ClearState");
  gcpRendD3D->m_pActuald3dDeviceContext->ClearState();
}

HRESULT CMyDirect3DDevice11::OpenSharedResource(
  HANDLE hResource,
  REFIID ReturnedInterface,
  void ** ppResource
  )
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDevice::OpenSharedResource");
  return gcpRendD3D->m_pActuald3dDevice->OpenSharedResource(hResource, ReturnedInterface, ppResource);
}

UINT CMyDirect3DDevice11::GetCreationFlags()
{
  gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDevice::GetCreationFlags");
  return gcpRendD3D->m_pActuald3dDevice->GetCreationFlags();
}

void CMyDirect3DDeviceContext11::GetDevice( 
	/* [out] */  ID3D11Device **ppDevice)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::GetDevice", ppDevice);
	gcpRendD3D->m_pActuald3dDeviceContext->GetDevice(ppDevice);
}

HRESULT CMyDirect3DDeviceContext11::GetPrivateData( 
	/* [in] */  REFGUID guid,
	/* [inout] */  UINT *pDataSize,
	/* [out_opt] */  void *pData)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::GetPrivateData", pData);
	return gcpRendD3D->m_pActuald3dDeviceContext->GetPrivateData(guid, pDataSize, pData);
}

HRESULT CMyDirect3DDeviceContext11::SetPrivateData( 
	/* [in] */  REFGUID guid,
	/* [in] */  UINT DataSize,
	/* [in_opt] */  const void *pData)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::SetPrivateData", pData);
	return gcpRendD3D->m_pActuald3dDeviceContext->SetPrivateData(guid, DataSize, pData);
}

HRESULT CMyDirect3DDeviceContext11::SetPrivateDataInterface( 
	/* [in] */  REFGUID guid,
	/* [in_opt] */  const IUnknown *pData)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::SetPrivateDataInterface", pData);
	return gcpRendD3D->m_pActuald3dDeviceContext->SetPrivateDataInterface(guid, pData);
}

 HRESULT CMyDirect3DDeviceContext11::Map( 
																			/* [in] */  ID3D11Resource *pResource,
																			/* [in] */  UINT Subresource,
																			/* [in] */  D3D11_MAP MapType,
																			/* [in] */  UINT MapFlags,
																			/* [out] */  D3D11_MAPPED_SUBRESOURCE *pMappedResource)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d, %d, %d, 0x%x)\n", "D3DDeviceContext::Map", pResource, Subresource, MapType, MapFlags, pMappedResource);
	return gcpRendD3D->m_pActuald3dDeviceContext->Map(pResource, Subresource, MapType, MapFlags, pMappedResource);
}

 void CMyDirect3DDeviceContext11::Unmap( 
																		 /* [in] */  ID3D11Resource *pResource,
																		 /* [in] */  UINT Subresource)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d)\n", "D3DDeviceContext::Unmap", pResource, Subresource);
	gcpRendD3D->m_pActuald3dDeviceContext->Unmap(pResource, Subresource);
}

 void CMyDirect3DDeviceContext11::Begin( 
																		 /* [in] */  ID3D11Asynchronous *pAsync)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::Begin", pAsync);
	gcpRendD3D->m_pActuald3dDeviceContext->Begin(pAsync);
}

 void CMyDirect3DDeviceContext11::End( 
																	 /* [in] */  ID3D11Asynchronous *pAsync)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::End", pAsync);
	gcpRendD3D->m_pActuald3dDeviceContext->End(pAsync);
}

 HRESULT CMyDirect3DDeviceContext11::GetData( 
	/* [in] */  ID3D11Asynchronous *pAsync,
	/* [out_opt] */  void *pData,
	/* [in] */  UINT DataSize,
	/* [in] */  UINT GetDataFlags)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x, %d, %d)\n", "D3DDeviceContext::GetData", pAsync, pData, DataSize, GetDataFlags);
	return gcpRendD3D->m_pActuald3dDeviceContext->GetData(pAsync, pData, DataSize, GetDataFlags);
}

#ifndef PS3
 void CMyDirect3DDeviceContext11::OMSetRenderTargetsAndUnorderedAccessViews( 
	/* [in] */  UINT NumRTVs,
	/* [in_opt] */  ID3D11RenderTargetView *const *ppRenderTargetViews,
	/* [in_opt] */  ID3D11DepthStencilView *pDepthStencilView,
	/* [in] */  UINT UAVStartSlot,
	/* [in] */  UINT NumUAVs,
	/* [in_opt] */  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
	/* [in_opt] */  const UINT *pUAVInitialCounts)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, 0x%x, 0x%x, %d, %d, 0x%x, 0x%x)\n", "D3DDeviceContext::OMSetRenderTargetsAndUnorderedAccessViews", NumRTVs, ppRenderTargetViews, pDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
	gcpRendD3D->m_pActuald3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(NumRTVs, ppRenderTargetViews, pDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}

 void CMyDirect3DDeviceContext11::DrawIndexedInstancedIndirect( 
	/* [in] */  ID3D11Buffer *pBufferForArgs,
	/* [in] */  UINT AlignedByteOffsetForArgs)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d)\n", "D3DDeviceContext::DrawIndexedInstancedIndirect", pBufferForArgs, AlignedByteOffsetForArgs);
	gcpRendD3D->m_pActuald3dDeviceContext->DrawIndexedInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
}

 void CMyDirect3DDeviceContext11::DrawInstancedIndirect( 
	/* [in] */  ID3D11Buffer *pBufferForArgs,
	/* [in] */  UINT AlignedByteOffsetForArgs)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d)\n", "D3DDeviceContext::DrawInstancedIndirect", pBufferForArgs, AlignedByteOffsetForArgs);
	gcpRendD3D->m_pActuald3dDeviceContext->DrawInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
}

 void CMyDirect3DDeviceContext11::Dispatch( 
																				/* [in] */  UINT ThreadGroupCountX,
																				/* [in] */  UINT ThreadGroupCountY,
																				/* [in] */  UINT ThreadGroupCountZ)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, %d)\n", "D3DDeviceContext::Dispatch", ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	gcpRendD3D->m_pActuald3dDeviceContext->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

 void CMyDirect3DDeviceContext11::DispatchIndirect( 
	/* [in] */  ID3D11Buffer *pBufferForArgs,
	/* [in] */  UINT AlignedByteOffsetForArgs)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d)\n", "D3DDeviceContext::DispatchIndirect", pBufferForArgs, AlignedByteOffsetForArgs);
	gcpRendD3D->m_pActuald3dDeviceContext->DispatchIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
}

 void CMyDirect3DDeviceContext11::CopyStructureCount( 
	/* [in] */  ID3D11Buffer *pDstBuffer,
	/* [in] */  UINT DstAlignedByteOffset,
	/* [in] */  ID3D11UnorderedAccessView *pSrcView)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d, 0x%x)\n", "D3DDeviceContext::CopyStructureCount", pDstBuffer, DstAlignedByteOffset, pSrcView);
	gcpRendD3D->m_pActuald3dDeviceContext->CopyStructureCount(pDstBuffer, DstAlignedByteOffset, pSrcView);
}

 void CMyDirect3DDeviceContext11::ClearUnorderedAccessViewUint( 
	/* [in] */  ID3D11UnorderedAccessView *pUnorderedAccessView,
	/* [in] */  const UINT Values[ 4 ])
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x)\n", "D3DDeviceContext::ClearUnorderedAccessViewUint", pUnorderedAccessView, Values);
	gcpRendD3D->m_pActuald3dDeviceContext->ClearUnorderedAccessViewUint(pUnorderedAccessView, Values);
}

 void CMyDirect3DDeviceContext11::ClearUnorderedAccessViewFloat( 
	/* [in] */  ID3D11UnorderedAccessView *pUnorderedAccessView,
	/* [in] */  const FLOAT Values[ 4 ])
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x)\n", "D3DDeviceContext::ClearUnorderedAccessViewFloat", pUnorderedAccessView, Values);
	gcpRendD3D->m_pActuald3dDeviceContext->ClearUnorderedAccessViewFloat(pUnorderedAccessView, Values);
}

 void CMyDirect3DDeviceContext11::SetResourceMinLOD( 
	/* [in] */  ID3D11Resource *pResource,
	FLOAT MinLOD)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %f)\n", "D3DDeviceContext::SetResourceMinLOD", pResource, MinLOD);
	gcpRendD3D->m_pActuald3dDeviceContext->SetResourceMinLOD(pResource, MinLOD);
}

 FLOAT CMyDirect3DDeviceContext11::GetResourceMinLOD( 
	/* [in] */  ID3D11Resource *pResource)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x)\n", "D3DDeviceContext::GetResourceMinLOD", pResource);
	return gcpRendD3D->m_pActuald3dDeviceContext->GetResourceMinLOD(pResource);
}

 void CMyDirect3DDeviceContext11::ExecuteCommandList( 
	/* [in] */  ID3D11CommandList *pCommandList,
	/* [in] */BOOL RestoreContextState)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, %d)\n", "D3DDeviceContext::ExecuteCommandList", pCommandList, RestoreContextState);
	gcpRendD3D->m_pActuald3dDeviceContext->ExecuteCommandList(pCommandList, RestoreContextState);
}

 void CMyDirect3DDeviceContext11::HSSetShaderResources( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumViews,
	/* [in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::HSSetShaderResources", StartSlot, NumViews, ppShaderResourceViews);
	gcpRendD3D->m_pActuald3dDeviceContext->HSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}

 void CMyDirect3DDeviceContext11::HSSetShader( 
	/* [in_opt] */  ID3D11HullShader *pHullShader,
	/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
	UINT NumClassInstances)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x, %d)\n", "D3DDeviceContext::HSSetShader", pHullShader, ppClassInstances, NumClassInstances);
	gcpRendD3D->m_pActuald3dDeviceContext->HSSetShader(pHullShader, ppClassInstances, NumClassInstances);
}

 void CMyDirect3DDeviceContext11::HSSetSamplers( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumSamplers,
	/* [in] */ ID3D11SamplerState *const *ppSamplers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::HSSetSamplers", StartSlot, NumSamplers, ppSamplers);
	gcpRendD3D->m_pActuald3dDeviceContext->HSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}

 void CMyDirect3DDeviceContext11::HSSetConstantBuffers( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumBuffers,
	/* [in] */ ID3D11Buffer *const *ppConstantBuffers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::HSSetConstantBuffers", StartSlot, NumBuffers, ppConstantBuffers);
	gcpRendD3D->m_pActuald3dDeviceContext->HSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

 void CMyDirect3DDeviceContext11::DSSetShaderResources( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumViews,
	/* [in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::DSSetShaderResources", StartSlot, NumViews, ppShaderResourceViews);
	gcpRendD3D->m_pActuald3dDeviceContext->DSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}

 void CMyDirect3DDeviceContext11::DSSetShader( 
	/* [in_opt] */  ID3D11DomainShader *pDomainShader,
	/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
	UINT NumClassInstances)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x, %d)\n", "D3DDeviceContext::DSSetShader", pDomainShader, ppClassInstances, NumClassInstances);
	gcpRendD3D->m_pActuald3dDeviceContext->DSSetShader(pDomainShader, ppClassInstances, NumClassInstances);
}

 void CMyDirect3DDeviceContext11::DSSetSamplers( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumSamplers,
	/* [in] */ ID3D11SamplerState *const *ppSamplers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::DSSetSamplers", StartSlot, NumSamplers, ppSamplers);
	gcpRendD3D->m_pActuald3dDeviceContext->DSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}

 void CMyDirect3DDeviceContext11::DSSetConstantBuffers( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumBuffers,
	/* [in] */ ID3D11Buffer *const *ppConstantBuffers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::DSSetConstantBuffers", StartSlot, NumBuffers, ppConstantBuffers);
	gcpRendD3D->m_pActuald3dDeviceContext->DSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

 void CMyDirect3DDeviceContext11::CSSetShaderResources( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumViews,
	/* [in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::CSSetShaderResources", StartSlot, NumViews, ppShaderResourceViews);
	gcpRendD3D->m_pActuald3dDeviceContext->CSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}

 void CMyDirect3DDeviceContext11::CSSetUnorderedAccessViews( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumUAVs,
	/* [in] */ ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
	/* [in] */ const UINT *pUAVInitialCounts)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x, 0x%x)\n", "D3DDeviceContext::CSSetUnorderedAccessViews", StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
	gcpRendD3D->m_pActuald3dDeviceContext->CSSetUnorderedAccessViews(StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}

 void CMyDirect3DDeviceContext11::CSSetShader( 
	/* [in_opt] */  ID3D11ComputeShader *pComputeShader,
	/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
	/* [in] */ UINT NumClassInstances)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x, %d)\n", "D3DDeviceContext::CSSetShader", pComputeShader, ppClassInstances, NumClassInstances);
	gcpRendD3D->m_pActuald3dDeviceContext->CSSetShader(pComputeShader, ppClassInstances, NumClassInstances);
}

 void CMyDirect3DDeviceContext11::CSSetSamplers( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumSamplers,
	/* [in] */ ID3D11SamplerState *const *ppSamplers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::CSSetSamplers", StartSlot, NumSamplers, ppSamplers);
	gcpRendD3D->m_pActuald3dDeviceContext->CSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}

 void CMyDirect3DDeviceContext11::CSSetConstantBuffers( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumBuffers,
	/* [in] */ ID3D11Buffer *const *ppConstantBuffers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::CSSetConstantBuffers", StartSlot, NumBuffers, ppConstantBuffers);
	gcpRendD3D->m_pActuald3dDeviceContext->CSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

 void CMyDirect3DDeviceContext11::OMGetRenderTargetsAndUnorderedAccessViews( 
	/* [in] */ UINT NumRTVs,
	/* [out] */ ID3D11RenderTargetView **ppRenderTargetViews,
	/* [out] */  ID3D11DepthStencilView **ppDepthStencilView,
	/* [in] */ UINT UAVStartSlot,
	/* [in] */ UINT NumUAVs,
	/* [out] */ ID3D11UnorderedAccessView **ppUnorderedAccessViews)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, 0x%x, 0x%x, %d, %d, 0x%x)\n", "D3DDeviceContext::OMGetRenderTargetsAndUnorderedAccessViews", NumRTVs, ppRenderTargetViews, ppDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews);
	gcpRendD3D->m_pActuald3dDeviceContext->OMGetRenderTargetsAndUnorderedAccessViews(NumRTVs, ppRenderTargetViews, ppDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews);
}


 void CMyDirect3DDeviceContext11::HSGetShaderResources( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumViews,
	/* [out] */ ID3D11ShaderResourceView **ppShaderResourceViews)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::HSGetShaderResources", StartSlot, NumViews, ppShaderResourceViews);
	gcpRendD3D->m_pActuald3dDeviceContext->HSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}

 void CMyDirect3DDeviceContext11::HSGetShader( 
	/* [out] */  ID3D11HullShader **ppHullShader,
	/* [out] */ ID3D11ClassInstance **ppClassInstances,
	/* [inout] */ UINT *pNumClassInstances)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x 0x%x)\n", "D3DDeviceContext::HSGetShader", ppHullShader, ppClassInstances, pNumClassInstances);
	gcpRendD3D->m_pActuald3dDeviceContext->HSGetShader(ppHullShader, ppClassInstances, pNumClassInstances);
}

 void CMyDirect3DDeviceContext11::HSGetSamplers( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumSamplers,
	/* [in] */ ID3D11SamplerState **ppSamplers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::HSGetSamplers", StartSlot, NumSamplers, ppSamplers);
	gcpRendD3D->m_pActuald3dDeviceContext->HSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}

 void CMyDirect3DDeviceContext11::HSGetConstantBuffers( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumBuffers,
	/* [out] */ ID3D11Buffer **ppConstantBuffers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::HSGetConstantBuffers", StartSlot, NumBuffers, ppConstantBuffers);
	gcpRendD3D->m_pActuald3dDeviceContext->HSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

 void CMyDirect3DDeviceContext11::DSGetShaderResources( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumViews,
	/* [out] */ ID3D11ShaderResourceView **ppShaderResourceViews)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::DSGetShaderResources", StartSlot, NumViews, ppShaderResourceViews);
	gcpRendD3D->m_pActuald3dDeviceContext->DSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}

 void CMyDirect3DDeviceContext11::DSGetShader( 
	/* [out] */  ID3D11DomainShader **ppDomainShader,
	/* [out] */ ID3D11ClassInstance **ppClassInstances,
	/* [inout] */  UINT *pNumClassInstances)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x, 0x%x)\n", "D3DDeviceContext::DSGetShader", ppDomainShader, ppClassInstances, pNumClassInstances);
	gcpRendD3D->m_pActuald3dDeviceContext->DSGetShader(ppDomainShader, ppClassInstances, pNumClassInstances);
}

 void CMyDirect3DDeviceContext11::DSGetSamplers( 
	/* [in] */  UINT StartSlot,
	/* [in] */  UINT NumSamplers,
	/* [out] */  ID3D11SamplerState **ppSamplers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::DSGetSamplers", StartSlot, NumSamplers, ppSamplers);
	gcpRendD3D->m_pActuald3dDeviceContext->DSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}

 void CMyDirect3DDeviceContext11::DSGetConstantBuffers( 
	/* [in] */  UINT StartSlot,
	/* [in] */  UINT NumBuffers,
	/* [out] */ ID3D11Buffer **ppConstantBuffers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::DSGetConstantBuffers", StartSlot, NumBuffers, ppConstantBuffers);
	gcpRendD3D->m_pActuald3dDeviceContext->DSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

 void CMyDirect3DDeviceContext11::CSGetShaderResources( 
	/* [in] */  UINT StartSlot,
	/* [in] */  UINT NumViews,
	/* [out] */ ID3D11ShaderResourceView **ppShaderResourceViews)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::CSGetShaderResources", StartSlot, NumViews, ppShaderResourceViews);
	gcpRendD3D->m_pActuald3dDeviceContext->CSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}

 void CMyDirect3DDeviceContext11::CSGetUnorderedAccessViews( 
	/* [in] */  UINT StartSlot,
	/* [in] */  UINT NumUAVs,
	/* [out] */ ID3D11UnorderedAccessView **ppUnorderedAccessViews)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::CSGetUnorderedAccessViews", StartSlot, NumUAVs, ppUnorderedAccessViews);
	gcpRendD3D->m_pActuald3dDeviceContext->CSGetUnorderedAccessViews(StartSlot, NumUAVs, ppUnorderedAccessViews);
}

 void CMyDirect3DDeviceContext11::CSGetShader( 
	/* [out] */ ID3D11ComputeShader **ppComputeShader,
	/* [out] */ ID3D11ClassInstance **ppClassInstances,
	/* [inout] */ UINT *pNumClassInstances)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(0x%x, 0x%x, 0x%x)\n", "D3DDeviceContext::CSGetUnorderedAccessViews", ppComputeShader, ppClassInstances, pNumClassInstances);
	gcpRendD3D->m_pActuald3dDeviceContext->CSGetShader(ppComputeShader, ppClassInstances, pNumClassInstances);
}

 void CMyDirect3DDeviceContext11::CSGetSamplers( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumSamplers,
	/* [out] */ ID3D11SamplerState **ppSamplers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::CSGetSamplers", StartSlot, NumSamplers, ppSamplers);
	gcpRendD3D->m_pActuald3dDeviceContext->CSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}

 void CMyDirect3DDeviceContext11::CSGetConstantBuffers( 
	/* [in] */ UINT StartSlot,
	/* [in] */ UINT NumBuffers,
	/* [out] */ ID3D11Buffer **ppConstantBuffers)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x)\n", "D3DDeviceContext::CSGetConstantBuffers", StartSlot, NumBuffers, ppConstantBuffers);
	gcpRendD3D->m_pActuald3dDeviceContext->CSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

 D3D11_DEVICE_CONTEXT_TYPE CMyDirect3DDeviceContext11::GetType(void)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::GetType");
	return gcpRendD3D->m_pActuald3dDeviceContext->GetType();
}

 UINT CMyDirect3DDeviceContext11::GetContextFlags(void)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s()\n", "D3DDeviceContext::GetContextFlags");
	return gcpRendD3D->m_pActuald3dDeviceContext->GetContextFlags();
}

 HRESULT CMyDirect3DDeviceContext11::FinishCommandList( 
	BOOL RestoreDeferredContextState,
	/* [out] */ ID3D11CommandList **ppCommandList)
{
	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, 0x%x)\n", "D3DDeviceContext::FinishCommandList", RestoreDeferredContextState, ppCommandList);
	return gcpRendD3D->m_pActuald3dDeviceContext->FinishCommandList(RestoreDeferredContextState, ppCommandList);
}
#endif	// !PS3
#endif  // (DIRECT3D9) || defined (OPENGL)

 void CD3D9Renderer::SetLogFuncs(bool bSet)
 {
	 //#ifndef PS3
	 static bool sSet = 0;

#ifdef OPENGL
	 //  glSetLogFuncs(bSet);
	 return;
#else
	 if (bSet == sSet)
		 return;

	 sSet = bSet;

#if defined (DIRECT3D9) || defined (OPENGL)
	 if (bSet)
	 {
		 m_pActuald3dDevice = m_pd3dDevice;
		 if (!m_pMyd3dDevice)
			 m_pMyd3dDevice = new CMyDirect3DDevice9;
		 m_pd3dDevice = m_pMyd3dDevice;
	 }
	 else
	 {
		 m_pd3dDevice = m_pActuald3dDevice;
	 }
#elif defined (DIRECT3D10)
	 if (bSet)
	 {
		 m_pActuald3dDevice = m_pd3dDevice;
		 m_pActuald3dDeviceContext = m_pd3dDeviceContext;
		 if (!m_pMyd3dDevice)
			 m_pMyd3dDevice = new CMyDirect3DDevice11;
		 m_pd3dDevice = m_pMyd3dDevice;
		 if (!m_pMyd3dDeviceContext)
			 m_pMyd3dDeviceContext = new CMyDirect3DDeviceContext11;
		 m_pd3dDeviceContext = m_pMyd3dDeviceContext;
	 }
	 else
	 {
		 m_pd3dDevice = m_pActuald3dDevice;
		 m_pd3dDeviceContext = m_pActuald3dDeviceContext;
	 }
#endif

#endif
	 //#endif//PS3
 }

#endif //DO_RENDERLOG