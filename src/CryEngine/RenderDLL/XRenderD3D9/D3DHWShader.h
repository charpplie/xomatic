/*=============================================================================
  D3DCGPShader.h : Direct3D9 CG pixel shaders interface declaration.
  Copyright (c) 2001-2005 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honitch Andrey

=============================================================================*/

#ifndef __D3DHWSHADER_H__
#define __D3DHWSHADER_H__

#if !defined(OPENGL)
	#define MERGE_SHADER_PARAMETERS 1
#endif
// Streams redefinitions for OpenGL/PS3 (TEXCOORD#)
#define VSTR_COLOR2        2  //SH Stream (VSF_SH_INFO)
#define VSTR_COLOR3        3  //SH Stream (VSF_SH_INFO)
#define VSTR_COLOR1        4  //Base Stream
#define VSTR_NORMAL1       5  //Base Stream
#define VSTR_PSIZE         6  //Base particles stream
#define VSTR_MORPHTARGETDELTA 7 // MorphTarget stream (VSF_HWSKIN_MORPHTARGET_INFO)
#define VSTR_TANGENT       8  // Tangents stream (VSF_TANGENTS)
#define VSTR_BINORMAL      9  // Tangents stream (VSF_TANGENTS)
#define VSTR_BLENDWEIGHTS 10  // HWSkin stream (VSF_HWSKIN_INFO)
#define VSTR_BLENDINDICES 11  // HWSkin stream (VSF_HWSKIN_INFO)
#define VSTR_BONESPACE    12  // HWSkin stream (VSF_HWSKIN_INFO)
#define VSTR_SHAPEDEFORMINFO 13 // ShapeDeform stream (VSF_HWSKIN_SHAPEDEFORM)
#define VSTR_THIN         14  // ShapeDeform stream (VSF_HWSKIN_SHAPEDEFORM)
#define VSTR_FAT          15  // ShapeDeform stream (VSF_HWSKIN_SHAPEDEFORM)

#if defined (DIRECT3D9) || defined (OPENGL)



  #define MAX_CONSTANTS_PS 64

  #define MAX_CONSTANTS_VS 256
#else
#if !defined(PS3)
  #define MAX_CONSTANTS_PS 128
  #define MAX_CONSTANTS_VS 512
  #define MAX_CONSTANTS_GS 128




#endif
#endif

//==============================================================================

int D3DXGetSHParamHandle(void *pSH, SCGBind *pParam);

struct SParamsGroup
{
  std::vector<SCGParam> Params[2];
  std::vector<SCGParam> Params_Inst;
};

enum ED3DShError
{
  ED3DShError_NotCompiled,
  ED3DShError_CompilingError,
  ED3DShError_Fake,
  ED3DShError_Ok,
  ED3DShError_Compiling,
};

//====================================================================================

struct SCGParamsGroup
{
  int nParams;
  SCGParam *pParams;
  int nPool;
  int nRefCounter;
  SCGParamsGroup()
  {
    nParams = 0;
    nPool = 0;
    pParams = NULL;
    nRefCounter = 1;
  }
  unsigned Size() { return sizeof(*this); }
	void GetMemoryUsage(ICrySizer *pSizer)const {}
};

#define PARAMS_POOL_SIZE 256

struct SCGParamPool
{
  PodArray<alloc_info_struct> m_alloc_info;
  DynArray<SCGParam> m_Params;
  int m_nMaxEntries;

  SCGParamsGroup Alloc(int nEntries);
  bool Free(SCGParamsGroup& Group);
  unsigned Size() { return sizeof(*this) + sizeOfV(m_alloc_info) + sizeOfV(m_Params); }
	void GetMemoryUsage(ICrySizer *pSizer)const 
	{
		pSizer->AddObject(m_alloc_info);
		pSizer->AddObject(m_Params);
	}
};


class CGParamManager
{
  friend class CHWShader_D3D;
  //friend struct CHWShader_D3D::SHWSInstance;

  static DynArray<uint32> m_FreeGroups;

public:
  static SCGParamPool *NewPool(int nEntries);
  static int GetParametersGroup(std::vector<SCGParam>& Params);
  static bool FreeParametersGroup(int nID);

  static DynArray<SCGParamsGroup> m_Groups;
  static DynArray<SCGParamPool> m_Pools;
};

//=========================================================================================

struct SD3DShader
{
  int m_nRef;
  void *m_pHandle;
  bool m_bBound;

  SD3DShader()
  {
    m_nRef = 1;
    m_pHandle = NULL;
    m_bBound = false;
  }
  int AddRef()
  {
    return m_nRef++;
  }
  int Release(EHWShaderClass eSHClass, int nSize);

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{ 
		pSizer->AddObject(this, sizeof(*this));



	}
};

struct SD3DShaderHandle
{
  SD3DShader *m_pShader;
  byte *m_pData;
  int m_nData;
  byte m_bStatus;
  SD3DShaderHandle()
  {
    m_pShader = NULL;
    m_bStatus = 0;
    m_nData = 0;
    m_pData = NULL;
  }
  void SetShader(SD3DShader *pShader)
  {
    m_bStatus = 0;
    m_pShader = pShader;
  }
  void SetFake()
  {
    m_bStatus = 2;
  }
  void SetNonCompilable()
  {
    m_bStatus = 1;
  }
  int AddRef()
  {
    if (!m_pShader)
      return 0;
    return m_pShader->AddRef();
  }
  int Release(EHWShaderClass eSHClass, int nSize)
  {
    if (!m_pShader)
      return 0;
    return m_pShader->Release(eSHClass, nSize);
  }

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_pShader);
	}
};

struct SShaderAsyncInfo
{
  SShaderAsyncInfo *m_Next;           //!<
  SShaderAsyncInfo *m_Prev;           //!<
  _inline void Unlink()
  {
		if (!m_Next || !m_Prev)
			return;
    m_Next->m_Prev = m_Prev;
    m_Prev->m_Next = m_Next;
    m_Next = m_Prev = NULL;
  }
  _inline void Link(SShaderAsyncInfo* Before)
  {
		if (m_Next || m_Prev)
			return;
    m_Next = Before->m_Next;
    Before->m_Next->m_Prev = this;
    Before->m_Next = this;
    m_Prev = Before;
  }
  static void FlushPendingShaders();

#if !defined (XENON) && defined (WIN32)
  PROCESS_INFORMATION m_ProcessInfo;
  HANDLE m_hPipeOutputRead;
	HANDLE m_hPipeOutputWrite;
#endif
  int m_nOwner;
  uint64 m_RTMask;
  uint32 m_LightMask;
  uint32 m_MDMask;
  uint32 m_MDVMask;
  EHWShaderClass m_eClass;
  class CHWShader_D3D *m_pShader;
  CShader *m_pFXShader;

  LPD3DXBUFFER m_pDevShader;
  LPD3DXBUFFER m_pErrors;
  LPD3DXCONSTANTTABLE m_pConstants;
  CCryName m_Name;
  string m_Text;
  string m_Errors;
  string m_Profile;
  //CShaderThread *m_pThread;
  DynArray<SCGBind> m_InstBindVars;
  byte m_bPending;
  bool m_bPendedFlush;
  bool m_bPendedSamplers;
  bool m_bPendedEnv;
  float m_fMinDistance;
  int m_nFrame;
  int m_nThread;
  int m_nCombination;

  SShaderAsyncInfo()
  {
    m_Next = m_Prev = NULL;
#if !defined (XENON) && defined (WIN32)
    memset(&m_ProcessInfo, 0, sizeof(PROCESS_INFORMATION));
    m_hPipeOutputRead = NULL;
    m_hPipeOutputWrite = NULL;
#endif
    m_nThread = 0;
    m_fMinDistance = 0;
    m_nOwner = -1;
    m_pShader = NULL;
    m_pFXShader = NULL;
    m_pDevShader = NULL;
    m_pErrors = NULL;
    m_pConstants = NULL;
    m_bPending = true; //false; - this flag is now used as an atomic indication that if the async shader has been compiled
    m_nCombination = -1;

    m_bPendedEnv = false;
    m_bPendedFlush = false;
    m_bPendedSamplers = false;
  }
  ~SShaderAsyncInfo();
  static volatile int s_nPendingAsyncShaders;
  static int s_nPendingAsyncShadersFXC;
  static SShaderAsyncInfo m_PendingList;
  static SShaderAsyncInfo m_PendingListT;
	static CryEvent m_RequestEv;
};

#ifdef SHADER_ASYNC_COMPILATION

#include "CryThread.h"
#define SHADER_THREAD_NAME "ShaderCompile"

