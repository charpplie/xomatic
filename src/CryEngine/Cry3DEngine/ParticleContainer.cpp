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

#include "StdAfx.h"
#include "ICryAnimation.h"
#include "ParticleContainer.h"
#include "ParticleEnviron.h"
#include "Particle.h"
#include "partman.h"
#include "FogVolumeRenderNode.h"
#include "ISound.h"
#include "IThreadTask.h"

#include <SpuUtils.h>

#ifdef WIN64
	#pragma warning(disable: 4244)
#endif // WIN64





#define fPARTICLE_NON_EQUILUBRIUM_RESET_TIME		float(2.f)		// Time before purging unseen mortal effects
#define fPARTICLE_EQUILIBRIUM_RESET_TIME				float(30.f)		// Time before purging unseen immortal effects
#define fMAX_EQUILIBRIUM_UPDATE_TIME						float(0.25f)	// Update time for non-changing effects
#define fMAX_STATIC_BB_RADIUS										float(1000.f)	// Static bounding box above this size forces dynamic BB

//////////////////////////////////////////////////////////////////////////
CParticleContainer::CParticleContainer( CParticleContainer* pParent, CParticleEmitter* pMain, CParticleEffect const* pEffect, ParticleParams const* pParams )
: m_pMainEmitter(pMain), m_pEffect(pEffect), m_pParams(0), m_nRenderOrder(0)
{
	CRY_ASSERT(pEffect || pParams);

	if (pEffect)
	{
		non_const(*pEffect).AddRef();
		assert(pEffect->IsEnabled());
		m_pEffect->LoadResources(false);
		m_pParams = &m_pEffect->GetParams();
	}
	else if (pParams)
	{
		// Make copy of custom params.
		ResourceParticleParams* pNewParams = new ResourceParticleParams(*pParams);
		pNewParams->LoadResources("(Params)");
		if (!(pNewParams->nEnvFlags & (REN_SPRITE | REN_GEOMETRY)))
		{
			// Assume custom params with no specified texture or geometry can have geometry particles added.
			pNewParams->nEnvFlags |= REN_GEOMETRY;
		}
		m_pParams = pNewParams;
	}

	m_pParentContainer = (m_pEffect && m_pEffect->IsIndirect()) ? pParent : 0;
	if (m_pParentContainer)
	{
		m_pParentContainer->m_nChildFlags |= m_pParams->nEnvFlags;
	}

	// Mark if a parent effect provides a local target for these particles.
	m_bHasLocalTarget = false;
	if (m_pEffect)
	{
		for (IParticleEffect* p = m_pEffect->GetParent(); p; p = p->GetParent())
		{
			if (p->GetParticleParams().eForceGeneration == ParticleForce_Target)
			{
				m_bHasLocalTarget = true;
				break;
			}
		}
	}

	// To do: Invalidate static bounds on updating areas.
	// To do: Use dynamic bounds if in non-uniform areas ??
	m_bbWorld.Reset();
	m_bbWorldStat.Reset();
	m_bbWorldDyn.Reset();
	m_nReserveCount = 0;
	m_pHistoryFree = 0;
	m_nHistorySteps = m_pParams->fTailLength ? m_pParams->fTailLength.nTailSteps : 0;
#ifdef SHARED_GEOM
	m_nInstInfosUsed = 0;
#endif // SHARED_GEOM
	m_fEmitCountScale = 1.f;
	m_nChildFlags = 0;
	m_bUnused = false;
	m_bDynamicBounds = false;






	// calculate the texel area density just once, if required
	m_texelAreaDensity = 0.0f;
	assert(m_pParams->fSize.GetMaxValue() > 0.0f);

	if(m_pParams->nTexId > 0)
	{
		ITexture* pTexture = GetRenderer()->EF_GetTextureByID((int)m_pParams->nTexId);
		if(pTexture)
		{
			// Each particle is rendered as a quad. Its texel area in normalized coordinates is
			// 1/(num tiles in x direction) * 1/(num tiles in y direction).
			const float texelArea = 1.0f / (m_pParams->TextureTiling.nTilesX * m_pParams->TextureTiling.nTilesY);
			const float posArea = (m_pParams->fSize.GetMaxValue() * m_pParams->fSize.GetMaxValue());
			if (posArea != 0)
				m_texelAreaDensity = texelArea / posArea;
			else
				m_texelAreaDensity = 1.0f;
			assert(m_texelAreaDensity > 0.0f);
		}
	}
}

