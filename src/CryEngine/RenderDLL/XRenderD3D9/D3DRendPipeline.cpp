/*=============================================================================
D3DRendPipeline.cpp : Direct3D rendering pipeline.
Copyright (c) 2001-2004 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include <I3DEngine.h>
#include <CryHeaders.h>
#include "../Common/PostProcess/PostProcessUtils.h"

using namespace NPostEffects;

#pragma warning(disable: 4244)

//============================================================================================
// Shaders rendering
//============================================================================================

//============================================================================================
// Init Shaders rendering

void CD3D9Renderer::EF_InitWaveTables()
{
  int i;

  //Init wave Tables
	for (i=0; i<SRenderPipeline::sSinTableCount; i++)
  {
    float f = (float)i;

    m_RP.m_tSinTable[i] = sin_tpl(f * (360.0f/(float)SRenderPipeline::sSinTableCount) * (float)M_PI / 180.0f);
  }
}

// build vertex declarations on demand (for programmable pipeline)
void CD3D9Renderer::EF_OnDemandVertexDeclaration( SOnDemandD3DVertexDeclaration &out, 
	const int nStreamMask, const int vertexformat, const bool bMorph )
{
//	iLog->Log("EF_OnDemandVertexDeclaration %d %d %d (DEBUG test - shouldn't log too often)",nStreamMask,vertexformat,bMorph?1:0);

	if (!m_RP.m_D3DVertexDeclaration[vertexformat].m_Declaration.Num())
    return;

	uint32 j;

#if defined (DIRECT3D9)
  for (j=0; j<m_RP.m_D3DVertexDeclaration[vertexformat].m_Declaration.Num()-1; j++)
  {
    out.m_Declaration.AddElem(m_RP.m_D3DVertexDeclaration[vertexformat].m_Declaration[j]);
  }
  for (j=1; j<VSF_NUM; j++)
  {
    if (!(nStreamMask & (1<<(j-1))))
      continue;
    int n = 0;
    while (m_RP.m_D3DStreamProperties[j].m_pElements[n].Stream != 0xff)
    {
      out.m_Declaration.Add(m_RP.m_D3DStreamProperties[j].m_pElements[n]); 
      n++;
    }
  }

	if(bMorph)
	{
		uint32 dwNumWithoutMorph = out.m_Declaration.Num();

		for (j=0; j<dwNumWithoutMorph; j++)
		{
			D3DVERTEXELEMENT9 El = out.m_Declaration[j];
			El.Stream += VSF_MORPHBUDDY;
			El.UsageIndex += 8;
			out.m_Declaration.AddElem(El);
		}
		
		D3DVERTEXELEMENT9 El = {VSF_MORPHBUDDY_WEIGHTS, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 1};	// BlendWeight
		out.m_Declaration.AddElem(El);
	}

	D3DVERTEXELEMENT9 elemEnd = D3DDECL_END();         // terminate
  out.m_Declaration.Add(elemEnd);

#elif defined (DIRECT3D10)
  for (j=0; j<m_RP.m_D3DVertexDeclaration[vertexformat].m_Declaration.Num(); j++)
  {
    out.m_Declaration.AddElem(m_RP.m_D3DVertexDeclaration[vertexformat].m_Declaration[j]);
  }
  for (j=1; j<VSF_NUM; j++)
  {
    if (!(nStreamMask & (1<<(j-1))))
      continue;
    int n;
		for (n=0; n<m_RP.m_D3DStreamProperties[j].m_nNumElements; n++)
    {
      out.m_Declaration.AddElem(m_RP.m_D3DStreamProperties[j].m_pElements[n]); 
    }
  }

	if(bMorph)
	{
		uint32 dwNumWithoutMorph = out.m_Declaration.Num();

		for (j=0; j<dwNumWithoutMorph; j++)
		{
			D3D11_INPUT_ELEMENT_DESC El = out.m_Declaration[j];
			El.InputSlot += VSF_MORPHBUDDY;
			El.SemanticIndex += 8;
			out.m_Declaration.AddElem(El);
		}
		D3D11_INPUT_ELEMENT_DESC El = {"BLENDWEIGHT", 1, DXGI_FORMAT_R32G32_FLOAT, VSF_MORPHBUDDY_WEIGHTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}; // BlendWeight
		out.m_Declaration.AddElem(El);
	}
#endif
}

void CD3D9Renderer::EF_InitD3DVertexDeclarations()
{
  int i;

#if defined (DIRECT3D9)
  SBufInfoTable *pOffs;
  int n = 0;

  //========================================================================================
  // base stream declarations (stream 0)
  D3DVERTEXELEMENT9 elemPos = {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};

#ifdef FP16_MESH
  D3DVERTEXELEMENT9 elemPosHalf = {0, 0, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};  // position
  D3DVERTEXELEMENT9 elemTCHalf = {0, 0, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0};  // texture
#endif



  D3DVERTEXELEMENT9 elemPosTR = {0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0};  // position


  D3DVERTEXELEMENT9 elemNormalB = {0, 0, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0};
  D3DVERTEXELEMENT9 elemPS = {0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE, 0};      // psize
  D3DVERTEXELEMENT9 elemColor = {0, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0};    // diffuse
  D3DVERTEXELEMENT9 elemTC = {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0};      // texture
  D3DVERTEXELEMENT9 elemTC3 = {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0};      // texture
  D3DVERTEXELEMENT9 elemEnd = D3DDECL_END();                                                                   // terminate
  for (n=0; n<eVF_Max; n++)
  {
    pOffs = &CRenderMesh2::m_cBufInfoTable[n];
    if (n == eVF_TP3F_C4B_T2F || n == eVF_TP3F_T2F_T3F)
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemPosTR);
    else
#ifdef FP16_MESH
    if (n == eVF_P3S_C4B_T2S || n == eVF_P3S_N4B_C4B_T2S )
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemPosHalf);
    else
#endif
    m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemPos);
    if (n == eVF_P3S_N4B_C4B_T2S)
    {
      elemNormalB.Offset = sizeof(Vec3f16);
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemNormalB);
    }
    if (pOffs->OffsColor)
    {
      elemColor.Offset = pOffs->OffsColor;
      elemColor.UsageIndex = 0;
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemColor);
    }
    if (n == eVF_P3F_C4B_I4B_PS4F)
    {
      elemColor.Offset = (int)(UINT_PTR)&(((SVF_P3F_C4B_I4B_PS4F *)0)->info);
      elemColor.UsageIndex = 1;
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemColor);

      elemPS.Offset = (int)(UINT_PTR)&(((SVF_P3F_C4B_I4B_PS4F *)0)->xaxis);
      elemPS.UsageIndex = 0;
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemPS);
    }
    if (pOffs->OffsTC)
    {
      elemTC.Offset = pOffs->OffsTC;
      elemTC.UsageIndex = 0;
#ifdef FP16_MESH
      if (n == eVF_P3S_C4B_T2S || n == eVF_P3S_N4B_C4B_T2S)
      {
        elemTCHalf.Offset = pOffs->OffsTC;
        elemTCHalf.UsageIndex = 0;
        m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemTCHalf);
      }
      else
#endif
      if (n == eVF_P3F_T3F)
      {
        elemTC3.Offset = pOffs->OffsTC;
        elemTC3.UsageIndex = 0;
        m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemTC3);
      }
      else
        m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemTC);
      if (n == eVF_TP3F_T2F_T3F || n == eVF_P3F_T2F_T3F)
      {
        elemTC3.Offset = pOffs->OffsTC+8;
        elemTC3.UsageIndex = 1;
        m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemTC3);
      }
    }
    m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemEnd);
    m_RP.m_D3DVertexDeclaration[n].m_Declaration.Shrink();
  }

  //=============================================================================
  // Additional streams declarations:

  // Tangents stream
  static D3DVERTEXELEMENT9 VElemTangents[] =
  {
#ifdef TANG_FLOATS
    {VSF_TANGENTS, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},  // tangent
    {VSF_TANGENTS, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},  // binormal
#else
    {VSF_TANGENTS, 0, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},  // tangent
    {VSF_TANGENTS, 8, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},  // binormal
#endif
    D3DDECL_END()
  };
  // Tangents stream
  static D3DVERTEXELEMENT9 VElemQTangents[] =
  {
#ifdef TANG_FLOATS
    {VSF_QTANGENTS, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},  // tangent
#else
    {VSF_QTANGENTS, 0, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},  // tangent
#endif
    D3DDECL_END()
  };
  /*static D3DVERTEXELEMENT9 VElemLMTC[] =
  {
    {VSF_LMTC, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},  // LM texcoord
    D3DDECL_END()
  };*/

  //HW Skin stream
  static D3DVERTEXELEMENT9 VElemHWSkin[] =
  {
    {VSF_HWSKIN_INFO, 0, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,	0	},	// BlendWeight
    {VSF_HWSKIN_INFO, 4, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0	},	// BlendIndices
    {VSF_HWSKIN_INFO, 8, D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,			4	},	// BoneSpace
    D3DDECL_END()
  };

  static D3DVERTEXELEMENT9 VElemHWSkin_ShapeDeformation[] =
  {
//#ifdef FP16_MESH
//    {VSF_HWSKIN_SHAPEDEFORM_INFO, 0, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4}, // thin vertex pos
//    {VSF_HWSKIN_SHAPEDEFORM_INFO, 8, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 5}, // fat vertex pos
//    {VSF_HWSKIN_SHAPEDEFORM_INFO, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1}, // index
//#else
    {VSF_HWSKIN_SHAPEDEFORM_INFO, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4}, // thin vertex pos
    {VSF_HWSKIN_SHAPEDEFORM_INFO, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 5}, // fat vertex pos
    {VSF_HWSKIN_SHAPEDEFORM_INFO, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1}, // index
//#endif
    D3DDECL_END()
  };

  static D3DVERTEXELEMENT9 VElemHWSkin_MorphTargets[] =
  {
    {VSF_HWSKIN_MORPHTARGET_INFO, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 3}, // morph target
    D3DDECL_END()
  };

  //SH coef. stream
  static D3DVERTEXELEMENT9 VElemSH[] =
  {
    {VSF_SH_INFO, 0, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 2}, // SH coefs0
    {VSF_SH_INFO, 4, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 3}, // SH coefs1
    D3DDECL_END()
  };

  // stream 1 (Tangent basis vectors)
  // stream 2 (LM tc)
  // stream 3 (HW skin info)
  // stream 4 (SH coefs)
  // stream 5 (Shape deform)
  // stream 6 (Morph target)
	m_RP.m_D3DStreamProperties[VSF_GENERAL].m_pElements = NULL;
	m_RP.m_D3DStreamProperties[VSF_TANGENTS].m_pElements = VElemTangents;
  m_RP.m_D3DStreamProperties[VSF_QTANGENTS].m_pElements = VElemQTangents;
	//m_RP.m_D3DStreamProperties[VSF_LMTC].m_pElements=VElemLMTC;
	m_RP.m_D3DStreamProperties[VSF_HWSKIN_INFO].m_pElements = VElemHWSkin;
	m_RP.m_D3DStreamProperties[VSF_SH_INFO].m_pElements = VElemSH;
	m_RP.m_D3DStreamProperties[VSF_HWSKIN_SHAPEDEFORM_INFO].m_pElements = VElemHWSkin_ShapeDeformation;
	m_RP.m_D3DStreamProperties[VSF_HWSKIN_MORPHTARGET_INFO].m_pElements = VElemHWSkin_MorphTargets;

/*
  // Vertex declarations for mixed streams
  for (i=1; i<(1<<VSF_NUM); i++)
  {
    EF_CreateVertexDeclarations(i, pStreamVElements);
  }
*/






#elif defined (DIRECT3D10)
  SBufInfoTable *pOffs;
  int n = 0;

  //========================================================================================
  // base stream declarations (stream 0)

#ifdef FP16_MESH
  D3D11_INPUT_ELEMENT_DESC elemPosHalf = {"POSITION", 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};
  D3D11_INPUT_ELEMENT_DESC elemTCHalf = {"TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};
#endif

  D3D11_INPUT_ELEMENT_DESC elemPos = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};

  D3D11_INPUT_ELEMENT_DESC elemPosTR = {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};  // position

  D3D11_INPUT_ELEMENT_DESC elemNormalB = {"NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};
  D3D11_INPUT_ELEMENT_DESC elemPS = {"PSIZE", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};   // psize
  D3D11_INPUT_ELEMENT_DESC elemColor = {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};        // diffuse
  D3D11_INPUT_ELEMENT_DESC elemTC0 = {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};      // texture
  D3D11_INPUT_ELEMENT_DESC elemTC1 = {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};      // texture
  D3D11_INPUT_ELEMENT_DESC elemTC1_3 = {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};      // texture
  for (n=0; n<eVF_Max; n++)
  {
    pOffs = &CRenderMesh2::m_cBufInfoTable[n];
    if (n == eVF_TP3F_C4B_T2F || n == eVF_TP3F_T2F_T3F)
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemPosTR);
    else
#ifdef FP16_MESH
    if (n == eVF_P3S_C4B_T2S || n == eVF_P3S_N4B_C4B_T2S )
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemPosHalf);
    else
#endif
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemPos);
    if (n == eVF_P3S_N4B_C4B_T2S)
    {
      elemNormalB.AlignedByteOffset = sizeof(Vec3f16);
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemNormalB);
    }
    if (pOffs->OffsColor)
    {
      elemColor.AlignedByteOffset = pOffs->OffsColor;
      elemColor.SemanticIndex = 0;
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemColor);
    }
    if (n == eVF_P3F_C4B_I4B_PS4F)
    {
      elemColor.AlignedByteOffset = (int)(UINT_PTR)&(((SVF_P3F_C4B_I4B_PS4F *)0)->info);
      elemColor.SemanticIndex = 1;
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemColor);
    }
    if (pOffs->OffsTC)
    {
      elemTC0.AlignedByteOffset = pOffs->OffsTC;
      elemTC0.SemanticIndex = 0;
#ifdef FP16_MESH
      if (n == eVF_P3S_C4B_T2S || n == eVF_P3S_N4B_C4B_T2S)
      {
        elemTCHalf.AlignedByteOffset = pOffs->OffsTC;
        elemTCHalf.SemanticIndex = 0;
        m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemTCHalf);
      }
      else
#endif
      if (n == eVF_P3F_T3F)
      {
        elemTC1_3.AlignedByteOffset = pOffs->OffsTC;
        elemTC1_3.SemanticIndex = 0;
        m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemTC1_3);
      }
      else
        m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemTC0);
      if (n == eVF_TP3F_T2F_T3F || n == eVF_P3F_T2F_T3F )
      {
        elemTC1_3.AlignedByteOffset = pOffs->OffsTC+8;
        elemTC1_3.SemanticIndex = 1;
        m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemTC1_3);
      }
    }
    if (n == eVF_P3F_C4B_I4B_PS4F)
    {
      elemPS.AlignedByteOffset = (int)(UINT_PTR)&(((SVF_P3F_C4B_I4B_PS4F *)0)->xaxis);
      elemPS.SemanticIndex = 0;
      m_RP.m_D3DVertexDeclaration[n].m_Declaration.AddElem(elemPS);
    }
    m_RP.m_D3DVertexDeclaration[n].m_Declaration.Shrink();
  }

  //=============================================================================
  // Additional streams declarations:

  // Tangents stream
  static D3D11_INPUT_ELEMENT_DESC VElemTangents[] =
  {
#ifdef TANG_FLOATS
    {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, VSF_TANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},   // Binormal
    {"BINORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, VSF_TANGENTS, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}, // Tangent
#else
    {"TANGENT", 0, DXGI_FORMAT_R16G16B16A16_SNORM, VSF_TANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},   // Binormal
    {"BINORMAL", 0, DXGI_FORMAT_R16G16B16A16_SNORM, VSF_TANGENTS, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},  // Tangent
#endif
  };
  // Tangents stream
  static D3D11_INPUT_ELEMENT_DESC VElemQTangents[] =
  {
#ifdef TANG_FLOATS
    {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, VSF_QTANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},   // Binormal
#else
    {"TANGENT", 0, DXGI_FORMAT_R16G16B16A16_SNORM, VSF_QTANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},   // Binormal
#endif
  };
  /*static D3D11_INPUT_ELEMENT_DESC VElemLMTC[] =
  {
    {"TEXCOORD1", 0, DXGI_FORMAT_R32G32_FLOAT, VSF_LMTC, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, // LM texcoord
  };*/

  //HW Skin stream
  static D3D11_INPUT_ELEMENT_DESC VElemHWSkin[] =
  {
    {"BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, VSF_HWSKIN_INFO, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, // BlendWeight
    {"BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UNORM, VSF_HWSKIN_INFO, 4, D3D11_INPUT_PER_VERTEX_DATA, 0}, // BlendIndices
    {"POSITION", 4, DXGI_FORMAT_R32G32B32_FLOAT, VSF_HWSKIN_INFO, 8, D3D11_INPUT_PER_VERTEX_DATA, 0}, // BoneSpace
  };

  static D3D11_INPUT_ELEMENT_DESC VElemHWSkin_ShapeDeformation[] =
  {
    {"TEXCOORD", 4, DXGI_FORMAT_R32G32B32_FLOAT, VSF_HWSKIN_SHAPEDEFORM_INFO, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, // thin vertex pos
    {"TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT, VSF_HWSKIN_SHAPEDEFORM_INFO, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}, // fat vertex pos
    {"COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, VSF_HWSKIN_SHAPEDEFORM_INFO, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}, // index
  };

  static D3D11_INPUT_ELEMENT_DESC VElemHWSkin_MorphTargets[] =
  {
    {"POSITION", 3, DXGI_FORMAT_R32G32B32_FLOAT, VSF_HWSKIN_MORPHTARGET_INFO, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, // morph target
  };

  //SH coef. stream
  static D3D11_INPUT_ELEMENT_DESC VElemSH[] =
  {
    {"COLOR", 2, DXGI_FORMAT_R8G8B8A8_UINT, VSF_SH_INFO, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, // SH coefs0
    {"COLOR", 3, DXGI_FORMAT_R8G8B8A8_UINT, VSF_SH_INFO, 4, D3D11_INPUT_PER_VERTEX_DATA, 0}, // SH coefs1
  };

  // stream 1 (Tangent basis vectors)
  // stream 2 (LM tc)
  // stream 3 (HW skin info)
  // stream 4 (SH coefs)
  // stream 5 (Shape deform)
  // stream 6 (Morph target)
	m_RP.m_D3DStreamProperties[VSF_GENERAL].m_pElements=NULL;
	m_RP.m_D3DStreamProperties[VSF_GENERAL].m_nNumElements=0;
	m_RP.m_D3DStreamProperties[VSF_TANGENTS].m_pElements=VElemTangents;
	m_RP.m_D3DStreamProperties[VSF_TANGENTS].m_nNumElements=sizeof(VElemTangents) / sizeof(D3D11_INPUT_ELEMENT_DESC);
  m_RP.m_D3DStreamProperties[VSF_QTANGENTS].m_pElements=VElemQTangents;
  m_RP.m_D3DStreamProperties[VSF_QTANGENTS].m_nNumElements=sizeof(VElemQTangents) / sizeof(D3D11_INPUT_ELEMENT_DESC);
	//m_RP.m_D3DStreamProperties[VSF_LMTC].m_pElements=VElemLMTC;
	//m_RP.m_D3DStreamProperties[VSF_LMTC].m_nNumElements=sizeof(VElemLMTC) / sizeof(D3D11_INPUT_ELEMENT_DESC);
	m_RP.m_D3DStreamProperties[VSF_HWSKIN_INFO].m_pElements=VElemHWSkin;
	m_RP.m_D3DStreamProperties[VSF_HWSKIN_INFO].m_nNumElements=sizeof(VElemHWSkin) / sizeof(D3D11_INPUT_ELEMENT_DESC);
	m_RP.m_D3DStreamProperties[VSF_SH_INFO].m_pElements=VElemSH;
	m_RP.m_D3DStreamProperties[VSF_SH_INFO].m_nNumElements=sizeof(VElemSH) / sizeof(D3D11_INPUT_ELEMENT_DESC);
	m_RP.m_D3DStreamProperties[VSF_HWSKIN_SHAPEDEFORM_INFO].m_pElements=VElemHWSkin_ShapeDeformation;
	m_RP.m_D3DStreamProperties[VSF_HWSKIN_SHAPEDEFORM_INFO].m_nNumElements=sizeof(VElemHWSkin_ShapeDeformation) / sizeof(D3D11_INPUT_ELEMENT_DESC);
	m_RP.m_D3DStreamProperties[VSF_HWSKIN_MORPHTARGET_INFO].m_pElements=VElemHWSkin_MorphTargets;
	m_RP.m_D3DStreamProperties[VSF_HWSKIN_MORPHTARGET_INFO].m_nNumElements=sizeof(VElemHWSkin_MorphTargets) / sizeof(D3D11_INPUT_ELEMENT_DESC);
/*
  // Vertex declarations for mixed streams
  for (i=1; i<(1<<VSF_NUM); i++)
  {
    EF_CreateVertexDeclarations(i, pStreamVElements, nNumElements);
  }
*/

#endif

  m_CurVertBufferSize = 0;
  m_CurIndexBufferSize = 0;

  for (i=0; i<MAX_STREAMS; i++)
  {
    m_RP.m_ReqStreamFrequence[i] = 1;
    m_RP.m_VertexStreams[i].nFreq = 1;
  }
}

_inline static void *sAlign0x20(byte *vrts)
{
  return (void*)(((INT_PTR)vrts + 0x1f) & ~0x1f);
}

// Init shaders pipeline
void CD3D9Renderer::EF_Init()
{
  bool nv = 0;

  if (CV_r_logTexStreaming && !m_LogFileStr)
  {
    m_LogFileStr = fxopen ("Direct3DLogStreaming.txt", "w");
    if (m_LogFileStr)
    {      
      iLog->Log("Direct3D texture streaming log file '%s' opened", "Direct3DLogStreaming.txt");
      char time[128];
      char date[128];

      _strtime( time );
      _strdate( date );

      fprintf(m_LogFileStr, "\n==========================================\n");
      fprintf(m_LogFileStr, "Direct3D Textures streaming Log file opened: %s (%s)\n", date, time);
      fprintf(m_LogFileStr, "==========================================\n");
    }
  }

  m_RP.m_MaxVerts = CV_d3d9_rb_verts;
  m_RP.m_MaxTris = CV_d3d9_rb_tris;

  iLog->Log("Allocate render buffer for particles (%d verts, %d tris)...", m_RP.m_MaxVerts, m_RP.m_MaxTris);

  int n = 0;

  int nSizeV = sizeof(SVF_P3F_C4B_I4B_PS4F);
  for (int i=0; i<eVF_Max; i++)
    nSizeV = max(nSizeV, CRenderMesh2::m_cSizeVF[i]);

  n += nSizeV * m_RP.m_MaxVerts + 32;

  n += sizeof(SPipTangents) * m_RP.m_MaxVerts + 32;

  //m_RP.mRendIndices;
  n += sizeof(uint16)*3*m_RP.m_MaxTris+32;

	{
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Renderer Particles Buffer");

		byte *buf = new byte [n];
		m_RP.m_SizeSysArray = n;
		m_RP.m_SysArray = buf;
		if (!buf)
			iConsole->Exit("Can't allocate buffers for RB");

		memset(buf, 0, n);

		m_RP.m_Ptr.Ptr = sAlign0x20(buf);
		buf += sizeof(SVF_P3F_C4B_I4B_PS4F) * m_RP.m_MaxVerts + 32;

		m_RP.m_PtrTang.Ptr = sAlign0x20(buf);
		buf += sizeof(SPipTangents) * m_RP.m_MaxVerts + 32;

		m_RP.m_RendIndices = (uint16 *)sAlign0x20(buf);
		m_RP.m_SysRendIndices = m_RP.m_RendIndices;
		buf += sizeof(uint16)*3*m_RP.m_MaxTris+32;
	}

  EF_Restore();

  EF_InitWaveTables();
  EF_InitD3DVertexDeclarations();
#if !defined(XENON) && !defined(PS3)
  EF_InitLightInfotable_DB();
#endif
  CHWShader_D3D::mfInit();

  //==================================================

  for (int i=0; i<2; i++)
  {
    for (int j=0; j<MAX_RECURSION_LEVELS; j++)
    {
      m_RP.m_DLights[i][j].Reserve(MAX_LIGHTS_NUM);
    }
  }
  if (!m_RP.m_TempObjects[0].Num())
    m_RP.m_TempObjects[0].Reserve(MAX_REND_OBJECTS);
#if !defined(STRIP_RENDER_THREAD)
  if (!m_RP.m_TempObjects[1].Num())
    m_RP.m_TempObjects[1].Reserve(MAX_REND_OBJECTS);
#endif
  if (!m_RP.m_Objects.Num())
  {
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_D3D, 0, "Renderer RenderObjects");

    m_RP.m_Objects.Reserve(MAX_REND_OBJECTS);
    CRenderObject::m_pPermObjData = (SRenderObjData *)CryModuleMemalign(sizeof(SRenderObjData)*MAX_PERM_OBJECTS, 16);
    memset(CRenderObject::m_pPermObjData, 0, sizeof(SRenderObjData)*MAX_PERM_OBJECTS);
    CRenderObject::m_sFreePermObjData.Alloc(MAX_PERM_OBJECTS);
    SAFE_DELETE_ARRAY(m_RP.m_ObjectsPool);
    m_RP.m_nNumObjectsInPool = 512;
    m_RP.m_ObjectsPool = new CRenderObject[m_RP.m_nNumObjectsInPool*2+1024];
    for (int j=0; j<2; j++)
    {
      for (uint32 i=0; i<m_RP.m_nNumObjectsInPool; i++)
      {
        m_RP.m_TempObjects[j][i] = &m_RP.m_ObjectsPool[j*m_RP.m_nNumObjectsInPool+i];
        m_RP.m_TempObjects[j][i]->Init(0);
        m_RP.m_TempObjects[j][i]->m_II.m_AmbColor = Col_White;
        m_RP.m_TempObjects[j][i]->m_ObjFlags = 0;
        m_RP.m_TempObjects[j][i]->m_II.m_Matrix.SetIdentity();
        m_RP.m_TempObjects[j][i]->m_RState = 0;
      }
    }
    // Permanent objects pool
    for (int i=0; i<1024; i++)
    {
      m_RP.m_Objects[i] = &m_RP.m_ObjectsPool[1024+i];
      m_RP.m_Objects[i]->Init(0);
      m_RP.m_Objects[i]->m_II.m_AmbColor = Col_White;
      m_RP.m_Objects[i]->m_ObjFlags = FOB_PERMANENT;
      m_RP.m_Objects[i]->m_II.m_Matrix.SetIdentity();
      m_RP.m_Objects[i]->m_RState = 0;
    }
    m_RP.m_Objects.SetUse(1);
  }
  m_RP.m_pCurObject = m_RP.m_Objects[0];
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;

  // create hdr element  
  m_RP.m_pREHDR = (CREHDRProcess *)EF_CreateRE(eDATA_HDRProcess);
  // create deferred shading element  
  m_RP.m_pREDeferredShading = (CREDeferredShading *)EF_CreateRE(eDATA_DeferredShading);

  //SDynTexture::CreateShadowPool();

  m_RP.m_fLastWaterFOVUpdate = 0;
  m_RP.m_LastWaterViewdirUpdate = Vec3(0, 0, 0);
  m_RP.m_LastWaterUpdirUpdate = Vec3(0, 0, 0);
  m_RP.m_LastWaterPosUpdate = Vec3(0, 0, 0);
  m_RP.m_fLastWaterUpdate = 0;
	m_RP.m_nLastWaterFrameID = 0;

	m_nPointState = CTexture::GetTexState(STexState(FILTER_POINT, true));
}

// Invalidate shaders pipeline
void CD3D9Renderer::EF_Invalidate()
{
  int j;
  for (j=0; j<MAX_DYNVBS; j++)
  {
    SAFE_DELETE (m_RP.m_VBs[j].VBPtr_0);
  }
  SAFE_DELETE(m_RP.m_IndexBuf);
  SAFE_DELETE(m_RP.m_VB_Inst);





}

// Restore shaders pipeline
void CD3D9Renderer::EF_Restore()
{
  int j;

  if (!m_RP.m_MaxTris)
    return;

  EF_Invalidate();

  m_RP.m_IndexBuf = new DynamicIB<uint16>(m_pd3dDevice, m_RP.m_MaxTris*3, false);
  m_RP.m_VB_Inst = new DynamicVB <vec4_t>(m_pd3dDevice, 0, MAX_HWINST_PARAMS, false);








  for (j=0; j<MAX_DYNVBS; j++)
  {
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_D3D, 0, "D3D Dynamic VertexBuffer");

    m_RP.m_VBs[j].VBPtr_5 = new DynamicVB <SVF_P3F_C4B_I4B_PS4F>(m_pd3dDevice, 0, m_RP.m_MaxVerts, false);
  }
}

// Shutdown shaders pipeline
void CD3D9Renderer::EF_PipelineShutdown()
{
  uint32 i, j;












  EF_Invalidate();

  SAFE_DELETE_ARRAY(m_RP.m_SysArray);  
  m_RP.m_SysVertexPool[0].Free();  
  m_RP.m_SysIndexPool[0].Free();  
#if !defined(STRIP_RENDER_THREAD)
  m_RP.m_SysVertexPool[1].Free();  
  m_RP.m_SysIndexPool[1].Free();  
#endif
  for (j=0; j<eVF_Max; j++)
  {
    m_RP.m_D3DVertexDeclaration[j].m_Declaration.Free();

	  for (i=0; i<1<<VSF_NUM; i++)
		{
      SAFE_RELEASE(m_RP.m_D3DVertexDeclarationCache[i][j][0].m_pDeclaration);
      SAFE_RELEASE(m_RP.m_D3DVertexDeclarationCache[i][j][1].m_pDeclaration);
		}
  }

  for (int n=0; n<2; n++)
  {
    for (j=0; j<2; j++)
    {
      for (i=0; i<CREClientPoly::m_PolysStorage[n][j].Num(); i++)
      {
        CREClientPoly::m_PolysStorage[n][j][i]->Release(true);
      }
      CREClientPoly::m_PolysStorage[n][j].Free();
    }
  }

  SafeReleaseParticleREs();

  CHWShader_D3D::ShutDown();
}

void CD3D9Renderer::FX_ResetPipe()
{
  int i;
  
  EF_SetState(GS_NODEPTHTEST);
  D3DSetCull(eCULL_None);
  EF_SelectTMU(0);
  m_RP.m_FlagsStreams_Decl = 0;
  m_RP.m_FlagsStreams_Stream = 0;
  m_RP.m_FlagsPerFlush = 0;
  m_RP.m_FlagsShader_RT = 0;
  m_RP.m_FlagsShader_MD = 0;
  m_RP.m_FlagsShader_MDV = 0;
  m_RP.m_FlagsShader_LT = 0;

  m_RP.m_nZOcclusionProcess = 0;
  m_RP.m_nZOcclusionReady = 1;

  CTexture::BindNULLFrom();

  HRESULT h = FX_SetIStream(NULL);
  EF_Scissor(false, 0, 0, 0, 0);
  m_RP.m_pShader = NULL;
  m_RP.m_pCurTechnique = NULL;
  m_RP.m_PrevLMask = -1;
  for (i=1; i<VSF_NUM; i++)
  {
    if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_USESTREAM<<i))
    {
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~(RBPF_USESTREAM<<i);
      h = FX_SetVStream(i, NULL, 0, 0, m_RP.m_ReqStreamFrequence[i]);
    }
  }
  CHWShader_D3D::mfSetGlobalParams();
}

void DrawFullScreenQuad(float fLeftU, float fTopV, float fRightU, float fBottomV);


//==========================================================================
// Calculate current scene node matrices
void CD3D9Renderer::EF_SetCameraInfo()
{
  m_pRT->RC_SetCamera();
}

void CD3D9Renderer::RT_SetCameraInfo()
{
  GetModelViewMatrix(&m_ViewMatrix(0,0));
  m_CameraMatrix = m_ViewMatrix;

  GetProjectionMatrix(&m_ProjMatrix(0,0));  

  if( m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_OBLIQUE_FRUSTUM_CLIPPING)
  {
    Matrix44A mObliqueProjMatrix;
    mObliqueProjMatrix.SetIdentity();

    mObliqueProjMatrix.m02 = m_RP.m_TI[m_RP.m_nProcessThreadID].m_pObliqueClipPlane.n[0]; 
    mObliqueProjMatrix.m12 = m_RP.m_TI[m_RP.m_nProcessThreadID].m_pObliqueClipPlane.n[1];
    mObliqueProjMatrix.m22 = m_RP.m_TI[m_RP.m_nProcessThreadID].m_pObliqueClipPlane.n[2];
    mObliqueProjMatrix.m32 = m_RP.m_TI[m_RP.m_nProcessThreadID].m_pObliqueClipPlane.d; 

    m_ProjMatrix.Multiply(m_ProjMatrix, mObliqueProjMatrix); 
  }

  if( CV_r_PostMSAA && SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]<=1 && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags&RBPF_SHADOWGEN))
  {
    static int nFrameID = -1;
    static Vec3 pOffset = Vec3(0,0,0);
    if( nFrameID != GetFrameID() )
    {
      Vec2 offs[2*2] =
      {
				//Vec2(cry_frand()*2.0f-1.0f,cry_frand()*2.0f-1.0f),
				//Vec2(cry_frand()*2.0f-1.0f,cry_frand()*2.0f-1.0f),
				//Vec2(cry_frand()*2.0f-1.0f,cry_frand()*2.0f-1.0f),
				//Vec2(cry_frand()*2.0f-1.0f,cry_frand()*2.0f-1.0f),
/*        Vec2(-0.5,-0.5),
				Vec2(0.5,0.5),
				Vec2(0.5,-0.5),
				Vec2(-0.5,0.5),		*/		              

        Vec2(0.96f,0.25f),
        Vec2(-0.25f,0.96f),
        Vec2(-0.96f,-0.25f),
        Vec2(0.25f,-0.96f),
      };

      int nCurrID = GetFrameID()%(CV_r_PostMSAA==1? 2 : 4); // select 2x msaa or 4x msaa
      pOffset.x = /*CV_r_PostMSAA * 0.01f **/ (offs[nCurrID].x / (float)GetWidth());
      pOffset.y = /*CV_r_PostMSAA * 0.01f **/ (offs[nCurrID].y / (float)GetHeight());
      nFrameID = GetFrameID();
    }

    m_ProjMatrix.m20 -=pOffset.x;
    m_ProjMatrix.m21 -=pOffset.y;
  }

  m_CameraProjMatrix.Multiply(m_CameraMatrix, m_ProjMatrix);
  m_CameraProjZeroMatrix.Multiply(m_CameraZeroMatrix[m_RP.m_nProcessThreadID], m_ProjMatrix);
  m_InvCameraProjMatrix.Invert(m_CameraProjMatrix);
  
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_WASWORLDSPACE;
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
  m_RP.m_ObjFlags = FOB_TRANS_MASK;
  m_RP.m_TransformFrame++;
  m_RP.m_FrameObject++;

  m_NewViewport.fMinZ = m_RP.m_TI[m_RP.m_nProcessThreadID].m_cam.GetZRangeMin();
  m_NewViewport.fMaxZ = m_RP.m_TI[m_RP.m_nProcessThreadID].m_cam.GetZRangeMax();
  m_bViewportDirty = true;

  CHWShader_D3D::mfSetCameraParams();
}

// Set object transform for fixed pipeline shader
void CD3D9Renderer::FX_SetObjectTransform(CRenderObject *obj, CShader *pSH, int nTransFlags)
{
  assert(m_pRT->IsRenderThread());

  if (nTransFlags & FOB_TRANS_MASK)
    m_ViewMatrix.Multiply(GetTransposed44(Matrix44A(obj->m_II.m_Matrix)), m_CameraMatrix);
  else
    m_ViewMatrix = m_CameraMatrix;
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadMatrix(&m_ViewMatrix);
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_MATRIXDIRTY;
}

// Set clip plane for the current scene
// on NVidia NV2X GPUs we use fake clip planes using texkill PS instruction : m_RP.m_ClipPlaneEnabled = 1
// on ATI hardware and NV30 and better we use native ClipPlanes : m_RP.m_ClipPlaneEnabled = 2
void CD3D9Renderer::FX_SetClipPlane (bool bEnable, float *pPlane, bool bRefract)
{
  if (!CV_d3d9_clipplanes)
    return;

  if (bEnable)
  {
#ifdef DO_RENDERLOG
    if (CV_r_log)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "Set clip-plane\n");
#endif
    if (m_RP.m_ClipPlaneEnabled)
      return;
    Plane p;
    p.n[0] = pPlane[0];
    p.n[1] = pPlane[1];
    p.n[2] = pPlane[2];
    p.d = pPlane[3];
    //if (bRefract)
    //p[3] += 0.1f;
    m_RP.m_bClipPlaneRefract = bRefract;
    m_RP.m_CurClipPlane.m_Normal.x = p.n[0];
    m_RP.m_CurClipPlane.m_Normal.y = p.n[1];
    m_RP.m_CurClipPlane.m_Normal.z = p.n[2];
    m_RP.m_CurClipPlane.m_Dist = p.d;
    m_RP.m_CurClipPlane.Init();

    m_RP.m_CurClipPlaneCull = m_RP.m_CurClipPlane;
    m_RP.m_CurClipPlaneCull.m_Dist = -m_RP.m_CurClipPlaneCull.m_Dist;
    int nGPU = m_Features & RFT_HW_MASK;

    m_RP.m_ClipPlaneEnabled = 2;

    // Since we use programmable pipeline only
    // always keep clip-plane in clip space
    Plane pTr = TransformPlane2(m_InvCameraProjMatrix, p);
        
#if defined (DIRECT3D9)
    m_pd3dDevice->SetClipPlane(0, &pTr.n[0]);
    m_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 1);
#elif defined (DIRECT3D10)
    assert(0);
#endif
  }
  else
  {
#ifdef DO_RENDERLOG
    if (CV_r_log)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "Reset clip-plane\n");
