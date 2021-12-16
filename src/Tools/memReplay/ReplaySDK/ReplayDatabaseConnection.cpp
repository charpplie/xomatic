#include "stdafx.h"
#include "ReplayDatabaseConnection.h"
#include "ReplayQuery.h"

#include "ReplayLogProgressTask.h"
#include "SymbolTableBuilder.h"
#include "ContextTreeFunctors.h"

#include "AllocValidationQuery.h"
#include "CodeTreeContextQuery.h"
#include "CodeTreeOffsetQuery.h"
#include "FreedSpaceQuery.h"
#include "SizerTreeQuery.h"
#include "SizerCodeQuery.h"
#include "CodeUsageQuery.h"
#include "BucketUsageQuery.h"

#include "memHier2.h"

ReplayDatabaseConnection::ReplayDatabaseConnection()
{
}

bool ReplayDatabaseConnection::Open(const char* filename)
{
	const char *ext = strrchr(filename, '.');

	if (!ext)
		return false;
	if (strcmp(ext, ".mrl") && strcmp(ext, ".zmrl"))
		return false;

	SharedPtr<ReplayLogReader> replayStream = ReplayLogReader::FromFile(filename);
	if (!replayStream.IsValid())
		return false;

	SharedPtr<SymbolHelper> symbols = ReadSymbolTable(filename);
	SharedPtr<ContextTree> conTree = ReadContextTree(filename);
	SharedPtr<FrameUsageTracker> fuTracker = ReadFrameUsages(filename);

	bool buildSymbols = symbols.IsValid() == false;
	bool buildConTree = conTree.IsValid() == false;
	bool buildFUTracker = fuTracker.IsValid() == false;

	SharedPtr<SymbolTableBuilder> symbolBuilder;
	SharedPtr<ContextTreeReplayBuilder> contextTreeBuilder;

	if (buildSymbols)
		symbolBuilder = new SymbolTableBuilder(std::string(filename) + ".sym");

	if (buildConTree)
		contextTreeBuilder = ContextTree::CreateBuilder();

	CompositeReplayListener builders;

	if (buildConTree)
		builders.AddListener(*contextTreeBuilder);

	if (buildSymbols)
		builders.AddListener(*symbolBuilder);

	if (buildFUTracker)
	{
		fuTracker = new FrameUsageTracker();
		builders.AddListener(*fuTracker);
	}

	if (builders.ListenerCount() > 0)
	{
		ReplayLogProgressTask task(*replayStream, builders);
		IInterfaceHooks::RunProgressTask(task);
	}

	if (buildConTree)
	{
		conTree = contextTreeBuilder->GetBuiltTree();

		std::string fn = std::string(filename) + ".contree";
		FileSerialiser ser(fn.c_str());
		if (ser.IsOpen())
			conTree->Serialise(ser);
	}

	if (symbolBuilder.IsValid())
		symbols = symbolBuilder->GetSymbols();

	if (buildFUTracker)
	{
		std::string fn = std::string(filename) + ".fu";
		FileSerialiser ser(fn.c_str());
		if (ser.IsOpen())
			fuTracker->Serialise(ser);
	}

	m_taskProcessor = new ReplayTaskProcessor(replayStream);
	m_replayStream = replayStream;
	m_symbols = symbols;
	m_frameUsage = fuTracker;
	m_assetTree = conTree;

	BuildCommonContextTrees();

	return true;
}

ReplayQueryFuture<SharedPtr<GenericTree> > ReplayDatabaseConnection::BeginQueryCodeTree(const std::vector<ContextStreamOffsetSpan>& offsets, CodeTreeFilter ctf)
{
	SharedPtr<CodeTreeContextQuery> query = new CodeTreeContextQuery(
		offsets,
		(ctf & CTF_Allocations) != 0,
		(ctf & CTF_Frees) != 0,
		m_symbols);
	m_taskProcessor->QueueTask(query);
	return query;
}

ReplayQueryFuture<SharedPtr<GenericTree> > ReplayDatabaseConnection::BeginQueryCodeTree(
	const std::vector<std::pair<u64, u64> >& offsets,
	CodeTreeFilter ctf,
	const std::vector<int>& limitToBuckets)
{
	SharedPtr<CodeTreeOffsetQuery> query = new CodeTreeOffsetQuery(
		offsets,
		(ctf & CTF_Allocations) != 0,
		(ctf & CTF_Frees) != 0,
		limitToBuckets,
		m_symbols);
	m_taskProcessor->QueueTask(query);
	return query;
}

