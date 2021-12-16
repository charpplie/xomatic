#include "StdAfx.h"
#include "SplinePanel.h"

#include "Viewport.h"
#include "Objects/SplineObject.h"
#include "Include/ITransformManipulator.h"
#include "MeasurementSystem/MeasurementSystem.h"



//////////////////////////////////////////////////////////////////////////
// CEditSplineObjectTool

class CEditSplineObjectTool : public CEditTool
{
public:
	DECLARE_DYNCREATE(CEditSplineObjectTool)

	CEditSplineObjectTool():
		m_pSpline(0),
		m_currPoint(-1),
		m_modifying(false),
		m_curCursor(STD_CURSOR_DEFAULT)
	{}

	// Ovverides from CEditTool
	bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags );
	void OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, CPoint& point0, CPoint& point1, const Vec3 &value) override;

	virtual void SetUserData( const char *key, void *userData );

	virtual void Display(DisplayContext &dc) {}
	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );

	bool IsNeedMoveTool() override { return true; }
	bool IsNeedToSkipPivotBoxForObjects()	override {	return true; }

protected:
	virtual ~CEditSplineObjectTool();
	void DeleteThis() { delete this; }

	void SelectPoint(int index);
	void SetCursor(EStdCursor cursor, bool bForce = false);

private:
	CSplineObject* m_pSpline;
	int m_currPoint;
	bool m_modifying;
	CPoint m_mouseDownPos;
	Vec3 m_pointPos;
	EStdCursor m_curCursor;
};

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CEditSplineObjectTool,CEditTool)


//////////////////////////////////////////////////////////////////////////
void CEditSplineObjectTool::SetUserData( const char *key, void *userData )
{
	m_pSpline = (CSplineObject*)userData;
	assert( m_pSpline != 0 );

	m_pSpline->SetEditMode(true);

	// Modify Spline undo.
	if (!CUndo::IsRecording())
	{
		CUndo ("Modify Spline");
		m_pSpline->StoreUndo("Spline Modify");
	}

	if (GetIEditor()->GetEditMode() == eEditModeSelect)
	{
		GetIEditor()->SetEditMode(eEditModeMove);
	}

	SelectPoint(-1);
}


//////////////////////////////////////////////////////////////////////////
CEditSplineObjectTool::~CEditSplineObjectTool()
{
	if (m_pSpline)
	{
		m_pSpline->SetEditMode(false);
		SelectPoint(-1);
	}
	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->CancelUndo();
}


