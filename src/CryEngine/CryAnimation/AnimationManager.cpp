//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File: AnimationManager.cpp
//  Implementation of Animation Manager.cpp
//
//	History:
//	January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#if !defined(__CRYCG__)
#include <CryHeaders.h>	

#include "LoaderCAF.h"
#include "ModelAnimationSet.h"
#endif // !__CRYCG__
#include "LoaderCGA.h"
#include "LoaderDBA.h"
#include <StlUtils.h>

#include "ControllerPQ.h"
#include "ControllerOpt.h"


#include "AnimationManager.h"

#if !defined(__CRYCG__)

#include <StringUtils.h>
#include "CryPath.h"

#endif

stl::PoolAllocatorNoMT<sizeof(SAnimationSelectionProperties)> *g_Alloc_AnimSelectProps = 0;





int CAnimationManager::GetGlobalAnimID_CAF(const char* pFilePath)
{
	return this->GetGlobalIDbyFilePath_CAF(pFilePath);
}
int CAnimationManager::GetGlobalAnimID_LMG(const char* pFilePath)
{
	return this->GetGlobalIDbyFilePath_LMG(pFilePath);
}
int CAnimationManager::GetGlobalAnimID_PMG(const char* pFilePath)
{
	return this->GetGlobalIDbyFilePath_PMG(pFilePath);
}


/////////////////////////////////////////////////////////////////////
// finds the animation-asset by path-name. 
// Returns -1 if no animation was found.
// Returns the animation ID if it was found.
/////////////////////////////////////////////////////////////////////
int CAnimationManager::GetGlobalIDbyFilePath_CAF( const char * sAnimFileName)
{
	int id = m_AnimationMapCAF.GetValue(sAnimFileName);
	if (id<0)
		return -1;
	uint32 numCAF = m_arrGlobalCAF.size();
	assert(id<numCAF);
	if (id>=numCAF)
		return -1;
	assert(!stricmp(m_arrGlobalCAF[id].GetFilePath(),sAnimFileName));
	return id;
}


/////////////////////////////////////////////////////////////////////
// finds the animation-asset by path-name. 
// Returns -1 if no animation was found.
// Returns the animation ID if it was found.
/////////////////////////////////////////////////////////////////////
int CAnimationManager::GetGlobalIDbyFilePath_AIM(const char * sAnimFileName)
{
#ifdef _USE_LOWERCASE
		assert (strlen(sAnimFileName)<MAX_STATIC_CHARS);
		char pStringArray[MAX_STATIC_CHARS];
		NameCRCHelper::MakeLowercase(pStringArray,sAnimFileName);
		uint32 nCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(pStringArray);
#else
		uint32 nCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(sAnimFileName);
#endif
	uint32 numAIM = m_arrGlobalAIM.size();
	for (uint32 id=0; id<numAIM; id++)
	{
		if (m_arrGlobalAIM[id].m_FilePathCRC32==nCRC32)
		{
			assert(!stricmp(m_arrGlobalAIM[id].GetFilePath(),sAnimFileName));
			return id;
		}
	}
	return -1;
}


