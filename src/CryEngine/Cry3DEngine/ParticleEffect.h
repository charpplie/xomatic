////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ParticleEffect.h
//  Version:     v1.00
//  Created:     10/7/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __particleeffect_h__
#define __particleeffect_h__
#pragma once

#include "IParticles.h"
#include "Cry3DEngineBase.h"
#include "ParticleParams.h"
#include "CryPtrArray.h"

#define fHUGE			1e18f		// Use for practically infinite values, to simplify comparisons.
													// 1e18 s > 30 billion years, 1e18 m > 100 light years.
													// < sqrt(FLT_MAX), so safe to multiply by itself once.
													// Can subtract up to 1e10 without changing its value, under 32bit precision.
													// 1e10 s > 300 years, 1e10 m > 20x earth-moon distance

//---------------------------------------------------------------------------------------------
// Memory stats support 

#define for_const_container(Type, cont, it) \
	for (Type::const_iterator it = (cont).begin(); it != (cont).end(); ++it)

// Default sizer template, adds 0 extra memory.
template<class T> inline
void GetMemoryUsage( ICrySizer* pSizer, T const& t )
{}

// Macro for standard specialisation
#define Declare_GetMemoryUsage(T) \
	template<> inline void GetMemoryUsage( ICrySizer* pSizer, T const& t ) \
		{ t.GetMemoryUsage(pSizer); }

template<> inline
void GetMemoryUsage( ICrySizer* pSizer, string const& str )
{
	pSizer->AddString(str);
}

template<class T> inline
void GetMemoryUsage( ICrySizer* pSizer, _smart_ptr<T> const& p )
{
	if (p && pSizer->AddObject(p.get(), sizeof(*p)))
		GetMemoryUsage(pSizer, *p);
}

template<class T1, class T2> inline
void GetMemoryUsage( ICrySizer* pSizer, std::pair<T1,T2> const& p )
{
	GetMemoryUsage(pSizer, p.first);
	GetMemoryUsage(pSizer, p.second);
}

// Container sizer
template<class Cont> 
void GetContainerMemoryUsage( ICrySizer* pSizer, Cont const& cont )
{
	if (pSizer->AddContainer(cont))
		for_const_container (typename Cont, cont, it)
			GetMemoryUsage(pSizer, *it);
}

template<class T, class S>
bool AddArrayMemory( ICrySizer* pSizer, const Array<T,S>& arr )
{
	return !arr.empty() && pSizer->AddObject(arr.begin(), arr.get_alloc_size());
}

template<class Cont> 
void GetPtrArrayMemoryUsage( ICrySizer* pSizer, Cont const& cont )
{
	if (AddArrayMemory(pSizer, cont))
		for_const_container (typename Cont, cont, it)
			if (pSizer->Add(*it))
				GetMemoryUsage(pSizer, *it);
}

//////////////////////////////////////////////////////////////////////////
enum EEffectFlags
{
	// Physical phenomena affecting particles.
	EFF_LOADED				= 1,

	ENV_GRAVITY				= 2,
	ENV_WIND					= 4,
	ENV_WATER					= 8,
	ENV_FORCE					= 16,

	// Collision targets
	ENV_TERRAIN				= 32,
	ENV_STATIC_ENT		= 64,
	ENV_DYNAMIC_ENT		= 128,

	// Exclusive rendering methods.
	REN_SPRITE				= 256,
	REN_GEOMETRY			= 512,
	REN_DECAL					= 1024,

	// Visual effects of particles
	REN_TAKE_SHADOWS	= 2048,
	REN_CAST_SHADOWS	= 4096,

	// Additional effects.
	REN_LIGHTS				= 0x2000,		// Adds light to scene.
	REN_SOUND					= 0x4000,		// Produces sound.

	REN_THREAD				= 0x8000,		// Can update in secondary thread.

	REN_ANY						= REN_SPRITE | REN_GEOMETRY | REN_DECAL | REN_LIGHTS | REN_SOUND
};

struct STileInfo
{
	Vec2								vSize;										// Size of texture tile UVs.
	float								fTileCount, fTileStart;		// Range of tiles in texture for emitter.
};

//
// Additional runtime parameters.
//
struct ResourceParticleParams: ParticleParams, Cry3DEngineBase
{
	// Texture, material, geometry params.
	UFloat16							fTexAspect;								// H/V aspect ratio.
	uint16								nTexId;										// Texture id for sprite.
	_smart_ptr<IMaterial>	pMaterial;								// Used to override the material
	_smart_ptr<IStatObj>	pStatObj;									// If it isn't set to 0, this object will be used instead of a sprite
	int16									iPhysMat;									// Surface material for physicalized particles
	uint16								nEnvFlags;								// Summary of environment info needed for effect.
	uint32								nRenObjFlags;							// Flags for renderer.

	ResourceParticleParams()
	{
		Init();
	}

	explicit ResourceParticleParams( const ParticleParams& params )
		: ParticleParams(params)
	{
		Init();
	}

