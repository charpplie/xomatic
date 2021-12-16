// Moved these out into their own file so they can be shared between the GUI tool
// (Demo) and the command line tool (SummaryTool)

#ifndef __CSVCOLLATOR_H__
#define __CSVCOLLATOR_H__

#include "ExcelExport.h"
#include "ICSVExporter.h"

class WorksheetCSVCollator : public ICSVCollator
{
public:
	explicit WorksheetCSVCollator(ExcelExportWorksheet& ws);

	void AddRow(const char** cols, size_t numCols);

private:
	ExcelExportWorksheet* m_ws;
	bool m_firstRow;
};

class ContextTreeCSVExporter : public ICSVExporter
{
public:
	void Export(ICSVCollator& collator, const ContextTreeNode* node) const;
};

#endif
