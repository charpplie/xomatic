////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ParticleParams.h
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _PARTICLEPARAMS_H_
#define _PARTICLEPARAMS_H_ 1

#include <FinalizingSpline.h>
#include <Cry_Color.h>
#include <CryString.h>
#include <CryName.h>

string ToString(const Vec3& v);
bool FromString(Vec3& v, cstr str);

#include <CryCustomTypes.h>

BASIC_TYPE_INFO(CCryName)

AUTO_TYPE_INFO(EParticleBlendType)

enum EParticleFacing
{
	ParticleFacing_Camera,
	ParticleFacing_Free,
	ParticleFacing_Velocity,
	ParticleFacing_Horizontal,
	ParticleFacing_Water,
	ParticleFacing_Terrain,
	ParticleFacing_Decal,
};

enum EParticlePhysicsType
{
	ParticlePhysics_None,
	ParticlePhysics_SimpleCollision,
	ParticlePhysics_SimplePhysics,
	ParticlePhysics_RigidBody
};

enum EParticleForceType
{
	ParticleForce_None,
	ParticleForce_Wind,
	ParticleForce_Gravity,
	ParticleForce_Target,
};

enum ETrinary
{
	Trinary_Both,
	Trinary_If_True,
	Trinary_If_False
};

inline bool TrinaryMatch(ETrinary t1, ETrinary t2)
{
	return (t1 | t2) != 3;
}

inline bool TrinaryMatch(ETrinary t, bool b)
{
	return t != (Trinary_If_True + (int)b);
}

enum EConfigSpecBrief
{
	ConfigSpec_Low,
	ConfigSpec_Medium,
	ConfigSpec_High,
	ConfigSpec_VeryHigh,
};

// Sound related
enum ESoundControlTime
{
	SoundControlTime_EmitterLifeTime,
	SoundControlTime_EmitterExtendedLifeTime,
	SoundControlTime_EmitterPulsePeriod
};

// Pseudo-random number generation, from a key.
class CChaosKey
{
public:
	// Initialize with an int.
	explicit inline CChaosKey(uint32 uSeed)
		: m_Key(uSeed) {}

	explicit inline CChaosKey(float fSeed)
		: m_Key((uint32)(fSeed * float(0xFFFFFFFF))) {}

	CChaosKey Jumble(CChaosKey key2) const
	{
		return CChaosKey( Jumble(m_Key ^ key2.m_Key) );
	}
	CChaosKey Jumble(void const* ptr) const
	{
		return CChaosKey( Jumble(m_Key ^ (uint32)((UINT_PTR)ptr & 0xFFFFFFFF)) );
	}

	// Scale input range.
	inline float operator *(float fRange) const
	{
		return (float)m_Key / float(0xFFFFFFFF) * fRange;
	}
	inline uint32 operator *(uint32 nRange) const
	{
		return m_Key % nRange;
	}

	uint32 GetKey() const
	{
		return m_Key;
	}

	// Jumble with a range variable to produce a random value.
	template<class T>
	inline T Random(T const* pRange) const
	{
		return Jumble(CChaosKey(uint32(pRange))) * *pRange;
	}

private:
	uint32	m_Key;

	static inline uint32 Jumble(uint32 key)
	{
		key += ~rot_left(key, 15);
		key ^= rot_right(key, 10);
		key += rot_left(key, 3);
		key ^= rot_right(key, 6);
		key += ~rot_left(key, 11);
		key ^= rot_right(key, 16);
		return key;
	}

	static inline uint32 rot_left(uint32 u, int n)
	{
		return (u<<n) | (u>>(32-n));
	}
	static inline uint32 rot_right(uint32 u, int n)
	{
		return (u>>n) | (u<<(32-n));
	}
};

//
// Custom behavior for param types.
//

template<class T>
struct TStorageTraits
{
	typedef float TValue;
	typedef UnitFloat8 TMod;
	typedef UnitFloat8 TRandom;
};

//////////////////////////////////////////////////////////////////////////
//
// Compressed Vec3
//

inline string ToString(const Vec3& v)
	{ return TypeInfo(&v).ToString(&v); }
inline bool FromString(Vec3& v, cstr str)
	{ return TypeInfo(&v).FromString(&v, str); }

