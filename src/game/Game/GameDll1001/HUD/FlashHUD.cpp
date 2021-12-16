#include "StdAfx.h"
#include "FlashHUD.h"

#include <IGame.h>
#include <IGameFramework.h>
#include "Player.h"
#include "Weapon.h"
#include "IVehicleSystem.h"
#include "GameCVars.h"
#include "FlashHelper.h"

const char* FLASH_HUD_FILENAME = "Libs\\UI\\FlashHUD.gfx";
const static int NUM_MAX_ENTITIES_IN_PROXIMITY = 128;

CFlashHUD::CFlashHUD() :
	m_pFlashPlayer(NULL)
{
	m_pFlashPlayer = gEnv->pSystem->CreateFlashPlayerInstance();
	if (m_pFlashPlayer == NULL) return;

	g_pGameCVars->g_flash_hud_enable = 1;

	bool bRes = m_pFlashPlayer->Load(FLASH_HUD_FILENAME);
	CryLogAlways("Loading %s: %s",FLASH_HUD_FILENAME, (bRes)?"Done":"Error");
	if (!bRes)
	{
		SAFE_RELEASE(m_pFlashPlayer);
		return;
	}

	m_iWidth = 0;
	m_iHeight = 0;
	updateViewPort();

	m_pFlashPlayer->SetBackgroundAlpha(0);
	if ( gEnv->pInput )
	{
		gEnv->pInput->AddEventListener(this);
	}
	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->AddListener(this);
}

CFlashHUD::~CFlashHUD()
{
	if ( gEnv->pInput )
	{
		gEnv->pInput->RemoveEventListener(this);
	}
	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->RemoveListener(this);
	SAFE_RELEASE(m_pFlashPlayer);
}

void CFlashHUD::Draw(float fDeltaTime)
{	
	if (m_pFlashPlayer == NULL || !g_pGameCVars->g_flash_hud_enable
		|| !gEnv->pGame->GetIGameFramework()->IsGameStarted()
		|| gEnv->pGame->GetIGameFramework()->IsGamePaused()) return;

	updateViewPort();

	// get the client actor
	CActor *pActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());

	if (pActor != NULL && pActor->GetEntity() != 0)
	{
		// health
		if (m_pFlashPlayer->IsAvailable("setHealth"))
			m_pFlashPlayer->Invoke1("setHealth", pActor->GetHealth() );

		// minimap
		Vec3 pos = pActor->GetEntity()->GetWorldPos();

		// calculate players position on minimap, start and end values are defined in the map xml file that is created by the editor during minimap export
		float posx = clamp((pos.x - m_LevelMapInfo.fStartX) / m_LevelMapInfo.fDimX, 0, 1); // value from 0 to 1
		float posy = clamp((pos.y - m_LevelMapInfo.fStartY) / m_LevelMapInfo.fDimY, 0, 1);

		if (m_pFlashPlayer->IsAvailable("setPlayerPos"))
		{
			SFlashVarValue args[3] = { posx, posy , pActor->GetAngles().z *180.0f/gf_PI-90.0f}; 
			m_pFlashPlayer->Invoke("setPlayerPos", args, 3);
		}


		// objects on minimap
		ScanProximity(pActor->GetEntity(), 100.f);

		// ammo of current fire mode
		IItem* pItem = pActor->GetCurrentItem();
		if (pItem != NULL)
		{
			CWeapon* pWeapon = pActor->GetWeapon(pItem->GetEntityId());
			if (pWeapon != NULL)
			{
				//m_pFlashPlayer->Invoke1("setAmmo", pWeapon->GetFirstAmmo().count);

				IFireMode* pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
				if (pFireMode != NULL && m_pFlashPlayer->IsAvailable("setAmmo"))
				{
					SFlashVarValue ammoArgs[4] = { pWeapon->GetAmmoCount(pFireMode->GetAmmoType()), pFireMode->GetClipSize(), pWeapon->GetEntityId(), pWeapon->GetCurrentFireMode() };
					m_pFlashPlayer->Invoke("setAmmo", ammoArgs, 4);
					// crosshair
					if (m_pFlashPlayer->IsAvailable("setCrossHair"))
					{
						m_pFlashPlayer->Invoke1("setCrossHair", CanDrawCrosshair());
					}
				}

			}
		}
	}
	m_pFlashPlayer->Advance(fDeltaTime);
	m_pFlashPlayer->Render();
}