#endif
    m_RP.m_ClipPlaneEnabled = 0;

#if defined (DIRECT3D9)
    m_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
#elif defined (DIRECT3D10)
    assert(0);
#endif
  }
}



//==============================================================================
// Shader Pipeline
//=======================================================================

void CD3D9Renderer::EF_SetFogColor(ColorF &Color)
{
  int nThreadID = m_pRT->GetThreadList();

  ColorF pCol = Color;
  if (UseSkyLightBasedFog())
  {
    const SSkyLightRenderParams* pSkyLightRenderParams(GetSkyLightRenderParams());
    if(IsHDRModeEnabled())
    {
      pCol.r += min( pSkyLightRenderParams->m_hazeColor.x, 64.0f );
      pCol.g += min( pSkyLightRenderParams->m_hazeColor.y, 64.0f );
      pCol.b += min( pSkyLightRenderParams->m_hazeColor.z, 64.0f );
    }
    else
    {
      pCol.r += 1.0f - expf( -1 * pSkyLightRenderParams->m_hazeColor.x );
      pCol.g += 1.0f - expf( -1 * pSkyLightRenderParams->m_hazeColor.y );
      pCol.b += 1.0f - expf( -1 * pSkyLightRenderParams->m_hazeColor.z );
    }
  }

  m_RP.m_TI[nThreadID].m_FS.m_CurColor = pCol;
}

// Set current texture color op modes (used in fixed pipeline shaders)
void CD3D9Renderer::SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa)
{
  EF_SetColorOp(eCo, eAo, eCa, eAa);
}

void CD3D9Renderer::EF_SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa)
{
  int stage = CTexture::s_CurStage;
  int nThreadID = m_pRT->GetThreadList();

  if (eCo != 255 && m_RP.m_TI[nThreadID].m_eCurColorOp[stage] != eCo)
  {
    m_RP.m_TI[nThreadID].m_eCurColorOp[stage] = eCo;
    m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_FP_DIRTY;
  }
  if (eAo != 255 && m_RP.m_TI[nThreadID].m_eCurAlphaOp[stage] != eAo)
  {
    m_RP.m_TI[nThreadID].m_eCurAlphaOp[stage] = eAo;
    m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_FP_DIRTY;
  }
  if (eCa != 255 && m_RP.m_TI[nThreadID].m_eCurColorArg[stage] != eCa)
  {
    m_RP.m_TI[nThreadID].m_eCurColorArg[stage] = eCa;
    m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_FP_DIRTY;
  }
  if (eAa != 255 && m_RP.m_TI[nThreadID].m_eCurAlphaArg[stage] != eAa)
  {
    m_RP.m_TI[nThreadID].m_eCurAlphaArg[stage] = eAa;
    m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_FP_DIRTY;
  }
}

void CD3D9Renderer::FX_ScreenStretchRect( CTexture *pDst )
{
  if (CTexture::IsTextureExist(pDst))
  {
    int iTempX, iTempY, iWidth, iHeight;
    gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);












    // in hdr we render to HDRScene target - can just render to scene target instead of using resolve		
    if (CRenderer::CV_r_HDRRendering && CTexture::s_ptexSceneTarget && !(CTexture::s_ptexHDRTarget && CTexture::s_ptexHDRTarget->GetFlags() & FT_USAGE_FSAA && gRenDev->m_RP.m_FSAAData.Type)) 
    {
      using namespace NPostEffects;
      
      gcpRendD3D->Set2DMode(true, 1, 1);      

      SD3DSurface *pCurrDepthBuffer = (CTexture::s_ptexSceneTarget == pDst && gRenDev->m_RP.m_FSAAData.Type)?&gcpRendD3D->m_DepthBufferOrigFSAA : &gcpRendD3D->m_DepthBufferOrig;

      FX_PushRenderTarget(0, pDst, pCurrDepthBuffer); 
      RT_SetViewport(0, 0, pDst->GetWidth(), pDst->GetHeight());        

      static CCryName pTechName("TextureToTexture");                 
      SPostEffectsUtils::ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
      gRenDev->EF_SetState(GS_NODEPTHTEST);   

      SPostEffectsUtils::SetTexture(CTexture::s_ptexHDRTarget, 0, FILTER_POINT);    
      SPostEffectsUtils::DrawFullScreenQuad(pDst->GetWidth(), pDst->GetHeight());
      SPostEffectsUtils::ShEndPass();

      // Restore previous viewport
      FX_PopRenderTarget(0);
      RT_SetViewport(iTempX, iTempY, iWidth, iHeight);        
      Set2DMode(false, 1, 1);     
    }
    else
    {
      // update scene target before using it for water rendering
#if defined (DIRECT3D9)
      LPDIRECT3DSURFACE9 pBackSurface= gcpRendD3D->GetBackSurface();

      D3DSurface *pTexSurf = pDst->GetSurface(-1, 0);

      RECT pSrcRect={0, 0, iWidth, iHeight };
      RECT pDstRect={0, 0, pDst->GetWidth(), pDst->GetHeight() };

      CD3D9Renderer::SRTStack *pCurRT = m_pNewTarget[0];
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCopied++;
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCopiedSize += pDst->GetDeviceDataSize();
      m_pd3dDevice->StretchRect(pCurRT->m_pTarget, &pSrcRect, pTexSurf, &pDstRect, D3DTEXF_NONE); 

      SAFE_RELEASE(pTexSurf);
#elif defined (DIRECT3D10)
      CDeviceTexture *pDstResource = pDst->GetDevTexture();
      ID3D11RenderTargetView* pOrigRT = m_pNewTarget[0]->m_pTarget;



      ID3D11Resource *pSrcResource;

      D3D11_RENDER_TARGET_VIEW_DESC Desc;
      if (pOrigRT)
      {



        pOrigRT->GetResource(&pSrcResource);

        pOrigRT->GetDesc(&Desc);
        if (Desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS)
        {
          m_pd3dDeviceContext->ResolveSubresource(pDstResource->Get2DTexture(), 0, pSrcResource, 0, Desc.Format);
        }
        else
        {
          D3D11_BOX box;
          ZeroStruct(box);
          box.right = pDst->GetWidth();
          box.bottom = pDst->GetHeight();
          box.back = 1;

          m_pd3dDeviceContext->CopySubresourceRegion(pDstResource->Get2DTexture(), 0, 0, 0, 0, pSrcResource, 0, &box);
        }
        SAFE_RELEASE(pSrcResource);
      }
#endif

    }

  }
}


bool CD3D9Renderer::FX_ScatterScene(bool bEnable, bool ScatterPass)
{  
//////////////////////////////////////////////////////////////////////////
  // get list of shadow casters for nLightID
  /*CDLight* pLight( m_RP.m_DLights[m_RP.m_nProcessThreadID][ SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] ][ 0 ] );

  //check for valid light to process
  if (!pLight)
    return false;

  if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS ))
    return false;

  ShadowMapFrustum** ppSMFrustumList = pLight->m_pShadowMapFrustums;
  if (!ppSMFrustumList)
    return false;
  if (!ppSMFrustumList[0])
    return false;

  ConfigShadowTexgen(0,ppSMFrustumList[0]);*/

//////////////////////////////////////////////////////////////////////////

  if(bEnable)
  {
    //GetUtils().Log(" +++ Begin Scattering layer +++ \n"); 

    if (ScatterPass)
    {
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "\n   +++ Scattering layers - depth accumulation +++ \n"); 
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_SCATTERPASS;
      EF_SetState(m_RP.m_CurState | (GS_BLSRC_ONE | GS_BLDST_ONE)) ;

#if defined(DIRECT3D10)
      SStateBlend bl = m_StatesBL[m_nCurStateBL];
      //bmOut.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
      bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_REV_SUBTRACT;
      SetBlendState(&bl);
#else
      m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
#endif

    }
    else 
    {
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "\n   +++ Start Scattering depth layers +++ \n"); 
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_SCATTERPASS;

      EF_SetState(m_RP.m_CurState | (GS_BLSRC_ONE | GS_BLDST_ONE)) ;
#if defined(DIRECT3D10)
      SStateBlend bl = m_StatesBL[m_nCurStateBL];
      //bmOut.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
      bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
      SetBlendState(&bl);
#else
      m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
#endif

      //CTexture::s_ptexScatterLayer->Invalidate(GetWidth()/2, GetHeight()/2,eTF_A16B16G16R16F);  
      //int nWidth = CTexture::s_ptexScatterLayer->m_nWidth;
      //int nHeight = CTexture::s_ptexScatterLayer->m_nHeight;
      //SD3DSurface* pSepDepthSurf = FX_GetDepthSurface(nWidth, nHeight, false);

      //gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexScatterLayer, pSepDepthSurf);
      //gcpRendD3D->SetViewport(0, 0, nWidth, nHeight);
      //ColorF clearColor(0.0f, 0.0f, 0.0f, 0.0f);
      //gcpRendD3D->EF_ClearBuffers(FRT_CLEAR, &clearColor);
    }

  }
  else
  { 
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_SCATTERPASS;

      EF_SetState(m_RP.m_CurState | (GS_BLSRC_ONE | GS_BLDST_ONE)) ;
    //EF_SetState(m_RP.m_CurState & (~GS_BLEND_MASK)) ;
#if defined(DIRECT3D10)
    SStateBlend bl = m_StatesBL[m_nCurStateBL];
    //bl.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    SetBlendState(&bl);
#else
    m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
#endif

    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "\n   +++ End Scattering depth layers +++ \n"); 

    //gcpRendD3D->Set2DMode(true, 1, 1);

    //GetUtils().m_pCurDepthSurface = &gcpRendD3D->m_DepthBufferOrig;//FX_GetScreenDepthSurface();
    // Investigate: There's some issue with viewports, if copyScreenToTexture/copyTextureToTexture orders swap around, it doens't copy correctly
    //GetUtils().StretchRect(CTexture::m_Text_BackBuffer, CTexture::s_ptexScatterLayer);
    //GetUtils().TexBlurGaussian(CTexture::s_ptexScatterLayer, 1, 1.25f, 8.0f, true);  

    //gcpRendD3D->Set2DMode(false, 1, 1);     
    //gcpRendD3D->SetViewport(GetUtils().m_pScreenRect.left, GetUtils().m_pScreenRect.top, GetUtils().m_pScreenRect.right, GetUtils().m_pScreenRect.bottom); 
    //gcpRendD3D->SetViewport( 0, 0, GetWidth(), GetHeight() );
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_ZPrePassScene(bool bEnable, bool bClearZBuffer)
{




	if (bEnable)
	{
		if (m_LogFile)
			Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Start Z-prepass scene +++ \n");
		int nWidth = m_MainViewport.nWidth; //m_d3dsdBackBuffem.Width;
		int nHeight = m_MainViewport.nHeight; //m_d3dsdBackBuffem.Height;
		if (!CTexture::s_ptexZTarget
			|| CTexture::s_ptexZTarget->IsFSAAChanged()
			|| CTexture::s_ptexZTarget->GetDstFormat() != CTexture::s_eTFZ 
			|| CTexture::s_ptexZTarget->GetWidth() != nWidth
			|| CTexture::s_ptexZTarget->GetHeight() != nHeight)
			CTexture::GenerateZMaps();
		EF_SetState(GS_DEPTHWRITE);

		RT_SetViewport(0, 0, m_MainViewport.nWidth, m_MainViewport.nHeight);

		if (bClearZBuffer)
		{
			const float fDefaultPowFactor = 16.0f / 255.0f;
			const ColorF cDepthClear(0.0f, (m_RP.m_FSAAData.Type? 1.0f :  0.0f), 0.0f, (!CRenderer::CV_r_deferredshading)? 0.0f : fDefaultPowFactor);
			EF_ClearBuffers(FRT_CLEAR_DEPTH|FRT_CLEAR_STENCIL|FRT_CLEAR_IMMEDIATE, &cDepthClear);
		}

		m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_ZPASS;
		m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_DISABLECOLORWRITES|RBPF2_ZPREPASS;
		if (CTexture::s_eTFZ == eTF_R32F || CTexture::s_eTFZ == eTF_G16R16F || CTexture::s_eTFZ == eTF_A16B16G16R16F || CTexture::s_eTFZ ==eTF_DEPTH24)
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NOALPHABLEND|RBPF2_NOALPHATEST;
	}
	else
	if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_ZPASS)
	{
		if (m_LogFile)
			Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ End Z-prepass scene +++ \n");

	m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_ZPASS;
	m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~(RBPF2_DISABLECOLORWRITES|RBPF2_ZPREPASS);
	if (CTexture::s_eTFZ == eTF_G16R16F || CTexture::s_eTFZ == eTF_R32F || CTexture::s_eTFZ == eTF_A16B16G16R16F || CTexture::s_eTFZ ==eTF_DEPTH24)
		m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~(RBPF2_NOALPHABLEND | RBPF2_NOALPHATEST);

	}
	else
	{
		if (!CV_r_usezpass)
			CTexture::DestroyZMaps();
	}

		return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_ZScene(bool bEnable, bool bUseHDR, bool bClearZBuffer, bool bRenderNormalsOnly)
{




  
  if (bEnable)
  {
    if (m_LogFile)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Start Z scene +++ \n");
    int nWidth = m_MainViewport.nWidth; //m_d3dsdBackBuffem.Width;
    int nHeight = m_MainViewport.nHeight; //m_d3dsdBackBuffem.Height;
    if (!CTexture::s_ptexZTarget
      || CTexture::s_ptexZTarget->IsFSAAChanged()
      || CTexture::s_ptexZTarget->GetDstFormat() != CTexture::s_eTFZ 
      || CTexture::s_ptexZTarget->GetWidth() != nWidth
      || CTexture::s_ptexZTarget->GetHeight() != nHeight)
      CTexture::GenerateZMaps();

		bool bClearRT = false;
		bClearRT |= CV_r_wireframe != 0;
#if /*!defined(PS3) && */!defined(XENON)
		bClearRT |= !bRenderNormalsOnly;
#endif
		if(bClearRT)
			EF_ClearBuffers(FRT_CLEAR_COLOR, NULL);

    // Set float render target for Z frame buffer

		int nStates = GS_DEPTHWRITE;










    if( bRenderNormalsOnly && CRenderer::CV_r_deferredshading )
    {
      // Note: Current initial version will not work with FSAA      
      FX_PushRenderTarget(0, CTexture::s_ptexSceneNormalsMap, &m_DepthBufferOrigFSAA, false, -1, true);
			m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_DEFERRED_SHADING ];
    }
    else
    {
      FX_PushRenderTarget(0, CTexture::s_ptexZTarget, &m_DepthBufferOrigFSAA, false, -1, true);
      if( CRenderer::CV_r_deferredshading )      
      {
        // Note: Current initial version will not work with FSAA      
        FX_PushRenderTarget(1, CTexture::s_ptexSceneNormalsMap, NULL);
				m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_DEFERRED_SHADING ];
      }
    }


		EF_SetState(nStates);

    RT_SetViewport(0, 0, m_MainViewport.nWidth, m_MainViewport.nHeight);

    if (bClearZBuffer)
    {
      const float fDefaultPowFactor = 16.0f / 255.0f;
      const ColorF cDepthClear(0.0f, (m_RP.m_FSAAData.Type? 1.0f :  0.0f), 0.0f, (!CRenderer::CV_r_deferredshading)? 0.0f : fDefaultPowFactor);
			uint32 nClearFlags = FRT_CLEAR_IMMEDIATE;
			if( !CRenderer::CV_r_ZPrePass )
				nClearFlags |= FRT_CLEAR_DEPTH|FRT_CLEAR_STENCIL;
			EF_ClearBuffers(nClearFlags, &cDepthClear);
    }

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_ZPASS;
    if (CTexture::s_eTFZ == eTF_R32F || CTexture::s_eTFZ == eTF_G16R16F || CTexture::s_eTFZ == eTF_A16B16G16R16F || CTexture::s_eTFZ ==eTF_DEPTH24)
		{
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NOALPHABLEND;
#if !defined(XENON) && !defined(PS3)
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NOALPHATEST;


#endif
		}
  }
  else
  if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_ZPASS)
  {
    if (m_LogFile)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ End Z scene +++ \n");
    












    
    FX_PopRenderTarget(0);
		m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[ HWSR_DEFERRED_SHADING ];
		if( !bRenderNormalsOnly && CRenderer::CV_r_deferredshading )
			FX_PopRenderTarget(1); 


    CTexture::s_ptexZTarget->Resolve();
    //SetViewport(0, 0, GetWidth(), GetHeight());
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_ZPASS;
    if (CTexture::s_eTFZ == eTF_G16R16F || CTexture::s_eTFZ == eTF_R32F || CTexture::s_eTFZ == eTF_A16B16G16R16F)
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~(RBPF2_NOALPHABLEND | RBPF2_NOALPHATEST);
    if (m_RP.m_FSAAData.Type)
    {
      FX_Commit();
    #if defined (DIRECT3D9)
      m_pd3dDevice->EndScene();
      HRESULT hr = m_pd3dDevice->SetDepthStencilSurface(NULL);
      LPDIRECT3DSURFACE9 pDestSurf = (LPDIRECT3DSURFACE9)m_DepthBufferOrig.pSurf;
      LPDIRECT3DSURFACE9 pSrcSurf = (LPDIRECT3DSURFACE9)m_DepthBufferOrigFSAA.pSurf;
      int nWidth = m_d3dsdBackBuffer.Width;
      int nHeight = m_d3dsdBackBuffer.Height;

      RECT pSrcRect = {0, 0, nWidth, nHeight};
      RECT pDstRect = {0, 0, nWidth, nHeight};

      m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCopied++;
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCopiedSize += CTexture::s_ptexZTarget->GetDeviceDataSize();
      hr = m_pd3dDevice->StretchRect(pSrcSurf, &pSrcRect, pDestSurf, &pDstRect, D3DTEXF_NONE); 
      assert(hr == S_OK);
      m_pd3dDevice->BeginScene();
      hr = m_pd3dDevice->SetDepthStencilSurface(m_pNewTarget[0]->m_pDepth);
    #elif defined (DIRECT3D10)
      m_pd3dDeviceContext->OMSetRenderTargets(1, &m_pNewTarget[0]->m_pTarget, NULL);
      ID3D11Texture2D *pDestSurf = (ID3D11Texture2D *)m_DepthBufferOrig.pTex;
      ID3D11Texture2D *pSrcSurf = (ID3D11Texture2D *)m_DepthBufferOrigFSAA.pTex;

      //m_pd3dDeviceContext->ResolveSubresource(pDestSurf, 0, pSrcSurf, 0, DXUTGetCurrentDeviceSettings()->d3d10.AutoDepthStencilFormat); 

      FX_ResolveDepthTarget(CTexture::s_ptexZTarget, &m_DepthBufferOrig);

      //m_pd3dDeviceContext->OMSetRenderTargets(1, &m_pNewTarget[0]->m_pTarget, m_pNewTarget[0]->m_pDepth);
    #endif
    }



  }
  else
  {
    if (!CV_r_usezpass)
      CTexture::DestroyZMaps();
  }

  return true;
}

bool CD3D9Renderer::FX_FogScene()
{
  FX_ResetPipe();
  CTexture::BindNULLFrom();

  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Fog scene +++ \n");
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~(RBPF2_NOSHADERFOG);

  FX_SetVStream(3, NULL, 0, 0);

  if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_bEnable && CV_r_usezpass)
  {
    PROFILE_SHADER_START

		PROFILE_LABEL_PUSH( "FOG_GLOBAL" );

    m_pNewTarget[0]->m_ClearFlags = 0;
    RT_SetViewport(m_MainViewport.nX, m_MainViewport.nY, m_MainViewport.nWidth, m_MainViewport.nHeight);

    if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_HDR_FP16)
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_HDR_MODE];

    if (m_RP.m_FSAAData.Type)
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FSAA];
    if (UseSkyLightBasedFog())
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SKYLIGHT_BASED_FOG];

    CShader *pSH = CShaderMan::m_shHDRPostProcess;

    Vec3 vFarPlaneVerts[4];
    UnProjectFromScreen( m_MainViewport.nWidth, m_MainViewport.nHeight, 1, &vFarPlaneVerts[0].x, &vFarPlaneVerts[0].y, &vFarPlaneVerts[0].z);
    UnProjectFromScreen( 0,				m_MainViewport.nHeight, 1, &vFarPlaneVerts[1].x, &vFarPlaneVerts[1].y, &vFarPlaneVerts[1].z);
    UnProjectFromScreen( 0,							 0, 1, &vFarPlaneVerts[2].x, &vFarPlaneVerts[2].y, &vFarPlaneVerts[2].z);
    UnProjectFromScreen( m_MainViewport.nWidth,				 0, 1, &vFarPlaneVerts[3].x, &vFarPlaneVerts[3].y, &vFarPlaneVerts[3].z);

    Vec3 vRT = vFarPlaneVerts[0] - GetCamera().GetPosition();
    Vec3 vLT = vFarPlaneVerts[1] - GetCamera().GetPosition();
    Vec3 vLB = vFarPlaneVerts[2] - GetCamera().GetPosition();
    Vec3 vRB = vFarPlaneVerts[3] - GetCamera().GetPosition();

    //Vec3 vCoords[8];
    //gRenDev->GetRCamera().CalcVerts( vCoords);

    //Vec3 vRT = vCoords[4] - vCoords[0];
    //Vec3 vLT = vCoords[5] - vCoords[1];
    //Vec3 vLB = vCoords[6] - vCoords[2];
    //Vec3 vRB = vCoords[7] - vCoords[3];


    uint64 nFlagsShaderRTSave = m_RP.m_FlagsShader_RT;
		m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE2]);
		if (CV_r_FogDepthTest > 0)
			m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

    bool bUseFogDepthTest = (CV_r_FogDepthTest > 0);












    static CCryName TechName("FogPass");
    pSH->FXSetTechnique(TechName);

    uint32 nPasses;
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    pSH->FXBeginPass(0);

    STexState TexStatePoint = STexState(FILTER_POINT, true);
    CTexture::s_ptexZTarget->Apply(0, CTexture::GetTexState(TexStatePoint));





    if (bUseFogDepthTest)
		{
			float fogBlendStart = CV_r_FogDepthTest / GetCamera().GetFarPlane();
			float fogBlendEnd = fogBlendStart * 2.0f;
			float fogBlendDist = fogBlendEnd - fogBlendStart;

			float p0 = 1.0f / fogBlendDist;
			float p1 = -fogBlendStart / fogBlendDist;

			Vec4 fogDepthTestBlend(p0, p1, 0, 0);
			static CCryName fogDepthTestBlendParam("fogDepthTestBlend");
			pSH->FXSetPSFloat(fogDepthTestBlendParam, &fogDepthTestBlend, 1);
		}

    //////////////////////////////////////////////////////////////////////////
    //set world basis 
    //float maskRTWidth = float(m_MainViewport.nWidth);
    //float maskRTHeight = float(m_MainViewport.nHeight);
    //Vec4r vWBasisX, vWBasisY, vWBasisZ, vCamPos;
    //bool bVPosSM30 = true; //(GetFeatures() & (RFT_HW_PS30|RFT_HW_PS40))!=0;
    //Vec4 vParamValue, vMag;

    ////normalize
    ////vMag.x = vWBasisX.GetLength();
    ////vMag.y = vWBasisY.GetLength();
    ////vMag.z = vWBasisZ.GetLength();
    ////vMag.w = 1.0f;

    ////vWBasisX /= vWBasisX.GetLength();
    ////vWBasisY /= vWBasisY.GetLength();
    ////vWBasisZ /= vWBasisZ.GetLength();

    //Vec4 vWorldBasisX, vWorldBasisY, vWorldBasisZ;

    //CShadowUtils::ProjectScreenToWorldExpansionBasis(m_IdentityMatrix , GetCamera(), maskRTWidth, maskRTHeight, vWBasisX, vWBasisY, vWBasisZ, vCamPos, bVPosSM30, NULL);

    //vWorldBasisX = vWBasisX;
    //vWorldBasisY = vWBasisY;
    //vWorldBasisZ = vWBasisZ;
    //{
    //  static CCryName paramName("vWBasisX");
    //  pSH->FXSetPSFloat(paramName, &vWorldBasisX, 1);
    //}

    //{
    //  static CCryName paramName("vWBasisY");
    //  pSH->FXSetPSFloat(paramName, &vWorldBasisY, 1);
    //}

    //{
    //  static CCryName paramName("vWBasisZ");
    //  pSH->FXSetPSFloat(paramName, &vWorldBasisZ, 1);
    //} 

    ////{
    ////  static CCryName paramName("vBasisMagnitudes");
    ////  vParamValue = vMag;
    ////  pSH->FXSetPSFloat(paramName, &vParamValue, 1);
    ////}

    //////////////////////////////////////////////////////////////////////////

    int nOffs;
    SVF_P3F_T3F *Verts = (SVF_P3F_T3F *)GetVBPtr(4, nOffs, POOL_P3F_TEX3F);
    if (Verts)
    {





			const Matrix44A& projMat = *m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop();
			float clipZ = 0;
			if (bUseFogDepthTest)
			{
				// projMat.m23 is -1 or 1 depending on whether we use a RH or LH coord system
				// done in favor of an if check to make homogeneous divide by CV_r_FogDepthTest (which is always positive) work
				clipZ = projMat.m23 * CV_r_FogDepthTest * projMat.m22 + projMat.m32;
				clipZ /= CV_r_FogDepthTest;
				clipZ = clamp_tpl(clipZ, 0.f, 1.f);
			}

      Verts[0].p = Vec3(-1, -1, clipZ);
      Verts[0].st = vLB;

      Verts[1].p = Vec3(1, -1, clipZ);
      Verts[1].st = vRB;

      Verts[2].p = Vec3(-1, 1, clipZ);
      Verts[2].st = vLT;

      Verts[3].p = Vec3(1, 1, clipZ);
      Verts[3].st = vRT;




    }
    UnlockVB(POOL_P3F_TEX3F);

		static CCryName szHDRParams("HDRParams2");
		Vec4 v = Vec4(m_vSceneLuminanceInfo.x, m_fAdaptedSceneScale, 0, 0);
		pSH->FXSetPSFloat(szHDRParams, &v, 1);

    FX_Commit();

    uint32 nRS = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | (bUseFogDepthTest? GS_DEPTHFUNC_LEQUAL : GS_NODEPTHTEST);




    // Draw a fullscreen quad to sample the RT
		EF_SetState(nRS);
    D3DSetCull(eCULL_None);

    if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
    {
      FX_SetVStream(0, m_pVB[POOL_P3F_TEX3F], 0, sizeof(SVF_P3F_T3F));
  #if defined (DIRECT3D9)
      m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, 2);
  #elif defined (DIRECT3D10)
      SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      m_pd3dDeviceContext->Draw(4, nOffs);
  #endif
    }
    pSH->FXEndPass();

    //////////////////////////////////////////////////////////////////////////

    Vec3 lCol;
    gEnv->p3DEngine->GetGlobalParameter( E3DPARAM_SKY_HIGHLIGHT_COLOR, lCol );

    bool useFogPassWithLightning(lCol.x > 1e-4f || lCol.y > 1e-4f || lCol.z > 1e-4f);
    if (useFogPassWithLightning)
    {
      static CCryName TechNameAlt("FogPassWithLightning");
      if (pSH->FXSetTechnique(TechNameAlt))
      {
        pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        pSH->FXBeginPass(0);

        Vec3 lPos;
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_POS, lPos);
        Vec4 lightningPosition(lPos.x, lPos.y, lPos.z, 0.0f);
        static CCryName Param1Name("LightningPos");
        pSH->FXSetPSFloat(Param1Name, &lightningPosition, 1);

        Vec3 lSize;
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_SIZE, lSize);
        Vec4 lightningColorSize(lCol.x, lCol.y, lCol.z, lSize.x * 0.01f);
        static CCryName Param2Name("LightningColSize");
        pSH->FXSetPSFloat(Param2Name, &lightningColorSize, 1);

        FX_Commit();

        EF_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);

