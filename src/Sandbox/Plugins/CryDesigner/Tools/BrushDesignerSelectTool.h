#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSelectTool.h
//  Created:     July/1/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"
#include "Core/BrushDesignerElementManager.h"

class CBrushDesignerSelectTool : public CBrushDesignerBaseTool
{
public:

	CBrushDesignerSelectTool() : m_SelectionType(eST_Nothing), m_bHitGizmo(false)
	{
	}
	CBrushDesignerSelectTool( int pickFlag ) : m_SelectionType(eST_Nothing), m_bHitGizmo(false)
	{
		m_nPickFlag = pickFlag;
	}

	virtual ~CBrushDesignerSelectTool(){}

	virtual void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	virtual void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override;
	virtual void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;
	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) override;

	virtual void Display( DisplayContext &dc ) override;
	virtual void Enter() override;

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

	void SelectAllElements();	

	void AddPickFlag( int nFlag ){ m_nPickFlag |= nFlag; }
	void SetPickFlag( int nFlag ){ m_nPickFlag = nFlag; }
	void RemovePickFlag( int nFlag ){ m_nPickFlag &= ~nFlag; }
	bool CheckPickFlag( int nFlag ){ return m_nPickFlag & nFlag ? true : false; }
	int GetPickFlag(){ return m_nPickFlag; }

	CString GetStatusText() const override;

	static void UpdateSelectionMeshFromSelectedElementList( BUtil::SMainContext& mc );
	static void ResetDesignerRejectedEdgeList( BUtil::SMainContext& mc );

public:

	// first - the index in query result.
	// second - the index in a mark set
	typedef std::pair<int,int> QueryInput;
	typedef std::vector<QueryInput> QueryInputs;
	// first - region index, second - query results of a region
	typedef std::map<CBrushRegion::RegionPtr,QueryInputs> OrganizedQueryResults;
	typedef std::vector<CBrushRegion::RegionPtr> RegionList;

	static OrganizedQueryResults CreateOrganizedResultsAroundRegionFromQueryResults( const CBrushDesignerDB::QueryResult& queryResult );	

protected:
	

	enum eSelectionType
	{
		eST_NormalSelection,
		eST_RectangleSelection,
		eST_LikelyToMoveSelection,
		eST_MoveSelection,
		eST_Nothing
	};
	void UpdateCursor( CViewport* view, bool bPickingElements );

	eSelectionType m_SelectionType;
	CPoint m_MouseDownPos;

	CBrushDesignerElementManager m_InitialSelectionElementsInRectangleSel;	

	int m_nPickFlag;
	bool m_bHitGizmo;
	BrushVec3 m_PickedPosAsLMBDown;
};