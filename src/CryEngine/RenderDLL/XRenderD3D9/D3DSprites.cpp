#include "StdAfx.h"
#include "DriverD3D.h"

//=========================================================================================

#include <IStatObj.h>
#include <I3DEngine.h>
#include <IEntityRenderState.h>





bool WriteTGA(byte *dat, int wdt, int hgt, char *name);

static SSpriteInfo *sSPInfo;
static int snSPInfo;
static int snSPInfoMax;
extern CTexture *gTexture;

static float sfTableTCFloat[129];
static int snAtlasSize = 0;

static void sInitTCTable()
{
  if (snAtlasSize == gRenDev->CV_r_texatlassize)
    return;
  snAtlasSize = gRenDev->CV_r_texatlassize;

  int i;
  const float fByte2TexMul = 1.0f/128.0f;
  float fHalfInvWidth = 0.5f/gRenDev->CV_r_texatlassize;
  for (i=0; i<=128; i++)
  {
    sfTableTCFloat[i] = i*fByte2TexMul-fHalfInvWidth;
  }
}

void sSetViewPort(int nCurX, int nCurY, int nSpriteRes)
{
	// one pixel border for proper border handling
  gcpRendD3D->RT_SetViewport(nCurX*nSpriteRes+1, nCurY*nSpriteRes+1, nSpriteRes-3, nSpriteRes-3);
}

static const float sf60Sin = sin(60.0f*(gf_PI/180.0f));
static const float sf60Cos = cos(60.0f*(gf_PI/180.0f));

//#define SPRITES_2_TEXTURES 1

void sFlushSprites(SSpriteGenInfo *pSGI, int nCurX, int nCurY, CTexture *pSrcTex, int nTexSizeInt, int nTexHeightFinal)
{
  int n = 0;
  int ncX = 0;
  int ncY = 0;
  CD3D9Renderer *r = gcpRendD3D;
  STexState pTexState;
#ifndef SPRITES_2_TEXTURES
  pTexState.SetFilterMode(FILTER_POINT);        
#else
  pTexState.SetFilterMode(FILTER_LINEAR);        
#endif
  pTexState.SetClampMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);
  int nTS = CTexture::GetTexState(pTexState);
  r->Set2DMode(true, 1, 1);
  CTexture *pCurRT = r->m_pNewTarget[0]->m_pTex;

  SDynTexture2 *pNewTex;
  while (true)
  {
    SSpriteGenInfo *pSG = &pSGI[n++];
    SSpriteInfo *pSP = &sSPInfo[pSG->nSP];
    /*if ((int)pSP->m_vPos[0] == 1049 && (int)pSP->m_vPos[1] == 2069 && (int)pSP->m_vPos[2] == 248)
    {
      int nnn = 0;
    }*/

    // dilate rendered sprite and put into sprite texture -------------------
    pNewTex = (SDynTexture2 *)*pSG->ppTexture;
    assert (pNewTex);
    if (!pNewTex)
    {
      r->Set2DMode(false, 1, 1);
      return;
    }

		pNewTex->Update(pNewTex->GetWidth(), pNewTex->GetHeight());

    if (!CTexture::IsTextureExist(pNewTex->m_pTexture))
    {
      assert(0);
      SAFE_RELEASE(pNewTex);
      *pSG->ppTexture = NULL;
      return;
    }
    *pSG->ppTexture = pNewTex;

    uint32 nX, nY, nW, nH;
    pNewTex->GetSubImageRect(nX, nY, nW, nH);
#ifdef SPRITES_2_TEXTURES
    nW >>= 1;
#endif

    CTexture *pT = (CTexture *)pNewTex->GetTexture();
    pT->m_nAccessFrameID = r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
    pSP->m_pTex = pNewTex;
    const uint32 dwWidth = pT->GetWidth();
    const uint32 dwHeight = pT->GetHeight();

    pSP->m_ucTexCoordMinX = (nX*128)/dwWidth;					assert((pSP->m_ucTexCoordMinX*dwWidth)/128==nX);
    pSP->m_ucTexCoordMinY = (nY*128)/dwHeight;				assert((pSP->m_ucTexCoordMinY*dwHeight)/128==nY);
    pSP->m_ucTexCoordMaxX = ((nX+nW)*128)/dwWidth;		assert((pSP->m_ucTexCoordMaxX*dwWidth)/128==nX+nW);
    pSP->m_ucTexCoordMaxY = ((nY+nH)*128)/dwHeight;		assert((pSP->m_ucTexCoordMaxY*dwHeight)/128==nY+nH);

    CShader *pPostEffects = CShaderMan::m_shPostEffects;

		gcpRendD3D->FX_ApplyShaderQuality(eST_PostProcess);		// needed for GetShaderQuality()

    r->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Generating '%s' - %s (%d, %d, %d, %d) (%d)\n", pT->GetName(), pNewTex->IsSecondFrame() ? "Second" : "First", nX, nY, nW, nH, r->GetFrameID(false));

    if (pCurRT != pT)
    {
      pCurRT = pT;
#ifndef XENON
      pNewTex->SetRT(0, false, r->FX_GetDepthSurface(dwWidth, dwHeight, false));


#endif
    }
    else
      pNewTex->SetRectStates();
    bool bOk;
#ifdef SPRITES_2_TEXTURES
    bOk = pPostEffects->FXSetTechnique("Dilate2");
#else
    bOk = pPostEffects->FXSetTechnique("Dilate");
#endif
    r->FX_Commit();

    assert(bOk);

    uint32 nPasses = 0;
    pPostEffects->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    pPostEffects->FXBeginPass(0);

    r->EF_SetState(GS_NODEPTHTEST);

    float fInvBrightnessMultiplier = 1.0f/pSG->fBrightnessMultiplier;

    Vec4 v;
    v[1] = v[2] = v[3] = 0;
    v[0] = fInvBrightnessMultiplier;
		static CCryName vDilateParamsName("vDilateParams");
    pPostEffects->FXSetPSFloat(vDilateParamsName, &v, 1); 

    float fTexSize = (float)nTexSizeInt;
    float fHalfX = 1.0f / (float)pSrcTex->GetWidth();
    float fHalfY = 1.0f / (float)pSrcTex->GetHeight();
    float fSizeX = fTexSize * fHalfX;
    float fSizeY = fTexSize * fHalfY;
    float fOffsX = (float)ncX*fSizeX;
    float fOffsY = (float)ncY*fSizeY;
#if defined(DIRECT3D9) || defined(OPENGL)
    fOffsX += fHalfX*0.5f;
    fOffsY += fHalfY*0.5f;
#endif

    //fHalfX *= 0.5f;
    //fHalfY *= 0.5f;

    v[0] = fHalfX*pNewTex->GetHeight()/pNewTex->GetWidth();
    v[1] = fHalfY;
    v[2] = fSizeX;
		static CCryName vPixelOffsetName("vPixelOffset");
    pPostEffects->FXSetPSFloat(vPixelOffsetName, &v, 1);

#ifdef SPRITES_2_TEXTURES
    fSizeX *= 2;
#endif

    pSrcTex->Apply(0, nTS);

    float fZ = 0.5f;
    r->DrawQuad3D(Vec3(0,0,fZ), Vec3(1,0,fZ), Vec3(1,1,fZ), Vec3(0,1,fZ), Col_White, fOffsX, fOffsY, fOffsX+fSizeX, fOffsY+fSizeY);

    pPostEffects->FXEndPass();

//    pNewTex->RestoreRT(0, false);

    ncX += 2;
    if ((ncX+2)*nTexSizeInt > pSrcTex->GetWidth())
    {
      ncX = 0;
      ncY++;
    }
    if (ncX==nCurX && ncY==nCurY)
      break;
  }

  r->Set2DMode(false, 1, 1);
  gcpRendD3D->EF_Scissor(false, 0, 0, 0, 0);
}

