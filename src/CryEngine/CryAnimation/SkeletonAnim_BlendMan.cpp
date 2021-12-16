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
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include <IVisualLog.h>
#include "CharacterInstance.h"
#include "Model.h"
#include "ModelSkeleton.h"
#include "CharacterManager.h"
#include <float.h>
#include "LMG.h"

// Define this flag if you want to debug PMG
#define VISUALIZE_PMG_SPACE
#if defined __SPU__
#undef VISUALIZE_PMG_SPACE
#endif

// Function used to display animation flags as text
const char * AnimFlags( uint32 flags )
{
	static char result[33];
	const char codes[] = "+ybsWVFxUnIAKTRLM";
	for ( int i = 0; result[i] = codes[i]; ++i )
		if ( (flags & (1<<(sizeof(codes)-i-2))) == 0 )
			result[i] = '-';
	return result;
}



/////////////////////////////////////////////////////////////////////////////////
// this function is handling animation-update, interpolation and transitions   //
/////////////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE uint32 CSkeletonAnim::BlendManager(	f32 fDeltaTime,	DynArray<CAnimation>& arrAFIFO, uint32 nLayer )
{
	uint32 NumAnimsInQueue = arrAFIFO.size();
	if (NumAnimsInQueue==0)
		return 0;

	CAnimation *arrAnimFiFo = SPU_PTR_SELECT( &arrAFIFO[0], (nLayer == 0 ? gAninmationFifoLayer0 : gAninmationFifo) );

#if defined(__SPU__)	
	memtransfer_sync( ASYNC_ANIM_DMA_1 );
#endif

	//the first animation in the queue MUST be activated
	if (arrAnimFiFo[0].m_bActivated==0)
	{ 
		CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
		uint32 counter=0;
		uint32 num = arrAnimFiFo[0].m_Parametric.m_numAnims;
		for (uint32 a=0; a<num; a++)
		{
			int32 nAnimID = arrAnimFiFo[0].m_Parametric.m_nAnimID[a];
			const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimID) );
			if (pAnim->m_nAssetType==CAF_File)
			{
				int32 nGlobalID = pAnimationSet->GetGlobalIDByAnimID_Fast( nAnimID );
				GlobalAnimationHeaderCAF& rGAHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalID];
			//	if (rGAHeader.IsAssetOnDemand())
				{
					uint32 loaded = rGAHeader.IsAssetLoaded();
					if (loaded)
						counter++;
				}
			}
			if (pAnim->m_nAssetType==AIM_File)
			{
				int32 nGlobalID = pAnimationSet->GetGlobalIDByAnimID_Fast( nAnimID );
				GlobalAnimationHeaderAIM& rGAHeader = g_AnimationManager.m_arrGlobalAIM[nGlobalID];
			//	if (rGAHeader.IsAssetOnDemand())
				{
					uint32 loaded = rGAHeader.IsAssetLoaded();
					if (loaded)
						counter++;
				}
			}
		}

		if (counter==num)
			arrAnimFiFo[0].m_bActivated=1;
	}

	if (arrAnimFiFo[0].m_bActivated==0)
		return 0;


	//	uint32 nMaxActiveInQueue=2;	
	uint32 nMaxActiveInQueue=MAX_EXEC_QUEUE;	

	if (nMaxActiveInQueue>NumAnimsInQueue)
		nMaxActiveInQueue=NumAnimsInQueue;

	if (NumAnimsInQueue>nMaxActiveInQueue)
		NumAnimsInQueue=nMaxActiveInQueue;


	uint32 aq = EvaluateTransitionFlags( arrAnimFiFo, NumAnimsInQueue );	

	//-----------------------------------------------------------------------------------------------
	//-----------------------------------------------------------------------------------------------
	//-----------------------------------------------------------------------------------------------

	if (aq>nMaxActiveInQueue)
		aq=nMaxActiveInQueue;

	nMaxActiveInQueue=aq;

#ifdef _DEBUG
	for (uint32 a=0; a<nMaxActiveInQueue; a++)
	{
		assert(arrAnimFiFo[a].m_bActivated);
		f32 t=arrAnimFiFo[a].m_fAnimTime;
		assert(t>=0.0f && t<=1.0f);  //most likely this is coming from outside if CryAnimation
	}
#endif

	TransitionsBetweenSeveralAnimations( arrAnimFiFo, nMaxActiveInQueue );

	UpdateTransitionTime( arrAnimFiFo, nMaxActiveInQueue, fDeltaTime );

	AdjustTransitionWeights( arrAnimFiFo, nMaxActiveInQueue );				

#ifdef _DEBUG
	for (uint32 j=0; j<nMaxActiveInQueue; j++)
	{
		if (arrAnimFiFo[j].m_fTransitionWeight<0.0f)
			assert(0);
		if (arrAnimFiFo[j].m_fTransitionWeight>1.0f)
			assert(0);
		if (arrAnimFiFo[j].m_fTransitionPriority<0.0f)
			assert(0);
		if (arrAnimFiFo[j].m_fTransitionPriority>1.0f)
			assert(0);
	}
#endif

	f32 TotalWeights=0.0f;
	for (uint32 i=0; i<nMaxActiveInQueue; i++) 
		TotalWeights += arrAnimFiFo[i].m_fTransitionWeight;
	//		assert( fabsf(TotalWeights-1.0f) < 0.001f );

	f32 fTotal = fabsf(TotalWeights-1.0f);
	if (fTotal>0.01f)
	{
		assert(!"sum of transition-weights is wrong");	
		CryFatalError("CryAnimation: sum of transition-weights is wrong: %f %s",fTotal,m_pInstance->m_pModel->GetModelFilePath());
	}

	for (uint32 a=0; a<nMaxActiveInQueue; a++)
	{
		CAnimation &rCurAnim = arrAnimFiFo[a];
		assert(rCurAnim.m_bActivated);
		if (rCurAnim.m_AnimParams.m_nFlags & CA_FORCE_SKELETON_UPDATE)
		{
			m_pSkeletonPose->m_bFullSkeletonUpdate=1; //This means we have to do a full-skeleton update
		//	float fColor[4] = {1,1,0,1};
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.7f, fColor, false,"CA_FORCE_SKELETON_UPDATE" );	
		//	g_YLine+=16.0f;
		}
	}

	// editor sets parameter elsewhere, so avoid setting parameters here, which overwrites editor parameters.
	// if the locomotion macro is active, don't execute this function, because it is doing it's own calculations
	// TSB: Not updating this on !m_pSkeletonPose->m_bFullSkeletonUpdate causes offscreen chars animTimes to be 
	// updated without setting the delta time, which defaults to -1
	//if (m_pSkeletonPose->m_bFullSkeletonUpdate)
	{
		if (m_CharEditMode==0)
			SetDesiredMotionParamsFromDesiredLocalLocation(fDeltaTime);
		LMGBlendWeightCalculation( arrAnimFiFo, nMaxActiveInQueue );
		UpdateMotionParamBlendSpacesForActiveMotions(fDeltaTime);
		ComputeBlendWeights( arrAnimFiFo, nMaxActiveInQueue, fDeltaTime, nLayer );
	}

	AdjustAnimationTimeForTimeWarpedAnimations( arrAnimFiFo, nMaxActiveInQueue );	

	UpdateAnimTimeAndExecBuffer( arrAnimFiFo, nMaxActiveInQueue, nLayer, NumAnimsInQueue );

	return 0;
}



/////////////////////////////////////////////////////////////////////////////////
// this function is handling animation-update, interpolation and transitions   //
/////////////////////////////////////////////////////////////////////////////////
uint32 CSkeletonAnim::BlendManagerDebug(	DynArray<CAnimation>& arrAFIFO, uint32 nVLayer )
{
#if !defined(__SPU__)
	//const char* mname = m_pInstance->GetFilePath();
	//if ( strcmp(mname,"objects/characters/alien/hunter/hunter.cdf") )
	//	return 0;

	ColorF cColor = ColorF(1,0,0,1);
	f32 fColor[4] = {1,0,0,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"model: %s",mname); g_YLine+=0x10;

	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;

	uint32 NumAnimsInQueue = arrAFIFO.size();
	for (uint32 i=0; i<NumAnimsInQueue; i++)
	{

		if (arrAFIFO[i].m_bActivated)
		{
			fColor[0] = arrAFIFO[i].m_fTransitionWeight;
			fColor[1] = (nVLayer>0);// % 2 ? arrAFIFO[i].m_fTransitionWeight * 0.3f : 0.0f;
			fColor[2] = 0.0f;
			fColor[3] = arrAFIFO[i].m_fTransitionWeight;
			if (nVLayer)
				fColor[3]=m_arrLayerBlending[nVLayer]*m_arrLayerBlendingMult[nVLayer]*arrAFIFO[i].m_fTransitionWeight;
		}
		else
		{
			fColor[0] = 0.0f;
			fColor[1] = 0.0f;
			fColor[2] = 0.0f;
			fColor[3] = 0.5f;
		}


		const char* pAnimName=0;
		if (arrAFIFO[i].m_Parametric.m_nParametricID>=0)
			pAnimName=pAnimationSet->GetModelAnimationHeaderRef(arrAFIFO[i].m_Parametric.m_nParametricID).GetAnimName();
		else 
			pAnimName=pAnimationSet->GetModelAnimationHeaderRef(arrAFIFO[i].m_Parametric.m_nAnimID[0]).GetAnimName();

		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"AnimInAFIFO %02d: t:%d  %s  ATime:%.2f (%.2fs/%.2fs) ASpd:%.2f Flag:%08x (%s) TTime:%.2f TWght:%.2f seg:%02d", nVLayer, arrAFIFO[i].m_AnimParams.m_nUserToken,  pAnimName, arrAFIFO[i].m_fAnimTime, arrAFIFO[i].m_fAnimTime*arrAFIFO[i].m_fCurrentDuration, arrAFIFO[i].m_fCurrentDuration, arrAFIFO[i].m_fCurrentPlaybackSpeed, arrAFIFO[i].m_AnimParams.m_nFlags, AnimFlags(arrAFIFO[i].m_AnimParams.m_nFlags), arrAFIFO[i].m_AnimParams.m_fTransTime, arrAFIFO[i].m_fTransitionWeight, arrAFIFO[i].m_Parametric.m_nSegmentCounter[0] ); g_YLine+=0x0e;
		if (nVLayer == 0)
		{
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"AimPoses: %s %s  AimIK: %d  AimIKBlend: %.2f",arrAFIFO[i].m_strAimPosName0,arrAFIFO[i].m_strAimPosName1,m_pSkeletonPose->m_AimIK().m_UseAimIK,m_pSkeletonPose->m_AimIK().m_AimIKBlend ); g_YLine+=0x0e;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"speed: %.2f   turn: %.2f   slope: %.2f   strafe:(%.2f %.2f)", arrAFIFO[i].m_Parametric.m_BlendSpace.m_speed, arrAFIFO[i].m_Parametric.m_BlendSpace.m_turn, arrAFIFO[i].m_Parametric.m_BlendSpace.m_slope,  arrAFIFO[i].m_Parametric.m_BlendSpace.m_strafe.x, arrAFIFO[i].m_Parametric.m_BlendSpace.m_strafe.y );g_YLine+=0x0e;
		}
		else
		{
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"Additive: %.2f", m_arrAdditiveWeights[nVLayer]); g_YLine+=0x0e;
		}
		g_YLine+=0x02;

	}
	//if (NumAnimsInQueue > 0)
	//	g_YLine+=0x0e;