	bool ResourcesLoaded() const
	{
		return nEnvFlags & EFF_LOADED;
	}
	void Correct( int nVersion, XmlNodeRef paramsNode );

	int LoadResources( const char* sName );
	void UnloadResources();
	void UpdateTextureAspect();
	void SetTileInfo( STileInfo& info ) const;

	void ComputeEnvironmentFlags();

	void GetStaticBounds( AABB& bbResult, const QuatTS& loc, const Vec3& vSpawnSize, bool bWithSize, float fMaxLife, const Vec3& vGravity, const Vec3& vWind ) const;
	void GetTravelBounds( AABB& bbResult, float fTime, const QuatTS& loc, bool bOmniDir, const Vec3& vGravity, const Vec3& vWind ) const;
	bool IsImmortal() const
	{
		return bEnabled && ((bContinuous && !fEmitterLifeTime) || fPulsePeriod || !fParticleLifeTime);
	}
	bool HasEquilibrium() const
	{
		// Effect reaches a steady state: No emitter lifespawn or pulsing.
		return bContinuous && fCount && fParticleLifeTime && !fPulsePeriod && !fEmitterLifeTime;
	}
	float GetEquilibriumAge() const
	{
		return fSpawnDelay.GetMaxValue() + fParticleLifeTime.GetMaxValue();
	}
	float GetMaxParticleLife() const
	{
		return fParticleLifeTime ? fParticleLifeTime.GetMaxValue() : fEmitterLifeTime.GetMaxValue();
	}
	float GetMaxParticleSize(bool bAdjusted) const
	{
		float fMaxSize = fSize.GetMaxValue();
		if (pStatObj)
			fMaxSize *= pStatObj->GetRadius();
		if (bAdjusted)
			fMaxSize *= fViewDistanceAdjust;
		return fMaxSize;
	}
	inline bool IsRenderable() const
	{
		return ((nTexId > 0 && nTexId < MAX_VALID_TEXTURE_ID) || pMaterial != 0 || pStatObj != 0)
			&& fSize
			&& (eBlendType != ParticleBlendType_AlphaBased || fAlpha);
	}
	inline bool CanUseGeomShader() const
	{
		// Geometry shader can be used for 4-vertex camera-aligned sprites.
		return eFacing == ParticleFacing_Camera &&
			fTailLength.nTailSteps == 0 && 
			// BUG: Work around current rendering errors with materials and texture tiling (2009-09-29).
			(!pMaterial || TextureTiling.nTilesX*TextureTiling.nTilesY == 0);
	}
	int GetSubGeometryCount() const
	{
		return pStatObj ? pStatObj->GetSubObjectCount() : 0;
	}
	SPU_NO_INLINE IStatObj::SSubObject* GetSubGeometry(int i) const
	{
		assert(i < GetSubGeometryCount());
		IStatObj::SSubObject* pSub = pStatObj->GetSubObject(i);
		return pSub && pSub->nType==STATIC_SUB_OBJECT_MESH && pSub->pStatObj && pSub->pStatObj->GetRenderMesh() ? pSub : 0;
	}

protected:

	void Init()
	{
		nTexId = iPhysMat = 0;
		fTexAspect = 0.f;
		ComputeEnvironmentFlags();
	}

	float GetMaxSpeed(const Vec3& vGravity, const Vec3& vWind) const;
} _ALIGN(64);


/*!	CParticleEffect implements IParticleEffect interface and contain all components necessary to 
		to create the effect
 */
 // nasty workaround for devirtualizer having problems with the stl::intrusive_linked_list_node part
 class CParticleEffect;
 typedef stl::intrusive_linked_list_node<CParticleEffect> ParticleEffect_Intrusive_Linked_List;
class CParticleEffect : public IParticleEffect, public Cry3DEngineBase, public ParticleEffect_Intrusive_Linked_List
{
public:
	CParticleEffect();
	CParticleEffect( const char* sName );
	~CParticleEffect();

	void Release()
	{
		--m_nRefCounter;
		if (m_nRefCounter <= 0)
			delete this;
	}

	// IParticle interface.
	VIRTUAL IParticleEmitter* Spawn( bool bIndependent, const Matrix34& mat );

	VIRTUAL void SetName( const char *sFullName );
	VIRTUAL const char* GetName() const		
		{ return m_strFullName.c_str(); }
	VIRTUAL const char* GetBaseName() const;

	VIRTUAL void SetEnabled( bool bEnabled );
	VIRTUAL bool IsEnabled() const 
		{ return m_pParticleParams && m_pParticleParams->bEnabled; };

	//////////////////////////////////////////////////////////////////////////
	//! Load resources, required by this particle effect (Textures and geometry).
	VIRTUAL bool LoadResources()
		{ return LoadResources(true); }
	VIRTUAL void UnloadResources()
		{ UnloadResources(true); }

