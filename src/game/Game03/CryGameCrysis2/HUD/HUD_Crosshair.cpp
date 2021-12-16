#include "StdAfx.h"
#include "HUD_Crosshair.h"
#include "HUD.h"
#include "HUD/HUDCVars.h"
#include "HUD/HUDDefines.h"

#include "IActorSystem.h"
#include "IItemSystem.h"
#include "GameCVars.h"
#include "IGameFramework.h"
#include "GameRules.h"
#include "FM_TunnellingGrenade.h"
#include "PerkDbgDisplay.h"
#include "IAIObject.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

//////////////////////////////////////////////////////////////////////////


CHUD_Crosshair::CHUD_Crosshair()
: m_rootObj(NULL)
, m_friendlyFireObj(NULL)
, m_dirty(CHUD_Crosshair::eDF_Crosshair|CHUD_Crosshair::eDF_Target)
, m_dontFire(false)
, m_spread(0.0f)
, m_opacity(1.0f)
, m_crosshair(CHUD_Crosshair::eHCH_None)
, m_targetType(CHUD_Crosshair::eTT_None)
, m_targetHitTimer(0.f)
, m_targetHitTime(0.f)
, m_hitIndicatorFade(1)
, m_visible(true)
, m_reloading(false)
, m_enemyToLockOnTo(0)
, m_timeLockedOnEnemy(-1.0f)
{
	for(int i = 0; i < eHUDEventHT_numTypes; i++)
	{
		m_hitIndicatorTime[i] = 0.5f;
	}

	gEnv->pConsole->Register("hud_CrosshairHitIndicatorTime", &m_hitIndicatorTime[eHUDEventHT_Bullet], 0.5f, 0, "time that the 'you hit a target' indicator is shown for a bullet hit");
	gEnv->pConsole->Register("hud_CrosshairExplosionHitIndicatorTime", &m_hitIndicatorTime[eHUDEventHT_Explosive], 0.5f, 0, "time that the 'you hit a target' indicator is shown for an explosion");
	gEnv->pConsole->Register("hud_CrosshairHitIndicatorFade", &m_hitIndicatorFade, 1, 0, "Should the 'you hit a target' indicator fade out over time.");

	m_rootDI.Clear();
	m_friendlyFireDI.Clear();


	for( int i=0; i<k_numberCrosshairs; i++ )
	{
		m_crosshairObj[i] = NULL;
		//m_hitObj[i] = NULL;
		m_crosshairDI[i].Clear();
		//m_hitDI[i].Clear();
	}
}



CHUD_Crosshair::~CHUD_Crosshair()
{
	IConsole *pConsole = gEnv->pConsole;
	if (pConsole)
	{
		pConsole->UnregisterVariable("hud_CrosshairHitIndicatorTime");
		pConsole->UnregisterVariable("hud_CrosshairExplosionHitIndicatorTime");
		pConsole->UnregisterVariable("hud_CrosshairHitIndicatorFade");
	}
}

void CHUD_Crosshair::Init( void )
{
	IHUDAsset* pAsset = GetAsset2D();
	HUD_FLASVAROBJ_REG_DI( pAsset, "Crosshair_Root",              m_rootObj,                 m_rootDI);
	HUD_FLASVAROBJ_REG_DI( pAsset, "Crosshair_Root.FriendlyFire", m_friendlyFireObj, m_friendlyFireDI);

	for( int i=0; i<k_numberCrosshairs; i++ )
	{
		const int crosshairType = i+1; // types are 1 based
		const int crosshairsFrameNumber = (int)crosshairType + 1; // crosshair frames start at 2 but next to each other on frame 3 & 4.
		                                                          // will need to update if frames shift around in flash
		m_rootObj->GotoAndStop( crosshairsFrameNumber );
		pAsset->Update(1.0f);
		HUD_FLASVAROBJ_REG_DI( pAsset, "Crosshair_Root.Crosshair",    m_crosshairObj[i],    m_crosshairDI[i] );

		CRY_TODO( 19, 02, 2009, "HUD: Crosshair: work out why the hit MC can't be ref'd as a complex object.")
		//CRY_TODO( 19, 02, 2009, "HUD: Crosshair: fixme when we have \"I have hit someone\" indicators for all crosshairs.")
		//m_rootObj->GotoAndStop( 1 );
		//pAsset->Update(1.0f);
		//HUD_FLASVAROBJ_REG_DI( pAsset, "Crosshair_Root.Hit",          m_hitObj[i],          m_hitDI[i] );
	}
}

