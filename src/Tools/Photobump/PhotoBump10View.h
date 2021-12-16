// PhotoBump10View.h : interface of the CPhotoBump10View class
//

#pragma once

//////////////////////////////////////////////////////////////////////////
class CVideo;
class CUtils;

//////////////////////////////////////////////////////////////////////////
#define DEFAULT_CAMERA_SPEED	10
#define DEFAULT_CAMERA_POS		vector3f(0.0,1.70,5.0)
#define DEFAULT_CAMERA_ANGLES	vector3f(0,0,0)

#define DEFAULT_GROUND_SIZE		100.0
#define DEFAULT_GRID_SIZE			2.0

class CPhotoBump10Doc;

//////////////////////////////////////////////////////////////////////////
class CPhotoBump10View : public CView
{
protected: // create from serialization only
	CPhotoBump10View();
	DECLARE_DYNCREATE(CPhotoBump10View)

// Attributes
public:
	CPhotoBump10Doc* GetDocument() const;

// Operations
public:

	void	SetGLCurrent()	{ wglMakeCurrent(this->GetWindowDC()->GetSafeHdc(), GetRC()); }
	void	DumpError()			{ m_bDumpError=true; }

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:

protected:

// Implementation
public:
	virtual ~CPhotoBump10View();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	void	MouseTranslate(CPoint point);
	void	MouseRotate(CPoint point);
	void	MouseForward(CPoint point);

	afx_msg void OnKeyDown(UINT nChar,UINT nRepCnt,UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags,CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags,CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags,CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags,CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta,CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	bool SetPixelFrmt(HDC hdc);
	bool CreateRenderingContext(HDC hdc);
	HGLRC GetRC();
	void	DrawScene();

	CPoint m_ptMousePos;
	CRect m_ClientRect;
	int m_GLPixelIndex; 
	HGLRC m_hGLContext;

	CVideo	*m_pVideo;
	CUtils	*m_pUtils;
	CCamera	m_Camera;

	CPoint		m_ptDesktop,m_ptLogical;
	int				m_nShowCameraInfo;
	bool			m_bRotateCamera,m_bTranslateCamera,m_bForward;
	bool			m_bSetAsReference,m_bLButtonDown;
	int				m_nSelectedCamera;
	bool			m_bDumpError,m_bDrawing;
	bool			m_bUseGLLight;

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnDestroy();
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg void	OnPaint();	
};

#ifndef _DEBUG  // debug version in PhotoBump10View.cpp
inline CPhotoBump10Doc* CPhotoBump10View::GetDocument() const
   { return reinterpret_cast<CPhotoBump10Doc*>(m_pDocument); }
#endif

