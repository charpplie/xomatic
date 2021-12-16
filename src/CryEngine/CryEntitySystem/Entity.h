////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   Entity.h
//  Version:     v1.00
//  Created:     18/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Entity_h__
#define __Entity_h__
#pragma once

#include <IEntity.h>

//////////////////////////////////////////////////////////////////////////
// This is the order in which the proxies are serialized
const static EEntityProxy ProxySerializationOrder[] = { 	
	ENTITY_PROXY_RENDER,
	ENTITY_PROXY_SCRIPT,
	ENTITY_PROXY_SOUND,
	ENTITY_PROXY_AI,
	ENTITY_PROXY_AREA,
	ENTITY_PROXY_BOIDS,
	ENTITY_PROXY_BOID_OBJECT,
	ENTITY_PROXY_CAMERA,
	ENTITY_PROXY_FLOWGRAPH,
	ENTITY_PROXY_SUBSTITUTION,
	ENTITY_PROXY_TRIGGER,
	ENTITY_PROXY_USER,
	ENTITY_PROXY_PHYSICS,
	ENTITY_PROXY_ROPE,
	ENTITY_PROXY_LAST
};

//////////////////////////////////////////////////////////////////////////
// These headers cannot be replaced with forward references.
// They are needed for correct up casting from IEntityProxy to real proxy class.
#include "RenderProxy.h"
#include "PhysicsProxy.h"
#include "ScriptProxy.h"
#include "SubstitutionProxy.h"
//////////////////////////////////////////////////////////////////////////

// forward declarations.
struct IEntitySystem;
struct IEntityArchetype;
class  CEntitySystem;
struct AIObjectParameters;
struct SGridLocation;
struct SProximityElement;

// (MATT) This should really live in a minimal AI include, which right now we don't have  {2009/04/08}
#ifndef INVALID_AIOBJECTID
	typedef uint32	tAIObjectID;
	#define INVALID_AIOBJECTID ((tAIObjectID)(0))
#endif

//////////////////////////////////////////////////////////////////////
class CEntity : public IEntity
{
public:
	// Entity constructor.
	// Should only be called from Entity System.
	CEntity( CEntitySystem *pEntitySystem,SEntitySpawnParams &params );
	
	// Entity destructor.
	// Should only be called from Entity System.
	virtual ~CEntity();

	IEntitySystem* GetEntitySystem() const { return g_pIEntitySystem; };

	// Called by entity system to complete initialization of the entity.
	bool Init( SEntitySpawnParams &params );
	// Called by EntitySystem every frame for each pre-physics active entity.
	void PrePhysicsUpdate( float fFrameTime );
	// Called by EntitySystem every frame for each active entity.
	void Update( SEntityUpdateContext &ctx );
	// Called by EntitySystem before entity is destroyed.
	void ShutDown();

	//////////////////////////////////////////////////////////////////////////
	// IEntity interface implementation.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL EntityId GetId() const { return m_nID; };
	VIRTUAL EntityGUID GetGuid() const { return m_guid; };

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL IEntityClass* GetClass() const { return m_pClass; }
	VIRTUAL IEntityArchetype* GetArchetype() { return m_pArchetype; };

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetFlags( uint32 flags );
	VIRTUAL uint32 GetFlags() const { return m_flags; };
	VIRTUAL void AddFlags( uint32 flagsToAdd ) { SetFlags( m_flags|flagsToAdd); };
	VIRTUAL void ClearFlags( uint32 flagsToClear ) { SetFlags(m_flags&(~flagsToClear)); };
	VIRTUAL bool CheckFlags( uint32 flagsToCheck ) const { return (m_flags&flagsToCheck) == flagsToCheck; };

