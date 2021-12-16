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
#include "AudioControlsLoader.h"
#include <StringUtils.h>
#include "CryFile.h"
#include "ISystem.h"
#include "ATLControlsModel.h"
#include "CryPath.h"
#include "AudioLibrary.h"
#include "common/IAudioSystemEditor.h"
#include "common/IAudioSystemControl.h"

using namespace PathUtil;

namespace AudioControls
{
	const string CAudioControlsLoader::ms_sControlsPath = "libs/gameaudio/";
	const string CAudioControlsLoader::ms_sLevelsFolder = "levels/";
	const string CAudioControlsLoader::ms_sControlsLevelsFolder = "levels/";
	const string CAudioControlsLoader::ms_sConfigFilePath = "libs/gameaudio/config.xml";

	EACBControlType TagToType(const string& tag)
	{
		if (tag == "ATLSwitch")
		{
			return eACBT_SWITCH;
		}
		else if (tag == "ATLEnvironment")
		{
			return eACBT_ENVIRONMENTS;
		}
		else if (tag == "ATLRtpc")
		{
			return eACBT_RTPC;
		}
		else if (tag == "ATLTrigger")
		{
			return eACBT_TRIGGER;
		}
		else if (tag == "ATLPreloadRequest")
		{
			return eACBT_PRELOADS;
		}
		return eACBT_NUM_TYPES;
	}

	CAudioControlsLoader::CAudioControlsLoader(CATLControlsModel* pATLModel, IAudioSystemEditor* pAudioSystemImpl)
		: m_pModel(pATLModel)
		, m_pAudioSystemImpl(pAudioSystemImpl) {}

	void CAudioControlsLoader::LoadAll()
	{
		LoadScopes();
		LoadControls();
	}

	void CAudioControlsLoader::LoadControls()
	{
		LoadSettings();

		// load the global controls
		LoadAllLibrariesInFolder(ms_sControlsPath, "");

		// load the level specific controls
		_finddata_t fd;
		ICryPak* pCryPak = gEnv->pCryPak;
		intptr_t handle = pCryPak->FindFirst(ms_sControlsPath + ms_sControlsLevelsFolder + "*.*", &fd);
		if (handle != -1)
		{
			do
			{
				if (fd.attrib & _A_SUBDIR)
				{
					string name = fd.name;
					if (name != "." && name != "..")
					{
						LoadAllLibrariesInFolder(ms_sControlsPath, name);
						if (!m_pModel->ScopeExists(fd.name))
						{
							// if the control doesn't exist it
							// means it is not a real level in the
							// project so it is flagged as LocalOnly
							m_pModel->AddScope(fd.name, true);
						}
					}
				}
			}
			while (pCryPak->FindNext(handle, &fd) >= 0);
			pCryPak->FindClose(handle);
		}

		// Set all libraries as NOT modified
		int size = m_pModel->GetLibraryCount();
		for (int i = 0; i < size; ++i)
		{
			CAudioLibrary* pLibrary = m_pModel->GetLibrary(i);
			if (pLibrary)
			{
				pLibrary->SetModified(false);
			}
		}

		CreateDefaultControls();
	}

	void CAudioControlsLoader::LoadSettings()
	{
		XmlNodeRef root = GetISystem()->LoadXmlFromFile(ms_sConfigFilePath);
		if (root)
		{
			string tag = root->getTag();
			if (tag.compare("ACBConfig") == 0)
			{
				int size = root->getChildCount();
				for (int i = 0; i < size; ++i)
				{
					XmlNodeRef child = root->getChild(i);
					if (child)
					{
						string group = child->getAttr("name");
						if (group.compare("") != 0)
						{
							m_pModel->AddConnectionGroup(group);
						}
					}
				}
			}
		}
		else
		{
			// hard code some groups if the config.xml file is missing.
			m_pModel->AddConnectionGroup("default");
			m_pModel->AddConnectionGroup("High");
			m_pModel->AddConnectionGroup("Low");

			m_pModel->AddPlatform("PC");
			m_pModel->AddPlatform("PS4");
			m_pModel->AddPlatform("Xbox");
			m_pModel->AddPlatform("Mac");
			m_pModel->AddPlatform("Linux");
		}
	}

