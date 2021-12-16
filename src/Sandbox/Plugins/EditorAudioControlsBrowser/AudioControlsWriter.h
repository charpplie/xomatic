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

	class CAudioControlsWriter
	{
	public:
		CAudioControlsWriter(CATLControlsModel& ATLModel, IAudioSystemEditor* pAudioSystemImpl, std::set<string>& loadedFilenames);

	private:
		void ProcessPreloads(XmlNodeRef preloadsNode, CATLControl* pControl);
		void ProcessSwitches(std::map<string, std::vector<CATLControl*>>& switches, XmlNodeRef switchesNode);
		void AddControlToNode(XmlNodeRef node, CATLControl* pControl, const string& tag);
		void AddConnections(XmlNodeRef node, CATLControl* pControl);
		void CheckOutFile(const string& filepath);
		void DeleteLibraryFile(const string& filepath);
		CATLControlsModel& m_ATLModel;
		IAudioSystemEditor* m_pAudioSystemImpl;
	};
}