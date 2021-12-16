#include "stdafx.h"
#include "ReplayDoc.h"

#include "ReplayLogProgressTask.h"
#include "SymbolTableBuilder.h"
#include "ContextTreeFunctors.h"

static const char* s_overviewExportColumns[] = {
	"Item"
};

static const char* s_textureExportColumns[] = {
	"Texture"
};

static const char* s_renderMeshExportColumns[] = {
	"Name",
	"Type"
};

static const char* s_animationExportColumns[] = {
	"Name",
};

static const char* s_navExportColumns[] = {
	"Name"
};

static const char* s_entityExportColumns[] = {
	"Name",
	"Archetype"
};

static const char* s_soundExportColumns[] = {
	"Name"
};


static const char* s_particlesExportColumns[] = {
	"Name"
};

static const char* s_d3dExportColumns[] = {
	"Name"
};

SharedPtr<ReplayDoc> ReplayDoc::FromFile(const char* filename)
{
	const char *ext = strrchr(filename, '.');

	if (!ext)
		return SharedPtr<ReplayDoc>();
	if (strcmp(ext, ".mrl") && strcmp(ext, ".zmrl"))
		return SharedPtr<ReplayDoc>();

	SharedPtr<ReplayLogReader> replayStream = ReplayLogReader::FromFile(filename);
	if (!replayStream.IsValid())
		return SharedPtr<ReplayDoc>();

	SharedPtr<SymbolHelper> symbols = ReadSymbolTable(filename);
	SharedPtr<ContextTree> conTree = ReadContextTree(filename);
	SharedPtr<ReplayValidator> validator = ReadValidatorTable(filename);
	SharedPtr<FrameUsageTracker> fuTracker = ReadFrameUsages(filename);

	bool buildSymbols = symbols.IsValid() == false;
	bool buildConTree = conTree.IsValid() == false;
	bool buildValidation = validator.IsValid() == false;
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

	if (buildValidation)
	{
		validator = new ReplayValidator();
		builders.AddListener(*validator);
	}

	/*
	m_memMap = new MemoryMapReplay();
	builders.AddListener(*m_memMap);
	*/

	/*
	std::basic_string<TCHAR> ptFilenameName(lpszPathName);
	size_t extension = ptFilenameName.rfind(_T('.'));
	if (extension != ptFilenameName.npos)
	ptFilenameName.erase(extension);
	ptFilenameName += _T(".lst.zip");

	m_allocSetLST = new AllocSetLST(ptFilenameName.c_str());

	bool needsToBuildPT = !m_allocSetLST->Restore();

	if (needsToBuildPT)
	{
	builders.AddListener(*m_allocSetLST);
	}
	*/

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

	if (buildValidation)
	{
		std::string fn = std::string(filename) + ".val";
		FileSerialiser ser(fn.c_str());
		if (ser.IsOpen())
			validator->Serialise(ser);
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

	SharedPtr<ReplayDoc> doc = new ReplayDoc();
	doc->m_replayStream = replayStream;
	doc->m_symbols = symbols;
	doc->m_frameUsage = fuTracker;
	doc->m_assetTree = conTree;
	doc->m_validation = validator;

	doc->BuildCommonContextTrees();

	return doc;
}

ReplayDoc::ReplayDoc()
	: m_overviewExporter(1, 2, s_overviewExportColumns, false)
	, m_textureExporter(1, 2, s_textureExportColumns, false)
	, m_renderMeshExporter(1, 3, s_renderMeshExportColumns, false)
	, m_animationExporter(1, 2, s_animationExportColumns, false)
	, m_navExporter(2, 3, s_navExportColumns, false)
	, m_entityExporter(1, 3, s_entityExportColumns, true)
	, m_soundExporter(1, 2, s_soundExportColumns, false)
{
	memset( m_TypeExporter,0,sizeof(m_TypeExporter) );
	
	m_TypeExporter[MemStatContextTypes::MSC_D3D] = new RangedContextExporter(1, 2, s_d3dExportColumns, false);
	m_TypeExporter[MemStatContextTypes::MSC_ParticleLibrary] = new RangedContextExporter(1, 2, s_particlesExportColumns, false);
}

ReplayDoc::~ReplayDoc()
{
	for (int i = 0; i < MemStatContextTypes::MSC_Last; i++)
	{
		delete m_TypeExporter[i];
	}
}

void ReplayDoc::BuildCommonContextTrees()
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


	GatherSubTreesBottomUp( MemStatContextTypes::MSC_ParticleEffect );
	GatherSubTreesBottomUp( MemStatContextTypes::MSC_D3D );
	GatherSubTreesBottomUp( MemStatContextTypes::MSC_SoundBuffer );
	GatherSubTreesBottomUp( MemStatContextTypes::MSC_FSB );

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

		AddTypeToContextTreeNode( overRoot,MemStatContextTypes::MSC_ParticleEffect );
		AddTypeToContextTreeNode( overRoot,MemStatContextTypes::MSC_D3D );
		AddTypeToContextTreeNode( overRoot,MemStatContextTypes::MSC_SoundBuffer );
		AddTypeToContextTreeNode( overRoot,MemStatContextTypes::MSC_FSB );

		{
			ContextTreeNode* ovOther = new ContextTreeNode("Other", MemStatContextTypes::MSC_Other);
			ovOther->GetSize() += assetTree->GetRoot()->GetSize();

			for (ContextTreeNode* ovChild = overRoot->GetChildren(); ovChild; ovChild = ovChild->GetNextSibling())
				ovOther->GetSize() -= ovChild->GetSize();

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
					freeSz.AddToGroup(MemGroups::RSX, SizeInfo(0, 256 * 1024 * 1024, 256 * 1024 * 1024, 256 * 1024 * 1024) - *it);
				}
			}

			{
				FrameUsageTracker::ConstIterator it = m_frameUsage->AllocEvInfoEnd(MemGroups::Main);
				std::advance(it, -1);

				SizeInfo mainMax = hasRSX 
					? SizeInfo(0, 212 * 1024 * 1024, 212 * 1024 * 1024, 212 * 1024 * 1024)
					: SizeInfo(0, 512 * 1024 * 1024, 512 * 1024 * 1024, 512 * 1024 * 1024);

				freeSz.AddToGroup(MemGroups::Main, mainMax - *it);
			}

			ContextTreeNode* ovFree = new ContextTreeNode("Free", MemStatContextTypes::MSC_Other);
			ovFree->GetSize() += freeSz;
			overRoot->AddChild(ovFree);
		}

		overviewTree->EndEdit();
		m_overviewTree = overviewTree;
	}
}


