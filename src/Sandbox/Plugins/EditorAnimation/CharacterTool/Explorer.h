#pragma once

#include <QtCore/QObject>

#include <vector>
#include <map>
#include <memory>
#include "Strings.h"
#include "Pointers.h"
#include "functor.h"

namespace Serialization { 
class IArchive; 
struct SStruct;
}

struct SDBATable;

namespace CharacterTool {
using std::vector;
using std::map;
using std::unique_ptr;
class AnimationList;
class DependencyManager;
class EditorDBATable;
class EditorCompressionPresetTable;
class SkeletonList;
class CUndoStack;

enum EExplorerSubtree
{
	SUBTREE_CHARACTERS,
	SUBTREE_SKELETONS,
	SUBTREE_PHYSICS,
	SUBTREE_RIGS,
	SUBTREE_ANIMATIONS,
	SUBTREE_COMPRESSION,
	SUBTREE_SOURCE_ASSETS,
	NUM_SUBTREES
};

enum EExplorerEntryType
{
	ENTRY_NONE,
	ENTRY_SUBTREE_ROOT,
	ENTRY_GROUP,
	ENTRY_ANIMATION,
	ENTRY_SKELETON,
	ENTRY_CHARACTER,
	ENTRY_CHARACTER_SKELETON,
	ENTRY_PHYSICS,
	ENTRY_RIG,
	ENTRY_DBA_TABLE,
	ENTRY_COMPRESSION_PRESETS,
	ENTRY_SKELETON_LIST,
	ENTRY_SOURCE_ASSET,
	ENTRY_LOADING
};

struct ExplorerEntry : _i_reference_target_t
{
	EExplorerEntryType type;
	EExplorerSubtree subtree;
	string name;
	string path;
	bool modified;
	unsigned int id;
	const char* icon;

	ExplorerEntry* parent;
	vector<ExplorerEntry*> children;
	unique_ptr<CUndoStack> history;
	vector<unsigned int> columnValues;

	ExplorerEntry(EExplorerSubtree subtree, EExplorerEntryType type, unsigned int id);
	void Serialize(Serialization::IArchive& ar);

	void SetColumnValue(int column, unsigned int value);
	unsigned int GetColumnValue(int column) const;
};
typedef vector<_smart_ptr<ExplorerEntry> > ExplorerEntries;

typedef map<string, ExplorerEntry*, less_stricmp<string> > EntriesByPath;
typedef map<unsigned int, ExplorerEntry*> EntriesById;
struct FolderSubtree
{
	_smart_ptr<ExplorerEntry> root;
	ExplorerEntries entries;
	ExplorerEntries groups;
	ExplorerEntries helpers;
	_smart_ptr<ExplorerEntry> loadingEntry;

	EntriesByPath groupsByPath;
	EntriesByPath entriesByPath;
	EntriesById entriesById;
	string commonPrefix;

	void Clear()
	{
		entries.clear();
		groups.clear();
		groupsByPath.clear();
		entriesByPath.clear();
		entriesById.clear();
		commonPrefix.clear();
	}
};

struct ExplorerColumnValue
{
	const char* tooltip;
	const char* icon;
};

struct ExplorerColumn
{
	enum Format
	{
		TEXT,
		FILESIZE,
		ICON
	};

	string label;
	bool visibleByDefault;
	Format format;
	vector<ExplorerColumnValue> values;

	ExplorerColumn() : visibleByDefault(false), format(TEXT) {}
};


struct EntryModifiedEvent;
struct ExplorerAction;
struct ActionOutput;
typedef vector<ExplorerAction> ExplorerActions;
class IExplorerEntryProvider;
struct ExplorerEntryId;

enum 
{
	ENTRY_PART_STATUS_COLUMNS = 1 << 0,
	ENTRY_PART_CONTENT = 1 << 1
};

struct ExplorerEntryModifyEvent
{
	ExplorerEntry* entry;
	int entryParts;
	bool continuousChange;
	vector<ExplorerEntry*> dependentEntries;

	ExplorerEntryModifyEvent()
	: entry()
	, entryParts()
	, continuousChange(false)
	{
	}
};

// Explorer aggregates available asset information in a uniform tree
class Explorer : public QObject
{
	Q_OBJECT
public:
	Explorer();

	void AddProvider(int subtree, IExplorerEntryProvider* provider);
	int AddColumn(const char* label, ExplorerColumn::Format format, bool visibleByDefault, const ExplorerColumnValue* values = 0, size_t numValues = 0);
	int FindColumn(const char* label) const;

	void Populate();
	ExplorerEntry* GetRoot() { return m_root.get(); }

	int GetColumnCount() const;
	const char* GetColumnLabel(int column) const;
	const ExplorerColumn* GetColumn(int column) const;

	bool GetSerializerForEntry(Serialization::SStruct* out, ExplorerEntry* entry);
	void GetActionsForEntry(ExplorerActions* actions, ExplorerEntry* entry);
	void GetCommonActions(ExplorerActions* actions, const ExplorerEntries& entries);
	void SetEntryColumn(const ExplorerEntryId& id, int column, unsigned int value, bool notify);