// Overrides of "StartEF / AddEF / EndEF" for rendering thread
static void sRT_StartEf (SRenderListDesc& DSC)
{
  int i, j;
  CD3D9Renderer *rd = gcpRendD3D;
  int nThreadID = rd->m_RP.m_nProcessThreadID;
  for (j=0; j<MAX_LIST_ORDER; j++)
  {
    for (i=0; i<EFSLIST_NUM; i++)
    {
      DSC.m_nStartRI[j][i] = SRendItem::RendItems(nThreadID,j,i).Num();
      DSC.m_nBatchFlags[j][i] = 0;
      DSC.m_nEndRI[j][i] = 0;
    }
  }

  rd->m_RP.m_DLights[nThreadID][SRendItem::m_RecurseLevel[nThreadID]-1].SetUse(0);
}
static void sRT_AddEf(CRendElementBase *pRE, SShaderItem& SH, CRenderObject *pObj, int nList, int nAW)
{
  CShader *pSH = (CShader *)SH.m_pShader;
  if (pSH->m_Flags2 & EF2_NODRAW)
    return;
  CD3D9Renderer *rd = gcpRendD3D;
  int nThreadID = rd->m_RP.m_nProcessThreadID;
  if (pSH->m_Flags & EF_RELOADED)
  {
    pSH->m_Flags &= ~EF_RELOADED;
    SShaderItem SHCopy = SH;
    SHCopy.PostLoad();
  }
  uint32 nBatchFlags = rd->EF_BatchFlags(SH, pObj, nThreadID, pRE);
  if (nBatchFlags & FB_TRANSPARENT)
    nList = EFSLIST_TRANSP;

  SRendItem::mfAdd(nThreadID, pRE, pObj, SH, nList, nAW, nBatchFlags);
}

static void sRT_RenderObject (IStatObj *pEngObj, SRendParams& RP)
{
  CD3D9Renderer *rd = gcpRendD3D;
  int nThreadID = rd->m_RP.m_nProcessThreadID;

  TArray <CRenderObject *>& Objs = rd->m_RP.m_TempObjects[nThreadID];
  int n = Objs.Num();
  if (n >= MAX_REND_OBJECTS)
    return;

  Objs.AddIndex(1);
  CRenderObject *pObj;
  if (!(pObj=Objs[n]))
  {
    pObj = new CRenderObject;
    Objs[n] = pObj;
    pObj->m_Id = n;
  }
  pObj->Init(nThreadID);

  pObj->m_ObjFlags |= FOB_TRANS_MASK;
  pObj->m_ObjFlags |= RP.dwFObjFlags;
  pObj->m_II.m_Matrix = *RP.pMatrix;  
  pObj->m_nMotionBlurAmount = 0;
  pObj->m_II.m_AmbColor = RP.AmbientColor;
  pObj->m_pShadowCasters=0;
  pObj->m_ObjFlags |= FOB_INSHADOW;
  pObj->m_fAlpha = RP.fAlpha;
  pObj->m_nRenderQuality = (short)(RP.fRenderQuality * 65535.0f);
  pObj->m_fDistance =	RP.fDistance;
  pObj->m_DynLMMask[nThreadID] = RP.nDLightMask;
  pObj->m_nMaterialLayers = RP.nMaterialLayersBlend;
  pObj->m_pCurrMaterial = RP.pMaterial;
  pObj->m_pRenderNode = RP.pRenderNode;
  assert(pEngObj->GetRenderMesh());
  if (!pEngObj->GetRenderMesh())
    return;
  CRenderMesh2 *pMesh = (CRenderMesh2 *)pEngObj->GetRenderMesh();
  IMaterial *pMaterial = RP.pMaterial;
  if (!pMaterial)
    pMaterial = pMesh->m_pMaterial;
  assert(pMaterial);
  if(!pMaterial)
    return;

  PodArray<CRenderChunk> *pChunks = &pMesh->m_Chunks;
  const uint32 ni = (uint32)pChunks->Count();
  for (uint32 i=0; i<ni; i++)     
  {
    CRenderChunk *pChunk = pChunks->Get(i);
    CRendElementBase *pREMesh = pChunk->pRE;

    SShaderItem& ShaderItem = pMaterial->GetShaderItem(pChunk->m_nMatID);
    if (pREMesh && ShaderItem.m_pShader && ShaderItem.m_pShaderResources)
    {
      if (((CShader *)(ShaderItem.m_pShader))->m_Flags2 & EF2_NODRAW)
        continue;

      sRT_AddEf(pREMesh, ShaderItem, pObj, EFSLIST_GENERAL, 0);  
    }
  } 
}

static void sRT_ADDDlight(CDLight *Source)
{
  CD3D9Renderer *rd = gcpRendD3D;
  int nThreadID = rd->m_RP.m_nProcessThreadID;

  Source->m_Id = rd->m_RP.m_DLights[nThreadID][SRendItem::m_RecurseLevel[nThreadID]-1].Num();
  rd->m_RP.m_DLights[nThreadID][SRendItem::m_RecurseLevel[nThreadID]-1].AddElem(*Source);
}

static void sRT_EndEf (SRenderListDesc& RLD)
{
  int i, j;
  CD3D9Renderer *rd = gcpRendD3D;
  int nThreadID = rd->m_RP.m_nProcessThreadID;
  for (j=0; j<MAX_LIST_ORDER; j++)
  {
    for (i=0; i<EFSLIST_NUM; i++)
    {
      RLD.m_nEndRI[j][i] = SRendItem::RendItems(nThreadID,j,i).Num();
    }
  }
  rd->EF_SortRenderLists(&RLD, nThreadID);
  rd->m_pRT->RC_RenderScene(0, CD3D9Renderer::FX_FlushShader_General, &RLD);
}

static void sCreateFT(bool bHDR)
{
  if (bHDR)
    CTexture::GenerateHDRMaps();
  else
    CTexture::GenerateSceneMap(eTF_A8R8G8B8);
}

static bool AreTexturesStreamed(IMaterial* pMaterial, float fMipFactor)
{
	// check texture streaming distances
	if(pMaterial)
	{
		if(IRenderShaderResources * pRes = pMaterial->GetShaderItem().m_pShaderResources)
		{
			for(int iSlot = 0;iSlot<EFTT_MAX;++iSlot)
			{
				if(SEfResTexture* pResTex = pRes->GetTexture(iSlot))
				{
					if(ITexture * pITex = pResTex->m_Sampler.m_pITex)
					{
						float fCurMipFactor = fMipFactor * pResTex->m_TexModificator.m_Tiling[0] * pResTex->m_TexModificator.m_Tiling[1];

						if(!pITex->IsParticularMipStreamed(fCurMipFactor))
							return false;
					}
				}
			}
		}
	}

	return true;
}

