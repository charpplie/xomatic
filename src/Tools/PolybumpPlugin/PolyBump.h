//////////////////////////////////////////////////////////////////////
//
//	Crytek SuperFramework Source code
//	
//	File:PolyBump.h
//
//	History: 	
//    * July 16,2001: Created by Marco Corbetta
//    * 2/15/2002:    modified by Martin Mittring, integrated in a 3D Studio Plugin
//		- 2/21/2002 modified to work in 3DStudio max as a thread
//
//////////////////////////////////////////////////////////////////////

#ifndef POLYBUMP_H
#define POLYBUMP_H

#if _MSC_VER > 1000
# pragma once
#endif

#include "types.h"																		// uint32
#include "PbTri.h"																		// CPbTri
#include "EveryObjectOnlyOnce.h"											// CEveryObjectOnlyOnce

#include "RasterCube.h"																// CRasterCube



// splitting up the memory into chunks is only useful for 32bit where virtual address space is quite limited
// todo: the non memory implementation does not support multiple high poly models
// #if !defined(WIN64)
#define USE_MEMORY_BLOCKS
// #endif // !defined(WIN64)



#ifdef USE_MEMORY_BLOCKS
//	#define MAX_MEMORY_BLOCK 4*1024*1024
//	const uint32 g_nObjPerBlock = MAX_MEMORY_BLOCK / sizeof(CPbHighTri);

// for performance g_nObjPerBlock needs to be power of two,    sizeof(CPbHighTri)=88
//const uint32 g_nObjPerBlock = 128*256;
const uint32 g_nObjPerBlock = 256*256;		// could be the best for most CPUs

#endif

class CSimpleIndexedMesh;
class CPolyBumpWorkerThread;





class CPbLowMesh
{
public:

	//! constructor
	CPbLowMesh() :m_pMesh(0), m_nNumTris(0) {}
		
	//! destructor
	~CPbLowMesh()
	{
		if(m_pMesh)
		{
			delete [] m_pMesh;
			m_pMesh=NULL;
		}    
	}

	//! copy the data, doesn´t build a accelerator data structure
	//! /param mesh pointer to the input mesh
  void GetNormalsAndClearBaseVectors( CSimpleIndexedMesh &mesh );

	//! for object and tangentspace
	//! copy the data, doesn´t build a accelerator data structure (classes BuildNormalsAndClearBaseVectors)
	//! /param mesh pointer to the input mesh
	void BuildNormalsAndBaseVectors( CSimpleIndexedMesh &mesh );

	//! this method builds the internal data structured for speedup the raycasting
	//! /param inbDebug debug output is generated
	//! /return status information that could be shown later (is terminated with '\n')
	std::string BuildRayAccelerator( const bool inbDebug );

	//! calculate the size of the bounding sphere (needed for BuildRayAccelerator())
	//! /return outMin minimum x,y,z of the bounding box
	//! /return outMin maximum x,y,z of the bounding box
	//! /return diameter size
	float CalcBoundingVolume( Vec3 &outMin, Vec3 &outMax );

	//! needed for low and high poly
	//! raytracing, gather unsorted hits
	//! /param inStart start vector in worldspace
	//! /param inDirection end vector of the ray
	//! /param outIntersections reference to the intersection list (make sure the list is empty)
	void GatherRayHits( const Vec3 invStart, const Vec3 invEnd, CIntInfoLowList &outIntersections );
	
	// -------------------------------------------------------------------------------------------

	uint32													m_nNumTris;								//!< number of triangles in the mesh
  CPbLowTri *											m_pMesh;									//!< pointer to the triangle data

	CRasterCube<uint32>							m_LPRaster;								//!< triangle intersection accelerator data structure


	inline CPbLowTri *GetTrianglePtr( const uint32 dwIndex ) const
	{
		return &m_pMesh[dwIndex];
	}

	inline float CalcTriIntersectionFromDir( const uint32 dwIndex, const Vec3 &vFrom, const Vec3 &vDir, CPbLowTri * &pElement ) const
	{
		pElement = GetTrianglePtr(dwIndex);

		return pElement->CalcIntersectionFromDir(vFrom,vDir);
	}
};




class CPbHighMesh
{
public:

	//! constructor
	CPbHighMesh();
	//! destructor
	~CPbHighMesh();