#endif 
	return 0;
}






/////////////////////////////////////////////////////////////////////////////////
//             this function is handling blending between layers               //
/////////////////////////////////////////////////////////////////////////////////
void CSkeletonAnim::LayerBlendManager( f32 fDeltaTime, uint32 nLayer )
{
	if (nLayer==0)
		return;


	int8 status = GetActiveLayer(nLayer);

	f32 time = m_arrLayerBlendingTime[nLayer];

	f32 fCurLayerBlending = m_arrLayerBlending[nLayer];

	if (time<0.00001f)
		time=0.00001f;

	//f32 fColor[4] = {0.5f,0.0f,0.5f,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"nLayer: %02d  status: %1d, blend: %f  ", nLayer, status, fCurLayerBlending[nLayer]  ); 
	//g_YLine+=0x10;
	if (status)
		fCurLayerBlending += fDeltaTime/time;
	else
		fCurLayerBlending -= fDeltaTime/time;

	if (fCurLayerBlending > 1.0f)
		fCurLayerBlending = 1.0f;
	if (fCurLayerBlending < 0.0f)
		fCurLayerBlending = 0.0f;


	uint32 numAnims = m_arrLayer_AFIFO[nLayer].size();
	if (numAnims&& (fCurLayerBlending==0.0f) && (status==0))
	{		
		// layer0 needs special treatment on spu
#if defined(__SPU__)
		if( nLayer == 0 )
			gAninmationFifoLayer0[0].m_bRemoveFromQueue=1;
		else
#endif		
			m_arrLayer_AFIFO[nLayer][0].m_bRemoveFromQueue=1;
	}

	m_arrLayerBlending[nLayer] = fCurLayerBlending;
	return;
}






//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------

SPU_NO_INLINE void CSkeletonAnim::UpdateAndPushIntoExecBuffer( CAnimation *arrAnimFiFo, uint32 nLayer, uint32 NumAnimsInQueue, uint32 AnimNo )
{
	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;

	CAnimation &rAnimation = arrAnimFiFo[AnimNo];

	assert(rAnimation.m_fAnimTime<=2.0f);

	rAnimation.m_bEndOfCycle=0;
	uint32 numAimPoses=0;
	numAimPoses+=(rAnimation.m_nAnimAimID0>=0);
	numAimPoses+=(rAnimation.m_nAnimAimID1>=0);
	uint32 ManualUpdate			= (rAnimation.m_AnimParams.m_nFlags & CA_MANUAL_UPDATE)!=0;
	uint32 LoopAnimation			= (rAnimation.m_AnimParams.m_nFlags & CA_LOOP_ANIMATION)!=0;
	uint32 RepeatLastKey			= (rAnimation.m_AnimParams.m_nFlags & CA_REPEAT_LAST_KEY)!=0;
	if (LoopAnimation)	RepeatLastKey=0;
	if (ManualUpdate) rAnimation.m_fCurrentDeltaTime=0.0f;

	if (rAnimation.m_fCurrentDeltaTime>0.9f)	rAnimation.m_fCurrentDeltaTime=0.9f;
	if (rAnimation.m_fCurrentDeltaTime<-0.9f) rAnimation.m_fCurrentDeltaTime=-0.9f;


	f32 fCurrentDeltaTime = rAnimation.m_fCurrentDeltaTime;
	rAnimation.m_fAnimTimePrev	=	rAnimation.m_fAnimTime;
	rAnimation.m_fAnimTime		 +=	fCurrentDeltaTime;
	assert(rAnimation.m_fAnimTime<=2.0f);

	if (rAnimation.m_fAnimTime>=1.9999f)
		rAnimation.m_fAnimTime=1.9999f; //this is not supposed to happen

	if (fCurrentDeltaTime>=0.0f)
	{
		//time is running forward
		if (rAnimation.m_fAnimTime>1.0f)
		{
			rAnimation.m_nSegCounter++;

			uint32 Loop   =  rAnimation.m_nSegCounter<rAnimation.m_nSegHighest || LoopAnimation;
			uint32 Repeat = (RepeatLastKey && rAnimation.m_nSegCounter>=rAnimation.m_nSegHighest);

			if (Loop==0)
			{
				rAnimation.m_bRemoveFromQueue=1;
				rAnimation.m_fAnimTime = 1.0f;	//anim is not looping
			}

			if (Repeat)
			{
				rAnimation.m_nRepeatCount=1;
				rAnimation.m_bRemoveFromQueue=0;
				rAnimation.m_fAnimTime = 1.0f;

				//just for multi-layer 
				uint32 flags = rAnimation.m_AnimParams.m_nFlags;
				uint32  la	= flags&CA_LOOP_ANIMATION;
				if (NumAnimsInQueue==1 && nLayer && la==0)
				{
					uint32  rlk = flags&CA_REPEAT_LAST_KEY;
					uint32  fo	= flags&CA_FADEOUT;
					if (rlk && fo)
					{
						// to move a lot of unecessary stl main memory operations away from spu, call StopAnimationLayer deffered
						// for this re just remember the layer to stop the animation in a qeue, which is checked after the job has finished
						m_arrStopAnimationQueue[nStopAnimationQueuePos++] = nLayer;
					}

				}
			}

			if (Loop==1 && Repeat==0)
			{
				rAnimation.m_fAnimTime-=1.0f;
				assert(rAnimation.m_fAnimTime>=0.0f && rAnimation.m_fAnimTime<=1.0f);

				rAnimation.m_nLoopCount++;

				uint32 numAnims0 = rAnimation.m_Parametric.m_numAnims;
				for (uint32 i=0; i<numAnims0; i++)
				{
					int32 nAnimID = rAnimation.m_Parametric.m_nAnimID[i];
					const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimID) );
					if (pAnim->m_nAssetType==CAF_File)
					{
						int32 GlobalID = pAnim->m_nGlobalAnimId;
						GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[ GlobalID ];
						int32 seg = rGAH.m_Segments-1;
						rAnimation.m_Parametric.m_nSegmentCounter[i]++;
						if (rAnimation.m_Parametric.m_nSegmentCounter[i]>seg)
							rAnimation.m_Parametric.m_nSegmentCounter[i]=0;
					}
				}

				rAnimation.m_bEndOfCycle=1;
			}	
		}
	}
	else
	{
		//time is running backwards
		if (rAnimation.m_fAnimTime<0.0f)
		{
			if (LoopAnimation==0)
			{
				rAnimation.m_bRemoveFromQueue=1;
				rAnimation.m_fAnimTime = 0.0f;
			}
			if (RepeatLastKey)
			{
				rAnimation.m_bRemoveFromQueue=0;
				rAnimation.m_nRepeatCount=1;
				rAnimation.m_fAnimTime = 0.0f;
			}
			if (LoopAnimation && RepeatLastKey==0)
			{
				rAnimation.m_fAnimTime+=1.0f;
				rAnimation.m_nLoopCount++;
				rAnimation.m_bEndOfCycle=1;
			}	
		}
	}
	if( rAnimation.m_fTransitionWeight == 0.0f )
	{
		rAnimation.m_bRemoveFromQueue = 1;
	}

	assert(rAnimation.m_fAnimTime>=0.0f && rAnimation.m_fAnimTime<=1.0f);



	return;
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

