////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   particleeffect.cpp
//  Version:     v1.00
//  Created:     10/7/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleEffect.h"
#include "ParticleEmitter.h"
#include "partman.h"

//////////////////////////////////////////////////////////////////////////
// TypeInfo XML serialisation code

#include "ParticleParams_TypeInfo.h"

DEFINE_INTRUSIVE_LINKED_LIST(CParticleEffect)

static const int nSERIALIZE_VERSION = 20;
static const int nMIN_SERIALIZE_VERSION = 13;

// Serialize a variable using a VarInfo.
void SerializeVar( IXmlNode& xml, void* data, void* def_data, const CTypeInfo::CVarInfo& var, bool bLoading, int flags, int nVersion )
{
	assert(var.GetDim() == 1);

	if (&var.Type == &TypeInfo((void**)0))
		// Never serialize pointers.
		return;
	
	cstr name = var.GetName();

	if (name[0] == '_')
		// Do not serialize private vars.
		return;

	if (bLoading)
	{
		cstr value = xml.getAttr(name);
		var.FromString(data, value, flags);
	}
	else 
	{
		string val;
		if (flags & CTypeInfo::WRITE_SKIP_DEFAULT)
		{
			val = var.ToString(data, flags, def_data ? (char*)def_data + var.Offset : 0);
			if (val.length() == 0)
				return;
		}
		else
			val = var.ToString(data, flags);
		xml.setAttr(name, val);
	}
}

// Serialize a struct to an XML node.
template<class T>
void Serialize( IXmlNode& xml, T& val, T& def_val, bool bLoading, int flags, int nVersion  )
{
	CTypeInfo const& info = TypeInfo(&val);
	if (bLoading)
	{
		int nAttrs = xml.getNumAttributes();
		for (int a = 0; a < nAttrs; a++)
		{
			cstr sKey, sVal;
			xml.getAttributeByIndex(a, &sKey, &sVal);

			CTypeInfo::CVarInfo const* pVar = info.FindSubVar(sKey);
			if (pVar)
			{
				assert(pVar->GetDim() == 1);
				pVar->FromString(&val, sVal, flags);
			}
		}
	}
	else
	{
		for AllSubVars( pVar, info )
		{
			SerializeVar( xml, &val, &def_val, *pVar, false, flags, nVersion );
		}
	}
}

template<class T>
bool FromString(T* data, cstr str, int flags = 0)
{
	return TypeInfo(data).FromString(data, str, flags);
}

template<class T>
T GetAttrValue( IXmlNode const& node, cstr attr, T defval )
{
	T val = defval;
	node.getAttr(attr, val);
	return val;
}

