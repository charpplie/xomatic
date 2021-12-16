#include "ScanAndLoadFilesTask.h"
#include <ICryPak.h>
#include <StringUtils.h>

namespace CharacterTool
{

static bool LoadFile(vector<char>* buf, const char* filename)
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

SScanAndLoadFilesTask::SScanAndLoadFilesTask(const SLookupRule& rule, const char* description, int index)
: m_rule(rule)
, m_description(description)
, m_index(index)
{
}

ETaskResult SScanAndLoadFilesTask::Work()
{
	const SLookupRule& rule = m_rule;
	vector<string> masks = rule.masks;

	vector<string> filenames;

	for (size_t j = 0; j < masks.size(); ++j)
	{
		filenames.clear();
		const string& mask = masks[j];
		SDirectoryEnumeratorHelper dirHelper;
		dirHelper.ScanDirectoryRecursive("", "", mask.c_str(), filenames);

		m_loadedFiles.reserve(m_loadedFiles.size() + filenames.size());
		for (int k = 0; k < filenames.size(); ++k)
		{
			if (!CryStringUtils::MatchWildcard(filenames[k].c_str(), mask.c_str()))
				continue;

			ScanLoadedFile file;
			file.scannedFile = filenames[k].c_str();
			file.fromPak = gEnv->pCryPak->IsFileExist(file.scannedFile.c_str(), ICryPak::eFileLocation_InPak);
			file.fromDisk = gEnv->pCryPak->IsFileExist(file.scannedFile.c_str(), ICryPak::eFileLocation_OnDisk);
			m_loadedFiles.push_back(file);
		}
	}
	
	return eTaskResult_Completed;
}

void SScanAndLoadFilesTask::Finalize()
{		
	for (size_t i = 0; i < m_loadedFiles.size(); ++i)
	{
		const ScanLoadedFile& loadedFile = m_loadedFiles[i];

		SignalFileLoaded(loadedFile);
	}

	SignalLoadingFinished(m_index);
}

}

#include <CharacterTool/moc_ScanAndLoadFilesTask.cpp>
