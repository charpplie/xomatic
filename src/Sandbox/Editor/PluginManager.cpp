// PluginManager.cpp: implementation of the CPluginManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PluginManager.h"
#include <ISourceControl.h>

#include <io.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CPluginManager::CPluginManager()
{
	m_pIAssociatedPlugin = NULL;
	m_iAssociatedPluginUIID = 0;

	iUIID = 0;
}

CPluginManager::~CPluginManager()
{
	CleanUp();
}

bool CPluginManager::LoadAllPlugins(CString strPath)
{
	//////////////////////////////////////////////////////////////////////
	// Load all plugin DLLs which are in the passed path
	//////////////////////////////////////////////////////////////////////

	CFileEnum cDLLFiles;
	__finddata64_t sFile;
	char szFilePath[_MAX_PATH];
	HMODULE hPlugin = NULL;
	pfnCreatePluginInstance pfnFactory = NULL;
	PLUGIN_INIT_PARAM sInitParam = { GetIEditor(), NULL /*TODO: CEngineSingleton::GetGameInterface()*/ };
	IPlugin *pIPlugin = NULL;
	
	// Terminate old plugins first
	CleanUp();

	CLogFile::WriteLine("Loading plugins...");

	if (!PathFileExists(strPath.GetBuffer(0)))
	{
		CLogFile::FormatLine("Can't find plugin directory '%s'", strPath.GetBuffer(1));
		return false;
	}

	if (cDLLFiles.StartEnumeration(strPath.GetBuffer(1), "*.DLL", &sFile))
	{
		do
		{
			// Construct the full filepath of the current file
			strcpy(szFilePath, strPath.GetBuffer(0));
			PathAddBackslash(szFilePath);
			strcat(szFilePath, sFile.name);

			// Load the plugin's DLL
			hPlugin = LoadLibrary(szFilePath);

			if (!hPlugin)
			{
				CLogFile::FormatLine("Can't load plugin DLL '%s' !", szFilePath);
				continue;
			}

			// Query the factory pointer
			pfnFactory = (pfnCreatePluginInstance) GetProcAddress(hPlugin, "CreatePluginInstance");

			if (!pfnFactory)
			{
				CLogFile::WriteLine("Can't query plugin DLL factory pointer !");
				ASSERT(pfnFactory);
				continue;
			}

			// Create an instance of the plugin 
			pIPlugin = pfnFactory(&sInitParam);

			if (!pIPlugin)
			{
				CLogFile::FormatLine("Can't create instance of plugin DLL '%s' !", szFilePath);
				ASSERT(pfnFactory);
				continue;
			}

		  CString errorMsg;
      if (CheckPluginVersion(pIPlugin, errorMsg) == false)
      {
        if (errorMsg.GetLength())
          MessageBox(NULL, errorMsg, "Plugin loader", MB_OK | MB_ICONERROR);

        pIPlugin->Release();
        continue;
      }

			RegisterPlugin( pIPlugin );

			// Write log string about plugin
			CLogFile::FormatLine("Successfully loaded plugin '%s' version '%i' (GUID: %s)",
				pIPlugin->GetPluginName(), pIPlugin->GetPluginVersion(), pIPlugin->GetPluginGUID());

		} while (cDLLFiles.GetNextFile(&sFile));
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CPluginManager::RegisterPlugin( IPlugin *pIPlugin )
{
	// Push the plugin on the list
	m_lPlugins.push_back(pIPlugin);

	// Store the UI ID - Plugin association in the map
	m_mUIIDPluginMap[iUIID] = pIPlugin;

	// Next UIID
	iUIID++;
}

//////////////////////////////////////////////////////////////////////////
void CPluginManager::CleanUp()
{
	//////////////////////////////////////////////////////////////////////
	// Unloads all plugins without asking for unsaved data etc.
	//////////////////////////////////////////////////////////////////////

	PluginIt it;

	CLogFile::WriteLine("Unloading all previous plugins");

	// Release the plugins
	for (it=m_lPlugins.begin(); it!=m_lPlugins.end(); it++)
		(* it)->Release();

	// Clear the list of plugins
	m_lPlugins.clear();

	// Clear the map of plugin event maps
	m_mPluginEventMap.clear();

	// Clear the map of UI IDs and plugins
	m_mUIIDPluginMap.clear();
}

void CPluginManager::NewDocument()
{
}

IPlugin * CPluginManager::GetPluginByGUID(const char * pszGUID)
{
	//////////////////////////////////////////////////////////////////////
	// Try to find a plugin which the passed GUID
	//////////////////////////////////////////////////////////////////////

	PluginIt it;

	for (it=m_lPlugins.begin(); it!=m_lPlugins.end(); it++)
	{
		const char *sGuid = (* it)->GetPluginGUID();

		if (sGuid && strcmp(sGuid, pszGUID) == 0)
			return (* it);
	}

	return NULL;
}

IPlugin * CPluginManager::GetPluginByUIID(uint8 iUserInterfaceID)
{
	//////////////////////////////////////////////////////////////////////
	// Try to find the plugin which has the passed user interface ID
	//////////////////////////////////////////////////////////////////////

	UIIDPluginIt it;

	// Look up the plugin pointer from the list
	it = m_mUIIDPluginMap.find(iUserInterfaceID);

	// Found ?
	if (it == m_mUIIDPluginMap.end())
		return NULL;

	return (* it).second;
}

IUIEvent * CPluginManager::GetEventByIDAndPluginID(uint8 iPluginID, uint8 iEvtID)
{
	//////////////////////////////////////////////////////////////////////
	// Return the event interface of a user interface element which is
	// specified by its ID and the user interface ID of the plugin which
	// created the UI element
	//////////////////////////////////////////////////////////////////////

	IPlugin * pIPlugin = NULL;
	EventHandlerIt EventIt;
	PluginEventIt PluginIt;

	// Try to get the plugin pointer
	pIPlugin = GetPluginByUIID(iPluginID);

	// Failed ?
	if (!pIPlugin)
		return NULL;

	// Try to get the event map for the plugin
	PluginIt = m_mPluginEventMap.find(pIPlugin);

	// Failed ?
	if (PluginIt == m_mPluginEventMap.end())
		return NULL;

	// Try to get the event handler for the passed event ID
	EventIt = (* PluginIt).second.find(iEvtID);

	// Failed ?
	if (EventIt == (* PluginIt).second.end())
		return NULL;

	return (* EventIt).second;
}

bool CPluginManager::CanAllPluginsExitNow()
{
	//////////////////////////////////////////////////////////////////////
	// Calls the CanExitNow() function of all plugins
	//////////////////////////////////////////////////////////////////////

	PluginIt it;

	for (it=m_lPlugins.begin(); it!=m_lPlugins.end(); it++)
	{
		if (!(* it)->CanExitNow())
			return false;
	}

	return true;
}

bool CPluginManager::CallExport(const char * pszGamePath)
{
	//////////////////////////////////////////////////////////////////////
	// Call the export function for all plugins
	//////////////////////////////////////////////////////////////////////

	return true;
}

void CPluginManager::AddHandlerForCmdID(IPlugin *pIPluginOfHandler, uint8 iCmdID, IUIEvent *pIEvt)
{
	//////////////////////////////////////////////////////////////////////
	// Associates the UI element ID dwCmdID of the plugin 
	// pIPluginOfHandler with the event handler pIEvt
	//////////////////////////////////////////////////////////////////////

	(m_mPluginEventMap[pIPluginOfHandler])[iCmdID] = pIEvt;
}

void CPluginManager::SetAssociatedPlugin(IPlugin * pIPlugin,uint8 iUIID)
{
	m_pIAssociatedPlugin = pIPlugin;
	m_iAssociatedPluginUIID = iUIID;
};

bool CPluginManager::CheckPluginVersion(IPlugin* pIPlugin, CString& errorMsg)
{
  const char* pluginName = pIPlugin->GetPluginName();

  if (strcmp(pluginName, PERFORCE_PLUGIN_NAME) == 0)
  {
    if (pIPlugin->GetPluginVersion() != PERFORCE_PLUGIN_CURRENT_VERSION)
    {
      errorMsg = "Perforce plug-in is out of date and will not be loaded. Please update.";
      return false;
    }
  }

  return true;
}
