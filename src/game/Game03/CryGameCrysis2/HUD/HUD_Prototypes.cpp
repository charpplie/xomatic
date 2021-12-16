/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 
	G02 HUD using Flash player
	Code specific to G02 should go here
	For code needed by all games, see CHUDCommon


-------------------------------------------------------------------------
History:
- 07:11:2005: Created by Julien Darre
- 01:02:2006: Modified by Jan Müller
- 22:02:2006: Refactored for G04 by Matthew Jack
- 2007: Refactored by Jan Müller

*************************************************************************/
#include "StdAfx.h"
#include <StlUtils.h>
#include <ctype.h>

#include "Game.h"
#include "GameActions.h"
#include "GameCVars.h"
#include "HUD.h"

//#include "MPTutorial.h"

#include "HUDObituary.h"
#include "HUDRadar.h"
#include "HUDScore.h"
#include "HUDTextArea.h"
#include "HUDTextChat.h"

#include "GameFlashAnimation.h"
#include "GameFlashLogic.h"
#include "IPlayerProfiles.h"

#include "IUIDraw.h"
#include "ISound.h"
#include "IPlayerInput.h"
#include "IWorldQuery.h"
#include "IInput.h"
#include "IMaterialEffects.h"
#include "ISaveGame.h"
#include "ILoadGame.h"
#include "ICryPak.h"
#include "IMovieSystem.h"

#include "GameRules.h"
#include "Item.h"
#include "Weapon.h"
//#include "OffHand.h"

#include "Tweaks/HUDTweakMenu.h"

//#include "HUDVehicleInterface.h"
#include "HUDPowerStruggle.h"
#include "HUDScopes.h"
#include "HUDCrosshair.h"
#include "HUDTagNames.h"
#include "HUDSilhouettes.h"
#include "HUD_RadarPrototypes.h"

#include "Menus/OptionsManager.h"
#include "WeaponSystem.h"
//#include "Radio.h"

//-----------------------------------------------------------------------------------------------------

void CHUD::PrototypeRender_All()
{
  // current HUD implementation is not thread safe
  assert(!gEnv->pRenderer->EF_Query(EFQ_RenderMultithreaded));
  if(gEnv->pRenderer->EF_Query(EFQ_RenderMultithreaded))
    return;

	UpdateTransparency();
	
	const CCamera& cam = gEnv->pRenderer->GetCamera();
	Vec3 WorldCameraPos = cam.GetPosition();
	Vec3 WorldCameraFwd = cam.GetMatrix().GetColumn1()*cam.GetEdgeP().y * 0.1f;
	Vec3 WorldCameraRgt = cam.GetMatrix().GetColumn0()*-cam.GetEdgeP().x * 0.1f;
	Vec3 WorldCameraUp = cam.GetMatrix().GetColumn2()*cam.GetEdgeP().z * 0.1f;

	//IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	//Vec3 PlayerPos = pActor->GetEntity()->GetWorldPos();

	PrototypeRender_LevelObjectives(WorldCameraPos, WorldCameraFwd);

	m_pUIDraw->PreRender();

	PrototypeRender_SuitMenu();
	PrototypeRender_HealthBar();
	PrototypeRender_EnergyMeter();
	PrototypeRender_Crosshair();
	PrototypeRender_AmmoCounter();
	PrototypeRender_WeaponSelectionMenu();
	PrototypeRender_Cover();
	Prototype_RenderTextMessages();
	
	m_pUIDraw->PostRender();

	IRenderAuxGeom* pRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags oldFlags = pRenderer->GetRenderFlags();
	SAuxGeomRenderFlags newFlags = e_Def3DPublicRenderflags;
	newFlags.SetAlphaBlendMode(e_AlphaBlended);
	newFlags.SetDepthTestFlag(e_DepthTestOff);
	newFlags.SetCullMode(e_CullModeNone);
	pRenderer->SetRenderFlags(newFlags);

	{
		Vec3 WorldScreenCenter = WorldCameraPos + WorldCameraFwd;
		Vec3 WorldScreenUp = WorldCameraUp;
		Vec3 WorldScreenRight = WorldCameraRgt;
		float WorldScreenUpLen = WorldScreenUp.GetLength();
		float WorldScreenRightLen = WorldScreenRight.GetLength();
		float WorldScreenSquareSize = min(WorldScreenUpLen, WorldScreenRightLen);

		{
/*
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(WorldScreenCenter, ColorB(255,0,0,255), WorldScreenCenter + WorldScreenRight * 1.0f, ColorB(255,255,255,255), 5.0f);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(WorldScreenCenter, ColorB(0,255,0,255), WorldScreenCenter + WorldScreenUp * 1.0f, ColorB(255,255,255,255), 5.0f);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(WorldScreenCenter, ColorB(0,0,255,255), WorldScreenCenter + WorldCameraFwd * 1.0f, ColorB(255,255,255,255), 5.0f);
*/

			PrototypeRender_SuitMenu_Old(WorldScreenCenter, WorldScreenUp, WorldScreenRight, WorldScreenSquareSize);
			Prototype_UpdateWeaponCustomMenu_1_v2(WorldScreenCenter, WorldScreenUp, WorldScreenRight, WorldScreenSquareSize);
			//PrototypeRender_EnergyMeter(WorldScreenCenter, WorldScreenUp, WorldScreenRight, WorldScreenSquareSize);
		}
	}
	
	pRenderer->SetRenderFlags(oldFlags);
	m_pHUDDebugRadar->Draw();
}

void CHUD::UpdateTransparency()
{
	if(m_pNanoSuit == NULL)
		return;

	if(m_pNanoSuit->GetGameParams().GetState() != eNanoSuitState_Normal)
	{
		m_prototypeHUDOpacity -= gEnv->pTimer->GetFrameTime();
		if( m_prototypeHUDOpacity < 0.25f)
			m_prototypeHUDOpacity = 1.0f;
	}
	else
	{
		m_prototypeHUDOpacity = 1.0f;
	}
}

void CHUD::Prototype_RenderTextMessages()
{
	static Vec2 positions[MAX_TEXT_MESSAGE_POS]={
		Vec2(400, 80),
		Vec2(400, 280),
		Vec2(400, 480),
		Vec2(400, 340),
	};

	static float sizes[MAX_TEXT_MESSAGE_POS]=
	{
		25.0f,
		20.0f,
		25.0f,
		20.0f
	};

	float frameTime=gEnv->pTimer->GetFrameTime();

	for (int i=0;i<MAX_TEXT_MESSAGE_POS;i++)
	{
		if (m_textMessageTimeout[i]>0.0f && !m_textMessage[i].empty())
		{
			m_pUIDraw->DrawTextSimple(m_pDefaultFont, positions[i].x, positions[i].y, sizes[i], sizes[i], 
				m_textMessage[i].c_str(), m_textMessageColor[i], UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER);
			m_textMessageTimeout[i]-=frameTime;
		}
	}
};

//-----------------------------------------------------------------------------------------------------

void CHUD::PrototypeRender_SuitMenu_Old(Vec3 wsCenter, Vec3 wsUp, Vec3 wsRight, float wsSize)
{
	if (!m_pNanoSuit)
		return;

	if (!m_actualSuitMenuOpen)
		return;

	float normalSize = 0.075f * wsSize;
	float selectedSize = 0.12f * wsSize;
	float radius = 0.13f * wsSize;

	wsUp.Normalize();
	wsRight.Normalize();

	int index[4] = { eNanoSuitMode_COMBAT, eNanoSuitMode_MANEUVER, eNanoSuitMode_INFILTRATION, eNanoSuitMode_TACTICAL };
	ColorB color0[4] = { ColorB(0xFF, 0, 0, 0x80), ColorB(0, 0xFF, 0, 0x80), ColorB(0, 0, 0xFF, 0x80), ColorB(0xFF, 0xFF, 0, 0x80) };
	ColorB color1[4] = { ColorB(0xFF, 0, 0, 0xC0), ColorB(0, 0xFF, 0, 0xC0), ColorB(0, 0, 0xFF, 0xC0), ColorB(0xFF, 0xFF, 0, 0xC0) };
	Vec2 offset[4] = { Vec2(1, 0), Vec2(0, -1), Vec2(-1, 0), Vec2(0, 1) };
	Vec2 letter[4][5] = { { Vec2(1, 0.8f), Vec2(-0.2f, 1), Vec2(-1, 0), Vec2(-0.2f, -1), Vec2(1, -0.8f), }, // C
												{ Vec2(-1, -1), Vec2(-0.6f, 1), Vec2(0, 0.3f), Vec2(0.6f, 1), Vec2(1, -1), }, // M
												{ Vec2(0, 1), Vec2(0, -1), Vec2(0, 0), Vec2(0, 0), Vec2(0, 0), }, // I
												{ Vec2(-1, 1), Vec2(0, 1), Vec2(1, 1), Vec2(0, 1), Vec2(0, -1), } }; // T

	for (int i = 0; i < 4; i++)
	{
		bool current = (index[i] == m_pNanoSuit->GetGameParams().GetMode());
		float size = current ? selectedSize : normalSize;
		Vec3 o = radius * (offset[i].x * wsRight + offset[i].y * wsUp);
		ColorB c = current ? color1[i] : color0[i];

		Vec3 lt = wsCenter + o + 0.5f * (size * (wsUp - wsRight));
		Vec3 rt = wsCenter + o + 0.5f * (size * (wsUp + wsRight));
		Vec3 lb = wsCenter + o + 0.5f * (size * (-wsUp - wsRight));
		Vec3 rb = wsCenter + o + 0.5f * (size * (-wsUp + wsRight));
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rt, c, rb, c);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rb, c, lb, c);

		Vec3 v[2];
		for (int j = 0; j < 4; j++)
		{
			v[0] = wsCenter + o + 0.35f * size * (letter[i][j].x * wsRight + letter[i][j].y * wsUp);
			v[1] = wsCenter + o + 0.35f * size * (letter[i][j+1].x * wsRight + letter[i][j+1].y * wsUp);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLines(v, 2, ColorB(0x10, 0x00, 0x00, current ? 0xFF : 0x80), current ? 10.0f : 7.0f);
		}
		for (int j = 0; j < 4; j++)
		{
			v[0] = wsCenter + o + 0.35f * size * (letter[i][j].x * wsRight + letter[i][j].y * wsUp);
			v[1] = wsCenter + o + 0.35f * size * (letter[i][j+1].x * wsRight + letter[i][j+1].y * wsUp);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLines(v, 2, ColorB(0xFF, 0xFF, 0xFF, current ? 0xFF : 0x80), current ? 6.0f : 3.0f);
		}
	}
}

