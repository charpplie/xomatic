//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:Skeleton.cpp
//  Implementation of Skeleton class (Forward Kinematics)
//
//	History:
//	January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include <float.h>
#include "CharacterInstance.h"
#include "CommandBuffer.h"

#include "Command_Commands.h"

SPU_NO_INLINE void CommandBuffer::Create(CCharInstance* pInstance, const QuatT& rPhysLocationCurr, const QuatTS& rAnimLocationCurr)
{
	m_pInstance = pInstance;
	m_pSkeletonPose = &pInstance->m_SkeletonPose;
	m_pSkeletonAnim = &pInstance->m_SkeletonAnim;

	m_pSkeletonPose->m_feetLock.Reset();

	m_buffer.Initialize(m_pInstance);

	// store often used pointer in local variables to improve SPU cache simulation
	CSkeletonAnim *pSkeletonAnim = m_pSkeletonAnim;

//	m_state.m_lod = m_pInstance->GetAnimationLOD();

//	m_CommandBufferCounter=0;
//	m_CommandBufferInstructions=0;

#if !defined(__SPU__)
	//just for debugging
	extern uint32 g_AnimationUpdates; 	g_AnimationUpdates++;
#endif


	//compute number of animation in this layer
	DynArray<CAnimation> &rCurLayer = pSkeletonAnim->m_arrLayer_AFIFO[0];
	uint32 numAnimsInLayer = rCurLayer.size();
	uint32 numActiveAnims=0;
	for (uint32 a=0; a<numAnimsInLayer; a++)
	{
		if (rCurLayer[a].m_bActivated==0)
			break;
		numActiveAnims++;	
	}


	if( numActiveAnims )
	{
		pSkeletonAnim->m_fAllowMultilayerAnim = 0.0f;
		for (int i=0; i<NUM_ANIMATION_USER_DATA_SLOTS; i++)
			pSkeletonAnim->m_fUserData[i] = 0.0f;

		f32 fTransWeight=0;
		for (uint32 a=0; a<numActiveAnims; a++)
		{
			f32 fAllowMultilayer = f32( (rCurLayer[a].m_AnimParams.m_nFlags & CA_DISABLE_MULTILAYER) ==0 );
			pSkeletonAnim->m_fAllowMultilayerAnim += fAllowMultilayer*rCurLayer[a].m_fTransitionWeight;

			// store often used pointer/variables in local variables to improve SPU cache simulation
			f32 fTransitionWeight = rCurLayer[a].m_fTransitionWeight;			
			f32 *pSrc = &rCurLayer[a].m_AnimParams.m_fUserData[0];
			f32 *pDst = &pSkeletonAnim->m_fUserData[0];

			for (int i=0; i<NUM_ANIMATION_USER_DATA_SLOTS; i++)
				pDst[i] += pSrc[i]*fTransitionWeight;

			fTransWeight   += rCurLayer[a].m_fTransitionWeight;
		}

		if (m_pSkeletonPose->m_bFullSkeletonUpdate==0)
			return;

		//----------------------------------------------------------------------------------------
		//------             create the Command-Buffer for the base-layer                      ---
		//----------------------------------------------------------------------------------------
		Command::ClearFull* clearbuffer = m_buffer.CreateCommand<Command::ClearFull>();
		clearbuffer->m_TargetBuffer		= Command::TargetBuffer;


		f32 fRootTransition=0;
		for (uint32 i=0; i<numActiveAnims; i++)
		{
			fRootTransition=0;
			SpuStackValue<CAnimation, false,true> spuAnim( rCurLayer[i] );
			const CAnimation &rAnim1 = spuAnim;		
			BasePlayback(spuAnim,fRootTransition);
		}

		f32 fRootScale=1.0f;
		if (fRootTransition)
			fRootScale=1.0f/fRootTransition;

		Command::NormalizeFull* norm = m_buffer.CreateCommand<Command::NormalizeFull>();
		norm->m_TargetBuffer		= Command::TargetBuffer;
		norm->m_fRootScale			=	fRootScale;

		if (rAnimLocationCurr.s != 1.0f)
		{
			Command::ScaleUniformFull* uscale = m_buffer.CreateCommand<Command::ScaleUniformFull>();
			uscale->m_TargetBuffer	= Command::TargetBuffer;
			uscale->m_fScale				=	rAnimLocationCurr.s;
		}
	}

	//------------------------------------------------------------------------------------
	//---                   and now process the layers 1-15                            ---
	//------------------------------------------------------------------------------------
/*
	uint32 hasLayerAnimation=0;
	for(uint32 nVLayerNo=1; nVLayerNo<numVIRTUALLAYERS; nVLayerNo++)
	{
		DynArray<CAnimation> &rCurLayer = pSkeletonAnim->m_arrLayer_AFIFO[nVLayerNo];
		uint32 numAnimsInLayer = rCurLayer.size();
		uint32 numActiveAnims=0;
		for (uint32 a=0; a<numAnimsInLayer; a++)
		{
			if (rCurLayer[a].m_bActivated==0)
				break;
			numActiveAnims++;
		}
		if (numActiveAnims==0)
			continue;

		hasLayerAnimation++;
	}	*/

	uint32 nVLayerPre = numVIRTUALLAYERS;
	if (m_pSkeletonPose->m_AimIK().m_nLayer<numVIRTUALLAYERS)
		nVLayerPre=m_pSkeletonPose->m_AimIK().m_nLayer;

	CModelSkeleton* const __restrict 	pModelSkeleton	= m_pSkeletonPose->m_pModelSkeleton;
//	if ( numActiveAnims && hasLayerAnimation)
	if ( numActiveAnims)
	{
		uint32 t0=uint32(Console::GetInst().ca_LockFeetWithIK);
		uint32 t1=pSkeletonAnim->m_IsAnimPlaying&1;
		uint32 t2=pModelSkeleton->IsHuman();
		if (t0 && t1 && t2)
		{		
			Command::PoseModifier* ac = m_buffer.CreateCommand<Command::PoseModifier>();
			ac->m_TargetBuffer = Command::TargetBuffer;
			ac->m_pPoseModifier = m_pSkeletonPose->m_feetLock.Store();
			ac->m_PhysLocation = rPhysLocationCurr;
			ac->m_AnimLocation = QuatT(rAnimLocationCurr);
		}
	}

	//-------------------------------------------------------------------------------------------
	//---              push layers into command-buffer                                        ---
	//-------------------------------------------------------------------------------------------

	uint16 mask=0xffff; 
	uint32 numJoints = pSkeletonAnim->m_arrJointMask.size();
	for (uint32 i=0; i<numJoints; i++)
		mask &= pSkeletonAnim->m_arrJointMask[i]; 


	//	float fColor2[4] = {1,0,1,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor2, false,"mask: %d",mask);
	//	g_YLine+=10;


	//float fColor2[4] = {1,0,1,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor2, false,"AimIK Layer: %d",nVLayerPre);
	//g_YLine+=10;

	for(uint32 nVLayerNo=1; nVLayerNo<nVLayerPre; nVLayerNo++)
	{
		DynArray<CAnimation> &layer = pSkeletonAnim->m_arrLayer_AFIFO[nVLayerNo];
		uint32 animCount = layer.size();
		uint32 activeCount=0;
		for (uint32 a=0; a<animCount; a++)
		{
			if (layer[a].m_bActivated==0)
				break;
			activeCount++;
		}
		for (uint32 i=0; i<activeCount; i++)
		{
			SpuStackValue<CAnimation, false,true> spuAnim(layer[i]);
			const CAnimation &rAnim = spuAnim;
			LPlayback( rAnim, Command::TargetBuffer,nVLayerNo,mask);
		}
	}	


	if (m_pSkeletonPose->m_AimIK().m_UseAimIK || m_pSkeletonPose->m_AimIK().m_AimIKBlend)
	{
#if (NEWAIMSYSTEM==0)
		CreateAimPoseQueue_Old(m_pInstance,	numActiveAnims, nVLayerPre);
#endif
#if (NEWAIMSYSTEM==1)
		CreateAimPoseQueue_New(m_pInstance,	numActiveAnims, nVLayerPre);
#endif
		Command::PoseModifier* ac = m_buffer.CreateCommand<Command::PoseModifier>();
		ac->m_TargetBuffer = Command::TargetBuffer;
		ac->m_pPoseModifier = m_pSkeletonPose->m_PoseBlenderAim.get();
		ac->m_PhysLocation = rPhysLocationCurr;
		ac->m_AnimLocation = QuatT(rAnimLocationCurr);
	}

	for(uint32 nVLayerNo=nVLayerPre; nVLayerNo<numVIRTUALLAYERS; nVLayerNo++)
	{
		DynArray<CAnimation> &layer = pSkeletonAnim->m_arrLayer_AFIFO[nVLayerNo];
		uint32 animCount = layer.size();
		uint32 activeCount=0;
		for (uint32 a=0; a<animCount; a++)
		{
			if (layer[a].m_bActivated==0)
				break;
			activeCount++;
		}
		for (uint32 i=0; i<activeCount; i++)
		{
			SpuStackValue<CAnimation, false,true> spuAnim( layer[i] );
			const CAnimation &rAnim = spuAnim;
			if (nVLayerNo==nVLayerPre)
			{
				CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
				const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(rAnim.m_Parametric.m_nAnimID[0]) );
				if (pAnim->m_nAssetType==AIM_File)
				{
					//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor2, false,"Aim in AimIK Layer");
					//g_YLine+=10;
					continue;
				}
			}
			LPlayback( rAnim, Command::TargetBuffer,nVLayerNo,mask);
		}
	}	

