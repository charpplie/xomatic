#include "stdafx.h"

#include "nvDXTLibrary.h"


using namespace nvDDS;

NVDXTLIBRARY_API NV_ERROR_CODE nvDXTdecompress_Wrapped(
	nvImageContainer & imageData,
	nvPixelFormat pf,        
	int readMIPMapCount,
	DXTReadCallback fileReadRoutine,
	void * userData)
{
	return nvDXTdecompress(imageData, pf, readMIPMapCount, fileReadRoutine, userData);
}

NVDXTLIBRARY_API NV_ERROR_CODE nvDXTcompress_Wrapped(const unsigned char * srcImage,
									   size_t width,
									   size_t height,
									   size_t byte_pitch,
									   nvPixelOrder pixelOrder,
									   nvCompressionOptions * options,
									   DXTWriteCallback fileWriteRoutine,  // call to .dds write routine
									   const RECT * rect)
{
	return nvDXTcompress(srcImage, width, height, byte_pitch, pixelOrder, options, fileWriteRoutine, rect);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

