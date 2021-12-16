#include "StdAfx.h"
#include "HUD_Visor.h"
#include "HUD/HUD.h"
#include "HUD/HUDDefines.h"
#include "HUD/HUDMissionObjectiveSystem.h"
#include "HUD/HUD_UnifiedAsset.h"

#include "Player.h"
#include "Game.h"
#include "GameCVars.h"
#include "TacticalManager.h"

//////////////////////////////////////////////////////////////////////////


CHUD_Visor::CHUD_Visor()
: m_dirty(CHUD_Visor::eDF_None)
, m_upperLimit(0)
, m_lowerLimit(100)
, m_zoomPos(0.0f)
, m_zoomTarget(0.0f)
, m_sliderSpeed(10.0f)
, m_currentZoom(2.0f)
, m_scanTimeLeft(0.0f)
, m_scanTime(2.0f)
, m_flashProgress(0)
, m_currentCenterEntity(0)
, m_currentScanEntity(0)
, m_speed(0)
, m_suitMode("")
, m_suitModeIdent(eNanoSuitMode_Invalid)
, m_visorEnabled(false)
, m_objectRoot(NULL)
, m_objectScanning(NULL)
, m_objectArrowLeft(NULL)
, m_objectArrowTop(NULL)
, m_objectArrowRight(NULL)
, m_objectInfinity(NULL)
, m_object2DVisor(NULL)
, m_objectStealthBar(NULL)
, m_showScanning(true)
{
}



CHUD_Visor::~CHUD_Visor()
{
	m_scanningSound.Stop(); // TODO: should not be needed. 
}



void CHUD_Visor::Init()
{
	IHUDAsset* pAsset = GetAsset();
	HUD_FLASVAROBJ_REG( pAsset, "Root_Binoculars", m_objectRoot);
	HUD_FLASVAROBJ_REG( pAsset, "Scanning", m_objectScanning);
	HUD_FLASVAROBJ_REG( pAsset, "ArrowLeft", m_objectArrowLeft);
	HUD_FLASVAROBJ_REG( pAsset, "ArrowTop", m_objectArrowTop);
	HUD_FLASVAROBJ_REG( pAsset, "ArrowRight", m_objectArrowRight);
	HUD_FLASVAROBJ_REG( pAsset, "ArrowRight", m_objectArrowRight);
	HUD_FLASVAROBJ_REG( pAsset, "Root_Radar.StealthBar", m_objectStealthBar);
	

	pAsset->GetVariable("m_upperLimit", m_upperLimit);
	pAsset->GetVariable("m_lowerLimit", m_lowerLimit);
	pAsset->GetVariable("m_sliderSpeed", m_sliderSpeed);
	m_scanningSound.SetSignal("HUD_TacticalScanning");
	m_scanDoneSound.SetSignal("HUD_TacticalScanningComplete");
	m_newObjectiveText = g_pGame->GetHUD()->LocalizeStringW("@ui_visor_newobjective");
	ShowScanning(false);

	pAsset = GetAsset2D();
	HUD_FLASVAROBJ_REG( pAsset, "Visor", m_object2DVisor);
	HUD_FLASVAROBJ_REG( pAsset, "Visor.InfinityIcon", m_objectInfinity);
}



void CHUD_Visor::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_objectRoot);
	HUD_FLASHOBJ_SAFERELEASE(m_objectScanning);
	HUD_FLASHOBJ_SAFERELEASE(m_objectArrowLeft);
	HUD_FLASHOBJ_SAFERELEASE(m_objectArrowTop);
	HUD_FLASHOBJ_SAFERELEASE(m_objectArrowRight);
	HUD_FLASHOBJ_SAFERELEASE(m_object2DVisor);
	HUD_FLASHOBJ_SAFERELEASE(m_objectInfinity);
	HUD_FLASHOBJ_SAFERELEASE(m_objectStealthBar);
}



