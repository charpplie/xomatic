//--------------------------------------------------------------------------------------
// File: DXUTMisc.cpp
//
// Shortcut macros and functions for using DX objects
//
// Copyright (c) Microsoft Corporation. All rights reserved
//--------------------------------------------------------------------------------------
#include "StdAfx.h"
#include "../DriverD3D.h"

#if !defined(XENON)

#define UNICODE 1
#include "DXUT.h"
#undef min // use __min instead
#undef max // use __max instead

CDXUTTimer* WINAPI DXUTGetGlobalTimer()
{
    // Using an accessor function gives control of the construction order
    static CDXUTTimer timer;
    return &timer;
}


//--------------------------------------------------------------------------------------
CDXUTTimer::CDXUTTimer()
{
    m_bTimerStopped     = true;
    m_llQPFTicksPerSec  = 0;

    m_llStopTime        = 0;
    m_llLastElapsedTime = 0;
    m_llBaseTime        = 0;

    // Use QueryPerformanceFrequency to get the frequency of the counter
    LARGE_INTEGER qwTicksPerSec;
    QueryPerformanceFrequency( &qwTicksPerSec );
    m_llQPFTicksPerSec = qwTicksPerSec.QuadPart;
}


//--------------------------------------------------------------------------------------
void CDXUTTimer::Reset()
{
    LARGE_INTEGER qwTime = GetAdjustedCurrentTime();
    
    m_llBaseTime        = qwTime.QuadPart;
    m_llLastElapsedTime = qwTime.QuadPart;
    m_llStopTime        = 0;
    m_bTimerStopped     = FALSE;
}


//--------------------------------------------------------------------------------------
void CDXUTTimer::Start()
{
    // Get the current time
    LARGE_INTEGER qwTime;
    QueryPerformanceCounter( &qwTime );

    if( m_bTimerStopped )
        m_llBaseTime += qwTime.QuadPart - m_llStopTime;
    m_llStopTime = 0;
    m_llLastElapsedTime = qwTime.QuadPart;
    m_bTimerStopped = FALSE;
}


//--------------------------------------------------------------------------------------
void CDXUTTimer::Stop()
{
    if( !m_bTimerStopped )
    {
        LARGE_INTEGER qwTime;
        QueryPerformanceCounter( &qwTime );
        m_llStopTime = qwTime.QuadPart;
        m_llLastElapsedTime = qwTime.QuadPart;
        m_bTimerStopped = TRUE;
    }
}


//--------------------------------------------------------------------------------------
void CDXUTTimer::Advance()
{
    m_llStopTime += m_llQPFTicksPerSec/10;
}


//--------------------------------------------------------------------------------------
double CDXUTTimer::GetAbsoluteTime()
{
    LARGE_INTEGER qwTime;
    QueryPerformanceCounter( &qwTime );

    double fTime = qwTime.QuadPart / (double) m_llQPFTicksPerSec;

    return fTime;
}


//--------------------------------------------------------------------------------------
double CDXUTTimer::GetTime()
{
    LARGE_INTEGER qwTime = GetAdjustedCurrentTime();

    double fAppTime = (double) ( qwTime.QuadPart - m_llBaseTime ) / (double) m_llQPFTicksPerSec;

    return fAppTime;
}


//--------------------------------------------------------------------------------------
void CDXUTTimer::GetTimeValues( double* pfTime, double* pfAbsoluteTime, float* pfElapsedTime )
{
    assert( pfTime && pfAbsoluteTime && pfElapsedTime );    

    LARGE_INTEGER qwTime = GetAdjustedCurrentTime();

    float fElapsedTime = (float) ((double) ( qwTime.QuadPart - m_llLastElapsedTime ) / (double) m_llQPFTicksPerSec);
    m_llLastElapsedTime = qwTime.QuadPart;

    // Clamp the timer to non-negative values to ensure the timer is accurate.
    // fElapsedTime can be outside this range if processor goes into a 
    // power save mode or we somehow get shuffled to another processor.  
    // However, the main thread should call SetThreadAffinityMask to ensure that 
    // we don't get shuffled to another processor.  Other worker threads should NOT call 
    // SetThreadAffinityMask, but use a shared copy of the timer data gathered from 
    // the main thread.
    if( fElapsedTime < 0.0f )
        fElapsedTime = 0.0f;
    
    *pfAbsoluteTime = qwTime.QuadPart / (double) m_llQPFTicksPerSec;
    *pfTime = ( qwTime.QuadPart - m_llBaseTime ) / (double) m_llQPFTicksPerSec;   
    *pfElapsedTime = fElapsedTime;
}


