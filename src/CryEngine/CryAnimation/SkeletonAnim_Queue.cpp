//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:Anim_Weights.cpp
//  Implementation of Animation class for parameterisation
//
//	History:
//	January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "CharacterInstance.h"
#include "Model.h"
#include "ModelSkeleton.h"
#include "CharacterManager.h"
#include <float.h>
#include "LMG.h"
#include "LoaderDBA.h"


SPU_NO_INLINE bool CSkeletonAnim::StartAnimation(const char* szAnimName, const struct CryCharAnimationParams& Params)
{
	if (szAnimName==0)
	{
#if !defined(__SPU__)
		g_pILog->LogError ("No name for animation specified");
#endif
		assert(!"No name for animation specified");
		return false;
	}

	if (szAnimName)
	{
		uint32 isNullAnimation = strstr(szAnimName, "null") || strstr(szAnimName, "NULL");
		if (isNullAnimation)
		{
			m_bSetDefaultPose=1;
			return false; 
		}
	}

	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
	int32 nAnimID = pAnimationSet->GetAnimIDByName(szAnimName);
	if (nAnimID<0)	
	{
		if (Console::GetInst().ca_AnimWarningLevel > 0)
		{
			bool bShowWarnings = !(Params.m_nFlags & CA_SUPPRESS_WARNINGS);
			if (bShowWarnings)
				AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"animation-name '%s' not in CAL-file", szAnimName );
			CRY_ASSERT_MESSAGE(!"animation-name not in CAL-file",szAnimName);
		}
		return 0;
	}

	return StartAnimationById(nAnimID, Params);
}

