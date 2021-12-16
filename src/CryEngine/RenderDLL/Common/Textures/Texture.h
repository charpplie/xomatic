/*=============================================================================
  TexMan.h : Common texture manager declarations.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
											* Created by Khonich Andrey
- 19:8:2008   12:14 : * Refactored by Anton Kaplanyan

=============================================================================*/


      
#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "../ResFile.h"
#include "PowerOf2BlockPacker.h"
#include "STLPoolAllocator.h"

namespace CImageExtensionHelper
{
	struct DDS_PIXELFORMAT;
};

class CTexture;
struct IFlashPlayer;
struct IVideoPlayer;

#define TEXPOOL_LOADSIZE 6*1024*1024

#define MAX_MIP_LEVELS (100)






// Custom Textures IDs
enum 
{
  TO_RT_2D = 1,

  TO_FROMLIGHT,
  TO_FOG,
  TO_FROMOBJ,

  TO_SHADOWID0,
  TO_SHADOWID1,
  TO_SHADOWID2,
  TO_SHADOWID3,
  TO_SHADOWID4,
  TO_SHADOWID5,
  TO_SHADOWID6,
  TO_SHADOWID7,

  TO_FROMRE0,
  TO_FROMRE1,

  TO_FROMRE0_FROM_CONTAINER,
  TO_FROMRE1_FROM_CONTAINER,

  TO_SCREENMAP,
  TO_SCREENSHADOWMAP,
  TO_SCATTER_LAYER,
  TO_TERRAIN_LM,
  TO_RT_CM,
  TO_CLOUDS_LM,
  TO_BACKBUFFERMAP,
  TO_PREVBACKBUFFERMAP,
  TO_MIPCOLORS_DIFFUSE,
  TO_MIPCOLORS_BUMP,
  TO_LIGHTINFO,

  TO_FROMSF0,
  TO_FROMSF1,

  TO_SCREEN_AO_TARGET,
  TO_DOWNSCALED_ZTARGET_FOR_AO,

	TO_SCREEN_SSGI_TARGET,

  TO_WATEROCEANMAP,
  TO_WATERVOLUMEMAP,

  TO_WATERPUDDLESMAP,

	TO_VOLOBJ_DENSITY,
	TO_VOLOBJ_SHADOW,

	TO_COLORCHART,

	TO_ZTARGET_MS,

	TO_SCENE_NORMALMAP,

	TO_IRRADVOLUME_R,
	TO_IRRADVOLUME_G,
	TO_IRRADVOLUME_B,
	TO_IRRADVOLUME_NORMAL,
	TO_IRRADVOLUME_COLOR,
	TO_IRRADVOLUME_DEPTH,

	TO_SCENE_DIFFUSE_ACC,
	TO_SCENE_SPECULAR_ACC,
	TO_SCENE_TEXTURES,
  TO_SCENE_TARGET,

  TO_BACKBUFFERSCALED_D2,
  TO_BACKBUFFERSCALED_D4,
  TO_BACKBUFFERSCALED_D8,

  TO_HDRTARGET_ENCODED,

	TO_SKYDOME_MIE,
	TO_SKYDOME_RAYLEIGH,
	TO_SKYDOME_MOON
};


#define NUM_HDR_TONEMAP_TEXTURES 4
#define NUM_HDR_BLOOM_TEXTURES   3
#define NUM_SCALEFORM_TEXTURES   2

#define DYNTEXTURE_TEXCACHE_LIMIT 32

inline int LogBaseTwo(int iNum)
{
  int i, n;
  for(i = iNum-1, n = 0; i > 0; i >>= 1, n++ );
  return n;
}

enum EShadowBuffers_Pool
{
  SBP_DF16,
  SBP_DF24,
  SBP_D16,
  SBP_D24S8,
  SBP_G16R16,
  SBP_D32F,
  SBP_MAX,
  SBP_UNKNOWN
};

//dx9 usages
enum ETexture_Usage
{
  eTXUSAGE_AUTOGENMIPMAP,
  eTXUSAGE_DEPTHSTENCIL,
  eTXUSAGE_RENDERTARGET
};



#define TEX_POOL_BLOCKLOGSIZE 5  // 32
#define TEX_POOL_BLOCKSIZE   (1 << TEX_POOL_BLOCKLOGSIZE)

struct SDynTexture_Shadow;

//======================================================================
// Dynamic textures
struct SDynTexture : public IDynTexture
{
  static int              s_nMemoryOccupied;
  static SDynTexture      s_Root;

  SDynTexture *           m_Next;           //!<
  SDynTexture *           m_Prev;           //!<
  char                    m_sSource[128];   //!< pointer to the given name in the constructor call

  CTexture *              m_pTexture;
  ETEX_Format             m_eTF;
  ETEX_Type               m_eTT;
  uint32                  m_nWidth;
  uint32                  m_nHeight;
  uint32                  m_nReqWidth;
  uint32                  m_nReqHeight;
  int                     m_nTexFlags;
	uint32                  m_nFrameReset;


  bool                    m_bLocked;
  byte                    m_nUpdateMask;
  byte                    m_nPriority;

	//////////////////////////////////////////////////////////////////////////
	// Shadow specific vars.
	//////////////////////////////////////////////////////////////////////////
	int m_nUniqueID;

	SDynTexture_Shadow *m_NextShadow;           //!<
	SDynTexture_Shadow *m_PrevShadow;           //!<

  ShadowMapFrustum* m_pFrustumOwner;
	IRenderNode * pLightOwner;
	int nObjectsRenderedCount;
	//////////////////////////////////////////////////////////////////////////

  SDynTexture(const char *szSource);
  SDynTexture(int nWidth, int nHeight, ETEX_Format eTF, ETEX_Type eTT, int nTexFlags, const char *szSource, int8 nPriority=-1);
  ~SDynTexture();

	//////////////////////////////////////////////////////////////////////////
	// Custom new/delete for pool allocator.
	//////////////////////////////////////////////////////////////////////////
	ILINE void* operator new( size_t nSize );
	ILINE void operator delete( void *ptr );
	//////////////////////////////////////////////////////////////////////////

	virtual int GetTextureID();
  virtual bool Update(int nNewWidth, int nNewHeight);
  bool RT_Update(int nNewWidth, int nNewHeight);
  virtual void Apply(int nTUnit, int nTS=-1);
  virtual bool SetRT(int nRT, bool bPush, SD3DSurface *pDepthSurf, bool bScreenVP=false);
  virtual bool RT_SetRT(int nRT, int nWidth, int nHeight, bool bPush, bool bScreenVP=false);
  virtual bool SetRectStates() { return true; }
  virtual bool RestoreRT(int nRT, bool bPop);
	virtual ITexture *GetTexture() { return (ITexture *)m_pTexture; }
	virtual void Release() { delete this; }
  virtual void SetUpdateMask();
  virtual void ResetUpdateMask();
  virtual bool IsSecondFrame() { return m_nUpdateMask == 3; }
	virtual byte GetFlags() const { return 0; }
	virtual void GetSubImageRect(uint32 & nX, uint32 & nY, uint32 & nWidth, uint32 & nHeight)
	{
		nX = 0;
		nY = 0;
		nWidth = m_nWidth;
		nHeight = m_nHeight;
	}
  virtual void GetImageRect(uint32 & nX, uint32 & nY, uint32 & nWidth, uint32 & nHeight);
	virtual int GetWidth() { return m_nWidth; }
	virtual int GetHeight() { return m_nHeight; }

  void Lock() { m_bLocked = true; }
  void UnLock() { m_bLocked = false; }

  virtual void AdjustRealSize();
  virtual bool IsValid();

  _inline void UnlinkGlobal()
  {
    if (!m_Next || !m_Prev)
      return;
    m_Next->m_Prev = m_Prev;
    m_Prev->m_Next = m_Next;
    m_Next = m_Prev = NULL;
  }
  _inline void LinkGlobal(SDynTexture* Before)
  {
    if (m_Next || m_Prev)
      return;
    m_Next = Before->m_Next;
    Before->m_Next->m_Prev = this;
    Before->m_Next = this;
    m_Prev = Before;
  }
  virtual void Unlink()
  {
    UnlinkGlobal();
  }
  virtual void Link()
  {
    LinkGlobal(&s_Root);
  }
  ETEX_Format GetFormat() { return m_eTF; }
  bool FreeTextures(bool bOldOnly, int nNeedSpace);

	typedef std::multimap<unsigned int, CTexture*, std::less<unsigned int>, stl::STLPoolAllocator<std::pair<unsigned int, CTexture*>, stl::PoolAllocatorSynchronizationSinglethreaded> > TextureSubset;
  typedef TextureSubset::iterator TextureSubsetItor;
  typedef std::multimap<unsigned int, TextureSubset*, std::less<unsigned int>, stl::STLPoolAllocator<std::pair<unsigned int, TextureSubset*>, stl::PoolAllocatorSynchronizationSinglethreaded> >  TextureSet;
  typedef TextureSet::iterator TextureSetItor;
  static TextureSet      s_availableTexturePool2D_DXT1;
  static TextureSubset   s_checkedOutTexturePool2D_DXT1;
  static TextureSet      s_availableTexturePool2D_A8R8G8B8;
  static TextureSubset   s_checkedOutTexturePool2D_A8R8G8B8;
	static TextureSet      s_availableTexturePool2D_R5G6B5;
	static TextureSubset   s_checkedOutTexturePool2D_R5G6B5;
  static TextureSet      s_availableTexturePool2D_R32F;
  static TextureSubset   s_checkedOutTexturePool2D_R32F;
	static TextureSet      s_availableTexturePool2D_R16F;
	static TextureSubset   s_checkedOutTexturePool2D_R16F;
  static TextureSet      s_availableTexturePool2D_G16R16F;
  static TextureSubset   s_checkedOutTexturePool2D_G16R16F;
  static TextureSet      s_availableTexturePool2D_A16B16G16R16F;
  static TextureSubset   s_checkedOutTexturePool2D_A16B16G16R16F;
  static TextureSet      s_availableTexturePool2D_A2B10G10R10;
  static TextureSubset   s_checkedOutTexturePool2D_A2B10G10R10;

