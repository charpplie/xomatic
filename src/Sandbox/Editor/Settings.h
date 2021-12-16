////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   settings.h
//  Version:     v1.00
//  Created:     14/1/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: General editor settings.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __settings_h__
#define __settings_h__
#pragma once

class CGrid;
class CryMutex;

struct SGizmoSettings
{
	float axisGizmoSize;
	bool axisGizmoText;
	int axisGizmoMaxCount;

	float helpersScale;

	SGizmoSettings();
};

#ifndef RGBA8
#define RGBA8(r,g,b,a)     ((uint32)(((uint8)(r)|((uint16)((uint8)(g))<<8))|(((uint32)(uint8)(b))<<16)) | (((uint32)(uint8)(a))<<24) )
#endif

//////////////////////////////////////////////////////////////////////////
// Settings for snapping in the viewports.
//////////////////////////////////////////////////////////////////////////
struct SSnapSettings
{
	SSnapSettings()
	{
		constructPlaneDisplay = false;
		constructPlaneSize = 5;

		markerDisplay = false;
		markerColor = RGB(0,200,200);
		markerSize = 1.0f;

		bGridUserDefined = false;
		bGridGetFromSelected = false;
	}

	// Display settings for construction plane.
	bool  constructPlaneDisplay;
	float constructPlaneSize;

	// Display settings for snapping marker.
	bool  markerDisplay;
	COLORREF markerColor;
	float markerSize;

	bool bGridUserDefined;
	bool bGridGetFromSelected;
};

//////////////////////////////////////////////////////////////////////////
// Settings for AVI encoding.
//////////////////////////////////////////////////////////////////////////
struct SAVIEncodingSettings
{
	SAVIEncodingSettings() : nFrameRate(25),codec("DIVX") {};
	
	int nFrameRate;
	CString codec;
};


//////////////////////////////////////////////////////////////////////////
// Settings for deep selection.
//////////////////////////////////////////////////////////////////////////
struct SDeepSelectionSettings
{
	SDeepSelectionSettings() : fRange(1.f){}

	//! If there are other objects hit within this value, one of them needs
	//! to be selected by user.
	//! If this value is 0.f, then deep selection mode won't work.
	float fRange;
};

//////////////////////////////////////////////////////////////////////////
struct SHyperGraphColors
{
	SHyperGraphColors()
	{
		colorArrow = RGBA8(0,220,255,255);
		colorInArrowHighlighted = RGBA8(220,0,110,255);
		colorOutArrowHighlighted = RGBA8(110,0,220,255);
		colorArrowDisabled = RGBA8(50,50,50,255);
		colorNodeOutline = RGBA8(255,255,255,255);
		colorNodeBkg = RGBA8(210,210,210,255);
		colorNodeBkgSelected = RGBA8(255,255,245,255);
		colorText = RGBA8(0,0,0,255);
		colorBackground = RGBA8(140,140,140,255);
		colorGrid = RGBA8(145,145,145,255);
	}

	COLORREF colorArrow;
	COLORREF colorInArrowHighlighted;
	COLORREF colorOutArrowHighlighted;
	COLORREF colorArrowDisabled;
	COLORREF colorNodeOutline;
	COLORREF colorNodeBkg;
	COLORREF colorNodeBkgSelected;
	COLORREF colorText;
	COLORREF colorBackground;
	COLORREF colorGrid;
};

//////////////////////////////////////////////////////////////////////////
struct SViewportsSettings
{
	//! If enabled always show entity radiuse.
	bool bAlwaysShowRadiuses;
	//! If enabled always display boxes for prefab entity.
	bool bAlwaysDrawPrefabBox;
	//! If enabled always display objects inside prefab.
	bool bAlwaysDrawPrefabInternalObjects;
	//! True if 2D viewports will be synchronized with same view and origin.
	bool bSync2DViews;
	//! Camera FOV for perspective View.
	float fDefaultFov;
	//! Camera Aspect Ratio for perspective View.
	float fDefaultAspectRatio;
	//! Show safe frame.
	bool bShowSafeFrame;
	//! To highlight selected geometry.
	bool bHighlightSelectedGeometry;
	