	VIRTUAL bool IsGarbage() const { return m_bGarbage; };

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetName( const char *sName );
	VIRTUAL const char* GetName() const { return m_szName.c_str(); };
	VIRTUAL const char* GetEntityTextDescription() const;

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void AttachChild( IEntity *pChildEntity,int nAttachFlags=0 );
	VIRTUAL void DetachAll( int nDetachFlags=0  );
	VIRTUAL void DetachThis( int nDetachFlags=0,int nWhyFlags=0 );
	VIRTUAL int  GetChildCount() const;
	VIRTUAL IEntity* GetChild( int nIndex ) const;
	virtual void EnableInheritXForm( bool bEnable );
	VIRTUAL IEntity* GetParent() const { return (m_pBinds) ? m_pBinds->pParent : NULL; };
	virtual IEntity* GetAdam()  
	{ 
		IEntity *pParent,*pAdam=this; 
		while(pParent=pAdam->GetParent()) pAdam=pParent; 
		return pAdam;
	}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetWorldTM( const Matrix34 &tm,int nWhyFlags=0 );
	VIRTUAL void SetLocalTM( const Matrix34 &tm,int nWhyFlags=0 );

	// Set position rotation and scale at once.
	VIRTUAL void SetPosRotScale( const Vec3 &vPos,const Quat &qRotation,const Vec3 &vScale,int nWhyFlags=0 );

	VIRTUAL Matrix34 GetLocalTM() const;
	VIRTUAL const Matrix34& GetWorldTM() const;

	VIRTUAL void GetWorldBounds( AABB &bbox );
	VIRTUAL void GetLocalBounds( AABB &bbox );

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetPos( const Vec3 &vPos,int nWhyFlags=0 );
	VIRTUAL const Vec3& GetPos() const { return m_vPos; }

	VIRTUAL void SetRotation( const Quat &qRotation,int nWhyFlags=0 );
	VIRTUAL const Quat& GetRotation() const { return m_qRotation; };

	VIRTUAL void SetScale( const Vec3 &vScale,int nWhyFlags=0 );
	VIRTUAL const Vec3& GetScale() const { return m_vScale; };

	VIRTUAL Vec3 GetWorldPos() const { return GetWorldTM().GetTranslation(); };
	VIRTUAL Ang3 GetWorldAngles() const;
	VIRTUAL Quat GetWorldRotation() const;
	VIRTUAL const Vec3& GetForwardDir() const { ComputeForwardDir(); return m_vForwardDir; }
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void Activate( bool bActive );
	VIRTUAL bool IsActive() const { return m_bActive; };

	VIRTUAL void PrePhysicsActivate( bool bActive );
	VIRTUAL bool IsPrePhysicsActive();

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void Serialize( TSerialize ser, int nFlags );

	VIRTUAL bool SendEvent( SEntityEvent &event );
	//////////////////////////////////////////////////////////////////////////
	
	VIRTUAL void SetTimer( int nTimerId,int nMilliSeconds );
	VIRTUAL void KillTimer( int nTimerId );

	VIRTUAL void Hide( bool bHide );
  VIRTUAL bool IsHidden() const { return m_bHidden; }

	VIRTUAL void Invisible( bool bInvisible );
  VIRTUAL bool IsInvisible() const { return m_bInvisible; }

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL CSmartObject* GetSmartObject() const { return m_pSmartObject; }
	VIRTUAL void SetSmartObject( CSmartObject* pSmartObject ) { m_pSmartObject = pSmartObject; }
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL IAIObject* GetAI() { return (m_aiObjectID ? GetAIObject() : NULL); };
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL bool RegisterInAISystem(unsigned short type, const AIObjectParameters &params);
	//////////////////////////////////////////////////////////////////////////
	// reflect changes in position or orientation to the AI object
	virtual void UpdateAIObject( );
	//////////////////////////////////////////////////////////////////////////

	VIRTUAL void SetUpdatePolicy( EEntityUpdatePolicy eUpdatePolicy );
	VIRTUAL EEntityUpdatePolicy GetUpdatePolicy() const { return (EEntityUpdatePolicy)m_eUpdatePolicy; }

