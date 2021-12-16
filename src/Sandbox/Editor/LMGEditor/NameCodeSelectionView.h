////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   NameCodeSelectionView.h
//  Version:     v1.00
//  Created:     02/09/2009 by Pau Novau
//  Description: Control that displays a dropdown list and stores a string 
//               value associated to each name displayed.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Name_Code_Selection_View__h__
#define __Name_Code_Selection_View__h__
#pragma once

#include "..\LMGEditor/DockingBaseView.h"

class CNameCodeSelectionView
	: public CDockingBaseView
{
	DECLARE_DYNCREATE( CNameCodeSelectionView )

public:
	CNameCodeSelectionView();
	virtual ~CNameCodeSelectionView();

	void AddNameCodePair( const CString& name, const CString& code );
	void Clear();

	void SetCurrentNameByCode( const CString& codeToSelect );

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnOK() {}
	virtual void OnCancel() {}

	afx_msg void OnSelectionChanged();
	DECLARE_MESSAGE_MAP()

	virtual void OnSelectionChangedListener( const CString& code ) {}

private:
	CXTPComboBox m_names;

	std::vector< CString > m_codes;
};

#endif
