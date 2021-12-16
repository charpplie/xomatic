////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   AudioControlsWriter.cpp
//  Created:     12/05/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AudioControlsWriter.h"
#include <StringUtils.h>
#include <CryFile.h>
#include <ISystem.h>
#include "ATLControlsModel.h"
#include <ISourceControl.h>
#include <IEditor.h>
#include "AudioLibrary.h"
#include "common/IAudioSystemEditor.h"
#include "common/IAudioSystemControl.h"

#define LEVEL_AUDIO_FOLDER "levels"

using namespace PathUtil;

namespace AudioControls
{
	//-------------------------------------
	CAudioControlsWriter::CAudioControlsWriter(CATLControlsModel& ATLModel, IAudioSystemEditor* pAudioSystemImpl, std::set<string>& loadedFilenames)
		: m_ATLModel(ATLModel), m_pAudioSystemImpl(pAudioSystemImpl)
	{

		std::set<string> writtenFilenames;

		int size = m_ATLModel.GetLibraryCount();
		for (int i = 0; i < size; ++i)
		{
			CAudioLibrary* pLibrary = m_ATLModel.GetLibrary(i);
			if (pLibrary)
			{
				std::map <string, std::vector<CATLControl*> > controlsByFilename;
				int controlCount = pLibrary->GetControlCount();
				for (int j = 0; j < controlCount; ++j)
				{
					CATLControl* pControl = pLibrary->GetControl(j);
					if (pControl)
					{
						string scope = pControl->GetScope();
						string filepath = AddSlash(pControl->GetFilepath());
						if (scope != "")
						{
							string levelPath = AddSlash(LEVEL_AUDIO_FOLDER) + scope;
							filepath = filepath + AddSlash(levelPath);
						}
						filepath = filepath + pLibrary->GetName();
						PathUtil::RemoveExtension(filepath);
						filepath = filepath + ".xml";
						filepath.MakeLowerLocale();

						controlsByFilename[filepath].push_back(pControl);
						writtenFilenames.insert(filepath);
					}
				}

				if (pLibrary->IsModified())
				{

					for (auto it = controlsByFilename.begin(); it != controlsByFilename.end(); ++it)
					{
						std::vector<CATLControl*>& controls = it->second;

						XmlNodeRef rtpcNode = GetISystem()->CreateXmlNode("AudioRTPCs");
						XmlNodeRef switchesNode = GetISystem()->CreateXmlNode("AudioSwitches");
						XmlNodeRef triggerNode = GetISystem()->CreateXmlNode("AudioTriggers");
						XmlNodeRef environmentsNode = GetISystem()->CreateXmlNode("AudioEnvironments");
						XmlNodeRef preloadsNode = GetISystem()->CreateXmlNode("AudioPreloads");

						std::map<string, std::vector<CATLControl*>> switches;
						size_t controlCount = controls.size();
						for (int j = 0; j < controlCount; ++j)
						{
							CATLControl* pControl = controls[j];
							if (pControl)
							{
								switch (pControl->GetType())
								{
								case eACBT_RTPC:
									AddControlToNode(rtpcNode, pControl, "ATLRtpc");
									break;
								case eACBT_SWITCH:
									switches[pControl->GetVirtualPath()].push_back(pControl);
									break;
								case eACBT_TRIGGER:
									AddControlToNode(triggerNode, pControl, "ATLTrigger");
									break;
								case eACBT_ENVIRONMENTS:
									AddControlToNode(environmentsNode, pControl, "ATLEnvironment");
									break;
								case eACBT_PRELOADS:
									ProcessPreloads(preloadsNode, pControl);
									break;
								}
								pControl->SetModified(false);
							}
						}

						ProcessSwitches(switches, switchesNode);

						XmlNodeRef filenode = GetISystem()->CreateXmlNode("ATLConfig");
						filenode->setAttr("atl_name", pLibrary->GetName());
						if (rtpcNode->getChildCount() > 0)
						{
							filenode->addChild(rtpcNode);
						}
						if (switchesNode->getChildCount() > 0)
						{
							filenode->addChild(switchesNode);
						}
						if (triggerNode->getChildCount() > 0)
						{
							filenode->addChild(triggerNode);
						}
						if (environmentsNode->getChildCount() > 0)
						{
							filenode->addChild(environmentsNode);
						}
						if (preloadsNode->getChildCount() > 0)
						{
							filenode->addChild(preloadsNode);
						}

						string filepath = it->first;
						string fullFilePath = PathUtil::GetGameFolder() + "/" + filepath;

						DWORD fileAttributes = GetFileAttributesA(fullFilePath.c_str());
						if (fileAttributes & FILE_ATTRIBUTE_READONLY)
						{
							// file is read-only
							SetFileAttributesA(fullFilePath.c_str(), FILE_ATTRIBUTE_NORMAL);
						}
						filenode->saveToFile(filepath);

						CheckOutFile(filepath);
					}
					pLibrary->SetModified(false);
				}
			}
		}

		std::set<string> librariesToDelete;
		std::set_difference(loadedFilenames.begin(), loadedFilenames.end(), writtenFilenames.begin(), writtenFilenames.end(),
		                    std::inserter(librariesToDelete, librariesToDelete.begin()));

		for (auto it = librariesToDelete.begin(); it != librariesToDelete.end(); ++it)
		{
			string fullFilePath = PathUtil::GetGameFolder() + "/" + *it;
			DeleteLibraryFile(fullFilePath);
		}

		loadedFilenames = writtenFilenames;

	}