class CAsyncShaderTask
{
	friend class CD3D9Renderer; // so it can instantiate us

public:
  CAsyncShaderTask();

	static void InsertPendingShader(SShaderAsyncInfo* pAsync);
  int GetThread() { return m_nThread; }
  int GetThreadFXC() { return m_nThreadFXC; }
  void SetThread(int nThread)
  {
    m_nThread = nThread; m_nThreadFXC = -1;
  }
  void SetThreadFXC(int nThread) { m_nThreadFXC = nThread; }

private:

	void FlushPendingShaders();

	static SShaderAsyncInfo m_build_list;
	SShaderAsyncInfo m_flush_list;
  int m_nThread;
  int m_nThreadFXC;

	class CShaderThread : public CrySimpleThread<>
	{
	public:
		CShaderThread(CAsyncShaderTask* task) : m_task(task), m_quit(false)
		{	
      CAsyncShaderTask::m_build_list.m_Next = &CAsyncShaderTask::m_build_list;
      CAsyncShaderTask::m_build_list.m_Prev = &CAsyncShaderTask::m_build_list;

      task->m_flush_list.m_Next = &task->m_flush_list;
      task->m_flush_list.m_Prev = &task->m_flush_list;



			Start();

		}

		~CShaderThread()
		{
			m_quit = true;



			WaitForThread();
		}

	private:
		void Run();

		CAsyncShaderTask* m_task;
		volatile bool m_quit;
	};

	CShaderThread m_thread;

	bool CompileAsyncShader(SShaderAsyncInfo* pAsync);
  bool PostCompile(SShaderAsyncInfo* pAsync);
};
#endif

class CHWShader_D3D : public CHWShader
{
  friend class CD3D9Renderer;
  friend class CAsyncShaderTask;
  friend class CGParamManager;
  friend struct SShaderAsyncInfo;
  friend class CHWShader;

  SShaderCache *m_pCache;
  SPreprocessTree *m_pTree;

  struct SHWSInstance
  {
    friend struct SShaderAsyncInfo;

    SD3DShaderHandle m_Handle;
    EHWShaderClass m_eClass;

    uint64 m_RTMask;      // run-time mask
    uint64 m_GLMask;        // global mask
    uint32 m_LightMask;     // light mask
    uint32 m_MDMask;        // texture coordinates modifier mask
    uint32 m_MDVMask;       // vertex modifier mask

    int m_nParams[2];   // 0: Instance independent; 1: Instance depended
    DynArray<STexSampler> m_pSamplers;
    std::vector<SCGBind> m_pBindVars;
    int m_nParams_Inst;
    float m_fLastAccess;
    int m_nUsed;
    int m_nUsedFrame;
#if defined (DIRECT3D10)
    void *m_pShaderData;
		size_t m_nShaderByteCodeSize;
    int m_nMaxVecs[3];
#endif



    short m_nInstMatrixID;
    short m_nInstIndex;
    short m_nInstructions;
    uint16 m_VStreamMask_Stream;
    uint16 m_VStreamMask_Decl;
    short m_nCache;
    short m_nParent;
    byte m_bDeleted : 1;
    byte m_bXenon : 1;
    byte m_bPS3   : 1;
    byte m_bD3D11   : 1;
    byte m_bShared : 1;
    byte m_bHasPMParams : 1;
    byte m_bFallback : 1;
    byte m_bCompressed : 1;
    byte m_nVertexFormat;
    byte m_nNumInstAttributes;

    int m_nDataSize;
    int m_DeviceObjectID;
    SShaderAsyncInfo *m_pAsync;
    SHWSInstance()
    {
      m_nNumInstAttributes = 0;
      m_RTMask = 0;
      m_GLMask = 0;
      m_LightMask = 0;
      m_MDMask = 0;
      m_MDVMask = 0;
      m_nParams_Inst = -1;
      m_nParams[0] = -1;
      m_nParams[1] = -1;
      //m_pSamplers = NULL;
      //m_pBindVars = NULL;
      m_nInstructions = 0;
      m_nUsed = 0;
      m_nUsedFrame = 0;
      m_eClass = eHWSC_Max;
      m_bFallback = false;
      m_nInstMatrixID = 1;
      m_nCache = -1;
      m_nParent = -1;
#if defined (DIRECT3D10)
      m_pShaderData = NULL;
			m_nShaderByteCodeSize = 0;
#endif
#if defined (XENON) || defined (WIN32)	|| defined(PS3)
      m_pAsync = NULL;
#endif
      m_bCompressed = false;

      m_fLastAccess = 0;
      m_DeviceObjectID = -1;
      m_nInstIndex = -1;
      m_nDataSize = 0;
      m_nVertexFormat = (byte)1;
      m_VStreamMask_Decl = 0;
      m_VStreamMask_Stream = 0;

      m_bDeleted = false;
      m_bXenon = false;
      m_bPS3 = false;
      m_bD3D11 = false;
      m_bShared = false;
      m_bHasPMParams = false;
    }
    void Release(SShaderCache *pCache=NULL, bool bReleaseData=true);

    int Size()
    {
      int nSize = sizeof(*this);
      nSize += sizeOfV(m_pSamplers);
      nSize += sizeOfV(m_pBindVars);




      return nSize;
    }

		void GetMemoryUsage(ICrySizer* pSizer) const
		{
			pSizer->AddObject(m_Handle);
			pSizer->AddObject( m_pSamplers );
			pSizer->AddObject( m_pBindVars );
			



		}

    bool IsAsyncCompiling()
    {
#if defined(WIN32) || defined(XENON)	||	defined(PS3)
      if (m_pAsync)
        return true;
#endif
      return false;
    }
  };
  struct SHWSSharedInstance
  {
    uint64 m_GLMask;
    DynArray<SHWSInstance> m_Insts;
    unsigned Size() { return sizeof(SHWSSharedInstance) + sizeOfV(m_Insts); }
		void GetMemoryUsage(ICrySizer* pSizer)const
		{
			pSizer->AddObject(m_Insts);
		}
  };
  struct SHWSSharedName
  {
    string m_Name;
    uint32 m_CRC32;
    unsigned Size() { return sizeof(SHWSSharedName) + m_Name.capacity(); }
		void GetMemoryUsage(ICrySizer* pSizer)const
		{
			pSizer->AddObject(m_Name);
		}
  };
  struct SHWSSharedList
  {
    DynArray<SHWSSharedName> m_SharedNames;
    DynArray<SHWSSharedInstance> m_SharedInsts;
    ~SHWSSharedList();
    unsigned Size()
    {
      return sizeof(SHWSSharedList) + sizeOfV(m_SharedNames) + sizeOfV(m_SharedInsts);
    }
		void GetMemoryUsage(ICrySizer* pSizer)const
		{
			pSizer->AddObject(this, sizeof(*this));
			pSizer->AddObject(m_SharedNames);			
			pSizer->AddObject(m_SharedInsts);			
		}
  };
  typedef std::map<CCryName, SHWSSharedList *> InstanceMap;
  typedef InstanceMap::iterator InstanceMapItor;

public:

  SHWSInstance *m_pCurInst;
  DynArray<SHWSInstance> m_Insts;

  static InstanceMap m_SharedInsts;

  static int m_FrameObj;

  // FX support
  DynArray<STexSampler> m_Samplers;
  DynArray<SFXParam> m_Params;
  int m_nCurInstFrame;

  // Bin FX support
  FXShaderToken m_Table;
  DynArray<uint32> m_TokenData;

  VIRTUAL int Size()
  {
		int nSize = sizeof(*this);
    nSize += sizeOfV(m_Insts);
    nSize += m_TokenData.get_alloc_size();
    nSize += m_Samplers.get_alloc_size();
    nSize += sizeOfV(m_Params);
    nSize += sizeOfMap(m_Table);

    return nSize;
  }

	VIRTUAL void GetMemoryUsage(ICrySizer* pSizer) const
	{				
		pSizer->AddObject(this, sizeof(*this));
		//pSizer->AddObject(m_pCurInst); // crahes.., looks like somewhere this ptr is not set back to NULL
		pSizer->AddObject(m_pCache);













		pSizer->AddObject( m_Insts );
		pSizer->AddObject( m_TokenData );
		pSizer->AddObject( m_Samplers );
		pSizer->AddObject( m_Params );
	}
  CHWShader_D3D()
  {
    m_pTree = NULL;
    mfConstruct();
  }
  static void mfInit();
  void mfConstruct()
  {
		if(ms_bInitShaders)
		{
#if !defined (XENON)
			memset(m_CurPSParams,0,sizeof(Vec4)*MAX_CONSTANTS_PS);
			memset(m_CurVSParams,0,sizeof(Vec4)*MAX_CONSTANTS_VS);
#endif
			ms_bInitShaders = false;
		}

    m_pCurInst = NULL;
    m_FrameObj = -1;
    m_pCache = NULL;
    m_nCurInstFrame = 0;

    m_dwShaderType = 0;
  }