SPU_NO_INLINE bool CSkeletonAnim::StartAnimationById(int32 id, const struct CryCharAnimationParams& Params)
{

	//m_threadTask.Wait();
	//m_bDidRun = false;

/*
	FinishAnimationComputations();

		while (m_bDidRun)
		{

		}*/
/*	if (m_bDidRun)
	{
		CryFatalError("StartAnimation: Thread Running");
	}*/


	DEFINE_PROFILER_FUNCTION();
	if (m_pInstance->m_bEnableStartAnimation==0)
		return 0;

	if (Params.m_nLayerID>=numVIRTUALLAYERS || Params.m_nLayerID<0)
		return 0;

	if(!m_pSkeletonPose->m_parrModelJoints)
		return 0;

	const char* RootName = m_pSkeletonPose->m_parrModelJoints[0].GetJointName();
	if (RootName == 0)
		return 0;
	if (g_pCharacterManager->m_IsDedicatedServer && RootName[0]=='B' && RootName[1]=='i' && RootName[2]=='p' && RootName[3]=='0' && RootName[4]=='1')
		return 0; //we don't load animation for humans on dedicated server

	m_bSetDefaultPose=0;

	//	const char* mname = m_pInstance->GetFilePath();
	//	if ( strcmp(mname,"objects/library/architecture/multiplayer/prototype_factory/pf_factory_gate.cga")==0 )
	//		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,mname,	"StartAnimation: %s",szAnimName0);

	CryCharAnimationParams AnimPrams = Params;
	if (AnimPrams.m_nLayerID>0)
	{
		//	AnimPrams.m_nFlags|=CA_PARTIAL_BODY_UPDATE;
		uint32 loop=AnimPrams.m_nFlags&CA_LOOP_ANIMATION;
		uint32 repeat=AnimPrams.m_nFlags&CA_REPEAT_LAST_KEY;
		if (loop==0 && repeat==0)
		{
			AnimPrams.m_nFlags|=CA_REPEAT_LAST_KEY;
			AnimPrams.m_nFlags|=CA_FADEOUT;
		}
	}

	//----------------------------------------------------------------------------

	uint32 nDisableMultilayer = AnimPrams.m_nFlags & CA_DISABLE_MULTILAYER;
	if (nDisableMultilayer)
	{
		AnimPrams.m_fAllowMultilayerAnim=0.0f;
	}

	//----------------------------------------------------------------------------

	bool bShowWarnings = !(Params.m_nFlags & CA_SUPPRESS_WARNINGS);
	uint32 TrackViewExclusive = Params.m_nFlags & CA_TRACK_VIEW_EXCLUSIVE;
	if (m_TrackViewExclusive && !TrackViewExclusive)
		return 0;

	const ModelAnimationHeader* pAnim=SPU_CONST_MAIN_PTR(0);
	int32 nAnimID			= id;

	const ModelAnimationHeader* pAnimAim0=SPU_CONST_MAIN_PTR(0); 
	int32 nAnimAimID0		=-1;
	const ModelAnimationHeader* pAnimAim1=SPU_CONST_MAIN_PTR(0); 
	int32 nAnimAimID1		=-1;

	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;

	assert(Params.m_fTransTime<60.0f); //transition times longer than 1 minute are useless

	if (nAnimID<0)	
		return 0;

	//--------------------------------------------------------------
	//---                evaluate the 1st animation              ---
	//--------------------------------------------------------------

	pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimID) );
	const char* szAnimName = pAnim->GetAnimName();

	if (pAnim==0)	
	{
		assert(0);
		return 0;
	}

	uint32 nGlobalID = pAnim->m_nGlobalAnimId;


	//-----------------------------------------------------------------------------------------------------------------------
	//--- CAF start     -----------------------------------------------------------------------------------------------------
	//-----------------------------------------------------------------------------------------------------------------------
	if (pAnim->m_nAssetType==CAF_File)
	{
		GlobalAnimationHeaderCAF& rHAG_CAF = g_AnimationManager.m_arrGlobalCAF[nGlobalID];
		uint32 IsCreated = rHAG_CAF.IsAssetCreated();
		if (IsCreated==0)		
		{
			if (Console::GetInst().ca_AnimWarningLevel > 0)
				AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"Asset for animation-name '%s' does not exist", pAnim->GetAnimName());
			return 0;
		}

		if (rHAG_CAF.IsAssetOnDemand())
		{
			if (rHAG_CAF.IsAssetLoaded()==0 && rHAG_CAF.IsAssetRequested()==0)
				pAnimationSet->StreamCAF(nGlobalID,1);

			if (rHAG_CAF.IsAssetLoaded()==0)
			{
				uint32 ddd=0;(void)ddd;
			}
		}
	}


	//-----------------------------------------------------------------------------------------------------------------------
	//--- CAF start     -----------------------------------------------------------------------------------------------------
	//-----------------------------------------------------------------------------------------------------------------------
	if (pAnim->m_nAssetType==AIM_File)
	{
		GlobalAnimationHeaderAIM& rHAG_AIM = g_AnimationManager.m_arrGlobalAIM[nGlobalID];
		uint32 IsCreated = rHAG_AIM.IsAssetCreated();
		if (IsCreated==0)		
		{
			if (Console::GetInst().ca_AnimWarningLevel > 0)
				AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"Asset for animation-name '%s' does not exist", pAnim->GetAnimName());
			return 0;
		}

		if (rHAG_AIM.IsAimposeUnloaded())
		{
			if (Params.m_nLayerID != m_pSkeletonPose->m_AimIK().m_nLayer)
			{
				if (Console::GetInst().ca_AnimWarningLevel>0 && bShowWarnings)
					AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"trying to play an unloaded aimpose '%s'", szAnimName );
				return 0;
			}
		}
	}


	//-----------------------------------------------------------------------------------------------------------------------
	//--- LMG start     -----------------------------------------------------------------------------------------------------
	//-----------------------------------------------------------------------------------------------------------------------
	if (pAnim->m_nAssetType==LMG_File)
	{
		GlobalAnimationHeaderLMG& rHAG_LMG = g_AnimationManager.m_arrGlobalLMG[nGlobalID];
		assert( rHAG_LMG.IsAssetLMG() ); 
		uint32 IsCreated = rHAG_LMG.IsAssetCreated();
		if (IsCreated==0)		
		{
			if (Console::GetInst().ca_AnimWarningLevel>0)
				AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"Asset for animation-name '%s' does not exist", pAnim->GetAnimName());
			return 0;
		}

		if (Params.m_nLayerID)
			return 0; //LMGs in laers are currently not supported

		if (rHAG_LMG.IsAssetLMGValid()==0)
		{
			ANIM_ASSET_CHECK_TRACE(rHAG_LMG.IsAssetLMGValid() != 0, ("LMG is invalid: '%s'", szAnimName));
			return 0;
		} 

		rHAG_LMG.m_nTouchedCounter++;

		uint32 numLMGAnims = rHAG_LMG.m_arrBSAnimations.size();
		for (uint32 i=0; i<numLMGAnims; i++)
		{
			int local = pAnimationSet->GetAnimIDByName( rHAG_LMG.m_arrBSAnimations[i].m_strAnimName );
			int global = pAnimationSet->GetGlobalIDByAnimID(local);
			if (global != -1)
			{
				GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[global];
				if (rGAH.IsAssetOnDemand())
				{
					if (rGAH.IsAssetLoaded()==0 && rGAH.IsAssetRequested()==0)
						pAnimationSet->StreamCAF(global,1);
				}
			}
		}

		uint32 numSpliceAnims = rHAG_LMG.m_strSpliceAnim.size();
		for (uint32 i=0; i<numSpliceAnims; i++)
		{
			int local = pAnimationSet->GetAnimIDByName( rHAG_LMG.m_strSpliceAnim[i] );
			int global = pAnimationSet->GetGlobalIDByAnimID(local);
			if (global != -1)
			{
				GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[global];
				if (rGAH.IsAssetOnDemand())
				{
					if (rGAH.IsAssetLoaded()==0 && rGAH.IsAssetRequested()==0)
						pAnimationSet->StreamCAF(global,1);
				}
			}
		}


	}


	//-----------------------------------------------------------------------------------------------------------------------
	//--- PMG start     -----------------------------------------------------------------------------------------------------
	//-----------------------------------------------------------------------------------------------------------------------
	if (pAnim->m_nAssetType==PMG_File)
	{
		GlobalAnimationHeaderPMG& rHAG_PMG = g_AnimationManager.m_arrGlobalPMG[nGlobalID];
		uint32 IsCreated = rHAG_PMG.IsAssetCreated();
		if (IsCreated==0)		
		{
			if (Console::GetInst().ca_AnimWarningLevel>0)
				AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"Asset for animation-name '%s' does not exist", pAnim->GetAnimName());
			return 0;
		}

		assert(rHAG_PMG.IsAssetPMG()); 
	//	rHAG_PMG.OnAssetLMG();

		//if (rHAG_PMG.IsAssetPMG()) 
		{
			if (Params.m_nLayerID)
				return 0; //LMGs in laers are currently not supported

			if (rHAG_PMG.IsAssetPMGValid()==0)
			{
				ANIM_ASSET_CHECK_TRACE(rHAG_PMG.IsAssetPMGValid() != 0, ("PMG is invalid: '%s'", szAnimName));
				return 0;
			} 

			rHAG_PMG.m_nTouchedCounter++;

			uint32 num = rHAG_PMG.m_arrBSAnimationsPMG.size();
			for (uint32 i=0; i<num; i++)
			{
				int local = pAnimationSet->GetAnimIDByName( rHAG_PMG.m_arrBSAnimationsPMG[i].m_strAnimName );
				int global = pAnimationSet->GetGlobalIDByAnimID(local);
				if (global != -1)
				{
					GlobalAnimationHeaderCAF& rGAH0 = g_AnimationManager.m_arrGlobalCAF[global];
					if (rGAH0.IsAssetOnDemand())
					{
						if (rGAH0.IsAssetLoaded()==0 && rGAH0.IsAssetRequested()==0)
						{
							pAnimationSet->StreamCAF(global,1);
						}
						if (rGAH0.IsAssetLoaded()==0)
						{
							uint32 ddd=0;(void)ddd;
						}
					}
				}
			}
		}
	}




	//--------------------------------------------------------------
	//---                evaluate the aim-pose                   ---
	//--------------------------------------------------------------
	const char* szAim0 = pAnim->m_strAimPose1[0];
	if (szAim0 && szAim0[0]==0)
		szAim0=0;

	const char* szAim1 = 0;
	if (szAim0)
	{
		nAnimAimID0 = pAnimationSet->GetAnimIDByName( szAim0 );
		if (nAnimAimID0<0)	
		{
			if (Console::GetInst().ca_AnimWarningLevel>0 && bShowWarnings)
				AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"animation-name '%s' not in CAL-file", szAim0);
			assert(0);
			return 0;
		}
		//check if the asset exists
		pAnimAim0 = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimAimID0) );
		if (pAnimAim0==0)	
		{
			assert(0);	return 0;
		}

		szAim1 = pAnim->m_strAimPose1[1];
		if (szAim1 && szAim1[0]==0) szAim1=0;
		if (szAim1)
		{
			nAnimAimID1 = pAnimationSet->GetAnimIDByName( szAim1 );
			if (nAnimAimID1<0)	
			{
				if (Console::GetInst().ca_AnimWarningLevel>0 && bShowWarnings)
					AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"animation-name '%s' not in CAL-file", szAim1);
				assert(0);
				return 0;
			}
			//check if the asset exists
			pAnimAim1 = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimAimID1) );
			if (pAnimAim1==0)	
			{
				assert(0);	return 0;
			}

		}
	}




