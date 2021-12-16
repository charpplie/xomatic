////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	28/09/2004 - Created by Ivo Herzeg <ivo@crytek.de>
//
//  Contains:
//  high-level loading of characters
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <CryHeaders.h>
#include <I3DEngine.h>

#include "CharacterInstance.h"
#include "CharacterManager.h"
#include "CryAnimationScriptCommands.h"
#include "LoaderCHR.h"
#include "LoaderCGA.h"
#include "FacialAnimation/FaceAnimation.h"
#include <IGameFramework.h>
#include <ICharacterPartsManager.h>
#include "AnimationManager.h"
#include "LoaderDBA.h"

float g_YLine=0.0f;

CCharacterModel* model_thin=0;
CCharacterModel* model_fat=0;

// forward declaration to access translation unit global memory container
namespace AimPoseTempData { void GetMemoryUsage( ICrySizer *pSizer );	}

//////////////////////////////////////////////////////////////////////////
// Loads a cgf and the corresponding caf file and creates an animated object,
// or returns an existing object.
ICharacterInstance* CharacterManager::CreateInstance(const char* szCharacterFileName, uint32 IsSkinAtt, IAttachment* pIMasterAttachment)
{
	//g_pILog->LogError ("CryAnimation: creating character instance: %s", szCharacterFileName);
	//MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Characters");

	g_pIRenderer			= g_pISystem->GetIRenderer();
	g_pIPhysicalWorld	= g_pISystem->GetIPhysicalWorld();
	g_pI3DEngine			=	g_pISystem->GetI3DEngine();
	g_pAuxGeom				= g_pIRenderer->GetIRenderAuxGeom();
	LoadAnimationImageFile( "animations/animations.img","animations/DirectionalBlends.img" );

	if (!szCharacterFileName)
		return (NULL);	// to prevent a crash in the frequent case the designers will mess 
	// around with the entity parameters in the editor

	stack_string strPath = szCharacterFileName;
	CryStringUtils::UnifyFilePath(strPath);

	const char* fileExt2 = PathUtil::GetExt(strPath);
	bool IsCGA = (0 == stricmp(fileExt2,"cga"));
	bool IsCHR = (0 == stricmp(fileExt2,"chr"));
	bool IsCDF = (0 == stricmp(fileExt2,"cdf"));

#ifdef INCLUDE_FACEGEN
	bool IsCPF = (0 == stricmp(fileExt2,"cpf"));

	if (IsCPF)
	{
		//
		ICharacterPartsManager* pIPartsManager = gEnv->pGame->GetIGameFramework()->GetICharacterPartsManager();
		//
		XmlNodeRef xeRoot = g_pISystem->LoadXmlFile(strPath);
		if (xeRoot == 0)
			return NULL;

		string baseFile = xeRoot->getAttr("base");
		if (baseFile.length() == 0)
			return NULL;

		string fileExt = PathUtil::GetExt(baseFile);

		ICharacterInstance* pCharInstance = NULL;
		if (fileExt == "chr")
			pCharInstance = CreateCHRInstance( baseFile, IsSkinAtt,(CAttachment*)pIMasterAttachment ); //Loading CHR file.
		if (fileExt == "cdf")
			pCharInstance = LoadCharacterDefinition( baseFile ); //Loading CDF file.

		if (pCharInstance == NULL)
			return NULL;
		//
		CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_WARNING,"CPF file allowed for use only in ChracterEditor - use it in other place only for tests.");
		//
		ICompoundCharacter* pCC = gEnv->pGame->GetIGameFramework()->GetICharacterPartsManager()->LoadCPF(szCharacterFileName, pCharInstance);
		pCharInstance = pCC->GetCharacterInstance();
		pCharInstance->AddRef();
		//pCC->Release();
		//
		return pCharInstance;
	}
#endif

	if (IsCGA)
		return CreateCGAInstance( strPath ); //Loading CGA file.

	if (IsCHR)
		return CreateCHRInstance( strPath, IsSkinAtt,(CAttachment*)pIMasterAttachment ); //Loading CHR file.

	if (IsCDF)
		return LoadCharacterDefinition( strPath ); //Loading CDF file.

	g_pILog->LogError ("CryAnimation: no valid character file-format: %s", szCharacterFileName);
	return 0; //if it ends here, then we have no valid file-format
}

ICharacterInstance* CharacterManager::CreateCHRInstance( const char* strPath, uint32 IsSkinAtt, IAttachment* pMasterAttachment  )
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);

#ifdef MEMORY_REPORT
	//if (single)
	{
		//single = false;
		setLogAllocations(true);
	}
#endif

	uint64 membegin=0;
	if (Console::GetInst().ca_MemoryUsageLog)
	{
		//char tmp[4096];
		CryModuleMemoryInfo info;
		CryGetMemoryInfoForModule(&info);
		membegin = info.allocated - info.freed;
	}





	//IVO! this is no good implementation, but for testing it will do  
	model_thin=0;
	model_fat=0;


	CSkinInstance* pCryCharInstance;

	CCharacterModel* pModel = CheckIfModelLoaded(strPath);
	if (pModel==0)
	{
		//there is no model in memory; so lets load one
		if (Console::GetInst().ca_SkipLoadThinFat == 0) 
		{
			stack_string strPath_thin=strPath;
			PathUtil::RemoveExtension(strPath_thin);
			strPath_thin=strPath_thin+"_thin." + CRY_CHARACTER_FILE_EXT;
			if (gEnv->pCryPak->IsFileExist(strPath_thin.c_str()))
				model_thin = FetchModel(strPath_thin,1,0);

			stack_string strPath_fat=strPath;
			PathUtil::RemoveExtension(strPath_fat);
			strPath_fat=strPath_fat+"_fat." + CRY_CHARACTER_FILE_EXT;
			if (gEnv->pCryPak->IsFileExist(strPath_fat.c_str()))
				model_fat  = FetchModel(strPath_fat,1,0);
		}
		// try to find already loaded model, or load a new one
		pModel = FetchModel (strPath,0,1);

		if (model_thin)	model_thin->Release();
		if (model_fat)	model_fat->Release();
		model_thin=0;
		model_fat=0;
		if(pModel==0)
			return NULL; // the model has not been loaded

		if (IsSkinAtt==0xDeadBeef)
		{
			pCryCharInstance = new CSkinInstance (strPath, pModel, IsSkinAtt, static_cast<CAttachment*>(pMasterAttachment));
		}
		else
		{
			pCryCharInstance = new CCharInstance (strPath, pModel);
			if (pModel->m_IsProcessed==0)
			{
				pModel->m_AnimationSet.AnalyseAndModifyAnimations(strPath,pModel);
				pModel->m_IsProcessed++;
			}
			SetFootPlantsFlag( (CCharInstance*)pCryCharInstance ); 
		}
		DecideModelLockStatus(pModel, 0);

		uint32 numLODs = pModel->GetModelMeshCount();
		for (uint32 i=0; i<numLODs; i++)
		{
			pModel->GetModelMesh(i)->m_arrIntVertices.clear();
		}

		if (Console::GetInst().ca_MemoryUsageLog)
		{
			//char tmp[4096];
			CryModuleMemoryInfo info;
			CryGetMemoryInfoForModule(&info);
			g_AnimStatisticsInfo.m_iModelsSizes += info.allocated - info.freed  - membegin;
			g_pILog->UpdateLoadingScreen("Models size %i",g_AnimStatisticsInfo.GetModelsSize());
		}
	}
	else
	{
		//the model is already loaded
		if (IsSkinAtt==0xDeadBeef)
		{
			pCryCharInstance = new CSkinInstance(strPath, pModel, IsSkinAtt,static_cast<CAttachment*>(pMasterAttachment));
		}
		else
		{
			pCryCharInstance = new CCharInstance (strPath, pModel);
			if (pModel->m_IsProcessed==0)
			{
				pModel->m_AnimationSet.AnalyseAndModifyAnimations(strPath,pModel);
				pModel->m_IsProcessed++;
			}
			SetFootPlantsFlag( (CCharInstance*)pCryCharInstance ); 
		}
	}


	if (Console::GetInst().ca_MemoryUsageLog)
	{
		//char tmp[4096];
		CryModuleMemoryInfo info;
		CryGetMemoryInfoForModule(&info);
		g_pILog->UpdateLoadingScreen("CreateCHRInstance %s. Memstat %i. %i",strPath, (int)(info.allocated - info.freed), (int)membegin );
		g_AnimStatisticsInfo.m_iInstancesSizes +=  info.allocated - info.freed  - membegin;
		//		modelsSize += info.allocated - info.freed  - membegin;
		g_pILog->UpdateLoadingScreen("Instances memstat %i", g_AnimStatisticsInfo.GetInstancesSize());
		//		STLALLOCATOR_CLEANUP;
	}

	return pCryCharInstance;
}


//////////////////////////////////////////////////////////////////////////
CCharacterModel* CharacterManager::CheckIfModelLoaded (const string& strFileName)
{
	uint32 numModels = m_arrModelCache.size();
	ValidateModelCache();
	CryModelCache::iterator it = std::lower_bound (m_arrModelCache.begin(), m_arrModelCache.end(), strFileName, OrderByFileName());

	if (it != m_arrModelCache.end())
	{
		const string& strBodyFilePath = (*it)->GetFilePath();
		if (strBodyFilePath == strFileName)
			return *it;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Finds a cached or creates a new CCharacterModel instance and returns it
// returns NULL if the construction failed
CCharacterModel* CharacterManager::FetchModel (const string& strFileName, uint32 sw, uint32 la)
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);
	if (sw == 0)
	{
		g_pILog->LogToFile("Loading Character %s", strFileName.c_str());	// to file only so console is cleaner
		g_pILog->UpdateLoadingScreen(0);
	}

	CryCHRLoader CHRLoader;
	CCharacterModel* pModel = CHRLoader.LoadNewCHR(strFileName,this,sw,la);
	if (pModel)
		RegisterModel(pModel);


	return pModel;
}

//////////////////////////////////////////////////////////////////////////
ICharacterInstance* CharacterManager::CreateCGAInstance( const char* strPath )
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);

	uint64 membegin=0;
	if (Console::GetInst().ca_MemoryUsageLog)
	{
		//char tmp[4096];
		CryModuleMemoryInfo info;
		CryGetMemoryInfoForModule(&info);
		membegin = info.allocated - info.freed;
	}


	CCharacterModel* pModel = 0;
	ValidateModelCache();

	CryModelCache::iterator it = std::lower_bound (m_arrModelCache.begin(), m_arrModelCache.end(), strPath, OrderByFileName());
	if (it != m_arrModelCache.end())
	{
		const string& strBodyFilePath = (*it)->GetFilePath();
		if (strBodyFilePath == strPath)
			pModel=*it; 
	}

	if (pModel==0)
	{
		g_pILog->UpdateLoadingScreen("Loading CGA %s", strPath);

		CryCGALoader CGALoader;
		pModel = CGALoader.LoadNewCGA( strPath, this );
		if (pModel)
		{
			RegisterModel(pModel);
			//			pModel->m_AnimationSet.AnalyseAndModifyAnimations(strPath,pModel);
			//DecideModelLockStatus(pModel, nFlags);
		}
	}

	if (pModel==0)
		return NULL; // the model has not been loaded

	CCharInstance* pCryCharInstance = new CCharInstance (strPath, pModel);
	pCryCharInstance->m_SkeletonPose.InitCGASkeleton();


	pCryCharInstance->m_SkeletonPose.UpdateBBox(1);

	if (Console::GetInst().ca_MemoryUsageLog)
	{
		CryModuleMemoryInfo info;
		CryGetMemoryInfoForModule(&info);
		g_pILog->UpdateLoadingScreen("CreateCGAInstance %s. Memstat %i",strPath, (int)(info.allocated - info.freed));
		g_AnimStatisticsInfo.m_iInstancesSizes +=  info.allocated - info.freed  - membegin;
		g_pILog->UpdateLoadingScreen("Instances Memstat %i",g_AnimStatisticsInfo.GetInstancesSize());
		//		STLALLOCATOR_CLEANUP;
	}

	return pCryCharInstance;
}



