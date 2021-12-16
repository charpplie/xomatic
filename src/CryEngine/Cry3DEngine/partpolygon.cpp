////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   partpolygon.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: sprite particles, big independent polygons
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "Particle.h"
#include "partman.h"
#include "ObjMan.h"
#include "3dEngine.h"

#include <SpuUtils.h>

#define SPU_VERTEX_BUFFER_SIZE (120*1024)


















template<class T>
class SaveRestore
{
public:
	SaveRestore(T& var)
		: m_pVar(&var), m_Val(var)
	{}
	SaveRestore(T& var, T const& newval)
		: m_pVar(&var), m_Val(var)
	{
		*m_pVar = newval;
	}
	~SaveRestore()
	{
		*m_pVar = m_Val;
	}
protected:
	T*	m_pVar;
	T		m_Val;
};

static const float fMIN_RENDER_ALPHA = 0.5f / 255.f;

// Shadow alpha not supported.
//static int FOB_SHADOW_ALPHA_FIX = FOB_RENDER_INTO_SHADOWMAP;

struct SParticleRenderData
{
	// Computed params needed for rendering.
	ColorF	cColor;						// Color & alpha.
	float		fSize;						// Particle average radius.
	float		fDistSq;					// Distance^2 from camera.
	float		fFillPix;					// Approx number of pixels drawn.
};

struct SParticleVertexContext: SParticleRenderContext
{
	SVertexParticle::SpriteInfo
											m_TexInfo;							// Pre-computed info vals.

	bool								m_bSwapRGB;
	float								m_fEmitterScale;
	float								m_fInvMinPix;
	float								m_fFillFactor;
	float								m_fFillMax;
	float								m_fFillFade;
	float								m_fDistFuncCoefs[3];		// Coefficients for alpha(dist^2) function.
	Vec2								m_vTexAspect;						// Multipliers for non-square textures (max is always 1).

	SVisEnviron const*	m_pVisEnv;							// Vis env to clip against, if needed.

	float								m_fPixelsProcessed;
	float								m_fPixelsRendered;

	SParticleVertexContext( SParticleRenderContext const& RenderContext, float fMaxContainerPixels, CParticleContainer* pContainer )
	:	SParticleRenderContext(RenderContext)
	{
		Init( fMaxContainerPixels, pContainer, 
					pContainer->GetRenderer()->GetFeatures(), pContainer->GetRenderer()->GetCamera().GetFov() );
	}

	SParticleVertexContext( SParticleRenderContext const& RenderContext, float fMaxContainerPixels, CParticleContainer* pContainer, int nRendererFeatures, float fFov )
	: SParticleRenderContext(RenderContext)
	{
		Init( fMaxContainerPixels, pContainer, nRendererFeatures, fFov );
	}

	void Init( float fMaxContainerPixels, CParticleContainer* pContainer, int nRendererFeatures, float fFov );

	inline float DistFunc( float fDist ) const
	{
		return clamp_tpl( m_fDistFuncCoefs[0] + m_fDistFuncCoefs[1]*fDist + m_fDistFuncCoefs[2]*fDist*fDist, 0.f, 1.f	);
	}
};

//////////////////////////////////////////////////////////////////////////
bool CParticle::RenderGeometry( SRendParams& RenParamsShared, SParticleVertexContext& Context )
{
	// Render 3d object
	if (!m_pStatObj)
		return false;

	const ResourceParticleParams& params = GetParams();

	SParticleRenderData RenderData;
	ComputeRenderData( RenderData, Context ); 
	if (RenderData.fSize == 0.f)
		return false;
	RenderData.cColor.a *= RenParamsShared.fAlpha;
	if (RenderData.cColor.a < fMIN_RENDER_ALPHA)
		return false;
	if (Get3DEngine()->_GetRenderIntoShadowmap())
	{
		// Shadow alpha (and color) not supported, scale size instead.
		RenderData.fSize *= RenderData.cColor.a;
		RenderData.cColor = Col_White;
	}

	// Get matrices.
	Matrix34 matPart( Vec3(RenderData.fSize), m_qRot, m_vPos );

#ifdef PARTICLE_MOTION_BLUR
  Matrix34 matPartPrev( Vec3(RenderData.fSize), m_qRotPrev, m_vPosPrev );
  
  m_qRotPrev = m_qRot;
  m_vPosPrev = m_vPos;
#endif

	if (!params.bNoOffset && params.ePhysicsType != ParticlePhysics_RigidBody)
	{
		// Recenter object pre-rotation.
		Vec3 vCenter = m_pStatObj->GetAABB().GetCenter();
    Matrix34 pTranslationMat = Matrix34::CreateTranslationMat(-vCenter);
		matPart = matPart * pTranslationMat;
#ifdef PARTICLE_MOTION_BLUR
    matPartPrev = matPartPrev * pTranslationMat;
#endif
	}

#ifdef SHARED_GEOM
	if (RenParamsShared.pInstInfo && m_pStatObj == params.pStatObj)
	{
		// Add shared geometry instance.
		SInstanceInfo& Inst = *RenParamsShared.pInstInfo->arrMats.push_back();
		Inst.m_Matrix			= matPart;
		// Inst.m_PrevMatrix	= matPartPrev;		// Add this when it's supported.
		Inst.m_AmbColor		= RenParamsShared.AmbientColor * RenderData.cColor;
	}
	else
#endif
	{
		// Render separate draw call.
		SaveRestore<SInstancingInfo*> SaveInst(RenParamsShared.pInstInfo, 0);
		SaveRestore<ColorF> SaveColor(RenParamsShared.AmbientColor);
		SaveRestore<float> SaveAlpha(RenParamsShared.fAlpha);

		// Apply particle color to RenParams.Ambient, tho it's not quite the same thing.
		RenParamsShared.AmbientColor.r *= RenderData.cColor.r;
		RenParamsShared.AmbientColor.g *= RenderData.cColor.g;
		RenParamsShared.AmbientColor.b *= RenderData.cColor.b;
		RenParamsShared.fAlpha = RenderData.cColor.a;

		RenParamsShared.pMatrix = &matPart;            
	#ifdef PARTICLE_MOTION_BLUR
		RenParamsShared.pPrevMatrix = &matPartPrev;
	#endif

		m_pStatObj->Render(RenParamsShared);
	}

	return true;
}