#if !defined(__SPU__) // superimposed is only used from the editor, so no need for it on spu
	if (m_pSkeletonPose->m_Superimposed)
	{
		uint32 tr=m_AnimationDrivenMotion;
		m_AnimationDrivenMotion=0;
		extern std::vector< std::vector<DebugJoint> > g_arrSkeletons;
		extern int32 g_nAnimID;
		extern int32 g_nGlobalAnimID;

		g_nAnimID				=	nAnimID;
		g_nGlobalAnimID	=	nGlobalID;

		pAnimationSet->SetFootplantBitsAutomatically( g_arrSkeletons, nAnimID,nGlobalID, m_pSkeletonPose->m_pModelSkeleton->m_IdxArray[eIM_LHeelIdx],m_pSkeletonPose->m_pModelSkeleton->m_IdxArray[eIM_RHeelIdx], m_pSkeletonPose->m_pModelSkeleton->m_IdxArray[eIM_LToe0Idx],m_pSkeletonPose->m_pModelSkeleton->m_IdxArray[eIM_RToe0Idx], 1 );
		m_AnimationDrivenMotion=tr;
	}
#endif

	bool result = AnimationToQueue ( pAnim,nAnimID, pAnimAim0,nAnimAimID0,pAnimAim1,nAnimAimID1,  -1, AnimPrams) > 0;

	if (result)
	{
		if (TrackViewExclusive)
		{
			int32 Layer = Params.m_nLayerID;
			int32 numAnims = m_arrLayer_AFIFO[Layer].size()-1;
			if (numAnims>0)
				for (int32 i=0; i<numAnims; i++)
					m_arrLayer_AFIFO[Layer][i].m_AnimParams.m_fTransTime=fabsf(m_arrLayer_AFIFO[Layer][i].m_AnimParams.m_fTransTime*0.01f);
		}

		SetActiveLayer(Params.m_nLayerID,1);
		m_arrLayerBlendingTime[Params.m_nLayerID] = fabsf(Params.m_fTransTime);
		m_arrLayerBlendingMult[Params.m_nLayerID] = Params.m_fLayerWeight;
	}

	return result;
}




