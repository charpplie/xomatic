////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ParticleEmitter.cpp
//  Created:     18/7/2003 by Timur.
//  Modified:    17/3/2005 by Scott Peter
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleEmitter.h"
#include "ParticleContainer.h"
#include "ICryAnimation.h"
#include "Particle.h"
#include "partman.h"
#include "FogVolumeRenderNode.h"
#include "ISound.h"
#include "IThreadTask.h"

#include <SpuUtils.h>

#ifdef WIN64
	#pragma warning(disable: 4244)
#endif // WIN64





#define fSOUND_RANGE_START_BUFFER								float(2.f)		// Distance to max sound range in order to start sound
#define fVELOCITY_SMOOTHING_TIME								float(0.125f)	// Averaging interval to smooth computed entity velocity

// global holder class for SPU deferred Release Objects
SpuDeferredReleaseObjects gSPUDeferredReleaseObjects;

/*
	Scheme for Emitter updating & bounding volume computation.

	Whenever possible, we update emitter particles only on viewing.
	However, the bounding volume for all particles must be precomputed, 
	and passed to the renderer,	for visibility. Thus, whenever possible, 
	we precompute a Static Bounding Box, loosely estimating the possible
	travel of all particles, without actually updating them.

	Currently, Dynamic BBs are computed for emitters that perform physics,
	or are affected by non-uniform physical forces.
*/

//////////////////////////////////////////////////////////////////////////
// Misc functions.

inline float round_up(float f, float r)
{
	float m = fmod(f, r);
	if (m > 0.f)
		f += r-m;
	return f;
}

SPU_NO_INLINE void Interp( QuatTS& out, QuatTS const& a, QuatTS const& b, float t )
{
	out.q.SetNlerp( a.q, b.q, t );
	out.s = a.s * (1.f-t) + b.s * t;
	out.t.SetLerp( a.t, b.t, t );
}

// Compute estimated lifetime based on current motion and target.
// Currently ignore gravity and wind.
float GetTravelTime(float fDist, float fVel, float fDrag)
{
	if (fDist*fVel <= 0.f)
		return 0.f;
	if (fDrag > 0.f)
	{
		//	X = V0 (1 - e^(-d t)) / d
		//	1 - X d / V0 = e^(-d t)
		//	t = - log(1 - X d / V0) / d
		return -logf( 1.f - fDist * fDrag / fVel ) / fDrag;
	}
	else
	{
		// No drag.
		return fDist / fVel;
	}
}



























































//////////////////////////////////////////////////////////////////////////
// CParticleSubEmitter implementation.
//////////////////////////////////////////////////////////////////////////

SPU_NO_INLINE CParticleSubEmitter::CParticleSubEmitter( CParticleSubEmitter* pParent, CParticleLocEmitter* pLoc, CParticleContainer* pCont )
	: m_ChaosKey(0U), m_fSoundRange(0.f),
		m_pParams(pCont->GetParamsAddr()), m_pContainer(pCont), m_pLocEmitter(pLoc), m_pParentEmitter(pParent)
{
	m_qpLastLoc = pLoc->GetLocation();
	m_fStartAge = m_fEndAge = m_fRepeatAge = fHUGE;
	m_fStrength = 0.f;
	m_fToEmit = 0.f;
	m_pForce = NULL;
	m_bSoundPlayed = false;

#ifdef _DEBUG
	//if (!gEnv->pSystem->IsEditorMode())
	//{
	//	IParticleEffect* pParentEffect = GetContainer().GetEffect() ? GetContainer().GetEffect()->GetParent() : 0;
	//	while (pParentEffect && !CPartManager::GetManager()->IsActive(pParentEffect->GetParticleParams()))
	//		pParentEffect = pParentEffect->GetParent();
	//	assert((m_pParentEmitter ? m_pParentEmitter->GetContainer().GetEffect() : 0) == pParentEffect);
	//}
#endif
}

SPU_NO_INLINE void CParticleSubEmitter::Destroy()
{
	assert(GetRefCount() == 0);
	Deactivate();
}

//////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE void CParticleSubEmitter::Initialize( float fPast )
{
	m_fToEmit = 0.f;

	// Reseed randomness.
	m_ChaosKey = CChaosKey(cry_rand32());

	// Compute lifetime params.
	m_fStartAge = m_fEndAge = GetAge() - fPast + GetCurValue(m_pParams->fSpawnDelay);
	if (m_pParams->bContinuous)
	{
		if (m_pParams->fEmitterLifeTime)
			m_fEndAge += GetCurValue(m_pParams->fEmitterLifeTime);
		else
			m_fEndAge = fHUGE;
	}

	if (m_pParams->eSoundControlTime != SoundControlTime_EmitterPulsePeriod)
		m_bSoundPlayed = false;

	// Compute next repeat age.
	if (m_pParams->fPulsePeriod.GetMaxValue() > 0.f)
	{
		float fRepeat = GetCurValue(m_pParams->fPulsePeriod);
		m_fRepeatAge = GetAge() - fPast + fRepeat;
		if (m_fRepeatAge < GetAge())
			m_fRepeatAge += round_up( GetAge()-m_fRepeatAge, fRepeat );
	}
	else if (m_pParams->fPulsePeriod.GetMaxValue() < 0.f && m_pParentEmitter)
		// Inherit from parent.
		m_fRepeatAge = m_pParentEmitter->m_fRepeatAge;
	else
		m_fRepeatAge = fHUGE;
}

