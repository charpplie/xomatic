////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   LevelIndependentFileMan.h
//  Version:     v1.00
//  Created:     09/8/2011 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __LevelIndependentFileMan_H__
#define __LevelIndependentFileMan_H__

struct ILevelIndependentFileModule
{
	// this function should prompt some message box if changed files need to be saved
	// if return false, editor will not continue with current action (e.g close the editor)
	virtual bool PromptChanges() = 0;
};

class CLevelIndependentFileMan
{
public:
	CLevelIndependentFileMan();
	~CLevelIndependentFileMan();

	bool PromptChangedFiles();

	void RegisterModule(ILevelIndependentFileModule* pModule);
	void UnregisterModule(ILevelIndependentFileModule* pModule);
private:
	std::vector<ILevelIndependentFileModule* > m_Modules;
};

#endif