//--------------------------------------------------------------------------------------
double CDXUTTimer::GetElapsedTime()
{
    LARGE_INTEGER qwTime = GetAdjustedCurrentTime();

    double fElapsedTime = (double) ( qwTime.QuadPart - m_llLastElapsedTime ) / (double) m_llQPFTicksPerSec;
    m_llLastElapsedTime = qwTime.QuadPart;

    // See the explanation about clamping in CDXUTTimer::GetTimeValues()
    if( fElapsedTime < 0.0f )
        fElapsedTime = 0.0f;

    return fElapsedTime;
}


//--------------------------------------------------------------------------------------
// If stopped, returns time when stopped otherwise returns current time
//--------------------------------------------------------------------------------------
LARGE_INTEGER CDXUTTimer::GetAdjustedCurrentTime()
{
    LARGE_INTEGER qwTime;
    if( m_llStopTime != 0 )
        qwTime.QuadPart = m_llStopTime;
    else
        QueryPerformanceCounter( &qwTime );
    return qwTime;
}

//--------------------------------------------------------------------------------------
bool CDXUTTimer::IsStopped()
{
    return m_bTimerStopped;
}


//--------------------------------------------------------------------------------------
// Returns the string for the given D3DFORMAT.
//--------------------------------------------------------------------------------------
LPCWSTR WINAPI DXUTD3DFormatToString( D3DFORMAT format, bool bWithPrefix )
{
    WCHAR* pstr = NULL;
    switch( format )
    {
    case D3DFMT_UNKNOWN:         pstr = L"D3DFMT_UNKNOWN"; break;
    case D3DFMT_R8G8B8:          pstr = L"D3DFMT_R8G8B8"; break;
    case D3DFMT_A8R8G8B8:        pstr = L"D3DFMT_A8R8G8B8"; break;
    case D3DFMT_X8R8G8B8:        pstr = L"D3DFMT_X8R8G8B8"; break;
    case D3DFMT_R5G6B5:          pstr = L"D3DFMT_R5G6B5"; break;
    case D3DFMT_X1R5G5B5:        pstr = L"D3DFMT_X1R5G5B5"; break;
    case D3DFMT_A1R5G5B5:        pstr = L"D3DFMT_A1R5G5B5"; break;
    case D3DFMT_A4R4G4B4:        pstr = L"D3DFMT_A4R4G4B4"; break;
    case D3DFMT_R3G3B2:          pstr = L"D3DFMT_R3G3B2"; break;
    case D3DFMT_A8:              pstr = L"D3DFMT_A8"; break;
    case D3DFMT_A8R3G3B2:        pstr = L"D3DFMT_A8R3G3B2"; break;
    case D3DFMT_X4R4G4B4:        pstr = L"D3DFMT_X4R4G4B4"; break;
    case D3DFMT_A2B10G10R10:     pstr = L"D3DFMT_A2B10G10R10"; break;
    case D3DFMT_A8B8G8R8:        pstr = L"D3DFMT_A8B8G8R8"; break;
    case D3DFMT_X8B8G8R8:        pstr = L"D3DFMT_X8B8G8R8"; break;
    case D3DFMT_G16R16:          pstr = L"D3DFMT_G16R16"; break;
    case D3DFMT_A2R10G10B10:     pstr = L"D3DFMT_A2R10G10B10"; break;
    case D3DFMT_A16B16G16R16:    pstr = L"D3DFMT_A16B16G16R16"; break;
    case D3DFMT_A8P8:            pstr = L"D3DFMT_A8P8"; break;
    case D3DFMT_P8:              pstr = L"D3DFMT_P8"; break;
    case D3DFMT_L8:              pstr = L"D3DFMT_L8"; break;
    case D3DFMT_A8L8:            pstr = L"D3DFMT_A8L8"; break;
    case D3DFMT_A4L4:            pstr = L"D3DFMT_A4L4"; break;
    case D3DFMT_V8U8:            pstr = L"D3DFMT_V8U8"; break;
    case D3DFMT_L6V5U5:          pstr = L"D3DFMT_L6V5U5"; break;
    case D3DFMT_X8L8V8U8:        pstr = L"D3DFMT_X8L8V8U8"; break;
    case D3DFMT_Q8W8V8U8:        pstr = L"D3DFMT_Q8W8V8U8"; break;
    case D3DFMT_V16U16:          pstr = L"D3DFMT_V16U16"; break;
    case D3DFMT_A2W10V10U10:     pstr = L"D3DFMT_A2W10V10U10"; break;
    case D3DFMT_UYVY:            pstr = L"D3DFMT_UYVY"; break;
    case D3DFMT_YUY2:            pstr = L"D3DFMT_YUY2"; break;
    case D3DFMT_DXT1:            pstr = L"D3DFMT_DXT1"; break;
    case D3DFMT_DXT2:            pstr = L"D3DFMT_DXT2"; break;
    case D3DFMT_DXT3:            pstr = L"D3DFMT_DXT3"; break;
    case D3DFMT_DXT4:            pstr = L"D3DFMT_DXT4"; break;
    case D3DFMT_DXT5:            pstr = L"D3DFMT_DXT5"; break;
    case D3DFMT_D16_LOCKABLE:    pstr = L"D3DFMT_D16_LOCKABLE"; break;
    case D3DFMT_D32:             pstr = L"D3DFMT_D32"; break;
    case D3DFMT_D15S1:           pstr = L"D3DFMT_D15S1"; break;
    case D3DFMT_D24S8:           pstr = L"D3DFMT_D24S8"; break;
    case D3DFMT_D24X8:           pstr = L"D3DFMT_D24X8"; break;
    case D3DFMT_D24X4S4:         pstr = L"D3DFMT_D24X4S4"; break;
    case D3DFMT_D16:             pstr = L"D3DFMT_D16"; break;
    case D3DFMT_L16:             pstr = L"D3DFMT_L16"; break;
    case D3DFMT_VERTEXDATA:      pstr = L"D3DFMT_VERTEXDATA"; break;
    case D3DFMT_INDEX16:         pstr = L"D3DFMT_INDEX16"; break;
    case D3DFMT_INDEX32:         pstr = L"D3DFMT_INDEX32"; break;
    case D3DFMT_Q16W16V16U16:    pstr = L"D3DFMT_Q16W16V16U16"; break;
    case D3DFMT_MULTI2_ARGB8:    pstr = L"D3DFMT_MULTI2_ARGB8"; break;
    case D3DFMT_R16F:            pstr = L"D3DFMT_R16F"; break;
    case D3DFMT_G16R16F:         pstr = L"D3DFMT_G16R16F"; break;
    case D3DFMT_A16B16G16R16F:   pstr = L"D3DFMT_A16B16G16R16F"; break;
    case D3DFMT_R32F:            pstr = L"D3DFMT_R32F"; break;
    case D3DFMT_G32R32F:         pstr = L"D3DFMT_G32R32F"; break;
    case D3DFMT_A32B32G32R32F:   pstr = L"D3DFMT_A32B32G32R32F"; break;
    case D3DFMT_CxV8U8:          pstr = L"D3DFMT_CxV8U8"; break;
    default:                     pstr = L"Unknown format"; break;
    }
#if defined (DIRECT3D9)
    if( bWithPrefix || wcsstr( pstr, L"D3DFMT_" )== NULL )
        return pstr;
    else
        return pstr + lstrlenW( L"D3DFMT_" );
#else
    return pstr;
#endif
}

