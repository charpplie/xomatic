////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   SlotsView.h
//  Version:     v1.00
//  Created:     15/07/2009 by Pau Novau
//  Description: View used to hold a list of animation slots.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Slots_View_h__
#define __Slots_View_h__
#pragma once

#include "..\LMGEditor/DockingBaseView.h"

#include "..\LMGEditor/IAnimationNameDragDropListener.h"
#include "..\LMGEditor/SlotControl.h"
#include "..\LMGEditor/ScrollHelper.h"

#include <vector>

struct ISlotsViewListener
{
	virtual void OnSlotAnimationNameChanged() = 0;
};

class CSlotsView
	: public CDockingBaseView
	, public IAnimationNameDragDropListener
	, public ISlotControlAnimationNameListener
{
	DECLARE_DYNCREATE( CSlotsView )

public:
	CSlotsView();
	virtual ~CSlotsView();

	// IAnimationNameDragDropListener
	virtual bool IsValidDropPoint( const CPoint& screenPoint, const CString& animationName ) const;
	virtual void DoDrop( const CPoint& screenPoint, const CString& animationName );
	// ~IAnimationNameDragDropListener

	// ISlotControlAnimationNameListener
	virtual void OnSlotAnimationNameChanged( CSlotControl* pSlotControl );
	// ~ISlotControlAnimationNameListener

	void BatchSlotCreationStart();
	void BatchSlotCreationEnd();
	void CreateSlot( const CString& slotName, const CString& description, const CString& animationName = "" );
	void RemoveAllSlots();

	void SetSlotStatus( size_t slotId, CSlotControl::SlotStatus status, const CString& message = "" );
	void SetAnimationName( size_t slotId, const CString& animationName );
	CString GetAnimationName( size_t slotId ) const;
	size_t GetSlotCount() const;

	void SetListener( ISlotsViewListener* pListener );

protected:
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual void OnDestroy();
	virtual void OnOK() {}
	virtual void OnCancel() {}

	void RecalculateSlotsLayout();

	CSlotControl* GetSlotAtPoint( const CPoint& screenPoint );
	const CSlotControl* GetSlotAtPoint( const CPoint& screenPoint ) const;

	void DestroySlots();

private:
	std::vector< CSlotControl* > m_slots;
	int m_activeSlots;

	int m_totalSlotsHeight;

	ISlotsViewListener* m_pListener;

	CScrollHelper m_scrollHelper;

	bool m_batchCreatingSlots;
};


#endif
