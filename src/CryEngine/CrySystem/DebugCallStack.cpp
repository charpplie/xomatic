////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   debugcallstack.cpp
//  Version:     v1.00
//  Created:     1/10/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "DebugCallStack.h"

#ifdef WIN32

#include <IConsole.h>
#include <IScriptSystem.h>
#include "JiraClient.h"
#include "System.h"
#include "Log.h"

#include "resource.h"

#pragma comment(lib, "version.lib")

//! Needs one external of DLL handle.
extern HMODULE gDLLHandle;

static FILE*		gMemAllocFile;

#include <dbghelp.h>
#pragma comment( lib, "dbghelp" )
#pragma warning(disable: 4244)

#include "Psapi.h"
typedef BOOL (WINAPI *GetProcessMemoryInfoProc)( HANDLE,PPROCESS_MEMORY_COUNTERS,DWORD );

#define MAX_PATH_LENGTH 1024
#define MAX_SYMBOL_LENGTH 512

static HWND hwndException = 0;
static bool g_bUserDialog=true;			// true=on crash show dialog box, false=supress user interaction 

static int	PrintException( EXCEPTION_POINTERS* pex );

static bool IsFloatingPointException( EXCEPTION_POINTERS* pex );

extern LONG WINAPI CryEngineExceptionFilterWER( struct _EXCEPTION_POINTERS * pExceptionPointers );
extern LONG WINAPI CryEngineExceptionFilterMiniDump( struct _EXCEPTION_POINTERS * pExceptionPointers, const char* szDumpPath, MINIDUMP_TYPE mdumpValue );

//=============================================================================
LONG __stdcall CryUnhandledExceptionHandler( EXCEPTION_POINTERS *pex )
{
	return DebugCallStack::instance()->handleException( pex );
}

//=============================================================================
// Class Statics
//=============================================================================

// Return single instance of class.
IDebugCallStack* IDebugCallStack::instance()
{
	static DebugCallStack sInstance;
	return &sInstance;
}

//------------------------------------------------------------------------------------------------------------------------
// Sets up the symbols for functions in the debug file.
//------------------------------------------------------------------------------------------------------------------------
DebugCallStack::DebugCallStack()
{
	prevExceptionHandler = 0;
	m_pSystem = 0;
	m_symbols = false;
	m_hThread = 0;
	m_nSkipNumFunctions = 0;
}

DebugCallStack::~DebugCallStack()
{
	if(gMemAllocFile)
		fclose(gMemAllocFile);
}

/*
BOOL CALLBACK func_PSYM_ENUMSOURCFILES_CALLBACK( PSOURCEFILE pSourceFile, PVOID UserContext )
{
	CryLogAlways( pSourceFile->FileName );
	return TRUE;
}

BOOL CALLBACK func_PSYM_ENUMMODULES_CALLBACK64(
																				PSTR ModuleName,
																				DWORD64 BaseOfDll,
																				PVOID UserContext
																				)
{
	CryLogAlways( "<SymModule> %s: %x",ModuleName,(uint32)BaseOfDll );
	return TRUE;
}

BOOL CALLBACK func_PSYM_ENUMERATESYMBOLS_CALLBACK(
	PSYMBOL_INFO  pSymInfo,
	ULONG         SymbolSize,
	PVOID         UserContext
	)
{
	CryLogAlways( "<Symbol> %08X Size=%08X  :%s",(uint32)pSymInfo->Address,(uint32)pSymInfo->Size,pSymInfo->Name );
	return TRUE;
}
*/

