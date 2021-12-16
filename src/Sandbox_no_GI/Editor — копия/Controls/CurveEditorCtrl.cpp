#include "StdAfx.h"
#include "CurveEditorCtrl.h"

namespace CurveEditor
{
const int kHandleSize = 6;
const int kHandleSizeHalf = kHandleSize/2;
const int kDefaultPadding = 10;
const int kInfoFontSize = 70;
const int kGrid = 4;
const COLORREF kColor_SelectCross = RGB(132, 132, 132);
const COLORREF kColor_DisabledCross = RGB(90, 90, 90);
const COLORREF kColor_MiddleLines = RGB(80, 80, 80);
const COLORREF kColor_Background = RGB(41, 41, 41);
const COLORREF kColor_Disabled = RGB(60, 60, 60);
const COLORREF kColor_PaddingBorder = RGB(128,128,128);
const COLORREF kColor_Text = RGB(128, 128, 128);
const COLORREF kColor_TextCrtPos = RGB(187, 187, 187);
const COLORREF kColor_Curve = RGB(255, 0, 0);
const COLORREF kColor_SelHandle = RGB(200, 200, 200);
const COLORREF kColor_NormalHandle = RGB( 30, 30, 30);
const COLORREF kColor_HandleLight = RGB(60, 60, 60);
const COLORREF kColor_HandleShadow = RGB(0, 0, 0);
}

IMPLEMENT_DYNAMIC(CCurveEditorCtrl, CStatic)

CCurveEditorCtrl::CCurveEditorCtrl()
{
	m_domainMinX = 0.0f;
	m_domainMinY = 0.0f;
	m_domainMaxX = 1.0f;
	m_domainMaxY = 1.0f;
	m_bMouseDown = m_bDragging = false;
	m_bAllowMouse = true;
	m_padding = CurveEditor::kDefaultPadding;
	m_flags = eFlag_ShowVerticalRuler
			| eFlag_ShowHorizontalRuler
			| eFlag_ShowVerticalRulerText
			| eFlag_ShowHorizontalRulerText
			| eFlag_ShowPaddingBorder
			| eFlag_ShowMovingPointAxis
			| eFlag_ShowPointHandles;
	m_gridSplits.set(CurveEditor::kGrid, CurveEditor::kGrid);
	m_fntInfo.CreatePointFont(CurveEditor::kInfoFontSize, "Arial");
	m_bHovered = false;
	m_selCrossPen.CreatePen(PS_SOLID, 1, CurveEditor::kColor_SelectCross);

	GenerateDefaultCurve();
}

CCurveEditorCtrl::~CCurveEditorCtrl()
{
	m_fntInfo.DeleteObject();
	m_selCrossPen.DeleteObject();
}

BEGIN_MESSAGE_MAP(CCurveEditorCtrl, CStatic)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SETFOCUS()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

void CCurveEditorCtrl::SetFlags(UINT aFlags)
{
	m_flags = aFlags;
}

UINT CCurveEditorCtrl::GetFlags() const
{
	return m_flags;
}

bool CCurveEditorCtrl::SetDomainBounds(float aMinX, float aMinY, float aMaxX, float aMaxY)
{
	ASSERT(aMinX < aMaxX);
	ASSERT(aMinY < aMaxY);

	if (aMinX >= aMaxX)
	{
		return false;
	}

	if (aMinY >= aMaxY)
	{
		return false;
	}

	m_domainMinX = aMinX;
	m_domainMinY = aMinY;
	m_domainMaxX = aMaxX;
	m_domainMaxY = aMaxY;

	return true;
}

void CCurveEditorCtrl::GetDomainBounds(float& rMinX, float& rMinY, float& rMaxX, float& rMaxY) const
{
	rMinX = m_domainMinX;
	rMinY = m_domainMinY;
	rMaxX = m_domainMaxX;
	rMaxY = m_domainMaxY;
}