  static TextureSet    s_availableTexturePool2D_Shadows[SBP_MAX];
  static TextureSubset s_checkedOutTexturePool2D_Shadows[SBP_MAX];
  static TextureSet    s_availableTexturePoolCube_Shadows[SBP_MAX];
  static TextureSubset s_checkedOutTexturePoolCube_Shadows[SBP_MAX];

  static TextureSet      s_availableTexturePool2DCustom_G16R16F;
  static TextureSubset   s_checkedOutTexturePool2DCustom_G16R16F;


  static TextureSet      s_availableTexturePoolCube_DXT1;
  static TextureSubset   s_checkedOutTexturePoolCube_DXT1;
  static TextureSet      s_availableTexturePoolCube_A8R8G8B8;
  static TextureSubset   s_checkedOutTexturePoolCube_A8R8G8B8;
  static TextureSet      s_availableTexturePoolCube_R32F;
  static TextureSubset   s_checkedOutTexturePoolCube_R32F;
	static TextureSet      s_availableTexturePoolCube_R16F;
	static TextureSubset   s_checkedOutTexturePoolCube_R16F;
  static TextureSet      s_availableTexturePoolCube_G16R16F;
  static TextureSubset   s_checkedOutTexturePoolCube_G16R16F;
  static TextureSet      s_availableTexturePoolCube_A16B16G16R16F;
  static TextureSubset   s_checkedOutTexturePoolCube_A16B16G16R16F;

  static TextureSet      s_availableTexturePoolCube_A2B10G10R10;
  static TextureSubset   s_checkedOutTexturePoolCube_A2B10G10R10;

  static TextureSet      s_availableTexturePoolCubeCustom_G16R16F;
  static TextureSubset   s_checkedOutTexturePoolCubeCustom_G16R16F;

  static uint32    s_iNumTextureBytesCheckedOut;
  static uint32    s_iNumTextureBytesCheckedIn;

  static uint32    s_SuggestedDynTexAtlasCloudsMaxsize;
  static uint32    s_SuggestedDynTexAtlasSpritesMaxsize;
  static uint32    s_SuggestedTexAtlasSize;
  static uint32    s_SuggestedDynTexMaxSize;

  static uint32    s_CurDynTexAtlasCloudsMaxsize;
  static uint32    s_CurDynTexAtlasSpritesMaxsize;
  static uint32    s_CurTexAtlasSize;
  static uint32    s_CurDynTexMaxSize;

  CTexture *CreateDynamicRT();
  CTexture *GetDynamicRT();
  void ReleaseDynamicRT(bool bForce);

  EShadowBuffers_Pool ConvertTexFormatToShadowsPool( ETEX_Format e );

  static bool FreeAvailableDynamicRT(int nNeedSpace, TextureSet *pSet, bool bOldOnly);

  static void Init();
  static void Tick();

	void GetMemoryUsage( ICrySizer *pSizer ) const 
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};


typedef stl::PoolAllocatorNoMT<sizeof(SDynTexture)> SDynTexture_PoolAlloc;
extern SDynTexture_PoolAlloc* g_pSDynTexture_PoolAlloc;

//////////////////////////////////////////////////////////////////////////
// Custom new/delete for pool allocator.
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
inline void* SDynTexture::operator new( size_t nSize )
{
	void *ptr = g_pSDynTexture_PoolAlloc->Allocate();
	if (ptr)
		memset( ptr,0,nSize ); // Clear objects memory.
	return ptr;
}
//////////////////////////////////////////////////////////////////////////
inline void SDynTexture::operator delete( void *ptr )
{
	if (ptr)
		g_pSDynTexture_PoolAlloc->Deallocate(ptr);
}


//==============================================================================

struct STextureSetFormat
{
  struct SDynTexture2    *m_pRoot;
  ETEX_Format             m_eTF;
  ETEX_Type               m_eTT;
  int                     m_nTexFlags;
  std::vector<CPowerOf2BlockPacker *> m_TexPools;

  STextureSetFormat(ETEX_Format eTF, uint32 nTexFlags)
  {
    m_eTF = eTF;
    m_eTT = eTT_2D;
    m_pRoot = NULL;
    m_nTexFlags = nTexFlags;
  }
};

enum ETexPool
{
  eTP_Clouds,
  eTP_Sprites,
  eTP_VoxTerrain,
  eTP_DynTexSources,

  eTP_Max
};

struct SDynTexture2 : public IDynTexture
{
#ifndef _DEBUG
  char                   *m_sSource;        //!< pointer to the given name in the constructor call
#else
  char                   m_sSource[128];        //!< pointer to the given name in the constructor call
#endif
  STextureSetFormat      *m_pOwner;
  CPowerOf2BlockPacker   *m_pAllocator;
  CTexture               *m_pTexture;
  uint32                  m_nBlockID;
  ETexPool               m_eTexPool;

  SDynTexture2           *m_Next;           //!<
  SDynTexture2          **m_PrevLink;           //!<

  _inline void UnlinkGlobal()
  {
    if (m_Next)
      m_Next->m_PrevLink = m_PrevLink;
    if (m_PrevLink)
      *m_PrevLink = m_Next;
  }
  _inline void LinkGlobal(SDynTexture2*& Before)
  {
    if (Before)
      Before->m_PrevLink = &m_Next;
    m_Next = Before;
    m_PrevLink = &Before;
    Before = this;
  }
  void Link()
  {
    LinkGlobal(m_pOwner->m_pRoot);
  }
  void Unlink()
  {
    UnlinkGlobal();
    m_Next = NULL;
    m_PrevLink = NULL;
  }
  bool Remove();

  uint32                  m_nX;
  uint32                  m_nY;
  uint32                  m_nWidth;
  uint32                  m_nHeight;

  bool                    m_bLocked;
	byte                    m_nFlags;
	byte                    m_nUpdateMask;  // Crossfire odd/even frames
	uint32                    m_nFrameReset;
  uint32                    m_nAccessFrame;
  virtual bool IsValid();
  _inline bool _IsValid()
  {





    return IsValid();

  }

  SDynTexture2(int nWidth, int nHeight, int nTexFlags, const char *szSource, ETexPool eTexPool);
  SDynTexture2(const char *szSource, ETexPool eTexPool);
  ~SDynTexture2();

  bool UpdateAtlasSize(int nNewWidth, int nNewHeight);

  virtual bool Update(int nNewWidth, int nNewHeight);
  virtual void Apply(int nTUnit, int nTS=-1);
  virtual bool SetRT(int nRT, bool bPush, SD3DSurface *pDepthSurf, bool bScreenVP=false);
  virtual bool SetRectStates();
  virtual bool RestoreRT(int nRT, bool bPop);
	virtual ITexture *GetTexture() { return (ITexture *)m_pTexture; }
  ETEX_Format GetFormat() { return m_pOwner->m_eTF; }
	virtual void SetUpdateMask();
  virtual void ResetUpdateMask();
  virtual bool IsSecondFrame() { return m_nUpdateMask == 3; }
	virtual byte GetFlags() const { return m_nFlags; }
	virtual void SetFlags(byte flags) { m_nFlags = flags; }

	// IDynTexture implementation
	virtual void Release() { delete this; }
	virtual void GetSubImageRect(uint32 & nX, uint32 & nY, uint32 & nWidth, uint32 & nHeight)
	{
		nX = m_nX;
		nY = m_nY;
		nWidth = m_nWidth;
		nHeight = m_nHeight;
	}
  virtual void GetImageRect(uint32 & nX, uint32 & nY, uint32 & nWidth, uint32 & nHeight);
	virtual int GetTextureID();
  virtual void Lock() { m_bLocked = true; }
  virtual void UnLock() { m_bLocked = false; }
	virtual int GetWidth() { return m_nWidth; }
	virtual int GetHeight() { return m_nHeight; }
  virtual bool GetImageData32(byte * pData, int nDataSize);

  typedef std::map<uint32, STextureSetFormat *>  TextureSet2;
  typedef TextureSet2::iterator TextureSet2Itor;
  static TextureSet2      s_TexturePool[eTP_Max];
  static int              s_nMemoryOccupied[eTP_Max];

  static void Init(ETexPool eTexPool);
  static int GetPoolMaxSize(ETexPool eTexPool);
  static const char * GetPoolName(ETexPool eTexPool);
  static ETEX_Format GetPoolTexFormat(ETexPool eTexPool);
  static int GetPoolTexNum(ETexPool eTexPool)
  {
    int nT = 0;
    for (TextureSet2Itor it=s_TexturePool[eTexPool].begin(); it!=s_TexturePool[eTexPool].end(); it++)
    {
      STextureSetFormat *pF = it->second;
      nT += pF->m_TexPools.size();
    }
    return nT;
  }
};

//////////////////////////////////////////////////////////////////////////
// Dynamic texture for the shadow.
// This class must not contain any non static member variables, 
//  because SDynTexture allocated used constant size pool.
//////////////////////////////////////////////////////////////////////////
struct SDynTexture_Shadow : public SDynTexture
{
	static SDynTexture_Shadow  s_RootShadow;

	//////////////////////////////////////////////////////////////////////////
  SDynTexture_Shadow(int nWidth, int nHeight, ETEX_Format eTF, ETEX_Type eTT, int nTexFlags, const char *szSource);
  SDynTexture_Shadow(const char *szSource);
  ~SDynTexture_Shadow();

