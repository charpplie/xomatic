#ifndef _DynamicVB_H_
#define _DynamicVB_H_

template <class VertexType> class DynamicVB 
{
public:
  D3DVertexBuffer *m_pVB;

private :
    uint32 m_BytesCount;
    uint32 m_nBytesOffs;
    bool m_bLocked;
    bool m_bReadWrite;
    VertexType* m_pLockedData;

  public :

    uint32 GetBytesCount() const 
    { 
      return m_BytesCount; 
    }
    int GetBytesOffset() const 
    { 
      return m_nBytesOffs; 
    }
    void Reset()
    {
      m_nBytesOffs = m_BytesCount;
    }

    DynamicVB(D3DDevice *pD3D, const DWORD& theFVF, const unsigned int& theVertsCount, bool bReadWrite)
    {
      m_pVB = 0;

      m_bLocked = false;
      m_pLockedData = NULL;
      m_nBytesOffs = 0;
      m_bReadWrite = bReadWrite;

      m_BytesCount = theVertsCount * sizeof(VertexType);
    
#if defined (DIRECT3D9) || defined(OPENGL)
      HRESULT hr = gRenDev->m_pRT->RC_CreateVertexBuffer(m_BytesCount, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, theFVF, D3DPOOL_DEFAULT, (void **)&m_pVB, NULL);
      //HRESULT hr = pD3D->CreateVertexBuffer(m_BytesCount, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, theFVF, D3DPOOL_DEFAULT, &m_pVB, NULL);
#elif defined (DIRECT3D10)
      D3D11_BUFFER_DESC BufDesc;
      ZeroStruct(BufDesc);
      BufDesc.ByteWidth = m_BytesCount;
      BufDesc.Usage = D3D11_USAGE_DYNAMIC;
      BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      BufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      if (bReadWrite)
      {
        BufDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
        BufDesc.Usage = D3D11_USAGE_STAGING;
#if !defined(PS3)
        BufDesc.BindFlags = 0;
#endif
      }
      BufDesc.MiscFlags = 0;
      HRESULT hr = pD3D->CreateBuffer(&BufDesc, NULL, &m_pVB);
#endif
      assert((hr == D3D_OK) && (m_pVB));
    }

    VertexType* Lock(const unsigned int& theLockBytesCount, uint32 &nOffs, bool bWrite=true)
    {
      if (theLockBytesCount > m_BytesCount)
      {
        assert(false);
        return NULL;
      }

      if (m_bLocked)
        return m_pLockedData;

      HRESULT hr = S_OK;







      if (m_pVB)
      {
#if defined (DIRECT3D9) || defined(OPENGL)
        if (theLockBytesCount+m_nBytesOffs > m_BytesCount)
        {
          gcpRendD3D->m_RP.m_VertexStreams[3].pStream = NULL;
#if !defined(XENON) && !defined(PS3)
          hr = m_pVB->Lock(0, theLockBytesCount, (void **) &m_pLockedData, D3DLOCK_DISCARD);


#endif
          nOffs = 0;
          m_nBytesOffs = theLockBytesCount;
        }
        else
        {
#if !defined(XENON) && !defined(PS3)
          hr = m_pVB->Lock(m_nBytesOffs, theLockBytesCount, (void **) &m_pLockedData, D3DLOCK_NOOVERWRITE);




#endif
          nOffs = m_nBytesOffs;
          m_nBytesOffs += theLockBytesCount;
        }
#elif defined (DIRECT3D10)
        if (theLockBytesCount+m_nBytesOffs > m_BytesCount)
        {
					STALL_PROFILER("set vertex_buffer")
          D3D11_MAP Map = D3D11_MAP_WRITE_DISCARD;
          if (m_bReadWrite)
            Map = bWrite ? D3D11_MAP_WRITE : D3D11_MAP_READ;
					D3D11_MAPPED_SUBRESOURCE mappedResource;
          hr = gcpRendD3D->m_pd3dDeviceContext->Map(m_pVB, 0, Map, 0, &mappedResource);
					m_pLockedData = (VertexType*)mappedResource.pData;
          nOffs = 0;
          m_nBytesOffs = theLockBytesCount;
        }
        else
        {
          byte *pD;
					STALL_PROFILER("set vertex_buffer")
          D3D11_MAP Map = D3D11_MAP_WRITE_NO_OVERWRITE;
          if (m_bReadWrite)
            Map = bWrite ? D3D11_MAP_WRITE : D3D11_MAP_READ;
					D3D11_MAPPED_SUBRESOURCE mappedResource;
					hr = gcpRendD3D->m_pd3dDeviceContext->Map(m_pVB, 0, Map, 0, &mappedResource);
					pD = (byte*)mappedResource.pData;
          m_pLockedData = (VertexType *)(pD + m_nBytesOffs);
          nOffs = m_nBytesOffs;
          m_nBytesOffs += theLockBytesCount;
        }
#endif
        assert(m_pLockedData != NULL);
        m_bLocked = true;
      }

      return m_pLockedData;
    }

    void Unlock()
    {
      if ((m_bLocked) && (m_pVB))
      {
#if defined (DIRECT3D9) || defined(OPENGL)
        HRESULT hr = m_pVB->Unlock();        
#elif defined (DIRECT3D10)
        HRESULT hr = S_OK;
				gcpRendD3D->GetDeviceContext()->Unmap(m_pVB, 0);
#endif
        assert(hr == D3D_OK);
        m_bLocked = false;
      }
    }
    int GetSize()
    {
      return _VertBufferSize(m_pVB);
    }

    HRESULT Bind(uint32 StreamNumber, int nBytesOffset, int Stride, uint32 nFreq=1)
    {
      HRESULT h = gcpRendD3D->FX_SetVStream(StreamNumber, m_pVB, nBytesOffset, Stride, nFreq);
      return h;
    }
    ~DynamicVB()
    {
      Unlock();







      SAFE_RELEASE(m_pVB);
    }
  
};

#endif  _DynamicVB_H_
