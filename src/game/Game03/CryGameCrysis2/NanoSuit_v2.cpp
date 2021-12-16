/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Nanosuit v2 
-------------------------------------------------------------------------
History:
- 3-3-2008: Created by Benito G.R.

*************************************************************************/

#include "StdAfx.h"

#include "NanoSuit_v2.h"
#include "Player.h"
#include <ItemParams.h>
#include "ItemParamReader.h"
#include "GameCVars.h"
#include "HUD/HUD.h"

#include "NanoModePower.h"
#include "NanoModeArmor.h"
#include "NanoModeStealth.h"
#include "NanoModeTactical.h"

#include "ScreenEffects.h"
#include "Game.h"

#include <IMaterialEffects.h>
#include "Item.h"

#include "GameRules.h"
#include "TeamPerks.h"
#include "GameForceFeedback.h"
#include <IForceFeedbackSystem.h>

#include <IVehicleSystem.h>

#include "ITargetTrackManager.h"

#define NANOSUIT_DATA_FILE	"scripts/entities/nanosuit/nanosuit.xml"

const float DELAYTIMEEVENT_AFTERACTIVATION = 0.7f;  // seconds

const char* CNanoSuit::s_nanoSuitModeName[eNanoSuitMode_Last] = 
{
	"Power",
	"Armor",
	"Stealth",
	"Tactical"
};

CNanoSuitParams& CNanoSuit::GetNanoSuitParams()
{
	static CNanoSuitParams s_nanoSuitParams;
	return s_nanoSuitParams;
}

//Constructor, destructor

CNanoSuit::CNanoSuit(CPlayer& _owner)
: m_suitOwner(_owner)
, s_nanoSuitParams(GetNanoSuitParams())
, m_lastState(eNanoSuitState_Normal)
, m_lastMode(eNanoSuitMode_Tactical)
{
	m_pNanoMode = NULL;
	Init();
}

CNanoSuit::~CNanoSuit()
{
	Release();
}

//-------------------------------------------------
void CNanoSuit::Init()
{
	CreateNanoModes();
	
	s_nanoSuitParams.AddNanoSuit(this);

	//Each suit needs some instanced copy of certain data
	m_specialSuitStates = s_nanoSuitParams.GetSuitStates();
	m_suitGameParams = s_nanoSuitParams.GetDefaultGameParams();

	m_pNanoMode = m_nanoModes[eNanoSuitMode_Power];
	
	Reset();
}

//-------------------------------------------------
void CNanoSuit::Reset()
{
	m_reseting = true;
	
	for (uint32 i=0; i<eNanoSuitMode_Last; ++i)
		m_allowedModes[i] = true;

	m_suitGameParams.Reset();
	m_suitWasStressed = false;
	m_lastState = m_suitGameParams.GetState();
	m_notifiedAfterModeActivation = false;
	m_suitModeTimer = 0.0f;
	m_modeTransitioning = false;
	m_hasSilhouette	= false;
	m_curretSuitShaderLayerTransitionWeight = 0.0f;

	OnSuitStateChanged();
	ActivateMode(eNanoSuitMode_Tactical);
	SynchNanoSuitMaterial();

	StopRunningSpecialStates();
	EndModeTransition();
	
	//Knife will be only created if not loaded already
	CreateSuitKnife();
	ShowKnife(false);

	m_suitShapeDeformation.ResetSuitShape(m_suitOwner.GetEntity());

	m_reseting = false;
}

//-------------------------------------------------
void CNanoSuit::CreateNanoModes()
{
	//WARNING - Keep order defined in NanoSuitDefs.h (enum ENanoSuitMode)
	m_nanoModes.push_back(new CNanoModePower(*this));
	m_nanoModes.push_back(new CNanoModeArmor(*this));
	m_nanoModes.push_back(new CNanoModeStealth(*this));
	m_nanoModes.push_back(new CNanoModeTactical(*this));
}

//-----------------------------------------------
void CNanoSuit::Release()
{
	m_reseting = true;

	m_pNanoMode->Activate(false);

	TNanoModeVector::iterator endIt = m_nanoModes.end();
	for(TNanoModeVector::iterator it = m_nanoModes.begin(); it!=endIt; ++it )
	{
		SAFE_RELEASE(*it);
	}

	m_pNanoMode = NULL;
	s_nanoSuitParams.RemoveNanoSuit(this);

	//Work around to prevent engine crash...
	if (IAttachment* pLightAttachment = GetCharacterAttachment(s_nanoSuitParams.GetLayerEffectParams().light_attachment.c_str()))
	{
		pLightAttachment->ClearBinding();
	}
}

//----------------------------------------------
void CNanoSuit::SetMaximumSuitMaterial(bool setMaterial)
{
	const CNanoSuitParams::SMaximumSuitPerk& maxSuitParams = GetNanoSuitParams().GetMaximumSuitPerkParams();

	IMaterial* material = (setMaterial) ? maxSuitParams.maximumSuitMaterial : maxSuitParams.nanoSuitMaterial;
	if(material)
	{
		SEntitySlotInfo slotInfo;
		IEntity* entity = m_suitOwner.GetEntity();
		if(entity)
		{
			entity->GetSlotInfo(0, slotInfo);
			if(slotInfo.pCharacter)
			{
				IAttachmentManager* attachmentManager = slotInfo.pCharacter->GetIAttachmentManager();
				if(attachmentManager)
				{
					const int attachmentCount = maxSuitParams.bodyAttachmentIndexArray.size();

					for(int i=0; i<attachmentCount; i++)
					{
						IAttachment* attachment = attachmentManager->GetInterfaceByIndex(maxSuitParams.bodyAttachmentIndexArray[i]);

						if(attachment)
						{
							IAttachmentObject* attachmentObj = attachment->GetIAttachmentObject();
							if(attachmentObj)
							{
								ICharacterInstance* charInstance = attachmentObj->GetICharacterInstance();
								if(charInstance)
								{
									charInstance->SetMaterial(material);
								}
							}
						}
					}
				}
			}
		}
	}
}

//----------------------------------------------
void CNanoSuit::ActivateMode(ENanoSuitMode mode)
{
	if((mode == m_suitGameParams.GetMode()) || (mode < 0) || (mode >= eNanoSuitMode_Last) || (!m_allowedModes[mode]) )
		return;

	m_suitModeTimer = 0.0f;

	ActivateSuitPower(false);
	m_pNanoMode->Activate(false);

	m_lastMode = m_suitGameParams.m_mode;
	m_suitGameParams.m_mode = mode; 
	m_pNanoMode = m_nanoModes[mode];
	m_suitGameParams.m_suitPowerActive = false; //Reset on mode change

	m_pNanoMode->Activate(true);

	m_notifiedAfterModeActivation = false;

	UpdateSoundParam();
	UpdateAIClassThreat();
	
	StartModeTransition();

	SynchNanoSuitMaterial();

	OnSuitModeChanged();
}


