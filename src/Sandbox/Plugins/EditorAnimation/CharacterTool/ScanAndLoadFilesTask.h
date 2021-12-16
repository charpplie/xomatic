#pragma once

#include <QObject>
#include "Strings.h"
#include "Pointers.h"
#include <IBackgroundTaskManager.h>
#include <vector>

namespace CharacterTool
{
using std::vector;

struct SLookupRule
{
	vector<string> masks;

	SLookupRule()
	{
	}
};

struct ScanLoadedFile
{
	string scannedFile;
	string loadedFile;
	vector<char> content;
	bool fromPak;
	bool fromDisk;

	ScanLoadedFile() : fromPak(false), fromDisk(false) {}
};

struct SScanAndLoadFilesTask : QObject, IBackgroundTask
{
  Q_OBJECT
public:
	SLookupRule m_rule;
	int m_index;
	vector<ScanLoadedFile> m_loadedFiles;
	string m_description;

	SScanAndLoadFilesTask(const SLookupRule& rule, const char* description, int index);
	ETaskResult Work() override;
	void Finalize() override;
	const char* Description() const { return m_description; }
	void Delete() override{ delete this; }

signals:
	void SignalFileLoaded(const ScanLoadedFile& loadedFile);
	void SignalLoadingFinished(int index);
};

}
