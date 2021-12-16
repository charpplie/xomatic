////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   3dengine.h
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef C3DENGINE_H
#define C3DENGINE_H

#if _MSC_VER > 1000
# pragma once
#endif

#ifdef DrawText
#undef DrawText
#endif //DrawText

struct SLevelInfo
{
  SLevelInfo()
  {
    m_fSunSpecMult = 1.f;
  }

  float m_fSkyBoxAngle,
		m_fSkyBoxStretching;

  float m_fMaxViewDistScale;
  float m_fMaxViewDistHighSpec;
  float m_fMaxViewDistLowSpec;
  float m_fTerrainDetailMaterialsViewDistRatio;

	float m_volFogGlobalDensity;
	float m_volFogGlobalDensityMultiplierLDR;
	float m_volFogAtmosphereHeight;
	float m_volFogArtistTweakDensityOffset;

	float m_volFogGlobalDensityMod;
	float m_volFogAtmosphereHeightMod;

	float m_fCloudShadingSunLightMultiplier;
	float m_fCloudShadingSkyLightMultiplier;

  Vec3 m_vFogColor; 
  Vec3 m_vDefFogColor; 
  Vec3 m_vSunDir;
	Vec3 m_vSunDirNormalized;
  float m_fSunDirUpdateTime;
  Vec3 m_vSunDirRealtime;
  Vec3 m_vWindSpeed;

  Vec3 m_vScatteringDir;

	Vec3 m_volFogRamp;

	Vec3 m_nightSkyHorizonCol;
	Vec3 m_nightSkyZenithCol;
	float m_nightSkyZenithColShift;
	float m_nightSkyStarIntensity;
	Vec3 m_nightMoonCol;
	float m_nightMoonSize;
	Vec3 m_nightMoonInnerCoronaCol;
	float m_nightMoonInnerCoronaScale;
	Vec3 m_nightMoonOuterCoronaCol;
	float m_nightMoonOuterCoronaScale;

	float m_sunRotationZ;
	float m_sunRotationLongitude;
	float m_moonRotationLatitude;
	float m_moonRotationLongitude;
	Vec3 m_moonDirection;
	int m_nWaterBottomTexId;
	int m_nCloudShadowTexId;
	int m_nNightMoonTexId;
	bool m_bShowTerrainSurface;
	bool m_bSunShadows;
	Vec3 m_oceanFogColor;
	Vec3 m_oceanFogColorShallow;
	float m_oceanFogDensity;
	float m_oceanFogColorMultiplier;
	float m_oceanFogColorMultiplierEnvironment;
	float m_skyboxMultiplier;
	float m_dayNightIndicator;
	Vec3 m_fogColor2;

  float m_oceanCausticsDistanceAtten; 
  float m_oceanCausticsMultiplier;  
  float m_oceanCausticsDarkeningMultiplier;

  string m_skyMatName;
  string m_skyLowSpecMatName;

  float m_oceanWindDirection;
  float m_oceanWindSpeed;
  float m_oceanWavesSpeed;
  float m_oceanWavesAmount;
  float m_oceanWavesSize;

	float m_dawnStart;
	float m_dawnEnd;
	float m_duskStart;
	float m_duskEnd;

  // special case for combat mode adjustments
  float m_fSaturation;
  Vec4 m_pPhotoFilterColor;
  float m_fPhotoFilterColorDensity;
  float m_fGrainAmount;
  float m_fSunSpecMult;
};

struct SLevelShaders
{
  _smart_ptr<IMaterial> m_pTerrainWaterMat;
	_smart_ptr<IMaterial> m_pSkyMat;
	_smart_ptr<IMaterial> m_pSkyLowSpecMat;
	_smart_ptr<IMaterial> m_pSunMat;
  _smart_ptr<IMaterial> m_pTerrainMat;
  _smart_ptr<IMaterial> m_pVoxTerrainMat;

	_smart_ptr<IShader> m_pFarTreeSprites;

	void CleanLevelShaders()
	{
		m_pTerrainWaterMat = 0;
		m_pSkyMat = 0;
		m_pSkyLowSpecMat = 0;
		m_pSunMat = 0;
		m_pTerrainMat = 0;
		m_pVoxTerrainMat = 0;
		m_pFarTreeSprites = 0;
	}
};

struct SRenderElements
{
  CRESky              * m_pRESky;
	CREHDRSky           * m_pREHDRSky;
  CREPostProcess      * m_pREPostProcess;   
};

struct SEntInFoliage 
{
	int id;
	float timeIdle;
};

struct IFaceGen;

class CMemoryBlock : public IMemoryBlock
{
public:
	virtual void * GetData() { return m_pData; }
	virtual int GetSize() { return m_nSize; }
	virtual ~CMemoryBlock() { delete [] m_pData; }
	
	CMemoryBlock() { m_pData=0; m_nSize=0; }
	CMemoryBlock(const void * pData, int nSize) 
	{ 
		m_pData=0; 
		m_nSize=0; 
		SetData(pData, nSize); 
	}
	void SetData(const void * pData, int nSize) 
	{ 
		delete [] m_pData;
		m_pData = new uint8[nSize];
		memcpy(m_pData, pData, nSize);
		m_nSize = nSize;
	}
  void Free() 
  { 
    delete [] m_pData;
    m_pData = NULL;
    m_nSize = 0;
  }
	void Allocate(int nSize) 
	{ 
		delete [] m_pData;
		m_pData = new uint8[nSize];
		memset(m_pData, 0, nSize);
		m_nSize = nSize;
	}

  static CMemoryBlock * CompressToMemBlock(void * pData, int nSize, ISystem * pSystem)
  { 
    CMemoryBlock * pMemBlock = NULL;
    uint8 * pTmp = new uint8[nSize+4];
    size_t nComprSize = nSize;
    *(uint32*)pTmp = nSize;
    if( pSystem->CompressDataBlock(pData, nSize, pTmp+4, nComprSize ))
    {
      pMemBlock = new CMemoryBlock(pTmp, nComprSize+4);
    }

    delete [] pTmp;
    return pMemBlock;
  }

  static CMemoryBlock * DecompressFromMemBlock(CMemoryBlock * pMemBlock, ISystem * pSystem)
  { 
    size_t nUncompSize = *(uint32*)pMemBlock->GetData();
		SwapEndian(nUncompSize);
    CMemoryBlock * pResult = new CMemoryBlock;
    pResult->Allocate(nUncompSize);
    if(!pSystem->DecompressDataBlock((byte*)pMemBlock->GetData()+4, pMemBlock->GetSize()-4, pResult->GetData(), nUncompSize))
    {
      assert(!"CMemoryBlock::DecompressFromMemBlock failed");
      delete pResult;
      pResult = NULL;
    }

    return pResult;
  }

	uint8 * m_pData;
	int m_nSize;
};

struct SNodeInfo;
class CStitchedImage;
struct DLightAmount{ CDLight * pLight; float fAmount; };

#define LIGHTS_CULL_INFO_CACHE_SIZE 16