//----------------------------------------------
void CNanoSuit::AllowMode( ENanoSuitMode mode, bool allow )
{
	if (mode>=0 && mode<eNanoSuitMode_Last)
		m_allowedModes[mode] = allow;

	// activate another mode if current one is forbidden
	if (mode==m_suitGameParams.GetMode() && !allow)
	{
		for (uint32 i=0; i<eNanoSuitMode_Last; ++i)
		{
			if (m_allowedModes[i])
			{
				ActivateMode( ENanoSuitMode(i) )		;
				break;
			}
		}
	}

	// make sure cant forbid all modes	
	bool allAreForbidden = true;
	for (uint32 i=0; i<eNanoSuitMode_Last; ++i)
		allAreForbidden = allAreForbidden && !m_allowedModes[i];
		
	if (allAreForbidden)
	{
		m_allowedModes[eNanoSuitMode_Tactical] = true;
		ActivateMode( eNanoSuitMode_Tactical );
	}
}


//----------------------------------------------
bool CNanoSuit::IsModeAllowed( ENanoSuitMode mode )
{
	bool allowed = false;
	if (mode>=0 && mode<eNanoSuitMode_Last)
		allowed = m_allowedModes[mode];
	return allowed;
}


//-----------------------------------------------
void CNanoSuit::UpdateSoundParam()
{
	gEnv->pSoundSystem->SetGlobalParameter( "suit_mode", m_suitGameParams.GetSoundParamValue() );
}

//-----------------------------------------------
void CNanoSuit::UpdateAIClassThreat() const
{
	IAISystem *pAISystem = gEnv->pAISystem;
	ITargetTrackManager *pManager = pAISystem ? pAISystem->GetTargetTrackManager() : NULL;
	if (pManager)
	{
		assert(m_suitOwner.GetEntity());
		IAIObject *pAI = m_suitOwner.GetEntity()->GetAI();
		if (pAI)
		{
			const float fClassThreat = m_suitGameParams.GetProps().aiClassThreat;
			pManager->SetTargetClassThreat(pAI->GetAIObjectID(), fClassThreat);
		}
	}
}

//-----------------------------------------------
void CNanoSuit::ActivateLastMode()
{
	//ActivateMode(m_lastMode);
}

//-----------------------------------------------
void CNanoSuit::SynchNanoSuitMaterial()
{
	const CNanoSuitParams::SSuitLayerEffects& layerFxParams = s_nanoSuitParams.GetLayerEffectParams();

	SetLayerEffectParameters(layerFxParams.GetLayerConfig(m_suitGameParams.GetMode()), 1.0f);
}

//----------------------------------------------
void CNanoSuit::Update(float frameTime)
{
	if(gEnv->bMultiplayer)
	{
		UpdateOutlineColor();
	}

	CRY_TODO(8, 2, 2010, "Remove this when not needed anymore!");
	if(gEnv->bMultiplayer && g_pGameCVars->g_mpCoreGameplayTesting)	//Core gameplay testing
		return;

	UpdateSuitModeTimers(frameTime);

	const SPlayerStats* pStats = static_cast<const SPlayerStats*>(m_suitOwner.GetActorStats());
	CRY_ASSERT_MESSAGE(pStats, "Nano suit owner does not have stats!");

	const float prevEnergy = m_suitGameParams.GetEnergy().Get();

	UpdateCommon(frameTime);

	SNanoSuitEnergy::SUpdateContext energyUpdateCtx(frameTime, pStats->velocity.len(), m_suitGameParams.GetState());
	m_suitGameParams.GetEnergy().Update(energyUpdateCtx);
	
	if (prevEnergy<m_suitGameParams.GetEnergy().Get() && m_suitGameParams.GetEnergy().Get()==m_suitGameParams.GetEnergy().GetMaxEnergy())
	{
		ProcessSuitAction( "Suit_EnergyFullyRestored");
	}

	if (m_suitOwner.IsClient())
	{
		m_suitShapeDeformation.Update(m_suitOwner.GetEntity(), frameTime);
	}

	if (m_suitGameParams.GetState() != eNanoSuitState_Disabled)
	{
		m_pNanoMode->Update(frameTime);

		if (CGameRules *pGameRules = g_pGame->GetGameRules())
		{
			int teamId = pGameRules->GetTeam(m_suitOwner.GetEntityId());
			const CTeamPerks *pTeamPerks = pGameRules->GetTeamPerks(teamId);
			if (pTeamPerks && pTeamPerks->IsTeamPerkActive(eTeamPerk_SuitBoost))
			{
				m_suitGameParams.GetEnergy().ScaleEnergyUse(CPerk::GetInstance()->GetVars()->perk_teamSuitBoost_scaleEnergyUse);
			}
		}

		if (m_suitOwner.IsClient() && m_suitGameParams.GetMode() != eNanoSuitMode_Tactical)
		{
			float energy = m_suitGameParams.GetEnergy().Get() * (1.0f/m_suitGameParams.GetEnergy().GetMaxEnergy());
			bool recharging = m_suitGameParams.GetEnergy().IsRecharging();
			bool consuming = m_suitGameParams.IsSuitPowerActive() && m_suitGameParams.GetState() == eNanoSuitState_Normal;
			CAudioSignalPlayer* energyBarAudioSignal = s_nanoSuitParams.GetEnergyBarAudioSignal();

			if ((consuming || recharging) && !energyBarAudioSignal->IsPlaying())
			{
				energyBarAudioSignal->Play(m_suitOwner.GetEntityId(), "suit_energy", energy);
			}
			else if ((!consuming && !recharging) && energyBarAudioSignal->IsPlaying())
			{
				energyBarAudioSignal->Stop(m_suitOwner.GetEntityId());
			}

			if (consuming || recharging)
			{
				energyBarAudioSignal->SetParam(m_suitOwner.GetEntityId(), "suit_energy", energy);
			}
		}
	}

	PostStateUpdate(prevEnergy);

#ifdef DEBUG_NANOSUIT
	if(g_pGameCVars->pl_debug_suit != 0)
	{
		DebugDraw(frameTime);
	}
#endif

}

