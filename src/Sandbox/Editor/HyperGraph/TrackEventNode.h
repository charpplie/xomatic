////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TrackEventNode.h
//  Version:     v1.00
//  Created:     4/4/2008 by Kevin.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TRACKEVENTNODE_H__
#define __TRACKEVENTNODE_H__

#pragma once

#include <IMovieSystem.h>
#include "FlowGraphNode.h"

#define TRACKEVENT_CLASS ("TrackEvent")
#define TRACKEVENTNODE_TITLE	("TrackEvent")
#define TRACKEVENTNODE_CLASS	("TrackEvent")
#define TRACKEVENTNODE_DESC		("Outputs for Trackview Events")

class CTrackEventNode : public CFlowNode, public ITrackEventListener
{
public:
	static const char* GetClassType() {
		return TRACKEVENT_CLASS;
	}
	CTrackEventNode();
	virtual ~CTrackEventNode();

	// CHyperNode overwrites
	virtual void Init();
	virtual void Done();
	virtual CHyperNode* Clone();
	virtual void Serialize( XmlNodeRef &node,bool bLoading, CObjectArchive* ar);

	virtual CString GetTitle() const;
	virtual const char* GetClassName() const;
	virtual const char* GetDescription();
	virtual Gdiplus::Color GetCategoryColor() { return Gdiplus::Color(220,40,40); }
	virtual void OnInputsChanged();

	// Description:
	//		Re-populates output ports based on input sequence
	// Arguments:
	//		bLoading - TRUE if called from serialization on loading
	virtual void PopulateOutput(bool bLoading);

	// ~ITrackEventListener
	virtual void OnTrackEvent(IAnimSequence *pSequence, int reason, const char* event, void* pUserData);

protected:
	//! Add an output port for an event
	virtual void AddOutputEvent(const char* event);

	//! Remove an output port for an event (including any edges)
	virtual void RemoveOutputEvent(const char* event);

private:
	CString m_sequenceName;	// Current sequence name

	IAnimSequence *m_pSequence; // Current sequence
};

#endif //__TRACKEVENTNODE_H__