/////////////////////////////////////////////////////////////////////
// finds the animation-asset by path-name. 
// Returns -1 if no animation was found.
// Returns the animation ID if it was found.
/////////////////////////////////////////////////////////////////////
int CAnimationManager::GetGlobalIDbyFilePath_LMG(const char * sAnimFileName)
{
#ifdef _USE_LOWERCASE
		assert (strlen(sAnimFileName)<MAX_STATIC_CHARS);
		char pStringArray[MAX_STATIC_CHARS];
		NameCRCHelper::MakeLowercase(pStringArray,sAnimFileName);
		uint32 nCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(pStringArray);
#else
		uint32 nCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(sAnimFileName);
#endif
	uint32 numLMG = m_arrGlobalLMG.size();
	for (uint32 id=0; id<numLMG; id++)
	{
		if (m_arrGlobalLMG[id].m_FilePathCRC32==nCRC32)
		{
			assert(!stricmp(m_arrGlobalLMG[id].GetFilePath(),sAnimFileName));
			return id;
		}
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////
// finds the animation-asset by path-name. 
// Returns -1 if no animation was found.
// Returns the animation ID if it was found.
/////////////////////////////////////////////////////////////////////
int CAnimationManager::GetGlobalIDbyFilePath_PMG(const char * sAnimFileName)
{
#ifdef _USE_LOWERCASE
		assert (strlen(sAnimFileName)<MAX_STATIC_CHARS);
		char pStringArray[MAX_STATIC_CHARS];
		NameCRCHelper::MakeLowercase(pStringArray,sAnimFileName);
		uint32 nCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(pStringArray);
#else
		uint32 nCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(sAnimFileName);
#endif
	uint32 numPMG = m_arrGlobalPMG.size();
	for (uint32 id=0; id<numPMG; id++)
	{
		if (m_arrGlobalPMG[id].m_FilePathCRC32==nCRC32)
		{
			assert(!stricmp(m_arrGlobalPMG[id].GetFilePath(),sAnimFileName));
			return id;
		}
	}
	return -1;
}


////////////////////////////////////////////////////////////////////////////////////
// loads the animation with the specified name; if the animation is already loaded,
// then just returns its id
// The caller MUST TAKE CARE to bind the animation if it's already loaded before it has registered itself within this manager
// RETURNS:
//   The global animation id.
//   -1 if the animation couldn't be loaded 
// SIDE EFFECT NOTES:
//   This function does not put up a warning in the case the file was not found.
//   The caller must do so. 
int CAnimationManager::CreateGAH_CAF( const string& strFilePath )
{
	int nGlobalAnimId = GetGlobalIDbyFilePath_CAF(strFilePath);
	if (nGlobalAnimId<0)
	{
		nGlobalAnimId = int32(m_arrGlobalCAF.size());
			// add a new animation structure that will hold the info about the new animation.
			// the new animation id is the index of this structure in the array
			m_arrGlobalCAF.push_back(GlobalAnimationHeaderCAF());
		m_arrGlobalCAF[nGlobalAnimId].SetFilePath(strFilePath);
		m_AnimationMapCAF.InsertValue(m_arrGlobalCAF[nGlobalAnimId].GetFilePathCRC32(), nGlobalAnimId);
		}
	return nGlobalAnimId;
}


int CAnimationManager::CreateGAH_AIM(const string& strFilePath)
{
	int nGlobalAnimId = GetGlobalIDbyFilePath_AIM(strFilePath);
	if (nGlobalAnimId<0)
	{
		nGlobalAnimId = int32(m_arrGlobalAIM.size());
			// add a new animation structure that will hold the info about the new animation.
			// the new animation id is the index of this structure in the array
			m_arrGlobalAIM.push_back(GlobalAnimationHeaderAIM());
		m_arrGlobalAIM[nGlobalAnimId].SetFilePath(strFilePath);
		//	m_AnimationMapAIM.InsertValue(m_arrGlobalAIM[nGlobalAnimId].GetFilePathCRC32(), nGlobalAnimId);
		}
	return nGlobalAnimId;
}


int CAnimationManager::CreateGAH_LMG(const string& strFilePath)
{
	int nGlobalAnimId = GetGlobalIDbyFilePath_LMG(strFilePath);
	if (nGlobalAnimId<0)
	{
		nGlobalAnimId = int32(m_arrGlobalLMG.size());
			// add a new animation structure that will hold the info about the new animation.
			// the new animation id is the index of this structure in the array
			m_arrGlobalLMG.push_back(GlobalAnimationHeaderLMG());
		m_arrGlobalLMG[nGlobalAnimId].SetFilePath(strFilePath);
		//	m_AnimationMapLMG.InsertValue(m_arrGlobalLMG[nGlobalAnimId].GetFilePathCRC32(), nGlobalAnimId);
		}
	return nGlobalAnimId;
}



int CAnimationManager::CreateGAH_PMG( const string& strFilePath)
{
	int nGlobalAnimId = GetGlobalIDbyFilePath_PMG(strFilePath);
	if (nGlobalAnimId<0)
	{
		nGlobalAnimId = int32(m_arrGlobalPMG.size());
			// add a new animation structure that will hold the info about the new animation.
			// the new animation id is the index of this structure in the array
			m_arrGlobalPMG.push_back(GlobalAnimationHeaderPMG());
		m_arrGlobalPMG[nGlobalAnimId].SetFilePath(strFilePath);
		//	m_AnimationMapPMG.InsertValue(m_arrGlobalPMG[nGlobalAnimId].GetFilePathCRC32(), nGlobalAnimId);
		}
	return nGlobalAnimId;
}




//////////////////////////////////////////////////////////////////////////
// Loads (from file) the animation controllers into the existing animation record.
// May be used to load the animation the first time, or reload it upon request.
// Does nothing if there are already controls in the existing animation record.
/*
bool CAnimationManager::InitGAHCAF_fromDBA(GlobalAnimationHeaderCAF& rGAH, bool bSuppressWarning)
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);
	const CCommonSkinningInfo* pSkinningInfo=0; 
	uint32 nDBACRC32=0;
	uint32 numDBAs = m_arrGlobalHeaderDBA.size();
	for (uint32 i=0; i<numDBAs; i++)
	{
		if (m_arrGlobalHeaderDBA[i]->m_pDatabaseInfo==0)
			m_arrGlobalHeaderDBA[i]->StreamDatabaseDBA();
		assert(m_arrGlobalHeaderDBA[i]->m_pDatabaseInfo);

		const char* pFilePath = rGAH.GetFilePath();
		pSkinningInfo = m_arrGlobalHeaderDBA[i]->GetSkinningInfoDBA(pFilePath);
		nDBACRC32     = m_arrGlobalHeaderDBA[i]->m_FilePathDBACRC32;
		if (pSkinningInfo)
			break;
	}


	rGAH.ClearAssetNotFound();
	rGAH.ClearAssetRequested();
	if (pSkinningInfo == 0)
	{
		rGAH.OnAssetNotFound();
#ifndef PS3
		if (!bSuppressWarning) 
		{
			const char* pPathName=rGAH.GetFilePath();
			g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,rGAH.GetFilePath(),	"Failed to load animation CAF file" );
		}
#endif
		return false;
	}

	rGAH.SetAnimationHeaderFromCommonSkinningInfo(pSkinningInfo, eLoadFullData);

	//----------------------------------------------------------------------------------------------

	rGAH.m_FilePathDBACRC32 = nDBACRC32;

	uint32 numController = pSkinningInfo->m_pControllers.size();
	if (rGAH.m_arrController && rGAH.m_nControllers!=numController)
	{
		delete[] (rGAH.m_arrController);
		rGAH.m_arrController = new  IController_AutoPtr[numController];
	}
	if (rGAH.m_arrController==0)
		rGAH.m_arrController = new  IController_AutoPtr[numController];

	for(uint32 i=0; i<numController; i++ )
		rGAH.m_arrController[i] = pSkinningInfo->m_pControllers[i];
	rGAH.m_nControllers  = numController;
	rGAH.m_nControllers2 = numController;

	std::sort(rGAH.m_arrController,	rGAH.m_arrController + numController, AnimCtrlSortPred()	);

	rGAH.InitControllerLookup();
	rGAH.OnAssetCreated();
	rGAH.OnAssetLoaded();
	rGAH.ClearAssetRequested();

	return true;
}*/



//////////////////////////////////////////////////////////////////////////
// Loads (from file) the animation controllers into the existing animation record.
// May be used to load the animation the first time, or reload it upon request.
// Does nothing if there are already controls in the existing animation record.
bool CAnimationManager::InitGAHCAF_fromDBA(GlobalAnimationHeaderCAF& rGAH, bool bSuppressWarning)
{
	if (rGAH.m_bInitializedByAIF)
	{
		rGAH.OnAssetCreated();
		rGAH.OnAssetLoaded();
		rGAH.ClearAssetRequested();
		return true;
	}


	LOADING_TIME_PROFILE_SECTION(g_pISystem);
	const CCommonSkinningInfo* pSkinningInfo=0; 
	uint32 nDBACRC32=0;
	const char* pFilePath = rGAH.GetFilePath();
	uint32 numDBAs = m_arrGlobalHeaderDBA.size();
	for (uint32 i=0; i<numDBAs; i++)
	{
		if (m_arrGlobalHeaderDBA[i]->m_pDatabaseInfo==0)
			continue;

		if (m_arrGlobalHeaderDBA[i]->m_pDatabaseInfo==0)
			m_arrGlobalHeaderDBA[i]->StreamDatabaseDBA();
		assert(m_arrGlobalHeaderDBA[i]->m_pDatabaseInfo);

		/*
		const char*	pDBAName = m_arrGlobalHeaderDBA[i]->m_strFilePathDBA;
		if ( strcmp(pDBAName,"animations/human/male/weapons/javelin.dba")==0 )
		{
			uint32 sdsd=0;
			pSkinningInfo = m_arrGlobalHeaderDBA[i]->GetSkinningInfoDBA(pFilePath);
			assert(pSkinningInfo==0);
		}*/

		pSkinningInfo = m_arrGlobalHeaderDBA[i]->GetSkinningInfoDBA(pFilePath);
		nDBACRC32     = m_arrGlobalHeaderDBA[i]->m_FilePathDBACRC32;
		if (pSkinningInfo)
			break;
	}

	rGAH.ClearAssetNotFound();
	rGAH.ClearAssetRequested();
	if (pSkinningInfo == 0)
	{
		rGAH.OnAssetNotFound();
#ifndef PS3
		if (!bSuppressWarning) 
		{
			const char* pPathName=rGAH.GetFilePath();
			g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,rGAH.GetFilePath(),	"Failed to load animation CAF file" );
		}
#endif
		return false;
	}



	//----------------------------------------------------------------------------------------------

	if (rGAH.m_bInitializedByAIF)
	{
		uint32 numController = pSkinningInfo->m_pControllers.size();
		assert(rGAH.IsAssetOnDemand()==0);
		assert(rGAH.m_FilePathDBACRC32==nDBACRC32);

		int32 nStartKey				= pSkinningInfo->m_nStart;
		int32 nEndKey					= pSkinningInfo->m_nEnd;
		uint32 IsAdditive     = (pSkinningInfo->m_nAnimFlags & CA_ASSET_ADDITIVE);
		if (IsAdditive)
			nStartKey++;
		int32 fTicksPerFrame  = TICKS_PER_FRAME;
		f32		fSecsPerTick		= SECONDS_PER_TICK;
		f32		fSecsPerFrame		= fSecsPerTick * fTicksPerFrame;
		f32 fStartSec = nStartKey * fSecsPerFrame;
		f32 fEndSec   = nEndKey   * fSecsPerFrame;

		assert( fabsf(rGAH.m_fStartSec-fStartSec)<0.0001f );
		assert( fabsf(rGAH.m_fEndSec-fEndSec)<0.0001f );
	
		assert(rGAH.m_fSpeed		== pSkinningInfo->m_fSpeed);
		assert(rGAH.m_fDistance == pSkinningInfo->m_fDistance);
		assert(rGAH.m_fSlope		== pSkinningInfo->m_fSlope);

		assert(rGAH.m_FootPlantVectors.m_LHeelEnd		== pSkinningInfo->m_LHeelEnd);
		assert(rGAH.m_FootPlantVectors.m_LHeelStart == pSkinningInfo->m_LHeelStart);
		assert(rGAH.m_FootPlantVectors.m_LToe0End		== pSkinningInfo->m_LToe0End);
		assert(rGAH.m_FootPlantVectors.m_LToe0Start == pSkinningInfo->m_LToe0Start);
		assert(rGAH.m_FootPlantVectors.m_RHeelEnd		== pSkinningInfo->m_RHeelEnd);
		assert(rGAH.m_FootPlantVectors.m_RHeelStart == pSkinningInfo->m_RHeelStart);
		assert(rGAH.m_FootPlantVectors.m_RToe0End		== pSkinningInfo->m_RToe0End);
		assert(rGAH.m_FootPlantVectors.m_RToe0Start == pSkinningInfo->m_RToe0Start);
		assert( rGAH.m_StartLocation.IsEquivalent(pSkinningInfo->m_StartPosition,0.001f) );

		assert(rGAH.m_arrController);
		assert(rGAH.m_nControllers2 == numController);


	//	for(uint32 i=0; i<numController; i++ )
	//		rGAH.m_arrController[i] = pSkinningInfo->m_pControllers[i];
	//	std::sort(rGAH.m_arrController,	rGAH.m_arrController + numController, AnimCtrlSortPred()	);
	//	rGAH.m_nControllers  = numController;
	//	rGAH.m_nControllers2 = numController;
		rGAH.InitControllerLookup();

		rGAH.OnAssetCreated();
		rGAH.OnAssetLoaded();
		rGAH.ClearAssetRequested();
		return true;
	}



	//	rGAH.SetAnimationHeaderFromCommonSkinningInfo(pSkinningInfo, eLoadFullData);
	{
		rGAH.OnAssetCreated();
		//----------------------------------------------------------------------------------------------
		int32 nStartKey				= pSkinningInfo->m_nStart;
		int32 nEndKey					= pSkinningInfo->m_nEnd;
		if (pSkinningInfo->m_nAnimFlags & CA_ASSET_ADDITIVE)
			nStartKey++;
		int32 fTicksPerFrame  = TICKS_PER_FRAME;
		f32		fSecsPerTick		= SECONDS_PER_TICK;
		f32		fSecsPerFrame		= fSecsPerTick * fTicksPerFrame;
		rGAH.m_fStartSec = nStartKey * fSecsPerFrame;
		rGAH.m_fEndSec   = nEndKey   * fSecsPerFrame;

		if(rGAH.m_fEndSec<=rGAH.m_fStartSec)
			rGAH.m_fEndSec  = rGAH.m_fStartSec; //+(1.0f/30.0f);

		rGAH.m_fTotalDuration = rGAH.m_fEndSec - rGAH.m_fStartSec;


		//----------------------------------------------------------------
		//Initialize from chunk. Don't initialize at loading-time
		rGAH.m_fSpeed		= pSkinningInfo->m_fSpeed;
		rGAH.m_fDistance = pSkinningInfo->m_fDistance;
		rGAH.m_fSlope		= pSkinningInfo->m_fSlope;

		rGAH.m_FootPlantVectors.m_LHeelEnd		= pSkinningInfo->m_LHeelEnd;
		rGAH.m_FootPlantVectors.m_LHeelStart = pSkinningInfo->m_LHeelStart;
		rGAH.m_FootPlantVectors.m_LToe0End		= pSkinningInfo->m_LToe0End;
		rGAH.m_FootPlantVectors.m_LToe0Start = pSkinningInfo->m_LToe0Start;

		rGAH.m_FootPlantVectors.m_RHeelEnd		= pSkinningInfo->m_RHeelEnd;
		rGAH.m_FootPlantVectors.m_RHeelStart = pSkinningInfo->m_RHeelStart;
		rGAH.m_FootPlantVectors.m_RToe0End		= pSkinningInfo->m_RToe0End;
		rGAH.m_FootPlantVectors.m_RToe0Start = pSkinningInfo->m_RToe0Start;

		//m_MoveDirection = pSkinningInfo->m_MoveDirectionInfo; //will be replace by velocity
		rGAH.m_StartLocation = pSkinningInfo->m_StartPosition;

		if (pSkinningInfo->m_nAnimFlags & CA_ASSET_CYCLE)
			rGAH.OnAssetCycle();
		if (pSkinningInfo->m_nAnimFlags & CA_ASSET_ADDITIVE)
			rGAH.OnAssetAdditive();

		rGAH.m_FootPlantBits.resize(pSkinningInfo->m_FootPlantBits.size());
		std::copy(pSkinningInfo->m_FootPlantBits.begin(), pSkinningInfo->m_FootPlantBits.end(),rGAH.m_FootPlantBits.begin());
		//m_FootPlantBits.assign(pSkinningInfo->m_FootPlantBits.begin(), pSkinningInfo->m_FootPlantBits.end());

		//	if (m_fSpeed>=0 && (!m_FootPlantBits.empty()))
		if (rGAH.m_fSpeed>=0 && rGAH.m_fDistance>=0)
			rGAH.OnAssetProcessed();
	}


	if (rGAH.m_bInitializedByAIF==0)
	{
		uint32 numController = pSkinningInfo->m_pControllers.size();
		if (rGAH.m_arrController && rGAH.m_nControllers!=numController)
		{
			delete[] (rGAH.m_arrController);
			rGAH.m_arrController = new  IController_AutoPtr[numController];
		}
		if (rGAH.m_arrController==0)
			rGAH.m_arrController = new  IController_AutoPtr[numController];

		assert(rGAH.m_arrController);
		for(uint32 i=0; i<numController; i++ )
			rGAH.m_arrController[i] = pSkinningInfo->m_pControllers[i];
		std::sort(rGAH.m_arrController,	rGAH.m_arrController + numController, AnimCtrlSortPred()	);
		rGAH.m_nControllers  = numController;
		rGAH.m_nControllers2 = numController;
		rGAH.InitControllerLookup();
		rGAH.m_FilePathDBACRC32 = nDBACRC32;
		rGAH.OnAssetCreated();
		rGAH.OnAssetLoaded();
		rGAH.ClearAssetRequested();
	}

	return true;
}




//////////////////////////////////////////////////////////////////////////
// Loads (from file) the animation controllers into the existing animation record.
// May be used to load the animation the first time, or stream it upon request.
// Does nothing if there are already controls in the existing animation record.
bool CAnimationManager::InitGAHCAF_fromCAF(GlobalAnimationHeaderCAF& rCAF, eLoadingOptions flags, bool bAsync)
{	
	
	if (flags == eLoadOnlyInfo)
	{
		if (rCAF.m_bInitializedByAIF)
		{
			rCAF.OnAssetCreated();
			rCAF.ClearAssetRequested();
			return 0;
		}
	}

	if (bAsync)
		return rCAF.StartStreaming(bAsync, flags);

	CLoaderCAF loader;
	loader.SetLoadOnlyCommon(flags == eLoadOnlyInfo);
	loader.SetLoadOldChunks(Console::GetInst().ca_LoadUncompressedChunks > 0);

	CInternalSkinningInfo* pSkinningInfo = loader.LoadCAF(rCAF.m_FilePath, 0); 
	if (pSkinningInfo==0)
	{
#ifndef PS3
			//const char* pPathName=m_arrGlobalCAF[nGlobalAnimId].GetPathName();
			//g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,pPathName,	"Failed to load animation CAF file" );
#endif
		rCAF.OnAssetNotFound();
		return 0;
	}

	if (Console::GetInst().ca_AnimWarningLevel > 2 && loader.GetHasOldControllers())
	{
		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,rCAF.GetFilePath(),	"Animation file has uncompressed data" );
	}

	if (Console::GetInst().ca_AnimWarningLevel > 2 && !loader.GetHasNewControllers())
	{
		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,rCAF.GetFilePath(),	"Animation file has no compressed data" );
	}

	//----------------------------------------------------------------------------------------------