	//////////////////////////////////////////////////////////////////////////
	// Entity Proxies Interfaces access functions.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL IEntityProxy* GetProxy( EEntityProxy proxy ) const;
	VIRTUAL void SetProxy(EEntityProxy proxy, IEntityProxy *pProxy);
	VIRTUAL IEntityProxy* CreateProxy( EEntityProxy proxy );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Physics.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void Physicalize( SEntityPhysicalizeParams &params );
	VIRTUAL void EnablePhysics( bool enable );
	VIRTUAL IPhysicalEntity* GetPhysics() const;

	VIRTUAL int PhysicalizeSlot(int slot, SEntityPhysicalizeParams &params);
	VIRTUAL void UnphysicalizeSlot(int slot);
	VIRTUAL void UpdateSlotPhysics(int slot);

	VIRTUAL void SetPhysicsState( XmlNodeRef & physicsState );

	//////////////////////////////////////////////////////////////////////////
	// Custom entity material.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetMaterial( IMaterial *pMaterial );
	VIRTUAL IMaterial* GetMaterial();

	//////////////////////////////////////////////////////////////////////////
	// Entity Slots interface.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL bool IsSlotValid( int nSlot ) const;
	VIRTUAL void FreeSlot( int nSlot );
	VIRTUAL int  GetSlotCount() const;
	VIRTUAL bool GetSlotInfo( int nSlot,SEntitySlotInfo &slotInfo ) const;
	VIRTUAL const Matrix34& GetSlotWorldTM( int nIndex ) const;
	VIRTUAL const Matrix34& GetSlotLocalTM( int nIndex, bool bRelativeToParent) const;
	VIRTUAL void SetSlotLocalTM( int nIndex,const Matrix34 &localTM,int nWhyFlags=0  );
	VIRTUAL bool SetParentSlot( int nParentSlot,int nChildSlot );
	VIRTUAL void SetSlotMaterial( int nSlot,IMaterial *pMaterial );
	VIRTUAL void SetSlotFlags( int nSlot,uint32 nFlags );
	VIRTUAL uint32 GetSlotFlags( int nSlot ) const;
	VIRTUAL bool ShouldUpdateCharacter( int nSlot) const;
	VIRTUAL ICharacterInstance* GetCharacter( int nSlot );
	VIRTUAL int SetCharacter( ICharacterInstance *pCharacter, int nSlot );
	VIRTUAL IStatObj* GetStatObj( int nSlot );
	VIRTUAL int SetStatObj( IStatObj *pStatObj, int nSlot, bool bUpdatePhysics, float mass=-1.0f );
	VIRTUAL IParticleEmitter* GetParticleEmitter( int nSlot );

	VIRTUAL int LoadGeometry( int nSlot,const char *sFilename,const char *sGeomName=NULL,int nLoadFlags=0 );
	VIRTUAL int LoadCharacter( int nSlot,const char *sFilename,int nLoadFlags=0 );
	VIRTUAL int SetParticleEmitter( int nSlot, IParticleEmitter* pEmitter, bool bSerialize = false );
	VIRTUAL int	LoadParticleEmitter( int nSlot, IParticleEffect *pEffect, SpawnParams const* params=NULL, bool bPrime = false, bool bSerialize = false );
	VIRTUAL int LoadLight( int nSlot,CDLight *pLight );
	VIRTUAL bool UpdateLightClipBounds(CDLight &light);
	VIRTUAL int LoadCloud( int nSlot,const char *sFilename );
	virtual int SetCloudMovementProperties(int nSlot, const SCloudMovementProperties& properties);
	VIRTUAL int LoadFogVolume( int nSlot, const SFogVolumeProperties& properties );
	virtual int FadeGlobalDensity( int nSlot, float fadeTime, float newGlobalDensity );
	virtual int LoadVolumeObject(int nSlot, const char* sFilename);

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
	virtual int LoadPrismObject(int nSlot);
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