	void LoadEntries(const ExplorerEntries& currentEntries);
	void CheckIfModified(ExplorerEntry* entry, const char* reason, bool continuousChange);
	string GetFilePathForEntry(const ExplorerEntry* entry);
	bool IsEntryAvailableLocally(const ExplorerEntry* entry);
	ExplorerEntry* FindEntryById(const ExplorerEntryId& id);
	ExplorerEntry* FindEntryById(EExplorerSubtree subtree, unsigned int id);
	ExplorerEntry* FindEntryByPath(EExplorerSubtree subtree, const char* path);
	void FindEntriesByPath(vector<ExplorerEntry*>* entries, const char* path);
	string GetCanonicalPath(EExplorerSubtree subtree, const char* path) const;
	bool HasProvider(EExplorerSubtree subtree) const;
	void GetDependingEntries(vector<ExplorerEntry*>* entries, ExplorerEntry* entry);

	static const char* IconForEntry(EExplorerEntryType type, const ExplorerEntry* entry);

	void Revert(ExplorerEntry* entry);
	bool SaveEntry(ActionOutput* output, ExplorerEntry*);
	bool SaveAll(ActionOutput* output);
	bool HasAtLeastOneUndoAvailable(const ExplorerEntries& explorerEntries);

	void UndoInOrder(const ExplorerEntries& entries);
	void RedoInOrder(const ExplorerEntries& entries);
	void Undo(const ExplorerEntries& entries, int count);
	bool CanUndo(const ExplorerEntries& entries) const;
	void Redo(const ExplorerEntries& entries);
	bool CanRedo(const ExplorerEntries& entries) const;
	void GetUndoActions(vector<string>* actionNames, int maxActionCount, const ExplorerEntries& entries) const;

	void ActionRevert(ActionOutput* out, ExplorerEntry* entry);
	void ActionSave(ActionOutput* out, ExplorerEntry* entry);
	void ActionShowInExplorer(ActionOutput* out, ExplorerEntry* entry);

signals:
	void SignalEntryModified(ExplorerEntryModifyEvent& ev);
	void SignalEntryLoaded(ExplorerEntry* entry);
	void SignalBeginAddEntry(ExplorerEntry* entry);
	void SignalEndAddEntry();
	void SignalBeginRemoveEntry(ExplorerEntry* entry);
	void SignalEndRemoveEntry();
	void SignalEntryImported(ExplorerEntry* entry, ExplorerEntry* oldEntry);
	void SignalRefreshFilter();
	
protected slots:
	void OnProviderSubtreeReset(int subtree);
	void OnProviderSubtreeLoadingFinished(int subtree);
	void OnProviderEntryModified(EntryModifiedEvent&);
	void OnProviderEntryAdded(int subtree, unsigned int id);
	void OnProviderEntryRemoved(int subtree, unsigned int id);

private:
	void EntryModified(ExplorerEntry* entry, bool continuousChange);
	void ResetSubtree(FolderSubtree* subtree, EExplorerSubtree subtreeIndex, const char* text, bool addLoadingEntry);
	void CreateGroups(ExplorerEntry* globalRoot, FolderSubtree* subtree, EExplorerSubtree subtreeIndex, EExplorerEntryType rootType);
	void UpdateSingleLevelGroups(FolderSubtree* subtree, EExplorerSubtree subtreeIndex);
	void UpdateGroups();
	ExplorerEntry* CreateGroupsForPath(FolderSubtree* subtree, const char* animationPath, const char* commonPathPrefix);
	void RemoveEmptyGroups(FolderSubtree* subtree);
	bool UnlinkEntryFromParent(ExplorerEntry* entry);
	void RemoveEntry(ExplorerEntry* entry);
	void RemoveChildren(ExplorerEntry* entry);
	void LinkEntryToParent(ExplorerEntry* parent, ExplorerEntry* entry);
	void SetEntryState(ExplorerEntry* entry, const vector<char>& state);
	void GetEntryState(vector<char>* state, ExplorerEntry* entry);

	_smart_ptr<ExplorerEntry> m_root;

	FolderSubtree m_subtrees[NUM_SUBTREES];
	unsigned long long m_undoCounter;

	vector<IExplorerEntryProvider*> m_providers;
	vector<IExplorerEntryProvider*> m_providerBySubtree;
	vector<ExplorerColumn> m_columns;

	unique_ptr<DependencyManager> m_dependencyManager;
};

struct ExplorerAction
{
	const char* icon;
	const char* text;
	const char* description;
	bool enabled;
	bool important;
	bool stackable;

	typedef Functor2<ActionOutput*, ExplorerEntry*> Func;
	Func func;

	ExplorerAction()
	: icon("")
	, text("")
	, description("")
	, enabled(false)
	, important(false)
	, stackable(true)
	{
	}

	ExplorerAction(const Func& func, const char* text, bool enabled, const char* icon, bool important, bool stackable = true)
	: icon(icon), text(text), enabled(enabled), important(important), func(func), stackable(stackable), description("")
	{
	}
	ExplorerAction(const Func& func, const char* text, const char* description, bool enabled, const char* icon, bool important, bool stackable)
	: icon(icon), text(text), description(description), enabled(enabled), important(important), func(func), stackable(stackable)
	{
	}
};

struct ActionOutput
{
	typedef vector<string> DetailList;
	typedef map<string, DetailList> ErrorToDetails;
	int errorCount;

	ActionOutput() : errorCount(0) {}

	void AddError(const char* error, const char* details)
	{
		errorToDetails[error].push_back(details);
		++errorCount;
	}

	ErrorToDetails errorToDetails;
};

class ExplorerActionHandler : public QObject
{
	Q_OBJECT
public:
	ExplorerActionHandler(const ExplorerAction& action)
	: m_action(action)
	{
	}
public slots:
	void OnTriggered()
	{
		if (m_action.func)
			SignalAction(m_action);
	}
signals:
	void SignalAction(const ExplorerAction& action);
private:
	ExplorerAction m_action;
};

}