  void mfFree(uint32 CRC32)
  {
    SAFE_DELETE(m_pTree);
    m_Flags = 0;
    mfReset(CRC32);
  }

  //============================================================================
  // Binary cache support
  SShaderCacheHeaderItem *mfGetCompressedItem(uint32 nFlags, uint32& nSize);
  SShaderCacheHeaderItem *mfGetCacheItem(uint32& nFlags, uint32& nSize);
  static bool mfAddCacheItem(SShaderCache *pCache, SShaderCacheHeaderItem *pItem, const byte *pData, int nLen, bool bFlush, CCryNameCRC Name);

  bool mfCloseCacheFile()
  {
    SAFE_RELEASE (m_pCache);

    return true;
  }

  static byte *mfBindsToCache(SHWSInstance *pInst, DynArray<SCGBind>* Binds, int nParams, byte *pP);
  byte *mfBindsFromCache(DynArray<SCGBind>*& Binds, int nParams, byte *pP);

  bool mfActivateCacheItem(SShaderCacheHeaderItem *pItem, uint32 nSize, uint32 nFlags);
  static bool mfCreateCacheItem(SHWSInstance *pInst, DynArray<SCGBind>& InstBinds, byte *pData, int nLen, CHWShader_D3D *pSH, bool bShaderThread);

  //============================================================================

  int mfGetParams(int Type)
  {
    assert(m_pCurInst);
    return m_pCurInst->m_nParams[Type];
  }

  bool mfSetHWStartProfile(uint32 nFlags);
  //bool mfNextProfile(uint32 nFlags);

  void mfSaveCGFile(const char *scr, const char *path);
  void mfOutputCompilerError(string& strErr, const char *szSrc);
  static bool mfCreateShaderEnv(int nThread, SHWSInstance *pInst, LPD3DXBUFFER pShader, LPD3DXCONSTANTTABLE pConstantTable, LPD3DXBUFFER pErrorMsgs, DynArray<SCGBind>& InstBindVars, CHWShader_D3D *pSH, bool bShaderThread, CShader *pFXShader, int nCombination, const char *src=NULL);
  void mfPrintCompileInfo(SHWSInstance *pInst);
  bool mfCompileHLSL_Int(char *prog_text, LPD3DXBUFFER* ppShader, LPD3DXCONSTANTTABLE *ppConstantTable, LPD3DXBUFFER* ppErrorMsgs, string& strErr, DynArray<SCGBind>& InstBindVars);

#if !defined(PS3)
  int mfAsyncCompileReady(SHWSInstance *pInst);
  bool mfRequestAsync(SHWSInstance *pInst, DynArray<SCGBind>& InstBindVars, const char *prog_text, const char *szProfile, const char *szEntry);
#endif

#ifdef WIN32
  bool mfPostCompilingFXC(SHWSInstance *pInst, const char *szNameSrc, const char *szNameDst, LPD3DXBUFFER* ppShader, LPD3DXCONSTANTTABLE *ppConstantTable);
  int mfCompileAsyncFXC(SHWSInstance *pInst);
#endif
  LPD3DXBUFFER mfCompileHLSL(char *prog_text, LPD3DXCONSTANTTABLE *ppConstantTable, LPD3DXBUFFER* ppErrorMsgs, uint32 nFlags, DynArray<SCGBind>& InstBindVars);
  bool mfUploadHW(SHWSInstance *pInst, byte *pBuf, uint32 nSize, CShader *pSH, uint32 nFlags);
  bool mfUploadHW(LPD3DXBUFFER pShader, SHWSInstance *pInst, CShader *pSH, uint32 nFlags);
  ED3DShError mfIsValid(SHWSInstance *&pInst, bool bFinalise);
  ED3DShError mfFallBack(SHWSInstance *&pInst, int nStatus);

  void mfBind()
  {
		PROFILE_LABEL_SHADER(GetName());
    HRESULT hr = S_OK;
    if (mfIsValid(m_pCurInst, true) == ED3DShError_Ok)
    {
      if (gRenDev->m_nFrameSwapID != m_pCurInst->m_nUsedFrame)
      {
        m_pCurInst->m_nUsedFrame = gRenDev->m_nFrameSwapID;
        m_pCurInst->m_nUsed++;
      }
#if defined(DIRECT3D9) || defined(OPENGL)
      if (m_eSHClass == eHWSC_Pixel)
        hr = gcpRendD3D->GetD3DDevice()->SetPixelShader((D3DPixelShader *)m_pCurInst->m_Handle.m_pShader->m_pHandle);
      else
        hr = gcpRendD3D->GetD3DDevice()->SetVertexShader((D3DVertexShader *)m_pCurInst->m_Handle.m_pShader->m_pHandle);










#else
      if (m_eSHClass == eHWSC_Pixel)
        gcpRendD3D->GetDeviceContext()->PSSetShader((D3DPixelShader *)m_pCurInst->m_Handle.m_pShader->m_pHandle, NULL, 0);
      else
      if (m_eSHClass == eHWSC_Vertex)
        gcpRendD3D->GetDeviceContext()->VSSetShader((D3DVertexShader *)m_pCurInst->m_Handle.m_pShader->m_pHandle, NULL, 0);
      else
      if (GEOMETRYSHADER_SUPPORT && m_eSHClass == eHWSC_Geometry)
        gcpRendD3D->GetDeviceContext()->GSSetShader((ID3D11GeometryShader *)m_pCurInst->m_Handle.m_pShader->m_pHandle, NULL, 0);
#endif
		}
    assert (SUCCEEDED(hr));
  }

  static void mfCommitParams(bool bSetPM);

#if defined (DIRECT3D9) || defined (OPENGL)
  static _inline void mfSetPSConst(int nReg, const float *vData, const int nParams)
  {
#ifdef OPENGL //XENON
    gcpRendD3D->GetD3DDevice()->SetPixelShaderConstantF(nReg, vData, nParams);
    return;
#else
    int i, nID;
    if ((nID=m_NumPSParamsToCommit)+nParams > 256)
      return;
    const Vec4 *vSrc = (Vec4 *)vData;
    Vec4 *vDst = &m_CurPSParams[nReg];
    for (i=0; i<nParams; i++)
    {
      if (vSrc[i] != vDst[i])
      {
        memcpy(vDst, vSrc, sizeof(Vec4)*nParams);
 #if defined (MERGE_SHADER_PARAMETERS)
        if (nID+nParams < 64)
        {
          for (i=0; i<nParams; i++)
          {
            m_PSParamsToCommit[nID++] = i+nReg;
          }
          m_NumPSParamsToCommit = nID;
        }
 #else
        gcpRendD3D->GetD3DDevice()->SetPixelShaderConstantF(nReg, vData, nParams);
 #endif
        break;
      }
    }
#endif
  }

  static _inline void mfSetPSConstA(int nReg, const float *vData, const int nParams)
  {
#ifdef OPENGL //XENON
    gcpRendD3D->GetD3DDevice()->SetPixelShaderConstantF(nReg, vData, nParams);
    return;
#else
    int i, nID;
    if ((nID=m_NumPSParamsToCommit)+nParams > 256)
      return;
    const Vec4 *const __restrict vSrc = (Vec4 *)vData;
    Vec4 *const __restrict vDst = (Vec4 *)&m_CurPSParams[nReg];
    for (i=0; i<nParams; i++)
    {



















 #if defined(_CPU_SSE) && !defined(_DEBUG)
      __m128 a = _mm_load_ps(&vSrc[i].x);
      __m128 b = _mm_load_ps(&vDst[i].x);
      __m128 mask = _mm_cmpneq_ps(a, b);
      int maskBits = _mm_movemask_ps(mask);
      if (maskBits != 0)
      {
        assert(nID+nParams < 64);
        for (i=0; i<nParams; i++)
        {
          a = _mm_load_ps(&vSrc[i].x);
          m_PSParamsToCommit[nID++] = nReg+i;
          _mm_store_ps(&vDst[i].x, a);
        }
        m_NumPSParamsToCommit = nID;
  #if !defined (MERGE_SHADER_PARAMETERS)
        gcpRendD3D->GetD3DDevice()->SetPixelShaderConstantF(nReg, vData, nParams);
  #endif
        break;
      }
 #else
      if (vSrc[i] != vDst[i])
      {
        memcpy(vDst, vSrc, sizeof(Vec4)*nParams);

  #if defined (MERGE_SHADER_PARAMETERS)
        if (nID+nParams < 64)
        {
          for (i=0; i<nParams; i++)
          {
            m_PSParamsToCommit[nID++] = i+nReg;
          }
          m_NumPSParamsToCommit = nID;
        }
  #else
        gcpRendD3D->GetD3DDevice()->SetPixelShaderConstantF(nReg, vData, nParams);
  #endif
        break;
      }
 #endif
    }
#endif
  }

