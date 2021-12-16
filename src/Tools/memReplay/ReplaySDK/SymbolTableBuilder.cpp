#include "stdafx.h"
#include "SymbolTableBuilder.h"

#include "IInterfaceHooks.h"

static void MissingDIA(void* verror)
{
	const char* error = reinterpret_cast<const char*>(verror);

	TCHAR msg[512];
	_stprintf_s(msg, sizeof(msg), _T("Couldn't start DIA - code symbols won't be available.\n\nError: %hs"), error);

	IInterfaceHooks::ShowMessage(msg, _T("Error"));
}

SymbolTableBuilder::SymbolTableBuilder(const std::string& filename)
	: m_filename(filename)
	, m_handledSelf(false)
{
	m_symbols = SymbolHelper::Create();
}

void SymbolTableBuilder::ReplayBegin()
{
	const char* error;
	if (!m_symbols->BeginDIA(error))
	{
		IInterfaceHooks::InvokeOnMain(MissingDIA, const_cast<void*>(reinterpret_cast<const void*>(error)));
	}

	m_handledSelf = false;

#ifdef ENABLE_DIA
	m_handledPdbs.clear();
#endif
}

static void FindPS3Launcher(void* vpathOut)
{
	char* pathOut = reinterpret_cast<char*>(vpathOut);

	std::string path = IInterfaceHooks::OpenFile("Self Files (*.self)|*.self||", _T("Find PS3 self"));
	sprintf_s(pathOut, MAX_PATH, "%s", path.c_str());
}

static void FindUnFSelf(void* vpathOut)
{
	char* pathOut = reinterpret_cast<char*>(vpathOut);

	std::string path = IInterfaceHooks::OpenFile("unfself.exe|unfself.exe||", _T("Find unfself.exe"));
	sprintf_s(pathOut, MAX_PATH, "%s", path.c_str());
}

static void FindAddr2Line(void* vpathOut)
{
	char* pathOut = reinterpret_cast<char*>(vpathOut);

	std::string path = IInterfaceHooks::OpenFile("ppu-lv2-addr2line.exe|ppu-lv2-addr2line.exe||", _T("Find ppu-lv2-addr2line.exe"));
	sprintf_s(pathOut, MAX_PATH, "%s", path.c_str());
}

