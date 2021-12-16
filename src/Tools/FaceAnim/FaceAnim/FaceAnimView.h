// FaceAnimView.h : interface of the CFaceAnimView class
//


#pragma once


class CFaceAnimView : public CView
{
protected: // create from serialization only
	CFaceAnimView();
	DECLARE_DYNCREATE(CFaceAnimView)

// Attributes
public:
	CFaceAnimDoc* GetDocument() const;

// Operations
public:

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CFaceAnimView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in FaceAnimView.cpp
inline CFaceAnimDoc* CFaceAnimView::GetDocument() const
   { return reinterpret_cast<CFaceAnimDoc*>(m_pDocument); }
#endif

