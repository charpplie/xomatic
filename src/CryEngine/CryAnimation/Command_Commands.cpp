#include "stdafx.h"

#include "Helper.h"

#include "ControllerOpt.h"

#include "CharacterInstance.h"
#include "SkeletonAnim.h"
#include "SkeletonPose.h"

#include "Command_Buffer.h"
#include "Command_Commands.h"

#include "Skeleton.h"

uint32 g_SkeletonUpdates=0;

namespace Command {

void LoadControllers(GlobalAnimationHeaderCAF& rGAH, const Command::CState& state, IController* controllers[MAX_JOINT_AMOUNT])
{
	uint32 num=sizeof(controllers);
	memset(controllers, 0, sizeof(controllers)*MAX_JOINT_AMOUNT);

//	if (rGAH.m_nControllers != rGAH.m_nControllers2)
	{
		if (rGAH.m_nControllers==0)
		{
#if !defined(__SPU__)
			//this part will be replaced by a CryFatalErrors()
			//float fColor[4] = {1,1,0,1};
			//g_pIRenderer->Draw2dLabel( 1,g_YLine, 3.3f, fColor, false,"WTF: %s",rGAH.m_FilePath.c_str() );	g_YLine+=16.0f;

			// Disable the following spam until the actual issue is solved in Engine (it seems to give a lot of 'false positives')
			//g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, 0 , "No Controllers found in Asset: %s" ,rGAH.m_FilePath.c_str() );
#endif
			return;
		}
	}

	uint32* pLodJointMask = NULL;
	uint32 lodJointMaskCount = 0;
	if (state.m_lod > 0)
	{
		if (uint32 lodCount = state.m_pModel->m_arrAnimationLOD.size())
		{
			uint32 lod = state.m_lod;
			if (lod > lodCount)
				lod = lodCount;
			--lod;

			pLodJointMask = &state.m_pModel->m_arrAnimationLOD[lod][0];
			lodJointMaskCount = state.m_pModel->m_arrAnimationLOD[lod].size();
		}
	}

	const CModelJoint* pModelJoint = &state.m_pModel->m_ModelSkeleton.m_arrModelJoints[0];
	uint32 jointCount = state.m_jointCount;
	for (uint32 i=0; i<jointCount; ++i)
	{
		uint32 crc32 = pModelJoint[i].m_nJointCRC32;
		if (pLodJointMask)
		{
			if (Helper::FindFromSorted<uint32>(pLodJointMask, lodJointMaskCount, crc32) == NULL)
				continue;
		}

		if (!state.IsJointActive(crc32))
			continue;

		controllers[i] = rGAH.GetControllerByJointCRC32( pModelJoint[i].m_nJointCRC32 );
	}
}

SPU_NO_INLINE void ClearFull::Execute(const CState& state, void* buffers[]) const
{
	const ClearFull& ac = *this;
	void** CBTemp = buffers;

#if !defined(__SPU__)
	//	float fColor[4] = {0,1,0,1};
	extern float g_YLine;
#endif // !__SPU__

	assert( ac.m_TargetBuffer<=Command::TargetBuffer );
	QuatT*		parrRelPoseDst	= SPU_LOCAL_PTR( (QuatT*)  CBTemp[ac.m_TargetBuffer+0] );
	Status4*	parrStatusDst		= SPU_LOCAL_PTR( (Status4*)CBTemp[ac.m_TargetBuffer+1] );

	Skeleton::CLocator*	pLocatorDst			= SPU_LOCAL_PTR( (Skeleton::CLocator*)CBTemp[ac.m_TargetBuffer+2] );

	pLocatorDst->Initialize();
	uint32 numJoints = state.m_jointCount;
	for (uint32 j=0; j<numJoints; j++)
	{
		parrRelPoseDst[j].q.v.x = 0.0f;
		parrRelPoseDst[j].q.v.y = 0.0f;
		parrRelPoseDst[j].q.v.z = 0.0f;
		parrRelPoseDst[j].q.w   = 0.0f;
		parrRelPoseDst[j].t.x		= 0.0f;
		parrRelPoseDst[j].t.y		= 0.0f;
		parrRelPoseDst[j].t.z		= 0.0f;
		parrStatusDst[j].o = 1;
		parrStatusDst[j].p = 1;
		parrStatusDst[j].s = 0;
	} 
}

SPU_NO_INLINE void ClearSingle::Execute(const CState& state, void* buffers[]) const
{
	const ClearSingle& ac = *this;
	void** CBTemp = buffers;

	assert( ac.m_TargetBuffer<=Command::TargetBuffer );
	QuatT*		parrRelPoseDst	= SPU_LOCAL_PTR( (QuatT*)  CBTemp[ac.m_TargetBuffer+0] );
	Status4*	parrStatusDst		= SPU_LOCAL_PTR( (Status4*)CBTemp[ac.m_TargetBuffer+1] );
	Skeleton::CLocator*	pLocatorDst			= SPU_LOCAL_PTR( (Skeleton::CLocator*)CBTemp[ac.m_TargetBuffer+2] );

	pLocatorDst->Initialize();

	parrRelPoseDst[0].q.v.x = 0.0f;
	parrRelPoseDst[0].q.v.y = 0.0f;
	parrRelPoseDst[0].q.v.z = 0.0f;
	parrRelPoseDst[0].q.w   = 0.0f;
	parrRelPoseDst[0].t.x		= 0.0f;
	parrRelPoseDst[0].t.y		= 0.0f;
	parrRelPoseDst[0].t.z		= 0.0f;
}

SPU_NO_INLINE void SampleAddAnimFull::Execute(const CState& state, void* buffers[]) const
{
	const SampleAddAnimFull& ac = *this;
	void** CBTemp = buffers;

#if !defined(__SPU__)
	float fColor[4] = {0,1,0,1};
	extern float g_YLine;
#endif // !__SPU__

	assert( ac.m_nEAnimID>=0   );
	int32 nBufferID=(ac.m_flags&Flag_TmpBuffer)?0:3;
	QuatT*		parrRelPoseDst	= SPU_LOCAL_PTR( (QuatT*)  CBTemp[nBufferID+0] );
	Status4*	parrStatusDst		= SPU_LOCAL_PTR( (Status4*)CBTemp[nBufferID+1] );
	Skeleton::CLocator*	pLocatorDst			= SPU_LOCAL_PTR( (Skeleton::CLocator*)CBTemp[nBufferID+2] );

	uint32 numJoints = state.m_jointCount;
	Status4 &getOPResult = parrStatusDst[0];
#if 0
#if !defined(__SPU__)
	uint32 nAnimationLOD = state.m_lod;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"nAnimationLOD: %d",nAnimationLOD);	
	g_YLine+=10.0f;
#endif // !__SPU__
#endif // #if 0

