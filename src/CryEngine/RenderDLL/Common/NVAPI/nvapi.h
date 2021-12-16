 /***************************************************************************\
|*                                                                           *|
|*      Copyright 2005-2006 NVIDIA Corporation.  All rights reserved.        *|
|*                                                                           *|
|*   NOTICE TO USER:                                                         *|
|*                                                                           *|
|*   This source code is subject to NVIDIA ownership rights under U.S.       *|
|*   and international Copyright laws.                                       *|
|*                                                                           *|
|*   This software and the information contained herein is PROPRIETARY       *|
|*   and CONFIDENTIAL to NVIDIA and is being provided under the terms        *|
|*   and conditions of a Non-Disclosure Agreement. Any reproduction or       *|
|*   disclosure to any third party without the express written consent of    *|
|*   NVIDIA is prohibited.                                                   *|
|*                                                                           *|
|*   NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE     *|
|*   CODE FOR ANY PURPOSE. IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR         *|
|*   IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH     *|
|*   REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF         *|
|*   MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR          *|
|*   PURPOSE. IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL,            *|
|*   INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES          *|
|*   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN      *|
|*   AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING     *|
|*   OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE      *|
|*   CODE.                                                                   *|
|*                                                                           *|
|*   U.S. Government End Users. This source code is a "commercial item"      *|
|*   as that term is defined at 48 C.F.R. 2.101 (OCT 1995), consisting       *| 
|*   of "commercial computer software" and "commercial computer software     *|
|*   documentation" as such terms are used in 48 C.F.R. 12.212 (SEPT 1995)   *|
|*   and is provided to the U.S. Government only as a commercial end item.   *|
|*   Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through        *|
|*   227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the       *|
|*   source code with only those rights set forth herein.                    *|
|*                                                                           *|
|*                                                                           *|
 \***************************************************************************/
///////////////////////////////////////////////////////////////////////////////
//
//$Date$
// File: nvapi.h
//
// NvAPI provides an interface to NVIDIA devices. This file contains the 
// interface constants, structure definitions and function prototypes.
//
// Target Profile: NDA-developer
//      Target OS: winxp_x86
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _NVAPI_H
#define _NVAPI_H

#pragma pack(push,8) // Make sure we have consistent structure packings