  _inline void UnlinkShadow()
  {
    //assert(gRenDev->m_pRT->IsRenderThread());
    if (!m_NextShadow || !m_PrevShadow)
      return;
    m_NextShadow->m_PrevShadow = m_PrevShadow;
    m_PrevShadow->m_NextShadow = m_NextShadow;
    m_NextShadow = m_PrevShadow = NULL;
  }
  _inline void LinkShadow(SDynTexture_Shadow* Before)
  {
    assert(gRenDev->m_pRT->IsRenderThread());
    if (m_NextShadow || m_PrevShadow)
      return;
    m_NextShadow = Before->m_NextShadow;
    Before->m_NextShadow->m_PrevShadow = this;
    Before->m_NextShadow = this;
    m_PrevShadow = Before;
  }

  SDynTexture_Shadow* GetByID(int nID)
  {
    assert(gRenDev->m_pRT->IsRenderThread());
    SDynTexture_Shadow *pTX = SDynTexture_Shadow::s_RootShadow.m_NextShadow;
    for (pTX=SDynTexture_Shadow::s_RootShadow.m_NextShadow; pTX != &SDynTexture_Shadow::s_RootShadow; pTX=pTX->m_NextShadow)
    {
      if (pTX->m_nUniqueID == nID)
        return pTX;
    }
    return NULL;
  }
  static void RT_EntityDelete(IRenderNode *pRenderNode);

  virtual void Unlink()
  {
    assert(gRenDev->m_pRT->IsRenderThread());
    UnlinkGlobal();
    UnlinkShadow();
  }
  virtual void Link()
  {
    assert(gRenDev->m_pRT->IsRenderThread());
    LinkGlobal(&s_Root);
    LinkShadow(&s_RootShadow);
  }
  virtual void AdjustRealSize();

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		SDynTexture::GetMemoryUsage(pSizer);
	}
};

struct SDynTextureArray/* : public SDynTexture*/
{

  //SDynTexture members

  //////////////////////////////////////////////////////////////////////////
  char                    m_sSource[128];   //!< pointer to the given name in the constructor call

  CTexture *              m_pTexture;
  ETEX_Format             m_eTF;
  ETEX_Type               m_eTT;
  uint32                  m_nWidth;
  uint32                  m_nHeight;
  uint32                  m_nReqWidth;
  uint32                  m_nReqHeight;
  int                     m_nTexFlags;
  int                     m_nPool;
  uint32                    m_nFrameReset;


  bool                    m_bLocked;
  byte                    m_nUpdateMask;
  byte                    m_nPriority;

  //////////////////////////////////////////////////////////////////////////
  // Shadow specific vars.
  //////////////////////////////////////////////////////////////////////////
  IRenderNode * pLightOwner;
  int nObjectsRenderedCount;

  int m_nUniqueID;
  //////////////////////////////////////////////////////////////////////////

  uint32 m_nArraySize;

  SDynTextureArray(int nWidth, int nHeight, int nArraySize, ETEX_Format eTF, int nTexFlags, const char *szSource);
  ~SDynTextureArray();

  bool Update(int nNewWidth, int nNewHeight);

  /*_inline void UnlinkShadow()
  {
    if (!m_NextShadow || !m_PrevShadow)
      return;
    m_NextShadow->m_PrevShadow = m_PrevShadow;
    m_PrevShadow->m_NextShadow = m_NextShadow;
    m_NextShadow = m_PrevShadow = NULL;
  }
  _inline void LinkShadow(SDynTexture_Shadow* Before)
  {
    if (m_NextShadow || m_PrevShadow)
      return;
    m_NextShadow = Before->m_NextShadow;
    Before->m_NextShadow->m_PrevShadow = this;
    Before->m_NextShadow = this;
    m_PrevShadow = Before;
  }

  SDynTexture_Shadow* GetByID(int nID)
  {
    SDynTexture_Shadow *pTX = SDynTexture_Shadow::s_RootShadow.m_NextShadow;
    for (pTX=SDynTexture_Shadow::s_RootShadow.m_NextShadow; pTX != &SDynTexture_Shadow::s_RootShadow; pTX=pTX->m_NextShadow)
    {
      if (pTX->m_nUniqueID == nID)
        return pTX;
    }
    return NULL;
  }

  virtual void Unlink()
  {
    UnlinkGlobal();
    UnlinkShadow();
  }
  virtual void Link()
  {
    LinkGlobal(&s_Root);
    LinkShadow(&s_RootShadow);
  }
  virtual void AdjustRealSize();*/

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};


//=======================================================================

// round up sigh
#define BILERP_PROTECTION	2
// attenuation texture function width ranges from 0-1 with linear interpolation between sample
#define ATTENUATION_WIDTH			512
// space for 512 different attuentation functions
#define NUM_ATTENUATION_FUNCTIONS	8

/* Some attenuation shapes
*/
enum ATTENUATION_FUNCTION
{
  AF_LINEAR,
  AF_SQUARED,
  AF_SHAPE1,
  AF_SHAPE2,
};

////////////////////////////////////////////////////////////////////////////////////
// CBaseMap: Base class so that different implementations can be stored together //
////////////////////////////////////////////////////////////////////////////////////
class CTexFunc
{
protected:
  uint32 m_dwWidth, m_dwHeight;
  int m_nLevels;

public:
  CTexture *m_pTex;
  CTexFunc()
  {
    m_dwWidth = m_dwHeight = 1;
    m_nLevels = -1;
    m_pTex = NULL;
  }
  ~CTexFunc()
  {
    m_pTex = NULL;
  }

  virtual int Initialize() = 0;
};


//==========================================================================
// Texture

struct STexPool;






struct STexPoolItem

{
	CTexture *m_pTex;
	STexPool *m_pOwner;
	STexPoolItem *m_Next;
	STexPoolItem *m_Prev;
	STexPoolItem *m_NextFree;
	STexPoolItem *m_PrevFree;
	CDeviceTexture *m_pDevTexture;
	float m_fLastTimeUsed;

	STexPoolItem ()
	{
		m_pTex = NULL;
		m_pDevTexture = NULL;
		m_pOwner = NULL;
		m_Next = NULL;
		m_Prev = NULL;
		m_NextFree = NULL;
		m_PrevFree = NULL;
		m_fLastTimeUsed = 0.f;
	}
	~STexPoolItem();

	_inline void Unlink();
	_inline void Link( STexPoolItem* Before );
	_inline void UnlinkFree();
	_inline void LinkFree( STexPoolItem* Before );
	int GetSize()
	{
		int nSize = sizeof(*this);
		return nSize;
	}
	void GetMemoryUsage(ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_pDevTexture);
	}
	bool IsStillUsedByGPU();
};

struct STexPool
{
  int m_Width;
  int m_Height;
  D3DFormat m_eFormat;
  ETEX_Type m_eTT;
  int m_nMips;
  int m_Size;
#ifndef XENON
  CDeviceTexture *m_pSysTexture;           // System texture for updating
#endif
  STexPoolItem m_ItemsList;
	int GetSize()
	{
		int nSize = sizeof(*this);
		STexPoolItem *pIT = m_ItemsList.m_Next;
		while (pIT != &m_ItemsList)
		{
			nSize += pIT->GetSize();
			pIT = pIT->m_Next;
		}

		return nSize;
	}

	void GetMemoryUsage(ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this,sizeof(*this));
		STexPoolItem *pIT = m_ItemsList.m_Next;
		while (pIT != &m_ItemsList)
		{
			pSizer->AddObject(pIT);	
			pIT = pIT->m_Next;
		}
	}
	
};

_inline void STexPoolItem::Unlink()
{
	if (!m_Next || !m_Prev)
		return;
	m_Next->m_Prev = m_Prev;
	m_Prev->m_Next = m_Next;
	m_Next = m_Prev = NULL;
}

_inline void STexPoolItem::Link( STexPoolItem* Before )
{
	if (m_Next || m_Prev)
		return;
	m_Next = Before->m_Next;
	Before->m_Next->m_Prev = this;
	Before->m_Next = this;
	m_Prev = Before;
}

_inline void STexPoolItem::UnlinkFree()
{
	if (!m_NextFree || !m_PrevFree)
		return;
	m_NextFree->m_PrevFree = m_PrevFree;
	m_PrevFree->m_NextFree = m_NextFree;
	m_NextFree = m_PrevFree = NULL;
}
_inline void STexPoolItem::LinkFree( STexPoolItem* Before )
{
	if (m_NextFree || m_PrevFree)
		return;
	m_NextFree = Before->m_NextFree;
	Before->m_NextFree->m_PrevFree = this;
	Before->m_NextFree = this;
	m_PrevFree = Before;
}

struct STexUploadInfo
{
  int nStartMip;
  int nEndMip;
  int m_TexId;
  CTexture *m_pTex;
  int m_LoadedSize;
};

struct STexCacheFileHeader
{
  int m_SizeOf;
  byte m_nSides;
  byte m_nMipsPersistent;
  ETEX_Format m_DstFormat;
  STexCacheFileHeader()
  {
    memset(this, 0, sizeof(STexCacheFileHeader));
  }
};

struct SMipData
{
public:
  TArray<byte> DataArray; // Data.
  bool m_bNative;
	int m_Seek;
	SMipData() : m_bNative(false), m_Seek(0) {}

  void Free()
  {
    DataArray.Free();



  }
	void Init( int InSize, int nWidth, int nHeight )
	{
		DataArray.Reserve(InSize);





		m_bNative = true;

	}
};


struct STexCacheMipHeader
{
  int m_SizeOf;
  short m_USize;
  short m_VSize;
  int m_SideSize;
  int m_SideSizeWithMips;

