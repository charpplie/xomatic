#include "StdAfx.h"

#include "HUD/UI/UITagNames.h"

#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"
#include "HUD/ScreenLayoutManager.h"
#include "HUD/HUD_Crosshair.h"

#include "Graphics/2DRenderUtils.h"

#include "Game.h"
#include "GameCVars.h"
#include "Player.h"
#include "PerkDbgDisplay.h"
#include <IWorldQuery.h>

#include "Utility/CryWatch.h"

//////////////////////////////////////////////////////////////////////////

static const ColorF COLOR_DEAD(0.4f,0.4f,0.4f);
static const ColorF COLOR_ENEMY(0.9f,0.1f,0.1f);
static const ColorF COLOR_FRIEND(0.0353f,0.6235f,0.9137f);
static const int MAX_TAGNAME_HEALTHBAR_NUMBLOCKS = 20;

//////////////////////////////////////////////////////////////////////////

const float CUITagNames::k_ignoreEntityForever = -1.0f;

//////////////////////////////////////////////////////////////////////////

#if ENABLE_HUD_EXTRA_DEBUG
#define DEBUG_TAGNAMES(...) if(g_pGame->GetHUD()->GetCVars()->hud_mpTagNames_debug > 0) { CryWatch(string().Format("<UITagNames> " __VA_ARGS__)); }
#else
#define DEBUG_TAGNAMES(...) (0)
#endif

//////////////////////////////////////////////////////////////////////////

CTagnameCommon::CTagnameCommon()
{
	// ... 
}

bool CTagnameCommon::CheckActorIsInValidStateToShowTagName(IActor * pActor)
{
	return (pActor->GetSpectatorMode() == CActor::eASM_None) && (g_pGame->GetHUD()->GetCVars()->hud_tagNames_showOverCorpses || pActor->GetHealth() > 0);
}

//////////////////////////////////////////////////////////////////////////

CUITagNames::CUITagNames()
: m_lookAt(0)
, m_enemyToLockOnTo(0)
, m_timeLockedOnEnemy(-1.0f)
, m_alpha(1.0f)
{
	m_tagsCount = 0;

	memset(m_ignoreEnts, 0, sizeof(m_ignoreEnts));
	memset(m_tagInfo, 0, sizeof(m_tagInfo));

	CHUD::AddHUDEventListener(this, "OnLookAtChanged" );
	CHUD::AddHUDEventListener(this, "OnBlind" );
	CHUD::AddHUDEventListener(this, "OnEndBlind" );
	CHUD::AddHUDEventListener(this, "OnSpawn" );
	CHUD::AddHUDEventListener(this, "OnIgnoreEntity");
	CHUD::AddHUDEventListener(this, "OnStopIgnoringEntity");
}

CUITagNames::~CUITagNames()
{
	CHUD::RemoveHUDEventListener(this);
}

void CUITagNames::Initialize(const IItemParamsNode* xmlElement, IUIElement* parent)
{
	TTagNamesParent::Initialize(xmlElement, parent);

	const IItemParamsNode * child = xmlElement->GetChild("Font");
	CRY_ASSERT_MESSAGE(child, "Expecting font for TagNames element");

	const char* fontName = child->GetAttribute("name");

	m_font = gEnv->pCryFont->GetFont(fontName);
	CRY_ASSERT_MESSAGE(m_font, "Failed to load font for TagNames");
	m_alpha = 1.0f;
}

