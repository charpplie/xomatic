#pragma once

#include <Serialization/IArchive.h>
#include <functional>

namespace Serialization
{

struct ActionButton
{
	typedef std::function<void()> Functor;
	Functor callback;
	string icon;

	explicit ActionButton(const Functor& callback, const char* icon = "")
	: callback(callback)
	, icon(icon)
	{
	}
};

inline bool Serialize(Serialization::IArchive& ar, Serialization::ActionButton& button, const char* name, const char* label)
{
	if (ar.IsEdit())
		return ar(Serialization::SStruct::ForEdit(button), name, label);
	else
		return false;
}

}