void CharacterManager::RegisterModel (CCharacterModel* pModel)
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);
	m_arrModelCache.insert (std::lower_bound(m_arrModelCache.begin(), m_arrModelCache.end(), pModel->GetFilePath(), OrderByFileName()), pModel);
}


//////////////////////////////////////////////////////////////////////////
void CharacterManager::LockModel( CCharacterModel* pModel )
{
	m_arrTempLock.push_back( pModel );
}

// Deletes itself
void CharacterManager::Release()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////
IFacialAnimation* CharacterManager::GetIFacialAnimation()
{
	return m_pFacialAnimation;
}

//////////////////////////////////////////////////////////////////////////
bool IsResourceLocked( const char *filename )
{
	IResourceList *pResList = gEnv->pCryPak->GetRecorderdResourceList(ICryPak::RFOM_NextLevel);
	if (pResList)
	{
		return pResList->IsExist(filename);
	}
	return false;
}


// locks or unlocks the given body if needed, depending on the hint and console variables
void CharacterManager::DecideModelLockStatus(CCharacterModel* pModel, uint32 nHints)
{
	// there's already a character, so we can safely remove the body from the lock pool
	// basic rules:
	//  transient hint takes precedence over everything else
	//  any hint takes precedence over the console settings
	//  if there are no hints, console variable ca_KeepModels is used to determine
	//    if the model should be kept in memory all the time (ca_KeepModels 1) or not (0)
	//  Note: changing ca_KeepModels won't immediately lock or unlock all models. It will only affect
	//  models which are actively used to create new characters or destroy old ones.
	if (nHints & nHintModelTransient) 
		m_setLockedModels.erase (pModel);
	else
		if (nHints & nHintModelPersistent) 
			m_setLockedModels.insert (pModel);
		else
			if (Console::GetInst().ca_KeepModels)
				m_setLockedModels.insert (pModel);
			else
				m_setLockedModels.erase (pModel);
}



// Deletes the given body from the model cache. This is done when there are no instances using this body info.
void CharacterManager::UnregisterModel (CCharacterModel* pModel)
{
	ValidateModelCache();
	/*
	uint32 numBodies = m_arrModelCache.size();
	if (numBodies)
	{
	const char* fname = pModel->GetModelFilePath();
	uint32 i=0;
	}
	*/
	uint32 counter=0;
	uint32 numModels2 = m_arrModelCache.size();
	for (uint32 i=0; i<numModels2; i++)
	{
		if (pModel==m_arrModelCache[i])
			counter++;
	}
	assert(counter<2);


	CryModelCache::iterator itCache = std::lower_bound (m_arrModelCache.begin(), m_arrModelCache.end(), pModel, OrderByFileName());
	if (itCache != m_arrModelCache.end())
	{
		if (*itCache == pModel)
			m_arrModelCache.erase (itCache);
		else
		{
			assert (false); // there must be no duplicate name pointers here
			while (++itCache != m_arrModelCache.end())
				if (*itCache == pModel)
				{
					m_arrModelCache.erase(itCache);
					break;
				}
		}
	}
	else
		assert (false); // this pointer must always be in the cache
	ValidateModelCache();

	/*
	uint32 numModels = m_arrModelCache.size();
	for (uint32 i=0; i<numModels; i++)
	{
	CCharacterModel* pModelInCache=m_arrModelCache[i];
	const char* filename = pModelInCache->GetModelFilePath();
	uint32 tt=0;
	}
	*/

}


// returns statistics about this instance of character animation manager
// don't call this too frequently
void CharacterManager::GetStatistics(Statistics& rStats)
{
	memset (&rStats, 0, sizeof(rStats));
	rStats.numCharModels = m_arrModelCache.size();
	for (CryModelCache::const_iterator it = m_arrModelCache.begin(); it != m_arrModelCache.end(); ++it)
		rStats.numCharacters += (*it)->GetNumInstances();

	//rStats.numAnimObjectModels = rStats.numAnimObjects = m_pAnimObjectManager->NumObjects();
}


// Validates the cache
void CharacterManager::ValidateModelCache()
{
#ifdef _DEBUG
	CryModelCache::iterator itPrev = m_arrModelCache.end();
	for (CryModelCache::iterator it = m_arrModelCache.begin(); it != m_arrModelCache.end(); ++it)
	{
		if (itPrev != m_arrModelCache.end())
			assert ((*itPrev)->GetFilePath() < (*it)->GetFilePath());
	}
#endif
}



//////////////////////////////////////////////////////////////////////////
// Deletes all the cached bodies and their associated character instances
void CharacterManager::CleanupModelCache( bool bForceCleanup )
{
	m_bImmidiateInstanceDeleteMode = true; // Will force instance to immediately delete itself, not delaying deletion to next frames.

	m_setLockedModels.clear();

	m_AnimationManager.Clear();

	CleanEraseList(true);

	std::vector<CCharacterModel*> models = m_arrModelCache;
	// Make sure nothing gets deleted.
	for (int i = 0; i < (int)models.size(); i++)
	{
		models[i]->AddRef();
	}

	// Clean all attachments.
	for (int i = 0; i < (int)models.size(); i++)
	{
		models[i]->CleanupAttachments();
	}

	if (bForceCleanup)
	{
		// Clean all instances, must be after all attachments are cleaned.
		// Only do it if explicitly requested to clean all.
		for (int i = 0; i < (int)models.size(); i++)
		{
			models[i]->CleanupInstances();
		}
		// Check that no model was deleted.
		assert( m_arrModelCache.size() == models.size() );

		for (int i = 0; i < (int)models.size(); i++)
		{
			delete models[i];
		}
		// m_arrModelCache must be empty by that point because all models should have been released.
		assert( m_arrModelCache.empty() );
	}

	m_bImmidiateInstanceDeleteMode = false;
}


bool CharacterManager::OrderByFileName::operator () (const CCharacterModel* pLeft, const CCharacterModel* pRight)
{
	const char* l = pLeft->GetModelFilePath();
	const char* r =	pRight->GetModelFilePath();
	return pLeft->GetFilePath() < pRight->GetFilePath();
}
bool CharacterManager::OrderByFileName::operator () (const string& strLeft, const CCharacterModel* pRight)
{
	return strLeft < pRight->GetFilePath();
}
bool CharacterManager::OrderByFileName::operator () (const CCharacterModel* pLeft, const string& strRight)
{
	const char* l = pLeft->GetModelFilePath();
	const char* r = strRight.c_str();
	return pLeft->GetFilePath() < strRight;
}

// puts the size of the whole subsystem into this sizer object, classified,
// according to the flags set in the sizer
void CharacterManager::GetMemoryUsage(class ICrySizer* pSizer)const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(m_AnimationManager);	
	pSizer->AddObject(m_cdfFiles);	
	pSizer->AddObject(m_EraseList);
	pSizer->AddObject(g_AnimationManager);
	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "Model Data");
		pSizer->AddObject(m_arrModelCache);
	}

	AimPoseTempData::GetMemoryUsage(pSizer);

#ifndef _LIB // Only when compiling as dynamic library
	{
		//SIZER_COMPONENT_NAME(pSizer,"Strings");
		//pSizer->AddObject( (this+1),string::_usedMemory(0) );
	}
	{
		SIZER_COMPONENT_NAME(pSizer,"STL Allocator Waste");
		CryModuleMemoryInfo meminfo;
		ZeroStruct(meminfo);
		CryGetMemoryInfoForModule( &meminfo );
		pSizer->AddObject( (this+2),(uint32)meminfo.STL_wasted );
	}
#endif

}

void CharacterManager::GetModelCacheSize(class ICrySizer* pSizer)const
{
	SIZER_SUBCOMPONENT_NAME(pSizer, "Model Data");
	uint32 numModels = m_arrModelCache.size();
	for (uint32 i=0; i<numModels; i++)
		m_arrModelCache[i]->SizeOfModelData(pSizer);
}


void CharacterManager::GetCharacterInstancesSize(class ICrySizer* pSizer)const
{
	SIZER_SUBCOMPONENT_NAME(pSizer, "Character instances");

	uint32 nSize=0;
	uint32 numModelCache = m_arrModelCache.size();
	for (uint32 m=0; m<numModelCache; ++m)
	{
		CCharacterModel* pModel = m_arrModelCache[m];
		uint32 numInstances=pModel->m_RefByInstances.size();
		for (uint32 i=0; i<numInstances; i++)
		{
			CSkinInstance* pCharInstance = pModel->m_RefByInstances[i];
			nSize += pCharInstance->SizeOfCharInstance(pSizer);
		}
	}
}



//! Cleans up all resources - currently deletes all bodies and characters (even if there are references on them)
void CharacterManager::ClearResources( bool bForceCleanup )
{
	m_AnimationManager.Clear();

	// clean animations cache and statistics
	if (m_pFacialAnimation)
	{
		m_pFacialAnimation->ClearAllCaches();
	}

	if (!gEnv->IsEditor())
	{
		CleanupModelCache( bForceCleanup );
	}
	else
	{
		CleanEraseList(true);
	}
}

//------------------------------------------------------------------------
//--  average frame-times to avoid stalls and peaks in framerate
//------------------------------------------------------------------------
f32 CharacterManager::GetAverageFrameTime(f32 sec, f32 FrameTime, f32 fTimeScale, f32 LastAverageFrameTime) 
{ 
	uint32 numFT=m_arrFrameTimes.size();
	for (int32 i=(numFT-2); i>-1; i--)
		m_arrFrameTimes[i+1] = m_arrFrameTimes[i];

	m_arrFrameTimes[0] = FrameTime;

	//get smoothed frame
	uint32 FrameAmount = 1;
	if (LastAverageFrameTime)
	{
		FrameAmount = uint32(sec/LastAverageFrameTime*fTimeScale+0.5f); //average the frame-times for a certain time-period (sec)
		if (FrameAmount>numFT)	FrameAmount=numFT;
		if (FrameAmount<1)	FrameAmount=1;
	}

	f32 AverageFrameTime=0;
	for (uint32 i=0; i<FrameAmount; i++)	
		AverageFrameTime += m_arrFrameTimes[i];
	AverageFrameTime /= FrameAmount;

	//don't smooth if we pase the game
	if (FrameTime<0.0001f)
		AverageFrameTime=FrameTime;

	//	g_YLine+=66.0f;
	//	float fColor[4] = {1,0,1,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"AverageFrameTime:  Frames:%d  FrameTime:%f  AverageTime:%f", FrameAmount, FrameTime, AverageFrameTime);	
	//	g_YLine+=16.0f;

	return AverageFrameTime;
}