void CUITagNames::Update(float frameTime)
{
	m_tagsCount = 0;

	float size = 0.0f;
	float fDistance = 0.f;
	Vec3 drawPos(0.0f, 0.0f, 0.0f);

	IActor* clientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	CActor* pClientActor = static_cast<CActor*>(clientActor);

	if (!pClientActor || m_alpha<1.0f)
	{
		DEBUG_TAGNAMES("Not Entering update (%s || %s)", pClientActor == NULL ? "Null Client Actor" : "Valid Client Actor", m_alpha<1.0f ? "transparent" : "shown");
		return;
	}

	CHUDCVars* pCvars = g_pGame->GetHUD()->GetCVars();

	bool showAllNames = (pClientActor->GetHealth() <= 0) || pCvars->hud_mpTagNames_ForceDraw;
	float asyncTime = gEnv->pTimer->GetAsyncTime().GetSeconds();

	if( m_enemyToLockOnTo != 0 )
	{
		if( m_timeLockedOnEnemy > 0 )
		{
			m_timeLockedOnEnemy -= frameTime;
		}
		else
		{
			IgnoreEntity( m_enemyToLockOnTo, false, g_pGame->GetHUD()->GetCVars()->hud_mpTagNames_Duration  );
			if( m_lookAt != m_enemyToLockOnTo )
			{
				m_enemyToLockOnTo = 0;
			}
		}
	}

	UpdateIgnoreEntities(frameTime);

	IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
	IActor *pActor = NULL;
	while( (pActor = pIter->Next()) )
	{
		IEntity* pEntity = pActor->GetEntity();
		if(!pEntity || !pEntity->IsActive())
		{
			DEBUG_TAGNAMES("\t Skipping %s (%d) - Null or Non-active entity", pActor->GetEntity()->GetName(), pActor->GetEntityId() );
			continue;
		}

		const EntityId entityId = pActor->GetEntityId();

		const SIgnoreEnts::eIE_IgnoreState shouldIgnore = ShouldIgnoreEntity( entityId );
		if (entityId == pClientActor->GetEntityId() || shouldIgnore==SIgnoreEnts::eIEIS_ignore )
		{
			DEBUG_TAGNAMES("\t Skipping %s - (%s || %s)", pActor->GetEntity()->GetName(), entityId == pClientActor->GetEntityId() ? "Client Entity" : "No Client Entity", ShouldIgnoreEntity( entityId ) ? "Should Ignore" : "Not ignoring");
			continue;
		}

		if(pActor->IsPlayer() && CheckActorIsInValidStateToShowTagName(pActor) && GetTagNamePosition(pEntity, &drawPos, &size, &fDistance))
		{
			CRY_ASSERT_MESSAGE(pClientActor->GetGameObject(), "HUD: Tagnames: Missing Game Object for client");
			CRY_ASSERT_MESSAGE(pClientActor->GetGameObject()->GetWorldQuery(), "HUD: Tagnames: Missing World Query for client");
			CRY_ASSERT_MESSAGE(m_tagsCount < HUD_MAX_TAGNAMES, "HUD: Tagnames: Trying to render more tag names than players?");
			if(m_tagsCount >= HUD_MAX_TAGNAMES)
			{
				break;
			}

			const bool isFriendly = pClientActor->IsFriendlyEntity(entityId) != 0;
			const bool seeWhenObscuredVar = (isFriendly ? pCvars->hud_mpTagNames_ThroughGeom_friendies : pCvars->hud_mpTagNames_ThroughGeom_enemies) > 0;
			const bool visibleOverride = seeWhenObscuredVar       // see through walls (without $5 X-Ray specs! Cool)
			                             || showAllNames;         // rendering all names.
			bool visible = true;
			if( !visibleOverride )
			{
				// don't need to do this when showing all or ignoring obstructions.
				visible = g_pGame->GetPlayerVisTable()->CanLocalPlayerSee(pEntity->GetId(), 5);
			}

			const bool shouldDisplay = visibleOverride             // Don't care whether the player's hidden or not.
			                           || (visible && shouldIgnore==SIgnoreEnts::eIEIS_forceShow) // Is visible and the entity is _not_ being ignored i.e. enemies.
			                           || (visible && isFriendly); // Can you see your friends?

			if (shouldDisplay)
			{
				bool addIt = isFriendly || showAllNames || shouldIgnore==SIgnoreEnts::eIEIS_forceShow;

				if (! addIt)
				{
					CPlayer* pPlayer = static_cast<CPlayer*>(pActor);

					// Only show names for enemies if damaged recently or being looked at by local player...
					if(pPlayer->GetLastDamageSeconds() + pCvars->hud_mpTagNames_Duration > asyncTime)
					{
						addIt = true;
					}
					//else if ((seeWhenObscuredVar == 0 || g_pGame->GetPlayerVisTable()->CanLocalPlayerSee(pEntity->GetId(), 5)) && ! pPlayer->GetActorSuitGameParameters().IsCloakEnabled())
					else if ((seeWhenObscuredVar == 0) && ! pPlayer->GetActorSuitGameParameters().IsCloakEnabled())
					{
						const Vec3 dirToEnemyUnit = (pEntity->GetPos() - pClientActor->GetEntity()->GetPos()).GetNormalized();
						const Vec3 & lookDirUnit = pClientActor->GetGameObject()->GetWorldQuery()->GetDir();
						assert (lookDirUnit.IsUnit());
						addIt = (lookDirUnit.dot(dirToEnemyUnit) > pCvars->hud_tagNames_showOverEnemiesWhenLookDirIsThisAccurate);
						if(!addIt)
						{
							EntityId lookAt = pClientActor->GetGameObject()->GetWorldQuery()->GetLookAtEntityId();
							addIt = (lookAt == pEntity->GetId());
						}
					}
				}

				if (addIt)
				{
					DEBUG_TAGNAMES("\t Added %s", pActor->GetEntity()->GetName());
					m_tagInfo[m_tagsCount].set(entityId, isFriendly, drawPos, size, fDistance);
					m_tagsCount++;
				}
				else
				{
					DEBUG_TAGNAMES("\t Skipping %s - Not Recently Damaged Or not looking at entity", pActor->GetEntity()->GetName());
				}
			}
			else
			{
				DEBUG_TAGNAMES("\t Skipping %s - Should Display (%s || %s)", pActor->GetEntity()->GetName(), seeWhenObscuredVar ? "SeeWhenObsured" : "Not SeeWhenObsured", g_pGame->GetPlayerVisTable()->CanLocalPlayerSee(pEntity->GetId(), 5) ? "Can See" : "Can Not See");
			}
		}
		else
		{
			DEBUG_TAGNAMES("\t Skipping %s - Failed GetTagNamePosition (Not On Screen)", pActor->GetEntity()->GetName());
		}
	}

	ZSort();
}

