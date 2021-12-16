////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TelemetryDialog.h
//  Version:     v1.00
//  Created:     17/12/2009 by Sergey Mikhtonyuk
//  Description: Main dialog of telemetry plug-in
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef		_TELEMETRYDIALOG_H__
#define		_TELEMETRYDIALOG_H__

# pragma once

//////////////////////////////////////////////////////////////////////////

#include "TelemetryRepository.h"
#include "Controls/RollupCtrl.h"
#include "ConnectionPanel.h"

//////////////////////////////////////////////////////////////////////////

class CTelemetryDialog : public CWnd
{
	DECLARE_DYNCREATE(CTelemetryDialog)

public:

	static void RegisterViewClass();

	CTelemetryDialog();

	~CTelemetryDialog();

protected:

	DECLARE_MESSAGE_MAP()

	afx_msg void OnSize(UINT nType, int cx, int cy);

private:
	CTelemetryRepository m_repository;

	CRollupCtrl m_rollupCtrl;
	CConnectionPanel m_connectionPanel;
};

//////////////////////////////////////////////////////////////////////////

#endif // __TELEMETRYDIALOG_H__
