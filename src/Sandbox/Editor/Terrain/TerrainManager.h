////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TerrainManager.h
//  Created:     30/3/2009 by Paulo Zaffari.
//  Description: Manages layer and heightfield data.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef TerrainManager_h__
#define TerrainManager_h__

#pragma once

//////////////////////////////////////////////////////////////////////////
// Dependencies
#include "..\Terrain\Heightmap.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
class CLayer;
class CSurfaceType;
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Class
class CTerrainManager
{
	//////////////////////////////////////////////////////////////////////////
	// Types & typedefs
	//////////////////////////////////////////////////////////////////////////
	public:
		typedef std::vector<_smart_ptr<CSurfaceType> >  TDSurfaceTypeContainer;
		typedef std::vector<CLayer*>										TDLayerContainer;

	protected:
	private:

	//////////////////////////////////////////////////////////////////////////
	// Methods
	//////////////////////////////////////////////////////////////////////////
	public:
		//////////////////////////////////////////////////////////////////////////
		// Singleton entry-point.
		//////////////////////////////////////////////////////////////////////////
		static CTerrainManager&	GetTerrainManager();


		//////////////////////////////////////////////////////////////////////////
		// Surface Types.
		//////////////////////////////////////////////////////////////////////////
		// Returns:
		//   can be 0 if the id does not exit
		CSurfaceType* GetSurfaceTypePtr( int i ) const { if(i >= 0 && i<m_surfaceTypes.size())return m_surfaceTypes[i]; return 0; }
		int  GetSurfaceTypeCount() const { return m_surfaceTypes.size(); }
		//! Find surface type by name, return -1 if name not found.
		int FindSurfaceType( const CString &name );
		void RemoveSurfaceType( int i );
		void RemoveSurfaceType( CSurfaceType *pSrfType );
		int  AddSurfaceType( CSurfaceType *srf );
		void RemoveAllSurfaceTypes();
		void SerializeSurfaceTypes( CXmlArchive &xmlAr, bool bUpdateEngineTerrain = true);
		void ConsolidateSurfaceTypes();

		/** Put surface types from Editor to game.
		*/
		void ReloadSurfaceTypes(bool bUpdateEngineTerrain = true);
		//////////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		// Layers
		int GetLayerCount() const { return m_layers.size(); };
		CLayer* GetLayer( int layer ) const { return m_layers[layer]; };
		//! Find layer by name.
		CLayer* FindLayer( const char *sLayerName ) const;
		CLayer* FindLayerByLayerId( const uint32 dwLayerId ) const;
		void SwapLayers( int layer1,int layer2 );
		void AddLayer( CLayer *layer ) { m_layers.push_back(layer); };
		void RemoveLayer( int layer );
		void RemoveLayer( CLayer *layer );
		void InvalidateLayers();
		void ClearLayers();
		void SerializeLayerSettings( CXmlArchive &xmlAr );
		void MarkUsedLayerIds( bool bFree[256] ) const;

		void CreateDefaultLayer();

		// slow
		// Returns:
		//   0xffffffff if the method failed (internal error)
		uint32 GetDetailIdLayerFromLayerId( const uint32 dwLayerId );

		// slow
		// Returns:
		//   0xffffffff if the method failed (internal error)
		uint32 GetLayerIdFromDetailLayerId( const uint32 dwDetailLayerId );

		// needed to convert from old layer structure to the new one
		bool ConvertLayersToRGBLayer();
		//////////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		// Heightmap
		CHeightmap&	GetHeightmap() { return m_cHeightmap; };

		void SetTerrainSize( int resolution,int unitSize );

		void ResetHeightMap();

		void FreeHeightMapData();

		bool WouldHeightmapSaveSucceed();
		//////////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		// Misc methods
		bool										IsNewTerranTextureSystem() { return m_bIsNewTerranTextureSystem; }
		void                    SerializeTerrain(CXmlArchive& xmlAr,char* szLevelPath=NULL,char* currentMissionName=NULL,bool bReloadEngineLevel=false);
		bool										ExportTerrainAsGeometry(const char *pszFileName, RECT rcExport);
		void										GetTerrainMemoryUsage( ICrySizer *pSizer );
		//////////////////////////////////////////////////////////////////////////

	protected:
		//////////////////////////////////////////////////////////////////////////
		// Constructor/destructor
		//////////////////////////////////////////////////////////////////////////
		CTerrainManager();
		~CTerrainManager();
		
	private:

	//////////////////////////////////////////////////////////////////////////
	// Data
	//////////////////////////////////////////////////////////////////////////
	public:
	protected:
		TDSurfaceTypeContainer				m_surfaceTypes;
		TDLayerContainer							m_layers;											// Terrain layers
		CHeightmap										m_cHeightmap;
		bool													m_bIsNewTerranTextureSystem;	// true=terrain texture is stored in RGB, false=terrain texture is stored in layers
	private:

};
//////////////////////////////////////////////////////////////////////////

#endif // TerrainManager_h__
