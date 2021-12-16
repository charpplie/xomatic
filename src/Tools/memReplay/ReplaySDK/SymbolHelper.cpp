//--------------------------------------------------------------------------------------
// SymbolHelper.cpp
//
// Microsoft Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "StdAfx.h"
#include "SymbolHelper.h"

#ifdef ENABLE_DIA
#include "xbdm.h"

#include <xdevkit.h>
#include <dbghelp.h>
#endif

#include <string>
#include <stdio.h>

#include "IInterfaceHooks.h"

namespace
{
	char* trim(char* str)
	{
		while (*str && isspace(*str))
			++ str;

		char* end = &str[strlen(str) - 1];
		while ((end > str) && isspace(*end))
			-- end;
		end[1] = '\0';

		return str;
	}

	bool Parse(char*& line, TAddress& out)
	{
		if (*line)
		{
			sscanf(line, "%08x", &out);
			while (*line && *line != ';')
				++ line;
			return true;
		}

		return false;
	}

	bool Parse(char*& line, ULONG& out)
	{
		if (*line)
		{
			sscanf(line, "%08x", &out);
			while (*line && *line != ';')
				++ line;
			return true;
		}

		return false;
	}
	
	bool Parse(char*& line, int& out)
	{
		if (*line)
		{
			sscanf(line, "%08x", &out);
			while (*line && *line != ';')
				++ line;
			return true;
		}

		return false;
	}

	bool ParseSemiColon(char*& line)
	{
		if (*line != ';')
			return false;

		++ line;
		return true;
	}

	bool Parse(char*& line, std::string& out)
	{
		if (*line)
		{
			char* end = line;
			while (*end && *end != ';')
				++ end;

			out.assign(line, end - line);
			line = end;

			return true;
		}

		return false;
	}
	
	bool Parse(char*& line, s8* &out)
	{
		out=NULL;
		if (*line)
		{
			char* end = line;
			while (*end && *end != ';')
				++ end;

			u32 size=1+(u32)(end-line);
			out=(s8*)malloc(size);
			strncpy(out, line, size-1);
			out[size-1]=0;

			line = end;

			return true;
		}
		return false;
	}

	void FindPDB(void* vpathOut)
	{
		char* pathOut = reinterpret_cast<char*>(vpathOut);

		char displayName[MAX_PATH] = {0};

		const char* pdbName = std::max(strrchr(pathOut, '\\'), strrchr(pathOut, '/'));
		pdbName = (pdbName == NULL) ? pathOut : pdbName + 1;

		TCHAR title[512];
		//_stprintf_s(title, 512, "Find folder containing %hs", pdbName);
		_stprintf_s(title, 512, "Find PDB for %hs", pdbName);

		std::string path = IInterfaceHooks::OpenFile("PDB Files (*.pdb)|*.pdb||", title); //IInterfaceHooks::OpenFolder(title);

		sprintf_s(pathOut, MAX_PATH, "%s", path.c_str());
	}
}

#ifdef ENABLE_DIA
// We link with dbghelp.lib to make it convenient to call functions in dbghelp.dll,
// however we specify delay loading of dbghelp.dll so that we can load a specific
// version in LoadDbgHelp. This is crucial since otherwise we may get the version in
// the system directory, and then dbghelp.dll will not load symsrv.dll.
#pragma comment(lib, "dbghelp.lib")

// Name: LoadDbgHelp
// Desc: Forcibly loads DbgHelp.dll from %XEDK%\bin\win32. This is done by calling
//       LoadLibrary with a fully specified path, since otherwise the version in the
//       system directory will take precedence.
//--------------------------------------------------------------------------------------
BOOL LoadDbgHelp(const char*& errorOut)
{
    // Get the XEDK environment variable.
    CHAR* xedkDir;
    size_t xedkDirSize;
    errno_t err = _dupenv_s( &xedkDir, &xedkDirSize, "xedk" );
    if( err || !xedkDir )
    {
        errorOut = "Couldn't read xedk environment variable.";
        return FALSE;
    }

    // Create a fully specified path to the XEDK version of dbghelp.dll
    // This is necessary because symsrv.dll must be in the same directory
    // as dbghelp.dll, and only a fully specified path can guarantee which
    // version of dbghelp.dll we load.
    std::string dbgHelpPath = std::string( xedkDir ) + "\\bin\\win32\\dbghelp.dll";

    // Free xedkDir
    if( xedkDir )
        free( xedkDir );

    // Call LoadLibrary on DbgHelp.DLL with our fully specified path.
    HMODULE hDbgHelp = LoadLibrary( dbgHelpPath.c_str() );

    // Print an error message and return FALSE if DbgHelp.DLL didn't load.
    if( !hDbgHelp )
    {
        errorOut = "ERROR: Couldn't load DbgHelp.dll from %%xedk%%\\bin\\win32.";
        return FALSE;
    }

    // DbgHelp.DLL loaded.
    return TRUE;
}


//--------------------------------------------------------------------------------------
// Name: cbSymbol
// Desc: DbgHelp callback which can be used for printing DbgHelp diagnostics.
//--------------------------------------------------------------------------------------
static BOOL CALLBACK cbSymbol( HANDLE/*hProcess*/, ULONG ActionCode, PVOID CallbackData, PVOID /*UserContext*/ )
{
    switch( ActionCode )
    {
        case CBA_DEBUG_INFO:
            printf( "%s", ( PSTR )CallbackData );
            break;

        default:
            return false;
    }

    return true;
}
#endif

SharedPtr<SymbolHelper> SymbolHelper::Create()
{
	return SharedPtr<SymbolHelper>(new SymbolHelper());
}

