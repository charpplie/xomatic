////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   MatMan.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Material Manager Implementation
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "MatMan.h"
#include "Material.h"
#include "3dEngine.h"
#include "ObjMan.h"
#include "IRenderer.h"
#include "SurfaceTypeManager.h"
#include "CGFContent.h"
#include <IResourceManager.h>

#define MATERIAL_EXT ".mtl"
#define MATERIAL_NODRAW "nodraw"

#define MATERIAL_DECALS_FOLDER "Materials/Decals"
#define MTL_LEVEL_CACHE_PAK "mtl.pak"

int CMatMan::e_sketch_mode = 0;
int CMatMan::e_pre_sketch_spec = 0;
int CMatMan::e_texeldensity = 0;

static bool defaultTexMod_Initialized = false;
static SEfTexModificator defaultTexMod;
static SInputShaderResources defaultShaderResource;

static const char *szReplaceMe="Shaders/EngineAssets/TextureMsg/ReplaceMe.tif";

static void OnSketchModeChange( ICVar *pVar )
{
	int mode = pVar->GetIVal();
	((CMatMan*)gEnv->p3DEngine->GetMaterialManager())->SetSketchMode( mode );
}

static void OnDebugTexelDensityChange( ICVar *pVar )
{
	int mode = pVar->GetIVal();
	((CMatMan*)gEnv->p3DEngine->GetMaterialManager())->SetTexelDensityDebug( mode );
}

//////////////////////////////////////////////////////////////////////////
CMatMan::CMatMan()
{
	m_bInitialized = false;
	m_pListener = NULL;
	m_pDefaultMtl = NULL;
	m_pDefaultTerrainLayersMtl = NULL;
  m_pDefaultLayersMtl = NULL;
	m_pDefaultHelperMtl = NULL;
	m_pNoDrawMtl = NULL;
	Cry3DEngineBase::m_pMaterialManager = this;

	m_pSurfaceTypeManager = new CSurfaceTypeManager(GetSystem());

	ICVar *pCVar = REGISTER_CVAR( e_sketch_mode,0,VF_CHEAT,"Enables Sketch mode drawing" );  
	if (pCVar)
	{
		pCVar->SetOnChangeCallback( &OnSketchModeChange );
		pCVar = REGISTER_CVAR( e_texeldensity,0,VF_CHEAT,
			"Enables texel density debug\n"
			" 1: Objects texel density\n"
			" 2: Objects texel density with colored mipmaps\n"
			" 3: Terrain texel density\n"
			" 4: Terrain texel density with colored mipmaps\n");
		pCVar->SetOnChangeCallback( &OnDebugTexelDensityChange );
	}

	m_pXmlParser = GetISystem()->GetXmlUtils()->CreateXmlParser();

	//InitDefaults();
}

//////////////////////////////////////////////////////////////////////////
CMatMan::~CMatMan()
{
	delete m_pSurfaceTypeManager;
	int nNotUsed=0, nNotUsedParents=0;
	m_pDefaultMtl = NULL;
	m_pDefaultTerrainLayersMtl = NULL;
  m_pDefaultLayersMtl = NULL;
	m_pDefaultHelperMtl = NULL;

	/*
  for (MtlSet::iterator it = m_mtlSet.begin(); it != m_mtlSet.end(); ++it)
  {
    IMaterial *pMtl = *it;
    SShaderItem Sh = pMtl->GetShaderItem();
		if(Sh.m_pShader)
		{
			Sh.m_pShader->Release();
			Sh.m_pShader = 0;
		}
    if(Sh.m_pShaderResources)
		{
			Sh.m_pShaderResources->Release();
			Sh.m_pShaderResources = 0;
		}
		pMtl->SetShaderItem( Sh );

		if(!(pMtl->GetFlags()&MIF_CHILD))
		if(!(pMtl->GetFlags()&MIF_WASUSED))
		{
			PrintMessage("Warning: CMatMan::~CMatMan: Material was loaded but never used: %s", pMtl->GetName());
			nNotUsed += (pMtl->GetSubMtlCount()+1);
			nNotUsedParents++;
		}
		if (pMtl->GetNumRefs() > 1)
		{
			//
			PrintMessage("Warning: CMatMan::~CMatMan: Material %s is being referenced", pMtl->GetName());
		}
  }
	*/

	if(nNotUsed)
		PrintMessage("Warning: CMatMan::~CMatMan: %d(%d) of %d materials was not used in level", 
		nNotUsedParents, nNotUsed, m_mtlNameMap.size());
}

//////////////////////////////////////////////////////////////////////////
const char* CMatMan::UnifyName( const char *sMtlName ) const
{
	static char name[260];
	int n = strlen(sMtlName);

	//TODO: change this code to not use a statically allocated buffer and return it ...
	//TODO: provide a general name unification function, which can be used in other places as well and remove this thing
	if (n>=260)
		Error("Static buffer size exceeded by material name!");

	for (int i = 0; i < n && i < sizeof(name); i++)
	{
		if (sMtlName[i] != '\\')
			name[i] = __ascii_tolower(sMtlName[i]);
		else
			name[i] = '/';
	}
	name[n] = '\0';
	return name;
}

