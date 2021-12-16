#include "StdAfx.h"
#include "CoordinationLoop.h"
#include "CoordinationStep.h"
#include "CoordinationSetup.h"

#include "DebugDrawContext.h"


CoordinationLoop::CoordinationLoop(const CoordinationSetup* setup)
: m_setup(setup)
{
}

bool CoordinationLoop::LoadFromXML(uint32 elementId, const XmlNodeRef& rootNode)
{
	if (rootNode->haveAttr("name"))
	{
		const char* name;
		rootNode->getAttr("name", &name);
		m_name = name;
	}
	else
		m_name.Format("Loop:%d", elementId);

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


uint32 CoordinationLoop::GetRunningCount() const
{
	return m_running.size();
}

const CoordinationID& CoordinationLoop::GetRunningCoordinationID(uint32 index) const
{
	assert(index >= 0 && index < m_running.size());
	RunningLoops::const_iterator it = m_running.begin();
	std::advance(it, index);

	return it->first;
}

uint32 CoordinationLoop::GetRunningElementID(const CoordinationID& coordinationID) const
{
	RunningLoops::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const RunningLoop& loop = it->second;
	return loop.elementID;
}

const CoordinationElement& CoordinationLoop::GetRunningElement(const CoordinationID& coordinationID) const
{
	RunningLoops::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const RunningLoop& loop = it->second;
	return *m_elements[loop.elementID];
}

uint32 CoordinationLoop::GetElementCount() const
{
	return m_elements.size();
}

const CoordinationElement& CoordinationLoop::GetElement(uint32 index) const
{
	assert(index >= 0 && index < m_elements.size());

	return *m_elements[index];
}

void CoordinationLoop::DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const
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

ECoordinationState CoordinationLoop::GetState(const CoordinationID& coordinationID) const
{
	RunningLoops::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningLoop& loop = it->second;

		return loop.state;
	}

	return CoordinationNotRunning;
}

bool CoordinationLoop::CanStart(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params) const
{
	if (!m_elements.empty())
		return m_elements[0]->CanStart(coordinationID, params);

	return true;
}

void CoordinationLoop::Start(const CoordinationID& coordinationID)
{
	RunningLoops::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
		return;

	std::pair<RunningLoops::iterator, bool> result = m_running.insert(RunningLoops::value_type(coordinationID, RunningLoop()));
	RunningLoop& running = result.first->second;

	running.elementID = 0;
	running.state = CoordinationRunning;

	CoordinationElement::Ptr& element = m_elements[running.elementID];
	element->Start(coordinationID);
}

void CoordinationLoop::Stop(const CoordinationID& coordinationID)
{
	RunningLoops::iterator it = m_running.find(coordinationID);

	if (it != m_running.end())
	{
		RunningLoop& running = it->second;

		m_elements[running.elementID]->Stop(coordinationID);

		m_running.erase(it);
	}
}

void CoordinationLoop::Update(float frameTime)
{
	if (!m_running.empty())
	{
		CoordinationElements::iterator it = m_elements.begin();
		CoordinationElements::iterator end = m_elements.end();

		for ( ; it != end; ++it)
			(*it)->Update(frameTime);

		RunningLoops::iterator lit = m_running.begin();
		RunningLoops::iterator lend = m_running.end();

		for ( ; lit != lend; ++lit)
		{
			RunningLoop& loop = lit->second;
			const CoordinationID& coordinationID = lit->first;

			ECoordinationState state = m_elements[loop.elementID]->GetState(coordinationID);
			if (state == CoordinationRunning)
				continue;
			else if (state == CoordinationFinished)
			{
				m_elements[loop.elementID]->Stop(coordinationID);

				++loop.elementID;
				if (loop.elementID >= m_elements.size())
					loop.elementID = 0;

				const CoordinationInternalStartParams& params = m_setup->GetParams(coordinationID);

				if (m_elements[loop.elementID]->CanStart(coordinationID, params))
				{
					m_elements[loop.elementID]->Start(coordinationID);

					// TODO: Should support instant-finish here
					continue;
				}
			}

			loop.state = CoordinationFailed;
		}
	}
}