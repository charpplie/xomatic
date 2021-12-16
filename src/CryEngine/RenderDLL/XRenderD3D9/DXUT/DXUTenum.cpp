//--------------------------------------------------------------------------------------
// File: DXUTEnum.cpp
//
// Enumerates D3D adapters, devices, modes, etc.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "StdAfx.h"
#include "../DriverD3D.h"
#include MATH_H
#if !defined(XENON)

#include "DXUT.h"
#undef min // use __min instead
#undef max // use __max instead

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
#if defined (DIRECT3D9) || defined (OPENGL)
extern void DXUTGetCallbackD3D9DeviceAcceptable( LPDXUTCALLBACKISD3D9DEVICEACCEPTABLE* ppCallbackIsDeviceAcceptable, void** ppUserContext );
#endif

#if defined (DIRECT3D9) || defined (OPENGL)
HRESULT DXUTFindValidD3D9DeviceSettings( DXUTD3D9DeviceSettings* pOut, DXUTD3D9DeviceSettings* pIn, DXUTMatchOptions* pMatchOptions, DXUTD3D9DeviceSettings* pOptimal );
void    DXUTBuildOptimalD3D9DeviceSettings( DXUTD3D9DeviceSettings* pOptimalDeviceSettings, DXUTD3D9DeviceSettings* pDeviceSettingsIn, DXUTMatchOptions* pMatchOptions );
bool    DXUTDoesD3D9DeviceComboMatchPreserveOptions( CD3D9EnumDeviceSettingsCombo* pDeviceSettingsCombo, DXUTD3D9DeviceSettings* pDeviceSettingsIn, DXUTMatchOptions* pMatchOptions );
float   DXUTRankD3D9DeviceCombo( CD3D9EnumDeviceSettingsCombo* pDeviceSettingsCombo, DXUTD3D9DeviceSettings* pOptimalDeviceSettings, D3DDISPLAYMODE* pAdapterDesktopDisplayMode );
void    DXUTBuildValidD3D9DeviceSettings( DXUTD3D9DeviceSettings* pValidDeviceSettings, CD3D9EnumDeviceSettingsCombo* pBestDeviceSettingsCombo, DXUTD3D9DeviceSettings* pDeviceSettingsIn, DXUTMatchOptions* pMatchOptions );
HRESULT DXUTFindValidD3D9Resolution( CD3D9EnumDeviceSettingsCombo* pBestDeviceSettingsCombo, D3DDISPLAYMODE displayModeIn, D3DDISPLAYMODE* pBestDisplayMode );
#endif

static int __cdecl SortModesCallback( const void* arg1, const void* arg2 );


//======================================================================================
//======================================================================================
// General Direct3D section
//======================================================================================
//======================================================================================


//--------------------------------------------------------------------------------------
// This function tries to find valid device settings based upon the input device settings 
// struct and the match options.  For each device setting a match option in the 
// DXUTMatchOptions struct specifies how the function makes decisions.  For example, if 
// the caller wants a HAL device with a back buffer format of D3DFMT_A2B10G10R10 but the 
// HAL device on the system does not support D3DFMT_A2B10G10R10 however a REF device is 
// installed that does, then the function has a choice to either use REF or to change to 
// a back buffer format to compatible with the HAL device.  The match options lets the 
// caller control how these choices are made.
//
// Each match option must be one of the following types: 
//      DXUTMT_IGNORE_INPUT: Uses the closest valid value to a default 
//      DXUTMT_PRESERVE_INPUT: Uses the input without change, but may cause no valid device to be found
//      DXUTMT_CLOSEST_TO_INPUT: Uses the closest valid value to the input 
//
// If pMatchOptions is NULL then, all of the match options are assumed to be DXUTMT_IGNORE_INPUT.  
// The function returns failure if no valid device settings can be found otherwise 
// the function returns success and the valid device settings are written to pOut.
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTFindValidDeviceSettings( DXUTDeviceSettings* pOut, DXUTDeviceSettings* pIn, DXUTMatchOptions* pMatchOptions )
{
    HRESULT hr = S_OK;

    if( pOut == NULL )
        return DXUT_ERR_MSGBOX( L"DXUTFindValidDeviceSettings", E_INVALIDARG );

    // Default to DXUTMT_IGNORE_INPUT for everything unless pMatchOptions isn't NULL
    DXUTMatchOptions defaultMatchOptions;
    if( NULL == pMatchOptions )
    {
        ZeroMemory( &defaultMatchOptions, sizeof(DXUTMatchOptions) );
        pMatchOptions = &defaultMatchOptions;
    }

    bool bAppSupportsD3D9 = DXUTDoesAppSupportD3D9();
    
    if( !bAppSupportsD3D9 )
    {
        bAppSupportsD3D9 = true;
    }
        
    bool bPreferD3D9 = false;
    if( pMatchOptions->eAPIVersion != DXUTMT_IGNORE_INPUT && pIn && pIn->ver == DXUT_D3D9_DEVICE )
        bPreferD3D9 = true;

    // Build an optimal device settings structure based upon the match 
    // options.  If the match option is set to ignore, then a optimal default value is used.
    // The default value may not exist on the system, but later this will be taken 
    // into account.
    bool bFoundValidD3D9 = false;

    DXUTDeviceSettings validDeviceSettings;
    ZeroMemory( &validDeviceSettings, sizeof(DXUTDeviceSettings) );
    DXUTDeviceSettings optimalDeviceSettings;

    if( !bPreferD3D9 )
    {
        hr = DXUTERR_NODIRECT3D;
    }

#if defined (DIRECT3D9) || defined (OPENGL)
    if( bAppSupportsD3D9 )
    {
        // Force an enumeration with the IsDeviceAcceptable callback
        DXUTGetD3D9Enumeration( false );

        DXUTD3D9DeviceSettings d3d9In;
        ZeroMemory( &d3d9In, sizeof(DXUTD3D9DeviceSettings) );
        if( pIn )
        {
            d3d9In = pIn->d3d9;
        }

        optimalDeviceSettings.ver = DXUT_D3D9_DEVICE;
        DXUTBuildOptimalD3D9DeviceSettings( &optimalDeviceSettings.d3d9, &d3d9In, pMatchOptions );

        validDeviceSettings.ver = DXUT_D3D9_DEVICE;
        hr = DXUTFindValidD3D9DeviceSettings( &validDeviceSettings.d3d9, &d3d9In, pMatchOptions, &optimalDeviceSettings.d3d9 );
        if( SUCCEEDED(hr) )
            bFoundValidD3D9 = true;
    }
#endif

    if( bFoundValidD3D9 )
    {
        *pOut = validDeviceSettings;
        return S_OK;
    }
    else
    {
        return DXUT_ERR( L"DXUTFindValidDeviceSettings", hr );
    }
}


//======================================================================================
//======================================================================================
// Direct3D 9 section
//======================================================================================
//======================================================================================

#if defined (DIRECT3D9) || defined (OPENGL)

//--------------------------------------------------------------------------------------
CD3D9Enumeration* WINAPI DXUTGetD3D9Enumeration( bool bForceEnumerate )
{
    // Using an accessor function gives control of the construction order
    static CD3D9Enumeration d3denum;

    if( !d3denum.HasEnumerated() || bForceEnumerate )
    {
        LPDXUTCALLBACKISD3D9DEVICEACCEPTABLE pCallbackIsDeviceAcceptable;
        void* pUserContext;
        DXUTGetCallbackD3D9DeviceAcceptable( &pCallbackIsDeviceAcceptable, &pUserContext );
        d3denum.Enumerate( pCallbackIsDeviceAcceptable, pUserContext );
    }
    
    return &d3denum;
}


//--------------------------------------------------------------------------------------
CD3D9Enumeration::CD3D9Enumeration()
{
    m_bHasEnumerated = false;
    m_pD3D = NULL;
    m_IsD3D9DeviceAcceptableFunc = NULL;
    m_pIsD3D9DeviceAcceptableFuncUserContext = NULL;
    m_bRequirePostPixelShaderBlending = true;

    m_nMinWidth = 640;
    m_nMinHeight = 480;
    m_nMaxWidth = UINT_MAX;
    m_nMaxHeight = UINT_MAX;

    m_nRefreshMin = 0;
    m_nRefreshMax = UINT_MAX;

    m_nMultisampleQualityMax = 0xFFFF;

    ResetPossibleDepthStencilFormats();
    ResetPossibleMultisampleTypeList();                                   
    ResetPossiblePresentIntervalList();
    SetPossibleVertexProcessingList( true, true, true, false );
}


//--------------------------------------------------------------------------------------
CD3D9Enumeration::~CD3D9Enumeration()
{
    ClearAdapterInfoList();
}



