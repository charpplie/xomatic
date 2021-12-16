#ifndef __CommunicationVoiceLibrary_h__
#define __CommunicationVoiceLibrary_h__

#pragma once

#include <ICommunicationVoiceLibrary.h>

typedef uint32 VoiceLibraryID;

class CommunicationVoiceLibrary : public ICommunicationVoiceLibrary
{
	enum
	{
		MaxVariationCount = 32,
	};
public:
	void Reset();
	void ResetHistory();

	void Reload();
	void ScanFolder(const char* folderName, bool recursing = false);
	bool LoadFromFile(const char* fileName);

	void InitEditor(IGameToEditorInterface* pGameToEditor);

	bool GetVoice(VoiceLibraryID libraryID, const char* name, const char*& libraryPath, const char*& voiceName);

	VoiceLibraryID GetVoiceLibraryID(const char* name) const;
	virtual uint32 GetGroupNames(const char* libraryName, uint32 maxCount, const char** names) const;

private:
	typedef std::vector<string> FolderNames;
	FolderNames m_folderNames;

	typedef std::vector<string> VoiceVariations;
	struct VoiceGroup
	{
		struct History
		{
			History()
				: played(0)
			{
			}

			uint32 played;
		};

		History history;

		VoiceVariations variations;
	};

	typedef std::map<string, VoiceGroup> VoiceGroups;
	struct VoiceLibrary
	{
		string name;
		string base;

		VoiceGroups voiceGroups;
	};

	typedef std::map<VoiceLibraryID, VoiceLibrary> VoiceLibraries;
	VoiceLibraries m_libraries;

	std::vector<uint8> m_randomPool;
};


#endif