	CAnimationSet* pAnimationSet = &state.m_pModel->m_AnimationSet;
	const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(ac.m_nEAnimID);
	assert(pMAG);
	int32 nEGlobalID = pMAG->m_nGlobalAnimId;
	assert(pMAG->m_nAssetType==CAF_File);

#if !defined(__SPU__)
	assert(nEGlobalID>=0);
	if (nEGlobalID<0)
	{
		AnimFileWarning(state.m_pModel->GetModelFilePath(),"illegal index in RSingleEvaluation:  index: %d",nEGlobalID);
	}
#endif // !__SPU__

	// use a stack object on SPU here to reduce simulation time
	SpuStackValue<GlobalAnimationHeaderCAF, true, true> stackAnimHeader( g_AnimationManager.m_arrGlobalCAF[nEGlobalID] );
	GlobalAnimationHeaderCAF& rGlobalAnimHeaderCAF = stackAnimHeader; 

#ifdef _DEBUG
	if (rGlobalAnimHeaderCAF.IsAssetOnDemand())
	{
		assert(rGlobalAnimHeaderCAF.IsAssetLoaded());
	}
#endif

	InitSpuControllerLookupTable( rGlobalAnimHeaderCAF.m_arrControllerLookupVector );
	rGlobalAnimHeaderCAF.m_nTouchedCounter++;
/*
	SFootPlant& rFootPlantVectors = rGlobalAnimHeaderCAF.m_FootPlantVectors;

	uint32 numPoses=rGlobalAnimHeaderCAF.m_FootPlantBits.size();
	if (numPoses)
	{
		uint32 idx = min(uint32(ac.m_fETimeNew*(numPoses-1)), numPoses-1);
		pLocatorDst->m_nFootBits=rGlobalAnimHeaderCAF.m_FootPlantBits[idx];
	}
*/
	memset( parrStatusDst,0xff,numJoints );

	IController* parrController[MAX_JOINT_AMOUNT];
	LoadControllers(rGlobalAnimHeaderCAF, state, parrController);

	QuatT qtemp[MAX_JOINT_AMOUNT];
	QuatT* parrDefJoints = &state.m_pModel->m_ModelSkeleton.m_poseData.m_jointsRelative[0];
	{
		DEFINE_PROFILER_SECTION("cryMemcpy");
		cryMemcpy( &qtemp[0], parrDefJoints,numJoints*sizeof(QuatT) );
	}

	f32 fKeyTime1 =rGlobalAnimHeaderCAF.NTime2KTime(1);
	f32 fKeyTimeNew =rGlobalAnimHeaderCAF.NTime2KTime(ac.m_fETimeNew);
	f32 fKeyTimeOld = rGlobalAnimHeaderCAF.NTime2KTime(ac.m_fETimeOld);

