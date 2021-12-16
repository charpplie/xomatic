////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   Roadpanel.cpp
//  Version:     v1.00
//  Created:     25/07/2005 by Sergiy Shaykin.
//  Compilers:   Visual C++.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Resource.h"
#include "RoadPanel.h"

#include "Viewport.h"
#include "Objects\\RoadObject.h"
#include "EditTool.h"

#define SHAPE_CLOSE_DISTANCE 0.8f

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
class CEditRoadObjectTool : public CEditTool
{
public:
	DECLARE_DYNCREATE(CEditRoadObjectTool)

	CEditRoadObjectTool();

	// Ovverides from CEditTool
	bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags );

	virtual void SetUserData( void *userData );
	
	virtual void BeginEditParams( IEditor *ie,int flags ) {};
	virtual void EndEditParams() {};

	virtual void Display( DisplayContext &dc ) {};
	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );
	virtual bool OnKeyUp( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) { return false; };

protected:
	virtual ~CEditRoadObjectTool();
	// Delete itself.
	void DeleteThis() { delete this; };

private:
	CRoadObject *m_road;
	int m_currPoint;
	bool m_modifying;
	CPoint m_mouseDownPos;
	Vec3 m_pointPos;
};

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CEditRoadObjectTool,CEditTool)

//////////////////////////////////////////////////////////////////////////
CEditRoadObjectTool::CEditRoadObjectTool()
{
	m_road = 0;
	m_currPoint = -1;
	m_modifying = false;
}

//////////////////////////////////////////////////////////////////////////
void CEditRoadObjectTool::SetUserData( void *userData )
{
	m_road = (CRoadObject*)userData;
	assert( m_road != 0 );

	// Modify Road undo.
	if (!CUndo::IsRecording())
	{
		CUndo ("Modify Road");
		m_road->StoreUndo( "Road Modify" );
	}

	m_road->SelectPoint(-1);
}

//////////////////////////////////////////////////////////////////////////
CEditRoadObjectTool::~CEditRoadObjectTool()
{
	if (m_road)
	{
		m_road->SelectPoint(-1);
	}
	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->CancelUndo();
}