#ifdef __cplusplus
extern "C" {
#endif

// ====================================================
// Universal NvAPI Definitions
// ====================================================
#ifdef _WIN32
#define NVAPI_INTERFACE extern NvAPI_Status __cdecl 
#else
#define NVAPI_INTERFACE extern NvAPI_Status __cdecl 
#endif

typedef unsigned __int64 NvU64;
typedef unsigned long    NvU32;
typedef long             NvS32;
typedef unsigned char    NvU8;

#ifndef DECLARE_HANDLE
#define DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#endif

// NVAPI Handles - These handles are retrieved from various calls and passed in to others in NvAPI
//                 These are meant to be opaque types.  Do not assume they correspond to indices, HDCs,
//                 display indexes or anything else.
//
//                 Most handles remain valid until a display re-configuration (display mode set) or GPU
//                 reconfiguration (going into or out of SLI modes) occurs.  If NVAPI_HANDLE_INVALIDATED
//                 is received by an app, it should discard all handles, and re-enumerate them.
//
DECLARE_HANDLE(NvDisplayHandle);            // Display Device driven by NVidia GPU(s) (an attached display)
DECLARE_HANDLE(NvUnAttachedDisplayHandle);  // Unattached Display Device driven by NVidia GPU(s)
DECLARE_HANDLE(NvLogicalGpuHandle);         // One or more physical GPUs acting in concert (SLI)
DECLARE_HANDLE(NvPhysicalGpuHandle);        // A single physical GPU

#define NVAPI_DEFAULT_HANDLE        0

#define NVAPI_GENERIC_STRING_MAX    4096
#define NVAPI_LONG_STRING_MAX       256
#define NVAPI_SHORT_STRING_MAX      64

typedef struct 
{
    NvS32   sX;
    NvS32   sY;
    NvS32   sWidth;
    NvS32   sHeight;
} NvSBox;

#define NVAPI_MAX_PHYSICAL_GPUS             64
#define NVAPI_MAX_LOGICAL_GPUS              64
#define NVAPI_MAX_AVAILABLE_GPU_TOPOLOGIES  256
#define NVAPI_MAX_GPU_TOPOLOGIES            NVAPI_MAX_PHYSICAL_GPUS
#define NVAPI_MAX_GPU_PER_TOPOLOGY          8
#define NVAPI_MAX_DISPLAY_HEADS             2
#define NVAPI_MAX_DISPLAYS                  NVAPI_MAX_PHYSICAL_GPUS * NVAPI_MAX_DISPLAY_HEADS

typedef char NvAPI_String[NVAPI_GENERIC_STRING_MAX];
typedef char NvAPI_LongString[NVAPI_LONG_STRING_MAX];
typedef char NvAPI_ShortString[NVAPI_SHORT_STRING_MAX];

// =========================================================================================
// NvAPI Version Definition
// Maintain per structure specific version define using the MAKE_NVAPI_VERSION macro.
// Usage: #define NV_GENLOCK_STATUS_VER  MAKE_NVAPI_VERSION(NV_GENLOCK_STATUS, 1)
// =========================================================================================
#define MAKE_NVAPI_VERSION(typeName,ver) (NvU32)(sizeof(typeName) | ((ver)<<16))

// ====================================================
// NvAPI Status Values
//    All NvAPI functions return one of these codes.
// ====================================================
typedef enum 
{
    NVAPI_OK                                    =  0,      // Success
    NVAPI_ERROR                                 = -1,      // Generic error
    NVAPI_LIBRARY_NOT_FOUND                     = -2,      // nvapi.dll can not be loaded
    NVAPI_NO_IMPLEMENTATION                     = -3,      // not implemented in current driver installation
    NVAPI_API_NOT_INTIALIZED                    = -4,      // NvAPI_Initialize has not been called (successfully)
    NVAPI_INVALID_ARGUMENT                      = -5,      // invalid argument
    NVAPI_NVIDIA_DEVICE_NOT_FOUND               = -6,      // no NVIDIA display driver was found
    NVAPI_END_ENUMERATION                       = -7,      // no more to enum
    NVAPI_INVALID_HANDLE                        = -8,      // invalid handle
    NVAPI_INCOMPATIBLE_STRUCT_VERSION           = -9,      // an argument's structure version is not supported
    NVAPI_HANDLE_INVALIDATED                    = -10,     // handle is no longer valid (likely due to GPU or display re-configuration)
    NVAPI_OPENGL_CONTEXT_NOT_CURRENT            = -11,     // no NVIDIA OpenGL context is current (but needs to be)
    NVAPI_EXPECTED_LOGICAL_GPU_HANDLE           = -100,    // expected a logical GPU handle for one or more parameters
    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE          = -101,    // expected a phyiscal GPU handle for one or more parameters
    NVAPI_EXPECTED_DISPLAY_HANDLE               = -102,    // expected an NV display handle for one or more parameters
    NVAPI_INVALID_COMBINATION                   = -103,    // used in some commands to indicate that the combination of parameters is not validexpected an NV display handle for one or more parameters
    NVAPI_NOT_SUPPORTED                         = -104,    // Requested feature not supported in the selected GPU
    NVAPI_PORTID_NOT_FOUND                      = -105,    // NO port ID found for I2C transaction
    NVAPI_EXPECTED_UNATTACHED_DISPLAY_HANDLE    = -106,    // expected an unattached display handle as one of the input param
    NVAPI_INVALID_PERF_LEVEL                    = -107,    // invalid perf level 
    NVAPI_DEVICE_BUSY                           = -108,    // device is busy, request not fulfilled
    NVAPI_NV_PERSIST_FILE_NOT_FOUND             = -109,    // NV persist file is not found
    NVAPI_PERSIST_DATA_NOT_FOUND                = -110,    // NV persist data is not found
    NVAPI_EXPECTED_TV_DISPLAY                   = -111,    // expected TV output display
    NVAPI_EXPECTED_TV_DISPLAY_ON_DCONNECTOR     = -112,    // expected TV output on D Connector - HDTV_EIAJ4120.

} NvAPI_Status;


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Initialize
//
//   DESCRIPTION: Initializes NVAPI library. This must be called before any 
//                other NvAPI_ function.
//
// RETURN STATUS: NVAPI_ERROR            Something is wrong during the initialization process (generic error)
//                NVAPI_LIBRARYNOTFOUND  Can not load nvapi.dll
//                NVAPI_OK                  Initialized
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Initialize();

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetErrorMessage
//
//   DESCRIPTION: converts an NvAPI error code into a null terminated string
//
// RETURN STATUS: null terminated string (always, never NULL)
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetErrorMessage(NvAPI_Status nr,NvAPI_ShortString szDesc);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetInterfaceVersionString
//
//   DESCRIPTION: Returns a string describing the version of the NvAPI library.
//                Contents of the string are human readable.  Do not assume a fixed
//                format.
//
// RETURN STATUS: User readable string giving info on NvAPI's version
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetInterfaceVersionString(NvAPI_ShortString szDesc);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetDisplayDriverVersion
//
//   DESCRIPTION: Returns a struct that describes aspects of the display driver
//                build.
//
// RETURN STATUS: NVAPI_ERROR or NVAPI_OK
//
///////////////////////////////////////////////////////////////////////////////
typedef struct 
{
    NvU32              version;             // Structure version
    NvU32              drvVersion;           
    NvU32              bldChangeListNum;     
    NvAPI_ShortString  szBuildBranchString; 
    NvAPI_ShortString  szAdapterString;
} NV_DISPLAY_DRIVER_VERSION;
#define NV_DISPLAY_DRIVER_VERSION_VER  MAKE_NVAPI_VERSION(NV_DISPLAY_DRIVER_VERSION,1)
NVAPI_INTERFACE NvAPI_GetDisplayDriverVersion(NvDisplayHandle hNvDisplay, NV_DISPLAY_DRIVER_VERSION *pVersion);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnumNvidiaDisplayHandle
//
//   DESCRIPTION: Returns the handle of the NVidia display specified by the enum 
//                index (thisEnum). The client should keep enumerating until it
//                returns NVAPI_END_ENUMERATION.
//
//                Note: Display handles can get invalidated on a modeset, so the calling applications need to 
//                renum the handles after every modeset.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: either the handle pointer is NULL or enum index too big
//                NVAPI_OK: return a valid NvDisplayHandle based on the enum index
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia device found in the system
//                NVAPI_END_ENUMERATION: no more display device to enumerate.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnumNvidiaDisplayHandle(NvU32 thisEnum, NvDisplayHandle *pNvDispHandle);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnumNvidiaUnAttachedDisplayHandle
//
//   DESCRIPTION: Returns the handle of the NVidia UnAttached display specified by the enum 
//                index (thisEnum). The client should keep enumerating till it
//                return error.
//
//                Note: Display handles can get invalidated on a modeset, so the calling applications need to 
//                renum the handles after every modeset.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: either the handle pointer is NULL or enum index too big
//                NVAPI_OK: return a valid NvDisplayHandle based on the enum index
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia device found in the system
//                NVAPI_END_ENUMERATION: no more display device to enumerate.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnumNvidiaUnAttachedDisplayHandle(NvU32 thisEnum, NvUnAttachedDisplayHandle *pNvUnAttachedDispHandle);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnumPhysicalGPUs
//
//   DESCRIPTION: Returns an array of physical GPU handles.
//
//                Each handle represents a physical GPU present in the system.
//                That GPU may be part of a SLI configuration, or not be visible to the OS directly.
//
//                At least 1 GPU must be present in the system and running an NV display driver.
//
//                The array nvGPUHandle will be filled with physical GPU handle values.  The returned
//                gpuCount determines how many entries in the array are valid.
//
//                Note: All GPU handles get invalidated on a modeset, so the calling applications need to 
//                renum the handles after every modeset.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: nvGPUHandle or pGpuCount is NULL
//                NVAPI_OK: one or more handles were returned
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnumPhysicalGPUs(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnumLogicalGPUs
//
//   DESCRIPTION: Returns an array of logical GPU handles.
//
//                Each handle represents one or more GPUs acting in concert as a single graphics device.
//
//                At least 1 GPU must be present in the system and running an NV display driver.
//
//                The array nvGPUHandle will be filled with logical GPU handle values.  The returned
//                gpuCount determines how many entries in the array are valid.
//
//                Note: All GPU handles get invalidated on a modeset, so the calling applications need to 
//                renum the handles after every modeset.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: nvGPUHandle or pGpuCount is NULL
//                NVAPI_OK: one or more handles were returned
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnumLogicalGPUs(NvLogicalGpuHandle nvGPUHandle[NVAPI_MAX_LOGICAL_GPUS], NvU32 *pGpuCount);
 
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetPhysicalGPUsFromDisplay
//
//   DESCRIPTION: Returns an array of physical GPU handles associated with the specified display.
//
//                At least 1 GPU must be present in the system and running an NV display driver.
//
//                The array nvGPUHandle will be filled with physical GPU handle values.  The returned
//                gpuCount determines how many entries in the array are valid.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hNvDisp is not valid; nvGPUHandle or pGpuCount is NULL
//                NVAPI_OK: one or more handles were returned
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetPhysicalGPUsFromDisplay(NvDisplayHandle hNvDisp, NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
 
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetPhysicalGPUFromUnAttachedDisplay
//
//   DESCRIPTION: Returns a physical GPU handle associated with the specified unattached display.
//
//                At least 1 GPU must be present in the system and running an NV display driver.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hNvUnAttachedDisp is not valid or pPhysicalGpu is NULL.
//                NVAPI_OK: one or more handles were returned
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetPhysicalGPUFromUnAttachedDisplay(NvUnAttachedDisplayHandle hNvUnAttachedDisp, NvPhysicalGpuHandle *pPhysicalGpu);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_CreateDisplayFromUnAttachedDisplay
//
//   DESCRIPTION: The unattached display handle is converted to a active attached display handle.
//
//                At least 1 GPU must be present in the system and running an NV display driver.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hNvUnAttachedDisp is not valid or pNvDisplay is NULL.
//                NVAPI_OK: one or more handles were returned
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_CreateDisplayFromUnAttachedDisplay(NvUnAttachedDisplayHandle hNvUnAttachedDisp, NvDisplayHandle *pNvDisplay);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetLogicalGPUFromDisplay
//
//   DESCRIPTION: Returns the logical GPU handle associated with the specified display.
//
//                At least 1 GPU must be present in the system and running an NV display driver.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hNvDisp is not valid; pLogicalGPU is NULL
//                NVAPI_OK: one or more handles were returned
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetLogicalGPUFromDisplay(NvDisplayHandle hNvDisp, NvLogicalGpuHandle *pLogicalGPU);
 
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetLogicalGPUFromPhysicalGPU
//
//   DESCRIPTION: Returns the logical GPU handle associated with specified physical GPU handle.
//
//                At least 1 GPU must be present in the system and running an NV display driver.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGPU is not valid; pLogicalGPU is NULL
//                NVAPI_OK: one or more handles were returned
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetLogicalGPUFromPhysicalGPU(NvPhysicalGpuHandle hPhysicalGPU, NvLogicalGpuHandle *pLogicalGPU);
   
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetPhysicalGPUsFromLogicalGPU
//
//   DESCRIPTION: Returns the physical GPU handles associated with the specified logical GPU handle.
//
//                At least 1 GPU must be present in the system and running an NV display driver.
//
//                The array hPhysicalGPU will be filled with physical GPU handle values.  The returned
//                gpuCount determines how many entries in the array are valid.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hLogicalGPU is not valid; hPhysicalGPU is NULL
//                NVAPI_OK: one or more handles were returned
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_LOGICAL_GPU_HANDLE: hLogicalGPU was not a logical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetPhysicalGPUsFromLogicalGPU(NvLogicalGpuHandle hLogicalGPU,NvPhysicalGpuHandle hPhysicalGPU[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
   
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetAssociatedNvidiaDisplayHandle
//
//   DESCRIPTION: Returns the handle of the NVidia display that is associated
//                with the display name given.  Eg: "\\DISPLAY1"
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: either argument is NULL
//                NVAPI_OK: *pNvDispHandle is now valid
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia device maps to that display name
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetAssociatedNvidiaDisplayHandle(const char *szDisplayName, NvDisplayHandle *pNvDispHandle);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetAssociatedNvidiaDisplayName
//
//   DESCRIPTION: Returns the display name given.  Eg: "\\DISPLAY1" using the NVidia display handle
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: either argument is NULL
//                NVAPI_OK: *pNvDispHandle is now valid
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia device maps to that display name
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetAssociatedNvidiaDisplayName(NvDisplayHandle NvDispHandle, NvAPI_ShortString szDisplayName);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnableHWCursor
//
//   DESCRIPTION: Enable hardware cursor support
//
// RETURN STATUS: NVAPI_ERROR or NVAPI_OK
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnableHWCursor(NvDisplayHandle hNvDisplay);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DisableHWCursor
//
//   DESCRIPTION: Disable hardware cursor support
//
// RETURN STATUS: NVAPI_ERROR or NVAPI_OK
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DisableHWCursor(NvDisplayHandle hNvDisplay);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetVBlankCounter
//
//   DESCRIPTION: get vblank counter
//
// RETURN STATUS: NVAPI_ERROR or NVAPI_OK
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetVBlankCounter(NvDisplayHandle hNvDisplay, NvU32 *pCounter);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: NvAPI_OverrideRefreshRate
//   DESCRIPTION: Override the refresh rate on the given  display/outputsMask.
//                The new refresh rate can be applied right away in this API call or deferred to happen with the
//                next OS modeset. The override is only good for one modeset (doesn't matter it's deferred or immediate).
//               
//
//         INPUT: hNvDisplay - the NVidia display handle. It can be NVAPI_DEFAULT_HANDLE or a handle
//                             enumerated from NvAPI_EnumNVidiaDisplayHandle().
//                
//                outputsMask - a set of bits that identify all target outputs which are associated with the NVidia 
//                              display handle to apply the refresh rate override. Note when SLI is enabled,  the
//                              outputsMask only applies to the GPU that is driving the display output.
//
//                refreshRate - the override value. "0.0" means cancel the override.
//                              
//
//                bSetDeferred - "0": apply the refresh rate override immediately in this API call.
//                               "1":  apply refresh rate at the next OS modeset.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hNvDisplay or outputsMask is invalid
//                NVAPI_OK: the refresh rate override is correct set
//                NVAPI_ERROR: the operation failed
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_SetRefreshRateOverride(NvDisplayHandle hNvDisplay, NvU32 outputsMask, float refreshRate, NvU32 bSetDeferred);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetAssociatedDisplayOutputId
//
//   DESCRIPTION: Gets the active outputId associated with the display handle.
//
//    PARAMETERS: hNvDisplay(IN) - NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//                outputId(OUT)  - The active display output id associated with the selected display handle hNvDisplay.
//                                 The outputid will have only one bit set. In case of clone or span this  will indicate the display
//                                 outputId of the primay display that the GPU is driving.
// RETURN STATUS: NVAPI_OK: call successful.
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found.
//                NVAPI_EXPECTED_DISPLAY_HANDLE: hNvDisplay is not a valid display handle.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetAssociatedDisplayOutputId(NvDisplayHandle hNvDisplay, NvU32 *pOutputId);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetHDMISupportInfo
//
// DESCRIPTION:     This API returns the current infoframe data on the specified device(monitor)
//
// PARAMETERS:      hNvDisplay(IN) - NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//                  outputId(IN)   - The display output id. If it's "0" then the default outputId from NvAPI_GetAssociatedDisplayOutputId() will be used.
//                  pInfo(OUT)     - The monitor and GPU's HDMI support info
//
// RETURN STATUS: 
//                  NVAPI_OK - completed request
//                  NVAPI_ERROR - miscellaneous error occurred
//                  NVAPI_INVALID_ARGUMENT: Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////
typedef struct
{
    NvU32      version;                     // structure version
    NvU32      isGpuHDMICapable       : 1;  // if the GPU can handle HDMI
    NvU32      isMonUnderscanCapable  : 1;  // if the monitor supports underscan
    NvU32      isMonBasicAudioCapable : 1;  // if the monitor supports basic audio
    NvU32      isMonYCbCr444Capable   : 1;  // if YCbCr 4:4:4 is supported
    NvU32      isMonYCbCr422Capable   : 1;  // if YCbCr 4:2:2 is supported
    NvU32      isMonHDMI              : 1;  // if the monitor is HDMI (with IEEE's HDMI registry ID)
    NvU32      EDID861ExtRev;               // the revision number of the EDID 861 extension
 } NV_HDMI_SUPPORT_INFO; 

#define NV_HDMI_SUPPORT_INFO_VER  MAKE_NVAPI_VERSION(NV_HDMI_SUPPORT_INFO,1)

NVAPI_INTERFACE NvAPI_GetHDMISupportInfo(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_HDMI_SUPPORT_INFO *pInfo);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetInfoFrame
//
// DESCRIPTION:     This API returns the current infoframe data on the specified device(monitor)
//
// PARAMETERS:      hNvDisplay(IN) - NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//                  outputId(IN)   - The display output id. If it's "0" then the default outputId from NvAPI_GetAssociatedDisplayOutputId() will be used.
//                  type(IN)       - The type of infoframe to set
//                  pInfoFrame(IN) - The infoframe data
//
// RETURN STATUS: 
//                  NVAPI_OK - completed request
//                  NVAPI_ERROR - miscellaneous error occurred
//                  NVAPI_INVALID_ARGUMENT: Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////
typedef enum _NV_INFOFRAME_TYPE
{
    NV_INFOFRAME_TYPE_AVI   = 2,
    NV_INFOFRAME_TYPE_SPD   = 3,
    NV_INFOFRAME_TYPE_AUDIO = 4,
    NV_INFOFRAME_TYPE_MS    = 5,
} NV_INFOFRAME_TYPE;

typedef struct
{
    NvU8 type;
    NvU8 version;
    NvU8 length;
} NV_INFOFRAME_HEADER;

// since this is for Windows OS so far, we use this bit little endian defination
// to handle the translation
typedef struct
{
    // byte 1
    NvU8 channelCount     : 3;
    NvU8 rsvd_bits_byte1  : 1;
    NvU8 codingType       : 4;

    // byte 2
    NvU8 sampleSize       : 2;
    NvU8 sampleRate       : 3;
    NvU8 rsvd_bits_byte2  : 3;

    // byte 3
    NvU8  byte3;

    // byte 4
    NvU8  speakerPlacement;

    // byte 5
    NvU8 rsvd_bits_byte5  : 3;
    NvU8 levelShift       : 4;
    NvU8 downmixInhibit   : 1;

    // byte 6~10
    NvU8 rsvd_byte6;
    NvU8 rsvd_byte7;
    NvU8 rsvd_byte8;
    NvU8 rsvd_byte9;
    NvU8 rsvd_byte10;

}NV_AUDIO_INFOFRAME;

typedef struct
{
    // byte 1
    NvU8 scanInfo                : 2;
    NvU8 barInfo                 : 2;
    NvU8 activeFormatInfoPresent : 1;
    NvU8 colorSpace              : 2;
    NvU8 rsvd_bits_byte1         : 1;

    // byte 2
    NvU8 activeFormatAspectRatio : 4;
    NvU8 picAspectRatio          : 2;
    NvU8 colorimetry             : 2;

    // byte 3
    NvU8 nonuniformScaling       : 2;
    NvU8 rsvd_bits_byte3         : 6;

    // byte 4
    NvU8 vic                     : 7;
    NvU8 rsvd_bits_byte4         : 1;

    // byte 5
    NvU8 pixelRepeat             : 4;
    NvU8 rsvd_bits_byte5         : 4;

    // byte 6~13 
    NvU8 topBarLow;
    NvU8 topBarHigh;
    NvU8 bottomBarLow;
    NvU8 bottomBarHigh;
    NvU8 leftBarLow;
    NvU8 leftBarHigh;
    NvU8 rightBarLow;
    NvU8 rightBarHigh;

} NV_VIDEO_INFOFRAME;

typedef struct
{
    NV_INFOFRAME_HEADER    header;
    union
    {
        NV_AUDIO_INFOFRAME audio;
        NV_VIDEO_INFOFRAME video;
    }u;
} NV_INFOFRAME;
NVAPI_INTERFACE NvAPI_GetInfoFrame(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_INFOFRAME_TYPE type, NV_INFOFRAME *pInfoFrame);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_SetInfoFrame
//
// DESCRIPTION:     This API returns the current infoframe data on the specified device(monitor)
//
// PARAMETERS:      hNvDisplay(IN) - NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//                  outputId(IN)   - The display output id. If it's "0" then the default outputId from NvAPI_GetAssociatedDisplayOutputId() will be used.
//                  type(IN)       - The type of infoframe to set
//                  pInfoFrame(IN) - The infoframe data, NULL mean reset to the default value.
//
// RETURN STATUS: 
//                  NVAPI_OK - completed request
//                  NVAPI_ERROR - miscellaneous error occurred
//                  NVAPI_INVALID_ARGUMENT: Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////

NVAPI_INTERFACE NvAPI_SetInfoFrame(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_INFOFRAME_TYPE type, NV_INFOFRAME *pInfoFrame);


//-----------------------------------------------------------------------------
// DirectX APIs
//-----------------------------------------------------------------------------
DECLARE_HANDLE(NVDX_ObjectHandle);  // DX Objects
const NVDX_ObjectHandle NVDX_OBJECT_NONE = 0;

//-----------------------------------------------------------------------------
// Direct3D9 APIs
//-----------------------------------------------------------------------------

#if defined(_D3D9_H_) && defined(__cplusplus)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GetSurfaceHandle
//
//   DESCRIPTION: Get the handle of a given surface. This handle uniquely 
//                identify the surface through all NvAPI entries.
//
//         INPUT: pSurface - Surface to be identified
//
//        OUTPUT: pHandle - Will be filled by the return handle
//
//          DEMO: PrimTexture
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GetSurfaceHandle(IDirect3DSurface9 *pSurface, 
                                        NVDX_ObjectHandle *pHandle);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GetTextureHandle
//
//   DESCRIPTION: Get the handle of a given DX9 texture.
//
//         INPUT: pTexture - Surface to be identified
//
//        OUTPUT: pHandle - Will be filled by the return handle
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GetTextureHandle(IDirect3DTexture9 *pTexture, 
                                        NVDX_ObjectHandle *pHandle);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GpuSyncGetHandleSize
//
//   DESCRIPTION: returns size of the init and copy sync handles. These handles are
//                then allocated and initialized to zero by the application
//
//        OUTPUT: pInitHandleSize - size of GpuSync init handle
//                pMapHandleSize - size of GpuSync copy handle
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GpuSyncGetHandleSize(IDirect3DDevice9 *pDev, 
                                            unsigned int *pInitHandleSize,
                                            unsigned int *pMapHandleSize);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GpuSyncInit
//
//   DESCRIPTION: setup sync functionality
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GpuSyncInit(IDirect3DDevice9 *pDev, 
                                            void * syncInitData);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GpuSyncEnd
//
//   DESCRIPTION: tear down sync structures
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GpuSyncEnd(IDirect3DDevice9 *pDev, 
                                            void * syncData);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GpuSyncMapTexBuffer
//
//   DESCRIPTION: map a texture to receive ogl data
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GpuSyncMapTexBuffer(IDirect3DDevice9 *pDev, 
                                            IDirect3DTexture9 *pTexture,
                                            void * syncData);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GpuSyncMapVertexBuffer
//
//   DESCRIPTION: map a vertex buffer to recieve ogl data
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GpuSyncMapVertexBuffer(IDirect3DDevice9 *pDev, 
                                            IDirect3DVertexBuffer9 *pVertexBuffer,
                                            void * syncData);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GpuSyncAcquire
//
//   DESCRIPTION: semaphore acquire for synchronization control of mapped buffer
//
//         INPUT: accessMode - acquire mapped buffer read/write access
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GpuSyncAcquire(IDirect3DDevice9 *pDev, 
                                            void * syncData);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GpuSyncRelease
//
//   DESCRIPTION: semaphore release for synchronization control of mapped buffer
//
//         INPUT: accessMode - release mapped buffer read/write access
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GpuSyncRelease(IDirect3DDevice9 *pDev, 
                                            void * syncData);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GetCurrentRenderTargetHandle
//
//   DESCRIPTION: Get the handle of current RT/ZB.
//
//         INPUT: pDev - Device whose current ZB to be identified
//
//        OUTPUT: pHandle - Will be filled by the return handle
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GetCurrentRenderTargetHandle(IDirect3DDevice9 *pDev,
                                            NVDX_ObjectHandle *pHandle);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GetCurrentZBufferHandle
//
//   DESCRIPTION: 
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GetCurrentZBufferHandle(IDirect3DDevice9 *pDev,
                                               NVDX_ObjectHandle *pHandle);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GetIndexBufferHandle
//
//   DESCRIPTION: Get the handle of a given DX9 index buffer.
//
//         INPUT: pIndexBuffer - Index Buffer to be identified
//
//        OUTPUT: pHandle - Will be filled by the return handle
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GetIndexBufferHandle(IDirect3DIndexBuffer9 *pIndexBuffer, 
                                        NVDX_ObjectHandle *pHandle);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_GetVertexBufferHandle
//
//   DESCRIPTION: Get the handle of a given DX9 vertex buffer.
//
//         INPUT: pVertexBuffer - Vertex Buffer to be identified
//
//        OUTPUT: pHandle - Will be filled by the return handle
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_GetVertexBufferHandle(IDirect3DVertexBuffer9 *pVertexBuffer, 
                                        NVDX_ObjectHandle *pHandle);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_AliasPrimaryAsTexture
//
//   DESCRIPTION: Create an texture that is an alias of current device's primary surface
//
//         INPUT:  pDev       The device to get primary surface from
//                 dwIndex    The index to the primary flipchain of device (usually 0)
// 
//        OUTPUT:  ppTexture  Fill with the texture created
//                 pHandle    If non-NULL, fill with the NVDX handle of the created texture
//
//          DEMO:  PrimTexture
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_AliasPrimaryAsTexture(IDirect3DDevice9 *pDev, 
                                            NvU32 dwIndex,
                                            IDirect3DTexture9 **ppTexture,
                                            NVDX_ObjectHandle *pHandle = 0);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_PresentSurfaceToDesktop
//
//   DESCRIPTION: Present a given surface to the desktop. This interface can be
//                used to start a fullscreen flipping mode even within windowed D3D app.
//
//         INPUT: pDev              The device (display) to present to surfaceHandle   The surface
//                                  handle obtained from NVD3D9_GetSurfaceHandle
//                                  NOTE: NVDX_OBJECT_NONE means restore
//                dwFlipFlags       Flags to indicate SYNC mode (other bits reserved and must be 0)
//                dwExcludeDevices  This is a bitmask (usually 0) to indicate which device
//                                  will be EXCLUDED from this present. This is only
//                                  effective when used in a clone mode configuration where
//                                  the application wants one monitor to show the specially
//                                  rendered screen and the other normal desktop.
//
//          NOTE: It is applications responsibility to determine which devices are
//                available on the current clone config, through nvcpl interfaces.
//
//          DEMO: PrimTexture
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////

// Parameter used for dwFlipFlags (All other bits reserved)
#define NV_FLIPFLAG_VSYNC               0x00000001  // SYNCMODE         (bit 0:1) - 0:NOSYNC, 1:VSYNC, 2:HSYNC
#define NV_FLIPFLAG_HSYNC               0x00000002
#define NV_FLIPFLAG_TRIPLEBUFFERING     0x00000004  // TRIPLEBUFFERING  (bit 2)   - 0: DoubleBuffer, 1:TripleBuffer or more

NVAPI_INTERFACE NvAPI_D3D9_PresentSurfaceToDesktop(IDirect3DDevice9 *pDev, 
                                               NVDX_ObjectHandle surfaceHandle, 
                                               NvU32 dwFlipFlags,
                                               NvU32 dwExcludeDevices = 0);

///////////////////////////////////////////////////////////////////////////////
// 
// FUNCTION_NAME: NvAPI_D3D9_CreateVideo
//
//   DESCRIPTION: Allocates and initializes video resources for playback within a D3D9 context
//
//         INPUT: pDev              The device (display) to present to
//                dwVersion         Version of the CreateVideoParams structure
//                dwCVFlags         Initialization flags (see defines for description)
//                dwFlipQueueHint   If overlay is allocated, indication of the desired number of flips
//                                  to be queued (minimum of 1).  A larger number is better for performance,
//                                  but also consumes more frame buffer resources.  This number should be
//                                  set according to the number of fields/frames that can be decoded in
//                                  advance of presentation.  Eg., if the decoder cannot decode ahead and
//                                  must display immediately after decoding a frame, then set this to 1.
//                                  If the decoder is decoding field content one frame at a time, then it
//                                  is decoding one field ahead and should set this to 2.  If the decoder
//                                  can decode n flips ahead, then set this to n+1.  If the GPU has
//                                  constrained frame buffer resources, then set this number lower, and also
//                                  reduce the decode-ahead pipeline resources accordingly.
//                dwMaxSrcWidth     Maximum video source width
//                dwMaxSrcHeight    Maximum video source height
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////

#define NV_CVFLAG_OVERLAY                   0x00000001      // Overlay will be used
#define NV_CVFLAG_OVERLAY_REINIT            0x00000002      // Change source size or flip queue hint once overlay
                                                            // resources have already been allocated.  Be wary
                                                            // of upward allocating new resources, if there is
                                                            // a failure, be prepared to deallocate everything
                                                            // and start from scratch.
#define NV_CVFLAG_2ND_FORCE_DISABLE         0x00010000      // Forces full screen video on secondary device to be
                                                            // disabled in multihead modes, regardless of user setting
#define NV_CVFLAG_2ND_FORCE_ENABLE          0x00020000      // Forces full screen video on secondary device to be
                                                            // enabled on the default device in multihead modes, 
                                                            // regardless of user setting
#define NV_CVFLAG_2ND_FORCE_ENABLE1         0x00040000      // Forces full screen video on secondary device to be
                                                            // enabled on the primary device in clone mode, 
                                                            // regardless of user setting
#define NV_CVFLAG_2ND_COMMANDEER            0x00100000      // If another application owns the secondary full screen
                                                            // device, forcibly take possession of it.

typedef struct
{
    NvU32 version;
    NvU32 cvFlags;
    NvU32 flipQueueHint;
    NvU32 maxSrcWidth;
    NvU32 maxSrcHeight;
} NV_DX_CREATE_VIDEO_PARAMS1;

typedef NV_DX_CREATE_VIDEO_PARAMS1  NV_DX_CREATE_VIDEO_PARAMS;

#define NV_DX_CREATE_VIDEO_PARAMS_VER  MAKE_NVAPI_VERSION(NV_DX_CREATE_VIDEO_PARAMS1,1)

NVAPI_INTERFACE NvAPI_D3D9_CreateVideo(IDirect3DDevice9 *pDev, 
                                       NV_DX_CREATE_VIDEO_PARAMS *pCVParams);

///////////////////////////////////////////////////////////////////////////////
// 
// FUNCTION_NAME: NvAPI_D3D9_FreeVideo
//
//   DESCRIPTION: Releases all video resources
//
//         INPUT: pDev              The device (display) to present to
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////

NVAPI_INTERFACE NvAPI_D3D9_FreeVideo(IDirect3DDevice9 *pDev);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_PresentVideo
//
//   DESCRIPTION: Signals a final and complete frame ready for presentation.
//                Can optionally render to the overlay, but should be called
//                regardless of whether any actual rendering occurs.  If the
//                user has enabled full screen video in a multihead mode,
//                this frame will also be rendered on the secondary device.
//
//         INPUT: pDev              The device (display) to present to
//                dwVersion         Version of the PresentVideoParams structure
//                surfaceHandle     The surface handle obtained from NvAPI_D3D9_GetSurfaceHandle
//                                  or NvAPI_D3D9_GetCurrentRenderTargetHandle
//                dwPVFlags         Presentation flags (see defines for description)
//                dwColourKey       Colour key to use if NV_PVFLAG_DST_KEY is set
//                qwTimeStamp*      If NV_PVFLAG_USE_STAMP is set, time in ns when the frame is to be presented
//                                  If NV_PVFLAG_SET_STAMP is set, set the current time to this, and present on next vblank
//                dwFlipRate        Set to the current flip rate
//                                  Set to zero if the frame to be presented is a still frame
//                srcUnclipped      Unclipped source rectangle of the entire frame of data
//                srcClipped        Cropped source rectangle.  It is the caller's responsibility to crop
//                                  the source if the desktop crops the destination.
//                dst               Destination rectangle (in desktop coordinates) of the overlay.  It is the
//                                  caller's responsibility to crop the destination against the desktop.
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////

// PresentVideo flags
#define NV_PVFLAG_ODD           0x00000001      // Field is odd
#define NV_PVFLAG_EVEN          0x00000002      // Field is even
#define NV_PVFLAG_PROTECTED     0x00000004      // Indicates that this frame is protected and guarantees full 
                                                // screen video will not display this frame on any secondary device.
                                                // Conversely, not setting this indicates an unprotected frame
#define NV_PVFLAG_PROGRESSIVE   0x00000008      // Indicates progressive frame.  If the odd or even flags are set
                                                // in conjunction with this, it indicates the original field that
                                                // generated this deinterlaced frame, and attempts to synchronize
                                                // this presentation to the corresponding display field of an
                                                // interlaced display
#define NV_PVFLAG_SHOW          0x00000010      // Show the overlay
                                                // If the app is minimized or obscured, continue to call NvAPI_D3D9_PresentVideo
                                                // for every complete frame without this flag set.
                                                // If enabled, unprotected video will continue to play full screen
                                                // on the secondary device, using the pixel aspect cached from
                                                // the last time a frame was shown.  To change the pixel aspect while hidden,
                                                // the caller must "show" a frame at least once with a new clipped source and 
                                                // destination rectangle.  This shown frame can be rendered invisible with
                                                // appropriate selection of colour key.
#define NV_PVFLAG_FAST_MOVE     0x00000020      // Move overlay position without waiting for vblank.
                                                // The only parameters used are dwDstX, dwDstY, and NV_PVFLAG_SHOW.
#define NV_PVFLAG_WAIT          0x00000040      // If set, blocking flip, wait until flip queue can accept another flip.
                                                // A non-blocking flip will return an error if flip cannot be queued yet.
#define NV_PVFLAG_REPEAT        0x00000080      // Video data is completely unchanged from the previous flip (used for telecine)
#define NV_PVFLAG_DST_KEY       0x00000100      // Use destination colour key.
#define NV_PVFLAG_FULLSCREEN    0x00000200      // Indicates that the overlay is playing fullscreen on the desktop.
                                                // This bit is used to automatically overscan the image on TV's.
#define NV_PVFLAG_SET_STAMP     0x00001000      // Set the current time.
#define NV_PVFLAG_USE_STAMP     0x00002000      // If set, use timestamps.
                                                // If not set, flip on the next vblank.

typedef struct 
{
    NvU32 version;
    NVDX_ObjectHandle surfaceHandle;
    NvU32 pvFlags;
    NvU32 colourKey;
    NvU32 timeStampLow;
    NvU32 timeStampHigh;
    NvU32 flipRate;
    NvSBox srcUnclipped;
    NvSBox srcClipped;
    NvSBox dst;
} NV_DX_PRESENT_VIDEO_PARAMS1;

typedef NV_DX_PRESENT_VIDEO_PARAMS1 NV_DX_PRESENT_VIDEO_PARAMS;

#define NV_DX_PRESENT_VIDEO_PARAMS_VER  MAKE_NVAPI_VERSION(NV_DX_PRESENT_VIDEO_PARAMS1,2)

NVAPI_INTERFACE NvAPI_D3D9_PresentVideo(IDirect3DDevice9 *pDev, 
                                        NV_DX_PRESENT_VIDEO_PARAMS *pPVParams);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_RestoreDesktop
//
//   DESCRIPTION: 
//
// RETURN STATUS: NVAPI_ERROR or NVAPI_OK
//
// NOTE: This is not an interface, this is just a short-hand helper
//
///////////////////////////////////////////////////////////////////////////////
inline int NvAPI_D3D9_RestoreDesktop(IDirect3DDevice9 *pDev)
{
    return NvAPI_D3D9_PresentSurfaceToDesktop(pDev,NVDX_OBJECT_NONE,0);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_AliasPrimaryFromDevice
//
//   DESCRIPTION: Create an alias surface from the given pDevFrom's primary swap chain.
//
// INPUT:   pDevTo      Where new surfaces created in
//          pDevFrom    Where the surfaces aliased from
//          dwIndex     Index to the primary flipchain of pDevFrom
//          
// OUTPUT:  ppSurf      Filled with new surface pointer (to be released by caller)
//          pHandle     (optional) If non-NULL, filled with SurfaceHandle of the surface
//                      Same can be achieved by calling NVD3D9_GetSurfaceHandle afterwards
// 
// DEMO:    Multihead
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
NVAPI_INTERFACE NvAPI_D3D9_AliasPrimaryFromDevice(IDirect3DDevice9 *pDevTo, 
                                              IDirect3DDevice9 *pDevFrom, 
                                              NvU32 dwIndex,
                                              IDirect3DSurface9 **ppSurf,
                                              NVDX_ObjectHandle *pHandle = 0);
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_SetResourceHint
//
//   DESCRIPTION: General purpose function for passing down various resource 
//                related hints to the driver. Hints are divided into categories
//                and types within each cateroy. 
//
//  INPUT: pDev - valid device context
//         obj  - previously obtained HV resource handle 
//         dwHintCategory - category of the hints
//         dwHintType     - a hint within this category
//         *pdwHintValue  - pointer to location containing hint value
//  
//  OUTPUT:
//         *dwHintValue   - receives previous value of this hint.
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code

// Avaliable categories / hint names:
//     Sli:
typedef enum _NVAPI_SETRESOURCEHINT_CATEGORY
{
    NvApiHints_Sli = 1,

}  NVAPI_SETRESOURCEHINT_CATEGORY;


// types of Sli hints
  // InterframeAwareForTexturing tells the driver to discard any interframe
  // dirty state (skip inter-gpu transfers) on this object when texturing from it.
  // other operations (clear, render, blit, cpu lock) are not affected by this bit.
  // The value of the bit is a max difference (in frames) of the last modifications
  // of the current and golden-copy gpus, which is allowed to get discarded.
  // For example, if the value is 1 then inactive buffer will be discarded only if it was
  // modified on the same or previous frame and will be transfered to active gpu otherwise.
  //
  // default value: zero

typedef enum _NVAPI_SETRESOURCEHINT_SLI_HINTS
{
    NvApiHints_Sli_InterfarameAwareForTexturing = 1,

}  NVAPI_SETRESOURCEHINT_SLI_HINTS;

//
// end of categories

NVAPI_INTERFACE NvAPI_D3D9_SetResourceHint(IDirect3DDevice9 *pDev, NVDX_ObjectHandle obj, 
                            NVAPI_SETRESOURCEHINT_CATEGORY dwHintCategory, NvU32 dwHintName, NvU32 *pdwHintValue);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_Lock
//
//   DESCRIPTION: Lock and Unlock a given surface identified by handle. This
//                function can provide CPU access to all object including
//                RTs, ZBs, textures, VBs and IBs.
// NOTE: 
// (a) If an object can be accessed with normal DX9 means, please do not use this
// (b) Lock should be called right before CPU access, and Unlock called right after
//     the access is done. Any 3D rendering or state change may cause the locked
//     surface to be lost. When that happens, trying to access the cached CPU 
//     address may causing app to crash.
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#define NV_ACCESSFLAG_READONLY  0x00000001 // TBD: do these go with dwLockFlags?
#define NV_ACCESSFLAG_DISCARD   0x00000002
NVAPI_INTERFACE NvAPI_D3D9_Lock(IDirect3DDevice9 *pDev, NVDX_ObjectHandle obj, NvU32 dwLockFlags,
                            void **ppAddress, NvU32 *pPitch);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_Unlock
//
//   DESCRIPTION: 
//
// RETURN STATUS: an int which could be an NvAPI status or DX HRESULT code
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D9_Unlock(IDirect3DDevice9 *pDev, NVDX_ObjectHandle obj);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D9_LockForCUDA
//
//   DESCRIPTION: Lock a given surface identified by handle. This
//                function can provide access to all objects including
//                RTs, ZBs, textures, VBs and IBs.
// NOTE: 
//     LockForCUDA is analogous to LockForCPU, except that the memory will be
//     read/written by CUDA (compute running in a separate channel) as opposed 
//     to the CPU.  It should be called right before giving access to CUDA
//     and Unlock called right after the access is done.
//
// RETURN STATUS: NvAPI status
//
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
NVAPI_INTERFACE NvAPI_D3D9_LockForCUDA(IDirect3DDevice9 *pDev, NVDX_ObjectHandle obj, NvU32 dwLockFlags,
                            NvU32 *pClient, NvU32 *pHandle, NvU64 *pOffset, NvU32 *pSize);

#endif //defined(_D3D9_H_) && defined(__cplusplus)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetAllOutputs
//
//   DESCRIPTION: Returns set of all GPU-output identifiers as a bitmask.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu or pOutputsMask is NULL
//                NVAPI_OK: *pOutputsMask contains a set of GPU-output identifiers
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetAllOutputs(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pOutputsMask);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetConnectedOutputs
//
//   DESCRIPTION: Same as NvAPI_GPU_GetAllOutputs but returns only the set of GPU-output 
//                identifiers that are connected to display devices.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu or pOutputsMask is NULL
//                NVAPI_OK: *pOutputsMask contains a set of GPU-output identifiers
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetConnectedOutputs(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetConnectedOutputsWithLidState
//
//   DESCRIPTION: Similar to NvAPI_GPU_GetConnectedOutputs this API returns the connected display identifiers that are connected 
//		          as a output mask but unlike NvAPI_GPU_GetConnectedOutputs this API "always" reflects the Lid State in the output mask.
//                Thus if you expect the LID close state to be available in the connection mask use this API.
//		          If LID is closed then this API will remove the LID panel from the connected display identifiers. 
//		          If LID is open then this API will reflect the LID panel in the connected display identifiers. 
//		          Note:This API should be used on laptop systems and on systems where LID state is required in the connection output mask. 
//                     On desktop systems the returned identifiers will match NvAPI_GPU_GetConnectedOutputs.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu or pOutputsMask is NULL
//                NVAPI_OK: *pOutputsMask contains a set of GPU-output identifiers
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetConnectedOutputsWithLidState(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetActiveOutputs
//
//   DESCRIPTION: Same as NvAPI_GPU_GetAllOutputs but returns only the set of GPU-output 
//                identifiers that are actively driving display devices.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu or pOutputsMask is NULL
//                NVAPI_OK: *pOutputsMask contains a set of GPU-output identifiers
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetActiveOutputs(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetEDID
//
//   DESCRIPTION: Returns the EDID data for the specified GPU handle and connection bit mask.
//                outputsMask should have exactly 1 bit set to indicate a single display.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: pEDID is NULL; outputsMask has 0 or > 1 bits set
//                NVAPI_OK: *pEDID contains valid data
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
#define NV_EDID_DATA_SIZE   256
typedef struct
{
    NvU32   version;        //structure version
    NvU8    EDID_Data[NV_EDID_DATA_SIZE];
} NV_EDID;

#define NV_EDID_VER  MAKE_NVAPI_VERSION(NV_EDID,1)
NVAPI_INTERFACE NvAPI_GPU_GetEDID(NvPhysicalGpuHandle hPhysicalGpu, NvU32 outputsMask, NV_EDID *pEDID);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetOutputType
//
//   DESCRIPTION: Give a physical GPU handle and a single outputId (exactly 1 bit set), this
//                returns the output type.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu, outputId or pOutputsMask is NULL; or outputId has > 1 bit set
//                NVAPI_OK: *pOutputType contains a NvGpuOutputType value
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
typedef enum _NV_GPU_OUTPUT_TYPE
{
    NVAPI_GPU_OUTPUT_UNKNOWN  = 0,
    NVAPI_GPU_OUTPUT_CRT      = 1,     // CRT display device
    NVAPI_GPU_OUTPUT_DFP      = 2,     // Digital Flat Panel display device
    NVAPI_GPU_OUTPUT_TV       = 3,     // TV display device
} NV_GPU_OUTPUT_TYPE;

NVAPI_INTERFACE NvAPI_GPU_GetOutputType(NvPhysicalGpuHandle hPhysicalGpu, NvU32 outputId, NV_GPU_OUTPUT_TYPE *pOutputType);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_ValidateOutputCombination
//
//   DESCRIPTION: This call is used to determine if a set of GPU outputs can be active 
//                simultaneously.  While a GPU may have <n> outputs, they can not typically
//                all be active at the same time due to internal resource sharing.
//
//                Given a physical GPU handle and a mask of candidate outputs, this call
//                will return NVAPI_OK if all of the specified outputs can be driven
//                simultaneously.  It will return NVAPI_INVALID_COMBINATION if they cannot.
//                
//                Use NvAPI_GPU_GetAllOutputs() to determine which outputs are candidates.
//
// RETURN STATUS: NVAPI_OK: combination of outputs in outputsMask are valid (can be active simultaneously)
//                NVAPI_INVALID_COMBINATION: combination of outputs in outputsMask are NOT valid
//                NVAPI_INVALID_ARGUMENT: hPhysicalGpu or outputsMask does not have at least 2 bits set
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_ValidateOutputCombination(NvPhysicalGpuHandle hPhysicalGpu, NvU32 outputsMask);

typedef enum _NV_GPU_CONNECTOR_TYPE
{
    NVAPI_GPU_CONNECTOR_VGA_15_PIN          = 0x00000000,
    NVAPI_GPU_CONNECTOR_TV_COMPOSITE        = 0x00000010,
    NVAPI_GPU_CONNECTOR_TV_SVIDEO           = 0x00000011,
    NVAPI_GPU_CONNECTOR_TV_HDTV_COMPONENT   = 0x00000013,
    NVAPI_GPU_CONNECTOR_TV_SCART            = 0x00000014,
    NVAPI_GPU_CONNECTOR_TV_HDTV_EIAJ4120    = 0x00000017,
    NVAPI_GPU_CONNECTOR_PC_POD_HDTV_YPRPB   = 0x00000018,
    NVAPI_GPU_CONNECTOR_PC_POD_SVIDEO       = 0x00000019,
    NVAPI_GPU_CONNECTOR_PC_POD_COMPOSITE    = 0x0000001A,
    NVAPI_GPU_CONNECTOR_DVI_I_TV_SVIDEO     = 0x00000020,
    NVAPI_GPU_CONNECTOR_DVI_I_TV_COMPOSITE  = 0x00000021,
    NVAPI_GPU_CONNECTOR_DVI_I               = 0x00000030,
    NVAPI_GPU_CONNECTOR_DVI_D               = 0x00000031,
    NVAPI_GPU_CONNECTOR_ADC                 = 0x00000032,
    NVAPI_GPU_CONNECTOR_LFH_DVI_I_1         = 0x00000038,
    NVAPI_GPU_CONNECTOR_LFH_DVI_I_2         = 0x00000039,
    NVAPI_GPU_CONNECTOR_SPWG                = 0x00000040,
    NVAPI_GPU_CONNECTOR_OEM                 = 0x00000041,
    NVAPI_GPU_CONNECTOR_HDMI_A              = 0x00000061,
    NVAPI_GPU_CONNECTOR_UNKNOWN             = 0xFFFFFFFF,
} NV_GPU_CONNECTOR_TYPE;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetFullName
//
//   DESCRIPTION: Retrieves the full GPU name as an ascii string.  Eg: "Quadro FX 1400"
//
// RETURN STATUS: NVAPI_ERROR or NVAPI_OK
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetFullName(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szName);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetPCIIdentifiers
//
//   DESCRIPTION: Returns the PCI identifiers associated this this GPU.
//                  DeviceId - the internal PCI device identifier for the GPU.
//                  SubSystemId - the internal PCI subsystem identifier for the GPU.
//                  RevisionId - the internal PCI device-specific revision identifier for the GPU.
//                  ExtDeviceId - the external PCI device identifier for the GPU.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu or an argument is NULL
//                NVAPI_OK: arguments are populated with PCI identifiers
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetPCIIdentifiers(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pDeviceId,NvU32 *pSubSystemId,NvU32 *pRevisionId,NvU32 *pExtDeviceId);
    
typedef enum _NV_GPU_BUS_TYPE
{
    NVAPI_GPU_BUS_TYPE_UNDEFINED    = 0,
    NVAPI_GPU_BUS_TYPE_PCI          = 1,
    NVAPI_GPU_BUS_TYPE_AGP          = 2,
    NVAPI_GPU_BUS_TYPE_PCI_EXPRESS  = 3,
    NVAPI_GPU_BUS_TYPE_FPCI         = 4,
} NV_GPU_BUS_TYPE;
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBusType
//
//   DESCRIPTION: Returns the type of bus associated this this GPU.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu or pBusType is NULL
//                NVAPI_OK: *pBusType contains bus identifier
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetBusType(NvPhysicalGpuHandle hPhysicalGpu,NV_GPU_BUS_TYPE *pBusType);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetIRQ
//
//   DESCRIPTION: Returns the interrupt number associated this this GPU.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu or pIRQ is NULL
//                NVAPI_OK: *pIRQ contains interrupt number
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetIRQ(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pIRQ);
    
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosRevision
//
//   DESCRIPTION: Returns the revision of the video bios associated this this GPU.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu or pBiosRevision is NULL
//                NVAPI_OK: *pBiosRevision contains revision number
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosRevision(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pBiosRevision);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosOEMRevision
//
//   DESCRIPTION: Returns the OEM revision of the video bios associated this this GPU.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu or pBiosRevision is NULL
//                NVAPI_OK: *pBiosRevision contains revision number
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosOEMRevision(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pBiosRevision);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosVersionString
//
//   DESCRIPTION: Returns the full bios version string in the form of xx.xx.xx.xx.yy where
//                the xx numbers come from NvAPI_GPU_GetVbiosRevision and yy comes from 
//                NvAPI_GPU_GetVbiosOEMRevision.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu is NULL
//                NVAPI_OK: szBiosRevision contains version string
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosVersionString(NvPhysicalGpuHandle hPhysicalGpu,NvAPI_ShortString szBiosRevision);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetAGPAperture
//
//   DESCRIPTION: Returns AGP aperture in megabytes
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: pSize is NULL
//                NVAPI_OK: call successful
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetAGPAperture(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pSize);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetCurrentAGPRate
//
//   DESCRIPTION: Returns the current AGP Rate (1 = 1x, 2=2x etc, 0 = AGP not present)
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: pRate is NULL
//                NVAPI_OK: call successful
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetCurrentAGPRate(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pRate);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetCurrentPCIEDownstreamWidth
//
//   DESCRIPTION: Returns the number of PCIE lanes being used for the PCIE interface 
//                downstream from the GPU.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: pWidth is NULL
//                NVAPI_OK: call successful
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetCurrentPCIEDownstreamWidth(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pWidth);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetPhysicalFrameBufferSize
//
//   DESCRIPTION: Returns the physical size of framebuffer in Kb.  This does NOT include any
//                system RAM that may be dedicated for use by the GPU.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: pSize is NULL
//                NVAPI_OK: call successful
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetPhysicalFrameBufferSize(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pSize);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVirtualFrameBufferSize
//
//   DESCRIPTION: Returns the virtual size of framebuffer in Kb.  This includes the physical RAM plus any
//                system RAM that has been dedicated for use by the GPU.
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: pSize is NULL
//                NVAPI_OK: call successful
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVidia GPU driving a display was found
//                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVirtualFrameBufferSize(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pSize);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetQuadroStatus
//
//   DESCRIPTION: Retrieves the Quadro status for the GPU (1 if Quadro, 0 if GeForce)
//
// RETURN STATUS: NVAPI_ERROR or NVAPI_OK
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetQuadroStatus(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pStatus); 


//Performance table overclocking 

typedef enum _NV_GPU_PERF_CLOCK_DOMAIN_ID
{
    NVAPI_GPU_PERF_CLOCK_DOMAIN_NV      = 0,
    NVAPI_GPU_PERF_CLOCK_DOMAIN_M       = 4,

} NV_GPU_PERF_CLOCK_DOMAIN_ID;

#define NVAPI_MAX_GPU_PERF_CLOCKS       32

#define NVAPI_MAX_PERF_CLOCK_LEVELS     12
#define NVAPI_TARGET_ALL_PERF_LEVELS    0xffffffff

#define NV_PERF_CLOCK_LEVEL_STATE_DEFAULT                   0x00000000 //level is in its default state
#define NV_PERF_CLOCK_LEVEL_STATE_OVERCLOCKED               0x00000001 //level is overclocked
#define NV_PERF_CLOCK_LEVEL_STATE_DESKTOP                   0x00000002 //2d desktop perf level
#define NV_PERF_CLOCK_LEVEL_STATE_PERFORMANCE               0x00000004 //3d applications perf level
#define NV_PERF_CLOCK_LEVEL_STATE_TEST                      0x00000008 //test the new clocks for this level. Does not apply.
#define NV_PERF_CLOCK_LEVEL_STATE_TEST_SUCCESS              0x00000010 //test result
      
#define NV_PERF_CLOCK_GPU_STATE_DEFAULT                     0x00000000 //default state
#define NV_PERF_CLOCK_GPU_STATE_DYNAMIC_SUPPORTED           0x00000001 //gpu supports dynamic performance level transitions
#define NV_PERF_CLOCK_GPU_STATE_DESKTOP                     0x00000002 //gpu in desktop level
#define NV_PERF_CLOCK_GPU_STATE_PERFORMANCE                 0x00000004 //gpu in performance level
#define NV_PERF_CLOCK_GPU_STATE_ACTIVE_CLOCKING_SUPPORTED   0x00000008 //active clocking supported
#define NV_PERF_CLOCK_GPU_STATE_ACTIVE_CLOCKING_ENABLE      0x00000010 //enable active clocking
#define NV_PERF_CLOCK_GPU_STATE_ACTIVE_CLOCKING_DISABLE     0x00000020 //disable active clocking
#define NV_PERF_CLOCK_GPU_STATE_MEMCLK_CONTROL_DISABLED     0x00000040 //memory clock control disabled
#define NV_PERF_CLOCK_GPU_STATE_GFXCLK_CONTROL_DISABLED     0x00000080 //core clock control disabled
#define NV_PERF_CLOCK_GPU_STATE_SET_DEFERRED                0x00000100 //No immediate perf transitions. Deferred until perf triggers kick in.

typedef struct
{
    NvU32   version;                                //IN perf clock table version
    NvU32   levelCount;                             //number of the performance levels. count increases everytime a level is overclocked
    NvU32   gpuPerflevel;                           //OUT the current perf level. This is a dynamic level which can possibly change on every call
    NvU32   domainCount;                            //IN/OUT number of domains
    NvU32   gpuPerfFlags;                           //IN/OUT gpu flags - one of the flags defined in NV_PERF_CLOCK_GPU_STATE
    struct
    {
        NvU32   level;                              //IN/OUT performance level indicator, range 0 to levelCount - 1.
        NvU32   flags;                              //IN/OUT per level flags - one of more flags defined in NV_PERF_CLOCK_LEVEL_STATE 
        struct
        {
            NV_GPU_PERF_CLOCK_DOMAIN_ID domainId;       //IN/OUT current domain indicator - one of the ids from NV_GPU_PERF_CLOCK_DOMAIN_ID
            NvU32                       domainFlags;    //reserved unused domain flags 
            NvU32                       currentFreq;    //IN/OUT current clock KHz
            NvU32                       defaultFreq;    //default clock KHz
            NvU32                       minFreq;        //min KHz
            NvU32                       maxFreq;        //max KHz
            NvU32                       bSetClock:1;    //IN if set during NvAPI_GPU_SetPerfClocks call, this domain currentFreq will be applied
        } domain[NVAPI_MAX_GPU_PERF_CLOCKS];
    } perfLevel[NVAPI_MAX_PERF_CLOCK_LEVELS];
    
} NV_GPU_PERF_CLOCK_TABLE;

#define NV_GPU_PERF_CLOCK_TABLE_VER  MAKE_NVAPI_VERSION(NV_GPU_PERF_CLOCK_TABLE,1)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetPerfClocks
//
// DESCRIPTION:     Retrieves the performance clock table information for one or all the supported levels.
//
// PARAMETERS:      hPhysicalGPU(IN) - GPU selection.
//                  level(IN) - Specific level selection. Zero for all levels. Number of levels increases with overclocking of the levels.
//                  pPerfClkTable(OUT) - Table of performance levels retrieved.
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//    NVAPI_INCOMPATIBLE_STRUCT_VERSION - the version of the PERF_CLOCK_TABLE struct is not supported
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetPerfClocks(NvPhysicalGpuHandle hPhysicalGpu, NvU32 level, NV_GPU_PERF_CLOCK_TABLE *pPerfClkTable);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_SetPerfClocks
//
// DESCRIPTION:     Overclock a specific level in the performance table or overclock all levels with bSetClock set.
//
// PARAMETERS:      hPhysicalGPU(IN) - GPU selection.
//                  level(IN) - Specific level selection. Zero for all levels. Number of levels increases with overclocking of the levels.
//                  pPerfClkTable(IN) - Table of performance levels to set.
//                  Any other than DEFAULT for GPU and Level flags - gpuPerfFlags and level flags gets applied.
//                  If bSetClock is set, currentFreq gets applied.
//                  Overclocking DOMAIN_NV requires simulteneous overclocking of DOMAIN_M, else overclocking will fail.
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//    NVAPI_INCOMPATIBLE_STRUCT_VERSION - the version of the PERF_CLOCK_TABLE struct is not supported
//
///////////////////////////////////////////////////////////////////////////////

NVAPI_INTERFACE NvAPI_GPU_SetPerfClocks(NvPhysicalGpuHandle hPhysicalGpu, NvU32 level, NV_GPU_PERF_CLOCK_TABLE *pPerfClkTable);

///////////////////////////////////////////////////////////////////////////////////
//  GPU Cooler Controls
//  Provides ability to Get and Set the fan level or equivalent cooler levels for various target devices associated with the GPU

#define NVAPI_MAX_COOLERS_PER_GPU   3
#define NVAPI_MIN_COOLER_LEVEL      0
#define NVAPI_MAX_COOLER_LEVEL      100
#define NVAPI_MAX_COOLER_LEVELS 24

typedef enum 
{
    NVAPI_COOLER_TYPE_NONE = 0,
    NVAPI_COOLER_TYPE_FAN,
    NVAPI_COOLER_TYPE_WATER,
    NVAPI_COOLER_TYPE_LIQUID_NO2,
} NV_COOLER_TYPE;

typedef enum 
{
    NVAPI_COOLER_CONTROLLER_NONE = 0,
    NVAPI_COOLER_CONTROLLER_ADI,
    NVAPI_COOLER_CONTROLLER_INTERNAL,
} NV_COOLER_CONTROLLER;

typedef enum
{
    NVAPI_COOLER_POLICY_NONE = 0,
    NVAPI_COOLER_POLICY_MANUAL,                     //Manual adjustment of cooler level. Gets applied right away independent of temperature or performance level.
    NVAPI_COOLER_POLICY_PERF,                       //GPU performance controls the cooler level.
    NVAPI_COOLER_POLICY_TEMPERATURE_DISCRETE = 4,   //Discrete thermal levels control the cooler level.
    NVAPI_COOLER_POLICY_TEMPERATURE_CONTINUOUS = 8, //Cooler level adjusted at continuous thermal levels.
    NVAPI_COOLER_POLICY_HYBRID,                     //Hybrid of performance and temperature levels.
} NV_COOLER_POLICY;

typedef enum
{
    NVAPI_COOLER_TARGET_NONE = 0,
    NVAPI_COOLER_TARGET_GPU,
    NVAPI_COOLER_TARGET_MEMORY,
    NVAPI_COOLER_TARGET_POWER_SUPPLY = 4,
    NVAPI_COOLER_TARGET_ALL = 7                    //This cooler cools all of the components related to its target gpu.
} NV_COOLER_TARGET;

typedef enum
{
    NVAPI_COOLER_CONTROL_NONE = 0,
    NVAPI_COOLER_CONTROL_TOGGLE,                   //ON/OFF
    NVAPI_COOLER_CONTROL_VARIABLE,                 //Suppports variable control.
} NV_COOLER_CONTROL;

typedef enum
{
    NVAPI_INACTIVE = 0,             //inactive or unsupported
    NVAPI_ACTIVE = 1,               //active and spinning in case of fan
} NV_COOLER_ACTIVITY_LEVEL;

typedef struct
{
    NvU32   version;                //structure version 
    NvU32   count;                  //number of associated coolers with the selected GPU
    struct 
    {
        NV_COOLER_TYPE              type;               //type of cooler - FAN, WATER, LIQUID_NO2...
        NV_COOLER_CONTROLLER        controller;         //internal, ADI...
        NvU32                       defaultMinLevel;    //the min default value % of the cooler
        NvU32                       defaultMaxLevel;    //the max default value % of the cooler
        NvU32                       currentMinLevel;    //the current allowed min value % of the cooler
        NvU32                       currentMaxLevel;    //the current allowed max value % of the cooler
        NvU32                       currentLevel;       //the current value % of the cooler
        NV_COOLER_POLICY            defaultPolicy;      //cooler control policy - auto-perf, auto-thermal, manual, hybrid...
        NV_COOLER_POLICY            currentPolicy;      //cooler control policy - auto-perf, auto-thermal, manual, hybrid...
        NV_COOLER_TARGET            target;             //cooling target - GPU, memory, chipset, powersupply, canoas...
        NV_COOLER_CONTROL           controlType;        //toggle or variable
        NV_COOLER_ACTIVITY_LEVEL    active;             //is the cooler active - fan spinning...
    } cooler[NVAPI_MAX_COOLERS_PER_GPU];

} NV_GPU_GETCOOLER_SETTINGS;

#define NV_GPU_GETCOOLER_SETTINGS_VER  MAKE_NVAPI_VERSION(NV_GPU_GETCOOLER_SETTINGS,1)

typedef struct 
{
    NvU32   version;        //structure version 
    struct 
    {
        NvU32               currentLevel;           //the new value % of the cooler
        NV_COOLER_POLICY    currentPolicy;          //the new cooler control policy - auto-perf, auto-thermal, manual, hybrid...
    } cooler[NVAPI_MAX_COOLERS_PER_GPU];

} NV_GPU_SETCOOLER_LEVEL;

#define NV_GPU_SETCOOLER_LEVEL_VER  MAKE_NVAPI_VERSION(NV_GPU_SETCOOLER_LEVEL,1)

typedef struct
{
    NvU32               version;        //structure version
    NV_COOLER_POLICY    policy;         //selected policy to update the cooler levels for, example NVAPI_COOLER_POLICY_PERF
    struct 
    {
        NvU32 levelId;      // level indicator for a policy
        NvU32 currentLevel; // new cooler level for the selected policy level indicator.
        NvU32 defaultLevel; // default cooler level for the selected policy level indicator.
    } policyCoolerLevel[NVAPI_MAX_COOLER_LEVELS];
} NV_GPU_COOLER_POLICY_TABLE;

#define NV_GPU_COOLER_POLICY_TABLE_VER MAKE_NVAPI_VERSION(NV_GPU_COOLER_POLICY_TABLE,1)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetCoolerSettings
//
// DESCRIPTION:     Retrieves the cooler information of all coolers or a specific cooler associated with the selected GPU.
//                  Coolers are indexed 0 to NVAPI_MAX_COOLERS_PER_GPU-1.
//                  To retrieve specific cooler info set the coolerIndex to the appropriate cooler index. 
//                  To retrieve info for all cooler set coolerIndex to NVAPI_COOLER_TARGET_ALL. 
//
// PARAMETERS :     hPhysicalGPU(IN) - GPU selection.
//                  coolerIndex(IN)  - Explict cooler index selection. 
//                  pCoolerInfo(OUT) - Array of cooler settings.
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_INVALID_ARGUMENT - pCoolerInfo is NULL
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//    NVAPI_INCOMPATIBLE_STRUCT_VERSION - the version of the INFO struct is not supported
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetCoolerSettings(NvPhysicalGpuHandle hPhysicalGpu, NvU32 coolerIndex, NV_GPU_GETCOOLER_SETTINGS *pCoolerInfo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_SetCoolerLevels
//
// DESCRIPTION:     Set the cooler levels for all coolers or a specific cooler associated with the selected GPU.
//                  Coolers are indexed 0 to NVAPI_MAX_COOLERS_PER_GPU-1. Every cooler level with non-zero currentpolicy gets applied.           
//                  The new level should be in the range of minlevel and maxlevel retrieved from GetCoolerSettings API or between 
//                  and NVAPI_MIN_COOLER_LEVEL to MAX_COOLER_LEVEL.
//                  To set level for a specific cooler set the coolerIndex to the appropriate cooler index. 
//                  To set level for all coolers set coolerIndex to NVAPI_COOLER_TARGET_ALL. 
// NOTE:            To lock the fan speed independent of the temperature or performance changes set the cooler currentPolicy to 
//                  NVAPI_COOLER_POLICY_MANUAL else set it to the current policy retrieved from the GetCoolerSettings API.
// PARAMETERS:      hPhysicalGPU(IN) - GPU selection.
//                  coolerIndex(IN)  - Explict cooler index selection.
//                  pCoolerLevels(IN) - Updated cooler level and cooler policy.
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_INVALID_ARGUMENT - pCoolerLevels is NULL
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//    NVAPI_INCOMPATIBLE_STRUCT_VERSION - the version of the INFO struct is not supported
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_SetCoolerLevels(NvPhysicalGpuHandle hPhysicalGpu, NvU32 coolerIndex, NV_GPU_SETCOOLER_LEVEL *pCoolerLevels);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_RestoreCoolerSettings
//
// DESCRIPTION:     Restore the modified cooler settings to NVIDIA defaults.
//
// PARAMETERS:      hPhysicalGPU(IN) - GPU selection.
//                  pCoolerIndex(IN) - Array containing absolute cooler indexes to restore. Pass NULL restore all coolers.
//                  CoolerCount - Number of coolers to restore.
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//    NVAPI_INCOMPATIBLE_STRUCT_VERSION - the version of the INFO struct is not supported
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_RestoreCoolerSettings(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pCoolerIndex, NvU32 coolerCount);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetCoolerPolicyTable
//
// DESCRIPTION:     Retrieves the table of cooler and policy levels for the selected policy. Supported only for NVAPI_COOLER_POLICY_PERF.
//
// PARAMETERS:      hPhysicalGPU(IN) - GPU selection.
//                  coolerIndex(IN) - cooler index selection.
//                  pCoolerTable(OUT) - Table of policy levels and associated cooler levels.
//                  count(OUT) - Count of the number of valid levels for the selected policy.
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//    NVAPI_INCOMPATIBLE_STRUCT_VERSION - the version of the INFO struct is not supported
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetCoolerPolicyTable(NvPhysicalGpuHandle hPhysicalGpu, NvU32 coolerIndex, NV_GPU_COOLER_POLICY_TABLE *pCoolerTable, NvU32 *count);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_SetCoolerPolicyTable
//
// DESCRIPTION:     Restore the modified cooler settings to NVIDIA defaults. Supported only for NVAPI_COOLER_POLICY_PERF.
//
// PARAMETERS:      hPhysicalGPU(IN) - GPU selection.
//                  coolerIndex(IN) - cooler index selection.
//                  pCoolerTable(IN) - Updated table of policy levels and associated cooler levels. Every non-zero policy level gets updated.
//                  count(IN) - Number of valid levels in the policy table.
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//    NVAPI_INCOMPATIBLE_STRUCT_VERSION - the version of the INFO struct is not supported
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_SetCoolerPolicyTable(NvPhysicalGpuHandle hPhysicalGpu, NvU32 coolerIndex, NV_GPU_COOLER_POLICY_TABLE *pCoolerTable, NvU32 count);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_RestoreCoolerPolicyTable
//
// DESCRIPTION:     Restores the perf table policy levels to the defaults.
//
// PARAMETERS:      hPhysicalGPU(IN) - GPU selection.
//                  coolerIndex(IN) - cooler index selection.
//                  pCoolerIndex(IN) - Array containing absolute cooler indexes to restore. Pass NULL restore all coolers.
//                  coolerCount - Number of coolers to restore.
//                  policy - restore for the selected policy
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_RestoreCoolerPolicyTable(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pCoolerIndex, NvU32 coolerCount, NV_COOLER_POLICY policy);

///////////////////////////////////////////////////////////////////////////////////
//  Thermal API
//  Provides ability to get temperature levels from the various thermal sensors associated with the GPU

#define NVAPI_MAX_THERMAL_SENSORS_PER_GPU 3

typedef enum 
{
    NVAPI_THERMAL_TARGET_NONE          = 0,
    NVAPI_THERMAL_TARGET_GPU           = 1,
    NVAPI_THERMAL_TARGET_MEMORY        = 2,
    NVAPI_THERMAL_TARGET_POWER_SUPPLY  = 4,
    NVAPI_THERMAL_TARGET_BOARD         = 8,
    NVAPI_THERMAL_TARGET_ALL           = 15,
    NVAPI_THERMAL_TARGET_UNKNOWN       = -1,
} NV_THERMAL_TARGET;

typedef enum
{
    NVAPI_THERMAL_CONTROLLER_NONE = 0,
    NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL,  
    NVAPI_THERMAL_CONTROLLER_ADM1032,
    NVAPI_THERMAL_CONTROLLER_MAX6649,       
    NVAPI_THERMAL_CONTROLLER_MAX1617,      
    NVAPI_THERMAL_CONTROLLER_LM99,      
    NVAPI_THERMAL_CONTROLLER_LM89,         
    NVAPI_THERMAL_CONTROLLER_LM64,         
    NVAPI_THERMAL_CONTROLLER_ADT7473,
    NVAPI_THERMAL_CONTROLLER_SBMAX6649,
    NVAPI_THERMAL_CONTROLLER_VBIOSEVT,  
    NVAPI_THERMAL_CONTROLLER_OS,    
    NVAPI_THERMAL_CONTROLLER_UNKNOWN = -1,
} NV_THERMAL_CONTROLLER;

typedef struct
{
    NvU32   version;                //structure version 
    NvU32   count;                  //number of associated thermal sensors with the selected GPU
    struct 
    {
        NV_THERMAL_CONTROLLER       controller;         //internal, ADM1032, MAX6649...
        NvU32                       defaultMinTemp;     //the min default temperature value of the thermal sensor in degrees centigrade 
        NvU32                       defaultMaxTemp;    //the max default temperature value of the thermal sensor in degrees centigrade 
        NvU32                       currentTemp;       //the current temperature value of the thermal sensor in degrees centigrade 
        NV_THERMAL_TARGET           target;             //thermal senor targetted @ GPU, memory, chipset, powersupply, canoas...
    } sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];

} NV_GPU_THERMAL_SETTINGS;

#define NV_GPU_THERMAL_SETTINGS_VER  MAKE_NVAPI_VERSION(NV_GPU_THERMAL_SETTINGS,1)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetThermalSettings
//
// DESCRIPTION:     Retrieves the thermal information of all thermal sensors or specific thermal sensor associated with the selected GPU.
//                  Thermal sensors are indexed 0 to NVAPI_MAX_THERMAL_SENSORS_PER_GPU-1.
//                  To retrieve specific thermal sensor info set the sensorIndex to the required thermal sensor index. 
//                  To retrieve info for all sensors set sensorIndex to NVAPI_THERMAL_TARGET_ALL. 
//
// PARAMETERS :     hPhysicalGPU(IN) - GPU selection.
//                  sensorIndex(IN)  - Explict thermal sensor index selection. 
//                  pThermalSettings(OUT) - Array of thermal settings.
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_INVALID_ARGUMENT - pThermalInfo is NULL
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//    NVAPI_INCOMPATIBLE_STRUCT_VERSION - the version of the INFO struct is not supported
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetThermalSettings(NvPhysicalGpuHandle hPhysicalGpu, NvU32 sensorIndex, NV_GPU_THERMAL_SETTINGS *pThermalSettings);

typedef enum _NV_DISPLAY_TV_FORMAT
{
    NV_DISPLAY_TV_FORMAT_NONE         = 0,
    NV_DISPLAY_TV_FORMAT_SD_NTSCM     = 0x00000001,
    NV_DISPLAY_TV_FORMAT_SD_NTSCJ     = 0x00000002,
    NV_DISPLAY_TV_FORMAT_SD_PALM      = 0x00000004,
    NV_DISPLAY_TV_FORMAT_SD_PALBDGH   = 0x00000008,
    NV_DISPLAY_TV_FORMAT_SD_PALN      = 0x00000010,
    NV_DISPLAY_TV_FORMAT_SD_PALNC     = 0x00000020,
    NV_DISPLAY_TV_FORMAT_SD_576i      = 0x00000100,
    NV_DISPLAY_TV_FORMAT_SD_480i      = 0x00000200,
    NV_DISPLAY_TV_FORMAT_ED_480p      = 0x00000400,
    NV_DISPLAY_TV_FORMAT_ED_576p      = 0x00000800,
    NV_DISPLAY_TV_FORMAT_HD_720p      = 0x00001000,
    NV_DISPLAY_TV_FORMAT_HD_1080i     = 0x00002000,
    NV_DISPLAY_TV_FORMAT_HD_1080p     = 0x00004000,
    NV_DISPLAY_TV_FORMAT_HD_720p50    = 0x00008000,
    NV_DISPLAY_TV_FORMAT_HD_1080p24   = 0x00010000,
    NV_DISPLAY_TV_FORMAT_HD_1080i50   = 0x00020000,

} NV_DISPLAY_TV_FORMAT;

///////////////////////////////////////////////////////////////////////////////////
//  I2C API
//  Provides ability to read or wirte data using I2C protocol.

#define NVAPI_MAX_SIZEOF_I2C_DATA_BUFFER 256
#define NVAPI_NO_PORTID_FOUND 5
#define NVAPI_DISPLAY_DEVICE_MASK_MAX 24

typedef struct 
{
    NvU32                   version;        //structure version 
    NvU32                   displayMask;    //the Display Mask of the concerned display
    NvU8                    bIsDDCPort;     //Flag indicating DDC port or a communication port
    NvU8                    i2cDevAddress;  //the I2C target device address
    NvU8*                   pbI2cRegAddress;//the I2C target register address  
    NvU32                   regAddrSize;    //the size in bytes of target register address  
    NvU8*                   pbData;         //The buffer of data which is to be read/written
    NvU32                   cbSize;         //The size of Data buffer to be read.
    NvU32                   i2cSpeed;       //The speed at which the transaction is be made(between 28kbps to 40kbps)
} NV_I2C_INFO;

#define NV_I2C_INFO_VER  MAKE_NVAPI_VERSION(NV_I2C_INFO,1)
/***********************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  NvAPI_I2CRead
//
// DESCRIPTION:    Read data buffer from I2C port
//
// PARAMETERS:     hPhysicalGPU(IN) - GPU selection.
//                 NV_I2C_INFO *pI2cInfo -The I2c data input stucture
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_I2CRead(NvPhysicalGpuHandle hPhysicalGpu, NV_I2C_INFO *pI2cInfo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  NvAPI_I2CWrite
//
// DESCRIPTION:    Writes data buffer to I2C port
//
// PARAMETERS:     hPhysicalGPU(IN) - GPU selection.
//                 NV_I2C_INFO *pI2cInfo -The I2c data input stucture
//
// RETURN STATUS: 
//    NVAPI_OK - completed request
//    NVAPI_ERROR - miscellaneous error occurred
//    NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_I2CWrite(NvPhysicalGpuHandle hPhysicalGpu, NV_I2C_INFO *pI2cInfo);

// END OF I2C API

 

typedef struct
{
    NvU32               version;        //structure version
    NvU32               vendorId;       //vendorId
    NvU32               deviceId;       //deviceId
    NvAPI_ShortString   szVendorName;   //vendor Name
    NvAPI_ShortString   szChipsetName;  //device Name
} NV_CHIPSET_INFO;

#define NV_CHIPSET_INFO_VER  MAKE_NVAPI_VERSION(NV_CHIPSET_INFO,1)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_SYS_GetChipSetInfo
//
//   DESCRIPTION: Returns information about the System's ChipSet
//
// RETURN STATUS: NVAPI_INVALID_ARGUMENT: pChipSetInfo is NULL
//                NVAPI_OK: *pChipSetInfo is now set
//                NVAPI_INCOMPATIBLE_STRUCT_VERSION - NV_CHIPSET_INFO version not compatible with driver
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_SYS_GetChipSetInfo(NV_CHIPSET_INFO *pChipSetInfo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_OGL_ExpertModeSet[Get]
//
//   DESCRIPTION: Configure OpenGL Expert Mode, an API usage feedback and
//                advice reporting mechanism. The effects of this call are
//                applied only to the current context, and are reset to the
//                defaults when the context is destroyed.
//
//                Note: These functions are valid only for the current OpenGL
//                      context. Calling these functions prior to creating a
//                      context and calling MakeCurrent with it will result
//                      in errors and undefined behavior.
//
//    PARAMETERS: expertDetailLevel Value which specifies the detail level in
//                                  the feedback stream. Set the detail level
//                                  to anything greater than zero to enable
//                                  output. A value of zero disables output.
//                                  Meaningful values range by 10s from 0-30.
//
//                expertReportMask  Mask made up of NVAPI_OGLEXPERT_REPORT bits,
//                                  this parameter specifies the areas of
//                                  fuctional interest.
//
//                expertOutputMask  Mask made up of NVAPI_OGLEXPERT_OUTPUT bits,
//                                  this parameter specifies the feedback output
//                                  location.
//
//                expertCallback    Used in conjunction with OUTPUT_TO_CALLBACK,
//                                  this is a simple callback function the user
//                                  may use to obtain the feedback stream. The
//                                  function will be called once per fully
//                                  qualified feedback stream extry.
//
// RETURN STATUS: NVAPI_API_NOT_INTIALIZED         : NVAPI not initialized
//                NVAPI_NVIDIA_DEVICE_NOT_FOUND    : no NVIDIA GPU found
//                NVAPI_OPENGL_CONTEXT_NOT_CURRENT : no NVIDIA OpenGL context
//                                                   which supports GLExpert
//                                                   has been made current
//                NVAPI_ERROR : OpenGL driver failed to load properly
//                NVAPI_OK    : success
//
///////////////////////////////////////////////////////////////////////////////
#define NVAPI_OGLEXPERT_REPORT_NONE          0x00000000
#define NVAPI_OGLEXPERT_REPORT_ERROR         0x00000001
#define NVAPI_OGLEXPERT_REPORT_SWFALLBACK    0x00000002
#define NVAPI_OGLEXPERT_REPORT_PROGRAM       0x00000004
#define NVAPI_OGLEXPERT_REPORT_VBO           0x00000008
#define NVAPI_OGLEXPERT_REPORT_FBO           0x00000010
#define NVAPI_OGLEXPERT_REPORT_ALL           0xFFFFFFFF

#define NVAPI_OGLEXPERT_OUTPUT_TO_NONE       0x00000000
#define NVAPI_OGLEXPERT_OUTPUT_TO_CONSOLE    0x00000001
#define NVAPI_OGLEXPERT_OUTPUT_TO_DEBUGGER   0x00000004
#define NVAPI_OGLEXPERT_OUTPUT_TO_CALLBACK   0x00000008
#define NVAPI_OGLEXPERT_OUTPUT_TO_ALL        0xFFFFFFFF

typedef void (* NVAPI_OGLEXPERT_CALLBACK)(unsigned int, unsigned int, const char *);

NVAPI_INTERFACE NvAPI_OGL_ExpertModeSet(NvU32 expertDetailLevel,
                                        NvU32 expertReportMask,
                                        NvU32 expertOutputMask,
                     NVAPI_OGLEXPERT_CALLBACK expertCallback);

NVAPI_INTERFACE NvAPI_OGL_ExpertModeGet(NvU32 *pExpertDetailLevel,
                                        NvU32 *pExpertReportMask,
                                        NvU32 *pExpertOutputMask,
                     NVAPI_OGLEXPERT_CALLBACK *pExpertCallback);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_OGL_ExpertModeDefaultsSet[Get]
//
//   DESCRIPTION: Configure OpenGL Expert Mode global defaults. These settings
//                apply to any OpenGL application which starts up after these
//                values are applied (i.e. these settings *do not* apply to
//                currently running applications).
//
//    PARAMETERS: expertDetailLevel Value which specifies the detail level in
//                                  the feedback stream. Set the detail level
//                                  to anything greater than zero to enable
//                                  output. A value of zero disables output.
//                                  Meaningful values range by 10s from 0-30.
//
//                expertReportMask  Mask made up of NVAPI_OGLEXPERT_REPORT bits,
//                                  this parameter specifies the areas of
//                                  fuctional interest.
//
//                expertOutputMask  Mask made up of NVAPI_OGLEXPERT_OUTPUT bits,
//                                  this parameter specifies the feedback output
//                                  location. Note that using OUTPUT_TO_CALLBACK
//                                  here is meaningless and has no effect, but
//                                  using it will not cause an error.
//
// RETURN STATUS: NVAPI_API_NOT_INTIALIZED or NVAPI_ERROR or NVAPI_OK
//
///////////////////////////////////////////////////////////////////////////////

NVAPI_INTERFACE NvAPI_OGL_ExpertModeDefaultsSet(NvU32 expertDetailLevel,
                                                NvU32 expertReportMask,
                                                NvU32 expertOutputMask);

NVAPI_INTERFACE NvAPI_OGL_ExpertModeDefaultsGet(NvU32 *pExpertDetailLevel,
                                                NvU32 *pExpertReportMask,
                                                NvU32 *pExpertOutputMask);

#define NVAPI_MAX_VIEW_TARGET  2

typedef enum _NV_TARGET_VIEW_MODE
{
    NV_VIEW_MODE_STANDARD  = 0,
    NV_VIEW_MODE_CLONE     = 1,
    NV_VIEW_MODE_HSPAN     = 2,
    NV_VIEW_MODE_VSPAN     = 3,
    NV_VIEW_MODE_DUALVIEW  = 4,
    NV_VIEW_MODE_MULTIVIEW = 5,
} NV_TARGET_VIEW_MODE;


// Following definitions are used in NvAPI_SetViewEx.
// Scaling modes
typedef enum _NV_SCALING
{
    NV_SCALING_DEFAULT          = 0,        // No change
    NV_SCALING_MONITOR_SCALING  = 1,
    NV_SCALING_ADAPTER_SCALING  = 2,
    NV_SCALING_CENTERED         = 3,
    NV_SCALING_ASPECT_SCALING   = 5,
    NV_SCALING_CUSTOMIZED       = 255       // For future use
} NV_SCALING;

// Rotate modes
typedef enum _NV_ROTATE
{
    NV_ROTATE_0           = 0,
    NV_ROTATE_90          = 1,
    NV_ROTATE_180         = 2,
    NV_ROTATE_270         = 3
} NV_ROTATE;

// Color formats
typedef enum _NV_FORMAT
{
    NV_FORMAT_UNKNOWN       =  0,       // unknown. Driver will choose one as following value.
    NV_FORMAT_P8            = 41,       // for 8bpp mode
    NV_FORMAT_R5G6B5        = 23,       // for 16bpp mode
    NV_FORMAT_A8R8G8B8      = 21,       // for 32bpp mode
    NV_FORMAT_A16B16G16R16F = 113       // for 64bpp(floating point) mode.
} NV_FORMAT;

// TV standard



///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_SetView
//
// DESCRIPTION:     This API lets caller to modify target display arrangement for selected source display handle in any of the nview modes.
//                  It also allows to modify or extend the source display in dualview mode.
//
// PARAMETERS:      hNvDisplay(IN) - NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//                  pTargetInfo(IN) - Pointer to array of NV_VIEW_TARGET_INFO, specifying device properties in this view.
//                                    The first device entry in the array is the physical primary.
//                                    The device entry with the lowest source id is the desktop primary.
//                  targetCount(IN) - Count of target devices specified in pTargetInfo.
//                  targetView(IN) - Target view selected from NV_TARGET_VIEW_MODE.
//
// RETURN STATUS: 
//                  NVAPI_OK - completed request
//                  NVAPI_ERROR - miscellaneous error occurred
//                  NVAPI_INVALID_ARGUMENT: Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    NvU32 version;     // (IN) structure version
    NvU32 count;       // (IN) target count
    struct 
    {
        NvU32 deviceMask;  // (IN/OUT) device mask
        NvU32 sourceId;    // (IN/OUT) source id
        NvU32 bPrimary:1;  // (OUT) Indicates if this is the desktop primary
    } target[NVAPI_MAX_VIEW_TARGET];
} NV_VIEW_TARGET_INFO; 

#define NV_VIEW_TARGET_INFO_VER  MAKE_NVAPI_VERSION(NV_VIEW_TARGET_INFO,1)

NVAPI_INTERFACE NvAPI_SetView(NvDisplayHandle hNvDisplay, NV_VIEW_TARGET_INFO *pTargetInfo, NV_TARGET_VIEW_MODE targetView);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetView
//
// DESCRIPTION:     This API lets caller retrieve the target display arrangement for selected source display handle.
//
// PARAMETERS:      hNvDisplay(IN) - NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//                  pTargetInfo(OUT) - User allocated storage to retrieve an array of  NV_VIEW_TARGET_INFO. Can be NULL to retrieve the targetCount.
//                  targetMaskCount(IN/OUT) - Count of target device mask specified in pTargetMask.
//                  targetView(OUT) - Target view selected from NV_TARGET_VIEW_MODE.
//
// RETURN STATUS: 
//                  NVAPI_OK - completed request
//                  NVAPI_ERROR - miscellaneous error occurred
//                  NVAPI_INVALID_ARGUMENT: Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetView(NvDisplayHandle hNvDisplay, NV_VIEW_TARGET_INFO *pTargets, NvU32 *pTargetMaskCount, NV_TARGET_VIEW_MODE *pTargetView);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_SetViewEx
//
// DESCRIPTION:     This API lets caller to modify the display arrangement for selected source display handle in any of the nview modes.
//                  It also allows to modify or extend the source display in dualview mode.
//
// PARAMETERS:      hNvDisplay(IN) - NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//                  pPathInfo(IN)  - Pointer to array of NV_VIEW_PATH_INFO, specifying device properties in this view.
//                                    The first device entry in the array is the physical primary.
//                                    The device entry with the lowest source id is the desktop primary.
//                  pathCount(IN)  - Count of paths specified in pPathInfo.
//                  displayView(IN)- Display view selected from NV_TARGET_VIEW_MODE.
//
// RETURN STATUS: 
//                  NVAPI_OK - completed request
//                  NVAPI_ERROR - miscellaneous error occurred
//                  NVAPI_INVALID_ARGUMENT: Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////

#define NVAPI_MAX_DISPLAY_PATH  NVAPI_MAX_VIEW_TARGET

typedef struct
{
    NvU32 version;     // (IN) structure version
    NvU32 count;       // (IN) path count
    struct 
    {
        NvU32                   deviceMask;     // (IN) device mask
        NvU32                   sourceId;       // (IN) source id
        NvU32                   bPrimary:1;     // (OUT) Indicates if this is the desktop primary
        NV_GPU_CONNECTOR_TYPE   connector;      // (IN) Specify connector type. For TV only.
        
        // source mode information
        NvU32                   width;          // (IN) width of the mode
        NvU32                   height;         // (IN) height of the mode
        NvU32                   depth;          // (IN) depth of the mode
        NV_FORMAT               colorFormat;    //      color format if needs to specify. Not used now.
        
        //rotation setting of the mode
        NV_ROTATE               rotation;       // (IN) rotation setting.
      
        // the scaling mode
        NV_SCALING              scaling;        // (IN) scaling setting

        // Timing info
        NvU32                   refreshRate;    // (IN) refresh rate of the mode
        NvU32                   interlaced:1;   // (IN) interlaced mode flag

        NV_DISPLAY_TV_FORMAT    tvFormat;       // (IN) Only make sence for HDTV. For SDTV, we should follow the setting.
    } path[NVAPI_MAX_DISPLAY_PATH];
} NV_DISPLAY_PATH_INFO; 

#define NV_DISPLAY_PATH_INFO_VER  MAKE_NVAPI_VERSION(NV_DISPLAY_PATH_INFO,1)

NVAPI_INTERFACE NvAPI_SetViewEx(NvDisplayHandle hNvDisplay, NV_DISPLAY_PATH_INFO *pPathInfo, NV_TARGET_VIEW_MODE displayView);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetSupportedViews
//
// DESCRIPTION:     This API lets caller enumerate all the supported NVIDIA display views - nview and dualview modes.
//
// PARAMETERS:      hNvDisplay(IN) - NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//                  pTargetViews(OUT) - Array of supported views. Can be NULL to retrieve the pViewCount first.
//                  pViewCount(IN/OUT) - Count of supported views.
//
// RETURN STATUS: 
//                  NVAPI_OK - completed request
//                  NVAPI_ERROR - miscellaneous error occurred
//                  NVAPI_INVALID_ARGUMENT: Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetSupportedViews(NvDisplayHandle hNvDisplay, NV_TARGET_VIEW_MODE *pTargetViews, NvU32 *pViewCount);
                                         
#ifdef __cplusplus
}; //extern "C" {
#endif

#pragma pack(pop)

#endif // _NVAPI_H
