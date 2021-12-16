#include "stdafx.h"
#include "CodeTreePage.h"
#include "VSInterop.h"

#include "MainFrm.h"
#include "MemReplayView.h"
#include "MemReplay.h"

namespace
{
	float GetEfficiency(const SizeInfoGroups& sz)
	{
		return (sz.GetTotal().consumed == 0)
			? 100.0f
			: (static_cast<float>(sz.GetTotal().requested) / static_cast<float>(sz.GetTotal().consumed)) * 100.0f;
	}

	bool FindSymbol(const ISymbolTable& syms, TAddress addr, const char*& name, const char*& filename, int& line)
	{
		syms.GetSymbol(addr, name, filename, line);

		if (filename)
		{
			// Get the filename part of the path.
			const char* relFilename = std::max(strrchr( filename, '\\' ), strrchr( filename, '/' ));
			filename = relFilename
				? relFilename + 1
				: filename;
		}

		return (name != NULL) && (filename != NULL);
	}

	char* MakeTableEntry(
		char *nameBuffer,
		const ISymbolTable& symbols,
		const GenericTreeStream<SizeInfoGroups>& szInclsiveStream,
		const GenericTreeStream<SizeInfoGroups>& szExclusiveStream,
		const GenericTreeNode* hierNode)
	{
		TAddress address = hierNode->GetKey<TAddress>();
		const char *context=NULL;

		const SizeInfoGroups& szInclusive = szInclsiveStream[hierNode->id];
		const SizeInfoGroups& szExclusive = szExclusiveStream[hierNode->id];

		char sizeTotalStr[32], sizeMainStr[32], sizeRSXStr[32];
		FormatSize(sizeTotalStr, 32, szInclusive.GetTotal().consumed);
		FormatSize(sizeMainStr, 32, szInclusive.GetGroup(MemGroups::Main).consumed);
		FormatSize(sizeRSXStr, 32, szInclusive.GetGroup(MemGroups::RSX).consumed);

		char exsizeTotalStr[32], exsizeMainStr[32], exsizeRSXStr[32];
		FormatSize(exsizeTotalStr, 32, szExclusive.GetTotal().consumed);
		FormatSize(exsizeMainStr, 32, szExclusive.GetGroup(MemGroups::Main).consumed);
		FormatSize(exsizeRSXStr, 32, szExclusive.GetGroup(MemGroups::RSX).consumed);

		char sizeStr[64];
		sprintf_s(sizeStr, 64, "%s\t%s\t%s\t%s\t%s\t%s",
			sizeTotalStr, sizeMainStr, sizeRSXStr,
			exsizeTotalStr, exsizeMainStr, exsizeRSXStr);

		const char* symbolName;
		const char* filename;
		int line;
		if (FindSymbol(symbols, address, symbolName, filename, line))
		{
			int acount = szInclusive.GetTotal().allocCount;
			int fcount = szInclusive.GetTotal().freeCount;
			int count = acount - fcount;

			char countStr[32], acountStr[43], fcountStr[32];

			sprintf(nameBuffer, "%s\t%s:%i (%08x)\t%s\t%s\t%s\t%s\t%i%%", 
				symbolName, filename, line, address, 
				FormatThousands(countStr, 32, count), FormatThousands(acountStr, 32, acount), FormatThousands(fcountStr, 32, fcount),
				sizeStr, static_cast<int>(GetEfficiency(szInclusive)));
		}

		return nameBuffer;
	}

	void SanitizeName(char* out, size_t outCapacity, const char* in)
	{
		const char* parenth = strchr(in, '(');
		const char* templ = strchr(in, '<');
		const char* term = parenth && templ
			? std::min(parenth, templ)
			: (parenth ? parenth : templ);
		if (term)
		{
			size_t n = std::min<size_t>(outCapacity - 1, term - in);
			strncpy(out, in, n);
			out[n] = '\0';
		}
		else
		{
			strncpy(out, in, outCapacity - 1);
			out[outCapacity - 1] = '\0';
		}
	}

	std::string CreateTabName(const char* baseName, const char* subName)
	{
		std::string result;

		const char* sep = strchr(baseName, '/');
		if (sep)
		{
			result = std::string(baseName, sep);
			result += "/.../";
		}
		else
		{
			result = std::string(baseName) + "/";
		}

		result += subName;

		return result;
	}

