// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "IAudioSystemEditor.h"
#include "AudioConnectionInspectorPanel_sdlmixer.h"
#include "IAudioConnection.h"

namespace AudioControls
{
	class CAudioConnectionInspectorPanel_sdlmixer;

	class CSDLMixerConnection : public IAudioConnection
	{
	public:
		CSDLMixerConnection()
			: bPanningEnabled(true)
			, bAttenuationEnabled(true)
			, fMinAttenuation(0.0f)
			, fMaxAttenuation(100.0f)
			, fVolume(1.0f)
			, nLoopCount(1)
		{ }

		virtual ~CSDLMixerConnection() {}

		float fMinAttenuation;
		float fMaxAttenuation;
		float fVolume;
		int nLoopCount;
		bool bPanningEnabled;
		bool bAttenuationEnabled;
	};

	class CAudioSystemEditor_sdlmixer : public IAudioSystemEditor
	{
	public:
		CAudioSystemEditor_sdlmixer();
		virtual ~CAudioSystemEditor_sdlmixer();

		virtual IAudioSystemControl* CreateControl(const string& name, TImplControlType type);

		// Access controls
		virtual int ControlCount() const;
		virtual IAudioSystemControl* GetControlByID(CID id) const;
		virtual IAudioSystemControl* GetControlByIndex(unsigned int index) const;
		virtual EACBControlType ImplTypeToATLType(TImplControlType type) const;
		virtual TImplControlTypeMask GetCompatibleTypes(EACBControlType eATLControlType) const;
		CID GetId(const string& sName) const;

		// Connections
		virtual IAudioConnection* CreateConnectionToControl(IAudioSystemControl* pControl);
		virtual IAudioConnection* CreateConnectionFromXMLNode(XmlNodeRef pNode);
		virtual void DestroyConnection(IAudioConnection* pConnection);
		virtual void WriteConnectionToXMLNode(XmlNodeRef pNode, const IAudioConnection* pConnection, const EACBControlType eATLControlType);

		// UI
		virtual IAudioConnectionInspectorPanel* NewConnectionInspectorPanel() const;
		virtual QIcon GetTypeIcon(TImplControlType type) const;

	private:

		static const string ms_sProjectFilePath;
		static const string ms_sControlNameTag;
		static const string ms_sEventConnectionTag;
		static const string ms_sSampleConnectionTag;
		static const string ms_sPanningEnabledTag;
		static const string ms_sAttenuationEnabledTag;
		static const string ms_sAttenuationDistMin;
		static const string ms_sAttenuationDistMax;
		static const string ms_sVolumeTag;
		static const string ms_sLoopCountTag;

		std::vector<IAudioSystemControl*> m_controls;

	};
}