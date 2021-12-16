// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "StdAfx.h"
#include "AudioSystemEditor_wwise.h"
#include "AudioWwiseLoader.h"
#include "AudioSystemControl_wwise.h"
#include "ISystem.h"

namespace AudioControls
{
	TImplControlType TagToType(const string& tag)
	{
		if (tag == "WwiseSwitch" || tag == "WwiseState")
		{
			return eWCT_WWISE_SWITCH;
		}
		else if (tag == "WwiseFile")
		{
			return eWCT_WWISE_SOUND_BANK;
		}
		else if (tag == "WwiseRtpc")
		{
			return eWCT_WWISE_RTPC;
		}
		else if (tag == "WwiseEvent")
		{
			return eWCT_WWISE_EVENT;
		}
		else if (tag == "WwiseAuxBus")
		{
			return eWCT_WWISE_AUX_BUS;
		}
		return eWCT_INVALID;
	}

	CAudioSystemEditor_wwise::CAudioSystemEditor_wwise()
		: m_nextId(1)
	{
		AudioControls::CAudioWwiseLoader(this);
	}

	CAudioSystemEditor_wwise::~CAudioSystemEditor_wwise()
	{
		size_t size = m_controls.size();
		for (size_t i = 0; i < size; ++i)
		{
			SAFE_DELETE(m_controls[i]);
		}
		m_controls.clear();
	}

	IAudioSystemControl* CAudioSystemEditor_wwise::CreateControl(const string& name, TImplControlType type)
	{
		IAudioSystemControl_wwise* pControl = new IAudioSystemControl_wwise(name, GenerateUniqueId(), type);
		m_controls.push_back(pControl);
		return pControl;
	}

	int CAudioSystemEditor_wwise::ControlCount() const
	{
		return m_controls.size();
	}

	IAudioSystemControl* CAudioSystemEditor_wwise::GetControlByID(CID id) const
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

	IAudioSystemControl* CAudioSystemEditor_wwise::GetControlByName(TImplControlType type, const string& name, const string& path) const
	{
		size_t size = m_controls.size();
		for (size_t i = 0; i < size; ++i)
		{
			if ((name.compareNoCase(m_controls[i]->GetName()) == 0) && m_controls[i]->GetType() == type)
			{
				if (type != eWCT_WWISE_SWITCH || (path.compareNoCase(m_controls[i]->GetVirtualPath()) == 0))
				{
					return m_controls[i];
				}
			}
		}
		return nullptr;
	}

	IAudioSystemControl* CAudioSystemEditor_wwise::GetControlByIndex(unsigned int index) const
	{
		if (index < m_controls.size())
		{
			return m_controls[index];
		}
		return nullptr;
	}

	IAudioConnection* CAudioSystemEditor_wwise::CreateConnectionToControl(IAudioSystemControl* pControl)
	{
		CWwiseConnection* pConnection = new CWwiseConnection();
		pConnection->SetControl(pControl);
		return pConnection;
	}