ReplayQueryFuture<SharedPtr<SizeInfoGroupVector> > ReplayDatabaseConnection::BeginQuerySizeInfoPerFrame()
{
	typedef SharedPtr<SizeInfoGroupVector> Result;

	Result result = new SizeInfoGroupVector();

	size_t count = m_frameUsage->FrameInfoEnd(MemGroups::Main) - m_frameUsage->FrameInfoBegin(MemGroups::Main);
	result->resize(count);

	for (size_t idx = 0; idx != count; ++ idx)
	{
		(*result)[idx].AddToGroup(MemGroups::Main, m_frameUsage->FrameInfoBegin(MemGroups::Main)[idx]);
		(*result)[idx].AddToGroup(MemGroups::RSX, m_frameUsage->FrameInfoBegin(MemGroups::RSX)[idx]);
	}

	return new ReplayImmediateFuture<Result>(result);
}

ReplayQueryFuture<SharedPtr<SizeInfoGroupVector> > ReplayDatabaseConnection::BeginQuerySizeInfoPerAlloc()
{
	typedef SharedPtr<SizeInfoGroupVector> Result;

	Result result = new SizeInfoGroupVector();

	size_t count = m_frameUsage->AllocEvInfoEnd(MemGroups::Main) - m_frameUsage->AllocEvInfoBegin(MemGroups::Main);
	result->resize(count);

	for (size_t idx = 0; idx != count; ++ idx)
	{
		(*result)[idx].AddToGroup(MemGroups::Main, m_frameUsage->AllocEvInfoBegin(MemGroups::Main)[idx]);
		(*result)[idx].AddToGroup(MemGroups::RSX, m_frameUsage->AllocEvInfoBegin(MemGroups::RSX)[idx]);
	}

	return new ReplayImmediateFuture<Result>(result);
}

ReplayQueryFuture<SharedPtr<BucketUsageResult> > ReplayDatabaseConnection::BeginQueryBucketUsage()
{
	SharedPtr<BucketUsageQuery> query = new BucketUsageQuery();
	m_taskProcessor->QueueTask(query);
	return query;
}

ReplayQueryFuture<SharedPtr<SizeInfoUsageResult> > ReplayDatabaseConnection::BeginQueryCodeUsage(const std::vector<TAddress>& addresses)
{
	SharedPtr<CodeUsageQuery> query = new CodeUsageQuery(addresses);
	m_taskProcessor->QueueTask(query);
	return query;
}

ReplayQueryFuture<SharedPtr<ValidateResult> > ReplayDatabaseConnection::BeginQueryValidation()
{
	SharedPtr<AllocValidationQuery> query = new AllocValidationQuery();
	m_taskProcessor->QueueTask(query);
	return query;
}

ReplayQueryFuture<SharedPtr<GenericTree> > ReplayDatabaseConnection::BeginQueryFreedSpace(u64 allocEvEnd, const std::vector<int>& limitToBuckets)
{
	SharedPtr<FreedSpaceQuery> query = new FreedSpaceQuery(allocEvEnd, limitToBuckets);
	m_taskProcessor->QueueTask(query);
	return query;
}

ReplayQueryFuture<SharedPtr<GenericTree> > ReplayDatabaseConnection::BeginQueryTypeStructure(const char* typeName)
{
	typedef SharedPtr<GenericTree> Result;

	Result typeTree = MemHierFromTypeInfo(typeName, m_symbols);
	return new ReplayImmediateFuture<Result>(typeTree);
}

ReplayQueryFuture<SharedPtr<GenericTree> > ReplayDatabaseConnection::BeginQuerySizerTree()
{
	SharedPtr<SizerTreeQuery> query = new SizerTreeQuery(false);
	m_taskProcessor->QueueTask(query);
	return query;
}

ReplayQueryFuture<SharedPtr<GenericTree> > ReplayDatabaseConnection::BeginQuerySizerAddObjectOverrunTree()
{
	SharedPtr<SizerTreeQuery> query = new SizerTreeQuery(true);
	m_taskProcessor->QueueTask(query);
	return query;
}

ReplayQueryFuture<SharedPtr<GenericTree> > ReplayDatabaseConnection::BeginQueryCodeTreeFromSizer(size_t treeIdx, const std::vector<const char*>& namesMatchAgainst)
{
	SharedPtr<SizerCodeQuery> query = new SizerCodeQuery(treeIdx, namesMatchAgainst);
	m_taskProcessor->QueueTask(query);
	return query;
}

