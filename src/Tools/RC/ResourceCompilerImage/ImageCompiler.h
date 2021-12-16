#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>
#include "IConvertor.h"							// IConvertor
#include "ImageProperties.h"				// CImageProperties
#include "SimpleBitmap.h"						// CSimpleBitmap<>
#include "IPTCHeader.h"
#include "weightfilterset.h"				// CWeightFilterSet
#include "angularcubemapfilter.h"		// CAngularFilterSet
#include "AtiCubemapGen/CCubeMapProcessor.h"
#include "ResourceCompiler.h"

#include "GenerationProgress.h"

enum EConfigPriority; 
struct ImageObject;
class CFloat4Image;
struct tiff;
class CTextureFallbackSystem;
class CImageUserDialog;

class CImageCompiler : public ICompiler
{
	friend class CGenerationProgress;

public:

	//! constructor
	CImageCompiler(CTextureFallbackSystem* pTextureFallbackSystem);
	//! destructor
	~CImageCompiler();

	//!
	bool Init( HWND inhWnd );
	//!
	void DeInit();
	// Arguments:
	//	lpszExtension - must not be 0, must be lower case
	bool LoadInput( const char *lpszPathName, const char *lpszExtension, const bool bLoadConfig=true );

	// Arguments:
	//   pOutput - must not be 0
	//   szType must not be 0, for log output
	bool SaveOutput( ImageObject * &pOutput, const char *szType, const char *lpszPathName );

	// used to pass existing image as input
	// Arguments:
	//   pInput - data will be stored and freed automatically
	void GetInputFromMemory( ImageObject *pInput );

	// used to pass resulting image
	ImageObject *GetOutputAndRelease();

	//
	void FreeTexture();

	// Arguments:
	//   pInputImage - must not be 0
	//   fGamma >0, 1.0 means no gamma correction is applied
	ImageObject *CreateMipMaps( const ImageObject *pInputImage, uint32 indwReduceResolution, const bool inbRemoveMips, const bool bRenormalize,
		const float fGamma );

	// Arguments:
	//   pInputImage - must not be 0, aspect 6:1
	//   fGamma >0, 1.0 means no gamma correction is applied
	//	 cubemap filter params - see ATICubemapGen lib
	ImageObject *CreateCubemapMipMaps( ImageObject *pInputImage, const SCubemapFilterParams& params, uint32 indwReduceResolution, const float fGamma );

	ImageObject *CreateHighPass( ImageObject *pInputImage, uint32 dwMipDown );

	// only applies the conversion if the image is 3Dc
	// Arguments:
	//   pInAndOutImage - must not be 0
	// applied to attached images recursively
	void Convert3DcToDXT5_InPlace( ImageObject *pInAndOutImage );

	// Arguments:
	//   pInAndOutImage - must not be 0
	// applied to attached images recursively
	void PreSwizzleTexturePS3like_InPlace( ImageObject *pInAndOutImage );

	//! used for sRGB textures to fix the issue with XBox 360 linear gamma approximation
	ImageObject *ConvertGammaToXBox360( ImageObject *inSrc );

	//! can be used to compress
	ImageObject *ConvertFormat( ImageObject *inSrc, D3DFORMAT fmtTo );

	//! can be used to convert HDR textures to lossy LDR RGBK representation
	ImageObject *CompressToRGBK( ImageObject *inSrc, float brightnessMultiplier );

	//! /param infOffsetX
	//! /param infOffsetY
	//! /param iniScale64 16=1:1, bigger values magnify more
	//! /return true=blit was successful, false otherwise
	bool BlitTo( HWND inHWND, RECT &inRect, const float infOffsetX, const float infOffsetY, const int iniScale64, const bool inbOrig );

	// text for the info below the preview
	string GetInfoStringUI( const bool inbOrig );

	// text for file statistics (not multiline, tab separated)
	string GetDestInfoString();

	//!
	//! /return true=x and y is bound to 0.5 because the texture is smaller than the preview area, false otherwise
	bool ClampBlitOffset( const int iniWidth, const int iniHeight, float &inoutfX, float &inoutfY, const int iniScale64 ) const;

	// useful for error printout
	static char *GetNameFromD3DFormat( D3DFORMAT inFormat );

	//! 
	uint32 GetInputImageWidth() const;

	//!
	uint32 GetInputImageHeight() const;

	// run with the user specified user properties in m_Prop
	// Arguments:
	//   inbSave - true=save the result, false=update only the internal structures for previewing
	//   szExtendFileName - e.g. "_DDN" or "_DDNDIF" is inserted before the file extension
	// Return:
	//   return true=success, false otherwise(e.g. compression failed because of non power of two)
	bool RunWithProperties( bool inbSave, const char *szExtendFileName=0 );

	// does not harm if this is called multiple times, can affect the preset (important to know for UI updates)
	// is also called by RunWithProperties()
	void AutoOptimize();

	// 
	void AutoPreset();

	//! set the stored properties to the current file and save it
	//! @return true=success, false otherwise
	bool UpdateAndSaveConfig();

