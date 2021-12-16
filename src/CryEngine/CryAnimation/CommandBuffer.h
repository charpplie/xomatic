//////////////////////////////////////////////////////////////////////
//
// CryEngine Source code
//
// File:CommandBuffer.h
// Implementation of Skeleton class
//
// History:
// August 6, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#ifndef _COMMAND_BUFFER_H
#define _COMMAND_BUFFER_H

#include "Command_Buffer.h"

class CommandBuffer
{
public:
	void Create(CCharInstance* pInstance, const QuatT& rPhysLocationCurr, const QuatTS& rAnimLocationCurr);
	void Execute();

private:
	void BasePlayback(const CAnimation& rAnim, f32& fRootPlayback);
	void BaseEvaluationLMG(const CAnimation& rAnim, uint32 nTargetBuffer);
	void LPlayback(const CAnimation& rAnim, uint32 nTargetBuffer, uint32 nVLayer, uint32 mask);
	void CreateAimPoseQueue_New(CCharInstance* pInstance, uint32 numAnimsL0, uint32 nAimIKLayer);
	void CreateAimPoseQueue_Old(CCharInstance* pInstance, uint32 numAnimsL0, uint32 nAimIKLayer);

private:
	Command::CBuffer m_buffer;

	CCharInstance* m_pInstance;
	CSkeletonPose* m_pSkeletonPose;
	CSkeletonAnim* m_pSkeletonAnim;
};

#endif // _COMMAND_BUFFER
