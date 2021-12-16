#include "StdAfx.h"
#include "BrushDesignerObjectModeTool.h"
#include "EditMode/ObjectMode.h"
#include "Core/BrushDesigner.h"
#include "ViewManager.h"
#include "BrushDesignerEditTool.h"

void CBrushDesignerObjectModeTool::Enter()
{
	__super::Enter();

	if( !m_pObjectMode )
		m_pObjectMode = new CObjectMode;

	if( GetDesigner() )
	{
		GetDesigner()->SetShelf(0);
		if( GetDesigner()->GetRegionSize() > 0 )
		{
			GetIEditor()->ShowTransformManipulator(false);
			CreateObjectGizmo();
		}
	}

	if( GetIEditor()->GetEditMode() == eEditModeRotateCircle )
		GetIEditor()->SetEditMode(eEditModeRotate);
}

void CBrushDesignerObjectModeTool::Leave()
{
	if( m_pObjectMode )
	{
		delete m_pObjectMode;
		m_pObjectMode = NULL;
	}

	DynArray<CBaseObject*> objects;
	GetIEditor()->GetObjectManager()->GetObjects(objects);
	for( int i = 0, iObjectCount(objects.size()); i < iObjectCount; ++i )
		objects[i]->ClearFlags(OBJFLAG_HIGHLIGHT);

	__super::Leave();
}

void CBrushDesignerObjectModeTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch(event)
	{
	case eNotify_OnSelectionChange:
		{
			CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection();
			if( pGroup->GetCount() == 1 && pGroup->GetObject(0) == GetBaseObject() )
				m_bSelectedAnother = false;
			else 
				m_bSelectedAnother = true;
		}
		break;
	}
}

void CBrushDesignerObjectModeTool::OnLButtonDown( CViewport *view, UINT nFlags, CPoint point )
{
	DESIGNER_ASSERT(m_pObjectMode);
	if( !m_pObjectMode )
		return;
	m_bSelectedAnother = false;
	m_pObjectMode->MouseCallback(view,eMouseLDown,point,nFlags);
}

void CBrushDesignerObjectModeTool::OnLButtonUp( CViewport *view, UINT nFlags, CPoint point )
{
	DESIGNER_ASSERT(m_pObjectMode);
	if( !m_pObjectMode )
		return;
	m_pObjectMode->MouseCallback(view,eMouseLUp,point,nFlags);
	if( m_bSelectedAnother )
	{
		GetIEditor()->GetObjectManager()->SetSkipUpdate(true);
		if( GetIEditor()->GetEditTool() )
			GetIEditor()->GetEditTool()->Finish();
		GetIEditor()->SetEditTool(NULL);
		GetIEditor()->GetObjectManager()->SetSkipUpdate(false);
	}
}

void CBrushDesignerObjectModeTool::OnLButtonDblClk( CViewport *view,UINT nFlags,CPoint point )
{
	DESIGNER_ASSERT(m_pObjectMode);
	if( !m_pObjectMode )
		return;
	m_pObjectMode->MouseCallback(view,eMouseLDblClick,point,nFlags);
}

void CBrushDesignerObjectModeTool::OnRButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	DESIGNER_ASSERT(m_pObjectMode);
	if( !m_pObjectMode )
		return;
	m_pObjectMode->MouseCallback(view,eMouseRDown,point,nFlags);
}

void CBrushDesignerObjectModeTool::OnRButtonUp( CViewport *view,UINT nFlags,CPoint point )
{
	DESIGNER_ASSERT(m_pObjectMode);
	if( !m_pObjectMode )
		return;
	m_pObjectMode->MouseCallback(view,eMouseRUp,point,nFlags);
}

void CBrushDesignerObjectModeTool::OnMButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	DESIGNER_ASSERT(m_pObjectMode);
	if( !m_pObjectMode )
		return;
	m_pObjectMode->MouseCallback(view,eMouseMDown,point,nFlags);
}

void CBrushDesignerObjectModeTool::OnMouseMove( CViewport *view, UINT nFlags, CPoint point )
{
	DESIGNER_ASSERT(m_pObjectMode);
	if( !m_pObjectMode )
		return;
	m_pObjectMode->MouseCallback(view,eMouseMove,point,nFlags);
}

bool CBrushDesignerObjectModeTool::OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags )
{
	if( nChar == VK_ESCAPE )
	{
		if( GetDesigner() )
		{
			GetEditTool()->DeleteObjectIfEmpty();
			GetBaseObject()->EndEditParams(GetIEditor());
			GetEditTool()->SetPanelOwner(NULL);
		}
		GetEditTool()->SetBaseObject(NULL);
		GetIEditor()->GetObjectManager()->ClearSelection();
		return true;
	}

	DESIGNER_ASSERT(m_pObjectMode);
	if( !m_pObjectMode )
		return true;

	m_pObjectMode->OnKeyDown(view,nChar,nRepCnt,nFlags);
	return true;
}

void CBrushDesignerObjectModeTool::OnManipulatorDrag( CViewport *view,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value )
{
	DESIGNER_ASSERT(m_pObjectMode);
	if( !m_pObjectMode )
		return;
	return m_pObjectMode->OnManipulatorDrag(view,pManipulator,p0,p1,value);
}

void CBrushDesignerObjectModeTool::Display( DisplayContext &dc )
{
	__super::Display(dc);
	DESIGNER_ASSERT(m_pObjectMode);
	if( !m_pObjectMode )
		return;
	Matrix34 tm = dc.GetMatrix();
	dc.PopMatrix();
	m_pObjectMode->Display(dc);
	dc.PushMatrix(tm);
}