	uint32 nStartJoint=0;
	uint32 nADM = ac.m_flags & Flag_ADMotion;
	if ( parrController[0] && nADM )
	{
		nStartJoint=1;
		//-------------------------------------------------------------------------------------
		//----                          Trajectory Extraction                             -----
		//-------------------------------------------------------------------------------------

		QuatT _new; _new.SetIdentity();
		QuatT _old; _old.SetIdentity();

		if (ac.m_flags & Flag_AnimEOC)
		{
			QuatT EndKey;
			getOPResult = SPU_MAIN_PTR(parrController[0])->GetOP( fKeyTime1, EndKey.q, EndKey.t );
			if ((EndKey.q.v|EndKey.q.v) < 0.0001f)
				EndKey.q.SetIdentity();

			f32 TimeFlow  = state.m_timeDelta;
			if (TimeFlow<0)
			{
				//time moves backwards
				getOPResult = SPU_MAIN_PTR(parrController[0])->GetOP( fKeyTimeOld, _old.q, _old.t );
				if ((_old.q.v|_old.q.v) < 0.0001f)
					_old.q.SetIdentity();
				_old=EndKey*_old;
			}
			else
			{
				//time moves forward
				getOPResult = SPU_MAIN_PTR(parrController[0])->GetOP( fKeyTimeOld, _old.q, _old.t );
				if ((_old.q.v|_old.q.v) < 0.0001f)
					_old.q.SetIdentity();
				_old=EndKey.GetInverted()*_old;
			}
		}
		else
		{
			getOPResult = SPU_MAIN_PTR(parrController[0])->GetOP( fKeyTimeOld, _old.q, _old.t );
			if ((_old.q.v|_old.q.v) < 0.0001f)
				_old.q.SetIdentity();
		}

		getOPResult = SPU_MAIN_PTR(parrController[0])->GetOP( fKeyTimeNew, _new.q, _new.t );
		if ((_new.q.v|_new.q.v) < 0.0001f)
			_new.q.SetIdentity();

#ifdef _DEBUG
//		QuatT test;
//		ControllerGetOP( parrController[0], nEGlobalID, 0.0f, test.q, test.t );
//		assert(test.IsEquivalent(IDENTITY,0.001f));
#endif

		parrRelPoseDst[0].SetIdentity();
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"root: %f (%f %f %f)",_new.q.w, _new.q.v.x,_new.q.v.y,_new.q.v.z  );
		//	g_YLine+=16.0f;

		_old.q.v.x=0; _old.q.v.y=0; _old.q.Normalize(); 
		_new.q.v.x=0; _new.q.v.y=0; _new.q.Normalize(); 
		const Vec3 pDir = _old.q.GetColumn1();
		const Vec3 cDir = _new.q.GetColumn1();
		pLocatorDst->m_rotationRelative += ac.m_fWeight*Ang3::CreateRadZ(pDir, cDir);
		pLocatorDst->m_translationRelative += ac.m_fWeight*((_old.t-_new.t)*_new.q);						//inverse multiplication to linearize the motion path

		if ((ac.m_flags&Flag_IsPMG)==0) 
		{
			pLocatorDst->m_speed += ac.m_fWeight*rGlobalAnimHeaderCAF.m_fSpeed;
			pLocatorDst->m_turn += ac.m_fWeight*rGlobalAnimHeaderCAF.m_fTurnSpeed;
			pLocatorDst->m_slope += ac.m_fWeight*rGlobalAnimHeaderCAF.m_fSlope;
			pLocatorDst->m_strafe += 0.0f;
			pLocatorDst->m_velocity += ac.m_fWeight*rGlobalAnimHeaderCAF.m_vVelocity;
		}
		else
		{
#if !defined(__SPU__) // Rendere output is not supportet on SPU
			uint32 t0 = rGlobalAnimHeaderCAF.m_arrLocoMoveSpeedPMG.size();
			uint32 t1 = rGlobalAnimHeaderCAF.m_arrLocoTurnSpeedPMG.size();
			uint32 t2 = rGlobalAnimHeaderCAF.m_arrLocoHorizAnglePMG.size();
			assert(t0);	assert(t1);	assert(t2);
			assert(t0==t1 && t1==t2);

			uint32 numKeys=rGlobalAnimHeaderCAF.m_arrLocoMoveSpeedPMG.size()-1;
			f32 floatKey = ac.m_fETimeNew*(numKeys+1);
			uint32 key0 = uint32(floatKey); //floor
			uint32 key1 = key0+1;
			if(key0 >= (numKeys))
				key0 = (numKeys);
			if(key1 >= (numKeys))
				key1 = (numKeys);
			f32 tBlend = floatKey-f32(key0);


			f32	fLocatorSpeed = rGlobalAnimHeaderCAF.m_arrLocoMoveSpeedPMG[key0]*(1-tBlend) + rGlobalAnimHeaderCAF.m_arrLocoMoveSpeedPMG[key1]*tBlend;
			//		f32	fLocatorTurn = rGlobalAnimHeader.m_arrLocoTurnSpeedPMG[key0]*(1-tBlend) + rGlobalAnimHeader.m_arrLocoTurnSpeedPMG[key1] * (tBlend);
			f32	fLocatorTurn = rGlobalAnimHeaderCAF.m_arrLocoTurnSpeedPMG[key1];
			Quat q0 = Quat::CreateRotationZ(rGlobalAnimHeaderCAF.m_arrLocoTurnSpeedPMG[key0]);
			Quat q1 = Quat::CreateRotationZ(rGlobalAnimHeaderCAF.m_arrLocoTurnSpeedPMG[key1]);
			Quat tangle; tangle.SetNlerp(q0,q1,tBlend);
			f32	fLocatorStrafe = rGlobalAnimHeaderCAF.m_arrLocoHorizAnglePMG[key0]*(1-tBlend) + rGlobalAnimHeaderCAF.m_arrLocoHorizAnglePMG[key1]*tBlend;

			f32	fLocatorSlope = rGlobalAnimHeaderCAF.m_arrLocoVertAnglePMG[key0]*(1-tBlend) + rGlobalAnimHeaderCAF.m_arrLocoVertAnglePMG[key0]*tBlend;

			// Use this code if you only need average parameters for the locater
			pLocatorDst->m_speed += ac.m_fWeight*fLocatorSpeed; 
			pLocatorDst->m_turn += ac.m_fWeight*fLocatorTurn;
			pLocatorDst->m_strafe += ac.m_fWeight*fLocatorStrafe;
			pLocatorDst->m_slope += ac.m_fWeight*fLocatorSlope;

			//calculate the velocity-vector	
			Vec3 Velocity = Matrix33::CreateRotationZ(pLocatorDst->m_strafe) * Vec3(0,pLocatorDst->m_speed,0);
			//			Vec3 Velocity = tangle * Vec3(0,pLocatorDst->m_fCurrentSpeed,0);
			pLocatorDst->m_velocity = Velocity;
			pLocatorDst->m_rotationRelative = pLocatorDst->m_turn * state.m_timeDelta;
			pLocatorDst->m_translationRelative = -pLocatorDst->m_velocity * state.m_timeDelta;

			/*float fColor2[4] = {1,1,0,1};
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor2, false,"RelTrans:(%f %f %f) length: %f  speed: %f  radiant: %f strafe: %f key0: %d", pLocatorDst->m_vCurrentVelocity.x,pLocatorDst->m_vCurrentVelocity.y,pLocatorDst->m_vCurrentVelocity.z,pLocatorDst->m_vCurrentVelocity.GetLength(), pLocatorDst->m_fCurrentSpeed, pLocatorDst->m_fCurrentTurn, pLocatorDst->m_fCurrentStreife,key0);	
			g_YLine+=16.0f;*/

			//	float fColor1[4] = {0,1,0,1};
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor1, false,"numKeys: %d  fLocatorMoveSpeedY: %f  fLocatorTurnSpeedZ: %f", numKeys,pLocatorDst->m_fCurrentSpeed, pLocatorDst->m_fCurrentTurn );	
			//	g_YLine+=16.0f;
#endif
		}

		//---------------------------------------------------------------------------
