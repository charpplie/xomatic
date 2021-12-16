#include "stdafx.h"
#include "Command_Buffer.h"
#include "Command_Commands.h"

#include "CharacterInstance.h"
#include "SkeletonAnim.h"
#include "SkeletonPose.h"

using namespace Command;

/*
CState
*/

bool CState::Initialize(CCharInstance* pInstance)
{
	m_pInstance = pInstance;
	m_pModel = pInstance->m_pModel;

	m_pPoseData = pInstance->m_SkeletonPose.GetPoseDataWriteable();
	m_pLocator = pInstance->m_SkeletonPose.GetLocatorWriteable();

	m_jointCount = pInstance->m_SkeletonPose.GetJointCount();

	m_lod = m_pInstance->GetAnimationLOD();

	m_timeDelta = m_pInstance->m_fDeltaTime;

	m_pJointMask = NULL;
	m_jointMaskCount = 0;

	m_bFootPlants = pInstance->m_SkeletonPose.m_FootPlants;

	return true;
}

/*
CBuffer
*/

bool CBuffer::Initialize(CCharInstance* pInstance)
{
	m_pCommands = m_pBuffer;
	m_commandCount = 0;

	m_state.Initialize(pInstance);

	m_pInstance = pInstance;
	return true;
}

void CBuffer::SetPoseData(Skeleton::CPoseData& poseData, Skeleton::CLocator& locator)
{
	m_state.m_pPoseData = &poseData;
	m_state.m_pLocator = &locator;
}

