/*=============================================================================
DeferredShading.h : Deferred shading pipeline
Copyright (c) 2001 Crytek Studios. All Rights Reserved.

=============================================================================*/

#ifndef _DEFERREDSHADING_H_
#define _DEFERREDSHADING_H_

struct IVisArea;

class CDeferredShading
{
public:

  static CDeferredShading &Instance()
  {
    return m_pInstance;
  }

  void Render();  
  void SetupPasses();
  void SetupGlobalConsts();

  bool AmbientPass(const CDLight *pGlobalCubemap);
  bool DrawAmbientIndexed();
  void DeferredDecalPass( SDeferrredDecal& rDecal);
  void SunLightPasses(const CDLight& light, const int nLightID);
  bool ShadowLightPasses(const CDLight& light, const int nLightID);
  void DrawLightVolume(const CDLight *pDL);
  void LightStencilPrePass( const CDLight *pDL );
  void LightPass( const CDLight *pDL, bool bForceStencilDisable = false);
	void DeferredCubemaps( const TArray<CDLight>& rCubemaps, const uint32 nStartIndex = 0 );
	void DeferredCubemapPass( const CDLight *pDL );
	void DeferredLights( const TArray<CDLight>& rLights );
	void NegativeLightPass( const CDLight *pDL );
	void DeferredScenePass();
	void ComputeSSGI(CTexture* pSSGITarget, bool bMergeWithGI);

  void CreateDeferredMaps();
  void DestroyDeferredMaps();
  void Release();
  void Debug();

  uint32 AddLight( const CDLight &pDL, float fMult );
  inline uint32 AddVisArea( const IVisArea *pVisArea );
	inline void ResetLights();
	inline void ResetVisAreas();

	uint32 GetVisAreaID( uint32 nThreadID, const IVisArea *pVisArea );

  TArray<CDLight>& GetLights(const int nThreadID, const int nCurRecLevel, const eDeferredLightType eType = eDLT_DeferredLight);
  uint32 GetLightsNum(const eDeferredLightType eType);

  inline uint32 GetLightsCount() const 
  {
    return m_nLightsProcessedCount;
  }

  inline Vec4 GetLightDepthBounds( const CDLight *pDL ) const
  {
    if( !CRenderer::CV_r_deferredshadingdepthboundstest ) 
      return Vec4(0.0f, 0.0f, 1.0f, 1.0f);

    float fMinZ = 0.0f, fMaxZ = 1.0f;
    float fMinW = 0.0f, fMaxW = 1.0f;

    Vec3 pBounds = m_pCamFront * pDL->m_fRadius; 
    Vec3 pMax = pDL->m_Origin - pBounds;
    Vec3 pMin = pDL->m_Origin + pBounds;

    fMinZ = m_pViewProj.m20 * pMin.x + m_pViewProj.m21 * pMin.y + m_pViewProj.m22 * pMin.z + m_pViewProj.m23;
    fMinW = m_pViewProj.m30 * pMin.x + m_pViewProj.m31 * pMin.y + m_pViewProj.m32 * pMin.z + m_pViewProj.m33;
    fMinZ = iszero(fMinW)? 1.0f: fMinZ / fMinW;
    if( fMinW < 0.0f)
      fMinZ = 0.0f; 

    fMinZ = max( fMinZ, 0.01f );

    fMaxZ = m_pViewProj.m20 * pMax.x + m_pViewProj.m21 * pMax.y + m_pViewProj.m22 * pMax.z + m_pViewProj.m23;
    fMaxW = m_pViewProj.m30 * pMax.x + m_pViewProj.m31 * pMax.y + m_pViewProj.m32 * pMax.z + m_pViewProj.m33; 
    fMaxZ = iszero(fMaxW)? 1.0f: fMaxZ / fMaxW;
    if( fMaxW < 0.0f )
      fMaxZ = 0.0f;

    return Vec4( fMinZ, max(fMinW /*/ m_fCamFar*/, 0.000001f ), fMaxZ, max(fMaxW/*/ m_fCamFar*/, 0.000001f ));
  }


