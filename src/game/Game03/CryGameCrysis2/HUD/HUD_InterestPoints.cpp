#include "StdAfx.h"
#include "HUD_InterestPoints.h"
#include "IEntitySystem.h"
#include "IGameFramework.h"
#include "IViewSystem.h"
#include "GameCVars.h"
#include "IUIDraw.h"
#include "HUDMissionObjectiveSystem.h"
#include "Game.h"
#include "IItemSystem.h"
#include "IVehicleSystem.h"
#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"
#include "HUD/HUD_InteractWithObject.h"
#include "Graphics/2DRenderUtils.h"
#include "IAIObject.h"
#include "HUD/HUDDefines.h"
#include "HUD/HUDUtils.h"

//////////////////////////////////////////////////////////////////////////

struct compareDistance
{
	bool operator() (const CHUD_InterestPoints::SInterestPoint& left, const CHUD_InterestPoints::SInterestPoint& right ) const
	{
		return (left.m_distance > right.m_distance);
	}
};


//////////////////////////////////////////////////////////////////////////


CHUD_InterestPoints::CHUD_InterestPoints()
: m_lastFrameCount(0)
, m_lastFrameNearestToCenter(-1)
, m_xOffset(0.0f)
, m_nearestToCenterTacticalInfo(NULL)
, m_dirty(CHUD_InterestPoints::eDF_None)
, m_queryTimer(0.0f)
, m_visorActive(false)
, m_lookAt(0)
, m_edge(0.0f, 0.0f)
, m_sizeRatio(0.0f, 0.0f)
, m_attentionVisible(false)
, m_attentionObject(NULL)
{
	m_ignoreClasses.push_back(gEnv->pEntitySystem->GetClassRegistry()->FindClass( "DestroyableObject" ));
	m_ignoreClasses.push_back(gEnv->pEntitySystem->GetClassRegistry()->FindClass( "GeomEntity" ));
	m_ignoreClasses.push_back(gEnv->pEntitySystem->GetClassRegistry()->FindClass( "DeadBody" ));
	m_ignoreClasses.push_back(gEnv->pEntitySystem->GetClassRegistry()->FindClass( "RigidBodyEx" ));
	m_ignoreClasses.push_back(gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Breakage" ));
	m_ignoreClasses.push_back(gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Default" ));
	m_ignoreClasses.push_back(gEnv->pEntitySystem->GetClassRegistry()->FindClass( "AnimObject" ));
	

	InitTacticalInfoDefinitions();
	for(int i=0; i<k_maxFlashIconsSupported; ++i)
	{
		m_icons[i] = NULL;
		m_arrows[i] = NULL;
	}
}



CHUD_InterestPoints::~CHUD_InterestPoints()
{
	if(m_pFakeTexture)
		gEnv->pRenderer->RemoveTexture(m_pFakeTexture->GetTextureID());
}



void CHUD_InterestPoints::Init()
{
	IHUDAsset* pIcon = GetAsset2D();

	float assetWidth = (float)pIcon->GetWidth();
	float assetHeight = (float)pIcon->GetHeight();

	float visibleAssetWidth = ((float)gEnv->pRenderer->GetWidth() / (float)gEnv->pRenderer->GetHeight() ) * assetHeight;
	m_xOffset = assetWidth - visibleAssetWidth;

	float pixelSize;
	pIcon->GetVariable("m_pixelSize", pixelSize);
	
	m_maxDistanceToCenter.x = (pixelSize / assetWidth) * 50.0f;
	m_maxDistanceToCenter.y = (pixelSize / assetHeight) * 50.0f;

	if(gEnv->pRenderer)
	{
		m_pFakeTexture = gEnv->pRenderer->EF_LoadTexture("Textures/UI2/interest_point.tif", FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D);
		UpdateResolutionDependentData((float)gEnv->pRenderer->GetWidth(), (float)gEnv->pRenderer->GetHeight());
	}
	m_contextualInfo.SetAsset(pIcon);

	for(int i=0; i<k_maxFlashIconsSupported; ++i)
	{
		HUD_FLASVAROBJ_REG(pIcon, string().Format("I%d", i).c_str(), m_icons[i]);
		HUD_FLASVAROBJ_REG(pIcon, string().Format("I%d.Arrow", i).c_str(), m_arrows[i]);
	}
	HUD_FLASVAROBJ_REG(pIcon, "Attention", m_attentionObject);
}



void CHUD_InterestPoints::PreDelete()
{
	for(int i=0; i<k_maxFlashIconsSupported; ++i)
	{
		HUD_FLASHOBJ_SAFERELEASE(m_icons[i]);
		HUD_FLASHOBJ_SAFERELEASE(m_arrows[i]);
	}
	HUD_FLASHOBJ_SAFERELEASE(m_attentionObject);
}



void CHUD_InterestPoints::Reload()
{
}



void CHUD_InterestPoints::OnHUDEvent(const SHUDEvent& event)
{
	switch (event.eventType)
	{
	case eHUDEvent_OnLookAtChanged:
		{
			m_lookAt = (EntityId)event.GetData(0).GetInt();
			m_dirty |= CHUD_InterestPoints::eDF_Info;
		}
		break;
	case eHUDEvent_OnSpawn:
		{
			m_queryTimer = 0.0f;
		}
		break;
	case eHUDEvent_OnVisorChanged:
		{
			m_visorActive = event.eventIntData==1;
			m_queryTimer = 0.0f;
		}
	break;
	case eHUDEvent_OnSuitModeChanged:
		{
			SetSuitMode(static_cast<ENanoSuitMode>(event.eventIntData));
		}
		break;
	case eHUDEvent_OnEntityScanned:
		{
			CRY_TODO(9, 3, 2010, "Add some processing animation to the asset, so that the time until next query is covered.");
//			GetAsset2D()->SetVariable("Attention._visible", false);
		}
		break;
	default:
		{
			assert(event.eventType == eHUDEvent_OnResolutionChange);
			UpdateResolutionDependentData(event.GetData(0).GetFloat(), event.GetData(1).GetFloat());
		}
	}
}



void CHUD_InterestPoints::Update(float frameTime)
{
	m_queryTimer -= frameTime;
	if(m_queryTimer<=0.0f)
	{
		m_interestPoints.clear();

		QueryMissionObjectives();
		QueryAllInterestPoints();

		std::sort(m_interestPoints.begin(), m_interestPoints.end(), compareDistance());

		m_queryTimer			= g_pGame->GetHUD()->GetCVars()->hud_InterestPointsQueryRate;
		m_dirty |= CHUD_InterestPoints::eDF_Info;
	}

	UpdateAllIcons();

	int nearestToCenter = -1;
	bool attentionVisible = m_attentionVisible;

	nearestToCenter = GetNearestToCenter();

	IHUDAsset* pIcon = GetAsset2D();

	int currentFrameNearestToCenter = m_lastFrameNearestToCenter;


	if(currentFrameNearestToCenter!=-1)
	{
		attentionVisible = false;
	}

	currentFrameNearestToCenter = nearestToCenter;

	if(currentFrameNearestToCenter!=-1)
	{
		SInterestPoint& point = m_interestPoints[currentFrameNearestToCenter];
		if(point.m_scanned < CTacticalManager::eScanned_Max)
		{
			attentionVisible = true;
		}
	}

	if(m_attentionVisible != attentionVisible)
	{
		m_attentionObject->SetVisible(attentionVisible);
		if(attentionVisible)
		{
			m_attentionObject->GotoAndPlay(2);
		}
		m_attentionVisible = attentionVisible;
		m_dirty |= CHUD_InterestPoints::eDF_Info;
	}

	if(currentFrameNearestToCenter != -1)
	{
		SInterestPoint& point = m_interestPoints[currentFrameNearestToCenter];
		float x = point.m_screenPos.x * 0.01f * ((float)pIcon->GetWidth()-m_xOffset) + m_xOffset*0.5f;
		float y = point.m_screenPos.y * 0.01f * (float)pIcon->GetHeight();
		SFlashDisplayInfo info;
		info.SetX(x);
		info.SetY(y);
		m_attentionObject->SetDisplayInfo(info);
		if(!gEnv->bMultiplayer)
			m_contextualInfo.Update(x, y);
	}

	if(m_lookAt || m_nearestEntityToCenter || m_dirty & CHUD_InterestPoints::eDF_Info)
	{

		const char* primary = NULL;
		const char* secondary = NULL;
		const char* tertiary = NULL;
		int health = -1;
		int alertness = -1;

		SHUDEvent tacticalInfoEvent(eHUDEvent_OnTacticalInfoChanged);
		tacticalInfoEvent.ReserveData(4);

		if(m_dirty & CHUD_InterestPoints::eDF_Info)
		{
			primary = "";
			secondary = "";
			tertiary = "";
			if(m_nearestToCenterTacticalInfo)
			{
				primary = m_nearestToCenterTacticalInfo->m_primary.c_str();
				if(currentFrameNearestToCenter!=-1)
				{
					SInterestPoint& point = m_interestPoints[currentFrameNearestToCenter];
					if(point.m_scanned >= CTacticalManager::eScanned_Once)
					{
						secondary = m_nearestToCenterTacticalInfo->m_secondary.c_str();
						if(point.m_scanned >= CTacticalManager::eScanned_Max)
						{
							tertiary = m_nearestToCenterTacticalInfo->m_tertiary.c_str();
						}
					}
				}
			}
		}

		if(m_lookAt || m_nearestEntityToCenter)
		{
			if(IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_nearestEntityToCenter ? m_nearestEntityToCenter : m_lookAt))
			{
				float curhealth = (float)pActor->GetHealth();
				float maxhealth = (float)pActor->GetMaxHealth();
				float ratio = curhealth / max(maxhealth, 1.0f);
				health = int_round(ratio * 100.0f);

				if(!gEnv->bMultiplayer && curhealth>0.0f)
				{
					if(IEntity* pEntity = pActor->GetEntity())
					{
						if(IAIObject* pAIObject = pEntity->GetAI())
						{
							if(IAIActorProxy* pAIActorProxy = pAIObject->GetProxy())
							{
								alertness = pAIActorProxy->GetAlertnessState();
							}
						}
					}
				}
			}
		}

		tacticalInfoEvent.AddData(SHUDEventData((void*)primary));
		tacticalInfoEvent.AddData(SHUDEventData((void*)secondary));
		tacticalInfoEvent.AddData(SHUDEventData((void*)tertiary));
		tacticalInfoEvent.AddData(SHUDEventData(health));
		tacticalInfoEvent.AddData(SHUDEventData(alertness));

		CHUD::CallEvent(tacticalInfoEvent);

	}

	m_lastFrameNearestToCenter = currentFrameNearestToCenter;
	m_dirty = CHUD_InterestPoints::eDF_None;
}



void CHUD_InterestPoints::UpdateAllIcons()
{
	IHUDAsset* pIcon = GetAsset2D();

	Points::iterator it = m_interestPoints.begin();
	Points::iterator end = m_interestPoints.end();

	int count = 0;

	EntityId clientId = g_pGame->GetIGameFramework()->GetClientActorId();

	C2DRenderUtils* p2DRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();
	ScreenLayoutManager* pLayoutMgr = g_pGame->GetHUD()->GetLayoutManager();

	ScreenLayoutStates previousState = pLayoutMgr->GetState();
	pLayoutMgr->SetState(eSLO_DoNotAdaptToSafeArea);

	for(; it!=end; ++it)
	{
		SInterestPoint& point = (*it);

		if(clientId==point.m_entity)
		{
			continue;
		}

		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(point.m_entity);
		if(!pEntity)
		{
			continue;
		}

		AABB box;
		pEntity->GetWorldBounds(box);
		Vec3 entityPos = box.GetCenter();

		Vec3 newScreenPos;
		gEnv->pRenderer->ProjectToScreen(entityPos.x,entityPos.y,entityPos.z,&newScreenPos.x,&newScreenPos.y,&newScreenPos.z);
		newScreenPos.z = 0.0f;

		const bool isInFront = CHUDUtils::IsInFront(entityPos);

		if(point.m_objective)
		{
			CHUDUtils::ClampToScreen(newScreenPos, isInFront);
		}

		point.SetPos(newScreenPos);

		float x = point.m_screenPos.x * 0.01f;
		float y = point.m_screenPos.y * 0.01f;

		if(point.m_objective)
		{
			const float clampedX = clamp(x, m_edge.x, 1.0f-m_edge.x);
			const float clampedY = clamp(y, m_edge.y, 1.0f-m_edge.y);

			bool clamped = abs(clampedX-x)>0.001f || abs(clampedY-y)>0.001f;

			SFlashDisplayInfo info;

			if(m_lastFrameCount <= count)
			{
				info.SetVisible(true);
			}

			if(point.m_dirty & CHUD_InterestPoints::SInterestPoint::eDF_PosX)
			{
				info.SetX(clampedX * ((float)pIcon->GetWidth()-m_xOffset) + m_xOffset*0.5f);
			}

			if(point.m_dirty & CHUD_InterestPoints::SInterestPoint::eDF_PosY)
			{
				info.SetY(clampedY * (float)pIcon->GetHeight());
			}

			if(info.IsAnyFlagSet())
			{
				m_icons[count]->SetDisplayInfo(info);
			}
	
			info.Clear();

			if(clamped)
			{
				const Vec2 center(50.0f, 50.0f);
				const Vec2 dir = (point.m_screenPos - center);
				float rot = cry_atan2f(dir.x, dir.y);
				info.SetRotation(180.0f + RAD2DEG(-rot));
			}
			
			info.SetVisible(clamped);
			m_arrows[count]->SetDisplayInfo(info);

			if(point.m_dirty & CHUD_InterestPoints::SInterestPoint::eDF_Distance)
			{
				CryFixedStringT<16> distance;
				distance.Format("%.1fm", point.m_distance);
				m_icons[count]->SetMember("Distance", SFlashVarValue(distance.c_str()));
			}

			point.m_dirty = CHUD_InterestPoints::SInterestPoint::eDF_None;
			++count;
		}
		else if(isInFront)
		{
			x *= 800.0f;
			y *= 600.0f;
			p2DRenderUtils->PreRender();
			ColorF color(1.0f, 1.0f, 1.0f, point.m_alpha);
			if(m_pFakeTexture)
			{
				if(CHUD_InteractWithObject::IsUsableEntity(pEntity))
					color.b = color.g = 0.0f;

				p2DRenderUtils->DrawImageCentered(m_pFakeTexture->GetTextureID(), x, y, m_sizeRatio.x*800.0f, m_sizeRatio.y*600.0f, 0, color);
			}
			else
			{
				p2DRenderUtils->DrawLine(x-5, y-5, x-5, y+5, color);
				p2DRenderUtils->DrawLine(x-5, y+5, x+5, y+5, color);
				p2DRenderUtils->DrawLine(x+5, y+5, x+5, y-5, color);
				p2DRenderUtils->DrawLine(x+5, y-5, x-5, y-5, color);
			}
			p2DRenderUtils->PostRender();
		}

	}

	pLayoutMgr->SetState(previousState);

	for(int i = count; i<m_lastFrameCount; ++i)
	{
		m_icons[count]->SetVisible(false);
	}

	m_lastFrameCount = count;
}



int CHUD_InterestPoints::GetNearestToCenter()
{
	int count = 0;

	Vec2 center(50.0f, 50.0f);

	int nearestToCenter = -1;
	float nearestSquaredDistanceToCenter = 99999.9f;

	m_nearestToCenterTacticalInfo = NULL;
	EntityId nearestEntity = 0;

	bool objective = false;
	bool lookAtInfo = false;

	const int size = m_interestPoints.size();
	for(int i=0; i<size; ++i)
	{
		SInterestPoint& point = m_interestPoints[i];
		Vec2 distToCenter = (point.m_screenPos - center);
		if(abs(distToCenter.x) < m_maxDistanceToCenter.x && abs(distToCenter.y) < m_maxDistanceToCenter.y)
		{
			float squaredDistanceToCenter = (point.m_screenPos - center).GetLength2();
			if(squaredDistanceToCenter < nearestSquaredDistanceToCenter)
			{
				nearestSquaredDistanceToCenter = squaredDistanceToCenter;
				nearestToCenter = i;
			}
		}
	}
	
	if(nearestToCenter != -1)
	{
		SInterestPoint& point = m_interestPoints[nearestToCenter];

		m_nearestToCenterTacticalInfo = GetTacticalInfoDefinition(point.m_tacticalInfoClass, point.m_entity);
		lookAtInfo = (m_nearestToCenterTacticalInfo != NULL);
		nearestEntity = point.m_entity;
		objective = point.m_objective;
	}

	if(m_lookAt && nearestEntity!=m_lookAt)
	{
		if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_lookAt))
		{
			if(IEntityClass* pClass = pEntity->GetClass())
			{
				if(!stl::find(m_ignoreClasses, pClass))
				{
					m_nearestToCenterTacticalInfo = GetTacticalInfoDefinition(pClass->GetName(), m_lookAt);
					if(m_nearestToCenterTacticalInfo != NULL)
					{
						nearestEntity = m_lookAt;
						nearestToCenter = -1;
					}
				}
			}
		}
	}

	if(nearestEntity != m_nearestEntityToCenter)
	{
		m_nearestEntityToCenter = nearestEntity;
		SHUDEvent centerIconEvent;
		centerIconEvent.eventType = eHUDEvent_OnCenterEntityChanged;
		centerIconEvent.eventIntData = (int)nearestEntity;
		CHUD::CallEvent(centerIconEvent);
		m_contextualInfo.SetTarget(objective ? 0 : nearestEntity);
	}


	return nearestToCenter;
}