SharedPtr<SymbolHelper> SymbolHelper::FromFile(const char *filename)
{
	SharedPtr<SymbolHelper> syms(new SymbolHelper());

	if (syms->FromFileImpl(filename))
		return syms;

	return SharedPtr<SymbolHelper>();
}

SharedPtr<SymbolHelper> SymbolHelper::FromSerialised(IDeserialiser& ser)
{
	SharedPtr<SymbolHelper> symHelper = new SymbolHelper();

	int stringsLen = ser.Read<int>();
	std::vector<char> strings(stringsLen);
	ser.ReadRaw(&strings[0], stringsLen);

	int symbolsSize = ser.Read<int>();

	for (int i = 0; i != symbolsSize; ++ i)
	{
		TAddress addr = ser.Read<TAddress>();
		Symbol sym;

		sym.name = &strings[ser.Read<int>()];
		sym.filename = &strings[ser.Read<int>()];
		sym.displacement = ser.Read<u32>();
		sym.lineNumber = ser.Read<u32>();
		sym.next = ser.Read<u32>();

		symHelper->m_symbols.insert(std::make_pair(addr, sym));
	}

	return symHelper;
}

//--------------------------------------------------------------------------------------
// Name: SymbolHelper destructor
// Desc: Cleanup the symbol helper class.
//--------------------------------------------------------------------------------------
SymbolHelper::~SymbolHelper()
{
	EndDIA();
}

void SymbolHelper::Save(const char *filename)
{
	FILE* fp = fopen(filename, "wt");
	if (fp == NULL)
		return;

	for_citerator(SymbolMap, m_symbols, it)
	{
		fprintf(fp, "%08x;%s;%s;%08x;%08x;%08x\n",
			it->first, it->second.name.c_str(), it->second.filename.c_str(), 
			it->second.displacement, it->second.lineNumber, it->second.next);
	}

	fprintf(fp, "----\n");

	for (size_t i=0; i<m_typeList.size(); i++)
	{
		SaveTypeRecurse(fp, m_typeList[i]);
	}

	fclose(fp);
}

void SymbolHelper::SaveTypeRecurse(FILE *fp, TypeSymbol *sym)
{
	fprintf(fp, "%s;%s;%08x;%08x;%08x;%08x\n",sym->m_name,sym->m_typeName,sym->m_offset,sym->m_length,sym->m_count,sym->m_children.size());
	for (size_t i=0; i<sym->m_children.size(); i++)
	{
		SaveTypeRecurse(fp, sym->m_children[i]);
	}
}

TypeSymbol* SymbolHelper::LoadTypeRecurse(FILE *fp)
{
	std::vector<char> line(16384);
	if (!feof(fp))
	{
		TypeSymbol *sym=new TypeSymbol();
		u32 numChildren=0;
		fgets(&line[0], line.size(), fp);
		char* lineFront = trim(&line[0]);
		if (!Parse(lineFront, sym->m_name) || !ParseSemiColon(lineFront))
			return NULL;
		if (!Parse(lineFront, sym->m_typeName) || !ParseSemiColon(lineFront))
			return NULL;
		if (!Parse(lineFront, sym->m_offset) || !ParseSemiColon(lineFront))
			return NULL;
		if (!Parse(lineFront, sym->m_length) || !ParseSemiColon(lineFront))
			return NULL;
		if (!Parse(lineFront, sym->m_count) || !ParseSemiColon(lineFront))
			return NULL;
		if (!Parse(lineFront, numChildren))
			return NULL;
		for (u32 i=0; i<numChildren; i++)
		{
			sym->AddChild(LoadTypeRecurse(fp));
		}
		return sym;
	}
	return NULL;
}

void SymbolHelper::Serialise(ISerialiser &ser)
{
	typedef std::map<std::string, int> StringMap;

	StringMap strings;
	int stringsLen = 0;

	for_citerator(SymbolMap, m_symbols, it)
	{
		if (strings.find(it->second.name) == strings.end())
		{
			strings[it->second.name] = stringsLen;
			stringsLen += it->second.name.size() + 1;
		}
		if (strings.find(it->second.filename) == strings.end())
		{
			strings[it->second.filename] = stringsLen;
			stringsLen += it->second.filename.size() + 1;
		}
	}

	ser.Write(stringsLen);

	stringsLen = 0;
	for (StringMap::iterator it = strings.begin(), itEnd = strings.end(); it != itEnd; ++ it)
	{
		it->second = stringsLen;
		stringsLen += it->first.size() + 1;

		ser.Write(it->first.c_str(), it->first.c_str() + it->first.size() + 1);
	}

	ser.Write((int) m_symbols.size());

	for_citerator(SymbolMap, m_symbols, it)
	{
		TAddress addr = it->first;
		int nameStr = strings[it->second.name];
		int filenameStr = strings[it->second.filename];
		u32 disp = it->second.displacement;
		u32 line = it->second.lineNumber;
		u32 next = it->second.next;

		ser.Write(addr);
		ser.Write(nameStr);
		ser.Write(filenameStr);
		ser.Write(disp);
		ser.Write(line);
		ser.Write(next);
	}
}