void SymbolTableBuilder::Replay(ReplayRange reader)
{
	ReplayEventIds::Ids id;

	while (reader.ReadNext(id))
	{
		switch (id)
		{
		case ReplayEventIds::RE_Alloc3:
			{
				const ReplayAlloc3Event& ev = reader.Get<ReplayAlloc3Event>();

				for (int i = 0; i != ev.callstackLength; ++ i)
				{
					TAddress addr = ev.callstack[i];

					std::vector<byte>& pages = GetPage(addr);
					pages[(addr & AddrMask) >> 3] |= 1 << (addr & 0x7);
				}
			}
			break;

		case ReplayEventIds::RE_Free4:
			{
				const ReplayFree4Event& ev = reader.Get<ReplayFree4Event>();

				for (int i = 0; i != ev.callstackLength; ++ i)
				{
					TAddress addr = ev.callstack[i];

					std::vector<byte>& pages = GetPage(addr);
					pages[(addr & AddrMask) >> 3] |= 1 << (addr & 0x7);
				}
			}
			break;

		case ReplayEventIds::RE_PS3ModuleRef:
			{
				const ReplayPS3ModuleRefEvent& ev = reader.Get<ReplayPS3ModuleRefEvent>();

				if (!m_handledSelf)
				{
					char path[MAX_PATH];
					IInterfaceHooks::InvokeOnMain(FindPS3Launcher, path);

					if (path[0])
					{
						std::replace(path, path + strlen(path), '\\', '/');

						// Check for unfself and ppu-lv2-addr2line
						ImportPS3Tools(path);

						m_symbols->LoadSymbolsPS3(path);
						m_handledSelf = true;
					}
				}
			}
			break;

		case ReplayEventIds::RE_XenonModuleRef:
			{
				const ReplayXenonModuleRefEvent& ev = reader.Get<ReplayXenonModuleRefEvent>();

#ifdef ENABLE_DIA
				// Parse the numbers on the last line.
				VOID* baseAddress = 0;
				size_t size = 0;
				DWORD timeStamp = 0;
				DM_PDB_SIGNATURE signature = {0};

				// The 'numbers' line describes the module address, size, timestamp,
				// PDB GUID, and PDB 'age'
				// A typical numbers line looks like this:
				// 82000000, 7077888, 458C1F76, {393A724D-577C-4DC5-B499-069DE207353D}, 11
				// address,  size,    timestamp, Data1,  Data2,Data3, ---Data4---
				int count = sscanf_s( ev.signature, "%x, %x, %x, {%x-%x-%x-%x %x-%x %x %x %x %x %x}, %d",
					&baseAddress, &size, &timeStamp,
					&signature.Guid.Data1, &signature.Guid.Data2, &signature.Guid.Data3,
					&signature.Guid.Data4[0], &signature.Guid.Data4[1],
					&signature.Guid.Data4[2], &signature.Guid.Data4[3],
					&signature.Guid.Data4[4], &signature.Guid.Data4[5],
					&signature.Guid.Data4[6], &signature.Guid.Data4[7],
					&signature.Age );

				strcpy_s( signature.Path, ev.path );
				signature.Path[ _countof( signature.Path ) - 1 ] = 0;

				if (m_handledPdbs.find(signature) == m_handledPdbs.end())
				{
					// Load the symbols for this module.
					bool result = m_symbols->LoadSymbolsForModule(baseAddress, size, timeStamp, &signature);

					// Convert the 32-bit link time stamp to a readable format.
					tm linkTime;
					_localtime32_s( &linkTime, ( __time32_t* )&timeStamp );
					char asciiTime[26];
					asctime_s( asciiTime, _countof( asciiTime ), &linkTime );

					// Print a summary of the symbol loading, including the link time
					// for the executable.
					TCHAR msg[512];
					_stprintf_s(
						msg, sizeof(msg), "Symbols loaded %s for %s, link time %s",
						result ? "successfully" : "unsuccessfully",
						ev.name, asciiTime);

					//	IInterfaceHooks::InvokeOnMain(MissingDIA, msg);

					m_handledPdbs.insert(signature);
				}
#endif
			}

			break;
		}
	}
}

void SymbolTableBuilder::ReplayEnd(u64 position)
{
	for (std::map<TAddress, std::vector<byte> >::const_iterator it = m_hitAddressPages.begin(), itEnd = m_hitAddressPages.end();
		it != itEnd;
		++ it)
	{
		const byte* page = &*it->second.begin();

		for (int i = 0; i < PageSize; ++ i)
		{
			if (page[i >> 3] & (1 << (i & 0x7)))
				m_symbols->ImportSymbol(it->first + i);
		}
	}

	m_symbols->GetAllTypes();

	m_symbols->Save(m_filename.c_str());

	m_symbols->EndDIA();
}

void SymbolTableBuilder::ImportPS3Tools(const char* path)
{
	std::string unfselfName = path;
	std::replace(unfselfName.begin(), unfselfName.end(), '/', '\\');
	size_t sep = unfselfName.find_last_of('\\');
	if (sep != std::string::npos)
		unfselfName.resize(sep);

	unfselfName += "\\unfself.exe";

	if (GetFileAttributes(unfselfName.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		char unfselfPath[MAX_PATH];
		IInterfaceHooks::InvokeOnMain(FindUnFSelf, unfselfPath);

		if (unfselfPath[0])
			unfselfName = unfselfPath;
	}

	if (GetFileAttributes(unfselfName.c_str()) != INVALID_FILE_ATTRIBUTES)
		m_symbols->SetUnFSelf(unfselfName.c_str());

	std::string addr2lineName = path;
	std::replace(addr2lineName.begin(), addr2lineName.end(), '/', '\\');
	sep = addr2lineName.find_last_of('\\');
	if (sep != std::string::npos)
		addr2lineName.resize(sep);

	addr2lineName += "\\ppu-lv2-addr2line.exe";

	if (GetFileAttributes(addr2lineName.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		char addr2linePath[MAX_PATH];
		IInterfaceHooks::InvokeOnMain(FindAddr2Line, addr2linePath);

		if (addr2linePath[0])
			addr2lineName = addr2linePath;
	}

	if (GetFileAttributes(addr2lineName.c_str()) != INVALID_FILE_ATTRIBUTES)
		m_symbols->SetAddr2Line(addr2lineName.c_str());
}
