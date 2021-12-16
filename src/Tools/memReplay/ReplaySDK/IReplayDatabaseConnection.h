#pragma once

#include "ReplayQueryFuture.h"

#include "ISymbolTable.h"
#include "ContextTree.h"
#include "ValidateError.h"
#include "FrameUsageTracker.h"
#include "GenericTree.h"

class IReplayDatabaseConnection
{
public:
	enum CodeTreeFilter
	{
		CTF_Allocations = 1,
		CTF_Frees = 2
	};

public:
	static SharedPtr<IReplayDatabaseConnection> Open(const char* filename);

public:
	virtual ~IReplayDatabaseConnection() {}

	virtual ReplayQueryFuture<SharedPtr<GenericTree> > BeginQueryCodeTree(const std::vector<ContextStreamOffsetSpan>& offsets, CodeTreeFilter ctf) = 0;
	virtual ReplayQueryFuture<SharedPtr<GenericTree> > BeginQueryCodeTree(const std::vector<std::pair<u64, u64> >& offsets, CodeTreeFilter ctf, const std::vector<int>& limitToBuckets) = 0;

	virtual ReplayQueryFuture<SharedPtr<SizeInfoGroupVector> > BeginQuerySizeInfoPerFrame() = 0;
	virtual ReplayQueryFuture<SharedPtr<SizeInfoGroupVector> > BeginQuerySizeInfoPerAlloc() = 0;

	virtual ReplayQueryFuture<SharedPtr<BucketUsageResult> > BeginQueryBucketUsage() = 0;

	virtual ReplayQueryFuture<SharedPtr<SizeInfoUsageResult> > BeginQueryCodeUsage(const std::vector<TAddress>& addresses) = 0;

	virtual ReplayQueryFuture<SharedPtr<ValidateResult> > BeginQueryValidation() = 0;
	virtual ReplayQueryFuture<SharedPtr<GenericTree> > BeginQueryFreedSpace(u64 allocEvEnd, const std::vector<int>& limitToBuckets) = 0;

	virtual ReplayQueryFuture<SharedPtr<GenericTree> > BeginQueryTypeStructure(const char* typeName) = 0;

	virtual ReplayQueryFuture<SharedPtr<GenericTree> > BeginQuerySizerTree() = 0;
	virtual ReplayQueryFuture<SharedPtr<GenericTree> > BeginQuerySizerAddObjectOverrunTree() = 0;
	virtual ReplayQueryFuture<SharedPtr<GenericTree> > BeginQueryCodeTreeFromSizer(size_t treeIdx, const std::vector<const char*>& namesMatchAgainst) = 0;

	virtual SharedPtr<ISymbolTable> GetSymbolTable() const = 0;

	virtual SharedPtr<FrameUsageTracker> GetFrameUsageTracker() = 0;

	virtual SharedPtr<ContextTree> GetRootContextTree() const = 0;
	virtual SharedPtr<ContextTree> GetCgfPhysicsTree() = 0;
	virtual SharedPtr<ContextTree> GetTextureTree() = 0;
	virtual SharedPtr<ContextTree> GetRenderMeshTree() = 0;
	virtual SharedPtr<ContextTree> GetTerrainTree() = 0;
	virtual SharedPtr<ContextTree> GetAnimationTree() = 0;
	virtual SharedPtr<ContextTree> GetNavTree() = 0;
	virtual SharedPtr<ContextTree> GetEntityTree() = 0;
	virtual SharedPtr<ContextTree> GetSoundProjectTree() = 0;
	virtual SharedPtr<ContextTree> GetOverviewTree() = 0;
};
