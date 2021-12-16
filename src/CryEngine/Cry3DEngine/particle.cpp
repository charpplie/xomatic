////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   particle.cpp
//  Created:     28/5/2001 by Vladimir Kajalin
//  Modified:    11/3/2005 by Scott Peter
//  Compilers:   Visual Studio.NET
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "Particle.h"
#include "ParticleEmitter.h"
#include "ParticleContainer.h"
#include "terrain.h"
#include "GeomQuery.h"
#include "MatMan.h"

#include <SpuUtils.h>

#define OT_RIGID_PARTICLE 10

#define fMAX_COLLIDE_DEVIATION	float(0.1f)
#define fMAX_TAIL_DEVIATION			float(0.01f)
#define fMAX_NONUNIFORM_TRAVEL	float(0.25f)
#define fCOLLIDE_BUFFER_DIST		float(0.001f)
#define fREST_VEL								float(0.001f)
#define ATTACH_BUFFER_DIST			float(0.01f)





#define nMAX_ITERATIONS					int(6)
#define fMAX_ITERATIONS					float(6.0f)




float InterpVariance(Vec3 const& v0, Vec3 const& v1, Vec3 const& v2)
{
	/*
		(dist(v0,v1) + dist(v1,v2))^2 - dist(v0,v2)^2
		= dist(v0,v1)^2 + dist(v1,v2)^2 + 2 dist(v0,v1) dist(v1,v2) - dist(v0,v2)^2
	*/
	Vec3 v01 = v1-v0,
			 v12 = v2-v1,
			 v02 = v2-v0;
	return v01*v01 + v12*v12 - v02*v02 + 2.f * v01.GetLength() * v12.GetLength();
}

SPU_NO_INLINE float ComputeMaxStep( Vec3 const& vDeviate, float fStepTime, float fMaxDeviate )
{
	float fDevSqr = vDeviate.GetLengthSquared();
	if (fDevSqr > sqr(fMaxDeviate))
		return fMaxDeviate * isqrt_tpl(fDevSqr) * fStepTime;
	return fStepTime;
}

SPU_NO_INLINE void TravelLinear( Vec3& vPos, Vec3& vVel, float& fTime, const SExternForces& forces, float fMaxDev, float fMinTime)
{
	// First limit path curve deviation to max 90 deg.
	float fNewTime = fTime;

	// Limit travel time to stay within max deviation from linear path.
	Vec3 vPos0 = vPos, vVel0 = vVel;
	float fBendTime = div_min(-(vVel*vVel), vVel*forces.vAccel, fNewTime);
	fNewTime = (float)__fsel(-fBendTime, fNewTime, min(fNewTime, max(fBendTime, fMinTime)));

	for (;;)
	{
		Travel( vPos, vVel, fNewTime, forces );

		Vec3 vDev = vVel-vVel0;
		const float fTimeModSqr = sqr(fNewTime*0.125f);
		float fDevSq = vDev.GetLengthSquaredFloat() * fTimeModSqr;
		if (fDevSq > sqr(fMaxDev))
		{
			Vec3 vLin = (vPos-vPos0).GetNormalized();
			vDev -= vLin * (vDev*vLin);
			fDevSq = vDev.GetLengthSquared() * fTimeModSqr;
			if (fDevSq > sqr(fMaxDev))
			{
				float fLinTime = sqrt_tpl(fMaxDev * isqrt_fast_tpl(fDevSq)) * fNewTime * 0.75f;
				fLinTime = max(fLinTime, fMinTime);
				if (fLinTime < fNewTime)
				{
					fNewTime = fLinTime;
					vPos = vPos0;
					vVel = vVel0;
					continue;
				}
			}
		}
		break;
	}

	fTime = fNewTime;
}

