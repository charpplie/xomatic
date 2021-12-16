////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 1999-2010.
// -------------------------------------------------------------------------
//  File Name        : JointListControl.cpp
//  Version          : v1.00
//  Created          : 14/6/2010 by Pau Novau
//  Description      : Control that holds a hierarchical list of joint names
//                     for a character's skeleton and allows the user to
//                     interactively select some of the joints and mark them
//                     as enabled or disabled.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/JointListControl.h"
#include "LMGEditor/JointListRecord.h"


IMPLEMENT_DYNCREATE( CJointListControl, CXTPReportControl )

BEGIN_MESSAGE_MAP( CJointListControl, CXTPReportControl )
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


CJointListControl::CJointListControl()
: m_pListener( NULL )
{

}


CJointListControl::~CJointListControl()
{

}


BOOL CJointListControl::Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext )
{
	dwStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	BOOL createSuccess = CXTPReportControl::Create( dwStyle, rect, pParentWnd, nID, pContext );
	if ( ! createSuccess )
	{
		return FALSE;
	}

	ModifyStyleEx( 0, WS_EX_STATICEDGE );

	CXTPReportColumn* pJointNameColumn = AddColumn( new CXTPReportColumn( COLUMN_JOINT_NAME, _T( "Name" ), 150, TRUE, XTP_REPORT_NOICON, TRUE, TRUE) );
	pJointNameColumn->SetTreeColumn( true );
	pJointNameColumn->SetExpanded( TRUE );
	pJointNameColumn->SetSortable( TRUE );
	pJointNameColumn->SetAllowDrag( FALSE );

	GetColumns()->SetSortColumn( pJointNameColumn, true );
	GetReportHeader()->AllowColumnRemove( FALSE );
	ShadeGroupHeadings( FALSE );
	SkipGroupsFocus( TRUE );
	SetMultipleSelection( TRUE );
	SetGridStyle( TRUE, xtpReportGridNoLines );
	SetGridStyle( FALSE, xtpReportGridNoLines );

	return TRUE;
}


void CJointListControl::SetListener( IJointListRecordsListener* pListener )
{
	m_pListener = pListener;
}


void CJointListControl::UpdateJoints( ISkeletonPose* pSkeletonPose )
{
	if ( pSkeletonPose == NULL )
	{
		Clear();
		return;
	}

	BeginUpdate();
	Clear();

	const uint32 jointCount = pSkeletonPose->GetJointCount();
	for ( uint32 jointId = 0; jointId < jointCount; ++jointId )
	{
		CString jointName = pSkeletonPose->GetJointNameByID( jointId );
		CString parentJointName = "";

		int16 parentJointId = pSkeletonPose->GetParentIDByID( jointId );
		bool hasParentJoint = ( 0 <= parentJointId );
		if ( hasParentJoint )
		{
			parentJointName = pSkeletonPose->GetJointNameByID( parentJointId );
		}

		AddJointRecord( jointName, parentJointName );
	}

	EndUpdate();
	Populate();

	ExpandAll( TRUE );
}


void CJointListControl::Clear()
{
	m_jointLists.clear();
	GetRecords()->RemoveAll();
}


void CJointListControl::SetStatusFromJointList( const std::vector< CString >& jointNames )
{
	if ( jointNames.empty() )
	{
		EnableAllJoints();
		return;
	}

	for ( JointListMap::const_iterator cit = m_jointLists.begin(); cit != m_jointLists.end(); ++cit )
	{
		CJointListRecord* pRecord = cit->second;
		assert( pRecord != NULL );

		const CString& currentJointName = pRecord->GetName();

		bool isJointInList = stl::find( jointNames, currentJointName );
		pRecord->SetEnabled( isJointInList );
	}

	if ( m_pListener != NULL )
	{
		m_pListener->OnEnabledJointsChanged();
	}
}


