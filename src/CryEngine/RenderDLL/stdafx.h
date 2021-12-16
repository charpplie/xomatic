/*=============================================================================
stdafx.cpp: Cry Render support precompiled header generator.
Copyright 2001 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Honitch Andrey

=============================================================================*/

#ifndef __STDAFX_H__
#define __STDAFX_H__

#pragma once

#define CRY_API

#define DEFINE_MODULE_NAME "CryRender9"
//#define  TRACE_ALL_ALLOCATIONS

// enable support for baked meshes and decals on PC
#if !defined(XENON) && !defined(PS3)
#define RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
#define TEXTURE_GET_SYSTEM_COPY_SUPPORT
#endif

#ifdef _DEBUG
#define CRTDBG_MAP_ALLOC
#endif //_DEBUG

#undef USE_STATIC_NAME_TABLE
#define USE_STATIC_NAME_TABLE

#if !defined(_RELEASE)
#define ENABLE_FRAME_PROFILER 
#endif

#ifdef ENABLE_FRAME_PROFILER
#	define PROFILE 1
#	define ENABLE_FRAME_PROFILER_LABELS
//#	if defined(XENON)
//#		define ENABLE_X360_TEXTURE_CAPTURE
//#	endif
#endif

#if !defined(XENON) && !defined(PS3)

#ifdef ENABLE_FRAME_PROFILER_LABELS
// This is need for D3DPERF_ functions
#pragma comment(lib, "d3d9.lib")
#endif

#endif

#include <CryModuleDefs.h>

#undef eCryModule
#define eCryModule eCryM_Render

#if __HAS_SSE__
// <fvec.h> includes <assert.h>, include it before platform.h
#include <fvec.h>
#define CONST_INT32_PS(N, V3, V2, V1, V0) \
	const _MM_ALIGN16 int _##N[] = { V0, V1, V2, V3 }; /*little endian!*/ \
	const F32vec4 N = _mm_load_ps((float*)_##N);
#endif

#include <platform.h>
#include <CrySizer.h>


// Use LIBPATH_FIXUP to keep relative paths specified in #pragma comment(lib, ...) relative to renderer.
// This is needed when linking the renderer statically into launcher exe.
#if defined(_LIB)
#	define LIBPATH_FIXUP(x) "../../CryEngine/RenderDll/XRenderD3D9/"##x
#else
#	define LIBPATH_FIXUP(x) x
#endif



#if defined(_CPU_SSE) && !defined(WIN64)
#define __HAS_SSE__ 1
#endif

#define MAX_REND_RECURSION_LEVELS 2

//enable this macro to strip out the overhead for render thread
#ifdef STRIP_RENDER_THREAD
#define ASSERT_IS_RENDER_THREAD(rt)
#define ASSERT_IS_MAIN_THREAD(rt)
#else
#define ASSERT_IS_RENDER_THREAD(rt) assert((rt)->IsRenderThread());
#define ASSERT_IS_MAIN_THREAD(rt) assert((rt)->IsMainThread());
#endif

template <typename T, int Count>
struct TMultiVal
{
#if defined(STRIP_RENDER_THREAD)
	T m_Storage[Count];
	T* operator[](const int){return m_Storage;}
	const T* operator[](const int)const{return m_Storage;}
	void* GetAddr(const int)const {return (void*)this;}
	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		for( int i = 0 ; i < Count ; ++i)
			pSizer->AddObject(m_Storage[i]);
	}
#else
	T m_Storage[2][Count];
	T* operator[](const int index){return m_Storage[index];}
	const T* operator[](const int index)const{return m_Storage[index];}
	void* GetAddr(const int index)const {return (void*)&m_Storage[index];}
	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		for( int i = 0 ; i < Count ; ++i)
			pSizer->AddObject(m_Storage[0][i]);
		for( int i = 0 ; i < Count ; ++i)
			pSizer->AddObject(m_Storage[1][i]);
	}
#endif
};

#include <Tarray.h>
template <typename T>
struct TArrayHolder : public TMultiVal<TArray<T>,MAX_REND_RECURSION_LEVELS>
{};

