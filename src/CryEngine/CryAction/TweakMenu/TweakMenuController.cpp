/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	CTweakMenuController - Draws the tweak menu

-------------------------------------------------------------------------
History:
- 04:02:2009  : Created by Kevin Kirst

*************************************************************************/


#include "StdAfx.h"
#include "TweakMenuController.h"
#include "TweakItemSimple.h"
#include "CryActionCVars.h"
#include "LuaVarUtils.h"
#include "AtomicFiles.h"
#include <errno.h> 
// Note that <errno.h> may not be required locally, but still on the build system

#define PROFILE_FOLDER "/Profiles/Tweaks" 
#define TWEAK_RESCUE "TweakRescue.txt"
#define PROFILE_TABLES "Tweaks.Saving.Profiles"
#define TEMP_TABLE "Tweaks.Saving.Temp"
#define PROFILE_DEFAULT "Standard"
#define LUA_FUN_DEFAULTS "Tweaks.Saving.GetDefaultValues"
#define LUA_FUN_SAVE "Tweaks.Saving.GetSaveValues"
#define LUA_FUN_RELOAD_STRUCTURE "Tweaks.ReloadStructure"

// NOTE [Kevin] Xbox requires "*" for finding files in directory



	#define SEARCH_STRING "*.*"



static const float INDENT_AMOUNT = 15.0f;

//-----------------------------------------------------------------------------------------------------

CTweakMenuController::CTweakMenuController()
{
	m_pLog = gEnv->pLog;
	m_pScript = gEnv->pScriptSystem;
	m_pRenderer = gEnv->pRenderer;

	m_fX = 150.0f;
	m_fY = 75.0f;

	m_fWidth = 650.0f;
	m_fHeight = 500.0f;
	m_fDescriptionHeight = (m_fHeight*0.15f);
	
	m_fTimeHeld = 0.0f;
	m_fTimeSinceSave = 0.0f;

	m_fFontSize = 1.5f;
	m_fVertSpacing = 24.0f;

	m_bActive = false;
	m_bShowHidden = false;

	m_nRepeatAction = -1;

	m_menu = new CTweakMenu();
	m_traverser = m_menu->GetTraverser();
	m_traverser.First();
  
	m_nBlackTexID = gEnv->pRenderer->EF_LoadTexture("Textures/Gui/black.dds",FT_NOMIPS|FT_DONT_STREAM,eTT_2D)->GetTextureID();

	m_pDefaultFont = gEnv->pCryFont->GetFont("default");
	CRY_ASSERT(m_pDefaultFont);

	m_sProfilesPath = PathUtil::MakeFullPath( PathUtil::GetGameFolder() + PROFILE_FOLDER );
	ConvertDirPath(m_sProfilesPath);

	// Discover defaults
	GetDefaults();

	// Trigger menu to be created on each level load
	LuaVarUtils::SetRecursive("Tweaks.Scratchpad.RELOAD", true);

	// Saving isn't supported on PS3 so it is helpful not to warn of changes




	Init();
}

//-----------------------------------------------------------------------------------------------------

