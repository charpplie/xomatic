////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   AnimationListRecord.cpp
//  Version:     v1.00
//  Created:     15/07/2009 by Pau Novau
//  Description: Record item used by animation list control.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AnimationListRecord.h"

IMPLEMENT_SERIAL( CAnimationListRecord, CXTPReportRecord, 0 )

CAnimationListRecord::CAnimationListRecord()
{

}

CAnimationListRecord::CAnimationListRecord( bool isGroup, const CString& name )
: m_isGroup( isGroup )
, m_name( name )
{
	CXTPReportRecordItem* pItem = new CXTPReportRecordItemText( name );
	AddItem( pItem );
}

void CAnimationListRecord::Serialize( CArchive& ar )
{
	CXTPReportRecord::Serialize( ar );
	if ( ar.IsStoring() )
	{
		ar << m_isGroup << m_name;
	}
	else
	{
		ar >> m_isGroup >> m_name;
	}
}

const CString& CAnimationListRecord::GetName() const
{
	return m_name;
}

bool CAnimationListRecord::IsGroup() const
{
	return m_isGroup;
}