//	g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,rCAF.GetFilePath(),	"Pre-Loading Streaming File" );

	int32 nStartKey				= pSkinningInfo->m_nStart;
	int32 nEndKey					= pSkinningInfo->m_nEnd;
	if (pSkinningInfo->m_nAnimFlags & CA_ASSET_ADDITIVE)
		nStartKey++;

	int32 fTicksPerFrame  = TICKS_PER_FRAME;
	f32		fSecsPerTick		= SECONDS_PER_TICK;
	f32		fSecsPerFrame		= fSecsPerTick * fTicksPerFrame;
	rCAF.m_fStartSec = nStartKey * fSecsPerFrame;
	rCAF.m_fEndSec   = nEndKey   * fSecsPerFrame;
	if(rCAF.m_fEndSec<=rCAF.m_fStartSec)
		rCAF.m_fEndSec = rCAF.m_fStartSec;//+(1.0f/30.0f);
	assert(rCAF.m_fStartSec>=0);
	assert(rCAF.m_fEndSec>=0);

	rCAF.m_fTotalDuration = rCAF.m_fEndSec - rCAF.m_fStartSec;
	//	assert(rGlobalAnim.m_fTotalDuration > 0);

	//----------------------------------------------------------------

	//Initialize from chunk. Don't initialize at loading-time
	rCAF.m_fSpeed		= pSkinningInfo->m_fSpeed;
	rCAF.m_fDistance = pSkinningInfo->m_fDistance;
	rCAF.m_fSlope		= pSkinningInfo->m_fSlope;

	rCAF.m_FootPlantVectors.m_LHeelEnd		= pSkinningInfo->m_LHeelEnd;
	rCAF.m_FootPlantVectors.m_LHeelStart = pSkinningInfo->m_LHeelStart;
	rCAF.m_FootPlantVectors.m_LToe0End		= pSkinningInfo->m_LToe0End;
	rCAF.m_FootPlantVectors.m_LToe0Start = pSkinningInfo->m_LToe0Start;

	rCAF.m_FootPlantVectors.m_RHeelEnd		= pSkinningInfo->m_RHeelEnd;
	rCAF.m_FootPlantVectors.m_RHeelStart = pSkinningInfo->m_RHeelStart;
	rCAF.m_FootPlantVectors.m_RToe0End		= pSkinningInfo->m_RToe0End;
	rCAF.m_FootPlantVectors.m_RToe0Start = pSkinningInfo->m_RToe0Start;

	rCAF.m_StartLocation = pSkinningInfo->m_StartPosition;

	if (pSkinningInfo->m_nAnimFlags & CA_ASSET_CYCLE)
		rCAF.OnAssetCycle();
	if (pSkinningInfo->m_nAnimFlags & CA_ASSET_ADDITIVE)
		rCAF.OnAssetAdditive();

	rCAF.m_FootPlantBits.resize(pSkinningInfo->m_FootPlantBits.size());
	std::copy(pSkinningInfo->m_FootPlantBits.begin(), pSkinningInfo->m_FootPlantBits.end(),rCAF.m_FootPlantBits.begin());

	if (rCAF.m_fSpeed>=0 && rCAF.m_fDistance>=0)
		rCAF.OnAssetProcessed();

	//----------------------------------------------------------------

	uint32 numController = pSkinningInfo->m_pControllers.size();

	if (rCAF.m_arrController && rCAF.m_nControllers!=numController)
	{
		delete[] (rCAF.m_arrController);
		rCAF.m_arrController = new  IController_AutoPtr[numController];
	}
	if (rCAF.m_arrController==0)
		rCAF.m_arrController = new  IController_AutoPtr[numController];

	for(uint32 i=0; i<numController; i++ )
		rCAF.m_arrController[i] = (IController*)pSkinningInfo->m_pControllers[i];
	rCAF.m_nControllers = numController;
	rCAF.m_nControllers2 = numController;

	//if (numController != rGlobalAnim.m_arrController.size())
	//{
	//	g_pILog->LogError ("%d controllers (%d expected) loaded from file %s. Please re-export the file. The animations will be discarded.",
	//		numController, rGlobalAnim.m_arrController.size(), rGlobalAnim.GetPathName());
	//}

	std::sort(rCAF.m_arrController,	rCAF.m_arrController + numController, AnimCtrlSortPred() );

	rCAF.InitControllerLookup();
	rCAF.OnAssetLoaded();
	rCAF.OnAssetCreated();

	return 1;
}