//	if ( numActiveAnims && hasLayerAnimation)
	if ( numActiveAnims )
	{
		uint32 t0 = uint32(Console::GetInst().ca_LockFeetWithIK);
		uint32 t1 = pSkeletonAnim->m_IsAnimPlaying;
		uint32 t3 = pModelSkeleton->IsHuman();
		if (t0 && t1 && t3)
		{
			Command::PoseModifier* ac = m_buffer.CreateCommand<Command::PoseModifier>();
			ac->m_TargetBuffer = Command::TargetBuffer;
			ac->m_pPoseModifier = m_pSkeletonPose->m_feetLock.Restore();
			ac->m_PhysLocation = rPhysLocationCurr;
			ac->m_AnimLocation = QuatT(rAnimLocationCurr);
		}
	}

	uint32 t3 = pModelSkeleton->IsHuman();
	if (t3) //important or it will crash with CGAs
	{
		if (m_pSkeletonPose->m_LookIK().m_UseLookIK || m_pSkeletonPose->m_LookIK().m_LookIKBlend)
		{
			Command::PoseModifier* ac = m_buffer.CreateCommand<Command::PoseModifier>();
			ac->m_TargetBuffer = Command::TargetBuffer;
			ac->m_pPoseModifier = m_pSkeletonPose->m_LookAt.get();
			ac->m_PhysLocation = rPhysLocationCurr;
			ac->m_AnimLocation = QuatT(rAnimLocationCurr);
		}
	}
}

SPU_NO_INLINE void CommandBuffer::Execute()
{	
	m_buffer.Execute();
}

