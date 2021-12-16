#include "stdafx.h"
#include "SettingsManager.h"
#include "SettingsManagerDialog.h"
#include "ToolBox.h"

#define TOOLBOX_FILE "Editor/ToolBox.xml"
#define TOOLBOX_NODE "ToolBox"

CSettingsManager::CSettingsManager(EditorSettingsManagerType managerType)
{
	if (managerType<eSettingsManagerLast)
		m_managerType=managerType;
	else
		managerType=eSettingsManagerMemoryStorage;

	if (m_managerType==eSettingsManagerMemoryStorage)
		m_pSettingsManagerMemoryNode = XmlHelpers::CreateXmlNode(EDITOR_SETTINGS_ROOT_NODE);

	// Main frame layouts must be processed as first
	AddToolName(_T(MAINFRM_LAYOUT_NORMAL),_T("Sandbox Layout"));
	AddToolName(_T(MAINFRM_LAYOUT_PREVIEW),_T("Sandbox Preview Layout"));

	CreateDefaultLayoutSettingsFile();

	CTime theTime = CTime::GetCurrentTime();
	SaveLogEventSetting("EditorStart","time", theTime.Format(LOG_DATETIME_FORMAT) );

}

CSettingsManager::~CSettingsManager()
{
}

void CSettingsManager::SaveLayoutSettings(CXTPDockingPaneLayout &layout, LPCSTR toolName )
{
	IXMLDOMDocumentPtr domDocument = NULL;

	COleVariant xmlFile(EDITOR_LAYOUT_FILE_PATH);
	CComBSTR editorRootNodeStr(EDITOR_SETTINGS_ROOT_NODE);
	CComBSTR rootNodeStr(EDITOR_LAYOUT_ROOT_NODE);
	CComBSTR dockingPanelNodeName(EDITOR_LAYOUT_NODE);
	IXMLDOMNode *pRootNode = NULL;
	IXMLDOMNode *pEditorRootNode = NULL;

	if (!OpenEditorSettingsFile(domDocument))
	{
		domDocument==NULL;
		return;
	}

	IXMLDOMNode *pDockingLayoutNode= NULL;
	
	HRESULT hr = domDocument->selectSingleNode(editorRootNodeStr ,&pEditorRootNode);

	if (FAILED(hr))
	{
		pDockingLayoutNode=NULL;
		domDocument=NULL;
		return;
	}

	hr = pEditorRootNode->selectSingleNode(rootNodeStr,&pRootNode);

	if (FAILED(hr) || !pRootNode)
	{
		domDocument=NULL;
		pEditorRootNode=NULL;
		domDocument==NULL;
		return;
	}

	hr = pRootNode->selectSingleNode(dockingPanelNodeName,&pDockingLayoutNode);

	if (FAILED(hr) || !pDockingLayoutNode)
	{
		domDocument=NULL;
		pEditorRootNode=NULL;
		pRootNode=NULL;
		pDockingLayoutNode=NULL;
		return;
	}

	domDocument->put_async(VARIANT_FALSE);

	layout.SaveToNode(CXTPDOMNodePtr(pDockingLayoutNode),toolName);

	if (m_managerType==eSettingsManagerMemoryStorage)
	{
		_bstr_t xmlString("");
		BSTR xmlTmpString = NULL;

		pRootNode->get_xml(&xmlTmpString);

		if (SysStringLen(xmlTmpString) > 0 )
			xmlString += xmlTmpString;

		SysFreeString(xmlTmpString);
		xmlTmpString=NULL;

		domDocument=NULL;
		pEditorRootNode=NULL;
		pRootNode=NULL;
		pDockingLayoutNode=NULL;
		domDocument = NULL;

		XmlNodeRef rootLayoutNode = m_pSettingsManagerMemoryNode->findChild(EDITOR_LAYOUT_ROOT_NODE);
		if (rootLayoutNode)
		{
			XmlNodeRef xmlDockingLayoutNode = rootLayoutNode->findChild(EDITOR_LAYOUT_NODE);

			if (!xmlDockingLayoutNode)
				return;

			XmlNodeRef newToolNode = NULL;
			XmlNodeRef toolNode= xmlDockingLayoutNode->findChild(toolName);

			if (toolNode)
			{
				xmlDockingLayoutNode->removeChild(toolNode);

				newToolNode = XmlHelpers::LoadXmlFromBuffer((const char*)xmlString, strlen((const char*)xmlString));
				XmlNodeRef newXmlDockingLayoutNode = newToolNode->findChild(EDITOR_LAYOUT_NODE);

				if (!newXmlDockingLayoutNode)
					return;

				toolNode = newXmlDockingLayoutNode->findChild(toolName);

				if (!toolNode)
					return;

				xmlDockingLayoutNode->addChild(toolNode);
			}
			else
			{
				newToolNode = XmlHelpers::LoadXmlFromBuffer((const char*)xmlString, strlen((const char*)xmlString));
				XmlNodeRef newXmlDockingLayoutNode = newToolNode->findChild(EDITOR_LAYOUT_NODE);

				if (!newXmlDockingLayoutNode)
					return;

				toolNode = newXmlDockingLayoutNode->findChild(toolName);

				if (!toolNode)
					return;

				xmlDockingLayoutNode->addChild(toolNode);
			}
		}
	}
}