//--------------------------------------------------------------------------------------
// Enumerate for each adapter all of the supported display modes, 
// device types, adapter formats, back buffer formats, window/full screen support, 
// depth stencil formats, multisampling types/qualities, and presentations intervals.
//
// For each combination of device type (HAL/REF), adapter format, back buffer format, and
// IsWindowed it will call the app's ConfirmDevice callback.  This allows the app
// to reject or allow that combination based on its caps/etc.  It also allows the 
// app to change the BehaviorFlags.  The BehaviorFlags defaults non-pure HWVP 
// if supported otherwise it will default to SWVP, however the app can change this 
// through the ConfirmDevice callback.
//--------------------------------------------------------------------------------------
HRESULT CD3D9Enumeration::Enumerate( LPDXUTCALLBACKISD3D9DEVICEACCEPTABLE IsD3D9DeviceAcceptableFunc,
                                     void* pIsD3D9DeviceAcceptableFuncUserContext )
{
    CDXUTPerfEventGenerator eventGenerator( DXUT_PERFEVENTCOLOR, L"DXUT D3D9 Enumeration" );
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( pD3D == NULL )
    {
        pD3D = DXUTGetD3D9Object();
        if( pD3D == NULL )
            return DXUTERR_NODIRECT3D;
    }

    m_bHasEnumerated = true; 
    m_pD3D = pD3D;
    m_IsD3D9DeviceAcceptableFunc = IsD3D9DeviceAcceptableFunc;
    m_pIsD3D9DeviceAcceptableFuncUserContext = pIsD3D9DeviceAcceptableFuncUserContext;

    HRESULT hr;
    ClearAdapterInfoList();
    CGrowableArray<D3DFORMAT> adapterFormatList;

    const D3DFORMAT allowedAdapterFormatArray[] = 
    {   
        D3DFMT_X8R8G8B8, 
        D3DFMT_X1R5G5B5, 
        D3DFMT_R5G6B5, 
        D3DFMT_A2R10G10B10
    };
    const UINT allowedAdapterFormatArrayCount  = sizeof(allowedAdapterFormatArray) / sizeof(allowedAdapterFormatArray[0]);

    UINT numAdapters = pD3D->GetAdapterCount();
    for (UINT adapterOrdinal = 0; adapterOrdinal < numAdapters; adapterOrdinal++)
    {
        CD3D9EnumAdapterInfo* pAdapterInfo = new CD3D9EnumAdapterInfo;
        if( pAdapterInfo == NULL )
            return E_OUTOFMEMORY;

        pAdapterInfo->AdapterOrdinal = adapterOrdinal;
        pD3D->GetAdapterIdentifier(adapterOrdinal, 0, &pAdapterInfo->AdapterIdentifier);

        // Get list of all display modes on this adapter.  
        // Also build a temporary list of all display adapter formats.
        adapterFormatList.RemoveAll();

        for( UINT iFormatList = 0; iFormatList < allowedAdapterFormatArrayCount; iFormatList++ )
        {
            D3DFORMAT allowedAdapterFormat = allowedAdapterFormatArray[iFormatList];
            UINT numAdapterModes = pD3D->GetAdapterModeCount( adapterOrdinal, allowedAdapterFormat );
            for (UINT mode = 0; mode < numAdapterModes; mode++)
            {
                D3DDISPLAYMODE displayMode;
                pD3D->EnumAdapterModes( adapterOrdinal, allowedAdapterFormat, mode, &displayMode );

                if( displayMode.Width < m_nMinWidth ||
                    displayMode.Height < m_nMinHeight || 
                    displayMode.Width > m_nMaxWidth ||
                    displayMode.Height > m_nMaxHeight || 
                    displayMode.RefreshRate < m_nRefreshMin ||
                    displayMode.RefreshRate > m_nRefreshMax )
                {
                    continue;
                }

                pAdapterInfo->displayModeList.Add( displayMode );

                if( !adapterFormatList.Contains(displayMode.Format) )
                    adapterFormatList.Add( displayMode.Format );
            }

        }

        D3DDISPLAYMODE displayMode;
        pD3D->GetAdapterDisplayMode( adapterOrdinal, &displayMode );
        if( !adapterFormatList.Contains(displayMode.Format) )
            adapterFormatList.Add( displayMode.Format );

        // Sort displaymode list
        qsort( pAdapterInfo->displayModeList.GetData(), 
               pAdapterInfo->displayModeList.GetSize(), sizeof( D3DDISPLAYMODE ),
               SortModesCallback );

        // Get info for each device on this adapter
        if( FAILED( EnumerateDevices( pAdapterInfo, &adapterFormatList ) ) )
        {
            delete pAdapterInfo;
            continue;
        }

        // If at least one device on this adapter is available and compatible
        // with the app, add the adapterInfo to the list
        if( pAdapterInfo->deviceInfoList.GetSize() > 0 )
        {
            hr = m_AdapterInfoList.Add( pAdapterInfo );
            if( FAILED(hr) )
                return hr;
        } else
            delete pAdapterInfo;
    }

    //
    // Check for 2 or more adapters with the same name. Append the name
    // with some instance number if that's the case to help distinguish
    // them.
    //
    bool bUniqueDesc = true;
    CD3D9EnumAdapterInfo* pAdapterInfo;
    for( int i=0; i<m_AdapterInfoList.GetSize(); i++ )
    {
        CD3D9EnumAdapterInfo* pAdapterInfo1 = m_AdapterInfoList.GetAt(i);

        for( int j=i+1; j<m_AdapterInfoList.GetSize(); j++ )
        {
            CD3D9EnumAdapterInfo* pAdapterInfo2 = m_AdapterInfoList.GetAt(j);
            if( _stricmp( pAdapterInfo1->AdapterIdentifier.Description, 
                          pAdapterInfo2->AdapterIdentifier.Description ) == 0 )
            {
                bUniqueDesc = false;
                break;
            }
        }

        if( !bUniqueDesc )
            break;
    }

    for( int i=0; i<m_AdapterInfoList.GetSize(); i++ )
    {
        pAdapterInfo = m_AdapterInfoList.GetAt(i);

#if defined (DIRECT3D9)
        MultiByteToWideChar( CP_ACP, 0, 
                             pAdapterInfo->AdapterIdentifier.Description, -1, 
                             pAdapterInfo->szUniqueDescription, 100 );
        pAdapterInfo->szUniqueDescription[100] = 0;

        if( !bUniqueDesc )
        {
            WCHAR sz[100];
            StringCchPrintfW( sz, 100, L" (#%d)", pAdapterInfo->AdapterOrdinal );
            StringCchCatW( pAdapterInfo->szUniqueDescription, 256, sz );

        }
#endif
    }

    return S_OK;
}



//--------------------------------------------------------------------------------------
// Enumerates D3D devices for a particular adapter.
//--------------------------------------------------------------------------------------
HRESULT CD3D9Enumeration::EnumerateDevices( CD3D9EnumAdapterInfo* pAdapterInfo, CGrowableArray<D3DFORMAT>* pAdapterFormatList )
{
    HRESULT hr;

    const D3DDEVTYPE devTypeArray[] = 
    { 
        D3DDEVTYPE_HAL, 
        D3DDEVTYPE_SW, 
        D3DDEVTYPE_REF, 
        D3DDEVTYPE_NULLREF 
    };
    const UINT devTypeArrayCount = sizeof(devTypeArray) / sizeof(devTypeArray[0]);

    // Enumerate each Direct3D device type
    for( UINT iDeviceType = 0; iDeviceType < devTypeArrayCount; iDeviceType++ )
    {
        CD3D9EnumDeviceInfo* pDeviceInfo = new CD3D9EnumDeviceInfo;
        if( pDeviceInfo == NULL )
            return E_OUTOFMEMORY;

        // Fill struct w/ AdapterOrdinal and D3DDEVTYPE
        pDeviceInfo->AdapterOrdinal = pAdapterInfo->AdapterOrdinal;
        pDeviceInfo->DeviceType = devTypeArray[iDeviceType];

        // Store device caps
        if (pDeviceInfo->DeviceType == D3DDEVTYPE_NULLREF)
        {
          delete pDeviceInfo;
          continue;
        }
        if( FAILED( hr = m_pD3D->GetDeviceCaps( pAdapterInfo->AdapterOrdinal, pDeviceInfo->DeviceType, 
                                                &pDeviceInfo->Caps ) ) )
        {
            delete pDeviceInfo;
            continue;
        }

        if( pDeviceInfo->DeviceType != D3DDEVTYPE_HAL )
        {
            // Create a dummy device to verify that we can really create this device type.
            D3DDISPLAYMODE Mode;
            m_pD3D->GetAdapterDisplayMode(0, &Mode);
            D3DPRESENT_PARAMETERS pp;
            ZeroMemory( &pp, sizeof(D3DPRESENT_PARAMETERS) );
            pp.BackBufferWidth  = 1;
            pp.BackBufferHeight = 1;
            pp.BackBufferFormat = Mode.Format;
            pp.BackBufferCount  = 1;
            pp.SwapEffect       = D3DSWAPEFFECT_COPY;
            pp.Windowed         = TRUE;
            pp.hDeviceWindow    = DXUTGetHWNDFocus();
            IDirect3DDevice9 *pDevice = NULL;
            if( FAILED( hr = m_pD3D->CreateDevice( pAdapterInfo->AdapterOrdinal, pDeviceInfo->DeviceType, DXUTGetHWNDFocus(),
                                                   D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &pDevice ) ) )
            {
                delete pDeviceInfo;
                continue;
            }
            SAFE_RELEASE( pDevice );
        }

        // Get info for each devicecombo on this device
        if( FAILED( hr = EnumerateDeviceCombos( pAdapterInfo, pDeviceInfo, pAdapterFormatList ) ) )
        {
            delete pDeviceInfo;
            continue;
        }

        // If at least one devicecombo for this device is found, 
        // add the deviceInfo to the list
        if (pDeviceInfo->deviceSettingsComboList.GetSize() > 0 )
            pAdapterInfo->deviceInfoList.Add( pDeviceInfo );
        else
            delete pDeviceInfo;
    }

    return S_OK;
}



//--------------------------------------------------------------------------------------
// Enumerates DeviceCombos for a particular device.
//--------------------------------------------------------------------------------------
HRESULT CD3D9Enumeration::EnumerateDeviceCombos( CD3D9EnumAdapterInfo* pAdapterInfo, CD3D9EnumDeviceInfo* pDeviceInfo, CGrowableArray<D3DFORMAT>* pAdapterFormatList )
{
    const D3DFORMAT backBufferFormatArray[] = 
    {   
        D3DFMT_A8R8G8B8, 
        D3DFMT_X8R8G8B8, 
        D3DFMT_A2R10G10B10, 
        D3DFMT_R5G6B5, 
        D3DFMT_A1R5G5B5, 
        D3DFMT_X1R5G5B5 
    };
    const UINT backBufferFormatArrayCount = sizeof(backBufferFormatArray) / sizeof(backBufferFormatArray[0]);

    // See which adapter formats are supported by this device
    for( int iFormat=0; iFormat<pAdapterFormatList->GetSize(); iFormat++ )
    {
        D3DFORMAT adapterFormat = pAdapterFormatList->GetAt(iFormat);

        for( UINT iBackBufferFormat = 0; iBackBufferFormat < backBufferFormatArrayCount; iBackBufferFormat++ )
        {
            D3DFORMAT backBufferFormat = backBufferFormatArray[iBackBufferFormat];

            for( int nWindowed = 0; nWindowed < 2; nWindowed++)
            {
                if( !nWindowed && pAdapterInfo->displayModeList.GetSize() == 0 )
                    continue;

                if (FAILED( m_pD3D->CheckDeviceType( pAdapterInfo->AdapterOrdinal, pDeviceInfo->DeviceType, 
                                                     adapterFormat, backBufferFormat, nWindowed )))
                {
                    continue;
                }

                if( m_bRequirePostPixelShaderBlending )
                {
                    // If the backbuffer format doesn't support D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING
                    // then alpha test, pixel fog, render-target blending, color write enable, and dithering. 
                    // are not supported.
                    if( FAILED( m_pD3D->CheckDeviceFormat( pAdapterInfo->AdapterOrdinal, pDeviceInfo->DeviceType,
                                                        adapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
                                                        D3DRTYPE_TEXTURE, backBufferFormat ) ) )
                    {
                        continue;
                    }
                }

                // If an application callback function has been provided, make sure this device
                // is acceptable to the app.
                if( m_IsD3D9DeviceAcceptableFunc != NULL )
                {
                    if( !m_IsD3D9DeviceAcceptableFunc( &pDeviceInfo->Caps, adapterFormat, backBufferFormat, FALSE != nWindowed, m_pIsD3D9DeviceAcceptableFuncUserContext ) )
                        continue;
                }
                
                // At this point, we have an adapter/device/adapterformat/backbufferformat/iswindowed
                // DeviceCombo that is supported by the system and acceptable to the app. We still 
                // need to find one or more suitable depth/stencil buffer format,
                // multisample type, and present interval.
                CD3D9EnumDeviceSettingsCombo* pDeviceCombo = new CD3D9EnumDeviceSettingsCombo;
                if( pDeviceCombo == NULL )
                    return E_OUTOFMEMORY;

                pDeviceCombo->AdapterOrdinal = pAdapterInfo->AdapterOrdinal;
                pDeviceCombo->DeviceType = pDeviceInfo->DeviceType;
                pDeviceCombo->AdapterFormat = adapterFormat;
                pDeviceCombo->BackBufferFormat = backBufferFormat;
                pDeviceCombo->Windowed = (nWindowed != 0);
               
                BuildDepthStencilFormatList( pDeviceCombo );
                BuildMultiSampleTypeList( pDeviceCombo );
                if (pDeviceCombo->multiSampleTypeList.GetSize() == 0)
                {
                    delete pDeviceCombo;
                    continue;
                }
                BuildDSMSConflictList( pDeviceCombo );
                BuildPresentIntervalList(pDeviceInfo, pDeviceCombo );
                pDeviceCombo->pAdapterInfo = pAdapterInfo;
                pDeviceCombo->pDeviceInfo = pDeviceInfo;

                if( FAILED( pDeviceInfo->deviceSettingsComboList.Add( pDeviceCombo ) ) )
                    delete pDeviceCombo;
            }
        }
    }

    return S_OK;
}



