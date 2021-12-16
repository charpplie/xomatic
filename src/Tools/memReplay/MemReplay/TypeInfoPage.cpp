// TypeInfoPage.cpp : implementation file
//

#include "stdafx.h"
#include "TypeInfoPage.h"

#include "ReplaySDK/TypeSymbol.h"

#include "MainFrm.h"
#include "MemReplay.h"

static std::string CreateTitle(const char* name)
{
	char title[512];
	_snprintf_s(title, 512, "TypeInfo (%s)", name);
	return title;
}

CTypeInfoPage::CTypeInfoPage(const SharedPtr<GenericTree>& typeTree, const std::string& name)
	: CGenericTreePage(typeTree, CreateTitle(name.c_str()))
{
	SetTreeMapStyle(&m_style);
}

void CTypeInfoPage::OnTabActivate()
{
	theApp.GetMainFrame()->SetActiveToolBar(CMainFrame::TB_None);
}

bool CTypeInfoPage::FindMatches(StrStrHandler searchFunc, const char* searchString, const GenericTreeNode* searchNode)
{
	if (searchFunc(searchNode->GetKey<TypeSymbol*>()->m_name, searchString))
		return true;
	if (searchFunc(searchNode->GetKey<TypeSymbol*>()->m_typeName, searchString))
		return true;
	return false;
}

void CTypeInfoPage::FormatTableItem(char* nameBuffer, const GenericTreeNode* item)
{
	TypeSymbol *type = item->GetKey<TypeSymbol*>();

	char sizeTotalStr[32];
	int size=type->m_length;
	if (size&7)
		sprintf(sizeTotalStr, "%d bits", size);
	else
		sprintf(sizeTotalStr, "%d bytes", size/8);

	if (type)
	{
		sprintf(nameBuffer, "%s\t%s\t%s\t%d", type->m_name, type->m_typeName, sizeTotalStr, type->m_offset);
	}
	else
	{
		sprintf(nameBuffer, "%s\t%s\t%s\t%d", "Waste", "Waste", sizeTotalStr, -1);
	}
}

void CTypeInfoPage::GetColumnDefs(std::vector<ColumnDef>& headers)
{
	ColumnDef col = {0};
	col.hdr.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
	col.hdr.fmt = HDF_LEFT | HDF_STRING;

	headers.clear();
	headers.resize(4, col);

	headers[0].hdr.cxy = 300;
	headers[0].hdr.pszText = "Member Name";

	headers[1].hdr.cxy = 200;
	headers[1].hdr.pszText = "Type";

	headers[2].hdr.cxy = 50;
	headers[2].hdr.pszText = "Size";

	headers[3].hdr.cxy = 50;
	headers[3].hdr.pszText = "Offset";
}

double TypeInfoStyle::MeasureNode(const treemapItem* item) const
{
	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);
	if (node)
	{
		return node->GetKey<TypeSymbol*>()->m_length;
	}
	else
	{
		return item->size;
	}
}

void TypeInfoStyle::GetLeafNodeText(const treemapItem* item, char* textOut, size_t textOutCapacity) const
{
	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);
	TypeSymbol* type = node->GetKey<TypeSymbol*>();

	if (type->m_typeName==type->m_name)
		_snprintf(textOut, textOutCapacity, "%s:%d", type->m_name, item->size);
	else
		_snprintf(textOut, textOutCapacity, "%s %s:%d", type->m_typeName, type->m_name, item->size);
}

void TypeInfoStyle::GetNodeColour(const treemapItem* node, int depth, float* col) const
{
	if (0||node->children.empty())
	{
		const GenericTreeNode* gennode = reinterpret_cast<const GenericTreeNode*>(node->clientNode);
		TypeSymbol* type = gennode->GetKey<TypeSymbol*>();
		float eff = type->m_name ? 1.0f : 0.0f;
		col[0] = 1.0f - eff;
		col[1] = eff;
		col[2] = 0.0f;
		col[3] = 1.0f;
	}
	else
	{
		CGLTreemapWnd::DefaultColour(node, depth, col);
	}
}

void TypeInfoStyle::GetPopupText(const treemapItem* item, std::string& popupTextOut) const
{
	char temp[2028];

	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);

	unsigned int size = 0;
	unsigned int count = 1;
	
	TypeSymbol* type = node ? node->GetKey<TypeSymbol*>() : 0;

	if (type)
	{
		size = type->m_length;
		count = type->m_count;
	}
	else
	{
		size = (unsigned int)item->defaultSize;
	}

	char sizeFmt[64];

	if (size >= 8)
	{
		FormatSize(sizeFmt, 64, size / 8);
		strcat(sizeFmt, " bytes");
	}
	else
	{
		FormatSize(sizeFmt, 64, size);
		strcat(sizeFmt, " bits");
	}

	int meanSize = (count != 0) ? (size / count) : 0;

	char meanSizeFmt[64];
	if (meanSize >= 8)
	{
		FormatSize(meanSizeFmt, 64, meanSize / 8);
		strcat(meanSizeFmt, " bytes");
	}
	else
	{
		FormatSize(meanSizeFmt, 64, meanSize);
		strcat(meanSizeFmt, " bits");
	}

	_snprintf_s(temp, 2048, "%s\n\n%s: %i\nCount: %d\nSize: %s\nMean alloc size: %s\n",
		type->m_name,
		type->m_typeName,
		0,
		count,
		sizeFmt,
		meanSizeFmt);

	popupTextOut = temp;
}

const char* TypeInfoStyle::GetNodeTitle(const treemapItem* item) const
{
	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);
	TypeSymbol* type = node->GetKey<TypeSymbol*>();

	return type ? type->m_name : "Waste";
}
