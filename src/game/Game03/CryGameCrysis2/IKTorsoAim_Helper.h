/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Helper class to setup / update ik torso aim for first person 

-------------------------------------------------------------------------
History:
- 20-8-2009		Benito Gangoso Rodriguez
- 09-9-2009		Tom Berry - reintegrated into non-component system

*************************************************************************/

#pragma once

#ifndef _IKTORSOAIM_HELPER_H_
#define _IKTORSOAIM_HELPER_H_

#include <ICryAnimation.h>
#include "ITransformationPinning.h"
#include "Animation/PoseModifier/IKTorsoAim.h"

class CIKTorsoAim_Helper
{
public:

	struct SIKTorsoParams
	{
		explicit SIKTorsoParams(ICharacterInstance *_character, ICharacterInstance *_shadowCharacter, Vec3 _aimDirection, Vec3 _viewOffset, Vec3 _targetPosition, bool _updateTranslationPinning = true, float _blendRate = 3.0f, float _cameraLockBlendRate = 3.0f)
			: character(_character)
			, shadowCharacter(_shadowCharacter)
			, aimDirection(_aimDirection)
			, blendRate(_blendRate)
			, cameraLockBlendRate(_cameraLockBlendRate)
			, viewOffset(_viewOffset)
			, targetPosition(_targetPosition)
			, updateTranslationPinning(_updateTranslationPinning)
		{

		}

		ICharacterInstance *character;
		ICharacterInstance *shadowCharacter;
		Vec3 aimDirection;
		Vec3 viewOffset;
		Vec3 targetPosition;
		float blendRate;
		float cameraLockBlendRate;
		bool updateTranslationPinning;
	};

public:
	
	CIKTorsoAim_Helper();

	void Update(SIKTorsoParams& ikTorsoParams);
	void Enable(bool snap = false);
	void Disable(bool snap = false);
	void Reset()
	{
		m_initialized = false;
	}
	bool IsEnabled() const
	{
		return m_enabled;
	}

	void SetCameraBonePinning(bool pin)
	{
		m_cameraBoneLocked = pin;
	}
	bool GetCameraBonePinning() const
	{
		return m_cameraBoneLocked;
	}

	const Vec3& GetTorsoOffset() const {return m_actualTorsoOffset;}

private:
	
	void Init(SIKTorsoParams& ikTorsoParams);

	cryshared_ptr<CIKTorsoAim> m_ikTorsoAim;
	ITransformationPinningPtr  m_transformationPin;
	
	bool m_initialized;
	bool m_enabled;
	bool m_cameraBoneLocked;

	float m_blendTime;

	float m_cameraLockFactor;

	Vec3 m_actualTorsoOffset;
};

#endif