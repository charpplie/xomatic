/*
Copyright (c) 2007, Michael Kazhdan
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution. 

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/
#ifndef ISO_OCTREE_INCLUDED
#define ISO_OCTREE_INCLUDED

#include <map>
#include "MarchingCubes.h"
#include "Octree.h"
#include "Cry3DEngineBase.h"

class EdgeKey
{
public:
	size_t key1,key2;
	EdgeKey(void){;}
	EdgeKey(const size_t& n1,const size_t &n2);

	EdgeKey& operator = (const EdgeKey& key);
	operator size_t () const;
	operator size_t ();
	bool operator < (const EdgeKey& key) const;

	bool operator != (const EdgeKey& key) const;
};

class NeighborKey : public OctNode::NeighborKey
{
	void __GetCornerNeighbors(OctNode* node,const int& depth,const int& c,OctNode* neighbors[Cube::CORNERS]);
	OctNode* __FaceNeighbor(OctNode* node,const int& depth,int dir,int off);
	OctNode* __EdgeNeighbor(OctNode* node,const int& depth,int o,int i1,int i2);
	OctNode* __CornerNeighbor(OctNode* node,const int& depth,int x,int y,int z);
public:
	void GetCornerNeighbors(OctNode* node,const int& c,OctNode* neighbors[Cube::CORNERS]);
	OctNode* FaceNeighbor(OctNode* node,int dir,int off);
	OctNode* EdgeNeighbor(OctNode* node,int o,int i1,int i2);
	OctNode* CornerNeighbor(OctNode* node,int x,int y,int z);

	static void CornerIndex(const int& c,int idx[3]);
	static void EdgeIndex(const int& c,int idx[3]);
	static void FaceIndex(const int& c,int idx[3]);
};

//typedef std::hash_map<ISO_KEY,  SVoxValue, std::hash<ISO_KEY> > TVoxValues;
// simple hash class that has the avalanche property (a change in one bit affects all others)
// ... use this if you have uint32 key values!
class IsoOctTree_hash_uint64
{
public:
	enum {	// parameters for hash table
		bucket_size = 4,	// 0 < bucket_size
		min_buckets = 256 // min_buckets = 2 ^^ N, 0 < N
	};

	ILINE size_t operator()( ISO_KEY key ) const
	{
		key = (~key) + (key << 18); // key = (key << 18) - key - 1;
		key = key ^ (key >> 31);
		key = key * 21; // key = (key + (key << 2)) + (key << 4);
		key = key ^ (key >> 11);
		key = key + (key << 6);
		key = key ^ (key >> 22);
		return (size_t) key;
		/*
		key = (key+0x7ed55d16) + (key<<12);
		key = (key^0xc761c23c) ^ (key>>19);
		key = (key+0x165667b1) + (key<<5);
		key = (key+0xd3a2646c) ^ (key<<9);
		key = (key+0xfd7046c5) + (key<<3);
		key = (key^0xb55a4f09) ^ (key>>16);
		return (size_t) key;
		*/    
	};
	ILINE bool operator()( const ISO_KEY& key1,const ISO_KEY& key2 ) const
	{
		return key1 < key2;
	}
};

typedef stl::hash_map<ISO_KEY,  SVoxValue, IsoOctTree_hash_uint64> TVoxValues;
typedef std::map<ISO_KEY,  SIsoMesh*> TIsoMeshes;
typedef std::map<ISO_KEY,  int> TRoots;
typedef std::map<ISO_KEY, SFlatnessItem> TFlatness;

struct SNodeUpdateRequest
{
  int nRequestFrameId;
  int nRequestPrioriry;
  float fNodeSize;
  OctNode::NodeIndex nIdx;
  float fPredictionRatio;
  ISO_KEY centerKey;
  bool bInProgress;
  void UpdateSortVal_HQ(int nMainFrameId, const Vec3 & vCamPosNorm, TIsoMeshes & threadMeshes);
};

const int nResIndex = 256;
const int nResData = 2048;
const int nNodeDataSize = 64;

struct SOcNodeInfo
{
  Vec4_tpl<uint16> nodeBox;
  Vec4_tpl<uint16> nodeValues[2];
  Vec4_tpl<uint16> nodeEncodedNormals[2];
  Vec4_tpl<uint16> arrLeafData[4][4];
  Vec4_tpl<uint16> arrLeafNorm[4][4];
  Vec4_tpl<uint16> lastUsedFrameId;
  Vec4_tpl<uint16> szAligh[nNodeDataSize-22-16];
};