//////////////////////////////////////////////////////////////////////////
bool CEditSplineObjectTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE)
	{
		GetIEditor()->SetEditTool(0);
	}
	else if(nChar == VK_DELETE)
	{
		int index = m_pSpline->GetSelectedPoint();
		if (index >= 0)
		{
			m_pSpline->SelectPoint(-1);
			m_pSpline->RemovePoint(index);
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
void CEditSplineObjectTool::SelectPoint(int index)
{
	if (index<0)
		SetCursor(STD_CURSOR_DEFAULT, true);

	if (!m_pSpline)
		return;

	m_pSpline->SelectPoint(index);
}


//////////////////////////////////////////////////////////////////////////
void CEditSplineObjectTool::OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, CPoint& point0, CPoint& point1, const Vec3 &value)
{
	// get world/local coordinate system setting.
	RefCoordSys coordSys = GetIEditor()->GetReferenceCoordSys();
	int editMode = GetIEditor()->GetEditMode();

	// get current axis constrains.
	if (editMode == eEditModeMove)
	{
		m_modifying = true;
		GetIEditor()->RestoreUndo();
		const Matrix34 &splineTM = m_pSpline->GetWorldTM();
		Matrix34 invSplineTM = splineTM;
		invSplineTM.Invert();

		int index = m_pSpline->GetSelectedPoint();
		Vec3 pos = m_pSpline->GetPoint(index);
		
		Vec3 wp = splineTM.TransformPoint(pos);
		Vec3 newPos = wp + value;

		if (!GetAsyncKeyState(VK_CONTROL) && pView->GetAxisConstrain() == AXIS_TERRAIN)
		{
			float height = wp.z - GetIEditor()->GetTerrainElevation(wp.x, wp.y);
			newPos.z = GetIEditor()->GetTerrainElevation(newPos.x, newPos.y) + height;
		}

		if (GetIEditor()->IsUndoRecording())
			m_pSpline->StoreUndo( "Move Point" );

		newPos = invSplineTM.TransformPoint(newPos);
		m_pSpline->SetPoint(m_pSpline->GetSelectedPoint(), newPos);
	}
}


//////////////////////////////////////////////////////////////////////////
bool CEditSplineObjectTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	if (!m_pSpline)
		return false;

	if (event == eMouseLDown)
	{
		m_mouseDownPos = point;
	}

	if (event == eMouseLDown || event == eMouseMove || event == eMouseLDblClick || event == eMouseLUp)
	{

		const Matrix34 &splineTM = m_pSpline->GetWorldTM();

		Vec3 raySrc,rayDir;
		view->ViewToWorldRay( point,raySrc,rayDir );

		// Find closest point on the spline.
		int p1,p2;
		float dist;
		Vec3 intPnt(0,0,0);
		m_pSpline->GetNearestEdge(raySrc, rayDir, p1, p2, dist, intPnt);

		float fSplineCloseDistance = kSplinePointSelectionRadius * view->GetScreenScaleFactor(intPnt) * 0.01f;


		if ((flags & MK_CONTROL) && !m_modifying)
		{
			// If control we are editing edges..
			if (p1 >= 0 && p2 >= 0 && dist < fSplineCloseDistance+view->GetSelectionTolerance())
			{
				// Cursor near one of edited Spline edges.
				view->ResetCursor();
				if (event == eMouseLDown)
				{
					view->CaptureMouse();
					m_modifying = true;
					GetIEditor()->BeginUndo();
					if (GetIEditor()->IsUndoRecording())
						m_pSpline->StoreUndo( "Make Point" );

					// If last edge, insert at end.
					if (p2 == 0)
						p2 = -1;

					// Create new point between nearest edge.
					// Put intPnt into local space of Spline.
					intPnt = splineTM.GetInverted().TransformPoint(intPnt);

					int index = m_pSpline->InsertPoint( p2,intPnt );
					SelectPoint(index);

					// Set construction plane for view.
					m_pointPos = splineTM.TransformPoint( m_pSpline->GetPoint(index) );
					Matrix34 tm;
					tm.SetIdentity();
					tm.SetTranslation( m_pointPos );
					view->SetConstructionMatrix( COORDS_LOCAL,tm );
				}
			}
			return true;
		}

		int index = m_pSpline->GetNearestPoint( raySrc,rayDir,dist );
		if (index >= 0 && dist < fSplineCloseDistance+view->GetSelectionTolerance())
		{
			// Cursor near one of edited Spline points.
			SetCursor(GetIEditor()->GetEditMode() ? STD_CURSOR_MOVE : STD_CURSOR_HIT);

			if (event == eMouseLDown)
			{
				if (!m_modifying)
				{				
					if (CMeasurementSystem::GetMeasurementSystem().ProcessedLButtonClick(index))
						return false;

					SelectPoint(index);
					m_modifying = true;
					view->CaptureMouse();
					GetIEditor()->BeginUndo();

					// Set construction plance for view.
					m_pointPos = splineTM.TransformPoint( m_pSpline->GetPoint(index) );
					Matrix34 tm;
					tm.SetIdentity();
					tm.SetTranslation( m_pointPos );
					view->SetConstructionMatrix( COORDS_LOCAL,tm );
				}
			}

			if (event == eMouseLDblClick)
			{
				if ( CMeasurementSystem::GetMeasurementSystem().ProcessedDblButtonClick(m_pSpline->GetPointCount()) )
					return false;

				m_modifying = false;
				m_pSpline->RemovePoint( index );
				SelectPoint(-1);
			}
		}
		else
		{
			SetCursor(STD_CURSOR_DEFAULT);

			if (event == eMouseLDown)
			{
				SelectPoint(-1);
			}
		}

		if (m_modifying && event == eMouseLUp)
		{
			// Accept changes.
			m_modifying = false;
			view->ReleaseMouse();
			m_pSpline->CalcBBox(); // TODO: Avoid and make CalcBBox() private
			m_pSpline->OnUpdate();

			if (GetIEditor()->IsUndoRecording())
				GetIEditor()->AcceptUndo( "Spline Modify" );
		}

		return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
void CEditSplineObjectTool::SetCursor(EStdCursor cursor, bool bForce)
{
	CViewport* pViewport = GetIEditor()->GetActiveView();
	if ((m_curCursor!=cursor || bForce) && pViewport)
	{
		m_curCursor=cursor;
		pViewport->SetCurrentCursor(m_curCursor);
	}
}



//////////////////////////////////////////////////////////////////////////
// CSplitSplineObjectTool

class CSplitSplineObjectTool : public CEditTool
{
public:
	DECLARE_DYNCREATE(CSplitSplineObjectTool)

	CSplitSplineObjectTool();

	// Ovverides from CEditTool
	bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags );
	virtual void SetUserData( const char *key, void *userData );
	virtual void Display( DisplayContext &dc ) {};
	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );

protected:
	virtual ~CSplitSplineObjectTool();
	void DeleteThis() { delete this; };

private:
	CSplineObject *m_pSpline;
	int m_curPoint;
};