SharedPtr<SymbolHelper> ReplayDoc::ReadSymbolTable(const char* filename)
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

SharedPtr<ContextTree> ReplayDoc::ReadContextTree(const char* filename)
{
	std::string fn = std::string(filename) + ".contree";

	FileDeserialiser ser(fn.c_str());
	if (ser.IsOpen())
		return ContextTree::Deserialise(ser);
	
	return SharedPtr<ContextTree>();
}

SharedPtr<ReplayValidator> ReplayDoc::ReadValidatorTable(const char* filename)
{
	std::string fn = std::string(filename) + ".val";

	FileDeserialiser ser(fn.c_str());
	if (ser.IsOpen())
	{
		SharedPtr<ReplayValidator> val = new ReplayValidator();

		if (val->Deserialise(ser))
			return val;
	}

	return SharedPtr<ReplayValidator>();
}

SharedPtr<FrameUsageTracker> ReplayDoc::ReadFrameUsages(const char* filename)
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

//////////////////////////////////////////////////////////////////////////
void ReplayDoc::GatherSubTreesBottomUp( MemStatContextTypes::Type type )
{
	using namespace ContextTreeFunctor;
	m_TypeTree[type] = ContextTree::GatherSubTreesBottomUp(*m_assetTree, MemStatContextTypes::ToString(type), TypeFilter(type));
	m_TypeTree[type] = ContextTree::GroupChildren(*m_TypeTree[type], NameGrouper());
}

//////////////////////////////////////////////////////////////////////////
void ReplayDoc::AddTypeToContextTreeNode( ContextTreeNode *root,MemStatContextTypes::Type type )
{
	if ( m_TypeTree[type]->GetRoot() )
	{
		ContextTreeNode* ov = new ContextTreeNode(MemStatContextTypes::ToString(type), type);
		ov->GetSize() += m_TypeTree[type]->GetRoot()->GetSize();
		root->AddChild(ov);
	}
}