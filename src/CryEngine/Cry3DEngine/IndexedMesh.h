////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   meshidx.h
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: prepare shaders for object
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef IDX_MESH_H
#define IDX_MESH_H

#include "CryArray.h"
#include "CryHeaders.h"
#include "IIndexedMesh.h"

struct StatHelperInfo
{
	StatHelperInfo(char * new_name, int new_type, IShader *pShader, const Matrix34 & newMat)
	{
		//    vPos = new_pos;
		//    qRot = new_rot;
		strncpy(sName,new_name,64);
		nType = new_type;
		m_pShader = pShader;
		//  m_pObject = NULL;
		tMat = newMat;
	}

	//Vec3 vPos; CryQuat qRot;
	int nType;
	char sName[64];
	IShader *m_pShader;
	//  CRenderObject *m_pObject;
	Matrix34 tMat;
};

#define SFACE_FLAG_REMOVE 1

struct SGroupInfo
{
  SGroupInfo() { aabb.Reset(); } 
  PodArray<uint32> arrTris;
  AABB aabb;
};

struct SDecalProjectionInfo
{
  SDecalProjectionInfo() { ZeroStruct(*this); nLastHitTriId=-2; } 
  Matrix34 matObjInv;
  Matrix34 matObj;
  float fScale;
  IMaterial * pMat;
  CStatObj * pStatObj;
	int nLastHitTriId;
	int nLastHitMatId;
};

class CIndexedMesh : public IIndexedMesh, public CMesh, public Cry3DEngineBase
{
public:
	bool m_bInvalidated;

public:

	// constructor
  CIndexedMesh();
  ~CIndexedMesh();

	virtual void Release() { delete this; }

	// gives read-only access to mesh data
	VIRTUAL void GetMesh(SMeshDesc & MeshDesc)
	{
		MeshDesc.m_nCoorCount = m_nCoorCount;
		MeshDesc.m_nVertCount = m_numVertices;
		MeshDesc.m_nFaceCount = m_numFaces;
		MeshDesc.m_pFaces = m_pFaces;
		MeshDesc.m_pVerts = m_pPositions;
		MeshDesc.m_pNorms = m_pNorms;
		MeshDesc.m_pColor = m_pColor0;
		MeshDesc.m_pTexCoord = m_pTexCoord;
		MeshDesc.m_pIndices = m_pIndices;
		MeshDesc.m_nIndexCount = m_nIndexCount;
	}

	VIRTUAL int GetFacesCount() { return m_numFaces; }
	VIRTUAL int GetVertexCount() { return m_numVertices; }
	VIRTUAL int GetTexCoordsCount() { return m_nCoorCount; }

	VIRTUAL void Invalidate();

	VIRTUAL void SetFacesCount(int nNewCount)
	{
		CMesh::SetFacesCount( nNewCount );
	}

	VIRTUAL void SetVertexCount(int nNewCount)
	{
		CMesh::SetVertexCount( nNewCount );
	}

	VIRTUAL void SetColorsCount(int nNewCount)
	{
		CMesh::ReallocStream( COLORS_0,nNewCount );
	}

	VIRTUAL void SetTexCoordsCount(int nNewCount)
	{
		CMesh::SetTexCoordsCount( nNewCount );
	}

	VIRTUAL void SetTexCoordsAndTangentsCount(int nNewCount)
	{
		CMesh::SetTexCoordsAndTangentsCount( nNewCount );
	}

	VIRTUAL void SetIndexCount( int nNewCount )
	{
		CMesh::SetIndexCount(nNewCount);
	}

	VIRTUAL int GetIndexCount()
	{
		return m_nIndexCount;
	}

	VIRTUAL void AllocateBoneMapping()
	{
		m_pBoneMapping = (SMeshBoneMapping *)malloc(m_numVertices*sizeof(SMeshBoneMapping));
	}

	VIRTUAL void AllocateSHData()
	{
		m_pSHInfo = new SSHInfo;
		m_pSHInfo->pSHCoeffs = new SMeshSHCoeffs[m_numVertices];
		m_pSHInfo->pDecompressions = new SSHDecompressionMat[m_subsets.size()];
	}

	size_t GetMemoryUsage();

	void DoBoxTexGen(float fScale, Vec3 * pvNormal = NULL);
	void SimplifyMesh(float fMaxEdgeLen, const AABB & boxBoundary, float fSurfTypeError);
	float GetEdgeError(struct TriEdge & tEdge, Vec3 * pVertsShared, struct VertInfo * pVertsInfo, std::vector<int> &lstIndices, SMeshColor * pColorShared, SMeshColor * pColorShared1, float fSurfTypeError);
	void RebuildMesh(Vec3 * pVertsShared, SMeshTexCoord * pCoordShared,Vec3 * pNormsShared, 
		SMeshColor * pColorShared0, 
		SMeshColor * pColorShared1, 
		std::vector<int> &lstIndices, int nVertCountShared, int * pVertMatsShared);
	bool RemoveEdge(Vec3 * pVertsShared, struct VertInfo * pVertsInfo, SMeshTexCoord * pCoordShared,Vec3 * pNormsShared, 
		SMeshColor * pColorShared0, 
		SMeshColor * pColorShared1, 
    int * pVertMatsShared,
		std::vector<int> & lstIndices, int nVertCountShared,
		PodArray<struct TriEdge> & lstTriEdges, int nEdgeForRemoveId, const AABB & boxBoundary, float fSurfTypeError);
	void RemoveDuplicatedEdges(PodArray<struct TriEdge> & lstTriEdges);
	bool TestEdges(int nBeginWith, PodArray<TriEdge> & lstTriEdges, std::vector<int> & lstIndices, TriEdge * pEdgeForDelete);
	void MergeVertexFaceLists(int v0, int v1, struct VertInfo * pVertsInfo);
	bool InitEdgeFaces(TriEdge & e, VertInfo * pVertsInfo, Vec3 * pVertsShared, TriEdge * pe0, const AABB & boxBoundary);
	void MergeVertexComponents(TriEdge & e0, Vec3 * pVertsShared, struct VertInfo * pVertsInfo, 
		SMeshTexCoord * pCoordShared,Vec3 * pNormsShared, SMeshColor * pColorShared0, SMeshColor * pColorShared1, int * pVertMatsShared);
	void InitStreamSize();
  void ShareVertices(ISystem * pSystem, const AABB & boxBoundary);
  void ShareVerticesTC(ISystem * pSystem, const AABB & boxBoundary);


