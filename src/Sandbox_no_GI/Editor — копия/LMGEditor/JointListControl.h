////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 1999-2010.
// -------------------------------------------------------------------------
//  File Name        : JointListControl.h
//  Version          : v1.00
//  Created          : 14/6/2010 by Pau Novau
//  Description      : Control that holds a hierarchical list of joint names
//                     for a character's skeleton and allows the user to
//                     interactively select some of the joints and mark them
//                     as enabled or disabled.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Joint_List_Control__h__
#define __Joint_List_Control__h__
#pragma once

class CJointListRecord;


struct IJointListRecordsListener
{
	virtual void OnEnabledJointsChanged() = 0;
};



class CJointListControl
	: public CXTPReportControl
{
protected:
	static const int COLUMN_JOINT_NAME = 0;

public:
	DECLARE_DYNCREATE( CJointListControl )

	CJointListControl();
	virtual ~CJointListControl();

	virtual BOOL Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL );

	void SetListener( IJointListRecordsListener* pListener );

	void SetStatusFromJointList( const std::vector< CString >& jointNames );

	void UpdateJoints( ISkeletonPose* pSkeletonPose );
	void Clear();

	bool AreAllJointsEnabled() const;
	void GetEnabledJointsNames( std::vector< CString >& jointNamesOut ) const;

protected:
	afx_msg void OnLButtonDblClk( UINT nFlags, CPoint ptDblClick );
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnLButtonUp( UINT nFlags, CPoint point );
	afx_msg void OnRButtonUp( UINT nFlags, CPoint point );
	DECLARE_MESSAGE_MAP()

	virtual void AddJointRecord( const CString& joitnName, const CString& parentJointName );

	CJointListRecord* GetJointRecord( const CString& jointName );

	void EnableAllJoints();
	void EnableSelectedJoints();
	void DisableSelectedJoints();
	void SetSelectedJointsEnabledState( bool enabled );
	
	typedef enum
	{
		SelectMode_Add,
		SelectMode_Remove,
	} SelectMode;

	typedef enum
	{
		ChildSelectMode_Ignore,
		ChildSelectMode_Recursive,
	} ChildSelectMode;

	bool SelectJointsFromMouseClick( UINT nFlags, CPoint point, ChildSelectMode childSelectMode );
	void SelectJointsFromRecord( CXTPReportRecord* pRecord, SelectMode selectMode, ChildSelectMode childSelectMode );
	void ClearSelection();

private:
	void SelectJointFromRow( CXTPReportRow* pRow, SelectMode selectMode );

private:
	IJointListRecordsListener* m_pListener;

	typedef std::map< CString, CJointListRecord* > JointListMap;
	JointListMap m_jointLists;
};

#endif
