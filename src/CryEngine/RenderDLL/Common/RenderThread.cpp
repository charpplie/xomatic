/*=============================================================================
	RenderThread.cpp: Render thread commands processing.
	Copyright (c) 2001 Crytek Studios. All Rights Reserved.

	Revision history:
		* Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include <IFlashPlayer.h>
#include <IVideoPlayer.h>
#include <I3DEngine.h>
#include "RenderAuxGeom.h"
#include "IColorGradingControllerInt.h"

#ifdef STRIP_RENDER_THREAD
	#define m_nCurThreadFill 0
	#define m_nCurThreadProcess 0
#endif

#ifdef WIN32
HWND SRenderThread::GetRenderWindowHandle()
{
	return (HWND)gRenDev->GetHWND();
}
#endif

void CRenderThread::Run()
{
	CryThreadSetName(-1, RENDER_THREAD_NAME);
	gEnv->pSystem->GetIThreadTaskManager()->MarkThisThreadForDebugging(RENDER_THREAD_NAME,true);





	uint32 renderThreadId = ::GetCurrentThreadId();
  gRenDev->m_pRT->m_nRenderThread = renderThreadId;
	gEnv->pCryPak->SetRenderThreadId( renderThreadId );
#ifndef STRIP_RENDER_THREAD
	gRenDev->m_pRT->m_nCurThreadProcess = 1;
#endif
	m_started.Set();
	gRenDev->m_pRT->Process();
	gEnv->pSystem->GetIThreadTaskManager()->MarkThisThreadForDebugging(RENDER_THREAD_NAME,false);
}

SRenderThread::SRenderThread()
{
  Init(2);
}

void SRenderThread::Init(int nCPU)
{
  m_bQuit = false;
#ifndef STRIP_RENDER_THREAD
  m_nCurThreadFill = 0;
  m_nCurThreadProcess = 0;
#endif
  InitFlushCond();
  m_nRenderThread = ::GetCurrentThreadId();
  m_nMainThread = m_nRenderThread;
  m_bSuccessful = true;
  m_pThread = NULL;
#if !defined(STRIP_RENDER_THREAD)
  SSystemGlobalEnvironment *pEnv = iSystem->GetGlobalEnvironment();
  if (pEnv && !pEnv->bTesting && !pEnv->bEditor && pEnv->pi.numCoresAvailableToProcess > 1 && CRenderer::CV_r_multithreaded > 0)
  {
    m_pThread = new CRenderThread(nCPU);
  }
  else
    CRenderer::CV_r_multithreaded = 0;
#else
  CRenderer::CV_r_multithreaded = 0;
#endif

  m_Commands[0].Free();
#ifndef STRIP_RENDER_THREAD
  m_Commands[1].Free();
#endif

  gRenDev->m_RP.m_nProcessThreadID = m_nCurThreadProcess;
  gRenDev->m_RP.m_nFillThreadID = m_nCurThreadFill;
  gRenDev->m_fTimeWaitForMain[0] = 0;
  gRenDev->m_fTimeWaitForMain[1] = 0;
  gRenDev->m_fTimeWaitForRender[0] = 0;
  gRenDev->m_fTimeWaitForRender[1] = 0;
	gRenDev->m_fTimeProcessedRT[0] = 0;
	gRenDev->m_fTimeProcessedRT[1] = 0;
}

SRenderThread::~SRenderThread()
{
  Quit();
}

//==============================================================================================
// NOTE: Render commands can be added from main thread only

bool SRenderThread::RC_CreateDevice()
{
  if (IsRenderThread())
  {
    return gRenDev->RT_CreateDevice();
  }
  AddCommand(eRC_CreateDevice);

  FlushAndWait();

  return !IsFailed();
}

void SRenderThread::RC_ResetDevice()
{
  if (IsRenderThread())
  {
    return gRenDev->RT_Reset();
  }
  AddCommand(eRC_ResetDevice);

  FlushAndWait();
}

void SRenderThread::RC_TexStreamComplete(IReadStream* pStream)
{
  if (IsRenderThread())
  {
    CTexture::StreamOnComplete(pStream);
		return;
  }
  AddCommand(eRC_TexStreamComplete);
  AddPointer(pStream);
}

void SRenderThread::RC_PreloadTextures()
{
  if (IsRenderThread())
  {
    return CTexture::RT_Precache();
  }
  AddCommand(eRC_PreloadTextures);

  FlushAndWait();
}

void SRenderThread::RC_Init()
{
  //LOADING_TIME_PROFILE_SECTION(GetISystem());

  if (IsRenderThread())
  {
    return gRenDev->RT_Init();
  }
  AddCommand(eRC_Init);

  FlushAndWait();
}

HRESULT SRenderThread::RC_CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, UINT Pool, void** ppVertexBuffer, HANDLE* pSharedHandle)
{
  if (IsRenderThread())
  {
    return gRenDev->RT_CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);
  }
  AddCommand(eRC_CreateVertexBuffer);
  AddDWORD(Length);
  AddDWORD(Usage);
  AddDWORD(FVF);
  AddDWORD(Pool);
  AddPointer(ppVertexBuffer);
  AddPointer(pSharedHandle);

  FlushAndWait();

  return m_hResult;
}
HRESULT SRenderThread::RC_CreateIndexBuffer(UINT Length, DWORD Usage, DWORD Format, UINT Pool, void** ppIndexBuffer, HANDLE* pSharedHandle)
{
  if (IsRenderThread())
  {
    return gRenDev->RT_CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);
  }

  AddCommand(eRC_CreateIndexBuffer);
  AddDWORD(Length);
  AddDWORD(Usage);
  AddDWORD(Format);
  AddDWORD(Pool);
  AddPointer(ppIndexBuffer);
  AddPointer(pSharedHandle);

  FlushAndWait();

  return m_hResult;
}

SShaderItem SRenderThread::RC_LoadShaderItem (const char *szName, bool bShare, int flags, SInputShaderResources *Res, uint64 nMaskGen)
{
  if (IsRenderThread())
  {
    return gRenDev->m_cEF.mfShaderItemForName(szName, bShare, flags, Res, nMaskGen);
  }
  SShaderItem It;

  AddCommand(eRC_LoadShaderItem);
  AddPointer(&It);
  AddPointer(szName);
  AddDWORD(bShare);
  AddDWORD(flags);
  AddPointer(Res);
  AddDWORD64(nMaskGen);

  FlushAndWait();

  return It;
}

void SRenderThread::RC_UpdateMaterialConstants(SRenderShaderResources *pSR, IShader *pSH)
{
  if (IsRenderThread())
  {
    return pSR->RT_UpdateConstants(pSH);
  }
  AddCommand(eRC_UpdateMaterialConstants);
  AddPointer(pSR);
  AddPointer(pSH);

  FlushAndWait();
}


void SRenderThread::RC_ReleaseVBStream(void *pVB, int nStream)
{
  if (IsRenderThread())
  {
    gRenDev->RT_ReleaseVBStream(pVB, nStream);
    return;
  }

  AddCommand(eRC_ReleaseVBStream);
  AddPointer(pVB);
  AddDWORD(nStream);
}

HRESULT SRenderThread::RC_CreatePixelShader(DWORD *pBuf, void**pShader)
{
  if (IsRenderThread())
  {
    return gRenDev->RT_CreatePixelShader(pBuf, pShader);
  }

  AddCommand(eRC_CreatePixelShader);
  AddPointer(pBuf);
  AddPointer(pShader);
  FlushAndWait();

  return m_hResult;
}
HRESULT SRenderThread::RC_CreateVertexShader(DWORD *pBuf, void **pShader, void *pInst)
{
  if (IsRenderThread())
  {
    return gRenDev->RT_CreateVertexShader(pBuf, pShader, pInst);
  }

  AddCommand(eRC_CreateVertexShader);
  AddPointer(pBuf);
  AddPointer(pShader);
  AddPointer(pInst);
  FlushAndWait();

  return m_hResult;
}

void SRenderThread::RC_UpdateTextureRegion(CTexture *pTex, byte *data, int nX, int nY, int USize, int VSize, ETEX_Format eTFSrc)
{
  if (IsRenderThread())
  {
    return pTex->RT_UpdateTextureRegion(data, nX, nY, USize, VSize, eTFSrc);
  }

  int nSize = CTexture::TextureDataSize(USize, VSize, 1, 1, eTFSrc);
  byte *pData = new byte[nSize];
  cryMemcpy(pData, data, nSize);
  pTex->AddRef();
  AddCommand(eRC_UpdateTexture);
  AddPointer(pTex);
  AddPointer(pData);
  AddDWORD(nX);
  AddDWORD(nY);
  AddDWORD(USize);
  AddDWORD(VSize);
  AddDWORD(eTFSrc);
}
bool SRenderThread::RC_DynTexUpdate(SDynTexture *pTex, int nNewWidth, int nNewHeight)
{
  if (IsRenderThread())
  {
    return pTex->RT_Update(nNewWidth, nNewHeight);
  }

  AddCommand(eRC_DynTexUpdate);
  AddPointer(pTex);
  AddDWORD(nNewWidth);
  AddDWORD(nNewHeight);

  return true;
}

bool SRenderThread::RC_DynTexSourceUpdate(IDynTextureSource *pSrc, float fDistance)
{
  if (IsRenderThread())
  {
    return pSrc->Update(fDistance);
  }

  AddCommand(eRC_DynTexSourceUpdate);
  AddPointer(pSrc);
  AddFloat(fDistance);

  return true;
}

void SRenderThread::RC_EntityDelete(IRenderNode * pRenderNode)
{
  if (IsRenderThread())
  {
    return SDynTexture_Shadow::RT_EntityDelete(pRenderNode);
  }

  AddCommand(eRC_EntityDelete);
  AddPointer(pRenderNode);
}

void TexBlurAnisotropicVertical(CTexture *pTex, int nAmount, float fScale, float fDistribution, bool bAlphaOnly);

void SRenderThread::RC_TexBlurAnisotropicVertical(CTexture *Tex, float fAnisoScale)
{
  if (IsRenderThread())
  {
    TexBlurAnisotropicVertical(Tex, 1, 8 * max( 1.0f - min( fAnisoScale / 100.0f, 1.0f), 0.2f), 1, false);
    return;
  }

  AddCommand(eRC_TexBlurAnisotropicVertical);
  AddPointer(Tex);
  AddFloat(fAnisoScale);
}

bool SRenderThread::RC_CreateDeviceTexture(CTexture *pTex, byte *pData[6])
{
  if (IsRenderThread())
  {
    return pTex->RT_CreateDeviceTexture(pData);
  }

  if(pTex->IsAsyncDevTexCreation())
    return !IsFailed();

  AddCommand(eRC_CreateDeviceTexture);
  AddPointer(pTex);
  for (int i=0; i<6; i++)
  {
    AddPointer(pData[i]);
  }
  FlushAndWait();

  return !IsFailed();
}

void SRenderThread::RC_CreateResource(SResourceAsync* pRes)
{
  if (IsRenderThread())
  {
    gRenDev->RT_CreateResource(pRes);
    return;
  }

  AddCommand(eRC_CreateResource);
  AddPointer(pRes);
}
void SRenderThread::RC_ReleaseResource(SResourceAsync* pRes)
{
  if (IsRenderThread())
  {
    gRenDev->RT_ReleaseResource(pRes);
    return;
  }

  AddCommand(eRC_ReleaseResource);
  AddPointer(pRes);
}

void SRenderThread::RC_CreateREPostProcess(CRendElementBase **re)
{
  if (IsRenderThread())
  {
    return gRenDev->RT_CreateREPostProcess(re);
  }

  AddCommand(eRC_CreateREPostProcess);
  AddPointer(re);

  FlushAndWait();
}

bool SRenderThread::RC_CheckUpdate2(CRenderMesh2 *pMesh, CRenderMesh2 *pVContainer, EVertexFormat eVF, uint32 nStreamMask)
{
  if (IsRenderThread())
  {
    return pMesh->RT_CheckUpdate(pVContainer, eVF, nStreamMask);
  }

  AddCommand(eRC_UpdateMesh2);
  AddPointer(pMesh);
  AddPointer(pVContainer);
  AddDWORD(eVF);
  AddDWORD(nStreamMask);

  FlushAndWait();

  return !IsFailed();
}

void SRenderThread::RC_ReleaseCB_SI(int nID)
{
#if defined (DIRECT3D10) || defined(PS3)
  if (!IsMultithreaded())
  {
    CHWShader::mfReleaseCB_SI(nID);
    return;
  }
  AddCommand(eRC_ReleaseCB_SI);
  AddDWORD(nID);
#endif
}
void SRenderThread::RC_ReleaseCB(void *pCB)
{
#if defined (DIRECT3D10) || defined(PS3)
  if (!IsMultithreaded())
  {
    gRenDev->RT_ReleaseCB(pCB);
    return;
  }
  AddCommand(eRC_ReleaseCB);
  AddPointer(pCB);
#endif
}

void SRenderThread::RC_ReleaseVB(int nID)
{
  if (IsRenderThread())
  {
    gRenDev->m_DevBufMan.ReleaseVB(nID);
    return;
  }

  AddCommand(eRC_ReleaseVB);
  AddDWORD(nID);
}
void SRenderThread::RC_ReleaseIB(int nID)
{
  if (IsRenderThread())
  {
    gRenDev->m_DevBufMan.ReleaseIB(nID);
    return;
  }

  AddCommand(eRC_ReleaseIB);
  AddDWORD(nID);
}

void SRenderThread::RC_DrawDynVB(SVF_P3F_C4B_T2F *pBuf, uint16 *pInds, int nVerts, int nInds, int nPrimType)
{
  if (IsRenderThread())
  {
    gRenDev->RT_DrawDynVB(pBuf, pInds, nVerts, nInds, nPrimType);
    return;
  }

  AddCommand(eRC_DrawDynVB);
  AddData(pBuf, sizeof(SVF_P3F_C4B_T2F)*nVerts);
  AddData(pInds, sizeof(uint16)*nInds);
  AddDWORD(nVerts);
  AddDWORD(nInds);
  AddDWORD(nPrimType);
}

void SRenderThread::RC_Draw2dImage(float xpos,float ypos,float w,float h,int textureid,float s0,float t0,float s1,float t1,float angle,float r,float g,float b,float a,float z)
{
  DWORD col = D3DRGBA(r,g,b,a);

  if (IsRenderThread())
  {
    gRenDev->RT_Draw2dImage(xpos, ypos, w, h, textureid, s0, t0, s1, t1, angle, col, z);
    return;
  }

  AddCommand(eRC_Draw2dImage);
  AddFloat(xpos);
  AddFloat(ypos);
  AddFloat(w);
  AddFloat(h);
  AddDWORD(textureid);
  AddFloat(s0);
  AddFloat(t0);
  AddFloat(s1);
  AddFloat(t1);
  AddFloat(angle);
  AddDWORD(col);
  AddFloat(z);
}
void SRenderThread::RC_DrawImageWithUV(float xpos,float ypos,float z, float w,float h,int textureid,float *s,float *t,float r,float g,float b,float a)
{
  DWORD col = D3DRGBA(r,g,b,a);
  if (IsRenderThread())
  {
    gRenDev->RT_DrawImageWithUV(xpos, ypos, z, w, h, textureid, s, t, col);
    return;
  }

  int i;

  AddCommand(eRC_DrawImageWithUV);
  AddFloat(xpos);
  AddFloat(ypos);
  AddFloat(z);
  AddFloat(w);
  AddFloat(h);
  AddDWORD(textureid);
  for (i=0; i<4; i++)
  {
    AddFloat(s[i]);
  }
  for (i=0; i<4; i++)
  {
    AddFloat(t[i]);
  }
  AddDWORD(col);
}

void SRenderThread::RC_SetState(int State, int AlphaRef)
{
  if (IsRenderThread())
  {
    gRenDev->EF_SetState(State, AlphaRef);
    return;
  }

  AddCommand(eRC_SetState);
  AddDWORD(State);
  AddDWORD(AlphaRef);
}

void SRenderThread::RC_SetCull(int nMode)
{
  if (IsRenderThread())
  {
    gRenDev->RT_SetCull(nMode);
    return;
  }

  AddCommand(eRC_SetCull);
  AddDWORD(nMode);
}

void SRenderThread::RC_ReadFrameBuffer(unsigned char * pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY)
{
  if (IsRenderThread())
  {
    gRenDev->RT_ReadFrameBuffer(pRGB, nImageX, nSizeX, nSizeY, eRBType, bRGBA, nScaledX, nScaledY);
    return;
  }

  AddCommand(eRC_ReadFrameBuffer);
  AddPointer(pRGB);
  AddDWORD(nImageX);
  AddDWORD(nSizeX);
  AddDWORD(nSizeY);
  AddDWORD(eRBType);
  AddDWORD(bRGBA);
  AddDWORD(nScaledX);
  AddDWORD(nScaledY);

  FlushAndWait();
}

void SRenderThread::RC_SetCamera()
{
	if (!IsRenderThread())
	{
		AddCommand(eRC_SetCamera);
		byte *pData = m_Commands[m_nCurThreadFill].Grow(sizeof(Matrix44)*3);

		gRenDev->GetProjectionMatrix((float *)&pData[0]);
		gRenDev->GetModelViewMatrix((float *)&pData[sizeof(Matrix44)]);
		*(Matrix44 *)((float *)&pData[sizeof(Matrix44)* 2]) = gRenDev->m_CameraZeroMatrix[m_nCurThreadFill];
		 		
		if( gRenDev->m_RP.m_TI[m_nCurThreadFill].m_PersFlags & RBPF_OBLIQUE_FRUSTUM_CLIPPING)
		{
			Matrix44A mObliqueProjMatrix;
			mObliqueProjMatrix.SetIdentity();

			Plane pPlane = gRenDev->m_RP.m_TI[m_nCurThreadFill].m_pObliqueClipPlane;

			mObliqueProjMatrix.m02 = pPlane.n[0]; 
			mObliqueProjMatrix.m12 = pPlane.n[1];
			mObliqueProjMatrix.m22 = pPlane.n[2];
			mObliqueProjMatrix.m32 = pPlane.d; 
			
			Matrix44 *mProj = (Matrix44 *)&pData[0];

			(*mProj).Multiply((*mProj), mObliqueProjMatrix); 
			gRenDev->m_RP.m_TI[m_nCurThreadFill].m_PersFlags &= ~RBPF_OBLIQUE_FRUSTUM_CLIPPING;
		}
	}
	else
		gRenDev->RT_SetCameraInfo();
}


void SRenderThread::RC_PushFog()
{
  if (!IsRenderThread())
  {
    AddCommand(eRC_PushFog);
  }
  else
    gRenDev->EF_PushFog();
}

void SRenderThread::RC_PopFog()
{
  if (!IsRenderThread())
  {
    AddCommand(eRC_PopFog);
  }
  else
    gRenDev->EF_PopFog();
}

void SRenderThread::RC_PushVP()
{
  if (!IsRenderThread())
  {
    AddCommand(eRC_PushVP);
  }
  else
    gRenDev->FX_PushVP();
}

void SRenderThread::RC_PopVP()
{
  if (!IsRenderThread())
  {
    AddCommand(eRC_PopVP);
  }
  else
    gRenDev->FX_PopVP();
}

void SRenderThread::RC_ClearRT(int nFlags, ColorF *pCol, float fDepth)
{
  if (!IsRenderThread())
  {
    AddCommand(eRC_ClearRT);
    AddDWORD(nFlags);
    AddColor(*pCol);
    AddFloat(fDepth);
  }
  else
    gRenDev->EF_ClearBuffers(nFlags, pCol, fDepth);
}

void SRenderThread::RC_FlushTextMessages()
{
  if (!IsRenderThread())
    AddCommand(eRC_FlushTextMessages);
  else
    gRenDev->RT_FlushTextMessages();
}

void SRenderThread::RC_SetEnvTexRT(SEnvTexture *pEnvTex, int nWidth, int nHeight, bool bPush)
{
  if (!IsRenderThread())
  {
    AddCommand(eRC_SetEnvTexRT);
    AddPointer(pEnvTex);
    AddDWORD(nWidth);
    AddDWORD(nHeight);
    AddDWORD(bPush);
  }
  else
    pEnvTex->m_pTex->RT_SetRT(0, nWidth, nHeight, bPush);
}


void SRenderThread::RC_SetEnvTexMatrix(SEnvTexture *pEnvTex)
{
	if (!IsRenderThread())
	{
		AddCommand(eRC_SetEnvTexMatrix);
		AddPointer(pEnvTex);
	}
	else
		pEnvTex->RT_SetMatrix();
}

void SRenderThread::RC_PushRT(int nTarget, CTexture *pTex, SD3DSurface *pDS, int nS)
{
  if (!IsRenderThread())
  {
    AddCommand(eRC_PushRT);
    AddDWORD(nTarget);
    AddPointer(pTex);
    AddPointer(pDS);
    AddDWORD(nS);
  }
  else
    gRenDev->RT_PushRenderTarget(nTarget, pTex, pDS, nS);
}
void SRenderThread::RC_PopRT(int nTarget)
{
  if (!IsRenderThread())
  {
    AddCommand(eRC_PopRT);
    AddDWORD(nTarget);
  }
  else
    gRenDev->RT_PopRenderTarget(nTarget);
}

void SRenderThread::RC_VideoReleaseTextures(IVideoPlayer_RenderProxy* pVideo)
{
  if (IsRenderThread())
  {
    pVideo->DestroyTexturesCallback();
    return;
  }

  AddCommand(eRC_VideoReleaseTextures);
  AddPointer(pVideo);

  FlushAndWait();
}

bool SRenderThread::RC_VideoRender(IVideoPlayer_RenderProxy* pVideo)
{
  if (IsRenderThread())
  {
    pVideo->RenderCallback();
    return true;
  }

  AddCommand(eRC_VideoRender);
  AddPointer(pVideo);
  return true;
}

void SRenderThread::RC_BeginFrame()
{
  if (IsRenderThread())
  {
    gRenDev->RT_BeginFrame();
    return;
  }

  AddCommand(eRC_BeginFrame);
}
void SRenderThread::RC_EndFrame()
{
  if (IsRenderThread())
  {
    gRenDev->RT_EndFrame();
    return;
  }

	gRenDev->FlushMainThreadAuxGeomCB(); // need to issue flush of main thread's aux cb before EndFrame (otherwise it is processed after p3dDev->EndScene())
  AddCommand(eRC_EndFrame);
  FlushFrame();
	gRenDev->ToggleMainThreadAuxGeomCB(); // toggle main thread's aux cb after frame is flushed (otherwise interferes with threaded execution of double buffered data of same cb)
}

void SRenderThread::RC_PrecacheResource(ITexture *pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId)
{
	if (IsRenderThread())
	{
		gRenDev->PrecacheTexture(pTP, fMipFactor, fTimeToReady, Flags, nUpdateId);
		return;
	}

	AddCommand(eRC_PrecacheTexture);
	AddPointer(pTP);
	AddFloat(fMipFactor);
	AddFloat(fTimeToReady);
	AddDWORD(Flags);
	AddDWORD(nUpdateId);
}

void SRenderThread::RC_ReleaseDeviceTexture(CTexture *pTexture)
{
  if (IsRenderThread())
  {
    pTexture->RT_ReleaseDevice();
    return;
  }

  AddCommand(eRC_ReleaseDeviceTexture);
  AddPointer(pTexture);

  FlushAndWait();
}

void SRenderThread::RC_DrawStringW(IFFont *pFont, float fBaseX, float fBaseY, float fBaseZ, const wchar_t *szMsg, const bool bASCIIMultiLine)
{
  if (IsRenderThread())
  {
    pFont->RT_DrawStringW(fBaseX, fBaseY, fBaseZ, szMsg, bASCIIMultiLine);
    return;
  }

  int nSize;
  struct SFontRenderContext *pCont = pFont->GetRenderContext(nSize);
  AddCommand(eRC_DrawStringW);
  AddFloat(fBaseX);
  AddFloat(fBaseY);
  AddFloat(fBaseZ);
  AddDWORD(bASCIIMultiLine);
  byte *pData = m_Commands[m_nCurThreadFill].Grow(nSize);
  memcpy(pData, pCont, nSize);
  AddText(szMsg);
}

void SRenderThread::RC_ClearBuffers(uint32 nFlags, ColorF *vColor, float depth)
{
  if (IsRenderThread())
  {
    gRenDev->EF_ClearBuffers(nFlags, vColor, depth);
    return;
  }

  AddCommand(eRC_ClearBuffers);
  AddDWORD(nFlags);
  AddColor(*vColor);
  AddFloat(depth);
}

void SRenderThread::RC_SetViewport(int x, int y, int width, int height)
{
  if (IsRenderThread())
  {
    gRenDev->RT_SetViewport(x, y, width, height);
    return;
  }

  AddCommand(eRC_SetViewport);
  AddDWORD(x);
  AddDWORD(y);
  AddDWORD(width);
  AddDWORD(height);
}

void SRenderThread::RC_RenderScene(int nFlags, RenderFunc pRenderFunc, SRenderListDesc* pRLD)
{
  if (IsRenderThread())
  {
    gRenDev->RT_RenderScene(nFlags, gRenDev->m_RP.m_TI[m_nCurThreadProcess], pRenderFunc, pRLD);
    return;
  }

  AddCommand(eRC_RenderScene);
  AddDWORD(nFlags);
  AddTI(gRenDev->m_RP.m_TI[m_nCurThreadFill]);
  AddPointer((void*)pRenderFunc);
  AddRLD(pRLD);
  AddDWORD(SRendItem::m_RecurseLevel[m_nCurThreadFill]);
}

void SRenderThread::RC_AuxFlush(IRenderAuxGeomImpl* pAux, const SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end)
{
#if defined(ENABLE_RENDER_AUX_GEOM)
	if (IsRenderThread())
	{
		pAux->RT_Flush(data, begin, end);
		return;
	}

	AddCommand(eRC_AuxFlush);
	AddPointer(pAux);
	AddPointer(data.m_pData);
	AddPointer((void*) begin);
	AddPointer((void*) end);
#endif
}

void SRenderThread::RC_FlashRender(IFlashPlayer_RenderProxy* pPlayer, bool stereo)
{
  if (IsRenderThread())
  {
    gRenDev->SetProfileMarker("FLASH_RENDERING", CRenderer::ESPM_PUSH);
    //pPlayer->RenderCallback(); //Diesel cut
	pPlayer->RenderCallback(IFlashPlayer_RenderProxy::EFT_Mono); //Diesel new
    gRenDev->SetProfileMarker("FLASH_RENDERING", CRenderer::ESPM_POP);
    return;
  }

  AddCommand(eRC_FlashRender);
  AddPointer(pPlayer);
  AddDWORD(stereo ? 1 : 0);
}

void SRenderThread::RC_FlashRemoveTexture(CTexture* pTexture)
{
	if (IsRenderThread())
	{
		SAFE_RELEASE(pTexture);
		return;
	}

	AddCommand(eRC_FlashRemoveTexture);
	AddPointer(pTexture);
}

void SRenderThread::RC_SetTexture(int nTex, int nUnit)
{
  if (IsRenderThread())
  {
    CTexture::ApplyForID(nTex, nUnit);
    return;
  }

  AddCommand(eRC_SetTexture);
  AddDWORD(nTex);
  AddDWORD(nUnit);
}

void SRenderThread::RC_PreprGenerateFarTrees(CREFarTreeSprites *pRE)
{
  if (IsRenderThread())
  {
    gRenDev->GenerateObjSprites(pRE->m_arrVegetationSprites[gRenDev->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID]-1]);
    return;
  }
  AddCommand(eRC_PreprGenerateFarTrees);
  AddPointer(pRE);
}
void SRenderThread::RC_PreprGenerateCloud(CRendElementBase *pRE, CShader *pShader, SRenderShaderResources *pRes, CRenderObject *pObject)
{
  if (IsRenderThread())
  {
    CRECloud *pREC = (CRECloud *)pRE;
    pREC->GenerateCloudImposter(pShader, pRes, pObject);
    return;
  }

  AddCommand(eRC_PreprGenerateCloud);
  AddPointer(pRE);
  AddPointer(pShader);
  AddPointer(pRes);
  AddPointer(pObject);
}

bool SRenderThread::RC_OC_ReadResult_Try(CREOcclusionQuery *pRE)
{
  if (IsRenderThread())
  {
    return pRE->RT_ReadResult_Try();
  }
  AddCommand(eRC_OC_ReadResult_Try);
  AddPointer(pRE);

  return true;
}

void SRenderThread::RC_CGCSetLayers(IColorGradingControllerInt* pController, const SColorChartLayer* pLayers, uint32 numLayers)
{
	if (IsRenderThread())
	{
		pController->RT_SetLayers(pLayers, numLayers);
		return;
	}

	AddCommand(eRC_CGCSetLayers);
	AddPointer(pController);
	AddDWORD((uint32)numLayers);

	if (numLayers)
	{
		const size_t copySize = sizeof(SColorChartLayer) * numLayers;
		SColorChartLayer* pLayersDst = (SColorChartLayer*) m_Commands[m_nCurThreadFill].Grow(copySize);
		memcpy(pLayersDst, pLayers, copySize);
	}
}

//===========================================================================================

#pragma warning(push)
#pragma warning(disable : 4800)
void SRenderThread::ProcessCommands()
{
#ifndef STRIP_RENDER_THREAD
  assert (GetCurrentThreadId() == m_nRenderThread);
  if (!CheckFlushCond())
    return;

  int n = 0;
  m_bSuccessful = true;
  m_hResult = S_OK;
  byte *pP;
  while (n < (int)m_Commands[m_nCurThreadProcess].Num())
  {
#ifdef RENDERTHREAD_ALIGN_COMMANDS
		pP = &m_Commands[m_nCurThreadProcess][n];
		n += sizeof(int);
		byte nC = (byte)*((int *)pP);
#else
    byte nC = m_Commands[m_nCurThreadProcess][n++];
#endif
    switch (nC)
    {
    case eRC_CreateDevice:
      m_bSuccessful &= gRenDev->RT_CreateDevice();
      break;
    case eRC_ResetDevice:
      gRenDev->RT_Reset();
      break;
    case eRC_Init:
      gRenDev->RT_Init();
      break;
    case eRC_BeginFrame:
      gRenDev->RT_BeginFrame();
      break;
    case eRC_EndFrame:
      gRenDev->RT_EndFrame();
      break;
    case eRC_PreloadTextures:
      CTexture::RT_Precache();
      break;

		case eRC_PrecacheTexture:
			{
				ITexture* pTP = ReadCommand<ITexture*>(n);
				float fMipFactor = ReadCommand<float>(n);
				float fTimeToReady = ReadCommand<float>(n);
				int Flags = ReadCommand<int>(n);
				int nUpdateId = ReadCommand<int>(n);
				gRenDev->PrecacheTexture(pTP, fMipFactor, fTimeToReady, Flags, nUpdateId);
			}
			break;
    case eRC_ClearBuffers:
      {
        uint32 nFlags = ReadCommand<uint32>(n);
        ColorF vColor = ReadCommand<ColorF>(n);
        float fDepth = ReadCommand<float>(n);
        gRenDev->EF_ClearBuffers(nFlags, &vColor, fDepth);
      }
      break;
    case eRC_ReadFrameBuffer:
      {
        byte *pRGB = ReadCommand<byte *>(n);
        int nImageX = ReadCommand<int>(n);
        int nSizeX = ReadCommand<int>(n);
        int nSizeY = ReadCommand<int>(n);
        ERB_Type RBType = ReadCommand<ERB_Type>(n);
        int bRGBA = ReadCommand<int>(n);
        int nScaledX = ReadCommand<int>(n);
        int nScaledY = ReadCommand<int>(n);
        gRenDev->RT_ReadFrameBuffer(pRGB, nImageX, nSizeX, nSizeY, RBType, bRGBA, nScaledX, nScaledY);
      }
      break;
    case eRC_ReleaseDeviceTexture:
      {
        CTexture *pTexture = ReadCommand<CTexture *>(n);
        pTexture->RT_ReleaseDevice();
      }
      break;
    case eRC_AuxFlush:
      {
#if defined(ENABLE_RENDER_AUX_GEOM)
				IRenderAuxGeomImpl* pAux = ReadCommand<IRenderAuxGeomImpl*>(n);
				CAuxGeomCB::SAuxGeomCBRawData* pData = ReadCommand<CAuxGeomCB::SAuxGeomCBRawData*>(n);
				size_t begin = ReadCommand<size_t>(n);
				size_t end = ReadCommand<size_t>(n);
				pAux->RT_Flush(SAuxGeomCBRawDataPackaged(pData), begin, end);
#endif
      }
      break;
    case eRC_FlashRender:
      {
        IFlashPlayer_RenderProxy* pPlayer = ReadCommand<IFlashPlayer_RenderProxy *>(n);
        bool stereo = ReadCommand<int>(n) != 0;
        gRenDev->SetProfileMarker("FLASH_RENDERING", CRenderer::ESPM_PUSH);
        //pPlayer->RenderCallback(); //Diesel cut
		pPlayer->RenderCallback(IFlashPlayer_RenderProxy::EFT_Mono); //Diesel new
        gRenDev->SetProfileMarker("FLASH_RENDERING", CRenderer::ESPM_POP);
      }
      break;
    case eRC_FlashRemoveTexture:
      {
        CTexture *pTexture = ReadCommand<CTexture *>(n);
        SAFE_RELEASE(pTexture);
      }
      break;
    case eRC_SetTexture:
      {
        int nTex = ReadCommand<int>(n);
        int nUnit = ReadCommand<int>(n);
        CTexture::ApplyForID(nTex, nUnit);
      }
      break;
    case eRC_DrawStringW:
      {
        pP = &m_Commands[m_nCurThreadProcess][n];
        static IFFont *s_pFont = NULL;
        if (!s_pFont)
          s_pFont = gEnv->pCryFont->NewFont("TempRender");
        int nCN = s_pFont->SetRenderContext((struct SFontRenderContext *)&pP[16]);
        wchar_t *szText = (wchar_t *)&pP[20+nCN];
        s_pFont->RT_DrawStringW(*(float *)&pP[0], // x
          *(float *)&pP[4],  // y
          *(float *)&pP[8], // z
          szText,  // pText
          *(bool *)&pP[12]);  // bAscii
        int nLen = *(int *)&pP[16+nCN];
        n += 20 + nCN + nLen;
#ifdef RENDERTHREAD_ALIGN_COMMANDS
				n += (unsigned)-nLen % 4;
#endif
      }
      break;
    case eRC_SetState:
      {
        int nState = ReadCommand<int>(n);
        int nAlphaRef = ReadCommand<int>(n);
        gRenDev->EF_SetState(nState, nAlphaRef);
      }
      break;
    case eRC_SetCull:
      {
        int nMode = ReadCommand<int>(n);
        gRenDev->RT_SetCull(nMode); // nMode
      }
      break;
    case eRC_UpdateTexture:
      {
        CTexture *pTexture = ReadCommand<CTexture *>(n);
        byte *pData = ReadCommand<byte *>(n);
        int nX = ReadCommand<int>(n);
        int nY = ReadCommand<int>(n);
        int nUSize = ReadCommand<int>(n);
        int nVSize = ReadCommand<int>(n);
        ETEX_Format eTFSrc = ReadCommand<ETEX_Format>(n);
        pTexture->RT_UpdateTextureRegion(pData, nX, nY, nUSize, nVSize, eTFSrc);
        delete [] pData;
        pTexture->Release();
      }
      break;
    case eRC_CreateResource:
      {
        SResourceAsync *pRA = ReadCommand<SResourceAsync *>(n);
        gRenDev->RT_CreateResource(pRA);
      }
      break;
    case eRC_ReleaseResource:
      {
        SResourceAsync* pRes = ReadCommand<SResourceAsync *>(n);
        gRenDev->RT_ReleaseResource(pRes);
      }
      break;
    case eRC_UpdateMesh2:
      {
        CRenderMesh2 *pMesh = ReadCommand<CRenderMesh2 *>(n);
        CRenderMesh2 *pVContainer = ReadCommand<CRenderMesh2 *>(n);
        EVertexFormat eVF = ReadCommand<EVertexFormat>(n);
        uint32 nStreamMask = ReadCommand<uint32>(n);
        pMesh->RT_CheckUpdate(pVContainer, eVF, nStreamMask);
      }
      break;
    case eRC_CreateDeviceTexture:
      {
        CTexture *pTex = ReadCommand<CTexture *>(n);
        byte *pData[6];
        for (int i=0; i<6; i++)
        {
          pData[i] = ReadCommand<byte *>(n);
        }
        m_bSuccessful = pTex->RT_CreateDeviceTexture(pData);
      }
      break;
    case eRC_LoadShaderItem:
      {
        SShaderItem *pIT = ReadCommand<SShaderItem *>(n);
        char *szName = ReadCommand<char *>(n);
        int bShare = ReadCommand<int>(n);
        int nFlags = ReadCommand<int>(n);
        SInputShaderResources *pRes = ReadCommand<SInputShaderResources *>(n);
        uint64 nMaskGen = ReadCommand<uint64>(n);
        *pIT = gRenDev->m_cEF.mfShaderItemForName(szName, bShare, nFlags, pRes, nMaskGen);
      }
      break;
    case eRC_UpdateMaterialConstants:
      {
        SRenderShaderResources *pSR = ReadCommand<SRenderShaderResources *>(n);
        IShader *pSH = ReadCommand<IShader *>(n);
        pSR->RT_UpdateConstants(pSH);
      }
      break;
    case eRC_CreateVertexBuffer:
      {
        UINT nLength = ReadCommand<UINT>(n);
        DWORD dwUsage = ReadCommand<DWORD>(n);
        DWORD dwFVF = ReadCommand<DWORD>(n);
        UINT nPool = ReadCommand<UINT>(n);
        void** pVB = ReadCommand<void**>(n);
        HANDLE* pHandle = ReadCommand<HANDLE*>(n);
        m_hResult = gRenDev->RT_CreateVertexBuffer(nLength, dwUsage, dwFVF, nPool, pVB, pHandle);
        m_bSuccessful &= (m_hResult == S_OK);
      }
      break;
    case eRC_CreateIndexBuffer:
      {
        UINT nLength = ReadCommand<UINT>(n);
        DWORD dwUsage = ReadCommand<DWORD>(n);
        DWORD dwFormat = ReadCommand<DWORD>(n);
        UINT nPool = ReadCommand<UINT>(n);
        void** pIB = ReadCommand<void**>(n);
        HANDLE* pHandle = ReadCommand<HANDLE*>(n);
        m_hResult = gRenDev->RT_CreateIndexBuffer(nLength, dwUsage, dwFormat, nPool, pIB, pHandle);
        m_bSuccessful &= (m_hResult == S_OK);
      }
      break;
    case eRC_CreateVertexShader:
      {
        DWORD *pBuf = ReadCommand<DWORD*>(n);
        void **pShader = ReadCommand<void**>(n);
        void *pInst = ReadCommand<void*>(n);
        m_hResult = gRenDev->RT_CreateVertexShader(pBuf, pShader, pInst);
        m_bSuccessful &= (m_hResult == S_OK);
      }
      break;
    case eRC_CreatePixelShader:
      {
        DWORD *pBuf = ReadCommand<DWORD*>(n);
        void **pShader = ReadCommand<void**>(n);
        m_hResult = gRenDev->RT_CreatePixelShader(pBuf, pShader);
        m_bSuccessful &= (m_hResult == S_OK);
      }
      break;
    case eRC_CreateREPostProcess:
      {
        CRendElementBase **pRE = ReadCommand<CRendElementBase**>(n);
        gRenDev->RT_CreateREPostProcess(pRE);
      }
      break;

    case eRC_DrawDynVB:
      {
        pP = &m_Commands[m_nCurThreadProcess][0];
        uint32 nSize = *(uint32 *)&pP[n];
        SVF_P3F_C4B_T2F *pBuf = (SVF_P3F_C4B_T2F *)&pP[n+4];
        n += nSize+4;
        nSize = *(uint32 *)&pP[n];
        uint16 *pInds = (uint16 *)&pP[n+4];
        n += nSize+4;
        int nVerts = ReadCommand<int>(n);
        int nInds = ReadCommand<int>(n);
        int nPrimType = ReadCommand<int>(n);
        gRenDev->RT_DrawDynVB(pBuf, pInds, nVerts, nInds, nPrimType);
      }
      break;
    case eRC_Draw2dImage:
      {
        float xpos = ReadCommand<float>(n);
        float ypos = ReadCommand<float>(n);
        float w = ReadCommand<float>(n);
        float h = ReadCommand<float>(n);
        int nTexID = ReadCommand<int>(n);
        float s0 = ReadCommand<float>(n);
        float t0 = ReadCommand<float>(n);
        float s1 = ReadCommand<float>(n);
        float t1 = ReadCommand<float>(n);
        float angle = ReadCommand<float>(n);
        int col = ReadCommand<int>(n);
        float z = ReadCommand<float>(n);
        gRenDev->RT_Draw2dImage(xpos, ypos, w, h, nTexID, s0, t0, s1, t1, angle, col, z);
      }
      break;
    case eRC_DrawImageWithUV:
      {
        pP = &m_Commands[m_nCurThreadProcess][n];
        gRenDev->RT_DrawImageWithUV(*(float *)pP, // xpos
          *(float *)&pP[4],  // ypos
          *(float *)&pP[8],  // z
          *(float *)&pP[12],  // w
          *(float *)&pP[16], // h
          *(int *)&pP[20],   // textureid
          (float *)&pP[24], // s
          (float *)&pP[40], // t
          *(int *)&pP[56]);   // col
        n += 60;
      }
      break;
		case eRC_SetCamera:
			{
				Matrix44A ProjMat = ReadCommand<Matrix44>(n);
				Matrix44A ViewMat = ReadCommand<Matrix44>(n);
				Matrix44A CameraZeroMat = ReadCommand<Matrix44>(n);

				gRenDev->SetMatrices(ProjMat.GetData(), ViewMat.GetData());
				gRenDev->m_CameraZeroMatrix[m_nCurThreadProcess] = CameraZeroMat;

				gRenDev->RT_SetCameraInfo();
			}
			break;
#if defined (DIRECT3D10) || defined(PS3)
    case eRC_ReleaseCB_SI:
      {
        int nID = ReadCommand<int>(n);
        CHWShader::mfReleaseCB_SI(nID);
      }
      break;
    case eRC_ReleaseCB:
      {
        void *pCB = ReadCommand<void *>(n);
        gRenDev->RT_ReleaseCB(pCB);
      }
      break;
#endif
    case eRC_ReleaseVBStream:
      {
        void* pVB = ReadCommand<void*>(n);
        int nStream = ReadCommand<int>(n);
        gRenDev->RT_ReleaseVBStream(pVB, nStream);
      }
      break;
    case eRC_ReleaseVB:
      {
        int nID = ReadCommand<int>(n);
        gRenDev->m_DevBufMan.ReleaseVB(nID);
      }
      break;
    case eRC_ReleaseIB:
      {
        int nID = ReadCommand<int>(n);
        gRenDev->m_DevBufMan.ReleaseIB(nID);
      }
      break;
    case eRC_VideoRender:
      {
        IVideoPlayer_RenderProxy* pVideo = ReadCommand<IVideoPlayer_RenderProxy*>(n);
        pVideo->RenderCallback();
      }
      break;
    case eRC_VideoReleaseTextures:
      {
        IVideoPlayer_RenderProxy* pVideo = ReadCommand<IVideoPlayer_RenderProxy*>(n);
        pVideo->DestroyTexturesCallback();
      }
      break;
    case eRC_RenderScene:
      {
        int nFlags = ReadCommand<int>(n);
        SThreadInfo *pTI = (SThreadInfo *)&m_Commands[m_nCurThreadProcess][n];
        n += sizeof(SThreadInfo);
        RenderFunc pRenderFunc = ReadCommand<RenderFunc>(n);
        SRenderListDesc *pRLD = (SRenderListDesc *)&m_Commands[m_nCurThreadProcess][n];
        n += sizeof(SRenderListDesc);
        int nR = ReadCommand<int>(n);
        int nROld = SRendItem::m_RecurseLevel[m_nCurThreadProcess];
        SRendItem::m_RecurseLevel[m_nCurThreadProcess] = nR;
        gRenDev->RT_RenderScene(nFlags, *pTI, pRenderFunc, pRLD);
        SRendItem::m_RecurseLevel[m_nCurThreadProcess] = nROld;
      }
      break;
    case eRC_PreprGenerateFarTrees:
      {
        CREFarTreeSprites* pRE = ReadCommand<CREFarTreeSprites*>(n);
        gRenDev->GenerateObjSprites(pRE->m_arrVegetationSprites[gRenDev->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID]]);
      }
      break;
    case eRC_TexStreamComplete:
      {
        IReadStream* pStream = ReadCommand<IReadStream*>(n);
        CTexture::StreamOnComplete(pStream);
      }
      break;
    case eRC_PreprGenerateCloud:
      {
        CRECloud* pRE = ReadCommand<CRECloud*>(n);
        CShader* pShader = ReadCommand<CShader*>(n);
        SRenderShaderResources* pRes = ReadCommand<SRenderShaderResources*>(n);
        CRenderObject* pObject = ReadCommand<CRenderObject*>(n);
        pRE->GenerateCloudImposter(pShader, pRes, pObject);
      }
      break;
    case eRC_DynTexUpdate:
      {
        SDynTexture* pTex = ReadCommand<SDynTexture*>(n);
        int nNewWidth = ReadCommand<int>(n);
        int nNewHeight = ReadCommand<int>(n);
        pTex->RT_Update(nNewWidth, nNewHeight);
      }
      break;
    case eRC_DynTexSourceUpdate:
      {
        IDynTextureSource *pSrc = ReadCommand<IDynTextureSource *>(n);
        float fDistance = ReadCommand<float>(n);
        pSrc->Update(fDistance);
      }
      break;
    case eRC_PushFog:
      gRenDev->EF_PushFog();
      break;
    case eRC_PopFog:
      gRenDev->EF_PopFog();
      break;
    case eRC_PushVP:
      gRenDev->FX_PushVP();
      break;
    case eRC_PopVP:
      gRenDev->FX_PopVP();
      break;
    case eRC_ClearRT:
      {
        int nFlags = ReadCommand<int>(n);
        ColorF vCol = ReadCommand<ColorF>(n);
        float fDepth = ReadCommand<float>(n);
        gRenDev->EF_ClearBuffers(nFlags, &vCol, fDepth);
      }
      break;
    case eRC_FlushTextMessages:
      gRenDev->RT_FlushTextMessages();
      break;
    case eRC_SetEnvTexRT:
      {
        SEnvTexture* pTex = ReadCommand<SEnvTexture*>(n);
        int nWidth = ReadCommand<int>(n);
        int nHeight = ReadCommand<int>(n);
        int bPush = ReadCommand<int>(n);
        pTex->m_pTex->RT_SetRT(0, nWidth, nHeight, bPush);
      }
      break;
		case eRC_SetEnvTexMatrix:
			{
				SEnvTexture* pTex = ReadCommand<SEnvTexture*>(n);
				pTex->RT_SetMatrix();
			}
			break;
    case eRC_PushRT:
      {
        int nTarget = ReadCommand<int>(n);
        CTexture* pTex = ReadCommand<CTexture*>(n);
        SD3DSurface* pDS = ReadCommand<SD3DSurface*>(n);
        int nS = ReadCommand<int>(n);
        gRenDev->RT_PushRenderTarget(nTarget, pTex, pDS, nS);
      }
      break;
    case eRC_PopRT:
      {
        int nTarget = ReadCommand<int>(n);
        gRenDev->RT_PopRenderTarget(nTarget);
      }
      break;
    case eRC_EntityDelete:
      {
        IRenderNode* pRN = ReadCommand<IRenderNode*>(n);
        SDynTexture_Shadow::RT_EntityDelete(pRN);
      }
      break;
    case eRC_SetViewport:
      {
        int nX = ReadCommand<int>(n);
        int nY = ReadCommand<int>(n);
        int nWidth = ReadCommand<int>(n);
        int nHeight = ReadCommand<int>(n);
        gRenDev->RT_SetViewport(nX, nY, nWidth, nHeight);
      }
      break;
    case eRC_TexBlurAnisotropicVertical:
      {
        CTexture* pTex = ReadCommand<CTexture *>(n);
        float fAnisoScale = ReadCommand<float>(n);
        TexBlurAnisotropicVertical(pTex, 1, 8 * max( 1.0f - min( fAnisoScale / 100.0f, 1.0f), 0.2f), 1, false);
      }
      break;

    case eRC_OC_ReadResult_Try:
      {
        CREOcclusionQuery *pRE = ReadCommand<CREOcclusionQuery *>(n);
        pRE->RT_ReadResult_Try();
      }
      break;

		case eRC_CGCSetLayers:
			{
				IColorGradingControllerInt* pController = ReadCommand<IColorGradingControllerInt*>(n);
				uint32 numLayers = ReadCommand<uint32>(n);
				const SColorChartLayer* pLayers = numLayers ? (const SColorChartLayer*) &m_Commands[m_nCurThreadProcess][n] : 0;
				n += sizeof(SColorChartLayer) * numLayers;
				pController->RT_SetLayers(pLayers, numLayers);
			}
			break;

    default:
      {
        assert(0);
      }
      break;
    }
  }
  SignalFlushFinishedCond();
#endif//STRIP_RENDER_THREAD
}
#pragma warning(pop)

void SRenderThread::Process()
{
  while(true)
  {
    float fTime = iTimer->GetAsyncCurTime();
		WaitFlushCond();
		if(m_bQuit)	
		{
			SignalFlushFinishedCond();
			break;//put it here to safely shut down
		}
		float fTimeAfterWait = iTimer->GetAsyncCurTime();
		gRenDev->m_fTimeWaitForMain[m_nCurThreadProcess] += fTimeAfterWait - fTime;
    ProcessCommands();
		float fTimeAfterProcess = iTimer->GetAsyncCurTime();
		gRenDev->m_fTimeProcessedRT[m_nCurThreadFill] += fTimeAfterProcess - fTimeAfterWait;
  }



}

#ifndef STRIP_RENDER_THREAD
// Flush current frame and wait for result
void SRenderThread::FlushAndWait()
{
  FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_RENDERER,g_bProfilerEnabled );

  if (!m_pThread)
    return;

	WaitFlushFinishedCond();

  int nCurProcess = m_nCurThreadProcess;
  m_nCurThreadProcess = m_nCurThreadFill;
  gRenDev->m_RP.m_nProcessThreadID = m_nCurThreadProcess;
  SignalFlushCond();

	WaitFlushFinishedCond();

	m_nCurThreadProcess = nCurProcess;
  gRenDev->m_RP.m_nProcessThreadID = m_nCurThreadProcess;
  m_Commands[m_nCurThreadFill].SetUse(0);
}
#endif//STRIP_RENDER_THREAD

// Flush current frame without waiting (should be called from main thread)
void SRenderThread::FlushFrame()
{
  FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_RENDERER,g_bProfilerEnabled );

  if (!IsMultithreaded())
    return;
#ifndef STRIP_RENDER_THREAD
//	gRenDev->FlushMainThreadAuxGeomCB();
  float fTime = iTimer->GetAsyncCurTime();
	WaitFlushFinishedCond();
	gRenDev->m_fTimeWaitForRender[m_nCurThreadFill] = iTimer->GetAsyncCurTime() - fTime;
//	gRenDev->ToggleMainThreadAuxGeomCB();
  gRenDev->m_RP.m_TI[m_nCurThreadProcess].m_nFrameUpdateID = gRenDev->m_RP.m_TI[m_nCurThreadFill].m_nFrameUpdateID;
  gRenDev->m_RP.m_TI[m_nCurThreadProcess].m_nFrameID = gRenDev->m_RP.m_TI[m_nCurThreadFill].m_nFrameID;
  m_nCurThreadProcess = m_nCurThreadFill;
  m_nCurThreadFill = (m_nCurThreadProcess+1) & 1;
  gRenDev->m_RP.m_nProcessThreadID = m_nCurThreadProcess;
  gRenDev->m_RP.m_nFillThreadID = m_nCurThreadFill;
  m_Commands[m_nCurThreadFill].SetUse(0);
	gRenDev->m_fTimeProcessedRT[m_nCurThreadFill] = 0;
	gRenDev->m_fTimeWaitForMain[m_nCurThreadProcess] = 0;

  SignalFlushCond();
#endif
}

void SRenderThread::Quit()
{
  if (IsMultithreaded())
  {
		SignalQuitCond();
    while (m_pThread->IsRunning()) 
    {
#if defined(USE_LOCKS_FOR_FLUSH_SYNC)
      FlushAndWait();
#endif 
    }
		m_pThread->WaitForThread();
    SAFE_DELETE(m_pThread);
  }
  m_bQuit = 1;
}

bool SRenderThread::IsFailed()
{
  return !m_bSuccessful;
}

#undef m_nCurThreadFill
#undef m_nCurThreadProcess
