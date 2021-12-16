/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 09:05:2005   14:32 : Created by Carsten Wenzel

*************************************************************************/

#include "StdAfx.h"
#include "SkyLightManager.h"
#include "SkyLightNishita.h"
#include <CRESky.h>


const int c_skyTextureWidth( 128 );
const int c_skyTextureHeight( 64 );
const int c_skyDomeTextureSize( 128 * 64 );

const int c_skyTextureWidthBy8( 16 );
const int c_skyTextureWidthBy4Log( 5 ); // = log2(128/4)
const int c_skyTextureHeightBy2Log( 5 ); // = log2(64/2)








CSkyLightManager::CSkyLightManager()
: m_pSkyLightNishita( new CSkyLightNishita )
, m_pSkyDomeMesh( 0 )
, m_curSkyDomeCondition()
, m_updatingSkyDomeCondition()
, m_numSkyDomeColorsComputed( c_skyDomeTextureSize )
, m_curBackBuffer( 0 )
, m_lastFrameID( 0 )
, m_curSkyHemiColor()
, m_curHazeColor( 0.0f, 0.0f, 0.0f )
, m_curHazeColorMieNoPremul( 0.0f, 0.0f, 0.0f )
, m_curHazeColorRayleighNoPremul( 0.0f, 0.0f, 0.0f )
, m_skyHemiColorAccum()
, m_hazeColorAccum( 0.0f, 0.0f, 0.0f )
, m_hazeColorMieNoPremulAccum( 0.0f, 0.0f, 0.0f )
, m_hazeColorRayleighNoPremulAccum( 0.0f, 0.0f, 0.0f )
, m_renderParams()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "SkyLightManager");

	InitSkyDomeMesh();

	m_updateRequested[0] = m_updateRequested[1] = 0;

	// init textures with default data
	m_skyDomeTextureDataMie[ 0 ].resize( c_skyDomeTextureSize );
	m_skyDomeTextureDataMie[ 1 ].resize( c_skyDomeTextureSize );
	m_skyDomeTextureDataRayleigh[ 0 ].resize( c_skyDomeTextureSize );
	m_skyDomeTextureDataRayleigh[ 1 ].resize( c_skyDomeTextureSize );

	// init time stamps
	m_skyDomeTextureTimeStamp[ 0 ] = C3DEngine::GetMainFrameID();
	m_skyDomeTextureTimeStamp[ 1 ] = C3DEngine::GetMainFrameID();

	// init sky hemisphere colors and accumulators
	memset(m_curSkyHemiColor, 0, sizeof(m_curSkyHemiColor));
	memset(m_skyHemiColorAccum, 0, sizeof(m_skyHemiColorAccum));

	// set default render parameters
	UpdateRenderParams();
}


CSkyLightManager::~CSkyLightManager()
{
	SAFE_RELEASE(m_pSkyDomeMesh);
	delete m_pSkyLightNishita;
}

inline static void Sync()
{







}

void CSkyLightManager::PushUpdateParams()
{
	//pushes the update parameters, explicite call since engine requests asynchronously
	memcpy(&m_reqSkyDomeCondition[0], &m_reqSkyDomeCondition[1], sizeof(SSkyDomeCondition));
	m_updateRequested[0] = m_updateRequested[1];
	m_updateRequested[1] = 0;
}

void CSkyLightManager::SetSkyDomeCondition( const SSkyDomeCondition& skyDomeCondition )
{
	m_reqSkyDomeCondition[1] = skyDomeCondition;
	m_updateRequested[1] = 1;
}


void CSkyLightManager::FullUpdate()
{
	Sync();
	PushUpdateParams();










	UpdateInternal( C3DEngine::GetMainFrameID(), c_skyDomeTextureSize, 1 );
	Sync();
	UpdateRenderParams();			// update render params
}

