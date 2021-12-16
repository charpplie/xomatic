#ifndef __settingsmanager_h__
#define __settingsmanager_h__
#pragma once

#define EDITOR_LAYOUT_FILE_PATH "Editor/EditorLayout.xml"
#define EDITOR_SETTINGS_FILE_PATH "Editor/EditorSettings.xml"
#define EDITOR_LAYOUT_ROOT_NODE "EditorLayout"
#define EDITOR_SETTINGS_ROOT_NODE "EditorSettings"
#define EDITOR_SETTINGS_CONTENT_NODE "EditorSettingsContent"
#define NEED_SETTINGS_VALID_LOOKUP_PATH "Settings"
#define EDITOR_LAYOUT_NODE "DockingPaneLayouts"
#define EDITOR_SETTINGS_ATTRIB_NAME "value"
#define CVARS_NODE "CVars"
#define CVAR_NODE "CVar"

#define EDITOR_EVENT_LOG_FILE_PATH "Editor/EditorEventLog.xml"
#define EDITOR_EVENT_LOG_ROOT_NODE "EventRecorder"
#define EVENT_LOG_EVENT_NAME "eventName"
#define EDITOR_EVENT_LOG_ATTRIB_NAME "value"
#define EVENT_LOG_START "start"
#define EVENT_LOG_END "end"

#define LOG_DATETIME_FORMAT "%m/%d/%Y %H:%M:%S"

#define EVENT_LOG_CALLER_VERSION "callerVersion"

enum EditorSettingsManagerType
{
	eSettingsManagerMemoryStorage=0,
	eSettingsManagerLast
};

enum EditorSettingsExportType
{
	eSettingsManagerExportSettings=0,
	eSettingsManagerExportLayout
};

typedef std::map<CString,CString> TToolNamesMap;

struct SEventLog;

struct SEventLog
{
	SEventLog( CString eventName,CString eventState,CString callerVersion = "" )
	{
		m_eventName=eventName;
		m_eventState = eventState;
		m_callerVersion = callerVersion;
	}

	CString m_eventName;
	CString m_callerVersion;
	CString m_eventState;
};

class CSettingsManager
{
public:
	// eMemoryStorage=0
	CSettingsManager(EditorSettingsManagerType managerType);
	~CSettingsManager();

	// Sandbox Editor events
	void RegisterEvent(const SEventLog &event);
	void UnregisterEvent(const SEventLog &event);
	bool IsOpenSafe(const CString &sPaneClassName);
	bool IsEventSafe(const SEventLog &event);
	CString GenerateContentHash(XmlNodeRef &node, CString sourceName);

	XmlNodeRef LoadLogEventSetting(const CString &path, const CString &attr,CString &val,XmlNodeRef &root);
	void SaveLogEventSetting(const CString &path, const CString &attr,const CString &val);

	bool CreateDefaultLayoutSettingsFile();
	bool OpenEditorSettingsFile(IXMLDOMDocumentPtr &domDocument);

	bool LoadLayoutSettings(CXTPDockingPaneLayout &layout, const LPCSTR &toolName );
	void SaveLayoutSettings(CXTPDockingPaneLayout &slayout, LPCSTR toolName );

	void LoadSetting(const CString &path, const CString &attr,bool &val);
	void LoadSetting(const CString &path, const CString &attr,int &iVal);
	void LoadSetting(const CString &path, const CString &attr,float &fVal);
	void LoadSetting(const CString &path, const CString &attr,COLORREF &color);
	XmlNodeRef LoadSetting(const CString &path, const CString &attr,CString &val);

	void SaveSetting(const CString &path, const CString &attr,bool val);
	void SaveSetting(const CString &path, const CString &attr,float fVal);
	void SaveSetting(const CString &path, const CString &attr,int iVal);
	void SaveSetting(const CString &path, const CString &attr,COLORREF color);
	void SaveSetting(const CString &path, const CString &attr,const CString &val);
	
	void AddSettingsNode(XmlNodeRef newNode);

	void AddToolVersion(CString toolName,CString toolVersion );
	CString& GetToolVersion(CString sPaneClassName){ return m_toolVersions[sPaneClassName]; };

	void AddToolName(CString toolName,CString humanReadableName = NULL );
	TToolNamesMap* GetToolNames() { return &m_toolNames; };

	// Test if all tools can be safely opened
	bool IsToolsOpenSafe();

	void ClearToolNames(){ m_toolNames.clear(); m_toolVersions.clear(); };

	void UpdateLayoutNode();

	void ImportSettings(CString file);

	void ExportSettings(XmlNodeRef exportNode, CString fileName);
	void Export();
	
	void GetMatchingLayoutNames(TToolNamesMap &foundTools, XmlNodeRef &resultNode, CString file);
	bool NeedSettingsNode(const CString &path);
	
	void SerializeCVars(XmlNodeRef &node, bool bLoad);

    void ReadValueStr(XmlNodeRef &sourceNode,const CString &path, const CString &attr,CString &val);
	void ReadThemeSettings(CString &filePath);

	void SetExportFileName(CString exportFilePath) { m_ExportFilePath=exportFilePath; };

private:
	// Save settings to memory or file
	EditorSettingsManagerType m_managerType;

	// Full path of exported file
	CString m_ExportFilePath;

	// Node created in memory to be used for exporting editor settings, console-set cvars and layout
	XmlNodeRef m_pSettingsManagerMemoryNode;

	// Registered Tool Names
	TToolNamesMap m_toolNames;
	TToolNamesMap m_toolVersions;

	//static B4BugCrytekExtension s_b4BugCrytekExtension;
};

#endif // __settingsmanager_h__