//////////////////////////////////////////////////////////////////////////
void CNanoSuit::UpdateOutlineColor()
{
	EntityId localClientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	int friendly = m_suitOwner.IsFriendlyEntity(localClientId);
	SSilhouette silhouette = friendly ? g_pGameCVars->g_teamSilhouette : g_pGameCVars->g_enemySilhouette;
	bool hasSilhouette = (m_suitOwner.GetEntityId() != localClientId) &&		/* Don't apply if player is client */
														(m_suitOwner.GetHealth() > 0) &&									/* Don't apply to dead players */
														(silhouette.enabled);															/* Allow CVar to turn on/off silhouettes */

#ifndef _RELEASE
	if(silhouette.enableUpdate)
	{
		m_hasSilhouette = false;
	}
#endif

	if(hasSilhouette != m_hasSilhouette)
	{
		if(hasSilhouette)
		{
			const float infiniteTime = (60.0f * 60.0f * 24.0f);
			g_pGame->GetHUD()->GetSilhouettes()->SetSilhouette(	m_suitOwner.GetEntity(), 
																													silhouette.r, 
																													silhouette.g, 
																													silhouette.b, 
																													silhouette.a, 
																													infiniteTime, 
																													true );
		}
		else
		{
			g_pGame->GetHUD()->GetSilhouettes()->ResetSilhouette(m_suitOwner.GetEntityId());
		}

		m_hasSilhouette = hasSilhouette;
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuit::UpdateSuitModeTimers(float frameTime)
{
	// Update mode timer
	const float newSuitTime = m_suitModeTimer + frameTime;
	
	m_suitModeTimer = newSuitTime;

	if (!m_notifiedAfterModeActivation && (newSuitTime >= DELAYTIMEEVENT_AFTERACTIVATION) && (m_suitGameParams.GetState() != eNanoSuitState_Disabled))
	{
		SNanoSuitEvent suitEvent; 
		suitEvent.event = eNanoSuitEvent_ONAFTER_MODE_ACTIVATION;
		m_pNanoMode->ProcessEvent( suitEvent );
		m_notifiedAfterModeActivation = true;
	}

	//UpdateActiveMode(frameTime);
	UpdateModeTransition();
}

//-------------------------------------------
void CNanoSuit::UpdateCommon(float frameTime)
{
	CNanoSuitParams::TNanoSuitSpecialStates::iterator it = m_specialSuitStates.begin();
	CNanoSuitParams::TNanoSuitSpecialStates::iterator end = m_specialSuitStates.end();

	for ( ; it != end; ++it)
	{
		if(!it->second.bRunning)
			continue;

		CNanoSuitParams::SNanoSuitSpecialState& state = it->second;

		//Update force feedback
		const CNanoSuitParams::SActionFFeedback& ffeedBack = state.readability.ffeedBack;
		if((ffeedBack.name.empty()) && (ffeedBack.time > 0.0f))
		{
			if(gEnv->pInput)
				gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, ffeedBack.time, ffeedBack.amplifierA, ffeedBack.amplifierB) );
		}

		//Update timers 
		// fExternaTimer < 0.0f --> Do not update the internal timer
		// fExternalTimer == 0.0f --> Use normal timer if applies
		// fExternalTimer > 0.0f --> Use external timer instead of internal
		if(state.fExternalTimer > 0.0f)
		{
			state.fRunningTime += frameTime;
			if(state.fRunningTime >= state.fExternalTimer)
			{
				ActivateSuitState(it->first.c_str(), false);
			}
		}
		else if(state.fTime > 0.0f && state.fExternalTimer == 0.0f)
		{
			state.fRunningTime += frameTime;
			if(state.fRunningTime >= state.fTime)
			{
				ActivateSuitState(it->first.c_str(), false);
			}
		}
	}

}

//--------------------------------------------
void CNanoSuit::PostStateUpdate(float prevEnergy)
{
	const float maxEnergy = m_suitGameParams.GetEnergy().GetMaxEnergy();
	const float currentEnergy = m_suitGameParams.GetEnergy().Get();
	
	switch(m_suitGameParams.GetState())
	{
	case eNanoSuitState_Normal:
		{
			if (currentEnergy <= 0.0f)
			{
				SwitchToSuitState(eNanoSuitState_Critical);
			}
			break;
		}
	
	case eNanoSuitState_Critical:
		{
			if (m_suitGameParams.GetEnergy().IsMercyTimeOver())
			{
				SwitchToSuitState(eNanoSuitState_Disabled);
			}
			else if (m_suitGameParams.GetEnergy().IsRecoveredFromMercyTime())
			{
				SwitchToSuitState(eNanoSuitState_Normal);
			}
			break;
		}

	case eNanoSuitState_Disabled:
		{
			if (m_suitGameParams.GetEnergy().IsReadyToReboot())
			{
				SwitchToSuitState(eNanoSuitState_Normal);
			}
			break;
		}

	case eNanoSuitState_Invalid:
		{
			CRY_ASSERT_MESSAGE(false, "Invalid NanoSuit state");
			break;
		}
	}

	if(prevEnergy != currentEnergy)
	{
		OnEnergyChanged(currentEnergy);
	}
}

//----------------------------------------------------
void CNanoSuit::StartModeTransition()
{
	const CNanoSuitParams::SSuitLayerEffects& layerFxParams = s_nanoSuitParams.GetLayerEffectParams();
	
	if (IAttachment* pLightAttachment = GetCharacterAttachment(layerFxParams.light_attachment.c_str()))
	{
		SetUpLightAttachment(pLightAttachment);
	}

	if (m_suitOwner.IsClient())
	{
		m_suitShapeDeformation.ExecuteShapeTransition(CNanoSuitShapeDeformation::eST_SuitModeChange, DELAYTIMEEVENT_AFTERACTIVATION);
	}

	m_modeTransitioning = true;
}

//----------------------------------------------------
void CNanoSuit::EndModeTransition()
{
	const CNanoSuitParams::SSuitLayerEffects& layerFxParams = s_nanoSuitParams.GetLayerEffectParams();

	if (IAttachment* pLightAttachment = GetCharacterAttachment(layerFxParams.light_attachment.c_str()))
	{
		pLightAttachment->HideAttachment(1);
		if (ILightSource* pLightSource = GetLightSource(pLightAttachment))
		{
			CDLight& lightProperties = pLightSource->GetLightProperties();
			lightProperties.m_Flags |= DLF_DISABLED;
		}
	}

	SetLayerEffectParameters(layerFxParams.GetLayerConfig(m_suitGameParams.GetMode()), 0.0f);

	m_modeTransitioning = false;
}

//-------------------------------------------------
void CNanoSuit::UpdateModeTransition()
{
	if (m_modeTransitioning == false)
		return;

	if (m_suitModeTimer < DELAYTIMEEVENT_AFTERACTIVATION)
	{
		const CNanoSuitParams::SSuitLayerEffects& layerFxParams = s_nanoSuitParams.GetLayerEffectParams();

		if (IAttachment* pLightAttachment = GetCharacterAttachment(layerFxParams.light_attachment.c_str()))
		{
			if (ILightSource* pLightSource = GetLightSource(pLightAttachment))
			{
				const float colorMult = cry_cosf((m_suitModeTimer/DELAYTIMEEVENT_AFTERACTIVATION - 0.5f) * (float)g_PI) * layerFxParams.light_diffuseMult;
				CDLight& lightProperties = pLightSource->GetLightProperties();
				lightProperties.SetLightColor(ColorF(layerFxParams.light_color.x * colorMult, layerFxParams.light_color.y * colorMult, layerFxParams.light_color.z * colorMult, 1.0f));
			}

			const float transitionMult = cry_cosf((m_suitModeTimer/DELAYTIMEEVENT_AFTERACTIVATION) * ((float)g_PI * 0.5f));
			SetLayerEffectParameters(layerFxParams.GetLayerConfig(m_suitGameParams.GetMode()), transitionMult);
		}

	}
	else
	{
		EndModeTransition();
	}
}

