////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ParticleContainer.h
//  Version:     v1.00
//  Created:     11/03/2010 by Corey (split out from other files).
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __particlecontainer_h__
#define __particlecontainer_h__
#pragma once


#include "ParticleEffect.h"
#include "ParticleEnviron.h"
#include "ParticleUtils.h"


#undef PlaySound

class CParticle;
class CParticleEffect;
class CParticleEmitter;
class CParticleSubEmitter;
class CParticleLocEmitter;
struct SParticleUpdateContext;
struct SParticleHistory;
struct SParticleVertexContext;
struct ResourceParticleParams;
struct SContainerCounts;

//////////////////////////////////////////////////////////////////////////
// Particle rendering helper params.
struct SPartRenderParams: SParticleRenderContext
{
	float						m_fCamDistance;
	uint16					m_nFogVolumeContribIdx;
	uint16					m_nEmitterOrder;
	uint32					m_nRenFlags;
	float						m_fHDRDynamicMultiplier;
} _ALIGN(16);

//////////////////////////////////////////////////////////////////////////
// Contains, updates, and renders a list of particles, of a single effect

class CParticleContainer : public Cry3DEngineBase, public IParticleVertexCreator
{
public:

	CParticleContainer( CParticleContainer* pParent, CParticleEmitter* pMain, CParticleEffect const* pEffect, ParticleParams const* pParams = 0 );
	~CParticleContainer();

	// Associated structures.
	ResourceParticleParams const& GetParams() const		
	{ 



		return *const_cast<ResourceParticleParams*>(m_pParams); 

	}
	const ResourceParticleParams* GetParamsAddr() const	
																										{ return m_pParams; }
	const CParticleEffect* GetEffect() const					{ return m_pEffect; }
	CParticleEmitter& GetMain()												{ return *m_pMainEmitter; }
	const CParticleEmitter& GetMain() const						{ return *m_pMainEmitter; }

	CParticleSubEmitter* GetDirectEmitter() const			{ return m_pParentContainer || m_Emitters.empty() ? NULL : &non_const(m_Emitters).front(); }

	// If indirect container.
	CParticleContainer* GetParent() const							{ return m_pParentContainer; }

	void Update();
	void Reset();

	CParticleSubEmitter* AddEmitter( CParticleLocEmitter* pLoc, CParticleSubEmitter* pParentEmitter );
	CParticleLocEmitter* AddLocEmitter( CParticleEmitter *pEmitter, float fPast );
	CParticle* AddParticle( CParticleSubEmitter* pEmitter );
	void DeleteParticle( CParticle* pPart );
	void UpdateParticles();
	void EmitParticles( SParticleUpdateContext const& context );
	uint32 UpdateBounds( AABB& bbMain, AABB& bbDyn );
	void UpdateSound();
	void UpdateForce();
	void InvalidateStaticBounds();
	void Render( SRendParams const& RenParams, SPartRenderParams& PRParams );
	void RenderGeometry( const SRendParams& RenParams, const SParticleRenderContext& RenContext );
	void RenderLights();
	void UnqueueUpdate();
	void SetDynamicBounds()
	{
		m_bDynamicBounds = true;
	}
	bool NeedsDynamicBounds() const
	{
		return m_bDynamicBounds;
	}
	uint32 GetEnvironmentFlags() const
	{
		return GetParams().nEnvFlags;
	}

	bool IsImmortal() const;
	bool HasEquilibrium() const;
	float GetEquilibriumAge(bool bAll = true) const;
	float TimeNotRendered() const;
	bool HasParticles() const
	{
		return !m_Particles.empty();
	}
	bool HasExtendedLifetime() const
	{
		return GetParams().bRemainWhileVisible && TimeNotRendered() < 0.5f;
	}
	uint32 GetChildFlags() const
	{
		return m_nChildFlags;
	}
	float GetEmitCountScale() const
	{
		return m_fEmitCountScale;
	}
	float GetMaxParticleLife(bool bAll = true) const
	{
		return m_pEffect ? m_pEffect->GetMaxParticleLife(bAll) : m_pParams->GetMaxParticleLife();
	}
	bool OwnsParams() const
	{
		return !m_pEffect;
	}
	bool HasLocalTarget() const
	{
		return m_bHasLocalTarget;
	}
	float GetTimeToUpdate() const;
	bool NeedsUpdate() const
	{
		return GetTimer()->GetFrameStartTime() > m_timeLastUpdate;
	}

	// Temp functions to update edited effects.
	inline bool IsUsed() const		
	{ 
		return !m_bUnused;
	}
	void SetUsed(bool b)
	{
		m_bUnused = !b;
	}
	void ClearUnused();

	bool Is( IParticleEffect const* pEffect, CParticleContainer const* pParent ) const
	{
		return m_pParentContainer == pParent && m_pEffect == pEffect;
	}