SPU_NO_INLINE void CSkeletonAnim::AnimCallback ( CAnimation* arrAFIFO, uint32 AnimNo, uint32 AnimQueueLen)
{
	CAnimation &rAnimation = arrAFIFO[AnimNo];

	int32 nEOC =rAnimation.m_bEndOfCycle;
	f32 timeold=rAnimation.m_fAnimTimePrev;
	f32 timenew=rAnimation.m_fAnimTime;

	// prevent sending events when animation is not playing
	// like when there's an event at time 1.0f and repeat-last-key flag is used
	// otherwise it may happen to get the event each frame
	if ( timeold == timenew )
		return;
	if ( fabsf(m_pInstance->m_fDeltaTime) <0.00001f)
		return;
	if (nEOC)
		timenew+=1.0f;

	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;

	uint32 numEvents=0;
	AnimEvents* pCurAnimEvent=0;
	int32 nAnimID=-1;
	const char* pFilePath=0;
	if (rAnimation.m_Parametric.m_nParametricID<0)
	{
		const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(rAnimation.m_Parametric.m_nAnimID[0]) );
		if (pAnim->m_nAssetType==CAF_File)
		{
			int32 nGlobalLMGID	=	pAnim->m_nGlobalAnimId;
			nAnimID							= rAnimation.m_Parametric.m_nAnimID[0];
			GlobalAnimationHeaderCAF& rGlobalAnimHeaderCAF = g_AnimationManager.m_arrGlobalCAF[nGlobalLMGID];
			pFilePath = rGlobalAnimHeaderCAF.GetFilePath();
			numEvents = rGlobalAnimHeaderCAF.m_AnimEventsCAF.size();
			if (numEvents==0)
				return;
			pCurAnimEvent = &rGlobalAnimHeaderCAF.m_AnimEventsCAF[0];
		}
	}
	else
	{
		assert(rAnimation.m_Parametric.m_nParametricID>=0);
		const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(rAnimation.m_Parametric.m_nParametricID);
	//	int32 nAnimID			= rAnimation.m_Parametric.m_nParametricID;
		if(pAnim->m_nAssetType==LMG_File)
		{
			GlobalAnimationHeaderLMG& rGlobalAnimHeaderLMG = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];
			pFilePath = rGlobalAnimHeaderLMG.GetFilePath();
			numEvents = rGlobalAnimHeaderLMG.m_AnimEventsLMG.size();
			if (numEvents==0)
				return;
			pCurAnimEvent = &rGlobalAnimHeaderLMG.m_AnimEventsLMG[0];
		}
		if(pAnim->m_nAssetType==PMG_File)
		{
			GlobalAnimationHeaderPMG& rGlobalAnimHeaderPMG = g_AnimationManager.m_arrGlobalPMG[pAnim->m_nGlobalAnimId];
			pFilePath = rGlobalAnimHeaderPMG.GetFilePath();
			numEvents = rGlobalAnimHeaderPMG.m_AnimEventsPMG.size();
			if (numEvents==0)
				return;
			pCurAnimEvent = &rGlobalAnimHeaderPMG.m_AnimEventsPMG[0];
		}
	}


	if (pCurAnimEvent)
	{
		f32 deltaTime = fabs(timenew - timeold);
		for (uint32 i=0; i<numEvents; i++)
		{
			f32 time = pCurAnimEvent[i].m_time;

			if( ( time < deltaTime && (timeold -deltaTime) < FLT_EPSILON ) ||	( ( timeold<time || fabs(timeold-time)<FLT_EPSILON ) && ( timenew>time || fabs(timenew-time)<FLT_EPSILON) ) ) 
			{
				int32 a,b;

				const char* strModelName = pCurAnimEvent[i].m_strModelName.c_str();
				const char* strFilePath = SPU_MAIN_PTR( m_pInstance->CCharInstance::GetFilePath() );

				for (a=0; a<256; a++)	if (strModelName[a]==0)	break;
				for (b=0; b<256; b++) if (strFilePath[b]==0) break;
				uint32 equal=1;
				if (a)
				{
					for (; a>-1; a--,b--)
					{
						uint32 c0=strModelName[a];
						uint32 c1=strFilePath[b];
						if (c0!=c1)	{	equal=0; break;	}
					}
				}

				if (equal)
				{
					if( m_nEventQueuePos >= 5 )
					{
						//AnimWarning("SkeletonAnimEventQueue full, discarding new Events for this frame");
					}
					else
					{
						AnimEventInstance &rCurEventInstance = spuEventQueue[m_nEventQueuePos++];		
						rCurEventInstance.m_time								= time; 
						rCurEventInstance.m_AnimPathName				= pFilePath; 
						rCurEventInstance.m_AnimID							= nAnimID;
						rCurEventInstance.m_nAnimNumberInQueue	= AnimQueueLen-1 - AnimNo;  
						rCurEventInstance.m_fAnimPriority				= rAnimation.m_fTransitionPriority;
						rCurEventInstance.m_EventName						= pCurAnimEvent[i].m_strEventName.c_str();
						rCurEventInstance.m_CustomParameter			= pCurAnimEvent[i].m_strCustomParameter;
						rCurEventInstance.m_BonePathName				= pCurAnimEvent[i].m_strBoneName.c_str();
						rCurEventInstance.m_vOffset							= pCurAnimEvent[i].m_vOffset;
						rCurEventInstance.m_vDir								= pCurAnimEvent[i].m_vDir;
					}	
				}

				/*
				const char* mname = m_pInstance->GetFilePath();
				if ( strcmp(mname,"objects/characters/alien/hunter/hunter.cdf")==0 )
				{
				float fColor[4] = {1,0,0,1};
				g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.9f, fColor, false,"HunterEvent: time %f  AnimNo: %d	 Priority: %f   name: %s",time,AnimNo,arrAFIFO[AnimNo].m_fTransitionPriority,rCurAnimEvent.m_strEventName);
				g_YLine+=16.0f;
				}
				*/

			}             
		}
	}
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
bool CSkeletonAnim::RemoveAnimFromFIFO(uint32 nLayer, uint32 num)
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
			f32 fColor[4] = {1,1,0,1};
			const char* mname = m_pInstance->GetFilePath();
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 3.2f, fColor, false,"model: %s",mname); 
			g_YLine+=0x20;
		}
	}*/

	if (nLayer >= numVIRTUALLAYERS)
		return false;
	DynArray<CAnimation>& rAnim = m_arrLayer_AFIFO[nLayer];
	if (num >= rAnim.size())
		return false;
	if (rAnim[num].m_bActivated)
		return false;

	uint32 numAnims = rAnim[num].m_Parametric.m_numAnims;
	for (uint32 i=0; i<numAnims; i++)
	{
		CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
		int32 nAnimID = rAnim[num].m_Parametric.m_nAnimID[i];
		const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimID) );
		if (pAnim->m_nAssetType==CAF_File)
		{
#ifdef _DEBUG
			const char* pname = pAnim->GetAnimName();
#endif
			int32 nGlobalID = pAnimationSet->GetGlobalIDByAnimID_Fast(rAnim[num].m_Parametric.m_nAnimID[i]);
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
				g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,0,	"++++Dec: Frame:%d  %x Name: %s",nCurrentFrameID, rGAH.m_nRef_at_Runtime,rGAH.m_FilePath );
				*/
			}
		}
	}

	rAnim.erase(rAnim.begin() + num);
	return true;
}


CAnimation* CSkeletonAnim::FindAnimInFIFO( uint32 nUserToken,int nLayer, bool includeDeferredAnimation )
{
	int startLayer = 0;
	int endLayer = numVIRTUALLAYERS;
	if (nLayer >= 0)
	{
		assert( nLayer >= 0 && nLayer < numVIRTUALLAYERS );
		startLayer = nLayer;
		endLayer = nLayer+1;
	}

	for (int lyr = startLayer; lyr < endLayer; lyr++)
	{
		DynArray<CAnimation>& fifo = m_arrLayer_AFIFO[lyr];
		for (int i = 0,num = (int)fifo.size(); i < num; i++)
		{
			if (fifo[i].m_AnimParams.m_nUserToken == nUserToken)
			{
				return &fifo[i];
			}
		}
	}

	if (includeDeferredAnimation)
	{
		for (int i = 0,num = (int)m_arrDeferredQueueUpdates.size(); i < num; i++)
		{
			if (m_arrDeferredQueueUpdates[i].m_nLayer >= startLayer && m_arrDeferredQueueUpdates[i].m_nLayer < endLayer)
			{
				if (m_arrDeferredQueueUpdates[i].m_Anim.m_AnimParams.m_nUserToken == nUserToken)
				{
					return &m_arrDeferredQueueUpdates[i].m_Anim;
				}
			}
		}
	}
	return NULL;
}

int CSkeletonAnim::GetNumAnimsInFIFOAndDeferredQueue(uint32 nLayer)
{
	int iNumAnimsInFIFO = GetNumAnimsInFIFO(nLayer);

	int iNumAnimsInDeferredQueue = 0;
	const int iDeferredQueueSize = m_arrDeferredQueueUpdates.size();
	for (int i = 0; i < iDeferredQueueSize; ++i)
	{
		if (m_arrDeferredQueueUpdates[i].m_nLayer == nLayer)
			++iNumAnimsInDeferredQueue;
	}

	return iNumAnimsInFIFO + iNumAnimsInDeferredQueue;
}

CAnimation& CSkeletonAnim::GetAnimFromFIFOAndDeferredQueue(uint32 nLayer, uint32 num)
{
#if defined(__SPU__) // crycg workaround for multiple constructor calls
	int iNumAnimsInFIFO = GetNumAnimsInFIFO(nLayer);
	if (num >= iNumAnimsInFIFO)
	{
		CAnimation* ret = NULL;

		const int iIdx = num - iNumAnimsInFIFO;
		int iLayerAnimCount = 0;

		const int iDeferredQueueSize = m_arrDeferredQueueUpdates.size();
		for (int i = 0; i < iDeferredQueueSize; ++i)
		{
			DeferredQueueUpdate* deferredAnim = &(m_arrDeferredQueueUpdates[i]);
			if (deferredAnim->m_nLayer == nLayer)
			{
				if (iLayerAnimCount == iIdx)
				{
					ret = &deferredAnim->m_Anim;
					return *ret;
				}

				++iLayerAnimCount;
			}
		}

		ret = &g_DefaultAnim;
		return *ret;
	}
	else
	{
		return GetAnimFromFIFO(nLayer, num);
	}

#else
	int iNumAnimsInFIFO = GetNumAnimsInFIFO(nLayer);
	if (num >= iNumAnimsInFIFO)
	{
		const int iIdx = num - iNumAnimsInFIFO;
		int iLayerAnimCount = 0;

		const int iDeferredQueueSize = m_arrDeferredQueueUpdates.size();
		for (int i = 0; i < iDeferredQueueSize; ++i)
		{
			DeferredQueueUpdate& deferredAnim = m_arrDeferredQueueUpdates[i];
			if (deferredAnim.m_nLayer == nLayer)
			{
				if (iLayerAnimCount == iIdx)
					return deferredAnim.m_Anim;

				++iLayerAnimCount;
			}
		}

		return g_DefaultAnim;
	}
	else
	{
		return GetAnimFromFIFO(nLayer, num);
	}
#endif
}