#if 1
	// Cannot afford performance penalty in ResourceParticleParams::GetStaticBounds right now
	struct Vec3_Zero: Vec3
	{
		Vec3_Zero() : Vec3(ZERO) {}
		Vec3_Zero(const Vec3& v) : Vec3(v) {}
	};
	typedef Vec3_Zero Vec3S16;
	typedef Vec3_Zero Vec3U16;
#else
	typedef Vec3_tpl<SFloat16> Vec3S16;
	typedef Vec3_tpl<UFloat16> Vec3U16;

	template<class T, int N>
	inline Vec3 BiRandom( Vec3_tpl< TFloat<T,N> > const& sv )
	{
		return BiRandom( Vec3(sv) );
	}

	template<class T, int N>
	inline const CTypeInfo& TypeInfo( Vec3_tpl< TFloat<T,N> > const* )
	{
		// Override TypeInfo to provide Vec3 conversion.
		static TTypeInfo< Vec3, Vec3_tpl< TFloat<T,N> > > Info("Vec3<TFloat>");
		return Info;
	}
#endif

//////////////////////////////////////////////////////////////////////////
//
// Color specialisations.


// Comparison functions for ColorF, used in splines.

inline ColorF min( const ColorF& c, const ColorF& d )
	{ return ColorF( min(c.r, d.r), min(c.g, d.g), min(c.b, d.b), min(c.a, d.a) ); }
inline ColorF max( const ColorF& c, const ColorF& d )
	{ return ColorF( max(c.r, d.r), max(c.g, d.g), max(c.b, d.b), max(c.a, d.a) ); }
inline ColorF minmag( const ColorF& c, const ColorF& d )
	{ return ColorF( minmag(c.r, d.r), minmag(c.g, d.g), minmag(c.b, d.b), minmag(c.a, d.a) ); }

class RandomColor
{
public:
	inline RandomColor(float f = 0.f, bool bRandomHue = false)
		: m_VarRandom(f), m_bRandomHue(bRandomHue)
	{}
	inline operator bool() const
		{ return !!m_VarRandom; }
	inline bool operator!() const
		{ return !m_VarRandom; }

	ColorF GetRandom() const
	{
		if (m_bRandomHue)
		{
			ColorB clr(cry_rand32());
			float fScale = m_VarRandom / 255.f;
			return ColorF(clr.r * fScale, clr.g * fScale, clr.b * fScale);
		}
		else
		{
			return ColorF(Random(m_VarRandom));
		}
	}

	AUTO_STRUCT_INFO

protected:
	UnitFloat8	m_VarRandom;
	bool	m_bRandomHue;
};

inline ColorF Random(RandomColor const& rc)
{
	return rc.GetRandom();
}

struct Color3B
{
	TFixed<uint8,1>		r, g, b;
	inline Color3B( float f = 0.f )
	{
		r = g = b = f;
	}
	inline Color3B( float r_, float g_, float b_ )
		: r(r_), g(g_), b(b_)
	{}
	inline Color3B( const ColorF& c )
		: r(c.r), g(c.g), b(c.b)
	{}
	inline operator ColorF() const
		{ return ColorF(r, g, b, 1.f); }
	inline bool operator ==(const Color3B& x) const
		{ return r == x.r && g == x.g && b == x.b; }

	AUTO_STRUCT_INFO
};

template<>
struct TStorageTraits<ColorF>
{
	typedef ColorF TValue;
	typedef Color3B	TMod;
	typedef RandomColor	TRandom;
};

template<>
struct TStorageTraits<Color3B>: TStorageTraits<ColorF>
{
};


//
// Spline implementation class.
//

#define ParticleSpline LookupTableSpline

template<class S>
class TCurveSpline: public spline::CBaseSplineInterpolator< 
	typename TStorageTraits<S>::TValue,
	spline::ParticleSpline<
		S, spline::TSplineSlopes< 
			typename TStorageTraits<S>::TValue, 
			spline::SplineKey<typename TStorageTraits<S>::TValue>, 
			true 
		> 
	>
>
{
public:
	typedef typename TStorageTraits<S>::TValue T;

	// Implement serialisation for particles.
	string ToString( int flags = 0 ) const;
	void FromString( const char* str, int flags = 0 );
	virtual void SerializeSpline( XmlNodeRef &node, bool bLoading );
};

