#include "StdAfx.h"
#include "IGameStatistics.h"
#include "StatHelpers.h"

#include "Game.h"
#include "Actor.h"
#include "GameRules.h"

#if 0	// MT : not imported from kiev
#include "ActorInventory.h"
#include "CharacterEquip.h"
#include "WeaponGameObject.h"
#include "Attachment.h"

bool CStatHelpers::SaveActorArmor( CActor* pActor )
{
	if ( IStatsTracker *tracker = pActor->GetStatsTracker() )
		if ( const CCharacterEquip *pEquip = pActor->GetCharacterEquip() )
		{
			XmlNodeRef resNode = GetISystem()->GetXmlUtils()->CreateStatsXmlNode(MULTIPLE_STAT_XML_TAG);
			for ( int i = 0; i < pEquip->GetItemCount(); ++i )
			{
				XmlNodeRef childNode = GetISystem()->CreateXmlNode("part");
				childNode->setAttr( "name", pEquip->GetItem( i ) );
				childNode->setAttr( "amount", 1 );

				resNode->addChild( childNode );
			}	

			tracker->StateValue( eSP_Armor, resNode );
			return true;
		}
		return false;
}

bool CStatHelpers::SaveActorStats( CActor* pActor, XmlNodeRef stats )
{
	if ( IStatsTracker *tracker = pActor->GetStatsTracker() )
	{
		tracker->StateValue(eSP_PlayerInfo, stats);
		return true;
	}
	return false;
}

bool CStatHelpers::SaveActorWeapons( CActor* pActor )
{
	if ( IStatsTracker *tracker = pActor->GetStatsTracker() )
		if ( const CActorInventory *pInv = pActor->GetActorInventory() )
		{
			XmlNodeRef resNode = GetISystem()->GetXmlUtils()->CreateStatsXmlNode(MULTIPLE_STAT_XML_TAG);
			for ( int i = 0; i < pInv->GetWeaponsCount() ; ++i )
			{
				XmlNodeRef childNode = GetISystem()->CreateXmlNode("weapon");
				childNode->setAttr( "name", pInv->GetWeapon( i )->GetName() );
				childNode->setAttr( "amount", 1 );

				resNode->addChild( childNode );
			}	

			tracker->StateValue( eSP_Weapons, resNode );
			return true;
		}
	return false;
}

bool CStatHelpers::SaveActorAttachments( CActor* pActor )
{
	if ( IStatsTracker *tracker = pActor->GetStatsTracker() )
		if ( const CActorInventory *pInv = pActor->GetActorInventory() )
		{
			XmlNodeRef resNode = GetISystem()->GetXmlUtils()->CreateStatsXmlNode(MULTIPLE_STAT_XML_TAG);
			for ( int i = 0; i < pInv->GetAttachmentsCount() ; ++i )
			{
				XmlNodeRef childNode = GetISystem()->CreateXmlNode("attachment");
				childNode->setAttr( "name", pInv->GetAttachment( i )->GetName() );
				childNode->setAttr( "amount", 1 );

				resNode->addChild( childNode );
			}	

			tracker->StateValue( eSP_Attachments, resNode );
			return true;
		}
	return false;
}

bool CStatHelpers::SaveActorAmmos( CActor* pActor )
{
	if ( IStatsTracker *tracker = pActor->GetStatsTracker() )
		if ( const CActorInventory *pInv = pActor->GetActorInventory() )
		{
			XmlNodeRef resNode = GetISystem()->GetXmlUtils()->CreateStatsXmlNode(MULTIPLE_STAT_XML_TAG);
			for ( int i = 0; i < pInv->GetAmmoTypesCount(); ++i )
				if ( IEntityClass* pEC = pInv->GetAmmoType( i ) )
				{
					XmlNodeRef childNode = GetISystem()->CreateXmlNode("ammo");
					childNode->setAttr( "name", pEC->GetName() );
					childNode->setAttr( "amount", pInv->GetAmmoCount( pEC ) );

					resNode->addChild( childNode );
				}	

			tracker->StateValue( eSP_Ammos, resNode );
			return true;
		}
	return false;
}
#endif

int CStatHelpers::GetProfileId( int channelId )
{
	int profileId = 0;
	if(INetChannel* pCh = g_pGame->GetIGameFramework()->GetNetChannel(channelId))
		profileId = pCh->GetProfileId();

	return profileId;
}

IActor* CStatHelpers::GetProfileActor( int profileId )
{
	IActorIteratorPtr aIt = g_pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
	while( IActor* pActor = aIt->Next() )
	{
		if ( pActor->IsPlayer() )
			if ( profileId == GetProfileId(pActor->GetChannelId()) )
				return pActor;
	}

	return 0;
}

int CStatHelpers::GetChannelId( int profileId )
{
	if ( IActor* pActor = GetProfileActor(profileId) )
		return pActor->GetChannelId();

	return 0;
}