void CSkeletonAnim::ManualSeekAnimationInFIFO(uint32 nLayer, uint32 num, float time2, bool advance)
{
	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
	uint32 numAnimsLayer = GetNumAnimsInFIFO(nLayer);
	if (num < numAnimsLayer)
	{
		CAnimation &animation = GetAnimFromFIFO(nLayer, num);

		if (animation.m_AnimParams.m_nFlags & (CA_MANUAL_UPDATE | CA_TRACK_VIEW_EXCLUSIVE))
		{

			if (animation.m_Parametric.m_nParametricID<0)
			{
				//this is a CAF file

				int32 animID		=  animation.m_Parametric.m_nAnimID[0];
				assert(animID>=0);
				int32 globalID	= pAnimationSet->GetGlobalIDByAnimID_Fast(animID);
				assert(globalID>=0);

				uint32 numCAFs = g_AnimationManager.m_arrGlobalCAF.size();
				assert(globalID<numCAFs);

				if ((globalID>=0) && (globalID<numCAFs))
				{
					const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(animID) );
					if (pAnim->m_nAssetType==CAF_File)
					{
						const GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[globalID];

						f32 timeold, timenew;
						if (advance)
							timeold = animation.m_fAnimTime, timenew = time2;
						else
							timeold = time2, timenew = animation.m_fAnimTime;
						if (timenew < timeold)
							timenew += 1.0f;

						// prevent sending events when animation is not playing
						// otherwise it may happen to get the event each frame
						if ( timeold != timenew )
						{
							uint32 numEvents = rGlobalAnimHeader.m_AnimEventsCAF.size();
							for (uint32 i=0; i<numEvents; i++)
							{
								f32 time = rGlobalAnimHeader.m_AnimEventsCAF[i].m_time;
								if ((timeold<=time && timenew>=time) || (timeold<=time + 1.0f && timenew>=time + 1.0f))
								{
									m_LastAnimEvent.m_time=time; 
									m_LastAnimEvent.m_AnimPathName=	rGlobalAnimHeader.GetFilePath(); 
									m_LastAnimEvent.m_AnimID = animID;
									m_LastAnimEvent.m_EventName				=	rGlobalAnimHeader.m_AnimEventsCAF[i].m_strEventName.c_str(); 
									m_LastAnimEvent.m_CustomParameter	=	rGlobalAnimHeader.m_AnimEventsCAF[i].m_strCustomParameter;
									m_LastAnimEvent.m_BonePathName		= rGlobalAnimHeader.m_AnimEventsCAF[i].m_strBoneName.c_str();
									m_LastAnimEvent.m_vOffset					= rGlobalAnimHeader.m_AnimEventsCAF[i].m_vOffset;
									m_LastAnimEvent.m_vDir						= rGlobalAnimHeader.m_AnimEventsCAF[i].m_vDir;

									if (m_pEventCallback)		
										(*m_pEventCallback)(m_pInstance,m_pEventCallbackData);
								}			
							}
						}
					}
				}
			}
			else 
			{
				const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(animation.m_Parametric.m_nParametricID);
				assert(pAnim->m_nAssetType==LMG_File);

				int32 globalID	= pAnim->m_nGlobalAnimId;
				assert(globalID >= 0 && globalID < int(g_AnimationManager.m_arrGlobalLMG.size()));
				if (globalID >= 0 && globalID < int(g_AnimationManager.m_arrGlobalLMG.size()))
				{
					const GlobalAnimationHeaderLMG& rGlobalAnimHeaderLMG = g_AnimationManager.m_arrGlobalLMG[globalID];

					f32 timeold, timenew;
					if (advance)
						timeold = animation.m_fAnimTime, timenew = time2;
					else
						timeold = time2, timenew = animation.m_fAnimTime;
					if (timenew < timeold)
						timenew += 1.0f;

					// prevent sending events when animation is not playing
					// otherwise it may happen to get the event each frame
					if ( timeold != timenew )
					{
						uint32 numEvents = rGlobalAnimHeaderLMG.m_AnimEventsLMG.size();
						for (uint32 i=0; i<numEvents; i++)
						{
							f32 time = rGlobalAnimHeaderLMG.m_AnimEventsLMG[i].m_time;
							if ((timeold<=time && timenew>=time) || (timeold<=time + 1.0f && timenew>=time + 1.0f))
							{
								m_LastAnimEvent.m_time=time; 
								m_LastAnimEvent.m_AnimPathName		=	rGlobalAnimHeaderLMG.GetFilePath(); 
								m_LastAnimEvent.m_AnimID					= animation.m_Parametric.m_nParametricID;
								m_LastAnimEvent.m_EventName				=	rGlobalAnimHeaderLMG.m_AnimEventsLMG[i].m_strEventName.c_str(); 
								m_LastAnimEvent.m_CustomParameter	=	rGlobalAnimHeaderLMG.m_AnimEventsLMG[i].m_strCustomParameter;
								m_LastAnimEvent.m_BonePathName		= rGlobalAnimHeaderLMG.m_AnimEventsLMG[i].m_strBoneName.c_str();
								m_LastAnimEvent.m_vOffset					= rGlobalAnimHeaderLMG.m_AnimEventsLMG[i].m_vOffset;
								m_LastAnimEvent.m_vDir						= rGlobalAnimHeaderLMG.m_AnimEventsLMG[i].m_vDir;

								if (m_pEventCallback)		
									(*m_pEventCallback)(m_pInstance,m_pEventCallbackData);
							}			
						}
					}
				}
			}




			// Update the time.
			animation.m_fAnimTime = time2;
		}
	}
}


f32 CSkeletonAnim::GetRelRotationZ()
{
	f32 assetRot = m_pSkeletonPose->GetLocator().m_rotationRelative;

	//	float fColor1[4] = {0,1,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor1, false,"assetRot%f: assetRot %f",assetRot,assetRot/m_pInstance->m_fDeltaTime );	
	//	g_YLine+=16.0f;

	if (m_CharEditBlendSpaceOverrideEnabled[eMotionParamID_TurnSpeed])
		return assetRot;

	f32 parameterizedRotBlended = 0.0f;

	int layer = 0;
	int animCount = GetNumAnimsInFIFO(layer);

	//	float weightSum = 0.0f;
	for (int animIndex = 0; animIndex < animCount; ++animIndex)
	{
		const CAnimation& anim = GetAnimFromFIFO(layer, animIndex);
		if (anim.m_bActivated)
		{
			const MotionParamBlendSpace& turnSpeed = anim.m_Parametric.m_params[eMotionParamID_TurnSpeed].blendspace;
			const MotionParamBlendSpace& turnAngle = anim.m_Parametric.m_params[eMotionParamID_TurnAngle].blendspace;
			//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"turnSpeed.proceduralOffset: %f   turnSpeed.proceduralScale: %f",turnSpeed.proceduralOffset,turnSpeed.proceduralScale);	g_YLine+=16.0f;
			//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"turnAngle.proceduralOffset: %f   turnAngle.proceduralScale: %f",turnAngle.proceduralOffset,turnAngle.proceduralScale);	g_YLine+=16.0f;

			// Both of these parameters will not be active/used at the same time,
			// and thus either parameter will have zero offset and a scale of 1.0, which will not have any affect.
			f32 offset	= turnSpeed.m_fProceduralOffset + turnAngle.m_fProceduralOffset;
			//			f32 scale		= turnSpeed.m_fProceduralScale * turnAngle.m_fProceduralScale;

			// If there is no turn in the asset, the offset will be the desired turn speed.
			// If there is turn in the asset, the offset will be zero and the actual asset rotation is used untouched.
			// The scaling is used to scale the rotation from the asset to reach the desired turn speed.
			// For details on how offset and scale is calculated, see CalculateMotionParamBlendSpace().


			//this old code made heavy problems, because the tuning-speed of the "assetRot" was never constant
			//	f32 parameterizedRot = offset*m_pInstance->m_fDeltaTime + scale*assetRot;

			f32 parameterizedRot=0;
			if (offset)
				parameterizedRot = offset*m_pInstance->m_fDeltaTime;  //just for parameterized turning while running
			else
				parameterizedRot = assetRot;  //Currently used just for Idle2Move and IdleStepTurns

			// Apply each motions rotation in relation to it's transition blend weight.
			//float weight = anim.m_fTransitionPriority * anim.m_fTransitionPriority;
			//weightSum += weight;
			//parameterizedRotBlended = LERP(parameterizedRotBlended, parameterizedRot, weight);
			parameterizedRotBlended += parameterizedRot*anim.m_fTransitionWeight; 
		}
	}

	//	if (weightSum > 0.0f)
	//		parameterizedRotBlended /= weightSum;
	//	float fColor[4] = {1,1,1,1};
	//	g_pIRenderer->Draw2dLabel(100, g_YLine, 2.0f, fColor, false, "parameterizedRotBlended: %f ",parameterizedRotBlended); 
	//	g_YLine+=0x20;

	return parameterizedRotBlended;

	//IVO: if we set desired turn, then we have to make sure that we really get desired turn and not something else.
	//using the blend-weight for this is giving a result that is very similar, but it is not EXACTLY what I need.
	//I will try to find a better parameterization for the turns, but this will do for now. 
	//Currently the turn are just "decoration". The accuracy is just 90% of the animation that is actually doing the turn-animation.

	//DAVID: For the system to work properly and in a generic enough way the motion-parameters to animation-blendweights calculations must be 99% accurate.
};


const QuatT& CSkeletonAnim::GetRelMovement()
{
	return m_RelativeMovement;
}




//! Set the current time of the given layer, in seconds
void CSkeletonAnim::SetLayerTime (uint32 nLayer, f32 fNormalizedTime)
{
	assert(fNormalizedTime>=0.0f && fNormalizedTime<=1.0f);

	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;

	uint32 numAnims = m_arrLayer_AFIFO[nLayer].size();
	if (numAnims==1)
	{
		int32 nLMGAnimID = m_arrLayer_AFIFO[nLayer][0].m_Parametric.m_nParametricID;
		if (nLMGAnimID<0)
		{
			//caf file
			uint32 segment=0;
			int32	agid = pAnimationSet->GetGlobalIDByAnimID_Fast( m_arrLayer_AFIFO[nLayer][0].m_Parametric.m_nAnimID[0] );
			GlobalAnimationHeaderCAF& rsubGAH0 = g_AnimationManager.m_arrGlobalCAF[ agid ];

			for (uint32 i=0; i<4; i++)
			{
				if (fNormalizedTime>=rsubGAH0.m_SegmentsTime[i] && fNormalizedTime<=rsubGAH0.m_SegmentsTime[i+1])
				{							
					segment=i;
					break;
				}
			}
			m_arrLayer_AFIFO[nLayer][0].m_Parametric.m_nSegmentCounter[0]=segment;

			f32 t0	=	rsubGAH0.m_SegmentsTime[segment+0];
			f32 t1	=	rsubGAH0.m_SegmentsTime[segment+1];
			f32 d		=	t1-t0;
			f32 t		=	fNormalizedTime-t0;
			m_arrLayer_AFIFO[nLayer][0].m_fAnimTime = t/d;
		}
		else
		{
			//parametric animation
			m_arrLayer_AFIFO[nLayer][0].m_fAnimTime = fNormalizedTime;
		} 

		assert(m_arrLayer_AFIFO[nLayer][0].m_fAnimTime>=0.0f && m_arrLayer_AFIFO[nLayer][0].m_fAnimTime<=1.0f);

	}

};


//! Return the current time of the given layer, in normalized time [0...1]
float CSkeletonAnim::GetLayerTime (uint32 nLayer)const
{
	if (m_arrLayer_AFIFO[nLayer].empty())
		return 0.0f;

	return m_arrLayer_AFIFO[nLayer][0].m_fAnimTime;
};


void CSkeletonAnim::SetAnimationDrivenMotion(uint32 ts) 
{
	m_AnimationDrivenMotion=ts;  
};
void CSkeletonAnim::SetMirrorAnimation(uint32 ts) 
{
	m_MirrorAnimation=ts;  
};


// sets the animation speed scale for layers
void CSkeletonAnim::SetLayerUpdateMultiplier(int32 nLayer, f32 fSpeed)
{
	if (nLayer>=numVIRTUALLAYERS || nLayer<0)
	{
		g_pILog->LogError ("SetLayerUpdateMultiplier() was using invalid layer id: %d", nLayer);
		return;
	}
	m_arrLayerSpeedMultiplier[nLayer] = fSpeed;
}

void CSkeletonAnim::SetLayerBlendMultiplier(int32 nLayer, f32 fMult)
{
	if (nLayer>=numVIRTUALLAYERS || nLayer<0)
	{
		g_pILog->LogError ("invalid layer id: %d", nLayer);
		return;
	}
	m_arrLayerBlendingMult[nLayer] = fMult;
}

// sets the animation speed scale for layers
void CSkeletonAnim::SetAdditiveWeight(int32 nLayer, f32 fWeight)
{
	if (nLayer>=numVIRTUALLAYERS || nLayer<0)
	{
		g_pILog->LogError ("SetAdditiveWeight() was using invalid layer id: %d", nLayer);
		return;
	}
	m_arrAdditiveWeights[nLayer] = fWeight;
}

//---------------------------------------------------------



Vec3 CSkeletonAnim::GetCurrentAimDirection() 
{	
	return m_pSkeletonPose->m_AimIK().m_AimDirection; 
};
Vec3 CSkeletonAnim::GetCurrentLookDirection() 
{ 
	return m_pSkeletonPose->m_LookIK().m_LookDirection; 
};



