
/*=============================================================================
RenderThread.h : Render thread commands processing.
Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
  * Created by Honitch Andrey
  
=============================================================================*/

#ifndef __RENDERTHREAD_H__
#define __RENDERTHREAD_H__

#define RENDER_THREAD_NAME "RenderThread"

typedef void (*RenderFunc)(void);

//====================================================================

struct IFlashPlayer_RenderProxy;
struct IVideoPlayer_RenderProxy;
struct IRenderAuxGeomImpl;
struct SAuxGeomCBRawDataPackaged;
struct IColorGradingControllerInt;
struct SColorChartLayer;
class CRenderMesh2;





enum ERenderCommand
{
  eRC_Unknown = 0,
  eRC_Init,
  eRC_CreateDevice,
  eRC_ResetDevice,
  eRC_BeginFrame,
  eRC_EndFrame,
  eRC_ClearBuffers,
  eRC_FlushTextMessages,
  eRC_PreloadTextures,
  eRC_ReadFrameBuffer,

  eRC_DrawStringW,
  eRC_UpdateTexture,
  eRC_UpdateMesh2,
  eRC_ReleaseIB,
  eRC_ReleaseVB,
  eRC_ReleaseCB,
  eRC_ReleaseCB_SI,
  eRC_ReleaseVBStream,
  eRC_CreateResource,
  eRC_ReleaseResource,
  eRC_CreateDeviceTexture,
  eRC_CreateVertexBuffer,
  eRC_CreateIndexBuffer,
  eRC_CreatePixelShader,
  eRC_CreateVertexShader,
  eRC_CreateREPostProcess,
  eRC_LoadShaderItem,
  eRC_VideoRender,
  eRC_VideoReleaseTextures,
  eRC_ReleaseDeviceTexture,
  eRC_FlashRender,
  eRC_FlashRemoveTexture,
  eRC_AuxFlush,
  eRC_RenderScene,
  eRC_SetCamera,

  eRC_SetState,
  eRC_SetCull,

  eRC_DrawDynVB,
  eRC_Draw2dImage,
  eRC_DrawImageWithUV,

  eRC_PreprGenerateFarTrees,
  eRC_PreprGenerateCloud,
  eRC_DynTexUpdate,
  eRC_DynTexSourceUpdate,
  eRC_PushFog,
  eRC_PopFog,
  eRC_PushVP,
  eRC_PopVP,
  eRC_ClearRT,
  eRC_SetEnvTexRT,
	eRC_SetEnvTexMatrix,
  eRC_PushRT,
  eRC_PopRT,
  eRC_SetViewport,
  eRC_TexStreamComplete,
  eRC_TexBlurAnisotropicVertical,
  eRC_UpdateMaterialConstants,

  eRC_OC_ReadResult_Try,

	eRC_CGCSetLayers,
  eRC_EntityDelete,

	// streaming queries
	eRC_PrecacheTexture,
  eRC_SetTexture,
};

//====================================================================

#ifdef STRIP_RENDER_THREAD
	#define m_nCurThreadFill 0
	#define m_nCurThreadProcess 0
	#define COMMAND_BUF_COUNT 1
#else
	#define COMMAND_BUF_COUNT 2
#endif

struct CRenderThread : CrySimpleThread<>
{
  int m_nCPU;
  CRenderThread(int nCPU)
  {	
    m_nCPU = CLAMP(nCPU, 1, 5);
  }

  virtual ~CRenderThread()
  {
    // Stop thread task.
    Stop();
  }

	CryEvent	m_started;

protected:
  virtual void Run();
};







struct SRenderThread
{
  CRenderThread *m_pThread;
  bool m_bQuit;
  bool m_bSuccessful;
#ifndef STRIP_RENDER_THREAD
  int m_nCurThreadProcess;
  int m_nCurThreadFill;
#endif
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
	int m_nFlush;
	CryMutex							m_LockFlushNotify;
	CryConditionVariable	m_FlushCondition;
	CryConditionVariable	m_FlushFinishedCondition;
#else
	volatile int m_nFlush;
#endif
  uint32 m_nRenderThread;
  uint32 m_nMainThread;
  HRESULT m_hResult;
  TArray<byte> m_Commands[COMMAND_BUF_COUNT]; // m_nCurThreadFill shows which commands are filled by main thread

  SRenderThread();
  ~SRenderThread();
#ifndef __SPU__
	inline void SignalFlushFinishedCond()
	{
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
		m_LockFlushNotify.Lock();
#endif
		m_nFlush = 0;
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
		m_FlushFinishedCondition.Notify();
		m_LockFlushNotify.Unlock();
#endif
	}