	IAudioConnection* CAudioSystemEditor_wwise::CreateConnectionFromXMLNode(XmlNodeRef pNode)
	{
		if (pNode)
		{
			CWwiseConnection* pConnection = new CWwiseConnection();
			const string sTag = pNode->getTag();
			TImplControlType type = TagToType(sTag);
			if (type == AUDIO_IMPL_INVALID_TYPE)
			{
				return nullptr;
			}

			string sName = pNode->getAttr("wwise_name");
			string sParent = "";

			IAudioSystemControl* pControl = nullptr;
			if (type == eWCT_WWISE_SWITCH)
			{
				if (pNode->getChildCount() == 1)
				{
					XmlNodeRef pChild = pNode->getChild(0);
					if (pChild)
					{
						sParent = sName;
						sName = pChild->getAttr("wwise_name");
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Audio Controls Browser (Wwise): Error reading connection to Wwise control %s", sName);
				}
			}

			pControl = GetControlByName(type, sName, sParent);
			if (pControl == nullptr)
			{
				pControl = CreateControl(sName, type);
				if (pControl)
				{
					pControl->SetPlaceholder(true);
					pControl->m_controlTag = sTag;
					pControl->SetVirtualPath(sParent);
				}
			}

			if (pControl)
			{
				pConnection->SetControl(pControl);

				switch (type)
				{
				case eWCT_WWISE_RTPC:
				{
					float mult = 1.0f;
					float shift = 0.0f;
					if (pNode->haveAttr("atl_mult"))
					{
						const string sProperty = pNode->getAttr("atl_mult");
						mult = (float)std::atof(sProperty.c_str());
					}
					if (pNode->haveAttr("atl_shift"))
					{
						const string sProperty = pNode->getAttr("atl_shift");
						shift = (float)std::atof(sProperty.c_str());
					}
					pConnection->fMult = mult;
					pConnection->fShift = shift;

				}
				break;
				case eWCT_WWISE_SWITCH:
				{
					float value = 0.0f;
					if (pNode->haveAttr("atl_mult"))
					{
						const string sProperty = pNode->getAttr("wwise_value");
						value = (float)std::atof(sProperty.c_str());
					}
					pConnection->fValue = value;
				}
				break;
				}
			}
			return pConnection;
		}
		return nullptr;
	}

	void CAudioSystemEditor_wwise::WriteConnectionToXMLNode(XmlNodeRef pNode, const IAudioConnection* pConnection, const EACBControlType eATLControlType)
	{
		const CWwiseConnection* pWwiseConnection = static_cast<const CWwiseConnection*>(pConnection);
		const IAudioSystemControl* pControl = pConnection->GetControl();
		if (pControl && pWwiseConnection)
		{
			switch (pControl->GetType())
			{
			case AudioControls::eWCT_WWISE_SWITCH:
			{
				XmlNodeRef pSwitchNode;
				pSwitchNode = pNode->createNode(pControl->m_controlTag.c_str());
				pSwitchNode->setAttr("wwise_name", pControl->GetVirtualPath());
				XmlNodeRef pStateNode = pSwitchNode->createNode("WwiseValue");
				pStateNode->setAttr("wwise_name", pControl->GetName());
				pSwitchNode->addChild(pStateNode);
				pNode->addChild(pSwitchNode);
			}
			break;

			case AudioControls::eWCT_WWISE_RTPC:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = pNode->createNode("WwiseRtpc");
				pConnectionNode->setAttr("wwise_name", pControl->GetName());
				if (eATLControlType == AudioControls::eACBT_SWITCH)
				{
					pConnectionNode->setAttr("wwise_value", pWwiseConnection->fValue);
				}
				else
				{
					if (pWwiseConnection->fMult != 1.0f)
					{
						pConnectionNode->setAttr("atl_mult", pWwiseConnection->fMult);
					}
					if (pWwiseConnection->fShift != 0.0f)
					{
						pConnectionNode->setAttr("atl_shift", pWwiseConnection->fShift);
					}
				}
				pNode->addChild(pConnectionNode);
			}
			break;

			case AudioControls::eWCT_WWISE_EVENT:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = pNode->createNode("WwiseEvent");
				pConnectionNode->setAttr("wwise_name", pControl->GetName());
				pNode->addChild(pConnectionNode);
			}
			break;

			case AudioControls::eWCT_WWISE_AUX_BUS:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = pNode->createNode("WwiseAuxBus");
				pConnectionNode->setAttr("wwise_name", pControl->GetName());
				pNode->addChild(pConnectionNode);
			}
			break;

			case AudioControls::eWCT_WWISE_SOUND_BANK:
			{
				XmlNodeRef pConnectionNode = pNode->createNode("WwiseFile");
				pConnectionNode->setAttr("wwise_name", pControl->GetName());
				if (pControl->IsLocalised())
				{
					pConnectionNode->setAttr("wwise_localised", "true");
				}
				pNode->addChild(pConnectionNode);
			}
			break;
			}
		}
	}

	void CAudioSystemEditor_wwise::DestroyConnection(IAudioConnection* pConnection)
	{
		SAFE_DELETE(pConnection);
	}

	IAudioConnectionInspectorPanel* CAudioSystemEditor_wwise::NewConnectionInspectorPanel() const
	{
		return new CAudioConnectionInspectorPanel_wwise();
	}

	QIcon CAudioSystemEditor_wwise::GetTypeIcon(TImplControlType type) const
	{
		switch (type)
		{
		case eWCT_WWISE_EVENT:
			return QIcon("://icons/event_nor.png");;
			break;
		case eWCT_WWISE_RTPC:
			return QIcon("://icons/gameparameter_nor.png");
			break;
		case eWCT_WWISE_SWITCH:
			return QIcon("://icons/switch_nor.png");
			break;
		case eWCT_WWISE_AUX_BUS:
			return QIcon("://icons/auxbus_nor.png");
			break;
		case eWCT_WWISE_SOUND_BANK:
			return QIcon("://icons/soundbank_nor.png");
			break;
		}
		return QIcon("://icons/switchgroup_nor.png");
	}

	AudioControls::EACBControlType CAudioSystemEditor_wwise::ImplTypeToATLType(TImplControlType type) const
	{
		switch (type)
		{
		case eWCT_WWISE_EVENT:
			return eACBT_TRIGGER;
			break;
		case eWCT_WWISE_RTPC:
			return eACBT_RTPC;
			break;
		case eWCT_WWISE_SWITCH:
			return eACBT_SWITCH;
			break;
		case eWCT_WWISE_AUX_BUS:
			return eACBT_ENVIRONMENTS;
			break;
		case eWCT_WWISE_SOUND_BANK:
			return eACBT_PRELOADS;
			break;
		}
		return eACBT_NUM_TYPES;
	}

	AudioControls::TImplControlTypeMask CAudioSystemEditor_wwise::GetCompatibleTypes(EACBControlType eATLControlType) const
	{
		switch (eATLControlType)
		{
		case eACBT_TRIGGER:
			return eWCT_WWISE_EVENT;
			break;
		case eACBT_RTPC:
			return eWCT_WWISE_RTPC;
			break;
		case eACBT_SWITCH:
			return (eWCT_WWISE_SWITCH | eWCT_WWISE_RTPC);
			break;
		case eACBT_ENVIRONMENTS:
			return eWCT_WWISE_AUX_BUS;
			break;
		case eACBT_PRELOADS:
			return eWCT_WWISE_SOUND_BANK;
			break;
		}
		return AUDIO_IMPL_INVALID_TYPE;
	}
}