CParticleContainer::~CParticleContainer()
{
	DEBUG_MODIFYLOCK(m_Lock);

	ReleaseParams();
	if (m_pEffect)
	{
		non_const(*m_pEffect).Release();
	}

	// Clear particles before emitters, to remove references.
	m_Particles.clear();
}

void CParticleContainer::ReleaseParams()
{
	if (m_pParams && OwnsParams())
	{
		if (m_pParams->nTexId > 0)
			GetRenderer()->RemoveTexture((int)m_pParams->nTexId);
		non_const(*m_pParams).UnloadResources();
		delete m_pParams;
		m_pParams = 0;
	}
}

void CParticleContainer::UnloadResources()
{
	if (m_pParams && OwnsParams())
	{
		// Unload owned params.
		non_const(*m_pParams).UnloadResources();
	}
}

// Reasonable limits on particle counts;
#define nMAX_SPRITE_PARTICLES			int(0x10000)
#define nMAX_GEOM_PARTICLES 			int(0x1000)
#define nMAX_ATTACHED_PARTICLES		int(0x4000)

SPU_NO_INLINE void CParticleContainer::ComputeReserveCount()
{
	const ResourceParticleParams& params = GetParams();

	if (!m_pParentContainer)
		m_fEmitCountScale = GetMain().GetEmitCountScale();
	else
		m_fEmitCountScale = 1.f;
	float fCount = params.fCount.GetMaxValue() * m_fEmitCountScale;
	m_nReserveCount = int_ceil(fCount);

	if (params.bGeometryInPieces && params.pStatObj != 0)
	{
		// Emit 1 particle per piece.
		int nPieces = 0;
		for (int i = params.GetSubGeometryCount()-1; i >= 0; i--)
		{
			if (params.GetSubGeometry(i))
				nPieces++;
		}
		m_nReserveCount *= max(nPieces,1);
	}

	if (m_pParentContainer)
	{
		// Multiply by parent particle count.
		m_nReserveCount *= m_pParentContainer->m_nReserveCount;
	}

	int nMaxCount = params.pStatObj ? nMAX_GEOM_PARTICLES : nMAX_SPRITE_PARTICLES;
	if (GetMain().m_EmitGeom)
		nMaxCount = min(nMaxCount, nMAX_ATTACHED_PARTICLES);
	if (m_nReserveCount > nMaxCount)
	{
		m_fEmitCountScale *= float(nMaxCount) / m_nReserveCount;
		m_nReserveCount = nMaxCount;
	}
}

SPU_NO_INLINE void CParticleContainer::AllocParticles( int nExtra )
{
	if (m_pParentContainer)
		m_pParentContainer->AllocParticles();
	int nPrevCount = m_nReserveCount;
	ComputeReserveCount();
	m_nReserveCount = max(m_nReserveCount, m_Particles.size() + nExtra);
	m_Particles.reserve(m_nReserveCount);

	// Update history buffer if needed.
	int nPrevSteps = m_nHistorySteps;
	m_nHistorySteps = m_pParams->fTailLength ? m_pParams->fTailLength.nTailSteps : 0;
	if (!m_aHistoryPool.empty() && (m_nHistorySteps != nPrevSteps || m_nReserveCount > nPrevCount))
	{
		// Tail steps changed in editor. Realloc all histories.
		DynArrayAlign<SParticleHistory, 128> aHistNew;
		aHistNew.reserve( m_nHistorySteps * m_nReserveCount );
		aHistNew.swap(m_aHistoryPool);
		m_pHistoryFree = NULL;
		for (TParticleList::traverser it(m_Particles); it; ++it)
			it->ReallocHistory(nPrevSteps);
	}
	else
		m_aHistoryPool.reserve( m_nHistorySteps * m_nReserveCount );
}

SPU_NO_INLINE CParticleSubEmitter* CParticleContainer::AddEmitter( CParticleLocEmitter* pLoc, CParticleSubEmitter* pParentEmitter )
{
	return new (m_Emitters.push_back_new()) CParticleSubEmitter( pParentEmitter, pLoc, this );
}