#if defined (DIRECT3D9)
        m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, 2);
#elif defined (DIRECT3D10)
        SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_pd3dDeviceContext->Draw(4, nOffs);
#endif
        pSH->FXEndPass();
      }
    }

    //////////////////////////////////////////////////////////////////////////

    m_RP.m_FlagsShader_RT = nFlagsShaderRTSave;

    EF_SelectTMU(0);

		PROFILE_LABEL_POP( "FOG_GLOBAL" );

		PROFILE_SHADER_END
  }

  // make sure to disable
  if (UseSkyLightBasedFog())
    m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SKYLIGHT_BASED_FOG];

  return true;
}

//================================================================================

#if !defined(XENON) && !defined(PS3)
void CD3D9Renderer::EF_InitLightInfotable_DB()
{
  int i, j;
  for (i=0; i<256; i++)
  {
    m_RP.m_LightInfo[i][0] = -1.0f;
    m_RP.m_LightInfo[i][1] = -1.0f;
    m_RP.m_LightInfo[i][3] = 1.0f / 64.0f;
    m_RP.m_LightInfo[i][2] = m_RP.m_LightInfo[i][3] * 4.0f;

    int nID[4];
    for (j=0; j<4; j++)
    {
      nID[j] = (i>>(2*j) & 3);
    }
    if (!nID[1] && !nID[2] && !nID[3])
    {
      m_RP.m_LightInfo[i][0] = nID[0] * m_RP.m_LightInfo[i][2];
      continue;
    }
    if (!nID[2] && !nID[3])
    {
      if (nID[1] == nID[0]+1)
        m_RP.m_LightInfo[i][0] = nID[0] * m_RP.m_LightInfo[i][2];
      else
      if (nID[1] == nID[0]+2)
      {
        m_RP.m_LightInfo[i][0] = nID[0] * m_RP.m_LightInfo[i][2];
        m_RP.m_LightInfo[i][2] *= 2;
      }
      else
      if (nID[1] == nID[0]+3)
      {
        m_RP.m_LightInfo[i][0] = nID[0] * m_RP.m_LightInfo[i][2];
        m_RP.m_LightInfo[i][2] *= 3;
      }
    }
    else
    if (!nID[3])
    {
      if (nID[1]==nID[0]+1 && nID[2]==nID[1]+1)
        m_RP.m_LightInfo[i][0] = nID[0] * m_RP.m_LightInfo[i][2];
      else
      if (nID[1]==nID[0]+2 && nID[2]==nID[1]+1)
        m_RP.m_LightInfo[i][0] = m_RP.m_LightInfo[i][2] * 4;
      else
      if (nID[1]==nID[0]+1 && nID[2]==nID[1]+2)
        m_RP.m_LightInfo[i][0] = m_RP.m_LightInfo[i][2] * 8;
    }
    else
    if (nID[1]==nID[0]+1 && nID[2]==nID[1]+1 && nID[3]==nID[2]+1)
      m_RP.m_LightInfo[i][0] = 0;
  }
}

bool CD3D9Renderer::FX_PrepareLightInfoTexture(bool bEnable)
{
  if (bEnable)
  {
    int nr = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1;
    if (!m_RP.m_DLights[m_RP.m_nProcessThreadID][nr].Num())
      return true;
    if (CV_r_shadersdynamicbranching == 4)
      return true;
    if (m_LogFile)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Prepare LightInfo texture +++ \n");
    ETEX_Format eTF = eTF_A32B32G32R32F;
    // 4 sets by 4 lights, 4 vectors per light
    int nWidth = 4*4*4;
    // 8 light groups (32 lights - maximum)
    int nHeight = 8;
    bool bGenerate = false;
    if (!CTexture::s_ptexLightInfo[nr-1]->GetDevTexture())
      bGenerate = true;
    if (CV_r_shadersdynamicbranching == 2 && !(CTexture::s_ptexLightInfo[nr-1]->GetFlags() & FT_USAGE_DYNAMIC))
      bGenerate = true;
    else
    if (!(CTexture::s_ptexLightInfo[nr-1]->GetFlags() & FT_USAGE_RENDERTARGET))
      bGenerate = true;
    if (bGenerate)
      CTexture::GenerateLightInfo(eTF, nWidth, nHeight);

    int i, j, n;
    static byte index[4][4] = 
    {
      {0,1,2,3}, {0,2,3,1}, {0,1,3,2}, {0,0,0,0}
    };

    Vec3 CameraPos = m_RP.m_TI[m_RP.m_nProcessThreadID].m_rcam.Orig;
    if (CV_r_shadersdynamicbranching == 2)
    {
      // Update light info on CPU
      int nPitch;
      Vec4 *pData = (Vec4 *)CTexture::s_ptexLightInfo[nr-1]->LockData(nPitch);
      assert(nPitch == nWidth*sizeof(Vec4));
      for (i=0; i<8; i++)
      {
        for (j=0; j<4; j++)
        {
          for (n=0; n<4; n++)
          {
            int nIdInGroup = index[j][n];
            uint32 nL = i*4+nIdInGroup;
            CDLight *dl = NULL;
            if (nL < m_RP.m_DLights[m_RP.m_nProcessThreadID][nr].Num() && (dl=&m_RP.m_DLights[m_RP.m_nProcessThreadID][nr][nL]) != NULL)
            {
              Vec3 vPos = dl->m_Origin - CameraPos;
              pData[0] = Vec4(vPos.x, vPos.y, vPos.z, 1.0f/dl->m_fRadius);
              pData[1] = dl->m_Color.toVec4();
              pData[2] = Vec4(0,0,0,0);
              float fType = 0;
              if (dl->m_Flags & DLF_POINT)
                fType = 1;
              else
              if (dl->m_Flags & DLF_PROJECT)
                fType = 2;
              pData[2].w = fType;
              pData[3] = Vec4(0,0,0,0);
              pData[3][nIdInGroup] = 1;
            }
            else
            {
#ifdef _DEBUG
              // Non-used light
              pData[0] = Vec4(0,0,0,0);
              pData[1] = Vec4(0,0,0,0);
              pData[2] = Vec4(0,0,0,0);
              pData[3] = Vec4(0,0,0,0);
#endif
            }
            pData += 4;
          }
        }
      }
      CTexture::s_ptexLightInfo[nr-1]->UnlockData();
    }
    else
    {
      // Update light info on GPU
      int iTempX, iTempY, iWidth, iHeight;
      GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
      RT_SetViewport(0, 0, nWidth, nHeight);

      // Update LightInfo
      FX_ResetPipe();

      FX_PushRenderTarget(0, CTexture::s_ptexLightInfo[nr-1], &m_DepthBufferOrig);
      ColorF Black(Col_Black);
      EF_ClearBuffers(FRT_CLEAR_COLOR, &Black);

      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~(RBPF2_NOSHADERFOG);
      CShader *pSH = CShaderMan::m_shHDRPostProcess;
      pSH->FXSetTechnique("LightInfo");
      uint32 nPasses;
      pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
      pSH->FXBeginPass(0);
      SShaderPass *pPass = m_RP.m_pCurPass;
      assert (pPass->m_VShader && pPass->m_PShader);
      if (pPass->m_VShader && pPass->m_PShader)
      {
        CHWShader_D3D *curVS = (CHWShader_D3D *)pPass->m_VShader;
        int nGroups = (m_RP.m_DLights[m_RP.m_nProcessThreadID][nr].Num()-1) / 4 + 1;
        int nPoints = nGroups*4*4*4;
        int nOffs;
        SVF_TP3F_T2F_T3F *Verts = (SVF_TP3F_T2F_T3F *)GetVBPtr(nPoints, nOffs, POOL_TRP3F_TEX2F_TEX3F);
        SVF_TP3F_T2F_T3F *OrigVerts = Verts;

        Vec4 vData;
        if (Verts)
        {
          for (i=0; i<8; i++)
          {
            if (i >= nGroups)
              break;
            for (j=0; j<4; j++)
            {
              for (n=0; n<4; n++)
              {
                int nIdInGroup = index[j][n];
                uint32 nL = i*4+nIdInGroup;
                CDLight *dl = NULL;
                Vec4 v = Vec4((float)((j*4+n)*4),(float)i,1,1);
                if (nL < m_RP.m_DLights[m_RP.m_nProcessThreadID][nr].Num() && (dl=&m_RP.m_DLights[m_RP.m_nProcessThreadID][nr][nL]) != NULL)
                {
                  Verts[0].p = v; v.x += 1;
                  Vec3 vPos;
                  if (dl->m_Flags & DLF_DIRECTIONAL)
                    vPos = gEnv->p3DEngine->GetSunDirNormalized();
                  else
                    vPos = dl->m_Origin - CameraPos;
                  Verts[0].st0 = Vec2(vPos.x, vPos.y); Verts[0].st1 = Vec3(vPos.z, 1.0f/dl->m_fRadius, 0);

                  Verts[1].p = v; v.x += 1;
                  vData = dl->m_Color.toVec4();
                  Verts[1].st0 = Vec2(vData.x, vData.y); Verts[1].st1 = Vec3(vData.z, vData.w, 0);
                  vData = Vec4(0,0,0,0);
                  float fType = 0;
                  if (dl->m_Flags & DLF_POINT)
                    fType = 1;
                  else
                  if (dl->m_Flags & DLF_PROJECT)
                    fType = 2;
                  vData.w = fType;

                  Verts[2].p = v; v.x += 1;
                  Verts[2].st0 = Vec2(vData.x, vData.y); Verts[2].st1 = Vec3(vData.z, vData.w, 0);

                  Verts[3].p = v; v.x += 1;
                  vData = Vec4(0, 0, 0, 0);
                  vData[nIdInGroup] = 1;
                  Verts[3].st0 = Vec2(vData.x, vData.y); Verts[3].st1 = Vec3(vData.z, vData.w, 0);
                }
                else
                {
                  // Non-used light
                  Verts[0].p = v; v.x += 1;
                  Verts[1].p = v; v.x += 1;
                  Verts[2].p = v; v.x += 1;
                  Verts[3].p = v; v.x += 1;
                }
                Verts += 4;
              }
            }
          }

          UnlockVB(POOL_TRP3F_TEX2F_TEX3F);

          FX_Commit();

          // Draw a fullscreen quad to sample the RT
          EF_SetState(GS_NODEPTHTEST);

          if (!FAILED(FX_SetVertexDeclaration(0, eVF_TP3F_T2F_T3F)))
          {
            FX_SetVStream(0, m_pVB[POOL_TRP3F_TEX2F_TEX3F], 0, sizeof(SVF_TP3F_T2F_T3F));
  #if defined (DIRECT3D9)
            HRESULT hr = m_pd3dDevice->DrawPrimitive(D3DPT_POINTLIST, nOffs, nPoints);
  #elif defined (DIRECT3D10)
            assert(0);
  #endif
          }
          pSH->FXEndPass();

          EF_SelectTMU(0);
        }
        pSH->FXEnd();
      }
      FX_PopRenderTarget(0);
      RT_SetViewport(iTempX, iTempY, iWidth, iHeight);
    }
  }
  else
    CTexture::DestroyLightInfo();
  return true;
}
#endif

bool CD3D9Renderer::FX_HDRScene(bool bEnableHDR, bool bClear, bool bEncoded, bool bDecodePass)
{
#ifdef USE_HDR
   if (bEnableHDR)
  {
    if (m_LogFile)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], (!bEncoded)? " +++ Start HDR scene +++ \n" : " +++ Start HDR encoded scene +++ \n");

    if (!CTexture::s_ptexHDRTarget || CTexture::s_ptexHDRTarget->IsFSAAChanged() || CTexture::s_ptexHDRTarget->GetWidth() != GetWidth() || CTexture::s_ptexHDRTarget->GetHeight() != GetHeight())
      CTexture::GenerateHDRMaps();

    bool bEmpty = SRendItem::IsListEmpty(EFSLIST_HDRPOSTPROCESS, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
    if( bEmpty )
      return false;

    // Restore backbuffer
    if( bDecodePass && !bEncoded )
      FX_PopRenderTarget(0);

		if( !bDecodePass )
			FX_HDRRangeAdaptUpdate();

    // Set float render target for HDR frame buffer
    CTexture *ptexHDRTarget = (!bEncoded)? CTexture::s_ptexHDRTarget : CTexture::s_ptexHDRTargetEncoded;
    FX_PushRenderTarget(0, ptexHDRTarget, &m_DepthBufferOrigFSAA, false, -1, true);
    
    if( bClear )
      EF_ClearBuffers(FRT_CLEAR, NULL);

    // Restore encoded into main HDR target
    if( 0 && bDecodePass && !bEncoded )
    {
      PROFILE_LABEL_PUSH( "DECODE_INTO_HDRTARGET" );
      gcpRendD3D->Set2DMode(true, 1, 1);

      uint32 nPasses = 0;
      static CCryName TechName("DecodeIntoHDRTarget");
      CShaderMan::m_shHDRPostProcess->FXSetTechnique(TechName);
      CShaderMan::m_shHDRPostProcess->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

      CShaderMan::m_shHDRPostProcess->FXBeginPass(0);

      SPostEffectsUtils::SetTexture(CTexture::s_ptexHDRTargetEncoded, 0, FILTER_POINT);
      SPostEffectsUtils::DrawFullScreenQuad( CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight() );

      CShaderMan::m_shHDRPostProcess->FXEndPass();
      CShaderMan::m_shHDRPostProcess->FXEnd();  

      gcpRendD3D->Set2DMode(false, 1, 1);
      PROFILE_LABEL_POP( "DECODE_INTO_HDRTARGET" );
    }

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_HDR;
  }
  else
  if (!CV_r_HDRRendering && CTexture::s_ptexHDRTarget)
  {
    if (m_LogFile)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ End HDR scene +++ \n");
    CTexture::DestroyHDRMaps();
  }
  return true;
#endif
  return false;
}

// Draw overlay geometry in wireframe mode
void CD3D9Renderer::FX_DrawWire()
{
  float fColor = 1.f;
  int nState = GS_WIREFRAME;

  if (CV_r_showlines == 1)
    nState |= GS_NODEPTHTEST;

  if (CV_r_showlines == 3)
  {
    if(!gcpRendD3D->m_RP.m_pRE || !gcpRendD3D->m_RP.m_pRE->m_CustomData)
      return; // draw only terrain
    nState |= GS_BLSRC_DSTCOL | GS_BLDST_ONE;
    fColor = .5f;
  }

  gcpRendD3D->EF_SetState(nState);
  gcpRendD3D->SetMaterialColor(fColor,fColor,fColor,1.f);
  CTexture::s_ptexWhite->Apply();
  gcpRendD3D->EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, (eCA_Texture|(eCA_Constant<<3)), (eCA_Texture|(eCA_Constant<<3)));
  CRenderObject *pObj = gcpRendD3D->m_RP.m_pCurObject;
  gcpRendD3D->FX_SetFPMode();
  gcpRendD3D->m_RP.m_pCurObject = pObj;
  gcpRendD3D->m_RP.m_pCurInstanceInfo = &gcpRendD3D->m_RP.m_pCurObject->m_II;
  if (gcpRendD3D->m_RP.m_pRE)
    gcpRendD3D->m_RP.m_pRE->mfCheckUpdate(gcpRendD3D->m_RP.m_pShader->m_eVertexFormat, 0);

  uint32 i;
  if (gcpRendD3D->m_RP.m_pCurPass)
  {
    CHWShader_D3D *curVS = (CHWShader_D3D *)gcpRendD3D->m_RP.m_pCurPass->m_VShader;
    for (i=0; i<gcpRendD3D->m_RP.m_RIs.Num(); i++)
    {
      SRendItem *pRI = gcpRendD3D->m_RP.m_RIs[i];
      gcpRendD3D->FX_SetObjectTransform(pRI->pObj, NULL, pRI->pObj->m_ObjFlags);
      curVS->mfSetParametersPI(NULL, gcpRendD3D->m_RP.m_pShader);
      gcpRendD3D->FX_Commit();
      gcpRendD3D->FX_DrawBatch(gcpRendD3D->m_RP.m_pShader, NULL);
    }
  }
}

// Draw geometry normal vectors
void CD3D9Renderer::FX_DrawNormals()
{
  HRESULT h = S_OK;

  float len = CRenderer::CV_r_normalslength;
  int StrVrt, StrNrm;
  //if (gcpRendD3D->m_RP.m_pRE)
  //  gcpRendD3D->m_RP.m_pRE->mfCheckUpdate(gcpRendD3D->m_RP.m_pShader->m_VertexFormatId, SHPF_NORMALS);
  byte *verts = (byte *)gcpRendD3D->EF_GetPointer(eSrcPointer_Vert, &StrVrt, eType_FLOAT, eSrcPointer_Vert, FGP_SRC | FGP_REAL);
  byte *norms = (byte *)gcpRendD3D->EF_GetPointer(eSrcPointer_Normal, &StrNrm, eType_FLOAT, eSrcPointer_Normal, FGP_SRC | FGP_REAL);
  if ((INT_PTR)norms > 256 && (INT_PTR)verts > 256)
  {
    gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);
    gcpRendD3D->EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, (eCA_Diffuse|(eCA_Diffuse<<3)), (eCA_Diffuse|(eCA_Diffuse<<3)));
    gcpRendD3D->FX_SetFPMode();
    //gcpRendD3D->m_pd3dDevice->SetVertexShader(NULL);

    int numVerts = gcpRendD3D->m_RP.m_RendNumVerts;

    gcpRendD3D->EF_SetState(0);
    SVF_P3F_C4B_T2F *Verts = new SVF_P3F_C4B_T2F[numVerts*2];

    uint32 col0 = 0x000000ff;
    uint32 col1 = 0x00ffffff;

    for (int v=0; v<numVerts*2; v+=2,verts+=StrVrt,norms+=StrNrm)
    {
      float *fverts = (float *)verts;
      Vec3 vNorm = Vec3((norms[0]-128.0f)/127.5f, (norms[1]-128.0f)/127.5f, (norms[2]-128.0f)/127.5f);

      Verts[v].xyz = Vec3(fverts);
      Verts[v].color.dcolor = col0;

      Verts[v+1].xyz = Vec3(fverts[0] + vNorm[0]*len, fverts[1] + vNorm[1]*len, fverts[2] + vNorm[2]*len);
      Verts[v+1].color.dcolor = col1;
    }
    uint32 i;
    if (gcpRendD3D->m_RP.m_pCurPass)
    {
      CHWShader_D3D *curVS = (CHWShader_D3D *)gcpRendD3D->m_RP.m_pCurPass->m_VShader;
      for (i=0; i<gcpRendD3D->m_RP.m_RIs.Num(); i++)
      {
        SRendItem *pRI = gcpRendD3D->m_RP.m_RIs[i];
        gcpRendD3D->FX_SetObjectTransform(pRI->pObj, NULL, pRI->pObj->m_ObjFlags);
        curVS->mfSetParametersPI(NULL, gcpRendD3D->m_RP.m_pShader);
        gcpRendD3D->FX_Commit();

#if defined (DIRECT3D9)
        h = gcpRendD3D->m_pd3dDevice->DrawPrimitiveUP(D3DPT_LINELIST, numVerts, Verts, sizeof(SVF_P3F_C4B_T2F));
#elif defined (DIRECT3D10)
        assert(0);
#endif
      }
    }

    delete [] Verts;
    gcpRendD3D->m_RP.m_VertexStreams[0].pStream = NULL;
  }
}

// Draw geometry tangent vectors
void CD3D9Renderer::FX_DrawTangents()
{
  HRESULT h = S_OK;

  float len = CRenderer::CV_r_normalslength;
  //if (gcpRendD3D->m_RP.m_pRE)
  //  gcpRendD3D->m_RP.m_pRE->mfCheckUpdate(gcpRendD3D->m_RP.m_pShader->m_VertexFormatId, SHPF_TANGENTS);
  int StrVrt, StrTang, StrBinorm;
  byte *verts = NULL;
  byte *tangs = NULL;
  byte *binorm = NULL;
  int flags = 0;
  if (CRenderer::CV_r_showtangents == 1)
    flags = FGP_SRC | FGP_REAL;
  else
    flags = FGP_REAL;
  verts = (byte *)gcpRendD3D->EF_GetPointer(eSrcPointer_Vert, &StrVrt, eType_FLOAT, eSrcPointer_Vert, flags);
  tangs = (byte *)gcpRendD3D->EF_GetPointer(eSrcPointer_Tangent, &StrTang, eType_BYTE, eSrcPointer_Tangent, flags);
  binorm = (byte *)gcpRendD3D->EF_GetPointer(eSrcPointer_Binormal, &StrBinorm, eType_BYTE, eSrcPointer_Binormal, flags);
  if ((INT_PTR)tangs>256 && (INT_PTR)binorm>256)
  {
    int numVerts = gcpRendD3D->m_RP.m_RendNumVerts;

    CTexture::s_ptexWhite->Apply();
    gcpRendD3D->EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, (eCA_Diffuse|(eCA_Diffuse<<3)), (eCA_Diffuse|(eCA_Diffuse<<3)));
    if (gcpRendD3D->m_wireframe_mode == R_SOLID_MODE)
      gcpRendD3D->EF_SetState(GS_DEPTHWRITE);
    else
      gcpRendD3D->EF_SetState(0);
    gcpRendD3D->FX_SetFPMode();
    gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);
    SVF_P3F_C4B_T2F *Verts = new SVF_P3F_C4B_T2F[numVerts*6];

    for (int v=0; v<numVerts; v++,verts+=StrVrt,tangs+=StrTang, binorm+=StrBinorm)
    {
      uint32 col0 = 0xffff0000;
      uint32 col1 = 0xffffffff;
      float *fverts = (float *)verts;
      int16f *fv = (int16f *)tangs;
      Vec3 vTang = Vec3(tPackB2F(fv[0]), tPackB2F(fv[1]), tPackB2F(fv[2]));
      Verts[v*6+0].xyz = Vec3(fverts);
      Verts[v*6+0].color.dcolor = col0;

      Verts[v*6+1].xyz = Vec3(fverts[0] + vTang[0]*len, fverts[1] + vTang[1]*len, fverts[2] + vTang[2]*len);
      Verts[v*6+1].color.dcolor = col1;

      col0 = 0x0000ff00;
      col1 = 0x00ffffff;
      fverts = (float *)verts;
      int16f *fv1 = (int16f *)binorm;
      Vec3 vBinorm = Vec3(tPackB2F(fv1[0]), tPackB2F(fv1[1]), tPackB2F(fv1[2]));

      Verts[v*6+2].xyz = Vec3(fverts);
      Verts[v*6+2].color.dcolor = col0;

      Verts[v*6+3].xyz = Vec3(fverts[0] + vBinorm[0]*len, fverts[1] + vBinorm[1]*len, fverts[2] + vBinorm[2]*len);
      Verts[v*6+3].color.dcolor = col1;

      col0 = 0x000000ff;
      col1 = 0x00ffffff;
      fverts = (float *)verts;
      Vec3 vTNorm = (vTang ^ vBinorm) * tPackB2F(fv[3]);

      Verts[v*6+4].xyz = Vec3(fverts);
      Verts[v*6+4].color.dcolor = col0;

      Verts[v*6+5].xyz = Vec3(fverts[0] + vTNorm[0]*len, fverts[1] + vTNorm[1]*len, fverts[2] + vTNorm[2]*len);
      Verts[v*6+5].color.dcolor = col1;
    }
    uint32 i;
    if (gcpRendD3D->m_RP.m_pCurPass)
    {
      CHWShader_D3D *curVS = (CHWShader_D3D *)gcpRendD3D->m_RP.m_pCurPass->m_VShader;
      for (i=0; i<gcpRendD3D->m_RP.m_RIs.Num(); i++)
      {
        SRendItem *pRI = gcpRendD3D->m_RP.m_RIs[i];
        gcpRendD3D->FX_SetObjectTransform(pRI->pObj, NULL, pRI->pObj->m_ObjFlags);
        curVS->mfSetParametersPI(NULL, gcpRendD3D->m_RP.m_pShader);
        gcpRendD3D->FX_Commit();

  #if defined (DIRECT3D9)
        h = gcpRendD3D->m_pd3dDevice->DrawPrimitiveUP(D3DPT_LINELIST, numVerts*3, Verts, sizeof(SVF_P3F_C4B_T2F));
  #elif defined (DIRECT3D10)
        assert(0);
  #endif
      }
    }
    delete [] Verts;
    gcpRendD3D->m_RP.m_VertexStreams[0].pStream = NULL;
  }
}

// Draw light sources in debug mode
void CD3D9Renderer::EF_DrawDebugLights()
{
  static int sFrame = 0;
  ResetToDefault();
  GetIRenderAuxGeom()->SetRenderFlags(e_DepthTestOff | e_DepthWriteOff | e_CullModeNone | e_FillModeSolid | e_AlphaBlended);
  if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID != sFrame)
  {
    uint32 i;
    sFrame = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;

    CTexture::s_ptexWhite->Apply();

    for (i=0; i<m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1].Num(); i++)
    {
      CDLight *dl = &m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][i];
      if (!dl)
        continue;
      ColorF colF = dl->m_Color;
      colF.NormalizeCol(colF);
      ColorB col = ColorB(colF);
      col.a = 0x3f;
      SetMaterialColor(dl->m_Color[0], dl->m_Color[1], dl->m_Color[2], dl->m_Color[3]);
      if (dl->m_Flags & DLF_DIRECTIONAL)
        GetIRenderAuxGeom()->DrawSphere(dl->m_Origin, 0.02f, col);
      else
      if (dl->m_Flags & DLF_POINT)
        GetIRenderAuxGeom()->DrawSphere(dl->m_Origin, 0.05f, col);
      if (dl->m_Flags & DLF_PROJECT)
      {
        Vec3 dir, rgt, org;
        EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
        FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);

        dir = dl->m_ObjMatrix.GetColumn2(); // fro
        rgt = dl->m_ObjMatrix.GetColumn1();

        float ang = dl->m_fLightFrustumAngle;          
        if (ang == 0)
          ang = 45.0f;
        org = dl->m_Origin;

        dir *= 0.3f;

        ColorF Col = dl->m_Color;

        Matrix44 m;
        Vec3 vertex = dir;

        vertex = Matrix33::CreateRotationAA(DEG2RAD(ang),rgt.GetNormalized()) * vertex; //NOTE: angle need to be in radians
        Matrix44 mat = Matrix33::CreateRotationAA(DEG2RAD(60),dir.GetNormalized()); //NOTE: angle need to be in radians
        Vec3 tmpvertex;
        int ctr;

        //fill the inside of the light
        EnableTMU(false);
        SVF_P3F_C4B_T2F Verts[32];
        memset(Verts, 0, sizeof(Verts));
        ColorF cl = Col*0.3f;
        int n = 0;
        Verts[n].xyz = org;
        Verts[n].color.dcolor = D3DRGBA(cl[0], cl[1], cl[2], 1.0f);
        n++;
        tmpvertex = org + vertex;
        Verts[n].xyz = tmpvertex;
        Verts[n].color.dcolor = D3DRGBA(Col[0], Col[1], Col[2], 1.0f);
        n++;
        for (ctr=0; ctr<6; ctr++)
        {
          vertex = mat.TransformVector(vertex);
          tmpvertex = org + vertex;
          Verts[n].xyz = tmpvertex;
          Verts[n].color.dcolor = D3DRGBA(Col[0], Col[1], Col[2], 1.0f);
          n++;
        }
#if defined (DIRECT3D9)
        m_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n-2, Verts, sizeof(SVF_P3F_C4B_T2F));
        m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
#elif defined (DIRECT3D10)
        assert(0);
#endif

        //draw the inside of the light with lines and the outside filled
        SetCullMode(R_CULL_NONE);
        n = 0;
        Verts[n].xyz = org;
        Verts[n].color.dcolor = D3DRGBA(0.3f, 0.3f, 0.3f, 1.0f);
        n++;
        tmpvertex = org + vertex;
        Verts[n].xyz = tmpvertex;
        Verts[n].color.dcolor = D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
        n++;
        for (ctr=0; ctr<6; ctr++)
        {
          vertex = mat.TransformVector(vertex);
          tmpvertex = org + vertex;
          Verts[n].xyz = tmpvertex;
          Verts[n].color.dcolor = D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
          n++;
        }
#if defined (DIRECT3D9)
        m_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n-2, Verts, sizeof(SVF_P3F_C4B_T2F));
        m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
#elif defined (DIRECT3D10)
        assert(0);
#endif
        SetCullMode(R_CULL_FRONT);

        //set the color to the color of the light
        Verts[0].xyz = org;
        Verts[0].color.dcolor = D3DRGBA(Col[0], Col[1], Col[2], 1.0f);

        //draw a point at the origin of the light
