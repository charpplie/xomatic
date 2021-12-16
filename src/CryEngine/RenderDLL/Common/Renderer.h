
//////////////////////////////////////////////////////////////////////
//
//	Crytek CryENGINE Source code
//	
//	File:Renderer.h - API Indipendent
//
//	History:
//	-Jan 31,2001:Originally Created by Marco Corbetta
//	-: taken over by Andrey Khonich
//
//////////////////////////////////////////////////////////////////////

#ifndef _RENDERER_H
#define _RENDERER_H

#if _MSC_VER > 1000
# pragma once
#endif


#include "TextMessages.h"															// CTextMessages

typedef void (PROCRENDEF)(SShaderPass *l, int nPrimType);

#define USE_HDR 1

#define USE_FX 1
#define USE_NATIVE_DEPTH 1

struct ICVar;
struct ShadowMapFrustum;
struct IStatObj;
struct SShaderPass;
class CREParticle;
struct IDirectBee;

typedef int (*pDrawModelFunc)(void);

//=============================================================

#define D3DRGBA(r, g, b, a) \
	(   (((int)((a) * 255)) << 24) | (((int)((r) * 255)) << 16) \
	|   (((int)((g) * 255)) << 8) | (int)((b) * 255) \
	)


//try_to_merge flags
#define TTM_NEEDLM						1
#define TTM_NEEDOCCLM					2
#define TTM_NEEDSHADOWCASTERS	4
#define TTM_NEEDMATERIAL			8
#define TTM_NEEDDYNLIHGTS     16

#if defined(DIRECT3D9) || defined(XENON)
#define HALF_PIXEL_SHIFT_NEEDED 1
#else
#define HALF_PIXEL_SHIFT_NEEDED 0
#endif

struct SSpriteInfo
{
	SDynTexture2 *m_pTex;
	struct SSectorTextureSet * m_pTerrainTexInfo;
	Vec3 m_vPos;
	float m_fDX;
	float m_fDY;
	float m_fScaleV;
	UCol m_Color;
	uint8	m_ucTexCoordMinX;			// 0..128 used for the full range (0..1) in the texture (to fit in byte)
	uint8	m_ucTexCoordMinY;			// 0..128 used for the full range (0..1) in the texture (to fit in byte)
	uint8	m_ucTexCoordMaxX;			// 0..128 used for the full range (0..1) in the texture (to fit in byte)
	uint8	m_ucTexCoordMaxY;			// 0..128 used for the full range (0..1) in the texture (to fit in byte)
};

struct SSpriteGenInfo
{
	float fAngle;								// horizontal rotation in degree
	float fGenDist;
	float fBrightnessMultiplier;
	int nMaterialLayers;
	IMaterial *pMaterial;
	float *pMipFactor;
	uint8 *pTexturesAreStreamedIn;
	SDynTexture2 **ppTexture;
	IStatObj *pStatObj;
	int nSP;
};

struct SObjsState
{
	int nNumVisObjects;
	int nNumTempObjects;
};

class CMatrixStack
{
public:
	CMatrixStack(uint32 maxDepth,uint32 dirtyFlag);
	~CMatrixStack();

	// Pops the top of the stack, returns the current top
	// *after* popping the top.
	bool Pop();

	// Pushes the stack by one, duplicating the current matrix.
	bool Push();

	// Loads identity in the current matrix.
	bool LoadIdentity();

	// Loads the given matrix into the current matrix
	bool LoadMatrix(const Matrix44* pMat );
	bool LoadMatrix(const Matrix44A* pMat );

	// Right-Multiplies the given matrix to the current matrix.
	// (transformation is about the current world origin)
	bool MultMatrix(const Matrix44* pMat );

	// Left-Multiplies the given matrix to the current matrix
	// (transformation is about the local origin of the object)
	bool MultMatrixLocal(const Matrix44* pMat );

	// Right multiply the current matrix with the computed rotation
	// matrix, counterclockwise about the given axis with the given angle.
	// (rotation is about the current world origin)
	bool RotateAxis	(const Vec3* pV, f32 Angle);

	// Left multiply the current matrix with the computed rotation
	// matrix, counterclockwise about the given axis with the given angle.
	// (rotation is about the local origin of the object)
	bool RotateAxisLocal(const Vec3* pV, f32 Angle);

	// Right multiply the current matrix with the computed rotation
	// matrix. All angles are counterclockwise. (rotation is about the
	// current world origin)

	// Right multiply the current matrix with the computed scale
	// matrix. (transformation is about the current world origin)
	bool Scale(f32 x, f32 y, f32 z);

	// Left multiply the current matrix with the computed scale
	// matrix. (transformation is about the local origin of the object)
	bool ScaleLocal(f32 x, f32 y, f32 z);

	// Right multiply the current matrix with the computed translation
	// matrix. (transformation is about the current world origin)
	bool Translate(f32 x, f32 y, f32 z);

	// Left multiply the current matrix with the computed translation
	// matrix. (transformation is about the local origin of the object)
	bool TranslateLocal(f32 x, f32 y, f32 z);

	// Obtain the current matrix at the top of the stack
	_inline Matrix44A* GetTop()
	{
		assert (m_pTop!=NULL);
		return m_pTop;
	}

	_inline int GetDepth() { return m_nDepth; }

private:
	Matrix44A *m_pTop;		//top of the stack
	Matrix44A *m_pStack; // array of Matrix44
	uint32 m_nDepth;
	uint32 m_nMaxDepth;
	uint32 m_nDirtyFlag; //flag for new matrices
};

//////////////////////////////////////////////////////////////////////
// 3D engine duplicated data for non-thread safe data

struct S3DEngineCommon
{
public:
	enum EVisAreaFlags
	{
		VAF_EXISTS_FOR_POSITION = (1<<0),
		VAF_CONNECTED_TO_OUTDOOR = (1<<1),
		VAF_AFFECTED_BY_OUT_LIGHTS = (1<<2),
		VAF_MASK = VAF_EXISTS_FOR_POSITION | VAF_CONNECTED_TO_OUTDOOR | VAF_AFFECTED_BY_OUT_LIGHTS
	};

	struct SVisAreaInfo
	{
		SVisAreaInfo():nFlags(0)
		{
		};
		uint32 nFlags;
	};

	SVisAreaInfo m_pCamVisAreaInfo;

	struct SOceanInfo
	{
		SOceanInfo(): m_nOceanRenderFlags(0), m_fWaterLevel(0.0f), m_vCausticsParams(0.0f, 0.0f, 0.0f, 0.0f) { };
		uint8 m_nOceanRenderFlags;
		float m_fWaterLevel;
		Vec4 m_vCausticsParams;
	};

	SOceanInfo m_OceanInfo;

public:

	void Update();
};


#if defined(ENABLE_GPU_TIMERS)
class GpuTimerEvent
{
public:

	GpuTimerEvent() : open(false), parent(NULL) {}

	void Init(const char* eventName) 
	{
		open = true;
		parent=NULL;
		startTime = 0;
		totalTime = 0;
		strncpy(name, eventName, min(64, strlen(eventName)));
	}

	void Reset() 
	{
		open = false;
		parent=NULL;
		startTime = 0;
		totalTime = 0;
		memset(name, 0, sizeof(name));
	}

	char name[64];
	uint32 startTime;
	uint32 totalTime;
	bool open;
	GpuTimerEvent* parent;

	//
	// Static data for gpu timers 
	//
	static const int s_numBuffers = 3;
	static const int s_maxNumEvents = 128;

	static GpuTimerEvent	s_events[s_numBuffers][s_maxNumEvents];
	static uint32					s_eventCount[s_numBuffers];
	static bool						s_outOfEvents[s_numBuffers];

	//current event for closing
	static GpuTimerEvent*	s_currentOpenEvent;

	static uint32					s_readIdx;
	static uint32					s_callbackIdx;
	static uint32					s_writeIdx;

	static void BeginTimerCallback(DWORD id);
	static void EndTimerCallback(DWORD id); 
	static void SwapTimersCallback(DWORD id); 
};
#endif

//////////////////////////////////////////////////////////////////////
class CRenderer : public IRenderer
{
	friend struct IDirect3D9;

public:	
	DEFINE_ALIGNED_DATA(Matrix44A, m_IdentityMatrix, 16);
	DEFINE_ALIGNED_DATA(Matrix44A, m_ViewMatrix, 16);
	DEFINE_ALIGNED_DATA(Matrix44A, m_CameraMatrix, 16);
	DEFINE_ALIGNED_DATA(Matrix44A, m_CameraZeroMatrix[2], 16);
	DEFINE_ALIGNED_DATA(Matrix44A, m_CameraMatrixPrev[2], 16); // todo: generalize for multiple cameras
	DEFINE_ALIGNED_DATA(Matrix44A, m_ProjMatrix, 16);
	DEFINE_ALIGNED_DATA(Matrix44A, m_TranspOrigCameraProjMatrix, 16);
	DEFINE_ALIGNED_DATA(Matrix44A, m_CameraProjMatrix, 16);  
	DEFINE_ALIGNED_DATA(Matrix44A, m_CameraProjZeroMatrix, 16);  
	DEFINE_ALIGNED_DATA(Matrix44A, m_InvCameraProjMatrix, 16);
	DEFINE_ALIGNED_DATA(Matrix44A, m_TempMatrices[4][8], 16);