// stops the animation at the given layer, and returns true if the animation was
// actually stopped (if the layer existed and the animation was played there)
bool CSkeletonAnim::StopAnimationInLayer (int32 nLayer, f32 BlendOutTime)
{
	if (nLayer<0 || nLayer>=numVIRTUALLAYERS)
	{
		AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"illegal layer index used in function StopAnimationInLayer: '%d'", nLayer);
		assert(0);

		return 0;
	}

	//	const char* mname = m_pInstance->GetFilePath();
	//	if ( strcmp(mname,"objects/library/architecture/multiplayer/prototype_factory/pf_factory_gate.cga")==0 )
	//		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,mname,	"StopAnimationInLayer");

	SetActiveLayer(nLayer,0);
	m_arrLayerBlendingTime[nLayer]=BlendOutTime;
	if (nLayer)
		return 1;

	ClearFIFOLayer(nLayer);

	return 1;
}

// stops the animation at the given layer, and returns true if the animation was
// actually stopped (if the layer existed and the animation was played there)
bool CSkeletonAnim::StopAnimationsAllLayers ()
{
	//	const char* mname = m_pInstance->GetFilePath();
	//	if ( strcmp(mname,"objects/library/architecture/multiplayer/prototype_factory/pf_factory_gate.cga")==0 )
	//		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,mname,	"StopAnimationsAllLayers");

	for (uint32 i=0; i<numVIRTUALLAYERS; i++)
		ClearFIFOLayer(i);

	return 1;
}


