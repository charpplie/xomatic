//==================================================================================================
// Name: CDeflectionGameEffect
// Desc: Deflection game effect
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include "DeflectionGameEffect.h"

REGISTER_EFFECT_DEBUG_DATA(CDeflectionGameEffect::DebugOnInputEvent,CDeflectionGameEffect::DebugDisplay,Deflection);

//--------------------------------------------------------------------------------------------------
// Name: CDeflectionGameEffect
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CDeflectionGameEffect::CDeflectionGameEffect()
{
	m_screenEffectId = InvalidEffectId;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CDeflectionGameEffect
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
CDeflectionGameEffect::~CDeflectionGameEffect()
{
	
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Initialise
// Desc: Initializes game effect
//--------------------------------------------------------------------------------------------------
void CDeflectionGameEffect::Initialise(const SGameEffectParams* gameEffectParams)
{
	CGameEffect::Initialise(gameEffectParams);

	SDeflectionGameEffectParams deflectionGameEffectParams;
	if(gameEffectParams)
	{
		deflectionGameEffectParams = *(SDeflectionGameEffectParams*)(gameEffectParams);
	}

	m_screenEffectId = deflectionGameEffectParams.screenEffectId;
	CRY_ASSERT_MESSAGE(m_screenEffectId,"Deflection effect initialised without a screen effect");
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Release
// Desc: Releases game effect
//--------------------------------------------------------------------------------------------------
void CDeflectionGameEffect::Release()
{
	CGameEffect::Release();
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Update
// Desc: Updates game effect
//--------------------------------------------------------------------------------------------------
void CDeflectionGameEffect::Update(float frameTime)
{
	CGameEffect::Update(frameTime);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetActive
// Desc: Sets active status
//--------------------------------------------------------------------------------------------------
void CDeflectionGameEffect::SetActive(bool isActive)
{
	CGameEffect::SetActive(isActive);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Deflect
// Desc: Displays deflection effect using the bullet direction
//--------------------------------------------------------------------------------------------------
void CDeflectionGameEffect::Deflect(const Quat& playerRot, const Vec3& bulletDir)
{
	if(IsFlagSet(GAME_EFFECT_ACTIVE))
	{
		// Calc hit direction by taking player rotation off bullet direction and reversing the direction
		Quat playerRotInverted = playerRot;
		playerRotInverted.Invert();
		Vec3 hitDir = bulletDir * playerRot;
		hitDir = -hitDir;

		// Avoid division by 0
		if(hitDir.x == 0.0f)
		{
			hitDir.x = 0.0001f;
		}

		Vec2 screenPos(0.0f,0.0f);
		float deflectAngle = fabs(atan(hitDir.y/hitDir.x));
		const float QUARTER_PI = gf_PI * 0.25f;

		// Calculate screen position within quarter of screen
		if(deflectAngle < QUARTER_PI)
		{
			screenPos.y = (deflectAngle / QUARTER_PI) * 0.5f;
		}
		else
		{
			screenPos.x = ((deflectAngle - QUARTER_PI) / QUARTER_PI) * 0.5f;
		}

		// Translate position to correct quarter of screen
		if(hitDir.y >= 0.0f) // Front
		{
			if(deflectAngle < QUARTER_PI) // left and right middle eighths
			{
				screenPos.y = 0.5f - screenPos.y;
			}
		}
		else // Back
		{
			if(deflectAngle < QUARTER_PI) // left and right middle eighths
			{
				screenPos.y = 0.5f + screenPos.y;
			}
			else // top and bottom eighths
			{
				screenPos.y = 1.0f;
			}
		}

		if(hitDir.x >= 0.0f) // Right
		{
			if(deflectAngle < QUARTER_PI) // Middle eighths
			{
				screenPos.x = 1.0f;
			}
			else // top and bottom eighths
			{
				screenPos.x = 1.0f - screenPos.x;
			}
		}

		// Start screen filter
		IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();

		SMFXRunTimeEffectParams effectParams;
		effectParams.pos = gEnv->pRenderer->GetCamera().GetPosition();
		effectParams.soundSemantic = eSoundSemantic_None;

		bool screenEffectExecuted = pMaterialEffects->ExecuteEffect(m_screenEffectId, effectParams);
		CRY_ASSERT_MESSAGE(screenEffectExecuted,"Deflection effect failed to execute screen effect");

		SMFXCustomParamValue paramPosX;
		paramPosX.fValue = screenPos.x;
		pMaterialEffects->SetCustomParameter(m_screenEffectId,"Intensity",paramPosX); // Use intensity param to pass x pos

		SMFXCustomParamValue paramPosY;
		paramPosY.fValue = screenPos.y;
		pMaterialEffects->SetCustomParameter(m_screenEffectId,"BlendOutTime",paramPosY); // Use blendOutTime param to pass y pos
	}
}//-------------------------------------------------------------------------------------------------

#if DEBUG_GAME_FX_SYSTEM
//--------------------------------------------------------------------------------------------------
// Name: DebugOnInputEvent
// Desc: Called when input events happen in debug builds
//--------------------------------------------------------------------------------------------------
void CDeflectionGameEffect::DebugOnInputEvent(int keyId)
{
	EntityId localClientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	IEntity* playerEntity = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(localClientId)->GetEntity();

	static CDeflectionGameEffect deflectionScreenEffect;

	// Initialise static version of effect
	if(!deflectionScreenEffect.IsFlagSet(GAME_EFFECT_INITIALISED))
	{
		SDeflectionGameEffectParams deflectionScreenEffectParams;
		deflectionScreenEffectParams.screenEffectId = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects()->GetEffectIdByName("cw2_player_fx","cw2_deflection");
		deflectionScreenEffectParams.autoUpdatesWhenActive = false;
		deflectionScreenEffect.Initialise(&deflectionScreenEffectParams);
		deflectionScreenEffect.SetActive(true);
	}

	switch(keyId)
	{
	case eKI_NP_1:
		{
			Vec3 bulletDir;
			bulletDir.Set(Random(-1.0f,1.0f),Random(-1.0f,1.0f),Random(-1.0f,1.0f));
			bulletDir.Normalize();
			deflectionScreenEffect.Deflect(playerEntity->GetRotation(),bulletDir);
			break;
		}
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: DebugDisplay
// Desc: Display when this effect is selected to debug through the game effects system
//--------------------------------------------------------------------------------------------------
void CDeflectionGameEffect::DebugDisplay(const Vec2& textStartPos,float textSize,float textYStep)
{
	ColorF textCol(1.0f,1.0f,0.0f,1.0f);
	Vec2 currentTextPos = textStartPos;
	gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"Deflect: NumPad 1");
}//-------------------------------------------------------------------------------------------------
#endif
