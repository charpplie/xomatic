/************************************************************************/
/* UI Minimap Texture, Ben Parbury , 2009						  									*/
/************************************************************************/

#include "StdAfx.h"
#include "UIMinimapTextureElement.h"

#include "HUD/HUD.h"
#include "HUD/HUDAsset.h"
#include "HUD/HUDCVars.h"
#include "HUD/ScreenLayoutManager.h"

#include <ILevelSystem.h>
#include "GameCVars.h"
#include "Player.h"

#include "Utility/CryWatch.h"

#if !(defined(_RELEASE) || defined(PROFILE))
#define MapDeployWatch(...)        (m_hud_mapDeployDebug) && CryWatch (__VA_ARGS__);
#else
#define MapDeployWatch(...)        (void)0
#endif

static const float RADAR_W = 103.0f;
static const float RADAR_H = 104.0f;

//--------------------------------------------------------------------------------
CUIMinimapTextureElement::CUIMinimapTextureElement() :
CUIElement()
{
	m_minimapTexture = NULL;
	m_minimapTextureMask = NULL;
	m_mapTextureMask = NULL;
	m_startX = 0.0f;
	m_startY = 0.0f;
	m_endX = 0.0f;
	m_endY = 0.0f;
	m_scale = 0.1f;
	m_enabled = false;
	m_rotation = 0.0f;
	m_playerX = 0.0f;
	m_playerY = 0.0f;
	m_usingMenuMap = false;
	m_wantToUseMenu = false;
	m_mapCenter.zero();
	m_mapTextureScale = 1.0f;
	m_blindAlpha = 1.0f;

	CHUD::AddHUDEventListener(this,"OnInitPlayer");
	CHUD::AddHUDEventListener(this,"OnHUDReload");
	CHUD::AddHUDEventListener(this,"RadarRotation");
	CHUD::AddHUDEventListener(this,"AlignToRadarAsset");
	CHUD::AddHUDEventListener(this,"ShowSelectMenuMap");
	CHUD::AddHUDEventListener(this,"HideSelectMenuMap");
	CHUD::AddHUDEventListener(this,"OnBlind");
	CHUD::AddHUDEventListener(this,"OnEndBlind");
	CHUD::AddHUDEventListener(this,"OnSpawn");
	//CHUD::AddHUDEventListener(this,"OpenedIngameMenu");
	//CHUD::AddHUDEventListener(this,"ToggleMap");

	SAFE_HARDWARE_MOUSE_FUNC(AddListener(this));

	if (gEnv->pInput)
	{
		gEnv->pInput->AddEventListener(this);
	}

	if(gEnv->pConsole)
	{
		gEnv->pConsole->Register("hud_minimapOverrideScale", &m_hud_minimapOverrideScale, 0.0f, VF_CHEAT, CVARHELP("overrides the scale of the minimap background texture"));
		gEnv->pConsole->Register("hud_minimapMapOverrideScale", &m_hud_minimapMapOverrideScale, 0.0f, VF_CHEAT, CVARHELP("overrides the scale of the main map background texture"));
		gEnv->pConsole->Register("hud_mapDeployDebug", &m_hud_mapDeployDebug, 0.0f, VF_CHEAT, CVARHELP("Adds debug watches for map deployment"));
	}
}

//--------------------------------------------------------------------------------
CUIMinimapTextureElement::~CUIMinimapTextureElement()
{
	if(m_minimapTexture)
	{
		gEnv->pRenderer->RemoveTexture(m_minimapTexture->GetTextureID());
	}
	
	if(m_minimapTextureMask)
	{
		gEnv->pRenderer->RemoveTexture(m_minimapTextureMask->GetTextureID());
	}

	if(m_mapTextureMask)
	{
		gEnv->pRenderer->RemoveTexture(m_mapTextureMask->GetTextureID());
	}
	
	CHUD::RemoveHUDEventListener(this);

	SAFE_HARDWARE_MOUSE_FUNC(RemoveListener(this));

	if (gEnv->pInput)
	{
		gEnv->pInput->RemoveEventListener(this);
	}

	if(gEnv->pConsole)
	{
		gEnv->pConsole->UnregisterVariable("hud_minimapOverrideScale", true);
		gEnv->pConsole->UnregisterVariable("hud_minimapMapOverrideScale", true);
		gEnv->pConsole->UnregisterVariable("hud_mapDeployDebug", true);
	}
}