//--------------------------------------------------------------------------------------
// Adds all depth/stencil formats that are compatible with the device 
//       and app to the given D3DDeviceCombo.
//--------------------------------------------------------------------------------------
void CD3D9Enumeration::BuildDepthStencilFormatList( CD3D9EnumDeviceSettingsCombo* pDeviceCombo )
{
    D3DFORMAT depthStencilFmt;
    for( int idsf = 0; idsf < m_DepthStencilPossibleList.GetSize(); idsf++ )
    {
        depthStencilFmt = m_DepthStencilPossibleList.GetAt(idsf);
        if (SUCCEEDED(m_pD3D->CheckDeviceFormat(pDeviceCombo->AdapterOrdinal, 
                pDeviceCombo->DeviceType, pDeviceCombo->AdapterFormat, 
                D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, depthStencilFmt)))
        {
            if (SUCCEEDED(m_pD3D->CheckDepthStencilMatch(pDeviceCombo->AdapterOrdinal, 
                    pDeviceCombo->DeviceType, pDeviceCombo->AdapterFormat, 
                    pDeviceCombo->BackBufferFormat, depthStencilFmt)))
            {
                pDeviceCombo->depthStencilFormatList.Add( depthStencilFmt );
            }
        }
    }
}




//--------------------------------------------------------------------------------------
// Adds all multisample types that are compatible with the device and app to
//       the given D3DDeviceCombo.
//--------------------------------------------------------------------------------------
void CD3D9Enumeration::BuildMultiSampleTypeList( CD3D9EnumDeviceSettingsCombo* pDeviceCombo )
{
    D3DMULTISAMPLE_TYPE msType;
    DWORD msQuality;
    for( int imst = 0; imst < m_MultiSampleTypeList.GetSize(); imst++ )
    {
        msType = m_MultiSampleTypeList.GetAt(imst);
        if( SUCCEEDED( m_pD3D->CheckDeviceMultiSampleType( pDeviceCombo->AdapterOrdinal, 
                pDeviceCombo->DeviceType, pDeviceCombo->BackBufferFormat, 
                pDeviceCombo->Windowed, msType, &msQuality ) ) )
        {
            pDeviceCombo->multiSampleTypeList.Add( msType );
            if( msQuality > m_nMultisampleQualityMax + 1 )
                msQuality = m_nMultisampleQualityMax + 1;
            pDeviceCombo->multiSampleQualityList.Add( msQuality );
        }
    }
}




//--------------------------------------------------------------------------------------
// Find any conflicts between the available depth/stencil formats and
//       multisample types.
//--------------------------------------------------------------------------------------
void CD3D9Enumeration::BuildDSMSConflictList( CD3D9EnumDeviceSettingsCombo* pDeviceCombo )
{
    CD3D9EnumDSMSConflict DSMSConflict;

    for( int iDS=0; iDS<pDeviceCombo->depthStencilFormatList.GetSize(); iDS++ )
    {
        D3DFORMAT dsFmt = pDeviceCombo->depthStencilFormatList.GetAt(iDS);

        for( int iMS=0; iMS<pDeviceCombo->multiSampleTypeList.GetSize(); iMS++ )
        {
            D3DMULTISAMPLE_TYPE msType = pDeviceCombo->multiSampleTypeList.GetAt(iMS);

            if( FAILED( m_pD3D->CheckDeviceMultiSampleType( pDeviceCombo->AdapterOrdinal, pDeviceCombo->DeviceType,
                                                            dsFmt, pDeviceCombo->Windowed, msType, NULL ) ) )
            {
                DSMSConflict.DSFormat = dsFmt;
                DSMSConflict.MSType = msType;
                pDeviceCombo->DSMSConflictList.Add( DSMSConflict );
            }
        }
    }
}


//--------------------------------------------------------------------------------------
// Adds all present intervals that are compatible with the device and app 
//       to the given D3DDeviceCombo.
//--------------------------------------------------------------------------------------
void CD3D9Enumeration::BuildPresentIntervalList( CD3D9EnumDeviceInfo* pDeviceInfo, 
                                                CD3D9EnumDeviceSettingsCombo* pDeviceCombo )
{
    UINT pi;
    for( int ipi = 0; ipi < m_PresentIntervalList.GetSize(); ipi++ )
    {
        pi = m_PresentIntervalList.GetAt(ipi);
        if( pDeviceCombo->Windowed )
        {
            if( pi == D3DPRESENT_INTERVAL_TWO ||
                pi == D3DPRESENT_INTERVAL_THREE ||
                pi == D3DPRESENT_INTERVAL_FOUR )
            {
                // These intervals are not supported in windowed mode.
                continue;
            }
        }
        // Note that D3DPRESENT_INTERVAL_DEFAULT is zero, so you
        // can't do a caps check for it -- it is always available.
        if( pi == D3DPRESENT_INTERVAL_DEFAULT ||
            (pDeviceInfo->Caps.PresentationIntervals & pi) )
        {
            pDeviceCombo->presentIntervalList.Add( pi );
        }
    }
}



//--------------------------------------------------------------------------------------
// Release all the allocated CD3D9EnumAdapterInfo objects and empty the list
//--------------------------------------------------------------------------------------
void CD3D9Enumeration::ClearAdapterInfoList()
{
    CD3D9EnumAdapterInfo* pAdapterInfo;
    for( int i=0; i<m_AdapterInfoList.GetSize(); i++ )
    {
        pAdapterInfo = m_AdapterInfoList.GetAt(i);
        delete pAdapterInfo;
    }

    m_AdapterInfoList.RemoveAll();
}



//--------------------------------------------------------------------------------------
// Call GetAdapterInfoList() after Enumerate() to get a STL vector of 
//       CD3D9EnumAdapterInfo* 
//--------------------------------------------------------------------------------------
CGrowableArray<CD3D9EnumAdapterInfo*>* CD3D9Enumeration::GetAdapterInfoList()
{
    return &m_AdapterInfoList;
}



//--------------------------------------------------------------------------------------
CD3D9EnumAdapterInfo* CD3D9Enumeration::GetAdapterInfo( UINT AdapterOrdinal )
{
    for( int iAdapter=0; iAdapter<m_AdapterInfoList.GetSize(); iAdapter++ )
    {
        CD3D9EnumAdapterInfo* pAdapterInfo = m_AdapterInfoList.GetAt(iAdapter);
        if( pAdapterInfo->AdapterOrdinal == AdapterOrdinal )
            return pAdapterInfo;
    }

    return NULL;
}


//--------------------------------------------------------------------------------------
CD3D9EnumDeviceInfo* CD3D9Enumeration::GetDeviceInfo( UINT AdapterOrdinal, D3DDEVTYPE DeviceType )
{
    CD3D9EnumAdapterInfo* pAdapterInfo = GetAdapterInfo( AdapterOrdinal );
    if( pAdapterInfo )
    {
        for( int iDeviceInfo=0; iDeviceInfo<pAdapterInfo->deviceInfoList.GetSize(); iDeviceInfo++ )
        {
            CD3D9EnumDeviceInfo* pDeviceInfo = pAdapterInfo->deviceInfoList.GetAt(iDeviceInfo);
            if( pDeviceInfo->DeviceType == DeviceType )
                return pDeviceInfo;
        }
    }

    return NULL;
}


//--------------------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------------------
CD3D9EnumDeviceSettingsCombo* CD3D9Enumeration::GetDeviceSettingsCombo( UINT AdapterOrdinal, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed )
{
    CD3D9EnumDeviceInfo* pDeviceInfo = GetDeviceInfo( AdapterOrdinal, DeviceType );
    if( pDeviceInfo )
    {
        for( int iDeviceCombo=0; iDeviceCombo<pDeviceInfo->deviceSettingsComboList.GetSize(); iDeviceCombo++ )
        {
            CD3D9EnumDeviceSettingsCombo* pDeviceSettingsCombo = pDeviceInfo->deviceSettingsComboList.GetAt(iDeviceCombo);
            if( pDeviceSettingsCombo->AdapterFormat == AdapterFormat &&
                pDeviceSettingsCombo->BackBufferFormat == BackBufferFormat &&
                pDeviceSettingsCombo->Windowed == bWindowed )
                return pDeviceSettingsCombo;
        }
    }
    static CD3D9EnumDeviceSettingsCombo* pDevCombo;
    if (DeviceType == D3DDEVTYPE_NULLREF)
    {
      if (!pDevCombo)
      {
        pDevCombo = new CD3D9EnumDeviceSettingsCombo;
        pDevCombo->AdapterFormat = D3DFMT_X8R8G8B8;
        pDevCombo->AdapterOrdinal = AdapterOrdinal;
        pDevCombo->BackBufferFormat = D3DFMT_A8R8G8B8;
        pDevCombo->depthStencilFormatList.Add(D3DFMT_D24S8);
        pDevCombo->DeviceType = DeviceType;
        pDevCombo->pAdapterInfo = new CD3D9EnumAdapterInfo;
        pDevCombo->pDeviceInfo = new CD3D9EnumDeviceInfo;
        pDevCombo->Windowed = TRUE;
      }
      return pDevCombo;
    }

    return NULL;
}


//--------------------------------------------------------------------------------------
// Returns the number of color channel bits in the specified D3DFORMAT
//--------------------------------------------------------------------------------------
UINT WINAPI DXUTGetD3D9ColorChannelBits( D3DFORMAT fmt )
{
    switch( fmt )
    {
        case D3DFMT_R8G8B8:
            return 8;
        case D3DFMT_A8R8G8B8:
            return 8;
        case D3DFMT_X8R8G8B8:
            return 8;
        case D3DFMT_R5G6B5:
            return 5;
        case D3DFMT_X1R5G5B5:
            return 5;
        case D3DFMT_A1R5G5B5:
            return 5;
        case D3DFMT_A4R4G4B4:
            return 4;
        case D3DFMT_R3G3B2:
            return 2;
        case D3DFMT_A8R3G3B2:
            return 2;
        case D3DFMT_X4R4G4B4:
            return 4;
        case D3DFMT_A2B10G10R10:
            return 10;
        case D3DFMT_A8B8G8R8:
            return 8;
        case D3DFMT_A2R10G10B10:
            return 10;
        case D3DFMT_A16B16G16R16:
            return 16;
        default:
            return 0;
    }
}

//--------------------------------------------------------------------------------------
// Returns the number of alpha channel bits in the specified D3DFORMAT
//--------------------------------------------------------------------------------------
UINT WINAPI DXUTGetAlphaChannelBits( D3DFORMAT fmt )
{
    switch( fmt )
    {
        case D3DFMT_R8G8B8:
            return 0;
        case D3DFMT_A8R8G8B8:
            return 8;
        case D3DFMT_X8R8G8B8:
            return 0;
        case D3DFMT_R5G6B5:
            return 0;
        case D3DFMT_X1R5G5B5:
            return 0;
        case D3DFMT_A1R5G5B5:
            return 1;
        case D3DFMT_A4R4G4B4:
            return 4;
        case D3DFMT_R3G3B2:
            return 0;
        case D3DFMT_A8R3G3B2:
            return 8;
        case D3DFMT_X4R4G4B4:
            return 0;
        case D3DFMT_A2B10G10R10:
            return 2;
        case D3DFMT_A8B8G8R8:
            return 8;
        case D3DFMT_A2R10G10B10:
            return 2;
        case D3DFMT_A16B16G16R16:
            return 16;
        default:
            return 0;
    }
}


//--------------------------------------------------------------------------------------
// Returns the number of depth bits in the specified D3DFORMAT
//--------------------------------------------------------------------------------------
UINT WINAPI DXUTGetDepthBits( D3DFORMAT fmt )
{
    switch( fmt )
    {
        case D3DFMT_D32F_LOCKABLE:
        case D3DFMT_D32:
            return 32;

        case D3DFMT_D24X8:
        case D3DFMT_D24S8:
        case D3DFMT_D24X4S4:
        case D3DFMT_D24FS8:
            return 24;

        case D3DFMT_D16_LOCKABLE:
        case D3DFMT_D16:
            return 16;

        case D3DFMT_D15S1:
            return 15;

        default:
            return 0;
    }
}




