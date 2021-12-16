////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   LevelIndependentFileMan.cpp
//  Version:     v1.00
//  Created:     09/8/2011 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "LevelIndependentFileMan.h"

CLevelIndependentFileMan::CLevelIndependentFileMan()
{

}

CLevelIndependentFileMan::~CLevelIndependentFileMan()
{
	assert(m_Modules.size() == 0);
}

bool CLevelIndependentFileMan::PromptChangedFiles()
{
	for (std::vector<ILevelIndependentFileModule*>::iterator it = m_Modules.begin(); it != m_Modules.end(); ++it)
	{
		if (!(*it)->PromptChanges())
			return false;
	}
	return true;
}

void CLevelIndependentFileMan::RegisterModule(ILevelIndependentFileModule* pModule)
{
	stl::push_back_unique(m_Modules, pModule);
}
void CLevelIndependentFileMan::UnregisterModule(ILevelIndependentFileModule* pModule)
{
	stl::find_and_erase(m_Modules, pModule);
}