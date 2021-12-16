#include "StdAfx.h"
#include <Dbghelp.h>									// GetTimestampForLoadedLibrary()
#include <assert.h>										// assert()
#include "IConfig.h"									// IConfig
#include "ICfgFile.h"									// ICfgFile
#include "SRFCompiler.h"							// CSRFCompiler
#include "SRFUserDialog.h"						// CSRFUserDialog
#include "tools.h"										// GetBoolParam()
#include "IRCLog.h"										// IRCLog
#include "ImageObject.h"							// CFloat4Image
#include "IResCompiler.h"							// IResourceCompiler

CSRFCompiler::CSRFCompiler() :m_pCC(0), m_refCount(1)
{
}

CSRFCompiler::~CSRFCompiler()
{
}


void CSRFCompiler::Release()
{
	if (--m_refCount <= 0)
		delete this;
}

bool CSRFCompiler::Process( ConvertContext &cc )
{
	m_pCC=&cc;
	m_Props.SetCC(&cc);

	static bool bFirstTime=true;
	if(bFirstTime)											// print the supported formats
	{
//		RCLog("");
//		RCLog("ResourceCompilerImage supported output pixel formats:");
		bFirstTime=false;
	}


	string sourceFile = cc.getSourcePath();

	bool bRet = true;

	if(m_InputSRFFile.Load(sourceFile))
	{
		if(!LoadConfigFile())
			return false;			// error

//		SetPresetSettings();

		if(m_Props.m_bUserDialog)
		{
			CSRFUserDialog dialog;

			dialog.DoModal(this);
		}
		else bRet = RunWithProperties();			// run and save
	}
	else
	{
		RCLogError("CImageCompiler::Process load '%s' Ext:'%s' failed",(const char*)sourceFile,(const char*)cc.sourceFileFinalExtension );
		return false;
	}

	m_pCC=0;
	m_InputSRFFile.FreeData();

	return bRet;
}