void SymbolHelper::LoadSymbolsPS3(const char *filename)
{
#ifdef ENABLE_DIA
	if (m_unfself.empty())
		return;

	if (m_addr2line.empty())
		return;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char directory[4096];
	char cmd[4096];
	strcpy(directory, filename);
	if (strrchr(directory, '/'))
	{
		strrchr(directory, '/')[0]='\0';
	}
	CloseHandle(hStdInRead);
	CloseHandle(hStdInWrite);
	CloseHandle(hStdOutRead);
	CloseHandle(hStdOutWrite);
	CloseHandle(m_pi.hProcess);
	CloseHandle(m_pi.hThread);
	hStdInRead=NULL;
	hStdInWrite=NULL;
	hStdOutRead=NULL;
	hStdOutWrite=NULL;
	m_pi.hProcess=NULL;
	m_pi.hThread=NULL;
	ps3SymbolsOpen=FALSE;
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb=sizeof(STARTUPINFO);

	std::replace(m_unfself.begin(), m_unfself.end(), '\\', '/');
	std::replace(m_addr2line.begin(), m_addr2line.end(), '\\', '/');

	m_tempElf = directory;
	m_tempElf += "/temp.elf";

	sprintf(cmd, "\"%s\" \"%s\" \"%s\"", m_unfself.c_str(), filename, m_tempElf.c_str());
	if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		SECURITY_ATTRIBUTES sa;
		if (WaitForSingleObject(pi.hProcess, INFINITE)!=WAIT_OBJECT_0)
		{
			exit(1);
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		sprintf(cmd, "\"%s\" -Csife \"%s\"", m_addr2line.c_str(), m_tempElf.c_str());
		memset(&sa, 0, sizeof(sa));
		memset(&si, 0, sizeof(si));
		memset(&m_pi, 0, sizeof(m_pi));
		sa.nLength=sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle=TRUE;
		sa.lpSecurityDescriptor=NULL;
		if (CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0))
		{
			if (SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0))
			{
				if (CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0))
				{
					if (SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0))
					{
						si.cb=sizeof(STARTUPINFO);
						si.hStdInput=hStdInRead;
						si.hStdOutput=hStdOutWrite;
						si.hStdError=hStdOutWrite;
						si.dwFlags|=STARTF_USESTDHANDLES;
						if (CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &m_pi))
						{
							int i;
							m_vAddress=0xFFFFFFFF;
							ps3SymbolsOpen=TRUE;
							for (i=0; i<8; i++) // first 7 lookups are broke :(
							{
								const Symbol *temp;
								LookupSymbolPS3(0xC00000, temp);
							}
							m_symbols.clear();
							m_vAddress=0xFFFFFFFF;
						}
					}
				}
			}
		}
	}
#endif
}

VOID SymbolHelper::GetSymbol(TAddress address, const char*& name, const char*& filename, int& line ) const
{
	bool success = false;

	name = "Unknown";
	filename = "Unknown";
	line = 0;

	const Symbol* symbol = NULL;
	success = LookupSymbol(address, symbol);

	if (!success)
	{
		success = LookupSymbolPS3(address, symbol);
	}

	if( success )
	{
		// Print out the symbol name and the offset of the address from
		// that symbol.
		if( symbol->filename[0] )
		{
			filename = symbol->filename.c_str();
			name = symbol->name.c_str();
			line = symbol->lineNumber;
		}
	}
}

bool SymbolHelper::BeginDIA(const char*& errorOut)
{
#ifdef ENABLE_DIA
	errorOut = "";

	if (m_diaBegun)
	{
		errorOut = "DIA already begun";
		return false;
	}

	ps3SymbolsOpen=FALSE;
	hStdInRead=NULL;
	hStdInWrite=NULL;
	hStdOutRead=NULL;
	hStdOutWrite=NULL;
	memset(&m_pi, 0, sizeof(m_pi));

	// Initialize COM.
	if( FAILED( CoInitialize( NULL ) ) )
	{
		errorOut = "CoInitialize failed.";
		return false;
	}

	// Load DbgHelp. This must be done before any DbgHelp functions are used,
	// and it only works if DbgHelp.dll is delay loaded.
	if( !LoadDbgHelp(errorOut) )
	{
		return false;
	}

	// Most DbgHelp functions use a 'process' handle to identify their context.
	// This can be virtually any number, except zero.
	m_DebugProcess = ( HANDLE )1;

	// Enable DbgHelp debug messages, make sure that DbgHelp only loads symbols that
	// exactly match, and do deferred symbol loading for greater efficiency.
	SymSetOptions( SYMOPT_DEBUG | SYMOPT_EXACT_SYMBOLS | SYMOPT_DEFERRED_LOADS );

	// Create an XboxManager object to let us get the symbol server path.
	CComPtr <IXboxManager> spManager;
	HRESULT hr1 = spManager.CoCreateInstance( __uuidof( XboxManager ) );
	if( !SUCCEEDED( hr1 ) )
	{
		errorOut = "Couldn't create XboxManager object.";
		return false;
	}

	// Get the XEDK symbol server path.
	CComBSTR bstrSymbolServerPath = NULL;
	hr1 = spManager->get_SystemSymbolServerPath( &bstrSymbolServerPath );
	if( !SUCCEEDED( hr1 ) )
	{
		errorOut = "Couldn't get XEDK symbol server path.";
		return false;
	}

	// Convert the symbol server path from wide characters to char.
	char symbolServerPath[MAX_PATH];
	sprintf_s( symbolServerPath, "%S", bstrSymbolServerPath );

	// Now build up a complete symbol search path to give to DbgHelp.

	// Add the XEDK symbol server to the symbol search path.
	std::string fullSearchPath = symbolServerPath;

	CHAR* ntSymbolPath;
	size_t symbolPathSize;
	errno_t err = _dupenv_s( &ntSymbolPath, &symbolPathSize, "_NT_SYMBOL_PATH" );
	if( !err && ntSymbolPath )
	{
		fullSearchPath += ";" + std::string( ntSymbolPath );
		free( ntSymbolPath );
	}

	// Add the current directory to the search path.
	fullSearchPath += std::string( ";." );

	// Pass the symbol search path on to DbgHelp.
	SymInitialize( m_DebugProcess, const_cast<char*>( fullSearchPath.c_str() ), FALSE );

	// Set up a callback to help debug symbol loading problems. If symsrv.dll can't be loaded
	// then this will print a message to that effect.
	//SymRegisterCallback( m_DebugProcess, cbSymbol, NULL );
#endif

	m_diaBegun = true;

	return true;
}

