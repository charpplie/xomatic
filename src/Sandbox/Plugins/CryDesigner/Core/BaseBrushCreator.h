#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BaseBrushCreator.h
//  Created:     June/14/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BaseBrush.h"
#include "BrushDesigner.h"

class CBaseBrushCreator : public CRefCountBase
{
public:

	CBaseBrushCreator( CBaseObject* pBaseObject, CBaseBrush* pBrush, CBrushDesigner* pDesigner ) :
	m_pBaseObject(pBaseObject),
	m_pBrush(pBrush),
	m_pDesigner(pDesigner)
	{
		DESIGNER_ASSERT( m_pBaseObject && m_pBrush && m_pDesigner );
	}

	bool CreateBrush( const AABB &bbox,BUtil::ESolidBrushCreateType createType,int numSides );
	void Display( DisplayContext &dc );

	CBaseObject* GetBaseObject() const;
	CBrushDesigner* GetDesigner() const {	return m_pDesigner;	}
	CBaseBrush* GetBrush() const {	return m_pBrush; }

	enum ECreatorEvent
	{
		eCreatorEvent_Start,
		eCreatorEvent_End,
		eCreatorEvent_CreateAfterEnd
	};

	virtual void OnEvent( ECreatorEvent event );
	virtual void SetPos( const Vec3& pos );
	virtual void Reset( const std::vector<CBrushRegion::RegionPtr>& regionList, bool bUnionOp = false );
	virtual void Attach( const std::vector<CBrushRegion::RegionPtr>& regionList, bool bUnionOp = false );

private:

	_smart_ptr<CBaseObject> m_pBaseObject;
	_smart_ptr<CBaseBrush> m_pBrush;
	_smart_ptr<CBrushDesigner> m_pDesigner;
};