CTweakMenuController::~CTweakMenuController(void)
{
	// Check if we need to save on exit
	bool bSave = false;
	LuaVarUtils::GetRecursive("Tweaks.Scratchpad.SAVEONEXIT", bSave);

	if (bSave) 
		SaveProfile();

	// Handle exiting game with tweak menu active
	if (m_bActive)
	{
		OnAction(eTA_Exit,0,0);
	}

	gEnv->pRenderer->RemoveTexture(m_nBlackTexID);

	SAFE_DELETE(m_menu);
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::Init() 
{
	SmartScriptTable scratchpad;
	if (!LuaVarUtils::GetRecursive("Tweaks.Scratchpad", scratchpad)) 
		return;

	// Check if we should load the Tweak values
	bool bReloadAllValues = false;
	scratchpad->GetValue("RELOADVALUES",bReloadAllValues);
	if (bReloadAllValues) {
		bReloadAllValues = false;
		scratchpad->SetValue("RELOADVALUES",bReloadAllValues);
		LoadAllProfiles();
	}
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::Release()
{
	delete this;
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::Update(float fDeltaTime) 
{
	if (!m_bActive)
	{
		return;
	}
	if (fDeltaTime < FLT_EPSILON)
	{
		fDeltaTime = 0.1f;
	}

	SmartScriptTable scratchpad;
	if (!LuaVarUtils::GetRecursive("Tweaks.Scratchpad", scratchpad)) 
		return;

	// Keep track of time since autosave
	// Save is triggered from input code
	m_fTimeSinceSave += fDeltaTime;
	
	// Autorepeat timing checks
	if (m_nRepeatAction >= 0) 
	{
		// Update timer
		m_fTimeHeld += fDeltaTime;
		
		float fRepeatRate = 0.2f, fRepeatDelay = 0.4f;
		scratchpad->GetValue("REPEATRATE", fRepeatRate);
		scratchpad->GetValue("REPEATDELAY", fRepeatDelay);

		if (m_fTimeHeld > fRepeatDelay) 
		{
			m_fTimeHeld -= fRepeatRate;
			// Re-execute the action, as if from the action system
			OnAction((ETweakAction)m_nRepeatAction, eAAM_OnPress, 0.0f);
		}
	}

	// Check if the menu needs reloading
	bool bReload = false;
	scratchpad->GetValue("RELOAD",bReload);
	if (bReload) 
	{
		SmartScriptTable menus;
		if (LuaVarUtils::GetRecursive("Tweaks.Menus.Main", menus)) 
		{
			SAFE_DELETE (m_menu);
			m_menu = new CTweakMenu(menus.GetPtr());

			m_traverser = m_menu->GetTraverser();
			m_traverser.First();
		}
		bReload = false;
		scratchpad->SetValue("RELOAD",bReload);
	}

	// Check if we should reload the Tweak values
	bool bReloadAllValues = false;
	scratchpad->GetValue("RELOADVALUES",bReloadAllValues);
	if (bReloadAllValues) {
		bReloadAllValues = false;
		scratchpad->SetValue("RELOADVALUES",bReloadAllValues);
		LoadAllProfiles();
	}

	// Check if we should save the LUA Tweak values
	bool bSaveAllValues = false;
	scratchpad->GetValue("SAVEVALUES",bSaveAllValues);
	if (bSaveAllValues) {
		bSaveAllValues = false;
		scratchpad->SetValue("SAVEVALUES",bSaveAllValues);
		SaveProfile();
	}

	// Check if we should apply the active Profile
	bool bApplyProfile = false;
	scratchpad->GetValue("APPLYPROFILE",bApplyProfile);
	if (bApplyProfile) {
		bApplyProfile = false;
		scratchpad->SetValue("APPLYPROFILE",bApplyProfile);
		ApplyActiveProfile();
	}

	// let the script know whether the menu is active or not 
	scratchpad->SetValue("MENUACTIVE",m_bActive);

	// Draw the menu
	if (m_bActive)
	{
		m_traverser.Update(fDeltaTime);
		DrawMenu();
	}
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::DrawMenu() 
{
	IRenderer *pRenderer = gEnv->pRenderer;
	CRY_ASSERT(pRenderer);

	// Update some values from Lua
	LuaVarUtils::GetRecursive("Tweaks.Scratchpad.FONTSIZE",m_fFontSize);
	LuaVarUtils::GetRecursive("Tweaks.Scratchpad.VERTSPACE",m_fVertSpacing);
	LuaVarUtils::GetRecursive("Tweaks.Scratchpad.SHOWHIDDEN",m_bShowHidden);

	ResetMenuPane();

	bool displayBackground=true;
	float backgroundAlpha = 0.70f;
	SmartScriptTable scratchpad;
	if (LuaVarUtils::GetRecursive("Tweaks.Scratchpad", scratchpad)) 
	{
		scratchpad->GetValue("BLACKBACKGROUND", displayBackground);
		if (scratchpad->GetValue("BLACKBACKGROUNDALPHA", backgroundAlpha))
		{
			// Tonyproofing
			backgroundAlpha=(backgroundAlpha<0) ? 0 : (backgroundAlpha>1) ? 1 : backgroundAlpha;
			scratchpad->SetValue("BLACKBACKGROUNDALPHA", backgroundAlpha);
		}
	}
	
	if (displayBackground)
	{
		// draw a semi-transparent black box background to make
		// sure the menu text is visible on any background		
		pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);	
		const float fHeightMinusDesc = m_fHeight-m_fDescriptionHeight;
		pRenderer->Draw2dImage(80.f,50.f,m_fWidth,fHeightMinusDesc,m_nBlackTexID,0,1,1,0,0,0,0,0,backgroundAlpha);
		pRenderer->Draw2dImage(80.f,fHeightMinusDesc+55.0f,m_fWidth,m_fDescriptionHeight,m_nBlackTexID,0,1,1,0,0,0,0,0,backgroundAlpha);
	}

	// Set up renderer state
	pRenderer->SetState(GS_BLSRC_SRCALPHA|GS_BLDST_ONE|GS_NODEPTHTEST);

	{
		string title;
		const char *psCvar=CCryActionCVars::Get().g_TweakProfile->GetString();
		title.Format("Tweak Menu - Profile: %s - %s",psCvar ? psCvar : "",gEnv->bServer?"Server":"Client");
		PrintToMenuPane(title.c_str(), eTC_White );
	}

	// Display the menu path
	PrintToMenuPane( GetMenuPath().c_str(), eTC_Cyan );

	// Update line visible count with remaining space (in 1024x768 res)
	const float fRemainingSpace = ((m_fHeight-m_fDescriptionHeight)*(1.f/600.f)*768.f) - (m_fY+m_fVertSpacing*m_nLineCount);
	m_traverser.SetLineVisibleCount(int(fRemainingSpace/m_fVertSpacing));

	// Get a Traverser pointing at the top of this menu
	CTweakTraverser itemIter = m_traverser;
	itemIter.Top();

	string sDescription = "Description: ";
	int nItemNumber = -1;
	const int nLineVisibleCount = m_traverser.GetLineVisibleCount();
	const int nLineStartOffset = m_traverser.GetLineStartOffset();

	while (itemIter.Next()) {
		// Skip items that are off the screen
		nItemNumber++;
		if (nItemNumber < nLineStartOffset)
		{
			float fColor[4] = {1.0f,0.0f,1.0f,1.0f};
			pRenderer->Draw2dLabel(m_fX - 15.0f, m_fY + m_fVertSpacing * m_nLineCount, m_fFontSize, fColor, false, "*");
			continue;
		}
		else if (nItemNumber > nLineVisibleCount + nLineStartOffset)
		{
			float fColor[4] = {1.0f,0.0f,1.0f,1.0f};
			pRenderer->Draw2dLabel(m_fX - 15.0f, m_fY + m_fVertSpacing * (m_nLineCount-1), m_fFontSize, fColor, false, "*");
			continue;
		}

		CTweakCommon * item = itemIter.GetItem();
		string text = item->GetName();

		if (item->IsHidden() && !m_bShowHidden)
			continue;

		// Accessorize by type
		ETextColor colour = eTC_White;
		switch (item->GetType()) {
			case CTweakCommon::eTT_Menu:
				colour = eTC_Yellow; 
				break;
			case CTweakCommon::eTT_Metadata:
				colour = eTC_White; 
				text += " " + (static_cast<CTweakMetadata*>(item))->GetValue();
				break;
			case CTweakCommon::eTT_ItemSimple:
				colour = eTC_White; 
				text += " " + (static_cast<CTweakItemSimple*>(item))->GetValue();
				break;
		}
		 
		// Is this the currently selected item?
		bool isSelected = false;
		if (itemIter == m_traverser) 
		{
			isSelected = true;
			sDescription += item->GetDescription();
			colour = eTC_Red;
		}

		// Display forced draw 'hidden' items slightly transparent
		float alpha = 1.f;
		if (item->IsHidden())
			alpha = 0.5f;

		// Display it
		PrintToMenuPane( text.c_str(), colour, alpha, isSelected, 1, (isSelected?eDTM_Scroll:eDTM_Normal) );
	}

	// Draw description of selected item at bottom
	m_nLineCount += (max(nLineVisibleCount-nItemNumber,0) + 2);
	PrintToMenuPane( sDescription.c_str(), eTC_Cyan, 1.0f, false, 1, eDTM_Wrap );

	// Set back renderer state
	pRenderer->SetState(GS_BLSRC_SRCALPHA|GS_BLDST_ONEMINUSSRCALPHA|GS_NODEPTHTEST);
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::ResetMenuPane(void) 
{
	m_nLineCount = 0;
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::PrintToMenuPane( const char * line, ETextColor color, float alpha, bool isSelected, int indentLevel, EDrawTextMode eMode ) 
{
	IRenderer *pRenderer = gEnv->pRenderer;
	CRY_ASSERT(pRenderer);

	float fColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	float& fR = fColor[0];
	float& fG = fColor[1];
	float& fB = fColor[2];
	float fI = 1.0f;
	switch (color) {
		case eTC_Red:				
			fR = fI; break;		// Selection
		case eTC_Green:
			fG = fI; break;		// Tweaks
		case eTC_Blue:
			fB = fI; break;		// Path
		case eTC_Yellow:
			fR = fG = fI; break;// Submenus
		case eTC_White:
			fR = fG = fB = fI; break; // Default
		case eTC_Cyan:
			fR=0.0f; fG=1.0f; fB=1.0f; break;
		case eTC_Magenta:
			fR=1.0f; fG=0.0f; fB=1.0f; break;
	}

	float posX(m_fX);
	float posY(m_fY + m_fVertSpacing * m_nLineCount);

	float fIndentLevel = CLAMP( indentLevel, 0, 10 ) * INDENT_AMOUNT;

	if (isSelected)
	{
		alpha = 1.f; // Always draw selected options at full opacity
		pRenderer->Draw2dLabel(posX + fIndentLevel - 15.f, posY, m_fFontSize, fColor, false, ">");
	}

	int nLineCount = 0;
	int nCharOffset = 0;
	string sLine = line;
	
	const int nLineSize = strlen(line);
	const vector2f vTextSize = m_pDefaultFont->GetTextSize(line);
	const float fTextWidth = vTextSize.x;
	const float fMaxTextWidth = m_fWidth-fIndentLevel*2.0f;
	int nExtractSize = int(fMaxTextWidth / (fTextWidth / (float)nLineSize));
	
	switch (eMode)
	{
		case eDTM_Scroll:
		{
			// Scroll text by incrementing character offset over time
			nCharOffset = (int)max(m_traverser.GetSelectionTimer()-2.0f,0.0f);
			nCharOffset = min(nCharOffset, max(nLineSize-nExtractSize,0));
			pRenderer->Draw2dLabel(posX + fIndentLevel, posY, m_fFontSize, fColor, false, sLine.substr(nCharOffset,nExtractSize).c_str());
			nLineCount = 1;
		}
		break;

		case eDTM_Wrap:
		{
			// Perform text wrap
			do 
			{
				string::size_type nLastSpace = (nCharOffset+nExtractSize > nLineSize ? nLineSize : sLine.find_last_of(' ', nCharOffset+nExtractSize) + 1);
				pRenderer->Draw2dLabel(posX + fIndentLevel, posY + m_fVertSpacing*nLineCount, m_fFontSize, fColor, false, sLine.substr(nCharOffset,(nLastSpace-nCharOffset)).c_str());
				nCharOffset = nLastSpace;
				nLineCount++;
			} while (nCharOffset < nLineSize);
		}
		break;

		// Just print out the whole line (up to extraction size)
		case eDTM_Normal:
		{
			pRenderer->Draw2dLabel(posX + fIndentLevel, posY, m_fFontSize, fColor, false, sLine.substr(0,nExtractSize).c_str());
			nLineCount = 1;
		}
		break;

		default:
			CRY_ASSERT_MESSAGE(0, "CTweakMenuController::PrintToMenuPane Non-implemented version of EDrawTextMode used");
	}

	m_nLineCount += nLineCount;
}


//-----------------------------------------------------------------------------------------------------

bool CTweakMenuController::OnAction(ETweakAction eTweakAction, int activationMode, float value) 
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if((eTweakAction != eTA_Enter) && !m_bActive)
	{
		return false;
	}

	bool bCheckAutoSave = false;

	if (activationMode == eAAM_OnPress)
	{
		// Only actions to be repeated send an OnPress event
		m_nRepeatAction = eTweakAction;
	}
	else if (activationMode == eAAM_OnRelease)
	{
		// If there was a repeating action, OnRelease _just_ clears it
		if (m_nRepeatAction >= 0)
		{
			m_nRepeatAction = -1;
			m_fTimeHeld = 0.0f;

			// Don't then trigger a final action
			return true;
		} 

		// Otherwise OnRelease causes a single action 
		// I.e. for a non-repeating action, just set the OnRelease flag
	}

	// make enter a toggle
	if (eTweakAction == eTA_Enter && m_bActive)
		eTweakAction = eTA_Exit;

	// Check and perform the actions
	if (eTweakAction == eTA_Enter)
	{
		CRY_ASSERT(!m_bActive);

		// Check if the Tweak menu has been successfully loaded yet
		// If not, try again at this point. Mainly only useful for coder debugging.
		SmartScriptTable menus;
		if (!LuaVarUtils::GetRecursive("Tweaks.Menus.Main", menus)) 
		{
			LuaVarUtils::SetRecursive("Tweaks.Scratchpad.RELOAD",true);

			// Call Lua function to get text dump of changes table
			ExecuteString(string(LUA_FUN_RELOAD_STRUCTURE) + "();");
		}
	
		m_bActive = true;

		/*if (!gEnv->bMultiplayer)
		{
			CCryAction::GetCryAction()->PauseGame(true, true);
		}*/

	}
	else if (eTweakAction == eTA_Exit)
	{
		CRY_ASSERT(m_bActive);

		m_bActive = false;

		/*if (!gEnv->bMultiplayer)
		{
			CCryAction::GetCryAction()->PauseGame(false, true);
		}*/

	}
	else if (eTweakAction == eTA_Left)
	{
		CRY_ASSERT(m_bActive);
		m_traverser.Back();
	}
	else if (eTweakAction == eTA_Right)
	{
		CRY_ASSERT(m_bActive);
		m_traverser.Forward();
	}
	else if (eTweakAction == eTA_Up)
	{
		CRY_ASSERT(m_bActive);

		// Keep traversing until we find an unhidden item or have tried them all
		int count = (int)m_traverser.GetMenuSize();
		do 
		{
			--count;
			if (!m_traverser.Previous())
				m_traverser.Last();
		} 
		while (count > 0 && m_traverser.GetItem()->IsHidden() && !m_bShowHidden);		 
	}
	else if (eTweakAction == eTA_Down)
	{
		CRY_ASSERT(m_bActive);

		// Keep traversing until we find an unhidden item or have tried them all
		int count = (int)m_traverser.GetMenuSize();
		do 
		{
			--count;
			if (!m_traverser.Next())
				m_traverser.First();
		} 
		while (count > 0 && m_traverser.GetItem()->IsHidden() && !m_bShowHidden);		 
	}
	else if (eTweakAction == eTA_Inc)
	{
		CRY_ASSERT(m_bActive);

		CTweakCommon *item = m_traverser.GetItem();
		if (item) 
		{
			if (item->GetType() == CTweakCommon::eTT_Metadata)
			{
				(static_cast<CTweakMetadata*>(item))->IncreaseValue();	
				bCheckAutoSave = true;
			}
			else if (item->GetType() == CTweakCommon::eTT_ItemSimple)
				(static_cast<CTweakItemSimple*>(item))->IncreaseValue();		
		}
	}
	else if (eTweakAction == eTA_Dec)
	{
		CRY_ASSERT(m_bActive);

		CTweakCommon *item = m_traverser.GetItem();
		if (item) 
		{
			if (item->GetType() == CTweakCommon::eTT_Metadata)
			{
				(static_cast<CTweakMetadata*>(item))->DecreaseValue();	
				bCheckAutoSave = true;
			}
			else if (item->GetType() == CTweakCommon::eTT_ItemSimple)
				(static_cast<CTweakItemSimple*>(item))->DecreaseValue();
		}
	}

	// Autosave trigger
	// We only bother to check when a value may have been changed by the user, as a simple way
	// to cut down on redundant autosaves
	if (bCheckAutoSave && m_fTimeSinceSave > 600.0f)
	{
		SaveProfile(true);
		m_fTimeSinceSave = 0.0f;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------------

string CTweakMenuController::GetMenuPath(void) const 
{
	// Check validity
	if (!m_traverser.IsRegistered()) return "No valid menu";

	// Create a copy of the traverser we can use
	CTweakTraverser backTracker = m_traverser;

	// Form string to display of menu position
	string sPathText;
	do {
		sPathText = backTracker.GetMenu()->GetName() + "->" + sPathText;
	}	while (backTracker.Back());
	return sPathText;
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::SaveProfile( bool autoSave ) 
{
	string rescuePath = PathUtil::MakeFullPath( PathUtil::Make( PathUtil::GetGameFolder() + PROFILE_FOLDER, TWEAK_RESCUE ) );
	ConvertDirPath(rescuePath);

	CheckProfilesDir();

	// Call Lua function to get text dump of changes table
	ExecuteString(string(LUA_FUN_SAVE) + "();");
	
	SmartScriptTable flatProfileTable;
	LuaVarUtils::GetRecursive("Tweaks.Saving.SaveDump", flatProfileTable);


	if (!flatProfileTable.GetPtr())
	{
		m_pLog->Log("[Tweaks] Failed to collect Tweak values for save");
		ExecuteString("Tweaks.Saving.SaveFailed(\"Unrecoverable save failure, nothing could be saved\")");
		return;
	}

	// Form list of files to write
	CAtomicFiles atomicFiles;
	if (!CreateProfileFileList( atomicFiles, flatProfileTable))
	{
		// Nothing to save!
		if (!autoSave) 
			ExecuteString("Tweaks.Saving.SaveFailed(\"No changes to save\");");
		return;
	}

	if (autoSave)
	{
		// Simple dump procedure, non-robust, then early-out	
		atomicFiles.AppendToFile(rescuePath, "--Autosave" );
		m_pLog->Log("[Tweaks] Autosave complete");
		return;
} 


	// Try to lock files
	std::vector<string> failures;
	failures = atomicFiles.AcquireFiles();
	
	if (!failures.empty())
	{
		ExecuteString("Tweaks.Saving.SaveFailed(\"Failed - perhaps read-only? See log for details\")");
		m_pLog->LogError("[Tweaks] Failed to acquire write access to files for save:");
	}
	else
	{
		// Check if the files are up-to-date
		failures = atomicFiles.CheckCurrency();
		if (!failures.empty())
		{
			ExecuteString("Tweaks.Saving.SaveFailed(\"Failed - files have changed. See log for details\")");
			m_pLog->LogError("[Tweaks] Failed currency check on files for save:");
		}
		else
		{
			// Try to write the files
			failures = atomicFiles.WriteFiles();
			if (!failures.empty())
			{
				ExecuteString("Tweaks.Saving.SaveFailed(\"Failed - perhaps read-only? See log for details\")");
				m_pLog->LogError("[Tweaks] Failed to write files for save:");
			}
		}
	}

	if (!failures.empty())
	{
		// Log the files that failed
		for (std::vector<string>::iterator it = failures.begin(); it != failures.end(); ++it)
			m_pLog->LogError(it->c_str());
		
		// Try to append to the TweakRescue file
		if (!atomicFiles.AppendToFile( rescuePath, "--Save failed" ))
			atomicFiles.DumpToLog("Tweak value dump:");
	}
	else
	{
		m_pLog->Log("[Tweaks] Saved profile successfully");
	
		// Put them into the rescue file for consistency, don't worry if it fails
		atomicFiles.AppendToFile( rescuePath, "--Save completed" );

		// In the case of actually overwriting the previous settings, we update the timestamp for feedback
		ExecuteString("Tweaks.Saving.SaveComplete();");
	}
}

//-----------------------------------------------------------------------------------------------------

// (Re)load all Profiles
void CTweakMenuController::LoadAllProfiles( void ) 
{
	// Clear profiles
	string sLuaBuffer = string(PROFILE_TABLES) + " = {}";
	ExecuteString(sLuaBuffer);

	// Clear temporary table
	sLuaBuffer = string(TEMP_TABLE) + " = {}";
	ExecuteString(sLuaBuffer);

	// Load all scripts in all subfolders of the Profiles folder
	// We do this firmly outside of the pak structure
	string sProfilesSpec = PathUtil::Make( m_sProfilesPath , SEARCH_STRING );
	ConvertDirPath(sProfilesSpec);
	__finddata64_t fd;
	intptr_t handle = _findfirst64( sProfilesSpec, &fd);
	if (handle != -1)
	{
		// Iterate through all entries looking for folders
		do 
		{
			// Is it a real subdirectory?
			if (fd.attrib & _A_SUBDIR && fd.name[0]!='.')
			{
				// Prepare to iterate through all files in that profile subfolder
				const char *sProfileName = fd.name;
				string sFolderPath  = PathUtil::Make( m_sProfilesPath, sProfileName );
				// Useful for debug
				//m_pLog->Log((sFolderPath+string("(FOLDER)")).c_str());
				string sSpec = PathUtil::Make( sFolderPath, "*.lua" );
				ConvertDirPath(sSpec);
				__finddata64_t scriptFd;			// File descriptor for scripts in sub-folder
				intptr_t scriptHandle = _findfirst64( sSpec, &scriptFd);

				// If preparations succeeded we can start
				if (scriptHandle != -1)
				{
					// What path will this profile be given?
					string sProfileTable = string(PROFILE_TABLES) + "." + string(sProfileName);
					// Useful for debug
					//m_pLog->Log((sProfileTable+string("(PROFILE_TABLE)")).c_str());

					// Iterate through, executing each script file
					do 
					{
						// Form path and execute, which should result in a new entry in TEMP_TABLE
						string sScriptFileName = PathUtil::Make( sFolderPath, scriptFd.name );
						ConvertDirPath(sScriptFileName);
						m_pScript->ExecuteFile(sScriptFileName.c_str(), true, true);
						// Useful for debug
						//m_pLog->Log((sScriptFileName+string("(SCRIPT_FILENAME)")).c_str());

						// Check that a table of the right name was created
						string sTableName = string(scriptFd.name);
						PathUtil::RemoveExtension(sTableName);
						sLuaBuffer = TEMP_TABLE;
						sLuaBuffer += string(".");
						sLuaBuffer += string(sTableName).MakeLower();
						ScriptAnyValue result = ANY_TNIL;
						LuaVarUtils::GetRecursive( sLuaBuffer, result );
						if (result.type != ANY_TTABLE)
						{
							m_pLog->LogWarning("[Tweaks] Tried to load file \"%s\" in profile \"%s\" but no matching table was created - (format last changed April '08 - is it older?) ", scriptFd.name, fd.name );
						}
					} while(0 == _findnext64 (scriptHandle, &scriptFd));
					_findclose(scriptHandle);

					// Transfer to profile table
					sLuaBuffer = sProfileTable + " = " + TEMP_TABLE;
					ExecuteString(sLuaBuffer);

					// Clear temporary table
					string sLuaBufferClearTable = string(TEMP_TABLE) + " = {}";
					ExecuteString(sLuaBufferClearTable);
				}
			}
		}while (0 == _findnext64 (handle, &fd));
		_findclose(handle);
	}

	// Prepare the profiles for use
	ExecuteString("Tweaks.Saving.PrepareProfiles()");

	m_pLog->Log("[Tweaks] Values loaded");
	
	ApplyActiveProfile();
}

//-----------------------------------------------------------------------------------------------------

bool CTweakMenuController::CreateProfileFileList( CAtomicFiles &atomicFiles, SmartScriptTable &flatProfileTable )
{
	bool nonEmpty = false;
	string profilePath = GetProfilePath();

	// Iterate over flatProfileTable ( a table of string table-dumps )
	IScriptTable::Iterator iter = flatProfileTable->BeginIteration();
	while (flatProfileTable->MoveNext(iter)) 
	{
		const char *content = iter.value.str;
		string filename = string(iter.sKey) + ".lua";
		string path = PathUtil::Make( profilePath, filename );
		ConvertDirPath(path);
		atomicFiles.AddFile(path, content);
		nonEmpty = true;
	}

	return nonEmpty;
}

//-----------------------------------------------------------------------------------------------------

bool CTweakMenuController::ApplyActiveProfile(void)
{
	// Note that the entirety of this application could be done in Lua, probably more neatly.

	// Fetch the table of saved values for the active profile
	SmartScriptTable profileTable;
	if (!FetchProfileTable(profileTable))
	{
		m_pLog->LogError("[Tweaks] Cannot locate active profile table");
		return false;
	}

	RecursivelyApplyProfile(profileTable.GetPtr());

	m_pLog->Log("[Tweaks] Values applied");
	return true;
}

//-----------------------------------------------------------------------------------------------------


bool CTweakMenuController::RecursivelyApplyProfile(IScriptTable *profileTable) 
{
	// Go through and apply active profile values
	// Identify and recurse on each element of the table
	IScriptTable::Iterator iter = profileTable->BeginIteration();
	while (profileTable->MoveNext(iter)) 
	{
		// Is this a subtable, or a console variable, or else should be a Lua variable
		ICVar *cvar = gEnv->pConsole->GetCVar(iter.sKey);
		if (iter.value.type == ANY_TTABLE) 
		{
			// Should be able to recurse on this
			RecursivelyApplyProfile(iter.value.table);
		}
		else if (cvar) 
		{
			// Figure out type - lame, shouldn't there be a tostring conversion?
			switch (cvar->GetType()) {
						case CVAR_INT:
						case CVAR_FLOAT:
							if (iter.value.type == ANY_TNUMBER)
								cvar->Set( iter.value.number );
							else if (iter.value.type == ANY_TBOOLEAN)
								cvar->Set( iter.value.b );
							else
								m_pLog->LogWarning("[Tweaks] While loading Tweak changes, types don't match (Lua should be number/bool)");
							break;
						case CVAR_STRING:
							if (iter.value.type == ANY_TSTRING)
								cvar->Set( iter.value.str );
							else if (iter.value.type == ANY_TBOOLEAN)
								cvar->Set( (iter.value.b ? "1" : "0") );
							else if (iter.value.type == ANY_TNUMBER)
								cvar->Set( iter.value.number );
							else
								m_pLog->LogWarning("[Tweaks] While loading Tweak changes, types don't match (Lua should be a string)");
							break;
			}
		}
		else 
		{
			// Try to set this as a Lua variable
			ScriptAnyValue tester;
			if (!LuaVarUtils::GetRecursive(iter.sKey, tester)) {
				m_pLog->LogWarning("[Tweaks] Expected Lua variable \"%s\" to already exist", iter.sKey);
			}
		LuaVarUtils::SetRecursive(iter.sKey, iter.value);
		}
	}
	profileTable->EndIteration(iter);
		
	return true;
}

//-----------------------------------------------------------------------------------------------------


void CTweakMenuController::GetDefaults( void ) 
{
	// Call Lua function to store defaults
	
	// [12/9/2009 evgeny] Scripts/Tweaks.lua may not have been loaded at this point -- adding ReloadScript
	if (!ExecuteString("Script.ReloadScript(\"scripts/Tweaks.lua\"); Tweaks.Saving.GetDefaultValues();"))
		m_pLog->Log("[Tweaks] Failed to store defaults");
}

//-----------------------------------------------------------------------------------------------------

bool CTweakMenuController::FetchProfileTable( SmartScriptTable &table ) 
{
	ICVar* pCVarProfile = CCryActionCVars::Get().g_TweakProfile;

	// Try to fetch the active profile table
	if (LuaVarUtils::GetRecursive( string(PROFILE_TABLES) + "." + pCVarProfile->GetString(), table )) 
		return true;
	// Try lowercase
	if (LuaVarUtils::GetRecursive( string(PROFILE_TABLES) + "." + string(pCVarProfile->GetString()).MakeLower(), table )) 
		return true;

	// Failed, perhaps because the CVAR is wrong or blank as yet. Try to find the default (lowercase).
	pCVarProfile->Set(string(PROFILE_DEFAULT).MakeLower());
	if (LuaVarUtils::GetRecursive( string(PROFILE_TABLES) + "." + string(pCVarProfile->GetString()), table ))
	{
		m_pLog->LogWarning("[Tweaks] No profile was active - selected default");
		return true;
	}

	// Failed, perhaps because the CVAR is wrong or blank as yet. Try to find the default.
	pCVarProfile->Set(string(PROFILE_DEFAULT));
	if (LuaVarUtils::GetRecursive( string(PROFILE_TABLES) + "." + string(pCVarProfile->GetString()), table ))
	{
		m_pLog->LogWarning("[Tweaks] No profile was active - selected Default");
		return true;
	}

	// Failed, create an empty default profile
	CheckProfilesDir();
	m_pLog->LogWarning("[Tweaks] No profiles found - created empty default");
	return (LuaVarUtils::GetRecursive( string(PROFILE_TABLES) + "." + pCVarProfile->GetString(), table ));
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::CheckProfilesDir(void)
{
	// Does the Tweak Profiles directory exist?
	string path = PathUtil::MakeFullPath( PathUtil::GetGameFolder() + PROFILE_FOLDER );
	ConvertDirPath(path);
	if (!_mkdir(path.c_str())) {
		m_pLog->Log("[Tweaks] Created Profiles directory for Tweaks save");
	} else if (errno == ENOENT) {
		// Can't actually conceive of a reason this would occur, but...
		m_pLog->Log("[Tweaks] No Save(?) - couldn't create directory for Tweaks save");
	}
	// EEXIST error code just denotes it was already there, which is fine

	// Does this specific Profile exist?
	if (!_mkdir( GetProfilePath().c_str())) {
		m_pLog->Log("[Tweaks] Created new folder to save this Profile");
	} else if (errno == ENOENT) {
		// Can't actually conceive of a reason this would occur, but...
		m_pLog->Log("[Tweaks] No Save(?) - couldn't create profile folder for Tweaks save");
	}
}

//-----------------------------------------------------------------------------------------------------

string CTweakMenuController::GetProfilePath(void)
{
	string result;

	const char * sProfile = CCryActionCVars::Get().g_TweakProfile->GetString();
	result = PathUtil::Make( PathUtil::GetGameFolder() + PROFILE_FOLDER, sProfile );
	result = PathUtil::MakeFullPath(result);
	ConvertDirPath(result);

	return result;
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::ConvertDirPath( string &path ) const
{
	// NOTE [Kevin] Xbox requires backslashes and full path for file searching

















}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
}

//-----------------------------------------------------------------------------------------------------

bool CTweakMenuController::ExecuteString(const char *sBuffer) const
{
	return gEnv->pScriptSystem->ExecuteBuffer( sBuffer, strlen(sBuffer));
}