////////////////////////////////////////////////////////////////////////////
//                    playback of one single animation                    //
////////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE void CommandBuffer::BasePlayback(const CAnimation& rAnim, f32& fRootTransition)
{	
	const SParametric &curGroup =  rAnim.m_Parametric;

	if (rAnim.m_AnimParams.m_nFlags & CA_FULL_ROOT_PRIORITY)
	{
		if (rAnim.m_fTransitionWeight)
		{
			fRootTransition=rAnim.m_fTransitionWeight;
		//	float fColor1[4] = {1,0,0,1};
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor1, false,"fRootTransition: %f",fRootTransition);
		//	g_YLine+=10;

			Command::ClearSingle* clearbuffer = m_buffer.CreateCommand<Command::ClearSingle>();
			clearbuffer->m_TargetBuffer			= Command::TargetBuffer;
		}
	}

	if (curGroup.m_nParametricID<0)
	{
		//regular asset
		CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;

		uint32 numAnims = pAnimationSet->GetAnimationCount();
		uint32 nAnimID=rAnim.m_Parametric.m_nAnimID[0];


		const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(rAnim.m_Parametric.m_nAnimID[0]);
		assert(pMAG);
		int32 nEGlobalID = pMAG->m_nGlobalAnimId;
		if (pMAG->m_nAssetType==CAF_File)
		{
			// use a stack object on SPU here to reduce simulation time
			SpuStackValue<GlobalAnimationHeaderCAF, true, true> stackAnimHeader( g_AnimationManager.m_arrGlobalCAF[nEGlobalID] );
			GlobalAnimationHeaderCAF& rGlobalAnimHeader = stackAnimHeader; 


			Command::SampleAddAnimFull* ac = m_buffer.CreateCommand<Command::SampleAddAnimFull>();
			ac->m_nEAnimID					=	rAnim.m_Parametric.m_nAnimID[0];
			ac->m_flags             = 0;
			ac->m_flags			       |=	m_pSkeletonAnim->m_AnimationDrivenMotion ? Command::SampleAddAnimFull::Flag_ADMotion : 0;
			ac->m_flags					   |=	(rAnim.m_bEndOfCycle && (rAnim.m_Parametric.m_nSegmentCounter[0]==0)) ? Command::SampleAddAnimFull::Flag_AnimEOC : 0;		//if 1 then "end of cycle" reached
			ac->m_flags		         |= ((rAnim.m_nAnimAimID0>=0)+(rAnim.m_nAnimAimID1>=0))<<6;
			//	ac->m_flags		         |= (rAnim.m_AnimParams.m_nFlags&CA_FULL_ROOT_PRIORITY) ? Command::SampleAddAnimFull::Flag_RootPriority : 0;
			ac->m_fWeight						=	rAnim.m_fTransitionWeight;

			f32 time_new0	=	rAnim.m_fAnimTime; 
			f32 time_old0	=	rAnim.m_fAnimTimePrev;
			int32 segtotal	= rGlobalAnimHeader.m_Segments - 1;
			int32 segcount 	= rAnim.m_Parametric.m_nSegmentCounter[0];
			f32 segdur 			= rGlobalAnimHeader.GetSegmentDuration(segcount);
			f32 totdur 			= rGlobalAnimHeader.m_fTotalDuration ? rGlobalAnimHeader.m_fTotalDuration : 1.0f;
			f32 segbase 		= rGlobalAnimHeader.m_SegmentsTime[segcount];
			f32 percent 		= segdur/totdur;
			f32 time_old	=	time_old0*percent+segbase;	
			f32 time_new	=	time_new0*percent+segbase;	

			assert(time_new>=0.0f && time_new<=1.0f);
			if (rAnim.m_bEndOfCycle && segtotal)
			{
				int32 _segcount 	= rAnim.m_Parametric.m_nSegmentCounter[0]-1;
				if (_segcount<0)
					_segcount=segtotal;
				f32 _segdur 		= rGlobalAnimHeader.GetSegmentDuration(_segcount);
				f32 _totdur 		= rGlobalAnimHeader.m_fTotalDuration ? rGlobalAnimHeader.m_fTotalDuration : 1.0f;
				f32 _segbase 		= rGlobalAnimHeader.m_SegmentsTime[_segcount];
				f32 _percent 		= _segdur/_totdur;
				time_old	=	time_old0*_percent+_segbase;	
			}

			ac->m_fETimeNew					=	time_new;//rAnim.m_fAnimTime;			//this is a percentage value between 0-1
			ac->m_fETimeOld					=	time_old;//rAnim.m_fAnimTimePrev; //this is a percentage value between 0-1
			if (m_pInstance->m_fDeltaTime>=0 && (ac->m_flags & Command::SampleAddAnimFull::Flag_AnimEOC) )
				ac->m_fETimeOld					=	time_old0;//rAnim.m_fAnimTimePrev; //this is a percentage value between 0-1
		}

		if (pMAG->m_nAssetType==AIM_File)
		{
			// use a stack object on SPU here to reduce simulation time
			SpuStackValue<GlobalAnimationHeaderAIM, true, true> stackAnimHeader( g_AnimationManager.m_arrGlobalAIM[nEGlobalID] );
			GlobalAnimationHeaderAIM& rGlobalAnimHeader = stackAnimHeader; 
			assert(rAnim.m_fAnimTime>=0.0f && rAnim.m_fAnimTime<=1.0f);
			Command::SampleAddPoseFull* ac = m_buffer.CreateCommand<Command::SampleAddPoseFull>();
			ac->m_nEAnimID					=	rAnim.m_Parametric.m_nAnimID[0];
			ac->m_flags             = 0;
			ac->m_fWeight						=	rAnim.m_fTransitionWeight;
			ac->m_fETimeNew					=	rAnim.m_fAnimTime; //this is a percentage value between 0-1
		}

	}
	else 
	{
		BaseEvaluationLMG( rAnim, Command::TargetBuffer );
	}
	m_pSkeletonAnim->m_IsAnimPlaying = 1;

}

