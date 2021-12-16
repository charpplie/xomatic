#ifndef REPLAYDOC_H
#define REPLAYDOC_H

#include "SymbolHelper.h"
#include "ContextTree.h"
#include "ReplayValidator.h"
#include "FrameUsageTracker.h"
#include "DistributionMap.h"
#include "LST.h"

#include "CSVCollator.h"
#include "CGFPhysicsExporter.h"
#include "RangedContextExporter.h"

class ReplayDoc
{
public:
	static SharedPtr<ReplayDoc> FromFile(const char* filename);

public:
	ReplayDoc();
	~ReplayDoc();

	const SharedPtr<ReplayLogReader>& GetReplayStream() const { return m_replayStream; }
	const SharedPtr<SymbolHelper>& GetSymbolTable() const { return m_symbols; }

	const SharedPtr<ContextTree>& GetAssetTree() const { return m_assetTree; }
	const SharedPtr<AllocSetLST>& GetAllocSetLST() const { return m_allocSetLST; }

	const SharedPtr<FrameUsageTracker>& GetFrameUsageTracker() { return m_frameUsage; }

	const SharedPtr<MemoryMapReplay>& GetMemMap() const { return m_memMap; }
	const SharedPtr<ReplayValidator>& GetValidation() const { return m_validation; }

	SharedPtr<ContextTree> GetCgfPhysicsTree() { return m_cgfPhysTree; }
	SharedPtr<ContextTree> GetTextureTree() { return m_textureTree; }
	SharedPtr<ContextTree> GetRenderMeshTree() { return m_renderMeshTree; }
	SharedPtr<ContextTree> GetTerrainTree() { return m_terrainTree; }
	SharedPtr<ContextTree> GetAnimationTree() { return m_animationTree; }
	SharedPtr<ContextTree> GetNavTree() { return m_navTree; }
	SharedPtr<ContextTree> GetEntityTree() { return m_entityTree; }
	SharedPtr<ContextTree> GetSoundProjectTree() { return m_soundTree; }
	SharedPtr<ContextTree> GetOverviewTree() { return m_overviewTree; }
	
	SharedPtr<ContextTree> GetContextTree( MemStatContextTypes::Type type ) { return m_TypeTree[type]; }
	ICSVExporter* GetExporterByType( MemStatContextTypes::Type type ) { return m_TypeExporter[type]; }

	ICSVExporter& GetOverviewExporter() { return m_overviewExporter; }
	ICSVExporter& GetCgfPhysicsExporter() { return m_cgfPhysExporter; }
	ICSVExporter& GetTextureExporter() { return m_textureExporter; }
	ICSVExporter& GetRenderMeshExporter() { return m_renderMeshExporter; }
	ICSVExporter& GetAnimationExporter() { return m_animationExporter; }
	ICSVExporter& GetNavExporter() { return m_navExporter; }
	ICSVExporter& GetEntityExporter() { return m_entityExporter; }
	ICSVExporter& GetSoundProjectExporter() { return m_soundExporter; }

private:
	static SharedPtr<SymbolHelper> ReadSymbolTable(const char* filename);
	static SharedPtr<ContextTree> ReadContextTree(const char* filename);
	static SharedPtr<ReplayValidator> ReadValidatorTable(const char* filename);
	static SharedPtr<FrameUsageTracker> ReadFrameUsages(const char* filename);

private:
	void BuildCommonContextTrees();
	void GatherSubTreesBottomUp( MemStatContextTypes::Type type );
	void AddTypeToContextTreeNode( ContextTreeNode *root,MemStatContextTypes::Type type );

private:
	SharedPtr<ReplayLogReader> m_replayStream;
	SharedPtr<SymbolHelper> m_symbols;

	SharedPtr<FrameUsageTracker> m_frameUsage;

	SharedPtr<MemoryMapReplay> m_memMap;
	SharedPtr<ContextTree> m_assetTree;
	SharedPtr<ReplayValidator> m_validation;

	SharedPtr<AllocSetLST> m_allocSetLST;

private:
	ContextTreeCSVExporter m_csvExporter;

	SharedPtr<ContextTree> m_cgfPhysTree;
	CGFPhysicsCSVExporter m_cgfPhysExporter;

	SharedPtr<ContextTree> m_overviewTree;
	RangedContextExporter m_overviewExporter;

	SharedPtr<ContextTree> m_textureTree;
	RangedContextExporter m_textureExporter;

	SharedPtr<ContextTree> m_renderMeshTree;
	RangedContextExporter m_renderMeshExporter;

	SharedPtr<ContextTree> m_terrainTree;

	SharedPtr<ContextTree> m_animationTree;
	RangedContextExporter m_animationExporter;

	SharedPtr<ContextTree> m_navTree;
	RangedContextExporter m_navExporter;

	SharedPtr<ContextTree> m_entityTree;
	RangedContextExporter m_entityExporter;

	SharedPtr<ContextTree> m_soundTree;
	RangedContextExporter m_soundExporter;

	ContextTreePtr m_TypeTree[MemStatContextTypes::MSC_Last];
	RangedContextExporter* m_TypeExporter[MemStatContextTypes::MSC_Last];
};

#endif