//------------------------------------------------
void CNanoSuit::UpdateActiveMode(float frameTime)
{
	// temporary visual effect to give some readability to the "disabled" mode. Should be replaced with something more specific.
	if (m_suitGameParams.GetState()==eNanoSuitState_Disabled)
	{
		const CNanoSuitParams::SSuitLayerEffects& layerFxParams = s_nanoSuitParams.GetLayerEffectParams();
		m_curretSuitShaderLayerTransitionWeight = 0.1f;
		SetLayerEffectParameters(Vec3(0,0,0), m_curretSuitShaderLayerTransitionWeight );
		return;
	} 
	//


	const float minimumWeight = 0.001f;
	float desiredLayerWeight = 0.0f;

	ENanoSuitMode suitMode = m_suitGameParams.GetMode();

	bool activateTransitionLayer = m_suitGameParams.IsSuitPowerActive() && (suitMode == eNanoSuitMode_Power);
	bool noUpdateNeeded = !activateTransitionLayer && (m_curretSuitShaderLayerTransitionWeight <= minimumWeight);

	if (noUpdateNeeded)
		return;

	if (activateTransitionLayer)
	{
		desiredLayerWeight = max(minimumWeight, pow(m_suitGameParams.GetEnergy().Get()/m_suitGameParams.GetEnergy().GetMaxEnergy(), 2.0f));
	}	

	Interpolate(m_curretSuitShaderLayerTransitionWeight, desiredLayerWeight, 4.0f, frameTime);
	const CNanoSuitParams::SSuitLayerEffects& layerFxParams = s_nanoSuitParams.GetLayerEffectParams();

	bool disabled = (m_curretSuitShaderLayerTransitionWeight <= minimumWeight);

	SetLayerEffectParameters(layerFxParams.GetLayerConfig(suitMode), disabled ? 0.0f : m_curretSuitShaderLayerTransitionWeight);

	if (IAttachment* pLightAttachment = GetCharacterAttachment(layerFxParams.light_attachment.c_str()))
	{
		if (ILightSource* pLightSource = GetLightSource(pLightAttachment))
		{
			CDLight& lightProperties = pLightSource->GetLightProperties();
			const float colorMult = m_curretSuitShaderLayerTransitionWeight * layerFxParams.light_diffuseMult;
			lightProperties.SetLightColor(ColorF(layerFxParams.light_color.x * colorMult, layerFxParams.light_color.y * colorMult, layerFxParams.light_color.z * colorMult, 1.0f));
			lightProperties.m_Flags = (disabled) ? lightProperties.m_Flags | DLF_DISABLED : lightProperties.m_Flags & ~DLF_DISABLED;
		}
	}
}

//-------------------------------------------------
void CNanoSuit::SetLayerEffectParameters(const Vec3& layerConfig, float transitionWeight)
{
	if (IEntityRenderProxy* pOwnerRenderProxy = static_cast<IEntityRenderProxy*>(m_suitOwner.GetEntity()->GetProxy(ENTITY_PROXY_RENDER)))
	{
		Vec4  layerParams[2];
		layerParams[0].x = layerConfig.x;
		layerParams[0].y = layerConfig.y;
		layerParams[0].z = layerConfig.z;
		layerParams[0].w = transitionWeight;
		layerParams[1].x = layerParams[1].x = layerParams[1].y = layerParams[1].z = layerParams[1].w = 0.0f;
//		pOwnerRenderProxy->SetEffectLayerParams(layerParams); //Diesel cut
	}
}

//-------------------------------------------------
IAttachment* CNanoSuit::GetCharacterAttachment(const char* attachmentName)
{
	assert(attachmentName);

	ICharacterInstance* pCharacterInstance = m_suitOwner.GetEntity()->GetCharacter(0);
	IAttachmentManager* pAttachmentMgr = pCharacterInstance ? pCharacterInstance->GetIAttachmentManager() : NULL;

	return pAttachmentMgr ? (pAttachmentMgr->GetInterfaceByName(attachmentName)) : NULL;
}

//--------------------------------------------------
ILightSource* CNanoSuit::GetLightSource(IAttachment* pAttachment)
{
	assert(pAttachment);

	IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject();

	if (pAttachmentObject && (pAttachmentObject->GetAttachmentType() == IAttachmentObject::eAttachment_Light))
	{
		CLightAttachment* pLightAttachment = static_cast<CLightAttachment*>(pAttachmentObject);
		return pLightAttachment->GetLightSource();
	}

	return NULL;
}