struct CRNTmpData
{
  struct SRNUserData
  {
    int m_narrDrawFrames[MAX_RECURSION_LEVELS];	
    SDissolveTransitionState dissolveTransitionState;
    SLightInfo lightsCache[LIGHTS_CULL_INFO_CACHE_SIZE];
    Matrix34 objMat;
    int objMatFlags;
    float fRadius;
    Vec2 m_vCurrentBending;
    OcclusionTestClient m_OcclState;
    uint32 m_nOccluderFrameId;
    struct IFoliage * m_pFoliage;
    CRenderObject * pRenderObject;
    int nLod;
    SBending m_Bending;
  } userData;

  CRNTmpData() { memset(this,0,sizeof(*this)); assert((uint32)this == (uint32)&(this->userData)); }
  CRNTmpData *pNext, *pPrev;
  CRNTmpData ** pOwnerRef;
  uint32 nLastUsedFrameId;
  int nCreatedFrameId;

  void Unlink()
  {
    if (!pNext || !pPrev)
      return;
    pNext->pPrev = pPrev;
    pPrev->pNext = pNext;
    pNext = pPrev = NULL;
  }

  void Link( CRNTmpData* Before )
  {
    if (pNext || pPrev)
      return;
    pNext = Before->pNext;
    Before->pNext->pPrev = this;
    Before->pNext = this;
    pPrev = Before;
  }

  int Count()
  {
    int nCounter = 0;
    for(CRNTmpData * pElem = pNext; pElem!=this; pElem = pElem->pNext)
      nCounter++;
    return nCounter;
  }
};

template <class T, int nMaxElemsInChunk> struct CPoolAllocator
{
  CPoolAllocator() { m_nCounter=0; } 

  ~CPoolAllocator()
  {
    Reset();
  }

  void Reset()
  {
    for(int i=0; i<m_Pools.Count(); i++)
    {
      delete [] m_Pools[i];
      m_Pools[i] = NULL;
    }
    m_nCounter = 0;
  }

  T * GetNewElement() 
  { 
    int nPoolId = m_nCounter/nMaxElemsInChunk;
    int nElemId = m_nCounter - nPoolId*nMaxElemsInChunk;
    m_Pools.PreAllocate(nPoolId+1,nPoolId+1);
    if(!m_Pools[nPoolId])
      m_Pools[nPoolId] = new T[nMaxElemsInChunk];
    m_nCounter++;
    return &m_Pools[nPoolId][nElemId];
  }

  int GetMemUsage() { return m_Pools.Count() * nMaxElemsInChunk * sizeof(T); }

  int GetMaxElemsInChunk() { return nMaxElemsInChunk; }

  int m_nCounter;
  PodArray<T*> m_Pools;
};

struct SImageSubInfo
{
  SImageSubInfo() { memset(this,0,sizeof(*this)); fTiling=fTilingIn=1.f; }
  
  static const int nMipsNum = 4;

  union
  {
    byte * pImgMips[nMipsNum];
    int pImgMipsSizeKeeper[8];
  };
 
  float fAmount;
  int  nReady;
  int nDummy[4];

  union
  {
    IMaterial * pMat;
    int pMatSizeKeeper[2];
  };

  int nDim;
  float fTilingIn;
  float fTiling;
  float fSpecularAmount;
  int nSortOrder;
  int nAlignFix;

  AUTO_STRUCT_INFO
};

struct SVoxTexHeader
{
  SVoxTexHeader() { ZeroStruct(*this); }
  uint16 w, h, eFormat, nPitch;
  inline int GetDataSize() { assert(nPitch<2048*4); return nPitch*h; }
  inline void SetDataSize(int nDataSize) { nPitch = nDataSize/h; assert(nDataSize == nDataSize/h*h); assert(nPitch<2048*4); }

  AUTO_STRUCT_INFO
};

struct SImageInfo : public Cry3DEngineBase
{
  SImageInfo() 
  { 
    szDetMatName[0] = szBaseTexName[0] = nPhysSurfaceType = 0; nLayerId=0; fUseRemeshing=0; fBr=1.f; layerFilterColor=Col_White; 
    ZeroStruct(arrTextureId);
  }

  SImageSubInfo baseInfo;
  SImageSubInfo detailInfo;

  char szDetMatName[128-20];

  int arrTextureId[4];
  int nPhysSurfaceType; 

  char szBaseTexName[128];

  float fUseRemeshing;
  ColorF layerFilterColor;
  int nLayerId;
  float fBr;
  int nDetailSurfTypeId;

  int GetMemoryUsage();

  AUTO_STRUCT_INFO
};

