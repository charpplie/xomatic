////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ParticleEmitter.h
//  Version:     v1.00
//  Created:     18/7/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __particleemitter_h__
#define __particleemitter_h__
#pragma once

#ifdef _WIN32
#pragma warning(disable: 4355)
#endif

#include "ParticleEffect.h"
#include "ParticleEnviron.h"
#include "partman.h"
#include "ParticleContainer.h"
#include "ParticleUtils.h"
#include "GeomQuery.h"
#include "BitFiddling.h"

#include <SpuUtils.h>

#undef PlaySound

class CParticle;
class CParticleContainer;
class CParticleSubEmitter;
class CParticleLocEmitter;
class CParticleEmitter;
struct SParticleUpdateContext;
struct SParticleHistory;
struct SParticleVertexContext;

//////////////////////////////////////////////////////////////////////////
enum EActivateMode
{ 
	eAct_Activate,
	eAct_Deactivate,
	eAct_Reactivate,
	eAct_Restart,
	eAct_Pause,
	eAct_Resume,
};

enum EEmitterState
{
	eEmitter_Dead,				// No current or future particles.
	eEmitter_Dormant,			// No current particles, but may in future.
	eEmitter_Particles,		// Currently has particles.
	eEmitter_Active,			// Currently emitting particles
};














class CParticleSubEmitter: public Cry3DEngineBase, public DumbRefCount
// Maintains an emitter source state, emits particles to a container
// Ref count increased only by emitted particles
{
public:

	CParticleSubEmitter( CParticleSubEmitter* pParent, CParticleLocEmitter* pLoc, CParticleContainer* pCont );
	ILINE ~CParticleSubEmitter()										{ Destroy(); }

	ResourceParticleParams const& GetParams() const	
	{



		return *m_pParams; 

	}
	CParticleContainer& GetContainer() const				{ return *m_pContainer; }
	CParticleLocEmitter& GetLoc() const							{ return *m_pLocEmitter; }
	bool GetLocalTarget( ParticleTarget& target ) const;
	CParticleEmitter& GetMain() const;

	// State.
	EEmitterState GetState() const;
	void Activate( EActivateMode mode, float fPast = 0.f );

	// Timing.
	float GetAge() const;
	SPU_NO_INLINE float GetRelativeAge( float fAgeAdjust = 0.f, ESoundControlTime const eControl = SoundControlTime_EmitterLifeTime ) const
	{
		// Choose control time.
		float fEnd;
		switch (eControl)
		{
		default:
		case SoundControlTime_EmitterLifeTime:
			fEnd = min(GetEndAge(), m_fRepeatAge);
			break;
		case SoundControlTime_EmitterExtendedLifeTime:
			fEnd = min(GetEndAge() + GetContainer().GetMaxParticleLife(), m_fRepeatAge);
			break;
		case SoundControlTime_EmitterPulsePeriod:
			fEnd = m_fRepeatAge;
			break;
		}

		float const fAge = GetAge() + fAgeAdjust;
		if (min(fAge, fEnd) <= m_fStartAge)
			return 0.f;

		return div_min(fAge - m_fStartAge, fEnd - m_fStartAge, 1.f);
	}

	float GetStrength( float fAgeAdjust = 0.f, ESoundControlTime const eControl = SoundControlTime_EmitterLifeTime ) const;

	// Param evaluation.
	template<class T> inline
	T GetCurValue( TVarParam<T> const& param ) const
	{
		return param.GetVarValue( m_ChaosKey );
	}

	template<class T> inline
	T GetCurValue( TVarEParam<T> const& param, float fAbs = 0.f ) const
	{
		return param.GetVarValue( m_ChaosKey, m_fStrength, fAbs );
	}

	template<class T> inline
	T GetCurValue( float fMinMax, TVarEParam<T> const& param ) const
	{
		return param.GetVarValue( fMinMax, m_fStrength );
	}

	float GetEmitRate() const;
	Vec3 GetEmitPos() const;

	// Actions.
	void Update();
	void UpdateSound();
	void ResetLoc()
	{
		m_qpLastLoc.s = -1.f;
	}
	void SetLastLoc();
	void MoveRelative( Vec3& vPos, Quat& qRot, Vec3& vVel );
	void UpdateForce();
	int EmitParticle( SParticleUpdateContext const& context, float fAge, IStatObj* pStatObj = NULL, IPhysicalEntity* pPhysEnt = NULL, const QuatTS* pLocation = SPU_LOCAL_PTR((QuatTS*)NULL), const Vec3* pVel = NULL );
	void EmitParticles( SParticleUpdateContext const& context );

protected:

	// Associated structures.
	ResourceParticleParams const*		m_pParams;
	CParticleContainer*							m_pContainer;					// Direct or shared container to emit particles into.
	_smart_ptr<CParticleSubEmitter>	m_pParentEmitter;
	_smart_ptr<CParticleLocEmitter>	m_pLocEmitter;

	// State.
	float					m_fStartAge;			// Relative age when scheduled to start (default 0).
	float					m_fEndAge;				// Relative age when scheduled to end (fHUGE if never).
	float					m_fRepeatAge;			// Relative age when scheduled to repeat (fHUGE if never).

	float					m_fStrength;			// Emitter strength, set externally, or from relative age.

	CChaosKey			m_ChaosKey;				// Seed for randomising; inited every pulse.
	float					m_fToEmit;				// Fractional particles to emit.
	QuatTS				m_qpLastLoc;

	// External objects.
	bool								m_bSoundPlayed;		// Has been played since last Initialize.
	float								m_fSoundRange;		// Cached sound radius, to avoid playing far sounds.
	_smart_ptr<ISound>	m_pSound;
	IPhysicalEntity*		m_pForce;

	// Methods.
	void Initialize( float fPast = 0.f );
	void Deactivate();
	bool SoundInRange() const;

	float GetEndAge() const;

	inline float GetTimeToUpdate() const
	{
		return min( GetContainer().GetTimeToUpdate(), GetAge() );
	}
	void UpdateStrength()
	{
		m_fStrength = GetStrength();
		assert(m_fStrength >= 0.f && m_fStrength <= 1.f);
	}

	void Destroy();

} _ALIGN(16);

