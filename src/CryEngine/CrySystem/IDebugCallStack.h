/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
Description: 
A multiplatform base class for handling errors and collecting call stacks

-------------------------------------------------------------------------
History:
- 12:10:2009	: Created by Alex McCarthy
*************************************************************************/

#ifndef __I_DEBUG_CALLSTACK_H__
#define __I_DEBUG_CALLSTACK_H__

#if !defined(PS3) && !defined(LINUX)

//! Limits the maximal number of functions in call stack.
const int MAX_DEBUG_STACK_ENTRIES = 80;

class IDebugCallStack
{
public:
	// Returns single instance of DebugStack
	static IDebugCallStack*	instance();

	virtual int handleException( EXCEPTION_POINTERS *exception_pointer ) = 0;
	virtual void CollectCurrentCallStack(int maxStackEntries = MAX_DEBUG_STACK_ENTRIES) = 0;

	// Collects the low level callstack frames.
	// Returns number of collected stack frames.
	virtual int CollectCallStackFrames(void **pCallstack,int maxStackEntries) { return 0; };

	//! Dumps Current Call Stack to log.
	virtual void LogCallstack();

	//! Get current call stack information.
	void getCallStack( std::vector<string> &functions ) { functions = m_functions; }

protected:
	virtual ~IDebugCallStack() {}

	static const char* TranslateExceptionCode( DWORD dwExcept );
	static void PutVersion( char *str );

	static void WriteLineToLog( const char* format,... );

	std::vector<string> m_functions;
};

#endif //PS3

#endif // __I_DEBUG_CALLSTACK_H__
