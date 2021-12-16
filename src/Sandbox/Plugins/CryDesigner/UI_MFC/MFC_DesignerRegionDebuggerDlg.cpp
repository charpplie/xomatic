#include "stdafx.h"
#include "MFC_DesignerRegionDebuggerDlg.h"
#include "afxdialogex.h"

static const int nCanvasOffsetX = 10;
static const int nCanvasOffsetY = 10;

IMPLEMENT_DYNAMIC(MFC_DesignerRegionDebuggerDlg, CDialog)

BEGIN_MESSAGE_MAP(MFC_DesignerRegionDebuggerDlg, CDialog)
	ON_WM_MBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_RESETPIVOTVIEW, OnBnClickedResetpivotview)
	ON_LBN_SELCHANGE(IDC_REGIONLIST, OnLbnSelchangeRegionlist)
	ON_NOTIFY(TVN_SELCHANGED, IDC_REGION_ELEMENTINFO_TREE, OnTvnSelchangedRegionElementinfoTree)
	ON_BN_CLICKED(IDC_DISPLAYTRANSPARENTTEXT, OnBnClickedDisplaytransparenttext)
	ON_BN_CLICKED(IDC_DISPLAYZOOMRECT, OnBnClickedDisplayzoomrect)
END_MESSAGE_MAP()

IDesignerRegionDebuggerDlg* CreateRegionDebuggerDlg()
{
	return new MFC_DesignerRegionDebuggerDlg;
}

MFC_DesignerRegionDebuggerDlg::MFC_DesignerRegionDebuggerDlg(CWnd* pParent /*=NULL*/)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(MFC_DesignerRegionDebuggerDlg::IDD, pParent);
}

void MFC_DesignerRegionDebuggerDlg::DoDataExchange(CDataExchange* pDX)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REGIONLIST, m_RegionListBox);
	DDX_Control(pDX, IDC_REGION_ELEMENTINFO_TREE, m_ElementInfoTree);
	DDX_Control(pDX, IDC_DISPLAYTRANSPARENTTEXT, m_TransparentCheckBox);
	DDX_Control(pDX, IDC_DISPLAYZOOMRECT, m_DisplayZoomRectCheckBox);
}

void MFC_DesignerRegionDebuggerDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	__super::OnRButtonDown(nFlags,point);
	m_prevMousePos = point;
	SetCapture();
}

void MFC_DesignerRegionDebuggerDlg::OnMButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	__super::OnMButtonDown(nFlags,point);
}

void MFC_DesignerRegionDebuggerDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
}

void MFC_DesignerRegionDebuggerDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	__super::OnLButtonDown(nFlags,point);
	SetCapture();
	m_prevMousePos = point;
	MoveZoomRect(point);
}

void MFC_DesignerRegionDebuggerDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
}

void MFC_DesignerRegionDebuggerDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	__super::OnMouseMove(nFlags,point);

	if( nFlags & MK_RBUTTON )
	{
		float zoomBoundaryWidth = m_ZoomedBoundary.max.x-m_ZoomedBoundary.min.x;
		float zoomBoundaryHeight = m_ZoomedBoundary.max.z-m_ZoomedBoundary.min.z;

		int dx = point.x-m_prevMousePos.x;
		int dy = point.y-m_prevMousePos.y;

		float unitX = zoomBoundaryWidth * 0.002f;
		float unitY = zoomBoundaryHeight * 0.002f;

		m_ZoomedBoundary.min.x -= unitX*dx;
		m_ZoomedBoundary.max.x -= unitX*dx;

		m_ZoomedBoundary.min.z -= unitY*dy;
		m_ZoomedBoundary.max.z -= unitY*dy;

		Invalidate(FALSE);

		m_prevMousePos = point;
	}
	else if( nFlags & MK_LBUTTON )
	{
		MoveZoomRect(point);
	}
}

CPoint MFC_DesignerRegionDebuggerDlg::Dlg2CanvasPos( const CPoint& point) const
{
	CRect rDlg;
	GetWindowRect(rDlg);

	CRect rCanvas;
	GetWindowRect(rCanvas);

	return CPoint(point.x+rDlg.left-rCanvas.left,point.y+rDlg.top-rCanvas.top);
}