void CUITagNames::Draw(void) const
{
	CUIElement::Draw();

	C2DRenderUtils* m_renderUtils = g_pGame->GetHUD()->Get2DRenderUtils();
	m_renderUtils->SetFont(m_font);

	ScreenLayoutManager* pLayoutManager = g_pGame->GetHUD()->GetLayoutManager();
	CHUDCVars* pCvars = g_pGame->GetHUD()->GetCVars();

	pLayoutManager->SetState(eSLO_ScaleMethod_None|eSLO_DoNotAdaptToSafeArea);

	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	if (pClientActor && m_alpha>=1.0f)
	{
		CRY_ASSERT(pClientActor->IsPlayer());
		CPlayer * pClientPlayer = static_cast<CPlayer*>(pClientActor);
		float fMinDistance = pCvars->hud_tagNames_healthBar_nearDistance;
		float fMaxDistance = pCvars->hud_tagNames_healthBar_farDistance;
		float fHealthDistance = (fMinDistance + fMaxDistance) * 0.5f;

		bool drawStatusForFriends = pClientPlayer->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Tactical;
		bool drawStatusForEnemies = drawStatusForFriends && pClientPlayer->IsPerkActive(ePerk_EnemyStatus);

		IActorSystem * actorSystem = g_pGame->GetIGameFramework()->GetIActorSystem();

		for(int i = 0; i < m_tagsCount; i++)
		{
			const STagInfo * tag = & m_tagInfo[m_zsortedTagInfos[i]];
			EntityId entityId = tag->m_entityId;
			IActor * pActor = actorSystem->GetActor(entityId);

			if(pActor)
			{
				IEntity* pEntity = pActor->GetEntity();
				CRY_ASSERT(pEntity);

				int myHealth = pActor->GetHealth();
				bool friendly = tag->m_friendly;
				bool dead = myHealth <= 0;
				string displayBefore = "";
				bool displayStatus = !dead && (friendly ? drawStatusForFriends : drawStatusForEnemies);

				if (displayStatus)
				{
					IItem * usingItem = pActor->GetCurrentItem();
					IWeapon * usingWeapon = usingItem ? usingItem->GetIWeapon() : NULL;
					if (usingWeapon && usingWeapon->IsReloading())
					{
						displayBefore = "[RELOADING] ";
					}
				}

				DEBUG_TAGNAMES("Drawing - %s %s (%fm) (font size = %f)", displayBefore.c_str(), pEntity->GetName(), tag->m_distance, tag->m_size);

				const ColorF& textCol = dead ? COLOR_DEAD : (friendly ? COLOR_FRIEND : COLOR_ENEMY);
				m_renderUtils->DrawText( tag->m_pos.x, tag->m_pos.y, tag->m_size, tag->m_size, displayBefore + pEntity->GetName(), textCol, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_BOTTOM );

				if (displayStatus)
				{
					if (tag->m_distance < fHealthDistance && pCvars->hud_tagNames_healthBar_numBlocks > 0)
					{
						int numBlocks = min(pCvars->hud_tagNames_healthBar_numBlocks, MAX_TAGNAME_HEALTHBAR_NUMBLOCKS);
						float blockSizeX = tag->m_size * pCvars->hud_tagNames_healthBar_blockSizeX;
						float blockSizeY = tag->m_size * pCvars->hud_tagNames_healthBar_blockSizeY;

						float healthFraction = myHealth / (float) pActor->GetMaxHealth();
						healthFraction = clamp(healthFraction, 0.f, 1.f);
						float greenFraction = friendly ? 1.0f : healthFraction * healthFraction;
						ColorF healthColor(1.0f - greenFraction, greenFraction, 0.0f, 1.f - tag->m_distance / fHealthDistance);
						char szHealthText[MAX_TAGNAME_HEALTHBAR_NUMBLOCKS + 3];
						int numCharJustWritten = -1;
						bool bright = true;

						for(int i = 0; i < numBlocks; ++ i)
						{
							if (bright && i >= healthFraction * numBlocks)
							{
								// Add $0 to the string
								szHealthText[++ numCharJustWritten] = '$';
								szHealthText[++ numCharJustWritten] = '0';
								bright = false;
							}
							szHealthText[++ numCharJustWritten] = '.';
						}
						szHealthText[++ numCharJustWritten] = '\0';
						assert(numCharJustWritten < sizeof(szHealthText));

						HudDbgDisplayForEntity(entityId, "Drawing health blocks %s$o %.2f (size %.2f %.2f)", szHealthText, healthFraction, blockSizeX, blockSizeY);
						m_renderUtils->DrawText( tag->m_pos.x, tag->m_pos.y - pCvars->hud_tagNames_healthBar_blockPositionOffsetY * tag->m_size, blockSizeX, blockSizeY, szHealthText, healthColor, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_BOTTOM);
					}
				}
			}
			else
			{
				DEBUG_TAGNAMES("Not Drawing - Failed to find Actor");
			}
		}
	}
	else
	{
		DEBUG_TAGNAMES("Not Drawing - Failed to find Client || Disabled");
	}

	return;
}

