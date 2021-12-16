////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2004.
// -------------------------------------------------------------------------
//  File name:   PS3Specific.h
//  Version:     v1.00
//  Created:     05/03/2004 by MichaelG.
//  Compilers:   Visual Studio.NET, GCC 3.2
//  Description: Specific to PS3 declarations, inline functions etc.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef _CRY_COMMON_PS3_SPECIFIC_HDR_
#define _CRY_COMMON_PS3_SPECIFIC_HDR_

#define FP16_MESH




#ifdef __CRYCG__
	// Undef harmful predefined macros from the frontend GCC of the code
	// generator.
	#undef __i386
	#undef __i386__
	#undef i386
	#undef __MMX__
	#undef __SSE__
	#undef __gnu_linux__
	#undef __linux
	#undef __linux__
	#undef __unix
	#undef __unix__
	#undef unix
#endif//__CRYCG

//////////////////////////////////////////////////////////////////////////
// Standart includes.
//////////////////////////////////////////////////////////////////////////
#include <ctype.h>
//#include <stdlib.h> 
//#include <time.h>
//#include <pthread.h>
//#include <pu_thread.h>
#include <string.h>
#include <errno.h>
//#include <sys/io.h>
#include <stddef.h>
#include <float.h>

#ifndef _isnan
	#define _isnan isnan
#endif

#ifndef __COUNTER__
#define __COUNTER__ __LINE__
#endif

#if defined(_LIB)
	extern unsigned short GetKeyState(int vKey);
#endif
#define CryGetAsyncKeyState GetAsyncKeyState











	#define __debugbreak() __asm__ volatile ( "tw 31,1,1" )


#define DebugBreak() __debugbreak()




	#include <sys/sys_time.h>
	#include <sys/time_util.h>
	#include <sys/types.h>
	#include <sys/paths.h>
	#include <sys/select.h>
	#include <sys/socket.h>
	#include <cell/keyboard.h>
//////////////////////////////////////////////////////////////////////////

	//temp definitions for GetAsynchKeyState, should not be used on PS3
	#define VK_SCROLL      CELL_KEYC_SCROLL_LOCK
	#define VK_PAUSE       CELL_KEYC_PAUSE
	#define VK_UP          CELL_KEYC_UP_ARROW
	#define VK_DOWN        CELL_KEYC_DOWN_ARROW   
	//KEY OFF - code
	#define VK_CONTROL     0
	#define VK_RIGHT       CELL_KEYC_RIGHT_ARROW
	#define VK_LEFT        CELL_KEYC_LEFT_ARROW
	#define VK_SPACE       CELL_KEYC_SPACE
	#define VK_ESCAPE			 CELL_KEYC_ESCAPE
	//not available
	#define VK_LBUTTON		 0xFF	

	#ifndef _PS3
		#define _PS3
	#endif

	//this defines enables all file loading from hard disk
	#define USE_HDD0
	#if defined(USE_HDD0)
		#include <sysutil/sysutil_common.h>