void CHUD_Visor::OnHUDEvent(const SHUDEvent& event)
{
	if(event.eventType == eHUDEvent_OnZoom)
	{
		OnZoom(event.eventIntData, event.eventIntData2);

		m_currentZoom = event.eventFloatData;
		m_dirty |= CHUD_Visor::eDF_SliderText;
	}
	else if(event.eventType == eHUDEvent_OnScanningStart)
	{
		StartScanning(event.eventFloatData);
	}
	else if(event.eventType == eHUDEvent_OnScanningStop)
	{
		StopScanning();
	}
	else if(event.eventType == eHUDEvent_OnCenterEntityChanged)
	{
		CenterEntityChanged((EntityId)event.eventIntData);
	}
	else if(event.eventType == eHUDEvent_OnSuitModeChanged)
	{
		const char* data = (const char*) event.eventPtrData;
		if(data)
			SetSuitMode(event.eventIntData, data);
	}
	else if(event.eventType == eHUDEvent_OnVisorChanged)
	{
		SetVisorEnabled(event.eventIntData == 1);
	}
	else if(event.eventType == eHUDEvent_OnAIAwarenessChanged)
	{
		SetAIAwareness(event.eventIntData);
	}
	else if(event.eventType == eHUDEvent_OnLeanChanged)
	{
		LeaningChanged(static_cast<CCoverAndLean::ELeanMode>(event.eventIntData), event.eventIntData2==1, event.eventIntData3==1);
	}
	else if(event.eventType == eHUDEvent_OnObjectiveChanged)
	{
		CHUDMissionObjective* pObjective = static_cast<CHUDMissionObjective*>(event.GetData(0).GetPtr());
		if(pObjective)
			ObjectiveChanged(pObjective);
	}
	else if(event.eventType == eHUDEvent_OnViewDistanceChanged)
	{
		OnRangeFinderChanged(event.GetData(0).GetInt());
	}
}



void CHUD_Visor::Update(float frameTime)
{
	IHUDAsset* pVisor = GetAsset();

	UpdateScanningProgress(frameTime);

	if(m_dirty & CHUD_Visor::eDF_SuitMode)
	{
		if(m_visorEnabled) 
		{
			m_objectRoot->GotoAndStop("Visor");
			m_object2DVisor->SetVisible(true);
		}
		else
		{
			m_objectRoot->GotoAndStop(m_suitMode.c_str());
			m_object2DVisor->SetVisible(false);
		}
	}

	if(m_dirty & CHUD_Visor::eDF_AIAwareness)
	{
		m_objectStealthBar->GotoAndStop(m_aiAwareness);
	}


	if(m_dirty & CHUD_Visor::eDF_Range)
	{
		if(m_visorEnabled)
		{
			bool infinity = (m_range == -1);
			m_objectInfinity->SetVisible(infinity);

			if(infinity)
			{
				m_object2DVisor->SetMember("Range", SFlashVarValue(""));
			}
			else
			{
				m_object2DVisor->SetMember("Range", SFlashVarValue(string().Format("%dm", m_range).c_str()));
			}
		}
	}

	if(abs(m_zoomTarget - m_zoomPos) > 0.001f)
	{
		m_zoomPos += (m_zoomTarget - m_zoomPos) * (frameTime * m_sliderSpeed);
		m_dirty |= CHUD_Visor::eDF_SliderPos;
	}
	else if(m_zoomTarget != m_zoomPos)
	{
		m_zoomPos = m_zoomTarget;
		m_dirty |= CHUD_Visor::eDF_SliderPos;
	}

	if(m_dirty & CHUD_Visor::eDF_SliderPos)
	{
		float pos = m_lowerLimit - m_zoomPos * (float)(m_lowerLimit - m_upperLimit);
		pVisor->SetVariable("Root_Binoculars.ZoomBar.Zoom_mc._y", pos);
	}

	if(m_dirty & CHUD_Visor::eDF_SliderText)
	{
		pVisor->SetVariable("curZoomMode", string().Format("%2.0fx", m_currentZoom).c_str());
	}

	if(m_dirty & CHUD_Visor::eDF_ScanningProgress)
	{
		m_objectScanning->GotoAndStop(m_flashProgress);
	}

	if(m_suitModeIdent == eNanoSuitMode_Power)
	{
		UpdateSpeed();

		if(m_dirty & CHUD_Visor::eDF_Speed)
		{
			GetAsset()->Invoke("Root_Binoculars.SpeedBar.gotoAndStop", m_speed);
		}
	}

	m_dirty = CHUD_Visor::eDF_None;
}



