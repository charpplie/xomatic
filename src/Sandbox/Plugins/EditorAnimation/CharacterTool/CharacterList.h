#pragma once

#include <QtCore/QObject>
#include "Strings.h"
#include "Pointers.h"
#include <Cry_Math.h>

#include <memory>
#include <vector>
#include <map>
#include "../Shared/AnimSettings.h"
#include <IFileChangeMonitor.h>
#include "SkeletonParameters.h"
#include "CharacterDefinition.h"
#include "EntryList.h"
#include "ExplorerDataProvider.h"
#include "ExplorerFileList.h"

struct IAnimationSet;

namespace CharacterTool {
using std::unique_ptr;
using std::vector;
using std::map;

struct ActionOutput;
struct ExplorerAction;

struct CharacterContent
{
	enum EngineLoadState
	{
		CHARACTER_NOT_LOADED,
		CHARACTER_LOADED,
		CHARACTER_INCOMPLETE,
		CHARACTER_LOAD_FAILED
	} engineLoadState;

	CharacterDefinition cdf;
	bool hasDefinitionFile;

	CharacterContent() : hasDefinitionFile(true), engineLoadState(CHARACTER_NOT_LOADED) {}

	void GetDependencies(vector<string>* paths) const;
	void Serialize(Serialization::IArchive& ar);
};

struct CDFDependencies : IEntryDependencyExtractor
{
	void Extract(vector<string>* paths, const EntryBase* entry) override;
};

struct CHRParamsLoader : IEntryLoader
{
	bool Load(EntryBase* entry, const char* filename, LoaderContext* context) override;
	bool Save(EntryBase* entry, const char* filename, LoaderContext* context) override;
};

struct CDFLoader : IEntryLoader
{
	bool Load(EntryBase* entry, const char* filename, LoaderContext* context) override;
	bool Save(EntryBase* entry, const char* filename, LoaderContext* context) override;
};

struct CGALoader : IEntryLoader
{
	bool Load(EntryBase* entry, const char* filename, LoaderContext* context) override;
	bool Save(EntryBase* entry, const char* filename, LoaderContext* context) override;
};

struct CHRParamsDependencies : IEntryDependencyExtractor
{
	void Extract(vector<string>* paths, const EntryBase* entry) override;
};

}
