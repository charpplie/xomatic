#pragma once
#include <ILiveCreatePlatform.h>
#include "EditorLiveCreateManager.h"
#include "ConfigPanel.h"

#ifndef NO_LIVECREATE

class CLiveCreateSettingsPanel : public CConfigPanel
{
public:
	CLiveCreateSettingsPanel(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLiveCreateSettingsPanel();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	virtual void OnConfigValueChanged(Config::IConfigVar* pVar);
};

#endif