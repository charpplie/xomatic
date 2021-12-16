#ifndef _CVegetation_H_
#define _CVegetation_H_

#define VEGETATION_CONV_FACTOR 64.f

// Warning: Average outdoor level has about 200.000 objects of this class allocated - so keep it small
class CVegetation : public IVegetation, public Cry3DEngineBase
{
public:
  Vec3 m_vPos;
  IPhysicalEntity * m_pPhysEnt;
	byte  m_nObjectTypeID;
	byte  m_ucAngle;
	byte  m_ucSunDotTerrain;
  byte  m_ucScale;
  byte  m_boxExtends[6];
  byte  m_ucRadius;
  byte  m_ucInSpritesList;
  union
  {
    float		m_HMARange;
    uint32	m_HMAIndex;
  };
  byte m_alignNormal[2];

  static float g_scBoxDecomprTable[256] _ALIGN(128);

	CVegetation();
  virtual ~CVegetation();
  void SetStatObjGroupId(int nVegetationanceGroupId) { m_nObjectTypeID = nVegetationanceGroupId; }
  int GetStatObjGroupId() const { return m_nObjectTypeID; }
  const char *GetEntityClassName(void) const { return "Vegetation"; }
	Vec3 GetPos(bool bWorldOnly = true) const { assert(bWorldOnly); return m_vPos; }
  virtual float GetScale(void) const { return (1.f/VEGETATION_CONV_FACTOR)*m_ucScale; }
  void SetScale(float fScale) { m_ucScale = (uint8)SATURATEB(fScale*VEGETATION_CONV_FACTOR); }
  const char *GetName(void) const;
  virtual void Render(const SRendParams &RendParams){ assert(0); }
  void Render(SRenderObjectModifier * pROM, int nThreadId, SSectorTextureSet * pTerrainTexInfo = NULL) const;
  IPhysicalEntity *GetPhysics(void) const { return m_pPhysEnt; }
	IRenderMesh *		GetRenderMesh(int nLod);
  void SetPhysics(IPhysicalEntity * pPhysEnt) { m_pPhysEnt = pPhysEnt; }
  void SetMaterial(IMaterial *) {}
  IMaterial *GetMaterial(Vec3 * pHitPos = NULL);
	IMaterial *GetMaterialOverride();
  void SetMatrix( const Matrix34& mat ) { m_vPos = mat.GetTranslation(); }
  virtual void Physicalize( bool bInstant = false );
	bool PhysicalizeFoliage(bool bPhysicalize = true, int iSource=0);
	IPhysicalEntity* GetBranchPhys(int idx);
	IFoliage *GetFoliage();
	bool IsBreakable() { pe_params_part pp; pp.ipart=0; return m_pPhysEnt && m_pPhysEnt->GetParams(&pp) && pp.idmatBreakable>=0; }
	void AddBending(Vec3 const& v);
  virtual float GetMaxViewDist();
  IStatObj * GetEntityStatObj( unsigned int nPartId, unsigned int nSubPartId = 0, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false);
  virtual EERType GetRenderNodeType() { return eERType_Vegetation; }
  virtual void Dephysicalize(bool bKeepIfReferenced=false);
  void Dematerialize( );
  virtual void GetMemoryUsage(ICrySizer * pSizer);
  virtual const AABB GetBBox() const;
  virtual void FillBBox(AABB & aabb);
  virtual void SetBBox( const AABB& WSBBox );
  const float GetRadius() const;
	void UpdateRndFlags();
	ILINE CStatObj* GetStatObj() const { return GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].GetStatObj(); }
	ILINE float GetZAngle() const;
	AABB CalcBBox();
	void CalcTerrainAdaption();
	void CalcMatrix( Matrix34A &tm, int * pTransFags = NULL );
	virtual uint8 GetMaterialLayers() const { return GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].nMaterialLayers; }
//	float GetLodForDistance(float fDistance);
	void UpdateSunDotTerrain();
	void Init();
	void ShutDown();
  void OnRenderNodeBecomeVisible();
  void UpdateSpriteInfo(SVegetationSpriteInfo & properties, float fSpriteAmount, SSectorTextureSet * pTerrainTexInfo) const;
  void UpdateBending();
  static void InitVegDecomprTable();
  void UpdateRenderQuality(float fDistance);
  void UpdateFoliageState();

  ILINE void SetBoxExtends(byte * pBoxExtends, byte * pRadius, const AABB& RESTRICT_REFERENCE aabb, const Vec3& RESTRICT_REFERENCE vPos)
  {
    const float fRatio = (255.f / VEGETATION_CONV_FACTOR);
    Vec3 v0 = aabb.max - vPos;
    pBoxExtends[0] = (byte)SATURATEB(v0.x*fRatio+1.f);
    pBoxExtends[1] = (byte)SATURATEB(v0.y*fRatio+1.f);
    pBoxExtends[2] = (byte)SATURATEB(v0.z*fRatio+1.f);
    Vec3 v1 = vPos - aabb.min;
    pBoxExtends[3] = (byte)SATURATEB(v1.x*fRatio+1.f);
    pBoxExtends[4] = (byte)SATURATEB(v1.y*fRatio+1.f);
    pBoxExtends[5] = (byte)SATURATEB(v1.z*fRatio+1.f);
    *pRadius = (byte)SATURATEB(max(v0.GetLength(),v1.GetLength())*fRatio+1.f);
  }

  ILINE void FillBBoxFromExtends(AABB & aabb, const byte * const __restrict pBoxExtends, const Vec3& RESTRICT_REFERENCE vPos) const
  {
    const float *const __restrict cpDecompTable = g_scBoxDecomprTable;
    const float cData0 = cpDecompTable[pBoxExtends[0]];
    const float cData1 = cpDecompTable[pBoxExtends[1]];
    const float cData2 = cpDecompTable[pBoxExtends[2]];
    const float cData3 = cpDecompTable[pBoxExtends[3]];
    const float cData4 = cpDecompTable[pBoxExtends[4]];
    const float cData5 = cpDecompTable[pBoxExtends[5]];
    aabb.max.x = vPos.x + cData0;
    aabb.max.y = vPos.y + cData1;
    aabb.max.z = vPos.z + cData2;
    aabb.min.x = vPos.x - cData3;
    aabb.min.y = vPos.y - cData4;
    aabb.min.z = vPos.z - cData5;
  }

  ILINE void FillBBox_NonVirtual(AABB & aabb) const
  {
    FillBBoxFromExtends(aabb, m_boxExtends, m_vPos); 
  }
};

#endif // _CVegetation_H_
