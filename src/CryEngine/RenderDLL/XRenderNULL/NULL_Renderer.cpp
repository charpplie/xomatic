//////////////////////////////////////////////////////////////////////
//
//  Crytek CryENGINE Source code
//  
//  File:NULL_Renderer.cpp
//  Description: Implementation of the NULL renderer API
//
//  History:
//  -Jan 31,2001:Originally created by Marco Corbetta
//	-: taken over by Andrey Khonich
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "NULL_Renderer.h"
#include <IColorGradingController.h>

#include <IEngineModule.h>
#include <CryExtension/Impl/ClassWeaver.h>
// init memory pool usage

// Included only once per DLL module.
#include <platform_impl.h>

CCryName CTexture::s_sClassName = CCryName("CTexture");

CCryName CHWShader::m_sClassNameVS = CCryName("CHWShader_VS");
CCryName CHWShader::m_sClassNamePS = CCryName("CHWShader_PS");

CCryName CShader::m_sClassName = CCryName("CShader");

CNULLRenderer *gcpNULL = NULL;

#ifdef WIN32
IDirectBee *CRenderer::m_pDirectBee=0;		// connection to D3D9 wrapper DLL, 0 if not established
#endif

//////////////////////////////////////////////////////////////////////

class CNullColorGradingController : public IColorGradingController
{
public:
	virtual int LoadColorChart(const char* pChartFilePath) const { return 0; }
	virtual int LoadDefaultColorChart() const { return 0; }
	virtual void UnloadColorChart(int texID) const {}
	virtual void SetLayers(const SColorChartLayer* pLayers, uint32 numLayers) {}
};

//////////////////////////////////////////////////////////////////////
CNULLRenderer::CNULLRenderer()
{
  gcpNULL = this;
	m_pNULLRenderAuxGeom = CNULLRenderAuxGeom::Create(*this);
	m_pNULLColorGradingController = new CNullColorGradingController();
}


