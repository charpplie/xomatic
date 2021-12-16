#include "StdAfx.h"

#include "UIScorePopup.h"

#include "HUD/UI/UIButtonPromptRegion.h"
#include "HUD/HUD.h"
#include "HUD/HUDDefines.h"

#include "GameRulesTypes.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

//-----------------------------------------

UIScorePopup::UIScorePopup()
 : TScorePopupParent()
{
	CHUD::AddHUDEventListener(this,"OnNewScore");
	CHUD::AddHUDEventListener(this,"OnNewXP");

	REGISTER_COMMAND("HUD_testScore", ScoreCmd, 0, "HUD test");
}

UIScorePopup::~UIScorePopup()
{
	gEnv->pConsole->RemoveCommand("HUD_testScore");

	CHUD::RemoveHUDEventListener(this);
}

void UIScorePopup::NewScoreElement( const SGameRulesScoreInfo& score_info )
{	
	CryLogAlways ("UIScorePopup::NewScoreElement rewarding %d points to local player", score_info.score);

	if (score_info.score)
	{
		char number[8];
		string message;
		CControllerInputRenderInfo iconInfo;

		sprintf (number, (score_info.score > 0) ? "+%d" : "%d", score_info.score);

		CCCPOINT_IF(score_info.score > 0, HUD_DisplayScoreIncrease);
		CCCPOINT_IF(score_info.score < 0, HUD_DisplayScoreDecrease);

		switch( score_info.type )
		{
		case EGRST_PlayerKill:                  message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_killed", HUDGetVictimName(score_info.data.PlayerKill.victim).c_str(), NULL);			break;
		case EGRST_PlayerKillAssist:            message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_assist", HUDGetVictimName(score_info.data.PlayerKill.victim).c_str(), NULL);			break;
		case EGRST_PlayerTeamKill:              message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_teamkill", HUDGetVictimName(score_info.data.PlayerKill.victim).c_str(), NULL);			break;
		case EGRST_Suicide:                     message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_suicide");			break;
		case EGRST_CTF_FlagPickedUp:            message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_ctf_pickup");			break;
		case EGRST_CTF_FlagCompleted:           message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_ctf_completed");			break;
		case EGRST_CTF_FlagRetrieved:           message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_ctf_retrieved");			break;
		case EGRST_SingleHanded:                message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_singlehanded");			break;
		case EGRST_FlagAssist:                  message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_flagassist");			break;
		case EGRST_SelfDestructPrevention:      message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_selfdestructprevent", HUDGetVictimName(score_info.data.PlayerKill.victim).c_str(), NULL);			break;
		case EGRST_CaptureObjectiveHeld:        message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_objective_held");			break;
		case EGRST_CaptureObjectiveTaken:       message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_objective_taken");			break;
		case EGRST_CaptureObjectiveCompleted:   message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_objective_complete");			break;
		case EGRST_AON_Win:                     message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_aon_win");			break;
		case EGRST_AON_Draw:                    message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_aon_draw");			break;
		case EGRST_CarryObjectiveTaken:         message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_objective_taken");			break;
		case EGRST_CarryObjectiveRetrieved:     message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_objective_retrieved");			break;
		case EGRST_CarryObjectiveCarrierKilled: message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_carrier_killed");			break;
		case EGRST_CarryObjectiveDefended:      message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_objective_defended");			break;
		case EGRST_CarryObjectiveCompleted:     message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_objective_complete");			break;
		case EGRST_CarryObjectiveHeld:          message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_objective_held");			break;
		case EGRST_BombTheBaseCompleted:        message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_btb_complete");			break;
		case EGRST_KingOfTheHillObjectiveHeld:  message = g_pGame->GetHUD()->LocalizeString("@ui_scorepopup_objective_held");			break;
		}

		CRY_ASSERT(message.empty() == false);

		iconInfo.SetText(number);

		CryLogAlways ("UIScorePopup::NewScoreElement displaying '%s' '%s'", number, message.c_str());
		CUIButtonPromptRegion::SetOnScreenMessageText("scoreMessages", message.c_str(), &iconInfo, 3.5f);
	}
}

void UIScorePopup::NewXPElement(const EPPType type, const int xp)
{
	CryLogAlways ("UIScorePopup::NewXPElement rewarding %d xp to local player", xp);

	char number[8];
	string message;
	CControllerInputRenderInfo iconInfo;

	sprintf (number,"$3+%d", xp);

	switch( type )
	{
	case EPP_TeamRadar:           message.Format("@ui_xppoints_teamradar");						break;
	case EPP_EMPStrike:           message.Format("@ui_xppoints_empstrike");						break;
	case EPP_SatStrike:           message.Format("@ui_xppoints_satstrike");						break;
	case EPP_SuitBoost:           message.Format("@ui_xppoints_suitboost");						break;
	case EPP_DogtagCollection:    message.Format("@ui_xppoints_dogtags");							break;
	case EPP_Headshot:            message.Format("@ui_xppoints_headshot");						break;
	case EPP_AirDeath:            message.Format("@ui_xppoints_airdeath");						break;
	case EPP_MeleeTakedown:       message.Format("@ui_xppoints_meleetakedown");				break;
	case EPP_PistolKill:          message.Format("@ui_xppoints_pistolkill");					break;
	case EPP_BlindKill:           message.Format("@ui_xppoints_blindkill");						break;
	case EPP_Impale:              message.Format("@ui_xppoints_impale");							break;
	case EPP_FirstBlood:          message.Format("@ui_xppoints_firstblood");					break;
	case EPP_Rumbled:             message.Format("@ui_xppoints_rumbled");							break;
	case EPP_UnderwaterKill:      message.Format("@ui_xppoints_underwaterkill");			break;
	case EPP_RoadKill:            message.Format("@ui_xppoints_roadkill");						break;
	case EPP_NearDeathExperience: message.Format("@ui_xppoints_neardeathexperience");	break;
	case EPP_KillJoy:			  message.Format("@ui_xppoints_killjoy");							break;
	}

	CRY_ASSERT(message.empty() == false);

	iconInfo.SetText(number);

	CryLogAlways ("UIScorePopup::NewXPElement displaying '%s' '%s'", number, message.c_str());
	CUIButtonPromptRegion::SetOnScreenMessageText("scoreMessages", message.c_str(), &iconInfo, 3.5f);

}

void UIScorePopup::ScoreCmd(IConsoleCmdArgs* cmdArgs)
{
	EntityId localId = g_pGame->GetIGameFramework()->GetClientActorId();

	SHUDEvent newScoreEvent(eHUDEvent_OnNewScore);
	newScoreEvent.AddData((int)(EGameRulesScoreType)(cry_rand() % EGRST_Num));
	newScoreEvent.AddData((int)(cry_rand() % 9) + 1);
	newScoreEvent.AddData((int)localId);
	CHUD::CallEvent(newScoreEvent);
}

void UIScorePopup::OnHUDEvent( const SHUDEvent& event )
{
	if( event.eventType == eHUDEvent_OnNewScore )
	{
		const SGameRulesScoreInfo& scoreInfo = *(static_cast<SGameRulesScoreInfo*>(event.GetData(0).GetPtr()));
		NewScoreElement( scoreInfo );
		return;
	}
	else //if( event.eventType == eHUDEvent_OnNewXP )
	{
		const EPPType type = static_cast<EPPType>(event.GetData(0).GetInt());
		const int xp = event.GetData(1).GetInt();
		
		NewXPElement(type, xp);
	}

}