void CHUD::PrototypeRender_HealthBar()
{
	if(!g_pGameCVars->pl_debug_suit)
		return;

	ColorF colors[] = { ColorF(1,0,0), ColorF(1,0.5f,0), ColorF(1,1,0), ColorF(0,1,0) };
	float fractions[] = { 0.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 100.0f };
	int n = sizeof(fractions) / sizeof(float);

	const float health = (m_fHealth < 0.0f) ? 0.0f : m_fHealth;

	ColorF color = colors[n/2-1];
	for (int i = 0; i < (n-1); ++i)
	{
		if (health <= fractions[i+1])
		{
			float fraction = (health - fractions[i]) / (fractions[i+1] - fractions[i]);
			color.a = LERP(colors[i/2].a, colors[(i+1)/2].a, fraction);
			color.r = LERP(colors[i/2].r, colors[(i+1)/2].r, fraction);
			color.g = LERP(colors[i/2].g, colors[(i+1)/2].g, fraction);
			color.b = LERP(colors[i/2].b, colors[(i+1)/2].b, fraction);
			break;
		}
	}
	ColorF bcolor = color/2;
	ColorF fcolor = color;

    float width = (float)gEnv->pRenderer->GetWidth();
    float height = (float)gEnv->pRenderer->GetHeight();

	Vec2 o(width * 0.1f, height * 0.91f);
	Vec2 s(width * 0.2f, height * 0.04f);

    gEnv->pRenderer->Draw2dImage(o.x, o.y, s.x - o.x, o.x - s.y, 0, 0, 0, 1, 1, 0, bcolor.r, bcolor.g, bcolor.b, bcolor.a);

	//m_pUIDraw->DrawQuadSimple(o.x, o.y, s.x, s.y, bcolor.pack_argb8888(), 0);
	if(health > 0.0f)
    {
        gEnv->pRenderer->Draw2dImage(o.x, o.y, (s.x - o.x) * health / 99.0f, o.x - s.y, 0, 0, 0, 1, 1, 0, fcolor.r, fcolor.g, fcolor.b, fcolor.a);
		//m_pUIDraw->DrawQuadSimple(o.x, o.y, s.x * health/99.0f, s.y, fcolor.pack_argb8888(), 0);
    }
}

void CHUD::PrototypeRender_EnergyMeter()
{
	if(!m_pNanoSuit)
		return;

	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(!pActor)
		return;

	CActor* pOldActor = static_cast<CActor*>(pActor->GetComponent<IActorComponent_OldActor>());
	if (!pOldActor)
		return;

	SActorStats* pActorStats = pOldActor->GetActorStats();
	assert(pActorStats);

	const SNanoSuitGameParameters& suitParams = m_pNanoSuit->GetGameParams();

	float currentEnergy = (suitParams.GetState() == eNanoSuitState_Critical) ? suitParams.GetEnergy().GetMaxEnergyCritical() : suitParams.GetEnergy().Get();
	const float maximumEnergy = suitParams.GetEnergy().GetMaxEnergy();
	const float maximumEnergyCritical = suitParams.GetEnergy().GetMaxEnergyCritical();

	//Add some fluctuation when at constant level (just faked it)
	const float currentConstantConsumption = suitParams.GetEnergy().GetConstantConsumption();
	if(currentEnergy == currentConstantConsumption)
	{
		currentEnergy = LERP(currentEnergy, CLAMP(currentEnergy + Random(-1.0f, 1.0f), currentConstantConsumption - 1.0f, currentConstantConsumption + 1.0f), 0.5f);
	}

	const float width = (float)gEnv->pRenderer->GetWidth();
	const float height = (float)gEnv->pRenderer->GetHeight();

    const float widthForQuads = 800.0f;
    const float heightForQuads = 600.0f;

	ColorF colors[5] = { ColorF(0.0f,1.0f,0.0f), ColorF(0.5f,0.5f,0.0f), ColorF(1.0f,1.0f,0.0f), ColorF(1.0f,1.0f,0.0f), ColorF(1.0f,1.0f,0.0f) };
	float fractions[5] = { 0.0f, 0.65f * maximumEnergy, 0.99f * maximumEnergy, maximumEnergy, maximumEnergyCritical + 1.0f };
	int n = 5;

	ColorF color = colors[n-1];
	if(suitParams.GetState() == eNanoSuitState_Normal)
	{
		for (int i = 0; i < (n - 1); ++i)
		{
			if (currentEnergy <= fractions[i+1])
			{
				float fraction = (currentEnergy - fractions[i]) / (fractions[i+1] - fractions[i]);
				color.a = LERP(colors[i].a, colors[i+1].a, fraction);
				color.r = LERP(colors[i].r, colors[i+1].r, fraction);
				color.g = LERP(colors[i].g, colors[i+1].g, fraction);
				color.b = LERP(colors[i].b, colors[i+1].b, fraction);
				break;
			}
		}
	}
	else
	{
		if(suitParams.GetState() != eNanoSuitState_Stressed)
		{
			color.Set(1.0f, 0.0f, 0.0f);
		}
		color.a *= m_prototypeHUDOpacity;
	}

	ColorF bcolor = color/2;
	ColorF fcolor = color;

	Vec2 origin(widthForQuads * 0.1f, heightForQuads * 0.85f);
	Vec2 size(widthForQuads * 0.2f, heightForQuads * 0.04f);
    
    
    gEnv->pRenderer->Draw2dImage(origin.x, origin.y, size.x, size.y, 0, 0, 0, 1, 1, 0, bcolor.r, bcolor.g, bcolor.b, bcolor.a);
    gEnv->pRenderer->Draw2dImage(origin.x, origin.y, size.x * min(currentEnergy/maximumEnergy, 1.0f), size.y, 0, 0, 0, 1, 1, 0, fcolor.r, fcolor.g, fcolor.b, fcolor.a);


	 //m_pUIDraw->DrawQuadSimple(origin.x, origin.y, size.x, size.y, bcolor.pack_argb8888(), 0);
	// m_pUIDraw->DrawQuadSimple(origin.x, origin.y, size.x * min(currentEnergy/maximumEnergy, 1.0f), size.y, fcolor.pack_argb8888(), 0);

	//Render energy bar
	if(g_pGameCVars->pl_debug_suit)
	{
		if(currentEnergy > maximumEnergy)
		{
			origin.set(width * 0.31f, height * 0.85f);
			size.set(width* 0.045f, height * 0.04f);

			const float criticalFraction = min((currentEnergy - maximumEnergy) / (maximumEnergyCritical - maximumEnergy),1.0f);

            gEnv->pRenderer->Draw2dImage(origin.x, origin.y, size.x, size.y, 0, 0, 0, 1, 1, 0, bcolor.r, bcolor.g, bcolor.b, bcolor.a);
            gEnv->pRenderer->Draw2dImage(origin.x, origin.y, size.x * criticalFraction, size.y, 0, 0, 0, 1, 1, 0, fcolor.r, fcolor.g, fcolor.b, fcolor.a);

			//m_pUIDraw->DrawQuadSimple(origin.x, origin.y, size.x, size.y, bcolor.pack_argb8888(), 0);
			//m_pUIDraw->DrawQuadSimple(origin.x, origin.y, size.x * criticalFraction, size.y, fcolor.pack_argb8888(), 0);
		}
	}

	//Render mini-suit menu displaying current mode
	if(!m_actualSuitMenuOpen)
	{
		int index[4] = { eNanoSuitMode_MANEUVER, eNanoSuitMode_COMBAT, eNanoSuitMode_INFILTRATION, eNanoSuitMode_TACTICAL };
		ColorF color0[4] = { ColorF(0.0f, 1.0f, 0.0f, 0.5f), ColorF(1.0f, 0.0f, 0.0f, 0.5f), ColorF(0.0f, 0.0f, 1.0f, 0.5f), ColorF(1.0f, 1.0f, 0.0f, 0.5f) };
		ColorF color1[4] = { ColorF(0.0f, 1.0f, 0.0f, 0.85f), ColorF(1.0f, 0.0f, 0.0f, 0x85), ColorF(0.0f, 0.0f, 1.0f, 0.85f), ColorF(1.0f, 1.0f, 0.0f, 0.85f)};
		
		Vec2 iconsCenter(widthForQuads*0.05f, heightForQuads * 0.82f); float iconRadius = heightForQuads * 0.03f;
		Vec2	 iconOrigin[4] = { Vec2(iconsCenter + Vec2(0.0f, iconRadius)), Vec2(iconsCenter + Vec2(iconRadius,0.0f)), Vec2(iconsCenter + Vec2(-iconRadius,0.0f)), Vec2(iconsCenter + Vec2(0.0f, -iconRadius)) }; 
		Vec2 iconSize(heightForQuads*0.017f, heightForQuads*0.017f);


		for(int i = 0; i < 4; ++i )
		{
			const Vec2 currentSize = (suitParams.GetMode() == i) ? iconSize * 1.5f : iconSize;
			ColorF currentColor = (suitParams.GetMode() == i) ? color1[i] : color0[i];
			currentColor.a *= m_prototypeHUDOpacity;
			const Vec2 currentOrigin = iconOrigin[i] - (currentSize * 0.5f);

            gEnv->pRenderer->Draw2dImage(currentOrigin.x, currentOrigin.y, currentSize.x, currentSize.y, 0, 0, 0, 1, 1, 0, currentColor.r, currentColor.g, currentColor.b, currentColor.a);

			// m_pUIDraw->DrawQuadSimple(currentOrigin.x, currentOrigin.y, currentSize.x, currentSize.y, currentColor.pack_argb8888(), 0);

		}
	}

	//Energy Usage (Numeric)
	CryFixedStringT<32> percent;
	percent.Format("%d", (int) CLAMP((currentEnergy / maximumEnergy)* 100.0f, 0.0f, 100.0f));
	m_pUIDraw->DrawTextSimple(m_pDefaultFont, width * 0.038f , height * 0.925f , 25, 35, 
		percent, fcolor, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_BOTTOM);

	//Render text displaying current mode
	if(m_pDefaultFont)
	{
		CryFixedStringT<32> suitModeText = "";
		ColorF textColor(1.0f, 1.0f, 1.0f, m_prototypeHUDOpacity);
		switch(suitParams.GetMode())
		{
			case eNanoSuitMode_MANEUVER:
				suitModeText = "Manuever Mode";
				textColor.Set(0.0f, 1.0f, 0.0f, m_prototypeHUDOpacity);
				break;
	
			case eNanoSuitMode_COMBAT:
				suitModeText = "Combat Mode";
				textColor.Set(1.0f, 0.0f, 0.0f, m_prototypeHUDOpacity);
				break;

			case eNanoSuitMode_INFILTRATION:
				suitModeText = "Infiltration Mode";
				textColor.Set(0.0f, 0.0f, 1.0f,m_prototypeHUDOpacity);
				break;

			case eNanoSuitMode_TACTICAL:
				suitModeText = "Tactical Mode";
				textColor.Set(1.0f, 1.0f, 0.0f, m_prototypeHUDOpacity);
				break;

		}

		m_pUIDraw->DrawTextSimple(m_pDefaultFont, width * 0.1f , height * 0.83f , 25, 35, 
			suitModeText.c_str(), textColor, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_BOTTOM);

		if(suitParams.GetState() == eNanoSuitState_Disabled)
		{
			//textColor.Set(1.0f, 0.0f, 0.0f);
			m_pUIDraw->DrawTextSimple(m_pDefaultFont, width * 0.31f , height * 0.9f , 25, 35, 
				"Shutdown", fcolor, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_BOTTOM);
		}
		else if(suitParams.GetState() == eNanoSuitState_Stressed)
		{
			//textColor.Set(1.0f, 0.0f, 0.0f);
			m_pUIDraw->DrawTextSimple(m_pDefaultFont, width * 0.31f , height * 0.9f , 25, 35, 
				"Stressed", fcolor, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_BOTTOM);
		}
		else if(suitParams.GetState() == eNanoSuitState_Critical)
		{
			//textColor.Set(1.0f, 0.0f, 0.0f);
			m_pUIDraw->DrawTextSimple(m_pDefaultFont, width * 0.31f , height * 0.9f , 25, 35, 
				"Critical", fcolor, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_BOTTOM);
		}
		
		if(suitParams.GetMode() == eNanoSuitMode_MANEUVER)
		{
			float velocity = pActorStats->velocity.len() * 3.6f; // In km/h

			suitModeText.Format("%2.1f km/h", velocity);
			m_pUIDraw->DrawTextSimple(m_pDefaultFont, width * 0.25f , height * 0.95f , 25, 35, 
				suitModeText.c_str(), fcolor, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_BOTTOM);
		}

		CPlayer* pPlayer = pActor->IsPlayer() ? (CPlayer*)pActor : 0;
		if (pPlayer && pPlayer->GetStance()==STANCE_CROUCH)
		{
			const float frequency = 3.0f;
			const float currentTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
			const float sawCycle = 1.0f - cry_fmod(currentTime*frequency, 1.0f);
			const float colorSeq = sawCycle * 0.9f + 0.1f;
			m_pUIDraw->DrawTextSimple(m_pDefaultFont, width * 0.02f , height * 0.75f , 25, 35, 
				"Crouch", ColorF(colorSeq,colorSeq,colorSeq), UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_BOTTOM);
		}

	}
}