	//! raytracing, test if there is a hit or not
	//! /param inStart start vector in worldspace
	//! /param inDirection end vector of the ray
	//! /return true: ray has a intersection, false: otherwise
	bool CalcIntersection( const Vec3 invStart, const Vec3 invEnd, const uint32 indwIgnoreObject=0xffffffff );

	//! copy the data, doesn´t build a accelerator data structure
	//! /param mesh pointer to the input mesh
  void GetNormalsAndClearBaseVectors( const CSimpleIndexedMesh &mesh );

	//! this methor builds the internal data structured for speedup the raycasting
	//! /param inbDebug debug output is generated
	//! /return status information that could be shown later (is terminated with '\n')
	std::string BuildRayAccelerator( const bool inbDebug );

	//! calculate the size of the bounding sphere (needed for BuildRayAccelerator())
	//! /return outMin minimum x,y,z of the bounding box
	//! /return outMin maximum x,y,z of the bounding box
	void CalcBoundingVolume( Vec3 &outMin, Vec3 &outMax );

	//! ray casting, gather unsorted hits
	//! /param inStart start vector in world space
	//! /param inDirection end vector of the ray
	//! /param outIntersections reference to the intersection list (make sure the list is empty)
	void GatherRayHitsHP( const Vec3 invStart, const Vec3 invEnd, CIntInfoHighList &outIntersections );

	//! ray casting, gather unsorted hits
	//! /param inStart start vector in world space
	//! /param inDirection end vector of the ray
	//! /param outIntersections reference to the intersection list (make sure the list is empty)
	//! /param vLowPolyNormal not necessarily normalized
	CIntersInfo<CPbHighTri> GatherRayHitsHP_Latest( const Vec3 invStart, const Vec3 invEnd, const Vec3 vLowPolyNormal );

	//! ray casting, gather unsorted hits
	//! /param inStart start vector in world space
	//! /param inDirection end vector of the ray
	//! /param outIntersections reference to the intersection list (make sure the list is empty)
	//! /param vLowPolyNormal not necessarily normalized
	CIntersInfo<CPbHighTri> GatherRayHitsHP_Nearest( const Vec3 invStart, const Vec3 invEnd, const Vec3 vLowPolyNormal );

	//! ray casting, gather unsorted hits
	//! /param inStart start vector in world space
	//! /param inDirection end vector of the ray
	//! /param outIntersections reference to the intersection list (make sure the list is empty)
	void GatherRayHitsHP_Nearest2Sides( const Vec3 invStart, const Vec3 invEnd, float &outMin, float &outMax );

	uint32 GetFaceCount() const; 

	//!
	void FreeData();
	
	// -------------------------------------------------------------------------------------------

	uint32													m_nNumTris;								//!< number of triangles in the mesh

#ifdef USE_MEMORY_BLOCKS
	typedef std::vector<CPbHighTri> TPbHighTriBlock;

	std::vector<TPbHighTriBlock>		m_MeshBlocks;							//!< data is stored in smaller chunks to limit the 32bit memory allocation problems
#else
  CPbHighTri *										m_pMesh;									//!< pointer to the triangle data
#endif //USE_MEMORY_BLOCKS

	CRasterCube<DWORD>					m_HPRaster;									//!< triangle intersection accelerator data structure (triangle index)

	inline CPbHighTri *GetTrianglePtr( const uint32 dwIndex )
	{
		if(dwIndex==0xffffffff)
			return 0;
#ifdef USE_MEMORY_BLOCKS
		uint32 dwBlock = dwIndex/g_nObjPerBlock;
		uint32 dwIndexInBlock = dwIndex%g_nObjPerBlock;

		assert(dwBlock<m_MeshBlocks.size());
		assert(dwIndexInBlock<m_MeshBlocks[dwBlock].size());

		return &m_MeshBlocks[dwBlock][dwIndexInBlock];
#else //USE_MEMORY_BLOCKS
		return &m_pMesh[dwIndex];
#endif //USE_MEMORY_BLOCKS
	}

	inline float CalcTriIntersectionFromDir( const uint32 dwIndex, const Vec3 &vFrom, const Vec3 &vDir, CPbHighTri * &pElement )
	{
		pElement = GetTrianglePtr(dwIndex);

		return pElement->CalcIntersectionFromDir(vFrom,vDir);
	}
};



	


bool CreateBumpMap( CPolyBumpWorkerThread *inpData );

#endif


