//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File: AnimationManager.h
//  Implementation of Animation Manager.h
//
//	History:
//	January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#ifndef _CRYTEK_GAHAIM_
#define _CRYTEK_GAHAIM_

#include "GlobalAnimationHeader.h"
#include "Controller.h"
#include "ControllerPQLog.h"

#include "IStreamEngine.h"
#include <NameCRCHelper.h>




#define PRECISION (0.001)

struct AimIKPose
{
	DynArray<Quat> m_arrRotation;
	DynArray<Vec3> m_arrPosition;
};

struct QuadIndices 
{ 
	uint8 i0,i1,i2,i3; 
	Vec4	w0,w1,w2,w3;
	ColorB col;
	Vec3 height;
};






//////////////////////////////////////////////////////////////////////////
// this is the animation information on the module level (not on the per-model level)
// it doesn't know the name of the animation (which is model-specific), but does know the file name
// Implements some services for bone binding and ref counting
struct GlobalAnimationHeaderAIM : public GlobalAnimationHeader
{
	friend struct CAnimationManager;
	friend class CAnimationSet;

	GlobalAnimationHeaderAIM ()
	{
		m_FilePathCRC32		=	0;
		m_nRef_by_Model		= 0;
		m_nRef_at_Runtime = 0;
		m_nTouchedCounter = 0;

		m_fStartSec				= -1;		// Start time in seconds.
		m_fEndSec					= -1;		// End time in seconds.
		m_fTotalDuration	= -1;		//asset-features

		m_nControllers		= 0;
		m_AnimTokenCRC32	= 0;

		m_nExist					=	0;
	}


	virtual ~GlobalAnimationHeaderAIM()
	{
		ClearControllers();
	};


	const char* GetFilePath() const {	return m_FilePath.c_str(); };
	int GetFilePathCRC32() { return m_FilePathCRC32; }
	void SetFilePath(const string& name) 
	{ 
		m_FilePath = name; 
		const char* pname=m_FilePath.c_str();
		m_FilePathCRC32 = g_crcGen.GetCRC32Lowercase(pname); 
	};


	void AddRef()
	{
		++m_nRef_by_Model;
	}

	void Release()
	{
	}




	//---------------------------------------------------------------
	IController* GetControllerByJointCRC32(uint32 nControllerID)
	{
		uint32 numControllers = m_arrController.size();
		for (uint32 i=0; i<numControllers; i++)
		{
			uint32 nCID = m_arrController[i]->GetID();
			if (nControllerID==nCID)
				return m_arrController[i];
		}
		return 0;
	}

	f32 NTime2KTime( f32 ntime)
	{
		ntime = min(ntime, 1.0f);
		assert(ntime>=0 && ntime<=1);
		f32 duration	=	m_fEndSec-m_fStartSec;		
		f32 start			=	m_fStartSec;		
		f32 key				= (ntime*TICKS_PER_SECOND*duration  + start*TICKS_PER_SECOND);///40.0f;
		return key;
	}

	size_t SizeOfAIM() const
	{
		size_t nSize = sizeof(*this);

		size_t nTemp00 = m_FilePath.capacity();					nSize += nTemp00;
		size_t nTemp08 = m_arrAimIKPosesAIM.get_alloc_size();				nSize += nTemp08;
		uint32 numAimPoses = m_arrAimIKPosesAIM.size();
		for(size_t i=0; i<numAimPoses; ++i)
		{
			nSize += m_arrAimIKPosesAIM[i].m_arrRotation.get_alloc_size();
			nSize += m_arrAimIKPosesAIM[i].m_arrPosition.get_alloc_size();
		}
		return nSize;
	}





	size_t GetControllersCount() 
	{
		return m_nControllers;
	}

	void ClearControllers() 
	{
		ClearAssetProcessed();
		ClearAssetRequested();
		ClearAssetLoaded();
	}


	string m_FilePath;					//path-name - unique per-model
	uint32 m_FilePathCRC32;			//hash value for searching animations

	uint16 m_nRef_by_Model;    //counter how many models are referencing this animation
	uint16 m_nRef_at_Runtime;  //counter how many times we use this animation at run-time at the same time (need for streaming)
	uint16 m_nTouchedCounter;  //do we use this asset at all?
	uint16 m_nControllers;	
	f32 m_fStartSec;					//Start time in seconds.
	f32 m_fEndSec;						//End time in seconds.
	f32 m_fTotalDuration;			//asset-feature: total duration in seconds.
//	IController_AutoPtr* m_arrController;
	TControllersVector m_arrController;


	uint32 m_AnimTokenCRC32;
	DynArray<AimIKPose> m_arrAimIKPosesAIM; //if this animation contains aim-poses, we store them here

	uint64 m_nExist;	
	Quat m_MiddleAimPoseRot;
	Quat m_MiddleAimPose;
	CHUNK_GAHAIM_INFO::VirtualExample m_PolarGrid[CHUNK_GAHAIM_INFO::XGRID * CHUNK_GAHAIM_INFO::YGRID];
	DynArray<CHUNK_GAHAIM_INFO::VirtualExampleInit2> VE2;
};




#endif
