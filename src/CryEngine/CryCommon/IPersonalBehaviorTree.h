////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   IPersonalBehaviorTree.h
//  Created:     Ricardo Pillosu
//  Description: Interface of the PersonalBehaviorTree
//    A Personal Behavior Tree represents an AI's current state 
//    in the Behavior Selection System
//               
// -------------------------------------------------------------------------
//  History:
//  - ?
//  - 2 Mar 2009			 : Evgeny Adamenkov: Removed IRenderer
//
////////////////////////////////////////////////////////////////////////////

#include DEVIRTUALIZE_HEADER_FIX(IPersonalBehaviorTree.h)

#ifndef __I_PERSONAL_BEHAVIOR_TREE_H__
#define __I_PERSONAL_BEHAVIOR_TREE_H__
#pragma once

#if defined (DEVIRT_GEN_MANGLE_NAMES)
	#include "ITexture.h"
#endif

struct SBTProfile;
class SmartScriptTable;

UNIQUE_IFACE struct IPersonalBehaviorTree
{
	virtual bool ReloadProfile() = 0;
	virtual void Enable() = 0;
	virtual void Disable() = 0;
	virtual void Reset() = 0;
	virtual bool RunTree() = 0;
	virtual void AllowNodeChange() = 0;
	virtual bool IsWaitingForNodeChange() const = 0;
	virtual const char* GetNextNode() const = 0;
	virtual const char* GetCurrentNode() const = 0;
	virtual const char* GetPreviousNode() const = 0;
	virtual void GetNextNodePath( string & sPath ) const = 0;
	virtual void GetCurrentNodePath( string & sPath ) const = 0;
	virtual void GetPreviousNodePath( string & sPath ) const = 0;
	virtual bool CheckCondition( int iPreCompiledCode, bool bDebug = false ) const = 0;
	virtual bool CreateConditionsTable( SmartScriptTable &pTable ) const = 0;
	virtual SBTProfile* GetProfile() = 0;
	virtual SBTProfile const* GetProfile() const = 0;

	virtual void DebugDraw() = 0;
};

#endif // __I_PERSONAL_BEHAVIOR_TREE_H__