#if defined (DIRECT3D9)
        m_pd3dDevice->DrawPrimitiveUP(D3DPT_POINTLIST, 1, Verts, sizeof(SVF_P3F_C4B_T2F));
#elif defined (DIRECT3D10)
        assert(0);
#endif
        //draw a line in the center of the light
        Verts[0].xyz = org;
        Verts[0].color.dcolor = D3DRGBA(Col[0], Col[1], Col[2], 1.0f);

        tmpvertex = org + dir;
        Verts[1].xyz = tmpvertex;
        Verts[1].color.dcolor = D3DRGBA(Col[0], Col[1], Col[2], 1.0f);
#if defined (DIRECT3D9)
        m_pd3dDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, Verts, sizeof(SVF_P3F_C4B_T2F));
#elif defined (DIRECT3D10)
        assert(0);
#endif
        EnableTMU(true);
      }
							
			if (CV_r_debuglights >= 2 && !(dl->m_Flags & DLF_DIRECTIONAL))
      {
        EF_SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
        SetCullMode(R_CULL_NONE);
        SetMaterialColor(dl->m_Color[0], dl->m_Color[1], dl->m_Color[2], 0.25f);
        GetIRenderAuxGeom()->DrawSphere(dl->m_Origin, dl->m_fRadius, col);

        if(CV_r_debuglights >= 3)
          DrawLabel(dl->m_Origin, 1.5f, "(%.2f %.2f %.2f)\n(%.2f %.2f %.2f)\nHDRDyn:%.2f S:%.2f F:%x Sty:%d\nName:%s DistRatio:%d",
          dl->m_BaseColor[0], dl->m_BaseColor[1], dl->m_BaseColor[2],
          dl->m_Color[0], dl->m_Color[1], dl->m_Color[2],
          dl->m_fHDRDynamic,dl->m_SpecMult,dl->m_Flags,dl->m_nLightStyle,
          dl->m_sName ? dl->m_sName : "Unknown",
          dl->m_pOwner ? dl->m_pOwner->GetViewDistRatio() : -1);
      }
    }
  }
  gcpRendD3D->m_RP.m_VertexStreams[0].pStream = NULL;
}

// Draw debug geometry/info
void CD3D9Renderer::EF_DrawDebugTools(SViewport& VP)
{
  if (CV_r_showlines)
    EF_ProcessRenderLists(FX_DrawWire, 0, VP);

  if (CV_r_shownormals)
    EF_ProcessRenderLists(FX_DrawNormals, 0, VP);

  if (CV_r_showtangents)
    EF_ProcessRenderLists(FX_DrawTangents, 0, VP);

	if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]==1 && CV_r_debuglights)
    EF_DrawDebugLights();
}

//#include "FMallocWindows.h"

/*struct vertex
{
  Vec3 position;
  Vec3 normal;
  Vec4 color;
};
vertex mesh1[1000];
vertex mesh2[1000];

__declspec(noinline) 
void rotate_mesh(const vertex *from, vertex *to, int size, Matrix33 m, Vec3 offset)
{
  for (int i=0; i<size; ++i)
  {
    Vec3 pos = from[i].position;
    to[i].position = pos.x * m.GetRow(0) + pos.y * m.GetRow(1) + pos.z * m.GetRow(2) + offset;			
    Vec3 normal = from[i].normal;
    to[i].position = normamOut.x * m.GetRow(0) + normamOut.y * m.GetRow(1) + normamOut.z * m.GetRow(2);
    to[i].color = from[i].color;
  }
}*/

static int __cdecl TimeProfCallback( const VOID* arg1, const VOID* arg2 )
{
  SProfInfo *pi1 = (SProfInfo *)arg1;
  SProfInfo *pi2 = (SProfInfo *)arg2;
  if (pi1->pTechnique->m_fProfileTime > pi2->pTechnique->m_fProfileTime)
    return -1;
  if (pi1->pTechnique->m_fProfileTime < pi2->pTechnique->m_fProfileTime)
    return 1;
  return 0;
}

static int __cdecl Compare_SProfInfo( const VOID* arg1, const VOID* arg2 )
{
  SProfInfo *pi1 = (SProfInfo *)arg1;
  SProfInfo *pi2 = (SProfInfo *)arg2;

  if(gRenDev->CV_r_ProfileShadersGroupByName == 1)
  {
    char str1[128];
    char str2[128];

    sprintf_s(str1, sizeof(str1), "%s.%s", pi1->pShader->GetName(), pi1->pTechnique->m_Name.c_str());
    sprintf_s(str2, sizeof(str2), "%s.%s", pi2->pShader->GetName(), pi2->pTechnique->m_Name.c_str());

    return stricmp(str1,str2);
  }
  else if(gRenDev->CV_r_ProfileShadersGroupByName == 2)
  {
    return stricmp(pi1->pTechnique->m_Name.c_str(),pi2->pTechnique->m_Name.c_str());
  }

  if (pi1->pTechnique > pi2->pTechnique)
    return -1;
  if (pi1->pTechnique < pi2->pTechnique)
    return 1;

  return 0;
}

struct STimeStorage
{
  float fNumPolys;
  float fNumDips;
  double fTime;
  float fItems;
  uint32 nUsedFrameId;
  STimeStorage()
  {
    fNumPolys = 0;
    fNumDips = 0;
    fTime = 0;
    fItems = 0;
    nUsedFrameId = 0;
  }
};


// Print shaders profile info on the screen
void CD3D9Renderer::EF_PrintProfileInfo()
{ 
  TextToScreenColor(1,14, 0,2,0,1, "Instances: %d, Batches: %d, DrawCalls: %d, Text: %d, Stat: %d, PShad: %d, VShad: %d",  m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendInstances, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendBatches, GetCurrentNumberOfDrawCalls(), m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumTextChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumStateChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumPShadChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumVShadChanges);
  TextToScreenColor(1,17, 0,2,0,1, "VShad: %d, PShad: %d, Text: %d",  m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumVShaders, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumPShaders, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumTextures);
  TextToScreenColor(1,20, 0,2,0,1, "Preprocess: %8.02f ms, OccmOut. queries: %8.02f ms",  m_RP.m_PS[m_RP.m_nProcessThreadID].m_fPreprocessTime*1000.f, m_RP.m_PS[m_RP.m_nProcessThreadID].m_fOcclusionTime*1000.f);
  TextToScreenColor(1,23, 0,2,0,1, "Skinning:   %8.02f ms (Skinned Objects: %d)",  m_RP.m_PS[m_RP.m_nProcessThreadID].m_fSkinningTime*1000.f, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendSkinnedObjects);

  // TODO: implement CV_r_profileDIPs=2 mode - draw only one triangle per draw call

  // subtract flush overhead
  double fFlushOverhead = 0;
  if(CV_r_profileGPU)
  {
    FX_Flush();
    fFlushOverhead -= iTimer->GetAsyncCurTime();
    for(int i=0; i<100; i++)
      FX_Flush();
    fFlushOverhead += iTimer->GetAsyncCurTime();
    fFlushOverhead /= 100;

    for(uint32 i=0; i<m_RP.m_Profile.Num(); i++)
      m_RP.m_Profile[i].Time -= fFlushOverhead;
  }

  // merge items with same grouping factor into single item
  if(m_RP.m_Profile.Num())
  {
    qsort(&m_RP.m_Profile[0], m_RP.m_Profile.Num(), sizeof(SProfInfo), Compare_SProfInfo);

    for(uint32 i=0; (i+1)<m_RP.m_Profile.Num(); i++)
    {
      if(!Compare_SProfInfo(&m_RP.m_Profile[i], &m_RP.m_Profile[i+1]))
      {
        m_RP.m_Profile[i].Time += m_RP.m_Profile[i+1].Time;
        m_RP.m_Profile[i].m_nItems++;
        m_RP.m_Profile[i].NumPolys += m_RP.m_Profile[i+1].NumPolys;
        m_RP.m_Profile[i].NumDips += m_RP.m_Profile[i+1].NumDips;
        m_RP.m_Profile.DelElem(i+1);
        i--;
      }
    }
  }

  // smooth values over time
  if(CV_r_ProfileShadersSmooth && (CV_r_ProfileShadersGroupByName == 1 || CV_r_ProfileShadersGroupByName == 2))
  {
    typedef std::map<string,STimeStorage*,stl::less_stricmp<string> > TimeStorageMap;
    static TimeStorageMap timeStorageMap;

    char strName[128]="";

    for(uint32 i=0; i<m_RP.m_Profile.Num(); i++)
    {
      SProfInfo *pi1 = &m_RP.m_Profile[i];
    
      if(CV_r_ProfileShadersGroupByName == 1)
        _snprintf(strName, sizeof(strName), "%s.%s", pi1->pShader->GetName(), pi1->pTechnique->m_Name.c_str());
      else
        strncpy(strName, pi1->pTechnique->m_Name.c_str(), sizeof(strName));

      STimeStorage * pTimeStorage = stl::find_in_map( timeStorageMap, CONST_TEMP_STRING(strName), NULL );
      if(!pTimeStorage)
        pTimeStorage = timeStorageMap[strName] = new STimeStorage();

      float fSmooth = CV_r_ProfileShadersSmooth;
      m_RP.m_Profile[i].pTechnique->m_fProfileTime = pTimeStorage->fTime = (m_RP.m_Profile[i].Time + pTimeStorage->fTime*fSmooth)/(fSmooth+1.f);
      m_RP.m_Profile[i].m_nItems = (int)(pTimeStorage->fItems = ((float)m_RP.m_Profile[i].m_nItems + pTimeStorage->fItems*fSmooth)/(fSmooth+1.f));
      m_RP.m_Profile[i].NumDips = (int)(pTimeStorage->fNumDips = ((float)m_RP.m_Profile[i].NumDips + pTimeStorage->fNumDips*fSmooth)/(fSmooth+1.f));
      m_RP.m_Profile[i].NumPolys = (int)(pTimeStorage->fNumPolys = ((float)m_RP.m_Profile[i].NumPolys + pTimeStorage->fNumPolys*fSmooth)/(fSmooth+1.f));
      pTimeStorage->nUsedFrameId = GetFrameID(false);
    }

    // fade items not used in this frame, delete not important items
    TimeStorageMap::iterator next;
    for (TimeStorageMap::iterator it = timeStorageMap.begin(); it != timeStorageMap.end(); it = next)
    {
      next = it; next++;
      STimeStorage * pTimeStorage = (STimeStorage *)(it->second);
      if(pTimeStorage->nUsedFrameId != GetFrameID(false))
      {
        float fSmooth = CV_r_ProfileShadersSmooth;
        pTimeStorage->fTime = (0 + pTimeStorage->fTime*fSmooth)/(fSmooth+1.f);
        pTimeStorage->fItems = (0 + pTimeStorage->fItems*fSmooth)/(fSmooth+1.f);
        pTimeStorage->fNumDips = (0 + pTimeStorage->fNumDips*fSmooth)/(fSmooth+1.f);
        pTimeStorage->fNumPolys = (0 + pTimeStorage->fNumPolys*fSmooth)/(fSmooth+1.f);

        if(pTimeStorage->fTime < 0.0001f)
        {
          timeStorageMap.erase(it);
          delete pTimeStorage;
        }
      }
    }
  }
  else
  {
    for(uint32 i=0; i<m_RP.m_Profile.Num(); i++)
      m_RP.m_Profile[i].pTechnique->m_fProfileTime = 
        (float)(m_RP.m_Profile[i].Time + m_RP.m_Profile[i].pTechnique->m_fProfileTime*(float)CV_r_ProfileShadersSmooth)/((float)CV_r_ProfileShadersSmooth+1);
  }

  const uint32 nMaxLines = 18;

  // sort by final smoothed time
  if(m_RP.m_Profile.Num())
    qsort(&m_RP.m_Profile[0], m_RP.m_Profile.Num(), sizeof(SProfInfo), TimeProfCallback );

  float fTimeAll = 0;

  // print
  for(uint32 nLine=0; nLine<m_RP.m_Profile.Num(); nLine++)
  {
    float fProfTime = m_RP.m_Profile[nLine].pTechnique->m_fProfileTime*1000.f;

    fTimeAll += fProfTime;

    if (nLine >= nMaxLines)
      continue;

    if(CV_r_ProfileShadersGroupByName == 1)
    { // no RT flags
      TextToScreenColor(4,(27+(nLine*3)), 1,0,0,1, "%8.2f ms, %6d tris, %4d DIPs, '%s.%s', %d item(s)", 
        fProfTime, 
        m_RP.m_Profile[nLine].NumPolys,
        m_RP.m_Profile[nLine].NumDips,
        m_RP.m_Profile[nLine].pShader->GetName(),
        m_RP.m_Profile[nLine].pTechnique->m_Name.c_str(),
        m_RP.m_Profile[nLine].m_nItems+1);
    }
    else if(CV_r_ProfileShadersGroupByName == 2)
    { // only Technique name - no RT flag, no shader name
      TextToScreenColor(4,(27+(nLine*3)), 1,0,0,1, "%8.2f ms, %6d tris, %4d DIPs, '%s', %d item(s)", 
        fProfTime, 
        m_RP.m_Profile[nLine].NumPolys,
        m_RP.m_Profile[nLine].NumDips,
        m_RP.m_Profile[nLine].pTechnique->m_Name.c_str(),
        m_RP.m_Profile[nLine].m_nItems+1);
    }
    else
    { // with RT flags and all names
      TextToScreenColor(4,(27+(nLine*3)), 1,0,0,1, "%8.2f ms, %6d tris, %4d DIPs, '%s.%s(0x%x)', %d item(s)", 
        fProfTime, 
        m_RP.m_Profile[nLine].NumPolys,
        m_RP.m_Profile[nLine].NumDips,
        m_RP.m_Profile[nLine].pShader->GetName(),
        m_RP.m_Profile[nLine].pTechnique->m_Name.c_str(),
        m_RP.m_Profile[nLine].pShader->m_nMaskGenFX,
        m_RP.m_Profile[nLine].m_nItems+1);
    }
  }

  TextToScreenColor(1,(28+(nMaxLines*3)), 0,2,0,1, "Total unique items:            %8d",      m_RP.m_Profile.Num());
  TextToScreenColor(1,(31+(nMaxLines*3)), 0,2,0,1, "Total flush time:              %8.2f ms", fTimeAll);
  TextToScreenColor(1,(34+(nMaxLines*3)), 0,2,0,1, "Total shaders processing time: %8.2f ms", m_RP.m_PS[m_RP.m_nProcessThreadID].m_fSceneTime);
  TextToScreenColor(1,(37+(nMaxLines*3)), 0,2,0,1, "FX_Flush() overhead time:      %8.2f ms", fFlushOverhead*1000);

  //std::vector<Matrix33> vv;
  //vv.resize(10);

    /*int N = 1000;
    memset(mesh1, 0, sizeof(mesh1));
    memset(mesh2, 0, sizeof(mesh2));
    Matrix33 rotation;
    memset(&rotation, 0, sizeof(rotation));
    Vec3 offset(0, 0, 0);
    for (int j = 0; j<10000; ++j)
    {
      rotate_mesh(mesh1, mesh2, N, rotation, offset);
    }*/




  /*{
  int i = 0;
  double timeC = 0;
  double timeSSE = 0;

  CCamera cam = GetCamera();
  Vec3d camPos = cam.GetPos();
  AABB aabb;
  aabb.min = camPos+Vec3d(-10, -16,-16);
  aabb.max = camPos+Vec3d(16,32,16);
  Vec3d Origin = (aabb.min+aabb.max)*0.5f;
  Vec3d Extent = aabb.max - Origin;

  ticks(timeC);
  for (i=0; i<100000; i++)
  {
  cam.IsAABBVisible_exact(aabb, NULL);
  }
  unticks(timeC);

  ticks(timeSSE);
  for (i=0; i<100000; i++)
  {
  cam.IsAABBVisible_exact_SSE(Origin, Extent, NULL);
  }
  unticks(timeSSE);

  TextToScreenColor(8,(36+(nLine*3)), 0,2,0,1, "TimeC %8.02f ms, TimeSSE %8.02f ms", (float)(timeC*1000.0*m_RP.m_SecondsPerCycle), (float)(timeSSE*1000.0*m_RP.m_SecondsPerCycle));
  }*/
  /*{
  int i = 0;
  float timeVMX = 0;
  float timeFloat = 0;
  //double time3DN = 0;
  //double timeSSE = 0;
  Matrix44A m, m1, mOut;
  m = GetCamera().GetMatrix();
  m1 = GetCamera().GetMatrix();
  m1.SetIdentity();

  timeVMX = iTimer->GetAsyncCurTime();
  for (i=0; i<400000; i++)
  {
    mOut.Multiply(m, m1);
    m1 = mOut;
  }
  timeVMX = iTimer->GetAsyncCurTime()-timeVMX;
  m = mOut;

  timeFloat = iTimer->GetAsyncCurTime();
  for (i=0; i<400000; i++)
  {
    mOut.m00 = m1.m00*m.m00 + m1.m01*m.m10 + m1.m02*m.m20 + m1.m03*m.m30;
    mOut.m10 = m1.m10*m.m00 + m1.m11*m.m10 + m1.m12*m.m20 + m1.m13*m.m30;
    mOut.m20 = m1.m20*m.m00 + m1.m21*m.m10 + m1.m22*m.m20 + m1.m23*m.m30;
    mOut.m30 = m1.m30*m.m00 + m1.m31*m.m10 + m1.m32*m.m20 + m1.m33*m.m30;
    mOut.m01 = m1.m00*m.m01 + m1.m01*m.m11 + m1.m02*m.m21 + m1.m03*m.m31;
    mOut.m11 = m1.m10*m.m01 + m1.m11*m.m11 + m1.m12*m.m21 + m1.m13*m.m31;
    mOut.m21 = m1.m20*m.m01 + m1.m21*m.m11 + m1.m22*m.m21 + m1.m23*m.m31;
    mOut.m31 = m1.m30*m.m01 + m1.m31*m.m11 + m1.m32*m.m21 + m1.m33*m.m31;
    mOut.m02 = m1.m00*m.m02 + m1.m01*m.m12 + m1.m02*m.m22 + m1.m03*m.m32;
    mOut.m12 = m1.m10*m.m02 + m1.m11*m.m12 + m1.m12*m.m22 + m1.m13*m.m32;
    mOut.m22 = m1.m20*m.m02 + m1.m21*m.m12 + m1.m22*m.m22 + m1.m23*m.m32;
    mOut.m32 = m1.m30*m.m02 + m1.m31*m.m12 + m1.m32*m.m22 + m1.m33*m.m32;
    mOut.m03 = m1.m00*m.m03 + m1.m01*m.m13 + m1.m02*m.m23 + m1.m03*m.m33;
    mOut.m13 = m1.m10*m.m03 + m1.m11*m.m13 + m1.m12*m.m23 + m1.m13*m.m33;
    mOut.m23 = m1.m20*m.m03 + m1.m21*m.m13 + m1.m22*m.m23 + m1.m23*m.m33;
    mOut.m33 = m1.m30*m.m03 + m1.m31*m.m13 + m1.m32*m.m23 + m1.m33*m.m33;
    m1 = mOut;
  }
  timeFloat = iTimer->GetAsyncCurTime()-timeFloat;
  TextToScreenColor(8,(40+(nMaxLines*3)), 0,2,0,1, "TimeFloat: %f s, TimeVMX: %f s,", timeFloat, timeVMX);
  }*/
  /*ticks(timeC);
  for (i=0; i<100000; i++)
  {
  QQinvertMatrixf(m1.GetData(), m.GetData());
  }
  unticks(timeC);

  if (m_Cpu->mCpu[0].mFeatures & CFI_3DNOW)
  {
  ticks(time3DN);
  for (i=0; i<100000; i++)
  {
  invertMatrixf_3DNow(m1.GetData(), m.GetData());
  }
  unticks(time3DN);
  }

  ticks(timeSSE);
  for (i=0; i<100000; i++)
  {
  invertMatrixf_SSE(m1.GetData(), m.GetData());
  }
  unticks(timeSSE);

  TextToScreenColor(8,(36+(nLine*3)), 0,2,0,1, "TimeC: %8.02f ms, TimeC33: %8.02f ms,, Time3DN: %8.02f ms, TimeSSE: %8.02f ms", (float)(timeC*1000.0*m_RP.m_SecondsPerCycle), (float)(timeC33*1000.0*m_RP.m_SecondsPerCycle), (float)(time3DN*1000.0*m_RP.m_SecondsPerCycle), (float)(timeSSE*1000.0*m_RP.m_SecondsPerCycle));
  }*/
  /*{
  int i = 0;
  double timeC = 0;
  double timeSSE = 0;
  double timeSSENew = 0;
  byte *dataSrc = new byte[1024*1024*10];
  byte *dataDst = new byte[1024*1024*10];

  ticks(timeC);
  for (i=0; i<10; i++)
  {
  memcpy(dataDst, dataSrc, 1024*1024*10-128);
  }
  unticks(timeC);

  byte *Dst = (byte *)((int)(dataDst+15)&0xfffffff0);
  byte *Src = (byte *)((int)(dataSrc+15)&0xfffffff0);
  ticks(timeSSE);
  for (i=0; i<10; i++)
  {
  cryMemcpy(Dst, Src, 1024*1024*10-128);
  }
  unticks(timeSSE);

  ticks(timeSSENew);
  for (i=0; i<10; i++)
  {
  cryMemcpy(Dst, Src, 1024*1024*10-128, 0);
  }
  unticks(timeSSENew);

  delete [] dataSrc;
  delete [] dataDst;

  TextToScreenColor(8,(36+(nLine*3)), 0,2,0,1, "TimeC: %8.02f ms, TimeSSE: %8.02f ms", (float)(timeC*1000.0*g_SecondsPerCycle), (float)(timeSSE*1000.0*g_SecondsPerCycle));
  }*/
  /*{
  int i = 0;
  double timeCM10 = 0;
  double timeCF10 = 0;
  double timeCM60 = 0;
  double timeCF60 = 0;
  double timeCM110 = 0;
  double timeCF110 = 0;
  double timeCM510 = 0;
  double timeCF510 = 0;
  double timeCM1010 = 0;
  double timeCF1010 = 0;
  double timeCM5010 = 0;
  double timeCF5010 = 0;
  double timeCM10010 = 0;
  double timeCF10010 = 0;
  double timeCM100010 = 0;
  double timeCF100010 = 0;

  double timeUM10 = 0;
  double timeUF10 = 0;
  double timeUM60 = 0;
  double timeUF60 = 0;
  double timeUM110 = 0;
  double timeUF110 = 0;
  double timeUM510 = 0;
  double timeUF510 = 0;
  double timeUM1010 = 0;
  double timeUF1010 = 0;
  double timeUM5010 = 0;
  double timeUF5010 = 0;
  double timeUM10010 = 0;
  double timeUF10010 = 0;
  double timeUM100010 = 0;
  double timeUF100010 = 0;

  static FMallocWindows *pM;
  if (!pM)
  {
  pM = new FMallocWindows;
  pM->Init();
  }
  void *pPtr[1000];

  ticks(timeCM10);
  for (i=0; i<1000; i++)
  pPtr[i] = malloc(10);
  unticks(timeCM10);
  ticks(timeCF10);
  for (i=0; i<1000; i++)
  free(pPtr[i]);
  unticks(timeCF10);

  ticks(timeUM10);
  for (i=0; i<1000; i++)
  pPtr[i] = pM->Malloc(10, "Test");
  unticks(timeUM10);
  ticks(timeUF10);
  for (i=0; i<1000; i++)
  pM->Free(pPtr[i]);
  unticks(timeUF10);

  ticks(timeCM60);
  for (i=0; i<1000; i++)
  pPtr[i] = malloc(60);
  unticks(timeCM60);
  ticks(timeCF60);
  for (i=0; i<1000; i++)
  free(pPtr[i]);
  unticks(timeCF60);

  ticks(timeUM60);
  for (i=0; i<1000; i++)
  pPtr[i] = pM->Malloc(60, "Test");
  unticks(timeUM60);
  ticks(timeUF60);
  for (i=0; i<1000; i++)
  pM->Free(pPtr[i]);
  unticks(timeUF60);

  ticks(timeCM110);
  for (i=0; i<1000; i++)
  pPtr[i] = malloc(110);
  unticks(timeCM110);
  ticks(timeCF110);
  for (i=0; i<1000; i++)
  free(pPtr[i]);
  unticks(timeCF110);

  ticks(timeUM110);
  for (i=0; i<1000; i++)
  pPtr[i] = pM->Malloc(110, "Test");
  unticks(timeUM110);
  ticks(timeUF110);
  for (i=0; i<1000; i++)
  pM->Free(pPtr[i]);
  unticks(timeUF110);

  ticks(timeCM510);
  for (i=0; i<1000; i++)
  pPtr[i] = malloc(510);
  unticks(timeCM510);
  ticks(timeCF510);
  for (i=0; i<1000; i++)
  free(pPtr[i]);
  unticks(timeCF510);

  ticks(timeUM510);
  for (i=0; i<1000; i++)
  pPtr[i] = pM->Malloc(510, "Test");
  unticks(timeUM510);
  ticks(timeUF510);
  for (i=0; i<1000; i++)
  pM->Free(pPtr[i]);
  unticks(timeUF510);

  ticks(timeCM1010);
  for (i=0; i<1000; i++)
  pPtr[i] = malloc(1010);
  unticks(timeCM1010);
  ticks(timeCF1010);
  for (i=0; i<1000; i++)
  free(pPtr[i]);
  unticks(timeCF1010);

  ticks(timeUM1010);
  for (i=0; i<1000; i++)
  pPtr[i] = pM->Malloc(1010, "Test");
  unticks(timeUM1010);
  ticks(timeUF1010);
  for (i=0; i<1000; i++)
  pM->Free(pPtr[i]);
  unticks(timeUF1010);

  ticks(timeCM5010);
  for (i=0; i<1000; i++)
  pPtr[i] = malloc(5010);
  unticks(timeCM5010);
  ticks(timeCF5010);
  for (i=0; i<1000; i++)
  free(pPtr[i]);
  unticks(timeCF5010);

  ticks(timeUM5010);
  for (i=0; i<1000; i++)
  pPtr[i] = pM->Malloc(5010, "Test");
  unticks(timeUM5010);
  ticks(timeUF5010);
  for (i=0; i<1000; i++)
  pM->Free(pPtr[i]);
  unticks(timeUF5010);

  ticks(timeCM10010);
  for (i=0; i<1000; i++)
  pPtr[i] = malloc(10010);
  unticks(timeCM10010);
  ticks(timeCF10010);
  for (i=0; i<1000; i++)
  free(pPtr[i]);
  unticks(timeCF10010);

  ticks(timeUM10010);
  for (i=0; i<1000; i++)
  pPtr[i] = pM->Malloc(10010, "Test");
  unticks(timeUM10010);
  ticks(timeUF10010);
  for (i=0; i<1000; i++)
  pM->Free(pPtr[i]);
  unticks(timeUF10010);

  ticks(timeCM100010);
  for (i=0; i<1000; i++)
  pPtr[i] = malloc(100010);
  unticks(timeCM100010);
  ticks(timeCF100010);
  for (i=0; i<1000; i++)
  free(pPtr[i]);
  unticks(timeCF100010);

  ticks(timeUM100010);
  for (i=0; i<1000; i++)
  pPtr[i] = pM->Malloc(100010, "Test");
  unticks(timeUM100010);
  ticks(timeUF100010);
  for (i=0; i<1000; i++)
  pM->Free(pPtr[i]);
  unticks(timeUF100010);

  TextToScreenColor(1,(36+(nLine*3)), 0,2,0,1, "CM_10: %3.02f, CM_60: %3.02f, CM_110: %3.02f, CM_510: %3.02f, CM_1010: %3.02f, CM_5010: %3.02f, CM_10010: %3.02f, CM_100010: %3.02f", (float)(timeCM10*1000.0*g_SecondsPerCycle), (float)(timeCM60*1000.0*g_SecondsPerCycle), (float)(timeCM110*1000.0*g_SecondsPerCycle), (float)(timeCM510*1000.0*g_SecondsPerCycle), (float)(timeCM1010*1000.0*g_SecondsPerCycle), (float)(timeCM5010*1000.0*g_SecondsPerCycle), (float)(timeCM10010*1000.0*g_SecondsPerCycle), (float)(timeCM100010*1000.0*g_SecondsPerCycle));
  TextToScreenColor(1,(40+(nLine*3)), 0,2,0,1, "UM_10: %3.02f, UM_60: %3.02f, UM_110: %3.02f, UM_510: %3.02f, UM_1010: %3.02f, UM_5010: %3.02f, UM_10010: %3.02f, UM_100010: %3.02f", (float)(timeUM10*1000.0*g_SecondsPerCycle), (float)(timeUM60*1000.0*g_SecondsPerCycle), (float)(timeUM110*1000.0*g_SecondsPerCycle), (float)(timeUM510*1000.0*g_SecondsPerCycle), (float)(timeUM1010*1000.0*g_SecondsPerCycle), (float)(timeUM5010*1000.0*g_SecondsPerCycle), (float)(timeUM10010*1000.0*g_SecondsPerCycle), (float)(timeUM100010*1000.0*g_SecondsPerCycle));
  TextToScreenColor(1,(44+(nLine*3)), 0,2,0,1, "CF_10: %3.02f, CF_60: %3.02f, CF_110: %3.02f, CF_510: %3.02f, CF_1010: %3.02f, CF_5010: %3.02f, CF_10010: %3.02f, CF_100010: %3.02f", (float)(timeCF10*1000.0*g_SecondsPerCycle), (float)(timeCF60*1000.0*g_SecondsPerCycle), (float)(timeCF110*1000.0*g_SecondsPerCycle), (float)(timeCF510*1000.0*g_SecondsPerCycle), (float)(timeCF1010*1000.0*g_SecondsPerCycle), (float)(timeCF5010*1000.0*g_SecondsPerCycle), (float)(timeCF10010*1000.0*g_SecondsPerCycle), (float)(timeCF100010*1000.0*g_SecondsPerCycle));
  TextToScreenColor(1,(48+(nLine*3)), 0,2,0,1, "UF_10: %3.02f, UF_60: %3.02f, UF_110: %3.02f, UF_510: %3.02f, UF_1010: %3.02f, UF_5010: %3.02f, UF_10010: %3.02f, UF_100010: %3.02f", (float)(timeUF10*1000.0*g_SecondsPerCycle), (float)(timeUF60*1000.0*g_SecondsPerCycle), (float)(timeUF110*1000.0*g_SecondsPerCycle), (float)(timeUF510*1000.0*g_SecondsPerCycle), (float)(timeUF1010*1000.0*g_SecondsPerCycle), (float)(timeUF5010*1000.0*g_SecondsPerCycle), (float)(timeUF10010*1000.0*g_SecondsPerCycle), (float)(timeUF100010*1000.0*g_SecondsPerCycle));
  }*/
}


_MS_ALIGN(16) struct SPreprocess
{
  int m_nPreprocess;
  int m_Num;
  CRenderObject *m_pObject;
  int m_nTech;
  CShader *m_Shader;
  SRenderShaderResources *m_pRes;
  CRendElementBase *m_RE;

#if (defined(PS3) && !defined(__SPU__)) || defined(XENON)
	
	SPreprocess () {}

	SPreprocess (const SPreprocess& cpy)
  {    
		COMPILE_TIME_ASSERT((sizeof(SPreprocess)&0xf)==0);

		cryVecMemcpy<sizeof(SPreprocess)>(this, &cpy);
  }
  
	const SPreprocess& operator = (const SPreprocess& cpy)
  {    
		cryVecMemcpy<sizeof(SPreprocess)>(this, &cpy);

		return *this;
  }
#endif
} _ALIGN(16);