//--------------------------------------------------
void CNanoSuit::SetUpLightAttachment(IAttachment* pAttachment)
{
	assert(pAttachment);

	const CNanoSuitParams::SSuitLayerEffects& layerFxParams = s_nanoSuitParams.GetLayerEffectParams();

	CDLight lightProperties;
	lightProperties.SetLightColor(ColorF(0.0f, 0.0f, 0.0f, 1.0f));
	lightProperties.m_SpecMult = layerFxParams.light_specularMult;
	lightProperties.m_nLightStyle = 0;
	lightProperties.m_Origin.Set(0,0,0);
	lightProperties.m_fLightFrustumAngle = 45.0f;
	lightProperties.m_fRadius = layerFxParams.light_radius;
//	lightProperties.m_NumCM = -1; //Diesel cut
//	lightProperties.m_fLifeTime = 0; //Diesel cut
	lightProperties.m_fLightFrustumAngle = 0.0f;
	lightProperties.m_Flags |= DLF_DEFERRED_LIGHT;
	lightProperties.m_fHDRDynamic = layerFxParams.light_hdrDynamic;

	if (ILightSource* pLightSource = GetLightSource(pAttachment))
	{
		pLightSource->SetLightProperties(lightProperties);
	}
	else
	{
		CLightAttachment* pLightAttachment = new CLightAttachment();
		pLightAttachment->LoadLight(lightProperties);
		pAttachment->AddBinding(pLightAttachment);
	}

	pAttachment->HideAttachment(0);
}
//-------------------------------------------------
void CNanoSuit::ProcessEvent(const SNanoSuitEvent& event)
{
	switch(event.event)
	{
		case eNanoSuitEvent_SUITPOWER:
			{
				//At the moment the eNanoSuitEvent_SUITPOWER is analogous to the start sprinting control
				if(m_suitGameParams.GetState() != eNanoSuitState_Disabled)
				{
					ActivateSuitPower(!m_suitGameParams.IsSuitPowerActive());
				}
			}
			break;

		case eNanoSuitEvent_COMBO_LINK:
			{
				m_suitGameParams.m_state = eNanoSuitState_Normal;
				m_suitGameParams.GetEnergy().AddImmediateConsumption(m_suitGameParams.GetEnergy().GetMaxEnergy());
			}
			break;

		case eNanoSuitEvent_ACTION:
			{
				ProcessSuitAction(event.sParam, true);
			}
			break;

		case eNanoSuitEvent_SUIT_CLIENTSTARTUP:
			{
				if(g_pGameCVars->cl_suitModeSpecificScreenFX)
					ProcessSuitAction("Suit_StartUp", true);
			}
			break;

		case eNanoSuitEvent_DISABLE_SUIT:
			{
				SwitchToSuitState(eNanoSuitState_Disabled);
				
				float time = event.fParam;
				bool isPermanent = (time<0.f);
				bool isDefaultTime = (time==0.f);
				ActivateSuitState("Suit_Disabled", true);
				m_suitGameParams.GetEnergy().OnDisabled( isPermanent, isDefaultTime, time );
			}
			break;

		case eNanoSuitEvent_REBOOT_SUIT:
			{
				if(m_suitGameParams.GetState() != eNanoSuitState_Disabled)
					return;

				m_suitGameParams.GetEnergy().OnRebooted();

			}
			break;

		case eNanoSuitEvent_SLIDE_ACTIVE:
			{
				ActivateSuitState("Player_Sliding", event.bParam);
				if(event.bParam)
				{
					if(g_pGame->GetScreenEffects())
						g_pGame->GetScreenEffects()->ProcessSlidingFX();
				}
			}
			break;

		case eNanoSuitEvent_ENEMY_RAMMING_RUN:
			{
				ProcessSuitAction("Enemy_Ramming_Run", true);
			}
			break;
		case eNanoSuitEvent_ENEMY_RAMMING_JUMP:
			{
				ProcessSuitAction("Enemy_Ramming_Jump", true);
			}
			break;
		case eNanoSuitEvent_ENEMY_RAMMING_SLIDE:
			{
				ProcessSuitAction("Enemy_Ramming_Slide", true);
			}
			break;

		case eNanoSuitEvent_EMP_DISCHARGE:
			{
				ProcessSuitAction("Suit_EMP");
			}
			break;
		case eNanoSuitEvent_HIT:
			{
				if (GetGameParams().GetMode() == eNanoSuitMode_Armor)
				{
					ProcessSuitAction(m_suitGameParams.IsSuitPowerActive() ? "Suit_Hit_Armor_Active" : "Suit_Hit_Armor", true);
				}
				else
				{
					ProcessSuitAction("Suit_Hit", true);
					if (m_suitOwner.IsClient())
					{
						m_suitShapeDeformation.ExecuteShapePulse(CNanoSuitShapeDeformation::eSP_HitPulse, 0.4f);
					}
				}

				int32 health = m_suitOwner.GetHealth();
				if (health <= 0.0f)
					ProcessSuitAction("Suit_Death", true);
				else if (health < m_suitOwner.GetMaxHealth() * 0.15f)
					ProcessSuitAction("Suit_Damage_2", true);
				else if (health < m_suitOwner.GetMaxHealth() * 0.3f)
					ProcessSuitAction("Suit_Damage_1", true);
			}
			break;

		case eNanoSuitEvent_CLOAK_INTERFERENCE:
			static_cast<CNanoModeStealth*>(m_nanoModes[eNanoSuitMode_Stealth])->SetInterference(event.bParam);
			break;
	
		case eNanoSuitEvent_KVOLT_CHARGE:
			{
				if(m_suitGameParams.m_state == eNanoSuitState_Disabled)
				{
					CNanoSuitParams::TNanoSuitSpecialStates::iterator it = m_specialSuitStates.find(CONST_TEMPITEM_STRING("Suit_KVolt"));
					if(it != m_specialSuitStates.end())
					{
						CNanoSuitParams::SNanoSuitSpecialState* state = &it->second;
						state->fRunningTime = state->fExternalTimer - event.fParam; //Reset remaining time
					}		
				}
				else
				{
					StopRunningSpecialStates();
					ActivateSuitState("Suit_KVolt", true, event.fParam);
				}
			}
			break;


		case eNanoSuitEvent_ENERGY_DRAIN:
			m_suitGameParams.GetEnergy().AddImmediateConsumption(event.fParam);
			StopRunningSpecialStates();
			ActivateSuitState("Suit_EnergyDrain", true, 0.5f);
			break;

		case eNanoSuitEvent_CLOAK:
		case eNanoSuitEvent_UNCLOAK:
			m_nanoModes[eNanoSuitMode_Stealth]->ProcessEvent(event);
			break;
			
		case eNanoSuitEvent_STEALTH_KILL:
			{
				ProcessSuitAction("StealthKill");
			}
			break;

		case eNanoSuitEvent_AIRFRICTION:
			{
				ActivateSuitState("Player_AirFriction", event.bParam);
			}
			break;

		default:
			break;
	}

	m_pNanoMode->ProcessEvent(event);
}

