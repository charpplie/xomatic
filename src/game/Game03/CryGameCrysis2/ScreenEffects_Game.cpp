//---------------------------------------------------------
// ScreenEffects_Game - List of effects called from game
//
//	21-5-2008: Benito G.R.
//-------------------------------------------------------

#include "StdAfx.h"
#include "ScreenEffects.h"

#include "BlendTypes.h"
#include "BlendedEffect.h"
#include "BlendNode.h"

#include "GameCVars.h"

#include <IActorSystem.h>
#include <IMovementController.h>


//------------------------------------------------------------------------
void CScreenEffects::ProcessEndScaledEffect(const char* postFX, float fadeOutTime)
{
	//Blur effect (used for smoke grenades)
	if(fadeOutTime <= 0.0f)
	{
		gEnv->p3DEngine->SetPostEffectParam(postFX, 0.0f);
	}
	else
	{
		IBlendedEffect *blur = CBlendedEffect<CPostProcessEffect>::Create(CPostProcessEffect(postFX, 0.0f));
		IBlendType *linear = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
		StartBlend(blur, linear, 1.0f/fadeOutTime, CScreenEffects::eSFX_GID_SmokeBlur);
	}
}

//-------------------------------------------------------------------------
void CScreenEffects::ProcessExplosionEffect(float blurRadius, const Vec3& explosionPos)
{
	//Enable radial-blur postFX on near explosions
	gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Radius", blurRadius);
	gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Amount", 1.0f);

	IBlendedEffect *pBlur	= CBlendedEffect<CPostProcessEffect>::Create(CPostProcessEffect("FilterRadialBlurring_Amount", 0.0f));
	IBlendType   *pLinear = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
	StartBlend(pBlur, pLinear, 1.0f, CScreenEffects::eSFX_GID_RBlur);
	SetUpdateCoords("FilterRadialBlurring_ScreenPosX","FilterRadialBlurring_ScreenPosY", explosionPos);
}

//-------------------------------------------------------------------------
void CScreenEffects::ProcessFreeFallingFX(bool enable)
{
	//Radial-blur postFX for free falling state
	float currentRBlur = 0.0f;
	//if(!enable)
	gEnv->p3DEngine->GetPostEffectParam("FilterRadialBlurring_Amount", currentRBlur);

	ResetBlendGroup(CScreenEffects::eSFX_GID_RBlur);

	gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Radius", 0.65f);
	gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Amount", currentRBlur);

	IBlendedEffect *pBlur	= CBlendedEffect<CPostProcessEffect>::Create(CPostProcessEffect("FilterRadialBlurring_Amount", enable ? 0.7f : 0.0f));
	IBlendType   *pLinear = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
	StartBlend(pBlur, pLinear, 1.0f, CScreenEffects::eSFX_GID_RBlur);
	//SetUpdateCoords("FilterRadialBlurring_ScreenPosX","FilterRadialBlurring_ScreenPosY", explosionPos);
}

//--------------------------------------------------------------------------
void CScreenEffects::ProcessZoomInEffect()
{
	//Disable some blend groups while zoomed-in
	EnableBlends(false, CScreenEffects::eSFX_GID_ZoomIn);
	EnableBlends(false, CScreenEffects::eSFX_GID_ZoomOut);
	EnableBlends(false, CScreenEffects::eSFX_GID_HitReaction);
}

//-----------------------------------------------------------------------
void CScreenEffects::ProcessZoomOutEffect()
{
	//Renable blend groups after zooming out
	ResetBlendGroup(CScreenEffects::eSFX_GID_ZoomOut);
	ResetBlendGroup(CScreenEffects::eSFX_GID_ZoomIn);
	ResetBlendGroup(CScreenEffects::eSFX_GID_HitReaction);
	EnableBlends(true, CScreenEffects::eSFX_GID_ZoomIn);
	EnableBlends(true, CScreenEffects::eSFX_GID_ZoomOut);
	EnableBlends(true, CScreenEffects::eSFX_GID_HitReaction);
}