void CParticleContainer::RenderGeometry( const SRendParams& RenParams, const SParticleRenderContext& RenContext )
{
  FUNCTION_PROFILER_CONTAINER(this);

	const ResourceParticleParams& params = GetParams();
	SRendParams RenParamsGeom = RenParams;
	RenParamsGeom.nMotionBlurAmount = clamp_tpl(int_round(params.fMotionBlurScale * 128), 0, 255);
  RenParamsGeom.dwFObjFlags |= FOB_TRANS_MASK;
	RenParamsGeom.pMaterial = params.pMaterial;
	if (params.bDrawNear)
	{
		RenParamsGeom.dwFObjFlags |= FOB_NEAREST;
		//RenParamsGeom.nRenderList = EFSLIST_POSTPROCESS;
	}
	RenParamsGeom.fCustomSortOffset = (float)clamp_tpl((int)params.nDrawLast, -100, 100);

	// Trigger transparency rendering if any particles can be transparent.
	if (!Get3DEngine()->_GetRenderIntoShadowmap() && params.fAlpha.GetMinValue() < 1.f)
		RenParamsGeom.fAlpha *= 0.999f;

	float fEmissive = params.fEmissiveLighting;
	if (GetRenderer()->EF_Query( EFQ_HDRModeEnabled ))
		fEmissive *= powf( Get3DEngine()->GetHDRDynamicMultiplier(), params.fEmissiveHDRDynamic );
	RenParamsGeom.AmbientColor *= params.fDiffuseLighting + fEmissive;

#ifdef SHARED_GEOM
	if (m_pCVars->e_ParticlesDebug & AlphaBit('g'))
	{
		if (m_nInstInfosUsed == m_InstInfos.size())
			m_InstInfos.push_back();
		RenParamsGeom.pInstInfo = &m_InstInfos[m_nInstInfosUsed++];
		RenParamsGeom.pInstInfo->arrMats.resize(0);
		RenParamsGeom.pInstInfo->aabb = m_bbWorld;
	}
	else
	{
		RenParamsGeom.pInstInfo = 0;
	}
#endif

	UpdateParticles();

	// Set up shared and unique geom rendering.
	SParticleVertexContext Context( RenContext, 0.f, this );

	AUTO_READLOCK(m_Lock);

	m_Counts.EmittersRendered += 1.f;
	for (TParticleList::traverser pPart(m_Particles); pPart; ++pPart)
	{
		m_Counts.ParticlesRendered += pPart->RenderGeometry( RenParamsGeom, Context );
	}

#ifdef SHARED_GEOM
	// Render shared geom.
	if (RenParamsGeom.pInstInfo && RenParamsGeom.pInstInfo->arrMats.size())
	{
		static Matrix34 mxIdentity(IDENTITY);
		RenParamsGeom.pMatrix = &mxIdentity;
		params.pStatObj->Render(RenParamsGeom);
	}
#endif
}

void CParticle::AddLight()
{
	ParticleParams const& params = GetParams();
	CCamera const& cam = GetRenderer()->GetCamera();

	const float fFillLightIntensity = params.LightSource.fIntensity.GetValueFromMod(m_BaseMods.LightSourceIntensity, m_fRelativeAge);
	const float fFillLightRadius = params.LightSource.fRadius.GetValueFromMod(m_BaseMods.LightSourceRadius, m_fRelativeAge);
	if (fFillLightIntensity * fFillLightRadius > 0.001f
	&& m_vPos.GetSquaredDistance(cam.GetPosition()) < sqr(fFillLightRadius*GetCVars()->e_ParticlesLightsViewDistRatio)
	&& cam.IsSphereVisible_F(Sphere(m_vPos, fFillLightRadius))
  && GetCVars()->e_DynamicLights && GetCVars()->e_ParticlesLights == 1)
	{
		// Deferred light.
	  CDLight dl;
		dl.m_Origin = m_vPos;
		dl.m_fRadius = fFillLightRadius;
		dl.m_Color = params.cColor.GetValueFromBase(m_BaseColor, m_fRelativeAge) * ColorF(fFillLightIntensity, fFillLightIntensity, fFillLightIntensity, 1.f);
		// AntonK: remained from the fill lights code. Is it needed for deferred lights?
		//dl.m_Color *= powf( Get3DEngine()->GetHDRDynamicMultiplierInline(), params.LightSource.fHDRDynamic.GetValueFromMod(m_BaseMods.LightHDRDynamic, m_fRelativeAge) );
		dl.m_fHDRDynamic = params.LightSource.fHDRDynamic.GetValueFromMod(m_BaseMods.LightHDRDynamic, m_fRelativeAge);

		dl.m_Flags |= DLF_DEFERRED_LIGHT;
		Get3DEngine()->SetupLightScissors(&dl);

		GetRenderer()->EF_AddDeferredLight(dl, 1.f);
		Get3DEngine()->m_nDeferredLightsNum++;
	}
}

