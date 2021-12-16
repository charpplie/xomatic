#ifndef UTILITY_H
#define UTILITY_H

template <typename T> struct StripReference { };
template <typename T> struct StripReference<T&> { typedef T value_type; };

template <typename T> struct IsPod { enum { Result = 0 }; };
template <> struct IsPod<bool> { enum { Result = 1 }; };
template <> struct IsPod<s8> { enum { Result = 1 }; };
template <> struct IsPod<u8> { enum { Result = 1 }; };
template <> struct IsPod<s16> { enum { Result = 1 }; };
template <> struct IsPod<u16> { enum { Result = 1 }; };
template <> struct IsPod<s32> { enum { Result = 1 }; };
template <> struct IsPod<u32> { enum { Result = 1 }; };
template <> struct IsPod<s64> { enum { Result = 1 }; };
template <> struct IsPod<u64> { enum { Result = 1 }; };
template <typename T> struct IsPod<T*> { enum { Result = 1 }; };
template <typename T> struct IsPod<const T> { enum { Result = IsPod<T>::Result }; };

inline void SwapEndian(u16& ui16)
{
	ui16 = (ui16 << 8) | (ui16 >> 8);
}

inline void SwapEndian(u32& ui32)
{
	ui32 = (ui32 >> 24)|(ui32 << 24)|((ui32 & 0xff0000) >> 8) | ((ui32 & 0xff00) << 8);
}

inline void SwapEndian(s32& si32)
{
	SwapEndian(reinterpret_cast<u32&>(si32));
}

inline void SwapEndian(u64& ui64)
{
	ui64 = ((ui64 & 0xffffffff00000000) >> 32) | ((ui64 & 0x00000000ffffffff) << 32);
	ui64 = ((ui64 & 0xffff0000ffff0000) >> 16) | ((ui64 & 0x0000ffff0000ffff) << 16);
	ui64 = ((ui64 & 0xff00ff00ff00ff00) >> 8) | ((ui64 & 0x00ff00ff00ff00ff) << 8);
}

inline void FormatSize(char* out, int outSize, s64 size)
{
	if (size >= 0)
	{
		if (size >= 1024 * 1024)
			sprintf_s(out, outSize, "%.1fM", static_cast<double>(size) / (1024.0f * 1024.0f));
		else if(size >= 1024)
			sprintf_s(out, outSize, "%iK", (size + 512) / 1024);
		else
			sprintf_s(out, outSize, "%i", size);
	}
	else
	{
		if (size <= -1024 * 1024)
			sprintf_s(out, outSize, "%.1fM", static_cast<double>(size) / (1024.0f * 1024.0f));
		else if(size <= -1024)
			sprintf_s(out, outSize, "%iK", (size - 512) / 1024);
		else
			sprintf_s(out, outSize, "%i", size);
	}
}

template <typename T>
char* FormatThousands(char* out, int outCapacity, T input);

static u32 HashString(const char* key)
{
	u32 h=0;
	const u8 *pKey=(const u8*)key;
	while (pKey[0])
	{
		h+=pKey[0];
		h+=(h<<10);
		h^=(h>>6);
		pKey++;
	}
	h+=(h<<3);
	h^=(h>>11);
	h+=(h<<15);
	return h;
}

template <typename T, typename LessFunc, typename NextFunc>
void SortLL(T*& root)
{
	if (!root)
		return;

	int k = 1;
	T* p;

	LessFunc less;
	NextFunc next;

	for (int k = 1; ; k <<= 1)
	{
		T* nl = root;
		T** c = &root;

		int mrgr = 0;

		do
		{
			T* a = nl;
			T* b = nl;

			for (int i = k; i && b; -- i, p = b, b = next(*b))
				;
			next(*p) = NULL;

			nl = b;

			for (int i = k; i && nl; -- i, p = nl, nl = next(*nl))
				;
			next(*p) = NULL;

			while (a && b)
			{
				if (less(*a, *b))
				{
					*c = a;
					a = next(*a);
				}
				else
				{
					*c = b;
					b = next(*b);
				}

				c = &next(**c);
			}

			if (a)
			{
				*c = a;

				for (p = a; next(*p); p = next(*p))
					;
				c = &next(*p);
			}
			else if(b)
			{
				*c = b;

				for (p = b; next(*p); p = next(*p))
					;
				c = &next(*p);
			}

			++ mrgr;
		}
		while (nl);

		if (mrgr == 1)
			break;
	}
}

template <typename T>
inline T nlz(T x)
{
	T y = x, n = 0;

	for (;;)
	{
		if (x < 0) return n;
		if (y == 0) return sizeof(T) * 8 - n;
		++ n;
		x <<= 1;
		y >>= 1;
	}
}

template <typename T>
inline T ilog2(T n)
{
	return 31 - nlz(n);
}

template <typename T>
inline bool within(T a, T d, T b)
{
	return std::abs(a - b) <= d;
}

inline float lerp(float a, float b, float t)
{
	return a * (1.0f - t) + b * t;
}

template <typename T>
inline T Clamp(T min, T max, T value)
{
	return std::max(min, std::min(max, value));
}

inline const char* strstri(const char* inStr, const char* forStr)
{
	char* inStrUpper = (char*) alloca(strlen(inStr) + 1);
	strcpy(inStrUpper, inStr);
	_strupr(inStrUpper);

	char* forStrUpper = (char*) alloca(strlen(forStr) + 1);
	strcpy(forStrUpper, forStr);
	_strupr(forStrUpper);

	char* res = strstr(inStrUpper, forStrUpper);
	if (!res)
		return res;

	return inStr + (res - inStrUpper);
}

#endif