//////////////////////////////////////////////////////////////////////
class C3DEngine : public I3DEngine, 
  public SLevelInfo, 
  public SLevelShaders, 
  public SRenderElements,
  public Cry3DEngineBase
{
  // IProcess Implementation
  void	SetFlags(int flags) { m_nFlags=flags; }
	int		GetFlags(void) { return m_nFlags; }
	int		m_nFlags;

public:

  // I3DEngine interface implementation
	VIRTUAL bool Init();
  VIRTUAL void OnFrameStart();
  VIRTUAL void Update();
	VIRTUAL void RenderWorld(const int nRenderFlags, const CCamera * pCameras, int nCamerasNum, const char *szDebugName, const int dwDrawFlags, const int nFilterFlags);
	VIRTUAL void ShutDown();
  virtual void Release() { delete this; };
	VIRTUAL void SetLevelPath( const char * szFolderName );
	VIRTUAL bool LoadLevel(const char * szFolderName, const char * szMissionName);
	VIRTUAL void UnloadLevel();
	VIRTUAL void PostLoadLevel();
	VIRTUAL bool InitLevelForEditor(const char * szFolderName, const char * szMissionName);
  VIRTUAL void DisplayInfo(float & fTextPosX, float & fTextPosY, float & fTextStepY, const bool bEnhanced);
  VIRTUAL void SetupDistanceFog();
	VIRTUAL IStatObj* LoadStatObj( const char *szFileName,const char *szGeomName=NULL,/*[Out]*/IStatObj::SSubObject **ppSubObject=NULL, bool bUseStreaming=true,unsigned long nLoadingFlags=0 );
	VIRTUAL IStatObj * FindStatObjectByFilename(const char * filename);
  VIRTUAL void RegisterEntity( IRenderNode * pEnt, int nSID=-1 );
  VIRTUAL void SelectEntity( IRenderNode * pEnt );
  VIRTUAL bool UnRegisterEntity( IRenderNode * pEnt );
	VIRTUAL bool IsUnderWater( const Vec3& vPos) const;
  VIRTUAL void SetOceanRenderFlags( uint8 nFlags );
  VIRTUAL uint8 GetOceanRenderFlags() const;
  VIRTUAL uint32 GetOceanVisiblePixelsCount() const;
	VIRTUAL float GetBottomLevel(const Vec3 & referencePos, float maxRelevantDepth, int objtypes);
	VIRTUAL float GetBottomLevel(const Vec3 & referencePos, float maxRelevantDepth /* = 10.0f*/);
	VIRTUAL float GetBottomLevel(const Vec3 & referencePos, int objflags);
  VIRTUAL float GetWaterLevel(const Vec3 * pvPos, Vec3 * pvFlowDir = NULL);
  VIRTUAL float GetWaterLevel();
  VIRTUAL float GetOceanWaterLevel( const Vec3 &pCurrPos ) const;
  VIRTUAL Vec4 GetCausticsParams() const;  
  VIRTUAL void GetOceanAnimationParams(Vec4 &pParams0, Vec4 &pParams1 ) const;  
	VIRTUAL void CreateDecal( const CryEngineDecalInfo& Decal );
  VIRTUAL void DrawFarTrees();
  VIRTUAL void GenerateFarTrees();
  VIRTUAL float GetTerrainElevation(float x, float y, bool bIncludeOutdoorVoxles = false);
  VIRTUAL float GetTerrainElevation3D(Vec3 vPos);
  VIRTUAL float GetTerrainZ(int x, int y);
  VIRTUAL bool GetTerrainHole(int x, int y);
  VIRTUAL int GetHeightMapUnitSize();
  VIRTUAL int GetTerrainSize();
	virtual void SetSunDir( const Vec3& newSunOffset );
  VIRTUAL Vec3 GetSunDir() const;
  VIRTUAL Vec3 GetSunDirNormalized() const;
  VIRTUAL Vec3 GetRealtimeSunDirNormalized() const;
	VIRTUAL void SetSkyColor(Vec3 vColor);
	VIRTUAL void SetSunColor(Vec3 vColor);
  virtual void SetSunSpecMultiplier(float fMult) { m_fSunSpecMult = fMult; }
	VIRTUAL void SetSkyBrightness(float fMul); 
	virtual void SetSSAOAmount(float fMul);
  virtual void SetGIAmount(float fMul);
	VIRTUAL float GetSunRel() const;
  VIRTUAL void OnExplosion(Vec3 vPos, float fRadius, bool bDeformTerrain = true);
  //! For editor  
  VIRTUAL void RemoveAllStaticObjects(int nSID);
  VIRTUAL void SetTerrainSurfaceType(int x, int y, int nType);
	VIRTUAL void SetTerrainSectorTexture( const int nTexSectorX, const int nTexSectorY, unsigned int textureId );
  VIRTUAL void SetPhysMaterialEnumerator(IPhysMaterialEnumerator * pPhysMaterialEnumerator);
  VIRTUAL IPhysMaterialEnumerator * GetPhysMaterialEnumerator();
  VIRTUAL void LoadMissionDataFromXMLNode(const char * szMissionName);

  void AddDynamicLightSource(const class CDLight & LSource, ILightSource *pEnt, int nEntityLightId=-1, float fFadeout = 0.f);
  VIRTUAL void ApplyForceToEnvironment(Vec3 vPos, float fRadius, float fAmountOfForce);
  VIRTUAL void SetMaxViewDistanceScale(float fScale) { m_fMaxViewDistScale = fScale; }
  VIRTUAL float GetMaxViewDistance( bool bScaled = true );
  VIRTUAL void SetFogColor(const Vec3& vFogColor);
  VIRTUAL Vec3 GetFogColor( );
	VIRTUAL float GetDistanceToSectorWithWater();

	VIRTUAL void GetVolumetricFogSettings( float& globalDensity, float& atmosphereHeight, float& artistTweakDensityOffset, float& globalDensityMultiplierLDR );
	VIRTUAL void SetVolumetricFogSettings( float globalDensity, float atmosphereHeight, float artistTweakDensityOffset );
	VIRTUAL void GetVolumetricFogModifiers(float& globalDensityModifier, float& atmosphereHeightModifier);
	VIRTUAL void SetVolumetricFogModifiers(float globalDensityModifier, float atmosphereHeightModifier, bool reset = false);	

	VIRTUAL void GetSkyLightParameters( Vec3& sunIntensity, float& Km, float& Kr, float& g, Vec3& rgbWaveLengths );
	VIRTUAL void SetSkyLightParameters( const Vec3& sunIntensity, float Km, float Kr, float g, const Vec3& rgbWaveLengths, bool forceImmediateUpdate = false );
		
	VIRTUAL void GetCloudShadingMultiplier( float& sunLightMultiplier, float& skyLightMultiplier );
	VIRTUAL void SetCloudShadingMultiplier( float sunLightMultiplier, float skyLightMultiplier );

	VIRTUAL IAutoCubeMapRenderNode* GetClosestAutoCubeMap(const Vec3& p);

	VIRTUAL float GetHDRDynamicMultiplier() const;
	VIRTUAL void SetHDRDynamicMultiplier( const float value );
  inline float GetHDRDynamicMultiplierInline() const { return m_fHDRDynamicMultiplier; }

  VIRTUAL void SetRenderIntoShadowmap( bool bSet ) { m_bRenderIntoShadowmap = bSet; }
  VIRTUAL bool GetRenderIntoShadowmap() const { return m_bRenderIntoShadowmap; }
  inline  bool _GetRenderIntoShadowmap() const { return m_bRenderIntoShadowmap; }

	VIRTUAL void TraceFogVolumes( const Vec3& worldPos, ColorF& fogVolumeContrib );


  VIRTUAL Vec3 GetSkyColor() const;
  VIRTUAL Vec3 GetSunColor() const;
  virtual float GetSunSpecMultiplier() { return m_fSunSpecMult; }
	VIRTUAL float GetSkyBrightness() const;
	VIRTUAL float GetSSAOAmount() const;
  VIRTUAL float GetGIAmount() const;
	VIRTUAL float GetTerrainTextureMultiplier(int nSID) const;

  VIRTUAL Vec3 GetAmbientColorFromPosition(const Vec3 & vPos, float fRadius=1.f);
  VIRTUAL void ClearRenderResources();
  VIRTUAL void FreeRenderNodeState(IRenderNode * pEnt);
  VIRTUAL const char * GetLevelFilePath(const char * szFileName);
  VIRTUAL void SetTerrainBurnedOut(int x, int y, bool bBurnedOut);
  VIRTUAL bool IsTerrainBurnedOut(int x, int y);
  VIRTUAL int GetTerrainSectorSize();
  VIRTUAL void LoadTerrainSurfacesFromXML(XmlNodeRef pDoc, bool bUpdateTerrain, int nSID);
  VIRTUAL bool SetStatInstGroup(int nGroupId, const IStatInstGroup & siGroup, int nSID);
  VIRTUAL bool GetStatInstGroup(int nGroupId,       IStatInstGroup & siGroup, int nSID);
  VIRTUAL void ActivatePortal(const Vec3 &vPos, bool bActivate, const char * szEntityName);
  VIRTUAL void GetMemoryUsage(ICrySizer * pSizer);
	VIRTUAL void GetResourceMemoryUsage(ICrySizer * pSizer,const AABB& cstAABB);
  VIRTUAL IVisArea * CreateVisArea();
  VIRTUAL void DeleteVisArea(IVisArea * pVisArea);
  VIRTUAL void UpdateVisArea(IVisArea * pArea, const Vec3 * pPoints, int nCount, const char * szName, 
    const SVisAreaInfo & info, bool bReregisterObjects);
  VIRTUAL void ResetParticlesAndDecals( );
  VIRTUAL IRenderNode* CreateRenderNode( EERType type );
  VIRTUAL void DeleteRenderNode(IRenderNode * pRenderNode);
  VIRTUAL void SetWind( const Vec3 & vWind );
  VIRTUAL Vec3 GetWind( const AABB& box, bool bIndoors ) const;
  VIRTUAL IVisArea* GetVisAreaFromPos(const Vec3 &vPos);	
	VIRTUAL	bool IntersectsVisAreas(const AABB& box, void** pNodeCache = 0);
	VIRTUAL	bool ClipToVisAreas(IVisArea* pInside, Sphere& sphere, Vec3 const& vNormal, void* pNodeCache = 0);
  VIRTUAL bool IsVisAreasConnected(IVisArea * pArea1, IVisArea * pArea2, int nMaxReqursion, bool bSkipDisabledPortals);
  void EnableOceanRendering(bool bOcean); // todo: remove
	//////////////////////////////////////////////////////////////////////////
  // Materials access.
	VIRTUAL IMaterialManager* GetMaterialManager();
	//////////////////////////////////////////////////////////////////////////

	VIRTUAL struct ILightSource * CreateLightSource();
	VIRTUAL void DeleteLightSource(ILightSource * pLightSource);
  VIRTUAL const PodArray<CDLight*> * GetStaticLightSources();
  VIRTUAL bool IsTerrainHightMapModifiedByGame();
  VIRTUAL bool RestoreTerrainFromDisk(int nSID);  
  VIRTUAL void CheckMemoryHeap();
	VIRTUAL bool SetMaterialFloat( char * szMatName, int nSubMatId, int nTexSlot, char * szParamName, float fValue );
	VIRTUAL void CloseTerrainTextureFile(int nSID);
	VIRTUAL int GetLoadedObjectCount();
	VIRTUAL void GetLoadedStatObjArray( IStatObj** pObjectsArray,int &nCount );
  VIRTUAL void GetObjectsStreamingStatus(SObjectsStreamingStatus &outStatus);
	VIRTUAL void DeleteEntityDecals(IRenderNode * pEntity);
	VIRTUAL void DeleteDecalsInRange( AABB * pAreaBox, IRenderNode * pEntity );
	VIRTUAL void CompleteObjectsGeometry();
	VIRTUAL void LockCGFResources();
	VIRTUAL void UnlockCGFResources();
	//! paint voxel shape
	IMemoryBlock * Voxel_GetObjects(Vec3 vPos, float fRadius, int nSurfaceTypeId, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, EVoxelEditTarget eTarget);
	VIRTUAL void Voxel_Paint(Vec3 vPos, float fRadius, int nSurfaceTypeId, Vec3 vBaseColor, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, EVoxelEditTarget eTarget, PodArray<IRenderNode*> * pBrushes, float fMinVoxelSize);
	VIRTUAL void Voxel_SetFlags(bool bPhysics, bool bSimplify, bool bShadows, bool bMaterials);

	VIRTUAL void SerializeState( TSerialize ser );
	VIRTUAL void PostSerialize( bool bReading );

	VIRTUAL void SetHeightMapMaxHeight(float fMaxHeight);

	//////////////////////////////////////////////////////////////////////////
	// CGF Loader.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL CContentCGF* LoadChunkFileContent( const char *filename,bool bNoWarningMode=false );
	VIRTUAL void ReleaseChunkFileContent( CContentCGF *pCGF );
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL IChunkFile *CreateChunkFile( bool bReadOnly=false );

  //////////////////////////////////////////////////////////////////////////
  // Post processing effects interfaces    

  VIRTUAL void SetPostEffectParam(const char *pParam, float fValue, bool bForceValue=false) const;
  VIRTUAL void SetPostEffectParamVec4(const char *pParam, const Vec4 &pValue, bool bForceValue=false) const;
  VIRTUAL void SetPostEffectParamString(const char *pParam, const char *pszArg) const;

  VIRTUAL void GetPostEffectParam(const char *pParam, float &fValue) const;  
  VIRTUAL void GetPostEffectParamVec4(const char *pParam, Vec4 &pValue) const;  
  VIRTUAL void GetPostEffectParamString(const char *pParam, const char *pszArg) const;

  VIRTUAL void ResetPostEffects() const;

	VIRTUAL uint32 GetObjectsByType( EERType objType, IRenderNode **pObjects );
	VIRTUAL uint32 GetObjectsByTypeInBox( EERType objType, const AABB &bbox, IRenderNode **pObjects );
	VIRTUAL uint32 GetObjectsInBox(const AABB &bbox, IRenderNode **pObjects=0 );
  VIRTUAL const CCamera & GetCurrentCamera() { return GetCamera(); }

  //////////////////////////////////////////////////////////////////////////
  
	VIRTUAL int GetTerrainTextureNodeSizeMeters();
	VIRTUAL int GetTerrainTextureNodeSizePixels(int nLayer);

  VIRTUAL void SetTerrainLayerBaseTextureData(int nLayerId, byte*pImage, int nDim, const char * nImgFileName, IMaterial * pMat, float fBr, float fTiling, int nDetailSurfTypeId, float fTilingDetail, float fSpecularAmount, float fSortOrder, ColorF layerFilterColor, float fUseRemeshing, bool bShowSelection);
  SImageInfo * GetBaseTextureData(int nLayerId);
  SImageInfo * GetBaseTextureDataFromSurfType(int nSurfTypeId);

  const char * GetLevelFolder() { return m_szLevelFolder; }

	bool SaveCGF(std::vector<IStatObj *>& pObjs);
	

  int GetCurrentLightSpec()
  {
    if (m_pLightQuality)
      return m_pLightQuality->GetIVal() + CONFIG_LOW_SPEC; // Light quality is 0,1,2
    return CONFIG_VERYHIGH_SPEC; // very high spec.
  }

  bool SetupCamera(const CCamera &cam, const char * szCallerName);

	bool IsCameraUnderWater() const { return m_bCameraUnderWater; }

	void ScreenShotHighRes(CStitchedImage* pStitchedImage,const int nRenderFlags, const CCamera &cam, const int dwDrawFlags, const int nFilterFlags,uint32 SliceCount,f32 fTransitionSize);

	// cylindrical mapping made by multiple slices rendered and distorted
	// Returns:
	//   true=mode is active, stop normal rendering, false=mode is not active
	bool ScreenShotPanorama(CStitchedImage*	pStitchedImage,const int nRenderFlags, const CCamera &_cam, const int dwDrawFlags, const int nFilterFlags,uint32 SliceCount,f32 fTransitionSize );
	// Render simple top-down screenshot for map overviews
	bool ScreenShotMap(CStitchedImage* pStitchedImage,const int nRenderFlags, const CCamera &_cam, const int dwDrawFlags, const int nFilterFlags,uint32 SliceCount,f32 fTransitionSize );

	void ScreenshotDispatcher(const int nRenderFlags, const CCamera &_cam, const int dwDrawFlags, const int nFilterFlags);

	VIRTUAL void FillDebugFPSInfo(SDebugFPSInfo& info);

	void ClearDebugFPSInfo()
	{
		m_fAverageFPS = 0.0f;
		m_fMinFPS = 0.0f;
		m_fMaxFPS = 0.0f;
		m_nFramesCount = 0;
		arrFPSforSaveLevelStats.clear();
	}

	VIRTUAL bool SaveCHR(const char* srcfilename,const char* filename,IMergeCHRMorphsPipe* pipe,bool bSaveMorps=false);



	VIRTUAL bool MergeCHRs(const char* basechr,IMergePipe* pipe,const char* result_filename,IMergeMaterialsResult**,IMergePipe::EMergeFlags flags);

	bool SaveFaceCHRInternal(const char* temlatefilename,const char* headFGTfilename,const char* facefilename,XmlNodeRef& head_node, IFaceGen* headFG,const char* attFGTfilename, const char* outfilename, IMaterial** material,bool bAttachment,bool bLeaveMorphMasks);

	VIRTUAL bool CanBeMerged(IMaterial* mt1,IMaterial* mt2,bool bVerbose,IMaterialMergePipe::EMergeMaterialsFlags flags);
	VIRTUAL IMergeMaterialsResult* MergeMaterials(IMaterialMergePipe* pipe,IMaterialMergePipe::EMergeMaterialsFlags flags);

public:
  C3DEngine( ISystem * pSystem );
  ~C3DEngine();

	VIRTUAL void RenderScene(const int nRenderFlags, unsigned int dwDrawFlags, const int nFilterFlags);
	VIRTUAL void DebugDraw_PushFrustrum( const char *szName, const CCamera &rCam, const ColorB col, const float fQuadDist=-1.0f );
	VIRTUAL void DebugDraw_PushFrustrum( const char *szName, const CRenderCamera &rCam, const ColorB col, const float fQuadDist=-1.0f );
	VIRTUAL void DebugDraw_UpdateDebugNode();

  uint32 BuildLightMask( const AABB & objBox );
  uint32 BuildLightMask( const AABB & objBox, PodArray<CDLight*> * pAffectingLights, CVisArea * pObjArea, bool bObjOutdoorOnly, SRestLightingInfo * pRestLightingInfo = NULL );
  bool BuildLightMask_CheckPortals( CVisArea * pObjArea, CVisArea * pLightArea, CDLight * pLight, const AABB & objBox );
	void DebugDraw_Draw();
  uint32 GetFullLightMask();
  bool IsOutdoorVisible();
  void RenderSkyBox(IMaterial*pMat);

	bool CreateDecalOnStatObj( const CryEngineDecalInfo& DecalInfo, class CDecal* pCallerManagedDecal );
	//void CreateDecalOnCharacterComponents(ICharacterInstance * pChar, const struct CryEngineDecalInfo & decal);
  Vec3 GetTerrainSurfaceNormal(Vec3 vPos);
  void LoadEnvironmentSettingsFromXML(XmlNodeRef pInputNode, int nSID);
	void LoadDefaultAssets();

  // access to components
  ILINE static CVars * GetCVars() { return m_pCVars; }
  ILINE CVisAreaManager * GetVisAreaManager() { return m_pVisAreaManager; }
	ILINE CMatMan * GetMatMan() { return m_pMatMan; }
	ILINE PodArray<ILightSource*> * GetLightEntities() { return &m_lstStaticLights; }

	ICVar *m_pSphericalSkinning;

  ICVar *m_pDeferredShading;
  ICVar *m_pDeferredShadingILSamples;
  ICVar *m_pDeferredShadingDebug;
  int m_nDeferredLightsNum;

private:

	std::vector<byte> arrFPSforSaveLevelStats;

  float						m_fHDRDynamicMultiplier;						// only used for HDR, to amplify the dynamic range for emissive objects, 1.0 means no amplification

  int m_nBlackTexID;
  char m_sGetLevelFilePathTmpBuff[MAX_PATH_LENGTH];
  char m_szLevelFolder[_MAX_PATH];

  bool m_bOcean; // todo: remove

	Vec3 m_vSkyHightlightPos;
	Vec3 m_vSkyHightlightCol;
	float m_fSkyHighlightSize;
	Vec3 m_vAmbGroundCol;
	float m_fAmbMaxHeight;
	float m_fAmbMinHeight;
	IPhysicalEntity* m_pGlobalWind;
	bool m_bCameraUnderWater;
  uint8 m_nOceanRenderFlags[2];
  Vec3  m_vPrevMainFrameCamPos;
  bool m_bContentPrecacheRequested;
  
  // interfaces
  IPhysMaterialEnumerator * m_pPhysMaterialEnumerator;

  // data containers
  PodArray<CDLight*> m_lstDynLights;
	PodArray<CDLight*> m_lstDynLightsNoLight;
	int m_nRealLightsNum;
  int m_nRenderLightsNum;
  
  PodArray<ILightSource*> m_lstStaticLights;
	PodArray<PodArray<struct ILightSource*>*> m_lstAffectingLightsCombinations;

#define MAX_LIGHTS_NUM 32
  PodArray<CCamera> m_arrLightProjFrustums;

	class CTimeOfDay*    m_pTimeOfDay;

  ICVar*					m_pLightQuality;
    
  bool            m_bHRDEnabled;  
  ICVar           *m_pResetSfx;

  ICVar           *m_pHRDRendering;

	IFFont          *m_pConsolepFont;

	class CColorBleeding* m_pColorBleeding;

	// FPS for savelevelstats 

	float m_fAverageFPS;
	float m_fMinFPS;
	float m_fMaxFPS;
	long  m_nFramesCount;


	ITexture*	m_ptexIconLowMemoryUsage;
	ITexture*	m_ptexIconAverageMemoryUsage;
	ITexture*	m_ptexIconHighMemoryUsage;
	ITexture*	m_ptexIconEditorConnectedToConsole;

// not sorted
  
  //! Saving of cgf file
  bool WriteMaterials(TArray<CHUNK_HEADER>& Chunks, TArray<IShader *>& Shaders, FILE *out, int &MatChunk);
  bool WriteNodes(TArray<CHUNK_HEADER>& Chunks, TArray<NODE_CHUNK_DESC>& Nodes, FILE *out, TArray<SNodeInfo>& NI, int& MatChunk, int& ExpFrame, std::vector<IStatObj *>& pObjs);
  bool WriteMesh(TArray<CHUNK_HEADER>& Chunks, TArray<NODE_CHUNK_DESC>& Nodes, TArray<IShader *>& Shaders, FILE *out, TArray<SNodeInfo>& NI, int& MatChunk, int& ExpFrame);
  bool WriteNodeMesh(int nNode, MESH_CHUNK_DESC *chunk, FILE *out, TArray<IShader *>& Shaders, TArray<SNodeInfo>& NI, struct CStatObj *pObj);
  bool WriteLights(TArray<CHUNK_HEADER>& Chunks, TArray<NODE_CHUNK_DESC>& Nodes, FILE *out, std::vector<IStatObj *>& pObjs);

	void LoadTimeOfDaySettingsFromXML( XmlNodeRef node );
  char * GetXMLAttribText(XmlNodeRef pInputNode, const char * szLevel1,const char * szLevel2,const char * szDefaultValue);
	char * GetXMLAttribText( XmlNodeRef pInputNode, const char* szLevel1, const char* szLevel2, const char* szLevel3, const char* szDefaultValue );

	// without calling high level functions like panorama screenshot 
	void RenderInternal(const int nRenderFlags, const CCamera &cam, const char *szDebugName, const int dwDrawFlags, const int nFilterFlags);

  void RegisterLightSourceInSectors(CDLight * pDynLight, int nSID);

  bool IsCameraAnd3DEngineInvalid(const CCamera cam, const char * szCaller);

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	

	void ResetCasterCombinationsCache();

	void FindPotentialLightSources();
	void DeleteAllStaticLightSources();
	void LoadParticleEffects( XmlNodeRef &levelDataRoot );

	void UpdateSunLightSource();

	class CCullBuffer* m_pCoverageBuffer;
	struct CLightEntity * m_pSun;

	void UpdateMoonDirection();

	// Copy objects from tree
	uint32 CopyObjectsByType( EERType objType, const AABB *pBox, IRenderNode **pObjects );
	uint32 CopyObjects(const AABB *pBox, IRenderNode **pObjects );

public:
  
  void SetupLightScissors(CDLight *pLight);
  void SetupIndirectLights(CDLight *pLight);

  bool IsTerrainSyncLoad() { return m_bContentPrecacheRequested && GetCVars()->e_LevelAutoPrecacheTerrainAndProcVeget; }
  bool IsShadersSyncLoad() { return m_bContentPrecacheRequested && GetCVars()->e_LevelAutoPrecacheTexturesAndShaders; }
  bool IsStatObjSyncLoad() { return m_bContentPrecacheRequested && GetCVars()->e_LevelAutoPrecacheCgf; }

	typedef std::map<uint64,PodArray<ShadowMapFrustum*>*> ShadowFrustumListsCache;
	ShadowFrustumListsCache m_FrustumsCache[2];
	typedef std::map<uint64,int> ShadowFrustumListsCacheUsers;
	ShadowFrustumListsCacheUsers m_FrustumsCacheUsers[2];

	class CStatObjFoliage *m_pFirstFoliage,*m_pLastFoliage;
	PodArray<SEntInFoliage> m_arrEntsInFoliage;
	void RemoveEntInFoliage(int i, IPhysicalEntity *pent=0);

  PodArray<class CVoxelObject*> m_lstVoxelObjectsForUpdate;
  PodArray<class CRoadRenderNode*> m_lstRoadRenderNodesForUpdate;

	struct ILightSource * GetSunEntity();
	PodArray<struct ILightSource*> * GetAffectingLights(const AABB & bbox, bool bAllowSun);
	void UregisterLightFromAccessabilityCache(ILightSource * pLight);
	void OnCasterDeleted(IShadowCaster*pCaster);

	CCullBuffer * GetCoverageBuffer() { return m_pCoverageBuffer; }

	void UpdateScene();
  void SortForShadowMask();
	void UpdateLightSources();
	void PrepareLightSourcesForRendering_0();
  void PrepareLightSourcesForRendering_1();
	void InitShadowFrustums();

  void FreeLightSourceComponents(CDLight *pLight, bool bDeleteLight = true);
	void RemoveEntityLightSources(IRenderNode * pEntity);

  void CheckPhysicalized(const Vec3 & vBoxMin, const Vec3 & vBoxMax);

  VIRTUAL PodArray<CDLight*> * GetDynamicLightSources() { return &m_lstDynLights; }  

  int GetRealLightsNum() { return m_nRealLightsNum; }
  int & GetRenderLightsNum() { return m_nRenderLightsNum; }
	void SetupClearColor();
  void CheckAddLight(CDLight*pLight);

	void DrawTextRightAligned( const float x, const float y, const char * format, ...) PRINTF_PARAMS(4, 5);
	void DrawTextRightAligned( const float x, const float y, const float scale, const ColorF &color,const char * format, ...) PRINTF_PARAMS(6, 7);

	float GetLightAmount(CDLight * pLight, const AABB & objBox);
    
	IStatObj * CreateStatObj();

	IStatObj * UpdateDeformableStatObj(IGeometry *pPhysGeom, bop_meshupdate *pLastUpdate=0, IFoliage *pSrcFoliage=0);
	IStatObj * SmearStatObj(IStatObj *pObj, const Vec3 &pthit,const Vec3 &dir, float r,float depth);

	// Creates a new indexed mesh.
	IIndexedMesh* CreateIndexedMesh();

	void InitMaterialDefautMappingAxis(IMaterial * pMat);    

	VIRTUAL ITerrain * GetITerrain() { return (ITerrain*)m_pTerrain; }
	VIRTUAL IVisAreaManager * GetIVisAreaManager() { return (IVisAreaManager*)m_pVisAreaManager; }
	
  IVoxTerrain * GetIVoxTerrain() { return (IVoxTerrain*)m_pVoxTerrain; }
  IVoxTerrain * CreateVoxTerrain(const SVoxTerrainInfo & info);
  void DeleteVoxTerrain();
	ITerrain * CreateTerrain(const STerrainInfo & TerrainInfo);
	void DeleteTerrain();
	bool LoadTerrain(XmlNodeRef pDoc, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, int nSID, Vec3 vSegmentOrigin);
  bool LoadVoxTerrain(XmlNodeRef pDoc, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, int nSID, Vec3 vSegmentOrigin);
	bool LoadVisAreas(std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable);
	bool LoadUsedShadersList();
	bool PrecreateDecals();
	void LoadPhysicsData();

	void GetVoxelRenderNodes(struct IRenderNode**pRenderNodes, int & nCount);
 
  VIRTUAL float GetLightAmountInRange(const Vec3 &pPos, float fRange, bool bAccurate = 0);
    
	VIRTUAL void PrecacheLevel(bool bPrecacheAllVisAreas, Vec3 * pPrecachePoints, int nPrecachePointsNum);
  VIRTUAL void ProposeContentPrecache() { m_bContentPrecacheRequested = true; }
      
	VIRTUAL ITimeOfDay* GetTimeOfDay();
	//! [GDC09]: Return SkyBox material
	VIRTUAL IMaterial* GetSkyMaterial();
	bool IsHDRSkyMaterial(IMaterial* pMat) const;

	bool RenderPotentialOccluders(CCullBuffer & rCB, const CCamera & viewCam, bool bResetAffectedLights);
	bool RenderVisAreaPotentialOccluders(CVisArea * pThisArea, CCullBuffer & rCB, const CCamera & viewCam, bool bResetAffectedLights);

	VIRTUAL void SetGlobalParameter( E3DEngineParameter param,const Vec3 &v );
	VIRTUAL void GetGlobalParameter( E3DEngineParameter param,Vec3 &v );

	VIRTUAL int SaveStatObj(IStatObj *pStatObj, TSerialize ser);
	VIRTUAL IStatObj *LoadStatObj(TSerialize ser);

	VIRTUAL bool CheckIntersectClouds(const Vec3 & p1, const Vec3 & p2);
	VIRTUAL void OnRenderMeshDeleted(IRenderMesh * pRenderMesh);
	VIRTUAL bool RayObjectsIntersection2D( Vec3 vStart, Vec3 vEnd, Vec3 & vHitPoint, EERType eERType );

  VIRTUAL bool IsAmbientOcclusionEnabled();
  VIRTUAL const char * GetVoxelEditOperationName(EVoxelEditOperation eOperation);

  VIRTUAL void SetGetLayerIdAtCallback(IGetLayerIdAtCallback * pCallBack) { m_pGetLayerIdAtCallback = pCallBack; }
  static IGetLayerIdAtCallback * m_pGetLayerIdAtCallback;
  
	VIRTUAL IParticleManager* GetParticleManager() { return m_pPartManager; }

  virtual void RegisterForStreaming(IStreamable*pObj);
  virtual void UnregisterForStreaming(IStreamable*pObj);

  void MarkRNTmpDataPoolForReset() { m_bResetRNTmpDataPool = true; }

  static void GetObjectsByTypeGlobal(PodArray<IRenderNode*> & lstObjects, EERType objType, const AABB * pBBox, float fViewDist = -1);
  static void MoveObjectsIntoListGlobal(PodArray<SRNInfo> * plstResultEntities, const AABB * pAreaBox, bool bRemoveObjects = false, bool bSkipDecals = false, bool bSkip_ERF_NO_DECALNODE_DECALS = false, bool bSkipDynamicObjects = false, EERType eRNType = eERType_TypesNum);

	PodArray<class COctreeNode *> m_pObjectsTree;
//  class CSceneTree * m_pSceneTree;

	int m_idMatLeaves; // for shooting foliages
  bool m_bResetRNTmpDataPool;

  float m_fRefreshSceneDataCVarsSumm;

  PodArray<IRenderNode*> m_lstAlwaysVisible;
  PodArray<IRenderNode *> m_lstKilledVegetations;

  PodArray<SImageInfo> m_arrBaseTextureData;
//#define MAX_SURFACE_TYPES_COUNT 32
  //int m_arrSurfTypeMapping[MAX_SURFACE_TYPES_COUNT];

	CRNTmpData m_LTPRootFree, m_LTPRootUsed;
  void CreateRNTmpData(CRNTmpData ** ppInfo, IRenderNode * pRNode);
  ILINE void CheckCreateRNTmpData(CRNTmpData ** ppInfo, IRenderNode * pRNode)
  {
    if(!(*ppInfo))
      CreateRNTmpData(ppInfo, pRNode);
    (*ppInfo)->nLastUsedFrameId = GetMainFrameID();
  }
	void FreeRNTmpData(CRNTmpData ** ppInfo);
	void UpdateRNTmpDataPool(bool bFreeAll);
	void FreeRNTmpDataPool();
  
//  CPoolAllocator<CRNTmpData, 512> m_RNTmpDataPools;

  void UpdateStatInstGroups();
  void ProcessOcean(const int dwDrawFlags);
  void ReRegisterKilledVegetationInstances();
  Vec3 GetEntityRegisterPoint( IRenderNode* pEnt );

  VIRTUAL void RenderRenderNode(IShadowCaster * pRNode, SRenderObjectModifier * pROModifier);
  void ProcessCVarsChange();
  int GetGeomDetailScreenRes();
  int GetBlackTexID() { return m_nBlackTexID; }

	VIRTUAL void SyncProcessStreamingUpdate(); 

	VIRTUAL void SetScreenshotCallback(IScreenshotCallback* pCallback);

//  void CheckUpdateImageInfos();
  SImageSubInfo * RegisterImageInfo(byte ** pMips, int nDim, const char * pName);
  SImageSubInfo * GetImageInfo(const char * pName);
  std::map<string,SImageSubInfo*> m_imageInfos;
  byte ** AllocateMips(byte*pImage, int nDim, byte ** pImageMips);
	IScreenshotCallback* m_pScreenshotCallback;
};

