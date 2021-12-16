////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 1999-2010.
// -------------------------------------------------------------------------
//  File Name        : JointListView.h
//  Version          : v1.00
//  Created          : 9/6/2010 by Pau Novau
//  Description      : View that holds the controls necessary to create the
//                     JointList for LMGs in the LMG Editor.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Joint_List_View__h__
#define __Joint_List_View__h__
#pragma once

#include "LMGEditor/DockingBaseView.h"

#include "LMGEditor/JointListControl.h"

class CJointListView
	: public CDockingBaseView
{
	DECLARE_DYNCREATE( CJointListView )

public:
	CJointListView();
	virtual ~CJointListView();


	void SetStatusFromJointList( const std::vector< CString >& jointNames );
	void UpdateCharacterJoints( ISkeletonPose* pSkeletonPose );
	void SetListener( IJointListRecordsListener* pListener );

	bool AreAllJointsEnabled() const;
	void GetEnabledJointsNames( std::vector< CString >& jointNamesOut ) const;

protected:
	void AddCharacterJoint( const CString& name, const CString& parentName = "" );
	void ClearCharacterJoints();

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnOK() {}
	virtual void OnCancel() {}

	DECLARE_MESSAGE_MAP()

private:
	CJointListControl m_jointsTree;
};

#endif