	struct SRenderTileInfo { SRenderTileInfo() { nPosX=nPosY=nGridSizeX=nGridSizeY=0.f; } f32 nPosX, nPosY, nGridSizeX, nGridSizeY; };

	bool              m_bDeviceLost;
	bool              m_bAquireDeviceThread;

	SRenderThread *m_pRT;


	// Shaders pipeline states
	//=============================================================================================================
	CDeviceManager m_DevMan;
	CDevBufferMan  m_DevBufMan;
	SRenderPipeline m_RP;
	//=============================================================================================================

	float m_fTimeWaitForMain[2];
	float m_fTimeWaitForRender[2];
	float m_fTimeProcessedRT[2];
	float m_fTimeWaitForGPU[2];
	float m_fTimeGPUIdlePercent[2];

	int              m_CurVertBufferSize;
	int              m_CurIndexBufferSize;

	SVF_P3F_C4B_T2F m_DynVB[2048];
	SVF_P3F_C4B_T2F *m_pTempDynVB;

	int  m_VSync;
	int  m_Predicated;
	int  m_FSAA;
	int  m_FSAA_quality;
	int  m_FSAA_samples;
	int		m_deskwidth, m_deskheight, m_deskbpp, m_singleMonitorWidth, m_singleMonitorHeight;
	int  m_nHDRType;

	int m_nStencilMaskRef;

	byte  m_bDeviceSupportsATOC;
	byte  m_bDeviceSupportsInstancing;

	uint32  m_bDeviceSupports_ATI_DF24 : 1;
	uint32  m_bDeviceSupports_ATI_DF16 : 1;
	uint32  m_bDeviceSupports_NV_D24S8_SM : 1;

	uint32  m_bDeviceSupports_NVDBT : 1;

	uint32  m_bDeviceSupportsG16R16Filter : 1;
	uint32  m_bDeviceSupportsFP16Separate : 1;
	uint32  m_bDeviceSupportsFP16Filter : 1;
	uint32  m_bDeviceSupportsDynBranching : 1;
	uint32  m_bDeviceSupportsR16FRendertarget : 1;
	uint32  m_bDeviceSupports_G16R16_FSAA : 1;
	uint32  m_bDeviceSupports_A16B16G16R16_FSAA : 1;
	uint32  m_bDeviceSupports_A2B10G10R10F : 1;
	uint32  m_bDeviceSupportsVertexTexture : 1;
	uint32  m_bDeviceSupportsR2VB : 1;

	uint32 m_bEditor : 1; // Render instance created from editor
	uint32 m_bUseHWSkinning : 1;
	uint32 m_bUseDynBranching : 1;
	uint32 m_bShadersPresort : 1;
	uint32 m_bEnd : 1;
	uint32 m_bUsePOM : 1;
	uint32 m_bInResolutionChange : 1;
	uint32 m_bWaterCaustics : 1;

	ICVar *m_CVWidth;
	ICVar *m_CVHeight;
	ICVar *m_CVFullScreen;
	ICVar *m_CVColorBits;
	ICVar *m_CVDisplayInfo;

	ColorF m_CurFontColor;

	SFogState m_FSStack[8];
	int m_nCurFSStackLevel;

	SObjsState m_ObjsStack[16];

	byte m_iDeviceSupportsComprNormalmaps; // 0:none(DXT5 will be used) 1: 3DC, 2: V8U8, 3: CxV8U8

	DWORD m_Features;
	int m_MaxTextureSize;
	size_t m_MaxTextureMemory;
	int m_nShadowTexSize;

	unsigned short m_GammaTable[256];
	float m_fLastGamma;
	float m_fLastBrightness;
	float m_fLastContrast;
	float m_fDeltaGamma;

	static Vec2 s_overscanBorders;

protected:

	typedef std::list<IRendererEventListener *> TListRendererEventListeners;
	TListRendererEventListeners m_listRendererEventListeners;

public:

	CRenderer();
	virtual ~CRenderer();
	void PostInit();

	// Multithreading support


  virtual void DrawStringW(IFFont *pFont, float fBaseX, float fBaseY, float fBaseZ, const wchar_t *szMsg, const bool bASCIIMultiLine);


	virtual int  RT_CurThreadList();
	virtual void RT_BeginFrame()=0;
	virtual void RT_EndFrame()=0;
	virtual void RT_Init()=0;
	virtual bool RT_CreateDevice()=0;
	virtual void RT_Reset()=0;
	virtual void RT_FlushTextMessages();
	virtual void RT_SetCull(int nMode)=0;
	virtual void RT_RenderScene(int nFlags, SThreadInfo& TI, RenderFunc pRenderFunc, SRenderListDesc* pRLD)=0;
	virtual void RT_SetCameraInfo()=0;
	virtual void RT_CreateResource(SResourceAsync* pRes)=0;
	virtual void RT_ReleaseResource(SResourceAsync* pRes)=0;
	virtual void RT_ReadFrameBuffer(unsigned char * pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY)=0;
	virtual void RT_CreateREPostProcess(CRendElementBase **re);
	virtual void RT_FlashRender(IFlashPlayer_RenderProxy* pPlayer, bool stereo);
	virtual void RT_FlashRemoveTexture(ITexture* pTexture);
	