struct IsoTreeNodeData
{
public:
  IsoTreeNodeData() { mcIndex=0; }
  uint16 mcIndex;

  AUTO_STRUCT_INFO
};

struct OctNodeChunk
{
  int nHasChilds;
  IsoTreeNodeData nodeData;

  AUTO_STRUCT_INFO
};

struct SIsoTreeInfo
{
  AABB aabbTerrain;
  float fOceanWaterLevel;
  int maxDepth;
  int nSVoxValueSize;
  int nModId;

  AUTO_STRUCT_INFO
};

struct SIsoTreeChunkHeader
{
  int nVersion;
  int nChunkSize;
  SIsoTreeInfo TerrainInfo;

  AUTO_STRUCT_INFO
};

typedef float ISO_DOUBLE;

struct SSurfTypeInfo
{
  SSurfTypeInfo() { ZeroStruct(*this); }
  SSurfTypeInfo(int nSurfType) { *this = nSurfType; }

  void Normalize()
  {
    float fSumm = 0;

    for(int c=0; c<4; c++)
      fSumm += (float)we[c];

    for(int c=0; c<4; c++)
      we[c] = (byte)SATURATEB((float)we[c] * (255.f/fSumm));
  }

  int GetDomSType() const
  {
    int nRes = 0;
    int fW = 0;
    for(int i=0; i<4; i++)
    {
      if(we[i]>fW)
      {
        fW = we[i];
        nRes = i;
      }
    }
    return ty[nRes];
  }

#define MAX_VOXTER_STYPES_NUM 32