void CHUD::PrototypeRender_Crosshair()
{
	float width = 800.0f;
	float height = 600.0f;

	uint32 color = 0xFFFFFFFF;

	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(!pActor)
		return;

	IActorComponent_OldActor* pOldActor = pActor->GetComponent<IActorComponent_OldActor>();
	if (!pOldActor)
		return;

	IItem* pItem = pOldActor->GetCurrentItem();
	if (!pItem)
		return;

	IWeapon* pWeapon = pItem->GetIWeapon();
	if (!pWeapon)
		return;

	IFireMode* pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
	if (!pFireMode)
		return;

	// Don't show crosshair when zooming.
	if (pWeapon->IsZoomed() || pWeapon->IsZoomingInOrOut())
		return;

	// Don't show crosshair when laser sight is attached and activated.
	if (static_cast<CWeapon*>(pWeapon)->IsLaserActivated())
		return;

	// Don't show crosshair when sprinting (only while in maneuver mode).
	if (static_cast<CPlayer*>(pActor)->IsSprinting())
		return;

	// Don't show crosshair if the player can not fire
	if (static_cast<CPlayer*>(pActor)->CanFire() == false)
		return;

/*
	if (static_cast<CWeapon*>(pWeapon)->IsWeaponLowered())
		return;

	if ((m_pNanoSuit->GetState().mode == eNSM_MANEUVER) && m_pNanoSuit->GetState().isSprinting)
		return;
*/

	float spread = pFireMode->GetSpread();
	float radius = cry_sinf(DEG2RAD(spread*g_pGameCVars->hud_crosshair_spread));

/*
	int suitmode = m_pNanoSuit->GetState().mode;
	int suitmodes[] = { eNSM_COMBAT, eNSM_MANEUVER, eNSM_INFILTRATION };
	float accuracy[] = { 1.0f, 0.5f, 0.5f };
	for (int i = 0; i < 3; ++i)
		if (suitmode == suitmodes[i])
			radius /= accuracy[i];
*/

	float screen = min(width, height);

	Vec2 center(width * 0.5f, height * 0.5f);

	Vec2 o[] = { Vec2(-1,0),Vec2(1,0),Vec2(0,-1),Vec2(0,1) };
	int n = sizeof(o)/sizeof(Vec2);
	for (int i = 0; i < n; ++i)
	{
		Vec3 offset = o[i] * radius;
        gEnv->pRenderer->Draw2dImage(center.x + offset.x * screen - 0.0f * o[i].x - 0.5f, 
            center.y + offset.y * screen - 0.0f * o[i].y - 0.5f, 
            10.0f * o[i].x + 1.0f, 
            10.0f * o[i].y + 1.0f,
            0, 0, 0, 1, 1, 0,
            1, 1, 1, 1, 0.2f);
	}

	//m_pUIDraw->DrawQuadSimple(center.x - 1, center.y - 1, 1, 1, color, 0);
}

void CHUD::PrototypeRender_SuitMenu()
{
//	float width = (float)gEnv->pRenderer->GetWidth();
//	float height = (float)gEnv->pRenderer->GetHeight();

//	int texID = m_pUIDraw->CreateTexture("textures/gui/crysis_logo.dds");
//	uint c = 0x10FFFFFF;
	//m_pUIDraw->DrawQuad(width * 0.25f, height * 0.25f, width * 0.5f, height * 0.5f, c, c, c, c, c, texID);
	//m_pUIDraw->DrawQuadSimple(0, 0, width * 0.5f, height * 0.5f, c, texID);
}

void CHUD::PrototypeRender_AmmoCounter()
{
/*
	main ammo and ammopool, lower right
	attachment name, ammo and ammopool, lower right
	firemode, lower right
*/

	float width = (float)gEnv->pRenderer->GetWidth();
	float height = (float)gEnv->pRenderer->GetHeight();

	ColorF color(1.0f, 1.0f, 1.0f, m_prototypeHUDOpacity);
	IFFont* pFont = m_pDefaultFont;

	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(!pActor)
		return;

	IActorComponent_OldActor* pOldActor = pActor->GetComponent<IActorComponent_OldActor>();
	if (!pOldActor)
		return;

	IItem* pItem = pOldActor->GetCurrentItem();
	if (!pItem)
		return;

	IWeapon* pWeapon = pItem->GetIWeapon();
	if (!pWeapon)
		return;

	IFireMode* pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
	if (!pFireMode)
		return;

	int mainClipAmmo = pFireMode->GetAmmoCount();
	int mainPoolAmmo = pFireMode->GetClipSize();

	IInventory* pInv = pOldActor->GetInventory();
	if (pInv)
	{
		IEntityClass* pAmmoType = pFireMode->GetAmmoType();
		mainPoolAmmo = pInv->GetAmmoCount(pAmmoType);
	}

	CryFixedStringT<96> buffer;

	const char* weaponName = pItem->GetEntity()->GetClass()->GetName();
	m_pUIDraw->DrawTextSimple(pFont, width * 0.9f, height * 0.85f, 25, 25, 
		weaponName, color, UIDRAWHORIZONTAL_RIGHT, UIDRAWVERTICAL_BOTTOM);

	if (pFireMode->GetClipSize() > 0)
		buffer.Format("%s Ammo %d/%d", pFireMode->GetName(), mainClipAmmo, mainPoolAmmo);
	else
		buffer.Format("%s Ammo Infinite", pFireMode->GetName(), mainClipAmmo, mainPoolAmmo);
	m_pUIDraw->DrawTextSimple(pFont, width * 0.9f, height * 0.9f, 25, 25, 
		buffer.c_str(), color, UIDRAWHORIZONTAL_RIGHT, UIDRAWVERTICAL_BOTTOM);

	IFireMode* pFireModeAttachment = pWeapon->GetFireMode(FireModeIndex_Secondary);
	if (pFireModeAttachment)
	{
		int attachmentClipAmmo = pFireModeAttachment->GetAmmoCount();
		int attachmentPoolAmmo = pFireModeAttachment->GetClipSize();

		if (pInv)
		{
			IEntityClass* pAmmoTypeAttacment = pFireModeAttachment->GetAmmoType();
			attachmentPoolAmmo = pInv->GetAmmoCount(pAmmoTypeAttacment);
		}

		if (pFireModeAttachment->GetClipSize() > 0)
			buffer.Format("%s Ammo %d/%d", pFireModeAttachment->GetName(), attachmentClipAmmo, attachmentPoolAmmo);
		else
			buffer.Format("%s Ammo Infinite", pFireModeAttachment->GetName(), attachmentClipAmmo, attachmentPoolAmmo);
		m_pUIDraw->DrawTextSimple(pFont, width * 0.9f , height * 0.85f, 25, 25, 
			buffer.c_str(), color, UIDRAWHORIZONTAL_RIGHT, UIDRAWVERTICAL_BOTTOM);
	}
}