SharedPtr<ISymbolTable> ReplayDatabaseConnection::GetSymbolTable() const
{
	return m_symbols;
}

SharedPtr<FrameUsageTracker> ReplayDatabaseConnection::GetFrameUsageTracker()
{
	return m_frameUsage;
}

SharedPtr<ContextTree> ReplayDatabaseConnection::GetRootContextTree() const 
{
	return m_assetTree;
}

SharedPtr<ContextTree> ReplayDatabaseConnection::GetCgfPhysicsTree()
{
	return m_cgfPhysTree;
}

SharedPtr<ContextTree> ReplayDatabaseConnection::GetTextureTree()
{
	return m_textureTree;
}

SharedPtr<ContextTree> ReplayDatabaseConnection::GetRenderMeshTree()
{
	return m_renderMeshTree;
}

SharedPtr<ContextTree> ReplayDatabaseConnection::GetTerrainTree()
{
	return m_terrainTree;
}

SharedPtr<ContextTree> ReplayDatabaseConnection::GetAnimationTree()
{
	return m_animationTree;
}

SharedPtr<ContextTree> ReplayDatabaseConnection::GetNavTree()
{
	return m_navTree;
}

SharedPtr<ContextTree> ReplayDatabaseConnection::GetEntityTree()
{
	return m_entityTree;
}

SharedPtr<ContextTree> ReplayDatabaseConnection::GetSoundProjectTree()
{
	return m_soundTree;
}

SharedPtr<ContextTree> ReplayDatabaseConnection::GetOverviewTree()
{
	return m_overviewTree;
}