  inline const SSurfTypeInfo & Lerp(float t, const SSurfTypeInfo & v0, const SSurfTypeInfo & v1)
  {
    byte arrUnroll[MAX_VOXTER_STYPES_NUM];
    memset(arrUnroll,0,sizeof(arrUnroll));

    int s_min = MAX_VOXTER_STYPES_NUM-1;
    int s_max = 0;

    for(int c=0; c<4; c++)
    {
      const byte ty = v0.ty[c];
      arrUnroll[ty] += (byte)((1.f-t)*(float)v0.we[c]);
      if(arrUnroll[ty])
      {
        s_min = min(s_min, ty);
        s_max = max(s_max, ty);
      }
    }

    for(int c=0; c<4; c++)
    {
      const byte ty = v1.ty[c];
      arrUnroll[ty] += (byte)((    t)*(float)v1.we[c]);
      if(arrUnroll[ty])
      {
        s_min = min(s_min, ty);
        s_max = max(s_max, ty);
      }
    }

    for(int c=0; c<4; c++)
    {
      we[c] = 0;

      for(int s=s_min; s<=s_max; s++)
      {
        if(arrUnroll[s]>we[c])
        {
          we[c] = arrUnroll[s];
          ty[c] = s;
        }
      }

      arrUnroll[ty[c]] = 0;
    }

    Normalize();

    return *this;
  }