IMPLEMENT_DYNCREATE(CSplitSplineObjectTool,CEditTool)

//////////////////////////////////////////////////////////////////////////
CSplitSplineObjectTool::CSplitSplineObjectTool()
{
	m_pSpline = 0;
	m_curPoint = -1;
}

//////////////////////////////////////////////////////////////////////////
void CSplitSplineObjectTool::SetUserData( const char *key, void *userData )
{
	m_curPoint = -1;
	m_pSpline = (CSplineObject*)userData;
	assert( m_pSpline != 0 );

	// Modify Spline undo.
	if (!CUndo::IsRecording())
	{
		CUndo ("Modify Spline");
		m_pSpline->StoreUndo( "Spline Modify" );
	}
}

//////////////////////////////////////////////////////////////////////////
CSplitSplineObjectTool::~CSplitSplineObjectTool()
{
	//if (m_pSpline)
	//m_pSpline->SetSplitPoint( -1,Vec3(0,0,0), -1 );
	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->CancelUndo();
}

//////////////////////////////////////////////////////////////////////////
bool CSplitSplineObjectTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE)
		GetIEditor()->SetEditTool(0);
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSplitSplineObjectTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	if (!m_pSpline)
		return false;

	if (event == eMouseLDown || event == eMouseMove)
	{
		const Matrix34 &shapeTM = m_pSpline->GetWorldTM();

		float dist;
		Vec3 raySrc,rayDir;
		view->ViewToWorldRay( point,raySrc,rayDir );

		// Find closest point on the shape.
		int p1,p2;
		Vec3 intPnt;
		m_pSpline->GetNearestEdge( raySrc,rayDir,p1,p2,dist,intPnt );

		float fShapeCloseDistance = kSplinePointSelectionRadius * view->GetScreenScaleFactor(intPnt) * 0.01f;

		// If control we are editing edges..
		if (p1 >= 0 && p2 > 0 && dist < fShapeCloseDistance+view->GetSelectionTolerance())
		{
			view->SetCurrentCursor( STD_CURSOR_HIT );
			// Put intPnt into local space of shape.
			intPnt = shapeTM.GetInverted().TransformPoint(intPnt);

			if (event == eMouseLDown)
			{
				GetIEditor()->BeginUndo();
				m_pSpline->Split(p2, intPnt);
				if (GetIEditor()->IsUndoRecording())
					GetIEditor()->AcceptUndo( "Split Spline" );
				GetIEditor()->SetEditTool(0);
			}
		}
		else
		{
			view->ResetCursor();
		}

		return true;
	}
	return false;
}





//////////////////////////////////////////////////////////////////////////
// CMergeSplineObjectsTool

class CMergeSplineObjectsTool : public CEditTool
{
public:
	DECLARE_DYNCREATE(CMergeSplineObjectsTool)

	CMergeSplineObjectsTool();

	// Ovverides from CEditTool
	bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags );
	virtual void SetUserData( const char *key, void *userData );
	virtual void Display( DisplayContext &dc ) {};
	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );

protected:
	virtual ~CMergeSplineObjectsTool();
	void DeleteThis() { delete this; };

	int m_curPoint;
	CSplineObject * m_pSpline;

private:
};

IMPLEMENT_DYNCREATE(CMergeSplineObjectsTool,CEditTool)

//////////////////////////////////////////////////////////////////////////
CMergeSplineObjectsTool::CMergeSplineObjectsTool()
{
	m_curPoint = -1;
	m_pSpline = 0;
}

//////////////////////////////////////////////////////////////////////////
void CMergeSplineObjectsTool::SetUserData( const char *key, void *userData )
{
	m_pSpline = (CSplineObject*)userData;
	assert( m_pSpline != 0 );

	// Modify Spline undo.
	if (!CUndo::IsRecording())
	{
		CUndo ("Spline Merging");
		m_pSpline->StoreUndo( "Spline Merging" );
	}

	m_pSpline->SelectPoint(-1);
}

//////////////////////////////////////////////////////////////////////////
CMergeSplineObjectsTool::~CMergeSplineObjectsTool()
{
	if(m_pSpline)
		m_pSpline->SetMergeIndex( -1 );
	m_pSpline = 0;
	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->CancelUndo();
}

//////////////////////////////////////////////////////////////////////////
bool CMergeSplineObjectsTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE)
		GetIEditor()->SetEditTool(0);
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CMergeSplineObjectsTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
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

		if(!pObj->IsKindOf(RUNTIME_CLASS(CSplineObject)))
			return false;

		CSplineObject* pSpline = (CSplineObject*) pObj;

		if(!(m_curPoint==-1 && pSpline==m_pSpline || m_curPoint!=-1 && pSpline!=m_pSpline))
			return false;

		const Matrix34 &splineTM = pSpline->GetWorldTM();

		float dist;
		Vec3 raySrc,rayDir;
		view->ViewToWorldRay( point,raySrc,rayDir );

		// Find closest point on the Spline.
		int p1,p2;
		Vec3 intPnt;
		pSpline->GetNearestEdge( raySrc,rayDir,p1,p2,dist,intPnt );

		if (p1 < 0 || p2<=0)
			return false;

		float fShapeCloseDistance = kSplinePointSelectionRadius * view->GetScreenScaleFactor(intPnt) * 0.01f;

		if (dist > fShapeCloseDistance+view->GetSelectionTolerance())
			return false;

		int cnt = pSpline->GetPointCount();
		if(cnt < 2)
			return false;

		int p = pSpline->GetNearestPoint( raySrc, rayDir, dist );
		if (p!=0 && p!=cnt-1)
			return false;

		Vec3 pnt = splineTM.TransformPoint(pSpline->GetPoint(p));

		if(intPnt.GetDistance(pnt) > fShapeCloseDistance+view->GetSelectionTolerance())
			return false;

		view->SetCurrentCursor( STD_CURSOR_HIT );

		if (event == eMouseLDown)
		{
			if(m_curPoint==-1)
			{
				m_curPoint = p;
				m_pSpline->SetMergeIndex(p);
			}
			else
			{
				GetIEditor()->BeginUndo();
				if(m_pSpline)
				{
					pSpline->SetMergeIndex(p);
					m_pSpline->Merge(pSpline);
				}
				if (GetIEditor()->IsUndoRecording())
					GetIEditor()->AcceptUndo( "Spline Merging" );
				GetIEditor()->SetEditTool(0);
			}
		}

		return true;
	}

	return false;
}





//////////////////////////////////////////////////////////////////////////
// CSplinePanel

IMPLEMENT_DYNAMIC(CSplinePanel, CDialog)

//////////////////////////////////////////////////////////////////////////
CSplinePanel::CSplinePanel( CWnd* pParent /* = NULL */)
: CDialog(CSplinePanel::IDD, pParent)
{
	Create( IDD,AfxGetMainWnd() );
}

CSplinePanel::~CSplinePanel()
{
	CMeasurementSystem::GetMeasurementSystem().ShutdownMeasurementSystem();
}

void CSplinePanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_SHAPE, m_editSplineButton);
	DDX_Control(pDX, IDC_SPLITBUT, m_splitButton);
	DDX_Control(pDX, IDC_MERGE, m_mergeButton);
}