	//
	bool InputHasAlpha() const { return m_bInputUsesAlpha; }
	//
	bool InputUsesDenormalizedNormals() const { return m_bInputUsesDenormalizedNormals; }
	//
	void SetCC( ConvertContext *pCC );

	// \return is ddndif processing enabled and the file extension is _ddndif
//	bool IsDDNDIFProcessing( ImageObject *pCurrentImage ) const;

	// interface ICompiler ----------------------------------------------------

	virtual void Release();
	virtual bool Process(ConvertContext &cc);
	virtual void ConstructAndSetOutputFile(ConvertContext& cc);
	virtual void GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const;

	// -------------------------------------------------------------------------

	void SetPresetSettings();

	CImageProperties          m_Props;                          // user settings
	ConvertContext *          m_pCC;                            // pointer to the object give to Process (is 0 outside of the call)

	CGenerationProgress       m_Progress;

private: // ------------------------------------------------------------------

	D3DPRESENT_PARAMETERS     m_presentParams;                  // presentation parameters used on device creation and reset
	LPDIRECT3DDEVICE9         m_pd3ddev;                        //
	LPDIRECT3D9               m_pd3d;                           //

	ImageObject *             m_pInputImage;                    // input image
	bool                      m_bInputUsesAlpha;                // affected by ClearInputImageFlags(), only valid if m_pInputImage!=0
	bool                      m_bInputUsesDenormalizedNormals;  // affected by ClearInputImageFlags(), only valid if m_pInputImage!=0 and in
	ImageObject *             m_pFinalImage;                    // changed destination image

	int                       m_iOrigPixelFormatNo;             //
	HWND                      m_hWnd;                           // DirectX needs a window to attach to
	bool                      m_bInitialized;                   // True when converter initialized.
	float                     m_fTimeProfiled;                  // in seconds

	CTextureFallbackSystem*   m_pTextureFallbackSystem;         //

	CIPTCHeader               m_iptcHeader;                     // Stores the IPTC (ie caption, author) metadata - used to store settings.

	// mips filtering framework
	CWeightFilterSet          m_mipFilterset;                   // one mip down
	float                     m_fCurrentMipFilterSharpness;     // to detect if we need to compute the filterset
	CWeightFilterSet          m_mipFilterset2x2;                // one mip down, 2x2 filter kernel
	CWeightFilterSet          m_alphaCoverageFilterset;         // for alpha coverage filtering
	bool                      m_bValidAlphaCoverageFilterset;   // alpha coverage filter flag

	// cubemap processing
	CCubeMapProcessor         m_AtiCubemanGen;                  // for cubemap filtering
	//CAngularFilterSet         m_cubemapAngularFilterset;      // for cubemap filtering

	CImageUserDialog *        m_pImageUserDialog;               //backlink to user dialog for preview calculation handling
	bool                      m_bInternalPreview;               //indicates whether currently an internal preview is calculated (true) or results are stored (false)

	// does export-time checks
	void ValidateSourceImage(ImageObject *pSourceImage);

	//
	ImageObject* ChangeFormatWithSquishToDXT5n( ImageObject *pInputImage, bool highQuality );

	//
	void SwizzleRedToGreenAndGreenToAlpha_InPlace(ImageObject* pInputImage);

	//!
	HRESULT ChangeFormatWithDirectX( ImageObject *pInputImage, D3DFORMAT fmtTo, ImageObject *&pNewImage, 
		const uint32 indwReduceResolution );

	//
	HRESULT BltAllLevels( ImageObject *pInputImage, D3DCUBEMAP_FACES FaceType, LPDIRECT3DBASETEXTURE9 ptexSrc, ImageObject *pNewImage,
		const uint32 indwReduceResolution, D3DFORMAT fmtTo );

	// convert to 8bit paletized texture
	HRESULT PaletteQuantize(LPDIRECT3DSURFACE9 psurfSrc, LPDIRECT3DSURFACE9 psurfDest);

	// only called by ConvertFormat()
	ImageObject *ConvertFormatWithSpecifiedCompressor( ImageObject *inSrc, D3DFORMAT fmtTo, CImageProperties::EGlobalCompressor compressor );

	// for test/debug only
	bool test_DXT5Compression(ImageObject *inSrc);

	// Arguments:
	//   inSrc - must not be 0
	class CRAWImage *CopyToNewCRAWImage( ImageObject *inSrc );

	// Convert between RGB8, RGBA8, RGBX8, RGBF32, RGBAF32, RGBXF32
	ImageObject* ConvertBetweenSimpleFormats( ImageObject *inSrc, D3DFORMAT dstDstFormat );

	// Convert to Xbox 360 specific CXT1 normal map format
	ImageObject* ConvertARGB8ToXbox360CTX1( ImageObject *inSrc );

	// depending on extension this calls a different loading function
	// Arguments:
	//   lpszExtension - extension passed as parameter to provide processing of files with wrong extension
	//   pFileConfig - can be 0, pointer where it can store the file specific config  
	ImageObject *LoadImage( const char *lpszPathName, const char *lpszExtension, IConfig *pFileConfig );

