#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerGlobalSettings.h
//  Created:     Jan/15/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

struct SConsoleVarsForExcluisveMode
{
	SConsoleVarsForExcluisveMode()
	{
		memset( this, 0, sizeof(*this) );
	}
	int r_DisplayInfo;
	int r_HDRRendering;
	int r_PostProcessEffects;
	int e_Vegetation;
	int e_WaterOcean;
	int e_WaterVolumes;
	int e_Terrain;
	int e_Shadows;
	int e_Particles;
	int e_Clouds;
	int r_Beams;
	int e_SkyBox;
};

struct SDesignerEnvironmentInfo
{
	SDesignerEnvironmentInfo()
	{
		m_bEnableExclusiveMode = false;
		m_OldTimeOfDay = NULL;
		m_OldTimeOfTOD = 0;
		m_OldCameraTM = Matrix34::CreateIdentity();
		m_OldObjectHideMask = 0;
	}

	void Save();
	void Load();

	void EnableExclusiveMode( bool bEnable );
	bool IsEnableExclusiveMode() const { return m_bEnableExclusiveMode; }

	void SetTimeOfDayForExclusiveMode();
	void SetCVForExclusiveMode();
	void SetObjectsFlagForExclusiveMode();

	void RestoreTimeOfDay();
	void RestoreCV();
	void RestoreObjectsFlag();

	void CenterCameraForExclusiveMode();

	void SetTime( ITimeOfDay* pTOD, float fTime );

	bool m_bEnableExclusiveMode;

	SConsoleVarsForExcluisveMode m_OldConsoleVars;
	std::map<CBaseObject*,bool> m_ObjectHiddenFlagMap;
	XmlNodeRef m_OldTimeOfDay;
	float m_OldTimeOfTOD;
	Matrix34 m_OldCameraTM;
	int m_OldObjectHideMask;
};