void CParticleContainer::RenderLights()
{
  FUNCTION_PROFILER_CONTAINER(this);

	assert(GetEnvironmentFlags() & REN_LIGHTS);

	// Must be done in a separate pass here, as cannot be done in thread.
	UpdateParticles();

	AUTO_READLOCK(m_Lock);

	for (TParticleList::traverser pPart(m_Particles); pPart; ++pPart)
	{
		pPart->AddLight();
	}
}

//////////////////////////////////////////////////////////////////////////
void GetScreenAxes( Vec3 aAxes[3], const Vec3& vPos, const SParticleRenderContext& context )
{
	aAxes[2] = (vPos - context.m_vCamPos).GetNormalized();						// Z: Forward.
	Vec3 vCamUp = Vec3(0,0,1) - context.m_vCamDir * context.m_vCamDir.z;	// Synthesize cam up from forward ^ world Z.
	aAxes[0] = (aAxes[2] ^ vCamUp).GetNormalized();										// X: Right = Z ^ cam up
	aAxes[1] = aAxes[0] ^ aAxes[2];																		// Y: Up
}

int CParticleContainer::GetMaxVertexCount() const
{
	ResourceParticleParams const& params = GetParams();
	if (GetHistorySteps() > 0)
		return (GetHistorySteps()+3) * 2;
	else if (params.eFacing == ParticleFacing_Camera)
		return 1;
	else
		return 4;
}

/*
		What a complete mess

							Data		shader	Color		UCol
		DX9:			xyzw		zyxw		bgra		bgra	*
		XENON:		wxyz						bgra		argb	*		
		DX11:			xyzw						rgba		rgba	*
		PS3:			xyzw						rgba		abgr	
*/

inline void ConvertColor( UCol& col, ColorF const& color, bool bSwapRGB )
{
	// 8-bit color.
	if (bSwapRGB)
	{
		col.bcolor[0] = float_to_ufrac8(color.b);
		col.bcolor[1] = float_to_ufrac8(color.g);
		col.bcolor[2] = float_to_ufrac8(color.r);
	}
	else
	{
		col.bcolor[0] = float_to_ufrac8(color.r);
		col.bcolor[1] = float_to_ufrac8(color.g);
		col.bcolor[2] = float_to_ufrac8(color.b);
	}
	col.bcolor[3] = float_to_ufrac8(color.a);



}

void CParticle::ComputeRenderData( SParticleRenderData& RenderData, SParticleVertexContext& Context ) const
{
	const ResourceParticleParams & params = GetParams();

	// Color and alpha.
	RenderData.cColor = params.cColor.GetValueFromBase(m_BaseColor, m_fRelativeAge);
	RenderData.cColor.a = params.fAlpha.GetValueFromBase(m_BaseAlpha, m_fRelativeAge);

	RenderData.fDistSq = m_vPos.GetSquaredDistance(Context.m_vCamPos);

	RenderData.fSize = params.fSize.GetValueFromMod(m_BaseMods.Size, m_fRelativeAge) * Context.m_fEmitterScale;
	float fObjectSize = m_pStatObj ? m_pStatObj->GetRadius() : 1.f;

	if (params.fMinPixels > 0.f)
	{
		float fMinSize = params.fMinPixels * 0.5f * sqrtf(RenderData.fDistSq) / Context.m_fAngularRes;
		float fMaxSize = params.fSize.GetMaxValue() * Context.m_fEmitterScale * fObjectSize;
		if (fMaxSize < fMinSize)
			RenderData.fSize *= fMinSize / fMaxSize;
	}
	float fVisSize = RenderData.fSize * GetBaseRadius();

	// Shrink particle when approaching visible limits.
	float fClipRadius = m_fClipDistance;
	if (Context.m_pVisEnv && fClipRadius > 0.f)
	{
		// Particles clipped against visibility areas. Shrink only.
		fClipRadius = min(fClipRadius, fVisSize);
		Sphere sphere(m_vPos, fClipRadius);
		Vec3 vNormal;
		if (params.bSoftParticle)
			vNormal.zero();
		else if (params.eFacing == ParticleFacing_Camera)
			vNormal = (Context.m_vCamPos - m_vPos).GetNormalized();
		else
			vNormal = GetNormal();
		if (Context.m_pVisEnv->ClipVisAreas(sphere, vNormal))
		{
			fClipRadius = sphere.radius;
			GetContainer().GetCounts().ParticlesClip += 1.f;
		}
	}

	if (fClipRadius < fVisSize)
	{
		if (fClipRadius <= 0.f)
			RenderData.fSize = fClipRadius = 0.f;
		else
			RenderData.fSize *= fClipRadius / fVisSize;
		fVisSize = fClipRadius;
	}

	// Area of particle in square radians.
	RenderData.fFillPix = div_min(square(RenderData.fSize * fObjectSize) * Context.m_fFillFactor, RenderData.fDistSq, Context.m_fFillMax);

	// Adjust alpha by distance fading.
	RenderData.cColor.a *= clamp_tpl(RenderData.fFillPix * Context.m_fInvMinPix - 1.f, 0.f, 1.f);

	// Fade near distance limits.
	RenderData.cColor.a *= Context.DistFunc(RenderData.fDistSq);
}

