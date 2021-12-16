////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   AnimationsListView.h
//  Version:     v1.00
//  Created:     15/07/2009 by Pau Novau
//  Description: View that lists animations that can be used to create an
//               Lmg animation.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AnimationsListView__h__
#define __AnimationsListView__h__
#pragma once

#include "..\LMGEditor/DockingBaseView.h"

#include "..\ToolbarDialog.h"
#include "..\LMGEditor/AnimationListControl.h"

struct IAnimationSet;
struct IAnimationNameDragDropListener;
struct IAnimationNameSelectionChangedListener;

class CAnimationsListView
	: public CDockingBaseView
{
	DECLARE_DYNCREATE( CAnimationsListView )

public:
	CAnimationsListView();
	virtual ~CAnimationsListView();

	void AddDragDropListener( IAnimationNameDragDropListener* pDragDropListener );
	void SetSelectionChangeListener( IAnimationNameSelectionChangedListener* pSelectionChangedListener );

	void UpdateAnimations( IAnimationSet* pAnimationSet );
	void Clear();

	void ResetFilterText();

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnOK() {}
	virtual void OnCancel() {}

	afx_msg void OnAnimationNameFilterChange();
	afx_msg void OnResetFilterTextButtonClicked();
	DECLARE_MESSAGE_MAP()

private:
	CButton m_resetFilterTextButton;
	CEdit m_animationNameFilter;
	CAnimationListControl m_animationsReport;
};

#endif
