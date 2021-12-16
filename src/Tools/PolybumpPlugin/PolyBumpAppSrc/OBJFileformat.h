#pragma once


#include <vector>															// STL vector<>
#include <map>																// STL map<>
#include <hash_map>														// STL hash_multimap<>

#include "Cry_Math.h"													// Vec3
#include "../ITriangleMeshOutputProxy.h"			// ITriangleMeshOutputProxy
#include "LineReader.h"												// CLineReader
//#include "ProgressWindow.h"										// CProgressWindow

struct IDirect3DVertexBuffer8;
struct IVertexBuffer;


class COBJFileFormat
{
public:

	//! constructor
	COBJFileFormat( const bool bRecreateNormals );

	//! destructor
	virtual ~COBJFileFormat();

	//! load the .OBJ file format (Alias Wavefront)
	//! /param inszFileName
	bool LoadOBJ( const char *inszFileName, ITriangleMeshOutputProxy &rOutput );

///	uint32 GetMaterialCount() const;
	
//	const COBJMaterial *GetMaterials() const;

private: // ------------------------------------------------------------------------------------------

	//! free the trianglebunch and materials
	void FreeData();

	//!
	void _LoadOBJ_ComputeSize( CLineReader &in, uint32 &dwTriangleCount, uint32 &dwPosCount,
		uint32 &dwNormalCount, uint32 &dwUVCount, bool &bSmoothingGroupsSpecified );
/*
	// first try to load data from ZBrush obj file (fast), if that fails - load whole file and ComputeSize manually
	void  _LoadOBJ_ComputeSize_WithZBrushDetection( CLineReader &in, uint32 &dwTriangleCount, uint32 &dwPosCount,
		uint32 &dwNormalCount, uint32 &dwUVCount, bool &bSmoothingGroupsSpecified );
*/
	//!
	//! /return true=everything was fine, false=an error occured
	bool _LoadOBJ_LoadData( CLineReader &in, const bool bSmoothingGroupsSpecified, ITriangleMeshOutputProxy &rOutput );

	//! load the materials to an .OBJ file
	//! /param inszFileName path and filename of the material definition (.MTL) file
	//! /return true=everything was fine, false=an error occured
	bool _LoadMTL( const char *inszFileName );

	//! recreate the normal information (with smoothing groups)
	//! \param pSmoothingGroups [0..rOutput.GetTriangleCount()-1]
	//! /return true=everything was fine, false=an error occured
	bool _RecreateNormals( ITriangleMeshOutputProxy &rOutput, const std::vector<uint32> SmoothingGroups );

private: // --------------------------------------------------------------------------------

	std::vector<COBJMaterial *>			m_Materials;						//!< vector with all materials (destruction is done for each element here)
	bool														m_bHasTex;							//!< texture uv information
	bool														m_bRecreateNormals;			//!<
//	CProgressWindow									m_Progress;							//!<

	// ---------------------------------------------------------------------------------------

	//!
	void MessageAdd( const char *szStr, ... );

	//!
	void ErrorAdd( const char *szStr, ... );

	//! O(n)
	//! creates a new one - or returns an exiting one with the same name
	COBJMaterial *FindOrCreateNewMaterial( const char *insName );

	friend class CTriangleInputProxy;
};