//////////////////////////////////////////////////////////////////////////
int CParticle::SetVertices( SVertexParticle aVerts[], SParticleVertexContext& Context ) const
{
	const ResourceParticleParams& params = GetParams();

	SVertexParticle& BaseVert = aVerts[0];

	SParticleRenderData RenderData;
	ComputeRenderData( RenderData, Context );

	// Track scr fill.
	RenderData.fFillPix *= params.fFillRateCost;
	Context.m_fPixelsProcessed += RenderData.fFillPix;

	// Cull oldest particles to enforce max screen fill.
	float fAdjust = clamp_tpl(2.f - Context.m_fPixelsProcessed * Context.m_fFillFade, 0.f, 1.f);
	RenderData.cColor.a *= fAdjust;

	if (RenderData.cColor.a < fMIN_RENDER_ALPHA)
		return 0;

	// Stats.
	Context.m_fPixelsRendered += RenderData.fFillPix;

	// For non-alpha blend modes, Alpha is just a redundant color reducer.
	if (params.eBlendType == ParticleBlendType_Additive || params.eBlendType == ParticleBlendType_ColorBased)
	{
		RenderData.cColor.r *= RenderData.cColor.a;
		RenderData.cColor.g *= RenderData.cColor.a;
		RenderData.cColor.b *= RenderData.cColor.a;

		// Additional early-out for near-zero color.
		if (RenderData.cColor.r + RenderData.cColor.g + RenderData.cColor.b < 2.f*fMIN_RENDER_ALPHA)
			return 0;
	}

	// 8-bit color.
	ConvertColor( BaseVert.color, RenderData.cColor, Context.m_bSwapRGB );

	// Texture info.
	BaseVert.info = Context.m_TexInfo;

	BaseVert.info.tex_z = m_nTileVariant * params.TextureTiling.nVariantCount / 256;
	if (params.TextureTiling.nAnimFramesCount > 1)
	{
		// Select tile based on particle age.
		float fAnimPos;
		if (params.TextureTiling.fAnimFramerate > 0.f)
		{
			fAnimPos = m_fAge * params.TextureTiling.fAnimFramerate / params.TextureTiling.nAnimFramesCount;
			if (params.TextureTiling.bAnimCycle)
				fAnimPos = fmod(fAnimPos, 1.f);
			else
				fAnimPos = min(fAnimPos, 0.999f);
		}
		else
			fAnimPos = min(m_fRelativeAge, 0.999f);

		fAnimPos += BaseVert.info.tex_z;

		if (params.TextureTiling.bAnimBlend)
			// Store as 8-bit fraction.
			BaseVert.info.tex_z = float_to_ufrac8(fAnimPos / params.TextureTiling.nVariantCount);
		else
			// Store as integer tile number, to prevent rounding errors.
			BaseVert.info.tex_z = uint(fAnimPos * params.TextureTiling.nAnimFramesCount);
	}

	BaseVert.xyz = m_vPos;

	Matrix33 matRot;
	Vec3 vVelPlane;

	BaseVert.yaxis.set(0.f, 1.f);
	if (params.eFacing != ParticleFacing_Camera)
	{
		// 3D oriented particles (Y facing).
		matRot = Matrix33(m_qRot);
	}
	else
	{
		// Force sprite facing camera.
		// Not using matRot, just 2D axis vectors.
		if (params.bOrientToVelocity || params.bEncodeVelocity || params.fStretch)
		{
			Vec3 aAxes[3];
			GetScreenAxes( aAxes, m_vPos, Context );
			vVelPlane( m_vVel * aAxes[0], m_vVel * aAxes[1], m_vVel * aAxes[2] );
		}
		if (params.bOrientToVelocity && vVelPlane.x != 0.f)
			BaseVert.yaxis = Vec2(vVelPlane.x, vVelPlane.y).GetNormalized();
	}

	if (m_fAngle != 0.f)
	{
		// Apply planar rotation.
		float c, s;
		sincos_tpl(m_fAngle, &s, &c);
		BaseVert.yaxis.set( BaseVert.yaxis.x * c - BaseVert.yaxis.y * s, BaseVert.yaxis.y * c + BaseVert.yaxis.x * s );
	}

	BaseVert.xaxis.x = BaseVert.yaxis.y;
	BaseVert.xaxis.y = -BaseVert.yaxis.x;

	BaseVert.xaxis *= RenderData.fSize * Context.m_vTexAspect.x;
	BaseVert.yaxis *= RenderData.fSize * Context.m_vTexAspect.y;

	// Stretch.
	float fStretch;
	if (!params.fStretch)
		fStretch = 0.f;
	else
	{
		// Disallow stretching further back than starting position.
		fStretch = params.fStretch.GetValueFromMod(m_BaseMods.Stretch, m_fRelativeAge);
		if (fStretch * (1.f - params.fStretch.fOffsetRatio) > m_fAge)
			fStretch = m_fAge / (1.f - params.fStretch.fOffsetRatio);
	}

	if ((params.bEncodeVelocity || fStretch != 0.f) && params.eFacing != ParticleFacing_Velocity)
	{
		// Project velocity dir into screen space.
		if (params.eFacing != ParticleFacing_Camera)
			vVelPlane(	matRot.m00*m_vVel.x + matRot.m10*m_vVel.y + matRot.m20*m_vVel.z,
									matRot.m02*m_vVel.x + matRot.m12*m_vVel.y + matRot.m22*m_vVel.z,
									matRot.m01*m_vVel.x + matRot.m11*m_vVel.y + matRot.m21*m_vVel.z );
		Vec2 vVelPlane2(vVelPlane.x, vVelPlane.y);

		float xy = vVelPlane2.GetLength();
		if (xy != 0.f)
		{
			Vec2 vVelNorm = vVelPlane2 / xy;
			if (params.bEncodeVelocity)
			{
				// Rotate into velocity-screen direction.
				BaseVert.xaxis.set( vVelNorm.y * BaseVert.xaxis.x + vVelNorm.x * BaseVert.xaxis.y,
														-vVelNorm.x * BaseVert.xaxis.x + vVelNorm.y * BaseVert.xaxis.y );
				BaseVert.yaxis.set( vVelNorm.y * BaseVert.yaxis.x + vVelNorm.x * BaseVert.yaxis.y,
														-vVelNorm.x * BaseVert.yaxis.x + vVelNorm.y * BaseVert.yaxis.y );
			}
			if (fStretch != 0.f)
			{
				// Stretch along speed direction.
				vVelPlane2 *= fStretch;
				BaseVert.xaxis += vVelPlane2 * ((BaseVert.xaxis*vVelNorm) / (RenderData.fSize * Context.m_vTexAspect.x));
				BaseVert.yaxis += vVelPlane2 * ((BaseVert.yaxis*vVelNorm) / (RenderData.fSize * Context.m_vTexAspect.y));
				BaseVert.xyz += m_vVel * (fStretch * params.fStretch.fOffsetRatio);
			}
		}
		if (params.bEncodeVelocity)
		{
			BaseVert.xaxis.x = xy / RenderData.fSize;
			BaseVert.xaxis.y = vVelPlane.z;
		}
	}

	if (params.eFacing == ParticleFacing_Camera)
	{
		// Solid camera-aligned particle.
		if (m_aPosHistory)
		{
			return FillTailVertBuffer( aVerts, Context, RenderData.fSize );
		}
		else
		{
			return 1;
		}
	}
	else
	{
		// Planar 3D-oriented particle.
		Vec3 vRight(	BaseVert.xaxis.x * matRot.m00 + BaseVert.xaxis.y * matRot.m02,
									BaseVert.xaxis.x * matRot.m10 + BaseVert.xaxis.y * matRot.m12,
									BaseVert.xaxis.x * matRot.m20 + BaseVert.xaxis.y * matRot.m22 );
		Vec3 vUp(			BaseVert.yaxis.x * matRot.m00 + BaseVert.yaxis.y * matRot.m02,
									BaseVert.yaxis.x * matRot.m10 + BaseVert.yaxis.y * matRot.m12,
									BaseVert.yaxis.x * matRot.m20 + BaseVert.yaxis.y * matRot.m22 );

		BaseVert.xaxis = BaseVert.yaxis = Vec2(0,0);

		aVerts[3] = aVerts[2] = aVerts[1] = aVerts[0];

		// Set texture and axis offsets for 3 additional verts
		aVerts[0].xyz += vRight; aVerts[0].xyz += vUp;

		aVerts[1].xyz -= vRight; aVerts[1].xyz += vUp;
		aVerts[1].info.tex_x = 255;

		aVerts[2].xyz += vRight; aVerts[2].xyz -= vUp;
		aVerts[2].info.tex_y = 255;

		aVerts[3].xyz -= vRight; aVerts[3].xyz -= vUp;
		aVerts[3].info.tex_x = 255;
		aVerts[3].info.tex_y = 255;

		return 4;
	}
}

