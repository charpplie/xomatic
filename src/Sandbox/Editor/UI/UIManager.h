////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2010.
// -------------------------------------------------------------------------
//  File name:   UIManager.h
//  Version:     v1.00
//  Created:     10/9/2010 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __UIManager_H__
#define __UIManager_H__

#include <IFlashUI.h>
#include "LevelIndependentFileMan.h"

class CUIEditor;

class CUIManager : public ILevelIndependentFileModule, public IUIModule
{
public:
	CUIManager();
	~CUIManager();
	void Init();

	bool IsFlashEnabled() const;
	void ReloadActionGraphs(bool bReloadGraphs = false);
	void SaveChangedGraphs();
	bool HasModifications();
	bool NewUIAction( CString& filename );

	void ReloadScripts();

	//ILevelIndependentFileModule
	virtual bool PromptChanges();
	//~ILevelIndependentFileModule

	//IUIModule
	virtual bool EditorAllowReload();
	virtual void EditorReload();
	//~IUIModule

	void SetEditor(CUIEditor* pEditor) { m_pEditor = pEditor; }
	CUIEditor* GetEditor() const { return m_pEditor; }

	void EnableUpdates(bool bEnabled);

	static float CV_gfx_FlashReloadTime;
	static int CV_gfx_FlashReloadEnabled;

private:
	CString GetUIActionFolder();

private:
	CUIEditor* m_pEditor;

};

#endif // #ifndef __UIManager_H__