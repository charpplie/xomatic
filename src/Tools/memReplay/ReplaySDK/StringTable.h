#pragma once

struct string_equal
{
	bool operator () (const char* a, const char* b) const
	{
		return strcmp(a, b) == 0;
	}
};

struct string_less
{
	bool operator () (const char* a, const char* b) const
	{
		return strcmp(a, b) < 0;
	}
};

class StringTable
{
public:
	StringTable();
	~StringTable();

	void Clear();

	const char* InternString(const char* str);

private:
	typedef std::map<const char*, const char*, string_less> Table;

private:
	Table m_strings;
};