bool CFlashHUD::OnInputEvent( const SInputEvent &event )
{
	if (m_pFlashPlayer == NULL) return false;

	if (event.state == eIS_Released)
	{
		switch (event.keyId)
		{
		case eKI_PgUp:
			m_fCurrScale = clamp(m_fCurrScale * 1.2f, 1.f, 20.f);
			if (m_pFlashPlayer->IsAvailable("scaleMap"))
				m_pFlashPlayer->Invoke1("scaleMap", m_fCurrScale);
			break;
		case eKI_PgDn:
			m_fCurrScale = clamp(m_fCurrScale / 1.2f, 1.f, 20.f);
			if (m_pFlashPlayer->IsAvailable("scaleMap"))
				m_pFlashPlayer->Invoke1("scaleMap", m_fCurrScale);
			break;
		}
	}
	return false;
}

void CFlashHUD::OnLoadingComplete(ILevel *pLevel)
{
	if (m_pFlashPlayer == NULL) return;

	if (pLevel != NULL && pLevel->GetLevelInfo() != NULL)
	{
		m_fCurrScale = 10;
		if (m_pFlashPlayer->IsAvailable("scaleMap"))
			m_pFlashPlayer->Invoke1("scaleMap", m_fCurrScale);

		IXmlParser*	pxml = gEnv->pGame->GetIGameFramework()->GetISystem()->GetXmlUtils()->CreateXmlParser();
		if(!pxml) return;

		char tmp[255];
		sprintf(tmp,"%s/%s.xml", pLevel->GetLevelInfo()->GetPath(), pLevel->GetLevelInfo()->GetName());
		XmlNodeRef node = GetISystem()->LoadXmlFile(tmp);
		if(!node) return;

		node = node->findChild("Minimap");
		if (node) 
		{
			const char* minimap_dds;
			node->getAttr("Filename", &minimap_dds);
			sprintf(tmp,"%s/%s", pLevel->GetLevelInfo()->GetPath(), minimap_dds);

			node->getAttr("startX", m_LevelMapInfo.fStartX);
			node->getAttr("startY", m_LevelMapInfo.fStartY);
			node->getAttr("endX", m_LevelMapInfo.fEndX);
			node->getAttr("endY", m_LevelMapInfo.fEndY);
			m_LevelMapInfo.fDimX = m_LevelMapInfo.fEndX - m_LevelMapInfo.fStartX;
			m_LevelMapInfo.fDimY = m_LevelMapInfo.fEndY - m_LevelMapInfo.fStartY;
			m_LevelMapInfo.fDimX = m_LevelMapInfo.fDimX > 0 ? m_LevelMapInfo.fDimX : 1;
			m_LevelMapInfo.fDimY = m_LevelMapInfo.fDimY > 0 ? m_LevelMapInfo.fDimY : 1;
			m_LevelMapInfo.sMinimapName = tmp;
		}
		if (m_pFlashPlayer->IsAvailable("setMiniMap"))
			m_pFlashPlayer->Invoke1("setMiniMap", m_LevelMapInfo.sMinimapName.c_str());
	}
}