SPU_NO_INLINE CParticleLocEmitter* CParticleContainer::AddLocEmitter( CParticleEmitter *pEmitter, float fPast )
{
	return new (m_ChildEmitters.push_back_new()) CParticleLocEmitter( pEmitter, fPast );
}

SPU_NO_INLINE CParticle* CParticleContainer::AddParticle( CParticleSubEmitter* pEmitter )
{
	if (m_Particles.size() >= m_Particles.capacity())
		// Reserve count exceeded, should rarely happen (due to inaccuracy in complex emit rate calculation).
		return NULL;

	// Highly approximate sort. Place particle at start or end of list,
	// depending on emitter position relative to bounding box center.
	Vec3 const& vEmitPos = pEmitter->GetEmitPos();
	Vec3 vCamPos = GetCamera().GetPosition();
	float fEmitDistSqr = (vCamPos - vEmitPos).GetLengthSquared();

	Vec3 vSortPos = m_bbWorld.GetCenter();
	float fSortBias = GetParams().GetMaxParticleSize(false);
	if (sqr((vCamPos - vSortPos).GetLength() - fSortBias) > fEmitDistSqr)
		return m_Particles.push_back();
	else
		return m_Particles.push_front();
}

SPU_NO_INLINE void CParticleContainer::DeleteParticle( CParticle* pPart )
{
	m_Particles.erase(pPart);
}

void CParticleContainer::Update()
{
#ifdef SHARED_GEOM
	m_nInstInfosUsed = 0;
#endif // SHARED_GEOM
	if (NeedsDynamicBounds())
	{
		AllocParticles();
		UpdateParticles();
	}
	else if (m_Particles.capacity())
	{
		const float resetTime = HasEquilibrium() ? fPARTICLE_EQUILIBRIUM_RESET_TIME : fPARTICLE_NON_EQUILUBRIUM_RESET_TIME;
		if(GetTimeToUpdate() > (GetMaxParticleLife() + resetTime))
		{
			// Dealloc particle array when last render time is long enough for all particles to have died.
			// Should never conflict with a render thread.
			Reset();
		}
		if(!GetParams().bEnabled)
		{
			Reset();
		}
	}

	// Cull dead emitters, only if a child container.
	// Direct containers must retain their emitter to determine state, and prevent early termination.
	if (m_pParentContainer)
	{
		for_rev_all_ptrs (CParticleSubEmitter, e, m_Emitters)
		{
			if (!e->GetLoc().IsActive())
				if (e->GetRefCount() == 0)
					e = m_Emitters.erase_rev(e);
		}
	}

	// Cull dead child sources.
	for_rev_all_ptrs (CParticleLocEmitter, ce, m_ChildEmitters)
	{
		if (!ce->IsActive())
			if (ce->GetRefCount() == 0)
				ce = m_ChildEmitters.erase_rev(ce);
	}
}

void CParticleContainer::ComputeStaticBounds( AABB& bb, bool bWithSize, float fMaxLife )
{
  FUNCTION_PROFILER_SYS(PARTICLE);

	QuatTS loc = GetMain().GetLocation();
	AABB bbSpawn(AABB::RESET);

	SPhysEnviron const& PhysEnv = GetMain().GetUniformPhysEnv();

	if (m_pParentContainer)
	{
		// Expand by parent spawn volume.
		bool bParentSize = m_pParams->eAttachType != GeomType_None;
		float fMaxParentLife = m_pParams->bSpawnOnParentCollision ? fHUGE : m_pParams->fSpawnDelay.GetMaxValue();
		if (m_pParams->bContinuous)
		{
			if (m_pParams->fEmitterLifeTime)
				fMaxParentLife += m_pParams->fEmitterLifeTime.GetMaxValue();
			else
				fMaxParentLife = fHUGE;
		}
		m_pParentContainer->ComputeStaticBounds( bbSpawn, bParentSize, fMaxParentLife );
		loc.SetIdentity();
		loc.t = bbSpawn.GetCenter();
		bbSpawn.Move(-loc.t);
	}
	else if (GetMain().m_EmitGeom)
	{
		// Expand by attached geom.
		GetAABB(bbSpawn, loc, GetMain().m_EmitGeom);
	}

	Vec3 vSpawnSize(ZERO);
	if (!bbSpawn.IsReset())
	{
		loc.t = loc * bbSpawn.GetCenter();
		vSpawnSize = bbSpawn.GetSize() * 0.5f;
	}

	loc.s = GetMain().GetParticleScale();
	m_pParams->GetStaticBounds(bb, loc, vSpawnSize, bWithSize, fMaxLife, PhysEnv.m_vUniformGravity, PhysEnv.m_vUniformWind);

	if (GetMain().GetTarget().bTarget)
	{
		AABB bbTarg = bb;
		bbTarg.Move(GetMain().GetTarget().vTarget - loc.t);
		bb.Add(bbTarg);
	}
}