	const Matrix44A& GetCameraProjMatrix() const { return m_pViewProj; }

private:

  CDeferredShading()
  {
    m_pShader = 0;
    m_pTechName = "DeferredLightPass";
		m_pAmbientTechName = "AmbientPass";
    m_pCubemapsTechName = "DeferredCubemapPass";
    m_pDebugTechName = "Debug";
		m_pDeferredDecalTechName = "DeferredDecal";
    m_pLightVolumeTechName = "DeferredLightVolume";
		m_pDeferredSceneTechName = "DeferredScenePass";
		
    m_pParamPalette = "g_palette";
    m_pParamLightPos = "g_LightPos";
    m_pParamCameraMatrix = "g_mCamera";
    m_pParamLightProjMatrix = "g_mLightProj";
    m_pParamLightProjParams = "g_LightProjParams";
    m_pGeneralParams = "g_GeneralParams";    
    m_pParamLightDiffuse = "g_LightDiffuse";        
    m_pParamViewProjI = "g_mViewProjI";

    m_pDiffuseAccRT = CTexture::s_ptexCurrentSceneDiffuseAccMap;
		m_pSpecularAccRT = CTexture::s_ptexSceneSpecularAccMap;
		m_pNormalsRT = CTexture::s_ptexSceneNormalsMap;
    m_pDepthRT = CTexture::s_ptexZTarget;
		m_pSceneTexturesRT = CTexture::s_ptexSceneTexturesMap;

    m_pDebugRT = 0;
    m_nLightsProcessedCount = 0;

		m_pSSAORT = NULL;

    m_nRenderState = GS_BLSRC_ONE|GS_BLDST_ONE;
		
    memset(m_nVisAreasCount, 0, sizeof( m_nVisAreasCount ) ) ;
  }

  ~CDeferredShading()
  {
    Release();
  }

  // Allow disable mrt usage: for double zspeed and on other passes less fillrate hit
  void SpecularAccEnableMRT( bool bEnable );

private:

  // Vis areas for current view 
  typedef std::map< const IVisArea *, uint32 > VisAreaIDMap;    
  typedef VisAreaIDMap::iterator VisAreaIDMapItor;
  VisAreaIDMap m_pVisAreas[2][MAX_REND_RECURSION_LEVELS];
  uint32 m_nVisAreasCount[2][MAX_REND_RECURSION_LEVELS];

  // Deferred passes common 
	TArrayHolder<CDLight> m_pLights[eDLT_NumLightTypes];

  Vec3 m_pCamPos;
  Vec3 m_pCamFront;
  float m_fCamFar;
  float m_fCamNear;

  float m_fRatioWidth;
  float m_fRatioHeight;

  CShader  *m_pShader; 
  CCryName m_pDeferredDecalTechName;
  CCryName m_pLightVolumeTechName;
  CCryName m_pTechName;
	CCryName m_pAmbientTechName;
	CCryName m_pCubemapsTechName;
  CCryName m_pDebugTechName;
	CCryName m_pDeferredSceneTechName;
  CCryName m_pParamLightPos;
  CCryName m_pParamPalette;
  CCryName m_pParamCameraMatrix;
  CCryName m_pParamLightDiffuse;        
  CCryName m_pParamViewProjI;
  CCryName m_pParamLightProjMatrix;
  CCryName m_pParamLightProjParams;
  CCryName m_pGeneralParams;

  Matrix44A m_pViewProj;
  Matrix44A m_pViewProjI;
  Matrix44A m_pView;  

  Vec4 vWorldBasisX, vWorldBasisY, vWorldBasisZ;

  CTexture *m_pDiffuseAccRT;
	CTexture *m_pSpecularAccRT;
  CTexture *m_pNormalsRT;
  CTexture *m_pDepthRT;
	CTexture *m_pSceneTexturesRT;

  CTexture *m_pDebugRT;

  int m_nRenderState;
  uint32 m_nLightsProcessedCount;

  static CDeferredShading m_pInstance;
public:
	SDynTexture* m_pSSAORT;
};

#endif
