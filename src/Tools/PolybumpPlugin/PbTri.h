#pragma once

#include <vector>														// STL vector<>


#include "Cry_Math.h"												// Vec3


// to save memory
struct SCompressedNormal
{
	// default constructor
	SCompressedNormal()
	{
	}

	// constructor
	SCompressedNormal( const Vec3 &Value )
	{
		assert(Value.x>=-1 && Value.x<=1);
		assert(Value.y>=-1 && Value.y<=1);
		assert(Value.z>=-1 && Value.z<=1);

		x = static_cast<short>(Value.x*32767.0f);
		y = static_cast<short>(Value.y*32767.0f);
		z = static_cast<short>(Value.z*32767.0f);
	}

	//
	Vec3 GetDecompressed() const
	{
		Vec3 ret;

		ret.x=static_cast<float>(x)/32767.0f;
		ret.y=static_cast<float>(y)/32767.0f;
		ret.z=static_cast<float>(z)/32767.0f;

		return ret;
	}

private:

	short x,y,z;		// –32767 .. 32767 is mapped to the range -1..1, –32768 is illegal to use
};



class CPbBaseTri
{
public: 

	Vec3							m_Verts[3];						//!< vertex positions
#pragma pack(push,1)
	SCompressedNormal	m_VertsNormal[3];			//!< vertex normals
	unsigned char			m_TriMaterialID;			//!< material id 0..
	char							m_dummy;							//!< to get good alignment
#pragma pack(pop)

	// ------------------------------------------------------------------------

	//! not normalized for speed reasons, call ComputeTriangleNormal().GetNormalizedSafe() if required
	//! \return not normalized normal (for performance reasons)
	Vec3 ComputeTriangleNormal() const;

	//! extend the given bounding box
	void ExtendMinMax( Vec3 &inoutMin, Vec3 &inoutMax ) const;

	//!
	void ExtendSphere( Vec3 invMid, float &outfRadius2 );

	//! /return FLT_MAX nothing was intersected
	float CalcIntersectionFromDir( const Vec3 &invStart, const Vec3 &invDir );
};



//////////////////////////////////////////////////////////////////////
class CPbHighTri :public CPbBaseTri
{
public:

	//! /param inbDoClipping do clipping within the triangle area
  void CalcBarycentricCoordsFromPoint( const Vec3 &point, Vec3 &normal, bool inbDoClipping );
};

//////////////////////////////////////////////////////////////////////
class CPbLowTri :public CPbBaseTri
{
public:
	//! normalize normal after calling this function
	//! /param s x coordinate in the texture
	//! /param t y coordinate in the texture
	//! /param inbDoClipping do clipping within the triangle area
  void CalcBarycentricCoordsFromUV( float s, float t, Vec3 &point, Vec3 &normal, bool inbDoClipping ) const;

	//! calculates m_Plane and m_fUVArea
	void RefreshInternals();

	void CalcBarycentrics( float inS, float inT, float outCoor[3], bool inbDoClipping ) const;

	// -------------------------------------------------------------------------------------------

	Vec3					m_vTangent[3];				//!< used for tangent space bump mapping
	Vec3					m_vBinormal[3];				//!< used for tangent space bump mapping
	Vec3					m_vTNormal[3];				//!< used for tangent space bump mapping

	float					m_fS[3];							//!< texture x=s coordinates
  float					m_fT[3];							//!< texture y=t coordinates

  float					m_fUVArea;						//!< area in texture-space
};



template <class TTri>
class CIntersInfo
{
public:
//  Vec3					m_Point;							//!< needed? can be optimized away
  float					m_fDist;							//!< from starting point

	uint32				m_dwObjectIndex;			//!< only valid if m_pTri!=0
	TTri *				m_pTri;								//!< must not be 0

	//! \param vDir needs to be normalized
	Vec3 ComputeIntersectionPoint( const Vec3 &vStart, const Vec3 &vDir ) const
	{
		return vStart+m_fDist*vDir;
	}
};


//! calculates the area of a 2D triangle
//! /param pfX pointer to three x values
//! /param pfY pointer to three y values
inline float CalcUVArea( float *pfX, float *pfY )
{
  float prod1=pfX[1]*pfY[2]-pfY[1]*pfX[2];
  float prod2=pfX[0]*pfY[1]-pfY[0]*pfX[1];
  float prod3=pfY[0]*pfX[2]-pfX[0]*pfY[2];

  return (prod1+prod2+prod3)/2.0f; 
}


typedef std::vector<CIntersInfo<CPbHighTri> >						CIntInfoHighList;
typedef CIntInfoHighList::iterator											CIntInfoHighIt;

typedef std::vector<CIntersInfo<CPbLowTri> >						CIntInfoLowList;
typedef CIntInfoLowList::iterator												CIntInfoLowIt;