	//! To highlight selected geometry.
	int bHighlightMouseOverGeometry;

	//! If enabled will always display entity labels.
	bool bDrawEntityLabels;
	//! Show Trigger bounds.
	bool bShowTriggerBounds;
	//! Show Trigger bounds.
	bool bShowIcons;
	//! Fill Selected Shapes.
	bool bFillSelectedShapes;

	// Swap X/Y in map viewport.
	bool bTopMapSwapXY;
	// Texture resolution in the Top map viewport.
	int nTopMapTextureResolution;
};

//////////////////////////////////////////////////////////////////////////
struct SGUI_Settings
{
	bool bWindowsVista;        // true when running on windows Vista
	bool bSkining;             // Skining enabled.
	HFONT hSystemFont;         // Default system GUI font.
	HFONT hSystemFontBold;     // Default system GUI bold font.
	HFONT hSystemFontItalic;   // Default system GUI italic font.
	int nDefaultFontHieght;    // Default font height for 8 logical units.
};

//////////////////////////////////////////////////////////////////////////
struct STextureBrowserSettings
{
	int nCellSize; // Stores the default texture maximum cell size for 
};

//////////////////////////////////////////////////////////////////////////

struct SAssetBrowserSettings
{
	int nThumbSize; // Stores the default thumb size
};

//////////////////////////////////////////////////////////////////////////
class CHotUpdateSystem
{
public:
	enum eConsolePlatforms
	{
		eFirstPlatform=0,

		eXBOX360=0,
		ePLAYSTATION3=1,

		eNumberOfPlatforms=2,
	};

	bool			boShowHotUpdateProgress;

	bool			boPlatformEnabled[eNumberOfPlatforms];
	bool      boPlatformAutodetectTargetMachine[eNumberOfPlatforms];


	bool			boSyncCamera;
	bool			boRealtimeSyncEnabled;

	CString&				GetPlatformConsoleRootDirectory(int ePlatform,CString&	strTargetString);
	const CString&	SetPlatformConsoleRootDirectory(int ePlatform,const CString&	strSourceString);

	CString&				GetPlatformTargetMachine(int ePlatform,CString&	strTargetString);
	const CString&	SetPlatformTargetMachine(int ePlatform,const CString&	strSourceString);

	CString&				GetPlatformExecutableName(int ePlatform,CString&				strTargetString);
	const CString&	SetPlatformExecutableName(int ePlatform,const CString&	strSourceString);


	CString&				GetPlatformConsoleRootDirectoryNoLock(int ePlatform,CString&	strTargetString);
	CString&				GetPlatformTargetMachineNoLock(int ePlatform,CString&	strTargetString);
	CString&				GetPlatformExecutableNameNoLock(int ePlatform,CString&	strTargetString);
protected:
	static CryMutex&	GetIOMutex();
	CString		strPlatformConsoleRootDirectory[eNumberOfPlatforms];
	CString		strPlatformTargetMachine[eNumberOfPlatforms];
	CString		strPlatformExecutableName[eNumberOfPlatforms];
};
//////////////////////////////////////////////////////////////////////////
/** Various editor settings.
*/
struct SEditorSettings
{
	SEditorSettings();
	void	Save();
	void	Load();

	// needs to be called after crysystem has been loaded
	void	LoadDefaultGamePaths();

	void PostInitApply();

	bool BrowseTerrainTexture(bool bIsSave);

	//////////////////////////////////////////////////////////////////////////
	// Variables.
	//////////////////////////////////////////////////////////////////////////
	int undoLevels;

	//! Speed of camera movement.
	float cameraMoveSpeed;
	float cameraFastMoveSpeed;
	float wheelZoomSpeed;