bool DebugCallStack::initSymbols()
{
#ifndef WIN98
	if (m_symbols) return true;
	
	char fullpath[MAX_PATH_LENGTH+1];
	char pathname[MAX_PATH_LENGTH+1];
	char fname[MAX_PATH_LENGTH+1];
	char directory[MAX_PATH_LENGTH+1];
	char drive[10];

	{
		// Print dbghelp version.
		HMODULE dbgHelpDll = GetModuleHandle( "dbghelp.dll" );

		char ver[1024*8];
		GetModuleFileName( dbgHelpDll, fullpath, _MAX_PATH );
		int fv[4];

		DWORD dwHandle;
		int verSize = GetFileVersionInfoSize( fullpath,&dwHandle );
		if (verSize > 0)
		{
			unsigned int len;
			GetFileVersionInfo( fullpath,dwHandle,1024*8,ver );
			VS_FIXEDFILEINFO *vinfo;
			VerQueryValue( ver,"\\",(void**)&vinfo,&len );

			fv[0] = vinfo->dwFileVersionLS & 0xFFFF;
			fv[1] = vinfo->dwFileVersionLS >> 16;
			fv[2] = vinfo->dwFileVersionMS & 0xFFFF;
			fv[3] = vinfo->dwFileVersionMS >> 16;

//			CryLogAlways( "dbghelp.dll version %d.%d.%d.%d",fv[3],fv[2],fv[1],fv[0] );
		}
	}

//	SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_UNDNAME|SYMOPT_LOAD_LINES|SYMOPT_OMAP_FIND_NEAREST|SYMOPT_INCLUDE_32BIT_MODULES);
	//DWORD res1 = SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_UNDNAME|SYMOPT_LOAD_LINES|SYMOPT_OMAP_FIND_NEAREST);

	SymSetOptions(SYMOPT_UNDNAME|SYMOPT_DEFERRED_LOADS|SYMOPT_INCLUDE_32BIT_MODULES|SYMOPT_LOAD_ANYTHING|SYMOPT_LOAD_LINES);

		
	HANDLE hProcess = GetCurrentProcess();
	
	// Get module file name.
	GetModuleFileName( NULL, fullpath, MAX_PATH_LENGTH );

	// Convert it into search path for symbols.
	strcpy( pathname,fullpath );
	_splitpath( pathname, drive, directory, fname, NULL );
	sprintf( pathname, "%s%s", drive,directory );
	
	// Append the current directory to build a search path forSymInit
	strcat( pathname, ";.;" );

	int result = 0;

	m_symbols = false;

	result = SymInitialize( hProcess,pathname,TRUE );
	if (result) {
		//HMODULE hMod = GetModuleHandle( "imagehlp" );
		//SymGetLineFromAddrPtr = (SymGetLineFromAddrFunction)GetProcAddress( hMod,"SymGetLineFromAddr" );

		char pdb[MAX_PATH_LENGTH+1];
		char res_pdb[MAX_PATH_LENGTH+1];
		sprintf( pdb, "%s.pdb",fname );
		sprintf( pathname, "%s%s", drive,directory );
		if (SearchTreeForFile( pathname,pdb,res_pdb )) {
			m_symbols = true;
		}
	} else {
		result = SymInitialize( hProcess,pathname,FALSE );
		if (!result)
		{
			CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"SymInitialize failed" );
		}
	}
#else
	return false;
#endif

	return result != 0;
}

void	DebugCallStack::doneSymbols()
{
#ifndef WIN98
	if (m_symbols) {
		SymCleanup( GetCurrentProcess() );
	}
	m_symbols = false;
#endif
}

//////////////////////////////////////////////////////////////////////////
DWORD WINAPI DebugCallStack::ContextThreadProc( void *p )
{
	((DebugCallStack*)p)->DoCollectCurrentCallStack();
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void DebugCallStack::DoCollectCurrentCallStack()
{
	SuspendThread(m_hThread);
	BOOL bRes = GetThreadContext(m_hThread, &m_context);

	//m_context.Rip = GetProgramCounter();

	//CryLogAlways( "GetThreadContext In Thread" );
	//CryLogAlways( "eip=%p, esp=%p, ebp=%p",m_context.Eip,m_context.Esp,m_context.Ebp );
	//CryLogAlways( "Rip=%p",m_context.Rip );


	m_nSkipNumFunctions = 3;
	FillStackTrace();

	ResumeThread(m_hThread);
}

extern "C" void * _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)
#pragma auto_inline(off)
DWORD_PTR GetProgramCounter()
{
	return (DWORD_PTR)_ReturnAddress();
}
#pragma auto_inline(on)

//////////////////////////////////////////////////////////////////////////
void DebugCallStack::CollectCurrentCallStack(int maxStackEntries)
{
	if (!initSymbols())
		return;

	m_functions.clear();

	memset(&m_context,0,sizeof(m_context));
	m_context.ContextFlags = CONTEXT_FULL;

	GetThreadContext( GetCurrentThread(),&m_context );

	m_nSkipNumFunctions = 2;
	
	FillStackTrace(maxStackEntries);
}

