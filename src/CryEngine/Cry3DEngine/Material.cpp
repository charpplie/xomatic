////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   Material.cpp
//  Version:     v1.00
//  Created:     3/9/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Material.h"
#include "MatMan.h"
#include <IRenderer.h>

DEFINE_INTRUSIVE_LINKED_LIST(CMatInfo)

//////////////////////////////////////////////////////////////////////////
void CMaterialLayer::SetShaderItem( const IMaterial *pParentMtl, const SShaderItem &pShaderItem)
{
  assert(pParentMtl && "CMaterialLayer::SetShaderItem invalid material");

  if (pShaderItem.m_pShader)
  {
    pShaderItem.m_pShader->AddRef();
  }
  
  if (pShaderItem.m_pShaderResources)  
  {
    pShaderItem.m_pShaderResources->AddRef();
		CMatInfo *pParentMatInfo = (CMatInfo*)(const_cast<IMaterial*>(pParentMtl));
    pShaderItem.m_pShaderResources->SetMaterialName(pParentMatInfo->m_sUniqueMaterialName);
  }

  SAFE_RELEASE( m_pShaderItem.m_pShader );
  SAFE_RELEASE( m_pShaderItem.m_pShaderResources );

  m_pShaderItem = pShaderItem;

}
//////////////////////////////////////////////////////////////////////////
void CMaterialLayer::GetMemoryUsage( ICrySizer *pSizer )
{
	SIZER_COMPONENT_NAME(pSizer, "MaterialLayer");
	pSizer->AddObject(this, sizeof(*this));

	// Pushing some context to use the Macro.
	{
		SIZER_COMPONENT_NAME(pSizer, "ShaderItem");
		pSizer->AddObject(&m_pShaderItem, sizeof(m_pShaderItem));
	}
}
//////////////////////////////////////////////////////////////////////////
size_t CMaterialLayer::GetResourceMemoryUsage( ICrySizer *pSizer )
{
	size_t nResourceMemory(0);
	// Pushing some context to use the Macro.
	{
		SIZER_COMPONENT_NAME(pSizer, "Textures");
		size_t	nCount(0);
		size_t  nTotal(0);
		size_t	nCurrentResourceMemoryUsage(0);

		IRenderShaderResources	*piResources(m_pShaderItem.m_pShaderResources);
		if (piResources)
		{
			//////////////////////////////////////////////////////////////////////////
			// Texture
			for (nCount=0;nCount<EFTT_MAX;++nCount)
			{
				SEfResTexture*	piTextureResource(piResources->GetTexture(nCount));
				if (piTextureResource)
				{
					ITexture*	piTexture=piTextureResource->m_Sampler.m_pITex;
					if (piTexture)
					{
						SIZER_COMPONENT_NAME(pSizer, "MemoryTexture");
						nCurrentResourceMemoryUsage=piTexture->GetDataSize();
						nResourceMemory+=nCurrentResourceMemoryUsage;
						pSizer->AddObject(piTexture,nCurrentResourceMemoryUsage);

						if (&pSizer->GetResourceCollector())
						{
							pSizer->GetResourceCollector().AddResource(piTexture->GetName(),nCurrentResourceMemoryUsage);
						}
					}
				}
			}
			//////////////////////////////////////////////////////////////////////////
		}
	}
	return nResourceMemory;
}

//////////////////////////////////////////////////////////////////////////
CMatInfo::CMatInfo()
{
	m_nRefCount = 0;
	m_Flags=0;
	fAlpha=0;
	pShaderParams=0;

	fAlpha=1;
	m_nSurfaceTypeId = 0;

  m_pMaterialLayers = 0;

  m_ucDefautMappingAxis = 0;
	m_fDefautMappingScale = 1.f;

	m_pPreSketchShader = 0;
	m_nPreSketchTechnique = 0;
	m_pUserData = NULL;

  m_nActiveLayerMask = (1<<7);
  m_nActiveLayerUsageMask = (1<<7);

  m_pActiveLayer = NULL;

  m_nUpdateStreamingPrioriryRoundId = 0;
  m_fMinMipFactor = 1000000.f;
	m_bStartLoading = false;

#ifdef TRACE_MATERIAL_LEAKS
	m_sLoadingCallstack = GetSystem()->GetLoadingProfilerCallstack();
#endif
}