//DynArray<AimIKPose> g_arrAimIKPosesAIM; //if this animation contains aim-poses, we store them here


// should be called every frame
void CharacterManager::Update()
{
	DEFINE_PROFILER_FUNCTION();

	m_nUpdateCounter++;

	//update interfaces every frame
	g_YLine=16.0f;
	g_pIRenderer			= g_pISystem->GetIRenderer();
	g_pIPhysicalWorld	= g_pISystem->GetIPhysicalWorld();
	g_pI3DEngine			=	g_pISystem->GetI3DEngine();
	g_bProfilerOn			= g_pISystem->GetIProfileSystem()->IsProfiling();
	LoadAnimationImageFile( "animations/animations.img","animations/DirectionalBlends.img" );

	g_fCurrTime		= g_pITimer->GetCurrTime();

	f32 fTimeScale	=	g_pITimer->GetTimeScale();
	f32 fFrameTime	=	g_pITimer->GetFrameTime();
	if (fFrameTime>0.2f) fFrameTime = 0.2f;
	if (fFrameTime<0.0f) fFrameTime = 0.0f;
	g_AverageFrameTime = GetAverageFrameTime( 0.25f, fFrameTime, fTimeScale, g_AverageFrameTime ); 

	if (Console::GetInst().ca_DebugModelCache)
		DebugModelCache();
	if (Console::GetInst().ca_DebugAnimUsage)
		DebugAnimUsage();
//	if (gEnv->IsEditor()==0)
		DatabaseUnloading();


	extern uint32 g_AnimationUpdates;
	extern uint32 g_SkeletonUpdates;
	if (Console::GetInst().ca_DebugAnimUpdates)
	{
		float fColor[4] = {0,1,1,1};
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"nCharInstanceLoaded: %d  nSkinInstanceLoaded: %d  Total:%d",g_nCharInstanceLoaded,g_nSkinInstanceLoaded, g_nCharInstanceLoaded+g_nSkinInstanceLoaded );	
		g_YLine+=16.0f;
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"nCharInstanceDeleted: %d  nSkinInstanceDeleted: %d",g_nCharInstanceDeleted,g_nSkinInstanceDeleted );	
		g_YLine+=32.0f;



		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"AnimationUpdates: %d",g_AnimationUpdates );	
		g_YLine+=16.0f; 
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"SkeletonUpdates: %d",g_SkeletonUpdates );	
		g_YLine+=16.0f; 

		uint32 numFSU = m_arrSkeletonUpdates.size();
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"Instances with 'Force Skeleton Update': %d",numFSU );	
		g_YLine+=16.0f; 
		for (uint32 i=0; i<numFSU; i++)
		{
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"Anim:(%d)  Force:(%d)  Visible:(%d)  ModelPath: %s",m_arrAnimPlaying[i],m_arrForceSkeletonUpdates[i],m_arrVisible[i],m_arrSkeletonUpdates[i].c_str() );	
			g_YLine+=14.0f; 
		}
	}
	g_AnimationUpdates=0;
	g_SkeletonUpdates=0;

	m_arrSkeletonUpdates.resize(0);
	m_arrAnimPlaying.resize(0);
	m_arrForceSkeletonUpdates.resize(0);
	m_arrVisible.resize(0);


	if (Console::GetInst().ca_DumpUsedAnims)
	{
		DumpAssetStatistics();
		Console::GetInst().ca_DumpUsedAnims=0;
	}

	CAnimDecal::setGlobalTime(g_pITimer->GetCurrTime());

	CleanEraseList(false);
}

//----------------------------------------------------------------------

void CharacterManager::DebugModelCache()
{
	ICrySizer* pICrySizer = g_pISystem->CreateSizer();
	uint32 numModels = m_arrModelCache.size();
	for(uint32 m=0; m<numModels; m++ )
	{
		float fColor[4] = {0,1,0,1};
		CCharacterModel* pModel = m_arrModelCache[m];
		fColor[0] = f32(pModel->pCGA_Object!=0);
		const char* PathName = pModel->GetModelFilePath();
		uint32 numLODs = pModel->m_arrModelMeshes.size();

		uint32 numInstances			= pModel->m_RefByInstances.size();
		uint32 numAnims         = pModel->m_AnimationSet.GetAnimationCount();
		uint32 numSkinInstances = 0;
		uint32 numCharInstances = 0;
		uint32 nIMememory=0;
		uint32 nMMememory=pModel->SizeOfModelData(pICrySizer);
		f32 fScale=1.0f;
		for (uint32 i=0; i<numInstances; i++)
		{
			CSkinInstance* pCharInstance = pModel->m_RefByInstances[i];
			if (pCharInstance->IsSkinAttachment())
			{
				fScale=0.5f;
				numSkinInstances++;
				CSkinInstance* pSkinInstance = (CSkinInstance*)pModel->m_RefByInstances[i];
				nIMememory += pSkinInstance->SizeOfCharInstance(pICrySizer);
			}
			else
			{
				numCharInstances++;
				nIMememory += pCharInstance->SizeOfCharInstance(pICrySizer);
			}
		}

		fColor[0]*=fScale;
		fColor[1]*=fScale;
		fColor[2]*=fScale;
		fColor[3]*=fScale;
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"Model: %02d   Char: %02d  Skin: %02d imem: %07d  mmem: %07d anim: %03d  LOD: %02d %s",m,numCharInstances,numSkinInstances,nIMememory,nMMememory,numAnims,numLODs,PathName);	
		g_YLine+=16.0f;
	}
	pICrySizer->Release();
}

//--------------------------------------------------------------------

void CharacterManager::DebugAnimUsage()
{
	size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
	for (uint32 d=0; d<numDBA_Files; d++)
	{
		const char* pName  = g_AnimationManager.m_arrGlobalHeaderDBA[d]->m_strFilePathDBA;
		uint32 nDBACRC32   = g_AnimationManager.m_arrGlobalHeaderDBA[d]->m_FilePathDBACRC32;
		uint32 nUsedAssets = g_AnimationManager.m_arrGlobalHeaderDBA[d]->m_nUsedAnimations;
		size_t nSizeOfDBA  = g_AnimationManager.m_arrGlobalHeaderDBA[d]->SizeOf_DBA();
		size_t nTCount     = g_AnimationManager.m_arrGlobalHeaderDBA[d]->m_nTCount;

		float fColorGreen[4] = {0,1,0,1};
		if (g_AnimationManager.m_arrGlobalHeaderDBA[d]->m_pDatabaseInfo==0)
		{
			fColorGreen[3]=0.4f;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 0.7f, fColorGreen, false,"UsedAssets: %04d  nTCount: %08x  Size: %08d  FilePathDBA: %s",nUsedAssets,nTCount,nSizeOfDBA,pName );	
			g_YLine+=7.0f; 
		} 
		else
		{
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.1f, fColorGreen, false,"UsedAssets: %04d  nTCount: %08x  Size: %08d  FilePathDBA: %s",nUsedAssets,nTCount,nSizeOfDBA,pName );	
			g_YLine+=11.0f; 
		}
	}
	g_YLine+=16.0f; 





	float fColor[4] = {1,0,0,1};

	size_t nAnimationManager=sizeof(CAnimationManager);

	size_t nMap = g_AnimationManager.m_AnimationMapCAF.GetAllocMemSize();



	size_t nFilePathSize = 0;

	{
		uint32 numCAF    = g_AnimationManager.m_arrGlobalCAF.size();
		for (uint32 i=0; i<numCAF; i++)
			nFilePathSize += g_AnimationManager.m_arrGlobalCAF[i].m_FilePath.capacity();
		uint32 numAIM    = g_AnimationManager.m_arrGlobalAIM.size();
		for (uint32 i=0; i<numAIM; i++)
			nFilePathSize += g_AnimationManager.m_arrGlobalAIM[i].m_FilePath.capacity();
		uint32 numLMG    = g_AnimationManager.m_arrGlobalLMG.size();
		for (uint32 i=0; i<numLMG; i++)
			nFilePathSize += g_AnimationManager.m_arrGlobalLMG[i].m_FilePath.capacity();
		uint32 numPMG    = g_AnimationManager.m_arrGlobalPMG.size();
		for (uint32 i=0; i<numPMG; i++)
			nFilePathSize += g_AnimationManager.m_arrGlobalPMG[i].m_FilePath.capacity();

		numCAF = g_AnimationManager.m_arrGlobalCAF.capacity();
		numAIM = g_AnimationManager.m_arrGlobalAIM.capacity();
		numLMG = g_AnimationManager.m_arrGlobalLMG.capacity();
		numPMG = g_AnimationManager.m_arrGlobalPMG.capacity();

		size_t nEmptyHeaderSize = 0;
		nEmptyHeaderSize += numCAF*sizeof(GlobalAnimationHeaderCAF);
		nEmptyHeaderSize += numAIM*sizeof(GlobalAnimationHeaderAIM);
		nEmptyHeaderSize += numLMG*sizeof(GlobalAnimationHeaderLMG);
		nEmptyHeaderSize += numPMG*sizeof(GlobalAnimationHeaderPMG);
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.0f, fColor, false,"size of path names: %d bytes     Empty Headers: %d bytes",nFilePathSize,nEmptyHeaderSize);	
		g_YLine+=16.0f; 
	}




	//calculate size of DBAs
	size_t nDBAalloc    = g_AnimationManager.GetSizeOfDBA();
	size_t nClientAlloc = g_AnimationManager.m_arrClients.get_alloc_size();
	nAnimationManager+=nDBAalloc;
	nAnimationManager+=nClientAlloc;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.0f, fColor, false,"DBA: %d KBytes   Clients: %d KBytes  map: %d KBytes", nDBAalloc/1024, nClientAlloc/1024, nMap/1024 );	
	g_YLine+=16.0f; 
	{
		size_t nCapacity = g_AnimationManager.m_arrGlobalCAF.capacity();
		uint32 numCAF    = g_AnimationManager.m_arrGlobalCAF.size();
		size_t  nSize = g_AnimationManager.m_arrGlobalCAF.get_alloc_size(); 
		nSize -=numCAF*sizeof(GlobalAnimationHeaderCAF);
		for (uint32 i=0; i<numCAF; i++)
			nSize += g_AnimationManager.m_arrGlobalCAF[i].SizeOfCAF();

		uint32 nNumHeaders = g_AnimationManager.m_arrGlobalCAF.size();
		uint32 nLoaded = 0;
		uint32 nUsed = 0;
		for (uint32 i=0; i<nNumHeaders; i++)
		{
			nLoaded += (g_AnimationManager.m_arrGlobalCAF[i].IsAssetLoaded() != 0);
			nUsed   += (g_AnimationManager.m_arrGlobalCAF[i].m_nTouchedCounter != 0);
			if (g_AnimationManager.m_arrGlobalCAF[i].m_nRef_at_Runtime==0)
				continue;
		}


		float fColor2[4] = {1,0,0,1};
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.0f, fColor2, false,"CAF: %04d    Loaded: %04d    Used: %04d   Memory: %05d KBytes",nNumHeaders, nLoaded,nUsed,nSize/1024 );	
		g_YLine+=16.0f; 
		nAnimationManager+=nSize;
	}

	{
		size_t  nCap  = g_AnimationManager.m_arrGlobalAIM.capacity();
		uint32 numAIM = g_AnimationManager.m_arrGlobalAIM.size();
		size_t  nSize = g_AnimationManager.m_arrGlobalAIM.get_alloc_size(); 
		nSize -=numAIM*sizeof(GlobalAnimationHeaderAIM);
		for (uint32 i=0; i<numAIM; i++)
			nSize += g_AnimationManager.m_arrGlobalAIM[i].SizeOfAIM();

		uint32 nNumHeaders = g_AnimationManager.m_arrGlobalAIM.size();
		uint32 nLoaded = 0;
		uint32 nUsed = 0;
		for (uint32 i=0; i<nNumHeaders; i++)
		{
			nLoaded += (g_AnimationManager.m_arrGlobalAIM[i].IsAssetLoaded() != 0);
			nUsed   += (g_AnimationManager.m_arrGlobalAIM[i].m_nTouchedCounter != 0);
		}
		float fColor3[4] = {1,0,0,1};
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.0f, fColor3, false,"AIM: %04d    Loaded: %04d    Used: %04d   Memory: %05d KBytes",nNumHeaders, nLoaded,nUsed,nSize/1024 );	
		g_YLine+=16.0f; 
		nAnimationManager+=nSize;
	}


	{
		size_t  nCap  = g_AnimationManager.m_arrGlobalLMG.capacity();
		uint32 numLMG = g_AnimationManager.m_arrGlobalLMG.size();
		size_t  nSize = g_AnimationManager.m_arrGlobalLMG.get_alloc_size(); 
		nSize -=numLMG*sizeof(GlobalAnimationHeaderLMG);
		for (uint32 i=0; i<numLMG; i++)
			nSize += g_AnimationManager.m_arrGlobalLMG[i].SizeOfLMG();
		
		uint32 nNumHeaders = g_AnimationManager.m_arrGlobalLMG.size();
		uint32 nLoaded = 0;
		uint32 nUsed = 0;
		for (uint32 i=0; i<nNumHeaders; i++)
		{
			nLoaded += (g_AnimationManager.m_arrGlobalLMG[i].IsAssetLoaded() != 0);
			nUsed   += (g_AnimationManager.m_arrGlobalLMG[i].m_nTouchedCounter != 0);
		}
		float fColor4[4] = {1,0,0,1};
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.0f, fColor4, false,"LMG: %04d    Loaded: %04d    Used: %04d  Memory: %05d KBytes",nNumHeaders, nLoaded,nUsed,nSize/1024  );	
		g_YLine+=16.0f; 
		nAnimationManager+=nSize;
	}


	{
		size_t  nCap  = g_AnimationManager.m_arrGlobalPMG.capacity();
		uint32 numPMG = g_AnimationManager.m_arrGlobalPMG.size();
		size_t  nSize = g_AnimationManager.m_arrGlobalPMG.get_alloc_size(); 
		nSize -=numPMG*sizeof(GlobalAnimationHeaderPMG);
		for (uint32 i=0; i<numPMG; i++)
			nSize += g_AnimationManager.m_arrGlobalPMG[i].SizeOfThis();

		uint32 nNumHeaders = g_AnimationManager.m_arrGlobalPMG.size();
		uint32 nLoaded = 0;
		uint32 nUsed = 0;
		for (uint32 i=0; i<nNumHeaders; i++)
		{
			nLoaded += (g_AnimationManager.m_arrGlobalPMG[i].IsAssetLoaded() != 0);
			nUsed   += (g_AnimationManager.m_arrGlobalPMG[i].m_nTouchedCounter != 0);
		}
		float fColor5[4] = {1,0,0,1};
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.0f, fColor5, false,"PMG: %04d    Loaded: %04d    Used: %04d  Memory: %05d KBytes",nNumHeaders, nLoaded,nUsed,nSize/1024 );	
		g_YLine+=16.0f; 
		nAnimationManager+=nSize;
	}
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.0f, fColor, false,"AnimationManager: %4d",nAnimationManager/1024 );	
	g_YLine+=16.0f; 
}