void MFC_DesignerRegionDebuggerDlg::MoveZoomRect( const CPoint& point )
{
	CRect rCanvas = GetCanvasRect();
	CPoint pointInCanvas = Dlg2CanvasPos(point);
	if( !rCanvas.PtInRect(pointInCanvas) )
		return;

	int nWidth = m_ZoomRect.Width();
	int nHeight = m_ZoomRect.Height();

	m_ZoomRect.left = point.x - nWidth/2;
	m_ZoomRect.right = point.x + nWidth/2;
	m_ZoomRect.top = point.y - nHeight/2;
	m_ZoomRect.bottom = point.y + nHeight/2;

	DESIGNER_ASSERT( m_ZoomRect.left < m_ZoomRect.right && m_ZoomRect.top < m_ZoomRect.bottom );

	Invalidate(FALSE);
}

void MFC_DesignerRegionDebuggerDlg::AddRegion( CBrushRegion* pRegion, const char* name )
{
	if( !pRegion || !pRegion->IsValid() )
		return; 

	m_Regions.push_back(pRegion);
	m_RegionNames.push_back(name);
	UpdateRegionsBoundary();
}

void MFC_DesignerRegionDebuggerDlg::UpdateRegionsBoundary()
{
	m_RegionsBoundary.Reset();

	for( int i = 0, iRegionCount(m_Regions.size()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = m_Regions[i];
		if( !pRegion || !pRegion->IsValid() )
			continue;
		AABB boundaryRectangle;
		boundaryRectangle.Reset();
		
		BrushVec2 r_min = pRegion->GetPlane().W2P(ToBrushVec3(pRegion->GetBoundBox().min));
		BrushVec2 r_max = pRegion->GetPlane().W2P(ToBrushVec3(pRegion->GetBoundBox().max));
		boundaryRectangle.Add(Vec3(r_min.x,0,r_min.y));
		boundaryRectangle.Add(Vec3(r_max.x,0,r_max.y));
		
		m_RegionsBoundary.Add(boundaryRectangle);
	}

	m_ZoomedBoundary = m_RegionsBoundary;
}

BOOL MFC_DesignerRegionDebuggerDlg::OnInitDialog()
{
	RESOURCEHANDLER_RECONSTRUCTOR;

	__super::OnInitDialog();

	CRect rCanvas = GetCanvasRect();
	m_ZoomRect.left = 0;
	m_ZoomRect.top = 0;
	m_ZoomRect.right = rCanvas.Width()/2;
	m_ZoomRect.bottom = rCanvas.Height()/2;

	UpdateRegionsBoundary();
	MoveZoomRect(rCanvas.CenterPoint());

	m_RegionListBox.ResetContent();
	for( int i = 0, iRegionCount(m_RegionNames.size()); i < iRegionCount; ++i )
		m_RegionListBox.AddString(m_RegionNames[i]);

	CRect rFill = GetCanvasFillRect();
	CDC screenDC;
	screenDC.Attach(::GetWindowDC(0));
	m_MemoryBitmap.CreateCompatibleBitmap(&screenDC, rFill.Width(), rFill.Height());
	m_MemoryDC.CreateCompatibleDC(&screenDC);
	m_MemoryDC.SelectObject(&m_MemoryBitmap);
	::ReleaseDC(0, screenDC.Detach());

	if( !m_Regions.empty() && m_Regions[0] )
	{
		m_RegionListBox.SetCurSel(0);
		UpdateElementTree(m_Regions[0]);
	}

	return TRUE;
}

void MFC_DesignerRegionDebuggerDlg::OnPaint()
{
	__super::OnPaint();


	CWnd* pCanvasWnd = GetDlgItem(IDC_REGIONDEBUGGERAREA);
	if( pCanvasWnd == NULL )
		return;

	CDC* pDC = &m_MemoryDC;

	pDC->SetBkMode(OPAQUE);

	CRect rFill = GetCanvasFillRect();
	rFill.OffsetRect(-rFill.left,-rFill.top);
	pDC->FillSolidRect(&rFill, RGB(255,255,255));

	CRect rCanvas = GetCanvasRect();
	rCanvas.OffsetRect(-rCanvas.left+nCanvasOffsetX,-rCanvas.top+nCanvasOffsetY);

	int nCurSel = m_RegionListBox.GetCurSel();

	SRegionDrawInfo drawColorInfo(RGB(225,225,225),RGB(225,225,225),RGB(225,225,225));
	for( int i = 0, iRegionCount(m_Regions.size()); i < iRegionCount; ++i )
	{
		if( i != nCurSel )
			DrawRegion(pDC,drawColorInfo,rCanvas,m_Regions[i],false);
	}

	if( m_TransparentCheckBox.GetCheck() == BST_CHECKED )
		pDC->SetBkMode(TRANSPARENT);

	if( nCurSel != -1 )
		DrawRegion(pDC,SRegionDrawInfo(),rCanvas,m_Regions[nCurSel],true);

	if( m_DisplayZoomRectCheckBox.GetCheck() == BST_CHECKED )
		DrawRect( pDC, RGB(200,100,100), m_ZoomRect );

	if( GetDC() )
	{
		CRect rFillWithoutOffset = GetCanvasFillRect();
		GetDC()->BitBlt(
			rFillWithoutOffset.left,
			rFillWithoutOffset.top,
			rFillWithoutOffset.Width(),
			rFillWithoutOffset.Height(),
			pDC, 
			0, 
			0, SRCCOPY);
	}
}

void MFC_DesignerRegionDebuggerDlg::DrawRect( CDC* pDC, COLORREF color, const CRect& rect )
{
	CPen pen;
	pen.CreatePen(PS_SOLID,3,color);
	pDC->SelectObject(&pen);

	pDC->MoveTo(m_ZoomRect.left,m_ZoomRect.top);
	pDC->LineTo(m_ZoomRect.right,m_ZoomRect.top);
	pDC->LineTo(m_ZoomRect.right,m_ZoomRect.bottom);
	pDC->LineTo(m_ZoomRect.left,m_ZoomRect.bottom);
	pDC->LineTo(m_ZoomRect.left,m_ZoomRect.top);
}

CPoint MFC_DesignerRegionDebuggerDlg::World2Screen( const BrushVec2& v, const CRect& rCanvas ) const
{
	float zoomBoundaryWidth = m_ZoomedBoundary.max.x-m_ZoomedBoundary.min.x;
	float zoomBoundaryHeight = m_ZoomedBoundary.max.z-m_ZoomedBoundary.min.z;

	CPoint pt;
	pt.x = ((v.x-m_ZoomedBoundary.min.x)/zoomBoundaryWidth)*rCanvas.Width()+rCanvas.left;
	pt.y = ((v.y-m_ZoomedBoundary.min.z)/zoomBoundaryHeight)*rCanvas.Height()+rCanvas.top;

	return pt;
}

void MFC_DesignerRegionDebuggerDlg::DrawRegion( CDC* pDC, const MFC_DesignerRegionDebuggerDlg::SRegionDrawInfo& colorInfo, const CRect& rCanvas, CBrushRegion::RegionPtr pRegion, bool bSelectedInfo )
{
	if( !pRegion || !pRegion->IsValid() )
		return;

	pDC->SetTextColor(colorInfo.edgeNumberColor);

	CPen pen;
	pen.CreatePen(PS_SOLID,1,colorInfo.edgeColor);
	pDC->SelectObject(&pen);

	float zoomBoundaryWidth = m_ZoomedBoundary.max.x-m_ZoomedBoundary.min.x;
	float zoomBoundaryHeight = m_ZoomedBoundary.max.z-m_ZoomedBoundary.min.z;

	for( int k = 0, iEdgeCount(pRegion->GetEdgeSize()); k < iEdgeCount; ++k )
	{
		BrushEdge3D e3D = pRegion->GetEdge(k);
		BrushEdge e;
		e.m_v[0] = pRegion->GetPlane().W2P(e3D.m_v[0]);
		e.m_v[1] = pRegion->GetPlane().W2P(e3D.m_v[1]);

		CPoint p0 = World2Screen(e.m_v[0],rCanvas);
		CPoint p1 = World2Screen(e.m_v[1],rCanvas);

		DrawEdge(pDC,p0.x,p0.y,p1.x,p1.y);
		DrawNumber(pDC,(p0.x+p1.x)/2,(p0.y+p1.y)/2,k);
	}

	pDC->SetTextColor(colorInfo.vertexNumberColor);
	for( int k = 0, iVertexCount(pRegion->GetVertexListSize()); k < iVertexCount; ++k )
	{	
		CPoint p = World2Screen(pRegion->GetPlane().W2P(pRegion->GetVertex(k)),rCanvas);
		DrawNumber(pDC,p.x,p.y,k);
	}

	if( bSelectedInfo && m_SelectedItemNode )
	{
		if( !strcmp( m_SelectedItemNode->getTag(), "Vertex") )
		{
			int nIndex = 0;
			if( m_SelectedItemNode->getAttr("Index",nIndex) )
			{
				pDC->SetTextColor(RGB(255,0,0));
				CPoint p = World2Screen(pRegion->GetPlane().W2P(pRegion->GetVertex(nIndex)),rCanvas);
				DrawNumber(pDC,p.x,p.y,nIndex);
			}
		}
		else if( !strcmp( m_SelectedItemNode->getTag(), "Edge") )
		{
			int nIndex = 0;
			if( m_SelectedItemNode->getAttr("Index",nIndex) )
			{
				pDC->SetTextColor(RGB(255,0,0));
				const BUtil::SEdge& edge = pRegion->GetEdgeIndexPair(nIndex);
				CPoint p0 = World2Screen(pRegion->GetPlane().W2P(pRegion->GetVertex(edge.m_i[0])),rCanvas);
				CPoint p1 = World2Screen(pRegion->GetPlane().W2P(pRegion->GetVertex(edge.m_i[1])),rCanvas);

				CPen pen;
				pen.CreatePen(PS_SOLID,3,RGB(255,0,0));
				pDC->SelectObject(&pen);
				DrawEdge(pDC,p0.x,p0.y,p1.x,p1.y);

				pDC->SetTextColor(RGB(0,255,255));
				DrawNumber(pDC,p0.x,p0.y,edge.m_i[0]);
				DrawNumber(pDC,p1.x,p1.y,edge.m_i[1]);

				pDC->SetTextColor(RGB(255,0,255));
				DrawNumber(pDC,(p0.x+p1.x)/2,(p0.y+p1.y)/2,nIndex);
			}
		}
	}
}

void MFC_DesignerRegionDebuggerDlg::DrawEdge( CDC* pDC, int x0, int y0, int x1, int y1 )
{
	pDC->MoveTo(x0,y0);
	pDC->LineTo(x1,y1);
}

void MFC_DesignerRegionDebuggerDlg::DrawNumber( CDC* pDC, int x, int y, int number )
{
	CString bufferNum;
	bufferNum.Format("%d",number);
	pDC->TextOut( x-4, y-8, bufferNum );
}

BOOL MFC_DesignerRegionDebuggerDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	float zoomBoundaryWidth = m_ZoomedBoundary.max.x-m_ZoomedBoundary.min.x;
	float zoomBoundaryHeight = m_ZoomedBoundary.max.z-m_ZoomedBoundary.min.z;

	if( zDelta > 0 )
	{
		CRect rCanvas = GetCanvasRect();

		float zoomedX = ((float)(m_ZoomRect.left-rCanvas.left)/(float)rCanvas.Width())*zoomBoundaryWidth+m_ZoomedBoundary.min.x;
		float zoomedY = ((float)(m_ZoomRect.top-rCanvas.top)/(float)rCanvas.Height())*zoomBoundaryHeight+m_ZoomedBoundary.min.z;

		float zoomedWidth = ((float)m_ZoomRect.Width()/(float)rCanvas.Width())*zoomBoundaryWidth;
		float zoomedHeight = ((float)m_ZoomRect.Height()/(float)rCanvas.Height())*zoomBoundaryHeight;

		m_ZoomedBoundary.min.x = zoomedX;
		m_ZoomedBoundary.min.z = zoomedY;

		m_ZoomedBoundary.max.x = zoomedX + zoomedWidth;
		m_ZoomedBoundary.max.z = zoomedY + zoomedHeight;

		Invalidate(FALSE);
	}
	else if( zDelta < 0 )
	{
		CRect rCanvas = GetCanvasRect();

		float anchorX = ((float)(m_ZoomRect.CenterPoint().x-rCanvas.left)/(float)rCanvas.Width())*zoomBoundaryWidth+m_ZoomedBoundary.min.x;
		float anchorY = ((float)(m_ZoomRect.CenterPoint().y-rCanvas.top)/(float)rCanvas.Height())*zoomBoundaryHeight+m_ZoomedBoundary.min.z;

		float width = m_ZoomedBoundary.max.x-m_ZoomedBoundary.min.x;
		float height = m_ZoomedBoundary.max.z-m_ZoomedBoundary.min.z;

		const float ratio = 1.25f;
		m_ZoomedBoundary.min.x = anchorX - width*ratio;
		m_ZoomedBoundary.max.x = anchorX + width*ratio;

		m_ZoomedBoundary.min.z = anchorY - height*ratio;
		m_ZoomedBoundary.max.z = anchorY + height*ratio;

		if( m_ZoomedBoundary.max.x-m_ZoomedBoundary.min.x > m_RegionsBoundary.max.x-m_RegionsBoundary.min.x || 
			m_ZoomedBoundary.max.z-m_ZoomedBoundary.min.z > m_RegionsBoundary.max.z-m_RegionsBoundary.min.z )
		{
			m_ZoomedBoundary = m_RegionsBoundary;
		}

		Invalidate(FALSE);
	}

	return TRUE;
}

