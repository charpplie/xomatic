// CEntryList is used to store content of setup files in memory.
//
// Its responsibility:
//    Assign unique int identifiers;
//    Track if entry are modified since last save and last change;
//    Index entries by id (int), path and unique name, in some cases;
#pragma once

#include <map>
#include <memory>
#include <vector>
#include "Strings.h"

namespace Serialization
{
class IArchive;
};

namespace CharacterTool
{
using std::map;
using std::vector;
using std::unique_ptr;

typedef unsigned int EntryId;

struct EntryBase
{
	EntryId id;
	string name;
	string path;
	bool created;
	bool loaded;
	bool modified;
	bool dataLostDuringLoading;
	bool failedToLoad;

	vector<char> lastContent;
	vector<char> savedContent;

	EntryBase() 
	: id()
	, loaded(false)
	, modified(false)
	, dataLostDuringLoading(false)
	, failedToLoad(false)
	{
	}

	virtual ~EntryBase() {}
	virtual void Serialize(Serialization::IArchive& ar) = 0;
	void StoreSavedContent();
};

typedef vector<unique_ptr<EntryBase> > SEntries;

class EntryListBase
{
public:
	EntryListBase() : m_nextId(1), m_idProvider(0) {}

	size_t Count() const{ return m_entries.size(); }
	void Clear();
	void SaveAll();
	bool RemoveById(EntryId id);
	unsigned int RemoveByPath(const char* path);

	void SetIdProvider(EntryListBase* list) { m_idProvider = list; }

	virtual EntryBase* AddEntry(bool* isNewEntry, const char* path, const char* name = 0) = 0;

	bool EntryChanged(vector<char>* previousContent, EntryBase* entry) const;
	bool EntryReverted(vector<char>* previousContent, EntryBase* entry) const;
	bool EntryAdded(EntryBase* entry) const;
	bool EntrySaved(EntryBase* entry) const;

	EntryBase* GetBaseById(EntryId id) const;
	EntryBase* GetBaseByName(const char* name) const;
	EntryBase* GetBaseByPath(const char* path) const;
	EntryBase* GetBaseByIndex(size_t index) const;
protected:
	void AddEntry(EntryBase* entry);
	EntryId MakeNextId();
	EntryListBase* m_idProvider;

	typedef map<string, EntryBase*, less_stricmp<string> > EntriesByPath;
	EntriesByPath m_entriesByPath;
	EntriesByPath m_entriesByName;
	typedef map<int, EntryBase*> EntriesById;
	EntriesById m_entriesById;
	SEntries m_entries;
	EntryId m_nextId;
};


template<class T>
struct SEntry : public EntryBase
{
	T content;

	void Serialize(Serialization::IArchive& ar) override;
};

template<class T>
class CEntryList : public EntryListBase
{
public:
	SEntry<T>* AddEntry(bool* isNewEntry, const char* path, const char* name = 0) override
	{
		SEntry<T>* entry = GetByPath(path);
		if (!entry)
		{
			if (isNewEntry)
				*isNewEntry = true;
			entry = new SEntry<T>;
			entry->path = path;
			if (name)
				entry->name = name;
			EntryListBase::AddEntry(entry);			
		}
		else if (isNewEntry)
			*isNewEntry = false;
		return entry;
	}

	bool EntryReverted(vector<char>* previousContent, SEntry<T>* entry) const { return EntryListBase::EntryReverted(previousContent, entry);	}
	bool EntryChanged(vector<char>* previousContent, SEntry<T>* entry) const { return EntryListBase::EntryChanged(previousContent, entry);}
	bool EntrySaved(SEntry<T>* entry)	const { return EntryListBase::EntrySaved(entry); }
	SEntry<T>* GetById(EntryId id) const { return static_cast<SEntry<T>*>(GetBaseById(id));	}
	SEntry<T>* GetByIndex(size_t index) const { return static_cast<SEntry<T>*>(GetBaseByIndex(index));	}
	SEntry<T>* GetByName(const char* name) const { return static_cast<SEntry<T>*>(GetBaseByName(name));	}
	SEntry<T>* GetByPath(const char* path) const { return static_cast<SEntry<T>*>(GetBaseByPath(path));	}
};

}
