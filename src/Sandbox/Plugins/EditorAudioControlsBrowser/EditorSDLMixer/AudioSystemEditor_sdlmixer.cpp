// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "StdAfx.h"
#include "AudioSystemEditor_sdlmixer.h"
#include "SDLMixerProjectLoader.h"
#include "AudioSystemControl_sdlmixer.h"
#include "ISystem.h"
#include <crc32.h>

namespace AudioControls
{
	const string CAudioSystemEditor_sdlmixer::ms_sProjectFilePath = "sounds/sdlmixer";
	const string CAudioSystemEditor_sdlmixer::ms_sEventConnectionTag = "SDLMixerEvent";
	const string CAudioSystemEditor_sdlmixer::ms_sSampleConnectionTag = "SDLMixerSample";
	const string CAudioSystemEditor_sdlmixer::ms_sControlNameTag = "sdl_name";
	const string CAudioSystemEditor_sdlmixer::ms_sPanningEnabledTag = "enable_panning";
	const string CAudioSystemEditor_sdlmixer::ms_sAttenuationEnabledTag = "enable_distance_attenuation";
	const string CAudioSystemEditor_sdlmixer::ms_sAttenuationDistMin = "attenuation_dist_min";
	const string CAudioSystemEditor_sdlmixer::ms_sAttenuationDistMax = "attenuation_dist_max";
	const string CAudioSystemEditor_sdlmixer::ms_sVolumeTag = "volume";
	const string CAudioSystemEditor_sdlmixer::ms_sLoopCountTag = "loop_count";

	CAudioSystemEditor_sdlmixer::CAudioSystemEditor_sdlmixer()
	{
		AudioControls::CSDLMixerProjectLoader(ms_sProjectFilePath, this);
	}

	CAudioSystemEditor_sdlmixer::~CAudioSystemEditor_sdlmixer()
	{
	}

	IAudioSystemControl* CAudioSystemEditor_sdlmixer::CreateControl(const string& name, TImplControlType type)
	{
		IAudioSystemControl_sdlmixer* pControl = new IAudioSystemControl_sdlmixer(name, GetId(name), type);
		m_controls.push_back(pControl);
		return pControl;
	}

	int CAudioSystemEditor_sdlmixer::ControlCount() const
	{
		return m_controls.size();
	}

	IAudioSystemControl* CAudioSystemEditor_sdlmixer::GetControlByID(CID id) const
	{
		if (id >= 0)
		{
			size_t size = m_controls.size();
			for (size_t i = 0; i < size; ++i)
			{
				if (m_controls[i]->GetId() == id)
				{
					return m_controls[i];
				}
			}
		}
		return nullptr;
	}

	IAudioSystemControl* CAudioSystemEditor_sdlmixer::GetControlByIndex(unsigned int index) const
	{
		if (index < m_controls.size())
		{
			return m_controls[index];
		}
		return nullptr;
	}

	IAudioConnection* CAudioSystemEditor_sdlmixer::CreateConnectionToControl(IAudioSystemControl* pControl)
	{
		CSDLMixerConnection* pConnection = new CSDLMixerConnection();
		pConnection->SetControl(pControl);
		return pConnection;
	}

	IAudioConnection* CAudioSystemEditor_sdlmixer::CreateConnectionFromXMLNode(XmlNodeRef pNode)
	{
		if (pNode)
		{
			const string sTag = pNode->getTag();
			if (sTag == ms_sEventConnectionTag || sTag == ms_sSampleConnectionTag)
			{
				const string sName = pNode->getAttr(ms_sControlNameTag);
				CID id = GetId(sName);
				if (id != ACB_INVALID_ID)
				{
					CSDLMixerConnection* pConnection = new CSDLMixerConnection();

					IAudioSystemControl* pControl = GetControlByID(id);
					if (pControl == nullptr)
					{
						pControl = CreateControl(sName, eSDLMT_EVENT);
						if (pControl)
						{
							pControl->SetPlaceholder(true);
							pControl->m_controlTag = sTag;
						}
					}

					if (pControl)
					{
						pConnection->SetControl(pControl);

						const string sEnablePanning = pNode->getAttr(ms_sPanningEnabledTag);
						pConnection->bPanningEnabled = sEnablePanning == "true" ? true : false;

						const string sEnableDistAttenuation = pNode->getAttr(ms_sAttenuationEnabledTag);
						pConnection->bAttenuationEnabled  = sEnableDistAttenuation == "true" ? true : false;

						const string sAttenuationMin = pNode->getAttr(ms_sAttenuationDistMin);
						pConnection->fMinAttenuation = (float)std::atof(sAttenuationMin.c_str());

						const string sAttenuationMax = pNode->getAttr(ms_sAttenuationDistMax);
						pConnection->fMaxAttenuation = (float)std::atof(sAttenuationMax.c_str());

						const string sVolume = pNode->getAttr(ms_sVolumeTag);
						pConnection->fVolume = std::atof(sVolume.c_str());

						const string sLoopCount = pNode->getAttr(ms_sLoopCountTag);
						pConnection->nLoopCount = std::atoi(sLoopCount.c_str());

					}
					return pConnection;
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Audio Controls Browser (SDL Mixer): Error reading connection to %s", sName);
				}
			}
		}
		return nullptr;
	}