void CD3D9Renderer::MakeSprites(TArray<SSpriteGenInfo>& SGI)
{
	PROFILE_FRAME(MakeSprites);

  assert(m_pRT->IsRenderThread());





  int nThreadID = m_RP.m_nProcessThreadID;
  int nRecursion = SRendItem::m_RecurseLevel[nThreadID];
  SRendItem::m_RecurseLevel[nThreadID] = 2;

#if !defined(PS3) && !defined(XENON)
	bool bHDR = IsHDRModeEnabled();


#endif

  CTexture *pTempTex = bHDR ? CTexture::s_ptexHDRTarget : CTexture::s_ptexBackBuffer;
  if (!pTempTex)
  {
    sCreateFT(bHDR);
    pTempTex = bHDR ? CTexture::s_ptexHDRTarget : CTexture::s_ptexBackBuffer;
  }
  assert (CTexture::IsTextureExist(pTempTex));
  if (!CTexture::IsTextureExist(pTempTex))
    return;
  int nWidth = GetWidth();
  int nHeight = GetHeight();
  assert (pTempTex && nWidth == pTempTex->GetWidth() && nHeight == pTempTex->GetHeight());
  int nSpriteResFinal = CV_r_VegetationSpritesTexRes;  
  int nSpriteResInt = nSpriteResFinal << 1;
  int nSprX = nWidth / nSpriteResInt;
  int nSprY = nHeight / nSpriteResInt;
  int nCurX = 0;
  int nCurY = 0;
  SD3DSurface *pDepthSurfFSAA = &m_DepthBufferOrigFSAA;
  SD3DSurface *pDepthSurf = &m_DepthBufferOrig;
  if (m_RP.m_FSAAData.Type > 1)
  {
    assert(pTempTex->GetFlags() & FT_USAGE_RENDERTARGET);
  }

  SRendParams rParms;
  CShader *sh = m_RP.m_pShader;
	int nTech = m_RP.m_nShaderTechnique;
  SRenderShaderResources *pRes = m_RP.m_pShaderResources;
  CRenderObject *pObj = m_RP.m_pCurObject;
  SShaderTechnique *pTech = m_RP.m_pCurTechnique;
  SShaderTechnique *pRootTech = m_RP.m_pRootTechnique;
  CRendElementBase *pRE = m_RP.m_pRE;

  //int nPrevStr = CV_r_texturesstreamingsync;
  int nPrevAsync = CV_r_shadersasynccompiling;
  //CV_r_texturesstreamingsync = 1;
  CV_r_shadersasynccompiling = 0;
  int nPrevZpass = CV_r_usezpass;
  CV_r_usezpass = 0;

  rParms.dwFObjFlags |= FOB_TRANS_MASK;
  rParms.nDLightMask = 1;
  rParms.fRenderQuality = 0.0f;
  rParms.pRenderNode = (struct IRenderNode*)(uint32)-1; // avoid random skipping of rendering

  Vec3 cSkyBackup = gEnv->p3DEngine->GetSkyColor();
  Vec3 cSunBackup = gEnv->p3DEngine->GetSunColor();
  ColorF white(1,1,1,0);

  CDLight light;
  light.SetLightColor(cSunBackup);
  light.m_SpecMult = 0;
  light.m_fRadius  = 100000000;
  light.m_Flags |= DLF_DIRECTIONAL | DLF_SUN | DLF_THIS_AREA_ONLY | DLF_LM | DLF_SPECULAROCCLUSION | DLF_CASTSHADOW_MAPS;

  int vX, vY, vWidth, vHeight;
  GetViewport(&vX, &vY, &vWidth, &vHeight);

  CCamera origCam = GetCamera();
  uint32 saveFlags = m_RP.m_TI[nThreadID].m_PersFlags;
  uint32 saveFlags2 = m_RP.m_TI[nThreadID].m_PersFlags2;
  m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_MAKESPRITE;
  m_RP.m_TI[nThreadID].m_PersFlags2 &= ~(RBPF2_NOALPHABLEND| RBPF2_NOALPHATEST);

  EF_PushFog();
  EF_PushMatrix();
  m_RP.m_TI[nThreadID].m_matProj->Push();
  EnableFog(false);

  const Vec3 vEye(0,0,0);
  const Vec3 vAt(-1,0,0);
  const Vec3 vUp(0,0,1);
  Matrix44A mView;  
  mathMatrixLookAt(&mView, vEye, vAt, vUp);
  FX_PushRenderTarget(0, pTempTex, pDepthSurf);
  RT_SetViewport(0, 0, m_width, m_height);
  EF_ClearBuffers(FRT_CLEAR, &white);
  FX_Commit();

  int nStart = 0;
  m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumSpriteUpdates += SGI.Num();
  for (uint32 i=0; i<SGI.Num(); i++)
  {
    SSpriteGenInfo &SG = SGI[i];
    SSpriteInfo *pSP = &sSPInfo[SG.nSP];
    /*if ((int)pSP->m_vPos[0] == 1049 && (int)pSP->m_vPos[1] == 2069 && (int)pSP->m_vPos[2] == 248)
    {
      int nnn = 0;
    }*/
    if (CV_r_VegetationSpritesNoGen && CTexture::s_ptexNoTexture)
    {
      SAFE_DELETE(*SG.ppTexture);
      SSpriteInfo *pSpriteInfo = &sSPInfo[SG.nSP];
      pSpriteInfo->m_pTex = NULL;
      continue;
    }
    if (!nCurX && !nCurY && i)
    {
      FX_SetRenderTarget(0, pTempTex, pDepthSurf);
      RT_SetViewport(0, 0, m_width, m_height);
      EF_ClearBuffers(FRT_CLEAR, &white);
      FX_Commit();
    }

    rParms.nMaterialLayers = SG.nMaterialLayers;  // pass material layers info
    rParms.pMaterial = SG.pMaterial;

    assert(SG.fBrightnessMultiplier>0);
#ifdef DO_RENDERLOG
	  if (CV_r_log)
		  Logv(SRendItem::m_RecurseLevel[nThreadID], "****************** CD3D9Renderer::MakeSprite - Begin ******************\n");
#endif
	  float fRadiusHors = SG.pStatObj->GetRadiusHors();
	  float fRadiusVert = SG.pStatObj->GetRadiusVert();

	  Vec3 vCenter = SG.pStatObj->GetVegCenter();

	  float fFOV = 0.565f/SG.fGenDist*200.f*(gf_PI/180.0f);
	  float fDrawDist = fRadiusVert*SG.fGenDist;

	  // make fake camera to pass some camera into to the rendering pipeline
	  CCamera tmpCam = origCam;
	  Matrix33 matRot = Matrix33::CreateOrientation(vAt-vEye, vUp, 0);
	  tmpCam.SetMatrix(Matrix34(matRot, Vec3(0,0,0)));
	  tmpCam.SetFrustum(nSpriteResInt, nSpriteResInt, fFOV, max(0.1f, fDrawDist-fRadiusHors), fDrawDist+fRadiusHors);
	  SetCamera(tmpCam);

    Matrix44A *m = m_RP.m_TI[nThreadID].m_matProj->GetTop();
    mathMatrixPerspectiveFov(m, fFOV, fRadiusHors/fRadiusVert, fDrawDist-fRadiusHors, fDrawDist+fRadiusHors);
    m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_FP_MATRIXDIRTY;

    *m_RP.m_TI[nThreadID].m_matView->GetTop() = mView;
    m_CameraZeroMatrix[nThreadID] = mView;

    RT_SetCameraInfo();
  
	  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	  // Start sprite gen
	  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  	float globalMultiplier( 1 );

	  // setup environment ------------------
	  Matrix34A matTrans;
	  matTrans.SetIdentity();
	  matTrans.SetTranslation(Vec3(-fDrawDist,0,0));

	  Matrix34A matRotation;
	  matRotation = matRotation.CreateRotationZ((SG.fAngle)*(gf_PI/180.0f));

//	  Matrix34A matRotation2;
//	  matRotation2 = matRotation2.CreateRotationY((SG.fAngle2)*(gf_PI/180.0f));
//	  matRotation = matRotation2 * matRotation;

	  Matrix34A matCenter; matCenter.SetIdentity(); matCenter.SetTranslation(-vCenter);
	  matCenter = matRotation * matCenter;

	  Matrix34 mat = matTrans * matCenter;

	  rParms.pMatrix = &mat;

	  // sun (rotate sun direction)
	  light.m_Origin = matRotation.TransformVector(gEnv->p3DEngine->GetSunDir());

    SRenderListDesc RLD;

    {
      rParms.AmbientColor = Col_Black;
      //gEnv->p3DEngine->SetSkyColor(Vec3(0,0,0));

      sRT_StartEf(RLD);  
      sSetViewPort(nCurX, nCurY, nSpriteResInt);
      sRT_ADDDlight(&light);
      sRT_RenderObject(SG.pStatObj, rParms);
      sRT_EndEf(RLD);

      //gEnv->p3DEngine->SetSkyColor(cSkyBackup);

      nCurX++;
    }
	  {
		  //gEnv->p3DEngine->SetSunColor(Vec3(0,0,0));
      rParms.AmbientColor = gEnv->p3DEngine->GetSkyColor();
      rParms.AmbientColor.a = 1.f;

      sRT_StartEf(RLD);  
      sSetViewPort(nCurX, nCurY, nSpriteResInt);
      sRT_RenderObject(SG.pStatObj, rParms);
      sRT_EndEf(RLD);

		  //gEnv->p3DEngine->SetSunColor(cSunBackup);

      nCurX++;
	  }

    if ((nCurX+2)*nSpriteResInt > pTempTex->GetWidth())
    {
      nCurX = 0;
      nCurY++;
      if ((nCurY+1)*nSpriteResInt > pTempTex->GetHeight())
      {
        sFlushSprites(&SGI[nStart], nCurX, nCurY, pTempTex, nSpriteResInt, nSpriteResFinal);
        nStart = i+1;
        nCurY = 0;
      }
    }

		// check regeneration in case of streaming
		{
			if(*SG.ppTexture)
			{
				IMaterial* pMaterial = SG.pMaterial ? SG.pMaterial : SG.pStatObj->GetMaterial();
				IRenderMesh *pRenderMesh = SG.pStatObj->GetRenderMesh();
				bool needRegenerate = false;

				if (pRenderMesh)
				{
					float fRealDistance = fDrawDist*tan(fFOV/2.0f)*2.0f;
					float fMipFactor = fRealDistance * fRealDistance / (nSpriteResInt * nSpriteResInt) * fRadiusHors / fRadiusVert;

					*SG.pMipFactor = fMipFactor;

				  PodArray<CRenderChunk> *pChunks = &pRenderMesh->GetChunks();

					if (pChunks)
					{
						for (size_t j = 0; j < pChunks->size(); j++)
						{
							CRenderChunk &currentChunk = (*pChunks)[j];
							IMaterial *pCurrentSubMtl = pMaterial->GetSubMtl(currentChunk.m_nMatID);
							float fCurrentMipFactor = fMipFactor*currentChunk.m_texelAreaDensity;

							if (!AreTexturesStreamed(pCurrentSubMtl,fCurrentMipFactor))
							{
								pCurrentSubMtl->PrecacheTextures(fCurrentMipFactor,FPR_STARTLOADING,gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nObjectUpdateId);
								needRegenerate = true;
							}
						}
					}
				}

				if (needRegenerate)
				{
					((SDynTexture2*)(*SG.ppTexture))->m_nFlags |= IDynTexture::fNeedRegenerate;
					*SG.pTexturesAreStreamedIn = false;
				}
				else
				{
					((SDynTexture2*)(*SG.ppTexture))->m_nFlags &= ~IDynTexture::fNeedRegenerate;
					*SG.pTexturesAreStreamedIn = true;
				}
			}
		}
  }
  if (nCurX || nCurY)
    sFlushSprites(&SGI[nStart], nCurX, nCurY, pTempTex, nSpriteResInt, nSpriteResFinal);
  
  FX_PopRenderTarget(0);

  /*char name[256];
  sprintf(name, "%s.dds", pTempTex->GetName());
  pTempTex->SaveDDS(name, false);

  IDynTexture *pD = (*SGI[0].ppTexture);
  CTexture *pDst = (CTexture *)pD->GetTexture();
  sprintf(name, "%s.dds", pDst->GetName());
  pDst->SaveDDS(name, false);*/

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// End sprite gen
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  //SAFE_DELETE(pTempTex);

	EF_PopMatrix();
	m_RP.m_TI[nThreadID].m_matProj->Pop();

//  gEnv->p3DEngine->SetSkyColor(cSkyBackup);
  //gEnv->p3DEngine->SetSunColor(cSunBackup);
  CV_r_usezpass = nPrevZpass;

  //CV_r_texturesstreamingsync = nPrevStr;
  CV_r_shadersasynccompiling = nPrevAsync;

  RT_SetViewport(vX, vY, vWidth, vHeight);
  EF_PopFog();
  FX_Commit();

  m_RP.m_TI[nThreadID].m_PersFlags = saveFlags;
	m_RP.m_TI[nThreadID].m_PersFlags2 = saveFlags2;

	SetCamera(origCam);
#ifdef DO_RENDERLOG
	if (CV_r_log)
		Logv(SRendItem::m_RecurseLevel[nThreadID], "****************** CD3D9Renderer::MakeSprite - End ******************\n");
#endif
	m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_MAKESPRITE;
  CHWShader_D3D::mfSetGlobalParams();

  if (SRendItem::m_RecurseLevel[nThreadID] == 1)
    EF_ClearBuffers(FRT_CLEAR, NULL);

  SRendItem::m_RecurseLevel[nThreadID] = nRecursion;

  m_RP.m_pShader = sh;
  m_RP.m_pCurTechnique = pTech;
  m_RP.m_pShaderResources = pRes;
  m_RP.m_pCurObject = pObj;
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
  m_RP.m_pRootTechnique = pRootTech;
  m_RP.m_nShaderTechnique = nTech;
  m_RP.m_pRE = pRE;
}


