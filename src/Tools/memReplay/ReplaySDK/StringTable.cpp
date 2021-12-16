#include "stdafx.h"
#include "StringTable.h"

StringTable::StringTable()
{
}

StringTable::~StringTable()
{
	Clear();
}

void StringTable::Clear()
{
	for (Table::iterator it = m_strings.begin(), itEnd = m_strings.end(); it != itEnd; ++ it)
	{
		free((void*) it->second);
	}
	m_strings.clear();
}

const char* StringTable::InternString(const char* str)
{
	Table::iterator it = m_strings.find(str);
	if (it != m_strings.end())
	{
		return it->second;
	}

	const char* dup = _strdup(str);
	return m_strings.insert(std::make_pair(dup, dup)).first->second;
}