SPU_NO_INLINE void CParticleContainer::GetDynamicBounds( AABB& bb )
{
	DEBUG_READLOCK(m_Lock);

	bb.Reset();
	for (TParticleList::traverser pPart(m_Particles); pPart; ++pPart)
	{
		pPart->GetPhysicsState();
		pPart->UpdateBounds( bb );
	}
	assert(bb.IsReset() || bb.GetRadius() < 1e5f);
}

SPU_NO_INLINE uint32 CParticleContainer::UpdateBounds(AABB& bbMain, AABB& bbDyn)
{
	DEBUG_MODIFYLOCK(m_Lock);

	uint32 uBoundsType = 0;

	if (NeedsDynamicBounds())
	{
		m_bbWorld = m_bbWorldDyn;
		uBoundsType = AlphaBit('d');
	}
	else
	{
		if (m_bbWorldStat.IsReset())
			ComputeStaticBounds( SpuStackValue<AABB>(m_bbWorldStat) );
		if (m_bbWorldStat.GetRadius() >= fMAX_STATIC_BB_RADIUS)
		{
			SetDynamicBounds();
			m_bbWorld = m_bbWorldDyn;
			uBoundsType = AlphaBit('d');
		}
		else
		{
			m_bbWorld = m_bbWorldStat;
			uBoundsType = AlphaBit('s');
 			if (!StaticBoundsStable())
			{
				m_bbWorld.Add(m_bbWorldDyn);
				uBoundsType |= AlphaBit('d');
			}
		}
	}

	bbMain.Add(m_bbWorld);
	bbDyn.Add(m_bbWorldDyn);

	m_bbWorldDyn.Reset();

	return uBoundsType;
}

// To do: Add flags for movement/gravity/wind.
SPU_NO_INLINE void CParticleContainer::InvalidateStaticBounds()
{
	if (!NeedsDynamicBounds() && !m_bbWorldStat.IsReset())
	{
		m_bbWorldStat.Reset();
		if (NeedsExtendedBounds())
			m_timeStaticBounds = GetTimer()->GetFrameStartTime() + m_pParams->GetMaxParticleLife();
	}
}

SPU_NO_INLINE void CParticleContainer::ComputeUpdateContext( SParticleUpdateContext& context )
{
	const ResourceParticleParams& params = GetParams();

	context.nEnvFlags = GetEnvironmentFlags();
	if (GetCVars()->e_ParticlesObjectCollisions < 2)
	{
		context.nEnvFlags &= ~ENV_DYNAMIC_ENT;
		if (GetCVars()->e_ParticlesObjectCollisions < 1)
			context.nEnvFlags &= ~ENV_STATIC_ENT;
	}
	if (params.bSpaceLoop)
	{
		// Check for special space-loop collide behavior.
		if (!(GetCVars()->e_ParticlesDebug & AlphaBit('i')))
		if (params.fBounciness < 0.f)
		{
			context.nSpaceLoopFlags = context.nEnvFlags & (ENV_TERRAIN | ENV_STATIC_ENT | ENV_DYNAMIC_ENT);
			context.nEnvFlags &= ~context.nSpaceLoopFlags;
		}

		CCamera const& cam = GetCamera();
		AABB bbSpaceLocal;

		if (params.fCameraMaxDistance > 0.f)
		{
			// Cam-local bounds.
			// Scale cam dist limits to handle zoom.
			float fZoom = 1.f / cam.GetFov();
			float fHSinCos[2]; cry_sincosf( cam.GetHorizontalFov()*0.5f, &fHSinCos[0], &fHSinCos[1] );
			float fVSinCos[2]; cry_sincosf( cam.GetFov()*0.5f, &fVSinCos[0], &fVSinCos[1] );
			float fMin = fHSinCos[1] * fVSinCos[1] * (params.fCameraMinDistance * fZoom);

			bbSpaceLocal.max = Vec3( fHSinCos[0], 1.f, fVSinCos[0] ) * (params.fCameraMaxDistance * fZoom);
			bbSpaceLocal.min = Vec3( -bbSpaceLocal.max.x, fMin, -bbSpaceLocal.max.z );
		}
		else
		{
			// Use emission bounding volume.
			bbSpaceLocal.min = params.vPositionOffset - params.vRandomOffset;
			bbSpaceLocal.max = params.vPositionOffset + params.vRandomOffset;
		}

		// Transform to centered cube, -.5 .. +.5
		context.matSpaceLoop = GetMain().GetMatrix() * Matrix34::CreateScale( bbSpaceLocal.GetSize(), bbSpaceLocal.GetCenter() );
		context.matSpaceLoopInv = context.matSpaceLoop.GetInverted();
	}
}