EntityId CStatHelpers::GetEntityId( int profileId )
{
	if ( IActor* pActor = GetProfileActor(profileId) )
		return pActor->GetEntityId();

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// CPositionStats
//////////////////////////////////////////////////////////////////////////
CPositionStats::CPositionStats(const Vec3& pos, float elevation)
: m_pos(pos)
, m_elev(elevation)
{ }

//////////////////////////////////////////////////////////////////////////

XmlNodeRef CPositionStats::GetXML(IGameStatistics* pGS)
{
	XmlNodeRef node = pGS->CreateStatXMLNode();
	node->setAttr("x", m_pos.x);
	node->setAttr("y", m_pos.y);
	node->setAttr("z", m_pos.z);
	node->setAttr("g", m_elev);
	return node;
}

//////////////////////////////////////////////////////////////////////////

void CPositionStats::GetMemoryStatistics(ICrySizer *pSizer) const
{
	pSizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CLookDirStats
//////////////////////////////////////////////////////////////////////////

CLookDirStats::CLookDirStats(const Vec3& dir)
: m_dir(dir)
{ }

//////////////////////////////////////////////////////////////////////////

XmlNodeRef CLookDirStats::GetXML(IGameStatistics* pGS)
{
	XmlNodeRef node = pGS->CreateStatXMLNode();
	node->setAttr("x", m_dir.x);
	node->setAttr("y", m_dir.y);
	node->setAttr("z", m_dir.z);
	return node;
}

//////////////////////////////////////////////////////////////////////////

void CLookDirStats::GetMemoryStatistics(ICrySizer *pSizer) const
{
	pSizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// CShotFiredStats
//////////////////////////////////////////////////////////////////////////

CShotFiredStats::CShotFiredStats(EntityId projectileId, int ammo_left, const char* ammo_type)
: m_projectileId(projectileId), m_ammoLeft(ammo_left), m_ammoType(ammo_type)
{ }

//////////////////////////////////////////////////////////////////////////

XmlNodeRef CShotFiredStats::GetXML(IGameStatistics* pGS)
{
	XmlNodeRef node = pGS->CreateStatXMLNode();
	node->setAttr("projectile_id", m_projectileId);
	node->setAttr("ammo_left", m_ammoLeft);
	node->setAttr("ammo_type", m_ammoType.c_str());
	return node;
}

//////////////////////////////////////////////////////////////////////////

void CShotFiredStats::GetMemoryStatistics(ICrySizer *pSizer) const
{
	pSizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// CKillStats
//////////////////////////////////////////////////////////////////////////

CKillStats::CKillStats(EntityId projectileId, EntityId killerId, const char* hit_type)
: m_projectileId(projectileId), m_killerId(killerId), m_hitType(hit_type)
{ }

//////////////////////////////////////////////////////////////////////////

XmlNodeRef CKillStats::GetXML(IGameStatistics* pGS)
{
	XmlNodeRef node = pGS->CreateStatXMLNode();
	node->setAttr("projectile_id", m_projectileId);
	node->setAttr("killer_id", m_killerId);
	node->setAttr("hit_type", m_hitType.c_str());
	return node;
}

//////////////////////////////////////////////////////////////////////////

void CKillStats::GetMemoryStatistics(ICrySizer *pSizer) const
{
	pSizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// CDeathStats
//////////////////////////////////////////////////////////////////////////

CDeathStats::CDeathStats(EntityId projectileId, EntityId targetId, const char* hit_type)
: m_projectileId(projectileId), m_targetId(targetId), m_hitType(hit_type)
{ }

//////////////////////////////////////////////////////////////////////////

XmlNodeRef CDeathStats::GetXML(IGameStatistics* pGS)
{
	XmlNodeRef node = pGS->CreateStatXMLNode();
	node->setAttr("projectile_id", m_projectileId);
	node->setAttr("target_id", m_targetId);
	node->setAttr("hit_type", m_hitType.c_str());
	return node;
}

//////////////////////////////////////////////////////////////////////////

void CDeathStats::GetMemoryStatistics(ICrySizer *pSizer) const
{
	pSizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CHitStats
//////////////////////////////////////////////////////////////////////////

CHitStats::CHitStats(EntityId projectileId, EntityId targetId, float damage, const char* hit_type, const char* weapon_class, const char* hit_part)
: m_projectileId(projectileId), m_targetId(targetId), m_damage(damage), m_hitType(hit_type), m_weaponClass(weapon_class), m_hitPart(hit_part)
{ }

//////////////////////////////////////////////////////////////////////////

XmlNodeRef CHitStats::GetXML(IGameStatistics* pGS)
{
	XmlNodeRef node = pGS->CreateStatXMLNode();
	node->setAttr("projectile_id", m_projectileId);
	node->setAttr("target_id", m_targetId);
	node->setAttr("damage", m_damage);
	node->setAttr("weapon_class", m_weaponClass.c_str());
	node->setAttr("hit_type", m_hitType.c_str());
	node->setAttr("hit_part", m_hitPart.c_str());
	return node;
}

//////////////////////////////////////////////////////////////////////////

void CHitStats::GetMemoryStatistics(ICrySizer *pSizer) const
{
	pSizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////