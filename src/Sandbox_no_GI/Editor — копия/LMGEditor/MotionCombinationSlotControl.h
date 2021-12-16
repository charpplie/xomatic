////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   MotionCombinationSlotControl.h
//  Version:     v1.00
//  Created:     03/09/2009 by Pau Novau
//  Description: Control that contains a single motion combination slot.
//               It contains the name of the animation and a "-" button
//               to ask the parent control to remove itself from it.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Motion_Combination_Slot_Control__h__
#define __Motion_Combination_Slot_Control__h__
#pragma once

class CMotionCombinationSlotControl;
struct IMotionCombinationSlotControlListener
{
	virtual void OnMotionCombinationSlotNameChanged( CMotionCombinationSlotControl* pMotionCombinationSlotControl ) = 0;
	virtual void OnRemoveButtonClicked( CMotionCombinationSlotControl* pMotionCombinationSlotControl ) = 0;
};

class CMotionCombinationSlotControl
	: public CXTResizeDialog
{
	DECLARE_DYNCREATE( CMotionCombinationSlotControl )

public:
	CMotionCombinationSlotControl( CWnd* pParent );

	void SetMotionCombinationName( const CString& motionCombinationName );
	CString GetMotionCombinationName() const;

	void SetListener( IMotionCombinationSlotControlListener* pMotionCombinationSlotListener );

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );

	afx_msg void OnMotionCombinationNameChanged();
	afx_msg void OnRemoveButtonClicked();
	afx_msg void OnEnable( BOOL enable );
	DECLARE_MESSAGE_MAP()

private:
	CEdit m_motionCombinationName;
	CButton m_removeMotionCombination;
	IMotionCombinationSlotControlListener* m_pMotionCombinationSlotListener;
};

#endif