	virtual void RT_DrawStringW(IFFont *pFont, float fBaseX, float fBaseY, float fBaseZ, const wchar_t *szMsg, const bool bASCIIMultiLine);
	virtual HRESULT RT_CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, UINT Pool, void** ppVertexBuffer, HANDLE* pSharedHandle) = 0;
	virtual HRESULT RT_CreateIndexBuffer(UINT Length, DWORD Usage, DWORD Format, UINT Pool, void** ppVertexBuffer, HANDLE* pSharedHandle) = 0;
	virtual HRESULT RT_CreateVertexShader(DWORD *pBuf, void **pShader, void *pInst) = 0;
	virtual HRESULT RT_CreatePixelShader(DWORD *pBuf, void **pShader) = 0;
	virtual void RT_ReleaseVBStream(void *pVB, int nStream) = 0;
	virtual void RT_ReleaseCB(void *pCB) = 0;
	virtual void RT_DrawDynVB(int nOffs, int Pool, int nVerts) = 0;
	virtual void RT_DrawDynVB(SVF_P3F_C4B_T2F *pBuf, uint16 *pInds, int nVerts, int nInds, int nPrimType) = 0;
	virtual void RT_Draw2dImage(float xpos,float ypos,float w,float h,int textureid,float s0,float t0,float s1,float t1,float angle,DWORD col,float z) = 0;
	virtual void RT_DrawImageWithUV(float xpos,float ypos,float z,float w,float h,int texture_id,float *s,float *t,DWORD col)=0;
	virtual void RT_PushRenderTarget(int nTarget, CTexture *pTex, SD3DSurface *pDS, int nS)=0;
	virtual void RT_PopRenderTarget(int nTarget)=0;
	virtual	void RT_SetViewport(int x, int y, int width, int height)=0;

	virtual bool GetOcclusionBuffer(uint16* pOutOcclBuffer, int32 nSizeX, int32 nSizeY, Matrix44* pmViewProj) = 0;

	virtual void EF_ClearBuffers(uint32 nFlags, const ColorF *Colors, float fDepth=1.0f)=0;

	//===============================================================================

	virtual void AddListener		(IRendererEventListener *pRendererEventListener);
	virtual void RemoveListener	(IRendererEventListener *pRendererEventListener);

	virtual ERenderType GetRenderType() const;

	virtual WIN_HWND Init(int x,int y,int width,int height,unsigned int cbpp, int zbpp, int sbits, bool fullscreen,WIN_HINSTANCE hinst, WIN_HWND Glhwnd=0, bool bReInit=false, const SCustomRenderInitArgs* pCustomArgs=0)=0;

	virtual WIN_HWND GetCurrentContextHWND() {	return GetHWND();	};

	virtual int  CreateRenderTarget (int nWidth, int nHeight, ETEX_Format eTF=eTF_A8R8G8B8)=0;
	virtual bool DestroyRenderTarget (int nHandle)=0;
	virtual bool SetRenderTarget (int nHandle, int nFlags=0)=0;

	virtual int GetFeatures() {return m_Features;}
	virtual int GetCurrentNumberOfDrawCalls()
	{
		int nDIPs = 0;
		int nThr = m_pRT->GetThreadList();
		for (int i=0; i<EFSLIST_NUM; i++)
		{
			nDIPs += m_RP.m_PS[nThr].m_nDIPs[i];
		}
		return nDIPs;
	}

	virtual void SetDebugRenderNode(IRenderNode* pRenderNode)
	{
		m_pDebugRenderNode = pRenderNode;
	}

	virtual bool IsDebugRenderNode(IRenderNode* pRenderNode) const 
	{ 
		return (m_pDebugRenderNode && m_pDebugRenderNode==pRenderNode); 
	}

	//! Fills array of all supported video formats (except low resolution formats)
	//! Returns number of formats, also when called with NULL
	virtual int	EnumDisplayFormats(SDispFormat *formats)=0;

	//! Return all supported by video card video AA formats
	virtual int	EnumAAFormats(const SDispFormat &rDispFmt, SAAFormat* formats)=0;

	//! Changes resolution of the window/device (doen't require to reload the level
	virtual bool	ChangeResolution(int nNewWidth, int nNewHeight, int nNewColDepth, int nNewRefreshHZ, bool bFullScreen, bool bForceReset)=0;
	virtual bool CheckDeviceLost() { return false; };

	virtual EScreenAspectRatio GetScreenAspect(int nWidth, int nHeight);

	virtual void	Release();
	virtual void  FreeResources(int nFlags);

	virtual void	BeginFrame()=0;
	virtual void	RenderDebug()=0;	
	virtual void	EndFrame()=0;	

	virtual void	Reset (void) = 0;

	struct BBoxInfo 
	{ 
		BBoxInfo () { nPrimType=0; }
		Vec3 vMins, vMaxs; float fColor[4]; int nPrimType; 
	};
	std::vector<BBoxInfo> m_arrBoxesToDraw;

	virtual	void	SetCamera(const CCamera &cam)=0;
	virtual	void	SetViewport(int x, int y, int width, int height)=0;
	virtual	void	SetScissor(int x=0, int y=0, int width=0, int height=0)=0;
	virtual void  GetViewport(int *x, int *y, int *width, int *height)=0;

	virtual void	SetState(int State, int AlphaRef=-1)
	{
		m_pRT->RC_SetState(State, AlphaRef);
	}
	virtual void	SetCullMode	(int mode=R_CULL_BACK)=0;
	virtual bool	EnableFog	(bool enable)=0;
	virtual void	SetFog		(float density,float fogstart,float fogend,const float *color,int fogmode)=0;
	virtual void  SetFogColor(float * color)=0;
	virtual	void	EnableVSync(bool enable)=0;

	virtual void  DrawPrimitives(CVertexBuffer *src, int vert_num=4, int prim_type = R_PRIMV_TRIANGLE_STRIP)=0;

	virtual void	PushMatrix()=0;
	virtual void	PopMatrix()=0;

	virtual	void	EnableTMU(bool enable)=0;
	virtual void	SelectTMU(int tnum)=0;

	virtual bool	ChangeDisplay(unsigned int width,unsigned int height,unsigned int cbpp)=0;
	virtual void  ChangeViewport(unsigned int x,unsigned int y,unsigned int width,unsigned int height)=0;

	virtual	bool	SaveTga(unsigned char *sourcedata,int sourceformat,int w,int h,const char *filename,bool flip) const;

	//download an image to video memory. 0 in case of failure
	virtual	unsigned int DownLoadToVideoMemory(unsigned char *data,int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat=true, int filter=FILTER_BILINEAR, int Id=0, const char *szCacheName=NULL, int flags=0, EEndian eEndian = eLittleEndian, RectI * pRegion = NULL, bool bAsynDevTexCreation = false)=0;
	virtual	void UpdateTextureInVideoMemory(uint32 tnum, unsigned char *newdata,int posx,int posy,int w,int h,ETEX_Format eTF=eTF_X8R8G8B8)=0;

	virtual bool DXTCompress( byte *raw_data,int nWidth,int nHeight,ETEX_Format eTF, bool bUseHW, bool bGenMips, int nSrcBytesPerPix, const Vec3 vLumWeight, MIPDXTcallback callback );
	virtual bool DXTDecompress(byte *srcData,const size_t srcFileSize,byte *dstData,int nWidth,int nHeight,int nMips,ETEX_Format eSrcTF, bool bUseHW, int nDstBytesPerPix);
	virtual void DXTCompress_FastOnCPU( byte * pInData, byte * pOutData, const int width, const int height, const bool isDXT5 );





	virtual	bool	SetGammaDelta(const float fGamma)=0;

	virtual	void	RemoveTexture(unsigned int TextureId)=0;

	virtual	void	SetTexture(int tnum);
	virtual	void	SetWhiteTexture();

	virtual void  PrintToScreen(float x, float y, float size, const char *buf);
	virtual void TextToScreen(float x, float y, const char * format, ...);
	virtual void TextToScreenColor(int x, int y, float r, float g, float b, float a, const char * format, ...);

	void  WriteXY( int x,int y, float xscale,float yscale,float r,float g,float b,float a,const char *message, ...) PRINTF_PARAMS(10, 11);
	void	Draw2dText( float posX,float posY,const char *szText,SDrawTextInfo &info );

	/*!	Draw an image on the screen as a label text
	@param vPos:	3d position
	@param fSize: size of the image
	@nTextureId:	Texture Id dell'immagine
	*/
	virtual void DrawLabelImage(const Vec3 &vPos,float fSize,int nTextureId);

	virtual void DrawTextQueued( Vec3 pos,SDrawTextInfo &ti,const char *text );
	virtual void DrawTextQueued( Vec3 pos,SDrawTextInfo &ti,const char *format,va_list args );

	//	Not thread-safe, but it can be called from a thread which is not the main thread.
	virtual bool CalculateTextSize(Vec2& rOutputValue, SDrawTextInfo &ti,const char *format,va_list args,const bool bASCIIMultiLine=true);
	virtual bool CalculateTextSize(Vec2& rOutputValue, SDrawTextInfo &ti,const char *text,const bool bASCIIMultiLine=true);

	virtual void	Draw2dImage	(float xpos,float ypos,float w,float h,int texture_id,float s0=0,float t0=0,float s1=1,float t1=1,float angle=0,float r=1,float g=1,float b=1,float a=1,float z=1)=0;

	virtual int	SetWireframeMode(int mode)=0;

	virtual void ResetToDefault()=0;

	inline const bool IsEditorMode() const
	{



		return (m_bEditor != 0);

	}

	inline float ScaleCoordX(float value)
	{
		//		value=(int)((float)(value)*(float)(m_width)/800.0f);	
		value*=float(m_width)/800.0f;
		return (value);
	}

	inline float ScaleCoordY(float value)
	{
		//		value=(int)((float)(value)*(float)(m_height)/600.0f);	
		value*=float(m_height)/600.0f;
		return (value);
	}

	void SetWidth(int nW) { m_width=nW; }
	void SetHeight(int nH) { m_height=nH; }
	void SetPixelAspectRatio(float fPAR) {m_pixelAspectRatio = fPAR;}
	virtual	int		GetWidth()	{ return (m_width); }
	virtual	int		GetHeight() { return (m_height); }
	virtual	float	GetPixelAspectRatio() const { return (m_pixelAspectRatio); }

	//	Get mipmap distance factor (depends on screen width, screen height and aspect ratio)
	_inline float GetMipDistFactor() { return TANGENT30_2 * TANGENT30_2 / (m_height * m_height); }

	bool DoCompressedNormalmapEmulation() const;

	virtual int		GetWireframeMode() { return(m_wireframe_mode); }

	_inline const CCamera& GetCamera(void) { return(m_RP.m_TI[m_pRT->GetThreadList()].m_cam);  }
	_inline const CRenderCamera& GetRCamera(void) { return(m_RP.m_TI[m_pRT->GetThreadList()].m_rcam);  }

	void		GetPolyCount(int &nPolygons,int &nShadowVolPolys) 
	{
		nPolygons = GetPolyCount();
		nShadowVolPolys = 0;
	}

	int GetPolyCount()
	{
		ASSERT_IS_MAIN_THREAD(m_pRT);
		int nPolys = 0;
		for (int i=0; i<EFSLIST_NUM; i++)
		{
			nPolys += m_RP.m_PS[m_RP.m_nFillThreadID].m_nPolygons[i];
		}
		return nPolys;
	}
	int RT_GetPolyCount()
	{
		ASSERT_IS_RENDER_THREAD(m_pRT);
		int nPolys = 0;
		for (int i=0; i<EFSLIST_NUM; i++)
		{
			nPolys += m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[i];
		}
		return nPolys;
	}

	virtual void SetMaterialColor(float r, float g, float b, float a)=0;

	virtual bool WriteDDS(byte *dat, int wdt, int hgt, int Size, const char *name, ETEX_Format eF, int NumMips);
	virtual bool WriteTGA(byte *dat, int wdt, int hgt, const char *name, int src_bits_per_pixel, int dest_bits_per_pixel );
	virtual bool WriteJPG(byte *dat, int wdt, int hgt, char *name, int src_bits_per_pixel, int nQuality = 100);

	virtual char * GetStatusText(ERendStats type)=0;
	virtual void GetMemoryUsage(ICrySizer* Sizer);
	virtual void GetLogVBuffers() = 0;

	virtual int GetFrameID(bool bIncludeRecursiveCalls=true)
	{
		int nThreadID = m_pRT ? m_pRT->GetThreadList() : 0;
		if (bIncludeRecursiveCalls)
			return m_RP.m_TI[nThreadID].m_nFrameID;
		return m_RP.m_TI[nThreadID].m_nFrameUpdateID;
	}

	// Project/UnProject
	virtual void ProjectToScreen( float ptx, float pty, float ptz, 
		float *sx, float *sy, float *sz )=0;
	virtual int UnProject(float sx, float sy, float sz, 
		float *px, float *py, float *pz,
		const float modelMatrix[16], 
		const float projMatrix[16], 
		const int    viewport[4])=0;
	virtual int UnProjectFromScreen( float  sx, float  sy, float  sz, 
		float *px, float *py, float *pz)=0;

	virtual void FlushTextMessages();

	// Shadow Mapping
	virtual void PrepareDepthMap(ShadowMapFrustum* SMSource, CDLight* pLight=NULL, int nFrustumLOD = 0)=0;
	virtual void SetupShadowOnlyPass(int Num, ShadowMapFrustum* pShadowInst, Matrix34A * pObjMat)=0;
	virtual void DrawAllShadowsOnTheScreen()=0;
	virtual void OnEntityDeleted(IRenderNode * pRenderNode)=0;

	virtual void FX_SetClipPlane (bool bEnable, float *pPlane, bool bRefract)=0;

	virtual void SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa)=0;

	//for editor
	virtual void  GetModelViewMatrix(float *mat)=0;
	virtual void  GetProjectionMatrix(float *mat)=0;
	virtual void SetMatrices(float *pProjMat, float *pViewMat)=0;
	virtual Vec3 GetUnProject(const Vec3 &WindowCoords,const CCamera &cam) { return(Vec3(1,1,1)); };

	virtual void DrawQuad(const Vec3 &right, const Vec3 &up, const Vec3 &origin,int nFlipMode=0)=0;
	virtual void DrawQuad(float dy,float dx, float dz, float x, float y, float z)=0;

	virtual void ClearBuffer(uint32 nFlags, ColorF *vColor, float depth = 1.0f)=0;
	virtual void ReadFrameBuffer(unsigned char * pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX=-1, int nScaledY=-1)=0;

	//misc 
	virtual bool ScreenShot(const char *filename=NULL, int width=0)=0;

	virtual int	GetColorBpp()		{ return m_cbpp; }
	virtual int	GetDepthBpp()		{ return m_zbpp; }
	virtual int	GetStencilBpp()		{ return m_sbpp; }

	virtual void Set2DMode(bool enable, int ortox, int ortoy,float znear=-1e10f,float zfar=1e10f)=0;

	virtual int ScreenToTexture(int nTexID)=0;

	// Shaders/Shaders support
	// RE - RenderElement
	bool m_bTimeProfileUpdated;
	int m_PrevProfiler;
	int m_nCurSlotProfiler;

	FILE *m_LogFile;
	FILE *m_LogFileStr;
	FILE *m_LogFileSh;
	inline void Logv(int RecLevel, const char *format, ...)
	{
		va_list argptr;

		if (m_LogFile)
		{
			for (int i=0; i<RecLevel; i++)
			{
				fprintf(m_LogFile, "  ");
			}
			va_start (argptr, format);
			vfprintf (m_LogFile, format, argptr);
			va_end (argptr);
		}
	}
	inline void LogStrv(int RecLevel, char *format, ...)
	{
		va_list argptr;

		if (m_LogFileStr)
		{
			for (int i=0; i<RecLevel; i++)
			{
				fprintf(m_LogFileStr, "  ");
			}
			va_start (argptr, format);
			vfprintf (m_LogFileStr, format, argptr);
			va_end (argptr);
		}
	}
	inline void LogShv(int RecLevel, char *format, ...)
	{
		va_list argptr;

		if (m_LogFileSh)
		{
			for (int i=0; i<RecLevel; i++)
			{
				fprintf(m_LogFileSh, "  ");
			}
			va_start (argptr, format);
			vfprintf (m_LogFileSh, format, argptr);
			va_end (argptr);
			fflush(m_LogFileSh);
		}
	}
	_inline void Log(char *str)
	{
		if (m_LogFile)
		{
			fprintf (m_LogFile, str);
		}
	}

	virtual void DrawObjSprites(PodArray<struct SVegetationSpriteInfo> *pList) {};
	virtual void GenerateObjSprites(PodArray<struct SVegetationSpriteInfo> *pList) {};

	void EF_AddClientPolys();  
	void EF_RemovePolysFromScene();

	bool FX_TryToMerge(CRenderObject *pNewObject, CRenderObject *pOldObject, CRendElementBase *pRE);

	void EF_TransformDLights();
	void EF_IdentityDLights();

	_inline void *EF_GetPointer(ESrcPointer ePT, int *Stride, EParamType Type, ESrcPointer Dst, int Flags)
	{
		void *p;

		if (m_RP.m_pRE)
			p = m_RP.m_pRE->mfGetPointer(ePT, Stride, Type, Dst, Flags);
		else
			p = SRendItem::mfGetPointerCommon(ePT, Stride, Type, Dst, Flags);

		return p;
	}
	inline void FX_StartMerging()
	{
		if (m_RP.m_FrameMerge != m_RP.m_Frame)
		{
			m_RP.m_FrameMerge = m_RP.m_Frame;
			SBufInfoTable *pOffs = &CRenderMesh2::m_cBufInfoTable[m_RP.m_CurVFormat];
			int Size = CRenderMesh2::m_cSizeVF[m_RP.m_CurVFormat];    
			m_RP.m_Stride = Size;
			m_RP.m_OffsD  = pOffs->OffsColor;
			m_RP.m_OffsT  = pOffs->OffsTC;
			m_RP.m_NextPtr = m_RP.m_Ptr;
			m_RP.m_NextPtrTang = m_RP.m_PtrTang;
		}
	}
	struct SSavedDLight
	{
		CDLight *m_pLight;
		CDLight m_Contents;
	};
	float EF_LightRadius(CDLight *dl)
	{
		float fRadius;
		if (m_RP.m_ObjFlags & FOB_TRANS_MASK)
		{
			float fLen = m_RP.m_pCurObject->m_II.m_Matrix(0,0)*m_RP.m_pCurObject->m_II.m_Matrix(0,0) + m_RP.m_pCurObject->m_II.m_Matrix(0,1)*m_RP.m_pCurObject->m_II.m_Matrix(0,1) + m_RP.m_pCurObject->m_II.m_Matrix(0,2)*m_RP.m_pCurObject->m_II.m_Matrix(0,2);
			//avoid breaking aliasing rules
			union f32_u
			{
				float floatVal;
				unsigned int uintVal;
			};
			f32_u u; u.floatVal = fLen;
			f32_u un;	un.uintVal = 0x5f3759df - (u.uintVal >> 1);
			float fISqrt = (1.5f - (fLen * 0.5f) * un.floatVal * un.floatVal) * un.floatVal;
			fRadius = dl->m_fRadius * fISqrt;
		}
		else
			fRadius = dl->m_fRadius;

		return fRadius;
	}
	/*_inline void EF_PushObjectsList(int nIDThread)
	{
	m_ObjsStack[SRendItem::m_RecurseLevel[nIDThread]].nNumVisObjects = m_RP.m_NumVisObjects[nIDThread];
	m_ObjsStack[SRendItem::m_RecurseLevel[nIDThread]].nNumTempObjects = m_RP.m_TempObjects[nIDThread].Num();
	}
	_inline void EF_PopObjectsList(int nIDThread)
	{
	m_RP.m_NumVisObjects[nIDThread] = m_ObjsStack[SRendItem::m_RecurseLevel[nIDThread]].nNumVisObjects;
	m_RP.m_TempObjects[nIDThread].SetUse(m_ObjsStack[SRendItem::m_RecurseLevel[nIDThread]].nNumTempObjects);
	}*/

	_inline void EF_PushFog()
	{
		assert(m_pRT->IsRenderThread());
		int nLevel = m_nCurFSStackLevel;
		if (nLevel >= 8)
			return;
		memcpy(&m_FSStack[nLevel], &m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS, sizeof(SFogState));
		m_nCurFSStackLevel++;
	}
	_inline void EF_PopFog()
	{
		assert(m_pRT->IsRenderThread());
		int nLevel = m_nCurFSStackLevel;
		if (nLevel <= 0)
			return;
		nLevel--;
		bool bFog = m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_bEnable;
		if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS != m_FSStack[nLevel])
		{
			memcpy(&m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS, &m_FSStack[nLevel], sizeof(SFogState));
			SetFog(m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_FogDensity, m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_FogStart, m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_FogEnd, &m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_FogColor[0], m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_nFogMode);
		}
		else
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_bEnable = m_FSStack[nLevel].m_bEnable;
		bool bNewFog = m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_bEnable;
		m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_bEnable = bFog;
		EnableFog(bNewFog);
		m_nCurFSStackLevel--;
	}

	//================================================================================
	SViewport m_MainRTViewport;
	SViewport m_MainViewport;

	SViewport m_CurViewport;
	SViewport m_NewViewport;
	bool m_bViewportDirty;

	int m_nCurVPStackLevel;
	SViewport m_VPStack[8];
	_inline void FX_PushVP()
	{
		int nLevel = m_nCurVPStackLevel;
		if (nLevel >= 8)
			return;
		memcpy(&m_VPStack[nLevel], &m_NewViewport, sizeof(SViewport));
		m_nCurVPStackLevel++; 
	}
	_inline void FX_PopVP()
	{
		int nLevel = m_nCurVPStackLevel;
		if (nLevel <= 0)
			return;
		nLevel--;
		if (m_NewViewport != m_VPStack[nLevel])
		{
			memcpy(&m_NewViewport, &m_VPStack[nLevel], sizeof(SViewport));
			m_bViewportDirty = true;
		}
		m_nCurVPStackLevel--;
	}


	void EF_AddRTStat(CTexture *pTex, int nFlags = 0, int nW=-1, int nH=-1);
	void EF_PrintRTStats(const char *szName);

	void FX_ApplyShadowQuality();

	void FX_ApplyShaderQuality(EShaderType eST);
	virtual EShaderQuality EF_GetShaderQuality(EShaderType eST);
	virtual ERenderQuality EF_GetRenderQuality() const;

	CREMesh *EF_GetTempMeshRE();
	uint32 EF_BatchFlags(const SShaderItem& SH, CRenderObject *pObj, int nThreadID, CRendElementBase *re);

	virtual float EF_GetWaterZElevation(float fX, float fY);
	virtual void EF_PipelineShutdown() = 0;

	virtual bool EF_PrecacheResource(IShader *pSH, float fMipFactor, float fTimeToReady, int Flags);
	virtual bool EF_PrecacheResource(ITexture *pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId) = 0;
	virtual bool EF_PrecacheResource(IRenderMesh *pPB, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId);
	virtual bool EF_PrecacheResource(CDLight *pLS, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId);

	virtual void PrecacheResources() = 0;

	virtual CRenderObject* EF_AddPolygonToScene(const SShaderItem& si, int numPts, const SVF_P3F_C4B_T2F *verts, const SPipTangents *tangs, CRenderObject *obj, uint16 *inds, int ninds, int nAW, bool bMerge = true);
	virtual CRenderObject* EF_AddPolygonToScene(const SShaderItem& si, CRenderObject* obj, int numPts, int ninds, SVF_P3F_C4B_T2F*& verts, SPipTangents*& tangs, uint16*& inds, int nAW, bool bMerge = true);

	void FX_CheckOverflow(int nVerts, int nInds, CRendElementBase *re, int* nNewVerts=NULL, int* nNewInds=NULL);
	void FX_Start(CShader *ef, int nTech, SRenderShaderResources *Res, CRendElementBase *re);

	void UpdateConstParamsPF();

	//==========================================================
	// external interface for shaders
	//==========================================================

	virtual CREParticle* EF_AddParticlesToScene(const SShaderItem& si, CRenderObject* pRO, IParticleVertexCreator* pPVC, int nAW, bool& canUseGS);
	virtual void EF_ComputeQueuedParticles();

	// Shaders management
	virtual string *EF_GetShaderNames(int& nNumShaders);
	virtual IShader *EF_LoadShader (const char *name, int flags=0, uint64 nMaskGen=0);
	virtual SShaderItem EF_LoadShaderItem (const char *name, bool bShare, int flags=0, SInputShaderResources *Res=NULL, uint64 nMaskGen=0);
	virtual uint64 EF_GetRemapedShaderMaskGen( const char *name, uint64 nMaskGen = 0, bool bFixup = 0);

	virtual uint64      EF_GetShaderGlobalMaskGenFromString( const char *szShaderName, const char *szShaderGen, uint64 nMaskGen = 0 ) ;
	virtual const char  *EF_GetStringFromShaderGlobalMaskGen( const char *szShaderName, uint64 nMaskGen = 0 ) ;


	// reload file
	virtual bool EF_ReloadFile (const char *szFileName);
	virtual void EF_ReloadShaderFiles (int nCategory);
	virtual void EF_ReloadTextures ();
	virtual int EF_LoadLightmap(const char* nameTex);
	virtual bool	EF_ScanEnvironmentCM (const char *name, int size, Vec3& Pos);
	virtual bool  EF_RenderEnvironmentCubeHDR (int size, Vec3& Pos, TArray<unsigned short>& vecData);
	virtual ITexture *EF_GetTextureByID(int Id);
	virtual ITexture *EF_LoadTexture(const char* nameTex, uint32 flags, byte eTT, float fAmount1=-1.0f, float fAmount2=-1.0f);
	virtual const SShaderProfile &GetShaderProfile(EShaderType eST) const;
	virtual void SetShaderQuality(EShaderType eST, EShaderQuality eSQ);

	// Create new RE of type (edt)
	virtual CRendElementBase *EF_CreateRE (EDataType edt);

	// Begin using shaders
	virtual void EF_StartEf ();

	virtual SRenderObjData *EF_GetObjData(CRenderObject *pObj, bool bCreate);
	SRenderObjData *FX_GetObjData(CRenderObject *pObj, bool bCreate);
	virtual void EF_ObjRemovePermanent(CRenderObject *pObj);

	// Get Object for RE transformation
	virtual CRenderObject *EF_GetObject (bool bTemp=false, int num=-1, bool bInit=true);

	// Add shader to the list (virtual)
	virtual void EF_AddEf (CRendElementBase *pRE, SShaderItem& pSH,  CRenderObject *pObj, int nList=EFSLIST_GENERAL, int nAW=1);

	// Add shader to the list 
	void EF_AddEf_NotVirtual (CRendElementBase *pRE, const SShaderItem& pSH, CRenderObject *pObj, int nList=EFSLIST_GENERAL, int nAW=1);

	// Draw all shaded REs in the list
	virtual void EF_EndEf3D (int nFlags, int nPrecacheUpdateId)=0;

	// 2d interface for shaders
	virtual void EF_EndEf2D(bool bSort)=0;

	// Dynamic lights
	virtual bool EF_IsFakeDLight (const CDLight *Source);
	virtual void EF_ADDDlight(CDLight *Source);
	virtual void EF_AddDeferrredDecal(const SDeferrredDecal & rDecal);
	virtual uint32 EF_GetDeferredLightsNum(const eDeferredLightType eLightType = eDLT_DeferredLight);
	virtual int EF_AddDeferredLight( const CDLight &pLight, float fMult );
	virtual TArray<CDLight>* EF_GetDeferredLights(const eDeferredLightType eLightType = eDLT_DeferredLight);
	virtual void EF_ClearDeferredLightsList();
	virtual void EF_ClearLightsList();
	virtual bool EF_UpdateDLight(CDLight *pDL);
	void EF_CheckLightMaterial( CDLight *pLight, CDLight *pNewLight =0 );

	virtual void *EF_Query(int Query, INT_PTR Param=0);
	virtual void EF_SetState(int st, int AlphaRef=-1, int RestoreState=-1) = 0;
	void EF_SetStencilState(int st, uint32 nStencRef, uint32 nStencMask, uint32 nStencWriteMask);

	void CheckRenderObjVisArea( CRenderObject *pObj );
	void FX_SetRenderObjDefaultVisAreaStencilRef( CRenderObject *pObj );
	void FX_SetRenderObjVisAreaStencilRef( CRenderObject *pObj );

	virtual void EF_AddDeferredVisArea( const IVisArea *pVisArea );
	virtual void EF_ClearDeferredVisAreasList();

	// create/delete RenderMesh object
	virtual IRenderMesh* CreateRenderMesh(const char *szSourceType, const char *szSourceName);

	virtual IRenderMesh * CreateRenderMeshInitialized(
		void * pVertBuffer, int nVertCount, EVertexFormat eVF, 
		uint16* pIndices, int nIndices,
		int nPrimetiveType, const char *szType,const char *szSourceName, ERenderMeshType eBufType = eRMT_Static,
		int nMatInfoCount=1, int nClientTextureBindID=0,    
		bool (*PrepareBufferCallback)(IRenderMesh *, bool)=NULL,
		void *CustomData = NULL,
		bool bOnlyVideoBuffer=false, bool bPrecache=true, SPipTangents * pTangents=NULL);

	virtual void DeleteRenderMesh(IRenderMesh * pLBuffer);

	// calculate texel area density
	virtual const float CalculateTexelAreaDensity(CMesh &mesh, const int subMeshIndex, const char* sMeshName);
	virtual const float CalculateTexelAreaDensity(const PodArray<SVF_P3F_C4B_T2F> &vertices,const PodArray<unsigned short> &indices, const char* sMeshName);
	virtual const float CalculateTexelAreaDensity(const PodArray<SVF_P3S_C4B_T2S> &vertices,const PodArray<unsigned short> &indices, const char* sMeshName);

	virtual int GetMaxActiveTexturesARB() { return 0;}

	//////////////////////////////////////////////////////////////////////
	// Replacement functions for the Font engine ( vlad: for font can be used old functions )
	virtual	bool FontUploadTexture(class CFBitmap*, ETEX_Format eTF=eTF_A8R8G8B8)=0;
	virtual	int  FontCreateTexture(int Width, int Height, byte *pData, ETEX_Format eTF=eTF_A8R8G8B8, bool genMips=false)=0;
	virtual	bool FontUpdateTexture(int nTexId, int X, int Y, int USize, int VSize, byte *pData)=0;
	virtual	void FontReleaseTexture(class CFBitmap *pBmp)=0;
	virtual void FontSetTexture(class CFBitmap*, int nFilterMode)=0;
	virtual void FontSetTexture(int nTexId, int nFilterMode)=0;
	virtual void FontSetRenderingState(unsigned int nVirtualScreenWidth, unsigned int nVirtualScreenHeight)=0;
	virtual void FontSetBlending(int src, int dst)=0;
	virtual void FontRestoreRenderingState()=0;

	virtual void Hint_DontSync( CTexture &rTex ){}

	//////////////////////////////////////////////////////////////////////
	// Used for pausing timer related stuff (eg: for texture animations, and shader 'time' parameter)
	void PauseTimer(bool bPause) {  m_bPauseTimer=bPause; }
	virtual IShaderPublicParams* CreateShaderPublicParams();

