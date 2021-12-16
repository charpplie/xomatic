#pragma once

#include "SimpleBitmap.h"							// CSimpleBitmap<>
#include <vector>											// STL vector
#include <map>												// STL map<,>
#include "Cry_Math.h"									// 
#include "Cry_Vector3.h"							// Vec3




typedef void * gzFile;

// used to store the .SRF file format in memory and to handle persistency (load/save)
// With this data format we decouple the PolyBump process from it's usage in the shader
// Example usage: horizonmaps, unoccluded area direction, accessibility, radiosity fakes, PTM, SH,
// parallax bump mapping, displacement mapping (static/dynamic, not only in normal direction)
// >8 Bit normal maps, diffuse, specular, precalculated transparency (subsurface scattering), wrinkles?
class CSRFData
{
public:
	// constructor
	CSRFData();
	// destructor
	virtual ~CSRFData();

	// ------------------------------------------------------------------------------------

	//
	void FreeData();

	// Arguments:
	//   dwWidth - must not be 0 
	//   dwHeight - must not be 0 
	void Init( const uint32 dwWidth, const uint32 dwHeight );

	// Arguments:
	//   szFilename - must not be 0
	bool Save( const char *szFilename );

	// Arguments:
	//   szFilename - must not be 0
	bool Load( const char *szFilename );

	// ------------------------------------------------

	//
	bool AllocSurfaceWorldNormals();
	//
	bool AllocSurfaceDiffuse();
	//
	bool AllocSurfaceSpecular();
	// specular power in float
	bool AllocSurfaceGloss();
	//
	bool AllocSurfaceTransparency();
	//
	bool AllocSurfaceHighPos();
	// Arguments:
	//   dwHorizonCount - must not be 0
	bool AllocSurfaceHorizon( const uint32 dwHorizonCount );

	bool AllocSurfaceHighPolyMaterialId();

	// Arguments:
	//   dwTriangleCount - must not be 0
	bool AllocSourceLowPoly( const uint32 dwTriangleCount );
	//
	bool AllocAALevel();

	// also creates the data
	void AllocSurfaceIndirection( const uint32 dwPixelCount );

	// ------------------------------------------------

	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	//   vNormal - in range -1 .. 1
	void SetWorldHighPolyNormal( const uint32 dwX, const uint32 dwY, const Vec3 &vNormal );
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	//   vColor - in range 0 .. 1, order: RGB
	void SetDiffuse( const uint32 dwX, const uint32 dwY, const float vColor[3] );
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	//   vColor - in range 0 .. 1, order: RGB
	void SetSpecular( const uint32 dwX, const uint32 dwY, const float vColor[3] );
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	//   fValue - in range 1 .. 100 ..
	void SetGloss( const uint32 dwX, const uint32 dwY, const float fValue );
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	//   fValue - 0..1
	void SetTransparency( const uint32 dwX, const uint32 dwY, const float fValue );
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	void SetHighPos( const uint32 dwX, const uint32 dwY, const Vec3 &vRelPos );
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	//   dwHorizonId - in range 0..m_dwHorizonCount-1
	//   fAngle - 0..PI/2
	void SetHorizon( const uint32 dwX, const uint32 dwY, const uint32 dwHorizonId, const float &fAngle );
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	void SetLowPolyTriangleId( const uint32 dwX, const uint32 dwY, const uint32 dwTriangleId );
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	//   MaterialId - 1..255, 0 if there was no hit (don't call for 0)
	void SetHighPolyMaterialId( const uint32 dwX, const uint32 dwY, const uint8 MaterialId );

	// Arguments:
	//   dwIndex - 0/1/2
	void SetLowPolyVertex( const uint32 dwTriangle, const uint32 dwIndex, const Vec3 &vPos,
		const Vec3 &vTanU, const Vec3 &vTanV, const Vec3 &vTanN, const float fU, const float fV);
	//
	// Arguments:
	//   dwIndex - 0/1/2
	void SetLowPolyTriangle( const uint32 dwTriangle, const uint32 dwMaterialId );
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	void SetAALevel( const uint32 dwX, const uint32 dwY, const uint8 AALevel );

	// ------------------------------------------------

	bool IsThereDiffuse() const;

	bool IsThereHighPolyMaterialId() const;

	// ------------------------------------------------

