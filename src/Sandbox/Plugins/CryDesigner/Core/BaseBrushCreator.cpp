#include "StdAfx.h"
#include "BaseBrushCreator.h"
#include "BrushPrimitive.h"
#include "Tools/BrushDesignerBaseTool.h"

void CBaseBrushCreator::OnEvent( ECreatorEvent event )
{
	switch(event)
	{
		case eCreatorEvent_End:
			if( m_pDesigner )
				m_pDesigner->ResetDB(BUtil::eDBRF_ALL);
			break;
		case eCreatorEvent_CreateAfterEnd:
			break;
	}
}

void CBaseBrushCreator::Reset( const std::vector<CBrushRegion::RegionPtr>& regionList, bool bUnionOp )
{
	DESIGNER_ASSERT(m_pDesigner);
	if( !m_pDesigner )
		return;
	m_pDesigner->Clear();
	Attach(regionList, bUnionOp);
}

void CBaseBrushCreator::Attach( const std::vector<CBrushRegion::RegionPtr>& regionList, bool bUnionOp )
{
	DESIGNER_ASSERT(m_pDesigner && m_pBrush && m_pBaseObject);
	if( !m_pDesigner || !m_pBrush || !m_pBaseObject )
		return;

	for( int i = 0, iRegionCount(regionList.size()); i < iRegionCount; ++i )
		m_pDesigner->AddRegion(regionList[i],bUnionOp ? CBrushDesigner::eOpType_Union : CBrushDesigner::eOpType_Add);

	m_pBrush->Update(m_pBaseObject, m_pDesigner);
}

void CBaseBrushCreator::SetPos( const Vec3& pos )
{
	DESIGNER_ASSERT(m_pBaseObject);
	m_pBaseObject->SetPos(pos);
}

bool CBaseBrushCreator::CreateBrush( const AABB &bbox,BUtil::ESolidBrushCreateType createType,int numSides )
{
	CBrushPrimitive brushPrimitive(this);

	const BrushVec3& mins(bbox.min);
	const BrushVec3& maxs(bbox.max);

	switch(createType)
	{
	case BUtil::eBrushCreateType_Box:
		brushPrimitive.CreateBox( mins, maxs );
		break;
	case BUtil::eBrushCreateType_Cone: 
		brushPrimitive.CreateCone( mins, maxs, numSides );
		break;
	case BUtil::eBrushCreateType_Sphere:
		brushPrimitive.CreateSphere( mins, maxs, numSides );
		break;
	case BUtil::eBrushCreateType_Cylinder:
		brushPrimitive.CreateCylinder( mins, maxs, numSides );
		break;
	case BUtil::eBrushCreateType_Rectangle:
		brushPrimitive.CreateRectangle( mins, maxs );
		break;
	case BUtil::eBrushCreateType_Disc:
		brushPrimitive.CreateDisc( mins, maxs, numSides );
		break;
	}

	return true;
}

CBaseObject *CBaseBrushCreator::GetBaseObject() const
{
	return m_pBaseObject;
}

void CBaseBrushCreator::Display( DisplayContext &dc )
{
	if( !m_pDesigner || !m_pBaseObject )
		return;
	m_pBaseObject->DrawDimensionsImpl(dc,m_pDesigner->GetBoundBox());
}