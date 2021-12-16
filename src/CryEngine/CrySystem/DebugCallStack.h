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

#include "IDebugCallStack.h"

#ifdef WIN32

//! Limits the maximal number of functions in call stack.
const int MAX_DEBUG_STACK_ENTRIES_FILE_DUMP = 12;

struct ISystem;

//!============================================================================
//!
//! DebugCallStack class, capture call stack information from symbol files.
//!
//!============================================================================
class DebugCallStack : public IDebugCallStack
{
public:
	ISystem* GetSystem() { return m_pSystem; };
	//! Dumps Current Call Stack to log.
	void LogMemCallstackFile(int memSize);
	void SetMemLogFile(bool open, const char *filename);

	virtual void CollectCurrentCallStack(int maxStackEntries = MAX_DEBUG_STACK_ENTRIES);
	virtual int CollectCallStackFrames(void **pCallstack,int maxStackEntries);

	void installErrorHandler( ISystem *pSystem );
	virtual int handleException( EXCEPTION_POINTERS *exception_pointer );

	void dumpCallStack( std::vector<string> &functions );

	//! Return name of module where exception happened.
	const char* getExceptionModule() { return m_excModule; }
	const char* getExceptionLine() { return m_excLine; }

	void SetUserDialogEnable( const bool bUserDialogEnable );

public:
	DebugCallStack();
	virtual ~DebugCallStack();

	bool initSymbols();
	void doneSymbols();
	
	string	LookupFunctionName( void *adderss,bool fileInfo );
	int			updateCallStack( void *exception_pointer );
	void		FillStackTrace(int maxStackEntries = MAX_DEBUG_STACK_ENTRIES, HANDLE hThread = GetCurrentThread());
	void    LogExceptionInfo( void *exception_pointer );

	static DWORD WINAPI ContextThreadProc( void* p );
	void	DoCollectCurrentCallStack();

	static INT_PTR CALLBACK ExceptionDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

protected:
	bool BackupCurrentLevel();
	void ResetFPU( EXCEPTION_POINTERS* pex );

	static const int s_iCallStackSize = 32768;

	char m_excLine[256];
	char m_excModule[128];

	char m_excDesc[256];
	char m_excCode[256];
	char m_excAddr[256];
	char m_excCallstack[s_iCallStackSize];

	void *prevExceptionHandler;

	bool	m_symbols;

	ISystem *m_pSystem;

	int m_nSkipNumFunctions;
	HANDLE   m_hThread;
	CONTEXT  m_context;
};

#endif //WIN32

#endif // __DebugCallStack_h__
