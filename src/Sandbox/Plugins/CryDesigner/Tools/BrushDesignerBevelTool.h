#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerBevelTool.h
//  Created:     Oct/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerSelectTool.h"

class CBrushDesignerBevelTool : public CBrushDesignerSelectTool
{

public:

	CBrushDesignerBevelTool()
	{
		m_nMousePrevY = 0;
		m_fDelta = 0;
	}

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;
	bool OnKeyDown( CViewport *view,uint32 nKeycode,uint32 nRepCnt,uint32 nFlags ) override;
	void Display( DisplayContext &dc ) override;

	void Enter() override;
	void Leave() override;

private:

	typedef std::vector< std::pair<BrushVec3,BrushVec3> > MapBewteenSpreadedVertexAndApex;
	typedef std::map<int,std::vector<BrushEdge3D> > MapBetweenElementIndexAndEdges;
	typedef std::map<int,CBrushRegion::RegionPtr > MapBetweenElementIndexAndOrignialRegion;
	struct SMappingInfo
	{
		void Reset()
		{
			mapSpreadedVertex2Apex.clear();
			mapElementIdx2Edges.clear();
			mapElementIdx2OriginalRegion.clear();
			vertexSetToMakeRegion.clear();
		}
		MapBewteenSpreadedVertexAndApex mapSpreadedVertex2Apex;
		MapBetweenElementIndexAndEdges mapElementIdx2Edges;
		MapBetweenElementIndexAndOrignialRegion mapElementIdx2OriginalRegion;
		std::set<BrushVec3> vertexSetToMakeRegion;
	};

	// First - Region, Second - EdgeIndex
	typedef std::pair<CBrushRegion::RegionPtr,int> EdgeIdentifier;
	struct SResultForNextPhase
	{
		void Reset()
		{
			mapBetweenEdgeIdToApex.clear();
			mapBetweenEdgeIdToVertex.clear();
			middlePhaseEdgeRegions.clear();
			middlePhaseSideRegions.clear();
			middlePhaseBottomRegions.clear();
			middlePhaseApexRegions.clear();
		}
		std::map<EdgeIdentifier,BrushVec3> mapBetweenEdgeIdToApex;
		std::map<EdgeIdentifier,BrushVec3> mapBetweenEdgeIdToVertex;
		std::vector<CBrushRegion::RegionPtr> middlePhaseEdgeRegions;
		std::vector<CBrushRegion::RegionPtr> middlePhaseSideRegions;
		std::vector<CBrushRegion::RegionPtr> middlePhaseBottomRegions;
		std::vector<CBrushRegion::RegionPtr> middlePhaseApexRegions;
	};
	SResultForNextPhase m_ResultForSecondPhase;

	bool PP0_Initialize( bool bSpreadEdge = false );

	void PP0_SpreadEdges( int offset, bool bSpreadEdge = true );
	bool PP1_PushEdgesAndVerticesOut( SResultForNextPhase& outResultForNextPhase, SMappingInfo& outMappingInfo );
	void PP1_MakeEdgeRegions( const SMappingInfo& mappingInfo, SResultForNextPhase& outResultForNextPhase );
	void PP2_MapBetweenEdgeIdToApexPos(
		const SMappingInfo& mappingInfo,
		CBrushRegion::RegionPtr pEdgeRegion,
		const BrushEdge3D& sideEdge0,
		const BrushEdge3D& sideEdge1,
		SResultForNextPhase& outResultForNextPhase );
	void PP1_MakeApexRegions( const SMappingInfo& mappingInfo, SResultForNextPhase& outResultForNextPhase );
	void PP0_SubdivideSpreadedEdge( int nSubdivideNum );

	struct SInfoForSubdivingApexRegion
	{
		BrushEdge3D edge;
		std::vector< std::pair<BrushVec3,BrushVec3> > vIntermediate;
	};
	void PP1_SubdivideApexRegion( int nSubdivideNum, const std::vector<SInfoForSubdivingApexRegion>& infoForSubdividingApexRegionList );

private:

	int GetEdgeCountHavingVertexInElementList( const BrushVec3& vertex, const CBrushDesignerElementManager& elementList ) const;
	int FindCorrespondingEdge( const BrushEdge3D& e, const std::vector<SInfoForSubdivingApexRegion>& infoForSubdividingApexRegionList ) const;

	std::vector<CBrushRegion::RegionPtr> CreateFirstOddSubdividedApexRegions( const std::vector<const SInfoForSubdivingApexRegion*>& subdividedEdges );
	std::vector<CBrushRegion::RegionPtr> CreateFirstEvenSubdividedApexRegions( const std::vector<const SInfoForSubdivingApexRegion*>& subdividedEdges );

	enum EBevelMode
	{
		eBevelMode_Nothing,
		eBevelMode_Spread,
		eBevelMode_Divide,
	};
	EBevelMode m_BevelMode;

	_smart_ptr<CBrushDesigner> m_pOriginalDesigner;
	CBrushDesignerElementManager m_OriginalSelectedElements;

	std::vector<CBrushRegion::RegionPtr> m_OriginalRegions;

	int m_nMousePrevY;
	BrushFloat m_fDelta;
	int m_nDividedNumber;

};