_inline bool CompareSpriteItem(const SSpriteInfo& p1, const SSpriteInfo& p2)
{
	if(p1.m_pTerrainTexInfo != p2.m_pTerrainTexInfo)
		return p1.m_pTerrainTexInfo < p2.m_pTerrainTexInfo;
  return p1.m_pTex < p2.m_pTex;
}


void CD3D9Renderer::GenerateObjSprites (PodArray<struct SVegetationSpriteInfo> *pList)
{
	if (pList == 0)
		return;

  FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

  static TArray<SSpriteGenInfo> sSGI;

  I3DEngine *pEngine = gEnv->p3DEngine;

  const uint32 dwSunSkyRel = (uint32)(256.0f*pEngine->GetSunRel());			assert(dwSunSkyRel<=256); // 0..256
  const uint32 dwSkySunRel = 256-dwSunSkyRel;														assert(dwSkySunRel<=256); // 0..256

  const CCamera & rCamera = pEngine->GetCurrentCamera(); // 3dengine camera is used in order to make e_camera_freeze working correct
  const Vec3& vCamPos = rCamera.GetPosition();//m_RP.m_TI[m_RP.m_nProcessThreadID].m_rcam.Orig;
  const float rad2deg = 180.0f/PI;    

  int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID];

	int nPolygonMode = CV_r_wireframe;
  int nZPass = CV_r_usezpass;

	CV_r_wireframe = 0;
  CV_r_usezpass = 0;

  static uint32 s_dwOldMakeSpriteCalls=0;

//	static ICVar *pVarDebugMask = iConsole->GetCVar( "e_DebugMask" );
  int e_debug_mask = 0;//pVarDebugMask->GetIVal(); 

  static ICVar *pVarDR = iConsole->GetCVar("e_VegetationSpritesDistanceRatio");

  float fCustomDistRatio = pVarDR->GetFVal();
  float fTerrainLMThreshold = .1f;
  float fDirThreshold = fTerrainLMThreshold*pEngine->GetSunDir().GetLength();
  float fColorThreshold = fTerrainLMThreshold;

  int nTexHeightFinal = CV_r_VegetationSpritesTexRes;  
  int nSizeSprX = nTexHeightFinal;
#ifdef SPRITES_2_TEXTURES
  nSizeSprX <<= 1;
