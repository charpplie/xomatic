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

#ifndef _CRYTEK_GAHCAF_
#define _CRYTEK_GAHCAF_

#include "GlobalAnimationHeader.h"
#include "Controller.h"
#include "ControllerPQLog.h"
#include "ControllerTCB.h"

#include "IStreamEngine.h"
#include <NameCRCHelper.h>
#include <SpuUtils.h>


class CInternalSkinningInfo;
class CCommonSkinningInfo;


#include <PoolAllocator.h>
extern stl::PoolAllocatorNoMT<sizeof(SAnimationSelectionProperties)> *g_Alloc_AnimSelectProps;

enum eLoadingOptions 
{
	eLoadFullData,
	eLoadOnlyInfo,
	eLoadOnlyControllers
};

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

}_ALIGN(32);

struct SSegments
{
	f32 a,b,c,d,e,f;

	ILINE SSegments()
	{
		a=1.0f;
		b=1.0f;
		c=1.0f;
		d=1.0f;
		e=1.0f;
	};
	ILINE SSegments( f32 va, f32 vb, f32 vc, f32 vd, f32 ve, f32 vf ) { a=va; b=vb; c=vc; d=vd; e=ve; f=vf; };
	ILINE void operator () ( f32 va, f32 vb, f32 vc, f32 vd, f32 ve, f32 vf ) { a=va; b=vb; c=vc; d=vd; e=ve; f=vf; };
};

ILINE SFootPlant operator * (const SFootPlant& fp, f32 t) 
{ 
	SFootPlant rfp;
	rfp.m_LHeelStart=fp.m_LHeelStart*t;	rfp.m_LHeelEnd=fp.m_LHeelEnd*t;
	rfp.m_LToe0Start=fp.m_LToe0Start*t;	rfp.m_LToe0End=fp.m_LToe0End*t;
	rfp.m_RHeelStart=fp.m_RHeelStart*t;	rfp.m_RHeelEnd=fp.m_RHeelEnd*t;
	rfp.m_RToe0Start=fp.m_RToe0Start*t;	rfp.m_RToe0End=fp.m_RToe0End*t;
	return rfp;
}

//vector self-addition
ILINE void operator += (SFootPlant& v0, const SFootPlant& v1) 
{
	v0.m_LHeelStart+=v1.m_LHeelStart;	v0.m_LHeelEnd+=v1.m_LHeelEnd;
	v0.m_LToe0Start+=v1.m_LToe0Start;	v0.m_LToe0End+=v1.m_LToe0End;
	v0.m_RHeelStart+=v1.m_RHeelStart;	v0.m_RHeelEnd+=v1.m_RHeelEnd;
	v0.m_RToe0Start+=v1.m_RToe0Start;	v0.m_RToe0End+=v1.m_RToe0End;
}






//////////////////////////////////////////////////////////////////////////
// this is the animation information on the module level (not on the per-model level)
// it doesn't know the name of the animation (which is model-specific), but does know the file name
// Implements some services for bone binding and ref counting
struct GlobalAnimationHeaderCAF : public GlobalAnimationHeader, public IStreamCallback
{
	friend class CAnimationManager;
	friend class CAnimationSet;

	GlobalAnimationHeaderCAF ()
	{
		m_FilePathCRC32			= 0;
		m_FilePathDBACRC32	= 0;
		m_nRef_by_Model			= 0;
		m_nRef_at_Runtime		= 0;
		m_nTouchedCounter		= 0;
		m_bInitializedByAIF	= 0;

		m_fStartSec				 = -1;		// Start time in seconds.
		m_fEndSec					 = -1;		// End time in seconds.
		m_fTotalDuration	 = -1.0f;				//asset-features
		m_StartLocation.SetIdentity();		//asset-features

		m_Segments				=	1;						//asset-features 
		m_SegmentsTime[0] = 0.0f;					//asset-features
		m_SegmentsTime[1] = 1.0f;					//asset-features
		m_SegmentsTime[2] = 1.0f;					//asset-features
		m_SegmentsTime[3] = 1.0f;					//asset-features
		m_SegmentsTime[4] = 1.0f;					//asset-features

		m_pSelectionProperties = NULL;

		m_pReadStream=0;

		m_arrController = 0;
		m_nControllers = 0;
		m_nControllers2 = 0;

		m_fDistance				=	-1.0f;				//asset-features
		m_fSpeed					=	0.0f;				//asset-features
		m_fSlope				  =	0.0f;					//asset-features
		m_fTurnSpeed 			=	0.0f;					//asset-features
		m_fAssetTurn 			=	0.0f;					//asset-features
		m_vVelocity				= Vec3(0,0,0);	//asset-features
	}

	void AllocateAnimSelectProps()
	{
		assert(m_pSelectionProperties == NULL);
		m_pSelectionProperties = (SAnimationSelectionProperties*)g_Alloc_AnimSelectProps->Allocate();
		m_pSelectionProperties->init();
	}