	inline void SignalFlushCond()
	{
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
		m_LockFlushNotify.Lock();
#endif
		m_nFlush = 1;
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
		m_FlushCondition.Notify();
		m_LockFlushNotify.Unlock();
#endif
	}

	inline void SignalQuitCond()
	{
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
		m_LockFlushNotify.Lock();
#endif
		m_bQuit = 1;
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
		m_FlushCondition.Notify();
		m_LockFlushNotify.Unlock();
#endif
	}

	inline void WaitFlushCond()
	{
    FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_RENDERER,g_bProfilerEnabled );

#ifdef USE_LOCKS_FOR_FLUSH_SYNC
		m_LockFlushNotify.Lock();
    while (!*(volatile int*)&m_nFlush)
			m_FlushCondition.Wait(m_LockFlushNotify);
		m_LockFlushNotify.Unlock();
#else
    while (!*(volatile int*)&m_nFlush)
    {
      if (m_bQuit)
        break;
      ::Sleep(0);
    }
#endif
	}

#ifdef WIN32
	HWND GetRenderWindowHandle();
#endif

	inline void WaitFlushFinishedCond()
	{
#ifdef USE_LOCKS_FOR_FLUSH_SYNC
		m_LockFlushNotify.Lock();
		while(*(volatile int*)&m_nFlush)
			m_FlushFinishedCondition.Wait(m_LockFlushNotify);
		m_LockFlushNotify.Unlock();
#else
		while(*(volatile int*)&m_nFlush)
		{
		#ifdef WIN32
			if (GetRenderWindowHandle())
			{
				MSG msg;
				while (PeekMessage(&msg, GetRenderWindowHandle(), 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		#endif



		}
#endif
	}

	inline void InitFlushCond()
	{
		m_nFlush = 0;
	}

	inline bool CheckFlushCond()
	{
		return *(int*)&m_nFlush != 0;
	}
#endif//__SPU__

	void Start()
	{
		if (m_pThread != NULL)
		{
			m_pThread->Start(2, RENDER_THREAD_NAME, THREAD_PRIORITY_NORMAL, 72*1024);
			m_pThread->m_started.Wait();
		}
	}

  bool IsFailed();
  _inline void AddCommand(ERenderCommand eRC)
  {
#ifdef RENDERTHREAD_ALIGN_COMMANDS
		AddDWORD(eRC);
#else
    m_Commands[m_nCurThreadFill].AddElem(eRC);
#endif
  }
  _inline void AddDWORD(uint32 nVal)
  {
    *(uint32 *)(m_Commands[m_nCurThreadFill].Grow(sizeof(uint32))) = nVal;
  }
  _inline void AddDWORD64(uint64 nVal)
  {
    *(uint64 *)(m_Commands[m_nCurThreadFill].Grow(sizeof(uint64))) = nVal;
  }
  _inline void AddTI(SThreadInfo& TI)
  {
    SThreadInfo *pTI = (SThreadInfo *)m_Commands[m_nCurThreadFill].Grow(sizeof(SThreadInfo));
    memcpy(pTI, &TI, sizeof(SThreadInfo));
  }
  _inline void AddRLD(SRenderListDesc* pInRLD)
  {
    SRenderListDesc *pRLD = (SRenderListDesc *)m_Commands[m_nCurThreadFill].Grow(sizeof(SRenderListDesc));
    memcpy(pRLD, pInRLD, sizeof(SRenderListDesc));
  }
  _inline void AddFloat(const float fVal)
  {
    *(float *)(m_Commands[m_nCurThreadFill].Grow(sizeof(float))) = fVal;
  }
  _inline void AddVec3(const Vec3& cVal)
  {
    Vec3 *vData = (Vec3 *)m_Commands[m_nCurThreadFill].Grow(sizeof(Vec3));
    *vData = cVal;
  }
  _inline void AddColor(const ColorF& cVal)
  {
    float *fData = (float *)m_Commands[m_nCurThreadFill].Grow(sizeof(ColorF));
    fData[0] = cVal[0];
    fData[1] = cVal[1];
    fData[2] = cVal[2];
    fData[3] = cVal[3];
  }
  _inline void AddColorB(const ColorB& cVal)
  {
    byte *fData = (byte *)m_Commands[m_nCurThreadFill].Grow(sizeof(ColorB));
    fData[0] = cVal[0];
    fData[1] = cVal[1];
    fData[2] = cVal[2];
    fData[3] = cVal[3];
  }
  _inline void AddPointer(const void *pVal)
  {
    *(const void **)(m_Commands[m_nCurThreadFill].Grow(sizeof(void *))) = pVal;
  }
  _inline void AddData(const void *pData, int nLen)
  {
#ifdef RENDERTHREAD_ALIGN_COMMANDS
    unsigned pad = (unsigned)-nLen % 4;
#else
    unsigned pad = 0;
#endif
    AddDWORD(nLen + pad);
    byte *pDst = m_Commands[m_nCurThreadFill].Grow(nLen + pad);
    memcpy(pDst, pData, nLen);
  }
  _inline void AddText(const char *pText)
  {
    int nLen = strlen(pText)+1;
    AddDWORD(nLen);
#ifdef RENDERTHREAD_ALIGN_COMMANDS
		unsigned pad = (unsigned)-nLen % 4;
#else
		unsigned pad = 0;
#endif
    byte *pData = m_Commands[m_nCurThreadFill].Grow(nLen + pad);
    memcpy(pData, pText, nLen);
  }
  _inline void AddText(const wchar_t *pText)
  {
    int nLen = (wcslen(pText)+1) * sizeof(wchar_t);
    AddDWORD(nLen);
#ifdef RENDERTHREAD_ALIGN_COMMANDS
		unsigned pad = (unsigned)-nLen % 4;
#else
		unsigned pad = 0;
#endif
    byte *pData = m_Commands[m_nCurThreadFill].Grow(nLen + pad);
    memcpy(pData, pText, nLen);
  }
  template<class T> T ReadCommand(int& nIndex)
  {
    T Res = *(T*)&m_Commands[m_nCurThreadProcess][nIndex];
    nIndex += sizeof(T);
    return Res;
  }
  
  _inline int GetCurrentThreadId(bool bAlwaysCheck=false)
  {
#ifdef STRIP_RENDER_THREAD
		return m_nRenderThread;
#else
    if (!bAlwaysCheck && m_nRenderThread == m_nMainThread)
      return m_nRenderThread;
    return ::GetCurrentThreadId();
#endif
  }

  void Init(int nCPU);
  void Quit();
  void FlushFrame();
  void FlushAndWait();
  void ProcessCommands();
  void Process();
  int  GetThreadList();
  bool IsRenderThread(bool bAlwaysCheck=false);
  bool IsMainThread(bool bAlwaysCheck=false);
  bool IsMultithreaded();
	int	 CurThreadFill() const;

  void    RC_Init();
  bool    RC_CreateDevice();
  void    RC_ResetDevice();
  void    RC_PreloadTextures();
  void    RC_ReadFrameBuffer(unsigned char * pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY);
  HRESULT RC_CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, UINT Pool, void** ppVertexBuffer, HANDLE* pSharedHandle);
  HRESULT RC_CreateIndexBuffer(UINT Length, DWORD Usage, DWORD Format, UINT Pool, void** ppIndexBuffer, HANDLE* pSharedHandle);
  HRESULT RC_CreatePixelShader(DWORD *pBuf, void **pShader);
  HRESULT RC_CreateVertexShader(DWORD *pBuf, void **pShader, void *pInst);
  bool    RC_CreateDeviceTexture(CTexture *pTex, byte *pData[6]);
  void    RC_CreateResource(SResourceAsync* pRes);
  void    RC_ReleaseResource(SResourceAsync* pRes);
  void    RC_CreateREPostProcess(CRendElementBase **re);
  bool    RC_CheckUpdate2(CRenderMesh2 *pMesh, CRenderMesh2 *pVContainer, EVertexFormat eVF, uint32 nStreamMask);
  void    RC_ReleaseCB_SI(int nID);
  void    RC_ReleaseCB(void *pCB);
  void    RC_ReleaseVB(int nID);
  void    RC_ReleaseIB(int nID);
  void    RC_DrawDynVB(SVF_P3F_C4B_T2F *pBuf, uint16 *pInds, int nVerts, int nInds, int nPrimType);
  void    RC_Draw2dImage(float xpos,float ypos,float w,float h,int textureid,float s0,float t0,float s1,float t1,float angle,float r,float g,float b,float a,float z);
  void    RC_DrawImageWithUV(float xpos,float ypos,float z,float w,float h,int texture_id,float s[4],float t[4],float r,float g,float b,float a);
  void    RC_UpdateTextureRegion(CTexture *pTex, byte *data, int nX, int nY, int USize, int VSize, ETEX_Format eTFSrc);
  void    RC_TexStreamComplete(IReadStream* pStream);
  void    RC_SetState(int State, int AlphaRef);
  void    RC_SetCull(int nMode);
  void    RC_DrawStringW(IFFont *pFont, float fBaseX, float fBaseY, float fBaseZ, const wchar_t *szMsg, const bool bASCIIMultiLine);
  bool    RC_VideoRender(IVideoPlayer_RenderProxy* pVideo);
  void    RC_VideoReleaseTextures(IVideoPlayer_RenderProxy* pVideo);
  void    RC_ReleaseDeviceTexture(CTexture *pTex);
	void		RC_PrecacheResource(ITexture *pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId);
  void    RC_ClearBuffers(uint32 nFlags, ColorF *vColor, float depth);
  void    RC_FlushTextMessages();
  void    RC_AuxFlush(IRenderAuxGeomImpl* pAux, const SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end);
  void    RC_FlashRender(IFlashPlayer_RenderProxy* pPlayer, bool stereo);
  void    RC_FlashRemoveTexture(CTexture* pTexture);
  void    RC_SetCamera();
  void    RC_RenderScene(int nFlags, RenderFunc pRenderFunc, SRenderListDesc* pRLD);
  void    RC_BeginFrame();
  void    RC_EndFrame();
  bool    RC_DynTexUpdate(SDynTexture *pTex, int nNewWidth, int nNewHeight);
  bool    RC_DynTexSourceUpdate(IDynTextureSource *pSrc, float fDistance);
  void    RC_PushFog();
  void    RC_PopFog();
  void    RC_PushVP();
  void    RC_PopVP();
  void    RC_ClearRT(int nFlags, ColorF *pCol, float fDepth);
  void    RC_SetEnvTexRT(SEnvTexture *pEnvTex, int nWidth, int nHeight, bool bPush);
	void    RC_SetEnvTexMatrix(SEnvTexture *pEnvTex);
  void    RC_PushRT(int nTarget, CTexture *pTex, SD3DSurface *pDS, int nS);
  void    RC_PopRT(int nTarget);
  void    RC_TexBlurAnisotropicVertical(CTexture *Tex, float fAnisoScale);
  void    RC_EntityDelete(IRenderNode * pRenderNode);
  void    RC_UpdateMaterialConstants(SRenderShaderResources *pSR, IShader *pSH);
  void    RC_SetTexture(int nTex, int nUnit);

  bool    RC_OC_ReadResult_Try(CREOcclusionQuery *pRE);

  void    RC_PreprGenerateFarTrees(CREFarTreeSprites *pRE);
  void    RC_PreprGenerateCloud(CRendElementBase *pRE, CShader *pShader, SRenderShaderResources *pRes, CRenderObject *pObject);
  void    RC_SetViewport(int x, int y, int width, int height);

  void    RC_ReleaseVBStream(void *pVB, int nStream);
  SShaderItem RC_LoadShaderItem (const char *szName, bool bShare, int flags, SInputShaderResources *Res, uint64 nMaskGen);

	void RC_CGCSetLayers(IColorGradingControllerInt* pController, const SColorChartLayer* pLayers, uint32 numLayers);

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject( m_Commands[0] );
		pSizer->AddObject( m_Commands[1] );
	}
} _ALIGN(128);//align to cache line

