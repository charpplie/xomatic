/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Class to control shape deformation for different suit actions
-------------------------------------------------------------------------
History:
- 20-01-2010: Created by Benito G.R.

*************************************************************************/

#include "StdAfx.h"
#include "SuitShapeDeformation.h"
#include "GameCVars.h"

#include <ICryAnimation.h>

//////////////////////////////////////////////////////////////////////////

CNanoSuitShapeDeformation::CNanoSuitShapeDeformation()
: m_armsShapeTransitionDeformation(0.0f)
, m_armsPulseShapeDeformation(0.0f)
{
}

void CNanoSuitShapeDeformation::ResetSuitShape(IEntity* pSuitOwnerEntity)
{
	ICharacterInstance* pMainCharacter = pSuitOwnerEntity->GetCharacter(0);
	if (pMainCharacter)
	{
		f32* pShapeDeformationArray = pMainCharacter->GetShapeDeformArray();
		for (int i = 0; i < SUIT_SHAPE_ARRAY_SIZE; ++i)
		{
			pShapeDeformationArray[i] = 0.0f;
		}
	}

	m_armsShapeTransitionDeformation = 0.0f;
	m_armsPulseShapeDeformation = 0.0f;

	m_activeTransition.Reset();
	m_activePulse.Reset();
}

void CNanoSuitShapeDeformation::Update(IEntity *pSuitOwnerEntity, const float frameTime)
{
	UpdateCurrentTransition(frameTime);
	UpdateCurrentPulse(frameTime);

	UpdateCharacterShape(pSuitOwnerEntity);
}

void CNanoSuitShapeDeformation::UpdateCurrentTransition(const float frameTime)
{
	m_activeTransition.runningTime += frameTime;

	switch(m_activeTransition.transitionType)
	{
	case eST_SuitModeChange:
		{
			UpdateSuitModeChangeTransition(frameTime);
		}
		break;

	case eST_SuitModeActivatePower:
		{
			UpdateSuitModeActivatePowerTransition(frameTime);
		}
		break;

	case eST_SuitModeActivateArmor:
		{
			UpdateSuitModeActivateArmorTransition(frameTime);
		}
		break;

	case eST_SuitModeDeactivate:
		{
			UpdateSuitModeDeactivateTransition(frameTime);
		}
		break;
	}
}

void CNanoSuitShapeDeformation::UpdateCurrentPulse(const float frameTime)
{
	m_activePulse.runningTime += frameTime;

	switch(m_activePulse.pulseType)
	{
	case eSP_HitPulse:
		{
			UpdateSuitHitPulse(frameTime);
		}
		break;
	}
}

void CNanoSuitShapeDeformation::UpdateCharacterShape( IEntity* pSuitOwnerEntity )
{
	if (g_pGameCVars->g_nanoSuitEnableSuitShapeDeformation != 0)
	{
		ICharacterInstance* pMainCharacter = pSuitOwnerEntity->GetCharacter(0);
		if (pMainCharacter)
		{
			f32* pShapeDeformationArray = pMainCharacter->GetShapeDeformArray();
			pShapeDeformationArray[SUIT_SHAPE_ARMS_IDX] = clamp(m_armsShapeTransitionDeformation + m_armsPulseShapeDeformation, -2.0f, 2.0f);
		}
	}
}


void CNanoSuitShapeDeformation::ExecuteShapeTransition( const EShapeTransition shapeTransition, const float transitionTime )
{
	CRY_ASSERT(transitionTime > 0.0f);

	m_activeTransition.runningTime = 0.0f;
	m_activeTransition.transitionType = shapeTransition;
	m_activeTransition.transitionTime = transitionTime;
}

void CNanoSuitShapeDeformation::ExecuteShapePulse( const EShapePulse shapePulse, const float pulseTime )
{
	//Running pulse is kept till the end
	if (m_activePulse.pulseType == eSP_None)
	{
		CRY_ASSERT(pulseTime > 0.0f);

		m_activePulse.runningTime = 0.0f;
		m_activePulse.pulseType = shapePulse;
		m_activePulse.pulseTime = pulseTime;
	}
}