/*
		if (state.m_bFootPlants)
			pLocatorDst->m_FootPlant = rFootPlantVectors;
*/
		//-----------------------------------------------------------------------------

#ifdef _DEBUG
		Quat q = parrRelPoseDst[0].q;
		Vec3 t = parrRelPoseDst[0].t;
		assert(q.IsEquivalent(IDENTITY,0.001f));
		assert(t.IsEquivalent(ZERO,0.001f));
#endif

	}
	else
	{
		pLocatorDst->m_speed = 0.0f;
		pLocatorDst->m_turn = 0.0f;
		pLocatorDst->m_slope = 0.0f;
		pLocatorDst->m_strafe = 0.0f;
		pLocatorDst->m_velocity = Vec3(ZERO);
	}

	//-------------------------------------------------------------------------------------
	//----             evaluate all controllers for this animation                    -----
	//-------------------------------------------------------------------------------------
	for (uint32 j=nStartJoint; j<numJoints; j++)
	{
		if (parrController[j])
		{
			SPU_MAIN_PTR(parrController[j])->GetOP( fKeyTimeNew, qtemp[j].q, qtemp[j].t );	
			//this could be optimized at loading-time
			qtemp[j].q *= fsgnnz(parrDefJoints[j].q|qtemp[j].q);
	//		f32 dot=parrDefJoints[j].q|qtemp[j].q;
	//		if (dot<0)
	//			qtemp[j].q=-qtemp[j].q;
		}
		parrRelPoseDst[j].q   += ac.m_fWeight*qtemp[j].q;
		parrRelPoseDst[j].t   += ac.m_fWeight*qtemp[j].t;
	} 

#if !defined(__SPU__)
g_SkeletonUpdates++;
#endif // __SPU__
}

