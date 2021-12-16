#pragma once

namespace Serialization
{

inline bool Serialize(Serialization::IArchive& ar, Serialization::ToggleButton& button, const char* name, const char* label)
{
	if (ar.IsEdit())
		return ar(Serialization::SStruct::ForEdit(button), name, label);
	else
		return ar(*button.value, name, label);
}

inline bool Serialize(Serialization::IArchive& ar, Serialization::RadioButton& button, const char* name, const char* label)
{
	if (ar.IsEdit())
		return ar(Serialization::SStruct::ForEdit(button), name, label);
	else
		return false;
}

}