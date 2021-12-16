#include "StdAfx.h"
#include "CoordinationPlan.h"
#include "CoordinationStep.h"
#include "CoordinationLoop.h"
#include "CoordinationSetup.h"

#include "DebugDrawContext.h"


CoordinationPlan::CoordinationPlan(const CoordinationSetup* setup)
: m_setup(setup)
{
}

CoordinationPlan::~CoordinationPlan()
{
}

const char* CoordinationPlan::GetName() const
{
	return m_name.c_str();
}

bool CoordinationPlan::LoadFromXML(uint32 planId, const XmlNodeRef& rootNode)
{
	if (rootNode->haveAttr("name"))
	{
		const char* name;
		rootNode->getAttr("name", &name);
		m_name = name;
	}
	else
		m_name.Format("Plan:%d", planId);

	int childCount = rootNode->getChildCount();

	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef childNode = rootNode->getChild(i);
		
		const char* tagName = childNode->getTag();

		if (!stricmp(tagName, "Step"))
		{
			CoordinationElement::Ptr step(new CoordinationStep(m_setup));
			
			if (!step->LoadFromXML(m_elements.size(), childNode))
				return false;

			m_elements.push_back(step);
		}
		else if (!stricmp(tagName, "Loop"))
		{
			CoordinationElement::Ptr loop(new CoordinationLoop(m_setup));

			if (!loop->LoadFromXML(m_elements.size(), childNode))
				return false;

			m_elements.push_back(loop);
		}
		else
		{
			AIWarning("Unexpected tag '%s' at line %d...", tagName, childNode->getLine());

			return false;
		}
	}

	return true;
}

uint32 CoordinationPlan::GetRunningCount() const
{
	return m_running.size();
}

const CoordinationID& CoordinationPlan::GetRunningCoordinationID(uint32 index) const
{
	assert(index >= 0 && index < m_running.size());
	RunningPlans::const_iterator it = m_running.begin();
	std::advance(it, index);

	return it->first;
}

uint32 CoordinationPlan::GetRunningElementID(const CoordinationID& coordinationID) const
{
	RunningPlans::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const RunningPlan& plan = it->second;
	return plan.elementId;
}

const CoordinationElement& CoordinationPlan::GetRunningElement(const CoordinationID& coordinationID) const
{
	RunningPlans::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const RunningPlan& plan = it->second;
	return *m_elements[plan.elementId];
}

uint32 CoordinationPlan::GetElementCount() const
{
	return m_elements.size();
}

const CoordinationElement& CoordinationPlan::GetElement(uint32 index) const
{
	assert(index >= 0 && index < m_elements.size());

	return *m_elements[index];
}

void CoordinationPlan::DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const
{
	context->Draw2dLabel(context.x + context.ident, context.y, context.fontSize, Col_CadetBlue, false, "%s", 
		GetName());
	context.y += context.lineHeight;

	uint32 elementCount = GetElementCount();
	uint32 runningElementID = GetRunningElementID(coordinationID);

	context.ident += context.identWidth;

	for (uint32 e = 0; e < elementCount; ++e)
	{
		const CoordinationElement& element = GetElement(e);
		const char* elementName = element.GetName();

		if (runningElementID != e)
		{
			context->Draw2dLabel(context.x + context.ident, context.y, context.fontSize, Col_DarkGray, false, "%s", elementName);
			context.y += context.lineHeight;
		}
		else
		{
			element.DebugDrawStatus(coordinationID, context);
		}
	}

	context.ident -= context.identWidth;
}


//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

ECoordinationState CoordinationPlan::GetState(const CoordinationID& coordinationID) const
{
	RunningPlans::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningPlan& plan = it->second;

		return plan.state;
	}

	return CoordinationNotRunning;
}

bool CoordinationPlan::CanStart(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params) const
{
	if (!m_elements.empty())
		return m_elements[0]->CanStart(coordinationID, params);

	return true;
}

void CoordinationPlan::Start(const CoordinationID& coordinationID)
{
	RunningPlans::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
		return;

	std::pair<RunningPlans::iterator, bool> result = m_running.insert(RunningPlans::value_type(coordinationID, RunningPlan()));
	RunningPlan& running = result.first->second;

	running.elementId = 0;

	CoordinationElement::Ptr& element = m_elements[running.elementId];
	element->Start(coordinationID);
}

void CoordinationPlan::Stop(const CoordinationID& coordinationID)
{
	RunningPlans::iterator it = m_running.find(coordinationID);

	if (it != m_running.end())
	{
		RunningPlan& running = it->second;

		if (running.elementId < m_elements.size())
			m_elements[running.elementId]->Stop(coordinationID);

		m_running.erase(it);
	}
}

void CoordinationPlan::Update(float frameTime)
{
	if (!m_running.empty())
	{
		CoordinationElements::iterator it = m_elements.begin();
		CoordinationElements::iterator end = m_elements.end();

		for ( ; it != end; ++it)
			(*it)->Update(frameTime);

		RunningPlans::iterator rit = m_running.begin();
		RunningPlans::iterator rend = m_running.end();

		for ( ; rit != rend; ++rit)
		{
			RunningPlan& plan = rit->second;
			const CoordinationID& coordinationID = rit->first;

			if (plan.elementId >= m_elements.size())
				continue;

			ECoordinationState state = m_elements[plan.elementId]->GetState(coordinationID);
			if (state == CoordinationRunning)
				continue;
			else if (state == CoordinationFinished)
			{
				m_elements[plan.elementId]->Stop(coordinationID);

				++plan.elementId;
				if (plan.elementId >= m_elements.size())
				{
					plan.state = CoordinationFinished;
					continue;
				}

				const CoordinationInternalStartParams& params = m_setup->GetParams(coordinationID);

				if (m_elements[plan.elementId]->CanStart(coordinationID, params))
				{
					m_elements[plan.elementId]->Start(coordinationID);

					// TODO: Should support instant-finish here
					continue;
				}
			}

			plan.state = CoordinationFailed;
		}
	}
}