bool CSettingsManager::LoadLayoutSettings(CXTPDockingPaneLayout &layout, const LPCSTR &toolName )
{
	IXMLDOMDocumentPtr domDocument = NULL;

	CComBSTR dockingPanelNodeName(EDITOR_LAYOUT_NODE);
	CComBSTR rootNodeStr(EDITOR_SETTINGS_ROOT_NODE);
	CComBSTR rootLayoutNodeStr(EDITOR_LAYOUT_ROOT_NODE);
	IXMLDOMNode *pRootLayoutNode = NULL;

	if (!OpenEditorSettingsFile(domDocument))
		return false;

	IXMLDOMNode *pDockingLayoutNode = NULL;

	domDocument->selectSingleNode(rootNodeStr,&pRootLayoutNode);

	if (!pRootLayoutNode)
	{
		domDocument = NULL;
		return false;
	}

	pRootLayoutNode->selectSingleNode(rootLayoutNodeStr,&pRootLayoutNode);

	if (!pRootLayoutNode)
	{
		domDocument = NULL;
		return false;
	}

	pRootLayoutNode->selectSingleNode(dockingPanelNodeName,&pDockingLayoutNode);

	if (!pDockingLayoutNode)
	{
		pRootLayoutNode= NULL;
		domDocument = NULL;
		return false;
	}

	bool res = layout.LoadFromNode(CXTPDOMNodePtr(pDockingLayoutNode),toolName);

	pDockingLayoutNode=NULL;
	pRootLayoutNode=NULL;
	domDocument= NULL;

	return res;
}

bool CSettingsManager::OpenEditorSettingsFile(IXMLDOMDocumentPtr &domDocument)
{
	::CoInitialize(NULL);

	HRESULT hr = domDocument.CreateInstance(CLSID_DOMDocument);
	if (FAILED(hr) || domDocument==NULL)
		return false;

	VARIANT_BOOL isSuccessful=false;
	COleVariant xmlFile(EDITOR_LAYOUT_FILE_PATH);

    domDocument->put_async(VARIANT_FALSE);
	domDocument->put_preserveWhiteSpace(false);

	CString xmlContent = m_pSettingsManagerMemoryNode->getXML(0);
	BSTR tmpStr = xmlContent.AllocSysString();
	_bstr_t xmlString("");

	if (SysStringLen(tmpStr) > 0 )
		xmlString += tmpStr;

	domDocument->loadXML(xmlString,&isSuccessful);

	SysFreeString(tmpStr);
	tmpStr=NULL;

	if (!isSuccessful)
	{
		CreateDefaultLayoutSettingsFile();

		_bstr_t xmlStr2(m_pSettingsManagerMemoryNode->getXML(0));
		domDocument->loadXML(xmlStr2,&isSuccessful);

		if (!isSuccessful)
			return false;
	}

	return true;
}