  static _inline void mfSetVSConst(int nReg, const float *vData, const int nParams)
  {
#ifdef OPENGL //XENON
    gcpRendD3D->GetD3DDevice()->SetVertexShaderConstantF(nReg, vData, nParams);
    return;
#else
    int i, nID;
    if ((nID=m_NumVSParamsToCommit)+nParams > 256)
      return;
    const Vec4 *vSrc = (Vec4 *)vData;
    Vec4 *vDst = &m_CurVSParams[nReg];
    for (i=0; i<nParams; i++)
    {
      assert(nReg+i>=0 && nReg+i<MAX_CONSTANTS_VS);
      if (vSrc[i] != vDst[i])
      //XMVECTOR a = XMLoadFloat4A((XMFLOAT4*)&v[0]);
      //XMVECTOR b = XMLoadFloat4A((XMFLOAT4*)&pC[0]);
      //if (XMVector4NotEqual(a, b))
      {
#ifndef XENON
        memcpy(vDst, vSrc, sizeof(Vec4)*nParams);



#endif
#if defined (MERGE_SHADER_PARAMETERS)
        for (i=0; i<nParams; i++)
        {












          m_VSParamsToCommit[nID++] = nReg+i;
        }
        m_NumVSParamsToCommit = nID;
#else
        gcpRendD3D->GetD3DDevice()->SetVertexShaderConstantF(nReg, vData, nParams);
#endif
        break;
      }
    }
#endif
  }

  static _inline void mfSetVSConstA(int nReg, const float *vData, const int nParams)
  {
#ifdef OPENGL //XENON
    gcpRendD3D->GetD3DDevice()->SetVertexShaderConstantF(nReg, vData, nParams);
    return;
#else

    int i, nID;
    if ((nID=m_NumVSParamsToCommit)+nParams > 256)
      return;
    const Vec4 *vSrc = (Vec4 *)vData;
    Vec4 *vDst = &m_CurVSParams[nReg];
    for (i=0; i<nParams; i++)
    {
      assert(nReg+i>=0 && nReg+i<MAX_CONSTANTS_VS);




















 #if defined(_CPU_SSE) && !defined(_DEBUG)
      __m128 a = _mm_load_ps(&vSrc[i].x);
      __m128 b = _mm_load_ps(&vDst[i].x);
      __m128 mask = _mm_cmpneq_ps(a, b);
      int maskBits = _mm_movemask_ps(mask);
      if (maskBits != 0)
      {
        assert(nID+nParams < 256);
        for (i=0; i<nParams; i++)
        {
          a = _mm_load_ps(&vSrc[i].x);
          m_VSParamsToCommit[nID++] = nReg+i;
          _mm_store_ps(&vDst[i].x, a);
        }
        m_NumVSParamsToCommit = nID;
  #if !defined (MERGE_SHADER_PARAMETERS)
        gcpRendD3D->GetD3DDevice()->SetVertexShaderConstantF(nReg, vData, nParams);
  #endif
        break;
      }
#else
      if (vSrc[i] != vDst[i])
      {
        memcpy(vDst, vSrc, sizeof(Vec4)*nParams);
  #if defined (MERGE_SHADER_PARAMETERS)
        for (i=0; i<nParams; i++)
        {
          m_VSParamsToCommit[nID++] = i+nReg;
        }
        m_NumVSParamsToCommit = nID;
  #else
        gcpRendD3D->GetD3DDevice()->SetVertexShaderConstantF(nReg, vData, nParams);
  #endif
        break;
      }
 #endif
    }
#endif
  }

  static _inline void mfParameterReg(int nReg, const float *v, const int nComps, EHWShaderClass eSHClass)
  {
#if !defined(OPENGL) && !defined(PS3)
    assert(nReg>=0 && nReg<MAX_CONSTANTS_VS && nReg+nComps<MAX_CONSTANTS_VS);
#endif
    if (eSHClass == eHWSC_Pixel)
      mfSetPSConst(nReg, v, nComps);
    else
    if (eSHClass == eHWSC_Vertex)
      mfSetVSConst(nReg, v, nComps);
#if defined (DIRECT3D10)
    else
    if (GEOMETRYSHADER_SUPPORT && eSHClass == eHWSC_Geometry)
      mfSetGSConst(nReg, v, nComps);
#endif
  }

  static _inline void mfSetVSConstI(int nReg, const int *vData, int nParams)
  {
    gcpRendD3D->GetD3DDevice()->SetVertexShaderConstantI(nReg, vData, nParams);
  }
  static _inline void mfSetPSConstI(int nReg, const int *vData, int nParams)
  {
    gcpRendD3D->GetD3DDevice()->SetPixelShaderConstantI(nReg, vData, nParams);
  }

  static _inline void mfParameterRegI(int nReg, const int *v, int nComps, EHWShaderClass eSHClass)
  {
    if (eSHClass == eHWSC_Pixel)
      mfSetPSConstI(nReg, v, nComps);
    else
    if (eSHClass == eHWSC_Vertex)
      mfSetVSConstI(nReg, v, nComps);
  }
  static _inline void mfParameterRegA(int nReg, const float *v, const int nComps, EHWShaderClass eSHClass)
  {
#if !defined(OPENGL) && !defined(PS3) && !defined(XENON)
    if (eSHClass == eHWSC_Vertex && (nReg<0 || nReg+nComps>MAX_CONSTANTS_VS))
    {
      assert(0);
      iLog->Log("Exceed maximum number of constants (%d) for vertex shader", MAX_CONSTANTS_VS);
    }
    else
    if (eSHClass == eHWSC_Pixel && (nReg<0 || nReg+nComps>MAX_CONSTANTS_PS))
    {
      assert(0);
      iLog->Log("Exceed maximum number of constants (%d) for pixel shader", MAX_CONSTANTS_PS);
    }
#endif
    if (eSHClass == eHWSC_Pixel)
      mfSetPSConstA(nReg, v, nComps);
    else
    if (eSHClass == eHWSC_Vertex)
      mfSetVSConstA(nReg, v, nComps);
#if defined (DIRECT3D10)
    else
    if (GEOMETRYSHADER_SUPPORT && eSHClass == eHWSC_Geometry)
      mfSetGSConst(nReg, v, nComps);
#endif
  }


  static _inline void mfParameterReg_NoCheck(int nReg, const float *v, int nComps, EHWShaderClass eSHClass)
  {
    if (eSHClass == eHWSC_Pixel)
      gcpRendD3D->GetD3DDevice()->SetPixelShaderConstantF(nReg, v, nComps);
    else
      gcpRendD3D->GetD3DDevice()->SetVertexShaderConstantF(nReg, v, nComps);
  }

  static void mfParameterf(const SCGBind *ParamBind, const float *v, int nComps, EHWShaderClass eSHClass)
  {
    if(!ParamBind || ParamBind->m_dwBind < 0)
      return;
		int nReg = ParamBind->m_dwBind;
    mfParameterReg(nReg, v, nComps, eSHClass);
  }
  static _inline void mfParameterfA(const SCGBind *ParamBind, const float *v, int nComps, EHWShaderClass eSHClass)
  {
    if(!ParamBind || ParamBind->m_dwBind < 0)
      return;
    int nReg = ParamBind->m_dwBind;
    mfParameterRegA(nReg, v, nComps, eSHClass);
  }

