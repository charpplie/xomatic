////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TelemetryPath.h
//  Version:     v1.00
//  Created:     22/12/2009 by Sergey Mikhtonyuk
//  Description: Implementation of path drawing object
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef		_TELEMETRYPATH_H__
#define		_TELEMETRYPATH_H__

# pragma once

//////////////////////////////////////////////////////////////////////////
#include "TelemetryObjects.h"
#include "Util/XmlArchive.h"
#include "Util/Variable.h"
#include "Objects/BaseObject.h"
//////////////////////////////////////////////////////////////////////////

class CTelemetryPathObject : public CBaseObject
{
public:
	DECLARE_DYNCREATE(CTelemetryPathObject)

	// BaseObject
	virtual void Display(DisplayContext &dc);
	virtual void GetBoundBox(AABB &box);
	virtual void GetLocalBounds(AABB &box);
	virtual bool HitTest(HitContext &hc);
	virtual bool HitTestRect(HitContext &hc);

	// Own methods
	void FinalConstruct(CTelemetryRepository* repo, CTelemetryTimeline* timeline);
	CTelemetryTimeline& getPath();

protected:
	CTelemetryPathObject();
	~CTelemetryPathObject();
	virtual void Done();
	virtual void DeleteThis();

private:
	CTelemetryRepository* m_repository;
	CTelemetryTimeline* m_path;

	typedef std::vector<CSmartVariable<CString> > TVarVector;
	TVarVector m_variables;
};

//////////////////////////////////////////////////////////////////////////

class CTelemetryPathObjectClassDesc : public CObjectClassDesc
{
public:
	REFGUID ClassID()
	{
		// {dd4eef86-7a7f-4d29-b402-74844fb26a20} 
		static const GUID guid = { 0xdd4eef86, 0x7a7f, 0x4d29, { 0xb4, 0x02, 0x74, 0x84, 0x4f, 0xb2, 0x6a, 0x20 } };
		return guid;
	}
	ObjectType GetObjectType() { return OBJTYPE_TELEMETRY; };
	const char* ClassName() { return "TelemetryPath"; };
	const char* Category() { return "Misc"; };
	CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTelemetryPathObject); };
	int GameCreationOrder() { return 500; };
};

//////////////////////////////////////////////////////////////////////////

#endif // __TELEMETRYPATH_H__