//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
void CharacterManager::DatabaseUnloading()
{
	if (Console::GetInst().ca_DatabaseUnloading)
	{
		size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
	for (uint32 d=0; d<numDBA_Files; d++)
		g_AnimationManager.m_arrGlobalHeaderDBA[d]->m_nUsedAnimations=0;



	uint32 nNumHeaders = g_AnimationManager.m_arrGlobalCAF.size();
	for (uint32 i=0; i<nNumHeaders; i++)
	{
		if (g_AnimationManager.m_arrGlobalCAF[i].m_nRef_at_Runtime==0)
			continue;

		uint32 nCRC32 = g_AnimationManager.m_arrGlobalCAF[i].m_FilePathDBACRC32;
		if (nCRC32)
		{
			for (uint32 d=0; d<numDBA_Files; d++)
			{
				if (nCRC32==g_AnimationManager.m_arrGlobalHeaderDBA[d]->m_FilePathDBACRC32)
				{
					g_AnimationManager.m_arrGlobalHeaderDBA[d]->m_nUsedAnimations++;
					/*		if (nCRC32 == 0xb8c75347)
					continue;
					const char* pn=g_AnimationManager.m_arrGlobalCAF[i].m_FilePath;
					const char* e0 = strstr(pn,"male");
					if (e0)
					continue;
					const char* e1 = strstr(pn,"alien");
					if (e1)
					continue;
					float fColorGreen[4] = {0,1,0,1};
					g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColorGreen, false,"Used: %04d    name: %s",g_AnimationManager.m_arrGlobalCAF[i].m_nRef_at_Runtime,g_AnimationManager.m_arrGlobalCAF[i].m_FilePath );	
					g_YLine+=12.0f; */

				}
			}
		}
	}


		float fColor[4] = {1,0,0,1};
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"DatabaseUnloading");	
		//	g_YLine+=12.0f; 

		for (uint32 d=0; d<numDBA_Files; d++)
		{
			CGlobalHeaderDBA& rGHDBA = *g_AnimationManager.m_arrGlobalHeaderDBA[d];
			if (rGHDBA.m_pDatabaseInfo==0)
				continue;

			const char* pName  = rGHDBA.m_strFilePathDBA;
			uint32 nDBACRC32   = rGHDBA.m_FilePathDBACRC32;
			if (rGHDBA.m_nUsedAnimations)
			{
				rGHDBA.m_nTCount=0;
				continue;
			}


			rGHDBA.m_nTCount++;
			if (rGHDBA.m_nTCount < 0x00c0)
				continue;

			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"Scanning: %s",pName );	
			//	g_YLine+=12.0f; 

			uint32 nNumHeadersCAF= g_AnimationManager.m_arrGlobalCAF.size();
			for (uint32 i=0; i<nNumHeadersCAF; i++)
			{
				GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[i];
				if (rGAH.m_FilePathDBACRC32!=nDBACRC32)
					continue;

				rGAH.m_nControllers=0;

				if (rGHDBA.m_nTCount < 0x00e0)
					continue;
				for (uint32 c=0; c<rGAH.m_nControllers2; c++)
					rGAH.m_arrController[c]=0;
			}
		}

		//check if the time  has come to remove DBA
		for (uint32 d=0; d<numDBA_Files; d++)
		{
			CGlobalHeaderDBA& rGHDBA = *g_AnimationManager.m_arrGlobalHeaderDBA[d];
			if (rGHDBA.m_pDatabaseInfo==0)
				continue;
			if (rGHDBA.m_nTCount < 0x0100)
				continue;
			if (rGHDBA.m_nUsedAnimations)
				continue;
#ifdef _DEBUG
			const char* pName  = rGHDBA.m_strFilePathDBA;
#endif
			rGHDBA.DeleteDatabaseDBA();
		}


	}
}


//////////////////////////////////////////////////////////////////////////
class CAnimationStatsResourceList
{
public:
	string UnifyFilename( const char *sResourceFile )
	{
		string filename = sResourceFile;
		filename.replace( '\\','/' );
		filename.MakeLower();
		return filename;
	}

