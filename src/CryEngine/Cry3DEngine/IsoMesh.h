#ifndef ISOMESH_H
#define ISOMESH_H

struct SIsoMesh : public IRenderNode, public IStreamCallback, public Cry3DEngineBase
{
  static PodArray<SIsoMesh*> * GetCounter()
  {
    static PodArray<SIsoMesh*> arrAll;
    return &arrAll;
  }

  SIsoMesh(ISO_KEY key);
  ~SIsoMesh ();

  IRenderMesh * CreateRenderMeshFromIndexedMesh(CMesh * pMatMesh);
  void MakeRenderMesh();
  bool InitMaterials(int nSID);
  void LoadMeshFromMemBlock( CMemoryBlock * pMemBlock, CMesh * pMesh );
  bool BuildTextures(bool bSerializeTextures);
  void ReleaseTextures(bool bReleaseOnlyFinalTexture = false);

  void PhysicalizeMesh(CMesh * pMesh, std::vector<char> * physData);
  virtual void Dephysicalize( bool bKeepIfReferenced=false );

  byte * GetCompiledMeshData(int & nSizeOut, EEndian eEndian, bool bSaveForEditing, int nSectorId);
  int GetCompiledMeshDataSize(bool bSaveForEditing, int nSectorId);
  void SerializeMesh( CMesh * pMesh );
//  bool SaveIndexedMeshToFile( CMesh * pMesh, const char *sFilename, IChunkFile** pOutChunkFile );
  void ResetRenderMeshs();
  void BildIndexedMeshForPhysics( std::vector<uint16> & indices, 
    const std::vector<Vec3> & vertices, const std::vector<SSurfTypeInfo> & surf_types);
  void BildIndexedMeshForRendering(const std::vector<TriangleIndex> & triangles, const std::vector<Vec3> & vertices, 
    const std::vector<SSurfTypeInfo> & surf_types, const std::vector<ColorB> & colors, 
    float fPosScale, const AABB & WSBBox, bool bLog, bool bCopiedFromParent,
    void * node, void * pIdx, char * szCurrentMessageIn);
  void GetTrianglesInAABB(const AABB & aabb, PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices);
  void GetTrianglesInSphere(const Sphere & sp, PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices);
  void EnablePhysEntity(bool bEnable);
  int GetTexDimInternal();

  // interface IStreamCallback -----------------------------------------------------

  virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
  virtual void StreamOnComplete (IReadStream* pStream, unsigned nError);
  void StartStreaming( bool bFinishNow, float fTime );
  IReadStreamPtr m_pReadStream;
  EFileStreamingStatus m_eStreamingStatus;
  bool m_bUseStreaming;

  // IRenderNode implementation
  virtual const char * GetEntityClassName(void) const { return "SIsoMesh"; }
  virtual const char *GetName(void) const { return "SIsoMesh"; }
  virtual Vec3 GetPos(bool) const { return Vec3(0,0,0); }
  virtual void Render(const SRendParams &RendParams);
  virtual IPhysicalEntity *GetPhysics(void) const { return m_pPhysEnt; }
  virtual void SetPhysics(IPhysicalEntity * pPhys) { m_pPhysEnt = pPhys; }
  virtual void SetMaterial(IMaterial * pMat) { }
  virtual IMaterial *GetMaterial(Vec3 * pHitPos = NULL) { return NULL; }
	virtual IMaterial *GetMaterialOverride() { return NULL; }
  virtual float GetMaxViewDist() { return 10000; };
  virtual EERType GetRenderNodeType() { return eERType_IsoMesh; }
  virtual struct IRenderMesh * GetRenderMesh(int nLod) { return m_pRenderMesh; }
  virtual void GetMemoryUsage(ICrySizer * pSizer);
  virtual const AABB GetBBox() const { return m_WSBBox; }
  virtual void SetBBox( const AABB& WSBBox ) { m_WSBBox = WSBBox; }
  virtual void OnRenderNodeBecomeVisible() {};
  virtual void Physicalize(bool) {}

  void CheckInitAffectingLights();
  PodArray<CDLight*> * GetAffectingLights();
  void AddLightSource(CDLight * pSource);
  bool IsMeshCompiled();
  static char * MakeTempTextureFileName(char * szFileName, int nMaxFileNameLen, int nTexSlotId, const char * szExt, ISO_KEY key, bool bMakeFolder = false);
  const Vec3 GetOrigin() const;
  void RequestTextureUpdate();
  int GetMeshKey2D();
  static int UnloadOldSrcTextures();
  void MarkMixMasksMap2DAsUsed();

  IDynTexture * m_arrDstDynTex[VOX_TEX_NUM_DST];
  Vec3 m_arrTexRangeInfo[4];
  
  int m_arrSrcTex[VOX_TEX_NUM_SRC];
  CMemoryBlock * m_arrSrcTexData[VOX_TEX_NUM_SRC];

  IRenderMesh * m_pRenderMesh;
  SVoxSurfaceTypeInfo m_arrSurfaceTypeInfos[MAX_SURFACE_TYPES_COUNT];
  IPhysicalEntity * m_pPhysEnt;
  phys_geometry * m_pPhysGeom;
  AABB m_WSBBox;
  class CMemoryBlock* m_pMeshForSerialization;
  int m_arrTexturesZipSize[2];
  int m_nMeshFileOffset;
  int m_nMeshFileDataSize;
  int m_nRebuildRequestedFrameId;
  int m_nRebuildStartedFrameId;
  bool m_bInProgress;
  int m_nTexDimX;
  int m_nTexDimY;
//  int m_nTexDimScale;
  uint32 m_nLastDrawFrameId;
	float m_fLastDrawTime;
  static int m_nThisFrameTextureMemoryUsage;
  static int m_nThisFrameMeshMemoryUsage;
  CIndexedMesh * m_pGeneratedMesh;
  PodArray<CDLight*> m_lstAffectingLights; uint32 m_nLightMaskFrameId;
  float m_arrTexInfo[6][4];
  bool m_bMeshWasCopiedFromParent;
  ISO_KEY m_key;
  int m_nBuildTexturesFrameId;
  CMemoryBlock * m_pZipData;
  bool m_bRenderSuccess;
  int m_nDiskUsage;
};


#endif // ISOMESH_H