//////////////////////////////////////////////////////////////////////////
// An emitter system, maintains several emitters, with a single world location and presence.
// Can be top-level, or attached to a particle (indirect emitter)

class CParticleLocEmitter: public Cry3DEngineBase, public DumbRefCount
{
public:

	// World location.
	QuatTS							m_qpLoc;

	// Custom particle-emission shape, options, and targeting.
	GeomRef							m_EmitGeom;
	mutable GeomQuery		m_GeomQuery;

public:

	CParticleLocEmitter( CParticleEmitter* pMainEmitter, float fAge = 0.f );
	ILINE ~CParticleLocEmitter()												{ Destroy(); }

	CParticleSubEmitter* AddEmitter( CParticleSubEmitter* pParentEmitter, CParticleContainer* pContainer );

	// Location.
	void SetLoc( QuatTS const& qp )											{ m_qpLoc = qp; }
	QuatTS const& GetLocation() const										{ return m_qpLoc; }
	Matrix34 GetMatrix() const													{ return Matrix34( Vec3(m_qpLoc.s), m_qpLoc.q, m_qpLoc.t ); }
	Vec3 GetWorldPosition( Vec3 const& relpos ) const		{ return m_qpLoc * relpos; }
	Vec3 GetWorldOffset( Vec3 const& relpos ) const			{ return m_qpLoc.q * relpos * m_qpLoc.s; }

	CParticleEmitter& GetMain() const										{ return *m_pMainEmitter; }

	void SetVel( Vec3 const& v )												{ m_vVel = v; }
	Vec3 const& GetVel() const													{ return m_vVel; }

	bool SetEmitGeom( GeomRef geom );

