// Copyright (c) 1999-2014 Crytek.

#pragma once
#include <IEditor.h>
#include <Include/IPlugin.h>
#include <QPixmap>
#include "SceneGraph.h"

// FBX import plugin
class CFbxImportPlugin : public IPlugin
{
	// Create plugin instance, only accessible to CreatePluginInstance
	// If you need instance, use static GetInstance()
	friend IPlugin* ::CreatePluginInstance(PLUGIN_INIT_PARAM *pInitParam);
	CFbxImportPlugin(IEditor* pEditor);

public:
	// Get the singleton instance of the plugin
	static CFbxImportPlugin *GetInstance() { return s_pInstance; }

	// Get the editor used to create this plugin
	IEditor *GetIEditor() const { return m_pEditor; }

	// Get game folder
	const QString &GetGameFolder() const { return m_gameFolder; }

	// Get icon (by value, since the implementation is shared) for a specified node class
	QPixmap GetIcon(ENodeClass nodeClass) const { assert(nodeClass >= 0 && nodeClass < eNC_COUNT); return m_icons[nodeClass]; }

	// IPlugin implementation
	void Release() override;
	void ShowAbout() override {}
	const char* GetPluginGUID() override { return "{73A03A18-842C-4433-830E-49B6B42DB192}"; }
	DWORD GetPluginVersion() override { return 1; }
	const char* GetPluginName() override { return "QtFbxImport"; }
	bool CanExitNow() override { return true; }
	void OnEditorNotify(EEditorNotifyEvent aEventId) override {}

private:
	// Singleton instance
	static CFbxImportPlugin *s_pInstance;

	// The editor used to construct the plugin
	IEditor * const m_pEditor;

	// The game folder to use
	QString m_gameFolder;

	// Mutex for FBX SDK
	CryCriticalSection m_globalLock;

	// Shared icons
	QPixmap m_icons[eNC_COUNT];

	// Translated tool name
	string m_translatedToolName;
};