struct Compare2
{
	bool operator()(const SPreprocess& a, const SPreprocess& b) const
	{
		return a.m_nPreprocess < b.m_nPreprocess;
	}
};

// Current scene preprocess operations (Rendering to RT, screen effects initializing, ...)
int CD3D9Renderer::EF_Preprocess(SRendItem *ri, uint32 nums, uint32 nume, RenderFunc pRenderFunc)
{
  uint32 i, j;
  CShader *Shader;
  SRenderShaderResources *Res;
  CRenderObject *pObject;
  int nTech;

  SPreprocess Procs[512];
  uint32 nProcs = 0;

  float time0 = iTimer->GetAsyncCurTime();

  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID], "*** Start preprocess frame ***\n");

  int DLDFlags = 0;
  int nReturn = 0;

  for (i=nums; i<nume; i++)
  {
    if (nProcs >= 512)
      break;
    SRendItem::mfGet(ri[i].SortVal, nTech, Shader, Res);
    pObject = ri[i].pObj;
    if (!(ri[i].nBatchFlags & 0xffff0000))
      break;
    nReturn++;
    if (nTech < 0)
      nTech = 0;
    if (nTech < (int)Shader->m_HWTechniques.Num())
    {
      SShaderTechnique *pTech = Shader->m_HWTechniques[nTech];
      for (j=SPRID_FIRST; j<32; j++)
      {
        uint32 nMask = 1<<j;
        if (nMask >= FSPR_MAX || nMask > (ri[i].nBatchFlags & 0xffff0000))
          break;
        if (nMask & ri[i].nBatchFlags)
        {
          Procs[nProcs].m_nPreprocess = j;
          Procs[nProcs].m_Num = i;
          Procs[nProcs].m_Shader = Shader;
          Procs[nProcs].m_pRes = Res;
          Procs[nProcs].m_RE = ri[i].Item;
          Procs[nProcs].m_pObject = pObject;
          Procs[nProcs].m_nTech = nTech;
          nProcs++;
        }
      }
    }
  }
  if (!nProcs)
    return 0;
  std::sort(&Procs[0], &Procs[nProcs], Compare2());

  if (pRenderFunc != FX_FlushShader_General)
    return nReturn;

  bool bRes = true;
  for (i=0; i<nProcs; i++)
  {
    SPreprocess *pr = &Procs[i];
    if (!pr->m_Shader)
      continue;
    switch (pr->m_nPreprocess)
    {
    case SPRID_CORONA:
      break;

    case SPRID_GENSPRITES:
      m_pRT->RC_PreprGenerateFarTrees((CREFarTreeSprites *)pr->m_RE);
      break;

    case SPRID_SCANTEX:
    case SPRID_SCANCM:
    case SPRID_SCANLCM:
    case SPRID_SCANTEXWATER:
      if (!(m_RP.m_TI[m_RP.m_nFillThreadID].m_PersFlags & RBPF_DRAWTOTEXTURE))
      {
        CRenderObject *pObj = pr->m_pObject;
        int nT = pr->m_nTech;
        if (nT < 0)
          nT = 0;
        SShaderTechnique *pTech = pr->m_Shader->m_HWTechniques[nT];
        SRenderShaderResources *pRes = pr->m_pRes;
        for (j=0; j<pTech->m_RTargets.Num(); j++)
        {
          SHRenderTarget *pTarg = pTech->m_RTargets[j];
          if (pTarg->m_eOrder == eRO_PreProcess)
            bRes &= FX_DrawToRenderTarget(pr->m_Shader, pRes, pObj, pTech, pTarg, pr->m_nPreprocess, pr->m_RE);
        }
        if (pRes)
        {
          for (j=0; j<pRes->m_RTargets.Num(); j++)
          {
            SHRenderTarget *pTarg = pRes->m_RTargets[j];
            if (pTarg->m_eOrder == eRO_PreProcess)
              bRes &= FX_DrawToRenderTarget(pr->m_Shader, pRes, pObj, pTech, pTarg, pr->m_nPreprocess, pr->m_RE);
          }
        }
      }
      break;

    case SPRID_CUSTOMTEXTURE:
      if (!(m_RP.m_TI[m_RP.m_nFillThreadID].m_PersFlags & RBPF_DRAWTOTEXTURE))
      {
        CRenderObject *pObj = pr->m_pObject;
        int nT = pr->m_nTech;
        if (nT < 0)
          nT = 0;
        SShaderTechnique *pTech = pr->m_Shader->m_HWTechniques[nT];
        SRenderShaderResources *pRes = pr->m_pRes;
        for (j=0; j<pRes->m_RTargets.Num(); j++)
        {
          SHRenderTarget *pTarg = pRes->m_RTargets[j];
          if (pTarg->m_eOrder == eRO_PreProcess)
            bRes &= FX_DrawToRenderTarget(pr->m_Shader, pRes, pObj, pTech, pTarg, pr->m_nPreprocess, pr->m_RE);
        }
      }
      break;
    case SPRID_PANORAMA:
      {
        assert (pr->m_RE->mfGetType() == eDATA_PanoramaCluster);
        if (pr->m_RE->mfGetType() == eDATA_PanoramaCluster)
        {
          CREPanoramaCluster *pRE = (CREPanoramaCluster *)pr->m_RE;
          CRenderObject *pObj = pr->m_pObject;

          pRE->UpdateImposter();
        }
      }
      break;
    case SPRID_GENCLOUDS:







      break;

    default:
      assert(0);
    }
  }

  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID], "*** End preprocess frame ***\n");

  m_RP.m_PS[m_RP.m_nProcessThreadID].m_fPreprocessTime += iTimer->GetAsyncCurTime()-time0;

  return nReturn;
}

void CD3D9Renderer::EF_EndEf2D(bool bSort)
{
}

#ifndef EXCLUDE_SCALEFORM_SDK

//////////////////////////////////////////////////////////////////////////
struct SSF_ResourcesD3D
{
  CCryName m_shTech_SolidColor;
  CCryName m_shTech_Glyph;
  CCryName m_shTech_CxformMultiplyTexture;
  CCryName m_shTech_CxformTexture;
  CCryName m_shTech_CxformGouraudMultiplyNoAddAlpha;
  CCryName m_shTech_CxformGouraudNoAddAlpha;
  CCryName m_shTech_CxformGouraudMultiply;
  CCryName m_shTech_CxformGouraud;
  CCryName m_shTech_CxformGouraudMultiplyTexture;
  CCryName m_shTech_CxformGouraudTexture;
  CCryName m_shTech_CxformMultiply2Texture;
  CCryName m_shTech_Cxform2Texture;

  CShader* m_pShader;

  D3DVertexDeclaration* m_pVertexDeclXY16i;
  D3DVertexDeclaration* m_pVertexDeclXY16iC32;
  D3DVertexDeclaration* m_pVertexDeclXY16iCF32;
  D3DVertexDeclaration* m_pVertexDeclGlyph;
  D3DQuery* m_pQuery;

  SSF_ResourcesD3D(CD3D9Renderer* pRenderer)
  : m_shTech_SolidColor("SolidColor")
  , m_shTech_Glyph("Glyph")
  , m_shTech_CxformMultiplyTexture("CxformMultiplyTexture")
  , m_shTech_CxformTexture("CxformTexture")
  , m_shTech_CxformGouraudMultiplyNoAddAlpha("CxformGouraudMultiplyNoAddAlpha")
  , m_shTech_CxformGouraudNoAddAlpha("CxformGouraudNoAddAlpha")
  , m_shTech_CxformGouraudMultiply("CxformGouraudMultiply")
  , m_shTech_CxformGouraud("CxformGouraud")
  , m_shTech_CxformGouraudMultiplyTexture("CxformGouraudMultiplyTexture")
  , m_shTech_CxformGouraudTexture("CxformGouraudTexture")
  , m_shTech_CxformMultiply2Texture("CxformMultiply2Texture")
  , m_shTech_Cxform2Texture("Cxform2Texture")
  , m_pShader(0)
  , m_pVertexDeclXY16i(0)
  , m_pVertexDeclXY16iC32(0)
  , m_pVertexDeclXY16iCF32(0)
  , m_pVertexDeclGlyph(0)
  , m_pQuery(0)
  {
  }

  ~SSF_ResourcesD3D()
  {
    SAFE_RELEASE(m_pShader);

    SAFE_RELEASE(m_pVertexDeclXY16i);
    SAFE_RELEASE(m_pVertexDeclXY16iC32);
    SAFE_RELEASE(m_pVertexDeclXY16iCF32);
    SAFE_RELEASE(m_pVertexDeclGlyph);

    SAFE_RELEASE(m_pQuery);
  }

  CShader* GetShader(CD3D9Renderer* pRenderer)
  {
    if (!m_pShader)
      m_pShader = pRenderer->m_cEF.mfForName("Scaleform", EF_SYSTEM);
    return m_pShader;
  }
};

//////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::SF_CreateResources()
{
  m_pSFResD3D = new SSF_ResourcesD3D(this);
}

//////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::SF_DestroyResources()
{
  SAFE_DELETE(m_pSFResD3D);
}

//////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::SF_ResetResources()
{
  if (m_pSFResD3D)
  {
    SAFE_RELEASE(m_pSFResD3D->m_pQuery);
  }
}

//////////////////////////////////////////////////////////////////////////
bool CD3D9Renderer::SF_SetVertexDeclaration(SSF_GlobalDrawParams::EVertexFmt vertexFmt)
{
#if !defined(DIRECT3D10)
  const D3DVERTEXELEMENT9 VertexDeclXY16i[] =
  {
    {0, 0, D3DDECLTYPE_SHORT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    D3DDECL_END()
  };
  const D3DVERTEXELEMENT9 VertexDeclXY16iC32[] =
  {
    {0, 0, D3DDECLTYPE_SHORT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    D3DDECL_END()
  };
  const D3DVERTEXELEMENT9 VertexDeclXY16iCF32[] =
  {
    {0, 0, D3DDECLTYPE_SHORT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    {0, 8, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
    D3DDECL_END()
  };
  const D3DVERTEXELEMENT9 VertexDeclGlyph[] =
  {
    {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    {0, 16, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    D3DDECL_END()
  };

  IDirect3DVertexDeclaration9* pVD(0);

#	define SF_CREATE_VERTEX_DECL(inputElements, pDecl) m_pd3dDevice->CreateVertexDeclaration(inputElements, &pDecl);

#else // #if !defined(DIRECT3D10)
  const D3D11_INPUT_ELEMENT_DESC VertexDeclXY16i[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R16G16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
  };
  const D3D11_INPUT_ELEMENT_DESC VertexDeclXY16iC32[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R16G16_SINT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM,	0, 4, D3D11_INPUT_PER_VERTEX_DATA, 0}
  };
  const D3D11_INPUT_ELEMENT_DESC VertexDeclXY16iCF32[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R16G16_SINT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM,	0, 4, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM,	0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0}
  };
  const D3D11_INPUT_ELEMENT_DESC VertexDeclGlyph[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}
  };

  ID3D11InputLayout* pVD(0);

	if (!CHWShader_D3D::m_pCurInstVS || !CHWShader_D3D::m_pCurInstVS->m_pShaderData || CHWShader_D3D::m_pCurInstVS->m_bFallback)
		return false;

#	define SF_CREATE_VERTEX_DECL(inputElements, pDecl) \
	m_pd3dDevice->CreateInputLayout(inputElements, sizeof(inputElements)/sizeof(inputElements[0]), CHWShader_D3D::m_pCurInstVS->m_pShaderData, \
		CHWShader_D3D::m_pCurInstVS->m_nShaderByteCodeSize, &pDecl);

#endif // #if !defined(DIRECT3D10)

  SSF_ResourcesD3D& sfRes(SF_GetResources());

  HRESULT hr(S_OK);
  switch (vertexFmt)
  {
  case SSF_GlobalDrawParams::Vertex_XY16i:
    {
      if (!sfRes.m_pVertexDeclXY16i)
        hr = SF_CREATE_VERTEX_DECL(VertexDeclXY16i, sfRes.m_pVertexDeclXY16i);
      pVD = sfRes.m_pVertexDeclXY16i;
      break;
    }
  case SSF_GlobalDrawParams::Vertex_XY16iC32:
    {
      if (!sfRes.m_pVertexDeclXY16iC32)
        hr = SF_CREATE_VERTEX_DECL(VertexDeclXY16iC32, sfRes.m_pVertexDeclXY16iC32);
      pVD = sfRes.m_pVertexDeclXY16iC32;
      break;
    }
  case SSF_GlobalDrawParams::Vertex_XY16iCF32:
    {
      if (!sfRes.m_pVertexDeclXY16iCF32)
        hr = SF_CREATE_VERTEX_DECL(VertexDeclXY16iCF32, sfRes.m_pVertexDeclXY16iCF32);
      pVD = sfRes.m_pVertexDeclXY16iCF32;
      break;
    }
  case SSF_GlobalDrawParams::Vertex_Glyph:
    {
      if (!sfRes.m_pVertexDeclGlyph)
        hr = SF_CREATE_VERTEX_DECL(VertexDeclGlyph, sfRes.m_pVertexDeclGlyph);
      pVD = sfRes.m_pVertexDeclGlyph;
      break;
    }
  default:
    {
      assert(0);
      break;
    }
  }

  assert(SUCCEEDED(hr) && pVD);
  if (FAILED(hr) || !pVD)
    return false;

  if (m_pLastVDeclaration != pVD)
  {
    m_pLastVDeclaration = pVD;
#if !defined(DIRECT3D10)
    hr = m_pd3dDevice->SetVertexDeclaration(pVD);
#else
    m_pd3dDeviceContext->IASetInputLayout(pVD);
    hr = S_OK;
#endif
  }

  return SUCCEEDED(hr);
}

//////////////////////////////////////////////////////////////////////////
CShader* CD3D9Renderer::SF_SetTechnique(const CCryName& techName)
{
  CShader* pShader(SF_GetResources().GetShader(this));
  if (!pShader)
    return 0;

  SShaderTechnique* pTech(0);
  uint32 i(0);
  for (; i<pShader->m_HWTechniques.Num(); ++i)
  {
    pTech = pShader->m_HWTechniques[i];
    if (techName == pTech->m_Name)
      break;
  }

  if (i == pShader->m_HWTechniques.Num())
    return 0;

  CRenderer* rd(gRenDev);
  rd->m_RP.m_pShader = pShader;
  rd->m_RP.m_nShaderTechnique = i;
  rd->m_RP.m_pCurTechnique = pShader->m_HWTechniques[i];

  return pShader;
}

//////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::SF_SetBlendOp(SSF_GlobalDrawParams::EAlphaBlendOp blendOp, bool reset)
{
  if (!reset)
  {
    if (blendOp != SSF_GlobalDrawParams::Add)
    {
      switch(blendOp)
      {
      case SSF_GlobalDrawParams::Substract:
        {
#if defined(DIRECT3D10)
          SStateBlend bl = m_StatesBL[m_nCurStateBL];
          bl.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_SUBTRACT;
          bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_SUBTRACT;
          SetBlendState(&bl);
#else
          m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT);
#endif
        }
        break;
      case SSF_GlobalDrawParams::RevSubstract:
        {
#if defined(DIRECT3D10)
          SStateBlend bl = m_StatesBL[m_nCurStateBL];
          bl.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
          bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_REV_SUBTRACT;
          SetBlendState(&bl);
#else
          m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
#endif
        }
        break;
      case SSF_GlobalDrawParams::Min:
        {
#if defined(DIRECT3D10)
          SStateBlend bl = m_StatesBL[m_nCurStateBL];
          bl.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MIN;
          bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MIN;
          SetBlendState(&bl);
#else
          m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MIN);
#endif
        }
        break;
      case SSF_GlobalDrawParams::Max:
        {
#if defined(DIRECT3D10)
          SStateBlend bl = m_StatesBL[m_nCurStateBL];
          bl.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MAX;
          bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
          SetBlendState(&bl);
#else
          m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX);
#endif
        }
        break;
      default:
        assert(0);
        break;
      }
    }
  }
  else
  {
    if (blendOp != SSF_GlobalDrawParams::Add)
    {
#if defined(DIRECT3D10)
      SStateBlend bl = m_StatesBL[m_nCurStateBL];
      bl.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
      bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
      SetBlendState(&bl);
#else
      m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
#endif
    }
  }
}


uint32 CD3D9Renderer::SF_AdjustBlendStateForMeasureOverdraw(uint32 blendModeStates)
{
  if (CV_r_measureoverdraw)
  {
    blendModeStates = (blendModeStates & ~GS_BLEND_MASK) | (GS_BLSRC_ONE | GS_BLDST_ONE);
    blendModeStates &= ~GS_ALPHATEST_MASK;
  }
  return blendModeStates;
}


//////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::SF_DrawIndexedTriList( int baseVertexIndex, int minVertexIndex, int numVertices, int startIndex, int triangleCount, const SSF_GlobalDrawParams& params )
{
  FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

  if (IsDeviceLost())
    return;

	SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
	SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

  assert(params.vertexFmt != SSF_GlobalDrawParams::Vertex_Glyph && params.vertexFmt != SSF_GlobalDrawParams::Vertex_None);
  assert(params.pIndexPtr);
  assert(params.indexFmt ==  SSF_GlobalDrawParams::Index_16);

  const SSF_ResourcesD3D& sfRes(SF_GetResources());
  CShader* pSFShader(0);
  {
    //FRAME_PROFILER_FAST("SF_DITL::SetShader", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Set appropriate shader
    SSF_GlobalDrawParams::EFillType fillType(params.fillType);
    if (fillType >= SSF_GlobalDrawParams::GColor && params.texture[0].texID <= 0)
      fillType = SSF_GlobalDrawParams::GColor;

    switch(fillType)
    {
    case SSF_GlobalDrawParams::SolidColor:
      pSFShader = SF_SetTechnique(sfRes.m_shTech_SolidColor);
      break;
    case SSF_GlobalDrawParams::Texture:
      pSFShader = SF_SetTechnique(params.isMultiplyDarkBlendMode ? sfRes.m_shTech_CxformMultiplyTexture : sfRes.m_shTech_CxformTexture);
      break;
    case SSF_GlobalDrawParams::GColor:
      if (params.vertexFmt == SSF_GlobalDrawParams::Vertex_XY16iC32)
        pSFShader = SF_SetTechnique(params.isMultiplyDarkBlendMode ? sfRes.m_shTech_CxformGouraudMultiplyNoAddAlpha : sfRes.m_shTech_CxformGouraudNoAddAlpha);
      else
        pSFShader = SF_SetTechnique(params.isMultiplyDarkBlendMode ? sfRes.m_shTech_CxformGouraudMultiply : sfRes.m_shTech_CxformGouraud);
      break;
    case SSF_GlobalDrawParams::G1Texture:
    case SSF_GlobalDrawParams::G1TextureColor:
      pSFShader = SF_SetTechnique(params.isMultiplyDarkBlendMode ? sfRes.m_shTech_CxformGouraudMultiplyTexture : sfRes.m_shTech_CxformGouraudTexture);
      break;
    case SSF_GlobalDrawParams::G2Texture:
      pSFShader = SF_SetTechnique(params.isMultiplyDarkBlendMode ? sfRes.m_shTech_CxformMultiply2Texture : sfRes.m_shTech_Cxform2Texture);
      break;
    default:
      assert(0);
      break;
    }
  }

  if (!pSFShader)
    return;

	if( params.renderMaskedStates & GS_COLMASK_NONE )
		rTI.m_PersFlags2 |= RBPF2_DISABLECOLORWRITES;

  {
    //FRAME_PROFILER_FAST("SF_DITL::FxBegin", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    m_pSFDrawParams = &params;
    uint32 numPasses(0);
    pSFShader->FXBegin(&numPasses, /*FEF_DONTSETTEXTURES |*/ FEF_DONTSETSTATES);
    if (!numPasses)
    {
      m_pSFDrawParams = 0;
      return;	
    }
    pSFShader->FXBeginPass(0);
  }
  {
    //FRAME_PROFILER_FAST("SF_DITL::SetState", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Set states
    EF_SetState(SF_AdjustBlendStateForMeasureOverdraw(params.blendModeStates) | /*GS_NODEPTHTEST | */params.renderMaskedStates);
    D3DSetCull(eCULL_None);
  }
  {
    //FRAME_PROFILER_FAST("SF_DITL::FX_Commit", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Commit all render changes
    FX_Commit();
  }
  {
    //FRAME_PROFILER_FAST("SF_DITL::SetVertexDecl", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Set vertex declaration
    if (!SF_SetVertexDeclaration(params.vertexFmt))
    {
      pSFShader->FXEndPass();
      pSFShader->FXEnd();
      m_pSFDrawParams = 0;
      return;
    }
  }

  // Copy vertex data...
  uint32 finalStartIndex(-1);
  {
    //FRAME_PROFILER_FAST("SF_DITL::DynBufUpdate", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);
    {
      if (params.pVertexPtr)
      {
        //FRAME_PROFILER_FAST("SF_DITL::CopyVerts", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

        size_t vertexSize(-1);
        switch(params.vertexFmt)
        {
        case SSF_GlobalDrawParams::Vertex_XY16i:
          vertexSize = 4;
          break;
        case SSF_GlobalDrawParams::Vertex_XY16iC32:
          vertexSize = 8;
          break;
        case SSF_GlobalDrawParams::Vertex_XY16iCF32:
          vertexSize = 12;
          break;
        default:
          assert(0);
          break;
        }

        uint32 reqBufferSize(vertexSize * params.numVertices);
        uint32 bufferOffset(-1);
        void* pVB(FX_LockVB(reqBufferSize, bufferOffset));
        memcpy(pVB, params.pVertexPtr, reqBufferSize);
        FX_UnlockVB();
        m_RP.m_VBs[m_RP.m_CurVB].VBPtr_0->Bind(0, bufferOffset, vertexSize);
      }
    }
    {
      //FRAME_PROFILER_FAST("SF_DITL::CopyInds", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

      assert(params.pIndexPtr);			
      void* pIB(m_RP.m_IndexBuf->Lock(params.numIndices, finalStartIndex));
      memcpy(pIB, params.pIndexPtr, params.numIndices * sizeof(uint16));
      m_RP.m_IndexBuf->Unlock();
      m_RP.m_IndexBuf->Bind();
      finalStartIndex += startIndex;
    }		
  }
  {
    //FRAME_PROFILER_FAST("SF_DITL::BlendStateAndDraw", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Override blend op if necessary
    if (!CV_r_measureoverdraw)
      SF_SetBlendOp(params.blendOp);

    // Submit draw call
#if !defined(DIRECT3D10)
    m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, baseVertexIndex, minVertexIndex, numVertices, finalStartIndex, triangleCount);
#else
    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dDeviceContext->DrawIndexed(triangleCount * 3, finalStartIndex, baseVertexIndex);
#endif

    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += triangleCount;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;

    // Reset overridden blend op if necessary
    if (!CV_r_measureoverdraw)
      SF_SetBlendOp(params.blendOp, true);
  }
  {
    //FRAME_PROFILER_FAST("SF_DITL::FXEnd", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // End shader pass
    pSFShader->FXEndPass();
    pSFShader->FXEnd();
  }

	if( params.renderMaskedStates & GS_COLMASK_NONE )
		rTI.m_PersFlags2 &= ~RBPF2_DISABLECOLORWRITES;


  m_pSFDrawParams = 0;
}

//////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::SF_DrawLineStrip( int baseVertexIndex, int lineCount, const SSF_GlobalDrawParams& params )
{
  FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

  if (IsDeviceLost())
    return;

  assert(params.vertexFmt == SSF_GlobalDrawParams::Vertex_XY16i);

	SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
	SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

  const SSF_ResourcesD3D& sfRes(SF_GetResources());
  CShader* pSFShader(0);
  {
    //FRAME_PROFILER_FAST("SF_DLS::SetShader", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Set shader
    switch(params.fillType)
    {
    case SSF_GlobalDrawParams::SolidColor:
      pSFShader = SF_SetTechnique(sfRes.m_shTech_SolidColor);
      break;
    default:
      assert(0);
      break;
    }		
  }

  if (!pSFShader)
    return;

  {
    //FRAME_PROFILER_FAST("SF_DLS::FxBegin", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

		if( params.renderMaskedStates & GS_COLMASK_NONE )
			rTI.m_PersFlags2 |= RBPF2_DISABLECOLORWRITES;

    m_pSFDrawParams = &params;
    uint32 numPasses(0);
    pSFShader->FXBegin(&numPasses, /*FEF_DONTSETTEXTURES |*/ FEF_DONTSETSTATES);
    if (!numPasses)
    {
      m_pSFDrawParams = 0;
      return;	
    }
    pSFShader->FXBeginPass(0);
  }
  {
    //FRAME_PROFILER_FAST("SF_DLS::SetState", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Set states
    EF_SetState(SF_AdjustBlendStateForMeasureOverdraw(params.blendModeStates) | /*GS_NODEPTHTEST | */params.renderMaskedStates);
    D3DSetCull(eCULL_None);
  }
  {
    //FRAME_PROFILER_FAST("SF_DLS::FX_Commit", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Commit all render changes
    FX_Commit();
  }
  {
    //FRAME_PROFILER_FAST("SF_DLS::SetVertexDecl", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Set vertex declaration
    if (!SF_SetVertexDeclaration(params.vertexFmt))
    {
      pSFShader->FXEndPass();
      pSFShader->FXEnd();
      m_pSFDrawParams = 0;
      return;
    }
  }

  // Copy vertex data...
  {
    //FRAME_PROFILER_FAST("SF_DLS::DynVBUpdate", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    if (params.pVertexPtr)
    {
      //FRAME_PROFILER_FAST("SF_DLS::CopyVerts", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

      size_t vertexSize(-1);
      switch(params.vertexFmt)
      {
      case SSF_GlobalDrawParams::Vertex_XY16i:
        vertexSize = 4;
        break;
      default:
        assert(0);
        break;
      }

      uint32 reqBufferSize(vertexSize * params.numVertices);
      uint32 bufferOffset(-1);
      void* pVB(FX_LockVB(reqBufferSize, bufferOffset));
      memcpy(pVB, params.pVertexPtr, reqBufferSize);
      FX_UnlockVB();
      m_RP.m_VBs[m_RP.m_CurVB].VBPtr_0->Bind(0, bufferOffset, vertexSize);
    }
  }
  {
    //FRAME_PROFILER_FAST("SF_DLS::BlendStateAndDraw", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Override blend op if necessary
		if (!CV_r_measureoverdraw)
      SF_SetBlendOp(params.blendOp);

    // Submit draw call
#if !defined(DIRECT3D10)
    m_pd3dDevice->DrawPrimitive(D3DPT_LINESTRIP, baseVertexIndex, lineCount);
#else
    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    m_pd3dDeviceContext->Draw(params.numVertices, baseVertexIndex);
#endif

    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += lineCount;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;

    // Reset overridden blend op if necessary
    if (!CV_r_measureoverdraw)
      SF_SetBlendOp(params.blendOp, true);
  }
  {
    //FRAME_PROFILER_FAST("SF_DLS::FXEnd", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // End shader pass
    pSFShader->FXEndPass();
    pSFShader->FXEnd();
  }


	if( params.renderMaskedStates & GS_COLMASK_NONE )
		rTI.m_PersFlags2 &= ~RBPF2_DISABLECOLORWRITES;

  m_pSFDrawParams = 0;
}

//////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::SF_DrawGlyphClear( const SSF_GlobalDrawParams& params )
{
  FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

  if (IsDeviceLost())
    return;

	SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
	SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

  assert(params.vertexFmt == SSF_GlobalDrawParams::Vertex_Glyph || params.vertexFmt == SSF_GlobalDrawParams::Vertex_XY16i);
  assert(params.pVertexPtr);

  const SSF_ResourcesD3D& sfRes(SF_GetResources());
  CShader* pSFShader(0);
  {
    //FRAME_PROFILER_FAST("SF_DG::SetShader", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Set shader
    switch(params.fillType)
    {
    case SSF_GlobalDrawParams::Glyph:
      pSFShader = SF_SetTechnique(sfRes.m_shTech_Glyph);
      break;
    case SSF_GlobalDrawParams::SolidColor:
      pSFShader = SF_SetTechnique(sfRes.m_shTech_SolidColor);
      break;
    default:
      assert(0);
      break;
    }		
  }

  if (!pSFShader)
    return;

  {
    //FRAME_PROFILER_FAST("SF_DG::FxBegin", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);


		if( params.renderMaskedStates & GS_COLMASK_NONE )
			rTI.m_PersFlags2 |= RBPF2_DISABLECOLORWRITES;

    m_pSFDrawParams = &params;
    uint32 numPasses(0);
    pSFShader->FXBegin(&numPasses, /*FEF_DONTSETTEXTURES |*/ FEF_DONTSETSTATES);
    if (!numPasses)
    {
      m_pSFDrawParams = 0;
      return;	
    }
    pSFShader->FXBeginPass(0);
  }
  {
    //FRAME_PROFILER_FAST("SF_DG::SetState", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Set states
    EF_SetState(SF_AdjustBlendStateForMeasureOverdraw(params.blendModeStates) | /*GS_NODEPTHTEST | */params.renderMaskedStates);
    D3DSetCull(eCULL_None);
  }
  {
    //FRAME_PROFILER_FAST("SF_DG::FX_Commit", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Commit all render changes
    FX_Commit();
  }
  {
    //FRAME_PROFILER_FAST("SF_DG::SetVertexDecl", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Set vertex declaration
    if (!SF_SetVertexDeclaration(params.vertexFmt))
    {
      pSFShader->FXEndPass();
      pSFShader->FXEnd();
      m_pSFDrawParams = 0;
      return;
    }
  }

  // Copy vertex data...
  {
    //FRAME_PROFILER_FAST("SF_DG::DynVBUpdate", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    size_t vertexSize(-1);
    switch(params.vertexFmt)
    {
    case SSF_GlobalDrawParams::Vertex_XY16i:
      vertexSize = 4;
      break;
    case SSF_GlobalDrawParams::Vertex_Glyph:
      vertexSize = 20;
      break;
    default:
      assert(0);
      break;
    }

    uint32 reqBufferSize(vertexSize * params.numVertices);
    uint32 bufferOffset(-1);
    void* pVB(FX_LockVB(reqBufferSize, bufferOffset));
    memcpy(pVB, params.pVertexPtr, reqBufferSize);
    FX_UnlockVB();
    m_RP.m_VBs[m_RP.m_CurVB].VBPtr_0->Bind(0, bufferOffset, vertexSize);
  }
  {
    //FRAME_PROFILER_FAST("SF_DG::BlendStateAndDraw", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // Override blend op if necessary
		if (!CV_r_measureoverdraw)
      SF_SetBlendOp(params.blendOp);

    // Submit draw call
#if !defined(DIRECT3D10)
    m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, params.numVertices - 2);
#else
    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pd3dDeviceContext->Draw(params.numVertices, 0);
#endif

    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += params.numVertices - 2;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;

    // Reset overridden blend op if necessary
    if (!CV_r_measureoverdraw)
      SF_SetBlendOp(params.blendOp, true);
  }
  {
    //FRAME_PROFILER_FAST("SF_DG::FXEnd", gEnv->pSystem, PROFILE_SYSTEM, g_bProfilerEnabled);

    // End shader pass
    pSFShader->FXEndPass();
    pSFShader->FXEnd();
  }


	if( params.renderMaskedStates & GS_COLMASK_NONE )
		rTI.m_PersFlags2 &= ~RBPF2_DISABLECOLORWRITES;

  m_pSFDrawParams = 0;
}