  const SSurfTypeInfo & operator = (int nSurfType)
  {
    ZeroStruct(*this);
    assert(nSurfType>=0 && nSurfType<MAX_VOXTER_STYPES_NUM);
    we[0] = 255;
    ty[0] = nSurfType;
    return *this;
  }

  bool operator != (const SSurfTypeInfo & o) const
  {
    return memcmp(this,&o,sizeof(o))!=0;
  }

  bool operator == (const SSurfTypeInfo & o) const
  {
    return memcmp(this,&o,sizeof(o))==0;
  }

  byte we[4];
  byte ty[4];

  AUTO_STRUCT_INFO
};

struct SVoxValueOld
{
  float val;
  uint8 surf_type;
  uint8 color[3];

  AUTO_STRUCT_INFO
};

struct SVoxValueOld2
{
  float val;
  SSurfTypeInfo surf_type;
  uint8 color[4];

  AUTO_STRUCT_INFO
};

struct SVoxValue
{
  SVoxValue()
  {
    ZeroStruct(*this);
    val=1.f;
  }

  static SVoxValue Average(const SVoxValue & v0, const SVoxValue & v1)
  {
    SVoxValue result;

    const SVoxValue * ppVals[] = { &v0,&v1 };
    result.surf_type = AverageSurfTypeInfoPtr(ppVals, 2);

    result.val = (v0.val + v1.val)*.5f;

    for(int i=0; i<3; i++)
      result.color[i] = (uint8)(((float)v0.color[i] + (float)v1.color[i])*.5f);

    for(int i=0; i<3; i++)
      result.normal[i] = (uint8)(((float)v0.normal[i] + (float)v1.normal[i])*.5f);

    return result;
  }

