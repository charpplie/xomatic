////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   AnimationListControl.cpp
//  Version:     v1.00
//  Created:     15/07/2009 by Pau Novau
//  Description: Control that displays a list of animations.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\LMGEditor/AnimationListControl.h"

#include "..\LMGEditor/IAnimationNameDragDropListener.h"
#include "..\LMGEditor/IAnimationNameSelectionChangedListener.h"
#include "..\LMGEditor/AnimationListRecord.h"

#include "ICryAnimation.h"

IMPLEMENT_DYNCREATE( CAnimationListControl, CXTPReportControl )

BEGIN_MESSAGE_MAP( CAnimationListControl, CXTPReportControl )
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

CAnimationListControl::CAnimationListControl()
: m_dragging( false )
, m_pSelectionChangeListener( NULL )
{
	m_defaultArrowCursor = LoadCursor( NULL, IDC_ARROW );
	m_dropAllowedCursor = LoadCursor( LoadLibrary( "Ole32.dll" ), MAKEINTRESOURCE( 2 ) );   
	m_dropNotAllowedCursor = LoadCursor( NULL, IDC_NO );
}


CAnimationListControl::~CAnimationListControl()
{

}


BOOL CAnimationListControl::Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext )
{
	dwStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	BOOL createSuccess =CXTPReportControl::Create( dwStyle, rect, pParentWnd, nID, pContext );
	if ( ! createSuccess )
	{
		return FALSE;
	}

	ModifyStyleEx( 0, WS_EX_STATICEDGE );

	CXTPReportColumn* pAnimationNameColumn = AddColumn( new CXTPReportColumn( COLUMN_ANIMATION_NAME, _T( "Animation" ), 150, TRUE, XTP_REPORT_NOICON, TRUE, TRUE) );
	pAnimationNameColumn->SetTreeColumn( true );
	pAnimationNameColumn->SetSortable( TRUE );

// 	CXTPReportColumn* pAnimationLenghtColumn = AddColumn( new CXTPReportColumn( COLUMN_ANIMATION_LENGTH, _T( "Frame" ), 50, TRUE, XTP_REPORT_NOICON, TRUE, TRUE) );
// 	pAnimationLenghtColumn->SetAlignment( DT_CENTER );

	GetColumns()->SetSortColumn( pAnimationNameColumn, true );
	GetReportHeader()->AllowColumnRemove( FALSE );
	ShadeGroupHeadings( FALSE );
	SkipGroupsFocus( TRUE );
	SetMultipleSelection( FALSE );
	SetGridStyle( TRUE, xtpReportGridNoLines );
	SetGridStyle( FALSE, xtpReportGridNoLines );

	return TRUE;
}

void CAnimationListControl::SetSelectionChangedListener( IAnimationNameSelectionChangedListener* pSelectionChangedListener )
{
	m_pSelectionChangeListener = pSelectionChangedListener;
}

void CAnimationListControl::OnSelectionChanged()
{
	__super::OnSelectionChanged();

	if ( m_pSelectionChangeListener != NULL )
	{
		const CAnimationListRecord* pSelectedAnimationListRecord = GetSelectedAnimationListRecord();
		if ( pSelectedAnimationListRecord == NULL )
		{
			m_pSelectionChangeListener->AnimationNameSelectionChanged( "" );
			return;
		}

		if ( ! pSelectedAnimationListRecord->IsGroup() )
		{
			const CString& selectedAnimationName = pSelectedAnimationListRecord->GetName();
			m_pSelectionChangeListener->AnimationNameSelectionChanged( selectedAnimationName );
		}
		else
		{
			m_pSelectionChangeListener->AnimationNameSelectionChanged( "" );
		}
	}
}

void CAnimationListControl::UpdateAnimations( IAnimationSet* pAnimationSet )
{
	if ( pAnimationSet == NULL )
	{
		Clear();
		return;
	}

	BeginUpdate();
	Clear();

	uint32 numAnimations = pAnimationSet->GetAnimationCount();

	for ( uint32 animationId = 0; animationId < numAnimations; animationId++ )
	{
		AddAnimationRecord( pAnimationSet, animationId );	
	}

	EndUpdate();
	Populate();
}

void CAnimationListControl::AddAnimationRecord( IAnimationSet* pAnimationSet, int animationId )
{
	uint32 flags = pAnimationSet->GetAnimationFlags( animationId );
	bool isLmg = ( flags & CA_ASSET_LMG );
	bool isPmg = ( flags & CA_ASSET_PMG );
	
	bool isSupportedAnimation = ( ! isLmg && ! isPmg );
	if ( ! isSupportedAnimation )
	{
		return;
	}

	const char* name = pAnimationSet->GetNameByAnimID( animationId );

	CAnimationListRecord* pParent = CreateGroupRecord( name );
	CAnimationListRecord* pRecord = new CAnimationListRecord( false, name );

	if ( pParent == NULL )
	{
		AddRecord( pRecord );
	}
	else
	{
		pParent->GetChilds()->Add( pRecord );
	}
}


