// VisualLogTextPreview.cpp : implementation file
//

#include "stdafx.h"
#include "VisualLogTextPreview.h"


// CSnapTextPreview
IMPLEMENT_DYNAMIC(CVLogTextPreview, CToolbarDialog)

BEGIN_MESSAGE_MAP(CVLogTextPreview, CToolbarDialog)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CVLogTextPreview
CVLogTextPreview::CVLogTextPreview()
{
}

CVLogTextPreview::~CVLogTextPreview()
{
}



// CVLogTextPreview message handlers
BOOL CVLogTextPreview::OnEraseBkgnd(CDC *pDC)
{
	return TRUE;
}
