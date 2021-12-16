////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UISettingsPanels.cpp
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "UISettingsPanels.h"
#include "UIManager.h"
#include "UIEditor.h"
#include "UIViewport.h"


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
CUISettingsPropertiesPanel::CUISettingsPropertiesPanel()
{
	m_pVarBlock = new CVarBlock;
}

void CUISettingsPropertiesPanel::UpdateChanges()
{
	for (std::list<ICVarSmartVar*>::iterator it = m_Vars.begin(); it != m_Vars.end(); ++it)
		(*it)->ReadIfChanged();
}

void CUISettingsPropertiesPanel::UpdateVars()
{
	for (std::list<ICVarSmartVar*>::iterator it = m_Vars.begin(); it != m_Vars.end(); ++it)
		(*it)->Read();
}

void CUISettingsPropertiesPanel::RegisterVar( ICVarSmartVar* pVar, const char* description )
{
	GetVarBlock()->AddVariable( pVar->GetVar(), description );
	m_Vars.push_back(pVar);
}

void CUISettingsPropertiesPanel::OnVarChange( IVariable *pVar )
{
	for (std::list<ICVarSmartVar*>::iterator it = m_Vars.begin(); it != m_Vars.end(); ++it)
		(*it)->Write();
	UpdateVars();
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
CUISettingsPropertiesPanelDebug::CUISettingsPropertiesPanelDebug()
: mv_FlashInfo("sys_flash_info")
, mv_UIElementInfo("gfx_debugdraw", 0, 1, true)
, mv_UIActionInfo("gfx_debugdraw", 0, 2, true)
, mv_DebugDraw("r_DisplayInfo", 0, 2)
, mv_FlashAutoReload("gfx_FlashReloadEnabled", 0, 1)
, mv_WaitForFlash("gfx_FlashReloadTime")
, mv_LogUIAction("gfx_uiaction_log")
, mv_LogUIActionFilter("gfx_uiaction_log_filter")
{
	RegisterVar( &mv_FlashInfo, "Flash Debug Info" );
	RegisterVar( &mv_UIElementInfo, "UI Element Debug Info" );
	RegisterVar( &mv_UIActionInfo, "UI Action Debug Info" );
	RegisterVar( &mv_DebugDraw, "Display Info" );
	RegisterVar( &mv_FlashAutoReload, "Enables flash auto reload" );
	RegisterVar( &mv_WaitForFlash, "Flash auto reload delay" );
	RegisterVar( &mv_LogUIAction, "UIAction logging" );
	RegisterVar( &mv_LogUIActionFilter, "UIAction logging filter" );

	AddVariables();
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CUISettingsPropertiesPanelFlashInfo::CUISettingsPropertiesPanelFlashInfo()
: sys_flash_info_peak_tolerance("sys_flash_info_peak_tolerance")
, sys_flash_info_peak_exclude("sys_flash_info_peak_exclude")
, sys_flash_info_histo_scale("sys_flash_info_histo_scale")
, sys_flash_curve_tess_error("sys_flash_curve_tess_error")
, sys_flash_check_filemodtime("sys_flash_check_filemodtime")
{
	RegisterVar( &sys_flash_info_peak_tolerance, "Peak tolerance" );
	RegisterVar( &sys_flash_info_peak_exclude, "Peak exclude list" );
	RegisterVar( &sys_flash_info_histo_scale, "Historgram scale" );
	RegisterVar( &sys_flash_curve_tess_error, "Curve tessellation" );
	RegisterVar( &sys_flash_check_filemodtime, "Check Filemodtime" );

	AddVariables();
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CUISettingsPropertiesPanelFlashRenderInfo::CUISettingsPropertiesPanelFlashRenderInfo()
: sys_flash_debugdraw("sys_flash_debugdraw")
, sys_flash_newstencilclear("sys_flash_newstencilclear")
, sys_flash_edgeaa("sys_flash_edgeaa")
, sys_flash_stereo_maxparallax("sys_flash_stereo_maxparallax")
{
	RegisterVar( &sys_flash_debugdraw, "Debug Draw" );
	RegisterVar( &sys_flash_newstencilclear, "New Stencil clear" );
	RegisterVar( &sys_flash_edgeaa, "Edge AA" );
	RegisterVar( &sys_flash_stereo_maxparallax, "Stereo max parallax" );

	AddVariables();
}