  SMipData m_Mips[6];

	void GetMemoryUsage( ICrySizer *pSizer ) 
	{		
		for (int i=0; i<6; i++)
		{
			pSizer->AddObject(  m_Mips[i].DataArray );
		}
	}

  int GetSize()
  {
    int nSize = sizeof(*this);
    for (int i=0; i<6; i++)
      nSize += m_Mips[i].DataArray.GetMemoryUsage();
    return nSize;
  }

};

class CTextureStreamCallback : public IStreamCallback
{
	virtual void StreamOnComplete (IReadStream* pStream, unsigned nError);
	virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
};
class CTextureStreamLodCallback : public IStreamCallback
{
	virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
	virtual void StreamOnComplete (IReadStream* pStream, unsigned nError);
};

// shared streaming data 
struct STextureStreamingSharedInfo
{
  CTexture *              m_pTexture;
  CTimeValue							m_fStartTime;
	int											m_nHigherUploadedMip;
	int											m_nLowerUploadedMip;
	volatile int						m_nAsyncRefCount;
	int											m_nSyncRefCount;
  STextureStreamingSharedInfo()
  {
    memset(&m_pTexture, 0, sizeof(*this));
  }
};

// streaming data  - is attached to the asynchronous read request to pass data to the StreamOnComplete() callback
struct STextureStreamingInfo
{
	CTextureStreamCallback				m_Callback;
	int														m_nRelativeMipLevel;		// mip of new texture
	STextureStreamingSharedInfo*	m_pSharedInfo;
	STextureStreamingInfo()
	{
		m_nRelativeMipLevel = MAX_MIP_LEVELS;
		m_pSharedInfo = NULL;
	}
};

struct STexStreamLodInfo
{
  CTextureStreamLodCallback m_Callback;
  CTexture *								m_pTexture;
  int m_nStartLoadMip;
  int m_nEndLoadMip;
  float m_fStartTime;
  STexStreamLodInfo()
  {
    memset(&m_pTexture, 0, sizeof(*this)-sizeof(CTextureStreamLodCallback));
  }
  ~STexStreamLodInfo()
  {
  }
};

//===================================================================

struct SEnvTexture
{
  bool m_bInprogress;
  bool m_bReady;
  bool m_bWater;
  bool m_bReflected;
  bool m_bReserved;
  int m_MaskReady;
  int m_Id;
  int m_TexSize;
  // Result Cube-Map or 2D RT texture
  SDynTexture *m_pTex;
  float m_TimeLastUpdated;
  Vec3 m_CamPos;
  Vec3 m_ObjPos;
  Ang3 m_Angle;
  int m_nFrameReset;
  Matrix44 m_Matrix;
  //void *m_pQuery[6];
  //void *m_pRenderTargets[6];

  // Cube maps average colors (used for RT radiosity)
  int m_nFrameCreated[6];
  ColorF m_EnvColors[6];

  SEnvTexture()
  {
    m_bInprogress = false;
    m_bReady = false;
    m_bWater = false;
    m_bReflected = false;
    m_bReserved = false;
    m_MaskReady = 0;
    m_Id = 0;
    m_pTex = NULL;
    m_nFrameReset = -1;
		m_CamPos.Set(0.0f,0.0f,0.0f);
		m_ObjPos.Set(0.0f,0.0f,0.0f);
		m_Angle.Set(0.0f,0.0f,0.0f);

    for (int i=0; i<6; i++)
    {
      //m_pQuery[i] = NULL;
      m_nFrameCreated[i] = -1;
      //m_pRenderTargets[i] = NULL;
    }
  }

  void Release();
  void ReleaseDeviceObjects();
	void RT_SetMatrix();
};


//===============================================================================
// custom X360 formats 


































#ifndef XENON
#define D3DFMT_3DC ((D3DFormat)(MAKEFOURCC('A', 'T', 'I', '2')))





#endif

//ATI's native depth buffers
#define D3DFMT_DF16 ((D3DFormat)(MAKEFOURCC('D','F','1','6')))
#define D3DFMT_DF24 ((D3DFormat)(MAKEFOURCC('D','F','2','4'))) //TODO: support for NVIDIA: D3DFMT_D24S8
//NVIDIA's NULL format
#define D3DFMT_NULL ((D3DFormat)(MAKEFOURCC('N','U','L','L')))

struct SPixFormat
{
  // Pixel format info.
  D3DFormat       DeviceFormat; // Pixel format from Direct3D/OpenGL.
  const char*     Desc;     // Stat: Human readable name for stats.
  int             BitsPerPixel; // Total bits per pixel.
  bool            bCanAutoGenMips;
	bool						bCanReadSRGB;
  int             MaxWidth;
  int             MaxHeight;
  SPixFormat     *Next;

  SPixFormat()
  {
    Init();
  }
  void Init()
  {
    BitsPerPixel = 0;
    DeviceFormat = (D3DFormat)0;
    Desc = NULL;
    Next = NULL;
    bCanAutoGenMips = false;
		bCanReadSRGB = false;
  }
  bool IsValid()
  {
    if (BitsPerPixel)
      return true;
    return false;
  }
  bool CheckSupport(int Format, const char *szDescr, ETexture_Usage eTxUsage = eTXUSAGE_AUTOGENMIPMAP);
};

struct STexStageInfo
{
  int m_nCurState;
  STexState  m_State;

  // Per-stage color operations
	float			 m_fMipBias;
  CTexture   *m_Texture;
#if defined(DIRECT3D10)
  int32      m_nCurResView;
#endif
  STexStageInfo()
  {
    Flush();
  }
  void Flush()
  {
    m_nCurState = -1;
    m_State.m_nMipFilter = -1;
    m_State.m_nMinFilter = -1;
    m_State.m_nMagFilter = -1;
    m_State.m_nAddressU = -1;
    m_State.m_nAddressV = -1;
    m_State.m_nAddressW = -1;
    m_State.m_nAnisotropy = 0;
#if defined(DIRECT3D10)
    m_nCurResView = -1;
#endif
    m_Texture = NULL;
		m_fMipBias = 0.f;
  }
};


struct STexData
{
  byte *m_pData;
  int m_nWidth;
  int m_nHeight;
  int m_nDepth;
  ETEX_Format m_eTF;
  int m_nMips;
  int m_nSize;
  int m_nFlags;
  bool m_bReallocated;
  float m_fAmount;
  ColorF m_AvgColor;
  string m_FileName;

  STexData()
  {
    m_pData = NULL;
    m_eTF = eTF_Unknown;
    m_nFlags = 0;
    m_bReallocated = false;
    m_fAmount = 100.0f;
    m_nWidth = 0;
    m_nHeight = 0;
    m_nDepth = 1;
    m_nSize = 0;
	m_AvgColor = ColorF(0.5f,0.5f,0.5f,1.0f);
  }
  void AssignData(byte *pNewData)
  {
    if (m_bReallocated)
      delete [] m_pData;
    m_pData = pNewData;
    m_bReallocated = true;
  }
};

struct STexGrid
{
  int m_Width;
  int m_Height;
  CTexture *m_TP;
};

struct SD3DSurface
{
  int nWidth;
  int nHeight;
  bool bBusy;
  int nFrameAccess;








  void *pSurf;
  void *pTex;
  SD3DSurface()
  {
    pSurf = NULL;
    pTex = NULL;
    bBusy = false;
    nFrameAccess = -1;






  }
  ~SD3DSurface();
};

class CTexture : public ITexture, public CBaseResource
{
  friend class CFnMap;
  friend class CFnMap3D;
  friend struct SDynTexture;
  friend class CD3D9Renderer;

private:
  static CCryName s_sClassName;

  static int			s_nMaxTextureSize;

  static CCryName GenName(const char *name, uint32 nFlags, float fAmount1=-1, float fAmount2=-1);
  static CTexture *NewTexture(const char *name, uint32 nFlags, ETEX_Format eTFDst, bool& bFound, float fAmount1=-1, float fAmount2=-1, int8 nPriority=-1);

  void *m_pDeviceRTV;
#if defined (DIRECT3D10)
  void *m_pDeviceShaderResource;
  void *m_pDeviceShaderResourceSRGB;
  void *m_pShaderResourceViews[4]; //fix replace static array by malloc
  CDeviceTexture *m_pDeviceTextureMSAA;
  void *m_pDeviceDepthStencilSurf;
  void *m_pDeviceDepthStencilCMSurfs[6];
  void *m_pDeviceShaderResourceMS;
#endif




  short m_nWidth;
  short m_nHeight;
  short m_nDepth;
  uint32 m_nArraySize;
  ETEX_Format m_eTFDst;
  uint32 m_nFlags;									// e.g. FT_USAGE_DYNAMIC
  uint32 m_nSize;









  CDeviceTexture *m_pDevTexture;

	uint32 m_bAsyncDevTexCreation : 1;
  uint32 m_bIsLocked : 1;
  uint32 m_bNeedRestoring : 1;
  uint32 m_bNoTexture : 1;
  uint32 m_bResolved : 1;
	uint32 m_bHighQualityRT : 1;
	uint32 m_bCustomFormat : 1;	// Allow custom texture formats - for faster texture fetches

  uint32 m_bWasUnloaded : 1;
  uint32 m_bStreamed : 1;
  uint32 m_bStreamPrepared : 1;
  uint32 m_bStreamFromDDS : 1;
  uint32 m_bStreamingInProgress : 1;
  uint32 m_bStreamRequested : 1;

  uint32 m_bVertexTexture : 1;
	uint32 m_bUseDecalBorderCol : 1;
	uint32 m_bIsSRGB : 1;
	uint32 m_bFP10 : 1;
	uint32 m_bNoDevTexture : 1;

