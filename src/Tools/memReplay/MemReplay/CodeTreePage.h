#pragma once

#include "GenericTreePage.h"

class CodeTreeStyle : public ITreeMapStyle
{
public:
	CodeTreeStyle(const SharedPtr<ISymbolTable>& symbols, const GenericTreeStream<SizeInfoGroups>& szInclusiveStream);

	double MeasureNode(const treemapItem* item) const;
	void GetLeafNodeText(const treemapItem* item, char* textOut, size_t textOutCapacity) const;
	void GetNodeColour(const treemapItem* item, int depth, float* colourOut) const;
	void GetPopupText(const treemapItem* item, std::string& popupTextOut) const;
	const char* GetNodeTitle(const treemapItem* item) const;
	void SetMemoryGroup(MemGroups::Group group) { m_group = group; }
	void ToggleCountScaling() {m_countScaling=!m_countScaling;}
	bool GetCountScaling() {return m_countScaling;}

private:
	MemGroups::Group m_group;
	SharedPtr<ISymbolTable> m_symbols;
	const GenericTreeStream<SizeInfoGroups>* m_szInclusiveStream;
	bool m_countScaling;
};

class CCodeTreePage : public CGenericTreePage
{
public:
	CCodeTreePage(const SharedPtr<GenericTree>& memHier, const SharedPtr<ISymbolTable>& symbols, const std::string& name);

	void OnTabActivate();

	void OnInspectClicked();
	void OnReverseClicked(GatherType gt);
	void OnGatherTDClicked(GatherType gt);
	void OnGatherBUClicked(GatherType gt);
	void OnExcludeClicked(GatherType gt);
	void OnPlotClicked(GatherType gt);
	void OnEditCopy();
	void SetMemoryType(int type);
	void ToggleCountScaling();
	bool GetCountScaling();

private:
	bool FindMatches(StrStrHandler searchFunc, const char* searchString, const GenericTreeNode* searchNode);
	void FormatTableItem(char* nameBuffer, const GenericTreeNode* item);
	void GetColumnDefs(std::vector<ColumnDef>& headers);
	std::vector<TAddress> CollectAddressesFor(const GenericTreeNode* node, GatherType gt);
	std::string GetNameFor(const GenericTreeNode* node, GatherType gt);

	void BuildPathRemap(const char* from, const char* to);
	std::string ApplyPathRemap(const char* from);

private:
	SharedPtr<ISymbolTable> m_symbols;
	CodeTreeStyle m_style;

	GenericCountColumnSorter m_totalCountSorter;
	GenericCountColumnSorter m_allocCountSorter;
	GenericCountColumnSorter m_freeCountSorter;

	GenericSizeColumnSorter m_inclTotalSorter;
	GenericSizeColumnSorter m_inclMainSorter;
	GenericSizeColumnSorter m_inclRSXSorter;

	GenericSizeColumnSorter m_exclTotalSorter;
	GenericSizeColumnSorter m_exclMainSorter;
	GenericSizeColumnSorter m_exclRSXSorter;

	size_t m_remapFrom;
	std::string m_remapTo;
};
