#pragma once

#include "GenericTreePage.h"

class TypeInfoStyle : public ITreeMapStyle
{
public:
	double MeasureNode(const treemapItem* item) const;
	void GetLeafNodeText(const treemapItem* item, char* textOut, size_t textOutCapacity) const;
	void GetNodeColour(const treemapItem* item, int depth, float* colourOut) const;
	void GetPopupText(const treemapItem* item, std::string& popupTextOut) const;
	const char* GetNodeTitle(const treemapItem* item) const;
};

// CTypeInfoPage dialog

class CTypeInfoPage : public CGenericTreePage
{
public:
	CTypeInfoPage(const SharedPtr<GenericTree>& typeTree, const std::string& name);

public: // ITabPage Members
	void OnTabActivate();

private:
	bool FindMatches(StrStrHandler searchFunc, const char* searchString, const GenericTreeNode* searchNode);
	void FormatTableItem(char* nameBuffer, const GenericTreeNode* item);
	void GetColumnDefs(std::vector<ColumnDef>& headers);

private:
	TypeInfoStyle m_style;
};