//--------------------------------------------------------------------------------------
// Outputs to the debug stream a formatted Unicode string with a variable-argument list.
//--------------------------------------------------------------------------------------
VOID WINAPI DXUTOutputDebugStringW( LPCWSTR strMsg, ... )
{
#if defined (DIRECT3D9)
#if defined(DEBUG) || defined(_DEBUG)
    WCHAR strBuffer[512];
    
    va_list args;
    va_start(args, strMsg);
    StringCchVPrintfW( strBuffer, 512, strMsg, args );
    strBuffer[511] = L'\0';
    va_end(args);

    OutputDebugStringW( strBuffer );
#else
    UNREFERENCED_PARAMETER(strMsg);
#endif
#endif
}


//--------------------------------------------------------------------------------------
// Outputs to the debug stream a formatted MBCS string with a variable-argument list.
//--------------------------------------------------------------------------------------
VOID WINAPI DXUTOutputDebugStringA( LPCSTR strMsg, ... )
{
#if defined (DIRECT3D9)
#if defined(DEBUG) || defined(_DEBUG)
    CHAR strBuffer[512];
    
    va_list args;
    va_start(args, strMsg);
    StringCchVPrintfA( strBuffer, 512, strMsg, args );
    strBuffer[511] = '\0';
    va_end(args);

    OutputDebugStringA( strBuffer );
#else
    UNREFERENCED_PARAMETER(strMsg);
#endif
#endif
}


