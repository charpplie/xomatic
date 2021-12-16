///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   LMG_ExtractParameters.cpp
//  Version:     v1.00
//  Created:     07/06/2009 by Jaewon Jung
//  Description: Locomotion Group utility functions
// -------------------------------------------------------------------------
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMG.h"
#include "ModelAnimationSet.h"
#include "AnimationManager.h"
#include "LoaderDBA.h"

namespace LMG
{
	namespace _private
	{
		IController* GAH_GetRootController(CAnimationSet* pAnimSet, GlobalAnimationHeaderCAF& rGAH, uint32 globalID, const char* pAnimName)
		{
			const CModelJoint* pRootJoint	= &pAnimSet->m_pModel->m_ModelSkeleton.m_arrModelJoints[0];
			IController* pController = rGAH.GetControllerByJointCRC32(pRootJoint->m_nJointCRC32);
			if (pController==0)
				rGAH.LoadControllersCAF();
			pController = rGAH.GetControllerByJointCRC32(pRootJoint->m_nJointCRC32);
			if (pController==0)
			{
				const char* pModelName = pAnimSet->m_pModel->GetFilePath();
				g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,
					VALIDATOR_FLAG_FILE, pModelName, "LMG is invalid. Locomotion-asset '%s' not in memory ",pAnimName );

				rGAH.InvalidateAssetLMG();
				return 0;
			}

			return pController;
		}

		void GAH_ComputeVelocity(GlobalAnimationHeaderCAF& rGAH, const Vec3& travelDir, BSAnimationLMG& bsAnimation)
		{
			rGAH.m_vVelocity = travelDir * rGAH.m_fSpeed;
			bsAnimation.m_qVelocity = travelDir*rGAH.m_fSpeed;
		}

		void GAH_ComputeTurn(GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl, BSAnimationLMG& bsAnimation)
		{
			assert(pRootCtl);
			Quat startRot, middleRot, endRot;
			pRootCtl->GetO( rGAH.NTime2KTime(0), startRot);
			pRootCtl->GetO( rGAH.NTime2KTime(0.5f), middleRot);
			pRootCtl->GetO( rGAH.NTime2KTime(1), endRot);

			const Vec3 startDir = startRot.GetColumn1();
			const Vec3 middleDir = middleRot.GetColumn1();
			const Vec3 endDir = endRot.GetColumn1();

			const f32 leftAngle = Ang3::CreateRadZ(startDir, middleDir);
			const f32 rightAngle = Ang3::CreateRadZ(middleDir, endDir);

			f32 angle = leftAngle + rightAngle;

			const f32 duration = rGAH.m_fEndSec - rGAH.m_fStartSec;
			rGAH.m_fAssetTurn = angle;
			rGAH.m_fTurnSpeed = angle / duration;
		}

		Vec3 GetTravelDir(IController* pRootCtl, int GAID)
		{
			GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[GAID];
			Vec3 startPos;
			pRootCtl->GetP( rGAH.NTime2KTime(0), startPos);
			Vec3 endPos;
			pRootCtl->GetP( rGAH.NTime2KTime(1), endPos);
			return (endPos - startPos).GetNormalizedSafe(Vec3(0, 1, 0));
		}

		template <bool bVelocity, bool bUseTravelDir, bool bTurn>
		void GAH_Compute(GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl, BSAnimationLMG& bsAnimation)
		{
			assert(0);
		}

		template <>
		void GAH_Compute<true, true, true>(GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl, BSAnimationLMG& bsAnimation)
		{
			GAH_ComputeVelocity(rGAH, GetTravelDir(pRootCtl,GAID),bsAnimation);
			GAH_ComputeTurn(rGAH, GAID, pRootCtl,bsAnimation);
		}

		template <>
		void GAH_Compute<true, false, true>(GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl, BSAnimationLMG& bsAnimation)
		{
			GAH_ComputeVelocity(rGAH, Vec3(0,1,0), bsAnimation);
			GAH_ComputeTurn(rGAH, GAID, pRootCtl, bsAnimation);
		}

		template <>
		void GAH_Compute<true, true, false>(GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl, BSAnimationLMG& bsAnimation)
		{
			GAH_ComputeVelocity(rGAH, GetTravelDir(pRootCtl,GAID), bsAnimation);
		}

