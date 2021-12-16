#ifndef __CRYDXPSGIFACTORY__
#define __CRYDXPSGIFACTORY__

#include "CCryDXPSGIAdapter.hpp"
#include "../Layer0/CCryDXPS.hpp"



typedef void *HMODULE;

class CCryDXPSGIFactory;
class CCryDXPSGIFactory		:		public	CCryRefAndWeak<CCryDXPSGIFactory>
{
	CCryDXPSGIAdapter			m_Adapter;
public:

												CCryDXPSGIFactory();
    HRESULT 						EnumAdapters( uint32 Adapter,IDXGIAdapter **ppAdapter);
    HRESULT 						MakeWindowAssociation(HWND WindowHandle,uint32 Flags);
    HRESULT 						GetWindowAssociation( HWND *pWindowHandle);
    HRESULT 						CreateSwapChain(ID3D11Device* pDevice,DXGI_SWAP_CHAIN_DESC *pDesc,IDXGISwapChain **ppSwapChain);
    HRESULT 						CreateSoftwareAdapter( HMODULE Module,IDXGIAdapter **ppAdapter);
		bool								Locked()const{return false;}
		void								Release();
		void								FreeMe(){delete this;}
		ILINE	void					ReleaseResources(){}
};

typedef CCryDXPSGIFactory IDXGIFactory;

#endif

