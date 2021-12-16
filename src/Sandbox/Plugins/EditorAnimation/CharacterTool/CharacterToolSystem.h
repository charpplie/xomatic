#pragma once

#include <memory>
#include <QObject>

class CAnimationCompressionManager;

namespace Serialization
{
class IArchive;
class CContextList;
struct SContextLink;
struct INavigationProvider;
}

namespace CharacterTool {
using std::unique_ptr;
class AnimationList;
class AnimationTagList;
class CharacterGizmoManager;
class CharacterSpaceProvider;
class CharacterDocument;
class EditorDBATable;
class EditorCompressionPresetTable;
class Explorer;
class ExplorerFileList;
class FilterAnimationList;
class GizmoSink;
class SkeletonList;
struct CharacterDefinition;
struct DisplayParameters;
struct EntryModifiedEvent;
struct ExplorerEntry;
struct LoaderContext;
struct SceneContent;

struct System : public QObject
{
	Q_OBJECT
public:
	bool loaded;
	unique_ptr<SceneContent> scene;
	unique_ptr<CharacterDocument> document;
	int explorerColumnAudio;
	int explorerColumnPak;
	unique_ptr<Explorer> explorer;
	unique_ptr<LoaderContext> loaderContext;
	unique_ptr<CAnimationCompressionManager> animationCompressionManager;
	unique_ptr<CharacterGizmoManager> characterGizmoManager;

	// explorer data providers:
	unique_ptr<AnimationList> animationList;
	unique_ptr<ExplorerFileList> characterList;
	unique_ptr<ExplorerFileList> physicsList;
	unique_ptr<ExplorerFileList> rigList;
	unique_ptr<ExplorerFileList> skeletonList;
	unique_ptr<ExplorerFileList> compressionGlobalList;
	unique_ptr<ExplorerFileList> sourceAssetList;
	EditorDBATable* dbaTable;
	EditorCompressionPresetTable* compressionPresetTable;
	SkeletonList* compressionSkeletonList;
	// ^^^

	// serialization contexts
	unique_ptr<Serialization::CContextList> contextList;
	unique_ptr<GizmoSink> gizmoSink;
	unique_ptr<Serialization::INavigationProvider> explorerNavigationProvider;
	unique_ptr<CharacterSpaceProvider> characterSpaceProvider;
	unique_ptr<FilterAnimationList> filterAnimationList;
	unique_ptr<AnimationTagList> animationTagList;
	// ^^^

	System();
	~System();
	void Serialize(Serialization::IArchive& ar);
	void Initialize();
	void LoadGlobalData();
};

}