	void Add( const char *sResourceFile )
	{
		string filename = UnifyFilename(sResourceFile);
		m_set.insert(filename);
	}
	void Remove( const char *sResourceFile )
	{
		string filename = UnifyFilename(sResourceFile);
		m_set.erase(filename);
	}
	void Clear()
	{
		m_set.clear();
	}
	bool IsExist( const char *sResourceFile )
	{
		string filename = UnifyFilename(sResourceFile);
		if (m_set.find(filename) != m_set.end())
			return true;
		return false;
	}
	void Load( const char *sResourceListFilename )
	{
		m_filename = sResourceListFilename;
		FILE *file = fxopen( sResourceListFilename,"rb" );
		if (file)
		{
			PREFAST_SUPPRESS_WARNING(6031) fseek(file,0,SEEK_END);
			int nFileLentgh = ftell(file);
			PREFAST_SUPPRESS_WARNING(6031) fseek(file,0,SEEK_SET);
			char *buf = new char[nFileLentgh+16];
			fread( buf,nFileLentgh,1,file );
			buf[nFileLentgh] = 0;

			// Parse file, every line in a file represents a resource filename.
			char seps[] = "\r\n";
			char *token = strtok( buf, seps );
			while (token != NULL)
			{
				Add(token);
				token = strtok( NULL, seps );
			}
			delete []buf;
		}
	}
	void Save()
	{
		FILE *file = fxopen( m_filename,"wt" );
		for (ResourceSet::const_iterator it = m_set.begin(); it != m_set.end(); ++it)
		{
			fprintf( file,"%s\n",(*it).c_str() );
		}
		fclose(file);
	}

private:
	typedef std::set<string> ResourceSet;
	ResourceSet m_set;
	ResourceSet::iterator m_iter;
	string m_filename;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CharacterManager::DumpAssetStatistics()
{
	CAnimationStatsResourceList caf_usedList;
	CAnimationStatsResourceList caf_unusedList;
	CAnimationStatsResourceList lmg_usedList;
	CAnimationStatsResourceList lmg_unusedList;

	caf_usedList.Load( "AnimStats_Used_CAF.txt" );
	caf_unusedList.Load( "AnimStats_NotUsed_CAF.txt" );

	lmg_usedList.Load( "AnimStats_Used_LMG.txt" );
	lmg_unusedList.Load( "AnimStats_NotUsed_LMG.txt" );

	uint32 nNumAnimHeaders = g_AnimationManager.m_arrGlobalCAF.size();
	for (uint32 i=0; i<nNumAnimHeaders; i++)
	{
		GlobalAnimationHeaderCAF *pAnim = &(g_AnimationManager.m_arrGlobalCAF[i]);

		const char* strPathName = pAnim->GetFilePath();
		bool IsAssetLMG    = (pAnim->IsAssetLMG())!=0;
		bool IsAssetOnDemand = (pAnim->IsAssetOnDemand())!=0;

		if (caf_usedList.IsExist( strPathName ))
			pAnim->m_nTouchedCounter++;

		if (IsAssetLMG && pAnim->m_nTouchedCounter != 0)
		{
			lmg_usedList.Add( strPathName );
			lmg_unusedList.Remove( strPathName );
		}
		else if ((IsAssetLMG==0) && (IsAssetOnDemand==0) && pAnim->m_nTouchedCounter != 0)
		{
			caf_usedList.Add( strPathName );
			caf_unusedList.Remove( strPathName );
		}
		else if (IsAssetLMG && pAnim->m_nTouchedCounter == 0)
		{
			if (!lmg_usedList.IsExist( strPathName ))
				lmg_unusedList.Add( strPathName );
		}
		else if ((IsAssetLMG==0) && (IsAssetOnDemand==0) && pAnim->m_nTouchedCounter == 0)
		{
			if (!caf_usedList.IsExist( strPathName ))
				caf_unusedList.Add( strPathName );
		}
	}

	caf_usedList.Save();
	caf_unusedList.Save();
	lmg_usedList.Save();
	lmg_unusedList.Save();
}

// can be called instead of Update() for UI purposes (such as in preview viewports, etc).
void CharacterManager::DummyUpdate()
{
	m_nUpdateCounter++;
}

//! Locks all models in memory
void CharacterManager::LockResources()
{
	m_arrTempLock.clear();
	m_arrTempLock.resize (m_arrModelCache.size());
	for (size_t i = 0, end = m_arrModelCache.size(); i < end; ++i)
		m_arrTempLock[i] = m_arrModelCache[i];
}

//! Unlocks all models in memory
void CharacterManager::UnlockResources()
{
	m_arrTempLock.clear();
}


ICharacterInstance* CharacterManager::LoadCharacterDefinition( const string pathname, uint32 IsSkinAtt, CCharInstance* pSkelInstance  )
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);

	ICharacterInstance* pCharacter=0;

	//	__sgi_alloc::get_wasted_in_blocks();
	if (Console::GetInst().ca_MemoryUsageLog)
	{ 
		CryModuleMemoryInfo info;
		CryGetMemoryInfoForModule(&info);
		g_pILog->UpdateLoadingScreen("CDF %s. Start. Memstat %i", pathname.c_str(), (int)(info.allocated - info.freed));
	}


	uint32 ind = ~0;
	uint32 end = m_cdfFiles.size();
	for (uint32 i=0; i<end; i++)
	{
		if (m_cdfFiles[i].FilePath.compareNoCase(pathname) == 0)
		{
			ind = i;
			break;
		}
	}

	if (ind == ~0)
	{
		ind = LoadCDF(pathname);
		if (ind == ~0)
		{
			g_pILog->LogError ("CryAnimation: character-definition not found: %s", pathname.c_str());
			return 0;
		}

	}

	m_cdfFiles[ind].AddRef();
	uint32 numRefs = m_cdfFiles[ind].m_nRefCounter;
	CharacterDefinition def = m_cdfFiles[ind];

	if (!def.Model.empty()) 
	{
		pCharacter = CreateInstance( def.Model.c_str() );
		if (pCharacter==0)
		{
			assert("!filepath for character not found");
			g_pILog->LogError ("filepath for character not found: %s", def.Model.c_str());
			return 0;
		}

		if (IsSkinAtt==0xDeadBeef)
			((CCharInstance*)(pCharacter))->m_AttachmentManager.m_pSkelInstance=pSkelInstance;

		if (!def.Material.empty())
		{
			_smart_ptr<IMaterial> pMaterial = g_pISystem->GetI3DEngine()->GetMaterialManager()->LoadMaterial(def.Material, false);
			pCharacter->SetMaterial(pMaterial);
		}

		//store the CGF-pathname inside the instance
		((CSkinInstance*)pCharacter)->SetFilePath(pathname);

		//-----------------------------------------------------------
		//load attachment-list 
		IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
		uint32 num = def.arrAttachments.size();
		for (uint32 i=0; i<num; i++) 
		{
			CharacterAttachment& attach = def.arrAttachments[i];
			IAttachment* pIAttachment=0;

			if (attach.Type==CA_BONE) 
			{
				pIAttachment = pIAttachmentManager->CreateAttachment(attach.Name,CA_BONE,attach.BoneName);
				if (pIAttachment==0) continue;

				pIAttachment->SetAttAbsoluteDefault(QuatT(attach.WRotation,attach.WPosition));
				//	pIAttachment->SetRMWRotation(attach.WRotation);

				const char* fileExt = PathUtil::GetExt( attach.BindingPath);

				bool IsCDF = (0 == stricmp(fileExt,"cdf"));
				bool IsCHR = (0 == stricmp(fileExt,"chr"));
				bool IsCGA = (0 == stricmp(fileExt,"cga"));
				bool IsCGF = (0 == stricmp(fileExt,"cgf"));
				if (IsCDF) 
				{
					ICharacterInstance* pIChildCharacter = LoadCharacterDefinition( attach.BindingPath );
					if (pIChildCharacter) 
					{
						CCHRAttachment* pCharacterAttachment = new CCHRAttachment();
						pCharacterAttachment->m_pCharInstance = pIChildCharacter;
						IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
						pIAttachment->AddBinding(pIAttachmentObject);
					}
				}
				if (IsCHR||IsCGA) 
				{
					ICharacterInstance* pIChildCharacter = CreateInstance( attach.BindingPath );
					if (pIChildCharacter==0)
					{
						g_pILog->LogError ("CryAnimation: no character created: %s", pathname.c_str());
					} 
					else
					{
						CCHRAttachment* pCharacterAttachment = new CCHRAttachment();
						pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
						IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
						pIAttachment->AddBinding(pIAttachmentObject);
					}
				}
				if (IsCGF) 
				{
					IStatObj* pIStatObj = g_pISystem->GetI3DEngine()->LoadStatObj( attach.BindingPath );
					if (pIStatObj)
					{
						CCGFAttachment* pStatAttachment = new CCGFAttachment();
						pStatAttachment->pObj  = pIStatObj;
						IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pStatAttachment;
						pIAttachment->AddBinding(pIAttachmentObject);
					}
				}

				if (*(int*)attach.PhysInfo[0].spring_angle==0x12345678)
				{
					CModelJoint &joint = ((CCharacterModel*)pCharacter->GetICharacterModel())->m_ModelSkeleton.m_arrModelJoints[pIAttachment->GetBoneID()];
					int idxSurf = *(int*)(joint.m_PhysInfo[0].spring_angle+1); // don't override surface idx
					joint.m_PhysInfo[0] = attach.PhysInfo[0]; 
					*(int*)(joint.m_PhysInfo[0].spring_angle+1) = idxSurf;
					idxSurf = *(int*)(joint.m_PhysInfo[1].spring_angle+1);
					joint.m_PhysInfo[1] = attach.PhysInfo[1]; 
					*(int*)(joint.m_PhysInfo[1].spring_angle+1) = idxSurf;
				}
			} 
			else
				if (attach.Type == CA_FACE) 
				{
					pIAttachment = pIAttachmentManager->CreateAttachment(attach.Name,CA_FACE);
					if (pIAttachment==0) continue;

					pIAttachment->SetAttAbsoluteDefault( QuatT(attach.WRotation,attach.WPosition) );

					const char* fileExt = PathUtil::GetExt( attach.BindingPath );

					bool IsCDF = (0 == stricmp(fileExt,"cdf"));
					bool IsCGF = (0 == stricmp(fileExt,"cgf"));
					bool IsCHR = (0 == stricmp(fileExt,"chr"));
					if (IsCDF) 
					{
						ICharacterInstance* pIChildCharacter = LoadCharacterDefinition( attach.BindingPath );
						if (pIChildCharacter) 
						{
							CCHRAttachment* pCharacterAttachment = new CCHRAttachment();
							pCharacterAttachment->m_pCharInstance = pIChildCharacter;
							IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
							pIAttachment->AddBinding(pIAttachmentObject);
						}
					}
					if (IsCHR) 
					{
						ICharacterInstance* pIChildCharacter = CreateInstance( attach.BindingPath );
						if (pIChildCharacter==0)
						{
							g_pILog->LogError ("CryAnimation: no character created: %s", pathname.c_str());
						} 
						else
						{
							CCHRAttachment* pCharacterAttachment = new CCHRAttachment();
							pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
							IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
							pIAttachment->AddBinding(pIAttachmentObject);
						}
					}
					if (IsCGF) 
					{
						IStatObj* pIStatObj = g_pISystem->GetI3DEngine()->LoadStatObj( attach.BindingPath );
						if (pIStatObj)
						{
							CCGFAttachment* pStatAttachment = new CCGFAttachment();
							pStatAttachment->pObj  = pIStatObj;
							IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pStatAttachment;
							pIAttachment->AddBinding(pIAttachmentObject);
						}
					}
				}
				else
					if (attach.Type == CA_SKIN) 
					{
						pIAttachment = pIAttachmentManager->CreateAttachment(attach.Name,CA_SKIN);

						if (!pIAttachment)
							continue;

						pIAttachment->SetAttAbsoluteDefault( QuatT(attach.WRotation,attach.WPosition) );

						const char* fileExt = PathUtil::GetExt(attach.BindingPath);

						bool IsCDF = (0 == stricmp(fileExt,"cdf"));
						bool IsCHR = (0 == stricmp(fileExt,"chr"));
						if (IsCDF) 
						{
							ICharacterInstance* pIChildCharacter = LoadCharacterDefinition( attach.BindingPath, 0xDeadBeef, (CCharInstance*)pCharacter );
							if (pIChildCharacter) 
							{
								CCharInstance* pSkinAttachment = (CCharInstance*)pIChildCharacter;
								//						CCharInstance* pSkeleton =  pSkinAttachment->m_AttachmentManager.m_pSkelInstance;
								//						CSkinInstance* pSkin3 = (CCharInstance*)pCharacter;

								pSkinAttachment->m_AttachmentManager.m_pSkelInstance=(CCharInstance*)pCharacter;

								CCHRAttachment* pCharacterAttachment = new CCHRAttachment();
								pCharacterAttachment->m_pCharInstance = pIChildCharacter;
								IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
								pIAttachment->AddBinding(pIAttachmentObject);
							}
						}
						if (IsCHR)
						{
							ICharacterInstance* pIChildCharacter = CreateInstance( attach.BindingPath, 0xDeadBeef, pIAttachment );
							if (pIChildCharacter==0)
							{
								g_pILog->LogError ("CryAnimation: no character created: %s", pathname.c_str());
							} 
							else 
							{
								if (pIChildCharacter->GetFacialInstance())
									pIChildCharacter->GetFacialInstance()->SetMasterCharacter(pCharacter);
								CCHRAttachment* pCharacterAttachment = new CCHRAttachment();
								pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
								IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
								pIAttachment->AddBinding(pIAttachmentObject);
							}
						}

					}

					if (!attach.Material.empty() && pIAttachment != 0)
					{
						IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
						if (pIAttachmentObject != 0)
						{
							_smart_ptr<IMaterial> pMaterial = g_pISystem->GetI3DEngine()->GetMaterialManager()->LoadMaterial(attach.Material, false);
							pIAttachmentObject->SetMaterial(pMaterial);
						}
					}

					//----------------------------------------------------------------

					CRY_ASSERT(pIAttachment);
					pIAttachment->SetFlags(attach.AttFlags);
					pIAttachment->HideAttachment(attach.AttFlags&nHideAttachment);

					pIAttachment->SetHingeParams(attach.Idx,attach.Limit,attach.Damping);

		}

		uint32 count = pIAttachmentManager->GetAttachmentCount();
		//assert(count==num); //some attachments are invalid or attachment names are duplicated or whatever 
		pIAttachmentManager->ProjectAllAttachment();

		//-----------------------------------------------------------
		//load Shape-Deformation data
		//-----------------------------------------------------------
		f32* pMorphValues = pCharacter->GetShapeDeformArray();
		for (uint32 kk = 0; kk < 8; ++kk)
		{	
			pMorphValues[kk] = def.Morphing[kk];
		}
	}


	if (Console::GetInst().ca_MemoryUsageLog)
	{
		CryModuleMemoryInfo info;
		CryGetMemoryInfoForModule(&info);
		g_pILog->UpdateLoadingScreen("CDF. Finish. Memstat %i", (int)(info.allocated - info.freed));
	}

	return pCharacter;
}




