#pragma once

#include "Serialization.h"
#include "../Shared/SourceAssetScene.h"
#include "../Shared/SourceAssetSettings.h"
#include "ExplorerFileList.h" // IEntryLoader

namespace CharacterTool
{
struct CreateAssetManifestTask;
struct SourceAssetContent
{
	enum State
	{
		STATE_EMPTY,
		STATE_LOADING,
		STATE_LOADED
	};

	State state;

	SourceAsset::Scene scene;
	SourceAsset::Settings settings;
	bool changingView;

	CreateAssetManifestTask* loadingTask;

	SourceAssetContent()
	: state(STATE_EMPTY)
	, changingView(false)
	, loadingTask()
	{
	}

	void Serialize(IArchive& ar);

};

struct RCAssetLoader : IEntryLoader
{
	bool Load(EntryBase* entry, const char* filename, LoaderContext* context) override;
	bool Save(EntryBase* entry, const char* filename, LoaderContext* context) override { return false; }
};


}