///////////////////////////////////////////////////////////////////////////////////////////
//                       evaluation of a locomotion group                                //
///////////////////////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE void CommandBuffer::BaseEvaluationLMG( const CAnimation& rAnim, uint32 nTargetBufferaa )
{	
	const SParametric &rCurLocoGroup = rAnim.m_Parametric;
	assert(rCurLocoGroup.m_nParametricID>=0);
	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;

	uint32 numAnims = pAnimationSet->GetAnimationCount();
	uint32 nAnimID=rCurLocoGroup.m_nParametricID;

	const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(rCurLocoGroup.m_nParametricID);
	if (pAnim==0)
		return;
	assert(pAnim->m_nAssetType == LMG_File || pAnim->m_nAssetType == PMG_File);

	GlobalAnimationHeaderLMG& rGlobalAnimHeaderLMG = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];
	uint32 numSpliceAnims = rGlobalAnimHeaderLMG.m_strSpliceAnim.size();
	uint32 nTargetBuffer  = (numSpliceAnims) ? Command::TmpBuffer : Command::TargetBuffer;

	f32 fTransitionWeight=1.0f;
	if (nTargetBuffer==Command::TargetBuffer)
		fTransitionWeight=rAnim.m_fTransitionWeight;

	//-----------------------------------------------------------------------------------
	//-----              average all animations in a blend-space                   ------
	//-----------------------------------------------------------------------------------
	int8 idf[MAX_LMG_ANIMS];	memset( &idf,1,MAX_LMG_ANIMS*sizeof(int8));
	int32 ids[MAX_LMG_ANIMS];

#if !defined(__SPU__)
	f32 fTotalSum=0;
	for (int32 i=0; i<rCurLocoGroup.m_numAnims; i++)
		fTotalSum += rCurLocoGroup.m_fBlendWeight[i];
	if ( fabsf(fTotalSum-1.0f)>0.09f )
		AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"CryAnimation: LMG weights don't sum up to 1. fTotalSum: %f",fTotalSum);
#endif

	uint32 f=0;
	for (int32 i=0; i<rCurLocoGroup.m_numAnims; i++)
	{
		f32 weight = rCurLocoGroup.m_fBlendWeight[i];
		uint32 nID = rCurLocoGroup.m_nAnimID[i];
		int32 nGlobalID=pAnimationSet->GetGlobalIDByAnimID_Fast(rCurLocoGroup.m_nAnimID[i]);

		GlobalAnimationHeaderCAF& rGlobalAnimHeaderAssets = g_AnimationManager.m_arrGlobalCAF[nGlobalID];
		rGlobalAnimHeaderAssets.m_nTouchedCounter++;
		if (weight && idf[i])
		{
			idf[i]=0;
			ids[f]=i;
			f++;
			int32 id0=rCurLocoGroup.m_nAnimID[i];
			for (int32 q=i+1; q<rCurLocoGroup.m_numAnims; q++)
			{
				int32 id1=rCurLocoGroup.m_nAnimID[q];
				if (id0==id1 && rCurLocoGroup.m_fBlendWeight[q])
				{
					idf[q]=0;
					ids[f]=q;
					f++;
				}
			}
		}
	}

#if !defined(__SPU__)
	if (f==0)
		AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"CryAnimation: LMG asset sorting problem: f=%d",f);
	assert (f);
