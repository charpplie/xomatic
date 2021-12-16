////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   UndoSlotChange.h
//  Version:     v1.00
//  Created:     17/09/2009 by Pau Novau
//  Description: Undo object for text changes in animation name slots.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Undo_Slot_Change__h__
#define __Undo_Slot_Change__h__
#pragma once

class CSlotControl;

class CUndoSlotChange
	: public IUndoObject
{
public:
	CUndoSlotChange( CSlotControl* pSlotControl, const CString& animationNameToSave );

protected:
	virtual int GetSize() { return sizeof( *this ); }
	virtual const char* GetDescription() { return "Slot Change"; };

	virtual void Undo( bool storeDataForRedo );
	virtual void Redo();

	bool IsValidSlotControl() const;
	CString GetSlotAnimationName() const;
	void SetSlotAnimationName( const CString& animationName );

private:
	CSlotControl* m_pSlotControl;
	CString m_storedAnimationName;
};

#endif