bool CEditRoadObjectTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE)
	{
		GetIEditor()->SetEditTool(0);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEditRoadObjectTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	if (!m_road)
		return false;

	if (event == eMouseLDown)
	{
		m_mouseDownPos = point;
	}

	if (event == eMouseLDown || event == eMouseMove || event == eMouseLDblClick || event == eMouseLUp)
	{
		const Matrix34 &RoadTM = m_road->GetWorldTM();

		float dist;

		Vec3 raySrc,rayDir;
		view->ViewToWorldRay( point,raySrc,rayDir );

		// Find closest point on the Road.
		int p1,p2;
		Vec3 intPnt(0,0,0);
		m_road->GetNearestEdge( raySrc,rayDir,p1,p2,dist,intPnt );
		
		float fRoadCloseDistance = ROAD_CLOSE_DISTANCE * view->GetScreenScaleFactor(intPnt) * 0.01f;


		if ((flags & MK_CONTROL) && !m_modifying)
		{
			// If control we are editing edges..
			if (p1 >= 0 && p2 >= 0 && dist < fRoadCloseDistance+view->GetSelectionTolerance())
			{
				// Cursor near one of edited Road edges.
				view->ResetCursor();
				if (event == eMouseLDown)
				{
					view->CaptureMouse();
					m_modifying = true;
					GetIEditor()->BeginUndo();
					if (GetIEditor()->IsUndoRecording())
						m_road->StoreUndo( "Make Point" );

					// If last edge, insert at end.
					if (p2 == 0)
						p2 = -1;
					
					// Create new point between nearest edge.
					// Put intPnt into local space of Road.
					intPnt = RoadTM.GetInverted().TransformPoint(intPnt);

					int index = m_road->InsertPoint( p2,intPnt );
					m_road->SelectPoint( index );

					// Set construction plane for view.
					m_pointPos = RoadTM.TransformPoint( m_road->GetPoint(index) );
					Matrix34 tm;
					tm.SetIdentity();
					tm.SetTranslation( m_pointPos );
					view->SetConstructionMatrix( COORDS_LOCAL,tm );
				}
			}
			return true;
		}

		int index = m_road->GetNearestPoint( raySrc,rayDir,dist );
		if (index >= 0 && dist < fRoadCloseDistance+view->GetSelectionTolerance())
		{
			// Cursor near one of edited Road points.
			view->ResetCursor();
			if (event == eMouseLDown)
			{
				if (!m_modifying)
				{
					m_road->SelectPoint( index );
					m_modifying = true;
					view->CaptureMouse();
					GetIEditor()->BeginUndo();
					
					// Set construction plance for view.
					m_pointPos = RoadTM.TransformPoint( m_road->GetPoint(index) );
					Matrix34 tm;
					tm.SetIdentity();
					tm.SetTranslation( m_pointPos );
					view->SetConstructionMatrix( COORDS_LOCAL,tm );
				}
			}

			//GetNearestEdge

			if (event == eMouseLDblClick)
			{
				m_modifying = false;
				m_road->RemovePoint( index );
				m_road->SelectPoint( -1 );
			}
		}
		else
		{
			if (event == eMouseLDown)
			{
				m_road->SelectPoint( -1 );
			}
		}

		if (m_modifying && event == eMouseLUp)
		{
			// Accept changes.
			m_modifying = false;
			//m_road->SelectPoint( -1 );
			view->ReleaseMouse();
			m_road->CalcBBox();
			m_road->SetRoadSectors();

			if (GetIEditor()->IsUndoRecording())
				GetIEditor()->AcceptUndo( "Road Modify" );
		}

		if (m_modifying && event == eMouseMove)
		{
			// Move selected point point.
			Vec3 p1 = view->MapViewToCP(m_mouseDownPos);
			Vec3 p2 = view->MapViewToCP(point);
			Vec3 v = view->GetCPVector(p1,p2);

			if (m_road->GetSelectedPoint() >= 0)
			{
				Vec3 wp = m_pointPos;
				Vec3 newp = wp + v;
				if (GetIEditor()->GetAxisConstrains() == AXIS_TERRAIN)
				{
					// Keep height.
					newp = view->MapViewToCP(point);
					//float z = wp.z - GetIEditor()->GetTerrainElevation(wp.x,wp.y);
					//newp.z = GetIEditor()->GetTerrainElevation(newp.x,newp.y) + z;
					//newp.z = GetIEditor()->GetTerrainElevation(newp.x,newp.y) + ROAD_Z_OFFSET;
					newp.z += ROAD_Z_OFFSET;
				}

				if (newp.x != 0 && newp.y != 0 && newp.z != 0)
				{
					newp = view->SnapToGrid(newp);
					// Put newp into local space of Road.
					Matrix34 invRoadTM = RoadTM;
					invRoadTM.Invert();
					newp = invRoadTM.TransformPoint(newp);

					if (GetIEditor()->IsUndoRecording())
						m_road->StoreUndo( "Move Point" );
					m_road->SetPoint( m_road->GetSelectedPoint(),newp );
				}
			}
		}
		return true;
	}
	return false;
}





//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
class CSplitRoadObjectTool : public CEditTool
{
public:
	DECLARE_DYNCREATE(CSplitRoadObjectTool)

	CSplitRoadObjectTool();

	// Ovverides from CEditTool
	bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags );
	virtual void SetUserData( void *userData );
	virtual void BeginEditParams( IEditor *ie,int flags ) {};
	virtual void EndEditParams() {};
	virtual void Display( DisplayContext &dc ) {};
	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );

protected:
	virtual ~CSplitRoadObjectTool();
	void DeleteThis() { delete this; };

private:
	CRoadObject *m_road;
	int m_curPoint;
};