//------------------------------------------------------------------------------------------------------------------------
static int callCount = 0;
int DebugCallStack::updateCallStack( void *exception_pointer )
{
	if (callCount > 0)
	{
		if (prevExceptionHandler)
		{
			// uninstall our exception handler.
			SetUnhandledExceptionFilter( (LPTOP_LEVEL_EXCEPTION_FILTER)prevExceptionHandler );
		}
		// Immidiate termination of process.
		abort();
	}
	callCount++;
	EXCEPTION_POINTERS *pex = (EXCEPTION_POINTERS*)exception_pointer;

	HANDLE process = GetCurrentProcess();

	//! Find source line at exception address.
	//m_excLine = lookupFunctionName( (void*)pex->ExceptionRecord->ExceptionAddress,true );

	//! Find Name of .DLL from Exception address.
	strcpy( m_excModule,"<Unknown>" );

	if (m_symbols)
	{
		DWORD64 dwAddr = SymGetModuleBase64( process,(DWORD64)pex->ExceptionRecord->ExceptionAddress );
		if (dwAddr) 
		{
			char szBuff[MAX_PATH_LENGTH];
			if (GetModuleFileName( (HMODULE)dwAddr,szBuff,MAX_PATH_LENGTH )) {
				strcpy( m_excModule,szBuff );
				string path,fname,ext;
				
				char fdir[_MAX_PATH];
				char fdrive[_MAX_PATH];
				char file[_MAX_PATH];
				char fext[_MAX_PATH];
				_splitpath( m_excModule,fdrive,fdir,file,fext );
				_makepath( fdir,NULL,NULL,file,fext );

				strcpy(m_excModule,fdir);
			}
		}
	}

	// Fill stack trace info.
	m_context = *pex->ContextRecord;
	m_nSkipNumFunctions = 0;
	FillStackTrace();

	return EXCEPTION_CONTINUE_EXECUTION;
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void DebugCallStack::FillStackTrace(int maxStackEntries, HANDLE hThread)
{
	HANDLE hProcess = GetCurrentProcess();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	int count;
	STACKFRAME64 stack_frame;
	BOOL b_ret=TRUE; //Setup stack frame 
	memset(&stack_frame, 0, sizeof(stack_frame));
	stack_frame.AddrPC.Mode = AddrModeFlat;
	stack_frame.AddrFrame.Mode = AddrModeFlat;
	stack_frame.AddrStack.Mode = AddrModeFlat;
	stack_frame.AddrReturn.Mode = AddrModeFlat;
	stack_frame.AddrBStore.Mode = AddrModeFlat;

	DWORD MachineType = IMAGE_FILE_MACHINE_I386;

#if defined(_M_IX86)
	MachineType                   = IMAGE_FILE_MACHINE_I386;
	stack_frame.AddrPC.Offset     = m_context.Eip;
	stack_frame.AddrStack.Offset  = m_context.Esp;
	stack_frame.AddrFrame.Offset  = m_context.Ebp;
#elif defined(_M_X64)
	MachineType                   = IMAGE_FILE_MACHINE_AMD64;
	stack_frame.AddrPC.Offset     = m_context.Rip;
	stack_frame.AddrStack.Offset  = m_context.Rsp;
	stack_frame.AddrFrame.Offset  = m_context.Rdi;
#endif

	m_functions.clear();

	//CryLogAlways( "Start StackWalk" );
	//CryLogAlways( "eip=%p, esp=%p, ebp=%p",m_context.Eip,m_context.Esp,m_context.Ebp );

	//While there are still functions on the stack.. 
	for(count=0; count < maxStackEntries && b_ret==TRUE; count++)
	{
		b_ret = StackWalk64( MachineType,	hProcess, hThread, &stack_frame, &m_context,NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL);

		if (count < m_nSkipNumFunctions)
			continue;

		if (m_symbols)
		{
			string funcName = LookupFunctionName( (void*)stack_frame.AddrPC.Offset,true );
			if (!funcName.empty())
			{
				m_functions.push_back( funcName );
			}
			else
			{
				DWORD64 p = (DWORD64)stack_frame.AddrPC.Offset;
				char str[80];
				sprintf( str,"function=0x%p",p );
				m_functions.push_back( str );
			}
		} else {
			DWORD64 p = (DWORD64)stack_frame.AddrPC.Offset;
			char str[80];
			sprintf( str,"function=0x%p",p );
			m_functions.push_back( str );
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------
string DebugCallStack::LookupFunctionName( void *pointer,bool fileInfo )
{
	string symName = "";

#ifndef WIN98
	HANDLE process = GetCurrentProcess();
	char symbolBuf[sizeof(SYMBOL_INFO)+MAX_SYMBOL_LENGTH+1];
	memset( symbolBuf, 0, sizeof(symbolBuf));
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuf;
 
	DWORD displacement = 0;
	DWORD64 displacement64 = 0;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYMBOL_LENGTH;
	if (SymFromAddr( process,(DWORD64)pointer,&displacement64,pSymbol ))
	{
		symName = string(pSymbol->Name) + "()";
	}
		
	if (fileInfo)
	{
		// Lookup Line in source file.
		IMAGEHLP_LINE64 lineImg;
		memset( &lineImg,0,sizeof(lineImg) );
		lineImg.SizeOfStruct = sizeof(lineImg);

		if (SymGetLineFromAddr64( process,(DWORD_PTR)pointer, &displacement, &lineImg ))
		{
			char lineNum[1024];
			itoa( lineImg.LineNumber,lineNum,10 );
			string path;

			char file[1024];
			char fname[1024];
			char fext[1024];
			_splitpath( lineImg.FileName,NULL,NULL,fname,fext );
			_makepath( file,NULL,NULL,fname,fext );
			string fileName = file;
			/*
			string finfo = string("[" ) + fileName + ", line:" + lineNum + "]";
			//symName += string(" --- [" ) + fileName + ", line:" + lineNum + "]";
			//char finfo[1024];
			//sprintf( finfo,"[%s,line:%d]",fileName.
			char temp[4096];
			sprintf( temp,"%30s --- %s",finfo.c_str(),symName.c_str() );
			symName = temp;
			*/

			symName += string("  [" ) + fileName + ":" + lineNum + "]";
		}
	}
#endif

	return symName;
}

void DebugCallStack::installErrorHandler( ISystem *pSystem )
{
	m_pSystem = pSystem;
	prevExceptionHandler = (void*)SetUnhandledExceptionFilter( CryUnhandledExceptionHandler );
}

//////////////////////////////////////////////////////////////////////////
void DebugCallStack::SetUserDialogEnable( const bool bUserDialogEnable ) 
{ 
	g_bUserDialog=bUserDialogEnable; 
}


DWORD g_idDebugThreads[10];
const char *g_nameDebugThreads[10];
int g_nDebugThreads = 0;
volatile int g_lockThreadDumpList = 0;

void MarkThisThreadForDebugging(const char *name)
{
	WriteLock lock(g_lockThreadDumpList);
	DWORD id = GetCurrentThreadId();
	if (g_nDebugThreads==sizeof(g_idDebugThreads)/sizeof(g_idDebugThreads[0]))
		return;
	for(int i=0;i<g_nDebugThreads;i++) if (g_idDebugThreads[i]==id)
		return;
	g_nameDebugThreads[g_nDebugThreads] = name;
	g_idDebugThreads[g_nDebugThreads++] = id;
	((CSystem*)gEnv->pSystem)->EnableFloatExceptions( g_cvars.sys_float_exceptions );
}

void UnmarkThisThreadFromDebugging()
{
	WriteLock lock(g_lockThreadDumpList);
	DWORD id = GetCurrentThreadId();
	for(int i=g_nDebugThreads-1;i>=0;i--) if (g_idDebugThreads[i]==id) {
		memmove(g_idDebugThreads+i, g_idDebugThreads+i+1, (g_nDebugThreads-1-i)*sizeof(g_idDebugThreads[0]));
		memmove(g_nameDebugThreads+i, g_nameDebugThreads+i+1, (g_nDebugThreads-1-i)*sizeof(g_nameDebugThreads[0]));
		--g_nDebugThreads;
	}
}

extern int prev_sys_float_exceptions;
void UpdateFPExceptionsMaskForThreads()
{
	int mask = -iszero(g_cvars.sys_float_exceptions);
	CONTEXT ctx;
	for(int i=0;i<g_nDebugThreads;i++) if (g_idDebugThreads[i]!=GetCurrentThreadId())
	{
		HANDLE hThread = OpenThread(THREAD_ALL_ACCESS,TRUE,g_idDebugThreads[i]);
		ctx.ContextFlags = CONTEXT_ALL;
		SuspendThread(hThread);
		GetThreadContext(hThread, &ctx);
#ifndef WIN64
		(ctx.FloatSave.ControlWord |= 7) &= ~5 | mask;
		(*(WORD*)(ctx.ExtendedRegisters+24) |= 0x280) &= ~0x280	| mask;
#else
		(ctx.FltSave.ControlWord |= 7) &= ~5 | mask;
		(ctx.FltSave.MxCsr |= 0x280) &= ~0x280	| mask;
#endif
		SetThreadContext(hThread, &ctx);
		ResumeThread(hThread);
	}
}

//////////////////////////////////////////////////////////////////////////
int	DebugCallStack::handleException( EXCEPTION_POINTERS *exception_pointer )
{
	((CLog*)(gEnv->pLog))->AllowDirectLoggingFromAnyThread(true);

	ResetFPU( exception_pointer );

	prev_sys_float_exceptions = 0;
	const int cached_sys_float_exceptions = g_cvars.sys_float_exceptions;
	g_cvars.sys_float_exceptions = cached_sys_float_exceptions;
	((CSystem*)gEnv->pSystem)->EnableFloatExceptions( 0 );

	if (g_cvars.sys_WER)
		return CryEngineExceptionFilterWER( (_EXCEPTION_POINTERS*)exception_pointer );

	EXCEPTION_POINTERS *pex = (EXCEPTION_POINTERS*)exception_pointer;
	int ret = IDB_EXIT;
	static bool firstTime = true;

	if (g_cvars.sys_dump_aux_threads | g_cvars.sys_keyboard_break)
		for(int i=0;i<g_nDebugThreads;i++) if (g_idDebugThreads[i]!=GetCurrentThreadId())
			SuspendThread(OpenThread(THREAD_ALL_ACCESS,TRUE,g_idDebugThreads[i]));

	// uninstall our exception handler.
	SetUnhandledExceptionFilter( (LPTOP_LEVEL_EXCEPTION_FILTER)prevExceptionHandler );

	if (!firstTime)
	{
		WriteLineToLog( "Critical Exception! Called Multiple Times!" );
		// Exception called more then once.
		return EXCEPTION_EXECUTE_HANDLER;
	}

	// Print exception info:
	{
		char excCode[80];
		char excAddr[80];
		WriteLineToLog( "<CRITICAL EXCEPTION>" );
		sprintf( excAddr,"0x%04X:0x%p",pex->ContextRecord->SegCs,pex->ExceptionRecord->ExceptionAddress );
		sprintf( excCode,"0x%08X",pex->ExceptionRecord->ExceptionCode );
		WriteLineToLog( "Exception: %s, at Address: %s",excCode,excAddr );

    if(CSystem * pSystem = (CSystem*)GetSystem())
    {
      if(const char * pLoadingProfilerCallstack = pSystem->GetLoadingProfilerCallstack())
        if(pLoadingProfilerCallstack[0])
          WriteLineToLog( "<CrySystem> LoadingProfilerCallstack: %s", pLoadingProfilerCallstack );
    }

		{
			HMODULE hPSAPI = LoadLibrary("psapi.dll");
			if (hPSAPI)
			{
				GetProcessMemoryInfoProc pGetProcessMemoryInfo = (GetProcessMemoryInfoProc)GetProcAddress(hPSAPI, "GetProcessMemoryInfo");
				if (pGetProcessMemoryInfo)
				{
					PROCESS_MEMORY_COUNTERS pc;
					HANDLE hProcess = GetCurrentProcess();
					pc.cb = sizeof(pc);
					pGetProcessMemoryInfo( hProcess, &pc, sizeof(pc) );
					uint32 nMemUsage = (uint32)pc.PagefileUsage/(1024*1024);
					WriteLineToLog( "Virtual memory usage: %dMb",nMemUsage );
				}
			}
		}
	}

	firstTime = false;

	assert(!hwndException);

	// If in full screen minimize render window
	{
		ICVar *pFullscreen = (gEnv && gEnv->pConsole) ? gEnv->pConsole->GetCVar("r_Fullscreen") : 0;
		if (pFullscreen && pFullscreen->GetIVal() != 0 && gEnv->pRenderer && gEnv->pRenderer->GetHWND())
		{
			::ShowWindow((HWND)gEnv->pRenderer->GetHWND(),SW_MINIMIZE);
		}
	}

	//hwndException = CreateDialog( gDLLHandle,MAKEINTRESOURCE(IDD_EXCEPTION),NULL,NULL );
	
	if (initSymbols())
	{
		// Rise exception to call updateCallStack method.
		updateCallStack( exception_pointer );

		LogExceptionInfo( exception_pointer );

		if(IsFloatingPointException( exception_pointer ))
		{
			//! Print exception dialog.
			ret = PrintException( pex );
		}

		doneSymbols();
		//exit(0);
	}

	if (ret != IDB_IGNORE)
		CryEngineExceptionFilterWER( (_EXCEPTION_POINTERS*)exception_pointer );
	
	if (pex->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
	{
		// This is non continuable exception. abort application now.
		exit(1);
	}

	//typedef long (__stdcall *ExceptionFunc)(EXCEPTION_POINTERS*);
	//ExceptionFunc prevFunc = (ExceptionFunc)prevExceptionHandler;
	//return prevFunc( (EXCEPTION_POINTERS*)exception_pointer );
	if (ret == IDB_EXIT)
	{
		// Immidiate exit.
		exit(1);
	} else if (ret == IDB_IGNORE) {
#ifndef WIN64
		pex->ContextRecord->FloatSave.StatusWord &= ~31;
		pex->ContextRecord->FloatSave.ControlWord |= 7;
		(*(WORD*)(pex->ContextRecord->ExtendedRegisters+24) &= 31) |= 0x1F80;
#else
		pex->ContextRecord->FltSave.StatusWord &= ~31;
		pex->ContextRecord->FltSave.ControlWord |= 7;
		(pex->ContextRecord->FltSave.MxCsr &= 31) |= 0x1F80;
#endif
		firstTime = true;
		callCount = 0;
		prevExceptionHandler = (void*)SetUnhandledExceptionFilter( CryUnhandledExceptionHandler );
		g_cvars.sys_float_exceptions = cached_sys_float_exceptions;
		((CSystem*)gEnv->pSystem)->EnableFloatExceptions( g_cvars.sys_float_exceptions );
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	// Continue;
	return EXCEPTION_EXECUTE_HANDLER;
}


void DebugCallStack::dumpCallStack( std::vector<string> &funcs )
{
	WriteLineToLog( "=============================================================================" );
	int len = (int)funcs.size();
	for (int i = 0; i < len; i++) {
		const char* str = funcs[i].c_str();
		WriteLineToLog( "%2d) %s",len-i,str );
	}
	WriteLineToLog( "=============================================================================" );
}

void DebugCallStack::LogMemCallstackFile(int memSize)
{
	if(!gMemAllocFile)
		return;

	CollectCurrentCallStack(MAX_DEBUG_STACK_ENTRIES_FILE_DUMP);		// is updating m_functions

	char buffer[16];
	itoa(memSize, buffer, 10);
	CryFixedStringT<64> temp("*** Memory allocation for ");
	temp.append(buffer);
	temp.append(" bytes ");
	int frame = gEnv->pRenderer->GetFrameID(false);
	itoa(frame,buffer, 10);
	temp.append("in frame ");
	temp.append(buffer);
	temp.append("****\n");
	fwrite(temp.c_str(), temp.size(), 1, gMemAllocFile);
	int len = (int)m_functions.size();
	for (int i = 0; i < len; i++) {
		const char* str = m_functions[i].c_str();
		itoa(len-i, buffer, 10);
		temp = buffer;
		temp.append(" ");
		temp.append(str);
		temp.append("\n");
		fwrite(temp.c_str(), temp.size(), 1, gMemAllocFile);
	}
	temp = "=============================================================================\n";
	fwrite(temp.c_str(), temp.size(), 1, gMemAllocFile);
}

//////////////////////////////////////////////////////////////////////////
void DebugCallStack::SetMemLogFile(bool open, const char *filename)
{
	if(open)
	{
		if(!gMemAllocFile)
			gMemAllocFile = fopen("memallocfile.txt", "wb");
		assert(gMemAllocFile);
	}
	else if(gMemAllocFile)
	{
		fclose(gMemAllocFile);
		gMemAllocFile = NULL;
	}
}

void ReportJiraBug()
{
	// (Kevin) - Acknowledging the cvar or user settings to suppress the dialog box messages for when crashes occur.
	//	In the future, a separate cvar should control if Jira client can submit a crash bug silently. (24/08/09)
	if (g_cvars.sys_no_crash_dialog != 0 || !g_bUserDialog)
	{
		return;
	}

	if(!CJiraClient::ReportBug())
	{
		MessageBox( NULL,"Error running jira crash handler: bug submission failed.","Bug submission failed",MB_OK|MB_ICONWARNING );
	}
}

//////////////////////////////////////////////////////////////////////////
void DebugCallStack::LogExceptionInfo( void *exception_pointer )
{
	EXCEPTION_POINTERS *pex = (EXCEPTION_POINTERS*)exception_pointer;
	static char errorString[s_iCallStackSize] = "";

	// Time and Version.
	char versionbuf[1024];
	strcpy( versionbuf,"" );
	PutVersion( versionbuf );
	strcat( errorString,versionbuf );
	strcat( errorString,"\n" );

	//! Get call stack functions.
	DebugCallStack *cs = static_cast<DebugCallStack*>(DebugCallStack::instance());
	std::vector<string> funcs;
	cs->getCallStack( funcs );

	// Init dialog.
	int iswrite = 0;
	DWORD64 accessAddr = 0;
	char excCode[80];
	char excAddr[80];
	sprintf( excAddr,"0x%04X:0x%p",pex->ContextRecord->SegCs,pex->ExceptionRecord->ExceptionAddress );
	sprintf( excCode,"0x%08X",pex->ExceptionRecord->ExceptionCode );
	string moduleName = cs->getExceptionModule();
	const char *excModule = moduleName.c_str();

	char desc[1024];
	char excDesc[1024];
	const char *excName = TranslateExceptionCode(pex->ExceptionRecord->ExceptionCode);

	strcpy( desc,"" );

	if (pex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
		if (pex->ExceptionRecord->NumberParameters > 1) {
			int iswrite = pex->ExceptionRecord->ExceptionInformation[0];
			accessAddr = pex->ExceptionRecord->ExceptionInformation[1];
			if (iswrite) {
				sprintf( desc,"Attempt to write data to address 0x%08p\r\nThe memory could not be \"written\"",accessAddr );
			} else {
				sprintf( desc,"Attempt to read from address 0x%08p\r\nThe memory could not be \"read\"",accessAddr );
			}
		}
	}
	sprintf( excDesc,"%s\r\n%s",excName,desc );

	WriteLineToLog( "Exception Code: %s",excCode );
	WriteLineToLog( "Exception Addr: %s",excAddr );
	WriteLineToLog( "Exception Module: %s",excModule );
	WriteLineToLog( "Exception Name  : %s",excName );
	WriteLineToLog( "Exception Description: %s",desc );
	cs->dumpCallStack( funcs );

	strcpy_s(m_excDesc,excDesc);
	strcpy_s(m_excAddr,excAddr);
	strcpy_s(m_excModule,excModule);
	strcpy_s(m_excCode,excCode);

	char errs[32768];
	sprintf( errs,"Exception Code: %s\nException Addr: %s\nException Module: %s\nException Description: %s, %s\n",
		excCode,excAddr,excModule,excName,desc );

	strcat( errs,"\nCall Stack Trace:\n" );

	// Fill call stack.
	char str[s_iCallStackSize];
	strcpy( str,"" );
	for (unsigned int i = 0; i < funcs.size(); i++) {
		char temp[s_iCallStackSize];
		sprintf( temp,"%2d) %s",funcs.size()-i,(const char*)funcs[i].c_str() );
		strcat( str,temp );
		strcat( str,"\r\n" );
		strcat( errs,temp );
		strcat( errs,"\n" );
	}

	strcpy_s(m_excCallstack,str);

	strcat( errorString,errs );

	//////////////////////////////////////////////////////////////////////////
	FILE *f = fopen( "error.log","wt" );
	if (f) {
		fwrite( errorString,strlen(errorString),1,f );
		if (g_cvars.sys_dump_aux_threads | g_cvars.sys_keyboard_break)
		{
			funcs.clear();
			for(int i=0;i<g_nDebugThreads;i++) if (g_idDebugThreads[i]!=GetCurrentThreadId())
			{
				fprintf(f, "\n\nSuspended thread (%s):\n", g_nameDebugThreads[i]);
				HANDLE hThread = OpenThread(THREAD_ALL_ACCESS,TRUE,g_idDebugThreads[i]);
				GetThreadContext(hThread, &cs->m_context);
				cs->m_nSkipNumFunctions = 0;
				cs->FillStackTrace(10, hThread);
				cs->getCallStack( funcs );
				for (uint32 i=0; i<funcs.size(); i++)
					fprintf(f, "%2d) %s\n",funcs.size()-i,funcs[i].c_str() );
				ResumeThread(hThread);
			}
		}
		fflush(f);
		fclose(f);
	}

	// This combination of flags makes a roughly 30MB minidump that
	// compresses down to roughly 3MB.
	MINIDUMP_TYPE mdumpValue = (MINIDUMP_TYPE)(
		MiniDumpNormal |	// normal minidump data. Callstack, thread info, etc
		MiniDumpWithIndirectlyReferencedMemory | // try to find pointers on the stack and dump memory near where they're pointing
		MiniDumpWithDataSegs // dump global variables (like gEnv)
		);
	CryEngineExceptionFilterMiniDump( (_EXCEPTION_POINTERS*)pex, "error.dmp", mdumpValue );

	static int g_numScreenshots = 0;
	if (gEnv->pRenderer && !g_numScreenshots++)
	{
		gEnv->pRenderer->ScreenShot("error.bmp");
	}

	ReportJiraBug();

	if (g_cvars.sys_no_crash_dialog == 0 && g_bUserDialog && gEnv->bEditor)
	{
		int res = MessageBox( NULL,"Warning!\r\nEditor has crashed and is now unstable.\r\nSaving this document may cause level corruption or further crashes.\r\nProceed with Save?","Save Level",MB_YESNO|MB_ICONEXCLAMATION );
		if (res == IDYES)
		{
			// Make one additional backup.
			if (BackupCurrentLevel())
			{
				MessageBox( NULL,"Level has been successfully saved!\r\nPress Ok to terminate Editor.","Save",MB_OK );
			}
			else
			{
				MessageBox( NULL,"Error saving level.\r\nPress Ok to terminate Editor.","Save",MB_OK|MB_ICONWARNING );
			}
		}
	}

	if (g_cvars.sys_no_crash_dialog != 0 || !g_bUserDialog)
	{
		exit(1);
	}
}

		
INT_PTR CALLBACK DebugCallStack::ExceptionDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static EXCEPTION_POINTERS *pex;

	static char errorString[32768] = "";

	switch (message) 
	{
	case WM_INITDIALOG:
		{
			pex = (EXCEPTION_POINTERS*)lParam;
			HWND h;
			
			if (pex->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) {
				// Disable continue button for non continuable exceptions.
				//h = GetDlgItem( hwndDlg,IDB_CONTINUE );
				//if (h) EnableWindow( h,FALSE );
			}

			DebugCallStack *pDCS = static_cast<DebugCallStack*>(DebugCallStack::instance());

			h = GetDlgItem( hwndDlg,IDC_EXCEPTION_DESC );
			if (h) SendMessage( h,EM_REPLACESEL,FALSE, (LONG_PTR)pDCS->m_excDesc );
			
			h = GetDlgItem( hwndDlg,IDC_EXCEPTION_CODE );
			if (h) SendMessage( h,EM_REPLACESEL,FALSE, (LONG_PTR)pDCS->m_excCode );
			
			h = GetDlgItem( hwndDlg,IDC_EXCEPTION_MODULE );
			if (h) SendMessage( h,EM_REPLACESEL,FALSE, (LONG_PTR)pDCS->m_excModule );

			h = GetDlgItem( hwndDlg,IDC_EXCEPTION_ADDRESS );
			if (h) SendMessage( h,EM_REPLACESEL,FALSE, (LONG_PTR)pDCS->m_excAddr );

			// Fill call stack.
			HWND callStack = GetDlgItem( hwndDlg,IDC_CALLSTACK );
			if (callStack)
			{
				SendMessage( callStack,WM_SETTEXT,FALSE, (LPARAM)pDCS->m_excCallstack );
			}

			if (hwndException)
			{
				DestroyWindow( hwndException );
				hwndException=0;
			}

			if(IsFloatingPointException(pex))
			{
					EnableWindow(GetDlgItem(hwndDlg,IDB_IGNORE), TRUE);
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
			case IDB_EXIT:
			case IDB_IGNORE:
				// Fall through.

				EndDialog(hwndDlg, wParam);
				return TRUE;
		}
	}
	return FALSE; 
}

bool DebugCallStack::BackupCurrentLevel()
{
	CSystem* pSystem = static_cast<CSystem*>(m_pSystem);
	if (pSystem && pSystem->GetUserCallback())
	{
		return pSystem->GetUserCallback()->OnSaveDocument();
	}

	return false;
}

void DebugCallStack::ResetFPU( EXCEPTION_POINTERS* pex )
{
	if(IsFloatingPointException(pex))
	{
		// How to reset FPU: http://www.experts-exchange.com/Programming/System/Windows__Programming/Q_10310953.html
		_clearfp();
#ifndef WIN64
		pex->ContextRecord->FloatSave.ControlWord |= 0x2F;
		pex->ContextRecord->FloatSave.StatusWord &= ~0x8080;
#endif
	}
}

//////////////////////////////////////////////////////////////////////////
int __cdecl WalkStackFrames( CONTEXT &context,void **pCallstack,int maxStackEntries )
{
	int count;
	BOOL b_ret=TRUE; //Setup stack frame 

	HANDLE hThread = GetCurrentThread();
	HANDLE hProcess = GetCurrentProcess();

	STACKFRAME64 stack_frame;

	memset(&stack_frame, 0, sizeof(stack_frame));
	stack_frame.AddrPC.Mode = AddrModeFlat;
	stack_frame.AddrFrame.Mode = AddrModeFlat;
	stack_frame.AddrStack.Mode = AddrModeFlat;
	stack_frame.AddrReturn.Mode = AddrModeFlat;
	stack_frame.AddrBStore.Mode = AddrModeFlat;

	DWORD MachineType = IMAGE_FILE_MACHINE_I386;

#if defined(_M_IX86)
	MachineType                   = IMAGE_FILE_MACHINE_I386;
	stack_frame.AddrPC.Offset     = context.Eip;
	stack_frame.AddrStack.Offset  = context.Esp;
	stack_frame.AddrFrame.Offset  = context.Ebp;
#elif defined(_M_X64)
	MachineType                   = IMAGE_FILE_MACHINE_AMD64;
	stack_frame.AddrPC.Offset     = context.Rip;
	stack_frame.AddrStack.Offset  = context.Rsp;
	stack_frame.AddrFrame.Offset  = context.Rdi;
#endif

	//While there are still functions on the stack.. 
	for(count=0; count < maxStackEntries && b_ret==TRUE; count++)
	{
		b_ret = StackWalk64( MachineType,	hProcess, hThread, &stack_frame, &context,NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL);
		pCallstack[count] = (void*)(stack_frame.AddrPC.Offset);
	}
	return count;	
}

//////////////////////////////////////////////////////////////////////////
int DebugCallStack::CollectCallStackFrames( void **pCallstack,int maxStackEntries )
{
	if (!m_symbols)
	{
		if (!initSymbols())
			return 0;
	}
	CONTEXT context;
	memset(&context,0,sizeof(context));
	context.ContextFlags = CONTEXT_FULL;

	do {
		memset(&context,0,sizeof(context));
		context.ContextFlags = CONTEXT_FULL;
		//GetThreadContext( GetCurrentThread(),&context );
		RtlCaptureContext(&context);
	} while(0);
	
	HANDLE hProcess = GetCurrentProcess();

	int count = WalkStackFrames( context,pCallstack,maxStackEntries );
	return count;
}

static bool IsFloatingPointException( EXCEPTION_POINTERS* pex )
{
	DWORD exceptionCode = pex->ExceptionRecord->ExceptionCode;
	switch (exceptionCode)
	{
		case EXCEPTION_FLT_DENORMAL_OPERAND:	case EXCEPTION_FLT_DIVIDE_BY_ZERO: 
		case EXCEPTION_FLT_INEXACT_RESULT: case EXCEPTION_FLT_INVALID_OPERATION: 
		case EXCEPTION_FLT_OVERFLOW: case EXCEPTION_FLT_UNDERFLOW:
		case STATUS_FLOAT_MULTIPLE_FAULTS:	case STATUS_FLOAT_MULTIPLE_TRAPS:	
			return true;

		default:
			return false;
	}
}

static int	PrintException( EXCEPTION_POINTERS* pex)
{
	return  DialogBoxParam( gDLLHandle,MAKEINTRESOURCE(IDD_CRITICAL_ERROR),NULL,DebugCallStack::ExceptionDialogProc,(LPARAM)pex );
}

#else
void MarkThisThreadForDebugging(const char*) {}
void UnmarkThisThreadFromDebugging() {}
void UpdateFPExceptionsMaskForThreads() {}
#endif //WIN32