	//! Hide mask for objects.
  int objectHideMask;

	//! Selection mask for objects.
	int objectSelectMask;

	//////////////////////////////////////////////////////////////////////////
	// Viewport settings.
	//////////////////////////////////////////////////////////////////////////
	SViewportsSettings viewports;

	//////////////////////////////////////////////////////////////////////////
	// Files.
	//////////////////////////////////////////////////////////////////////////
	bool bBackupOnSave;

	//////////////////////////////////////////////////////////////////////////
	// Autobackup.
	//////////////////////////////////////////////////////////////////////////
	//! Save auto backup file every autoSaveTime minutes.
	int autoBackupTime;
	//! When this variable set to true automatic file backup is enabled.
	bool autoBackupEnabled;
	//! After this amount of minutes message box with reminder to save will pop on.
	int autoRemindTime;
	//! Autobackup filename.
	CString autoBackupFilename;
	//////////////////////////////////////////////////////////////////////////


	//! If true preview windows is displayed when browsing geometries.
	bool bPreviewGeometryWindow;
	//! If true display geometry browser window for brushes and simple entities.
	bool bGeometryBrowserPanel;

	//! Pointer to currently used grid.
	CGrid *pGrid;

	SGizmoSettings gizmo;

	// Settigns of the snapping.
	SSnapSettings snap;

	//! Source Control Enabling.
	bool enableSourceControl;

	//! External layers: Save only changed.
	bool saveOnlyModified;

	//! Text editor.
	CString textEditorForScript;
	CString textEditorForShaders;

	//////////////////////////////////////////////////////////////////////////
	//! Editor data search paths.
	std::vector<CString> searchPaths[10]; // EDITOR_PATH_LAST

	// This directory is related to the editor root.
	CString								strStandardTempDirectory;

	SAVIEncodingSettings aviSettings;

	SGUI_Settings gui;

	SHyperGraphColors hyperGraphColors;
	bool              bFlowGraphMigrationEnabled;
	bool              bFlowGraphShowNodeIDs;
	bool              bFlowGraphShowToolTip;
	bool              bApplyConfigSpecInEditor;

	ESystemConfigSpec editorConfigSpec;

	//! Terrain Texture Export/Import filename.
	CString	terrainTextureExport;

	// Read only parameter.
	// Refects the status of GetIEditor()->GetOperationMode
	// To change current operation mode use GetIEditor()->SetOperationMode
	// see EOperationMode
	int operationMode;

	// For the texture browser configurations.
	STextureBrowserSettings sTextureBrowserSettings;

	// For the asset browser configurations.
	SAssetBrowserSettings sAssetBrowserSettings;

	// For hot update system configurations.
	CHotUpdateSystem				oHotUpdateSystemSettings;

	// For Terrain Texture Generation Multiplier.
	float fBrMultiplier;

	// For Console window background
	COLORREF colorConsoleBG;

	// Time of day
	bool bShowBasicPropertiesInTimeOfDay;

	// Deep Selection Mode Settings
	SDeepSelectionSettings deepSelectionSettings;


private:
	void SaveValue( const char *sSection,const char *sKey,int value );
	void SaveValue( const char *sSection,const char *sKey,COLORREF value );
	void SaveValue( const char *sSection,const char *sKey,float value );
	void SaveValue( const char *sSection,const char *sKey,const CString &value );

	void LoadValue( const char *sSection,const char *sKey,int &value );
	void LoadValue( const char *sSection,const char *sKey,COLORREF &value );
	void LoadValue( const char *sSection,const char *sKey,float &value );
	void LoadValue( const char *sSection,const char *sKey,bool &value );
	void LoadValue( const char *sSection,const char *sKey,CString &value );
	void LoadValue( const char *sSection,const char *sKey,ESystemConfigSpec &value);
};

//! Single instance of editor settings for fast access.
SANDBOX_API extern SEditorSettings gSettings;


#endif // __settings_h__