bool CSRFCompiler::RunWithProperties()
{
	bool bRet = true;

	float fDispMax=-FLT_MAX, fDispMin=FLT_MAX, fDispMul=0.0f;

	uint32 dwWidth=m_InputSRFFile.GetWidth(), dwHeight=m_InputSRFFile.GetHeight();

	if(dwWidth==0 || dwHeight==0)
	{
		RCLogError("CSRFCompiler::RunWithProperties width*heigtht == 0");
		return false;
	}

	// calculated fDispMax, fDispMin and fDispMul before ExpandBorder
	if(m_Props.m_eDisplaceOutputType!=EDOT_None)
	{
		
		m_InputSRFFile.CalcDisplacementRange(fDispMin,fDispMax,false,false);		// false,false specifies the way the value is calculated

		fDispMul=1.0f/(fDispMax-fDispMin+0.000001f);		// +epsilon to avoid div by 0
	}

	m_InputSRFFile.AllocSurfaceIndirection(m_Props.GetExpandBorder());

	CImageCompiler imagecompiler(0);		// one instance to save the overhead of init/deinit

//	LoadConfigFile();

	ImageObject	*pAddBumpmap=0;										// optional bump map file
	CSimpleBitmap<Vec4> *pAddBumpmapBitmap=0;

//	imagecompiler.SetCC(m_pCC);
	ConvertContext LocalCC;

	LocalCC = *m_pCC;

//	LocalCC.pFileSpecificConfig = m_pCC->pRC->CreateCfgFile();

	imagecompiler.SetCC(&LocalCC);
	imagecompiler.Init(0);

	// set metadata of output files
	{
		char str[80];

		sprintf_s(str,"%.4g",fDispMin);
		LocalCC.config->Set(eCP_PriorityFile,"fDispMin",str);
		sprintf_s(str,"%.4g",fDispMax);
		LocalCC.config->Set(eCP_PriorityFile,"fDispMax",str);

//		LocalCC.config->Set(eCP_PriorityFile,"preset","Normalmap_lowQ");		// better to leave preset not specified, RC can do it by _DDN and it should not be used for _ACC
	}

	if(m_Props.GetBumpToNormalFilter()!=0)
	if(!m_Props.GetBumpmapName().empty())
	{
		if(imagecompiler.LoadInput(m_Props.GetBumpmapName(),"tif",false))
		{
			imagecompiler.m_Props.SetToDefault();
//			imagecompiler.m_Props.OverwriteGlobalCompressor(CImageProperties::eCGTIF);		// output TIF file
			imagecompiler.m_Props.SetDestPixelFormat(-1,imagecompiler.InputHasAlpha());		// don't convert
			imagecompiler.m_Props.SetMipMaps(false);
			imagecompiler.m_Props.m_bApplyRangeAdjustment=false;
			imagecompiler.m_Props.m_bAverageColor=false;
			imagecompiler.m_Props.SetAutoOptimizeFile(false);
//			*((CBumpProperties *)&imagecompiler.m_Props) = *((CBumpProperties *)&m_Props);
			imagecompiler.m_Props.m_Bump2Normal.SetBumpToNormalFilter(1);

			if(imagecompiler.RunWithProperties(false))		// process but do not save
			{
				pAddBumpmap = imagecompiler.GetOutputAndRelease();
				pAddBumpmapBitmap = pAddBumpmap->GetSimpleBitmap();				assert(pAddBumpmapBitmap);
			}
			else RCLogError("CSRFCompiler::RunWithProperties additional bumpmap '%s' processing failed",m_Props.GetBumpmapName());

			imagecompiler.m_Props.m_Bump2Normal.SetBumpToNormalFilter(0);
		}
		else RCLogError("CSRFCompiler::RunWithProperties additional bumpmap '%s' load failed",m_Props.GetBumpmapName());

		imagecompiler.FreeTexture();
	}

	// normal map
	{
		uint32 dwWidth=m_InputSRFFile.GetWidth(), dwHeight=m_InputSRFFile.GetHeight();

		CFloat4Image *pNormalmap=new CFloat4Image(dwWidth,dwHeight);

		Vec4 *pMem=0;
		uint32 dwPitch=0;		// in bytes;

		float fBumpScaleX=0,fBumpScaleY=0;

		if(pAddBumpmapBitmap)
		{
			fBumpScaleX = (float)pAddBumpmapBitmap->GetWidth() / (float)dwWidth;
			fBumpScaleY = (float)pAddBumpmapBitmap->GetHeight() / (float)dwHeight;
		}

		if(pNormalmap->Lock(0,0,(char * &)pMem,dwPitch))
		{
			Vec4 *pStartMem=pMem;

			for(int iY=dwHeight-1;iY>=0;--iY)		// upside down because this is the way 3dsmax is handling the texture coordinated
			for(int iX=0;iX<(int)dwWidth;++iX)
			{
				uint32 dwNearX, dwNearY;
				m_InputSRFFile.GetNearestXY(iX,iY,dwNearX,dwNearY);

				Vec3 vNormal = m_InputSRFFile.GetHighPolyNormal(dwNearX,dwNearY);		// gets normal in WS

				switch(m_Props.m_eUsedSpaceType)
				{
					case EUST_TangentA:
						vNormal = m_InputSRFFile.TransformDirectionIntoTangentSpace(iX,iY,vNormal,false);
						break;
					case EUST_TangentB:
						vNormal = m_InputSRFFile.TransformDirectionIntoTangentSpace(iX,iY,vNormal,true);
						break;
					case EUST_Object:
						break;
					default:
						assert(0);
				}

				// bump mapping
				if(pAddBumpmapBitmap)			// is bumpmap supplied?
				{
					Vec4 vBumpNormal;

					pAddBumpmapBitmap->GetFiltered(iX*fBumpScaleX,(float)(pAddBumpmapBitmap->GetHeight())-iY*fBumpScaleY,vBumpNormal);		// 1.0- because of 3DSMax texture coordinates

					{
						Vec3 point12,point21;
						Vec3 vUnused;		// unused

						Vec3 vTangentU, vTangentV, vTangentN;

						if(m_InputSRFFile.CalcLowTangentBase(iX,iY,vTangentU,vTangentV,vTangentN,m_Props.m_eUsedSpaceType==EUST_TangentB))
						{
							if(m_InputSRFFile.CalcLowPolyUVOrientation(iX,iY))
							{ vTangentU=-vTangentU;vTangentV=-vTangentV; }

//							float fValidCheck = vTangentU|vTangentV;

//							if(fValidCheck<-0.00001f || fValidCheck>0.00001f)					// 
							{
								vTangentU.Normalize();
								vTangentV.Normalize();

								Matrix33 mat12;		mat12.SetRotationAA(-asin(vBumpNormal.x),vTangentU);		// GetRotation should be static
								Matrix33 mat21;		mat21.SetRotationAA(-asin(vBumpNormal.y),vTangentV);		// GetRotation should be static

								vNormal = mat12*vNormal + mat21*vNormal;						// trick to make the rotation invariant to its position
								vNormal.NormalizeSafe();
							}
						}
					}
				}

				float fAlpha=1.0f;
				
				if(m_Props.m_eDisplaceOutputType!=EDOT_None)
				{
					fAlpha=m_InputSRFFile.CalcDisplacementDist(iX,iY,false,false);		// false,false specifies the way the value is calculated

					fAlpha = (fAlpha-fDispMin)*fDispMul;			// renormalized alpha channel fMin..fMax -> 0..1
				}

				vNormal+=Vec3(1,1,1);vNormal*=0.5f;		// from -1..1 to 0..1	

				*pMem++=Vec4( vNormal.x,vNormal.y,vNormal.z,fAlpha );
			}

			pNormalmap->Unlock(0);
		}

		imagecompiler.m_Props.SetToDefault();
		imagecompiler.m_Props.OverwriteGlobalCompressor(CImageProperties::eCGTIF);		// output TIF file
		imagecompiler.m_Props.SetAutoOptimizeFile(false);
		imagecompiler.m_Props.SetMipMaps(false);
		imagecompiler.m_Props.m_bAverageColor=false;
		imagecompiler.m_Props.SetAutoDetectLuminanceOnly(false);
		imagecompiler.GetInputFromMemory(pNormalmap);	// data will be stored and freed automatically
		bRet = imagecompiler.RunWithProperties(true,"_DDN");
	}

	// accessibility
	if(IsHorizonInformationAvailable())
	if(m_Props.m_bOutputAccess)
	{
		uint32 dwWidth=m_InputSRFFile.GetWidth(), dwHeight=m_InputSRFFile.GetHeight();

		CFloat4Image *pAccessmap=new CFloat4Image(dwWidth,dwHeight);

		Vec4 *pMem=0;
		uint32 dwPitch=0;		// in bytes;

		if(pAccessmap->Lock(0,0,(char * &)pMem,dwPitch))
		{
			for(int iY=dwHeight-1;iY>=0;--iY)		// upside down because this is the way 3dsmax is handling the texture coordinated
				for(int iX=0;iX<(int)dwWidth;++iX)
				{
					uint32 dwNearX, dwNearY;
					m_InputSRFFile.GetNearestXY(iX,iY,dwNearX,dwNearY);

					float fVal = m_InputSRFFile.CalcAccessibilityValue(dwNearX,dwNearY);

					*pMem++=Vec4(fVal,fVal,fVal,fVal);
				}

				pAccessmap->Unlock(0);
		}

		imagecompiler.m_Props.SetToDefault();
		imagecompiler.m_Props.OverwriteGlobalCompressor(CImageProperties::eCGTIF);		// output TIF file
		imagecompiler.m_Props.SetAutoOptimizeFile(false);
		imagecompiler.m_Props.SetMipMaps(false);
		imagecompiler.m_Props.m_bAverageColor=false;	
		imagecompiler.GetInputFromMemory(pAccessmap);	// data will be stored and freed automatically
		bRet = imagecompiler.RunWithProperties(true,"_ACC");
	}



	// diffuse
	{
		if(m_InputSRFFile.IsThereDiffuse())
		{
			uint32 dwWidth=m_InputSRFFile.GetWidth(), dwHeight=m_InputSRFFile.GetHeight();

			CFloat4Image *pDiffuse=new CFloat4Image(dwWidth,dwHeight);

			Vec4 *pMem=0;
			uint32 dwPitch=0;		// in bytes;

			if(pDiffuse->Lock(0,0,(char * &)pMem,dwPitch))
			{
				for(int iY=dwHeight-1;iY>=0;--iY)		// upside down because this is the way 3dsmax is handling the texture coordinated
					for(int iX=0;iX<(int)dwWidth;++iX)
					{
						uint32 dwNearX, dwNearY;
						m_InputSRFFile.GetNearestXY(iX,iY,dwNearX,dwNearY);

						float Value[3];

						m_InputSRFFile.GetDiffuse(dwNearX,dwNearY,Value);

						*pMem++=Vec4(Value[0],Value[1],Value[2],0);
					}

					pDiffuse->Unlock(0);
			}

			imagecompiler.m_Props.SetToDefault();
			imagecompiler.m_Props.OverwriteGlobalCompressor(CImageProperties::eCGTIF);		// output TIF file
			imagecompiler.m_Props.SetAutoOptimizeFile(false);
			imagecompiler.m_Props.SetMipMaps(false);
			imagecompiler.m_Props.m_bAverageColor=false;	
			imagecompiler.GetInputFromMemory(pDiffuse);	// data will be stored and freed automatically
			bRet = imagecompiler.RunWithProperties(true,"_DIFFUSE");
		}
	}



	// combo (high poly materialId)
	if(m_Props.m_bOutputCombo)
	{
		static const Vec4 ColTable[]=
		{
			// Vec4(red,green,blue,alpha)
			Vec4(0,0,0,0),
			Vec4(1,0,0,0),
			Vec4(0,1,0,0),
			Vec4(1,1,0,0),
			Vec4(0,0,1,0),
			Vec4(1,0,1,0),
			Vec4(0,1,1,0),
			Vec4(1,1,1,0)
		};

		if(m_InputSRFFile.IsThereHighPolyMaterialId())
		{
			uint32 dwWidth=m_InputSRFFile.GetWidth(), dwHeight=m_InputSRFFile.GetHeight();

			CFloat4Image *pCombo=new CFloat4Image(dwWidth,dwHeight);

			Vec4 *pMem=0;
			uint32 dwPitch=0;		// in bytes;

			if(pCombo->Lock(0,0,(char * &)pMem,dwPitch))
			{
				for(int iY=dwHeight-1;iY>=0;--iY)		// upside down because this is the way 3dsmax is handling the texture coordinated
					for(int iX=0;iX<(int)dwWidth;++iX)
					{
						uint32 dwNearX, dwNearY;
						m_InputSRFFile.GetNearestXY(iX,iY,dwNearX,dwNearY);

						uint8 matid = m_InputSRFFile.GetHighPolyMaterialId(dwNearX,dwNearY);

						if(matid==0)
							*pMem++ = Vec4(0.5f,0.5f,0.5f,0);
						 else
							*pMem++ = ColTable[(matid-1)%((sizeof(ColTable)/sizeof(Vec3)))];
					}

					pCombo->Unlock(0);
			}

			imagecompiler.m_Props.SetToDefault();
			imagecompiler.m_Props.OverwriteGlobalCompressor(CImageProperties::eCGTIF);		// output TIF file
			imagecompiler.m_Props.SetAutoOptimizeFile(false);
			imagecompiler.m_Props.SetMipMaps(false);
			imagecompiler.m_Props.m_bAverageColor=false;	
			imagecompiler.GetInputFromMemory(pCombo);	// data will be stored and freed automatically
			bRet = imagecompiler.RunWithProperties(true,"_COMBO");
		}
	}


	// unoccluded area direction
	if(IsHorizonInformationAvailable())
	if(m_Props.m_bOutputUnoccAreaDir)
	{
		uint32 dwWidth=m_InputSRFFile.GetWidth(), dwHeight=m_InputSRFFile.GetHeight();

		CFloat4Image *pUnoccdirmap=new CFloat4Image(dwWidth,dwHeight);

		Vec4 *pMem=0;
		uint32 dwPitch=0;		// in bytes;

		float fBrighter = m_Props.m_OccmapBrightenup*0.01f;		// 0..100 -> 0..1

		if(pUnoccdirmap->Lock(0,0,(char * &)pMem,dwPitch))
		{
			for(int iY=dwHeight-1;iY>=0;--iY)		// upside down because this is the way 3dsmax is handling the texture coordinates
				for(int iX=0;iX<(int)dwWidth;++iX)
				{
					uint32 dwNearX, dwNearY;
					m_InputSRFFile.GetNearestXY(iX,iY,dwNearX,dwNearY);

					Vec3 vUnoccdir;

					m_InputSRFFile.CalcUnocccludedAreaDirection(dwNearX,dwNearY,vUnoccdir,fBrighter);

					switch(m_Props.m_eUsedSpaceType)
					{
						case EUST_TangentA:
							vUnoccdir = m_InputSRFFile.TransformDirectionIntoTangentSpace(iX,iY,vUnoccdir,false);
							break;
						case EUST_TangentB:
							vUnoccdir = m_InputSRFFile.TransformDirectionIntoTangentSpace(iX,iY,vUnoccdir,true);
							break;
						case EUST_Object:
							break;
						default:
							assert(0);
					}

					*pMem++=Vec4(vUnoccdir.x,vUnoccdir.y,vUnoccdir.z,0)*0.5f+Vec4(0.5f,0.5f,0.5f,0);		// from -1..1 to 0..1
				}

				pUnoccdirmap->Unlock(0);
		}

		imagecompiler.m_Props.SetToDefault();
		imagecompiler.m_Props.OverwriteGlobalCompressor(CImageProperties::eCGTIF);	// output TIF file
		imagecompiler.m_Props.SetAutoOptimizeFile(false);
		imagecompiler.m_Props.SetMipMaps(false);
		imagecompiler.m_Props.m_bAverageColor=false;	
		imagecompiler.m_Props.SetAutoDetectLuminanceOnly(false);
		imagecompiler.GetInputFromMemory(pUnoccdirmap);	// data will be stored and freed automatically
		bRet = imagecompiler.RunWithProperties(true,"_DDNDIF");
	}

	// horizonmap
	if(IsHorizonInformationAvailable())
	if(m_Props.m_bOutputHorizonMap)
	{
		uint32 dwWidth=m_InputSRFFile.GetWidth(), dwHeight=m_InputSRFFile.GetHeight();

		CFloat4Image *pHorizonmap=new CFloat4Image(dwWidth,dwHeight);

		Vec4 *pMem=0;
		uint32 dwPitch=0;		// in bytes;

		float fBrighter = m_Props.m_OccmapBrightenup*0.01f;		// 0..100 -> 0..1

		if(pHorizonmap->Lock(0,0,(char * &)pMem,dwPitch))
		{
			for(int iY=dwHeight-1;iY>=0;--iY)		// upside down because this is the way 3dsmax is handling the texture coordinated
				for(int iX=0;iX<(int)dwWidth;++iX)
				{
					float fValue = m_InputSRFFile.CalcHorizon(iX,iY,0);

					*pMem++=Vec4(fValue,fValue,fValue,fValue);		// from -1..1 to 0..1
				}

				pHorizonmap->Unlock(0);
		}

		imagecompiler.m_Props.SetToDefault();
		imagecompiler.m_Props.OverwriteGlobalCompressor(CImageProperties::eCGTIF);		// output TIF file
		imagecompiler.m_Props.SetAutoOptimizeFile(false);
		imagecompiler.m_Props.SetMipMaps(false);
		imagecompiler.m_Props.m_bAverageColor=false;	
		imagecompiler.GetInputFromMemory(pHorizonmap);	// data will be stored and freed automatically
		bRet = imagecompiler.RunWithProperties(true,"_HRZ");
	}

	if(pAddBumpmap)
		delete pAddBumpmap;

//	delete LocalCC.pFileSpecificConfig;

	return bRet;
}