CRect MFC_DesignerRegionDebuggerDlg::GetCanvasRect() const
{
	CWnd* pCanvasWnd = GetDlgItem(IDC_REGIONDEBUGGERAREA);
	if( pCanvasWnd == NULL )
		return CRect(0,0,0,0);

	CRect rDlgWnd;
	GetWindowRect(rDlgWnd);

	int captionHeight = GetSystemMetrics(SM_CYCAPTION);

	CRect rCanvasWnd;
	pCanvasWnd->GetWindowRect(rCanvasWnd);

	rCanvasWnd.OffsetRect(-rDlgWnd.left,-rDlgWnd.top-captionHeight);

	return rCanvasWnd;
}

CRect MFC_DesignerRegionDebuggerDlg::GetCanvasFillRect() const
{
	CRect rCanvas = GetCanvasRect();
	rCanvas.InflateRect(nCanvasOffsetX,nCanvasOffsetY);
	return rCanvas;
}

void MFC_DesignerRegionDebuggerDlg::OnBnClickedResetpivotview()
{
	m_ZoomedBoundary = m_RegionsBoundary;
	Invalidate(FALSE);
}

void MFC_DesignerRegionDebuggerDlg::OnLbnSelchangeRegionlist()
{
	int nCurSel = m_RegionListBox.GetCurSel();
	if( nCurSel != -1 )
		UpdateElementTree(m_Regions[nCurSel]);
	m_SelectedItemNode = NULL;
	Invalidate(FALSE);
}

