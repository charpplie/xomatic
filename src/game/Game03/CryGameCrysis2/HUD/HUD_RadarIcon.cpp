#include "StdAfx.h"
#include "HUD/HUD_RadarIcon.h"

#include "HUD/HUDAsset.h"
#include "HUD/HUD_Radar.h"
#include "HUD/HUDDefines.h"


//////////////////////////////////////////////////////////////////////////


CHUD_RadarIcon::SFlashData::SFlashIcon::SFlashAlertness::SFlashAlertness()
: m_up(NULL)
, m_down(NULL)
, m_even(NULL)
{

}



void CHUD_RadarIcon::SFlashData::SFlashIcon::SFlashAlertness::Init(IHUDAsset* pAsset, const int index)
{
	HUD_FLASVAROBJ_REG(pAsset, string().Format("Root_Radar.M%d.Icons.Alertness.Up", index).c_str(), m_up);
	HUD_FLASVAROBJ_REG(pAsset, string().Format("Root_Radar.M%d.Icons.Alertness.Down", index).c_str(), m_down);
	HUD_FLASVAROBJ_REG(pAsset, string().Format("Root_Radar.M%d.Icons.Alertness.Ground", index).c_str(), m_even);
}



void CHUD_RadarIcon::SFlashData::SFlashIcon::SFlashAlertness::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_up);
	HUD_FLASHOBJ_SAFERELEASE(m_down);
	HUD_FLASHOBJ_SAFERELEASE(m_even);
}


//////////////////////////////////////////////////////////////////////////


CHUD_RadarIcon::SFlashData::SFlashIcon::SFlashIcon()
: m_firing(NULL)
, m_alertnessObject(NULL)
{

}



void CHUD_RadarIcon::SFlashData::SFlashIcon::Init(IHUDAsset* pAsset, const int index)
{
	HUD_FLASVAROBJ_REG(pAsset, string().Format("Root_Radar.M%d.Icons.Firing", index).c_str(), m_firing);
	HUD_FLASVAROBJ_REG(pAsset, string().Format("Root_Radar.M%d.Icons.Alertness", index).c_str(), m_alertnessObject);

	if(!m_alertnessObject)
	{
		return;
	}

	for(int i=0; i<CHUD_RadarIcon::eAlertness_End; ++i)
	{
		m_alertnessObject->GotoAndStop(i+1);
		m_alertnessStates[i].Init(pAsset, index);
	}
}



void CHUD_RadarIcon::SFlashData::SFlashIcon::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_firing);
	HUD_FLASHOBJ_SAFERELEASE(m_alertnessObject);
	for(int i=0; i<CHUD_RadarIcon::eAlertness_End; ++i)
	{
		m_alertnessStates[i].PreDelete();
	}
}


//////////////////////////////////////////////////////////////////////////


CHUD_RadarIcon::SFlashData::SFlashData()
: m_object(NULL)
, m_icon(NULL)
{

}



void CHUD_RadarIcon::SFlashData::Init(IHUDAsset* pAsset, const int index)
{
	HUD_FLASVAROBJ_REG(pAsset, string().Format("Root_Radar.M%d", index).c_str(), m_object);
	HUD_FLASVAROBJ_REG(pAsset, string().Format("Root_Radar.M%d.Icons", index).c_str(), m_icon);

	if(!m_icon)
	{
		return;
	}

	m_iconStates[CHUD_RadarIcon::eIcon_EnemyArrow].Init(pAsset, index);
	m_iconStates[CHUD_RadarIcon::eIcon_EnemyNoArrow].Init(pAsset, index);
	m_iconStates[CHUD_RadarIcon::eIcon_FriendlyArrow].Init(pAsset, index);

}



void CHUD_RadarIcon::SFlashData::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_object);
	HUD_FLASHOBJ_SAFERELEASE(m_icon);

	m_iconStates[CHUD_RadarIcon::eIcon_EnemyArrow].PreDelete();
	m_iconStates[CHUD_RadarIcon::eIcon_EnemyNoArrow].PreDelete();
	m_iconStates[CHUD_RadarIcon::eIcon_FriendlyArrow].PreDelete();
}


//////////////////////////////////////////////////////////////////////////


CHUD_RadarIcon::CHUD_RadarIcon()
: m_dirty(eDF_None)
, m_flags(eRF_None)
, m_entityId(0)
, m_noUpdatingPos(0,0,0)
, m_posX(0)
, m_posY(0)
, m_rotation(0)
, m_alertnessValue(-1)
, m_timeVisible(0)
, m_alpha(100.0f)
, m_icon(eIcon_None)
, m_tempIcon(eIcon_None)
, m_heightDisplay(eHD_Even)
, m_offset(0.0f, 0.0f)
{
	m_pFlashData = new SFlashData();
}