void CSkyLightManager::IncrementalUpdate( f32 updateRatioPerFrame )
{
	Sync();

	FUNCTION_PROFILER_3DENGINE;
	
	// get current ID of "main" frame (no recursive rendering), 
	// incremental update should only be processed once per frame
	if( m_lastFrameID != C3DEngine::GetMainFrameID() )
	{
		int32 numUpdate( (int32) ( (f32) c_skyDomeTextureSize * updateRatioPerFrame / 100.0f + 0.5f ) );
		numUpdate = clamp_tpl( numUpdate, 1, c_skyDomeTextureSize );
		if(m_needRenderParamUpdate)
			UpdateRenderParams();			// update render params

		PushUpdateParams();













		UpdateInternal( C3DEngine::GetMainFrameID(), numUpdate );
	}
}
#if !defined(CRCYG_CM)
SPU_ENTRY(SkyUpdate)
#endif 
void CSkyLightManager::UpdateInternal( int32 newFrameID, int32 numUpdates, int callerIsFullUpdate )
{
  FUNCTION_PROFILER_3DENGINE;

	__cache_range_write_async(m_pSkyLightNishita, (char*)m_pSkyLightNishita + sizeof(CSkyLightNishita));
	// update sky dome if requested -- requires last update request to be fully processed!
	int procUpdate = callerIsFullUpdate;
	procUpdate |= (int)IsSkyDomeUpdateFinished();
	procUpdate &= m_updateRequested[0];
	if( procUpdate ) 
	{
		// set sky dome settings
		m_updatingSkyDomeCondition = m_reqSkyDomeCondition[0];
		m_pSkyLightNishita->SetSunDirection( m_updatingSkyDomeCondition.m_sunDirection );
		m_pSkyLightNishita->SetRGBWaveLengths( m_updatingSkyDomeCondition.m_rgbWaveLengths );
		m_pSkyLightNishita->SetAtmosphericConditions( m_updatingSkyDomeCondition.m_sunIntensity, 
			1e-4f * m_updatingSkyDomeCondition.m_Km, 1e-4f * m_updatingSkyDomeCondition.m_Kr, m_updatingSkyDomeCondition.m_g );	// scale mie and rayleigh scattering for more convenient editing in time of day dialog

		// update request has been accepted
		m_updateRequested[0] = 0; 
		m_numSkyDomeColorsComputed = 0;

		// reset sky & haze color accumulator
		m_hazeColorAccum = Vec3( 0.0f, 0.0f, 0.0f );
		m_hazeColorMieNoPremulAccum = Vec3( 0.0f, 0.0f, 0.0f );
		m_hazeColorRayleighNoPremulAccum = Vec3( 0.0f, 0.0f, 0.0f );
		memset(m_skyHemiColorAccum, 0, sizeof(m_skyHemiColorAccum));
	}

	// any work to do?
	if( false == IsSkyDomeUpdateFinished() )
	{
		if( numUpdates <= 0 )
		{
			// do a full update
			numUpdates = c_skyDomeTextureSize;
		}

		// find minimally required work load for this incremental update
		numUpdates = min( c_skyDomeTextureSize - m_numSkyDomeColorsComputed, numUpdates );

		// perform color computations
		SkyDomeTextureData& skyDomeTextureDataMie( m_skyDomeTextureDataMie[ GetBackBuffer() ] );
		SkyDomeTextureData& skyDomeTextureDataRayleigh( m_skyDomeTextureDataRayleigh[ GetBackBuffer() ] );
		__cache_range_write_async(&skyDomeTextureDataMie[0], &skyDomeTextureDataMie[c_skyDomeTextureSize]); 
		__cache_range_write_async(&skyDomeTextureDataRayleigh[0], &skyDomeTextureDataRayleigh[c_skyDomeTextureSize]);
		//huge optimization here for SPUs: optical lut is small but accessed the most, cache onto stack





		int32 numSkyDomeColorsComputed( m_numSkyDomeColorsComputed );
		for( ; numUpdates > 0; --numUpdates, ++numSkyDomeColorsComputed )
		{
			// calc latitude/longitude
			int lon( numSkyDomeColorsComputed / c_skyTextureWidth );
			int lat( numSkyDomeColorsComputed % c_skyTextureWidth );

			float lonArc( DEG2RAD( (float) lon * 90.0f / (float) c_skyTextureHeight ) );
			float latArc( DEG2RAD( (float) lat * 360.0f / (float) c_skyTextureWidth ) );

			float sinLon(0); float cosLon(0);
			sincos_tpl(lonArc, &sinLon, &cosLon);
			float sinLat(0); float cosLat(0);
			sincos_tpl(latArc, &sinLat, &cosLat);

			// calc sky direction for given update latitude/longitude (hemisphere)
			Vec3 skyDir( sinLon * cosLat, sinLon * sinLat, cosLon );

			// compute color
			//Vec3 skyColAtDir( 0.0, 0.0, 0.0 );
			Vec3 skyColAtDirMieNoPremul( 0.0, 0.0, 0.0 ); 
			Vec3 skyColAtDirRayleighNoPremul( 0.0, 0.0, 0.0 );
			Vec3 skyColAtDirRayleigh( 0.0, 0.0, 0.0 );



			m_pSkyLightNishita->ComputeSkyColor( skyDir, 0 , &skyColAtDirMieNoPremul, &skyColAtDirRayleighNoPremul, &skyColAtDirRayleigh );

			// store color in texture
			skyDomeTextureDataMie[ numSkyDomeColorsComputed ] = SFloatABGR32( skyColAtDirMieNoPremul.x, skyColAtDirMieNoPremul.y, skyColAtDirMieNoPremul.z, 1.0f );
			skyDomeTextureDataRayleigh[ numSkyDomeColorsComputed ] = SFloatABGR32( skyColAtDirRayleighNoPremul.x, skyColAtDirRayleighNoPremul.y, skyColAtDirRayleighNoPremul.z, 1.0f );

			// update haze color accum (accumulate second last sample row)
			if( lon == c_skyTextureHeight - 2 )
			{
				m_hazeColorAccum += skyColAtDirRayleigh;
				m_hazeColorMieNoPremulAccum += skyColAtDirMieNoPremul;
				m_hazeColorRayleighNoPremulAccum += skyColAtDirRayleighNoPremul;
			}

			// update sky hemisphere color accumulator
			int y(lon >> c_skyTextureHeightBy2Log); 		
			int x(((lat+c_skyTextureWidthBy8) & (c_skyTextureWidth-1)) >> c_skyTextureWidthBy4Log); 
			int skyHemiColAccumIdx(x*y + y);
			assert(((unsigned int)skyHemiColAccumIdx) < 5);
			m_skyHemiColorAccum[skyHemiColAccumIdx] += skyColAtDirRayleigh;
		}

		m_numSkyDomeColorsComputed = numSkyDomeColorsComputed;

		// sky dome update finished?
		if( false != IsSkyDomeUpdateFinished() )
		{
			// update time stamp
			m_skyDomeTextureTimeStamp[ GetBackBuffer() ] = newFrameID;

			// get new haze color
			const float c_invNumHazeSamples( 1.0f / (float) c_skyTextureWidth );
			m_curHazeColor = m_hazeColorAccum * c_invNumHazeSamples;
			m_curHazeColorMieNoPremul = m_hazeColorMieNoPremulAccum * c_invNumHazeSamples;
			m_curHazeColorRayleighNoPremul = m_hazeColorRayleighNoPremulAccum * c_invNumHazeSamples;

			// get new sky hemisphere colors
			const float c_scaleHemiTop(2.0f / (c_skyTextureWidth * c_skyTextureHeight));
			const float c_scaleHemiSide(8.0f / (c_skyTextureWidth * c_skyTextureHeight));
			m_curSkyHemiColor[0] = m_skyHemiColorAccum[0] * c_scaleHemiTop;
			m_curSkyHemiColor[1] = m_skyHemiColorAccum[1] * c_scaleHemiSide;
			m_curSkyHemiColor[2] = m_skyHemiColorAccum[2] * c_scaleHemiSide;
			m_curSkyHemiColor[3] = m_skyHemiColorAccum[3] * c_scaleHemiSide;
			m_curSkyHemiColor[4] = m_skyHemiColorAccum[4] * c_scaleHemiSide;

			// toggle sky light buffers
			ToggleBuffer();
		}
	}

	// update frame ID
	m_lastFrameID = newFrameID;
}

