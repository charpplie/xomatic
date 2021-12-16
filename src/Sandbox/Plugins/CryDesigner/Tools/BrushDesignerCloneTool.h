#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerCloneTool.h
//  Created:     June/6/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"
#include "Objects/DesignerBrushObject.h"

namespace BUtil
{
	enum EArrangeType
	{
		eArrangeType_Array,
		eArrangeType_Circle
	};

	static const char* kDefaultPlacement = "Divide";
	static const int kDefaultNumberOfClone = 5;
}

class CBrushDesignerCloneTool : public CBrushDesignerBaseTool
{
public:

	CBrushDesignerCloneTool( BUtil::EArrangeType arrangeType ) : CBrushDesignerBaseTool()
	{
		m_ArrangeType = arrangeType;
		m_bSuspendedUndo = false;
	}

	void BeginEditParams() override;
	void EndEditParams() override;

	void Enter() override;
	void Leave() override;

	void Display( struct DisplayContext& dc ) override;

	void OnLButtonDown( CViewport *view, UINT nFlags, CPoint point ) override;
	void OnMouseMove( CViewport *view, UINT nFlags, CPoint point ) override;
	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

	BUtil::EArrangeType GetArrangeType() const { return m_ArrangeType; }

	void Update()
	{
		UpdateCloneList();
		UpdateClonePositions();
	}

private:

	void FreezeClones();
	void DeleteClones();
	
	void SetPivotToObject( CBaseObject* pObj, const Vec3& pos );
	Vec3 GetCenterBottom( const AABB& aabb ) const;
	void UpdateClonePositions()
	{
		if( m_ArrangeType == BUtil::eArrangeType_Array )
			UpdateClonePositionsAlongLine();
		else
			UpdateClonePositionsAlongCircle();
	}
	void UpdateCloneList();

	void UpdateClonePositionsAlongCircle();
	void UpdateClonePositionsAlongLine();

	typedef _smart_ptr<CDesignerBrushObject> BrushDesignerObjectPtr;

	std::vector<BrushDesignerObjectPtr> m_ClonedObjects;
	BrushDesignerObjectPtr m_SelectedObject;

	BrushPlane m_Plane;
	Vec3 m_vStartPos;
	Vec3 m_vPickedPos;
	BUtil::EArrangeType m_ArrangeType;
	float m_fRadius;
	bool m_bSuspendedUndo;
};