//////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::SF_Flush()
{
  if (IsDeviceLost())
    return;

  HRESULT hr(S_OK);

#if !defined(DIRECT3D10)
  SSF_ResourcesD3D& sfRes(SF_GetResources());
  if (!sfRes.m_pQuery)
    hr = m_pd3dDevice->CreateQuery(D3DQUERYTYPE_EVENT, &sfRes.m_pQuery);

#else
  SSF_ResourcesD3D& sfRes(SF_GetResources());
  if (!sfRes.m_pQuery)
  {
    D3D11_QUERY_DESC desc;
    desc.Query = D3D11_QUERY_EVENT;
    desc.MiscFlags = 0;
    hr = m_pd3dDevice->CreateQuery(&desc, &sfRes.m_pQuery);
  }
#endif

  if (sfRes.m_pQuery)
  {
		BOOL data(FALSE);
#if !defined(DIRECT3D10)
    hr = sfRes.m_pQuery->Issue(D3DISSUE_END);
		while (S_FALSE == (hr = sfRes.m_pQuery->GetData(&data, sizeof(data), D3DGETDATA_FLUSH)));		
#else
		m_pd3dDeviceContext->End(sfRes.m_pQuery);
		while (S_FALSE == (hr = m_pd3dDeviceContext->GetData(sfRes.m_pQuery, &data, sizeof(data), 0)));		
#endif
  }
}

//////////////////////////////////////////////////////////////////////////
bool CD3D9Renderer::SF_UpdateTexture(int texId, int mipLevel, int numRects, const SUpdateRect* pRects, unsigned char* pData, size_t pitch, ETEX_Format eTF)
{
  FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

  assert(texId > 0 && numRects > 0 && pRects != 0 && pData != 0 && pitch > 0);

  CTexture* pTexture(CTexture::GetByID(texId));
  assert(pTexture);

  if (pTexture->GetDstFormat() != eTF || pTexture->GetTextureType() != eTT_2D)
  {
    assert(0);
    return false;
  }

#if defined(DIRECT3D9)
  CDeviceTexture* pTex = pTexture->GetDevTexture();
  if (!pTex)
    return false;































  // get surface of given texture mip level 
  D3DSurface* pSurf = pTexture->GetSurface(-1, mipLevel);
  if (!pSurf)
    return false;

  // build union of all rectangle in the destination surfaces
  RECT rc = {pRects[0].dstX, pRects[0].dstY, pRects[0].dstX + pRects[0].width, pRects[0].dstY + pRects[0].height};
  for (int i(1); i<numRects; ++i)
  {
    rc.left = min(rc.left, (LONG)pRects[i].dstX);
    rc.top = min(rc.top, (LONG)pRects[i].dstY);
    rc.right = max(rc.right, (LONG)(pRects[i].dstX + pRects[i].width));
    rc.bottom = max(rc.bottom, (LONG)(pRects[i].dstY + pRects[i].height));
  }

  // lock surface and update all rects
  bool successful(false);
  D3DLOCKED_RECT lr;
  if (SUCCEEDED(pSurf->LockRect(&lr, &rc, 0)))
  {
    for (int i(0); i<numRects; ++i)
    {
      int sizePixel(CTexture::BitsPerPixel(eTF) >> 3);
      int sizeLine(sizePixel * pRects[i].width);

      const unsigned char* pSrc(&pData[pRects[i].srcY * pitch + sizePixel * pRects[i].srcX]);
      unsigned char* pDst(&(((unsigned char*) lr.pBits)[(pRects[i].dstY - rc.top) * lr.Pitch + sizePixel * (pRects[i].dstX - rc.left)]));

      for (int y(0); y<pRects[i].height; ++y)
      {
        memcpy(pDst, pSrc, sizeLine);
        pSrc += pitch;
        pDst += lr.Pitch;
      }
    }
    pSurf->UnlockRect();
    successful = true;
  }
  SAFE_RELEASE(pSurf);
  return successful;

#elif defined(DIRECT3D10)
  CDeviceTexture* pTex = pTexture->GetDevTexture();
  if (!pTex)
    return false;

  for (int i(0); i<numRects; ++i)
  {
    int sizePixel(CTexture::BitsPerPixel(eTF) >> 3);
    const unsigned char* pSrc(&pData[pRects[i].srcY * pitch + sizePixel * pRects[i].srcX]);

    D3D11_BOX box = {pRects[i].dstX, pRects[i].dstY, 0, pRects[i].dstX + pRects[i].width, pRects[i].dstY + pRects[i].height, 1};
    m_pd3dDeviceContext->UpdateSubresource(pTex->Get2DTexture(), mipLevel, &box, pSrc, (unsigned int) pitch, 0);
  }
  return true;
#else
  assert(!"CD3D9Renderer::SF_UpdateTexture() - Not implemented for this platform!");
  return false;
#endif	
}

#endif //EXCLUDE_SCALEFORM_SDK

//========================================================================================================

bool CRenderer::FX_TryToMerge(CRenderObject *pObjN, CRenderObject *pObjO, CRendElementBase *pRE)
{
  if (CV_e_DebugTexelDensity != 0)
	return false;

  if (!m_RP.m_pRE || m_RP.m_pRE != pRE || pRE->mfGetType() != eDATA_Mesh)
    return false;

  // Batching/Instancing case
  if ((pObjN->m_ObjFlags ^ pObjO->m_ObjFlags) & FOB_MASK_AFFECTS_MERGING)
    return false;

  if ((pObjN->m_ObjFlags | pObjO->m_ObjFlags) & FOB_CHARACTER)
    return false;

  if (pObjN->m_DynLMMask[0] != pObjO->m_DynLMMask[0])
    return false;

  if (pObjN->m_nMaterialLayers != pObjO->m_nMaterialLayers)
    return false;

  if (pObjN->m_nTextureID != pObjO->m_nTextureID)
    return false;

  if ((INT_PTR)pObjN->m_pShadowCasters != (INT_PTR)pObjO->m_pShadowCasters)
    return false;
  m_RP.m_ObjFlags |= pObjN->m_ObjFlags & FOB_SELECTED;
  if (m_RP.m_fMinDistance > pObjN->m_fDistance)
    m_RP.m_fMinDistance = pObjN->m_fDistance;
  //if (pObjN->GetInstanceInfo(m_RP.m_nProcessThreadID))
  //  m_RP.m_FlagsPerFlush |= RBSI_INSTANCED;
  return true;
}

// Note: If you add any new technique, update this list with it's name, in correct order (used for debug output)
static char *sDescList[] = 
{ 
  "NULL", 
  "Preprocess", 
  "General", 
  "TerrainLayer", 
  "Decal", 
  "WaterVolume", 
  "Transparent", 
  "Water", 
	"HDRPostProcess", 
	"AfterHDRPostProcess", 
  "PostProcess", 
  "AfterPostprocess",
  "ShadowGen",
  "ShadowPass",
  "RefractPass",
	"DeferredPreprocess"
//	"DeferredSkin",
  //"DeferredWaterParticles"
};

static char *sBatchList[] = 
{ 
  "FB_GENERAL", 
  "FB_TRANSPARENT", 
  "FB_DETAIL", 
	"FB_ZPREPASS", 
  "FB_Z", 
  "FB_GLOW", 
  "FB_SCATTER", 
  "FB_PREPROCESS", 
  "FB_MOTIONBLUR", 
  "FB_REFRACTIVE",
  "FB_MULTILAYERS",
  "FB_CAUSTICS",
  "FB_CUSTOM_RENDER",
  "FB_RAIN",
  "FB_FUR"
	"FB_DEBUG"
	//"FB_DEFERRED_SKIN_DIFFUSION"
};

// Process render items list [nList] from [nums] to [nume]
// 1. Sorting of the list
// 2. Preprocess shaders handling
// 3. Process sorted ordered list of render items

void CD3D9Renderer::EF_SortRenderList(int nList, int nAW, SRenderListDesc *pRLD, int nThread)
{
  int nStart = pRLD->m_nStartRI[nAW][nList];
  int nEnd   = pRLD->m_nEndRI[nAW][nList];
  int n = nEnd - nStart;
  if (!n)
    return;

  switch (nList)
  {    
  case EFSLIST_PREPROCESS:
    {
      PROFILE_FRAME(State_SortingPre);
      SRendItem::mfSortPreprocess(&SRendItem::RendItems(nThread,nAW,nList)[nStart], n);
    }
    break;
  
  case EFSLIST_DEFERRED_PREPROCESS:    
  case EFSLIST_HDRPOSTPROCESS:    
  case EFSLIST_POSTPROCESS:    
    {
      PROFILE_FRAME(State_SortingPost);
      // Don't sort post-process!!!
      //SRendItem::mfSortPreprocess(&SRendItem::RendItems(nAW,nList,nStart), n, nList, nAW);
    }
    break;
  case EFSLIST_WATER_VOLUMES:
  case EFSLIST_TRANSP:
  case EFSLIST_WATER:
  case EFSLIST_REFRACTPASS:
//  case EFSLIST_WATERPARTICLES:
    {
      PROFILE_FRAME(State_SortingDist); 
      SRendItem::mfSortByDist(&SRendItem::RendItems(nThread,nAW,nList)[nStart], n, false);
    }
    break;
  case EFSLIST_DECAL:
    {
      PROFILE_FRAME(State_SortingDecals); 
      SRendItem::mfSortByDist(&SRendItem::RendItems(nThread,nAW,nList)[nStart], n, true);
    }
    break;

  case EFSLIST_SHADOW_GEN:
  case EFSLIST_GENERAL:
  case EFSLIST_AFTER_POSTPROCESS:
  case EFSLIST_AFTER_HDRPOSTPROCESS:
    {
      PROFILE_FRAME(State_SortingLight);
#if defined(RENDITEM_SORT_STABLE)
      //this leads to front 2 back rendering within the batched drawcalls
      SRendItem::mfSortByDist(&SRendItem::RendItems(nThread,nAW,nList)[nStart], n, false, true);
#endif
      SRendItem::mfSortByLight(&SRendItem::RendItems(nThread,nAW,nList)[nStart], n, true, false, nList == EFSLIST_DECAL);
    }
    break;
  case EFSLIST_TERRAINLAYER:
    {
      PROFILE_FRAME(State_SortingLight_TerrainLayers);
      SRendItem::mfSortByLight(&SRendItem::RendItems(nThread,nAW,nList)[nStart], n, true, true, false);
    }
    break;

  default:
    assert(0);
  }
}

void CD3D9Renderer::EF_SortRenderLists(SRenderListDesc *pRLD, int nThreadID)
{
  PROFILE_FRAME(Sort_Lists);

  int i, j;
  for (j=0; j<MAX_LIST_ORDER; j++)
  {
    for (i=1; i<EFSLIST_NUM; i++)
    {
      EF_SortRenderList(i, j, pRLD, nThreadID);
    }
  }
}

// Init states before rendering of the scene
void CD3D9Renderer::FX_PreRender(int Stage)
{
  uint32 i;

  if (Stage & 1)
  { // Before preprocess
    m_RP.m_pSunLight = NULL;

    m_RP.m_Flags = 0;
    m_RP.m_pPrevObject = NULL;
    m_RP.m_FrameObject++;

    RT_SetCameraInfo();

    for (i=0; i<m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1].Num(); i++)
    {
      CDLight *dl = &m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][i];
      if (dl->m_Flags & DLF_FAKE)
        continue;

      if (dl->m_Flags & DLF_SUN)
        m_RP.m_pSunLight = dl;
    }
  }

  CHWShader_D3D::mfSetGlobalParams();
  m_RP.m_DynLMask = 0;
  m_RP.m_PrevLMask = -1;
  FX_PushVP();
}

// Restore states after rendering of the scene
void CD3D9Renderer::FX_PostRender()
{
  //FrameProfiler f("CD3D9Renderer:EF_PostRender", iSystem );

  FX_ObjectChange(NULL, NULL, m_RP.m_Objects[0], NULL);
  m_RP.m_pRE = NULL;

  FX_ResetPipe();

  if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_SETCLIPPLANE)
  {
    FX_SetClipPlane(false, NULL, false);
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_SETCLIPPLANE;
  }
  FX_PopVP();

  m_RP.m_FlagsShader_MD = 0;
  m_RP.m_FlagsShader_MDV = 0;
  m_RP.m_FlagsShader_LT = 0;
  m_RP.m_pCurObject = m_RP.m_Objects[0];
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
  m_RP.m_PrevLMask = -1;
}


// Object changing handling (skinning, shadow maps updating, initial states setting, ...)
bool CD3D9Renderer::FX_ObjectChange(CShader *Shader, SRenderShaderResources *Res, CRenderObject *obj, CRendElementBase *pRE)
{
  if ((obj->m_ObjFlags & FOB_NEAREST) && ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_DONTDRAWNEAREST) || CV_r_nodrawnear))
    return false;

  if (Shader)
  {
    if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_pIgnoreObject && m_RP.m_TI[m_RP.m_nProcessThreadID].m_pIgnoreObject->m_pRenderNode == obj->m_pRenderNode)
      return false;
  }

  if (obj == m_RP.m_pPrevObject)
    return true;

  m_RP.m_FrameObject++;

  m_RP.m_pCurObject = obj;

  int flags = 0;
  if (obj->m_Id) // Non-default object
  {
    if (obj->m_ObjFlags & FOB_NEAREST)
      flags |= RBF_NEAREST;

    if (m_drawNearFov>0 && (flags ^ m_RP.m_Flags) & RBF_NEAREST && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
    {
      if (flags & RBF_NEAREST)
      {
        CCamera Cam = m_RP.m_TI[m_RP.m_nProcessThreadID].m_cam;
        m_RP.m_PrevCamera = Cam;
        if (m_LogFile)
          Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Prepare nearest Z range ***\n");
        // set nice fov for weapons

        float fFov = Cam.GetFov();
        if (m_drawNearFov>1.0f && m_drawNearFov<179.0f)
          fFov = DEG2RAD(m_drawNearFov);

        Cam.SetFrustum(Cam.GetViewSurfaceX(), Cam.GetViewSurfaceZ(), fFov, DRAW_NEAREST_MIN, CV_r_DrawNearFarPlane);

        SetCamera(Cam);
        m_NewViewport.fMaxZ = CV_r_DrawNearZRange;
        m_bViewportDirty = true;
        m_RP.m_Flags |= RBF_NEAREST;
      }
      else
      {
        if (m_LogFile)
          Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Restore Z range ***\n");

        SetCamera(m_RP.m_PrevCamera);
        m_NewViewport.fMaxZ = m_RP.m_PrevCamera.GetZRangeMax();
        m_bViewportDirty = true;
        m_RP.m_Flags &= ~RBF_NEAREST;
      }
    }

    if(obj->m_ObjFlags & FOB_CUSTOM_CAMERA)
      flags |= RBF_CUSTOM_CAMERA;

    if ((flags ^ m_RP.m_Flags) & RBF_CUSTOM_CAMERA)
    {
      if (flags & RBF_CUSTOM_CAMERA)
      {
        CCamera Cam = GetCamera();
        m_RP.m_PrevCamera = Cam;
        if (m_LogFile)
          Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Prepare custom camera ***\n");
        CCamera *pCam = obj->m_pCustomCamera;
        /*if (!pCam)
        {
        // Find camera in shader public params.
        for (unsigned int i = 0; i < obj->m_ShaderParams->size(); i++)
        {
        SShaderParam &sp = (*obj->m_ShaderParams)[i];
        if (sp.m_Type == eType_CAMERA)
        {
        pCam = sp.m_Value.m_pCamera;
        break;
        }
        }
        }*/
        if (pCam)
        {
          SetCamera(*pCam);
          m_NewViewport.fMinZ = pCam->GetZRangeMin();
          m_NewViewport.fMaxZ = pCam->GetZRangeMax();
          m_bViewportDirty = true;
        }
        m_RP.m_Flags |= RBF_CUSTOM_CAMERA;
      }
      else
      {
        if (m_LogFile)
          Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Restore camera ***\n");

        SetCamera(m_RP.m_PrevCamera);
        if (m_NewViewport.fMinZ != m_RP.m_PrevCamera.GetZRangeMin() || m_NewViewport.fMaxZ != m_RP.m_PrevCamera.GetZRangeMax())
        {
          m_NewViewport.fMinZ = m_RP.m_PrevCamera.GetZRangeMin();
          m_NewViewport.fMaxZ = m_RP.m_PrevCamera.GetZRangeMax();
          m_bViewportDirty = true;
        }
        m_RP.m_Flags &= ~RBF_CUSTOM_CAMERA;
      }
    }
  }
  else
  {
    if (m_RP.m_Flags & (RBF_NEAREST | RBF_CUSTOM_CAMERA))
    {
      if (m_LogFile)
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Restore Z range/camera ***\n");
      SetCamera(m_RP.m_PrevCamera);
      m_NewViewport.fMaxZ = 1.0f;
      m_bViewportDirty = true;
      m_RP.m_Flags &= ~(RBF_NEAREST | RBF_CUSTOM_CAMERA);
    }
    m_ViewMatrix = m_CameraMatrix;
    // Restore transform
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadMatrix(&m_CameraMatrix);
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_MATRIXDIRTY;
  }
  m_RP.m_fMinDistance = obj->m_fDistance;
  m_RP.m_pPrevObject = obj;

  return true;
}

//=================================================================================
// Check buffer overflow during geometry batching
void CRenderer::FX_CheckOverflow(int nVerts, int nInds, CRendElementBase *re, int* nNewVerts, int* nNewInds)
{
  if (nNewVerts)
    *nNewVerts = nVerts;
  if (nNewInds)
    *nNewInds = nInds;

  if (m_RP.m_pRE || (m_RP.m_RendNumVerts+nVerts >= m_RP.m_MaxVerts || m_RP.m_RendNumIndices+nInds >= m_RP.m_MaxTris*3))
  {
    m_RP.m_pRenderFunc();
    if (nVerts >= m_RP.m_MaxVerts)
    {
      // iLog->Log("CD3D9Renderer::EF_CheckOverflow: numVerts >= MAX (%d > %d)\n", nVerts, m_RP.m_MaxVerts);
			assert(nNewVerts);
      *nNewVerts = m_RP.m_MaxVerts;
    }
    if (nInds >= m_RP.m_MaxTris*3)
    {
      // iLog->Log("CD3D9Renderer::EF_CheckOverflow: numIndices >= MAX (%d > %d)\n", nInds, m_RP.m_MaxTris*3);
			assert(nNewInds);
      *nNewInds = m_RP.m_MaxTris*3;
    }
    FX_Start(m_RP.m_pShader, m_RP.m_nShaderTechnique, m_RP.m_pShaderResources, re);
    FX_StartMerging();
  }
}


// Start of the new shader pipeline (3D pipeline version)
void CRenderer::FX_Start(CShader *ef, int nTech, SRenderShaderResources *Res, CRendElementBase *re)
{
  assert(ef);

  if (!ef)		// should not be 0, check to prevent crash
    return;

#if defined(DO_RENDERSTATS) && defined(ENABLE_GPU_TIMERS)
  if (CV_r_stats == 8)
  {
    //end timer
    if (m_RP.m_pEventRE != NULL)
    {
      CREMesh* pReMesh = (CREMesh*)m_RP.m_pEventRE;
      CRenderMesh2* pCurRenderMesh = pReMesh->m_pRenderMesh;
      //const char* pReName = pReMesh->m_pRenderMesh->GetSourceName();
			IRenderMesh::SRenderMeshStat* pCurStat = &(pCurRenderMesh->m_meshStat[GpuTimerEvent::s_callbackIdx]);

	    //m_pd3dDevice->InsertCallback(D3DCALLBACK_IDLE, &CRenderMesh2::EndTimerCallback, reinterpret_cast<uint32>(pCurStat));
      RT_InsertGpuCallback(reinterpret_cast<uint32>(pCurStat), CRenderMesh2::EndTimerCallback);
    }

    //start timer
    if (re->m_Type == eDATA_Mesh)
    {
      CREMesh* pReMesh = (CREMesh*)re;
      CRenderMesh2* pCurRenderMesh = pReMesh->m_pRenderMesh;
      //const char* pReName = pReMesh->m_pRenderMesh->GetSourceName();

			IRenderMesh::SRenderMeshStat* pCurStat = &(pCurRenderMesh->m_meshStat[GpuTimerEvent::s_callbackIdx]);

      int nCurFrameID = GetFrameID(false);

      //reset timing for next frame
      if (pCurStat->nFrameID != nCurFrameID)
      {
        pCurStat->nFrameID = nCurFrameID;
        pCurStat->nTotalTime = 0;
        pCurStat->nBatchNumber = 0;
      }

	    //m_pd3dDevice->InsertCallback(D3DCALLBACK_IDLE, &CRenderMesh2::StartTimerCallback, reinterpret_cast<uint32>(pCurStat));
      RT_InsertGpuCallback(reinterpret_cast<uint32>(pCurStat), CRenderMesh2::StartTimerCallback);
      m_RP.m_pEventRE = re;
    }
    else
    {
      m_RP.m_pEventRE = NULL;
    }
  }
#endif
  

  m_RP.m_nNumRendPasses = 0;
  m_RP.m_FirstIndex = 0;
  m_RP.m_IndexOffset = 0;
  m_RP.m_FirstVertex = 0;
  m_RP.m_RendNumIndices = 0;
  m_RP.m_RendNumVerts = 0;
  m_RP.m_RendNumGroup = -1;
  m_RP.m_pShader = ef;
  m_RP.m_nShaderTechnique = nTech;
  m_RP.m_pShaderResources = Res;
  CTexture::s_pCurEnvTexture = NULL;
  m_RP.m_FlagsPerFlush = 0;

  m_RP.m_FlagsStreams_Decl = 0;
  m_RP.m_FlagsStreams_Stream = 0;
  m_RP.m_FlagsShader_RT = 0;
  m_RP.m_FlagsShader_MD = 0;
  m_RP.m_FlagsShader_MDV = 0;

  FX_ApplyShaderQuality(ef->m_eShaderType);

  if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_HDR_FP16) && !(m_RP.m_nBatchFilter & (FB_ZPREPASS|FB_Z)))
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_HDR_MODE];
  if (m_RP.m_FSAAData.Type)
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FSAA];

  m_RP.m_fCurOpacity = 1.0f;
  m_RP.m_CurVFormat = ef->m_eVertexFormat;
  m_RP.m_ObjFlags = m_RP.m_pCurObject->m_ObjFlags;
  m_RP.m_DynLMask = m_RP.m_pCurObject->m_DynLMMask[m_RP.m_nProcessThreadID];
  //if (m_RP.m_pCurObject->GetInstanceInfo(m_RP.m_nProcessThreadID))
  //  m_RP.m_FlagsPerFlush |= RBSI_INSTANCED;
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
  m_RP.m_RIs.SetUse(0);

  m_RP.m_pRE = NULL;
  m_RP.m_Frame++;
}

//==============================================================================================

static void sBatchFilter(uint32 nFilter, char *sFilt)
{
  sFilt[0] = 0;
  int n = 0;
  for (int i=0; i<sizeof(sBatchList) / sizeof(sBatchList[0]); i++)
  {
    if (nFilter & (1<<i))
    {
      if (n)
        strcat(sFilt, "|");
      strcat(sFilt, sBatchList[i]);
      n++;
    }
  }
}

void CD3D9Renderer::FX_StartBatching()
{
}

void CD3D9Renderer::FX_ProcessBatchesList(int nums, int nume, uint32 nBatchFilter)
{
  PROFILE_FRAME(ProcessBatchesList);

  int i;
  CShader *pShader, *pCurShader;
  SRenderShaderResources *pRes;
  CRenderObject *pObject, *pCurObject;
  int nTech;

  if (nume-nums == 0)
    return;

  FX_StartBatching();

  int nList = m_RP.m_nPassGroupID; 
  int nAW = m_RP.m_nSortGroupID;
  int nThreadID = m_RP.m_nProcessThreadID;
  TArray<SRendItem>& RESTRICT_REFERENCE RI = SRendItem::RendItems(nThreadID,nAW,nList);




  m_RP.m_nBatchFilter = nBatchFilter;
  m_RP.m_bNotFirstPass = false;

  uint32 oldVal = -1;
	pShader = NULL;
	pRes = NULL;
  pCurObject = NULL;
  pCurShader = NULL;
  bool bChangedShader;

#ifdef DO_RENDERLOG
  if (CV_r_log)
  {
    char sFilt[256];
    sBatchFilter(nBatchFilter, sFilt);
    Logv(SRendItem::m_RecurseLevel[nThreadID], "\n*** Start batch list %s (Filter: %s) (%s) ***\n", sDescList[nList], sFilt, nAW ? "After water" : "Before water");
  }
#endif

  bool bUseBatching = true; //(m_RP.m_pRenderFunc == FX_FlushShader);
  m_RP.m_nCurLightGroup = -1;

  CRendElementBase *pRE=0;
  SRendItem *ri=0;
  for (i=nums; i<nume; i++)
  {
#if (defined(PS3) || defined(XENON)) && !defined(_DEBUG)
    if (!(i & 3))
      PrefetchLine(&RI[i+4], 0);
#endif
    ri = &RI[i];
    if (!(ri->nBatchFlags & nBatchFilter))
      continue;
    pObject = ri->pObj;



    pRE = ri->Item;
    if (oldVal != ri->SortVal)
    {
      oldVal = ri->SortVal;
      SRendItem::mfGet(ri->SortVal, nTech, pShader, pRes);
      bChangedShader = true;
    }
    else
      bChangedShader = false;
    if (pObject != pCurObject)
    {
      if (!bChangedShader && bUseBatching)
      {
        if (FX_TryToMerge(pObject, pCurObject, pRE))
        {
          m_RP.m_RIs.AddElem(ri);
          continue;
        }
      }
      if (pCurShader)
      {
        m_RP.m_pRenderFunc();



        pCurShader = NULL;
        bChangedShader = true;
      }
      if (!FX_ObjectChange(pShader, pRes, pObject, pRE))
        continue;
      pCurObject = pObject;
    }




    if (bChangedShader)
    {
      if (pCurShader)
      {
        m_RP.m_pRenderFunc();      




      }




      pCurShader = pShader;
      FX_Start(pShader, nTech, pRes, pRE);
    }

    {
      //PROFILE_FRAME_TOTAL(Mesh_REPrepare);
      pRE->mfPrepare();
    }
    if (!m_RP.m_RIs.Num())
      m_RP.m_RIs.AddElem(ri);
  }
  if (pCurShader)
    m_RP.m_pRenderFunc();

#ifdef DO_RENDERLOG
  if (CV_r_log)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End batch list ***\n\n");
#endif
}

void CD3D9Renderer::FX_ProcessRenderList(int nums, int nume, int nList, int nAW, void (*RenderFunc)(), bool bLighting)
{
  if (nume-nums < 1)
    return;

	// todo: skip light mask generation for EF_SUPPORTSDEFERREDSHADING_FULL
  if (bLighting)
  {
    if (nList == EFSLIST_TRANSP)
      SRendItem::mfGenerateLightGroupsTransparent(&SRendItem::RendItems(m_RP.m_nProcessThreadID,nAW,nList)[nums], nume-nums);
    else
      SRendItem::mfGenerateLightGroupsOpaque(&SRendItem::RendItems(m_RP.m_nProcessThreadID,nAW,nList)[nums], nume-nums);
  }

  EF_PushMatrix();
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();

  m_RP.m_pRenderFunc = RenderFunc;

  m_RP.m_pCurObject = m_RP.m_Objects[0];
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
  m_RP.m_pPrevObject = m_RP.m_pCurObject;

  FX_PreRender(3);

  int nPrevGroup = m_RP.m_nPassGroupID;
  int nPrevGroup2 = m_RP.m_nPassGroupDIP;
  int nPrevSortGroupID = m_RP.m_nSortGroupID;

  m_RP.m_nPassGroupID = nList; 
  m_RP.m_nPassGroupDIP = nList; 
  m_RP.m_nSortGroupID = nAW;
  //if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN)
  // m_RP.m_nPassGroupDIP = EFSLIST_SHADOW_GEN;
  m_RP.m_Flags |= RBF_3D;

  if (bLighting)
  {
    if ((nList != EFSLIST_TRANSP) || CV_r_usealphablend)
      FX_ProcessLightGroups(nums, nume);
  }
  else
  {
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_LIGHTSTENCILCULL;

    FX_ProcessBatchesList(nums, nume, FB_GENERAL);
   // FX_ProcessPostGroups(nums, nume);
  }

  FX_PostRender();

  EF_PopMatrix();
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();

  m_RP.m_nPassGroupID = nPrevGroup;
  m_RP.m_nPassGroupDIP = nPrevGroup2;
  m_RP.m_nSortGroupID = nPrevSortGroupID;
}

//////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_ProcessLightsListForLightGroup(int nGroup, SRendLightGroup *pGr, int nOffsRI)
{
  uint32 j;

  m_RP.m_pRE = NULL;
  m_RP.m_pShader = NULL;
  m_RP.m_pCurTechnique = NULL;

  CShader *pShader, *pCurShader;
  SRenderShaderResources *pRes;
  CRenderObject *pObject, *pCurObject;
  int nTech;
  int nThreadID = m_RP.m_nProcessThreadID;

  uint32 oldVal = -1;
	pShader = NULL;
	pRes = NULL;
  pCurObject = NULL;
  pCurShader = NULL;
  bool bChanged;
  m_RP.m_TI[nThreadID].m_PersFlags2 |= RBPF2_DRAWLIGHTS;

  // stencil cull pre-passes
  if (CV_r_optimisedlightsetup == 3 && m_RP.m_nPassGroupID != EFSLIST_TRANSP && m_RP.m_nPassGroupID != EFSLIST_REFRACTPASS && nGroup < 0x4 && SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] == 1)
  {
    EF_ClearBuffers(FRT_CLEAR_STENCIL|FRT_CLEAR_IMMEDIATE, NULL, 1);
    m_RP.m_TI[nThreadID].m_PersFlags2 |= RBPF2_LIGHTSTENCILCULL;
    FX_StencilCullPassForLightGroup(nGroup);
  }
  else
  {
    m_RP.m_TI[nThreadID].m_PersFlags2 &= ~RBPF2_LIGHTSTENCILCULL;
  }
  uint32 nBatchFlags = m_RP.m_nBatchFilter;
  const int nAW = m_RP.m_nSortGroupID;

  TArray<SRendItem>& RESTRICT_REFERENCE RI = SRendItem::RendItems(nThreadID,nAW,m_RP.m_nPassGroupID);
  PrefetchLine(&RI[0], 0);

  int nPrevGroup = m_RP.m_nCurLightGroup;
  m_RP.m_nCurLightGroup = (nGroup == MAX_REND_LIGHT_GROUPS) ? -1 : nGroup;
  assert(pGr!=NULL);
  m_RP.m_pCurrentLightGroup = pGr;

  bool bNotFirstPass;
  const uint32 nSize = pGr->RendItemsLights.size();
  SRendItem *ri = NULL;
  for (j=0; j<nSize; j++)
  {
    uint32 nRI = pGr->RendItemsLights[j];
    bNotFirstPass = (nRI & 0x80000000) ? true : false;

    nRI = (nRI & 0xffff) + nOffsRI;
    ri = &RI[nRI];
    if (!(ri->nBatchFlags & nBatchFlags))
      continue;

    CRendElementBase *pRE = ri->Item;
    pObject = ri->pObj;



    if (oldVal != ri->SortVal)
    {
      SRendItem::mfGet(ri->SortVal, nTech, pShader, pRes);
      bChanged = true;
    }
    else
      bChanged = false;

    oldVal = ri->SortVal;
    if (pObject != pCurObject)
    {
      if (!bChanged)
      {
        if (FX_TryToMerge(pObject, pCurObject, pRE))
        {
          m_RP.m_RIs.AddElem(ri);
          continue;
        }
      }
      if (pCurShader)
      {
	        m_RP.m_pRenderFunc();



        pCurShader = NULL;
        bChanged = true;
      }
      if (!FX_ObjectChange(pShader, pRes, pObject, pRE))
      {
        oldVal = ~0;
        continue;
      }
      pCurObject = pObject;
    }




    if (bChanged)
    {
      if (pCurShader)
      {
					m_RP.m_pRenderFunc();






      }

      FX_Start(pShader, nTech, pRes, pRE);
      pCurShader = pShader;
      m_RP.m_nMaxPasses = (ri->ObjSort >> 8) & 0xff;
    }

    {
      //PROFILE_FRAME_TOTAL(Mesh_REPrepare);
      pRE->mfPrepare();
      m_RP.m_bNotFirstPass = bNotFirstPass;
    }
    if (!m_RP.m_RIs.Num())
      m_RP.m_RIs.AddElem(ri);
  }
  if (pCurShader)
    m_RP.m_pRenderFunc();

  m_RP.m_TI[nThreadID].m_PersFlags2 &= ~RBPF2_DRAWLIGHTS;
  m_RP.m_nCurLightGroup = nPrevGroup;

  return true;
}