IMPLEMENT_DYNCREATE(CSplitRoadObjectTool,CEditTool)

//////////////////////////////////////////////////////////////////////////
CSplitRoadObjectTool::CSplitRoadObjectTool()
{
	m_road = 0;
	m_curPoint = -1;
}

//////////////////////////////////////////////////////////////////////////
void CSplitRoadObjectTool::SetUserData( void *userData )
{
	m_curPoint = -1;
	m_road = (CRoadObject*)userData;
	assert( m_road != 0 );

	// Modify road undo.
	if (!CUndo::IsRecording())
	{
		CUndo ("Modify Road");
		m_road->StoreUndo( "Road Modify" );
	}
}

//////////////////////////////////////////////////////////////////////////
CSplitRoadObjectTool::~CSplitRoadObjectTool()
{
	//if (m_road)
		//m_road->SetSplitPoint( -1,Vec3(0,0,0), -1 );
	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->CancelUndo();
}

//////////////////////////////////////////////////////////////////////////
bool CSplitRoadObjectTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE)
		GetIEditor()->SetEditTool(0);
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSplitRoadObjectTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	if (!m_road)
		return false;

	if (event == eMouseLDown || event == eMouseMove)
	{
		const Matrix34 &shapeTM = m_road->GetWorldTM();

		float dist;
		Vec3 raySrc,rayDir;
		view->ViewToWorldRay( point,raySrc,rayDir );

		// Find closest point on the shape.
		int p1,p2;
		Vec3 intPnt;
		m_road->GetNearestEdge( raySrc,rayDir,p1,p2,dist,intPnt );
		
		float fShapeCloseDistance = SHAPE_CLOSE_DISTANCE * view->GetScreenScaleFactor(intPnt) * 0.01f;

		// If control we are editing edges..
		if (p1 >= 0 && p2 > 0 && dist < fShapeCloseDistance+view->GetSelectionTolerance())
		{
			view->SetCurrentCursor( STD_CURSOR_HIT );
			// Put intPnt into local space of shape.
			intPnt = shapeTM.GetInverted().TransformPoint(intPnt);

			if (event == eMouseLDown)
			{
				GetIEditor()->BeginUndo();
				m_road->Split(p2, intPnt);
				if (GetIEditor()->IsUndoRecording())
					GetIEditor()->AcceptUndo( "Split road" );
				GetIEditor()->SetEditTool(0);
			}
		}
		else
			view->ResetCursor();

		return true;
	}
	return false;
}






//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CMergeRoadObjectsTool : public CEditTool
{
public:
	DECLARE_DYNCREATE(CMergeRoadObjectsTool)

	CMergeRoadObjectsTool();

	// Ovverides from CEditTool
	bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags );
	virtual void SetUserData( void *userData );
	virtual void BeginEditParams( IEditor *ie,int flags ) {};
	virtual void EndEditParams() {};
	virtual void Display( DisplayContext &dc ) {};
	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );

protected:
	virtual ~CMergeRoadObjectsTool();
	void DeleteThis() { delete this; };

	int m_curPoint;
	CRoadObject * m_road;

private:
};

IMPLEMENT_DYNCREATE(CMergeRoadObjectsTool,CEditTool)

//////////////////////////////////////////////////////////////////////////
CMergeRoadObjectsTool::CMergeRoadObjectsTool()
{
	m_curPoint = -1;
	m_road = 0;
}

//////////////////////////////////////////////////////////////////////////
void CMergeRoadObjectsTool::SetUserData( void *userData )
{
	m_road = (CRoadObject*)userData;
	assert( m_road != 0 );

	// Modify Road undo.
	if (!CUndo::IsRecording())
	{
		CUndo ("Road Merging");
		m_road->StoreUndo( "Road Merging" );
	}

	m_road->SelectPoint(-1);
}