_inline int SRenderThread::GetThreadList()
{
#ifdef STRIP_RENDER_THREAD
	return m_nCurThreadFill;
#else
	DWORD d = this->GetCurrentThreadId();
	if (d == m_nRenderThread)
		return m_nCurThreadProcess;
	return m_nCurThreadFill;
#endif
}

_inline bool SRenderThread::IsRenderThread(bool bAlwaysCheck)
{
#ifdef STRIP_RENDER_THREAD
	return true;
#else
	DWORD d = this->GetCurrentThreadId(bAlwaysCheck);
	if (d == m_nRenderThread)
		return true;
	return false;
#endif
}

_inline bool SRenderThread::IsMainThread(bool bAlwaysCheck)
{
#ifdef STRIP_RENDER_THREAD
	return false;
#else
	DWORD d = this->GetCurrentThreadId(bAlwaysCheck);
	if (d == m_nMainThread)
		return true;
	return false;
#endif
}

_inline bool SRenderThread::IsMultithreaded()
{
#ifdef STRIP_RENDER_THREAD
	return false;
#else
	return m_pThread != NULL;
#endif
}

_inline int SRenderThread::CurThreadFill() const
{
	return m_nCurThreadFill;
}

#ifdef STRIP_RENDER_THREAD
_inline void SRenderThread::FlushAndWait()
{
	return;
}
#endif

#undef COMMAND_BUF_COUNT
#undef m_nCurThreadFill
#undef m_nCurThreadProcess

#endif	// __RENDERTHREAD_H__