#endif


	int arrAnimIndices[MAX_LMG_ANIMS];
	f32 arrAnimWeights[MAX_LMG_ANIMS];
	int arrExampleID[MAX_LMG_ANIMS];

	int32 oldid =	-1; //rCurLocoGroup.m_nAnimID[i];
	uint32 c=0;
	for (uint32 s=0; s<f; s++)
	{
		int i	=	ids[s];
		f32 weight  = rCurLocoGroup.m_fBlendWeight[i];
		int newid		= rCurLocoGroup.m_nAnimID[i];
		//	float fColor1[4] = {1,0,0,1};
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor1, false,"example %d used weight:%f",newid, weight);
		//	g_YLine+=10;

		if (newid != oldid)
		{
			arrAnimIndices[c]=newid;
			arrAnimWeights[c]=weight;
			arrExampleID[c]=i;
			c++;
			oldid=newid;
			continue;
		}
		if (c > 0) arrAnimWeights[c-1]+=weight;
	}


	if (nTargetBuffer==Command::TmpBuffer)
	{
		Command::ClearFull* clearbuffer = m_buffer.CreateCommand<Command::ClearFull>();
		clearbuffer->m_TargetBuffer		= Command::TmpBuffer;
	}

	if (pAnim->m_nAssetType == LMG_File)
	{
		if (rGlobalAnimHeaderLMG.m_jointList.empty()==0)
		{
			Command::JointMask* pCommand = m_buffer.CreateCommand<Command::JointMask>();
			pCommand->m_pMask =  &rGlobalAnimHeaderLMG.m_jointList[0];
			pCommand->m_count = rGlobalAnimHeaderLMG.m_jointList.size();
		}
	} 

	//-------------------------------------------------------------------------------------
	//----------            evaluate and blend root-joint       ---------------------------
	//-------------------------------------------------------------------------------------
	f32 fBlendWeights  =	0; //rCurLocoGroup.m_fBlendWeight[i];
	for (uint32 s=0; s<c; s++)
	{
		int example = arrExampleID[s];
		f32 weight = arrAnimWeights[s];

		//	int id		 = arrAnimIndices[s];
		//	float fColor2[4] = {0,1,0,1};
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor2, false,"example %d used weight:%f",id, weight);
		//	g_YLine+=10;

		const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(rAnim.m_Parametric.m_nAnimID[example]);
		assert(pMAG);
		int32 nEGlobalID = pMAG->m_nGlobalAnimId;
		assert(pMAG->m_nAssetType==CAF_File);
		SpuStackValue<GlobalAnimationHeaderCAF, true, true> stackAnimHeader( g_AnimationManager.m_arrGlobalCAF[nEGlobalID] );
		GlobalAnimationHeaderCAF& rGlobalAnimHeader = stackAnimHeader; 

		Command::SampleAddAnimFull* fetch = m_buffer.CreateCommand<Command::SampleAddAnimFull>();
		fetch->m_nEAnimID			=	rAnim.m_Parametric.m_nAnimID[example];
		fetch->m_fWeight      = weight*fTransitionWeight;
		fetch->m_flags=0;
		fetch->m_flags		   |=	m_pSkeletonAnim->m_AnimationDrivenMotion ? Command::SampleAddAnimFull::Flag_ADMotion : 0;
		fetch->m_flags		   |=	(rAnim.m_bEndOfCycle && (rAnim.m_Parametric.m_nSegmentCounter[example]==0)) ? Command::SampleAddAnimFull::Flag_AnimEOC : 0;		//if 1 then "end of cycle" reached
		fetch->m_flags			 |=	(pAnim->m_nAssetType==PMG_File) ? Command::SampleAddAnimFull::Flag_IsPMG : 0;
		fetch->m_flags		   |=	(nTargetBuffer==Command::TmpBuffer) ? Command::SampleAddAnimFull::Flag_TmpBuffer : 0;
		fetch->m_flags	     |= ((rAnim.m_nAnimAimID0>=0)+(rAnim.m_nAnimAimID1>=0))<<6;

		f32 time_new0	=	rAnim.m_fAnimTime; 
		f32 time_old0	=	rAnim.m_fAnimTimePrev;

		int32 segtotal	= rGlobalAnimHeader.m_Segments - 1;
		int32 segcount 	= rAnim.m_Parametric.m_nSegmentCounter[example];
		f32 segdur 			= rGlobalAnimHeader.GetSegmentDuration(segcount);
		f32 totdur 			= rGlobalAnimHeader.m_fTotalDuration ? rGlobalAnimHeader.m_fTotalDuration : 1.0f;
		f32 segbase 		= rGlobalAnimHeader.m_SegmentsTime[segcount];
		f32 percent 		= segdur/totdur;
		f32 time_old	=	time_old0*percent+segbase;	
		f32 time_new	=	time_new0*percent+segbase;	

		assert(time_new>=0.0f && time_new<=1.0f);
		if (rAnim.m_bEndOfCycle && segtotal)
		{
			int32 _segcount 	= rAnim.m_Parametric.m_nSegmentCounter[example]-1;
			if (_segcount<0)
				_segcount=segtotal;
			f32 _segdur 		= rGlobalAnimHeader.GetSegmentDuration(_segcount);
			f32 _totdur 		= rGlobalAnimHeader.m_fTotalDuration ? rGlobalAnimHeader.m_fTotalDuration : 1.0f;;
			f32 _segbase 		= rGlobalAnimHeader.m_SegmentsTime[_segcount];
			f32 _percent 		= _segdur/_totdur;
			time_old	=	time_old0*_percent+_segbase;	
		}

		fetch->m_fETimeNew				=	time_new;//rAnim.m_fAnimTime;			//this is a percentage value between 0-1
		fetch->m_fETimeOld				=	time_old;//rAnim.m_fAnimTimePrev; //this is a percentage value between 0-1
		if (m_pInstance->m_fDeltaTime>=0 && (fetch->m_flags & Command::SampleAddAnimFull::Flag_AnimEOC) )
			fetch->m_fETimeOld			=	time_old0;//rAnim.m_fAnimTimePrev; //this is a percentage value between 0-1

		fBlendWeights+=weight;
	}

	if (pAnim->m_nAssetType == LMG_File)
	{
		if (!rGlobalAnimHeaderLMG.m_jointList.empty())
		{
			Command::JointMask* pCommand = m_buffer.CreateCommand<Command::JointMask>();
			pCommand->m_pMask =  NULL;
			pCommand->m_count = 0;
		}
	}
	m_pSkeletonAnim->m_IsAnimPlaying |= 1;

#if !defined(__SPU__)
	assert( fabsf(fBlendWeights-1.0f)<0.05f );