void ReplayDatabaseConnection::BuildCommonContextTrees()
{
	using namespace ContextTreeFunctor;

	if (!m_assetTree.IsValid())
		return;

	SharedPtr<ContextTree> assetTree = m_assetTree;

	SharedPtr<ContextTree> physicsTree = ContextTree::GatherSubTreesBottomUp(*assetTree, "CGF Physics", TypeFilter(MemStatContextTypes::MSC_MAX));
	physicsTree = ContextTree::FilterLeaves(*physicsTree, PhysicsLeafFilter() && EmptyLeafFilter());
	physicsTree = ContextTree::FoldLeaves(*physicsTree, PhysicsLeafFolder());
	physicsTree = ContextTree::GroupChildren(*physicsTree, NameGrouper());
	m_cgfPhysTree = physicsTree;

	SharedPtr<ContextTree> textureTree = ContextTree::GatherSubTrees(*assetTree, "Textures", TypeFilter(MemStatContextTypes::MSC_Texture));
	textureTree = ContextTree::GroupChildren(*textureTree, NameGrouper());
	m_textureTree = textureTree;

	SharedPtr<ContextTree> renderMeshTree = ContextTree::GatherSubTreesBottomUp(*assetTree, "Render Mesh", TypeFilter(MemStatContextTypes::MSC_RenderMesh));
	renderMeshTree = ContextTree::GroupChildren(*renderMeshTree, NameGrouper());
	renderMeshTree = ContextTree::FoldLeaves(*renderMeshTree, MiscLeafFolder());
	renderMeshTree = ContextTree::FilterLeaves(*renderMeshTree, EmptyLeafFilter());
	m_renderMeshTree = renderMeshTree;

	SharedPtr<ContextTree> terrainTree = ContextTree::GatherSubTreesBottomUp(*assetTree, "Terrain", 
		TypeFilter(MemStatContextTypes::MSC_Terrain));
	terrainTree = ContextTree::Filter(*terrainTree,
		!(
		TypeFilter(MemStatContextTypes::MSC_MAX) ||
		TypeFilter(MemStatContextTypes::MSC_CGF) ||
		TypeFilter(MemStatContextTypes::MSC_MTL) ||
		TypeFilter(MemStatContextTypes::MSC_DBA) ||
		TypeFilter(MemStatContextTypes::MSC_CHR) ||
		TypeFilter(MemStatContextTypes::MSC_CGA) ||
		TypeFilter(MemStatContextTypes::MSC_LMG) ||
		TypeFilter(MemStatContextTypes::MSC_AG) ||
		TypeFilter(MemStatContextTypes::MSC_Texture) ||
		TypeFilter(MemStatContextTypes::MSC_RenderMesh) ||
		TypeFilter(MemStatContextTypes::MSC_RenderMeshType) ||
		TypeFilter(MemStatContextTypes::MSC_ParticleLibrary) ||
		TypeFilter(MemStatContextTypes::MSC_CDF)));
	terrainTree = ContextTree::GroupChildren(*terrainTree, NameGrouper());
	m_terrainTree = terrainTree;

	SharedPtr<ContextTree> animationTree = ContextTree::GatherSubTrees(
		*assetTree, "Animation", 
		TypeFilter(MemStatContextTypes::MSC_DBA) ||
		TypeFilter(MemStatContextTypes::MSC_ANM) ||
		TypeFilter(MemStatContextTypes::MSC_CAF) ||
		TypeFilter(MemStatContextTypes::MSC_LMG) ||
		TypeFilter(MemStatContextTypes::MSC_AG));
	animationTree = ContextTree::GroupChildren(*animationTree, NameGrouper());
	m_animationTree = animationTree;

	SharedPtr<ContextTree> navTree = ContextTree::GatherSubTrees(*assetTree, "Navigation", TypeFilter(MemStatContextTypes::MSC_Navigation));
	navTree = ContextTree::GroupChildren(*navTree, NameGrouper());
	m_navTree = navTree;

	SharedPtr<ContextTree> entTree = ContextTree::GatherSubTrees(*assetTree, "Entity", TypeFilter(MemStatContextTypes::MSC_Entity));
	entTree = ContextTree::Filter(*entTree,
		TypeFilter(MemStatContextTypes::MSC_Entity) ||
		TypeFilter(MemStatContextTypes::MSC_Other) ||
		TypeFilter(MemStatContextTypes::MSC_ScriptCall));
	entTree = ContextTree::FoldLeaves(*entTree, NotEntityLeafFolder());

	{
		SharedPtr<ContextTree> archetypeLibTree = ContextTree::GatherSubTreesBottomUp(*assetTree, "Archetype Libs", TypeFilter(MemStatContextTypes::MSC_ArchetypeLib));
		entTree = ContextTree::MergeChildren(*entTree, *archetypeLibTree);
	}

	m_entityTree = entTree;

	SharedPtr<ContextTree> soundProjectTree = ContextTree::GatherSubTreesBottomUp(*assetTree, "Sound Projects", TypeFilter(MemStatContextTypes::MSC_SoundProject));
	soundProjectTree = ContextTree::GroupChildren(*soundProjectTree, NameGrouper());
	m_soundTree = soundProjectTree;

	SharedPtr<ContextTree> overviewTree = new ContextTree();

	{
		ContextTreeNode* overRoot = overviewTree->BeginEdit();
		overRoot->Rename("Overview");

		ContextTreeNode* ovCgfPhys = new ContextTreeNode("CGF Physics", MemStatContextTypes::MSC_Physics);
		if ( physicsTree->GetRoot() )
		{
			ovCgfPhys->GetSize() += physicsTree->GetRoot()->GetSize();
			overRoot->AddChild(ovCgfPhys);
		}

		ContextTreeNode* ovTextures = new ContextTreeNode("Textures", MemStatContextTypes::MSC_Texture);
		if ( textureTree->GetRoot() )
		{
			ovTextures->GetSize() += textureTree->GetRoot()->GetSize();
			overRoot->AddChild(ovTextures);
		}

		ContextTreeNode* ovRenderMesh = new ContextTreeNode("Render Mesh", MemStatContextTypes::MSC_RenderMesh);
		if ( renderMeshTree->GetRoot() )
		{
			ovRenderMesh->GetSize() += renderMeshTree->GetRoot()->GetSize();
			overRoot->AddChild(ovRenderMesh);
		}

		ContextTreeNode* ovAnimation = new ContextTreeNode("Animation", MemStatContextTypes::MSC_AG);
		if ( animationTree->GetRoot() )
		{
			ovAnimation->GetSize() += animationTree->GetRoot()->GetSize();
			overRoot->AddChild(ovAnimation);
		}

		ContextTreeNode* ovNavigation = new ContextTreeNode("Navigation", MemStatContextTypes::MSC_Navigation);
		if ( navTree->GetRoot() )
		{
			ovNavigation->GetSize() += navTree->GetRoot()->GetSize();
			overRoot->AddChild(ovNavigation);
		}

		ContextTreeNode* ovEntity = new ContextTreeNode("Entity", MemStatContextTypes::MSC_Entity);
		if ( entTree->GetRoot() )
		{
			ovEntity->GetSize() += entTree->GetRoot()->GetSize();
			overRoot->AddChild(ovEntity);
		}

		ContextTreeNode* ovSoundProjects = new ContextTreeNode("Sound Projects", MemStatContextTypes::MSC_SoundProject);
		if ( soundProjectTree->GetRoot() )
		{
			ovSoundProjects->GetSize() += soundProjectTree->GetRoot()->GetSize();
			overRoot->AddChild(ovSoundProjects);
		}

		{
			ContextTreeNode* ovOther = new ContextTreeNode("Other", MemStatContextTypes::MSC_Other);
			ovOther->GetSize() += assetTree->GetRoot()->GetSize();

			SizeInfoGroups sz = ovOther->GetSize();

			for (ContextTreeNode* ovChild = overRoot->GetChildren(); ovChild; ovChild = ovChild->GetNextSibling())
			{
				for (int mg = 0; mg < MemGroups::Count; ++ mg)
				{
					const SizeInfo& mgsz = ovChild->GetSize().GetGroup(static_cast<MemGroups::Group>(mg));
					sz.AddToGroup(static_cast<MemGroups::Group>(mg), SizeInfo(-mgsz.allocCount, -mgsz.freeCount, -mgsz.requested, -mgsz.consumed, -mgsz.global));
				}
			}

			ovOther->GetSize() = sz;

			overRoot->AddChild(ovOther);
		}

		{
			SizeInfoGroups freeSz;

			bool hasRSX = false;

			{
				FrameUsageTracker::ConstIterator it = m_frameUsage->AllocEvInfoEnd(MemGroups::RSX);
				std::advance(it, -1);
				if (it->requested)
				{
					hasRSX = true;
					freeSz.AddToGroup(MemGroups::RSX, SizeInfo(0, 0, 256 * 1024 * 1024 - it->requested, 256 * 1024 * 1024 - it->consumed, 256 * 1024 * 1024 - it->global));
				}
			}

			SizeInfoGroups fragmentationSize;

			{
				FrameUsageTracker::ConstIterator it = m_frameUsage->AllocEvInfoEnd(MemGroups::Main);
				std::advance(it, -1);

				SizeInfo mainMax = hasRSX 
					? SizeInfo(0, 0, 209 * 1024 * 1024, 209 * 1024 * 1024, 209 * 1024 * 1024)
					: SizeInfo(0, 0, 512 * 1024 * 1024, 512 * 1024 * 1024, 512 * 1024 * 1024);

				freeSz.AddToGroup(MemGroups::Main, SizeInfo(0, 0, mainMax.global - it->global, mainMax.global - it->global, mainMax.global - it->global));
				fragmentationSize.AddToGroup(MemGroups::Main, SizeInfo(0, 0, it->global - it->consumed, it->global - it->consumed, 0));
			}

			ContextTreeNode* ovFragmentation = new ContextTreeNode("Fragmentation", MemStatContextTypes::MSC_Other);
			ovFragmentation->GetSize() += fragmentationSize;
			overRoot->AddChild(ovFragmentation);

			ContextTreeNode* ovFree = new ContextTreeNode("Free", MemStatContextTypes::MSC_Other);
			ovFree->GetSize() += freeSz;
			overRoot->AddChild(ovFree);
		}

		overviewTree->EndEdit();
		m_overviewTree = overviewTree;
	}
}