template <typename T>
struct TDualVal
{
#if defined(STRIP_RENDER_THREAD)
	T m_Storage;
	T& operator[](const int){return m_Storage;}
	const T& operator[](const int)const{return m_Storage;}
#else
	T m_Storage[2];
	T& operator[](const int index){return m_Storage[index];}
	const T& operator[](const int index)const{return m_Storage[index];}
#endif
};



#if defined (NULL_RENDERER) || defined (XENON) || defined(PS3)
#define EXCLUDE_SQUISH_SDK
#endif

#define _USE_MATH_DEFINES
#include MATH_H

#if defined (DIRECT3D9) || defined (DIRECT3D10)
// Direct3D9 includes
#if !defined(CRY_USE_GCM)
#include "d3d9.h"
#include "d3dx9.h"
#endif

// Direct3D11 includes
#if !defined(XENON)
#if defined(CRY_USE_GCM)
#include "XRenderD3D9/DXPS/CCryDXPSMisc.hpp"
#include "XRenderD3D9/DXPS/CCryDXPSRenderer.hpp"
#include "XRenderD3D9/DXPS/DXPSGI/CCryDXPSGI.hpp"
#else
#include "dxgi.h"
#include "d3d11.h"
#include "d3dx11.h"
#endif
#endif
#elif defined (NULL_RENDERER) && defined(WIN32)
#include "windows.h"
#endif

#include "CryThread.h"

//////////////////////////////////////////////////////////////////////////
// XBOX specific defines for Renderer.
//////////////////////////////////////////////////////////////////////////
#if defined (DIRECT3D9)
#define Direct3D                         IDirect3D9
#define D3DDevice                        IDirect3DDevice9
#define D3DDeviceContext                 IDirect3DDevice9
#define D3DVertexDeclaration             IDirect3DVertexDeclaration9
#define D3DVertexShader                  IDirect3DVertexShader9
#define D3DPixelShader                   IDirect3DPixelShader9
#define D3DBaseTexture                   IDirect3DBaseTexture9
#define D3DTexture                       IDirect3DTexture9
#define D3DVolumeTexture                 IDirect3DVolumeTexture9
#define D3DCubeTexture                   IDirect3DCubeTexture9
#define D3DVertexBuffer                  IDirect3DVertexBuffer9
#define D3DIndexBuffer                   IDirect3DIndexBuffer9
#define D3DSurface                       IDirect3DSurface9
#define D3DDepthSurface                  IDirect3DSurface9
#define D3DQuery                         IDirect3DQuery9
#define D3DViewPort                      D3DVIEWPORT9
#define D3DFormat                        D3DFORMAT
inline bool operator != (const D3DVIEWPORT9& v0, const D3DVIEWPORT9& v1)
{
	if (v0.X!=v1.X || v0.Y!=v1.Y || v0.Width!=v1.Width || v0.Height != v1.Height || v0.MinZ!=v1.MinZ || v0.MaxZ!=v1.MaxZ)
		return true;
	return false;
}
#elif defined (DIRECT3D10)
#define Direct3D                         IDXGIAdapter
#define D3DDevice                        ID3D11Device
#define D3DDeviceContext                 ID3D11DeviceContext
#define D3DVertexDeclaration             ID3D11InputLayout
#define D3DVertexShader                  ID3D11VertexShader
#define D3DPixelShader                   ID3D11PixelShader
#define D3DBaseTexture                   ID3D11Resource
#define D3DTexture                       ID3D11Texture2D
#define D3DVolumeTexture                 ID3D11Texture3D
#define D3DCubeTexture                   ID3D11Texture2D
#define D3DVertexBuffer                  ID3D11Buffer
#define D3DIndexBuffer                   ID3D11Buffer
#define D3DSurface                       ID3D11RenderTargetView
#define D3DDepthSurface                  ID3D11DepthStencilView
#define D3DQuery                         ID3D11Query
#define D3DViewPort                      D3D11_VIEWPORT
#define D3DFormat                        DXGI_FORMAT

const int32 g_nD3D10MaxSupportedSubres = (6 * 15);
//////////////////////////////////////////////////////////////////////////
#else
#define D3DFormat                        int
#endif //_XBOX
//////////////////////////////////////////////////////////////////////////

#define USAGE_WRITEONLY 8

//////////////////////////////////////////////////////////////////////////
// Linux specific defines for Renderer.
//////////////////////////////////////////////////////////////////////////