#ifndef EXCLUDE_SCALEFORM_SDK
	virtual void SF_ConfigMask( ESFMaskOp maskOp, unsigned int stencilCount );
	const SSF_GlobalDrawParams* SF_GetGlobalDrawParams() const { return m_pSFDrawParams; }
	virtual int SF_CreateTexture(int width, int height, int numMips, unsigned char* pData, ETEX_Format eTF);
	virtual void SF_GetMeshMaxSize(int& numVertices, int& numIndices) const;
#endif // #ifndef EXCLUDE_SCALEFORM_SDK

	virtual IVideoPlayer* CreateVideoPlayerInstance() const;

	enum ESPM {ESPM_PUSH = 0, ESPM_POP = 1};
	virtual void SetProfileMarker(const char* label, ESPM mode) {};

	virtual void SetCloudShadowTextureId( int id, const Vec3 & vSpeed ) { m_nCloudShadowTexId = id; m_vCloudShadowSpeed = vSpeed;}
	virtual void SetSkyLightRenderParams( const SSkyLightRenderParams* pSkyLightRenderParams ) { m_pSkyLightRenderParams = pSkyLightRenderParams; }
	const SSkyLightRenderParams* GetSkyLightRenderParams() const { return m_pSkyLightRenderParams; }
	const bool UseSkyLightBasedFog() const { return GetSkyLightRenderParams() && !CV_r_FogColorGradient; }
	virtual uint16 PushFogVolumeContribution( const ColorF& fogVolumeContrib );
	const ColorF& GetFogVolumeContribution( uint16 idx ) const;

	virtual int GetMaxTextureSize() { return m_MaxTextureSize; }

	int GetCloudShadowTextureId() const { return m_nCloudShadowTexId; }  
	Vec3 GetCloudShadowSpeed() const
	{
		return m_vCloudShadowSpeed;
	}

	bool IsHDRModeEnabled()
	{
		bool bHDR = true;
		if (!CV_r_HDRRendering)
			bHDR = false;
		if (CV_r_measureoverdraw || (m_wireframe_mode!= R_SOLID_MODE))
			bHDR = false;
		return bHDR;
	}


	alloc_info_struct *GetFreeChunk(int bytes_count, int nBufSize, PodArray<alloc_info_struct>& alloc_info, const char *szSource);
	bool ReleaseChunk(int p, PodArray<alloc_info_struct>& alloc_info);

	virtual const char * GetTextureFormatName(ETEX_Format eTF);
	virtual int GetTextureFormatDataSize(int nWidth, int nHeight, int nDepth, int nMips, ETEX_Format eTF);
	virtual void SetDefaultMaterials(IMaterial * pDefMat, IMaterial * pTerrainDefMat) { m_pDefaultMaterial = pDefMat; m_pTerrainDefaultMaterial = pTerrainDefMat; }
	virtual byte * GetTextureSubImageData32(byte * pData, int nDataSize, int nX, int nY, int nW, int nH, CTexture * pTex){return 0;}

	virtual void FlushMainThreadAuxGeomCB() = 0;
	virtual void ToggleMainThreadAuxGeomCB() = 0;

	virtual void PrecacheTexture(ITexture *pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId);