void CHUD_Visor::OnZoom(int targetStep, int numSteps)
{
	if(numSteps<=1 || targetStep==0)
	{
		m_zoomTarget = 0.0f;
		m_zoomPos = 0.0f;
		return;
	}

	m_zoomTarget = (float)(targetStep-1) / (float)(numSteps-1);
}



void CHUD_Visor::StartScanning(const float time)
{
	if(!m_currentCenterEntity)
		return;

	m_currentScanEntity = m_currentCenterEntity;
	m_scanningSound.Play();

	m_scanTime = time;
	if(m_scanTime<=0.0f)
	{
		m_scanTimeLeft = 0.0f;
	}
	else
	{
		m_scanTimeLeft = time;
	}
	m_dirty |= CHUD_Visor::eDF_ScanningProgress;
	ShowScanning(true);
	m_objectScanning->GotoAndStop(1);
}



void CHUD_Visor::StopScanning()
{
	m_scanningSound.Stop();
	ShowScanning(false);
	m_scanTimeLeft = 0.0f;
	m_currentScanEntity = 0;
}



void CHUD_Visor::ShowScanning(const bool show)
{
	if(m_showScanning != show)
	{
		SHUDEvent event(eHUDEvent_HUDElementVisibility);
		event.AddData(SHUDEventData(show));
		event.AddData(SHUDEventData((int)eHUDElement_scanning));
		CHUD::CallEvent(event);
		m_objectScanning->SetVisible(show);
		m_showScanning = show;
	}
}



void CHUD_Visor::SucceedScanning()
{
	m_scanningSound.Stop();
	m_scanDoneSound.Play();
	g_pGame->GetTacticalManager()->SetEntityScanned(m_currentScanEntity);
	SHUDEvent scannedEvent(eHUDEvent_OnEntityScanned);
	scannedEvent.AddData(SHUDEventData(static_cast<int>(m_currentScanEntity)));
	CHUD::CallEvent(scannedEvent);

	if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_currentScanEntity))
	{
		if(IScriptTable *pScriptTable = pEntity->GetScriptTable())
		{
			Script::CallMethod(pScriptTable, "HasBeenScanned");
		}
	}
	ShowScanning(false);

	m_currentScanEntity = 0;
}



void CHUD_Visor::UpdateScanningProgress(const float frameTime)
{
	if(m_scanTimeLeft<=0.0f)
		return;

	m_scanTimeLeft = max(m_scanTimeLeft-frameTime, 0.0f);
	if(m_scanTimeLeft==0.0f)
	{
		SucceedScanning();
	}

	float ratio = (float)__fsel(-m_scanTime, 1.0f, (m_scanTimeLeft / m_scanTime));

	int flashProgress = int_round( 100.0f * ( 1.0f -  ratio) );
	if(m_flashProgress != flashProgress)
	{
		m_flashProgress = flashProgress;
		m_dirty |= CHUD_Visor::eDF_ScanningProgress;
	}
}



void CHUD_Visor::OnRangeFinderChanged(const int range)
{
	if(m_range != range)
	{
		m_range = range;
		m_dirty |= CHUD_Visor::eDF_Range;
	}
}



void CHUD_Visor::SetSuitMode(int suitModeIdent, const char* suitMode)
{
	if (m_suitModeIdent != suitModeIdent)
	{
		m_suitMode = suitMode;
		m_suitModeIdent = static_cast<ENanoSuitMode>(suitModeIdent);
		m_dirty |= CHUD_Visor::eDF_SuitMode;
	}
}