void CHUD_Crosshair::PreDelete( void )
{
	HUD_FLASHOBJ_SAFERELEASE( m_rootObj );
	HUD_FLASHOBJ_SAFERELEASE( m_friendlyFireObj );

	for( int i=0; i<k_numberCrosshairs; i++ )
	{
		HUD_FLASHOBJ_SAFERELEASE( m_crosshairObj[i] );
		//HUD_FLASHOBJ_SAFERELEASE( m_hitObj[i] );
	}
}

void CHUD_Crosshair::OnHUDEvent(const SHUDEvent& event)
{
	switch(event.eventType)
	{
	case eHUDEvent_OnCrosshairSelected :
		SelectCrosshair((eHCH_Types)event.GetData(0).GetInt());
		break;
	case eHUDEvent_OnLookAtChanged :
		SetLookAtTarget((EntityId)event.GetData(0).GetInt());
		break;
	case eHUDEvent_OnHitTarget :
		OnHitTarget(event);
		break;
	case eHUDEvent_OnSuitMenuOpened :
		m_visible = false;
		break;
	case eHUDEvent_OnSuitMenuClosed :
		m_visible = true;
		break;
	case eHUDEvent_OnItemSelected :
	case eHUDEvent_OnReloaded :
		CRY_TODO(4, 2, 2010, "Fix the missing EndReload (or ReloadInterrupted) event in Weapon directly.");
		m_reloading = false;
		m_dirty |= eDF_Visibility;
		break;
	case eHUDEvent_OnStartReload :
		if( g_pGame->GetHUD()->GetCVars()->hud_CrosshairHideOnReload )
		{
			m_reloading = true;
			m_dirty |= eDF_Visibility;
		}
		break;
	case eHUDEvent_OnBlind :
		m_rootDI.SetAlpha( event.GetData(0).GetFloat()*100.0f );
		m_dirty |= eDF_SetDisplayInfo;
		break;
	case eHUDEvent_OnEndBlind :
		m_rootDI.SetAlpha( 100.0f );
		m_dirty |= eDF_SetDisplayInfo;
		break;
	case eHUDEvent_OnSpawn :
		m_reloading = false;
		m_dirty |= eDF_Visibility;
		m_rootDI.SetAlpha( 100.0f );
		m_dirty |= eDF_SetDisplayInfo;
		break;
	default :
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "HUD: Unhandled event for crosshair!" );
	}
}



