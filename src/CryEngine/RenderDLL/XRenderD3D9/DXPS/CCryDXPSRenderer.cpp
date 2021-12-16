#include "StdAfx.h"
#include "Layer0/CCryDXPS.hpp"
#include "CCryDXPSRenderer.hpp"
#include "Device/Resource/Textures/CCryDXPSTexture2D.hpp"
#include <string.h>
#include <stdio.h>
#include <cell/fs/cell_fs_file_api.h>
#include <sys/fs_external.h>
#include <Cg/cgc.h>


#define REMOTE_CG_COMPILE


long D3D11CreateDevice(IDXGIAdapter*					pAdapter,
											 D3D_DRIVER_TYPE 		DriverType,
											 HMODULE Software,
											 UINT Flags,
											 CONST D3D_FEATURE_LEVEL *pFeatureLevels,
											 UINT FeatureLevels,
											 UINT SDKVersion,
											 ID3D11Device**				ppDevice,
											 D3D_FEATURE_LEVEL *pFeatureLevel,
											 ID3D11Device **ppImmediateContext)
{
	*ppDevice	=	(CCryDXPSRenderDevice*)CRY_DXPS_CREATE(CCryDXPSRenderDevice,(pAdapter,true));
	if(ppImmediateContext)
		*ppImmediateContext = *ppDevice;
	return ppDevice?0:-1;
}
long D3D11CreateDeviceAndSwapChain(IDXGIAdapter*					pAdapter,
																	 D3D_DRIVER_TYPE 				DriverType,
																	 HMODULE								Software,
																	 uint32									Flags,
																	 const D3D_FEATURE_LEVEL *pFeatureLevels,
																	 uint32									FeatureLevels,
																	 uint32									SDKVersion,
																	 DXGI_SWAP_CHAIN_DESC*	pSwapChainDesc,
																	 IDXGISwapChain**				ppSwapChain,    
																	 ID3D11Device**					ppDevice,
																	 D3D_FEATURE_LEVEL *		pFeatureLevel,
																	 ID3D11DeviceContext **	ppImmediateContext)
{
	long RDevice	=	D3D11CreateDevice(pAdapter,DriverType,Software,Flags,pFeatureLevels,FeatureLevels,SDKVersion,ppDevice,pFeatureLevel,ppImmediateContext);
	if(RDevice<0)
		return RDevice;
	*ppSwapChain	=	CRY_DXPS_CREATE(CCryDXPSSwapChain,((CCryDXPSRenderDevice*)*ppDevice,pSwapChainDesc));
#ifdef PSGL
	cgRTCgcInit();	//initialize CG for runtime shader compilation 
#endif
	return ppSwapChain?0:-1;
}

long D3D10CreateBlob(size_t NumBytes, LPD3D10BLOB *ppBuffer)
{
	*ppBuffer	=	CRY_DXPS_CREATE(CCryDXPSBlob,(NumBytes));//creation with RefCount 1
	if(!(*ppBuffer)->GetBufferPointer())
	{
		delete *ppBuffer;
		*ppBuffer	=	0;
	}
	return ppBuffer?0:-1;
}

size_t cgSrvCompile(const bool VertexShader,
										const char *program,
										const char *entry,
										const char *comment,
										const char **options,
										char *buffer,
										size_t bufferSize);
size_t cgSrvCompile2(const bool VertexShader,
										const char *program,
										const char *entry,
										const char *comment,
										const char* pCompileFlags,
										const char **options,
										char *buffer,
										size_t bufferSize);

static char g_CombinationBuffer[256]={0};
long D3DX11CompileFromMemory(				const char*								pSrcData,
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
																		long*											pResult)
{
	*pResult	=	0;

	if(pDefines!=0 || pInclude!=0 || pPump!=0 || ppErrorMsgs==0)
		return -1;

	const size_t bufferSize = 0x40000;
	char *pLocalShader = new char[bufferSize];
/*	char *pLocalShader2 = new char[bufferSize];
	const char* args[]={"-fastmath",0};
	const int32 Size2	=	cgSrvCompile(	*pProfile=='v',//?"sce_vp_rsx":"sce_fp_rsx",
																		pSrcData,
																		pFunctionName,
																		0,
																		args,
																		pLocalShader2,
																		bufferSize);*/
	const int32 Size	=	cgSrvCompile2(*pProfile=='v',//?"sce_vp_rsx":"sce_fp_rsx",
																		pSrcData,
																		pFunctionName,
																		g_CombinationBuffer,
																		0,
																		0,
																		pLocalShader,
																		bufferSize);
	const int Ret=Size;
	if(Ret<0)
	{
		CRY_DEBUGOUT("Error 0x%x on compiling : %s \n",Ret,pFunctionName);
		delete[]	pLocalShader;
		return -1;
	}
	else
	{
		CRY_DEBUGOUT("Successfully compiled %s of %s\n",pFunctionName,pSrcFile);
	}
	pLocalShader[Size] = 0;

	(*ppShader)	=	CRY_DXPS_CREATE(CCryDXPSBlob,(&pLocalShader[4],Size+1-4));
	delete[]	pLocalShader;

	return 0;
}

