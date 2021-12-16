#ifndef PARTICLE_H
#define PARTICLE_H

#include "CryArray.h"
#include "ParticleEnviron.h"
#include "ParticleContainer.h"
#include "ParticleEmitter.h"

class CParticleContainer;
class CParticleSubEmitter;
class CParticleLocEmitter;
class CParticleEmitter;
struct SParticleRenderData;
struct SParticleVertexContext;

struct SParticleUpdateContext
{
	SPhysEnviron	PhysEnv;
	uint32				nEnvFlags, nSpaceLoopFlags;
	Matrix34			matSpaceLoop, matSpaceLoopInv;

	SParticleUpdateContext()
	{
		nEnvFlags = nSpaceLoopFlags = 0;
		matSpaceLoop = matSpaceLoopInv = Matrix34::CreateIdentity();
	}
} _ALIGN(16);

// For particles with tail, keeps history of previous positions.
struct SParticleHistory
{
	float	fAge;
	Vec3	vPos;

	bool IsUsed() const		{ return fAge >= 0.f; }
	void SetUnused()			{ fAge = -1.f; }
} _ALIGN(16);

// dynamic particle data
// To do opt: subclass for geom particles.

#if !defined(PS3)
	#define PARTICLE_MOTION_BLUR	
#endif

class CParticle : public Cry3DEngineBase
{
public:

  CParticle() 
  {
		memset(this, 0, sizeof(*this));
  }
	ILINE ~CParticle()
	{ 
		Destroy(); 
	}

	template<class S, class M> inline
	typename TVarEPParam<S>::T GetMaxValueMod( TVarEPParam<S> const& param, M mod ) const
	{
		typename TVarEPParam<S>::T val = mod;
		return val * param.GetMaxValue();
	}

	void Init( SParticleUpdateContext const& context, float fAge, CParticleSubEmitter* pEmitter, 
		QuatTS const& loc, bool bFixedLoc, const Vec3* pVel = NULL, IStatObj* pStatObj = NULL, IPhysicalEntity* pPhysEnt = NULL );
  void Update( SParticleUpdateContext const& context, float fUpdateTime );
	void GetPhysicsState();

	ILINE bool IsAlive() const
	{
		return m_fAge < m_fLifeTime;
	}

	CParticleLocEmitter* GetChildEmitter() const
	{
		return m_pChildEmitter;
	}

	Vec3 const& GetPos() const
		{ return m_vPos; }

	void Transform( QuatTS const& qp )
	{
		m_qRot = qp.q * m_qRot;
		m_vPos = qp * m_vPos;
		m_vVel = qp.q * m_vVel * qp.s;
	}

	inline bool GetTarget( ParticleTarget& target ) const
	{
		if (!GetParams().TargetAttraction.bIgnore)
		{
			ParticleTarget const& targetMain = GetMain().GetTarget();
			if (m_pEmitter && GetContainer().HasLocalTarget() && !targetMain.bPriority)
			{
				if (m_pEmitter->GetLocalTarget(target))
					return true;
			}
			if (targetMain.bTarget)
			{
				target = targetMain;
				return true;
			}
		}
		return false;
	}

	void UpdateBounds( AABB& bb ) const;

	// Rendering functions.
	bool RenderGeometry( SRendParams& RenParamsShared, SParticleVertexContext& context );
	void AddLight();
	int SetVertices( SVertexParticle aVerts[], SParticleVertexContext& context ) const;
	void ReallocHistory( int nPrevHistorySteps );

protected:

  // Current state/param values, needed for updating.
  float		m_fAge;
	float		m_fRelativeAge;
  Vec3		m_vPos;
	Quat		m_qRot;										// 3D rotational position (Y only for 2D)
	float		m_fAngle;
  Vec3		m_vVel;
	Vec3		m_vRotVel;								// Angular velocity (2D and/or 3D).
	float		m_fClipDistance;					// Distance to nearest clipping border (water, vis area).

	bool		m_bRest;									// Particle at rest, no further updates needed.
	uint8		m_nCollisionCount;				// Number of collisions this particle has had.
	uint8		m_nTileVariant;						// Constant value, moved here for alignment.

#ifdef PARTICLE_MOTION_BLUR
  // Previous state. Used for motion blur (geom particles only atm)
  Vec3		m_vPosPrev;
  Quat		m_qRotPrev;
#endif

	SParticleHistory*			m_aPosHistory;		// History of positions, for tail. Array allocated by container, maintained by particle.

	// Constant values.
	float									m_fLifeTime;

	struct SBaseMods
	{
		TStorageTraits<float>::TMod
								Size,
								TailLength,
								Stretch,

