#include "Serialization.h"
#include "AnimSettings.h"

bool Serialize(Serialization::IArchive& ar, SkeletonAlias& value, const char* name, const char* label)
{
	return ar(value.alias, name, label);
}