#endif

  Vec3 vSunDirWS = pEngine->GetSunDir();
  Vec3 vSunCol = pEngine->GetSunColor();
  Vec3 vSkyCol = pEngine->GetSkyColor();

  sSGI.SetUse(0);

  m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumSprites += snSPInfo;
  snSPInfo = 0;
  //static byte *sPtr;
  if (snSPInfoMax < pList->Count())
  {
    snSPInfoMax = pList->Count();
    if (sSPInfo)
    {
#if !defined(XENON) && !defined(PS3)
      delete [] sSPInfo;



#endif
    }
#if !defined(XENON) && !defined(PS3)
    sSPInfo = new SSpriteInfo[snSPInfoMax];



#endif
  }





  register int nS = 0;

  SVegetationSpriteInfo * const pStart = &pList->GetAt(0);
  register int i;
  for (i=0; i<pList->Count(); i++)
  { 
    SVegetationSpriteInfo & vi = pStart[i];






    Vec3 vDir = vi.sp.center - vCamPos;

    float fSqDist2d = vDir.x*vDir.x + vDir.y*vDir.y;

    float fSqDist3d = (fSqDist2d+vDir.z*vDir.z);

    if(fSqDist3d > vi.fMaxViewDistSq || !rCamera.IsSphereVisible_F(vi.sp))
      continue;










    SSpriteInfo * const pSP = &sSPInfo[nS];
    nS++;

    pSP->m_pTex = NULL;
    pSP->m_vPos = vi.sp.center;

    float fMul = vi.fScaleH*isqrt_fast_tpl(fSqDist2d);
    pSP->m_fDY = vDir.x*fMul;
    pSP->m_fDX = vDir.y*fMul;
    pSP->m_fScaleV = vi.fScaleV;

		pSP->m_pTerrainTexInfo = vi.pTerrainTexInfo;

    SVegetationSpriteLightInfo * pLightInfo = vi.pLightInfo;

    bool bLightInfoIsUpToDate = pLightInfo->IsEqual(vSunDirWS, vSunCol, vSkyCol, fDirThreshold, fColorThreshold);

    SDynTexture2*& pTex = (SDynTexture2*&)pLightInfo->m_pDynTexture;

		bool bStreamingStateIsUpToDate = !(bLightInfoIsUpToDate && pTex && (pTex->GetFlags() & IDynTexture::fNeedRegenerate) && vi.pStatInstGroup->nTexturesAreStreamedIn);

		if( !bLightInfoIsUpToDate || !bStreamingStateIsUpToDate || !pTex || !pTex->_IsValid() || (pTex->m_nHeight != nTexHeightFinal) || CV_r_VegetationSpritesGenAlways )
    {
      if(!vi.pStatInstGroup->pStatObj || !vi.pStatInstGroup->pStatObj->GetRenderMesh())
      { 
        snSPInfo--;
        continue;
      }

      if(CV_r_VegetationSpritesGenDebug)
      {
        gEnv->pLog->Log("Regenerating vegetation sprite: ");

        if( !pTex )
          gEnv->pLog->LogPlus(" !pTex");
        else if( pTex && !pTex->_IsValid() )
          gEnv->pLog->LogPlus(" !pTex->_IsValid()");
        else if( !bLightInfoIsUpToDate ) 
          gEnv->pLog->LogPlus(" !bLightInfoIsUpToDate");        
        else if( !bStreamingStateIsUpToDate )
          gEnv->pLog->LogPlus(" !bStreamingStateIsUpToDate");               
        else if( pTex && (pTex->m_nHeight != nTexHeightFinal) )
          gEnv->pLog->LogPlus(" pTex->m_nHeight != nTexHeightFinal");        
        else if( CV_r_VegetationSpritesGenAlways )
          gEnv->pLog->LogPlus(" CV_r_VegetationSpritesGenAlways");
        else 
          gEnv->pLog->LogPlus(" Unknown");
      }

      if (!pTex || (pTex->m_nHeight != nTexHeightFinal))
      {
        SAFE_RELEASE(pTex);

        char *szName;

#ifdef _DEBUG
        char Name[128];
        sprintf(Name, "$SpriteTex_(%1.f %1.f %1.f)_%d", pSP->m_vPos.x, pSP->m_vPos.y, pSP->m_vPos.z, vi.ucSlotId);
        szName = Name;
#else
        szName = "$SpriteTex";
#endif

        int nDownScaleX = 1;
        Vec3 objSize = vi.pStatInstGroup->pStatObj->GetAABB().GetSize();
        if (objSize.z>objSize.x*2 && objSize.z>objSize.y*2)
          nDownScaleX = 2;

        int nTexX = nTexHeightFinal/nDownScaleX;
#ifdef SPRITES_2_TEXTURES
        nTexX <<= 1;
#endif

        pTex = new SDynTexture2(nTexX, nTexHeightFinal, FT_DONTSYNCMULTIGPU |  FT_STATE_CLAMP | FT_NOMIPS, szName, eTP_Sprites);
        pTex->Update(nTexX, nTexHeightFinal);
      }

      if (!bLightInfoIsUpToDate || !bStreamingStateIsUpToDate)
      {
        if (pTex)
          pTex->ResetUpdateMask();
        pLightInfo->SetLightingData(pEngine->GetSunDir(), pEngine->GetSunColor(), pEngine->GetSkyColor());
      }

      if (pTex)
        pTex->SetUpdateMask();

      SSpriteGenInfo SGI;
      SGI.fAngle = FAR_TEX_ANGLE*vi.ucSlotId + 90.f;
      SGI.fGenDist = 18.f*max(0.5f, vi.pStatInstGroup->fSpriteDistRatio*fCustomDistRatio);
      SGI.fBrightnessMultiplier = pLightInfo->GetSpriteBrightnessMultiplier() / (1e-10f + vi.pStatInstGroup->fBrightness);
      SGI.nMaterialLayers = vi.pStatInstGroup->nMaterialLayers;
			SGI.pMipFactor = &pLightInfo->m_MipFactor;
			SGI.pTexturesAreStreamedIn = &vi.pStatInstGroup->nTexturesAreStreamedIn;
      SGI.ppTexture = &pTex;
      SGI.pMaterial = vi.pStatInstGroup->pMaterial;
      SGI.pStatObj = vi.pStatInstGroup->pStatObj;
      SGI.nSP = nS-1;
      sSGI.AddElem(SGI);
    }
    else
    {
      CTexture *pT = (CTexture *)pTex->GetTexture();
      pT->m_nAccessFrameID = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;
      const uint32 dwWidth = pT->GetWidth();
      const uint32 dwHeight = pT->GetHeight();
      pSP->m_pTex = pTex;
      uint32 nX, nY, nW, nH;
      pTex->GetSubImageRect(nX, nY, nW, nH);
#ifdef SPRITES_2_TEXTURES
      nW >>= 1;
#endif
      pSP->m_ucTexCoordMinX = (nX*128)/dwWidth;					assert((pSP->m_ucTexCoordMinX*dwWidth)/128==nX);
      pSP->m_ucTexCoordMinY = (nY*128)/dwHeight;				assert((pSP->m_ucTexCoordMinY*dwHeight)/128==nY);
      pSP->m_ucTexCoordMaxX = ((nX+nW)*128)/dwWidth;		assert((pSP->m_ucTexCoordMaxX*dwWidth)/128==nX+nW);
      pSP->m_ucTexCoordMaxY = ((nY+nH)*128)/dwHeight;		assert((pSP->m_ucTexCoordMaxY*dwHeight)/128==nY+nH);
    }







#define  bcolor0	0
#define  bcolor1	1
#define  bcolor2	2
#define  bcolor3	3


    pSP->m_Color.bcolor[bcolor0] = max(vi.ucAlphaTestRef, (uint8)(255.f*SATURATE(fSqDist3d/vi.fMaxViewDistSq*3.f-2.f)));
		pSP->m_Color.bcolor[bcolor1] = vi.ucSunDotTerrain;

#ifdef SPRITES_2_TEXTURES
    if (pTex)
    {
      assert((pTex)->GetWidth() == 64 || (pTex)->GetWidth() == 128);
      pSP->m_Color.bcolor[bcolor2] = (pTex)->GetWidth() == 64 ? 0x40 : 0xc0;
    }
#endif

    pSP->m_Color.bcolor[bcolor3] = (uint8)((((vi.fAmbientValue*255))*pLightInfo->m_BrightnessMultiplier)/256);
  }

  if (sSGI.Num())
    MakeSprites(sSGI);

  snSPInfo = nS;

  if(!CV_r_VegetationSpritesGenAlways)
    if(sSGI.Num()>100 && s_dwOldMakeSpriteCalls>100)
      gEnv->pLog->LogWarning("MakeSprite() was called %d times in this frame and %d last frame (quick player view change/fast moving sun/too many sprites/low dynamic texture memory)",sSGI.Num(), s_dwOldMakeSpriteCalls);

  if((e_debug_mask & 0x20)!=0)
    gEnv->pLog->Log("MakeSprite() was called %d times in this frame (%d objects)",sSGI.Num(),pList->Count());

  s_dwOldMakeSpriteCalls = sSGI.Num();

  //static ICVar *pVarTerrainAO = iConsole->GetCVar( "e_TerrainAo" );
  //static ICVar *pVarTerrBlend = iConsole->GetCVar( "e_VegetationUseTerrainColor" );
  //if(SDynTexture2::GetPoolTexNum(eTP_Sprites)>1 || pVarTerrainAO->GetIVal() || pVarTerrBlend->GetIVal())
  //if((e_debug_mask & 0x10)==0)
  //{
  //  PROFILE_FRAME(Generate_ObjSpritesList_Sort);
  //  std::sort(sSPInfo, sSPInfo+nS, CompareSpriteItem); 
  //}

  CV_r_wireframe = nPolygonMode;
  CV_r_usezpass = nZPass;
}


//static CObjManager *sObjMan;