void CNanoSuitShapeDeformation::UpdateSuitModeChangeTransition( const float frameTime )
{
	if (m_activeTransition.runningTime < m_activeTransition.transitionTime)
	{
		const float timeFactor = clamp(m_activeTransition.runningTime/m_activeTransition.transitionTime, 0.0f, 1.0f);
		m_armsShapeTransitionDeformation = LERP(m_armsShapeTransitionDeformation, cry_cosf((timeFactor - 0.5f) * gf_PI) * 0.75f, clamp(timeFactor * 2.0f, 0.0f, 1.0f));;
	}
	else
	{
		m_armsShapeTransitionDeformation = 0.0f;
		m_activeTransition.transitionType = eST_None;
	}
}

void CNanoSuitShapeDeformation::UpdateSuitModeActivatePowerTransition( const float frameTime )
{
	if (m_activeTransition.runningTime < m_activeTransition.transitionTime)
	{
		const float timeFactor = clamp(m_activeTransition.runningTime/m_activeTransition.transitionTime, 0.0f, 1.0f);
		m_armsShapeTransitionDeformation = LERP(m_armsShapeTransitionDeformation, cry_sinf(timeFactor * gf_PI * 0.5f), clamp(timeFactor * 2.0f, 0.0f, 1.0f));;
	}
	else
	{
		const float timeFactor = (m_activeTransition.runningTime - m_activeTransition.transitionTime)/(m_activeTransition.transitionTime) * 2.25f;
		const float shapeDeform = 1.0f + ((cry_sinf(timeFactor * gf_PI)) * 0.175f) +  (cry_sinf((timeFactor - Random(0.4f, 0.5f) * gf_PI)) * 0.075f);
		m_armsShapeTransitionDeformation = LERP(m_armsShapeTransitionDeformation, shapeDeform, 0.66f);
	}
}

void CNanoSuitShapeDeformation::UpdateSuitModeActivateArmorTransition( const float frameTime )
{
	if (m_activeTransition.runningTime < m_activeTransition.transitionTime)
	{
		const float timeFactor = clamp(m_activeTransition.runningTime/m_activeTransition.transitionTime, 0.0f, 1.0f);
		const float increaseAmount = cry_cosf((timeFactor - 0.5f) * gf_PI) * 0.4f;
		m_armsShapeTransitionDeformation = LERP(m_armsShapeTransitionDeformation, (cry_sinf(timeFactor * gf_PI * 0.5f) * 1.4f) + increaseAmount, clamp(timeFactor * 2.0f, 0.0f, 1.0f));;
	}
	else
	{
		m_armsShapeTransitionDeformation = 1.2f;
	}
}

void CNanoSuitShapeDeformation::UpdateSuitModeDeactivateTransition( const float frameTime )
{
	if (m_activeTransition.runningTime < m_activeTransition.transitionTime)
	{
		const float timeFactor = clamp((1.0f - m_activeTransition.runningTime/m_activeTransition.transitionTime), 0.0f, 1.0f);
		m_armsShapeTransitionDeformation = LERP(m_armsShapeTransitionDeformation, cry_sinf(timeFactor * gf_PI * 0.5f), clamp(timeFactor * 2.0f, 0.0f, 1.0f));;
	}
	else
	{
		m_activeTransition.transitionType = eST_None;
		m_armsShapeTransitionDeformation = 0.0f;
	}
}

void CNanoSuitShapeDeformation::UpdateSuitHitPulse( const float frameTime )
{
	const float timeFactor = clamp((1.0f - m_activePulse.runningTime/m_activePulse.pulseTime), 0.0f, 1.0f);

	if (m_activePulse.runningTime < m_activePulse.pulseTime)
	{
		m_armsPulseShapeDeformation = LERP(m_armsPulseShapeDeformation, cry_sinf(timeFactor * (timeFactor * 8.0f) * gf_PI) * Random(0.4f, 0.5f), 0.66f);
	}
	else
	{
		m_armsPulseShapeDeformation = 0.0f;
		m_activePulse.pulseType = eSP_None;
	}
}


