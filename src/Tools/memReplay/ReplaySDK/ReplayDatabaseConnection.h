#pragma once

#include "IReplayDatabaseConnection.h"

#include "CSVCollator.h"

#include "ReplayTaskProcessor.h"
#include "ReplayQueryFuture.h"
#include "SymbolHelper.h"

class ReplayDatabaseConnection : public IReplayDatabaseConnection
{
public:
	ReplayDatabaseConnection();

	bool Open(const char* filename);

public: // IReplayDatabaseConnection Members
	ReplayQueryFuture<SharedPtr<GenericTree> > BeginQueryCodeTree(const std::vector<ContextStreamOffsetSpan>& offsets, CodeTreeFilter ctf);
	ReplayQueryFuture<SharedPtr<GenericTree> > BeginQueryCodeTree(const std::vector<std::pair<u64, u64> >& offsets, CodeTreeFilter ctf, const std::vector<int>& limitToBuckets);

	ReplayQueryFuture<SharedPtr<SizeInfoGroupVector> > BeginQuerySizeInfoPerFrame();
	ReplayQueryFuture<SharedPtr<SizeInfoGroupVector> > BeginQuerySizeInfoPerAlloc();

	ReplayQueryFuture<SharedPtr<BucketUsageResult> > BeginQueryBucketUsage();

	ReplayQueryFuture<SharedPtr<SizeInfoUsageResult> > BeginQueryCodeUsage(const std::vector<TAddress>& addresses);

	ReplayQueryFuture<SharedPtr<ValidateResult> > BeginQueryValidation();
	ReplayQueryFuture<SharedPtr<GenericTree> > BeginQueryFreedSpace(u64 allocEvEnd, const std::vector<int>& limitToBuckets);

	ReplayQueryFuture<SharedPtr<GenericTree> > BeginQueryTypeStructure(const char* typeName);

	ReplayQueryFuture<SharedPtr<GenericTree> > BeginQuerySizerTree();
	ReplayQueryFuture<SharedPtr<GenericTree> > BeginQuerySizerAddObjectOverrunTree();
	ReplayQueryFuture<SharedPtr<GenericTree> > BeginQueryCodeTreeFromSizer(size_t treeIdx, const std::vector<const char*>& namesMatchAgainst);

	SharedPtr<ISymbolTable> GetSymbolTable() const;

	SharedPtr<FrameUsageTracker> GetFrameUsageTracker();

	SharedPtr<ContextTree> GetRootContextTree() const;
	SharedPtr<ContextTree> GetCgfPhysicsTree();
	SharedPtr<ContextTree> GetTextureTree();
	SharedPtr<ContextTree> GetRenderMeshTree();
	SharedPtr<ContextTree> GetTerrainTree();
	SharedPtr<ContextTree> GetAnimationTree();
	SharedPtr<ContextTree> GetNavTree();
	SharedPtr<ContextTree> GetEntityTree();
	SharedPtr<ContextTree> GetSoundProjectTree();
	SharedPtr<ContextTree> GetOverviewTree();

private:
	static SharedPtr<SymbolHelper> ReadSymbolTable(const char* filename);
	static SharedPtr<ContextTree> ReadContextTree(const char* filename);
	static SharedPtr<FrameUsageTracker> ReadFrameUsages(const char* filename);

private:
	void BuildCommonContextTrees();

private:
	SharedPtr<ReplayLogReader> m_replayStream;
	SharedPtr<SymbolHelper> m_symbols;

	SharedPtr<FrameUsageTracker> m_frameUsage;

	SharedPtr<ContextTree> m_assetTree;

private:
	SharedPtr<ContextTree> m_cgfPhysTree;
	SharedPtr<ContextTree> m_overviewTree;
	SharedPtr<ContextTree> m_textureTree;
	SharedPtr<ContextTree> m_renderMeshTree;
	SharedPtr<ContextTree> m_terrainTree;
	SharedPtr<ContextTree> m_animationTree;
	SharedPtr<ContextTree> m_navTree;
	SharedPtr<ContextTree> m_entityTree;
	SharedPtr<ContextTree> m_soundTree;

	SharedPtr<ReplayTaskProcessor> m_taskProcessor;
};
