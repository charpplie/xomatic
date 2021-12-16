// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "IResourceSelectorHost.h"
#include "Events/EventManager.h"
#include "QParentWndWidget.h"
#include "ListSelectionDialog.h"
#include "QAudioControlBrowserIcons.h"
#include "GameEngine.h"
#include "AudioControlsBrowserPlugin.h"
#include "AudioLibrary.h"
#include "ATLControlsResourceDialog.h"
#include "IGameFramework.h"
#include "IEditor.h"

using namespace AudioControls;

namespace
{
	dll_string ShowSelectDialog(const SResourceSelectorContext& context, const char* pPreviousValue, const EACBControlType controlType)
	{
		CATLControlsModel* pModel = CAudioControlsBrowserPlugin::GetATLModel();
		assert(pModel);

		QParentWndWidget parent(context.parentWindow);
		parent.center();
		parent.setWindowModality(Qt::ApplicationModal);

		ATLControlsDialog dialog(&parent, controlType);

		char *sLevelName;
		GetIEditor()->GetGame()->GetIGameFramework()->GetEditorLevel(&sLevelName, nullptr);
		dialog.SetScope(sLevelName);
		return dialog.ChooseItem(pPreviousValue);
	}

	dll_string AudioTriggerSelector(const SResourceSelectorContext& context, const char* pPreviousValue)
	{
		return ShowSelectDialog(context, pPreviousValue, eACBT_TRIGGER);
	}

	dll_string AudioSwitchSelector(const SResourceSelectorContext& context, const char* pPreviousValue)
	{
		return ShowSelectDialog(context, pPreviousValue, eACBT_SWITCH);
	}

	dll_string AudioSwitchStateSelector(const SResourceSelectorContext& context, const char* pPreviousValue)
	{
		return ShowSelectDialog(context, pPreviousValue, eACBT_SWITCH);
	}

	dll_string AudioRTPCSelector(const SResourceSelectorContext& context, const char* pPreviousValue)
	{
		return ShowSelectDialog(context, pPreviousValue, eACBT_RTPC);
	}

	dll_string AudioEnvironmentSelector(const SResourceSelectorContext& context, const char* pPreviousValue)
	{
		return ShowSelectDialog(context, pPreviousValue, eACBT_ENVIRONMENTS);
	}

	dll_string AudioPreloadRequestSelector(const SResourceSelectorContext& context, const char* pPreviousValue)
	{
		return ShowSelectDialog(context, pPreviousValue, eACBT_PRELOADS);
	}

	REGISTER_RESOURCE_SELECTOR("AudioTrigger", AudioTriggerSelector, "Editor/Icons/audio/Trigger.png")
	REGISTER_RESOURCE_SELECTOR("AudioSwitch", AudioSwitchSelector, "Editor/Icons/audio/Switch.png")
	REGISTER_RESOURCE_SELECTOR("AudioSwitchState", AudioSwitchStateSelector, "Editor/Icons/audio/State.png")
	REGISTER_RESOURCE_SELECTOR("AudioRTPC", AudioRTPCSelector, "Editor/Icons/audio/RTPC.png")
	REGISTER_RESOURCE_SELECTOR("AudioEnvironment", AudioEnvironmentSelector, "Editor/Icons/audio/Environment.png")
	REGISTER_RESOURCE_SELECTOR("AudioPreloadRequest", AudioPreloadRequestSelector, "Editor/Icons/audio/Preload.png")
}