#endif // !__SPU__

	/*
	Vec3 Velocity = Matrix33::CreateRotationZ(result.m_fCurrentStreife) * Vec3(0,result.m_fCurrentSpeed,0);
	Vec3 m_vCurrentVelocity	=	Velocity;
	f32  m_fRelRotation			= result.m_fCurrentTurn*m_pInstance->m_fDeltaTime;
	f32  m_fRelRotation2		= result.m_fRelRotation;
	Vec3 m_vRelTranslation	=	-Velocity*m_pInstance->m_fDeltaTime;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor2, false,"Velocity: %f %f %f   speed: %f  turn: %f   travel: %f",Velocity.x,Velocity.y,Velocity.z,result.m_fCurrentSpeed,result.m_fCurrentTurn,result.m_fCurrentStreife);
	g_YLine+=10;
	*/

	//-----------------------------------------------------------------------------------------------------
	if (nTargetBuffer==Command::TmpBuffer)
	{
		Command::NormalizeFull* norm = m_buffer.CreateCommand<Command::NormalizeFull>();
		norm->m_TargetBuffer		= nTargetBuffer;
		norm->m_fRootScale			= 1.0f;

		for (uint32 i=0; i<numSpliceAnims; i++)
		{
			const char* strSpliceAnim = rGlobalAnimHeaderLMG.m_strSpliceAnim[i];
			int32 nAnimIDNS	= pAnimationSet->CAnimationSet::GetAnimIDByName(strSpliceAnim);
			if (nAnimIDNS>=0)
			{ 
				const ModelAnimationHeader* pAnimHeaderNS = pAnimationSet->GetModelAnimationHeader(nAnimIDNS);
				assert(pAnimHeaderNS->m_nAssetType==CAF_File);
				if (pAnimHeaderNS->m_nAssetType==CAF_File)
				{
					Command::SampleAnimPart* ac = m_buffer.CreateCommand<Command::SampleAnimPart>();
					ac->m_TargetBuffer				= nTargetBuffer;
					ac->m_nMask								= 0;
					ac->m_nFeatherBlend				= 0;
					ac->m_nEAnimID						=	nAnimIDNS;
					ac->m_fAnimTime						=	rAnim.m_fAnimTime;			//this is a percentage value between 0-1
					ac->m_fWeight 						= 1.0f;
				}
				if (pAnimHeaderNS->m_nAssetType==AIM_File)
				{
					Command::SamplePosePart* ac = m_buffer.CreateCommand<Command::SamplePosePart>();
					ac->m_TargetBuffer				= nTargetBuffer;
					ac->m_nEAnimID						=	nAnimIDNS;
					ac->m_fAnimTime						=	rAnim.m_fAnimTime;			//this is a percentage value between 0-1
					ac->m_fWeight 						= 1.0f;
				}
			}
		}

		Command::CopyAddAnimFull* copy_add = m_buffer.CreateCommand<Command::CopyAddAnimFull>();
		copy_add->m_SourceBuffer		= Command::TmpBuffer; 
		copy_add->m_TargetBuffer		= Command::TargetBuffer;
		copy_add->m_fWeight					= rAnim.m_fTransitionWeight;
		copy_add->m_IsPMG			      =	(pAnim->m_nAssetType==PMG_File) ? Command::SampleAddAnimFull::Flag_IsPMG : 0;
	}
}

////////////////////////////////////////////////////////////////////////////
//                    playback of one single animation                    //
////////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE void CommandBuffer::LPlayback( const CAnimation& rAnim, uint32 nTargetBuffer, uint32 nVLayer, uint32 mask )
{
	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
	const SParametric &rCurGroup = rAnim.m_Parametric;
	if (rCurGroup.m_nParametricID<0)
	{
		int32 nAnimID = rAnim.m_Parametric.m_nAnimID[0];
		const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(nAnimID);
		if (pAnim->m_nAssetType==CAF_File)
		{
			if (mask==0xffff)
			{
				Command::SampleAnimPart* ac = m_buffer.CreateCommand<Command::SampleAnimPart>();
				ac->m_TargetBuffer				= nTargetBuffer;
				ac->m_nMask								= 255;
				ac->m_nFeatherBlend				= 0;
				ac->m_nEAnimID						=	rAnim.m_Parametric.m_nAnimID[0];
				ac->m_fAnimTime						=	rAnim.m_fAnimTime;					//this is a percentage value between 0-1
				f32 w0 = rAnim.m_fTransitionWeight;											//this is a percentage value between 0-1
				f32 w1 = m_pSkeletonAnim->m_arrAdditiveWeights[nVLayer];
				f32 w2 = m_pSkeletonAnim->m_fAllowMultilayerAnim;
				f32 w3 = m_pSkeletonAnim->m_arrLayerBlending[nVLayer];
				f32 w4 = m_pSkeletonAnim->m_arrLayerBlendingMult[nVLayer];
				ac->m_fWeight							=	w0*w1*w2*w3*w4;								//this is a percentage value between 0-1
			} 
			else 
			{
				Command::SampleFeatherAnimPart* ac = m_buffer.CreateCommand<Command::SampleFeatherAnimPart>();
				ac->m_TargetBuffer				= nTargetBuffer;
				ac->m_nMask								= 255;
				ac->m_nFeatherBlend				= 1;
				ac->m_nEAnimID						=	rAnim.m_Parametric.m_nAnimID[0];
				ac->m_fAnimTime						=	rAnim.m_fAnimTime;			//this is a percentage value between 0-1
				f32 w0 = rAnim.m_fTransitionWeight;			
				f32 w1 = m_pSkeletonAnim->m_arrAdditiveWeights[nVLayer];
				f32 w2 = m_pSkeletonAnim->m_fAllowMultilayerAnim;
				f32 w3 = m_pSkeletonAnim->m_arrLayerBlending[nVLayer];
				f32 w4 = m_pSkeletonAnim->m_arrLayerBlendingMult[nVLayer];
				ac->m_fWeight							=	w0*w1*w2*w3*w4;			//this is a percentage value between 0-1

				uint32 numJoints = m_pSkeletonAnim->m_arrJointMask.size();
				for (uint32 i=0; i<numJoints; i++)
					ac->m_arrFeatherMask[i] = ((m_pSkeletonAnim->m_arrJointMask[i]>>nVLayer)&1)<<7; //temporary	
			}
		}

		if (pAnim->m_nAssetType==AIM_File)
		{
			Command::SamplePosePart* ac = m_buffer.CreateCommand<Command::SamplePosePart>();
			ac->m_TargetBuffer				= nTargetBuffer;
			ac->m_nEAnimID						=	rAnim.m_Parametric.m_nAnimID[0];
			ac->m_fAnimTime						=	rAnim.m_fAnimTime;					//this is a percentage value between 0-1
			f32 w0 = rAnim.m_fTransitionWeight;											//this is a percentage value between 0-1
			f32 w1 = m_pSkeletonAnim->m_arrAdditiveWeights[nVLayer];
			f32 w2 = m_pSkeletonAnim->m_fAllowMultilayerAnim;
			f32 w3 = m_pSkeletonAnim->m_arrLayerBlending[nVLayer];
			f32 w4 = m_pSkeletonAnim->m_arrLayerBlendingMult[nVLayer];
			ac->m_fWeight							=	w0*w1*w2*w3*w4;								//this is a percentage value between 0-1
		}
		m_pSkeletonAnim->m_IsAnimPlaying |= 0xfffe;
	}

}











