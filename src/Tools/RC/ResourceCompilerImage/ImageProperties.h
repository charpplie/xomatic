#pragma once

#include "Cry_Math.h"									// 
#include "PixelFormats.h"							// CPixelFormats
#include "IConfig.h"									// IConfig
#include "ICfgFile.h"									// ICfgFile
#include "ConvertContext.h"						// ConvertContext
#include "IRCLog.h"										// IRCLog



// needs to be in the same order as in IDC_PREVIEWMODE
enum EPreviewMode
{
	ePM_RGB=0,						// normal
	ePM_AAA=1,						// replicate the alpha channel as greyscale value
	ePM_RGBA=2,						// alpha to fade between RGB and background
	ePM_NormalLength=3,		// replicate length of the normal as greyscale value
	ePM_RGBmulA=4,				// RGB * Alpha (good to fake HDR)
	ePM_AlphaTest=5,			// Shows the Alpha Channel visualized with an AlpheTest of 0.5
	ePM_mCIE2RGB=6,				// mCIE
};

enum EHDRDivider
{
	ehdDefaultHDRDevider = 16,
};


class CBumpProperties
{
public:

	// Arguments:
	//   bAlphaAsBump true=m_AlphaAsBump, false=m_Bump2Normal
	CBumpProperties( const bool bAlphaAsBump ) 
		:m_pCC(0), m_bAlphaAsBump(bAlphaAsBump)
	{
	}

	// Arguments:
	//  pCC - can be 0
	void SetCC( ConvertContext *pCC )
	{
		m_pCC=pCC;
	}

	// ----------------------------------------

	// Arguments:
	//  fValue - 0 .. 10.0
	void SetBumpBlurAmount( const float fValue )
	{
		if(GetBumpBlurAmount()==fValue)
			return;			
		char str[80];	sprintf(str,"%f",fValue);
		m_pCC->config->Set(eCP_PriorityFile,m_bAlphaAsBump?"bumpblur_a":"bumpblur",str);
	}
	// Returns:
	//  0 .. 10.0
	float GetBumpBlurAmount() const
	{
		float fRet=0.0f;		// default
		m_pCC->config->Get(m_bAlphaAsBump?"bumpblur_a":"bumpblur",fRet);
		fRet=max(fRet,0.0f);
		return fRet;
	}
	void EnableWindowBumpBlurAmount( HWND hWnd ) const
	{
		string Value;	
		bool bSet = m_pCC->config->Get(m_bAlphaAsBump?"bumpblur_a":"bumpblur",Value,eCP_PriorityNotFile);
		if(!GetBumpToNormalFilter())
			bSet=true;
		EnableWindow(hWnd,!bSet);
	}

	// ----------------------------------------

	// Arguments:
	//  fValue - -1000.0 .. 1000.0
	void SetBumpStrengthAmount( const float fValue )
	{
		if(GetBumpStrengthAmount()==fValue)
			return;			
		char str[80];	sprintf(str,"%f",fValue);
		m_pCC->config->Set(eCP_PriorityFile,m_bAlphaAsBump?"bumpstrength_a":"bumpstrength",str);
	}
	// Returns:
	//  -1000.0 .. 1000.0
	float GetBumpStrengthAmount() const
	{
		float fRet=5.0f;		// default like NVidia plugin
		m_pCC->config->Get(m_bAlphaAsBump?"bumpstrength_a":"bumpstrength",fRet);
		return fRet;
	}
	void EnableWindowBumpStrengthAmount( HWND hWnd ) const
	{
		string Value;	
		bool bSet = m_pCC->config->Get(m_bAlphaAsBump?"bumpstrength_a":"bumpstrength",Value,eCP_PriorityNotFile);
		if(!GetBumpToNormalFilter())
			bSet=true;
		EnableWindow(hWnd,!bSet);
	}
	// ----------------------------------------