class IsoOctree : public Cry3DEngineBase
{
	class RootInfo
	{
	public:
		const OctNode* node;
		int edgeIndex;
		ISO_KEY key;
		OctNode::NodeIndex nIdx;
	};
	
	class MeshInfo
	{
	public:
		std::vector<Vec3 > vertexNormals;
		std::map<EdgeKey,Vec3 > edgeNormals;
		std::vector<Vec3 > triangleNormals;
		std::vector<TriangleIndex> triangles;
		std::vector<Vec3 > vertices;
    AABB meshBox;
		void set(const std::vector<Vec3>& vertices,const std::vector<std::vector<int> > & polygons,const float& width,
			Vec3& translate,float& scale,const int& noTransform);
	};

	void getRoots(OctNode* node,const OctNode::NodeIndex& nIdx,TRoots& roots,std::vector<Vec3>& vertices,std::vector<SSurfTypeInfo> * surf_types, std::vector<ColorB> * colors);
	int getRootIndex(OctNode* node,const OctNode::NodeIndex& nIdx,const int& edgeIndex,RootInfo& ri);
	int getRootPosition(const OctNode* node,const OctNode::NodeIndex& nIdx,const int& eIndex,Vec3& position,SSurfTypeInfo& surf_type,ColorB & color);
	ISO_KEY getRootKey(const OctNode::NodeIndex& nIdx,const int& edgeIndex);
	int getRootPair(const RootInfo& root,const int& maxDepth,RootInfo& pair);
	void getIsoFaceEdges(OctNode* node,const OctNode::NodeIndex& nIdx,const int& faceIndex,std::vector<std::pair<RootInfo,RootInfo> > & edges,const int& flip,const int& useFull);
	void getIsoPolygons(OctNode* node,const OctNode::NodeIndex& nIdx,TRoots& roots,std::vector<std::vector<int> > & polygons,const int& useFull);
	template<class C>
	void getEdgeLoops(std::vector<std::pair<C,C> > & edges,TRoots& roots,std::vector<std::vector<int> > & polygons);
	template<class C>
	void getEdgeLoops(std::vector<std::pair<C,C> > & edges,std::vector<std::vector<C> > & loops);
	bool setDistanceFromMesh(const std::vector<int>& triangles,MeshInfo& mInfo,const Vec3& p,	ISO_KEY key, int nSurfaceType, const ColorB & color, const EVoxelEditOperation & eOperation, TRoots * pVisitedValues);	
	void setChildrenFromMesh(int nSurfaceType, const ColorB & color, OctNode* node,const OctNode::NodeIndex& nIdx,
    const std::vector<int>& trianglesForThisNode,
    const std::vector<int>& trianglesForFromParent,
    MeshInfo& mInfo,const int& maxDepth,const int& setCenter,const float& flatness,
    const SBrushAABB * pBrushAABB, TVoxValues * pValuesBk, const EVoxelEditOperation & eOperation,
    bool bSetValues, TRoots * pVisitedValues);
	void setMCIndex(const int& useFull);

public:

  static int maxDepth;
  int editDepth;
  static int nMeshDepth;
  NeighborKey nKey;
  static AABB m_aabbDirty;
  static AABB m_aabbPhysDirty;
  int m_nSID;

	// The octree itself
	OctNode tree;
  OctNode renderTree;

  TVoxValues * m_pValues;
  TVoxValues m_tmpValues;
  TIsoMeshes threadMeshes;
  TIsoMeshes threadEmptyMeshes;
  TIsoMeshes threadEmptyMeshesDelta;
  std::map<int,struct SIsoMesh*> physMeshes;
  PodArray<SNodeUpdateRequest> m_nodesForUpdate;
  PodArray<byte> m_loadingCache;
  static TFlatness m_flatnessMap;

  int m_nProgress;
  int m_nCurrentNodeSlotId;

  static float m_lastEditingTime;

	// Sets an octree from a polygon mesh, generating an octree that is fully refined around the surface
	int setFromMesh(const std::vector<Vec3>& vertices,const std::vector<std::vector<int> > & polygons,const int& maxDepth,const int& setCenter,const float& flatness,const int& noTransform, int nSurfaceType, const ColorB & color, const SBrushAABB * pBrushAABB, const EVoxelEditOperation & eOperation);	
	int setFromMesh(const std::vector<Vec3>& vertices,const std::vector<std::vector<int> > & polygons,const int& maxDepth,const int& setCenter,const float& flatness,Vec3& translate,float& scale,const int& noTransform, int nSurfaceType, const ColorB & color, const SBrushAABB * pBrushAABB, const EVoxelEditOperation & eOperation);
	// Sets an octree from a polygon mesh, generating an octree that is fully refined around the surface	
//	int setConforming(const std::vector<Vec3>& vertices,const std::vector<std::vector<int> > & polygons,const int& maxDepth,const int& setCenter,const float& flatness,const int& noTransform);
//	int setConforming(const std::vector<Vec3>& vertices,const std::vector<std::vector<int> > & polygons,const int& maxDepth,const int& setCenter,const float& flatness,Vec3& translate,float& scale,const int& noTransform);