void CUITagNames::IgnoreEntity( EntityId entity_id, bool ignore, float time )
{
	int useIndex = -1;
	for(int i=0; i<HUD_MAX_TAGNAMES; i++)
	{
		if( m_ignoreEnts[i].m_entityId == entity_id )
		{
			useIndex = i;
			break;
		}
		else
		{
			// record a new place to store ignore.
			if( useIndex < 0 && m_ignoreEnts[i].m_entityId == 0 )
			{
				useIndex = i;
			}
		}
	}		

	if( ignore )
	{
		assert( useIndex >= 0 );
		m_ignoreEnts[useIndex].m_entityId = entity_id;
		if(time < 0.f || m_ignoreEnts[useIndex].m_ignoreForever)
		{
			m_ignoreEnts[useIndex].m_time = k_ignoreEntityForever;
			m_ignoreEnts[useIndex].m_ignoreForever = true;
		}
		else
		{
			m_ignoreEnts[useIndex].m_time = max(m_ignoreEnts[useIndex].m_time, time);
			m_ignoreEnts[useIndex].m_ignoreForever = false;
		}
	}
	else 
	{
		if( time < 0 )
		{
			if(m_ignoreEnts[useIndex].m_entityId)
			{
				// stop ignoring
				m_ignoreEnts[useIndex].m_entityId = 0;
				m_ignoreEnts[useIndex].m_time = 0.0f;
				m_ignoreEnts[useIndex].m_ignoreForever = false;
			}
		}
		else
		{
			m_ignoreEnts[useIndex].m_entityId = entity_id;
			m_ignoreEnts[useIndex].m_time = -1.0f;
			m_ignoreEnts[useIndex].m_time_dontIgnore = time;
			m_ignoreEnts[useIndex].m_ignoreForever = false;
		}
	}
}


