#pragma once

#include "ReplayLogDefs.h"
#include "SymbolHelper.h"

struct Callstack
{
	Callstack()
		: m_cs(NULL)
		, m_ownsCS(0)
	{
	}

	Callstack(const TAddress* cs, size_t len, bool clone)
	{
		if (clone)
		{
			TAddress* myCS = new TAddress[len];
			std::copy(cs, cs + len, myCS);

			m_cs = myCS;
			m_len = len;
			m_ownsCS = 1;
		}
		else
		{
			m_cs = cs;
			m_len = len;
			m_ownsCS = 0;
		}
	}

	~Callstack()
	{
		if (m_ownsCS)
			delete [] m_cs;
	}

	Callstack(const Callstack& cs)
	{
		if (cs.m_ownsCS)
		{
			TAddress* myCS = new TAddress[cs.m_len];
			std::copy(cs.m_cs, cs.m_cs + cs.m_len, myCS);

			m_cs = myCS;
			m_len = cs.m_len;
			m_ownsCS = cs.m_ownsCS;
		}
		else
		{
			memcpy(this, &cs, sizeof(cs));
		}
	}

	Callstack& operator = (const Callstack& cs)
	{
		this->~Callstack();
		new (this) Callstack(cs);

		return *this;
	}

	friend bool operator < (const Callstack& a, const Callstack& b)
	{
		if (a.m_len != b.m_len) return a.m_len < b.m_len;
		return memcmp(a.m_cs, b.m_cs, sizeof(u32) * a.m_len) < 0;
	}

	const TAddress* m_cs;
	u32 m_len : 31;
	u32 m_ownsCS : 1;
};

class CallstackTable
{
public:
	CallstackTable()
	{
		m_symbols=NULL;
	}
	CallstackTable(SharedPtr<SymbolHelper> symbols)
	{
		m_symbols=symbols;
	}

	size_t AddCallstack(const ReplayAlloc3Event& ev)
	{
		return AddCallstack((const TAddress*) ev.callstack, ev.callstackLength);
	}

	size_t AddCallstack(const ReplayFree4Event& ev)
	{
		return AddCallstack((const TAddress*) ev.callstack, ev.callstackLength);
	}

	size_t AddCallstack(const TAddress* cs, size_t csLen, bool bModifiedCallstack=false);

	bool TryFindCallstackId(const TAddress* cs, size_t csLen, size_t& idOut) const;
	bool TryFindCallstack(size_t id, TAddress* csOut, size_t& csOutCapacity) const;

	void Deserialise(const u8* data, size_t dataLen);

	void Serialise(ISerialiser& ser);
	bool Deserialise(IDeserialiser& ser);

private:
	typedef std::map<Callstack, size_t> UniqCallstackMap;
	typedef std::map<size_t, UniqCallstackMap::iterator> UniqCallstackIdMap;
	typedef std::map<size_t, size_t> ModifiedCallstackMap;

	enum
	{
		Ser_Version = 1,
	};
	
	void CallstackTable::ReverseAddModifiedCallstack(std::vector<TAddress> &list, TAddress start);

private:
	UniqCallstackMap m_uniqCallstacks;
	UniqCallstackIdMap m_uniqCallstackIds;
	ModifiedCallstackMap m_modifiedCallstacks;
	SharedPtr<SymbolHelper> m_symbols;
};
