#ifndef VOXTERRAIN_H
#define VOXTERRAIN_H

#define VOXNODE_MESH_CHUNK_VERSION 1

#include "VoxMan.h"

/*class CVoxDataNode : public Cry3DEngineBase
{
protected:
  CVoxDataNode * m_arrChilds[8];
  float m_fIsoVal;
  float m_fIsoValPrev;
  short m_nMatId;

public:
  CVoxDataNode(short nMatId);
  ~CVoxDataNode();
  float GetVoxelValueF(Vec3 vPos, float fGridSize, short * pMatId, bool bPrev, const AABB & nodeBox);
  void DoVoxelShape(Sphere sp, EVoxelBrushShape eShape, EVoxelEditOperation eOperation, int nMatId, const AABB & nodeBox, int nDepth, int nMaxDepth);
  static AABB GetChildBBox(const AABB & parentBox, int nChildId);
  void ReleaseChilds();
  void UpdateStateFromChilds(int nMatId, EVoxelEditOperation eOperation);
  float GetElevation(Vec3 vPos, float & fMaxZ, const AABB & nodeBox);
  CVoxDataNode * GetMinNodeContainingAABB(const AABB & box, const AABB & nodeBox);
  void Submit(Sphere sp, EVoxelBrushShape eShape, const AABB & nodeBox);
  int GetTreeDepth(Vec3 vPos, int nDepth, const AABB & nodeBox);
  bool HasChilds();
  void RenderDebug(const AABB & nodeBox);

  // serialization
  int Save(byte * & pData, int & nDataSize, EEndian eEndian);
  int Load(FILE * & f, int & nDataSize, EEndian eEndian);
  int Load(uint8 * & f, int & nDataSize, EEndian eEndian);
  template <class T> int Load_T(T * & f, int & nDataSize, EEndian eEndian);
};*/
/*
class CVoxMeshNode : public Cry3DEngineBase
{
protected:
  AABB m_nodeBox;
  CVoxMeshNode * m_arrChilds[8];
  class CVoxelObject * m_pVoxObj;
  CVoxMeshNode * m_pParent;
  float m_fNodeSize;

public:
  CVoxMeshNode(AABB aabb, CVoxMeshNode * pParent);
  ~CVoxMeshNode();
  float GetNodeSize() { return m_fNodeSize; }
  void Render();
  void UpdateMesh();
  void InitVoxelObject();
  void ReleaseChilds();
  void UpdateStateFromChilds(int nMatId, EVoxelEditOperation eOperation);
  void InitVoxelObjectFromChilds();
  void ScheduleRebuild();
  void RequestUpdate(Sphere sp, EVoxelBrushShape eShape);
  void FillShadowCastersList(bool bAllIn, CDLight * pLight, ShadowMapFrustum * pFr, PodArray<SPlaneObject> * pShadowHull, bool bUseFrustumTest, int nMaxReqursion);
  float GetElevation(Vec3 vPos, float & fMaxZ);
  uint32 GetRebuildFrameId();
  const AABB & GetNodeAABB() { return m_nodeBox; }
  int GetTreeDepth(Vec3 vPos, int nDepth);
  bool HasChilds();
  AABB GetChildBBox(int nChildId);
  void CheckAllocate(Sphere sp, EVoxelBrushShape eShape, int nDepth, int nMaxDepth);
  bool CleanUpTree();

  // serialization
  int SaveMesh(class CMemoryBlock * pMemBlock, EEndian eEndian);
  int SaveTexture(CMemoryBlock * pMemBlock, EEndian eEndian);
  void LoadMesh(class CMemoryBlock * pMemBlock, EEndian eEndian, bool bDataIsAlligned);
  void LoadTexture(CMemoryBlock * pMemBlock, EEndian eEndian);
  int Save(byte * & pData, int & nDataSize, bool bSaveMesh, EEndian eEndian);
  int Load(FILE * & f, int & nDataSize, bool bUpdateMesh, EEndian eEndian);
  int Load(uint8 * & f, int & nDataSize, bool bUpdateMesh, EEndian eEndian);
  template <class T> int Load_T(T * & f, int & nDataSize, bool bUpdateMesh, EEndian eEndian);
};
*/

class CVoxTerrain : public IVoxTerrain, public Cry3DEngineBase
{
  struct VertInfo
  {
    PodArray<int> lstFacesIds;
  };

public:
  // interface
  VIRTUAL bool SetCompiledData(byte * pData, int nDataSize, bool bUpdateMesh, EEndian eEndian, AABB * pAreaBox, int nSID);
  bool Load(FILE * f, int nDataSize, SIsoTreeChunkHeader * pTerrainChunkHeader, int nSID);
  VIRTUAL bool GetCompiledData(byte * pData, int nDataSize, bool bSaveMesh, EEndian eEndian, bool bSaveForEditing, AABB * pAreaBox, int nSID);
  VIRTUAL int  GetCompiledDataSize(bool bSaveMesh, bool bSaveForEditing, AABB * pAreaBox, int nSID);
  VIRTUAL IMemoryBlock * GetCompiledData(bool bSaveMesh, EEndian eEndian, bool bSaveForEditing, AABB * pAreaBox, int nSID);
  VIRTUAL void DrawEditingHelper(const Sphere & sp, EVoxelEditOperation eOperation, IMaterial * pHelperMat);
  VIRTUAL bool RayIntersection(const Ray & r, Vec3 & vHitPoint);
	VIRTUAL void OnMouse(bool bUp);
	VIRTUAL PodArray<IRenderNode*> * GetNodesForUpdate() { return &m_arrNodesForUpdate; }

