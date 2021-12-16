////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TelemetryViewClass.h
//  Version:     v1.00
//  Created:     17/12/2009 by Sergey Mikhtonyuk
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef		_TELEMETRYVIEWCLASS_H__
#define		_TELEMETRYVIEWCLASS_H__

# pragma once

#include "TelemetryDialog.h"
#include "Include/IViewPane.h"
#include "Util/RefCountBase.h"


class CTelemetryViewClass : public TRefCountBase<IViewPaneClass>
{
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	
	virtual REFGUID ClassID()
	{
		// {50087104-84c4-44a8-a733-2d23809f453d} 
		static const GUID guid = 
		{ 0x50087104, 0x84c4, 0x44a8, { 0xa7, 0x33, 0x2d, 0x23, 0x80, 0x9f, 0x45, 0x3d } };
		return guid;
	}

	virtual const char* ClassName() { return "Telemetry"; }
	
	virtual const char* Category() { return "Telemetry"; }
	
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTelemetryDialog); }
	
	virtual const char* GetPaneTitle() { return "Telemetry"; }
	
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; }
	
	virtual CRect GetPaneRect() { return CRect(200,200,600,500); }
	
	virtual bool SinglePane() { return false; }
	
	virtual bool WantIdleUpdate() { return true; }
};

#endif // __TELEMETRYVIEWCLASS_H__