  byte m_nMips;
  int8 m_nPriority;

	// streaming mip fading
	float		m_fCurrentMipBias;

  ETEX_Type m_eTT;

  short m_nSrcWidth;
  short m_nSrcHeight;
  short m_nSrcDepth;
  ETEX_Format m_eTFSrc;
  ColorF m_AvgColor;
  string m_SrcName;

  int m_nCustomID;
  CTexFunc *m_pFunc;
  SPixFormat *m_pPixelFormat;

  int m_nFSAASamples;
  int m_nFSAAQuality;

	CryMT::queue<IReadStreamPtr>	m_TextureStreamTasks;

  // system copy of texture (used for decals baked into terrain texture and objects integration)
#ifdef TEXTURE_GET_SYSTEM_COPY_SUPPORT
  static const int m_nImageSystemCopyMipsNum = 16;
  byte ** m_pImageSystemCopy;
  CryCriticalSection m_sGetSystemCopyLock;
#endif

  static void Resample(byte *pDst, int nNewWidth, int nNewHeight, const byte *pSrc, int nSrcWidth, int nSrcHeight);
  static void Resample8(byte *pDst, int nNewWidth, int nNewHeight, const byte *pSrc, int nSrcWidth, int nSrcHeight);
	static void Resample4Bit(byte *pDst, int nNewWidth, int nNewHeight, const byte *pSrc, int nSrcWidth, int nSrcHeight);

  int GenerateMipsForNormalmap(TArray<Vec4>* Normals, int nWidth, int nHeight);
  void ConvertFloatNormalMap_To_ARGB8(TArray<Vec4>* Normals, int nWidth, int nHeight, int nMips, byte *pDst);
  void MergeNormalMaps(STexData Data[]);
  void GenerateNormalMap(STexData *pData);
  void GenerateNormalMaps(STexData Data[6]);
  void ImagePreprocessing(STexData Data[6]);

	static void OutputDebugInfo();

public:
  CTexture()
  {
    m_nFlags = 0;
    m_eTFDst = eTF_Unknown;
    m_eTFSrc = eTF_Unknown;
    m_pFunc = NULL;
    m_nMips = 1;
    m_nWidth = 0;
    m_nHeight = 0;
    m_eTT = eTT_2D;
    m_nSrcDepth = 1;
    m_nDepth = 1;
    m_nArraySize = 1;
    m_nDeviceSize = 0;
    m_nSize = 0;
    m_AvgColor = Col_White;
    m_nPriority = -1;

    m_nFSAASamples = 0;
    m_nFSAAQuality = 0;

    m_nUpdateFrameID = -1;
    m_nAccessFrameID = -1;
    m_nRTSetFrameID = -1;
		m_nStreamingSwitchFrameID = -1;
    m_nCustomID = -1;
    m_pPixelFormat = NULL;
    m_pDevTexture = NULL;
    m_pDeviceRTV = NULL;
#if defined (DIRECT3D10)
    m_pDeviceShaderResource = NULL;
    m_pDeviceShaderResourceSRGB = NULL;
    m_pDeviceTextureMSAA = NULL;
    for( int i = 0; i < 4; ++i ) m_pShaderResourceViews[i] = NULL;
    m_pDeviceDepthStencilSurf = NULL;
    for( int i = 0; i < 6; ++i ) m_pDeviceDepthStencilCMSurfs[i] = NULL;
    m_pDeviceShaderResourceMS = NULL;
#endif




    m_NextTxt = NULL;









    m_bAsyncDevTexCreation = false;

    m_bIsLocked = false;
    m_bNeedRestoring = false;
    m_bNoTexture = false;
    m_bResolved = true;
    m_bHighQualityRT = false;
		m_bCustomFormat = false;

    m_bWasUnloaded = false;
    m_bStreamFromDDS = false;
    m_bStreamingInProgress = false;
    m_bStreamed = false;
    m_bStreamPrepared = false;
    m_bStreamRequested = false;
    m_bVertexTexture = false;
		m_bUseDecalBorderCol = false;
		m_bIsSRGB = false;
		m_bFP10 = false;
		m_bNoDevTexture = false;

    m_nAsyncStartMip = -1;
    m_nMipFactorUpdateId = -1;
    m_Next = NULL;
    m_Prev = NULL;
		m_fMinMipFactor = 0;
		m_fLastMipFactor = 0;
    m_nMinMipCur = MAX_MIP_LEVELS;
    m_nMinMipVidUploaded = MAX_MIP_LEVELS;

		m_fCurrentMipBias = 0.f;

    m_pPoolItem = NULL;
		m_pNewPoolItem = NULL;
    m_pFileTexMips = NULL;
    
#ifdef TEXTURE_GET_SYSTEM_COPY_SUPPORT
    m_pImageSystemCopy = NULL;
#endif
  }
  virtual ~CTexture();

  int m_nDefState;
  CTexture *m_NextTxt;
  float m_fAnimSpeed;
  int m_nRTSetFrameID;					// last read access, compare with GetFrameID(false)
	int m_nUpdateFrameID;					// last write access, compare with GetFrameID(false)

  std::vector<RECT> m_DirtyRects;

  // ITexture interface
  virtual int AddRef() { return CBaseResource::AddRef(); }
  virtual int Release()
  {
    if (!(m_nFlags & FT_DONT_RELEASE) || IsLocked())
    {
      if (m_bStreamingInProgress)
      {
        if (GetRefCounter() > 1)
          return CBaseResource::Release();
        AbortStreamingTasks(this);
      }

      return CBaseResource::Release();
    }

    return -1;
  }
  virtual int ReleaseForce()
  {
    m_nFlags &= ~FT_DONT_RELEASE;
    int nRef = 0;
    while (true)
    {
      nRef = Release();
      if (nRef <= 0)
        break;
    }
    return nRef;
  }
  VIRTUAL const char *GetName() const { return GetSourceName(); }
  VIRTUAL const int GetWidth() const { return m_nWidth; }
	VIRTUAL const int GetHeight() const { return m_nHeight; }
	VIRTUAL const int GetDepth() const { return m_nDepth; }
  VIRTUAL const int GetSourceWidth() const { return m_nSrcWidth; }
  VIRTUAL const int GetSourceHeight() const { return m_nSrcHeight; }
  VIRTUAL const int GetTextureID() const { return GetID(); }
  VIRTUAL const uint32 GetFlags() const { return m_nFlags; }
	ILINE const int GetNumMipsNonVirtual() const { return m_nMips; }
	VIRTUAL const int GetNumMips() const { return m_nMips; }
  VIRTUAL const ETEX_Type GetTextureType() const { return m_eTT; }
  VIRTUAL void SetClamp(bool bEnable)
  {
    int nMode = bEnable ? TADDR_CLAMP : TADDR_WRAP;
    SetClampingMode(nMode, nMode, nMode);
  }
  VIRTUAL const bool IsTextureLoaded() const { return IsLoaded(); }
  VIRTUAL void PrecacheAsynchronously(float fMipFactor, int nFlags, int nUpdateId);
  VIRTUAL void Preload (int nFlags) {};
  VIRTUAL byte *GetData32(int nSide=0, int nLevel=0, byte * pDst=NULL, ETEX_Format eDstFormat = eTF_A8R8G8B8);
  VIRTUAL const int GetDeviceDataSize() const
  {
    int nSize = m_nDeviceSize;
    if (m_bStreamPrepared && m_pNewPoolItem)
      nSize += m_pNewPoolItem->m_pOwner->m_Size;
    return nSize;
  }
  VIRTUAL const int GetDataSize() const { return m_nSize; }
  VIRTUAL byte *LockData(int& nPitch, int nSide=0, int nLevel=0);
  VIRTUAL void UnlockData(int nSide=0, int nLevel=0);
  VIRTUAL bool SaveTGA(const char *szName, bool bMips=false);
  VIRTUAL bool SaveJPG(const char *szName, bool bMips=false);
  virtual bool SaveDDS(const char *szName, bool bMips=true);
  VIRTUAL bool SetFilter(int nFilter) { return SetFilterMode(nFilter); }
  virtual bool Fill(const ColorF& color);

//Diesel cut -> dooble
  //VIRTUAL const ColorF& GetAvgColor() const { return m_AvgColor; }
//Diesel new
VIRTUAL const ColorF& GetAvgColor() const { return m_AvgColor; }


	virtual void GetMemoryUsage( ICrySizer *pSizer ) const;
	VIRTUAL const char* GetFormatName() const;
	VIRTUAL const char* GetTypeName() const;

	VIRTUAL const bool IsShared() const
	{
		return CBaseResource::GetRefCounter() > 1;
	}

  VIRTUAL const ETEX_Format GetTextureDstFormat() const { return m_eTFDst; }
  VIRTUAL const ETEX_Format GetTextureSrcFormat() const { return m_eTFSrc; }

	VIRTUAL const bool IsParticularMipStreamed(float fMipFactor) const;

  // Internal functions
  const ETEX_Format GetDstFormat() const { return m_eTFDst; }
  const ETEX_Format GetSrcFormat() const { return m_eTFSrc; }
  const ETEX_Type GetTexType() const { return m_eTT; }

  void RT_ReleaseDevice();

  static _inline bool IsTextureExist(const CTexture *pTex) { return pTex && pTex->GetDevTexture(); } 