  CVoxTerrain( const SVoxTerrainInfo & info );
  ~CVoxTerrain();
  template <class T> bool Load_T(T * & f, int & nDataSize, SIsoTreeChunkHeader * pTerrainChunkHeader, bool bUpdateMesh, EEndian eEndian, AABB * pAreaBox, int nSID);
  void Render();
  void DoVoxelShape(Vec3 vWSPos, float fRadius, int nMatId, ColorB color, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, PodArray<IRenderNode*> * pBrushes, float fVoxelSize, AABB * pUpdateArea = NULL);
  void FillShadowCastersList(CDLight * pLight, ShadowMapFrustum * pFr, PodArray<SPlaneObject> * pShadowHull, bool bUseFrustumTest, int nMaxReqursion);
  float GetElevation3D(Vec3 vPos, Vec3 & vNorm);

  static void GetVoxelValueInterpolated(class IsoOctree * isoTree, Vec3 vPos, ColorF & resColor, void* node, void* pIdx, SSurfTypeInfo & surf_type, Vec3 & vNormal, struct SVoxValueCache * pVVC);
  static void GetVoxelColorInterpolated(class IsoOctree * isoTree, Vec3 vPos, ColorB & resColor, void* node, void* pIdx);
  void ResetHeightMapCache();

  void DrawDebug(int nRes);
  void RasterizeVolume(const int nRes);
  Vec3 ProjectFromScreen(const float & x, const float & y, const float & z, int nRes);

  static PodArray<IDynTexture*> m_arrTexturesForRel;
  static PodArray<struct SIsoMesh*> m_arrIsoMeshesForDel;

  static void AddTextureForRelease(IDynTexture * pTex);
  static void ReleaseIsoMeshFromMainThread(SIsoMesh * pMesh, ISO_KEY key);

#define ISO_THREADS_NUM 16
  struct CIsoOctreeThread * m_pIsoOctreeThreads[ISO_THREADS_NUM];

  static float m_fMapSize;

  void GetTrianglesInAABB(const AABB & aabb, PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices);
  void GetTrianglesInSphere(const Sphere & sp, PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices);
  Vec3 GetTerrainSurfaceNormal3D(Vec3 vPos, float fRange);
  char * GetStatusString(int nLine);
  void GetMemoryUsage(ICrySizer * pSizer);
  static void PolygonToTriangleMeshForPhysics(const std::vector<Vec3>& vertices, const std::vector<SSurfTypeInfo>& surf_types, const std::vector< std::vector<int> > & polygons, std::vector<uint16> & indices);
  static void PolygonToTriangleMeshForRendering(const std::vector<Vec3>& vertices, const std::vector<SSurfTypeInfo>& surf_types, const std::vector< std::vector<int> > & polygons, std::vector<TriangleIndex> & triangles, int nIn);
  void RegisterLightSource(CDLight * pLight);
  void RequestTextureUpdate(AABB areaBox);
  void RequestMeshUpdate(AABB aabb);
  void RequestPhysicsUpdate();
	void ProcessNodesForUpdate();
  void ImportUPC();
  
  static void DrawStreamingActivity();
  static void LogStreamingActivity(int nSeek);
  static void SetStreamingActivityMax(int nSeekMax);
  static const int nActLogRes = 256;
  static int nSeekMax;
  static int32 arrActLog[nActLogRes];

  struct SElevCacheItem
  {
    const bool IsEquivalent2D(const Vec3& v0, const Vec3& v1, f32 epsilon ) const 
    { return  ((fabs_tpl(v0.x-v1.x) <= epsilon) &&	(fabs_tpl(v0.y-v1.y) <= epsilon)); }
    const bool operator == (const SElevCacheItem & o) const
    { return IsEquivalent(vPos, o.vPos, .1f); }
    Vec3 vPos, vNor;
  };

  PodArray<SElevCacheItem> m_arrElevCache;
  int m_nElevCacheLastFrameId;

  static bool IsEditing();

  void PhysicalizeContentOfBBox(const AABB & areaBox);
  void DephysicalizeContentOfBBox(const AABB & areaBox);
  bool IsContinue();
  static bool m_bAllowEditing;
  IRenderMesh * m_pEditorHelper;
  bool m_bModified;
  FILE * m_pDataFile;
  int m_nMeshesStreamedIn;
  int m_nReMeshTexStreamedIn;
  int m_nMeshAmountStreamedIn;

#define INDEX_TARGETS_NUM 8

  struct STargetMipData
  { Array2d<Vec4_tpl<uint16> > arrData; };

  CVoxTerrain::STargetMipData m_arrTargetIndex[INDEX_TARGETS_NUM];
  Array2d<uint8> m_arrTargetCounter;
  int m_arrTargetTexId[INDEX_TARGETS_NUM+1];

  static int m_nSelLayer;
  static float m_fSelLayerTime;
  static float m_arrTimeStats[20];

	PodArray<IRenderNode*> m_arrNodesForUpdate;

  Array2d<SSurfTypeInfo> m_arrSurfType2D;
  SSurfTypeInfo GetLayerAtPosition( const int x, const int y );
  void PaintLayerId( const float fpx, const float fpy, const float radius, const float hardness, const uint32 dwLayerId );
  void SetLayerData(void * pData, int nDataSize);
  void GetLayerData(void ** pData, int & nDataSize);
};

#endif // VOXTERRAIN_H