SharedPtr<SymbolHelper> ReplayDatabaseConnection::ReadSymbolTable(const char* filename)
{
	std::string symFilename = std::string(filename) + ".sym";
	if (GetFileAttributes(symFilename.c_str()) != INVALID_FILE_ATTRIBUTES)
	{
		SharedPtr<SymbolHelper> symbols = SymbolHelper::FromFile(symFilename.c_str());
		if (symbols.IsValid())
			return symbols;
	}

	return SharedPtr<SymbolHelper>();
}

SharedPtr<ContextTree> ReplayDatabaseConnection::ReadContextTree(const char* filename)
{
	std::string fn = std::string(filename) + ".contree";

	FileDeserialiser ser(fn.c_str());
	if (ser.IsOpen())
		return ContextTree::Deserialise(ser);
	
	return SharedPtr<ContextTree>();
}

SharedPtr<FrameUsageTracker> ReplayDatabaseConnection::ReadFrameUsages(const char* filename)
{
	std::string fn = std::string(filename) + ".fu";

	FileDeserialiser ser(fn.c_str());
	if (ser.IsOpen())
	{
		SharedPtr<FrameUsageTracker> fuTracker = new FrameUsageTracker();
		if (fuTracker->Deserialise(ser))
			return fuTracker;
	}

	return SharedPtr<FrameUsageTracker>();
}
