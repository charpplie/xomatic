#include "ExplorerFileList.h"
#include "EntryListImpl.h"
#include <IBackgroundTaskManager.h>
#include <IEditorFileMonitor.h>
#include <IEditor.h>
#include "Util/PathUtil.h"
#include "ScanAndLoadFilesTask.h"
#include "Explorer.h"
#include "Expected.h"
#include "Serialization/JSONIArchive.h"
#include "Serialization/JSONOArchive.h"

namespace CharacterTool
{

string EntryFormat::MakeFilename(const char* entryPath) const
{
	return path.empty() ? PathUtil::ReplaceExtension(entryPath, extension) : path;
}

static bool LoadFile(vector<char>* buf, const char* filename, LoaderContext* context)
{
	FILE* f = gEnv->pCryPak->FOpen(filename, "rb");
	if (!f)
		return false;

	gEnv->pCryPak->FSeek(f, 0, SEEK_END);
	size_t size = gEnv->pCryPak->FTell(f);
	gEnv->pCryPak->FSeek(f, 0, SEEK_SET);

	buf->resize(size);
	bool result = gEnv->pCryPak->FRead(&(*buf)[0], size, f) == size;
	gEnv->pCryPak->FClose(f);
	return result;
}

bool JSONLoader::Load(EntryBase* entry, const char* filename, LoaderContext* context)
{
	string filePath = string(gEnv->pCryPak->GetGameFolder()) + "\\" + filename;
	filePath.replace('/', '\\');

	vector<char> content;
	if (!LoadFile(&content, filePath.c_str(), context))
		return false;

	Serialization::JSONIArchive ia;
	if (content.empty())
		return true;

	if (!ia.open(content.data(), content.size()))
		return false;

	ia(*entry, "");
	return true;
}

bool JSONLoader::Save(EntryBase* entry, const char* filename, LoaderContext* context)
{
	Serialization::JSONOArchive oa;
	oa(*entry, "");

	string filePath = string(gEnv->pCryPak->GetGameFolder()) + "\\" + filename;
	filePath.replace('/', '\\');
	return oa.save(filePath.c_str());
}

// ---------------------------------------------------------------------------

ExplorerFileList::~ExplorerFileList()
{
	for (size_t i = 0; i < m_entryTypes.size(); ++i)
	{
		delete m_entryTypes[i].entryList;
		m_entryTypes[i].entryList = 0;
	}

	GetIEditor()->GetFileMonitor()->UnregisterListener(this);
}

EntryType& ExplorerFileList::AddEntryType(EntryListBase* list, int subtree, int explorerEntryType, IEntryDependencyExtractor* extractor)
{
	EntryType type;
	type.entryList = list;
	type.subtree = subtree;
	type.explorerEntryType = explorerEntryType;
	type.dependencyExtractor.reset(extractor);

	m_entryTypes.push_back(type);
	
	if (m_entryTypes.size() > 1)
		m_entryTypes.back().entryList->SetIdProvider(m_entryTypes[0].entryList);

	return m_entryTypes.back();
}

EntryType* ExplorerFileList::AddSingleFileEntryType(EntryListBase* list, const char* path, const char* label, int subtree, int explorerEntryType, IEntryLoader* loader)
{
	EntryType entryType;
	entryType.entryList = list;
	entryType.subtree = subtree;
	entryType.explorerEntryType = explorerEntryType;

	EntryFormat format;
	format.path = path;
	format.loader = loader;
	format.usage = FORMAT_LOAD|FORMAT_SAVE|FORMAT_LIST;
	entryType.formats.push_back(format);

	m_entryTypes.push_back(entryType);
	
	if (m_entryTypes.size() > 1)
		m_entryTypes.back().entryList->SetIdProvider(m_entryTypes[0].entryList);

	return &m_entryTypes.back();
}

void ExplorerFileList::Populate()
{
	for (size_t i = 0; i < m_entryTypes.size(); ++i)
	{
		EntryType& entryType = m_entryTypes[i];

		bool singleFileFormat = false;
		if (!entryType.fileListenerRegistered)
		{
			for (size_t j = 0; j < entryType.formats.size(); ++j)
		{
				const EntryFormat& format = entryType.formats[j];
			if (!format.path.empty())
				{
				GetIEditor()->GetFileMonitor()->RegisterListener(this, format.path.c_str());
					singleFileFormat = true;
				}
			else
				{
				GetIEditor()->GetFileMonitor()->RegisterListener(this, "", format.extension);
				}
			}
			entryType.fileListenerRegistered = true;
		}

		if (singleFileFormat)
			continue;
		
		entryType.entryList->Clear();
		SignalSubtreeReset(entryType.subtree);

		string firstExtension;
		SLookupRule r;
		for (size_t j = 0; j < entryType.formats.size(); ++j)
		{
			const EntryFormat& format = entryType.formats[j];
			if (!format.path.empty())
				continue;
			if ((format.usage & FORMAT_LIST) == 0)
				continue;
			r.masks.push_back(string("*.") + format.extension);
			if (firstExtension.empty())
				firstExtension = format.extension;
		}
		string description = firstExtension.empty() ? string() : (firstExtension + " scan");
		SScanAndLoadFilesTask* task = new SScanAndLoadFilesTask(r, description.c_str(), entryType.subtree);
		EXPECTED(connect(task, SIGNAL(SignalFileLoaded(const ScanLoadedFile&)), this, SLOT(OnBackgroundFileLoaded(const ScanLoadedFile&))));
		EXPECTED(connect(task, SIGNAL(SignalLoadingFinished(int)), this, SLOT(OnBackgroundLoadingFinished(int))));
		GetIEditor()->GetBackgroundTaskManager()->AddTask(task, eTaskPriority_FileUpdate, eTaskThreadMask_IO);
	}
}

EntryType* ExplorerFileList::GetEntryTypeByExtension(const char* ext)
{
	for (size_t i = 0; i < m_entryTypes.size(); ++i)
		for (size_t j = 0; j < m_entryTypes[i].formats.size(); ++j)
			if (stricmp(m_entryTypes[i].formats[j].extension, ext) == 0)
				return &m_entryTypes[i];
	return 0;
}

EntryType* ExplorerFileList::GetEntryTypeByPath(const char* singleFilePath)
{
	for (size_t i = 0; i < m_entryTypes.size(); ++i)
		for (size_t j = 0; j < m_entryTypes[i].formats.size(); ++j)
			if (stricmp(m_entryTypes[i].formats[j].path.c_str(), singleFilePath) == 0)
				return &m_entryTypes[i];
	return 0;
}

EntryType* ExplorerFileList::GetEntryTypeByExplorerEntryType(int explorerEntryType)
{
	for (size_t i = 0; i < m_entryTypes.size(); ++i)
		if (m_entryTypes[i].explorerEntryType == explorerEntryType)
			return &m_entryTypes[i];
	return 0;
}

bool ExplorerFileList::AddAndSaveEntry(const char* filename)
{
	unsigned int id = AddEntry(filename).id;
	if (!id)
		return false;
	return SaveEntry(0, id);
}

void ExplorerFileList::SetExplorer(Explorer* explorer)
{
	m_explorer = explorer; 
	m_explorerColumnPak = explorer->FindColumn("Pak");
}

ExplorerEntryId ExplorerFileList::AddEntry(const char* filename)
{
	const char* ext = PathUtil::GetExt(filename);
	const EntryType* format = GetEntryTypeByExtension(ext);
	if (!format)
		return ExplorerEntryId();		

	bool newEntry;
	EntryBase* entry = format->entryList->AddEntry(&newEntry, filename, 0);

	if (newEntry)
		SignalEntryAdded(format->subtree, entry->id);
	else
	{
		EntryModifiedEvent ev;
		ev.subtree = format->subtree;
		ev.id = entry->id;
		SignalEntryModified(ev);
	}
	return ExplorerEntryId(format->subtree, entry->id);
}

EntryBase* ExplorerFileList::GetEntryBaseByPath(const char* path)
{
	for (size_t i = 0; i < m_entryTypes.size(); ++i) {
		EntryBase* entry = m_entryTypes[i].entryList->GetBaseByPath(path);
		if (entry)
			return entry;
	}
	return 0;
}

EntryBase* ExplorerFileList::GetEntryBaseById(unsigned int id)
{
	for (size_t i = 0; i < m_entryTypes.size(); ++i) {
		EntryBase* entry = m_entryTypes[i].entryList->GetBaseById(id);
		if (entry)
			return entry;
	}
	return 0;
}

void ExplorerFileList::OnBackgroundLoadingFinished(int subtree)
{
	SignalSubtreeLoadingFinished(subtree);
}

string ExplorerFileList::GetCanonicalPath(const char* path) const
{
	const char* canonicalExtension = CanonicalExtension();
	if (canonicalExtension[0] != '\0')
		return PathUtil::ReplaceExtension(path, canonicalExtension);
	else
		return path;
}

void ExplorerFileList::OnBackgroundFileLoaded(const ScanLoadedFile& file)
{
	string path = GetCanonicalPath(file.scannedFile.c_str());
	ExplorerEntryId entryId = AddEntry(path.c_str());

	int pakState = 0;
		if (file.fromPak)
			pakState |= PAK_STATE_PAK;
	if (file.fromDisk)
			pakState |= PAK_STATE_LOOSE_FILES;
	UpdateEntryPakState(entryId, pakState);
}

int ExplorerFileList::GetEntryCount(int subtree) const
{
	int result = 0;
	for (size_t i = 0; i < m_entryTypes.size(); ++i)
		if (m_entryTypes[i].subtree == subtree)
			result += m_entryTypes[i].entryList->Count();
	return result;
}

unsigned int ExplorerFileList::GetEntryIdByIndex(int subtree, int index) const
{
	int startRange = 0;
	for (size_t i = 0; i < m_entryTypes.size(); ++i)
		if (m_entryTypes[i].subtree == subtree)
		{
			int size = m_entryTypes[i].entryList->Count();
			if (index >= startRange && index < startRange + size)
			{
				if (EntryBase* entry = m_entryTypes[i].entryList->GetBaseByIndex(index - startRange))
					return entry->id;
				else
					return 0;
			}
			startRange += size;
		}

	return 0;
}

int ExplorerFileList::GetEntryType(int subtreeIndex) const 
{
	for (size_t i = 0; i < m_entryTypes.size(); ++i)
		if (m_entryTypes[i].subtree == subtreeIndex)
			return m_entryTypes[i].explorerEntryType;

	return 0;
}

bool ExplorerFileList::SaveAll(ActionOutput* output)
{
	bool failed = false;
	for (size_t j = 0; j < m_entryTypes.size(); ++j)
	{
		EntryType& format = m_entryTypes[j];
		for (size_t i = 0; i < format.entryList->Count(); ++i)
		{
			if (EntryBase* entry = format.entryList->GetBaseByIndex(i))
			{
				if (!entry->modified)
					continue;
				int errorCount = output->errorCount;
				if (!SaveEntry(output, entry->id))
				{
					if (errorCount == output->errorCount)
						output->AddError("Failed to save file", entry->path.c_str());
					failed = true;
				}
			}
		}
	}
	return !failed;
}

static bool SaveEntryOfType(ActionOutput* output, EntryBase* entry, LoaderContext* loaderContext, const EntryType* entryType)
{
	for (size_t i = 0; i < entryType->formats.size(); ++i)
	{
		const EntryFormat& format = entryType->formats[i];
		if (!format.loader.get())
			continue;
		if (!(format.usage & FORMAT_SAVE))
				continue;
		string filename = format.MakeFilename(entry->path.c_str());
		if (!format.loader->Save(entry, filename.c_str(), loaderContext))
		{
			if (output)
				output->AddError("Failed to save file", filename.c_str());
			return false;
		}
	}
	return true;
}

bool ExplorerFileList::SaveEntry(ActionOutput* output, unsigned int id)
{
	const EntryType* entryType;
	if (EntryBase* entry = GetEntry(id, &entryType))
	{
		if (!entry->loaded)
			LoadOrGetChangedEntry(id);
		
		if (SaveEntryOfType(output, entry, m_loaderContext, entryType))
		{
			if (entryType->entryList->EntrySaved(entry))
			{
				EntryModifiedEvent ev;
				ev.subtree = entryType->subtree;
				ev.id = id;
				SignalEntryModified(ev);
			}

			int pakState = GetEntryFilesPakState(*entryType, entry->path.c_str());
			UpdateEntryPakState(ExplorerEntryId(entryType->subtree, id), pakState);
			return true;
		}
		else
		{
			if (output)
				output->AddError("Failed to save entry", "");
			return false;
		}
	}

	if (output)
		output->AddError("Saving unexisting entry", "");
	return false;
}

int ExplorerFileList::GetEntryFilesPakState(const EntryType& entryType, const char* filename)
{
	int pakState = 0;
	for (size_t i = 0; i < entryType.formats.size(); ++i)
	{
		const char* extension = entryType.formats[i].extension.c_str();

		if (!extension[0])
			continue;

		string assetFile = PathUtil::ReplaceExtension(filename, extension);
		if (gEnv->pCryPak->IsFileExist(assetFile, ICryPak::eFileLocation_OnDisk))
			pakState |= PAK_STATE_LOOSE_FILES;
		else if (gEnv->pCryPak->IsFileExist(assetFile, ICryPak::eFileLocation_InPak))
			pakState |= PAK_STATE_PAK;
	}
	return pakState;
}

void ExplorerFileList::RevertEntry(unsigned int id)
{
	const EntryType* entryType;
	EntryBase* entry = GetEntry(id, &entryType);
	if (entry)
	{
		bool listedFileExists = false;
		for (size_t i = 0; i < entryType->formats.size(); ++i)
		{
			const EntryFormat& format = entryType->formats[i];
			if ((format.usage & FORMAT_LIST) == 0)
				continue;

			string filename = format.MakeFilename(entry->path.c_str());
			if (gEnv->pCryPak->IsFileExist(filename))
				listedFileExists = true;
		}

		if (!listedFileExists)
		{
			if (entryType->entryList->RemoveById(id))
				SignalEntryRemoved(entryType->subtree, id);
		 	return;
		}			

		entry->failedToLoad = false;
		for (size_t i = 0; i < entryType->formats.size(); ++i)
		{
			const EntryFormat& format = entryType->formats[i];

			string filename = format.MakeFilename(entry->path.c_str());
			bool fileExists = gEnv->pCryPak->IsFileExist(entry->path.c_str());
			if (!format.loader)
				continue;
			if (!format.loader->Load(entry, filename.c_str(), m_loaderContext))
				entry->failedToLoad = true;
		}
		entry->StoreSavedContent();

		EntryModifiedEvent ev;
		if (entryType->entryList->EntryReverted(&ev.previousContent, entry))
		{
			ev.subtree = entryType->subtree;
			ev.id = entry->id;
			ev.reason = "Revert";
			ev.contentChanged = ev.previousContent != entry->lastContent;
			SignalEntryModified(ev);
		}

		int pakState = GetEntryFilesPakState(*entryType, entry->path.c_str());
		UpdateEntryPakState(ExplorerEntryId(entryType->subtree, id), pakState);
	}
}

EntryBase* ExplorerFileList::GetEntry(EntryId id, const EntryType** outFormat) const
{
	for (size_t i = 0; i < m_entryTypes.size(); ++i)
	{
		const EntryType& format = m_entryTypes[i];

		if (EntryBase* entry = format.entryList->GetBaseById(id))
		{
			if (outFormat)
				*outFormat = &format;
			return entry;
		}
	}
	return 0;
}

void ExplorerFileList::CheckIfModified(unsigned int id, const char* reason, bool continuousChange)
{
	const EntryType* format;
	EntryBase* entry = GetEntry(id, &format);
	if (!entry)
		return;
	EntryModifiedEvent ev;
	ev.continuousChange = continuousChange;
	if (continuousChange || format->entryList->EntryChanged(&ev.previousContent, entry))
	{
		ev.subtree = format->subtree;
		ev.id = entry->id;
		if (reason)
		{
			ev.reason = reason;
			ev.contentChanged = true;
		}
		SignalEntryModified(ev);
	}
}

void ExplorerFileList::GetEntryActions(vector<ExplorerAction>* actions, unsigned int id, Explorer* explorer)
{
	if (GetEntry(id))
	{
		actions->push_back(ExplorerAction(functor(*explorer, &Explorer::ActionRevert), "Revert", true, "Editor/Icons/animation/revert.png", false));
		actions->push_back(ExplorerAction(functor(*explorer, &Explorer::ActionSave), "Save", true, "Editor/Icons/animation/save.png", false));
		actions->push_back(ExplorerAction());
		actions->push_back(ExplorerAction(functor(*explorer, &Explorer::ActionShowInExplorer), "Show in Explorer", true, "Editor/Icons/animation/show_in_explorer.png", false, false));
	}
}

void ExplorerFileList::UpdateEntry(ExplorerEntry* explorerEntry) 
{
	if (EntryBase* entry = GetEntry(explorerEntry->id))
	{
		explorerEntry->name = entry->name;
		explorerEntry->path = entry->path;
		explorerEntry->modified = entry->modified;
	}
}

bool ExplorerFileList::GetEntrySerializer(Serialization::SStruct* out, unsigned int id) const
{
	if (EntryBase* entry = GetEntry(id))
	{
		*out = Serialization::SStruct(*entry);
		return true;
	}
	return false;
}

static void LoadEntryOfType(EntryBase* entry, LoaderContext* loaderContext, const EntryType* entryType)
{
	entry->loaded = true;
	entry->failedToLoad = false;
	for (size_t i = 0; i < entryType->formats.size(); ++i)
	{
		const EntryFormat& format = entryType->formats[i];
		if (!(format.usage & FORMAT_LOAD))
			continue;
		if (!format.loader)
			continue;
		string filename = format.MakeFilename(entry->path.c_str());
		if (!format.loader->Load(entry, filename.c_str(), loaderContext))
			entry->failedToLoad = true;
	}
}

static bool ListedFilesExist(const EntryBase* entry, const EntryType* entryType)
{
	for (size_t i = 0; i < entryType->formats.size(); ++i)
	{
		const EntryFormat& format = entryType->formats[i];
		if (!(format.usage & FORMAT_LIST))
			continue;
		string filename = format.MakeFilename(entry->path.c_str());
		if (gEnv->pCryPak->IsFileExist(filename.c_str()))
			return true;
	}
	return false;
}

void ExplorerFileList::OnFileChange(const char* filename, EChangeType eType)
{
	EntryType* entryType = GetEntryTypeByPath(filename);
	const string originalExt = PathUtil::GetExt(filename);
	if (!entryType)
		entryType = GetEntryTypeByExtension(originalExt);

	if (!entryType)
		return;

	bool fileExists = gEnv->pCryPak->IsFileExist(filename, ICryPak::eFileLocation_Any);

	string listedPath = GetCanonicalPath(filename);
	listedPath.replace('\\', '/');
	if (fileExists)
	{

		ExplorerEntryId entryId;

		EntryBase* entry = entryType->entryList->GetBaseByPath(listedPath.c_str());
		if (!entry)
		{

			entryId = AddEntry(listedPath.c_str());

			entry = entryType->entryList->GetBaseByPath(listedPath.c_str());
		}

		if (entry)
		{
			entryId = ExplorerEntryId(entryType->subtree, entry->id);

			LoadEntryOfType(entry, m_loaderContext, entryType);
					
			EntryModifiedEvent ev;
			if (entryType->entryList->EntryChanged(&ev.previousContent, entry))
			{
				ev.subtree = entryType->subtree;
				ev.id = entry->id;
				ev.reason = "Reload";
				ev.contentChanged = true;					
				SignalEntryModified(ev);
			}

			int pakState = GetEntryFilesPakState(*entryType, listedPath.c_str());
			UpdateEntryPakState(entryId, pakState);
		}

	}
	else
	{
		EntryBase* entry = entryType->entryList->GetBaseByPath(listedPath.c_str());
		if (entry && !entry->modified && !ListedFilesExist(entry, entryType))
		{
			unsigned int id = entry->id;
			if (entryType->entryList->RemoveById(id))
				SignalEntryRemoved(entryType->subtree, id);
		}
	}
}

bool ExplorerFileList::HasBackgroundLoading() const
{ 
	for (size_t i = 0; i < m_entryTypes.size(); ++i)
		for (size_t j = 0; j < m_entryTypes[i].formats.size(); ++j)
			if (m_entryTypes[i].formats[j].path.empty())
			return true;
	return false;
}

bool ExplorerFileList::LoadOrGetChangedEntry(unsigned int id)
{
	const EntryType* entryType = 0;
	if (EntryBase* entry = GetEntry(id, &entryType))
	{
		if (!entry->loaded)
		{
			LoadEntryOfType(entry, m_loaderContext, entryType);
			entry->StoreSavedContent();
			entry->lastContent = entry->savedContent;
		}
		return true;
	}
	return false;
}

const char* ExplorerFileList::CanonicalExtension() const
{
	const EntryType& entryType = m_entryTypes[0];
	for (size_t i = 0; i < entryType.formats.size(); ++i)
	{
		const EntryFormat& format = entryType.formats[i];
		if (format.usage & FORMAT_MAIN)
			return format.extension.c_str();
	}
	return "";
}


void ExplorerFileList::UpdateEntryPakState(const ExplorerEntryId& id, int state)
{
	if (m_explorer)
		m_explorer->SetEntryColumn(id, m_explorerColumnPak, state, true);
}

void ExplorerFileList::GetDependencies(vector<string>* paths, unsigned int id)
{
	const EntryType* format = 0;
	EntryBase* entry = GetEntry(id, &format);
	if (!entry)
		return;
	if (format->dependencyExtractor)
		format->dependencyExtractor->Extract(paths, entry);
}

}

#include <CharacterTool/moc_ExplorerFileList.cpp>
