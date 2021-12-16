////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   QAudioControlBrowserIcons.h
//  Version:     v1.00
//  Created:     04/08/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#pragma once
#include <QIcon>
#include "AudioControl.h"

namespace AudioControls
{
	inline QIcon GetControlTypeIcon(EACBControlType type)
	{
		switch (type)
		{
		case AudioControls::eACBT_TRIGGER:
			return QIcon("://Icons/Trigger_Icon.png");
			break;
		case AudioControls::eACBT_RTPC:
			return QIcon("://Icons/RTPC_Icon.png");
			break;
		case AudioControls::eACBT_SWITCH:
			return QIcon("://Icons/Switch_Icon.png");
			break;
		case AudioControls::eACBT_ENVIRONMENTS:
			return QIcon("://Icons/Enviroment_Icon.png");
			break;
		case AudioControls::eACBT_PRELOADS:
			return QIcon("://Icons/Bank_Icon.png");
			break;
		}
		return QIcon("://Icons/RTPC_Icon.png");
	}

	inline QIcon GetFolderIcon()
	{
		return QIcon("://Icons/Folder_Icon.png");
	}

	inline QIcon GetPropertyIcon()
	{
		return QIcon("://Icons/Property_Icon.png");
	}

	inline QIcon GetSoundBankIcon()
	{
		return QIcon("://Icons/Preload_Icon.png");
	}

	inline QIcon GetGroupIcon(int group)
	{
		const int numberOfGroups = 4;
		group = group % numberOfGroups;
		switch (group)
		{
		case 0:
			return QIcon("://Icons/Config_Red_Icon.png");
			break;
		case 1:
			return QIcon("://Icons/Config_Blue_Icon.png");
			break;
		case 2:
			return QIcon("://Icons/Config_Green_Icon.png");
			break;
		case 3:
			return QIcon("://Icons/Config_Purple_Icon.png");
			break;
		}
		return QIcon("://Icons/Config_Red_Icon.png");
	}
}