void CCurveEditorCtrl::SetGrid(UINT aHorizontalSplits, UINT aVerticalSplits)
{
	ASSERT(aHorizontalSplits);
	ASSERT(aVerticalSplits);

	if (!aHorizontalSplits)
	{
		// defaults
		aHorizontalSplits = 2;
	}

	if (!aVerticalSplits)
	{
		// defaults
		aVerticalSplits = 2;
	}

	m_gridSplits.x = aHorizontalSplits;
	m_gridSplits.y = aVerticalSplits;
}

CPoint CCurveEditorCtrl::ProjectPoint(float x, float y)
{
	CPoint pt;

	pt.x = m_padding + (m_canvas.GetWidth() - m_padding*2) * (x - m_domainMinX) / (m_domainMaxX - m_domainMinX);
	pt.y = m_padding + (m_canvas.GetHeight() - m_padding*2) * (1.0f - (y - m_domainMinY) / (m_domainMaxY - m_domainMinY));

	return pt;
}

Vec2 CCurveEditorCtrl::UnprojectPoint(const CPoint& pt)
{
	Vec2 vec;
	int y = m_canvas.GetHeight() - pt.y;
	float dx = (m_canvas.GetWidth() - m_padding*2);
	float dy = (m_canvas.GetHeight() - m_padding*2);
	const float kEpsilon = 0.00000001f;

	if (fabs(dx) <= kEpsilon)
	{
		dx = 1.0f;
	}

	if (fabs(dy) <= kEpsilon)
	{
		dy = 1.0f;
	}

	vec.x = m_domainMinX + (float)(pt.x - m_padding) / dx * (m_domainMaxX - m_domainMinX);
	vec.y = m_domainMinY + (float)(y - m_padding) / dy * (m_domainMaxY - m_domainMinY);

	return vec;
}

void CCurveEditorCtrl::SetControlPointCount(UINT aCount)
{
	m_points.resize(aCount);
	m_projectedPoints.clear();
}

UINT CCurveEditorCtrl::GetControlPointCount() const
{
	return m_points.size();
}

void CCurveEditorCtrl::AddControlPoint(const Vec2& rPosition)
{
	m_points.push_back(CurvePoint(rPosition.x, rPosition.y));
}

void CCurveEditorCtrl::ClearControlPoints()
{
	m_points.clear();
}

void CCurveEditorCtrl::SetControlPoint(UINT aIndex, const Vec2& rPosition)
{
	ASSERT(aIndex < m_points.size());

	if (aIndex >= m_points.size())
	{
		return;
	}

	m_points[aIndex].pos = rPosition;
}

void CCurveEditorCtrl::SetControlPointTangents(UINT aIndex, const Vec2& rLeft, const Vec2& rRight)
{
	ASSERT(aIndex < m_points.size());

	if (aIndex >= m_points.size())
	{
		return;
	}

	m_points[aIndex].tanA = rLeft;
	m_points[aIndex].tanB = rRight;
}

void CCurveEditorCtrl::GetControlPoint(UINT aIndex, Vec2& rOutPosition) const
{
	ASSERT(aIndex < m_points.size());

	if (aIndex >= m_points.size())
	{
		return;
	}

	rOutPosition = m_points[aIndex].pos;
}

void CCurveEditorCtrl::GetControlPointTangents(UINT aIndex, Vec2& rOutLeft, Vec2& rOutRight) const
{
	ASSERT(aIndex < m_points.size());

	if (aIndex >= m_points.size())
	{
		return;
	}

	rOutLeft = m_points[aIndex].tanA;
	rOutRight = m_points[aIndex].tanB;
}

