////////////////////////////////////////////////////////////////////////////
//
// Crytek Engine Source File.
// Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
// File name: AnimationInfoLoader.h
// Version: v1.00
// Created: 22/6/2006 by Alexey Medvedev.
// Compilers: Visual Studio.NET 2005
// Description:
// -------------------------------------------------------------------------
// History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef _CRYTEK_GLOBAL_ANIMATION_HEADER_CAF
#define _CRYTEK_GLOBAL_ANIMATION_HEADER_CAF



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <CryCharAnimationParams.h>
#include "Controller.h"
#include "AnimationInfoLoader.h"
#include "IXMLSerializer.h"
#include "CGF\CGFLoader.h"
#include "CGF\CGFSaver.h"
#include "CGF\LoaderCAF.h"
#include "ControllerPQLog.h"
#include "ControllerPQ.h"
#include "SkeletonInfo.h"
#include "CGFContent.h"
#include "ConvertContext.h"
#include "CryVersion.h"
#include "CompressionController.h"
#include <crc32.h>

extern Crc32Gen g_crcGen;

//#define PRINTOUT (1)

struct SFootPlant
{
	f32 m_LHeelStart,m_LHeelEnd;
	f32 m_LToe0Start,m_LToe0End;
	f32 m_RHeelStart,m_RHeelEnd;
	f32 m_RToe0Start,m_RToe0End;

	SFootPlant()
	{
		m_LHeelStart=-10000.0f; m_LHeelEnd=-10000.0f;
		m_LToe0Start=-10000.0f; m_LToe0End=-10000.0f;
		m_RHeelStart=-10000.0f; m_RHeelEnd=-10000.0f;
		m_RToe0Start=-10000.0f; m_RToe0End=-10000.0f;
	};

};

ILINE SFootPlant operator * (const SFootPlant& fp, f32 t)
{
	SFootPlant rfp;
	rfp.m_LHeelStart=fp.m_LHeelStart*t; rfp.m_LHeelEnd=fp.m_LHeelEnd*t;
	rfp.m_LToe0Start=fp.m_LToe0Start*t; rfp.m_LToe0End=fp.m_LToe0End*t;
	rfp.m_RHeelStart=fp.m_RHeelStart*t; rfp.m_RHeelEnd=fp.m_RHeelEnd*t;
	rfp.m_RToe0Start=fp.m_RToe0Start*t; rfp.m_RToe0End=fp.m_RToe0End*t;
	return rfp;
}

//vector self-addition
ILINE void operator += (SFootPlant& v0, const SFootPlant& v1)
{
	v0.m_LHeelStart+=v1.m_LHeelStart; v0.m_LHeelEnd+=v1.m_LHeelEnd;
	v0.m_LToe0Start+=v1.m_LToe0Start; v0.m_LToe0End+=v1.m_LToe0End;
	v0.m_RHeelStart+=v1.m_RHeelStart; v0.m_RHeelEnd+=v1.m_RHeelEnd;
	v0.m_RToe0Start+=v1.m_RToe0Start; v0.m_RToe0End+=v1.m_RToe0End;
}


struct ColorRGB{ uint8 r,g,b; };

typedef std::vector<IController_AutoPtr> TControllersVector;



struct GlobalAnimationHeaderCAF : public GlobalAnimationHeader
{
	GlobalAnimationHeaderCAF ()
	{
		m_nFlags = 0;

		m_FilePathCRC32		 = 0;
		m_FilePathDBACRC32 = 0;

		m_fSecsPerTick   = 0;
		m_nTicksPerFrame = 0;
		m_nStartKey = -1;
		m_nEndKey   = -1;
		m_fStartSec = -1; // Start time in seconds.
		m_fEndSec   = -1; // End time in seconds.

		m_fDistance = -1.0f;
		m_fSpeed    = -1.0f;
		m_fSlope    =  0.0f;
		m_StartLocation2.SetIdentity();// = Vec3(ZERO);
	}

	virtual ~GlobalAnimationHeaderCAF()
	{
	}

	const char* GetFilePath() const {	return m_FilePath.c_str(); };
	int GetFilePathCRC32() { return m_FilePathCRC32; }
	void SetFilePathCAF(const string& name) 
	{ 
		m_FilePath = name; 
		const char* pname=m_FilePath.c_str();
		m_FilePathCRC32 = g_crcGen.GetCRC32Lowercase(pname); 
	}

	void SetFilePathDBA(const string& name) 
	{ 
		m_FilePathDBA = name; 
		const char* pname=m_FilePathDBA.c_str();
		m_FilePathDBACRC32 = g_crcGen.GetCRC32Lowercase(pname); 
	}

	SPU_NO_INLINE f32 NTime2KTime( f32 ntime)
	{
		ntime = min(ntime, 1.0f);
		assert(ntime>=0 && ntime<=1);
		f32 duration	=	m_fEndSec-m_fStartSec;		
		f32 start			=	m_fStartSec;		
		f32 key				= (ntime*TICKS_PER_SECOND*duration  + start*TICKS_PER_SECOND);///40.0f;
		return key;
	}

	//---------------------------------------------------------------
	IController* GetControllerByJointCRC32(uint32 nControllerID)
	{
		size_t numControllers = m_arrController.size();
		assert(numControllers);
		for (uint32 i=0; i<numControllers; i++)
		{
			uint32 nCID = m_arrController[i]->GetID();
			if (nControllerID==nCID)
				return m_arrController[i];
		}
		return 0;
	}

	IController* GetController(uint32 nControllerID);
	string m_FilePath;								//path-name - unique per-model
	uint32 m_FilePathCRC32;						//hash value for searching animations
	string m_FilePathDBA;				      //path-name of DBA (if this animation is is a DBA)
	uint32 m_FilePathDBACRC32;				//hash value (if the file is coming from a DBA)

	// timing data, retrieved from the timing_chunk_desc
	int32 m_nTicksPerFrame;
	f32 m_fSecsPerTick;
	int32 m_nStartKey;
	int32 m_nEndKey;
	f32 m_fStartSec; //Start time in seconds.
	f32 m_fEndSec; //End time in seconds.

	SFootPlant m_FootPlantVectors;
	std::vector<uint8> m_FootPlantBits;

	std::vector<CControllerInfo> m_arrControlerInfo;
	// controllers comprising the animation; within the animation, they're sorted by ids
	TControllersVector m_arrController;

	f32 m_fDistance; //the absolute distance this objects is moving
	f32 m_fSpeed; //speed (meters in second)
	f32 m_fSlope; //uphill-downhill measured in degrees
	QuatT m_StartLocation2;
};


#endif