	// A clean-up method to remove un-needed entries in the cornerValues hash-table
	void CleanUpValues();

  template <class T> 
  bool Load_T(T * & f, int & nDataSize, SIsoTreeChunkHeader * pTerrainChunkHeader, bool bUpdateMesh, EEndian eEndian, AABB * pAreaBox, int nSID);

  // Writes the octree to a file pointer
  int GetCompiledData(byte * & pData, int & nDataSize, EEndian eEndian, bool bSaveMesh, bool bSaveForEditing, AABB * pAreaBox);
  int GetCompiledDataSize(bool bSaveMesh, bool bSaveForEditing, AABB * pAreaBox);

	// Extracts an iso-surface from the octree	
  void getIsoSurface(
    std::vector<Vec3>& vertices,
    std::vector<SSurfTypeInfo> *surface_types,
    std::vector<ColorB> *colors,
    std::vector<std::vector<int> > * pPolygonsIn,
    std::vector<std::vector<int> > * pPolygonsOut,
    const int& useFull,
    const AABB * pAreaBox,
    bool bSetMCIndex);

	void _getIsoSoup(std::vector<Vec3>& vertices,
    std::vector<int>& surf_types, std::vector<ColorB>& colors,std::vector<std::vector<int> > & polygons,const int& useFull);	
//	void getDualIsoSurface(std::vector<Vec3>& vertices,std::vector<std::vector<int> > & polygons,const int& useFull);
	// Generates a hash-table indexed by octree node, with each entry containing two pieces of data. The first is
	// mean-curvature vector associated to the intersection of the iso-surface with the node. The second is the area
	// of the intersection of the iso-surface with the node.
	void setNormalFlatness(TFlatness & flatnessMap);
	// A method for determing if a node has grand-children along an edge
	static int HasEdgeGrandChildren(const OctNode* node,int eIndex);
	// A method for determing if a node has grand-children along a face
	static int HasFaceGrandChildren(const OctNode* node,int fIndex);

  void DrawDebugValues(OctNode* node, const OctNode::NodeIndex& nIdx);
  void Render(const AABB & aabb);
  bool Render(OctNode* node, const OctNode::NodeIndex& nIdx);
  void ActivateNodes(OctNode* node, const OctNode::NodeIndex& nIdx, AABB parentBoxWS, CTerrainNode * pTerrainNode);
  SIsoMesh * GenerateIsoMeshForPhysics(const OctNode::NodeIndex& nIdx, bool bLog, const AABB & clipBox);
  SIsoMesh * GenerateIsoMeshForRendering(const OctNode::NodeIndex& nIdx, bool bLog, SIsoMesh * pParentMesh, void * node, void * pIdx, char * szCurrentMessageIn);
  void GetMeshes(OctNode* node, const OctNode::NodeIndex& nIdx, TIsoMeshes & foundMeshes);
  bool GetTrianglesInAABB(OctNode* node, const OctNode::NodeIndex& nIdx, const AABB & aabb,
    PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices);
  bool GetTrianglesInSphere(OctNode* node, const OctNode::NodeIndex& nIdx, const Sphere & sp,
    PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices);

  int setFromSphere(const Sphere & sp,
    const int& maxDepth,const int& setCenter,const float& flatness,EVoxelEditOperation eOperation, bool forceConforming, int surf_type, ColorB color, const SBrushAABB * pBrushAABB, IsoOctree * isoTreeBk);