BEGIN_MESSAGE_MAP(CSplinePanel, CDialog)
	ON_BN_CLICKED(IDC_DEFAULT_WIDTH, OnDefaultWidth)
END_MESSAGE_MAP()


// CSplinePanel message handlers

BOOL CSplinePanel::OnInitDialog()
{
	__super::OnInitDialog();

	m_width.Create( this,IDC_WIDTH,CNumberCtrl::LEFTALIGN );
	m_width.SetRange(0.0f, 99999.0f);

	m_angle.Create( this,IDC_ANGLE );
	GetDlgItem(IDC_ANGLE)->EnableWindow( FALSE );
	GetDlgItem(IDC_WIDTH)->EnableWindow( FALSE );
	GetDlgItem(IDC_DEFAULT_WIDTH)->EnableWindow( FALSE );
	m_angle.SetUpdateCallback( functor(*this,&CSplinePanel::OnUpdateParams) );
	m_width.SetUpdateCallback( functor(*this,&CSplinePanel::OnUpdateParams) );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSplinePanel::SetSpline(CSplineObject* pSpline)
{
	assert(pSpline);
	m_pSpline = pSpline;
	m_angle.SetRange(-m_pSpline->GetAngleRange(), m_pSpline->GetAngleRange());
	
	Update();
}


//////////////////////////////////////////////////////////////////////////
void CSplinePanel::Update()
{
	if (m_pSpline->GetPointCount() > 1)
	{
		m_editSplineButton.EnableWindow(TRUE);
		m_editSplineButton.SetToolClass(RUNTIME_CLASS(CEditSplineObjectTool),"object", m_pSpline);
	}
	else
	{
		m_editSplineButton.EnableWindow( FALSE );
	}

	CString str;
	str.Format( "Num Points: %d", m_pSpline->GetPointCount() );
	if (GetDlgItem(IDC_NUM_POINTS))
		GetDlgItem(IDC_NUM_POINTS)->SetWindowText( str );

	if (m_pSpline->GetPointCount() >= 2)
	{
		m_splitButton.SetToolClass(RUNTIME_CLASS(CSplitSplineObjectTool),"object", m_pSpline);
		m_splitButton.EnableWindow(TRUE);

		m_mergeButton.SetToolClass(RUNTIME_CLASS(CMergeSplineObjectsTool),"object", m_pSpline );
		m_mergeButton.EnableWindow(TRUE);
	}
	else
	{
		m_splitButton.EnableWindow(FALSE);
		m_mergeButton.EnableWindow(FALSE);
	}

	int index = m_pSpline->GetSelectedPoint();

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
		float val = m_pSpline->GetPointAngle();
		m_angle.SetValue(val);

		GetDlgItem(IDC_DEFAULT_WIDTH)->EnableWindow( TRUE );

		bool isDefault = m_pSpline->IsPointDefaultWidth();
		((CButton *)GetDlgItem(IDC_DEFAULT_WIDTH))->SetCheck( isDefault  );
		GetDlgItem(IDC_WIDTH)->EnableWindow( !isDefault );

		CString str;
		str.Format("Selected point: %d", index+1);
		GetDlgItem(IDC_SELECTED_POINT)->SetWindowText(str);
	}

	m_width.SetValue(m_pSpline->GetPointWidth());
}


void CSplinePanel::OnUpdateParams( CNumberCtrl *ctrl )
{
	m_pSpline->SetPointAngle(m_angle.GetValue());
	m_pSpline->SetPointWidth(m_width.GetValue());
}

void CSplinePanel::OnDefaultWidth()
{
	BOOL isDefault = ((CButton *)GetDlgItem(IDC_DEFAULT_WIDTH))->GetCheck( );
	GetDlgItem(IDC_WIDTH)->EnableWindow( !isDefault );
	m_pSpline->PointDafaultWidthIs(isDefault);
	m_width.SetValue(m_pSpline->GetPointWidth());
}

void CSplineEditButton::OnClicked()
{
	CMeasurementSystem::GetMeasurementSystem().ShutdownMeasurementSystem();

	CToolButton::OnClicked();
}

BEGIN_MESSAGE_MAP(CSplineEditButton, CToolButton)
	//{{AFX_MSG_MAP(CToolButton)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()