		template <>
		void GAH_Compute<true, false, false>(GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl, BSAnimationLMG& bsAnimation)
		{
			GAH_ComputeVelocity(rGAH,Vec3(0,1,0), bsAnimation);
		}

		template <>
		void GAH_Compute<false, false, true>(GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl, BSAnimationLMG& bsAnimation)
		{
			GAH_ComputeTurn(rGAH,GAID,pRootCtl, bsAnimation);
		}

		void GAH_ComputeForCLMB(GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl, BSAnimationLMG& bsAnimation)
		{
			GAH_Compute<true, false, true>(rGAH, GAID, pRootCtl, bsAnimation);

			Vec3 startPos, endPos;
			pRootCtl->GetP( rGAH.NTime2KTime(0), startPos);
			pRootCtl->GetP( rGAH.NTime2KTime(1), endPos);
			bsAnimation.m_Position.z = endPos.z - startPos.z;
		}

		typedef void ComputeFuncType(GlobalAnimationHeaderCAF&, int, IController*, BSAnimationLMG&);
		template <ComputeFuncType Compute>
		void ExtractParametersTemplate(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& rGHLMG)
		{
			uint32 numExamples = rGHLMG.m_arrBSAnimations.size();
			for (uint32 i=0; i<numExamples; i++)
			{
				int32 globalID = pAnimSet->GetGlobalIDByName( rGHLMG.m_arrBSAnimations[i].m_strAnimName );
				assert(globalID >= 0);

				GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[globalID];
				IController* pRootCtl = GAH_GetRootController(pAnimSet,rGAH,globalID,rGHLMG.m_arrBSAnimations[i].m_strAnimName);
				if(pRootCtl)
					Compute(rGAH, globalID, pRootCtl, rGHLMG.m_arrBSAnimations[i]);
				else
					rGAH.InvalidateAssetLMG();
			}
		}