//////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE void CParticleContainer::UpdateParticles()
{
	assert(m_Particles.capacity() >= m_nReserveCount);









#if !defined(PS3)
	if (GetCVars()->e_ParticlesDebug & AlphaBit('z'))
		return;
#endif // !PS3

	if (!GetParams().bEnabled)
		return;

#if !defined(__SPU__) // no need to lock on SPUs, since the update order ensures no parallel access there
	AUTO_MODIFYLOCK(m_Lock);
#endif // __SPU__

	float fUpdateTime = GetTimeToUpdate();
	if (fUpdateTime > 0.f || m_timeLastUpdate == CTimeValue())
	{
	  FUNCTION_PROFILER_CONTAINER(this);

		if (m_pParentContainer)
		{
			m_pParentContainer->UpdateParticles();
		}

		ZeroStruct(m_Counts);

		// spus do not support function static variables, also this is only used with the particlethread which is inactive with spus
#if !defined(__SPU__)
		static uint32 g_RenderOrder = 0;
		m_nRenderOrder = g_RenderOrder++;
#endif // __SPU__

		// Limit update time for steady state.
		if (fUpdateTime > fMAX_EQUILIBRIUM_UPDATE_TIME && m_timeLastUpdate != CTimeValue() && HasEquilibrium())
		{
			if (GetMain().GetAge() - fUpdateTime >= GetEquilibriumAge())
			{
				fUpdateTime = fMAX_EQUILIBRIUM_UPDATE_TIME;
				m_timeLastUpdate = GetTimer()->GetFrameStartTime() - CTimeValue(fUpdateTime);
			}
		}

		// Erase all dead particles.
		DeleteDeadParticles();

		SParticleUpdateContext context;
		ComputeUpdateContext(context);
		if (GetMain().GetUniformPhysEnv().m_nNonUniformFlags & context.nEnvFlags)
		{
#if !defined(__SPU__)
			context.PhysEnv.GetPhysAreas( &GetMain(), m_bbWorld, context.nEnvFlags, &m_aNonUniformAreas );


#endif // !__SPU__
		}
		else
			context.PhysEnv = GetMain().GetUniformPhysEnv();

		if (fUpdateTime > 0.f)
		{
			// Emit new particles.
			EmitParticles( context );

			// Update all particle positions etc.
			UpdateParticleStates( context, fUpdateTime );

			for_all_ptrs (CParticleSubEmitter, e, m_Emitters)
				e->SetLastLoc();
		}

		m_timeLastUpdate = GetTimer()->GetFrameStartTime();

		context.PhysEnv.Clear();
	}

	// Update dynamic bounds if required.
	if (NeedsDynamicBounds() || !StaticBoundsStable() || GetMain().IsSelected() || (GetCVars()->e_ParticlesDebug & AlphaBit('b')))
		GetDynamicBounds( SpuStackValue<AABB>(m_bbWorldDyn) );
}

SPU_NO_INLINE void CParticleContainer::DeleteDeadParticles()
{
	if (HasExtendedLifetime())
		return;
	
	for (TParticleList::traverser it(m_Particles); it; )
	{
		if (!it->IsAlive())
			it.erase();
		else
			++it;
	}
}

