////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   ConnectionPanel.h
//  Version:     v1.00
//  Created:     04/02/2010 by Sergey Mikhtonyuk
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef		_CONNECTIONPANEL_H__
#define		_CONNECTIONPANEL_H__

#include "TelemetryRepository.h"
#include "PipeClient.h"
#include "PipeMessageParser.h"

# pragma once

class CConnectionPanel : public CDialog, public IEditorNotifyListener
{
public:

	enum { IDD = IDD_PANEL_CONNECTION };

	CConnectionPanel(CTelemetryRepository& repo, CWnd* pParent = NULL);

	virtual ~CConnectionPanel();

	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

protected:

	bool OpenFile(char* filename, unsigned int bufSize, const char* filter, const char* title);

	virtual BOOL OnInitDialog();

	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()

	afx_msg void OnOpenClicked();

	afx_msg void OnConnectClicked();

	afx_msg void OnSetMarkersMode();

	afx_msg void OnSetDensityMode();

private:

	string m_statsToolPath;

	CTelemetryRepository& m_repository;

	CPipeClient m_pipe;
	CPipeMessageParser m_pipeMsgClient;

	static const uint32 PIPE_BUFFER_SIZE = 100*1024;

	CButton m_btnConnect;
	CButton m_btnOpen;
	CButton m_rbMarkers;
	CButton m_rbDensity;
};

#endif // __CONNECTIONPANEL_H__