	void SplitPath(std::vector<std::pair<std::string, ptrdiff_t> >& out, const char* str)
	{
		const char* p = str;

		out.push_back(std::make_pair(std::string(), p - str));

		for (; *p; ++ p)
		{
			if (*p != '\\')
				out.back().first += tolower(*p);
			else
				out.push_back(std::make_pair(std::string(), p - str + 1));
		}
	}
}

CodeTreeStyle::CodeTreeStyle(const SharedPtr<ISymbolTable>& symbols, const GenericTreeStream<SizeInfoGroups>& szInclusiveStream)
	: m_symbols(symbols)
	, m_szInclusiveStream(&szInclusiveStream)
{
	m_countScaling=false;
	m_group=MemGroups::Count;
}

double CodeTreeStyle::MeasureNode(const treemapItem* item) const
{
	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);
	if (node)
	{
		const SizeInfoGroups& sz = (*m_szInclusiveStream)[node->id];
		if (m_countScaling)
			return abs((m_group == MemGroups::Count) ? (sz.GetTotal().allocCount-sz.GetTotal().freeCount) : (sz.GetGroup(m_group).allocCount-sz.GetGroup(m_group).freeCount));
		else
			return abs((m_group == MemGroups::Count) ? sz.GetTotal().consumed : sz.GetGroup(m_group).consumed);
	}
	else
	{
		return item->size;
	}
}

void CodeTreeStyle::GetLeafNodeText(const treemapItem* item, char* textOut, size_t textOutCapacity) const
{
	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);
	if (node)
	{
		const SizeInfoGroups& sz = (*m_szInclusiveStream)[node->id];

		char totalSize[32], mainSize[32], rsxSize[32];
		FormatSize(totalSize, sizeof(totalSize), sz.GetTotal().consumed);
		FormatSize(mainSize, 32, sz.GetGroup(MemGroups::Main).consumed);
		FormatSize(rsxSize, 32, sz.GetGroup(MemGroups::RSX).consumed);

		sprintf_s(textOut, textOutCapacity, "%s (%s/%s)", totalSize, mainSize, rsxSize);
	}
	else
	{
		FormatSize(textOut, textOutCapacity, (ptrdiff_t)item->size);
	}
}

void CodeTreeStyle::GetNodeColour(const treemapItem* node, int depth, float* col) const
{
	if (node->children.empty())
	{
		const GenericTreeNode* gennode = reinterpret_cast<const GenericTreeNode*>(node->clientNode);
		const SizeInfoGroups& sz = (*m_szInclusiveStream)[gennode->id];

		float eff = (sz.GetTotal().requested/(float)sz.GetTotal().consumed) < 0.75f ? 0.0f : 1.0f;
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

void CodeTreeStyle::GetPopupText(const treemapItem* item, std::string& popupTextOut) const
{
	char temp[2028];

	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);

	char sizeFmt[64];
	int count;
	int size;
	float eff=1.0f;
	
	if (node)
	{
		const SizeInfoGroups& sz = (*m_szInclusiveStream)[node->id];
		eff=(sz.GetTotal().requested/(float)sz.GetTotal().consumed);

		char totalSize[32], mainSize[32], rsxSize[32];
		size=sz.GetTotal().consumed;
		count=sz.GetTotal().allocCount-sz.GetTotal().freeCount;
		FormatSize(totalSize, sizeof(totalSize), size);
		FormatSize(mainSize, sizeof(mainSize), sz.GetGroup(MemGroups::Main).consumed);
		FormatSize(rsxSize, sizeof(rsxSize), sz.GetGroup(MemGroups::RSX).consumed);

		const GenericTreeNode* root = node;
		while (root->parent)
			root = root->parent;

		float percent = (sz.GetTotal().consumed * 100.0f) / (*m_szInclusiveStream)[root->id].GetTotal().consumed;
		sprintf_s(sizeFmt, sizeof(sizeFmt), "%s (%s/%s) %f%%", totalSize, mainSize, rsxSize, percent);
	}
	else
	{
		FormatSize(sizeFmt, 64, (ptrdiff_t)item->defaultSize);
	}

	char meanSizeFmt[64];
	FormatSize(meanSizeFmt, 64, (count != 0) ? (size / count) : 0);

	const char* name;
	const char* file;
	int line;
	m_symbols->GetSymbol(node->GetKey<TAddress>(), name, file, line);

	_snprintf_s(temp, 2048, "%s\n\n%s: %i\nCount: %d\nSize: %s\nMean alloc size: %s\nEfficiency: %f%%",
		name, file, line,
		count,
		sizeFmt,
		meanSizeFmt,
		eff * 100.0f);

	popupTextOut = temp;
}

const char* CodeTreeStyle::GetNodeTitle(const treemapItem* item) const
{
	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);

	const char* name;
	const char* filename;
	int line;
	m_symbols->GetSymbol(node->GetKey<TAddress>(), name, filename, line);
	return name ? name : "";
}

