#include "StdAfx.h"
#include "PlayerPlugin_Perk_ModifyValues.h"
#include "Player.h"

void CPlayerPlugin_Perk_WeaponsTraining::InformActiveHasChanged()
{
	CPlayerModifiableValues& pModifiableValues = m_ownerPlayer->GetModifiableValues();
	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
	pModifiableValues.GetValue(kPMV_WeaponReloadSpeedScale).ChangeModifierStateIfRequired(& perkVars->perk_weaponsTrainingReloadSpeedScale, InRequiredSuitMode());

	pModifiableValues.GetValue(kPMV_WeaponSelectSpeedScale).ChangeModifierStateIfRequired(& perkVars->perk_weaponsTrainingSelectSpeedScale, InRequiredSuitMode() && IsTier(eTierTwo));
}

void CPlayerPlugin_Perk_WeaponsSpread::InformActiveHasChanged()
{
	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
	m_ownerPlayer->GetModifiableValues().GetValue(kPMV_WeaponSpread).ChangeModifierStateIfRequired(& perkVars->perk_weaponsSpreadSpreadScale, InRequiredSuitMode());
}

void CPlayerPlugin_Perk_SuperStrength::InformActiveHasChanged()
{
	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
	m_ownerPlayer->GetModifiableValues().GetValue(kPMV_WeaponRecoil).ChangeModifierStateIfRequired(& perkVars->perk_superStrength_scaleRecoil, InRequiredSuitMode());
}