bool CJointListControl::AreAllJointsEnabled() const
{
	for ( JointListMap::const_iterator cit = m_jointLists.begin(); cit != m_jointLists.end(); ++cit )
	{
		CJointListRecord* pRecord = cit->second;
		assert( pRecord != NULL );

		bool enabled = pRecord->IsEnabled();
		if ( ! enabled )
		{
			return false;
		}
	}

	return true;
}


void CJointListControl::GetEnabledJointsNames( std::vector< CString >& jointNamesOut ) const
{
	for ( JointListMap::const_iterator cit = m_jointLists.begin(); cit != m_jointLists.end(); ++cit )
	{
		CJointListRecord* pRecord = cit->second;
		assert( pRecord != NULL );

		bool enabled = pRecord->IsEnabled();
		if ( enabled )
		{
			const CString& jointName = pRecord->GetName();
			jointNamesOut.push_back( jointName );
		}
	}
}


void CJointListControl::AddJointRecord( const CString& jointName, const CString& parentJointName )
{
	CJointListRecord* pParentRecord = GetJointRecord( parentJointName );
	CJointListRecord* pRecord = new CJointListRecord( jointName );
	m_jointLists[ jointName ] = pRecord;

	if ( pParentRecord == NULL )
	{
		AddRecord( pRecord );
	}
	else
	{
		pParentRecord->GetChilds()->Add( pRecord );
	}
}


CJointListRecord* CJointListControl::GetJointRecord( const CString& jointName )
{
	if ( jointName.IsEmpty() )
	{
		return NULL;
	}

	CJointListRecord* pRecord = stl::find_in_map( m_jointLists, jointName, NULL );
	return pRecord;
}


void CJointListControl::EnableSelectedJoints()
{
	SetSelectedJointsEnabledState( true );
}


void CJointListControl::DisableSelectedJoints()
{
	SetSelectedJointsEnabledState( false );
}


void CJointListControl::SetSelectedJointsEnabledState( bool enabled )
{
	CXTPReportSelectedRows* pSelectedRows = GetSelectedRows();
	assert( pSelectedRows != NULL );

	for ( int i = 0; i < pSelectedRows->GetCount(); ++i )
	{
		CXTPReportRow* pRow = pSelectedRows->GetAt( i );
		assert( pRow != NULL );

		CXTPReportRecord* pRecord = pRow->GetRecord();
		assert( pRecord != NULL );

		CJointListRecord* pJointRecord = DYNAMIC_DOWNCAST( CJointListRecord, pRecord );
		assert( pJointRecord != NULL );
		if ( pJointRecord  != NULL )
		{
			pJointRecord->SetEnabled( enabled );		
		}

		bool isExpanded = pJointRecord->IsExpanded();
		if ( ! isExpanded )
		{
			// Childs of not expanded records don't have rows, thus cannot be selected, but we want
			// to select all children any way if we select a not expanded item.

			pJointRecord->SetEnabledRecursive( enabled );
		}
	}

	if ( m_pListener != NULL )
	{
		m_pListener->OnEnabledJointsChanged();
	}
}


void CJointListControl::EnableAllJoints()
{
	for ( JointListMap::const_iterator cit = m_jointLists.begin(); cit != m_jointLists.end(); ++cit )
	{
		CJointListRecord* pRecord = cit->second;
		assert( pRecord != NULL );

		pRecord->SetEnabled( true );
	}

	if ( m_pListener != NULL )
	{
		m_pListener->OnEnabledJointsChanged();
	}
}


void CJointListControl::OnLButtonDown( UINT nFlags, CPoint point )
{
	{
		// Workaround to keep default control behaviour for node children collapse/expand
		// and still get nice multi selection functionality
		__super::OnLButtonDown( nFlags, point );
		__super::OnLButtonUp( nFlags, point );
	}

	SelectJointsFromMouseClick( nFlags, point, ChildSelectMode_Ignore );
}


void CJointListControl::OnLButtonUp( UINT nFlags, CPoint point )
{
	// Intentionally overriding parent behaviour so that records don't get unselected after double-click.
}