  void setDistanceAndNormalFromSphere(const Sphere & sp,const Vec3& p,float& dist,ISO_KEY key, EVoxelEditOperation eOperation, int nSurfType, ColorB color, IsoOctree * isoTreeBk, TRoots * pVisitedValues);
  void setChildrenFromSphere(OctNode* node,
    const OctNode::NodeIndex& nIdx,
    const Sphere & sp,
    const int& maxDepth,const int& setCenter,
    const float& flatness,
    EVoxelEditOperation eOperation,
    CTerrainNode * pTerrainNode,
    int surf_type, ColorB color, 
    const SBrushAABB * pBrushAABB, IsoOctree * isoTreeBk, TRoots * pVisitedValues);
  bool GetValueByPosition(OctNode* node, const OctNode::NodeIndex& nIdx, const Vec3 & vPos, float * pVal, int nMaxDepth);
  OctNode * GetValueByPosition_BIN(OctNode* node, const OctNode::NodeIndex& nIdx, const Vec3 & vPos, int nMaxDepth);
  bool GetMaterialByPosition(OctNode* node, const OctNode::NodeIndex& nIdx, Vec3 vPos, ColorF & resColor, SSurfTypeInfo & resSType, Vec3 & resNormal, SVoxValueCache * pVoxValueCache);
  bool GetColorByPosition(OctNode* node, const OctNode::NodeIndex& nIdx, Vec3 vPos, ColorB & resColor);

  void LimitDepth(OctNode* node, const OctNode::NodeIndex& nIdx, const Sphere & sp, const SBrushAABB * pBrushAABB);
  
  IsoOctree(TVoxValues * pValues);
  ~IsoOctree();
  void ShutDown();

  void interpolateChilds(const OctNode::NodeIndex& nIdx, OctNode*tmp, TVoxValues * pSrcValues, TVoxValues * pDstValues);
  void interpolateChildsNormals(const OctNode::NodeIndex& nIdx, OctNode*temp, TFlatness * pSrcValues, TFlatness * pDstValues);

  bool FillShadowCastersList(OctNode* node, const OctNode::NodeIndex& nIdx,
    bool bAllIn, CDLight * pLight, ShadowMapFrustum * pFr, PodArray<SPlaneObject> * pShadowHull, bool bUseFrustumTest);

  float GetElevation3D(OctNode* node, const OctNode::NodeIndex& nIdx, Vec3 vPos, float & fMaxZ, Vec3 & vMaxZNorm);

  void CutChilds(OctNode* node, const OctNode::NodeIndex& nIdx, std::map<OctNode*,OctNode*> & detachedChilds, int nMaxDepth);
  void RestoreChilds(std::map<OctNode*,OctNode*> & detachedChilds);
  void CutOutsideOfBox(const AABB & aabbCut, OctNode* node, const OctNode::NodeIndex& nIdx, std::map<OctNode*,OctNode*> & detachedChilds);
  void CollectValues(OctNode* node, OctNode::NodeIndex nIdx, TVoxValues & outValues, const AABB * pAreaBox);
  
//  bool CheckDepth(OctNode* node, const OctNode::NodeIndex& nIdx, int nDepth);