protected:
	void EF_AddParticle( CREParticle* pParticle, const SShaderItem& shaderItem, CRenderObject* pRO, int nAW );
	void EF_RemoveParticlesFromScene();
	void SafeReleaseParticleREs();
	void GetMemoryUsageParticleREs( ICrySizer * pSizer );

	CRenderObject* MergeParticleRO( CRenderObject* pRO);
	CRenderObject* MergePolygonRO( CRenderObject* pRO);

protected:
	int	m_width, m_height, m_cbpp, m_zbpp, m_sbpp, m_FullScreen;
	int	m_wireframe_mode, m_wireframe_mode_prev;
	float m_drawNearFov;
	float m_pixelAspectRatio;

	struct text_info_struct 
	{ 
		char	mess[256];
		Vec3 pos; 
		float fFontSizeX;
		float fFontSizeY;
		float fColor[4]; 
		int		nTextureId;
		// @see EDrawTextFlags
		int nDrawFlags;
	};

	CTextMessages						m_TextMessages[2];		// [ThreadID], temporary stores 2d/3d text messages to render them at the end of the frame

	int m_nCloudShadowTexId;
	Vec3 m_vCloudShadowSpeed;
	const SSkyLightRenderParams* m_pSkyLightRenderParams;

#ifndef EXCLUDE_SCALEFORM_SDK
	const SSF_GlobalDrawParams* m_pSFDrawParams;
