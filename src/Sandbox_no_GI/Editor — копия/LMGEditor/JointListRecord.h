////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 1999-2010.
// -------------------------------------------------------------------------
//  File Name        : JointListRecord.h
//  Version          : v1.00
//  Created          : 14/6/2010 by Pau Novau
//  Description      : Single record in a joint list control.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Joint_List_Record__h__
#define __Joint_List_Record__h__
#pragma once

class CJointListRecord
	: public CXTPReportRecord
{
	DECLARE_SERIAL( CJointListRecord )

public:
	CJointListRecord();
	CJointListRecord( const CString& name );

	void Serialize( CArchive& ar );

	const CString& GetName() const;

	bool IsHierarchyEnabled() const;
	bool IsEnabled() const;
	void SetEnabled( bool enabled );
	void SetEnabledRecursive( bool enabled );

protected:
	void UpdateColors();
	void UpdateChildColors();
	bool IsParentEnabled() const;

private:
	CString m_name;
	bool m_enabled;
};

#endif