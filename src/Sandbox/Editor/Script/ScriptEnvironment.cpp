#include "StdAfx.h"
#include "ScriptEnvironment.h"

#include "..\IEditor.h"
#include "..\Objects/Entity.h"


void EditorScriptEnvironment::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	if (event == eNotify_OnInit)
	{
		RegisterWithScriptSystem();
	}

	if (event == eNotify_OnSelectionChange)
	{
		SelectionChanged();
	}
}

void EditorScriptEnvironment::RegisterWithScriptSystem()
{
	Init(gEnv->pScriptSystem, gEnv->pSystem);
	SetGlobalName("Editor");

	m_selection = SmartScriptTable(gEnv->pScriptSystem, false);
	m_pMethodsTable->SetValue("Selection", m_selection);

	// Editor.* functions go here
	RegisterTemplateFunction("Command", "commandName", *this, &EditorScriptEnvironment::Command);
}

void EditorScriptEnvironment::SelectionChanged()
{
	if (!m_selection)
		return;

	m_selection->Clear();

	CSelectionGroup* selectionGroup = GetIEditor()->GetSelection();
	int selectionCount = selectionGroup->GetCount();

	for (int i = 0, k = 0; i < selectionCount; ++i)
	{
		CBaseObject* object = selectionGroup->GetObject(i);
		if (object->GetType() == OBJTYPE_ENTITY)
		{
			CEntity* entity = (CEntity *)object;

			if (IEntity* iEntity = entity->GetIEntity())
				m_selection->SetAt(++k, ScriptHandle(iEntity->GetId()));
		}
	}
}

int EditorScriptEnvironment::Command(IFunctionHandler* pH, const char* commandName)
{
	GetIEditor()->GetCommandManager()->Execute(commandName);
	
	return pH->EndFunction();
}
