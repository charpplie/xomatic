////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   IParticles.h
//  Version:     v1.00
//  Created:     2008-02-26 by Scott Peter
//  Compilers:   Visual Studio.NET
//  Description: Interfaces to particle types
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include DEVIRTUALIZE_HEADER_FIX(IParticles.h)

#ifndef IPARTICLES_H
#define IPARTICLES_H

#include <IMemory.h>
#include <IEntityRenderState.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Summary:
//     Reference to one geometry type
// Description:
//	   Reference to one of several types of geometry, for particle attachment.
struct GeomRef
{
	IStatObj*	m_pStatObj;
	ICharacterInstance* m_pChar;
	IPhysicalEntity* m_pPhysEnt;

	GeomRef(): m_pStatObj(0), m_pChar(0), m_pPhysEnt(0) {}
	GeomRef(IStatObj* pObj) : m_pStatObj(pObj), m_pChar(0), m_pPhysEnt(0) {}
	GeomRef(ICharacterInstance* pChar) : m_pChar(pChar), m_pStatObj(0), m_pPhysEnt(0) {}
	GeomRef(IPhysicalEntity* pPhys) : m_pChar(0), m_pStatObj(0), m_pPhysEnt(pPhys) {}

	operator bool() const
	{
		return m_pStatObj || m_pChar || m_pPhysEnt;
	}
};
//Summary:
//		Real-time params to control particle emitters.
// Description:
//		Real-time params to control particle emitters. Some parameters override emitter params.
struct SpawnParams
{
	EGeomType	eAttachType;			// What type of object particles emitted from.
	EGeomForm	eAttachForm;			// What aspect of shape emitted from.
	int				nAttachSlot;			// Entity slot where particle emitter attaches.
	float			fCountScale;			// Multiple for particle count (on top of bCountPerUnit if set).
	float			fSizeScale;				// Multiple for all effect sizes.
	float			fSpeedScale;			// Multiple for particle emission speed.
	float			fPulsePeriod;			// How often to restart emitter.
	float			fStrength;				// Controls parameter strength curves.
	bool			bCountPerUnit;		// Multiply particle count also by geometry extent (length/area/volume).
	bool			bIndependent;			// Emitter should spawn independent of entity (momentary attachment).
	bool			bIgnoreLocation;	// Ignore all location-based culling tests (useful for detached rendering calls).

	inline SpawnParams( EGeomType eType = GeomType_None, EGeomForm eForm = GeomForm_Surface )
	{
		eAttachType = eType;
		eAttachForm = eForm;
		nAttachSlot = -1;
		fCountScale = 1;
		fSizeScale = 1;
		fSpeedScale = 1;
		fPulsePeriod = 0;
		fStrength = -1;
		bCountPerUnit = false;
		bIndependent = false;
		bIgnoreLocation = false;
	}
};

struct ParticleTarget
{
	Vec3	vTarget;				// Target location for particles.
	Vec3	vVelocity;				// Velocity of moving target.
	float	fRadius;				// Size of target, for orbiting.

	// Options.
	bool	bTarget;				// Target is enabled.
	bool	bPriority;				// This target takes priority over others (attractor entities).

	ParticleTarget()
	{
		memset(this, 0, sizeof(*this));
	}
};


struct ParticleParams;