  const bool IsNoTexture() const { return m_bNoTexture; };
  void SetNeedRestoring() { m_bNeedRestoring = true; }
  void SetWasUnload(bool bSet) { m_bWasUnloaded = bSet; }
	const bool IsPartiallyLoaded() const { return m_nMinMipVidUploaded != 0; }
  const bool IsUnloaded(void) const { return m_bWasUnloaded; }
  void SetStreamingInProgress(bool bSet) { m_bStreamingInProgress = bSet; }
  void ResetNeedRestoring() { m_bNeedRestoring = false; }
  const bool IsNeedRestoring() const { return m_bNeedRestoring; }
	const int StreamGetLoadedMip() const { return m_nMinMipVidUploaded; }
  const bool IsResolved() const { return m_bResolved; }
  const bool IsVertexTexture() const { return m_bVertexTexture; }
  void SetVertexTexture( bool bEnable ) { m_bVertexTexture = bEnable; }  
  const bool IsDynamic() const { return ((m_nFlags & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET)) != 0); }
  const bool IsStreamFromDDS() const { return m_bStreamFromDDS; }
	bool IsStillUsedByGPU();
  void Lock() { m_bIsLocked = true; }
  void Unlock() { m_bIsLocked = false; }
  const bool IsLoaded() const { return ((m_nFlags & FT_FAILED) == 0); }
  const bool IsLocked() const { return m_bIsLocked; }
  ILINE const bool IsStreamed() const { return m_bStreamed; }
  VIRTUAL const bool IsStreamedVirtual() const { return IsStreamed(); }
	VIRTUAL const int GetAccessFrameId() const { return m_nAccessFrameID; }
  void SetResolved(bool bResolved) { m_bResolved = bResolved; }
  const int8 GetPriority() const { return m_nPriority; }
  const int GetCustomID() const { return m_nCustomID; }
  void SetCustomID(int nID) { m_nCustomID = nID; }
  void SetFlags(uint32 nFlags) { m_nFlags |= nFlags; }
  void ResetFlags(uint32 nFlags) { m_nFlags &= ~nFlags; }
  const uint32 _GetFlags() const { return m_nFlags; }
  int GetDefState() const { return m_nDefState; }
	const bool UseDecalBorderCol() const { return m_bUseDecalBorderCol; }
	const bool IsSRGB() const { return m_bIsSRGB; }
  void SRGBRead( bool bEnable = false ) { m_bIsSRGB = bEnable; }
	const bool IsCustomFormat() const { return m_bCustomFormat; }
	void SetCustomFormat() { m_bCustomFormat = true; }

	VIRTUAL const bool IsStreamable() const {
		return IsStreamed();
	}

	const bool IsStreamableNonVirtual() const {
		return !(m_nFlags & FT_DONT_STREAM) && !(m_eTT == eTT_3D || m_eTT == eTT_AutoCube);
	}

	// Sets render target destination edram tile. 
	//  -  0 = backbuffer, 1 = secondary, mostly free, 2 = zbuffer, can use when zbuffer not needed anymore
	void SetRenderTargetTile( uint8 nTile = 0);
	const uint8 GetRenderTargetTile() const;








  const bool IsHighQualityRT() const { return m_bHighQualityRT; }
  void SetHighQualityRT() { m_bHighQualityRT = true; }

	bool IsFP10Format() const { return m_bFP10; };
	void SetFP10Format() { m_bFP10 = true; };
 
  void *GetDeviceRT() const { return m_pDeviceRTV; }
  void SetDeviceRT(void *pDevRT) { m_pDeviceRTV = pDevRT; }
  CDeviceTexture *GetDevTexture() const { return m_pDevTexture; }
  void SetDevTexture(CDeviceTexture *pDeviceTex) { m_pDevTexture = pDeviceTex; }
  void AddDirtRect(RECT& rc);
  bool IsAsyncDevTexCreation() { return m_bAsyncDevTexCreation; }
#if defined (DIRECT3D10)
  void *GetDeviceResource() const { return m_pDeviceShaderResource; }
  CDeviceTexture *GetDevTextureMSAA() const { return m_pDeviceTextureMSAA; }
  void *GetDeviceResourceMS() const { return m_pDeviceShaderResourceMS; }
  void *GetDeviceResource(int nSlice) const { assert(nSlice >= 0 && nSlice < 4); return m_pShaderResourceViews[nSlice]; } 
  void *GetDeviceDepthStencilSurf() const { return m_pDeviceDepthStencilSurf; }
  void *GetDeviceDepthStencilCMSurf(int nCMSide) const { assert(nCMSide >= 0 && nCMSide < 6); return m_pDeviceDepthStencilCMSurfs[nCMSide]; }
#endif
#ifndef NULL_RENDERER
  D3DSurface *GetSurface(int nCMSide, int nLevel);
  D3DPOOL GetPool();
#endif
  const SPixFormat *GetPixelFormat() const { return m_pPixelFormat; }
  bool Invalidate(int nNewWidth, int nNewHeight, ETEX_Format eTF);
  const char *GetSourceName() const { return m_SrcName.c_str(); }
  const int GetSize(bool bIncludePool) const;
  void PostCreate();

  //===================================================================
  // Streaming support
  static CTexture s_Root;
  
  static int s_nTexSizeHistory;
  static int s_TexSizeHistory[8];
  static int s_nPhaseProcessingTextures;
  static int s_nProcessedTextureID1;
  static int s_nProcessedTextureID2;
	static TArray<CTexture *> s_StreamingRequested;
	static TArray<CTexture *> s_DistanceSortedTextures;
	static CryMT::queue<IReadStreamPtr> s_StreamingTextures;
	static int s_bPoolOverflow;

  CTexture *m_Next;           //!<
  CTexture *m_Prev;           //!<
  STexCacheFileHeader m_CacheFileHeader;
  STexCacheMipHeader *m_pFileTexMips;
  int m_nMipFactorUpdateId;
  int m_nAccessFrameID;					// last read access, compare with GetFrameID(false)
  int m_nStreamingSwitchFrameID;// last streaming access, compare with GetFrameID(false)
  float m_fMinMipFactor;
	float m_fLastMipFactor;
  volatile int m_nDeviceSize;
  int m_nAsyncStartMip;
  int m_nMinMipCur;
  int m_nMinMipVidUploaded;
	STexPoolItem *m_pPoolItem;
	STexPoolItem *m_pNewPoolItem;

  static void Precache();
  static void RT_Precache();

	static void StreamCheckTexLimits();
	static void StreamUnloadOldTextures(int nCurTexPoolSize);
	static void StreamValidateTexSize();
#ifndef NULL_RENDERER
	void StreamUploadMip(IReadStream *pStream, int nMip, int nBaseMipOffset);
	static void StreamCopyMipsFast(STexPoolItem* pSrcItem, int nMipSrc, STexPoolItem* pDestItem, int nMipDest, int nNumMips);	// GPU-assisted platform-dependend