	void AllocParticles( int nExtra = 0 );

	// Particle tail management.
	SParticleHistory* AllocPosHistory();
	void FreePosHistory( SParticleHistory* aHist );
	int GetHistorySteps() const
	{
		return m_nHistorySteps;
	}

	// Stat/profile functions.
	SContainerCounts& GetCounts() 
	{ 
		return m_Counts; 
	}
	void GetCounts( SParticleCounts& counts, bool bEffectStats = false ) const;
	
	void GetMemoryUsage( ICrySizer* pSizer ) const;

	//////////////////////////////////////////////////////////////////////////
	// IParticleVertexCreator methods

	virtual void ComputeVertices( const SParticleRenderContext& Context, IAllocRender& alloc, bool bIsParticleThread = false );
	virtual float GetDistSquared( const Vec3& vPos ) const;
	virtual uint32 GetRenderOrder() const
	{
		return m_nRenderOrder;
	}
	virtual void AddRef();
	virtual void Release();

	// Other methods.
	bool CanThread();

	void WriteVerticesDirect( const SParticleRenderContext& context, IAllocRender& alloc );
	void WriteVerticesIndirect( const SParticleRenderContext& context, IAllocRender& alloc );
	void WriteVerticesIndirectImpl( SParticleVertexContext& vcontext, SRenderVertices& alloc );













	void CollectSpuUsage( VecSpuUsageT &vecSpuUsage ) const;
	
	CryReadModifyLock& GetLock() { return m_Lock; }

	float GetTexelAreaDensity() const { return m_texelAreaDensity; }

protected:

	CryReadModifyLock								m_Lock;						// Threading support.
	uint32													m_nRenderOrder;

	const ResourceParticleParams*		m_pParams;				// Pointer to particle params (effect or code).
	const CParticleEffect*					m_pEffect;				// Particle effect used for this emitter.
	uint32													m_nChildFlags;		// Summary of rendering/environment info for child containers.

	List<CParticleSubEmitter>				m_Emitters;				// All emitters into this container.
	List<CParticleLocEmitter>				m_ChildEmitters;	// Emitter sources for particles if parents
	
	// Shared array of tail positions. Must be declared before particle list, to be destroyed after.
	DynArrayAlign<SParticleHistory, 128>
																	m_aHistoryPool;
	SParticleHistory*								m_pHistoryFree;
	int															m_nHistorySteps;

	// Fixed-size array of particles.
	typedef FixedArray<CParticle>		TParticleList;
	TParticleList										m_Particles;
	int															m_nReserveCount;
	float														m_fEmitCountScale;

	// Last time when emitter updated, and static bounds validated.
	CTimeValue											m_timeLastUpdate;
	CTimeValue											m_timeStaticBounds;

	// Final bounding volume for rendering. Also separate static & dynamic volumes for sub-computation.
	AABB														m_bbWorld, m_bbWorldStat, m_bbWorldDyn;
	bool														m_bDynamicBounds;						// Requires dynamic bounds computation.
	bool														m_bUnused;									// Temp var used during param editing.
	bool														m_bHasLocalTarget;					// Particles targeted to a parent emitter.
	DynArray<SPhysEnviron::SArea>		m_aNonUniformAreas;					// Persistent array.

	// Associated structures.
	CParticleContainer*							m_pParentContainer;					// Parent container, if indirect.
	CParticleEmitter*								m_pMainEmitter;							// Emitter owning this container.

#ifdef SHARED_GEOM
	AutoPtrArray<SInstancingInfo>		m_InstInfos;								// For geom particle rendering.
	int															m_nInstInfosUsed;						// Infos used in a given frame (avoid clearing Infos array each frame).
#endif
	SContainerCounts								m_Counts;										// Counts for stats.






	float														m_texelAreaDensity;					// Required for streaming calculations for particle systems which use textures

	// Bounds functions.
	bool NeedsExtendedBounds() const
	{
		// Bounds need extending on movement unless bounds always restricted relative to emitter.
		return !m_pParams->bMoveRelEmitter && !m_pParams->bSpaceLoop;
	}
	void ComputeStaticBounds( AABB& bb, bool bWithSize = true, float fMaxLife = fHUGE );
	inline bool StaticBoundsStable() const
	{
		return m_timeStaticBounds < GetTimer()->GetFrameStartTime();
	}
	void GetDynamicBounds( AABB& bb );
	void ComputeUpdateContext( SParticleUpdateContext& context );

	void ComputeReserveCount();
	int GetMaxVertexCount() const;

	void DeleteDeadParticles();
	void UpdateParticleStates( const SParticleUpdateContext& context, float fUpdateTime );

	// Other functions.
	void ReleaseParams();
	void UnloadResources();
};

Declare_GetMemoryUsage(CParticleContainer)

//////////////////////////////////////////////////////////////////////////

#endif // __particlecontainer_h__