void CHUD_InterestPoints::InitTacticalInfoDefinitions()
{
	XmlNodeRef xml = GetISystem()->LoadXmlFile("Libs/UI/TacticalInfo.xml");
	if(!xml)
		return;
	
	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xml);

	const int numChildren = paramNode->GetChildCount();
	for(int i = 0; i < numChildren; ++i)
	{

		const IItemParamsNode * infoXML = paramNode->GetChild(i);
		if(stricmp(infoXML->GetName(),"Info"))
			continue;

		AddTacticalInfoDefinition(infoXML->GetAttribute("class"), infoXML->GetAttribute("primary"), infoXML->GetAttribute("secondary"), infoXML->GetAttribute("tertiary"));
	}

	paramNode->Release();
}



void CHUD_InterestPoints::AddTacticalInfoDefinition(const char* infoClass, const char* primary, const char* secondary, const char* tertiary)
{
	CHUD_InterestPoints::STacticalInfoDefinition info;

	info.m_primary = primary ? primary : "";
	info.m_secondary = secondary ? secondary : "";
	info.m_tertiary = tertiary ? tertiary : "";

	info.m_primary.replace("{}", "\n");
	info.m_secondary.replace("{}", "\n");
	info.m_tertiary.replace("{}", "\n");

	m_infoDefinitions.insert(std::make_pair(string(infoClass), info));
}