void CHUD_Visor::SetVisorEnabled(bool enabled)
{
	if(m_visorEnabled != enabled)
	{
		m_visorEnabled = enabled;
		m_dirty |= CHUD_Visor::eDF_SuitMode;
		if(m_visorEnabled)
		{
			m_dirty |= CHUD_Visor::eDF_Range;
		}
	}
}



void CHUD_Visor::CenterEntityChanged(const EntityId entity)
{
	if(m_currentCenterEntity != entity)
	{
		m_currentCenterEntity = entity;
	}
}



void CHUD_Visor::SetAIAwareness(int level)
{
	if(m_aiAwareness != level)
	{
		m_aiAwareness = level;
		m_dirty |= CHUD_Visor::eDF_AIAwareness;
	}
}



void CHUD_Visor::LeaningChanged(const CCoverAndLean::ELeanMode mode, const bool canLean, const bool leaning)
{
	bool left = false;
	bool right = false;
	bool top = false;

	CryFixedStringT<32> button;
	CryFixedStringT<32> message;
	bool enabled = false;
	if(canLean && !leaning)
	{
#if defined(PS3)
		button = "PS3_L2";
#else
		button = "XBox360_LT";
#endif

		enabled = true;

		switch (mode)
		{

		case CCoverAndLean::ELeanMode_LEAN_AROUND_LEFT:
			{
				message = "@ui_interaction_leanleft";
				left = true;
			}
			break;

		case CCoverAndLean::ELeanMode_LEAN_AROUND_RIGHT:
			{
				message = "@ui_interaction_leanright";
				right = true;
			}
			break;

		case CCoverAndLean::ELeanMode_LEAN_OVER:
			{
				message = "@ui_interaction_leanover";
				top = true;
			}
			break;

		default:
			enabled = false;
			break;
		}
	}
	else if(leaning)
	{
#if defined(PS3)
		button = "PS3_R";
#else
		button = "XBox360_R";
#endif
		message = "@ui_interaction_lean";
		enabled = true;
	}

	SHUDEvent leanTextEvent;
	leanTextEvent.eventType = eHUDEvent_OnInteractionRequest;
	leanTextEvent.ReserveData(4);
	leanTextEvent.AddData(SHUDEventData(enabled));
	leanTextEvent.AddData(SHUDEventData((void*)message.c_str()));
	leanTextEvent.AddData(SHUDEventData((void*)button.c_str()));
	leanTextEvent.AddData(SHUDEventData(-1.0f));
	CHUD::CallEvent(leanTextEvent);

	m_objectArrowLeft->SetVisible(left);
	m_objectArrowTop->SetVisible(top);
	m_objectArrowRight->SetVisible(right);
}



void CHUD_Visor::ObjectiveChanged(CHUDMissionObjective* pObjective)
{
	if(!pObjective->IsSecondary() && pObjective->IsActivated())
		GetAsset()->Invoke("newObjectiveAlarm", m_newObjectiveText.c_str());
}

void CHUD_Visor::UpdateSpeed()
{
	CPlayer* pPlayer = static_cast<CPlayer*>(gEnv->pGame->GetIGameFramework()->GetClientActor());

	if(!pPlayer)
		return;

	const float maxSpeed = 8.9f;
	const float frameScale = 21.0f;
	const int frameOffset = 1;

	//stuartm 20/01/10 I'm leaving this as flat speed for now, but we can easily change
	//it to accurately read off the true velocity using the code below
/*
	float playerSpeed = pPlayer->GetActorStats()->velocity.GetLengthSquared();

	if (playerSpeed > VEC_EPSILON)
	{
		playerSpeed = min(pPlayer->GetActorStats()->velocity.GetLength(), maxSpeed);
	}
*/

	float playerSpeed = min(pPlayer->GetActorStats()->speedFlat, maxSpeed);

	const int speedRounded = (int)((playerSpeed / maxSpeed) * frameScale) + frameOffset;

	if(m_speed != speedRounded)
	{
		m_speed = speedRounded;
		m_dirty |= CHUD_Visor::eDF_Speed;
	}
}


//////////////////////////////////////////////////////////////////////////