//--------------------------------------------------------------------------------------
// Direct3D9 dynamic linking support -- calls top-level D3D9 APIs with graceful
// failure if APIs are not present.
//--------------------------------------------------------------------------------------

// Function prototypes
typedef IDirect3D9* (WINAPI * LPDIRECT3DCREATE9) (UINT);
typedef INT         (WINAPI * LPD3DPERF_BEGINEVENT)(D3DCOLOR, LPCWSTR);
typedef INT         (WINAPI * LPD3DPERF_ENDEVENT)(void);
typedef VOID        (WINAPI * LPD3DPERF_SETMARKER)(D3DCOLOR, LPCWSTR);
typedef VOID        (WINAPI * LPD3DPERF_SETREGION)(D3DCOLOR, LPCWSTR);
typedef BOOL        (WINAPI * LPD3DPERF_QUERYREPEATFRAME)(void);
typedef VOID        (WINAPI * LPD3DPERF_SETOPTIONS)( DWORD dwOptions );
typedef DWORD       (WINAPI * LPD3DPERF_GETSTATUS)( void );

// Module and function pointers
static HMODULE s_hModD3D9 = NULL;
static LPDIRECT3DCREATE9 s_DynamicDirect3DCreate9 = NULL;
static LPD3DPERF_BEGINEVENT s_DynamicD3DPERF_BeginEvent = NULL;
static LPD3DPERF_ENDEVENT s_DynamicD3DPERF_EndEvent = NULL;
static LPD3DPERF_SETMARKER s_DynamicD3DPERF_SetMarker = NULL;
static LPD3DPERF_SETREGION s_DynamicD3DPERF_SetRegion = NULL;
static LPD3DPERF_QUERYREPEATFRAME s_DynamicD3DPERF_QueryRepeatFrame = NULL;
static LPD3DPERF_SETOPTIONS s_DynamicD3DPERF_SetOptions = NULL;
static LPD3DPERF_GETSTATUS s_DynamicD3DPERF_GetStatus = NULL;

