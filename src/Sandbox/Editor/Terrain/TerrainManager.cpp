////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TerrainManager.cpp
//  Created:     30/3/2009 by Paulo Zaffari.
//  Description: Manages layer and heightfield data.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include <StdAfx.h>
#include "TerrainManager.h"

//////////////////////////////////////////////////////////////////////////
#include "SurfaceType.h"
#include "Layer.h"
#include "TerrainTexGen.h"
#include "..\Util\AutoLogTime.h"
#include "..\OBJExporter1.h"

#include "..\GameEngine.h"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTerrainManager&	CTerrainManager::GetTerrainManager()
{
	static CTerrainManager oTerrainManager;
	return oTerrainManager;
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::RemoveSurfaceType( int i )
{
	GetIEditor()->SetModifiedFlag(TRUE);
	GetIEditor()->SetModifiedModule(eModifiedTerrain);
	m_surfaceTypes.erase( m_surfaceTypes.begin()+i );
	ConsolidateSurfaceTypes();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::RemoveSurfaceType( CSurfaceType *pSurfaceType )
{
	for (int i = 0; i < m_surfaceTypes.size(); i++)
	{
		if (m_surfaceTypes[i] == pSurfaceType)
		{
			m_surfaceTypes.erase( m_surfaceTypes.begin()+i );
			break;
		}
	}
	ConsolidateSurfaceTypes();
}

//////////////////////////////////////////////////////////////////////////
int CTerrainManager::AddSurfaceType( CSurfaceType *srf )
{
	GetIEditor()->SetModifiedFlag(TRUE);
	GetIEditor()->SetModifiedModule(eModifiedTerrain);
	m_surfaceTypes.push_back(srf);

	return m_surfaceTypes.size()-1;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::RemoveAllSurfaceTypes()
{
	GetIEditor()->SetModifiedFlag(TRUE);
	GetIEditor()->SetModifiedModule(eModifiedTerrain);
	m_surfaceTypes.clear();
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SerializeSurfaceTypes( CXmlArchive &xmlAr, bool bUpdateEngineTerrain )
{
	if (xmlAr.bLoading)
	{
		// Clear old layers
		RemoveAllSurfaceTypes();

		// Load the layer count
		XmlNodeRef node = xmlAr.root->findChild( "SurfaceTypes" );
		if (!node)
			return;

		// Read all node
		for (int i=0; i < node->getChildCount(); i++)
		{
			CXmlArchive ar( xmlAr );
			ar.root = node->getChild(i);
			CSurfaceType *sf = new CSurfaceType;
			sf->Serialize( ar );

			if (sf->GetSurfaceTypeID() < 0) // For older levels.
			{
				sf->SetSurfaceTypeID(i);
			}

			AddSurfaceType( sf );
		}
		ReloadSurfaceTypes(bUpdateEngineTerrain);
	}
	else
	{
		// Storing
		CLogFile::WriteLine("Storing surface types...");

		// Save the layer count

		XmlNodeRef node = xmlAr.root->newChild( "SurfaceTypes" );

		// Write all surface types.
		for (int i = 0; i < m_surfaceTypes.size(); i++)
		{
			CXmlArchive ar( xmlAr );
			ar.root = node->newChild( "SurfaceType" );
			m_surfaceTypes[i]->Serialize( ar );
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::ConsolidateSurfaceTypes()
{
	// We must consolidate the IDs after removing 
	for (size_t i = 0; i < m_surfaceTypes.size(); ++i)
	{
		m_surfaceTypes[i]->SetSurfaceTypeID(i);
	}
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::ReloadSurfaceTypes(bool bUpdateEngineTerrain)
{
  LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

	CXmlArchive	ar;
	XmlNodeRef node = CreateXmlNode( "SurfaceTypes" );
	// Write all surface types.
	for (int i = 0; i < GetSurfaceTypeCount(); i++)
	{
		ar.root = node->newChild( "SurfaceType" );
		GetSurfaceTypePtr(i)->Serialize( ar );
	}

  // temporary console variable for world segmentation testing
  static ICVar * pWSeg = GetIEditor()->GetSystem()->GetIConsole()->GetCVar("e_WorldSegmentationTest");

  if(pWSeg && pWSeg->GetIVal()>1)
  {
    int nWorldGridDim = pWSeg->GetFVal();

    // pass same surface types list to all segments
    for(int nSID=0; nSID < nWorldGridDim*nWorldGridDim; nSID++)
      gEnv->p3DEngine->LoadTerrainSurfacesFromXML(node, bUpdateEngineTerrain, nSID);
  }
  else
    gEnv->p3DEngine->LoadTerrainSurfacesFromXML(node, bUpdateEngineTerrain);

	if (gEnv->p3DEngine->GetITerrain())
	{
		m_cHeightmap.UpdateEngineTerrain(false,false);
	}
}
//////////////////////////////////////////////////////////////////////////
int CTerrainManager::FindSurfaceType( const CString &name )
{
	for (int i = 0; i < m_surfaceTypes.size(); i++)
	{
		if (stricmp(m_surfaceTypes[i]->GetName(),name) == 0)
			return i;
	}
	return -1;
}
//////////////////////////////////////////////////////////////////////////












//////////////////////////////////////////////////////////////////////////
void CTerrainManager::RemoveLayer( int layer )
{
	assert( layer >= 0 && layer < m_layers.size() );
	delete m_layers[layer];
	m_layers.erase( m_layers.begin() + layer );
}

//////////////////////////////////////////////////////////////////////////
CLayer* CTerrainManager::FindLayer( const char *sLayerName ) const
{
	for (int i = 0; i < GetLayerCount(); i++)
	{
		CLayer *layer = GetLayer(i);
		if (strcmp(layer->GetLayerName(),sLayerName) == 0)
			return layer;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
CLayer* CTerrainManager::FindLayerByLayerId( const uint32 dwLayerId ) const
{
	for (int i = 0; i < GetLayerCount(); i++)
	{
		CLayer *layer = GetLayer(i);
		if (layer->GetCurrentLayerId() == dwLayerId)
			return layer;
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::RemoveLayer( CLayer *layer )
{
	if (layer)
	{
		delete layer;
		m_layers.erase( std::remove(m_layers.begin(),m_layers.end(),layer),m_layers.end() );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SwapLayers( int layer1,int layer2 )
{
	assert( layer1 >= 0 && layer1 < m_layers.size() );
	assert( layer2 >= 0 && layer2 < m_layers.size() );
	std::swap( m_layers[layer1],m_layers[layer2] );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainManager::InvalidateLayers()
{
	////////////////////////////////////////////////////////////////////////
	// Set the update needed flag for all layer
	////////////////////////////////////////////////////////////////////////

	unsigned int i;

	// Free the layer objects
	for (i=0; i< GetLayerCount(); i++)
		GetLayer(i)->InvalidateMask();
}
//////////////////////////////////////////////////////////////////////////








//////////////////////////////////////////////////////////////////////////
void CTerrainManager::ClearLayers()
{
	////////////////////////////////////////////////////////////////////////
	// Clear all texture layers
	////////////////////////////////////////////////////////////////////////
	unsigned int i;

	// Free the layer objects
	for (i=0; i<GetLayerCount(); i++)
		delete GetLayer(i);

	// Clear the list
	m_layers.clear();
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SerializeLayerSettings( CXmlArchive &xmlAr )
{
	////////////////////////////////////////////////////////////////////////
	// Load or restore the layer settings from an XML
	////////////////////////////////////////////////////////////////////////
	if (xmlAr.bLoading)
	{
		// Loading

		CLogFile::WriteLine("Loading layer settings...");
		CWaitProgress progress( _T("Loading Terrain Layers") );

		// Clear old layers
		ClearLayers();

		// Load the layer count
		XmlNodeRef layers = xmlAr.root->findChild( "Layers" );
		if (!layers)
			return;

		// Read all layers
		int numLayers = layers->getChildCount();
		for (int i = 0; i < numLayers; i++)
		{
			progress.Step( 100*i/numLayers );
			// Create a new layer
			AddLayer( new CLayer );

			CXmlArchive ar( xmlAr );
			ar.root = layers->getChild(i);
			// Fill the layer with the data
			GetLayer(i)->Serialize( ar );

			CryLog("  loaded editor layer %d  name='%s' LayerID=%d",i,GetLayer(i)->GetLayerName(),GetLayer(i)->GetCurrentLayerId());
		}

		// If surface type ids are unassigned, assign them.
		for (int i = 0; i < GetSurfaceTypeCount(); i++)
		{
			if (GetSurfaceTypePtr(i)->GetSurfaceTypeID() < 0 || GetSurfaceTypePtr(i)->GetSurfaceTypeID() >= MAX_SURFACE_TYPE_ID_COUNT)
				GetSurfaceTypePtr(i)->AssignUnusedSurfaceTypeID();
		}
	}
	else
	{
		// Storing

		CLogFile::WriteLine("Storing layer settings...");

		// Save the layer count

		XmlNodeRef layers = xmlAr.root->newChild( "Layers" );

		// Write all layers
		for (int i=0; i < GetLayerCount(); i++)
		{
			CXmlArchive ar( xmlAr );
			ar.root = layers->newChild( "Layer" );;
			GetLayer(i)->Serialize( ar );
		}
	}
}
//////////////////////////////////////////////////////////////////////////
uint32 CTerrainManager::GetDetailIdLayerFromLayerId( const uint32 dwLayerId )
{
	for (int i = 0,num = (int)m_layers.size(); i < num; i++)
	{
		CLayer &rLayer = *(m_layers[i]);

		if(dwLayerId==rLayer.GetOrRequestLayerId())
		{
			return rLayer.GetEngineSurfaceTypeId();
		}
	}

	// recreate referenced layer
	if(!GetIEditor()->GetSystem()->GetI3DEngine()->GetITerrain())		// only if terrain loaded successfully
	{
		CString no;

		no.Format("%d",dwLayerId);

		CLayer *pNewLayer = new CLayer;

		pNewLayer->SetLayerName(CString("LAYER_")+no);
		pNewLayer->SetLayerId(dwLayerId);

		AddLayer(pNewLayer);
	}

	return 0xffffffff;	
}
//////////////////////////////////////////////////////////////////////////
uint32 CTerrainManager::GetLayerIdFromDetailLayerId( const uint32 _dwDetailLayerId )
{
	uint32 dwDetailLayerId=_dwDetailLayerId;

	CSurfaceType *pSurfaceType = 0;

	for (int i = 0; i < (int)m_surfaceTypes.size(); i++)
	{
		if (m_surfaceTypes[i]->GetSurfaceTypeID() == _dwDetailLayerId)
		{
			pSurfaceType = m_surfaceTypes[i];
			break;
		}
	}

	if(!pSurfaceType)
	{
		// recreate referenced surfacetype

		pSurfaceType = new CSurfaceType;

		CString no;

		no.Format("%d",dwDetailLayerId);

		pSurfaceType->SetName(CString("SURFACETYPE_")+no);
		pSurfaceType->SetSurfaceTypeID(_dwDetailLayerId);
		AddSurfaceType(pSurfaceType);
	}

	std::vector<CLayer*>::iterator it, end=m_layers.end();

	for(it=m_layers.begin();it!=end;++it)
	{
		CLayer &rLayer = *(*it);

		if(rLayer.GetEngineSurfaceTypeId()==dwDetailLayerId)
			return rLayer.GetOrRequestLayerId();
	}

	// recreate referenced layer

	CLayer *pNewLayer = new CLayer;

	uint32 dwRet = pNewLayer->GetOrRequestLayerId();		// to create new LayerId

	assert(dwRet!=0xffffffff);

	//pNewLayer->SetSurfaceTypeId(dwDetailLayerId);
	pNewLayer->SetLayerName(CString("DETAILLAYER_")+pSurfaceType->GetName());

	pNewLayer->SetSurfaceType(pSurfaceType);

	AddLayer(pNewLayer);

	return dwRet;
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::MarkUsedLayerIds( bool bFree[256] ) const
{
	std::vector<CLayer*>::const_iterator it;

	for(it=m_layers.begin();it!=m_layers.end();++it)
	{
		CLayer *pLayer=*it;

		if(pLayer->GetCurrentLayerId()!=0xffffffff)
			bFree[pLayer->GetCurrentLayerId()]=false;
	}
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::CreateDefaultLayer()
{
	// Create default layer.
	CLayer *layer = new CLayer;
	AddLayer( layer );
	layer->SetLayerName( "Default" );
	layer->LoadTexture( Path::GetGameFolder()+"/Textures/defaults/defaultnouvs.dds" );
	layer->SetLayerId(0);

	// Create default surface type.
	CSurfaceType *sfType = new CSurfaceType;
	sfType->SetName( "Materials/material_terrain_default" );
	uint32 dwDetailLayerId = AddSurfaceType( sfType );
	sfType->SetMaterial("Materials/material_terrain_default");
	sfType->AssignUnusedSurfaceTypeID();

	layer->SetSurfaceType(sfType);

	m_bIsNewTerranTextureSystem=true;
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SerializeTerrain(CXmlArchive& xmlAr,char* szLevelPath,char* currentMissionName,bool bReloadEngineLevel)
{
	if (xmlAr.bLoading)
	{
		m_cHeightmap.Serialize(xmlAr,false,false);

		if (bReloadEngineLevel)
		{
			CAutoLogTime logtime( "3D Engine level load" );
			GetIEditor()->GetGameEngine()->LoadLevel( szLevelPath,currentMissionName,true,true,false );
		}

		//////////////////////////////////////////////////////////////////////////
		// Load terrain heightmap data
		//////////////////////////////////////////////////////////////////////////

		bool bTerrainRefreshRequested=false;

		// Surface Types ///////////////////////////////////////////////////////

		{
			CAutoLogTime logtime( "Loading Surface Types" );
			SerializeSurfaceTypes( xmlAr );
		}	

		// Terrain texture /////////////////////////////////////////////////////
		{
			CAutoLogTime logtime( "Loading Terrain Layers Info" );
			SerializeLayerSettings(xmlAr);
		}

		{
			CAutoLogTime logtime( "Load Terrain" );
			m_cHeightmap.SerializeTerrain( xmlAr, bTerrainRefreshRequested );

			{
				XmlNodeRef layers = xmlAr.root->findChild( "Layers" );
				if (layers)
				{
					int numLayers = layers->getChildCount();
					for (int i = 0; i < numLayers; i++)
					{
						GetLayer(i)->Update3dengineInfo();
					}
				}
			}
		}

		{
			CAutoLogTime logtime( "Load Vegetation" );
			m_cHeightmap.SerializeVegetation(xmlAr);
		}

		{
			CAutoLogTime logtime( "Terrain Postprocess" );
			m_cHeightmap.ProcessAfterLoading();		// after loading layers and surfacetypes
		}


		{
			CAutoLogTime logtime( "Process RGB Terrain Layers" );
			ConvertLayersToRGBLayer();
		}

		// Update terrain in engine.
		m_cHeightmap.UpdateEngineTerrain();
	}
	else
	{
		// save terrain heightmap data
		bool bTerrainRefreshRequested(false);

		// Heightmap ///////////////////////////////////////////////////////////
		xmlAr.root->setAttr( "HeightmapWidth",m_cHeightmap.GetWidth() );
		xmlAr.root->setAttr( "HeightmapHeight",m_cHeightmap.GetHeight() );

		m_cHeightmap.Serialize( xmlAr,true,false );

		m_cHeightmap.SerializeTerrain( xmlAr, bTerrainRefreshRequested );

		// Terrain texture /////////////////////////////////////////////////////

		// Surface Types ///////////////////////////////////////////////////////
		SerializeSurfaceTypes( xmlAr );

		SerializeLayerSettings(xmlAr);

		m_cHeightmap.SerializeVegetation(xmlAr);
	}
}
//////////////////////////////////////////////////////////////////////////
bool	CTerrainManager::ExportTerrainAsGeometry(const char *pszFileName, RECT rcExport)
{
	////////////////////////////////////////////////////////////////////////
	// Store the terrain in the OBJ format on the disk
	////////////////////////////////////////////////////////////////////////

	t_hmap *pHeigthmap = NULL;
	unsigned int i, j;
	COBJExporter cModel;
	CVector3D vVertex[6];
	CTexCoord2D cTexCoord[6];

	float fScaling = m_cHeightmap.GetUnitSize();

	int iWidth = m_cHeightmap.GetWidth();
	CFace sNewFace;
	char szInfoPath[_MAX_PATH];
	const int iSurfaceTexWidth = 4096;
	DWORD *pSurface = NULL;
	float fScale = (1.0f / m_cHeightmap.GetWidth()) * (float) iSurfaceTexWidth;

	if ((int)rcExport.bottom - rcExport.top <= 0 && (int)rcExport.right - rcExport.left <= 0)
		return false;

	cModel.StartNewObject("Terrain");
	cModel.StartNewSubMesh();

	// Get data from the heightmap
	pHeigthmap = m_cHeightmap.GetData();
	ASSERT(pHeigthmap);

	// Adjust the rectangle to be valid
	if (rcExport.right < 0)
		rcExport.right = 0;
	if (rcExport.left < 0)
		rcExport.left = 0;
	if (rcExport.top < 0)
		rcExport.top = 0;
	if (rcExport.bottom < 0)
		rcExport.bottom = 0;

	if (rcExport.right >= m_cHeightmap.GetWidth())
		rcExport.right = m_cHeightmap.GetWidth() - 1;
	if (rcExport.bottom >= m_cHeightmap.GetHeight())
		rcExport.bottom = m_cHeightmap.GetHeight() - 1;

	////////////////////////////////////////////////////////////////////////
	// Create triangles, texture coordinates and indices
	////////////////////////////////////////////////////////////////////////

	for (j=rcExport.top; j<rcExport.bottom; j++)
	{
		for (i=rcExport.left; i<rcExport.right; i++)
		{
			int sx = i;
			int sy = j;

			// we need world offset
			//int dx = i - rcExport.left;
			//int dy = j - rcExport.top;
			int dx = i;
			int dy = j;

			// scale the same like for objects
			float fExScale = 100.0f;

			// First triangle
			vVertex[0].fY = dx * fScaling * fExScale;
			vVertex[0].fX = dy * fScaling * fExScale;
			vVertex[0].fZ = pHeigthmap[sx + sy * iWidth] * fExScale;
			vVertex[2].fY = (dx + 1) * fScaling * fExScale;
			vVertex[2].fX = dy * fScaling * fExScale;
			vVertex[2].fZ = pHeigthmap[(sx + 1) + sy*iWidth] * fExScale;
			vVertex[1].fY = dx * fScaling * fExScale;
			vVertex[1].fX = (dy + 1) * fScaling * fExScale;
			vVertex[1].fZ = pHeigthmap[sx + (sy + 1)*iWidth] * fExScale;

			cTexCoord[0].fU = (float) (dx) / (float) (rcExport.right - rcExport.left);
			cTexCoord[0].fV = -(float) (dy) / (float) (rcExport.bottom - rcExport.top);

			cTexCoord[2].fU = (float) ((dx + 1)) / (float) (rcExport.right - rcExport.left);
			cTexCoord[2].fV = -(float) (dy) / (float) (rcExport.bottom - rcExport.top);

			cTexCoord[1].fU = (float) (dx) / (float) (rcExport.right - rcExport.left);
			cTexCoord[1].fV = -(float) ((dy + 1)) / (float) (rcExport.bottom - rcExport.top);

			// Second triangle
			vVertex[3].fY = (dx + 1) * fScaling * fExScale;
			vVertex[3].fX = dy * fScaling * fExScale;
			vVertex[3].fZ = pHeigthmap[(sx + 1) + sy * iWidth] * fExScale;
			vVertex[5].fY = (dx + 1) * fScaling * fExScale;
			vVertex[5].fX = (dy + 1) * fScaling * fExScale;
			vVertex[5].fZ = pHeigthmap[(sx + 1) + (sy + 1) * iWidth] * fExScale;
			vVertex[4].fY = dx * fScaling * fExScale;
			vVertex[4].fX = (dy + 1) * fScaling * fExScale;
			vVertex[4].fZ = pHeigthmap[sx + (sy + 1) * iWidth] * fExScale;

			cTexCoord[3].fU = (float) ((dx + 1)) / (float) (rcExport.right - rcExport.left);
			cTexCoord[3].fV = -(float) (dy) / (float) (rcExport.bottom - rcExport.top);
			cTexCoord[5].fU = (float) ((dx + 1)) / (float) (rcExport.right - rcExport.left);
			cTexCoord[5].fV = -(float) ((dy + 1)) / (float) (rcExport.bottom - rcExport.top);
			cTexCoord[4].fU = (float) (dx) / (float) (rcExport.right - rcExport.left);
			cTexCoord[4].fV = -(float) ((dy + 1)) / (float) (rcExport.bottom - rcExport.top);

			// Add the vertices to the model and store the indices in the face
			memset(&sNewFace, 0, sizeof(CFace));
			sNewFace.iVertexIndices[0] = cModel.GetVertexIdx(&vVertex[0]);
			sNewFace.iVertexIndices[1] = cModel.GetVertexIdx(&vVertex[1]);
			sNewFace.iVertexIndices[2] = cModel.GetVertexIdx(&vVertex[2]);
			sNewFace.iTexCoordIndices[0] = cModel.GetTexCoordIdx(&cTexCoord[0]);
			sNewFace.iTexCoordIndices[1] = cModel.GetTexCoordIdx(&cTexCoord[1]);
			sNewFace.iTexCoordIndices[2] = cModel.GetTexCoordIdx(&cTexCoord[2]);
			cModel.AddFace(sNewFace);
			memset(&sNewFace, 0, sizeof(CFace));
			sNewFace.iVertexIndices[0] = cModel.GetVertexIdx(&vVertex[3]);
			sNewFace.iVertexIndices[1] = cModel.GetVertexIdx(&vVertex[4]);
			sNewFace.iVertexIndices[2] = cModel.GetVertexIdx(&vVertex[5]);
			sNewFace.iTexCoordIndices[0] = cModel.GetTexCoordIdx(&cTexCoord[3]);
			sNewFace.iTexCoordIndices[1] = cModel.GetTexCoordIdx(&cTexCoord[4]);
			sNewFace.iTexCoordIndices[2] = cModel.GetTexCoordIdx(&cTexCoord[5]);
			cModel.AddFace(sNewFace);
		}
	}

	CExpMtl mtl;
	strcpy(mtl.m_name, "terrain");
	cModel.AddMtl(mtl);

	////////////////////////////////////////////////////////////////////////
	// Export .OBJ and .MTL files
	////////////////////////////////////////////////////////////////////////

	// Write the model into the file
	if (!cModel.WriteOBJ(pszFileName))
	{
		CLogFile::WriteLine("Error while exporting part of the heightmap as OBJ model !");
		AfxMessageBox("Crititcal error during exporting !");
		return false;
	}

	////////////////////////////////////////////////////////////////////////
	// Export Info
	////////////////////////////////////////////////////////////////////////
	{
		strcpy(szInfoPath, pszFileName);
		PathRemoveExtension(szInfoPath);
		strcat(szInfoPath,".inf");
		if (CFileUtil::OverwriteFile( szInfoPath ))
		{
			FILE *infoFile = fopen( szInfoPath,"wt" );
			if (infoFile)
			{
				fprintf( infoFile,"x=%d,y=%d,width=%d,height=%d",rcExport.left,rcExport.top,rcExport.right-rcExport.left,rcExport.bottom-rcExport.top );
				fclose( infoFile );
			}
		}
	}

	/*
	////////////////////////////////////////////////////////////////////////
	// Generate the texture
	////////////////////////////////////////////////////////////////////////

	char szBitmapPath[_MAX_PATH];
	UINT iSrcX, iSrcY, iDestX, iDestY, iSrcWidth, iDestWidth,iDestHeight;

	// Allocate memory and generate the surface
	CImage terrainSurface;
	terrainSurface.Allocate( iSurfaceTexWidth,iSurfaceTexWidth );
	pSurface = (DWORD*)terrainSurface.GetData();
	//cTexture.GenerateSurface(pSurface, iSurfaceTexWidth, iSurfaceTexWidth, GEN_USE_LIGHTING|GEN_STATOBJ_SHADOWS|GEN_SHOW_WATER, NULL );
	CTerrainTexGen texGen;
	texGen.GenerateSurfaceTexture( ETTG_LIGHTING|ETTG_STATOBJ_SHADOWS|ETTG_SHOW_WATER,terrainSurface );

	//CImageUtil::SaveJPEG( "Terrain.jpg",iSurfaceTexWidth,iSurfaceTexWidth,pSurface );

	// Scale the export rectangle to texture coordinates
	rcExport.bottom *= fScale;
	rcExport.left *= fScale;
	rcExport.right *= fScale;
	rcExport.top *= fScale;

	CRect rc1 = rcExport;
	rc1 &= CRect( 0,0,iSurfaceTexWidth,iSurfaceTexWidth );
	rcExport = rc1;

	iDestWidth = rcExport.right - rcExport.left;
	iDestHeight =  rcExport.bottom - rcExport.top;
	iSrcWidth = iSurfaceTexWidth;

	// Allocate memory for the needed portion of the surface
	CImage exportImage;
	exportImage.Allocate( iDestWidth,iDestHeight );

	// Extract the needed portion of the surface
	for (j=rcExport.top; j<rcExport.bottom; j++)
	{
		iSrcX = rcExport.left;
		iSrcY = j;

		iDestX = 0;
		iDestY = j - rcExport.top;

		memcpy( &exportImage.ValueAt(iDestX,iDestY), &pSurface[iSrcX + iSrcY * iSrcWidth], sizeof(DWORD)*iDestWidth );
	}

	////////////////////////////////////////////////////////////////////////
	// Export the texture
	////////////////////////////////////////////////////////////////////////

	strcpy(szBitmapPath, pszFileName);
	PathRemoveFileSpec(szBitmapPath);
	PathAddBackslash(szBitmapPath);
	strcat(szBitmapPath, "Terrain.bmp");

	if (!CImageUtil::SaveImage( szBitmapPath, exportImage ))
		return false;
	*/

	return true;
}
//////////////////////////////////////////////////////////////////////////
void	CTerrainManager::GetTerrainMemoryUsage( ICrySizer *pSizer )
{
	{
		SIZER_COMPONENT_NAME(pSizer,"Layers");

		std::vector<CLayer*>::iterator it;

		for(it=m_layers.begin();it!=m_layers.end();++it)
		{
			CLayer *pLayer=*it;

			pLayer->GetMemoryUsage(pSizer);
		}
	}

	{
		SIZER_COMPONENT_NAME(pSizer,"CHeightmap");

		GetHeightmap().GetMemoryUsage(pSizer);
	}
}
//////////////////////////////////////////////////////////////////////////
bool CTerrainManager::ConvertLayersToRGBLayer()
{
	bool bConvertNeeded = !GetIEditor()->GetHeightmap()->m_TerrainRGBTexture.IsAllocated();

	if(!bConvertNeeded)
		return false;

	std::vector<CLayer*>::iterator it;

	uint32 dwTexResolution=0;

	for(it=m_layers.begin();it!=m_layers.end();++it)
	{
		CLayer *pLayer=*it;

		dwTexResolution = max(dwTexResolution,(uint32)pLayer->GetMaskResolution());
		/*
		if(pLayer->GetMask().IsValid())
		{ 
		dwTexResolution=pLayer->GetMask().GetWidth();
		bLayersStillHaveAMask=true;
		break; 
		}
		*/
	}

	if(AfxMessageBox("Convert LayeredTerrainTexture to RGBTerrainTexture?",MB_YESNO,0)==IDNO)
		return false;

	if(AfxMessageBox("Double the resolution of the existing data?",MB_YESNO,0)==IDYES)
		dwTexResolution*=2;

	// extract RGB texture
	{
		CTerrainLayerTexGen texGen;

		texGen.Init(dwTexResolution);

		SSectorInfo si;

		GetIEditor()->GetHeightmap()->GetSectorsInfo( si );

		uint32 dwNumSectors = si.numSectors;

		uint32 dwSectorTexResolution = 512;
		uint32 dwSectorResolution = dwTexResolution/dwNumSectors; 
		uint32 dwNumTexSectors = dwTexResolution/dwSectorTexResolution; 

		// no error check because this is allocating only very few bytes
		m_cHeightmap.m_TerrainRGBTexture.AllocateTiles(dwNumTexSectors,dwNumTexSectors,dwSectorTexResolution);	// dwTexResolution x dwTexResolution;

		CImage sectorDiffuseImage;

		if (!sectorDiffuseImage.Allocate( dwSectorResolution,dwSectorResolution ))
			return false;

		int flags = ETTG_QUIET|ETTG_NO_TERRAIN_SHADOWS|ETTG_ABGR;

		for (int y = 0; y < dwNumSectors; y++)
			for (int x = 0; x < dwNumSectors; x++)
			{
				CPoint sector(x,y);

				CRect rect(0,0,sectorDiffuseImage.GetWidth(),sectorDiffuseImage.GetHeight());

				if(!texGen.GenerateSectorTexture( sector,rect,flags,sectorDiffuseImage ))
					return false;			// maybe this should be handled better

				// M.M. to take a look at the result of the calculation
				/*				char str[80];
				sprintf(str,"c:\\temp\\tile%d_%d.bmp",x,y);
				CImageUtil::SaveBitmap(str,sectorDiffuseImage);
				*/
				m_cHeightmap.m_TerrainRGBTexture.SetSubImageRGBLayer(x*dwSectorResolution,y*dwSectorResolution,sectorDiffuseImage);
			}
	}

	// extract Detail LayerId info
	GetIEditor()->GetHeightmap()->CalcSurfaceTypes();
	/*
	// M.M. to take a look at the result of the calculation
	uint32 dwExtend = m_cHeightmap.m_TerrainRGBTexture.CalcMinRequiredTextureExtend();
	CImage img;
	img.Allocate(dwExtend,dwExtend);
	m_cHeightmap.m_TerrainRGBTexture.GetSubImageStretched(0,0,1,1,img);
	CImageUtil::SaveBitmap("C:\\temp\\res.bmp",img);
	*/
	// we no longer need the layer mask data
	for(it=m_layers.begin();it!=m_layers.end();++it)
	{
		CLayer *pLayer=*it;

		pLayer->ReleaseMask();
	}

	GetIEditor()->GetHeightmap()->UpdateEngineTerrain(false);

	m_bIsNewTerranTextureSystem=true;

	return true;
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::SetTerrainSize( int resolution,int unitSize )
{
	m_cHeightmap.Resize( resolution,resolution,unitSize );
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::ResetHeightMap()
{
	////////////////////////////////////////////////////////////////////////
	// Reset Heightmap
	////////////////////////////////////////////////////////////////////////
	m_cHeightmap.SetWaterLevel(16); // Default water level.
	m_cHeightmap.Resize(1024,1024,2 );
	m_cHeightmap.SetMaxHeight(1024);
}
//////////////////////////////////////////////////////////////////////////
void CTerrainManager::FreeHeightMapData()
{
	m_cHeightmap.m_TerrainRGBTexture.FreeData();
}
//////////////////////////////////////////////////////////////////////////
bool CTerrainManager::WouldHeightmapSaveSucceed()
{
	return m_cHeightmap.m_TerrainRGBTexture.WouldSaveSucceed();
}
//////////////////////////////////////////////////////////////////////////
CTerrainManager::CTerrainManager()
{

}
//////////////////////////////////////////////////////////////////////////
CTerrainManager::~CTerrainManager()
{

}
//////////////////////////////////////////////////////////////////////////