CCodeTreePage::CCodeTreePage(const SharedPtr<GenericTree>& memHier, const SharedPtr<ISymbolTable>& symbols, const std::string& name)
	: CGenericTreePage(memHier, name)
	, m_symbols(symbols)
	, m_style(symbols, *memHier->GetStream<SizeInfoGroups>("Inclusive"))
	, m_totalCountSorter(*memHier->GetStream<SizeInfoGroups>("Inclusive"), (MemGroups::Group) -1, GenericCountColumnSorter::Total)
	, m_allocCountSorter(*memHier->GetStream<SizeInfoGroups>("Inclusive"), (MemGroups::Group) -1, GenericCountColumnSorter::Allocs)
	, m_freeCountSorter(*memHier->GetStream<SizeInfoGroups>("Inclusive"), (MemGroups::Group) -1, GenericCountColumnSorter::Frees)
	, m_inclTotalSorter(*memHier->GetStream<SizeInfoGroups>("Inclusive"), (MemGroups::Group) -1)
	, m_inclMainSorter(*memHier->GetStream<SizeInfoGroups>("Inclusive"), MemGroups::Main)
	, m_inclRSXSorter(*memHier->GetStream<SizeInfoGroups>("Inclusive"), MemGroups::RSX)
	, m_exclTotalSorter(*memHier->GetStream<SizeInfoGroups>("Exclusive"), (MemGroups::Group) -1)
	, m_exclMainSorter(*memHier->GetStream<SizeInfoGroups>("Exclusive"), MemGroups::Main)
	, m_exclRSXSorter(*memHier->GetStream<SizeInfoGroups>("Exclusive"), MemGroups::RSX)
	, m_remapFrom(0)
{
	SetTreeMapStyle(&m_style);
}

void CCodeTreePage::OnTabActivate()
{
	theApp.GetMainFrame()->SetActiveToolBar(CMainFrame::TB_Code);
}

void CCodeTreePage::OnInspectClicked()
{
	const GenericTreeNode* node = GetSelectedNode();

	if (node)
	{
		const char* name;
		const char* filename;
		int line;
		m_symbols->GetSymbol(node->GetKey<TAddress>(), name, filename, line);

		if (filename && line)
		{
			std::string saneFilename = filename;
			std::replace(saneFilename.begin(), saneFilename.end(), '/', '\\');

			std::string remapFilename = ApplyPathRemap(saneFilename.c_str());

			if (GetFileAttributes(remapFilename.c_str()) == INVALID_FILE_ATTRIBUTES)
			{
				std::string::size_type sep = saneFilename.rfind('\\');
				std::string baseFilename = (sep == std::string::npos)
					? saneFilename
					: saneFilename.substr(sep + 1);

				CString filter;
				filter += baseFilename.c_str();
				filter += "|";
				filter += baseFilename.c_str();

				CFileDialog findDialog(
					TRUE,
					NULL,
					NULL,
					OFN_FILEMUSTEXIST |	OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
					filter,
					AfxGetMainWnd());	

				findDialog.m_ofn.lpstrTitle = _T("Find source file");

				if (findDialog.DoModal() == IDOK)
				{
					CString sel = findDialog.GetPathName().GetString();
					BuildPathRemap(saneFilename.c_str(), sel.GetBuffer());

					remapFilename = ApplyPathRemap(saneFilename.c_str());
				}
			}

			VSInterop::OpenFileAtLine(remapFilename.c_str(), line);
		}
	}
}

void CCodeTreePage::OnReverseClicked(GatherType gt)
{
	CMemReplayView* view = GetView();

	if (view)
	{
		const GenericTreeNode* node = GetSelectedNode();

		if (node)
		{
			std::string name = CreateTabName(GetTitle(), (std::string("Reverse ") + GetNameFor(node, gt)).c_str());
			view->OpenReverseCodeViewFor(name.c_str(), *GetTree(), CollectAddressesFor(node, gt));
		}
	}
}