//-----------------------------------------------
void CNanoSuit::ProcessSuitAction(const char* actionName, bool playSound /* = true */)
{
	if(IsReseting())
		return;

	CNanoSuitParams::TNanoSuitActions::iterator actionIt = s_nanoSuitParams.GetSuitActions().find(CONST_TEMPITEM_STRING(actionName));

#ifdef DEBUG_NANOSUIT
	if (g_pGameCVars->g_logSuitActions)
	{
		CryLog ("Frame %d: %s doing '%s' playSound=%s found=%s", gEnv->pRenderer->GetFrameID(), m_suitOwner.GetEntity()->GetName(), actionName, playSound ? "TRUE" : "FALSE", (actionIt != s_nanoSuitParams.GetSuitActions().end()) ? "TRUE" : "FALSE");
	}
#endif

	if(actionIt != s_nanoSuitParams.GetSuitActions().end())
	{
		bool suitOwnerIsClient = m_suitOwner.IsClient();

		//Update energy
		const CNanoSuitParams::SActionCost& energyCost = actionIt->second.cost;
		m_suitGameParams.GetEnergy().AddImmediateConsumption(energyCost.energyCost);

		//Sound
		if (playSound)
		{
			int slot = (m_suitOwner.IsThirdPerson() == false) ? 0 : 1;
			actionIt->second.audioSignals[slot].Play(m_suitOwner.GetEntityId());
		}

		//Client only
		if(suitOwnerIsClient)
		{
			//Force feedback
			const CNanoSuitParams::SActionFFeedback& fFeedback = actionIt->second.fFeedback;
			if (!fFeedback.name.empty())
			{
				g_pGame->GetIGameFramework()->GetIForceFeedbackSystem()->PlayForceFeedbackEffect(fFeedback.name.c_str(), 1.0f);
			}
			else if(fFeedback.time > 0.0f)
			{
				if(gEnv->pInput)
					gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, fFeedback.time, fFeedback.amplifierA, fFeedback.amplifierB) );
			}	

			const CNanoSuitParams::SActionScreenFX& screenFX = actionIt->second.screenFX;
			if (!screenFX.effectName.empty())
			{
					//Screen fx
				IMaterialEffects* pMaterialEffects = g_pGame->GetIGameFramework()->GetIMaterialEffects();
				CRY_ASSERT(pMaterialEffects);

				TMFXEffectId fxId = pMaterialEffects->GetEffectIdByName(screenFX.libraryName.c_str(), screenFX.effectName.c_str());
				if(fxId != InvalidEffectId)
				{
					SMFXRunTimeEffectParams fxParams;
					fxParams.pos = m_suitOwner.GetEntity()->GetWorldPos();
					fxParams.soundSemantic = eSoundSemantic_Player_Foley;
					pMaterialEffects->ExecuteEffect(fxId, fxParams);

					if(screenFX.blendOutTime > 0.0f)
					{
						SMFXCustomParamValue fxCustomParam;
						fxCustomParam.fValue = screenFX.blendOutTime;
						pMaterialEffects->SetCustomParameter(fxId, "BlendOutTime", fxCustomParam);
					}
				}
			}

			//Camera shaking
			const CNanoSuitParams::SActionCameraShake& camShake = actionIt->second.cameraShake;
			if(camShake.time > 0.0f)
			{
				CScreenEffects* pScreenFX = g_pGame->GetScreenEffects();
				CRY_ASSERT(pScreenFX);

				float attenuation = 1.0f;
				if (camShake.viewAngleAttenuation)
				{
					const Vec3 up(0, 0, 1);
					Vec3 upVector = m_suitOwner.GetViewQuat() * up;
					float viewDot = upVector * up;
					attenuation = viewDot * viewDot;
				}
				float time = camShake.time * attenuation;
				float frequency = camShake.frequency * attenuation;
				Vec3 shift = camShake.shift * attenuation;
				Vec3 rotate = camShake.rotate * attenuation;

				bool changeCamDir = camShake.randomDirection && (cry_rand()%2);
				if (changeCamDir)
				{
					shift.x = -shift.x;
					rotate.y = -rotate.y;
				}

				pScreenFX->CamShake(rotate, shift, time, frequency,	0.0f, CScreenEffects::eCS_GID_Nanosuit);
			}
		}
	}
}

//----------------------------------------------
void CNanoSuit::ActivateSuitState(const char* stateName, bool activate, float timer /* = 0.0f */, bool playSound /* = true */)
{
	CNanoSuitParams::TNanoSuitSpecialStates::iterator it = m_specialSuitStates.find(CONST_TEMPITEM_STRING(stateName));
	if(it != m_specialSuitStates.end())
	{
		int slot = m_suitOwner.IsThirdPerson() ? 1 : 0;

		CNanoSuitParams::SNanoSuitSpecialState& state = it->second;
		if(activate)
		{
			//Already running? Skip it...
			if(state.bRunning)
			{
				//Restart only timer
				if(state.bAllowRestart)
					state.fRunningTime = 0.0f;

				return;
			}

			state.bRunning = true;
			state.fExternalTimer = timer;
			ProcessSuitAction(state.onEnterAction.c_str(), playSound);

			//Start loop sound
			if(playSound)
			{
				state.readability.audioSignals[slot].Play(m_suitOwner.GetEntityId());
			}

			//Particle FX
			if(!state.readability.particleFX.empty())
			{
				IParticleEffect* pParticleEffect = gEnv->pParticleManager->FindEffect(state.readability.particleFX.c_str());
				if(pParticleEffect)
				{
					if (state.readability.isSurfaceFX)
					{
						SpawnParams spawnParams;
						spawnParams.eAttachType = GeomType_Render;
						spawnParams.eAttachForm = GeomForm_Surface;

						state.readability.particleFXSlot = m_suitOwner.GetEntity()->LoadParticleEmitter( -1, pParticleEffect, &spawnParams);
					}
					else
					{
						state.readability.particleFXSlot = m_suitOwner.GetEntity()->LoadParticleEmitter( -1, pParticleEffect);
					}

					Matrix34 localOffset;
					localOffset.SetIdentity(); localOffset.SetTranslation(state.readability.particleFXOffset);
					m_suitOwner.GetEntity()->SetSlotLocalTM(state.readability.particleFXSlot, localOffset);
				}
			}

			//Force feedback
			if (m_suitOwner.IsClient())
			{
				if (!state.readability.ffeedBack.name.empty())
				{
					g_pGame->GetIGameFramework()->GetIForceFeedbackSystem()->PlayForceFeedbackEffect(state.readability.ffeedBack.name.c_str(), 1.0f);
				}
			}

		}
		else
		{
			//Not running? Skip it...
			if(!state.bRunning)
				return;

			state.bRunning = false;
			state.fRunningTime = 0.0f;
			state.fExternalTimer = 0.0f;
			ProcessSuitAction(state.onLeaveAction.c_str(), playSound);

			//Stop looping sounds
			state.readability.audioSignals[0].Stop(m_suitOwner.GetEntityId());
			state.readability.audioSignals[1].Stop(m_suitOwner.GetEntityId());

			//Stop particle FX
			if(state.readability.particleFXSlot != -1)
			{
				m_suitOwner.GetEntity()->FreeSlot(state.readability.particleFXSlot);
				state.readability.particleFXSlot = -1;
			}

			//Force feedback
			if (m_suitOwner.IsClient())
			{
				if (!state.readability.ffeedBack.name.empty())
				{
					g_pGame->GetIGameFramework()->GetIForceFeedbackSystem()->StopForceFeedbackEffect(state.readability.ffeedBack.name.c_str());
				}
			}
		}
	}
}

//---------------------------------------------
void CNanoSuit::ActivateSuitPower(bool activate)
{
	if (activate == m_suitGameParams.IsSuitPowerActive())
		return;

	if (activate && !m_pNanoMode->CanActivateSuitPower())
		return;

	m_suitGameParams.m_suitPowerActive = activate;
	
	UpdateSoundParam();
	m_pNanoMode->ActivateSuitPower(activate);
	OnSuitPowerActivated(activate);
}

//---------------------------------------------
void CNanoSuit::StopRunningSpecialStates()
{
	CNanoSuitParams::TNanoSuitSpecialStates::const_iterator cit = m_specialSuitStates.begin();
	CNanoSuitParams::TNanoSuitSpecialStates::const_iterator cEnd = m_specialSuitStates.end();

	for ( ; cit != cEnd; ++cit)
	{
		if(cit->second.bRunning)
			ActivateSuitState(cit->first.c_str(), false);
	}
}

