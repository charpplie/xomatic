////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   MotionCombinationSlotsView.h
//  Version:     v1.00
//  Created:     03/09/2009 by Pau Novau
//  Description: View that shows a user expandable list of view combination 
//               animation slots.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Motion_Combination_Slots_View__h__
#define __Motion_Combination_Slots_View__h__
#pragma once

#include "..\LMGEditor/DockingBaseView.h"
#include "..\LMGEditor/IAnimationNameDragDropListener.h"
#include "..\LMGEditor/MotionCombinationSlotControl.h"
#include "..\LMGEditor/ScrollHelper.h"


struct IMotionCombinationSlotsViewListener
{
	virtual void OnMotionCombinationSlotNameChanged() = 0;
	virtual void OnMotionCombinationSlotRemoved() = 0;
};


class CMotionCombinationSlotsView
	: public CDockingBaseView
	, public IAnimationNameDragDropListener
	, public IMotionCombinationSlotControlListener
{
	DECLARE_DYNCREATE( CMotionCombinationSlotsView )
public:
	CMotionCombinationSlotsView();
	virtual ~CMotionCombinationSlotsView();

	// IAnimationNameDragDropListener
	virtual bool IsValidDropPoint( const CPoint& screenPoint, const CString& animationName ) const;
	virtual void DoDrop( const CPoint& screenPoint, const CString& animationName );
	// ~IAnimationNameDragDropListener

	// IMotionCombinationSlotControlListener
	virtual void OnMotionCombinationSlotNameChanged( CMotionCombinationSlotControl* pMotionCombinationSlotControl );
	virtual void OnRemoveButtonClicked( CMotionCombinationSlotControl* pMotionCombinationSlotControl );
	// ~IMotionCombinationSlotControlListener

	void CreateMotionCombinationSlot( const CString& motionCombinationName = "" );
	void RemoveAllSlots();

	void SetMotionCombinationSlotName( size_t slotId, const CString& motionCombinationName );
	CString GetMotionCombinationName( size_t slotId ) const;
	size_t GetSlotCount() const;

	void SetListener( IMotionCombinationSlotsViewListener* pListener );

protected:
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void OnAddMotionCombination();
	afx_msg void OnEnableEditStatusChanged();
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual void OnDestroy();
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnOK() {}
	virtual void OnCancel() {}

	void RecalculateSlotsLayout();

	void DestroySlots();

	CMotionCombinationSlotControl* GetSlotAtPoint( const CPoint& screenPoint );
	const CMotionCombinationSlotControl* GetSlotAtPoint( const CPoint& screenPoint ) const;

	bool IsEditingEnabled() const;

private:
	std::vector< CMotionCombinationSlotControl* > m_slots;

	int m_totalSlotsHeight;

	CScrollHelper m_scrollHelper;

	CButton m_enableCheckBox;
	CButton m_addMotionCombinationButton;

	IMotionCombinationSlotsViewListener* m_pListener;
};

#endif