void CCodeTreePage::OnGatherTDClicked(GatherType gt)
{
	CMemReplayView* view = GetView();

	if (view)
	{
		const GenericTreeNode* node = GetSelectedNode();

		if (node)
		{
			std::string name = CreateTabName(GetTitle(), (std::string("Gather TD ") + GetNameFor(node, gt)).c_str());
			view->OpenGatherTDCodeViewFor(name.c_str(), *GetTree(), CollectAddressesFor(node, gt));
		}
	}
}

void CCodeTreePage::OnGatherBUClicked(GatherType gt)
{
	CMemReplayView* view = GetView();

	if (view)
	{
		const GenericTreeNode* node = GetSelectedNode();

		if (node)
		{
			std::string name = CreateTabName(GetTitle(), (std::string("Gather BU ") + GetNameFor(node, gt)).c_str());
			view->OpenGatherBUCodeViewFor(name.c_str(), *GetTree(), CollectAddressesFor(node, gt));
		}
	}
}

void CCodeTreePage::OnExcludeClicked(GatherType gt)
{
	CMemReplayView* view = GetView();

	if (view)
	{
		const GenericTreeNode* node = GetSelectedNode();

		if (node)
		{
			std::string name = CreateTabName(GetTitle(), (std::string("Exclude ") + GetNameFor(node, gt)).c_str());
			view->OpenExcludeCodeViewFor(name.c_str(), *GetTree(), CollectAddressesFor(node, gt));
		}
	}
}

void CCodeTreePage::OnPlotClicked(GatherType gt)
{
	CMemReplayView* view = GetView();

	if (view)
	{
		const GenericTreeNode* node = GetSelectedNode();

		if (node)
		{
			const char* name;
			const char* filename;
			int line;
			m_symbols->GetSymbol(node->GetKey<TAddress>(), name, filename, line);

			view->AddCodeUsagePlot(name, CollectAddressesFor(node, gt), GetMemoryType());
		}
	}
}

void CCodeTreePage::OnEditCopy()
{
	const GenericTreeNode* node = GetSelectedNode();

	std::string toCopy;
	std::vector<const GenericTreeNode*> unwind;

	for (; node; node = node->parent)
		unwind.push_back(node);

	for (std::vector<const GenericTreeNode*>::reverse_iterator it = unwind.rbegin(), itEnd = unwind.rend(); it != itEnd; ++ it)
	{
		char buf[2048];
		FormatTableItem(buf, *it);
		toCopy += buf;
		toCopy += "\r\n";
	}

	theApp.ClipboardCopy(toCopy.c_str());
}

bool CCodeTreePage::FindMatches(StrStrHandler searchFunc, const char* searchString, const GenericTreeNode* searchNode)
{
	const char* name;
	const char* filename;
	int line;
	m_symbols->GetSymbol(searchNode->GetKey<TAddress>(), name, filename, line);

	if (name && (*searchFunc)(name, searchString))
		return true;

	if (filename)
	{
		char tmp[260];
		sprintf_s(tmp, 260, "%s:%i", filename, line);
		if ((*searchFunc)(tmp, searchString))
			return true;
	}

	return false;
}
	
void CCodeTreePage::ToggleCountScaling()
{
	m_style.ToggleCountScaling();
	m_treeMapWnd.RefreshNodeSizes();
}

bool CCodeTreePage::GetCountScaling()
{
	return m_style.GetCountScaling();
}

void CCodeTreePage::SetMemoryType(int type)
{
	m_currentMemType = type;
	switch(type)
	{
	case MEMTYPE_MAIN:
		m_style.SetMemoryGroup(MemGroups::Main);
		break;
	case MEMTYPE_RSX:
		m_style.SetMemoryGroup(MemGroups::RSX);
		break;
	default:
		m_style.SetMemoryGroup(MemGroups::Count);
		break;
	}
	m_treeMapWnd.RefreshNodeSizes();
}

void CCodeTreePage::FormatTableItem(char* nameBuffer, const GenericTreeNode* item)
{
	GenericTree& tree = *GetTree();
	MakeTableEntry(nameBuffer, *m_symbols, *tree.GetStream<SizeInfoGroups>("Inclusive"), *tree.GetStream<SizeInfoGroups>("Exclusive"), item);
}