SPU_NO_INLINE void SampleAddPoseFull::Execute(const CState& state, void* buffers[]) const
{
	const SampleAddPoseFull& ac = *this;
	void** CBTemp = buffers;

#if !defined(__SPU__)
	float fColor[4] = {0,1,0,1};
	extern float g_YLine;
#endif // !__SPU__

	assert( ac.m_nEAnimID>=0   );
	int32 nBufferID=(ac.m_flags&SampleAddAnimFull::Flag_TmpBuffer)?0:3;
	QuatT*		parrRelPoseDst	= SPU_LOCAL_PTR( (QuatT*)  CBTemp[nBufferID+0] );
	Status4*	parrStatusDst		= SPU_LOCAL_PTR( (Status4*)CBTemp[nBufferID+1] );
	Skeleton::CLocator*	pLocatorDst			= SPU_LOCAL_PTR( (Skeleton::CLocator*)CBTemp[nBufferID+2] );

	uint32 numJoints = state.m_jointCount;
	Status4 &getOPResult = parrStatusDst[0];

	CAnimationSet* pAnimationSet = &state.m_pModel->m_AnimationSet;
	const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(ac.m_nEAnimID);
	assert(pMAG);
	int32 nEGlobalID = pMAG->m_nGlobalAnimId;
	assert(pMAG->m_nAssetType==AIM_File);

	// use a stack object on SPU here to reduce simulation time
	SpuStackValue<GlobalAnimationHeaderAIM, true, true> stackAnimHeader( g_AnimationManager.m_arrGlobalAIM[nEGlobalID] );
	GlobalAnimationHeaderAIM& rGlobalAnimHeaderAIM = stackAnimHeader; 

	memset( parrStatusDst,0xff,numJoints );
	const CModelJoint* pModelJoint = &state.m_pModel->m_ModelSkeleton.m_arrModelJoints[0];
	IController* parrController[MAX_JOINT_AMOUNT];
	memset(parrController, 0, sizeof(parrController));
	for (uint32 i=0; i<numJoints; ++i)
		parrController[i] = rGlobalAnimHeaderAIM.GetControllerByJointCRC32( pModelJoint[i].m_nJointCRC32 );

	QuatT qtemp[MAX_JOINT_AMOUNT];
	QuatT* parrDefJoints = &state.m_pModel->m_ModelSkeleton.m_poseData.m_jointsRelative[0];
	cryMemcpy( &qtemp[0], parrDefJoints,numJoints*sizeof(QuatT) );

	f32 fKeyTimeNew =rGlobalAnimHeaderAIM.NTime2KTime(ac.m_fETimeNew);

	//-------------------------------------------------------------------------------------
	//----             evaluate all controllers for this animation                    -----
	//-------------------------------------------------------------------------------------
	for (uint32 j=0; j<numJoints; j++)
	{
		if (parrController[j])
		{
			SPU_MAIN_PTR(parrController[j])->GetOP( fKeyTimeNew, qtemp[j].q, qtemp[j].t );	
			//this could be optimized at loading-time
			f32 dot=parrDefJoints[j].q|qtemp[j].q;
			if (dot<0)
				qtemp[j].q=-qtemp[j].q;
		}
		parrRelPoseDst[j].q   += ac.m_fWeight*qtemp[j].q;
		parrRelPoseDst[j].t   += ac.m_fWeight*qtemp[j].t;
	} 

}

SPU_NO_INLINE void CopyAddAnimFull::Execute(const CState& state, void* buffers[]) const
{
	const CopyAddAnimFull& ac = *this;
	void** CBTemp = buffers;

	assert(ac.m_SourceBuffer<=Command::TargetBuffer);
	QuatT*		parrRelPoseSrc		= SPU_LOCAL_PTR( (QuatT*)  CBTemp[ac.m_SourceBuffer+0]);
//	Status4*	parrStatusSrc			= SPU_LOCAL_PTR( (Status4*)CBTemp[ac.m_SourceBuffer+1]);
	Skeleton::CLocator*	pLocatorSrc				= SPU_LOCAL_PTR( (Skeleton::CLocator*)CBTemp[ac.m_SourceBuffer+2]);

	assert(ac.m_TargetBuffer<=Command::TargetBuffer);
	QuatT*		parrRelPoseDst		= SPU_LOCAL_PTR( (QuatT*)  CBTemp[ac.m_TargetBuffer+0]);
//	Status4*	parrStatusDst			= SPU_LOCAL_PTR( (Status4*)CBTemp[ac.m_TargetBuffer+1]);
	Skeleton::CLocator*	pLocatorDst				= SPU_LOCAL_PTR( (Skeleton::CLocator*)CBTemp[ac.m_TargetBuffer+2]);

	f32 t=ac.m_fWeight;

	pLocatorDst->m_speed += pLocatorSrc->m_speed*t;
	pLocatorDst->m_turn += pLocatorSrc->m_turn*t;
	pLocatorDst->m_slope += pLocatorSrc->m_slope*t;
	pLocatorDst->m_strafe += pLocatorSrc->m_strafe*t;

	pLocatorDst->m_velocity += pLocatorSrc->m_velocity*t;
	pLocatorDst->m_rotationRelative += pLocatorSrc->m_rotationRelative*t;
	pLocatorDst->m_translationRelative += pLocatorSrc->m_translationRelative*t;

	if (ac.m_IsPMG)
	{
		Vec3 Velocity = Matrix33::CreateRotationZ(pLocatorDst->m_strafe) * Vec3(0,pLocatorDst->m_speed,0);
		pLocatorDst->m_velocity = Velocity;
		pLocatorDst->m_rotationRelative = pLocatorDst->m_turn * state.m_timeDelta;
		pLocatorDst->m_translationRelative = -pLocatorDst->m_velocity * state.m_timeDelta;
	}
/*
	pLocatorDst->m_nFootBits					&=	pLocatorSrc->m_nFootBits;
	pLocatorDst->m_FootPlant					+=	pLocatorSrc->m_FootPlant*t;
*/

	//------------------------------------------------------------------------------------------------------
//	uint32 numAddJoints = sizeof(pLocatorDst->m_InitJoints)/sizeof(Quat);
//	for (uint32 i=0; i<numAddJoints; i++)
//		pLocatorDst->m_InitJoints[i]	%=  pLocatorSrc->m_InitJoints[i]*t;

	uint32 numJoints = state.m_jointCount;
	for (uint32 i=0; i<numJoints; i++)
	{
		parrRelPoseDst[i].q += parrRelPoseSrc[i].q*t;
		parrRelPoseDst[i].t += parrRelPoseSrc[i].t*t;
	}

}