/*
static _inline int Compare(CVegetation *& p1, CVegetation *& p2)
{
  CStatObj * pStatObj1 = sObjMan->m_lstStaticTypes[p1->m_nObjectTypeID].GetStatObj();
  CStatObj * pStatObj2 = sObjMan->m_lstStaticTypes[p2->m_nObjectTypeID].GetStatObj();
  if((UINT_PTR)p1 > (UINT_PTR)p2)
    return 1;
  else
  if((UINT_PTR)p1 < (UINT_PTR)p2)
    return -1;
  
  return 0;
}
*/
void CD3D9Renderer::ObjSpritesFlush (SVF_P3F_C4B_T2F *pVerts, uint16 *pInds, int nSprites, void *&pCurVB, SShaderTechnique *pTech)
{
  //int nPointState = CTexture::GetTexState(STexState(FILTER_POINT, true));

  uint32 i;
  int nOffs, nIOffs;
  int nVerts = nSprites * 4;
  SVF_P3F_C4B_T2F *vDst = (SVF_P3F_C4B_T2F *)GetVBPtr(nVerts, nOffs, POOL_P3F_COL4UB_TEX2F);
  if (!vDst)
    return;



  memcpy(vDst, pVerts, sizeof(SVF_P3F_C4B_T2F)*nVerts);

  UnlockVB();

  int nInds = nSprites * 6;
  uint16 *iDst = GetIBPtr(nInds, nIOffs);
  if (!iDst)
    return;



  memcpy(iDst, pInds, sizeof(uint16)*nInds);

  UnlockIB();

  if (pCurVB != m_pVB[POOL_P3F_COL4UB_TEX2F])
  {
    pCurVB = m_pVB[POOL_P3F_COL4UB_TEX2F];
    FX_SetVStream(0, m_pVB[POOL_P3F_COL4UB_TEX2F], 0, sizeof(SVF_P3F_C4B_T2F));
  }
  FX_SetIStream(m_pIB);

  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_MULTILIGHTS;

  bool bSunShadowExist = false;
  if(m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1].Num())
    if( m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][0].m_Flags & DLF_SUN )
      if( m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][0].m_Flags & DLF_CASTSHADOW_MAPS )
          bSunShadowExist = true;

  //set shadow mask texture
	if (bSunShadowExist)
  {
    CTexture *pTexMask = CTexture::s_ptexBackBuffer;
    pTexMask->Apply(1, m_nPointState);
  }
	else 
	{
		CTexture::s_ptexBlack->Apply( 1, m_nPointState);
	}

  if( CTexture::s_ptexAOTarget && CTexture::s_ptexAOTarget->GetDevTexture() &&  m_RP.m_nAOMaskUpdateLastFrameId == m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID )
		CTexture::s_ptexAOTarget->Apply(2, m_nPointState);
	else 
		CTexture::s_ptexWhite->Apply(2, m_nPointState);

  SShaderPass *pPass = &pTech->m_Passes[0];
  for (i=0; i<pTech->m_Passes.Num(); i++, pPass++)
  {
    m_RP.m_pCurPass = pPass;
    if (!pPass->m_VShader || !pPass->m_PShader)
      continue;
    CHWShader_D3D *curVS = (CHWShader_D3D *)pPass->m_VShader;
    CHWShader_D3D *curPS = (CHWShader_D3D *)pPass->m_PShader;

    curPS->mfSet(0);
    curPS->mfSetParametersPI(NULL, NULL);

    curVS->mfSet(0);
    curVS->mfSetParametersPI(NULL, NULL);

    FX_Commit();

    int nPolys = nVerts/2;
#if defined (DIRECT3D9) || defined (OPENGL)
    //m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, nOffs, nPolys);
    m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, nOffs, 0, nVerts, nIOffs, nPolys);
#elif defined (DIRECT3D10)
    if (CHWShader_D3D::m_pCurInstVS && !CHWShader_D3D::m_pCurInstVS->m_bFallback && CHWShader_D3D::m_pCurInstPS && !CHWShader_D3D::m_pCurInstPS->m_bFallback)
    {
      SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      m_pd3dDeviceContext->DrawIndexed(nInds, nIOffs, nOffs);
    }
#endif
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumSpriteDIPS++;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumSpritePolys += nPolys;
  }
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_MULTILIGHTS;
}
/*
static _inline int Compare(SSpriteInfo& p1, SSpriteInfo& p2)
{
  if(p1.m_TexID > p2.m_TexID)
    return 1;
  else
  if(p1.m_TexID < p2.m_TexID)
    return -1;
  if (p1.m_nLightMask < p2.m_nLightMask)
    return -1;
  if (p1.m_nLightMask > p2.m_nLightMask)
    return -1;

  return 0;
}
*/

extern CTexture *gTexture2;

void CD3D9Renderer::DrawObjSprites (SSpriteInfo *pSPInfo, const int nSPI, bool bZ, bool bShadows)
{
  uint32 i;

  CTexture *pPrevTex = NULL;
	SSectorTextureSet * pPrevTerrainTexInfo = NULL;

  int nState;
  int nAlphaRef = 0;
	uint64 nPrevFlagsShader_RT = m_RP.m_FlagsShader_RT;

  float fTimeP = iTimer->GetAsyncCurTime();

/*  if(CV_r_VegetationSpritesAlphaBlend)
  {
    nState = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_ALPHATEST_GREATER;
    nAlphaRef = 0;
  }
  else*/
  {
    if(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_IMPOSTERGEN)
      nState = GS_DEPTHWRITE;
    else
    {
      nState = GS_DEPTHWRITE;
      //nAlphaRef = 40;
    }
  }
  FX_ZState(nState);
  
  if (bShadows)
  {
    int nColorMask = ((~(1<<0))<<GS_COLMASK_SHIFT) & GS_COLMASK_MASK ;
    nState |= nColorMask | GS_BLSRC_ONE | GS_BLDST_ONE;
  }
#if defined(XENON) || defined( PS3 )
  else
  if( bZ )
    nState |= GS_COLMASK_NONE;
	// for consoles we use alpha test during z prepass
	if( bZ )
	{
		nState |= GS_ALPHATEST_GREATER;
		nAlphaRef = 1;
	}
#endif

  EF_SetState(nState, nAlphaRef);

  D3DSetCull(eCULL_None);
  
	if( CRenderer::CV_r_HDRRendering )    
		m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_HDR_MODE];

	if( CRenderer::CV_r_deferredshading )
		m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEFERRED_SHADING];

  m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];
#ifdef SPRITES_2_TEXTURES
  m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