// Ensure function pointers are initialized
static bool DXUT_EnsureD3D9APIs( void )
{
#if defined (DIRECT3D9)
    // If the module is non-NULL, this function has already been called.  Note
    // that this doesn't guarantee that all ProcAddresses were found.
    if( s_hModD3D9 != NULL )
        return true;

    WCHAR wszPath[ MAX_PATH+1 ] = {0};

    // This may fail if Direct3D 9 isn't installed
//    if( !::GetSystemDirectoryW( wszPath, MAX_PATH+1 ) )
//        return false;
//    StringCchCatW( wszPath, MAX_PATH, L"\\d3d9.dll" );
		StringCchCatW( wszPath, MAX_PATH, L"d3d9.dll" );

    s_hModD3D9 = LoadLibraryW( wszPath );
    if( s_hModD3D9 != NULL )
    {
        s_DynamicDirect3DCreate9 = (LPDIRECT3DCREATE9)GetProcAddress( s_hModD3D9, "Direct3DCreate9" );
        s_DynamicD3DPERF_BeginEvent = (LPD3DPERF_BEGINEVENT)GetProcAddress( s_hModD3D9, "D3DPERF_BeginEvent" );
        s_DynamicD3DPERF_EndEvent = (LPD3DPERF_ENDEVENT)GetProcAddress( s_hModD3D9, "D3DPERF_EndEvent" );
        s_DynamicD3DPERF_SetMarker = (LPD3DPERF_SETMARKER)GetProcAddress( s_hModD3D9, "D3DPERF_SetMarker" );
        s_DynamicD3DPERF_SetRegion = (LPD3DPERF_SETREGION)GetProcAddress( s_hModD3D9, "D3DPERF_SetRegion" );
        s_DynamicD3DPERF_QueryRepeatFrame = (LPD3DPERF_QUERYREPEATFRAME)GetProcAddress( s_hModD3D9, "D3DPERF_QueryRepeatFrame" );
        s_DynamicD3DPERF_SetOptions = (LPD3DPERF_SETOPTIONS)GetProcAddress( s_hModD3D9, "D3DPERF_SetOptions" );
        s_DynamicD3DPERF_GetStatus = (LPD3DPERF_GETSTATUS)GetProcAddress( s_hModD3D9, "D3DPERF_GetStatus" );
    }

    return s_hModD3D9 != NULL;
#elif defined(LINUX)
  s_DynamicDirect3DCreate9 = Direct3DCreate9;
  // TODO: Fill in the other function pointers.
  return true;
#else
  return true;
#endif
}

IDirect3D9 * WINAPI DXUT_Dynamic_Direct3DCreate9(UINT SDKVersion) 
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicDirect3DCreate9 != NULL )
        return s_DynamicDirect3DCreate9( SDKVersion );
    else
        return NULL;
}

int WINAPI DXUT_Dynamic_D3DPERF_BeginEvent( D3DCOLOR col, LPCWSTR wszName )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_BeginEvent != NULL )
        return s_DynamicD3DPERF_BeginEvent( col, wszName );
    else
        return -1;
}

int WINAPI DXUT_Dynamic_D3DPERF_EndEvent( void )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_EndEvent != NULL )
        return s_DynamicD3DPERF_EndEvent();
    else
        return -1;
}

void WINAPI DXUT_Dynamic_D3DPERF_SetMarker( D3DCOLOR col, LPCWSTR wszName )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_SetMarker != NULL )
        s_DynamicD3DPERF_SetMarker( col, wszName );
}

void WINAPI DXUT_Dynamic_D3DPERF_SetRegion( D3DCOLOR col, LPCWSTR wszName )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_SetRegion != NULL )
        s_DynamicD3DPERF_SetRegion( col, wszName );
}

BOOL WINAPI DXUT_Dynamic_D3DPERF_QueryRepeatFrame( void )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_QueryRepeatFrame != NULL )
        return s_DynamicD3DPERF_QueryRepeatFrame();
    else
        return FALSE;
}

void WINAPI DXUT_Dynamic_D3DPERF_SetOptions( DWORD dwOptions )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_SetOptions != NULL )
        s_DynamicD3DPERF_SetOptions( dwOptions );
}

DWORD WINAPI DXUT_Dynamic_D3DPERF_GetStatus( void )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_GetStatus != NULL )
        return s_DynamicD3DPERF_GetStatus();
    else
        return 0;
}

//--------------------------------------------------------------------------------------
// Trace a string description of a decl 
//--------------------------------------------------------------------------------------
void WINAPI DXUTTraceDecl( D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE] )
{
    int iDecl=0;
    for( iDecl=0; iDecl<MAX_FVF_DECL_SIZE; iDecl++ )
    {
        if( decl[iDecl].Stream == 0xFF )
            break;

        DXUTOutputDebugStringW( L"decl[%d]=Stream:%d, Offset:%d, %s, %s, %s, UsageIndex:%d\n", iDecl, 
                    decl[iDecl].Stream,
                    decl[iDecl].Offset,
                    DXUTTraceD3DDECLTYPEtoString( decl[iDecl].Type ),
                    DXUTTraceD3DDECLMETHODtoString( decl[iDecl].Method ),
                    DXUTTraceD3DDECLUSAGEtoString( decl[iDecl].Usage ),
                    decl[iDecl].UsageIndex );
    }

    DXUTOutputDebugStringW( L"decl[%d]=D3DDECL_END\n", iDecl );
}


