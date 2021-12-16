// PolyBumpApplicationView.h : interface of the CPolyBumpApplicationView class
//


#pragma once



class CPolyBumpApplicationView : public CView
{
protected: // create from serialization only
	CPolyBumpApplicationView();
	DECLARE_DYNCREATE(CPolyBumpApplicationView)

// Attributes
public:
	CPolyBumpApplicationDoc* GetDocument() const;

// Operations
public:

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:


// Implementation
public:
	virtual ~CPolyBumpApplicationView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in PolyBumpApplicationView.cpp
inline CPolyBumpApplicationDoc* CPolyBumpApplicationView::GetDocument() const
   { return reinterpret_cast<CPolyBumpApplicationDoc*>(m_pDocument); }
#endif