void CHUD::PrototypeRender_WeaponSelectionMenu()
{
	float width = (float)gEnv->pRenderer->GetWidth();
	float height = (float)gEnv->pRenderer->GetHeight();

	ColorF colorSelected(1.0f, 1.0f, 1.0f);
	ColorF colorNormal(0.5f, 0.7f, 1.0f);
	IFFont* pFont = m_pDefaultFont;

	Vec2 center(width * 0.5f, height * 0.5f);

/*	const char* categories[] = { "medium", "small", "explosive", "utility" };
	Vec2 offset[] = { Vec2(0,-0.7f), Vec2(-1,0), Vec2(1,0), Vec2(0, 0.7f) };
	float count[] = { 0.0f, 0.0f, 0.0f, 0.0f };*/

	const char* categories[] = { ".", "medium", "explosive", "small" };
	Vec2 offset[] = { Vec2(0,-0.7f), Vec2(-1,0), Vec2(1,0), Vec2(0, 0.7f) };
	float count[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (pActor)
	{
		IItemSystem * pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();
		IActorComponent_OldActor* pOldActor = pActor->GetComponent<IActorComponent_OldActor>();
		if (pItemSystem && pOldActor)
		{
			IInventory* pInv = pOldActor->GetInventory();
			if (pInv)
			{
				EntityId itemIDCurrent = pInv->GetCurrentItem();
				m_weaponMenu_selectedTimer += gEnv->pTimer->GetFrameTime();

/*
				if (itemIDCurrent == m_weaponMenu_selectedItemID)
				{
					m_weaponMenu_selectedTimer += gEnv->pTimer->GetFrameTime();
				}
				else
				{
					m_weaponMenu_selectedItemID = itemIDCurrent;
					m_weaponMenu_selectedTimer = 0.0f;
				}
*/

				float fadeMinTime = 2.0f;
				float fadeMaxTime = 2.5f;
				float fadeFraction = CLAMP((m_weaponMenu_selectedTimer - fadeMinTime) / (fadeMaxTime - fadeMinTime), 0.0f, 1.0f);

				if (fadeFraction > 1.0f)
					return;

				colorNormal.a = 1.0f - fadeFraction;
				colorSelected.a = 1.0f - fadeFraction;

				int itemCount = pInv->GetCount();
				for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex)
				{
					EntityId itemID = pInv->GetItem(itemIndex);
					IItem* pItem = pItemSystem->GetItem(itemID);
					if(pItem == NULL)
						continue;

					const char* itemNameSZ = pItem->GetEntity()->GetClass()->GetName();
					const char* itemCategorySZ = pItemSystem->GetItemCategory(itemNameSZ);

					for (int i = 0; i < 4; ++i)
					{
						if ((!pItem->IsDualWieldSlave()) && (strcmp(itemCategorySZ, categories[i]) == 0))
						{
							Vec2 p = center + offset[i] * (count[i] * 140.0f + 120.0f);
							//m_pUIDraw->DrawText(pFont, width * 0.5f, height * 0.5f, width * 0.2f, height * 0.2f, 
							if (itemIDCurrent == itemID)
								m_pUIDraw->DrawTextSimple(pFont, p.x, p.y, 25, 35, 
								itemNameSZ, colorSelected, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER);
							else
								m_pUIDraw->DrawTextSimple(pFont, p.x, p.y, 20, 30, 
								itemNameSZ, colorNormal, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER);

							if (pItem->IsDualWieldMaster())
								if (itemIDCurrent == itemID)
									m_pUIDraw->DrawTextSimple(pFont, p.x, p.y - 20, 25, 35, 
										"DUAL", colorSelected, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER);
							else
									m_pUIDraw->DrawTextSimple(pFont, p.x, p.y - 20, 20, 30, 
										"DUAL", colorNormal, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER);

							count[i]++;
						}
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::PrototypeRender_EnergyMeter(Vec3 wsCenter, Vec3 wsUp, Vec3 wsRight, float wsSize)
{
	if (!m_pNanoSuit)
		return;

	if (g_pGameCVars->hud_prototype_energymeter == 0)
		return;


	float curEnergy = m_pNanoSuit->GetGameParams().GetEnergy().Get();
	float maxEnergy = m_pNanoSuit->GetGameParams().GetEnergy().GetMaxEnergy();
	float blobEnergy = maxEnergy / (float)g_pGameCVars->hud_prototype_energymeter_blobs;
	float curBlobCount = curEnergy / blobEnergy;
	float maxBlobCount = cry_floorf(maxEnergy / blobEnergy);

	ColorB fullColor_hi(0x70, 0xB0, 0xF0, 0xF0);
	ColorB fillColor_hi(0x50, 0x90, 0xF0, 0xF0);
	ColorB backColor_hi(0x30, 0x50, 0xA0, 0x40);
	ColorB fullColor_lo(0xFF, 0xA0, 0x80, 0xF0);
	ColorB fillColor_lo(0xFF, 0x90, 0x50, 0xF0);
	ColorB backColor_lo(0xFF, 0x50, 0x30, 0x40);
	ColorB borderColor(0xFF, 0xFF, 0x00, 0xF0);

	ColorB fullColor, fillColor, backColor;
	if ((curEnergy/maxEnergy) < g_pGameCVars->hud_prototype_energymeter_critical)
	{
		fullColor = fullColor_lo;
		fillColor = fillColor_lo;
		backColor = backColor_lo;
	}
	else
	{
		fullColor = fullColor_hi;
		fillColor = fillColor_hi;
		backColor = backColor_hi;
	}

	Vec3 wsOffset(ZERO);
	wsOffset += wsRight * g_pGameCVars->hud_prototype_energymeter_offsetx;
	wsOffset += wsUp * g_pGameCVars->hud_prototype_energymeter_offsety;
	wsOffset += wsUp * -0.7f;
	wsUp.Normalize();
	wsRight.Normalize();
	wsRight *= g_pGameCVars->hud_prototype_energymeter_scalex;
	wsUp *= g_pGameCVars->hud_prototype_energymeter_scaley;

	if (g_pGameCVars->hud_prototype_energymeter == 1)
	{
		float blobWidth = wsSize * 0.06f;
		float blobHeight = wsSize * 0.04f;
		float blobSpacing = wsSize * 0.02f;

		float totalWidth = blobWidth * maxBlobCount + blobSpacing * (maxBlobCount - 1);

		for (float i = 0; i < maxBlobCount; i++)
		{
			Vec3 halfheight = wsUp * blobHeight * 0.5f;
			Vec3 leftSide = wsCenter+wsOffset + (-(totalWidth / 2.0f) + ((blobWidth + blobSpacing) * i)) * wsRight;
			Vec3 rightSide = wsCenter+wsOffset + (-(totalWidth / 2.0f) + ((blobWidth + blobSpacing) * i) + blobWidth) * wsRight;
			ColorB c = backColor;

			Vec3 lt = leftSide - halfheight;
			Vec3 rt = rightSide - halfheight;
			Vec3 lb = leftSide + halfheight;
			Vec3 rb = rightSide + halfheight;
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rt, c, rb, c);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rb, c, lb, c);
		}

		for (float i = 0; i < curBlobCount; i++)
		{
			float blobFraction = min(1.0f, curBlobCount - i);
			Vec3 halfheight = wsUp * blobHeight * 0.5f;
			Vec3 leftSide = wsCenter+wsOffset + (-(totalWidth / 2.0f) + ((blobWidth + blobSpacing) * i)) * wsRight;
			Vec3 rightSide = wsCenter+wsOffset + (-(totalWidth / 2.0f) + ((blobWidth + blobSpacing) * i) + (blobWidth * blobFraction)) * wsRight;
			ColorB c = (blobFraction == 1.0f) ? fullColor : fillColor;

			Vec3 lt = leftSide - halfheight;
			Vec3 rt = rightSide - halfheight;
			Vec3 lb = leftSide + halfheight;
			Vec3 rb = rightSide + halfheight;
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rt, c, rb, c);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rb, c, lb, c);
		}

		if(curEnergy > 0.0f)
		{
			int fullBlobs = (int)(floor_tpl(curBlobCount));
			for (int i = 0; i<=fullBlobs; i++)
			{
				if( (fullBlobs != 0) && (fullBlobs == i))
					break;

				Vec3 halfheight = wsUp * blobHeight * 0.5f;
				Vec3 leftSide = wsCenter+wsOffset + (-(totalWidth / 2.0f) + ((blobWidth + blobSpacing) * i)) * wsRight;
				Vec3 rightSide = wsCenter+wsOffset + (-(totalWidth / 2.0f) + ((blobWidth + blobSpacing) * i) + blobWidth) * wsRight;
				ColorB c = (fullBlobs!=0)?borderColor:ColorB(255,0,0,255);

				Vec3 lt = leftSide - halfheight;
				Vec3 rt = rightSide - halfheight;
				Vec3 lb = leftSide + halfheight;
				Vec3 rb = rightSide + halfheight;

				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(lt, c, rt, c, blobSpacing);
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(lb, c, rb, c, blobSpacing);
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(lt, c, lb, c, blobSpacing);
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(rt, c, rb, c, blobSpacing);
			}
		}

		//Super charge time bar
		{
			Vec3 barCenter = wsCenter - (wsUp*1.5f*blobHeight);
			Vec3 halfheight = wsUp * blobHeight * 0.5f;
			Vec3 leftSide = barCenter+wsOffset + (-(totalWidth / 2.0f)) * wsRight;
			Vec3 rightSide = barCenter+wsOffset + (-(totalWidth / 2.0f) + ((blobWidth + blobSpacing) * maxBlobCount)-blobSpacing) * wsRight;
			ColorB c(0xFF, 0xFF, 0xFF, 0x40);

			Vec3 lt = leftSide - halfheight;
			Vec3 rt = rightSide - halfheight;
			Vec3 lb = leftSide + halfheight;
			Vec3 rb = rightSide + halfheight;
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rt, c, rb, c);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rb, c, lb, c);

		}

	}

	if (g_pGameCVars->hud_prototype_energymeter == 2)
	{
		float blobWidth = wsSize * 0.04f;
		float blobHeight = wsSize * 0.03f;
		float blobSpacing = wsSize * 0.01f;

		float fullBlobCount = cry_floorf(curBlobCount);
		float blobFraction = curBlobCount - fullBlobCount;

		float totalWidth = blobWidth * maxBlobCount + blobSpacing * (maxBlobCount - 1);
		for (float i = 0; i < maxBlobCount; i++)
		{
			Vec3 halfheight = wsUp * blobHeight * 0.5f;
			Vec3 leftSide = wsCenter+wsOffset + (-(totalWidth / 2.0f) + ((blobWidth + blobSpacing) * i)) * wsRight;
			Vec3 rightSide = wsCenter+wsOffset + (-(totalWidth / 2.0f) + ((blobWidth + blobSpacing) * i) + blobWidth) * wsRight;
			ColorB c = (i < fullBlobCount) ? fullColor : backColor;

			Vec3 lt = leftSide - halfheight;
			Vec3 rt = rightSide - halfheight;
			Vec3 lb = leftSide + halfheight;
			Vec3 rb = rightSide + halfheight;
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rt, c, rb, c);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rb, c, lb, c);
		}

		Vec3 halfheight = wsUp * blobHeight * 0.5f;
		Vec3 leftSide = wsCenter+wsOffset + (-(totalWidth / 2.0f)) * wsRight;
		Vec3 rightSide = wsCenter+wsOffset + (-(totalWidth / 2.0f) + (totalWidth * blobFraction)) * wsRight;
		ColorB c = fillColor;

		Vec3 lt = leftSide - halfheight + (halfheight*2.0f+blobSpacing*wsUp);
		Vec3 rt = rightSide - halfheight + (halfheight*2.0f+blobSpacing*wsUp);
		Vec3 lb = leftSide + halfheight + (halfheight*2.0f+blobSpacing*wsUp);
		Vec3 rb = rightSide + halfheight + (halfheight*2.0f+blobSpacing*wsUp);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rt, c, rb, c);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rb, c, lb, c);

		Vec3 rightEdge = wsCenter+wsOffset + (totalWidth / 2.0f) * wsRight;
		lt = rt;
		lb = rb;
		rt = rightEdge - halfheight + (halfheight*2.0f+blobSpacing*wsUp);
		rb = rightEdge + halfheight + (halfheight*2.0f+blobSpacing*wsUp);
		c = backColor;
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rt, c, rb, c);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rb, c, lb, c);
	}

	if (g_pGameCVars->hud_prototype_energymeter == 3)
	{
		float fullBlobCount = cry_floorf(curBlobCount);
		float blobFraction = curBlobCount - fullBlobCount;

		float radiusInner = wsSize * 0.12f;
		float radiusOuter = wsSize * 0.15f;

		float activeAngleFraction = 0.7f;
		float activeSegments = 30.0f;

		float activeAngleStart = 0.5f*(float)g_PI + (float)g_PI * activeAngleFraction;
		float activeAngleEnd = 0.5f*(float)g_PI - (float)g_PI * activeAngleFraction;
		float activeSegmentAngleSpan = (activeAngleEnd - activeAngleStart) / activeSegments;

		float fullAngleStart = activeAngleStart;
		float fullAngleEnd = activeAngleEnd + 2.0f*(float)g_PI;
		float fullAngleSpacingFraction = 0.5f;
		float fullTotalAngleSpan = fullAngleEnd - fullAngleStart;
		float fullBlobAngleSpan = fullTotalAngleSpan / (maxBlobCount + (maxBlobCount+1)*fullAngleSpacingFraction);
		float fullBlobAngleSpacing = (fullTotalAngleSpan - (fullBlobAngleSpan * maxBlobCount)) / (maxBlobCount+1);

		for (float i = 0.0f; i < activeSegments; i++)
		{
			float fraction = i / activeSegments;
			float angleOffset = LERP(activeAngleStart, activeAngleEnd, 1.0f-fraction);

			float angle0 = angleOffset;
			float angle1 = angleOffset+activeSegmentAngleSpan*1.02f;
			Vec3 i0 = wsCenter+wsOffset + radiusInner*(wsRight*cry_cosf(angle0) + wsUp*cry_sinf(angle0));
			Vec3 i1 = wsCenter+wsOffset + radiusInner*(wsRight*cry_cosf(angle1) + wsUp*cry_sinf(angle1));
			Vec3 o0 = wsCenter+wsOffset + radiusOuter*(wsRight*cry_cosf(angle0) + wsUp*cry_sinf(angle0));
			Vec3 o1 = wsCenter+wsOffset + radiusOuter*(wsRight*cry_cosf(angle1) + wsUp*cry_sinf(angle1));
			ColorB c = (fraction < blobFraction) ? fillColor : backColor;
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(i0, c, i1, c, o1, c);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(i0, c, o1, c, o0, c);
		}

		for (float i = 0.0f; i < maxBlobCount; i++)
		{
			float angleOffset = fullAngleStart + fullBlobAngleSpacing + i * (fullBlobAngleSpan + fullBlobAngleSpacing);

			float angle0 = angleOffset;
			float angle1 = angleOffset+fullBlobAngleSpan;
			Vec3 i0 = wsCenter+wsOffset + radiusInner*(wsRight*cry_cosf(angle0) + wsUp*cry_sinf(angle0));
			Vec3 i1 = wsCenter+wsOffset + radiusInner*(wsRight*cry_cosf(angle1) + wsUp*cry_sinf(angle1));
			Vec3 o0 = wsCenter+wsOffset + radiusOuter*(wsRight*cry_cosf(angle0) + wsUp*cry_sinf(angle0));
			Vec3 o1 = wsCenter+wsOffset + radiusOuter*(wsRight*cry_cosf(angle1) + wsUp*cry_sinf(angle1));
			ColorB c = (i < fullBlobCount) ? fullColor : backColor;
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(i0, c, i1, c, o1, c);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(i0, c, o1, c, o0, c);
		}
	}
}

