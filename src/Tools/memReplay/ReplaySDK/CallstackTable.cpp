#include "stdafx.h"
#include "CallstackTable.h"

void CallstackTable::ReverseAddModifiedCallstack(std::vector<TAddress> &list, TAddress start)
{
	TAddress nextAddress=m_symbols->GetNext(start);
	if (nextAddress)
		ReverseAddModifiedCallstack(list, nextAddress);
	list.push_back(start);
}

size_t CallstackTable::AddCallstack(const TAddress* cs, size_t csLen, bool modifiedCallstack)
{
	// Drop the CryMemStatAlloc/ReplayRecordAlloc parts of the callstack
	// TODO: this ought to be done via a more generic ignore list
//	if (csLen >= 2)
//		csLen -= 2;

	Callstack csItem(cs, csLen, false);

	UniqCallstackMap::iterator csit = m_uniqCallstacks.find(csItem);
	if (csit == m_uniqCallstacks.end())
	{
		size_t id = m_uniqCallstacks.size();
		UniqCallstackMap::iterator it = m_uniqCallstacks.insert(std::make_pair(Callstack(cs, csLen, true), id)).first;
		m_uniqCallstackIds.insert(std::make_pair(id, it));
		if (!modifiedCallstack && m_symbols.IsValid())
		{
			size_t newID;
			std::vector<TAddress> list;
			list.reserve(csLen);
			for (size_t i=0; i<csLen; i++)
			{
				ReverseAddModifiedCallstack(list, cs[i]);
			}
			newID=AddCallstack(&list[0], list.size(), true);
			if (newID!=id)
			{
				m_modifiedCallstacks.insert(std::make_pair(id, newID));
				return newID;
			}
		}
		return id;
	}
	else if (modifiedCallstack)
	{
		return csit->second;
	}
	else
	{
		ModifiedCallstackMap::iterator mit = m_modifiedCallstacks.find(csit->second);
		if (mit == m_modifiedCallstacks.end())
		{
			return csit->second;
		}
		else
		{
			return mit->second;
		}
	}
}

bool CallstackTable::TryFindCallstackId(const TAddress* cs, size_t csLen, size_t& idOut) const
{
	Callstack csItem(cs, csLen, false);

	UniqCallstackMap::const_iterator csit = m_uniqCallstacks.find(csItem);
	if (csit != m_uniqCallstacks.end())
	{
		idOut = csit->second;
		return true;
	}

	return false;
}

bool CallstackTable::TryFindCallstack(size_t id, TAddress* csOut, size_t& csOutCapacity) const
{
	UniqCallstackIdMap::const_iterator it = m_uniqCallstackIds.find(id);
	if (it != m_uniqCallstackIds.end())
	{
		const Callstack& cs = it->second->first;

		if (csOutCapacity >= cs.m_len)
		{
			memcpy(csOut, cs.m_cs, cs.m_len * sizeof(cs.m_cs[0]));
			csOutCapacity = cs.m_len;
			return true;
		}
		else
		{
			csOutCapacity = cs.m_len;
			return false;
		}
	}

	csOutCapacity = 0;
	return false;
}

void CallstackTable::Deserialise(const u8* data, size_t dataLen)
{
	const u8* dataEnd = data + dataLen;

	m_uniqCallstacks.clear();
	m_uniqCallstackIds.clear();
	m_modifiedCallstacks.clear();

	for (size_t index = 0; data < dataEnd; ++ index)
	{
		size_t len = *reinterpret_cast<const u32*>(data);
		data += sizeof(u32);

		UniqCallstackMap::iterator it = m_uniqCallstacks.insert(std::make_pair(
			Callstack(reinterpret_cast<const TAddress*>(data), len, true),
			index)).first;

		m_uniqCallstackIds.insert(std::make_pair(index, it));
		
		data += len * sizeof(TAddress);
	}
}

void CallstackTable::Serialise(ISerialiser& ser)
{
	ser.Write((int) Ser_Version);
	ser.Write((int) m_uniqCallstackIds.size());

	for (UniqCallstackIdMap::const_iterator it = m_uniqCallstackIds.begin(), itEnd = m_uniqCallstackIds.end();
		it != itEnd;
		++ it)
	{
		size_t len = it->second->first.m_len;
		ser.Write((int) len);
		ser.Write(it->second->first.m_cs, it->second->first.m_cs + len);
	}
}

bool CallstackTable::Deserialise(IDeserialiser& ser)
{
	if (ser.Read<int>() != Ser_Version)
		return false;

	m_uniqCallstacks.clear();
	m_uniqCallstackIds.clear();
	m_modifiedCallstacks.clear();

	size_t count = ser.Read<int>();

	std::vector<TAddress> callstack;

	for (size_t index = 0; index != count; ++ index)
	{
		int csLen = ser.Read<int>();

		if (static_cast<int>(callstack.size()) < csLen)
			callstack.resize(csLen);

		ser.Read(&callstack[0], csLen);

		UniqCallstackMap::iterator it = m_uniqCallstacks.insert(std::make_pair(
			Callstack(&callstack[0], csLen, true),
			index)).first;

		m_uniqCallstackIds.insert(std::make_pair(index, it));
	}

	return true;
}
