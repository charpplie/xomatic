#ifndef __CRYDXPSSAMPLERSTATE__
#define __CRYDXPSSAMPLERSTATE__

#include "../CCryDXPSResource.hpp"
#include "../../../Layer0/CCryDXPS.hpp"

class CCryDXPSSamplerState;
class CCryDXPSSamplerState		:		public CCryDXPSResource	,	public CCryRefAndWeak<CCryDXPSSamplerState>
{
private:
	// D3D11_FILTER Filter;
	// FLOAT MipLODBias;
	// cellGcmSetTextureFilter
	//uint16		m_Bias;
	uint8			m_FilterMin;
	uint8			m_FilterMag;

  // UINT MaxAnisotropy;
	// cellGcmSetTextureControl
	// cellGcmSetTextureFilter
	uint8			m_AnisotropicLevel;

	// D3D11_TEXTURE_ADDRESS_MODE AddressU;
	// D3D11_TEXTURE_ADDRESS_MODE AddressV;
	// D3D11_TEXTURE_ADDRESS_MODE AddressW;
	// cellGcmSetTextureAddress
	uint8			m_WrapS;
	uint8			m_WrapT;
	uint8			m_WrapR;

	//D3D11_COMPARISON_FUNC ComparisonFunc;
	//FLOAT BorderColor[4];
	uint32	m_BorderColor;

  //FLOAT MinLOD;
	//FLOAT MaxLOD;
	//cellGcmSetTextureControl
//	uint16	m_LODMin;
//	uint16	m_LODMax;

public:
													CCryDXPSSamplerState(const D3D11_SAMPLER_DESC&	rDesc MMRES_PARAM);
													~CCryDXPSSamplerState()
													{
														MMRES_SUBCOUNT();
														MMRES_SUBMM(sizeof(CCryDXPSSamplerState));
#if defined(CRY_DXPS_DESTRUCTORVALIDATION)
														if(EDXPS_RT_SAMPLERSTATE==CCryDXPSResource::Type())
															CCryDXPSResource::Type(EDXPS_RT_NONE);
#endif
													}

	inline void							ReleaseResources(){}
  inline void							GetDesc( D3D11_SAMPLER_DESC *pDesc)
													{
												//		*pDesc	=	m_Desc;
													}

	inline void							Set(uint32 Idx,int16 LODMin,int16 LODMax,uint16 LODBias,uint16 sRGB)	const
													{
														using namespace CRY_DXPS_GCMNAMESPACE;
														cellGcmSetTextureFilter(Idx,LODBias,m_FilterMin,m_FilterMag, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);
                            const uint32 nSRGBFlags = CELL_GCM_TEXTURE_GAMMA_R|CELL_GCM_TEXTURE_GAMMA_G|CELL_GCM_TEXTURE_GAMMA_B;
                            cellGcmSetTextureAddress(Idx,m_WrapS,m_WrapT,m_WrapR,CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL,CELL_GCM_TEXTURE_ZFUNC_GREATER, !sRGB?0:nSRGBFlags);
														if(m_WrapS == CELL_GCM_TEXTURE_BORDER || m_WrapT == CELL_GCM_TEXTURE_BORDER || m_WrapR == CELL_GCM_TEXTURE_BORDER)
															cellGcmSetTextureBorderColor(Idx,m_BorderColor);
														cellGcmSetTextureControl(Idx,CELL_GCM_TRUE,LODMin,LODMax,m_AnisotropicLevel);
													}
	inline unsigned long		AddRef(){return IncRef();}
	inline unsigned long		Release(){return DecRef();}
	inline uint8						FilterMin()const{return m_FilterMin;}
	inline uint8						FilterMag()const{return m_FilterMag;}

};

typedef CCryDXPSSamplerState ID3D11SamplerState;
#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrWeakCnt<CCryDXPSSamplerState>	APWeakSamplerState;
#else
	typedef CCryDXPSSamplerState* __restrict 	APWeakSamplerState;
#endif

#endif

