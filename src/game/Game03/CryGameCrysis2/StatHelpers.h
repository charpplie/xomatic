#ifndef		__STATHELPERS_H__
#define		__STATHELPERS_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IGameStatistics.h>

struct IStatsTracker;
struct IActor;
class CActor;

class CStatHelpers
{
public:
	static bool SaveActorArmor( CActor* pActor );
	static bool SaveActorStats( CActor* pActor, XmlNodeRef stats );
	static bool SaveActorWeapons( CActor* pActor );
	static bool SaveActorAttachments( CActor* pActor );
	static bool SaveActorAmmos( CActor* pActor );
	static int GetProfileId( int channelId );
	static int GetChannelId( int profileId );
	static EntityId GetEntityId( int profileId );

private:
	static IActor* GetProfileActor( int profileId );
};

//////////////////////////////////////////////////////////////////////////

class CPositionStats : public CXMLSerializableBase
{
public:

	CPositionStats(const Vec3& pos, float elevation);
	virtual XmlNodeRef GetXML(IGameStatistics* pGS);
	virtual void GetMemoryStatistics(ICrySizer* pSizer) const;

	Vec3 m_pos;
	float m_elev;
};

//////////////////////////////////////////////////////////////////////////

class CLookDirStats : public CXMLSerializableBase
{
public:

	CLookDirStats(const Vec3& dir);
	virtual XmlNodeRef GetXML(IGameStatistics* pGS);
	virtual void GetMemoryStatistics(ICrySizer* pSizer) const;

	Vec3 m_dir;
};

//////////////////////////////////////////////////////////////////////////

class CShotFiredStats : public CXMLSerializableBase
{
public:

	CShotFiredStats(EntityId projectileId, int ammo_left, const char* ammo_type);
	virtual XmlNodeRef GetXML(IGameStatistics* pGS);
	virtual void GetMemoryStatistics(ICrySizer* pSizer) const;

	EntityId m_projectileId;
	int m_ammoLeft;
	CCryName m_ammoType;
};

//////////////////////////////////////////////////////////////////////////

class CKillStats : public CXMLSerializableBase
{
public:

	CKillStats(EntityId projectileId, EntityId killerId, const char* hit_type);
	virtual XmlNodeRef GetXML(IGameStatistics* pGS);
	virtual void GetMemoryStatistics(ICrySizer* pSizer) const;

	EntityId m_projectileId;
	EntityId m_killerId;
	CCryName m_hitType;
};

//////////////////////////////////////////////////////////////////////////

class CDeathStats : public CXMLSerializableBase
{
public:

	CDeathStats(EntityId projectileId, EntityId targetId, const char* hit_type);
	virtual XmlNodeRef GetXML(IGameStatistics* pGS);
	virtual void GetMemoryStatistics(ICrySizer* pSizer) const;

	EntityId m_projectileId;
	EntityId m_targetId;
	CCryName m_hitType;
};

//////////////////////////////////////////////////////////////////////////

class CHitStats : public CXMLSerializableBase
{
public:
	CHitStats(EntityId projectileId, EntityId targetId, float damage, const char* hit_type, const char* weapon_class, const char* hit_part);
	virtual XmlNodeRef GetXML(IGameStatistics* pGS);
	virtual void GetMemoryStatistics(ICrySizer* pSizer) const;

	EntityId m_projectileId;
	EntityId m_targetId;
	float m_damage;
	CCryName m_hitType;
	CCryName m_weaponClass;
	CCryName m_hitPart;
};

#endif