//-------------------------------------------------------------------
bool CHUD::Prototype_CanEnterWeaponCustomMenu()
{
	CWeapon* pWeapon = GetCurrentWeapon();
	if (!pWeapon)
		return false;

	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	CPlayer *pPlayer = pActor? static_cast<CPlayer*>(pActor->GetComponent<IActorComponent_OldActor>()): NULL;

//	COffHand *pOffhand = pPlayer ? static_cast<COffHand*>(pPlayer->GetItemByClass(CItem::sOffHandClass)) : NULL;
//	if(pOffhand && pOffhand->IsSelected())
//		return false;

	bool busy = pWeapon->IsBusy() || pWeapon->IsModifying() || pWeapon->IsZoomingInOrOut() || pWeapon->IsReloading() || pWeapon->IsTargetOn();

	return !busy;
}

//-------------------------------------------------------------------
void CHUD::Prototype_OnEnterWeaponCustomMenu(int menuId)
{
	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (!pActor)
		return;

	IPlayerInput *pPlayerInput = pActor->GetComponent<IActorComponent_OldActor>()? static_cast<CPlayer*>(pActor->GetComponent<IActorComponent_OldActor>())->GetPlayerInput(): NULL;
	CWeapon * pWeapon = GetCurrentWeapon();

	if(!pWeapon || !pPlayerInput)
		return;

	if(menuId == 1)
	{
		//if (pPlayerInput)
			//pPlayerInput->DisableXI(true);

		g_pGame->GetIGameFramework()->GetIActionMapManager()->EnableActionMap("crysis2_weapon_menu_opened_1", true);
		g_pGameActions->FilterWeaponMenu()->Enable(true);
		m_pHUDCrosshair->SetOpacity(0.0f);

		//m_weaponModInfo.Reset();
	}
	else if(menuId == 2)
	{
		pWeapon->OnAction(g_pGame->GetIGameFramework()->GetClientActorId(),"modify",0,1);

		const char* action1 = "crysis2_weapon_menu_opened_2";
		const char* action2 = "crysis2_weapon_menu_opened_3";
		if (g_pGame->GetIGameFramework()->GetIActionMapManager()->GetActionMap(action1))
		{
			g_pGame->GetIGameFramework()->GetIActionMapManager()->EnableActionMap(action1, true);
		}
		else if (g_pGame->GetIGameFramework()->GetIActionMapManager()->GetActionMap(action2))
		{
			g_pGame->GetIGameFramework()->GetIActionMapManager()->EnableActionMap(action2, true);
		}
		g_pGameActions->FilterWeaponModScreen()->Enable(true);
	}
}

//-------------------------------------------------------------------
void CHUD::Prototype_OnExitWeaponCustomMenu(int menuId)
{
	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (!pActor)
		return;

	IPlayerInput *pPlayerInput = pActor->GetComponent<IActorComponent_OldActor>()? static_cast<CPlayer*>(pActor->GetComponent<IActorComponent_OldActor>())->GetPlayerInput(): NULL;
	CWeapon * pWeapon = GetCurrentWeapon();

	if(!pWeapon || !pPlayerInput)
		return;

	if(menuId == 0)
	{
		//if (pPlayerInput)
			//pPlayerInput->DisableXI(false);

		g_pGame->GetIGameFramework()->GetIActionMapManager()->EnableActionMap("crysis2_weapon_menu_opened_1", false);
		g_pGame->GetIGameFramework()->GetIActionMapManager()->EnableActionMap("crysis2_weapon_menu_opened_2", false);
		g_pGameActions->FilterWeaponMenu()->Enable(false);
		g_pGameActions->FilterWeaponModScreen()->Enable(false);

		m_pHUDCrosshair->SetOpacity(1.0f);

		m_weaponModInfo.Reset();
	}
	else if(menuId == 2)
	{
		pWeapon->OnAction(g_pGame->GetIGameFramework()->GetClientActorId(),"modify",0,1);
	}

}

//---------------------------------------------------------------------
void CHUD::Prototype_ShowWeaponCustomMenu_1(bool show)
{
	if (show)
	{
		Prototype_OnEnterWeaponCustomMenu(1);
		m_prototypeWeaponMenuState = 1;
	}
	else
	{
		Prototype_OnExitWeaponCustomMenu(1);
		m_prototypeWeaponMenuState = 0;
	}
}

//-----------------------------------------------------------------------------------------------------
void CHUD::Prototype_ShowWeaponCustomMenu_2(bool show)
{
	if(show)
	{
		if(m_prototypeWeaponMenuState == 1)
		{
			Prototype_ShowWeaponCustomMenu_1(false);
		}
		Prototype_OnEnterWeaponCustomMenu(2);
		m_prototypeWeaponMenuState = 2;
	}
	else
	{
		Prototype_OnExitWeaponCustomMenu(2);
		m_prototypeWeaponMenuState = 0;
	}
}
//--------------------------------------------------------------------------------
void CHUD::Prototype_UpdateWeaponCustomMenu_1()
{
	CWeapon* pCurrentWeapon = GetCurrentWeapon();
	if(!pCurrentWeapon)
		return;

	const float magicOffsetX = 122.0f;
	const float magicOffestY = magicOffsetX * 0.25f;

	SDrawTextInfo textInfo, textInfo2;
	CryFixedStringT<2> text;
	Vec3 basePos, basePosSS;

	IRenderAuxGeom* pRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags oldFlags = pRenderer->GetRenderFlags();
	SAuxGeomRenderFlags newFlags = e_Def3DPublicRenderflags;
	newFlags.SetAlphaBlendMode(e_AlphaBlended);
	newFlags.SetDepthTestFlag(e_DepthTestOff);
	newFlags.SetCullMode(e_CullModeNone);

	pRenderer->SetRenderFlags(newFlags);
	
	//Display available firemodes...
	if( m_weaponModInfo.fireModes.size() > 1)
	{
		text = "B";
		textInfo.color[0] = 1.0f; textInfo.color[1] = 0.0f; textInfo.color[2] = 0.0f; textInfo.color[3] = 0.75f;
		textInfo.xscale = textInfo.yscale = 3.0f;
		basePos = pCurrentWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, m_weaponModInfo.fireMode_Bone.c_str() , true );

		m_pRenderer->ProjectToScreen(basePos.x,basePos.y,basePos.z,&basePosSS.x,&basePosSS.y,&basePosSS.z);
		basePosSS.x = ((basePosSS.x / 100.0f) * m_pRenderer->GetWidth()) - magicOffsetX;
		basePosSS.y = ((basePosSS.y / 100.0f) * m_pRenderer->GetHeight()) - magicOffestY;

		gEnv->pRenderer->Draw2dLabel(basePosSS.x, basePosSS.y, textInfo.yscale, textInfo.color, true, text.c_str());

		textInfo2.color[0] = 0.8f; textInfo2.color[1] = 0.8f; textInfo2.color[2] = 0.8f; textInfo2.color[3] = 0.5f;
		textInfo2.xscale = textInfo2.yscale = 1.5f;
		textInfo.xscale = textInfo.yscale = 1.5f;
		for(int i = 0; i < m_weaponModInfo.fireModes.size(); i++)
		{
			if(m_weaponModInfo.currentFireMode == i)
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo.yscale, textInfo.color, true, m_weaponModInfo.fireModes[i].c_str());
			else
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo2.yscale, textInfo2.color, true, m_weaponModInfo.fireModes[i].c_str());

		}

	}

	//Display available magazines...
	if( m_weaponModInfo.magazines.size() > 1)
	{
		text = "X";
		textInfo.color[0] = 0.0f; textInfo.color[1] = 0.0f; textInfo.color[2] = 1.0f; textInfo.color[3] = 0.85f;
		textInfo.xscale = textInfo.yscale = 3.0f;
		basePos = pCurrentWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, "magazine" , true );

		m_pRenderer->ProjectToScreen(basePos.x,basePos.y,basePos.z,&basePosSS.x,&basePosSS.y,&basePosSS.z);
		basePosSS.x = ((basePosSS.x / 100.0f) * m_pRenderer->GetWidth()) - (magicOffsetX * 1.5f);
		basePosSS.y = ((basePosSS.y / 100.0f) * m_pRenderer->GetHeight());

		gEnv->pRenderer->Draw2dLabel(basePosSS.x, basePosSS.y, textInfo.yscale, textInfo.color, true, text.c_str() );

		textInfo2.color[0] = 0.8f; textInfo2.color[1] = 0.8f; textInfo2.color[2] = 0.8f; textInfo2.color[3] = 0.5f;
		textInfo2.xscale = textInfo2.yscale = 1.5f;
		textInfo.xscale = textInfo.yscale = 1.5f;
		for(int i = 0; i < m_weaponModInfo.magazines.size(); i++)
		{
			if(m_weaponModInfo.currentMagazine == i)
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo.yscale, textInfo.color, true, m_weaponModInfo.magazines[i].c_str());
			else
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo2.yscale, textInfo2.color, true, m_weaponModInfo.magazines[i].c_str());

		}
	}

	// Blinking Y button to access accessory menu
	if( m_weaponModInfo.hasSomeAccessory )
	{
		static float alpha = 0.25f;
		static float alphaDelta = 0.5f;

		text = "Y";
		textInfo.color[0] = 1.0f; textInfo.color[1] = 1.0f; textInfo.color[2] = 0.0f; textInfo.color[3] = alpha;
		textInfo.xscale = textInfo.yscale = 3.5f;

		basePos = pCurrentWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, "weapon_term" , true );

		m_pRenderer->ProjectToScreen(basePos.x,basePos.y,basePos.z,&basePosSS.x,&basePosSS.y,&basePosSS.z);
		basePosSS.x = ((basePosSS.x / 100.0f) * m_pRenderer->GetWidth()) - (magicOffsetX * 0.8f);
		basePosSS.y = ((basePosSS.y / 100.0f) * m_pRenderer->GetHeight()) - magicOffestY;

		gEnv->pRenderer->Draw2dLabel(basePosSS.x, basePosSS.y, textInfo.yscale, textInfo.color, true, "Y");

		if(alpha >= 1.0f)
			alphaDelta = -0.5f;
		else if(alpha <= 0.25f)
			alphaDelta = 0.5f;

		alpha += alphaDelta * gEnv->pTimer->GetFrameTime();

		alpha = CLAMP(alpha, 0.25f, 1.0f);
	}

	pRenderer->SetRenderFlags(oldFlags);
}

