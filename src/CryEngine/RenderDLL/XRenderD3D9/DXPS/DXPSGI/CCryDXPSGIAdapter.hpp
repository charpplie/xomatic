#ifndef __CRYDXPSGIADAPTER__
#define __CRYDXPSGIADAPTER__

#include "CCryDXPSGIOutput.hpp"
#include "../Layer0/CCryDXPS.hpp"


class CCryDXPSGIAdapter;
class CCryDXPSGIAdapter		:		public	CCryRefAndWeak<CCryDXPSGIAdapter>
{
	CCryDXPSGIOutput					m_Output;
public:
														CCryDXPSGIAdapter();
		ECRYDXPSResType					Type()const{return EDXPS_RT_ADAPTER;}
		HRESULT 								EnumOutputs( uint32 Output,IDXGIOutput **ppOutput);
    HRESULT 								GetDesc( DXGI_ADAPTER_DESC *pDesc);
    HRESULT 								CheckInterfaceSupport( REFGUID InterfaceName,uint32 *pUMDVersion);
		bool										Locked()const{return false;}
		void										Release();
		void										FreeMe(){delete this;}
		ILINE	void							ReleaseResources(){}

   
};

typedef CCryDXPSGIAdapter IDXGIAdapter;

#endif

