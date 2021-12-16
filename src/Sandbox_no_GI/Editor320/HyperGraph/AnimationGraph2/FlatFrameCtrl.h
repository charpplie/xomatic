#ifndef __FLATFRAMECTRL_H__
#define __FLATFRAMECTRL_H__

#pragma once

#include "Resource.h"
#include "Controls\TemplDef.h" // message map extensions for templates
#include "ToolbarDialog.h"
#include "Controls\PropertyCtrl.h"


template < class BASE_TYPE >
class CFlatFramedCtrl2 : public BASE_TYPE
{
public:
	CFlatFramedCtrl2() {}
	template < typename T > CFlatFramedCtrl2( T param ) : BASE_TYPE(param) {}
protected:
	//{{AFX_MSG(CFlatFramedCtrl)
	afx_msg void OnNcPaint();
	//}}AFX_MSG
	DECLARE_TEMPLATE_MESSAGE_MAP()
};

BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM( class BASE_TYPE, CFlatFramedCtrl2< BASE_TYPE >, BASE_TYPE )
//{{AFX_MSG_MAP(CFlatFramedCtrl)
ON_WM_NCPAINT()
//}}AFX_MSG_MAP
END_TEMPLATE_MESSAGE_MAP_CUSTOM()

template< class BASE_TYPE >
void CFlatFramedCtrl2< BASE_TYPE >::OnNcPaint()
{
	__super::OnNcPaint();
	//	if ( !IsAppThemed() )
	//	{
	CWindowDC dc( this );
	CRect rc; dc.GetClipBox( rc );
	COLORREF color = GetSysColor(COLOR_3DSHADOW);
	dc.Draw3dRect( rc, color, color );
	//	}
}

#endif