SPU_NO_INLINE void NormalizeFull::Execute(const CState& state, void* buffers[]) const
{
	const NormalizeFull& ac = *this;
	void** CBTemp = buffers;

	assert(ac.m_TargetBuffer<=Command::TargetBuffer);
	QuatT*			parrRelPoseDst	= SPU_LOCAL_PTR((QuatT*)  CBTemp[ac.m_TargetBuffer+0]);
	Status4*		parrStatusDst		= SPU_LOCAL_PTR((Status4*)CBTemp[ac.m_TargetBuffer+1]);
	Skeleton::CLocator*	  pLocatorDst			= SPU_LOCAL_PTR((Skeleton::CLocator*)CBTemp[ac.m_TargetBuffer+2]);

	/*
	uint32 numAddJoints = sizeof(pLocatorDst->m_InitJoints)/sizeof(Quat);
	for (uint32 i=0; i<numAddJoints; i++)
		pLocatorDst->m_InitJoints[i].Normalize();
//	for (uint32 i=0; i<numAddJoints; i++)
//		pLocatorDst->m_InitJoints[i].SetIdentity();


	for (uint32 i=0; i<numAddJoints; i++)
	{
		float fColor2[4] = {1,1,0,1};
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor2, false,"InitJoints: %f (%f %f %f)",pLocatorDst->m_InitJoints[i].w,pLocatorDst->m_InitJoints[i].v.x,pLocatorDst->m_InitJoints[i].v.y,pLocatorDst->m_InitJoints[i].v.z);	
		g_YLine+=16.0f;
	}
*/

	pLocatorDst->m_speed *= ac.m_fRootScale;
	pLocatorDst->m_turn *= ac.m_fRootScale;
	pLocatorDst->m_slope *= ac.m_fRootScale;
	pLocatorDst->m_strafe *= ac.m_fRootScale;
	pLocatorDst->m_velocity *= ac.m_fRootScale;
	pLocatorDst->m_rotationRelative *= ac.m_fRootScale;
	pLocatorDst->m_translationRelative *= ac.m_fRootScale;

	f32 dot = fabsf(parrRelPoseDst[0].q|parrRelPoseDst[0].q);
	if (dot>0.0001f)
		parrRelPoseDst[0].q.Normalize();

	uint32 numJoints = state.m_jointCount;
	for (uint32 i=1; i<numJoints; i++)
		parrRelPoseDst[i].q.Normalize();
}

SPU_NO_INLINE void ScaleUniformFull::Execute(const CState& state, void* buffers[]) const
{
	const ScaleUniformFull& ac = *this;
	void** CBTemp = buffers;

	assert(ac.m_TargetBuffer<=Command::TargetBuffer);
	QuatT*			parrRelPoseDst	= SPU_LOCAL_PTR((QuatT*)    CBTemp[ac.m_TargetBuffer+0]);
	Status4*		parrStatusDst		= SPU_LOCAL_PTR((Status4*)  CBTemp[ac.m_TargetBuffer+1]);
	Skeleton::CLocator*	pLocatorDst			= SPU_LOCAL_PTR((Skeleton::CLocator*)CBTemp[ac.m_TargetBuffer+2]);
	pLocatorDst->m_translationRelative *= ac.m_fScale;
	uint32 numJoints = state.m_jointCount;
	for (uint32 j=0; j<numJoints; j++)
	{
		if (parrStatusDst[j].o)
			parrRelPoseDst[j].t *= ac.m_fScale;
	}
}