	void CAudioControlsLoader::LoadAllLibrariesInFolder(const string& folderPath, const string& level)
	{
		string path = AddSlash(folderPath);
		if (!level.empty())
		{
			path = AddSlash(path + ms_sControlsLevelsFolder + level);
		}

		string searchPath = path + "*.xml";
		ICryPak* pCryPak = gEnv->pCryPak;
		_finddata_t fd;
		intptr_t handle = pCryPak->FindFirst(searchPath, &fd);
		if (handle != -1)
		{
			do
			{
				string filename = path + fd.name;
				XmlNodeRef root = GetISystem()->LoadXmlFromFile(filename);
				if (root)
				{
					string tag = root->getTag();
					if (tag == "ATLConfig")
					{
						m_loadedFilenames.insert(filename.MakeLowerLocale());
						string file = fd.name;
						if (root->haveAttr("atl_name"))
						{
							file = root->getAttr("atl_name");
						}
						RemoveExtension(file);
						LoadATLLibrary(root, folderPath, level, file);
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "(Audio Controls Browser) Failed parsing game sound file %s", filename);
				}
			}
			while (pCryPak->FindNext(handle, &fd) >= 0);

			pCryPak->FindClose(handle);
		}
	}

	void CAudioControlsLoader::ProcessConnections(XmlNodeRef pRoot, CATLControl* pControl)
	{
		if (pControl)
		{
			const int nSize = pRoot->getChildCount();
			for (int i = 0; i < nSize; ++i)
			{
				XmlNodeRef pNode = pRoot->getChild(i);
				const string sTag = pNode->getTag();
				if (m_pAudioSystemImpl)
				{
					IAudioConnection* pConnection = m_pAudioSystemImpl->CreateConnectionFromXMLNode(pNode);
					if (pConnection)
					{
						pControl->AddConnection(pConnection);
					}
					else
					{
						pControl->m_unknownConnectionNodes[AudioControls::g_sDefaultGroup].push_back(pNode);
					}
				}
			}
		}
	}

	void CAudioControlsLoader::LoadATLLibrary(XmlNodeRef pRoot, const string& sFilepath, const string& sLevel, const string& sFilename)
	{
		if (pRoot)
		{
			const int nControlTypeCount = pRoot->getChildCount();
			for (int i = 0; i < nControlTypeCount; ++i)
			{
				XmlNodeRef pNode = pRoot->getChild(i);
				const int nControlCount = pNode->getChildCount();
				for (int j = 0; j < nControlCount; ++j)
				{
					XmlNodeRef pATLNode = pNode->getChild(j);
					const string sName = pATLNode->getAttr("atl_name");
					const string sPath = pATLNode->getAttr("path");
					EACBControlType type = TagToType(pATLNode->getTag());

					if (type == eACBT_SWITCH)
					{
						const string sSwitchPathName = sPath + "/" + sName;
						const int nStateCount = pATLNode->getChildCount();
						for (int k = 0; k < nStateCount; ++k)
						{
							XmlNodeRef pStateNode = pATLNode->getChild(k);
							const string sStateName = pStateNode->getAttr("atl_name");
							CATLControl* pControl = m_pModel->CreateControlInLibrary(sStateName, type, sFilename, sFilepath, sSwitchPathName);
							if (pControl)
							{
								pControl->SetScope(sLevel);
								ProcessConnections(pStateNode, pControl);
								pControl->SetModified(false);
							}
						}
					}
					else if (type != eACBT_PRELOADS)
					{
						// handle all remaining controls (excluding preloads which have a different format)
						CATLControl* pControl = m_pModel->CreateControlInLibrary(sName, type, sFilename, sFilepath, sPath);
						if (pControl)
						{
							pControl->SetScope(sLevel);
							ProcessConnections(pATLNode, pControl);
							pControl->SetModified(false);
						}
					}
				}
			}
		}

		// Preloads
		ProcessPreloads(pRoot, sFilename, sFilepath, sLevel);
	}

	void CAudioControlsLoader::LoadScopes()
	{
		LoadScopesImpl(ms_sLevelsFolder);
	}

	void CAudioControlsLoader::LoadScopesImpl(const string& sLevelsFolder)
	{
		_finddata_t fd;
		ICryPak* pCryPak = gEnv->pCryPak;
		intptr_t handle = pCryPak->FindFirst(sLevelsFolder + "/*.*", &fd);
		if (handle != -1)
		{
			do
			{
				string name = fd.name;
				if (name != "." && name != ".." && !name.empty())
				{
					if (fd.attrib & _A_SUBDIR)
					{
						LoadScopesImpl(sLevelsFolder + "/" + name);
					}
					else
					{
						string extension = GetExt(name);
						if (extension == "cry")
						{
							RemoveExtension(name);
							m_pModel->AddScope(name);
						}
					}
				}
			}
			while (pCryPak->FindNext(handle, &fd) >= 0);
			pCryPak->FindClose(handle);
		}
	}