	// returns the normal in world space
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	Vec3 GetHighPolyNormal( const uint32 dwX, const uint32 dwY ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	void GetDiffuse( const uint32 dwX, const uint32 dwY, float outValue[3] ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	void GetSpecular( const uint32 dwX, const uint32 dwY, float outValue[3] ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	float GetGloss( const uint32 dwX, const uint32 dwY ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	float CalcHorizon( const uint32 dwX, const uint32 dwY, const float fAngle ) const;

	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	float GetTransparency( const uint32 dwX, const uint32 dwY ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	Vec3 GetHighPos( const uint32 dwX, const uint32 dwY ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	// Return
	//   0xffff=invalid, triangle index otherwise
	uint16 GetLowPolyTriangleId( const uint32 dwX, const uint32 dwY ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	// Returns:
	//   1..255 for material, 0 if there was no hit
	uint8 GetHighPolyMaterialId( const uint32 dwX, const uint32 dwY ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	uint8 GetAALevel( const uint32 dwX, const uint32 dwY ) const;
	
	// for the border expand feature, offsets the given position to the nearest border,
	// requires AllocSurfaceIndirection() to be called before
	// Arguments:
	//   dwInX - in range 0..m_dwWidth-1
	//   dwInY - in range 0..m_dwHeight-1
	//   dwOutX - in range 0..m_dwWidth-1
	//   dwOutY - in range 0..m_dwHeight-1
	void GetNearestXY( const uint32 dwInX, const uint32 dwInY, uint32 &dwOutX, uint32 &dwOutY ) const;

	//
	uint16 *GetLowPolyTriangleIdBuffer() const;

	//
	uint32 GetWidth() const;
	//
	uint32 GetHeight() const;
	// Return:
	//   0 if there is no horizon information
	uint32 GetHorizonCount() const;
	// Arguments:
	//   dwIndex start with 0 increase
	// Return:
	//   0 if end line was reached
	char *GetInfoString( const uint32 dwLine ) const;

	// ------------------------------------------------------------------------------------

	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	Vec3 CalcTangentHighPolyNormal( const uint32 dwX, const uint32 dwY, const bool bPerVertexOrthogonalizedTS ) const;

	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	// Return:
	//   true=success, false=failed
	bool CalcLowPos( const uint32 dwX, const uint32 dwY, Vec3 &outValue ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	// Return:
	//   true=success, false=failed
	bool CalcLowTangentBase( const uint32 dwX, const uint32 dwY, Vec3 &outTangU, Vec3 &outTangV, Vec3 &outTangN, const bool bPerVertexOrthogonalizedTS ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	// Return:
	//   true=success, false=failed
	bool CalcLowNormal( const uint32 dwX, const uint32 dwY, Vec3 &outTangN ) const;
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	//   bQuadratic - true=normal generation quadratic, false=normal generation linear
	// Return:
	//   true=success, false=failed
	bool CalcLowPNTrianglePosNormal( const uint32 dwX, const uint32 dwY, const bool bQuadratic, Vec3 &outPos, Vec3 &outNormal ) const;
	//
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	// Return:
	//   FLT_MAX if the method failed, world distance otherwise (signed)
	float CalcDisplacementDist( const uint32 dwX, const uint32 dwY, const bool bNPatch, const bool bQuadric ) const;

	// slow
	// Arguments:
	//   outMin in worldspace, FLT_MAX if there is no valid range
	//   outMax in worldspace, -FLT_MAX if there is no valid range
	void CalcDisplacementRange( float &outMin, float &outMax, const bool bNPatch, const bool bQuadric ) const;

	//
	// Arguments:
	//   vDirectionOut - Vector in worldspace, length <=1
	//   fBrighter - 0..1
	void CalcUnocccludedAreaDirection( const uint32 dwX, const uint32 dwY, Vec3 &vDirectionOut, const float fBrighter ) const;
	// Return:
	//   0=completely hidden .. 1=full hemisphere can see the sky
	float CalcAccessibilityValue( const uint32 dwX, const uint32 dwY ) const;
	// uses GetNearestXY() for proper border expand
	// Arguments:
	//   fBaryCorrsOut - barycentric coordinates
	// Return:
	//   0xffff=failed, otherwise success
	uint16 CalcLowTriangle( const uint32 dwX, const uint32 dwY, Vec3 &vBaryCorrsOut ) const;
	// to get the orientation of the horizon, not affected by and UV mapping, orthogonalized
	// Arguments:
	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
	//   outBaseN - == high poly normal
	void CalcHighPolyBase( const uint32 dwX, const uint32 dwY, Vec3 &outBaseA, Vec3 &outBaseB, Vec3 &outBaseN ) const;

	// convenience helper method
	Vec3 TransformDirectionIntoTangentSpace( const uint32 dwX, const uint32 dwY, const Vec3 vWorldDir, const bool bPerVertexOrthogonalizedTS ) const;
	//
	uint32 CalcUsedAreaSize() const;
	//
	uint32 CalcAALevelSum() const;
	//
	void SetPropertiesString( const char *szProp );
	// Return:
	//   don't store returned pointer !
	const char *GetPropertiesString() const;

	// Return:
	//   true=CW, false=CCW (needs to be tested)
	bool CalcLowPolyUVOrientation( const uint32 dwX, const uint32 dwY ) const;

	// ------------------------------------------------------------------------------------

	// user data (used to store computation settings to redo the operation)
  
	// Arguments:
	//   szKey must not be 0, must not be ""	(case sensitive)
	// Return:
	//   true if an entry was found, false otherwise (Value is not changed)
	bool GetValue( const char *szKey, char *szValue, const uint32 dwMaxSize ) const;

	// Arguments:
	//   szKey must not be 0, must not be ""	(case sensitive)
	// Return:
	//   true if an entry was found, false otherwise (Value is not changed)
	bool GetValue( const char *szKey, int &iValue ) const;

	// Arguments:
	//   szKey must not be 0, must not be ""	(case sensitive)
	// Return:
	//   true if an entry was found, false otherwise (Value is not changed)
	bool GetValue( const char *szKey, float &fValue ) const;

	// Arguments:
	//   szKey must not be 0, must not be ""
	//   szValue must not be 0, can be "" to remove the entry
	void SetValue( const char *szKey, const char *szValue );

	// Arguments:
	//   szKey must not be 0, must not be ""
	void SetValue( const char *szKey, const int iValue );

	// Arguments:
	//   szKey must not be 0, must not be ""
	void SetValue( const char *szKey, const float fValue );

	// ------------------------------------------------------------------------------------

	// convenience, can be easily extended to support pitch
	// Arguments:
	//   dwPreviewId - kind of preview, you can start with 0 and count up until the return value is false, 0xffffffff to get a single preview
	//   dwStartY - usually =0, included, 0..dwWidth
	//   dwEndY - usually =dwHeight, excluded, 0..dwWidth, >=dwStartY
	// Returns:
	//   true if there was data
	bool RequestPreview( const uint32 dwWidth, const uint32 dwHeight, const uint32 dwStartY, const uint32 dwEndY, 
		uint32 *pARGB, const uint32 dwPreviewId=0xffffffff ) const;

private: // -----------------------------------------------------------------------------

	struct SRGB8
	{
		unsigned char r,g,b;

		// default constructor
		SRGB8() :r(0),g(0),b(0)
		{
		}

		// constructor
		SRGB8( const float Value[3] )
		{
			assert(Value[0]>=0 && Value[0]<=1);
			assert(Value[1]>=0 && Value[1]<=1);
			assert(Value[2]>=0 && Value[2]<=1);

			r=static_cast<unsigned char>(Value[0]*255.0f+0.5f);
			g=static_cast<unsigned char>(Value[1]*255.0f+0.5f);
			b=static_cast<unsigned char>(Value[2]*255.0f+0.5f);
		}

		const SRGB8 operator*( const float fValue ) const
		{
			SRGB8 ret;

			ret.r=static_cast<unsigned char>(r*fValue);
			ret.g=static_cast<unsigned char>(g*fValue);
			ret.b=static_cast<unsigned char>(b*fValue);

			return ret; 
		}

		const SRGB8 operator+=( const SRGB8 &ref )
		{
			r+=ref.r;			assert(r>=ref.r);
			g+=ref.g;			assert(g>=ref.g);
			b+=ref.b;			assert(b>=ref.b);

			return *this; 
		}

		void GetDecompressed( float outValue[3] ) const
		{
			outValue[0] = static_cast<float>(r)/255.0f;
			outValue[1] = static_cast<float>(g)/255.0f;
			outValue[2] = static_cast<float>(b)/255.0f;
		}
	};

	// ----------------------------------------

	struct SNormal16
	{
		// default constructor
		SNormal16() :x(0),y(0),z(0)
		{
		}

		// constructor
		SNormal16( const Vec3 &Value )
		{
			assert(Value.x>=-1 && Value.x<=1);
			assert(Value.y>=-1 && Value.y<=1);
			assert(Value.z>=-1 && Value.z<=1);

			x = static_cast<short>(Value.x*32767.0f);
			y = static_cast<short>(Value.y*32767.0f);
			z = static_cast<short>(Value.z*32767.0f);
		}

		const SNormal16 operator*( const float fValue ) const
		{
			SNormal16 ret;

			ret.x=static_cast<short>(x*fValue);
			ret.y=static_cast<short>(y*fValue);
			ret.z=static_cast<short>(z*fValue);

			return ret; 
		}

		// be careful with overflow
		const SNormal16 operator+=( const SNormal16 &ref )
		{
			x+=ref.x;
			y+=ref.y;
			z+=ref.z;

			return *this; 
		}

		Vec3 GetDecompressed() const
		{
			Vec3 ret;

			ret.x=static_cast<float>(x)/32767.0f;
			ret.y=static_cast<float>(y)/32767.0f;
			ret.z=static_cast<float>(z)/32767.0f;

			return ret;
		}

		const bool operator==( const SNormal16 &ref ) const
		{
			return x==ref.x && y==ref.y && z==ref.z; 
		}

		short x,y,z;		// –32767 .. 32767 is mapped to the range -1..1, –32768 is illegal to use
	};

	// ----------------------------------------

	// beware of changing this - file format might be affected
	struct SSourceVertex
	{
		// constructor
		SSourceVertex() :m_fU(0), m_fV(0)
		{
		}

		Vec3							m_Pos;							//!<
		SNormal16					m_TangentU;					//!<
		SNormal16					m_TangentV;					//!<
		SNormal16					m_TangentN;					//!< normal
		float							m_fU;								//!<
		float							m_fV;								//!<
	};

	// ----------------------------------------

	// beware of changing this - file format might be affected
	struct SSourceTriangle
	{
		// constructor
		SSourceTriangle() :m_dwMaterialId(0xffffffff)
		{
		}

		SSourceVertex			m_Vertex[3];				//!<
		uint32						m_dwMaterialId;			//!<
	};

	struct SIndirectionXY
	{
		SIndirectionXY( const char _x=0, const char _y=0 ) :x(_x), y(_y)
		{
		}

		char							x;									//!< relative offset to the nearest border in x
		char							y;									//!< relative offset to the nearest border in y
	};


	// ----------------------------------------

	uint32																m_dwWidth;									// 0 if Init() wasn't called
	uint32																m_dwHeight;									// 0 if Init() wasn't called
	uint32																m_dwHorizonCount;						// 0 if not used

	string																m_sProperties;							// e.g. "/space=tangent /normal=1 /unocc=1"

	CSimpleBitmap<SNormal16>							m_SurfaceWorldNormals;			// [m_dwWidth][m_dwHeight]
	CSimpleBitmap<SRGB8>									m_SurfaceDiffuse;						// [m_dwWidth][m_dwHeight]
	CSimpleBitmap<SRGB8>									m_SurfaceSpecular;					// [m_dwWidth][m_dwHeight]
	CSimpleBitmap<float>									m_SurfaceGloss;							// [m_dwWidth][m_dwHeight] specular power
	CSimpleBitmap<uint8>									m_SurfaceTransparency;			// [m_dwWidth][m_dwHeight]
	CSimpleBitmap<Vec3>										m_SurfaceHighPos;						// [m_dwWidth][m_dwHeight]
	CSimpleBitmap<uint16>									m_SurfaceTriangleId;				// [m_dwWidth][m_dwHeight], 0xffff means invalid
	CSimpleBitmap<uint8>									m_SurfaceHighMatId;					// [m_dwWidth][m_dwHeight], 0 mean no material, materials start at 1

	CSimpleBitmap<SIndirectionXY>					m_SurfaceIndirection;				// for border expand, created by AllocSurfaceIndirection(), use with GetNearestXY()

	CSimpleBitmap<uint8>									m_SurfaceAALevel;						// [m_dwWidth][m_dwHeight], only used during creation to do proper adaptive Antialising

	CSimpleBitmap<uint8>									m_SurfaceHorizon;						// [m_dwWidth*dwHorizonCount][m_dwHeight] [HorizonCount] horizon angles (0=up,255=horizon), to get the orientation use CalcHighPolyBase()

	std::vector<SSourceTriangle>					m_SourceLowPoly;						// [triangle count] low poly source triangles, all traingles are stored independetly

	uint32																m_dwLoadedFileVersion;			// 0=not set, value is set when loading, supported: 1 and 2
	const static uint32										m_dwActualFileVersion=2;		//
	std::map<string,string>								m_KeyValuePairs;						// userdata (used to store computation settings to redo the operation)
	bool																	m_bNormalsAreInWS;					// false=TangentSpace

	// ----------------------------------------

	//
	float GetU( const uint32 dwX ) const;
	//
	float GetV( const uint32 dwY ) const;

	// Arguments:
 	//   dwX - in range 0..m_dwWidth-1
	//   dwY - in range 0..m_dwHeight-1
 	bool IsHorizonValidAt( const uint32 dwX, const uint32 dwY );

	// produces believable random numbers
	// tested with external test app
	// Return:
	//   0..1 (8bit quality)
	static float CalcPseudoRandom( const uint32 x, const uint32 y );

	friend bool _WriteWithChunkId( gzFile hnd, uint32 dwChunkId, const std::vector<CSRFData::SSourceTriangle> &rTriangles );
	friend bool _ReadWithoutChunkId( gzFile hnd, std::vector<CSRFData::SSourceTriangle> &rTriangles );
};