bool CSRFCompiler::IsHorizonInformationAvailable() const
{
	return m_InputSRFFile.GetHorizonCount()!=0;
}

// set the stored properties to the current file and save it
bool CSRFCompiler::LoadConfigFile()
{
	assert(m_pCC);

	m_Props.SetToDefault();

	// get properties from the file
	{
		string sProp = m_InputSRFFile.GetPropertiesString();

		UpdateOrCreateEntryFromString(m_pCC->config,sProp);

//		m_pCC->config->SetConfig( eCP_PriorityFile,COMMON_SECTION,m_pCC->config );
	}

//	m_pCC->config->Merge(m_pCC->m_pcfgOverwrite);

	// -----------------------

	{
		string sValue;

		if(m_pCC->config->Get("space",sValue))
		{
			if(sValue=="tangent")
				m_Props.m_eUsedSpaceType=EUST_TangentA;		// possibly sheared
			else if(sValue=="tangentO")
				m_Props.m_eUsedSpaceType=EUST_TangentB;		// orthogonalized
			else if(sValue=="object")
				m_Props.m_eUsedSpaceType=EUST_Object;
			else 
				RCLogError("CSRFCompiler::LoadConfigFile space '%s' not recognized",sValue.c_str());
		}
	}


	{
		string sValue;

		if(m_pCC->config->Get("normaloutput",sValue))
		{
			if(sValue=="R8G8B8")
				m_Props.m_eNormalOutputType=ENOT_R8G8B8;
			else if(sValue=="R8G8")
				m_Props.m_eNormalOutputType=ENOT_R8G8;
			else 
				RCLogError("CSRFCompiler::LoadConfigFile normaloutput '%s' not recognized",sValue.c_str());
		}
	}

	{
		string sValue;

		if(m_pCC->config->Get("outputaccess",sValue))
			m_Props.m_bOutputAccess = (sValue!="0");
	}

	{
		string sValue;

		if(m_pCC->config->Get("outputcombo",sValue))
			m_Props.m_bOutputCombo = (sValue!="0");
	}

	{
		string sValue;

		if(m_pCC->config->Get("outputoccdir",sValue))
			m_Props.m_bOutputUnoccAreaDir = (sValue!="0");
	}

	{
		string sValue;

		if(m_pCC->config->Get("outputhorizonmap",sValue))
			m_Props.m_bOutputHorizonMap = (sValue!="0");
	}

	{
		string sValue;

		if(m_pCC->config->Get("displaceoutput",sValue))
		{
			if(sValue=="none")
				m_Props.m_eDisplaceOutputType=EDOT_None;
			else if(sValue=="linearsigned")
				m_Props.m_eDisplaceOutputType=EDOT_LinearSigned;
			else 
				RCLogError("CSRFCompiler::LoadConfigFile displaceoutput '%s' not recognized",sValue.c_str());
		}
	}

	m_Props.m_bUserDialog = false;
	m_pCC->config->Get("userdialog",m_Props.m_bUserDialog);

	return true;
} 


