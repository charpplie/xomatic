////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   VisualBudgetSystem.h
//  Created:     4/08/2009 by Paulo Zaffari.
//  Description: This class implements the data gathering and rendering of
//							 the visual budget system.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef VisualBudgetSystem_h__
#define VisualBudgetSystem_h__

#pragma once

//////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////
#include "./Include/IRenderListener.h"

#include "ResourceCollector.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class ICrySizer;
class CAssetBrowserDialog;

typedef std::vector<AABB> AABBList;

class CVisualBudgetSystem:public IEditorNotifyListener,public IRenderListener
{
	//////////////////////////////////////////////////////////////////////////
	//	Types and typedefs
	//////////////////////////////////////////////////////////////////////////
	public:
	protected:
	private:

	//////////////////////////////////////////////////////////////////////////
	//	Methods
	//////////////////////////////////////////////////////////////////////////
	public:
		//////////////////////////////////////////////////////////////////////////
		// From IEditorNotifyListener
		//////////////////////////////////////////////////////////////////////////
		// We will be rendering during the idle notify.
		void OnEditorNotifyEvent( EEditorNotifyEvent event );
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// From IRenderListener
		//////////////////////////////////////////////////////////////////////////
		// We will be rendering during the idle notify.
		void Render(DisplayContext& rstDisplayContext);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// From IUnknown 
		//////////////////////////////////////////////////////////////////////////
		HRESULT STDMETHODCALLTYPE QueryInterface( const IID &riid, void **ppvObj ) 
		{ 
			if(riid == __uuidof(IRenderListener)/* && m_pIntegrator*/)
			{
				*ppvObj = this;
				return S_OK;
			}
			return E_NOINTERFACE ; 
		}
		ULONG STDMETHODCALLTYPE AddRef() { return ++m_ref; };
		ULONG STDMETHODCALLTYPE Release() 
		{ 
			if((--m_ref) == 0)
			{
				FreeData();
				delete this;
				return 0; 
			}
			else
				return m_ref;
		}
		//////////////////////////////////////////////////////////////////////////

		static CVisualBudgetSystem& GetVisualBudgetSystem();

		bool					GetDetailedAnalysisStatus() const;
		void					SetDetailedAnalysisStatus(const bool bDetailedAnalysis);

		float					GetCellSize()	const { return m_fCellSize; }
		void					SetCellSize(float fCellSize);
		float					GetMinimumCellSize() const { return m_fMinimumCellSize; }

		bool					GetAccountTerrainStatus() const { return m_boAccountTerrain; }
		void					SetAccountTerrainStatus(const bool bAccountTerrain);

		bool					GetAccountMiscelaneousStatus() const;
		void					SetAccountMiscelaneousStatus(const bool bAccountMiscelaneous);

		bool					GetAccountBrushStatus() const;
		void					SetAccountBrushStatus(const bool bAccountBrush);

		bool					GetAccountVegetationStatus() const;
		void					SetAccountVegetationStatus(const bool bAccountVegetation);

		bool					GetAccountRoadsStatus() const;
		void					SetAccountRoadsStatus(const bool bAccountRoads);

		bool					GetAccountCloudsStatus() const;
		void					SetAccountCloudsStatus(const bool bAccountClouds);

		bool					GetAccountEntitiesStatus() const;
		void					SetAccountEntitiesStatus(const bool bAccountEntities);

		bool					GetAccountDecalsStatus() const;
		void					SetAccountDecalsStatus(const bool bAccountDecals);

		void					ClearDetailedAnalysisRegions();
		void					AddDetailedAnalysisRegion(const AABB& aabb);
		const AABBList& GetDetailedAnalysisRegions() const;
		void					SetCurrentRegion(const AABB& aabb);

		bool					GetSectorBoundingBoxFromPoint(const Vec3& crstDetailedAnalysisPoint,AABB& rstBoundingBox);

		void					GetResourceMemoryUsage(ICrySizer * pSizerGeometry,ICrySizer * pSizerTexture,const AABB& cstAABB);

		bool          CalculateMapResourceMemoryUsage();
		CResourceCollector * CalculateMapResourceMemoryUsage(const AABBList& AABBs);

		void SetAssetBrowserDialog(CAssetBrowserDialog * pAssetBrowserDialog) {m_pAssetBrowserDialog = pAssetBrowserDialog;}
		CAssetBrowserDialog * GetAssetBrowserDialog() { return m_pAssetBrowserDialog; }

		int		GetDisplayType() const 
		{ return m_displayType; }
		void	SetDisplayType(int displayType) 
		{ m_displayType = displayType; }
		
		enum ResourceType
		{
			RSRC_GEOMETRY=0,
			RSRC_TEXTURE,
			RSRC_BOTH,
		};
		ResourceType GetMeasuredResourceType() const 
		{ return m_resourceType; }
		void	SetMeasuredResourceType(ResourceType resourceType);

		void Update();

	protected:
		CVisualBudgetSystem();
		virtual ~CVisualBudgetSystem();

		void					RenderTerrainSquare(DisplayContext& dc,float x1,float y1,float x2,float y2,
																			ColorB&	rstColor);

		void FreeData();		

		void GetResourceUsageOfRenderNode( IRenderNode *pRenderNode, ICrySizer * pSizerGeometry, ICrySizer * pSizerTexture );

		void FindObjectsInRegions();
		void SortObjectsByResourceUsage(ResourceType resourceType);
		void RenderResourceUsagePerObject(DisplayContext& dc);

	private:

	//////////////////////////////////////////////////////////////////////////
	//	Data
	//////////////////////////////////////////////////////////////////////////
	public:
	protected:
		bool							m_boPauseRendering;
		float							m_fCellSize;
		float							m_fMinimumCellSize;

		bool							m_boIsInDetailAnalysisMode;
		// AABBs that represent the currently analyzed region.
		AABBList					m_detailedAnalysisRegions;
		// A region where the VisualBudgetTool cursor is at the moment
		AABB							m_currentRegion;
		typedef std::pair<CBaseObject*, float> ObjectAndUsage;
		typedef std::vector<ObjectAndUsage> ObjectList;
		//! A sorted list of objects in detail analysis regions
		ObjectList				m_detailedAnalysisObjects;

		bool							m_boAccountTerrain;
		bool							m_boAccountMiscelaneous;
		bool							m_boAccountBrushes;
		bool							m_boAccountVegetation;
		bool							m_boAccountRoads;
		bool							m_boAccountClouds;
		bool							m_boAccoundEntities;
		bool							m_boAccountDecals;

		int								m_displayType;

		AABBList					m_cstDrawingBoundingBoxes;
		AABBList					m_cstQueryBoundingBoxes;
		// (geometry memory, texture memory)
		std::vector<std::pair<size_t, size_t> >					m_cnUsedCellMemory;

		float												m_fHalfDrawingHeight;

		ICrySizer* m_pCrySizerGeometry;
		ICrySizer* m_pCrySizerTexture;
		CResourceCollector		m_ResourceCollector;

		CAssetBrowserDialog * m_pAssetBrowserDialog;

		ICVar *m_e_StreamCgfPoolSize;
		ICVar *m_r_TexturesStreamPoolSize;
		// A scaling factor used for leveling overall grading to that of the streaming system
		float m_budgetGradingScale;

		// Which resource type will be measured?
		ResourceType m_resourceType;

		//////////////////////////////////////////////////////////////////////////
		// For IUnknown 
		//////////////////////////////////////////////////////////////////////////
		ULONG							m_ref;
	private:
};

#endif // VisualBudgetSystem_h__