#if defined(_AMD64_) && !defined(LINUX)
#include <io.h>
#endif


/////////////////////////////////////////////////////////////////////////////
// STL //////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <memory>

#define SIZEOF_ARRAY(arr) (sizeof(arr)/sizeof((arr)[0]))

#ifndef SAFE_DELETE_VOID_ARRAY
#define SAFE_DELETE_VOID_ARRAY(p)	{ if(p) { delete[] ((char*)p);		(p)=NULL; } }
#endif

#ifdef DEBUGALLOC

#include <crtdbg.h>
#define DEBUG_CLIENTBLOCK new( _NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_CLIENTBLOCK

// memman
#define   calloc(s,t)       _calloc_dbg(s, t, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)

#endif

#include <CryName.h>

#define	MAX_TMU 16
#define	MAX_STREAMS 16

//! Include main interfaces.
#include <ICryPak.h>
#include <IProcess.h>
#include <ITimer.h>
#include <ISystem.h>
#include <ILog.h>
#include <IConsole.h>
#include <IRenderer.h>
#include <IStreamEngine.h>
#include <CrySizer.h>
#include <smartptr.h>
#include <CryArray.h>
#include <PoolAllocator.h>

#include <CryArray.h>

#if defined(OPENGL)
#include <PSGL/psgl.h>
#include <PSGL/psglu.h>
#include "XRenderD3D9/OpenGL/D3DX9/PS3_d3dx9.h"
#endif

// Interfaces from the Game
extern ILog     *iLog;
extern IConsole *iConsole;
extern ITimer   *iTimer;
extern ISystem  *iSystem;

template <class Container>
unsigned sizeOfV (Container& arr)
{
	int i;
	unsigned size = 0;
	for (i=0; i<(int)arr.size(); i++)
	{
		typename Container::value_type& T = arr[i];
		size += T.Size();

	}
	size += (arr.capacity()-arr.size())*sizeof(typename Container::value_type);
	return size;
}
template <class Container>
unsigned sizeOfA (Container& arr)
{
	int i;
	unsigned size = 0;
	for (i=0; i<arr.size(); i++)
	{
		typename Container::value_type& T = arr[i];
		size += T.Size();

	}
	return size;
}
template <class Map>
unsigned sizeOfMap (Map& map)
{
	unsigned size = 0;
	for (typename Map::iterator it=map.begin(); it!=map.end(); it++)
	{
		typename Map::mapped_type& T = it->second;
		size += T.Size();
	}
	size += map.size() * sizeof(stl::MapLikeStruct);
	return size;
}
template <class Map>
unsigned sizeOfMapStr (Map& map)
{
	unsigned size = 0;
	for (typename Map::iterator it=map.begin(); it!=map.end(); it++)
	{
		typename Map::mapped_type& T = it->second;
		size += T.capacity();
	}
	size += map.size() * sizeof(stl::MapLikeStruct);
	return size;
}
template <class Map>
unsigned sizeOfMapP (Map& map)
{
	unsigned size = 0;
	for (typename Map::iterator it=map.begin(); it!=map.end(); it++)
	{
		typename Map::mapped_type& T = it->second;
		size += T->Size();
	}
	size += map.size() * sizeof(stl::MapLikeStruct);
	return size;
}
template <class Map>
unsigned sizeOfMapS (Map& map)
{
	unsigned size = 0;
	for (typename Map::iterator it=map.begin(); it!=map.end(); it++)
	{
		typename Map::mapped_type& T = it->second;
		size += sizeof(T);
	}
	size += map.size() * sizeof(stl::MapLikeStruct);
	return size;
}



#include <Cry_Math.h>
#include <Cry_Geo.h>
//#include "_Malloc.h"
//#include "math.h"
#include <StlUtils.h>
#include "XRenderD3D9/DeviceManager/DeviceManager.h"

#include <VertexFormats.h>