	// Time.
	inline CTimeValue GetTimeCreated() const
	{
		return m_timeCreated;
	}
	inline float GetAge() const
	{
		return GetTimer()->GetFrameStartTime().GetDifferenceInSeconds( m_timeCreated );
	}
	inline float GetStopAge() const
	{
		return m_fStopAge;
	}
	inline void SetStopAge( float fStopAge )
	{
		m_fStopAge = min(m_fStopAge, fStopAge);
	}
	void Collide( float fPast = 0.f )
	{
		m_fCollideAge = min(m_fCollideAge, GetAge() - fPast);
	}
	float GetCollideAge()
	{
		return m_fCollideAge;
	}

	// State.
	inline bool IsActive() const
	{
		return m_eMaxState >= eEmitter_Active;
	}
	void Deactivate()
	{
		m_eMaxState = (EEmitterState) min(m_eMaxState, eEmitter_Particles); 
		SetStopAge(GetAge());
	}

protected:

	CParticleEmitter*											m_pMainEmitter;

	Vec3																	m_vVel;								// World velocity (transfered to particles).

	CTimeValue														m_timeCreated;				// Creation and deactivation time for emitter system.
	float																	m_fStopAge;						// Age prematurely paused or deactivated, or fHUGE.
	float																	m_fCollideAge;				// Age of first collision (for SpawnOnCollision children).
	EEmitterState													m_eMaxState;					// External control of emitter state.

	void Destroy();

} _ALIGN(16);


//////////////////////////////////////////////////////////////////////////
// A top-level emitter system, interfacing to 3D engine

class CParticleEmitter : public IParticleEmitter, public CParticleLocEmitter
{
	typedef CParticleLocEmitter	TParent;

public:

	CParticleEmitter( bool bIndependent );
	~CParticleEmitter();

	// Ref counting disambiguation.
	using IParticleEmitter::AddRef;
	using IParticleEmitter::Release;
	using IParticleEmitter::GetRefCount;

	//////////////////////////////////////////////////////////////////////////
	// IRenderNode implementation.
	//////////////////////////////////////////////////////////////////////////

	virtual void ReleaseNode()													{ Register(false); }	
	virtual EERType GetRenderNodeType()									{ return eERType_ParticleEmitter; }
	virtual char const* GetName() const									{ return GetEffect() ? GetEffect()->GetName() : "Emitter"; }
	virtual char const* GetEntityClassName() const			{ return "ParticleEmitter"; }
	virtual string GetDebugString(char type = 0) const;

	virtual Vec3 GetPos(bool bWorldOnly = true) const		{ return m_qpLoc.t; }
	virtual void GetLocalBounds( AABB& bbox );
	virtual float GetMaxViewDist();
	virtual void SetMatrix( Matrix34 const& mat )				{ if(mat.IsValid()) SetLoc(QuatTS(mat)); }

	virtual void SetMaterial( IMaterial* pMaterial )		{ m_pMaterial = pMaterial; }
	virtual IMaterial* GetMaterial(Vec3 * pHitPos = NULL)							
	{ 
		return m_pMaterial ? (IMaterial*) m_pMaterial 
			: !m_Containers.empty() ? (IMaterial*) m_Containers.front().GetParams().pMaterial
			: 0;
	}
	virtual IMaterial* GetMaterialOverride()						{ return m_pMaterial; }

	virtual IPhysicalEntity *GetPhysics() const					{ return 0; }
	virtual void SetPhysics(IPhysicalEntity *)					{}

	virtual void Render( SRendParams const& rParam );
	virtual void Hide( bool bHide )											{ Activate(bHide ? eAct_Pause : eAct_Resume); }

	//////////////////////////////////////////////////////////////////////////
	// IParticleEmitter implementation.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetEffect( IParticleEffect const* pEffect )	
	{ 
		SetEffect(static_cast<CParticleEffect const*>(pEffect), 0);
	}
	IParticleEffect* GetEffect() const												{ return m_pTopEffect; }
	inline ParticleTarget const& GetTarget() const						{ return m_Target; }

