// VisualLogTextPreview.h : header file
//
#pragma once

#include "VisualLogCommon.h"
#include "ToolbarDialog.h"



// CSnapTextPreview docking frame
class CVLogTextPreview : public CToolbarDialog
{
	DECLARE_DYNAMIC(CVLogTextPreview)

public:			// Construction & destruction
	CVLogTextPreview();
	virtual ~CVLogTextPreview();

public:			// Dialog Data
	enum { IDD = IDD_DATABASE };

protected:	// Message handlers
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	DECLARE_MESSAGE_MAP()
};