	// Arguments:
	//  Value - filename without path and special characters like space, quotes, .. (dot is ok)
	void SetBumpmapName( const string Value )
	{
		if(GetBumpmapName()==Value)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"bumpname",Value);
	}
	// Returns:
	//  filename without path and special characters like space, quotes, .. (dot is ok)
	string GetBumpmapName() const
	{
		string Ret;
		m_pCC->config->Get("bumpname",Ret);
		return Ret;
	}

	// ----------------------------------------

	// Arguments:
	//  iValue - 0=none, 1=Gauss, 2=GaussAlpha
	void SetBumpToNormalFilter( const int iValue )
	{
		if(GetBumpToNormalFilter()==iValue)
			return;			
		char str[80];	sprintf(str,"%d",iValue);
		m_pCC->config->Set(eCP_PriorityFile,m_bAlphaAsBump?"bumptype_a":"bumptype",str);
	}
	// Returns:
	//  0=none, 1=Gauss, 2=GaussAlpha
	int GetBumpToNormalFilter() const
	{
		int iRet=0;		// default
		m_pCC->config->Get(m_bAlphaAsBump?"bumptype_a":"bumptype",iRet);
		iRet = max(iRet,0);
		return iRet;
	}
	void EnableWindowBumpToNormalFilter( HWND hWnd ) const
	{
		string Value;	
		bool bSet = m_pCC->config->Get(m_bAlphaAsBump?"bumptype_a":"bumptype",Value,eCP_PriorityNotFile);
		EnableWindow(hWnd,!bSet);
	}

private: // -------------------------------------------------------

	bool							m_bAlphaAsBump;							// true=m_AlphaAsBump, false=m_Bump2Normal

protected: // -------------------------------------------------------

	ConvertContext *	m_pCC;											// can be 0
};



class CImageProperties
{
public:
	enum {NUM_CONTROLLED_MIP_MAPS = 6};

	// keep number assignment as it's also used for file storage
	enum EGlobalCompressor
	{
		eCGDirectX=0,       // DirectX, might used hardware - fastest
		eCGNVDXT=1,         // NVDXT lib fast and good quality, supports 3Dc
		eCGATI=2,           // good quality, supports 3Dc, all unsupported formats are redirected to other compressor
		eCGTIF=3,           // .TIF file format
		eCGExperimental=4,  // experimental DXT compressor
		eCGNVSquish=5,      // NVidia & Squish - very slow, very high quality
		eCGNVSquishFast=6   // NVidia & Squish - slow, high quality
	};

	// --------------------------------------------------------------

	CImageProperties() :m_pCC(0), m_AlphaAsBump(true), m_Bump2Normal(false)
	{
		SetToDefault();
	}

	void SetToDefault()
	{
		m_ePreviewMode=ePM_RGB;
		m_bPreviewFiltered=false;
		m_bPreviewTiled=true;
		m_bApplyRangeAdjustment=true;
		m_bAverageColor=false;
		m_bHalfRes=false;
	}

	// Arguments:
	//  pCC - can be 0
	void SetPropsCC( ConvertContext *pCC )
	{
		m_pCC=pCC;
		m_Bump2Normal.SetCC(pCC);
		m_AlphaAsBump.SetCC(pCC);
	}

	// ----------------------------------------

	void SetPreset( const string &rStr )
	{
		if(GetPreset()!=rStr)
			m_pCC->config->Set(eCP_PriorityFile,"preset",rStr);

		m_pCC->config->ClearPriorityUsage(eCP_PriorityPreset);
		bool bPreset = rStr!="";
		if(bPreset)
			m_pCC->presets->SetConfig(eCP_PriorityPreset,rStr,m_pCC->config);
	}
	string GetPreset() const
	{
		string ret;

		m_pCC->config->Get("preset",ret,eCP_PriorityHighest);
		return ret;
	}

	// ----------------------------------------

	bool GetPowOf2() const
	{
		bool bRet=true;
		m_pCC->config->Get("powof2",bRet);
		return bRet;
	}

	// ----------------------------------------

	bool GetCubemap() const
	{
		bool bRet=false;
		m_pCC->config->Get("cm",bRet);
		return bRet;
	}

	// ----------------------------------------