#endif

public:  

#ifdef WIN32
	static IDirectBee *m_pDirectBee;		// connection to D3D9 wrapper DLL, 0 if not established
#endif
	bool m_bNVLibInitialized;
	bool m_bDriverHasActiveMultiGPU;		// SLI or crossfire (not detected currently), affects r_multigpu behavior
	_inline bool IsMultiGPUModeActive() const
	{



		if (CV_r_multigpu==1)		// on
			return true;

		if (CV_r_multigpu==2)		// auto
			return m_bDriverHasActiveMultiGPU;

		return false;						// off

	}

	_inline bool IsLinearSpaceShadingEnabled() const
	{
		// todo: value should not be directly from cvar, but from bool that becomes updated by cvar ever frame.
		// code should not use CV_r_usesrgb but IsLinearShadingSpace() istead

		return CV_r_useSRGB!=0;
	}

	uint32 m_nGPUs;

	CCamera	m_prevCamera;								// camera from previous frame

	uint32 m_nFrameLoad;
	uint32 m_nFrameReset;
	uint32 m_nFrameSwapID;							// without recursive calls, access through GetFrameID(false)
	bool m_bTemporaryDisabledSFX;

	ColorF m_cClearColor;
	int	 m_numtmus;

	CREPostProcess *m_pREPostProcess;

	// Used for pausing timer related stuff (eg: for texture animations, and shader 'time' parameter)
	bool m_bPauseTimer;
	float m_fPrevTime;
	bool m_bUseZpass;

	// HDR rendering stuff
	int m_dwHDRCropWidth;
	int m_dwHDRCropHeight;

	S3DEngineCommon m_p3DEngineCommon;

	//Debug Gun 
	IRenderNode* m_pDebugRenderNode;

	//=====================================================================
	// Shaders interface
	CShaderMan m_cEF;
	_smart_ptr<IMaterial> m_pDefaultMaterial;
	_smart_ptr<IMaterial> m_pTerrainDefaultMaterial;

	int m_TexGenID;

	IFFont *m_pDefaultFont;

	virtual void SetClearColor(const Vec3 & vColor) { m_cClearColor.r=vColor[0];m_cClearColor.g=vColor[1];m_cClearColor.b=vColor[2]; }

	//////////////////////////////////////////////////////////////////////
	// console variables
	//////////////////////////////////////////////////////////////////////

	static int CV_r_vsync;
	static int CV_r_stats;
	static int CV_r_log;
	static int CV_r_logTexStreaming;  
	static int CV_r_logShaders;
	static int CV_r_logVBuffers;
	static int CV_r_logVidMem;
	static int CV_r_flush;
	static int CV_r_predicatedtiling;
	static int CV_r_testSplitScreen;
	static int CV_r_SplitScreenActive;

	static int CV_r_ambientPartialStencilRefresh;
	static int CV_r_ambientIndexedDraw;

	static int CV_r_deferredshading;
	static int CV_r_deferredshadingLightVolumes;  
	static int CV_r_deferredshadingstencilprepass;  
	static int CV_r_deferredshadingscissor;
	static int CV_r_deferredshadingdepthboundstest; 
	static int CV_r_deferredshadingdebug;
	static float CV_r_deferredshadinglightlodratio;  
	static int CV_r_deferredshadingcubemaps;
	static int CV_r_deferredshadingheightbasedambient;



	static int CV_r_deferredshadinginterleavedacc;	
	static int CV_r_irradiancevolumes;

	static int CV_r_HDRRendering;
	static int CV_r_HDRDebug;

	static float CV_r_HDRLevel;
	static float CV_r_HDROffset;
	static float CV_r_HDRContrast;
	static float CV_r_HDRContrastLuminanceBlend;
	static int	 CV_r_HDRVignetting;

	static float CV_r_HDRBrightOffset;
	static float CV_r_HDRBrightThreshold;
	static float CV_r_HDRBrightLevel;

	static float CV_r_HDRBlueShift;		

	static int CV_r_HDRTexFormat;
	static int CV_r_HDRForceUpdateTextures;

	static int CV_r_HDRRangeAdapt;		
	static float CV_r_HDRRangeAdaptMaxRange;
	static float CV_r_HDRRangeAdaptMax;		
	static float CV_r_HDRRangeAdaptLBufferMaxRange;
	static float CV_r_HDRRangeAdaptLBufferMax;		
	static float CV_r_HDREyeAdaptationFactor;
	static float CV_r_HDREyeAdaptationBase;
	static float CV_r_HDREyeAdaptationSpeed;
	static float CV_r_HDRRangeAdaptationSpeed;

	static int CV_r_geominstancing;
	static int CV_r_geominstancingthreshold;
	static int m_iGeomInstancingThreshold;		// internal value, auto mapped depending on GPU hardware, 0 means not set yet
	static void ChangeGeomInstancingThreshold( ICVar *pVar=0 );

	static int CV_r_DebugLightVolumes;

	static int CV_r_UseShadowsPool;
	static float CV_r_ShadowsSlopeScaleBias;
	static float CV_r_ShadowsBias;
	static int CV_r_shadowtexformat;
	static int CV_r_ShadowsMaskResolution;
	static int CV_r_ShadowsMaskDownScale;
	static int CV_r_ShadowsDeferredMode;
	static int CV_r_ShadowsStencilPrePass;
	static int CV_r_ShadowsDepthBoundNV;
	static int CV_r_ShadowsForwardPass;
	static float CV_r_shadowbluriness;
	static float CV_r_shadow_jittering;
	static float CV_r_VarianceShadowMapBlurAmount;
	static int CV_r_ShadowsGridAligned;
	static int CV_r_ShadowGenGS;
	static int CV_r_ShadowPass;
	static int CV_r_ShadowGen;
	static int CV_r_ShadowsUseClipVolume;
	static int CV_r_shadowblur;

	static int CV_r_ShadowGenMode;
	static int CV_capture_misc_render_buffers;

	static float CV_r_RenderMeshHashGridUnitSize;
	static int CV_r_SSAO;
	static int CV_r_SSAO_normals;
	static int CV_r_SSAO_downscale;
	static int CV_r_SSAO_downscale_ztarget;
	static int CV_r_SSAO_downscale_result_mask;
	static float CV_r_SSAO_amount;
	static float CV_r_SSAO_darkening;
	static float CV_r_SSAO_radius;
	static int CV_r_SSAO_quality;

	static int CV_r_SSGI;
	static float CV_r_SSGI_amount;
	static float CV_r_SSGI_radius;
	static int CV_r_SSGI_quality;
	static int CV_r_SSGI_blur;

	static int CV_r_TerrainAO;
	static int CV_r_TerrainAO_FadeDist;

	static int CV_r_debuglights;

	static int		CV_r_rc_autoinvoke;

	static int    CV_r_glow;  
	static int    CV_r_glowanamorphicflares;  
	static int    CV_r_refraction;   
	static int    CV_r_sunshafts; 
	static int    CV_r_pointlightshafts; 
	static int    CV_r_distant_rain;  
	int						CV_r_nightvision;  

	static int    CV_r_use_merged_posteffects; 

	int						CV_r_PostProcess;
	static int    CV_r_PostProcessReset;
	static int    CV_r_PostProcessFilters;
	static int    CV_r_PostProcessGameFx;
	static int    CV_r_PostProcessScreenQuadTessX;
	static int    CV_r_PostProcessScreenQuadTessY;
	static int    CV_r_PostProcessParamsBlending;
	static float  CV_r_PostprocessParamsBlendingTimeScale;
	static int		CV_r_PostProcessHUD3D;

	static int    CV_r_showgammareference;

	static int    CV_r_colorgrading;
	static int    CV_r_colorgrading_selectivecolor;
	static int    CV_r_colorgrading_levels;
	static int    CV_r_colorgrading_filters;
	static int    CV_r_colorgrading_dof;
	static int    CV_r_colorgrading_charts;

	static int    CV_r_fur;

	static int CV_r_cloudsupdatealways;
	static int CV_r_cloudsdebug;

	static int CV_r_showdyntextures;
	static int CV_r_ShowDynTexturesMaxCount;
	static ICVar *CV_r_ShowDynTexturesFilter;
	static int CV_r_shownormals;
	static int CV_r_showlines;
	static float CV_r_normalslength;
	static int CV_r_showtangents;
	static int CV_r_showtimegraph;
	static int CV_r_DebugFontRendering;
	static int CV_profileStreaming;
	static int CV_r_graphstyle;
	static int CV_r_showmt;

	static ICVar*	CV_r_ShaderCompilerServer;
	static int CV_r_ShaderCompilerPort;
	static int CV_r_ShaderLod;

	static ICVar *CV_r_excludeshader;
	static ICVar *CV_r_showonlyshader;
	static int CV_r_profileshaders;
	static int CV_r_ProfileShadersSmooth;
	static int CV_r_ProfileShadersGroupByName;

	static int CV_r_dyntexmaxsize;
	static int CV_r_dyntexatlascloudsmaxsize;
	static int CV_r_dyntexatlasspritesmaxsize;
	static int CV_r_dyntexatlasvoxterrainsize;
	static int CV_r_dyntexatlasdyntexsrcsize;

	static int CV_r_texpostponeloading;
	static int CV_r_texpreallocateatlases;
	int CV_r_texatlassize;
	static int CV_r_texmaxanisotropy;
	static int CV_r_texmaxsize;
	static int CV_r_texminsize;
	static int CV_r_texresolution;
	static int CV_r_texlmresolution;
	static int CV_r_texbumpresolution;
	static int CV_r_texskyresolution;
	static int CV_r_texnormalmaptype;
	static int CV_r_texhwmipsgeneration;
	//	static int CV_r_texhwdxtcompression;
	static int CV_r_texgrid;
	static int CV_r_texlog;
	static int CV_r_texnoload;

	static int CV_r_texturesstreampoolsize;
	static int CV_r_texturesstreaming;
	static int CV_r_texturesstreamingdebug;
	static float CV_r_texturesstreamingmaxasync;
	static int CV_r_TexturesStreamingMipBias;
	static int CV_r_texturesstreamingnoupload;
	static int CV_r_texturesstreamingonlyvideo;
	static int CV_r_texturesstreamingsync;
	static int CV_r_texturesfilteringquality;

	static float CV_r_TextureLodDistanceRatio;
	static int CV_r_TextureLodMaxLod;

	static int CV_r_lightssinglepass;

	static int CV_r_envcmwrite;
	static int CV_r_envcmresolution;
	static int CV_r_envtexresolution;
	static float CV_r_envlcmupdateinterval;
	static float CV_r_envcmupdateinterval;
	static float CV_r_envtexupdateinterval;

	static float CV_r_waterupdateFactor;
	static float CV_r_waterupdateDistance;
	static int CV_r_waterreflections_mgpu;	
	static int CV_r_waterreflections_use_min_offset;	
	static float CV_r_waterreflections_min_visible_pixels_update;	
	static float CV_r_waterreflections_minvis_updatefactormul;	
	static float CV_r_waterreflections_minvis_updatedistancemul;	
	static int CV_r_waterreflections;	
	static float CV_r_waterreflections_offset;	

	static int CV_r_watercaustics;
	static float CV_r_watercausticsdistance;
	static int CV_r_watercausticsdeferred;
	static int CV_r_waterreflections_quality;  
	static int CV_r_water_godrays;  

	static int CV_r_water_ripples;  

	static int CV_r_reflections;	
	static int CV_r_reflections_quality;	

	static int CV_r_texture_anisotropic_level;
	static int CV_r_texnoaniso;

	static int CV_r_reloadshaders;

	static int CV_r_detailtextures;
	static float CV_r_detailscale;
	static float CV_r_detaildistance;
	static int CV_r_detailnumlayers;

	static int CV_r_noloadtextures;
	static int CV_r_texbindmode;
	static int CV_r_nodrawshaders;
	static int CV_r_nodrawnear;
	static float CV_r_DrawNearZRange;
	static float CV_r_DrawNearFarPlane;
	static float CV_r_drawnearfov;

	static int CV_r_optimisedlightsetup;

	static int CV_r_shadersdebug;