void D3D10SentCombition(						const char*								pCombination)
{
	if(strlen(pCombination)<sizeof(g_CombinationBuffer))
		strcpy(g_CombinationBuffer,pCombination);
}


HRESULT D3DReflect(					const void*		pShaderBytecode,
														size_t				BytecodeLength,
														UINT					pInterface,
														void**				ppReflector)
{
	if (!pShaderBytecode || !*(uint32*)pShaderBytecode)
	{
		gEnv->pLog->LogError("Illegal shader byte code passed to D3D10ReflectShader()!");
		return -1;
	}
	(*(ID3D11ShaderReflection**)ppReflector)=	CRY_DXPS_CREATE(CCryDXPSShaderReflection,((const char*)pShaderBytecode));
	return 0;
}

HRESULT D3DDisassemble(const void*							pShader,
											 size_t										BytecodeLength,
											 uint32										nFlags,
											 ID3D10Blob**							ppComments,
											 ID3D10Blob**							ppDisassembly)
{
	uint32 ByteSize	=	reinterpret_cast<const uint32*>(pShader)[0];
	const CDXPSShaderDesc* pDesc	=	reinterpret_cast<const CDXPSShaderDesc*>(&reinterpret_cast<const char*>(pShader)[4]);


	char* pTypes[12]	=	{	"half",
												"float",
												"bool",
												"bool2",
												"bool3",
												"bool4",
												"half2",
												"half3",
												"half4",
												"float2",
												"float3",
												"float4"};



	string Reflect	=	pDesc->PixelShader()?"Pixelshader\n\n":"Vertexshader\n\n";
	//vertex attributes
	if(!pDesc->PixelShader())
	{
		char* pAttributeName[16]	=	{
"ATTR0, POSITION Vertex (float4) ",
"ATTR1, BLENDWEIGHT Vertex weight (float) ",
"ATTR2, NORMAL Normal (float3) ",
"ATTR3, COLOR, COLOR0, DIFFUSE Color (float4) ",
"ATTR4, COLOR1, SPECULAR Secondary color (float4) ",
"ATTR5, FOGCOORD, TESSFACTOR Fog coordinate (float) ",
"ATTR6, PSIZE Point size (float) ",
"ATTR7, BLENDINDICES Palette index for skinning (float4) ",
"ATTR8, TEXCOORD0 Texture coordinate 0 (float4) ",
"ATTR9, TEXCOORD1 Texture coordinate 1 (float4) ",
"ATTR10, TEXCOORD2 Texture coordinate 2 (float4) ",
"ATTR11, TEXCOORD3 Texture coordinate 3 (float4) ",
"ATTR12, TEXCOORD4 Texture coordinate 4 (float4) ",
"ATTR13, TEXCOORD5 Texture coordinate 5 (float4) ",
"ATTR14, TEXCOORD6, TANGENT Texture coordinate 6 (float4),Tangent vector (float4) ",
"ATTR15, TEXCOORD7, BINORMAL "};

		Reflect+="Vertex attributes:\n";
		for(uint32 a=0;a<pDesc->VertexAttributeCount();a++)
		{
			const SRefVertexAttribute*	pAttr	=	&pDesc->VertexAttribute()[a];
			const char* pName			=	&pDesc->NameTable()[pAttr->m_SemanticNameIndex];
			const char* pSemantic	=	pAttributeName[pAttr->m_InputOffset];
			char Text[1024];
			sprintf(Text,"%s semantic: %s, element count: %d\n",pName,pSemantic,pAttr->m_Size);
			Reflect+=Text;
		}
		Reflect+="\n";
	}

	//sampler
	{
		Reflect+="sampler:\n";
		for(uint32 a=0;a<pDesc->SamplerCount();a++)
		{
			const SRefSampler* pSampler	=	&pDesc->Sampler()[a];
			const char* pName		=	&pDesc->NameTable()[pSampler->m_NameIndex];
			char Text[1024];
			sprintf(Text,"%s  index: %d\n",pName,pSampler->m_SamplerIndex);
			Reflect+=Text;
		}
		Reflect+="\n";
	}
	
	//consants
	{
		Reflect+="constants\n";

		for(uint32 a=0;a<pDesc->ConstantCount();a++)
		{
			const SRefConstant*	pConst	=	&pDesc->Constant()[a];
			const char* pName		=	&pDesc->NameTable()[pConst->m_NameIndex];
			char Text[1024];
			sprintf(Text,"%s[%d] %s  c[%d] cb[%d]\n",pTypes[pConst->m_Type],pConst->m_Size,pName,pConst->m_ConstRegister,pConst->m_ConstSlot);
			Reflect+=Text;
		}
		Reflect+="\n";
	}

	//OutputDebugString(Reflect.c_str());

	*ppDisassembly	=	CRY_DXPS_CREATE(CCryDXPSBlob,(Reflect.size()));//creation with RefCount 1
	if(!(*ppDisassembly)->GetBufferPointer())
	{
		delete *ppDisassembly;
		*ppDisassembly	=	0;
		return -1;
	}
	else
	{
		memcpy((*ppDisassembly)->GetBufferPointer(),Reflect.c_str(),Reflect.size());
	}

	return 0;
}