//////////////////////////////////////////////////////////////////////////
CMatInfo::~CMatInfo()
{
  ShutDown();
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::ShutDown()
{
	SAFE_DELETE(m_pMaterialLayers);

	if (m_shaderItem.m_pShaderResources)
	{
		CCamera *pC = m_shaderItem.m_pShaderResources->GetCamera();
		if (pC)
			delete pC;
	}
	SAFE_RELEASE( m_shaderItem.m_pShader );
	SAFE_RELEASE( m_shaderItem.m_pShaderResources );

	((CMatMan*)GetMatMan())->Unregister( this );
	
	m_subMtls.clear();
}

//////////////////////////////////////////////////////////////////////////
IMaterialManager* CMatInfo::GetMaterialManager()
{
	return GetMatMan();
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetName( const char *sName )
{
	m_sMaterialName = sName;
	m_sUniqueMaterialName = m_sMaterialName;
	if (m_shaderItem.m_pShaderResources)
		m_shaderItem.m_pShaderResources->SetMaterialName(m_sUniqueMaterialName); // Only for correct warning message purposes.
	if (m_Flags & MTL_FLAG_MULTI_SUBMTL)
	{
		for (int i = 0,num = m_subMtls.size(); i < num; i++)
		{
			if (m_subMtls[i] && (m_subMtls[i]->m_Flags&MTL_FLAG_PURE_CHILD))
			{
				m_subMtls[i]->m_sUniqueMaterialName = m_sMaterialName;
				if (m_subMtls[i]->m_shaderItem.m_pShaderResources)
					m_subMtls[i]->m_shaderItem.m_pShaderResources->SetMaterialName(m_sUniqueMaterialName);
			}

			if (m_subMtls[i] && strstr(m_subMtls[i]->m_sUniqueMaterialName, MTL_SPECIAL_NAME_RAYCAST_PROXY) != 0)
			{
				m_subMtls[i]->m_Flags |= MTL_FLAG_RAYCAST_PROXY;
				m_subMtls[i]->m_Flags |= MTL_FLAG_NODRAW;
			}
		}
	}
	if (strstr(sName,MTL_SPECIAL_NAME_COLLISION_PROXY) != 0)
	{
		m_Flags |= MTL_FLAG_COLLISION_PROXY;
	}
	else if (strstr(sName, MTL_SPECIAL_NAME_RAYCAST_PROXY) != 0)
	{
		m_Flags |= MTL_FLAG_RAYCAST_PROXY;
		m_Flags |= MTL_FLAG_NODRAW;
	}
}

//////////////////////////////////////////////////////////////////////////
bool CMatInfo::IsDefault()
{
	return this == GetMatMan()->GetDefaultMaterial();
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetShaderItem( const SShaderItem & _ShaderItem)
{
	if (_ShaderItem.m_pShader)
		_ShaderItem.m_pShader->AddRef();
	if (_ShaderItem.m_pShaderResources)
	{
		_ShaderItem.m_pShaderResources->AddRef();
		_ShaderItem.m_pShaderResources->SetMaterialName(m_sUniqueMaterialName);
	}

  SAFE_RELEASE( m_shaderItem.m_pShader );
  SAFE_RELEASE( m_shaderItem.m_pShaderResources );
  
  m_shaderItem = _ShaderItem;

	if(m_shaderItem.m_pShader && !m_shaderItem.IsZWrite() 
		&& !(m_shaderItem.m_pShader->GetFlags2() & EF2_NODRAW) && !(m_shaderItem.m_pShader->GetFlags() & EF_DECAL))
		m_Flags |= MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED;
	else if(m_shaderItem.m_pShaderResources && m_shaderItem.m_pShaderResources->GetGlow())
    m_Flags |= MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED;
	else if(m_shaderItem.m_pShader && m_shaderItem.m_pShader->GetFlags2()&EF2_DEPTHMAP_SUBSURFSCATTER) 
    m_Flags |= MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED;
  else if(m_shaderItem.m_pShader && m_shaderItem.m_pShader->GetFlags2()&EF2_HAIR) 
    m_Flags |= MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED;
  else
		m_Flags &= ~MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED;

	int sketchMode = m_pMatMan->GetSketchMode();
	if (sketchMode)
		SetSketchMode(sketchMode);
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::AssignShaderItem( const SShaderItem & _ShaderItem)
{
	//if (_ShaderItem.m_pShader)
	//	_ShaderItem.m_pShader->AddRef();
	if (_ShaderItem.m_pShaderResources)
	{
		_ShaderItem.m_pShaderResources->SetMaterialName(m_sUniqueMaterialName);
	}

	SAFE_RELEASE( m_shaderItem.m_pShader );
	SAFE_RELEASE( m_shaderItem.m_pShaderResources );

	m_shaderItem = _ShaderItem;

  if(m_shaderItem.m_pShader && !m_shaderItem.IsZWrite() 
    && !(m_shaderItem.m_pShader->GetFlags2() & EF2_NODRAW) && !(m_shaderItem.m_pShader->GetFlags() & EF_DECAL))
    m_Flags |= MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED;
  else if(m_shaderItem.m_pShaderResources && m_shaderItem.m_pShaderResources->GetGlow())
    m_Flags |= MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED;
  else if(m_shaderItem.m_pShader && m_shaderItem.m_pShader->GetFlags2()&EF2_DEPTHMAP_SUBSURFSCATTER) 
    m_Flags |= MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED;
  else
    m_Flags &= ~MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED;
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetSurfaceType( const char *sSurfaceTypeName )
{
	m_nSurfaceTypeId = 0;

	ISurfaceType *pSurfaceType = GetMatMan()->GetSurfaceTypeByName(sSurfaceTypeName,m_sMaterialName);
	if (pSurfaceType)
		m_nSurfaceTypeId = pSurfaceType->GetId();
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetMatTemplate( const char *sMatTemplate )
{
  m_sMatTemplate = sMatTemplate;
}
IMaterial* CMatInfo::GetMatTemplate()
{
  if( !m_sMatTemplate.empty() )
  {
    return GetMatMan()->LoadMaterial(m_sMatTemplate,false);
  }
  return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetSubMtlCount( int numSubMtl )
{
	m_Flags |= MTL_FLAG_MULTI_SUBMTL;
	m_subMtls.resize(numSubMtl);
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatInfo::GetSafeSubMtl( int nSubMtlSlot )
{
	if (m_subMtls.empty() || !(m_Flags&MTL_FLAG_MULTI_SUBMTL))
	{
		return this; // Not Multi material.
	}
	if (nSubMtlSlot >= 0 && nSubMtlSlot < (int)m_subMtls.size() && m_subMtls[nSubMtlSlot] != NULL)
		return m_subMtls[nSubMtlSlot];
	else
		return GetMatMan()->GetDefaultMaterial();
}

//////////////////////////////////////////////////////////////////////////
SShaderItem& CMatInfo::GetShaderItem( int nSubMtlSlot )
{
	if (m_subMtls.empty() || !(m_Flags&MTL_FLAG_MULTI_SUBMTL))
	{
		return m_shaderItem; // Not Multi material.
	}
	if (nSubMtlSlot >= 0 && nSubMtlSlot < (int)m_subMtls.size() && m_subMtls[nSubMtlSlot] != NULL)
		return m_subMtls[nSubMtlSlot]->m_shaderItem;
	else
		return ((CMatInfo*)(GetMatMan()->GetDefaultMaterial()))->m_shaderItem;
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetSubMtl( int nSlot,IMaterial *pMtl )
{
	assert( nSlot >= 0 && nSlot < (int)m_subMtls.size());
	m_subMtls[nSlot] = (CMatInfo*)pMtl;
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetLayerCount( uint32 nCount )
{
  if( !m_pMaterialLayers )
  {
    m_pMaterialLayers = new MatLayers;
  }

  m_pMaterialLayers->resize( nCount );
}

//////////////////////////////////////////////////////////////////////////
uint32 CMatInfo::GetLayerCount() const
{
  if( m_pMaterialLayers )
  {
    return (uint32) m_pMaterialLayers->size();
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetLayer( uint32 nSlot, IMaterialLayer *pLayer)
{
  assert( m_pMaterialLayers );
  assert( nSlot < (uint32) m_pMaterialLayers->size() );

  if( m_pMaterialLayers && pLayer && nSlot < (uint32) m_pMaterialLayers->size() )
    (* m_pMaterialLayers)[nSlot] = static_cast< CMaterialLayer *>( pLayer );
}

//////////////////////////////////////////////////////////////////////////

const IMaterialLayer* CMatInfo::GetLayer( uint8 nLayersMask, uint8 nLayersUsageMask ) const
{   
  if( m_pMaterialLayers && nLayersMask )    
  {
    //if( m_nActiveLayerMask == nLayersMask)
    //  return m_pActiveLayer;
    
    int nSlotCount = m_pMaterialLayers->size();
    for( int nSlot = 0; nSlot < nSlotCount; ++nSlot )
    {
      CMaterialLayer *pLayer = static_cast< CMaterialLayer *>((* m_pMaterialLayers)[nSlot] );

      if( nLayersMask & (1 << nSlot)) 
      {
        m_nActiveLayerMask = nLayersMask;
        m_nActiveLayerUsageMask = nLayersUsageMask;

        if( pLayer )
        {
          m_pActiveLayer = pLayer;
          return m_pActiveLayer;
        } 

        m_pActiveLayer = 0;

        return 0;
      }
    }
  }
  
  return 0;
}

//////////////////////////////////////////////////////////////////////////
const IMaterialLayer* CMatInfo::GetLayer( uint32 nSlot ) const
{
  if( m_pMaterialLayers && nSlot < (uint32) m_pMaterialLayers->size() )
    return static_cast< CMaterialLayer *>((* m_pMaterialLayers)[nSlot] );

  return 0;
}

//////////////////////////////////////////////////////////////////////////
IMaterialLayer *CMatInfo::CreateLayer()
{
  return new CMaterialLayer;
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetUserData( void *pUserData )
{
	m_pUserData = pUserData;
}

//////////////////////////////////////////////////////////////////////////
void* CMatInfo::GetUserData() const
{
	return m_pUserData;
}

//////////////////////////////////////////////////////////////////////////
inline int Material_SetTexType(TextureMap3 *tm)
{
	if (tm->type == TEXMAP_CUBIC)
		return eTT_Cube;
	else
	if (tm->type == TEXMAP_AUTOCUBIC)
		return eTT_AutoCube;
	return eTT_2D;
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::LoadFromMatEntity( MAT_ENTITY *me,const char *szFolderName )
{
	SInputShaderResources Res;

	Res.m_LMaterial.m_Diffuse  = ColorF(me->col_d.r/255.f,me->col_d.g/255.f,me->col_d.b/255.f);
	Res.m_LMaterial.m_Specular = ColorF(me->col_s.r/255.f,me->col_s.g/255.f,me->col_s.b/255.f);
	Res.m_LMaterial.m_Specular *= me->specLevel;
	Res.m_LMaterial.m_Specular.Clamp();
	Res.m_LMaterial.m_Emission = Res.m_LMaterial.m_Diffuse * me->selfIllum; 
	Res.m_LMaterial.m_SpecShininess = me->specShininess;

	char diffuse[256]="";
	strcpy(diffuse, me->map_d.name);

	char bump[256]="";
	strcpy(bump, me->map_b.name);

	char normalmap[256]="";
	if(me->map_displ.name[0] && (me->flags & MAT_ENTITY::MTLFLAG_CRYSHADER))
		strcpy(normalmap, me->map_displ.name);

	if (me->flags & MAT_ENTITY::MTLFLAG_WIRE)
		m_Flags |= MTL_FLAG_WIRE;
	if (me->flags & MAT_ENTITY::MTLFLAG_2SIDED)
		m_Flags |= MTL_FLAG_2SIDED;

	if (me->flags & MAT_ENTITY::MTLFLAG_CRYSHADER)
	{
		/*
		if (me->flags & MAT_ENTITY::MTLFLAG_PHYSICALIZE)
			m_Flags &= ~MTL_FLAG_NOPHYSICALIZE;
		else
			m_Flags |= MTL_FLAG_NOPHYSICALIZE;
			*/
		if (me->flags & MAT_ENTITY::MTLFLAG_ADDITIVE)
			m_Flags |= MTL_FLAG_ADDITIVE;
		if (me->flags & MAT_ENTITY::MTLFLAG_DETAILDECAL)
			m_Flags |= MTL_FLAG_DETAIL_DECAL;
	}
	else
	{
		/*
		if (me->Dyn_Bounce == 1.0f)
			m_Flags &= ~MTL_FLAG_NOPHYSICALIZE;
		else
			m_Flags |= MTL_FLAG_NOPHYSICALIZE;
		*/

		if (me->Dyn_StaticFriction == 1.0f)
			m_Flags |= MTL_FLAG_NOSHADOW;
	}

	char opacity[256]="";
	char decal[256]="";
	if(me->map_o.name[0])
	{
		if (me->flags & MAT_ENTITY::MTLFLAG_CRYSHADER)
			strcpy(decal, me->map_o.name);
		else
			strcpy(opacity, me->map_o.name);
	}

	char gloss[256]="";
	if(me->map_g.name[0])
		strcpy(gloss, me->map_g.name);

	char cubemap[256]="";

	char env[256]="";
	if(me->map_e.name[0])
		strcpy(env, me->map_e.name);

	char spec[256]="";
	if(me->map_s.name[0])
		strcpy(spec, me->map_s.name);

	char det[256]="";
	if(me->map_detail.name[0])
		strcpy(det, me->map_detail.name);

	char subsurf[256]="";
	if(me->map_subsurf.name[0])
		strcpy(subsurf, me->map_subsurf.name);

	char refl[256]="";
	if(me->map_e.name[0])
		strcpy(refl, me->map_e.name);

  char custom_map[256]="";
  if(me->map_custom.name[0])
    strcpy(custom_map, me->map_custom.name);  

  char custom_secondary_map[256]="";
  if(me->map_custom_secondary.name[0])
    strcpy(custom_secondary_map, me->map_custom_secondary.name);  
  
	char * mat_name = me->name;

	// fill MatInfo struct
	//  CMatInfo newMat & = MatInfo;
	//  strcpy(szDiffuse, diffuse);

	/*  if (nLM > 0)
	{
	Res.m_Textures[EFTT_LIGHTMAP].m_TU.m_ITexPic = m_pSystem->GetIRenderer()->EF_GetTextureByID(nLM);
	Res.m_Textures[EFTT_LIGHTMAP].m_Name = Res.m_Textures[EFTT_LIGHTMAP].m_TU.m_ITexPic->GetName();
	}
	if (nLM_LD > 0)
	{
	Res.m_Textures[EFTT_LIGHTMAP_DIR].m_TU.m_ITexPic = m_pSystem->GetIRenderer()->EF_GetTextureByID(nLM_LD);
	Res.m_Textures[EFTT_LIGHTMAP_DIR].m_Name = Res.m_Textures[EFTT_LIGHTMAP].m_TU.m_ITexPic->GetName();
	}*/

	Res.m_TexturePath = szFolderName;
	Res.m_Textures[EFTT_DIFFUSE].m_Name = diffuse;
	Res.m_Textures[EFTT_GLOSS].m_Name = gloss;
	Res.m_Textures[EFTT_SUBSURFACE].m_Name = subsurf;
	Res.m_Textures[EFTT_BUMP].m_Name = bump;

	if (strlen(bump) == 0 && strlen(normalmap) != 0)
		Res.m_Textures[EFTT_BUMP].m_Name = normalmap;

  Res.m_Textures[EFTT_ENV].m_Name = cubemap[0] ? cubemap : env[0] ? env : refl;
	Res.m_Textures[EFTT_SPECULAR].m_Name = spec;
	Res.m_Textures[EFTT_DETAIL_OVERLAY].m_Name = det;
	Res.m_Textures[EFTT_OPACITY].m_Name = opacity;
	Res.m_Textures[EFTT_DECAL_OVERLAY].m_Name = decal;
	Res.m_Textures[EFTT_SUBSURFACE].m_Name = subsurf;
  Res.m_Textures[EFTT_CUSTOM].m_Name = custom_map;
  Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_Name = custom_secondary_map;
	Res.m_Textures[EFTT_ENV].m_Sampler.m_eTexType = (ETEX_Type)Material_SetTexType(&me->map_e);
	Res.m_Textures[EFTT_SUBSURFACE].m_Sampler.m_eTexType = (ETEX_Type)Material_SetTexType(&me->map_subsurf);
	Res.m_Textures[EFTT_BUMP].m_Sampler.m_eTexType = eTT_2D;
	Res.m_ResFlags = me->flags;
	Res.m_Opacity = me->opacity;
	Res.m_AlphaRef = me->Dyn_SlidingFriction;
	if (me->flags & MAT_ENTITY::MTLFLAG_CRYSHADER)
		Res.m_AlphaRef = me->alpharef;

	if (decal[0])
		Res.m_Textures[EFTT_DECAL_OVERLAY].m_Amount = me->map_o.Amount;
	Res.m_Textures[EFTT_DIFFUSE].m_Amount = me->map_d.Amount;
	Res.m_Textures[EFTT_BUMP].m_Amount = me->map_b.Amount;
	if (me->flags & MAT_ENTITY::MTLFLAG_CRYSHADER)
		Res.m_Textures[EFTT_NORMALMAP].m_Amount = me->map_displ.Amount;

	Res.m_Textures[EFTT_OPACITY].m_Amount = me->map_o.Amount;
	Res.m_Textures[EFTT_ENV].m_Amount = me->map_e.Amount;
	Res.m_Textures[EFTT_SUBSURFACE].m_Amount = me->map_subsurf.Amount;
  Res.m_Textures[EFTT_CUSTOM].m_Amount = me->map_custom.Amount;

	Res.m_Textures[EFTT_DIFFUSE].m_TexFlags = me->map_d.flags;
	//Res.m_Textures[EFTT_SUBSURFACE].m_TexFlags = me->map_d.flags;
	Res.m_Textures[EFTT_GLOSS].m_TexFlags = me->map_g.flags;
	Res.m_Textures[EFTT_BUMP].m_TexFlags = me->map_b.flags;
	Res.m_Textures[EFTT_SPECULAR].m_TexFlags = me->map_s.flags;
	Res.m_Textures[EFTT_DETAIL_OVERLAY].m_TexFlags = me->map_detail.flags;
	Res.m_Textures[EFTT_SUBSURFACE].m_TexFlags = me->map_subsurf.flags;
  Res.m_Textures[EFTT_CUSTOM].m_TexFlags = me->map_custom.flags;
  Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_TexFlags = me->map_custom_secondary.flags;

	Res.m_Textures[EFTT_DIFFUSE].m_TexModificator.m_Tiling[0] = me->map_d.uscl_val;
	Res.m_Textures[EFTT_DIFFUSE].m_TexModificator.m_Tiling[1] = me->map_d.vscl_val;
	Res.m_Textures[EFTT_DIFFUSE].m_TexModificator.m_Offs[0] = me->map_d.uoff_val;
	Res.m_Textures[EFTT_DIFFUSE].m_TexModificator.m_Offs[1] = me->map_d.voff_val;
	Res.m_Textures[EFTT_DIFFUSE].m_TexModificator.m_Rot[0] = Degr2Word(me->map_d.urot_val);
	Res.m_Textures[EFTT_DIFFUSE].m_TexModificator.m_Rot[1] = Degr2Word(me->map_d.vrot_val);
	Res.m_Textures[EFTT_DIFFUSE].m_bUTile = me->map_d.utile;
	Res.m_Textures[EFTT_DIFFUSE].m_bVTile = me->map_d.vtile;

	Res.m_Textures[EFTT_BUMP].m_TexModificator.m_Tiling[0] = me->map_b.uscl_val;
	Res.m_Textures[EFTT_BUMP].m_TexModificator.m_Tiling[1] = me->map_b.vscl_val;
	Res.m_Textures[EFTT_BUMP].m_TexModificator.m_Offs[0] = me->map_b.uoff_val;
	Res.m_Textures[EFTT_BUMP].m_TexModificator.m_Offs[1] = me->map_b.voff_val;
	Res.m_Textures[EFTT_BUMP].m_TexModificator.m_Rot[0] = Degr2Word(me->map_b.urot_val);
	Res.m_Textures[EFTT_BUMP].m_TexModificator.m_Rot[1] = Degr2Word(me->map_b.vrot_val);
	Res.m_Textures[EFTT_BUMP].m_bUTile = me->map_b.utile;
	Res.m_Textures[EFTT_BUMP].m_bVTile = me->map_b.vtile;

	Res.m_Textures[EFTT_SPECULAR].m_TexModificator.m_Tiling[0] = me->map_s.uscl_val;
	Res.m_Textures[EFTT_SPECULAR].m_TexModificator.m_Tiling[1] = me->map_s.vscl_val;
	Res.m_Textures[EFTT_SPECULAR].m_TexModificator.m_Offs[0] = me->map_s.uoff_val;
	Res.m_Textures[EFTT_SPECULAR].m_TexModificator.m_Offs[1] = me->map_s.voff_val;
	Res.m_Textures[EFTT_SPECULAR].m_TexModificator.m_Rot[0] = Degr2Word(me->map_s.urot_val);
	Res.m_Textures[EFTT_SPECULAR].m_TexModificator.m_Rot[1] = Degr2Word(me->map_s.vrot_val);
	Res.m_Textures[EFTT_SPECULAR].m_bUTile = me->map_b.utile;
	Res.m_Textures[EFTT_SPECULAR].m_bVTile = me->map_b.vtile;

	Res.m_Textures[EFTT_GLOSS].m_TexModificator.m_Tiling[0] = me->map_g.uscl_val;
	Res.m_Textures[EFTT_GLOSS].m_TexModificator.m_Tiling[1] = me->map_g.vscl_val;
	Res.m_Textures[EFTT_GLOSS].m_TexModificator.m_Offs[0] = me->map_g.uoff_val;
	Res.m_Textures[EFTT_GLOSS].m_TexModificator.m_Offs[1] = me->map_g.voff_val;
	Res.m_Textures[EFTT_GLOSS].m_TexModificator.m_Rot[0] = Degr2Word(me->map_g.urot_val);
	Res.m_Textures[EFTT_GLOSS].m_TexModificator.m_Rot[1] = Degr2Word(me->map_g.vrot_val);
	Res.m_Textures[EFTT_GLOSS].m_bUTile = me->map_g.utile;
	Res.m_Textures[EFTT_GLOSS].m_bVTile = me->map_g.vtile;

	if (!Res.m_Textures[EFTT_DETAIL_OVERLAY].m_Name.empty())
	{
		Res.m_Textures[EFTT_DETAIL_OVERLAY].m_TexModificator.m_Tiling[0] = me->map_detail.uscl_val;
		Res.m_Textures[EFTT_DETAIL_OVERLAY].m_TexModificator.m_Tiling[1] = me->map_detail.vscl_val;
		Res.m_Textures[EFTT_DETAIL_OVERLAY].m_TexModificator.m_Offs[0] = me->map_detail.uoff_val;
		Res.m_Textures[EFTT_DETAIL_OVERLAY].m_TexModificator.m_Offs[1] = me->map_detail.voff_val;
		Res.m_Textures[EFTT_DETAIL_OVERLAY].m_TexModificator.m_Rot[0] = Degr2Word(me->map_detail.urot_val);
		Res.m_Textures[EFTT_DETAIL_OVERLAY].m_TexModificator.m_Rot[1] = Degr2Word(me->map_detail.vrot_val);
		Res.m_Textures[EFTT_DETAIL_OVERLAY].m_bUTile = me->map_detail.utile;
		Res.m_Textures[EFTT_DETAIL_OVERLAY].m_bVTile = me->map_detail.vtile;
	}
	if (!Res.m_Textures[EFTT_OPACITY].m_Name.empty())
	{
		Res.m_Textures[EFTT_OPACITY].m_TexModificator.m_Tiling[0] = me->map_o.uscl_val;
		Res.m_Textures[EFTT_OPACITY].m_TexModificator.m_Tiling[1] = me->map_o.vscl_val;
		Res.m_Textures[EFTT_OPACITY].m_TexModificator.m_Rot[0] = Degr2Word(me->map_o.urot_val);
		Res.m_Textures[EFTT_OPACITY].m_TexModificator.m_Rot[1] = Degr2Word(me->map_o.vrot_val);
		Res.m_Textures[EFTT_OPACITY].m_TexModificator.m_Offs[0] = me->map_o.uoff_val;
		Res.m_Textures[EFTT_OPACITY].m_TexModificator.m_Offs[1] = me->map_o.voff_val;
		Res.m_Textures[EFTT_OPACITY].m_bUTile = me->map_o.utile;
		Res.m_Textures[EFTT_OPACITY].m_bVTile = me->map_o.vtile;
	}
	if (!Res.m_Textures[EFTT_DECAL_OVERLAY].m_Name.empty())
	{
		Res.m_Textures[EFTT_DECAL_OVERLAY].m_TexModificator.m_Tiling[0] = me->map_o.uscl_val;
		Res.m_Textures[EFTT_DECAL_OVERLAY].m_TexModificator.m_Tiling[1] = me->map_o.vscl_val;
		Res.m_Textures[EFTT_DECAL_OVERLAY].m_TexModificator.m_Rot[0] = Degr2Word(me->map_o.urot_val);
		Res.m_Textures[EFTT_DECAL_OVERLAY].m_TexModificator.m_Rot[1] = Degr2Word(me->map_o.vrot_val);
		Res.m_Textures[EFTT_DECAL_OVERLAY].m_TexModificator.m_Offs[0] = me->map_o.uoff_val;
		Res.m_Textures[EFTT_DECAL_OVERLAY].m_TexModificator.m_Offs[1] = me->map_o.voff_val;
		Res.m_Textures[EFTT_DECAL_OVERLAY].m_bUTile = me->map_o.utile;
		Res.m_Textures[EFTT_DECAL_OVERLAY].m_bVTile = me->map_o.vtile;
	}
  if (!Res.m_Textures[EFTT_CUSTOM].m_Name.empty())
  {
    Res.m_Textures[EFTT_CUSTOM].m_TexModificator.m_Tiling[0] = me->map_custom.uscl_val;
    Res.m_Textures[EFTT_CUSTOM].m_TexModificator.m_Tiling[1] = me->map_custom.vscl_val;
    Res.m_Textures[EFTT_CUSTOM].m_TexModificator.m_Rot[0] = Degr2Word(me->map_custom.urot_val);
    Res.m_Textures[EFTT_CUSTOM].m_TexModificator.m_Rot[1] = Degr2Word(me->map_custom.vrot_val);
    Res.m_Textures[EFTT_CUSTOM].m_TexModificator.m_Offs[0] = me->map_custom.uoff_val;
    Res.m_Textures[EFTT_CUSTOM].m_TexModificator.m_Offs[1] = me->map_custom.voff_val;
    Res.m_Textures[EFTT_CUSTOM].m_bUTile = me->map_custom.utile;
    Res.m_Textures[EFTT_CUSTOM].m_bVTile = me->map_custom.vtile;
  }
  if (!Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_Name.empty())
  {
    Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_TexModificator.m_Tiling[0] = me->map_custom.uscl_val;
    Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_TexModificator.m_Tiling[1] = me->map_custom.vscl_val;
    Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_TexModificator.m_Rot[0] = Degr2Word(me->map_custom.urot_val);
    Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_TexModificator.m_Rot[1] = Degr2Word(me->map_custom.vrot_val);
    Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_TexModificator.m_Offs[0] = me->map_custom.uoff_val;
    Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_TexModificator.m_Offs[1] = me->map_custom.voff_val;
    Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_bUTile = me->map_custom.utile;
    Res.m_Textures[EFTT_CUSTOM_SECONDARY].m_bVTile = me->map_custom.vtile;
  }
  
	char mName[128];
	strcpy(mName, mat_name);
	char *str = strchr(mat_name, '/');
	if (str && *str != '\0')
	{
		mName[str-mat_name] = 0;
		SetSurfaceType( str+1 );
	}
	else
	{
		m_nSurfaceTypeId = 0;
	}

	char *templName = NULL;;
	if(strnicmp(mName, "$s_",3)==0)
	{
		templName = &mName[3];
	}
	else if(str=strchr(mName, '('))
	{
		mName[str-mName] = 0;
		templName = &mName[str-mName+1];
		if(str=strchr(templName, ')'))
			templName[str-templName] = 0;
	}

	if (templName && templName[0] == '#')
	{
		templName++;
//		nInvert = 1;
	}

	// SetName(mName);		// set material name
	m_sMaterialName = mName;

	// load shader
	if(mName[0]==0)
		strcpy(mName,"nodraw");

	if(!templName || !templName[0])
		templName = "nodraw";

	if (stricmp(templName,"nodraw") == 0)
		m_Flags |= MTL_FLAG_NODRAW;


	if (stricmp(templName,"TemplPlants") == 0 || stricmp(templName,"TemplPlants1") == 0)
		Res.m_AlphaRef = 0.5f;

  Res.m_szMaterialName = m_sMaterialName;
	m_shaderItem = GetRenderer()->EF_LoadShaderItem(templName, true, 0, &Res);
}

//////////////////////////////////////////////////////////////////////////
int CMatInfo::FillSurfaceTypeIds( int pSurfaceIdsTable[] )
{
	if (m_subMtls.empty() || !(m_Flags&MTL_FLAG_MULTI_SUBMTL))
	{
		pSurfaceIdsTable[0] = m_nSurfaceTypeId;
		return 1; // Not Multi material.
	}
	for (int i = 0; i < (int)m_subMtls.size(); i++)
	{
		if (m_subMtls[i] != 0)
			pSurfaceIdsTable[i] = m_subMtls[i]->m_nSurfaceTypeId;
		else
			pSurfaceIdsTable[i] = 0;
	}
	return m_subMtls.size();
}


void CMatInfo::Copy( IMaterial* pMtlDest, EMaterialCopyFlags flags )
{
  CMatInfo *pMatInfo = static_cast<CMatInfo*>(pMtlDest);
  if( flags & MTL_COPY_NAME )
  {
    pMatInfo->m_sMaterialName = m_sMaterialName;
    pMatInfo->m_sUniqueMaterialName = m_sUniqueMaterialName;
  }
  pMatInfo->m_nSurfaceTypeId = m_nSurfaceTypeId;
  pMatInfo->m_Flags = m_Flags;
  pMatInfo->fAlpha = fAlpha;
  if( flags & MTL_COPY_TEMPLATE )
  {
    pMatInfo->m_sMatTemplate = m_sMatTemplate;
  }

  if(GetShaderItem().m_pShaderResources)
  {
    //
    SShaderItem& siSrc(GetShaderItem());
    SInputShaderResources isr(siSrc.m_pShaderResources);
    //
    SShaderItem& siDstTex(pMatInfo->GetShaderItem());
    SInputShaderResources idsTex(siDstTex.m_pShaderResources);
    if( !(flags & MTL_COPY_TEXTURES) )
    {
      for(int n=0;n<EFTT_MAX;n++)
      {
        isr.m_Textures[n] = idsTex.m_Textures[n];
      }
    }
    SShaderItem siDst(GetRenderer()->EF_LoadShaderItem(siSrc.m_pShader->GetName(), false, 0, &isr, siSrc.m_pShader->GetGenerationMask()));
    pMatInfo->AssignShaderItem(siDst);
    siDst.m_pShaderResources->CloneConstants(siSrc.m_pShaderResources);
  }

  // Clone shader params.
  if (pShaderParams)
  {
    pMatInfo->pShaderParams = new TArray<SShaderParam>;
    int n = pShaderParams->Num();
    for (int i = 0; i < n; i++)
    {
      pMatInfo->pShaderParams->Add( (*pShaderParams)[i] );
    }
  }
}


//////////////////////////////////////////////////////////////////////////
IMaterial* CMatInfo::Clone()
{
	CMatInfo *pMatInfo = new CMatInfo;
	
	pMatInfo->m_sMaterialName = m_sMaterialName;
	pMatInfo->m_sUniqueMaterialName = m_sUniqueMaterialName;
	pMatInfo->m_nSurfaceTypeId = m_nSurfaceTypeId;
	pMatInfo->m_Flags = m_Flags;
	pMatInfo->fAlpha = fAlpha;
  pMatInfo->m_sMatTemplate = m_sMatTemplate;

	/*
	pMatInfo->m_shaderItem.m_pShader = m_shaderItem.m_pShader;
	if (m_shaderItem.m_pShader)
		m_shaderItem.m_pShader->AddRef();

	if (m_shaderItem.m_pShaderResources)
	{
		pMatInfo->m_shaderItem.m_pShaderResources = m_shaderItem.m_pShaderResources->Clone();
		if (pMatInfo->m_shaderItem.m_pShaderResources)
		{
			//pMatInfo->m_shaderItem.m_pShaderResources->AddRef();
			pMatInfo->m_shaderItem.m_pShaderResources->m_szMaterialName = pMatInfo->m_sUniqueMaterialName;
		}
	}
	*/

	SShaderItem& siSrc(GetShaderItem());
	SInputShaderResources isr(siSrc.m_pShaderResources);
	SShaderItem siDst(GetRenderer()->EF_LoadShaderItem(siSrc.m_pShader->GetName(), false, 0, &isr, siSrc.m_pShader->GetGenerationMask()));
	pMatInfo->AssignShaderItem(siDst);
	siDst.m_pShaderResources->CloneConstants(siSrc.m_pShaderResources);

	// Clone shader params.
	if (pShaderParams)
	{
		pMatInfo->pShaderParams = new TArray<SShaderParam>;
		int n = pShaderParams->Num();
		for (int i = 0; i < n; i++)
		{
			pMatInfo->pShaderParams->Add( (*pShaderParams)[i] );
		}
	}

	return pMatInfo;
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::GetMemoryUsage( ICrySizer *pSizer )
{
	SIZER_COMPONENT_NAME(pSizer, "Material");
	pSizer->AddObject(this, sizeof(*this));

	if (pShaderParams)
	{
		int n = pShaderParams->Num();
		for (int i = 0; i < n; i++)
		{
			SIZER_COMPONENT_NAME(pSizer, "ShaderParams");
			pSizer->AddObject(&pShaderParams[i], pShaderParams[i].GetMemoryUsage());			
		}		
	}

	if (m_pMaterialLayers)
	{
		int n = m_pMaterialLayers->size();
		for (int i = 0; i < n; i++)
		{
			CMaterialLayer*	pMaterialLayer=(*m_pMaterialLayers)[i];
			if (pMaterialLayer)
			{
				pMaterialLayer->GetMemoryUsage(pSizer);
			}
		}		
	}

	// all sub material
	{
		int iCnt = GetSubMtlCount();

		for(int i=0;i<iCnt;++i)
		{
			IMaterial*	piMaterial(GetSubMtl(i));
			if (piMaterial)
			{
				piMaterial->GetMemoryUsage(pSizer);
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
size_t CMatInfo::GetResourceMemoryUsage( ICrySizer *pSizer )
{
	size_t	nTotalResourceMemoryUsage(0);

	if (m_pMaterialLayers)
	{
		int n = m_pMaterialLayers->size();
		for (int i = 0; i < n; i++)
		{
			CMaterialLayer*	pMaterialLayer=(*m_pMaterialLayers)[i];
			if (pMaterialLayer)
			{
				nTotalResourceMemoryUsage+=pMaterialLayer->GetResourceMemoryUsage(pSizer);
			}
		}		
	}

	if (m_shaderItem.m_pShaderResources)
	{
		nTotalResourceMemoryUsage+=m_shaderItem.m_pShaderResources->GetResourceMemoryUsage(pSizer);
	}

	// all sub material
	{
		int iCnt = GetSubMtlCount();
		for(int i=0;i<iCnt;++i)
		{
			IMaterial*	piMaterial(GetSubMtl(i));
			if (piMaterial)
			{
				nTotalResourceMemoryUsage+=piMaterial->GetResourceMemoryUsage(pSizer);
			}
		}
	}
	return nTotalResourceMemoryUsage;
}
//////////////////////////////////////////////////////////////////////////
inline bool SetGetParamByName( IRenderShaderResources &shr,bool bGet,const char *sInParamName,float &inValue,const char *sTrgParamName,float &targetVal )
{
	if (strcmp(sInParamName,sTrgParamName) == 0)
	{
		if (bGet)
			inValue = targetVal;
		else
			targetVal = inValue;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
inline bool SetGetParamByName( IRenderShaderResources &shr,bool bGet,const char *sInParamName,Vec3 &inValue,const char *sTrgParamName,Vec3 &targetVal )
{
	if (strcmp(sInParamName,sTrgParamName) == 0)
	{
		if (bGet)
			inValue = targetVal;
		else
			targetVal = inValue;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
inline bool SetGetParamByName( IRenderShaderResources &shr,bool bGet,const char *sInParamName,Vec3 &inValue,const char *sTrgParamName,ColorF &targetVal )
{
	if (strcmp(sInParamName,sTrgParamName) == 0)
	{
		if (bGet)
			inValue = Vec3(targetVal[0],targetVal[1],targetVal[2]);
		else
			targetVal = ColorF(inValue.x,inValue.y,inValue.z,1);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CMatInfo::SetGetMaterialParamFloat( const char *sParamName,float &v,bool bGet )
{
	if (!m_shaderItem.m_pShaderResources)
		return false;
	IRenderShaderResources &shr = *m_shaderItem.m_pShaderResources;
	
	if (     SetGetParamByName(shr,bGet,sParamName,v,"glow",shr.GetGlow()))
		return true;
	else if (SetGetParamByName(shr,bGet,sParamName,v,"shininess",shr.GetSpecularShininess()))
		return true;
	else if (SetGetParamByName(shr,bGet,sParamName,v,"alpha", shr.GetAlphaRef() ))
		return true;
	else if (SetGetParamByName(shr,bGet,sParamName,v,"opacity", shr.GetOpacity() ))
		return true;

	return false;

}

//////////////////////////////////////////////////////////////////////////
bool CMatInfo::SetGetMaterialParamVec3( const char *sParamName,Vec3 &v,bool bGet )
{
	if (!m_shaderItem.m_pShaderResources)
		return false;
	IRenderShaderResources &shr = *m_shaderItem.m_pShaderResources;

	if (     SetGetParamByName(shr,bGet,sParamName,v,"diffuse",shr.GetDiffuseColor() ))
		return true;
	else if (SetGetParamByName(shr,bGet,sParamName,v,"specular",shr.GetSpecularColor() ))
		return true;
	else if (SetGetParamByName(shr,bGet,sParamName,v,"emissive",shr.GetEmissiveColor() ))
		return true;

	return false;

}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetCamera( CCamera &cam )
{
  CCamera *pC = m_shaderItem.m_pShaderResources->GetCamera();
	if (!pC)
		pC = new CCamera;
	m_shaderItem.m_pShaderResources->SetCamera(pC);
}

//////////////////////////////////////////////////////////////////////////
bool CMatInfo::IsSubSurfScatterCaster()
{
  if (m_subMtls.empty() || !(m_Flags&MTL_FLAG_MULTI_SUBMTL))
  {
    if (m_shaderItem.m_pShader != NULL && (m_shaderItem.m_pShader->GetFlags2() & EF2_DEPTHMAP_SUBSURFSCATTER))
    {
      return true;
    }

  }
  else ////////TOFIX
  for (int i = 0; i < (int)m_subMtls.size(); i++)
  {
    if (m_subMtls[i] != NULL && m_subMtls[i]->GetShaderItem().m_pShader != NULL)
    {
      if (m_subMtls[i]->GetShaderItem().m_pShader->GetFlags2() & EF2_DEPTHMAP_SUBSURFSCATTER)
      {
        return true;
      }
    }
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////
void CMatInfo::SetSketchMode( int mode )
{
	if (mode == 0)
	{
		if (m_pPreSketchShader)
		{
			m_shaderItem.m_pShader = m_pPreSketchShader;
			m_shaderItem.m_nTechnique = m_nPreSketchTechnique;
			m_pPreSketchShader = 0;
			m_nPreSketchTechnique = 0;
		}
	}
	else
	{
		if (m_shaderItem.m_pShader && m_shaderItem.m_pShader != m_pPreSketchShader)
		{
			EShaderType shaderType = m_shaderItem.m_pShader->GetShaderType();

//      nGenerationMask = (uint32)m_shaderItem.m_pShader->GetGenerationMask();
       
			// Do not replace this shader types.
			switch (shaderType)
			{
			case eST_Terrain:
			case eST_Shadow:
			case eST_Water:
			case eST_FX:
			case eST_PostProcess:
			case eST_HDR:
			case eST_Sky:
			case eST_Particle:
				// For this shaders do not replace them.
				return;
      case eST_Vegetation:
        {
          // in low spec mode also skip vegetation - we have low spec vegetation shader
          if (mode == 3)
            return;
        }
			}
		}

		if (!m_pPreSketchShader)
		{
			m_pPreSketchShader = m_shaderItem.m_pShader;
			m_nPreSketchTechnique = m_shaderItem.m_nTechnique;
		}

		//m_shaderItem.m_pShader = ((CMatMan*)GetMatMan())->GetDefaultHelperMaterial()->GetShaderItem().m_pShader;
		if (mode == 1)
		{
			m_shaderItem.m_pShader = gEnv->pRenderer->EF_LoadShader( "Sketch" );
			m_shaderItem.m_nTechnique = 0;
		}
		else if (mode == 2)
		{
			m_shaderItem.m_pShader = gEnv->pRenderer->EF_LoadShader( "Sketch.Fast" );
			m_shaderItem.m_nTechnique = 0;
		}
		else if (mode == 4)
		{
			SShaderItem tmp = gEnv->pRenderer->EF_LoadShaderItem("Sketch.TexelsPerMeter", false);
			m_shaderItem.m_pShader = tmp.m_pShader;
			m_shaderItem.m_nTechnique = tmp.m_nTechnique;
		}

		m_shaderItem.m_pShader->AddRef();
	}
	for (int i = 0; i < (int)m_subMtls.size(); i++)
	{
		if (m_subMtls[i])
		{
			m_subMtls[i]->SetSketchMode(mode);
		}	
	}
}

void CMatInfo::SetTexelDensityDebug( int mode )
{
	if (m_shaderItem.m_pShader)
	{
		EShaderType shaderType = m_pPreSketchShader ? m_pPreSketchShader->GetShaderType() : m_shaderItem.m_pShader->GetShaderType();

		switch (shaderType)
		{
			case eST_Terrain:
				if (mode == 3 || mode == 4)
				{
					if (m_nSurfaceTypeId == 0)
					{
						mode = 0;
					}

					break;
				}
			case eST_Shadow:
			case eST_Water:
			case eST_FX:
			case eST_PostProcess:
			case eST_HDR:
			case eST_Sky:
			case eST_Particle:
				// For this shaders do not replace them.
				mode = 0;
				break;
			default:
				if (mode == 1 || mode == 2)
				{
					break;
				}
				mode = 0;
				break;
		}

		if (mode == 0)
		{
			if (m_pPreSketchShader)
			{
				m_shaderItem.m_pShader = m_pPreSketchShader;
				m_shaderItem.m_nTechnique = m_nPreSketchTechnique;
				m_pPreSketchShader = 0;
				m_nPreSketchTechnique = 0;
			}
		}
		else
		{
			if (!m_pPreSketchShader)
			{
				m_pPreSketchShader = m_shaderItem.m_pShader;
				m_nPreSketchTechnique = m_shaderItem.m_nTechnique;
			}
			
			SShaderItem tmp;
			if (mode == 3 || mode == 4)
			{
				tmp = gEnv->pRenderer->EF_LoadShaderItem("SketchTerrain.TexelDensityTerrainLayer", false);
			}
			else
			{
				tmp = gEnv->pRenderer->EF_LoadShaderItem("Sketch.TexelDensity", false);
			}
			m_shaderItem.m_pShader = tmp.m_pShader;
			m_shaderItem.m_nTechnique = tmp.m_nTechnique;
		}
	}

	for (int i = 0; i < (int)m_subMtls.size(); i++)
	{
		if (m_subMtls[i])
		{
			m_subMtls[i]->SetTexelDensityDebug(mode);
		}	
	}
}

bool CMatInfo::IsPerObjectShadowPassNeeded()
{
	bool bPerObjectShadowPassNeeded = (m_Flags & MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED)!=0;

	if(!bPerObjectShadowPassNeeded)
	{
		for (int i = 0; i < (int)m_subMtls.size(); i++)
		{
			if(m_subMtls[i] != 0 && m_subMtls[i]->m_Flags & MTL_FLAG_PER_OBJECT_SHADOW_PASS_NEEDED)
			{
				bPerObjectShadowPassNeeded = true;
				break;
			}
		}
	}

	return bPerObjectShadowPassNeeded;
}

const char* CMatInfo::GetLoadingCallstack()
{
#ifdef TRACE_MATERIAL_LEAKS
	return m_sLoadingCallstack.c_str();
#else
	return "";
#endif
}

void CMatInfo::PrecacheTextures( const float fMipFactor, const int nFlags, const int nUpdateStreamingPrioriryRoundId )
{
	m_fMinMipFactor = min(m_fMinMipFactor,fMipFactor);
	m_bStartLoading |= nFlags & FPR_STARTLOADING ? true : false;

	if(m_nUpdateStreamingPrioriryRoundId != nUpdateStreamingPrioriryRoundId)
	{
		int nCurrentFlags = Get3DEngine()->IsShadersSyncLoad() ? FPR_SYNCRONOUS : 0;
		nCurrentFlags |= m_bStartLoading ? FPR_STARTLOADING : 0;

		if(IRenderShaderResources * pRes = GetShaderItem().m_pShaderResources)
		{
			for(int iSlot = 0;iSlot<EFTT_MAX;++iSlot)
			{
				if(SEfResTexture* pResTex = pRes->GetTexture(iSlot))
				{
					if(ITexture * pITex = pResTex->m_Sampler.m_pITex)
					{
						float fMinMipFactor = m_fMinMipFactor * min(fabsf(pResTex->m_TexModificator.m_Tiling[0]), fabsf(pResTex->m_TexModificator.m_Tiling[1]));
						GetRenderer()->EF_PrecacheResource(pITex, fMinMipFactor, 0.f, nCurrentFlags, nUpdateStreamingPrioriryRoundId);
					}
				}
			}
		}

		m_nUpdateStreamingPrioriryRoundId = nUpdateStreamingPrioriryRoundId;
		m_fMinMipFactor = fMipFactor;
		m_bStartLoading = false;
	}
}

void CMatInfo::PrecacheChunkTextures( const float fInstanceDistance, const int nFlags, const int nUpdateStreamingPrioriryRoundId, CRenderChunk *pRenderChunk )
{
  FUNCTION_PROFILER_3DENGINE;

	CMatInfo *pMaterial = this;

	if (pRenderChunk != NULL)
	{
		if (0 <= pRenderChunk->m_nMatID && pRenderChunk->m_nMatID < GetSubMtlCount())
		{
			pMaterial = (CMatInfo *)GetSubMtl(pRenderChunk->m_nMatID);
		}

		if (pMaterial != NULL)
		{
			float fMipFactor = fInstanceDistance*fInstanceDistance*pRenderChunk->m_texelAreaDensity;

			pMaterial->PrecacheTextures(fMipFactor,nFlags,nUpdateStreamingPrioriryRoundId);
		}
	}
	else
	{
		float fMipFactor = fInstanceDistance*fInstanceDistance;

		PrecacheTextures(fMipFactor,nFlags,nUpdateStreamingPrioriryRoundId);

    for(int nSubMtl = 0; nSubMtl < GetSubMtlCount(); ++nSubMtl)
		{
			pMaterial = (CMatInfo *)GetSubMtl(nSubMtl);

			if (pMaterial != NULL)
			{
				pMaterial->PrecacheTextures(fMipFactor,nFlags,nUpdateStreamingPrioriryRoundId);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
int CMatInfo::GetTextureMemoryUsage( ICrySizer *pSizer,int nSubMtlSlot )
{
	int textureSize = 0;
	std::set<ITexture*> used;

	int nSlotStart = 0;
	int nSlotEnd = (int)m_subMtls.size();

	if (nSubMtlSlot >= 0)
	{
		nSlotStart = nSubMtlSlot;
		nSlotEnd = nSubMtlSlot+1;
	}
	if (nSlotEnd >= (int)m_subMtls.size())
		nSlotEnd = (int)m_subMtls.size();

	if (nSlotEnd == 0)
		nSlotEnd = 1;

	for (int i = nSlotStart; i < nSlotEnd; i++ )
	{
		IRenderShaderResources *pRes = m_shaderItem.m_pShaderResources;
		if (i < (int)m_subMtls.size() && m_subMtls[i] != NULL && (m_Flags&MTL_FLAG_MULTI_SUBMTL))
		{
			SShaderItem shaderItem = m_subMtls[i]->m_shaderItem;
			if (!shaderItem.m_pShaderResources)
				continue;
			pRes = shaderItem.m_pShaderResources;
		}
		if (!pRes)
			continue;

		for (int j=0; j<EFTT_MAX; j++)
		{
			SEfResTexture *pResTexure = pRes->GetTexture(j);
			if (!pResTexure || !pResTexure->m_Sampler.m_pITex)
				continue;

			ITexture *pTexture = pResTexure->m_Sampler.m_pITex;
			if (!pTexture)
				continue;

			if (used.find(pTexture) != used.end()) // Already used in size calculation.
				continue;
			used.insert(pTexture);

			int nTexSize = pTexture->GetDataSize();
			textureSize += nTexSize;

			if (pSizer)
				pSizer->AddObject(pTexture, nTexSize);
		}
	}

	return textureSize;
}

#include UNIQUE_VIRTUAL_WRAPPER(IMaterial)
#include UNIQUE_VIRTUAL_WRAPPER(IMaterialLayer)