//--------------------------------------------------------------------------------
void CHUD::Prototype_UpdateWeaponCustomMenu_2()
{
	CWeapon* pCurrentWeapon = GetCurrentWeapon();
	if(!pCurrentWeapon)
		return;

	//Don't show menu right away
	float deltaTime = gEnv->pTimer->GetAsyncCurTime() - m_weaponModInfo.weaponModOpenTime;
	if(deltaTime < 0.2f)
		return;

	float fadeInFactor = CLAMP(deltaTime*1.35f, 0.0f, 1.0f);
	
	SDrawTextInfo textInfo, textInfo2;
	CryFixedStringT<2> text;
	Vec3 basePos, basePosSS;

	IRenderAuxGeom* pRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags oldFlags = pRenderer->GetRenderFlags();
	SAuxGeomRenderFlags newFlags = e_Def3DPublicRenderflags;
	newFlags.SetAlphaBlendMode(e_AlphaBlended);
	newFlags.SetDepthTestFlag(e_DepthTestOff);
	newFlags.SetCullMode(e_CullModeNone);

	pRenderer->SetRenderFlags(newFlags);

	const bool usesABYXButtons = 
		(g_pGame->GetIGameFramework()->GetIActionMapManager()->GetActionMap("crysis2_weapon_menu_opened_2") == 0);

	//Top attachments
	if( m_weaponModInfo.topAccessories.size() > 1)
	{
		text = usesABYXButtons ? "Y" : ">";
		if (!usesABYXButtons)
		{
			textInfo.color[0] = 1.0f; textInfo.color[1] = 1.0f; textInfo.color[2] = 1.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		}
		else
		{
			textInfo.color[0] = 1.0f; textInfo.color[1] = 1.0f; textInfo.color[2] = 0.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		}
		//text = "B";
		//textInfo.color[0] = 1.0f; textInfo.color[1] = 0.0f; textInfo.color[2] = 0.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		textInfo.xscale = textInfo.yscale = 3.0f;
		basePos = pCurrentWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, m_weaponModInfo.topBone.c_str() , true );

		m_pRenderer->ProjectToScreen(basePos.x,basePos.y,basePos.z,&basePosSS.x,&basePosSS.y,&basePosSS.z);
		basePosSS.x = ((basePosSS.x / 100.0f) * m_pRenderer->GetWidth());
		basePosSS.y = ((basePosSS.y / 100.0f) * m_pRenderer->GetHeight());
		AdjustWeaponAccessory(pCurrentWeapon->GetEntity()->GetClass()->GetName(),m_weaponModInfo.topBone.c_str(),&basePosSS);

		gEnv->pRenderer->Draw2dLabel(basePosSS.x-60.0f, basePosSS.y, textInfo.yscale, textInfo.color, true, text.c_str());

		textInfo2.color[0] = textInfo.color[0] * 0.8f; textInfo2.color[1] = textInfo.color[1] * 0.8f; textInfo2.color[2] = textInfo.color[2] * 0.8f; textInfo2.color[3] = textInfo.color[3] * 0.6f;
		textInfo2.xscale = textInfo2.yscale = 1.5f;
		textInfo.xscale = textInfo.yscale = 1.5f;
		for(int i = 0; i < m_weaponModInfo.topAccessories.size(); i++)
		{
			if(m_weaponModInfo.currentTopAccessory == i)
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo.yscale, textInfo.color, true, m_weaponModInfo.topAccessories[i].c_str());
			else
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo2.yscale, textInfo2.color, true, m_weaponModInfo.topAccessories[i].c_str());

		}
	}

	//Side attachments
	if( m_weaponModInfo.sideAccessories.size() > 1)
	{
		text = usesABYXButtons ? "B" : "V";
		if (!usesABYXButtons)
		{
			textInfo.color[0] = 1.0f; textInfo.color[1] = 1.0f; textInfo.color[2] = 1.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		}
		else
		{
			textInfo.color[0] = 1.0f; textInfo.color[1] = 0.0f; textInfo.color[2] = 0.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		}
		//text = "Y";
		//textInfo.color[0] = 1.0f; textInfo.color[1] = 1.0f; textInfo.color[2] = 0.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		textInfo.xscale = textInfo.yscale = 3.0f;
		basePos = pCurrentWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, m_weaponModInfo.sideBone.c_str() , true );

		m_pRenderer->ProjectToScreen(basePos.x,basePos.y,basePos.z,&basePosSS.x,&basePosSS.y,&basePosSS.z);
		basePosSS.x = ((basePosSS.x / 100.0f) * m_pRenderer->GetWidth());
		basePosSS.y = ((basePosSS.y / 100.0f) * m_pRenderer->GetHeight());
		AdjustWeaponAccessory(pCurrentWeapon->GetEntity()->GetClass()->GetName(),m_weaponModInfo.sideBone.c_str(),&basePosSS);

		gEnv->pRenderer->Draw2dLabel(basePosSS.x-60.0f, basePosSS.y, textInfo.yscale, textInfo.color, true, text.c_str());

		textInfo2.color[0] = textInfo.color[0] * 0.8f; textInfo2.color[1] = textInfo.color[1] * 0.8f; textInfo2.color[2] = textInfo.color[2] * 0.8f; textInfo2.color[3] = textInfo.color[3] * 0.6f;
		textInfo2.xscale = textInfo2.yscale = 1.5f;
		textInfo.xscale = textInfo.yscale = 1.5f;
		for(int i = 0; i < m_weaponModInfo.sideAccessories.size(); i++)
		{
			if(m_weaponModInfo.currentSideAccessory == i)
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo.yscale, textInfo.color, true, m_weaponModInfo.sideAccessories[i].c_str());
			else
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo2.yscale, textInfo2.color, true, m_weaponModInfo.sideAccessories[i].c_str());

		}
	}

	//Barrel attachments
	if( m_weaponModInfo.barrelAccessories.size() > 1)
	{
		text = usesABYXButtons ? "A" : "^";
		if (!usesABYXButtons)
		{
			textInfo.color[0] = 1.0f; textInfo.color[1] = 1.0f; textInfo.color[2] = 1.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		}
		else
		{
			textInfo.color[0] = 0.0f; textInfo.color[1] = 1.0f; textInfo.color[2] = 0.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		}
		//text = "A";
		//textInfo.color[0] = 0.0f; textInfo.color[1] = 1.0f; textInfo.color[2] = 0.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		textInfo.xscale = textInfo.yscale = 3.0f;
		basePos = pCurrentWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, m_weaponModInfo.barrelBone.c_str() , true );

		m_pRenderer->ProjectToScreen(basePos.x,basePos.y,basePos.z,&basePosSS.x,&basePosSS.y,&basePosSS.z);
		basePosSS.x = ((basePosSS.x / 100.0f) * m_pRenderer->GetWidth());
		basePosSS.y = ((basePosSS.y / 100.0f) * m_pRenderer->GetHeight());
		AdjustWeaponAccessory(pCurrentWeapon->GetEntity()->GetClass()->GetName(),m_weaponModInfo.barrelBone.c_str(),&basePosSS);

		gEnv->pRenderer->Draw2dLabel(basePosSS.x-60.0f, basePosSS.y, textInfo.yscale, textInfo.color, true, text.c_str());

		textInfo2.color[0] = textInfo.color[0] * 0.8f; textInfo2.color[1] = textInfo.color[1] * 0.8f; textInfo2.color[2] = textInfo.color[2] * 0.8f; textInfo2.color[3] = textInfo.color[3] * 0.6f;
		textInfo2.xscale = textInfo2.yscale = 1.5f;
		textInfo.xscale = textInfo.yscale = 1.5f;
		for(int i = 0; i < m_weaponModInfo.barrelAccessories.size(); i++)
		{
			if(m_weaponModInfo.currentBarrelAccessory == i)
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo.yscale, textInfo.color, true, m_weaponModInfo.barrelAccessories[i].c_str());
			else
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo2.yscale, textInfo2.color, true, m_weaponModInfo.barrelAccessories[i].c_str());

		}
	}

	//Front Attachments
	if( m_weaponModInfo.frontAccessories.size() > 1)
	{
		text = usesABYXButtons ? "X" : "<";
		if (!usesABYXButtons)
		{
			textInfo.color[0] = 1.0f; textInfo.color[1] = 1.0f; textInfo.color[2] = 1.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		}
		else
		{
			textInfo.color[0] = 0.0f; textInfo.color[1] = 0.0f; textInfo.color[2] = 1.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		}
		//text = "X";
		//textInfo.color[0] = 0.0f; textInfo.color[1] = 0.0f; textInfo.color[2] = 1.0f; textInfo.color[3] = 0.75f * fadeInFactor;
		textInfo.xscale = textInfo.yscale = 3.5f;
		basePos = pCurrentWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, m_weaponModInfo.frontBone.c_str() , true );

		m_pRenderer->ProjectToScreen(basePos.x,basePos.y,basePos.z,&basePosSS.x,&basePosSS.y,&basePosSS.z);
		basePosSS.x = ((basePosSS.x / 100.0f) * m_pRenderer->GetWidth());
		basePosSS.y = ((basePosSS.y / 100.0f) * m_pRenderer->GetHeight());
		AdjustWeaponAccessory(pCurrentWeapon->GetEntity()->GetClass()->GetName(),m_weaponModInfo.frontBone.c_str(),&basePosSS);

		gEnv->pRenderer->Draw2dLabel(basePosSS.x-60.0f, basePosSS.y, textInfo.yscale, textInfo.color, true, text.c_str());

		textInfo2.color[0] = textInfo.color[0] * 0.8f; textInfo2.color[1] = textInfo.color[1] * 0.8f; textInfo2.color[2] = textInfo.color[2] * 0.8f; textInfo2.color[3] = textInfo.color[3] * 0.6f;
		textInfo2.xscale = textInfo2.yscale = 1.5f;
		textInfo.xscale = textInfo.yscale = 1.5f;
		for(int i = 0; i < m_weaponModInfo.frontAccessories.size(); i++)
		{
			if(m_weaponModInfo.currentFrontAccessory == i)
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo.yscale, textInfo.color, true, m_weaponModInfo.frontAccessories[i].c_str());
			else
				gEnv->pRenderer->Draw2dLabel(basePosSS.x + 20.0f, basePosSS.y - 10.0f + (15.0f * i), textInfo2.yscale, textInfo2.color, true, m_weaponModInfo.frontAccessories[i].c_str());

		}
	}

	pRenderer->SetRenderFlags(oldFlags);
}

