////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   DisplaySettings.cpp
//  Version:     v1.00
//  Created:     3/12/2001 by Timur.
//  Compilers:   Visual C++ 6.0
//  Description: Display Settings implementation.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h" 
#include "DisplaySettings.h"
#include "Objects\ClassDesc.h"
#include "Settings.h"

#include <IConsole.h>

//////////////////////////////////////////////////////////////////////////
CDisplaySettings::CDisplaySettings()
{
	m_flags = SETTINGS_NOLABELS | SETTINGS_NOCOLLISION;
	m_objectHideMask = 0;

	// All flags besides GI enabled by default
	m_renderFlags = RENDER_FLAG_LAST_ONE-1;
	m_renderFlags &= ~(RENDER_FLAG_BBOX);
	m_renderFlags &= ~(RENDER_FLAG_GI);
	m_debugFlags = 0;
	m_labelsDistance = 100;
}

//////////////////////////////////////////////////////////////////////////
CDisplaySettings::~CDisplaySettings()
{
}

void CDisplaySettings::SaveRegistry()
{
	SaveValue("Settings","ObjectHideMask",m_objectHideMask);
	SaveValue("Settings","RenderFlags",m_renderFlags );
	SaveValue("Settings","DisplayFlags",m_flags );
	SaveValue("Settings","DebugFlags",m_debugFlags );
	SaveValue("Settings","LabelsDistance",m_labelsDistance );
}

void CDisplaySettings::LoadRegistry()
{
	LoadValue( "Settings","ObjectHideMask",m_objectHideMask );
	LoadValue( "Settings","RenderFlags",m_renderFlags );
	LoadValue( "Settings","DisplayFlags",m_flags );
	LoadValue("Settings","DebugFlags",m_debugFlags );
	int temp = m_labelsDistance;
	LoadValue("Settings","LabelsDistance",temp );
	m_labelsDistance = temp;

	gSettings.objectHideMask = m_objectHideMask;
}