SPU_NO_INLINE void CBuffer::Execute()
{
	DEFINE_PROFILER_FUNCTION();

#if defined(__SPU__)
	// assign global pointer to global memory blocks
	gSkeletonAbsolutePose = SPU_LOCAL_PTR((QuatT*)gSkeletonAbsolutPoseStorage);
	gSkeletonRelativePose = SPU_LOCAL_PTR((QuatT*)gSkeletonRelativePoseStorage);
	gSkeletonControllerInfo = SPU_LOCAL_PTR((Status4*)gSkeletonControllerInfoStorage);

	// Transfer data to SPU
	MemcpyManager<QuatT> AbsolutPoseData(gSkeletonAbsolutePose, const_cast<QuatT *>(m_state.m_pPoseData->GetJointsAbsolute()), g_jointCount * sizeof(QuatT), DMA_ABSOLUTEPOSE_ID);
	MemcpyManager<QuatT> RelativePoseData(gSkeletonRelativePose, const_cast<QuatT *>(m_state.m_pPoseData->GetJointsRelative()), g_jointCount * sizeof(QuatT), DMA_RELATIVEPOSE_ID);
	MemcpyManager<Status4> ControllerData(gSkeletonControllerInfo, const_cast<Status4 *>(m_state.m_pPoseData->GetJointsStatus()), g_jointCount * sizeof(Status4), DMA_CONTROLLERINFO_ID);

	// Start transfers to SPU
	AbsolutPoseData.start_transfer_to_spu();
	RelativePoseData.start_transfer_to_spu();			        		

	AbsolutPoseData.sync_transfer();
	RelativePoseData.sync_transfer();	
#endif

	QuatT pJointsTemp[MAX_JOINT_AMOUNT];
	Skeleton::CLocator tmpLocator;

	QuatT* const __restrict pJointsRelative = SPU_PTR_SELECT(&m_state.m_pPoseData->m_jointsRelative[0], gSkeletonRelativePose);
	Status4* const __restrict pJointsStatus = SPU_PTR_SELECT(&m_state.m_pPoseData->m_jointsStatus[0], gSkeletonControllerInfo);
	SpuStackValue<Skeleton::CLocator, true,true> localBlendedRoot(*m_state.m_pLocator);
	Skeleton::CLocator &rBlendedRoot = localBlendedRoot;
#if defined(__SPU__) // need to remember the spu local version, since AimIK access it later
	gBlendedRoot = &rBlendedRoot;
#endif

	::memset(pJointsStatus, 0, m_state.m_jointCount * sizeof(Status4));

	m_state.m_pJointMask = NULL;
	m_state.m_jointMaskCount = 0;

	void* CBTemp[TargetBuffer+3];
	CBTemp[TmpBuffer+0]	= pJointsTemp;
	CBTemp[TmpBuffer+1]	= pJointsStatus;
	CBTemp[TmpBuffer+2]	= &tmpLocator;

	CBTemp[TargetBuffer+0] = pJointsRelative;
	CBTemp[TargetBuffer+1] = pJointsStatus;
	CBTemp[TargetBuffer+2] = &rBlendedRoot;

	uint8* pCommands = m_pBuffer;
	uint32 commandOffset = 0;
	for (uint32 i=0; i<m_commandCount; ++i)
	{
		uint8& command = pCommands[commandOffset];
		switch (command)
		{
		case ClearFull::ID:
			((ClearFull&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(ClearFull);
			break;

		case ClearSingle::ID:
			((ClearSingle&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(ClearSingle);
			break;

		case SampleAddAnimFull::ID:
			((SampleAddAnimFull&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(SampleAddAnimFull);
			break;

		case CopyAddAnimFull::ID:
			((CopyAddAnimFull&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(CopyAddAnimFull);
			break;

		case NormalizeFull::ID:
			((NormalizeFull&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(NormalizeFull);
			break;

		case ScaleUniformFull::ID:
			((ScaleUniformFull&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(ScaleUniformFull);
			break;

#ifdef _DEBUG
		case VerifyFull::ID:
			((VerifyFull&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(VerifyFull);
			break;
#endif
		case PoseModifier::ID:
			((PoseModifier&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(PoseModifier);
			break;

		case SampleAnimPart::ID:
			((SampleAnimPart&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(SampleAnimPart);
			break;

		case SampleFeatherAnimPart::ID:
			((SampleFeatherAnimPart&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(SampleFeatherAnimPart);
			break;

		case JointMask::ID:
			if (Console::GetInst().ca_UseJointMasking)
			{
				m_state.m_pJointMask = ((JointMask&)command).m_pMask;
				m_state.m_jointMaskCount = ((JointMask&)command).m_count;
			}
			commandOffset += sizeof(JointMask);
			break;

		case SampleAddPoseFull::ID:
			((SampleAddPoseFull&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(SampleAddPoseFull);
			break;

		case SamplePosePart::ID:
			((SamplePosePart&)command).Execute(m_state, CBTemp);
			commandOffset += sizeof(SamplePosePart);
			break;

		default:
			CryFatalError("CryAnimation: Command-Buffer Invalid: Command %d  Model: %s",command,m_state.m_pModel->GetModelFilePath());
			break;
		}
	}

#if !defined(__SPU__)
	if (Console::GetInst().ca_DebugCommandBuffer)
		DebugDraw();
#endif

	// SPU post block, used to transfer finished data back to ppu
#if defined(__SPU__)
	// Transfer SPU data back to PPU
	AbsolutPoseData.start_transfer_from_spu();
	RelativePoseData.start_transfer_from_spu();			        		
	ControllerData.start_transfer_from_spu();

	AbsolutPoseData.sync_transfer();
	RelativePoseData.sync_transfer();
	ControllerData.sync_transfer();
#endif
}

void CBuffer::DebugDraw()
{
	float fColor2[4] = {1,0,1,1};
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor2, false,
		"m_CommandBufferCounter %d  Commands: %d", GetLengthUsed(), m_commandCount);
	g_YLine+=10;

	IAnimationSet* pAnimationSet = m_state.m_pModel->GetAnimationSet();

	uint8* pCommands = m_pBuffer;
	uint32 commandOffset = 0;
	for (uint32 c=0; c<m_commandCount; c++)
	{
		int32 anim_command = pCommands[commandOffset];
		switch (anim_command)
		{
		case ClearFull::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "BaseAnim_CLEAR_BUFFER"); g_YLine+=8;
			commandOffset += sizeof(ClearFull);
			break;

		case ClearSingle::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "BaseAnim_CLEAR_ROOT"); g_YLine+=8;
			commandOffset += sizeof(ClearSingle);
			break;

		case SampleAddAnimFull::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "BaseAnim_FETCH_ADD: %f %s", ((SampleAddAnimFull*)&pCommands[commandOffset])->m_fWeight, pAnimationSet->GetNameByAnimID( ((SampleAddAnimFull*)&pCommands[commandOffset])->m_nEAnimID ) ); g_YLine+=8;
			commandOffset += sizeof(SampleAddAnimFull);
			break;

		case CopyAddAnimFull::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "BaseAnim_COPY_ADD"); g_YLine+=8;
			commandOffset += sizeof(CopyAddAnimFull);
			break;

		case NormalizeFull::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "BaseAnim_NORMALIZE"); g_YLine+=8;
			commandOffset += sizeof(NormalizeFull);
			break;

		case ScaleUniformFull::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "BaseAnim_USCALE"); g_YLine+=8;
			commandOffset += sizeof(ScaleUniformFull);
			break;

#ifdef _DEBUG
		case VerifyFull::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "BaseAnim_VERIFY"); g_YLine+=8;
			commandOffset += sizeof(VerifyFull);
			break;
#endif

		case PoseModifier::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "Proc_POSEMOD"); g_YLine+=8;
			commandOffset += sizeof(PoseModifier);
			break;

		case SampleAnimPart::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "PartAnim_FETCH_ADD: %f %s", ((SampleAnimPart*)&pCommands[commandOffset])->m_fWeight, pAnimationSet->GetNameByAnimID( ((SampleAnimPart*)&pCommands[commandOffset])->m_nEAnimID ) ); g_YLine+=8;
			commandOffset += sizeof(SampleAnimPart);
			break;

		case SampleFeatherAnimPart::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "PartAnim_FEATHERFETCH"); g_YLine+=8;
			commandOffset += sizeof(SampleFeatherAnimPart);
			break;

		case JointMask::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "State_JOINTMASK"); g_YLine+=8;
			commandOffset += sizeof(JointMask);
			break;

		case SampleAddPoseFull::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "BasePose_FETCH_ADD: %f %s", ((SampleAddPoseFull*)&pCommands[commandOffset])->m_fWeight, pAnimationSet->GetNameByAnimID( ((SampleAddPoseFull*)&pCommands[commandOffset])->m_nEAnimID ) ); g_YLine+=8;
			commandOffset += sizeof(SampleAddPoseFull);
			break;

		case SamplePosePart::ID:
			g_pIRenderer->Draw2dLabel(1,g_YLine, 1.0f, fColor2, false, "PartPose_FETCH_ADD: %f %s", ((SamplePosePart*)&pCommands[commandOffset])->m_fWeight, pAnimationSet->GetNameByAnimID( ((SamplePosePart*)&pCommands[commandOffset])->m_nEAnimID ) ); g_YLine+=8;
			commandOffset += sizeof(SamplePosePart);
			break;

		default:
			CryFatalError("CryAnimation: Command-Buffer Invalid: Command %d  Model: %s", anim_command, m_state.m_pModel->GetModelFilePath());
			break;
		}
	}
}