SPU_NO_INLINE void SampleAnimPart::Execute(const CState& state, void* buffers[]) const
{
	const SampleAnimPart& ac = *this;
	void** CBTemp = buffers;

	assert(ac.m_nEAnimID>=0);
	assert(ac.m_TargetBuffer<=Command::TargetBuffer);
	QuatT*			parrRelPoseDst	= SPU_LOCAL_PTR( (QuatT*)    CBTemp[ac.m_TargetBuffer+0] );
	Status4*		parrStatusDst		= SPU_LOCAL_PTR( (Status4*)  CBTemp[ac.m_TargetBuffer+1] );

	SampleFeatherAnimPart* pFeatherMask=0;
	if (ac.m_nFeatherBlend)
		pFeatherMask=(SampleFeatherAnimPart*)&ac;

	CAnimationSet* pAnimationSet = &state.m_pModel->m_AnimationSet;
	const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(ac.m_nEAnimID);
	assert(pMAG);
	int32 nEGlobalID = pMAG->m_nGlobalAnimId;
	assert(pMAG->m_nAssetType==CAF_File);

	// cache vars to local pointers (spares using this pointer everytime)
	uint32 numJoints = state.m_jointCount;
	assert(ac.m_fAnimTime>=0.0f && ac.m_fAnimTime<=1.0f);	


	GlobalAnimationHeaderCAF& rGlobalAnimHeaderCAF = SPU_MAIN_REF( g_AnimationManager.m_arrGlobalCAF[nEGlobalID] );
	InitSpuControllerLookupTable( rGlobalAnimHeaderCAF.m_arrControllerLookupVector );
	rGlobalAnimHeaderCAF.m_nTouchedCounter++;
	uint32 nAdditiveAnimation = rGlobalAnimHeaderCAF.IsAssetAdditive();

	IController* parrController[MAX_JOINT_AMOUNT];
	LoadControllers(rGlobalAnimHeaderCAF, state, parrController);

#if !defined(__SPU__)
	//	float fColor[4] = {0,1,0,1};
	extern float g_YLine;
#endif // !__SPU__

	f32 fKeyTimeNew = rGlobalAnimHeaderCAF.NTime2KTime(ac.m_fAnimTime);
	const CModelJoint* pModelJoint = &state.m_pModel->m_ModelSkeleton.m_arrModelJoints[0];

	//-------------------------------------------------------------------------------------
	//----             evaluate all controllers for this animation                    -----
	//-------------------------------------------------------------------------------------
#if !defined(__SPU__)
g_SkeletonUpdates++;
#endif // !__SPU__

	if (nAdditiveAnimation)
	{
		//additive animations
		for (uint32 j=1; j<numJoints; j++)
		{
			if (pFeatherMask && pFeatherMask->m_arrFeatherMask[j]==0)
				continue;

			if (parrController[j])
			{
				Quat rot;	  Vec3 pos;
				Status4 s = SPU_MAIN_PTR(parrController[j])->GetOP(fKeyTimeNew,rot,pos);
				Status4 status;	status.ops = s.ops & parrStatusDst[j].ops;
				if (status.o)
					parrRelPoseDst[j].q = Quat::CreateNlerp(IDENTITY,rot,ac.m_fWeight)*parrRelPoseDst[j].q;
				if (status.p)
					parrRelPoseDst[j].t = Vec3::CreateLerp(ZERO,     pos,ac.m_fWeight)+parrRelPoseDst[j].t;
			}
		}
	} 
	else 
	{
		//override animations
		for (uint32 j=1; j<numJoints; j++)
		{
			if (pFeatherMask && pFeatherMask->m_arrFeatherMask[j]==0)
				continue;

			if (parrController[j])
			{
				QuatT & 	RESTRICT_REFERENCE rRelPoseDst	= parrRelPoseDst[j];
				Status4 & RESTRICT_REFERENCE rStatusDst	 	= parrStatusDst[j];
				Quat rot;	  Vec3 pos;
				Status4 status = SPU_MAIN_PTR(parrController[j])->GetOP(fKeyTimeNew,rot,pos);
				//Overwrite Mode
				if (status.o)
				{
					if (rStatusDst.o)
						rRelPoseDst.q.SetNlerp(rRelPoseDst.q,rot,ac.m_fWeight);
					else 
						rRelPoseDst.q=rot;
				}
				if (status.p)
				{
					if (rStatusDst.p)
						rRelPoseDst.t.SetLerp( rRelPoseDst.t,pos,ac.m_fWeight);
					else
						rRelPoseDst.t=pos;
				}
				rStatusDst.ops |= status.ops;
			}
		}
	}


#ifdef _DEBUG
	uint32 o=0;
	uint32 p=0;
	for (uint32 j=0; j<numJoints; j++)
	{
		assert(parrStatusDst[j].o<2);
		if (parrStatusDst[j].o)
		{
			Quat q = parrRelPoseDst[j].q;
			assert(parrRelPoseDst[j].q.IsValid());
			o++;
		}
		assert(parrStatusDst[j].p<2);
		if (parrStatusDst[j].p)
		{
			Vec3 t = parrRelPoseDst[j].t;
			assert(parrRelPoseDst[j].t.IsValid());
			p++;
		}
	}
#endif

}