SPU_NO_INLINE void CParticle::Init( SParticleUpdateContext const& context, float fAge, CParticleSubEmitter* pEmitter, 
	QuatTS const& loc, bool bFixedLoc, const Vec3* pVel, IStatObj* pStatObj, IPhysicalEntity* pPhysEnt )
{
	assert(pEmitter);
	m_pEmitter = pEmitter;
	m_pContainer = &pEmitter->GetContainer();

	CParticleContainer &rContainer = GetContainer();
	ResourceParticleParams const& params = rContainer.GetParams();

	// Init all base values.
	float fEmissionStrength = pEmitter->GetStrength(-fAge);

	m_fAge = m_fRelativeAge = 0.f;
	if (!params.fParticleLifeTime)
		m_fLifeTime = fHUGE;
	else
		m_fLifeTime = params.fParticleLifeTime.GetVarValue(fEmissionStrength);

	// Init base mods.
	SetVarMod( m_BaseMods.Size, params.fSize, fEmissionStrength );
	SetVarMod( m_BaseMods.TailLength, params.fTailLength, fEmissionStrength );
	SetVarMod( m_BaseMods.Stretch, params.fStretch, fEmissionStrength );

	SetVarMod( m_BaseMods.AirResistance, params.fAirResistance, fEmissionStrength );
	SetVarMod( m_BaseMods.GravityScale, params.fGravityScale, fEmissionStrength );
	SetVarMod( m_BaseMods.Turbulence3DSpeed, params.fTurbulence3DSpeed, fEmissionStrength );
	SetVarMod( m_BaseMods.TurbulenceSize, params.fTurbulenceSize, fEmissionStrength );
	SetVarMod( m_BaseMods.TurbulenceSpeed, params.fTurbulenceSpeed, fEmissionStrength );
	SetVarMod( m_BaseMods.OrbitDistance, params.TargetAttraction.fOrbitDistance, fEmissionStrength );

	SetVarMod( m_BaseMods.LightSourceIntensity, params.LightSource.fIntensity, fEmissionStrength );
	SetVarMod( m_BaseMods.LightHDRDynamic, params.LightSource.fHDRDynamic, fEmissionStrength );
	SetVarMod( m_BaseMods.LightSourceRadius, params.LightSource.fRadius, fEmissionStrength );
			
	SetVarBase( m_BaseColor, params.cColor, fEmissionStrength );
	SetVarBase( m_BaseAlpha, params.fAlpha, fEmissionStrength );

	m_nTileVariant = Random(256);

	IF (pPhysEnt && pStatObj, false)
	{
		// Pre-generated physics entity.
		m_pPhysEnt = pPhysEnt;
		m_pPhysEnt->AddRef();
		m_pStatObj = pStatObj;
		pe_params_foreign_data pfd;
		pfd.iForeignData = OT_RIGID_PARTICLE;
		pfd.pForeignData = pStatObj;
		pPhysEnt->SetParams(&pfd);

		// Get initial state.
		GetPhysicsState();
		rContainer.SetDynamicBounds();
	}
	else
	{
		// Set geometry.
		m_pStatObj = pStatObj ? pStatObj : (IStatObj*)params.pStatObj;
		if (!bFixedLoc || !pVel)
			InitPos( context, loc, fEmissionStrength );
		if (bFixedLoc)
		{
			m_vPos = loc.t;
			m_qRot = loc.q;
			rContainer.SetDynamicBounds();
		}
		if (pVel)
		{
			m_vVel = *pVel;
			rContainer.SetDynamicBounds();
		}

		if (rContainer.GetHistorySteps() > 0)
		{
			// Init history list.
			m_aPosHistory = GetContainer().AllocPosHistory();
			for (int n = 0; n < rContainer.GetHistorySteps(); n++)
				m_aPosHistory[n].SetUnused();
		}

		IF ( params.ePhysicsType >= ParticlePhysics_SimplePhysics, false )
		{
			// Emitter-generated physical particles.
			Physicalize( context );
			rContainer.SetDynamicBounds();
		}
	}

#ifdef PARTICLE_MOTION_BLUR
  m_vPosPrev = m_vPos;
  m_qRotPrev = m_qRot;
#endif

	// Add reference to the parent of the stat obj.
	if (m_pStatObj)
		m_pStatObj->AddRef();
	if (m_pPhysEnt && GetMain().GetVisEnviron().OriginIndoors())
	{
		pe_params_flags pf;
		pf.flagsOR = pef_ignore_ocean;
		m_pPhysEnt->SetParams(&pf);
	}

	if (params.eFacing == ParticleFacing_Decal)
	{
#if !defined(__SPU__) // no support for Decals from Particles for now		
		// Spawn associated decal.
		CryEngineDecalInfo decal;
		/*
		decal.bSkipOverlappingTest = true;
		decal.bAssemble = false;
		decal.bAdjustPos = true;
		*/
		// decal.bQueryObjects = true;

		decal.fLifeTime = m_fLifeTime;			// Todo: Manually delete instead? Inf lifetime? Use custom alpha settings?
		if (params.pMaterial)
			strncpy(decal.szMaterialName, params.sMaterial.c_str(), sizeof(decal.szMaterialName));
		else
			// Todo: support texture tile.
			strncpy(decal.szTextureName, params.sTexture.c_str(), sizeof(decal.szTextureName));

		decal.vNormal = GetNormal();
		decal.vPos = m_vPos + decal.vNormal * ATTACH_BUFFER_DIST;
		decal.fAngle = atan2_tpl(m_qRot.v.x, m_qRot.v.z);
		decal.fSize = GetMaxValueMod(GetParams().fSize, m_BaseMods.Size) * GetScale();
		// decal.fGrowTime = decal.fLifeTime;
		decal.vHitDirection = GetLoc().GetVel().GetNormalized();

		if (GetLoc().m_EmitGeom.m_pPhysEnt)
			decal.ownerInfo.pRenderNode = (IRenderNode*)GetLoc().m_EmitGeom.m_pPhysEnt->GetForeignData(PHYS_FOREIGN_ID_STATIC);

		// Big todo: adjust particle size, alpha, [pos] from callback.

		m_p3DEngine->CreateDecal(decal);


#endif
	}

	// Spawn any indirect emitters.
	if (GetContainer().GetChildFlags() & REN_ANY)
	{
		QuatTS l;
		GetRenderLocation(l);
		m_pChildEmitter = GetMain().CreateIndirectEmitter(pEmitter, l, fAge);
		if (m_pChildEmitter)
			UpdateChildEmitter();
	}

	m_fAge = -fAge;
}

////////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE void CParticle::InitPos( SParticleUpdateContext const& context, QuatTS const& loc, float fEmissionStrength )
{	
	ResourceParticleParams const& params = GetParams();

	// Position and direction.
	m_vPos = loc.t;
	m_qRot = loc.q;

	m_vVel = GetNormal();

	// Emit geom.
  assert(GetEmitter());
	CParticleSubEmitter& rEmitter = *GetEmitter();
	CParticleLocEmitter& rLoc = GetLoc();

	if (rLoc.m_EmitGeom)
	{
		EGeomType eAttachType = params.bSecondGeneration ? +params.eAttachType : GetMain().GetSpawnParams().eAttachType;
		if (eAttachType != GeomType_None)
		{
			EGeomForm	eAttachForm = params.bSecondGeneration ? +params.eAttachForm : GetMain().GetSpawnParams().eAttachForm;
			RandomPos ran;
			QuatTS locGeom = loc;
			if (GetRandomPos( ran, rLoc.m_GeomQuery, rLoc.m_EmitGeom, eAttachForm, locGeom ) && !ran.vNorm.IsZero())
			{
				m_vPos = ran.vPos;
				m_vVel = ran.vNorm;
			}
		}
	}

	// Offsets.
	if (params.bSpaceLoop && params.fCameraMaxDistance > 0.f)
	{
		RandomPos ran;
		BoxRandomPos( ran, GeomForm_Volume, Vec3(0.5f) );
		m_vPos = context.matSpaceLoop * ran.vPos;
	}
	else
	{
		Vec3 vOffset = params.vPositionOffset;
		if (params.fPosRandomOffset)
			vOffset += SphereRandom(params.fPosRandomOffset);
		if (!params.vRandomOffset.IsZero())
			vOffset += BiRandom(params.vRandomOffset);

		// To world orientation/scale.
		m_vPos += loc.q * vOffset * GetScale();
	}

	// Emit direction.
	if (params.bFocusGravityDir)
	{
		float fLenSq = context.PhysEnv.m_vUniformGravity.GetLengthSquaredFloat();
		if (fLenSq > FLT_MIN)
			m_vVel = context.PhysEnv.m_vUniformGravity * -isqrt_tpl(fLenSq);
	}

	float fFocusAngle = rEmitter.GetCurValue(params.fFocusAngle);
	if (fFocusAngle != 0.f)
	{
		Quat q;
		if (params.bFocusGravityDir && !context.PhysEnv.m_vUniformGravity.IsZero())
			q.SetRotationV0V1(Vec3(0,1,0), m_vVel);
		else
			q = loc.q;

		// Rotate focus about X.
		float fAzimuth = rEmitter.GetCurValue(params.fFocusAzimuth, 360.f);
		Vec3 vRot = q * Vec3(1,0,0);
		m_vVel = Quat::CreateRotationAA(DEG2RAD(fAzimuth), m_vVel) * (Quat::CreateRotationAA(DEG2RAD(fFocusAngle), vRot) * m_vVel);
	}

	// Velocity.
	float fPhi = params.fEmitAngle.GetVarValue(fEmissionStrength);
	if (fPhi > 0.f)
	{
		//
		// Adjust angle to create a uniform distribution.
		//
		//		density distribution d(phi) = sin phi
		//		cumulative density c(phi) = Int(0,phi) sin x dx = 1 - cos phi
		//		normalised cn(phi) = (1 - cos phi) / (1 - cos phiMax)
		//		reverse function phi(cn) = acos_tpl(1 + cn(cos phiMax - 1))
		//

		float fPhiMax = params.fEmitAngle.GetMaxValue();
		fPhi /= fPhiMax;
		fPhi = cry_acosf( 1.f + fPhi * (cry_cosf(DEG2RAD(fPhiMax))-1.f) );

		float fTheta = Random(DEG2RAD(360));

		float fPhiCS[2], fThetaCS[2];
		sincos_tpl(fPhi, &fPhiCS[1], &fPhiCS[0]);
		sincos_tpl(fTheta, &fThetaCS[1], &fThetaCS[0]);

		// Compute perpendicular bases.
		Vec3 vX;
		if (m_vVel.z != 0.f)
			vX( 0.f, -m_vVel.z, m_vVel.y );
		else
			vX( -m_vVel.y, m_vVel.x, 0.f );
		vX.NormalizeFast();
		Vec3 vY = m_vVel ^ vX;

		m_vVel = m_vVel * fPhiCS[0] + (vX * fThetaCS[0] + vY * fThetaCS[1]) * fPhiCS[1];
	}

	// Speed.
	float fSpeed = params.fSpeed.GetVarValue(fEmissionStrength);
	m_vVel *= fSpeed * GetMain().GetSpawnParams().fSpeedScale * GetScale();
	if (!params.bMoveRelEmitter)
		m_vVel += rLoc.GetVel() * params.fInheritVelocity;

	// Confine horizontal particles.
	if (params.eFacing == ParticleFacing_Horizontal)
	{
		Vec3 vY = GetNormal();
		m_vVel -= vY * (vY * m_vVel);
	}

	// Initial orientation.
	if (params.eFacing != ParticleFacing_Free)
	{
		// 2D relative rotation about Y.
		m_fAngle = DEG2RAD( params.vInitAngles.y + BiRandom(params.vRandomAngles.y) );
		m_vRotVel.y = DEG2RAD( params.vRotationRate.y + BiRandom(params.vRandomRotationRate.y) );
	}
	else
	{
		// 3D absolute rotation. Compute quat from init angles.
		Vec3 vAngles = DEG2RAD( params.vInitAngles + BiRandom(params.vRandomAngles) );
		// Convert to world space.
		m_qRot = loc.q * exp( vAngles * 0.5f );
		m_vRotVel = loc.q * DEG2RAD( params.vRotationRate + BiRandom(params.vRandomRotationRate) );
	}

	// Rotate to correct alignment.
	UpdateRotation( context, false );
}