//--------------------------------------------------------------------------------
void CUIMinimapTextureElement::OnHUDEvent(const SHUDEvent& event)
{
	switch( event.eventType )
	{
	case eHUDEvent_OnInitPlayer:
	case eHUDEvent_OnHUDReload :
		{
			LoadMinimap();
			break;
		}
	case eHUDEvent_RadarRotation:
		{
			SetRotation(event.GetData(0).GetFloat());
			break;
		}
	case eHUDEvent_AlignToRadarAsset:
		{
			AttachToRadar(static_cast<IHUDAsset*>(event.GetData(0).GetPtr()));
			break;
		}
	case eHUDEvent_ShowSelectMenuMap :
		m_wantToUseMenu = true;
		break;
	case eHUDEvent_HideSelectMenuMap :
		m_wantToUseMenu = false;
		break;
	//case eHUDEvent_OnResolutionChange :
	//	{
	//		AttachToRadar(m_attached);
	//		break;
	//	}
	case eHUDEvent_OnEndBlind :
	case eHUDEvent_OnSpawn :
		m_blindAlpha = 1.0f;
		break;
	default : //case eHUDEvent_OnBlind :
		CRY_ASSERT( eHUDEvent_OnBlind == event.eventType );
		m_blindAlpha = event.GetData(0).GetFloat();
		break;
	}
}