//		#include <sysutil/sysutil_gamedata.h>
	#endif
	//-------------------------------------socket stuff------------------------------------------

	#define select socketselect
	#define closesocket socketclose

	typedef int SOCKET;
	#define INVALID_SOCKET (-1)
	#define SOCKET_ERROR (-1)

	typedef struct in_addr_windows 
	{
		union 
		{
			struct { unsigned char s_b1,s_b2,s_b3,s_b4; } S_un_b;
			struct { unsigned short s_w1,s_w2; } S_un_w;
			unsigned int S_addr;
		} S_un;
	}in_addr_windows;

	#define WSAEINTR SYS_NET_EINTR
	#define WSAEBADF SYS_NET_EBADF
	#define WSAEACCES SYS_NET_EACCES
	#define WSAEFAULT SYS_NET_EFAULT
	#define WSAEACCES SYS_NET_EACCES
	#define WSAEFAULT SYS_NET_EFAULT
	#define WSAEINVAL SYS_NET_EINVAL
	#define WSAEMFILE SYS_NET_EMFILE
	#define WSAEWOULDBLOCK SYS_NET_EAGAIN
	#define WSAEINPROGRESS SYS_NET_EINPROGRESS
	#define WSAEALREADY SYS_NET_EALREADY
	#define WSAENOTSOCK SYS_NET_ENOTSOCK 
	#define WSAEDESTADDRREQ SYS_NET_EDESTADDRREQ
	#define WSAEMSGSIZE SYS_NET_EMSGSIZE
	#define WSAEPROTOTYPE SYS_NET_EPROTOTYPE
	#define WSAENOPROTOOPT SYS_NET_ENOPROTOOPT
	#define WSAEPROTONOSUPPORT SYS_NET_EPROTONOSUPPORT
	#define WSAESOCKTNOSUPPORT SYS_NET_ESOCKTNOSUPPORT
	#define WSAEOPNOTSUPP SYS_NET_EOPNOTSUPP
	#define WSAEPFNOSUPPORT SYS_NET_EPFNOSUPPORT
	#define WSAEAFNOSUPPORT SYS_NET_EAFNOSUPPORT
	#define WSAEADDRINUSE SYS_NET_EADDRINUSE
	#define WSAEADDRNOTAVAIL SYS_NET_EADDRNOTAVAIL
	#define WSAENETDOWN SYS_NET_ENETDOWN
	#define WSAENETUNREACH SYS_NET_ENETUNREACH
	#define WSAENETRESET SYS_NET_ENETRESET
	#define WSAECONNABORTED SYS_NET_ECONNABORTED
	#define WSAECONNRESET SYS_NET_ECONNRESET
	#define WSAENOBUFS SYS_NET_ENOBUFS
	#define WSAEISCONN SYS_NET_EISCONN
	#define WSAENOTCONN SYS_NET_ENOTCONN
	#define WSAESHUTDOWN SYS_NET_ESHUTDOWN
	#define WSAETOOMANYREFS SYS_NET_ETOOMANYREFS
	#define WSAETIMEDOUT SYS_NET_ETIMEDOUT
	#define WSAECONNREFUSED SYS_NET_ECONNREFUSED
	#define WSAELOOP SYS_NET_ELOOP
	#define WSAENAMETOOLONG SYS_NET_ENAMETOOLONG
	#define WSAEHOSTDOWN SYS_NET_EHOSTDOWN
	#define WSAEHOSTUNREACH SYS_NET_EHOSTUNREACH
	#define WSAENOTEMPTY SYS_NET_ENOTEMPTY
	#define WSAEPROCLIM SYS_NET_EPROCLIM
	#define WSAEUSERS SYS_NET_EUSERS
	#define WSAEDQUOT SYS_NET_EDQUOT
	#define WSAESTALE SYS_NET_ESTALE
	#define WSAEREMOTE SYS_NET_EREMOTE

	#define WSAHOST_NOT_FOUND (1024 + 1)
	#define WSATRY_AGAIN (1024 + 2)
	#define WSANO_RECOVERY (1024 + 3)
	#define WSANO_DATA (1024 + 4)
	#define WSANO_ADDRESS (WSANO_DATA)

	#define SD_RECEIVE      SHUT_RD
	#define SD_SEND         SHUT_WR
	#define SD_BOTH         SHUT_RDWR

	//-------------------------------------end socket stuff------------------------------------------

	#define SCOPED_ENABLE_FLOAT_EXCEPTIONS

	// Flag indicating if the system threading library should be used instead
	// of the POSIX threading library.
	//#undef USE_SYSTEM_THREADS
//	#define USE_SYSTEM_THREADS 1



#define SIZEOF_PTR 4

//built in stack allocation
#undef alloca

#undef __db16cycl__













	#ifdef __SNC__
    #define __db16cycl__ __db16cyc();
	#else
		#define alloca(size) __builtin_alloca(size)
		#define __db16cycl__ __asm__ volatile("db16cyc");
	#endif


#undef IF
#undef WHILE
#if !defined(__SPU__)
	//dummy for cache control in spu jobs (must stay compilable on PPU too)
	inline void SPUAddCacheWriteRangeAsync(const unsigned int, const unsigned int){}
	#define __cache_range_write_async(a,b)
	#define __spu_flush_cache_line(a)
	#define __flush_cache_range(a,b)
	#define __flush_cache()
