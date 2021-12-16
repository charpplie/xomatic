#ifndef __CRYTYPES__
#define __CRYTYPES__

#define eCryModule eCryM_Render
#include <CryModuleDefs.h>
#include <platform.h>

#define E_OUTOFMEMORY 0x8007000E
#define E_FAIL 0x80004005
#define E_ABORT 0x4004

#define INVALID_Z_COUNT ((uint32)~0)
#define PENDING_Z_COUNT (1 << 31)
#define TOO_OLD_Z_COUNT	((~1) & (~PENDING_Z_COUNT))
#define IsZCountInvalid(ZCOUNT) (((ZCOUNT) & PENDING_Z_COUNT) != 0)
#define IsZCountValid(ZCOUNT)		(((ZCOUNT) & PENDING_Z_COUNT) == 0)

#define FACILITY_AAF               18
#define FACILITY_ACS               20
#define FACILITY_BACKGROUNDCOPY    32
#define FACILITY_CERT              11
#define FACILITY_COMPLUS           17
#define FACILITY_CONFIGURATION     33
#define FACILITY_CONTROL           10
#define FACILITY_DISPATCH           2
#define FACILITY_DPLAY             21
#define FACILITY_HTTP              25
#define FACILITY_INTERNET          12
#define FACILITY_ITF                4
#define FACILITY_MEDIASERVER       13
#define FACILITY_MSMQ              14
#define FACILITY_NULL               0
#define FACILITY_RPC                1
#define FACILITY_SCARD             16
#define FACILITY_SECURITY           9
#define FACILITY_SETUPAPI          15
#define FACILITY_SSPI               9
#define FACILITY_STORAGE            3
#define FACILITY_SXS               23
#define FACILITY_UMI               22
#define FACILITY_URT               19
#define FACILITY_WIN32              7
#define FACILITY_WINDOWS            8
#define FACILITY_WINDOWS_CE        24

#define SEVERITY_ERROR 1

#define MAKE_HRESULT(sev,fac,code) ( (HRESULT) (((uint64)(sev)<<31) | ((uint64)(fac)<<16) | ((uint64)(code))) )

#define _FACDXGI    0x87a
#define MAKE_DXGI_HRESULT( code )    MAKE_HRESULT( 1, _FACDXGI, code )
#define MAKE_DXGI_STATUS( code )    MAKE_HRESULT( 0, _FACDXGI, code )
#define DXGI_STATUS_OCCLUDED                MAKE_DXGI_STATUS(1 )
#define DXGI_STATUS_CLIPPED                 MAKE_DXGI_STATUS(2 )
#define DXGI_STATUS_NO_REDIRECTION          MAKE_DXGI_STATUS(4 )
#define DXGI_STATUS_NO_DESKTOP_ACCESS       MAKE_DXGI_STATUS(5 )
#define DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE       MAKE_DXGI_STATUS(6 )
#define DXGI_STATUS_MODE_CHANGED            MAKE_DXGI_STATUS(7 )
#define DXGI_ERROR_INVALID_CALL    MAKE_DXGI_HRESULT( 1 )
#define DXGI_ERROR_NOT_FOUND    MAKE_DXGI_HRESULT( 2 )

#define StringCchCopyW(pszDest,cchDest,pszSrc) wcscpy(pszDest,pszSrc)
#define StringCchCatW(pszDest,cchDest,pszSrc) wcscat(pszDest,pszSrc)
#define StringCchPrintfW swprintf
#define lstrlenW wcslen

#endif

