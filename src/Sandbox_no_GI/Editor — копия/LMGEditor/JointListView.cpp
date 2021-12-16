////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 1999-2010.
// -------------------------------------------------------------------------
//  File Name        : JointListView.cpp
//  Version          : v1.00
//  Created          : 9/6/2010 by Pau Novau
//  Description      : View that holds the controls necessary to create the
//                     JointList for LMGs in the LMG Editor.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/JointListView.h"

IMPLEMENT_DYNCREATE( CJointListView, CDockingBaseView )

BEGIN_MESSAGE_MAP( CJointListView, CDockingBaseView )
END_MESSAGE_MAP()

const int IDC_JOINTS_TREE = 1;

CJointListView::CJointListView()
{
}


CJointListView::~CJointListView()
{
}


BOOL CJointListView::OnInitDialog()
{
	__super::OnInitDialog();

	CRect rc;
	GetClientRect( rc );

	m_jointsTree.Create( WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, rc, this, IDC_JOINTS_TREE );	

	SetResize( IDC_JOINTS_TREE, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );

	return TRUE;
}


void CJointListView::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );
}


void CJointListView::SetStatusFromJointList( const std::vector< CString >& jointNames )
{
	m_jointsTree.SetStatusFromJointList( jointNames );
}


void CJointListView::UpdateCharacterJoints( ISkeletonPose* pSkeletonPose )
{
	m_jointsTree.UpdateJoints( pSkeletonPose );
}


void CJointListView::SetListener( IJointListRecordsListener* pListener )
{
	m_jointsTree.SetListener( pListener );
}


bool CJointListView::AreAllJointsEnabled() const
{
	return m_jointsTree.AreAllJointsEnabled();
}


void CJointListView::GetEnabledJointsNames( std::vector< CString >& jointNamesOut ) const
{
	m_jointsTree.GetEnabledJointsNames( jointNamesOut );
}