HRESULT D3DX11CreateTextureFromMemory(
																		ID3D11Device*							pDevice,
																		const void*								pSrcData,
																		size_t										SrcDataSize,
																		D3DX11_IMAGE_LOAD_INFO*		pLoadInfo,
																		ID3DX11ThreadPump*				pPump,
																		ID3D11Resource**					ppTexture,
																		long*											pResult)
{
	*pResult	=	0;
	if(pLoadInfo->pSrcInfo->ResourceDimension==D3D11_RESOURCE_DIMENSION_TEXTURE2D)
	{
		D3D11_TEXTURE2D_DESC Data;
		Data.Format	=	pLoadInfo->pSrcInfo->Format;
		Data.Height	=	pLoadInfo->pSrcInfo->Height;
		Data.Width	=	pLoadInfo->pSrcInfo->Width;
		if(-1==pDevice->CreateTexture2D(&Data,0,reinterpret_cast<CCryDXPSTexture2D**>(ppTexture)))
			return -1;

		D3D11_MAPPED_SUBRESOURCE ResData;
		if(-1==(*reinterpret_cast<CCryDXPSTexture2D**>(ppTexture))->Map(0,D3D11_MAP(),0,&ResData))
			return -1;
		/*
		for(uint32 y=0;y<Data.Height;y++)
			for(uint32 x=0;x<Data.Width && x<ResData.RowPitch;y++)
				reinterpret_cast<uint8*>(ResData.pData)[x+y*ResData.RowPitch]	=	
				reinterpret_cast<const uint8*>(pSrcData)[x+y*Data.Width];
				*/
		//this function is just used for weird runtime texture conversion so we output just debug noise
//		for(uint32 y=0;y<Data.Height;y++)
//			for(uint32 x=0;x<Data.Width && x<ResData.RowPitch;y++)
//				reinterpret_cast<uint32*>(ResData.pData)[(x+y*ResData.RowPitch)/4]	=	x+y*Data.Width;

		return 0;
	}
	return -1;
}

HRESULT D3DX11SaveTextureToFile(ID3D11DeviceContext*			pDevice,
																ID3D11Resource*						pSrcResource,
																D3DX11_IMAGE_FILE_FORMAT	fmt,
																const char*								pDestFile)
{

	if(pSrcResource->Type()!=EDXPS_RT_TEX2D)
		return -1;

	CCryDXPSTexture2D* pSrc	=	(CCryDXPSTexture2D*)pSrcResource;

	int fd;
	const char FileName[] = SYS_APP_HOME"/screenshot";
	char FN[1024];
	int a=0;
	CellFsErrno err;
	do
	{
		if(a==9999)
			return -1;
		sprintf(FN,"%s%.4d.tga",FileName,a++);
		err = cellFsOpen(	FN,CELL_FS_O_CREAT | CELL_FS_O_EXCL | CELL_FS_O_WRONLY,&fd, NULL, 0);
	}while(err != CELL_OK);

	char Header[18];
	Header[0]	=	0;//id
	Header[1]	=	0;//palette?
	Header[2]	=	2;//=rgb
	Header[3]	=	0;//
	Header[4]	=	0;//first color in palette
	Header[5]	=	0;//
	Header[6]	=	0;//palette entry count
	Header[7]	=	0;//bpp in palette
	Header[8]	=	0;//
	Header[9]	=	0;//offX
	Header[10]	=	0;//
	Header[11]	=	0;//Offy
	Header[12]	=	pSrc->SizeX();//
	Header[13]	=	pSrc->SizeX()>>8;//sizex
	Header[14]	=	pSrc->SizeY();//
	Header[15]	=	pSrc->SizeY()>>8;//sizey
	Header[16]	=	32;//bpp
	Header[17]	=	1<<5;//id
	cellFsWrite(fd,Header,sizeof(Header),NULL);


	const uint32	SrcPitch	=	pSrc->Pitch(0);
	const uint32	LineLen		=	SrcPitch;
	D3D11_MAPPED_SUBRESOURCE pSrcPtr;
	pSrc->Map(0,D3D11_MAP_VOID,0,&pSrcPtr);
	uint8 *Tmp = new uint8[1024*64];
	for(uint32 y=0,SizeY=pSrc->SizeY(),SizeX=LineLen;y<SizeY;y++)
	{
		uint8* pData	=	&reinterpret_cast<uint8*>(pSrcPtr.pData)[y*SrcPitch];
		for(uint32 x=0;x<SizeX;x+=4)
		{
			Tmp[x+3]	=	pData[x+0];
			Tmp[x+2]	=	pData[x+1];
			Tmp[x+1]	=	pData[x+2];
			Tmp[x+0]	=	pData[x+3];
		}
		cellFsWrite(fd,Tmp,LineLen,0);
	}
	delete[] Tmp;

	pSrc->Unmap(0);

	printf("Saved texture to: %s\n",FN);

	cellFsClose(fd);

	return 0;
}