//////////////////////////////////////////////////////////////////////////
// push a new animation into the queue
//////////////////////////////////////////////////////////////////////////
uint32 CSkeletonAnim::AnimationToQueue ( const ModelAnimationHeader* pAnim0,int nID0,     const ModelAnimationHeader* pAnimAim0,int nAimID0,const ModelAnimationHeader* pAnimAim1,int nAimID1,   f32 btime, const CryCharAnimationParams& AnimParams)
{
	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
	uint32 status=0;

	// use for multi threading to remeber if we have to remove the first
	// animation from the queue in case StartAnimations was called while AnimationComputations were
	// running(for example SPUs)
	bool bRemoveFirstAnimInQueue = false;

	if (pAnim0)
	{
		uint32 nGlobalAnimId = pAnim0->m_nGlobalAnimId;
		if (pAnim0->m_nAssetType==LMG_File)
		{
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[nGlobalAnimId];
			if (rGlobalAnimHeader.IsAssetLMG())
				status|=1;
		}

		if (pAnim0->m_nAssetType==PMG_File)
		{
			GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[nGlobalAnimId];
			if (rGlobalAnimHeader.IsAssetPMG())
				status|=1;
		}
	}

	uint32 aar=AnimParams.m_nFlags&CA_ALLOW_ANIM_RESTART;
	if (aar==0)
	{
		//don't allow to start the same animation twice
		uint32 numAnimsOnStack = m_arrLayer_AFIFO[AnimParams.m_nLayerID].size();
		if (numAnimsOnStack)
		{

			//check is it the same animation only if the aim poses are different
			int32 nPrevAimID0 = m_arrLayer_AFIFO[AnimParams.m_nLayerID][numAnimsOnStack-1].m_nAnimAimID0;
			if (nPrevAimID0<0) nPrevAimID0=-1;

			if (nPrevAimID0==nAimID0)
			{
				CAnimation const & lastAnimation = m_arrLayer_AFIFO[AnimParams.m_nLayerID][numAnimsOnStack-1];

				int32 id0=lastAnimation.m_Parametric.m_nParametricID;
				if (id0<0)
					id0=lastAnimation.m_Parametric.m_nAnimID[0];
				if (id0<0) id0=-1;

				int32 num=(id0>=0); 

				if (status==0) 
				{
					if (num==1)
						if (lastAnimation.m_Parametric.m_nAnimID[0]==nID0)
							return 0;
				}

				if (status==1) 
				{
					if (num==1)
						if (lastAnimation.m_Parametric.m_nParametricID==nID0)
							return 0;
				}

				if (status==2) 
				{
					if (num==1)
						if (lastAnimation.m_Parametric.m_nAnimID[0]==nID0)
							return 0;
				}

				if (status==3) 
				{
					if (num==1)
						if (lastAnimation.m_Parametric.m_nParametricID==nID0)
							return 0;
				}
			}
		}
	}


	if (AnimParams.m_nFlags&CA_REMOVE_FROM_FIFO)
	{
		if( m_bDidRun )
		{
			bRemoveFirstAnimInQueue = true;
		}
		else
		{
			RemoveFirstAnimationFromQueue( AnimParams.m_nLayerID );
		}					
	}


	CAnimation AnimOnStack;

	if (pAnim0)
	{
		if (pAnim0->m_nAssetType==CAF_File)
			AnimOnStack.m_Parametric = LMG::BuildRuntimeStructCAF(pAnimationSet, pAnim0,nID0);
		if (pAnim0->m_nAssetType==AIM_File)
			AnimOnStack.m_Parametric = LMG::BuildRuntimeStructAIM(pAnimationSet, pAnim0,nID0);
		if (pAnim0->m_nAssetType==LMG_File)
			AnimOnStack.m_Parametric = LMG::BuildRuntimeStructLMG(pAnimationSet, pAnim0,nID0);
		if (pAnim0->m_nAssetType==PMG_File)
			AnimOnStack.m_Parametric = LMG::BuildRuntimeStructPMG(pAnimationSet, pAnim0,nID0);

		if (pAnimAim0)
		{
			AnimOnStack.m_strAimPosName0	=	pAnimAim0->GetAnimName();
			AnimOnStack.m_nAnimAimID0			= nAimID0;
			AnimOnStack.m_nGlobalAimID0		= pAnimAim0->m_nGlobalAnimId;

			if (pAnimAim1)
			{
				AnimOnStack.m_strAimPosName1	=	pAnimAim1->GetAnimName();
				AnimOnStack.m_nAnimAimID1			= nAimID1;
				AnimOnStack.m_nGlobalAimID1		= pAnimAim1->m_nGlobalAnimId;
			}
		}

	}

	// AnimParams is not initialized in the constructor anymore.
	// BuildRealTimeLMG does NOT initialize AnimParams implicitly, for cases where the caller overwrite stuff anyway. 
	AnimOnStack.m_Parametric.m_BlendSpace.Init();

	AnimOnStack.m_nSegHighest=0;

	const uint32 numAnims0 = AnimOnStack.m_Parametric.m_numAnims;

	for (uint32 i=0; i<numAnims0; i++)
	{
		int32 nAnimID = AnimOnStack.m_Parametric.m_nAnimID[i];
		const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimID) );
		if (pAnim->m_nAssetType==CAF_File)
		{
			int32 GlobalID = pAnimationSet->GetGlobalIDByAnimID_Fast(AnimOnStack.m_Parametric.m_nAnimID[i]);
			GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[ GlobalID ];
			//		rGAH.StartAnimation();
			int32 seg = rGAH.m_Segments;
			if (AnimOnStack.m_nSegHighest<seg)
				AnimOnStack.m_nSegHighest=seg;

			rGAH.m_nRef_at_Runtime++;

			if (rGAH.m_nControllers==0)
			{
				if (rGAH.m_FilePathDBACRC32==0)
					continue;

				size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
				for (uint32 d=0; d<numDBA_Files; d++)
				{
					CGlobalHeaderDBA& pGlobalHeaderDBA = *g_AnimationManager.m_arrGlobalHeaderDBA[d];
					if (rGAH.m_FilePathDBACRC32!=pGlobalHeaderDBA.m_FilePathDBACRC32)
						continue;

					if (pGlobalHeaderDBA.m_pDatabaseInfo==0)
					{
						const char* pName  = pGlobalHeaderDBA.m_strFilePathDBA;
						pGlobalHeaderDBA.StreamDatabaseDBA();
					}

					if (pGlobalHeaderDBA.m_pDatabaseInfo)
					{
						const CCommonSkinningInfo* pSkinningInfo = pGlobalHeaderDBA.GetSkinningInfoDBA(rGAH.m_FilePath);
						uint32 numController = pSkinningInfo->m_pControllers.size();
						if (numController!=rGAH.m_nControllers2)
							CryFatalError("Controller mismatch");

						for(uint32 s=0; s<numController; s++ )
							rGAH.m_arrController[s] = pSkinningInfo->m_pControllers[s];
						rGAH.m_nControllers = numController;
						std::sort(rGAH.m_arrController,	rGAH.m_arrController + numController, AnimCtrlSortPred()	);

						rGAH.InitControllerLookup();
						rGAH.OnAssetCreated();
						rGAH.OnAssetLoaded();
						rGAH.ClearAssetRequested();
					}
					break;
				}
			}


		}
	}


	if (AnimParams.m_fKeyTime >= 0.0f)
	{
		assert(AnimParams.m_fKeyTime<=1.0f);
		AnimOnStack.m_fAnimTimePrev				=	AnimParams.m_fKeyTime;
		AnimOnStack.m_fAnimTime						=	AnimParams.m_fKeyTime;
	}
	else
	{
		AnimOnStack.m_fAnimTimePrev				=	0.0f;
		AnimOnStack.m_fAnimTime						=	0.0f;
	}

	AnimOnStack.m_fTransitionPriority	=	0.0f;
	AnimOnStack.m_fTransitionWeight   =	0.0f;
	AnimOnStack.m_nRepeatCount        = 0;
	AnimOnStack.m_AnimParams					=	AnimParams;



	assert(AnimOnStack.m_fAnimTime>=0.0f);
	assert(AnimOnStack.m_fAnimTime<=1.0f);

	UpdateMotionParamDescs(AnimOnStack.m_Parametric, 1.0f);

	if( m_bDidRun )
	{
		m_arrDeferredQueueUpdates.push_back( DeferredQueueUpdate( bRemoveFirstAnimInQueue,AnimParams.m_nLayerID, AnimOnStack ));
	}
	else
	{
		AppendAnimationToQueue( AnimParams.m_nLayerID, AnimOnStack );	
	}	

	

	return 1;
}

