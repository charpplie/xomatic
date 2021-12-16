#pragma once

#include "GenericTree.h"
#include "SizeInfo.h"
#include "SymbolHelper.h"

class CallstackTable;
class ReplayLogReader;

SharedPtr<GenericTree> MemHierFromTypeInfo(const char* typeName, const SharedPtr<SymbolHelper>& symbolTable);

SharedPtr<GenericTree> QueryCode(ReplayLogReader& log, const std::vector<std::pair<u64, u64> >& logOffsetSpans, bool includeAllocs, bool includeFrees, SharedPtr<SymbolHelper> symbols);
SharedPtr<GenericTree> QueryCodeContext(ReplayLogReader& log, const std::vector<ContextStreamOffsetSpan>& logOffsets, bool includeAllocs, bool includeFrees, SharedPtr<SymbolHelper> symbols);

SharedPtr<GenericTree> CreateCodeGenericTree();

typedef std::map<u32, GenericTreeNode*, std::less<u32>, STLPoolAllocator<std::pair<const u32, GenericTreeNode*> > > CallstackNodeMap;
GenericTreeNode* FindNodeForCallstackId(CallstackNodeMap& csnm, size_t id, CallstackTable& csTable, GenericTree& tree, GenericTreeNode* root);

void SortChildren(GenericTreeNode* node);