SPU_NO_INLINE void CParticleContainer::UpdateParticleStates( const SParticleUpdateContext &context, float fUpdateTime )
{
	// Evolve & cull existing sprites.
	for (TParticleList::traverser it(m_Particles); it; ++it)
	{
		it->Update( context, fUpdateTime );
	}
}

SPU_NO_INLINE void CParticleContainer::EmitParticles( SParticleUpdateContext const& context )
{
  FUNCTION_PROFILER_CONTAINER(this);

	for_all_ptrs (CParticleSubEmitter, e, m_Emitters)
	{
		e->Update();
		e->EmitParticles( context );
	}
}

void CParticleContainer::UpdateSound()
{
	if (GetEnvironmentFlags() & REN_SOUND)
	{
		for_all_ptrs (CParticleSubEmitter, e, m_Emitters)
		{
			e->UpdateSound();
		}
	}
}

void CParticleContainer::UpdateForce()
{
	if (GetEnvironmentFlags() & ENV_FORCE)
	{
		for_all_ptrs (CParticleSubEmitter, e, m_Emitters)
			e->UpdateForce();
	}
}

/* Water sorting / filtering:

									Effect:::::::::::::::::::::::::::
Emitter	Camera		above					both					below
-------	------		-----					----					-----
above		above			AFTER					AFTER					skip
				below			BEFORE				BEFORE				skip

both		above			AFTER\below		AFTER[\below]	BEFORE\above
				below			BEFORE\below	AFTER[\above]	AFTER\above

below		above			skip					BEFORE				BEFORE
				below			skip					AFTER					AFTER
*/