//----------------------------------------------
tSoundID CNanoSuit::PlaySound(const char* soundName, bool is2DSound)
{
	ESoundSemantic eSemantic = eSoundSemantic_NanoSuit;

	IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*)m_suitOwner.GetEntity()->CreateProxy(ENTITY_PROXY_SOUND);
	if (pSoundProxy && ((is2DSound && m_suitOwner.IsClient()) || !is2DSound))
		return pSoundProxy->PlaySound(soundName,Vec3(0.0f,0.3f,1.0f),Vec3(0.0f,1.0f,0.0f),is2DSound?FLAG_SOUND_3D:FLAG_SOUND_2D,eSemantic);

	return INVALID_SOUNDID;
}

//-------------------------------------------
void CNanoSuit::StopSound(tSoundID soundId)
{
	if(soundId == INVALID_SOUNDID)
		return;

	IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*)m_suitOwner.GetEntity()->CreateProxy(ENTITY_PROXY_SOUND);
	if (pSoundProxy)
		pSoundProxy->StopSound(soundId);
}

//--------------------------------------------
void CNanoSuit::CreateSuitKnife()
{
	IAttachment* pKnifeAttachment = GetCharacterAttachment(s_nanoSuitParams.GetSuitKnifeInfo().GetKnifeAttachment());
	if (pKnifeAttachment && (pKnifeAttachment->GetIAttachmentObject() == NULL))
	{
		CCGFAttachment* pCGFAttachment = new CCGFAttachment();
		assert (pCGFAttachment);
		
		IStatObj* pKnifeObject = gEnv->p3DEngine->LoadStatObj(s_nanoSuitParams.GetSuitKnifeInfo().GetKnifeObjectName()); 
		pCGFAttachment->pObj = pKnifeObject;
		pKnifeAttachment->AddBinding(pCGFAttachment);
		pKnifeAttachment->HideAttachment(1);
	}
}

//--------------------------------------------
void CNanoSuit::ShowKnife( bool show )
{
	IAttachment* pKnifeAttachment = GetCharacterAttachment(s_nanoSuitParams.GetSuitKnifeInfo().GetKnifeAttachment());
	if (pKnifeAttachment)
	{
		pKnifeAttachment->HideAttachment( show ? 0 : 1);
	}
}

//---------------------------------------------
void CNanoSuit::AddListener(CNanoSuit::INanoSuitListener *pListener)
{
	stl::push_back_unique(m_listeners, pListener);
}

//---------------------------------------------
void CNanoSuit::RemoveListener(CNanoSuit::INanoSuitListener *pListener)
{
	stl::find_and_erase(m_listeners, pListener);
}

//-------------------------------------------
void CNanoSuit::OnEnergyChanged(float prevEnergy)
{
	//Notify listeners
	if (!m_listeners.empty())
	{
		TNanoSuitListenersVector::iterator iter = m_listeners.begin();
		while (iter != m_listeners.end())
		{
			(*iter)->EnergyChanged(m_suitGameParams.GetEnergy().Get());
			++iter;
		}
	}
}

//-------------------------------------------
void CNanoSuit::OnSuitModeChanged()
{
	//Notify listeners
	if (!m_listeners.empty())
	{
		TNanoSuitListenersVector::iterator iter = m_listeners.begin();
		while (iter != m_listeners.end())
		{
			(*iter)->ModeChanged(m_suitGameParams.GetMode());
			++iter;
		}
	}
}

//-------------------------------------------
void CNanoSuit::OnSuitPowerActivated(const bool activated)
{
	//Notify listeners
	if (!m_listeners.empty())
	{
		TNanoSuitListenersVector::iterator iter = m_listeners.begin();
		while (iter != m_listeners.end())
		{
			(*iter)->OnSuitPowerActivated(activated);
			++iter;
		}
	}
}

//-------------------------------------------
void CNanoSuit::SwitchToSuitState( ENanoSuitState newState )
{
	if (m_suitGameParams.GetState() == newState)
		return;

	OnLeaveState(m_suitGameParams.GetState());

	m_suitGameParams.m_state = newState;
	m_lastState = newState;

	OnEnterState(m_suitGameParams.GetState());

	OnSuitStateChanged();
}

//-------------------------------------------
void CNanoSuit::OnEnterState(ENanoSuitState enteringState)
{
	switch(enteringState)
	{
	case eNanoSuitState_Normal:
		{
			break;
		}

	case eNanoSuitState_Critical:
		{
			break;
		}

	case eNanoSuitState_Disabled:
		{
			ActivateSuitPower(false);
			m_suitGameParams.GetEnergy().OnDisabled();
			break;
		}

	case eNanoSuitState_Invalid:
		{
			CRY_ASSERT_MESSAGE(false, "Trying to enter invalid nano suit state");
			break;
		}
	}
}

//-------------------------------------------
void CNanoSuit::OnLeaveState(ENanoSuitState leavingState)
{
	switch(leavingState)
	{
	case eNanoSuitState_Disabled:
		{
			ActivateSuitState("Suit_Disabled", false);
			break;
		}

	default:
		{
			break;
		}
	}
}
//-------------------------------------------
void CNanoSuit::OnSuitModeSelected(ENanoSuitMode mode)
{
	//Notify listeners
	if (!m_listeners.empty())
	{
		TNanoSuitListenersVector::iterator iter = m_listeners.begin();
		while (iter != m_listeners.end())
		{
			(*iter)->OnModeSelected(mode);
			++iter;
		}
	}
}



//--------------------------------------------
void CNanoSuit::OnSuitStateChanged()
{
	m_pNanoMode->OnStateChanged(m_suitGameParams.GetState());

	//Notify listeners
	if (!m_listeners.empty())
	{
		TNanoSuitListenersVector::iterator iter = m_listeners.begin();
		while (iter != m_listeners.end())
		{
			(*iter)->OnSuitStateChanged(m_suitGameParams.GetState());
			++iter;
		}
	}
}


//--------------------------------------------
void CNanoSuit::EnterSlowMotion()
{
	if (!gEnv->bMultiplayer && g_pGameCVars->g_SuitModeSlowMotionRatio<1.f)
	{
		gEnv->pTimer->SetTimeScale(g_pGameCVars->g_SuitModeSlowMotionRatio);
		ProcessSuitAction( "OnNanoSuit_Slowmo_Enter" );
	}
}


//--------------------------------------------
void CNanoSuit::ExitSlowMotion()
{
	if (!gEnv->bMultiplayer && g_pGameCVars->g_SuitModeSlowMotionRatio<1.f)
	{
		gEnv->pTimer->SetTimeScale( 1.f );
		ProcessSuitAction( "OnNanoSuit_Slowmo_Exit" );
	}
}



