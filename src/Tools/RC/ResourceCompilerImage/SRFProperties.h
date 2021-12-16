#ifndef __SRFPROPERTIES_H
#define __SRFPROPERTIES_H

#pragma once

// should match the order in the dialog
enum EUsedSpaceType
{
	EUST_Object,
	EUST_TangentA,		// possible sheared
	EUST_TangentB,		// orthogonalized
};

// should match the order in the dialog
enum ENormalOutputType
{
	ENOT_R8G8B8,
	ENOT_R8G8,
};



// should match the order in the dialog
enum EDisplaceOutputType
{
	EDOT_None,
	EDOT_LinearSigned,
};

#include "ImageProperties.h"				// CBumpProperties


class CSRFProperties :public CBumpProperties
{
public:

	//
	CSRFProperties() :CBumpProperties(false)
	{		
		m_bUserDialog=false;
		SetToDefault();
	}

	void SetToDefault()
	{
		m_OccmapBrightenup=0.0f;
		m_eUsedSpaceType=EUST_TangentA;
		m_eNormalOutputType=ENOT_R8G8B8;
		m_bOutputAccess=false;
		m_bOutputCombo=false;
		m_bOutputUnoccAreaDir=false;
		m_bOutputHorizonMap=false;
		m_eDisplaceOutputType=EDOT_None;
	}

	// ----------------------------------------

	// Arguments:
	// Value -2..3 to remove the top mipmap levels  /reduce
	void SetReduceResolutionFile( const int Value )
	{
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
	// Value [0..[ to remove avoid problems in lower mipmaps
	void SetExpandBorder( const int Value )
	{
		char str[80];	sprintf(str,"%d",(int)Value);
		m_pCC->config->Set(eCP_PriorityFile,"expand",str);
	}
	// Returns:
	//  [0..[ to remove avoid problems in lower mipmaps
	int GetExpandBorder() const
	{
		int iRet=16;			// default
		m_pCC->config->Get("expand",iRet);
		iRet=max(iRet,0);
		return iRet;
	}

	// ----------------------------------------

	// config properties
	bool								m_bUserDialog;						// show the user dialog for interactive tweaking

	EUsedSpaceType			m_eUsedSpaceType;					//
	ENormalOutputType		m_eNormalOutputType;			//
	bool								m_bOutputAccess;					//
	bool								m_bOutputCombo;						//
	bool								m_bOutputUnoccAreaDir;		//
	bool								m_bOutputHorizonMap;			//
	EDisplaceOutputType m_eDisplaceOutputType;		//
	float								m_OccmapBrightenup;				// 0..100 in percent
};

#endif // __SRFPROPERTIES_H