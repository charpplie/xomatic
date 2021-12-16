/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Header for CTweakMenuController

-------------------------------------------------------------------------
History:
- 04:02:2009  : Created by Kevin Kirst

*************************************************************************/

#ifndef __TWEAKMENUCONTROLLER_H__
#define __TWEAKMENUCONTROLLER_H__

#pragma once

#include "TweakMenu.h"
#include "TweakTraverser.h"
#include "ITweakMenuController.h"

//-----------------------------------------------------------------------------------------------------
// Forward declarations
class CAtomicFiles;

//-----------------------------------------------------------------------------------------------------

class CTweakMenuController : public ITweakMenuController
{
public:
	CTweakMenuController();
	
	virtual ~CTweakMenuController(void);

	void Init();
	virtual void Release();

	virtual void Update(float fDeltaTime);

	virtual bool OnAction(ETweakAction eTweakAction, int activationMode, float value);

	// Save Tweak values for current Profile
	// Autosave implies that only an invisible shadow save for recovery should be performed
	void SaveProfile( bool autoSave = false );

	// Load Tweak values for all Profiles
	void LoadAllProfiles( void );

	// Apply active Profile values
	bool ApplyActiveProfile(void);

	// Discover defaults settings for all Tweaks
	void GetDefaults();

	void GetMemoryStatistics(ICrySizer * s);


protected:
	// Colors to use 
	enum ETextColor {
		eTC_Red,
		eTC_Green,
		eTC_Blue,
		eTC_Yellow,
		eTC_Cyan,
		eTC_Magenta,
		eTC_White,
	};

	// Methods to use to draw text
	enum EDrawTextMode {
		eDTM_Normal,
		eDTM_Wrap,
		eDTM_Scroll,
	};

	// Draw the menu
	void DrawMenu();

	// Reset the menu pane
	void ResetMenuPane(void);

	// Print line to menu pane
	void PrintToMenuPane( const char * line, ETextColor colour, float alpha=1.f, bool isSelected=false, int indentLevel=0, EDrawTextMode eMode=eDTM_Normal );

	// Return a string representing the current path through the menu
	string GetMenuPath(void) const;

	// Fetch the current Profile table
	bool FetchProfileTable( SmartScriptTable &table );

	// Recursively apply a Profile table
	bool RecursivelyApplyProfile(IScriptTable *profileTable);

	// Create an AtomicFiles list
	bool CreateProfileFileList( CAtomicFiles &atomicFiles, SmartScriptTable &scriptTable );

	// Get path to current profile
	string GetProfilePath(void);

	// Create Profiles dir if need be
	void CheckProfilesDir(void);

	// Convert directory paths for file searching
	void ConvertDirPath( string &path ) const;

	// Compile and execute a trivial Lua command
	// Use with caution
	bool ExecuteString(const char *sBuffer) const;

	// Modules interfaces used
	ILog *m_pLog;
	IScriptSystem *m_pScript;
	IRenderer *m_pRenderer;

	// Path to the Profiles folder
	string m_sProfilesPath;

	// The root Tweak menu
	CTweakMenu *m_menu;

	// Vector of strings type
	typedef std::vector<string> TStringVec;

	// Traverser to keep place in the menu
	CTweakTraverser m_traverser;	

	// Dimensions of menu pane
	float m_fX, m_fY;
	float m_fWidth, m_fHeight, m_fDescriptionHeight;

	// Font size and spacing
	float m_fFontSize, m_fVertSpacing;

	// State of console
	int m_nLineCount;

	// black texture
	uint32 m_nBlackTexID;

	IFFont *m_pDefaultFont;

	// Is the Tweak menu visible?
	bool m_bActive;

	// For autorepeat 
	int m_nRepeatAction;
	float m_fTimeHeld;

	// For autosave
	float m_fTimeSinceSave;

	// For showing hidden options
	bool m_bShowHidden;
};

#endif //__TWEAKMENUCONTROLLER_H__