void CParticleContainer::Render( SRendParams const& RenParams, SPartRenderParams& PRParams )
{
  FUNCTION_PROFILER_CONTAINER(this);

	if (!m_pParams->bEnabled)
		return;

	uint32 nRenderFlags = GetEnvironmentFlags() & PRParams.m_nRenFlags;
	if (!nRenderFlags)
		return;

	// Culling tests.
	if (Get3DEngine()->_GetRenderIntoShadowmap())
		if (!m_pParams->bCastShadows)
			return;

	if (m_pParams->eFacing == ParticleFacing_Water && m_nRenderStackLevel > 0)
		return;

	bool bAfterWater = true;
	if (!GetMain().GetSpawnParams().bIgnoreLocation)
	{
		if (!TrinaryMatch(m_pParams->tVisibleUnderwater, GetMain().GetUniformPhysEnv().m_tUnderWater))
			return;
		if (!TrinaryMatch(m_pParams->tVisibleIndoors, GetMain().GetVisEnviron().OriginIndoors()))
			return;

		bAfterWater = TrinaryMatch(m_pParams->tVisibleUnderwater, Get3DEngine()->IsCameraUnderWater())
			&& TrinaryMatch(GetMain().GetUniformPhysEnv().m_tUnderWater, Get3DEngine()->IsCameraUnderWater());
	}

	// Individual container distance culling.
	float fDist = m_bbWorld.GetDistance(PRParams.m_vCamPos);
	if (m_pParams->fCameraMaxDistance > 0.f && fDist * GetRenderer()->GetCamera().GetFov() > m_pParams->fCameraMaxDistance)
		return;

	float fMaxSize = m_pParams->GetMaxParticleSize(true);
	if (fMaxSize * PRParams.m_fAngularRes < fDist * GetMain().GetMinDrawPixels())
		return;

	// workaround: special case for when inside amphibious vehicle
	if ((m_pParams->bOceanParticle || m_pParams->eFacing == ParticleFacing_Water) && 
		(Get3DEngine()->GetOceanRenderFlags() & OCR_NO_DRAW) )
		return;

	AllocParticles();






	if (nRenderFlags & REN_LIGHTS)
	{
		RenderLights();
	}

	if (nRenderFlags & REN_GEOMETRY)
	{
		RenderGeometry( RenParams, PRParams );
	}
	else if (nRenderFlags & REN_DECAL)
	{
		UpdateParticles();
	}

	if (nRenderFlags & REN_SPRITE)
	{
		if (!CanThread())
		{
			// Update particles in main thread.
			UpdateParticles();
		}

		if (m_bbWorld.IsReset())
		{
			UpdateBounds(m_bbWorld, m_bbWorldDyn);
		}

		// Create emitter RenderObject.
		CRenderObject* pRenderObj = GetIdentityCRenderObject();
		if (!pRenderObj)
		{
			return;
		}

		// Copy pre-computed render flags.
		pRenderObj->m_RState = m_pParams->nRenObjFlags & 0x1F;
		pRenderObj->m_ObjFlags |= m_pParams->nRenObjFlags & ~0x1F;
		if((pRenderObj->m_ObjFlags | RenParams.dwFObjFlags) & FOB_NEAREST)
		{
			pRenderObj->m_ObjFlags &= ~FOB_SOFT_PARTICLE;
		}

		if (m_pParams->bReceiveShadows && RenParams.pShadowMapCasters != 0 && RenParams.pShadowMapCasters->Count() != 0 )
		{
			pRenderObj->m_pShadowCasters = RenParams.pShadowMapCasters;
			pRenderObj->m_ObjFlags |= FOB_INSHADOW;
		}
		
		// Ambient color for shader incorporates actual ambient lighting, as well as the constant emissive value.
		float fEmissive = m_pParams->fEmissiveLighting;
		fEmissive *= powf( PRParams.m_fHDRDynamicMultiplier, m_pParams->fEmissiveHDRDynamic );
		pRenderObj->m_II.m_AmbColor = ColorF(fEmissive) + RenParams.AmbientColor * m_pParams->fDiffuseLighting;

		// Ambient.a is used to modulate dynamic lighting.
		if (m_pParams->eFacing != ParticleFacing_Camera)
		{
			// Disable directional lighting for 3D particles, proper normals are not yet supported.
			pRenderObj->m_II.m_AmbColor.a = 0;
		}
		else
		{
			pRenderObj->m_II.m_AmbColor.a = m_pParams->fDiffuseLighting;
		}

		if (m_pParams->fDiffuseLighting > 0.f)
		{
			pRenderObj->m_DynLMMask[m_nRenderThreadListID] = RenParams.nDLightMask;
		}
		else
		{
			pRenderObj->m_DynLMMask[m_nRenderThreadListID] = 0;
		}

    SRenderObjData *pOD = GetRenderer()->EF_GetObjData(pRenderObj, true);
		pRenderObj->m_nTextureID = m_pParams->pMaterial ? -1 : m_pParams->nTexId;
		pOD->m_FogVolumeContribIdx[0] = PRParams.m_nFogVolumeContribIdx;
		pOD->m_FogVolumeContribIdx[1] = PRParams.m_nFogVolumeContribIdx;
		pRenderObj->m_nMotionBlurAmount = clamp_tpl(int_round(m_pParams->fMotionBlurScale * 128), 0, 255);

		// Set texture tiling params for particle shader in 1st 4 temp vars.
		assert(sizeof(STileInfo) == 4*sizeof(*pOD->m_fTempVars));
		m_pParams->SetTileInfo( *(STileInfo*)pOD->m_fTempVars );

		// Construct sort offset, using DrawLast param & effect order.
		pRenderObj->m_fDistance = PRParams.m_fCamDistance;
		pRenderObj->m_fSort = -(clamp_tpl((int)m_pParams->nDrawLast, -100, 100) + PRParams.m_nEmitterOrder * 0.01f);
		PRParams.m_nEmitterOrder++;

		if (m_pParams->bOceanParticle)
		{
			if (GetCVars()->e_WaterOceanSoftParticles)
			{
				pRenderObj->m_ObjFlags |= FOB_OCEAN_PARTICLE;
				pRenderObj->m_RState |= OS_NODEPTH_TEST;              
			}
		}

		IMaterial* pMatToUse = m_pParams->pMaterial;
		if (!pMatToUse)
		{
			pMatToUse = CPartManager::GetManager()->GetLightShader();
		}
		bool bCanUseGeomShader = m_pParams->CanUseGeomShader();
		GetRenderer()->EF_AddParticlesToScene( pMatToUse->GetShaderItem(), pRenderObj, this, bAfterWater, bCanUseGeomShader );
	}
	
}

SPU_NO_INLINE bool CParticleContainer::CanThread()
{
	uint32 nEnvFlags = SPU_MAIN_PTR(this)->GetEnvironmentFlags();

	if (!(nEnvFlags & REN_THREAD))
		return false;

	if (GetMain().m_EmitGeom.m_pChar)
		// Access to character data.
		return false;

	// for SPUs also skip sound and other things





















		return true;

}

