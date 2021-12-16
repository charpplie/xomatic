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

#include "StdAfx.h"
#include "SlotData.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CSlotData::CSlotData()
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CSlotData::~CSlotData()
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSlotData::Clear()
{
	bool bRet = ( m_Resources.size() > 0 );

	m_Resources.clear();

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CSlotData::GetAsString( string& sString ) const
{
	string  sTmp;
	sString.clear();

	TSlotTypeList::const_iterator it = m_Resources.begin();

	while( it != m_Resources.end() )
	{
		if( it == m_Resources.begin() )
		{
			sTmp.Format( "%d", *it );
		}
		else
		{
			sTmp.Format( " %d", *it );
		}

		sString.append( sTmp );

		++it;
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSlotData::Add( TSlotType typeId )
{
	bool bRet = false;

	if( Exists(typeId) == false )
	{
		m_Resources.push_back( typeId );
		bRet = true;
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSlotData::Delete( TSlotType typeId )
{
	bool bRet = false;

	TSlotTypeList::iterator it = m_Resources.begin();

	while( it != m_Resources.end() )
	{
		if( *it == typeId )
		{
			m_Resources.erase( it );
			bRet = true;
			break;
		}

		++it;
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
uint32 CSlotData::Count() const
{
	return( m_Resources.size() );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSlotData::Exists( TSlotType typeId ) const
{
	bool bRet = false;

	TSlotTypeList::const_iterator it = m_Resources.begin();

	while( it != m_Resources.end() )
	{
		if( *it == typeId )
		{
			bRet = true;
			break;
		}

		++it;
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
uint32 CSlotData::MoveFrom( CSlotData* pSlotData, uint32 uMax )
{
	assert( pSlotData != NULL );

	Clear();

	uint32 uCount = 0;
	TSlotTypeList::iterator it = pSlotData->m_Resources.begin();

	while( it != pSlotData->m_Resources.end() && (uMax > 0 && uCount < uMax) )
	{
		m_Resources.push_back( *it );
		++uCount;
		++it;
	}

	pSlotData->m_Resources.erase( pSlotData->m_Resources.begin(), it );

	return( uCount );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
uint32 CSlotData::CopyFrom( const CSlotData* pSlotData, uint32 uMax )
{
	assert( pSlotData != NULL );

	Clear();

	uint32 uCount = 0;
	TSlotTypeList::const_iterator it = pSlotData->m_Resources.begin();

	while( it != pSlotData->m_Resources.end() && (uMax > 0 && uCount < uMax) )
	{
		m_Resources.push_back( *it );
		++uCount;
		++it;
	}

	return( uCount );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlotData::IsEqual( CSlotData* pSlotData, uint32 uMax ) const
{
	assert( pSlotData != NULL );

	bool bRet = false;

	uint32 uMySize = m_Resources.size();
	uint32 uHisSize = pSlotData->m_Resources.size();
	uint32 uMaxSize = ( uMax == 0 ) ? uMySize : MIN( uMySize, uMax );

	if( (uMax > 0 && uHisSize >= uMaxSize) || (uMax == 0 && uMySize == uHisSize) )
	{
		uint32 uCount = 0;
		TSlotTypeList::const_iterator it = pSlotData->m_Resources.begin();
		TSlotTypeList::const_iterator it2 = m_Resources.begin();

		while( it != pSlotData->m_Resources.end() && it2 != m_Resources.end() && uCount < uMaxSize )
		{
			if( *it != *it2 )
			{
				bRet = false;
				break;
			}

			bRet = true;
			++uCount;
			++it2;
			++it;
		}
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlotData::IsEqualNotOrder( CSlotData* pSlotData, uint32 uMax ) const
{
	assert( pSlotData != NULL );

	bool bRet = false;

	uint32 uMySize = m_Resources.size();
	uint32 uHisSize = pSlotData->m_Resources.size();
	uint32 uMaxSize = ( uMax == 0 ) ? uMySize : MIN( uMySize, uMax );

	if( (uMax > 0 && uHisSize >= uMaxSize) || (uMax == 0 && uMySize == uHisSize) )
	{
		uint32 uCount = 0;
		TSlotTypeList::const_iterator it = m_Resources.begin();

		while( it != m_Resources.end() && uCount < uMaxSize )
		{
			if( pSlotData->Exists(*it) == false )
			{
				bRet = false;
				break;
			}

			bRet = true;
			++uCount;
			++it;
		}
	}

	return( bRet );
}

// Description:
//   Deletes resources shared from THIS class and copy new ones to pSlotData
// Arguments:
//
// Return:
//   True if it was needed to change pSlotData
bool CSlotData::DeleteSharedAndCopyNew( CSlotData* pSlotData )
{
	assert( pSlotData != NULL );

	bool bRet = ( Count() != pSlotData->Count() );
	TSlotTypeList::iterator it = pSlotData->m_Resources.begin();

	// First clear their list, the ones that are not shared
	while( it != pSlotData->m_Resources.end() )
	{
		if( Exists(*it) == false )
		{
			it = pSlotData->m_Resources.erase( it );
			bRet = true;
		}
		else
		{
			++it;
		}
	}

	if( bRet == false )
	{
		m_Resources.clear();
	}
	else
	{
		// Second, clear our list from shared resources
		it = m_Resources.begin();
		while( it != m_Resources.end() )
		{
			if( pSlotData->Exists(*it) == true )
			{
				it = m_Resources.erase( it );
			}
			else
			{
				++it;
			}
		}

		// Third, add to their list our non-shared resources left
		pSlotData->GetResources().insert( pSlotData->GetResources().end(), m_Resources.begin(), m_Resources.end() );
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
const TSlotTypeList& CSlotData::GetResources() const
{
	return( m_Resources );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
TSlotTypeList& CSlotData::GetResources()
{
	return( m_Resources );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CSlotData::Sort( bool (*CompareFunction) (TSlotType, TSlotType) )
{
	m_Resources.sort( CompareFunction );
}