void CSkeletonAnim::RemoveFirstAnimationFromQueue( int32 nLayer )
{
	uint32 numAnims=m_arrLayer_AFIFO[nLayer].size();
	if (numAnims>0x0e)
	{
		for (uint32 i=1; i<numAnims; i++)	
			m_arrLayer_AFIFO[nLayer][i-1]=m_arrLayer_AFIFO[nLayer][i];

		m_arrLayer_AFIFO[nLayer].pop_back();
	}	
}

void CSkeletonAnim::AppendAnimationToQueue( int32 nLayer, const CAnimation &rAnim )
{
	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;

	m_arrLayer_AFIFO[nLayer].push_back( rAnim );

	uint32 numAnimsInQueue = m_arrLayer_AFIFO[nLayer].size();

/* //Diesel cut
	if (numAnimsInQueue>0x10)
	{
		AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"Animation-queue overflow. More then %d entries. This is a serious performance problem.",numAnimsInQueue);
		if (numAnimsInQueue>0x80)
		{
			//this might be the perfect moment for a FATAL-ERROR
		}
	}
*/ //Diesel cut

	//activate assets immediately, if possible
	uint32 nMaxActiveInQueue=MAX_EXEC_QUEUE;	
	if (nMaxActiveInQueue>numAnimsInQueue)
		nMaxActiveInQueue=numAnimsInQueue;
	for (uint32 i=0; i<nMaxActiveInQueue; i++)
	{
		m_arrLayer_AFIFO[nLayer][i].m_bActivated=0;
		uint32 NotInMemory=0;



		//		int32 global_lmg_id = m_arrLayer_AFIFO[nLayer][i].m_Parametric.m_nGlobalLMGID;
		uint32 num = m_arrLayer_AFIFO[nLayer][i].m_Parametric.m_numAnims;
		for (uint32 a=0; a<num; a++)
		{

			int32 nAnimID = m_arrLayer_AFIFO[nLayer][i].m_Parametric.m_nAnimID[a];
			const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimID) );
			if (pAnim->m_nAssetType==CAF_File)
			{
				int32 nGlobalID = pAnim->m_nGlobalAnimId;
				GlobalAnimationHeaderCAF& rGAHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalID];
				if (rGAHeader.IsAssetOnDemand())
				{
					uint32 loaded = rGAHeader.IsAssetLoaded();
					if (loaded==0)
					{
						NotInMemory=1;
						break;
					}
				}
			}
			if (pAnim->m_nAssetType==AIM_File)
			{
				int32 nGlobalID = pAnim->m_nGlobalAnimId;
				GlobalAnimationHeaderAIM& rGAHeader = g_AnimationManager.m_arrGlobalAIM[nGlobalID];
				if (rGAHeader.IsAssetOnDemand())
				{
					uint32 loaded = rGAHeader.IsAssetLoaded();
					if (loaded==0)
					{
						NotInMemory=1;
						break;
					}
				}
			}

		}

		if (NotInMemory==0 && i==0)
			m_arrLayer_AFIFO[nLayer][i].m_bActivated=1;

		if (i==0)
			continue;
		if (m_arrLayer_AFIFO[nLayer][i-1].m_bActivated==0)
			break;

		uint32 StartAtKeytime = m_arrLayer_AFIFO[nLayer][i].m_AnimParams.m_nFlags & CA_START_AT_KEYTIME;
		uint32 StartAfter			= m_arrLayer_AFIFO[nLayer][i].m_AnimParams.m_nFlags & CA_START_AFTER;
		uint32 Idle2Move			= m_arrLayer_AFIFO[nLayer][i].m_AnimParams.m_nFlags & CA_IDLE2MOVE;
		uint32 Move2Idle			= m_arrLayer_AFIFO[nLayer][i].m_AnimParams.m_nFlags & CA_MOVE2IDLE;
		if ( (StartAtKeytime+StartAfter+Idle2Move+Move2Idle+NotInMemory) == 0)
			m_arrLayer_AFIFO[nLayer][i].m_bActivated = 1;
	}
}