  static SVoxValue Average(const SVoxValue & v0, const SVoxValue & v1, const SVoxValue & v2, const SVoxValue & v3)
  {
    SVoxValue result;

    const SVoxValue * ppVals[] = { &v0,&v1,&v2,&v3 };
    result.surf_type = AverageSurfTypeInfoPtr(ppVals, 4);

    result.val = (v0.val + v1.val + v2.val + v3.val)*.25f;

    for(int i=0; i<3; i++)
      result.color[i] = (uint8)(((float)v0.color[i] + (float)v1.color[i] + (float)v2.color[i] + (float)v3.color[i])*.25f);

    for(int i=0; i<3; i++)
      result.normal[i] = (uint8)(((float)v0.normal[i] + (float)v1.normal[i] + (float)v2.normal[i] + (float)v3.normal[i])*.25f);

    return result;
  }

  static const SSurfTypeInfo AverageSurfTypeInfoPtr(const SVoxValue ** ppVals, int nCount)
  {
    assert(nCount<MAX_VOXTER_STYPES_NUM);

    uint16 arrUnroll[MAX_VOXTER_STYPES_NUM];
    memset(arrUnroll,0,sizeof(arrUnroll));

    int s_min = MAX_VOXTER_STYPES_NUM-1;
    int s_max = 0;

    for(int e=0; e<nCount; e++)
    {
      const SSurfTypeInfo & v = ppVals[e]->surf_type;
      for(int c=0; c<4; c++)
      {
        const byte ty = v.ty[c];
        arrUnroll[ty] += v.we[c];
        if(arrUnroll[ty])
        {
          s_min = min(s_min, ty);
          s_max = max(s_max, ty);
        }
      }
    }

    SSurfTypeInfo res;

    for(int c=0; c<4; c++)
    {
      res.we[c] = 0;

      for(int s=s_min; s<=s_max; s++)
      {
        if(arrUnroll[s]/nCount>res.we[c])
        {
          res.we[c] = arrUnroll[s]/nCount;
          res.ty[c] = s;
        }
      }

      arrUnroll[res.ty[c]] = 0;
    }

    res.Normalize();

    return res;
  }