	VIRTUAL void SetTarget( ParticleTarget const& target )
	{
		if ((int)target.bPriority >= (int)m_Target.bPriority)
			m_Target = target;
	}
	VIRTUAL void SetSpawnParams( SpawnParams const& spawnParams, GeomRef geom = GeomRef() );
	VIRTUAL void GetSpawnParams( SpawnParams& spawnParams ) const	
																														{ spawnParams = m_SpawnParams; }
	VIRTUAL bool IsAlive() const															{ return GetState() > eEmitter_Dead; }
	VIRTUAL void Prime();
	VIRTUAL void Activate( bool bActive )											{ Activate(bActive ? eAct_Activate : eAct_Deactivate ); }
	VIRTUAL void Restart()																		{ Activate(eAct_Restart); }
	VIRTUAL void Update()																			{ UpdateEmitter(); }
	VIRTUAL void EmitParticle( IStatObj* pStatObj = NULL, IPhysicalEntity* pPhysEnt = NULL, QuatTS* pLocation = NULL, Vec3* pVel = NULL );
	VIRTUAL void SetEntity( IEntity* pEntity, int nSlot );
	VIRTUAL void Serialize( TSerialize ser );
	void GetMemoryUsage( ICrySizer* pSizer ) const;
	virtual void GetMemoryUsage( ICrySizer* pSizer )					{ return const_cast<const	CParticleEmitter*>(this)->GetMemoryUsage(pSizer); }
  virtual const AABB GetBBox() const { return m_WSBBox; }
  virtual void SetBBox( const AABB& WSBBox ) { m_WSBBox = WSBBox; }
	VIRTUAL bool UpdateStreamableComponents(float fImportance, Matrix34A&  objMatrix, IRenderNode * pRenderNode, float fEntDistance);
	VIRTUAL EntityId GetAttachedEntityId()										{ return m_nEntityId; }
	VIRTUAL int GetAttachedEntitySlot()												{ return m_nEntitySlot; }

	//////////////////////////////////////////////////////////////////////////
	// Other methods.
	//////////////////////////////////////////////////////////////////////////

	const SpawnParams& GetSpawnParams() const
		{ return m_SpawnParams; }
  const AABB& GetCurrentBBox() const
		{ return m_WSBBox; }

	void Register( bool b );
	float GetEmitCountScale() const;
	bool Kill()
	{
		if (m_eMaxState > eEmitter_Dead)
		{
			m_eMaxState = eEmitter_Dead;
			Register(false);
			return true;
		}
		return false;
	}

	EEmitterState UpdateEmitter();
	void UpdateForce();
	EEmitterState GetState() const;
	void Activate( EActivateMode mode, float fPast = 0.f );

	void AddEffect( CParticleContainer* pParentContainer, CParticleSubEmitter* pParentEmitter, CParticleEffect const* pEffect, ParticleParams const* pParams = 0, bool bUpdate = true );
	void SetEffect( CParticleEffect const* pEffect, ParticleParams const* pParams );
	CParticleLocEmitter* CreateIndirectEmitter( CParticleSubEmitter* pParentEmitter, QuatTS const& loc, float fPast );
	void CollideIndirectEmitter( CParticleLocEmitter* pLoc, float fPast = 0.f );

	void SetLoc( QuatTS const& qp );

	SPhysEnviron const& GetUniformPhysEnv() const
	{
		return m_PhysEnviron;
	}
	SVisEnviron const& GetVisEnviron() const
	{
		return m_VisEnviron;
	}
	void OnVisAreaDeleted( IVisArea* pVisArea )
	{
		m_VisEnviron.OnVisAreaDeleted(pVisArea);
	}

	uint32 BoundsTypes() const
	{
		return m_uBoundsTypes;
	}

	float GetParticleScale() const
	{
		// Somewhat obscure. But top-level emitters spawned from entities, 
		// and not attached to other objects, should apply the entity scale to their particles.
		if (m_SpawnParams.eAttachType == GeomType_None)
			return m_SpawnParams.fSizeScale * m_qpLoc.s;
		else
			return m_SpawnParams.fSizeScale;
	}

