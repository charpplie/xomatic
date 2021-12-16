/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Script Binding for Item

 -------------------------------------------------------------------------
  History:
  - 17:12:2007   11:49 : Created by Mieszko Zielinski

*************************************************************************/
#ifndef __SCRIPTBLACKCBOARD_H__
#define __SCRIPTBLACKCBOARD_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IScriptSystem.h>
#include <ScriptHelpers.h>
#include <IBlackBoard.h>

class CBlackBoard : public IBlackBoard
{
public:
	CBlackBoard();

	VIRTUAL SmartScriptTable& GetForScript() { return m_BB; }
	VIRTUAL void							SetFromScript( SmartScriptTable& sourceBB );
	VIRTUAL void							Clear() { m_BB->Clear(); }
	
private:
	SmartScriptTable	m_BB;
};

#endif // __SCRIPTBLACKCBOARD_H__