#if !defined (XENON) && !defined(PS3)
	static int CV_r_shadersxenon;
	static int CV_r_shadersps3;
	static int CV_r_shadersdx9;
	static int CV_r_shadersdx11;
#endif
	static int CV_r_shadersignoreincludeschanging;
	static int CV_r_shaderslazyunload;
	static int CV_r_shadersuserfolder;
	static int CV_r_shaderspreactivate;
	static int CV_r_shadersintcompiler;
	static int CV_r_shadersnocompile;
	static int CV_r_shadersnosources;
	static int CV_r_shadersremotecompiler;
	static int CV_r_shadersasynccompiling;
	static int CV_r_shadersasyncmaxthreads;
	static int CV_r_shaderscacheoptimiselog;
	static int CV_r_shadersprecachealllights;
	static int CV_r_shadersdynamicbranching;

	static int CV_r_debugrendermode;
	static int CV_r_debugrefraction;
	static float CV_r_DebugVoxTerrainX;
	static float CV_r_DebugVoxTerrainY;

	static int CV_r_meshprecache;

	static int CV_r_multithreaded;
	static int CV_r_multigpu;
	static int CV_r_validateDraw;
	static int CV_r_profileGPU;

	static int CV_r_fsaa;
	static int CV_r_fsaa_samples;
	static int CV_r_fsaa_quality;

	static int CV_r_atoc;

	static int CV_r_impostersdraw;
	static float CV_r_imposterratio;
	static int CV_r_impostersupdateperframe;

	static int CV_r_flares;

	static int CV_r_beams;
	static int CV_r_beamssoftclip;
	static int CV_r_beamshelpers;
	static int CV_r_beamsmaxslices;
	static float CV_r_beamsdistfactor;










	static int CV_r_ZPrePass;
	static float CV_r_ZPrePassRadiusThreshold;
	static float CV_r_ZPrePassDistanceThreshold;

	static int CV_r_CBStatic;
	static int CV_r_CBStaticDebug;

	static int CV_r_usezpass;
	static int CV_r_usealphablend;
	static int CV_r_useedgeaa;
	static int CV_r_usehwskinning;
	static int CV_r_usemateriallayers;
	static int CV_r_usesoftparticles;
	static int CV_r_usewaterparticles;
	static int CV_r_useparticles_refraction;
	static int CV_r_useparticles_glow;
	static int CV_r_usepom;

	static int CV_r_PostMSAA;
	static int CV_r_PostStereo;

	static int CV_r_rain;  
	static float CV_r_rainamount;  
	float CV_r_rain_maxviewdist;  

	static int CV_r_hair_sorting_quality;

	static int CV_r_motionblur;  
	static int s_MotionBlurMode;				// potentially adjusted internal state of r_motionblur
	static float CV_r_motionblur_shutterspeed;  
	static int CV_r_motionblurframetimescale;  
	static int CV_r_motionblurdynamicquality;  
	static float CV_r_motionblurdynamicquality_rotationacc_stiffness;  
	static float CV_r_motionblurdynamicquality_rotationthreshold;  
	static float CV_r_motionblurdynamicquality_translationthreshold;  

	static int CV_r_debug_extra_scenetarget_fsaa; 

	static float CV_r_cloak_light_scale;

	static int CV_r_customvisions;  
	static int CV_r_DebugLayerEffect; 

	static int CV_r_dof;
	static int CV_r_dof_bokeh;
	static int CV_r_dof_stencil_prepass;

	static float CV_r_gamma;
	static float CV_r_contrast;
	static float CV_r_brightness;
	static int CV_r_nohwgamma;

	static int CV_r_coronas;
	static int CV_r_scissor;
	static float CV_r_coronafade;
	static float CV_r_coronacolorscale;
	static float CV_r_coronasizescale;

	static int CV_r_cullbyclipplanes;

	static int CV_r_wireframe;		
	static int CV_r_GetScreenShot;	

	static int CV_r_printmemoryleaks;
	static int CV_r_releaseallresourcesonexit;
	static int CV_r_character_nodeform;
	static int CV_r_VegetationSpritesGenDebug;
	static int CV_r_VegetationSpritesNoBend;
	static int CV_r_ZPassOnly;
	static int CV_r_VegetationSpritesNoGen;
	static int CV_r_VegetationSpritesTexRes;
	static int CV_r_VegetationSpritesGenAlways;
	static int CV_r_VegetationSpritesIgnoreAngle;
	static int CV_r_VegetationAlphaTestOnly;

	static int CV_r_measureoverdraw;
	float CV_r_measureoverdrawscale;

	static int CV_r_ShowVideoMemoryStats;
	static int CV_r_ShowRenderTarget;
	static int CV_r_ShowRenderTarget_FullScreen;
	static ICVar* CV_r_ShowTexture;
	static int CV_r_ShowLightBounds;
	static int CV_r_MergeRenderChunksForDepth;
	static int CV_r_TextureCompressor;

	static int CV_r_UseGSParticles;
	static int CV_r_Force3DcEmulation;

	static float CV_r_ZFightingDepthScale;
	static float CV_r_ZFightingExtrude;

	static int CV_r_useSRGB;
	float CV_r_TexelsPerMeter;

	static int CV_r_ConditionalRendering;
	static int CV_r_PS3HalfResRendering;
	static int CV_r_PS3SaveRSXPerformanceTimings;
	static int CV_r_PS3VMemDefrag;
	static int CV_r_PS3VMemDefragDebug;
	static int CV_r_PS3ConstBufferAdjustment;
	static int CV_r_PS3SoftwareRasterizer;