#endif
  static STexPool *StreamCreatePool(int nWidth, int nHeight, int nMips, D3DFormat eTF, ETEX_Type eTT);
  static void AsyncRequestsProcessing();
  static void StreamUploadAsync(IReadStream *pStream);
  static void StreamSetLodAsync(IReadStream *pStream);
	static void FlushAllStreamingTasks(CTexture* pTex = NULL);
	static void AbortStreamingTasks(CTexture* pTex);
	static void StartStreaming(CTexture* pTex, const size_t nOffset, const size_t nSize, int nStartMip, int nEndMip);
	static void StreamAddTask(CTexture* pTex, const char* szFile, IStreamCallback* pCallback = NULL, StreamReadParams* pParams = NULL);
	static void StreamRemoveTask(CTexture* pTex, IReadStreamPtr pTask);
  static void StreamOnComplete (IReadStream* pStream);
  void StreamLoadSynchronously(int nStartMip, int nEndMip);
  void StreamRestore();
  void StreamCopyMips(int nStartMip, int nEndMip, bool bToDevice);
  void StreamReleaseMipsData(int nStartMip, int nEndMip);
  void StreamUpdateMip(float fMipFactor, int nUpdateId);
	VIRTUAL int StreamCalculateMips(float fMipFactor);
	VIRTUAL int GetStreamableMipNumber();
	VIRTUAL int GetStreamableMemoryUsage(int nStartMip);
  void StreamUploadMips(int nStartMip, int nEndMip);
  int StreamUnload(bool bDestroy);
  int StreamSetLod(int nToMip, bool bUnload);
  void StreamSetLodNonManagedAsync(int nStartMip);
  void StreamLoadFromCache(int nFlags, float fMipFactor, int nUpdateId);
  bool StreamPrepare(bool bReload, ETEX_Format eTFDst);
  void StreamRemoveFromPool();
  void StreamAssignPoolItem(STexPoolItem *pItem, int nMinMip);
  STexPoolItem *StreamGetPoolItem(int nStartMip, int nMips);

  _inline void Relink(CTexture* Before)
  {
    if (!IsStreamed())
      return;
    if (m_Next && m_Prev)
    {
      m_Next->m_Prev = m_Prev;
      m_Prev->m_Next = m_Next;
    }
    m_Next = Before->m_Next;
    Before->m_Next->m_Prev = this;
    Before->m_Next = this;
    m_Prev = Before;
		s_DistanceSortedTextures.SetUse(0);	// clear the array to hit that the list is touched
	}
  _inline void Unlink()
  {
    if (!m_Next || !m_Prev)
      return;
		s_DistanceSortedTextures.SetUse(0);	// clear the array to hit that the list is touched
		m_Next->m_Prev = m_Prev;
    m_Prev->m_Next = m_Next;
    m_Next = m_Prev = NULL;
  }
  _inline void Link(CTexture* Before)
  {
		if (!IsStreamed())
			return;
    if (m_Next || m_Prev)
      return;
    m_Next = Before->m_Next;
    Before->m_Next->m_Prev = this;
    Before->m_Next = this;
    m_Prev = Before;
		s_DistanceSortedTextures.SetUse(0);	// clear the array to hit that the list is touched
	}
  //=======================================================

  static void ApplyForID(int nID, int nTS=-1, int nSamplerSlot=-1)
  {
    CTexture *pTex = GetByID(nID);
    assert (pTex);
    if (pTex)
      pTex->Apply(-1, nTS, -1, nSamplerSlot);
  }
  static const CCryName& mfGetClassName();
  static CTexture *GetByID(int nID);
  static CTexture *GetByName(const char *szName);
  static CTexture *ForName(const char *name, uint32 nFlags, ETEX_Format eTFDst, float fAmount1=-1, float fAmount2=-1, int8 m_nPriority=-1);
  static CTexture *CreateTextureArray(const char *name, uint32 nWidth, uint32 nHeight, uint32 nArraySize, uint32 nFlags, ETEX_Format eTF, int nCustomID=-1, int8 nPriority=-1);
	static CTexture *CreateRenderTarget(const char *name, uint32 nWidth, uint32 nHeight, ETEX_Type eTT, uint32 nFlags, ETEX_Format eTF, int nCustomID=-1, int8 nPriority=-1, bool bHiQuality = false);
  static CTexture *CreateTextureObject(const char *name, uint32 nWidth, uint32 nHeight, int nDepth, ETEX_Type eTT, uint32 nFlags, ETEX_Format eTF, int nCustomID=-1, int8 nPriority=-1);
  static int  ComponentsForFormat(ETEX_Format eTF);
  static bool SetDefTexFilter(const char *szFilter);
  
  static void InitStreaming();
  static void Init();
  static void ShutDown();

  static void LoadDefaultTextures();
  static void GenerateFlareMap();
  static void GenerateNoiseVolumeMap();
  static void GenerateVectorNoiseVolMap();
  static CTexture *GenerateMipsColorMap(int nWidth, int nHeight);
  static void GenerateFuncTextures();

  static CTexture *MakeSpecularTexture(float fExp);
  static bool ReloadFile(const char *szFileName);
  static void ReloadTextures();
  static CTexture *Create2DTexture(const char *szName, int nWidth, int nHeight, int nMips, int nFlags, byte *pData, ETEX_Format eTFSrc, ETEX_Format eTFDst, int8 nPriority=-1, bool bAsyncDevTexCreation = false);
  static CTexture *Create3DTexture(const char *szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, byte *pData, ETEX_Format eTFSrc, ETEX_Format eTFDst, int8 nPriority=-1);
  static void Update();    

  // Loading/creating functions
  bool Load(ETEX_Format eTFDst, float fAmount1, float fAmount2);
  bool LoadFromImage(const char *name, const bool bReload, ETEX_Format eTFDst, float fAmount1=-1, float fAmount2=-1);
  bool Reload();
  bool ToggleStreaming(int bOn);
  bool CreateTexture(STexData Data[6]);

  // copy texture back into system memory (used for decals baked into terrain texture)
  byte ** GetSystemCopy();
  byte * GetSubImageData32(int nX, int nY, int nW, int nH, int & nOutTexDim);

  // API depended functions
  bool Resolve(int nTarget = 0, bool bUseViewportSize = false);
  bool CreateDeviceTexture(byte *pData[6]);
  bool RT_CreateDeviceTexture(byte *pData[6]);
  bool CreateRenderTarget(ETEX_Format eTF);
  void ReleaseDeviceTexture(bool bKeepLastMips);
  void Apply(int nTUnit=-1, int nTS=-1, int nTSlot=-1, int nSSlot=-1, int nResView=-1);
  void SetSamplerState(int nTUnit, int nTS, int nSSlot, int nOffSet = 0);
  ETEX_Format ClosestFormatSupported(ETEX_Format eTFDst);
  void SetTexStates();
	void UpdateTexStates();
  bool SetFilterMode(int nFilter);
  bool SetClampingMode(int nAddressU, int nAddressV, int nAddressW);
  void UpdateTextureRegion(byte *data, int X, int Y, int USize, int VSize, ETEX_Format eTFSrc);
  void RT_UpdateTextureRegion(byte *data, int X, int Y, int USize, int VSize, ETEX_Format eTFSrc);
  bool Create2DTexture(int nWidth, int nHeight, int nMips, int nFlags, byte *pData, ETEX_Format eTFSrc, ETEX_Format eTFDst);
	bool Create3DTexture(int nWidth, int nHeight, int nDepth, int nMips, int nFlags, byte *pData, ETEX_Format eTFSrc, ETEX_Format eTFDst);
  bool SetNoTexture();
  bool IsFSAAChanged();

  // Helper functions
  static bool SetProjector(int nUnit, int nState, int nSamplerSlot);
	static bool IsFormatSupported(ETEX_Format eTFDst);
  static void GenerateZMaps();
  static void DestroyZMaps();
  static void GenerateHDRMaps();
	// allocate or deallocate star maps
	static void DestroyHDRMaps();
  static void GenerateSceneMap(ETEX_Format eTF);
  static void DestroySceneMap();	

  static void DestroyLightInfo();
  static void GenerateLightInfo(ETEX_Format eTF, int nWidth, int nHeight);

  static int GetMaxTextureSize() { return s_nMaxTextureSize; }
	static bool IsFourBit(ETEX_Format eTF)
	{
		return (16 == BitsPerPixel(eTF));
	}
  static bool IsDXTCompressed(ETEX_Format eTF)
  {
    if (eTF == eTF_DXT1 || eTF == eTF_DXT3 || eTF == eTF_DXT5 || eTF == eTF_3DC || eTF == eTF_CTX1)
      return true;
    return false;
  }
  static void BindNULLFrom(int nForm = 0);
  static byte *Convert(byte *pSrc, int nWidth, int nHeight, int nMips, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nOutMips, int& nOutSize, bool bLinear);
  static int CalcNumMips(int nWidth, int nHeight);
	// upload mip data from file regarding to platform specifics
	static void ExpandMipFromFile(byte* dest, const int destSize, const byte* src, const int srcSize, const ETEX_Format fmt);
  static int TextureDataSize(int nWidth, int nHeight, int nDepth, int nMips, ETEX_Format eTF);
  static int BitsPerPixel(ETEX_Format eTF);
  static const char *NameForTextureFormat(ETEX_Format eTF);
  static const char *NameForTextureType(ETEX_Type eTT);
  static ETEX_Format TextureFormatForName(const char *str);
  static ETEX_Type TextureTypeForName(const char *str);
  static D3DFormat DeviceFormatFromTexFormat(ETEX_Format eTF);
  static ETEX_Format TexFormatFromDeviceFormat(D3DFormat nFormat);
  static D3DFormat GetD3DLinFormat(D3DFormat nFormat);
  static D3DFormat ConvertToDepthStencilFmt(D3DFormat nFormat);
  static D3DFormat ConvertToShaderResourceFmt(D3DFormat nFormat);

	static D3DFormat ConvertToSRGBFmt(D3DFormat fmt);
#if defined(DIRECT3D10)
  static D3DFormat ConvertToTypelessFmt(D3DFormat fmt);





#endif




  static SEnvTexture *FindSuitableEnvCMap(Vec3& Pos, bool bMustExist, int RendFlags, float fDistToCam);
  static SEnvTexture *FindSuitableEnvTex(Vec3& Pos, Ang3& Angs, bool bMustExist, int RendFlags, bool bUseExistingREs, CShader *pSH, SRenderShaderResources *pRes, CRenderObject *pObj, bool bReflect, CRendElementBase *pRE, bool *bMustUpdate);
	static bool RenderEnvironmentCMHDR(int size, Vec3& Pos, TArray<unsigned short>& vecData);
  static bool ScanEnvironmentCM(const char *name, int size, Vec3& Pos);
  static bool ScanEnvironmentCube(SEnvTexture *pEnv, uint32 nRendFlags, int Size, bool bLightCube);
  static void DrawCubeSide(Vec3& Pos, int tex_size, int side, int RendFlags, float fMaxDist);
  static void GetAverageColor(SEnvTexture *cm, int nSide);

  static TArray<STexGrid> m_TGrids;
  static void SetGridTexture(int nTUnit, CTexture *pThis, int nTSlot);

  static void GetBackBuffer(CTexture *pTex, int nRendFlags);

public:
  static SEnvTexture *s_pCurEnvTexture;

  static std::vector<STexState> s_TexStates;
  static int GetTexState(const STexState& TS)
  {
    uint32 i;


    int nTexStatesSize = s_TexStates.size();
    for (i=0; i<s_TexStates.size(); i++)
    {
      STexState *pTS = &s_TexStates[i];
      if (*pTS == TS)
        break;
    }

    if (i == s_TexStates.size())
    {
      s_TexStates.push_back(TS);
      s_TexStates[i].PostCreate();
    }

    return i;
  }

  static string s_GlobalTextureFilter;
  static int s_nGlobalDefState;
  static STexState s_sDefState;
  static STexState s_sGlobalDefState;
  static STexStageInfo s_TexStages[MAX_TMU];
  static int s_TexStateIDs[MAX_TMU];
  static int s_CurStage;
  static int s_nStreamingEnabled;
  static int s_nStreamingMode;
	static int s_nStreamingThroughput;	// in bytes
	static float s_nStreamingTotalTime;	// in secs
  static bool s_bStreamDontKeepSystem;
  static bool s_bPrecachePhase;
  static volatile bool s_bStreamingShow;

  static ETEX_Format s_eTFZ;
  static int s_nTexturesDataBytesLoaded;
  static volatile int s_nTexturesDataBytesUploaded;
  static float s_fAdaptiveStreamDistRatio;
  static volatile TIntAtomic s_nStatsCurManagedStreamedTexMem;
#ifndef PS3
  static volatile int s_nStatsCurManagedNonStreamedTexMem;
  static volatile int s_nStatsCurDynamicTexMem;
