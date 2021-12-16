/*=============================================================================
D3DOcclQuery.cpp : Occlusion queries unified interface implementation
Copyright (c) 2001-2005 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Tiago Sousa

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"

void COcclusionQuery::Create()
{
  Release();

  // Create visibility queries

	D3DQuery* pVizQuery = NULL;
#if defined (DIRECT3D9) || defined (OPENGL)
  HRESULT hr = gcpRendD3D->m_pd3dDevice->CreateQuery (D3DQUERYTYPE_OCCLUSION, &pVizQuery);
  assert(SUCCEEDED(hr));
#elif defined (DIRECT3D10) && !defined(PS3)
  D3D11_QUERY_DESC desc;
  desc.Query = D3D11_QUERY_OCCLUSION;
  desc.MiscFlags = 0;
  HRESULT hr(gcpRendD3D->m_pd3dDevice->CreateQuery(&desc, &pVizQuery));
  assert(SUCCEEDED(hr));
#endif

	m_nOcclusionID = (UINT_PTR) pVizQuery;
}

void COcclusionQuery::Release()
{
#if !defined(PS3)
	D3DQuery  *pVizQuery = (D3DQuery*)m_nOcclusionID;
	SAFE_RELEASE(pVizQuery);
#endif




	m_nOcclusionID = 0;

  m_nDrawFrame = 0;
  m_nCheckFrame = 0;
  m_nVisSamples = ~0;

}

void COcclusionQuery::BeginQuery()
{



  if( !m_nOcclusionID )
    return;

	D3DQuery* pVizQuery = (D3DQuery*)m_nOcclusionID;
# if defined (DIRECT3D9) || defined(OPENGL)
  pVizQuery->Issue( D3DISSUE_BEGIN );
# elif defined (DIRECT3D10)
	gcpRendD3D->m_pd3dDeviceContext->Begin(pVizQuery);
# endif


}

void COcclusionQuery::EndQuery()
{



	if( !m_nOcclusionID )

    return;

  CD3D9Renderer *rd = gcpRendD3D;
  m_nDrawFrame = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

#if !defined(PS3)
	D3DQuery*  pVizQuery = (D3DQuery*)m_nOcclusionID;
# if defined (DIRECT3D9) || defined(OPENGL)
	pVizQuery->Issue(D3DISSUE_END);
# elif defined (DIRECT3D10)
	rd->m_pd3dDeviceContext->End(pVizQuery);
# endif
#endif
}

bool COcclusionQuery::IsReady()
{
  CD3D9Renderer *rd = gcpRendD3D;
  int nFrame = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
  return (m_nCheckFrame == nFrame);
}

uint32 COcclusionQuery::GetVisibleSamples( bool bAsynchronous )
{



  if( !m_nOcclusionID )

    return ~0;

  CD3D9Renderer *rd = gcpRendD3D;
  int nFrame = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
  
  if (m_nCheckFrame == nFrame)
    return m_nVisSamples;

  int nVisSamples = ~0;
  
  if( !bAsynchronous )
  {
    PROFILE_FRAME(COcclusionQuery::GetVisibleSamples);

    HRESULT hRes = S_FALSE;







	D3DQuery  *pVizQuery = (D3DQuery*)m_nOcclusionID;
  #if defined (DIRECT3D9)
    while(hRes==S_FALSE)
      hRes = pVizQuery->GetData((void *) &nVisSamples, sizeof(DWORD),



      D3DGETDATA_FLUSH

      );
  #else
    while(hRes==S_FALSE)
			hRes = rd->m_pd3dDeviceContext->GetData(pVizQuery, (void *) &nVisSamples, sizeof(DWORD), 0);
  #endif

  if(hRes == S_OK)
  {
    m_nCheckFrame = nFrame;
    m_nVisSamples = nVisSamples;
  }



  }
  else
  {
    PROFILE_FRAME(COcclusionQuery::GetVisibleSamplesAsync);

    HRESULT hRes = S_OK;










		D3DQuery*  pVizQuery = (D3DQuery*)m_nOcclusionID;    
#if defined (DIRECT3D9)
    hRes = pVizQuery->GetData((void *) &nVisSamples, sizeof(DWORD), 0);    
  #elif defined (DIRECT3D10)
		hRes = rd->m_pd3dDeviceContext->GetData(pVizQuery, (void *) &nVisSamples, sizeof(DWORD), 0);
  #endif

  if (hRes == S_OK)
  {
    m_nCheckFrame = nFrame;
    m_nVisSamples = nVisSamples;
  } 


  }

  return m_nVisSamples;
}
