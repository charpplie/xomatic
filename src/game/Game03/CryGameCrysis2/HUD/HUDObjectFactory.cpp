#include "StdAfx.h"
#include "HUDObjectFactory.h"

#include "HUD_Radar.h"
#include "HUD_Interactive.h"
#include "HUD_EnergyAndSuitMode.h"
#include "HUD_Logic.h"
#include "HUD_LookAtUpdater.h"
#include "HUD_Ammo.h"
#include "HUD_Crosshair.h"
#include "HUD_InterestPoints.h"
#include "HUD_ReviveCycle.h"
#include "HUD_MPSelectMenu.h"
#include "HUD_Scoreboard.h"
#include "HUD_CTFScoreElement.h"
#include "HUD_AONScoreElement.h"
#include "HUD_IAScoreElement.h"
#include "HUD_TIAScoreElement.h"
#include "HUD_ASScoreElement.h"
#include "HUD_PSLScoreElement.h"
#include "HUD_CDScoreElement.h"
#include "HUD_SuitMenu.h"
#include "HUD_Visor.h"
#include "HUD_Debug.h"
#include "HUD_AIUpdater.h"
#include "HUD_Message.h"
#include "HUD_Vehicles.h"
#include "HUD_ScannedEnemiesUpdater.h"
#include "HUD_TacticalInfo.h"
#include "HUD_ObjectiveDisplay.h"
#include "HUD_NavPath.h"
#include "HUD/Objects/HUD_LeavingBattle.h"
#include "HUD_WeaponSelect.h"
#include "HUD_UnifiedAsset.h"
#include "HUD_InteractWithObject.h"
#include "HUD_InformationField.h"
#include "HUD_Video.h"
#include "HUD_SaveLoadDisplay.h"
#include "HUD/Objects/HUD_Tagnames.h"
#include "HUD/Objects/HUD_ProgressNotify.h"
#include "HUD/Objects/HUD_ScoreNotify.h"
#include "HUD/Objects/HUD_DogTagCollectedPopup.h"
#include "HUD_Detector.h"
#include "HUD_Stance.h"
#include "HUD_MessageQueue.h"


//////////////////////////////////////////////////////////////////////////


template <typename T, typename R> R *CreateHUDObject() { return new T(); };


//////////////////////////////////////////////////////////////////////////