#include <stdio.h>
//////////////////////////////////////////////////////////////////////
CNULLRenderer::~CNULLRenderer()
{ 
  ShutDown(); 
	delete m_pNULLRenderAuxGeom;
	delete m_pNULLColorGradingController;
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::EnableTMU(bool enable)
{ 
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::CheckError(const char *comment)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::BeginFrame()
{
	m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameID++;
	m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameUpdateID++;
	m_RP.m_TI[m_RP.m_nFillThreadID].m_RealTime = iTimer->GetCurrTime();

	m_pNULLRenderAuxGeom->BeginFrame();
}

//////////////////////////////////////////////////////////////////////
bool CNULLRenderer::ChangeDisplay(unsigned int width,unsigned int height,unsigned int bpp)
{
  return false;
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::ChangeViewport(unsigned int x,unsigned int y,unsigned int width,unsigned int height)
{
}

void CNULLRenderer::RenderDebug()
{
}

void CNULLRenderer::EndFrame()
{
	//m_pNULLRenderAuxGeom->Flush(true);
	m_pNULLRenderAuxGeom->EndFrame();
}

void CNULLRenderer::GetMemoryUsage(ICrySizer* Sizer)
{
}

WIN_HWND CNULLRenderer::GetHWND()
{
#if defined(WIN32)
  return GetDesktopWindow();
#else
	return NULL;
#endif
}

void TexBlurAnisotropicVertical(CTexture *pTex, int nAmount, float fScale, float fDistribution, bool bAlphaOnly)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//IMAGES DRAWING
////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::Draw2dImage(float xpos,float ypos,float w,float h,int texture_id,float s0,float t0,float s1,float t1,float angle,float r,float g,float b,float a, float z)
{ 
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::DrawImage(float xpos,float ypos,float w,float h,int texture_id,float s0,float t0,float s1,float t1,float r,float g,float b,float a)
{ 
}

void CNULLRenderer::DrawImageWithUV(float xpos,float ypos,float z,float w,float h,int texture_id,float s[4],float t[4],float r,float g,float b,float a)
{
}

///////////////////////////////////////////
void CNULLRenderer::DrawBuffer(CVertexBuffer *pVBuf, CIndexBuffer *pIBuf, int nNumIndices, int nOffsIndex, int nPrmode, int nVertStart, int nVertStop)
{
}

void CNULLRenderer::DrawPrimitives(CVertexBuffer *src, int vert_num, int prim_type)
{
}

void CRenderMesh2::DrawImmediately()
{
}

///////////////////////////////////////////
void CNULLRenderer::SetCullMode(int mode)
{
}

///////////////////////////////////////////
void CNULLRenderer::SetFog(float density,float fogstart,float fogend,const float *color,int fogmode)
{
}

///////////////////////////////////////////
bool CNULLRenderer::EnableFog(bool enable)
{
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MISC EXTENSIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::EnableVSync(bool enable)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::SelectTMU(int tnum)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MATRIX FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::PushMatrix()
{
}

///////////////////////////////////////////
void CNULLRenderer::RotateMatrix(float a,float x,float y,float z)
{
}

void CNULLRenderer::RotateMatrix(const Vec3 & angles)
{
}

///////////////////////////////////////////
void CNULLRenderer::TranslateMatrix(float x,float y,float z)
{
}

void CNULLRenderer::MultMatrix(const float * mat)
{
}

void CNULLRenderer::TranslateMatrix(const Vec3 &pos)
{
}

///////////////////////////////////////////
void CNULLRenderer::ScaleMatrix(float x,float y,float z)
{
}

///////////////////////////////////////////
void CNULLRenderer::PopMatrix()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void CNULLRenderer::LoadMatrix(const Matrix34 *src)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MISC
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::Draw3dBBox(const Vec3 &mins,const Vec3 &maxs, int nPrimType)
{
}


///////////////////////////////////////////
int CNULLRenderer::SetWireframeMode(int mode)
{
  return 0;
}


///////////////////////////////////////////
void CNULLRenderer::SetCamera(const CCamera &cam)
{
	int nThreadID = m_pRT->GetThreadList();
	m_RP.m_TI[nThreadID].m_cam = cam;
}

void CNULLRenderer::GetViewport(int *x, int *y, int *width, int *height)
{
  *x = 0;
  *y = 0;
  *width = m_width;
  *height = m_height;
}

void CNULLRenderer::SetViewport(int x, int y, int width, int height)
{
}

void CNULLRenderer::SetScissor(int x, int y, int width, int height)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::GetModelViewMatrix(float * mat)
{
  memcpy(mat, &m_IdentityMatrix, sizeof(m_IdentityMatrix));
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::GetProjectionMatrix(float *mat)
{
  memcpy(mat, &m_IdentityMatrix, sizeof(m_IdentityMatrix));
}

//////////////////////////////////////////////////////////////////////
Vec3 CNULLRenderer::GetUnProject(const Vec3 &WindowCoords,const CCamera &cam)
{
  return (Vec3(1,1,1));
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::DrawQuad(const Vec3 &right, const Vec3 &up, const Vec3 &origin,int nFlipmode/*=0*/)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::ProjectToScreen( float ptx, float pty, float ptz, float *sx, float *sy, float *sz )
{
}

int CNULLRenderer::UnProject(float sx, float sy, float sz, 
              float *px, float *py, float *pz,
              const float modelMatrix[16], 
              const float projMatrix[16], 
              const int    viewport[4])
{
  return 0;
}

//////////////////////////////////////////////////////////////////////
int CNULLRenderer::UnProjectFromScreen( float  sx, float  sy, float  sz, 
                                      float *px, float *py, float *pz)
{
  return 0;
}

//////////////////////////////////////////////////////////////////////
bool CNULLRenderer::ScreenShot(const char *filename, int width)
{
  return true;
}

int CNULLRenderer::ScreenToTexture(int nTexID)
{
  return 0;
}

void CNULLRenderer::ResetToDefault()
{
}

///////////////////////////////////////////
void CNULLRenderer::SetMaterialColor(float r, float g, float b, float a)
{
}

char * CNULLRenderer::GetStatusText(ERendStats type)
{
  return "NULL";
}

//////////////////////////////////////////////////////////////////////

void CNULLRenderer::ClearBuffer(uint32 nFlags, ColorF *vColor, float depth)
{
}

void CNULLRenderer::ReadFrameBuffer(unsigned char * pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY)
{
}

void CNULLRenderer::ReadFrameBufferFast(uint32* pDstARGBA8, int dstWidth, int dstHeight)
{
}

void CNULLRenderer::SetFogColor(float * color)
{
}

void CNULLRenderer::DrawQuad(float dy,float dx, float dz, float x, float y, float z)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::Set2DMode(bool enable, int ortox, int ortoy, float znear, float zfar)
{ 
}

int CNULLRenderer::CreateRenderTarget (int nWidth, int nHeight, ETEX_Format eTF)
{
  return 0;
}

bool CNULLRenderer::DestroyRenderTarget (int nHandle)
{
  return true;
}

bool CNULLRenderer::SetRenderTarget (int nHandle, int nFlags)
{
  return true;
}

bool CNULLRenderer::GetOcclusionBuffer(uint16* pOutOcclBuffer, int32 nSizeX, int32 nSizeY, Matrix44* pmViewProj)
{
  return true;
}

IColorGradingController* CNULLRenderer::GetIColorGradingController()
{
	return m_pNULLColorGradingController;
}

//=========================================================================================


ILog     *iLog;
IConsole *iConsole;
ITimer   *iTimer;
ISystem  *iSystem;

extern "C" DLL_EXPORT IRenderer* CreateCryRenderInterface(ISystem *pSystem)
{
	ModuleInitISystem(pSystem, "CryRenderer");

	gbRgb = false;

	iConsole	= gEnv->pConsole;
	iLog			= gEnv->pLog;
	iTimer		= gEnv->pTimer;
	iSystem		= gEnv->pSystem;

	CRenderer *rd = new CNULLRenderer();

#ifdef LINUX
	srand( clock() );
#else
	srand( GetTickCount() );
#endif

	return rd;
}

class CEngineModule_CryRenderer : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryRenderer, "EngineModule_CryRenderer", 0x540c91a7338e41d3, 0xaceeac9d55614450)

	virtual const char* GetName() {return "CryRenderer";}
	virtual const char* GetCategory() {return "CryEngine";}

	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
	{
		ISystem* pSystem = env.pSystem;
		env.pRenderer = CreateCryRenderInterface(pSystem);
		return env.pRenderer != 0;
	}
};

CRYREGISTER_CLASS(CEngineModule_CryRenderer)

CEngineModule_CryRenderer::CEngineModule_CryRenderer()
{
};

CEngineModule_CryRenderer::~CEngineModule_CryRenderer()
{
};

void *gGet_D3DDevice()
{
  return NULL;
}
void *gGet_glReadPixels()
{
  return NULL;
}

IVideoPlayer* CRenderer::CreateVideoPlayerInstance() const 
{ 
	return 0; 
}

void COcclusionQuery::Create()
{

}

void COcclusionQuery::Release()
{

}

void COcclusionQuery::BeginQuery()
{

}

void COcclusionQuery::EndQuery()
{

}

uint32 COcclusionQuery::GetVisibleSamples( bool bAsynchronous )
{
  return 0;
}

void CRenderer::CheckRenderObjVisArea( CRenderObject *pObj )
{

}

TArray<CDLight>* CRenderer::EF_GetDeferredLights(const eDeferredLightType eLightType)
{
	return NULL;
}

void CNULLRenderer::PrecacheResources()
{

}