//--------------------------------------------------------------------------------------
// Returns the number of stencil bits in the specified D3DFORMAT
//--------------------------------------------------------------------------------------
UINT WINAPI DXUTGetStencilBits( D3DFORMAT fmt )
{
    switch( fmt )
    {
        case D3DFMT_D16_LOCKABLE:
        case D3DFMT_D16:
        case D3DFMT_D32F_LOCKABLE:
        case D3DFMT_D32:
        case D3DFMT_D24X8:
            return 0;

        case D3DFMT_D15S1:
            return 1;

        case D3DFMT_D24X4S4:
            return 4;

        case D3DFMT_D24S8:
        case D3DFMT_D24FS8:
            return 8;

        default:
            return 0;
    }
}



//--------------------------------------------------------------------------------------
// Used to sort D3DDISPLAYMODEs
//--------------------------------------------------------------------------------------
static int __cdecl SortModesCallback( const void* arg1, const void* arg2 )
{
    D3DDISPLAYMODE* pdm1 = (D3DDISPLAYMODE*)arg1;
    D3DDISPLAYMODE* pdm2 = (D3DDISPLAYMODE*)arg2;

    if (pdm1->Width > pdm2->Width)
        return 1;
    if (pdm1->Width < pdm2->Width)
        return -1;
    if (pdm1->Height > pdm2->Height)
        return 1;
    if (pdm1->Height < pdm2->Height)
        return -1;
    if (pdm1->Format > pdm2->Format)
        return 1;
    if (pdm1->Format < pdm2->Format)
        return -1;
    if (pdm1->RefreshRate > pdm2->RefreshRate)
        return 1;
    if (pdm1->RefreshRate < pdm2->RefreshRate)
        return -1;
    return 0;
}



//--------------------------------------------------------------------------------------
CD3D9EnumAdapterInfo::~CD3D9EnumAdapterInfo( void )
{
    CD3D9EnumDeviceInfo* pDeviceInfo;
    for( int i=0; i<deviceInfoList.GetSize(); i++ )
    {
        pDeviceInfo = deviceInfoList.GetAt(i);
        delete pDeviceInfo;
    }
    deviceInfoList.RemoveAll();
}




//--------------------------------------------------------------------------------------
CD3D9EnumDeviceInfo::~CD3D9EnumDeviceInfo( void )
{
    CD3D9EnumDeviceSettingsCombo* pDeviceCombo;
    for( int i=0; i<deviceSettingsComboList.GetSize(); i++ )
    {
        pDeviceCombo = deviceSettingsComboList.GetAt(i);
        delete pDeviceCombo;
    }
    deviceSettingsComboList.RemoveAll();
}


//--------------------------------------------------------------------------------------
void CD3D9Enumeration::ResetPossibleDepthStencilFormats()
{
    m_DepthStencilPossibleList.RemoveAll();
    m_DepthStencilPossibleList.Add( D3DFMT_D16 );
    m_DepthStencilPossibleList.Add( D3DFMT_D15S1 );
    m_DepthStencilPossibleList.Add( D3DFMT_D24X8 );
    m_DepthStencilPossibleList.Add( D3DFMT_D24S8 );
    m_DepthStencilPossibleList.Add( D3DFMT_D24X4S4 );
    m_DepthStencilPossibleList.Add( D3DFMT_D32 );
}


//--------------------------------------------------------------------------------------
CGrowableArray<D3DFORMAT>* CD3D9Enumeration::GetPossibleDepthStencilFormatList() 
{
    return &m_DepthStencilPossibleList;
}


//--------------------------------------------------------------------------------------
CGrowableArray<D3DMULTISAMPLE_TYPE>* CD3D9Enumeration::GetPossibleMultisampleTypeList()
{
    return &m_MultiSampleTypeList;
}

UINT CD3D9Enumeration::GetMaxMultisampleQuality()
{
    return m_nMultisampleQualityMax;
}

//--------------------------------------------------------------------------------------
void CD3D9Enumeration::ResetPossibleMultisampleTypeList()
{
    m_MultiSampleTypeList.RemoveAll();
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_NONE );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_NONMASKABLE );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_2_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_3_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_4_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_5_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_6_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_7_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_8_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_9_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_10_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_11_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_12_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_13_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_14_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_15_SAMPLES );
    m_MultiSampleTypeList.Add( D3DMULTISAMPLE_16_SAMPLES );
}


//--------------------------------------------------------------------------------------
void CD3D9Enumeration::GetPossibleVertexProcessingList( bool* pbSoftwareVP, bool* pbHardwareVP, bool* pbPureHarewareVP, bool* pbMixedVP )
{
    *pbSoftwareVP = m_bSoftwareVP;
    *pbHardwareVP = m_bHardwareVP;
    *pbPureHarewareVP = m_bPureHarewareVP;
    *pbMixedVP = m_bMixedVP;
}


//--------------------------------------------------------------------------------------
void CD3D9Enumeration::SetPossibleVertexProcessingList( bool bSoftwareVP, bool bHardwareVP, bool bPureHarewareVP, bool bMixedVP )
{
    m_bSoftwareVP = bSoftwareVP;
    m_bHardwareVP = bHardwareVP;
    m_bPureHarewareVP = bPureHarewareVP;
    m_bMixedVP = bMixedVP;
}


//--------------------------------------------------------------------------------------
CGrowableArray<UINT>* CD3D9Enumeration::GetPossiblePresentIntervalList()
{
    return &m_PresentIntervalList;
}


//--------------------------------------------------------------------------------------
void CD3D9Enumeration::ResetPossiblePresentIntervalList()
{
    m_PresentIntervalList.RemoveAll();
    m_PresentIntervalList.Add( D3DPRESENT_INTERVAL_IMMEDIATE );
    m_PresentIntervalList.Add( D3DPRESENT_INTERVAL_DEFAULT );
    m_PresentIntervalList.Add( D3DPRESENT_INTERVAL_ONE );
    m_PresentIntervalList.Add( D3DPRESENT_INTERVAL_TWO );
    m_PresentIntervalList.Add( D3DPRESENT_INTERVAL_THREE );
    m_PresentIntervalList.Add( D3DPRESENT_INTERVAL_FOUR );
}


//--------------------------------------------------------------------------------------
void CD3D9Enumeration::SetResolutionMinMax( UINT nMinWidth, UINT nMinHeight, 
                                           UINT nMaxWidth, UINT nMaxHeight )
{
    m_nMinWidth = nMinWidth;
    m_nMinHeight = nMinHeight;
    m_nMaxWidth = nMaxWidth;
    m_nMaxHeight = nMaxHeight;
}

void CD3D9Enumeration::GetResolutionMinMax(UINT& nMinWidth, UINT& nMinHeight, 
                                           UINT& nMaxWidth, UINT& nMaxHeight )
{
    nMinWidth = m_nMinWidth;
    nMinHeight = m_nMinHeight;
    nMaxWidth = m_nMaxWidth;
    nMaxHeight = m_nMaxHeight;
}

//--------------------------------------------------------------------------------------
void CD3D9Enumeration::SetRefreshMinMax( UINT nMin, UINT nMax )
{
    m_nRefreshMin = nMin;
    m_nRefreshMax = nMax;
}


//--------------------------------------------------------------------------------------
void CD3D9Enumeration::SetMultisampleQualityMax( UINT nMax )
{
    if( nMax > 0xFFFF )
        nMax = 0xFFFF;
    m_nMultisampleQualityMax = nMax;
}

UINT WINAPI GetD3D9ColorBits( D3DFORMAT fmt );


HRESULT DXUTFindBestResolution(CD3D9EnumDeviceSettingsCombo* pDeviceSettingsCombo, DXUTD3D9DeviceSettings* pIn)
{
  int i;

  int curCD = GetD3D9ColorBits(pIn->AdapterFormat);
  int BestError = 999999;
  int Best;
  int BestCD;

  // Need to find a display mode on the best adapter that uses pBestDeviceCombo->AdapterFormat
  // and is as close to bestAdapterDesktopDisplayMode's res as possible
  D3DDISPLAYMODE bestDisplayMode;
  bestDisplayMode.Width = 0;
  bestDisplayMode.Height = 0;
  bestDisplayMode.Format = D3DFMT_UNKNOWN;
  bestDisplayMode.RefreshRate = 0;
  for (i=0; i<pDeviceSettingsCombo->pAdapterInfo->displayModeList.GetSize(); i++)
  {
    D3DDISPLAYMODE displayMode = pDeviceSettingsCombo->pAdapterInfo->displayModeList.GetAt( i );
    if( displayMode.Format != pDeviceSettingsCombo->AdapterFormat )
      continue; // Skip this display mode if it doesn't match the combo's adapter format

    int thisCD = GetD3D9ColorBits(pDeviceSettingsCombo->AdapterFormat);
    if (thisCD <= 0)
      continue;
    int ThisError = abs((int)displayMode.Width-(int)pIn->pp.BackBufferWidth) + abs((int)displayMode.Height-(int)pIn->pp.BackBufferHeight) + abs(thisCD-curCD);
    if (ThisError < BestError)
    {
      Best = i;
      BestCD = thisCD;
      BestError = ThisError;
      bestDisplayMode = displayMode;
    }
  }
  if (BestError == 999999)
    return S_FALSE;

  pIn->pp.BackBufferWidth = bestDisplayMode.Width;
  pIn->pp.BackBufferHeight = bestDisplayMode.Height;

  iLog->Log("Best display mode: %dx%dx%d", bestDisplayMode.Width, bestDisplayMode.Height, BestCD);

  return S_OK;
}