void CCurveEditorCtrl::OnPaint()
{
	CPaintDC ctrlDC(this);

	CDC& dc = m_canvas.GetDC();
	CRect rc;
	CString str;
	CSize textSize;

	if(m_flags & eFlag_Disabled)
	{
		// If disabled, just draw a blank square.
		GetClientRect(rc);
		dc.SelectObject(m_fntInfo);
		dc.FillSolidRect(rc, CurveEditor::kColor_Disabled);

		m_pen.CreatePen(PS_SOLID, 1, CurveEditor::kColor_DisabledCross);
		dc.SelectObject(m_pen);

		dc.MoveTo(0, 0);
		dc.LineTo(rc.Width(), rc.Height());

		dc.MoveTo(rc.Width(), 0);
		dc.LineTo(0, rc.Height());

		m_pen.DeleteObject();

		m_canvas.BlitTo(ctrlDC.GetSafeHdc());

		return;
	}

	GetClientRect(rc);
	dc.SelectObject(m_fntInfo);
	dc.FillSolidRect(rc, CurveEditor::kColor_Background);

	m_pen.CreatePen(PS_SOLID, 1, CurveEditor::kColor_MiddleLines);
	dc.SelectObject(m_pen);
	dc.SetTextColor(CurveEditor::kColor_Text);
	
	if (m_flags & eFlag_ShowVerticalRuler)
	{
		float y = m_domainMinY;
		float grid = (m_domainMaxY - m_domainMinY) / m_gridSplits.y;
		CPoint p;

		for (int i = 0; i <= m_gridSplits.y; ++i)
		{
			p = ProjectPoint(0, y);
			dc.MoveTo(m_padding, p.y);
			dc.LineTo(rc.Width() - m_padding, p.y);

			if (m_flags & eFlag_ShowVerticalRulerText)
			{
				str.Format("%0.2f", y);
				textSize = dc.GetTextExtent(str);
				dc.TextOut(m_padding + 2, p.y - textSize.cy, str);
			}

			y += grid;
		}
	}

	if (m_flags & eFlag_ShowHorizontalRuler)
	{
		float x = m_domainMinX;
		float grid = (m_domainMaxX - m_domainMinX) / m_gridSplits.x;
		CPoint p;

		for (int i = 0; i <= m_gridSplits.x; ++i)
		{
			p = ProjectPoint(x, 0);
			dc.MoveTo(p.x, m_padding);
			dc.LineTo(p.x, rc.Height() - m_padding);
			
			if (m_flags & eFlag_ShowHorizontalRulerText)
			{
				str.Format("%0.2f", x);
				textSize = dc.GetTextExtent(str);

				p.x += 2;

				if (p.x + textSize.cx > rc.Width())
				{
					p.x = rc.Width() - textSize.cx;
				}

				dc.TextOut(p.x, rc.Height() - m_padding, str);
			}

			x += grid;
		}
	}

	m_pen.DeleteObject();

	if (m_flags & eFlag_ShowPaddingBorder)
	{
		m_pen.CreatePen(PS_SOLID, 1, CurveEditor::kColor_PaddingBorder);
		dc.SelectObject(m_pen);
		dc.SelectObject(GetStockObject(NULL_BRUSH));
		dc.Rectangle(m_padding, m_padding, rc.Width() - m_padding, rc.Height() - m_padding);
		m_pen.DeleteObject();
	}

	if (m_bDragging 
		&& !m_selectedIndices.empty()
		&& (m_flags & eFlag_ShowMovingPointAxis))
	{
		const Vec2& crtPos = m_points[m_selectedIndices[0]].pos;

		dc.SetTextColor(CurveEditor::kColor_TextCrtPos);
		str.Format("(%0.2f,%0.2f)", crtPos.x, crtPos.y);
		textSize = dc.GetTextExtent(str);
		const int kOffsetFromPointer = 5;
		CPoint txtPos(m_lastMousePoint.x + kOffsetFromPointer, m_lastMousePoint.y + kOffsetFromPointer);

		if (txtPos.x + textSize.cx > rc.Width())
		{
			txtPos.x = rc.Width() - textSize.cx;
		}

		if (txtPos.y + textSize.cy > rc.Height())
		{
			txtPos.y = rc.Height() - textSize.cy;
		}
	
		dc.TextOut(txtPos.x, txtPos.y, str);
	}

	ComputeTangents();
	UpdateProjectedPoints();

	// for curve debug, tangents poly, dont delete
	//dc.SelectObject(GetStockObject(BLACK_PEN));
	//dc.Polyline(&m_projectedPoints[0], m_projectedPoints.size());

	m_pen.CreatePen(PS_SOLID, 1, CurveEditor::kColor_Curve);
	dc.SelectObject(m_pen);
	
	// curve
	dc.PolyBezier(&m_projectedPoints[0], m_projectedPoints.size());
	m_pen.DeleteObject();

	// curve control point handles
	if(m_flags & eFlag_ShowPointHandles)
	{
		for (size_t i = 0; i < m_points.size(); ++i)
		{
			CPoint ptProj = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
			CRect rcHandle;

			rcHandle.SetRect(
				ptProj.x - CurveEditor::kHandleSizeHalf,
				ptProj.y - CurveEditor::kHandleSizeHalf,
				ptProj.x + CurveEditor::kHandleSizeHalf,
				ptProj.y + CurveEditor::kHandleSizeHalf);

			std::vector<int>::iterator iter =
				std::find(m_selectedIndices.begin(), m_selectedIndices.end(), i);

			bool bSelected = (iter != m_selectedIndices.end());

			if (bSelected && m_bDragging)
			{
				dc.SelectObject(m_selCrossPen);
				dc.MoveTo(0, ptProj.y);
				dc.LineTo(rc.Width(), ptProj.y);
				dc.MoveTo(ptProj.x, 0);
				dc.LineTo(ptProj.x, rc.Height());
			}

			dc.FillSolidRect(rcHandle, 
				bSelected 
				? CurveEditor::kColor_SelHandle
				: CurveEditor::kColor_NormalHandle);

			dc.Draw3dRect(rcHandle, CurveEditor::kColor_HandleLight, CurveEditor::kColor_HandleShadow);
		}
	}

	m_canvas.BlitTo(ctrlDC.GetSafeHdc());
}