bool CSettingsManager::CreateDefaultLayoutSettingsFile()
{
	XmlNodeRef pLayoutRootNode = XmlHelpers::CreateXmlNode(EDITOR_LAYOUT_ROOT_NODE);
	XmlNodeRef pEditorLayoutNode = XmlHelpers::CreateXmlNode(EDITOR_LAYOUT_NODE);

	pLayoutRootNode->addChild(pEditorLayoutNode);

	if (m_pSettingsManagerMemoryNode->findChild(EDITOR_LAYOUT_ROOT_NODE))
		m_pSettingsManagerMemoryNode->removeChild(m_pSettingsManagerMemoryNode->findChild(EDITOR_LAYOUT_ROOT_NODE));

		m_pSettingsManagerMemoryNode->addChild(pLayoutRootNode);

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CSettingsManager::SaveSetting(const CString &path, const CString &attr,const CString &val)
{
	std::vector<CString> strNodes;

	CString fixedPath=path;

	// Make sure it ends with "\" before extraction of tokens
	CString slashChar = fixedPath.Right(fixedPath.GetLength()-1);

	if (slashChar!="\\")
		fixedPath +="\\";

	Path::GetDirectoryQueue(fixedPath,strNodes);

	CString writeAttr=attr;

	// Spaces in node names not allowed
	writeAttr.Replace(" ","");

	XmlNodeRef root = NULL;

	root = m_pSettingsManagerMemoryNode;
	
	XmlNodeRef tmpNode = root->findChild(EDITOR_SETTINGS_CONTENT_NODE);
	if (!tmpNode)
	{
		root->addChild(root->createNode(EDITOR_SETTINGS_CONTENT_NODE));
		tmpNode = root->findChild(EDITOR_SETTINGS_CONTENT_NODE);

		if (!tmpNode)
			return;
	}

	for (int i=0; i<strNodes.size(); ++i )
	{
		if (!tmpNode->findChild(strNodes[i]))
		{
			tmpNode->addChild(tmpNode->createNode(strNodes[i]));
			tmpNode = tmpNode->findChild(strNodes[i]);
		}
		else
			tmpNode = tmpNode->findChild(strNodes[i]);
	}

	if (!tmpNode->findChild(writeAttr))
	{
		tmpNode->addChild(tmpNode->createNode(writeAttr));
		tmpNode = tmpNode->findChild(writeAttr);
	}
	else
		tmpNode = tmpNode->findChild(writeAttr);

	if (!tmpNode)
		return;

	tmpNode->setAttr(EDITOR_SETTINGS_ATTRIB_NAME,val);
}

void CSettingsManager::SaveSetting(const CString &path, const CString &attr,bool bVal)
{
	CString str("");
	str.Format( "%d",bVal );
	SaveSetting(path,attr,str);
}

void CSettingsManager::SaveSetting(const CString &path, const CString &attr,float fVal)
{
	CString str("");
	str.Format( "%g",fVal );
	SaveSetting(path,attr,str);
}

void CSettingsManager::SaveSetting(const CString &path, const CString &attr,int iVal)
{
	CString str("");
	str.Format( "%d",iVal);
	SaveSetting(path,attr,str);
}

void CSettingsManager::SaveSetting(const CString &path, const CString &attr,COLORREF color)
{
	CString str;
	str.Format("%u",color);
	SaveSetting(path,attr,str);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSettingsManager::LoadSetting(const CString &path, const CString &attr,CString &val)
{
	// Make sure it ends with "\" before extraction of tokens
	CString fixedPath=path;
	CString slashChar = fixedPath.Right(fixedPath.GetLength()-1);

	if (slashChar!="\\")
		fixedPath +="\\";

	std::vector<CString> strNodes;
	Path::GetDirectoryQueue(fixedPath,strNodes);
	CString readAttr=attr;

	// Spaces in node names not allowed
	readAttr.Replace(" ","");

	XmlNodeRef root = NULL;

	root = m_pSettingsManagerMemoryNode;

	XmlNodeRef tmpNode = NULL;
		
	if (NeedSettingsNode(path))
		tmpNode = root->findChild(EDITOR_SETTINGS_CONTENT_NODE);
	else
		tmpNode = root;

	if (!tmpNode)
		return 0;

	for (int i=0; i<strNodes.size(); ++i )
	{
		if (tmpNode->findChild(strNodes[i]))
			tmpNode = tmpNode->findChild(strNodes[i]);
		else
			return 0;
	}

	if (!tmpNode->findChild(readAttr))
		return 0;
	else
		tmpNode = tmpNode->findChild(readAttr);

	if (tmpNode->haveAttr(EDITOR_SETTINGS_ATTRIB_NAME))
		tmpNode->getAttr(EDITOR_SETTINGS_ATTRIB_NAME,val);

	return tmpNode;

}

void CSettingsManager::LoadSetting(const CString &path, const CString &attr,bool &bVal)
{
	CString defaultVal("");	
	defaultVal.Format("%d",bVal);
	LoadSetting(path,attr,defaultVal);
	bVal = atoi(defaultVal);
}

void CSettingsManager::LoadSetting(const CString &path, const CString &attr,int &iVal)
{
	CString defaultVal("");	
	defaultVal.Format("%d",iVal);
	LoadSetting(path,attr,defaultVal);
	iVal = atoi(defaultVal);
}

void CSettingsManager::LoadSetting(const CString &path, const CString &attr,float &fVal)
{
	CString defaultVal("");
	defaultVal.Format( "%g",fVal);
	LoadSetting(path,attr,defaultVal);
	fVal = atof(defaultVal);
}

void CSettingsManager::LoadSetting(const CString &path, const CString &attr,COLORREF &val)
{
	CString defaultVal("");
	defaultVal.Format( "%u",val);
	LoadSetting(path,attr,defaultVal);
	val = strtoul(defaultVal,NULL,0);
}

void CSettingsManager::AddToolVersion(CString toolName,CString toolVersion)
{
	if (toolName.IsEmpty())
		return;

	if ( stl::find_in_map(m_toolNames,toolName,NULL)=="")
	{
		if (!toolVersion.IsEmpty())
			m_toolVersions[toolName]=toolVersion;
		else
			m_toolVersions[toolName]="";
	}
};

void CSettingsManager::AddToolName(CString toolName,CString humanReadableName)
{
	if (toolName=="")
		return;

	if ( stl::find_in_map(m_toolNames,toolName,NULL)=="")
	{
		if (!humanReadableName.IsEmpty())
			m_toolNames[toolName]=humanReadableName;
		else
			m_toolNames[toolName]=toolName;
	}
};

void CSettingsManager::AddSettingsNode(XmlNodeRef newNode)
{
	CString nodeStr=newNode->getTag();

	XmlNodeRef oldNode = m_pSettingsManagerMemoryNode->findChild(nodeStr);

	if (oldNode)
		m_pSettingsManagerMemoryNode->removeChild(oldNode);

	m_pSettingsManagerMemoryNode->addChild(newNode);
}

void CSettingsManager::ExportSettings(XmlNodeRef exportNode, CString fileName)
{
	exportNode->saveToFile(fileName);
}

void CSettingsManager::Export()
{
	// Feed in-memory node of CSettingsManager with global Sandbox settings
	gSettings.Load();

	if (m_ExportFilePath.IsEmpty())
		return;

	// Update to the latest layout
	UpdateLayoutNode();

	// Save CVars
	SerializeCVars(m_pSettingsManagerMemoryNode,false);

	m_pSettingsManagerMemoryNode->saveToFile(m_ExportFilePath);

	GetIEditor()->SetStatusText("Export Successful");
}

void CSettingsManager::UpdateLayoutNode()
{
	CMainFrame *pMainFrame = (CMainFrame*)AfxGetMainWnd();
	pMainFrame->SaveCurrentLayout();

	TToolNamesMap::const_iterator tIt;
	for (tIt = m_toolNames.begin(); tIt != m_toolNames.end(); ++tIt)
	{	
		CString toolName(tIt->first);

		CXTPDockingPaneManager dm;
		CXTPDockingPaneLayout layout(&dm);

		if (layout.Load((LPCSTR)toolName))
		{
			if (layout.GetPaneList().GetCount() > 0)
				SaveLayoutSettings(layout,toolName);
		}
	}
}

void CSettingsManager::GetMatchingLayoutNames(TToolNamesMap &foundTools, XmlNodeRef &resultNode, CString file)
{
	// Need to populate in-memory node with available layouts.
	UpdateLayoutNode();

	XmlNodeRef root = XmlHelpers::LoadXmlFromFile(file);

	if (!root)
		return;

	root = root->findChild(EDITOR_LAYOUT_ROOT_NODE);

	if (!root)
		return;

	root = root->findChild(EDITOR_LAYOUT_NODE);

	if (!root)
		return;

	TToolNamesMap *toolNames = NULL;
	
	if (!foundTools.empty())
		toolNames=&foundTools;
	else
		toolNames = GetToolNames();

	if (!toolNames)
		return;

	if (toolNames->empty())
		return;

	// toolIT->first is tool layout node name in XML file
	TToolNamesMap::const_iterator toolIT = toolNames->begin();

	for ( ; toolIT!=toolNames->end(); ++toolIT)
		if (root->findChild(toolIT->first))
		{
			foundTools[toolIT->first]=toolIT->second;

			if (resultNode)
				resultNode->addChild(root->findChild(toolIT->first));
		}
}

void CSettingsManager::ImportSettings(CString file)
{
	XmlNodeRef root = XmlHelpers::LoadXmlFromFile(file);

	if (!root)
		return;

	XmlNodeRef importedSettingsContentNode = root->findChild(EDITOR_SETTINGS_CONTENT_NODE);

	if (!importedSettingsContentNode)
		return;

    // Remove old settings node
	XmlNodeRef oldSettingsContentNode = m_pSettingsManagerMemoryNode->findChild(EDITOR_SETTINGS_CONTENT_NODE);
	m_pSettingsManagerMemoryNode->removeChild(oldSettingsContentNode);
	
	// Add new, imported settings node
	m_pSettingsManagerMemoryNode->addChild(importedSettingsContentNode);

	// Force global settings to reload from memory node instead of registry
	gSettings.bSettingsManagerMode = true;
	gSettings.Load();
	gSettings.bSettingsManagerMode = false;

	// Dump ToolBox node on disk, replacing the old one
	XmlNodeRef toolBoxNode =  root->findChild(TOOLBOX_NODE);
	if (toolBoxNode)
		toolBoxNode->saveToFile(TOOLBOX_FILE);

	// Dump UserTools node on disk, replacing the old one
	XmlNodeRef userToolsNode =  root->findChild(TOOLBOXMACROS_NODE);

	if (userToolsNode)
	{
		CString macroFilePath;
		GetIEditor()->GetToolBoxManager()->GetSaveFilePath(macroFilePath);
		userToolsNode->saveToFile(macroFilePath);
		GetIEditor()->GetToolBoxManager()->Load();
	}

	// Get and update CVars
	SerializeCVars(root,true);

	GetIEditor()->SetStatusText("Import Successful");
}

bool CSettingsManager::NeedSettingsNode(const CString &path)
{
	if ( (path!=EDITOR_LAYOUT_ROOT_NODE) && (path!=TOOLBOX_NODE) && (path!=TOOLBOXMACROS_NODE) )
		return TRUE;
	else
		return FALSE;
}

void CSettingsManager::SerializeCVars(XmlNodeRef &node, bool bLoad)
{
	int nNumberOfVariables(0);
	int	nCurrentVariable(0);
	IConsole* piConsole(NULL);
	ICVar* piVariable(NULL);
	std::vector<char*>	cszVariableNames;

	char* szKey(NULL);
	char* szValue(NULL);
	ICVar* piCVar(NULL);

	piConsole=gEnv->pConsole;

	if (!piConsole)
	{
		return;
	}

	if (bLoad)
	{
		XmlNodeRef readNode = NULL;
		XmlNodeRef inputCVarsNode = node->findChild(CVARS_NODE);

		if (!inputCVarsNode)
		{
			return;
		}

		// Remove Entities with ID`s from the list.
		for (int childNo=0 ; childNo < inputCVarsNode->getChildCount() ; ++childNo )
		{
			readNode = inputCVarsNode->getChild(childNo);

			for (int i = 0; i < readNode->getNumAttributes(); ++i)
			{
				readNode->getAttributeByIndex(i,(const char**)&szKey,(const char**)&szValue);
				piCVar=piConsole->GetCVar(szKey);
				if (!piCVar)
				{
					continue;
				}
				piCVar->Set(szValue);
			}
		}
	}
	else
	{
		XmlNodeRef newCVarNode = NULL;
		XmlNodeRef oldCVarsNode = node->findChild(CVARS_NODE);

		if (oldCVarsNode)
			node->removeChild(oldCVarsNode);

		XmlNodeRef cvarsNode = XmlHelpers::CreateXmlNode(CVARS_NODE);

		nNumberOfVariables=piConsole->GetNumVars();
		cszVariableNames.resize(nNumberOfVariables,NULL);

		if (piConsole->GetSortedVars((const char**)&cszVariableNames.front(),nNumberOfVariables,NULL)!=nNumberOfVariables)
		{
			assert(false);
			return;
		}

		for (nCurrentVariable=0;nCurrentVariable<cszVariableNames.size();++nCurrentVariable)
		{
			if ( stricmp(cszVariableNames[nCurrentVariable],"_TestFormatMessage") == 0 )
				continue;

			piVariable=piConsole->GetCVar(cszVariableNames[nCurrentVariable]);
			if (!piVariable)
			{
				assert(false);
				continue;
			}

			newCVarNode = XmlHelpers::CreateXmlNode(CVAR_NODE);
			newCVarNode->setAttr(cszVariableNames[nCurrentVariable],piVariable->GetString());
			cvarsNode->addChild(newCVarNode);
		}

		node->addChild(cvarsNode);

	}
}

void CSettingsManager::ReadValueStr(XmlNodeRef &sourceNode,const CString &path, const CString &attr,CString &val)
{
	// Make sure the path has "\" at the end
	CString fixedPath=path;

	// Make sure it ends with "\" before extraction of tokens
	CString slashChar = fixedPath.Right(fixedPath.GetLength()-1);

	if (slashChar!="\\")
		fixedPath +="\\";

	std::vector<CString> strNodes;
	Path::GetDirectoryQueue(fixedPath,strNodes);
	CString readAttr=attr;

	// Spaces in node names not allowed
	readAttr.Replace(" ","");

	XmlNodeRef root = NULL;
	XmlNodeRef tmpNode = NULL;

	if (NeedSettingsNode(path))
		tmpNode = sourceNode->findChild(EDITOR_SETTINGS_CONTENT_NODE);
	else
		tmpNode = sourceNode;

	if (!tmpNode)
		return;

	for (int i=0; i<strNodes.size(); ++i )
	{
		if (tmpNode->findChild(strNodes[i]))
			tmpNode = tmpNode->findChild(strNodes[i]);
		else
			return;
	}

	if (!tmpNode->findChild(readAttr))
		return;
	else
		tmpNode = tmpNode->findChild(readAttr);

	if (tmpNode->haveAttr(EDITOR_SETTINGS_ATTRIB_NAME))
		tmpNode->getAttr(EDITOR_SETTINGS_ATTRIB_NAME,val);

	return;
}

void CSettingsManager::ReadThemeSettings(CString &filePath)
{
	XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filePath);

	if (!root)
		return;

	CString defaultValue("0");

	ReadValueStr(root,"Settings", "Skinning", defaultValue );
	gSettings.gui.bSkining = atoi(defaultValue);

	defaultValue = "0";
	ReadValueStr(root,"Settings", "DarkSkin", defaultValue );
	gSettings.gui.bDarkSkin = atoi(defaultValue);

	if ( gSettings.gui.bSkining)
	{
		if (gSettings.gui.bDarkSkin)
			((CMainFrame*)AfxGetMainWnd())->SwitchSkin(ID_SKINS_CRYDARK);
		else
			((CMainFrame*)AfxGetMainWnd())->SwitchSkin(ID_SKINS_CRYLIGHT);
	}

	gSettings.Save();
}

void CSettingsManager::RegisterEvent(const SEventLog &event)
{
	if (event.m_eventName.IsEmpty())
		return;

	CString eventName = event.m_eventName;

	CString path(eventName);
	path.Replace(" ","");
	SaveLogEventSetting(path,EVENT_LOG_EVENT_NAME,event.m_eventName);
	CString subPath=path+"\\"+EVENT_LOG_EVENT_NAME;

	SaveLogEventSetting(subPath,EVENT_LOG_CALLER_VERSION,event.m_callerVersion);
	SaveLogEventSetting(subPath,"state",event.m_eventState);

	CTime theTime = CTime::GetCurrentTime();
	SaveLogEventSetting(subPath,"time", theTime.Format(LOG_DATETIME_FORMAT) );
}

void CSettingsManager::UnregisterEvent(const SEventLog &event)
{
	XmlNodeRef root = XmlHelpers::LoadXmlFromFile(EDITOR_EVENT_LOG_FILE_PATH);

	if (!root)
		return;

	CString eventName = event.m_eventName;

	CString path(eventName);
	path.Replace(" ","");
	CString subPath=path+"\\"+EVENT_LOG_EVENT_NAME;

	XmlNodeRef resNode = LoadLogEventSetting(subPath,EDITOR_EVENT_LOG_ATTRIB_NAME, eventName,root);
	if (!resNode)
		return;

	root->removeChild(resNode->getParent());
	root->saveToFile(EDITOR_EVENT_LOG_FILE_PATH);
}

bool CSettingsManager::IsEventSafe(const SEventLog &event)
{
	XmlNodeRef root = XmlHelpers::LoadXmlFromFile(EDITOR_EVENT_LOG_FILE_PATH);

	if (!root)
		return TRUE;

	CString eventName = event.m_eventName;

	CString path(eventName);
	path+= "\\";
	path+=EVENT_LOG_EVENT_NAME;
	path.Replace(" ","");
	XmlNodeRef resNode = LoadLogEventSetting(path,EDITOR_EVENT_LOG_ATTRIB_NAME,eventName,root);

	// Log entry not found, so it is safe to start
	if (!resNode)
		return TRUE;

	XmlNodeRef callerVersion = resNode->findChild(EVENT_LOG_CALLER_VERSION);

	if (callerVersion)
	{
		CString callerVersionStr("");

		if (callerVersion->haveAttr(EDITOR_EVENT_LOG_ATTRIB_NAME))
			callerVersionStr=callerVersion->getAttr(EDITOR_EVENT_LOG_ATTRIB_NAME);

		if (!callerVersionStr.IsEmpty())
			if (callerVersionStr!=GetToolVersion(eventName))
				return TRUE;

		// The same version of tool/level found
		return FALSE;
	}

	return TRUE;
	
}

//////////////////////////////////////////////////////////////////////////
void CSettingsManager::SaveLogEventSetting(const CString &path, const CString &attr,const CString &val)
{
	std::vector<CString> strNodes;

	CString fixedPath=path;
	char c = path[path.GetLength()];

	// Make sure it ends with "\" before extraction of tokens
	CString slashChar = fixedPath.Right(fixedPath.GetLength()-1);

	if (slashChar!="\\")
		fixedPath +="\\";

	Path::GetDirectoryQueue(fixedPath,strNodes);

	CString writeAttr=attr;

	// Spaces in node names not allowed
	writeAttr.Replace(" ","");


	XmlNodeRef root = XmlHelpers::LoadXmlFromFile(EDITOR_EVENT_LOG_FILE_PATH);

	if (!root)
	{
		root = XmlHelpers::CreateXmlNode(EDITOR_EVENT_LOG_ROOT_NODE);
		root->saveToFile(EDITOR_LAYOUT_FILE_PATH);
	}

	XmlNodeRef tmpNode = root;

	for (int i=0; i<strNodes.size(); ++i )
	{
		if (!tmpNode->findChild(strNodes[i]))
		{
			tmpNode->addChild(tmpNode->createNode(strNodes[i]));
			tmpNode = tmpNode->findChild(strNodes[i]);
		}
		else
			tmpNode = tmpNode->findChild(strNodes[i]);
	}

	if (!tmpNode->findChild(writeAttr))
	{
		tmpNode->addChild(tmpNode->createNode(writeAttr));
		tmpNode = tmpNode->findChild(writeAttr);
	}
	else
		tmpNode = tmpNode->findChild(writeAttr);

	if (!tmpNode)
		return;

	tmpNode->setAttr(EDITOR_EVENT_LOG_ATTRIB_NAME,val);

	root->saveToFile(EDITOR_EVENT_LOG_FILE_PATH);
}

XmlNodeRef CSettingsManager::LoadLogEventSetting(const CString &path, const CString &attr,CString &val,XmlNodeRef &root)
{
	// Make sure it ends with "\" before extraction of tokens
	CString fixedPath=path;
	CString slashChar = fixedPath.Right(fixedPath.GetLength()-1);

	if (slashChar!="\\")
		fixedPath +="\\";

	std::vector<CString> strNodes;
	Path::GetDirectoryQueue(fixedPath,strNodes);
	CString readAttr=attr;

	// Spaces in node names not allowed
	readAttr.Replace(" ","");

	if (!root)
		return 0;

	XmlNodeRef tmpNode = NULL;
	tmpNode = root;

	if (!tmpNode)
		return 0;

	for (int i=0; i<strNodes.size(); ++i )
	{
		if (tmpNode->findChild(strNodes[i]) )
			tmpNode = tmpNode->findChild(strNodes[i]);
		else
			return 0;
	}

	if (tmpNode->haveAttr(readAttr))
	{
		return tmpNode;
	}

	return 0;
}

CString CSettingsManager::GenerateContentHash(XmlNodeRef &node, CString sourceName)
{
	CString hashStr("");

	if (node->getChildCount()==0 )
		return sourceName;

	static Crc32Gen generator;
	int hash = generator.GetCRC32Lowercase( node->getXML(0) ) ;
	hashStr.Format("%u",hash);

	return hashStr;
}
