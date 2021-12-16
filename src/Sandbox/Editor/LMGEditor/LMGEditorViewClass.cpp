////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   LMGEditorViewClass.cpp
//  Version:     v1.00
//  Created:     07/07/2009 by Pau Novau
//  Description: Locomotion Group Editor main view registration.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\LMGEditor/LMGEditor.h"


class CLMGEditorViewClass 
	: public TRefCountBase< IViewPaneClass >
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; }
	virtual REFGUID ClassID()
	{
		// {B696747E-69F4-4f3b-AA33-61423D1425C5}
		static const GUID guid = { 0xb696747e, 0x69f4, 0x4f3b, { 0xaa, 0x33, 0x61, 0x42, 0x3d, 0x14, 0x25, 0xc5 } };
		return guid;
	}
	virtual const char* ClassName() { return "Locomotion Group Editor"; }
	virtual const char* Category() { return "Locomotion Group Editor"; }
	//////////////////////////////////////////////////////////////////////////

	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS( CLMGEditor ); }
	virtual const char* GetPaneTitle() { return _T( "Locomotion Group Editor" ); }
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; }
	virtual CRect GetPaneRect() { return CRect( 200, 200, 800, 700 ); }
	virtual bool SinglePane() { return false; }
	virtual bool WantIdleUpdate() { return true; }
};

REGISTER_CLASS_DESC( CLMGEditorViewClass )