//////////////////////////////////////////////////////////////////////////
CMergeRoadObjectsTool::~CMergeRoadObjectsTool()
{
	if(m_road)
		m_road->SetMergeIndex( -1 );
	m_road = 0;
	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->CancelUndo();
}

//////////////////////////////////////////////////////////////////////////
bool CMergeRoadObjectsTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE)
		GetIEditor()->SetEditTool(0);
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CMergeRoadObjectsTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	//return true;
	if (event == eMouseLDown || event == eMouseMove)
	{
		HitContext hc;
		hc.view = view;
		hc.point2d = point;
		view->ViewToWorldRay(point,hc.raySrc,hc.rayDir);
		if(!GetIEditor()->GetObjectManager()->HitTest(hc))
			return false;

		CBaseObject * pObj = hc.object;

		if(!pObj->IsKindOf(RUNTIME_CLASS(CRoadObject)))
			return false;

		CRoadObject * road = (CRoadObject*) pObj;

		if(!(m_curPoint==-1 && road==m_road || m_curPoint!=-1 && road!=m_road))
			return false;

		const Matrix34 &roadTM = road->GetWorldTM();

		float dist;
		Vec3 raySrc,rayDir;
		view->ViewToWorldRay( point,raySrc,rayDir );

		// Find closest point on the road.
		int p1,p2;
		Vec3 intPnt;
		road->GetNearestEdge( raySrc,rayDir,p1,p2,dist,intPnt );
		
		if (p1 < 0 || p2<=0)
			return false;

		float fShapeCloseDistance = SHAPE_CLOSE_DISTANCE * view->GetScreenScaleFactor(intPnt) * 0.01f;

		if (dist > fShapeCloseDistance+view->GetSelectionTolerance())
			return false;

		int cnt = road->GetPointCount();
		if(cnt < 2)
			return false;

		int p = road->GetNearestPoint( raySrc, rayDir, dist );
		if (p!=0 && p!=cnt-1)
			return false;

		Vec3 pnt = roadTM.TransformPoint(road->GetPoint(p));

		if(intPnt.GetDistance(pnt) > fShapeCloseDistance+view->GetSelectionTolerance())
			return false;

		view->SetCurrentCursor( STD_CURSOR_HIT );

		if (event == eMouseLDown)
		{
			if(m_curPoint==-1)
			{
				m_curPoint = p;
				m_road->SetMergeIndex(p);
			}
			else
			{
				GetIEditor()->BeginUndo();
				if(m_road)
				{
					road->SetMergeIndex(p);
					m_road->Merge(road);
				}
				if (GetIEditor()->IsUndoRecording())
					GetIEditor()->AcceptUndo( "Road Merging" );
				GetIEditor()->SetEditTool(0);
			}
		}

		return true;
	}

	return false;
}




//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// CRoadPanel dialog

IMPLEMENT_DYNAMIC(CRoadPanel, CDialog)

//////////////////////////////////////////////////////////////////////////
CRoadPanel::CRoadPanel( CWnd* pParent /* = NULL */)
	: CDialog(CRoadPanel::IDD, pParent)
{
	Create( IDD,AfxGetMainWnd() );
}

CRoadPanel::~CRoadPanel()
{
}

void CRoadPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ALIGN_HM, m_alignHmapButton);
	DDX_Control(pDX, IDC_EDIT_SHAPE, m_editRoadButton);
	DDX_Control(pDX, IDC_SPLITBUT, m_splitButton);
	DDX_Control(pDX, IDC_MERGE, m_mergeButton);
}


BEGIN_MESSAGE_MAP(CRoadPanel, CDialog)
	ON_BN_CLICKED(IDC_ALIGN_HM, OnAlignHeightMap)
	ON_BN_CLICKED(IDC_DEFAULT_WIDTH, OnDefaultWidth)
END_MESSAGE_MAP()


// CRoadPanel message handlers

