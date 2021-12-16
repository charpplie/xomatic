#pragma once

#include "Core/BrushRegion.h"
#include "BrushDesignerSelectTool.h"
#include "Core/BrushDesignerDB.h"

class CBrushDesignerMovePipeline
{
public:

	CBrushDesignerMovePipeline(){}
	~CBrushDesignerMovePipeline(){}

	void TransformSelections( BUtil::SMainContext& mc, const BrushMatrix34& offsetTM );

	void SetQueryResultsFromSelectedElements( const CBrushDesignerElementManager& selectedElements );
	bool ExcutedAdditionPass() const { return m_bExecutedAdditionPass; }
	void SetExcutedAdditionPass( bool bExcuted ){ m_bExecutedAdditionPass = bExcuted; }
	void CreateOrganizedResultsAroundRegionFromQueryResults();
	void ComputeIntermediatePositionsBasedOnInitQueryResults( const BrushMatrix34& offsetTM );
	CBrushDesignerDB::Mark& GetMark( const CBrushDesignerSelectTool::QueryInput& qInput ){ return m_QueryResult[qInput.first].m_MarkList[qInput.second]; }
	bool VertexAdditionFirstPass();
	bool VertexAdditionSecondPass();
	bool SubdivisionPass();
	void TransformationPass();
	bool MergeCoplanarPass();
	void AssignIntermediatedPosToSelectedElements( CBrushDesignerElementManager& selectedElements );
	CBrushRegion::RegionPtr FindAdjacentRegion( CBrushRegion::RegionPtr pRegion, const BrushVec3& vPos, int& outAdjacentRegionIndex );
	void Initialize( const CBrushDesignerElementManager& elements );
	void InitializeIndependently( CBrushDesignerElementManager& elements );
	void End();
	bool GetAveragePos( BrushVec3& outAveragePos ) const;
	void SetDesigner( CBrushDesigner* pDesigner ) { m_pDesigner = pDesigner; }

private:

	void SnappedToMirrorPlane();
	CBrushDesigner* GetDesigner(){return m_pDesigner;}

	CBrushDesigner* m_pDesigner;
	std::set<CBrushRegion::RegionPtr> m_UnsubdividedRegions;
	std::map<CBrushRegion::RegionPtr,CBrushDesignerSelectTool::RegionList> m_SubdividedRegions;
	CBrushDesignerDB::QueryResult m_QueryResult;
	CBrushDesignerDB::QueryResult m_InitQueryResult;
	std::vector<BrushVec3> m_IntermediateTransQueryPos;
	CBrushDesignerSelectTool::OrganizedQueryResults m_OrganizedQueryResult;
	bool m_bExecutedAdditionPass;
};