const CHUD_InterestPoints::STacticalInfoDefinition* CHUD_InterestPoints::GetTacticalInfoDefinition(const char* infoClass, const EntityId entity, const bool warn /*= true*/)
{
	if(!infoClass || !infoClass[0])
		return NULL; 

	CryFixedStringT<32> lookup = infoClass;

	// if entity is a vehicle, use its modification as part of the lookup
	if(IVehicle* pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(entity))
	{
		const char* mod =	pVehicle->GetModification();
		if(mod && mod[0])
		{
			lookup.Format("%s_%s", infoClass, mod);
		}
	}

	TacticalInfoDefinitions::const_iterator it = m_infoDefinitions.find(lookup);
	if (it != m_infoDefinitions.end())
	{
		return &(it->second);
	}

#ifndef _RELEASE
	if(warn && !stl::find(m_warnings, entity))
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Unknown Interest Point Type '%s'", lookup.c_str());
		m_warnings.push_back(entity);
	}
#endif //_RELEASE
	return NULL;
}



void CHUD_InterestPoints::QueryMissionObjectives()
{
	CHUDMissionObjectiveSystem *pMOSystem = g_pGame->GetMOSystem();
	if(!pMOSystem)
		return;

	Vec3 clientPos = CHUDUtils::GetClientPos();

	const std::vector<CHUDMissionObjective> &objectives = pMOSystem->GetObjectives();
	int numObjectives = objectives.size();
	for(int o = 0; o < numObjectives; ++o)
	{
		const CHUDMissionObjective &obj = objectives[o];

		if(obj.GetStatus() != CHUDMissionObjective::ACTIVATED)
			continue;

		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(obj.GetTrackedEntity());
		if(!pEntity)
			continue;

		SInterestPoint point;

		point.m_entity = pEntity->GetId();
		point.m_objective = !obj.IsSecondary();
		point.m_scanned = CTacticalManager::eScanned_Once;

		AABB box;
		pEntity->GetWorldBounds(box);
		point.SetDistance(clientPos.GetDistance(box.GetCenter()));

		point.m_tacticalInfoClass = obj.GetID();
		
		if(!GetTacticalInfoDefinition(point.m_tacticalInfoClass.c_str(), point.m_entity, false))
		{
			AddTacticalInfoDefinition(point.m_tacticalInfoClass.c_str(), obj.GetShortDescription(), obj.GetMessage());
		}

		m_interestPoints.push_back(point);
	}
}