  static void mfParameterf(const SCGBind *ParamBind, const float *v, EHWShaderClass eSHClass)
  {
		if(!ParamBind || ParamBind->m_dwBind < 0)
			return;
		int nReg = ParamBind->m_dwBind;
    mfParameterReg(nReg, v, ParamBind->m_nParameters, eSHClass);
  }
  static void mfParameterfA(const SCGBind *ParamBind, const float *v, EHWShaderClass eSHClass)
  {
    if(!ParamBind || ParamBind->m_dwBind < 0)
      return;
    int nReg = ParamBind->m_dwBind;
    mfParameterRegA(nReg, v, ParamBind->m_nParameters, eSHClass);
  }
  static void mfParameteri(const SCGBind *ParamBind, const float *v, EHWShaderClass eSHClass)
  {
    if(!ParamBind)
      return;
    assert(ParamBind->m_dwBind >= 0 && ParamBind->m_nParameters >= 1);
    if ((int)ParamBind->m_dwBind < 0)
      return;
#ifndef XENON
    int iparms[4];
    int n = ParamBind->m_dwBind;
    if (eSHClass == eHWSC_Pixel)
    {
      if (m_CurPSParamsI[n].x != v[0] || m_CurPSParamsI[n].y != v[1] || m_CurPSParamsI[n].z != v[2] || m_CurPSParamsI[n].w != v[3])
      {
        m_CurPSParamsI[n].x = v[0];
        m_CurPSParamsI[n].y = v[1];
        m_CurPSParamsI[n].z = v[2];
        m_CurPSParamsI[n].w = v[3];

        iparms[0] = (int)v[0];
        iparms[1] = (int)v[1];
        iparms[2] = (int)v[2];
        iparms[3] = (int)v[3];
        gcpRendD3D->GetD3DDevice()->SetPixelShaderConstantI(n, iparms, 1);
      }
    }
    else
    {
      if (m_CurVSParamsI[n].x != v[0] || m_CurVSParamsI[n].y != v[1] || m_CurVSParamsI[n].z != v[2] || m_CurVSParamsI[n].w != v[3])
      {
        m_CurVSParamsI[n].x = v[0];
        m_CurVSParamsI[n].y = v[1];
        m_CurVSParamsI[n].z = v[2];
        m_CurVSParamsI[n].w = v[3];

        iparms[0] = (int)v[0];
        iparms[1] = (int)v[1];
        iparms[2] = (int)v[2];
        iparms[3] = (int)v[3];
        gcpRendD3D->GetD3DDevice()->SetVertexShaderConstantI(n, iparms, 1);
      }
    }
    v += 4;


#endif
  }
  static void mfParameterb(const SCGBind *ParamBind, const float *v, EHWShaderClass eSHClass)
  {
    if(!ParamBind)
      return;

    assert(ParamBind->m_dwBind >= 0 && ParamBind->m_nParameters >= 1);
    if ((int)ParamBind->m_dwBind < 0)
      return;

#ifndef XENON
    BOOL iparms;
    int n = ParamBind->m_dwBind;
    if (eSHClass == eHWSC_Pixel)
    {
      if (m_CurPSParams[n].x != v[0] || m_CurPSParams[n].y != v[1] || m_CurPSParams[n].z != v[2] || m_CurPSParams[n].w != v[3])
      {
        m_CurPSParams[n].x = v[0];
        m_CurPSParams[n].y = v[1];
        m_CurPSParams[n].z = v[2];
        m_CurPSParams[n].w = v[3];

        iparms = (BOOL)v[0];
        gcpRendD3D->GetD3DDevice()->SetPixelShaderConstantB(n, &iparms, 1);
      }
    }
    else
    {
      if (m_CurVSParams[n].x != v[0] || m_CurVSParams[n].y != v[1] || m_CurVSParams[n].z != v[2] || m_CurVSParams[n].w != v[3])
      {
        m_CurVSParams[n].x = v[0];
        m_CurVSParams[n].y = v[1];
        m_CurVSParams[n].z = v[2];
        m_CurVSParams[n].w = v[3];

        iparms = (BOOL)v[0];
        gcpRendD3D->GetD3DDevice()->SetVertexShaderConstantB(n, &iparms, 1);
      }
    }
    v += 4;


#endif
  }

  static _inline bool mfBindPSNULL()
  {
    HRESULT hr = S_OK;

    /*if (!m_pCurPS)
    {
      hr = gcpRendD3D->GetD3DDevice()->SetPixelShader(NULL);
    }*/

    hr = gcpRendD3D->GetD3DDevice()->SetPixelShader(NULL);

    if (SUCCEEDED(hr))
    {
      m_pCurPS = NULL;
      return true;
    }

    return false;
  }

  static _inline void mfBindGS(SD3DShader *pShader, void *pHandle){}

#else // defined(DIRECT3D9) || defined(OPENGL)

  static _inline bool mfBindPSNULL() {return false;}

  static _inline void mfBindGS(SD3DShader *pShader, void *pHandle)
  {
    if (m_pCurGS != pShader)
    {
      m_pCurGS = pShader;
      gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_NumGShadChanges++;
      gcpRendD3D->GetDeviceContext()->GSSetShader((ID3D11GeometryShader *)pHandle, NULL, 0);
    }
    if (!m_pCurGS)
      m_pCurInstGS = NULL;
  }
  
  struct SCBuffer
  {
    ID3D11Buffer *pBuf;
    uint32 nMask;
#ifdef _DEBUG
    std::vector<Vec4> Vectors;
#endif
  };
  static std::vector<SCBuffer> m_CB_SI;
  static std::vector<ID3D11Buffer *> m_CB_SI_Released[CB_SI_MAXVECS];
  static ID3D11Buffer *m_CB_SI_Staged[CB_SI_MAXVECS];
  static std::vector<int> m_CB_SI_ReleasedID;
  static int mfGetCB_SI(int nMaxVecs, CShader *pFXShader);
  static void mfValidateCB_SI(CRenderObject *pObj, CShader *pFXShader, int nMaxVexs);
  static ID3D11Buffer *mfGetCB_SI_Interface(int nCBID)
  {
    assert(nCBID >= 0 && nCBID < m_CB_SI.size());
    return m_CB_SI[nCBID].pBuf;
  }