void MFC_DesignerRegionDebuggerDlg::UpdateElementTree( CBrushRegion::RegionPtr pRegion )
{
	DeleteAllTreeItems();

	if( !pRegion || !pRegion->IsValid() )
		return;

	HTREEITEM hItemVertex = m_ElementInfoTree.InsertItem("Vertices",TVI_ROOT);
	for( int i = 0, iVertexCount(pRegion->GetVertexListSize()); i < iVertexCount; ++i )
	{
		CString buffer;
		BrushVec2 v = pRegion->GetPlane().W2P(pRegion->GetVertex(i));
		buffer.Format("%d : %3.5f,%3.5f",i,v.x,v.y);
		HTREEITEM hNewVertex = m_ElementInfoTree.InsertItem(buffer,hItemVertex);

		XmlNodeRef pNodeRef = GetIEditor()->GetSystem()->CreateXmlNode();
		pNodeRef->setTag("Vertex");
		pNodeRef->setAttr("Index",i);
		pNodeRef->setAttr("Position2D",v);
		pNodeRef->setAttr("Position3D",pRegion->GetVertex(i));
		IXmlNode* pNode = pNodeRef;
		pNode->AddRef();
		m_ElementInfoTree.SetItemData(hNewVertex,(DWORD_PTR)pNode);
	}

	HTREEITEM hItemEdge = m_ElementInfoTree.InsertItem("Edges",TVI_ROOT);
	int nEdgeIndex = 0;
	std::set<int> usedIndices;

	do
	{
		usedIndices.insert(nEdgeIndex);

		CString buffer;
		const BUtil::SEdge& rEdge = pRegion->GetEdgeIndexPair(nEdgeIndex);
		buffer.Format("%d : (%d,%d)", nEdgeIndex, rEdge.m_i[0], rEdge.m_i[1] );
		HTREEITEM hNewEdge = m_ElementInfoTree.InsertItem(buffer,hItemEdge);
		XmlNodeRef pNodeRef = GetIEditor()->GetSystem()->CreateXmlNode();
		pNodeRef->AddRef();
		pNodeRef->setTag("Edge");
		pNodeRef->setAttr("Index",nEdgeIndex);
		pNodeRef->setAttr("v0",rEdge.m_i[0]);
		pNodeRef->setAttr("v1",rEdge.m_i[1]);
		IXmlNode* pNode = pNodeRef;
		pNode->AddRef();
		m_ElementInfoTree.SetItemData(hNewEdge,(DWORD_PTR)pNode);

		BUtil::EdgeIndexSet candidateIndexSet;
		BrushVec3 prevLastV = pRegion->GetVertex(rEdge.m_i[1]);
		int nEdgeInLeastDistance = -1;
		BrushFloat fLeastDistance = 3e10;
		for( int i = 0, iEdgeSize(pRegion->GetEdgeSize()); i < iEdgeSize; ++i )
		{
			if( usedIndices.find(i) != usedIndices.end() )
				continue;
			const BUtil::SEdge& nextEdge = pRegion->GetEdgeIndexPair(i);
			BrushVec3 currentFirstV = pRegion->GetVertex(nextEdge.m_i[0]);
			BrushFloat distance = currentFirstV.GetDistance(prevLastV);
			if( rEdge.m_i[1] == nextEdge.m_i[0] )
				candidateIndexSet.insert(nextEdge.m_i[1]);

			if( distance < fLeastDistance )
			{
				fLeastDistance = distance;
				nEdgeInLeastDistance = i;
			}
		}

		if( candidateIndexSet.empty() && nEdgeInLeastDistance == -1 )
			break;

		if( candidateIndexSet.size() < 2 )
			nEdgeIndex = nEdgeInLeastDistance;
		else
			nEdgeIndex = pRegion->ChooseNextEdge(rEdge,candidateIndexSet);

		DESIGNER_ASSERT(nEdgeIndex != -1);

	} while( usedIndices.size() < pRegion->GetEdgeSize() );

	m_ElementInfoTree.Expand(hItemVertex,TVE_EXPAND);
	m_ElementInfoTree.Expand(hItemEdge,TVE_EXPAND);
}

