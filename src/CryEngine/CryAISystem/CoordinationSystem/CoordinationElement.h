#ifndef __CoordinationElement_h__
#define __CoordinationElement_h__

#pragma once


#include "Coordination.h"
#include "CoordinationRole.h"


class CoordinationElement
	: public _reference_target_t
{
public:
	typedef _smart_ptr<CoordinationElement> Ptr;

public:
	virtual ~CoordinationElement()
	{
	}

	const char* GetName() const
	{
		return m_name.c_str();
	}

	virtual bool LoadFromXML(uint32 elementId, const XmlNodeRef& rootNode) = 0;

	virtual void DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const = 0;

	virtual ECoordinationState GetState(const CoordinationID& coordinationID) const = 0;
	virtual bool CanStart(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params) const = 0;
	virtual void Start(const CoordinationID& coordinationID) = 0;
	virtual void Stop(const CoordinationID& coordinationID) = 0;
	virtual void Update(float frameTime) = 0;

protected:
	string m_name;
};


class CoordinationElements :
	public std::vector<CoordinationElement::Ptr>
{
};


#endif //__CoordinationElement_h__