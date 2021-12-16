#ifndef __SelectionTreeValidator_H__
#define __SelectionTreeValidator_H__

#include "SelectionTreeManager.h"

class CSelectionTreeValidator
{
public:
	CSelectionTreeValidator(CSelectionTreeManager* pManager);

	void Validate();
private:
	enum EErrorType
	{
		eET_Error,
		eET_Warning,
		eET_Comment,
	};

	void ValidateTree(const SSelectionTreeInfo& info);
	void ValidateReference(const SSelectionTreeInfo& info);

	typedef std::list<string> TVariableList;
	struct SNode
	{
		string Name;
		std::vector<SNode> Children;
	};

	void ValidateVarBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, TVariableList& varlist);
	void ValidateSignalBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, const TVariableList& varlist);
	void ValidateTreeBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, XmlNodeRef xmlNode, SNode& node, const TVariableList& varlist);
	void ValidateLeafMappingBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, const SNode& rootnode);

	void ValidateCondition(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, const string& conditionStr, const TVariableList& varlist, const char* nodeName);
	bool ValidateNodePath(const SNode& node, std::vector<string> path, bool firstNodeFound = false);

	void ReportError( const SSelectionTreeInfo &info, const SSelectionTreeBlockInfo& block, const string& err, const string& desc, ESelectionTreeTypeId blockType, EErrorType errType );

private:
	CSelectionTreeManager* m_pManager;

	typedef std::list<CSelectionTreeErrorReport::TSelectionTreeErrorId> TReportedErrors;
	TReportedErrors m_ReportedErrors;
	TReportedErrors m_NewReportedErrors;
};

#endif // __SelectionTreeValidator_H__