bool CSRFCompiler::UpdateAndSaveConfig()
{
	string sPropString;

	sPropString+="/space=";
	switch(m_Props.m_eUsedSpaceType)
	{
		case EUST_TangentA:
			sPropString+="tangent ";	// possible sharead
			break;
		case EUST_TangentB:
			sPropString+="tangentO ";	// orthogonalized
			break;
		case EUST_Object:
			sPropString+="object ";
			break;
		default:
			assert(0);
	}	

	sPropString+="/normaloutput=";
	switch(m_Props.m_eNormalOutputType)
	{
		case ENOT_R8G8B8:
			sPropString+="R8G8B8 ";
			break;
		case ENOT_R8G8:
			sPropString+="R8G8 ";
			break;
		default:
			assert(0);
	}

	sPropString+=string("/outputaccess=") + (m_Props.m_bOutputAccess?"1 ":"0 ");

	sPropString+=string("/outputcombo=") + (m_Props.m_bOutputCombo?"1 ":"0 ");

	sPropString+=string("/outputoccdir=") + (m_Props.m_bOutputUnoccAreaDir?"1 ":"0 ");

	sPropString+=string("/outputhorizonmap=") + (m_Props.m_bOutputHorizonMap?"1 ":"0 ");
	
	sPropString+="/displaceoutput=";
	switch(m_Props.m_eDisplaceOutputType)
	{
		case EDOT_None:
			sPropString+="none ";
			break;
		case EDOT_LinearSigned:
			sPropString+="linearsigned ";
			break;
		default:
			assert(0);
	}	

	{
		char str[80];

		sprintf(str,"/occdirbright=%.2f ",m_Props.m_OccmapBrightenup);

		sPropString+=str;
	}

	// bump
	{
		char str[256];

		sprintf(str,"/bumptype=%d ",m_Props.GetBumpToNormalFilter());
		sPropString+=str;

		sprintf(str,"/bumpstrength=%f ",m_Props.GetBumpStrengthAmount());
		sPropString+=str;

		sprintf(str,"/bumpblur=%f ",m_Props.GetBumpBlurAmount());
		sPropString+=str;
	}


	{
		char str[80];

		sprintf(str,"/reduce=%d ",m_Props.GetReduceResolutionFile());

		sPropString+=str;
	}
	
	{
		char str[80];

		sprintf(str,"/expand=%d ",m_Props.GetExpandBorder());

		sPropString+=str;
	}

	if(!m_Props.GetBumpmapName().empty())
		sPropString += "/bumpname=\"" + m_Props.GetBumpmapName() + string("\" ");

	// --------------

	m_InputSRFFile.SetPropertiesString(sPropString);

	return m_InputSRFFile.Save(m_pCC->getSourcePath());
}