	//-------------------------------------
	void CAudioControlsWriter::AddControlToNode(XmlNodeRef node, CATLControl* pControl, const string& tag)
	{
		XmlNodeRef childNode = node->createNode(tag);
		childNode->setAttr("atl_name", pControl->GetName());
		if (pControl->GetVirtualPath() != "")
		{
			childNode->setAttr("path", pControl->GetVirtualPath());
		}
		node->addChild(childNode);
		AddConnections(childNode, pControl);
	}

	//-------------------------------------
	void CAudioControlsWriter::ProcessSwitches(std::map<string, std::vector<CATLControl*>>& switches, XmlNodeRef switchesNode)
	{
		if (m_pAudioSystemImpl)
		{
			for (auto switchesIt = switches.begin(); switchesIt != switches.end(); ++switchesIt)
			{
				// get the switch and the state name
				string path = switchesIt->first;

				string switchName = path;
				string finalPath = "";
				string::size_type pos = path.find_last_of("/");
				if (pos != string::npos)
				{
					switchName = path.substr(pos + 1);
					finalPath = path.substr(0, path.size() - switchName.size() - 1);
				}

				XmlNodeRef switchNode = switchesNode->createNode("ATLSwitch");
				switchNode->setAttr("atl_name", switchName);
				if (!finalPath.empty())
				{
					switchNode->setAttr("path", finalPath);
				}

				std::vector<CATLControl*>& states = switchesIt->second;
				const size_t stateCount = states.size();
				for (size_t i = 0; i < stateCount; ++i)
				{
					CATLControl* pControl = states[i];
					if (pControl)
					{
						XmlNodeRef pStateNode = switchNode->createNode("ATLSwitchState");
						pStateNode->setAttr("atl_name", pControl->GetName());

						const int nConnectionCount = pControl->ConnectionCount();
						for (int j = 0; j < nConnectionCount; ++j)
						{
							IAudioConnection* pConnection = pControl->GetConnectionAt(j);
							if (pConnection)
							{
								m_pAudioSystemImpl->WriteConnectionToXMLNode(pStateNode, pConnection, eACBT_SWITCH);
							}
						}

						auto it = pControl->m_unknownConnectionNodes.begin();
						auto end = pControl->m_unknownConnectionNodes.end();
						for (; it != end; ++it)
						{
							auto connectionIt = it->second.begin();
							auto connectionEnd = it->second.end();
							for (; connectionIt != connectionEnd; ++connectionIt)
							{
								pStateNode->addChild(*connectionIt);
							}
						}

						switchNode->addChild(pStateNode);
					}
				}
				switchesNode->addChild(switchNode);
			}
		}
	}

	//-------------------------------------
	void CAudioControlsWriter::AddConnections(XmlNodeRef pNode, CATLControl* pControl)
	{
		if (pControl && m_pAudioSystemImpl)
		{
			int size = pControl->ConnectionCount();
			for (int i = 0; i < size; ++i)
			{
				IAudioConnection* pConnection = pControl->GetConnectionAt(i);
				if (pConnection)
				{
					m_pAudioSystemImpl->WriteConnectionToXMLNode(pNode, pConnection, pControl->GetType());
				}
			}

			auto it = pControl->m_unknownConnectionNodes.begin();
			auto end = pControl->m_unknownConnectionNodes.end();
			for (; it != end; ++it)
			{
				auto connectionIt = it->second.begin();
				auto connectionEnd = it->second.end();
				for (; connectionIt != connectionEnd; ++connectionIt)
				{
					pNode->addChild(*connectionIt);
				}
			}
		}
	}