template<class S>
class TCurve
{
public:
	typedef typename TStorageTraits<S>::TValue T;

	// Spline interface.
	ISplineInterpolator* GetSpline(bool bCreate)
	{
		if (!m_pSpline && bCreate)
			m_pSpline = new TSpline;
		return m_pSpline;
	}

	//
	// Read access
	//

	// Get the value at any key point from 0..1.
	inline T GetValue(float fTime) const
	{
		T Val(1.f);
		if (m_pSpline)
			m_pSpline->fast_interpolate( fTime, Val );
		return Val;
	}
	inline void ModValue(T& Val, float fTime) const
	{
		if (m_pSpline)
		{
			T CVal(1.f);
			m_pSpline->fast_interpolate( fTime, CVal );
			Val *= CVal;
		}
	}

	T GetMinValue() const
	{
		T val = 1.f;
		if (m_pSpline)
			m_pSpline->min_value(val);
		return val;
	}
	T GetMaxValue() const
	{
		T val = 1.f;
		if (m_pSpline)
			m_pSpline->max_value(val);
		return val;
	}
	inline bool IsIdentity() const
	{
		return !m_pSpline;
	}

	//
	// Write access.
	//

	TCurve()
	{}

	TCurve( const TCurve& in )
		: m_pSpline(in.m_pSpline)
	{
		CleanUp();
	}

	TCurve& operator =( const TCurve& in )
	{
		m_pSpline = in.m_pSpline;
		CleanUp();
		return *this;
	}

	void Clear()
	{
		m_pSpline = NULL;
	}

	// Serialisation.
	string ToString(int flags) const
	{
		if (!m_pSpline)
			return string();
		return m_pSpline->ToString(flags);
	}

	void FromString( const char* str, int flags = 0 )
	{
		if (!*str)
			m_pSpline = NULL;
		else
		{
			m_pSpline = new TSpline;
			m_pSpline->FromString( str, flags );
			CleanUp();
		}
	}

	void GetMemoryUsage(ICrySizer* pSizer) const
	{
		if (m_pSpline)
		{
			pSizer->Add(*m_pSpline);
			m_pSpline->GetMemoryUsage(pSizer);
		}
	}

	// TypeInfo explicitly implemented in ParticleParamsTypeInfo.cpp.
	CUSTOM_STRUCT_INFO(CCustomInfo)

protected:

	typedef TCurveSpline<S> TSpline;
	deep_ptr<TSpline> m_pSpline;

	void CleanUp()
	{
		if (GetMinValue() == T(1.f))
			m_pSpline = NULL;
	}

	struct CCustomInfo: CTypeInfo
	{
		CCustomInfo()
			: CTypeInfo("TCurve<>", sizeof(TCurve<S>))
		{}
		virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const
		{
			return ((const TCurve<S>*)data)->ToString(flags);
		}
		virtual bool FromString(void* data, cstr str, int flags = 0) const
		{
			((TCurve<S>*)data)->FromString(str, flags);
			return true;
		}
		virtual void GetMemoryUsage(ICrySizer* pSizer, const void* data) const
		{
		#if !defined(PS3)
			((TCurve<S>*)data)->GetMemoryUsage(pSizer);
		#endif
		}
	};
};

//
// Composite parameter types, incorporating base value, randomness, and lifetime curves.
//

template<class S>
struct TVarParam
{
	typedef typename TStorageTraits<S>::TValue T;
	using_type(TStorageTraits<S>, TRandom)

	// Construction.
	TVarParam()
		: m_Base(), m_VarRandom(0)
	{
	}

	TVarParam(const T& tBase, const TRandom& tRan = TRandom(0))
		: m_Base(tBase), m_VarRandom(tRan)
	{
	}

	inline void Set(const T& tBase, const TRandom& tRan = TRandom(0))
	{ 
		m_Base = tBase;
		m_VarRandom = tRan;
	}

	//
	// Value extraction.
	//

	inline operator bool() const
	{
		return !!m_Base;
	}
	inline bool operator !() const
	{
		return !m_Base;
	}