void CHUD_InterestPoints::QueryAllInterestPoints()
{
	QueryInterestPointsType(CTacticalManager::eTacticalEntity_Item, 15.0f);

	if(!gEnv->bMultiplayer)
		QueryInterestPointsType(CTacticalManager::eTacticalEntity_Unit, 200.0f);

	QueryInterestPointsType(CTacticalManager::eTacticalEntity_Story, 15.0f);
}



void CHUD_InterestPoints::QueryInterestPointsType(const CTacticalManager::ETacticalEntityType type, const float distance)
{
	Vec3 clientPos = CHUDUtils::GetClientPos();

	const CTacticalManager::TInterestPoints& points = g_pGame->GetTacticalManager()->GetTacticalPoints(type);

	const float distanceSq = distance*distance;

	CTacticalManager::TInterestPoints::const_iterator it = points.begin();
	CTacticalManager::TInterestPoints::const_iterator end = points.end();
	for(; it!=end; ++it)
	{
		const CTacticalManager::STacticalInterestPoint &entry = (*it);
		SInterestPoint point;
		if(!entry.m_scanned && !m_visorActive)
			continue;

		if(QueryInterestPoint(point, gEnv->pEntitySystem->GetEntity(entry.m_entityId), clientPos, distanceSq))
		{
			CTacticalManager::TScanningCount scanned = entry.m_scanned;
			point.m_scanned = scanned;
			point.m_alpha = scanned ? 1.0f : 0.3f;
			point.m_objective = false;
			m_interestPoints.push_back(point);
		}
	}
}



