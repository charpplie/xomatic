#pragma once

class ContextTreeNode;

class ICSVCollator
{
public:
	virtual ~ICSVCollator() {}

	virtual void AddRow(const char** cols, size_t numCols) = 0;
};

class ICSVExporter
{
public:
	virtual ~ICSVExporter() {}

	virtual void Export(ICSVCollator& collator, const ContextTreeNode* node) const = 0;
};