////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   CapsCodeSelectionView.h
//  Version:     v1.00
//  Created:     02/09/2009 by Pau Novau
//  Description: View that provides a list of caps codes for the user to
//               choose from.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Caps_Code_Selection_View__h__
#define __Caps_Code_Selection_View__h__
#pragma once

#include "LMGEditor/NameCodeSelectionView.h"

struct ICapsCodeChangedListener
{
	virtual void OnCapsCodeChanged( const CString& capsCode ) = 0;
};

class CCapsCodeSelectionView
	: public CNameCodeSelectionView
{
	DECLARE_DYNCREATE( CCapsCodeSelectionView )

public:
	CCapsCodeSelectionView();
	virtual ~CCapsCodeSelectionView();

	void SetCapsCodeChangedListener( ICapsCodeChangedListener* pCapsCodeChangedListener );

protected:
	virtual void OnSelectionChangedListener( const CString& code );

private:
	ICapsCodeChangedListener* m_pCapsCodeChangedListener;
};

#endif