#include "StdAfx.h"
#include "BrushBSPTree3D.h"
#include "BrushDesigner.h"

struct SDesignerBooleanStruct
{
	CBrushDesigner::RegionList regionList;
	_smart_ptr<CBrushBSPTree3D> pBSPTree;
};

SDesignerBooleanStruct GetBooleanStruct( const CBrushDesigner* pDesigner )
{
	SDesignerBooleanStruct sbs;
	pDesigner->GetRegionList(sbs.regionList);
	sbs.pBSPTree = new CBrushBSPTree3D(sbs.regionList);
	return sbs;
}

void CBrushDesigner::Union( CBrushDesigner* BDesigner )
{
	SDesignerBooleanStruct sbsA = GetBooleanStruct(this);
	SDesignerBooleanStruct sbsB = GetBooleanStruct(BDesigner);

	RegionList outRegionList;
	Clip( sbsB.pBSPTree, sbsA.regionList, BUtil::eCT_Negative, outRegionList, BUtil::eCO_Union0 );
	BDesigner->Clip( sbsA.pBSPTree, sbsB.regionList, BUtil::eCT_Negative, outRegionList, BUtil::eCO_Union1 );

	ResetFromList(outRegionList);
}

void CBrushDesigner::Subtract( CBrushDesigner* BDesigner )
{
	SDesignerBooleanStruct sbsA = GetBooleanStruct(this);
	SDesignerBooleanStruct sbsB = GetBooleanStruct(BDesigner);

	RegionList outRegionList;
	Clip( sbsB.pBSPTree, sbsA.regionList, BUtil::eCT_Negative, outRegionList, BUtil::eCO_Subtract );
	BDesigner->Clip( sbsA.pBSPTree, sbsB.regionList, BUtil::eCT_Positive, outRegionList, BUtil::eCO_Subtract );

	ResetFromList(outRegionList);
}

void CBrushDesigner::Intersect( CBrushDesigner* BDesigner )
{
	SDesignerBooleanStruct sbsA = GetBooleanStruct(this);
	SDesignerBooleanStruct sbsB = GetBooleanStruct(BDesigner);

	RegionList outRegionList;
	Clip( sbsB.pBSPTree, sbsA.regionList, BUtil::eCT_Positive, outRegionList, BUtil::eCO_Intersection0IncludingCoSame );
	BDesigner->Clip( sbsA.pBSPTree, sbsB.regionList, BUtil::eCT_Positive, outRegionList, BUtil::eCO_Intersection1IncludingCoDiff );

	ResetFromList(outRegionList);
}

void CBrushDesigner::ClipOutside( CBrushDesigner* BDesigner )
{
	SDesignerBooleanStruct sbsA = GetBooleanStruct(this);
	SDesignerBooleanStruct sbsB = GetBooleanStruct(BDesigner);

	RegionList outRegionList;
	Clip( sbsB.pBSPTree, sbsA.regionList, BUtil::eCT_Positive, outRegionList, BUtil::eCO_JustClip );

	ResetFromList(outRegionList);
}

void CBrushDesigner::ClipInside( CBrushDesigner* BDesigner )
{
	SDesignerBooleanStruct sbsA = GetBooleanStruct(this);
	SDesignerBooleanStruct sbsB = GetBooleanStruct(BDesigner);

	RegionList outRegionList;
	Clip( sbsB.pBSPTree, sbsA.regionList, BUtil::eCT_Negative, outRegionList, BUtil::eCO_JustClip );

	ResetFromList(outRegionList);
}

void CBrushDesigner::Clip( const CBrushBSPTree3D* pTree, RegionList& regionList, BUtil::EClipType cliptype, RegionList& outRegions, BUtil::EClipObjective clipObjective )
{
	if( pTree == NULL )
		return;

	for( int i = 0, iRegionSize(regionList.size()); i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion = regionList[i];

		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;

		CBrushBSPTree3D::SOutputRegions outputRegions;
		pTree->GetPartitions( regionList[i], outputRegions );

		std::vector<RegionList> validRegions;

		if( clipObjective == BUtil::eCO_JustClip )
		{
			if( cliptype == BUtil::eCT_Negative )
			{
				validRegions.push_back(outputRegions.posList);
				validRegions.push_back(outputRegions.coSameList);
			}
			else
			{
				validRegions.push_back(outputRegions.negList);
				validRegions.push_back(outputRegions.coDiffList);
			}
		}
		if( clipObjective == BUtil::eCO_Union0 )
		{
			if( cliptype == BUtil::eCT_Negative )
			{
				validRegions.push_back(outputRegions.posList);
				validRegions.push_back(outputRegions.coSameList);
			}
		}
		else if( clipObjective == BUtil::eCO_Union1 )
		{
			if( cliptype == BUtil::eCT_Negative )
				validRegions.push_back(outputRegions.posList);
		}
		else if( clipObjective == BUtil::eCO_Intersection0 || clipObjective == BUtil::eCO_Intersection0IncludingCoSame )
		{
			if( cliptype == BUtil::eCT_Positive)
			{
				validRegions.push_back(outputRegions.negList);
				if( clipObjective == BUtil::eCO_Intersection0IncludingCoSame )
					validRegions.push_back(outputRegions.coSameList);
			}
		}
		else if( clipObjective == BUtil::eCO_Intersection1 || clipObjective == BUtil::eCO_Intersection1IncludingCoDiff )
		{
			if( cliptype == BUtil::eCT_Positive)
			{
				validRegions.push_back(outputRegions.negList);
				if( clipObjective == BUtil::eCO_Intersection1IncludingCoDiff )
					validRegions.push_back(outputRegions.coDiffList);
			}
		}
		else if( clipObjective == BUtil::eCO_Subtract )
		{
			if( cliptype == BUtil::eCT_Positive)
			{
				validRegions.push_back(outputRegions.negList);
			}
			else if( cliptype == BUtil::eCT_Negative)
			{
				validRegions.push_back(outputRegions.posList);
				validRegions.push_back(outputRegions.coDiffList);
			}
		}

		for( int a = 0; a < validRegions.size(); ++a )
		{
			for( int k = 0, iValidRegionsize(validRegions[a].size()); k < iValidRegionsize; k++ )
			{
				if( clipObjective == BUtil::eCO_Subtract && cliptype == BUtil::eCT_Positive )
					validRegions[a][k]->Flip();
				DESIGNER_ASSERT( !validRegions[a][k]->IsOpen() );
				if( !validRegions[a][k]->IsOpen() )
					outRegions.push_back(validRegions[a][k]);
			}
		}
	}
}

bool CBrushDesigner::IsInside( const BrushVec3& vPos ) const
{
	SDesignerBooleanStruct sbsA = GetBooleanStruct(this);
	return sbsA.pBSPTree->IsInside(vPos);
}

std::vector<CBrushRegion::RegionPtr> CBrushDesigner::GetIntersectedParts( CBrushRegion::RegionPtr pRegion ) const
{
	RegionList outNegRegions;

	SDesignerBooleanStruct sbs = GetBooleanStruct(this);
	if( !sbs.pBSPTree )
		return outNegRegions;

	CBrushBSPTree3D::SOutputRegions outRegions;
	sbs.pBSPTree->GetPartitions(pRegion,outRegions);

	return outRegions.negList;
}