void CSkeletonAnim::ClearFIFOLayer( uint32 nLayer )
{
#if !defined(__SPU__)
	// make sure no parallel computations are running while serialization
	//FinishAnimationComputations();
#endif
	uint32 numAnimsInLayer=m_arrLayer_AFIFO[nLayer].size();
	for (size_t i=0; i<numAnimsInLayer; i++)
		UnloadAnimationAssets(m_arrLayer_AFIFO[nLayer],i);

	m_arrLayer_AFIFO[nLayer].clear();
}


void CSkeletonAnim::UnloadAnimationAssets(DynArray<CAnimation>& arrAFIFO, int num)
{
/*
	const char* mname = m_pInstance->GetFilePath();
	uint32 identical=0;
	identical |= ( strcmp(mname,"objects/weapons/us/frag_grenade/frag_grenade.chr")==0 );
	identical |= ( strcmp(mname,"objects/weapons/us/scar_v2/scar_fp.cdf")==0 );
	identical |= ( strcmp(mname,"objects/weapons/us/nova/nova_fp.cdf")==0 );
//	if (Console::GetInst().ca_DebugText || m_ShowDebugText)
	{
		if (identical)
		{
			f32 fColor[4] = {1,0,0,1};
			const char* mname = m_pInstance->GetFilePath();
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 3.2f, fColor, false,"model: %s",mname); 
			g_YLine+=0x20;
		}
	}*/


	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;

	CAnimation& rAnim = arrAFIFO[num];

	uint32 numAnims0 = rAnim.m_Parametric.m_numAnims;
	for (uint32 i=0; i<numAnims0; i++)
	{
		int32 nAnimID = rAnim.m_Parametric.m_nAnimID[i];
		const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimID) );
		if (pAnim->m_nAssetType==CAF_File)
		{
#ifdef _DEBUG
			const char* pname = pAnim->GetAnimName();
#endif
			int32 nGlobalID = pAnimationSet->GetGlobalIDByAnimID_Fast(rAnim.m_Parametric.m_nAnimID[i]);
			GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[ nGlobalID ];

			if (rGAH.m_nRef_at_Runtime)
			{
				rGAH.m_nRef_at_Runtime--;
			/*	int nCurrentFrameID = g_pCharacterManager->m_nUpdateCounter; 
				const char* e0 = strstr(rGAH.m_FilePath,"male");
				if (e0)
					continue;
				const char* e1 = strstr(rGAH.m_FilePath,"alien");
				if (e1)
					continue;
				g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,0,	"Dec: Frame:%d  %x Name: %s",nCurrentFrameID, rGAH.m_nRef_at_Runtime,rGAH.m_FilePath );
				*/
			}

			if (rGAH.IsAssetOnDemand())
			{
				if (rGAH.m_nRef_at_Runtime==0)
				{
					g_AnimationManager.UnloadAnimationCAF(nGlobalID);
				}
			}
		}



	}
}

