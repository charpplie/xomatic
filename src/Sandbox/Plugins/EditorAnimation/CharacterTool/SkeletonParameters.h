#pragma once

#include <vector>
#include "../Shared/Strings.h"
#include "Serialization.h"

class XmlNodeRef;

namespace Serialization { class IArchive; }

namespace CharacterTool
{

using std::vector;

struct AnimationFilterWildcard
{
	string renameMask;
	string fileWildcard;

	AnimationFilterWildcard()
	: renameMask("")
	, fileWildcard("*/*.caf")
	{
	}

	void Serialize(IArchive& ar)
	{
		ar(renameMask, "renameMask", "^");
		ar(fileWildcard, "fileWildcard", "^ <- ");
	}

	bool operator==(const AnimationFilterWildcard& rhs) const
	{
		return renameMask == rhs.renameMask && fileWildcard == rhs.fileWildcard;
	}
	bool operator!=(const AnimationFilterWildcard& rhs) const{ return !operator==(rhs); }
};

struct AnimationFilterFolder
{
	string path;
	vector<AnimationFilterWildcard> wildcards;

	AnimationFilterFolder()
	{
		wildcards.resize(3);
		wildcards[0].fileWildcard = "*/*.caf";
		wildcards[1].fileWildcard = "*/*.bspace";
		wildcards[2].fileWildcard = "*/*.comb";
	}

	bool operator==(const AnimationFilterFolder& rhs) const
	{
		if (path != rhs.path)
			return false;
		if (wildcards.size() != rhs.wildcards.size())
			return false;
		for (size_t i = 0; i < wildcards.size(); ++i)
			if (wildcards[i] != rhs.wildcards[i])
				return false;
		return true;
	}
	bool operator!=(const AnimationFilterFolder& rhs) const{ return !operator==(rhs); }

	void Serialize(IArchive& ar)
	{
		ar(ResourceFolderPath(path), "path", "^");
		ar(wildcards, "wildcards", "^");
	}
};

struct AnimationSetFilter
{
	std::vector<AnimationFilterFolder> folders;

	void Serialize(IArchive& ar)
	{
		ar(folders, "folders", "^");
	}

	bool Matches(const char* cafPath) const;

	bool operator==(const AnimationSetFilter& rhs) const
	{
		if (folders.size() != rhs.folders.size())
			return false;
		for (size_t i = 0; i < folders.size(); ++i)
			if (folders[i] != rhs.folders[i])
				return false;
		return true;
	}
	bool operator!=(const AnimationSetFilter& rhs) const{ return !operator==(rhs); }
};

struct SkeletonParametersInclude
{
	string filename;
};

inline bool Serialize(IArchive& ar, SkeletonParametersInclude& ref, const char* name, const char* label)
{
	return ar(SkeletonParamsPath(ref.filename), name, label);
}

struct SkeletonParametersDBA
{
	string filename;
	bool persistent;

	SkeletonParametersDBA()
	: persistent(false)
	{
	}

	void Serialize(IArchive& ar)
	{
		ar(ResourceFilePath(filename, "Animation Databases (.dba)|*.dba"), "filename", "^");
		ar(persistent, "persistent", "^Persistent");
	}
};

struct SkeletonParameters
{
	vector<XmlNodeRef> unknownNodes;

	AnimationSetFilter animationSetFilter;
	vector<SkeletonParametersInclude> includes;
	string animationEventDatabase;
	string faceLibFile;

	string dbaPath;
	vector<SkeletonParametersDBA> individualDBAs;

	bool LoadFromXMLFile(const char* filename, bool* dataLost = 0);
	bool LoadFromXML(XmlNodeRef xml, bool* dataLost = 0);
	XmlNodeRef SaveToXML();
	bool SaveToMemory(vector<char>* buffer);

	void Serialize(Serialization::IArchive& ar);
};

}