	std::set<string> CAudioControlsLoader::GetLoadedFilenamesList()
	{
		return m_loadedFilenames;
	}

	void CAudioControlsLoader::CreateDefaultControls()
	{
		// Load default controls if the don't exist. These controls need to always exist in your project
		std::vector<string> defaultControls;
		defaultControls.push_back("get_focus");
		defaultControls.push_back("lose_focus");
		defaultControls.push_back("mute_all");
		defaultControls.push_back("unmute_all");
		defaultControls.push_back("do_nothing");
		size_t defaultControlCount = defaultControls.size();

		int controlCount = m_pModel->ControlCount();
		for (size_t i = 0; i < defaultControlCount; ++i)
		{
			bool bExists = false;
			for (size_t j = 0; j < controlCount; ++j)
			{
				CATLControl* pControl = m_pModel->GetControlByIndex(j);
				if (pControl && pControl->GetName() == defaultControls[i] && pControl->GetScope() == "")
				{
					bExists = true;
					break;
				}
			}

			if (!bExists)
			{
				CATLControl* pControl = m_pModel->CreateControlInLibrary(defaultControls[i], eACBT_TRIGGER, "default_controls", "libs/gameaudio", "");
				pControl->SetModified(true);
			}
		}
	}

	void CAudioControlsLoader::ProcessPreloads(XmlNodeRef root, const string& filename, const string& filepath, const string& level)
	{
		XmlNodeRef node = root->findChild("AudioPreloads");
		if (node)
		{
			int size = node->getChildCount();
			for (int i = 0; i < size; ++i)
			{
				XmlNodeRef child = node->getChild(i);
				if (child)
				{
					// Create the new control
					string name = child->getAttr("atl_name");
					string type = child->getAttr("atl_type");
					CATLControl* pControl = m_pModel->CreateControlInLibrary(name, eACBT_PRELOADS, filename, filepath, child->getAttr("path"));
					pControl->SetScope(level);
					if (type.compare("AutoLoad") == 0)
					{
						pControl->SetAutoLoad(true);
					}
					else
					{
						pControl->SetAutoLoad(false);
					}

					// Read all the platform definitions for this control
					// <ATLPlatforms>
					XmlNodeRef platformsNode = child->findChild("ATLPlatforms");
					if (platformsNode)
					{
						int numPlatforms = platformsNode->getChildCount();
						for (int j = 0; j < numPlatforms; ++j)
						{
							XmlNodeRef platformNode = platformsNode->getChild(j);
							string platformName = platformNode->getAttr("atl_name");
							string groupName = platformNode->getAttr("atl_config_group_name");
							m_pModel->AddPlatform(platformName);

							int groupId = m_pModel->GetConnectionGroupId(groupName);
							if (groupId >= 0)
							{
								pControl->SetGroupForPlatform(platformName, groupId);
							}
						}
					}

					// Read the connection information for
					// each of the platform groups
					int numChildren = child->getChildCount();
					for (int j = 0; j < numChildren; ++j)
					{
						// <ATLConfigGroup>
						XmlNodeRef groupNode = child->getChild(j);
						string tag = groupNode->getTag();
						if (tag.compare("ATLConfigGroup") != 0)
						{
							continue;
						}
						const string sGroupName = groupNode->getAttr("atl_name");

						const int nNumConnections = groupNode->getChildCount();
						for (int k = 0; k < nNumConnections; ++k)
						{
							XmlNodeRef pConnectionNode = groupNode->getChild(k);
							if (pConnectionNode && m_pAudioSystemImpl)
							{
								IAudioConnection* pAudioConnection = m_pAudioSystemImpl->CreateConnectionFromXMLNode(pConnectionNode);
								if (pAudioConnection)
								{
									pAudioConnection->SetGroup(sGroupName);
									pControl->AddConnection(pAudioConnection);
								}
								else
								{
									pControl->m_unknownConnectionNodes[sGroupName].push_back(pConnectionNode);
								}
							}
						}
					}
					pControl->SetModified(false);
				}
			}
		}
	}
}