void CCurveEditorCtrl::ComputeTangents()
{
	for (size_t i = 0; i < m_points.size(); ++i)
	{
		m_points[i].tanA = m_points[i].pos;
		m_points[i].tanB = m_points[i].pos;
	}

	int maxIndex = m_points.size() - 1;
	
	for (size_t i = 0; i < m_points.size(); ++i)
	{
		if (i > maxIndex)
			break;

		Vec2& p2 = m_points[i].pos;
		Vec2& back = m_points[i].tanA;
		Vec2& forw = m_points[i].tanB;
		const float kEpsilon = 0.000001f;

		// first point
		if(i == 0)
		{
			back = p2;
			
			if (maxIndex == 1)
			{
				Vec2& p3 = m_points[i + 1].pos;
				forw = p2 + (p3 - p2) / 3.0f;
			}
			else if (maxIndex > 0)
			{
				Vec2& p3 = m_points[i + 1].pos;
				Vec2& pb3 = m_points[i + 1].tanA;

				float lenOsn = (pb3 - p2).GetLength();
				float lenb = (p3 - p2).GetLength();

				if (lenOsn > kEpsilon && lenb > kEpsilon)
				{
				forw = p2 + (pb3 - p2) / (lenOsn / lenb * 3.0f);
			}
				else
				{
					forw = p2;
		}
			}
		}

		if (i == maxIndex)
		{
			forw = p2;

			if (i > 0)
			{
				Vec2& p1 = m_points[i - 1].pos;
				Vec2& pf1 = m_points[i - 1].tanB;

				float lenOsn = (pf1 - p2).GetLength();
				float lenf = (p1 - p2).GetLength();

				if (lenOsn > kEpsilon && lenf > kEpsilon)
				{
					back = p2 + (pf1 - p2) / (lenOsn / lenf * 3.0f);
				}
				else
				{
					back = p2;
				}
			}
		}
		else if (i >= 1 && i <= maxIndex - 1)
		{
			Vec2& p1 = m_points[i - 1].pos;
			Vec2& p3 = m_points[i + 1].pos;

			float lenOsn = (p3 - p1).GetLength();
			float lenb = (p1 - p2).GetLength();
			float lenf = (p3 - p2).GetLength();

			if (lenOsn > kEpsilon
				&& lenf > kEpsilon
				&& lenb > kEpsilon)
			{
			back = p2 + (p1 - p3) * (lenb / lenOsn / 3.0f);
			forw = p2 + (p3 - p1) * (lenf / lenOsn / 3.0f);
		}
		}
		
		ClampToDomain(back);
		ClampToDomain(forw);
	}

	// fix tangents in relation of one to another
	for (size_t i = 0; i < m_points.size(); ++i)
	{
		Vec2& p = m_points[i].pos;
		Vec2& tanA = m_points[i].tanA;
		Vec2& tanB = m_points[i].tanB;

		if (i < m_points.size() - 1)
		{
			if (tanB.x > m_points[i + 1].tanA.x)
			{
				float mid = p.x + (m_points[i + 1].pos.x - p.x) * 0.5f;
				tanB.x = mid;
			}
		}

		if (i > 0)
		{
			if (tanA.x < m_points[i - 1].tanB.x)
			{
				float mid = m_points[i - 1].pos.x + (p.x - m_points[i - 1].pos.x) * 0.5f;
				tanA.x = mid;
			}
		}
	}
}

