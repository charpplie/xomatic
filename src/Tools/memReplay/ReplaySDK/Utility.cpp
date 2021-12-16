#include "stdafx.h"
#include "Utility.h"

#include <locale.h>
#include <limits>

namespace
{
	template <typename T, bool> struct Negate {};
	template <typename T> struct Negate<T, true> { T operator () (T v) const { return -v; } };
	template <typename T> struct Negate<T, false> { T operator () (T v) const { return v; } };
}

template <typename T>
char* FormatThousands(char* out, int outCapacity, T input)
{
	static int comma = '\0';

	if (comma == '\0')
	{
		struct lconv *lcp = localeconv();
		comma = (lcp && lcp->thousands_sep && *lcp->thousands_sep)
			? *lcp->thousands_sep
			: ',';
	}

	char *p = &out[outCapacity - 1];
	*p = '\0';

	bool neg = input < 0;
	if (neg)
		input = Negate<T, std::numeric_limits<T>::is_signed>()(input);

	int i = 0;
	do
	{
		if((i % 3) == 0 && i)
			*--p = comma;
		*--p = '0' + input % 10;
		input /= 10;
		++ i;
	}
	while (input);

	if (neg)
		*--p = '-';

	return p;
}

template char* FormatThousands<int>(char*, int, int);
template char* FormatThousands<size_t>(char*, int, size_t);
template char* FormatThousands<u64>(char*, int, u64);
template char* FormatThousands<s64>(char*, int, s64);