#include "Common/CommonRender.h"
#include <IRenderAuxGeom.h>
#include "Common/Shaders/ShaderComponents.h"
#include "Common/Shaders/ShaderGraph.h"
#include "Common/Shaders/Shader.h"
//#include "Common/XFile/File.h"
//#include "Common/Image.h"
#include "Common/Shaders/CShader.h"
#include "Common/DevBuffer.h"
#include "Common/RenderMesh2.h"
#include "Common/RenderPipeline.h"
#include "Common/RenderThread.h"
#include "Common/Renderer.h"
#include "Common/Textures/Texture.h"
#include "Common/Shaders/Parser.h"
#include "Common/FrameProfiler.h"
#include "Common/Shadow_Renderer.h"
#include "Common/DeferredRenderUtils.h"
#include "Common/ShadowUtils.h"
#include "Common/WaterUtils.h"

#include "Common/3DUtils.h"

#include "Common/OcclQuery.h"

// All handled render elements (except common ones included in "RendElement.h")
#include "Common/RendElements/CREBeam.h"
#include "Common/RendElements/CREClientPoly.h"
#include "Common/RendElements/CREFlares.h"
#include "Common/RendElements/CRETempMesh.h"
#include "Common/RendElements/CREHDRProcess.h"
#include "Common/RendElements/CRECloud.h"
#include "Common/RendElements/CREDeferredShading.h"

#include "Common/PostProcess/PostProcess.h"
#include "Common/RendElements/CREPostProcessData.h"

/*-----------------------------------------------------------------------------
Vector transformations.
-----------------------------------------------------------------------------*/

//we need a better function-name for this exotic operation
//there already is a "TransformPoint" in Cry_Matrix.h
_inline void TransformPoint( const Matrix34A &Matr, Vec3& inp, Vec3& outp)
{
	Vec3 Temp = inp - Matr.GetTranslation();
	outp.x = Temp | Matr.GetColumn(0);
	outp.y = Temp | Matr.GetColumn(1);
	outp.z = Temp | Matr.GetColumn(2);
}


_inline void TransformVector(Vec3& out, const Vec3& in, const Matrix44A& m)
{
	out.x = in.x * m(0,0) + in.y * m(1,0) + in.z * m(2,0);
	out.y = in.x * m(0,1) + in.y * m(1,1) + in.z * m(2,1);
	out.z = in.x * m(0,2) + in.y * m(1,2) + in.z * m(2,2);
}

_inline void TransformPosition(Vec3& out, const Vec3& in, const Matrix44A& m)
{
	TransformVector (out, in, m);
	out += m.GetRow(3);
}


inline Plane TransformPlaneByUsingAdjointT( const Matrix44A& M, const Matrix44A& TA, const Plane plSrc)
{
	Vec3 newNorm;
	TransformVector (newNorm, plSrc.n, TA);
	newNorm.Normalize();

	if(M.Determinant() < 0.f)
		newNorm *= -1;

	Plane plane;
	Vec3 p;
	TransformPosition (p, plSrc.n * plSrc.d, M);
	plane.Set(newNorm, p | newNorm);

	return plane;
}

inline Matrix44 TransposeAdjoint(const Matrix44A& M)
{
	Matrix44 ta;

	ta(0,0) = M(1,1) * M(2,2) - M(2,1) * M(1,2);
	ta(1,0) = M(2,1) * M(0,2) - M(0,1) * M(2,2);
	ta(2,0) = M(0,1) * M(1,2) - M(1,1) * M(0,2);

	ta(0,1) = M(1,2) * M(2,0) - M(2,2) * M(1,0);
	ta(1,1) = M(2,2) * M(0,0) - M(0,2) * M(2,0);
	ta(2,1) = M(0,2) * M(1,0) - M(1,2) * M(0,0);

	ta(0,2) = M(1,0) * M(2,1) - M(2,0) * M(1,1);
	ta(1,2) = M(2,0) * M(0,1) - M(0,0) * M(2,1);
	ta(2,2) = M(0,0) * M(1,1) - M(1,0) * M(0,1);

	ta(0,3) = 0.f;
	ta(1,3) = 0.f;
	ta(2,3) = 0.f;


	return ta;
}

inline Plane TransformPlane( const Matrix44A& M, const Plane& plSrc)
{
	Matrix44 tmpTA = TransposeAdjoint(M);
	return TransformPlaneByUsingAdjointT(M, tmpTA, plSrc);
}

