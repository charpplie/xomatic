//--------------------------------------------------------------------------------------
// SymbolHelper.h
//
// Microsoft Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef SYMBOLHELPER_H
#define SYMBOLHELPER_H

#include "ISymbolTable.h"
#include "TypeSymbol.h"

#include <vector>
#include <map>

#ifdef ENABLE_DIA
#include <atlbase.h>
#include <xbdm.h>
#include "dia2.h"
#endif

#ifdef ENABLE_DIA
//--------------------------------------------------------------------------------------
// Name: class Sample
// Desc: Structure for keeping track of loaded symbols when using DIA2.
//       Each Module object records the memory range (start address and size) covered by
//       this code module, along with a DIA2 session object to use for symbol lookups.
//--------------------------------------------------------------------------------------
struct Module
{
    Module( ULONG_PTR address, size_t size, CComPtr <IDiaSession>& psession, bool gatherTypes ) : m_Address( address ),
                                                                                m_Size( size ),
                                                                                m_psession( psession ),
																																								m_gatherTypes(gatherTypes)
    {
    }

    ULONG_PTR m_Address;
    size_t m_Size;
    CComPtr <IDiaSession> m_psession;
		bool m_gatherTypes;
};
#endif

//--------------------------------------------------------------------------------------
// Name: class SymbolHelper
// Desc: Helper class for hiding the details of finding and loading symbols. Finding
//       symbols is done with DbgHelp and symbol lookups are done with DIA2. Additional
//       symbol lookup functions can be added as needed.
//--------------------------------------------------------------------------------------
class SymbolHelper : public ISymbolTable
{
public:
	struct Symbol
	{
		std::string name;
		std::string filename;
		ULONG displacement;
		ULONG lineNumber;
		ULONG next;
	};

	typedef std::map<TAddress, Symbol> SymbolMap;
	typedef SymbolMap::const_iterator SymbolIterator;

public:
	static SharedPtr<SymbolHelper> Create();
	static SharedPtr<SymbolHelper> FromFile(const char* filename);
	static SharedPtr<SymbolHelper> FromSerialised(IDeserialiser& ser);

public: // ISymbolTable Members
	void GetSymbol(TAddress address, const char*& name, const char*& filename, int& line ) const;
	TAddress GetNext(TAddress address) const;

	void FindFileLineMatchingAddresses(std::vector<TAddress>& addressSetOut, TAddress find) const;
	void FindFileMatchingAddresses(std::vector<TAddress>& addressSetOut, TAddress find) const;
	void FindFunctionMatchingAddresses(std::vector<TAddress>& addressSetOut, TAddress find) const;

public:
	~SymbolHelper();

	void Save(const char* filename);
	void Serialise(ISerialiser& ser);

	void ImportSymbol(TAddress address)
	{
		const Symbol* symbol;
		LookupSymbol(address, symbol);
	}

	// All symbol importing must be done between BeginDIA()/EndDIA() calls, and must all
	// occur on the same thread.
	bool BeginDIA(const char*& errorOut);

#ifdef ENABLE_DIA
	// Attempt to load the symbols for the specified module, including looking in the
	// XEDK symbol server. This function prints a success/failure message and returns
	// true for success.
	bool LoadSymbolsForModule(const VOID* baseAddress, size_t size, DWORD timeStamp, const DM_PDB_SIGNATURE* signature);
#endif

	void SetUnFSelf(const char* filename);
	void SetAddr2Line(const char* filename);

	void LoadSymbolsPS3(const char *filename);
	void GetAllTypes(void);
	TypeSymbol* FindType(const s8 *name);

	void EndDIA();

	SymbolIterator SymbolsBegin() const				{ return m_symbols.begin(); }
	SymbolIterator SymbolsEnd() const					{ return m_symbols.end(); }

private:
	SymbolHelper();

	SymbolHelper(const SymbolHelper&);
	SymbolHelper& operator = (const SymbolHelper& other);

	bool FromFileImpl(const char* filename);

	bool LookupSymbol(DWORD address, const Symbol*& out) const;
	bool LookupSymbolPS3(DWORD address, const Symbol*& out) const;
	
	char *ReadSymbol(char *symbolName, DWORD address, Symbol *&symbolOut) const;

	void SaveTypeRecurse(FILE *fp, TypeSymbol *sym);
	TypeSymbol* LoadTypeRecurse(FILE *fp);
private:
	bool m_diaBegun;
	mutable ULONG m_vAddress;

#ifdef ENABLE_DIA
	// List of code modules (DLLs and EXEs) to look for symbols in.
	std::vector <Module> m_ModuleList;

	// Unique identifier for DbgHelp functions.
	HANDLE m_DebugProcess;

	std::string m_unfself;
	std::string m_addr2line;

	int ps3SymbolsOpen;
	PROCESS_INFORMATION m_pi;

	HANDLE hStdInRead;
	HANDLE hStdInWrite;
	HANDLE hStdOutRead;
	HANDLE hStdOutWrite;
	std::string m_tempElf;

	std::vector<std::string> m_alternativePaths;
#endif
	
	std::vector<TypeSymbol*> m_typeList;

	mutable SymbolMap m_symbols;
};

#endif
