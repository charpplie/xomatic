////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_water.h
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef CWATER_H
#define CWATER_H 

#define CYCLE_BUFFERS_NUM    2

class COcean : public IRenderNode, public Cry3DEngineBase
{
	PodArray<SVF_P3F_C4B_T2F> Verts_DWQ;
  PodArray<uint16> Indices_DWQ;
  IRenderMesh *m_pRenderMeshWaters[MAX_RECURSION_LEVELS][CYCLE_BUFFERS_NUM];

  // water bottom geometry
  PodArray<SVF_P3F_C4B_T2F> OceanBottom_Verts_DWQ;
  PodArray<uint16> OceanBottom_Indices_DWQ;
  IRenderMesh *m_pOceanBottomRenderMeshWaters;

  IMaterial * m_pTerrainWaterMat;
  IMaterial * m_pOceanBottomMat;
	class CREOcclusionQuery * m_pREOcclusionQueries[MAX_RECURSION_LEVELS][CYCLE_BUFFERS_NUM];
	IShader * m_pShaderOcclusionQuery;
  int m_nLastVisibleFrameId;
  float m_fLastVisibleFrameTime;
  static uint32 m_nVisiblePixelsCount;
	int m_nBottomTexId;
	float m_fLastFov;
  CCamera m_Camera;

  float m_fRECustomData[8]; // used for passing data to renderer
  float m_fREOceanBottomCustomData[8]; // used for passing data to renderer

	// ocean fog related members
	CREWaterVolume::SParams m_wvParams;
	CREWaterVolume::SOceanParams m_wvoParams;
	_smart_ptr< IMaterial > m_pWaterBodyIntoMat;
	_smart_ptr< IMaterial > m_pWaterBodyOutofMat;
	_smart_ptr< IMaterial > m_pWaterBodyIntoMatLowSpec;
	_smart_ptr< IMaterial > m_pWaterBodyOutofMatLowSpec;
	CREWaterVolume* m_pWVRE;
	std::vector<SVF_P3F_C4B_T2F> m_wvVertices;
	std::vector<uint16> m_wvIndices;

  static ITimer *m_pTimer;

  static CREWaterOcean *m_pOceanRE;

	void RenderWaterVolume();
  void RenderWaterBottom(const int nRecursionLevel);

public:
  COcean(IMaterial * pMat);
  ~COcean();

  void SetLastFov(float fLastFov) {m_fLastFov = fLastFov;}
  void Render(const int nRecursionLevel);
	int GetMemoryUsage();
	bool IsWaterVisible();

  
  static void SetTimer(ITimer *pTimer);
  static float GetWave( const Vec3 &pPosition );
  static uint32 GetVisiblePixelsCount();

  // fake IRenderNode implementation
  virtual EERType GetRenderNodeType() { return eERType_WaterVolume; }
  virtual const char * GetEntityClassName(void) const { return "Ocean"; }
  virtual const char *GetName(void) const { return "Ocean"; }
  virtual Vec3 GetPos(bool) const { return Vec3(0,0,0); }
  virtual void Render(const SRendParams &) {}
  virtual IPhysicalEntity *GetPhysics(void) const { return 0; }
  virtual void SetPhysics(IPhysicalEntity *) { }
  virtual void SetMaterial(IMaterial * pMat) { }
  virtual IMaterial *GetMaterial(Vec3 * pHitPos = NULL) { return m_pTerrainWaterMat; }
	virtual IMaterial *GetMaterialOverride() { return m_pTerrainWaterMat; }
  virtual float GetMaxViewDist() { return 1000000.f; }
  virtual void GetMemoryUsage(ICrySizer * pSizer) {}
  virtual const AABB GetBBox() const { return AABB(Vec3(-1000000.f,-1000000.f,-1000000.f),Vec3(1000000.f,1000000.f,1000000.f)); }
  virtual void SetBBox( const AABB& WSBBox ) { }
};

#endif // CWATER_H
