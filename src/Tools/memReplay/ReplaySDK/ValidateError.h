#pragma once

#include "CallstackTable.h"

struct ValidateErrorItem
{
	u64 allocEv;

	TAddress ptr;
	size_t cs;
	size_t len;
	TThreadId thread;

	ValidateErrorItem()
	{
		memset(this, 0, sizeof(*this));
	}

	ValidateErrorItem(u64 allocEv, TAddress ptr, size_t cs, size_t len, TThreadId thread)
		: allocEv(allocEv)
		, ptr(ptr)
		, cs(cs)
		, len(len)
		, thread(thread)
	{
	}
};

struct ValidateError
{
	enum ID
	{
		ERR_Unknown,

		ERR_DoubleAlloc,
		ERR_DoubleFree,
		ERR_OffsetFree,

		ERR_Count
	};

	ValidateError() : id(ERR_Unknown) {}
	ValidateError(ID id, const ValidateErrorItem& a, const ValidateErrorItem& b = ValidateErrorItem())
		: id(id)
		, a(a)
		, b(b)
	{}

	ID id;
	ValidateErrorItem a, b;
};

typedef std::vector<ValidateError> ValidateErrorVector;

struct ValidateResult
{
	ValidateErrorVector errors;
	CallstackTable callstacks;
};
