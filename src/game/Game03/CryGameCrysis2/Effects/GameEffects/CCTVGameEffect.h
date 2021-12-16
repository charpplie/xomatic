#ifndef _CCTV_GAME_EFFECT_
#define _CCTV_GAME_EFFECT_

#pragma once

// Includes
#include "GameEffect.h"
#include "Effects/GameEffectsSystem.h"
#include "Effects/HUDEventListeners/CCTVHudEventListener.h"
#include "Effects/Tools/PostEffectActivationSystem.h"
#include "Effects/Tools/CVarActivationSystem.h"
#include "Audio/AudioSignalPlayer.h"

// Forward declares
class CScreenFilterRenderNode;

//--------------------------------------------------------------------------------------------------
// Name: ECCTV_Noise_State
// Desc: CCTV effect noise state
//--------------------------------------------------------------------------------------------------
enum ECCTV_Noise_State
{
	eCCTV_Noise_Standard = 0,
	eCCTV_Noise_Intense,
	eCCTV_Noise_State_Count
};//------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: CCCTVNoiseControlData
// Desc: Data to control noise for CCTV effect
//--------------------------------------------------------------------------------------------------
class CCCTVNoiseControlData
{
public:
	CCCTVNoiseControlData();

	void Initialise(const IItemParamsNode* xmlNode);

	void RandomiseNoise();

	float GetTime()
	{
		return m_currentTime;
	};

	float GetValue()
	{
		return m_currentValue;
	};

	float GetAudioLevel()
	{
		return m_currentAudio;
	}

private:

	float m_minValue;
	float m_maxValue;
	float m_currentValue;

	float m_minTime;
	float m_maxTime;
	float m_currentTime;

	float m_minAudio;
	float m_maxAudio;
	float m_currentAudio;

};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CCCTVGameEffect
// Desc: CCTV game effect
// Author: James Chilvers
//==================================================================================================
class CCCTVGameEffect : public CGameEffect
{
public:
	CCCTVGameEffect();
	~CCCTVGameEffect();

	void	Initialise(const SGameEffectParams* gameEffectParams = NULL);
	void	Release();
	void	Update(float frameTime);

	void	SetActive(bool isActive);

	void	SetCameraEntityId(EntityId cameraEntityId);

#if DEBUG_GAME_FX_SYSTEM
	static void  DebugOnInputEvent(int keyId);
	static void	 DebugDisplay(const Vec2& textStartPos,float textSize,float textYStep);
#endif

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));	
		pSizer->AddObject(m_noiseAudio);
		pSizer->AddObject(m_postEffectActivationSystem);		
	}
private:

	void	Start();
	void	Stop();

	CAudioSignalPlayer						m_noiseAudio;
	CCCTVHudEventListener					m_CCTVHudListener;
	CPostEffectActivationSystem		m_postEffectActivationSystem;
	CCVarActivationSystem					m_cvarActivationSystem;
	CCCTVNoiseControlData					m_noiseControlData[eCCTV_Noise_State_Count];
	CScreenFilterRenderNode*			m_renderNode;

	float													m_countDownTimer;
	ECCTV_Noise_State							m_noiseState;
	
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: SCCTVGameEffectParams
// Desc: CCTV game effect parameters
// Author: James Chilvers
//==================================================================================================
struct SCCTVGameEffectParams : public SGameEffectParams
{
	SCCTVGameEffectParams()
	{
		xmlNode = NULL;
	}

	const IItemParamsNode*	xmlNode;

};//------------------------------------------------------------------------------------------------

#endif // _CCTV_GAME_EFFECT_