#endif
  SRenderShaderResources rs;
  SRenderShaderResources *pResSave = m_RP.m_pShaderResources;
  m_RP.m_pShaderResources = &rs;
  rs.m_AlphaRef = (float)nAlphaRef / 255.0f;

  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_USESTREAM_MASK;

  m_RP.m_FlagsStreams_Decl = 0;
  m_RP.m_FlagsStreams_Stream = 0;

  EVertexFormat nf = eVF_P3F_C4B_T2F;
  m_RP.m_CurVFormat = nf;
  m_RP.m_FlagsShader_MD = 0;
  m_RP.m_FlagsShader_MDV = 0;
  m_RP.m_FlagsShader_LT = 0;

  if (!CV_r_usezpass) 
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NOZPASS];

  m_cEF.m_ShaderTreeSprites->FXBegin(&m_RP.m_nNumRendPasses, bShadows ? FEF_DONTSETSTATES : FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
  m_cEF.m_ShaderTreeSprites->FXBeginPass(0);

  m_RP.m_CurVFormat = nf;
  HRESULT hr = FX_SetVertexDeclaration(m_RP.m_FlagsStreams_Decl&7, m_RP.m_CurVFormat);
  if (hr != S_OK)
    return;

  // set atlas size of custom texture filter
  static CCryName TexAtlasSizeName("TexAtlasSize");
  const Vec4 cATVal((float)CV_r_texatlassize, (float)CV_r_texatlassize, 1.f/(float)CV_r_texatlassize, 1.f/(float)CV_r_texatlassize);
  m_cEF.m_ShaderTreeSprites->FXSetPSFloat(TexAtlasSizeName, &cATVal, 1);
  m_cEF.m_ShaderTreeSprites->FXSetVSFloat(TexAtlasSizeName, &cATVal, 1);

  void *pCurVB = NULL;

  /*static D3DSurface *pSurf = NULL;
  if (!pSurf)
  {
    hr = m_pd3dDevice->CreateRenderTarget(64, 64, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pSurf, NULL);
  }

  if (gDT)
  {
    int nFrame = m_nFrameSwapID;
    if ((1<<(nFrame&1)) & gnFrame)
    {
      SDynTexture2 *pDT = gDT;
      pDT->Apply(0);
      FX_PushRenderTarget(0, pSurf, &m_DepthBufferOrig);
      EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
      DrawFullScreenQuad(m_cEF.m_ShaderTreeSprites, "General_Debug", m_cEF.m_RTRect.x, m_cEF.m_RTRect.y, m_cEF.m_RTRect.x+m_cEF.m_RTRect.z, m_cEF.m_RTRect.y+m_cEF.m_RTRect.w);
      FX_PopRenderTarget(0);
      D3DLOCKED_RECT lr;
      pSurf->LockRect(&lr, NULL, 0);
      UCol *pData = (UCol *)lr.pBits;
      //for (int i=0; i<64*64; i++)
      //{
      UCol d = pData[32*64+32];
      if (d.bcolor[0] == 0xff)
      {
        //assert(0);
        int nnn = 0;
      }
      //}
      pSurf->UnlockRect();
      m_cEF.m_ShaderTreeSprites->FXSetTechnique("General");
      m_cEF.m_ShaderTreeSprites->FXBegin(&m_RP.m_nNumRendPasses, bShadows ? FEF_DONTSETSTATES : FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
      m_cEF.m_ShaderTreeSprites->FXBeginPass(0);
      m_RP.m_CurVFormat = nf;

      hr = FX_SetVertexDeclaration(m_RP.m_FlagsStreams_Decl&7, m_RP.m_CurVFormat);
      if (hr != S_OK)
        return;
      FX_SetIStream(m_pIB);
      pCurVB = NULL;

      // set atlas size of custom texture filter
      static CCryName TexAtlasSizeName("TexAtlasSize");
      const Vec4 cFloatVal(CV_r_texatlassize, CV_r_texatlassize, 1.f/CV_r_texatlassize, 1.f/CV_r_texatlassize);
      m_cEF.m_ShaderTreeSprites->FXSetPSFloat(TexAtlasSizeName, &cFloatVal, 1);
      EF_SetState(nState, nAlphaRef);

    }
  }*/

  static SVF_P3F_C4B_T2F *spVerts = NULL;
  static uint16 *spInds = NULL;
  const float fFracMem = ((float)(sizeof(short))*6.0f) / ((float)(sizeof(SVF_P3F_C4B_T2F))*4.0f);
  const int nMemV = (int)(128.0f*1024.0f*(1-fFracMem));
  const int nMemI = (int)(128.0f*1024.0f*fFracMem);
  assert(nMemI+nMemV <= 128*1024);
  const int nMaxVerts = nMemV/sizeof(SVF_P3F_C4B_T2F);
  const int nMaxSprites = nMaxVerts / 4;
  const int nMaxInds = nMemI / sizeof(short);
  if (!spVerts)
  {
    byte *pMem = (byte *)CryModuleMemalign(128*1028, 128);
    spVerts = (SVF_P3F_C4B_T2F *)pMem;
    spInds = (uint16 *)(pMem + nMemV + 256);
  }











  int nTexState;
#ifdef SPRITES_2_TEXTURES
  nTexState = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
#else
  nTexState = CTexture::GetTexState(STexState(FILTER_POINT, true));
#endif

  uint32 nPrevLMask = (uint32)-1;
  int nPasses = 0;
	bool bMultiGPU = CRenderer::IsMultiGPUModeActive();
  //static TArray<SVF_P3F_C4B_T2F> sVerts;
  //sVerts.SetUse(0);
  //static TArray<uint16> sInds;
  //sInds.SetUse(0);

  //int nS = nSPI;
  //int nMaxVerts = 256*1024/sizeof(SVF_P3F_C4B_T2F);
  //if (nS * 4 >= nMaxVerts)
  //  nS = nMaxVerts / 4 - 1;
  //sVerts.AddIndex(nS*4+4);
  //sInds.AddIndex(nS*6+6);
  //BOOL result = XLockL2( XLOCKL2_INDEX_TITLE, &sVerts[0], sVerts.Num()*sizeof(SVF_P3F_C4B_T2F), XLOCKL2_LOCK_SIZE_2_WAYS, 0);
  int nO = 0;  // Sprite ID
  int nLV = 256;
  int nLI = 128;
  for (i=0; (int)i<nSPI; i++)
  {
    const SSpriteInfo * __restrict pSP = &pSPInfo[i];

    SDynTexture2 *pDT = (SDynTexture2 *)pSP->m_pTex;
    if (!pDT)
      continue;
    CTexture *pTex = pDT->m_pTexture;
#ifdef _DEBUG
    // Make sure we updated the texture on this frame
		if(bMultiGPU)
    {
      assert(pDT->IsValid());
    }
#endif
    if (!pTex)
      continue;





    /*{
      pDT->Apply(0);
      if (!strcmp(pDT->m_sSource, "$SpriteTex_(1049 2070 249)_1_8"))
      {
        if (!gDT)
        {
          gDT = pDT;

          gnFrame = pDT->m_nUpdateMask;
        }
        int nnn = 0;
      }
      FX_PushRenderTarget(0, pSurf, &m_DepthBufferOrig);
      EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
      DrawFullScreenQuad(m_cEF.m_ShaderTreeSprites, "General_Debug", m_cEF.m_RTRect.x, m_cEF.m_RTRect.y, m_cEF.m_RTRect.x+m_cEF.m_RTRect.z, m_cEF.m_RTRect.y+m_cEF.m_RTRect.w);
      FX_PopRenderTarget(0);
      D3DLOCKED_RECT lr;
      pSurf->LockRect(&lr, NULL, 0);
      UCol *pData = (UCol *)lr.pBits;
      //for (int i=0; i<64*64; i++)
      //{
        UCol d = pData[32*64+32];
        if (d.bcolor[0] == 0xff)
        {
          //assert(0);
          int nnn = 0;
        }
      //}
      pSurf->UnlockRect();
      m_cEF.m_ShaderTreeSprites->FXSetTechnique("General");
      m_cEF.m_ShaderTreeSprites->FXBegin(&m_RP.m_nNumRendPasses, bShadows ? FEF_DONTSETSTATES : FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
      m_cEF.m_ShaderTreeSprites->FXBeginPass(0);
      m_RP.m_CurVFormat = nf;

      hr = FX_SetVertexDeclaration(m_RP.m_FlagsStreams_Decl&7, m_RP.m_CurVFormat);
      if (hr != S_OK)
        return;
      FX_SetIStream(m_pIB);
      pCurVB = NULL;

      // set atlas size of custom texture filter
      static CCryName TexAtlasSizeName("TexAtlasSize");
      const Vec4 cFloatVal(CV_r_texatlassize, CV_r_texatlassize, 1.f/CV_r_texatlassize, 1.f/CV_r_texatlassize);
      m_cEF.m_ShaderTreeSprites->FXSetPSFloat(TexAtlasSizeName, &cFloatVal, 1);
      EF_SetState(nState, nAlphaRef);
      //pTex->Apply(0, nPointState);
    }*/
		if(	pPrevTex != pTex || pPrevTerrainTexInfo != pSP->m_pTerrainTexInfo )
    {
      if (nO)
      {
        ObjSpritesFlush(spVerts, spInds, nO, pCurVB, m_RP.m_pCurTechnique);
        nO = 0;
        nLI = 128;
        nLV = 256;
      }
      if(!bShadows)
			{
        gTexture2 = pTex;
				pTex->Apply(0, nTexState);

				if(m_RP.m_nPassGroupID == EFSLIST_GENERAL && pSP->m_pTerrainTexInfo)
				{
          SSectorTextureSet * pTexInfo = pSP->m_pTerrainTexInfo;

					// set new RT flags
					m_cEF.m_ShaderTreeSprites->FXEndPass();
					m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_BLEND_WITH_TERRAIN_COLOR];

					m_cEF.m_ShaderTreeSprites->FXBeginPass(0);

					CTexture * pTerrTex = CTexture::GetByID(pTexInfo->nTex0);
          
          STexState pTerrainTexState = STexState(FILTER_LINEAR, true);
          pTerrainTexState.m_bSRGBLookup = gRenDev->IsLinearSpaceShadingEnabled();
          int nTerrainTexState = CTexture::GetTexState(pTerrainTexState);
					pTerrTex->Apply(3, nTerrainTexState);

					static CCryName SpritesOutdoorAOVertInfoName("SpritesOutdoorAOVertInfo");
					const Vec4 cSPVal(pTexInfo->fTexOffsetX, pTexInfo->fTexOffsetY, pTexInfo->fTexScale, pTexInfo->fTerrainMinZ);
					m_cEF.m_ShaderTreeSprites->FXSetVSFloat(SpritesOutdoorAOVertInfoName, &cSPVal, 1);
				}
        else
          m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_BLEND_WITH_TERRAIN_COLOR];


        { // set RT_AMBIENT if no light sources present, used by blend with terrain color feature
          bool bSunExist = false;
          if(m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1].Num())
            if( m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][0].m_Flags & DLF_SUN )
              bSunExist = true;

          if(!bSunExist)
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_AMBIENT];
          else
            m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_AMBIENT];
        }
			}
			pPrevTex = pTex;
			pPrevTerrainTexInfo = pSP->m_pTerrainTexInfo;

      m_cEF.m_ShaderTreeSprites->FXSetPSFloat(TexAtlasSizeName, &cATVal, 1);
      m_cEF.m_ShaderTreeSprites->FXSetVSFloat(TexAtlasSizeName, &cATVal, 1);
    }

    register int nOffs = nO * 4;
    if (nOffs+4 >= nMaxVerts)
    {
      ObjSpritesFlush(spVerts, spInds, nO, pCurVB, m_RP.m_pCurTechnique);
      nOffs = 0;
      nO = 0;
      nLI = 128;
      nLV = 256;
    }

    float fX0 = pSP->m_vPos.x+pSP->m_fDX;
    float fX1 = pSP->m_vPos.x-pSP->m_fDX;
    float fY0 = pSP->m_vPos.y+pSP->m_fDY;
    float fY1 = pSP->m_vPos.y-pSP->m_fDY;

    float vUpz = pSP->m_fScaleV;
    float pSPm_vPoszvUpz = pSP->m_vPos.z-vUpz;
    float pSPm_vPoszvUpzm = pSP->m_vPos.z+vUpz;

    uint32 dCol = pSP->m_Color.dcolor;

    float fST00 = sfTableTCFloat[pSP->m_ucTexCoordMaxX];
    float fST01 = sfTableTCFloat[pSP->m_ucTexCoordMaxY];
    float fST10 = sfTableTCFloat[pSP->m_ucTexCoordMinX];
    float fST11 = sfTableTCFloat[pSP->m_ucTexCoordMinY];

    SVF_P3F_C4B_T2F * __restrict pQuad = &spVerts[nOffs];









    pQuad[0].xyz(fX0, fY1, pSPm_vPoszvUpz);
    pQuad[0].color.dcolor = dCol;
    pQuad[0].st.x = fST00; pQuad[0].st.y = fST01;

    pQuad[1].xyz(fX1, fY0, pSPm_vPoszvUpz);
    pQuad[1].color.dcolor = dCol;
    pQuad[1].st.x = fST10; pQuad[1].st.y = fST01;

    // 2,3 verts
    pQuad[2].xyz(fX0, fY1, pSPm_vPoszvUpzm);
    pQuad[2].color.dcolor = dCol;
    pQuad[2].st.x = fST00; pQuad[2].st.y = fST11;

    pQuad[3].xyz(fX1, fY0, pSPm_vPoszvUpzm);
    pQuad[3].color.dcolor = dCol;
    pQuad[3].st.x = fST10; pQuad[3].st.y = fST11;

    int nIOffs = nO * 6;
    uint16 * __restrict pInds = &spInds[nIOffs];







    pInds[0] = nOffs;
    pInds[1] = nOffs+1;
    pInds[3] = nOffs+1;
    pInds[2] = nOffs+2;
    pInds[4] = nOffs+2;
    pInds[5] = nOffs+3;

    nO++;
  }

  if (nO)
    ObjSpritesFlush(spVerts, spInds, nO, pCurVB, m_RP.m_pCurTechnique);
  else
    FX_Commit();

  //XUnlockL2( XLOCKL2_INDEX_TITLE );

  m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsSprites = iTimer->GetAsyncCurTime() - fTimeP;

  m_RP.m_FlagsShader_RT = nPrevFlagsShader_RT;
  m_RP.m_pShaderResources = pResSave;
}

