////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CFillSliderCtrl.h
//  Version:     v1.00
//  Created:     24/9/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: Slider like control, with bar filled from the left.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CFillSliderCtrl_h__
#define __CFillSliderCtrl_h__
#pragma once

#include "SliderCtrlEx.h"

// This notification (Sent with WM_COMMAND) sent when slider changes position.
#define WMU_FS_CHANGED          (WM_USER+100)
#define WMU_FS_LBUTTONDOWN      (WM_USER+101)
#define WMU_FS_LBUTTONUP        (WM_USER+102)

/////////////////////////////////////////////////////////////////////////////
// CFillSliderCtrl window
class SANDBOX_API CFillSliderCtrl : public CSliderCtrlEx
{
public:
	enum EFillStyle
	{
		// Fill vertically instead of horizontally.
		eFillStyle_Vertical = 1<<0,

		// Fill with a gradient instead of a solid.
		eFillStyle_Gradient = 1<<1,

		// Fill the background instead of the slider.
		eFillStyle_Background = 1<<2,

		// Fill with color hue gradient instead of a solid.
		eFillStyle_ColorHueGradient = 1<<3,
	};

private:
	DECLARE_DYNCREATE(CFillSliderCtrl);
	// Construction
public:
	typedef Functor1<CFillSliderCtrl*>	UpdateCallback;

	CFillSliderCtrl();
	~CFillSliderCtrl();

	void SetFilledLook( bool bFilled );
	void SetFillStyle(uint32 style) { m_fillStyle = style; }
	void SetFillColors(COLORREF start, COLORREF end) { m_fillColorStart = start; m_fillColorEnd = end; }

	// Operations
public:
	//! Set current value.
	virtual void SetValue( float val );

	// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	void NotifyUpdate( bool tracking );
	void ChangeValue( int sliderPos,bool bTracking );

private:
	void DrawFill(CDC &dc, CRect &rect);

protected:
	bool m_bFilled;
	uint32 m_fillStyle;
	COLORREF m_fillColorStart;
	COLORREF m_fillColorEnd;
	CPoint m_mousePos;
};

#endif //__CFillSliderCtrl_h__