								AirResistance,
								GravityScale,
								Turbulence3DSpeed,
								TurbulenceSize,
								TurbulenceSpeed,
								OrbitDistance,

								LightSourceIntensity,
								LightHDRDynamic,
								LightSourceRadius;

		void Init()
		{
			memset(this, 0xFF, sizeof(*this));
		}
	}											m_BaseMods;

	Color3B					m_BaseColor;
	UnitFloat8 			m_BaseAlpha;

	template<class S, class M>
	SPU_NO_INLINE void SetVarBase( M& mod, TVarEPParam<S> const& param, float fEmissionStrength )
	{
		mod = param.GetVarValue(fEmissionStrength);
	}

	template<class S, class M>
	SPU_NO_INLINE void SetVarMod( M& mod, TVarEPParam<S> const& param, float fEmissionStrength )
	{
		mod = param.GetVarMod(fEmissionStrength);
	}

	// External references.
	CParticleContainer*							m_pContainer;			// Container particle lives in.
	_smart_ptr<CParticleSubEmitter>	m_pEmitter;				// Emitter who spawned this particle, if it still exists.
	IStatObj*												m_pStatObj;				// Geometry for 3D particles.
	IPhysicalEntity*								m_pPhysEnt;				// Attached physical object.
	_smart_ptr<CParticleLocEmitter>	m_pChildEmitter;	// Secondary emitter, if effect has indirect children.

	// Functions.
protected:
	CParticleSubEmitter* GetEmitter() const					{ return m_pEmitter; }
	CParticleContainer& GetContainer() const				{ return *m_pContainer; }
	CParticleLocEmitter& GetLoc() const							{ assert(m_pEmitter != 0); return m_pEmitter->GetLoc(); }
	CParticleEmitter& GetMain() const								{ return m_pContainer->GetMain(); }
	ResourceParticleParams const& GetParams() const	{ return GetContainer().GetParams(); }
	inline float GetScale() const										{ return GetMain().GetParticleScale(); }
	inline float GetSize() const										{ return GetParams().fSize.GetValueFromMod(m_BaseMods.Size, m_fRelativeAge) * GetScale(); }

	inline float GetBaseRadius() const							{ return m_pStatObj ? m_pStatObj->GetRadius() : 1.414f; }
	inline float GetVisibleRadius() const						{ return GetBaseRadius() * GetSize(); }
	inline float GetPhysicalRadius() const					{ return GetVisibleRadius() * GetParams().fThickness; }
	inline Vec3 GetNormal() const										{ return m_qRot.GetColumn1(); }

	void InitPos( SParticleUpdateContext const& context, QuatTS const& loc, float fEmissionRelAge );
	void AddPosHistory( float fCurrentAge, float fRelativeAge, SParticleHistory const& histPrev );
	void UpdateRotation( SParticleUpdateContext const& context, bool bCollided );
	Vec2 VortexRotation( float fCurrentAge, float fRelativeAge );
	bool TargetMovement( ParticleTarget const& target, float fNewAge, SExternForces& forces, float fStepTime, float fRelativeAge );
	
	ILINE void UpdateRelativeAge( float fAge, float fAgeAdjust = 0.f )
	{
		m_fRelativeAge = ComputeRelativeAge(fAge, fAgeAdjust);
	}

	SPU_NO_INLINE float ComputeRelativeAge( float fAge, float fAgeAdjust = 0.f )
	{
		float fRelativeAge = div_min(fAge + fAgeAdjust, m_fLifeTime, 1.f);
		if (!GetParams().fParticleLifeTime && m_pEmitter)
			fRelativeAge = max(fRelativeAge, m_pEmitter->GetRelativeAge(fAgeAdjust));
		assert(fRelativeAge >= 0.f && fRelativeAge <= 1.f);
		return fRelativeAge;		
	}

	void Physicalize( SParticleUpdateContext const& context );
	int GetSurfaceIndex() const;
	void GetCollisionParams(int nCollSurfaceIdx, float& fBounce, float& fDrag);

	void GetRenderLocation( QuatTS& loc ) const;
	void UpdateChildEmitter();
	void EndParticle()
	{
		m_fAge = m_fLifeTime = min(m_fAge, m_fLifeTime);
	}

	void ComputeRenderData( SParticleRenderData& data, SParticleVertexContext& context ) const;
	int FillTailVertBuffer( SVertexParticle aTailVerts[], SParticleVertexContext const& context, float fSize ) const;

	void RotateTo( Vec3 const& vNorm )
	{
		m_qRot = Quat::CreateRotationV0V1( GetNormal(), vNorm ) * m_qRot;
	}

	void Destroy();
} _ALIGN(128);

#endif // PARTICLE
