#include "StdAfx.h"
#include "SelectionVariables.h"

#include <StringUtils.h>
#include <DebugDrawContext.h>


SelectionVariables::SelectionVariables()
: m_changed(false)
{
}

bool SelectionVariables::GetVariable(const SelectionVariableID& variableID, bool* value) const
{
	Variables::const_iterator it = m_variables.find(variableID);
	
	if (it != m_variables.end())
	{
		const Variable& variable = it->second;
		*value = variable.value;

		return true;
	}

	return false;
}

bool SelectionVariables::SetVariable(const SelectionVariableID& variableID, bool value)
{
	Variable& variable = stl::map_insert_or_get(m_variables, variableID, Variable());
	if (variable.value != value)
	{
		variable.value = value;
		m_changed = true;

		CTimeValue now = gEnv->pTimer->GetFrameStartTime();

		m_history.push_front(VariableChangeEvent(now, variableID, value));
		while (m_history.size() > 25)
			m_history.pop_back();
	}

	return false;
}

void SelectionVariables::ResetChanged(bool state)
{
	m_changed = state;
}

bool SelectionVariables::Changed() const
{
	return m_changed;
}

struct DebugVariable
{
	const char* name;
	bool value;

	bool operator <(const DebugVariable& other) const
	{
		return strcmp(name, other.name) < 0;
	}
};

void SelectionVariables::DebugDraw(bool history, const SelectionVariableDeclarations& declarations)
{
	CDebugDrawContext dc;

	float minY = 420.0f;
	float maxY = -FLT_MAX;

	float x = 10.0f;
	float y = minY;
	const float fontSize = 1.25f;
	const float lineHeight = 11.5f * fontSize;
	const float columnWidth = 145.0f;

	Variables::const_iterator it = m_variables.begin();
	Variables::const_iterator end = m_variables.end();

	std::vector<DebugVariable> sorted;

	for ( ; it != end; ++it)
	{
		const SelectionVariableID& variableID = it->first;
		bool value = it->second.value;

		const SelectionVariableDeclarations::VariableDescription& description = declarations.GetVariableDescription(variableID);

		sorted.resize(sorted.size() + 1);
		sorted.back().name = description.name.c_str();
		sorted.back().value = value;
	}

	std::sort(sorted.begin(), sorted.end());

	ColorB trueColor(Col_SlateBlue, 1.0f);
	ColorB falseColor(Col_DarkGray, 1.0f);

	stack_string text;

	if (sorted.empty())
		return;

	uint32 variableCount = sorted.size();
	for (uint32 i = 0; i < variableCount; ++i)
	{
		dc->Draw2dLabel(x, y, fontSize, sorted[i].value ? trueColor : falseColor, false, "%s", sorted[i].name);
		y+= lineHeight;

		if (y > maxY)
			maxY = y;

		if (y + lineHeight > 760.0f)
		{
			y = minY;
			x += columnWidth;
		}
	}

	//dc->SetAlphaBlended(false);
	//dc->SetDepthTest(false);

	//dc->Init2DMode();
	//dc->Draw2dImage(x + columnWidth, minY, 1, maxY - minY, -1);
	
	if (history)
	{
		y = minY;
		CTimeValue now = gEnv->pTimer->GetFrameStartTime();

		History::iterator it = m_history.begin();
		History::iterator end = m_history.end();

		for ( ; it != end; ++it)
		{
			const VariableChangeEvent& changeEvent = *it;
			float alpha = 1.0f - (now - changeEvent.when).GetSeconds() / 10.0f;
			if (alpha > 0.01f)
			{
				alpha = clamp(alpha, 0.33f, 1.0f);
				const SelectionVariableDeclarations::VariableDescription& description = 
					declarations.GetVariableDescription(changeEvent.variableID);

				trueColor.a = (uint8)(alpha * 255.5f);
				falseColor.a = (uint8)(alpha * 255.5f);

				text = description.name;
				dc->Draw2dLabel(x + columnWidth + 2.0f, y, fontSize, changeEvent.value ? trueColor : falseColor, false, 
					"%s", text.c_str());

				y += lineHeight;
			}
		}
	}
}

SelectionVariableDeclarations::SelectionVariableDeclarations()
{
}

bool SelectionVariableDeclarations::LoadFromXML(const BlockyXmlBlocks::Ptr& blocks, const XmlNodeRef& rootNode,
																								const char* scopeName, const char* fileName)
{
	assert(scopeName);

	BlockyXmlNodeRef blockyNode(blocks, scopeName, rootNode, fileName);
	while (XmlNodeRef childNode = blockyNode.next())
	{
		if (!stricmp(childNode->getTag(), "Variable"))
		{
			const char* variableName = 0;
			if (childNode->haveAttr("name"))
				childNode->getAttr("name", &variableName);
			else
			{
				AIWarning("Missing 'name' attribute for tag '%s' in file '%s' at line %d.",
					childNode->getTag(), fileName, childNode->getLine());

				return false;
			}

			bool defaultValue = false;
			if (childNode->haveAttr("default"))
			{
				const char* value = 0;
				childNode->getAttr("default", &value);

				if (!stricmp(value, "true"))
					defaultValue = true;
				else if (!stricmp(value, "false"))
					defaultValue = false;
				else
				{
					AIWarning("Invalid variable value '%s' for variable '%s' in file '%s' at line %d.",
						value, variableName, fileName, childNode->getLine());

					return false;
				}
			}

			SelectionVariableID variableID = GetVariableID(variableName);
			std::pair<VariableDescriptions::iterator, bool> iresult = m_descriptions.insert(
				VariableDescriptions::value_type(variableID, VariableDescription(variableName)));

			if (!iresult.second)
			{
				if (!stricmp(iresult.first->second.name, variableName))
				{
					AIWarning("Duplicate variable declaration '%s' in file '%s' at line %d.",
						variableName, fileName, childNode->getLine());
				}
				else
				{
					AIWarning("Hash collision for variable declaration '%s' in file '%s' at line %d.",
						variableName, fileName, childNode->getLine());
				}

				return false;
			}

			m_defaults.SetVariable(variableID, defaultValue);
			m_defaults.ResetChanged(true);
		}
		else
		{
			AIWarning("Unexpected tag '%s' in file '%s' at line %d. 'Variable' expected.",
				childNode->getTag(), fileName, childNode->getLine());

			return false;
		}
	}

	return true;
}

bool SelectionVariableDeclarations::IsDeclared(const SelectionVariableID& variableID) const
{
	return m_descriptions.find(variableID) != m_descriptions.end();
}

SelectionVariableID SelectionVariableDeclarations::GetVariableID(const char* name) const
{
	return CryStringUtils::CalculateHashLowerCase(name);
}

const SelectionVariableDeclarations::VariableDescription& SelectionVariableDeclarations::GetVariableDescription(
	const SelectionVariableID& variableID) const
{
	VariableDescriptions::const_iterator it = m_descriptions.find(variableID);
	if (it != m_descriptions.end())
	{
		const VariableDescription& description = it->second;
		return description;
	}

	static VariableDescription empty("<invalid>");
	return empty;
}

const SelectionVariables& SelectionVariableDeclarations::GetDefaults() const
{
	return m_defaults;
}