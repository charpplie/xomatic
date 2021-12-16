#pragma once

#include <string>																			// STL string
#include "SRFData.h"																	// CSRFData
#include "SimpleIndexedMesh.h"												// CSimpleIndexedMesh


class CProperties
{
public:
	int									m_nHitMode;										//!< serialized, 0:nearest point, 1:latest hit
	float								m_fRayLength;									//!< serialized, ray length for ray shooting (1=100% of the low poly)

	int									m_iTextureSizePX;							//!< serialized, Texture Size horizontal 0=8, 1=16, 2=32, ..
	int									m_iTextureSizePY;							//!< serialized, Texture Size vertical   0=8, 1=16, 2=32, ..

	int									m_iAntiAliasingMode;					//!< serialized, anti-aliasing 0=none,1=low,2=med,3=high,4=max
//	bool								m_bExtendFilename;						//!< extend the filename with render attributes

	int									m_iHorizonMode;								//!< serialized, 0/1/2/3, if !=0 do additional horizon sampling (much slower)

	std::string					m_szOutputFilename;						//!< not serialized, Output Base Filename e.g. "PolyBump"

	bool								m_bOutputDebugInfo;						//!< not serialized, for internal debug purpose (output internal representation as .OBJ files, RasterCube Bitmaps)

	std::string					m_bSummariesString;						//!< not serialized, This string is used for putting the summaries together (use \n for seperating), this is only shown if m_bPrintSummary is true
	bool								m_bPrintSummary;							//!< not serialized, activates after working a messagebox with the summaries (m_bSummariesString)

	std::string					m_LowPolyMeshFileName;				//!< serialized
	std::string					m_HighPolyMeshFileName;				//!< serialized
	std::string					m_CageMeshFileName;						//!< serialized

	bool								m_bLoadHighPolyWithSmoothing;	//!< serialized, slower loading, more memory needed, not advised for ZBrush models
	std::string					m_LowPolyMaterialName;				//!< serialized

	int									m_iThreadPrio;								//!< serialized, thread priority -2=minimum, -1, 0=normal, 1, 2=maximum

private: // -----------------------------------------------------------------------

//	int									m_iMaterialID;								//!< material id of the low poly that we want to calculate, -1 for every material id

public:	// -----------------------------------------------------------------------

	//! constructor
	CProperties()
	{
		SetToDefault();
	}


	// material id of the low poly that we want to calculate, -1 for every material id
	int GetLowPolyMaterialId( CSimpleIndexedMesh &rMesh ) const
	{
		for(uint32 dwM=0;dwM<rMesh.m_MaterialCount;++dwM)
		{
			CSimpleMaterial &rMat = rMesh.m_pMaterials[dwM];

			if(!m_LowPolyMaterialName.empty())
			if(rMat.m_sMaterialName==m_LowPolyMaterialName)
				return dwM;
		}

		return -1;
	}

	void SetToDefault()
	{
		m_iHorizonMode=0;
		m_iAntiAliasingMode=0;
//		m_bExtendFilename=true;
		m_fRayLength=0.10f;							// 10%
		m_nHitMode=1;										// use latest hit
		m_iTextureSizePX=5;							// 256 
		m_iTextureSizePY=5;							//     x 256
		m_szOutputFilename="Polybump.srf";
		m_bOutputDebugInfo=false;
		m_bPrintSummary=false;
//		m_iMaterialID=-1;
		m_bLoadHighPolyWithSmoothing=false;
		m_iThreadPrio=0;
	}
	
	char *GetInfoString() const
	{
}

	//!
	DWORD GetOutputWidth()
	{
		return 1<<(m_iTextureSizePX+3);
	}

	//!
	DWORD GetOutputHeight()
	{
		return(1<<(m_iTextureSizePY+3));
	}

	bool DoNonAAStep()
	{
		if(m_iHorizonMode)
			return true;																						// if horizon needed

		return m_iAntiAliasingMode!=4 && m_iAntiAliasingMode!=3;	// if not adaptive AA
	}

	void GetAntialiasingParam( float &outfAntiAliasStep, float &outfAntialiasTreshold )
	{
		switch(m_iAntiAliasingMode)
		{
			case 1:	// low
				outfAntiAliasStep=1.0f/2.0f;			// 4 samples adaptive
				outfAntialiasTreshold=0.30f*0.30f;
				break;

			case 2:	// med
				outfAntiAliasStep=1.0f/3.0f;			// 9 samples adaptive
				outfAntialiasTreshold=0.10f*0.10f;
				break;

			case 3:	// high
				outfAntiAliasStep=1.0f/3.0f;			// 9 samples
				outfAntialiasTreshold=0.0f;
				break;

			case 4:	// max
				outfAntiAliasStep=1.0f/4.0f;			// 16 samples
				outfAntialiasTreshold=0.0f;
				break;

			default: assert(0);
		}
	}

	bool DoAAJittering()
	{
		return false;
	}

	void GetFromSRF( const CSRFData &rSRF )
	{
		SetToDefault();

		char str[1024];

		if(rSRF.GetValue("lowpolyfilename",str,1024))
			m_LowPolyMeshFileName=str;

		if(rSRF.GetValue("highpolyfilename",str,1024))
			m_HighPolyMeshFileName=str;

		if(rSRF.GetValue("cagemeshfilename",str,1024))
			m_CageMeshFileName=str;

		rSRF.GetValue("horizonmode",m_iHorizonMode);

		{
			int iVal = m_bLoadHighPolyWithSmoothing?1:0;

			rSRF.GetValue("loadhighpolysmooth",iVal);

			m_bLoadHighPolyWithSmoothing = iVal!=0;
		}

		// under construction
		if(rSRF.GetValue("lowpolymaterialname",str,1024))
			m_LowPolyMaterialName=str;

		rSRF.GetValue("texturesizepx",m_iTextureSizePX);
		rSRF.GetValue("texturesizepy",m_iTextureSizePY);

		rSRF.GetValue("threadprio",m_iThreadPrio);
		rSRF.GetValue("hitmode",m_nHitMode);
		rSRF.GetValue("raylength",m_fRayLength);
		rSRF.GetValue("antialiasing",m_iAntiAliasingMode);
	}

	void SetToSRF( CSRFData &rSRF ) const
	{
		rSRF.SetValue("lowpolyfilename",m_LowPolyMeshFileName.c_str());
		rSRF.SetValue("highpolyfilename",m_HighPolyMeshFileName.c_str());
		rSRF.SetValue("cagemeshfilename",m_CageMeshFileName.c_str());
		rSRF.SetValue("horizonmode",m_iHorizonMode);
		rSRF.SetValue("loadhighpolysmooth",m_bLoadHighPolyWithSmoothing?1:0);
		rSRF.SetValue("lowpolymaterialname",m_LowPolyMaterialName.c_str());		// under construction

		rSRF.SetValue("texturesizepx",m_iTextureSizePX);
		rSRF.SetValue("texturesizepy",m_iTextureSizePY);

		rSRF.SetValue("threadprio",m_iThreadPrio);
		rSRF.SetValue("hitmode",m_nHitMode);
		rSRF.SetValue("raylength",m_fRayLength);
		rSRF.SetValue("antialiasing",m_iAntiAliasingMode);
	}
};




