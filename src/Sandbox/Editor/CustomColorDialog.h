#if !defined(AFX_CUSTOMCOLORDIALOG_H__E1491910_A0B4_48B3_9556_CB079E1EAC7F__INCLUDED_)
#define AFX_CUSTOMCOLORDIALOG_H__E1491910_A0B4_48B3_9556_CB079E1EAC7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CustomColorDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCustomColorDialog dialog

class CCustomColorDialog : public CColorDialog
{
	DECLARE_DYNAMIC(CCustomColorDialog)
public:
	typedef Functor1<COLORREF> ColorChangeCallback;

	CCustomColorDialog(COLORREF clrInit = 0, DWORD dwFlags = 0,CWnd* pParentWnd = NULL);
	~CCustomColorDialog();

	void SetColorChangeCallback( ColorChangeCallback cb ) { m_callback = cb; };

public:
	static COLORREF LinearToGamma( ColorF col )
	{
		float gamma = gEnv->pRenderer->EF_Query( EFQ_sLinearSpaceShadingEnabled ) ? 1.0f/2.2f : 1.0f;
		return RGB( FtoI( clamp_tpl( pow( col.r, gamma ), 0.0f, 1.0f ) * 255.0f ),
		            FtoI( clamp_tpl( pow( col.g, gamma ), 0.0f, 1.0f ) * 255.0f ),
		            FtoI( clamp_tpl( pow( col.b, gamma ), 0.0f, 1.0f ) * 255.0f ) );
	}

	static ColorF GammaToLinear( COLORREF col )
	{
		float gamma = gEnv->pRenderer->EF_Query( EFQ_sLinearSpaceShadingEnabled ) ? 2.2f : 1.0f;
		return ColorF( pow( GetRValue( col ) / 255.0f, gamma ),
		               pow( GetGValue( col ) / 255.0f, gamma ),
		               pow( GetBValue( col ) / 255.0f, gamma ) );
	}

protected:
	//{{AFX_MSG(CCustomColorDialog)
	afx_msg void OnPickColor();
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void PickMode( bool bEnable );
	void OnColorChange( COLORREF col );

	CButton m_pickColor;
	bool m_bPickMode;
	HCURSOR m_pickerCusror;
	ColorChangeCallback m_callback;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CUSTOMCOLORDIALOG_H__E1491910_A0B4_48B3_9556_CB079E1EAC7F__INCLUDED_)