static inline void SetSegment(	SVertexParticle aVerts[2], SVertexParticle const& BaseVert, ResourceParticleParams const& params,
																Vec3 const& vDir, SParticleRenderContext const& context, float ty, float ny )
{
	*aVerts = BaseVert;

	aVerts->info.tex_y = float_to_ufrac8(ty);

	Vec3 aAxes[3];
	GetScreenAxes( aAxes, BaseVert.xyz, context );

	aVerts->yaxis.x = vDir*aAxes[0];
	aVerts->yaxis.y = vDir*aAxes[1];

	aVerts->yaxis.Normalize();
	aVerts->xaxis.set( aVerts->yaxis.y, -aVerts->yaxis.x );
	aVerts->xaxis *= BaseVert.xaxis.x;
	aVerts->yaxis *= BaseVert.xaxis.y;

	// Offset center position by desired extension, minus offset shader adds for tex coord.
	aVerts->xyz += (aAxes[0] * aVerts->yaxis.x + aAxes[1] * aVerts->yaxis.y) * (ny-(1.f-ty*2.f));

	aVerts[1] = aVerts[0];
	aVerts++;

	aVerts->info.tex_x = 255;
}

inline Vec4 DirLen( Vec3 const& v )
{
	return Vec4( v.x, v.y, v.z, v.GetLengthSquared() );
}