SPU_NO_INLINE void CParticle::UpdateBounds( AABB& bb ) const
{
	if (!IsAlive())
		return;
	ParticleParams const& params = GetParams();
	if (m_pStatObj)
	{
		float fRadius = m_pStatObj->GetRadius();
		if (params.bNoOffset || params.ePhysicsType == ParticlePhysics_RigidBody)
			// Add object origin.
			fRadius += m_pStatObj->GetAABB().GetCenter().GetLength();
		bb.Add(m_vPos, fRadius * GetSize());
	}
	else
	{
		// Radius is size * sqrt(2), round it up slightly.
		float fRadius = GetSize() * 1.415f;
		bb.Add(m_vPos, fRadius);

		if (params.fStretch)
		{
			// Stretch along delta direction.
			bb.Add(m_vPos + m_vVel * (params.fStretch.GetMaxValue() + abs(params.fStretch.fOffsetRatio)), fRadius);
		}
		if (m_aPosHistory && m_aPosHistory[0].IsUsed())
		{
			// Add oldest element.
			bb.Add(m_aPosHistory[0].vPos, fRadius);
		}
	}
}

void CParticle::ReallocHistory( int nPrevHistorySteps )
{
	SParticleHistory* aPrevHist = m_aPosHistory;
	if (aPrevHist)
	{
		m_aPosHistory = GetContainer().AllocPosHistory();
		memcpy(m_aPosHistory, aPrevHist, min(nPrevHistorySteps, GetContainer().GetHistorySteps()) * sizeof(*aPrevHist));
		for (int n = nPrevHistorySteps; n < GetContainer().GetHistorySteps(); n++)
			m_aPosHistory[n].SetUnused();
	}
}

SPU_NO_INLINE void CParticle::UpdateChildEmitter()
{
	CParticleLocEmitter* pChildEmitter = m_pChildEmitter;

	if (pChildEmitter)
	{
		// Transfer particle's location & velocity.
		QuatTS loc;
		GetRenderLocation(loc);
		pChildEmitter->SetLoc(loc);
		pChildEmitter->SetVel(m_vVel);
		pChildEmitter->SetEmitGeom(m_pStatObj);
		pChildEmitter->SetStopAge(m_fLifeTime);
	}
}

SPU_NO_INLINE void CParticle::AddPosHistory( float fCurrentAge, float fRelativeAge, SParticleHistory const& histPrev )
{
	if (!m_aPosHistory)
		return;

	// Store previous position in history.
	float fTailLength = GetParams().fTailLength.GetValueFromMod(m_BaseMods.TailLength, fRelativeAge);

	int nCount = GetContainer().GetHistorySteps();
	while (nCount > 0 && !m_aPosHistory[nCount-1].IsUsed())
		nCount--;

	// To do: subdivide for curvature: vel <> prevvel
	// Clear out old entries.
	float fMinAge = fCurrentAge - fTailLength;
	int nStart = 0;
	while (nStart+1 < nCount && m_aPosHistory[nStart+1].fAge < fMinAge)
		nStart++;
	
	if (nStart > 0)
	{
		for (int n = nStart; n < nCount; n++)
			m_aPosHistory[n-nStart] = m_aPosHistory[n];
		for (int n = nCount-nStart; n < nCount; n++)
			m_aPosHistory[n].SetUnused();
		nCount -= nStart;
	}

	// Interp oldest entry.
	if (nCount > 1)
	{
		float fT = div_min(fMinAge - m_aPosHistory[0].fAge, m_aPosHistory[1].fAge - m_aPosHistory[0].fAge, 1.f);
		if (fT > 0.f)
		{
			m_aPosHistory[0].fAge = fMinAge;
			m_aPosHistory[0].vPos += (m_aPosHistory[1].vPos - m_aPosHistory[0].vPos) * fT;
		}
	}

	if (nCount == GetContainer().GetHistorySteps())
	{
		// Remove least significant entry.
		int nLeast = nCount;
		float fLeastSignif = InterpVariance(m_vPos, histPrev.vPos, m_aPosHistory[nCount-1].vPos);
		for (int n = 1; n < nCount; n++)
		{
			float fSignif = InterpVariance( 
				n+1 == nCount ? histPrev.vPos : SpuStackValue<Vec3, false>(m_aPosHistory[n+1].vPos),
				m_aPosHistory[n].vPos, 
				m_aPosHistory[n-1].vPos );
			if (fSignif < fLeastSignif)
			{
				fLeastSignif = fSignif;
				nLeast = n;
			}
		}
		if (nLeast != nCount)
		{
			// Delete entry.
			for (int n = nLeast; n < nCount-1; n++)
				m_aPosHistory[n] = m_aPosHistory[n+1];
			nCount--;
		}
	}
	if (nCount < GetContainer().GetHistorySteps())
		m_aPosHistory[nCount] = histPrev;
}