SPU_NO_INLINE void SamplePosePart::Execute(const CState& state, void* buffers[]) const
{
	const SamplePosePart& ac = *this;
	void** CBTemp = buffers;

	assert(ac.m_nEAnimID>=0);
	assert(ac.m_TargetBuffer<=Command::TargetBuffer);
	QuatT* parrRelPoseDst = SPU_LOCAL_PTR((QuatT*)CBTemp[ac.m_TargetBuffer+0]);
	Status4* parrStatusDst = SPU_LOCAL_PTR((Status4*)CBTemp[ac.m_TargetBuffer+1]);

	CAnimationSet* pAnimationSet = &state.m_pModel->m_AnimationSet;
	const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(ac.m_nEAnimID);
	assert(pMAG);
	int32 nEGlobalID = pMAG->m_nGlobalAnimId;
	assert(pMAG->m_nAssetType==AIM_File);

	// cache vars to local pointers (spares using this pointer everytime)
	uint32 numJoints 				= state.m_jointCount;
	assert(ac.m_fAnimTime>=0.0f && ac.m_fAnimTime<=1.0f);	


	GlobalAnimationHeaderAIM& rGlobalAnimHeader = SPU_MAIN_REF( g_AnimationManager.m_arrGlobalAIM[nEGlobalID] );
	rGlobalAnimHeader.m_nTouchedCounter++;

#if !defined(__SPU__)
	//	float fColor[4] = {0,1,0,1};
	extern float g_YLine;
#endif // !__SPU__


	const CModelJoint* pModelJoint = &state.m_pModel->m_ModelSkeleton.m_arrModelJoints[0];

	//f32 fFadeColor[4] = {1,0,0,1};
	//if (nAdditiveAnimation)
	//	g_pIRenderer->Draw2dLabel( 1,300, 6.0f, fFadeColor, false,"SingleEvaluation" ); g_YLine+=0x18;

	//	f32 fFadeColor[4] = {1,0,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.0f, fFadeColor, false,"fTransitionBlendIn: %f   fMultiLayerBlend: %f",fTransitionBlendIn,fMultiLayerBlend); g_YLine+=0x18;

	//-------------------------------------------------------------------------------------
	//----             evaluate all controllers for this animation                    -----
	//-------------------------------------------------------------------------------------
#if !defined(__SPU__)
g_SkeletonUpdates++;
#endif // !__SPU__

	f32 fKeyTimeNew =rGlobalAnimHeader.NTime2KTime(ac.m_fAnimTime);
	//override animations
	for (uint32 j=1; j<numJoints; j++)
	{

		IController* pController = rGlobalAnimHeader.GetControllerByJointCRC32( pModelJoint[j].m_nJointCRC32 );

		if (pController)
		{
			QuatT & 	RESTRICT_REFERENCE rRelPoseDst	= parrRelPoseDst[j];
			Status4 & RESTRICT_REFERENCE rStatusDst	 	= parrStatusDst[j];
			Quat rot;	  Vec3 pos;
			Status4 status = pController->GetOP( fKeyTimeNew, rot, pos  );
			//Overwrite Mode
			if (status.o)
			{
				if (rStatusDst.o)
					rRelPoseDst.q.SetNlerp(rRelPoseDst.q,rot,ac.m_fWeight);
				else 
					rRelPoseDst.q=rot;
			}
			if (status.p)
			{
				if (rStatusDst.p)
					rRelPoseDst.t.SetLerp( rRelPoseDst.t,pos,ac.m_fWeight);
				else
					rRelPoseDst.t=pos;
			}
			rStatusDst.ops |= status.ops;
		}
	}


#ifdef _DEBUG
	uint32 o=0;
	uint32 p=0;
	for (uint32 j=0; j<numJoints; j++)
	{
		assert(parrStatusDst[j].o<2);
		if (parrStatusDst[j].o)
		{
			Quat q = parrRelPoseDst[j].q;
			assert(parrRelPoseDst[j].q.IsValid());
			o++;
		}
		assert(parrStatusDst[j].p<2);
		if (parrStatusDst[j].p)
		{
			Vec3 t = parrRelPoseDst[j].t;
			assert(parrRelPoseDst[j].t.IsValid());
			p++;
		}
	}
#endif
}

SPU_NO_INLINE void PoseModifier::Execute(const CState& state, void* buffers[]) const
{
	const PoseModifier& ac = *this;
	void** CBTemp = buffers;

	SAnimationPoseModiferParams params;
	params.pCharacterInstance = state.m_pInstance;
	params.timeDelta = state.m_pInstance->m_fOriginalDeltaTime;
	params.locationNextPhysics = ac.m_PhysLocation;
	params.locationNextAnimation = ac.m_AnimLocation;
	params.pPoseRelative = (QuatT*)CBTemp[ac.m_TargetBuffer+0];
	params.pPoseAbsolute = SPU_PTR_SELECT(&state.m_pPoseData->m_jointsAbsolute[0], gSkeletonAbsolutePose);
	params.jointCount = state.m_jointCount;
	ac.m_pPoseModifier->Execute(params);
}

#ifdef _DEBUG
SPU_NO_INLINE void VerifyFull::Execute(const CState& state, void* buffers[]) const
{
	const VerifyFull& ac = *this;
	void** CBTemp = buffers;

	assert(ac.m_TargetBuffer<=Command::TargetBuffer);
	QuatT*			parrRelPoseDst	= SPU_LOCAL_PTR( (QuatT*)    CBTemp[ac.m_TargetBuffer+0]);
	Status4*		parrStatusDst		= SPU_LOCAL_PTR( (Status4*)  CBTemp[ac.m_TargetBuffer+1]);
	Skeleton::CLocator*	pLocatorDst			= SPU_LOCAL_PTR( (Skeleton::CLocator*)CBTemp[ac.m_TargetBuffer+2]);
	uint32 numJoints = state.m_jointCount;
	for (uint32 j=0; j<numJoints; j++)
	{
		Status4 s4 = parrStatusDst[j];
		assert(parrRelPoseDst[j].q.IsValid());
		assert(parrRelPoseDst[j].t.IsValid());
	}
}
#endif

} // namespace Command
