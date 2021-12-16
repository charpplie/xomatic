#pragma once

#include <QObject>
#include <IFileChangeMonitor.h>
#include "Serialization/StringList.h"
#include "Strings.h"

namespace Serialization { class IArchive; }

namespace CharacterTool
{

class SkeletonList : public QObject, public IFileChangeListener
{
	Q_OBJECT
public:
	SkeletonList();
	~SkeletonList();

	bool Load();
	bool Save();
	bool HasSkeletonName(const char* skeletonName) const;
	string FindSkeletonNameByPath(const char* path) const;
	string FindSkeletonPathByName(const char* name) const;

	const Serialization::StringList& GetSkeletonNames() const{ return m_skeletonNames; }

	void Serialize(Serialization::IArchive& ar);

signals:
	void SignalSkeletonListModified();
private:
	void OnFileChange(const char* sFilename, EChangeType eType) override;

	Serialization::StringList m_skeletonNames;
	struct SEntry
	{
		string alias;
		string path;

		void Serialize(Serialization::IArchive& ar);

		bool operator<(const SEntry& rhs) const{ return alias == rhs.alias ? path < rhs.path : alias < rhs.alias; }
	};
	typedef std::vector<SEntry> SEntries;
	SEntries m_skeletonList;
};

}