//////////////////////////////////////////////////////////////////////////
// Loads (from file) the animation controllers into the existing animation record.
// May be used to load the animation the first time, or stream it upon request.
// Does nothing if there are already controls in the existing animation record.
bool CAnimationManager::InitGAHAIM_fromAIM( GlobalAnimationHeaderAIM& rAIM)
{
	CLoaderCAF loader;
	loader.SetLoadOnlyCommon(0);
	loader.SetLoadOldChunks(Console::GetInst().ca_LoadUncompressedChunks > 0);

	CInternalSkinningInfo* pSkinningInfo  = loader.LoadCAF(rAIM.m_FilePath,0);
	if (pSkinningInfo==0)
	{
		rAIM.OnAssetNotFound();
		return 0;
	}

	//----------------------------------------------------------------------------------------------

	int32 nStartKey				= pSkinningInfo->m_nStart;
	int32 nEndKey					= pSkinningInfo->m_nEnd;

	int32 fTicksPerFrame  = TICKS_PER_FRAME;
	f32		fSecsPerTick		= SECONDS_PER_TICK;
	f32		fSecsPerFrame		= fSecsPerTick * fTicksPerFrame;
	rAIM.m_fStartSec = nStartKey * fSecsPerFrame;
	rAIM.m_fEndSec   = nEndKey   * fSecsPerFrame;
	if(rAIM.m_fEndSec<=rAIM.m_fStartSec)
		rAIM.m_fEndSec  = rAIM.m_fStartSec;//+(1.0f/30.0f);
	assert(rAIM.m_fStartSec>=0);
	assert(rAIM.m_fEndSec>=0);

	rAIM.m_fTotalDuration = rAIM.m_fEndSec - rAIM.m_fStartSec;

	//----------------------------------------------------------------

	uint32 numController = pSkinningInfo->m_pControllers.size();
	rAIM.m_arrController = new IController_AutoPtr[numController];
	rAIM.m_nControllers = numController;
	for(uint32 i=0; i<numController; i++ )
		rAIM.m_arrController[i] = (IController*)pSkinningInfo->m_pControllers[i];

	std::sort(rAIM.m_arrController,	rAIM.m_arrController + numController, AnimCtrlSortPred() );

	//------------------------------------------------------------------------------

	rAIM.OnAssetLoaded();
	rAIM.OnAssetCreated();
	return 1;
}