	// Uses a chaos key for randomisation in every dimension of T.
	inline T GetVarValue(CChaosKey keyRan) const
	{
		if (!m_VarRandom)
			return m_Base;
		else
			return m_Base - keyRan.Jumble(this) * m_VarRandom * m_Base;
	}

	inline T GetMaxValue() const
	{
		return m_Base;
	}

	T GetMinValue() const
	{
		return m_Base - m_Base * float(m_VarRandom);
	}

	AUTO_STRUCT_INFO

protected:
	// Base value.
	S					m_Base;									

	// Variations, all multiplicative.
	TRandom			m_VarRandom;						// $<Inline=1>
};


template<class S>
struct TVarEParam: TVarParam<S>
{
	typedef typename TStorageTraits<S>::TValue T;
	using_type(TStorageTraits<S>, TRandom)
	using_type(TStorageTraits<S>, TMod)

	// Construction.
	TVarEParam()
	{
	}

	TVarEParam(const T& tBase)
		: TVarParam<S>(tBase)
	{
	}

	//
	// Value extraction.
	//

	inline T GetVarMod(float fEStrength) const
	{
		T val = T(1.f);
		if (m_VarRandom)
			val -= val * Random(m_VarRandom);
		m_VarEmitterStrength.ModValue(val, fEStrength);
		return val;
	}

	inline T GetVarValue(float fEStrength) const
	{
		return GetVarMod(fEStrength) * m_Base;
	}

	inline T GetVarValue(float fMinMax, float fEStrength) const
	{
		T val = m_Base;
		val -= val * m_VarRandom * (1.f-fMinMax);
		m_VarEmitterStrength.ModValue(val, fEStrength);
		return val;
	}

	inline T GetVarValue(CChaosKey keyRan, float fEStrength, T Abs = T(0.f)) const
	{
		T val = m_Base;
		if (!!m_VarRandom)
		{
			if (Abs != T(0.f))
				val += keyRan.Jumble(this) * m_VarRandom * Abs;
			else
				val -= keyRan.Jumble(this) * m_VarRandom * val;
		}
		m_VarEmitterStrength.ModValue(val, fEStrength);
		return val;
	}

	T GetMinValue() const
	{
		return TVarParam<S>::GetMinValue()
			* m_VarEmitterStrength.GetMinValue();
	}

	bool IsConstant() const
	{
		return m_VarEmitterStrength.IsIdentity();
	}

	AUTO_STRUCT_INFO

protected:
	TCurve<TMod>		m_VarEmitterStrength;

	// Dependent name nonsense.
	using TVarParam<S>::m_Base;
	using TVarParam<S>::m_VarRandom;
};

template<class S>
struct TVarEPParam: TVarEParam<S>
{
	typedef typename TStorageTraits<S>::TValue T;
	using_type(TStorageTraits<S>, TRandom)
	using_type(TStorageTraits<S>, TMod)

	// Construction.
	TVarEPParam()
	{}

	TVarEPParam(const T& tBase)
		: TVarEParam<S>(tBase)
	{}

	//
	// Value extraction.
	//

	inline T GetValueFromBase( T val, float fParticleAge ) const
	{
		m_VarParticleLife.ModValue(val, fParticleAge);
		return val;
	}

	inline T GetValueFromMod( T val, float fParticleAge ) const
	{
		m_VarParticleLife.ModValue(val, fParticleAge);
		return m_Base * val;
	}

	T GetMinValue() const
	{
		return TVarEParam<S>::GetMinValue()
			* m_VarParticleLife.GetMinValue();
	}

	bool IsConstant() const
	{
		return m_VarParticleLife.IsIdentity();
	}

	AUTO_STRUCT_INFO

protected:
	TCurve<TMod>		m_VarParticleLife;

	// Dependent name nonsense.
	using TVarEParam<S>::m_Base;
	using TVarEParam<S>::m_VarRandom;
	using TVarEParam<S>::m_VarEmitterStrength;
};