#endif//__SPU__
#if !defined(__SPU__) || !defined(SUPP_BRANCH_HINTS)
	#define IF(a, b) if((a))
	#define WHILE(a, b) while((a))



#endif //!defined(__SPU__) || !defined(SUPP_BRANCH_HINTS)
//#define _CPU_X86

#ifndef DEBUG_BREAK



	#define DEBUG_BREAK

#endif
#define RC_EXECUTABLE "rc"

#if !defined(USE_STATIC_NAME_TABLE)
#define USE_STATIC_NAME_TABLE 1
#endif
//#if !defined(_STLP_HASH_MAP)
//#define _STLP_HASH_MAP 1
//#endif
#define USE_CRT 1
#define TYPENAME(x) "<Not Supported>"

#define stricmp strcasecmp


//////////////////////////////////////////////////////////////////////////
// Define platform independent types.
//////////////////////////////////////////////////////////////////////////
typedef void*								LPVOID;
#define VOID            		void
#define PVOID								void*

typedef signed char         int8;
typedef signed char         INT8;
typedef signed short        int16;
typedef signed short        INT16;
typedef signed int					int32;
typedef int									INT32;
typedef signed long long		int64;
typedef signed long long		INT64;
typedef unsigned char				uint8;
typedef unsigned char				UINT8;
typedef unsigned short			uint16;
typedef unsigned short			UINT16;
typedef unsigned int				uint32;
typedef unsigned int				UINT32;
typedef unsigned long long	uint64;

typedef float               f32;
typedef double              f64;

//typedef double              real;  //biggest float-type on this machine
typedef float              real;  //biggest float-type on this machine
#define REAL_IS_FLOAT 1

/*
	pointers on SPU side are 4 byte rather than on PPU with 8 byte
	if a structure is to be used on PPU and SPU, they need to be the same in memory
	therefore pointers need to be specified with _spu_pad_(index) after the name:
		struct Foo
		{
			int* p _spu_pad_(0);
			int* p1 _spu_pad_(1);
		};
	if multiple pointers are within a struct, use _spu_pad_(0), _spu_pad_(1) for the 2nd, 3rd and so on
*/



typedef intptr_t INT_PTR, *PINT_PTR;
typedef uintptr_t UINT_PTR, *PUINT_PTR;

typedef char *LPSTR, *PSTR;

typedef long LONG_PTR, *PLONG_PTR, *PLONG;
typedef unsigned long ULONG_PTR, *PULONG_PTR;

typedef std::uint32_t				DWORD;
typedef uintptr_t						DWORD_PTR;
typedef unsigned long*			LPDWORD;
typedef unsigned char				BYTE;
typedef unsigned short			WORD;
typedef int                 INT;
typedef unsigned int        UINT;
typedef float               FLOAT;
typedef void*								HWND;
typedef UINT_PTR 						WPARAM;
typedef LONG_PTR 						LPARAM;
typedef LONG_PTR 						LRESULT;
#define PLARGE_INTEGER LARGE_INTEGER*
typedef const char *LPCSTR, *PCSTR;
typedef long long						LONGLONG;
typedef	ULONG_PTR						SIZE_T;
typedef unsigned char				byte;

typedef int32 __int32;
typedef uint32 __uint32;
typedef int64 __int64;
typedef uint64 __uint64;

//shortens alignment declaration
#define _PACK __attribute__ ((packed))

#if defined __CRYCG__
	#undef _ALIGN
	#define _ALIGN(num) __attribute__ ((aligned (num), crycg_attr (aligned, num)))
	#undef ILINE
	#define ILINE __attribute__((crycg_attr("inline")))
#else
	#undef _ALIGN
	#define _ALIGN(num) __attribute__ ((aligned(num)))	
	#undef ILINE
	#if !defined(_DEBUG) && !defined(NO_ILINE)
		#define ILINE __attribute__((always_inline)) inline
	#else
		#define ILINE inline
	#endif
#endif

