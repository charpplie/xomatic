#ifndef _DeviceManager_H_
#define _DeviceManager_H_

#include "../GCMemoryManager.h"

class CDeviceTexture;
class CDeviceVertexBuffer;
class CDeviceIndexBuffer;

typedef CDeviceTexture* LPDEVICETEXTURE;

struct STextureInfoData
{
  const void *pSysMem;
  uint32 SysMemPitch;
  uint32 SysMemSlicePitch;
};

struct STextureInfo
{
  uint32 m_nArraySize;
  uint32 m_nMSAASamples;
  uint32 m_nMSAAQuality;
  STextureInfoData *m_pData;
  STextureInfo()
  {
    m_nArraySize = 1;
    m_nMSAASamples = 1;
    m_nMSAAQuality = 0;
    m_pData = NULL;
  }
};

//===============================================================================================================

class CDeviceManager
{
  friend class CD3DRenderer;
  friend class CDeviceTexture;

private:
#ifndef NULL_RENDERER
  D3DDevice *m_pDevice;
#endif

public:

#ifndef NULL_RENDERER
  CDeviceManager() { m_pDevice = NULL; }
  CDeviceManager(D3DDevice *pDevice);

  void AssignDevice(D3DDevice *pDevice) { m_pDevice = pDevice; }

  HRESULT Create2DTexture(uint32 nWidth, uint32 nHeight, uint32 nMips, uint32 nUsage, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI=NULL);
  HRESULT CreateCubeTexture(uint32 nSize, uint32 nMips, uint32 nUsage, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI=NULL);
  HRESULT CreateVolumeTexture(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nUsage, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI=NULL);

  HRESULT CreateVertexBuffer(uint32 nSize, uint32 nUsage, D3DPOOL Pool, D3DVertexBuffer **ppBuff);
  HRESULT CreateIndexBuffer(uint32 nSize, uint32 nUsage, D3DPOOL Pool, D3DIndexBuffer **ppBuff);
#endif
};

//====================================================================================================

#define USAGE_DEPTH_STENCIL 0x20000
#define USAGE_RENDER_TARGET 0x40000
#define USAGE_DYNAMIC       0x80000
#define USAGE_CUSTOM (USAGE_DEPTH_STENCIL | USAGE_RENDER_TARGET | USAGE_DYNAMIC)

struct STexLock
{
  uint8* pData;
  uint32 Pitch;
};
struct STexLockBox
{
  uint8* pData;
  uint32 RowPitch;
  uint32 SlicePitch;
};

// Texture lock flags
#define LF_READ    1
#define LF_WRITE   2
#define LF_DISCARD 4




class CDeviceTexture

{
  friend class CDeviceManager;

#ifndef NULL_RENDERER
  D3DBaseTexture *m_pD3DTexture;
	// for native hand-made textures
	void*						m_pBaseAddress;
	size_t					m_nAllocatedSize;
#endif

public:
#ifndef NULL_RENDERER
	D3DBaseTexture *GetBaseTexture() { return m_pD3DTexture; }
	D3DTexture *Get2DTexture() { return (D3DTexture *)m_pD3DTexture; }
  D3DCubeTexture *GetCubeTexture() { return (D3DCubeTexture *)m_pD3DTexture; }
  D3DVolumeTexture *GetVolumeTexture() { return (D3DVolumeTexture *)m_pD3DTexture; }

  CDeviceTexture () { m_pD3DTexture = NULL; m_pBaseAddress = NULL; m_nAllocatedSize = 0; }
  CDeviceTexture (D3DBaseTexture *pBaseTexture) { m_pD3DTexture = pBaseTexture; }
  ~CDeviceTexture ();

  HRESULT LockRect(uint32 nLevel, STexLock& Data, uint32 nFlags);
  HRESULT LockRect(int32 nCubeSide, uint32 nLevel, STexLock& Data, uint32 nFlags);
  HRESULT LockBox(uint32 nLevel, STexLockBox& Data, uint32 nFlags);
  HRESULT UnlockRect(uint32 nLevel);
  HRESULT UnlockRect(int32 nCubeSide, uint32 nLevel);
  HRESULT UnlockBox(uint32 nLevel);
#endif

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));



	}

  int32 Release();
};

class CDeviceVertexBuffer
{

};

class CDeviceIndexBuffer
{

};

#endif  _DeviceManager_H_
