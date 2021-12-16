////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   commandmanager.h
//  Version:     v1.00
//  Created:     4/7/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __commandmanager_h__
#define __commandmanager_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "Command.h"

/** Implement this interface and register it to command manager to intercept commands
		before they executed.
*/
class	ICommandHandler
{
public:
	/** Called before this command is executed.
			@return Return false to stop command from executing.
	*/
	virtual	bool OnCommand( CCommand *command ) = 0;
};

/** Command manager stores all commands in the Editor.
*/
class	CCommandManager
{
public:
	enum
	{
		CUSTOM_COMMAND_ID_FIRST = 10000,
		CUSTOM_COMMAND_ID_LAST  = 15000,
	};

	typedef Functor0 CommandCallback;

	CCommandManager();	// Ctor.
	~CCommandManager();

	//! Execute command by name.
	void Execute( const char *command );

	//! Execute command by id.
	void ExecuteId( int commandId );

	void GetSortedCmdList( std::vector<CString> &cmds );
	CString	AutoComplete( const char* substr );
	CString	AutoCompletePrev( const char* substr );

	void	RegisterCommand( CCommand *cmd );
	void	UnregisterCommand( CCommand *cmd );

	// Attach UI info to the existing command.
	void AttachUI( const CString &sCommand,CCommand::UIInfo &ui );

	// Generates a new unique UI ID.
	int GetNewCommandId() const;

	void	AddCommandHandler( ICommandHandler *handler );
	void	RemoveCommandHandler( ICommandHandler *handler );

	//! Regster callback command.
	void RegisterCommand( const CString &sCommand,CommandCallback callback );
	//! Unregister command by name.
	void UnregisterCommand( const CString &sCommand );

	CCommand*	FindCommand( const CString &sCommand ) const;
	CCommand*	FindCommandById( int nCommandId ) const;

private:
	//typedef string_hash_multimap<CommandOp*> Commands;
	typedef std::map<CString,CCommandPtr> Commands;
	Commands m_commands;

	typedef std::map<int,CCommandPtr> MapCommandIds;
	MapCommandIds m_mapCommandIds;
	
	typedef	std::vector<ICommandHandler*>	CmdHandlers;
	CmdHandlers	m_cmdHandlers;
};

#endif // __commandmanager_h__