CAnimationListRecord* CAnimationListControl::CreateGroupRecord( const CString& animationName )
{
	CString groupName = GetGroupName( animationName );
	if ( groupName.IsEmpty() )
	{
		return NULL;
	}

	CAnimationListRecord* pRecord = stl::find_in_map( m_animationRecordGroups, groupName, NULL );
	if ( pRecord == NULL )
	{
		pRecord = new CAnimationListRecord( true, groupName );
		AddRecord( pRecord );
		m_animationRecordGroups[ groupName ] = pRecord;
	}

	return pRecord;
}

CString CAnimationListControl::GetGroupName( const CString& animationName )
{
	CString groupName( animationName );
	int index = 0;
	while ( index == 0 )
	{
		groupName.Tokenize( "_", index );
	}
	index = std::max< int >( index - 1, 0 );
	groupName = groupName.Left( index );

	return groupName;
}

void CAnimationListControl::Clear()
{
	m_animationRecordGroups.clear();
	GetRecords()->RemoveAll();
}

void CAnimationListControl::OnMouseMove( UINT nFlags, CPoint point )
{
	if ( IsDragging() )
	{
		UpdateDragAndDrop( point );
	}
	else
	{
		CXTPReportControl::OnMouseMove( nFlags, point );
	}
}

void CAnimationListControl::OnLButtonUp( UINT nFlags, CPoint point )
{
	if ( IsDragging() )
	{
		EndDragAndDrop( point );
	}
	else
	{
		CXTPReportControl::OnLButtonUp( nFlags, point );
	}
}

const CString& CAnimationListControl::GetSelectedAnimationName() const
{
	const CAnimationListRecord* pSelectedAnimationListRecord = GetSelectedAnimationListRecord();
	if ( pSelectedAnimationListRecord == NULL )
	{
		static CString dummyString;
		return dummyString;
	}
	
	return pSelectedAnimationListRecord->GetName();
}

const CAnimationListRecord* CAnimationListControl::GetSelectedAnimationListRecord() const
{
	CXTPReportSelectedRows* pSelectedRows = GetSelectedRows();

	POSITION selectedRowPosition = pSelectedRows->GetFirstSelectedRowPosition();
	if ( selectedRowPosition == NULL )
	{
		return NULL;
	}

	CXTPReportRow* pSelectedRow = pSelectedRows->GetNextSelectedRow( selectedRowPosition );
	assert( pSelectedRow != NULL );
	if ( pSelectedRow == NULL )
	{
		return NULL;
	}

	CXTPReportRecord* pSelectedRecord = pSelectedRow->GetRecord();
	assert( pSelectedRecord != NULL );
	if ( pSelectedRecord == NULL )
	{
		return NULL;
	}

	CAnimationListRecord* pSelectedAnimationRecord = DYNAMIC_DOWNCAST( CAnimationListRecord, pSelectedRecord );
	assert( pSelectedAnimationRecord != NULL );
	if ( pSelectedAnimationRecord == NULL )
	{
		return NULL;
	}

	return pSelectedAnimationRecord;
}

void CAnimationListControl::OnLButtonDown( UINT nFlags, CPoint point )
{
	CXTPReportControl::OnLButtonDown( nFlags, point );
}

void CAnimationListControl::OnBeginDrag( CPoint point )
{
	const CAnimationListRecord* pSelectedAnimationListRecord = GetSelectedAnimationListRecord();
	if ( pSelectedAnimationListRecord == NULL )
	{
		return;
	}

	bool isDraggable = ( ! pSelectedAnimationListRecord->IsGroup() );
	if ( ! isDraggable )
	{
		return;
	}

	StartDragAndDrop( point );
}


void CAnimationListControl::AddDragDropListener( IAnimationNameDragDropListener* pDragDropListener )
{
	m_dragDropListeners.insert( pDragDropListener );
}

void CAnimationListControl::RemoveDragDropListener( IAnimationNameDragDropListener* pDragDropListener )
{
	stl::find_and_erase( m_dragDropListeners, pDragDropListener );
}

void CAnimationListControl::StartDragAndDrop( const CPoint& clientPoint )
{
	assert( ! m_dragging );

	if ( m_dragDropListeners.empty() )
	{
		return;
	}

	m_dragging = true;
	SetCapture();

	CPoint screenPoint( clientPoint );
	ClientToScreen( &screenPoint );

	const CString& selectedAnimationName = GetSelectedAnimationName();

	for ( TDragDropSet::iterator it = m_dragDropListeners.begin(); it != m_dragDropListeners.end(); ++it )
	{
		IAnimationNameDragDropListener* pDragDropListener = *it;
		pDragDropListener->DragStarted( screenPoint, selectedAnimationName );
	}
}