ILINE f32 GetYaw( const Quat& quat)
{
	Vec3 forward = quat.GetColumn1(); 
	assert( fabsf(forward.z)<0.01f );
	forward.z=0; 
	forward.Normalize();
	return -atan2f( forward.x,forward.y );
}




SPU_NO_INLINE uint32 CSkeletonAnim::EvaluateTransitionFlags( CAnimation *arrAnimFiFo, uint32 numAnims )
{
	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
	//-------------------------------------------------------------------------------
	//---                            evaluate transition flags                    ---
	//-------------------------------------------------------------------------------
	uint32 aq=0;
	for (aq=1; aq<numAnims; aq++)
	{
		// on spu, sync the asynchron memtransfer during the loop in which they are needed
#if defined(__SPU__)
		if( aq == 2 )		memtransfer_sync( ASYNC_ANIM_DMA_2 );
		if( aq == 4 )		memtransfer_sync( ASYNC_ANIM_DMA_3 );
#endif

		CAnimation &rPrevAnimation = arrAnimFiFo[aq-1];
		CAnimation &rCurAnimation = arrAnimFiFo[aq];

		if (rCurAnimation.m_bActivated)
			continue; //an activated motion will stay activated

		uint32 IsLooping			= rPrevAnimation.m_AnimParams.m_nFlags & CA_LOOP_ANIMATION;

		uint32 StartAtKeytime		= rCurAnimation.m_AnimParams.m_nFlags & CA_START_AT_KEYTIME;
		uint32 StartAfter			= rCurAnimation.m_AnimParams.m_nFlags & CA_START_AFTER;
		uint32 Idle2Move			= rCurAnimation.m_AnimParams.m_nFlags & CA_IDLE2MOVE;
		uint32 Move2Idle			= rCurAnimation.m_AnimParams.m_nFlags & CA_MOVE2IDLE;
		uint32 NotInMemory		= 0; 

		uint32 num = rCurAnimation.m_Parametric.m_numAnims;
		for (uint32 a=0; a<num; a++)
		{
			int32 nGlobalID = pAnimationSet->GetGlobalIDByAnimID_Fast(rCurAnimation.m_Parametric.m_nAnimID[a]);
			GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalID];
			if (rGlobalAnimHeader.IsAssetOnDemand())
			{
				uint32 loaded = rGlobalAnimHeader.IsAssetLoaded();
				if (loaded==0)
				{
					NotInMemory=1;
					break;
				}
			}
		}


#if !defined(__SPU__)
		if (Console::GetInst().ca_DebugAnimationStreaming && NotInMemory)
		{
			const char* pName=0;
			const ModelAnimationHeader* anim;
			if (rCurAnimation.m_Parametric.m_nParametricID>=0)
				anim	=	&pAnimationSet->GetModelAnimationHeaderRef(rCurAnimation.m_Parametric.m_nParametricID);
			else
				anim	=	&pAnimationSet->GetModelAnimationHeaderRef(rCurAnimation.m_Parametric.m_nAnimID[0]);

			pName=anim->GetAnimName();

			float fC1[4] = {1,1,0,1};
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.6f, fC1, false,"Streaming Assets: %s ",pName );	
			g_YLine+=16.0f;

			g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pInstance->GetFilePath(), "CryAnimation: streaming file: %s",pName );
		}
#endif


		//can we use the "Idle2Move" flag?
		if (Idle2Move)
		{
			Idle2Move=0;  //lets be pessimistic and assume we can't use this flag at all
			assert(rCurAnimation.m_bActivated==0);
			int32 anim_lmg_id=rPrevAnimation.m_Parametric.m_nParametricID;

			if (anim_lmg_id>=0)
			{
			//	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
				const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(anim_lmg_id);
				assert(pAnim->m_nAssetType==LMG_File);

				GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];
				assert(rGlobalAnimHeader.m_nBlendCodeLMG);
				uint32 bc=rGlobalAnimHeader.m_nBlendCodeLMG;

				//NOTE cyrcg cannot handle casting directly from a char string to an int
				char *I2M1 = "I2M1";
				char *I2M2 = "I2M2";
				char *XIM2 = "XIM2";

				if (bc== *(uint32*)I2M1) Idle2Move=1;  //for this LMG we can use it
				if (bc== *(uint32*)I2M2) Idle2Move=1;  //for this LMG we can use it
				if (bc== *(uint32*)XIM2) Idle2Move=2;  //for this LMG we can use it

			}
		}

		//can we use the "StartAfter" flag?
		if (IsLooping && StartAfter)
			StartAfter=0; //if first animation is looping, then start transition immediately to prevent a hang in the FIFO

		//-------------------------------------------------------------------------------------------------
		//-- If there are several animations in the FIFO it depends on the transition flags whether      --
		//-- we can start a transition immediately. Maybe we want to play one animation after another.   --
		//-------------------------------------------------------------------------------------------------
		uint32 TransitionDelay = StartAtKeytime+StartAfter+Idle2Move+Move2Idle+NotInMemory;
		if (TransitionDelay)
		{
			if (StartAtKeytime)
			{
				assert(rCurAnimation.m_bActivated==0);
				f32 atnew=rPrevAnimation.m_fAnimTime;
				f32 atold=rPrevAnimation.m_fAnimTime-0.000001f;
				if (atold<rCurAnimation.m_AnimParams.m_fKeyTime  && atnew>rCurAnimation.m_AnimParams.m_fKeyTime)
					rPrevAnimation.m_nKeyPassed++;

				if (rPrevAnimation.m_nKeyPassed)
					rCurAnimation.m_bActivated=true;
			}

			if (StartAfter)
			{
				assert(rCurAnimation.m_bActivated==0);
				if (rPrevAnimation.m_nRepeatCount)
					rCurAnimation.m_bActivated=true;
			}

			if (Idle2Move==1)
			{
				uint32 nSegCount=rPrevAnimation.m_Parametric.m_nSegmentCounter[0];
				Vec2 vLockedStrafeVector = rPrevAnimation.m_Parametric.m_BlendSpace.m_strafe;

				if (vLockedStrafeVector.x<0.001f)
				{
					if (nSegCount)	
						rCurAnimation.m_bActivated=true;	//move to the left
				}
				else
				{
					if (nSegCount && rPrevAnimation.m_fAnimTime>0.50f)
						rCurAnimation.m_bActivated=true;	//move to the right
				}
			}

			if (Idle2Move==2)
			{
				uint32 nSegCount=rPrevAnimation.m_Parametric.m_nSegmentCounter[0];
				f32 vLockedTurnAngle = rPrevAnimation.m_Parametric.m_BlendSpace.m_turn;

				if (vLockedTurnAngle<0)
				{
					if (nSegCount)	
						rCurAnimation.m_bActivated=true;	//move to the left
				}
				else
				{
					if (nSegCount && rPrevAnimation.m_fAnimTime>0.50f)
						rCurAnimation.m_bActivated=true;	//move to the right
				}
			}


			if (Move2Idle)
			{
				assert(rCurAnimation.m_bActivated==0);
				if (rPrevAnimation.m_fAnimTime<0.40f)
					rCurAnimation.m_bActivated=true;
			}

			if (rCurAnimation.m_bActivated==0)
				break; //all other animations in the FIFO will remain not activated
		} 
		else 
		{
			//No transition-delay flag set. Thats means we can activate the transition immediately
			rCurAnimation.m_bActivated=true;
		}
	}

	return aq;
}

SPU_NO_INLINE void CSkeletonAnim::TransitionsBetweenSeveralAnimations( CAnimation *arrAnimFiFo, uint32 numAnims )
{
	//----------------------------------------------------------------------------------------
	//-------            transitions between several animations                        -------
	//----------------------------------------------------------------------------------------

	//init time-warping
	for (uint32 i=1; i<numAnims; i++)
	{
		CAnimation &rPrevAnimation = arrAnimFiFo[i-1];
		CAnimation &rCurAnimation = arrAnimFiFo[i];

		assert(rCurAnimation.m_bActivated);// = true;

		//the new animation determines if we use time-warping or not
		uint32 timewarp = rCurAnimation.m_AnimParams.m_nFlags & CA_TRANSITION_TIMEWARPING;
		if (timewarp)
		{
			//animations are time-warped, so we have to adjust the delta-time
			f32 tp = rCurAnimation.m_fTransitionPriority;
			if (tp==0)
			{
				int32 LMG_OLD = rPrevAnimation.m_Parametric.m_nParametricID;
				int32 LMG_NEW = rCurAnimation.m_Parametric.m_nParametricID;

				if (LMG_OLD>=0 && LMG_OLD==LMG_NEW)
					rCurAnimation.m_Parametric = rPrevAnimation.m_Parametric;

				rCurAnimation.m_fAnimTimePrev=rPrevAnimation.m_fAnimTimePrev; //copy the prev. time from previous
				rCurAnimation.m_fAnimTime=rPrevAnimation.m_fAnimTime; //copy the time from previous
				assert(rCurAnimation.m_fAnimTime>=0.0f && rCurAnimation.m_fAnimTime<=1.0f);
				rCurAnimation.m_nSegCounter=0; //don't copy the segment from previous
			}
		}
	}
}

SPU_NO_INLINE void CSkeletonAnim::UpdateTransitionTime( CAnimation *arrAnimFiFo, uint32 numAnims, f32 fDeltaTime )
{
	//-----------------------------------------------------------------------------
	//-----     update the TRANSITION-TIME of all animations in the queue     -----
	//-----------------------------------------------------------------------------
	arrAnimFiFo[0].m_fTransitionPriority	= 1.0f;
	//	if (arrAnimFiFo[0].m_fTransitionPriority>1.0f)	arrAnimFiFo[0].m_fTransitionPriority=1.0f;
	for (uint32 i=1; i<numAnims; i++)
	{
		CAnimation &rCurAnimation = arrAnimFiFo[i];

		f32 BlendTime   = rCurAnimation.m_AnimParams.m_fTransTime;

		if (BlendTime<0) BlendTime = 1.0f; //if transition-time not specified, just put the time to 1
		if (BlendTime==0.0f) BlendTime=0.0001f; //we don't want DivByZero

		f32 ttime = fabsf(fDeltaTime)/BlendTime;
		if (m_TrackViewExclusive)	
			ttime = m_pInstance->m_fOriginalDeltaTime/BlendTime;

		rCurAnimation.m_fTransitionPriority += ttime;  //update transition time

		if (rCurAnimation.m_fTransitionPriority>1.0f)
			rCurAnimation.m_fTransitionPriority=1.0f;

	}
}

