#pragma once

#include "GenericTreePage.h"

class SizerTreeMapStyle : public ITreeMapStyle
{
public:
	SizerTreeMapStyle(const GenericTreeStream<SizeInfoGroups>& szInclusiveStream);

	double MeasureNode(const treemapItem* item) const;
	void GetLeafNodeText(const treemapItem* item, char* textOut, size_t textOutCapacity) const;
	void GetNodeColour(const treemapItem* item, int depth, float* colourOut) const;
	void GetPopupText(const treemapItem* item, std::string& popupTextOut) const;
	const char* GetNodeTitle(const treemapItem* item) const;

private:
	const GenericTreeStream<SizeInfoGroups>* m_szInclusiveStream;
};

class CSizerTreePage : public CGenericTreePage
{
public:
	CSizerTreePage(const SharedPtr<GenericTree>& tree, const std::string& name);

	void OnTabActivate();

	void OnInspectClicked();
	void OnReverseClicked();
	void OnGatherTDClicked(GatherType gt);
	void OnExcludeClicked(GatherType gt);

private:
	bool FindMatches(StrStrHandler searchFunc, const char* searchString, const GenericTreeNode* searchNode);
	void FormatTableItem(char* nameBuffer, const GenericTreeNode* item);
	void GetColumnDefs(std::vector<ColumnDef>& headers);

private:
	SizerTreeMapStyle m_style;
};