struct STextureTiling
{
	uint16	nTilesX, nTilesY;	// $<Min=1> $<Max=256> Number of tiles texture is split into 
	uint16	nFirstTile;				// First (or only) tile to use.
	uint16	nVariantCount;		// $<Min=1> $<Max=256> Number of randomly selectable tiles; 0 or 1 if no variation.
	uint16	nAnimFramesCount;	// $<Min=1> $<Max=256> Number of tiles (frames) of animation; 0 or 1 if no animation.
	bool	bAnimCycle;					// Cycle animation, otherwise stop on last frame.
	bool	bAnimBlend;					// Blend between frames, at slow framerates (< 30).
	UFloat16	fAnimFramerate;	// $<SoftMax=60> Tex framerate; 0 = 1 cycle / particle life.

	STextureTiling()
	{
		nFirstTile = 0;
		fAnimFramerate = 0.f;
		bAnimCycle = false;
		bAnimBlend = false;
		nTilesX = nTilesY = nVariantCount = nAnimFramesCount = 1;
	}

	int GetTileCount() const
	{
		return nTilesX * nTilesY - nFirstTile;
	}

	int GetFrameCount() const
	{
		return nVariantCount * nAnimFramesCount;
	}

	void Correct()
	{
		nTilesX = max(nTilesX, 1U);
		nTilesY = max(nTilesY, 1U);
		nFirstTile = min(nFirstTile, nTilesX*nTilesY-1);
		nAnimFramesCount = max(min(nAnimFramesCount, GetTileCount()), 1U);
		nVariantCount = max(min(nVariantCount, GetTileCount() / nAnimFramesCount), 1U);
	}

	AUTO_STRUCT_INFO
};



//! Particle system parameters
struct ParticleParams
{
	// Emitter params.
	bool bEnabled;										// Set false to disable this effect.
	bool bContinuous;									// Emit particles gradually until nCount reached

	TVarEParam<UFloat16> fCount;					// $<SoftMax=1000> Number of particles in system at once.
	TVarParam<UFloat16> fEmitterLifeTime;	// Min lifetime of the emitter, 0 if infinite. Always emits at least nCount particles.
	TVarParam<SFloat16> fSpawnDelay;			// $<SoftMin=0> Delay the actual spawn time by this value	
	TVarParam<SFloat16> fPulsePeriod;			// Time between auto-restarts of finite systems. 0 if just once, or continuous.

	// Particle timing
	TVarEParam<UFloat16> fParticleLifeTime;	// Lifetime of particle
	bool bRemainWhileVisible;							// Particles will only die when not rendered (by any viewport).

	// Spawning
	bool bSecondGeneration;						// $<Group="Spawning"> Emitters tied to each parent particle.
	bool bSpawnOnParentCollision;			// (Second Gen only) Spawns when parent particle collides.
	TSmall<EGeomType> eAttachType;		// Where to emit from attached geometry.
	TSmall<EGeomForm> eAttachForm;		// How to emit from attached geometry.
	Vec3S16 vPositionOffset;						// Spawn Position offset from the effect spawn position
	Vec3U16 vRandomOffset;							// Random offset of the particle relative position to the spawn position
	UFloat16 fPosRandomOffset;						// Radial random offset.

	// Angles
	TVarEParam<UFloat16>	fFocusAngle;		// $<Group="Angles"> $<Max=180> Angle to vary focus from default (Y axis), for variation.
	TVarEParam<SFloat16>	fFocusAzimuth;	// $<SoftMax=360> Angle to rotate focus about default, for variation. 0 = Z axis.
	bool bFocusGravityDir;								// Uses negative gravity dir, rather than emitter Y, as focus dir.
	TVarEParam<UFloat16> fEmitAngle;			// $<Max=180> Angle from focus dir (emitter Y), in degrees. RandomVar determines min angle.
	
	// Sound
	CCryName sSound;											// $<Group="Sound"> Name of the sound file
	TVarEParam<UnitFloat8> fSoundFXParam;	// Custom real-time sound modulation parameter.
	TSmall<ESoundControlTime> eSoundControlTime;		// The sound control time type