//////////////////////////////////////////////////////////////////////////
IMaterial * CMatMan::CreateMaterial( const char *sMtlName,int nMtlFlags )
{
	CMatInfo *pMat = new CMatInfo;

	//m_mtlSet.insert( pMat );
	pMat->SetName( sMtlName );
	pMat->SetFlags( nMtlFlags | pMat->GetFlags() );
	if (!(nMtlFlags & MTL_FLAG_PURE_CHILD))
	{
		m_mtlNameMap[ UnifyName(sMtlName) ] = pMat;
	}

	if (nMtlFlags & MTL_FLAG_NON_REMOVABLE)
	{
		// Add reference to this material to prevent its deletion.
		m_nonRemovables.push_back( pMat );
	}

	return pMat;
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::NotifyCreateMaterial( IMaterial *pMtl )
{
	if (m_pListener)
		m_pListener->OnCreateMaterial( pMtl );
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::Unregister( CMatInfo * pMat )
{
	assert( pMat );

	if (!(pMat->m_Flags & MTL_FLAG_PURE_CHILD))
		m_mtlNameMap.erase( CONST_TEMP_STRING(UnifyName(pMat->GetName())) );

	if (m_pListener)
		m_pListener->OnDeleteMaterial(pMat);
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::RenameMaterial( IMaterial *pMtl,const char *sNewName )
{
	assert( pMtl );
	const char *sName = pMtl->GetName();
	if (*sName != '\0')
	{
		m_mtlNameMap.erase( CONST_TEMP_STRING(UnifyName(pMtl->GetName())) );
	}
	pMtl->SetName( sNewName );
	m_mtlNameMap[ UnifyName(sNewName) ] = pMtl;
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::FindMaterial( const char *sMtlName ) const
{
	const char *name = UnifyName(sMtlName);

	MtlNameMap::const_iterator it = m_mtlNameMap.find(CONST_TEMP_STRING(name));

	if(it==m_mtlNameMap.end())
		return 0;

	return it->second;
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::LoadMaterial( const char *sMtlName,bool bMakeIfNotFound, bool bNonremovable,unsigned long nLoadingFlags )
{
	if (!m_bInitialized)
		InitDefaults();

  if(m_pDefaultMtl && GetCVars()->e_StatObjPreload == 2)
    return m_pDefaultMtl;

	const char *name = UnifyName(sMtlName);

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_MTL, 0, "%s", name);

	MtlNameMap::const_iterator it = m_mtlNameMap.find(CONST_TEMP_STRING(name));

	IMaterial *pMtl = 0;

	if(it!=m_mtlNameMap.end())
	{
		pMtl = it->second;
		return pMtl;
	}

	LOADING_TIME_PROFILE_SECTION(GetSystem()); // Only profile actually loading of the material.

	if (!pMtl)
	{
		if (!(nLoadingFlags&ELoadingFlagsPreviewMode))
		{
			if (m_pListener)
			{
				pMtl = m_pListener->OnLoadMaterial( sMtlName,bMakeIfNotFound,nLoadingFlags );
				if (pMtl)
				{
					if (bNonremovable)
						m_nonRemovables.push_back(static_cast<CMatInfo*>(pMtl));

					if (pMtl && e_sketch_mode != 0)
					{
						((CMatInfo*)pMtl)->SetSketchMode(e_sketch_mode);
					}
					return pMtl;
				}
			}
		}

		// Try to load material from file.
		stack_string filename = name;
		if (filename.find('.') == stack_string::npos)
			filename += MATERIAL_EXT;

		static int nRecursionCounter = 0;

		bool bCanCleanPools = nRecursionCounter == 0; // Only clean pool, when we are called not recursively.
		nRecursionCounter++;

		XmlNodeRef mtlNode;
		if (m_pXmlParser)
		{
			// WARNING!!!
			// Shared XML parser does not support recursion!!!
			// On parsing a new XML file all previous XmlNodes are invalidated.
			mtlNode = m_pXmlParser->ParseFile( filename.c_str(),bCanCleanPools );
		}
		else
			mtlNode = GetSystem()->LoadXmlFile( filename.c_str() );

		if (mtlNode)
		{
			pMtl = MakeMaterialFromXml( name,mtlNode,false,0,0,nLoadingFlags );
			
			if (pMtl && e_sketch_mode != 0)
			{
				((CMatInfo*)pMtl)->SetSketchMode(e_sketch_mode);
			}
		}

		nRecursionCounter--;
	}
	if (!pMtl && bMakeIfNotFound)
	{
		pMtl = m_pDefaultMtl;
	}

	if (bNonremovable && pMtl)
		m_nonRemovables.push_back(static_cast<CMatInfo*>(pMtl));

	return pMtl;
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::LoadMaterialsLibrary( const char *sMtlFile,XmlNodeRef &levelDataRoot )
{
	PrintMessage("Loading materials ...");
	if (!m_bInitialized)
		InitDefaults();

	// load environment settings
	if (!levelDataRoot)
		return;
	
	XmlNodeRef mtlLibs = levelDataRoot->findChild( "MaterialsLibrary" );
	if (mtlLibs)
	{
		// Enumerate material libraries.
		for (int i = 0; i < mtlLibs->getChildCount(); i++)
		{
			XmlNodeRef mtlLib = mtlLibs->getChild(i);
			XmlString libraryName = mtlLib->getAttr( "Name" );
			for (int j =0; j < mtlLib->getChildCount(); j++)
			{
				XmlNodeRef mtlNode = mtlLib->getChild(j);
				OldLoadMaterialFromXml( mtlNode,libraryName.c_str() );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Loading from external materials.xml file.
	//////////////////////////////////////////////////////////////////////////
	mtlLibs = GetSystem()->LoadXmlFile( sMtlFile );
	if (mtlLibs)
	{
		// Enumerate material libraries.
		for (int i = 0; i < mtlLibs->getChildCount(); i++)
		{
			XmlNodeRef mtlLib = mtlLibs->getChild(i);
			if (!mtlLib->isTag("Library"))
				continue;
			XmlString libraryName = mtlLib->getAttr( "Name" );
			for (int j =0; j < mtlLib->getChildCount(); j++)
			{
				XmlNodeRef mtlNode = mtlLib->getChild(j);
				OldLoadMaterialFromXml( mtlNode,libraryName.c_str() );
			}
		}
	}

  PrintMessage(" %d mats loaded", m_mtlNameMap.size());
}

static struct
{
	int texId;
	const char *name;
} sUsedTextures[] =
{
	{ EFTT_DIFFUSE,		"Diffuse" },
	{ EFTT_GLOSS,			"Specular" },
	{ EFTT_BUMP,			"Bumpmap" },
	{ EFTT_NORMALMAP,	"Normalmap" },
	{ EFTT_ENV,		    "Environment" },
	{ EFTT_DETAIL_OVERLAY,"Detail" },
	{ EFTT_OPACITY,		"Opacity" },
	{ EFTT_DECAL_OVERLAY,	"Decal" },
  { EFTT_SUBSURFACE,	"SubSurface" },
  { EFTT_CUSTOM,	"Custom" },
  { EFTT_CUSTOM_SECONDARY,	"[1] Custom" },
};
inline ColorF ToCFColor( const Vec3 &col ) { return ColorF(col); }

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::OldLoadMaterialFromXml( XmlNodeRef node,const char *sLibraryName,bool bPureChild )
{
	XmlString name,shaderName,texmap,surfaceType,file,matTemplate;
	int mtlFlags = 0;
	uint64 nShaderGenMask = 0;	
	SInputShaderResources sr;

	// Make new mat info.
	name = node->getAttr( "Name" );

	if (bPureChild)
	{
		name += " <SubMtl>";
	}

	IMaterial* pMtl = NULL;
	if (!bPureChild)
		pMtl = CreateMaterial( name.c_str() );
	else
	{
		pMtl = new CMatInfo();
		pMtl->SetName(name);
	}

	// Loading from Material XML node.
	shaderName = node->getAttr( "Shader" );
	if (shaderName.empty())
	{
		// Replace empty shader with NoDraw material.
		shaderName = "NoDraw";
	}

	node->getAttr( "MtlFlags",mtlFlags );
	mtlFlags &= (MTL_FLAGS_SAVE_MASK); // Clear all flags that need not to be saved.
	mtlFlags |= pMtl->GetFlags();

	if( !( mtlFlags&MTL_64BIT_SHADERGENMASK) )
	{
		uint32 nShaderGenMask32 = 0;
		node->getAttr( "GenMask",nShaderGenMask32 );
		nShaderGenMask = nShaderGenMask32;

		// Remap 32bit flags to 64 bit version
		nShaderGenMask = GetRenderer()->EF_GetRemapedShaderMaskGen( (const char*) shaderName, nShaderGenMask ); 
		mtlFlags |= MTL_64BIT_SHADERGENMASK;
	}
	else
		node->getAttr( "GenMask",nShaderGenMask );

  if( node->haveAttr( "StringGenMask" ) )         
  {      
    const char *pszShaderGenMask = node->getAttr( "StringGenMask" );
    nShaderGenMask = GetRenderer()->EF_GetShaderGlobalMaskGenFromString( (const char*) shaderName, pszShaderGenMask, nShaderGenMask);  // get common mask gen
  }
  else
  {
    // version doens't has string gen mask yet ? Remap flags if needed
    nShaderGenMask = GetRenderer()->EF_GetRemapedShaderMaskGen( (const char*) shaderName, nShaderGenMask, ((mtlFlags & MTL_64BIT_SHADERGENMASK)!=0) ); 
  }

  mtlFlags |= MTL_64BIT_SHADERGENMASK;

		node->getAttr( "SurfaceType",surfaceType );
	pMtl->SetSurfaceType( surfaceType );

  node->getAttr( "MatTemplate",matTemplate);
  pMtl->SetMatTemplate( matTemplate );

  if (stricmp(shaderName,"nodraw") == 0)
		mtlFlags |= MTL_FLAG_NODRAW;

	pMtl->SetFlags(mtlFlags);

	// Load lighting data.
	Vec3 diffuse,specular,emissive;
	node->getAttr( "Diffuse",diffuse );
	node->getAttr( "Specular",specular );
	node->getAttr( "Emissive",emissive );
	node->getAttr( "Shininess",sr.m_LMaterial.m_SpecShininess );
	sr.m_LMaterial.m_Diffuse = ToCFColor(diffuse);
	sr.m_LMaterial.m_Specular = ToCFColor(specular);
	sr.m_LMaterial.m_Emission = ToCFColor(emissive);

	node->getAttr( "Opacity", sr.m_Opacity );
	node->getAttr( "AlphaTest", sr.m_AlphaRef );
  node->getAttr( "GlowAmount", sr.m_GlowAmount );
  node->getAttr( "FurAmount", sr.m_FurAmount );
  node->getAttr( "PostEffectType", sr.m_PostEffects );

	// Load material textures.
	XmlNodeRef texturesNode = node->findChild( "Textures" );
	if (texturesNode)
	{
		for (int i = 0; i < texturesNode->getChildCount(); i++)
		{
			texmap = "";
			XmlNodeRef texNode = texturesNode->getChild(i);
			texmap = texNode->getAttr( "Map" );

			int texId = -1;
			for (int j = 0; j < sizeof(sUsedTextures)/sizeof(sUsedTextures[0]); j++)
			{
				if (stricmp(sUsedTextures[j].name,texmap) == 0)
				{
					texId = sUsedTextures[j].texId;
					break;
				}
			}
			if (texId < 0)
				continue;

			// Correct texid found.

			const char *filename = texNode->getAttr( "File" );
			
			if (texId == EFTT_NORMALMAP)
			{
				// We are not using NORMALMAP slot anymore
				texId = EFTT_BUMP;
				if (strlen(filename) == 0)
					filename = sr.m_Textures[EFTT_BUMP].m_Name.c_str();
			}

			sr.m_Textures[texId].m_Name = filename;

			texNode->getAttr( "Amount",sr.m_Textures[texId].m_Amount );
			texNode->getAttr( "IsTileU",sr.m_Textures[texId].m_bUTile );
			texNode->getAttr( "IsTileV",sr.m_Textures[texId].m_bVTile );
      texNode->getAttr( "TexType",sr.m_Textures[texId].m_Sampler.m_eTexType );

			XmlNodeRef modNode = texNode->findChild( "TexMod" );
			if (modNode)
			{
				SEfTexModificator &texm = sr.m_Textures[texId].m_TexModificator;

				// Modificators
				modNode->getAttr( "TileU",texm.m_Tiling[0] );
				modNode->getAttr( "TileV",texm.m_Tiling[1] );
				modNode->getAttr( "OffsetU",texm.m_Offs[0] );
				modNode->getAttr( "OffsetV",texm.m_Offs[1] );
				modNode->getAttr( "TexType",sr.m_Textures[texId].m_Sampler.m_eTexType );

				float f;
				modNode->getAttr( "TexMod_bTexGenProjected",texm.m_bTexGenProjected );
				modNode->getAttr( "TexMod_UOscillatorType",texm.m_eUMoveType );
				modNode->getAttr( "TexMod_VOscillatorType",texm.m_eVMoveType );
				modNode->getAttr( "TexMod_RotateType",texm.m_eRotType );
				modNode->getAttr( "TexMod_TexGenType",texm.m_eTGType );

				if (modNode->getAttr( "RotateU",f ))
				  texm.m_Rot[0] = Degr2Word(f);
				if (modNode->getAttr( "RotateV",f ))
				  texm.m_Rot[1] = Degr2Word(f);
				if (modNode->getAttr( "RotateW",f ))
				  texm.m_Rot[2] = Degr2Word(f);
				if (modNode->getAttr( "TexMod_URotateRate",f ))
				  texm.m_RotOscRate[0] = Degr2Word(f);
				if (modNode->getAttr( "TexMod_VRotateRate",f ))
				  texm.m_RotOscRate[1] = Degr2Word(f);
				if (modNode->getAttr( "TexMod_WRotateRate",f ))
				  texm.m_RotOscRate[2] = Degr2Word(f);
				if (modNode->getAttr( "TexMod_URotatePhase",f ))
				  texm.m_RotOscPhase[0] = Degr2Word(f);
				if (modNode->getAttr( "TexMod_VRotatePhase",f ))
				  texm.m_RotOscPhase[1] = Degr2Word(f);
				if (modNode->getAttr( "TexMod_WRotatePhase",f ))
			  	texm.m_RotOscPhase[2] = Degr2Word(f);
				if (modNode->getAttr( "TexMod_URotateAmplitude",f ))
		  		texm.m_RotOscAmplitude[0] = Degr2Word(f);
				if (modNode->getAttr( "TexMod_VRotateAmplitude",f ))
	  			texm.m_RotOscAmplitude[1] = Degr2Word(f);
				if (modNode->getAttr( "TexMod_WRotateAmplitude",f ))
  				texm.m_RotOscAmplitude[2] = Degr2Word(f);
				modNode->getAttr( "TexMod_URotateCenter",texm.m_RotOscCenter[0] );
        modNode->getAttr( "TexMod_VRotateCenter",texm.m_RotOscCenter[1] );
        modNode->getAttr( "TexMod_WRotateCenter",texm.m_RotOscCenter[2] );

				modNode->getAttr( "TexMod_UOscillatorRate",texm.m_UOscRate );
				modNode->getAttr( "TexMod_VOscillatorRate",texm.m_VOscRate );
				modNode->getAttr( "TexMod_UOscillatorPhase",texm.m_UOscPhase );
				modNode->getAttr( "TexMod_VOscillatorPhase",texm.m_VOscPhase );
				modNode->getAttr( "TexMod_UOscillatorAmplitude",texm.m_UOscAmplitude );
				modNode->getAttr( "TexMod_VOscillatorAmplitude",texm.m_VOscAmplitude );
			}
    }
	}

	// Load sub materials.
	XmlNodeRef childsNode = node->findChild( "SubMaterials" );
	if (childsNode)
	{
		mtlFlags |= MTL_FLAG_MULTI_SUBMTL;
		pMtl->SetFlags(mtlFlags);
		
		int nSubSlots = childsNode->getChildCount();
		pMtl->SetSubMtlCount(nSubSlots);
		for (int i = 0; i < nSubSlots; i++)
		{
			XmlNodeRef mtlNode = childsNode->getChild(i);
			if (mtlNode->isTag("MaterialRef"))
			{
				XmlString subname;
				if (mtlNode->getAttr("Name",subname))
				{
					IMaterial *pChildMtl = LoadMaterial( subname );
					pMtl->SetSubMtl(i,pChildMtl);
				}
			}
			else
			{
				IMaterial *pChildMtl = OldLoadMaterialFromXml( mtlNode,sLibraryName );
				pMtl->SetSubMtl(i,pChildMtl);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Load public parameters.
	//////////////////////////////////////////////////////////////////////////
	XmlNodeRef publicsNode = node->findChild( "PublicParams" );

	//////////////////////////////////////////////////////////////////////////
	// Reload shader item with new resources and shader.
	//////////////////////////////////////////////////////////////////////////
	LoadMaterialShader( pMtl,shaderName.c_str(),nShaderGenMask,sr,publicsNode );

  //////////////////////////////////////////////////////////////////////////
  // Load material layers data
  //////////////////////////////////////////////////////////////////////////
  
  XmlNodeRef pMtlLayersNode = node->findChild( "MaterialLayers" ); 
  if( pMtlLayersNode )
  {
    int nLayerCount = min( (int) MTL_LAYER_MAX_SLOTS, (int) pMtlLayersNode->getChildCount());
    if( nLayerCount)
    {
      uint8 nMaterialLayerFlags = 0;

      pMtl->SetLayerCount( nLayerCount );
      for (int l(0); l < nLayerCount; ++l)
      {
        XmlNodeRef pLayerNode = pMtlLayersNode->getChild(l);
        if(pLayerNode)
        {
          XmlString pszShaderName;
          if( pLayerNode->getAttr( "Name", pszShaderName) )
          {
            bool bNoDraw = false;
            pLayerNode->getAttr( "NoDraw", bNoDraw);

            uint8 nLayerFlags = 0;
            if ( bNoDraw )
            {
              nLayerFlags |= MTL_LAYER_USAGE_NODRAW;

              if( !strcmpi(pszShaderName.c_str(), "frozenlayerwip")) 
                nMaterialLayerFlags |= MTL_LAYER_FROZEN ;
              else
              if( !strcmpi(pszShaderName.c_str(), "cloaklayer")) 
                nMaterialLayerFlags |= MTL_LAYER_CLOAK;
            }
            else
              nLayerFlags &= ~MTL_LAYER_USAGE_NODRAW;

            XmlNodeRef pPublicsParamsNode = pLayerNode->findChild( "PublicParams" );                    
            LoadMaterialLayerSlot( l, pMtl, pszShaderName.c_str(), sr, pPublicsParamsNode, nLayerFlags); 
          }
        }
      }

      SShaderItem pShaderItemBase = pMtl->GetShaderItem();  
      if( pShaderItemBase.m_pShaderResources )
        pShaderItemBase.m_pShaderResources->SetMtlLayerNoDrawFlags( nMaterialLayerFlags );
    }
  }

	return pMtl;
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::MakeMaterialFromXml( const char *sMtlName,XmlNodeRef node,bool bForcePureChild, uint16 sortPrio,IMaterial *pExistingMtl,unsigned long nLoadingFlags )
{
	int mtlFlags = 0;
	CryFixedStringT<128> shaderName;
	uint64 nShaderGenMask = 0;
	SInputShaderResources sr;

	sr.m_SortPrio = sortPrio;

	// Loading
	node->getAttr( "MtlFlags",mtlFlags );
	if (bForcePureChild)
		mtlFlags |= MTL_FLAG_PURE_CHILD;

	IMaterial *pMtl = pExistingMtl;
	if (!pMtl)
	{
		pMtl = CreateMaterial( sMtlName,mtlFlags );
	}
	else
	{
		mtlFlags &= (MTL_FLAGS_SAVE_MASK); // Clear all flags that need not to be saved.
		pMtl->SetFlags( mtlFlags | pMtl->GetFlags() );
	}

  const char *matTemplate = NULL;

	if (!(mtlFlags & MTL_FLAG_MULTI_SUBMTL))
	{ 
	    matTemplate = node->getAttr( "MatTemplate" );
    	pMtl->SetMatTemplate( matTemplate );
	    //
		shaderName = node->getAttr( "Shader" );

		if( !( mtlFlags&MTL_64BIT_SHADERGENMASK) )
		{
			uint32 nShaderGenMask32 = 0;
			node->getAttr( "GenMask",nShaderGenMask32 );
			nShaderGenMask = nShaderGenMask32;

			// Remap 32bit flags to 64 bit version
			nShaderGenMask = GetRenderer()->EF_GetRemapedShaderMaskGen( (const char*) shaderName, nShaderGenMask ); 
			mtlFlags |= MTL_64BIT_SHADERGENMASK;
		}
		else
			node->getAttr( "GenMask",nShaderGenMask );

    if( node->haveAttr( "StringGenMask" ) )         
    {      
      const char *pszShaderGenMask = node->getAttr( "StringGenMask" );
      nShaderGenMask = GetRenderer()->EF_GetShaderGlobalMaskGenFromString( (const char*) shaderName, pszShaderGenMask, nShaderGenMask);  // get common mask gen
    }
    else
    {
      // version doens't has string gen mask yet ? Remap flags if needed
      nShaderGenMask = GetRenderer()->EF_GetRemapedShaderMaskGen( (const char*) shaderName, nShaderGenMask, ((mtlFlags & MTL_64BIT_SHADERGENMASK)!=0) ); 
    }
    mtlFlags |= MTL_64BIT_SHADERGENMASK;


		const char *surfaceType = node->getAttr( "SurfaceType" );
		pMtl->SetSurfaceType( surfaceType );

    if (stricmp(shaderName,"nodraw") == 0)
      mtlFlags |= MTL_FLAG_NODRAW;

    // Load lighting data.
    Vec3 vColor;
    if (node->getAttr( "Diffuse",vColor ))
      sr.m_LMaterial.m_Diffuse = ToCFColor(vColor);
    if (node->getAttr( "Specular",vColor ))
      sr.m_LMaterial.m_Specular = ToCFColor(vColor);
    if (node->getAttr( "Emissive",vColor ))
      sr.m_LMaterial.m_Emission = ToCFColor(vColor);
    node->getAttr( "Shininess",sr.m_LMaterial.m_SpecShininess );
    node->getAttr( "Opacity",sr.m_Opacity );
    node->getAttr( "AlphaTest",sr.m_AlphaRef );
    node->getAttr( "GlowAmount",sr.m_GlowAmount );
    node->getAttr( "FurAmount",sr.m_FurAmount );
    node->getAttr( "PostEffectType", sr.m_PostEffects );
    //
		const char* texmap = "";
		const char* file = "";
		// 
		XmlNodeRef texturesNode = node->findChild( "Textures" );
		if (texturesNode)
		{
			for (int i = 0; i < texturesNode->getChildCount(); i++)
			{
				XmlNodeRef texNode = texturesNode->getChild(i);
				texmap = texNode->getAttr( "Map" );

				int texId = -1;
				for (int j = 0; j < sizeof(sUsedTextures)/sizeof(sUsedTextures[0]); j++)
				{
					if (stricmp(sUsedTextures[j].name,texmap) == 0)
					{
						texId = sUsedTextures[j].texId;
						break;
					}
				}
				if (texId < 0)
					continue;

				file = texNode->getAttr( "File" );

				// Correct texid found.
				sr.m_Textures[texId].m_Name = file;
				texNode->getAttr( "Amount",sr.m_Textures[texId].m_Amount );
				texNode->getAttr( "IsTileU",sr.m_Textures[texId].m_bUTile );
				texNode->getAttr( "IsTileV",sr.m_Textures[texId].m_bVTile );
				texNode->getAttr( "TexType",sr.m_Textures[texId].m_Sampler.m_eTexType );

				int filter = sr.m_Textures[texId].m_Filter;
				if (texNode->getAttr( "Filter",filter ))
					sr.m_Textures[texId].m_Filter = filter;

				XmlNodeRef modNode = texNode->findChild( "TexMod" );
				if (modNode)
				{
					SEfTexModificator &texm = sr.m_Textures[texId].m_TexModificator;

					// Modificators
					modNode->getAttr( "TileU",texm.m_Tiling[0] );
					modNode->getAttr( "TileV",texm.m_Tiling[1] );
					modNode->getAttr( "OffsetU",texm.m_Offs[0] );
					modNode->getAttr( "OffsetV",texm.m_Offs[1] );

					float f;
					modNode->getAttr( "TexMod_bTexGenProjected",texm.m_bTexGenProjected );
					modNode->getAttr( "TexMod_UOscillatorType",texm.m_eUMoveType );
					modNode->getAttr( "TexMod_VOscillatorType",texm.m_eVMoveType );
					modNode->getAttr( "TexMod_RotateType",texm.m_eRotType );
					modNode->getAttr( "TexMod_TexGenType",texm.m_eTGType );

					if (modNode->getAttr( "RotateU",f ))
						texm.m_Rot[0] = Degr2Word(f);
					if (modNode->getAttr( "RotateV",f ))
						texm.m_Rot[1] = Degr2Word(f);
					if (modNode->getAttr( "RotateW",f ))
						texm.m_Rot[2] = Degr2Word(f);
					if (modNode->getAttr( "TexMod_URotateRate",f ))
						texm.m_RotOscRate[0] = Degr2Word(f);
					if (modNode->getAttr( "TexMod_VRotateRate",f ))
						texm.m_RotOscRate[1] = Degr2Word(f);
					if (modNode->getAttr( "TexMod_WRotateRate",f ))
						texm.m_RotOscRate[2] = Degr2Word(f);
					if (modNode->getAttr( "TexMod_URotatePhase",f ))
						texm.m_RotOscPhase[0] = Degr2Word(f);
					if (modNode->getAttr( "TexMod_VRotatePhase",f ))
						texm.m_RotOscPhase[1] = Degr2Word(f);
					if (modNode->getAttr( "TexMod_WRotatePhase",f ))
						texm.m_RotOscPhase[2] = Degr2Word(f);
					if (modNode->getAttr( "TexMod_URotateAmplitude",f ))
						texm.m_RotOscAmplitude[0] = Degr2Word(f);
					if (modNode->getAttr( "TexMod_VRotateAmplitude",f ))
						texm.m_RotOscAmplitude[1] = Degr2Word(f);
					if (modNode->getAttr( "TexMod_WRotateAmplitude",f ))
						texm.m_RotOscAmplitude[2] = Degr2Word(f);
					modNode->getAttr( "TexMod_URotateCenter",texm.m_RotOscCenter[0] );
					modNode->getAttr( "TexMod_VRotateCenter",texm.m_RotOscCenter[1] );
					modNode->getAttr( "TexMod_WRotateCenter",texm.m_RotOscCenter[2] );

					modNode->getAttr( "TexMod_UOscillatorRate",texm.m_UOscRate );
					modNode->getAttr( "TexMod_VOscillatorRate",texm.m_VOscRate );
					modNode->getAttr( "TexMod_UOscillatorPhase",texm.m_UOscPhase );
					modNode->getAttr( "TexMod_VOscillatorPhase",texm.m_VOscPhase );
					modNode->getAttr( "TexMod_UOscillatorAmplitude",texm.m_UOscAmplitude );
					modNode->getAttr( "TexMod_VOscillatorAmplitude",texm.m_VOscAmplitude );
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Check if we have vertex deform.
	//////////////////////////////////////////////////////////////////////////
	XmlNodeRef deformNode = node->findChild("VertexDeform");
	if (deformNode)
	{
		int deform_type = eDT_Unknown;
		deformNode->getAttr( "Type",deform_type );
		sr.m_DeformInfo.m_eType = (EDeformType)deform_type;
		deformNode->getAttr( "DividerX",sr.m_DeformInfo.m_fDividerX );
		deformNode->getAttr( "DividerY",sr.m_DeformInfo.m_fDividerY );
		deformNode->getAttr( "NoiseScale",sr.m_DeformInfo.m_vNoiseScale );

		XmlNodeRef waveX = deformNode->findChild("WaveX");
		if (waveX)
		{
			int type = eWF_None;
			waveX->getAttr( "Type",type );
			sr.m_DeformInfo.m_WaveX.m_eWFType = (EWaveForm)type;
			waveX->getAttr( "Amp",sr.m_DeformInfo.m_WaveX.m_Amp );
			waveX->getAttr( "Level",sr.m_DeformInfo.m_WaveX.m_Level );
			waveX->getAttr( "Phase",sr.m_DeformInfo.m_WaveX.m_Phase );
			waveX->getAttr( "Freq",sr.m_DeformInfo.m_WaveX.m_Freq );
		}
		XmlNodeRef waveY = deformNode->findChild("WaveY");
		if (waveY)
		{
			int type = eWF_None;
			waveY->getAttr( "Type",type );
			sr.m_DeformInfo.m_WaveY.m_eWFType = (EWaveForm)type;
			waveY->getAttr( "Amp",sr.m_DeformInfo.m_WaveY.m_Amp );
			waveY->getAttr( "Level",sr.m_DeformInfo.m_WaveY.m_Level );
			waveY->getAttr( "Phase",sr.m_DeformInfo.m_WaveY.m_Phase );
			waveY->getAttr( "Freq",sr.m_DeformInfo.m_WaveY.m_Freq );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Check for detail decal
	//////////////////////////////////////////////////////////////////////////
	XmlNodeRef detailDecalNode = node->findChild("DetailDecal");
	if( detailDecalNode && (mtlFlags&MTL_FLAG_DETAIL_DECAL) )
	{
		SDetailDecalInfo *pDetailDecalInfo = &sr.m_DetailDecalInfo;
		detailDecalNode->getAttr( "Opacity", pDetailDecalInfo->nBlending);
		detailDecalNode->getAttr( "SSAOAmount", pDetailDecalInfo->nSSAOAmount);      
		detailDecalNode->getAttr( "topTileU", pDetailDecalInfo->vTileOffs[0].x);
		detailDecalNode->getAttr( "topTileV", pDetailDecalInfo->vTileOffs[0].y);
		detailDecalNode->getAttr( "topOffsV", pDetailDecalInfo->vTileOffs[0].z);
		detailDecalNode->getAttr( "topOffsU", pDetailDecalInfo->vTileOffs[0].w);
		detailDecalNode->getAttr( "topRotation", pDetailDecalInfo->nRotation[0]);
		detailDecalNode->getAttr( "topDeformation", pDetailDecalInfo->nDeformation[0]);
		detailDecalNode->getAttr( "topSortOffset", pDetailDecalInfo->nThreshold[0]);
		detailDecalNode->getAttr( "bottomTileU", pDetailDecalInfo->vTileOffs[1].x);
		detailDecalNode->getAttr( "bottomTileV", pDetailDecalInfo->vTileOffs[1].y);
		detailDecalNode->getAttr( "bottomOffsV", pDetailDecalInfo->vTileOffs[1].z);
		detailDecalNode->getAttr( "bottomOffsU", pDetailDecalInfo->vTileOffs[1].w);
		detailDecalNode->getAttr( "bottomRotation", pDetailDecalInfo->nRotation[1]);
		detailDecalNode->getAttr( "bottomDeformation", pDetailDecalInfo->nDeformation[1]);
		detailDecalNode->getAttr( "bottomSortOffset", pDetailDecalInfo->nThreshold[1]);
	}
	else
		sr.m_DetailDecalInfo.Reset();

	//////////////////////////////////////////////////////////////////////////
	// Load public parameters.
	XmlNodeRef publicVarsNode = node->findChild( "PublicParams" );

	//////////////////////////////////////////////////////////////////////////
	// Reload shader item with new resources and shader.
	if (!(mtlFlags & MTL_FLAG_MULTI_SUBMTL))
	{
    sr.m_szMaterialName = sMtlName;
		LoadMaterialShader( pMtl,shaderName.c_str(),nShaderGenMask,sr,publicVarsNode,nLoadingFlags );
	}

  //////////////////////////////////////////////////////////////////////////
  // Load material layers data
  //////////////////////////////////////////////////////////////////////////

  if( pMtl && pMtl->GetShaderItem().m_pShader && pMtl->GetShaderItem().m_pShaderResources )
  {
    XmlNodeRef pMtlLayersNode = node->findChild( "MaterialLayers" ); 
    if( pMtlLayersNode )
    {
      int nLayerCount = min( (int) MTL_LAYER_MAX_SLOTS, (int) pMtlLayersNode->getChildCount());
      if( nLayerCount)
      {
        uint8 nMaterialLayerFlags = 0;

        pMtl->SetLayerCount( nLayerCount );
        for (int l(0); l < nLayerCount; ++l)
        {
          XmlNodeRef pLayerNode = pMtlLayersNode->getChild(l);
          if(pLayerNode)
          {
            if( const char * pszShaderName = pLayerNode->getAttr( "Name" ) )
            {
              bool bNoDraw = false;
              pLayerNode->getAttr( "NoDraw", bNoDraw);                    

              uint8 nLayerFlags = 0;
              if ( bNoDraw )
              {
                nLayerFlags |= MTL_LAYER_USAGE_NODRAW;

                if( !strcmpi(pszShaderName, "frozenlayerwip")) 
                  nMaterialLayerFlags |= MTL_LAYER_FROZEN ;
                else
                if( !strcmpi(pszShaderName, "cloaklayer")) 
                  nMaterialLayerFlags |= MTL_LAYER_CLOAK;
              }
              else
                nLayerFlags &= ~MTL_LAYER_USAGE_NODRAW;
              
              XmlNodeRef pPublicsParamsNode = pLayerNode->findChild( "PublicParams" );
              sr.m_szMaterialName = sMtlName;
              LoadMaterialLayerSlot( l, pMtl, pszShaderName, sr, pPublicsParamsNode, nLayerFlags); 
            }
          }
        }

        SShaderItem pShaderItemBase = pMtl->GetShaderItem();  
        if( pShaderItemBase.m_pShaderResources )
          pShaderItemBase.m_pShaderResources->SetMtlLayerNoDrawFlags( nMaterialLayerFlags );
      }
    }
  }
	
	// Serialize sub materials.
	XmlNodeRef childsNode = node->findChild( "SubMaterials" );
	if (childsNode)
	{
		int nSubMtls = childsNode->getChildCount();
		pMtl->SetSubMtlCount(nSubMtls);
		for (int i = 0; i < nSubMtls; i++)
		{
			XmlNodeRef mtlNode = childsNode->getChild(i);
			if (mtlNode->isTag("Material"))
			{
				const char *name = mtlNode->getAttr("Name");
				IMaterial *pChildMtl = MakeMaterialFromXml( name,mtlNode,true,nSubMtls - i - 1,0,nLoadingFlags );
				if (pChildMtl)
					pMtl->SetSubMtl(i,pChildMtl);
				else
					pMtl->SetSubMtl(i,m_pDefaultMtl);
			}
			else
			{
				const char *name = mtlNode->getAttr("Name");
				if (name[0])
				{
					IMaterial *pChildMtl = LoadMaterial(name,true,false,nLoadingFlags);
					if (pChildMtl)
						pMtl->SetSubMtl(i,pChildMtl);
				}
			}
		}
	}
  //
  if( matTemplate!=NULL && strlen(matTemplate)!=0 && strcmp(matTemplate,sMtlName)!=0 )
  {
    CMatInfo* pMtlTmpl=NULL;
    pMtlTmpl = static_cast<CMatInfo*>(gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(matTemplate,false)); 
    if( pMtlTmpl )
    {
      pMtlTmpl->Copy(static_cast<CMatInfo*>(pMtl),MTL_COPY_DEFAULT);
      return pMtl;
    }
  }
  //
	return pMtl;
}

//////////////////////////////////////////////////////////////////////////
bool CMatMan::LoadMaterialShader( IMaterial *pMtl,const char *sShader,uint64 nShaderGenMask,SInputShaderResources &sr,XmlNodeRef &publicsNode,unsigned long nLoadingFlags)
{
	// Mark material invalid by default.
	int mtlFlags = pMtl->GetFlags();

	sr.m_ResFlags = pMtl->GetFlags();

	SShaderItem shaderItem = gEnv->pRenderer->EF_LoadShaderItem(sShader,false,0,&sr,nShaderGenMask);
	if (!shaderItem.m_pShader || (shaderItem.m_pShader->GetFlags() & EF_NOTFOUND) != 0)
	{
		Warning("Failed to load shader \"%s\" in material \"%s\"", sShader, pMtl->GetName());
		if (!shaderItem.m_pShader)
			return false;
	}
	pMtl->AssignShaderItem( shaderItem );

	// Set public params.
	if (publicsNode)
	{
		// Copy public params from the shader.
		sr.m_ShaderParams = shaderItem.m_pShader->GetPublicParams();
		// Parse public parameters, and assign them to source shader resources.
		ParsePublicParams( sr, publicsNode );
		shaderItem.m_pShaderResources->SetShaderParams(&sr, shaderItem.m_pShader);
	}
	
	return true;
}

bool CMatMan::LoadMaterialLayerSlot( uint32 nSlot, IMaterial *pMtl, const char *szShaderName, SInputShaderResources &pBaseResources, XmlNodeRef &pPublicsNode, uint8 nLayerFlags)
{
  if( !pMtl || pMtl->GetLayer( nSlot ) || !pPublicsNode)
  {
    return false;
  }

  // need to handle no draw case
  if (stricmp(szShaderName,"nodraw") == 0)
  {          
    // no shader = skip layer
    return false;
  }

  // Get base material/shaderItem info
  SInputShaderResources pInputResources;
  SShaderItem pShaderItemBase = pMtl->GetShaderItem();  

  uint32 nMaskGenBase = (uint32)pShaderItemBase.m_pShader->GetGenerationMask();  
  SShaderGen *pShaderGenBase = pShaderItemBase.m_pShader->GetGenerationParams();

  // copy diffuse and bump textures names  

  pInputResources.m_szMaterialName = pBaseResources.m_szMaterialName;
  pInputResources.m_Textures[EFTT_DIFFUSE].m_Name = pBaseResources.m_Textures[EFTT_DIFFUSE].m_Name;
  pInputResources.m_Textures[EFTT_BUMP].m_Name = pBaseResources.m_Textures[EFTT_BUMP].m_Name;
  
  // Check if names are valid - else replace with default textures

  if( pInputResources.m_Textures[EFTT_DIFFUSE].m_Name.empty() )
  {
    pInputResources.m_Textures[EFTT_DIFFUSE].m_Name = szReplaceMe;
//		pInputResources.m_Textures[EFTT_DIFFUSE].m_Name = "<default>";
  }

  if( pInputResources.m_Textures[EFTT_BUMP].m_Name.empty() )
  {
    pInputResources.m_Textures[EFTT_BUMP].m_Name = "Textures/defaults/white_ddn.dds";
  }

  // Load layer shader item
  IShader *pNewShader = gEnv->pRenderer->EF_LoadShader( szShaderName, 0);
  if (!pNewShader ) 
  {
    Warning( "Failed to load material layer shader %s in Material %s", szShaderName, pMtl->GetName() );
    return false;
  }
    
  // mask generation for base material shader
  uint32 nMaskGenLayer = 0;  
  SShaderGen *pShaderGenLayer = pNewShader->GetGenerationParams();    
  if (pShaderGenBase && pShaderGenLayer)
  {                        
    for (unsigned nLayerBit(0); nLayerBit < pShaderGenLayer->m_BitMask.size(); ++nLayerBit)
    {
      SShaderGenBit *pLayerBit = pShaderGenLayer->m_BitMask[nLayerBit];

      for (unsigned nBaseBit(0); nBaseBit < pShaderGenBase->m_BitMask.size(); ++nBaseBit)
      {
        SShaderGenBit *pBaseBit = pShaderGenBase->m_BitMask[nBaseBit];

        // Need to check if flag name is common to both shaders (since flags values can be different), if so activate it on this layer
        if( nMaskGenBase&pBaseBit->m_Mask )
        {
          if (!pLayerBit->m_ParamName.empty() && !pBaseBit->m_ParamName.empty())
          {
            if( pLayerBit->m_ParamName ==  pBaseBit->m_ParamName )
            {
              nMaskGenLayer |= pLayerBit->m_Mask; 
              break;
            }
          }
        }
      }
    }
  }

  // Reload with proper flags
  SShaderItem pShaderItem = gEnv->pRenderer->EF_LoadShaderItem( szShaderName, false, 0, &pInputResources, nMaskGenLayer);
  if (!pShaderItem.m_pShader )
  {
    Warning( "Failed to load material layer shader %s in Material %s", szShaderName, pMtl->GetName() );
    return false;
  }
 
  // Copy public params from the shader.
  pInputResources.m_ShaderParams = pShaderItem.m_pShader->GetPublicParams();

  SAFE_RELEASE( pShaderItem.m_pShaderResources );

  // Copy resources from base material
  pShaderItem.m_pShaderResources = pShaderItemBase.m_pShaderResources->Clone();

  // Parse public parameters, and assign them to source shader resources.
  ParsePublicParams( pInputResources, pPublicsNode );
  pShaderItem.m_pShaderResources->SetShaderParams(&pInputResources, pShaderItem.m_pShader);  

  IMaterialLayer *pCurrMtlLayer = pMtl->CreateLayer();
    
  pCurrMtlLayer->SetFlags( nLayerFlags );
  pCurrMtlLayer->SetShaderItem(pMtl, pShaderItem);
  
  pMtl->SetLayer(nSlot, pCurrMtlLayer);

  return true;
}

//////////////////////////////////////////////////////////////////////////

void CMatMan::ParsePublicParams( SInputShaderResources &sr,XmlNodeRef paramsNode )
{
	if (sr.m_ShaderParams.empty())
		return;

	for (int i = 0; i < sr.m_ShaderParams.size(); i++)
	{
		SShaderParam *pParam = &sr.m_ShaderParams[i];
		switch (pParam->m_Type)
		{
		case eType_BYTE:
			paramsNode->getAttr( pParam->m_Name,pParam->m_Value.m_Byte );
			break;
		case eType_SHORT:
			paramsNode->getAttr( pParam->m_Name,pParam->m_Value.m_Short );
			break;
		case eType_INT:
			paramsNode->getAttr( pParam->m_Name,pParam->m_Value.m_Int );
			break;
		case eType_FLOAT:
			paramsNode->getAttr( pParam->m_Name,pParam->m_Value.m_Float );
			break;
		//case eType_STRING:
			//paramsNode->getAttr( pParam->m_Name,pParam->m_Value.m_String );
			//break;
		case eType_FCOLOR:
			{
				Vec3 v(pParam->m_Value.m_Color[0],pParam->m_Value.m_Color[1],pParam->m_Value.m_Color[2]);
				paramsNode->getAttr( pParam->m_Name,v );
				pParam->m_Value.m_Color[0] = v.x;
				pParam->m_Value.m_Color[1] = v.y;
				pParam->m_Value.m_Color[2] = v.z;
			}
			break;
		case eType_VECTOR:
			{
				Vec3 v(pParam->m_Value.m_Vector[0],pParam->m_Value.m_Vector[1],pParam->m_Value.m_Vector[2]);
				paramsNode->getAttr( pParam->m_Name,v );
				pParam->m_Value.m_Vector[0] = v.x;
				pParam->m_Value.m_Vector[1] = v.y;
				pParam->m_Value.m_Vector[2] = v.z;
			}
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
ISurfaceType* CMatMan::GetSurfaceTypeByName( const char *sSurfaceTypeName,const char *sWhy )
{
	return m_pSurfaceTypeManager->GetSurfaceTypeByName(sSurfaceTypeName,sWhy);
};

//////////////////////////////////////////////////////////////////////////
int CMatMan::GetSurfaceTypeIdByName( const char *sSurfaceTypeName,const char *sWhy )
{
	ISurfaceType* pSurfaceType = m_pSurfaceTypeManager->GetSurfaceTypeByName(sSurfaceTypeName,sWhy);
	if (pSurfaceType)
		return pSurfaceType->GetId();
	return 0;
};

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::GetDefaultLayersMaterial()
{
	if (!m_bInitialized)
		InitDefaults();

  return m_pDefaultLayersMtl;
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::GetDefaultHelperMaterial()
{
	if (!m_bInitialized)
		InitDefaults();

	return m_pDefaultHelperMtl;
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::GetLoadedMaterials( IMaterial **pData, uint32 &nObjCount ) const
{
	nObjCount = m_mtlNameMap.size();

	if(!pData)
		return;

	MtlNameMap::const_iterator it, end=m_mtlNameMap.end();

	for(it=m_mtlNameMap.begin();it!=end;++it)
	{
		IMaterial *pMat = it->second;

		*pData++ = pMat;
	}
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::CloneMaterial( IMaterial* pSrcMtl,int nSubMtl )
{
	if (pSrcMtl->GetFlags() & MTL_FLAG_MULTI_SUBMTL)
	{
		IMaterial *pMultiMat = new CMatInfo;

		//m_mtlSet.insert( pMat );
		pMultiMat->SetName( pSrcMtl->GetName() );
		pMultiMat->SetFlags( pMultiMat->GetFlags()|MTL_FLAG_MULTI_SUBMTL );

		bool bCloneAllSubMtls = nSubMtl < 0;

		int nSubMtls = pSrcMtl->GetSubMtlCount();
		pMultiMat->SetSubMtlCount(nSubMtls);
		for (int i = 0; i < nSubMtls; i++)
		{
			CMatInfo *pChildSrcMtl = (CMatInfo*)pSrcMtl->GetSubMtl(i);
			if (!pChildSrcMtl)
				continue;
			if (bCloneAllSubMtls)
			{
				pMultiMat->SetSubMtl( i,pChildSrcMtl->Clone() );
			}
			else
			{
				pMultiMat->SetSubMtl( i,pChildSrcMtl );
				if (i == nSubMtls)
				{
					// Clone this slot.
					pMultiMat->SetSubMtl( i,pChildSrcMtl->Clone() );
				}
			}
		}
		return pMultiMat;
	}
	else
	{
		return ((CMatInfo*)pSrcMtl)->Clone();
	}
}

void CMatMan::CopyMaterial( IMaterial* pMtlSrc, IMaterial* pMtlDest, EMaterialCopyFlags flags )
{
  return ((CMatInfo*)pMtlSrc)->Copy(pMtlDest,flags);
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::CloneMultiMaterial( IMaterial* pSrcMtl,const char *sSubMtlName )
{
	if (pSrcMtl->GetFlags() & MTL_FLAG_MULTI_SUBMTL)
	{
		IMaterial *pMultiMat = new CMatInfo;

		//m_mtlSet.insert( pMat );
		pMultiMat->SetName( pSrcMtl->GetName() );
		pMultiMat->SetFlags( pMultiMat->GetFlags()|MTL_FLAG_MULTI_SUBMTL );

		bool bCloneAllSubMtls = sSubMtlName == 0;

		int nSubMtls = pSrcMtl->GetSubMtlCount();
		pMultiMat->SetSubMtlCount(nSubMtls);
		for (int i = 0; i < nSubMtls; i++)
		{
			CMatInfo *pChildSrcMtl = (CMatInfo*)pSrcMtl->GetSubMtl(i);
			if (!pChildSrcMtl)
				continue;
			if (bCloneAllSubMtls)
			{
				pMultiMat->SetSubMtl( i,pChildSrcMtl->Clone() );
			}
			else
			{
				pMultiMat->SetSubMtl( i,pChildSrcMtl );
				if (stricmp(pChildSrcMtl->GetName(),sSubMtlName) == 0)
				{
					// Clone this slot.
					pMultiMat->SetSubMtl( i,pChildSrcMtl->Clone() );
				}
			}
		}
		return pMultiMat;
	}
	else
	{
		return ((CMatInfo*)pSrcMtl)->Clone();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::InitDefaults()
{
	if (m_bInitialized)
		return;
	m_bInitialized = true;

  LOADING_TIME_PROFILE_SECTION(GetSystem());

	if (!m_pDefaultMtl)
	{
		PrintMessage("Initializing default materials...");

		m_pDefaultMtl = new CMatInfo;
		m_pDefaultMtl->SetName( "Default" );
		SInputShaderResources sr;
		sr.m_Opacity = 1;
		sr.m_LMaterial.m_Diffuse.set(1,1,1,1);
		sr.m_Textures[EFTT_DIFFUSE].m_Name = szReplaceMe;
		//sr.m_Textures[EFTT_DIFFUSE].m_Name = "<default>";
		SShaderItem si = GetRenderer()->EF_LoadShaderItem("Illum", true, 0, &sr);
		if (si.m_pShaderResources)
			si.m_pShaderResources->SetMaterialName("Default");
		m_pDefaultMtl->AssignShaderItem(si);
	}

	if(!m_pDefaultTerrainLayersMtl)
	{
		m_pDefaultTerrainLayersMtl = new CMatInfo;
		m_pDefaultTerrainLayersMtl->SetName( "DefaultTerrainLayer" );
		SInputShaderResources sr;
		sr.m_Opacity = 1;
		sr.m_LMaterial.m_Diffuse.set(1,1,1,1);
		sr.m_Textures[EFTT_DIFFUSE].m_Name = szReplaceMe;
		SShaderItem si = GetRenderer()->EF_LoadShaderItem("Terrain.Layer", true, 0, &sr);
		if (si.m_pShaderResources)
			si.m_pShaderResources->SetMaterialName("DefaultTerrainLayer");
		m_pDefaultTerrainLayersMtl->AssignShaderItem(si);
	}

  if( !m_pDefaultLayersMtl )
  {
    m_pDefaultLayersMtl = LoadMaterial( "Materials/material_layers_default", false );
  }

	if (!m_pNoDrawMtl)
	{
		m_pNoDrawMtl = new CMatInfo;
		m_pNoDrawMtl->SetFlags(MTL_FLAG_NODRAW);
		m_pNoDrawMtl->SetName(MATERIAL_NODRAW);
		SShaderItem si;
		si.m_pShader = GetRenderer()->EF_LoadShader(MATERIAL_NODRAW, EF_SYSTEM);
		m_pNoDrawMtl->AssignShaderItem(si);
		m_mtlNameMap[ UnifyName(m_pNoDrawMtl->GetName()) ] = m_pNoDrawMtl;
	}

	if (!m_pDefaultHelperMtl)
	{
		m_pDefaultHelperMtl = new CMatInfo;
		m_pDefaultHelperMtl->SetName( "DefaultHelper" );
		SInputShaderResources sr;
		sr.m_Opacity = 1;
		sr.m_LMaterial.m_Diffuse.set(1,1,1,1);
		sr.m_Textures[EFTT_DIFFUSE].m_Name = szReplaceMe;
		SShaderItem si = GetRenderer()->EF_LoadShaderItem("Helper", true, 0, &sr);
		if (si.m_pShaderResources)
			si.m_pShaderResources->SetMaterialName("DefaultHelper");
		m_pDefaultHelperMtl->AssignShaderItem(si);
	}
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::LoadCGFMaterial( CMaterialCGF *pMaterialCGF,const char *sCgfFilename,unsigned long nLoadingFlags )
{
  FUNCTION_PROFILER_3DENGINE;

	if (!pMaterialCGF->pMatEntity && !pMaterialCGF->bOldMaterial)
	{
		CryPathString sMtlName = pMaterialCGF->name;
		if (sMtlName.find('/') == stack_string::npos)
		{
			// If no slashes in the name assume it is in same folder as a cgf.
			sMtlName = PathUtil::AddSlash(PathUtil::GetPath(stack_string(sCgfFilename)) ) + sMtlName;
		}
		return LoadMaterial( sMtlName.c_str(),true, false,nLoadingFlags );
	}

	//////////////////////////////////////////////////////////////////////////
	// Create materials from Old CGF materials.
	//////////////////////////////////////////////////////////////////////////
	stack_string sMtlName;
	sMtlName = sCgfFilename;
	sMtlName.replace( '\\','/' );
	PathUtil::RemoveExtension(sMtlName);

	IMaterial *pMaterial = LoadMaterial( sMtlName.c_str(),false );
	if (!pMaterial)
	{
		stack_string sMtlPath = PathUtil::GetPath(sMtlName);
		if (pMaterialCGF->pMatEntity)
		{
			pMaterial = CreateMaterial( sMtlName );
			((CMatInfo*)pMaterial)->LoadFromMatEntity( pMaterialCGF->pMatEntity,sMtlPath.c_str() );
			pMaterial->SetName( sMtlName );
		}
		else
		{
			pMaterial = CreateMaterial( sMtlName,MTL_FLAG_MULTI_SUBMTL );
			pMaterial->SetSubMtlCount( pMaterialCGF->subMaterials.size() );
			for (int i = 0; i < (int)pMaterialCGF->subMaterials.size(); i++)
			{
				CMaterialCGF *pChildMtlCGF = pMaterialCGF->subMaterials[i];
				if (pChildMtlCGF && pChildMtlCGF->pMatEntity)
				{
					IMaterial *pChildMtl = CreateMaterial( pChildMtlCGF->name,MTL_FLAG_PURE_CHILD );
					((CMatInfo*)pChildMtl)->LoadFromMatEntity( pChildMtlCGF->pMatEntity,sMtlPath );
					pMaterial->SetSubMtl( i,pChildMtl );
				}
			}
		}
		if (pMaterial)
		{
			NotifyCreateMaterial(pMaterial);
		}
	}
	if (!pMaterial)
	{
		pMaterial = GetDefaultMaterial();
	}
	return pMaterial;
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::PrecacheDecalMaterials()
{
	LOADING_TIME_PROFILE_SECTION(GetSystem());

	// Done with PreloadLevelMaterials

	/*
	// Wildcards load.
	string sPath = PathUtil::Make((const char*)MATERIAL_DECALS_FOLDER,(const char*)"*.mtl" );
  PrintMessage("===== Loading all materials from a folder: %s =====", sPath.c_str());
	ICryPak *pack = gEnv->pCryPak;
	_finddata_t fd;
	intptr_t handle = pack->FindFirst( sPath, &fd );
	if (handle >= 0)
	{
		do {
			stack_string sMtlName = PathUtil::Make( stack_string(MATERIAL_DECALS_FOLDER),stack_string(fd.name) );
			PathUtil::RemoveExtension(sMtlName);
			IMaterial *pMtl = LoadMaterial( sMtlName,true );
			if (pMtl)
			{
				m_precachedDecalMaterial.push_back( (CMatInfo*)pMtl );
			}
		} while (pack->FindNext( handle, &fd ) >= 0);
		pack->FindClose(handle);
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::SetSketchMode( int mode )
{
  if (mode != 0)
  {
    gEnv->pConsole->ExecuteString( "exec sketch_on" );
  }
  else
  {
    gEnv->pConsole->ExecuteString( "exec sketch_off" );
    gEnv->pConsole->ExecuteString( "sys_RestoreSpec apply" ); // Restore the spec as it was before sketch.
  }

	for (MtlNameMap::iterator it = m_mtlNameMap.begin(); it != m_mtlNameMap.end(); ++it)
	{
		CMatInfo *pMtl = (CMatInfo*)it->second;
		pMtl->SetSketchMode( mode );
	}	
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::SetTexelDensityDebug( int mode )
{
	for (MtlNameMap::iterator it = m_mtlNameMap.begin(); it != m_mtlNameMap.end(); ++it)
	{
		CMatInfo *pMtl = (CMatInfo*)it->second;
		pMtl->SetTexelDensityDebug( mode );
	}	
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_LOAD_START:
		if (!m_pXmlParser)
		{
			// Create a shared XML parser for level loading.
			m_pXmlParser = m_pXmlParser = GetISystem()->GetXmlUtils()->CreateXmlParser();
		}
		break;
	case ESYSTEM_EVENT_LEVEL_LOAD_END:
		// Free xml parser after the level have been loaded.
		m_pXmlParser = 0;
		break;
	}
}

namespace
{
  inline Vec3 ToVec3( const ColorF &col ) { return Vec3(col.r,col.g,col.b); }
//  inline ColorF ToCFColor( const Vec3 &col ) { return ColorF(col); }
  static bool IsPureChild(IMaterial *pMtl)
  { 
    return (pMtl->GetFlags()&MTL_FLAG_PURE_CHILD) ? true : false; 
  }
  static bool IsMultiSubMaterial(IMaterial *pMtl)
  { 
    return (pMtl->GetFlags()&MTL_FLAG_MULTI_SUBMTL) ? true : false; 
  };

  //////////////////////////////////////////////////////////////////////////
  void SetXmlFromShaderParams( SInputShaderResources &pShaderResources, XmlNodeRef &node )
  {
    for (int i = 0; i < pShaderResources.m_ShaderParams.size(); i++)
    {
      SShaderParam *pParam = &pShaderResources.m_ShaderParams[i];
      switch (pParam->m_Type)
      {
      case eType_BYTE:
        node->setAttr(pParam->m_Name,(int)pParam->m_Value.m_Byte );
        break;
      case eType_SHORT:
        node->setAttr(pParam->m_Name,(int)pParam->m_Value.m_Short );
        break;
      case eType_INT:
        node->setAttr(pParam->m_Name,(int)pParam->m_Value.m_Int );
        break;
      case eType_FLOAT:
        node->setAttr(pParam->m_Name,(float)pParam->m_Value.m_Float );
        break;
      case eType_FCOLOR:
        node->setAttr(pParam->m_Name,Vec3(pParam->m_Value.m_Color[0],pParam->m_Value.m_Color[1],pParam->m_Value.m_Color[2]) );
        break;
      case eType_VECTOR:
        node->setAttr(pParam->m_Name,Vec3(pParam->m_Value.m_Vector[0],pParam->m_Value.m_Vector[1],pParam->m_Value.m_Vector[2]) );
        break;
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::LoadMaterialFromXml( const char *sMtlName,XmlNodeRef mtlNode )
{
	const char *name = UnifyName(sMtlName);

	MtlNameMap::const_iterator it = m_mtlNameMap.find(CONST_TEMP_STRING(name));

	IMaterial *pMtl = 0;

	if(it!=m_mtlNameMap.end()) {
		pMtl = it->second;
		pMtl = MakeMaterialFromXml( name,mtlNode,false,0,pMtl );
		return pMtl;
	}

	if (!pMtl)
	{
		pMtl = MakeMaterialFromXml( name,mtlNode,false );
	}

	return pMtl;	
}

//////////////////////////////////////////////////////////////////////////
bool CMatMan::SaveMaterial( XmlNodeRef node, IMaterial *pMtl )
{
  if (!defaultTexMod_Initialized)
  {
    ZeroStruct(defaultTexMod);
    defaultTexMod.m_Tiling[0] = 1;
    defaultTexMod.m_Tiling[1] = 1;
    defaultTexMod_Initialized = true;

  }
  // Saving.
  node->setAttr( "MtlFlags",pMtl->GetFlags() );

  SShaderItem& si = pMtl->GetShaderItem(0);
  SInputShaderResources m_shaderResources = SInputShaderResources(si.m_pShaderResources);

  if (!IsMultiSubMaterial(pMtl))
  {
    node->setAttr( "Shader", si.m_pShader->GetName() );
    node->setAttr( "GenMask",si.m_pShader->GetGenerationMask() );
    node->setAttr( "SurfaceType",	pMtl->GetSurfaceType() ? pMtl->GetSurfaceType()->GetName() : NULL);
    node->setAttr( "MatTemplate",	pMtl->GetMatTemplate() ? pMtl->GetMatTemplate()->GetName() : "");

    SInputShaderResources &sr = m_shaderResources;
    //if (!m_shaderName.IsEmpty() && (stricmp(m_shaderName,"nodraw") != 0))
    {
      // Save ligthing data.
      if (defaultShaderResource.m_LMaterial.m_Diffuse != m_shaderResources.m_LMaterial.m_Diffuse)
        node->setAttr( "Diffuse",ToVec3(m_shaderResources.m_LMaterial.m_Diffuse) );
      if (defaultShaderResource.m_LMaterial.m_Specular != m_shaderResources.m_LMaterial.m_Specular)
        node->setAttr( "Specular",ToVec3(m_shaderResources.m_LMaterial.m_Specular) );
      if (defaultShaderResource.m_LMaterial.m_Emission != m_shaderResources.m_LMaterial.m_Emission)
        node->setAttr( "Emissive",ToVec3(m_shaderResources.m_LMaterial.m_Emission) );
      if (defaultShaderResource.m_LMaterial.m_SpecShininess != m_shaderResources.m_LMaterial.m_SpecShininess)
        node->setAttr( "Shininess",m_shaderResources.m_LMaterial.m_SpecShininess );

      if (defaultShaderResource.m_Opacity != sr.m_Opacity)
        node->setAttr( "Opacity",sr.m_Opacity );
      if (defaultShaderResource.m_AlphaRef != sr.m_AlphaRef)
        node->setAttr( "AlphaTest",sr.m_AlphaRef );
      if (defaultShaderResource.m_GlowAmount != sr.m_GlowAmount)
        node->setAttr( "GlowAmount",sr.m_GlowAmount);
      if (defaultShaderResource.m_FurAmount != sr.m_FurAmount)
        node->setAttr( "FurAmount",sr.m_FurAmount);
      if (defaultShaderResource.m_PostEffects != sr.m_PostEffects)
        node->setAttr( "PostEffectType",sr.m_PostEffects);

      if (defaultShaderResource.m_DeformInfo.m_eType != sr.m_DeformInfo.m_eType)
        node->setAttr( "vertModifType",sr.m_DeformInfo.m_eType );

      // Save texturing data.
      XmlNodeRef texturesNode = node->newChild( "Textures" );
      for (int i = 0; i < sizeof(sUsedTextures)/sizeof(sUsedTextures[0]); i++)
      {
        int texId = sUsedTextures[i].texId;
        if (!sr.m_Textures[texId].m_Name.empty())
        {
          XmlNodeRef texNode = texturesNode->newChild( "Texture" );
          //			texNode->setAttr( "TexID",texId );
          texNode->setAttr( "Map",sUsedTextures[i].name );
          texNode->setAttr( "File",sr.m_Textures[texId].m_Name.c_str() );

          if (sr.m_Textures[texId].m_Filter != defaultShaderResource.m_Textures[texId].m_Filter)
            texNode->setAttr( "Filter",sr.m_Textures[texId].m_Filter );
          if (sr.m_Textures[texId].m_Amount != defaultShaderResource.m_Textures[texId].m_Amount)
            texNode->setAttr( "Amount",sr.m_Textures[texId].m_Amount );
          if (sr.m_Textures[texId].m_bUTile != defaultShaderResource.m_Textures[texId].m_bUTile)
            texNode->setAttr( "IsTileU",sr.m_Textures[texId].m_bUTile );
          if (sr.m_Textures[texId].m_bVTile != defaultShaderResource.m_Textures[texId].m_bVTile)
            texNode->setAttr( "IsTileV",sr.m_Textures[texId].m_bVTile );
          if (sr.m_Textures[texId].m_Sampler.m_eTexType != defaultShaderResource.m_Textures[texId].m_Sampler.m_eTexType)
            texNode->setAttr( "TexType",sr.m_Textures[texId].m_Sampler.m_eTexType );

          SEfTexModificator &texm = sr.m_Textures[texId].m_TexModificator;
          if (memcmp(&texm,&defaultTexMod,sizeof(texm)) == 0)
            continue;

          XmlNodeRef modNode = texNode->newChild( "TexMod" );
          //////////////////////////////////////////////////////////////////////////
          // Save texture modificators Modificators
          //////////////////////////////////////////////////////////////////////////
          if (texm.m_Tiling[0] != defaultTexMod.m_Tiling[0])
            modNode->setAttr( "TileU",texm.m_Tiling[0] );
          if (texm.m_Tiling[1] != defaultTexMod.m_Tiling[1])
            modNode->setAttr( "TileV",texm.m_Tiling[1] );
          if (texm.m_Offs[0] != defaultTexMod.m_Offs[0])
            modNode->setAttr( "OffsetU",texm.m_Offs[0] );
          if (texm.m_Offs[1] != defaultTexMod.m_Offs[1])
            modNode->setAttr( "OffsetV",texm.m_Offs[1] );
          if (texm.m_Rot[0] != defaultTexMod.m_Rot[0])
            modNode->setAttr( "RotateU",Word2Degr(texm.m_Rot[0]) );
          if (texm.m_Rot[1] != defaultTexMod.m_Rot[1])
            modNode->setAttr( "RotateV",Word2Degr(texm.m_Rot[1]) );
          if (texm.m_Rot[2] != defaultTexMod.m_Rot[2])
            modNode->setAttr( "RotateW",Word2Degr(texm.m_Rot[2]) );
          if (texm.m_eUMoveType != defaultTexMod.m_eUMoveType)
            modNode->setAttr( "TexMod_UOscillatorType",texm.m_eUMoveType );
          if (texm.m_eVMoveType != defaultTexMod.m_eVMoveType)
            modNode->setAttr( "TexMod_VOscillatorType",texm.m_eVMoveType );
          if (texm.m_eRotType != defaultTexMod.m_eRotType)
            modNode->setAttr( "TexMod_RotateType",texm.m_eRotType );
          if (texm.m_eTGType != defaultTexMod.m_eTGType)
            modNode->setAttr( "TexMod_TexGenType",texm.m_eTGType );

          if (texm.m_RotOscRate[0] != defaultTexMod.m_RotOscRate[0])
            modNode->setAttr( "TexMod_URotateRate",Word2Degr(texm.m_RotOscRate[0]) );
          if (texm.m_RotOscPhase[0] != defaultTexMod.m_RotOscPhase[0])
            modNode->setAttr( "TexMod_URotatePhase",Word2Degr(texm.m_RotOscPhase[0]) );
          if (texm.m_RotOscAmplitude[0] != defaultTexMod.m_RotOscAmplitude[0])
            modNode->setAttr( "TexMod_URotateAmplitude",Word2Degr(texm.m_RotOscAmplitude[0]) );
          if (texm.m_RotOscRate[1] != defaultTexMod.m_RotOscRate[1])
            modNode->setAttr( "TexMod_VRotateRate",Word2Degr(texm.m_RotOscRate[1]) );
          if (texm.m_RotOscPhase[1] != defaultTexMod.m_RotOscPhase[1])
            modNode->setAttr( "TexMod_VRotatePhase",Word2Degr(texm.m_RotOscPhase[1]) );
          if (texm.m_RotOscAmplitude[1] != defaultTexMod.m_RotOscAmplitude[1])
            modNode->setAttr( "TexMod_VRotateAmplitude",Word2Degr(texm.m_RotOscAmplitude[1]) );
          if (texm.m_RotOscRate[2] != defaultTexMod.m_RotOscRate[2])
            modNode->setAttr( "TexMod_WRotateRate",Word2Degr(texm.m_RotOscRate[2]) );
          if (texm.m_RotOscPhase[2] != defaultTexMod.m_RotOscPhase[2])
            modNode->setAttr( "TexMod_WRotatePhase",Word2Degr(texm.m_RotOscPhase[2]) );
          if (texm.m_RotOscAmplitude[2] != defaultTexMod.m_RotOscAmplitude[2])
            modNode->setAttr( "TexMod_WRotateAmplitude",Word2Degr(texm.m_RotOscAmplitude[2]) );
          if (texm.m_RotOscCenter[0] != defaultTexMod.m_RotOscCenter[0])
            modNode->setAttr( "TexMod_URotateCenter",texm.m_RotOscCenter[0] );
          if (texm.m_RotOscCenter[1] != defaultTexMod.m_RotOscCenter[1])
            modNode->setAttr( "TexMod_VRotateCenter",texm.m_RotOscCenter[1] );
          if (texm.m_RotOscCenter[2] != defaultTexMod.m_RotOscCenter[2])
            modNode->setAttr( "TexMod_WRotateCenter",texm.m_RotOscCenter[2] );

          if (texm.m_UOscRate != defaultTexMod.m_UOscRate)
            modNode->setAttr( "TexMod_UOscillatorRate",texm.m_UOscRate );
          if (texm.m_VOscRate != defaultTexMod.m_VOscRate)
            modNode->setAttr( "TexMod_VOscillatorRate",texm.m_VOscRate );
          if (texm.m_UOscPhase != defaultTexMod.m_UOscPhase)
            modNode->setAttr( "TexMod_UOscillatorPhase",texm.m_UOscPhase );
          if (texm.m_VOscPhase != defaultTexMod.m_VOscPhase)
            modNode->setAttr( "TexMod_VOscillatorPhase",texm.m_VOscPhase );
          if (texm.m_UOscAmplitude != defaultTexMod.m_UOscAmplitude)
            modNode->setAttr( "TexMod_UOscillatorAmplitude",texm.m_UOscAmplitude );
          if (texm.m_VOscAmplitude != defaultTexMod.m_VOscAmplitude)
            modNode->setAttr( "TexMod_VOscillatorAmplitude",texm.m_VOscAmplitude );
        }
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Check if we have vertex deform.
  //////////////////////////////////////////////////////////////////////////
  if (m_shaderResources.m_DeformInfo.m_eType != eDT_Unknown)
  {
    XmlNodeRef deformNode = node->newChild("VertexDeform");
    deformNode->setAttr( "Type",m_shaderResources.m_DeformInfo.m_eType );
    deformNode->setAttr( "DividerX",m_shaderResources.m_DeformInfo.m_fDividerX );
    deformNode->setAttr( "DividerY",m_shaderResources.m_DeformInfo.m_fDividerY );
    deformNode->setAttr( "NoiseScale",m_shaderResources.m_DeformInfo.m_vNoiseScale );

    if (m_shaderResources.m_DeformInfo.m_WaveX.m_eWFType != eWF_None)
    {
      XmlNodeRef waveX = deformNode->newChild("WaveX");
      waveX->setAttr( "Type",m_shaderResources.m_DeformInfo.m_WaveX.m_eWFType );
      waveX->setAttr( "Amp",m_shaderResources.m_DeformInfo.m_WaveX.m_Amp );
      waveX->setAttr( "Level",m_shaderResources.m_DeformInfo.m_WaveX.m_Level );
      waveX->setAttr( "Phase",m_shaderResources.m_DeformInfo.m_WaveX.m_Phase );
      waveX->setAttr( "Freq",m_shaderResources.m_DeformInfo.m_WaveX.m_Freq );
    }
    if (m_shaderResources.m_DeformInfo.m_WaveY.m_eWFType != eWF_None)
    {
      XmlNodeRef waveY = deformNode->newChild("WaveY");
      waveY->setAttr( "Type",m_shaderResources.m_DeformInfo.m_WaveY.m_eWFType );
      waveY->setAttr( "Amp",m_shaderResources.m_DeformInfo.m_WaveY.m_Amp );
      waveY->setAttr( "Level",m_shaderResources.m_DeformInfo.m_WaveY.m_Level );
      waveY->setAttr( "Phase",m_shaderResources.m_DeformInfo.m_WaveY.m_Phase );
      waveY->setAttr( "Freq",m_shaderResources.m_DeformInfo.m_WaveY.m_Freq );
    }
  }

  if (pMtl->GetSubMtlCount() > 0)
  {
    // Serialize sub materials.
    XmlNodeRef childsNode = node->newChild( "SubMaterials" );
    for (int i = 0; i < pMtl->GetSubMtlCount(); i++)
    {
      IMaterial *pSubMtl = pMtl->GetSubMtl(i);
      if (pSubMtl && IsPureChild(pSubMtl))
      {
        XmlNodeRef mtlNode = childsNode->newChild( "Material" );
        mtlNode->setAttr( "Name",pSubMtl->GetName() );
        SaveMaterial(mtlNode,pSubMtl);
      }
      else
      {
        XmlNodeRef mtlNode = childsNode->newChild( "MaterialRef" );
        if (pSubMtl)
          mtlNode->setAttr( "Name",pSubMtl->GetName() );
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Save public parameters.
  //////////////////////////////////////////////////////////////////////////
/*
  if (m_publicVarsCache)
  {
    node->addChild( m_publicVarsCache );
  }
  else */
  if (!m_shaderResources.m_ShaderParams.empty())
  {
    XmlNodeRef publicsNode = node->newChild( "PublicParams" );
    SetXmlFromShaderParams( m_shaderResources, publicsNode );
  }

  //////////////////////////////////////////////////////////////////////////
  // Save material layers data
  //////////////////////////////////////////////////////////////////////////

  bool bMaterialLayers = false;
  for(int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
  {
    const IMaterialLayer* pLayer = pMtl->GetLayer(l);
    if(pLayer && pLayer->GetShaderItem().m_pShader && strlen(pLayer->GetShaderItem().m_pShader->GetName())!=0 )
    {
      bMaterialLayers = true;
      break;
    }
  }

  if( bMaterialLayers )
  {
    XmlNodeRef mtlLayersNode = node->newChild( "MaterialLayers" );    
    for(int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
    {
      XmlNodeRef layerNode = mtlLayersNode->newChild( "Layer" );
      const IMaterialLayer* pLayer = pMtl->GetLayer(l);
      SInputShaderResources m_shaderRes = SInputShaderResources(pLayer->GetShaderItem().m_pShaderResources);
      if( pLayer && pLayer->GetShaderItem().m_pShader && strlen(pLayer->GetShaderItem().m_pShader->GetName())!=0 )
      {
        layerNode->setAttr( "Name", pLayer->GetShaderItem().m_pShader->GetName());                        
        layerNode->setAttr( "NoDraw", pLayer->GetShaderItem().m_pShader->GetFlags() & MTL_LAYER_USAGE_NODRAW );     

        /*
        if(m_shaderRes.m_publicVarsCache)
        {
          layerNode->addChild( m_pMtlLayerResources[l].m_publicVarsCache );
        }
        else 
        */
        if( !m_shaderRes.m_ShaderParams.empty() ) 
        {
          XmlNodeRef publicsNode = layerNode->newChild( "PublicParams" );
          SetXmlFromShaderParams( m_shaderRes, publicsNode ); 
        }
      }
    }
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::GetDefaultMaterial()
{
	return m_pDefaultMtl;
}

IMaterial* CMatMan::GetNoDrawMaterial()
{
	return m_pNoDrawMtl;
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::PreloadLevelMaterials()
{
	LOADING_TIME_PROFILE_SECTION(GetSystem());

	bool bMtlfCacheExist = GetISystem()->GetIResourceManager()->LoadLevelCachePak( MTL_LEVEL_CACHE_PAK );
	if (!bMtlfCacheExist)
		return;

	PrintMessage("Starting loading level Materials");
	float fStartTime = GetCurAsyncTimeSec();

	IResourceList *pResList = GetISystem()->GetIResourceManager()->GetLevelResourceList();

	int nCounter=0;

	_smart_ptr<IXmlParser> pXmlParser = GetISystem()->GetXmlUtils()->CreateXmlParser();

	// Request objects loading from Streaming System.
	CryPathString mtlName;
	CryPathString mtlFilename;
	CryPathString mtlCacheFilename;
	for (const char* sName = pResList->GetFirst(); sName != NULL; sName = pResList->GetNext())
	{
		if(strstr(sName,".mtl") == 0)
			continue;

		mtlFilename = sName;

		mtlName = sName;
		PathUtil::RemoveExtension(mtlName);

		if (FindMaterial(mtlName))
			continue;

		mtlCacheFilename = "LevelCache/mtl/";
		mtlCacheFilename += sName;
		mtlCacheFilename = PathUtil::ReplaceExtension(mtlFilename,"binmtl");
		if (gEnv->pCryPak->IsFileExist( mtlCacheFilename.c_str(),ICryPak::eFileLocation_InPak ))
		{
			mtlFilename = "LevelCache/mtl/";
			mtlFilename += sName;
		}

		XmlNodeRef root =	pXmlParser->ParseFile( mtlFilename,true );
		if (root)
		{
			MakeMaterialFromXml( mtlName,root,false );
		}

		LoadMaterial( mtlName,false );

		nCounter++;
	}

	GetISystem()->GetIResourceManager()->UnloadLevelCachePak( MTL_LEVEL_CACHE_PAK );
	PrintMessage("Finished loading level Materials: %d  mtls loaded in %.1f sec", nCounter, GetCurAsyncTimeSec()-fStartTime);
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::ShutDown()
{
	m_pXmlParser = 0;
	m_nonRemovables.clear();
	m_mtlNameMap.clear();

	m_pDefaultMtl = 0;
	m_pDefaultLayersMtl = 0;
	m_pDefaultTerrainLayersMtl = 0;
	m_pNoDrawMtl = 0;
	m_pDefaultHelperMtl = 0;

	FreeAllMaterials();

	m_pSurfaceTypeManager->RemoveAll();
	m_bInitialized = false;
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::FreeAllMaterials()
{
	//////////////////////////////////////////////////////////////////////////
	CMatInfo *pMtl = CMatInfo::get_intrusive_list_root();
	while (pMtl)
	{
		pMtl->m_nRefCount = 1000000; // This prevent sub material release
		pMtl = pMtl->m_next_intrusive;
	}
	pMtl = CMatInfo::get_intrusive_list_root();
	while (pMtl)
	{
		pMtl->ShutDown();
		pMtl = pMtl->m_next_intrusive;
	}

	while (CMatInfo::get_intrusive_list_root())
	{
		delete CMatInfo::get_intrusive_list_root(); // This will delete all materials
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IMaterialManager)
