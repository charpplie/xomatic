#pragma once

#include <QtCore/QObject>
#include "Strings.h"
#include "Pointers.h"

#include <memory>
#include <vector>
#include <map>
#include "../Shared/AnimSettings.h"
#include <IFileChangeMonitor.h>
#include <ICryAnimation.h>
#include "ExplorerDataProvider.h"
#include "EntryList.h"
#include "AnimationContent.h"
#include "SkeletonParameters.h"

struct IAnimationSet;
struct ICharacterInstance;

namespace Serialization
{
	struct SStruct;
};

namespace CharacterTool
{
struct System;
using std::unique_ptr;
using std::vector;
using std::map;

class Explorer;
struct ActionOutput;

class AnimationList : public IExplorerEntryProvider, public IFileChangeListener, public IAnimationSetListener
{
	Q_OBJECT
public:
	AnimationList(System* system, int explorerColumnSize, int explorerColumnAudio, int explorerColumnPak);
	~AnimationList();
	void Populate(ICharacterInstance* character, const char* defaultSkeletonAlias, const AnimationSetFilter& filter, const char* animEventsFilename);
	void SetAnimationFilterAndScan(const AnimationSetFilter& filter);
	
	void RemoveImportEntry(const char* animationPath);

	bool IsLoaded(unsigned int id) const;
	bool ImportAnimation(string* errorMessage, unsigned int id);
	SEntry<AnimationContent>* GetEntry(unsigned int id) const;
	bool IsNewAnimation(unsigned int id) const;
	SEntry<AnimationContent>* FindEntryByPath(const char* animationPath);
	unsigned int FindIdByAlias(const char* animationName);
	bool ResaveAnimSettings(const char* filePath);

	void OnAnimationSetAddAnimation(const char* animationPath, const char* animationName) override;
	void OnAnimationSetReload() override;
	const char* CanonicalExtension() const override{ return "caf"; }
	bool SaveAnimationEntry(ActionOutput* output, unsigned int id, bool notifyOfChange);

	// IExplorerDataProvider:
	int GetEntryCount(int subtree) const override;
	unsigned int GetEntryIdByIndex(int subtree, int index) const override;
	bool GetEntrySerializer(Serialization::SStruct* out, unsigned int id) const override;
	void UpdateEntry(ExplorerEntry* entry) override;
	void RevertEntry(unsigned int id) override;
	bool SaveEntry(ActionOutput* output, unsigned int id) override;
	bool SaveAll(ActionOutput* output) override;
	int GetEntryType(int subtreeIndex) const override;
	void CheckIfModified(unsigned int id, const char* reason, bool continuousChange) override;
	void GetEntryActions(vector<ExplorerAction>* actions, unsigned int id, Explorer* explorer) override;
	bool LoadOrGetChangedEntry(unsigned int id) override;
	// ^^^

signals:
	void SignalForceRecompile(const char* filepath);
private:
	void ActionImport(ActionOutput* out, ExplorerEntry* entry);
	void ActionForceRecompile(ActionOutput* out, ExplorerEntry* entry);
	void ActionGenerateFootsteps(ActionOutput* out, ExplorerEntry* entry);
	void ActionExportHTR(ActionOutput* out, ExplorerEntry* entry);
	void ActionComputeVEG(ActionOutput* out, ExplorerEntry* entry);
	void ActionCopyExamplePaths(ActionOutput* out, ExplorerEntry* entry);

	unsigned int MakeNextId();
	void ReloadAnimationList();
	void OnFileChange(const char* filename, EChangeType eType) override;
	bool UpdateImportEntry(SEntry<AnimationContent>* entry);
	void UpdateAnimationEntryByPath(const char* filename);
	void ScanForImportEntries(bool resetFollows);
	void SetEntryState(ExplorerEntry* entry, const vector<char>& state);
	void GetEntryState(vector<char>* state, ExplorerEntry* entry);

	System* m_system;
	IAnimationSet* m_animationSet;
	ICharacterInstance* m_character;
	string m_defaultSkeletonAlias;
	string m_animEventsFilename;

	CEntryList<AnimationContent> m_animations;
	bool m_importEntriesInitialized;
	std::vector<string> m_importEntries;
	typedef std::map<string, unsigned int, stl::less_stricmp<string> > AliasToId;
	AliasToId m_aliasToId;

	AnimationSetFilter m_filter;

	int m_explorerColumnAudio;
	int m_explorerColumnSize;
	int m_explorerColumnPak;

	string m_commonPrefix;
};

}