//////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE void CParticleSubEmitter::Deactivate()
{
	if (m_pSound != 0)
	{



		m_pSound->Stop();
		m_pSound = 0;

	}
	IF (m_pForce, false)
	{
		GetPhysicalWorld()->DestroyPhysicalEntity(m_pForce);
		m_pForce = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE void CParticleSubEmitter::Activate( EActivateMode mode, float fPast )
{
	float fAge = GetAge() - fPast;
	switch (mode)
	{
		case eAct_Pause:
		case eAct_Deactivate:
			Deactivate();
			break;
		case eAct_Restart:
			Deactivate();
			// continue;
		case eAct_Activate:
			if (!GetParams().bSpawnOnParentCollision && fAge < m_fStartAge)
				Initialize(fPast);
			break;
		case eAct_Resume:
			m_fToEmit = 0.f;
			break;
		case eAct_Reactivate:
			m_fToEmit = 0.f;
			if (fAge < m_fStartAge)
			{
				Deactivate();
				Initialize();
			}
			break;
	}
}

float CParticleSubEmitter::GetStrength( float fAgeAdjust /* = 0.f */, ESoundControlTime const eControl /* = SoundControlTime_EmitterLifeTime */ ) const
{
	float fStrength = GetMain().GetSpawnParams().fStrength;
	if (fStrength < 0.f)
		return GetRelativeAge(fAgeAdjust, eControl);
	else
		return min(fStrength, 1.f);
}

EEmitterState CParticleSubEmitter::GetState() const
{
	CParticleContainer&		rContainer = GetContainer();
	CParticleLocEmitter&	rLocEmitter = GetLoc();
	const ResourceParticleParams& params = GetParams();

	float fAge = GetAge();
	if (fAge >= m_fStartAge)
	{
		float fEndAge = GetEndAge();
		if (fAge <= fEndAge)
			// Emitter still active.
			return eEmitter_Active;

		if (params.fCount || rContainer.HasParticles())
		{
			// Has particles.
			if (rContainer.HasExtendedLifetime())
				return eEmitter_Particles;

			float fParticleLife = rContainer.GetMaxParticleLife();
			if (fParticleLife == 0.f
		  || fAge <= fEndAge + fParticleLife + GetTimer()->GetFrameTime()*2)
				return eEmitter_Particles;
		}
	}

	// No particles, see if emitter is dead.
	if (fAge > rLocEmitter.GetStopAge())
	{
		if (rLocEmitter.IsActive())
			// Paused but still alive.
			return eEmitter_Dormant;
	}
	else
	{
		if (fAge < m_fStartAge)
			return eEmitter_Dormant;
		if (GetMain().GetSpawnParams().fPulsePeriod || params.fPulsePeriod)
			return eEmitter_Dormant;
	}

	return eEmitter_Dead;
}

SPU_NO_INLINE void CParticleSubEmitter::EmitParticles( SParticleUpdateContext const& context )
{
	// Emit new particles only for enabled effects.
	float fAge = GetAge();
	if (fAge >= m_fStartAge)
	{
		UpdateStrength();

		// Target total particles in system.
		float fEmitRate = GetEmitRate();
		if (fEmitRate > 0.f)
		{
			float fUpdateTime = GetTimeToUpdate();
			float fAge0 = fAge - fUpdateTime;
			fAge0 = max(fAge0, m_fStartAge);
			float fLife = GetContainer().GetMaxParticleLife();

			if (m_pParams->bContinuous)
			{
				// Determine time window to update.
				float fAge1 = min(fAge, GetEndAge());

				// Adjust emit rate downward for current framerate to avoid most particle rejection.
				// Necessary because particles stay alive 1 extra frame after lifetime.
				fEmitRate *= fLife / (fLife + GetTimer()->GetFrameTime());
				float fEmitCount = m_fToEmit + (fAge1-fAge0) * fEmitRate;

				// Compute time of next emission.
				if (fEmitCount > 0.f)
				{
					// Skip time before emitted particles would still be alive.
					// To to opt: avoid even this much updating for steady-state emitters (freeze).
					float fAgeIncrement = 1.f / fEmitRate;
					fAge0 -= m_fToEmit * fAgeIncrement;
					float fSkip = round_up(fAge-fLife-fAge0, fAgeIncrement);
					if (fSkip > 0)
					{
						fAge0 += fSkip;
					}
					float fPartAge = fAge-fAge0;
					for (; fPartAge > fAge-fAge1; fPartAge -= fAgeIncrement)
					{
						if (!EmitParticle(context, fPartAge))
						{
							GetContainer().GetCounts().ParticlesReject += fPartAge * fEmitRate;
							break;
						}
					}
					fAge0 = fAge-fPartAge;
					m_fToEmit = (fAge1-fAge0) * fEmitRate;
				}
				else
				{
					m_fToEmit = fEmitCount;
				}
			}
			else if (m_fToEmit == 0.f)
			{
				// Emit only once, if still valid.
				// Always emit first frame, even if lifetime < frame time.				
				if (fLife == 0.f || fAge <= m_fStartAge + fLife + GetTimer()->GetFrameTime()*2)
				{
					for (int nEmit = int_round(fEmitRate); nEmit > 0; nEmit--)
						if (!EmitParticle(context, fAge - fAge0))
						{
							GetContainer().GetCounts().ParticlesReject += nEmit;



							break;
						}
						else 
						{
							// Emit only once, if still valid. 
							// was moved here because of spu allocation which can cause the memory for the needed particles only be avaible in the next frame
							m_fToEmit = -1.f;
						}
				}
			}
		}
	}
}

void CParticleSubEmitter::MoveRelative( Vec3& vPos, Quat& qRot, Vec3& vVel )
{
	const QuatTS& qpCur = GetLoc().GetLocation();
	if (m_qpLastLoc.s > 0.f || qpCur.s == m_qpLastLoc.s)
	{
		Quat qMove = qpCur.q * !m_qpLastLoc.q;
		float fMoveScale = qpCur.s == m_qpLastLoc.s ? 1.f : qpCur.s / m_qpLastLoc.s;

		qRot = qMove * qRot;
		vVel = fMoveScale * (qMove * vVel);
		vPos = qpCur.t + fMoveScale * (qMove * (vPos - m_qpLastLoc.t));
	}
}

bool CParticleSubEmitter::GetLocalTarget( ParticleTarget& target ) const
{
	if (GetContainer().HasLocalTarget())
	{
		// Local target from parent emitter.
		for (CParticleSubEmitter* pParent = m_pParentEmitter; pParent; pParent = pParent->m_pParentEmitter)
		{
			if (pParent->GetParams().eForceGeneration == ParticleForce_Target)
			{
				if (pParent->GetState() >= eEmitter_Active)
				{
					target.vTarget = pParent->GetEmitPos();
					target.fRadius = pParent->GetParams().fPosRandomOffset;
					target.vVelocity = pParent->GetLoc().GetVel();
					return true;
				}
			}
		}
	}
	return false;
}

void CParticleSubEmitter::UpdateForce()
{
	if (m_pParams->eForceGeneration == ParticleForce_None
	|| m_pParams->eForceGeneration == ParticleForce_Target)
		return;

  FUNCTION_PROFILER_SYS(PARTICLE);

	UpdateStrength();

	// Set or clear physical force.
	if (GetState() >= eEmitter_Particles)
	{
		struct SForceGeom
		{
			QuatTS	qpLoc;							// world placement of force
			AABB		bbOuter, bbInner;		// local boundaries of force.
			Vec3		vForce3;
			float		fForceW;
		} force;

		//
		// Compute force geom.
		//

		SPhysEnviron const& PhysEnv = GetMain().GetUniformPhysEnv();
		float fAge = GetAge();

		// Location.
		force.qpLoc = GetLoc().m_qpLoc;
		force.qpLoc.s *= GetMain().GetParticleScale();
		force.qpLoc.t = force.qpLoc * m_pParams->vPositionOffset;

		// Direction.
		Vec3 vFocus = force.qpLoc.q.GetColumn1();
		if (m_pParams->bFocusGravityDir)
			vFocus = (-PhysEnv.m_vUniformGravity).GetNormalizedSafe(vFocus);

		float fFocusAngle = GetCurValue(m_pParams->fFocusAngle);
		if (fFocusAngle != 0.f)
		{
			float fAzimuth = GetCurValue(m_pParams->fFocusAzimuth, 360.f);

			// Rotate focus about X.
			Vec3 vRot(1,0,0);
			vFocus = Quat::CreateRotationAA(DEG2RAD(fAzimuth), Vec3(0,1,0)) 
						 * Quat::CreateRotationAA(DEG2RAD(fFocusAngle), vRot) * vFocus;
		}
		force.qpLoc.q = Quat::CreateRotationV0V1(Vec3(0,1,0),vFocus);

		// Set inner box from spawn geometry.
		Quat qToLocal = force.qpLoc.q.GetInverted() * m_pLocEmitter->m_qpLoc.q;
		Vec3 vOffset = qToLocal * Vec3(m_pParams->vRandomOffset);
		force.bbInner.Reset();
		force.bbInner.Add(vOffset, m_pParams->fPosRandomOffset);
		force.bbInner.Add(-vOffset, m_pParams->fPosRandomOffset);

		// Emission directions.
		float fPhiMax = DEG2RAD(GetCurValue(1.f, m_pParams->fEmitAngle)), 
					fPhiMin = DEG2RAD(GetCurValue(0.f, m_pParams->fEmitAngle));

		AABB bbTrav;
		bbTrav.max.y = cosf(fPhiMin);
		bbTrav.min.y = cosf(fPhiMax);
		float fCosAvg = (bbTrav.max.y + bbTrav.min.y) * 0.5f;
		bbTrav.max.x = bbTrav.max.z = (bbTrav.min.y * bbTrav.max.y < 0.f ? 1.f : sin(fPhiMax));
		bbTrav.min.x = bbTrav.min.z = -bbTrav.max.x;
		bbTrav.Add(Vec3(ZERO));

		// Force magnitude: speed times relative particle density.
		float fSpeed = GetCurValue(1.f, m_pParams->fSpeed) * GetMain().GetSpawnParams().fSpeedScale;
		float fForce = fSpeed * GetCurValue(1.f, m_pParams->fAlpha) * force.qpLoc.s;

		float fPLife = GetCurValue(1.f, m_pParams->fParticleLifeTime);
		float fTime = fAge-m_fStartAge;
		if (m_pParams->bContinuous && fPLife > 0.f)
		{
			// Ramp up/down over particle life.
			float fEndAge = GetEndAge();
			if (fTime < fPLife)
				fForce *= fTime/fPLife;
			else if (fTime > fEndAge)
				fForce *= 1.f - (fTime-fEndAge) / fPLife;
		}

		// Force direction.
		force.vForce3.zero();
		force.vForce3.y = fCosAvg * fForce;
		force.fForceW = sqrtf(1.f - square(fCosAvg)) * fForce;

		// Travel distance.
		float fDist = TravelDistance( abs(fSpeed), GetCurValue(1.f, m_pParams->fAirResistance), min(fTime,fPLife) );
		bbTrav.min *= fDist;
		bbTrav.max *= fDist;

		// Set outer box.
		force.bbOuter = force.bbInner;
		force.bbOuter.Augment(bbTrav);

		// Expand by size.
		float fSize = GetCurValue(1.f, m_pParams->fSize);
		force.bbOuter.Expand( Vec3(fSize) );

		// Scale: Normalise box size, so we can handle some geom changes through scaling.
		Vec3 vSize = force.bbOuter.GetSize()*0.5f;
		float fRadius = max(max(vSize.x, vSize.y), vSize.z);

		if (fForce * fRadius == 0.f)
		{
			// No force.
			if (m_pForce)
			{
				GetPhysicalWorld()->DestroyPhysicalEntity(m_pForce);
				m_pForce = NULL;
			}
			return;
		}

		force.qpLoc.s *= fRadius;
		float fIRadius = 1.f / fRadius;
		force.bbOuter.min *= fIRadius;
		force.bbOuter.max *= fIRadius;
		force.bbInner.min *= fIRadius;
		force.bbInner.max *= fIRadius;

		//
		// Create physical area for force.
		//

		primitives::box geomBox;
		geomBox.Basis.SetIdentity();
		geomBox.bOriented = 0;
		geomBox.center = force.bbOuter.GetCenter();
		geomBox.size = force.bbOuter.GetSize() * 0.5f;

		pe_status_pos spos;
		if (m_pForce)
		{
			// Check whether shape changed.
			m_pForce->GetStatus(&spos);
			if (spos.pGeom)
			{
				primitives::box curBox;
				spos.pGeom->GetBBox(&curBox);
				if (!curBox.center.IsEquivalent(geomBox.center, 0.001f)
				 || !curBox.size.IsEquivalent(geomBox.size, 0.001f))
				 spos.pGeom = NULL;
			}
			if (!spos.pGeom)
			{
				GetPhysicalWorld()->DestroyPhysicalEntity(m_pForce);
				m_pForce = NULL;
			}
		}

		if (!m_pForce)
		{
			IGeometry *pGeom = m_pPhysicalWorld->GetGeomManager()->CreatePrimitive( primitives::box::type, &geomBox );
			m_pForce = m_pPhysicalWorld->AddArea( pGeom, force.qpLoc.t, force.qpLoc.q, force.qpLoc.s );
			if (!m_pForce)
				return;

			// Tag area with this emitter, so we can ignore it in the emitter family.
			pe_params_foreign_data fd;
			fd.pForeignData = (void*)&GetMain();
			fd.iForeignData = fd.iForeignFlags = 0;
			m_pForce->SetParams(&fd);
		}
		else
		{
			// Update position & box size as needed.
			if (!spos.pos.IsEquivalent(force.qpLoc.t, 0.01f)
				|| !spos.q.IsEquivalent(force.qpLoc.q)
				|| spos.scale != force.qpLoc.s)
			{
				pe_params_pos pos;
				pos.pos = force.qpLoc.t;
				pos.q = force.qpLoc.q;
				pos.scale = force.qpLoc.s;
				m_pForce->SetParams(&pos);
			}
		}

		// To do: 4D flow
		pe_params_area area;
		float fVMagSqr = force.vForce3.GetLengthSquared(),
					fWMagSqr = square(force.fForceW);
		float fMag = sqrtf(fVMagSqr + fWMagSqr);
		area.bUniform = (fVMagSqr > fWMagSqr) * 2;
		if (area.bUniform)
		{
			force.vForce3 *= fMag * isqrt_tpl(fVMagSqr);
		}
		else
		{
			force.vForce3.z = fMag * (force.fForceW < 0.f ? -1.f : 1.f);
			force.vForce3.x = force.vForce3.y = 0.f;
		}
		area.falloff0 = force.bbInner.GetRadius();
		area.size.x = max( abs(force.bbOuter.min.x), abs(force.bbOuter.max.x) );
		area.size.y = max( abs(force.bbOuter.min.y), abs(force.bbOuter.max.y) );
		area.size.z = max( abs(force.bbOuter.min.z), abs(force.bbOuter.max.z) );

		if (m_pParams->eForceGeneration == ParticleForce_Gravity)
			area.gravity = force.vForce3;
		m_pForce->SetParams(&area);

		if (m_pParams->eForceGeneration == ParticleForce_Wind)
		{
			pe_params_buoyancy buoy;
			buoy.iMedium = 1;
			buoy.waterDensity = buoy.waterResistance = 0;
			buoy.waterFlow = force.vForce3;
			buoy.waterPlane.n = PhysEnv.m_plUniformWater.n;
			buoy.waterPlane.origin = PhysEnv.m_plUniformWater.n * -PhysEnv.m_plUniformWater.d;
			m_pForce->SetParams(&buoy);
		}
	}
	else
	{
		if (m_pForce)
		{
			GetPhysicalWorld()->DestroyPhysicalEntity(m_pForce);
			m_pForce = NULL;
		}
	}
}

SPU_NO_INLINE void CParticleSubEmitter::Update()
{
	// Evolve emitter state.
	float fAge = GetAge();

	// Handle pulsing.
	// First of individual sub-effect repetition, or emitter entity repetition.
	if (GetMain().GetSpawnParams().fPulsePeriod > 0.f)
	{
		float fRepeat = round_up(fAge, GetMain().GetSpawnParams().fPulsePeriod);
		m_fRepeatAge = min(m_fRepeatAge, fRepeat);
	}

	if (fAge >= m_fRepeatAge)
		Initialize(fAge - m_fRepeatAge);

	if (GetParams().bSpawnOnParentCollision && fAge < m_fStartAge && fAge >= GetLoc().GetCollideAge())
		Initialize(fAge - GetLoc().GetCollideAge());
}

void CParticleSubEmitter::UpdateSound()
{
	if (!m_bSoundPlayed)
	{
		// Start sound if required.
		if (m_fSoundRange > 0.f)
		{
			if (!SoundInRange())
				return;
		}

		if (m_pParams->sSound.empty() || !GetCVars()->e_Particles)
			return;

		// Stop any existing looping sound. Do not stop existing one-shot sounds!!
		// If we hit a start delay fAge will be smaller than m_fStartAge
		// therefore make sure we always stop remaining sounds
		if (m_pSound != 0 && (m_pSound->GetFlags() & FLAG_SOUND_LOOP))
		{
			m_pSound->Stop();
			m_pSound = 0;
		}

		// Handle sound start/stop.
		float fAge = GetAge();
		if (fAge >= m_fStartAge)
		{
			float fEndAgeLoop = GetEndAge(),
						fEndAgeTransient = m_fStartAge + 0.25f;

			if (fAge < max(fEndAgeLoop, fEndAgeTransient))
			{
				int nSndFlags = FLAG_SOUND_DEFAULT_3D;
				if (m_pParams->bContinuous)
					// Will apply only to legacy .wav sounds, not events.
					nSndFlags |= FLAG_SOUND_LOOP;

				m_pSound = gEnv->pSoundSystem->CreateSound( m_pParams->sSound.c_str(), nSndFlags );
				if (m_pSound)
				{
					m_pSound->SetSemantic(eSoundSemantic_Particle);

					// Don't play sounds too late (this is only important if pulse time and emitter time are not set to 0 (infinite emitter))
					if (fAge >= fEndAgeTransient && GetCurValue(m_pParams->fPulsePeriod) != 0.0f && GetCurValue(m_pParams->fEmitterLifeTime) != 0.0f)
					{
						m_pSound->Stop();
						m_pSound = 0;
					}

					if (m_fSoundRange == 0.f && m_pSound && m_pSound->GetFlags() & FLAG_SOUND_RADIUS)
					{
						// Only start sounds within range.
						m_fSoundRange = m_pSound->GetMaxDistance() + fSOUND_RANGE_START_BUFFER;
						if (!SoundInRange())
						{
							int const bLooping = m_pSound->GetFlags() & FLAG_SOUND_LOOP;
							m_pSound->Stop();
							m_pSound = 0;
							if (bLooping)
								// Do not set bSoundPlayed, check range every frame.
								return;
						}
					}

					if (m_pSound)
					{
						// Start sound.
						m_pSound->SetPosition( GetEmitPos() );
						m_pSound->GetInterfaceExtended()->SetVelocity( GetLoc().GetVel() );
				
						if (m_pParams->fSoundFXParam.GetMinValue() < 1.f)
						{
								m_pSound->SetParam("particlefx", 1.f - m_pParams->fSoundFXParam.GetVarValue(m_ChaosKey, GetStrength(0.0f, m_pParams->eSoundControlTime), 0.0f), false);
						}

						m_pSound->Play();
						m_bSoundPlayed = true;
					}
				}
			}
		}
	}
	else if (m_pSound != 0)
	{
		// Stop a looping sound on its desired stop-time.
		bool bStopSound = false;
		if (m_pParams->eSoundControlTime != SoundControlTime_EmitterPulsePeriod && (m_pSound->GetFlags() & FLAG_SOUND_LOOP))
		{
			switch (GetState())
			{
			case eEmitter_Particles:
				{
					// If this state is set we just finished emitter life time,
					// therefore stop here if the control time is set to emitter life time.
					if (m_pParams->eSoundControlTime == SoundControlTime_EmitterLifeTime)
						bStopSound = true;
				}
				break;
			case eEmitter_Dormant:
				{
					// If this state is set we just finished emitter extended life time (last particle died),
					// therefore stop here if the control time is set to extended emitter life time.
					if (m_pParams->eSoundControlTime == SoundControlTime_EmitterExtendedLifeTime)
						bStopSound = true;
				}
				break;
			}
		}

		if (bStopSound)
		{
			m_pSound->Stop();
			m_pSound = 0;
		}
		else
		{
			// Update all sounds (looping + one-shot)
			m_pSound->SetPosition( GetEmitPos() );
			m_pSound->GetInterfaceExtended()->SetVelocity( GetLoc().GetVel() );

			if (!m_pParams->fSoundFXParam.IsConstant())
			{
				// Update SoundFX modifier
				m_pSound->SetParam("particlefx", 1.f - m_pParams->fSoundFXParam.GetVarValue(m_ChaosKey, GetStrength(0.0f, m_pParams->eSoundControlTime), 0.0f), false);
			}

		}
	}
}

SPU_NO_INLINE int CParticleSubEmitter::EmitParticle( SParticleUpdateContext const& context, float fAge, IStatObj* pStatObj, IPhysicalEntity* pPhysEnt, const QuatTS* pLocation, const Vec3* pVel )
{
	QuatTS qpLoc;
	if (pLocation)
		// Emit at specified location.
		qpLoc = *pLocation;
	else
	{
		// Use emitter location, interpolate if emitter moved.
		float fUpdateTime = GetTimeToUpdate();
		if (fAge*fUpdateTime > 0.f && m_qpLastLoc.s >= 0.f && !m_qpLastLoc.IsEquivalent(GetLoc().GetLocation()))
			// Interpolate emission position.
			Interp(qpLoc, GetLoc().GetLocation(), m_qpLastLoc, fAge / fUpdateTime);
		else
			qpLoc = GetLoc().GetLocation();
	}

	if (!pStatObj && !pPhysEnt && m_pParams->pStatObj != 0 && m_pParams->pStatObj->GetSubObjectCount())
	{
		// If bGeomInPieces, emit one particle per piece.
		// Else iterate in 2 passes to count pieces, and emit a random piece.
		int nPiece = -1;
		while (!pStatObj)
		{
			int nPieces = 0;
			for (int i = m_pParams->GetSubGeometryCount()-1; i >= 0; i--)
			{
				IStatObj::SSubObject* pSub = SPU_MAIN_PTR(m_pParams->GetSubGeometry(i));
				if (pSub)
				{
					if (m_pParams->bGeometryInPieces)
					{
						QuatTS qpSub = qpLoc * QuatTS(pSub->localTM);
						if (!EmitParticle(context, fAge, pSub->pStatObj, pPhysEnt, &qpSub, pVel))
							continue;
					}
					else if (nPieces == nPiece)
					{
						pStatObj = pSub->pStatObj;
						break;
					}
					nPieces++;
				}
			}
			if (m_pParams->bGeometryInPieces)
				return nPieces;
			if (nPieces <= 1)
				break;
			nPiece = Random(nPieces);
		}
	}

  // Allow particle growth if specified; otherwise, reject.
	CParticle* pPart = m_pContainer->AddParticle(this);
	if (pPart)
	{
		// use a local buffer on SPU and then transfer the result to main memory



		char buffer = NULL;


		CParticle* pParticle = SPU_PTR_SELECT( pPart, (CParticle*)buffer );
	
		pParticle->Init( context, fAge, this, qpLoc, pLocation != 0, pVel, pStatObj, pPhysEnt );







		return 1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CParticleSubEmitter::SoundInRange() const
{
	Vec3 vSoundPos = GetEmitPos();
	Vec3 vCamPos = gEnv->pRenderer->GetCamera().GetPosition();
	return (vCamPos - vSoundPos).GetLengthSquared() <= sqr(m_fSoundRange);
}

SPU_NO_INLINE float CParticleSubEmitter::GetEmitRate() const
{
	const ResourceParticleParams& params = GetParams();

	float fCount = GetCurValue(m_pParams->fCount) * GetContainer().GetEmitCountScale();
	float fLife = GetContainer().GetMaxParticleLife();
	float fPulse = m_fRepeatAge - m_fStartAge;

	if (params.bContinuous)
	{
		if (fLife <= 0.f)
			return 0.f;

		float fEmitterLife = GetCurValue(params.fEmitterLifeTime);
		if (fEmitterLife > 0.f && fEmitterLife < fPulse)
		{
			// Actually pulsing.
			if (fEmitterLife < fLife)
				// Ensure enough particles emitted.
				fCount *= fLife / fEmitterLife;

			// Reduce emit rate for overlapping pulsing.
			if (fLife >= fPulse)
			{
				float fLdP = floor(fLife/fPulse);
				float fLmP = fLife - fLdP*fPulse;
				float fOverlap = (fLdP * fEmitterLife + min(fLmP,fEmitterLife)) / min(fLife,fEmitterLife);
				fLife *= fOverlap;
			}
		}

		// Compute continual emission rate which maintains fCount particles.
		return fCount / fLife;
	}
	else
	{
		if (fPulse < fLife)
		{
			// Reduce emit count for overlapping pulsing.
			fCount *= fPulse / fLife;
		}
		return fCount;
	}
}

Vec3 CParticleSubEmitter::GetEmitPos() const
{
	return GetLoc().GetWorldPosition(GetParams().vPositionOffset);
}

//////////////////////////////////////////////////////////////////////////
// CParticleLocEmitter implementation.
//////////////////////////////////////////////////////////////////////////

SPU_NO_INLINE CParticleLocEmitter::CParticleLocEmitter( CParticleEmitter* pMainEmitter, float fAge )
	: m_qpLoc(IDENTITY)
	, m_pMainEmitter(pMainEmitter)
	, m_vVel(ZERO)
	, m_fStopAge(fHUGE)
	, m_fCollideAge(fHUGE)
	, m_eMaxState(eEmitter_Active)
{
	m_timeCreated = GetTimer()->GetFrameStartTime() - CTimeValue(fAge);
}

SPU_NO_INLINE void CParticleLocEmitter::Destroy()
{
	assert(GetRefCount() == 0);
	if (m_EmitGeom.m_pStatObj)
	{



		m_EmitGeom.m_pStatObj->Release();

	}
	if (m_EmitGeom.m_pChar)
		m_EmitGeom.m_pChar->Release();
	if (m_EmitGeom.m_pPhysEnt)
		m_EmitGeom.m_pPhysEnt->Release();
}

SPU_NO_INLINE CParticleSubEmitter* CParticleLocEmitter::AddEmitter( CParticleSubEmitter* pParentEmitter, CParticleContainer* pContainer )
{
	CParticleSubEmitter* pEmitter = pContainer->AddEmitter( this, pParentEmitter );
	pEmitter->Activate(eAct_Activate, GetAge());
	return pEmitter;
}

//////////////////////////////////////////////////////////////////////////
SPU_NO_INLINE bool CParticleLocEmitter::SetEmitGeom( GeomRef geom )
{
	if (geom.m_pStatObj == m_EmitGeom.m_pStatObj
	 && geom.m_pChar == m_EmitGeom.m_pChar
	 && geom.m_pPhysEnt == m_EmitGeom.m_pPhysEnt)
	 return false;

	if (m_EmitGeom.m_pStatObj)
	{



		m_EmitGeom.m_pStatObj->Release();

	}
	if (m_EmitGeom.m_pChar)
		m_EmitGeom.m_pChar->Release();
	if (m_EmitGeom.m_pPhysEnt)
		m_EmitGeom.m_pPhysEnt->Release();

	m_EmitGeom = geom;

	if (m_EmitGeom.m_pStatObj)
		m_EmitGeom.m_pStatObj->AddRef();
	if (m_EmitGeom.m_pChar)
		m_EmitGeom.m_pChar->AddRef();
	if (m_EmitGeom.m_pPhysEnt)
		m_EmitGeom.m_pPhysEnt->AddRef();

	return true;
}

EEmitterState CParticleEmitter::GetState() const
{
	if (m_eMaxState == eEmitter_Dead)
		return eEmitter_Dead;
	EEmitterState eState = eEmitter_Dead;
	for_all_ptrs (const CParticleContainer, c, m_Containers) 
		if (const CParticleSubEmitter* e = c->GetDirectEmitter())
			eState = (EEmitterState)max(eState, e->GetState());
	return (EEmitterState) min(eState, m_eMaxState);
}

SPU_NO_INLINE void CParticleEmitter::Activate( EActivateMode mode, float fPast )
{
	switch (mode)
	{
		case eAct_Activate:
			m_fStopAge = fHUGE;
			if (IsActive())
				return;
			m_eMaxState = eEmitter_Active;
			break;
		case eAct_Resume:
			m_fStopAge = fHUGE;
			m_eMaxState = eEmitter_Active;
			break;
		case eAct_Deactivate:
			m_eMaxState = eEmitter_Particles;
			// continue
		case eAct_Pause:
			SetStopAge(GetAge() - fPast);
			break;
		case eAct_Reactivate:
			Reset();
	}

	for_all_ptrs (CParticleContainer, c, m_Containers)
		if (CParticleSubEmitter* pE = c->GetDirectEmitter())
			pE->Activate(mode, fPast);
}

void CParticleEmitter::Prime()
{
	float fEqTime = 0.f;
	for_all_ptrs (CParticleContainer, c, m_Containers)
	{
		ResourceParticleParams const& params = c->GetParams();
		if (params.HasEquilibrium())
			// Continuous immortal effect.
			fEqTime = max(fEqTime, c->GetEquilibriumAge());
	}

	CTimeValue timePrime = GetTimer()->GetFrameStartTime() - CTimeValue(fEqTime);
	if (timePrime < m_timeCreated)
		m_timeCreated = timePrime;
}

float CParticleEmitter::GetEmitCountScale() const
{
	float fCount = m_SpawnParams.fCountScale;
	if (m_SpawnParams.bCountPerUnit)
	{
		// Count multiplied by geometry extent.
		fCount *= GetExtent(m_GeomQuery, m_EmitGeom, m_SpawnParams.eAttachForm);
		fCount *= ScaleExtent(m_SpawnParams.eAttachForm, m_qpLoc.s);
	}
	fCount *= max(0.75f, GetCVars()->e_ParticlesLod);		// clamped to low spec value to prevent cheating
	return fCount;
}

void CParticleEmitter::ClearUnused()
{
	// Clear in reverse order, children before parents.
	for_rev_all_ptrs (CParticleContainer, c, m_Containers)
	{
		if (!c->IsUsed())
			c = m_Containers.erase_rev(c);
	}
}

CParticleContainer* CParticleEmitter::AddContainer(CParticleContainer* pParentContainer, const CParticleEffect* pEffect, const ParticleParams* pParams)
{
	CParticleContainer* pContainer = new(m_Containers.push_back_new()) CParticleContainer(pParentContainer, this, pEffect, pParams);
	return pContainer;
}

//////////////////////////////////////////////////////////////////////////
// CParticleEmitter implementation.
////////////////////////////////////////////////////////////////////////

CParticleEmitter::CParticleEmitter( bool bIndependent )
	: CParticleLocEmitter(this)
	, m_bIndependent(bIndependent)
	, m_BindToCamera(false)
{
	m_nEntityId = 0;
	m_nEntitySlot = 0;
	m_bSelected = false;
	m_pMainEmitter = this;
	m_nEnvFlags = 0;
	m_WSBBox.Reset();
	m_bbWorld.Reset();
	m_bbWorldDyn.Reset();
	m_bbWorldEnv.Reset();
	m_vPrevPos.zero();




  GetInstCount(GetRenderNodeType())++;
}

//////////////////////////////////////////////////////////////////////////
CParticleEmitter::~CParticleEmitter() 
{
	// Clear all particles & indirect emitters in reverse order first, to eliminate pointer dependencies.
	Reset();
  Get3DEngine()->FreeRenderNodeState(this); // Also does unregister entity.
  GetInstCount(GetRenderNodeType())--;
}

//////////////////////////////////////////////////////////////////////////
void CParticleEmitter::Register(bool b)
{
	if (!b)
	{
		if (!m_WSBBox.IsReset())
		{
			Get3DEngine()->UnRegisterEntity(this);
			m_WSBBox.Reset();
		}
	}
	else
	{
		// Register top-level render node if applicable.
		if (m_SpawnParams.bIgnoreLocation)
			Get3DEngine()->UnRegisterEntity(this);
		else if (!IsEquivalent(m_WSBBox, m_bbWorld))
		{
			if (!m_WSBBox.IsReset())
				Get3DEngine()->UnRegisterEntity(this);
			m_WSBBox = m_bbWorld;
			if (!m_WSBBox.IsReset())
			{
				// Register node.
				// Normally, use emitter's designated position for visibility.
				// However, if all bounds are computed dynamically, they might not contain the origin, so use bounds for visibility.
				if (BoundsTypes() & AlphaBit('s'))
					SetRndFlags(ERF_REGISTER_BY_POSITION, true);
				else
					SetRndFlags(ERF_REGISTER_BY_POSITION, false);
				Get3DEngine()->RegisterEntity(this);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
string CParticleEmitter::GetDebugString(char type) const
{
	string s = GetName();
	if (type == 's')
	{
		// Serialization debugging.
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_nEntityId);
		if (pEntity)
			s += string().Format(" entity=%s slot=%d", pEntity->GetName(), m_nEntitySlot);
		if (m_bIndependent)
			s += " indep";
	}
	else
	{
		SParticleCounts counts;
		GetCounts(counts, false);
		s += string().Format(" E=%.0f P=%.0f R=%0.f", counts.EmittersActive, counts.ParticlesActive, counts.ParticlesRendered);
	}
	
	switch (GetState())
	{
		case eEmitter_Dead:
			s += " dead";
			break;
		case eEmitter_Dormant:
			s += " dormant";
			break;
		case eEmitter_Particles:
			s += " inactive";
			break;
	}
	s += string().Format(" age=%.3f", GetAge());
	return s;
}

//////////////////////////////////////////////////////////////////////////
void CParticleEmitter::AddEffect( CParticleContainer* pParentContainer, CParticleSubEmitter* pParentEmitter, CParticleEffect const* pEffect, ParticleParams const* pParams, bool bUpdate )
{
	if (pEffect)
		pParams = &pEffect->GetParams();
	else if (!pParams)
		return;

	CParticleContainer* pContainer = pParentContainer;

	// All descendents of an indirect effect are also indirect effects, with the same parent.
	if (!pParams->bSecondGeneration && pParentContainer && pParentContainer->GetParent())
	{
		pParentContainer = pParentContainer->GetParent();
	}

	// Add if playable in current config.
	if (CPartManager::GetManager()->IsActive(*pParams))
	{
		if (m_bIndependent && static_cast<const ResourceParticleParams*>(pParams)->IsImmortal())
		{
			// Do not allow immortal effects on independent emitters.
			static std::set<const ParticleParams*> s_setWarnings;
			if (s_setWarnings.insert(pParams).second)
				Warning("Ignoring spawning of immortal independent particle sub-effect %s",
					(pEffect ? pEffect->GetName() : "[programmatic]"));
		}
		else
		{
			if (bUpdate)
			{
				// Look for existing container. 
				// Note: Inactive/disabled effects will not be removed, so they can be disabled/re-enabled when editing.
				// They will be skipped on playback.
				for_all_ptrs (CParticleContainer, c, m_Containers)
				{
					if (!c->IsUsed() && c->Is( pEffect, pParentContainer ))
					{
						pContainer = c;
						pContainer->SetUsed(true);
						break;
					}
				}
			}
			if (pContainer == pParentContainer)
			{
				// Add new container
				pContainer = AddContainer(pParentContainer, pEffect, pParams);
				if (!pContainer->GetParent())
				{
					pParentEmitter = AddEmitter( pParentEmitter, pContainer );
				}
			}
		}
	}

	// Recurse effect tree.
	if (pEffect)
		for (int i = 0, n = pEffect->GetChildCount(); i < n; i++)
		{
			CParticleEffect const* pChild = static_cast<const CParticleEffect*>(pEffect->GetChild(i));
			if (!pChild->GetParams().bSecondGeneration)
				AddEffect( pParentContainer, pParentEmitter, pChild, 0, bUpdate );
			else if (pContainer)
				AddEffect( pContainer, 0, pChild, 0, bUpdate );
		}
}

void CParticleEmitter::SetEffect( CParticleEffect const* pEffect, ParticleParams const* pParams )
{
  FUNCTION_PROFILER_SYS(PARTICLE);

	if (m_pTopEffect.get() != pEffect)
	{
		// Clear any existing emitters only if changing effect.
		Reset();
		m_Containers.clear();
		m_pTopEffect = &non_const(*pEffect);
	}

	// Create or update emitters to reflect effect tree. Works correctly in game or from editor changes.
	bool bUpdate = !m_Containers.empty();
	for_all_ptrs (CParticleContainer, c, m_Containers)
		c->SetUsed(false);
	AddEffect(0, 0, pEffect, pParams, bUpdate);
	if(!m_Containers.empty() && m_Containers.front().GetParams().bBindEmitterToCamera)
	{
		// cache the first sub-emitter's "bind to camera" variable to speed up access in UpdateEmitter
		m_BindToCamera = m_Containers.front().GetParams().bBindEmitterToCamera;
	}

	if (bUpdate)
		ClearUnused();

	m_nEnvFlags = ENV_WATER;
	for_all_ptrs (const CParticleContainer, c, m_Containers)
		m_nEnvFlags |= c->GetEnvironmentFlags();
}

SPU_NO_INLINE CParticleLocEmitter* CParticleEmitter::CreateIndirectEmitter( CParticleSubEmitter* pParentEmitter, QuatTS const& loc, float fPast )
{
	CParticleLocEmitter* pLoc = 0;
	CParticleSubEmitter* pEmitter = 0;
	if (pParentEmitter->GetContainer().GetChildFlags() & REN_ANY)
	{
		for_all_ptrs (CParticleContainer, c, m_Containers)
		{
			if (c->GetParent() == &pParentEmitter->GetContainer())
			{
				// Found an indirect container for this emitter.
				if (!pLoc)
				{
					pLoc = pParentEmitter->GetContainer().AddLocEmitter(this, fPast);
					pLoc->SetLoc(loc);
				}
				
				if (c->GetEffect())
				{
					if (pEmitter && c->GetEffect()->GetParent() == pEmitter->GetContainer().GetEffect())
						pParentEmitter = pEmitter;
				}
				pEmitter = pLoc->AddEmitter(pParentEmitter, c);
			}
		}
	}
	return pLoc;
}

void CParticleEmitter::SetLoc( QuatTS const& qp )
{
	if (!m_qpLoc.IsEquivalent(qp, 1e-5f))
	{
		InvalidateStaticBounds();
		m_VisEnviron.Invalidate();
		if (GetAge() == 0.f)
			m_vPrevPos = qp.t;
	}

	CParticleLocEmitter::SetLoc(qp);
}

void CParticleEmitter::SetSpawnParams( SpawnParams const& spawnParams, GeomRef geom )
{
	m_SpawnParams = spawnParams;
	if (m_SpawnParams.fPulsePeriod > 0.f && m_SpawnParams.fPulsePeriod < 0.1f)
		Warning("Particle emitter (effect %s) PulsePeriod %g too low to be useful", 
			GetName(), m_SpawnParams.fPulsePeriod);
	NormalizeEmitGeom(geom, m_SpawnParams.eAttachType);
	if (SetEmitGeom(geom))
		InvalidateStaticBounds();
}

bool GetEntityVelocity( Vec3& vVel, IEntity* pEnt )
{
	IPhysicalEntity* pPhysEnt = pEnt->GetPhysics();
	if (pPhysEnt)
	{
		pe_status_dynamics dyn;
		if (pPhysEnt->GetStatus(&dyn))
		{
			if (vVel != dyn.v)
				vVel = dyn.v;
			return true;
		}
	}
	return false;
}

void CParticleEmitter::UpdateFromEntity() 
{
  FUNCTION_PROFILER_SYS(PARTICLE);

	m_bSelected = false;

	// Get emitter entity.
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_nEntityId);

	// Set external target.
	if (!m_Target.bPriority)
	{
		ParticleTarget target;
		if (pEntity)
		{
			for (IEntityLink* pLink = pEntity->GetEntityLinks(); pLink; pLink = pLink->next)
			{
				if (!stricmp(pLink->name, "Target") || !strnicmp(pLink->name, "Target-", 7))
				{
					IEntity* pTarget = gEnv->pEntitySystem->GetEntity(pLink->entityId);
					if (pTarget)
					{
						target.bTarget = true;
						target.vTarget = pTarget->GetPos();
						if (!GetEntityVelocity(target.vVelocity, pTarget))
							target.vVelocity.zero();
						AABB bb;
						pTarget->GetLocalBounds(bb);
						target.fRadius = max(bb.min.len(), bb.max.len());
						break;
					}
				}
			}
		}

		if (target.bTarget != m_Target.bTarget || target.vTarget != m_Target.vTarget)
			InvalidateStaticBounds();
		m_Target = target;
	}

	bool bShadows = (m_nEnvFlags & REN_CAST_SHADOWS) != 0;

	// Get entity of attached parent.
	if (pEntity)
	{
		if (!(pEntity->GetFlags() & ENTITY_FLAG_CASTSHADOW))
			bShadows = false;

		if (pEntity->GetParent())
			pEntity = pEntity->GetParent();

		const SpawnParams& spawn = GetSpawnParams();
		if (spawn.eAttachType != GeomType_None)
		{
			// If entity attached, find attached physics and geometry on parent.
			GeomRef geom;

			// Set physical entity as well.
			if (spawn.eAttachType == GeomType_Physics)
				geom.m_pPhysEnt = pEntity->GetPhysics();

			if (!geom.m_pPhysEnt)
			{
				int nStart = spawn.nAttachSlot < 0 ? 0 : spawn.nAttachSlot;
				int nEnd = spawn.nAttachSlot < 0 ? 	pEntity->GetSlotCount() : spawn.nAttachSlot+1;
				for (int nSlot = nStart; nSlot < nEnd; nSlot++)
				{
					SEntitySlotInfo slotInfo;
					if (pEntity->GetSlotInfo( nSlot, slotInfo ))
					{
						geom.m_pStatObj = slotInfo.pStatObj;
						geom.m_pChar = slotInfo.pCharacter;
						NormalizeEmitGeom(geom, spawn.eAttachType);

						if (geom)
						{
							if (slotInfo.pWorldTM)
								SetMatrix(*slotInfo.pWorldTM);
							break;
						}
					}
				}
			}

			SetSpawnParams(spawn, geom);
		}

		// Get velocity from entity.
		if (!GetEntityVelocity(m_vVel, pEntity))
		{
			// Compute from motion.
			float fStep = gEnv->pTimer->GetFrameTime();
			if (fStep > 0.f)
			{
				Vec3 vNewVel = (m_qpLoc.t - m_vPrevPos) / fStep;
				float fDecay = expf(- fStep / fVELOCITY_SMOOTHING_TIME);
				m_vVel = m_vVel * fDecay + vNewVel * (1.f - fDecay);
			}
		}
		m_vPrevPos = m_qpLoc.t;

		// Flag whether selected.
		if (GetSystem()->IsEditorMode())
		{
			IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER);
			if (pRenderProxy)
			{
				IRenderNode *pRenderNode = pRenderProxy->GetRenderNode();
				if (pRenderNode)
					m_bSelected = (pRenderNode->GetRndFlags() & ERF_SELECTED) != 0;
			}
		}
	}

	SetRndFlags(ERF_CASTSHADOWMAPS, bShadows);
}

void CParticleEmitter::GetLocalBounds( AABB& bbox )
{
	if (m_bbWorld.IsReset())
	{
		bbox.min = bbox.max = Vec3(0);
	}
	else
	{
		bbox.min = m_bbWorld.min - m_qpLoc.t;
		bbox.max = m_bbWorld.max - m_qpLoc.t;
	}
}

float CParticleEmitter::GetMinDrawPixels() const
{
	return max(GetCVars()->e_ParticlesMinDrawPixels, 0.125f) / max(GetViewDistRatioNormilized(), 0.01f);
}

float CParticleEmitter::GetMaxViewDist()
{
	return GetMaxParticleSize(true) * (GetRenderer()->GetHeight() / GetRenderer()->GetCamera().GetFov()) / GetMinDrawPixels();
}

EEmitterState CParticleEmitter::UpdateEmitter()
{
  FUNCTION_PROFILER_SYS(PARTICLE);

	EEmitterState returnEmitterState = eEmitter_Dead;

	const uint32 cvarsParticlesDebug = (GetCVars()->e_ParticlesDebug & AlphaBit('z'));
	const uint32 cvarsParticlesActive = (GetCVars()->e_Particles);	
	const bool maxEmitterStateDead = (m_eMaxState == eEmitter_Dead);
	const uint32 renderFlagsHidden = GetRndFlags() & ERF_HIDDEN;

	if(!maxEmitterStateDead)
	{
		if(!cvarsParticlesDebug)
		{
			UpdateFromEntity();

			// Check render flags for emitter
			if(!renderFlagsHidden)
			{
				if(m_BindToCamera)
				{
					SetMatrix(GetRenderer()->GetCamera().GetMatrix());
				}

				static const float fENV_BOX_INITIAL = 20.f, fENV_BOX_EXPAND = 10.f;
				if (!m_SpawnParams.bIgnoreLocation && m_bbWorldEnv.IsReset())
				{
					// For first-computed bounds, query environment in an area around origin.
					m_bbWorldEnv.Add(m_qpLoc.t, fENV_BOX_INITIAL);
					m_PhysEnviron.GetPhysAreas( this, m_bbWorldEnv, m_nEnvFlags );
				}

				m_bbWorld.Reset();
				m_bbWorldDyn.Reset();
				m_uBoundsTypes = 0;

				EEmitterState eSubState = eEmitter_Dead;

				// Update containers, and generate emitter bounding box(es).
				for_all_ptrs(CParticleContainer, c, m_Containers)
				{
					DEBUG_MODIFYLOCK(c->GetLock());

					// Update subemitters.
					CParticleSubEmitter* pEmitter = c->GetDirectEmitter();
					if (pEmitter)
					{
						pEmitter->Update();
						eSubState = pEmitter->GetState();
						eSubState = (EEmitterState) min(eSubState, m_eMaxState);
						returnEmitterState = (EEmitterState) max(returnEmitterState, eSubState);
					}

					c->Update();
					if(eSubState >= eEmitter_Particles)
					{
						m_uBoundsTypes |= c->UpdateBounds(m_bbWorld, m_bbWorldDyn);
					}

					c->UpdateSound();
				}

				if (!m_SpawnParams.bIgnoreLocation)
				{
					m_VisEnviron.Update(m_qpLoc.t, m_bbWorld);
					if (!m_bbWorldEnv.ContainsBox(m_bbWorld) || CPartManager::GetManager()->GetUpdatedAreaBB().IsIntersectBox(m_bbWorld))
					{
						// When emitter leaves valid range, or areas have been updated, requery environment.
						m_bbWorldEnv = m_bbWorld;
						m_bbWorldEnv.Expand( Vec3(fENV_BOX_EXPAND) );
						m_PhysEnviron.GetPhysAreas( this, m_bbWorldEnv, m_nEnvFlags );
					}
				}

				if(!cvarsParticlesActive)
				{
					Reset();
					returnEmitterState = (EEmitterState) min(returnEmitterState, eEmitter_Dormant);
				}
			}
			else
			{
				Reset();
				returnEmitterState = eEmitter_Dormant;
			}
		}
		else
		{
			returnEmitterState = eEmitter_Active;
		}
	}

	return returnEmitterState;
}

void CParticleEmitter::UpdateForce()
{
	if (m_nEnvFlags & ENV_FORCE)
	{
		for_all_ptrs (CParticleContainer, c, m_Containers)
			c->UpdateForce();
	}
}

void CParticleEmitter::EmitParticle( IStatObj* pStatObj, IPhysicalEntity* pPhysEnt, QuatTS* pLocation, Vec3* pVel )
{
	if (!m_Containers.empty())
	{
		if (CParticleSubEmitter* pEmitter = m_Containers.front().GetDirectEmitter())
		{
			// Reserve space for one more particle.
			m_Containers.front().AllocParticles(1);

			// Ensure any child containers have enough particle space.
			for_all_ptrs (CParticleContainer, c, m_Containers)
				c->AllocParticles();
			SParticleUpdateContext context;
			pEmitter->EmitParticle( context, 0.f, pStatObj, pPhysEnt, pLocation, pVel );
		}
	}
}

void CParticleEmitter::SetEntity( IEntity* pEntity, int nSlot )
{
	m_nEntityId = pEntity ? pEntity->GetId() : 0;
	m_nEntitySlot = nSlot;
	UpdateFromEntity();
	if (m_bIndependent)
		m_nEntityId = 0;

	for_all_ptrs (CParticleContainer, c, m_Containers) 
		if (CParticleSubEmitter* e = c->GetDirectEmitter())
			e->ResetLoc();
}

void CParticleEmitter::Render( SRendParams const& RenParams )
{
	if (!GetCVars()->e_Particles)
		return;

  FUNCTION_PROFILER_SYS(PARTICLE);

	if (m_nRenderStackLevel > 0 && !m_Containers.empty() && m_Containers.front().GetParams().bBindEmitterToCamera)
		return; 

	// Calculate contribution of fog volumes in scene
	m_timeLastRendered = GetTimer()->GetFrameStartTime();

	SPartRenderParams PRParams;
	PRParams.m_nEmitterOrder = 0;
	PRParams.m_vCamPos = GetRenderer()->GetCamera().GetPosition();
	PRParams.m_fCamDistance = m_bbWorld.GetDistance(PRParams.m_vCamPos);
	PRParams.m_fAngularRes = GetRenderer()->GetHeight() / GetRenderer()->GetCamera().GetFov();
	PRParams.m_fHDRDynamicMultiplier = GetRenderer()->EF_Query(EFQ_HDRModeEnabled) ? Get3DEngine()->GetHDRDynamicMultiplier() : 1.f;

	ColorF fogVolumeContrib;
	CFogVolumeRenderNode::TraceFogVolumes( m_qpLoc.t, fogVolumeContrib );
	PRParams.m_nFogVolumeContribIdx = GetRenderer()->PushFogVolumeContribution( fogVolumeContrib );

	uint32 nRenFlags = REN_SPRITE | REN_GEOMETRY | REN_DECAL | REN_TAKE_SHADOWS;
	if (!m_SpawnParams.bIgnoreLocation)
	{
		if (Get3DEngine()->_GetRenderIntoShadowmap())
		{
			// Only geometry in shadow maps.
			nRenFlags &= REN_GEOMETRY;
			nRenFlags |= REN_CAST_SHADOWS;
		}
		else if (GetCVars()->e_ParticlesLights && !m_nRenderStackLevel)
			nRenFlags |= REN_LIGHTS;
	}

	// Render all containers.
	nRenFlags &= m_nEnvFlags;
	PRParams.m_nRenFlags = nRenFlags;
	if (PRParams.m_nRenFlags)
		for_all_ptrs (CParticleContainer, c, m_Containers)
			c->Render( RenParams, PRParams );
}

void CParticleEmitter::RenderDebugInfo()
{
	if (TimeNotRendered() < 0.25f && (m_bSelected || (GetCVars()->e_ParticlesDebug & AlphaBit('b'))))
	{
		// Draw bounding box.
		CCamera const& cam = GetRenderer()->GetCamera();
		ColorF color(1,1,1,1);

		// Compute label position, in bb clipped to camera.
		AABB const& bb = GetBBox();
		Vec3 vLabel = GetLocation().t;

		// Clip to cam frustum.
		float fBorder = 1.f; 
		for (int i = 0; i < FRUSTUM_PLANES; i++)
		{
			Plane const& pl = *cam.GetFrustumPlane(i);
			float f = pl.DistFromPlane(vLabel) + fBorder;
			if (f > 0.f)
				vLabel -= pl.n * f;
		}

		Vec3 vDist = vLabel - cam.GetPosition();
		vLabel += (cam.GetViewdir() * (vDist * cam.GetViewdir()) - vDist) * 0.1f;
		vLabel.CheckMax(bb.min);
		vLabel.CheckMin(bb.max);

		SParticleCounts counts;
		GetCounts(counts, false);
		float fPixToScreen = 1.f / ((float)GetRenderer()->GetWidth() * (float)GetRenderer()->GetHeight());

		if (!m_bSelected)
		{
			// Randomize hue by entity.
			uint32 uRand = (uint32)(this);
			if (BoundsTypes() & AlphaBit('d'))
			{
				color.r = 1.f;
				color.g = ((uRand>>8)&0x3F) * (0.5f/63.f) + 0.25f;
				color.b = ((uRand>>14)&0x3F) * (0.5f/63.f) + 0.25f;
			}
			else
			{
				color.r = ((uRand>>2)&0x3F) * (0.5f/63.f) + 0.25f;
				color.g = ((uRand>>8)&0x3F) * (0.75f/63.f) + 0.25f;
				color.b = ((uRand>>14)&0x3F) * (0.75f/63.f) + 0.25f;
			}

			// Adjust by view angle, count, and fill.
			float fDist = vDist.NormalizeSafe();
			float fDistWt = div_min(20.f, fDist, 1.f);

			float fViewCos = cam.GetViewdir().GetNormalized() * vDist;
			float fAngleWt = sqr(sqr(sqr(fViewCos)));

			float fCountWt = clamp_tpl( sqrt_tpl(counts.ParticlesRendered * counts.PixelsRendered * fPixToScreen), 0.25f, 1.f );

			color.a = fDistWt * fAngleWt * fCountWt;
			if (color.a < 0.1f)
				return;
		}

		IRenderAuxGeom* pRenAux = GetRenderer()->GetIRenderAuxGeom();
		pRenAux->SetRenderFlags(SAuxGeomRenderFlags());

		char sLabel[256];
		sprintf(sLabel, "P=%.0f F=%.3f %s",
			counts.ParticlesRendered, // counts.ParticlesActive, 
			counts.PixelsRendered * fPixToScreen, // counts.PixelsProcessed * fPixToScreen,
			GetName());
		if (counts.ParticlesCollideTerrain)
			sprintf(sLabel+strlen(sLabel), " ColTr=%.0f", counts.ParticlesCollideTerrain);
		if (counts.ParticlesCollideObjects)
			sprintf(sLabel+strlen(sLabel), " ColOb=%.0f", counts.ParticlesCollideObjects);
		if (counts.ParticlesClip)
			sprintf(sLabel+strlen(sLabel), " Clip=%.0f", counts.ParticlesClip);
		if (BoundsTypes() & AlphaBit('d'))
		{
			strcat(sLabel, " Dyn");
			if (BoundsTypes() & AlphaBit('s'))
				strcat(sLabel, "+Stat");
		}
		GetRenderer()->DrawLabelEx( vLabel, 1.5f, (float*)&color, true, false, sLabel );

		color.a *= 0.4f;

		// Compare static and dynamic boxes.
		if (!m_bbWorldDyn.IsReset())
		{
			if (m_bbWorldDyn.min != bb.min || m_bbWorldDyn.max != bb.max)
			{
				// Separate dynamic BB.
				// Color outlying points bright red, draw connecting lines.
				ColorF colorGood = color * 0.6f;
				ColorF colorBad = Col_Red;
				Vec3 vStat[8], vDyn[8];
				ColorB clrDyn[8];
				for (int i = 0; i < 8; i++)
				{
					vStat[i] = Vec3( i&1 ? bb.max.x : bb.min.x, i&2 ? bb.max.y : bb.min.y, i&4 ? bb.max.z : bb.min.z );
					vDyn[i] = Vec3( i&1 ? m_bbWorldDyn.max.x : m_bbWorldDyn.min.x, i&2 ? m_bbWorldDyn.max.y : m_bbWorldDyn.min.y, i&4 ? m_bbWorldDyn.max.z : m_bbWorldDyn.min.z );
					clrDyn[i] = bb.IsContainPoint(vDyn[i]) ? colorGood : colorBad;
					pRenAux->DrawLine(vStat[i], color, vDyn[i], clrDyn[i]);
				}
				
				// Draw dyn bb.
				if (bb.ContainsBox(m_bbWorldDyn))
				{
					pRenAux->DrawAABB(m_bbWorldDyn, false, colorGood, eBBD_Faceted);
				}
				else
				{
					pRenAux->DrawLine(vDyn[0], clrDyn[0], vDyn[1], clrDyn[1]);
					pRenAux->DrawLine(vDyn[0], clrDyn[0], vDyn[2], clrDyn[2]);
					pRenAux->DrawLine(vDyn[0], clrDyn[0], vDyn[4], clrDyn[4]);

					pRenAux->DrawLine(vDyn[1], clrDyn[1], vDyn[3], clrDyn[3]);
					pRenAux->DrawLine(vDyn[1], clrDyn[1], vDyn[5], clrDyn[5]);

					pRenAux->DrawLine(vDyn[2], clrDyn[2], vDyn[3], clrDyn[3]);
					pRenAux->DrawLine(vDyn[2], clrDyn[2], vDyn[6], clrDyn[6]);

					pRenAux->DrawLine(vDyn[3], clrDyn[3], vDyn[7], clrDyn[7]);

					pRenAux->DrawLine(vDyn[4], clrDyn[4], vDyn[5], clrDyn[5]);
					pRenAux->DrawLine(vDyn[4], clrDyn[4], vDyn[6], clrDyn[6]);

					pRenAux->DrawLine(vDyn[5], clrDyn[5], vDyn[7], clrDyn[7]);

					pRenAux->DrawLine(vDyn[6], clrDyn[6], vDyn[7], clrDyn[7]);
				}
			}
			else
			{
				// Identical static & dynamic, presumably dynamic, render in brighter color.
				color = Col_Yellow;
			}
		}

		pRenAux->DrawAABB(bb, false, color, eBBD_Faceted);
	}
}

void CParticleEmitter::Serialize(TSerialize ser)
{
	ser.BeginGroup("Emitter");

	// Effect.
	string sEffect = GetEffect() ? GetEffect()->GetName() : "";
	ser.Value("Effect", sEffect);

	// Time value.
	CTimeValue timeCreated = m_timeCreated;
	bool bActive = IsActive();
	ser.Value("CreationTime", timeCreated);
	ser.Value("StopAge", m_fStopAge);
	ser.Value("Active", bActive);

	// Location.
	ser.Value("Pos", m_qpLoc.t);
	ser.Value("Rot", m_qpLoc.q);
	ser.Value("Scale", m_qpLoc.s);

	// Spawn params.
	ser.Value("SizeScale", m_SpawnParams.fSizeScale);
	ser.Value("SpeedScale", m_SpawnParams.fSpeedScale);
	ser.Value("CountScale", m_SpawnParams.fCountScale);
	ser.Value("CountPerUnit", m_SpawnParams.bCountPerUnit);
	ser.Value("PulsePeriod", m_SpawnParams.fPulsePeriod);

	if (ser.IsReading())
	{
		SetEffect(m_pPartManager->FindEffect(sEffect));
		m_timeCreated = timeCreated;
		m_eMaxState = bActive ? eEmitter_Active : eEmitter_Particles;
	}

	ser.EndGroup();
}

void CParticleEmitter::GetMemoryUsage( ICrySizer* pSizer ) const
{
	m_Containers.GetMemoryUsage(pSizer);
}

bool CParticleEmitter::UpdateStreamableComponents(float fImportance, Matrix34A & objMatrix, IRenderNode * pRenderNode, float fEntDistance)
{
  FUNCTION_PROFILER_3DENGINE;

	for_all_ptrs (const CParticleContainer, c, m_Containers)
  {
    ResourceParticleParams const& params = c->GetParams();
    if(params.pStatObj)
		{
      static_cast<CStatObj*>(params.pStatObj.get())->UpdateStreamableComponents(fImportance, objMatrix, pRenderNode, fEntDistance);
		}

		if(params.nTexId > 0)
		{
			ITexture* pTexture = GetRenderer()->EF_GetTextureByID((int)params.nTexId);
			if(pTexture)
			{
				const float minMipFactor = fEntDistance * fEntDistance * c->GetTexelAreaDensity();
				GetRenderer()->EF_PrecacheResource(pTexture, minMipFactor, 0.f, 0, GetObjManager()->m_nUpdateStreamingPrioriryRoundId);
			}
		}

		if(params.pMaterial)
		{
			Get3DEngine()->m_pObjManager->PrecacheMaterial(params.pMaterial, fEntDistance, NULL);
		}
  }

  return true;
}

void SpuDeferredReleaseObjects::ReleaseAll()
{
	for (int i = 0 ; i < NUM_SPUS ; ++i)
	{
		while (!m_deferredReleaseCalls[i].m_arrStatObj.empty())
		{
			IStatObj *pStatObj = m_deferredReleaseCalls[i].m_arrStatObj.pop();
			pStatObj->Release();
		}

		while (!m_deferredReleaseCalls[i].m_arrSound.empty())
		{
			ISound *pSound = m_deferredReleaseCalls[i].m_arrSound.pop();
			pSound->Stop();
			pSound->Release();
		}


	}
}
void CParticleEmitter::CollectSpuUsage( VecSpuUsageT &vecSpuUsage ) const
{
	for_all_ptrs (const CParticleContainer, c, m_Containers)
		c->CollectSpuUsage( vecSpuUsage );
}

#include UNIQUE_VIRTUAL_WRAPPER(IParticleEmitter)
#undef USE_SPU