  static _inline void mfSetCB(int eClass, int nSlot, ID3D11Buffer *pBuf)
  {
//PS3workaround





    if (m_pCurDevCB[eClass][nSlot] != pBuf)
    {
      m_pCurDevCB[eClass][nSlot] = pBuf;

      switch (eClass)
      {
        case eHWSC_Vertex:
          gcpRendD3D->m_pd3dDeviceContext->VSSetConstantBuffers(nSlot, 1, &pBuf);
          break;
        case eHWSC_Pixel:
          gcpRendD3D->m_pd3dDeviceContext->PSSetConstantBuffers(nSlot, 1, &pBuf);
          break;
#ifndef PS3
        case eHWSC_Geometry:
          gcpRendD3D->m_pd3dDeviceContext->GSSetConstantBuffers(nSlot, 1, &pBuf);
          break;
#endif
      }
    }
  }
  static _inline void mfCommitCB(int nCBufSlot, EHWShaderClass eSH)
  {
    if (!m_pDataCB[eSH][nCBufSlot])
      return;
		D3DDevice* dv = gcpRendD3D->GetD3DDevice();
		D3DDeviceContext* context = gcpRendD3D->GetDeviceContext();
    assert(m_pCurReqCB[eSH][nCBufSlot]);

    if ((int)m_pDataCB[eSH][nCBufSlot] != 1)
			context->Unmap(m_pCurReqCB[eSH][nCBufSlot], 0);





     m_pDataCB[eSH][nCBufSlot] = NULL;
    mfSetCB(eSH, nCBufSlot, m_pCurReqCB[eSH][nCBufSlot]);
  }
  static _inline void mfSetCBConst(int nReg, int nCBufSlot, EHWShaderClass eSH, const float *fData, const int nVecs, int nMaxVecs)
  {




    assert(nCBufSlot >= 0 || nCBufSlot < CB_MAX);
    assert(m_pCB[eSH][nCBufSlot]);
		//assert(nReg + nVecs <= nMaxVecs);
    if (nReg+nVecs > nMaxVecs)
    {
      iLog->Log("ERROR: Attempt to modify CB: %d outside of the range (%d+%d > %d) (Shader: %s)", nCBufSlot, nReg, nVecs, nMaxVecs, gRenDev->m_RP.m_pShader ? gRenDev->m_RP.m_pShader->GetName() : "Unknown");
      return;
    }
    if (m_pDataCB[eSH][nCBufSlot] && m_nCurMaxVecs[eSH][nCBufSlot] != nMaxVecs)
      mfCommitCB(nCBufSlot, eSH);
		if (!m_pDataCB[eSH][nCBufSlot])
    {
      m_nCurMaxVecs[eSH][nCBufSlot] = nMaxVecs;










			{
				if (!m_pCB[eSH][nCBufSlot][nMaxVecs] && nMaxVecs)
				{
					D3D11_BUFFER_DESC bd;
					ZeroStruct(bd);
					HRESULT hr;

					bd.Usage = D3D11_USAGE_DYNAMIC;
					bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
					bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					bd.MiscFlags = 0;
					bd.ByteWidth = nMaxVecs * sizeof(Vec4);
					hr = gcpRendD3D->m_pd3dDevice->CreateBuffer(&bd, NULL, &m_pCB[eSH][nCBufSlot][nMaxVecs]);
					assert(SUCCEEDED(hr));
				}

				{
					m_pCurReqCB[eSH][nCBufSlot] = m_pCB[eSH][nCBufSlot][nMaxVecs];
					STALL_PROFILER("set const_buffer");
					D3D11_MAPPED_SUBRESOURCE mappedResource;
					gcpRendD3D->GetDeviceContext()->Map(m_pCurReqCB[eSH][nCBufSlot], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
					m_pDataCB[eSH][nCBufSlot] = (Vec4*)mappedResource.pData;
				}
			}

      if (CD3D9Renderer::CV_d3d10_CBUpdateStats)
			{
				static unsigned int s_lastFrame(0);
				static unsigned int s_numCalls(0);
				static unsigned int s_minUpdateBytes(0);
				static unsigned int s_maxUpdateBytes(0);
				static unsigned int s_totalUpdateBytes(0);

				unsigned int updateBytes = (unsigned int) (nMaxVecs * sizeof(Vec4));
				unsigned int curFrame = gcpRendD3D->GetFrameID(false);
				if (s_lastFrame != curFrame)
				{
					if (s_lastFrame != 0)
					{
						unsigned int avgUpdateBytes = s_totalUpdateBytes / s_numCalls;
						gEnv->pLog->Log("-------------------------------------------------------");
						gEnv->pLog->Log("CB update statistics for frame %d:", s_lastFrame);
						gEnv->pLog->Log("#UpdateSubresource() = %d calls", s_numCalls);
						gEnv->pLog->Log("SmallestTransfer = %d kb (%d bytes)", (s_minUpdateBytes + 1023) >> 10, s_minUpdateBytes);
						gEnv->pLog->Log("BiggestTransfer = %d kb (%d bytes)", (s_maxUpdateBytes + 1023) >> 10, s_maxUpdateBytes);
						gEnv->pLog->Log("AvgTransfer = %d kb (%d bytes)", (avgUpdateBytes + 1023) >> 10, avgUpdateBytes);
						gEnv->pLog->Log("TotalTransfer = %d kb (%d bytes)", (s_totalUpdateBytes + 1023) >> 10, s_totalUpdateBytes);						
					}

					s_lastFrame = curFrame;
					s_numCalls = 1;
					s_minUpdateBytes = updateBytes;
					s_maxUpdateBytes = updateBytes;
					s_totalUpdateBytes = updateBytes;
				}
				else
				{
					++s_numCalls;
					s_minUpdateBytes = min(updateBytes, s_minUpdateBytes);
					s_maxUpdateBytes = max(updateBytes, s_maxUpdateBytes);
					s_totalUpdateBytes += updateBytes;
				}
			}
    }
    else
    {
      assert(m_nCurMaxVecs[eSH][nCBufSlot] == nMaxVecs);
    }
    const Vec4 *vData = (const Vec4 *)fData;
#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
      for (int i=0; i<nVecs; i++)
      {
        gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%d: (%.3f, %.3f, %.3f, %.3f)", i+nReg, vData[i][0], vData[i][1], vData[i][2], vData[i][3]);
      }
      gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "\n");
    }
#endif














		{
			memcpy(&m_pDataCB[eSH][nCBufSlot][nReg], vData, nVecs<<4);
		}

    if (nCBufSlot == CB_PER_FRAME && eSH == eHWSC_Vertex && vData != &m_CurVSParams[0])
      memcpy(&m_CurVSParams[nReg], vData, nVecs<<4);
  }
  static _inline void mfSetGSConst(int nReg, int nCBufSlot, const float *vData, int nParams, int nMaxVecs=32)
  {
#ifndef PS3
    mfSetCBConst(nReg, nCBufSlot, eHWSC_Geometry, vData, nParams, nMaxVecs);
#endif
  }

  static _inline void mfSetPSConst(int nReg, int nCBufSlot, const float *vData, int nParams, int nMaxVecs=32)
  {
    mfSetCBConst(nReg, nCBufSlot, eHWSC_Pixel, vData, nParams, nMaxVecs);
  }

  static _inline void mfSetPSConstA(int nReg, int nCBufSlot, const float *vData, int nParams, int nMaxVecs=32)
  {
    mfSetCBConst(nReg, nCBufSlot, eHWSC_Pixel, vData, nParams, nMaxVecs);
  }

  static _inline void mfSetVSConst(int nReg, int nCBufSlot, const float *vData, int nParams, int nMaxVecs=32)
  {
    mfSetCBConst(nReg, nCBufSlot, eHWSC_Vertex, vData, nParams, nMaxVecs);
  }

  static _inline void mfSetVSConstA(int nReg, int nCBufSlot, const float *vData, int nParams, int nMaxVecs=32)
  {
    mfSetCBConst(nReg, nCBufSlot, eHWSC_Vertex, vData, nParams, nMaxVecs);
  }
  static _inline void mfSetPSConst(int nReg, const float *vData, int nParams, int nMaxVecs=32)
  {
    mfSetPSConst(nReg, CB_PER_BATCH, vData, nParams, nMaxVecs);
  }
  static _inline void mfSetVSConst(int nReg, const float *vData, int nParams, int nMaxVecs=32)
  {
    mfSetVSConst(nReg, CB_PER_BATCH, vData, nParams, nMaxVecs);
  }

  static _inline void mfSetGSConst(int nReg, const float *vData, int nParams, int nMaxVecs=32)
  {
    mfSetGSConst(nReg, CB_PER_BATCH, vData, nParams, nMaxVecs);
  }

  static _inline void mfParameterReg(int nReg, int nCBufSlot, EHWShaderClass eSH, const float *v, int nComps, int nMaxVecs)
  {
    mfSetCBConst(nReg, nCBufSlot, eSH, v, nComps, nMaxVecs);
  }

  static void ILINE mfParameterf(const SCGBind *ParamBind, const float *v, int nComps, EHWShaderClass eSH, int nMaxVecs)
  {
    if(!ParamBind || ParamBind->m_dwBind < 0)
      return;
		int nReg = ParamBind->m_dwBind;
    mfParameterReg(nReg, ParamBind->m_dwCBufSlot, eSH, v, nComps, nMaxVecs);
  }
  static void ILINE mfParameterfA(const SCGBind *ParamBind, const float *v, int nComps, EHWShaderClass eSH, int nMaxVecs)
  {
    if(!ParamBind || ParamBind->m_dwBind < 0)
      return;
    int nReg = ParamBind->m_dwBind;
    mfParameterReg(nReg, ParamBind->m_dwCBufSlot, eSH, v, nComps, nMaxVecs);
  }

  static void mfParameterf(const SCGBind *ParamBind, const float *v, EHWShaderClass eSH, int nMaxVecs)
  {
		if(!ParamBind || ParamBind->m_dwBind < 0)
			return;
		int nReg = ParamBind->m_dwBind;
    mfParameterReg(nReg, ParamBind->m_dwCBufSlot, eSH, v, ParamBind->m_nParameters, nMaxVecs);
  }
  static void mfParameterfA(const SCGBind *ParamBind, const float *v, EHWShaderClass eSH, int nMaxVecs)
  {
    if(!ParamBind || ParamBind->m_dwBind < 0)
      return;
    int nReg = ParamBind->m_dwBind;
    mfParameterReg(nReg, ParamBind->m_dwCBufSlot, eSH, v, ParamBind->m_nParameters, nMaxVecs);
  }
  static void mfParameteri(const SCGBind *ParamBind, const float *v, EHWShaderClass eSH, int nMaxVecs)
  {
    if(!ParamBind)
      return;
    int nReg = ParamBind->m_dwBind;
    mfParameterReg(nReg, ParamBind->m_dwCBufSlot, eSH, v, ParamBind->m_nParameters, nMaxVecs);
  }
  static void mfParameterb(const SCGBind *ParamBind, const float *v, EHWShaderClass eSH, int nMaxVecs)
  {
    if(!ParamBind)
      return;

    assert(ParamBind->m_dwBind >= 0 && ParamBind->m_nParameters >= 1);
    if ((int)ParamBind->m_dwBind < 0)
      return;

    int nReg = ParamBind->m_dwBind;
    mfParameterReg(nReg, ParamBind->m_dwCBufSlot, eSH, v, ParamBind->m_nParameters, nMaxVecs);
  }