	virtual ~GlobalAnimationHeaderCAF()
	{
		ClearControllers();
		if (m_pSelectionProperties != NULL)
			g_Alloc_AnimSelectProps->Deallocate(m_pSelectionProperties);

		if (m_pReadStream) {
			m_pReadStream->Wait();
			m_pReadStream=0;
		}
	};

	const char* GetFilePath() const {	return m_FilePath.c_str(); };
	int GetFilePathCRC32() { return m_FilePathCRC32; }
	void SetFilePath(const string& name) 
	{ 
		m_FilePath = name; 
		m_FilePathCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(name.c_str()); 
	};

	ILINE f32 GetSegmentDuration(uint32 segment ) 
	{ 
		assert(segment<=m_Segments);
		assert(segment+1 < sizeof(m_SegmentsTime)/sizeof(m_SegmentsTime[0]) );
		f32 t0	=	m_SegmentsTime[segment+0];
		f32 t1	=	m_SegmentsTime[segment+1];
		f32 t		=	t1-t0;
		return m_fTotalDuration*t; 
	};

	ILINE f32 GetSegmentDuration2(uint32 segment )
	{ 
		assert(segment>0);
		f32 t0	=	0;//m_SegmentsTime[segment-1];
		f32 t1	=	m_SegmentsTime[segment+0];
		f32 t		=	t1-t0;
		return t; 
	};

	SPU_NO_INLINE f32 NTime2KTime( f32 ntime)
	{
		ntime = min(ntime, 1.0f);
		assert(ntime>=0 && ntime<=1);
		f32 duration	=	m_fEndSec-m_fStartSec;		
		f32 start			=	m_fStartSec;		
		f32 key				= (ntime*TICKS_PER_SECOND*duration  + start*TICKS_PER_SECOND);///40.0f;
		return key;
	}


	void AddRef()
	{
		++m_nRef_by_Model;
	}

	void Release()
	{
		if (!--m_nRef_by_Model)
		{
			ClearControllers();
#if 0
			delete [] m_arrController;
			m_nControllers = 0;
			m_arrController = 0;
#endif
			m_FootPlantBits.clear();			// nobody uses the foot-plants; clean them up.
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
		for (int i = 0; i < m_nControllers; ++i)
			if (m_arrController[i]->NumRefs() > nMax)
				nMax = m_arrController[i]->NumRefs();
		//for (IController_AutoArray::iterator it = m_arrController.begin()+1; it!= m_arrController.end(); ++it)
		//	if((*it)->NumRefs() > nMax)
		//		nMax = (*it)->NumRefs();
		return nMax;
	}
#endif

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

	size_t SizeOfCAF(const bool bForceControllerCalcu = false) const;
	void LoadControllersCAF();

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject( m_FilePath );
		pSizer->AddObject( m_pSelectionProperties );
		pSizer->AddObject( g_Alloc_AnimSelectProps );

		if( m_arrController )
		{
			pSizer->AddObject( m_arrController, std::max(m_nControllers2,m_nControllers) *sizeof(IController_AutoPtr*) );
		}
		pSizer->AddObject( m_arrLocoMoveSpeedPMG );
		pSizer->AddObject( m_arrLocoHorizAnglePMG );
		pSizer->AddObject( m_arrLocoTurnSpeedPMG );
		pSizer->AddObject( m_arrLocoVertAnglePMG );	
		pSizer->AddObject( m_FootPlantBits );
		pSizer->AddObject( m_AnimEventsCAF );
		pSizer->AddObject( m_arrControllerLookupVector );		

		
		for( int i = 0 ; i < m_nControllers ; ++i )
			pSizer->AddObject( m_arrController[i].get() );		
	}

	//---------------------------------------------------------------
	IController* GetControllerByJointCRC32(uint32 nControllerID)
	{
		
		uint32 IsCreated = IsAssetCreated();
		if (IsCreated==0)
			return 0;  //doesn't exist at all. there is nothing we can do
			
		if (m_FilePathDBACRC32 && m_nControllers==0) //this CAF is part of a DBA that is not in memory
		{
#if defined(__SPU__) // SPUs should never load controllers!!!			
			snPause();
#else			
			LoadControllersCAF();
#endif
		}
		
		/*
		if (m_FilePathDBACRC32==0 && IsAssetLoaded()==0) //this is a single CAF that is not in memory
			LoadControllersCAF();

		if (m_nControllers==0 )	
			CryFatalError("asset loading error" );
		if (IsAssetLoaded()==0)	
			CryFatalError("asset loading error" );
		*/

#if defined(__SPU__) //on SPU the size is cached globaly
		int32 nSize = gControllerLookupTableSize;
#else
		int32 nSize = m_arrControllerLookupVector.size();
#endif

		// don't try to search in empty arrays
		IF( nSize == 0, false )
			return NULL;

		uint32 *arrControllerLookup = SPU_PTR_SELECT( &m_arrControllerLookupVector[0], gControllerLookup);

		int32 low = 0;
		int32 high = nSize -1;

		while (low <= high)
		{
			int32 mid = (low + high) / 2;

			if ( arrControllerLookup[mid]> nControllerID)
				high = mid - 1;
			else if (arrControllerLookup[mid] < nControllerID)
				low = mid + 1;
			else
			{
				//get pointer to controller
				IController *pController = m_arrController[mid].get();

				//prefetch controller since it will be used soon
				CryPrefetch( pController );

				//return controller
				return pController;
			}
		}
		return NULL; // not found
	}