int32 CharacterManager::LoadCDF(const char* pathname ) 
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_CDF, EMemStatContextFlags::MSF_Instance, "%s", pathname);

	XmlNodeRef root		= g_pISystem->LoadXmlFile(pathname);
	if (root==0)
	{
		g_pILog->LogError ("CryAnimation: character-definition creation failed: %s", pathname);
		return -1;
	}

	CharacterDefinition def;
	def.FilePath = pathname;
	//-----------------------------------------------------------
	//load model 
	//-----------------------------------------------------------
	XmlNodeRef nodeModel = root->getChild(0);
	if (nodeModel==0)
	{
		g_pILog->LogError ("CryAnimation: XmlNodeRef is zero: %s", pathname);
		return -1;
	}

	const char* modeltag = nodeModel->getTag();
	if (strcmp(modeltag,"Model")==0) 
	{
		def.Model =nodeModel->getAttr( "File" ); 

		if (nodeModel->haveAttr("Material"))
			def.Material = nodeModel->getAttr("Material");
		//-----------------------------------------------------------
		//load attachment-list 
		//-----------------------------------------------------------
		uint32 ChildNo=1;
		XmlNodeRef nodeAttachList = root->getChild(ChildNo);
		const char* AttachListTag = nodeAttachList->getTag();
		if (strcmp(AttachListTag,"AttachmentList")==0) 
		{
			ChildNo++;

			uint32 num = nodeAttachList->getChildCount();
			def.arrAttachments.reserve(num);
			for (uint32 i=0; i<num; i++) 
			{
				CharacterAttachment attach;

				XmlNodeRef nodeAttach = nodeAttachList->getChild(i);
				const char* AttachTag = nodeAttach->getTag();
				if (strcmp(AttachTag,"Attachment")==0) 
				{
					Quat WRotation(IDENTITY);
					Vec3 WPosition(ZERO);
					string AName = nodeAttach->getAttr( "AName" );
					CryStringUtils::UnifyFilePath(AName);
					attach.Name = AName;

					stack_string Type = nodeAttach->getAttr( "Type" );
					nodeAttach->getAttr( "Rotation",attach.WRotation );
					nodeAttach->getAttr( "Position",attach.WPosition );


					attach.BoneName = nodeAttach->getAttr( "BoneName" );
					attach.BindingPath = nodeAttach->getAttr( "Binding" );

					if (nodeAttach->haveAttr("Material"))
						attach.Material = nodeAttach->getAttr("Material");


					if (Type=="CA_BONE") 
					{
						attach.Type = CA_BONE;
					} 
					else
						if (Type=="CA_FACE") 
						{
							attach.Type = CA_FACE;
						}
						else
							if (Type=="CA_SKIN") 
							{
								attach.Type = CA_SKIN;
							}

							uint32 flags;
							if (nodeAttach->getAttr("Flags",flags))
								attach.AttFlags = flags;

							int idx=-1;
							nodeAttach->getAttr("HingeIdx",attach.Idx);
							nodeAttach->getAttr("HingeLimit",attach.Limit);
							nodeAttach->getAttr("HingeDamping",attach.Damping);

							if (nodeAttach->haveAttr("PhysPropType"))
							{
								string propType = nodeAttach->getAttr("PhysPropType");
								char buf[32]; bool lodUsed;
								int type = !strcmp(propType,"Rope") ? 0 : (!strcmp(propType,"Cloth") ? 1:-1);
								memset(attach.PhysInfo, 0, sizeof(attach.PhysInfo));
								DynArray<SJointProperty> jp = CCharacterModel::GetPhysInfoProperties(attach.PhysInfo[0],type);
								for(int lod=0;lod<2;lod++) {
									for(idx=1,lodUsed=false; idx<jp.size(); idx++) {
										sprintf(buf, "lod%d_%s", lod,jp[idx].name);
										if (jp[idx].type==0)
											lodUsed |= nodeAttach->getAttr(buf, jp[idx].fval);
										else
											lodUsed |= nodeAttach->getAttr(buf, jp[idx].bval);
									}
									if (lodUsed)
										CCharacterModel::ParsePhysInfoProperties(attach.PhysInfo[lod], jp);
								}
							}
							def.arrAttachments.push_back(attach);
				}
			}
		}

		//-----------------------------------------------------------
		//load Shape-Deformation data
		//-----------------------------------------------------------
		XmlNodeRef ShapeDeformation = root->getChild(ChildNo);
		const char* ShapeDefTag = ShapeDeformation->getTag();
		if (strcmp(ShapeDefTag,"ShapeDeformation")==0) 
		{
			ShapeDeformation->getAttr( "COL0",	def.Morphing[0] );
			ShapeDeformation->getAttr( "COL1",	def.Morphing[1] );
			ShapeDeformation->getAttr( "COL2",	def.Morphing[2] );
			ShapeDeformation->getAttr( "COL3",	def.Morphing[3] );
			ShapeDeformation->getAttr( "COL4",	def.Morphing[4] );
			ShapeDeformation->getAttr( "COL5",	def.Morphing[5] );
			ShapeDeformation->getAttr( "COL6",	def.Morphing[6] );
			ShapeDeformation->getAttr( "COL7",	def.Morphing[7] );
		}
	}

	m_cdfFiles.push_back(def);
	return m_cdfFiles.size() - 1;
}


void CharacterManager::ReleaseCDF(const char* pathname)
{
	uint32 ind = ~0;
	uint32 end = m_cdfFiles.size();
	for (uint32 i=0; i<end; i++)
	{
		if (m_cdfFiles[i].FilePath.compareNoCase(pathname) == 0)
		{
			ind = i;
			break;
		}
	}

	if (ind != ~0)
	{
		//found CDF-name
		assert(m_cdfFiles[ind].m_nRefCounter);
		m_cdfFiles[ind].m_nRefCounter--;
		if (m_cdfFiles[ind].m_nRefCounter==0)
		{
			if (ind)
			{
				for (uint32 i=ind+1; i<end; i++)
					m_cdfFiles[i-1]=m_cdfFiles[i];
			}
			m_cdfFiles.pop_back();
		}
	}

}




uint32 CharacterManager::SaveCharacterDefinition(ICharacterInstance* pCharacter, const char* pathname ) {

	XmlNodeRef root		= g_pISystem->CreateXmlNode( "CharacterDefinition" );	
	if (root==0)
	{
		g_pILog->LogError ("CryAnimation: character-definition creation failed: %s", pCharacter->GetFilePath());
		return 0;
	}

	//-----------------------------------------------------------
	//save model-filepath 
	//-----------------------------------------------------------
	XmlNodeRef nodeModel = root->newChild( "Model" );
	const char* fn = pCharacter->GetICharacterModel()->GetModelFilePath();
	nodeModel->setAttr("File",fn);
	if (pCharacter->GetMaterialOverride())
		nodeModel->setAttr("Material", pCharacter->GetMaterialOverride()->GetName());


	//-----------------------------------------------------------
	//save attachment-list 
	//-----------------------------------------------------------
	IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
	uint32 count = pIAttachmentManager->GetAttachmentCount();

	XmlNodeRef nodeAttachements; 
	if (count) 
		nodeAttachements=root->newChild( "AttachmentList" );

	for (uint32 i=0; i<count; i++) 
	{
		IAttachment*  pIAttachment = pIAttachmentManager->GetInterfaceByIndex(i);

		XmlNodeRef nodeAttach = nodeAttachements->newChild( "Attachment" );
		const char* AName = pIAttachment->GetName();
		Quat WRotation = pIAttachment->GetAttAbsoluteDefault().q;
		Vec3 WPosition = pIAttachment->GetAttAbsoluteDefault().t;
		uint32 Type = pIAttachment->GetType();

		uint32 BoneID = pIAttachment->GetBoneID();
		const char* BoneName	= ((CCharInstance*)pCharacter)->m_SkeletonPose.GetJointNameByID(BoneID);	

		const char* BindingFilePath = "";

		if (Type==CA_BONE || Type==CA_FACE) 
		{

			IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
			if (pIAttachmentObject)
			{
				ICharacterInstance* pICharInstance = pIAttachmentObject->GetICharacterInstance();
				if (pICharInstance)
					BindingFilePath = pICharInstance->GetFilePath();

				IStatObj* pStaticObject = pIAttachmentObject->GetIStatObj();
				if (pStaticObject)	
					BindingFilePath = pStaticObject->GetFilePath();
			}
		}

		if (Type==CA_SKIN || Type==CA_PART) 
		{
			IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
			if (pIAttachmentObject)
			{
				ICharacterInstance* pICharacterChild=pIAttachmentObject->GetICharacterInstance();
				if (pICharacterChild) 
					BindingFilePath = pICharacterChild->GetFilePath();
			}
		}

		IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
		const char* szMaterialName = 0;
		if (pIAttachmentObject)
		{
			IMaterial* pMaterial = pIAttachmentObject->GetMaterialOverride();
			if (pMaterial)
				szMaterialName = pMaterial->GetName();
		}

		nodeAttach->setAttr( "AName",AName );
		if (Type==CA_BONE) nodeAttach->setAttr( "Type", "CA_BONE" );
		if (Type==CA_FACE) nodeAttach->setAttr( "Type", "CA_FACE" );
		if (Type==CA_SKIN) nodeAttach->setAttr( "Type", "CA_SKIN" );
		nodeAttach->setAttr( "Rotation",WRotation );
		nodeAttach->setAttr( "Position",WPosition );
		nodeAttach->setAttr( "BoneName",BoneName );
		nodeAttach->setAttr( "Binding", BindingFilePath );

		uint32 nFlags = pIAttachment->GetFlags();
		if (pIAttachment->IsAttachmentHidden())	nFlags|=nHideAttachment;
		nodeAttach->setAttr( "Flags", nFlags );
		if (szMaterialName)
			nodeAttach->setAttr( "Material", szMaterialName );

		int idx; float limit,damping;
		pIAttachment->GetHingeParams(idx,limit,damping);
		if (idx>=0) {
			nodeAttach->setAttr( "HingeIdx",idx );
			nodeAttach->setAttr( "HingeLimit",limit );
			nodeAttach->setAttr( "HingeDamping",damping );
		}

		char buf[32];
		int ibone = pIAttachment->GetBoneID();
		for(int lod=0; lod<2; lod++)
			if (*(int*)((CCharacterModel*)pCharacter->GetICharacterModel())->m_ModelSkeleton.m_arrModelJoints[ibone].m_PhysInfo[lod].spring_angle==0x12345678)
			{
				DynArray<SJointProperty> jp = pCharacter->GetICharacterModel()->GetJointPhysProperties(ibone,lod);
				if (jp.size()>0 && jp[0].type==2 && lod==0)
					nodeAttach->setAttr("PhysPropType", jp[0].strval);
				for(idx=1; idx<jp.size(); idx++) {
					sprintf(buf, "lod%d_%s", lod,jp[idx].name);
					if (jp[idx].type==0)
						nodeAttach->setAttr(buf, jp[idx].fval);
					else
						nodeAttach->setAttr(buf, jp[idx].bval);
				}
			}
	}

	//-----------------------------------------------------------
	//save Shape-Deformation data
	//-----------------------------------------------------------
	XmlNodeRef ShapeDeformation =	root->newChild( "ShapeDeformation" );
	f32* pMorphValues = pCharacter->GetShapeDeformArray();
	ShapeDeformation->setAttr( "COL0", pMorphValues[0] );
	ShapeDeformation->setAttr( "COL1", pMorphValues[1] );
	ShapeDeformation->setAttr( "COL2", pMorphValues[2] );
	ShapeDeformation->setAttr( "COL3", pMorphValues[3] );
	ShapeDeformation->setAttr( "COL4", pMorphValues[4] );
	ShapeDeformation->setAttr( "COL5", pMorphValues[5] );
	ShapeDeformation->setAttr( "COL6", pMorphValues[6] );
	ShapeDeformation->setAttr( "COL7", pMorphValues[7] );


	root->saveToFile(pathname);

	return 1;
}