CHUD_RadarIcon::~CHUD_RadarIcon()
{
	SAFE_DELETE(m_pFlashData);
}



void CHUD_RadarIcon::PreDelete()
{
	m_pFlashData->PreDelete();
}



void CHUD_RadarIcon::Init(CHUD_Radar* pRadar, IHUDAsset* pAsset, const int index, const Vec2& innerOffset)
{
	m_pRadar = pRadar;
	m_offset = innerOffset;
	m_pFlashData->Init(pAsset, index);
}



void CHUD_RadarIcon::Clear()
{
	m_dirty = eDF_None;
	m_flags = eRF_None;
	m_entityId = 0;
	m_noUpdatingPos.Set(0.0f, 0.0f, 0.0f);
	m_posX = 0;
	m_posY = 0;
	m_rotation = 0;
	m_alertnessValue = -1;
	m_timeVisible = 0;
	m_alpha = 100.0f;
	m_icon = eIcon_None;
	m_tempIcon = eIcon_None;
	m_heightDisplay = eHD_Even;
}



void CHUD_RadarIcon::CopyFrom(const CHUD_RadarIcon& source)
{
	m_dirty = source.m_dirty;
	m_flags = source.m_flags;
	m_entityId = source.m_entityId;
	m_noUpdatingPos = source.m_noUpdatingPos;
	m_posX = source.m_posX;
	m_posY = source.m_posY;
	m_rotation = source.m_rotation;
	m_alertnessValue = source.m_alertnessValue;
	m_timeVisible = source.m_timeVisible;
	m_alpha = source.m_alpha;
	m_icon = source.m_icon;
	m_tempIcon = source.m_tempIcon;
	m_heightDisplay = source.m_heightDisplay;
}



void CHUD_RadarIcon::Update(float frameTime)
{
	if(m_timeVisible>0)
	{
		float timeVisible = m_timeVisible - frameTime;
		if(timeVisible<=0.0f)
		{
			m_pRadar->ScheduleRemoveEntity(m_entityId);
			return;
		}
		m_timeVisible = timeVisible;
	}

	if(m_dirty==eDF_None)
	{
		return;
	}

	SFlashDisplayInfo info;

	if(m_dirty & eDF_PosX)
	{
		info.SetX(m_posX + m_offset.x);
	}

	if(m_dirty & eDF_PosY)
	{
		info.SetY(m_posY + m_offset.y);
	}

	if(info.IsAnyFlagSet())
	{
		if(m_pFlashData->m_object)
		{
			m_pFlashData->m_object->SetDisplayInfo(info);
		}
	}


	info.Clear();

	if(m_dirty & eDF_Rot)
	{
		info.SetRotation(m_rotation);
	}

	if(m_dirty & eDF_Alpha)
	{
		info.SetAlpha(m_alpha);
	}

	if(info.IsAnyFlagSet())
	{
		if(m_pFlashData->m_icon)
		{
			m_pFlashData->m_icon->SetDisplayInfo(info);
		}
	}

	EIcon icon = m_tempIcon==eIcon_None ? m_icon : m_tempIcon;

	SFlashData::SFlashIcon& iconObj = m_pFlashData->m_iconStates[icon];

	if(m_dirty & eDF_Icon)
	{
		if(m_pFlashData->m_icon)
		{
			m_pFlashData->m_icon->GotoAndStop(icon);
		}

		m_dirty |= eDF_Height | eDF_Alertness;
	}

	
	int alertness = (m_alertnessValue + 1);

	if(m_dirty & eDF_Alertness && (m_flags & eRF_HasAlertness))
	{
		if(iconObj.m_alertnessObject)
		{
			iconObj.m_alertnessObject->GotoAndStop(alertness + 1);
		}

		m_dirty |= eDF_Height;
	}

	SFlashData::SFlashIcon::SFlashAlertness& altertnessObj = iconObj.m_alertnessStates[alertness];

	if(m_dirty & eDF_Height)
	{
		if(altertnessObj.m_up)
		{
			altertnessObj.m_up->SetVisible(m_heightDisplay==eHD_Above);
		}
		if(altertnessObj.m_down)
		{
			altertnessObj.m_down->SetVisible(m_heightDisplay==eHD_Below);
		}
		if(altertnessObj.m_even)
		{
			altertnessObj.m_even->SetVisible(m_heightDisplay==eHD_Even);
		}
	}

	if(m_dirty & eDF_Firing)
	{
		if(iconObj.m_firing)
		{
			iconObj.m_firing->GotoAndPlay(2);
		}
	}

	m_dirty = eDF_None;
}


//////////////////////////////////////////////////////////////////////////