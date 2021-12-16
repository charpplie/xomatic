#include "StdAfx.h"
#include "CCryDXPSGIFactory.hpp"


CCryDXPSGIFactory::CCryDXPSGIFactory()
{
}

HRESULT CCryDXPSGIFactory::EnumAdapters( uint32 Adapter,IDXGIAdapter **ppAdapter)
{
	*ppAdapter	=	&m_Adapter;
	return (Adapter==0)-1;
}

HRESULT CCryDXPSGIFactory::MakeWindowAssociation(HWND WindowHandle,uint32 Flags)
{
	//no functionality needed
	return -1;
}

HRESULT CCryDXPSGIFactory::GetWindowAssociation( HWND *pWindowHandle)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

HRESULT CCryDXPSGIFactory::CreateSwapChain(ID3D11Device* pDevice,DXGI_SWAP_CHAIN_DESC *pDesc,IDXGISwapChain **ppSwapChain)
{
	*ppSwapChain	=	CRY_DXPS_CREATE(CCryDXPSSwapChain,((CCryDXPSRenderDevice*)pDevice,pDesc));
	return ppSwapChain?0:-1;	
}

HRESULT CCryDXPSGIFactory::CreateSoftwareAdapter( HMODULE Module,IDXGIAdapter **ppAdapter)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

void CCryDXPSGIFactory::Release()
{
	DecRef();
}