void CJointListControl::OnLButtonDblClk( UINT nFlags, CPoint ptDblClick )
{
	SelectJointsFromMouseClick( nFlags, ptDblClick, ChildSelectMode_Recursive );
}


bool CJointListControl::SelectJointsFromMouseClick( UINT nFlags, CPoint point, ChildSelectMode childSelectMode )
{
	CXTPReportRow* pRow = HitTest( point );
	if ( pRow == NULL )
	{
		return false;
	}

	CXTPReportRecord* pRecord = pRow->GetRecord();
	assert( pRecord != NULL );

	bool controlPressed = nFlags & MK_CONTROL;

	bool clearSelection = ( ! controlPressed );
	if ( clearSelection )
	{
		ClearSelection();
	}

	SelectMode selectMode = SelectMode_Add;
	if ( controlPressed )
	{
		selectMode = pRow->IsSelected() ? SelectMode_Add : SelectMode_Remove ;
	}

	SelectJointsFromRecord( pRecord, selectMode, childSelectMode );
	return true;
}


void CJointListControl::SelectJointsFromRecord( CXTPReportRecord* pRecord, SelectMode selectMode, ChildSelectMode childSelectMode )
{
	if ( pRecord == NULL )
	{
		return;
	}

	CXTPReportRows* pRows = GetRows();
	assert( pRows != NULL );

	CXTPReportRow* pRow = pRows->Find( pRecord );

	SelectJointFromRow( pRow, selectMode );

	if ( childSelectMode == ChildSelectMode_Recursive )
	{
		CXTPReportRecords* pChildRecords = pRecord->GetChilds();
		if ( pChildRecords == NULL )
		{
			return;
		}

		for ( int i = 0; i < pChildRecords->GetCount(); ++i )
		{
			CXTPReportRecord* pChildRecord = pChildRecords->GetAt( i );
			SelectJointsFromRecord( pChildRecord, selectMode, childSelectMode );
		}
	}
}


void CJointListControl::SelectJointFromRow( CXTPReportRow* pRow, SelectMode selectMode )
{
	if ( pRow == NULL )
	{
		return;
	}

	CXTPReportSelectedRows* pSelectedRows = GetSelectedRows();
	assert( pSelectedRows != NULL );

	switch ( selectMode )
	{
	case SelectMode_Add:
		pSelectedRows->Add( pRow );
		break;

	case SelectMode_Remove:
		pSelectedRows->Remove( pRow );
		break;

	default:
		assert( false );
		break;
	}
}


void CJointListControl::ClearSelection()
{
	CXTPReportSelectedRows* pSelectedRows = GetSelectedRows();
	assert( pSelectedRows != NULL );

	pSelectedRows->Clear();
}



void CJointListControl::OnRButtonUp( UINT nFlags, CPoint point )
{
	CMenu menu;
	menu.CreatePopupMenu();

	enum JointListControlPopupCommands
	{
		ENABLE_SELECTED_JOINTS = 1,
		DISABLE_SELECTED_JOINTS,
		ENABLE_ALL_JOINTS,
	};

	menu.AppendMenu( MF_STRING, ENABLE_SELECTED_JOINTS, "Enable Selected" );
	menu.AppendMenu( MF_STRING, DISABLE_SELECTED_JOINTS, "Disable Selected" );
	menu.AppendMenu( MF_STRING, ENABLE_ALL_JOINTS, "Enable All" );

	CPoint p;
	::GetCursorPos( &p );
	int selectedCmd = ::TrackPopupMenuEx( menu.GetSafeHmenu(), TPM_LEFTBUTTON | TPM_RETURNCMD, p.x, p.y, GetSafeHwnd(), NULL );
	
	switch ( selectedCmd )
	{
	case ENABLE_SELECTED_JOINTS:
		EnableSelectedJoints();
		break;

	case DISABLE_SELECTED_JOINTS:
		DisableSelectedJoints();
		break;

	case ENABLE_ALL_JOINTS:
		EnableAllJoints();
		break;
	}

	__super::OnRButtonUp( nFlags, point );
}