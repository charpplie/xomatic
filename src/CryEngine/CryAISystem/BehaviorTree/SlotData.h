/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   SlotData.h
$Id$
$DateTime$
Description: This is the most basic class. Every "CSlot" have some
"CSlotData" classes that stores spefici types of resources. For example
on CSlotData is used for the candidates resources and another for the
assigned resources.
---------------------------------------------------------------------
History:
- 16:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#ifndef _SLOT_DATA_H_
#define _SLOT_DATA_H_

#include "Slot.h"

class CSlotData
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CSlotData();
	~CSlotData();

	/*$1- Actions ------------------------------------------------------------*/
	bool Clear();
	bool Add( TSlotType typeId );
	bool Delete( TSlotType typeId );
	uint32 MoveFrom( CSlotData* pSlotData, uint32 uMax = 0 );
	uint32 CopyFrom( CSlotData const* pSlotData, uint32 uMax = 0 );
	void Sort( bool (*CompareFunction) (TSlotType, TSlotType) );

	/*$1- Public const queries -----------------------------------------------*/
	const TSlotTypeList& GetResources() const;
	TSlotTypeList& GetResources();
	bool Exists( TSlotType typeId ) const;
	void GetAsString( string& sString ) const;
	bool IsEqual( CSlotData* pSlotData, uint32 uMax = 0 ) const;
	bool IsEqualNotOrder( CSlotData* pSlotData, uint32 uMax = 0 ) const;
	bool DeleteSharedAndCopyNew( CSlotData* pSlotData );
	uint32 Count() const;

private:

	/*$1- Members ------------------------------------------------------------*/
	TSlotTypeList m_Resources;
};
#endif // _SLOT_H_