	void SetGammaCorrectedMips( const bool bValue )
	{
		if(GetGammaCorrectedMips()==bValue)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"mipgamma",bValue?"1":"0");
	}
	bool GetGammaCorrectedMips() const
	{
		bool bRet=false;
		m_pCC->config->Get("mipgamma",bRet);
		return bRet;
	}

	// ----------------------------------------

	void SetSupressEngineReduce( const bool bValue )
	{
		if(GetSupressEngineReduce()==bValue)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"mipgamma",bValue?"1":"0");
	}
	// flag propagated to engine to affect loading (reduce texture resolution during loading)
	bool GetSupressEngineReduce() const
	{
		bool bRet=false;
		m_pCC->config->Get("ser",bRet);
		return bRet;
	}

	// ----------------------------------------

	// flag propagated to engine to define right border clamp behavior on alpha test
	bool GetDecal() const
	{
		bool bRet=false;
		m_pCC->config->Get("decal",bRet);
		return bRet;
	}

	// ----------------------------------------

	void SetMaintainAlphaCoverage( const bool bValue )
	{
		if(GetMaintainAlphaCoverage()==bValue)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"mc",bValue?"1":"0");
	}
	// maintain alpha coverage in mipmaps
	bool GetMaintainAlphaCoverage() const
	{
		bool bRet=false;
		m_pCC->config->Get("mc",bRet);
		return bRet;
	}

	// ----------------------------------------

	void SetMipRenormalize( const bool bValue )
	{
		if(GetMipRenormalize()==bValue)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"mipnormalize",bValue?"1":"0");
	}
	bool GetMipRenormalize() const
	{
		bool bRet=false;
		m_pCC->config->Get("mipnormalize",bRet);
		return bRet;
	}

	// ----------------------------------------

	void SetMipMaps( const bool bValue )
	{
		if(GetMipMaps()==bValue)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"mipmaps",bValue?"1":"0");
	}
	bool GetMipMaps() const
	{
		bool bRet=true;
		m_pCC->config->Get("mipmaps",bRet);
		return bRet;
	}

	// ----------------------------------------

	void SetSRGBLookup( const bool bValue )
	{
		if(GetSRGBLookup()==bValue)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"srgb",bValue?"1":"0");
	}
	bool GetSRGBLookup() const
	{
		bool bRet=true;

		m_pCC->config->Get("srgb",bRet);
		return bRet;
	}

	// ----------------------------------------

	void SetUserDialog( const bool bValue )
	{
		if(GetUserDialog()==bValue)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"userdialog",bValue?"1":"0");
	}
	bool GetUserDialog() const
	{
		bool bRet=false;
		m_pCC->config->Get("userdialog",bRet);
		return bRet;
	}

	// ----------------------------------------

	void SetUserDialogCustom( const bool bValue )
	{
		if(GetUserDialogCustom()==bValue)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"userdialogcustom",bValue?"1":"0");
	}
	bool GetUserDialogCustom() const
	{
		bool bRet=true;
		m_pCC->config->Get("userdialogcustom",bRet);
		return bRet;
	}

	// ----------------------------------------

	void SetAutoDetectLuminanceOnly( const bool bValue )
	{
		if(GetAutoDetectLuminanceOnly()==bValue)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"detectL8",bValue?"1":"0");
	}
	bool GetAutoDetectLuminanceOnly() const
	{
		bool bRet=false;
		m_pCC->config->Get("detectL8",bRet);
		return bRet;
	}

	// ----------------------------------------

	// Arguments:
	//  iValue - 0=no sharpening .. 100=full sharpening
	void SetMipSharpen( const int iValue )
	{
		if(GetMipSharpen()==iValue)
			return;			
		char str[80];	sprintf(str,"%d",iValue);
		m_pCC->config->Set(eCP_PriorityFile,"ms",str);
	}
	// Returns:
	//  0=no sharpening .. 100=full sharpening
	int GetMipSharpen() const
	{
		int iRet=35;		// default is already sharpen a bit
		m_pCC->config->Get("ms",iRet);
		return iRet;
	}

	// ----------------------------------------
 
	// Arguments:
	//  iValue - 0=use default filter, 1=use 2x2 filter
	void SetMipGenType( const int iValue )
	{
		if(GetMipGenType()==iValue)
			return;			
		char str[80];	sprintf(str,"%d",iValue);
		m_pCC->config->Set(eCP_PriorityFile,"mipgentype",str);
	}
	// Returns:
	//  0=use default filter, 1=use 2x2 filter
	int GetMipGenType() const
	{
		int iRet=0;
		m_pCC->config->Get("mipgentype",iRet);
		return iRet;
	}

	// ----------------------------------------
 
	// the setting does not become stored in the file !!!
	void OverwriteGlobalCompressor( const EGlobalCompressor Value )
	{
		char str[80];	sprintf(str,"%d",(int)Value);
		m_pCC->config->Set(eCP_PriorityForce,"globalcompressor",str);
	}
	EGlobalCompressor GetGlobalCompressor() const
	{
		int iRet=(int)eCGNVDXT;
		m_pCC->config->Get("globalcompressor",iRet);
		return (EGlobalCompressor)iRet;
	}

	// ----------------------------------------

	// Returns:
	//   0..255 to limit per pixel pow factor adjustment - prevents NAN in shader
	int GetMinAlpha() const
	{
		int iRet=0;		// default
		m_pCC->config->Get("minalpha",iRet);
		return iRet;
	}

	// ----------------------------------------

	// -1 if not used, otherwise 32bit value is used to set the border of the texture to this color
	uint64 GetBorderColor()
	{
		string sHex;

		if(m_pCC->config->Get("mipbordercolor",sHex))
		{
			uint32 dw32BitValue=0;

			if(sscanf(sHex,"%x",&dw32BitValue)==1)
				return (int64)dw32BitValue;
		}

		return -1;										// -1 if not used, otherwise 32bit value is used to set the border of the texture to this color
	}

	// ----------------------------------------

	// Returns:
	//  0=no sharpening .. 100=full sharpening
	int GetHighPass() const
	{
		int iRet=0;		// default is off
		m_pCC->config->Get("highpass",iRet);
		return iRet;
	}

	// ----------------------------------------

	// Returns:
	//   0=RGB, 1=CIE
	int GetColorModel() const
	{
		int iRet=0;		// default
		m_pCC->config->Get("colormodel",iRet);
		return iRet;
	}

	// ----------------------------------------

	void SetAutoOptimizeFile( const bool bValue )
	{
		if(GetAutoOptimizeFile()==bValue)
			return;			
		m_pCC->config->Set(eCP_PriorityFile,"autooptimizefile",bValue?"1":"0");
	}
	bool GetAutoOptimizeFile() const
	{
		bool bRet=true;
		m_pCC->config->Get("autooptimizefile",bRet);
		return bRet;
	}

	// ----------------------------------------

	char *GetPreviewModeText() const
	{
		switch(m_ePreviewMode)
		{
			case ePM_RGB:								return "Normal RGB preview mode\n(no gamma correction)";
			case ePM_AAA:								return "Preview Alpha channel in greyscale\n(no gamma correction)";
			case ePM_RGBA:							return "RGB Alpha blended with the background\n(no gamma correction)";
			case ePM_NormalLength:			return "Normalmap debug: The brighter a pixel is the better the normal,\nblue indicates very short normals, red impossible normals (>1)";
			case ePM_RGBmulA:						return "RGB multiplied with Alpha, useful to get cheap HDR textures\n(no gamma correction)";
			case ePM_AlphaTest:					return "Shows the Alpha Channel vizualized with an AlpheTest of 0.5";
			case ePM_mCIE2RGB:					return "mCIE to RGB conversion (for internal tests)";
			default:	assert(0);				return "";
		}
	}

	// ----------------------------------------

	// Return:
	//   index in the g_pixelformats table [0..GetPixelFormatCount()-1] or -1 if no conversion should happen
	TPixelFormatID GetDestPixelFormat( const bool bAlphaChannelUsed, bool tryPreviewFormat = false ) const
	{
		int iDestPixelFormat=-1;
		string sPixelformat="UNKNOWN";					
		string sPreviewFormat="UNKNOWN";

		if(m_pCC->config->Get("pixelformat",sPixelformat))
		{
			if ( tryPreviewFormat && m_pCC->config->Get("previewformat",sPreviewFormat) )
			{
				if ( CPixelFormats::GetNoFromName(sPreviewFormat.c_str()) != -1 )
				{
					sPixelformat = sPreviewFormat;
				}
			}

			int iNo=CPixelFormats::GetNoFromName(sPixelformat.c_str());	

			if(iNo==-1)
			{ 
				if(strcmp(sPixelformat,g_szInvalidPixelFormatName))
				{
					assert(0);
					RCLogError("CImageCompiler::Process pixelformat '%s' not recognized",sPixelformat.c_str());
				}
				return (TPixelFormatID)-1; 
			}

			iDestPixelFormat = iNo;
		}

		return CPixelFormats::FindFinalTextureFormat(iDestPixelFormat,bAlphaChannelUsed);
	}
	// Arguments:
	//   iDestPixelFormat - index in the g_pixelformats table [0..GetPixelFormatCount()-1], -1 if not used
	void SetDestPixelFormat( const TPixelFormatID iDestPixelFormat, const bool bAlphaChannelUsed )
	{
		if(GetDestPixelFormat(bAlphaChannelUsed)==iDestPixelFormat)
			return;

		m_pCC->config->Set(eCP_PriorityFile,"pixelformat",CPixelFormats::GetPixelFormatName(iDestPixelFormat));
//		m_pCC->config->Set(eCP_PriorityFile,pixelformat",CPixelFormats::GetPixelFormatName(m_Props.GetDestPixelFormat(InputHasAlpha() )));
	}

	// ----------------------------------------

	uint32 GetReduceResolution() const
	{
		int iReduceResolutionGlobal = m_pCC->config->GetSum("globalreduce");	// [0..[ to remove the top mipmap levels  /globalreduce

		if(m_bHalfRes)
			iReduceResolutionGlobal++;

		return (uint32)max(0,(iReduceResolutionGlobal+GetReduceResolutionFile()));
	}



	// ----------------------------------------

	// Arguments:
	// Value -2..3 to remove the top mipmap levels  /reduce
	void SetReduceResolutionFile( const int Value )
	{
		if(GetReduceResolutionFile()==Value)
			return;			
		char str[80];	sprintf(str,"%d",(int)Value);
		m_pCC->config->Set(eCP_PriorityFile,"reduce",str);
	}
	// Returns:
	//  -2..3 to remove the top mipmap levels  /reduce
	int GetReduceResolutionFile() const
	{
		int iRet=0;
		m_pCC->config->Get("reduce",iRet);
		return iRet;
	}

	// ----------------------------------------
	// Arguments:
	//   iMIPAlpha - [0..NUM_CONTROLLED_MIP_MAPS-1] array to return the values in the range 0..100
	void GetMIPAlphaArray( int iMIPAlpha[NUM_CONTROLLED_MIP_MAPS] ) const
	{
		std::fill(iMIPAlpha, iMIPAlpha + NUM_CONTROLLED_MIP_MAPS, 50);

		string sMipControl;

		if(m_pCC->config->Get("M",sMipControl))			// e.g. "50,30,50,20,20"
		{
			const char *p = &sMipControl[0];

			for(uint32 dwI=0;dwI<NUM_CONTROLLED_MIP_MAPS;++dwI)
			{
				uint32 dwValue=0;

				while(*p>='0' && *p<='9')							// value
					dwValue = dwValue*10+(*p++ - '0');

				if(dwValue<=100)
					iMIPAlpha[dwI]=dwValue;

				while(*p==' ')	++p;									// jump over whitespace

				if(*p==',')														// separator
					++p;

				while(*p==' ')	++p;									// jump over whitespace

				if(*p==0)															// end terminator
					break;
			}
		}
	}

	// Arguments:
	//   iId - 0..NUM_CONTROLLED_MIP_MAPS-1
	//   iValue - 0..100
	void SetMIPAlpha( const int iId, const int iValue )
	{
		assert(iId>=0 && iId<NUM_CONTROLLED_MIP_MAPS);

		int iMIPAlpha[NUM_CONTROLLED_MIP_MAPS];

		GetMIPAlphaArray(iMIPAlpha);

		iMIPAlpha[iId]=iValue;

		char str[256];

		string mipControlString;			// e.g. "50,30,50,20,20"

		for (int i = 0; i < CImageProperties::NUM_CONTROLLED_MIP_MAPS; ++i)
		{
			char szPropName[1024];
			sprintf(szPropName, "M%d", i);
			sprintf(str,"%d",iMIPAlpha[i]);
			//m_pCC->config->Set(eCP_PriorityFile,szPropName,str);
			//		m_pCC->config->RemoveEntry("",szPropName);
			m_pCC->config->Set(eCP_PriorityFile,szPropName,0);		// remove key
			
			if(i!=0)
				mipControlString += ",";							// separator

			mipControlString += str;
		}

		m_pCC->config->Set(eCP_PriorityFile,"M",mipControlString.c_str());
	}

	// Arguments:
	//   iMip - 0..iMaxMip-1
	//   iMaxMip - >1
	// Returns:
	//   0..255
	float ComputeMIPAlphaOffset( const int iMip, const int iMaxMip )
	{		
		int iMIPAlpha[NUM_CONTROLLED_MIP_MAPS];

		GetMIPAlphaArray(iMIPAlpha);

		float fVal = (float)iMIPAlpha[NUM_CONTROLLED_MIP_MAPS - 1];
		if (iMip < NUM_CONTROLLED_MIP_MAPS * 2)
		{
			float fInterpolationSlider1 = (float)iMIPAlpha[iMip / 2];
			float fInterpolationSlider2 = (float)iMIPAlpha[iMip / 2 + 1];
			fVal = fInterpolationSlider1 + (fInterpolationSlider2 - fInterpolationSlider1) * (iMip & 1) * 0.5f;
		}

		return 0.5f-fVal/100.0f;
	}

	// -----------------------------------------------

	// cubemap filter type:
	//	DISC             0
	//	CONE             1
	//	COSINE           2
	//	ANGULAR_GAUSSIAN 3
	uint32 GetCubemapFilterType() const
	{
		uint32 type = 3;
		string sTypeName = "cosine";
		m_pCC->config->Get("cm_ftype", sTypeName);
		if(_stricmp(sTypeName.c_str(), "disc") == 0)
			type = 0;
		else if(_stricmp(sTypeName, "cone") == 0)
			type = 1;
		else if(_stricmp(sTypeName, "cosine") == 0)
			type = 2;
		else if(_stricmp(sTypeName, "gaussian") == 0)
			type = 3;
		else assert(0);

		return type;
	}

	// cubemap filter angle in radians	
	float GetCubemapFilterAngle() const
	{
		float angle = 0.f;
		m_pCC->config->Get("cm_fangle", angle);
		assert(angle >= 0.f && angle < 360);
		return angle;
	}

	// cubemap initial mip filter angle in radians	
	float GetCubemapMipFilterAngle() const
	{
		float angle = 0.f;
		m_pCC->config->Get("cm_fmipangle", angle);
		assert(angle >= 0.f && angle < 360);
		return angle;
	}

	// cubemap mip filter angle slope 
	float GetCubemapMipFilterSlope() const
	{
		float slope = 0.f;
		m_pCC->config->Get("cm_fmipslope", slope);
		assert(slope >= 1.f && slope <= 3.f);
		return slope;
	}

	// cubemap edge fixup width in texels 
	int GetCubemapEdgeFixupWidth() const
	{
		int width = 0;
		m_pCC->config->Get("cm_edgefixup", width);
		assert(width >= 0 && width <= 64);
		return width;
	}

	// ----------------------------------------
	bool GetRGBKCompression() const
	{
		bool bRet=false;
		m_pCC->config->Get("rgbk",bRet);
		return bRet;
	}

	float GetRGBKMultiplier() const
	{
		float fMultiplier = 1.f / ehdDefaultHDRDevider;

		return fMultiplier;
	}

	// -------------------------------------------------------------------------

	CBumpProperties		m_Bump2Normal;										//
	CBumpProperties		m_AlphaAsBump;										//

	// cannot be adjusted from outside (through RC key value pairs)
	bool							m_bApplyRangeAdjustment;					// true = -1..1 => 0..1 adjustment (needed for normalmaps)
	bool							m_bHalfRes;												// from auto optimize
	bool							m_bAverageColor;									//

	// preview properties
	EPreviewMode			m_ePreviewMode;										// e.g. 0:RGB, 1:AAA replicate the alpha channel as greyscale value
	bool							m_bPreviewFiltered;								// activate the bilinear filter in the preview
	bool							m_bPreviewTiled;									// 

private: // ---------------------------------------------------------------------

	ConvertContext *	m_pCC;														// can be 0
};
