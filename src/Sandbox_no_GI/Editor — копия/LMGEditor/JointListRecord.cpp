////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 1999-2010.
// -------------------------------------------------------------------------
//  File Name        : JointListRecord.cpp
//  Version          : v1.00
//  Created          : 14/6/2010 by Pau Novau
//  Description      : Single record in a joint list control.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/JointListRecord.h"


IMPLEMENT_SERIAL( CJointListRecord, CXTPReportRecord, 0 )

CJointListRecord::CJointListRecord()
{
}


CJointListRecord::CJointListRecord( const CString& name )
: m_name( name )
, m_enabled( true )
{
	CXTPReportRecordItem* pItem = new CXTPReportRecordItemText( name );
	pItem->SetTextColor( RGB( 0, 0, 0 ) );
	AddItem( pItem );
}


void CJointListRecord::Serialize( CArchive& ar )
{
	CXTPReportRecord::Serialize( ar );
	if ( ar.IsStoring() )
	{
		ar << m_name << m_enabled;
	}
	else
	{
		ar >> m_name >> m_enabled;
	}

	UpdateColors();
}


const CString& CJointListRecord::GetName() const
{
	return m_name;
}


bool CJointListRecord::IsHierarchyEnabled() const
{
	bool enabled = IsEnabled();
	bool parentEnabled = IsParentEnabled();

	return ( enabled && parentEnabled );
}


bool CJointListRecord::IsEnabled() const
{
	return m_enabled;
}


bool CJointListRecord::IsParentEnabled() const
{
	CXTPReportRecord* pParentRecord = GetParentRecord();
	if ( pParentRecord == NULL )
	{
		return true;
	}

	CJointListRecord* pJointListParentRecord = DYNAMIC_DOWNCAST( CJointListRecord, pParentRecord );
	assert( pJointListParentRecord != NULL );
	if ( pJointListParentRecord == NULL )
	{
		return true;
	}

	bool parentEnabled = pJointListParentRecord->IsHierarchyEnabled();
	return parentEnabled;
}


void CJointListRecord::SetEnabled( bool enabled )
{
	if ( enabled == m_enabled )
	{
		return;
	}

	m_enabled = enabled;
	UpdateColors();
}


void CJointListRecord::SetEnabledRecursive( bool enabled )
{
	m_enabled = enabled;

	CXTPReportRecords* pChildRecords = GetChilds();
	if ( pChildRecords == NULL )
	{
		return;
	}

	for ( int i = 0; i < pChildRecords->GetCount(); ++i )
	{
		CXTPReportRecord* pChildRecord = pChildRecords->GetAt( i );
		assert( pChildRecord != NULL );

		CJointListRecord* pJointListChildRecord = DYNAMIC_DOWNCAST( CJointListRecord, pChildRecord );
		assert( pJointListChildRecord != NULL );
		if ( pJointListChildRecord != NULL )
		{
			pJointListChildRecord->SetEnabledRecursive( enabled );
		}
	}

	UpdateColors();
}


void CJointListRecord::UpdateColors()
{
	COLORREF color = RGB( 0, 0, 0 );

	bool enabled = IsEnabled();
	if ( enabled )
	{
		bool hierarchyEnabled = IsHierarchyEnabled();
		if ( ! hierarchyEnabled )
		{
			// It is valid to have bones enabled that have some parent bone disabled.
			// Most probably it won't look too good, so give a slight color hint without
			// implying it is disabled...
			color = RGB( 100, 100, 100 );
		}
	}
	else
	{
		color = RGB( 200, 40, 40 );
	}

	GetItem( 0 )->SetTextColor( color );

	UpdateChildColors();
}

void CJointListRecord::UpdateChildColors()
{
	CXTPReportRecords* pChildRecords = GetChilds();
	if ( pChildRecords == NULL )
	{
		return;
	}

	for ( int i = 0; i < pChildRecords->GetCount(); ++i )
	{
		CXTPReportRecord* pChildRecord = pChildRecords->GetAt( i );
		assert( pChildRecord != NULL );
		
		CJointListRecord* pJointListChildRecord = DYNAMIC_DOWNCAST( CJointListRecord, pChildRecord );
		assert( pJointListChildRecord != NULL );
		if ( pJointListChildRecord != NULL )
		{
			pJointListChildRecord->UpdateColors();
		}
	}
}