bool CHUD_InterestPoints::QueryInterestPoint(SInterestPoint& outPoint, IEntity* pEntity, const Vec3& clientPos, const float distanceSq)
{
	if(!pEntity)
		return false;

	if(pEntity->IsHidden())
		return false;

	IEntityClass* pClass = pEntity->GetClass();
	if(!pClass || stl::find(m_ignoreClasses, pClass))
	{
		return false;
	}

	outPoint.m_entity = pEntity->GetId();
	AABB box;
	pEntity->GetWorldBounds(box);

	if(clientPos.GetSquaredDistance(box.GetCenter()) > distanceSq)
		return false;

	outPoint.SetDistance(clientPos.GetDistance(box.GetCenter()));

	const char *description = NULL;

	SmartScriptTable properties;
	if(pEntity->GetScriptTable() && pEntity->GetScriptTable()->GetValue("Properties", properties))
	{
		properties->GetValue("ui2_texture", description);
	}
	
	if(!description)
	{
		description = pClass->GetName();
	}

	if(description)
		outPoint.m_tacticalInfoClass = description;

	return true;
}



void CHUD_InterestPoints::SetSuitMode(const ENanoSuitMode mode)
{
	m_contextualInfo.SetSuitMode(mode);
}



void CHUD_InterestPoints::UpdateResolutionDependentData(const float width, const float height)
{
	m_edge.x = (OBJECTIVES_MIN_PIXEL_FROM_EDGE * __fres(width));
	m_edge.y = (OBJECTIVES_MIN_PIXEL_FROM_EDGE * __fres(height));

	m_sizeRatio.x = (float)m_pFakeTexture->GetWidth() * __fres(width);
	m_sizeRatio.y = (float)m_pFakeTexture->GetHeight() * __fres(height);
}


//////////////////////////////////////////////////////////////////////////