//--------------------------------------------------------------------------------------
WCHAR* WINAPI DXUTTraceD3DDECLTYPEtoString( BYTE t )
{
#if !defined(PS3)
    switch( t )
    {
        case D3DDECLTYPE_FLOAT1: return L"D3DDECLTYPE_FLOAT1";
        case D3DDECLTYPE_FLOAT2: return L"D3DDECLTYPE_FLOAT2";
        case D3DDECLTYPE_FLOAT3: return L"D3DDECLTYPE_FLOAT3";
        case D3DDECLTYPE_FLOAT4: return L"D3DDECLTYPE_FLOAT4";
        case D3DDECLTYPE_D3DCOLOR: return L"D3DDECLTYPE_D3DCOLOR";
        case D3DDECLTYPE_UBYTE4: return L"D3DDECLTYPE_UBYTE4";
        case D3DDECLTYPE_SHORT2: return L"D3DDECLTYPE_SHORT2";
        case D3DDECLTYPE_SHORT4: return L"D3DDECLTYPE_SHORT4";
        case D3DDECLTYPE_UBYTE4N: return L"D3DDECLTYPE_UBYTE4N";
        case D3DDECLTYPE_SHORT2N: return L"D3DDECLTYPE_SHORT2N";
        case D3DDECLTYPE_SHORT4N: return L"D3DDECLTYPE_SHORT4N";
        case D3DDECLTYPE_USHORT2N: return L"D3DDECLTYPE_USHORT2N";
        case D3DDECLTYPE_USHORT4N: return L"D3DDECLTYPE_USHORT4N";
        case D3DDECLTYPE_UDEC3: return L"D3DDECLTYPE_UDEC3";
        case D3DDECLTYPE_DEC3N: return L"D3DDECLTYPE_DEC3N";
        case D3DDECLTYPE_FLOAT16_2: return L"D3DDECLTYPE_FLOAT16_2";
        case D3DDECLTYPE_FLOAT16_4: return L"D3DDECLTYPE_FLOAT16_4";
        case D3DDECLTYPE_UNUSED: return L"D3DDECLTYPE_UNUSED";
    }
#endif
	return L"D3DDECLTYPE Unknown";
}

WCHAR* WINAPI DXUTTraceD3DDECLMETHODtoString( BYTE m )
{
#if !defined(PS3)
		switch( m )
    {
        case D3DDECLMETHOD_DEFAULT: return L"D3DDECLMETHOD_DEFAULT";
        case D3DDECLMETHOD_PARTIALU: return L"D3DDECLMETHOD_PARTIALU";
        case D3DDECLMETHOD_PARTIALV: return L"D3DDECLMETHOD_PARTIALV";
        case D3DDECLMETHOD_CROSSUV: return L"D3DDECLMETHOD_CROSSUV";
        case D3DDECLMETHOD_UV: return L"D3DDECLMETHOD_UV";
        case D3DDECLMETHOD_LOOKUP: return L"D3DDECLMETHOD_LOOKUP";
        case D3DDECLMETHOD_LOOKUPPRESAMPLED: return L"D3DDECLMETHOD_LOOKUPPRESAMPLED";
    }
#endif
	return L"D3DDECLMETHOD Unknown";
}

WCHAR* WINAPI DXUTTraceD3DDECLUSAGEtoString( BYTE u )
{
#if !defined(PS3)
    switch( u )
    {
        case D3DDECLUSAGE_POSITION: return L"D3DDECLUSAGE_POSITION";
        case D3DDECLUSAGE_BLENDWEIGHT: return L"D3DDECLUSAGE_BLENDWEIGHT";
        case D3DDECLUSAGE_BLENDINDICES: return L"D3DDECLUSAGE_BLENDINDICES";
        case D3DDECLUSAGE_NORMAL: return L"D3DDECLUSAGE_NORMAL";
        case D3DDECLUSAGE_PSIZE: return L"D3DDECLUSAGE_PSIZE";
        case D3DDECLUSAGE_TEXCOORD: return L"D3DDECLUSAGE_TEXCOORD";
        case D3DDECLUSAGE_TANGENT: return L"D3DDECLUSAGE_TANGENT";
        case D3DDECLUSAGE_BINORMAL: return L"D3DDECLUSAGE_BINORMAL";
        case D3DDECLUSAGE_TESSFACTOR: return L"D3DDECLUSAGE_TESSFACTOR";
        case D3DDECLUSAGE_POSITIONT: return L"D3DDECLUSAGE_POSITIONT";
        case D3DDECLUSAGE_COLOR: return L"D3DDECLUSAGE_COLOR";
        case D3DDECLUSAGE_FOG: return L"D3DDECLUSAGE_FOG";
        case D3DDECLUSAGE_DEPTH: return L"D3DDECLUSAGE_DEPTH";
        case D3DDECLUSAGE_SAMPLE: return L"D3DDECLUSAGE_SAMPLE";
    }
#endif
	return L"D3DDECLUSAGE Unknown";
}