void CCurveEditorCtrl::UpdateProjectedPoints()
{
	m_projectedPoints.resize(m_points.size()*3 - 2);

	int numPts = 0;

	for (size_t i = 0; i < m_points.size(); ++i)
	{
		if (i == 0)
		{
			m_projectedPoints[numPts++] = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
			m_projectedPoints[numPts++] = ProjectPoint(m_points[i].tanB.x, m_points[i].tanB.y);
		}
		else if (i == m_points.size() - 1)
		{
			m_projectedPoints[numPts++] = ProjectPoint(m_points[i].tanA.x, m_points[i].tanA.y);
			m_projectedPoints[numPts++] = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
		}
		else
		{
			m_projectedPoints[numPts++] = ProjectPoint(m_points[i].tanA.x, m_points[i].tanA.y);
			m_projectedPoints[numPts++] = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
			m_projectedPoints[numPts++] = ProjectPoint(m_points[i].tanB.x, m_points[i].tanB.y);
		}
	}
}

void CCurveEditorCtrl::ClampToDomain(Vec2& rVec)
{
	if (rVec.x < m_domainMinX)
	{
		rVec.x = m_domainMinX;
	}
	else if (rVec.x > m_domainMaxX)
	{
		rVec.x = m_domainMaxX;
	}

	if (rVec.y < m_domainMinY)
	{
		rVec.y = m_domainMinY;
	}
	else if (rVec.y > m_domainMaxY)
	{
		rVec.y = m_domainMaxY;
	}
}

void CCurveEditorCtrl::GenerateDefaultCurve()
{
	m_points.clear();
	m_domainMinX = 0.0f;
	m_domainMinY = 0.0f;
	m_domainMaxX = 1.0f;
	m_domainMaxY = 1.0f;
	m_points.push_back(CurvePoint(0.00f, 0.00f));
	m_points.push_back(CurvePoint(0.25f, 0.25f));
	m_points.push_back(CurvePoint(0.50f, 0.50f));
	m_points.push_back(CurvePoint(0.75f, 0.75f));
	m_points.push_back(CurvePoint(1.00f, 1.00f));
}

void CCurveEditorCtrl::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	CDC* pDC = GetDC();

	if (!m_canvas.GetDC().GetSafeHdc())
	{
		m_canvas.Create(pDC->GetSafeHdc(), cx, cy);
	}
	else
	{
		m_canvas.Resize(pDC->GetSafeHdc(), cx, cy);
	}

	ReleaseDC(pDC);
}

void CCurveEditorCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	if(m_bAllowMouse)
	{
	bool bSimpleSelect = !(nFlags & MK_SHIFT) && !(nFlags & MK_CONTROL);

	if (bSimpleSelect)
	{
		m_selectedIndices.clear();
	}
	
	for (size_t i = 0; i < m_points.size(); ++i)
	{
		CPoint ptProj = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
		CRect rcHandle;

		rcHandle.SetRect(
			ptProj.x - CurveEditor::kHandleSizeHalf,
			ptProj.y - CurveEditor::kHandleSizeHalf,
			ptProj.x + CurveEditor::kHandleSizeHalf,
			ptProj.y + CurveEditor::kHandleSizeHalf);

		if (rcHandle.PtInRect(point))
		{
			if (bSimpleSelect)
			{
				m_selectedIndices.push_back(i);
				break;
			}

			if (nFlags & MK_SHIFT)
			{
				m_selectedIndices.push_back(i);
			}
			else if (nFlags & MK_CONTROL)
			{
				std::vector<int>::iterator iter =
					std::find(m_selectedIndices.begin(), m_selectedIndices.end(), i);

				if (iter == m_selectedIndices.end())
				{
					m_selectedIndices.push_back(i);
				}
				else
				{
					m_selectedIndices.erase(iter);
				}
			}
		}
	}

	m_bMouseDown = true;
	m_lastMousePoint = point;
	}

	SetCapture();
	RedrawWindow();

	CStatic::OnLButtonDown(nFlags, point);
}

void CCurveEditorCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bMouseDown = false;
	m_bDragging = false;
	m_selectedIndices.clear();
	ReleaseCapture();
	RedrawWindow();
	
	CStatic::OnLButtonUp(nFlags, point);
}

void CCurveEditorCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bMouseDown && !m_bDragging)
	{
		m_bDragging = true;
	}

	m_bHovered = true;
	if(m_flags & eFlag_ShowCursorAlways)
	{
		m_bHovered = true;
	}
	else
	{
	m_bHovered = false;

	for (size_t i = 0; i < m_points.size(); ++i)
	{
		CPoint ptProj = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
		CRect rcHandle;

		rcHandle.SetRect(
			ptProj.x - CurveEditor::kHandleSizeHalf,
			ptProj.y - CurveEditor::kHandleSizeHalf,
			ptProj.x + CurveEditor::kHandleSizeHalf,
			ptProj.y + CurveEditor::kHandleSizeHalf);

		if (rcHandle.PtInRect(point))
		{
			m_bHovered = true;
			break;
		}
	}
	}

	if (m_bDragging)
	{
		Vec2 v1 = UnprojectPoint(m_lastMousePoint);
		Vec2 v2 = UnprojectPoint(point);
		Vec2 v = v1 - v2;

		for (size_t i = 0; i < m_selectedIndices.size(); ++i)
		{
			int index = m_selectedIndices[i];
			CurvePoint& cpt = m_points[index];

			// do not move first and last points on X
			if (index > 0 && index < m_points.size() - 1)
			{
				cpt.pos.x -= v.x;
			}

			cpt.pos.y -= v.y;

			// lets check if the point is overlapping its neighbours
			if (index > 0 && (index - 1) > 0)
			{
				if (cpt.pos.x < m_points[index - 1].pos.x)
				{
					CurvePoint p = m_points[index];
					
					// swap!
					m_points[index] = m_points[index - 1];
					m_points[index - 1] = p;
					m_selectedIndices[i] = index - 1;
				}
			}

			if (index < m_points.size() - 1 && (index + 1) < m_points.size() - 1)
			{
				if (cpt.pos.x > m_points[index + 1].pos.x)
				{
					CurvePoint p = m_points[index];

					// swap!
					m_points[index] = m_points[index + 1];
					m_points[index + 1] = p;
					m_selectedIndices[i] = index + 1;
				}
			}

			ClampToDomain(cpt.pos);
		}

		RedrawWindow();
		m_lastMousePoint = point;
	}

	CStatic::OnMouseMove(nFlags, point);
}

void CCurveEditorCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CStatic::OnSetFocus(pOldWnd);
}

BOOL CCurveEditorCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bHovered)
	{
		::SetCursor(::LoadCursor(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDC_ARRWHITE)));
		return TRUE;
	}

	return CStatic::OnSetCursor(pWnd, nHitTest, message);
}
