////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   AnimationListRecord.h
//  Version:     v1.00
//  Created:     15/07/2009 by Pau Novau
//  Description: Record item used by animation list control.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Animation_List_Record__h__
#define __Animation_List_Record__h__
#pragma once

class CAnimationListRecord
	: public CXTPReportRecord
{
	DECLARE_SERIAL( CAnimationListRecord )

public:
	CAnimationListRecord();
	CAnimationListRecord( bool isGroup, const CString& name );

	void Serialize( CArchive& ar );

	const CString& GetName() const;
	bool IsGroup() const;

private:
	bool m_isGroup;
	CString m_name;
};

#endif