void CSkyLightManager::SetQuality( int32 quality )
{
	if( quality != m_pSkyLightNishita->GetInScatteringIntegralStepSize() )
	{
		Sync();
		// when setting new quality we need to start sky dome update from scratch...
		// ... to avoid "artifacts" in the resulting texture
		m_numSkyDomeColorsComputed = 0; 
		m_pSkyLightNishita->SetInScatteringIntegralStepSize( quality );
	}
}

const SSkyLightRenderParams* CSkyLightManager::GetRenderParams() const
{
	return &m_renderParams;
}

void CSkyLightManager::UpdateRenderParams()
{
	// sky dome mesh data
	m_renderParams.m_pSkyDomeMesh = m_pSkyDomeMesh;

	// sky dome texture access
	m_renderParams.m_skyDomeTextureTimeStamp = m_skyDomeTextureTimeStamp[ GetFrontBuffer() ];
	m_renderParams.m_pSkyDomeTextureDataMie = (const void*) &m_skyDomeTextureDataMie[ GetFrontBuffer() ][ 0 ];
	m_renderParams.m_pSkyDomeTextureDataRayleigh = (const void*) &m_skyDomeTextureDataRayleigh[ GetFrontBuffer() ][ 0 ];
	m_renderParams.m_skyDomeTextureWidth = c_skyTextureWidth;
	m_renderParams.m_skyDomeTextureHeight = c_skyTextureHeight;
	m_renderParams.m_skyDomeTexturePitch = c_skyTextureWidth * sizeof( SFloatABGR32 );

	// shader constants for final per-pixel phase computation
	m_renderParams.m_partialMieInScatteringConst = m_pSkyLightNishita->GetPartialMieInScatteringConst();
	m_renderParams.m_partialRayleighInScatteringConst = m_pSkyLightNishita->GetPartialRayleighInScatteringConst();
	Vec3 sunDir( m_pSkyLightNishita->GetSunDirection() );
	m_renderParams.m_sunDirection = Vec4( sunDir.x, sunDir.y, sunDir.z, 0.0f );
	m_renderParams.m_phaseFunctionConsts = m_pSkyLightNishita->GetPhaseFunctionConsts();
	m_renderParams.m_hazeColor = Vec4( m_curHazeColor.x, m_curHazeColor.y, m_curHazeColor.z, 0 );
	m_renderParams.m_hazeColorMieNoPremul = Vec4( m_curHazeColorMieNoPremul.x, m_curHazeColorMieNoPremul.y, m_curHazeColorMieNoPremul.z, 0 );
	m_renderParams.m_hazeColorRayleighNoPremul = Vec4( m_curHazeColorRayleighNoPremul.x, m_curHazeColorRayleighNoPremul.y, m_curHazeColorRayleighNoPremul.z, 0 );

	// set sky hemisphere colors
	m_renderParams.m_skyColorTop = m_curSkyHemiColor[0];
	m_renderParams.m_skyColorNorth = m_curSkyHemiColor[3];
	m_renderParams.m_skyColorWest = m_curSkyHemiColor[4];
	m_renderParams.m_skyColorSouth = m_curSkyHemiColor[1];
	m_renderParams.m_skyColorEast = m_curSkyHemiColor[2];

	// copy sky dome condition params
	m_curSkyDomeCondition = m_updatingSkyDomeCondition;

	m_needRenderParamUpdate = 0;
}