	VIRTUAL CMesh* GetMesh() { return this; };
	VIRTUAL void SetMesh( CMesh &mesh )
	{
		Copy( mesh );
	};

	VIRTUAL void SetSubSetCount( int nSubsets ) { m_subsets.resize(nSubsets); };
	VIRTUAL int GetSubSetCount() { return m_subsets.size(); };
	VIRTUAL SMeshSubset& GetSubSet( int nIndex ) { return m_subsets[nIndex]; };
	VIRTUAL void SetSubsetBoneIds( int idx, const PodArray<uint16> &boneIds ) { m_subsets[idx].m_arrGlobalBonesPerSubset = boneIds; }

	VIRTUAL void SetBBox( const AABB &box ) { m_bbox = box; };
	VIRTUAL AABB GetBBox() { return m_bbox; };
	VIRTUAL void CalcBBox();

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void Optimize( const char * szComment, std::vector<uint16> *pVertexRemapping=NULL,std::vector<uint16> *pIndexRemapping=NULL );
	virtual void SimplifyMesh( float fLevel );
  void RestoreFacesFromIndices();
  void CheckValid();
  static void MarkSharpVertices(float fMaxDot, CMesh * pMesh);
  int GenerateTextureCoordinates(int nTexSizePlanar, float fMeshNodeSize, int * pOutTexSizeX, int * pOutTexSizeY, bool bLog, void * node, void * pIdx, PodArray<IRenderNode*> * pItems, ColorB ** pTexDatas, int nTexNum, float fRayLen);
  void FillTexel(const Vec3 & vPos, Vec3 & vMinCache, SVoxValueCache & voxValueCache, ColorB * pPixel0, ColorB * pPixel1, ColorB * pPixel2, bool bBuildPixMats, bool bFilter);
  bool FillTexturesPlanar( int nTexSize, ColorB ** pTexDatas, int nTexNum, int & nPixelsDrawn, bool bBuildPixMats, bool bFilter );
  bool RasterizeTriangles( int nTexSizeX, int nTexSizeY, float fMeshNodeSize, 
    PodArray<SGroupInfo> & arrGroups, ColorB ** pTexDatas, int nTexNum, void* node, void* pIdx, class IsoOctree * isoTree, 
    PodArray<IRenderNode*> * pDecals, std::map<string,SImageSubInfo*> & imageInfos, float fRayLen, int &nTrisDrawn, int &nPixelsDrawn );
  void Render2dTriangle(const Vec2 * pVerts, int i1, int i2, int i3, 
     int nTexSizeX, int nTexSizeY, ColorB ** pTexDatas, int nTexNum, const SMeshFace & face, float fMeshNodeSize, void* node, void* pIdx, class IsoOctree * isoTree, 
    PodArray<SDecalProjectionInfo> & decalProjectionInfo, float fRayLen, int &nTrisDrawn, int &nPixelsDrawn );
  void mySwap(int & v0, int & v1) { int tmp = v0; v0 = v1; v1 = tmp; }
  void Render2dSubTriangle(	int x0, int y0, int x1, int y1, int x2, int y2, 
    int nTexSizeX, int nTexSizeY, ColorB ** pTexDatas, int nTexNum, const SMeshFace & face, float fMeshNodeSize, 
    void* node, void* pIdx, class IsoOctree * isoTree, PodArray<SDecalProjectionInfo> & decalProjectionInfo, float fRayLen, const Matrix33 & interpMat,
    int &nTrisDrawn, int &nPixelsDrawn);
  
  void WeldPositionsTC( Vec3 *pVertices, int &nVerts, 
    SMeshTexCoord * pTexCoords, int &nTC,
    std::vector<int> &indices,float fEpsilon, const AABB & boxBoundary );

  template <class T> bool GetBarycentricCoordinates(T P_A, T B_A, T C_A, float & u, float & v, float & w, float fBorder)
  {
    // Compute vectors        
    const T & v0 = C_A;
    const T & v1 = B_A;
    const T & v2 = P_A;

    // Compute dot products
    float dot00 = v0.Dot(v0);
    float dot01 = v0.Dot(v1);
    float dot02 = v0.Dot(v2);
    float dot11 = v1.Dot(v1);
    float dot12 = v1.Dot(v2);

    // Compute barycentric coordinates
    float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);
    u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    w = 1.f - u - v;

    // Check if point is in triangle
    return (u >= -fBorder) && (v >= -fBorder) && (w >= -fBorder);
  }

  static void ProcessMaterial(IMaterial * pMat, int nHitMatID, const Vec4 & vertCol, const Vec2 & vDecalTC, int nMip, ColorF & colSumm, Vec3 & vSummNorm,
    const Vec4 & vHitTangent, const Vec4 & vHitBinormal, const Matrix34 * pObjMatrix);
};

#endif // IDX_MESH_H