//-----------------------------------------------------------------------------------------------------
// Create foot plant lookup info from the FOOTPLANT_TABLE section of the AnimPrototyping.ap xml file.
//-----------------------------------------------------------------------------------------------------
void CharacterManager::CreateFootplantTableFromXML(XmlNodeRef & rXml)
{
	if (rXml->isTag("FOOTPLANT_TABLE"))
	{
		uint32 num = rXml->getChildCount();
		m_arrFootPlantTable.resize(num);

		for (uint32 i=0; i<num; i++) 
		{
			XmlNodeRef nodeFootplant = rXml->getChild(i);
			if (0==nodeFootplant)
				break;

			const char* tagFootplant = nodeFootplant->getTag();
			if (0==strcmp(tagFootplant,"FOOTPLANT")) 
			{
				m_arrFootPlantTable[i].m_A=0;
				m_arrFootPlantTable[i].m_B=0;
				m_arrFootPlantTable[i].m_C=0;
				m_arrFootPlantTable[i].m_D=0;
				m_arrFootPlantTable[i].m_E=0;
				m_arrFootPlantTable[i].m_F=0;
				m_arrFootPlantTable[i].m_sAssetName=nodeFootplant->getAttr( "ASSET" );
				nodeFootplant->getAttr( "SEG_NO",	m_arrFootPlantTable[i].m_segNo );
				nodeFootplant->getAttr( "A",	m_arrFootPlantTable[i].m_A );
				nodeFootplant->getAttr( "B",	m_arrFootPlantTable[i].m_B );
				nodeFootplant->getAttr( "C", m_arrFootPlantTable[i].m_C );
				nodeFootplant->getAttr( "D", m_arrFootPlantTable[i].m_D );
				nodeFootplant->getAttr( "E", m_arrFootPlantTable[i].m_E );
				nodeFootplant->getAttr( "F", m_arrFootPlantTable[i].m_F );
			}
		}
	}
}