	virtual int SetVolumeObjectMovementProperties(int nSlot, const SVolumeObjectMovementProperties& properties);

	VIRTUAL void InvalidateTM( int nWhyFlags=0 );

	// Load/Save entity parameters in XML node.
	VIRTUAL void SerializeXML( XmlNodeRef &node,bool bLoading );

	VIRTUAL IEntityLink* GetEntityLinks();
	VIRTUAL IEntityLink* AddEntityLink( const char *sLinkName,EntityId entityId );
	VIRTUAL void RemoveEntityLink( IEntityLink* pLink );
	VIRTUAL void RemoveAllEntityLinks();
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	VIRTUAL IEntity *UnmapAttachedChild(int &partId);

	VIRTUAL void GetMemoryUsage( ICrySizer *pSizer ) const;

	//////////////////////////////////////////////////////////////////////////
	// Local methods.
	//////////////////////////////////////////////////////////////////////////

	// Returns true if entity was already fully initialized by this point.
	VIRTUAL bool IsInitialized() const { return m_bInitialized; }

	VIRTUAL void DebugDraw( const SGeometryDebugDrawInfo &info );
	//////////////////////////////////////////////////////////////////////////



	// Get fast access to the slot, only used internally by entity components.
	class CEntityObject* GetSlot( int nSlot ) const;

	//////////////////////////////////////////////////////////////////////////
	// Return number of proxies in object.
	ILINE int ProxyCount() const
	{
		if (m_bUseProxyArray)
			return m_proxy.array.numProxies;
		else
			return 3;
	}
	// Return proxy object for proxy id.
	IEntityProxy* Proxy( int nProxy ) const
	{
		if (m_bUseProxyArray)
			return m_proxy.array.pProxyArray[nProxy];
		else
		{
			assert(nProxy>=ENTITY_PROXY_RENDER&&nProxy<=ENTITY_PROXY_SCRIPT);
			return m_proxy.simple.pProxyArray[nProxy];
		}
	}
	//////////////////////////////////////////////////////////////////////////
	// Version of Proxy function that can receive any proxy index, and will return 0 if this proxy not exist.
	ILINE IEntityProxy* GetProxyByType( EEntityProxy nProxyType ) const
	{
		switch (nProxyType) {
			case ENTITY_PROXY_RENDER: return GetRenderProxy();
			case ENTITY_PROXY_PHYSICS: return GetPhysicalProxy();
			case ENTITY_PROXY_SCRIPT: return GetScriptProxy();
		}
		for (int i = ENTITY_PROXY_SCRIPT+1; i < ProxyCount(); i++)
		{
			IEntityProxy *pProxy = GetProxyAt(i);
			if (pProxy && pProxy->GetType() == nProxyType)
				return pProxy;
		}
		return 0;
	}
	//////////////////////////////////////////////////////////////////////////
	// Proxy at specified index.
	//////////////////////////////////////////////////////////////////////////
	ILINE IEntityProxy* GetProxyAt( int nIndex ) const
	{
		if (!m_bUseProxyArray)
		{
			if (nIndex <= ENTITY_PROXY_SCRIPT)
				return m_proxy.simple.pProxyArray[nIndex];
			return 0;
		}
		else
		{
			if (nIndex >= m_proxy.array.numProxies)
				return 0;
			return m_proxy.array.pProxyArray[nIndex];
		}
	}
	//////////////////////////////////////////////////////////////////////////
	ILINE void SetProxyAt( int nIndex,IEntityProxy* pProxy )
	{
		if (!m_bUseProxyArray)
		{
			if (nIndex <= ENTITY_PROXY_SCRIPT)
				m_proxy.simple.pProxyArray[nIndex] = pProxy;
		}
		else
		{
			if (nIndex < m_proxy.array.numProxies)
				m_proxy.array.pProxyArray[nIndex] = pProxy;
		}
	}
	// Reallocate proxy array and add one slot.
	// Return new index.
	int AllocNewProxyIndex();
	// Get render proxy object.
	ILINE CRenderProxy* GetRenderProxy() const { return (CRenderProxy*)GetProxyAt(ENTITY_PROXY_RENDER) ; };
	// Get physics proxy object.
	ILINE CPhysicalProxy* GetPhysicalProxy() const { return (CPhysicalProxy*)GetProxyAt(ENTITY_PROXY_PHYSICS); };
	// Get script proxy object.
	ILINE CScriptProxy* GetScriptProxy() const { return (CScriptProxy*)GetProxyAt(ENTITY_PROXY_SCRIPT); };
	//////////////////////////////////////////////////////////////////////////

