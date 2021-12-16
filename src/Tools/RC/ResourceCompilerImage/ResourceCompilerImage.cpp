// ResourceCompilerImage.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <assert.h>
#include "ResourceCompilerImage.h"
#include "IResCompiler.h"
#include "ImageConvertor.h"							// CImageConvertor
#include "SRFCompiler.h"								// CSRFCompiler
#include "Streaming\TextureSplitter.h"	// CTextureSplitter

// Must be included only once in DLL module.
#include <platform_implRC.h>


HMODULE g_hInst;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hInst = (HMODULE)hModule;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}


// This is an example of an exported function.
void __stdcall RegisterConvertors( IResourceCompiler *pRC )
{
	SetRCLog(pRC->GetIRCLog());

	// image formats
	{
		CImageConvertor *pImageConv=new CImageConvertor();

		pRC->RegisterConvertor(pImageConv);

		pRC->RegisterKey("userdialog","[SRF/TIF] 0/1 to show the userdialog for the ResourceCompilerImage");
		pRC->RegisterKey("preset","[TIF] e.g. \"NormalmapHigh\" to overwrite the preset");
		pRC->RegisterKey("reduce","[TIF] 0=non /1=half resolution /2=quarter resolution");
		pRC->RegisterKey("fallback","[TIF] TextureFallbackFile.dat is updated with down sampled versions");
		pRC->RegisterKey("userdialogcustom","[TIF] 0/1=enabled additional user controls");
		pRC->RegisterKey("mipmaps","[TIF] 0/1");
		pRC->RegisterKey("mipgamma",
			"[TIF] 1=on(better for diffuse textures)\n"
			"or 0=off(for non RGB textures)");
		pRC->RegisterKey("mipnormalize","[TIF] 0/1=useful for normalmaps");
		pRC->RegisterKey("pixelformat","[TIF] e.g. 3DC, DXT1, X8R8G8B8");
		pRC->RegisterKey("previewformat","[TIF] format for the preview panel to be displayed in if it doesnt exist pixelformat should be used");
		pRC->RegisterKey("bumpblur","[TIF]");
		pRC->RegisterKey("bumptype","[TIF]");
		pRC->RegisterKey("bumpstrength","[TIF]");
		pRC->RegisterKey("decal","[TIF] 0/1=use special border clamp behavior on alpha test, 0(default)");
		pRC->RegisterKey("lumintoalpha","[TIF] 1=compute luminance by using RGB values and put it into alpha channel");
		pRC->RegisterKey("globalcompressor","[TIF] 0=DirectX, 1=NVDXTLib(default), 2=ATI, 3=TIF, 5=NVSquish, 6=NVSquishFast");
		pRC->RegisterKey("outputuncompressed","[TIF] output uncompressed files instead of DXTx compressed files");
		pRC->RegisterKey("globalmipblur","[TIF] default blur is is 2 (->4x4 kernel)");
		pRC->RegisterKey("mipgentype","[TIF] 0=use default filter, 1=use 2x2 filter");
		pRC->RegisterKey("globalgamma","[TIF] linear math requires monitor gamma (default 2.2)");
		pRC->RegisterKey("autooptimize","[TIF] 0/1=automatic adjusting to reasonable settings");
		pRC->RegisterKey("ser","[TIF] 0/1=supress reduce resolution during loading, 0(default)");
		pRC->RegisterKey("mipbordercolor",
			"[TIF] if set, all mip borders during the mipmap\n"
			"      generation become this hex value AARRGGBB");
		pRC->RegisterKey("srgb","[TIF] 1=set SRGBLookup flag in the DDS header, 1(default)\nunprocessed DDS have this off (linear)");

		pRC->RegisterKey("rgbk","[TIF] 1=enables compression pass of HDR images to RGBK format, 0-default");
		pRC->RegisterKey("cm","[TIF] 1=enables cubemap TIF processing(TIF needs to have 6:1 aspect), 0 - default");
		pRC->RegisterKey("cm_ftype","[TIF] cubemap angular filter type: gaussian, cone, cosine, disc");
		pRC->RegisterKey("cm_fangle","[TIF] base filter angle for cubemap filtering(degrees), 0 - disabled");
		pRC->RegisterKey("cm_fmipangle","[TIF] initial mip filter angle for cubemap filtering(degrees), 0 - disabled");
		pRC->RegisterKey("cm_fmipslope","[TIF] mip filter angle multiplier for cubemap filtering, 1 - default");
		pRC->RegisterKey("cm_edgefixup","[TIF] cubemap edge fix-up width, 0 - disabled");

		pRC->RegisterKey("autooptimizefile","[TIF] 0/1=auto optimize for this file, 1(default)");
		pRC->RegisterKey("minalpha","[TIF] 0..255 to limit alpha value, (can prevent NAN in shader)");
		pRC->RegisterKey("powof2","[TIF] 1=texture needs to be power of 2 in width and height, 0 otherwise");
		pRC->RegisterKey("detectL8","[TIF] if R=G=B and no alpha then use L8 (luminance)");
		pRC->RegisterKey("mc","[TIF] maintain alpha coverage in mipmaps (experimental)");
		pRC->RegisterKey("ms","[TIF] 0=no sharpening .. 100=full sharpening");
		pRC->RegisterKey("M0","[TIF] adjust mipalpha, 0..50=normal..100");
		pRC->RegisterKey("M1","[TIF] adjust mipalpha, 0..50=normal..100");
		pRC->RegisterKey("M2","[TIF] adjust mipalpha, 0..50=normal..100");
		pRC->RegisterKey("M3","[TIF] adjust mipalpha, 0..50=normal..100");
		pRC->RegisterKey("M4","[TIF] adjust mipalpha, 0..50=normal..100");
		pRC->RegisterKey("M5","[TIF] adjust mipalpha, 0..50=normal..100");
		pRC->RegisterKey("nooutput","[TIF] supress output");
		pRC->RegisterKey("M","[TIF] compact version of M0,M1,...");
		pRC->RegisterKey("overwritefilename","[TIF] overwrite given input filename (destination not affected)");
		pRC->RegisterKey("overwriteextension","[TIF] overwrite the file extension");
		pRC->RegisterKey("globalreduce","[TIF] [0..[ to remove the top mipmap levels");
		pRC->RegisterKey("bumptype_a","[TIF] 0=none, 1=Gauss, 2=GaussAlpha");
		pRC->RegisterKey("bumpstrength_a","[TIF] -1000.0 .. 1000.0");
		pRC->RegisterKey("bumpblur_a","[TIF] 0 .. 10.0");
		pRC->RegisterKey("ext_tif","[TIF] ignore or copy tif files");	
		pRC->RegisterKey("ext_dds","[TIF] ignore or copy dds files");	

		pRC->RegisterKey("dont_split","[DDS] don't split the file for streaming layout");	

		pRC->RegisterKey("colormodel","[TIF] 0=rgb (default), 1=cie");
		pRC->RegisterKey("colorchart","[TIF] extract color chart from image, 0=off (default), 1=on (3d lut)");
		pRC->RegisterKey("previewstretched","[TIF] preview destination image with size of source image, 0=off, 1=on (default)");

		pRC->RegisterKey("Emulate3Dc",
			"[TIF] export dxt5 instead of 3Dc (useful for PS3)\n"
			"      0=off\n"
			"      1=low quality (converts image to 3DC, then to DXT5), obsolete\n"
			"      2=high quality (converts image directly to DXT5)");
		pRC->RegisterKey("SwizzleTex","[TIF] 0=off, 1=PS3 like pre-swizzled textures");

		pRC->RegisterKey("highpass",
			"[TIF] 0=off, >0=defines which mip level is subtracted when applying the\n"
			"      [cheap] high pass filter - this prepares assets to\n"
			"      allow this in the shader");

		// SRF processing
		pRC->RegisterKey("ext_srf","[SRF] ignore srf files");
		pRC->RegisterKey("space","[SRF] 3d the vectors are transformed to, can be 'tangent' or 'object'");
		pRC->RegisterKey("normaloutput","[SRF] normal vector format format, can be 'R8G8B8' or 'R8G8'");
		pRC->RegisterKey("outputaccess","[SRF] output accessibility 0=off/1=on");
		pRC->RegisterKey("outputoccdir","[SRF] output unoccluded area direction 0=off/1=on");
		pRC->RegisterKey("outputhorizonmap","[SRF] output horizon map 0=off/1=on");
		pRC->RegisterKey("outputcombo",
			"[SRF] output combo map (high poly materials encoded as colors)\n"
			"      0=off/1=on");

		pRC->RegisterKey("autopreset",
			"[TIF] depending on some rules defined in C++ (e.g. path) the preset may be overwritten\n"
			"      this is mostly useful to support bigger amounts of older assets"
			"      0=off/1=on (default)");

		pRC->RegisterKey("displaceoutput","[SRF] output displacement map 0=off/1=on");
		pRC->RegisterKey("bumpname","[SRF] name of bumpmap image to apply during SRF processing (.tif)");
		pRC->RegisterKey("expand","[SRF] size of the border expand");
	}

	// SRF format
	{
		CSRFCompiler *pSRFComp=new CSRFCompiler();

		pRC->RegisterConvertor(pSRFComp);
//		pRC->RegisterKey();
	}

	// Texture splitter
	{
		CTextureSplitter *pTextureSplitter = new CTextureSplitter();
		pRC->RegisterConvertor(pTextureSplitter);
	}
}

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID DECLSPEC_SELECTANY name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
DEFINE_GUID(IID_IDirect3DTexture9, 0x85c31227, 0x3de5, 0x4f00, 0x9b, 0x3a, 0xf1, 0x1a, 0xc3, 0x8c, 0x18, 0xb5);