// Summary:
//		Interface to control a particle effect.
// Description:
//		This interface is used by I3DEngine::CreateParticleEffect to control a particle effect.
// 	It is created by CreateParticleEffect method of 3d engine.
UNIQUE_IFACE struct IParticleEffect : public _i_reference_target_t
{
	static Matrix34 ParticleLoc(const Vec3& pos, const Vec3& dir = Vec3(0,0,1), float scale = 1.f)
	{
		Quat q(IDENTITY);
		if (!dir.IsZero())
		{
			// Rotate in 2 stages to avoid roll.
			Vec3 dirxy = Vec3(dir.x, dir.y, 0.f);
			if (!dirxy.IsZero())
			{
				dirxy.Normalize();
				q = Quat::CreateRotationV0V1(dirxy, dir.GetNormalized())
					* Quat::CreateRotationV0V1(Vec3(0,1,0), dirxy);
			}
			else
				q = Quat::CreateRotationV0V1(Vec3(0,1,0), dir.GetNormalized());
			Vec3 check = q * Vec3(0,1,0);
		}

		// Todo: 
		//	 Use QuatTS directly for all particle coords.
		return Matrix34( QuatTS(q, pos, scale) );
	}

	//////////////////////////////////////////////////////////////////////////
	// Spawn this effect.
	//////////////////////////////////////////////////////////////////////////

	// Summary:
	//     Spawns the particles.
	virtual struct IParticleEmitter* Spawn( bool bIndependent, const Matrix34& loc ) = 0;

	// Summary:
	//     Sets a new name to this particle effect.
	// Arguments:
	//     sFullName - The full name of this effect, including library and group qualifiers.
	virtual void SetName( cstr sFullName ) = 0;

	// Summary:
	//     Gets the name of this particle effect.
	// Return Value:
	//     A C string which holds the fully qualified name of this effect, with parents and library.
	virtual cstr GetName() const = 0;

	// Summary:
	//     Gets the base name of this particle effect.
	// Return Value:
	//     A C string which holds the base name of this effect.
	//		 For top-level effects, this includes group qualifier.
	virtual cstr GetBaseName() const = 0;

	// Summary:
	//     Enables or disables the effect.
	// Arguments:
	//     bEnabled - set to true to enable the effect or to false to disable it
	virtual void SetEnabled( bool bEnabled ) = 0;

	// Summary:
	//     Determines if the effect is already enabled.
	// Return Value:
	//     A boolean value which indicate the status of the effect; true if 
	//     enabled or false if disabled.
	virtual bool IsEnabled() const = 0;

	// Summary:
	//     Sets the particle parameters.
	// Return Value:
	//     An object of the type ParticleParams which contains several parameters.
	virtual void SetParticleParams( const ParticleParams &params ) = 0;

	//! Return ParticleParams.

	// Summary:
	//     Gets the particle parameters.
	// Return Value:
	//     An object of the type ParticleParams which contains several parameters.
	virtual const ParticleParams& GetParticleParams() const = 0;

	//////////////////////////////////////////////////////////////////////////
	// Child particle systems.
	//////////////////////////////////////////////////////////////////////////

	// Summary:
	//     Gets the number of sub particles childs.
	// Return Value:
	//     An integer representing the amount of sub particles childs
	virtual int GetChildCount() const = 0;

	//! Get sub Particles child by index.

	// Summary:
	//     Gets a specified particles child.
	// Arguments:
	//     index - The index of a particle child
	// Return Value:
	//     A pointer to a IParticleEffect derived object.
	virtual IParticleEffect* GetChild( int index ) const = 0;

	// Summary:
	//   Adds a child particle effect.
	// Arguments:
	//   pEffect - A pointer the particle effect to add as child
	virtual void AddChild( IParticleEffect *pEffect ) = 0;

	// Summary:
	//   Removes a sub particle effect.
	// Arguments:
	//   pEffect - A pointer to the child particle effect to be removed
	virtual void RemoveChild( IParticleEffect *pEffect ) = 0;

	// Summary:
	//   Removes all child particles.
	virtual void ClearChilds() = 0;

	// Summary:
	//   Inserts a child particle effect at a precise slot.
	// Arguments:
	//   slot - An integer value which specify the desired slot
	//   pEffect - A pointer to the particle effect to insert
	virtual void InsertChild( int slot,IParticleEffect *pEffect ) = 0;

	// Summary:
	//   Finds in which slot a child particle effect is stored.
	// Arguments:
	//   pEffect - A pointer to the child particle effect
	// Return Value:
	//   An integer representing the slot number or -1 if the slot is not found.
	virtual int FindChild( IParticleEffect *pEffect ) const = 0;

	// Summary:
	//	 Gets the particles effect parent, if any.
	// Return Value:
	// A pointer representing the particles effect parent.
	virtual IParticleEffect* GetParent() const = 0;

	// Summary:
	//   Loads all resources needed for a particle effects.
	// Returns:
	//	 True if any resources loaded.
	virtual bool LoadResources() = 0;

	// Summary:
	//   Unloads all resources previously loaded.
	virtual void UnloadResources() = 0;

	// Summary:
	//   Serializes particle effect to/from XML.
	// Arguments:
	//   bLoading - true when loading,false for saving.
	//   bChilds - When true also recursively serializes effect childs.
	virtual void Serialize( XmlNodeRef node,bool bLoading,bool bChilds ) = 0;
};

