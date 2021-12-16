////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   AudioControlsLoader.cpp
//  Created:     06/05/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AudioWwiseLoader.h"
#include "CryFile.h"
#include "ISystem.h"
#include "CryPath.h"
#include "IAudioSystemEditor.h"
#include <IAudioSystemControl.h>


using namespace PathUtil;

namespace AudioControls
{
	//-------------------------------------
	CAudioWwiseLoader::CAudioWwiseLoader(IAudioSystemEditor* pAudioSystemImpl)
		: m_pAudioSystemImpl(pAudioSystemImpl)
	{
		LoadControlsInFolder("sounds/wwise_project/Game Parameters");				// rtpc
		LoadControlsInFolder("sounds/wwise_project/States");							// switches
		LoadControlsInFolder("sounds/wwise_project/Switches");							// switches
		LoadControlsInFolder("sounds/wwise_project/Events");							// triggers
		LoadControlsInFolder("sounds/wwise_project/Master-Mixer Hierarchy");		// environments

		// Sound banks are not in .xml files, they're just files in a specific folder
		LoadSoundBanks("sounds/wwise", false);
	}

	void CAudioWwiseLoader::LoadSoundBanks(const string& folderPath, bool bLocalised)
	{
		_finddata_t fd;
		ICryPak* pCryPak = gEnv->pCryPak;
		intptr_t handle = pCryPak->FindFirst(folderPath + GetSlash() + "*.*", &fd);
		if (handle != -1)
		{
			bool bLocalisedLoaded = bLocalised;
			const string ignoreFilename = "Init.bnk";
			do
			{
				string name = fd.name;
				if (name != "." && name != ".." && !name.empty())
				{
					if (fd.attrib & _A_SUBDIR)
					{
						if (!bLocalisedLoaded)
						{
							// each sub-folder represents a different language, 
							// we load only one as all of them should have the
							// same content (in the future we want to have a
							// consistency report to highlight if this is not the case)
							LoadSoundBanks(AddSlash(folderPath) + name, true);
							bLocalisedLoaded = true;
						}
					}
					else if (name.find(".bnk") != string::npos && name.compareNoCase(ignoreFilename) != 0)
					{
						IAudioSystemControl* pControl = m_pAudioSystemImpl->CreateControl(name, eWCT_WWISE_SOUND_BANK);
						if (pControl)
						{
							pControl->SetLocalised(bLocalised);
						}
					}
				}
			}
			while (pCryPak->FindNext(handle, &fd) >= 0);
			pCryPak->FindClose(handle);
		}
	}

	//-------------------------------------
	void CAudioWwiseLoader::LoadControlsInFolder(const string& folderPath)
	{
		_finddata_t fd;
		ICryPak* pCryPak = gEnv->pCryPak;
		intptr_t handle = pCryPak->FindFirst(folderPath + "/*.wwu", &fd);
		if (handle != -1)
		{
			do
			{
				string filename = folderPath + GetSlash() + fd.name;
				XmlNodeRef root = GetISystem()->LoadXmlFromFile(filename);
				if (root)
				{
					LoadControl(root);
				}
			}
			while (pCryPak->FindNext(handle, &fd) >= 0);
			pCryPak->FindClose(handle);
		}
	}

	//-------------------------------------
	void CAudioWwiseLoader::ExtractControlsFromXML(XmlNodeRef root, EWwiseControlTypes type, const string& controlTag, const string& controlNameAttribute)
	{
		string xmlTag = root->getTag();
		if (xmlTag.compare(controlTag) == 0)
		{
			string name = root->getAttr(controlNameAttribute);
			m_pAudioSystemImpl->CreateControl(name, type);
		}
	}

	//-------------------------------------
	void CAudioWwiseLoader::LoadControl(XmlNodeRef root)
	{
		if (root)
		{
			ExtractControlsFromXML(root, eWCT_WWISE_RTPC, "GameParameter", "Name");
			ExtractControlsFromXML(root, eWCT_WWISE_EVENT, "Event", "Name");
			ExtractControlsFromXML(root, eWCT_WWISE_AUX_BUS, "AuxBus", "Name");

			// special case for switches
			string tag = root->getTag();
			bool bIsSwitch = tag.compare("SwitchGroup") == 0;
			bool bIsState = tag.compare("StateGroup") == 0;
			if (bIsSwitch || bIsState)
			{
				string parent = root->getAttr("Name");
				XmlNodeRef children = root->findChild("ChildrenList");
				if (children)
				{
					int size = children->getChildCount();
					for (int i = 0; i < size; ++i)
					{
						XmlNodeRef child = children->getChild(i);
						if (child)
						{
							string name = child->getAttr("Name");
							IAudioSystemControl* pControl = m_pAudioSystemImpl->CreateControl(name, eWCT_WWISE_SWITCH);
							if (pControl)
							{
								pControl->m_controlTag = bIsSwitch ? "WwiseSwitch" : "WwiseState";
								pControl->SetVirtualPath(parent);
							}
						}
					}
				}
			}

			int size = root->getChildCount();
			for (int i = 0; i < size; ++i)
			{
				LoadControl(root->getChild(i));
			}
		}
	}
}