//--------------------------------------------------------------------------------------
HRESULT DXUTFindValidD3D9DeviceSettings( DXUTD3D9DeviceSettings* pOut, DXUTD3D9DeviceSettings* pIn, DXUTMatchOptions* pMatchOptions, DXUTD3D9DeviceSettings* pOptimal )
{
    // Find the best combination of:
    //      Adapter Ordinal
    //      Device Type
    //      Adapter Format
    //      Back Buffer Format
    //      Windowed
    // given what's available on the system and the match options combined with the device settings input.
    // This combination of settings is encapsulated by the CD3D9EnumDeviceSettingsCombo class.
    float fBestRanking = -1.0f;
    CD3D9EnumDeviceSettingsCombo* pBestDeviceSettingsCombo = NULL;
    D3DDISPLAYMODE adapterDesktopDisplayMode;

    IDirect3D9* pD3D = DXUTGetD3D9Object();
    CD3D9Enumeration* pd3dEnum = DXUTGetD3D9Enumeration( false );
    CGrowableArray<CD3D9EnumAdapterInfo*>* pAdapterList = pd3dEnum->GetAdapterInfoList();
    for( int iAdapter=0; iAdapter<pAdapterList->GetSize(); iAdapter++ )
    {
        CD3D9EnumAdapterInfo* pAdapterInfo = pAdapterList->GetAt(iAdapter);

        // Get the desktop display mode of adapter 
        pD3D->GetAdapterDisplayMode( pAdapterInfo->AdapterOrdinal, &adapterDesktopDisplayMode );

        // Enum all the device types supported by this adapter to find the best device settings
        for( int iDeviceInfo=0; iDeviceInfo<pAdapterInfo->deviceInfoList.GetSize(); iDeviceInfo++ )
        {
            CD3D9EnumDeviceInfo* pDeviceInfo = pAdapterInfo->deviceInfoList.GetAt(iDeviceInfo);

            // Enum all the device settings combinations.  A device settings combination is 
            // a unique set of an adapter format, back buffer format, and IsWindowed.
            for( int iDeviceCombo=0; iDeviceCombo<pDeviceInfo->deviceSettingsComboList.GetSize(); iDeviceCombo++ )
            {
                CD3D9EnumDeviceSettingsCombo* pDeviceSettingsCombo = pDeviceInfo->deviceSettingsComboList.GetAt(iDeviceCombo);

                // If windowed mode the adapter format has to be the same as the desktop 
                // display mode format so skip any that don't match
                if (pDeviceSettingsCombo->Windowed && (pDeviceSettingsCombo->AdapterFormat != adapterDesktopDisplayMode.Format))
                    continue;

                // Skip any combo that doesn't meet the preserve match options
                if( false == DXUTDoesD3D9DeviceComboMatchPreserveOptions( pDeviceSettingsCombo, pIn, pMatchOptions ) )
                    continue;

                // Get a ranking number that describes how closely this device combo matches the optimal combo
                float fCurRanking = DXUTRankD3D9DeviceCombo( pDeviceSettingsCombo, pOptimal, &adapterDesktopDisplayMode );

                // If this combo better matches the input device settings then save it
                if( fCurRanking > fBestRanking )
                {
                    pBestDeviceSettingsCombo = pDeviceSettingsCombo;
                    fBestRanking = fCurRanking;
                }
            }
        }
    }

    // If no best device combination was found then fail
    if( pBestDeviceSettingsCombo == NULL ) 
        return DXUTERR_NOCOMPATIBLEDEVICES;

    if (!pIn->pp.Windowed)
      DXUTFindBestResolution(pBestDeviceSettingsCombo, pIn);

    // Using the best device settings combo found, build valid device settings taking heed of 
    // the match options and the input device settings
    DXUTD3D9DeviceSettings validDeviceSettings;
    DXUTBuildValidD3D9DeviceSettings( &validDeviceSettings, pBestDeviceSettingsCombo, pIn, pMatchOptions );
    *pOut = validDeviceSettings;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Internal helper function to build a D3D9 device settings structure based upon the match 
// options.  If the match option is set to ignore, then a optimal default value is used.
// The default value may not exist on the system, but later this will be taken 
// into account.
//--------------------------------------------------------------------------------------
void DXUTBuildOptimalD3D9DeviceSettings( DXUTD3D9DeviceSettings* pOptimalDeviceSettings, 
                                         DXUTD3D9DeviceSettings* pDeviceSettingsIn, 
                                         DXUTMatchOptions* pMatchOptions )
{
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    D3DDISPLAYMODE adapterDesktopDisplayMode;

    ZeroMemory( pOptimalDeviceSettings, sizeof(DXUTD3D9DeviceSettings) ); 

    //---------------------
    // Adapter ordinal
    //---------------------    
    if( pMatchOptions->eAdapterOrdinal == DXUTMT_IGNORE_INPUT )
        pOptimalDeviceSettings->AdapterOrdinal = D3DADAPTER_DEFAULT; 
    else
        pOptimalDeviceSettings->AdapterOrdinal = pDeviceSettingsIn->AdapterOrdinal;      

    //---------------------
    // Device type
    //---------------------
    if( pMatchOptions->eDeviceType == DXUTMT_IGNORE_INPUT )
        pOptimalDeviceSettings->DeviceType = D3DDEVTYPE_HAL; 
    else
        pOptimalDeviceSettings->DeviceType = pDeviceSettingsIn->DeviceType;

    //---------------------
    // Windowed
    //---------------------
    if( pMatchOptions->eWindowed == DXUTMT_IGNORE_INPUT )
        pOptimalDeviceSettings->pp.Windowed = TRUE; 
    else
        pOptimalDeviceSettings->pp.Windowed = pDeviceSettingsIn->pp.Windowed;

    //---------------------
    // Adapter format
    //---------------------
    if( pMatchOptions->eAdapterFormat == DXUTMT_IGNORE_INPUT )
    {
        // If windowed, default to the desktop display mode
        // If fullscreen, default to the desktop display mode for quick mode change or 
        // default to D3DFMT_X8R8G8B8 if the desktop display mode is < 32bit
        pD3D->GetAdapterDisplayMode( pOptimalDeviceSettings->AdapterOrdinal, &adapterDesktopDisplayMode );
        if( pOptimalDeviceSettings->pp.Windowed || DXUTGetD3D9ColorChannelBits(adapterDesktopDisplayMode.Format) >= 8 )
            pOptimalDeviceSettings->AdapterFormat = adapterDesktopDisplayMode.Format;
        else
            pOptimalDeviceSettings->AdapterFormat = D3DFMT_X8R8G8B8;
    }
    else
    {
        pOptimalDeviceSettings->AdapterFormat = pDeviceSettingsIn->AdapterFormat;
    }

    //---------------------
    // Vertex processing
    //---------------------
    if( pMatchOptions->eVertexProcessing == DXUTMT_IGNORE_INPUT )
        pOptimalDeviceSettings->BehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING; 
    else
        pOptimalDeviceSettings->BehaviorFlags = pDeviceSettingsIn->BehaviorFlags;

    //---------------------
    // Resolution
    //---------------------
    if( pMatchOptions->eResolution == DXUTMT_IGNORE_INPUT )
    {
        // If windowed, default to 640x480
        // If fullscreen, default to the desktop res for quick mode change
        if( pOptimalDeviceSettings->pp.Windowed )
        {
            pOptimalDeviceSettings->pp.BackBufferWidth = 640;
            pOptimalDeviceSettings->pp.BackBufferHeight = 480;
        }
        else
        {
            pD3D->GetAdapterDisplayMode( pOptimalDeviceSettings->AdapterOrdinal, &adapterDesktopDisplayMode );
            pOptimalDeviceSettings->pp.BackBufferWidth = adapterDesktopDisplayMode.Width;
            pOptimalDeviceSettings->pp.BackBufferHeight = adapterDesktopDisplayMode.Height;
        }
    }
    else
    {
        pOptimalDeviceSettings->pp.BackBufferWidth = pDeviceSettingsIn->pp.BackBufferWidth;
        pOptimalDeviceSettings->pp.BackBufferHeight = pDeviceSettingsIn->pp.BackBufferHeight;
    }

    //---------------------
    // Back buffer format
    //---------------------
    if( pMatchOptions->eBackBufferFormat == DXUTMT_IGNORE_INPUT )
        pOptimalDeviceSettings->pp.BackBufferFormat = pOptimalDeviceSettings->AdapterFormat; // Default to match the adapter format
    else
        pOptimalDeviceSettings->pp.BackBufferFormat = pDeviceSettingsIn->pp.BackBufferFormat;

    //---------------------
    // Back buffer count
    //---------------------
    if( pMatchOptions->eBackBufferCount == DXUTMT_IGNORE_INPUT )
        pOptimalDeviceSettings->pp.BackBufferCount = 2; // Default to triple buffering for perf gain
    else
        pOptimalDeviceSettings->pp.BackBufferCount = pDeviceSettingsIn->pp.BackBufferCount;
   
    //---------------------
    // Multisample
    //---------------------
    if( pMatchOptions->eMultiSample == DXUTMT_IGNORE_INPUT )
    {
        // Default to no multisampling 
        pOptimalDeviceSettings->pp.MultiSampleType = D3DMULTISAMPLE_NONE;
        pOptimalDeviceSettings->pp.MultiSampleQuality = 0; 
    }
    else
    {
        pOptimalDeviceSettings->pp.MultiSampleType = pDeviceSettingsIn->pp.MultiSampleType;
        pOptimalDeviceSettings->pp.MultiSampleQuality = pDeviceSettingsIn->pp.MultiSampleQuality;
    }

    //---------------------
    // Swap effect
    //---------------------
    if( pMatchOptions->eSwapEffect == DXUTMT_IGNORE_INPUT )
        pOptimalDeviceSettings->pp.SwapEffect = D3DSWAPEFFECT_DISCARD; 
    else
        pOptimalDeviceSettings->pp.SwapEffect = pDeviceSettingsIn->pp.SwapEffect;

    //---------------------
    // Depth stencil 
    //---------------------
    if( pMatchOptions->eDepthFormat == DXUTMT_IGNORE_INPUT &&
        pMatchOptions->eStencilFormat == DXUTMT_IGNORE_INPUT )
    {
        UINT nBackBufferBits = DXUTGetD3D9ColorChannelBits( pOptimalDeviceSettings->pp.BackBufferFormat );
        if( nBackBufferBits >= 8 )
            pOptimalDeviceSettings->pp.AutoDepthStencilFormat = D3DFMT_D32; 
        else
            pOptimalDeviceSettings->pp.AutoDepthStencilFormat = D3DFMT_D16; 
    }
    else
    {
        pOptimalDeviceSettings->pp.AutoDepthStencilFormat = pDeviceSettingsIn->pp.AutoDepthStencilFormat;
    }

    //---------------------
    // Present flags
    //---------------------
    if( pMatchOptions->ePresentFlags == DXUTMT_IGNORE_INPUT )
        pOptimalDeviceSettings->pp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    else
        pOptimalDeviceSettings->pp.Flags = pDeviceSettingsIn->pp.Flags;

    //---------------------
    // Refresh rate
    //---------------------
    if( pMatchOptions->eRefreshRate == DXUTMT_IGNORE_INPUT )
        pOptimalDeviceSettings->pp.FullScreen_RefreshRateInHz = 0;
    else
        pOptimalDeviceSettings->pp.FullScreen_RefreshRateInHz = pDeviceSettingsIn->pp.FullScreen_RefreshRateInHz;

    //---------------------
    // Present interval
    //---------------------
    if( pMatchOptions->ePresentInterval == DXUTMT_IGNORE_INPUT )
    {
        // For windowed and fullscreen, default to D3DPRESENT_INTERVAL_DEFAULT
        // which will wait for the vertical retrace period to prevent tearing.
        // For benchmarking, use D3DPRESENT_INTERVAL_IMMEDIATE which will
        // will wait not for the vertical retrace period but may introduce tearing.
        pOptimalDeviceSettings->pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    }
    else
    {
        pOptimalDeviceSettings->pp.PresentationInterval = pDeviceSettingsIn->pp.PresentationInterval;
    }
}


//--------------------------------------------------------------------------------------
// Returns false for any CD3D9EnumDeviceSettingsCombo that doesn't meet the preserve 
// match options against the input pDeviceSettingsIn.
//--------------------------------------------------------------------------------------
bool DXUTDoesD3D9DeviceComboMatchPreserveOptions( CD3D9EnumDeviceSettingsCombo* pDeviceSettingsCombo, 
                                                  DXUTD3D9DeviceSettings* pDeviceSettingsIn, 
                                                  DXUTMatchOptions* pMatchOptions )
{
    //---------------------
    // Adapter ordinal
    //---------------------
    if( pMatchOptions->eAdapterOrdinal == DXUTMT_PRESERVE_INPUT && 
        (pDeviceSettingsCombo->AdapterOrdinal != pDeviceSettingsIn->AdapterOrdinal) )
        return false;

    //---------------------
    // Device type
    //---------------------
    if( pMatchOptions->eDeviceType == DXUTMT_PRESERVE_INPUT && 
        (pDeviceSettingsCombo->DeviceType != pDeviceSettingsIn->DeviceType) )
        return false;

    //---------------------
    // Windowed
    //---------------------
    if( pMatchOptions->eWindowed == DXUTMT_PRESERVE_INPUT && 
        (pDeviceSettingsCombo->Windowed != pDeviceSettingsIn->pp.Windowed) )
        return false;

    //---------------------
    // Adapter format
    //---------------------
    if( pMatchOptions->eAdapterFormat == DXUTMT_PRESERVE_INPUT && 
        (pDeviceSettingsCombo->AdapterFormat != pDeviceSettingsIn->AdapterFormat) )
        return false;

    //---------------------
    // Vertex processing
    //---------------------
    // If keep VP and input has HWVP, then skip if this combo doesn't have HWTL 
    if( pMatchOptions->eVertexProcessing == DXUTMT_PRESERVE_INPUT && 
        ((pDeviceSettingsIn->BehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) != 0) && 
        ((pDeviceSettingsCombo->pDeviceInfo->Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0) )
        return false;

    //---------------------
    // Resolution
    //---------------------
    // If keep resolution then check that width and height supported by this combo
    if( pMatchOptions->eResolution == DXUTMT_PRESERVE_INPUT )
    {
        bool bFound = false;
        for( int i=0; i< pDeviceSettingsCombo->pAdapterInfo->displayModeList.GetSize(); i++ )
        {
            D3DDISPLAYMODE displayMode = pDeviceSettingsCombo->pAdapterInfo->displayModeList.GetAt( i );
            if( displayMode.Format != pDeviceSettingsCombo->AdapterFormat )
                continue; // Skip this display mode if it doesn't match the combo's adapter format

            if( displayMode.Width == pDeviceSettingsIn->pp.BackBufferWidth &&
                displayMode.Height == pDeviceSettingsIn->pp.BackBufferHeight )
            {
                bFound = true;
                break;
            }
        }

        // If the width and height are not supported by this combo, return false
        if( !bFound )
            return false;
    }

    //---------------------
    // Back buffer format
    //---------------------
    if( pMatchOptions->eBackBufferFormat == DXUTMT_PRESERVE_INPUT && 
        pDeviceSettingsCombo->BackBufferFormat != pDeviceSettingsIn->pp.BackBufferFormat )
        return false;

    //---------------------
    // Back buffer count
    //---------------------
    // No caps for the back buffer count

    //---------------------
    // Multisample
    //---------------------
    if( pMatchOptions->eMultiSample == DXUTMT_PRESERVE_INPUT )
    {
        bool bFound = false;
        for( int i=0; i<pDeviceSettingsCombo->multiSampleTypeList.GetSize(); i++ )
        {
            D3DMULTISAMPLE_TYPE msType = pDeviceSettingsCombo->multiSampleTypeList.GetAt(i);
            DWORD msQuality  = pDeviceSettingsCombo->multiSampleQualityList.GetAt(i);

            if( msType == pDeviceSettingsIn->pp.MultiSampleType &&
                msQuality > pDeviceSettingsIn->pp.MultiSampleQuality )
            {
                bFound = true;
                break;
            }
        }

        // If multisample type/quality not supported by this combo, then return false
        if( !bFound )
            return false;
    }
        
    //---------------------
    // Swap effect
    //---------------------
    // No caps for swap effects

    //---------------------
    // Depth stencil 
    //---------------------
    // If keep depth stencil format then check that the depth stencil format is supported by this combo
    if( pMatchOptions->eDepthFormat == DXUTMT_PRESERVE_INPUT &&
        pMatchOptions->eStencilFormat == DXUTMT_PRESERVE_INPUT )
    {
        if( pDeviceSettingsIn->pp.AutoDepthStencilFormat != D3DFMT_UNKNOWN &&
            !pDeviceSettingsCombo->depthStencilFormatList.Contains( pDeviceSettingsIn->pp.AutoDepthStencilFormat ) )
            return false;
    }

    // If keep depth format then check that the depth format is supported by this combo
    if( pMatchOptions->eDepthFormat == DXUTMT_PRESERVE_INPUT &&
        pDeviceSettingsIn->pp.AutoDepthStencilFormat != D3DFMT_UNKNOWN )
    {
        bool bFound = false;
        UINT dwDepthBits = DXUTGetDepthBits( pDeviceSettingsIn->pp.AutoDepthStencilFormat );
        for( int i=0; i<pDeviceSettingsCombo->depthStencilFormatList.GetSize(); i++ )
        {
            D3DFORMAT depthStencilFmt = pDeviceSettingsCombo->depthStencilFormatList.GetAt(i);
            UINT dwCurDepthBits = DXUTGetDepthBits( depthStencilFmt );
            if( dwCurDepthBits - dwDepthBits == 0)
                bFound = true;
        }

        if( !bFound )
            return false;
    }

    // If keep depth format then check that the depth format is supported by this combo
    if( pMatchOptions->eStencilFormat == DXUTMT_PRESERVE_INPUT &&
        pDeviceSettingsIn->pp.AutoDepthStencilFormat != D3DFMT_UNKNOWN )
    {
        bool bFound = false;
        UINT dwStencilBits = DXUTGetStencilBits( pDeviceSettingsIn->pp.AutoDepthStencilFormat );
        for( int i=0; i<pDeviceSettingsCombo->depthStencilFormatList.GetSize(); i++ )
        {
            D3DFORMAT depthStencilFmt = pDeviceSettingsCombo->depthStencilFormatList.GetAt(i);
            UINT dwCurStencilBits = DXUTGetStencilBits( depthStencilFmt );
            if( dwCurStencilBits - dwStencilBits == 0)
                bFound = true;
        }

        if( !bFound )
            return false;
    }

    //---------------------
    // Present flags
    //---------------------
    // No caps for the present flags

    //---------------------
    // Refresh rate
    //---------------------
    // If keep refresh rate then check that the resolution is supported by this combo
    if( pMatchOptions->eRefreshRate == DXUTMT_PRESERVE_INPUT )
    {
        bool bFound = false;
        for( int i=0; i<pDeviceSettingsCombo->pAdapterInfo->displayModeList.GetSize(); i++ )
        {
            D3DDISPLAYMODE displayMode = pDeviceSettingsCombo->pAdapterInfo->displayModeList.GetAt( i );
            if( displayMode.Format != pDeviceSettingsCombo->AdapterFormat )
                continue;
            if( displayMode.RefreshRate == pDeviceSettingsIn->pp.FullScreen_RefreshRateInHz )
            {
                bFound = true;
                break;
            }
        }

        // If refresh rate not supported by this combo, then return false
        if( !bFound )
            return false;
    }

    //---------------------
    // Present interval
    //---------------------
    // If keep present interval then check that the present interval is supported by this combo
    if( pMatchOptions->ePresentInterval == DXUTMT_PRESERVE_INPUT &&
        !pDeviceSettingsCombo->presentIntervalList.Contains( pDeviceSettingsIn->pp.PresentationInterval ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// Returns a ranking number that describes how closely this device 
// combo matches the optimal combo based on the match options and the optimal device settings
//--------------------------------------------------------------------------------------
float DXUTRankD3D9DeviceCombo( CD3D9EnumDeviceSettingsCombo* pDeviceSettingsCombo, 
                               DXUTD3D9DeviceSettings* pOptimalDeviceSettings,
                               D3DDISPLAYMODE* pAdapterDesktopDisplayMode )
{
    float fCurRanking = 0.0f;

    // Arbitrary weights.  Gives preference to the ordinal, device type, and windowed
    const float fAdapterOrdinalWeight   = 1000.0f;
    const float fDeviceTypeWeight       = 100.0f;
    const float fWindowWeight           = 10.0f;
    const float fAdapterFormatWeight    = 1.0f;
    const float fVertexProcessingWeight = 1.0f;
    const float fResolutionWeight       = 1.0f;
    const float fBackBufferFormatWeight = 1.0f;
    const float fMultiSampleWeight      = 1.0f;
    const float fDepthStencilWeight     = 1.0f;
    const float fRefreshRateWeight      = 1.0f;
    const float fPresentIntervalWeight  = 1.0f;

    //---------------------
    // Adapter ordinal
    //---------------------
    if( pDeviceSettingsCombo->AdapterOrdinal == pOptimalDeviceSettings->AdapterOrdinal )
        fCurRanking += fAdapterOrdinalWeight;

    //---------------------
    // Device type
    //---------------------
    if( pDeviceSettingsCombo->DeviceType == pOptimalDeviceSettings->DeviceType )
        fCurRanking += fDeviceTypeWeight;
    // Slightly prefer HAL 
    if( pDeviceSettingsCombo->DeviceType == D3DDEVTYPE_HAL )
        fCurRanking += 0.1f; 

    //---------------------
    // Windowed
    //---------------------
    if( pDeviceSettingsCombo->Windowed == pOptimalDeviceSettings->pp.Windowed )
        fCurRanking += fWindowWeight;

    //---------------------
    // Adapter format
    //---------------------
    if( pDeviceSettingsCombo->AdapterFormat == pOptimalDeviceSettings->AdapterFormat )
    {
        fCurRanking += fAdapterFormatWeight;
    }
    else
    {
        int nBitDepthDelta = abs( (long) DXUTGetD3D9ColorChannelBits(pDeviceSettingsCombo->AdapterFormat) -
                                  (long) DXUTGetD3D9ColorChannelBits(pOptimalDeviceSettings->AdapterFormat) );
        float fScale = __max(0.9f - (float)nBitDepthDelta*0.2f, 0.0f);
        fCurRanking += fScale * fAdapterFormatWeight;
    }

    if( !pDeviceSettingsCombo->Windowed )
    {
        // Slightly prefer when it matches the desktop format or is D3DFMT_X8R8G8B8
        bool bAdapterOptimalMatch;
        if( DXUTGetD3D9ColorChannelBits(pAdapterDesktopDisplayMode->Format) >= 8 )
            bAdapterOptimalMatch = (pDeviceSettingsCombo->AdapterFormat == pAdapterDesktopDisplayMode->Format);
        else
            bAdapterOptimalMatch = (pDeviceSettingsCombo->AdapterFormat == D3DFMT_X8R8G8B8);

        if( bAdapterOptimalMatch )
            fCurRanking += 0.1f;
    }

    //---------------------
    // Vertex processing
    //---------------------
    if( (pOptimalDeviceSettings->BehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) != 0 || 
        (pOptimalDeviceSettings->BehaviorFlags & D3DCREATE_MIXED_VERTEXPROCESSING) != 0 )
    {
        if( (pDeviceSettingsCombo->pDeviceInfo->Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0 )
            fCurRanking += fVertexProcessingWeight;
    }
    // Slightly prefer HW T&L
    if( (pDeviceSettingsCombo->pDeviceInfo->Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0 )
        fCurRanking += 0.1f;

    //---------------------
    // Resolution
    //---------------------
    bool bResolutionFound = false;
    for( int idm = 0; idm < pDeviceSettingsCombo->pAdapterInfo->displayModeList.GetSize(); idm++ )
    {
        D3DDISPLAYMODE displayMode = pDeviceSettingsCombo->pAdapterInfo->displayModeList.GetAt( idm );
        if( displayMode.Format != pDeviceSettingsCombo->AdapterFormat )
            continue;
        if( displayMode.Width == pOptimalDeviceSettings->pp.BackBufferWidth &&
            displayMode.Height == pOptimalDeviceSettings->pp.BackBufferHeight )
            bResolutionFound = true;
    }
    if( bResolutionFound )
        fCurRanking += fResolutionWeight;

    //---------------------
    // Back buffer format
    //---------------------
    if( pDeviceSettingsCombo->BackBufferFormat == pOptimalDeviceSettings->pp.BackBufferFormat )
    {
        fCurRanking += fBackBufferFormatWeight;
    }
    else
    {
        int nBitDepthDelta = abs( (long) DXUTGetD3D9ColorChannelBits(pDeviceSettingsCombo->BackBufferFormat) -
                                  (long) DXUTGetD3D9ColorChannelBits(pOptimalDeviceSettings->pp.BackBufferFormat) );
        float fScale = __max(0.9f - (float)nBitDepthDelta*0.2f, 0.0f);
        fCurRanking += fScale * fBackBufferFormatWeight;
    }

    // Check if this back buffer format is the same as 
    // the adapter format since this is preferred.
    bool bAdapterMatchesBB = (pDeviceSettingsCombo->BackBufferFormat == pDeviceSettingsCombo->AdapterFormat);
    if( bAdapterMatchesBB )
        fCurRanking += 0.1f;

    //---------------------
    // Back buffer count
    //---------------------
    // No caps for the back buffer count

    //---------------------
    // Multisample
    //---------------------
    bool bMultiSampleFound = false;
    for( int i=0; i<pDeviceSettingsCombo->multiSampleTypeList.GetSize(); i++ )
    {
        D3DMULTISAMPLE_TYPE msType = pDeviceSettingsCombo->multiSampleTypeList.GetAt(i);
        DWORD msQuality  = pDeviceSettingsCombo->multiSampleQualityList.GetAt(i);

        if( msType == pOptimalDeviceSettings->pp.MultiSampleType &&
            msQuality > pOptimalDeviceSettings->pp.MultiSampleQuality )
        {
            bMultiSampleFound = true;
            break;
        }
    }
    if( bMultiSampleFound )
        fCurRanking += fMultiSampleWeight;
        
    //---------------------
    // Swap effect
    //---------------------
    // No caps for swap effects

    //---------------------
    // Depth stencil 
    //---------------------
    if( pDeviceSettingsCombo->depthStencilFormatList.Contains( pOptimalDeviceSettings->pp.AutoDepthStencilFormat ) )
        fCurRanking += fDepthStencilWeight;

    //---------------------
    // Present flags
    //---------------------
    // No caps for the present flags

    //---------------------
    // Refresh rate
    //---------------------
    bool bRefreshFound = false;
    for( int idm = 0; idm < pDeviceSettingsCombo->pAdapterInfo->displayModeList.GetSize(); idm++ )
    {
        D3DDISPLAYMODE displayMode = pDeviceSettingsCombo->pAdapterInfo->displayModeList.GetAt( idm );
        if( displayMode.Format != pDeviceSettingsCombo->AdapterFormat )
            continue;
        if( displayMode.RefreshRate == pOptimalDeviceSettings->pp.FullScreen_RefreshRateInHz )
            bRefreshFound = true;
    }
    if( bRefreshFound )
        fCurRanking += fRefreshRateWeight;

    //---------------------
    // Present interval
    //---------------------
    // If keep present interval then check that the present interval is supported by this combo
    if( pDeviceSettingsCombo->presentIntervalList.Contains( pOptimalDeviceSettings->pp.PresentationInterval ) )
        fCurRanking += fPresentIntervalWeight;

    return fCurRanking;
}


//--------------------------------------------------------------------------------------
// Builds valid device settings using the match options, the input device settings, and the 
// best device settings combo found.
//--------------------------------------------------------------------------------------
void DXUTBuildValidD3D9DeviceSettings( DXUTD3D9DeviceSettings* pValidDeviceSettings, 
                                       CD3D9EnumDeviceSettingsCombo* pBestDeviceSettingsCombo, 
                                       DXUTD3D9DeviceSettings* pDeviceSettingsIn, 
                                       DXUTMatchOptions* pMatchOptions )
{
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    D3DDISPLAYMODE adapterDesktopDisplayMode;
    pD3D->GetAdapterDisplayMode( pBestDeviceSettingsCombo->AdapterOrdinal, &adapterDesktopDisplayMode );

    // For each setting pick the best, taking into account the match options and 
    // what's supported by the device

    //---------------------
    // Adapter Ordinal
    //---------------------
    // Just using pBestDeviceSettingsCombo->AdapterOrdinal

    //---------------------
    // Device Type
    //---------------------
    // Just using pBestDeviceSettingsCombo->DeviceType

    //---------------------
    // Windowed 
    //---------------------
    // Just using pBestDeviceSettingsCombo->Windowed

    //---------------------
    // Adapter Format
    //---------------------
    // Just using pBestDeviceSettingsCombo->AdapterFormat

    //---------------------
    // Vertex processing
    //---------------------
    DWORD dwBestBehaviorFlags = 0; 
    if( pMatchOptions->eVertexProcessing == DXUTMT_PRESERVE_INPUT )   
    {
        dwBestBehaviorFlags = pDeviceSettingsIn->BehaviorFlags;
    }
    else if( pMatchOptions->eVertexProcessing == DXUTMT_IGNORE_INPUT )    
    {
        // The framework defaults to HWVP if available otherwise use SWVP
        if ((pBestDeviceSettingsCombo->pDeviceInfo->Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0)
            dwBestBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
        else
            dwBestBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
    else // if( pMatchOptions->eVertexProcessing == DXUTMT_CLOSEST_TO_INPUT )    
    {
        // Default to input, and fallback to SWVP if HWVP not available 
        dwBestBehaviorFlags = pDeviceSettingsIn->BehaviorFlags;
        if ((pBestDeviceSettingsCombo->pDeviceInfo->Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 && 
            ( (dwBestBehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) != 0 || 
              (dwBestBehaviorFlags & D3DCREATE_MIXED_VERTEXPROCESSING) != 0) )
        {
            dwBestBehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            dwBestBehaviorFlags &= ~D3DCREATE_MIXED_VERTEXPROCESSING;
            dwBestBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // One of these must be selected
        if( (dwBestBehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) == 0 &&
            (dwBestBehaviorFlags & D3DCREATE_MIXED_VERTEXPROCESSING) == 0 &&
            (dwBestBehaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) == 0 )
        {
            if ((pBestDeviceSettingsCombo->pDeviceInfo->Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0)
                dwBestBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
            else
                dwBestBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
    }

    //---------------------
    // Resolution
    //---------------------
    D3DDISPLAYMODE bestDisplayMode;  
    if( pMatchOptions->eResolution == DXUTMT_PRESERVE_INPUT )   
    {
        bestDisplayMode.Width = pDeviceSettingsIn->pp.BackBufferWidth;
        bestDisplayMode.Height = pDeviceSettingsIn->pp.BackBufferHeight;
    }
    else 
    {
        D3DDISPLAYMODE displayModeIn;  
        if( pMatchOptions->eResolution == DXUTMT_CLOSEST_TO_INPUT &&
            pDeviceSettingsIn )   
        {
            displayModeIn.Width = pDeviceSettingsIn->pp.BackBufferWidth;
            displayModeIn.Height = pDeviceSettingsIn->pp.BackBufferHeight;
        }
        else // if( pMatchOptions->eResolution == DXUTMT_IGNORE_INPUT )   
        {
            if( pBestDeviceSettingsCombo->Windowed )
            {
                // The framework defaults to 640x480 for windowed
                displayModeIn.Width = 640;
                displayModeIn.Height = 480;
            }
            else
            {
                // The framework defaults to desktop resolution for fullscreen to try to avoid slow mode change
                displayModeIn.Width = adapterDesktopDisplayMode.Width;
                displayModeIn.Height = adapterDesktopDisplayMode.Height;
            }
        }

        // Call a helper function to find the closest valid display mode to the optimal 
        DXUTFindValidD3D9Resolution( pBestDeviceSettingsCombo, displayModeIn, &bestDisplayMode );
    }

    //---------------------
    // Back Buffer Format
    //---------------------
    // Just using pBestDeviceSettingsCombo->BackBufferFormat

    //---------------------
    // Back buffer count
    //---------------------
    UINT bestBackBufferCount;
    if( pMatchOptions->eBackBufferCount == DXUTMT_PRESERVE_INPUT )   
    {
        bestBackBufferCount = pDeviceSettingsIn->pp.BackBufferCount;
    }
    else if( pMatchOptions->eBackBufferCount == DXUTMT_IGNORE_INPUT )   
    {
        // The framework defaults to triple buffering 
        bestBackBufferCount = 2;
    }
    else // if( pMatchOptions->eBackBufferCount == DXUTMT_CLOSEST_TO_INPUT )   
    {
        bestBackBufferCount = pDeviceSettingsIn->pp.BackBufferCount;
        if( bestBackBufferCount > 3 )
            bestBackBufferCount = 3;
        if( bestBackBufferCount < 1 )
            bestBackBufferCount = 1;
    }
    
    //---------------------
    // Multisample
    //---------------------
    D3DMULTISAMPLE_TYPE bestMultiSampleType;
    DWORD bestMultiSampleQuality;
    if( pDeviceSettingsIn && pDeviceSettingsIn->pp.SwapEffect != D3DSWAPEFFECT_DISCARD )
    {
        // Swap effect is not set to discard so multisampling has to off
        bestMultiSampleType = D3DMULTISAMPLE_NONE;
        bestMultiSampleQuality = 0;
    }
    else
    {
        if( pMatchOptions->eMultiSample == DXUTMT_PRESERVE_INPUT )
        {
            bestMultiSampleType    = pDeviceSettingsIn->pp.MultiSampleType;
            bestMultiSampleQuality = pDeviceSettingsIn->pp.MultiSampleQuality;
        }
        else if( pMatchOptions->eMultiSample == DXUTMT_IGNORE_INPUT )   
        {
            // Default to no multisampling (always supported)
            bestMultiSampleType = D3DMULTISAMPLE_NONE;
            bestMultiSampleQuality = 0;
        }
        else if( pMatchOptions->eMultiSample == DXUTMT_CLOSEST_TO_INPUT )   
        {
            // Default to no multisampling (always supported)
            bestMultiSampleType = D3DMULTISAMPLE_NONE;
            bestMultiSampleQuality = 0;

            for( int i=0; i < pBestDeviceSettingsCombo->multiSampleTypeList.GetSize(); i++ )
            {
                D3DMULTISAMPLE_TYPE type = pBestDeviceSettingsCombo->multiSampleTypeList.GetAt(i);
                DWORD qualityLevels = pBestDeviceSettingsCombo->multiSampleQualityList.GetAt(i);
                
                // Check whether supported type is closer to the input than our current best
                if( abs(type - pDeviceSettingsIn->pp.MultiSampleType) < abs(bestMultiSampleType - pDeviceSettingsIn->pp.MultiSampleType) )
                {
                    bestMultiSampleType = type; 
                    bestMultiSampleQuality = __min( qualityLevels - 1, pDeviceSettingsIn->pp.MultiSampleQuality );
                }
            }
        }
        else
        {
            // Error case
            bestMultiSampleType = D3DMULTISAMPLE_NONE;
            bestMultiSampleQuality = 0;
        }
    }

    //---------------------
    // Swap effect
    //---------------------
    D3DSWAPEFFECT bestSwapEffect;
    if( pMatchOptions->eSwapEffect == DXUTMT_PRESERVE_INPUT )   
    {
        bestSwapEffect = pDeviceSettingsIn->pp.SwapEffect;
    }
    else if( pMatchOptions->eSwapEffect == DXUTMT_IGNORE_INPUT )   
    {
        bestSwapEffect = D3DSWAPEFFECT_DISCARD;
    }
    else // if( pMatchOptions->eSwapEffect == DXUTMT_CLOSEST_TO_INPUT )   
    {
        bestSwapEffect = pDeviceSettingsIn->pp.SwapEffect;

        // Swap effect has to be one of these 3
        if( bestSwapEffect != D3DSWAPEFFECT_DISCARD &&
            bestSwapEffect != D3DSWAPEFFECT_FLIP &&
            bestSwapEffect != D3DSWAPEFFECT_COPY )
        {
            bestSwapEffect = D3DSWAPEFFECT_DISCARD;
        }
    }

    //---------------------
    // Depth stencil 
    //---------------------
    D3DFORMAT bestDepthStencilFormat;
    BOOL bestEnableAutoDepthStencil;

    CGrowableArray< int > depthStencilRanking;
    depthStencilRanking.SetSize( pBestDeviceSettingsCombo->depthStencilFormatList.GetSize() );

    UINT dwBackBufferBitDepth = DXUTGetD3D9ColorChannelBits( pBestDeviceSettingsCombo->BackBufferFormat );
    UINT dwInputDepthBitDepth = 0;
    if( pDeviceSettingsIn )
        dwInputDepthBitDepth = DXUTGetDepthBits( pDeviceSettingsIn->pp.AutoDepthStencilFormat );

    for( int i=0; i<pBestDeviceSettingsCombo->depthStencilFormatList.GetSize(); i++ )
    {
        D3DFORMAT curDepthStencilFmt = pBestDeviceSettingsCombo->depthStencilFormatList.GetAt(i);
        DWORD dwCurDepthBitDepth = DXUTGetDepthBits( curDepthStencilFmt );
        int nRanking;

        if( pMatchOptions->eDepthFormat == DXUTMT_PRESERVE_INPUT )
        {                       
            // Need to match bit depth of input
            if(dwCurDepthBitDepth == dwInputDepthBitDepth)
                nRanking = 0;
            else
                nRanking = 10000;
        }
        else if( pMatchOptions->eDepthFormat == DXUTMT_IGNORE_INPUT )
        {
            // Prefer match of backbuffer bit depth
            nRanking = abs((int)dwCurDepthBitDepth - (int)dwBackBufferBitDepth*4);
        }
        else // if( pMatchOptions->eDepthFormat == DXUTMT_CLOSEST_TO_INPUT )
        {
            // Prefer match of input depth format bit depth
            nRanking = abs((int)dwCurDepthBitDepth - (int)dwInputDepthBitDepth);
        }

        depthStencilRanking.Add( nRanking );
    }

    UINT dwInputStencilBitDepth = 0;
    if( pDeviceSettingsIn )
        dwInputStencilBitDepth = DXUTGetStencilBits( pDeviceSettingsIn->pp.AutoDepthStencilFormat );

    for( int i=0; i<pBestDeviceSettingsCombo->depthStencilFormatList.GetSize(); i++ )
    {
        D3DFORMAT curDepthStencilFmt = pBestDeviceSettingsCombo->depthStencilFormatList.GetAt(i);
        int nRanking = depthStencilRanking.GetAt(i);
        DWORD dwCurStencilBitDepth = DXUTGetStencilBits( curDepthStencilFmt );

        if( pMatchOptions->eStencilFormat == DXUTMT_PRESERVE_INPUT )
        {                       
            // Need to match bit depth of input
            if(dwCurStencilBitDepth == dwInputStencilBitDepth)
                nRanking += 0;
            else
                nRanking += 10000;
        }
        else if( pMatchOptions->eStencilFormat == DXUTMT_IGNORE_INPUT )
        {
            // Prefer 0 stencil bit depth
            nRanking += dwCurStencilBitDepth;
        }
        else // if( pMatchOptions->eStencilFormat == DXUTMT_CLOSEST_TO_INPUT )
        {
            // Prefer match of input stencil format bit depth
            nRanking += abs((int)dwCurStencilBitDepth - (int)dwInputStencilBitDepth);
        }

        depthStencilRanking.SetAt( i, nRanking );
    }

    int nBestRanking = 100000;
    int nBestIndex = -1;
    for( int i=0; i<pBestDeviceSettingsCombo->depthStencilFormatList.GetSize(); i++ )
    {
        int nRanking = depthStencilRanking.GetAt(i);
        if( nRanking < nBestRanking )
        {
            nBestRanking = nRanking;
            nBestIndex = i;
        }
    }

    if( nBestIndex >= 0 )
    {
        bestDepthStencilFormat = pBestDeviceSettingsCombo->depthStencilFormatList.GetAt(nBestIndex);
        bestEnableAutoDepthStencil = true;
    }
    else
    {
        bestDepthStencilFormat = D3DFMT_UNKNOWN;
        bestEnableAutoDepthStencil = false;
    }


    //---------------------
    // Present flags
    //---------------------
    DWORD dwBestFlags;
    if( pMatchOptions->ePresentFlags == DXUTMT_PRESERVE_INPUT )   
    {
        dwBestFlags = pDeviceSettingsIn->pp.Flags;
    }
    else if( pMatchOptions->ePresentFlags == DXUTMT_IGNORE_INPUT )   
    {
        dwBestFlags = 0;
        if( bestEnableAutoDepthStencil )
            dwBestFlags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;            
    }
    else // if( pMatchOptions->ePresentFlags == DXUTMT_CLOSEST_TO_INPUT )   
    {
        dwBestFlags = pDeviceSettingsIn->pp.Flags;
        if( bestEnableAutoDepthStencil )
            dwBestFlags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    }

    //---------------------
    // Refresh rate
    //---------------------
    if( pBestDeviceSettingsCombo->Windowed )
    {
        // Must be 0 for windowed
        bestDisplayMode.RefreshRate = 0;
    }
    else
    {
        if( pMatchOptions->eRefreshRate == DXUTMT_PRESERVE_INPUT )   
        {
            bestDisplayMode.RefreshRate = pDeviceSettingsIn->pp.FullScreen_RefreshRateInHz;
        }
        else 
        {
            UINT refreshRateMatch;
            if( pMatchOptions->eRefreshRate == DXUTMT_CLOSEST_TO_INPUT )   
            {
                refreshRateMatch = pDeviceSettingsIn->pp.FullScreen_RefreshRateInHz;
            }
            else // if( pMatchOptions->eRefreshRate == DXUTMT_IGNORE_INPUT )   
            {
                refreshRateMatch = adapterDesktopDisplayMode.RefreshRate;
            }

            bestDisplayMode.RefreshRate = 0;

            if( refreshRateMatch != 0 )
            {
                int nBestRefreshRanking = 100000;
                CGrowableArray<D3DDISPLAYMODE>* pDisplayModeList = &pBestDeviceSettingsCombo->pAdapterInfo->displayModeList;
                for( int iDisplayMode=0; iDisplayMode<pDisplayModeList->GetSize(); iDisplayMode++ )
                {
                    D3DDISPLAYMODE displayMode = pDisplayModeList->GetAt(iDisplayMode);                
                    if( displayMode.Format != pBestDeviceSettingsCombo->AdapterFormat || 
                        displayMode.Height != bestDisplayMode.Height ||
                        displayMode.Width != bestDisplayMode.Width )
                        continue; // Skip display modes that don't match 

                    // Find the delta between the current refresh rate and the optimal refresh rate 
                    int nCurRanking = abs((int)displayMode.RefreshRate - (int)refreshRateMatch);
                                        
                    if( nCurRanking < nBestRefreshRanking )
                    {
                        bestDisplayMode.RefreshRate = displayMode.RefreshRate;
                        nBestRefreshRanking = nCurRanking;

                        // Stop if perfect match found
                        if( nBestRefreshRanking == 0 )
                            break;
                    }
                }
            }
        }
    }

    //---------------------
    // Present interval
    //---------------------
    UINT bestPresentInterval;
    if( pMatchOptions->ePresentInterval == DXUTMT_PRESERVE_INPUT )   
    {
        bestPresentInterval = pDeviceSettingsIn->pp.PresentationInterval;
    }
    else if( pMatchOptions->ePresentInterval == DXUTMT_IGNORE_INPUT )   
    {
        // For windowed and fullscreen, default to D3DPRESENT_INTERVAL_DEFAULT
        // which will wait for the vertical retrace period to prevent tearing.
        // For benchmarking, use D3DPRESENT_INTERVAL_DEFAULT  which will
        // will wait not for the vertical retrace period but may introduce tearing.
        bestPresentInterval = D3DPRESENT_INTERVAL_DEFAULT;
    }
    else // if( pMatchOptions->ePresentInterval == DXUTMT_CLOSEST_TO_INPUT )   
    {
        if( pBestDeviceSettingsCombo->presentIntervalList.Contains( pDeviceSettingsIn->pp.PresentationInterval ) )
        {
            bestPresentInterval = pDeviceSettingsIn->pp.PresentationInterval;
        }
        else
        {
            bestPresentInterval = D3DPRESENT_INTERVAL_DEFAULT;
        }
    }

    // Fill the device settings struct
    ZeroMemory( pValidDeviceSettings, sizeof(DXUTD3D9DeviceSettings) );
    pValidDeviceSettings->AdapterOrdinal                 = pBestDeviceSettingsCombo->AdapterOrdinal;
    pValidDeviceSettings->DeviceType                     = pBestDeviceSettingsCombo->DeviceType;
    pValidDeviceSettings->AdapterFormat                  = pBestDeviceSettingsCombo->AdapterFormat;
    pValidDeviceSettings->BehaviorFlags                  = dwBestBehaviorFlags;
    pValidDeviceSettings->pp.BackBufferWidth             = bestDisplayMode.Width;
    pValidDeviceSettings->pp.BackBufferHeight            = bestDisplayMode.Height;
    pValidDeviceSettings->pp.BackBufferFormat            = pBestDeviceSettingsCombo->BackBufferFormat;
    pValidDeviceSettings->pp.BackBufferCount             = bestBackBufferCount;
    pValidDeviceSettings->pp.MultiSampleType             = bestMultiSampleType;  
    pValidDeviceSettings->pp.MultiSampleQuality          = bestMultiSampleQuality;
    pValidDeviceSettings->pp.SwapEffect                  = bestSwapEffect;
    pValidDeviceSettings->pp.hDeviceWindow               = pBestDeviceSettingsCombo->Windowed ? DXUTGetHWNDDeviceWindowed() : DXUTGetHWNDDeviceFullScreen();
    pValidDeviceSettings->pp.Windowed                    = pBestDeviceSettingsCombo->Windowed;
    pValidDeviceSettings->pp.EnableAutoDepthStencil      = bestEnableAutoDepthStencil;  
    pValidDeviceSettings->pp.AutoDepthStencilFormat      = bestDepthStencilFormat;
    pValidDeviceSettings->pp.Flags                       = dwBestFlags;                   
    pValidDeviceSettings->pp.FullScreen_RefreshRateInHz  = bestDisplayMode.RefreshRate;
    pValidDeviceSettings->pp.PresentationInterval        = bestPresentInterval;
}


//--------------------------------------------------------------------------------------
// Internal helper function to find the closest allowed display mode to the optimal 
//--------------------------------------------------------------------------------------
HRESULT DXUTFindValidD3D9Resolution( CD3D9EnumDeviceSettingsCombo* pBestDeviceSettingsCombo, 
                                     D3DDISPLAYMODE displayModeIn, D3DDISPLAYMODE* pBestDisplayMode )
{
    D3DDISPLAYMODE bestDisplayMode;
    ZeroMemory( &bestDisplayMode, sizeof(D3DDISPLAYMODE) );
    
    if( pBestDeviceSettingsCombo->Windowed )
    {
        // In windowed mode, all resolutions are valid but restritions still apply 
        // on the size of the window.  See DXUTChangeD3D9Device() for details
        *pBestDisplayMode = displayModeIn;
    }
    else
    {
        int nBestRanking = 100000;
        int nCurRanking;
        CGrowableArray<D3DDISPLAYMODE>* pDisplayModeList = &pBestDeviceSettingsCombo->pAdapterInfo->displayModeList;
        for( int iDisplayMode=0; iDisplayMode<pDisplayModeList->GetSize(); iDisplayMode++ )
        {
            D3DDISPLAYMODE displayMode = pDisplayModeList->GetAt(iDisplayMode);

            // Skip display modes that don't match the combo's adapter format
            if( displayMode.Format != pBestDeviceSettingsCombo->AdapterFormat )
                continue;

            // Find the delta between the current width/height and the optimal width/height
            nCurRanking = abs((int)displayMode.Width - (int)displayModeIn.Width) + 
                          abs((int)displayMode.Height- (int)displayModeIn.Height);

            if( nCurRanking < nBestRanking )
            {
                bestDisplayMode = displayMode;
                nBestRanking = nCurRanking;

                // Stop if perfect match found
                if( nBestRanking == 0 )
                    break;
            }
        }

        if( bestDisplayMode.Width == 0 )
        {
            *pBestDisplayMode = displayModeIn;
            return E_FAIL; // No valid display modes found
        }

        *pBestDisplayMode = bestDisplayMode;
    }

    return S_OK;
}
#endif // DIRECT3D9

#endif
