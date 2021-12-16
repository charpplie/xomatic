////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   MatMan.h
//  Version:     v1.00
//  Created:     23/8/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MatMan_h__
#define __MatMan_h__
#pragma once

#include "Cry3DEngineBase.h"
#include "SurfaceTypeManager.h"

// forward declarations.
struct IMaterial;
struct ISurfaceType;
struct ISurfaceTypeManager;
class CMatInfo;


//////////////////////////////////////////////////////////////////////////
//
// CMatMan is a material manager class.
//
//////////////////////////////////////////////////////////////////////////
class CMatMan : public IMaterialManager,public ISystemEventListener, public Cry3DEngineBase
{
public:
	CMatMan();
	virtual ~CMatMan();

	void ShutDown();

	// interface IMaterialManager --------------------------------------------------------

	VIRTUAL IMaterial* CreateMaterial( const char *sMtlName,int nMtlFlags=0 );
	VIRTUAL IMaterial* FindMaterial( const char *sMtlName ) const;
	VIRTUAL IMaterial* LoadMaterial( const char *sMtlName,bool bMakeIfNotFound=true, bool bNonremovable = false,unsigned long nLoadingFlags=0  );
	VIRTUAL IMaterial* LoadMaterialFromXml( const char *sMtlName,XmlNodeRef mtlNode );
	VIRTUAL void SetListener( IMaterialManagerListener *pListener ) { m_pListener = pListener; };
	VIRTUAL IMaterial* GetDefaultMaterial();
	VIRTUAL IMaterial* GetDefaultTerrainLayerMaterial()
	{		
		if (!m_bInitialized)
		{



			InitDefaults();

		}
		return m_pDefaultTerrainLayersMtl;
	}
  VIRTUAL IMaterial* GetDefaultLayersMaterial();
	VIRTUAL IMaterial* GetDefaultHelperMaterial();
	VIRTUAL IMaterial* GetNoDrawMaterial();
	VIRTUAL ISurfaceType* GetSurfaceTypeByName( const char *sSurfaceTypeName,const char *sWhy=NULL );
	VIRTUAL int GetSurfaceTypeIdByName( const char *sSurfaceTypeName,const char *sWhy=NULL );
	VIRTUAL ISurfaceType* GetSurfaceType( int nSurfaceTypeId,const char *sWhy=NULL )
	{
		return m_pSurfaceTypeManager->GetSurfaceTypeFast(nSurfaceTypeId,sWhy);
	}
	VIRTUAL ISurfaceTypeManager* GetSurfaceTypeManager() { return m_pSurfaceTypeManager; }
	VIRTUAL IMaterial* LoadCGFMaterial( CMaterialCGF *pMaterialCGF,const char *sCgfFilename,unsigned long nLoadingFlags=0);
	VIRTUAL IMaterial* CloneMaterial( IMaterial* pMtl,int nSubMtl=-1 );
	VIRTUAL IMaterial* CloneMultiMaterial( IMaterial* pMtl,const char *sSubMtlName=0 );
	VIRTUAL void GetLoadedMaterials( IMaterial **pData, uint32 &nObjCount ) const;
  VIRTUAL bool SaveMaterial( XmlNodeRef mtlNode, IMaterial *pMtl );
  VIRTUAL void CopyMaterial( IMaterial* pMtlSrc, IMaterial* pMtlDest, EMaterialCopyFlags flags );

	VIRTUAL void RenameMaterial( IMaterial *pMtl,const char *sNewName );	
	// ------------------------------------------------------------------------------------

	// interface ISystemEventListener -------------------------------------------------------
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam );
	// ------------------------------------------------------------------------------------


	void InitDefaults();

	void PrecacheDecalMaterials();
	void ClearDecalMaterials();

	void PreloadLevelMaterials();

	void LoadMaterialsLibrary( const char *sMtlFile,XmlNodeRef &levelDataRoot );
	void SetSketchMode( int mode );
	int  GetSketchMode() { return e_sketch_mode; }
	void SetTexelDensityDebug( int mode );
	int  GetTexelDensityDebug() { return e_texeldensity; }
	
	//////////////////////////////////////////////////////////////////////////
	ISurfaceType* GetSurfaceTypeFast( int nSurfaceTypeId,const char *sWhy=NULL ) { return m_pSurfaceTypeManager->GetSurfaceTypeFast(nSurfaceTypeId,sWhy); }
	
private: // -----------------------------------------------------------------------------
	friend class CMatInfo;
	void Unregister( CMatInfo *pMat );	
	IMaterial* OldLoadMaterialFromXml( XmlNodeRef mtlNode,const char *sLibraryName,bool bPureChild=false );

  bool LoadMaterialShader( IMaterial *pMtl, const char *sShader, uint64 nShaderGenMask, SInputShaderResources &sr, XmlNodeRef &publicsNode,unsigned long nLoadingFlags=0 );
  bool LoadMaterialLayerSlot( uint32 nSlot, IMaterial *pMtl, const char *szShaderName, SInputShaderResources &pBaseResources, XmlNodeRef &pPublicsNode, uint8 nLayerFlags);

	void ParsePublicParams( SInputShaderResources &sr,XmlNodeRef paramsNode );
	const char* UnifyName( const char *sMtlName ) const;
	// Can be called after material creation and initialization, to inform editor that new material in engine exist.
	// Only used internally.
	void NotifyCreateMaterial( IMaterial *pMtl );
	// Make a valid material from the XML node.
	IMaterial* MakeMaterialFromXml( const char *sMtlName,XmlNodeRef node,bool bForcePureChild, uint16 sortPrio = 0,IMaterial *pExistingMtl=0,unsigned long nLoadingFlags=0 );

	void FreeAllMaterials();

private:
	typedef std::map<string,IMaterial*> MtlNameMap;

	MtlNameMap													m_mtlNameMap;									//

	IMaterialManagerListener *					m_pListener;									//
	_smart_ptr<CMatInfo>								m_pDefaultMtl;								//
  _smart_ptr<IMaterial>								m_pDefaultLayersMtl;					//
	_smart_ptr<IMaterial>								m_pDefaultTerrainLayersMtl;		//
	_smart_ptr<CMatInfo>								m_pNoDrawMtl;									//
	_smart_ptr<CMatInfo>                m_pDefaultHelperMtl;

	std::vector<_smart_ptr<CMatInfo> >	m_nonRemovables;							//

	CSurfaceTypeManager *								m_pSurfaceTypeManager;				//

	//////////////////////////////////////////////////////////////////////////
	// Cached XML parser.
	_smart_ptr<IXmlParser> m_pXmlParser;

	bool m_bInitialized;

	static int e_sketch_mode;
  static int e_lowspec_mode;
	static int e_pre_sketch_spec;
	static int e_texeldensity;
};

#endif // __MatMan_h__