inline Vec3 DirAvg( Vec4 const& v1, Vec4 const& v2 )
{
	return Vec3(v1.x, v1.y, v1.z) * v2.w + Vec3(v2.x, v2.y, v2.z) * v1.w;
}

int CParticle::FillTailVertBuffer( SVertexParticle aTailVerts[], SParticleVertexContext const& context, float fSize ) const
{
	const ResourceParticleParams& params = GetParams();

	int nVertCount = 0;
	float fTailLength = min(params.fTailLength.GetValueFromMod(m_BaseMods.TailLength, m_fRelativeAge), m_fAge);

	SVertexParticle BaseVert = aTailVerts[0];

	// Store sizes in xaxis.
	BaseVert.xaxis = context.m_vTexAspect * fSize;

	// Current pos expanded to half a particle, in direction of travel.
	Vec3 vDir = m_vVel;
	if (vDir.IsZero())
	{
		// No vel, extrapolate from history.
		for (int nPos = GetContainer().GetHistorySteps()-1; nPos >= 0; nPos--)
		{
			if (m_aPosHistory[nPos].IsUsed())
			{
				vDir = BaseVert.xyz - m_aPosHistory[nPos].vPos;
				if (!vDir.IsZero())
					break;
			}
		}
		if (vDir.IsZero())
			return 0;
	}
	float fLenApprox = vDir.len() * fTailLength;
	float fTexEnd = BaseVert.xaxis.y / (2.f * BaseVert.xaxis.y + fLenApprox);

	vDir.Normalize();
	vDir *= BaseVert.xaxis.y;
	Vec4 vDirCur = DirLen(vDir);

	SetSegment( aTailVerts+nVertCount, BaseVert, params, vDir, context, 0.f, 1.f );
	nVertCount += 2;

	// Center point.
	int nPos = GetContainer().GetHistorySteps()-1;
	for (; nPos >= 0; nPos--)
	{
		if (m_aPosHistory[nPos].IsUsed())
		{
			// Average direction.
			Vec4 vDirNext = DirLen(BaseVert.xyz - m_aPosHistory[nPos].vPos);
			if (vDirNext.w != 0.f)
			{
				vDir = DirAvg(vDirCur, vDirNext);
				vDirCur = vDirNext;
				break;
			}
		}
	}
	SetSegment( aTailVerts+nVertCount, BaseVert, params, vDir, context, fTexEnd, 0.f );
	nVertCount += 2;

  if (fTailLength > 0.f && m_fAge > 0.f)
	{
		// Fill with past positions.
		float fInvTail = 0.9999f / fTailLength;
		for (; nPos >= 0; nPos--)
		{
			float fAgeDelta = m_fAge - m_aPosHistory[nPos].fAge;
			BaseVert.xyz = m_aPosHistory[nPos].vPos;
			if (nPos > 0)
			{
				Vec4 vDirNext = DirLen(m_aPosHistory[nPos].vPos - m_aPosHistory[nPos-1].vPos);
				if (vDirCur.w == 0.f)
				{
					vDirCur = vDirNext;
					continue;
				}
				if (vDirNext.w != 0.f)
				{
					vDir = DirAvg(vDirCur, vDirNext);
					vDirCur = vDirNext;
				}
			}
			else
				vDir = Vec3(vDirCur.x, vDirCur.y, vDirCur.z);

			if (fAgeDelta > fTailLength)
			{
				// Interpolate between last vertices.
				if (nPos+1 < GetContainer().GetHistorySteps() && m_aPosHistory[nPos+1].IsUsed())
				{
					float fAgePrev = m_fAge - m_aPosHistory[nPos+1].fAge;
					float fT = div_min(fAgeDelta - fTailLength, fAgeDelta - fAgePrev, 1.f);
					BaseVert.xyz += (m_aPosHistory[nPos+1].vPos-BaseVert.xyz) * fT;
				}
				else
				{
					float fT = div_min(fAgeDelta - fTailLength, fAgeDelta, 1.f);
					BaseVert.xyz += (aTailVerts[0].xyz-BaseVert.xyz) * fT;
				}

				SetSegment( aTailVerts+nVertCount, BaseVert, params, vDir, context, 1.f - fTexEnd, 0.f );
				nVertCount += 2;
				break;
			}

			SetSegment( aTailVerts+nVertCount, BaseVert, params, vDir, context, fTexEnd + (1.f-2.f*fTexEnd) * fAgeDelta * fInvTail, 0.f );
			nVertCount += 2;
		}
	}

	// Half particle at tail end.
	SetSegment( aTailVerts+nVertCount, BaseVert, params, vDir, context, 1.f, -1.f );
	nVertCount += 2;

	return nVertCount;
}