void CParticle::Update( SParticleUpdateContext const& context, float fFrameTime )
{
	const float fTIME_EPSILON = 1e-5f;

	// Detect deactivated emitters.
	if (m_pEmitter && !GetLoc().IsActive())
		m_pEmitter = 0;
	CParticleContainer& rContainer = GetContainer();
	CParticleSubEmitter* pEmitter = GetEmitter();
	CParticleLocEmitter* pChildEmitter = m_pChildEmitter;
	ResourceParticleParams const& params = rContainer.GetParams();

	float fNewAge		= (float)__fsel(m_fAge, m_fAge, 0.0f);
	float fNewRelativeAge  = (float)__fsel(m_fAge, m_fRelativeAge, 0.0f);
	fFrameTime			= (float)__fsel(m_fAge, fFrameTime, min(fFrameTime, -m_fAge));	

	// Process only up to lifetime of particle.
	if (!rContainer.HasExtendedLifetime())
	{
		assert(fNewAge <= m_fLifeTime);
		fFrameTime = min(fFrameTime, m_fLifeTime - fNewAge);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Move
	////////////////////////////////////////////////////////////////////////////////////////////////

	m_fClipDistance = fHUGE;

  IF (m_pPhysEnt, false)
  {
#if !defined(__SPU__) // phys entities of particles are not supported on SPU, so no need to scan/simulate this code
		// Use physics engine to move particles.
		GetPhysicsState();

		// Get collision status.
		pe_status_collisions status;
		coll_history_item item;
		status.pHistory = &item;
		status.age = 1.f;
		status.bClearHistory = 1;
		if (m_pPhysEnt->GetStatus(&status) > 0)
		{
			if (pChildEmitter)
				pChildEmitter->Collide();
			if (params.fBounciness < 0)
			{
				// Kill particle on collision.
				EndParticle();
				return;
			}
		}


#endif
  }
	else if (fFrameTime > fTIME_EPSILON && !m_bRest)
	{
		uint32 nFlags = context.nEnvFlags & (ENV_GRAVITY | ENV_WIND);
		uint32 nCollideFlags = context.nEnvFlags & (ENV_TERRAIN | ENV_STATIC_ENT | ENV_DYNAMIC_ENT);
		float fMaxStepTime = fFrameTime;
		float fMaxDev = 0.f;
		bool bDeviationSet = false;

		// Test for excessive tail curvature (undesirable for purebreds).
		if (fFrameTime * rContainer.GetHistorySteps() > params.fTailLength.GetMaxValue())
		{
			fMaxDev = fMAX_TAIL_DEVIATION;
			bDeviationSet = true;
		}

		if (nCollideFlags)
		{
			fMaxDev = fMAX_COLLIDE_DEVIATION;
			bDeviationSet = true;
		}

		int nIterations = 0;
		float fIterations = 0;
		int nCollisions = 0;

		bool bSliding = false;
		Vec3 vSlidingPlane(ZERO);
		float fSlidingDrag = 0.f;

		// Transform emitter-relative particles by emitter movement.
		if (params.bMoveRelEmitter && pEmitter)
		{
			pEmitter->MoveRelative(m_vPos, m_qRot, m_vVel);
		}

		while (fFrameTime > fTIME_EPSILON && nIterations < nMAX_ITERATIONS)
		{
			float fMinStepTime = fFrameTime / (fMAX_ITERATIONS-fIterations);

			nIterations++;
			fIterations += 1.0f;
			Vec3 vPrevPos = m_vPos;
			Vec3 vPrevVel = m_vVel;

			SParticleHistory histPrev;
			histPrev.fAge = fNewAge;
			histPrev.vPos = m_vPos;

#ifdef _DEBUG
			// Backup point.
			m_vPos = vPrevPos;
			m_vVel = vPrevVel;
#endif

			// Apply previously computed MaxStepTime.
			float fStepTime = min(fFrameTime, fMaxStepTime);
			fMaxStepTime = fFrameTime;

			// Legacy 2D spiral turbulence.
			if (params.fTurbulenceSize && params.fTurbulenceSpeed)
			{
				// Apply exact rotation differences between this and next step.
				float fFutureAge = fNewAge + fStepTime;
				float fFutureRelativeAge = ComputeRelativeAge(fFutureAge);
				
				Vec2 vMove2 = VortexRotation(fFutureAge, fFutureRelativeAge);

				vMove2 -= VortexRotation(fNewAge, fNewRelativeAge);

				Vec3 vMove = vMove2;
				float fLenSq = m_vVel.GetLengthSquaredFloat();
				if (fLenSq > FLT_MIN)
				{
					Matrix33 m33;
					m33.SetRotationV0V1(Vec3(0,0,1), m_vVel * isqrt_tpl(fLenSq));
					m_vPos += m33 * vMove;
				}
				else
				{
					m_vPos += vMove;
				}

				// Do not adjust velocity, this is an artificial spiral motion.
			}

			// Evaluate params at halfway point thru frame.
			float fMidWayRelativeAge = ComputeRelativeAge(fNewAge, fStepTime * 0.5f);

			SExternForces forces;
			forces.fDrag = params.fAirResistance.GetValueFromMod(m_BaseMods.AirResistance, fMidWayRelativeAge);
			forces.vAccel = params.vAcceleration;
			forces.vWind.zero();

			Vec3 vGrav;
			if (nFlags)
			{
				context.PhysEnv.CheckPhysAreas( m_vPos, vGrav, forces.vWind, nFlags );
				float fGrav = params.fGravityScale.GetValueFromMod(m_BaseMods.GravityScale, fMidWayRelativeAge);
				forces.vAccel += vGrav * fGrav;
			}

			if (params.fTurbulence3DSpeed)
			{
				/*
					Random impulse dvm applied every time quantum tm.
					From random walk formula, the average velocity magnitude after time t
					(without other forces such as air resistance):
							dv(t) = dvm sqrt(t/tm)
										= vs sqrt(t),			vs := dvm/sqrt(tm)
					The average distance traveled (again with no forces) is then
							d(t) = Int[s=0..t] dv(s)
									 = vs/1.5 t^1.5
					Converting this to an acceleration applied over time dt so it can integrate with other forces:
							a(dt) = dv(dt) / dt = vs / sqrt(dt)
										= d(1) / sqrt(dt)
				*/
				float fMag = params.fTurbulence3DSpeed.GetValueFromMod(m_BaseMods.Turbulence3DSpeed, fMidWayRelativeAge) * isqrt_tpl(fStepTime);
				forces.vAccel += SphereRandom(fMag);
			}

			if (bSliding)
			{
				// Remove sliding component of acceleration.
				float fPen = forces.vAccel * vSlidingPlane;
				
				if (fPen < 0.f)
					forces.vAccel -= vSlidingPlane * fPen;

				assert(m_vVel * vSlidingPlane >= -1e-3f);

				if (fSlidingDrag > 0.f)
					forces.vWind *= forces.fDrag / (forces.fDrag + fSlidingDrag);

				forces.fDrag = forces.fDrag + fSlidingDrag;
			}

			if (bDeviationSet)
				TravelLinear( m_vPos, m_vVel, fStepTime, forces, fMaxDev, fMinStepTime);
			else
				Travel( m_vPos, m_vVel, fStepTime, forces );

			// Compute targeting force.
			ParticleTarget target;
			if (GetTarget(target) && !TargetMovement( target, fNewAge, forces, fStepTime, fNewRelativeAge ))
			{
				EndParticle();
				return;
			}

			Vec3 vMove = m_vPos - vPrevPos;

			if (bSliding && nIterations < nMAX_ITERATIONS && fStepTime > fMinStepTime)
			{
				// Limit travel when sliding, so we can re-query the geometry.
				float fMoveSqr = vMove.GetLengthSquared();
				if (fMoveSqr > sqr(fMAX_COLLIDE_DEVIATION))
				{
					fMaxStepTime = fStepTime * 0.5f*fMAX_COLLIDE_DEVIATION * isqrt_tpl(fMoveSqr);
					m_vPos = vPrevPos;
					m_vVel = vPrevVel;
					continue;
				}
			}
			bSliding = false;

			if (nFlags & context.PhysEnv.m_nNonUniformFlags && nIterations < nMAX_ITERATIONS)
			{
				// Subdivide travel in non-uniform areas.
				float fMoveSqr = vMove.GetLengthSquared();
				if (fMoveSqr > sqr(fMAX_NONUNIFORM_TRAVEL))
				{
					Vec3 vGrav2, vWind2;
					context.PhysEnv.CheckPhysAreas( m_vPos, vGrav2, vWind2, nFlags );
					Vec3 vErrorEst = (vGrav2-vGrav)*(0.5f*sqr(fStepTime));

					if (forces.fDrag > 0.f)
					{
						// Estimate wind difference.
						float fTEst = sqr(fStepTime) * forces.fDrag * 0.5f;
						vErrorEst += (vWind2-forces.vWind)*fTEst;
					}

					fMaxStepTime = ComputeMaxStep(vErrorEst, fStepTime, 0.75f*fMAX_NONUNIFORM_TRAVEL);
					fMaxStepTime = max(fMaxStepTime, fMinStepTime);
					if (fMaxStepTime < fStepTime)
					{
						m_vPos = vPrevPos;
						m_vVel = vPrevVel;
						continue;
					}
				}
			}

			if (params.bSpaceLoop)
			{
				// Wrap the particle into camera-aligned BB.
				Vec3 vPosLocal = context.matSpaceLoopInv * m_vPos;

				Vec3 vCurPos = m_vPos;

				int nCorrect;//lots of LHS due to repetetive float conversion
				if ((nCorrect = int_round(vPosLocal.x)) != 0)
					m_vPos -= context.matSpaceLoop.GetColumn(0) * (float)nCorrect;
				if ((nCorrect = int_round(vPosLocal.y)) != 0)
					m_vPos -= context.matSpaceLoop.GetColumn(1) * (float)nCorrect;
				if ((nCorrect = int_round(vPosLocal.z)) != 0)
					m_vPos -= context.matSpaceLoop.GetColumn(2) * (float)nCorrect;

				if (context.nSpaceLoopFlags && (fNewAge == 0.f || vCurPos != m_vPos))
				{
					// On init or wrap, set particle lifetime based on object collision.
					float fTBefore = 10.f, fTAfter = m_fLifeTime - fNewAge - fStepTime;
					Vec3 vEndPos = m_vPos, vEndVel = m_vVel;
					if (fTAfter > 0.f)
					{
						Travel( vEndPos, vEndVel, fTAfter, forces );
					}
					ray_hit hit;
					if (context.PhysEnv.PhysicsCollision( hit, m_vPos - vEndVel * fTBefore, vEndPos, GetVisibleRadius(), context.nSpaceLoopFlags ))
					{
						float fHitTime = hit.dist * (fTBefore + fTAfter) - fTBefore;
						if (fHitTime <= 0.f)
						{
							EndParticle();
							return;
						}
						else
							m_fLifeTime = min(m_fLifeTime, fNewAge+fStepTime+fHitTime);
					}
				}
				if (vCurPos != m_vPos)
				{
					// Move prev pos, and expand to space border.
					Vec3 vPrevDelta = vPrevPos-vCurPos;
					if (nCollideFlags)
						vPrevDelta = vPrevDelta.GetNormalized() * params.fCameraMaxDistance;
					vPrevPos = m_vPos + vPrevDelta;
				}
			}

			////////////////////////////////////////////////////////////////////////////////////////////////
			// Simple collision with physics entities
			// Collision strategy: no persistent sliding vars needed!
			//	* Move w/o sliding or collision
			//	* Test collision, determine bounce
			//	* If sliding, do it for remainder of step
			////////////////////////////////////////////////////////////////////////////////////////////////

			if (nCollideFlags && 
				(params.nMaxCollisionEvents == 0 || m_nCollisionCount < params.nMaxCollisionEvents) && 
				!vMove.IsZero())
			{
				ray_hit hit;
				if (context.PhysEnv.PhysicsCollision( hit, vPrevPos, m_vPos, fCOLLIDE_BUFFER_DIST, nCollideFlags ))
				{
					if (hit.bTerrain)
						rContainer.GetCounts().ParticlesCollideTerrain += 1.f;
					else
						rContainer.GetCounts().ParticlesCollideObjects += 1.f;

					// Set particle to collision point.
					// Linearly interpolate velocity based on distance.
					m_vVel = vPrevVel*(1.f-hit.dist) + m_vVel*hit.dist;
					m_vPos = hit.pt;

					fStepTime *= hit.dist;

					if (pChildEmitter)
					{
						// Rotate child emitter to surface normal.
						QuatTS loc;
						GetRenderLocation(loc);
						loc.q = Quat::CreateRotationV0V1( Vec3(0,1,0), hit.n);
						pChildEmitter->SetVel(m_vVel);
						pChildEmitter->SetLoc(loc);
						pChildEmitter->SetEmitGeom(hit.pCollider);
						pChildEmitter->Collide(fFrameTime - fStepTime);
					}

					if (params.fBounciness < 0.f)
					{
						// Particle dies on collision.
						fNewAge += fStepTime;
						UpdateRelativeAge(fNewAge);
						m_fAge = fNewAge;
						EndParticle();
						return;
					}
					else
					{
						// Remove perpendicular velocity component.
						float fVelPerp = min(m_vVel * hit.n, 0.f);
						m_vVel -= hit.n * fVelPerp;

						// Get phys params from material, or particle params.
						float fBounce;
						GetCollisionParams(hit.surface_idx, fBounce, fSlidingDrag);

						float fVelBounce = fVelPerp * -fBounce;
						float fAccelPerp = forces.vAccel * hit.n;

						// Sliding occurs when the bounce distance would be less than the collide buffer.
						// d = - v^2 / 2a <= dmax
						// v^2 <= -dmax*2a
						if (sqr(fVelBounce) <= fCOLLIDE_BUFFER_DIST * fAccelPerp * -2.f)
						{
							// Slide during next iteration.
							if (m_vVel.GetLengthSquared() < sqr(fREST_VEL))
							{
								// Slide has come to rest.
								m_vVel.zero();
								m_vRotVel.zero();
								if (!(nCollideFlags & ENV_DYNAMIC_ENT))
									m_bRest = true;
								break;
							}
							bSliding = true;
							vSlidingPlane = hit.n;
						}
						else
						{
							// Bouncing.
							if (m_nCollisionCount < 255)
								m_nCollisionCount++;

							nCollisions++;
							if (fSlidingDrag > 0.f)
							{
								// Apply sliding drag to parallel velocity component,
								// for period that particle is within collision buffer.
								float fBounceTime = div_min( fCOLLIDE_BUFFER_DIST, fVelBounce, fStepTime+fFrameTime );
								float fDecay = expf(-fSlidingDrag * fBounceTime);
								m_vVel *= fDecay;
								m_vRotVel *= fDecay;
							}

							// Bounce the particle, continue for remainder of frame.
							m_vVel += hit.n * fVelBounce;
						}
					}
				}
			}

			if (forces.fDrag * params.fRotationalDragScale > 0.f)
			{
				Vec3 newRotVel = m_vRotVel * expf(-forces.fDrag * params.fRotationalDragScale * fStepTime);
				
				if (newRotVel.GetLengthSquaredFloat() == 0.f)
					newRotVel.zero();

				m_vRotVel = newRotVel;
			}

			// Apply rotation.
			if (params.eFacing != ParticleFacing_Free)
				m_fAngle += m_vRotVel.y * fStepTime;
			else
				m_qRot = exp(m_vRotVel*(fStepTime*0.5f)) * m_qRot;

			UpdateRotation( context, nCollisions > 0 );

			fFrameTime -= fStepTime;
			fNewAge += fStepTime;
			
			fNewRelativeAge = ComputeRelativeAge(fNewAge);

			AddPosHistory(fNewAge, fNewRelativeAge, histPrev);
		}

		rContainer.GetCounts().ParticlesReiterate += (fIterations-1.0f);
  }

	fNewAge = (float)__fsel(fFrameTime, fNewAge + fFrameTime, fNewAge);
	
	UpdateRelativeAge(fNewAge);

	// Shrink particle when approaching visible limits.
	if (!(GetCVars()->e_ParticlesDebug & AlphaBit('c')))
	{
		if (params.tVisibleUnderwater != Trinary_Both)
		{
			// Particles not allowed to cross water boundary, shrink them as they get close.
			if (context.PhysEnv.m_tUnderWater == Trinary_Both)
			{
				Plane pl;
				m_fClipDistance = min(m_fClipDistance, context.PhysEnv.DistanceAboveWater(m_vPos, GetVisibleRadius(), pl));
				if (params.tVisibleUnderwater == Trinary_If_True)
					m_fClipDistance = -m_fClipDistance;
				if (m_fClipDistance <= 0.f && !params.bSpaceLoop)
					// Kill particle when it crosses water, unless recycled in space loop.
					EndParticle();
			}
			else if (context.PhysEnv.m_tUnderWater != params.tVisibleUnderwater)
				EndParticle();
		}
	}

	// Update emitter location for indirect particles.
	if (pChildEmitter)
		UpdateChildEmitter();

	m_fAge = fNewAge;
}

SPU_NO_INLINE void CParticle::UpdateRotation( SParticleUpdateContext const& context, bool bCollided )
{
	ResourceParticleParams const& params = GetParams();

	// Orient in world space as required.
	if (params.bOrientToVelocity)
	{
		float fLenSq = m_vVel.GetLengthSquaredFloat();
		if (fLenSq > FLT_MIN)
		{
			m_qRot = Quat::CreateRotationV0V1( m_qRot.GetColumn2(), m_vVel * isqrt_tpl(fLenSq)) * m_qRot;
			m_qRot.Normalize();
		}
	}

	switch (params.eFacing)
	{
		case ParticleFacing_Water:
		{
			// Project point and velocity onto plane.
			Plane plWater;
			float fDist = context.PhysEnv.DistanceAboveWater(m_vPos, -WATER_LEVEL_UNKNOWN, plWater);
			if (plWater.d < -WATER_LEVEL_UNKNOWN)
			{
				m_vPos -= plWater.n * (fDist - params.vPositionOffset.z - ATTACH_BUFFER_DIST);
				m_vVel -= plWater.n * (m_vVel * plWater.n);
				RotateTo(plWater.n);
			}
			else
			{
				// No water, kill it.
				m_fLifeTime = 0.f;
			}
			break;
		}
		case ParticleFacing_Terrain:
		{
			if (GetTerrain())
			{
				// Project center and velocity onto plane.
				float fSize = GetSize();
				if (fSize > 0.f)
				{
					// Sample terrain around point to get heights and average normal.
					Vec3 avCorner[5];
					for (int c = 0; c < 5; c++)
					{
						avCorner[c] = m_vPos;
						if (c < 4)
						{
							avCorner[c].x += ((c&1)*2-1) * fSize;
							avCorner[c].y += ((c&2)-1) * fSize;
						}
						// explicit pass false to the GetZApr function, which means to not include voxels
						// if you change this, please also look at the SPU code, since the voxel interaction part is
						// currently disabled for SPUs
						avCorner[c].z = GetTerrain()->GetZApr(avCorner[c].x, avCorner[c].y, false, GetDefSID());
					}

					// Rotate sprite to average normal of quad.
					Vec3 vTNorm = (avCorner[3] - avCorner[0]) ^ (avCorner[2] - avCorner[1]);
					if (vTNorm.z != 0.f)
					{
						if (vTNorm.z < 0.f)
							vTNorm = -vTNorm;
						vTNorm.Normalize();
						RotateTo(vTNorm);

						// Adjust z to match plane.	
						float fZAdd = -fHUGE;
						for (int c = 0; c < 5; c++)
							fZAdd = max(fZAdd, avCorner[c] * vTNorm);
						m_vPos.z += (fZAdd - m_vPos * vTNorm) / vTNorm.z + ATTACH_BUFFER_DIST; 
					}
				}
			}
			break;
		}
		case ParticleFacing_Horizontal:
		{
			RotateTo(GetMain().m_qpLoc.q.GetColumn1());
			break;
		}
		case ParticleFacing_Velocity:
		{
			// Rotate to movement dir.
			float fLenSq = m_vVel.GetLengthSquaredFloat();
			if (fLenSq > FLT_MIN)
				RotateTo(m_vVel * isqrt_tpl(fLenSq));
			break;
		}
	}
}

SPU_NO_INLINE Vec2 CParticle::VortexRotation(float fCurrentAge, float fRelativeAge)
{
	// Compute vortex rotational offset at current age.
	// p(t) = TSize e^(i TSpeed t) (t max 1)
	const ParticleParams& params = GetParams();
	float fVortexSpeed = DEG2RAD( params.fTurbulenceSpeed.GetValueFromMod(m_BaseMods.TurbulenceSpeed, fRelativeAge ));
	float fVortexSize = params.fTurbulenceSize.GetValueFromMod( m_BaseMods.TurbulenceSize, fRelativeAge ) * GetScale();
	Vec2 vRot;
	cry_sincosf( fVortexSpeed * fCurrentAge, &vRot.y, &vRot.x );
	vRot *= fVortexSize;
	return vRot;
}

SPU_NO_INLINE bool CParticle::TargetMovement( ParticleTarget const& target, float fCurrentAge, SExternForces& forces, float fStepTime, float fRelativeAge )
{
	const ParticleParams& params = GetParams();

	float fTargetRadius = params.TargetAttraction.fOrbitDistance.GetValueFromMod(m_BaseMods.OrbitDistance, fRelativeAge)
		+ target.fRadius;
	bool bOrbiting = params.TargetAttraction.bOrbit && fTargetRadius > 0.f;

	// Decompose current velocity into radial+angular components.
	Vec3 vPos = m_vPos - target.vTarget;
	Vec3 vVel = m_vVel - target.vVelocity;

	float fDist = vPos.GetLength();
	if (params.TargetAttraction.bShrink)
		m_fClipDistance = fDist;
	if (!bOrbiting)
	{
		if (fDist <= fTargetRadius)
			return false;
	}
	else
	{
		if (fDist < 1e-6f)
			return false;
	}

	float fVel = vVel.GetLength();
	Vec3 vRadialDir = vPos.GetNormalized();
	float fRadialVel = vVel | vRadialDir;
	Vec3 vOrbitalVel = vVel - vRadialDir * fRadialVel;
	Vec3 vOrbitalDir = vOrbitalVel.GetNormalized();
	float fOrbitalVel = vOrbitalVel.GetLength();
	float fAngularVel = fOrbitalVel / fDist;

	// Determine arrival time.
	float fArrivalTime = div_min(fTargetRadius - fDist, fRadialVel, fHUGE);
	if (fArrivalTime < 0.f)
		fArrivalTime = fHUGE;

	// Goal is to reach target radius in a quarter revolution.
	fArrivalTime = div_min(gf_PI*0.5f, fAngularVel, fArrivalTime);

	float fLife = m_fLifeTime - fCurrentAge;
	if (fArrivalTime > fLife)
	{
		if (params.TargetAttraction.bExtendSpeed || fDist - fTargetRadius < fVel * fLife)
			fArrivalTime = fLife;
		else
//Diesel cut
			//fArrivalTime = (fDist - fTargetRadius) / fVel;
//Diesel new 332	
			fArrivalTime = div_min(fDist - fTargetRadius, fVel, fHUGE);
	}
	else if (!bOrbiting)
	{
		if (fArrivalTime <= fStepTime)
		{
			// Allow 1-frame tolerance for reaching goal.
			m_vPos = target.vTarget;
			return false;
		}

		// Age particle prematurely based on target time.
		m_fLifeTime = fCurrentAge + fArrivalTime;
		UpdateRelativeAge(fCurrentAge, fStepTime*0.5f);
	}

	// Change particle direction, maintaining speed.
	fArrivalTime = max(fArrivalTime, fStepTime);
	fRadialVel = (fTargetRadius - fDist) / fArrivalTime;
	fOrbitalVel = sqrt_tpl( max(0.f, sqr(fVel) - sqr(fRadialVel)) );

	// Execute the orbit.
	float fNewDist = fDist + fRadialVel * fStepTime;
	if (fNewDist < 1e-6f)
	{
		m_vPos = target.vTarget;
		return false;
	}
	fAngularVel = fOrbitalVel * div_min(logf(fNewDist) - logf(fDist), fNewDist - fDist, 1.f/fNewDist);
	float fCos, fSin;
	sincos_tpl(fAngularVel * fStepTime, &fSin, &fCos);

	float fOrbitalDelta = fDist * fSin;
	float fRadialDelta = fDist * (fCos - 1.f);
	fRadialDelta += fRadialVel * fStepTime;

	m_vPos += vRadialDir * fRadialDelta + vOrbitalDir * fOrbitalDelta;

	vRadialDir = (m_vPos - target.vTarget).GetNormalized();
	vOrbitalDir -= vRadialDir * (vRadialDir | vOrbitalDir);
	vOrbitalDir.Normalize();

	m_vVel = vRadialDir * fRadialVel + vOrbitalDir * fOrbitalVel + target.vVelocity;

	if (params.TargetAttraction.bShrink)
		m_fClipDistance = fNewDist;
	return true;
}

SPU_NO_INLINE void CParticle::GetCollisionParams(int nCollSurfaceIdx, float& fBounce, float& fDrag)
{
	// Get phys params from material, or particle params.
	fBounce = GetParams().fBounciness;
	fDrag = GetParams().fDynamicFriction;

	int iSurfaceIndex = GetSurfaceIndex();
	if (iSurfaceIndex  > 0)
	{
		IMaterialManager* pMatMan = SPU_MAIN_PTR(GetMatMan());
		ISurfaceType* pSurfPart = pMatMan->GetSurfaceType(iSurfaceIndex);
		if (pSurfPart)
		{
			ISurfaceType::SPhysicalParams const& physPart = SPU_MAIN_PTR(pSurfPart)->GetPhyscalParams();

			// Combine with hit surface params.
			ISurfaceType *pSurfHit = pMatMan->GetSurfaceType(nCollSurfaceIdx);
			if (!pSurfHit)
				pSurfHit = SPU_MAIN_PTR( pMatMan->GetDefaultTerrainLayerMaterial() )->GetSurfaceType();
			ISurfaceType::SPhysicalParams const& physHit = SPU_MAIN_PTR(pSurfHit)->GetPhyscalParams();

			fBounce = clamp_tpl( (physPart.bouncyness+physHit.bouncyness)*0.5f, 0.f, 1.f );
			fDrag = max( (physPart.friction+physHit.friction)*0.5f, 0.f );
		}
	}
}

SPU_NO_INLINE void CParticle::GetRenderLocation( QuatTS& loc ) const
{
	ResourceParticleParams const& params = GetParams();

	loc.s = GetSize();
	loc.t = m_vPos;
	loc.q = m_qRot;
	if (m_pStatObj && !params.bNoOffset && params.ePhysicsType != ParticlePhysics_RigidBody)
	{
		// Recenter object pre-rotation.
		Vec3 vCenter = m_pStatObj->GetAABB().GetCenter();
		loc.t = loc * -vCenter;
	}
}

SPU_NO_INLINE void CParticle::Destroy()
{
	// Turn off child emitter
	if (m_pChildEmitter)
		m_pChildEmitter->Deactivate();

  if (m_pPhysEnt)
	{
		m_pPhysEnt->Release();
		GetPhysicalWorld()->DestroyPhysicalEntity(m_pPhysEnt);
	}

	if (m_pStatObj)
	{



		m_pStatObj->Release();

	}
	if (m_aPosHistory)
		GetContainer().FreePosHistory(m_aPosHistory);
}

char GetMinAxis(Vec3 const& vVec)
{
	float x = fabs(vVec.x);
	float y = fabs(vVec.y);
	float z = fabs(vVec.z);

	if (x<y && x<z)
		return 'x';
	
	if (y<x && y<z)
		return 'y';
	
	return 'z';
}

#define fSPHERE_VOLUME float(4.f / 3.f * gf_PI)

SPU_NO_INLINE int CParticle::GetSurfaceIndex() const
{
	ResourceParticleParams const& params = GetParams();
	if (params.iPhysMat == 0 && m_pStatObj)
	{
		phys_geometry* pGeom = m_pStatObj->GetPhysGeom();
		if (pGeom && pGeom->surface_idx < pGeom->nMats)
			return pGeom->pMatMapping[pGeom->surface_idx];
	}
	return params.iPhysMat;
}

void CParticle::Physicalize( SParticleUpdateContext const& context )
{
#if !defined(__SPU__) // never called on spu, so exclude to spare code size 
	ResourceParticleParams const& params = GetParams();

	Vec3 vGravity = context.PhysEnv.m_vUniformGravity * params.fGravityScale.GetValueFromMod(m_BaseMods.GravityScale, m_fRelativeAge) + params.vAcceleration;

	pe_params_pos par_pos;
	par_pos.pos = m_vPos;
	par_pos.q = m_qRot;

	if (params.ePhysicsType == ParticlePhysics_RigidBody)
	{
		if (!m_pStatObj)
			return;

		// Make Physical Rigid Body.
		m_pPhysEnt = GetPhysicalWorld()->CreatePhysicalEntity(PE_RIGID,&par_pos,m_pStatObj,OT_RIGID_PARTICLE );
		 
		pe_geomparams partpos;
		partpos.density = params.fDensity;
		partpos.scale = GetSize();
		partpos.flagsCollider = geom_colltype_debris;
		partpos.flags &= ~geom_colltype_debris; // don't collide with other particles.
		partpos.surface_idx = params.iPhysMat;
		m_pPhysEnt->AddGeometry( m_pStatObj->GetPhysGeom(),&partpos,0 );
		
		pe_simulation_params symparams;
		symparams.minEnergy = (0.2f)*(0.2f);
		symparams.damping = symparams.dampingFreefall = params.fAirResistance.GetValueFromMod(m_BaseMods.AirResistance, m_fRelativeAge);

		// Note: Customized gravity currently doesn't work for rigid body.
		symparams.gravity = symparams.gravityFreefall = vGravity;
		//symparams.softness = symparams.softnessGroup = 0.003f;
		//symparams.softnessAngular = symparams.softnessAngularGroup = 0.01f;
		symparams.maxLoggedCollisions = params.nMaxCollisionEvents;
		m_pPhysEnt->SetParams(&symparams);

		pe_action_set_velocity velparam;
		velparam.v = m_vVel;
		velparam.w = m_vRotVel;
		m_pPhysEnt->Action(&velparam);
	}
	else
	{
		// Make Physical Particle.
		m_pPhysEnt = GetPhysicalWorld()->CreatePhysicalEntity(PE_PARTICLE,&par_pos);
		pe_params_particle part;

		// Compute particle mass from volume of object.
		part.size = params.fSize.GetMaxValue() * GetScale();

		part.mass = params.fDensity * part.size * part.size * part.size;
		if (m_pStatObj)
		{
			part.size *= m_pStatObj->GetRadius() + 0.05f;
			phys_geometry* pgeom = m_pStatObj->GetPhysGeom();
			if (pgeom)
				part.mass *= pgeom->V;
			else
				part.mass *= m_pStatObj->GetAABB().GetVolume() * fSPHERE_VOLUME / 8.f;
		}
		else
		{
			// Assume spherical volume.
			part.mass *= fSPHERE_VOLUME;
		}

		part.thickness = params.fThickness * part.size;
		part.velocity = m_vVel.GetLength();
		if (part.velocity > 0.f)
			part.heading = m_vVel / part.velocity;
		part.q0 = m_qRot;
		part.wspin = m_vRotVel;
		part.q0 = m_qRot;
		
		if (m_pStatObj)
		{
			Vec3 vSize = m_pStatObj->GetBoxMax() - m_pStatObj->GetBoxMin();
			char cMinAxis = GetMinAxis(vSize);
			part.normal = Vec3((cMinAxis=='x')?1.f:0,(cMinAxis=='y')?1.f:0,(cMinAxis=='z')?1.f:0);
		}

		part.surface_idx = GetSurfaceIndex();
		part.flags = /*particle_no_roll|*/particle_no_path_alignment;
		part.kAirResistance = params.fAirResistance.GetValueFromMod(m_BaseMods.AirResistance, m_fRelativeAge);
		part.gravity = vGravity;

		m_pPhysEnt->SetParams(&part);
	}

	// Common settings.
	if (m_pPhysEnt)
	{
		pe_params_flags pf;
		pf.flagsOR = pef_never_affect_triggers;
		if (params.nMaxCollisionEvents > 0)
			pf.flagsOR |= pef_log_collisions;
		m_pPhysEnt->SetParams(&pf);
		m_pPhysEnt->AddRef();
	}


#endif
}

void CParticle::GetPhysicsState()
{
	IF (m_pPhysEnt, false)
	{
		pe_status_pos status_pos;
		if (m_pPhysEnt->GetStatus(&status_pos))
		{
			m_vPos = status_pos.pos;
			m_qRot = status_pos.q;
		}
		pe_status_dynamics status_dyn;
		if (m_pPhysEnt->GetStatus(&status_dyn))
		{
			m_vVel = status_dyn.v;
		}
	}
}

#ifdef _DEBUG

// Test for distribution evenness.
struct CChaosTest
{
	CChaosTest()
	{
		CChaosKey keyBase(0U);
		float f[100];
		for (uint32 i = 0; i < 100; i++)
		{
			f[i] = keyBase.Jumble(CChaosKey(i)) * 1.f;
		}
	}
};

static CChaosTest ChaosTest;

#endif