//--------------------------------------------------------------------------------------
// Name: Load->ymbolsForModule
// Desc: Load the specified symbols using DbgHelp and DIA2. Return true for success.
//--------------------------------------------------------------------------------------
#ifdef ENABLE_DIA
bool SymbolHelper::LoadSymbolsForModule( const VOID* baseAddress,
                                         size_t size, DWORD/*timeStamp*/, const DM_PDB_SIGNATURE* signature )
{
	if (!m_diaBegun)
		return false;

	// Create a DIA2 data source
	CComPtr <IDiaDataSource> pSource;
	HRESULT hr = CoCreateInstance( __uuidof( DiaSource ),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof( IDiaDataSource ),
		( void** )&pSource );

	if( FAILED( hr ) )
		return false;

	char pdbPath[MAX_PATH];
	strcpy(pdbPath, signature->Path);

	// Ask DbgHelp to look for the PDB file. Because DbgHelp has previously been told
	// to use the XEDK symbol server this should locate system debug information.
	// Note: this function should ignore files whose signature doesn't match, but
	// in reality it does not. Thus, if a PDB file with the correct name but wrong
	// signature is found first, it will stop searching.
	char resultPath[MAX_PATH];
	BOOL findResult = SymFindFileInPath( m_DebugProcess, 0,
		pdbPath,
		const_cast<GUID*>( &signature->Guid ), signature->Age, 0,
		SSRVOPT_GUIDPTR,
		resultPath,
		NULL,
		NULL );

	// If DbgHelp found the symbols then adjust our path.
	if( findResult )
		strcpy(pdbPath, resultPath);

	const char* pdbName = std::max(strrchr(pdbPath, '\\'), strrchr(pdbPath, '/'));
	pdbName = pdbName ? pdbName + 1 : pdbPath;

	// Convert the filename to wide characters for use with DIA2.
	wchar_t wPdbPath[ _MAX_PATH ];
	size_t convertedChars;
	mbstowcs_s( &convertedChars, wPdbPath, pdbPath, _TRUNCATE );

	do 
	{
		// See if there is a PDB file at the specified location, and if so,
		// load it, checking the GUID and age to make sure that it is the correct
		// PDB file. Note that the timeStamp is not used anymore.
		hr = pSource->loadAndValidateDataFromPdb( wPdbPath, const_cast<GUID*>( &signature->Guid ), 0, signature->Age );

#define IS_CAUSE_FOR_SEARCH(r) (FAILED(r) && (r != E_PDB_INVALID_SIG) && (r != E_PDB_INVALID_AGE))
		//((r == E_PDB_NOT_FOUND) || (r == E_INVALIDARG) || (r == E_UNEXPECTED))

		if (IS_CAUSE_FOR_SEARCH(hr))
		{
			for (size_t altIdx = 0; IS_CAUSE_FOR_SEARCH(hr) && altIdx != m_alternativePaths.size(); ++ altIdx)
			{
				char pdbPath2[MAX_PATH];
				sprintf_s(pdbPath2, MAX_PATH, "%s\\%s", m_alternativePaths[altIdx].c_str(), pdbName);

				strcpy(pdbPath, pdbPath2);
				pdbName = std::max(strrchr(pdbPath, '\\'), strrchr(pdbPath, '/'));
				pdbName = pdbName ? pdbName + 1 : pdbPath;

				mbstowcs_s( &convertedChars, wPdbPath, pdbPath, _TRUNCATE );

				hr = pSource->loadAndValidateDataFromPdb(wPdbPath, const_cast<GUID*>( &signature->Guid ), 0, signature->Age );
			}

			while (IS_CAUSE_FOR_SEARCH(hr))
			{
				char name[MAX_PATH] = {0};
				sprintf_s(name, MAX_PATH, "%s", pdbPath);

				//  Replaying may be happening on a worker thread, so make sure that if that is the case
				//  we execute the UI stuff on the main thread.
				IInterfaceHooks::InvokeOnMain(FindPDB, name);

				if (name[0])
				{
					char path[MAX_PATH];
					sprintf_s(path, MAX_PATH, "%s", name);
					if (char* term = std::max(strrchr(path, '\\'), strrchr(path, '/')))
						*term = '\0';
					if (std::find(m_alternativePaths.begin(), m_alternativePaths.end(), std::string(path)) == m_alternativePaths.end())
						m_alternativePaths.push_back(path);

					mbstowcs_s( &convertedChars, wPdbPath, name, _TRUNCATE );

					hr = pSource->loadAndValidateDataFromPdb(wPdbPath, const_cast<GUID*>( &signature->Guid ), 0, signature->Age );
				}
				else
				{
					goto uglyuglygoto;
				}
			}
		}
	}
	while (IS_CAUSE_FOR_SEARCH(hr));

uglyuglygoto:

	if( FAILED( hr ) )
	{
		// Check the error code for details on why the load failed, which
		// could be because the file doesn't exist, signature doesn't match,      
		// etc.

		bool loadWithoutValidation = false;

		TCHAR* reason = _T("Unknown error");
		switch (hr)
		{
		case E_PDB_NOT_FOUND: reason = _T("PDB not found"); break;
		case E_PDB_FORMAT: reason = _T("PDB format"); break;
		case E_PDB_INVALID_SIG: reason = _T("PDB invalid sig"); loadWithoutValidation = true; break;
		case E_PDB_INVALID_AGE: reason = _T("PDB invalid age"); loadWithoutValidation = true; break;
		case E_INVALIDARG: reason = _T("Invalid arg"); break;
		case E_UNEXPECTED: reason = _T("Unexpected"); break;
		}

		TCHAR msg[256];
		_stprintf_s(msg, 256, "Failed to import PDB %s, %s %s\n", pdbPath, reason, loadWithoutValidation ? "loading without validation" : "");
		OutputDebugString(msg);

		if (!loadWithoutValidation)
			return false;

		hr = pSource->loadDataFromPdb(wPdbPath);

		if (FAILED(hr))
		{
			TCHAR msg[256];
			_stprintf_s(msg, 256, "Failed to forcibly import PDB %s\n", pdbPath);
			OutputDebugString(msg);

			return false;
		}
	}

	// Create a session for the just loaded PDB file.
	CComPtr <IDiaSession> psession;
	if( FAILED( pSource->openSession( &psession ) ) )
	{
		return false;
	}

	// Tell DIA2 where the module was loaded.
	psession->put_loadAddress( ( ULONG_PTR )baseAddress );

	bool gatherTypes=false;
	if (strstri(pdbPath, "GameDll") || strstri(pdbPath, "/Cry") || strstri(pdbPath, "\\Cry"))
		gatherTypes=true;

#undef IS_CAUSE_FOR_SEARCH

	// Add this session to a list of loaded modules.
	m_ModuleList.push_back( Module( ( ULONG_PTR )baseAddress, size, psession, gatherTypes ) );
	return true;
}
#endif