//------------------------------------------
void CNanoSuit::NetSerialize(TSerialize ser, EEntityAspects aspect )
{
	if (aspect&CPlayer::ASPECT_NANO_SUIT_SETTING)
	{
		uint8 mode = m_suitGameParams.GetMode();
		bool suitPower = m_suitGameParams.IsSuitPowerActive();
		ser.Value("mode", mode, 'ui3');
		ser.Value("suitPower", suitPower, 'bool');
		if(ser.IsReading())
		{
			if (mode != m_suitGameParams.GetMode())
				ActivateMode((ENanoSuitMode)mode);
			if (suitPower != m_suitGameParams.IsSuitPowerActive())
				ActivateSuitPower(suitPower);
		}
	}

	if (aspect&CPlayer::ASPECT_NANO_SUIT_ENERGY)
	{
		float fEnergy = m_suitGameParams.GetEnergy().Get();
		ser.Value("energy", fEnergy, 'nNRG');
		if(ser.IsReading())
			m_suitGameParams.GetEnergy().NetSet(fEnergy);
	}
}

//---------------------------------------------------------
void CNanoSuit::ReloadSuitData()
{
	if (GetNanoSuitParams().IsDataLoaded())
	{
		GetNanoSuitParams().InvalidateData();
	}
	
	GetNanoSuitParams().LoadXMLData(NANOSUIT_DATA_FILE);
}

//---------------------------------------------------------
void CNanoSuit::OnSuitDataChanged()
{
	//Gather shared data again
	m_specialSuitStates = s_nanoSuitParams.GetSuitStates();
	m_suitGameParams = s_nanoSuitParams.GetDefaultGameParams();

	Reset();
}

//---------------------------------------------------------
void CNanoSuit::GetMemoryUsage(ICrySizer* s) const
{
	s->Add(*this);
	m_suitGameParams.GetMemoryStatistics(s);

	s->AddObject(m_nanoModes);	
	s->AddObject(m_specialSuitStates);	
	s->AddObject(m_listeners);

}

//----------------------------------------------------
#ifdef DEBUG_NANOSUIT
void CNanoSuit::DebugDraw(float frameTime)
{
	if(m_suitOwner.IsClient())
	{
		float green[4] = {0.0f,1.0f,0.0f,1.0f};
		float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
		bool cloakActive = m_suitGameParams.IsCloakEnabled();
		bool armorActive = m_suitGameParams.IsSuitPowerActive() && (m_suitGameParams.GetMode() == eNanoSuitMode_Armor);
		
		//const SNanoSuitGameParameters& debugParams = (m_suitGameParams.GetState()&eNanoSuitState_Disabled) ? m_defaultGameParams : m_suitGameParams;
		const SNanoSuitGameParameters& debugParams = m_suitGameParams;

		gEnv->pRenderer->Draw2dLabel(50.0f,50.0f,1.5f,green,false,"Base Speed scale : %.2f", debugParams.GetProps().speedScale);
		gEnv->pRenderer->Draw2dLabel(50.0f,65.0f,1.5f,green,false,"Sprint speed scale : %.2f", debugParams.GetProps().sprintSpeedScale);
		gEnv->pRenderer->Draw2dLabel(50.0f,80.0f,1.5f,green,false,"Rotation speed scale : %.2f", debugParams.GetProps().rotationSpeedScale);
		gEnv->pRenderer->Draw2dLabel(50.0f,95.0f,1.5f,green,false,"Super Jump scale : %.2f", debugParams.GetProps().superJumpScale);
		gEnv->pRenderer->Draw2dLabel(50.0f,110.0f,1.5f,green,false,"Melee Damage scale : %.2f", debugParams.GetProps().meleeDamageScale);
		gEnv->pRenderer->Draw2dLabel(50.0f,125.0f,1.5f,green,false,"Melee Strength scale : %.2f", debugParams.GetProps().meleeStrengthScale);
		gEnv->pRenderer->Draw2dLabel(50.0f,155.0f,1.5f,green,false,"Melee Strength scale (Vehicles): %.2f", debugParams.GetProps().meleeStrengthScale);
		gEnv->pRenderer->Draw2dLabel(50.0f,170.0f,1.5f,green,false,"Throw Strength scale : %.2f", debugParams.GetProps().throwStrengthScale);
		gEnv->pRenderer->Draw2dLabel(50.0f,185.0f,1.5f,green,false,"Camouflage : %.2f", debugParams.GetProps().camouflage);
		gEnv->pRenderer->Draw2dLabel(50.0f,200.0f,1.5f,green,false,"Noise supression : %.2f", debugParams.GetProps().noiseSupression);
		gEnv->pRenderer->Draw2dLabel(50.0f,215.0f,1.5f,green,false,"Damage absorption : %.2f", debugParams.GetProps().damageAbsorption);
		gEnv->pRenderer->Draw2dLabel(50.0f,230.0f,1.5f,green,false,"Damage absorption Melee: %.2f", debugParams.GetProps().damageAbsorptionMelee);
		gEnv->pRenderer->Draw2dLabel(50.0f,245.0f,1.5f,green,false,"Throw Strength scale : %.2f", debugParams.GetProps().hitReactionAbsorption);
		gEnv->pRenderer->Draw2dLabel(50.0f,260.0f,1.5f,green,false,"AI Target Class Threat : %.2f", debugParams.GetProps().aiClassThreat);
	}
}
#endif //DEBUG_NANOSUIT

const char* CNanoSuit::GetNanoSuitModeName(int mode)
{
	CRY_ASSERT(mode > eNanoSuitMode_Invalid && mode < eNanoSuitMode_Last);
	return s_nanoSuitModeName[mode];
}

/////////////////////////////////////////////
// Serialization
/////////////////////////////////////////////

void CNanoSuit::Serialize( TSerialize ser )
{
	m_suitGameParams.Serialize(ser);

	// Finally on read we need to activate the loaded parameters
	if(ser.IsReading())
	{
		int suitMode = static_cast<int>(m_suitGameParams.m_mode);
		int suitState = static_cast<int>(m_suitGameParams.m_state);
		bool powerActive = m_suitGameParams.IsSuitPowerActive();

		// Prevent cache checks from early exit
		m_suitGameParams.m_mode = eNanoSuitMode_Invalid;
		m_suitGameParams.m_state = eNanoSuitState_Invalid;

		// These may well have to be pulled out into a post-serialize step
		ActivateMode((ENanoSuitMode)suitMode);
		SwitchToSuitState((ENanoSuitState)suitState);
		if (powerActive)
			ActivateSuitPower(true);

		CRY_ASSERT(m_suitGameParams.m_mode == suitMode);
		CRY_ASSERT(m_suitGameParams.m_state == suitState);
		CRY_ASSERT(m_suitGameParams.m_suitPowerActive == powerActive);
	}
}
