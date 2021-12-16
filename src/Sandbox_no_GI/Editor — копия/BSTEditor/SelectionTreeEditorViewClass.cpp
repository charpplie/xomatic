////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeEditorViewClass.cpp
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BSTEditor/SelectionTreeEditor.h"

class CSelectionTreeEditorViewClass 
	: public IViewPaneClass
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; }
	virtual REFGUID ClassID()
	{
		// {83794077-181D-4aff-A353-C565E75D0A21}
		static const GUID guid = { 0x83794077, 0x181d, 0x4aff, { 0xa3, 0x53, 0xc5, 0x65, 0xe7, 0x5d, 0xa, 0x21 } };
		return guid;
	}
	virtual const char* ClassName() { return "Selection Tree Editor"; }
	virtual const char* Category() { return "SelectionTreeEditor"; }
	//////////////////////////////////////////////////////////////////////////

	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS( CSelectionTreeEditor ); }
	virtual const char* GetPaneTitle() { return _T( "Selection Tree Editor" ); }
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; }
	virtual CRect GetPaneRect() { return CRect( 200, 200, 800, 700 ); }
	virtual bool SinglePane() { return false; }
	virtual bool WantIdleUpdate() { return true; }
public:
	static void RegisterViewClass()
	{ 
		GetIEditor()->GetClassFactory()->RegisterClass( new CSelectionTreeEditorViewClass ); 
	} 
};


REGISTER_CLASS_DESC( CSelectionTreeEditorViewClass )