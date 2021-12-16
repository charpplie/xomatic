////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   DebugCallStack.h
//  Version:     v1.00
//  Created:     3/12/2001 by Timur.
//  Compilers:   Visual C++ 6.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __DebugCallStack_h__
#define __DebugCallStack_h__

#if _MSC_VER > 1000
#pragma once
#endif

class ILogFile;

//! Limits the maximal number of functions in call stack.
const int MAX_DEBUG_STACK_ENTRIES = 30;

//!============================================================================
//!
//! DebugCallStack class, capture call stack information from symbol files.
//!
//!============================================================================
class DebugCallStack
{
public:
	// Returns single instance of DebugStack
	static DebugCallStack*	instance();

	void SetLog(ILogFile* log);

	//! Get current call stack information.
	void getCallStack( std::vector<string> &functions );

	int	 handleException( void *exception_pointer );

	struct ErrorHandlerScope
	{
		ErrorHandlerScope(ILogFile* log);
		~ErrorHandlerScope();
	};

private:
	DebugCallStack();
	virtual ~DebugCallStack();

	bool initSymbols();
	void doneSymbols();
	
	string	LookupFunctionName( void *adderss,bool fileInfo );
	int			updateCallStack( void *exception_pointer );
	void		FillStackTrace(int maxStackEntries = MAX_DEBUG_STACK_ENTRIES);

	static	int unhandledExceptionHandler( void *exception_pointer );

	//! Return name of module where exception happened.
	const char* getExceptionModule() { return m_excModule; }
	const char* getExceptionLine() { return m_excLine; }

	void installErrorHandler();
	void uninstallErrorHandler();

	void PrintException(EXCEPTION_POINTERS* pex);

	std::vector<string> m_functions;
	static DebugCallStack* m_instance;

	char m_excLine[256];
	char m_excModule[128];

	void* m_pVectoredExceptionHandlerHandle;

	bool	m_symbols;

	int m_nSkipNumFunctions;
	CONTEXT  m_context;

	ILogFile* m_pLog;
};

#endif // __DebugCallStack_h__