void MFC_DesignerRegionDebuggerDlg::OnTvnSelchangedRegionElementinfoTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	HTREEITEM hItem = pNMTreeView->itemNew.hItem;

	if( hItem == NULL )
		return;

	m_SelectedItemNode = (IXmlNode*)m_ElementInfoTree.GetItemData(hItem);
	Invalidate(FALSE);

	*pResult = 0;
}

void MFC_DesignerRegionDebuggerDlg::DeleteAllTreeItems()
{
	HTREEITEM hRootItem = m_ElementInfoTree.GetRootItem();
	while(hRootItem)
	{
		HTREEITEM hChildItem = m_ElementInfoTree.GetChildItem(hRootItem);
		while(hChildItem)
		{
			IXmlNode* pNode = (IXmlNode*)m_ElementInfoTree.GetItemData(hChildItem);
			if( pNode )
				pNode->Release();
			hChildItem = m_ElementInfoTree.GetNextSiblingItem(hChildItem);
		}
		hRootItem = m_ElementInfoTree.GetNextSiblingItem(hRootItem);
	}
	m_ElementInfoTree.DeleteAllItems();
}

void MFC_DesignerRegionDebuggerDlg::OnBnClickedDisplaytransparenttext()
{
	Invalidate(FALSE);
}

void MFC_DesignerRegionDebuggerDlg::OnBnClickedDisplayzoomrect()
{
	Invalidate(FALSE);
}