void CD3D9Renderer::DrawObjSprites (PodArray<SVegetationSpriteInfo> *pList)
{
  assert(pList);
  if (!pList)
    return;

  if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] != 1)
    return;

  sInitTCTable();




  PROFILE_LABEL_PUSH( "SPRITES" );
  if (m_RP.m_nBatchFilter & FB_Z)
  {
    if (CV_r_usezpass && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_IMPOSTERGEN))
    {
      static CCryName TechName("Sprites_Z");
      m_cEF.m_ShaderTreeSprites->FXSetTechnique(TechName);
			{
				PROFILE_FRAME(Draw_ObjSprites_Z);
	      DrawObjSprites(sSPInfo, snSPInfo, true, false);
			}
    }
  }
  else
  {
    //DrawSpritesShadows();
    static CCryName TechName("General");
    m_cEF.m_ShaderTreeSprites->FXSetTechnique(TechName);
		{
			PROFILE_FRAME(Draw_ObjSprites);
	    DrawObjSprites(sSPInfo, snSPInfo, false, false);
		}
  }
  PROFILE_LABEL_POP( "SPRITES" );




}

uint32 CD3D9Renderer::RenderOccludersIntoBuffer(const CCamera & viewCam, 
																							 int nTexSize, 
																							 PodArray<IRenderNode*> & lstOccluders,
																							 float * pDst)
{
	PROFILE_FRAME(RenderOccludersIntoBuffer);

	int nOldNumDrawCalls = m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP];

	if (CV_r_flush == 3)
		FlushHardware(true);

	SetCamera(viewCam);

	static SDynTexture * pDynTexture = new SDynTexture(nTexSize, nTexSize, eTF_R32F, eTT_2D, 0, "RenderOccludersIntoBuffer");
	if(pDynTexture->GetWidth() != nTexSize)
	{
		pDynTexture->Release();
		pDynTexture = new SDynTexture(nTexSize, nTexSize, eTF_R32F, eTT_2D, 0, "RenderOccludersIntoBuffer");
	}

	pDynTexture->SetRT(0, true, FX_GetDepthSurface(nTexSize, nTexSize, false));

	m_cEF.m_TempVecs[0].x = viewCam.GetNearPlane();
	m_cEF.m_TempVecs[0].y = viewCam.GetFarPlane();
	m_cEF.m_TempVecs[0].z = 0;
	m_cEF.m_TempVecs[0].w = 1;

	// render object
	ColorF white(1,1,1,1);
	EF_ClearBuffers(FRT_CLEAR, &white);

	Vec4 vZRange;
	vZRange.x = -viewCam.GetNearPlane() / (viewCam.GetFarPlane() - viewCam.GetNearPlane());
	vZRange.y = 1.0f / (viewCam.GetFarPlane() - viewCam.GetNearPlane());
	vZRange.z = 1;
	vZRange.w = 0;
	FX_Commit();

	EF_StartEf();  

	int old_wireframe_mode = m_wireframe_mode;
	SetWireframeMode(R_SOLID_MODE);
	int oldCV_r_showlines = CV_r_showlines;
	CV_r_showlines = 0;

	for(int i=0; i<lstOccluders.Count(); i++)
	{
		SRendParams rParms;
//		rParms.nTechniqueID = TTYPE_SHADOWGEN; 
		rParms.dwFObjFlags |= FOB_TRANS_MASK; // | FOB_RENDER_INTO_SHADOWMAP; 
		lstOccluders[i]->Render(rParms);
	}

	EF_EndEf3D(0, -1);

	pDynTexture->RestoreRT(0,true);

	FX_Commit();

#if defined (DIRECT3D9) || defined (OPENGL)
	// read target texture into system memory
	D3DTexture *pID3DTextureDST = NULL;
	D3DSurface *pSurfDST, *pSurfSRC;
	HRESULT h = S_OK;
	h = D3DXCreateTexture(m_pd3dDevice, nTexSize, nTexSize, 1, 0, D3DFMT_R32F, D3DPOOL_SYSTEMMEM, &pID3DTextureDST);
	assert(SUCCEEDED(h));
	h = pID3DTextureDST->GetSurfaceLevel(0, &pSurfDST);
	pSurfSRC = pDynTexture->m_pTexture->GetSurface(-1, 0);
	m_pd3dDevice->GetRenderTargetData(pSurfSRC, pSurfDST);
	SAFE_RELEASE(pSurfSRC);
	D3DLOCKED_RECT rc;
	pSurfDST->LockRect(&rc, NULL, D3DLOCK_READONLY);
	assert(nTexSize == rc.Pitch/4);
	float * pSrc = (float*)rc.pBits;

	memset(pDst, 0, nTexSize*nTexSize*sizeof(float));

	// convert into coverage buffer format
	float fFrustumRange = viewCam.GetFarPlane()-viewCam.GetNearPlane();
	for(int x=0; x<nTexSize; x++)
	{
		for(int y=0; y<nTexSize; y++)
		{
			float fDist = pSrc[x*nTexSize + y] * fFrustumRange;
			if(fDist<0.001f)
				fDist = 0.001f;
			else if(fDist>fFrustumRange)
				fDist = fFrustumRange;

			fDist = 1.f/fDist;

			if(pDst[y + (nTexSize-1-x)*nTexSize] < fDist)
				pDst[y + (nTexSize-1-x)*nTexSize] = fDist;

			if(y>0 && fDist>pDst[(y-1) + (nTexSize-1-x)*nTexSize])
				pDst[(y-1) + (nTexSize-1-x)*nTexSize] = fDist;

			//if(x<nTexSize-1 && fDist>pDst[(y) + (nTexSize-1-x+1)*nTexSize])
			if(x>0 && fDist>pDst[(y) + (nTexSize-1-x+1)*nTexSize])
				pDst[(y) + (nTexSize-1-x+1)*nTexSize] = fDist;
		}
	}

	pSurfDST->UnlockRect();
	SAFE_RELEASE(pSurfDST);
	SAFE_RELEASE(pID3DTextureDST);
#endif

	SetWireframeMode(old_wireframe_mode);
	CV_r_showlines = oldCV_r_showlines;

	m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP] = nOldNumDrawCalls;

	return 1;
}
