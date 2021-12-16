////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   cry3denginebase.h
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Access to external stuff used by 3d engine. Most 3d engine classes 
//               are derived from this base class to access other interfaces
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _Cry3DEngineBase_h_
#define _Cry3DEngineBase_h_

struct ISystem;
struct IRenderer;
struct ILog;
struct IPhysicalWorld;
struct ITimer;
struct IConsole;
struct I3DEngine;
struct CVars;
struct CVisAreaManager;
struct IMaterialManager;
class  CTerrain;
class	 CIndirectLighting;
class  CObjManager;
class C3DEngine;
class CPartManager;
class CDecalManager;
class CRainManager;
class CCloudsManager;
class CSkyLightManager;
class CWaterWaveManager;
class CRenderMeshMerger;

#define DISTANCE_TO_THE_SUN 1000000

// since the Timer could't be devirtualized(there are two implementations of it)
// this approach is used, to prevent needing an virtual call and main memory lookup each time
// a timer function is used.
// if this code is used on spu, Timer.cpp must also be scanned to provide the functions used to init the Timer
// usses a second define to prevent compile error if scanning while don't including Timer.cpp































struct Cry3DEngineBase
{
  static ISystem * m_pSystem;
  static IRenderer * m_pRenderer;
  static ITimer * m_pTimer;
  static ILog * m_pLog;
  static IPhysicalWorld * m_pPhysicalWorld;
  static IConsole * m_pConsole;
  static C3DEngine * m_p3DEngine;
  static CVars * m_pCVars;
  static ICryPak * m_pCryPak;
	static IMaterialManager* m_pMaterialManager;
	static CObjManager *m_pObjManager;
	static CTerrain *m_pTerrain;
  static class CVoxTerrain *m_pVoxTerrain;
	static IParticleManager* m_pPartManager;
	static CDecalManager  * m_pDecalManager;
	static CCloudsManager* m_pCloudsManager;
	static CVisAreaManager* m_pVisAreaManager;
	static CMatMan        * m_pMatMan;
	static CSkyLightManager* m_pSkyLightManager;
  static CWaterWaveManager *m_pWaterWaveManager;
	static CRenderMeshMerger *m_pRenderMeshMerger;

  static int m_nRenderStackLevel;
	static float m_fZoomFactor;
  static int m_dwRecursionDrawFlags[MAX_RECURSION_LEVELS];
	static int m_nRenderFrameID;
	static uint32 m_nRenderMainFrameID;
  static int m_nRenderThreadListID;
	static bool m_bProfilerEnabled;
  static bool m_bRenderIntoShadowmap;
  static uint m_nMainThreadId;

  static int m_CpuFlags;
  static ESystemConfigSpec m_LightConfigSpec;
#if (defined(PS3) || defined(XENON))
	static const bool m_bEditor = false;
#else
	static bool m_bEditor;
#endif
	static CCamera m_Camera;
	static int m_arrInstancesCounter[eERType_TypesNum];

	// components access
  ILINE static ISystem						* GetSystem() { return m_pSystem; }
  ILINE static IRenderer					* GetRenderer() { return m_pRenderer; }






  ILINE static ITimer							* GetTimer() { return m_pTimer; }

  ILINE static ILog								* GetLog() 
	{ 



		return m_pLog; 

	}
  inline static IPhysicalWorld			* GetPhysicalWorld() { return m_pPhysicalWorld;}
  inline static IConsole						* GetConsole() { return m_pConsole; }
  inline static C3DEngine					* Get3DEngine() { return m_p3DEngine; }
	inline static CObjManager				* GetObjManager() { return m_pObjManager; };
	inline static CTerrain						* GetTerrain() { return m_pTerrain; };
  inline static CVars							* GetCVars() { return m_pCVars; }
  inline static CVisAreaManager		* GetVisAreaManager() { return m_pVisAreaManager; }
  inline static ICryPak						* GetPak() { return m_pCryPak; }
	inline static IMaterialManager		* GetMatMan() { return (IMaterialManager*)m_pMatMan; }
	inline static CCloudsManager			* GetCloudsManager() { return m_pCloudsManager; }
  inline static CWaterWaveManager	* GetWaterWaveManager() { return m_pWaterWaveManager; };
	inline static CRenderMeshMerger	* GetSharedRenderMeshMerger() { return m_pRenderMeshMerger; };

  inline static int GetDefSID() { return 0; };
	ILINE static int GetFrameID() { return m_nRenderFrameID; };
	ILINE static uint32 GetMainFrameID() { return m_nRenderMainFrameID; };

	inline static const CCamera & GetCamera() { /*assert(sizeof(Cry3DEngineBase) == 0); */return m_Camera; }
	inline static void SetCamera(const CCamera & newCam) { m_Camera = newCam; }

  float GetCurTimeSec();
	float GetCurAsyncTimeSec();





	void PrintMessage(const char *szText,...) PRINTF_PARAMS(2, 3);
	void PrintMessagePlus(const char *szText,...) PRINTF_PARAMS(2, 3);
	void PrintComment(const char *szText,...) PRINTF_PARAMS(2, 3);


	// Validator warning.
	static void Warning( const char *format,... ) PRINTF_PARAMS(1, 2);
	static void Error( const char *format,... ) PRINTF_PARAMS(1, 2);
	static void FileWarning( int flags,const char *file,const char *format,... )
		PRINTF_PARAMS(3, 4);
	
	CRenderObject * GetIdentityCRenderObject() 
	{
		CRenderObject * pCRenderObject = GetRenderer()->EF_GetObject(true);
    if (!pCRenderObject)
      return NULL;
		pCRenderObject->m_II.m_Matrix.SetIdentity();
		return pCRenderObject;
	}

	static bool IsValidFile( const char *sFilename );
	static bool IsResourceLocked( const char *sFilename );

	static bool IsPreloadEnabled();

	IMaterial * MakeSystemMaterialFromShader(const char * sShaderName);
  void DrawBBoxLabeled( const AABB & aabb, const Matrix34 & m34, const ColorB & col, const char *format, ... ) PRINTF_PARAMS(5, 6);
	void DrawBBox( const Vec3 & vMin, const Vec3 & vMax );
	void DrawBBox( const AABB & box, ColorB col = Col_White );
	void DrawLine( const Vec3 & vMin, const Vec3 & vMax );
	void DrawSphere( const Vec3 & vPos, float fRadius, ColorB color = ColorB(255,255,255,255) );

	int & GetInstCount(EERType eType) { return m_arrInstancesCounter[eType]; }

	uint32 GetMinSpecFromRenderNodeFlags( uint32 dwRndFlags ) { return (dwRndFlags&ERF_SPEC_BITS_MASK) >> ERF_SPEC_BITS_SHIFT; }
	bool CheckMinSpec( uint32 nMinSpec );

  static bool IsEscapePressed();

  size_t fread(
    void *buffer,
    size_t elementSize,
    size_t count,
    FILE *stream)
  {
    size_t res = ::fread(buffer,elementSize,count,stream);
    if(res != count)
      Error("fread() failed");
    return res;
  }

  int fseek (
    FILE *stream,
    long offset,
    int whence
    )
  {
    int res = ::fseek(stream,offset,whence);
    if(res != 0)
      Error("fseek() failed");
    return res;
  }

  inline int IsRenderIntoShadowmap() const { return m_bRenderIntoShadowmap ? 1 : 0; }
};

#endif // _Cry3DEngineBase_h_
