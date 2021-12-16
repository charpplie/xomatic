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

#pragma once

#ifndef _SUIT_SHAPE_DEFORMATION_H_
#define _SUIT_SHAPE_DEFORMATION_H_

#define SUIT_SHAPE_ARRAY_SIZE 8

#define SUIT_SHAPE_ARMS_IDX 3

class CNanoSuitShapeDeformation
{
public:

	enum EShapeTransition
	{
		eST_None = 0,
		eST_SuitModeChange,
		eST_SuitModeActivatePower,
		eST_SuitModeActivateArmor,
		eST_SuitModeDeactivate,
	};

	enum EShapePulse
	{
		eSP_None = 0,
		eSP_HitPulse,
	};

private:

	struct STransitionInfo
	{
		STransitionInfo()
		{
			Reset();
		}

		void Reset()
		{
			runningTime = 0.0f;
			transitionTime = 0.5f;
			transitionType = CNanoSuitShapeDeformation::eST_None;
		}

		float runningTime;
		float transitionTime;
		EShapeTransition transitionType;
	};

	struct SPulseInfo
	{
		SPulseInfo()
		{
			Reset();
		}

		void Reset()
		{
			runningTime = 0.0f;
			pulseTime = 0.5f;
			pulseType = CNanoSuitShapeDeformation::eSP_None;
		}

		float runningTime;
		float pulseTime;
		EShapePulse			 pulseType;
	};

public:

	CNanoSuitShapeDeformation();

	void ResetSuitShape(IEntity* pSuitOwnerEntity);

	void Update(IEntity* pSuitOwnerEntity, const float frameTime);

	void ExecuteShapeTransition(const CNanoSuitShapeDeformation::EShapeTransition shapeTransition, const float transitionTime);
	void ExecuteShapePulse(const CNanoSuitShapeDeformation::EShapePulse shapePulse, const float pulseTime);

	ILINE bool IsAnyTransitionEnabled() const { return (m_activeTransition.transitionType != eST_None); }

private:

	typedef void (CNanoSuitShapeDeformation::*UpdateTransitionFncPtr) (const float);
	typedef void (CNanoSuitShapeDeformation::*UpdatePulseFncPtr) (const float);

	void UpdateCurrentTransition(const float frameTime);
	void UpdateCurrentPulse(const float frameTime);

	void UpdateSuitModeChangeTransition(const float frameTime);
	void UpdateSuitModeActivatePowerTransition(const float frameTime);
	void UpdateSuitModeActivateArmorTransition(const float frameTime);
	void UpdateSuitModeDeactivateTransition(const float frameTime);

	void UpdateSuitHitPulse(const float frameTime);

	void UpdateCharacterShape(IEntity* pSuitOwnerEntity);

	f32 m_armsShapeTransitionDeformation;
	f32 m_armsPulseShapeDeformation;

	STransitionInfo m_activeTransition;
	SPulseInfo m_activePulse;
};

#endif