void CHUD_Crosshair::Update(float frameTime)
{
	UpdateWeaponEnvironment(frameTime);

	if( m_timeLockedOnEnemy > 0.f )
	{
		m_timeLockedOnEnemy -= frameTime;
	}
	else if( m_enemyToLockOnTo && m_targetType != eTT_Enemy )
	{
		m_targetType = eTT_Enemy;
		m_dirty |= CHUD_Crosshair::eDF_Target|CHUD_Crosshair::eDF_Spread;
		m_enemyToLockOnTo = 0;
	}

	if (m_dirty != CHUD_Crosshair::eDF_None)
	{
		if(m_dirty & eDF_Visibility)
		{
			m_rootObj->SetVisible(m_visible && !m_reloading);
		}

		if(m_dirty & CHUD_Crosshair::eDF_Crosshair)
		{
			m_rootObj->GotoAndStop( (m_crosshair+1) );
			m_dirty ^= CHUD_Crosshair::eDF_Crosshair;
		}
		else
		{
			if(m_dirty & CHUD_Crosshair::eDF_Target)
			{
				m_crosshairObj[m_crosshair-1]->GotoAndStop( (int)m_targetType );
				if(m_dontFire)
				{
					m_friendlyFireObj->GotoAndStop( (int)m_targetType );
				}
			}

			if(m_dirty & CHUD_Crosshair::eDF_Opacity)
			{
				m_crosshairDI[m_crosshair-1].SetAlpha(m_opacity*100.0f);
				m_dirty |= eDF_SetDisplayInfo;
			}

			if(m_dirty & CHUD_Crosshair::eDF_Spread)
			{
				m_rootObj->Invoke1("setSpread", GetScaledSpread(m_spread));
			}

			if(m_dirty & CHUD_Crosshair::eDF_DontFire)
			{
				m_friendlyFireDI.SetVisible( m_dontFire );
				if(m_dontFire)
				{
					m_friendlyFireObj->GotoAndStop( (int)m_targetType );
				}
			}

			if(m_dirty & CHUD_Crosshair::eDF_HitIndicator)
			{
				if (m_targetHitTimer > 0.f)
				{
					m_rootObj->Invoke1("setEnemyHit", true);
					//m_hitDI[m_crosshair-1].SetAlpha(100.0f);
					//m_dirty |= eDF_SetDisplayInfo;
					GetAsset2D()->Invoke("Crosshair_Root.setEnemyHit", true);
					GetAsset2D()->SetVariable("Crosshair_Root.Hit._alpha", 100.f);
				}
				else
				{
					m_rootObj->Invoke1("setEnemyHit", false);
				}
			}

			if(m_hitIndicatorFade==1 && m_targetHitTimer > 0.f && m_targetHitTime > 0.f)
			{
				const float alpha = (m_targetHitTimer / m_targetHitTime) * 100.f;
				//m_hitDI[m_crosshair-1].SetAlpha(alpha);
				//m_dirty |= eDF_SetDisplayInfo;
				GetAsset2D()->SetVariable("Crosshair_Root.Hit._alpha", alpha);
			}

			// Please keep last
			if( m_dirty & eDF_SetDisplayInfo )
			{
				m_rootObj->SetDisplayInfo(m_rootDI);
				m_crosshairObj[m_crosshair-1]->SetDisplayInfo( (m_crosshairDI[m_crosshair-1]) );
				//m_hitObj[m_crosshair-1]->SetDisplayInfo(m_hitDI[m_crosshair-1]);
				m_friendlyFireObj->SetDisplayInfo(m_friendlyFireDI);
			}

			m_dirty = CHUD_Crosshair::eDF_None;
		}
	}
}



void CHUD_Crosshair::Draw()
{
	if(!m_visible || m_reloading || m_crosshair==eHCH_None )
		return;

#if ENABLE_HUD_EXTRA_DEBUG
	if( g_pGame->GetHUD()->GetCVars()->hud_CrosshairShowInfo )
	{
		CryWatch( "Crosshair :");
		CryWatch( "\t m_spread = %f ", m_spread );
		CryWatch( "\t m_weaponSpread (orig) = %f ", m_weaponSpread );
		CryWatch( "\t hud_CrosshairSpreadMultiplier = %f", g_pGame->GetHUD()->GetCVars()->hud_CrosshairSpreadMultiplier );
		CryWatch( "\t hud_CrosshairMinSpread = %f", g_pGame->GetHUD()->GetCVars()->hud_CrosshairMinSpread );
		CryWatch( "\t hud_CrosshairUseSmoothSpread = %d", g_pGame->GetHUD()->GetCVars()->hud_CrosshairUseSmoothSpread );
		CryWatch( "\t invoke.setSpread = %f", GetScaledSpread(m_spread) );
	}
#endif
}