SPU_NO_INLINE void CSkeletonAnim::AdjustTransitionWeights( CAnimation *arrAnimFiFo, uint32 numAnims )
{
	//---------------------------------------------------------------------------------------
	//---  here we adjust the the TRANSITION-WEIGHTS between all animations in the queue  ---
	//----------------------------------------------------------------------------------------
	arrAnimFiFo[0].m_fTransitionWeight = 1.0f; //the first in the queue will always have the highest priority
	for (uint32 i=1; i<numAnims; i++)
	{
		CAnimation &rCurAnimation = arrAnimFiFo[i];

		rCurAnimation.m_fTransitionWeight = rCurAnimation.m_fTransitionPriority;
		f32 scale_previous = 1.0f - rCurAnimation.m_fTransitionWeight;

		for (uint32 j=0; j<i; j++)
			arrAnimFiFo[j].m_fTransitionWeight *= scale_previous;
	}

	f32 sum=0;
	for (uint32 i=0; i<numAnims; i++)
	{
		CAnimation &rCurAnimation = arrAnimFiFo[i];
		f32 w = rCurAnimation.m_fTransitionWeight;
		f32 x0 = clamp(w,0.0f,1.0f)-0.5f;
		x0 = x0/(0.5f+2.0f*x0*x0)+0.5f;
		rCurAnimation.m_fTransitionWeight=x0;
		sum+=x0;
	}

	for (uint32 i=0; i<numAnims; i++)
	{
		CAnimation &rCurAnimation = arrAnimFiFo[i];
		rCurAnimation.m_fTransitionWeight/=sum;
	}
}

void CSkeletonAnim::LMGBlendWeightCalculation( CAnimation *arrAnimFiFo, uint32 numAnims )
{
	//-------------------------------------------------------------------
	//--------           blend-weight calculation for LMGs         ------
	//-------------------------------------------------------------------
	//UpdateMotionParamDescsForActiveMotions();
	for (uint32 a=0; a<numAnims; a++)
	{
		CAnimation &rCurAnim = arrAnimFiFo[a];

		assert(rCurAnim.m_bActivated);
		UpdateMotionParamDescs(rCurAnim.m_Parametric, rCurAnim.m_fTransitionWeight);
	}
}

SPU_NO_INLINE void CSkeletonAnim::TestDrawParams(const std::vector< std::vector<f32> >& visParams, ColorB* cb, const Vec3& controlP, const uint32 numAsserts, f32* weights)
{
	uint32 numDim = (uint32)visParams.size();
	if(numDim == 0)
		return;

	uint32 numExample = 0;

	if(numDim > 0)
		numExample = (uint32)visParams[0].size();

	std::vector<Vec3> points;

	if(numDim == 1)
	{
		for(uint32 i=0; i<numExample; ++i)
		{
			Vec3 t(visParams[0][i], .0f, .0f);
			points.push_back(t);
		}
	}

	if(numDim == 1)
	{
		for(uint32 i=0; i<numExample; ++i)
		{
			Vec3 t(visParams[0][i], .0f, .0f);
			points.push_back(t);
		}
	}
	else if(numDim == 2)
	{
		for(uint32 i=0; i<numExample; ++i)
		{
			Vec3 t(visParams[0][i], visParams[1][i], .0f);
			points.push_back(t);
		}
	}
	else if(numDim >= 3)
	{
		for(uint32 i=0; i<numExample; ++i)
		{
			Vec3 t(visParams[0][i], visParams[1][i], visParams[2][i]);
			points.push_back(t);
		}
	}

	// The control point
	g_pAuxGeom->DrawSphere(	controlP,0.12f , RGBA8(0xff,0x00,0x00,0xff) );

	//------------------------------------------------------------------------------
	// Draw bounding box
	Vec3 minV(1e+6, 1e+6, 1e+6), maxV(-1e+6, -1e+6, -1e+6);
	uint32 pointSize = (uint32)points.size();
	for(uint32 i=0; i<pointSize; ++i)
	{
		if(points[i].x < minV.x)
			minV.x = points[i].x;
		if(points[i].y < minV.y)
			minV.y = points[i].y;
		if(points[i].z < minV.z)
			minV.z = points[i].z;

		if(points[i].x > maxV.x)
			maxV.x = points[i].x;
		if(points[i].y > maxV.y)
			maxV.y = points[i].y;
		if(points[i].z > maxV.z)
			maxV.z = points[i].z;
	}

	Vec3 boxCenter = 0.5f*(maxV + minV);
	
	AABB aabbBoundBox = AABB(minV - boxCenter,maxV - boxCenter);
	Matrix33 m33=Matrix33::CreateIdentity();
	OBB obb=OBB::CreateOBBfromAABB( m33,aabbBoundBox );
	g_pAuxGeom->DrawOBB(obb,boxCenter,0,cb[0],eBBD_Extremes_Color_Encoded);

	//------------------------------------------------------------------------------
	// Draw example motions

	static bool bFirstTime = true;
	static Ang3 arrAngles[MAX_LMG_ANIMS]; 
	if(bFirstTime)
	{
		bFirstTime = false;
		for (uint32 i=0; i<MAX_LMG_ANIMS; i++)
			arrAngles[i]=Ang3(ZERO);
	}

	for(int32 i=0; i<numExample; ++i)
	{
		if(i < numAsserts)
		{			
			AABB aabb1 = AABB(Vec3( -0.05f, -0.05f, -0.05f),Vec3( 0.05f, 0.05f, 0.05f));
			
			f32 fDelteTime=m_pInstance->m_fOriginalDeltaTime*20;
			arrAngles[i] += Ang3(fDelteTime,0.0f, 0.0f)*weights[i];

			if(arrAngles[i].x > 2*gf_PI)
				arrAngles[i].x -= 2*gf_PI;

			Matrix33 _m33=Matrix33::CreateRotationXYZ(arrAngles[i]);
			OBB _obb=OBB::CreateOBBfromAABB( _m33,aabb1 );
			g_pAuxGeom->DrawOBB(_obb,points[i],1,cb[i],eBBD_Extremes_Color_Encoded);

			//g_pAuxGeom->DrawSphere(points[i], 0.1f, cb[i]);
		}
		else
			g_pAuxGeom->DrawSphere(points[i], 0.03f, RGBA8(0xff, 0xff, 0x00, 0xff));
	}
}

//------------------------------------------------------------------------------
// Main function to calculate PMG weights during runtime
//------------------------------------------------------------------------------
SPU_NO_INLINE void CSkeletonAnim::WeightComputation_PMG(SParametric & pmg, CAnimationSet * pAnimationSet)
{
	const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(pmg.m_nParametricID);
	assert(pAnim->m_nAssetType==PMG_File);

	GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[pAnim->m_nGlobalAnimId];
	if( stricmp(rGlobalAnimHeader.GetFilePath(), "_PMG_Dummy_PATH") == 0)
	{
		pmg.m_fBlendWeight[0] = pmg.m_controlParam[0];
		pmg.m_fBlendWeight[1] = 1 - pmg.m_controlParam[0];
		return;
	}

	GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];	

	ColorB* cb = NULL;
	if ( gEnv->IsEditor() && Console::GetInst().ca_DrawPMGInfo ) 
	{
		// Color vector for output test 
		cb = (ColorB*)alloca(MAX_LMG_ANIMS*sizeof(ColorB));

		cb[0] = RGBA8(0xff,0x00,0x00,0xff);
		cb[1] = RGBA8(0xaf,0xe0,0x22,0xff);
		cb[2] = RGBA8(0xff,0xff,0xff,0xff);
		cb[3] = RGBA8(0xff,0xff,0x00,0xff);
		cb[4] = RGBA8(0x00, 0xff, 0xff, 0xff);
		cb[5] = RGBA8(0xff, 0x00, 0xff, 0xff);
		cb[6] = RGBA8(0x10, 0xff, 0x20, 0xff);
		cb[7] = RGBA8(0x00, 0x10, 0x21, 0xff);
		cb[8] = RGBA8(0x00, 0x11, 0xff, 0xff);
		cb[9] = RGBA8(0x99, 0x22, 0x00, 0xff);
		cb[10] = RGBA8(0xff, 0x42, 0x53, 0xff);
		cb[11] = RGBA8(0xee, 0xa0, 0x12, 0xff);
		cb[12] = RGBA8(0xff, 0x09, 0x89, 0xff);
		cb[13] = RGBA8(0x12, 0xaa, 0xcc, 0xff);
		cb[14] = RGBA8(0xee, 0x12, 0xcc, 0xff);
		cb[15] = RGBA8(0xee, 0xaa, 0x23, 0xff);
		cb[16] = RGBA8(0xee, 0x45, 0xcc, 0xff);
		cb[17] = RGBA8(0x90, 0xaa, 0xcc, 0xff);
		cb[18] = RGBA8(0xee, 0x10, 0xcc, 0xff);
		cb[19] = RGBA8(0xee, 0xaa, 0x56, 0xff);
		cb[20] = RGBA8(0x12, 0x12, 0xcc, 0xff);
		cb[21] = RGBA8(0x45, 0xf4, 0xcc, 0xff);
		cb[22] = RGBA8(0xee, 0xa6, 0xcc, 0xff);
		cb[23] = RGBA8(0x8e, 0xaa, 0xcc, 0xff);
		cb[24] = RGBA8(0xee, 0x9c, 0xdd, 0xff);
	}

	uint32 numAssets = rGlobalAnimHeader.m_arrBSAnimationsPMG.size();	
	uint32 N = rGlobalAnimHeader.m_paramSDI.N;
	uint32 D = rGlobalAnimHeader.m_paramSDI.D;
	f32* pParamAll = &rGlobalAnimHeader.m_paramSDI.params[0];
	f32* pA = &rGlobalAnimHeader.m_paramSDI.A[0];
	f32* pR = &rGlobalAnimHeader.m_paramSDI.R[0];
	f32* pScale = &rGlobalAnimHeader.m_paramSDI.Scale[0];

	f32* pParam = (f32*)alloca(D*sizeof(f32));
	fMatrix param(1, D, pParam);
	const std::map<string, uint32>& nameMap = rGlobalAnimHeader.m_paramNamePMGMap;
	uint32 sizeNames = nameMap.size();
	assert(D == sizeNames + 1);