	//-------------------------------------
	void CAudioControlsWriter::CheckOutFile(const string& filepath)
	{
		ISourceControl* pSourceControl = GetIEditor()->GetSourceControl();
		if (pSourceControl)
		{
			uint32 fileAttributes = pSourceControl->GetFileAttributes(filepath.c_str());
			if (fileAttributes & SCC_FILE_ATTRIBUTE_MANAGED)
			{
				pSourceControl->CheckOut(filepath);
			}
			else if ((fileAttributes == SCC_FILE_ATTRIBUTE_INVALID) || (fileAttributes & SCC_FILE_ATTRIBUTE_NORMAL))
			{
				pSourceControl->Add(filepath, "(ACB Changelist)", ADD_WITHOUT_SUBMIT | ADD_CHANGELIST);
			}
		}
	}

	//-------------------------------------
	void CAudioControlsWriter::DeleteLibraryFile(const string& filepath)
	{
		ISourceControl* pSourceControl = GetIEditor()->GetSourceControl();
		if (pSourceControl && pSourceControl->GetFileAttributes(filepath.c_str()) & SCC_FILE_ATTRIBUTE_MANAGED)
		{
			// if source control is connected, let it handle the delete
			pSourceControl->Delete(filepath, "(ACB Changelist)", DELETE_WITHOUT_SUBMIT | ADD_CHANGELIST);
			DeleteFile(filepath.c_str());
		}
		else
		{
			DWORD fileAttributes = GetFileAttributesA(filepath.c_str());
			if (fileAttributes == INVALID_FILE_ATTRIBUTES || !DeleteFile(filepath.c_str()))
			{
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Failed to delete file %s", filepath);
			}
		}
	}

	//-------------------------------------
	void CAudioControlsWriter::ProcessPreloads(XmlNodeRef preloadsNode, CATLControl* pControl)
	{
		if (m_pAudioSystemImpl)
		{
			XmlNodeRef pNode = preloadsNode->createNode("ATLPreloadRequest");
			pNode->setAttr("atl_name", pControl->GetName());
			if (pControl->IsAutoLoad())
			{
				pNode->setAttr("atl_type", "AutoLoad");
			}

			if (pControl->GetVirtualPath() != "")
			{
				pNode->setAttr("path", pControl->GetVirtualPath());
			}

			// Platforms
			XmlNodeRef platformsNode = pNode->createNode("ATLPlatforms");
			uint numPlatforms = m_ATLModel.GetPlatformCount();
			for (uint j = 0; j < numPlatforms; ++j)
			{
				XmlNodeRef platform = platformsNode->createNode("Platform");
				string platformName = m_ATLModel.GetPlatformAt(j);
				platform->setAttr("atl_name", platformName);
				platform->setAttr("atl_config_group_name", m_ATLModel.GetConnectionGroupAt(pControl->GetGroupForPlatform(platformName)));
				platformsNode->addChild(platform);
			}
			pNode->addChild(platformsNode);

			// Platform group connections
			uint numGroups = m_ATLModel.GetConnectionGroupCount();
			for (uint j = 0; j < numGroups; ++j)
			{
				XmlNodeRef pGroupNode = pNode->createNode("ATLConfigGroup");
				string groupName = m_ATLModel.GetConnectionGroupAt(j);
				pGroupNode->setAttr("atl_name", groupName);

				bool bGroupHasConnections = false;
				int connectionCount = pControl->ConnectionCount();
				for (int k = 0; k < connectionCount; ++k)
				{
					IAudioConnection* pConnection = pControl->GetConnectionAt(k);
					if (pConnection)
					{
						string connectionGroup = pConnection->GetGroup();
						if (connectionGroup.compare(groupName) == 0)
						{
							m_pAudioSystemImpl->WriteConnectionToXMLNode(pGroupNode, pConnection, eACBT_PRELOADS);
						}
					}
				}

				std::vector<XmlNodeRef>& connectionList = pControl->m_unknownConnectionNodes[groupName];
				auto connectionIt = connectionList.begin();
				auto connectionEnd = connectionList.end();
				for (; connectionIt != connectionEnd; ++connectionIt)
				{
					pGroupNode->addChild(*connectionIt);
				}

				if (pGroupNode->getChildCount() > 0)
				{
					pNode->addChild(pGroupNode);
				}
			}
			preloadsNode->addChild(pNode);
		}
	}
}