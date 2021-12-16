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
#include "ControllerTCB.h"

#include "IStreamEngine.h"
#include <NameCRCHelper.h>


#define NEWAIMSYSTEM (1)

#if (NEWAIMSYSTEM==1)
#define AIM_POSES (1)
#define PRECISION (0.001)
#endif

#if (NEWAIMSYSTEM==0)
#define AIM_POSES (49)
#define AIM_POSES_MID (24)
#endif


struct AimIKPose
{
	DynArray<Quat> m_arrRotation;
	DynArray<Vec3> m_arrPosition;

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_arrRotation);
		pSizer->AddObject(m_arrPosition);
	}
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
	friend class CAnimationManager;
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

		m_arrController = 0;
		m_nControllers	= 0;
		m_AnimTokenCRC32= 0;

#if (NEWAIMSYSTEM)
		m_nExist				=	0;
#endif
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
#ifdef _USE_LOWERCASE
		assert (name.size() < MAX_STATIC_CHARS);
		char pStringArray[MAX_STATIC_CHARS];
		NameCRCHelper::MakeLowercase(pStringArray, name.c_str());
		m_FilePathCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(pStringArray); 
#else
		m_FilePathCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(m_Name.c_str()); 
#endif
	};


	void AddRef()
	{
		++m_nRef_by_Model;
	}

	void Release()
	{
		if (!--m_nRef_by_Model)
		{
			delete [] m_arrController;
			m_nControllers = 0;
			m_arrController = 0;
		}
	}


#ifdef _DEBUG
	// returns the maximum reference counter from all controllers. 1 means that nobody but this animation
	// structure refers to them
	int MaxControllerRefCount()
	{
		//if (m_arrController.empty())
		//	return 0;
		if (!m_arrController)
			return 0;

		int nMax = m_arrController[0]->NumRefs();
		for (int i=0; i<m_nControllers; ++i)
			if (m_arrController[i]->NumRefs() > nMax)
				nMax = m_arrController[i]->NumRefs();
		//for (IController_AutoArray::iterator it = m_arrController.begin()+1; it!= m_arrController.end(); ++it)
		//	if((*it)->NumRefs() > nMax)
		//		nMax = (*it)->NumRefs();
		return nMax;
	}
#endif



	//---------------------------------------------------------------
	IController* GetControllerByJointCRC32(uint32 nControllerID)
	{
		assert(m_nControllers);
		for (uint32 i=0; i<m_nControllers; i++)
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

	//count how many position controllers this animation has
	uint32 GetTolalPosKeys() const
	{
		uint32 pos=0;
		for (uint32 i=0; i<m_nControllers; i++)
			pos += (m_arrController[i]->GetPositionKeysNum()!=0);
		return pos;
	}

	//count how many rotation controllers this animation has
	uint32 GetTolalRotKeys() const
	{
		uint32 rot = 0;
		for (uint32 i=0; i<m_nControllers; i++)
			rot += (m_arrController[i]->GetRotationKeysNum()!=0);
		return rot;
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

		for (uint16 i = 0; i < m_nControllers; ++i)
			nSize += m_arrController[i]->ApproximateSizeOfThis();
		return nSize;
	}

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject( m_FilePath );
		pSizer->AddObject( m_arrController, m_nControllers *sizeof(IController*) );
		pSizer->AddObject( m_arrAimIKPosesAIM );		
		
		for( int i = 0 ; i < m_nControllers ; ++i )
			pSizer->AddObject( m_arrController[i].get() );
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
		if (m_arrController)
		{
			delete[] m_arrController;
			m_arrController = 0;
			m_nControllers = 0;
		}
	}


private:



public:
	string m_FilePath;					//path-name - unique per-model
	uint32 m_FilePathCRC32;			//hash value for searching animations

	uint16 m_nRef_by_Model;    //counter how many models are referencing this animation
	uint16 m_nRef_at_Runtime;  //counter how many times we use this animation at run-time at the same time (need for streaming)
	uint16 m_nTouchedCounter;  //do we use this asset at all?
	uint16 m_nControllers;	
	f32 m_fStartSec;					//Start time in seconds.
	f32 m_fEndSec;						//End time in seconds.
	f32 m_fTotalDuration;			//asset-feature: total duration in seconds.
	IController_AutoPtr* m_arrController;
	uint32 m_AnimTokenCRC32;
	DynArray<AimIKPose> m_arrAimIKPosesAIM; //if this animation contains aim-poses, we store them here

#if (NEWAIMSYSTEM)
	uint64 m_nExist;	
	Quat m_MiddleAimPoseRot;
	Quat m_MiddleAimPose;
	CHUNK_GAHAIM_INFO::VirtualExample m_PolarGrid[CHUNK_GAHAIM_INFO::XGRID*CHUNK_GAHAIM_INFO::YGRID];
	DynArray<CHUNK_GAHAIM_INFO::VirtualExampleInit2> VE2;
#endif
};




#endif