#if !defined(__SPU__)
	std::vector< std::vector<f32> > visParams; // parameter space to be visualized
	if ( gEnv->IsEditor() && Console::GetInst().ca_DrawPMGInfo ) 
	{
		g_YLine += 10.0f;
		for (uint32 i=0; i<numAssets; ++i)
		{
			GlobalAnimationHeaderCAF& rsubGAH = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(pmg.m_nAnimID[i]) ]; 
			f32 fColor[4];
			fColor[0]= cb[i].r/255.0f;
			fColor[1]= cb[i].g/255.0f;
			fColor[2]= cb[i].b/255.0f;
			fColor[3]= cb[i].a/255.0f;

			f32 xPos = 1.0f;
			g_pIRenderer->Draw2dLabel( xPos,g_YLine, 1.3f, fColor, false,"%d", i);
			xPos += 10.0f;
			for(uint32 j=0; j<sizeNames; ++j)
			{
				g_pIRenderer->Draw2dLabel( xPos,g_YLine, 1.3f, fColor, false,"%s: %f", rGlobalAnimHeader.m_paramSpecPMG[j].name.c_str(), pParamAll[i*D + j]);
				xPos += 200.0f;
			}
			g_YLine+=0x10;
		}
	}

		// Read control parameters from pmg header
	for(uint32 j=0; j< sizeNames; ++j)
	{
		param(0, j) = pmg.m_controlParam[j] * rGlobalAnimHeader.m_paramSpecPMG[j].scale; 

		if ( gEnv->IsEditor() && Console::GetInst().ca_DrawPMGInfo ) 
		{
			std::vector<f32> t;
			for(uint32 k=0; k<N; ++k)
				t.push_back(pParamAll[k*D + j]);
			visParams.push_back(t);
		}

	}

	param( 0, D-1) = 1.0f;

	if ( gEnv->IsEditor() && Console::GetInst().ca_DrawPMGInfo ) 
	{
		f32 fColor[4] = {0,0,1,1};
		
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fColor, false,"controlParam: %f, %f, %f, %f, %f, %f", pmg.m_controlParam[0], pmg.m_controlParam[1], pmg.m_controlParam[2], pmg.m_controlParam[3],  pmg.m_controlParam[4], pmg.m_controlParam[5]);  
		g_YLine+=0x10;
	}
#endif

	//------------------------------------------------------------------------------
	// Only use nearest N examples

	float* pPdiff = (float*)alloca(D*sizeof(float));
	fMatrix Pdiff(1, D, pPdiff);

	/*f32* weightsTry = (f32*)alloca( N*sizeof(f32) );
	CSDI ami(N, D, pParamAll, pA, pR, pScale);
	ami.GetWeights(param.GetData(), weightsTry);*/

	std::vector<uint32> vipGuys;
	std::map<f32, uint32> distMap;
	for(uint32 i=0; i<N; ++i)
	{
		fMatrix paramI(1, D, pParamAll+i*D);
		Pdiff = param;
		Pdiff -= paramI;
		f32 d = Pdiff.SqrtDist();
		distMap[ d ] = i;
	}

	uint32 finallyUsedNum = 0;
	std::map<f32, uint32>::iterator it = distMap.begin();

	uint32 N_new = 0;
	if(D == 2)
		N_new = 2;
	else if(D == 3)
		N_new = 4;
	else if (D == 4)
		N_new = 8;

//	N_new=16;

	f32* pParam_new = (f32*)alloca( N_new*D*sizeof(f32) );
	f32* pA_new = (f32*)alloca( D*N_new*sizeof(f32) );
	f32* pR_new = (f32*)alloca( N_new*N_new*sizeof(f32) );
	f32* pScale_new = (f32*)alloca( N*sizeof(f32) );

	for(uint32 i=0; i< (uint32)distMap.size(); ++i)
	{
		if(finallyUsedNum >= N_new)
			break;

		uint32 ind = it->second;
		memcpy( pParam_new+i*D, pParamAll+ind*D, D*sizeof(f32) );

		++finallyUsedNum;
		++it;
	}

	assert(finallyUsedNum <= N_new);

	CSDI ami_new(finallyUsedNum, D, pParam_new, pA_new, pR_new, pScale_new);
	ami_new.ConstructSDI();
	f32* weightsTmp = (f32*)alloca( finallyUsedNum*sizeof(f32) );
	ami_new.GetWeights(param.GetData(), weightsTmp);

	f32* weights = (f32*) alloca( (numAssets + MAX_PMG_VIRTUAL_EXAMP) * sizeof(f32));
	memset(weights, 0, (numAssets+MAX_PMG_VIRTUAL_EXAMP)*sizeof(f32));

	it = distMap.begin();

	for(uint32 i=0; i<finallyUsedNum; ++i)
	{
		weights[it->second] = weightsTmp[i];
		++it;
	}


	//------------------------------------------------------------------------------
	// Please keep this code segment for future test
	//------------------------------------------------------------------------------
	// 
	// Get rid of examples with weights < 0.1

	/*f32* weightsTry = (f32*)alloca( N*sizeof(f32) );
	CSDI ami(N, D, pParamAll, pA, pR, pScale);
	ami.GetWeights(param.GetData(), weightsTry);

	std::vector<uint32> vipGuys;
	std::map<f32, uint32> distMap;
	for(uint32 i=0; i<N; ++i)
	{
		distMap[ weightsTry[i] ] = i;
	}

	uint32 finallyUsedNum = 0;
	std::map<f32, uint32>::iterator it = distMap.end();

	uint32 N_new = 4;
	f32* pParam_new = (f32*)alloca( N_new*D*sizeof(f32) );
	f32* pA_new = (f32*)alloca( D*N_new*sizeof(f32) );
	f32* pR_new = (f32*)alloca( N_new*N_new*sizeof(f32) );
	f32* pScale_new = (f32*)alloca( N*sizeof(f32) );

	for(uint32 i=0; i< (uint32)distMap.size(); ++i)
	{
		--it;
		if(fabs(it->first) < 0.1f && finallyUsedNum >= N_new)
			break;

		uint32 ind = it->second;
		memcpy( pParam_new+i*D, pParamAll+ind*D, D*sizeof(f32) );

		++finallyUsedNum;
	}

	assert(finallyUsedNum <= N_new);

	CSDI ami_new(finallyUsedNum, D, pParam_new, pA_new, pR_new, pScale_new);
	ami_new.ConstructSDI();
	f32* weightsTmp = (f32*)alloca( finallyUsedNum*sizeof(f32) );
	ami_new.GetWeights(param.GetData(), weightsTmp);

	f32* weights = (f32*) alloca(N * sizeof(f32));
	memset(weights, 0, N*sizeof(f32));

	it = distMap.end();

	for(uint32 i=0; i<finallyUsedNum; ++i)
	{
		--it;
		weights[it->second] = weightsTmp[i];
	}*/


	//------------------------------------------------------------------------------
	// Use this code for one-step SDI

	//f32* weights = (f32*)alloca( N*sizeof(f32) );
	//CSDI ami(N, D, pParamAll, pA, pR, pScale);
	//ami.GetWeights(param.GetData(), weights);

	//------------------------------------------------------------------------------
	// Deal with virtualExamples

	if( !rGlobalAnimHeader.m_virtualExmps.empty() )
	{
		const DynArray<GlobalAnimationHeaderPMG::SVirtualExamp>& virtualExamp = rGlobalAnimHeader.m_virtualExmps;
		uint32 size = (uint32)virtualExamp.size();
		for(uint32 i=0; i<size; ++i)
		{
			uint32 d1 = virtualExamp[i].ind1;
			uint32 d2 = virtualExamp[i].ind2;

			if(d1 >= numAssets)
			{
				uint32 t1 = virtualExamp[d1].ind1;
				uint32 t2 = virtualExamp[d1].ind2;
				f32 alpha1 = virtualExamp[d1].alpha;

				weights[t1] += alpha1 * virtualExamp[i].alpha * weights[numAssets + i];
				weights[t2] += (1-alpha1) * virtualExamp[i].alpha * weights[numAssets + i];
			}

			if(d2 >= numAssets)
			{
				uint32 t1 = virtualExamp[d2].ind1;
				uint32 t2 = virtualExamp[d2].ind2;
				f32 alpha1 = virtualExamp[d2].alpha;

				weights[t1] += alpha1 * (1-virtualExamp[i].alpha) * weights[numAssets + i];
				weights[t2] += (1-alpha1) * (1-virtualExamp[i].alpha) * weights[numAssets + i];
			}

			if(d1 >=0 && d1 <numAssets && d2 >= 0 && d2 < numAssets)
			{
				weights[ d1 ] += weights[numAssets + i] * virtualExamp[i].alpha;
				weights[ d2 ] += weights[numAssets + i] * (1-virtualExamp[i].alpha);
			}
		}
	}


	f32 sumWeights = .0f;
	for(uint32 i=0; i<numAssets; ++i)
	{
		sumWeights += weights[i];
	}

	f32* synthesizedParam = NULL;
	if ( gEnv->IsEditor() && Console::GetInst().ca_DrawPMGInfo ) 
	{
		synthesizedParam = (f32*)alloca((D-1) * sizeof(f32));
		memset( synthesizedParam, 0, sizeof(f32)*(D-1) );
	}

	f32 sumFinalWeights = .0f;

	for(uint32 i=0; i<numAssets; ++i)
	{		
		f32 w = weights[i];
		SmoothCD(pmg.m_fBlendWeight[i], pmg.m_fWeightsSmoothRate[i], m_pInstance->m_fOriginalDeltaTime, w, 0.07f);
		sumFinalWeights += pmg.m_fBlendWeight[i];		
	}

	for(uint32 i=0; i<numAssets; ++i)
	{
		pmg.m_fBlendWeight[i] = pmg.m_fBlendWeight[i]/sumFinalWeights;
		
		if ( gEnv->IsEditor() && Console::GetInst().ca_DrawPMGInfo ) 
		{
			for(uint32 j=0; j<D-1; ++j)
				synthesizedParam[j] += pmg.m_fBlendWeight[i]*pParamAll[i*D + j];
			f32 fColor[4];
			fColor[0]= cb[i].r/255.0f;
			fColor[1]= cb[i].g/255.0f;
			fColor[2]= cb[i].b/255.0f;
			fColor[3]= cb[i].a/255.0f;

			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"weight[%d]: %f", i, pmg.m_fBlendWeight[i]);
			g_YLine+=0x10;
		}
	}

#if !defined(__SPU__)
	if ( gEnv->IsEditor() && Console::GetInst().ca_DrawPMGInfo ) 
	{
		f32 fColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};

		f32 xPos = 1.0f;
		g_pIRenderer->Draw2dLabel( xPos,g_YLine, 1.3f, fColor, false,"Final parameter:");
		xPos += 130.0f;
		for(uint32 j=0; j<sizeNames; ++j)
		{
			g_pIRenderer->Draw2dLabel( xPos,g_YLine, 1.3f, fColor, false,"%s: %f", rGlobalAnimHeader.m_paramSpecPMG[j].name.c_str(), synthesizedParam[j]);
			xPos += 200.0f;
		}

		Vec3 controlP( .0f, .0f, .0f);
		if(param.GetSize() >= 2)
		{
			controlP.x = param(0, 0);
		}

		if(param.GetSize() >= 3)
		{
			controlP.y = param(0, 1);
		}

		if(param.GetSize() >= 4)
		{
			controlP.z = param(0, 2);
		}

		if ( gEnv->IsEditor() && Console::GetInst().ca_DrawPMGInfo ) 
			TestDrawParams( visParams, cb, controlP, numAssets, pmg.m_fBlendWeight);
	}
#endif 

}

