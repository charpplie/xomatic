////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   TelemetryViewTimelines.h
//  Version:     v1.00
//  Created:     13/05/11 by Steve Humphreys
//  Description: Tree-view of all telemetry timelines
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TelemetryViewTimelines_h__
#define __TelemetryViewTimelines_h__

#include "Controls\TreeCtrlReport.h"

#if _MSC_VER > 1000
#pragma once
#endif

#define TELEMETRY_TIMELINE_ITEM_HEIGHT 18
#define TELEMETRY_TIMELINE_GROUP_HEIGHT 18

// forward declarations.
class CTelemetryEventsView;
class CTelemetryViewDialog;

namespace Telemetry
{
	class CTelemetryTimeline;
	typedef std::vector<CTelemetryTimeline*> TTimelines;

	class CTimelineRecord : public CTreeItemRecord
	{
	public:
		CTimelineRecord(const CString& name="", bool isGroup=false) : m_name(name), m_bIsGroup(isGroup), m_bWasChecked(true)
		{		}
		bool IsGroup() const { return m_bIsGroup; }
		const CString& GetName() const { return m_name; }

	public:
		CString m_name;
		bool m_bIsGroup;
		bool m_bWasChecked;

		TTimelines m_timelines;
	};
}

//////////////////////////////////////////////////////////////////////////
class CTelemetryViewTimelinesCtrl : public CTreeCtrlReport
{
public:
	CTelemetryViewTimelinesCtrl();

	void SetTelemetryViewDialog(CTelemetryViewDialog* pView) { m_pTelemetryViewDialog = pView; }
	void SetEventsCtrl( CTelemetryEventsView *keysCtrl );
	void SyncEventsCtrl();
	//void ExpandNode( IAnimNode *node );
	void SelectNode( const char *sName );

	void SelectInViewPort();
	
	bool IsSelected(Telemetry::CTimelineRecord* pRecord);
	uint GetRecordCount() const { return m_itemInfos.size(); }
	Telemetry::CTimelineRecord* GetRecord(uint index) const
	{
		if(index < GetRecordCount())
			return m_itemInfos[index];

		return NULL;
	}

	float SaveVerticalScrollPos() const;
	void RestoreVerticalScrollPos(float fScrollPos);

	//bool ShouldRenderTimeline(const char* name);
	void UpdateRenderStatus();

	//////////////////////////////////////////////////////////////////////////
	// Callbacks.
	//////////////////////////////////////////////////////////////////////////
	virtual void Reload();
	virtual void OnFillItems();
	virtual void OnItemExpanded( CXTPReportRow *pRow,bool bExpanded );
	virtual void OnSelectionChanged();
	virtual void OnVerticalScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//////////////////////////////////////////////////////////////////////////

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	int ShowPopupMenu(CPoint point, const Telemetry::CTimelineRecord *pItemInfo );

	void SetPopupMenuLock(CMenu* menu);
	
	int GetIconIndexForParam( int nParamId );

	void FillTimelines( Telemetry::CTimelineRecord *pRecord );

	bool HasTimeline(const char *name) const;

	void ExpandTimelines();
	void CollapseTimelines();
	
	void FilterTimeline(Telemetry::CTimelineRecord* pItemInfo);

protected:
	CTelemetryEventsView* m_eventsCtrl;

	// Must not be vector, vector may invalidate pointers on add/remove.
	typedef std::vector<Telemetry::CTimelineRecord*> TRecords;
	TRecords m_itemInfos;

	class CTelemetryViewDialog *m_pTelemetryViewDialog;
};

#endif // __TelemetryViewTimelines_h__