#define UINT32_C(x)	x##U
#define INT32_C(x)	x
#define UINT64_C(x)	x##Ull
#define INT64_C(x)	x##ll
#define UINT16_C(x)	x
#define INT16_C(x)	x
#define UINT8_C(x)	x
#define INT8_C(x)	x


//#define PHYSICS_EXPORTS

// MSVC compiler-specific keywords
#define _inline ILINE
#define __cdecl
#define _cdecl
#define _stdcall
#define __stdcall
#define _fastcall
#define __fastcall

// Safe memory freeing
#ifndef SAFE_DELETE
	#define SAFE_DELETE(p)			{ if(p) { delete (p);		(p)=NULL; } }
#endif

#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY(p)	{ if(p) { delete[] (p);		(p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(p)			{ if(p) { (p)->Release();	(p)=NULL; } }
#endif

#ifndef SAFE_RELEASE_FORCE
	#define SAFE_RELEASE_FORCE(p)			{ if(p) { (p)->ReleaseForce();	(p)=NULL; } }
#endif

#if defined __CRYCG__
	#define DEFINE_ALIGNED_DATA( type, name, alignment ) \
		type __attribute__ ((aligned(alignment))) __attribute__ ((crycg_attr (aligned, alignment))) name;
	#define DEFINE_ALIGNED_DATA_STATIC( type, name, alignment ) \
		static type __attribute__ ((aligned(alignment))) __attribute__ ((crycg_attr (aligned, alignment))) name;
	#define DEFINE_ALIGNED_DATA_CONST( type, name, alignment ) \
		const type __attribute__ ((aligned(alignment))) __attribute__ ((crycg_attr (aligned, alignment))) name;
#else
	#define DEFINE_ALIGNED_DATA( type, name, alignment ) \
		type __attribute__ ((aligned(alignment))) name;
	#define DEFINE_ALIGNED_DATA_STATIC( type, name, alignment ) \
		static type __attribute__ ((aligned(alignment))) name;
	#define DEFINE_ALIGNED_DATA_CONST( type, name, alignment ) \
		const type __attribute__ ((aligned(alignment))) name;
#endif

#define MAKEWORD(a, b)      ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD_PTR)(l) >> 16))
#define LOBYTE(w)           ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((DWORD_PTR)(w) >> 8))

#define CALLBACK
#define WINAPI

#ifndef __cplusplus
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define TCHAR wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif
typedef char CHAR;
typedef wchar_t WCHAR;    // wc,   16-bit UNICODE character
typedef WCHAR *PWCHAR;
typedef WCHAR *LPWCH, *PWCH;
typedef const WCHAR *LPCWCH, *PCWCH;
typedef WCHAR *NWPSTR;
typedef WCHAR *LPWSTR, *PWSTR;
typedef WCHAR *LPUWSTR, *PUWSTR;

typedef const WCHAR *LPCWSTR, *PCWSTR;
typedef const WCHAR *LPCUWSTR, *PCUWSTR;

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |       \
	((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#define FILE_ATTRIBUTE_NORMAL               0x00000080

typedef int							BOOL;
typedef long						LONG;
typedef unsigned long		ULONG;
typedef long 						HRESULT;

#define _PTRDIFF_T_DEFINED

#define TRUE 1
#define FALSE 0

#undef _MAX_PATH 
#undef MAX_PATH
#define MAX_PATH 1024
#define _MAX_PATH MAX_PATH

#ifdef __cplusplus













template<typename S, typename T>
inline const S __min(const S rS, const T rT)
{
	return (rS <= rT)?rS : (const S)rT;
}

template<typename S, typename T>
inline const S __max(const S rS, const T rT)
{
	return (rS >= rT)?rS : (const S)rT;
}


//////////////////////////////////////////////////////////////////////////
#define NEED_ENDIAN_SWAP

//////////////////////////////////////////////////////////////////////////
extern size_t _msize(void *p);

//////////////////////////////////////////////////////////////////////////
// Multi platform Hi resolution ticks function, should only be used for profiling.
//////////////////////////////////////////////////////////////////////////
#if !defined(__SPU__)
ILINE int64 CryGetTicks()
{
	uint64 ticks;
	SYS_TIMEBASE_GET(ticks);
	return static_cast<int64>(ticks);
}


#endif

#endif //__cplusplus

#if !defined(__SPU__)
  // PS3_Win32Wrapper.h now directly included by platform.h.
	//#include "PS3_Win32Wrapper.h"
	void sys_timer_usleep_nops( unsigned int dwMicroSeconds );
	#define sys_timer_subusleep_inldb16cyc( dwMicroSeconds )({\
		for(int i=0; i<(dwMicroSeconds) * 4; ++i)\
		{\
			__db16cycl__\
			__db16cycl__\
			__db16cycl__\
			__db16cycl__\
		}})
	
	#define sys_timer_subusleep_inlnops( dwMicroSeconds )({\
		for(int i=0; i<(dwMicroSeconds) * 4; ++i)\
		{\
			asm volatile("nop");\
			asm volatile("nop");\
			asm volatile("nop");\
			asm volatile("nop");\
		}})

