// Homogeneous plane transform.
inline Plane TransformPlane2(const Matrix34A& m, const Plane& src)
{
	Plane plDst;

	float v0=src.n.x, v1=src.n.y, v2=src.n.z, v3=src.d;
	plDst.n.x = v0 * m(0,0) + v1 * m(1,0) + v2 * m(2,0);
	plDst.n.y = v0 * m(0,1) + v1 * m(1,1) + v2 * m(2,1);
	plDst.n.z = v0 * m(0,2) + v1 * m(1,2) + v2 * m(2,2);

	plDst.d = v0 * m(0,3) + v1 * m(1,3) + v2 * m(2,3) + v3;

	return plDst;
}

// Homogeneous plane transform.
inline Plane TransformPlane2(const Matrix44A& m, const Plane& src)
{
	Plane plDst;

	float v0=src.n.x, v1=src.n.y, v2=src.n.z, v3=src.d;
	plDst.n.x = v0 * m(0,0) + v1 * m(0,1) + v2 * m(0,2) + v3 * m(0,3);
	plDst.n.y = v0 * m(1,0) + v1 * m(1,1) + v2 * m(1,2) + v3 * m(1,3);
	plDst.n.z = v0 * m(2,0) + v1 * m(2,1) + v2 * m(2,2) + v3 * m(2,3);

	plDst.d = v0 * m(3,0) + v1 * m(3,1) + v2 * m(3,2) + v3 * m(3,3);

	return plDst;
}
inline Plane TransformPlane2_NoTrans(const Matrix44A& m, const Plane& src )
{
	Plane plDst;
	TransformVector(plDst.n, src.n, m);
	plDst.d = src.d;

	return plDst;
}

inline Plane TransformPlane2Transposed(const Matrix44A& m, const Plane& src )
{
	Plane plDst;

	float v0=src.n.x, v1=src.n.y, v2=src.n.z, v3=src.d;
	plDst.n.x = v0 * m(0,0) + v1 * m(1,0) + v2 * m(2,0) + v3 * m(3,0);
	plDst.n.y = v0 * m(0,1) + v1 * m(1,1) + v2 * m(2,1) + v3 * m(3,1);
	plDst.n.z = v0 * m(0,2) + v1 * m(2,1) + v2 * m(2,2) + v3 * m(3,2);

	plDst.d   = v0 * m(0,3) + v1 * m(1,3) + v2 * m(2,3) + v3 * m(3,3);

	return plDst;
}

//===============================================================================================

#define MAX_PATH_LENGTH	512

#if !defined(LINUX) && !defined(PS3)	//than it does already exist
inline int vsnprintf(char * buf, int size, const char * format, va_list & args)
{
	int res = _vsnprintf_s(buf, size,size, format, args);
	assert(res>=0 && res<size); // just to know if there was problems in past
	buf[size-1]=0;
	return res;
}
#endif

#if !defined(LINUX) && !defined(PS3)
inline int snprintf(char * buf, int size, const char * format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	int res = _vsnprintf_s(buf, size,size, format, arglist);
	va_end(arglist);	
	return res;
}
#endif

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void Warning( const char *format,... ) PRINTF_PARAMS(1, 2);
inline void Warning( const char *format,... )
{
	va_list args;
	va_start(args, format);
	if (iSystem)
		iSystem->WarningV( VALIDATOR_MODULE_RENDERER,VALIDATOR_WARNING,0,NULL,format,args );
	va_end(args);
}

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void LogWarning( const char *format,... ) PRINTF_PARAMS(1, 2);
inline void LogWarning( const char *format,... )
{
	va_list args;
	va_start(args, format);
	if (iSystem)
		iSystem->WarningV( VALIDATOR_MODULE_RENDERER,VALIDATOR_WARNING,0,NULL,format,args );
	va_end(args);
}

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void FileWarning( const char*filename,const char *format,... ) PRINTF_PARAMS(2, 3);
inline void FileWarning( const char*filename,const char *format,... )
{
	va_list args;
	va_start(args, format);
	if (iSystem)
		iSystem->WarningV( VALIDATOR_MODULE_RENDERER,VALIDATOR_WARNING,VALIDATOR_FLAG_FILE,filename,format,args );
	va_end(args);
}

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void TextureWarning( const char *filename,const char *format,... ) PRINTF_PARAMS(2, 3);
inline void TextureWarning( const char *filename,const char *format,... )
{
	va_list args;
	va_start(args, format);
	if (iSystem)
		iSystem->WarningV( VALIDATOR_MODULE_RENDERER,VALIDATOR_WARNING, (VALIDATOR_FLAG_FILE|VALIDATOR_FLAG_TEXTURE), filename,format,args );
	va_end(args);
}

