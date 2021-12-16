////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   Material.h
//  Version:     v1.00
//  Created:     3/9/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Material_h__
#define __Material_h__
#pragma once

// disabled the tracing of leaks for CGFs and Materials for Consoles
#if !defined(PS3) && !defined(XENON)
	#define TRACE_MATERIAL_LEAKS
#endif

class CMaterialLayer : public IMaterialLayer
{
public:
  CMaterialLayer(): m_nRefCount( 0 ), m_nFlags( 0 )
  {
  }

  ~CMaterialLayer()
  {
    SAFE_RELEASE(m_pShaderItem.m_pShader);
    SAFE_RELEASE(m_pShaderItem.m_pShaderResources);
  }

  virtual void AddRef() 
  { 
    m_nRefCount++; 
  };

  virtual void Release()
  {
    if (--m_nRefCount <= 0)
    {
      delete this;
    }
  }

  VIRTUAL void Enable( bool bEnable = true)
  {
    m_nFlags |= (bEnable == false)? MTL_LAYER_USAGE_NODRAW : 0;
  }

  VIRTUAL bool IsEnabled() const
  {
    return ( m_nFlags & MTL_LAYER_USAGE_NODRAW )? false: true;
  }

  VIRTUAL void SetShaderItem( const IMaterial *pParentMtl, const SShaderItem & pShaderItem );

  VIRTUAL const SShaderItem &GetShaderItem() const
  {
    return m_pShaderItem;
  }

  VIRTUAL SShaderItem &GetShaderItem()
  {
    return m_pShaderItem;
  }

  VIRTUAL void SetFlags( uint8 nFlags )
  {
    m_nFlags = nFlags;
  }

  VIRTUAL uint8 GetFlags( ) const
  {
    return m_nFlags;
  }

	void GetMemoryUsage( ICrySizer *pSizer );
	size_t GetResourceMemoryUsage( ICrySizer *pSizer );

private:
  uint8 m_nFlags;  
  int m_nRefCount;
  SShaderItem m_pShaderItem;
};

//////////////////////////////////////////////////////////////////////
class CMatInfo : public IMaterial, public stl::intrusive_linked_list_node<CMatInfo>, public Cry3DEngineBase
{
public: 
	CMatInfo();
	~CMatInfo();

	void ShutDown();

	virtual void AddRef() { m_nRefCount++; };
	virtual void Release()
	{
		if (--m_nRefCount <= 0)
			delete this;
	}
	VIRTUAL int GetNumRefs() { return m_nRefCount; };

	//////////////////////////////////////////////////////////////////////////
	// IMaterial implementation
	//////////////////////////////////////////////////////////////////////////
	int Size();

	VIRTUAL IMaterialManager *GetMaterialManager();

	VIRTUAL void SetName( const char *pName );
	VIRTUAL const char* GetName() { return m_sMaterialName; };

	VIRTUAL void SetFlags( int flags ) { m_Flags = flags; };
	VIRTUAL int GetFlags() const { return m_Flags; };

	// Returns true if this is the default material.
	VIRTUAL bool IsDefault();

	VIRTUAL int GetSurfaceTypeId() { return m_nSurfaceTypeId; };

	VIRTUAL void SetSurfaceType( const char *sSurfaceTypeName );
	VIRTUAL ISurfaceType* GetSurfaceType(){return GetMatMan()->GetSurfaceType(m_nSurfaceTypeId,m_sMaterialName);}

	VIRTUAL void SetMatTemplate( const char *sMatTemplate );
	VIRTUAL IMaterial* GetMatTemplate();
	// shader item
	
	VIRTUAL void SetShaderItem( const SShaderItem & _ShaderItem);
// [Alexey] EF_LoadShaderItem return value with RefCount = 1, so if you'll use SetShaderItem after EF_LoadShaderItem use Assign function
	VIRTUAL void AssignShaderItem( const SShaderItem & _ShaderItem);
	VIRTUAL SShaderItem & GetShaderItem() { return m_shaderItem; };

	VIRTUAL SShaderItem& GetShaderItem( int nSubMtlSlot );

	// shader params
	VIRTUAL void SetShaderParams(TArray<SShaderParam> * _pShaderParams) { pShaderParams = _pShaderParams; }
	VIRTUAL const TArray<SShaderParam> * GetShaderParams() { return pShaderParams; }

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL bool SetGetMaterialParamFloat( const char *sParamName,float &v,bool bGet );
	VIRTUAL bool SetGetMaterialParamVec3( const char *sParamName,Vec3 &v,bool bGet );

	VIRTUAL void SetCamera( CCamera &cam );