//----------------------------------------------------------------
void CScreenEffects::ProcessSuitStressed(bool stressed)
{
	ResetBlendGroup(CScreenEffects::eSFX_GID_SuitInterference);

	if(stressed)
	{
		gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("FilterSharpening_Amount", 1.0f);
		gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("FilterSharpening_Type", 2.0f);

		IBlendedEffect *pInterference	= CBlendedEffect<CPostProcessEffect>::Create(CPostProcessEffect("FilterSharpening_Amount", 4.0f));
		IBlendType   *pLinear = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
		StartBlend(pInterference, pLinear, 3.0f, CScreenEffects::eSFX_GID_SuitInterference);

		gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("AlienInterference_Amount", 0.0f);

		IBlendedEffect *pInterferenceGrain	= CBlendedEffect<CPostProcessEffect>::Create(CPostProcessEffect("AlienInterference_Amount", 0.3f));
		IBlendType   *pLinearGrain = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
		StartBlend(pInterferenceGrain, pLinearGrain, 1.5f, CScreenEffects::eSFX_GID_SuitInterference);
	}
	else
	{
		gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("FilterSharpening_Amount", 4.0f);
		gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("FilterSharpening_Type", 0.0f);

		IBlendedEffect *pInterference	= CBlendedEffect<CPostProcessEffect>::Create(CPostProcessEffect("FilterSharpening_Amount", 1.0f));
		IBlendType   *pLinear = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
		StartBlend(pInterference, pLinear, 3.0f, CScreenEffects::eSFX_GID_SuitInterference);

		gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("AlienInterference_Amount", 0.3f);

		IBlendedEffect *pInterferenceGrain	= CBlendedEffect<CPostProcessEffect>::Create(CPostProcessEffect("AlienInterference_Amount", 0.0f));
		IBlendType   *pLinearGrain = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
		StartBlend(pInterferenceGrain, pLinearGrain, 1.5f, CScreenEffects::eSFX_GID_SuitInterference);

	}
}

//-----------------------------------------------------------
void CScreenEffects::ProcessSlidingFX()
{
	//Enable radial-blur postFX when entering sliding
	gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Radius", 0.8f);
	gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Amount", 0.7f);
	gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_ScreenPosX", 0.5f);
	gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_ScreenPosY", 0.5f);

	IBlendedEffect *pBlur	= CBlendedEffect<CPostProcessEffect>::Create(CPostProcessEffect("FilterRadialBlurring_Amount", 0.0f));
	IBlendType   *pLinear = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
	StartBlend(pBlur, pLinear, 0.8f, CScreenEffects::eSFX_GID_RBlur);

	//Add also some view shake
	//CamShake(Vec3(0.02f, 0.03f, 0.007f), Vec3(0.15f, 0.15f, -0.1f), 0.15f, 0.8f);
}

//-----------------------------------------------------------
void CScreenEffects::ProcessSprintingFX(bool sprinting, bool isInAir)
{
	ClearBlendGroup(CScreenEffects::eSFX_GID_RBlur);

	if(sprinting)
	{
		//Enable radial-blur postFX when entering sprinting
		gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Radius", 0.85f);
		gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Amount", 0.6f);
		gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_ScreenPosX", 0.5f);
		gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_ScreenPosY", 0.5f);

		// Temporary multiplayer only value -> Screen Effects are going to be refactored so they are data driven
		float radialBlurAmount = 0.4f;
		if(gEnv->bMultiplayer)
		{
			radialBlurAmount = 0.2f;
		}
		IBlendedEffect *pBlur	= CBlendedEffect<CPostProcessEffect>::Create(CPostProcessEffect("FilterRadialBlurring_Amount", radialBlurAmount));
		IBlendType   *pLinear = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
		StartBlend(pBlur, pLinear, 1.2f, CScreenEffects::eSFX_GID_RBlur);

		//Zoom in effect
		ResetBlendGroup(CScreenEffects::eSFX_GID_ZoomIn);
		ResetBlendGroup(CScreenEffects::eSFX_GID_ZoomOut);
		{
			const float fovScale = g_pGameCVars->pl_movement.power_sprint_targetFov / g_pGameCVars->cl_fov;

			IBlendType   *blend = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
			IBlendedEffect *fovEffect	= CBlendedEffect<CFOVEffect>::Create(CFOVEffect(fovScale));

			StartBlend(fovEffect, blend, 1.0f, CScreenEffects::eSFX_GID_ZoomIn);
		}
	}
	else
	{
		//Blend out on exit
		//Enable radial-blur postFX when entering sprinting
		gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Radius", 0.85f);
		gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Amount", 0.4f);
		gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_ScreenPosX", 0.5f);
		gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_ScreenPosY", 0.5f);

		IBlendedEffect *pBlur	= CBlendedEffect<CPostProcessEffect>::Create(CPostProcessEffect("FilterRadialBlurring_Amount", 0.0f));
		IBlendType   *pLinear = CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
		StartBlend(pBlur, pLinear, 1.2f, CScreenEffects::eSFX_GID_RBlur);

		//Zoom out effect
		ResetBlendGroup(CScreenEffects::eSFX_GID_ZoomIn);
		ResetBlendGroup(CScreenEffects::eSFX_GID_ZoomOut);

		const float effectSpeed = isInAir ? 0.5f : 2.5f;

		IBlendedEffect *fov	= CBlendedEffect<CFOVEffect>::Create(CFOVEffect(1.0f));
		IBlendType *blend		= CBlendType<CLinearBlend>::Create(CLinearBlend(1.0f));
		StartBlend(fov, blend, effectSpeed, CScreenEffects::eSFX_GID_ZoomOut);
	}

}