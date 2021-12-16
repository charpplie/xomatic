// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "CryString.h"
#include "common/IAudioConnection.h"
#include "AudioControl.h"
#include <IXml.h>

namespace AudioControls
{
	class CATLControlsModel;
	class IAudioSystemEditor;

	class CAudioControlsLoader
	{
	public:
		CAudioControlsLoader(CATLControlsModel* pATLModel, IAudioSystemEditor* pAudioSystemImpl);
		std::set<string> GetLoadedFilenamesList();
		void LoadAll();
		void LoadControls();
		void LoadScopes();

	private:
		void LoadAllLibrariesInFolder(const string& folderPath, const string& level);
		void LoadATLLibrary(XmlNodeRef pRoot, const string& sFilepath, const string& sLevel, const string& sFilename);
		void ProcessPreloads(XmlNodeRef root, const string& filename, const string& filepath, const string& level);

		void CreateDefaultControls();

		void ProcessConnections(XmlNodeRef root, CATLControl* pControl);
		void LoadSettings();
		void LoadScopesImpl(const string& path);

		static const string ms_sControlsPath;
		static const string ms_sControlsLevelsFolder;
		static const string ms_sConfigFilePath;
		static const string ms_sLevelsFolder;

		CATLControlsModel* m_pModel;
		IAudioSystemEditor* m_pAudioSystemImpl;
		std::set<string> m_loadedFilenames;
	};
}