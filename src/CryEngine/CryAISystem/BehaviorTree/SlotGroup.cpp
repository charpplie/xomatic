/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   Tactic.cpp
$Id$
$DateTime$
Description: Every Slot belongs to a SlotGroup. All the resources 
throughout a SlotGroup are unique when they are assigned to run in
a certain slot.
---------------------------------------------------------------------
History:
- 12:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#include "StdAfx.h"
#include "SlotGroup.h"
#include "Slot.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CSlotGroup::CSlotGroup()
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CSlotGroup::~CSlotGroup()
{
	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		SAFE_DELETE( m_vecSlots[uIndex] );
	}

	m_vecSlots.clear();
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CSlot* CSlotGroup::AddSlot()
{
	CSlot*  pRes = NULL;

	pRes = new CSlot( this );
	m_vecSlots.push_back( pRes );

	return( pRes );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CSlotGroup::AssignResource( CSlot const* pSlot, TSlotType typeId )
{
	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		if( pSlot != m_vecSlots[uIndex] )
		{
			m_vecSlots[uIndex]->DeleteAssigned( typeId );
		}
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSlotGroup::IsResourceAssigned( TSlotType typeId, CSlot const* pSkip ) const
{
	bool  bRes = false;

	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		if( pSkip != m_vecSlots[uIndex] )
		{
			bRes = m_vecSlots[uIndex]->AssignedExists( typeId );
		}

		if( bRes == true )
		{
			break;
		}
	}

	return( bRes );
}

// Description:
//
// Arguments:
//
// Return:
//   True if something changed
bool CSlotGroup::AssignResources()
{
	bool  bRet = false;
	bool  bForcedAssigned;

	// Notify slots the process start
	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		m_vecSlots[uIndex]->StartAssignProcess();
	}

	// Forced check: if num_candidates == min_resources
	do
	{
		bForcedAssigned = false;
		for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
		{
			bForcedAssigned |= m_vecSlots[uIndex]->AssignForcedResources();
		}
	} while( bForcedAssigned == true );

	// Assign min resources
	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		m_vecSlots[uIndex]->AssignMinResources();
	}

	// Try to allocate rest of resources
	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		m_vecSlots[uIndex]->AssignMaxResources();
	}

	// End the process, copy new assigned list if needed
	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		bRet |= m_vecSlots[uIndex]->EndAssignProcess();
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlotGroup::DeAssignResources( bool bOnlyAssigned )
{
	bool  bRet = false;

	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		bRet |= m_vecSlots[uIndex]->DeAssignResources( bOnlyAssigned );
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSlotGroup::HaveEnoughCandidates() const
{
	bool  bRes = true;

	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		bRes = m_vecSlots[uIndex]->HaveEnoughCandidates();

		if( bRes == false )
		{
			break;
		}
	}

	if( bRes == true )
	{
		TSlotTypeList vecResources;
		GetCandidateResources( vecResources );

		if( vecResources.size() < GetMinResourcesCount() )
		{
			bRes = false;
		}
	}

	return( bRes );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSlotGroup::HaveCandidates() const
{
	bool bRet = false;

	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		bRet = m_vecSlots[uIndex]->HaveCandidates();

		if( bRet == true )
		{
			break;
		}
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSlotGroup::HaveEnoughAssigned() const
{
	bool bRes = true;

	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		bRes = m_vecSlots[uIndex]->HaveEnoughAssigned();

		if( bRes == false )
		{
			break;
		}
	}

	return( bRes );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSlotGroup::HaveEnoughRunning() const
{
	bool bRes = true;

	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		bRes = m_vecSlots[uIndex]->HaveEnoughRunning();

		if( bRes == false )
		{
			break;
		}
	}

	return( bRes );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CSlotGroup::GetNewAssignedResources( TSlotTypeList& vecResources ) const
{
	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		m_vecSlots[uIndex]->GetNewAssignedResources( vecResources );
	}

	vecResources.unique();
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CSlotGroup::GetCandidateResources( TSlotTypeList& vecResources ) const
{
	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		m_vecSlots[uIndex]->GetCandidateResources( vecResources );
	}

	vecResources.unique();
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
uint32 CSlotGroup::GetMinResourcesCount() const
{
	uint32 uCount = 0;

	for( uint32 uIndex = 0; uIndex < m_vecSlots.size(); ++uIndex )
	{
		uCount += m_vecSlots[uIndex]->GetMin();
	}

	return( uCount );
}