BOOL CRoadPanel::OnInitDialog()
{
	__super::OnInitDialog();

	m_width.Create( this,IDC_WIDTH,CNumberCtrl::LEFTALIGN );
	m_width.SetRange(0.0f, 99999.0f);

	m_angle.Create( this,IDC_ANGLE );
	m_angle.SetRange(-25.0f, 25.0f);
	GetDlgItem(IDC_ANGLE)->EnableWindow( FALSE );
	GetDlgItem(IDC_WIDTH)->EnableWindow( FALSE );
	GetDlgItem(IDC_DEFAULT_WIDTH)->EnableWindow( FALSE );
	m_angle.SetUpdateCallback( functor(*this,&CRoadPanel::OnUpdateParams) );
	m_width.SetUpdateCallback( functor(*this,&CRoadPanel::OnUpdateParams) );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRoadPanel::SetRoad( CRoadObject * road )
{
	assert( road );
	m_road = road;

	if (road->GetPointCount() > 1)
	{
		m_editRoadButton.EnableWindow( TRUE );
		m_editRoadButton.SetToolClass( RUNTIME_CLASS(CEditRoadObjectTool),m_road );
	}
	else
		m_editRoadButton.EnableWindow( FALSE );

	CString str;
	str.Format( "Num Points: %d", road->GetPointCount() );
	if (GetDlgItem(IDC_NUM_POINTS))
		GetDlgItem(IDC_NUM_POINTS)->SetWindowText( str );

	if (road->GetPointCount() >= 2)
	{
		m_splitButton.SetToolClass( RUNTIME_CLASS(CSplitRoadObjectTool), m_road );
		m_splitButton.EnableWindow( TRUE );

		m_mergeButton.SetToolClass( RUNTIME_CLASS(CMergeRoadObjectsTool), m_road );
		m_mergeButton.EnableWindow( TRUE );
	}
	else
	{
		m_splitButton.EnableWindow( FALSE );
		m_mergeButton.EnableWindow( FALSE );
	}

}

//////////////////////////////////////////////////////////////////////////
void CRoadPanel::OnAlignHeightMap()
{
	m_road->AlignHeightMap();
}

//////////////////////////////////////////////////////////////////////////
void CRoadPanel::SelectPoint( int index )
{
	if(index < 0)
	{
		GetDlgItem(IDC_ANGLE)->EnableWindow( FALSE );
		GetDlgItem(IDC_WIDTH)->EnableWindow( FALSE );
		GetDlgItem(IDC_DEFAULT_WIDTH)->EnableWindow( FALSE );
		GetDlgItem(IDC_SELECTED_POINT)->SetWindowText("Selected Point: no selection");
	}
	else
	{
		GetDlgItem(IDC_ANGLE)->EnableWindow( TRUE );
		float val = m_road->GetPointAngle();
		m_angle.SetValue(val);

		GetDlgItem(IDC_DEFAULT_WIDTH)->EnableWindow( TRUE );

		bool isDefault = m_road->IsPointDefaultWidth();
		((CButton *)GetDlgItem(IDC_DEFAULT_WIDTH))->SetCheck( isDefault  );
		GetDlgItem(IDC_WIDTH)->EnableWindow( !isDefault );
		val = m_road->GetPointWidth();
		m_width.SetValue(val);

		char out[256];
		sprintf(out,"Selected Point: %d",index+1);
		GetDlgItem(IDC_SELECTED_POINT)->SetWindowText(out);
	}
	m_width.SetValue(m_road->GetPointWidth());
}

void CRoadPanel::OnUpdateParams( CNumberCtrl *ctrl )
{
	m_road->SetPointAngle(m_angle.GetValue());
	m_road->SetPointWidth(m_width.GetValue());
}

void CRoadPanel::OnDefaultWidth()
{
	BOOL isDefault = ((CButton *)GetDlgItem(IDC_DEFAULT_WIDTH))->GetCheck( );
	GetDlgItem(IDC_WIDTH)->EnableWindow( !isDefault );
	m_road->PointDafaultWidthIs(isDefault);
	m_width.SetValue(m_road->GetPointWidth());
}