  static const SSurfTypeInfo AverageSurfTypeInfo(const SVoxValue * pVals, int nCount)
  {
    assert(nCount<MAX_VOXTER_STYPES_NUM);

    uint16 arrUnroll[MAX_VOXTER_STYPES_NUM];
    memset(arrUnroll,0,sizeof(arrUnroll));

    int s_min = MAX_VOXTER_STYPES_NUM-1;
    int s_max = 0;

    for(int e=0; e<nCount; e++)
    {
      const SSurfTypeInfo & v = pVals[e].surf_type;
      for(int c=0; c<4; c++)
      {
        const byte ty = v.ty[c];
        arrUnroll[ty] += v.we[c];
        if(arrUnroll[ty])
        {
          s_min = min(s_min, ty);
          s_max = max(s_max, ty);
        }
      }
    }

    SSurfTypeInfo res;

    for(int c=0; c<4; c++)
    {
      res.we[c] = 0;

      for(int s=s_min; s<=s_max; s++)
      {
        if(arrUnroll[s]/nCount>res.we[c])
        {
          res.we[c] = arrUnroll[s]/nCount;
          res.ty[c] = s;
        }
      }

      arrUnroll[res.ty[c]] = 0;
    }

    res.Normalize();

    return res;
  }

  static SVoxValue Average(const SVoxValue * pVals, int nCount)
  {
    SVoxValue result;

    float fValSumm = 0;
    ColorF colSumm = Col_Black;
    ColorF normSumm = Col_Black;

    for(int i=0; i<nCount; i++)
    {
      fValSumm += pVals[i].val;

      colSumm[0] += pVals[i].color[0];
      colSumm[1] += pVals[i].color[1];
      colSumm[2] += pVals[i].color[2];

      normSumm[0] += pVals[i].normal[0];
      normSumm[1] += pVals[i].normal[1];
      normSumm[2] += pVals[i].normal[2];
    }

    result.surf_type = AverageSurfTypeInfo(pVals,nCount);

    result.val = fValSumm/(float)nCount;

    colSumm /= (float)nCount;
    normSumm /= (float)nCount;

    result.color[0] = (uint8)colSumm[0];
    result.color[1] = (uint8)colSumm[1];
    result.color[2] = (uint8)colSumm[2];

    result.normal[0] = (uint8)normSumm[0];
    result.normal[1] = (uint8)normSumm[1];
    result.normal[2] = (uint8)normSumm[2];

    return result;
  }

  const bool operator == (const SVoxValue& o)
  {
    return !memcmp( this, &o, sizeof(o) );
  }

  const bool operator > (const SVoxValue& o)
  {
    return val > o.val;
  }

  const bool operator > (const float& oVal)
  {
    return val > oVal;
  }

  const bool operator < (const SVoxValue& o)
  {
    return val < o.val;
  }

  const bool operator < (const float& oVal)
  {
    return val < oVal;
  }

  float val;
  SSurfTypeInfo surf_type;
  uint8 color[4];
  uint8 normal[4];

  AUTO_STRUCT_INFO
};

struct SFlatnessItem
{
  SFlatnessItem() { first.Set(0,0,0); second=0; } 
  Vec3_tpl<ISO_DOUBLE> first;
  ISO_DOUBLE second;

  AUTO_STRUCT_INFO
};

struct SVoxValueCache
{
  SVoxValueCache() { node=0; }
  SVoxValue values[2][2][2];
  Vec3 vNormal;
  class OctNode * node;
};

#endif // C3DENGINE_H