		void ExtractParametersForIdleRot(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
		{
			ExtractParametersTemplate< GAH_Compute<true, false, true> >(pAnimSet, globalAnimHeader);
		}

		void ExtractParametersForStrafingMotion(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& rGHLMG)
		{
			uint32 numAssets = rGHLMG.m_arrBSAnimations.size();
			for (uint32 i=0; i<numAssets; i++)
			{
				const char* name = rGHLMG.m_arrBSAnimations[i].m_strAnimName; //0
				int32 globalID = pAnimSet->GetGlobalIDByName(name);
				GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[globalID];

				assert(globalID>=0);
				if (globalID < 0)
					continue;

				int32 animID = pAnimSet->GetAnimIDByName(name);
				assert(animID>=0);

				uint32 OnDemand	=	rGAH.IsAssetOnDemand();
				f32 fStart			= rGAH.m_fStartSec;
				f32 fEnd				= rGAH.m_fEndSec;
				f32 duration		= fEnd-fStart;

				const CModelJoint* pModelJoint	= &pAnimSet->m_pModel->m_ModelSkeleton.m_arrModelJoints[0];
				GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[globalID];
				IController* pController	= rCAF.GetControllerByJointCRC32(pModelJoint[0].m_nJointCRC32);//;//pModelJoint[0].m_arrControllersMJoint[animID];
				if (pController==0)
					rCAF.LoadControllersCAF();

				pController	= rCAF.GetControllerByJointCRC32(pModelJoint[0].m_nJointCRC32);
				if (pController==0)
				{
					const char* pModelName = pAnimSet->m_pModel->GetFilePath();
					g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE, pModelName, "LMG is invalid. Locomotion-asset '%s' not in memory ",name );
					rGHLMG.InvalidateAssetLMG();
					continue;
				}
				assert(pController);

				Vec3 TravelDir0; pController->GetP( rGAH.NTime2KTime(0),TravelDir0);
				Vec3 TravelDir1; pController->GetP( rGAH.NTime2KTime(1),TravelDir1);
				const Vec3 vDeltaTravelDir = TravelDir1 - TravelDir0;
				const float fLengthSquared = vDeltaTravelDir.GetLengthSquared();
				const bool bDistinctTravelDirs = fLengthSquared > (0.01f * 0.01f);
				ANIM_ASSET_CHECK_TRACE(bDistinctTravelDirs, ("LMG is invalid. Locomotion-asset '%s' in file '%s' has equivalent travel directions.", name, pAnimSet->m_pModel->GetFilePath().c_str()));
				if(!bDistinctTravelDirs)
				{
					rGHLMG.InvalidateAssetLMG();
					continue;
				}

				const Vec3 TravelDir = vDeltaTravelDir.GetNormalizedSafe(Vec3(0,1,0));

				f32 speed = g_AnimationManager.m_arrGlobalCAF[globalID].m_fSpeed;
				if (speed<0.1f)
				{
					const char* pModelName = pAnimSet->m_pModel->GetFilePath();
					g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE, pModelName, "LMG is invalid. Locomotion-asset '%s' has a speed: %d",name,speed );
					rGHLMG.InvalidateAssetLMG();
					continue;
				}
				g_AnimationManager.m_arrGlobalCAF[globalID].m_vVelocity = TravelDir * speed;
				rGHLMG.m_arrBSAnimations[i].m_qVelocity = TravelDir * speed;

				Quat rot0,rot1;
				//			pController	= pModelJoint[0].m_arrControllersMJoint[animID];
				pController->GetO( rGAH.NTime2KTime(0), rot0);
				pController->GetO( rGAH.NTime2KTime(1), rot1);
				Vec3 v0=rot0.GetColumn1();
				Vec3 v1=rot1.GetColumn1();
				f32 radiant = Ang3::CreateRadZ(v0,v1);

				//radiant=0.0f;

				f32 fTurnSec = radiant/duration;
				g_AnimationManager.m_arrGlobalCAF[globalID].m_fAssetTurn =	0.0f;//radiant;
				g_AnimationManager.m_arrGlobalCAF[globalID].m_fTurnSpeed = 0.0f; //fTurnSec;
				if (fabsf(fTurnSec)>0.3f)
				{
					g_AnimationManager.m_arrGlobalCAF[globalID].m_vVelocity	=	Vec3(0,speed,0);
					rGHLMG.m_arrBSAnimations[i].m_qVelocity=Vec3(0,speed,0);
				}
			}
		}
		void ExtractParametersForSTH2(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
		{
			uint32 numAssets = globalAnimHeader.m_arrBSAnimations.size();
			//not a perfect solution, but it will do for now
			{
				const char* name0 = globalAnimHeader.m_arrBSAnimations[21].m_strAnimName; //0
				int32 globalID0 = pAnimSet->GetGlobalIDByName(name0);
				assert(globalID0>=0);
				f32 radiant0 = g_AnimationManager.m_arrGlobalCAF[globalID0].m_fAssetTurn;
				f32 turnsec0 = g_AnimationManager.m_arrGlobalCAF[globalID0].m_fTurnSpeed;
				//assert(turnsec0>0.5f);

				const char* name1 = globalAnimHeader.m_arrBSAnimations[23].m_strAnimName; //0
				int32 globalID1 = pAnimSet->GetGlobalIDByName(name1);
				assert(globalID1>=0);
				f32 radiant1 = g_AnimationManager.m_arrGlobalCAF[globalID1].m_fAssetTurn;
				f32 turnsec1 = g_AnimationManager.m_arrGlobalCAF[globalID1].m_fTurnSpeed;
				//assert(turnsec1>0.5f);

				g_AnimationManager.m_arrGlobalCAF[globalID0].m_fAssetTurn=(radiant0+radiant1)*0.5f;
				g_AnimationManager.m_arrGlobalCAF[globalID0].m_fTurnSpeed=(turnsec0+turnsec1)*0.5f;

				g_AnimationManager.m_arrGlobalCAF[globalID1].m_fAssetTurn=(radiant0+radiant1)*0.5f;
				g_AnimationManager.m_arrGlobalCAF[globalID1].m_fTurnSpeed=(turnsec0+turnsec1)*0.5f;
			}

			{
				const char* name0 = globalAnimHeader.m_arrBSAnimations[22].m_strAnimName; //0
				int32 globalID0 = pAnimSet->GetGlobalIDByName(name0);
				assert(globalID0>=0);
				f32 radiant0 = g_AnimationManager.m_arrGlobalCAF[globalID0].m_fAssetTurn;
				f32 turnsec0 = g_AnimationManager.m_arrGlobalCAF[globalID0].m_fTurnSpeed;
				//assert(turnsec0<-0.5f);

				const char* name1 = globalAnimHeader.m_arrBSAnimations[24].m_strAnimName; //0
				int32 globalID1 = pAnimSet->GetGlobalIDByName(name1);
				assert(globalID1>=0);
				f32 radiant1 = g_AnimationManager.m_arrGlobalCAF[globalID1].m_fAssetTurn;
				f32 turnsec1 = g_AnimationManager.m_arrGlobalCAF[globalID1].m_fTurnSpeed;
				//assert(turnsec0<-0.5f);

				g_AnimationManager.m_arrGlobalCAF[globalID0].m_fAssetTurn=(radiant0+radiant1)*0.5f;
				g_AnimationManager.m_arrGlobalCAF[globalID0].m_fTurnSpeed=(turnsec0+turnsec1)*0.5f;

				g_AnimationManager.m_arrGlobalCAF[globalID1].m_fAssetTurn=(radiant0+radiant1)*0.5f;
				g_AnimationManager.m_arrGlobalCAF[globalID1].m_fTurnSpeed=(turnsec0+turnsec1)*0.5f;
			}
		}

		void ExtractParametersForIdleStep(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
		{
			ExtractParametersTemplate< GAH_Compute<true, true, false> >(pAnimSet, globalAnimHeader);
		}

		void ExtractParametersForTurningMotion(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
		{
			ExtractParametersTemplate< GAH_Compute<true, false, false> >(pAnimSet, globalAnimHeader);
		}

		void ExtractParametersForPROT(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
		{
			ExtractParametersTemplate< GAH_Compute<true, false, true> >(pAnimSet, globalAnimHeader);
		}

		void ExtractParametersForPUSH(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
		{
			ExtractParametersTemplate< GAH_Compute<true, true, true> >(pAnimSet, globalAnimHeader);
		}

		void ExtractParametersForSCAL(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
		{
			ExtractParametersTemplate< GAH_Compute<true, false, true> >(pAnimSet, globalAnimHeader);
		}

		void ExtractParametersForCLMB(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
		{
			ExtractParametersTemplate< GAH_ComputeForCLMB >(pAnimSet, globalAnimHeader);
		}

		void ExtractParametersForCOOP(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
		{
			DynArray<BSAnimationLMG> & bsAnimations = globalAnimHeader.m_arrBSAnimations;
			for (uint32 i = 0; i < (uint32)bsAnimations.size(); i++)
			{
				int32 globalID = pAnimSet->GetGlobalIDByName(bsAnimations[i].m_strAnimName);
				assert(globalID >= 0);

				GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[globalID];
				IController* pRootCtl = GAH_GetRootController(pAnimSet,rGAH,globalID,bsAnimations[i].m_strAnimName);
				assert(pRootCtl);
				GAH_ComputeTurn(rGAH, globalID, pRootCtl,bsAnimations[i]);
				//GAH_ComputeVelocity(rGAH, Vec3(0,1,0));
				rGAH.m_vVelocity = Vec3(0,1,0)*rGAH.m_fSpeed;
			}
		}

		void ExtractParametersForIdleToMove(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
		{
			ExtractParametersTemplate< GAH_Compute<false, false, true> >(pAnimSet, globalAnimHeader);
		}

		void ExtractParametersForXIM2(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& rGHLMG)
		{
			uint32 numAssets = rGHLMG.m_arrBSAnimations.size();
			for (uint32 i=0; i<numAssets; i++)
			{
				const char* name = rGHLMG.m_arrBSAnimations[i].m_strAnimName; //0
				int32 globalID = pAnimSet->GetGlobalIDByName(name);
				assert(globalID>=0);
				int32 animID = pAnimSet->GetAnimIDByName(name);
				assert(animID>=0);
				if (globalID < 0)
					break;

				f32 fStart	= g_AnimationManager.m_arrGlobalCAF[globalID].m_fStartSec;
				f32 fEnd		= g_AnimationManager.m_arrGlobalCAF[globalID].m_fEndSec;
				f32 duration = fEnd-fStart;
				const CModelJoint* pModelJoint = &pAnimSet->m_pModel->m_ModelSkeleton.m_arrModelJoints[0];

				GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[globalID];
				IController* pController = GAH_GetRootController(pAnimSet,rGAH,globalID,name);
		//		IController* pController = g_AnimationManager.m_arrGlobalCAF[globalID].GetControllerByJointID(pModelJoint[0].m_nJointCRC32);//pController = pModelJoint[0].m_arrControllersMJoint[AnimID];
				assert(pController);
				if (pController==0)
					break;
				Vec3 TravelDir0; pController->GetP( rGAH.NTime2KTime(0),TravelDir0);
				Vec3 TravelDir1; pController->GetP( rGAH.NTime2KTime(1),TravelDir1);
				Vec3 TravelDir = (TravelDir1-TravelDir0).GetNormalizedSafe(Vec3(0,1,0));

				Quat rot0; pController->GetO( rGAH.NTime2KTime(0), rot0);
				Vec3 v0=rot0.GetColumn1();
				Quat rot1; pController->GetO( rGAH.NTime2KTime(1), rot1);
				Vec3 v1=rot1.GetColumn1();
				f32 radiant = Ang3::CreateRadZ(v0,v1);
				g_AnimationManager.m_arrGlobalCAF[globalID].m_fAssetTurn =	radiant;
				g_AnimationManager.m_arrGlobalCAF[globalID].m_fTurnSpeed =	radiant/duration;

				uint32 numKeys = uint32((duration / (1.0f/30.0f))+1);
				f32 timestep = 1.0f/f32(numKeys);
				f32 t=0.0f;
				std::vector<Vec3> root_keys;
				root_keys.resize(numKeys);	

				for (uint32 k=0; k<numKeys; k++)
				{
					pController->GetP( t, root_keys[k]);
					t += timestep;
				}

				f32 speed = 0.0f; 
				uint32 poses=5;
				for (uint32 k=(numKeys-poses-1); k<(numKeys-1); k++)
					speed += (root_keys[k+0]-root_keys[k+1]).GetLength()*30.0f; 

				speed/=poses;

				g_AnimationManager.m_arrGlobalCAF[globalID].m_fSpeed =	speed;
				g_AnimationManager.m_arrGlobalCAF[globalID].m_vVelocity = Vec3(0,speed,0);
				rGHLMG.m_arrBSAnimations[i].m_qVelocity=Vec3(0,speed,0);
			}
		}

		uint32 GetAnimIDAndHeadersLMG(CAnimationSet* pAnimSet, const GlobalAnimationHeaderLMG& rGlobalAnimHeader, int32* nAnimID, const ModelAnimationHeader** pAnim)
		{
			nAnimID[0]=-1; 
			nAnimID[1]=-1;
			nAnimID[2]=-1; 

			nAnimID[3]=-1;
			nAnimID[4]=-1; 
			nAnimID[5]=-1;

			nAnimID[6]=-1;
			nAnimID[7]=-1;
			nAnimID[8]=-1;

			uint32 num=0;
			if (rGlobalAnimHeader.IsAssetLMG())
			{
				num = rGlobalAnimHeader.m_arrBSAnimations.size();
				for (uint32 i=0; i<num; i++)
				{
					nAnimID[i] = pAnimSet->GetAnimIDByName( rGlobalAnimHeader.m_arrBSAnimations[i].m_strAnimName );
					pAnim[i] = pAnimSet->GetModelAnimationHeader(nAnimID[i]);
				}
			}

			return num;
		}

		uint32 GetAnimIDAndHeadersPMG(CAnimationSet* pAnimSet, const GlobalAnimationHeaderPMG& rGlobalAnimHeader, int32* nAnimID, const ModelAnimationHeader** pAnim)
		{
			nAnimID[0]=-1; 
			nAnimID[1]=-1;
			nAnimID[2]=-1; 

			nAnimID[3]=-1;
			nAnimID[4]=-1; 
			nAnimID[5]=-1;

			nAnimID[6]=-1;
			nAnimID[7]=-1;
			nAnimID[8]=-1;

			uint32 num=0;
			if (rGlobalAnimHeader.IsAssetPMG())
			{
				num = rGlobalAnimHeader.m_arrBSAnimationsPMG.size();
				for (uint32 i=0; i<num; i++)
				{
					nAnimID[i] = pAnimSet->GetAnimIDByName( rGlobalAnimHeader.m_arrBSAnimationsPMG[i].m_strAnimName );
					pAnim[i] = pAnimSet->GetModelAnimationHeader(nAnimID[i]);
				}
			}

			return num;
		}

	} // namespace _private
} // namespace LMG

namespace LMG
{
	bool IsStrafingLMGCode(uint32 nBC)
	{
		uint32 nS__1 = *(uint32*)"S__1"; // strafe6,               one speed
		uint32 nM__1 = *(uint32*)"M__1"; // strafe8,               one speed
		uint32 nS__2 = *(uint32*)"S__2"; // strafe,               two speeds
		uint32 nS_H2 = *(uint32*)"S_H2"; // strafe, slope,        two speeds
		uint32 nST_2 = *(uint32*)"ST_2"; // strafe+curving,       two speeds
		uint32 n_TH2 = *(uint32*)"_TH2"; //        curving+slope, two speeds
		uint32 nSTH2 = *(uint32*)"STH2"; // strafe+curve+slope,   two speeds
		uint32 nSTF1 = *(uint32*)"STF1"; // strafe, one speed
		uint32 nSTF2 = *(uint32*)"STF2"; // strafe, two speeds

		uint32 nT_WS = *(uint32*)"T_WS"; // [artemk] tactical_walk_strafe. S__1 for tactical moves

		// OBSOLETE
		uint32 nSUD2 = *(uint32*)"SUD2";
		uint32 nSTHX = *(uint32*)"STHX";

		bool isStrafingLMG =( nBC==nS__1 || nBC==nM__1 || nBC==nS__2 || nBC==nS_H2 || nBC==nST_2 || nBC==n_TH2 || nBC==nSTH2 || nBC==nSTHX || nBC==nSTF1 || nBC==nSTF2 || nBC==nSUD2 || nBC==nT_WS );
		return isStrafingLMG;
	}

	bool IsValidBlendCode( uint32 blendCode )
	{
#define DRY(x) if ( *(uint32*)(#x) == blendCode ) { return true; }
#include "LMG_Types.inc"
#undef DRY
		return false;
	}

	void ExtractParameters(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader)
	{
		assert(pAnimSet);
		uint32 nBC = globalAnimHeader.m_nBlendCodeLMG;
		DynArray<BSAnimationLMG> & bsAnimations = globalAnimHeader.m_arrBSAnimations;

		uint32 nIROT = *(uint32*)"IROT";
		uint32 nTSTP = *(uint32*)"TSTP";

		uint32 nSTH2 = *(uint32*)"STH2"; // strafe+curve+slope,   two speeds

		uint32 nISTP = *(uint32*)"ISTP";
		uint32 nT_IS = *(uint32*)"T_IS"; // [artemk] tactical_idle_step. ISTP for tactical moves

		uint32 nT_IA = *(uint32*)"T_IA";

		uint32 nFLR1 = *(uint32*)"FLR1"; 
		uint32 nFLR2 = *(uint32*)"FLR2"; 
		uint32 nFLR3 = *(uint32*)"FLR3"; 

		uint32 nUDH1 = *(uint32*)"UDH1"; 
		uint32 nUDH2 = *(uint32*)"UDH2"; 
		uint32 nUDH3 = *(uint32*)"UDH3";
				
		uint32 nPROT = *(uint32*)"PROT";
		
		uint32 nPUSH = *(uint32*)"PUSH";

		uint32 nSCAL = *(uint32*)"SCAL";

		uint32 nCLMB = *(uint32*)"CLMB";

		uint32 nCOOP = *(uint32*)"COOP";

		uint32 nI2M1 = *(uint32*)"I2M1"; 
		uint32 nI2M2 = *(uint32*)"I2M2"; 

		uint32 nXIM2 = *(uint32*)"XIM2"; 

		if (nBC==nIROT || nBC==nTSTP)
			_private::ExtractParametersForIdleRot(pAnimSet, globalAnimHeader);
		else if ( IsStrafingLMGCode(nBC) )
		{
			_private::ExtractParametersForStrafingMotion(pAnimSet, globalAnimHeader);
			if (nBC==nSTH2)
				_private::ExtractParametersForSTH2(pAnimSet, globalAnimHeader);
		}
		else if ( nBC==nISTP || nBC==nT_IS )
			_private::ExtractParametersForIdleStep(pAnimSet, globalAnimHeader);
		else if (nBC == nT_IA)
		{
			// [artemk]: do not use any parameters for this blend type
		}
		else if (nBC==nFLR1 ||	nBC==nFLR2 ||	nBC==nFLR3 || nBC==nUDH1 || nBC==nUDH2 ||	nBC==nUDH3 )
			_private::ExtractParametersForTurningMotion(pAnimSet, globalAnimHeader);
		else if (nBC==nPROT )
			_private::ExtractParametersForPROT(pAnimSet, globalAnimHeader);
		else if (nBC == nPUSH)
			_private::ExtractParametersForPUSH(pAnimSet, globalAnimHeader);
		else if (nBC==nSCAL )
			_private::ExtractParametersForSCAL(pAnimSet, globalAnimHeader);
		else if (nBC==nCLMB )
			_private::ExtractParametersForCLMB(pAnimSet, globalAnimHeader);
		else if (nBC==nCOOP )
			_private::ExtractParametersForCOOP(pAnimSet, globalAnimHeader);
		if (nBC==nI2M1 || nBC==nI2M2)
			_private::ExtractParametersForIdleToMove(pAnimSet, globalAnimHeader);
		if (nBC==nXIM2)
			_private::ExtractParametersForXIM2(pAnimSet, globalAnimHeader);
	}



	SParametric BuildRuntimeStructCAF(CAnimationSet* pAnimSet, const ModelAnimationHeader* pAnim0, int nID)
	{
		SParametric lmg;
		if (pAnim0->m_nAssetType==CAF_File)
		{
			lmg.m_nAnimID[0]		=	nID;
			lmg.m_numAnims			=	1;
			lmg.m_nSegmentCounter[0]	=0;
			return lmg;
		}

		assert(0);
		return lmg;
	}
	SParametric BuildRuntimeStructAIM(CAnimationSet* pAnimSet, const ModelAnimationHeader* pAnim0, int nID)
	{
		SParametric lmg;
		if (pAnim0->m_nAssetType==AIM_File)
		{
			lmg.m_nAnimID[0]		=	nID;
			lmg.m_numAnims			=	1;
			lmg.m_nSegmentCounter[0]	=0;
			return lmg;
		}

		assert(0);
		return lmg;
	}


	SParametric BuildRuntimeStructLMG(CAnimationSet* pAnimSet, const ModelAnimationHeader* pAnim0, int nID)
	{
		SParametric lmg;

		int32 nAnimID[MAX_LMG_ANIMS]; 
		const ModelAnimationHeader* pAnimHeaders[MAX_LMG_ANIMS];

		uint32 nGlobalAnimId = pAnim0->m_nGlobalAnimId;
		if (pAnim0->m_nAssetType==LMG_File)
		{
			GlobalAnimationHeaderLMG& rGlobalAnimHeader0 = g_AnimationManager.m_arrGlobalLMG[nGlobalAnimId];
			assert(rGlobalAnimHeader0.IsAssetLMG());
			lmg.m_nParametricID				= nID;
			lmg.m_numAnims			=	_private::GetAnimIDAndHeadersLMG(pAnimSet, rGlobalAnimHeader0, &nAnimID[0],&pAnimHeaders[0]);
			for (int32 i=0; i<lmg.m_numAnims; i++)
			{
				if (pAnimHeaders[i])
				{
					lmg.m_nAnimID[i]		=	nAnimID[i];
					lmg.m_nSegmentCounter[i]	=0;
				}
				else
					assert(0);
			}
			return lmg;
		}

		//assert(0);
		return lmg;
	}


	SParametric BuildRuntimeStructPMG(CAnimationSet* pAnimSet, const ModelAnimationHeader* pAnim, int nID)
	{
		SParametric lmg;
		int32 nAnimID[MAX_LMG_ANIMS]; 
		const ModelAnimationHeader* pAnimHeaders[MAX_LMG_ANIMS];

		uint32 nGlobalAnimId = pAnim->m_nGlobalAnimId;
		if (pAnim->m_nAssetType==PMG_File)
		{
			GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[nGlobalAnimId];
			assert(rGlobalAnimHeader.IsAssetPMG());
			lmg.m_nParametricID				= nID;
			lmg.m_numAnims			=	_private::GetAnimIDAndHeadersPMG(pAnimSet, rGlobalAnimHeader, &nAnimID[0],&pAnimHeaders[0]);
			for (int32 i=0; i<lmg.m_numAnims; i++)
			{
				if (pAnimHeaders[i])
				{
					lmg.m_nAnimID[i]		=	nAnimID[i];
					lmg.m_nSegmentCounter[i]	=0;
				}
				else
					assert(0);
			}
			return lmg;
		}

		assert(0);
		return lmg;
	}
} // namespace LMG
