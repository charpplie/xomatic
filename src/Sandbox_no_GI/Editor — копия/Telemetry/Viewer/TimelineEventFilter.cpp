////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   TimelineEventFilter.h
//  Version:     v1.00
//  Created:     16/05/2011 by Steve Humphreys
//  Description: Dialog for selecting filters for specified timeline
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TimelineEventFilter.h"

#include "Telemetry\TelemetryObjects.h"
#include "Telemetry\Viewer\TelemetryViewDialog.h"

IMPLEMENT_DYNAMIC(CTimelineEventsFilterDialog, CDialog)

CTimelineEventsFilterDialog::CTimelineEventsFilterDialog(Telemetry::CTimelineRecord* pRecord, CWnd* pParent /*=NULL*/)
: CDialog(CTimelineEventsFilterDialog::IDD, pParent)
, m_pRecord(pRecord)
{

}

CTimelineEventsFilterDialog::~CTimelineEventsFilterDialog()
{
}

void CTimelineEventsFilterDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TEL_PARAMETER, m_parameters);
	DDX_Control(pDX, IDC_TEL_OPERATION, m_operations);
	DDX_Control(pDX, IDC_TEL_VALUE, m_values);
}


BEGIN_MESSAGE_MAP(CTimelineEventsFilterDialog, CDialog)
END_MESSAGE_MAP()

BOOL CTimelineEventsFilterDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	assert(m_pRecord && !m_pRecord->m_timelines.empty());
	Telemetry::CTelemetryTimeline* pTimeline = m_pRecord->m_timelines[0];

	// push parameters for the current timeline
	std::vector<string> parameterNames;
	for(uint32 i=0; i < pTimeline->GetNumberOfStateDescriptions(); ++i)
	{
		stl::push_back_unique(parameterNames, pTimeline->GetStateDescription(i));
	}

	std::vector<string> parameterValues;

	uint32 evntCount = pTimeline->GetEventCount();
	for(uint32 i=0; i<evntCount; ++i)
	{
		Telemetry::STelemetryEvent evnt = pTimeline->GetEvent(i);
	
		for(Telemetry::STelemetryEvent::TStates::const_iterator it = evnt.states.begin(), end = evnt.states.end(); it != end; ++it)
		{
			stl::push_back_unique(parameterValues, it->ToString());	// todo: place into separate vectors by parameter name; show only applicable ones
		}
	}
	for(std::vector<string>::const_iterator it = parameterNames.begin(), end = parameterNames.end(); it != end; ++it)
	{
		m_parameters.AddString(*it);
	}

	// now operations
	string operationNames[] = {"==", "!=", "<", ">", "<=", ">="};
	uint count = sizeof(operationNames) / sizeof(string);
	for(uint i=0; i<count; ++i)
	{
		m_operations.AddString(operationNames[i]);
	}

	// finally parameter values
	for(std::vector<string>::const_iterator it = parameterValues.begin(), end = parameterValues.end(); it != end; ++it)
	{
		m_values.AddString(*it);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CTimelineEventsFilterDialog::OnOK()
{
	int operationIndex = m_operations.GetCurSel();
	CString operationName;
	m_operations.GetLBText(operationIndex, operationName);

	CString paramName;
	int parameterIndex = m_parameters.GetCurSel();
	m_parameters.GetLBText(parameterIndex, paramName);

	CString valueName;
	int valueIndex = m_values.GetCurSel();
	m_values.GetLBText(valueIndex, valueName);

	string temp = operationName;
	Telemetry::STimelineSimpleFilter filter(temp);
	temp = valueName;
	filter.m_comparator = Telemetry::CAnyValue(temp.c_str());
	filter.m_state = paramName;

	for(Telemetry::TTimelines::const_iterator it = m_pRecord->m_timelines.begin(), end = m_pRecord->m_timelines.end(); it != end; ++it)
	{
		(*it)->Filter(filter);
	}

	CTelemetryViewDialog::GetCurrentInstance()->InvalidateTrackList();

	CDialog::OnOK();
}