//-----------------------------------------------------------------------------------------------------
// Load the Anim prototyping file.
//-----------------------------------------------------------------------------------------------------
// All asset name related hacks must be driven from this file.
// Project specific animation system hacks also need to be controlled from here.
#if 1
void CharacterManager::LoadAnimPrototypeFile(const char* psFilePath)
{
	XmlNodeRef root = g_pISystem->LoadXmlFile(psFilePath);
	if(root==0)
	{
		//CryFatalError ("CryAnimation: Error loading Animation Prototype file '%s'.",psFilePath);
		CryLogAlways("CryAnimation: Error loading Animation Prototype file '%s'.",psFilePath);
		return;
	}

	if(!root->isTag("ANIMPROTO"))
		return;

	uint32 nNodes = root->getChildCount();

	for(uint32 i=0; i<nNodes; ++i) 
	{
		XmlNodeRef nodeTable = root->getChild(i);
		if (0==nodeTable)
			break;

		const char* tagNodeTable = nodeTable->getTag();
		if(0==strcmp(tagNodeTable,"FOOTPLANT_TABLE"))
		{
			CreateFootplantTableFromXML(nodeTable);
		}

	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// CharacterManager
///////////////////////////////////////////////////////////////////////////////////////////////////////

CharacterManager::CharacterManager ()
{
	m_IsDedicatedServer=0;
	m_arrFrameTimes.resize(200);
	for (uint32 i=0; i<200; i++)
		m_arrFrameTimes[i]=0.014f;

	// default to no scaling of animation speeds, unless game requests it
	m_scalingLimits = Vec2(1.0f, 1.0f);
	m_pFacialAnimation = new CFacialAnimation();
	m_nUpdateCounter = 0;
	m_AIF_Initilization=0;
	m_bImmidiateInstanceDeleteMode = false;

	m_arrModelCache.reserve(100);
}

CharacterManager::~CharacterManager()
{
	m_setLockedModels.clear();
	uint32 numModels = m_arrModelCache.size();
	if (numModels)
	{
		g_pILog->LogToFile("*ERROR* CharacterManager: %u body instances still not deleted. Forcing deletion (though some instances may still refer to them)", m_arrModelCache.size());
		CleanupModelCache(true);
	}

	delete m_pFacialAnimation;
	g_DeleteInterfaces();
}







//////////////////////////////////////////////////////////////////////////
void CharacterManager::SetFootPlantsFlag(CCharInstance* pCryCharInstance) 
{
	if (pCryCharInstance->m_pModel->m_ModelSkeleton.IsHuman())
		pCryCharInstance->m_SkeletonPose.SetFootPlants(1);
}


//////////////////////////////////////////////////////////////////////////
void CharacterManager::GetLoadedModels( ICharacterModel** pCharacterModels,int &nCount )
{
	if (!pCharacterModels)
	{
		nCount = (int)m_arrModelCache.size();
		return;
	}
	int num = min( (int)m_arrModelCache.size(),nCount );
	for (int i = 0; i < nCount; i++)
	{
		pCharacterModels[i] = m_arrModelCache[i];
	}
}

//////////////////////////////////////////////////////////////////////////
void CharacterManager::ReloadModel(ICharacterModel* pModel)
{
	if (pModel == NULL)
		return;

	CCharacterModel& model = *(CCharacterModel*)pModel;

	string fileExt = PathUtil::GetExt(model.GetNameCStr());
	if (stricmp(fileExt,"chr")) // not CHR?
		return;

	CCharacterModel* pNewModel = FetchModel(model.GetFilePath(), 0, 1);

	if (pNewModel == NULL)
		return;

	model.AddRef();

	// Reregister old model instances to new one
	std::vector<ICharacterInstance*> aInstances;
	int numInstances = model.GetNumInstances();
	aInstances.reserve(numInstances);

	for (int nInstance = 0; nInstance < numInstances; ++nInstance)
		aInstances.push_back(model.GetInstance(nInstance));

	for (int nInstance = 0; nInstance < numInstances; ++nInstance)
		aInstances[nInstance]->ReplaceModel(pNewModel);

	UnregisterModel(&model);
	model.Release();
}

//////////////////////////////////////////////////////////////////////////
void CharacterManager::ReloadAllModels()
{
	for (int i = 0; i < (int)m_arrModelCache.size(); ++i)
		ReloadModel(m_arrModelCache[i]);
}

void CharacterManager::SyncAllAnimations()
{	
	for( size_t i=0 ; i<m_AnimationSyncQueue.size(); ++i)
	{
		m_AnimationSyncQueue[i]->FinishAnimationComputations();
	}

	m_AnimationSyncQueue.clear();
}

void CharacterManager::AddAnimationToSyncQueue( ICharacterInstance *pCharInstance )
{		
	m_AnimationSyncQueue.push_back( pCharInstance );
}

void CharacterManager::RemoveAnimationToSyncQueue( ICharacterInstance *pCharInstance )
{	
	for( std::vector<ICharacterInstance*>::iterator it = m_AnimationSyncQueue.begin() ; it != m_AnimationSyncQueue.end() ; )
	{
		if( *it == pCharInstance )
		{
			it = m_AnimationSyncQueue.erase( it );
		}
		else
		{
			++it;
		}
	}

}


//////////////////////////////////////////////////////////////////////////
void CharacterManager::AddToEraseList( CSkinInstance* pInstance )
{
	if (m_bImmidiateInstanceDeleteMode)
	{
		pInstance->DeleteThis();
	}
	else
	{
		m_InstancesEraseList.push_back( std::make_pair(gEnv->nMainFrameID,pInstance) );
	}
}

//////////////////////////////////////////////////////////////////////////
void CharacterManager::CleanEraseList( bool bForceCleanAll )
{
	bool bPrevDeleteMode = m_bImmidiateInstanceDeleteMode;
	m_bImmidiateInstanceDeleteMode = true;
	for (int i = 0,num = (int)m_InstancesEraseList.size(); i < num; i++)
	{
		int nEraseFrame = m_InstancesEraseList[i].first;
		CSkinInstance *pInstance = m_InstancesEraseList[i].second;
		
		if (bForceCleanAll || nEraseFrame < gEnv->nMainFrameID+2)
		{
			// Normally delete instance in 2 frames after it was released.
			pInstance->DeleteThis();
		}
	}
	m_InstancesEraseList.clear();
	m_bImmidiateInstanceDeleteMode = bPrevDeleteMode;
}

//////////////////////////////////////////////////////////////////////////
void CharacterManager::LoadAnimationImageFile( const char* filenameCAF,const char* filenameAIM )
{
	//	return;
	if (m_AIF_Initilization)
		return;
	m_AIF_Initilization=1;

	g_pI3DEngine			=	g_pISystem->GetI3DEngine();

	
	IChunkFile* chunkFileCGF = g_pI3DEngine->CreateChunkFile(true);
	uint32 HasIMG_CAF = LoadAnimationImageFileCAF( filenameCAF, chunkFileCGF );
	chunkFileCGF->Release();

	IChunkFile* chunkFileAIM = g_pI3DEngine->CreateChunkFile(true);
	uint32 HasIMG_AIM = LoadAnimationImageFileAIM( filenameAIM, chunkFileAIM );
	chunkFileAIM->Release();
}

//////////////////////////////////////////////////////////////////////////
bool CharacterManager::LoadAnimationImageFileCAF( const char* filenameCAF, IChunkFile* pChunkFile )
{
	return 0;

	stack_string strPath = filenameCAF;
	CryStringUtils::UnifyFilePath(strPath);
	if (!pChunkFile->Read( strPath ))
	{
		pChunkFile->GetLastError();
		return 0;
	}

	// Load mesh from chunk file.
	if ((pChunkFile->GetFileHeader().Version != ChunkFileVersion) && (pChunkFile->GetFileHeader().Version != ChunkFileVersion_Align))
	{
		//m_LastError.Format("Bad CAF file version: %s", m_filename.c_str() );
		return 0;
	}

	if (pChunkFile->GetFileHeader().FileType != FileType_Geom) 
	{
		if (pChunkFile->GetFileHeader().FileType != FileType_Anim) 
		{
			//m_LastError.Format("Illegal File Type for .caf file: %s", m_filename.c_str() );
			return 0;
		}
	}
	
	//-------------------------------------------------------------------------------------------

	uint32 numChunck = pChunkFile->NumChunks();
	g_AnimationManager.m_arrGlobalCAF.resize(numChunck);
	for (uint32 i=0; i<numChunck; i++)
	{
		const CHUNK_HEADER &hdr = pChunkFile->GetChunkHeader(i);
		if (hdr.ChunkType!=ChunkType_GlobalAnimationHeaderCAF)
		{
			g_AnimationManager.m_arrGlobalCAF.clear();
			return 0;
		}
		IChunkFile::ChunkDesc* pChunkDesc = pChunkFile->GetChunk(i);
		if (pChunkDesc->hdr.ChunkVersion != CHUNK_GAHCAF_INFO::VERSION)
		{
			g_AnimationManager.m_arrGlobalCAF.clear();
			return 0;
		}

		CHUNK_GAHCAF_INFO* pChunk = (CHUNK_GAHCAF_INFO*)pChunkDesc->data;
			//	SwapEndian(*pChunk);
		GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[i];

		rCAF.m_nFlags						= pChunk->m_Flags;
		rCAF.m_FilePath					=	pChunk->m_FilePath;
		rCAF.m_FilePathCRC32		=	pChunk->m_FilePathCRC32;
		rCAF.m_FilePathDBACRC32	=	pChunk->m_FilePathDBACRC32;
		uint32 nCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase( rCAF.m_FilePath ); 
		if (rCAF.m_FilePathCRC32 != nCRC32)	
			CryFatalError("CryAnimation: CRC32 Invalid! Most likely the endian conversion from RC is wrong");

		rCAF.m_FootPlantVectors.m_LHeelStart	= pChunk->m_LHeelStart;
		rCAF.m_FootPlantVectors.m_LHeelEnd		= pChunk->m_LHeelEnd;
		rCAF.m_FootPlantVectors.m_LToe0Start	= pChunk->m_LToe0Start;
		rCAF.m_FootPlantVectors.m_LToe0End		= pChunk->m_LToe0End;
		rCAF.m_FootPlantVectors.m_RHeelStart	= pChunk->m_RHeelStart;
		rCAF.m_FootPlantVectors.m_RHeelEnd		= pChunk->m_RHeelEnd;
		rCAF.m_FootPlantVectors.m_RToe0Start	= pChunk->m_RToe0Start;
		rCAF.m_FootPlantVectors.m_RToe0End		= pChunk->m_RToe0End;

		rCAF.m_fStartSec				= pChunk->m_fStartSec;
		rCAF.m_fEndSec					= pChunk->m_fEndSec;
		rCAF.m_fTotalDuration		= pChunk->m_fTotalDuration;
		rCAF.m_StartLocation    = pChunk->m_StartLocation;	//asset-feature: the original location of the animation in world-space

		rCAF.m_arrController		= 0;
		rCAF.m_nControllers			= 0;
		rCAF.m_nControllers2		= pChunk->m_nControllers;
		rCAF.m_arrController		= new IController_AutoPtr[rCAF.m_nControllers2];
		for(uint32 c=0; c<rCAF.m_nControllers2; c++)
			rCAF.m_arrController[c]=0;

		rCAF.m_fSpeed						= pChunk->m_fSpeed;
		rCAF.m_fDistance				= pChunk->m_fDistance;
		assert(rCAF.m_fDistance>=0);
		rCAF.m_fSlope						= pChunk->m_fSlope;
		rCAF.m_fTurnSpeed 			=	0.0f;					//asset-features
		rCAF.m_fAssetTurn 			=	0.0f;					//asset-features
		rCAF.m_vVelocity				= Vec3(0,0,0);	//asset-features

		m_AnimationManager.m_AnimationMapCAF.InsertValue(rCAF.m_FilePathCRC32,i);
		rCAF.OnAssetCreated();
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool CharacterManager::LoadAnimationImageFileAIM( const char* filenameAIM, IChunkFile* pChunkFile )
{
	return 0;
#if (NEWAIMSYSTEM)
	stack_string strPath = filenameAIM;
	CryStringUtils::UnifyFilePath(strPath);
	if ( !pChunkFile->Read(strPath) )
	{
		pChunkFile->GetLastError();
		return 0;
	}

	// Load mesh from chunk file.
	if ((pChunkFile->GetFileHeader().Version != ChunkFileVersion) && (pChunkFile->GetFileHeader().Version != ChunkFileVersion_Align))
	{
		//m_LastError.Format("Bad CAF file version: %s", m_filename.c_str() );
		return 0;
	}

	if (pChunkFile->GetFileHeader().FileType != FileType_Geom) 
	{
		if (pChunkFile->GetFileHeader().FileType != FileType_Anim) 
		{
			//m_LastError.Format("Illegal File Type for .caf file: %s", m_filename.c_str() );
			return 0;
		}
	}

	//-------------------------------------------------------------------------------------------

	uint32 numChunck = pChunkFile->NumChunks();
	g_AnimationManager.m_arrGlobalAIM.resize(numChunck);
	for (uint32 i=0; i<numChunck; i++)
	{
		const CHUNK_HEADER &hdr = pChunkFile->GetChunkHeader(i);
		if (hdr.ChunkType!=ChunkType_GlobalAnimationHeaderAIM)
		{
			g_AnimationManager.m_arrGlobalAIM.clear();
			return 0;
		}
		IChunkFile::ChunkDesc* pChunkDesc = pChunkFile->GetChunk(i);
		if (pChunkDesc->hdr.ChunkVersion != CHUNK_GAHAIM_INFO::VERSION)
		{
			g_AnimationManager.m_arrGlobalAIM.clear();
			return 0;
		}

		CHUNK_GAHAIM_INFO* pChunk = (CHUNK_GAHAIM_INFO*)pChunkDesc->data;
		//	SwapEndian(*pChunk);

		GlobalAnimationHeaderAIM& rAIM = g_AnimationManager.m_arrGlobalAIM[i];
		rAIM.m_nFlags						= pChunk->m_Flags;
		rAIM.m_FilePath					=	pChunk->m_FilePath;
		rAIM.m_FilePathCRC32		=	pChunk->m_FilePathCRC32;
		uint32 nCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase( rAIM.m_FilePath ); 
		if (rAIM.m_FilePathCRC32 != nCRC32)	
			CryFatalError("CryAnimation: CRC32 Invalid! Most likely the endian conversion from RC is wrong");

		const char* pname = "animations/human/male/weapons/scar/3p/aimposes/stand_tac_aimposes_idle_scar_shoulder_3p_01.caf";
		if ( strcmp(pname,rAIM.m_FilePath)==0 )
		{
			uint32 ddd=0;
		} 

		rAIM.m_fStartSec				= pChunk->m_fStartSec;
		rAIM.m_fEndSec					= pChunk->m_fEndSec;
		rAIM.m_fTotalDuration		= pChunk->m_fTotalDuration;

		rAIM.m_AnimTokenCRC32		= pChunk->m_AnimTokenCRC32;
		rAIM.m_nExist						= pChunk->m_nExist;
		rAIM.m_MiddleAimPoseRot = pChunk->m_MiddleAimPoseRot;		assert(rAIM.m_MiddleAimPoseRot.IsValid());
		rAIM.m_MiddleAimPose		= pChunk->m_MiddleAimPose;		  assert(rAIM.m_MiddleAimPose.IsValid());
		
		for (uint32 v=0; v<(CHUNK_GAHAIM_INFO::XGRID*CHUNK_GAHAIM_INFO::YGRID); v++)
			rAIM.m_PolarGrid[v]=pChunk->m_PolarGrid[v];

		char* pAimPoseMem = (char*)&pChunk->m_numAimPoses;
		uint32 numAimPoses = *(uint32*)(pAimPoseMem); 	pAimPoseMem+=sizeof(uint32);
		rAIM.m_arrAimIKPosesAIM.resize(numAimPoses);
		for (uint32 a=0; a<numAimPoses; a++)
		{
			uint32 numRot = *(uint32*)(pAimPoseMem); pAimPoseMem+=sizeof(uint32);
			rAIM.m_arrAimIKPosesAIM[a].m_arrRotation.resize(numRot);
			for (uint32 r=0; r<numRot; r++)
			{
				Quat quat = *(Quat*)(pAimPoseMem); pAimPoseMem+=sizeof(Quat);
				assert(quat.IsValid());
				rAIM.m_arrAimIKPosesAIM[a].m_arrRotation[r]=quat;
			}
			uint32 numPos = *(uint32*)(pAimPoseMem); pAimPoseMem+=sizeof(uint32);
			rAIM.m_arrAimIKPosesAIM[a].m_arrPosition.resize(numPos);
			for (uint32 p=0; p<numPos; p++)
			{
				Vec3 pos = *(Vec3*)(pAimPoseMem); pAimPoseMem+=sizeof(Vec3);
				assert(pos.IsValid());
				rAIM.m_arrAimIKPosesAIM[a].m_arrPosition[p]=pos;
			}
			
		}

		rAIM.OnAssetCreated();
	}
#endif
	return true;
}



#include UNIQUE_VIRTUAL_WRAPPER(ICharacterManager)