void CD3D9Renderer::FX_ProcessRenderList(int nList, uint32 nBatchFilter)
{
  FX_PreRender(3);

  m_RP.m_pRenderFunc = FX_FlushShader_General;
  m_RP.m_nPassGroupID = nList;
  m_RP.m_nPassGroupDIP = nList;

  PROFILE_DIPS_START;

  m_RP.m_nSortGroupID = 0;
  FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], nBatchFilter);

  m_RP.m_nSortGroupID = 1;
  FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], nBatchFilter);

  PROFILE_DIPS_END(nList);

  FX_PostRender();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_ProcessZPrePassRenderLists()
{
	PROFILE_LABEL_SCOPE("Z-PREPASS");
	int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
	if (nR > 1)
		return;
	int nList = EFSLIST_GENERAL;

	uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_GENERAL, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
	if ( nBatchMask & FB_ZPREPASS )
	{
#ifdef DO_RENDERLOG
		if (CV_r_log)
			Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Start z-prepass ***\n");
#endif

		FX_PreRender(3);

		m_RP.m_pRenderFunc = FX_FlushShader_ZPass;
		FX_ZPrePassScene(true,!(m_RP.m_nRendFlags & SHDF_DO_NOT_CLEAR_Z_BUFFER));
		FX_EnableATOC();

		if ((nBatchMask & FB_ZPREPASS))
		{
			m_RP.m_nPassGroupID = nList;
			m_RP.m_nPassGroupDIP = nList;

			m_RP.m_nSortGroupID = 0;

			FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_ZPREPASS);
			m_RP.m_nSortGroupID = 1;
			FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_ZPREPASS);
		}

		// Reload zcull for terrain layers/decals passes
		FX_ZCullReload();

		FX_PostRender();
		FX_DisableATOC();

		FX_ZPrePassScene(false, false);

		m_RP.m_pRenderFunc = FX_FlushShader_General;

#ifdef DO_RENDERLOG
		if (CV_r_log)
			Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End z-prepass ***\n");
#endif
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_ProcessZPassRenderLists()
{
  PROFILE_LABEL_SCOPE("ZPASS");
  int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
  if (nR > 1)
    return;
  int nList = EFSLIST_GENERAL;

  uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_GENERAL, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
  uint32 nBatchMaskTransp = SRendItem::BatchFlags(EFSLIST_TRANSP, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
  uint32 nBatchMaskDecal = SRendItem::BatchFlags(EFSLIST_DECAL, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
  
  uint32 nBatchMaskTerrainLayer = 0;
  if( CV_r_deferredshading )
    nBatchMaskTerrainLayer = SRendItem::BatchFlags(EFSLIST_TERRAINLAYER, m_RP.m_nProcessThreadID, m_RP.m_pRLD);

  if ((nBatchMask|nBatchMaskTransp|nBatchMaskDecal|nBatchMaskTerrainLayer) & FB_Z)
  {
#ifdef DO_RENDERLOG
    if (CV_r_log)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Start z-pass ***\n");
#endif

    FX_PreRender(3);

    m_RP.m_pRenderFunc = FX_FlushShader_ZPass;
    FX_ZScene(true, m_RP.m_bUseHDR, !(m_RP.m_nRendFlags & SHDF_DO_NOT_CLEAR_Z_BUFFER));
    FX_EnableATOC();

    if (nBatchMask & FB_Z)
    {
      m_RP.m_nPassGroupID = nList;
      m_RP.m_nPassGroupDIP = nList;

      m_RP.m_nSortGroupID = 0;

      FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_Z);
      m_RP.m_nSortGroupID = 1;
      FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_Z);
    }

    if (nBatchMaskTransp & FB_Z)
    {
      // Needed in case user explicitly forces zpass for alpha blended stuff

      nList = EFSLIST_TRANSP;

      m_RP.m_nPassGroupID = nList;
      m_RP.m_nPassGroupDIP = nList;

      m_RP.m_nSortGroupID = 0;

      FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_Z);
      m_RP.m_nSortGroupID = 1;
      FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_Z);
    }

		// Reload zcull for terrain layers/decals passes
		FX_ZCullReload();

#if (defined (DIRECT3D9) || defined (DIRECT3D10)) && !defined( XENON ) && !defined( PS3 )
		// PC special case: render terrain/decals/roads normals separately - disable mrt rendering, on consoles we always use single rt for output

		FX_ZScene(false, m_RP.m_bUseHDR, false);

		FX_ZScene(true, m_RP.m_bUseHDR, false ,true );
#endif

    // Add terrain/roads/decals normals into normal render target also
    if ( nBatchMaskTerrainLayer & FB_Z )
    {
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_NOALPHABLEND;

      nList = EFSLIST_TERRAINLAYER;
      m_RP.m_nPassGroupID = nList;
      m_RP.m_nPassGroupDIP = nList;

      m_RP.m_nSortGroupID = 0;

      FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_Z);
      m_RP.m_nSortGroupID = 1;
      FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_Z);

      nList = EFSLIST_GENERAL;

      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NOALPHABLEND;
    }

    if ( nBatchMaskDecal & FB_Z )
    {
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_NOALPHABLEND;
      // Needed in case user explicitly forces zpass for decals
      nList = EFSLIST_DECAL;

      m_RP.m_nPassGroupID = nList;
      m_RP.m_nPassGroupDIP = nList;

      m_RP.m_nSortGroupID = 0;  

      FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_Z);
      m_RP.m_nSortGroupID = 1;
      FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_Z);
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NOALPHABLEND;
    }

    FX_PostRender();
    FX_DisableATOC();
    
#if (defined (DIRECT3D9) || defined (DIRECT3D10)) && !defined( XENON ) && !defined( PS3 )
		FX_ZScene(false, m_RP.m_bUseHDR, false ,true );
#else
		FX_ZScene(false, m_RP.m_bUseHDR, false);
#endif

    m_RP.m_pRenderFunc = FX_FlushShader_General;

#ifdef DO_RENDERLOG
    if (CV_r_log)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End z-pass ***\n");
#endif
  }
}

void CD3D9Renderer::FX_ProcessScatterRenderLists()
{
  PROFILE_LABEL_PUSH("Scattering List");
  int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
  if (nR > 1)
    return;
  int nList = EFSLIST_GENERAL;
  uint32 nBatchMask = SRendItem::BatchFlags(nList, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
  if (nBatchMask & FB_SCATTER)
  {
#ifdef DO_RENDERLOG
    if (CV_r_log)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Start scatter-pass ***\n");
#endif

    FX_PreRender(3);

    //////////////////////////////////////////////////////////////////////////
    CTexture *tpScatterLayer = NULL;
    SDynTexture *pDynScatterLayer = NULL;

    //workaround to improve half-res quality
    //int nWidth = max (GetWidth(),GetHeight());
    //int nHeight = nWidth;

    //TF: make pow2
    int nWidth = GetWidth()/2;
    int nHeight = GetHeight()/2;


    ETEX_Format etfScatLayer = m_FormatA16B16G16R16.IsValid()?eTF_A16B16G16R16F:eTF_A8R8G8B8;





#if !defined (DIRECT3D10) && !defined (XENON)
    pDynScatterLayer = new SDynTexture(nWidth, nHeight, etfScatLayer, eTT_2D,  FT_STATE_CLAMP, "TempScatterRT", 95);
    pDynScatterLayer->Update(nWidth, nHeight);
    tpScatterLayer = pDynScatterLayer->m_pTexture;
#endif

    CTexture::s_ptexScatterLayer->Invalidate(nWidth, nHeight, etfScatLayer);


    SD3DSurface* pSepDepthSurf = FX_GetDepthSurface(nWidth, nHeight, false);
    gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexScatterLayer, pSepDepthSurf);
    //gcpRendD3D->SetViewport(0, 0, nWidth, nHeight);
    ColorF clearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gcpRendD3D->EF_ClearBuffers(FRT_CLEAR, &clearColor);
    //FX_Commit();

    //////////////////////////////////////////////////////////////////////////

    m_RP.m_pRenderFunc = FX_FlushShader_General;
    m_RP.m_nPassGroupID = nList;
    m_RP.m_nPassGroupDIP = nList;

    //first depth prepass
    FX_ScatterScene(true, false);

    m_RP.m_nSortGroupID = 0;
    FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_SCATTER);

    m_RP.m_nSortGroupID = 1;
    FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_SCATTER);

    //EF_PostRender();

    // depth accumulation pass
    FX_ScatterScene(true, true);

    m_RP.m_nSortGroupID = 0;
    FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_SCATTER);

    m_RP.m_nSortGroupID = 1;
    FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_SCATTER);

    FX_ScatterScene(false, false);
  
    FX_PopRenderTarget(0);

    FX_PostRender();
    //////////////////////////////////////////////////////////////////////////
    //remove blur from FX_ScatterScene
    //and put proper blur here

    //gcpRendD3D->FX_ResetPipe(); it's been made in EF_PostRender


    //is it correct parameters
#if !defined (DIRECT3D10) && !defined (XENON)
    FX_ShadowBlur(0.1f, pDynScatterLayer, CTexture::s_ptexScatterLayer, 1);
    FX_ShadowBlur(0.1f, pDynScatterLayer, CTexture::s_ptexScatterLayer, 1);
#endif
    //////////////////////////////////////////////////////////////////////////


    SAFE_DELETE(pDynScatterLayer);

  #ifdef DO_RENDERLOG
      if (CV_r_log)
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End scatter-pass ***\n");
  #endif
  }
  PROFILE_LABEL_POP("Scattering List");
}

/* deferred skin research
void CD3D9Renderer::FX_ProcessDeferredSkinDiffusionRenderLists(int nList, int nAW, void (*RenderFunc)())
{
	int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
	if ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && nR <= 1)
	{
		uint32 nBatchMask = SRendItem::BatchFlags(nList, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
		if (nBatchMask & FB_DEFERRED_SKIN_DIFFUSION)
		{
			FX_DeferredSkinDiffusionScene(true);
			FX_ProcessRenderList(EFSLIST_GENERAL, FB_DEFERRED_SKIN_DIFFUSION);
			FX_DeferredSkinDiffusionScene(false);
		}
	}
}*/

void CD3D9Renderer::FX_ProcessRainRenderLists(int nList, int nAW, void (*RenderFunc)())
{
  int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
  if ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && nR <= 1)
  {
    uint32 nBatchMask = SRendItem::BatchFlags(nList, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
    if (nBatchMask & FB_RAIN)
    {
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_RAINPASS;

      FX_PreRender(3);

      short nLightGroupPrev = m_RP.m_nCurLightGroup;
      m_RP.m_nCurLightGroup = 0;

      m_RP.m_nPassGroupID = nList; 
      m_RP.m_nPassGroupDIP = nList; 
      m_RP.m_nSortGroupID = nAW;

      FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_RAIN);

      m_RP.m_nCurLightGroup = nLightGroupPrev;
      
      FX_PostRender();

      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_RAINPASS;
    }
  }
}

void CD3D9Renderer::FX_ProcessFurRenderLists(int nList, int nAW, void (*RenderFunc)())
{
  int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
  if ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && nR <= 1)
  {
    uint32 nBatchMask = SRendItem::BatchFlags(nList, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
    if (nBatchMask & FB_FUR)
    {
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_FURPASS;

      FX_PreRender(3);

      short nLightGroupPrev = m_RP.m_nCurLightGroup;
      m_RP.m_nCurLightGroup = 0;

      m_RP.m_nPassGroupID = nList; 
      m_RP.m_nPassGroupDIP = nList; 
      m_RP.m_nSortGroupID = nAW;

      FX_ProcessBatchesList(m_RP.m_pRLD->m_nStartRI[m_RP.m_nSortGroupID][nList], m_RP.m_pRLD->m_nEndRI[m_RP.m_nSortGroupID][nList], FB_FUR);

      m_RP.m_nCurLightGroup = nLightGroupPrev;

      FX_PostRender();

      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_FURPASS;
    }
  }
}


void CD3D9Renderer::FX_ProcessPostRenderLists(uint32 nBatchFilter)
{
  int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];
  if ((m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && nR <= 1)
  {
    int nList = EFSLIST_GENERAL;
    uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_GENERAL, m_RP.m_nProcessThreadID, m_RP.m_pRLD) | SRendItem::BatchFlags(EFSLIST_TRANSP, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
    nBatchMask |= SRendItem::BatchFlags(EFSLIST_DECAL, m_RP.m_nProcessThreadID, m_RP.m_pRLD);
    if (nBatchMask & nBatchFilter)
    {
      if (nBatchFilter == FB_GLOW)
        FX_GlowScene(true);
      else
      if (nBatchFilter == FB_MOTIONBLUR)
      {
        if( FX_MotionBlurScene(true) == false )
          return;
      }
      else
      if (nBatchFilter == FB_CUSTOM_RENDER)
        FX_CustomRenderScene(true);
      else
        assert(0);

      FX_ProcessRenderList(EFSLIST_GENERAL, nBatchFilter);
      FX_ProcessRenderList(EFSLIST_DECAL, nBatchFilter);
      FX_ProcessRenderList(EFSLIST_TRANSP, nBatchFilter);

      if (nBatchFilter == FB_GLOW)
        FX_GlowScene(false);
      else
      if (nBatchFilter == FB_MOTIONBLUR)
        FX_MotionBlurScene(false);
      else
      if (nBatchFilter == FB_CUSTOM_RENDER)
        FX_CustomRenderScene(false);
      else
        assert(0);
    }
  }
}

void CD3D9Renderer::FX_ProcessPostGroups(int nums, int nume)
{
  uint32 nBatchMask = m_RP.m_pRLD->m_nBatchFlags[m_RP.m_nSortGroupID][m_RP.m_nPassGroupID];
  if (nBatchMask & FB_MULTILAYERS && CV_r_usemateriallayers)
    FX_ProcessBatchesList(nums, nume, FB_MULTILAYERS);
	if ((nBatchMask & FB_DETAIL) && CV_r_detailtextures)
		FX_ProcessBatchesList(nums, nume, FB_DETAIL);
	if ((nBatchMask & FB_LAYER_EFFECT) && CV_r_detailtextures)
		FX_ProcessBatchesList(nums, nume, FB_LAYER_EFFECT);
  if ((nBatchMask & FB_CAUSTICS) && CV_r_watercaustics && gRenDev->m_RP.m_eQuality>0)
    FX_ProcessBatchesList(nums, nume, FB_CAUSTICS);
  if (0 != (nBatchMask & FB_DEBUG))
    FX_ProcessBatchesList(nums, nume, FB_DEBUG);
}

void CD3D9Renderer::FX_ProcessShadows(int nList, int nAW)
{
  FX_PreRender(3);

  int nums = m_RP.m_pRLD->m_nStartRI[nAW][nList];
  int nume = m_RP.m_pRLD->m_nEndRI[nAW][nList];

  m_RP.m_pRenderFunc = FX_FlushShader_General;

  m_RP.m_pCurObject = m_RP.m_Objects[0];
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
  m_RP.m_pPrevObject = m_RP.m_pCurObject;

  int nPrevGroup = m_RP.m_nPassGroupID;
  int nPrevGroup2 = m_RP.m_nPassGroupDIP;
  int nPrevSortGroupID = m_RP.m_nSortGroupID;

  m_RP.m_nPassGroupID = nList; 
  m_RP.m_nPassGroupDIP = nList; 
  m_RP.m_nSortGroupID = nAW;
  //if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN)
  //  m_RP.m_nPassGroupDIP = EFSLIST_SHADOW_GEN;
  m_RP.m_Flags |= RBF_3D;

  int nFrameID = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;
  //// update screen space ambient occlusion mask
  //if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_ALLOW_AO) && SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]==1 && nFrameID != m_RP.m_nAOMaskUpdateLastFrameId)
  //{
  //  FX_ProcessAOTarget();
  //  m_RP.m_FillLights[SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1].SetUse(0);
  //}

  if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
  {
    for (int i=0; i<MAX_REND_LIGHT_GROUPS; i++)
    {
      if (m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1].Num() <= (uint32)i*4)
        break;
      // First process all the shadows for the light group
      FX_ProcessShadowsListsForLightGroup(i, nums);
    }
  }
  m_RP.m_nPassGroupID = nPrevGroup;
  m_RP.m_nPassGroupDIP = nPrevGroup2;
  m_RP.m_nSortGroupID = nPrevSortGroupID;

  FX_PostRender();
}

void CD3D9Renderer::FX_ProcessLightGroups(int nums, int nume)
{
  uint32 i;
	int j;
  int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1;

#if !defined(XENON)
  //// update screen space ambient occlusion mask
  //if(	m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_ALLOW_AO && !nR && GetFrameID(false) != m_RP.m_nAOMaskUpdateLastFrameId )
  //{
  //  FX_ProcessAOTarget();
  //  m_RP.m_FillLights[nR].SetUse(0);
  //}

  if (m_RP.m_nPassGroupID != EFSLIST_DECAL && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
  {
    for (i=0; i<MAX_REND_LIGHT_GROUPS; i++)
    {
      if (m_RP.m_DLights[m_RP.m_nProcessThreadID][nR].Num() <= i*4)
        break;
      // First process all the shadows for the light group
      FX_ProcessShadowsListsForLightGroup(i, nums);
    }
  }
#endif

  m_RP.m_PrevLMask = -1;
  m_RP.m_nBatchFilter = FB_GENERAL;

  // Second draw light passes for light groups
  for (j=0; j<SRendItem::m_nSortGroups; j++)
  {    
    for (i=0; i<=MAX_REND_LIGHT_GROUPS; i++)
    {
      SRendLightGroup *pGr = &SRendItem::m_RenderLightGroups[j][i];
      if (pGr->RendItemsLights.size())
      {
        m_RP.m_pShader = NULL;
        m_RP.m_pCurTechnique = NULL;
        m_RP.m_pShaderResources = NULL;
        m_RP.m_pCurObject = m_RP.m_Objects[0];
        m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
        m_RP.m_pPrevObject = NULL;
        FX_ProcessLightsListForLightGroup(i, pGr, nums);        
      }
    }
  }
  FX_ProcessPostGroups(nums, nume);
}


void CD3D9Renderer::FX_ApplyThreadState(SThreadInfo& TI, SThreadInfo *pOldTI)
{
  if (pOldTI)
    *pOldTI = m_RP.m_TI[m_RP.m_nProcessThreadID];

  m_RP.m_TI[m_RP.m_nProcessThreadID] = TI;
}

void CD3D9Renderer::FX_ZCullReload()
{










































}

// Render thread only scene rendering
void CD3D9Renderer::RT_RenderScene(int nFlags, SThreadInfo& TI, void (*RenderFunc)(), SRenderListDesc* pRLD)
{
	PROFILE_LABEL_PUSH( "Scene" );

  if (CV_r_flush == 1)
    FlushHardware(false);

  m_RP.m_pRLD = pRLD;
  
  int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1;
  float fTime = iTimer->GetAsyncCurTime();

  SDynTexture *pDT = NULL;
  bool bFullScreen = true;
  int nX = pRLD->m_VP.nX;
  int nY = pRLD->m_VP.nY;
  int nWidth = pRLD->m_VP.nWidth;
  int nHeight = pRLD->m_VP.nHeight;
  if (!CV_r_SplitScreenActive)
  {
    nX = nY = 0;
    nWidth = m_width;
    nHeight = m_height;
  }
  if (nX || nY || nWidth!=m_width || nHeight!=m_height)
    bFullScreen = false;
  if (!nR)
  {
    m_MainViewport.nX = 0;
    m_MainViewport.nY = 0;
    m_MainViewport.nWidth = nWidth;
    m_MainViewport.nHeight = nHeight;
  }
  if (!bFullScreen)
  {
    pDT = new SDynTexture(nWidth, nHeight, eTF_A8R8G8B8, eTT_2D, 0, "$SplitScreen", 0);
    pDT->Update(nWidth, nHeight);
    pDT->SetRT(0, true, &m_DepthBufferOrigFSAA, true);
    RT_SetViewport(0, 0, nWidth, nHeight);
  }


  //if (!nR)
  //{
  //  memset(&gRenDev->m_RP.m_PS, 0, sizeof(SPipeStat));
  //  gRenDev->m_RP.m_RTStats.resize(0);
  //  gRenDev->m_RP.m_Profile.Free();
  //}

  //invalidate shadow masks
  for (int j=0; j<MAX_REND_LIGHT_GROUPS; j++)
  {
    SRendItem::m_ShadowsValidMask[SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][j] = 0;
  }
  float fTimeP;
  //shadow generation
  if (!nR && !(nFlags & SHDF_ZPASS_ONLY) ) //|SHDF_ALLOWPOSTPROCESS
	{




		PROFILE_LABEL_PUSH( "SHADOWMAP PASSES" );
    fTimeP = iTimer->GetAsyncCurTime();
    EF_PrepareAllDepthMaps();
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_SHADOW_GEN] = iTimer->GetAsyncCurTime() - fTimeP;
		PROFILE_LABEL_POP( "SHADOWMAP PASSES" );
	}

  int nSaveScissor = CV_r_scissor;
  int nSaveDrawNear = CV_r_nodrawnear;
  int nSaveDrawCaustics = CV_r_watercaustics;
  int nSaveStreamSync = CV_r_texturesstreamingsync;
  if (nFlags & SHDF_NO_DRAWCAUSTICS)
    CV_r_watercaustics = 0;
  if (nFlags & SHDF_NO_DRAWNEAR)
    CV_r_nodrawnear = 1;
  if (nFlags & SHDF_NO_SCISSOR)
    CV_r_scissor = 0;
  if (nFlags & SHDF_STREAM_SYNC)
    CV_r_texturesstreamingsync = 1;

  uint32 nSaveRendFlags = m_RP.m_nRendFlags;
  m_RP.m_nRendFlags = nFlags;
  FX_ApplyThreadState(TI, &m_RP.m_OldTI[nR]);
  if (m_pNewTarget[0])
  {
    if (nFlags & SHDF_DO_NOT_RENDER_TARGET)
      m_pNewTarget[0]->m_bDontDraw = true;
    else
      m_pNewTarget[0]->m_bDontDraw = false;
  }
  if (nFlags & SHDF_CLEAR_SHADOW_MASK)
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_CLEAR_SHADOW_MASK;

#ifdef USE_HDR

  bool bHDRRendering = (nFlags & SHDF_ALLOWHDR) && IsHDRModeEnabled() && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE);

	if( !IsHDRModeEnabled() )
	{
		m_vSceneLuminanceInfo = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_fAdaptedSceneScale = m_fAdaptedSceneScaleLBuffer = m_fStocopicSceneScale = 1.0f;				
	}

  if (!nR && bHDRRendering )
  {
    m_RP.m_bUseHDR = true;        
    bool bEncodedHDRTarget = false;



    if( FX_HDRScene(m_RP.m_bUseHDR, false, bEncodedHDRTarget) )  
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_HDR_FP16;
  }
  else
  {
    m_RP.m_bUseHDR = false;   
    FX_HDRScene(false);
    
    if( (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_DRAWTOTEXTURE) && bHDRRendering)
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_HDR_FP16;
    else
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_HDR_FP16;
  }      

#endif

  // Prepare post processing	
  bool bAllowPostProcess = (nFlags & SHDF_ALLOWPOSTPROCESS) && !nR && (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_POSTPROCESS) 
														&& (CV_r_PostProcess) && !CV_r_measureoverdraw &&													 
														!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE) && !CV_r_wireframe;  
  FX_PostProcessScene(bAllowPostProcess);
  bool bAllowDeferred = (nFlags & SHDF_ZPASS) && !nR && (CV_r_deferredshading) && !CV_r_measureoverdraw && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_MAKESPRITE));
  if( bAllowDeferred ) 
  {
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_ALLOW_DEFERREDSHADING;
    fTimeP = iTimer->GetAsyncCurTime();
    FX_DeferredRendering( false, true);
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_DEFERRED_PREPROCESS] = iTimer->GetAsyncCurTime() - fTimeP;
  }
  else
  {
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_ALLOW_DEFERREDSHADING;
		m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_DEFERRED_SHADING];
  }

  if (!nR && (nFlags & SHDF_ALLOWHDR) && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE))
  {
    ETEX_Format eTF = (m_RP.m_bUseHDR && m_nHDRType==1) ? eTF_A16B16G16R16F : eTF_A8R8G8B8;




    int nW = gcpRendD3D->GetWidth(); //m_d3dsdBackBuffem.Width;
    int nH = gcpRendD3D->GetHeight(); //m_d3dsdBackBuffem.Height;
    if (!CTexture::s_ptexSceneTarget || CTexture::s_ptexSceneTarget->IsFSAAChanged() || CTexture::s_ptexSceneTarget->GetDstFormat() != eTF || CTexture::s_ptexSceneTarget->GetWidth() != nW || CTexture::s_ptexSceneTarget->GetHeight() != nH )
      CTexture::GenerateSceneMap(eTF);
  }

  if(nFlags & SHDF_ALLOW_AO)
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_ALLOW_AO;
  else
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_ALLOW_AO;

#if !defined(XENON) && !defined(PS3)
  if (CV_r_shadersdynamicbranching)
    FX_PrepareLightInfoTexture(true);
  else
    FX_PrepareLightInfoTexture(false);
#endif

  if (!(nFlags & SHDF_ZPASS_ONLY)) 
  {
    bool bLighting = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN) == 0;
    if (!nFlags)
      bLighting = false;






/*
#ifdef XENON
    if (CV_d3d9_predicatedtiling && bLighting) //skip shadowgen
    {
      XE_ZScene(true);
    }
#endif
*/
    if ((nFlags & (SHDF_ALLOWHDR | SHDF_ALLOWPOSTPROCESS)) && CV_r_usezpass)
    {







			
      fTimeP = iTimer->GetAsyncCurTime();

			if (CV_r_ZPrePass)
			{
				PROFILE_LABEL_PUSH("Z-PREPASS");
				FX_ProcessZPrePassRenderLists();
				PROFILE_LABEL_POP("Z-PREPASS");
			}

			PROFILE_LABEL_PUSH("ZPASS");
      FX_ProcessZPassRenderLists();
			PROFILE_LABEL_POP("ZPASS");
 
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsZ = iTimer->GetAsyncCurTime() - fTimeP;
		}




/*
#if defined(XENON)
    if (CV_d3d9_predicatedtiling && bLighting) //skip shadowgen
    {
      XE_ZScene(false);
    }
#endif
*/

// temporary fix for CXP - moving shadowmask gen order to after deferred, to avoid stencil conflicts
//#ifdef XENON
//    FX_ProcessShadows(EFSLIST_GENERAL, 0);
//    FX_ProcessShadows(EFSLIST_GENERAL, 1);
//#endif

    // update screen space ambient occlusion mask
    if(	m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_ALLOW_AO && !nR /* && GetFrameID(false) != m_RP.m_nAOMaskUpdateLastFrameId */)
    {
      fTimeP = iTimer->GetAsyncCurTime();
      FX_ProcessAOTarget();
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsAO = iTimer->GetAsyncCurTime() - fTimeP;
    }

#ifndef XENON
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE))
      FX_ProcessScatterRenderLists();
