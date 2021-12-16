#ifndef _DynamicIB_H_
#define _DynamicIB_H_

template <class Type> class DynamicIB 
{
  private :

		D3DIndexBuffer*	m_pIB;
    uint32 m_Count;
    int m_nOffs;
    bool m_bLocked;
    bool m_bReadWrite;
    Type* m_pLockedData;

  public :

    uint32 GetCount() const 
    { 
      return m_Count; 
    }
    int GetOffset() const 
    { 
      return m_nOffs; 
    }
    void Reset()
    {
      m_nOffs = 0;
    }

    DynamicIB(D3DDevice *pD3D, const unsigned int& theElementsCount, bool bReadWrite)
    {
      m_pIB = 0;

      m_bLocked = false;
      m_bReadWrite = bReadWrite;
      m_pLockedData = NULL;
      m_nOffs = 0;

      m_Count = theElementsCount;
    
#if defined (DIRECT3D9) || defined(OPENGL)
      HRESULT hr = gRenDev->m_pRT->RC_CreateIndexBuffer(theElementsCount*sizeof(Type), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, sizeof(Type)==2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, D3DPOOL_DEFAULT, (void **)&m_pIB, NULL);
      //HRESULT hr = pD3D->CreateIndexBuffer(theElementsCount*sizeof(Type), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, sizeof(Type)==2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, D3DPOOL_DEFAULT, &m_pIB, NULL);
      assert((hr == D3D_OK) && (m_pIB));
#elif defined (DIRECT3D10)
      D3D11_BUFFER_DESC BufDesc;
      ZeroStruct(BufDesc);
      BufDesc.ByteWidth = theElementsCount*sizeof(Type);
      BufDesc.Usage = D3D11_USAGE_DYNAMIC;
      BufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
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
      HRESULT hr = pD3D->CreateBuffer(&BufDesc, NULL, &m_pIB);
#endif
    }

    Type* Lock(const uint32& theLockCount, uint32 &nOffs, bool bWrite=true)
    {
      if (theLockCount > m_Count)
      {
        assert(false);
        return NULL;
      }

      if (m_bLocked)
        return m_pLockedData;

      HRESULT hr = S_OK;







      if ( m_pIB )
      {
#if defined (DIRECT3D9) || defined(OPENGL)
        if (theLockCount+m_nOffs > m_Count)
        {
#if !defined(XENON) && !defined(PS3)
          hr = m_pIB->Lock(0, theLockCount*sizeof(Type), (void **) &m_pLockedData, D3DLOCK_DISCARD);


#endif
          nOffs = 0;
          m_nOffs = theLockCount;
        }
        else
        {
#if !defined(XENON) && !defined(PS3)
          hr = m_pIB->Lock(m_nOffs*sizeof(Type), theLockCount*sizeof(Type), (void **) &m_pLockedData, D3DLOCK_NOOVERWRITE);




#endif
          nOffs = m_nOffs;
          m_nOffs += theLockCount;
        }
#elif defined (DIRECT3D10)
        if (theLockCount+m_nOffs > m_Count)
        {
					STALL_PROFILER("set index_buffer")
          D3D11_MAP Map = D3D11_MAP_WRITE_DISCARD;
          if (m_bReadWrite)
            Map = bWrite ? D3D11_MAP_WRITE : D3D11_MAP_READ;
					D3D11_MAPPED_SUBRESOURCE mappedResource;
          hr = gcpRendD3D->m_pd3dDeviceContext->Map(m_pIB, 0, Map, 0, &mappedResource);
					m_pLockedData = (Type*)mappedResource.pData;
          nOffs = 0;
          m_nOffs = theLockCount;
        }
        else
        {
          byte *pD;
					STALL_PROFILER("set index_buffer")
          D3D11_MAP Map = D3D11_MAP_WRITE_NO_OVERWRITE;
          if (m_bReadWrite)
            Map = bWrite ? D3D11_MAP_WRITE : D3D11_MAP_READ;
					D3D11_MAPPED_SUBRESOURCE mappedResource;
					hr = gcpRendD3D->m_pd3dDeviceContext->Map(m_pIB, 0, Map, 0, &mappedResource);
					pD = (byte*)mappedResource.pData;
          m_pLockedData = (Type *)(pD + m_nOffs*sizeof(Type));
          nOffs = m_nOffs;
          m_nOffs += theLockCount;
        }
#endif
        assert(m_pLockedData != NULL);
        m_bLocked = true;
      }

      return m_pLockedData;
    }

    void Unlock()
    {
      if ((m_bLocked) && (m_pIB))
      {
        HRESULT hr = S_OK;
#if defined (DIRECT3D9) || defined(OPENGL)
        hr = m_pIB->Unlock();        
#elif defined (DIRECT3D10)
        gcpRendD3D->GetDeviceContext()->Unmap(m_pIB, 0);        
#endif
        assert(hr == D3D_OK);
        m_bLocked = false;
      }
    }
    int GetSize()
    {
      return _IndexBufferSize(m_pIB);
    }

    D3DIndexBuffer* GetInterface() { return m_pIB; }

    HRESULT Bind()
    {
      return gcpRendD3D->FX_SetIStream(m_pIB);
    }

    ~DynamicIB()
    {
      Unlock();







      SAFE_RELEASE(m_pIB);
    }
  
};

#endif  _DynamicIB_H_
