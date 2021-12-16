//////////////////////////////////////////////////////////////////////
//
//	Crytek SuperFramework Source code
//	
//	File: SimpleIndexedMesh.h
//
//	History:
//	-:Created by Vladimir Kajalin
//  -:Modified by Martin Mittring
//
//////////////////////////////////////////////////////////////////////

#ifndef __SIMPLEINDEXEDMESH2_H
#define __SIMPLEINDEXEDMESH2_H

#include <stdio.h>																		// sprintf()
#include <math.h>																			// fabs()
#include "Cry_Math.h"																	// uint32
#include "ITriangleMeshOutputProxy.h"									// ITriangleMeshOutputProxy


#include <string>																			// STL string




struct CObjVert
{
	//! constructor
  CObjVert(){}

	//! destructor
  CObjVert(float _x, float _y, float _z)
  {
    x=_x;
    y=_y;
    z=_z;
  }

	//! compare operator
	bool operator != (CObjVert & other)
  {
		return(!(*this == other));
	}

	//! compare operator
	bool operator == (CObjVert & other)
  {
    if(fabs(x-other.x)<0.0001f)
    if(fabs(y-other.y)<0.0001f)
    if(fabs(z-other.z)<0.0001f)
      return true;

    return false;
  }
  
	float x,y,z;					//< position
};




struct CObjNorm : public CObjVert
{
	// default constructor
	CObjNorm()
	{
	}

	// constructor
	CObjNorm( float _x, float _y, float _z ): CObjVert(_x,_y,_z){}

  void Normalize();
};




struct CObjCoor
{
  float s,t;

	// default constructor
	CObjCoor()
	{
	}

	// constructor
	CObjCoor( const float _s, const float _t ) :s(_s), t(_t)
	{
	}

	//! compare operator
  bool operator == (CObjCoor & other)
  {
    if(s == other.s)
    if(t == other.t)
      return true;

    return false;
  }
};





struct CObjFace
{
  CObjFace() 
	{ 
		v[0]=v[1]=v[2]=0;
		t[0]=t[1]=t[2]=0;
		n[0]=n[1]=n[2]=0;
		shader_id=0;
	}
  
  int v[3];										//!< 3 position indices
  int t[3];										//!< 3 texture indices
  int n[3];										//!< 3 normal indices
  int shader_id;							//!< material identifier index into m_pMaterials
};



class CSimpleMaterial
{
public:

	//! constructor
	CSimpleMaterial()
	{
		m_iMaterialID=-1;
		m_AmbientColor[0]=1.0f;
		m_AmbientColor[1]=1.0f;
		m_AmbientColor[2]=1.0f;
		m_DiffuseColor[0]=1.0f;
		m_DiffuseColor[1]=1.0f;
		m_DiffuseColor[2]=1.0f;
		m_SpecularColor[0]=1.0f;
		m_SpecularColor[1]=1.0f;
		m_SpecularColor[2]=1.0f;
		m_fShininess=0.0f;
		m_fShiniStrength=0.0f;
		m_fSelfIllum=0.0f;
		m_dwFlags=0;
	}

	int						m_iMaterialID;					//!< material identifier (-1 is unset)
	std::string		m_sMaterialName;				//!< Material name

	float					m_AmbientColor[3];			//!< [0]=R [1]=G [2]=B
	float					m_DiffuseColor[3];			//!< [0]=R [1]=G [2]=B
	float					m_SpecularColor[3];			//!< [0]=R [1]=G [2]=B

	float					m_fShininess;						//!< like in MAX
	float					m_fShiniStrength;				//!< like in MAX
	float					m_fSelfIllum;						//!< like in MAX
	float					m_fTransparency;				//!< like in MAX

	std::string		m_sDiffuseTexturePath;	//!<
	std::string		m_sBumpTexturePath;			//!<

	uint32				m_dwFlags;							//!< bit0: 1=marked as duplicate name, 0=unique
};	


class CSimpleIndexedMesh :public ITriangleMeshOutputProxy
{
public:

	// interface ITriangleMeshOutputProxy ---------------------------------------------

	virtual bool Allocate( const uint32 dwTriangleCount, const uint32 dwPosCount, const uint32 dwNormalCount, const uint32 dwUVCount );
	virtual void SetPos( const uint32 dwIndex, const Vec3 &rValue );
	virtual Vec3 GetPos( const uint32 dwIndex ) const; 
	virtual void SetNormal( const uint32 dwIndex, const Vec3 &rValue );
	virtual Vec3 GetNormal( const uint32 dwIndex ) const;
	virtual void SetUV( const uint32 dwIndex, const Vec2 &rValue );
	virtual void SetTriangle( const uint32 dwIndex, SMeshOutputTriangle &rTri );
	virtual uint32 GetTriangleCount() const; 
	virtual void GetPosIndices( const uint32 dwTri, uint32 dwPosIndex[3] ) const;
	virtual void SetNormalIndices( const uint32 dwTri, const uint32 dwNormalIndex[3] );
	virtual void GetNormalIndices( const uint32 dwTri, uint32 dwNormalIndex[3] ) const;
	virtual bool AllocateNormals( const uint32 dwNormalCount );
	virtual uint32 GetNormalCount() const;
	virtual uint32 GetUVCount() const;
	virtual bool AllocateMaterials( const  uint32 dwCount );
	virtual void SetMaterial( const uint32 dwIndex, const COBJMaterial &rMat );