void CSRFCompiler::ConstructAndSetOutputFile(ConvertContext &cc)
{
	cc.SetOutputFile( PathHelpers::ReplaceExtension( cc.sourceFileFinal,"tif" ) );
}


void CSRFCompiler::GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const
{
	string const filename(cc.getOutputPath());

	if (filenameBuffer && (filename.length() < bufferSize))
	{
		strcpy(filenameBuffer, filename.c_str());		
	}
}


ICompiler* CSRFCompiler::CreateCompiler()
{
	// Only ever return one compiler, since we don't support multithreading. Since
	// the compiler is just this object, we can tell whether we have already returned
	// a compiler by checking the ref count.
	if (m_refCount >= 2)
		return 0;

	// Until we support multithreading for this convertor, the compiler and the
	// convertor may as well just be the same object.
	++m_refCount;
	return this;
}


bool CSRFCompiler::SupportsMultithreading() const
{
	return false;
}

int CSRFCompiler::GetNumPlatforms() const
{
	return 3;
}


EPlatform CSRFCompiler::GetPlatform( int index ) const
{
	switch (index)
	{
	case 0:	return ePlatform_PC;
	case 1:	return ePlatform_X360;
	case 2:	return ePlatform_PS3;
		//case 3:	return ePlatform_GAMECUBE;
	};
	//assert(0);
	return ePlatform_UNKNOWN;
}


int CSRFCompiler::GetNumExt() const
{
	return 1;
}


const char* CSRFCompiler::GetExt( int index ) const
{
	switch (index)
	{
		case 0:	return("srf");
		// advance CSRFCompiler::GetNumExt if you add more extensions
	};

	//assert(0);
	return(0);
}


DWORD CSRFCompiler::GetTimestamp() const
{
	return GetTimestampForLoadedLibrary(g_hInst);
}