	size_t GetControllersCount() 
	{
		return m_nControllers;
	}

	void ClearControllers() 
	{
		if (m_FilePathDBACRC32==0) 
		{
			ClearAssetProcessed();
			ClearAssetRequested();
			ClearAssetLoaded();
			if (m_arrController)
			{
				delete[] m_arrController;
				m_arrController = 0;
				m_nControllers = 0;

				// also update the lookup vector to the now empty array
				InitControllerLookup();
			}
		}
	}
//	ILINE uint32 IsAssetLoaded() const 
//	{
//		return (m_nControllers!=0);
//	}

	bool StartStreaming( bool bAsync, eLoadingOptions flags);
	void StreamOnComplete (IReadStream* pStream, unsigned nError);

	void SetAnimationHeaderFromInternalSkinningInfo(const CInternalSkinningInfo* pSkinningInfo);
	void SetAnimationHeaderFromCommonSkinningInfo(const CCommonSkinningInfo* pSkinningInfo, eLoadingOptions flags); 

	void InitControllerLookup()
	{
		if (m_nControllers > 512)
			CryLogAlways("ERROR, controller array size to big(size = %i", m_nControllers );
		m_arrControllerLookupVector.resize( m_nControllers);
		for( uint32 i = 0 ; i < m_nControllers ; ++i )
			m_arrControllerLookupVector[i] = m_arrController[i]->GetID();
	}

public:
	string m_FilePath;								//path-name - unique per-model
	uint32 m_FilePathCRC32;						//hash value for searching animations
	uint32 m_FilePathDBACRC32;				//hash value (if the file is comming from a DBA)
	SFootPlant m_FootPlantVectors;		//asset-feature
	DynArray<uint8> m_FootPlantBits;	//asset-feature
	QuatT	m_StartLocation;						//asset-feature: the original location of the animation in world-space
	f32		m_fStartSec;								//asset-feature: Start time in seconds.
	f32		m_fEndSec;									//asset-feature: End time in seconds.
	f32		m_fTotalDuration;						//asset-feature: asset-feature: total duration in seconds.
	uint16 m_nControllers;	
	uint16 m_nControllers2;	
	IController_AutoPtr* m_arrController;

	DynArray<uint32> m_arrControllerLookupVector; // use for spu to speed up controller lookup

	//Relative locator movement. This is something we must evaluate for every PMG
	DynArray<f32> m_arrLocoMoveSpeedPMG; //how fast do we move in meters per second. This is the XYZ-speed.
	DynArray<f32> m_arrLocoHorizAnglePMG; //in what direction do we move on the XY-plane 
	DynArray<f32> m_arrLocoTurnSpeedPMG; //how fast do we turn the body in radians per second
	DynArray<f32> m_arrLocoVertAnglePMG; //how strong do we move up&down (not implemented yet) 

	SAnimationSelectionProperties* m_pSelectionProperties;
	DynArray<AnimEvents> m_AnimEventsCAF;

	uint16	m_nRef_by_Model;				//counter how many models are referencing this animation
	uint16	m_nRef_at_Runtime;			//counter how many times we use this animation at run-time at the same time (needed for streaming)
	uint16	m_nTouchedCounter;			//for statistics: did we use this asset at all?
	uint8		m_bInitializedByAIF;		//Loaded from database
	uint8		m_Segments;							//asset-feature: amount of segements 
	f32			m_SegmentsTime[5];			//asset-feature: normalized-time for each segment


	Vec3  m_vVelocity;         //asset-feature: the velocity vector for this asset
	f32 m_fDistance;					//asset-feature: the absolute distance this objects is moving
	f32 m_fSpeed;							//asset-feature: speed (meters in second)
	f32 m_fSlope;							//asset-feature: uphill-downhill measured in degrees 
	f32 m_fTurnSpeed;					//asset-feature: turning speed per second 
	f32 m_fAssetTurn;					//asset-feature: radiant between first and last frame 

private:
	IReadStreamPtr m_pReadStream;
} _ALIGN(16);

#endif
