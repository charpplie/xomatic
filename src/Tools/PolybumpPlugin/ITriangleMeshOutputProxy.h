#pragma once

#include "Cry_Math.h"													// Vec3

struct SMeshOutputTriangle
{
	uint32		m_PosIndex[3];
	uint32		m_NormalIndex[3];
	uint32		m_UVIndex[3];
	uint32		m_dwMaterialId;
};

class COBJMaterial
{
public:

	//! constructor
	COBJMaterial( const char *inszName, uint32 dwId ) 
		:m_dwId(dwId), m_sName(inszName)
	{
		assert(inszName);

		m_Ka=Vec3(1,1,1);
		m_Kd=Vec3(1,1,1);
		m_Ks=Vec3(1,1,1);
		m_Kss=Vec3(0,0,0);
	}

	// ------------------------------------------------------------------------

	string								m_sName;											//!< used as connection between .OBJ and .MTL
	Vec3									m_Ka;													//!< ambient color
	Vec3									m_Kd;													//!< diffuse color
	Vec3									m_Ks;													//!< specular color
	Vec3									m_Kss;												//!< subsurface color
	string								m_sDiffuseTexture;						//!< diffuse texture
	string								m_sBumpTexture;								//!< bump texture
	uint32								m_dwId;												//!< 
};


struct ITriangleMeshOutputProxy
{
	virtual bool Allocate( const uint32 dwTriangleCount, const uint32 dwPosCount, const uint32 dwNormalCount, const uint32 dwUVCount )=0;

	virtual void SetPos( const uint32 dwIndex, const Vec3 &rValue )=0;

	virtual Vec3 GetPos( const uint32 dwIndex ) const=0; 

	virtual void SetNormal( const uint32 dwIndex, const Vec3 &rValue )=0;

	virtual Vec3 GetNormal( const uint32 dwIndex ) const=0;

	virtual uint32 GetNormalCount() const=0;

	virtual void SetUV( const uint32 dwIndex, const Vec2 &rValue )=0;

	virtual void SetTriangle( const uint32 dwIndex, SMeshOutputTriangle &rTri )=0;

	virtual uint32 GetTriangleCount() const=0; 

	virtual void GetPosIndices( const uint32 dwTri, uint32 dwPosIndex[3] ) const=0; 

	virtual void SetNormalIndices( const uint32 dwTri, const uint32 dwNormalIndex[3] )=0;

	virtual void GetNormalIndices( const uint32 dwTri, uint32 dwNormalIndex[3] ) const=0;

	// after allocation the normals are undefined
	virtual bool AllocateNormals( const uint32 dwNormalCount )=0;

	virtual bool AllocateMaterials( const  uint32 dwCount )=0;

	virtual void SetMaterial( const uint32 dwIndex, const COBJMaterial &rMat )=0;
};