	// --------------------------------------------------------------------------------

  // geometry data (alloceted with malloc)
  CObjFace *				m_pFaces;					// face (triangle) indices to m_pVerts,m_pCoors,m_pNorms, smoothing group information, shader_id
  CObjVert *				m_pVerts;					// vertex position
  CObjCoor *				m_pCoors;					// uv coordiantes
  CObjNorm *				m_pNorms;					// normals

	// allocated with new
	CSimpleMaterial *	m_pMaterials;			// materials [shader_id], may be 0

  uint32						m_FaceCount;			// face (triangle) indices to m_pVerts,m_pCoors,m_pNorms, smoothing group information, shader_id
  uint32						m_VertCount;			// vertex position
  uint32						m_CoorCount;			// uv coordiantes
  uint32						m_NormCount;			// normals

	uint32						m_MaterialCount;	// material
	

  //! constructor
	CSimpleIndexedMesh();

	//! free data 
	void FreeData();

	//! destructor
	~CSimpleIndexedMesh();


private: // ------------------------------------------------------------------------------

	// no copy constructor
	CSimpleIndexedMesh( CSimpleIndexedMesh &rIn )
	{ 
		assert(0);
	}

	// no assignment operator
	CSimpleIndexedMesh & operator=( CSimpleIndexedMesh &rIn )
	{ 
		assert(0);
	}

	//! /param indwTriID1 number of the first triangle
	//! /param indwTriID2 number of the second triangle
	//! /param outiSide1[2] the normal number if there is at least one vertex that is shared, -1 otherwise
	//! /param outiSide2[2] the other normal number if there are at least two vertices that are shared, -1 otherwise
	void GetVerticesBetweenTriangles( const DWORD indwTriID1, const DWORD indwTriID2, int outiSide1[2], int outiSide2[2] )
	{
		outiSide1[0]=-1;outiSide1[1]=-1;
		outiSide2[0]=-1;outiSide2[1]=-1;

		// from tri1 to tri2
		for(DWORD dwSide1=0;dwSide1<3;dwSide1++)
		{
			DWORD dwVertexNo1=m_pFaces[indwTriID1].v[dwSide1];

			for(DWORD dwSide2=0;dwSide2<3;dwSide2++)
			{
				if(dwVertexNo1==m_pFaces[indwTriID2].v[dwSide2])
				{
					if(outiSide1[0]==-1)outiSide1[0]=m_pFaces[indwTriID1].n[dwSide1];
						else outiSide1[1]=m_pFaces[indwTriID1].n[dwSide1];

					if(outiSide2[0]==-1)outiSide2[0]=m_pFaces[indwTriID2].n[dwSide2];
						else outiSide2[1]=m_pFaces[indwTriID2].n[dwSide2];
				}
			}
		}
	}

public:

	//! /param indwTriID1
	//! /param indwTriID2
	//! /return
	bool IsSmoothBetweenTri( const DWORD indwTriID1, const DWORD indwTriID2 )
	{
		if(!m_pFaces)return false;

		if(indwTriID1==indwTriID2)return true;

		if(!m_pNorms)
			return false;

		int iSide1[2],iSide2[2];

		GetVerticesBetweenTriangles(indwTriID1,indwTriID2,iSide1,iSide2);

		if(iSide1[0]!=-1)
		{
			assert(iSide2[0]!=-1);

			if(m_pNorms[iSide1[0]]!=m_pNorms[iSide2[0]])
				return false;

			if(iSide1[1]!=-1)
			{
				assert(iSide2[1]!=-1);

				if(m_pNorms[iSide1[1]]!=m_pNorms[iSide2[1]])
					return false;
			}
		}

		return true;
	}

private:
	static const char *RemovePath( const char *inszPathName )
	{
		const char *pEnd=inszPathName;
		while(*inszPathName!=0)
		{
			if(*inszPathName=='\\')pEnd=inszPathName+1;
			inszPathName++;
		}

		return(pEnd);
	}

public:

	// stupid but simple implementation
	void MarkDuplicatedMaterialNames()
	{
		for(uint32 i=0;i<m_MaterialCount;i++)
			for(uint32 e=0;e<i;e++)			// was already in array?
				if(m_pMaterials[i].m_sMaterialName==m_pMaterials[e].m_sMaterialName)
				{
					m_pMaterials[i].m_dwFlags|=1;		// mark as duplicate
					break;
				}
	}

	// currently smoothing groups and material ID's are not supported,
	// vertex position, vertex normal and uv texturing is supported
	bool ExportAsOBJ( const char *inszOBJPathName, const bool bWavefrontLikeLayout );

	void Debug() const;
};


#endif // __SIMPLEINDEXEDMESH2_H
