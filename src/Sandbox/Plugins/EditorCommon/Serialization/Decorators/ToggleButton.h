#pragma once

namespace Serialization
{
class IArchive;

struct ToggleButton
{
	bool* value;

	ToggleButton(bool& value)
	: value(&value)
	{
	}
};

struct RadioButton
{
	int* value;
	int buttonValue;

	RadioButton(int& value, int buttonValue)
	: value(&value)
	, buttonValue(buttonValue)
	{
	}
};

bool Serialize(Serialization::IArchive& ar, Serialization::ToggleButton& button, const char* name, const char* label);
bool Serialize(Serialization::IArchive& ar, Serialization::RadioButton& button, const char* name, const char* label);

}

#include "ToggleButtonImpl.h"
