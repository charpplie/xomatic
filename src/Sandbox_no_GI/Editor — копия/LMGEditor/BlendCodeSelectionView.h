////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   BlendCodeSelectionView.h
//  Version:     v1.00
//  Created:     29/07/2009 by Pau Novau
//  Description: View that provides a list of blend codes for the user to
//               choose from.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Blend_Code_Selection_View__h__
#define __Blend_Code_Selection_View__h__

#include "LMGEditor/NameCodeSelectionView.h"

struct IBlendCodeChangedListener
{
	virtual void OnBlendCodeChanged( const CString& blendCode ) = 0;
};

class CBlendCodeSelectionView
	: public CNameCodeSelectionView
{
	DECLARE_DYNCREATE( CBlendCodeSelectionView )

public:
	CBlendCodeSelectionView();
	virtual ~CBlendCodeSelectionView();

	void SetBlendCodeChangedListener( IBlendCodeChangedListener* pBlendCodeChangedListener );

protected:
	virtual void OnSelectionChangedListener( const CString& code );

private:
	IBlendCodeChangedListener* m_pBlendCodeChangedListener;
};

#endif