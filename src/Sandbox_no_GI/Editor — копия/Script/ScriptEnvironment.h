#ifndef __ScriptEnvironment_h__
#define __ScriptEnvironment_h__

#pragma once

#include <ScriptHelpers.h>


class EditorScriptEnvironment
	: public IEditorNotifyListener
	, public CScriptableBase
{
public:
	// IEditorNotifyListener
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
	// ~IEditorNotifyListener

private:
	void RegisterWithScriptSystem();
	void SelectionChanged();

	SmartScriptTable m_selection;
private:
	int Command(IFunctionHandler* pH, const char* commandName);
};


#endif