bool CParticleContainer::IsImmortal() const
{
	return GetParams().IsImmortal() || GetMain().GetSpawnParams().fPulsePeriod != 0.f;
}

bool CParticleContainer::HasEquilibrium() const
{
	if (m_pParentContainer)
		return m_pParentContainer->HasEquilibrium();
	if (GetMain().GetSpawnParams().fPulsePeriod > 0.f)
		return false;
	if (GetMain().GetAge() >= GetMain().GetStopAge())
		return false;
	return GetParams().HasEquilibrium();
}

float CParticleContainer::GetEquilibriumAge(bool bAll) const
{
	float fStableAge = m_timeStaticBounds.GetDifferenceInSeconds( GetMain().GetTimeCreated() );
	float fEquilibriumAge = GetParams().fSpawnDelay.GetMaxValue() + GetMaxParticleLife(bAll);
	if (m_pParentContainer)
		fEquilibriumAge += m_pParentContainer->GetEquilibriumAge(false);
	return max(fStableAge, fEquilibriumAge);
}

float CParticleContainer::TimeNotRendered() const
{
	return GetMain().TimeNotRendered();
}

SPU_NO_INLINE float CParticleContainer::GetTimeToUpdate() const
{
	if (m_timeLastUpdate == CTimeValue())
		return GetMain().GetAge();
	else
		return GetTimer()->GetFrameStartTime().GetDifferenceInSeconds( m_timeLastUpdate );
}

SPU_NO_INLINE SParticleHistory* CParticleContainer::AllocPosHistory()
{
	if (!m_nHistorySteps)
		return NULL;

	// Get free list.
	if (m_pHistoryFree)
	{
		SParticleHistory* aFree = m_pHistoryFree;
		m_pHistoryFree = *(SParticleHistory**)m_pHistoryFree;
		return aFree;
	}

	// Use pool.
	assert(m_aHistoryPool.size()+m_nHistorySteps <= m_aHistoryPool.capacity());
	m_aHistoryPool.resize(m_aHistoryPool.size()+m_nHistorySteps);
	return m_aHistoryPool.end() - m_nHistorySteps;
}

SPU_NO_INLINE void CParticleContainer::FreePosHistory( SParticleHistory* aHist)
{
	// Add to head of free list.
	assert(aHist >= m_aHistoryPool.begin() && aHist < m_aHistoryPool.end());
	*(SParticleHistory**)aHist = m_pHistoryFree;
	m_pHistoryFree = aHist;
}

SPU_NO_INLINE void CParticleContainer::Reset()
{
	DEBUG_MODIFYLOCK(m_Lock);

	m_Particles.clear();
	m_aHistoryPool.clear();
#ifdef SHARED_GEOM
	m_InstInfos.clear();
	m_nInstInfosUsed = 0;
#endif // SHARED_GEOM
	m_pHistoryFree = 0;
}

// Stat functions.
void CParticleContainer::GetCounts( SParticleCounts& counts, bool bEffectStats ) const
{
	float fEmitters = (float)m_Emitters.size();
	counts.EmittersAlloc += fEmitters;
	counts.ParticlesAlloc += m_Particles.capacity();
	if (GetTimeToUpdate() == 0.f)
	{
		// Was updated this frame.
		counts.EmittersActive += fEmitters;
		counts.ParticlesActive += m_Particles.size();
		AddArray(FloatArray(counts), FloatArray(m_Counts));
	}

	if (bEffectStats && m_pEffect)
		GetCounts(m_pEffect->GetCounts(), false);
}

// Reference counting redirected to main emitter, as emitter with its containers deleted all at once.
void CParticleContainer::AddRef()
{
	GetMain().AddRef();
}
void CParticleContainer::Release()
{
	assert(GetMain().GetRefCount() > 1);
	GetMain().Release();
}

void CParticleContainer::GetMemoryUsage(ICrySizer* pSizer) const
{
	m_Particles.GetMemoryUsage(pSizer);	
	AddArrayMemory(pSizer, m_aHistoryPool);
	m_Emitters.GetMemoryUsage(pSizer);
	m_ChildEmitters.GetMemoryUsage(pSizer);
}

void CParticleContainer::CollectSpuUsage( VecSpuUsageT &vecSpuUsage ) const
{


















































}

























#undef USE_SPU