//////////////////////////////////////////////////////////////////////////
// Loads the animation controllers into the existing animation record.
// May be used to load the animation the first time, or reload it upon request.
// Does nothing if there are already controls in the existing animation record.
bool CAnimationManager::LoadAnimationTCB(int nAnimId, std::vector<CControllerTCB>& arrNodeAnims, CryCGALoader* pCGA, uint32 unique_model_id )
{
	//	return false;
	LOADING_TIME_PROFILE_SECTION(g_pISystem);

	GlobalAnimationHeaderCAF& rGlobalAnim = m_arrGlobalCAF[nAnimId];
	int32 nStartKey				= pCGA->m_start;
	int32 nEndKey					= pCGA->m_end;

	int32 fTicksPerFrame  = TICKS_PER_FRAME;
	f32		fSecsPerTick		= SECONDS_PER_TICK;
	f32		fSecsPerFrame		= fSecsPerTick * fTicksPerFrame;
	rGlobalAnim.m_fStartSec = nStartKey * fSecsPerFrame;
	rGlobalAnim.m_fEndSec   = nEndKey   * fSecsPerFrame;
	if(rGlobalAnim.m_fEndSec<=rGlobalAnim.m_fStartSec)
		rGlobalAnim.m_fEndSec  = rGlobalAnim.m_fStartSec;//+(1.0f/30.0f);

	rGlobalAnim.m_fTotalDuration = rGlobalAnim.m_fEndSec - rGlobalAnim.m_fStartSec;
	//	assert(rGlobalAnim.m_fTotalDuration > 0);

	uint32 numCtrl = rGlobalAnim.m_nControllers;
	assert(numCtrl==0);
	if (numCtrl)
		return true;

	uint32 numController = 0;
	uint32 numAnimSize = arrNodeAnims.size();
	for(uint32 i=0; i<numAnimSize; i++)
	{
		uint32 numPos = arrNodeAnims[i].m_posTrack.num_keys();
		uint32 numRot = arrNodeAnims[i].m_rotTrack.num_keys();
		uint32 numScl = arrNodeAnims[i].m_sclTrack.num_keys();
		if (numPos+numRot+numScl) 
			numController++;
	}
	//	rGlobalAnim.m_arrController.resize(numController);
	rGlobalAnim.m_arrController = new IController_AutoPtr[numController];
	rGlobalAnim.m_nControllers = numController;

	for(uint32 i=0, j = 0; i<numAnimSize; i++)
	{
		uint32 numPos = arrNodeAnims[i].m_posTrack.num_keys();
		uint32 numRot = arrNodeAnims[i].m_rotTrack.num_keys();
		uint32 numScl = arrNodeAnims[i].m_sclTrack.num_keys();
		if (numPos+numRot+numScl) 
		{
			CControllerTCB* pControllerTCB = new CControllerTCB();
			*pControllerTCB =	arrNodeAnims[i];

			pControllerTCB->m_nControllerId=unique_model_id+i;	//g_nControllerJID+i;

			IController* pController = (IController*)pControllerTCB;
			PREFAST_SUPPRESS_WARNING(6386)
			rGlobalAnim.m_arrController[j] = static_cast<IController*>(pController);
			++j;
		}
	}

	/*std::sort( rGlobalAnim.m_arrController.begin(), rGlobalAnim.m_arrController.end(), AnimCtrlSortPred() );*/
	std::sort( rGlobalAnim.m_arrController, rGlobalAnim.m_arrController + numController, AnimCtrlSortPred() );

	rGlobalAnim.InitControllerLookup();	
	rGlobalAnim.OnAssetLoaded();
	rGlobalAnim.OnAssetCreated();

	return true;
}














// notifies the controller manager that this client doesn't use the given animation any more.
// these calls must be balanced with AnimationAddRef() calls
void CAnimationManager::AnimationRelease (int nGlobalAnimId, CAnimationSet* pClient)
{

	int32 numGlobalAnims = (int32)m_arrGlobalCAF.size();
	assert (nGlobalAnimId<numGlobalAnims && nGlobalAnimId>=0);

	if (nGlobalAnimId<0)
		return;
	if (nGlobalAnimId>=numGlobalAnims)
		return;

	int32 RefCount2 = m_arrGlobalCAF[nGlobalAnimId].m_nRef_by_Model;

	// if the given client is still interested in unload events, it will receive them all anyway,
	// so we don't force anything but pure release. Normally during this call the client doesn't need
	// any information about the animation being released
	m_arrGlobalCAF[nGlobalAnimId].Release();

	int32 RefCount = m_arrGlobalCAF[nGlobalAnimId].m_nRef_by_Model;
	if (RefCount==0)
		m_arrGlobalCAF[nGlobalAnimId].m_nFlags = 0;
}