//---------------------------------------------------------------------------------
void CHUD::Prototype_UpdateWeaponCustomMenu_1_v2(Vec3 wsCenter, Vec3 wsUp, Vec3 wsRight, float wsSize)
{
	//if (m_prototypeWeaponMenuState != 1)
		return;

	float radius = 0.1f * wsSize;

	float xScale = 800.0f / m_pRenderer->GetWidth();
	float yScale = 600.0f / m_pRenderer->GetHeight();

	wsUp.Normalize();
	wsRight.Normalize();

	ColorB color0[4] = { ColorB(0xFF, 0, 0, 0x20), ColorB(0, 0xFF, 0, 0x20), ColorB(0, 0, 0xFF, 0x20), ColorB(0xFF, 0xFF, 0, 0x20) };
	ColorB color1[4] = { ColorB(0xFF, 0, 0, 0xC0), ColorB(0, 0xFF, 0, 0xC0), ColorB(0, 0, 0xFF, 0xC0), ColorB(0xFF, 0xFF, 0, 0xC0) };
	Vec2 offset[4] = { Vec2(1, 0), Vec2(0, -1), Vec2(-1, 0), Vec2(0, 1) };

	SDrawTextInfo textInfo;

	CryFixedStringT<32> text;
	Vec2 textPos; 
	textPos.x = 400.0f; textPos.y = 300.0f;
	for (int i = 0; i < 4; i++)
	{
		bool available = false;
		float color3[4] = {1.0f,0.0f,0.0f,1.0f};

		switch(i)
		{
			case 0:
				available = (m_weaponModInfo.fireModes.size() > 1);
				text = "Fire mode ("; text.append(m_weaponModInfo.fireModes[m_weaponModInfo.currentFireMode]); text.append(")");
				textPos.x = textPos.y = 50.0f;
				textPos.x = 450.0f; textPos.y = 290.0f;
				if(m_weaponModInfo.isGrenades)
				{
					text = m_weaponModInfo.fireModes[i].c_str();
					available = (i == m_weaponModInfo.currentGrenade);
				}
				break;

			case 1:
				available = (m_weaponModInfo.magazines.size() > 1);
				text = "Ammo type";
				textPos.x = 365.0f; textPos.y = 345.0f;
				if(m_weaponModInfo.isGrenades)
				{
					textPos.x = 375.0f;
					text = m_weaponModInfo.fireModes[i].c_str();
					available = (i == m_weaponModInfo.currentGrenade);
				}
				break;

			case 2:
				available = false;
				text = "";
				if(m_weaponModInfo.hasFlashlight)
				{
					available = true;
					text = "Flashlight";
					textPos.x = 260.0f; textPos.y = 290.0f;
				}
				else if(m_weaponModInfo.hasLaser)
				{
					available = true;
					text = "Laser";
					textPos.x = 310.0f; textPos.y = 290.0f;
				}
				else if(m_weaponModInfo.isGrenades)
				{
					available = true;
					text = m_weaponModInfo.fireModes[i].c_str();
					textPos.x = 310.0f; textPos.y = 290.0f;
					available = (i == m_weaponModInfo.currentGrenade);
				}

				break;

			case 3: 
				available = m_weaponModInfo.hasSomeAccessory;
				text = "Weapon Mod Menu";
				textPos.x = 340.0f; textPos.y = 235.0f; 
				if(m_weaponModInfo.isGrenades)
				{
					textPos.x = 360.0f;
					text = m_weaponModInfo.fireModes[i].c_str();
					available = (i == m_weaponModInfo.currentGrenade);
				}
				break;

		}	

		//textPos.x *= xScale; textPos.y *= yScale;

		float size = 0.07f * wsSize;
		Vec3 o = radius * (offset[i].x * wsRight + offset[i].y * wsUp);
		ColorB c = available ? color1[i] : color0[i];

		Vec3 lt = wsCenter + o + 0.5f * (size * (wsUp - wsRight));
		Vec3 rt = wsCenter + o + 0.5f * (size * (wsUp + wsRight));
		Vec3 lb = wsCenter + o + 0.5f * (size * (-wsUp - wsRight));
		Vec3 rb = wsCenter + o + 0.5f * (size * (-wsUp + wsRight));

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rt, c, rb, c);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(lt, c, rb, c, lb, c);

		textInfo.color[0] = (float)c.r / 255.0f; textInfo.color[1] = (float)c.g / 255.0f; textInfo.color[2] = (float)c.b / 255.0f; textInfo.color[3] = (float)c.a / 255.0f; 
		textInfo.xscale = textInfo.yscale = 1.5f;
		gEnv->pRenderer->Draw2dLabel(textPos.x, textPos.y, textInfo.yscale, textInfo.color, true, text.c_str());

	}
}

//----------------------------------------------------------------------------------
bool CHUD::Prototype_GetAllWeaponModInfo()
{
	bool hasSomethingToModify =		false;

	CCryFixedStringListT<5, 30> attachments;

	CWeapon *pCurrentWeapon = GetCurrentWeapon();
	if(pCurrentWeapon)
	{
		//Attachments
		const CItem::THelperVector& helpers = pCurrentWeapon->GetAttachmentHelpers();
		for(int iHelper=0; iHelper<helpers.size(); iHelper++)
		{
			CItem::SAttachmentHelper helper = helpers[iHelper];
			if (helper.slot != CItem::eIGS_FirstPerson)
				continue;

			if (!strcmp(helper.name, "magazine"))
			{
				//Check for different ammo types
				pCurrentWeapon->GetAttachmentsAtHelper(helper.name, attachments);
				for(int i = 0; i < attachments.Size(); i++)
				{
					m_weaponModInfo.magazines.push_back(attachments[i]);
					if(pCurrentWeapon->IsAttachmentEquipped(attachments[i]))
						m_weaponModInfo.currentMagazine = i;
				}
				continue;
			}

			if (!strcmp(helper.name, "fire_mode_lever"))
			{
				m_weaponModInfo.fireMode_Bone = helper.bone.c_str();
				continue;
			}

			//Check for accessories
			pCurrentWeapon->GetAttachmentsAtHelper(helper.name, attachments);
			
			for(int i = 0; i < attachments.Size(); i++)
			{
				if(!strcmp(helper.name, "attachment_top"))
				{
					m_weaponModInfo.topBone = helper.bone.c_str();
					m_weaponModInfo.topAccessories.push_back(attachments[i]);
					if(pCurrentWeapon->IsAttachmentEquipped(attachments[i]))
						m_weaponModInfo.currentTopAccessory = i+1; // 0 is "No Attachment"
					m_weaponModInfo.hasSomeAccessory = true;
				}
				else if(!strcmp(helper.name, "attachment_bottom"))
				{
					m_weaponModInfo.barrelBone = helper.bone.c_str();
					m_weaponModInfo.barrelAccessories.push_back(attachments[i]);
					if(pCurrentWeapon->IsAttachmentEquipped(attachments[i]))
						m_weaponModInfo.currentBarrelAccessory = i+1;
					m_weaponModInfo.hasSomeAccessory = true;
				}
				else if(!strcmp(helper.name, "attachment_side") || !strcmp(helper.name, "attachment_bottom_LAM"))
				{
					m_weaponModInfo.sideBone = helper.bone.c_str();
					m_weaponModInfo.sideAccessories.push_back(attachments[i]);
					if(pCurrentWeapon->IsAttachmentEquipped(attachments[i]))
						m_weaponModInfo.currentSideAccessory = i+1;
					m_weaponModInfo.hasSomeAccessory = true;
				}
				else if(strcmp(helper.name, "shell_grenade"))
				{
					m_weaponModInfo.frontBone = helper.bone.c_str();
					m_weaponModInfo.frontAccessories.push_back(attachments[i]);
					if(pCurrentWeapon->IsAttachmentEquipped(attachments[i]))
						m_weaponModInfo.currentFrontAccessory = i+1;
					m_weaponModInfo.hasSomeAccessory = true;
				}

			}
		}

		//Fire modes
		for(int iFM = 0; iFM < pCurrentWeapon->GetNumOfFireModes(); iFM++)
		{
			IFireMode* pFM = pCurrentWeapon->GetFireMode(iFM);

			if(!pFM || !pFM->IsEnabled())
				continue;

			if(!strcmp(pFM->GetType(), "Melee"))
				continue;
			
			m_weaponModInfo.fireModes.push_back(pFM->GetName());
		}

		m_weaponModInfo.currentFireMode = pCurrentWeapon->GetCurrentFireMode();

		//Flashlight & laser
		m_weaponModInfo.hasFlashlight = pCurrentWeapon->Query(eWQ_Has_Accessory_Flashlight);
		m_weaponModInfo.hasLaser = pCurrentWeapon->Query(eWQ_Has_Accessory_Laser);

		//Grenades
		m_weaponModInfo.isGrenades = (strcmp(pCurrentWeapon->GetEntity()->GetClass()->GetName(), "Grenades") == 0) && m_weaponModInfo.fireModes.size() >= 4;
		
		if(m_weaponModInfo.isGrenades)
			m_weaponModInfo.currentGrenade = pCurrentWeapon->GetCurrentFireMode();
		if(m_weaponModInfo.currentFireMode >= m_weaponModInfo.fireModes.size())
			m_weaponModInfo.currentFireMode = 0;
	}

	hasSomethingToModify = !m_weaponModInfo.fireModes.empty() || !m_weaponModInfo.magazines.empty() || m_weaponModInfo.hasSomeAccessory;

	return hasSomethingToModify;
}