CHUDObjectFactory::CHUDObjectFactory()
{
	RegisterHUDObject("HUD_Radar", &CreateHUDObject<CHUD_Radar, CHUDObject>);
	RegisterHUDObject("HUD_Interactive", &CreateHUDObject<CHUD_Interactive, CHUDObject>);
	RegisterHUDObject("HUD_EnergyAndSuitMode", &CreateHUDObject<CHUD_EnergyAndSuitMode, CHUDObject>);
	RegisterHUDObject("HUD_Logic", &CreateHUDObject<CHUD_Logic, CHUDObject>);
	RegisterHUDObject("HUD_Ammo", &CreateHUDObject<CHUD_Ammo, CHUDObject>);
	RegisterHUDObject("HUD_Crosshair", &CreateHUDObject<CHUD_Crosshair, CHUDObject>);
	RegisterHUDObject("HUD_LookAtUpdater", &CreateHUDObject<CHUD_LookAtUpdater, CHUDObject>);
	RegisterHUDObject("HUD_tia", &CreateHUDObject<CHUD_TiaScoreElement, CHUDObject>);
	RegisterHUDObject("HUD_ctf", &CreateHUDObject<CHUD_CtfScoreElement, CHUDObject>);
	RegisterHUDObject("HUD_aon", &CreateHUDObject<CHUD_AONScoreElement, CHUDObject>);
	RegisterHUDObject("HUD_ia", &CreateHUDObject<CHUD_IAScoreElement, CHUDObject>);
	RegisterHUDObject("HUD_as", &CreateHUDObject<CHUD_AsScoreElement, CHUDObject>);
	RegisterHUDObject("HUD_PSLScoreElement", &CreateHUDObject<CHUD_PSLScoreElement, CHUDObject>);
	RegisterHUDObject("HUD_cd", &CreateHUDObject<CHUD_CDScoreElement, CHUDObject>);
	RegisterHUDObject("HUD_InterestPoints", &CreateHUDObject<CHUD_InterestPoints, CHUDObject>);
	RegisterHUDObject("HUD_SuitMenu", &CreateHUDObject<CHUD_SuitMenu, CHUDObject>);
	RegisterHUDObject("HUD_Visor", &CreateHUDObject<CHUD_Visor, CHUDObject>);
	RegisterHUDObject("HUD_Debug", &CreateHUDObject<CHUD_Debug, CHUDObject>);
	RegisterHUDObject("HUD_ReviveCycle", &CreateHUDObject<CHUD_ReviveCycle, CHUDObject>);
	RegisterHUDObject("HUD_Scoreboard", &CreateHUDObject<CHUD_Scoreboard, CHUDObject>);
	RegisterHUDObject("HUD_MPSelectMenu", &CreateHUDObject<CHUD_MPSelectMenu, CHUDObject>);
	RegisterHUDObject("HUD_AIUpdater", &CreateHUDObject<CHUD_AIUpdater, CHUDObject>);
	RegisterHUDObject("HUD_Message", &CreateHUDObject<CHUD_Message, CHUDObject>);
	RegisterHUDObject("HUD_Vehicles", &CreateHUDObject<CHUD_Vehicles, CHUDObject>);
	RegisterHUDObject("HUD_ScannedEnemiesUpdater", &CreateHUDObject<CHUD_ScannedEnemiesUpdater, CHUDObject>);
	RegisterHUDObject("HUD_TacticalInfo", &CreateHUDObject<CHUD_TacticalInfo, CHUDObject>);
	RegisterHUDObject("HUD_ObjectiveDisplay", &CreateHUDObject<CHUD_ObjectiveDisplay, CHUDObject>);
	RegisterHUDObject("HUD_NavPath", &CreateHUDObject<CHUD_NavPath, CHUDObject>);
	RegisterHUDObject("HUD_LeavingBattleNotification", &CreateHUDObject<CHUD_LeavingBattleNotification, CHUDObject>);
	RegisterHUDObject("HUD_WeaponSelect", &CreateHUDObject<CHUD_WeaponSelect, CHUDObject>);
	RegisterHUDObject("HUD_UnifiedAsset", &CreateHUDObject<CHUD_UnifiedAsset, CHUDObject>);
	RegisterHUDObject("HUD_InteractWithObject", &CreateHUDObject<CHUD_InteractWithObject, CHUDObject>);	
	RegisterHUDObject("HUD_InformationField", &CreateHUDObject<CHUD_InformationField, CHUDObject>);	
	RegisterHUDObject("HUD_Video", &CreateHUDObject<CHUD_Video, CHUDObject>);	
	RegisterHUDObject("HUD_SaveLoadDisplay", &CreateHUDObject<CHUD_SaveLoadDisplay, CHUDObject>);	
	RegisterHUDObject("HUD_Tagnames", &CreateHUDObject<CHUD_Tagnames, CHUDObject>);	
	RegisterHUDObject("HUD_XPNotify", &CreateHUDObject<CHUD_ProgressNotify, CHUDObject>);	
	RegisterHUDObject("HUD_ScoreNotify", &CreateHUDObject<CHUD_ScoreNotify, CHUDObject>);	
	RegisterHUDObject("HUD_DogTagCollectedPopup", &CreateHUDObject<CHUD_DogTagCollectedPopup, CHUDObject>);	
	RegisterHUDObject("HUD_Detector", &CreateHUDObject<CHUD_Detector, CHUDObject>);	
	RegisterHUDObject("HUD_Stance", &CreateHUDObject<CHUD_Stance, CHUDObject>);	
	RegisterHUDObject("HUD_MessageQueue", &CreateHUDObject<CHUD_MessageQueue, CHUDObject>);	
}



CHUDObjectFactory::~CHUDObjectFactory()
{
}



void CHUDObjectFactory::RegisterHUDObject(const char *name, CHUDObject *(*CreateProc)())
{
	m_objectRegistry.insert(THUDObjectRegistry::value_type(name, CreateProc));
}



CHUDObject* CHUDObjectFactory::GetClassInstance(const char* className)
{
	THUDObjectRegistry::iterator it = m_objectRegistry.find(CONST_TEMP_STRING(className));
	if (it != m_objectRegistry.end())
		return it->second();

	else
	{
		CRY_ASSERT_MESSAGE(0,string().Format("unknown class name! : '%s'", className ).c_str() );
		return NULL;
	}
}


//////////////////////////////////////////////////////////////////////////

