#include "StdAfx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "../CharacterInstance.h"
#include "../Model.h"
#include "../ModelSkeleton.h"
#include "../CharacterManager.h"


#include "PoseModifierHelper.h"
#include "LimbIk.h"

/*
CLimbIk
*/

CRYREGISTER_CLASS(CLimbIk)

//

CLimbIk::CLimbIk()
{
	m_pSetups = m_setupsBuffer[0];
	m_setupCount = 0;

	m_pSetupsExecute = m_setupsBuffer[1];
	m_setupCountExecute = 0;
}

CLimbIk::~CLimbIk()
{
}

//

void CLimbIk::AddSetup(uint64 setup, const QuatT& targetPositionLocal, const bool adjustEndEff)
{
	if (m_setupCount >= sizeof(m_setupsBuffer[0]) / sizeof(m_setupsBuffer[0][0]))
		return;

	for (uint32 i=0; i<m_setupCount; ++i)
	{
		if (m_pSetups[i].setup != setup)
			continue;

		m_pSetups[i].targetPositionLocal = targetPositionLocal;
		m_pSetups[i].adjustEndEffector = adjustEndEff;
		return;
	}

	m_pSetups[m_setupCount].setup = setup;
	m_pSetups[m_setupCount].targetPositionLocal = targetPositionLocal;
	m_pSetups[m_setupCount].adjustEndEffector = adjustEndEff;
	m_setupCount++;
}

// IAnimationPoseModifier

bool CLimbIk::Prepare(const SAnimationPoseModiferParams& params)
{
	std::swap(m_pSetups, m_pSetupsExecute);
	m_setupCountExecute = m_setupCount;
	m_setupCount = 0;
	return true;
}

SPU_INDIRECT(CommandBufferExecute(ML))
bool CLimbIk::Execute(const SAnimationPoseModiferParams& params)
{
	CModelSkeleton* pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);
	for (uint32 i=0; i<m_setupCountExecute; ++i)
	{
		PoseModifierHelper::IK_Solver(
			pModelSkeleton, (const char*)&m_pSetupsExecute[i].setup, m_pSetupsExecute[i].targetPositionLocal,
			SPU_LOCAL_PTR(params.pPoseRelative), SPU_LOCAL_PTR(params.pPoseAbsolute), m_pSetupsExecute[i].adjustEndEffector);
	}
	return true;
}

void CLimbIk::Synchronize()
{
}