  void OptimizeTree(AABB * pAABB, float fCurvature, std::map<OctNode*,OctNode*> * pDetachedChilds, int nTopLevelToKeep, bool bRebuildFlatnessMap);
  int GetEditDepth();
  void CleanUpMeshes(bool bRemoveAll);
  void UpdateTree();
  void ActivateNode(const OctNode::NodeIndex& nIdx, int nPriority);
  static int Cmp_OctNode_OLD(const SNodeUpdateRequest &p1, const SNodeUpdateRequest &p2);
  enum ENodeClipState
  {
    eHasImportantDetails=0,
    eNotEmpty=1,
    eEmpty=2,
  };
  static ENodeClipState IsNodeClippable(const IsoOctree& isoTree,OctNode* node,const OctNode::NodeIndex& nIndex,
    TFlatness & flatnessMap,const float& clipValue,const int& forceConforming);
  static AABB GetNodeAABB(const OctNode::NodeIndex& nIdx, bool bWS = false);
  void ForceDepth(OctNode* node, const OctNode::NodeIndex& nIdx, const Sphere & sp, const SBrushAABB * pBrushAABB, EVoxelEditOperation eOperation, CTerrainNode * pTerrainNode);
  static void CopyTree(const AABB * pAabbCutIn, const AABB & aabbCutOut, int nMaxDepth, OctNode* dstNode, const OctNode::NodeIndex& nIdx, OctNode* srcNode, bool bCopyOnlyIfMeshExist, const IsoOctree * pIsoTree, float fLodCurvature, PodArray<std::pair<ISO_KEY, SVoxValue> > * pColValues);
  bool TestGetElevationChilds(OctNode* node, const OctNode::NodeIndex& nIdx, const Ray & ray);
  int GetProgress();
  void GetMemoryUsage(ICrySizer * pSizer);
  void SaveLayerImageInfo(byte * & pData, int & nDataSize, EEndian eEndian, SImageInfo * pImgInfo);
  void GetSubImageData(byte * & pData, int & nDataSize, EEndian eEndian, SImageSubInfo * pImgSubInfo, const char * pName, bool bSaveImgData);
  int GetSubImageDataSize(SImageSubInfo & subInfo, bool bSaveImgData) const;
  void RegisterLightSource(OctNode* node, const OctNode::NodeIndex& nIdx, class CDLight * pLight);
  void RequestTextureUpdate(OctNode* node, const OctNode::NodeIndex& nIdx, const AABB & areaBox);
  static int GetEngineMemoryUsageMB();
  SIsoMesh * PhysicalizeBox(const OctNode::NodeIndex& nIdx, const AABB & areaBox);
  bool ClipTriangle(std::vector<uint16> & indices, std::vector<Vec3> & vertices, std::vector<SSurfTypeInfo> & surf_types , int nStartIdxId, Plane * pPlanes, int nPlanesNum);
  void ClipPolygon(PodArray<Vec4> * pPolygon, const Plane & ClipPlane);
  int ClipEdge(const Vec4 & v1, const Vec4 & v2, const Plane & ClipPlane, Vec4 & vRes1, Vec4 & vRes2);
  void RegisterBBoxInPODGrid(OctNode* node, const OctNode::NodeIndex& nIdx);
  void ClipMeshByAABB(const AABB & clipBox, std::vector<uint16> & indices, std::vector<Vec3> & vertices, std::vector<SSurfTypeInfo> & surf_types);
  void ClipMeshByAABB(const AABB & clipBox, std::vector<TriangleIndex> & tris, std::vector<Vec3> & vertices, std::vector<SSurfTypeInfo> & surf_types);
  void AddBrushesIntoMesh( const AABB & nodeBoxWS,
    std::vector<TriangleIndex> & triangles,
    std::vector<Vec3> & vertices,
    std::vector<SSurfTypeInfo> & surf_types);
  static void AddStatObjIntoMesh(Matrix34 objMat, IRenderMesh * pRM, IMaterial * pMat,
    const AABB & nodeBoxWS,
    std::vector<TriangleIndex> & triangles,
    std::vector<Vec3> & vertices,
    std::vector<SSurfTypeInfo> & surf_types,
    int nSurfType);
  void DoVoxelShapeTask(Vec3 vWSPos, float fRadius, int nSurfaceTypeId, ColorB color, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, int nDetailLevel, const char * szCGF,
    std::vector<Vec3> * vertices, std::vector<int> * indices, AABB meshBox, int nProgressCur, int nProgressAll, float editingTime, bool bAllowRequestTreeUpdate);
  bool PrintProgress();
  bool RequestMeshUpdate(OctNode* node, const OctNode::NodeIndex& nIdx, const AABB & areaBox, const AABB & brushBox);
  bool RayIntersection(OctNode* node, const OctNode::NodeIndex& nIdx, const Ray & r, Vec3 & vHitPoint);
  void ActivateStreaming(OctNode* node, const OctNode::NodeIndex& nIdx, const AABB & parentBoxWS);
  int ReconstructTree(int nMaxDepth, OctNode* node, const OctNode::NodeIndex& nIdx);
  void PreloadCachedMeshes(OctNode* node, const OctNode::NodeIndex& nIdx);
  void ResetPhysics();
  bool IsVisibleFromGamePlayArea(AABB WSBox, float fScale = 6.f);
  bool RayIntersectionNew( Ray r, Vec3 & vHitPos );
  bool RayIntersectionChilds(  float tx0, float ty0, float tz0, float tx1, float ty1, float tz1, OctNode *n );
  int RayIntersectionChildsFindFirstNode(float& tx0, float& ty0, float& tz0, float& txm, float& tym, float& tzm) const;
#ifdef VOX_DVR
  void RasterizeVolume(OctNode* node, const OctNode::NodeIndex& nIdx, bool bNotEmpty, bool bAllIn);
#endif
  Vec3 ProjectToScreen(const Vec3 & vIn, float * pMatrix);
  void FillCornerValues( const OctNode::NodeIndex& nIdx,  float * values );
  void FillCornerValues( const OctNode::NodeIndex& nIdx,  Vec4_tpl<uint16> * nodeValues, Vec4_tpl<uint16> * nodeEncodedNormals, Vec3 & vAverNormal, int & nSurfType, bool & bUseRemesh );
  static bool DeleteFolder(const char *szFolder,bool bRecurse);
	void DeleteTooNewCacheItems();
  static void CheckMemoryLimit();
  void UpdateNormals(OctNode* node, const OctNode::NodeIndex& nIdx, TFlatness & flatnessMap);
};

#include "IsoOctree.inl"

#endif // ISO_OCTREE_INCLUDED