CUITagNames::SIgnoreEnts::eIE_IgnoreState CUITagNames::ShouldIgnoreEntity( EntityId entity_id ) const
{
	for(int i=0; i<HUD_MAX_TAGNAMES; i++)
	{
		if( m_ignoreEnts[i].m_entityId == entity_id )
		{
			// Yes we should ignore.
			if( (m_ignoreEnts[i].m_time > 0.0f || m_ignoreEnts[i].m_ignoreForever))
				return SIgnoreEnts::eIEIS_ignore;
			else if( m_ignoreEnts[i].m_time_dontIgnore )
				return SIgnoreEnts::eIEIS_forceShow;
		}
	}

	return SIgnoreEnts::eIEIS_dontIgnore;
}

void CUITagNames::UpdateIgnoreEntities(float frameTime)
{
	for(int i=0; i<HUD_MAX_TAGNAMES; i++)
	{
		if(m_ignoreEnts[i].m_entityId != 0)
		{
			if(!m_ignoreEnts[i].m_ignoreForever)
			{
				if( m_ignoreEnts[i].m_time_dontIgnore > 0 )
				{
					m_ignoreEnts[i].m_time_dontIgnore -= frameTime;				
				}
				else
				{
					m_ignoreEnts[i].m_time -= frameTime;
					if(m_ignoreEnts[i].m_time <= 0.0f)
					{
						m_ignoreEnts[i].m_entityId = 0;
						m_ignoreEnts[i].m_time = 0.0f;
						m_ignoreEnts[i].m_ignoreForever = false;
					}
					HudDbgDisplayForEntity(m_ignoreEnts[i].m_entityId, "UITagName - hidden for %.2f", m_ignoreEnts[i].m_time);
				}
			}
#if !defined(_RELEASE)
			else
			{
				HudDbgDisplayForEntity(m_ignoreEnts[i].m_entityId, "UITagName - ignoreForever");
				assert(m_ignoreEnts[i].m_time == k_ignoreEntityForever);
			}
#endif
		}
	}
}

void CUITagNames::OnHUDEvent( const SHUDEvent& event )
{
	switch( event.eventType )
	{
	case eHUDEvent_OnLookAtChanged :
		{
			m_lookAt = (EntityId)event.GetData(0).GetInt();
			switch( CHUD_Crosshair::GetLookAtType(m_lookAt) )
			{
			case CHUD_Crosshair::eTT_Enemy :
				if( m_enemyToLockOnTo != m_lookAt )
				{
					// looking at some-one/thing else
					m_enemyToLockOnTo = m_lookAt;
					m_timeLockedOnEnemy = g_pGame->GetHUD()->GetCVars()->hud_mpTagNames_LockDuration;
				}
				break;
			default :
				m_timeLockedOnEnemy = -1.0f;
				m_enemyToLockOnTo = 0;
				// not looking at an enemy allow timer reduction.
				break;
			}
		}
		break;
	case eHUDEvent_OnIgnoreEntity:
		{
			const EntityId entityId = static_cast<EntityId>(event.GetData(0).GetInt());
			float timeToIgnore = k_ignoreEntityForever;
			if( event.GetDataSize() > 1 )
			{
				timeToIgnore = event.GetData(1).GetFloat();
			}
			IgnoreEntity(entityId, true, timeToIgnore);
		}
		break;
	case eHUDEvent_OnStopIgnoringEntity:
		{
			const EntityId entityId = static_cast<EntityId>(event.GetData(0).GetInt());
			IgnoreEntity(entityId, false, k_ignoreEntityForever);
		}
		break;
	case eHUDEvent_OnEndBlind :
	case eHUDEvent_OnSpawn :
		m_alpha = 1.0f;
		break;
	default: 
		assert( event.eventType == eHUDEvent_OnBlind );
		//don't want to alpha out the map screen
		m_alpha = event.GetData(0).GetFloat();
		break;
	}
}

