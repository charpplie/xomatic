////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEventLogView.h
//  Version:     v1.00
//  Created:     28/03/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------  
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __UIEventLogView__H__
#define __UIEventLogView__H__

// #include "Dialogs/BaseFrameWnd.h"
#include <IFlashUI.h>

struct IUIElement;
struct IUIAction;
struct IUIEventSystem;

class CUIEventLogView
	: public CXTResizeDialog
{
	DECLARE_DYNCREATE( CUIEventLogView )

public:
	CUIEventLogView( CWnd* pParent = NULL );
	virtual ~CUIEventLogView();

	enum { IDD = IDD_ERROR_REPORT };

	void ClearLog();
	void AddLogEvent(const IFlashUI::SUILogEvent& event);();

protected:
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();

	afx_msg void OnSize( UINT nType,int cx,int cy );
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	void ReloadLogEvents();

private:
	std::vector<IFlashUI::SUILogEvent> m_LogEvents;

	CXTPReportControl m_wndList;
	CImageList m_imageList;
};


#endif // __UIEventLogView__H__