	// Appearance
	TSmall<EParticleFacing> eFacing;	// $<Group="Appearance"> The basic particle shape type.
	bool bOrientToVelocity;						// Rotate particle X axis in velocity direction.
	TSmall<EParticleBlendType> eBlendType;	// Blend rendering type.
	CCryName sTexture;								// Texture for particle.
	bool bTextureUnstreamable;				// True if the texture should not be streamed but instead always loaded.
	STextureTiling TextureTiling;			// Animation etc.
	CCryName sMaterial;								// Material (overrides texture).
	CCryName sGeometry;								// Geometry for 3D particles.
	bool bGeometryInPieces;						// Spawn geometry pieces separately.
	bool bGeometryUnstreamable;				// True if the geometry should not be streamed but instead always loaded.
	bool bSoftParticle;								// Enable soft particle processing in the shader.
  bool bOceanParticle;							// Enable soft particle processing in the shader.
	TVarEPParam<UnitFloat8> fAlpha;		// Alpha value (fade parameters).
	TVarEPParam<Color3B> cColor;			// Color modulation.

	// Lighting
	UFloat16 fDiffuseLighting;				// $<Group="Lighting"> $<SoftMax=1> Multiplier for particle dynamic lighting.
	UnitFloat8 fDiffuseBacklighting;	// Fraction of diffuse lighting applied in all directions.
	UFloat16 fEmissiveLighting;				// $<SoftMax=1> Multiplier for particle emissive lighting.
	SFloat16 fEmissiveHDRDynamic;			// $<SoftMin=-2> $<SoftMax=2> Power to apply to engine HDR multiplier for emissive lighting in HDR.
	bool bReceiveShadows;							// Shadows will cast on these particles.
	bool bCastShadows;								// Particles will cast shadows (currently only geom particles).

	struct SLightSource
	{
		TVarEPParam<UFloat16> fRadius;			// $<SoftMax=8>
		TVarEPParam<UFloat16> fIntensity;		// $<SoftMax=16>
		TVarEPParam<SFloat16> fHDRDynamic;	// $<Min=-2> $<SoftMax=2>
		AUTO_STRUCT_INFO
	} LightSource;

	// Size
	TVarEPParam<UFloat16> fSize;					// $<Group="Size"> $<SoftMax=10> Particle size.
	struct SStretch: TVarEPParam<UFloat16>
	{
		SFloat16 fOffsetRatio;							// $<SoftMin=-1> $<SoftMax=1> Move particle center this fraction in direction of stretch.
		AUTO_STRUCT_INFO
	} fStretch;														// $<SoftMax=10> Stretch particle into moving direction
	struct STailLength: TVarEPParam<UFloat16>
	{
		uint8 nTailSteps;										// $<SoftMax=16> Number of tail segments
		AUTO_STRUCT_INFO
	} fTailLength;												// $<SoftMax=10> Length of tail in seconds
	UFloat16 fMinPixels;									// $<SoftMax=10> Augment true size with this many pixels.

	// Movement
	TVarEParam<SFloat16> fSpeed;					// $<Group="Movement"> Initial speed of a particle.
	SFloat16 fInheritVelocity;						// $<SoftMin=0> $<SoftMax=1> Fraction of emitter's velocity to inherit.
	TVarEPParam<UFloat16> fAirResistance;	// $<SoftMax=10> Air drag value, in inverse seconds.
	UFloat16 fRotationalDragScale;				// $<SoftMax=1> Multipler to AirResistance, for rotational motion.
	TVarEPParam<SFloat16> fGravityScale;	// $<SoftMin=0> $<SoftMax=1> Multiplier for world gravity.
	Vec3S16 vAcceleration;										// Specific world-space acceleration vector.
	TVarEPParam<UFloat16> fTurbulence3DSpeed;	// $<SoftMax=10> 3D random turbulence force
	TVarEPParam<UFloat16> fTurbulenceSize;		// $<SoftMax=10> Radius of turbulence
	TVarEPParam<SFloat16> fTurbulenceSpeed;		// $<SoftMin=-360> $<SoftMax=360> Speed of rotation

	struct STargetAttraction					// Customise movement toward ParticleTargets.
	{
		bool bIgnore;
		bool bExtendSpeed;									// Extend particle speed as necessary to reach target in normal lifetime.
		bool bShrink;												// Shrink particle as it approaches target.
		bool bOrbit;
		TVarEPParam<SFloat16> fOrbitDistance;	// If > 0, orbits at specified distance rather than disappearing.

		AUTO_STRUCT_INFO
	} TargetAttraction;