	//////////////////////////////////////////////////////////////////////////
	// Sub materials.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetSubMtlCount( int numSubMtl );
	VIRTUAL int GetSubMtlCount(){return m_subMtls.size();}
	VIRTUAL IMaterial* GetSubMtl( int nSlot )
	{
		if (m_subMtls.empty() || !(m_Flags&MTL_FLAG_MULTI_SUBMTL))
			return 0; // Not Multi material.
		if (nSlot >= 0 && nSlot < (int)m_subMtls.size())
			return m_subMtls[nSlot];
		else
			return 0;
	}
	VIRTUAL void SetSubMtl( int nSlot,IMaterial *pMtl );
	VIRTUAL void SetUserData( void *pUserData );
	VIRTUAL void* GetUserData() const;

	VIRTUAL IMaterial* GetSafeSubMtl( int nSlot );
  virtual IMaterial* Clone();
  virtual void Copy( IMaterial* pMtlDest, EMaterialCopyFlags flags );

  //////////////////////////////////////////////////////////////////////////
  // Layers
  //////////////////////////////////////////////////////////////////////////
  VIRTUAL void SetLayerCount( uint32 nCount );
  VIRTUAL uint32 GetLayerCount() const;
  VIRTUAL void SetLayer( uint32 nSlot, IMaterialLayer *pLayer);
  VIRTUAL const IMaterialLayer* GetLayer( uint8 nLayersMask, uint8 nLayersUsageMask ) const;
  VIRTUAL const IMaterialLayer* GetLayer( uint32 nSlot ) const;
  VIRTUAL IMaterialLayer *CreateLayer();
	
	// For backward compatibility with OLD cgf files.
	void LoadFromMatEntity( MAT_ENTITY *me,const char *szFolderName );

	// Fill int table with surface ids of sub materials.
	// Return number of filled items.
	int FillSurfaceTypeIds( int pSurfaceIdsTable[] );
  bool IsSubSurfScatterCaster();

	VIRTUAL void GetMemoryUsage( ICrySizer *pSizer );

	VIRTUAL size_t GetResourceMemoryUsage( ICrySizer *pSizer );

	//////////////////////////////////////////////////////////////////////////
	void SetSketchMode( int mode );
	void SetTexelDensityDebug( int mode );

	bool IsPerObjectShadowPassNeeded();

	//////////////////////////////////////////////////////////////////////////
	// Debug routines
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL const char* GetLoadingCallstack( );	// trace leaking materials by callstack

  VIRTUAL void PrecacheTextures( const float fMipFactor, const int nFlags, const int nUpdateStreamingPrioriryRoundId );
  VIRTUAL void PrecacheChunkTextures( const float fInstanceDistance, const int nFlags, const int nUpdateStreamingPrioriryRoundId, CRenderChunk *pRenderChunk );

	VIRTUAL int GetTextureMemoryUsage( ICrySizer *pSizer,int nSubMtlSlot=-1 );

public:
	//////////////////////////////////////////////////////////////////////////
	// for debug purposes
	//////////////////////////////////////////////////////////////////////////
#ifdef TRACE_MATERIAL_LEAKS
	string	m_sLoadingCallstack;
#endif

private:
	friend class CMatMan;
	friend class CMaterialLayer;

	//////////////////////////////////////////////////////////////////////////
	string m_sMaterialName;
	string m_sUniqueMaterialName;
	
	// Id of surface type assigned to this material.
	int m_nSurfaceTypeId;
  // name of mat templalte material
  string m_sMatTemplate;

	//! Number of references to this material.
	int m_nRefCount;
	//! Material flags.
	//! @see EMatInfoFlags
	int m_Flags;

	SShaderItem m_shaderItem;
	_smart_ptr<IShader> m_pPreSketchShader;
	int m_nPreSketchTechnique;

	float fAlpha;

	TArray<SShaderParam> *pShaderParams;

	//! Array of Sub materials.
	typedef std::vector<_smart_ptr<CMatInfo> > SubMtls;
	SubMtls m_subMtls;

	// User data used by Editor.
	void *m_pUserData;

  //! Material layers
  typedef std::vector< _smart_ptr< CMaterialLayer > > MatLayers;
  MatLayers *m_pMaterialLayers;
  
  //! Used for material layers
  mutable uint8 m_nActiveLayerMask;
  mutable uint8 m_nActiveLayerUsageMask;
  mutable CMaterialLayer *m_pActiveLayer;

  //! Used for textures streaming
  int m_nUpdateStreamingPrioriryRoundId;
  float m_fMinMipFactor;
	bool m_bStartLoading;

	//////////////////////////////////////////////////////////////////////////
};

#endif // __Material_h__