void CAnimationListControl::UpdateDragAndDrop( const CPoint& clientPoint )
{
	assert( m_dragging );
	if ( m_dragDropListeners.empty() )
	{
		SetCursor( m_dropNotAllowedCursor );
		return;
	}


	CPoint screenPoint( clientPoint );
	ClientToScreen( &screenPoint );

	const CString& selectedAnimationName = GetSelectedAnimationName();

	bool isValidDropPoint = false;
	for ( TDragDropSet::iterator it = m_dragDropListeners.begin(); it != m_dragDropListeners.end(); ++it )
	{
		IAnimationNameDragDropListener* pDragDropListener = *it;
		isValidDropPoint |= pDragDropListener->IsValidDropPoint( screenPoint, selectedAnimationName );
	}

	if ( isValidDropPoint )
	{
		SetCursor( m_dropAllowedCursor );
	}
	else
	{
		SetCursor( m_dropNotAllowedCursor );
	}
}

void CAnimationListControl::EndDragAndDrop( const CPoint& clientPoint )
{
	if ( m_dragDropListeners.empty() )
	{
		assert( ! m_dragging );
		return;
	}
	
	assert( m_dragging );

	m_dragging = false;

	ReleaseCapture();

	
	CPoint screenPoint( clientPoint );
	ClientToScreen( &screenPoint );

	const CString& selectedAnimationName = GetSelectedAnimationName();

	IAnimationNameDragDropListener* pFinalDropListener = NULL;
	for ( TDragDropSet::iterator it = m_dragDropListeners.begin(); it != m_dragDropListeners.end() && pFinalDropListener == NULL; ++it )
	{
		IAnimationNameDragDropListener* pDragDropListener = *it;
		bool isValidDropPoint = pDragDropListener->IsValidDropPoint( screenPoint, selectedAnimationName );
		if ( isValidDropPoint )
		{
			pFinalDropListener = pDragDropListener;
		}
	}


	if ( pFinalDropListener != NULL )
	{
		pFinalDropListener->DoDrop( screenPoint, selectedAnimationName );
	}

	for ( TDragDropSet::iterator it = m_dragDropListeners.begin(); it != m_dragDropListeners.end(); ++it )
	{
		IAnimationNameDragDropListener* pDragDropListener = *it;
		pDragDropListener->DragEnded( screenPoint, selectedAnimationName );
	}
}

bool CAnimationListControl::IsDragging() const
{
	return m_dragging;
}

void CAnimationListControl::FilterByName( const CString& name )
{
	std::vector< CString > includeFilters;
	std::vector< CString > excludeFilters;
	
	int pos = 0;
	while ( pos != -1 )
	{
		CString filter = name.Tokenize( " ", pos );
		if ( filter.GetLength() != 0 )
		{
			filter.MakeLower();
			bool isExcludeFilter = ( filter[ 0 ] == '-' );
			if ( isExcludeFilter )
			{
				CString excludeFilter = filter.Mid( 1 );
				if ( excludeFilter.GetLength() != 0 )
				{
					excludeFilters.push_back( excludeFilter );
				}
			}
			else
			{
				includeFilters.push_back( filter );
			}
		}
	}
	
	CXTPReportRecords* pGroupRecords = GetRecords();
	FilterByNamesRec( includeFilters, excludeFilters, pGroupRecords );

	Populate();
}

bool CAnimationListControl::FilterByNamesRec( const std::vector< CString >& includeFilters, const std::vector< CString >& excludeFilters, CXTPReportRecords* pRecordList )
{
	if ( pRecordList == NULL )
	{
		return false;
	}

	bool hasVisibleChildren = false;
	for ( int i = 0; i < pRecordList->GetCount(); i++ )
	{
		CAnimationListRecord* pRecord = ( CAnimationListRecord* )( pRecordList->GetAt( i ) );

		if ( pRecord->IsGroup() )
		{
			bool groupHasVisibleChildren = FilterByNamesRec( includeFilters, excludeFilters, pRecord->GetChilds() );
			pRecord->SetVisible( groupHasVisibleChildren );
			hasVisibleChildren |= groupHasVisibleChildren;
		}
		else
		{
			bool matchesFilters = MatchesFilters( includeFilters, excludeFilters, pRecord );
			pRecord->SetVisible( matchesFilters );
			hasVisibleChildren |= matchesFilters;
		}
	}

	return hasVisibleChildren;
}

bool CAnimationListControl::MatchesFilters( const std::vector< CString >& includeFilters, const std::vector< CString >& excludeFilters, const CAnimationListRecord* pRecord ) const
{
	assert( pRecord != NULL );
	if ( pRecord == NULL )
	{
		return false;
	}

	CString lowercaseName = pRecord->GetName();
	lowercaseName.MakeLower();

	for ( size_t i = 0; i < includeFilters.size(); i++ )
	{
		const CString& includeFilter = includeFilters[ i ];
		bool containsFilter = ( lowercaseName.Find( includeFilter ) != -1 );
		if ( ! containsFilter )
		{
			return false;
		}
	}

	for ( size_t i = 0; i < excludeFilters.size(); i++ )
	{
		const CString& excludeFilter = excludeFilters[ i ];
		bool containsFilter = ( lowercaseName.Find( excludeFilter ) != -1 );
		if ( containsFilter )
		{
			return false;
		}
	}

	return true;
}

void CAnimationListControl::ClearFilter()
{
	FilterByName( "" );
}