	// Rotation
	Vec3S16 vInitAngles;						// $<Group="Rotation"> $<SoftMin=-180> $<SoftMax=180> Initial rotation in symmetric angles (degrees).
	Vec3U16 vRandomAngles;					// $<Max=180> Bidirectional random angle variation.
	Vec3S16 vRotationRate;					// $<SoftMin=-360> $<SoftMax=360> Rotation speed (degree/sec).
	Vec3S16 vRandomRotationRate;		// $<SoftMax=360> Random variation.

	// Physics
	TSmall<EParticlePhysicsType> ePhysicsType;	// $<Group="Physics"> 
	bool bCollideTerrain;							// Collides with terrain (if Physics <> none)
	bool bCollideStaticObjects;				// Collides with static physics objects (if Physics <> none)
	bool bCollideDynamicObjects;			// Collides with dynamic physics objects (if Physics <> none)
	CCryName sSurfaceType;						// $<Select="Surface"> Surface type for physicalised particles.
	SFloat16 fBounciness;							// $<SoftMin=0> $<SoftMax=1> Elasticity: 0 = no bounce, 1 = full bounce, <0 = die.
	UFloat16 fDynamicFriction;				// $<SoftMax=10> Sliding drag value, in inverse seconds.
	UFloat16 fThickness;							// $<SoftMax=1> Lying thickness ratio - for physicalized particles only
	UFloat16 fDensity;								// $<SoftMax=2000> Mass density for physicslized particles.
	uint8 nMaxCollisionEvents;				// Max # collision events per particle (0 = no limit).
	TSmall<EParticleForceType> eForceGeneration;	// Generate physical forces if set.

	// Visibility
	bool bBindEmitterToCamera;		// $<Group="Visibility"> 
	bool bMoveRelEmitter;					// Particle motion is in emitter space.
	bool bSpaceLoop;							// Lock particles in box around emitter position, use vSpaceLoopBoxSize to set box size
	UFloat16 fCameraMaxDistance;		// $<SoftMax=100> Max distance from camera to render particles.
	UFloat16 fCameraMinDistance;		// $<SoftMax=100> Min distance from camera to render particles.
	UFloat16 fViewDistanceAdjust;		// $<SoftMax=1> Multiplier to automatic distance fade-out.
	int8 nDrawLast;									// $<SoftMin=-16> $<SoftMax=16> Adjust draw order within effect group.
	TSmall<ETrinary> tVisibleIndoors;			// Whether visible indoors/outdoors/both.
	TSmall<ETrinary> tVisibleUnderwater;	// Whether visible under/above water / both.

	// Advanced
	UFloat16 fFillRateCost;					// $<Group="Advanced"> $<SoftMax=10> Adjustment to max screen fill allowed per emitter.
	UFloat16 fMotionBlurScale;			// $<SoftMax=2> Multiplier to motion blur.
	bool bNotAffectedByFog;
	bool bDrawNear;								// Render particle in near space (weapon)
	bool bNoOffset;								// Disable centering of static objects
	bool bEncodeVelocity;					// Orient and encode velocity direction, for special shaders.

	// Configurations
	TSmall<EConfigSpecBrief> eConfigMin;	// $<Group="Configuration"> 
	TSmall<EConfigSpecBrief> eConfigMax;
	TSmall<ETrinary> tDX11;
	TSmall<ETrinary> tGPUComputation;

	ParticleParams() 
	{
		memset(this, 0, sizeof(*this));
		new(this) ParticleParams(true);
	}

	AUTO_STRUCT_INFO

protected:

	ParticleParams(bool) :
		bEnabled(true),
		fCount(1.f),
		fParticleLifeTime(1.f),
		fSize(1.f),
		fSpeed(1.f),
		cColor(1.f),
		fAlpha(1.f),
		fDiffuseLighting(1.f),
		fViewDistanceAdjust(1.f),
		fFillRateCost(1.f),
		fRotationalDragScale(1.f),
		fDensity(1000.f),
		fThickness(1.f),
		fMotionBlurScale(1.f),
		fSoundFXParam(1.f),
		eConfigMax(ConfigSpec_VeryHigh),
		eSoundControlTime(SoundControlTime_EmitterLifeTime)
	{}
};

#endif