CSkeletonAnim::TWarp CSkeletonAnim::GetTimewarpedDuration_PMG(CAnimationSet* pAnimationSet, SParametric& pmg)
{
	TWarp tw;
	if (pmg.m_nAnimID[0]<0)
		return tw;

	const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(pmg.m_nParametricID);
	assert(pAnim->m_nAssetType==PMG_File);
	GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[pAnim->m_nGlobalAnimId];

	for (int32 i=0; i<pmg.m_numAnims; i++)
	{
		int32 GlobalID = pAnimationSet->GetGlobalIDByAnimID_Fast(pmg.m_nAnimID[i]);
		int32 segcount = pmg.m_nSegmentCounter[i];
		GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[ GlobalID ];
		f32 fSegTime = rGAH.GetSegmentDuration(segcount);
		f32 fTotTime = rGAH.m_fTotalDuration;

		if (fSegTime != fTotTime)
		{
			f32 d0 = rGAH.GetSegmentDuration(segcount);
			tw.m_fDuration	+= d0*pmg.m_fBlendWeight[i];
			//	tw.m_fDistance	= 0;
			//	tw.m_fSpeed			= 0;
			//TODO: temp solution because we want only the duration
			tw.m_fDistance  += pmg.m_fBlendWeight[i]*tw.m_fDuration;
			tw.m_fSpeed		  += pmg.m_fBlendWeight[i];
		}
		else
		{
			tw.m_fDuration	+= pmg.m_fBlendWeight[i]*rGAH.m_fTotalDuration;
			tw.m_fDistance	+= pmg.m_fBlendWeight[i]*rGAH.m_fDistance;
			tw.m_fSpeed			+= pmg.m_fBlendWeight[i]*rGAH.m_fSpeed;
		}
	}
	// 	assert(fDuration);
	return tw;
}

SPU_NO_INLINE void CSkeletonAnim::ComputeBlendWeights( CAnimation *arrAnimFiFo, uint32 numAnims, f32 fDeltaTime, uint32 nLayer )
{
	for (uint32 a=0; a<numAnims; a++)
	{
		CAnimation &rCurAnim = arrAnimFiFo[a];
		assert(rCurAnim.m_bActivated);
		if (nLayer == 0)
			rCurAnim.m_fCurrentPlaybackSpeed = rCurAnim.m_Parametric.m_params[eMotionParamID_TravelSpeed].blendspace.m_fProceduralScale;
		else
			rCurAnim.m_fCurrentPlaybackSpeed = 1.0f;

		rCurAnim.m_fCurrentPlaybackSpeed *= rCurAnim.m_AnimParams.m_fPlaybackSpeed;

		// Here the new blendspace is converted into the old blendspace
		//UpdateOldMotionBlendSpace(rCurAnim.m_LMG0, m_pInstance->m_fDeltaTime, rCurAnim.m_fTransitionWeight);
		//UpdateOldMotionBlendSpace(rCurAnim.m_LMG1, m_pInstance->m_fDeltaTime, rCurAnim.m_fTransitionWeight);

		CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;

		/*
		uint32 IsPMG=0;
		if (rCurAnim.m_Parametric.m_nParametricID>=0)
		{
		GlobalAnimationHeaderLMG* rGlobalAnimHeader = pAnimationSet->GetGAH_LMG(rCurAnim.m_Parametric.m_nParametricID);
		if (rGlobalAnimHeader)
		IsPMG = rGlobalAnimHeader->IsAssetPMG();
		}*/

		GlobalAnimationHeaderPMG* rGlobalAnimHeaderPMG=0;
		if (rCurAnim.m_Parametric.m_nParametricID>=0)
			rGlobalAnimHeaderPMG = pAnimationSet->GetGAH_PMG(rCurAnim.m_Parametric.m_nParametricID);

		if (rGlobalAnimHeaderPMG)
		{
#if !defined(__SPU__) // exluded from spu since not used in game code and uses heap allocation
			WeightComputation_PMG(rCurAnim.m_Parametric, pAnimationSet);
			CheckBlendWeights_PMG(pAnimationSet,rCurAnim.m_Parametric);
			TWarp TW0 = GetTimewarpedDuration_PMG(pAnimationSet,rCurAnim.m_Parametric);
			rCurAnim.m_fCurrentDuration = TW0.m_fDuration;
#endif
		}
		else 
		{
			LMG::ComputeWeight(pAnimationSet, rCurAnim.m_Parametric);
			LMG::CheckBlendWeights(pAnimationSet, rCurAnim.m_Parametric);
			LMG::TW TW0 = LMG::GetTimewarpedDuration(pAnimationSet,rCurAnim.m_Parametric);
			rCurAnim.m_fCurrentDuration = TW0.m_fDuration;
			if ( fabs(TW0.m_fSpeed) > (float)1e-10 )
				rCurAnim.m_fCurrentDuration = TW0.m_fDistance/TW0.m_fSpeed;
		}

		f32 SpeedUp = rCurAnim.m_fCurrentPlaybackSpeed;
		f32 fDuration = rCurAnim.m_fCurrentDuration;
		if (fDuration==0.0f) //time-warping can produce positive and nagative durations. zero is also possible
			fDuration=0.000001f; //a rare case
		rCurAnim.m_fCurrentDeltaTime = (fDeltaTime/fDuration)*SpeedUp;
	}
}

SPU_NO_INLINE void CSkeletonAnim::AdjustAnimationTimeForTimeWarpedAnimations( CAnimation *arrAnimFiFo, uint32 numAnims )
{
	//---------------------------------------------------------------------------------------
	//---    In case animations are time-warped,  we have to adjust the ANIMATION-TIME    ---
	//---                 in relation to the TRANSITION-WEIGHTS                           ---
	//---------------------------------------------------------------------------------------	
	uint8 twflag=1;
	arrAnimFiFo[0].m_bTWFlag=0;
	for (uint32 i=1; i<numAnims; i++)
	{
		CAnimation &rPrevAnimation = arrAnimFiFo[i-1];
		CAnimation &rCurAnimation = arrAnimFiFo[i];

		uint32 timewarp = rCurAnimation.m_AnimParams.m_nFlags & CA_TRANSITION_TIMEWARPING;

		if (timewarp)
		{
			rPrevAnimation.m_bTWFlag=twflag;
			rCurAnimation.m_bTWFlag=twflag;
		}
		else
		{
			twflag++;
			rCurAnimation.m_bTWFlag=0;
		}
	}


	f32 fTransitionDelta=0;
	f32 fTransitionWeight=0;
	uint32 start=0;
	uint32 accumented=0;
	for (uint32 i=0; i<(numAnims+1); i++)
	{
		CAnimation &rCurAnimation = arrAnimFiFo[i];

		if (i<numAnims && rCurAnimation.m_bTWFlag)
		{
			if (accumented==0) start=i;

			fTransitionWeight += rCurAnimation.m_fTransitionWeight;
			fTransitionDelta	+= rCurAnimation.m_fCurrentDeltaTime*rCurAnimation.m_fTransitionWeight;
			accumented++;
		} 
		else 
		{
			f32 tt=0.0f;
			if (fTransitionWeight) tt=fTransitionDelta/fTransitionWeight;

			//all time-warped animation will get the same delta-time 
			for (uint32 a=start; a<(start+accumented); a++)
				arrAnimFiFo[a].m_fCurrentDeltaTime=tt;
		}
	}

}





SPU_NO_INLINE void CSkeletonAnim::UpdateAnimTimeAndExecBuffer( CAnimation *arrAnimFiFo, uint32 numAnims, uint32 nLayer, uint32 NumAnimsInQueue )
{
	//update anim-time and push them into the exec-buffer
	for (uint32 a=0; a<numAnims; a++)
	{
		CAnimation &rCurAnimation = arrAnimFiFo[a];

		assert(rCurAnimation.m_bActivated);

		UpdateAndPushIntoExecBuffer( arrAnimFiFo, nLayer, NumAnimsInQueue,a );
		AnimCallback(arrAnimFiFo,a,numAnims);

		f32 rad = Ang3::CreateRadZ( Vec2(0,1),rCurAnimation.m_Parametric.m_BlendSpace.m_strafe );
		SmoothCD(rCurAnimation.m_Parametric.m_BlendSpace.m_fAllowLeaningSmooth, rCurAnimation.m_Parametric.m_BlendSpace.m_fAllowLeaningSmoothRate, m_pInstance->m_fOriginalDeltaTime, f32(fabsf(rad)<0.02f), 0.25f);

		//	f32 fColor[4] = {1,1,0,1};
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"lmg.m_BlendSpace.m_fAllowLeaningSmooth:%f  DEG: %f",arrAFIFO[a].m_LMG0.m_BlendSpace.m_fAllowLeaningSmooth, RAD2DEG(0.01f) );	g_YLine+=0x20;

#if defined(__SPU__)
		if (nLayer != 0 )
			memtransfer_to_main( &m_arrLayer_AFIFO[nLayer][a], &gAninmationFifo[a], sizeof(CAnimation), ANIM_BACK_TRANSFER );
#endif

	}
}


SPU_NO_INLINE void CSkeletonAnim::CheckBlendWeights_PMG(CAnimationSet* pAnimationSet, SParametric& pmg)
{
	if (pmg.m_nParametricID>=0)
	{
		const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(pmg.m_nParametricID);
		assert(pAnim->m_nAssetType==PMG_File);
		GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[pAnim->m_nGlobalAnimId];

		f32 fTotal=0.0f;
		uint32 numAssets = rGlobalAnimHeader.m_arrBSAnimationsPMG.size();
		for (uint32 i=0; i<numAssets; i++)
			fTotal += pmg.m_fBlendWeight[i];
		assert( fabsf(fTotal-1.0f)<0.005f );

		uint32 nExtrapolation1=0;
		for (uint32 i=0; i<numAssets; i++)
			nExtrapolation1 |= uint32(pmg.m_fBlendWeight[i]<-5.2f);

		uint32 nExtrapolation2=0;
		for (uint32 i=0; i<numAssets; i++)
			nExtrapolation2 |= uint32(pmg.m_fBlendWeight[i]>5.2f);

		if( fabsf(fTotal-1.0f)>0.005f || nExtrapolation1 || nExtrapolation2 )
		{
			assert(!"Sum of weights is wrong");
			//	char blendCode[5];
			//	blendCode[0] = (rGlobalAnimHeader.m_nBlendCodeLMG >>  0) & 0xff;
			//	blendCode[1] = (rGlobalAnimHeader.m_nBlendCodeLMG >>  8) & 0xff;
			//	blendCode[2] = (rGlobalAnimHeader.m_nBlendCodeLMG >> 16) & 0xff;
			//	blendCode[3] = (rGlobalAnimHeader.m_nBlendCodeLMG >> 24) & 0xff;
			//	blendCode[4] = 0;
#if !defined(__SPU__)
			CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "Sum of weights is wrong for PMG");
#endif
		}
	}
}