inline void TextureError( const char *filename,const char *format,... )
{
	va_list args;
	va_start(args, format);
	if (iSystem)
		iSystem->WarningV( VALIDATOR_MODULE_RENDERER,VALIDATOR_ERROR, (VALIDATOR_FLAG_FILE|VALIDATOR_FLAG_TEXTURE), filename,format,args );
	va_end(args);
}

_inline void _SetVar(const char *szVarName, int nVal)
{
	ICVar *var = iConsole->GetCVar(szVarName);
	if (var)
		var->Set(nVal);
	else
	{
		assert(0);
	}
}

_inline void HeapCheck()
{
#if !defined(LINUX) && !defined (PS3)
	int Result = _heapchk();
	assert(Result!=_HEAPBADBEGIN);
	assert(Result!=_HEAPBADNODE);
	assert(Result!=_HEAPBADPTR);
	assert(Result!=_HEAPEMPTY);
	assert(Result==_HEAPOK);
#endif
}

const char* fpGetExtension (const char *in);
void fpStripExtension (const char *in, char *out);
void fpAddExtension (char *path, char *extension);
void fpConvertDOSToUnixName( char *dst, const char *src );
void fpConvertUnixToDosName( char *dst, const char *src );
void fpUsePath (char *name, char *path, char *dst);

//=========================================================================================

_inline void makeProjectionMatrix(float fovy, float aspect, float nearval, float farval, float *m)
{
	float left, right, bottom, top;

	top = nearval * cry_tanf(fovy * (float)M_PI / 360.0f);
	bottom = -top;

	left = bottom * aspect;
	right = top * aspect;


	float x, y, a, b, c, d;
	if ((nearval<=0.0f || farval<=0.0f) || (nearval == farval) || (left == right) || (top == bottom))
	{
		iLog->Log("Warning: makeProjectionMatrix: (near or far)\n");
		return;
	}
	x = (2.0f*nearval) / (right-left);
	y = (2.0f*nearval) / (top-bottom);
	a = (right+left) / (right-left);
	b = (top+bottom) / (top-bottom);
	c = -(farval+nearval) / ( farval-nearval);
	d = -(2.0f*farval*nearval) / (farval-nearval);
#define M(row,col)  m[col*4+row]
	M(0,0) = x;     M(0,1) = 0.0F;  M(0,2) = a;      M(0,3) = 0.0F;
	M(1,0) = 0.0F;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0F;
	M(2,0) = 0.0F;  M(2,1) = 0.0F;  M(2,2) = c;      M(2,3) = d;
	M(3,0) = 0.0F;  M(3,1) = 0.0F;  M(3,2) = -1.0F;  M(3,3) = 0.0F;
#undef M
}

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  product[(col<<2)+row]

_inline void matmul4( float *product, const float *a, const float *b )
{
	int i;
	for (i=0; i<4; i++)
	{
		float ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
		P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
		P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
		P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
		P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
	}
}

#undef A
#undef B
#undef P

//========================================================================================

//
// Normal timing.
//
#define ticks(Timer)   {Timer -= CryGetTicks();}
#define unticks(Timer) {Timer += CryGetTicks()+34;}

//=============================================================================

// the int 3 call for 32-bit version for .l-generated files.
#ifdef WIN64
#define LEX_DBG_BREAK
#else
#define LEX_DBG_BREAK DEBUG_BREAK
#endif

#include "Common/Defs.h"

#define FUNCTION_PROFILER_RENDERER FUNCTION_PROFILER_FAST( iSystem, PROFILE_RENDERER, g_bProfilerEnabled )

#if (defined (WIN32) || defined(XENON)) && !defined(PS3)
#define SHADER_ASYNC_COMPILATION
#endif

/*-----------------------------------------------------------------------------
The End.
-----------------------------------------------------------------------------*/
#endif