void CFlashHUD::ScanProximity(IEntity *pPlayerEntity, float radius)
{
	assert(m_pFlashPlayer != NULL);
	if (m_pFlashPlayer == NULL) return;

	m_EntitiesInProximity.clear();

	IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
	IVehicleSystem *pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
	IItemSystem *pItemSystem  = gEnv->pGame->GetIGameFramework()->GetIItemSystem();

	if (pPlayerEntity != NULL)
	{
		Vec3 vPlayerPos = pPlayerEntity->GetWorldPos();

		SEntityProximityQuery query;
		query.box = AABB( Vec3( vPlayerPos.x-radius, vPlayerPos.y-(radius*1.33f), vPlayerPos.z-radius),
						  Vec3(vPlayerPos.x+radius,vPlayerPos.y+(radius*1.33f),vPlayerPos.z+radius) );
		query.nEntityFlags = ENTITY_FLAG_ON_RADAR; // Filter by entity flag.
		gEnv->pEntitySystem->QueryProximity( query );

		for(int iEntity=0; iEntity<query.nCount && iEntity < NUM_MAX_ENTITIES_IN_PROXIMITY; iEntity++)
		{
			IEntity *pEntity = query.pEntities[iEntity];
			if(pEntity && !pEntity->IsHidden() && pEntity != pPlayerEntity)
			{
				EntityId id = pEntity->GetId();
				Vec3 pos = pEntity->GetWorldPos();
				float posx = clamp((pos.x - m_LevelMapInfo.fStartX) / m_LevelMapInfo.fDimX, 0, 1);
				float posy = clamp((pos.y - m_LevelMapInfo.fStartY) / m_LevelMapInfo.fDimY, 0, 1);
				int rot = (int) (pEntity->GetWorldAngles().z *180.0f/gf_PI-90.0f);

				if (IActor *pActor = pActorSystem->GetActor(id))
				{
					if (pActor->GetHealth() > 0)
					{
						if(IVehicle* pVehicle = pActor->GetLinkedVehicle())
						{
							m_EntitiesInProximity.push_back(RadarEntity(id, eRET_Actor, posx, posy, rot, pActor->GetHealth()));
						}
						else
						{
							m_EntitiesInProximity.push_back(RadarEntity(id, eRET_Actor, posx, posy, rot, pActor->GetHealth()));
						}
					}
				}
				else if(IItem *pItem = pItemSystem->GetItem(id))
				{
					m_EntitiesInProximity.push_back(RadarEntity(id, eRET_Item, posx, posy, rot));
				}
				else if(IVehicle *pVehicle = pVehicleSystem->GetVehicle(id))
				{
					m_EntitiesInProximity.push_back(RadarEntity(id, eRET_Vehicle, posx, posy, rot));
				}
			}
		}
	}

	int numOfValues = 0;
	std::vector<float> flashArray(m_EntitiesInProximity.size() * 6);
	for (std::list<RadarEntity>::iterator iter = m_EntitiesInProximity.begin(); iter != m_EntitiesInProximity.end(); ++iter)
	{
		flashArray[numOfValues] = (float)iter->iId;
		flashArray[numOfValues + 1] = (float)iter->eType;
		flashArray[numOfValues + 2] = (float)iter->fPosX;
		flashArray[numOfValues + 3] = (float)iter->fPosY;
		flashArray[numOfValues + 4] = (float)iter->iRotation;
		flashArray[numOfValues + 5] = (float)iter->iHealth;
		numOfValues += 6;
	}
	if (m_pFlashPlayer->IsAvailable("m_allEntities") && m_pFlashPlayer->IsAvailable("updateEntityList"))
	{
		m_pFlashPlayer->SetVariableArray(FVAT_Float, "m_allEntities", 0, &flashArray[0], numOfValues);
		m_pFlashPlayer->Invoke0("updateEntityList");
	}
}

void CFlashHUD::updateViewPort()
{
	assert(m_pFlashPlayer != NULL);
	if (m_pFlashPlayer == NULL) return;

	int x, y, dx, dy;
	gEnv->pRenderer->GetViewport(&x, &y, &dx, &dy);

	if (dx != m_iWidth || dy != m_iHeight || (g_pGameCVars->g_flash_hud_scale_enable != 0) != m_bScaleEnabled)
	{
		m_iWidth = dx;
		m_iHeight = dy;
		m_bScaleEnabled = g_pGameCVars->g_flash_hud_scale_enable != 0;

		SFlashHelper::updateViewPort(m_pFlashPlayer, m_bScaleEnabled, SFlashHelper::eFPT_Upper, SFlashHelper::eFPT_Lower);

		if (m_pFlashPlayer->IsAvailable("setCrossHairPos"))
		{
			int x, y, dx, dy;
			float aspect;
			m_pFlashPlayer->GetViewport(x,y,dx,dy,aspect);
			int centerX = (int) (m_pFlashPlayer->GetWidth()  / 2 - (x / 2.f) * (m_pFlashPlayer->GetWidth()  / (float) dx ));
			int centerY = (int) (m_pFlashPlayer->GetHeight() / 2 - (y / 2.f) * (m_pFlashPlayer->GetHeight() / (float) dy));

			SFlashVarValue crossHairPosition[2] = {centerX,  centerY};
			m_pFlashPlayer->Invoke("setCrossHairPos", crossHairPosition, 2);
		}

	}
}

bool CFlashHUD::CanDrawCrosshair() const
{

	if (!g_pGameCVars->g_show_crosshair)
	{	return false;	}

	CPlayer* pPlayer = static_cast< CPlayer* >( gEnv->pGame->GetIGameFramework()->GetClientActor() );
	if ( pPlayer == NULL )
	{	return false;	}

	EntityId mountedWeaponId = pPlayer->GetInventory()->GetCurrentItem();
	CWeapon* pWeapon = pPlayer->GetWeapon( mountedWeaponId );
	if ( pWeapon == NULL )
	{	return false;	}

	bool carryingMeleeWeapon = pWeapon->CanMeleeAttack();
	if ( carryingMeleeWeapon )
	{	return false;	}

	bool usingWeaponSightForAiming = pPlayer->IsThirdPerson() ? 
		!g_pGameCVars->g_show_crosshair_tp : 
	pWeapon->IsZoomed();
	if ( usingWeaponSightForAiming )
	{	return false;	}

	if ( pPlayer->IsDead() )
		return false;

	return true;
}