	// For internal use.
	CEntitySystem* GetCEntitySystem() const { return g_pIEntitySystem; };

	//////////////////////////////////////////////////////////////////////////
	// Activates entity only for specified number of frames.
	// numUpdates must be a small number from 0-15.
	void ActivateForNumUpdates( int numUpdates );
	void SetUpdateStatus();
	// Get status if entity need to be update every frame or not.
	bool GetUpdateStatus() const { return (m_bActive || m_nUpdateCounter) && (!m_bHidden || CheckFlags(ENTITY_FLAG_UPDATE_HIDDEN)); };

	//////////////////////////////////////////////////////////////////////////
	// Description:
	//   Invalidates entity and childs cached world transformation.
	void CalcLocalTM( Matrix34 &tm ) const;

	const Matrix34& GetWorldTM_Fast() const { return m_worldTM; };

	void	Hide(bool bHide, EEntityEvent eEvent1, EEntityEvent eEvent2);

	// for ProximityTriggerSystem
	SProximityElement * GetProximityElement() { return m_pProximityEntity; }

protected:

	//////////////////////////////////////////////////////////////////////////
	// Attachment.
	//////////////////////////////////////////////////////////////////////////
	void AllocBindings();
	void DeallocBindings();
	void OnRellocate( int nWhyFlags );
	void LogEvent( SEntityEvent &event,CTimeValue dt );
	//////////////////////////////////////////////////////////////////////////

private:
	void ComputeForwardDir() const;
	bool IsScaled(float threshold = 0.0003f) const
	{ 
		return (fabsf(m_vScale.x - 1.0f) + fabsf(m_vScale.y - 1.0f) + fabsf(m_vScale.z - 1.0f)) >= threshold;
	}
	// Fetch the IA object from the AIObjectID, if any
	IAIObject *GetAIObject();

private:
	//////////////////////////////////////////////////////////////////////////
	// VARIABLES.
	//////////////////////////////////////////////////////////////////////////
	friend class CEntitySystem;
	friend class CPhysicsEventListener; // For faster access to internals.
	friend class CEntityObject; // For faster access to internals.
	friend class CRenderProxy;  // For faster access to internals.
	friend class CTriggerProxy;
  friend class CPhysicalProxy;

	// Childs structure, only allocated when any child entity is attached.
	struct SBindings
	{
		std::vector<CEntity*> childs;
		CEntity *pParent;
	};

	//////////////////////////////////////////////////////////////////////////
	// Internal entity flags (must be first member var of Centity) (Reduce cache misses on access to entity data).
	//////////////////////////////////////////////////////////////////////////
	unsigned int m_bActive : 1;                 // Active Entities are updated every frame.
	unsigned int m_bInActiveList : 1;           // Added to entity system active list.
	mutable unsigned int m_bBoundsValid : 1;    // Set when the entity bounding box is valid.
	unsigned int m_bInitialized : 1;			      // Set if this entity already Initialized.
	unsigned int m_bHidden : 1;                 // Set if this entity is hidden.
	unsigned int m_bGarbage : 1;                // Set if this entity is garbage and will be removed soon.
	unsigned int m_bUseProxyArray : 1;          // Set if proxy array is allocated (see UProxies).
	unsigned int m_bHaveEventListeners : 1;     // Set if entity have an event listeners associated in entity system.
	unsigned int m_bTrigger : 1;                // Set if entity is proximity trigger itself.
	unsigned int m_bWasRellocated : 1;          // Set if entity was rellocated at least once.
	unsigned int m_nUpdateCounter : 4;          // Update counter is used to activate the entity for the limited number of frames.
	                                            // Usually used for Physical Triggers.
	                                            // When update counter is set, and entity is activated, it will automatically
	                                            // deactivate after this counter reaches zero
	unsigned int m_eUpdatePolicy : 4;           // Update policy defines in which cases to call
	                                            // entity update function every frame.
	unsigned int m_bInvisible: 1;               // Set if this entity is invisible.
	unsigned int m_bNotInheritXform : 1;        // Inherit or not transformation from parent.