#endif //SPU

//conditional selects

//implements branch free: 
//return (cA > cB)?cA : cB;
#if !defined(__SPU__)
template <class T>
T CondSelMax(const T cA, const T cB);
template <class T>
T CondSelMin(const T cA, const T cB);

template <class T>
__attribute__((always_inline))
inline T CondSelMax(const T cA, const T cB)
{
	const uint32 cMinMask = (uint32)(((int32)(cA - cB)) >> 31);
	return (cA & ~cMinMask | cB & cMinMask);
}

//implements branch free for integer types: 
//return (cA < cB)?cA : cB;
template <class T>
__attribute__((always_inline))
inline T CondSelMin(const T cA, const T cB)
{
	const uint32 cMinMask = (uint32)(((int32)(cB - cA)) >> 31);
	return (cA & ~cMinMask | cB & cMinMask);
}

template <>
__attribute__((always_inline))
inline float CondSelMin<float>(const float cA, const float cB)
{
	return (cA < cB)?cA : cB;
}

template <>
__attribute__((always_inline))
inline float CondSelMax<float>(const float cA, const float cB)
{
	return (cA > cB)?cA : cB;
}
#endif














































































































































#if defined(__CRYCG__)
void *operator new(size_t, void *);
#endif

extern uint32 CryMallocRSX(const unsigned int cSize, void*& rpMem);
extern void*	CryMallocRSXMem();//size is passed during initialization
extern uint32 CryMallocRSXMemSize();

typedef uint64	tdResHandle;
tdResHandle	CryRSXMemCpySync(void* pDst,const void* pSrc,uint32 Size);
tdResHandle	CryRSXMemCpySyncScaled2D(			void* pDst,uint32 DstSizeX,uint32 DstSizeY,uint32 DstPitch,
																		const void* pSrc,uint32 SrcSizeX,uint32 SrcSizeY,uint32 SrcPitch);
tdResHandle	CryRSXMemCpyASync(void* pDst,const void* pSrc,uint32 Size);
tdResHandle	CryRSXMemCpyASyncScaled2D(		void* pDst,uint32 DstSizeX,uint32 DstSizeY,uint32 DstPitch,
																		const void* pSrc,uint32 SrcSizeX,uint32 SrcSizeY,uint32 SrcPitch);

// Devirtualization.
#ifdef _DEVIRTUALIZE_
  #define UNIQUE_IFACE
  #define VIRTUAL
  #define UNIQUE_VIRTUAL_WRAPPER(NAME) <$IFACEWRAP_ ## NAME>
	#define DEVIRTUALIZE_HEADER_FIX( HEADER ) <HEADER>
	#define DEVIRTUALIZATION_VTABLE_FIX virtual void vtable_dummy_function();
	#define DEVIRTUALIZATION_VTABLE_FIX_IMPL( CLASS ) void CLASS :: vtable_dummy_function(){}