void CUITagNames::ZSort( void )
{
	// z sort
	memset(&m_zsortedTagInfos, 0, sizeof(m_zsortedTagInfos));
	m_zsortedTagInfos[0] = 0;//&m_tagInfo[0];

	const int tagsCount = m_tagsCount;
	for(int i = 0; i < tagsCount; i++)
	{
		const float di = m_tagInfo[i].m_distance;
		m_zsortedTagInfos[i] = i;
		for(int j = 0; j < i; j++)
		{
			const float dj = m_tagInfo[j].m_distance;
			if( di > dj )
			{
				// shift j down 1
				memmove((void*)&m_zsortedTagInfos[j+1],(void*)&m_zsortedTagInfos[j], (i-j)*sizeof(m_zsortedTagInfos[0]) );
				// insert i at j
				m_zsortedTagInfos[j] = i;
				break;
			}
		}
	}
}

bool GetTagNamePositionWorld(IEntity* pEntity, Vec3* pOut_WorldSpace, float* pOut_Distance )
{
	assert(pEntity);

	ICharacterInstance *pCharacterInstance = pEntity->GetCharacter(0);
	if(!pCharacterInstance)
		return false;

	ISkeletonPose *pSkeletonPose = pCharacterInstance->GetISkeletonPose();
	if(!pSkeletonPose)
		return false;

	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
	if( pActor && ( pActor->IsDead() ) )
	{
		AABB entAABB;
		pEntity->GetWorldBounds(entAABB);
		pOut_WorldSpace->z = entAABB.max.z;
		pOut_WorldSpace->x = entAABB.min.x + ( 0.5f*(entAABB.max.x - entAABB.min.x) );
		pOut_WorldSpace->y = entAABB.min.y + ( 0.5f*(entAABB.max.y - entAABB.min.y) );
	}
	else
	{
		static int16 s_sHeadId = -1;
#if ENABLE_HUD_EXTRA_DEBUG
		static int s_nextCheckFrame = gEnv->pRenderer->GetFrameID(false) + 10;
#endif
		if( s_sHeadId < 0 )
		{
			const int16 sHeadID = pSkeletonPose->GetJointIDByName("Bip01 Head");
			if(-1 == sHeadID)
				return false;
			s_sHeadId = sHeadID;
		}
#if ENABLE_HUD_EXTRA_DEBUG
		else
		{
			int curFrameId = gEnv->pRenderer->GetFrameID(false);
			if( s_nextCheckFrame < curFrameId )
			{
				CRY_ASSERT_MESSAGE( s_sHeadId>0, "HUD: Tagnames: No Head bone found! Code will need an update." );
				CRY_ASSERT_MESSAGE( s_sHeadId == pSkeletonPose->GetJointIDByName("Bip01 Head"), "HUD: Tagnames: New head bone ID found! Code will need an update." );
				s_nextCheckFrame = curFrameId + 10;
			}
		}
#endif

		Matrix34 matWorld = pEntity->GetWorldTM() * Matrix34(pSkeletonPose->GetAbsJointByID(s_sHeadId));

		*pOut_WorldSpace = matWorld.GetTranslation();
	}

	float fDistance = ( (*pOut_WorldSpace)-gEnv->pSystem->GetViewCamera().GetPosition()).len();

	// Adjust distance when zoomed. Default fov is 60, so we use (1/(60*pi/180)=3/pi)
	fDistance *= 3.0f * gEnv->pRenderer->GetCamera().GetFov() / gf_PI;

	*pOut_Distance = fDistance;

	// Offset in Z
	CHUDCVars* pCvars = g_pGame->GetHUD()->GetCVars();

	float zOffset = 0.0f;

	const float fMinDistance = pCvars->hud_mpTagNames_ZOffset_nearDist;

	if(fDistance < fMinDistance)
	{
		const float fMinOffset = pCvars->hud_mpTagNames_ZOffset_nearOffset;
		zOffset = fMinOffset;
	}
	else 
	{
		const float fMidDistance = pCvars->hud_mpTagNames_ZOffset_midDist;

		if(fDistance < fMidDistance)
		{
			const float fMinOffset = pCvars->hud_mpTagNames_ZOffset_nearOffset;
			const float fMidOffset = pCvars->hud_mpTagNames_ZOffset_midOffset;
			const float nearMidNormDist = (fDistance - fMinDistance) / (fMidDistance - fMinDistance);
			zOffset = fMinOffset + (fMidOffset-fMinOffset)*nearMidNormDist;
		}
		else 
		{
			const float fMaxDistance = pCvars->hud_mpTagNames_ZOffset_farDist;

			if(fDistance < fMaxDistance)
			{
				const float fMidOffset = pCvars->hud_mpTagNames_ZOffset_midOffset;
				const float fMaxOffset = pCvars->hud_mpTagNames_ZOffset_farOffset;
				const float midFarNormDist = (fDistance - fMidDistance) / (fMaxDistance - fMidDistance);
				zOffset = fMidOffset + (fMaxOffset-fMidOffset)*midFarNormDist;
			}
			else
			{
				return false;
			}
		}
	}

	pOut_WorldSpace->z += zOffset;

	//DEBUG_TAGNAMES( "ZOffset: %f", zOffset );

	return true;
}