bool IsEnabled( const IXmlNode& node, bool bAll = false, bool bDirectOnly = false )
{
	IXmlNode& nodeParams = *node.findChild("Params");
	if (&nodeParams && GetAttrValue(nodeParams, "Enabled", true))
	{
		// This effect is enabled.
		if (bDirectOnly && GetAttrValue(nodeParams, "SecondGeneration", false))
			return false;
		return true;
	}
	if (bAll)
	{
		// This effect disabled, search for direct children only.
		IXmlNode& nodeChildren = *node.findChild("Childs");
		if (&nodeChildren)
		{
			for (int i = nodeChildren.getChildCount()-1; i >= 0; i--)
			{
				IXmlNode& nodeChild = *nodeChildren.getChild(i);
				if (&nodeChild && IsEnabled(nodeChild, true, true))
					return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
// ResourceParticleParams implementation

static const float fTRAVEL_SAFETY = 0.1f;
static const float fSIZE_SAFETY = 0.01f;

struct SEmitParams
{
	Vec3	vAxis;
	float	fCosMin, fCosMax;
	float fSpeedMin, fSpeedMax;

	bool bOmniDir() const
		{ return fCosMin <= -1.f && fCosMax >= 1.f; }
};

struct SForceParams: SExternForces
{
	float		fDragMin;
	float		fBounce;
};

void AddTravel( AABB& bb, AABB& bbVel, Vec3 vVel, const SExternForces& force, float fTime )
{
	Vec3 vTravel(ZERO);
	Travel( vTravel, vVel, fTime, force );
	bb.Add( vTravel );
	bbVel.Add( vVel );
}

void AddTravel( AABB& bb, AABB& bbVel, Vec3 const& vVel, SForceParams const& force_in, float fTime )
{
	// Add end point.
	SExternForces force = force_in;
	if (vVel*force.vWind <= 0.f)
		force.fDrag = force_in.fDragMin;
	AddTravel( bb, bbVel, vVel, force, fTime );

	// Find time of min/max vals of each component, by solving for v.x etc = 0
	for (int i = 0; i < 3; i++)
	{
		float fT = 0.f;

		if (force.fDrag != 0)
		{
			// VT = G/d + W
			// Vt = 0 = (V-VT) * e^(-d t) + VT
			// e^(-d t) = -VT/(V-VT)
			// t = -log(VT/(VT-V)) / d
			float fVT = force.vAccel[i] / force.fDrag + force.vWind[i];
			if (fVT != vVel[i])
			{
				float f = fVT / (fVT - vVel[i]);
				if (f > 0.f && f < 1.f)
					fT = -logf(f);
			}
		}
		else if (force.vAccel[i] != 0.f)
		{
			// Vt = 0 = V + G t
			// t = -V/G
			fT = -vVel[i] / force.vAccel[i];
		}

		if (fT > 0.f && fT < fTime)
			AddTravel( bb, bbVel, vVel, force, fT );
	}
}

Vec3 GetExtremeEmitVec( Vec3 const& vRefDir, SEmitParams const& emit )
{
	float fEmitCos = vRefDir * emit.vAxis;
	if (fEmitCos >= emit.fCosMin && fEmitCos <= emit.fCosMax)
	{
		// Emit dir is in the cone.
		return vRefDir * emit.fSpeedMax;
	}
	else
	{
		// Find dir in emission cone closest to ref dir.
		Vec3 vEmitPerp = vRefDir - emit.vAxis*fEmitCos;
		vEmitPerp.NormalizeSafe();
		float fCos = clamp_tpl(fEmitCos, emit.fCosMin, emit.fCosMax);
		Vec3 vEmitMax = emit.vAxis*fCos + vEmitPerp*sqrtf(1.f - fCos*fCos);
		vEmitMax *= (vEmitMax*vRefDir > 0.f ? emit.fSpeedMax : emit.fSpeedMin);
		return vEmitMax;
	}
}

void AddEmitDirs( AABB& bb, AABB& bbVel, Vec3 const& vRefDir, SEmitParams const& emit, SForceParams const& force, float fTime )
{
	if (emit.bOmniDir())
	{
		AddTravel( bb, bbVel, vRefDir * emit.fSpeedMax, force, fTime );
		AddTravel( bb, bbVel, vRefDir * -emit.fSpeedMax, force, fTime );
	}
	else
	{
		Vec3 vEmit = GetExtremeEmitVec( vRefDir, emit );
		AddTravel( bb, bbVel, vEmit, force, fTime );
		vEmit = GetExtremeEmitVec( -vRefDir, emit );
		AddTravel( bb, bbVel, vEmit, force, fTime );
	}
}

inline float GetSinMax(float fCosMin, float fCosMax)
{
	return fCosMin*fCosMax < 0.f ? 1.f : sqrtf(1.f - min(fCosMin*fCosMin, fCosMax*fCosMax));
}

inline float MaxComponent(Vec3 const& v)
{
	return max(max(abs(v.x), abs(v.y)), abs(v.z));
}

inline void BounceAdjust( SEmitParams& emit, SForceParams const& force )
{
	emit.fSpeedMax *= force.fBounce;
	if (force.fBounce > 1.f)
		// Arbitrarily increase the speed 2 more factors.
		emit.fSpeedMax *= sqr(force.fBounce);
	else
		emit.fSpeedMin *= force.fBounce;
}

// Compute bounds of a cone of emission, with applied gravity.
void TravelBB( AABB& bb, AABB& bbVel, SEmitParams const& emit, SForceParams const& force, float fTime )
{
	bb = Vec3(ZERO);
	bbVel = Vec3(ZERO);

	// First expand box from emission in cardinal directions.
	AddEmitDirs( bb, bbVel, Vec3(1,0,0), emit, force, fTime );
	AddEmitDirs( bb, bbVel, Vec3(0,1,0), emit, force, fTime );
	AddEmitDirs( bb, bbVel, Vec3(0,0,1), emit, force, fTime );

	// Add extreme dirs along gravity.
	if (!force.vAccel.IsZero())
	{
		Vec3 vDir = force.vAccel.GetNormalized();
		if (MaxComponent(vDir) < 0.999f)
			AddEmitDirs( bb, bbVel, vDir, emit, force, fTime );
	}

	// And wind.
	if (force.fDrag > 0.f && !force.vWind.IsZero())
	{
		Vec3 vDir = force.vWind.GetNormalized();
		if (MaxComponent(vDir) < 0.999f)
			AddEmitDirs( bb, bbVel, vDir, emit, force, fTime );
	}

	// Expand by a safety factor.
	bb.min *= (1.f+fTRAVEL_SAFETY);
	bb.max *= (1.f+fTRAVEL_SAFETY);

	if (force.fBounce > 0.f)
	{
		// Additional expansion for bounce.
		float fFinalSpeedMax = max( bbVel.min.len(), bbVel.max.len() );

		// Test in all directions.
		SEmitParams emit_bounce = emit;
		emit_bounce.fCosMin = -1.f;
		emit_bounce.fCosMax = 1.f;

		SForceParams force_bounce = force;
		force_bounce.fBounce = 0.f;

		AABB bbBounce, bbBounceVel;

		// Bounce at t=0.
		if (emit.fSpeedMax > 0.f)
		{
			BounceAdjust( emit_bounce, force );
			TravelBB( bbBounce, bbBounceVel, emit_bounce, force_bounce, fTime );
			bb.Add(bbBounce);
		}

		// Bounce at half time.
		TravelBB( bbBounce, bbBounceVel, emit, force_bounce, fTime * 0.5f );
		
		emit_bounce.fSpeedMax = fFinalSpeedMax;
		BounceAdjust( emit_bounce, force );

		AABB bbBounce2;
		TravelBB( bbBounce2, bbBounceVel, emit_bounce, force_bounce, fTime * 0.5f );
		bbBounce.Augment(bbBounce2);
		bb.Add(bbBounce);
	}
}

namespace
{
	ILINE void ComputeSpawnSourceBox(AABB& bb, const Vec3S16& vPositionOffset, const Vec3U16& vRandomOffset, const float fPosRandomOffset)
	{
		//implements: 			
		//	bb.Add(vPositionOffset-vRandomOffset, fPosRandomOffset);
		//	bb.Add(vPositionOffset+vRandomOffset, fPosRandomOffset);
		const Vec3 vRad(fPosRandomOffset,fPosRandomOffset,fPosRandomOffset);
		const Vec3 v0 = vPositionOffset-vRandomOffset;
		const Vec3 v1 = vPositionOffset+vRandomOffset;
		bb.min.CheckMin(v0-vRad);
		bb.max.CheckMax(v0+vRad);
		bb.min.CheckMin(v1-vRad);
		bb.max.CheckMax(v1+vRad);
	}
};

void ResourceParticleParams::GetStaticBounds( AABB& bb, const QuatTS& loc, const Vec3& vSpawnSize, bool bWithSize, float fMaxLife, const Vec3& vGravity, const Vec3& vWind ) const
{
	fMaxLife = min(fMaxLife, GetMaxParticleLife());

	bb.Reset(); 

	if (bSpaceLoop)
	{
		if (fCameraMaxDistance > 0.f)
		{
			bb.Add(loc.t, fCameraMaxDistance*loc.s);
		}
		else
		{
			bb.Add(vPositionOffset-vRandomOffset);
			bb.Add(vPositionOffset+vRandomOffset);
			bb.SetTransformedAABB( Matrix34(loc), bb );
		}
	}
	else
	{
		// Compute spawn source box.
		ComputeSpawnSourceBox(bb, vPositionOffset, vRandomOffset, fPosRandomOffset);

		if (!vSpawnSize.IsZero())
		{
			// Expand spawn geometry by emitter geom, and emit in all directions.
			float fRadius = max(bb.min.GetLength(), bb.max.GetLength());
			Vec3 vSize = vSpawnSize + Vec3(fRadius);
			bb = AABB(-vSize, vSize);
		}

		bb.SetTransformedAABB( Matrix34(loc), bb );

		if (eFacing == ParticleFacing_Water)
			// Add slack for positioning emitter above water.
			bb.min.z -= 5.f;

		AABB bbTrav;
		GetTravelBounds( bbTrav, fMaxLife, loc, !vSpawnSize.IsZero(), vGravity, vWind );
		bb.Augment(bbTrav);
	}

	// Particle size.
	if (bWithSize)
	{
		float fMaxSize = fSize.GetMaxValue();
		if (pStatObj)
		{
			float fRadius = pStatObj->GetRadius();
			if (bNoOffset || ePhysicsType == ParticlePhysics_RigidBody)
				// Add object origin.
				fRadius += pStatObj->GetAABB().GetCenter().GetLength();
			fMaxSize *= fRadius;
		}
		else
		{
			if (fStretch)
				// Max stretch based on max speed.
				fMaxSize += (fStretch.GetMaxValue() + abs(fStretch.fOffsetRatio)) * GetMaxSpeed(vGravity, vWind);
			// Radius is size * sqrt(2), round it up slightly.
			fMaxSize *= 1.415f;
		}
		if (LightSource.fIntensity)
			fMaxSize += LightSource.fRadius.GetMaxValue();
		fMaxSize *= loc.s * (1.f + fSIZE_SAFETY);
		bb.Expand( Vec3(fMaxSize) );
	}
}

void ResourceParticleParams::GetTravelBounds( AABB& bbResult, float fTime, const QuatTS& loc, bool bOmniDir, const Vec3& vGravity, const Vec3& vWind ) const
{
	if (fTime <= 0.f)
	{
		bbResult = AABB(Vec3(0), Vec3(0));
		return;
	}

	// Emission direction.
	float fCosMin, fCosMax;
	float fSpeedMul = loc.s;

	if (bOmniDir)
	{
		fCosMin = -1.f;
		fCosMax = 1.f;
	}
	else
	{
		// Max sin and cos of emission relative to emit dir.
		fCosMin = cosf(DEG2RAD( fEmitAngle.GetMaxValue() + fFocusAngle.GetMaxValue() ));
		fCosMax = cosf(DEG2RAD( fEmitAngle.GetMinValue() + fFocusAngle.GetMinValue() ));
	}
	if (eFacing == ParticleFacing_Horizontal)
	{
		fSpeedMul *= GetSinMax(fCosMin, fCosMax);
		fCosMin = fCosMax = 0.f;
	}

	SForceParams force;
	force.vAccel = vGravity * fGravityScale.GetMaxValue() + vAcceleration * loc.s;
	force.vWind = vWind;
	force.fDrag = fAirResistance.GetMaxValue();
	force.fDragMin = fAirResistance.GetMinValue();
	force.fBounce = ePhysicsType > ParticlePhysics_None ? max(+fBounciness, 0.f) : 0.f;

	Vec3 vEmitAxis = bFocusGravityDir ? -vGravity.GetNormalizedSafe(Vec3(0,0,-1)) : loc.q.GetColumn1();
	SEmitParams emit = { vEmitAxis, fCosMin, fCosMax, fSpeed.GetMinValue()*fSpeedMul, fSpeed.GetMaxValue()*fSpeedMul };

	AABB bbVel;
	TravelBB( bbResult, bbVel, emit, force, fTime );

	if (ePhysicsType >= ParticlePhysics_SimplePhysics)
	{
		// For interaction with physics system, arbitrarily double the size.
		bbResult.min *= 2.f;
		bbResult.max *= 2.f;
	}

	if (fTurbulence3DSpeed)
	{
		// Expansion from 3D turbulence.
		float fAccel = fTurbulence3DSpeed.GetMaxValue() * isqrt_tpl(fTime);
		SExternForces forces = { Vec3(fAccel), Vec3(ZERO), fAirResistance.GetMinValue() };
		Vec3 vTrav(ZERO), vVel(ZERO);
		Travel( vTrav, vVel, fTime, forces );
		bbResult.Expand( vTrav );
	}

	// Expansion from spiral turbulence.
	if (fTurbulenceSpeed)
	{
		Vec3 vExp;
		vExp.x = vExp.y = fTurbulenceSize.GetMaxValue() * loc.s;
		vExp.z = 0.f;
		bbResult.Expand( vExp );
	}
}

void ResourceParticleParams::ComputeEnvironmentFlags()
{
	// Needs updated environ if particles interact with gravity or air.
	nEnvFlags = 0;

	if (tVisibleUnderwater != Trinary_Both || eFacing == ParticleFacing_Water)
		nEnvFlags |= ENV_WATER;
	if (fAirResistance)
		nEnvFlags |= ENV_WIND;
	if (fGravityScale || bFocusGravityDir)
		nEnvFlags |= ENV_GRAVITY;
	if (eForceGeneration != ParticleForce_None && eForceGeneration != ParticleForce_Target)
		nEnvFlags |= ENV_FORCE;
	if (ePhysicsType == ParticlePhysics_SimpleCollision)
	{
		if (bCollideTerrain)
			nEnvFlags |= ENV_TERRAIN;
		if (bCollideStaticObjects)
			nEnvFlags |= ENV_STATIC_ENT;
		if (bCollideDynamicObjects)
			nEnvFlags |= ENV_DYNAMIC_ENT;
	}

	// Rendering params.
	if (fSize && fAlpha)
	{
		if (pStatObj != 0)
			nEnvFlags |= REN_GEOMETRY;
		else if ((nTexId > 0 && nTexId < MAX_VALID_TEXTURE_ID) || pMaterial != 0)
		{
			if (eFacing == ParticleFacing_Decal)
				nEnvFlags |= REN_DECAL;
			else
				nEnvFlags |= REN_SPRITE;
		}
		if (bReceiveShadows)
			nEnvFlags |= REN_TAKE_SHADOWS;
		if (bCastShadows)
			nEnvFlags |= REN_CAST_SHADOWS;
	}
	if (LightSource.fIntensity && LightSource.fRadius)
		nEnvFlags |= REN_LIGHTS;
	if (!sSound.empty())
		nEnvFlags |= REN_SOUND;

	//
	// Check whether effect is threadable.
	//
	nEnvFlags |= REN_THREAD;

	// Disable threaded update for any physics interactions.
	if (ePhysicsType >= ParticlePhysics_SimpleCollision)
		// Physicalised particles.
		nEnvFlags &= ~REN_THREAD;
	else if (nEnvFlags & (ENV_STATIC_ENT | ENV_DYNAMIC_ENT))
		// Physics queries for collision.
		nEnvFlags &= ~REN_THREAD;
	
	// for SPUs also skip sound and other things






	//
	// Compute renderer flags.
	//

	nRenObjFlags = 0;
	uint32 nRenObjState = 0;

	switch (eBlendType)
	{
		case ParticleBlendType_AlphaBased:
			nRenObjState = OS_ALPHA_BLEND | OS_ALPHATEST_GREATER;
			break;
		case ParticleBlendType_ColorBased:
			nRenObjState = OS_COLOR_BLEND;
			break;
		case ParticleBlendType_Additive:
			nRenObjState = OS_ADD_BLEND;
			break;
	}

	if (bDrawNear)
		nRenObjFlags |= FOB_NEAREST;
	if (bNotAffectedByFog)
		nRenObjFlags |= FOB_NO_FOG;

	if (bSoftParticle && (eFacing == ParticleFacing_Camera))
		nRenObjFlags |= FOB_SOFT_PARTICLE;

	// Combine the 2 flag sets. The ones we use do not overlap.
	assert(!(nRenObjFlags & nRenObjState));
	nRenObjFlags |= nRenObjState;
}

float ResourceParticleParams::GetMaxSpeed(const Vec3& vGravity, const Vec3& vWind) const
{
	Vec3 vAccel = vAcceleration + vGravity * fGravityScale.GetMaxValue();
	float fAccel = vAccel.GetLength();

	float fDrag = fAirResistance.GetMinValue();
	if (fDrag > 0.f)
	{
		float fWind = vWind.GetLength();
		float fTerm = fWind + fAccel / fDrag;
		return max( fSpeed.GetMaxValue(), fTerm );
	}
	else
	{
		// Simple non-drag computation.
		return abs(fSpeed.GetMaxValue()) + fAccel * fParticleLifeTime.GetMaxValue();
	}
}

//////////////////////////////////////////////////////////////////////////
int ResourceParticleParams::LoadResources( const char* sName )
{
	// Load only what is not yet loaded. Check everything, but caller may check params.bResourcesLoaded first.
	// Call UnloadResources to force unload/reload.

	if (!bEnabled)
		return 0;

	if (ResourcesLoaded()|| gEnv->pSystem->IsDedicated())
	{
		ComputeEnvironmentFlags();
		return 0;
	}

	int nLoaded = 0;

	// Load material.
	if (!pMaterial && !nTexId && !sMaterial.empty())
	{
		pMaterial = Get3DEngine()->GetMaterialManager()->LoadMaterial( sMaterial.c_str() );
		if (!pMaterial || pMaterial == Get3DEngine()->GetMaterialManager()->GetDefaultMaterial())
		{
			CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_WARNING,"Particle effect material not found: %s (%s)", sMaterial.c_str(), sName );

			// Load error texture for artist debugging.
			pMaterial = 0;
			nTexId = GetRenderer()->EF_LoadTexture("!Error", FT_DONT_STREAM, eTT_2D)->GetTextureID();
		}
		else
			nLoaded++;
	}

	// Load texture.
	if (!nTexId && !sTexture.empty())
	{
#if !defined(NULL_RENDERER)
		const uint32 textureLoadFlags = (bTextureUnstreamable)?FT_DONT_STREAM:0;
		nTexId = GetRenderer()->EF_LoadTexture(sTexture.c_str(), textureLoadFlags, eTT_2D)->GetTextureID();
		if (!nTexId)
			CryLog( "Particle effect texture not found: %s (%s)", sTexture.c_str(), sName );
		else
			nLoaded++;
#endif
	}

	// Set aspect ratio.
	if (fTexAspect == 0.f)
	{
		UpdateTextureAspect();
	}

	// Load geometry.
	if (!pStatObj && !sGeometry.empty())
	{
		pStatObj = Get3DEngine()->LoadStatObj(sGeometry.c_str(), NULL, NULL, !bGeometryUnstreamable);
		if (!pStatObj)
			CryLog( "Particle effect geometry not found: %s (%s)", sGeometry.c_str(), sName );
		else
			nLoaded++;
	}

	// Find material type.
	if (!iPhysMat && !sSurfaceType.empty())
	{
		ISurfaceType *pSurface = Get3DEngine()->GetMaterialManager()->GetSurfaceTypeByName( sSurfaceType.c_str() );
		if (pSurface)
			iPhysMat = pSurface->GetId();
	}

	// Process sound
	if (!sSound.empty())
	{
		// TODO needs to be properly replaced by the dependency tracker
		gEnv->pSoundSystem->Precache(sSound.c_str(), 0, FLAG_SOUND_PRECACHE_EVENT_DEFAULT);
	}

	ComputeEnvironmentFlags();

	nEnvFlags |= EFF_LOADED;
	return nLoaded;
}

//////////////////////////////////////////////////////////////////////////
void ResourceParticleParams::SetTileInfo( STileInfo& info ) const
{
	info.vSize.x = 1.f / TextureTiling.nTilesX;
	info.vSize.y = 1.f / TextureTiling.nTilesY;
	info.fTileStart = (float)TextureTiling.nFirstTile;
	if (TextureTiling.nAnimFramesCount && TextureTiling.bAnimBlend)
		// Actual number of used tiles.
		info.fTileCount = (float)TextureTiling.GetFrameCount();
	else
		// Max 8-bit interp value, for interpolated tiling.
		info.fTileCount = 255.f;
}

void ResourceParticleParams::UpdateTextureAspect()
{
	TextureTiling.Correct();

	fTexAspect = 1.f;
	ITexture* pTexture = GetRenderer()->EF_GetTextureByID( (int)nTexId );
	if (pTexture)
	{
		float fWidth = pTexture->GetWidth() / (float)TextureTiling.nTilesX;
		float fHeight = pTexture->GetHeight() / (float)TextureTiling.nTilesY;
		if (fHeight == 0.f)
			fTexAspect = 0.f;
		else
			fTexAspect = fWidth / fHeight;
	}
}

//////////////////////////////////////////////////////////////////////////
void ResourceParticleParams::UnloadResources()
{
	// To do: Handle materials
	if (nTexId != 0)
	{
		GetRenderer()->RemoveTexture( (unsigned int)nTexId );
		nTexId = 0;
	}
	pStatObj = 0;
	pMaterial = 0;
	iPhysMat = 0;
	fTexAspect = 0.f;
	nEnvFlags &= ~EFF_LOADED;
}

template<class T>
inline void GetFloatAttr(XmlNodeRef node, cstr sKey, T& val)
{
	float f;
	if (node->getAttr( sKey, f))
		val = f;
}

void ResourceParticleParams::Correct( int nVersion, XmlNodeRef paramsNode )
{
	CTypeInfo const& info = ::TypeInfo(this);

	switch (nVersion)
	{
		case 13:
		{
			// Texture tile conversion.
			cstr sRect = paramsNode->getAttr("TextureUVRect");
			if (*sRect)
			{
				RectF rect;
				::TypeInfo(&rect).FromString(&rect, sRect, CTypeInfo::READ_SKIP_EMPTY);
				TextureTiling.nTilesX = rect.w > 0.f ? int_round(1.f / rect.w) : 1;
				TextureTiling.nTilesY = rect.h > 0.f ? int_round(1.f / rect.h) : 1;
				TextureTiling.nFirstTile = rect.w*rect.h > 0.f ? int_round((rect.x + rect.y / rect.h) / rect.w) : 0;
			}

			paramsNode->getAttr( "TextureVariantCount", TextureTiling.nVariantCount );
			paramsNode->getAttr( "TexAnimFramesCount", TextureTiling.nAnimFramesCount );
			GetFloatAttr( paramsNode, "AnimFramerate", TextureTiling.fAnimFramerate );
			FromString(&TextureTiling.bAnimCycle, paramsNode->getAttr("AnimCycle"));
		}

		case 14:
			FromString(&bSpawnOnParentCollision, paramsNode->getAttr("SpawnOnParentDeath"), CTypeInfo::READ_SKIP_EMPTY);
			if (string(paramsNode->getAttr("PhysicsType")) == "FullCollision")
				ePhysicsType = ParticlePhysics_SimpleCollision;
			else if (ePhysicsType == ParticlePhysics_SimpleCollision)
				bCollideTerrain = true;

		case 15:
			if (GetAttrValue(*paramsNode, "OnlyUnderWater", false))
				tVisibleUnderwater = Trinary_If_True;
			else if (GetAttrValue(*paramsNode, "OnlyAboveWater", false))
				tVisibleUnderwater = Trinary_If_False;
			if (GetAttrValue(*paramsNode, "OnlyOutdoors", false))
				tVisibleIndoors = Trinary_If_False;

		case 16:
			if (ePhysicsType >= ParticlePhysics_SimpleCollision)
			{
				// Apply previous version's defaults.
				bCollideTerrain = bCollideStaticObjects = true;
				paramsNode->getAttr("CollideTerrain", bCollideTerrain);
				paramsNode->getAttr("CollideObjects", bCollideStaticObjects);
				bCollideDynamicObjects = bCollideStaticObjects; 
			}

		case 17:
		case 18:
			// Rotation fixes.
			// Set 3D particles to Free rotation, matching old behavior. Can now be changed.
			if (!sGeometry.empty() || (sTexture.empty() && sMaterial.empty()))
				eFacing = ParticleFacing_Free;
			if (eFacing == ParticleFacing_Camera)
			{
				// Swap y and z angle vals.
				std::swap(vInitAngles.y, vInitAngles.z);
				std::swap(vRandomAngles.y, vRandomAngles.z);
				std::swap(vRotationRate.y, vRotationRate.z);
				std::swap(vRandomRotationRate.y, vRandomRotationRate.z);
			}

		case 19:
			// Some params combined into structs.
			FromString(&LightSource.fRadius, paramsNode->getAttr("LightSourceRadius"), CTypeInfo::READ_SKIP_EMPTY);
			FromString(&LightSource.fIntensity, paramsNode->getAttr("LightSourceIntensity"), CTypeInfo::READ_SKIP_EMPTY);
			FromString(&LightSource.fHDRDynamic, paramsNode->getAttr("LightHDRDynamic"), CTypeInfo::READ_SKIP_EMPTY);
			GetFloatAttr( paramsNode, "StretchOffsetRatio", fStretch.fOffsetRatio );
			paramsNode->getAttr( "TailSteps", fTailLength.nTailSteps );

			paramsNode->getAttr( "IgnoreAttractor", TargetAttraction.bIgnore );

		case 20:	// Current version
			if (!ePhysicsType && GetAttrValue(*paramsNode, "BindToEmitter", false))
			{
				// Obsolete parameter, set equivalent params.
				bMoveRelEmitter = true;
				vPositionOffset.zero();
				vRandomOffset.zero();
				fPosRandomOffset = 0.f;
				fSpeed = 0.f;
				fInheritVelocity = 0.f;
				fGravityScale = 0.f;
				fAirResistance = 0.f;
				vAcceleration.zero();
				fTurbulence3DSpeed = 0.f;
				fTurbulenceSize = 0.f;
				fTurbulenceSpeed = 0.f;
				bSpaceLoop = false;
			}
			break;

		default:
			assert(!"Particle version out of range 13-20");
	};

	TextureTiling.Correct();
}

//////////////////////////////////////////////////////////////////////////
// ParticleEffect implementation

//////////////////////////////////////////////////////////////////////////
CParticleEffect::CParticleEffect()
: m_parent(0), m_pCounts(0), m_pParticleParams(0)
{
}

CParticleEffect::CParticleEffect( const char* sName )
: m_parent(0), m_pCounts(0), m_pParticleParams(0), m_strFullName(sName)
{
}

//////////////////////////////////////////////////////////////////////////
CParticleEffect::~CParticleEffect()
{
	assert(!m_pCounts);
	UnloadResources();
	delete m_pParticleParams;
}

void CParticleEffect::SetEnabled( bool bEnabled )
{
	if (bEnabled != IsEnabled())
	{
		InstantiateParams();
		m_pParticleParams->bEnabled = bEnabled;
		CPartManager::GetManager()->UpdateEmitters(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::SetName( const char *sFullName )
{
	CPartManager::GetManager()->RenameEffect( this, sFullName );
}

const char* CParticleEffect::GetBaseName() const
{
	int nBase = m_parent ? m_strFullName.rfind('.') // For child effects, return only last component.
											 : m_strFullName.find('.'); // For top effects, return everything after lib name.
	if (nBase == string::npos)
		nBase = 0;
	else
		nBase++;
	return &m_strFullName[nBase];
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::AddChild( IParticleEffect *pEffect )	
{
	assert( pEffect );
	CParticleEffect* pEff = (CParticleEffect*) pEffect;
	m_childs.push_back(pEff);
	pEff->m_parent = this;
	CPartManager::GetManager()->UpdateEmitters(this);
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::RemoveChild( IParticleEffect *pEffect )
{
	assert( pEffect );
	stl::find_and_erase( m_childs,pEffect );
	CPartManager::GetManager()->UpdateEmitters(this);
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::ClearChilds()
{
	m_childs.clear();
	CPartManager::GetManager()->UpdateEmitters(this);
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::InsertChild( int slot, IParticleEffect *pEffect )
{
	if (slot < 0)
		slot = 0;
	if (slot > m_childs.size())
		slot = m_childs.size();

	assert( pEffect );
	CParticleEffect* pEff = (CParticleEffect*) pEffect;
	pEff->m_parent = this;
//	AddRef();
	m_childs.insert( m_childs.begin() + slot, pEff );
	CPartManager::GetManager()->UpdateEmitters(this);
}

//////////////////////////////////////////////////////////////////////////
int CParticleEffect::FindChild( IParticleEffect *pEffect ) const
{
	for_array (i, m_childs)
	{
		if (&m_childs[i] == pEffect)
			return i;
	}
	return -1;
}

CParticleEffect* CParticleEffect::FindChild( const char* szChildName ) const
{
	for_array (i, m_childs)
	{
		if (strcmp(m_childs[i].GetBaseName(), szChildName) == 0)
			return &m_childs[i];
	}

	return 0;
}

void CParticleEffect::LoadFromXML()
{
	if (m_XmlSource)
	{
		Serialize( m_XmlSource, true, true );
		SetXMLSource(0);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CParticleEffect::LoadResources( bool bAll ) const
{
	int nLoaded = 0;
	if (m_pParticleParams && m_pParticleParams->bEnabled && !m_pParticleParams->ResourcesLoaded())
		nLoaded = m_pParticleParams->LoadResources(GetName());
	if (bAll)
		for_array (i, m_childs)
			nLoaded += (int)m_childs[i].LoadResources();
	return nLoaded > 0;
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::UnloadResources( bool bAll ) const
{
	if (m_pParticleParams)
		m_pParticleParams->UnloadResources();
	if (bAll)
		for_all (m_childs).UnloadResources();
}

void CParticleEffect::SetParticleParams( const ParticleParams &params )
{
	InstantiateParams();
	if (params.sTexture != m_pParticleParams->sTexture || params.sMaterial != m_pParticleParams->sMaterial)
	{
		if (m_pParticleParams->nTexId != 0)
		{
			GetRenderer()->RemoveTexture( (unsigned int)m_pParticleParams->nTexId );
			m_pParticleParams->nTexId = 0;
			m_pParticleParams->fTexAspect = 0.f;
		}
		m_pParticleParams->pMaterial = 0;
		m_pParticleParams->nEnvFlags &= ~EFF_LOADED;
	}
	if (params.sGeometry != m_pParticleParams->sGeometry)
	{
		m_pParticleParams->pStatObj = 0;
		m_pParticleParams->nEnvFlags &= ~EFF_LOADED;
	}
	if (params.sSurfaceType != m_pParticleParams->sSurfaceType)
	{
		m_pParticleParams->iPhysMat = 0;
		m_pParticleParams->nEnvFlags &= ~EFF_LOADED;
	}
	if (memcmp(&params.TextureTiling, &m_pParticleParams->TextureTiling, sizeof(params.TextureTiling)) != 0)
	{
		m_pParticleParams->fTexAspect = 0.f;
		m_pParticleParams->nEnvFlags &= ~EFF_LOADED;
	}

	static_cast<ParticleParams&>(*m_pParticleParams) = params;
	m_pParticleParams->LoadResources(GetName());
	CPartManager::GetManager()->UpdateEmitters(this);
}

//////////////////////////////////////////////////////////////////////////
IParticleEmitter* CParticleEffect::Spawn( bool bIndependent, const Matrix34& loc )
{
	return CPartManager::GetManager()->CreateEmitter( bIndependent, loc, this );
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::Serialize( XmlNodeRef node, bool bLoading, bool bChilds )
{
	XmlNodeRef root = node;
	while (root->getParent())
		root = root->getParent();

	if (bLoading)
	{
		if (m_parent)
			// Set simple name, will be automatically qualified with hierarchy.
			SetName( node->getAttr("Name") );
		else
		{
			// Qualify with library name.
			stack_string sFullName;
			stack_string sRootTag = root->getTag();
			if (sRootTag == "ParticleLibrary" || sRootTag == "LevelLibrary")
				sFullName = root->getAttr("Name");
			else if (sRootTag == "Level")
				sFullName = sRootTag;
			if (sFullName.length() > 0)
				sFullName += ".";
			sFullName += node->getAttr("Name");
			SetName(sFullName.c_str());
		}

		int nVersion = 0;
		root->getAttr( "ParticleVersion", nVersion );
		if (nVersion < nMIN_SERIALIZE_VERSION || nVersion > nSERIALIZE_VERSION)
		{
			Warning( "Particle Effect %s not loaded: version (%d) out of supported range (%d-%d)",
				GetName(), nVersion, nMIN_SERIALIZE_VERSION, nSERIALIZE_VERSION );
			delete m_pParticleParams;
			m_pParticleParams = 0;
			return;
		}

		XmlNodeRef paramsNode = node->findChild("Params");
		if (paramsNode && (gEnv->IsEditor() || ::IsEnabled(*node, false)))
		{
			// Init params, then read from XML.
			if (!m_pParticleParams)
				m_pParticleParams = new ResourceParticleParams;
			else
				new(m_pParticleParams) ResourceParticleParams;

			ResourceParticleParams& params = *m_pParticleParams;
			int nReadFlags = CTypeInfo::READ_SKIP_EMPTY;
			if (!gEnv->IsEditor())
				nReadFlags |= CTypeInfo::READ_FINALIZE;
			::Serialize( *paramsNode, params, params, true, nReadFlags, nVersion );

			params.Correct( nVersion, paramsNode );

			stack_string soundName(params.sSound.c_str());
			if (params.bEnabled && soundName.Right(4) == ".wav")
			{
				CryLog("Particle Effect %s uses legacy wav sound %s, legacy params ignored", GetName(), params.sSound.c_str());
			}

			if (params.bSpaceLoop && params.fCameraMaxDistance == 0.f && params.vRandomOffset.GetVolume() == 0.f)
			{
				Warning( "Particle Effect %s has zero space loop volume: disabled", GetName() );
				params.bEnabled = false;
			}

			if (!params.sSound.empty())
			{
				gEnv->pSoundSystem->Precache(params.sSound.c_str(), 0, FLAG_SOUND_PRECACHE_EVENT_DEFAULT);
			}
		}

		if (bChilds)
		{
			// Serialize childs.
			XmlNodeRef childsNode = node->findChild( "Childs" );
			if (childsNode)
			{
				for (int i = 0; i < childsNode->getChildCount(); i++)
				{
					XmlNodeRef xchild = childsNode->getChild(i);
					if (!gEnv->IsEditor() && !::IsEnabled(*xchild, true))
						// Skip disabled effect branches.
						continue;

					IParticleEffect*	pChildEffect = 0;

					if (cstr sChildName = xchild->getAttr("Name"))
						pChildEffect = FindChild(sChildName);

					if (!pChildEffect)
					{
						pChildEffect = new CParticleEffect();
						AddChild( pChildEffect );
					}

					pChildEffect->Serialize( xchild,bLoading,bChilds );
				}
				// m_childs.shrink();
			}
		}
	}
	else
	{
		// Saving.
		node->setAttr( "Name", GetBaseName() );
		root->setAttr( "ParticleVersion", nSERIALIZE_VERSION );

		// Save particle params.
		XmlNodeRef paramsNode = node->newChild( "Params" );

		ParticleParams paramsDef;
		::Serialize( *paramsNode, non_const(GetParticleParams()), paramsDef, 
			false, CTypeInfo::WRITE_TRUNCATE_SUB | CTypeInfo::WRITE_SKIP_DEFAULT, nSERIALIZE_VERSION );

		if (bChilds && !m_childs.empty())
		{
			// Serialize childs.
			XmlNodeRef childsNode = node->newChild( "Childs" );
			for_array (i, m_childs)
			{
				XmlNodeRef xchild = childsNode->newChild( "Particles" );
				m_childs[i].Serialize( xchild,bLoading,bChilds );
			}
		}
	}
}

void CParticleEffect::GetMemoryUsage(ICrySizer* pSizer) const
{
	pSizer->AddString(m_strFullName);
	if (m_pParticleParams)
	{
		pSizer->Add(*m_pParticleParams);
		TypeInfo(m_pParticleParams).GetMemoryUsage(pSizer, m_pParticleParams);
	}
	GetPtrArrayMemoryUsage(pSizer, m_childs);
}

void CParticleEffect::GetEffectCounts( SEffectCounts& counts, bool bParentInactive ) const
{
	counts.nLoaded++;
	if (IsEnabled())
		counts.nEnabled++;
	if (GetParams().ResourcesLoaded())
		counts.nUsed++;
	if (!(bParentInactive && GetParams().bSecondGeneration) && CPartManager::GetManager()->IsActive(GetParams()))
		counts.nActive++;
	else
		bParentInactive = true;

	for_array (i, m_childs)
		m_childs[i].GetEffectCounts(counts, bParentInactive);
}

bool CParticleEffect::IsImmortal() const
{
	if (m_pParticleParams && m_pParticleParams->IsImmortal())
		return true;
	for_array (i, m_childs)
	{
		if (!m_childs[i].GetParticleParams().bSecondGeneration && m_childs[i].IsImmortal())
			return true;
	}
	return false;
}

bool CParticleEffect::IsIndirect() const
{
	if (m_parent)
	{
		for (CParticleEffect const* pEffect = this; pEffect; pEffect = pEffect->m_parent)
			if (pEffect->GetParams().bSecondGeneration)
				return true;
	}
	return false;
}


float CParticleEffect::GetMaxParticleLife(bool bAll) const
{
	float fParticleLife = GetParams().GetMaxParticleLife();
	if (bAll) for_array (i, m_childs)
	{
		CParticleEffect const& child = m_childs[i];
		if (child.GetParams().bSecondGeneration)
		{
			float fChildLife = child.GetMaxParticleLife();
			if (child.GetParams().bSpawnOnParentCollision)
				fChildLife += GetParams().GetMaxParticleLife();
			else
			{
				fChildLife += child.GetParams().fSpawnDelay.GetMaxValue();
				if (child.GetParams().bContinuous)
				{
					// Add emitter lifetime.
					if (child.GetParams().fEmitterLifeTime)
						fChildLife += min(child.GetParams().fEmitterLifeTime.GetMaxValue(), GetParams().GetMaxParticleLife());
					else
						fChildLife += GetParams().GetMaxParticleLife();
				}
			}
			fParticleLife = max(fParticleLife, fChildLife);
		}
		else
			fParticleLife = fParticleLife;
	}
	return fParticleLife;
}

void CParticleEffect::InitCounts()
{
	assert(!m_pCounts);
	m_pCounts = new SSumParticleCounts();

	for_all (m_childs).InitCounts();
}

void CParticleEffect::ClearCounts()
{
	delete m_pCounts;
	m_pCounts = 0;

	for_all (m_childs).ClearCounts();
}

void CParticleEffect::SumCounts()
{
	assert(m_pCounts);
	m_pCounts->SumParticlesAlloc = m_pCounts->ParticlesAlloc;
	m_pCounts->SumEmittersAlloc = m_pCounts->EmittersAlloc;

	for_array (i, m_childs)
	{
		m_childs[i].SumCounts();
		m_pCounts->SumParticlesAlloc += m_childs[i].m_pCounts->SumParticlesAlloc;
		m_pCounts->SumEmittersAlloc += m_childs[i].m_pCounts->SumEmittersAlloc;
	}
}

void CParticleEffect::PrintCounts( bool bHeader )
{
	if (bHeader)
		CryLogAlways(
			"Effect, "
			"Ems ren, Ems act, Ems all, "
			"Parts ren, Parts act, Parts all, "
			"Fill ren, Fill act, "
			"Coll trr, Coll obj, Clip, "
			"Reiter, Ovf, Reject"
		);
	SSumParticleCounts const& counts = GetCounts();
	if (counts.SumEmittersAlloc)
	{
		int iLevel = 0;
		for (CParticleEffect* p = m_parent; p; p = p->m_parent)
			iLevel++;
		if (iLevel == 0 || counts.EmittersAlloc || counts.ParticlesAlloc)
		{
			float fPixToScreen = 1.f / ((float)GetRenderer()->GetWidth() * (float)GetRenderer()->GetHeight());
			CryLogAlways(
				"%*s%s, "
				"%.0f, %.0f, %.0f, "
				"%.0f, %.0f, %.0f, "
				"%.3f, %.3f, "
				"%.2f, %.2f, %.2f, "
				"%.0f, %.2f, %.2f",
				iLevel, "", GetName(), 
				counts.EmittersRendered, counts.EmittersActive, counts.EmittersAlloc,
				counts.ParticlesRendered, counts.ParticlesActive, counts.ParticlesAlloc, 
				counts.PixelsRendered * fPixToScreen, counts.PixelsProcessed * fPixToScreen,
				counts.ParticlesCollideTerrain, counts.ParticlesCollideObjects, counts.ParticlesClip,
				counts.ParticlesReiterate, counts.ParticlesOverflow, counts.ParticlesReject);
		}
		for_all (m_childs).PrintCounts();
	}
}

ResourceParticleParams gEmptyParams;
	
const ResourceParticleParams& CParticleEffect::GetParams() const
{ 
	const ResourceParticleParams *ret = m_pParticleParams;
	
	if (m_pParticleParams == NULL)
	{
		gEmptyParams.bEnabled = false;
		ret = &gEmptyParams;
	}
	
	return *ret;
}

#include UNIQUE_VIRTUAL_WRAPPER(IParticleEffect)

