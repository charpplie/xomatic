////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   SlotControl.h
//  Version:     v1.00
//  Created:     28/07/2009 by Pau Novau
//  Description: Control that stores a named animation slot with description
//               and status information.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Slot_Control__h__
#define __Slot_Control__h__
#pragma once

class CSlotControl;
struct ISlotControlAnimationNameListener
{
	virtual void OnSlotAnimationNameChanged( CSlotControl* slotControl ) = 0;
};

class CSlotControl
	: public CXTResizeDialog
{
	DECLARE_DYNAMIC( CSlotControl )

public:

	typedef enum
	{
		SS_OK,
		SS_WARNING,
		SS_ERROR
	} SlotStatus;

	CSlotControl( CWnd* pParent );

	void SetName( const CString& slotName );
	CString GetName() const;

	void SetAnimationName( const CString& animationName );
	CString GetAnimationName() const;
	const CString& GetOldAnimationName() const;

	void SetDescription( const CString& description );

	void SetAnimationNameChangeListener( ISlotControlAnimationNameListener* animationNameListener );

	void SetStatus( SlotStatus status, const CString& message );

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL PreTranslateMessage( MSG* pMsg );

	afx_msg void OnAnimationNameChange();
	DECLARE_MESSAGE_MAP()

	void RecordAnimationNameUndo();

private:
	CStatic m_slotName;
	CEdit m_animationName;
	CStatic m_statusImage;

	CToolTipCtrl m_tooltip;

	ISlotControlAnimationNameListener* m_animationNameChangeListener;

	CString m_oldAnimationName;

	static HBITMAP ms_statusImages[ 3 ];
};

#endif