void CHUD_Crosshair::UpdateWeaponEnvironment(float frameTime)
{
	// Function changed 14/10/09 so that it doesn't return early. We want to tick all of this even if we have no weapon (which should make the crosshair vanish).
	// Otherwise, dying leaves the crosshair, 'don't shoot' cross etc. frozen in their most recent states... [TF]

	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	IItem* pItem = pActor ? pActor->GetCurrentItem() : NULL;
	IWeapon* pWeapon = pItem ? pItem->GetIWeapon() : NULL;

	float opacity = pWeapon ? pWeapon->GetCrosshairOpacity() : 0.f;
	if(opacity != m_opacity)
	{
		m_opacity = opacity;
		m_dirty |= CHUD_Crosshair::eDF_Opacity;
	}

	bool visible = pWeapon ? pWeapon->GetCrosshairVisibility() : false;
	if(visible != m_visible)
	{
		m_visible = visible;
		m_dirty |= eDF_Visibility;
	}

	IFireMode* pFireMode = pWeapon ? pWeapon->GetFireMode(pWeapon->GetCurrentFireMode()) : NULL;

	float spread = 0.f;
	bool drawDontFireCross = false;

	if (pFireMode)
	{
		spread = pFireMode->GetSpreadForHUD();
		if(!strcmp(pFireMode->GetType(), "Tun_Gren"))
		{
			CTunnellingGrenadeFM* pTunGren = static_cast<CTunnellingGrenadeFM*>(pFireMode);
			drawDontFireCross = pTunGren->IsBlocked();
		}

#if ENABLE_HUD_EXTRA_DEBUG
		m_weaponSpread = spread;
#endif
	}

	HudDbgDisplay("CROSSHAIR: fireMode=%s opacity=%f visible=%d don'tFire=%d", pFireMode ? pFireMode->GetType() : "NULL", opacity, visible, drawDontFireCross);

	if(spread != m_spread)
	{
		float rate = 0.0f;
		if( g_pGame->GetHUD()->GetCVars()->hud_CrosshairUseSmoothSpread )
		{
			SmoothCD(m_spread, rate, frameTime, spread, 0.035f);
		}
		else
		{
			m_spread = spread;
		}
		m_dirty |= CHUD_Crosshair::eDF_Spread;
	}

	if (drawDontFireCross != m_dontFire)
	{
		m_dontFire = drawDontFireCross;
		m_dirty |= CHUD_Crosshair::eDF_DontFire;
	}

	if (m_targetHitTimer > 0.f)
	{
		m_targetHitTimer -= frameTime;
		if (m_targetHitTimer <= 0.f)
		{
			m_dirty |= CHUD_Crosshair::eDF_HitIndicator;
		}
	}
}



void CHUD_Crosshair::SelectCrosshair(eHCH_Types crosshair)
{
	if(crosshair != m_crosshair)
	{
		m_crosshair = crosshair;
		if( m_crosshair )
		{
			m_dirty |= CHUD_Crosshair::eDF_Crosshair|CHUD_Crosshair::eDF_Target|CHUD_Crosshair::eDF_HitIndicator|CHUD_Crosshair::eDF_Spread;
		}
	}


#if ENABLE_HUD_EXTRA_DEBUG
	switch( crosshair )
	{
	case eHCH_Normal:
	case eHCH_Shotgun:
	case eHCH_None:
		break;
	default:
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Setting cross hair to unkown type! '%d'", crosshair).c_str() )
	}
#endif
}



