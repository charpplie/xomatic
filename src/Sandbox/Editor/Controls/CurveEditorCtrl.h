#pragma once
#include "Util/GdiUtil.h"

class CCurveEditorCtrl : public CStatic
{
	DECLARE_DYNAMIC(CCurveEditorCtrl)

public:
	enum EFlags
	{
		eFlag_ShowVerticalRuler = (1<<0),
		eFlag_ShowHorizontalRuler = (1<<1),
		eFlag_ShowVerticalRulerText = (1<<2),
		eFlag_ShowHorizontalRulerText = (1<<3),
		eFlag_ShowPaddingBorder = (1<<4),
		eFlag_ShowMovingPointAxis = (1<<5),
		eFlag_ShowPointHandles = (1<<6),
		eFlag_ShowCursorAlways = (1<<7),
		eFlag_Disabled = (1<<8) // special case, when disabling preview window.
	};

	CCurveEditorCtrl();
	virtual ~CCurveEditorCtrl();

	void SetFlags(UINT aFlags);
	UINT GetFlags() const;
	void SetMouseEnable(bool bEnable=true) { m_bAllowMouse = bEnable; }
	bool GetMouseEnable() const {return m_bAllowMouse;}
	bool SetDomainBounds(float aMinX, float aMinY, float aMaxX, float aMaxY);
	void GetDomainBounds(float& rMinX, float& rMinY, float& rMaxX, float& rMaxY) const;
	// labelsX/labelsY must be null (to use default labels) 
	// or contain aHorizontalSplits+1/aVerticalSplits+1 items.
	void SetGrid(UINT aHorizontalSplits, UINT aVerticalSplits, const CString *labelsX=nullptr, const CString *labelsY=nullptr);
	void SetPadding(float padding) { m_padding = padding; }
	void MarkX(float value);
	void MarkY(float value);
	void AddControlPoint(const Vec2& rPosition);
	void ClearControlPoints();
	void SetControlPointCount(UINT aCount);
	UINT GetControlPointCount() const;
	void SetControlPoint(UINT aIndex, const Vec2& rPosition);
	void SetControlPointTangents(UINT aIndex, const Vec2& rLeft, const Vec2& rRight);
	void GetControlPoint(UINT aIndex, Vec2& rOutPosition) const;
	void GetControlPointTangents(UINT aIndex, Vec2& rOutLeft, Vec2& rOutRight) const;
	CPoint ProjectPoint(float x, float y);
	Vec2 UnprojectPoint(const CPoint& pt);
	void UpdateProjectedPoints();

protected:
	struct CurvePoint
	{
		CurvePoint(float aX = 0.0f, float aY = 0.0f)
		{
			pos.x = aX;
			pos.y = aY;
		}

		Vec2 pos;
		Vec2 tanA, tanB;
	};

	void ComputeTangents();
	void ClampToDomain(Vec2& rVec);
	void GenerateDefaultCurve();
	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	CGdiCanvas m_canvas;
	std::vector<CurvePoint> m_points;
	std::vector<POINT> m_projectedPoints;
	float m_domainMinX;
	float m_domainMinY;
	float m_domainMaxX;
	float m_domainMaxY;
	Vec2 m_gridSplits;
	int m_padding;
	bool m_bMouseDown, m_bDragging, m_bAllowMouse;
	bool m_bHovered;
	CPoint m_lastMousePoint;
	std::vector<int> m_selectedIndices;
	CFont m_fntInfo;
	CPen m_pen, m_selCrossPen;
	UINT m_flags;
	std::vector<CString> m_labelsX;
	std::vector<CString> m_labelsY;
	std::vector<float> m_marksX;
	std::vector<float> m_marksY;

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};


