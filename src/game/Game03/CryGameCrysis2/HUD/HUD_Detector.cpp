#include "StdAfx.h"
#include "HUD/HUD_Detector.h"

#include "HUD/HUD.h"
#include "HUD/HUDDefines.h"
#include "HUD/HUDUtils.h"

//////////////////////////////////////////////////////////////////////////


CHUD_Detector::CHUD_Detector()
: m_edge(0.0f, 0.0f)
, m_assetSize(0.0f, 0.0f)
, m_xOffset (0.0f)
, m_suitMode(eNanoSuitMode_Invalid)
{
	m_detected.reserve(k_maxFlashIconsSupported);
}



CHUD_Detector::~CHUD_Detector()
{
}



void CHUD_Detector::Init()
{
	IHUDAsset* pAsset = GetAsset2D();

	m_assetSize.x = (float)pAsset->GetWidth();
	m_assetSize.y = (float)pAsset->GetHeight();

	float visibleAssetWidth = ((float)gEnv->pRenderer->GetWidth() / (float)gEnv->pRenderer->GetHeight() ) * m_assetSize.y;
	m_xOffset = m_assetSize.x - visibleAssetWidth;

	if(gEnv->pRenderer)
	{
		m_edge.x = (OBJECTIVES_MIN_PIXEL_FROM_EDGE * __fres((float)gEnv->pRenderer->GetWidth()));
		m_edge.y = (OBJECTIVES_MIN_PIXEL_FROM_EDGE * __fres((float)gEnv->pRenderer->GetHeight()));
	}
	for(int i=0; i<k_maxFlashIconsSupported; ++i)
	{
		HUD_FLASVAROBJ_REG(pAsset, string().Format("Grenade%d", i).c_str(), m_icons[i].m_object);
	}
}



void CHUD_Detector::PreDelete()
{
	for(int i=0; i<k_maxFlashIconsSupported; ++i)
	{
		HUD_FLASHOBJ_SAFERELEASE(m_icons[i].m_object);
	}
}



void CHUD_Detector::OnHUDEvent(const SHUDEvent& event)
{
	switch (event.eventType)
	{
	case eHUDEvent_OnExplosiveSpawned:
		{
			OnExplosiveSpawned((EntityId)event.GetData(0).GetInt());
		}
		break;
	case eHUDEvent_OnSuitModeChanged:
		{
			OnSuitModeChanged(static_cast<ENanoSuitMode>(event.eventIntData));
		}
		break;
	}
}
 


void CHUD_Detector::Update(float frameTime)
{
	const Vec3 clientPos = CHUDUtils::GetClientPos();

	SFlashDisplayInfo info;
	
	int i;
	int size = m_detected.size();
	for(i=0; i<size; ++i)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_detected[i].m_id);
		if(!pEntity)
		{
			m_detected[i] = m_detected[size-1];
			m_detected.pop_back();
			--i; --size;
			continue;
		}
		m_detected[i].m_pEntity = pEntity;
		m_detected[i].m_distance = clientPos.GetDistance(pEntity->GetWorldPos());
	}

	std::sort(m_detected.begin(), m_detected.end());

	size = (m_suitMode!=eNanoSuitMode_Tactical) ? 0 : min(m_detected.size(), k_maxFlashIconsSupported);
	for(i=0; i<size; ++i)
	{
		IEntity* pEntity = m_detected[i].m_pEntity;

		const Vec3 worldPos = pEntity->GetWorldPos();
		Vec3 screenPos;
		gEnv->pRenderer->ProjectToScreen(worldPos.x,worldPos.y,worldPos.z,&screenPos.x,&screenPos.y,&screenPos.z);
		CHUDUtils::ClampToScreen(screenPos, CHUDUtils::IsInFront(worldPos));

		float x = screenPos.x * 0.01f;
		float y = screenPos.y * 0.01f;

		x = clamp(x, m_edge.x, 1.0f-m_edge.x);
		y = clamp(y, m_edge.y, 1.0f-m_edge.y);

		SDetectorIcon& icon = m_icons[i];

		if(icon.m_posX != x)
		{
			info.SetX(x * (m_assetSize.x-m_xOffset) + m_xOffset*0.5f);
			icon.m_posX = x;
		}

		if(icon.m_posY != y)
		{
			info.SetY(y * (m_assetSize.y));
			icon.m_posY = y;
		}

		if(icon.m_visible != true)
		{
			info.SetVisible(true);
			icon.m_visible = true;
		}

		if(info.IsAnyFlagSet())
		{
			icon.m_object->SetDisplayInfo(info);
		}

		info.Clear();

		const float distance = m_detected[i].m_distance;
		if(fabs(icon.m_distance-distance) > 1.0f)
		{
			CryFixedStringT<16> distanceString;
			distanceString.Format("%.0fm", distance);
			icon.m_object->SetMember("Distance", SFlashVarValue(distanceString.c_str()));
			icon.m_distance = distance;
		}
	}

	info.SetVisible(false);

	while(i<k_maxFlashIconsSupported)
	{
		SDetectorIcon& icon = m_icons[i];
		if(icon.m_visible != false)
		{
			icon.m_object->SetDisplayInfo(info);
			icon.m_visible = false;
		}

		++i;
	}
}



void CHUD_Detector::OnExplosiveSpawned(const EntityId grenade)
{
	if(m_detected.size()>=k_maxDetectionsSupported)
		return;
	stl::push_back_unique(m_detected, SDetection(grenade));
}



void CHUD_Detector::OnSuitModeChanged(const ENanoSuitMode suitMode)
{
	m_suitMode = suitMode;
}

//////////////////////////////////////////////////////////////////////////