void CDisplaySettings::SetObjectHideMask( int hideMask )
{
	int prevMask = m_objectHideMask;
	m_objectHideMask = hideMask;

	gSettings.objectHideMask = m_objectHideMask;
};

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::PostInitApply()
{
	SetRenderFlags( m_renderFlags );
	SetDebugFlags( m_debugFlags );
}
//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SetRenderFlags( int flags )
{
	int prev = m_renderFlags;
	m_renderFlags = flags;

	if (!GetIEditor()->Get3DEngine())
		return;

	//////////////////////////////////////////////////////////////////////////
	//SetCVar( "e_Roads",m_renderFlags&RENDER_FLAG_ROADS );
	//SetCVar( "e_Decals",m_renderFlags&RENDER_FLAG_DECALS );
	//SetCVar( "e_TerrainDetailMaterials",m_renderFlags&RENDER_FLAG_DETAILTEX );
	//SetCVar( "r_DetailTextures",m_renderFlags&RENDER_FLAG_DETAILTEX );
	//SetCVar( "e_Fog",m_renderFlags&RENDER_FLAG_FOG );
	//SetCVar( "e_Vegetation",m_renderFlags&RENDER_FLAG_STATICOBJ );
	//SetCVar( "e_Shadows",m_renderFlags&RENDER_FLAG_SHADOWMAPS );
	//SetCVar( "e_SkyBox",m_renderFlags&RENDER_FLAG_SKYBOX );
	//SetCVar( "e_Terrain",m_renderFlags&RENDER_FLAG_TERRAIN );
	//SetCVar( "e_WaterOcean",m_renderFlags&RENDER_FLAG_WATER );
 // SetCVar( "e_WaterVolumes",m_renderFlags&RENDER_FLAG_WATER );
	//SetCVar( "e_ProcVegetation",m_renderFlags&RENDER_FLAG_DETAILOBJ );
	//SetCVar( "e_Particles",m_renderFlags&RENDER_FLAG_PARTICLES );
	//SetCVar( "e_Voxel",m_renderFlags&RENDER_FLAG_VOXELS );
	//SetCVar( "e_Clouds",m_renderFlags&RENDER_FLAG_CLOUDS );
	//SetCVar( "e_VegetationSprites",m_renderFlags&RENDER_FLAG_IMPOSTERS );
	//SetCVarInt( "r_Beams", (m_renderFlags&RENDER_FLAG_BEAMS)? 3: 0 );
	//SetCVar( "e_GI", m_renderFlags&RENDER_FLAG_GI );
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SetDebugFlags( int flags )
{
	m_debugFlags = flags;
	
	//
	//SetCVar( "ai_DebugDraw",m_debugFlags&DBG_AI_DEBUGDRAW );
	//SetCVarInt( "r_TexLog",(m_debugFlags&DBG_TEXTURE_MEMINFO) ? 2:0 );
	//SetCVarInt( "MemStats",(m_debugFlags&DBG_MEMSTATS) ? 1000:0 );
	//SetCVarInt( "p_draw_helpers",(m_debugFlags&DBG_PHYSICS_DEBUGDRAW) ? 5634:0 );

	//SetCVar( "r_ProfileShaders",(m_debugFlags&DBG_RENDERER_PROFILESHADERS) );

	//SetCVarInt( "r_MeasureOverdraw",(m_debugFlags&DBG_RENDERER_OVERDRAW) ? 1:0 );
	//SetCVarInt( "r_Stats",(m_debugFlags&DBG_RENDERER_RESOURCES) ? 4:0 );
	//SetCVarInt( "sys_enable_budgetmonitoring",(m_debugFlags&DBG_BUDGET_MONITORING) ? 4:0 );

	//SetCVarInt( "Profile",(m_debugFlags&DBG_FRAMEPROFILE) ? 1:0 );

	ISystem *pSystem = GetIEditor()->GetSystem();
	if (pSystem)
	{
		bool bOn = (m_debugFlags&DBG_FRAMEPROFILE) != 0;
		pSystem->GetIProfileSystem()->Enable( bOn,bOn );
	}
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SetCVar( const char *cvar,bool value )
{
	ICVar *var = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(cvar);
	if (var) {
		var->Set( (value)?1:0 );
	}
	else
	{
		CLogFile::FormatLine( "Console Variable %s not declared",cvar );
	}
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SetCVarInt( const char *cvar,int value )
{
	ICVar *var = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(cvar);
	if (var) {
		var->Set( value );
	}
	else
	{
		CLogFile::FormatLine( "Console Variable %s not declared",cvar );
	}
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::DisplayLabels( bool bEnable )
{
	if (bEnable)
		m_flags &= ~SETTINGS_NOLABELS;
	else
		m_flags |= SETTINGS_NOLABELS;
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::DisplayTracks( bool bEnable )
{
	if (bEnable)
		m_flags &= ~SETTINGS_HIDE_TRACKS;
	else
		m_flags |= SETTINGS_HIDE_TRACKS;
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::DisplayLinks( bool bEnable )
{
	if (bEnable)
		m_flags &= ~SETTINGS_HIDE_LINKS;
	else
		m_flags |= SETTINGS_HIDE_LINKS;
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::DisplayHelpers( bool bEnable )
{
	if (bEnable)
		m_flags &= ~SETTINGS_HIDE_HELPERS;
	else
		m_flags |= SETTINGS_HIDE_HELPERS;
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SaveValue( const char *sSection,const char *sKey,int value )
{
	AfxGetApp()->WriteProfileInt( sSection,sKey,value );
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::LoadValue( const char *sSection,const char *sKey,int &value )
{
	value = AfxGetApp()->GetProfileInt(sSection,sKey,value );
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::LoadValue( const char *sSection,const char *sKey,bool &value )
{
	value = AfxGetApp()->GetProfileInt(sSection,sKey,value );
}