bool GetTagNamePositionScreen(IEntity* pEntity, Vec3* pOut_ScreenSpace, float* pOut_Distance )
{
	Vec3 vWorldPos(0.f,0.f,0.f);

	if( !GetTagNamePositionWorld( pEntity, &vWorldPos, pOut_Distance ) )
		return false;

	//Needs to be done outside of the PreRender/PostRender to work
	gEnv->pRenderer->ProjectToScreen(vWorldPos.x, vWorldPos.y, vWorldPos.z, &pOut_ScreenSpace->x,&pOut_ScreenSpace->y,&pOut_ScreenSpace->z);

	ScreenLayoutManager* pLayoutManager = g_pGame->GetHUD()->GetLayoutManager();
	float virtualWidth = pLayoutManager->GetVirtualWidth();
	float virtualHeight = pLayoutManager->GetVirtualHeight();

	pOut_ScreenSpace->x *= virtualWidth * 0.01f;
	pOut_ScreenSpace->y *= virtualHeight * 0.01f;
	return true;
}

bool GetTagNamePosition(IEntity* pEntity, Vec3* pOut_ScreenSpace, float* pOut_Size, float* pOut_Distance)
{
	Vec3 vScreenSpace(0.f,0.f,0.f);

	if( !GetTagNamePositionScreen( pEntity, &vScreenSpace, pOut_Distance ) )
		return false;

	//things outside the range and typically behind you
	if(vScreenSpace.z > 1.0f || vScreenSpace.z < -1.0f)
	{
		return false;
	}

	pOut_ScreenSpace->Set(vScreenSpace.x, vScreenSpace.y, vScreenSpace.z);

	ScreenLayoutManager* pScreenLayout = g_pGame->GetHUD()->GetLayoutManager();
	float virtualWidth = pScreenLayout->GetVirtualWidth();
	float virtualHeight = pScreenLayout->GetVirtualHeight();

	if (pOut_ScreenSpace->x > 0.0f && pOut_ScreenSpace->y > 0.0f && pOut_ScreenSpace->x < virtualWidth && pOut_ScreenSpace->y < virtualHeight)
	{
		CHUDCVars* pCvars = g_pGame->GetHUD()->GetCVars();

		const float fDistance = *pOut_Distance;
		float fSize = 0.0f;

		const float fMinDistance = pCvars->hud_mpTagNames_MinFontSize_nearDist;
		const float fMinSize = pCvars->hud_mpTagNames_MinFontSize_nearSize;

		if(fDistance < fMinDistance)
		{
			fSize = fMinSize;
		}
		else 
		{
			const float fMidDistance = pCvars->hud_mpTagNames_MinFontSize_midDist;
			const float fMidSize = pCvars->hud_mpTagNames_MinFontSize_midSize;

			if(fDistance < fMidDistance)
			{
				const float nearMidNormDist = (fDistance - fMinDistance) / (fMidDistance - fMinDistance);
				fSize = fMinSize + (fMidSize-fMinSize)*nearMidNormDist;
			}
			else 
			{
				const float fMaxDistance = pCvars->hud_mpTagNames_MinFontSize_farDist;

				if(fDistance < fMaxDistance)
				{
					const float fMaxSize = pCvars->hud_mpTagNames_MinFontSize_farSize;
					const float midFarNormDist = (fDistance - fMidDistance) / (fMaxDistance - fMidDistance);
					fSize = fMidSize + (fMaxSize-fMidSize)*midFarNormDist;
				}
				else
				{
					return false;
				}
			}
		}

		*pOut_Size = fSize;

		return true;
	}

	return true;
}