	float GetMaxParticleSize(bool bAdjusted) const
	{
		float fSize = 0.f;
		for_all_ptrs (const CParticleContainer, c, m_Containers)
			fSize = max(fSize, c->GetParams().GetMaxParticleSize(bAdjusted));
		fSize *= GetParticleScale();
		return fSize;
	}

	void SetUsed( bool b )
	{ 
		for_all_ptrs (CParticleContainer, c, m_Containers)
			c->SetUsed(b);
	}
	void InvalidateStaticBounds()												
	{ 
		for_all_ptrs (CParticleContainer, c, m_Containers)
			c->InvalidateStaticBounds();
	}
	void RenderDebugInfo();
	void UpdateFromEntity();
	float GetMinDrawPixels() const;
	bool IsIndependent() const
	{
		return m_bIndependent;
	}
	bool NeedSerialize() const
	{
		return m_bIndependent && GetEffect() != 0;
	}
	bool IsSelected() const
	{
		return m_bSelected;
	}
	float TimeNotRendered() const
	{
		return GetTimer()->GetFrameStartTime().GetDifferenceInSeconds( m_timeLastRendered );
	}

	void GetCounts( SParticleCounts& counts, bool bEffectStats = false ) const	
	{
		for_all_ptrs (const CParticleContainer, c, m_Containers)
			c->GetCounts(counts, bEffectStats ); 
	}























	void CollectSpuUsage( VecSpuUsageT &vecSpuUsage ) const;
	
protected:

	struct // dummy struct to cast to syn var, needed to prevent dependencies with job compiliation while maintaining the same class layout
	{
		volatile uint32 running;
		uint32 pad[3];
	} _ALIGN(16) m_UpdateParticlesSPUState;

	_smart_ptr<CParticleEffect>	m_pTopEffect;
	_smart_ptr<IMaterial>				m_pMaterial;			// Override material for this emitter.

	SpawnParams									m_SpawnParams;		// External settings modifying emission.

	List<CParticleContainer, ParticleAllocator>		
															m_Containers;
	uint32											m_nEnvFlags;			// Union of environment flags affecting emitters.
	ParticleTarget							m_Target;					// Target set from external source.

	AABB												m_bbWorld,				// World bbox.
															m_bbWorldDyn,			// Dynamically-computed bbox (normally just for debug).
															m_bbWorldEnv;			// Last-queried phys environ bbox.
	uint32											m_uBoundsTypes;		// Type of bounds set.

	// Entity connection params.
	int													m_nEntityId;
	int													m_nEntitySlot;
	bool												m_bIndependent;		// Not controlled by entity.
	bool												m_bSelected;			// Whether entity selected in editor.
	CTimeValue									m_timeLastRendered;
	Vec3												m_vPrevPos;				// For velocity computation.

  AABB												m_WSBBox;
	SPhysEnviron								m_PhysEnviron;		// Common physical environment (uniform forces only) for emitter.
	SVisEnviron									m_VisEnviron;

	// Functions.
	void ClearUnused();
	void Reset()
	{
		for_rev_all_ptrs (CParticleContainer, c, m_Containers)
			c->Reset();
	}

private:
	// first sub-emitter's BindToCamera property cached for fast access by UpdateEmitter()
	bool m_BindToCamera;

	CParticleContainer* AddContainer(CParticleContainer* pParentContainer, const CParticleEffect* pEffect, const ParticleParams* pParams);
};

Declare_GetMemoryUsage(CParticleEmitter)

// inline CParticleSubEmitter implementations.
inline float CParticleSubEmitter::GetAge() const
{
	return GetLoc().GetAge();
}

inline float CParticleSubEmitter::GetEndAge() const
{
	return min(m_fEndAge, GetLoc().GetStopAge());
}

inline CParticleEmitter& CParticleSubEmitter::GetMain() const
{ 
	return GetLoc().GetMain(); 
}

inline void CParticleSubEmitter::SetLastLoc()
{
	m_qpLastLoc = GetLoc().GetLocation();
}

//////////////////////////////////////////////////////////////////////////

#endif // __particleemitter_h__