	// Unique ID of the entity.
	EntityId m_nID;

	// Optional entity guid.
	EntityGUID m_guid;

	// Entity flags. any combination of EEntityFlags values.
	uint32 m_flags;

	// Name of the entity. 
	string m_szName;

	// Pointer to the class that describe this entity.
	IEntityClass *m_pClass;

	// Pointer to the entity archetype.
	IEntityArchetype* m_pArchetype;

	// Position of the entity in local space.
	Vec3 m_vPos;
	// Rotation of the entity in local space.
	Quat m_qRotation;
	// Scale of the entity in local space.
	Vec3 m_vScale;
	// World transformation matrix of this entity.
	mutable Matrix34 m_worldTM;

	// Cached world transformed forward vector
	mutable bool m_bDirtyForwardDir;
	mutable Vec3 m_vForwardDir;

	// Pointer to hierarchical binding structure.
	// It contains array of child entities and pointer to the parent entity.
	// Because entity attachments are not used very often most entities do not need it,
	// and space is preserved by keeping it separate structure.
	SBindings *m_pBinds;

	// The representation of this entity in the AI system.
	tAIObjectID m_aiObjectID;
	CSmartObject *m_pSmartObject;

	// Custom entity material.
	_smart_ptr<IMaterial> m_pMaterial;

	//////////////////////////////////////////////////////////////////////////
	// m_proxy.simple or m_proxy.array member of this strucre are used 
	// depending on the m_bUseProxyArray flag.
	//////////////////////////////////////////////////////////////////////////
	union UProxies
	{
		// Simple structure is used when only Render and Physics proxies are needed.
		struct SProxySimple {
			IEntityProxy* pProxyArray[3]; // Array of 3 proxies, Only Render/Physics/Script proxy can go here.
		} simple;
		// Proxy array is used when more then just Render and Physics proxies are needed.
		// Proxy array is more expensive as it requires additional allocation of the array of pointers.
		struct SProxyArray {
			IEntityProxy** pProxyArray; // Array of all supported entity proxies.
			int            numProxies;  // Number of proxies in m_pProxyArray
		} array;
	};
	UProxies m_proxy;
	//////////////////////////////////////////////////////////////////////////

	// Entity Links.
	IEntityLink* m_pEntityLinks;

	// For tracking entity in the partition grid.
	SGridLocation *m_pGridLocation;
	// For tracking entity inside proximity trigger system.
	SProximityElement *m_pProximityEntity;
};

//////////////////////////////////////////////////////////////////////////
void ILINE CEntity::ComputeForwardDir() const
{
	if (m_bDirtyForwardDir)
	{
		if (IsScaled())
		{
			Matrix34 auxTM = m_worldTM;
			auxTM.OrthonormalizeFast();

			// assuming (0, 1, 0) as the local forward direction
			m_vForwardDir = auxTM.GetColumn1();
		}
		else
		{
			// assuming (0, 1, 0) as the local forward direction
			m_vForwardDir = m_worldTM.GetColumn1();
		}

		m_bDirtyForwardDir = false;
	}
}


#endif // __Entity_h__