// Description:
//     An IParticleEmitter should usually be created by 
//     I3DEngine::CreateParticleEmitter. Deleting the emitter should be done 
//     using I3DEngine::DeleteParticleEmitter.
// Summary:
//     Interface to a particle effect emitter.
UNIQUE_IFACE struct IParticleEmitter : public CMultiThreadRefCount, public IRenderNode
{
	// Summary: 
	//		Returns whether emitter still alive in engine.
	virtual bool IsAlive() const = 0;

	// Summary:
	//		Sets emitter state to active or inactive. Emitters are initially active.
	//		if bActive = true:
	//				Emitter updates and emits as normal, and deletes itself if limited lifetime.
	//		if bActive = false: 
	//				Stops all emitter updating and particle emission.
	//				Existing particles continue to update and render.
	//				Emitter is not deleted.
	virtual void Activate( bool bActive ) = 0;

	// Summary:
	//		 Advances the emitter to its equilibrium state.
	virtual void Prime() = 0;

	// Summary: 
	//		 Restarts the emitter from scratch (if active). 
	//		 Any existing particles are re-used oldest first.
	virtual void Restart() = 0;

	// Description:
	//     Will define the effect used to spawn the particles from the emitter.
	// Notes: 
	//     Never call this function if you already used SetParams.
	// See Also:
	//     SetParams
	// Arguments:
	//     pEffect - A pointer to an IParticleEffect object
	// Summary:
	//     Set the effects used by the particle emitter.
	virtual void SetEffect( const IParticleEffect* pEffect ) = 0;

	// Summary:
	//		 Specifies how particles are emitted from source.
	virtual void SetSpawnParams( const SpawnParams& spawnParams, GeomRef geom = GeomRef()) = 0;

	// Summary:
	//		 Retrieves current SpawnParams.
	virtual void GetSpawnParams( SpawnParams& spawnParams ) const = 0;

	// Summary:
	//      Associates emitter with entity, for dynamic updating of positions etc.
	// Notes:
	//      Must be done when entity created or serialized, entity association is not serialized.
	virtual void SetEntity( IEntity* pEntity, int nSlot ) = 0;

	// Attractors.
	virtual void SetTarget( const ParticleTarget& target ) = 0;

	virtual void Serialize(TSerialize ser) = 0;

	// Summary:
	//		Updates emitter's state to current time.
	virtual void Update() = 0;

	// Summary:
	//		 Programmatically add a particle to emitter for rendering.
	//		 With no arguments, spawns a particle according to emitter settings.
	//		 Specific objects can be passed for programmatic control.
	// Arguments:
	//		 pLocation	- Specified location for particle. If 0, set from effect params.
	//		 pVel		- Specified velocity for particle. If 0, set from effect params.
	//		 pStatObj	- The displayable geometry object for the entity. If 0, uses emitter settings for sprite or geometry.
	//       pPhysEnt	- A physical entity which controls the particle. If 0, uses emitter settings to physicalise or move particle.
	virtual void EmitParticle( IStatObj* pStatObj = NULL, IPhysicalEntity* pPhysEnt = NULL, QuatTS* pLocation = NULL, Vec3* pVel = NULL ) = 0;

	virtual bool UpdateStreamableComponents( float fImportance, Matrix34A& objMatrix, IRenderNode* pRenderNode, float fEntDistance ) = 0;

	// Summary:
	//		 Get the Entity ID that this particle emitter is attached to
	virtual unsigned int GetAttachedEntityId() = 0;

	// Summary:
	//		 Get the Entity Slot that this particle emitter is attached to
	virtual int GetAttachedEntitySlot() = 0;
};

//////////////////////////////////////////////////////////////////////////
// Description:
//	 A callback interface for a class that wants to be aware when particle emitters are being created/deleted.
struct IParticleEffectListener
{
	// Description:
	//	 This callback is called when a new particle emitter is created.
	// Arguments:
	//   pEmitter     - The emitter created
	//   bIndependent - 
	//	 mLoc		      - The location of the emitter
	//	 pEffect      - The particle effect
	//	 pParams      - The parameters of the particle effect
	virtual void OnCreateEmitter(IParticleEmitter* pEmitter, bool bIndependent, Matrix34 const& mLoc, const IParticleEffect* pEffect, const ParticleParams* pParams) = 0;

	// Description:
	//	 This callback is called when a particle emitter is deleted.
	// Arguments:
	//   pEmitter - The emitter being deleted
	virtual void OnDeleteEmitter(IParticleEmitter* pEmitter) = 0;
};

//////////////////////////////////////////////////////////////////////////
struct SContainerCounts
{
	float		EmittersRendered, ParticlesRendered;
	float		PixelsProcessed, PixelsRendered;
	float		ParticlesReiterate, ParticlesReject;
	float		ParticlesCollideTerrain, ParticlesCollideObjects, ParticlesClip;

	SContainerCounts()
		{ memset(this, 0, sizeof(*this)); }
};

struct SParticleCounts: SContainerCounts
{
	float		EmittersAlloc, EmittersActive;
	float		ParticlesAlloc, ParticlesActive;
	float		ParticlesOverflow;

	SParticleCounts()
		{ memset(this, 0, sizeof(*this)); }
};

struct SSumParticleCounts: SParticleCounts
{
	float	SumParticlesAlloc, SumEmittersAlloc;

