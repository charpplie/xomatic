#ifndef __SelectionVariables_h__
#define __SelectionVariables_h__

#pragma once

/*
	This file implements variable value and variable declaration collections.
*/

#include "BlockyXml.h"


typedef uint32 SelectionVariableID;


class SelectionVariableDeclarations;
class SelectionVariables
{
public:
	SelectionVariables();

	bool GetVariable(const SelectionVariableID& variableID, bool* value) const;
	bool SetVariable(const SelectionVariableID& variableID, bool value);

	void ResetChanged(bool state = false);
	bool Changed() const;

	void DebugDraw(bool history, const SelectionVariableDeclarations& declarations);

private:
	struct Variable
	{
		Variable()
			: value(false)
		{
		}

		bool value;
	};

	typedef std::map<SelectionVariableID, Variable> Variables;
	Variables m_variables;

	bool m_changed;

	struct VariableChangeEvent
	{
		VariableChangeEvent(const CTimeValue& _when, const SelectionVariableID _variableID, bool _value)
			: when(_when)
			, variableID(_variableID)
			, value(_value)
		{
		}

		CTimeValue when;
		SelectionVariableID variableID;
		bool value;
	};

	typedef std::deque<VariableChangeEvent> History;
	History m_history;
};


class SelectionVariableDeclarations
{
public:
	struct VariableDescription
	{
		VariableDescription(const char* _name)
			: name(_name)
		{
		}

		string name;
	};

	SelectionVariableDeclarations();
	bool LoadFromXML(const BlockyXmlBlocks::Ptr& blocks, const XmlNodeRef& rootNode, const char* scopeName, const char* fileName);
	
	bool IsDeclared(const SelectionVariableID& variableID) const;

	SelectionVariableID GetVariableID(const char* name) const;
	const VariableDescription& GetVariableDescription(const SelectionVariableID& variableID) const;

	const SelectionVariables& GetDefaults() const;

private:
	typedef std::map<SelectionVariableID, VariableDescription> VariableDescriptions;
	VariableDescriptions m_descriptions;
	SelectionVariables m_defaults;
};

#endif