void SymbolHelper::SetUnFSelf(const char* filename)
{
#if ENABLE_DIA
	m_unfself = filename;
#endif
}

void SymbolHelper::SetAddr2Line(const char* filename)
{
#if ENABLE_DIA
	m_addr2line = filename;
#endif
}

void SymbolHelper::EndDIA()
{
	if (m_diaBegun)
	{
#ifdef ENABLE_DIA
		if (hStdInRead)
			CloseHandle(hStdInRead);
		if (hStdInWrite)
			CloseHandle(hStdInWrite);
		if (hStdOutRead)
			CloseHandle(hStdOutRead);
		if (hStdOutWrite)
			CloseHandle(hStdOutWrite);
		if (m_pi.hProcess)
			CloseHandle(m_pi.hProcess);
		if (m_pi.hThread)
			CloseHandle(m_pi.hThread);
		hStdInRead=NULL;
		hStdInWrite=NULL;
		hStdOutRead=NULL;
		hStdOutWrite=NULL;
		m_pi.hProcess=NULL;
		m_pi.hThread=NULL;
		ps3SymbolsOpen=FALSE;

		if (!m_tempElf.empty() && GetFileAttributes(m_tempElf.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			std::string elf = m_tempElf;
			std::replace(elf.begin(), elf.end(), '/', '\\');
			unlink(elf.c_str());
		}

		// Free our DIA2 objects prior to shutting down COM.
		m_ModuleList.clear();

		// Shut down COM.
		CoUninitialize();
#endif

		m_diaBegun = false;
	}
}

namespace
{
	struct FindFileLineMatchingAddresses_SymbolLess
	{
		bool operator () (const SymbolHelper::Symbol* a, const SymbolHelper::Symbol* b) const
		{
			if (a->filename != b->filename) return a->filename < b->filename;
			return a->lineNumber < b->lineNumber;
		}
	};

	struct FindFileMatchingAddresses_SymbolLess
	{
		bool operator () (const SymbolHelper::Symbol* a, const SymbolHelper::Symbol* b) const
		{
			return a->filename < b->filename;
		}
	};

	struct FindFunctionMatchingAddresses_SymbolLess
	{
		bool operator () (const SymbolHelper::Symbol* a, const SymbolHelper::Symbol* b) const
		{
			return a->name < b->name;
		}
	};

	template <typename Less, typename Iterator>
	void FindMatchingAddresses(Iterator symBegin, Iterator symEnd, const SymbolHelper::Symbol* sym, std::vector<TAddress>& addressSetOut)
	{
		typedef std::multimap<const SymbolHelper::Symbol*, TAddress, Less> MatchMap;

		MatchMap mm;
		for (Iterator it = symBegin, itEnd = symEnd; it != itEnd; ++ it)
			mm.insert(std::make_pair(&it->second, it->first));

		for (MatchMap::iterator it = mm.lower_bound(sym), itEnd = mm.upper_bound(sym); it != itEnd; ++ it)
			addressSetOut.push_back(it->second);

		std::sort(addressSetOut.begin(), addressSetOut.end());
	}
}

void SymbolHelper::FindFileLineMatchingAddresses(std::vector<TAddress>& addressSetOut, TAddress find) const
{
	const Symbol* sym;
	if (!LookupSymbol(find, sym))
	{
		addressSetOut.clear();
		return;
	}
	FindMatchingAddresses<FindFileLineMatchingAddresses_SymbolLess>(SymbolsBegin(), SymbolsEnd(), sym, addressSetOut);
}

void SymbolHelper::FindFileMatchingAddresses(std::vector<TAddress>& addressSetOut, TAddress find) const
{
	const Symbol* sym;
	if (!LookupSymbol(find, sym))
	{
		addressSetOut.clear();
		return;
	}
	FindMatchingAddresses<FindFileMatchingAddresses_SymbolLess>(SymbolsBegin(), SymbolsEnd(), sym, addressSetOut);
}

void SymbolHelper::FindFunctionMatchingAddresses(std::vector<TAddress>& addressSetOut, TAddress find) const
{
	const Symbol* sym;
	if (!LookupSymbol(find, sym))
	{
		addressSetOut.clear();
		return;
	}
	FindMatchingAddresses<FindFunctionMatchingAddresses_SymbolLess>(SymbolsBegin(), SymbolsEnd(), sym, addressSetOut);
}

//--------------------------------------------------------------------------------------
// Name: SymbolHelper constructor
// Desc: Do necessary setup for locating and loading symbols. This includes initializing
//       COM, ensuring that the necessary DLLs are in the correct location, initializing
//       DbgHelp and passing the XEDK symbol server path on to DbgHelp.
//--------------------------------------------------------------------------------------
SymbolHelper::SymbolHelper()
{
	m_diaBegun = false;

#ifdef ENABLE_DIA
	ps3SymbolsOpen=FALSE;
	hStdInRead=NULL;
	hStdInWrite=NULL;
	hStdOutRead=NULL;
	hStdOutWrite=NULL;
	memset(&m_pi, 0, sizeof(m_pi));
#endif
}

bool SymbolHelper::FromFileImpl(const char* filename)
{
	FileRAII fp(fopen(filename, "rt"));
	if (!fp.IsValid())
		return false;

	std::vector<char> line(16384);

	while (!feof(fp))
	{
		fgets(&line[0], line.size(), fp);

		char* lineFront = trim(&line[0]);

		if (strcmp(lineFront, "----")==0)
			break;
		
		TAddress address = 0;
		if (!Parse(lineFront, address) || !ParseSemiColon(lineFront))
			return false;

		Symbol sym;
		
		if (!Parse(lineFront, sym.name) || !ParseSemiColon(lineFront))
			return false;

		if (!Parse(lineFront, sym.filename) || !ParseSemiColon(lineFront))
			return false;

		if (!Parse(lineFront, sym.displacement) || !ParseSemiColon(lineFront))
			return false;

		if (!Parse(lineFront, sym.lineNumber))
			return false;
		
		if (!ParseSemiColon(lineFront) || !Parse(lineFront, sym.next))
			sym.next=0;

		m_symbols.insert(std::make_pair(address, sym));
	}

	TypeSymbol *type;
	while (type=LoadTypeRecurse(fp))
	{
		m_typeList.push_back(type);
	}

	return true;
}

bool SymbolHelper::LookupSymbol(DWORD address, const Symbol*& out) const
{
	SymbolMap::const_iterator it = m_symbols.find(address);

	if (it != m_symbols.end())
	{
		out = &it->second;
		return true;
	}

	bool success = false;

#ifdef ENABLE_DIA
	{
		static std::vector<CHAR> symbolName(10000);
		memset(&symbolName[0], 0, symbolName.size() * sizeof(CHAR));

		//CHAR symbolName[10000] = {0};
		ULONG displacement = 0;
		CHAR filename[500] = {0};
		ULONG lineNumber = 0;

		// Scan through the list of loaded modules to find the one that contains the
		// requested address.
		for( size_t i = 0; i < m_ModuleList.size(); ++i )
		{
			// Find what module's address range the address falls in.
			if( address > m_ModuleList[i].m_Address &&
				address < m_ModuleList[i].m_Address + m_ModuleList[i].m_Size )
			{
				const CComPtr <IDiaSession>& pSession = m_ModuleList[i].m_psession;

				// Find the symbol using the virtual address--the raw address. This
				// only works if you have previously told DIA where the module was
				// loaded, using put_loadAddress.
				// Specify SymTagPublicSymbol instead of SymTagFunction if you want
				// the full decorated names.
				CComPtr <IDiaSymbol> pFunc;
				HRESULT result = pSession->findSymbolByVA( address, SymTagFunction, &pFunc );
				if( SUCCEEDED( result ) && pFunc )
				{
					// Get the name of the function.
					CComBSTR functionName = 0;
					pFunc->get_name( &functionName );
					if( functionName )
					{
						// Convert the function name from wide characters to char.
						sprintf_s( &symbolName[0], symbolName.size(), "%S", functionName );

						// Get the offset of the address from the symbol's address.
						ULONGLONG symbolBaseAddress;
						pFunc->get_virtualAddress( &symbolBaseAddress );
						displacement = address - ( ULONG )symbolBaseAddress;
						success = true;

						// Now try to get the filename and line number.
						// Get an enumerator that corresponds to this instruction.
						CComPtr <IDiaEnumLineNumbers> pLines;
						const DWORD instructionSize = 4;
						if( SUCCEEDED( pSession->findLinesByVA( address, instructionSize, &pLines ) ) )
						{
							// We could loop over all of the source lines that map to this instruction,
							// but there is probably at most one, and if there are multiple source
							// lines we still only want one.
							CComPtr <IDiaLineNumber> pLine;
							DWORD celt;
							if( SUCCEEDED( pLines->Next( 1, &pLine, &celt ) ) && celt == 1 )
							{
								// Get the line number.
								pLine->get_lineNumber( &lineNumber );

								// Get the source file object, and then its name.
								CComPtr <IDiaSourceFile> pSrc;
								pLine->get_sourceFile( &pSrc );
								CComBSTR sourceName = 0;
								pSrc->get_fileName( &sourceName );
								// Convert from wide characters to ASCII.
								sprintf_s( filename, "%S", sourceName );
							}
						}
					}
				}
			}
		}

		if (success)
		{
			Symbol sym;
			sym.filename = filename;
			sym.name = &symbolName[0];
			sym.lineNumber = lineNumber;
			sym.displacement=displacement;
			sym.next=0;
			SymbolMap::iterator it = m_symbols.insert(SymbolMap::value_type(address, sym)).first;
			out = &it->second;
			return true;
		}
	}

	success = LookupSymbolPS3(address, out);
#endif

	return success;
}
	
TAddress SymbolHelper::GetNext(TAddress address) const
{
	const Symbol *out;
	if (LookupSymbol(address, out))
		return out->next;
	return 0;
}

char *SymbolHelper::ReadSymbol(char *symbolName, DWORD address, Symbol *&symbolOut) const
{
	char *nextSymbol=NULL;
	char *line;
//	char out[1024];
//	sprintf(out, " %08x ", address);
//	OutputDebugString("ReadSymbol:");
//	OutputDebugString(out);
//	OutputDebugString(symbolName);
//	OutputDebugString("\n");
	line=strchr(symbolName, '\n');
	line[0]='\0';
	line++;
	if (symbolName[0]=='.')
		symbolName++;

	char* lineNumber = strchr(line, ':');
	if (lineNumber)
	{
		*lineNumber = 0;
		++lineNumber;
		nextSymbol=strchr(lineNumber, '\n');
		if (nextSymbol)
		{
			nextSymbol[0]='\0';
			nextSymbol++;
		}
	}

	Symbol sym;
	sym.filename = line;
	if (symbolName[0]=='?' && symbolName[1]=='?' && symbolName[2]=='\0')
		symbolName="(inlined)";
	sym.name = symbolName;
	sym.lineNumber = atoi(lineNumber);
	sym.displacement = 0;
	sym.next=0;
	SymbolMap::iterator it = m_symbols.insert(SymbolMap::value_type(address, sym)).first;
	symbolOut = &it->second;

	return nextSymbol;
}


bool SymbolHelper::LookupSymbolPS3(DWORD address, const Symbol*& symbolOut) const
{
#ifdef ENABLE_DIA
	if (ps3SymbolsOpen)
	{
		DWORD written;
		char symbolNameStorage[16384];
		char *symbolName=symbolNameStorage;
		char line[16384];
		memset(symbolNameStorage, 0, sizeof(symbolNameStorage));
		sprintf(line, "%08x\n\n", address);
		FlushFileBuffers(hStdOutRead);
		if (WriteFile(hStdInWrite, line, strlen(line), &written, NULL))
		{
			DWORD read;
			char *addressBreak=NULL;
			char out;
			int len=0;
			FlushFileBuffers(hStdInWrite);
			while (1)
			{
				ReadFile(hStdOutRead, &out, sizeof(out), &read, NULL);
				if (out=='\r' || read==0)
					continue;
				symbolName[len++]=out;
				addressBreak=strstr(symbolName, "\n??\n??:0\n");
				if (addressBreak)
				{
					break;
				}
			}
			addressBreak[0]='\0';

			Symbol *thisSymbol;
			symbolName = ReadSymbol(symbolName, address, thisSymbol);
			symbolOut=thisSymbol;
			assert(thisSymbol);
			DWORD lastAddress=address;
			while (symbolName)
			{
				symbolName=ReadSymbol(symbolName, m_vAddress, thisSymbol);
				SymbolMap::iterator it = m_symbols.find(lastAddress); // could have moved by allocation in ReadSymbol
				assert(it!=m_symbols.end());
				it->second.next=m_vAddress;
				lastAddress=m_vAddress;
				m_vAddress--;
			}

			return true;
		}
	}
#endif

	return false;
}

__declspec(noinline) void TrapError(void)
{
	OutputDebugString("A problem occurred looking up debug symbols. You'll probably want to breakpoint this line.");
}

#define CHECK_RESULT(a) (a==S_OK)
#define CHECK_RESULT_RETURN(a) do { HRESULT result=a; if (result!=S_OK) { TrapError(); return true; } } while (0)
#define CHECK_RESULT_CONTINUE(a) do { HRESULT result=a; if (result!=S_OK) { TrapError(); continue; } } while (0)

#ifdef ENABLE_DIA
static int DumpSymbolInfo(TypeSymbol *parent, IDiaSymbol *pInType, int scale)
{
	CComPtr <IDiaSymbol> pType;
	IDiaSymbol *pBaseType=NULL;
	IDiaEnumSymbols* pTypeList;
	ULONGLONG length;
	LONG offset;
	ULONG fetched;
	DWORD tag;
	BSTR name;
	BSTR typeName;
	int lastOffset=-1;
	int empty=true;
	CHECK_RESULT_RETURN(pInType->findChildren(SymTagNull, NULL, nsNone, &pTypeList));
	while (CHECK_RESULT(pTypeList->Next(1, &pType, &fetched)) && fetched==1)
	{
		bool keepGoing=true;
		DWORD arrayCount=1;
		ULONGLONG bitScale=8;
		CHECK_RESULT_CONTINUE(pType->get_symTag(&tag));
		offset=0;
		if (tag==SymTagBaseClass)
		{
			CHECK_RESULT_CONTINUE(pType->get_length(&length));
			if (!CHECK_RESULT(pType->get_name(&name)))
			{
				name=SysAllocString(L"Class/Struct");
			}
			TypeSymbol *sym=new TypeSymbol();
			sym->SetName(name);
			sym->SetTypeName(name);
			sym->SetOffset(-1);
			sym->SetLength(length*scale);
			sym->SetCount(scale);
			SysFreeString(name);
			if (DumpSymbolInfo(sym, pType, scale))
			{
				parent->AddChild(sym);
				empty=false;
			}
			else
			{
				delete sym;
			}
			pType=NULL;
			continue;
		}
		else if (tag==SymTagVTable)
		{
			if (!CHECK_RESULT(pType->get_name(&name)))
			{
				name=SysAllocString(L"V-Table");
			}
			keepGoing=false;
		}
		else if (tag==SymTagData)
		{
			DWORD locType;
			if (!CHECK_RESULT(pType->get_name(&name)))
			{
				name=SysAllocString(L"Unknown");
			}
			CHECK_RESULT_CONTINUE(pType->get_locationType(&locType));
			if (locType!=LocIsThisRel && locType!=LocIsBitField)
			{
				pType=NULL;
				SysFreeString(name);
				continue;
			}
			CHECK_RESULT_CONTINUE(pType->get_offset(&offset));
			offset*=8;
			if (locType==LocIsBitField)
			{
				DWORD bitPos;
				CHECK_RESULT_CONTINUE(pType->get_bitPosition(&bitPos));
				CHECK_RESULT_CONTINUE(pType->get_length(&length));
				offset+=bitPos;
				bitScale=1;
			}
			if ((int)offset<=lastOffset && locType!=LocIsBitField) // union or bit field weirdness
			{
				pType=NULL;
				SysFreeString(name);
				break;
			}
			lastOffset=(int)offset;
		}
		else
		{
			pType=NULL;
			continue;
		}
		CHECK_RESULT_CONTINUE(pType->get_type(&pBaseType));
		if (bitScale!=1)
		{
			CHECK_RESULT_CONTINUE(pBaseType->get_length(&length));
			length*=bitScale;
		}
		CHECK_RESULT_CONTINUE(pBaseType->get_symTag(&tag));
		if (tag==SymTagArrayType)
		{
			IDiaSymbol *pRealBaseType;
			CHECK_RESULT_CONTINUE(pBaseType->get_type(&pRealBaseType));
			CHECK_RESULT_CONTINUE(pBaseType->get_count(&arrayCount));
			pBaseType->Release();
			pBaseType=pRealBaseType;
		}
		if (!CHECK_RESULT(pBaseType->get_name(&typeName)))
		{
			DWORD btType;
			if (!CHECK_RESULT(pBaseType->get_baseType(&btType)))
				btType=btNoType;
			switch (btType)
			{
			case btVoid: 
				typeName=SysAllocString(L"void");
				break;
			case btChar:
				typeName=SysAllocString(L"char");
				break;
			case btWChar:
				typeName=SysAllocString(L"wchar");
				break;
			case btInt:
				typeName=SysAllocString(L"int");
				break;
			case btUInt:
				typeName=SysAllocString(L"uint");
				break;
			case btFloat:
				typeName=SysAllocString(L"float");
				break;
			case btBool:
				typeName=SysAllocString(L"bool");
				break;
			case btLong:
				typeName=SysAllocString(L"LONG");
				break;
			case btULong:
				typeName=SysAllocString(L"ULONG");
				break;
			case btBit:
				typeName=SysAllocString(L"Bit");
				break;
			default:
				if (tag==SymTagPointerType)
				{
					typeName=SysAllocString(L"void*");
				}
				else
				{
					typeName=SysAllocString(L"CType");
				}
				break;
			}
			keepGoing=false;
		}
		TypeSymbol *sym=new TypeSymbol();
		sym->SetName(name);
		sym->SetTypeName(typeName);
		sym->SetOffset(offset);
		sym->SetLengthInBits(length*scale);
		sym->SetCount(arrayCount*scale);
		parent->AddChild(sym);
		SysFreeString(name);
		SysFreeString(typeName);
		empty=false;
		pType=NULL;
		if (keepGoing)
		{
			DumpSymbolInfo(sym, pBaseType, scale*arrayCount);
		}
		pBaseType->Release();
	}
	pTypeList->Release();
	return !empty;
}
#endif

void SymbolHelper::GetAllTypes(void)
{
#ifdef ENABLE_DIA
	for( size_t i = 0; i < m_ModuleList.size(); ++i )
	{
		if (m_ModuleList[i].m_gatherTypes)
		{
			const CComPtr <IDiaSession>& pSession = m_ModuleList[i].m_psession;
			IDiaSymbol *pParent;
			CHECK_RESULT_CONTINUE(pSession->get_globalScope(&pParent));
			if (pParent)
			{
				IDiaEnumSymbols* pTypeList;
				CHECK_RESULT_CONTINUE(pSession->findChildren(pParent, SymTagUDT, NULL, nsNone, &pTypeList));
				if (pTypeList)
				{
					CComPtr <IDiaSymbol> pType;
					ULONG fetched;
					while (CHECK_RESULT(pTypeList->Next(1, &pType, &fetched)) && fetched==1)
					{
						ULONGLONG length;
						CHECK_RESULT_CONTINUE(pType->get_length(&length));
						if (length>0)
						{
							BSTR name;
							size_t l=m_typeList.size();
							s8 cname[64*1024];
							length*=8;
							if (!CHECK_RESULT(pType->get_name(&name)))
							{
								name=SysAllocString(L"Unknown");
							}
							else
							{
								sprintf_s(cname, sizeof(cname), "%ls", name);
								for (l=0; l<m_typeList.size(); l++)
								{
									if (m_typeList[l]->m_length==length && strcmp(cname, m_typeList[l]->m_name)==0)
										break;
								}
							}
							if (l==m_typeList.size())
							{
								TypeSymbol *sym=new TypeSymbol();
								sym->SetName(name);
								sym->SetTypeName(name);
								sym->SetOffset(0);
								sym->SetLengthInBits(length);
								DumpSymbolInfo(sym, pType, 1);
								m_typeList.push_back(sym);
							}
							SysFreeString(name);
						}
						pType=NULL;
					}
					pTypeList->Release();
				}
				pParent->Release();
			}
		}
	}
#endif
}

TypeSymbol* SymbolHelper::FindType(const s8 *name)
{
	for (size_t i=0; i<m_typeList.size(); i++)
	{
		if (strcmp(m_typeList[i]->m_name, name)==0)
			return m_typeList[i];
	}
	return NULL;
}