#if defined(CRY_DXPS_RASTERTHREAD)
	static int UsePS3SoftwareRastizer()
	{
		return CV_r_PS3SoftwareRasterizer && IsSPUEnabled();
	}
#endif
	static int CV_r_Stereo;
	static float CV_r_StereoEyeDist;
	static float CV_r_StereoScreenDist;

	static float CV_r_FogDepthTest;
	static int CV_r_FogColorGradient;
	int CV_e_DebugTexelDensity;

	static float CV_r_FlashMatTexResQuality;

	virtual void MakeMatrix(const Vec3 & pos, const Vec3 & angles,const Vec3 & scale, Matrix34 * mat){assert(0);};

	void* operator new( size_t Size )
	{
		void *pPtrRes = CryModuleMemalign(Size, 16);
		memset(pPtrRes, 0, Size);
		return pPtrRes;
	}
	void operator delete( void *Ptr )
	{
		CryModuleMemalignFree(Ptr);
	}

	virtual WIN_HWND GetHWND() = 0;

	void SetTextureAlphaChannelFromRGB(byte * pMemBuffer, int nTexSize);

	void EnableSwapBuffers(bool bEnable) { m_bSwapBuffers = bEnable; }
	bool m_bSwapBuffers;

	virtual bool GetImageCaps(const char* filename,int& width,int& height) const;
	virtual bool MergeImages(const char* output_filename,int out_width,int out_height,const char** files,int* offsetsX,int* offsetsY,int* widths,int* heights,int* src_offsetsX,int* src_offsetsY,int* src_width,int* src_height, int count) const;

#if defined(ENABLE_GPU_TIMERS)
	//GPU Timers
	virtual void	RT_BeginGPUTimer( const char* name );
	virtual void	RT_EndGPUTimer(const char* name);
	virtual void	SwapGpuTimers();
	virtual void	RenderGpuStats();
	virtual void	RenderGpuStatsDebugNode();
#else
	virtual void	RT_BeginGPUTimer( const char* name) {};
	virtual void	RT_EndGPUTimer(const char* name) {};
	virtual void	SwapGpuTimers() {};
	virtual void	RenderGpuStats() {};
	virtual void	RenderGpuStatsDebugNode() {};
#endif

	//platform specific
	virtual void	RT_InsertGpuCallback(uint32 context, GpuCallbackFunc callback) = 0;

	virtual float GetGPUFrameTime();
	virtual void GetRenderTimes(SRenderTimes &outTimes);

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
public:
	virtual void EF_AddGPUParticlesToScene( int32 nGPUParticleIdx, AABB const& bb, const SShaderItem& shaderItem, CRenderObject* pRO, bool nAW, bool canUseGS );

protected:
	void  EF_RemoveGPUParticlesFromScene();
	void  SafeReleaseParticleGPUREs();
	void	GetMemoryUsageParticleGPUREs( ICrySizer *pSizer );
#endif
};

//struct CVars;
class CryCharManager;
void *gGet_D3DDevice();
void *gGet_glReadPixels();
#include "CommonRender.h"

#define SKY_BOX_SIZE 32.f

#endif //rende