// puts the size of the whole subsystem into this sizer object, classified,
// according to the flags set in the sizer
void CAnimationManager::GetSize(class ICrySizer* pSizer)
{
#if ENABLE_GET_MEMORY_USAGE
	SIZER_SUBCOMPONENT_NAME(pSizer, "AnimationKeys");
	size_t nSize = sizeof(CAnimationManager);

	nSize += m_AnimationMapCAF.GetAllocMemSize();
	uint32 numCAF = m_arrGlobalCAF.size();
	nSize += m_arrGlobalCAF.get_alloc_size(); 
	nSize -= numCAF*sizeof(GlobalAnimationHeaderCAF);
	for (uint32 i=0; i<numCAF; i++)
		nSize += m_arrGlobalCAF[i].SizeOfCAF();

	//	nSize += m_AnimationMapAIM.GetSize();
	uint32 numAIM = m_arrGlobalAIM.size();
	nSize += m_arrGlobalAIM.get_alloc_size(); 
	nSize -= numAIM*sizeof(GlobalAnimationHeaderAIM);
	for (uint32 i=0; i<numAIM; i++)
		nSize += m_arrGlobalAIM[i].SizeOfAIM();

	//	nSize += m_AnimationMapLMG.GetAllocMemSize();
	uint32 numLMG = m_arrGlobalLMG.size();
	nSize += m_arrGlobalLMG.get_alloc_size(); 
	nSize -= numLMG*sizeof(GlobalAnimationHeaderLMG);
	for (uint32 i=0; i<numLMG; i++)
		nSize += m_arrGlobalLMG[i].SizeOfLMG();

	//	nSize += m_AnimationMapPMG.GetAllocMemSize();
	uint32 numPMG = m_arrGlobalPMG.size();
	nSize += m_arrGlobalPMG.get_alloc_size(); 
	nSize -= numPMG*sizeof(GlobalAnimationHeaderPMG);
	for (uint32 i=0; i<numPMG; i++)
		nSize += m_arrGlobalPMG[i].SizeOfThis();

	//calculate size of DBAs
	uint32 numDBAs = m_arrGlobalHeaderDBA.size();
	for (uint32 i=0; i<numDBAs; i++)
		nSize += m_arrGlobalHeaderDBA[i]->SizeOf_DBA();

	pSizer->AddObject(this, nSize);

#endif
}

size_t CAnimationManager::GetSizeOfDBA()
{
	size_t nSize=0;
	uint32 numDBAs = m_arrGlobalHeaderDBA.size();
	for (uint32 i=0; i<numDBAs; i++)
		nSize += m_arrGlobalHeaderDBA[i]->SizeOf_DBA();
	return nSize;
}

void CAnimationManager::GetMemoryUsage(class ICrySizer* pSizer) const
{
	SIZER_SUBCOMPONENT_NAME(pSizer, "AnimationKeys");
	pSizer->AddObject( m_AnimationMapCAF );
	pSizer->AddObject( m_arrGlobalCAF );
	pSizer->AddObject( m_arrGlobalAIM );
	pSizer->AddObject( m_arrGlobalLMG );
	pSizer->AddObject( m_arrGlobalPMG );
	pSizer->AddObject( m_arrGlobalHeaderDBA );
}




int32 CAnimationManager::GetAnimEventsCount_CAF(int32 nGlobalID) const
{
	int32 numAnims = (int32)m_arrGlobalCAF.size();
	if (nGlobalID<0)
		return -1;
	if (nGlobalID >= numAnims)
		return -1;
	uint32 numAnimEvents = m_arrGlobalCAF[nGlobalID].m_AnimEventsCAF.size();
	return (int)numAnimEvents;
} 
int32 CAnimationManager::GetAnimEventsCount_CAF(const char* pFilePath )
{
	//string sAnimFileName = pFilePath;
	int32 nGlobalID = GetGlobalIDbyFilePath_CAF(pFilePath );
	return GetAnimEventsCount_CAF(nGlobalID);
}


void CAnimationManager::AddAnimEvent_CAF(int nGlobalID, const char* pName, const char* pParameter, const char* pBone, float fTime, const Vec3& vOffset, const Vec3& vDir, const char* pModel)
{
	if (nGlobalID >= 0 && nGlobalID < int(m_arrGlobalCAF.size()))
	{
		AnimEvents aevents;
		aevents.m_time=fTime;
		aevents.m_strCustomParameter=pParameter;
		aevents.m_strEventName=pName;
		aevents.m_strBoneName=pBone;
		aevents.m_vOffset=vOffset;
		aevents.m_vDir=vDir;
		aevents.m_strModelName=pModel;
		m_arrGlobalCAF[nGlobalID].m_AnimEventsCAF.push_back(aevents);
	}
}
void CAnimationManager::AddAnimEvent_LMG(int nGlobalID, const char* pName, const char* pParameter, const char* pBone, float fTime, const Vec3& vOffset, const Vec3& vDir, const char* pModel)
{
	if (nGlobalID >= 0 && nGlobalID < int(m_arrGlobalCAF.size()))
	{
		AnimEvents aevents;
		aevents.m_time=fTime;
		aevents.m_strCustomParameter=pParameter;
		aevents.m_strEventName=pName;
		aevents.m_strBoneName=pBone;
		aevents.m_vOffset=vOffset;
		aevents.m_vDir=vDir;
		aevents.m_strModelName=pModel;
		m_arrGlobalLMG[nGlobalID].m_AnimEventsLMG.push_back(aevents);
	}
}
void CAnimationManager::AddAnimEvent_PMG(int nGlobalID, const char* pName, const char* pParameter, const char* pBone, float fTime, const Vec3& vOffset, const Vec3& vDir, const char* pModel)
{
	if (nGlobalID >= 0 && nGlobalID < int(m_arrGlobalCAF.size()))
	{
		AnimEvents aevents;
		aevents.m_time=fTime;
		aevents.m_strCustomParameter=pParameter;
		aevents.m_strEventName=pName;
		aevents.m_strBoneName=pBone;
		aevents.m_vOffset=vOffset;
		aevents.m_vDir=vDir;
		aevents.m_strModelName=pModel;
		m_arrGlobalPMG[nGlobalID].m_AnimEventsPMG.push_back(aevents);
	}
}


void CAnimationManager::DeleteAllEventsForCAF(int nGlobalID)
{
	if (nGlobalID<0)
		return;
	uint32 numCAF = m_arrGlobalCAF.size();
	if (nGlobalID < numCAF)
		m_arrGlobalCAF[nGlobalID].m_AnimEventsCAF.clear();
}