#endif

  SCGBind *mfGetParameterBind(const CCryName& Name)
  {
    if (!m_pCurInst)
      return NULL;

    std::vector<SCGBind>& pBinds = m_pCurInst->m_pBindVars;
    //if (!pBinds)
    //  return NULL;
    int i;
    int nSize = pBinds.size();
    for (i=0; i<nSize; i++)
    {
      if (Name == pBinds[i].m_Name)
        return &pBinds[i];
    }
    return NULL;
  }
  void mfParameterf(const CCryName& Name, const float *v)
  {
    SCGBind *pBind = mfGetParameterBind(Name);
    if (pBind)
    {
#if defined (DIRECT3D10)
      mfParameterf(pBind, v, m_eSHClass, m_pCurInst->m_nMaxVecs[CB_PER_BATCH]);
#else
      mfParameterf(pBind, v, m_eSHClass);
#endif
    }
  }

#if defined (DIRECT3D10)
  static float *mfSetParametersPI(SCGParam *pParams, const int nParams, float *pDst, EHWShaderClass eSH, int nMaxRegs); // handles only PI and SI parameters
  static void mfSetParameters(SCGParam *pParams, const int nParams, EHWShaderClass eSH, int nMaxRegs); // handles all the parameter except PI and SI ones
#else
  static float *mfSetParametersPI(SCGParam *pParams, const int nParams, float *pDst, EHWShaderClass eSH); // handles only PI and SI parameters
  static void mfSetParameters(SCGParam *pParams, const int nParams, EHWShaderClass eSH); // handles all the parameter except PI and SI ones
#endif

  //============================================================================

  void mfLostDevice(SHWSInstance *pInst, byte *pBuf, int nSize)
  {
    pInst->m_Handle.SetFake();
    pInst->m_Handle.m_pData = new byte[nSize];
    memcpy(pInst->m_Handle.m_pData, pBuf, nSize);
    pInst->m_Handle.m_nData = nSize;
  }

  int mfCheckActivation(SHWSInstance *&pInst, uint32 nFlags)
  {
    ED3DShError eError = mfIsValid(pInst, true);
    if (eError ==  ED3DShError_NotCompiled)
    {
      if (!mfActivate(nFlags))
      {
        pInst = m_pCurInst;
        if (gRenDev->m_cEF.m_bActivatePhase)
          return 0;
        if (!pInst->IsAsyncCompiling())
          pInst->m_Handle.SetNonCompilable();
        else
        {
          eError = mfIsValid(pInst, true);
          if (eError == ED3DShError_CompilingError)
            return 0;
          if (m_eSHClass == eHWSC_Vertex)
            return 1;
          else
            return -1;
        }
        return 0;
      }
      if (gRenDev->m_RP.m_pCurTechnique)
        mfGetPreprocessFlags(gRenDev->m_RP.m_pCurTechnique);
      pInst = m_pCurInst;
    }
    else
    if (eError == ED3DShError_Fake)
    {
      if (pInst->m_Handle.m_pData)
      {
        if (gRenDev && !gRenDev->CheckDeviceLost())
        {
          mfUploadHW(pInst, pInst->m_Handle.m_pData, pInst->m_Handle.m_nData, gRenDev->m_RP.m_pShader, nFlags);
          SAFE_DELETE_ARRAY(pInst->m_Handle.m_pData);
          pInst->m_Handle.m_nData = 0;
        }
        else
          eError = ED3DShError_CompilingError;
      }
    }
    if (eError == ED3DShError_CompilingError)
      return 0;
    return 1;
  }



  void mfSetForOverdraw(SHWSInstance *pInst, uint32 nFlags, uint64& RTMask);

  _inline void mfSetParametersPI(CRenderObject *pObj, CShader *pFXShader)
  {
    if (!m_pCurInst)
      return;
    SHWSInstance *pInst = m_pCurInst;
    if (pInst->m_nParams[1] >= 0)
    {
      SCGParamsGroup& Group = CGParamManager::m_Groups[pInst->m_nParams[1]];
#if defined (DIRECT3D10)
      mfSetParametersPI(Group.pParams, Group.nParams, NULL, m_eSHClass, pInst->m_nMaxVecs[1]);
#else
      mfSetParametersPI(Group.pParams, Group.nParams, NULL, m_eSHClass);
#endif
    }
#if defined (DIRECT3D10)
    int nSize;
    if (pFXShader && (nSize = pFXShader->m_nInstParams))
    {
      int nMaxVecs = pFXShader->m_InstParams[nSize-1].m_dwBind + pFXShader->m_InstParams[nSize-1].m_nParameters;
      if (!pObj || !(pObj->m_ObjFlags & FOB_PERMANENT) || !CRenderer::CV_r_CBStatic)
        mfSetParametersPI(&pFXShader->m_InstParams[0], nSize, NULL, m_eSHClass, nMaxVecs);
      else
      {
        if (pObj->m_nCBID < 0)
          pObj->m_nCBID = mfGetCB_SI(nMaxVecs, pFXShader);
        else
        {
          SCBuffer& cb = m_CB_SI[pObj->m_nCBID];
          if (cb.nMask != pFXShader->m_nMaskCB)
          {
            mfReleaseCB_SI(pObj->m_nCBID);
            pObj->m_nCBID = mfGetCB_SI(nMaxVecs, pFXShader);
          }
#ifdef _DEBUG
          else
          if (CRenderer::CV_r_CBStaticDebug)
          {
            mfValidateCB_SI(pObj, pFXShader, nMaxVecs);
          }
#endif
        }
        assert(pObj->m_nCBID>=0 && m_CB_SI[pObj->m_nCBID].nMask == pFXShader->m_nMaskCB);
        mfSetCB(eHWSC_Vertex, CB_STATIC_INSTANCE, mfGetCB_SI_Interface(pObj->m_nCBID));
      }
    }
#endif
  }
  _inline void mfSetParametersPB()
  {
    if (!m_pCurInst)
      return;
    SHWSInstance *pInst = m_pCurInst;
    if (pInst->m_nParams[0] >= 0)
    {
      SCGParamsGroup& Group = CGParamManager::m_Groups[pInst->m_nParams[0]];
#if defined (DIRECT3D10)
      mfSetParameters(Group.pParams, Group.nParams, m_eSHClass, pInst->m_nMaxVecs[0]);
#else
      mfSetParameters(Group.pParams, Group.nParams, m_eSHClass);
#endif
    }
  }
  bool mfSetSamplers();
  DynArray<SHWSInstance> *mfGetSharedInstContainer(bool bCreate, uint64 GLMask, bool bPrecache);
  SHWSInstance *mfGetInstance(uint64 RTMask, uint32 LightMask, uint64 GLMask, uint32 MDMask, uint32 MDVMask, uint32 nFlags);
  SHWSInstance *mfGetInstance(int nInstance, uint64 GLMask);
  static void mfPrepareShaderDebugInfo(SHWSInstance *pInst, CHWShader_D3D *pSH, const char *szAsm, DynArray<SCGBind>& InstBindVars, LPD3DXCONSTANTTABLE pBuffer);
  void mfGetSrcFileName(char *srcName, int nSize);
  static void mfGetDstFileName(SHWSInstance *pInst, CHWShader_D3D *pSH, char *dstname, int nSize, byte bType);
  static void mfGenName(SHWSInstance *pInst, char *dstname, int nSize, byte bType);
  void CorrectScriptEnums(CParserBin& Parser, SHWSInstance *pInst, DynArray<SCGBind>& InstBindVars, FXShaderToken* Table);
  bool ConvertBinScriptToASCII(CParserBin& Parser, SHWSInstance *pInst, DynArray<SCGBind>& InstBindVars, FXShaderToken* Table, TArray<char>& Scr);
  void RemoveUnaffectedParameters_D3D10(CParserBin& Parser, SHWSInstance *pInst, DynArray<SCGBind>& InstBindVars);
  bool mfStoreCacheTokenMap(FXShaderToken*& Table, DynArray<uint32>*& pSHData, const char *szName);
  void mfGetTokenMap(CResFile *pRes, SDirEntry *pDE, FXShaderToken*& Table, DynArray<uint32>*& pSHData);
  void mfSetDefaultRT(uint64& nAndMask, uint64& nOrMask);

