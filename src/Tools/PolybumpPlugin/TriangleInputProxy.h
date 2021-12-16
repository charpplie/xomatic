#pragma once

#include "TangentSpaceCalculation.h"					// CTangentSpaceCalculation

#include <map>					// STL map
#include <vector>				// STL vector



class CTriangleInputProxy
{
public:

	// constructor
	//! /param inpMesh must not be 0, normals must be welded
	CTriangleInputProxy( CSimpleIndexedMesh &rMesh )	:m_rMesh(rMesh)
	{
	}

	//! /return 0..
	unsigned int GetTriangleCount() const
	{
		return m_rMesh.m_FaceCount;
	}

	//! /param indwTriNo 0..
	//! /param outdwPos
	//! /param outdwNorm
	//! /param outdwUV
	void GetTriangleIndices( const unsigned int indwTriNo, unsigned int outdwPos[3], unsigned int outdwNorm[3], unsigned int outdwUV[3] ) const
	{
		CObjFace &ref=m_rMesh.m_pFaces[indwTriNo];
//		const CTriNormIndex &norm=m_NormIndx[indwTriNo];

		for(int i=0;i<3;i++)
		{
			outdwPos[i]=ref.v[i];
			outdwNorm[i]=ref.n[i];
//			outdwNorm[i]=norm.p[i];
			outdwUV[i]=ref.t[i];
		}
	}

	//! /param indwPos 0..
	//! /param outfPos
	void GetPos( const unsigned int indwPos, float outfPos[3] ) const
	{
		CObjVert &ref=m_rMesh.m_pVerts[indwPos];

		outfPos[0]=ref.x;
		outfPos[1]=ref.y;
		outfPos[2]=ref.z;
	}

	//! /param indwPos 0..
	//! /param outfUV 
	void GetUV( const unsigned int indwPos, float outfUV[2] ) const
	{
		assert(m_rMesh.m_pCoors);
		CObjCoor &ref=m_rMesh.m_pCoors[indwPos];

		outfUV[0]=ref.s;
		outfUV[1]=ref.t;
	}

private: // -------------------------------------------------------------------

	CSimpleIndexedMesh 					&m_rMesh;				//!< pointer to the mesh - is not allowed to be 0
};