#else
  #define UNIQUE_IFACE
  #define VIRTUAL virtual
  #define UNIQUE_VIRTUAL_WRAPPER(NAME) <NoIfaceWrapper.h>
	#define DEVIRTUALIZE_HEADER_FIX( HEADER ) <NoIfaceWrapper.h>
	#define DEVIRTUALIZATION_VTABLE_FIX
	#define DEVIRTUALIZATION_VTABLE_FIX_IMPL( CLASS )
#endif

































	#include <sys/ppu_thread.h>
	ILINE DWORD GetCurrentThreadId()
	{
    int thread_id = 0; 
    __asm__
    (
     "or %0,13,13\n" 
     : "=r" (thread_id) : :  );
    return thread_id;
	}
	//used for frameprofiler to obtain real id
	ILINE DWORD GetCurrentThreadSystemId()
	{
		sys_ppu_thread_t thread;
  	sys_ppu_thread_get_id(&thread);
  	return static_cast<DWORD>(thread);
	}

	//redefine used intrinsics to avoid including the broken gcc file containing vector the keyword
	ILINE static int __si_to_int(qword q) 
	{
		union{qword q;int i[4];} x;x.q = q;return (x.i[0]);
	}

	ILINE static qword __si_gb(qword a)
	{
		vec_uint4 bits;
		vec_uint4 bytes;
		bits  = vec_sl(vec_and((vec_uint4)(a), vec_splat_u32(1)), ((vec_uint4){3, 2, 1, 0}));
		bytes = (vec_uint4)vec_sums((vec_int4)(bits), ((vec_int4){0}));
		return ((qword)(vec_sld(bytes, bytes, 12)));
	}

	ILINE static qword __si_shufb(qword a, qword b, qword pattern)
	{
		vec_uchar16 pat;
	  pat = vec_sel(((vec_uchar16){0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15}), 
			vec_sr((vec_uchar16)(pattern), vec_splat_u8(3)),
			vec_sra((vec_uchar16)(pattern), vec_splat_u8(7)));
		return ((qword)(vec_perm(vec_perm(a, b, pattern), 
			   ((vec_uchar16){0, 0, 0, 0, 0, 0, 0, 0,0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80}),pat)));
	}

	ILINE static qword __si_fma(qword a, qword b, qword c)
	{
		return ((qword)(vec_madd((vec_float4)(a), (vec_float4)(b), (vec_float4)(c))));
	}

	ILINE static qword __si_fms(qword a, qword b, qword c)
	{
		return ((qword)(vec_sub(vec_madd((vec_float4)(a), (vec_float4)(b), (vec_float4){0.f,0.f,0.f,0.f}),(vec_float4)(c))));
	}

	ILINE static qword __si_fa(qword a, qword b)
	{
		return ((qword)(vec_add((vec_float4)(a), (vec_float4)(b))));
	}


	ILINE static qword __si_a(qword a, qword b)
	{
		return (qword)vec_add((vec_uint4)a, (vec_uint4)b);
	}

	ILINE static qword __si_sfh(qword a, qword b)
	{
		//be careful, parameter swap needed
		return (qword)vec_sub((vec_ushort8)b, (vec_ushort8)a);
	}

	ILINE static qword __si_ah(qword a, qword b)
	{
		return (qword)vec_add((vec_ushort8)a, (vec_ushort8)b);
	}

	ILINE static vec_uint4 __spu_splats(unsigned int a)
	{
		union {	vec_uint4 v; unsigned int i[4];} in;
		in.i[0] = a;
		return (vec_splat(in.v, 0));
	}

	ILINE static qword __si_clgth(qword a,qword b)
	{
		return (qword)vec_cmpgt((vec_ushort8)a,(vec_ushort8)b);
	}

	ILINE static qword __si_rotmai(qword a, int b)
	{
		const vec_uint4	B	=	__spu_splats(abs(b));
		return b<0?	(qword)vec_sra((vec_int4)a,B):
								(qword)vec_sl((vec_int4)a,B);
	}

	ILINE static qword __si_from_float(float f) 
	{
		union {qword q;float f[4];} x;x.f[0] = f;return (x.q);
	}

	ILINE static qword __si_or(qword a, qword b)
	{
		return ((qword)(vec_or((vec_uchar16)(a), (vec_uchar16)(b))));
	}

	ILINE static qword __si_selb(qword a, qword b, qword c)
	{
		return ((qword)(vec_sel((vec_uchar16)(a), (vec_uchar16)(b), (vec_uchar16)(c))));
	}

	ILINE static qword __si_fs(qword a, qword b)
	{
		return ((qword)(vec_sub((vec_float4)(a), (vec_float4)(b))));
	}

	ILINE static qword __si_rotqby(qword a, qword count)
	{
		vec_uchar16 left = vec_sl(vec_splat((vec_uchar16)(count), 3), vec_splat_u8(3));
		vec_uchar16 right = vec_sub(vec_splat_u8(0), left);
		return ((qword)(vec_or(vec_slo((vec_uchar16)(a), left), vec_sro((vec_uchar16)(a), right))));
	}

	ILINE static qword __si_fnms(qword a, qword b, qword c)
	{
		return ((qword)(vec_nmsub((vec_float4)(a), (vec_float4)(b), (vec_float4)(c))));
	}

	ILINE static qword __si_from_int(int i) 
	{
		union {	qword q; int i[4];} x;
		x.i[0] = i;
		return (x.q);
	}

	ILINE static qword __si_fsmb(qword a)
	{
	  vec_ushort8 in = (vec_ushort8)(a);
		vec_char16 mask = (vec_char16)(vec_perm(in, in, ((vec_uchar16){2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3})));
		return ((qword)(vec_sra(vec_sl(mask, ((vec_uchar16){0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7})), vec_splat_u8(7))));
	}

