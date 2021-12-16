#pragma once

#include <QObject>
#include <vector>
#include "Pointers.h"

namespace Serialization
{
	struct SStruct;
	class IArchive;
};

namespace CharacterTool
{

using std::vector;
struct ExplorerEntry;
struct ExplorerAction;
struct ActionOutput;
class Explorer;

enum EntryPakState
{
	PAK_STATE_LOADING,
	PAK_STATE_LOOSE_FILES = 1 << 0,
	PAK_STATE_PAK = 1 << 1,
	PAK_STATE_PAK_AND_LOOSE_FILES = PAK_STATE_PAK | PAK_STATE_LOOSE_FILES
};

enum EntryAudioState
{
	ENTRY_AUDIO_NONE,
	ENTRY_AUDIO_PRESENT
};

struct ExplorerEntryId
{
	int subtree;
	unsigned int id;

	ExplorerEntryId(int subtree, unsigned int id) : subtree(subtree), id(id)	{}
	ExplorerEntryId() : subtree(-1), id(0)	{ }

	bool operator==(const ExplorerEntryId& rhs) const { return subtree == rhs.subtree && id == rhs.id; }

	void Serialize(Serialization::IArchive& ar);
};

struct EntryModifiedEvent
{
	int subtree;
	unsigned int id;
	const char* reason;
	bool contentChanged;
	bool continuousChange;
	vector<char> previousContent;

	EntryModifiedEvent()
	: subtree(-1)
	, id(0)
	, reason("")
	, contentChanged(false)
	, continuousChange(false)
	{
	}
};

class IExplorerEntryProvider : public QObject
{
	Q_OBJECT
public:
	virtual void UpdateEntry(ExplorerEntry* entry) = 0;
	virtual bool GetEntrySerializer(Serialization::SStruct* out, unsigned int id) const = 0;
	virtual void GetEntryActions(vector<ExplorerAction>* actions, unsigned int id, Explorer* explorer) = 0;

	virtual int GetEntryCount(int subtree) const = 0;
	virtual unsigned int GetEntryIdByIndex(int subtree, int index) const = 0;
	virtual int GetEntryType(int subtree) const = 0;
	virtual bool LoadOrGetChangedEntry(unsigned int id) { return true; }
	virtual void CheckIfModified(unsigned int id, const char* reason, bool continuous) = 0;
	virtual bool HasBackgroundLoading() const{ return false; }

	virtual bool SaveEntry(ActionOutput* output, unsigned int id) = 0;
	virtual void RevertEntry(unsigned int id) = 0;
	virtual bool SaveAll(ActionOutput* output) = 0;
	virtual void SetExplorer(Explorer* explorer) {}
	virtual const char* CanonicalExtension() const { return ""; }
	virtual void GetDependencies(vector<string>* paths, unsigned int id) {}

signals:
	void SignalSubtreeReset(int subtree);
	void SignalEntryModified(EntryModifiedEvent& ev);
	void SignalEntryAdded(int subtree, unsigned int id);
	void SignalEntryRemoved(int subtree, unsigned int id);
	void SignalEntry(int subtree, unsigned int id);
	void SignalSubtreeLoadingFinished(int subtree);

protected:
	int m_subtree;
};

}
