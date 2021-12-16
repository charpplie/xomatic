// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include <IXml.h>
#include "ACBTypes.h"
#include "AudioSystemControl_wwise.h"

namespace AudioControls
{
	class IAudioSystemEditor;

	class CAudioWwiseLoader
	{
	public:
		CAudioWwiseLoader(IAudioSystemEditor* pAudioSystemImpl);

	private:
		void LoadSoundBanks(const string& folderPath, bool bLocalised);
		void LoadControlsInFolder(const string& folderPath);
		void LoadControl(XmlNodeRef root);
		void ExtractControlsFromXML(XmlNodeRef root, EWwiseControlTypes type, const string& controlTag, const string& controlNameAttribute);

		IAudioSystemEditor* m_pAudioSystemImpl;
	};
}