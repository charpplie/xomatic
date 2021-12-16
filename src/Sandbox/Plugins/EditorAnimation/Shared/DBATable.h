#pragma once

#include "AnimationFilter.h"

namespace Serialization { class IArchive; }

struct SDBAEntry
{
	SAnimationFilter filter;
	string path;

	void Serialize(Serialization::IArchive& ar);
};

class XmlNodeRef;
struct SDBATable
{
	std::vector<SDBAEntry> entries;

	void Serialize(Serialization::IArchive& ar);

	// returns -1 when nothing is found
	int FindDBAForAnimation(const SAnimationFilterItem& animation) const;

	bool Load(const char* dbaTablePath);
	bool Save(const char* dbaTablePath);

	bool ImportOldXML(const XmlNodeRef& root);
};