void CHUD_Crosshair::SetLookAtTarget(EntityId target)
{
	CHUD_Crosshair::TargetType targetType = GetLookAtType(target);
	if(    targetType == eTT_Enemy 
	    && m_enemyToLockOnTo != target
	    && m_timeLockedOnEnemy <= 0.f )
	{
		CCCPOINT(HUD_CrosshairMovesOverEnemy);
		m_enemyToLockOnTo = target;
		m_timeLockedOnEnemy = g_pGame->GetHUD()->GetCVars()->hud_mpTagNames_LockDuration;
	}
	else 
	{
		if(targetType != m_targetType)
		{
			CCCPOINT_IF(targetType == eTT_Friend, HUD_CrosshairMovesOverFriend);
			CCCPOINT_IF(targetType == eTT_None,   HUD_CrosshairMovesOverNone);
			m_targetType = targetType;
			m_dirty |= CHUD_Crosshair::eDF_Target|CHUD_Crosshair::eDF_Spread;
		}
		m_enemyToLockOnTo = 0;
		m_timeLockedOnEnemy = -1.0f;
	}
}



CHUD_Crosshair::TargetType CHUD_Crosshair::GetLookAtType(EntityId target)
{
	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(target);
	CRY_FIXME(02,10,09,"We need a single unified and accurate function where we can do friend or foe testing for all game code (like in the C2MP CXP codebase)");
	if (gEnv->bMultiplayer)
	{
		IActor* const clientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
		if (clientActor && !clientActor->IsDead() )
		{
			if(pActor && pActor->IsPlayer() && pActor != clientActor && !pActor->IsDead() )
			{
				CGameRules* const pGameRules=g_pGame->GetGameRules();
				const int ownTeam          = pGameRules->GetTeam(clientActor->GetEntityId());
				const int lookAtPlayerTeam = pGameRules->GetTeam(pActor->GetEntityId());
				if( ownTeam != lookAtPlayerTeam || ownTeam == 0 )
				{
					return CHUD_Crosshair::eTT_Enemy;
				}
				else
				{
					return CHUD_Crosshair::eTT_Friend;
				}
			}
		}
		return CHUD_Crosshair::eTT_None;
	}
	else
	{
		if(!pActor || pActor->GetChannelId()) //non-player actor, TODO : add support for factions
			return CHUD_Crosshair::eTT_None;

		IEntity* pEntity = pActor->GetEntity();
		if (!pEntity->GetPhysics() || pEntity->GetPhysics()->GetType()!=PE_LIVING)
			return CHUD_Crosshair::eTT_None;

		if (IAIObject* pAIObject = pEntity->GetAI())
		{
			IActor* pPlayer = gEnv->pGame->GetIGameFramework()->GetClientActor();
			IAIObject* pPlayerAI = pPlayer ? pPlayer->GetEntity()->GetAI() : 0;
			if (pPlayerAI)
			{
				IAIObject* pAI = pActor->GetEntity()->GetAI();
				if (pAI && !pAI->IsHostile(pPlayerAI, false))
					return CHUD_Crosshair::eTT_None;
			}
		}
		
		return CHUD_Crosshair::eTT_Enemy;
	}
}



float CHUD_Crosshair::GetScaledSpread(float base)
{
	return g_pGame->GetHUD()->GetCVars()->hud_CrosshairSpreadMultiplier * ( max(g_pGame->GetHUD()->GetCVars()->hud_CrosshairMinSpread, base) );
}

void CHUD_Crosshair::OnHitTarget(const SHUDEvent& theEvent)
{
	switch (theEvent.eventIntData)
	{
		case EGRTT_Hostile:
		m_dirty |= eDF_HitIndicator;

		CRY_ASSERT_MESSAGE(theEvent.eventIntData2 >= 0 && theEvent.eventIntData2 < eHUDEventHT_numTypes, "Invalid Hit Type for hit indicator");

		m_targetHitTimer = m_hitIndicatorTime[theEvent.eventIntData2];
		m_targetHitTime = m_targetHitTimer;
		break;

		// No feedback for injuring neutral objects yet...
		// No feedback for injuring friendly objects yet...
	}
}

//////////////////////////////////////////////////////////////////////////