#if !defined(PS3) && !defined(LINUX)
//--------------------------------------------------------------------------------------
// Multimon API handling for OSes with or without multimon API support
//--------------------------------------------------------------------------------------
#define DXUT_PRIMARY_MONITOR ((HMONITOR)0x12340042)
typedef HMONITOR (WINAPI* LPMONITORFROMWINDOW)(HWND, DWORD);
typedef BOOL     (WINAPI* LPGETMONITORINFO)(HMONITOR, LPMONITORINFO);
typedef HMONITOR (WINAPI* LPMONITORFROMRECT)(LPCRECT lprcScreenCoords, DWORD dwFlags);

BOOL WINAPI DXUTGetMonitorInfo(HMONITOR hMonitor, LPMONITORINFO lpMonitorInfo)
{
    static bool s_bInited = false;
    static LPGETMONITORINFO s_pFnGetMonitorInfo = NULL;
    if( !s_bInited )        
    {
        s_bInited = true;
        HMODULE hUser32 = GetModuleHandleW( L"USER32" );
        if (hUser32 ) 
        {
            OSVERSIONINFOA osvi = {0}; osvi.dwOSVersionInfoSize = sizeof(osvi); GetVersionExA((OSVERSIONINFOA*)&osvi);
            bool bNT = (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId);    
            s_pFnGetMonitorInfo = (LPGETMONITORINFO) (bNT ? GetProcAddress(hUser32,"GetMonitorInfoW") : GetProcAddress(hUser32,"GetMonitorInfoA"));
        }
    }

    if( s_pFnGetMonitorInfo ) 
        return s_pFnGetMonitorInfo(hMonitor, lpMonitorInfo);

    RECT rcWork;
    if ((hMonitor == DXUT_PRIMARY_MONITOR) && lpMonitorInfo && (lpMonitorInfo->cbSize >= sizeof(MONITORINFO)) && SystemParametersInfoA(SPI_GETWORKAREA, 0, &rcWork, 0))
    {
        lpMonitorInfo->rcMonitor.left = 0;
        lpMonitorInfo->rcMonitor.top  = 0;
        lpMonitorInfo->rcMonitor.right  = GetSystemMetrics(SM_CXSCREEN);
        lpMonitorInfo->rcMonitor.bottom = GetSystemMetrics(SM_CYSCREEN);
        lpMonitorInfo->rcWork = rcWork;
        lpMonitorInfo->dwFlags = MONITORINFOF_PRIMARY;
        return TRUE;
    }
    return FALSE;
}


HMONITOR WINAPI DXUTMonitorFromWindow(HWND hWnd, DWORD dwFlags)
{
    static bool s_bInited = false;
    static LPMONITORFROMWINDOW s_pFnGetMonitorFromWindow = NULL;
    if( !s_bInited )        
    {
        s_bInited = true;
        HMODULE hUser32 = GetModuleHandleW( L"USER32" );
        if (hUser32 ) s_pFnGetMonitorFromWindow = (LPMONITORFROMWINDOW) GetProcAddress(hUser32,"MonitorFromWindow");
    }

    if( s_pFnGetMonitorFromWindow ) 
        return s_pFnGetMonitorFromWindow(hWnd, dwFlags);
    else
        return DXUT_PRIMARY_MONITOR;
}