	// Arguments:
	//   pFileConfig - can be 0, pointer where it can store the file specific config  
	ImageObject *LoadUsingTIFLoader( const char *lpszPathName, IConfig *pFileConfig );

	// loads simple RAWImage from BGR8/BGRA8 tiff source raster
	ImageObject *Load8BitImageFromTIFF(struct tiff* tif);

	// loads simple FloatImage from BGRA32f HDR tiff source raster
	ImageObject *Load16BitHDRImageFromTIFF(struct tiff* tif);

	bool IsFinalPixelFormatCompressed() const;

	bool IsFinalPixelFormatValidForNormalmaps() const;

	bool IsPointOutsideOfRectangle(const uint32 dwX, const uint32 dwY, const RECT& rectangle) const;

	//!
	void ClearInputImageFlags();

	// Arguments:
	//   pImage - must not be 0
	// Returns:
	//   success
	bool CreateTextureFallback( ImageObject *pImage, const uint32 dwReduceMips, const uint32 dwMaxMips );

	// Returns:
	//   size in bytes, 0 if pImage is 0
	static uint32 CalcTextureMemory( const ImageObject *pImage, const uint32 dwImageScale=1 );

	// Arguments:
	//   psurf - has to be in the format A8R8G8B8
	// Returns:
	//   DirectX image
	ImageObject *GenerateSpecialPreviewToARGB( ImageObject *pInputImage, const EPreviewMode ePreview ) const;

	//
	IDirect3DTexture9 *CopyP8ToXRGB(LPDIRECT3DBASETEXTURE9 texp8);

	// Arguments:
	//   fReferenceAlphaCoverage - -1 meanst we don't know the value yet
//	void MaintainAlphaCoverage( CSimpleBitmap<Vec4> &inoutBitmap, float &fReferenceAlphaCoverage );

	void MaintainAlphaCoverage( CSimpleBitmap<Vec4> &inoutBitmap, const uint32 dwWidth, const uint32 dwHeight );

	//
	// with hard comparison of the alphatest
	float ComputeAlphaCoverage( const CSimpleBitmap<Vec4> &inBitmap, const uint32 dwWidth, const uint32 dwHeight, const float fRefAlpha );

	// with soft comparison
//	float ComputeWeightedAlphaCoverage( const CSimpleBitmap<Vec4> &inBitmap );

	bool LoadConfigFile( const EConfigPriority ePriMask );
	
	// can be called more than once but then it just returns true
	bool InitDirectX();

	//! /return 
	static uint32 _CalcMipCount( const uint32 indwWidth, const uint32 indwHeight, const bool bDXT=false, const bool bCubemap = false );

	// Return:
	//   if new image is valid
	bool ProceedToNextPass( ImageObject * &pCurrentImage, ImageObject *pNewImage );

	// Arguments:
	//   dwMinAlpha - 0..255
	ImageObject *MinAlpha( ImageObject &rFlatARGB, const uint32 dwMinAlpha ) const;
	//
	// Arguments:
	//   bApplyRangeAdjustment - true=-1..1 -> 0..1, false leaved in -1..1
	//   bValueFromAlpha - true=from alpha, false=value from RGB luminance
	ImageObject *Bump2Normal( const CBumpProperties &rProperties, ImageObject *pFlatFloatInput,
		const bool bApplyRangeAdjustment, const bool bValueFromAlpha );
	// both input need to be in range 0..1
	// Arguments:
	//   pInOut - must not be 0
	//   pBump - must not be 0
	// Returns:
	//   resulting image
	ImageObject *CombineNormalMaps( ImageObject *pInOut, ImageObject *pBump ) const;
	// input need to be in range 0..1
	// Arguments:
	//   pIn - must not be 0
	// Returns:
	//   resulting image
	ImageObject *ComputeLuminanceAndPutItIntoAlphaChannel( ImageObject *pIn ) const;
	//
	// might convert the input format L8 (luminance only) if this is possible without quality loss
	bool IsPerfectGreyscale( ImageObject * &rInput ) const;
	//
	void UserMessage( const char *szMessage );

	// e.g. ATI 3dc compression
	// Arguments:
	//   inSrc - must not be 0
	ImageObject *ConvertFormatWithATI( ImageObject *inSrc, const D3DFORMAT format );

	// Arguments:
	//   pInput - must not be 0, image might become converted
	//   szFilename - must not be 0
	// Returns:
	//   success
	bool SaveAsTIFF( ImageObject * &pInput, const char *szFilename );

	string CImageCompiler::GetSpecialInstructionsFromTIFF( tiff *tif );

	void DDNDIFProcessing( ImageObject *pFloatARGB ) const;
};


#define DXERRORCHECK(cmt,exp) { HRESULT _hr = (exp); /*assert(!_hr);*/ if(_hr) RCLogError("'%s' DX ERROR: %s", cmt,DXGetErrorString(_hr)); }