#endif
  static int s_nCurTexResolutionReduction;
  static int s_nCurTexBumpResolutionReduction;

  // ==============================================================================
  static CTexture* s_ptexNoTexture;
  static CTexture *s_ptexWhite;
  static CTexture *s_ptexGray;
  static CTexture *s_ptexBlack;
  static CTexture *s_ptexFlatBump;
	static CTexture *s_ptexMipMapDebug;
	static CTexture *s_ptexColorBlue;
	static CTexture *s_ptexColorCyan;
	static CTexture *s_ptexColorGreen;
	static CTexture *s_ptexColorPurple;
	static CTexture *s_ptexColorRed;
	static CTexture *s_ptexColorWhite;
	static CTexture *s_ptexColorYellow;
  static CTexture *s_ptexPaletteDebug;
	static CTexture *s_ptexPaletteTexelsPerMeter;
  static CTexture *s_ptexIconShaderCompiling;
  static CTexture *s_ptexIconStreaming;
  static CTexture *s_ptexIconStreamingTerrainTexture;
	static CTexture *s_ptexIconNullSoundSystem;
	static CTexture *s_ptexIconNullMusicSystem;
  static CTexture *s_ptexMipColors_Diffuse;
  static CTexture *s_ptexMipColors_Bump;
  static CTexture *s_ptexScreenNoiseMap;
  static CTexture *s_ptexNoiseVolumeMap;
  static CTexture *s_ptexVectorNoiseVolMap;
  static CTexture *s_ptexVignettingMap; // todo: create at runtime based on viggneting cvars
  static CTexture *s_ptexFromRE[8];
  static CTexture *s_ptexShadowID[8];
	static CTexture	*s_ptexFromRE_FromContainer[2];
  static CTexture *s_ptexFromObj;
  static CTexture *s_ptexFromLight;
  static CTexture *s_ptexRT_2D;
  static CTexture *s_ptexRT_CM;
  static CTexture *s_ptexFlare;  
	static CTexture *s_ptex16_PointsOnSphere;  
  static CTexture *s_ptexScreenMap;                 // screen buffer
  static CTexture *s_ptexScreenShadowMap[MAX_REND_LIGHT_GROUPS]; // screen render targets for shadow masks
  static CTexture *s_ptexCurrentScreenShadowMap[MAX_REND_LIGHT_GROUPS]; // screen render targets for shadow masks
  static CTexture *s_ptexAOTarget;
	static CTexture *s_ptexSSGITarget;
  
  static CTexture *s_ptexSceneNormalsMap;           // RT with normals for deferred shading

	// Irradiance volume stuffs
	static CTexture *s_ptexIrrVolumeRT[3];
	static CTexture *s_ptexIrrVolumeColorMap;
	static CTexture *s_ptexIrrVolumeNormalMap;
	static CTexture *s_ptexIrrVolumeDepthMap;
	static CTexture *s_ptexIrrVolumeRTDebug;


  static CTexture *s_ptexBackBuffer;                // back buffer copy  
  static CTexture *s_ptexPrevBackBuffer;                // previous frame back buffer copy  
	static CTexture *s_ptexPrevZTarget;                // previous frame ztarget copy
  static CTexture *s_ptexBackBufferTemp;            // back buffer copy  
  static CTexture *s_ptexBackBufferScaled[3];       // backbuffer low-resolution/blured version. 2x and 4x smaller than screen
  
  // depreceated - will be removed
  static CTexture *s_ptexEffectsAccum;              // shared effects accumulation texture  (rgb = unused atm, alpha = blood splats displacement)
  // depreceated - will be removed
  static CTexture *s_ptexGlow;  

  static CTexture *s_ptexCloak;                     // low-res with previous backbuffers accumulated 

  static CTexture *s_ptexWaterOcean;                // water ocean vertex texture
  static CTexture *s_ptexWaterVolumeDDN;               // water volume heightmap
  static CTexture *s_ptexWaterVolumeTemp;               // water volume heightmap
  static CTexture *s_ptexWaterPuddles[2];           // water wave propagation maps
  static CTexture *s_ptexWaterPuddlesDDN;           // final propagated waves normal map (and height in alpha)

  static CTexture *s_ptexScatterLayer;
  static CTexture *s_ptexTranslucenceLayer;

  static CTexture *s_ptexRT_ShadowPool;
  static CTexture *s_ptexRT_NULL;
  static CTexture *s_ptexCloudsLM;

  static CTexture *s_ptexLightInfo[4];
	static CTexture *s_ptexSceneTarget;  // Shared rt for generic usage (refraction/srgb/diffuse accumulation/hdr motionblur/etc)
  static CTexture *s_ptexCurrSceneTarget; // Pointer to current scene target, mostly for ps3 to read from destination rt
	
  static CTexture *s_ptexCurrentSceneDiffuseAccMap;
	static CTexture *s_ptexSceneDiffuseAccMap;
	static CTexture *s_ptexSceneSpecularAccMap;
	static CTexture *s_ptexSceneTexturesMap;

  static CTexture *s_ptexZTarget;
  static CTexture *s_ptexZOcclusion[2];
  static CTexture *s_ptexZTargetMS;
  static CTexture *s_ptexZTargetScaled;

  static CTexture *s_ptexHDRTarget;
  static CTexture *s_ptexHDRTargetEncoded;
  static CTexture *s_ptexHDRTargetScaled[3];
  static CTexture *s_ptexHDRBrightPass[2];
  static CTexture *s_ptexHDRAdaptedLuminanceCur[8];
  static int				s_nCurLumTextureIndex;
  static CTexture *s_ptexCurLumTexture;
	static CTexture *s_ptexCurLumTextureSys;
  static CTexture *s_ptexHDRToneMaps[NUM_HDR_TONEMAP_TEXTURES];
	static CTexture *s_ptexSkyDomeMie;
	static CTexture *s_ptexSkyDomeRayleigh;
	static CTexture *s_ptexSkyDomeMoon;
	static CTexture *s_ptexText_FromSF[NUM_SCALEFORM_TEXTURES];

	static CTexture *s_ptexVolObj_Density;
	static CTexture *s_ptexVolObj_Shadow;

	static CTexture *s_ptexColorChart;

  static CTexture *s_ptexStereoL;
  static CTexture *s_ptexStereoR;



  static SEnvTexture s_EnvCMaps[MAX_ENVCUBEMAPS];
  static SEnvTexture s_EnvTexts[MAX_ENVTEXTURES];  
  static TArray<SEnvTexture> s_CustomRT_CM;
  static TArray<SEnvTexture> s_CustomRT_2D;

  static TArray<STexPool *> s_TexturesPools;
  static STexPoolItem s_FreeTexPoolItems;

  static CTexture s_ShaderTemplates[EFTT_MAX];

  static CTexture *s_pBackBuffer;
  static CTexture s_FrontBufferTextures[2];

//virtual const ColorB& GetAvgColor() const; //Diesel new

//Diesel new
	virtual int StreamCalculateMips(float fMipFactor) const;
	virtual int GetStreamableMipNumber() const;
	virtual int GetStreamableMemoryUsage(int nStartMip) const;
//Diesel new

};

int sLimitSizeByScreenRes(uint32 size);

bool WriteTGA(byte*dat, int wdt, int hgt, const char*name, int src_bits_per_pixel, int dest_bits_per_pixel );
bool WriteJPG(byte*dat, int wdt, int hgt, const char*name, int bpp, int nQuality = 100);
byte * WriteDDS(byte*dat, int wdt, int hgt, int dpth, const char*name, ETEX_Format eTF, int nMips, ETEX_Type eTT, bool bToMemory=false, int* nSize=NULL);

//////////////////////////////////////////////////////////////////////////
class CDynTextureSource : public IDynTextureSource
{
public:
	CDynTextureSource();

	virtual void AddRef();
	virtual void Release();
	
	virtual bool Apply(int nTUnit, int nTS=-1);
	virtual void GetTexGenInfo(float& offsX, float& offsY, float& scaleX, float& scaleY) const;
	virtual byte GetFlags() const { return 0; }

protected:
	virtual ~CDynTextureSource();
	virtual void CalcSize(int& width, int& height, float distToCamera = -1) const;

	void InitDynTexture(ETexPool eTexPool);

protected:
	int m_refCount;
	int m_width;
	int m_height;
	float m_lastUpdateTime;
	int m_lastUpdateFrameID;
	SDynTexture2* m_pDynTexture;
};

//////////////////////////////////////////////////////////////////////////
class CFlashTextureSource : public CDynTextureSource
{
public:
	CFlashTextureSource(const char* pFlashFileName);

	virtual bool Update(float distToCamera);
	virtual bool Update(CTexture* pDstRT);
	virtual void GetDynTextureSource(void*& pIDynTextureSource, EDynTextureSource& dynTextureSource);

protected:
	virtual ~CFlashTextureSource();
	virtual void CalcSize(int& width, int& height, float distToCamera = -1) const;

private:
	bool m_highQualityRender;
	IFlashPlayer* m_pFlashPlayer;
#ifdef _DEBUG
	CCryName m_flashSrcFile;
#endif
};

//////////////////////////////////////////////////////////////////////////
class CVideoTextureSource : public CDynTextureSource
{
public:
	CVideoTextureSource(const char* pVideoFileName);

	virtual bool Update(float distToCamera);
	virtual bool Update(CTexture* pDstRT);
	virtual void GetDynTextureSource(void*& pIDynTextureSource, EDynTextureSource& dynTextureSource);

protected:
	virtual ~CVideoTextureSource();

private:
	IVideoPlayer* m_pVideoPlayer;
#ifdef _DEBUG
	CCryName m_videoSrcFile;
#endif
};

#endif