//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------
SPU_NO_INLINE void CommandBuffer::CreateAimPoseQueue_Old(CCharInstance* pInstance, uint32 numAnimsL0, uint32 qqnAimIKLayer)
{
	CAnimationSet* pAnimationSet = &pInstance->m_pModel->m_AnimationSet;
	CSkeletonPose* pSkeletonPose = &pInstance->m_SkeletonPose;
	CSkeletonAnim* pSkeletonAnim = &pInstance->m_SkeletonAnim;

	SAimIK& rAimIK = pSkeletonPose->m_AimIK();

	rAimIK.m_numActiveAimPoses=0;
	rAimIK.m_nLastValidAimPose=-1;
	for (uint32 i=0; i<MAX_EXEC_QUEUE*2; i++)
	{
		rAimIK.m_AimInfo[i].m_fWeight				= 0; 
		rAimIK.m_AimInfo[i].m_numAimPoses		= 0; 
		rAimIK.m_AimInfo[i].m_nGlobalAimID0	= -1; 
		rAimIK.m_AimInfo[i].m_nGlobalAimID1	= -1;
		rAimIK.m_AimInfo[i].m_fAnimTime			= 0.0f;
	}

	//-----------------------------------------------------------------------------------
	//-----    take all Aim-Poses attached to the animation name in the CAL-file    -----
	//-----------------------------------------------------------------------------------
	rAimIK.m_numActiveAimPoses = numAnimsL0;
	for (uint32 i=0; i<numAnimsL0; i++)
	{
		CAnimation 	&rCurAnimation = pSkeletonAnim->m_arrLayer_AFIFO[0][i];
		SAimInfo		&rCurAimInfo = rAimIK.m_AimInfo[i];

		assert(pSkeletonAnim->m_arrLayer_AFIFO[0][0].m_bActivated);

		rCurAimInfo.m_numAimPoses	= 0;
		rCurAimInfo.m_fWeight		= rCurAnimation.m_fTransitionWeight;
		rCurAimInfo.m_fAnimTime	= rCurAnimation.m_fAnimTime;


		//check the first aim-id
		if (rCurAnimation.m_nAnimAimID0<0)
			continue;
		const ModelAnimationHeader* pAnimHeader = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(rCurAnimation.m_nAnimAimID0) );
		if (pAnimHeader==0)
			continue;
		if (pAnimHeader->m_nAssetType!=AIM_File)
			continue;
		assert(pAnimHeader->m_nGlobalAnimId>-1);
		GlobalAnimationHeaderAIM& rGAH0 = g_AnimationManager.m_arrGlobalAIM[pAnimHeader->m_nGlobalAnimId];
		uint32 numPoses = rGAH0.m_arrAimIKPosesAIM.size();
		if (numPoses==0)
			continue;
		rCurAimInfo.m_nGlobalAimID0	= pAnimHeader->m_nGlobalAnimId; 
		rCurAimInfo.m_numAimPoses++;	


		//check the second aim-id
		if (rCurAnimation.m_nAnimAimID1<0)
			continue;
		pAnimHeader = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(rCurAnimation.m_nAnimAimID1) );
		if (pAnimHeader==0)
			continue;
		if (pAnimHeader->m_nAssetType!=AIM_File)
			continue;
		assert(pAnimHeader->m_nGlobalAnimId>-1);
		GlobalAnimationHeaderAIM& rGAH1 = g_AnimationManager.m_arrGlobalAIM[pAnimHeader->m_nGlobalAnimId];
		numPoses = rGAH1.m_arrAimIKPosesAIM.size();
		if (numPoses==0)
			continue;
		rCurAimInfo.m_nGlobalAimID1	= pAnimHeader->m_nGlobalAnimId; 
		rCurAimInfo.m_numAimPoses++;
	}


}












