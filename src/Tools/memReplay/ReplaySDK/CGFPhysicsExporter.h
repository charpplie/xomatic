#pragma once

#include "ICSVExporter.h"

class CGFPhysicsCSVExporter : public ICSVExporter
{
public:
	void Export(ICSVCollator& collator, const ContextTreeNode* node) const;
};
