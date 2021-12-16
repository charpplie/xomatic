////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   TimelineEventFilter.h
//  Version:     v1.00
//  Created:     16/05/2011 by Steve Humphreys.
//  Description: Dialog for selecting filters for specified timeline
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TIMELINEEVENTFILTER_H__
#define __TIMELINEEVENTFILTER_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "afxcmn.h"
#include "Resource.h"

#include "TelemetryViewTimelines.h"

class CTimelineEventsFilterDialog : public CDialog
{
	DECLARE_DYNAMIC(CTimelineEventsFilterDialog)

public:
	CTimelineEventsFilterDialog(Telemetry::CTimelineRecord* pRecord, CWnd* pParent = NULL);   // standard constructor
	virtual ~CTimelineEventsFilterDialog();

	// Dialog Data
	enum { IDD = IDD_TELEMETRY_FILTER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	Telemetry::CTimelineRecord* m_pRecord;
	
	CComboBox m_parameters;
	CComboBox m_operations;
	CComboBox m_values;
};

#endif // __TIMELINEEVENTFILTER_H__