	SSumParticleCounts()
		: SumParticlesAlloc(0.f), SumEmittersAlloc(0.f)
	{}
};

struct SEffectCounts
{
	int		nLoaded, nUsed, nEnabled, nActive;

	SEffectCounts()
		{ memset(this, 0, sizeof(*this)); }
};

template<class T>
Array<float> FloatArray( T& obj )
{
	return Array<float>( (float*)&obj, (float*)(&obj+1) );
}

inline void AddArray( Array<float> dst, Array<const float> src )
{
	for (int i = min(dst.size(), src.size())-1; i >= 0; --i)
		dst[i] += src[i];
}

inline void BlendArray( Array<float> dst, float fDst, Array<const float> src, float fSrc )
{
	for (int i = min(dst.size(), src.size())-1; i >= 0; --i)
		dst[i] = dst[i] * fDst + src[i] * fSrc;
}

//////////////////////////////////////////////////////////////////////////
struct IParticleManager
{
	//////////////////////////////////////////////////////////////////////////
	// ParticleEffects
	//////////////////////////////////////////////////////////////////////////

	// Summary:
	//     Creates a new empty particle effect.
	// Return Value:
	//     A pointer to a object derived from IParticleEffect.
	virtual IParticleEffect* CreateEffect() = 0;

	// Summary:
	//     Deletes a specified particle effect.
	// Arguments:
	//     pEffect - A pointer to the particle effect object to delete
	virtual void DeleteEffect( IParticleEffect* pEffect ) = 0;

	// Summary:
	//     Searches by name for a particle effect.
	// Arguments:
	//     sEffectName - The fully qualified name (with library prefix) of the particle effect to search.
	//	   bLoad - Whether to load the effect's assets if found.
	//	   sSource - Optional client context, for diagnostics.
	// Return Value:
	//     A pointer to a particle effect matching the specified name, or NULL if not found.
	virtual IParticleEffect* FindEffect( cstr sEffectName, cstr sSource = "", bool bLoadResources = true ) = 0;

	// Summary:
	//     Creates a particle effect from an XML node. Overwrites any existing effect of the same name.
	// Arguments:
	//     sEffectName:			the name of the particle effect to be created.
	//		 effectNode:			Xml structure describing the particle effect properties.
	//		 bLoadResources:	indicates if the resources for this effect should be loaded.
	// Return value:
	//     Returns a pointer to the particle effect.
	virtual IParticleEffect* LoadEffect( cstr sEffectName, XmlNodeRef& effectNode, bool bLoadResources ) = 0;

	// Summary:
	//     Loads a library of effects from an XML description.
	// Arguments:
	// Return value:
	virtual bool LoadLibrary( cstr sParticlesLibrary, XmlNodeRef& libNode, bool bLoadResources ) = 0;
	virtual bool LoadLibrary( cstr sParticlesLibrary, cstr sParticlesLibraryFile = NULL, bool bLoadResources = false ) = 0;

	//////////////////////////////////////////////////////////////////////////
	// ParticleEmitters
	//////////////////////////////////////////////////////////////////////////

	// Summary:
	//     Creates a new particle emitter, with custom particle params instead of a library effect.
	// Arguments:
	//		 bIndependent - 
	//		 mLoc - World location of emitter.
	//		 Params - Programmatic particle params.
	// Return Value:
	//     A pointer to an object derived from IParticleEmitter
	virtual IParticleEmitter* CreateEmitter( bool bIndependent, Matrix34 const& mLoc, const ParticleParams& Params ) = 0;

	// Summary:
	//     Deletes a specified particle emitter.
	// Arguments:
	//     pPartEmitter - Specify the emitter to delete
	virtual void DeleteEmitter( IParticleEmitter * pPartEmitter ) = 0;

	// Processing.
	virtual void Update() = 0;
	virtual void RenderDebugInfo() = 0;
	virtual void Reset( bool bIndependentOnly ) = 0;
	virtual void ClearRenderResources( bool bForceClear ) = 0;
	virtual void OnStartRuntime() = 0;
	virtual void OnFrameStart() = 0;
	virtual void Serialize( TSerialize ser ) = 0;
	virtual void PostSerialize( bool bReading ) = 0;

	// Stats
	virtual void GetMemoryUsage( ICrySizer* pSizer ) = 0;
	virtual void GetCounts( SParticleCounts& counts ) = 0;

	// Summary:
	//	 Registers new particle events listener.
	virtual void AddEventListener(IParticleEffectListener *pListener) = 0;
	virtual void RemoveEventListener(IParticleEffectListener *pListener) = 0;
};

#endif //IPARTICLES_H