#endif

    bool bEmpty = SRendItem::IsListEmpty(EFSLIST_GENERAL, m_RP.m_nProcessThreadID, pRLD);
    bEmpty &= SRendItem::IsListEmpty(EFSLIST_DEFERRED_PREPROCESS, m_RP.m_nProcessThreadID, pRLD);
    if (!nR && !bEmpty && m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_bEnable && CV_r_usezpass)
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NOSHADERFOG;

    if( bAllowDeferred && !bEmpty )
    {
      fTimeP = iTimer->GetAsyncCurTime();

      PROFILE_LABEL_PUSH("DEFERRED_PREPROCESS");
      FX_ProcessRenderList(EFSLIST_DEFERRED_PREPROCESS, 0, RenderFunc, false);       // Sorted list without preprocess of all deferred related passes and screen shaders
      FX_ProcessRenderList(EFSLIST_DEFERRED_PREPROCESS, 1, RenderFunc, false);       // Sorted list without preprocess of all deferred related passes and screen shaders
      PROFILE_LABEL_POP("DEFERRED_PREPROCESS");

      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_DEFERRED_PREPROCESS] += iTimer->GetAsyncCurTime() - fTimeP;
    }











    fTimeP = iTimer->GetAsyncCurTime();
		PROFILE_LABEL_PUSH("OPAQUE_PASSES");

    PROFILE_LABEL_PUSH("GENERAL_BW");
    FX_ProcessRenderList(EFSLIST_GENERAL, 0, RenderFunc, bLighting);         // Sorted list without preprocess
    PROFILE_LABEL_POP("GENERAL_BW");
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_GENERAL] += iTimer->GetAsyncCurTime() - fTimeP;

    fTimeP = iTimer->GetAsyncCurTime();
    PROFILE_LABEL_PUSH("TERRAINLAYERS_BW");
    FX_ProcessRenderList(EFSLIST_TERRAINLAYER, 0, RenderFunc, bLighting);    // Unsorted list without preprocess
    PROFILE_LABEL_POP("TERRAINLAYERS_BW");
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_TERRAINLAYER] += iTimer->GetAsyncCurTime() - fTimeP;

    fTimeP = iTimer->GetAsyncCurTime();
    PROFILE_LABEL_PUSH("GENERAL_AW");
    FX_ProcessRenderList(EFSLIST_GENERAL, 1, RenderFunc, bLighting);         // Sorted list without preprocess
    PROFILE_LABEL_POP("GENERAL_AW");
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_GENERAL] += iTimer->GetAsyncCurTime() - fTimeP;

    fTimeP = iTimer->GetAsyncCurTime();
    PROFILE_LABEL_PUSH("TERRAINLAYERS_AW");
    FX_ProcessRenderList(EFSLIST_TERRAINLAYER, 1, RenderFunc, bLighting);    // Unsorted list without preprocess
    PROFILE_LABEL_POP("TERRAINLAYERS_AW");
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_TERRAINLAYER] += iTimer->GetAsyncCurTime() - fTimeP;

    fTimeP = iTimer->GetAsyncCurTime();
    PROFILE_LABEL_PUSH("DECALS_BW");
    FX_ProcessRenderList(EFSLIST_DECAL, 0, RenderFunc, bLighting);           // Sorted list without preprocess
    PROFILE_LABEL_POP("DECALS_BW");
    PROFILE_LABEL_PUSH("DECALS_AW");
    FX_ProcessRenderList(EFSLIST_DECAL, 1, RenderFunc, bLighting);           // Sorted list without preprocess
    PROFILE_LABEL_POP("DECALS_AW");
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_DECAL] += iTimer->GetAsyncCurTime() - fTimeP;
    
    /* SSIL research by Vlad
    if(	CV_r_SSAO && m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_ALLOW_AO && SRendItem::m_RecurseLevel==1 && GetFrameID(false) != m_RP.m_nAOMaskUpdateLastFrameId )
    {
      FX_ProcessAOTarget();
      m_RP.m_FillLights[SRendItem::m_RecurseLevel].SetUse(0);

      // Modulate screen by colored SSAO buffer

      {
        PROFILE_SHADER_START;
        FX_ScreenStretchRect(CTexture::s_ptexSceneTarget);
        PROFILE_SHADER_END;
      }

      {
        PROFILE_SHADER_START;
        STexState TexStatePoint = STexState(FILTER_POINT, true);
        CTexture::m_Text_AOTarget->Apply(0, CTexture::GetTexState(TexStatePoint));
        SetViewport(0, 0, GetWidth(), GetHeight());
        static CCryName techName("TextureToTexture");
        SetCullMode(R_CULL_NONE);
        DrawFullScreenQuad(CShaderMan::m_shHDRPostProcess, techName, 0, 0, 1, 1, 
//          GS_NODEPTHTEST);
//        GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);
        GS_NODEPTHTEST | GS_BLSRC_ZERO | GS_BLDST_SRCCOL);
        PROFILE_SHADER_END;
      }
    }*/

    if( CV_r_rain )
    {
      fTimeP = iTimer->GetAsyncCurTime();
      FX_ProcessRainRenderLists(EFSLIST_GENERAL, 0, RenderFunc);
      FX_ProcessRainRenderLists(EFSLIST_GENERAL, 1, RenderFunc);
      if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsRAIN += iTimer->GetAsyncCurTime() - fTimeP;
    }

		PROFILE_LABEL_POP("OPAQUE_PASSES");

    bool bDeferredScenePasses = (nFlags & SHDF_ALLOWPOSTPROCESS) && !nR && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE) && !bEmpty;
    if ( bDeferredScenePasses )
    {
      // Avoid using ResetToDefault - it's reseting everything, including gpr's states - we leave this comment out for now
      // in case any problems
      //ResetToDefault();

      FX_ResetPipe();  





      FX_DeferredCaustics();
      FX_DeferredRainLayer();




    }














		PROFILE_LABEL_PUSH("TRANSPARENT_PASSES");

		if ( bDeferredScenePasses )
    {
      fTimeP = iTimer->GetAsyncCurTime();

			// Avoid using ResetToDefault - it's reseting everything, including gpr's states - we leave this comment out for now
			// in case any problems
			//ResetToDefault();

      FX_ResetPipe();  






      FX_FogScene();





      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsDeferredLayers += iTimer->GetAsyncCurTime() - fTimeP;
    }





    bool bUpdateSceneTarget = true;
#if defined( PS3 )
    if( m_RP.m_bUseHDR && CV_r_refraction == 2 )
    {
      CTexture::s_ptexCurrSceneTarget = CTexture::s_ptexHDRTarget;
      bUpdateSceneTarget = false;
    }
#endif

    if ((CRenderer::CV_r_fur) && (m_RP.m_nRendFlags & SHDF_ALLOWPOSTPROCESS) && !nR)
    {
      uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_GENERAL, m_RP.m_nProcessThreadID, pRLD);
      if( nBatchMask & FB_FUR )
      {
        if (bUpdateSceneTarget && !bEmpty && CTexture::IsTextureExist(CTexture::s_ptexSceneTarget))
        {
          if (!CRenderer::CV_r_debugrefraction)
            FX_ScreenStretchRect(CTexture::s_ptexSceneTarget);
          else
            CTexture::s_ptexSceneTarget->Fill(ColorF(1, 0, 0, 1));  
        }

        FX_ProcessFurRenderLists(EFSLIST_GENERAL, 0, RenderFunc);
        FX_ProcessFurRenderLists(EFSLIST_GENERAL, 1, RenderFunc);
      }    
    }

    //if (CRenderer::CV_r_usewaterparticles && (nFlags & SHDF_ALLOWPOSTPROCESS) && !nR && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE) )
    //{
    //  bool bEmpty = SRendItem::IsListEmpty(EFSLIST_WATERPARTICLES, m_RP.m_nProcessThreadID, pRLD);
    //  if( !bEmpty)
    //    FX_DeferredWaterParticles(true);
    //}

    if( nFlags & SHDF_ALLOW_WATER )
    {
      fTimeP = iTimer->GetAsyncCurTime();
      FX_ProcessRenderList(EFSLIST_WATER_VOLUMES, 0, RenderFunc, false);    // Sorted list without preprocess
      if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_WATER_VOLUMES] += iTimer->GetAsyncCurTime() - fTimeP;
    }







    fTimeP = iTimer->GetAsyncCurTime();
    PROFILE_LABEL_PUSH("GENERAL_BW");
    FX_ProcessRenderList(EFSLIST_TRANSP, 0, RenderFunc, bLighting); // Unsorted list
    PROFILE_LABEL_POP("GENERAL_BW");
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_TRANSP] += iTimer->GetAsyncCurTime() - fTimeP;







    if (!nR && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE))
    {
      PROFILE_LABEL_PUSH("REFRACTIVE_BW");
      if (nFlags & SHDF_ALLOWPOSTPROCESS)
      {
        if (CV_r_refraction)
        {
          bool isEmpty = SRendItem::IsListEmpty(EFSLIST_REFRACTPASS, m_RP.m_nProcessThreadID, pRLD, 0);
          if (!isEmpty && CTexture::IsTextureExist(CTexture::s_ptexSceneTarget))
          {
            if( bUpdateSceneTarget )
            {
              if (!CRenderer::CV_r_debugrefraction)
                FX_ScreenStretchRect(CTexture::s_ptexSceneTarget);
              else
                CTexture::s_ptexSceneTarget->Fill(ColorF(1, 0, 0, 1));                        
            }

            FX_ProcessRenderList(EFSLIST_REFRACTPASS, 0, RenderFunc, true); // Unsorted list            
          }
        }

        if( CV_r_rain )
        {
          fTimeP = iTimer->GetAsyncCurTime();
          FX_ProcessRainRenderLists(EFSLIST_TRANSP, 0, RenderFunc);
          if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
            m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsRAIN += iTimer->GetAsyncCurTime() - fTimeP;
        }
      }
      PROFILE_LABEL_POP("REFRACTIVE_BW");

      if( nFlags & SHDF_ALLOW_WATER )
      {
        bool isEmpty = SRendItem::IsListEmpty(EFSLIST_WATER, m_RP.m_nProcessThreadID, pRLD) && SRendItem::IsListEmpty(EFSLIST_WATER_VOLUMES, m_RP.m_nProcessThreadID, pRLD);
        if (bUpdateSceneTarget && !isEmpty && CTexture::IsTextureExist(CTexture::s_ptexSceneTarget))
        {
          if (!CRenderer::CV_r_debugrefraction)
            FX_ScreenStretchRect(CTexture::s_ptexSceneTarget);
          else
            CTexture::s_ptexSceneTarget->Fill(ColorF(1, 0, 0, 1));
        }
      }
    }

    if( nFlags & SHDF_ALLOW_WATER )
    {
      fTimeP = iTimer->GetAsyncCurTime();
      FX_ProcessRenderList(EFSLIST_WATER, 0, RenderFunc, false); // Unsorted list
      FX_ProcessRenderList(EFSLIST_WATER, 1, RenderFunc, false); // Unsorted list
      FX_ProcessRenderList(EFSLIST_WATER_VOLUMES, 1, RenderFunc, false);    // Sorted list without preprocess       

      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_OCEANPARTICLES;

      if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_WATER_VOLUMES] += iTimer->GetAsyncCurTime() - fTimeP;
    }








    fTimeP = iTimer->GetAsyncCurTime();
    PROFILE_LABEL_PUSH("GENERAL_AW");
    FX_ProcessRenderList(EFSLIST_TRANSP, 1, RenderFunc, true); // Unsorted list
    PROFILE_LABEL_POP("GENERAL_AW");
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_TRANSP] += iTimer->GetAsyncCurTime() - fTimeP;









    PROFILE_LABEL_PUSH("REFRACTIVE_AW");
    if (nFlags & SHDF_ALLOWPOSTPROCESS)
    {
      if (!nR && CV_r_refraction && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE)) 
      {
        bool isEmpty = SRendItem::IsListEmpty(EFSLIST_REFRACTPASS, m_RP.m_nProcessThreadID, pRLD, 1);
        if (!isEmpty && CTexture::IsTextureExist(CTexture::s_ptexSceneTarget))
        {
          if( bUpdateSceneTarget )
          {
            if( !CRenderer::CV_r_debugrefraction )
              FX_ScreenStretchRect(CTexture::s_ptexSceneTarget);
            else
              CTexture::s_ptexSceneTarget->Fill(ColorF(1, 0, 0, 1));                              
          }

          FX_ProcessRenderList(EFSLIST_REFRACTPASS, 1, RenderFunc, true); // Unsorted list
        }
      }

      if( CV_r_rain )
      {
        fTimeP = iTimer->GetAsyncCurTime();
        FX_ProcessRainRenderLists(EFSLIST_TRANSP, 1, RenderFunc);
        if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
          m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsRAIN += iTimer->GetAsyncCurTime() - fTimeP;
      }
    }
    PROFILE_LABEL_POP("REFRACTIVE_AW");

#if defined( PS3 )
    // Restore scene target
    if( m_RP.m_bUseHDR && CV_r_refraction == 2 )
      CTexture::s_ptexCurrSceneTarget = CTexture::s_ptexSceneTarget;
#endif

		PROFILE_LABEL_POP("TRANSPARENT_PASSES");

    fTimeP = iTimer->GetAsyncCurTime();
    if (CV_r_glow && bAllowPostProcess)
      FX_ProcessPostRenderLists(FB_GLOW);
    if (CRenderer::s_MotionBlurMode > 1)
      FX_ProcessPostRenderLists(FB_MOTIONBLUR);

    if (!nR)
    {
      FX_ProcessRenderList(EFSLIST_HDRPOSTPROCESS, 0, RenderFunc, false);       // Sorted list without preprocess of all fog passes and screen shaders
      FX_ProcessRenderList(EFSLIST_HDRPOSTPROCESS, 1, RenderFunc, false);       // Sorted list without preprocess of all fog passes and screen shaders
      FX_ProcessRenderList(EFSLIST_AFTER_HDRPOSTPROCESS, 0, RenderFunc, false); // for specific cases where rendering after tone mapping is needed
      FX_ProcessRenderList(EFSLIST_AFTER_HDRPOSTPROCESS, 1, RenderFunc, false);  
      FX_ProcessRenderList(EFSLIST_POSTPROCESS, 0, RenderFunc, false);       // Sorted list without preprocess of all fog passes and screen shaders
      FX_ProcessRenderList(EFSLIST_POSTPROCESS, 1, RenderFunc, false);       // Sorted list without preprocess of all fog passes and screen shaders
      FX_ProcessRenderList(EFSLIST_AFTER_POSTPROCESS, 0, RenderFunc, false); // for specific cases where rendering after all post effects is needed
      FX_ProcessRenderList(EFSLIST_AFTER_POSTPROCESS, 1, RenderFunc, false);  

      if( CV_r_deferredshadingdebug && bAllowDeferred)
        FX_DeferredRendering( true );
    }
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_MAKESPRITE)))
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_POSTPROCESS] += iTimer->GetAsyncCurTime() - fTimeP;
  }
  else
  {
    FX_ProcessRenderList(EFSLIST_GENERAL, 0, RenderFunc, true);    // Sorted list without preprocess
    FX_ProcessRenderList(EFSLIST_TERRAINLAYER, 0, RenderFunc, true);    // Unsorted list without preprocess
    FX_ProcessRenderList(EFSLIST_DECAL, 0, RenderFunc, true);    // Sorted list without preprocess
    FX_ProcessRenderList(EFSLIST_WATER_VOLUMES, 0, RenderFunc, false);    // Sorted list without preprocess

    FX_ProcessRenderList(EFSLIST_GENERAL, 1, RenderFunc, true);    // Sorted list without preprocess
    FX_ProcessRenderList(EFSLIST_TERRAINLAYER, 1, RenderFunc, true);    // Unsorted list without preprocess
    FX_ProcessRenderList(EFSLIST_DECAL, 1, RenderFunc, true);    // Sorted list without preprocess
    FX_ProcessRenderList(EFSLIST_WATER_VOLUMES, 1, RenderFunc, false);    // Sorted list without preprocess
  }
///* //Diesel cut
  if (pDT)
  {
    pDT->RestoreRT(0, true);
    RT_SetViewport(0, 0, m_width, m_height);
    pDT->Apply(0, -1);
    D3DSetCull(eCULL_None);
    EF_SetState(GS_NODEPTHTEST);
    FX_SetFPMode();
    DrawQuad(nX-0.5f, nY-0.5f, nX+nWidth-0.5f, nY+nHeight-0.5f, Col_White);
    SAFE_DELETE(pDT);
  }
//*/ //Diesel cut
  FX_ApplyThreadState(m_RP.m_OldTI[nR], NULL);

  m_RP.m_PS[m_RP.m_nProcessThreadID].m_fRenderTime = iTimer->GetAsyncCurTime() - fTime;

	if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] -1 == 0)
		m_CameraMatrixPrev[ min(1, SRendItem::m_RenderView[m_RP.m_nProcessThreadID]) ] = GetTransposed44(Matrix44A(Matrix33(Matrix33::CreateRotationX(-gf_PI/2)) * Matrix34A(m_RP.m_TI[m_RP.m_nProcessThreadID].m_cam.GetMatrix().GetInverted())));

  m_RP.m_nRendFlags = nSaveRendFlags;
  CV_r_scissor = nSaveScissor;
  CV_r_nodrawnear = nSaveDrawNear;
  CV_r_watercaustics = nSaveDrawCaustics;
  CV_r_texturesstreamingsync = nSaveStreamSync;
	
	PROFILE_LABEL_POP("Scene");
}

//======================================================================================================

// Process all render item lists (can be called recursively)
void CD3D9Renderer::EF_ProcessRenderLists(RenderFunc pRenderFunc, int nFlags, SViewport& VP)
{
//	/* //Diesel cut
	ASSERT_IS_MAIN_THREAD(m_pRT)
  int nThreadID = m_RP.m_nFillThreadID;
  int nR = SRendItem::m_RecurseLevel[nThreadID]-1;
  assert(nR >= 0);
  int i, j;
  SRenderListDesc RLD;
  RLD.m_VP = VP;
  for (j=0; j<MAX_LIST_ORDER; j++)
  {
    for (i=0; i<EFSLIST_NUM; i++)
    {
      RLD.m_nStartRI[j][i] = SRendItem::m_AppStartRI[nR][j][i];
      RLD.m_nEndRI[j][i] = SRendItem::RendItems(nThreadID,j,i).Num();
      RLD.m_nBatchFlags[j][i] = SRendItem::m_BatchFlags[nR][j][i];
    }
  }

  //disable sorting for shadowgen
  RLD.m_nStartRI[SG_SORT_GROUP][EFSLIST_SHADOW_GEN] = 0;
  RLD.m_nEndRI[SG_SORT_GROUP][EFSLIST_SHADOW_GEN] = 0;

  EF_SortRenderLists(&RLD, nThreadID);

  if (!nR)
  {
    assert(nThreadID == m_RP.m_nFillThreadID); // make sure this is main thread
    if ((nFlags & SHDF_ALLOWPOSTPROCESS) && !(m_RP.m_TI[nThreadID].m_PersFlags & RBPF_MAKESPRITE))
    {
      int nList = EFSLIST_PREPROCESS;
      int nums = RLD.m_nStartRI[0][nList];
      int nume = RLD.m_nEndRI[0][nList];
      // Perform pre-process operations for the current frame
      if (nume-nums>0 && SRendItem::RendItems(nThreadID,0,nList)[nums].nBatchFlags & 0xffff0000)
        nums += EF_Preprocess(&SRendItem::RendItems(nThreadID,0,nList)[0], nums, nume, pRenderFunc);
    }

    //TODO: make support for forward shadow map rendering
    //if (CV_r_ShadowsForwardPass && !(nFlags & SHDF_ZPASS_ONLY)) //|SHDF_ALLOWPOSTPROCESS
    //  EF_PrepareAllDepthMaps();
  }
  m_pRT->RC_RenderScene(nFlags, pRenderFunc, &RLD);
  //*/ //Diesel cut


//Diesel new
/*
  	ASSERT_IS_MAIN_THREAD(m_pRT)
  int nThreadID = m_RP.m_nFillThreadID;
  int nR = SRendItem::m_RecurseLevel[nThreadID]-1;
  assert(nR >= 0);
  int i, j;
  SRenderListDesc RLD;
  RLD.m_VP = VP;
  for (j=0; j<MAX_LIST_ORDER; j++)
  {
    for (i=0; i<EFSLIST_NUM; i++)
    {
      RLD.m_nStartRI[j][i] = SRendItem::m_AppStartRI[nR][j][i];
      RLD.m_nEndRI[j][i] = SRendItem::RendItems(nThreadID,j,i).Num();
      RLD.m_nBatchFlags[j][i] = SRendItem::m_BatchFlags[nR][j][i];
    }
  }
  

  //disable sorting for shadowgen
  RLD.m_nStartRI[SG_SORT_GROUP][EFSLIST_SHADOW_GEN] = 0;
  RLD.m_nEndRI[SG_SORT_GROUP][EFSLIST_SHADOW_GEN] = 0;

  EF_SortRenderLists(&RLD, nThreadID);
   m_pRT->RC_RenderScene(nFlags, pRenderFunc, &RLD);
*/
//Diesel new




}

void CD3D9Renderer::EF_RenderScene(int nFlags, SViewport& VP)
{
  //BOOL result = XLockL2( XLOCKL2_INDEX_TITLE, &m_RP, sizeof(SRenderPipeline), XLOCKL2_LOCK_SIZE_1_WAY, 0);

  int nThreadID = m_pRT->GetThreadList();
  int nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID]-1;

  float time0 = iTimer->GetAsyncCurTime();

  if (CV_r_excludeshader->GetString()[0] != '0')
  {
    char nm[256];
    strcpy(nm, CV_r_excludeshader->GetString());
    strlwr(nm);
    m_RP.m_sExcludeShader = nm;
  }
  else
    m_RP.m_sExcludeShader = "";

  if (CV_r_showonlyshader->GetString()[0] != '0')
  {
    char nm[256];
    strcpy(nm, CV_r_showonlyshader->GetString());
    strlwr(nm);
    m_RP.m_sShowOnlyShader = nm;
  }
  else
    m_RP.m_sShowOnlyShader = "";

  if (nFlags & SHDF_ALLOWPOSTPROCESS)
    EF_AddClientPolys();

  EF_ProcessRenderLists(FX_FlushShader_General, nFlags, VP);

  //EF_DrawDebugTools(VP); //Diesel cut
  //EF_PopObjectsList(m_RP.m_nProcessThreadID);

  m_RP.m_PS[m_RP.m_nProcessThreadID].m_fSceneTime += iTimer->GetAsyncCurTime()-time0;
}


// Process all render item lists
void CD3D9Renderer::EF_EndEf3D(int nFlags, int nPrecacheUpdateId)
{
  ASSERT_IS_MAIN_THREAD(m_pRT)
  int nThreadID = m_RP.m_nFillThreadID;
  assert(SRendItem::m_RecurseLevel[nThreadID] >= 1);
  if (SRendItem::m_RecurseLevel[nThreadID] < 1)
  {
    iLog->Log("Error: CRenderer::EF_EndEf3D without CRenderer::EF_StartEf");
    return;
  }          

	m_RP.m_TI[m_RP.m_nFillThreadID].m_nObjectUpdateId = max(m_RP.m_TI[m_RP.m_nFillThreadID].m_nObjectUpdateId, nPrecacheUpdateId);

#if defined(PS3) && defined( PS3_CLEAR_DEBUG )
  
  if( CRenderer::CV_r_deferredshading && CTexture::s_ptexSceneNormalsMap && CTexture::s_ptexSceneNormalsMap->GetDeviceTexture() )
  {
    const uint32 nDefaultPowFactor = 16;
    const uint32 nDepthClear = D3DCOLOR_RGBA( (!CRenderer::CV_r_deferredshading)? 0 : nDefaultPowFactor, 0, 0, 0);
    gcpRendD3D->m_pd3dDevice->ClearTexture((CCryDXPSTexture2D *)CTexture::s_ptexSceneNormalsMap->GetDeviceTexture(), nDepthClear);
  }

#endif

  m_p3DEngineCommon.Update();

  if (CV_r_nodrawshaders==1)
  {
    SetClearColor(Vec3(0,0,0));
    EF_ClearBuffers(FRT_CLEAR, NULL);
    SRendItem::m_RecurseLevel[nThreadID]--;
    //EF_PopObjectsList(nThreadID);
    return;
  }

  int nAsyncShaders = CV_r_shadersasynccompiling;
  int nTexStr = CV_r_texturesstreamingsync;
  if (nFlags & SHDF_NOASYNC)
    CV_r_shadersasynccompiling = 0;

  if (SRendItem::m_RecurseLevel[nThreadID]==1 && !(nFlags & SHDF_ZPASS_ONLY)) //|SHDF_ALLOWPOSTPROCESS
  {
    EF_PrepareShadowGenRenderList();
  }

  if (CV_r_testSplitScreen)
  {
    SViewport pViewPortSplit = SViewport(0, 0, m_width, m_height/2);
    EF_Scene3D(pViewPortSplit, nFlags);
    
    pViewPortSplit = SViewport(0, m_height/2, m_width, m_height/2);
    EF_Scene3D(pViewPortSplit, nFlags);
  }
  else if(CV_r_Stereo)
  {
		// Fran: workaround
		float sign = -1.0f;
//		if (CV_r_Stereo == 4)
//			sign = -1.0f;

    CCamera cam =  m_RP.m_TI[nThreadID].m_cam;
    float camOffset = sign * CV_r_StereoEyeDist * 0.5f;
		float frustumShift = camOffset * (cam.GetNearPlane() / CV_r_StereoScreenDist);

    // Left
    Matrix34 stereoMat = Matrix34::CreateTranslationMat( Vec3( -camOffset, 0, 0 ) );
    cam.SetMatrix( cam.GetMatrix() * stereoMat );
    cam.SetAsymmetry( frustumShift, frustumShift, 0, 0 );
    SetCamera(cam);

    EF_Scene3D(m_MainRTViewport, nFlags);
    CopyToStereo(0);

    // Right
    stereoMat = Matrix34::CreateTranslationMat( Vec3( camOffset, 0, 0 ) );
    cam.SetMatrix( cam.GetMatrix() * stereoMat );
    cam.SetAsymmetry( -frustumShift, -frustumShift, 0, 0 );
    SetCamera(cam);

    EF_Scene3D(m_MainRTViewport, nFlags);
    CopyToStereo(1);

    // Display
    if( CV_r_Stereo < 3 ||  CV_r_Stereo == 4)
			DisplayStereo();
  }
  else
    EF_Scene3D(m_MainRTViewport, nFlags);

  SRendItem::m_RecurseLevel[nThreadID]--;

  CV_r_shadersasynccompiling = nAsyncShaders;
}

void CD3D9Renderer::EF_Scene3D(SViewport& VP, int nFlags)
{
	ASSERT_IS_MAIN_THREAD(m_pRT)
  int nThreadID = m_RP.m_nFillThreadID;

  bool bFullScreen = true;
  SDynTexture *pDT = NULL;
  int nR = SRendItem::m_RecurseLevel[nThreadID]-1;
	int nRenderView = SRendItem::m_RenderView[nThreadID];

	if( !nR && !nRenderView && !CV_r_measureoverdraw && !(m_RP.m_TI[nThreadID].m_PersFlags & RBPF_MAKESPRITE) )
	{
		bool bAllowDeferred = (nFlags & SHDF_ZPASS) && (CV_r_deferredshading) ;         
		if( bAllowDeferred )
		{
			SShaderItem shItem( CShaderMan::m_shDeferredShading );
			CRenderObject *pObj = EF_GetObject(true);
			if (pObj)
			{
				pObj->m_II.m_Matrix.SetIdentity();
				EF_AddEf(m_RP.m_pREDeferredShading, shItem, pObj, EFSLIST_DEFERRED_PREPROCESS, 0);        
			}
		}

		if ((nFlags & SHDF_ALLOWHDR) && IsHDRModeEnabled()  )
		{
			SShaderItem shItem(CShaderMan::m_shHDRPostProcess);
			CRenderObject *pObj = EF_GetObject(true);
			if (pObj)
			{
				pObj->m_II.m_Matrix.SetIdentity();
				EF_AddEf(m_RP.m_pREHDR, shItem, pObj, EFSLIST_HDRPOSTPROCESS, 0);
			}
		}

		bool bAllowPostProcess = (nFlags & SHDF_ALLOWPOSTPROCESS) && (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_POSTPROCESS) 
														&& (CV_r_PostProcess) && CV_r_wireframe == 0;
		if (bAllowPostProcess)
		{
			SShaderItem shItem( CShaderMan::m_shPostEffects );
			CRenderObject *pObj = EF_GetObject(true);
			if (pObj)
			{
				pObj->m_II.m_Matrix.SetIdentity();
				EF_AddEf(m_pREPostProcess, shItem, pObj, EFSLIST_POSTPROCESS, 0);
			}
		}  
	}

  // Update per-frame params
  UpdateConstParamsPF();

  EF_RenderScene(nFlags, VP);

	SRendItem::m_RenderView[nThreadID]++;
}

void CD3D9Renderer::CopyToStereo( int channel )
{
  CTexture *pTex;

  if( channel == 0 )
    pTex = CTexture::s_ptexStereoL;
  else
    pTex = CTexture::s_ptexStereoR;

  if (pTex==NULL)
    return;

  int iTempX, iTempY, iWidth, iHeight;
  GetViewport( &iTempX, &iTempY, &iWidth, &iHeight );

#if defined DIRECT3D9
  RECT pSrcRect = {0, 0, iWidth, iHeight};
  RECT pDstRect = {0, 0, pTex->GetWidth(), pTex->GetHeight()};

  D3DSurface *pBackSurface = GetBackSurface();
  D3DSurface *pTexSurf = (D3DSurface *)pTex->GetSurface( -1, 0 );
  m_pd3dDevice->StretchRect( pBackSurface, &pSrcRect, pTexSurf, &pDstRect, D3DTEXF_NONE ); 

  SAFE_RELEASE( pTexSurf );
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::DisplayStereo()
{
  CShader *pSH = m_cEF.m_ShaderStereo;

//  if( !pSH || pSH->m_HWTechniques.empty())
//  {
//    assert(0);
//    return ;
//  }

  m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NO_SRGBWRITES;

  //FX_PushRenderTarget(0, m_pBackBuffer, &m_DepthBufferOrig);
  FX_PushRenderTarget(1, m_pSecondBackBuffer, NULL);

  uint32 nPasses;
	if (CV_r_Stereo == 1)
		pSH->FXSetTechnique("DualHead");
	else
  if (CV_r_Stereo == 2)
    pSH->FXSetTechnique("IZ3D");
  else
	if (CV_r_Stereo == 4)
		pSH->FXSetTechnique("SideBySide");
	
  pSH->FXBegin(&nPasses, FEF_DONTSETSTATES);
  pSH->FXBeginPass(0);

  float fWidth5 = (float)m_NewViewport.nWidth-0.5f;
  float fHeight5 = (float)m_NewViewport.nHeight-0.5f;

  int nOffs;
  SVF_TP3F_C4B_T2F *Verts = (SVF_TP3F_C4B_T2F *)GetVBPtr(4, nOffs, POOL_TRP3F_COL4UB_TEX2F);
  if( Verts )
  {
    Verts->pos = Vec4(-0.5f, -0.5f, 0.0f, 1.0f);
    Verts->st = Vec2(0, 0);
    Verts->color.dcolor = (uint32)-1;
    Verts++;

    Verts->pos = Vec4(fWidth5, -0.5f, 0.0f, 1.0f);
    Verts->st = Vec2(1, 0);
    Verts->color.dcolor = (uint32)-1;
    Verts++;

    Verts->pos = Vec4(-0.5f, fHeight5, 0.0f, 1.0f);
    Verts->st = Vec2(0, 1);
    Verts->color.dcolor = (uint32)-1;
    Verts++;

    Verts->pos = Vec4(fWidth5, fHeight5, 0.0f, 1.0f);
    Verts->st = Vec2(1, 1);
    Verts->color.dcolor = (uint32)-1;
  }
  UnlockVB(POOL_TRP3F_COL4UB_TEX2F);

  EF_SetState(GS_NODEPTHTEST);

  ColorF clClear(0,0,0,0);
  EF_ClearBuffers(FRT_CLEAR_COLOR, &clClear);
  FX_Commit();

  EF_SetState(GS_NODEPTHTEST);
  if (!FAILED(FX_SetVertexDeclaration(0, eVF_TP3F_C4B_T2F)))
  {
    FX_SetVStream(0, m_pVB[POOL_TRP3F_COL4UB_TEX2F], 0, sizeof(SVF_TP3F_C4B_T2F));

#if defined (DIRECT3D9)		
    m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, 2);
#endif
  }
  pSH->FXEndPass();

  EF_SelectTMU(0);


  //FX_PopRenderTarget(0);
  FX_PopRenderTarget(1);

#if defined (DIRECT3D9)		
  m_pd3dDevice->SetRenderTarget( 1, NULL );
#endif


  m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_NO_SRGBWRITES;

  m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[EFSLIST_GENERAL] += 2;
  m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[EFSLIST_GENERAL]++;

}


void CD3D9Renderer::RT_InsertGpuCallback(uint32 context, GpuCallbackFunc callback)
{





}

/*
void CD3D9Renderer::FX_Flush_ForProfiler(int nEnd)
{
  static CShader *arrCurrShader[2] = {0,0};
  static SShaderTechnique *arrCurrTechnique[2] = {0,0};

  if(CV_r_ProfileShadersGroupByName==1)
  { // detect shader or technique change
    if(arrCurrShader[nEnd] != m_RP.m_pShader || arrCurrTechnique[nEnd] != m_RP.m_pCurTechnique)
    {
      PROFILE_FRAME(FX_Flush_ForProfiler);
      FX_Flush();
      arrCurrShader[nEnd] = m_RP.m_pShader;
      arrCurrTechnique[nEnd] = m_RP.m_pCurTechnique;
    }
  }
  else if(CV_r_ProfileShadersGroupByName==2)
  { // detect only technique change
    if(arrCurrTechnique[nEnd] != m_RP.m_pCurTechnique)
    {
      PROFILE_FRAME(FX_Flush_ForProfiler);
      FX_Flush();
      arrCurrTechnique[nEnd] = m_RP.m_pCurTechnique;
    }
  }
  else
    FX_Flush();
}
*/


//========================================================================================================