void CCodeTreePage::GetColumnDefs(std::vector<ColumnDef>& headers)
{
	ColumnDef col = {0};
	col.hdr.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
	col.hdr.fmt = HDF_LEFT | HDF_STRING;

	headers.clear();
	headers.resize(12, col);

	headers[0].hdr.cxy = 300;
	headers[0].hdr.pszText = "Function";

	headers[1].hdr.cxy = 200;
	headers[1].hdr.pszText = "File/Line (Addr)";

	headers[2].hdr.cxy = 60;
	headers[2].hdr.pszText = "Total Allocs";
	headers[2].sorter = &m_totalCountSorter;

	headers[3].hdr.cxy = 60;
	headers[3].hdr.pszText = "Allocs";
	headers[3].sorter = &m_allocCountSorter;

	headers[4].hdr.cxy = 60;
	headers[4].hdr.pszText = "Frees";
	headers[4].sorter = &m_freeCountSorter;

	headers[5].hdr.cxy = 60;
	headers[5].hdr.pszText = "Total Size";
	headers[5].sorter = &m_inclTotalSorter;

	headers[6].hdr.cxy = 60;
	headers[6].hdr.pszText = "Main Size";
	headers[6].sorter = &m_inclMainSorter;

	headers[7].hdr.cxy = 60;
	headers[7].hdr.pszText = "RSX Size";
	headers[7].sorter = &m_inclRSXSorter;

	headers[8].hdr.cxy = 60;
	headers[8].hdr.pszText = "Excl. Total Size";
	headers[8].sorter = &m_exclTotalSorter;

	headers[9].hdr.cxy = 60;
	headers[9].hdr.pszText = "Excl. Main Size";
	headers[9].sorter = &m_exclMainSorter;

	headers[10].hdr.cxy = 60;
	headers[10].hdr.pszText = "Excl. RSX Size";
	headers[10].sorter = &m_exclRSXSorter;

	headers[11].hdr.cxy = 50;
	headers[11].hdr.pszText = "Efficiency";
}

std::vector<TAddress> CCodeTreePage::CollectAddressesFor(const GenericTreeNode* node, GatherType gt)
{
	std::vector<TAddress> addresses;

	switch (gt)
	{
	case GT_Address:
		addresses.push_back(node->GetKey<TAddress>());
		break;

	case GT_Function:
		m_symbols->FindFunctionMatchingAddresses(addresses, node->GetKey<TAddress>());
		break;

	case GT_Line:
		m_symbols->FindFileLineMatchingAddresses(addresses, node->GetKey<TAddress>());
		break;

	case GT_File:
		m_symbols->FindFileMatchingAddresses(addresses, node->GetKey<TAddress>());
		break;
	}

	return addresses;
}

std::string CCodeTreePage::GetNameFor(const GenericTreeNode* node, GatherType gt)
{
	const char* file;
	const char* name;
	int line;
	m_symbols->GetSymbol(node->GetKey<TAddress>(), name, file, line);

	char saneName[256];
	SanitizeName(saneName, 256, name);

	char res[256];

	switch (gt)
	{
	case GT_Address:
		sprintf_s(res, 256, "%s %08x", saneName, node->GetKey<TAddress>());
		break;

	case GT_Function:
		strcpy(res, saneName);
		break;

	case GT_Line:
		sprintf_s(res, 256, "%s:%i", file, line);
		break;

	case GT_File:
		strcpy(res, file);
		break;
	}

	return res;
}

void CCodeTreePage::BuildPathRemap(const char* from, const char* to)
{
	typedef std::vector<std::pair<std::string, ptrdiff_t> > SplitVec;

	SplitVec fromSplit, toSplit;
	SplitPath(fromSplit, from);
	SplitPath(toSplit, to);

	SplitVec::reverse_iterator fromIt = fromSplit.rbegin(), toIt = toSplit.rbegin(), fromItEnd = fromSplit.rend(), toItEnd = toSplit.rend();
	while ((fromIt != fromItEnd) && (toIt != toItEnd) && (fromIt->first == toIt->first))
	{
		++ fromIt;
		++ toIt;
	}

	if ((fromIt == fromItEnd) || (toIt == toItEnd))
	{
		// No remap could be made
		m_remapFrom = 0;
		m_remapTo = std::string();
		return;
	}
	
	m_remapFrom = fromIt->second + fromIt->first.length();
	m_remapTo = std::string(to, to + toIt->second + toIt->first.length());
}

std::string CCodeTreePage::ApplyPathRemap(const char* from)
{
	std::string res = from;

	if (!m_remapTo.empty() && (res.length() > m_remapFrom))
		res.replace(res.begin(), res.begin() + m_remapFrom, m_remapTo);

	return res;
}
