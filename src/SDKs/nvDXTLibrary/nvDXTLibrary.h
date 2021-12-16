#pragma once

#ifdef NVDXTLIBRARY_EXPORTS
#define NVDXTLIBRARY_API __declspec(dllexport)
#else
#define NVDXTLIBRARY_API __declspec(dllimport)
#endif

#ifdef NVDXTLIBRARY_EXPORTS
#ifdef _WIN32
# ifdef _WIN64
#  pragma comment ( lib, "lib/nvDXTlibMTDLL.vc9.x64.lib" )
# else
#  pragma comment ( lib, "lib/nvDXTlibMTDLL.vc9.lib" )
# endif
#endif
#endif

#include "dxtlib.h"

NVDXTLIBRARY_API NV_ERROR_CODE nvDXTdecompress_Wrapped(
	nvImageContainer & imageData,
	nvPixelFormat pf,        
	int readMIPMapCount,
	DXTReadCallback fileReadRoutine,
	void * userData);

NVDXTLIBRARY_API NV_ERROR_CODE nvDXTcompress_Wrapped(const unsigned char * srcImage,
	size_t width,
	size_t height,
	size_t byte_pitch,
	nvPixelOrder pixelOrder,
	nvCompressionOptions * options,
	DXTWriteCallback fileWriteRoutine,  // call to .dds write routine
	const RECT * rect = NULL);