void CParticleContainer::WriteVerticesDirect( const SParticleRenderContext& context, IAllocRender& alloc )
{
	ResourceParticleParams const& params = *m_pParams;
	SParticleVertexContext vcontext( context, alloc.fMaxPixels, this );
	int nMaxParticleVerts = GetMaxVertexCount();

	// Create directly in renderer.
	if (!alloc.bGeomShader)
		nMaxParticleVerts = max(nMaxParticleVerts, 4);
	int nMaxParticleInds = nMaxParticleVerts == 1 ? 0 : (nMaxParticleVerts-2)*3;

	int nParticles = m_Particles.size();
	for (TParticleList::traverser pPart(m_Particles); pPart; ++pPart)
	{
		if (alloc.aVertices.available() < nMaxParticleVerts || alloc.aIndices.available() < nMaxParticleInds)
		{
			if (nMaxParticleVerts == 4)
				alloc.SetQuadsIndices();

			alloc.Alloc(nParticles * nMaxParticleVerts, nParticles * nMaxParticleInds, 0);
			if (alloc.aVertices.available() < nMaxParticleVerts || alloc.aIndices.available() < nMaxParticleInds)
				return;
		}

		int nVerts = pPart->SetVertices( alloc.aVertices.grow(nMaxParticleVerts), vcontext );
		assert(nVerts <= nMaxParticleVerts);
		alloc.aVertices.resize(alloc.aVertices.size() - nMaxParticleVerts + nVerts);
		if (nVerts > 0)
		{
			m_Counts.ParticlesRendered += 1.f;
			if (nVerts > 4)
				// Create indices for variable-length particles.
				alloc.SetPolyIndices(nVerts);
			else if (nVerts == 1 && !alloc.bGeomShader)
				// Expand to 4 verts.
				alloc.ExpandQuadVertices();
		}
		nParticles--;
	}
	assert(nParticles == 0);

	if (nMaxParticleVerts == 4)
		alloc.SetQuadsIndices();

	alloc.fPixels += vcontext.m_fPixelsProcessed;

	alloc.Alloc( 0, 0, 0);	

	m_Counts.PixelsProcessed += vcontext.m_fPixelsProcessed;
	m_Counts.PixelsRendered += vcontext.m_fPixelsRendered;
	m_Counts.EmittersRendered += 1.f;
}

void CParticleContainer::WriteVerticesIndirect( const SParticleRenderContext &context, IAllocRender &alloc )
{	
	SParticleVertexContext vcontext( context, alloc.fMaxPixels, this );

	int nMaxParticleVerts = GetMaxVertexCount();
	alloc.Alloc(m_Particles.size() * nMaxParticleVerts, 0, nMaxParticleVerts > 4 ? m_Particles.size() : 1 );

	WriteVerticesIndirectImpl( vcontext, alloc );

	alloc.Alloc(0, 0, 0);
}

void CParticleContainer::WriteVerticesIndirectImpl( SParticleVertexContext &vcontext, SRenderVertices &alloc )
{	 
	FUNCTION_PROFILER_CONTAINER(this);
	



	char *buffer = NULL; // not used ont here to provide a buffer object to be used as parameter

	
	int nMaxParticleVerts = GetMaxVertexCount();			

	SVertexParticle* ptr = SPU_PTR_SELECT( alloc.aVertices.begin(), (SVertexParticle*)buffer );
	SPUChunkTransferToMain<4096>	VertUpTransfer( alloc.aVertices.begin(), buffer, alloc.aVertices.capacity() * sizeof(SVertexParticle), 0 );

	// Create and save vertices.
	if (nMaxParticleVerts <= 4)
		// Store common vert count as only array elem.
		alloc.aVertCounts.push_back(nMaxParticleVerts);

	for (TParticleList::traverser pPart(m_Particles); pPart; ++pPart)
	{
		// Stop if we would write over the buffer




		VertUpTransfer.TransferFinishedData( alloc.aVertices.size_mem() );

		int nCount = alloc.aVertices.size();
		alloc.aVertices.grow(nMaxParticleVerts);
		int nVerts = pPart->SetVertices( ptr + nCount, vcontext );
		alloc.aVertices.resize(nCount+nVerts);

		if (nVerts > 0)
		{
			m_Counts.ParticlesRendered += 1.f;
			if (nMaxParticleVerts > 4)
			{
				assert(nVerts <= nMaxParticleVerts);
				assert(nVerts < 256);
				alloc.aVertCounts.push_back(nVerts);
			}
			else
				assert(nVerts == nMaxParticleVerts);
		}
	}

	alloc.fPixels += vcontext.m_fPixelsProcessed;

	m_Counts.PixelsProcessed += vcontext.m_fPixelsProcessed;
	m_Counts.PixelsRendered += vcontext.m_fPixelsRendered;
	m_Counts.EmittersRendered += 1.f;
}


