#ifndef __CRYRENDERER__
#define __CRYRENDERER__

//struct DXGI_SWAP_CHAIN_DESC;

#include "Device/CCryDXPSRenderDevice.hpp"
#include "Device/CCryDXPSSwapChain.hpp"
#include "Device/CCryDXPSBlob.hpp"

//typedef class ID3D10Blob ID3D10Blob;
//typedef ID3D10Blob *LPD3D10BLOB;

long D3D11CreateDevice(							IDXGIAdapter*					pAdapter,
																		D3D_DRIVER_TYPE 		DriverType,
																		HMODULE Software,
																		UINT Flags,
																		CONST D3D_FEATURE_LEVEL *pFeatureLevels,
																		UINT FeatureLevels,
																		UINT SDKVersion,
																		ID3D11Device**				ppDevice,
																		D3D_FEATURE_LEVEL *pFeatureLevel,
																		ID3D11Device **ppImmediateContext);

long D3D11CreateDeviceAndSwapChain(	IDXGIAdapter*					pAdapter,
																		D3D_DRIVER_TYPE 			DriverType,
																		HMODULE								Software,
																		uint32								Flags,
																		const D3D_FEATURE_LEVEL *pFeatureLevels,
																		uint32								FeatureLevels,
																		uint32								SDKVersion,
																		DXGI_SWAP_CHAIN_DESC*	pSwapChainDesc,
																		IDXGISwapChain**			ppSwapChain,    
																		ID3D11Device**				ppDevice,
																		D3D_FEATURE_LEVEL *		pFeatureLevel,
																		ID3D11DeviceContext **ppImmediateContext);

long D3D10CreateBlob(								size_t								NumBytes,
										 								LPD3D10BLOB*					ppBuffer);


long D3DX11CompileFromMemory(
																		const char*								pSrcData,
																		size_t										SrcDataLen,
																		const char*								pSrcFile,
																		const D3D11_SHADER_MACRO*	pDefines,
																		LPD3D10INCLUDE						pInclude,
																		const char*								pFunctionName,
																		const char*								pProfile,
																		uint32										Flags1,
																		uint32										Flags2,
																		ID3DX11ThreadPump*				pPump,
																		ID3D10Blob**							ppShader,
																		ID3D10Blob**							ppErrorMsgs,
																		long*											pResult);

void D3D10SentCombition(						const char*								pCombination);
HRESULT D3DReflect(									const void*								pShaderBytecode,
																		size_t										BytecodeLength,
																		UINT											pInterface,
																		void**										ppReflector);

HRESULT D3DDisassemble(							const void*								pShader,
																		size_t										BytecodeLength,
																		uint32										nFlags,
																		ID3D10Blob**							ppComments,
																		ID3D10Blob**							ppDisassembly);

HRESULT D3DX11SaveTextureToFile(		ID3D11DeviceContext*			pDevice,
																		ID3D11Resource*						pSrcTexture,
																		D3DX11_IMAGE_FILE_FORMAT	DestFormat,
																		const char*								pDestFile);

HRESULT D3DX11CreateTextureFromMemory(
																		ID3D11Device*							pDevice,
																		const void*								pSrcData,
																		size_t										SrcDataSize,
																		D3DX11_IMAGE_LOAD_INFO*		pLoadInfo,
																		ID3DX11ThreadPump*				pPump,
																		ID3D11Resource**					ppTexture,
																		long*											pResult);
#endif

