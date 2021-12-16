#pragma once

#include "ReplayLogReader.h"
#include "SymbolHelper.h"

#ifdef ENABLE_DIA
struct DM_PDB_SIGNATURE_less
{
	bool operator () (const DM_PDB_SIGNATURE& a, const DM_PDB_SIGNATURE& b) const
	{
		return memcmp(&a, &b, sizeof(DM_PDB_SIGNATURE)) < 0;
	}
};
#endif

class SymbolTableBuilder : public IReplayListener
{
public:
	SymbolTableBuilder(const std::string& filename);

	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

	SharedPtr<SymbolHelper> GetSymbols() { return m_symbols; }

private:
	static const TAddress PageSize = 128 * 1024 * 1024;
	static const TAddress AddrMask = PageSize-1;
	static const TAddress PageMask = ~AddrMask;

private:
	std::vector<byte>& GetPage(TAddress address)
	{
		std::map<TAddress, std::vector<byte> >::iterator it = m_hitAddressPages.find(address & PageMask);
		if (it != m_hitAddressPages.end())
		{
			return it->second;
		}

		it = m_hitAddressPages.insert(std::make_pair(address & PageMask, std::vector<byte>())).first;
		it->second.resize(PageSize / 8);
		return it->second;
	}

	void ImportPS3Tools(const char* path);

private:
	std::string m_filename;

	SharedPtr<SymbolHelper> m_symbols;
	std::map<TAddress, std::vector<byte> > m_hitAddressPages;

	bool m_handledSelf;
#ifdef ENABLE_DIA
	std::set<DM_PDB_SIGNATURE, DM_PDB_SIGNATURE_less> m_handledPdbs;
#endif
};