void CAnimationManager::DeleteAllEventsForLMG(int nGlobalID)
{
	if (nGlobalID<0)
		return;
	uint32 numLMG = m_arrGlobalLMG.size();
	if (nGlobalID < numLMG)
		m_arrGlobalLMG[nGlobalID].m_AnimEventsLMG.clear();
}

void CAnimationManager::DeleteAllEventsForPMG(int nGlobalID)
{
	if (nGlobalID<0)
		return;
	uint32 numPMG = m_arrGlobalPMG.size();
	if (nGlobalID < numPMG)
		m_arrGlobalPMG[nGlobalID].m_AnimEventsPMG.clear();
}






//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
bool CAnimationManager::CreateGlobalHeaderDBA(DynArray<string>& arrFilePathDBA)
{
	uint32 numDBAs = arrFilePathDBA.size();
	for (size_t d=0; d<numDBAs; d++)
	{
		const char* pFilePathDBA = arrFilePathDBA[d];

		uint32 nHasHeader=0;
		uint32 numHeaders = m_arrGlobalHeaderDBA.size();
		for (uint32 dba=0; dba<numHeaders; dba++)
		{
			const char* pname = m_arrGlobalHeaderDBA[dba]->m_strFilePathDBA;
			int32 same = stricmp(pname,pFilePathDBA);
			if (same==0)
			{
				return 1;
			}
		}

		//create GloablHeaderDBA and load data
		CGlobalHeaderDBA* pHeaderDBA = new CGlobalHeaderDBA;
		pHeaderDBA->CreateDatabaseDBA( pFilePathDBA );
		m_arrGlobalHeaderDBA.push_back(pHeaderDBA);
	}
	return 0;
}


bool CAnimationManager::IsDatabaseInMeory(uint32 nDBACRC32)
{
	size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
	for (uint32 d=0; d<numDBA_Files; d++)
	{
		if (nDBACRC32==g_AnimationManager.m_arrGlobalHeaderDBA[d]->m_FilePathDBACRC32)
			return g_AnimationManager.m_arrGlobalHeaderDBA[d]->InMemory();
	}
		return 0;
}




void CAnimationManager::RemoveSkinningInfo_(const char * filename)
{
	uint32 num = m_arrGlobalHeaderDBA.size();
	for (size_t i=0; i<num; i++)
	{
		if (m_arrGlobalHeaderDBA[i]->RemoveSkinningInfoDBA(filename))
			return;
	}
}


//------------------------------------------------------------------------------
// Unloads animation from memory and remove 
//------------------------------------------------------------------------------
void CAnimationManager::UnloadAnimationCAF(int nGLobalAnimID)
{
	if (Console::GetInst().ca_UnloadAnimations==0)
		return;
	uint32 requested = m_arrGlobalCAF[nGLobalAnimID].IsAssetRequested();
	if	(requested)
		return;

	m_arrGlobalCAF[nGLobalAnimID].ClearControllers();
}
void CAnimationManager::UnloadAnimationAIM(int nGLobalAnimID)
{
	assert(m_arrGlobalAIM[nGLobalAnimID].GetControllersCount());
	m_arrGlobalAIM[nGLobalAnimID].ClearControllers();
}

//////////////////////////////////////////////////////////////////////////
void CAnimationManager::Clear()
{

}

//////////////////////////////////////////////////////////////////////////
bool GlobalAnimationHeaderCAF::StartStreaming( bool bFinishNow, eLoadingOptions flags)
{
	if (IsAssetLoaded() || IsAssetRequested())
		return true;

	// start streaming
	StreamReadParams params;
	params.dwUserData = flags;
	params.nSize = 0;
	params.pBuffer = NULL;
	params.nLoadTime = 10000;
	params.nMaxLoadTime = 1000;

	m_pReadStream = g_pISystem->GetStreamEngine()->StartRead(eStreamTaskTypeAnimation, GetFilePath(), this, &params);

	OnAssetCreated();
	OnAssetRequested();
	//m_pReadStream->Wait();
	//return !IsAssetNotFound();
	return true;
}


void GlobalAnimationHeaderCAF::StreamOnComplete(IReadStream* pStream, unsigned nError)
{
	DEFINE_PROFILER_FUNCTION();

	m_pReadStream = 0;

	if(pStream->IsError())
	{ 
		// file was not loaded successfully
		m_nFlags = 0;
		OnAssetNotFound();
		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE, GetFilePath(),	"Failed to stream CAF-file" );
		return;
	}

	CLoaderCAF loader;

	loader.SetLoadOnlyCommon(pStream->GetUserData() == eLoadOnlyInfo);
	loader.SetLoadOldChunks(Console::GetInst().ca_LoadUncompressedChunks > 0);


	CInternalSkinningInfo* pSkinningInfo  = loader.LoadCAFFromMemory((char*)pStream->GetBuffer(), pStream->GetBytesRead(), 0) ;

	if (pSkinningInfo) 
	{
		SetAnimationHeaderFromCommonSkinningInfo(pSkinningInfo, (eLoadingOptions)pStream->GetUserData());
		SetAnimationHeaderFromInternalSkinningInfo(pSkinningInfo);
	} 
	else 
	{
		m_nFlags = 0;
		OnAssetNotFound();
	}

}


void GlobalAnimationHeaderCAF::SetAnimationHeaderFromInternalSkinningInfo(const CInternalSkinningInfo* pSkinningInfo) 
{
	uint32 numController = pSkinningInfo->m_pControllers.size();
	//	m_arrController.resize(numController);
	m_arrController = new IController_AutoPtr[numController];
	m_nControllers = numController;

	for(uint32 i=0; i<numController; i++ )
		m_arrController[i] = (IController*)pSkinningInfo->m_pControllers[i];

	//if (numController != m_arrController.size())
	//{
	//	g_pILog->LogError ("%d controllers (%d expected) loaded from file %s. Please re-export the file. The animations will be discarded.",
	//		numController, m_arrController.size(), GetPathName());
	//}

	//std::stable_sort(m_arrController.begin(),	m_arrController.end(), AnimCtrlSortPred()	);
	std::sort(m_arrController,	m_arrController + numController, AnimCtrlSortPred()	);

	InitControllerLookup();
	OnAssetLoaded();
	ClearAssetRequested();
}