#if (NEWAIMSYSTEM==1)
//---------------------------------------------------------------------------------------------------------
//-----      IvoH: not a good idea to modify values in SkeletonPose directly                 --------------
//-----            this should go through the command-buffer                                 --------------
//---------------------------------------------------------------------------------------------------------
SPU_NO_INLINE void CommandBuffer::CreateAimPoseQueue_New(CCharInstance* pInstance, uint32 numAnimsL0, uint32 nAimIKLayer)
{
	CAnimationSet* pAnimationSet = &pInstance->m_pModel->m_AnimationSet;
	CSkeletonPose* pSkeletonPose = &pInstance->m_SkeletonPose;
	CSkeletonAnim* pSkeletonAnim = &pInstance->m_SkeletonAnim;

	f32 fAimIKLayerWeight = 0;
	if (nAimIKLayer<numVIRTUALLAYERS)
		fAimIKLayerWeight = pSkeletonAnim->m_arrLayerBlending[nAimIKLayer];

	f32 t0=1.0f-fAimIKLayerWeight;
	f32 t1=fAimIKLayerWeight;

	//float fTextColor[4] = {1,0,0,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"CreateTransitionInformation: %d  fAimIKLayerWeight: %f",nAimIKLayer,fAimIKLayerWeight );	
	//g_YLine+=16.0f;

	SAimIK& rAimIK = pSkeletonPose->m_AimIK();

	rAimIK.m_numActiveAimPoses=0;
	rAimIK.m_nLastValidAimPose=-1;
	for (uint32 i=0; i<MAX_EXEC_QUEUE*2; i++)
	{
		rAimIK.m_AimInfo[i].m_fWeight				= 0; 
		rAimIK.m_AimInfo[i].m_numAimPoses		= 0; 
		rAimIK.m_AimInfo[i].m_nGlobalAimID0	= -1; 
		rAimIK.m_AimInfo[i].m_nGlobalAimID1	= -1;
		rAimIK.m_AimInfo[i].m_fAnimTime			= 0.0f;
	}


	//-----------------------------------------------------------------------------------
	//-----    take all Aim-Poses attached to the animation name in the CAL-file    -----
	//-----------------------------------------------------------------------------------
	rAimIK.m_numActiveAimPoses = 0;
	for (uint32 i=0; i<numAnimsL0; i++)
	{
		CAnimation 	&rCurAnimation = pSkeletonAnim->m_arrLayer_AFIFO[0][i];
		assert(rCurAnimation.m_bActivated);

		//check the first aim-id
		if (rCurAnimation.m_nAnimAimID0<0)
			continue;
		const ModelAnimationHeader* pAnimHeader = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(rCurAnimation.m_nAnimAimID0) );
		if (pAnimHeader==0)
			continue;
		if (pAnimHeader->m_nAssetType!=AIM_File)
			continue;
		assert(pAnimHeader->m_nGlobalAnimId>-1);
		GlobalAnimationHeaderAIM& rGAH0 = g_AnimationManager.m_arrGlobalAIM[pAnimHeader->m_nGlobalAnimId];
		uint32 numPoses = rGAH0.m_arrAimIKPosesAIM.size();
		if (numPoses==0)
			continue;
		if (rGAH0.m_nExist==0)
			continue;

		SAimInfo		&rCurAimInfo = rAimIK.m_AimInfo[rAimIK.m_numActiveAimPoses];
		rCurAimInfo.m_numAimPoses	= 0;
		rCurAimInfo.m_fWeight		= rCurAnimation.m_fTransitionWeight*t0;
		rCurAimInfo.m_fAnimTime	= rCurAnimation.m_fAnimTime;
		rCurAimInfo.m_nGlobalAimID0	= pAnimHeader->m_nGlobalAnimId; 
		rCurAimInfo.m_numAimPoses++;	

		rAimIK.m_numActiveAimPoses++;

		//check the second aim-id
		if (rCurAnimation.m_nAnimAimID1<0)
			continue;
		pAnimHeader = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(rCurAnimation.m_nAnimAimID1) );
		if (pAnimHeader==0)
			continue;
		if (pAnimHeader->m_nAssetType!=AIM_File)
			continue;
		assert(pAnimHeader->m_nGlobalAnimId>-1);
		GlobalAnimationHeaderAIM& rGAH1 = g_AnimationManager.m_arrGlobalAIM[pAnimHeader->m_nGlobalAnimId];
		numPoses = rGAH1.m_arrAimIKPosesAIM.size();
		if (numPoses==0)
			continue;
		if (rGAH1.m_nExist==0)
			continue;
		rCurAimInfo.m_nGlobalAimID1	= pAnimHeader->m_nGlobalAnimId; 
		rCurAimInfo.m_numAimPoses++;
	}

	//-----------------------------------------------------------------------------------
	//-----    check if there are aim-poses in the same layer like the AIM-IK       -----
	//-----------------------------------------------------------------------------------
	if (nAimIKLayer<1 || nAimIKLayer>=numVIRTUALLAYERS)
		return;

	DynArray<CAnimation>& rCurLayer = pSkeletonAnim->m_arrLayer_AFIFO[nAimIKLayer];
	uint32 numAnimsInLayer = rCurLayer.size();
	uint32 numActiveAnims=0;
	for (uint32 a=0; a<numAnimsInLayer; a++)
	{
		if (rCurLayer[a].m_bActivated==0)
			break;
		numActiveAnims++;
	}

	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"fAimIKLayerWeight: %f    numActiveAnims: %d",fAimIKLayerWeight,numActiveAnims);	
	//g_YLine+=16.0f;
	for (uint32 i=0; i<numActiveAnims; i++)
	{
		DynArray<CAnimation>& rCurLayer = pSkeletonAnim->m_arrLayer_AFIFO[nAimIKLayer];
		int32 nAnimID = rCurLayer[i].m_Parametric.m_nAnimID[0];
		assert(rCurLayer[i].m_bActivated);
		const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimID) );
		assert(pAnim);
		assert(pAnim->m_nGlobalAnimId>-1);
		if (pAnim->m_nAssetType==AIM_File)
		{
			GlobalAnimationHeaderAIM& rGAH = g_AnimationManager.m_arrGlobalAIM[pAnim->m_nGlobalAnimId];
			uint32 numPoses = rGAH.m_arrAimIKPosesAIM.size();
			if (numPoses==0)
				continue;
			if (rGAH.m_nExist==0)
				continue;

			SAimInfo	&rCurAimInfo = rAimIK.m_AimInfo[rAimIK.m_numActiveAimPoses];
			rCurAimInfo.m_numAimPoses		= 1;
			rCurAimInfo.m_fWeight				= rCurLayer[i].m_fTransitionWeight*t1;
			rCurAimInfo.m_fAnimTime			= 0;
			rCurAimInfo.m_nGlobalAimID0	= pAnim->m_nGlobalAnimId; 

		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fTextColor, false,"Aim in AimIK Layer: %d  weight: %f",nAimIKLayer,rCurLayer[i].m_fTransitionWeight);
		//	g_YLine+=10;
			rAimIK.m_numActiveAimPoses++;
			continue;
		}
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"Normal Animation in Aim-IK");	
	//	g_YLine+=16.0f;
	}

}
#endif