void CSkyLightManager::GetCurSkyDomeCondition( SSkyDomeCondition& skyCond ) const
{
	skyCond = m_curSkyDomeCondition;
}

bool CSkyLightManager::IsSkyDomeUpdateFinished() const
{
	return( c_skyDomeTextureSize == m_numSkyDomeColorsComputed );
}


void CSkyLightManager::InitSkyDomeMesh()
{
	const uint32 c_numRings( 20 );
	const uint32 c_numSections( 20 );
	const uint32 c_numSkyDomeVertices( ( c_numRings + 1 ) * ( c_numSections + 1 ) );
	const uint32 c_numSkyDomeTriangles( 2 * c_numRings * c_numSections );
	const uint32 c_numSkyDomeIndices( c_numSkyDomeTriangles * 3 );

	std::vector< uint16 > skyDomeIndices;
	std::vector< SVF_P3F_C4B_T2F > skyDomeVertices;

	// setup buffers with source data
	skyDomeVertices.reserve( c_numSkyDomeVertices );
	skyDomeIndices.reserve( c_numSkyDomeIndices );

	// calculate vertices
	float sectionSlice( DEG2RAD( 360.0f / (float) c_numSections ) );
	float ringSlice( DEG2RAD( 180.0f / (float) c_numRings ) );
	for( uint32 a( 0 ); a <= c_numRings; ++a )
	{
		float w( sinf( a * ringSlice ) );
		float z( cosf( a * ringSlice ) );

		for( uint32 i( 0 ); i <= c_numSections; ++i )
		{
			SVF_P3F_C4B_T2F v;
			
			float ii( i - a * 0.5f ); // Gives better tessellation, requires texture address mode to be "wrap" 
																// for u when rendering (see v.st[ 0 ] below). Otherwise set ii = i;		
			v.xyz = Vec3(cosf( ii * sectionSlice ) * w, sinf( ii * sectionSlice ) * w, z);
			assert( fabs( v.xyz.GetLengthSquared() - 1.0 ) < 1e-2/*1e-4*/ );		// because of FP-16 precision
			v.st = Vec2(ii / (float) c_numSections, 2.0f * (float) a / (float) c_numRings);
			skyDomeVertices.push_back( v );
		}
	}

	// build faces
	for( uint32 a( 0 ); a < c_numRings; ++a )
	{
		for( uint32 i( 0 ); i < c_numSections; ++i )
		{
			skyDomeIndices.push_back( (uint16) ( a * ( c_numSections + 1 ) + i + 1 ) );
			skyDomeIndices.push_back( (uint16) ( a * ( c_numSections + 1 ) + i ) );
			skyDomeIndices.push_back( (uint16) ( ( a + 1 ) * ( c_numSections + 1 ) + i + 1 ) );

			skyDomeIndices.push_back( (uint16) ( ( a + 1 ) * ( c_numSections + 1 ) + i ) );
			skyDomeIndices.push_back( (uint16) ( ( a + 1 ) * ( c_numSections + 1 ) + i + 1 ) );
			skyDomeIndices.push_back( (uint16) ( a * ( c_numSections + 1 ) + i ) );
		}
	}

	// sanity checks
	assert( skyDomeVertices.size() == c_numSkyDomeVertices );
	assert( skyDomeIndices.size() == c_numSkyDomeIndices );

	// create static buffers in renderer
	IRenderer* pRenderer(gEnv->pRenderer);
	//m_pSkyDomeVB = pRenderer->CreateBuffer(c_numSkyDomeVertices, VERTEX_FORMAT_P3F_TEX2F, "SkyHDR", false);
	//if (m_pSkyDomeVB)
	//	pRenderer->UpdateBuffer(m_pSkyDomeVB, &skyDomeVertices[0], c_numSkyDomeVertices, true);

	//pRenderer->CreateIndexBuffer(&m_skyDomeIB, &skyDomeIndices[0], c_numSkyDomeIndices);
	m_pSkyDomeMesh = pRenderer->CreateRenderMeshInitialized(&skyDomeVertices[0], c_numSkyDomeVertices, eVF_P3F_C4B_T2F, 
		&skyDomeIndices[0], c_numSkyDomeIndices, R_PRIMV_TRIANGLES, "SkyHDR", "SkyHDR");
}


int CSkyLightManager::GetFrontBuffer() const
{
	assert( m_curBackBuffer >= 0 && m_curBackBuffer <= 1 );
	return( ( m_curBackBuffer + 1 ) & 1 );
}


int CSkyLightManager::GetBackBuffer() const
{
	assert( m_curBackBuffer >= 0 && m_curBackBuffer <= 1 );
	return( m_curBackBuffer );
}


void CSkyLightManager::ToggleBuffer()
{
	assert( m_curBackBuffer >= 0 && m_curBackBuffer <= 1 );
	//better enforce cache flushing then making PPU wait til job has been finished
	m_curBackBuffer = ( m_curBackBuffer + 1 ) & 1;
	m_needRenderParamUpdate = 1;
}