//	#define __si_cfltu(_a, _b)	((qword)(vec_ctu((vec_float4)(_a), _b)))

	ILINE static qword __si_cfltu(qword a,uint64 b) 
	{
		union {qword q;float f[4];uint32 i[4];} x;
		const float Scale	=	static_cast<float>(1LL<<b);
		const float Limit	=	static_cast<float>((1LL<<32LL)-1LL);
		x.q	=	a;
		x.f[0]	*=	Scale;
		x.f[1]	*=	Scale;
		x.f[2]	*=	Scale;
		x.f[3]	*=	Scale;
		x.i[0]	=	static_cast<uint32>(x.f[0]<0.f?0.f:x.f[0]>Limit?Limit:x.f[0]);
		x.i[1]	=	static_cast<uint32>(x.f[1]<0.f?0.f:x.f[1]>Limit?Limit:x.f[1]);
		x.i[2]	=	static_cast<uint32>(x.f[2]<0.f?0.f:x.f[2]>Limit?Limit:x.f[2]);
		x.i[3]	=	static_cast<uint32>(x.f[3]<0.f?0.f:x.f[3]>Limit?Limit:x.f[3]);
		return x.q;
	}

	ILINE static qword __si_fcgt(qword a, qword b)
	{
		return ((qword)(vec_cmpgt((vec_float4)(a), (vec_float4)(b))));
	}

	ILINE static qword __si_fm(qword a, qword b)
	{
		return ((qword)(vec_madd((vec_float4)(a), (vec_float4)(b), ((vec_float4){0.0f}))));
	}

	ILINE static qword __si_frest(qword a)
	{
		return (qword)vec_re((vec_float4)a);
	}

	ILINE static qword __si_fi(qword a, qword b)
	{
		return (qword)vec_madd((vec_float4)b,vec_nmsub((vec_float4)b,(vec_float4)a,(vec_float4){1.f,1.f,1.f,1.f}),(vec_float4)b);
	}

	ILINE static qword __si_and(qword a, qword b)
	{
		return ((qword)(vec_and((vec_uint4)(a), (vec_uint4)(b))));
	}

	ILINE static qword __si_xor(qword a, qword b)
	{
		return ((qword)(vec_xor((vec_uint4)(a), (vec_uint4)(b))));
	}

	ILINE static qword __si_ceq(qword a, qword b)
	{
		return ((qword)(vec_cmpeq((vec_uint4)(a), (vec_uint4)(b))));
	}


#define __forceinline ILINE

#endif //_CRY_COMMON_PS3_SPECIFIC_HDR_