	//////////////////////////////////////////////////////////////////////////
	// Child particle systems.
	//////////////////////////////////////////////////////////////////////////
	int GetChildCount() const	 { return m_childs.size(); }
	IParticleEffect* GetChild( int index ) const  { return &m_childs[index]; }

	VIRTUAL void AddChild( IParticleEffect *pEffect );
	VIRTUAL void RemoveChild( IParticleEffect *pEffect );
	VIRTUAL void ClearChilds();
	VIRTUAL void InsertChild( int slot,IParticleEffect *pEffect );
	VIRTUAL int FindChild( IParticleEffect *pEffect ) const;

	VIRTUAL IParticleEffect* GetParent() const 
		{ return m_parent; };

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetParticleParams( const ParticleParams &params );
	VIRTUAL const ParticleParams& GetParticleParams() const 
		{ return GetParams(); };

	void Serialize( XmlNodeRef node, bool bLoading, bool bChildren );
	void GetMemoryUsage( ICrySizer* pSizer ) const;
	
	// Further interface.

	const ResourceParticleParams& GetParams() const;	

	void InstantiateParams()
	{ 
		if (!m_pParticleParams)
			m_pParticleParams = new ResourceParticleParams;
	}

	bool IsNull() const
	{
		// Empty placeholder effect.
		return !m_XmlSource && !m_pParticleParams && m_childs.empty();
	}
	void SetXMLSource( XmlNodeRef node )
	{
		m_XmlSource = node; 
	}
	void LoadFromXML();
	bool LoadResources( bool bAll ) const;
	void UnloadResources( bool bAll ) const;

	CParticleEffect* FindChild( const char* szChildName ) const;
	void Rename( const char* sFullName )
		{ m_strFullName = sFullName; }

	bool IsImmortal() const;
	bool IsIndirect() const;
	float GetMaxParticleLife(bool bAll = true) const;

	// Stats.
	void InitCounts();
	void SumCounts();
	void PrintCounts( bool bHeader = false );
	void ClearCounts();

	void GetEffectCounts( SEffectCounts& counts, bool bDirectOnly = false ) const;

	SSumParticleCounts& GetCounts() const
	{
		assert(m_pCounts);
		return *m_pCounts;
	}

private:

	//////////////////////////////////////////////////////////////////////////
	string													m_strFullName;
	ResourceParticleParams*					m_pParticleParams;

	//! Parenting.
	CParticleEffect*								m_parent;
	SmartPtrArray<CParticleEffect>	m_childs;

	// Cached XML source for delayed loading.
	XmlNodeRef											m_XmlSource;

	// Stats (temporary allocation, used only in interactive profiling).
	SSumParticleCounts*							m_pCounts;
};

Declare_GetMemoryUsage(CParticleEffect)

//
// Utilities
//

#define fDRAG_APPROX_THRESHOLD	0.01f						// Max inaccuracy we allow in fast drag force approximation

inline float TravelDistance( float fV, float fDrag, float fT )
{
	float fDecay = fDrag*fT;
	if (fDecay < fDRAG_APPROX_THRESHOLD)
		return fV*fT * (1.f - fDecay);
	else
		// Compute accurate distance with drag.
		return fV / fDrag * (1.f - expf(-fDecay));
}

struct SExternForces
{
	Vec3	vAccel;
	Vec3	vWind;
	float	fDrag;
};

ILINE void Travel( Vec3& vPos, Vec3& vVel, float fTime, const SExternForces& forces )
{
	// Analytically compute new velocity and position, accurate for any time step.
	if (forces.fDrag * fTime >= fDRAG_APPROX_THRESHOLD)
	{
		//
		// Air resistance proportional to velocity is typical for slower laminar movement.
		// For drag d (units in 1/time), wind W, and gravity G:
		//
		//		V' = d (W-V) + G
		//
		//	The analytic solution is:
		//
		//		VT = G/d + W,									terminal velocity
		//		V = (V-VT) e^(-d t) + VT
		//		X = (V-VT) (1 - e^(-d t))/d + VT t
		//
		//	A faster approximation, accurate to 2nd-order t is:
		//
		//		e^(-d t) => 1 - d t + d² t²/2
		//		X += V t + (G + (W-V) d) t²/2
		//		V += (G + (W-V) d) t
		//

		float fInvDrag = 1.f / forces.fDrag;
		Vec3 vTerm = forces.vWind + forces.vAccel * fInvDrag;
		float fDecay = expf(-forces.fDrag * fTime);
		float fT = (1.0f - fDecay) * fInvDrag;
		vPos += vVel * fT + vTerm * (fTime-fT);
		vVel = vVel * fDecay + vTerm * (1.0f - fDecay);
	}
	else
	{
		// Fast approx drag computation.
		Vec3 vAccel = forces.vAccel + (forces.vWind - vVel) * forces.fDrag;
		vPos += vVel * fTime + vAccel * (fTime*fTime*0.5f);
		vVel += vAccel * fTime;
	}
}












































#endif //__particleeffect_h__