public:
  bool mfGetCacheTokenMap(FXShaderToken*& Table, DynArray<uint32>*& pSHData, uint64 nMaskGen);
  char *mfGenerateScript(SHWSInstance *&pInst, DynArray<SCGBind>& InstBindVars, uint32 nFlags, FXShaderToken* Table, DynArray<uint32>* pSHData);
  bool mfActivate(uint32 nFlags, FXShaderToken* Table=NULL, DynArray<uint32>* pSHData=NULL);

  void SetTokenFlags(uint32 nToken);
  uint64 CheckToken(uint32 nToken);
  uint64 CheckIfExpr_r(uint32 *pTokens, uint32& nCur, uint32 nSize);
  void mfConstructFX_Mask_RT(FXShaderToken* Table, DynArray<uint32>* pSHData);
  void mfConstructFX(FXShaderToken* Table, DynArray<uint32>* pSHData);
 
  static void mfAddFXParameter(SHWSInstance *pInst, SParamsGroup& OutParams, DynArray<SFXParam>& Params, DynArray<STexSampler>& Samplers, SFXParam *pr, const char *ParamName, SCGBind *pBind, CShader *ef, bool bInstParam, EHWShaderClass eSHClass);
  static bool mfAddFXParameter(SHWSInstance *pInst, SParamsGroup& OutParams, DynArray<SFXParam>& Params, DynArray<STexSampler>& Samplers, const char *param, const char *paramINT, SCGBind *bn, bool bInstParam, EHWShaderClass eSHClass, CShader *pFXShader);
  static void mfGatherFXParameters(SHWSInstance *pInst, std::vector<SCGBind>* BindVars, DynArray<SCGBind> *InstBindVars, CHWShader_D3D *pSH, int nFlags, CShader *pFXShader);

#if !defined(PS3)
  static void AnalyzeSemantic(SHWSInstance *pInst, DynArray<SFXParam>& Params, D3DXSEMANTIC *pSM, bool bUsed, bool& bPos, byte& bNormal, bool bTangent[2], bool bBinorm[2], bool& bHWSkin, bool& bShapeDeform, bool& bMorphTarget, bool& bBoneSpace, bool& bPSize, bool bSH[], bool& bMorph, bool& bTC0, bool bTC1[], bool& bCol, bool& bSecCol, DynArray<SCGBind>& InstBindVars);
#endif
  static void AddMissedInstancedParam(SHWSInstance *pInst, DynArray<SFXParam>& Params, int nIndex, DynArray<SCGBind>& InstBindVars);
  static void mfCreateBinds(SHWSInstance *pInst, LPD3DXCONSTANTTABLE pConstantTable, byte* pShader, int nSize);
  bool mfUpdateSamplers();
  static void mfPostVertexFormat(SHWSInstance *pInst, CHWShader_D3D *pHWSH, bool bCol, byte bNormal, bool bTC0, bool bTC1[2], bool bPSize, bool bTangent[2], bool bBinorm[2], bool bHWSkin, bool bSH[2], bool bShapeDeform, bool bMorphTarget, bool bMorph);
  void mfUpdateFXVertexFormat(SHWSInstance *pInst, CShader *pSH);
  /*EHWSProfile mfGetCurrentProfile()
  {
    return m_pCurInst->m_eProfileType;
  }*/
  void ModifyLTMask(uint32& nMask);

public:
  virtual ~CHWShader_D3D();
  bool mfSetVS(int nFlags=0);
  bool mfSetPS(int nFlags=0);
#if defined (DIRECT3D10)
  bool mfSetGS(int nFlags=0);
#endif
  bool mfSet(int nFlags=0)
  {
    if (m_eSHClass == eHWSC_Vertex)
      return mfSetVS(nFlags);
    else
    if (m_eSHClass == eHWSC_Pixel)
      return mfSetPS(nFlags);
    else
#if defined (DIRECT3D10)
    if (GEOMETRYSHADER_SUPPORT && m_eSHClass == eHWSC_Geometry)
      return mfSetGS(nFlags);
#else
    if (m_eSHClass == eHWSC_Geometry)
      return mfSetVS(nFlags);
#endif
    return false;
  }
  VIRTUAL bool mfModifyFlags(CShader *pSH);
  VIRTUAL bool mfAddEmptyCombination(CShader *pSH, uint64 nRT, uint64 nGL, uint32 nLT);
  VIRTUAL bool mfStoreEmptyCombination(SEmptyCombination& Comb);
  VIRTUAL bool mfSetV(int nFlags=0){ return mfSet(nFlags); };
  VIRTUAL void mfReset(uint32 CRC32);
  VIRTUAL const char *mfGetEntryName() { return m_EntryFunc.c_str(); }
  VIRTUAL bool mfFlushCacheFile();
  VIRTUAL bool Export(SShaderSerializeContext& SC);
  virtual bool mfPrecache(SShaderCombination& cmb, bool bForce);

  // Vertex shader specific functions
  VIRTUAL EVertexFormat mfVertexFormat(bool &bUseTangents, bool &bUseLM, bool &bUseHWSkin, bool& bUseSH);
  static EVertexFormat  mfVertexFormat(SHWSInstance *pInst, CHWShader_D3D *pSH, LPD3DXBUFFER pBuffer, DynArray<SCGBind>& InstBindVars);
  VIRTUAL uint32 mfGetPreprocessFlags(SShaderTechnique *pTech);

  VIRTUAL const char * mfGetActivatedCombinations(bool bForLevel);
  static  const char * mfGetSharedActivatedCombinations(bool bForLevel);

  static void mfSetLightParams(int nPass);
  static void mfSetGlobalParams();
  static void mfSetCameraParams();
  static void mfSetPF();
  static void mfSetCM();
  static bool mfAddGlobalParameter(SCGParam& Param, EHWShaderClass eSH, bool bSG, bool bCam);

  static void ShutDown();

  // Import/Export
  bool ExportSamplers(SCHWShader& SHW, SShaderSerializeContext& SC);
  bool ExportParams(SCHWShader& SHW, SShaderSerializeContext& SC);

  DEFINE_ALIGNED_DATA_STATIC(Vec4, m_CurPSParams[], 16);
  DEFINE_ALIGNED_DATA_STATIC(Vec4, m_CurVSParams[], 16);
#if !defined (XENON)
 #if defined (DIRECT3D9) || defined(OPENGL)
  DEFINE_ALIGNED_DATA_STATIC(Vec4, m_CurPSParamsI[], 16);
  DEFINE_ALIGNED_DATA_STATIC(Vec4, m_CurVSParamsI[], 16);
 #elif defined (DIRECT3D10)
  static ID3D11Buffer **m_pCB[eHWSC_Max][CB_MAX];
  static ID3D11Buffer *m_pCurReqCB[eHWSC_Max][CB_MAX];
  static void *m_pCurDevCB[eHWSC_Max][CB_MAX];
  static Vec4 *m_pDataCB[eHWSC_Max][CB_MAX];
  static int m_nCurMaxVecs[eHWSC_Max][CB_MAX];
  static int m_nMax_PF_Vecs[eHWSC_Max];
  static int m_nMax_SG_Vecs[eHWSC_Max];
  static ID3D11Buffer *m_pLightCB[eHWSC_Max];

  static CHWShader_D3D::SHWSInstance *m_pCurInstVS;
  static CHWShader_D3D::SHWSInstance *m_pCurInstPS;
  static CHWShader_D3D::SHWSInstance *m_pCurInstGS;
 #endif



#endif

  static int m_PSParamsToCommit[];
  static int m_NumPSParamsToCommit;
  static int m_VSParamsToCommit[];
  static int m_NumVSParamsToCommit;

	static bool ms_bInitShaders;

  static int m_nResetDeviceFrame;
  static int m_nInstFrame;

  static int m_nDevicePSDataSize;
  static int m_nDeviceVSDataSize;

  static std::vector<SCGParam> m_CM_Params[eHWSC_Max]; // Per-frame parameters
  static std::vector<SCGParam> m_PF_Params[eHWSC_Max]; // Per-frame parameters
  static std::vector<SCGParam> m_SG_Params[eHWSC_Max]; // Shadow-gen parameters

  friend struct SShaderTechniqueStat;
};

#if defined(DIRECT3D10)
  bool PatchDXBCShaderCode(LPD3D10BLOB& pShader, CHWShader_D3D *pSh);
#endif

  struct SShaderTechniqueStat
  {
    SShaderTechnique *pTech;
    CShader *pShader;
    CHWShader_D3D *pVS;
    CHWShader_D3D *pPS;
    CHWShader_D3D::SHWSInstance *pVSInst;
    CHWShader_D3D::SHWSInstance *pPSInst;
  };

  extern DynArray<SShaderTechniqueStat> g_SelectedTechs;

#endif  // __D3DHWSHADER_H__
