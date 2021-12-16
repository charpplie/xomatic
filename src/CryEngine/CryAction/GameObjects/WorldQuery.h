#ifndef __WORLDQUERY_H__
#define __WORLDQUERY_H__

#pragma once

#include "IWorldQuery.h"

#define WORLDQUERY_USE_DEFERRED_LINETESTS		1

#if WORLDQUERY_USE_DEFERRED_LINETESTS
#include "IDeferredRaycastManager.h"
#endif

struct IActor;
struct IViewSystem;

#if WORLDQUERY_USE_DEFERRED_LINETESTS
class CWorldQuery : public CGameObjectExtensionHelper<CWorldQuery, IWorldQuery>, public IDeferredRaycastReceiver
#else
class CWorldQuery : public CGameObjectExtensionHelper<CWorldQuery, IWorldQuery>
#endif
{
public:
	CWorldQuery();
	~CWorldQuery();

	// IGameObjectExtension
	virtual bool Init( IGameObject * pGameObject );
	virtual void InitClient(int channelId) {};
	virtual void PostInit( IGameObject * pGameObject );
	virtual void PostInitClient(int channelId) {};
	virtual void Release();
	virtual void FullSerialize( TSerialize ser );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags ) { return true; }
	virtual void PostSerialize() {}
	virtual void SerializeSpawnInfo( TSerialize ser ) {}
	virtual ISerializableInfoPtr GetSpawnInfo() {return 0;}
	virtual void Update( SEntityUpdateContext& ctx, int slot );
	virtual void HandleEvent( const SGameObjectEvent& );
	virtual void ProcessEvent(SEntityEvent& ) {}
	virtual void SetChannelId(uint16 id) {};
	virtual void SetAuthority(bool auth) {}
	virtual void PostUpdate(float frameTime) { CRY_ASSERT(false); }
	virtual void PostRemoteSpawn() {};
	virtual void GetMemoryStatistics(ICrySizer * s);
	// ~IGameObjectExtension

	ILINE void SetProximityRadius( float n )
	{
		m_proximityRadius = n;
		m_validQueries &= ~(eWQ_Proximity | eWQ_InFrontOf);
	}

	ILINE const ray_hit* RaycastQuery()
	{
		//ValidateQuery(eWQ_Raycast);
		//return m_rayHitAny? &m_rayHit : NULL;
		return GetLookAtPoint(m_proximityRadius);
	}

	ILINE const ray_hit* GetLookAtPoint(const float fMaxDist = 0)
	{
    ValidateQuery(eWQ_Raycast);
		if(m_rayHitAny)
			if(fMaxDist > 0 && m_rayHit.dist <= fMaxDist)
				return &m_rayHit;
		return NULL;
	}

  ILINE const ray_hit* GetBehindPoint(const float fMaxDist = 0)
  {
    ValidateQuery(eWQ_BackRaycast);
    if(m_backRayHitAny)
      if(fMaxDist > 0 && m_backRayHit.dist <= fMaxDist)
        return &m_backRayHit;
    return NULL;
  }

	ILINE const EntityId GetLookAtEntityId()
	{
    ValidateQuery(eWQ_Raycast);
		return m_lookAtEntityId;
	}

	ILINE const Entities& ProximityQuery()
	{
		ValidateQuery(eWQ_Proximity);
		return m_proximity;
	}

	ILINE const Entities& InFrontOfQuery()
	{
		ValidateQuery(eWQ_InFrontOf);
		return m_inFrontOf;
	}
	ILINE IEntity * GetEntityInFrontOf()
	{
		const Entities& entities = InFrontOfQuery();
		if (entities.empty())
			return NULL;
		else
			return m_pEntitySystem->GetEntity( entities[0] );
	}
	ILINE const Entities &GetEntitiesInFrontOf()
	{
		return InFrontOfQuery();
	}

	ILINE const EntityId* GetEntitiesAround(int& num)
	{
		ValidateQuery(eWQ_EntitiesAround);
		num = m_EntAroundOf.size();
		return num?&m_EntAroundOf[0]:0;
	}

	ILINE IPhysicalEntity*const* GetPhysicalEntitiesAround(int& num)
	{
		ValidateQuery(eWQ_PhysicalEntitiesAround);
		num = m_PhysEntAroundOf.size();
		return num?&m_PhysEntAroundOf[0]:0;
	}

	ILINE IPhysicalEntity* GetPhysicalEntityInFrontOf()
	{
		ValidateQuery(eWQ_PhysicalEntitiesInFrontOf);

		return gEnv->pPhysicalWorld->GetPhysicalEntityById(m_physicalEntityInFrontOf);
	}

	ILINE const Vec3& GetPos() const { return m_worldPosition; }
	ILINE const Vec3& GetDir() const { return m_dir; }

#if WORLDQUERY_USE_DEFERRED_LINETESTS
		//IDeferredRaycastReceiver
		//async raycasts results callback
		virtual void OnDataReceived(const EventPhysRWIResult *pRWIResult);

		//async primitive casts results callback
		virtual void OnDataReceived(const EventPhysPWIResult *pPWIResult) {}

		//reset data callback
		virtual void OnDRWReset() {}
		//~IDeferredRaycastReceiver
#endif

private:
	uint32  m_validQueries;
  int     m_renderFrameId;

	float m_proximityRadius;

	// keep in sync with m_updateQueryFunctions
	enum EWorldQuery
	{
		eWQ_Raycast = 0,
		eWQ_Proximity,
		eWQ_InFrontOf,
		eWQ_BackRaycast,
		eWQ_EntitiesAround,
		eWQ_PhysicalEntitiesAround,
		eWQ_PhysicalEntitiesInFrontOf,
	};
	typedef void (CWorldQuery::*UpdateQueryFunction)();

	Vec3 m_worldPosition;
	Vec3 m_dir;
	IActor * m_pActor;
	static UpdateQueryFunction m_updateQueryFunctions[];

	IPhysicalWorld * m_pPhysWorld;
	IEntitySystem * m_pEntitySystem;
	IViewSystem * m_pViewSystem;

#if WORLDQUERY_USE_DEFERRED_LINETESTS
	CDeferredRaycastHelper	m_raycastHelper;
	float m_timeLastDeferredResult;
#endif

	// ray-cast query
	bool m_rayHitAny;
	ray_hit m_rayHit;

  // back raycast query
  bool m_backRayHitAny;
  ray_hit m_backRayHit;

	//the entity the object is currently looking at...
	EntityId m_lookAtEntityId;

	// proximity query
	Entities m_proximity;
	// "in-front-of" query
	Entities m_inFrontOf;

	Entities m_EntAroundOf;
	std::vector<IPhysicalEntity*> m_PhysEntAroundOf;

	int m_physicalEntityInFrontOf;
  
	ILINE void ValidateQuery( EWorldQuery query )
	{
    uint32 queryMask = 1u << query;
    
    int frameid = gEnv->pRenderer->GetFrameID();
    if(m_renderFrameId != frameid)
    {
      m_renderFrameId = frameid;
      m_validQueries = 0;
    }else
    {
		  if (m_validQueries & queryMask)
			  return;
    }
		(this->*(m_updateQueryFunctions[query]))();
		m_validQueries |= queryMask;
	}

	void UpdateRaycastQuery();
	void UpdateProximityQuery();
	void UpdateInFrontOfQuery();
	void UpdateBackRaycastQuery();
	void UpdateEntitiesAround();
	void UpdatePhysicalEntitiesAround();
	void UpdatePhysicalEntityInFrontOf();
};

#endif