	void CAudioSystemEditor_sdlmixer::WriteConnectionToXMLNode(XmlNodeRef pNode, const IAudioConnection* pConnection, const EACBControlType eATLControlType)
	{
		const CSDLMixerConnection* pSDLMixerConnection = static_cast<const CSDLMixerConnection*>(pConnection);
		const IAudioSystemControl* pControl = pConnection->GetControl();
		if (pControl && pSDLMixerConnection)
		{
			if (eATLControlType == eACBT_TRIGGER)
			{
				XmlNodeRef pConnectionNode = pNode->createNode(ms_sEventConnectionTag);
				pConnectionNode->setAttr(ms_sControlNameTag, pControl->GetName());
				pConnectionNode->setAttr(ms_sPanningEnabledTag, pSDLMixerConnection->bPanningEnabled ? "true" : "false");
				pConnectionNode->setAttr(ms_sAttenuationEnabledTag, pSDLMixerConnection->bAttenuationEnabled ? "true" : "false");
				pConnectionNode->setAttr(ms_sAttenuationDistMin, pSDLMixerConnection->fMinAttenuation);
				pConnectionNode->setAttr(ms_sAttenuationDistMax, pSDLMixerConnection->fMaxAttenuation);
				pConnectionNode->setAttr(ms_sVolumeTag, pSDLMixerConnection->fVolume);
				pConnectionNode->setAttr(ms_sLoopCountTag, pSDLMixerConnection->nLoopCount);
				pNode->addChild(pConnectionNode);
			}
			else if (eATLControlType == eACBT_PRELOADS)
			{
				XmlNodeRef pConnectionNode = pNode->createNode(ms_sSampleConnectionTag);
				pConnectionNode->setAttr(ms_sControlNameTag, pControl->GetName());
				pNode->addChild(pConnectionNode);
			}
		}
	}

	void CAudioSystemEditor_sdlmixer::DestroyConnection(IAudioConnection* pConnection)
	{
		SAFE_DELETE(pConnection);
	}

	IAudioConnectionInspectorPanel* CAudioSystemEditor_sdlmixer::NewConnectionInspectorPanel() const
	{
		return new CAudioConnectionInspectorPanel_sdlmixer();
	}

	AudioControls::CID CAudioSystemEditor_sdlmixer::GetId(const string& sName) const
	{
		static Crc32Gen crcGenerator;
		return crcGenerator.GetCRC32(sName);
	}

	QIcon CAudioSystemEditor_sdlmixer::GetTypeIcon(TImplControlType type) const
	{
		return QIcon("://icons/Audio_Event.png");
	}

	AudioControls::EACBControlType CAudioSystemEditor_sdlmixer::ImplTypeToATLType(TImplControlType type) const
	{
		return eACBT_TRIGGER;
	}

	AudioControls::TImplControlTypeMask CAudioSystemEditor_sdlmixer::GetCompatibleTypes(EACBControlType eATLControlType) const
	{
		switch (eATLControlType)
		{
		case eACBT_TRIGGER:
			return eSDLMT_EVENT;
		case eACBT_PRELOADS:
			return eSDLMT_EVENT;
		}
		return AUDIO_IMPL_INVALID_TYPE;
	}
}