//--------------------------------------------------------------------------------
void CUIMinimapTextureElement::LoadMinimap()
{
	const char* levelName = g_pGame->GetIGameFramework()->GetLevelName();

	if(levelName)
	{
		CRY_ASSERT_MESSAGE(levelName[0] != '\0', "Invalid Level name - failing to load minimap");
		if( levelName[0] == '\0' )
		{
			return;
		}

		ILevelInfo* pLevelInfo = gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetLevelInfo(levelName);
		if(pLevelInfo)
		{
			string mapName = levelName;
			int slashPos = mapName.rfind('/');
			mapName = mapName.substr(slashPos+1, mapName.length()-slashPos);	//remove the Wars/ from in front

			string levelXml;
			levelXml.Format("%s/%s.xml", pLevelInfo->GetPath(), mapName.c_str());

			XmlNodeRef mapInfo = GetISystem()->LoadXmlFile(levelXml.c_str());
			if(mapInfo)
			{
				XmlNodeRef minimapNode = mapInfo->findChild("MiniMap");
				if(minimapNode)
				{
					LoadMinimap(pLevelInfo, minimapNode);
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------
void CUIMinimapTextureElement::LoadMinimap(ILevelInfo *pLevel, const XmlNodeRef node)
{
	CRY_ASSERT(pLevel);
	CRY_ASSERT(node->haveAttr("Filename"));
	const char* filename = node->getAttr("Filename");

	CRY_ASSERT(node->haveAttr("startX"));
	node->getAttr("startX", m_startX);

	CRY_ASSERT(node->haveAttr("startY"));
	node->getAttr("startY", m_startY);

	CRY_ASSERT(node->haveAttr("endX"));
	node->getAttr("endX", m_endX);

	CRY_ASSERT(node->haveAttr("endY"));
	node->getAttr("endY", m_endY);

	if(node->haveAttr("scale"))	//new parameter so if it's setup minimap should also be valid
	{
		node->getAttr("scale", m_scale);
		m_enabled = true;
	}
	else
	{
		m_enabled = false;
	}

	string texPath = pLevel->GetPath();
	texPath.append("/");
	texPath.append(filename);

	m_minimapTexture = gEnv->pRenderer->EF_LoadTexture(texPath.c_str(), FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D);
	m_minimapTextureMask = gEnv->pRenderer->EF_LoadTexture("Textures/UI2/generic/minimap_mask.tif", FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D);
	m_mapTextureMask = gEnv->pRenderer->EF_LoadTexture("Textures/UI2/generic/map_mask.tif", FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D);
	CRY_ASSERT(m_minimapTexture);
	CRY_ASSERT(m_minimapTextureMask);
	CRY_ASSERT(m_mapTextureMask);

	float mapRotation = 0.0f;
	if(node->haveAttr("mapRotation"))
	{
		node->getAttr("mapRotation", mapRotation);
		mapRotation = DEG2RAD(mapRotation);
	}

	float mapScale = 1.0f;
	if(node->haveAttr("mapScale"))
	{
		node->getAttr("mapScale", mapScale);
	}

	if(node->haveAttr("mapTextureScale"))
	{
		node->getAttr("mapTextureScale", m_mapTextureScale);
	}

	m_mapCenter.x = m_startX + ((m_endX - m_startX)/2.0f);
	if(node->haveAttr("mapCenterX"))
	{
		node->getAttr("mapCenterX", m_mapCenter.x);
	}

	m_mapCenter.y = m_startY + ((m_endY - m_startY)/2.0f);
	if(node->haveAttr("mapCenterY"))
	{
		node->getAttr("mapCenterY", m_mapCenter.y);
	}

	SHUDEvent event;
	event.eventType = eHUDEvent_MapInfo;
	event.ReserveData(4);

	SHUDEventData dataCenterX = m_mapCenter.x;
	SHUDEventData dataCenterY = m_mapCenter.y;
	SHUDEventData dataRotation = mapRotation;
	SHUDEventData dataScale = mapScale;
	
	event.AddData(dataCenterX);
	event.AddData(dataCenterY);
	event.AddData(dataRotation);
	event.AddData(dataScale);

	CHUD::CallEvent(event);
}

//--------------------------------------------------------------------------------
void CUIMinimapTextureElement::Update(float frameTime)
{
	if(!m_enabled)
		return;

	m_usingMenuMap = m_wantToUseMenu;

	IActor* pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pClientActor)
	{
		Vec3 centerPos;
		if(!m_usingMenuMap)
		{
			centerPos = pClientActor->GetEntity()->GetWorldPos();
		}
		else
		{
#if !defined(_RELEASE)
			if(g_pGameCVars->hud_mapOverrideCenterX != 0.0f)
			{
				m_mapCenter.x = g_pGameCVars->hud_mapOverrideCenterX;
			}

			if(g_pGameCVars->hud_mapOverrideCenterY != 0.0f)
			{
				m_mapCenter.y = g_pGameCVars->hud_mapOverrideCenterY;
			}
#endif
			centerPos.Set(m_mapCenter.x, m_mapCenter.y, 0.0f);
		}
		
		//flip x and y match minimap
		m_playerY = GetMinimapFraction(m_startX, m_endX, centerPos.x);
		m_playerX = GetMinimapFraction(m_startY, m_endY, centerPos.y);
	
		//mirror x and y so going to texture space
		m_playerX = 1.0f - m_playerX;
		m_playerY = 1.0f - m_playerY;
	}
#if !defined(_RELEASE)
	if(m_hud_minimapOverrideScale != 0.0f)
	{
		m_scale = m_hud_minimapOverrideScale;
	}

	if(m_hud_minimapMapOverrideScale != 0.0f)
	{
		m_mapTextureScale = m_hud_minimapMapOverrideScale;
	}
#endif
	CUIElement::Update(frameTime);

#if !defined(_RELEASE)
	if(m_hud_mapDeployDebug)
	{
		float x, y;
		GetGameCoordFromMouse(&x, &y);
	}
#endif
}

//--------------------------------------------------------------------------------
void CUIMinimapTextureElement::Draw(void) const
{
	if(!m_enabled)
		return;

	// transitioning between maps
	if( m_usingMenuMap != m_wantToUseMenu )
		return;

	float scale = m_scale;
	if(m_usingMenuMap)
	{
		scale *= m_mapTextureScale;
	}
	else if( !g_pGame->GetHUD()->GetCVars()->hud_radar_ShowRadarBg )
	{
		return;
	}
	
	float s0 = clamp(m_playerX - scale, 0.0f, 1.0f);
	float s1 = clamp(m_playerX + scale, 0.0f, 1.0f);

	//mirror minimap texture on X
	s0 = 1.0f - s0;
	s1 = 1.0f - s1;

	float t0 = clamp(m_playerY - scale, 0.0f, 1.0f);
	float t1 = clamp(m_playerY + scale, 0.0f, 1.0f);

	const int textureId = m_usingMenuMap ? m_mapTextureMask->GetTextureID() : m_minimapTextureMask->GetTextureID();
	gEnv->pRenderer->SetState(GS_COLMASK_A|GS_NODEPTHTEST);
	if(m_usingMenuMap)
		gEnv->pRenderer->Draw2dImage(GetPosX(), GetPosY(), GetWidth(), GetHeight(), textureId, 0.0f, 0.0f, 1.0f, 1.0f, 180.0f);
	else
		gEnv->pRenderer->Draw2dImage(GetPosX(), GetPosY(), GetWidth(), GetHeight(), textureId, 0.0f, 0.0f, 1.0f, 1.0f, m_rotation);



	gEnv->pRenderer->SetState(GS_BLSRC_DSTALPHA|GS_BLDST_ONEMINUSDSTALPHA|GS_COLMASK_RGB|GS_NODEPTHTEST);
	gEnv->pRenderer->Draw2dImage(GetPosX(), GetPosY(), GetWidth(), GetHeight(), m_minimapTexture->GetTextureID(), s0, t0, s1, t1, m_rotation);
}

//--------------------------------------------------------------------------------
void CUIMinimapTextureElement::SetRotation(float rot)
{
	//need to add 180 degs to move into texture rotation space
	m_rotation = rot + 90.0f;
}

//--------------------------------------------------------------------------------
float CUIMinimapTextureElement::GetMinimapFraction(float start, float end, float current)
{
	float range = end - start;
	float base = current - start;
	return clamp(base/range, 0.0f, 1.0f);
}

//--------------------------------------------------------------------------------
void CUIMinimapTextureElement::AttachToRadar(IHUDAsset* radar)
{
	CRY_ASSERT(radar);

	m_attached = radar;

	const SHUDSizeAndPositionInfo* info = m_attached->GetInfo();

	int x, y, w, h;
	float aspectRatio;
	m_attached->GetFlashPlayer()->GetViewport(x, y, w, h, aspectRatio);

	AdjustPosAndSize((float)x, (float)y, (float)w, (float)h);
}

//--------------------------------------------------------------------------------
void CUIMinimapTextureElement::AdjustPosAndSize(float x, float y, float w, float h)
{
	const float ratio = h / (float)m_attached->GetHeight();

	const float width = RADAR_W * ratio;
	const float height = RADAR_H * ratio;

	float fx = ((float)gEnv->pRenderer->GetWidth() - width) * 0.5f;
	float fy = (float)gEnv->pRenderer->GetHeight() - height;
	float fw = width;
	float fh = height;

	ScreenLayoutManager* pLayoutManager = g_pGame->GetHUD()->GetLayoutManager();
	pLayoutManager->ConvertFromRenderToVirtualScreenSpace( &fx, &fy );
	pLayoutManager->ConvertFromRenderToVirtualScreenSpace( &fw, &fh );

	SetPos(fx, fy);
	SetSize(fw, fh);
}

//--------------------------------------------------------------------------------
void CUIMinimapTextureElement::OnHardwareMouseEvent(int iX,int iY, EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta)
{
	if(m_usingMenuMap && eHardwareMouseEvent == HARDWAREMOUSEEVENT_LBUTTONDOWN)
	{
		float x = 0;
		float y = 0;

		if(GetGameCoordFromMouse(&x, &y))
		{
			DeployStrikeAtGameCoord(x, y);
		}
	}
}

//--------------------------------------------------------------------------------
bool CUIMinimapTextureElement::OnInputEvent(const SInputEvent &rInputEvent)
{
	if(m_usingMenuMap && rInputEvent.state == eIS_Pressed)
	{
		if(rInputEvent.keyId == eKI_XI_A || rInputEvent.keyId == eKI_PS3_Cross)
		{
			float x = 0;
			float y = 0;
			
			if(GetGameCoordFromMouse(&x, &y))
			{
				DeployStrikeAtGameCoord(x, y);
			}
		}
	}
	return false;
}

//--------------------------------------------------------------------------------
bool CUIMinimapTextureElement::GetGameCoordFromMouse(float *gameX, float* gameY)
{
	if(!m_usingMenuMap)
		return false;

	float fX = 0.0f;
	float fY = 0.0f;
	SAFE_HARDWARE_MOUSE_FUNC(GetHardwareMouseClientPosition(&fX, &fY));

	MapDeployWatch("MouseEvent %.0f, %.0f", fX, fY);

	//convert UIMinimapTextureElement into render screen space coords
	float posX = GetPosX();
	float posY = GetPosY();
	float width = GetWidth();
	float height = GetHeight();
	ScreenLayoutManager* pLayoutManager = g_pGame->GetHUD()->GetLayoutManager();
	pLayoutManager->ConvertFromVirtualToRenderScreenSpace(&posX, &posY);
	pLayoutManager->ConvertFromVirtualToRenderScreenSpace(&width, &height);

	MapDeployWatch("Minimap X, Y, W, H %.1f, %.1f, %.1f, %.1f", posX, posY, width, height);

	if(fX > posX && fX < posX + width &&
		fY > posY && fY < posY + height)
	{
		//convert render screen space coords into fraction on map
		float fractionX = GetMinimapFractionFromScreen(fX, posX, width);
		float fractionY = GetMinimapFractionFromScreen(fY, posY, height);

		MapDeployWatch("Swapped MinimapFraction %.2f, %.2f", fractionX, fractionY);

		float rotation = DEG2RAD(m_rotation);
		MapDeployWatch("Rotation %.2f (%.2f)", rotation, m_rotation);

		//rotate map fractions
		float sinRotation = cry_sinf(rotation);
		float cosRotation = cry_cosf(rotation);
		
		float rotatedFractionX = (cosRotation * fractionY) + (sinRotation * fractionX);
		float rotatedFractionY = (cosRotation * fractionX) - (sinRotation * fractionY);

		MapDeployWatch("Rotated Fraction %.2f, %.2f", rotatedFractionX, rotatedFractionY);

		//convert fraction on map into world coords
		float x = GetGameXFromMapFraction(rotatedFractionX);
		float y = GetGameYFromMapFraction(rotatedFractionY);

		MapDeployWatch("GameCoord %.1f, %.1f", x, y);
		*gameX = x;
		*gameY = y;

		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------
void CUIMinimapTextureElement::DeployStrikeAtGameCoord(float x, float y)
{
	Vec3 pos(x, y, 0.0f);

	IActor* pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pClientActor)
	{
		CPlayer* pPlayer = static_cast<CPlayer*>(pClientActor);
		pPlayer->SendPerkEvent(EPE_OrbitalStrikeFromMap, &pos);
	}
}

//--------------------------------------------------------------------------------
float CUIMinimapTextureElement::GetGameXFromMapFraction(float fraction)
{
	float scale =	m_scale * m_mapTextureScale;

	float centerX = GetMinimapFraction(m_startX, m_endX, m_mapCenter.x);
	MapDeployWatch("CenterX %.2f, %.2f", centerX, m_mapCenter.x);
	float s0 = clamp(centerX - scale, 0.0f, 1.0f);
	float s1 = clamp(centerX + scale, 0.0f, 1.0f);

	float screenStartX = GetPosFromFraction(s0, m_startX, m_endX);
	float screenEndX = GetPosFromFraction(s1, m_startX, m_endX);

	MapDeployWatch("ScreenX %.1f, %.1f (%.1f, %.1f)", screenStartX, screenEndX, m_startX, m_endX);
	return GetPosFromFraction(fraction, screenStartX, screenEndX);
}

//--------------------------------------------------------------------------------
float CUIMinimapTextureElement::GetGameYFromMapFraction(float fraction)
{
	float scale =	m_scale * m_mapTextureScale;

	float centerY = GetMinimapFraction(m_startY, m_endY, m_mapCenter.y);
	MapDeployWatch("CenterY %.2f, %.2f", centerY, m_mapCenter.y);
	float t0 = clamp(centerY - scale, 0.0f, 1.0f);
	float t1 = clamp(centerY + scale, 0.0f, 1.0f);

	float screenStartY = GetPosFromFraction(t0, m_startY, m_endY);
	float screenEndY = GetPosFromFraction(t1, m_startY, m_endY);

	MapDeployWatch("ScreenY %.1f, %.1f (%.1f, %.1f)", screenStartY, screenEndY, m_startY, m_endY);
	return GetPosFromFraction(fraction, screenStartY, screenEndY);
}

//static--------------------------------------------------------------------------------
float CUIMinimapTextureElement::GetPosFromFraction(float fraction, float start, float end)
{
	float range = end - start;
	if(fraction < 0.0f)
	{
		fraction = 1.0f + fraction;
	}
	return start + (fraction * range);
}

//--------------------------------------------------------------------------------
float CUIMinimapTextureElement::GetMinimapFractionFromScreen(float posX, float mapPosX, float mapWidth)
{
	float indentX = posX - mapPosX;
	return indentX / mapWidth;
}
