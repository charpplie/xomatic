////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   settings.cpp
//  Version:     v1.00
//  Created:     14/1/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Settings.h"
#include "CryThread.h"

//////////////////////////////////////////////////////////////////////////
// Global Instance of Editor settings.
//////////////////////////////////////////////////////////////////////////
SANDBOX_API SEditorSettings gSettings;

//////////////////////////////////////////////////////////////////////////
SGizmoSettings::SGizmoSettings()
{
	axisGizmoSize = 0.2f;
	axisGizmoText = true;
	axisGizmoMaxCount = 50;
	helpersScale = 1;
}
//////////////////////////////////////////////////////////////////////////
CString& CHotUpdateSystem::GetPlatformConsoleRootDirectory(int ePlatform,CString& strTargetString)
{
	assert(ePlatform>=eFirstPlatform);
	assert(ePlatform<eNumberOfPlatforms);

	if	( 
		(ePlatform>=eFirstPlatform)
		&&
		(ePlatform<eNumberOfPlatforms)
		)
	{
		GetIOMutex().Lock();
		strTargetString=strPlatformConsoleRootDirectory[ePlatform];
		GetIOMutex().Unlock();
	}
	return strTargetString;
}
//////////////////////////////////////////////////////////////////////////
const CString&	CHotUpdateSystem::SetPlatformConsoleRootDirectory(int ePlatform,const CString&	strSourceString)
{
	assert(ePlatform>=eFirstPlatform);
	assert(ePlatform<eNumberOfPlatforms);

	if	( 
		(ePlatform>=eFirstPlatform)
		&&
		(ePlatform<eNumberOfPlatforms)
		)
	{
		GetIOMutex().Lock();
		strPlatformConsoleRootDirectory[ePlatform]=strSourceString;
		GetIOMutex().Unlock();
	}
	return strSourceString;
}
//////////////////////////////////////////////////////////////////////////
CString&	CHotUpdateSystem::GetPlatformTargetMachine(int ePlatform,CString&	strTargetString)
{
	assert(ePlatform>=eFirstPlatform);
	assert(ePlatform<eNumberOfPlatforms);

	if	( 
		(ePlatform>=eFirstPlatform)
		&&
		(ePlatform<eNumberOfPlatforms)
		)
	{
		GetIOMutex().Lock();
		strTargetString=strPlatformTargetMachine[ePlatform];
		GetIOMutex().Unlock();
	}
	return strTargetString;
}
//////////////////////////////////////////////////////////////////////////
const CString&	CHotUpdateSystem::SetPlatformTargetMachine(int ePlatform,const CString&	strSourceString)
{
	assert(ePlatform>=eFirstPlatform);
	assert(ePlatform<eNumberOfPlatforms);

	if	( 
		(ePlatform>=eFirstPlatform)
		&&
		(ePlatform<eNumberOfPlatforms)
		)
	{
		GetIOMutex().Lock();
		strPlatformTargetMachine[ePlatform]=strSourceString;
		GetIOMutex().Unlock();
	}
	return strSourceString;
}
//////////////////////////////////////////////////////////////////////////
CString&					CHotUpdateSystem::GetPlatformExecutableName(int ePlatform,CString&				strTargetString)
{
	assert(ePlatform>=eFirstPlatform);
	assert(ePlatform<eNumberOfPlatforms);

	if	( 
		(ePlatform>=eFirstPlatform)
		&&
		(ePlatform<eNumberOfPlatforms)
		)
	{
		GetIOMutex().Lock();
		strTargetString=strPlatformExecutableName[ePlatform];
		GetIOMutex().Unlock();
	}
	return strTargetString;
}
//////////////////////////////////////////////////////////////////////////
const CString&		CHotUpdateSystem::SetPlatformExecutableName(int ePlatform,const CString&	strSourceString)
{
	assert(ePlatform>=eFirstPlatform);
	assert(ePlatform<eNumberOfPlatforms);

	if	( 
		(ePlatform>=eFirstPlatform)
		&&
		(ePlatform<eNumberOfPlatforms)
		)
	{
		GetIOMutex().Lock();
		strPlatformExecutableName[ePlatform]=strSourceString;
		GetIOMutex().Unlock();
	}
	return strSourceString;
}
//////////////////////////////////////////////////////////////////////////
CString&	CHotUpdateSystem::GetPlatformConsoleRootDirectoryNoLock(int ePlatform,CString&	strTargetString)
{
	strTargetString=strPlatformConsoleRootDirectory[ePlatform];
	return strTargetString;
}
//////////////////////////////////////////////////////////////////////////
CString&	CHotUpdateSystem::GetPlatformTargetMachineNoLock(int ePlatform,CString&	strTargetString)
{
	strTargetString=strPlatformTargetMachine[ePlatform];
	return strTargetString;
}
//////////////////////////////////////////////////////////////////////////
CString&	CHotUpdateSystem::GetPlatformExecutableNameNoLock(int ePlatform,CString&	strTargetString)
{
	strTargetString=strPlatformExecutableName[ePlatform];
	return strTargetString;
}
//////////////////////////////////////////////////////////////////////////
CryMutex&	CHotUpdateSystem::GetIOMutex()
{
	static CryMutex oCryMutex;
	return	oCryMutex;
}
//////////////////////////////////////////////////////////////////////////
SEditorSettings::SEditorSettings()
{
	undoLevels = 50;

	objectHideMask = 0;
	objectSelectMask = 0xFFFFFFFF; // Initially all selectable.

	autoBackupFilename = "Autobackup";
	autoBackupEnabled = false;
	autoBackupTime = 10;
	autoRemindTime = 0;

	editorConfigSpec = CONFIG_MEDIUM_SPEC;
	
	viewports.bAlwaysShowRadiuses = false;
	viewports.bAlwaysDrawPrefabBox = false;
	viewports.bAlwaysDrawPrefabInternalObjects = false;
	viewports.bSync2DViews = true;
	viewports.fDefaultAspectRatio = 800.0f/600.0f;
	viewports.fDefaultFov = DEG2RAD(60); // 60 degrees (to fit with current game)
	viewports.bShowSafeFrame = false;
	viewports.bHighlightSelectedGeometry = false;
	viewports.bHighlightMouseOverGeometry = true;
	viewports.bDrawEntityLabels = false;
	viewports.bShowTriggerBounds = false;
	viewports.bShowIcons = true;
	viewports.bFillSelectedShapes = false;
	viewports.nTopMapTextureResolution = 512;
	viewports.bTopMapSwapXY = false;

	cameraMoveSpeed = 1;
	cameraFastMoveSpeed = 2;
	wheelZoomSpeed = 1;
	fBrMultiplier = 1;
	bPreviewGeometryWindow = true;
	bGeometryBrowserPanel = true;
	bBackupOnSave = true;
	bFlowGraphMigrationEnabled = true;
	bFlowGraphShowNodeIDs = false;
	bFlowGraphShowToolTip = true;
	bApplyConfigSpecInEditor = true;

	colorConsoleBG = RGB(255, 255, 255);

	strStandardTempDirectory="Temp";

	// Init source safe params.
	enableSourceControl = true;

	saveOnlyModified = false;

	textEditorForScript = "notepad.exe";
	textEditorForShaders = "notepad.exe";

	terrainTextureExport = "";

	sTextureBrowserSettings.nCellSize = 128;
	sAssetBrowserSettings.nThumbSize = 128;

	oHotUpdateSystemSettings.boShowHotUpdateProgress=true;

	oHotUpdateSystemSettings.boPlatformEnabled[CHotUpdateSystem::eXBOX360]=false;
	oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[CHotUpdateSystem::eXBOX360]=true;
	oHotUpdateSystemSettings.SetPlatformTargetMachine(CHotUpdateSystem::eXBOX360,"");
	oHotUpdateSystemSettings.SetPlatformConsoleRootDirectory(CHotUpdateSystem::eXBOX360,"xE:\\CryEngine");
	oHotUpdateSystemSettings.SetPlatformExecutableName(CHotUpdateSystem::eXBOX360,"Crysis.xex");

	oHotUpdateSystemSettings.boPlatformEnabled[CHotUpdateSystem::ePLAYSTATION3]=false;
	oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[CHotUpdateSystem::ePLAYSTATION3]=true;
	oHotUpdateSystemSettings.SetPlatformTargetMachine(CHotUpdateSystem::ePLAYSTATION3,"");
	oHotUpdateSystemSettings.SetPlatformConsoleRootDirectory(CHotUpdateSystem::ePLAYSTATION3,"/dev_hdd0/game/GAME02/USRDIR/");
	oHotUpdateSystemSettings.SetPlatformExecutableName(CHotUpdateSystem::ePLAYSTATION3,"C:/work/PS3Launcher_profile.self");

	oHotUpdateSystemSettings.boSyncCamera=false;
	oHotUpdateSystemSettings.boRealtimeSyncEnabled=false;

	//////////////////////////////////////////////////////////////////////////
	// Time of day settings
	//////////////////////////////////////////////////////////////////////////
	bShowBasicPropertiesInTimeOfDay = true;

	//////////////////////////////////////////////////////////////////////////
	// Initialize GUI settings.
	//////////////////////////////////////////////////////////////////////////
	OSVERSIONINFO OSVerInfo;
	OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSVerInfo);
	gui.bWindowsVista = OSVerInfo.dwMajorVersion >= 6;
	gui.bSkining = false;

	int lfHeight = -MulDiv(8, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
	gui.nDefaultFontHieght = lfHeight;
	gui.hSystemFont = ::CreateFont(lfHeight,0,0,0,FW_NORMAL,0,0,0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH, "Ms Shell Dlg 2");

	gui.hSystemFontBold = ::CreateFont(lfHeight,0,0,0,FW_BOLD,0,0,0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH, "Ms Shell Dlg 2");

	gui.hSystemFontItalic = ::CreateFont(lfHeight,0,0,0,FW_NORMAL,TRUE,0,0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH, "Ms Shell Dlg 2");
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::SaveValue( const char *sSection,const char *sKey,int value )
{
	AfxGetApp()->WriteProfileInt( sSection,sKey,value );
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::SaveValue( const char *sSection,const char *sKey,COLORREF value )
{
	AfxGetApp()->WriteProfileInt( sSection,sKey,value );
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::SaveValue( const char *sSection,const char *sKey,float value )
{
	CString str;
	str.Format( "%g",value );
	AfxGetApp()->WriteProfileString( sSection,sKey,str );
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::SaveValue( const char *sSection,const char *sKey,const CString &value )
{
	AfxGetApp()->WriteProfileString( sSection,sKey,value );
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue( const char *sSection,const char *sKey,int &value )
{
	value = AfxGetApp()->GetProfileInt(sSection,sKey,value );
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue( const char *sSection,const char *sKey,COLORREF &value )
{
	value = AfxGetApp()->GetProfileInt(sSection,sKey,value );
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue( const char *sSection,const char *sKey,float &value )
{
	CString defaultVal;
	defaultVal.Format( "%g",value );
	defaultVal = AfxGetApp()->GetProfileString( sSection,sKey,defaultVal );
	value = atof(defaultVal);
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue( const char *sSection,const char *sKey,bool &value )
{
	value = AfxGetApp()->GetProfileInt(sSection,sKey,value );
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue( const char *sSection,const char *sKey,CString &value )
{
	value = AfxGetApp()->GetProfileString( sSection,sKey,value );
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue( const char *sSection,const char *sKey,ESystemConfigSpec &value)
{
	ESystemConfigSpec valuecheck = (ESystemConfigSpec)AfxGetApp()->GetProfileInt(sSection,sKey,(int)value);
	if ( valuecheck >= CONFIG_CUSTOM && valuecheck < END_CONFIG_SPEC_ENUM )
	{
		value = valuecheck;
	}
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::Save()
{
	CString strStringPlaceholder;

	// Save settings to registry.
	SaveValue( "Settings","UndoLevels",undoLevels );
	SaveValue("Settings","AutoBackup",autoBackupEnabled );
	SaveValue("Settings","AutoBackupTime",autoBackupTime );
	SaveValue("Settings","AutoRemindTime",autoRemindTime );
	SaveValue("Settings","CameraMoveSpeed",cameraMoveSpeed );
	SaveValue("Settings","WheelZoomSpeed",wheelZoomSpeed );
	SaveValue("Settings","BrMultiplier",fBrMultiplier);
	SaveValue("Settings","CameraFastMoveSpeed",cameraFastMoveSpeed );
	SaveValue("Settings","PreviewGeometryWindow",bPreviewGeometryWindow );
	SaveValue("Settings","GeometryBrowserPanel",bGeometryBrowserPanel );
	SaveValue("Settings","AutobackupFile",autoBackupFilename );

	SaveValue("Settings","BackupOnSave",bBackupOnSave );
	SaveValue("Settings","ApplyConfigSpecInEditor",bApplyConfigSpecInEditor );

	SaveValue("Settings","editorConfigSpec",editorConfigSpec);

	SaveValue("Settings","TemporaryDirectory",strStandardTempDirectory);

	SaveValue("Settings","colorConsoleBG",colorConsoleBG);

	//////////////////////////////////////////////////////////////////////////
	// Viewport settings.
	//////////////////////////////////////////////////////////////////////////
	SaveValue("Settings","AlwaysShowRadiuses",viewports.bAlwaysShowRadiuses );
	SaveValue("Settings","AlwaysShowPrefabBounds",viewports.bAlwaysDrawPrefabBox );
	SaveValue("Settings","AlwaysShowPrefabObjects",viewports.bAlwaysDrawPrefabInternalObjects );
	SaveValue("Settings","Sync2DViews",viewports.bSync2DViews );
	SaveValue("Settings","DefaultFov",viewports.fDefaultFov );
	SaveValue("Settings","AspectRatio",viewports.fDefaultAspectRatio );
	SaveValue("Settings","ShowSafeFrame",viewports.bShowSafeFrame );
	SaveValue("Settings","HighlightSelectedGeometry",viewports.bHighlightSelectedGeometry );
	SaveValue("Settings","HighlightMouseOverGeometry",viewports.bHighlightMouseOverGeometry );
	SaveValue("Settings","DrawEntityLabels",viewports.bDrawEntityLabels );
	SaveValue("Settings","ShowTriggerBounds",viewports.bShowTriggerBounds );
	SaveValue("Settings","ShowIcons",viewports.bShowIcons );
	SaveValue("Settings","FillSelectedShapes",viewports.bFillSelectedShapes );
	SaveValue("Settings","MapTextureResolution",viewports.nTopMapTextureResolution );
	SaveValue("Settings","MapSwapXY",viewports.bTopMapSwapXY );

	//////////////////////////////////////////////////////////////////////////
	// Gizmos.
	//////////////////////////////////////////////////////////////////////////
	SaveValue( "Settings","AxisGizmoSize",gizmo.axisGizmoSize );
	SaveValue( "Settings","AxisGizmoText",gizmo.axisGizmoText );
	SaveValue( "Settings","AxisGizmoMaxCount",gizmo.axisGizmoMaxCount );
	SaveValue( "Settings","HelpersScale",gizmo.helpersScale );
	//////////////////////////////////////////////////////////////////////////

	SaveValue( "Settings","TextEditorScript",textEditorForScript );
	SaveValue( "Settings","TextEditorShaders",textEditorForShaders );

	SaveValue( "Settings","EnableSourceControl",enableSourceControl );
	SaveValue( "Settings","SaveOnlyModified",saveOnlyModified );

	//////////////////////////////////////////////////////////////////////////
	// AVI Settings.
	//////////////////////////////////////////////////////////////////////////
	SaveValue( "Settings","AVI_FrameRate",aviSettings.nFrameRate );
	SaveValue( "Settings","AVI_Codec",aviSettings.codec );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Snapping Settings.
	SaveValue( "Settings\\Snap","ConstructPlaneSize",snap.constructPlaneSize );
	SaveValue( "Settings\\Snap","ConstructPlaneDisplay",snap.constructPlaneDisplay );
	SaveValue( "Settings\\Snap","SnapMarkerDisplay",snap.markerDisplay );
	SaveValue( "Settings\\Snap","SnapMarkerColor",(int)snap.markerColor );
	SaveValue( "Settings\\Snap","SnapMarkerSize",snap.markerSize );
	SaveValue( "Settings\\Snap","GridUserDefined", snap.bGridUserDefined );
	SaveValue( "Settings\\Snap","GridGetFromSelected", snap.bGridGetFromSelected );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// HyperGraph Colors
	//////////////////////////////////////////////////////////////////////////
	SaveValue( "Settings\\HyperGraph","ColorArrow",hyperGraphColors.colorArrow );
	SaveValue( "Settings\\HyperGraph","ColorInArrowHighlighted",hyperGraphColors.colorInArrowHighlighted );
	SaveValue( "Settings\\HyperGraph","ColorOutArrowHighlighted",hyperGraphColors.colorOutArrowHighlighted );
	SaveValue( "Settings\\HyperGraph","ColorArrowDisabled",hyperGraphColors.colorArrowDisabled );
	SaveValue( "Settings\\HyperGraph","ColorNodeOutline",hyperGraphColors.colorNodeOutline );
	SaveValue( "Settings\\HyperGraph","ColorNodeBkg",hyperGraphColors.colorNodeBkg );
	SaveValue( "Settings\\HyperGraph","ColorNodeBkgSelected",hyperGraphColors.colorNodeBkgSelected );
	SaveValue( "Settings\\HyperGraph","ColorText",hyperGraphColors.colorText );
	SaveValue( "Settings\\HyperGraph","ColorBackground",hyperGraphColors.colorBackground);
	SaveValue( "Settings\\HyperGraph","ColorGrid",hyperGraphColors.colorGrid);

	//////////////////////////////////////////////////////////////////////////
	// HyperGraph Expert
	//////////////////////////////////////////////////////////////////////////
	SaveValue( "Settings\\HyperGraph","EnableMigration",bFlowGraphMigrationEnabled );
	SaveValue( "Settings\\HyperGraph","ShowNodeIDs",bFlowGraphShowNodeIDs );
	SaveValue( "Settings\\HyperGraph","ShowToolTip",bFlowGraphShowToolTip );
	//////////////////////////////////////////////////////////////////////////

	SaveValue( "Settings","TerrainTextureExport",terrainTextureExport );

	//////////////////////////////////////////////////////////////////////////
	// Texture browser settings
	//////////////////////////////////////////////////////////////////////////
	SaveValue( "Settings\\TextureBrowser","Cell Size", sTextureBrowserSettings.nCellSize);

	//////////////////////////////////////////////////////////////////////////
	// Asset browser settings
	//////////////////////////////////////////////////////////////////////////
	SaveValue( "Settings\\AssetBrowser","ThumbSize", sAssetBrowserSettings.nThumbSize);

	//////////////////////////////////////////////////////////////////////////
	// Hot update system settings
	//////////////////////////////////////////////////////////////////////////
	SaveValue( "Settings\\HotUpdateSystem","Show Hot-Update Progress", oHotUpdateSystemSettings.boShowHotUpdateProgress);


	SaveValue( "Settings\\HotUpdateSystem\\XBOX360","Is enabled",oHotUpdateSystemSettings.boPlatformEnabled[CHotUpdateSystem::eXBOX360]);
	SaveValue( "Settings\\HotUpdateSystem\\XBOX360","Autodetect target machine",oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[CHotUpdateSystem::eXBOX360]);
	SaveValue( "Settings\\HotUpdateSystem\\XBOX360","Target machine",oHotUpdateSystemSettings.GetPlatformTargetMachine(CHotUpdateSystem::eXBOX360,strStringPlaceholder));
	SaveValue( "Settings\\HotUpdateSystem\\XBOX360","Console root directory",oHotUpdateSystemSettings.GetPlatformConsoleRootDirectory(CHotUpdateSystem::eXBOX360,strStringPlaceholder));
	SaveValue( "Settings\\HotUpdateSystem\\XBOX360","Console executable name",oHotUpdateSystemSettings.GetPlatformExecutableName(CHotUpdateSystem::eXBOX360,strStringPlaceholder));

	SaveValue( "Settings\\HotUpdateSystem\\PLAYSTATION3","Is enabled",oHotUpdateSystemSettings.boPlatformEnabled[CHotUpdateSystem::ePLAYSTATION3]);
	SaveValue( "Settings\\HotUpdateSystem\\PLAYSTATION3","Autodetect target machine",oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[CHotUpdateSystem::ePLAYSTATION3]);
	SaveValue( "Settings\\HotUpdateSystem\\PLAYSTATION3","Target machine",oHotUpdateSystemSettings.GetPlatformTargetMachine(CHotUpdateSystem::ePLAYSTATION3,strStringPlaceholder));
	SaveValue( "Settings\\HotUpdateSystem\\PLAYSTATION3","Console root directory",oHotUpdateSystemSettings.GetPlatformConsoleRootDirectory(CHotUpdateSystem::ePLAYSTATION3,strStringPlaceholder));
	SaveValue( "Settings\\HotUpdateSystem\\PLAYSTATION3","Console executable name",oHotUpdateSystemSettings.GetPlatformExecutableName(CHotUpdateSystem::ePLAYSTATION3,strStringPlaceholder));

	//SaveValue( "Settings\\HotUpdateSystem","Sync Camera", oHotUpdateSystemSettings.boSyncCamera);
	//SaveValue( "Settings\\HotUpdateSystem","Realtime Sync",oHotUpdateSystemSettings.boRealtimeSyncEnabled);


	//////////////////////////////////////////////////////////////////////////
	// Time of day settings
	//////////////////////////////////////////////////////////////////////////
	SaveValue( "Settings\\TimeOfDay","ShowBasicProperties", bShowBasicPropertiesInTimeOfDay);


	//////////////////////////////////////////////////////////////////////////
	// Deep Selection Settings
	//////////////////////////////////////////////////////////////////////////
	SaveValue( "Settings", "DeepSelectionNearness", deepSelectionSettings.fRange);


	/*
	//////////////////////////////////////////////////////////////////////////
	// Save paths.
	//////////////////////////////////////////////////////////////////////////
	for (int id = 0; id < EDITOR_PATH_LAST; id++)
	{
		for (int i = 0; i < searchPaths[id].size(); i++)
		{
			CString path = searchPaths[id][i];
			CString key;
			key.Format( "Paths","Path_%.2d_%.2d",id,i );
			SaveValue( "Paths",key,path );
		}
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::Load()
{
	CString		strPlaceholderString;
	// Load settings from registry.
	LoadValue( "Settings","UndoLevels",undoLevels );
	LoadValue( "Settings","AutoBackup",autoBackupEnabled );
	LoadValue("Settings","AutoBackupTime",autoBackupTime );
	LoadValue("Settings","AutoRemindTime",autoRemindTime );
	LoadValue("Settings","CameraMoveSpeed",cameraMoveSpeed );
	LoadValue("Settings","WheelZoomSpeed",wheelZoomSpeed );
	LoadValue("Settings","BrMultiplier",fBrMultiplier );
	LoadValue("Settings","CameraFastMoveSpeed",cameraFastMoveSpeed );
	LoadValue("Settings","PreviewGeometryWindow",bPreviewGeometryWindow );
	LoadValue("Settings","GeometryBrowserPanel",bGeometryBrowserPanel );
	LoadValue("Settings","AutobackupFile",autoBackupFilename );

	LoadValue("Settings","BackupOnSave",bBackupOnSave );
	LoadValue("Settings","ApplyConfigSpecInEditor",bApplyConfigSpecInEditor );
	LoadValue("Settings","editorConfigSpec",editorConfigSpec);

	LoadValue( "Settings","TemporaryDirectory",strStandardTempDirectory);

	LoadValue("Settings","colorConsoleBG",colorConsoleBG );

	//////////////////////////////////////////////////////////////////////////
	// Viewport Settings.
	//////////////////////////////////////////////////////////////////////////
	LoadValue("Settings","AlwaysShowRadiuses",viewports.bAlwaysShowRadiuses );
	LoadValue("Settings","AlwaysShowPrefabBounds",viewports.bAlwaysDrawPrefabBox );
	LoadValue("Settings","AlwaysShowPrefabObjects",viewports.bAlwaysDrawPrefabInternalObjects );
	LoadValue("Settings","Sync2DViews",viewports.bSync2DViews );
	LoadValue("Settings","DefaultFov",viewports.fDefaultFov );
	LoadValue("Settings","AspectRatio",viewports.fDefaultAspectRatio );
	LoadValue("Settings","ShowSafeFrame",viewports.bShowSafeFrame );
	LoadValue("Settings","HighlightSelectedGeometry",viewports.bHighlightSelectedGeometry );
	LoadValue("Settings","HighlightMouseOverGeometry",viewports.bHighlightMouseOverGeometry );
	LoadValue("Settings","DrawEntityLabels",viewports.bDrawEntityLabels );
	LoadValue("Settings","ShowTriggerBounds",viewports.bShowTriggerBounds );
	LoadValue("Settings","ShowIcons",viewports.bShowIcons );
	LoadValue("Settings","FillSelectedShapes",viewports.bFillSelectedShapes );
	LoadValue("Settings","MapTextureResolution",viewports.nTopMapTextureResolution );
	LoadValue("Settings","MapSwapXY",viewports.bTopMapSwapXY );

	//////////////////////////////////////////////////////////////////////////
	// Gizmos.
	//////////////////////////////////////////////////////////////////////////
	LoadValue( "Settings","AxisGizmoSize",gizmo.axisGizmoSize );
	LoadValue( "Settings","AxisGizmoText",gizmo.axisGizmoText );
	LoadValue( "Settings","AxisGizmoMaxCount",gizmo.axisGizmoMaxCount );
	LoadValue( "Settings","HelpersScale",gizmo.helpersScale );
	//////////////////////////////////////////////////////////////////////////

	LoadValue( "Settings","TextEditorScript",textEditorForScript );
	LoadValue( "Settings","TextEditorShaders",textEditorForShaders );

	LoadValue( "Settings","EnableSourceControl",enableSourceControl );
	LoadValue( "Settings","SaveOnlyModified",saveOnlyModified );

	LoadValue( "Settings","AVI_FrameRate",aviSettings.nFrameRate );
	LoadValue( "Settings","AVI_Codec",aviSettings.codec );

	//////////////////////////////////////////////////////////////////////////
	// Snapping Settings.
	LoadValue( "Settings\\Snap","ConstructPlaneSize",snap.constructPlaneSize );
	LoadValue( "Settings\\Snap","ConstructPlaneDisplay",snap.constructPlaneDisplay );
	LoadValue( "Settings\\Snap","SnapMarkerDisplay",snap.markerDisplay );
	int markerColor = snap.markerColor;
	LoadValue( "Settings\\Snap","SnapMarkerColor",markerColor ); snap.markerColor = markerColor;
	LoadValue( "Settings\\Snap","SnapMarkerSize",snap.markerSize );
	LoadValue( "Settings\\Snap","GridUserDefined", snap.bGridUserDefined );
	LoadValue( "Settings\\Snap","GridGetFromSelected", snap.bGridGetFromSelected );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// HyperGraph
	//////////////////////////////////////////////////////////////////////////
	LoadValue( "Settings\\HyperGraph","ColorArrow",hyperGraphColors.colorArrow );
	LoadValue( "Settings\\HyperGraph","ColorInArrowHighlighted",hyperGraphColors.colorInArrowHighlighted );
	LoadValue( "Settings\\HyperGraph","ColorOutArrowHighlighted",hyperGraphColors.colorOutArrowHighlighted );
	LoadValue( "Settings\\HyperGraph","ColorArrowDisabled",hyperGraphColors.colorArrowDisabled );
	LoadValue( "Settings\\HyperGraph","ColorNodeOutline",hyperGraphColors.colorNodeOutline );
	LoadValue( "Settings\\HyperGraph","ColorNodeBkg",hyperGraphColors.colorNodeBkg );
	LoadValue( "Settings\\HyperGraph","ColorNodeBkgSelected",hyperGraphColors.colorNodeBkgSelected );
	LoadValue( "Settings\\HyperGraph","ColorText",hyperGraphColors.colorText );
	LoadValue( "Settings\\HyperGraph","ColorBackground",hyperGraphColors.colorBackground);
	LoadValue( "Settings\\HyperGraph","ColorGrid",hyperGraphColors.colorGrid);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// HyperGraph Expert
	//////////////////////////////////////////////////////////////////////////
	LoadValue( "Settings\\HyperGraph","EnableMigration",bFlowGraphMigrationEnabled );
	LoadValue( "Settings\\HyperGraph","ShowNodeIDs",bFlowGraphShowNodeIDs );
	LoadValue( "Settings\\HyperGraph","ShowToolTip",bFlowGraphShowToolTip );

	//////////////////////////////////////////////////////////////////////////

	LoadValue( "Settings","TerrainTextureExport",terrainTextureExport );

	//////////////////////////////////////////////////////////////////////////
	// Texture browser settings
	//////////////////////////////////////////////////////////////////////////
	LoadValue( "Settings\\TextureBrowser","Cell Size", sTextureBrowserSettings.nCellSize);

	//////////////////////////////////////////////////////////////////////////
	// Asset browser settings
	//////////////////////////////////////////////////////////////////////////
	LoadValue( "Settings\\AssetBrowser","ThumbSize", sAssetBrowserSettings.nThumbSize);

	//////////////////////////////////////////////////////////////////////////
	// Hot update system settings.
	//////////////////////////////////////////////////////////////////////////
	LoadValue( "Settings\\HotUpdateSystem","Show Hot-Update Progress", oHotUpdateSystemSettings.boShowHotUpdateProgress);
	
	LoadValue( "Settings\\HotUpdateSystem\\XBOX360","Is enabled",oHotUpdateSystemSettings.boPlatformEnabled[CHotUpdateSystem::eXBOX360]);
	LoadValue( "Settings\\HotUpdateSystem\\XBOX360","Autodetect target machine",oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[CHotUpdateSystem::eXBOX360]);

	LoadValue( "Settings\\HotUpdateSystem\\XBOX360","Target machine",strPlaceholderString);
	oHotUpdateSystemSettings.SetPlatformTargetMachine(CHotUpdateSystem::eXBOX360,strPlaceholderString);

	LoadValue( "Settings\\HotUpdateSystem\\XBOX360","Console root directory",strPlaceholderString);
	oHotUpdateSystemSettings.SetPlatformConsoleRootDirectory(CHotUpdateSystem::eXBOX360,strPlaceholderString);

	LoadValue( "Settings\\HotUpdateSystem\\XBOX360","Console executable name",strPlaceholderString);
	oHotUpdateSystemSettings.SetPlatformExecutableName(CHotUpdateSystem::eXBOX360,strPlaceholderString);

	LoadValue( "Settings\\HotUpdateSystem\\PLAYSTATION3","Is enabled",oHotUpdateSystemSettings.boPlatformEnabled[CHotUpdateSystem::ePLAYSTATION3]);
	LoadValue( "Settings\\HotUpdateSystem\\PLAYSTATION3","Autodetect target machine",oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[CHotUpdateSystem::ePLAYSTATION3]);

	LoadValue( "Settings\\HotUpdateSystem\\PLAYSTATION3","Target machine",strPlaceholderString);
	oHotUpdateSystemSettings.SetPlatformTargetMachine(CHotUpdateSystem::ePLAYSTATION3,strPlaceholderString);

	LoadValue( "Settings\\HotUpdateSystem\\PLAYSTATION3","Console root directory",strPlaceholderString);
	oHotUpdateSystemSettings.SetPlatformConsoleRootDirectory(CHotUpdateSystem::ePLAYSTATION3,strPlaceholderString);

	LoadValue( "Settings\\HotUpdateSystem\\PLAYSTATION3","Console executable name",strPlaceholderString);
	oHotUpdateSystemSettings.SetPlatformExecutableName(CHotUpdateSystem::ePLAYSTATION3,strPlaceholderString);

	//LoadValue( "Settings\\HotUpdateSystem","Sync Camera", oHotUpdateSystemSettings.boSyncCamera);
	//LoadValue( "Settings\\HotUpdateSystem","Realtime Sync",oHotUpdateSystemSettings.boRealtimeSyncEnabled);

	//////////////////////////////////////////////////////////////////////////
	// Time of day settings
	//////////////////////////////////////////////////////////////////////////
	LoadValue( "Settings\\TimeOfDay","ShowBasicProperties", bShowBasicPropertiesInTimeOfDay);


	//////////////////////////////////////////////////////////////////////////
	// Deep Selection Settings
	//////////////////////////////////////////////////////////////////////////
	LoadValue( "Settings", "DeepSelectionNearness", deepSelectionSettings.fRange);


	//////////////////////////////////////////////////////////////////////////
	// Load paths.
	//////////////////////////////////////////////////////////////////////////
	for (int id = 0; id < EDITOR_PATH_LAST; id++)
	{
		int i = 0;
		searchPaths[id].clear();
		while (true)
		{
			CString key;
			key.Format( "Path_%.2d_%.2d",id,i );
			CString path;
			LoadValue( "Paths",key,path );
			if (path.IsEmpty())
				break;
			searchPaths[id].push_back( path );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::PostInitApply()
{
	if (!gEnv || !gEnv->pConsole)
		return;

	// Create CVars.
	gEnv->pConsole->Register("ed_highlightGeometry",&viewports.bHighlightMouseOverGeometry,viewports.bHighlightMouseOverGeometry,0,"Highlight geomtry when mouse over it");

	GetIEditor()->SetEditorConfigSpec(editorConfigSpec);
}

//////////////////////////////////////////////////////////////////////////
// needs to be called after crysystem has been loaded
void SEditorSettings::LoadDefaultGamePaths()
{
	//////////////////////////////////////////////////////////////////////////
	// Default paths.
	//////////////////////////////////////////////////////////////////////////
	if (searchPaths[EDITOR_PATH_OBJECTS].empty())
		searchPaths[EDITOR_PATH_OBJECTS].push_back( Path::GetGameFolder()+"/Objects" );
	if (searchPaths[EDITOR_PATH_TEXTURES].empty())
		searchPaths[EDITOR_PATH_TEXTURES].push_back( Path::GetGameFolder()+"/Textures" );
	if (searchPaths[EDITOR_PATH_SOUNDS].empty())
		searchPaths[EDITOR_PATH_SOUNDS].push_back( Path::GetGameFolder()+"/Sounds" );
	if (searchPaths[EDITOR_PATH_MATERIALS].empty())
		searchPaths[EDITOR_PATH_MATERIALS].push_back( Path::GetGameFolder()+"/Materials" );
}

//////////////////////////////////////////////////////////////////////////
bool SEditorSettings::BrowseTerrainTexture(bool bIsSave)
{
	char path[MAX_PATH] = "";
	CString fileName;
	if(strlen(terrainTextureExport))
		fileName = terrainTextureExport;
	else
	{
		fileName = "terraintex.bmp";
		strcpy(path, Path::GamePathToFullPath(""));
	}

	if(bIsSave)
	{
		if(CFileUtil::SelectSaveFile( "Bitmap Image File (*.bmp)|*.bmp||","BMP", path, fileName ))
		{
			terrainTextureExport = fileName;
			return true;
		}
	}
	else
		if(CFileUtil::SelectFile( "Bitmap Image File (*.bmp)|*.bmp||", path, fileName ))
		{
			terrainTextureExport = fileName;
			return true;
		}
	return false;
}