HMONITOR WINAPI DXUTMonitorFromRect(LPCRECT lprcScreenCoords, DWORD dwFlags)
{
    static bool s_bInited = false;
    static LPMONITORFROMRECT s_pFnGetMonitorFromRect = NULL;
    if( !s_bInited )        
    {
        s_bInited = true;
        HMODULE hUser32 = GetModuleHandleW( L"USER32" );
        if (hUser32 ) s_pFnGetMonitorFromRect = (LPMONITORFROMRECT) GetProcAddress(hUser32,"MonitorFromRect");
    }

    if( s_pFnGetMonitorFromRect ) 
        return s_pFnGetMonitorFromRect(lprcScreenCoords, dwFlags);
    else
        return DXUT_PRIMARY_MONITOR;
}
#endif

//--------------------------------------------------------------------------------------
// Get the desktop resolution of an adapter. This isn't the same as the current resolution 
// from GetAdapterDisplayMode since the device might be fullscreen 
//--------------------------------------------------------------------------------------
void WINAPI DXUTGetDesktopResolution( UINT AdapterOrdinal, UINT* pWidth, UINT* pHeight )
{
    DXUTDeviceSettings DeviceSettings = DXUTGetDeviceSettings();

    WCHAR strDeviceName[256] = {0};
#if defined (DIRECT3D9)
    DEVMODEW devMode;
    ZeroMemory( &devMode, sizeof(DEVMODE) );
    devMode.dmSize = sizeof(DEVMODE);
    if( DeviceSettings.ver == DXUT_D3D9_DEVICE )
    {
        CD3D9Enumeration* pd3dEnum = DXUTGetD3D9Enumeration();
        CD3D9EnumAdapterInfo* pAdapterInfo = pd3dEnum->GetAdapterInfo( AdapterOrdinal );
        if(pAdapterInfo)
        {
            MultiByteToWideChar( CP_ACP, 0, pAdapterInfo->AdapterIdentifier.DeviceName, -1, strDeviceName, 256 );
            strDeviceName[255] = 0;
        }
    }
    else
#endif

#if defined (DIRECT3D9)
    EnumDisplaySettingsW( strDeviceName, ENUM_REGISTRY_SETTINGS, &devMode );
    if( pWidth )
        *pWidth = devMode.dmPelsWidth;
    if( pHeight )
        *pHeight = devMode.dmPelsHeight;
#endif
}


#if defined (DIRECT3D9) || defined(OPENGL)
//--------------------------------------------------------------------------------------
// Display error msg box to help debug 
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTTrace( const CHAR* strFile, DWORD dwLine, HRESULT hr,
                          const WCHAR* strMsg, bool bPopMsgBox )
{
    //bool bShowMsgBoxOnError = DXUTGetShowMsgBoxOnError();
    //if( bPopMsgBox && bShowMsgBoxOnError == false )
    //    bPopMsgBox = false;

    return  hr; // DXTraceW( strFile, dwLine, hr, strMsg, bPopMsgBox );
}
#endif

//--------------------------------------------------------------------------------------
IDirect3DDevice9* WINAPI DXUTCreateRefDevice9( HWND hWnd, bool bNullRef )
{
    HRESULT hr;
    IDirect3D9* pD3D = DXUT_Dynamic_Direct3DCreate9( D3D_SDK_VERSION );
    if( NULL == pD3D )
        return NULL;

    D3DDISPLAYMODE Mode;
    pD3D->GetAdapterDisplayMode(0, &Mode);

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory( &pp, sizeof(D3DPRESENT_PARAMETERS) );
    pp.BackBufferWidth  = 1;
    pp.BackBufferHeight = 1;
    pp.BackBufferFormat = Mode.Format;
    pp.BackBufferCount  = 1;
    pp.SwapEffect       = D3DSWAPEFFECT_COPY;
    pp.Windowed         = TRUE;
    pp.hDeviceWindow    = hWnd;

    IDirect3DDevice9* pd3dDevice = NULL;
    hr = pD3D->CreateDevice( D3DADAPTER_DEFAULT, bNullRef ? D3DDEVTYPE_NULLREF : D3DDEVTYPE_REF,
                             hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &pd3dDevice );

    SAFE_RELEASE( pD3D );
    return pd3dDevice;
}

#endif