void CParticleContainer::ComputeVertices( const SParticleRenderContext& context, IAllocRender& alloc, bool bIsParticleThread )
{
	// Update, if not yet done.
	assert(GetEnvironmentFlags() & REN_SPRITE);
	assert(!m_pParams->pStatObj);

	UpdateParticles();
	if (m_Particles.empty())
		return;

  FUNCTION_PROFILER_CONTAINER(this);

	DEBUG_READLOCK(m_Lock);

	ResourceParticleParams const& params = *m_pParams;
	if (params.fTexAspect == 0.f)
		non_const(*m_pParams).UpdateTextureAspect();

	if (alloc.bDirect)
	{
		WriteVerticesDirect( context, alloc );	
	}
	else
	{







		{
			WriteVerticesIndirect( context, alloc );					
		}
	}
} 

float CParticleContainer::GetDistSquared( const Vec3& vPos ) const
{
	static float fInnerDistRatio = 0.25f;			// Amount to scale distance inside emitter bounds.

	AABB const& bb = GetMain().GetCurrentBBox();
	Vec3 vDist = vPos - bb.GetCenter();
	float fDist2 = vDist.GetLengthSquared();

	if (fInnerDistRatio != 1.f)
	{
		if (bb.GetVolume())
		{
			// For smoother results, treat the bounding volume as an ellipsoid rather than a box.
			Vec3 vSize = bb.GetSize() * 0.5f;
			Vec3 vUni(vDist.x/vSize.x, vDist.y/vSize.y, vDist.z/vSize.z);
			float fUni = vUni.GetLength();
			if (fUni <= 1.f)
				fDist2 *= square(fInnerDistRatio);
			else
				fDist2 *= square((fUni - 1.f + fInnerDistRatio) / fUni);
		}
	}

	return fDist2;
}

void SParticleVertexContext::Init( float fMaxContainerPixels, CParticleContainer* pContainer, int nRendererFeatures, float fFov )
{
	ResourceParticleParams const& params = pContainer->GetParams();

	m_bSwapRGB = !(nRendererFeatures & RFT_RGBA);
	m_fEmitterScale = pContainer->GetMain().GetParticleScale();
	m_fInvMinPix = 1.f / square(pContainer->GetMain().GetMinDrawPixels());
	m_fFillMax = sqr(m_fAngularRes);

	m_vTexAspect.x = m_vTexAspect.y = 1.f;
	if (params.fTexAspect < 1.f)
		m_vTexAspect.x = params.fTexAspect;
	else
		m_vTexAspect.y = 1.f / params.fTexAspect;

	m_fFillFactor = 4.f * m_vTexAspect.x * m_vTexAspect.y * m_fFillMax;
	m_fFillFade = fMaxContainerPixels > 0.f ? 2.f / fMaxContainerPixels : 0.f;

	m_fDistFuncCoefs[0] = 1.f;
	m_fDistFuncCoefs[1] = m_fDistFuncCoefs[2] = 0.f;

	// Zoom factor is angular res ratio between render and main camera.
	if (params.fCameraMaxDistance > 0.f)
	{
		float fZoom = 1.f / fFov;
		float fNear = sqr(params.fCameraMinDistance * fZoom);
		float fFar = sqr(params.fCameraMaxDistance * fZoom);
		float fBorder = (fFar-fNear) * 0.1f;
		if (fNear == 0.f)
			// No border on near side.
			fNear = -fBorder;

		/*	f(x) = (1 - ((x-C)/R)²) / s		; C = (N+F)/2, R = (F-N)/2
					= (-NF + (N+F) x - x²) / s
				f(N+B) = 1 
				s = B(F-N-B)
		*/
		float fS = 1.f / (fBorder*(fFar - fNear - fBorder));
		m_fDistFuncCoefs[0] = -fNear*fFar*fS;
		m_fDistFuncCoefs[1] = (fNear+fFar)*fS;
		m_fDistFuncCoefs[2] = -fS;
	}
	else if (params.fCameraMinDistance > 0.f)
	{
		float fZoom = 1.f / fFov;
		float fBorder = params.fCameraMinDistance  * fZoom * 0.25f;
		m_fDistFuncCoefs[1] = 1.f / fBorder; 
		m_fDistFuncCoefs[0] = -4.f;
	}
 
	// Pre-compute vertex encoding for offsets and backlighting.
	m_TexInfo.tex_x = m_TexInfo.tex_y = m_TexInfo.tex_z = 0;
	m_TexInfo.backlight = float_to_ufrac8(params.fDiffuseBacklighting);

	m_pVisEnv = &pContainer->GetMain().GetVisEnviron();
	if ((pContainer->GetCVars()->e_ParticlesDebug & AlphaBit('c')) || !m_pVisEnv->NeedVisAreaClip())
		m_pVisEnv = 0;

	m_fPixelsProcessed = m_fPixelsRendered = 0.f;
}






















































