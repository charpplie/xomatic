////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   AnimationListControl.h
//  Version:     v1.00
//  Created:     15/07/2009 by Pau Novau
//  Description: Control that displays a list of animations.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Animation_List_Control__h__
#define __Animation_List_Control__h__
#pragma once

struct IAnimationNameDragDropListener;
struct IAnimationNameSelectionChangedListener;
struct IAnimationSet;
class CAnimationListRecord;

class CAnimationListControl
	: public CXTPReportControl
{
protected:
	static const int COLUMN_ANIMATION_NAME = 0;

public:
	DECLARE_DYNCREATE( CAnimationListControl )

	CAnimationListControl();
	virtual ~CAnimationListControl();

	virtual BOOL Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL );

	void AddDragDropListener( IAnimationNameDragDropListener* pDragDropListener );
	void RemoveDragDropListener( IAnimationNameDragDropListener* pDragDropListener );
	void SetSelectionChangedListener( IAnimationNameSelectionChangedListener* pSelectionChangedListener );

	const CString& GetSelectedAnimationName() const;
	const CAnimationListRecord* GetSelectedAnimationListRecord() const;

	void UpdateAnimations( IAnimationSet* pAnimationSet );
	void Clear();

	void FilterByName( const CString& name );
	void ClearFilter();

protected:
	afx_msg void OnMouseMove( UINT nFlags, CPoint point );
	afx_msg void OnLButtonUp( UINT nFlags, CPoint point );
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	DECLARE_MESSAGE_MAP()

	virtual void OnSelectionChanged();

	virtual void OnBeginDrag( CPoint point );

	virtual void AddAnimationRecord( IAnimationSet* pAnimationSet, int animationId );
	virtual CAnimationListRecord* CreateGroupRecord( const CString& animationName );
	virtual CString GetGroupName( const CString& animationName );

private:
	void StartDragAndDrop( const CPoint& clientPoint );
	void UpdateDragAndDrop( const CPoint& clientPoint );
	void EndDragAndDrop( const CPoint& clientPoint );
	bool IsDragging() const;

	bool FilterByNamesRec( const std::vector< CString >& includeFilters, const std::vector< CString >& excludeFilters, CXTPReportRecords* pRecord );
	bool MatchesFilters( const std::vector< CString >& includeFilters, const std::vector< CString >& excludeFilters, const CAnimationListRecord* pRecord ) const;

private:
	bool m_dragging;
	HCURSOR m_defaultArrowCursor;
	HCURSOR m_dropAllowedCursor;
	HCURSOR m_dropNotAllowedCursor;
	
	typedef std::set< IAnimationNameDragDropListener* > TDragDropSet;
	TDragDropSet m_dragDropListeners;
	IAnimationNameSelectionChangedListener* m_pSelectionChangeListener;

	typedef std::map< CString, CAnimationListRecord* > AnimationRecordGroupMap;
	AnimationRecordGroupMap m_animationRecordGroups;
};

#endif