//-------------------------------------------------------------------
void CHUD::PrototypeRender_LevelObjectives(const Vec3& worldCameraPos, const Vec3& worldCameraFwd)
{
	const std::vector<CHUDMissionObjective>& objectives = m_missionObjectiveSystem.GetObjectives();
	std::vector<Vec3> objectiveScreenPos; objectiveScreenPos.resize(objectives.size());

	for (int iObjective = 0; iObjective < objectives.size(); ++iObjective)
	{
		const CHUDMissionObjective& objective = objectives[iObjective];

		if ((objective.GetStatus() == CHUDMissionObjective::COMPLETED) ||
			(objective.GetStatus() == CHUDMissionObjective::DEACTIVATED) ||
			(objective.GetStatus() == CHUDMissionObjective::FAILED))
			continue;

		EntityId eid = objective.GetTrackedEntity();
		IEntity* pEnt = gEnv->pEntitySystem->GetEntity(eid);
		if (pEnt)
		{
			Vec3 posWorld = pEnt->GetWorldPos();
			Vec3& posScreen = objectiveScreenPos[iObjective];
			gEnv->pRenderer->ProjectToScreen(posWorld.x, posWorld.y, posWorld.z, &posScreen.x, &posScreen.y, &posScreen.z);
			posScreen.x = posScreen.x / 100.0f * gEnv->pRenderer->GetWidth();
			posScreen.y = posScreen.y / 100.0f * gEnv->pRenderer->GetHeight();
		}
	}
	
	ENanoSuitMode suitMode = eNanoSuitMode_LAST;
	if(m_pNanoSuit)
		suitMode = m_pNanoSuit->GetGameParams().GetMode();

	m_pUIDraw->PreRender();

	for (int iObjective = 0; iObjective < objectives.size(); ++iObjective)
	{
		const CHUDMissionObjective& objective = objectives[iObjective];

		if ((objective.GetStatus() == CHUDMissionObjective::COMPLETED) ||
			(objective.GetStatus() == CHUDMissionObjective::DEACTIVATED) ||
			(objective.GetStatus() == CHUDMissionObjective::FAILED))
			continue;

		//Secondary objectives are only displayed in tactical mode
		if((suitMode != eNanoSuitMode_TACTICAL) && objective.IsSecondary())
			continue;

		EntityId eid = objective.GetTrackedEntity();
		IEntity* pEnt = gEnv->pEntitySystem->GetEntity(eid);
		if (pEnt)
		{
			ColorB shapeColor;
			ColorF textColor;
			float iconScale = 1.0f;
			if (objective.IsSecondary())
			{
				shapeColor = ColorB(255,255,0,255);
				textColor = ColorF(1,1,0,1);
			}
			else
			{
				shapeColor = (suitMode == eNanoSuitMode_TACTICAL) ? ColorB(255,255,0,255) : ColorB(0,255,0,255);
				textColor = (suitMode == eNanoSuitMode_TACTICAL) ? ColorF(1,1,0,1) : ColorF(0,1,0,1);
				iconScale = (suitMode == eNanoSuitMode_TACTICAL) ? 1.65f : 1.0f;
			}

			Vec3 posWorld = pEnt->GetWorldPos();
			Vec3 distanceVector = posWorld - worldCameraPos;
			if ((distanceVector * worldCameraFwd) < 0.0f)
				continue;

			const char* szDescUnLoc = objective.GetShortDescription();
			const wchar_t* szDescLoc = LocalizeWithParams(szDescUnLoc);
			struct IFFont* pFont = gEnv->pCryFont->GetFont("default");
			pFont->SetSize(Vec2(20.0f, 20.0f));
			pFont->SetColor(textColor);

			Vec2 size = pFont->GetTextSizeW(szDescLoc);

			float diamondRadius = 10.0f * iconScale;

			Vec3 posScreen = objectiveScreenPos[iObjective];

			if(suitMode == eNanoSuitMode_TACTICAL)
			{
				float distance = Vec2(distanceVector.x, distanceVector.y).GetLength();

				CryFixedStringT<16> distanceStr; distanceStr.Format("%3.1f", distance);
				pFont->DrawString(posScreen.x - size.x / 2.0f, posScreen.y - (2.0f * size.y) - diamondRadius, 0.0f, szDescUnLoc, false);
				pFont->DrawString(posScreen.x + diamondRadius * 1.5f, posScreen.y - size.y / 2.0f, 0.0f, distanceStr.c_str(), false);
				pFont->DrawStringW(posScreen.x - size.x / 2.0f, posScreen.y - size.y - diamondRadius, 0.0f, szDescLoc, false);

			}
			else
			{
				pFont->DrawString(posScreen.x - size.x / 2.0f, posScreen.y - size.y - diamondRadius, 0.0f, szDescUnLoc, false);
			}


			if(!objective.IsSecondary())
			{
				m_pUIDraw->DrawQuadSimple(posScreen.x - diamondRadius*0.7f, posScreen.y - diamondRadius*0.7f, 
					diamondRadius*1.4f, diamondRadius*1.4f, shapeColor.pack_argb8888(), 0);
			}

			m_pUIDraw->DrawLine(posScreen.x - diamondRadius, posScreen.y, posScreen.x, posScreen.y - diamondRadius, shapeColor.pack_argb8888());
			m_pUIDraw->DrawLine(posScreen.x - diamondRadius, posScreen.y, posScreen.x, posScreen.y + diamondRadius, shapeColor.pack_argb8888());
			m_pUIDraw->DrawLine(posScreen.x + diamondRadius, posScreen.y, posScreen.x, posScreen.y - diamondRadius, shapeColor.pack_argb8888());
			m_pUIDraw->DrawLine(posScreen.x + diamondRadius, posScreen.y, posScreen.x, posScreen.y + diamondRadius, shapeColor.pack_argb8888());

			if (posScreen.x < 0.0f)
			{
				//m_pUIDraw->DrawLine(0.0f, posScreen.y, 20.0f, posScreen.y - 10.0f, shapeColor.pack_argb8888());
				//m_pUIDraw->DrawLine(0.0f, posScreen.y, 20.0f, posScreen.y + 10.0f, shapeColor.pack_argb8888());
				m_pUIDraw->DrawTriangle(0.0f, posScreen.y, 15.0f, posScreen.y-5.0f, 15.0f, posScreen.y+5.0f, shapeColor.pack_argb8888());
			}

			float screenMaxX = (float)gEnv->pRenderer->GetWidth();
			if (posScreen.x > screenMaxX)
			{
				//m_pUIDraw->DrawLine(screenMaxX, posScreen.y, screenMaxX - 10.0f, posScreen.y - 5.0f, shapeColor.pack_argb8888());
				//m_pUIDraw->DrawLine(screenMaxX, posScreen.y, screenMaxX - 10.0f, posScreen.y + 5.0f, shapeColor.pack_argb8888());
				m_pUIDraw->DrawTriangle(screenMaxX, posScreen.y, screenMaxX-15.0f, posScreen.y-5.0f, screenMaxX-15.0f, posScreen.y+5.0f, shapeColor.pack_argb8888());
			}

			if (posScreen.y < 0.0f)
			{
				//m_pUIDraw->DrawLine(posScreen.x, 0.0f, posScreen.x - 5.0f, 10.0f, shapeColor.pack_argb8888());
				//m_pUIDraw->DrawLine(posScreen.x, 0.0f, posScreen.x + 5.0f, 10.0f, shapeColor.pack_argb8888());
				m_pUIDraw->DrawTriangle(posScreen.x, 0, posScreen.x-5.0f, 15.0f, posScreen.x+5.0f, 15.0f, shapeColor.pack_argb8888());
			}

			float screenMaxY = (float)gEnv->pRenderer->GetHeight();
			if (posScreen.y > screenMaxY)
			{
				//m_pUIDraw->DrawLine(posScreen.x, screenMaxY, posScreen.x - 5.0f, screenMaxY - 10.0f, shapeColor.pack_argb8888());
				//m_pUIDraw->DrawLine(posScreen.x, screenMaxY, posScreen.x + 5.0f, screenMaxY - 10.0f, shapeColor.pack_argb8888());
				m_pUIDraw->DrawTriangle(posScreen.x, screenMaxY, posScreen.x-5.0f, screenMaxY-15.0f, posScreen.x+5.0f, screenMaxY-15.0f, shapeColor.pack_argb8888());
			}

			//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(posWorld, 0.2f, sphereColor, false);
		}
	}

	m_pUIDraw->PostRender();
}


//-------------------------------------------------------------------
void CHUD::PrototypeRender_Cover()
{
	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	CPlayer* pPlayer = pActor->IsPlayer() ? (CPlayer*)pActor : 0;
	if (!pPlayer)
		return;

	const float centerX = m_pRenderer->GetWidth() * 0.5f;
	const float centerY = m_pRenderer->GetHeight() * 0.5f;
	const float offset = m_pRenderer->GetWidth() * 0.25f;

	SDrawTextInfo textInfo;
	textInfo.color[0] = 128.0f; textInfo.color[1] = 198.0f; textInfo.color[2] = 255.0f; textInfo.color[3] = 198.0f;
	textInfo.xscale = textInfo.yscale = 5.0f;

	if (pPlayer->IsInCrouchCover())
	{
		const char* text = "^";
		gEnv->pRenderer->Draw2dLabel(centerX, centerY-offset, textInfo.yscale, textInfo.color, true, text);
	}

	if (pPlayer->IsInRightCover())
	{
		const char* text = ">";
		gEnv->pRenderer->Draw2dLabel(centerX+offset, centerY, textInfo.yscale, textInfo.color, true, text);
	}

	if (pPlayer->IsInLeftCover())
	{
		const char* text = "<";
		gEnv->pRenderer->Draw2dLabel(centerX-offset, centerY, textInfo.yscale, textInfo.color, true, text);
	}
}