void GlobalAnimationHeaderCAF::SetAnimationHeaderFromCommonSkinningInfo(const CCommonSkinningInfo* pSkinningInfo, eLoadingOptions flags) 
{
	if (flags !=  eLoadOnlyControllers) 
	{
		OnAssetCreated();
		//----------------------------------------------------------------------------------------------
		int32 nStartKey				= pSkinningInfo->m_nStart;
		int32 nEndKey					= pSkinningInfo->m_nEnd;
		if (pSkinningInfo->m_nAnimFlags & CA_ASSET_ADDITIVE)
			 nStartKey++;

		int32 fTicksPerFrame  = TICKS_PER_FRAME;
		f32		fSecsPerTick		= SECONDS_PER_TICK;
		f32		fSecsPerFrame		= fSecsPerTick * fTicksPerFrame;
		m_fStartSec = nStartKey * fSecsPerFrame;
		m_fEndSec   = nEndKey   * fSecsPerFrame;

		if(m_fEndSec<=m_fStartSec)
			m_fEndSec  = m_fStartSec; //+(1.0f/30.0f);
		assert(m_fStartSec>=0);
		assert(m_fEndSec>=0);

		m_fTotalDuration = m_fEndSec - m_fStartSec;
		//		assert(m_fTotalDuration > 0);

		//----------------------------------------------------------------
		//Initialize from chunk. Don't initialize at loading-time
		m_fSpeed		= pSkinningInfo->m_fSpeed;
		m_fDistance = pSkinningInfo->m_fDistance;
		m_fSlope		= pSkinningInfo->m_fSlope;

		m_FootPlantVectors.m_LHeelEnd		= pSkinningInfo->m_LHeelEnd;
		m_FootPlantVectors.m_LHeelStart = pSkinningInfo->m_LHeelStart;
		m_FootPlantVectors.m_LToe0End		= pSkinningInfo->m_LToe0End;
		m_FootPlantVectors.m_LToe0Start = pSkinningInfo->m_LToe0Start;

		m_FootPlantVectors.m_RHeelEnd		= pSkinningInfo->m_RHeelEnd;
		m_FootPlantVectors.m_RHeelStart = pSkinningInfo->m_RHeelStart;
		m_FootPlantVectors.m_RToe0End		= pSkinningInfo->m_RToe0End;
		m_FootPlantVectors.m_RToe0Start = pSkinningInfo->m_RToe0Start;

		//m_MoveDirection = pSkinningInfo->m_MoveDirectionInfo; //will be replace by velocity
		m_StartLocation = pSkinningInfo->m_StartPosition;

		if (pSkinningInfo->m_nAnimFlags & CA_ASSET_CYCLE)
			OnAssetCycle();
		if (pSkinningInfo->m_nAnimFlags & CA_ASSET_ADDITIVE)
			OnAssetAdditive();

		m_FootPlantBits.resize(pSkinningInfo->m_FootPlantBits.size());
		std::copy(pSkinningInfo->m_FootPlantBits.begin(), pSkinningInfo->m_FootPlantBits.end(),m_FootPlantBits.begin());
		//m_FootPlantBits.assign(pSkinningInfo->m_FootPlantBits.begin(), pSkinningInfo->m_FootPlantBits.end());

		//	if (m_fSpeed>=0 && (!m_FootPlantBits.empty()))
		if (m_fSpeed>=0 && m_fDistance>=0)
			OnAssetProcessed();
	}
}

//--------------------------------------------------------------------------------------

size_t GlobalAnimationHeaderCAF::SizeOfCAF(const bool bForceControllerCalcu) const
{
	size_t nSize = sizeof(*this);

	size_t nTemp00 = m_FilePath.capacity();		nSize += nTemp00;

	size_t nTemp01 = m_pSelectionProperties ? sizeof(SAnimationSelectionProperties):0;	nSize += nTemp01;

	size_t nTemp02 = m_arrLocoMoveSpeedPMG.get_alloc_size();				nSize += nTemp02;
	size_t nTemp03 = m_arrLocoHorizAnglePMG.get_alloc_size();			  nSize += nTemp03;
	size_t nTemp04 = m_arrLocoTurnSpeedPMG.get_alloc_size();				nSize += nTemp04;
	size_t nTemp05 = m_arrLocoVertAnglePMG.get_alloc_size();				nSize += nTemp05;

	size_t nTemp06 = m_FootPlantBits.get_alloc_size();							nSize += nTemp06;
	size_t nTemp07 = m_AnimEventsCAF.get_alloc_size();							nSize += nTemp07;
	uint32 numEvents = m_AnimEventsCAF.size();
	for (uint32 i=0; i<numEvents; i++)
	{
		nSize += m_AnimEventsCAF[i].m_strModelName.capacity();
		nSize += m_AnimEventsCAF[i].m_strEventName.capacity();
		nSize += m_AnimEventsCAF[i].m_strCustomParameter.capacity();
		nSize += m_AnimEventsCAF[i].m_strBoneName.capacity();
	}

	size_t nTemp08 = m_arrControllerLookupVector.get_alloc_size();	nSize += nTemp08;
	size_t nTemp09 = m_nControllers*sizeof(IController);	nSize += nTemp09;

	if (m_FilePathDBACRC32)
	{
		bool InMem=g_AnimationManager.IsDatabaseInMeory(m_FilePathDBACRC32);
		if (InMem && bForceControllerCalcu)
		{
			for (uint16 i=0; i<m_nControllers; ++i)
				nSize += m_arrController[i]->ApproximateSizeOfThis();
		}
	} 
	else
	{
		for (uint16 i=0; i<m_nControllers; ++i)
			nSize += m_arrController[i]->SizeOfController();
	}

	return nSize;
}

//----------------------------------------------------------------------------------------

void GlobalAnimationHeaderCAF::LoadControllersCAF()
{
	uint32 OnDemand=IsAssetOnDemand();
	if (OnDemand)
	{
		g_AnimationManager.InitGAHCAF_fromCAF(*this,eLoadFullData,0);
	}
	else
	{
		if (m_nControllers==0 && m_FilePathDBACRC32)
		{
			size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
			for (uint32 d=0; d<numDBA_Files; d++)
			{
				CGlobalHeaderDBA& pGlobalHeaderDBA = *g_AnimationManager.m_arrGlobalHeaderDBA[d];
				if (m_FilePathDBACRC32!=pGlobalHeaderDBA.m_FilePathDBACRC32)
					continue;
				if (pGlobalHeaderDBA.m_pDatabaseInfo==0)
				{
					const char* pName  = pGlobalHeaderDBA.m_strFilePathDBA;
					pGlobalHeaderDBA.StreamDatabaseDBA();
				}

				if (pGlobalHeaderDBA.m_pDatabaseInfo)
				{
					const CCommonSkinningInfo* pSkinningInfo = pGlobalHeaderDBA.GetSkinningInfoDBA(m_FilePath);
					uint32 numController = pSkinningInfo->m_pControllers.size();
					if (numController!=m_nControllers2)
						CryFatalError("Controller mismatch");

					for(uint32 s=0; s<numController; s++ )
						m_arrController[s] = pSkinningInfo->m_pControllers[s];
					m_nControllers = numController;
					std::sort(m_arrController,	m_arrController + numController, AnimCtrlSortPred()	);
					InitControllerLookup();
					OnAssetCreated();
					OnAssetLoaded();
					ClearAssetRequested();
				}
				break;
			}
		}
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IAnimEvents)
