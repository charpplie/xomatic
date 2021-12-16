//////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2007.
// -------------------------------------------------------------------------
//  File name:   CryCustomTypes.h
//  Created:     2009-10-23 by Scott.
//  Description: Derived CTypeInfos for structs, enums, etc.
//               Compressed numerical types, and associated TypeInfos.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef _CRY_CUSTOM_TYPES_H
#define _CRY_CUSTOM_TYPES_H

#include "CryTypeInfo.h"





#define STATIC_CONST(T, name, val) \
	static inline T name()  { static T t = val; return t; }

	
//---------------------------------------------------------------------------
// String helper function.

template<class T>
inline bool HasString(const T& val, int flags, const void* def_data = 0)
{
	if (flags & CTypeInfo::WRITE_SKIP_DEFAULT)
	{
		if (val == (def_data ? *(const T*)def_data : T()))
			return false;
	}
	return true;
}

float NumToFromString(float val, int digits, bool floating, char buffer[], int buf_size);

template<class T>
string NumToString(T val, int min_digits, int max_digits, bool floating)
{
	char buffer[64];
	float f(val);
	for (int digits = min_digits; digits < max_digits; digits++)
	{
		if (T(NumToFromString(f, digits, floating, buffer, 64)) == val)
			break;
	}
	return buffer;
}

//---------------------------------------------------------------------------
// TypeInfo for structs

struct CStructInfo: CTypeInfo
{
	CStructInfo( cstr name, size_t size, size_t num_vars = 0, CVarInfo* vars = 0 );
	virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const;
	virtual bool FromString(void* data, cstr str, int flags = 0) const;
	virtual void SwapEndian(void* pData, size_t nCount, bool bWriting) const;
	virtual void GetMemoryUsage(ICrySizer* pSizer, void const* data) const;

	virtual const CVarInfo* NextSubVar(const CVarInfo* pPrev) const
	{
		pPrev = pPrev ? pPrev+1 : Vars.begin();
		return pPrev < Vars.end() ? pPrev : 0;
	}
	virtual const CVarInfo* FindSubVar(cstr name) const;
	virtual bool IsType( CTypeInfo const& Info ) const;

protected:
	Array<CVarInfo>	Vars;
	string					EndianDesc;					// Encodes instructions for endian swapping.
	bool						HasBitfields;

	bool FromStringParse(void* data, cstr& str, int flags) const;
	void MakeEndianDesc();
	size_t AddEndianDesc(cstr desc, size_t dim, size_t elem_size);
};

//---------------------------------------------------------------------------
// TypeInfo for enums

struct CEnumInfo: CTypeInfo
{
	CEnumInfo( cstr name, size_t size, size_t num_elems = 0, CEnumElem* elems = 0 );

	virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const;
	virtual bool FromString(void* data, cstr str, int flags = 0) const;

	virtual int EnumElemCount() const
		{ return Elems.size(); }
	virtual CEnumElem const* EnumElem(int nIndex) const 
		{ return nIndex < Elems.size() ? &Elems[nIndex] : 0; }

protected:
	Array<CEnumElem>	Elems;
	bool							bRegular;
	int								MinValue, MaxValue;
};

//---------------------------------------------------------------------------
// Template for base types, optionally with a converted storage type, using global To/FromString functions.

template<class T, class S = T>
struct TTypeInfo: CTypeInfo
{
	TTypeInfo( cstr name )
		: CTypeInfo( name, sizeof(S) )
	{}

	virtual bool IsType( CTypeInfo const& Info ) const		
		{ return &Info == this || &Info == &::TypeInfo((T*)0); }

	virtual bool ToValue(const void* data, void* value, const CTypeInfo& typeVal) const
	{
		if (&typeVal == &TypeInfo((T*)0))
			*(T*)value = *(const S*)data;
		else if (&typeVal == &TypeInfo((S*)0))
			*(S*)value = *(const S*)data;
		else
			return false;
		return true;
	}
	virtual bool FromValue(void* data, const void* value, const CTypeInfo& typeVal) const
	{
		if (&typeVal == &TypeInfo((T*)0))
			*(S*)data = *(const T*)value;
		else if (&typeVal == &TypeInfo((S*)0))
			*(S*)data = *(const S*)value;
		else
			return false;
		return true;
	}

	virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const
	{
		if (!HasString(*(const S*)data, flags, def_data))
			return string();
		T val(*(const S*)data);
		return ::ToString(val);
	}
	virtual bool FromString(void* data, cstr str, int flags = 0) const
	{
		T val;
		if (!*str || !::FromString(val, str))
		{
			if (flags & READ_SKIP_EMPTY)
				return false;
			*(S*)data = S();
		}
		else
			*(S*)data = S(val);
		return true;
	}
	virtual void GetMemoryUsage(ICrySizer* pSizer, void const* data) const
	{}
};

//---------------------------------------------------------------------------
// Customisation for string.

template<> 
inline string TTypeInfo<string>::ToString(const void* data, int flags, const void* def_data) const
{
	const string& val = *(const string*)data;
	if (def_data && (flags & CTypeInfo::WRITE_SKIP_DEFAULT))
	{
		if (val == *(const string*)def_data)
			return string();
	}
	return val;
}

template<> 
inline bool TTypeInfo<string>::FromString(void* data, cstr str, int flags) const
{
	if (!*str && (flags & READ_SKIP_EMPTY))
		return false;
	*(string*)data = str;
	return true;
}

template<> 
void TTypeInfo<string>::GetMemoryUsage(ICrySizer* pSizer, void const* data) const;

//---------------------------------------------------------------------------
//
// TypeInfo for small integer types.


#pragma warning(disable: 4554) // BOGUS WARNING: check operator precedence for possible error; use parentheses to clarify precedence

template<class T> struct TIntTraits
{
	static const bool bSIGNED
		= T(-1) < T(0);

	static const T nMIN_FACTOR
		= bSIGNED ? T(-1) : T(0);

	static const T nMIN
		= bSIGNED ? T (T(1) << T(sizeof(T)*8-1)) : T(0);

	static const T nMAX
		= ~nMIN;
};

template<class D, class S>
inline bool ConvertInt( D& dest, S src, D nMin = TIntTraits<D>::nMIN, D nMax = TIntTraits<D>::nMAX)
{
	if (src < S(nMin))
	{
		dest = nMin;
		return false;
	}
	if (src > S(nMax))
	{
		dest = nMax;
		return false;
	}
	dest = D(src);
	assert(S(dest) == src);
	return true;
}

template<class D>
inline bool ConvertInt( D& dest, const void* src, const CTypeInfo& typeSrc, D nMin = TIntTraits<D>::nMIN, D nMax = TIntTraits<D>::nMAX)
{
	if (typeSrc.IsType<int>())
	{
		switch (typeSrc.Size)
		{
			case 1: return ConvertInt(dest, *(const int8*)src, nMin, nMax);
			case 2: return ConvertInt(dest, *(const int16*)src, nMin, nMax);
			case 4: return ConvertInt(dest, *(const int32*)src, nMin, nMax);
			case 8: return ConvertInt(dest, *(const int64*)src, nMin, nMax);
		}
	}
	else if (typeSrc.IsType<uint>())
	{
		switch (typeSrc.Size)
		{
			case 1: return ConvertInt(dest, *(const uint8*)src, nMin, nMax);
			case 2: return ConvertInt(dest, *(const uint16*)src, nMin, nMax);
			case 4: return ConvertInt(dest, *(const uint32*)src, nMin, nMax);
			case 8: return ConvertInt(dest, *(const uint64*)src, nMin, nMax);
		}
	}
	return false;
}

template<class T>
struct TIntTypeInfo: TTypeInfo<T>
{
	TIntTypeInfo( cstr name )
		: TTypeInfo<T>(name)
	{}

	virtual bool IsType( CTypeInfo const& Info ) const		
		{ return &Info == this || &Info == ( TIntTraits<T>::bSIGNED ? &TypeInfo((int*)0) : &TypeInfo((uint*)0) ); }

	virtual bool GetLimits(float& fMin, float& fMax, float& fStep) const
	{
		fMin = float(TIntTraits<T>::nMIN);
		fMax = float(TIntTraits<T>::nMAX);
		fStep = 1.f;
		return true;
	}

	// Override to allow int conversion
	virtual bool FromValue(void* data, const void* value, const CTypeInfo& typeVal) const
		{ return ConvertInt(*(T*)data, value, typeVal); }
	virtual bool ToValue(const void* data, void* value, const CTypeInfo& typeVal) const
		{ return typeVal.FromValue(value, *(T*)data); }
};

//---------------------------------------------------------------------------
// Store any type, such as an enum, in a small int.

template<class T, class S = uint8>
struct TSmall
{
	typedef T				TValue;

	inline TSmall(T val = T(0))
		: m_Val(S(val))
	{
		assert(T(m_Val) == val);
	}
	inline operator T() const
		{ return T(m_Val); }
	inline T operator +() const
		{ return T(m_Val); }

	CUSTOM_STRUCT_INFO(CCustomInfo)

protected:
	S		m_Val;

	// Adaptor TypeInfo for converting between sizes.
	struct CCustomInfo: TIntTypeInfo<S>
	{
		using_type(CTypeInfo, CEnumElem)

		CCustomInfo()
			: TIntTypeInfo<S>( ValTypeInfo().Name )
		{}

		// Forward semantic functions to value TypeInfo.
		static const CTypeInfo& ValTypeInfo() 
			{ return ::TypeInfo((T*)0); }

		virtual bool IsType( CTypeInfo const& Info ) const 
			{ return ValTypeInfo().IsType(Info); }
		virtual int EnumElemCount() const
			{ return ValTypeInfo().EnumElemCount(); }
		virtual CEnumElem const* EnumElem(int nIndex) const 
			{ return ValTypeInfo().EnumElem(nIndex); }

		// Convert to/from string via intermediate full value.
		virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const
		{
			if (!HasString(*(const S*)data, flags, def_data))
				return string();
			T val = T(*(const S*)data);
			return ValTypeInfo().ToString(&val, flags & ~CTypeInfo::WRITE_SKIP_DEFAULT, def_data);
		}
		virtual bool FromString(void* data, cstr str, int flags = 0) const
		{
			T val;
			return ValTypeInfo().FromString(&val, str, flags) && ConvertInt(*(S*)data, val);
		}
	};
};

//---------------------------------------------------------------------------
// Quantise a float linearly in an int.
template<class S, int nLIMIT, S nQUANT = TIntTraits<S>::nMAX>
struct TFixed
{
	typedef float TValue;

	inline TFixed()
		: m_Store(0)
	{}

	inline TFixed(float fIn)
		{ FromFloat(this, fIn); }

	// Conversion.
	inline operator float() const
		{ return FromStore(m_Store); }
	inline float operator +() const
		{ return FromStore(m_Store); }
	inline bool operator !() const
		{ return !m_Store; }

	inline bool operator ==(const TFixed& x) const
		{ return m_Store == x.m_Store; }
	inline bool operator ==(float x) const
		{ return m_Store == TFixed(x); }

	CUSTOM_STRUCT_INFO(CCustomInfo)

protected:
	S		m_Store;

	typedef TFixed<S, nLIMIT, nQUANT> TThis;

	static const int nMAX = nLIMIT;
	static const int nMIN = TIntTraits<S>::nMIN_FACTOR * nLIMIT;

	static inline float ToStore(float f)
		{ return f * float(nQUANT) / float(nLIMIT); }
	static inline float FromStore(float f)
		{ return f * float(nLIMIT) / float(nQUANT); }

	static inline bool FromFloat(void* data, float val)
		{ return ConvertInt(*(S*)data, int_round(ToStore(val)), S(TIntTraits<S>::nMIN_FACTOR * nQUANT), nQUANT); }

	// TypeInfo implementation.
	struct CCustomInfo: TTypeInfo<float, TThis>
	{
		CCustomInfo()
			: TTypeInfo<float, TThis>("TFixed<>")
		{}

		virtual bool GetLimits(float& fMin, float& fMax, float& fStep) const
		{
			fMin = float(nMIN);
			fMax = float(nMAX);
			fStep = FromStore(1.f);
			return true;
		}

		// Override ToString: Limit to significant digits.
		virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const
		{
			if (!HasString(*(const S*)data, flags, def_data))
				return string();
			static int digits = int_ceil( log10f(float(nQUANT)) );
			return NumToString(*(const TFixed*)data, 1, digits+3, true);
		}
	};
};


// Define the canonical float-to-byte quantisation.
// We use 240 as the max rather than 255, so that common fractions are exactly represented.
typedef TFixed<uint8,1,240>	UnitFloat8;

//---------------------------------------------------------------------------
// A floating point number, with templated storage size (and sign), and number of exponent bits
template<class S, int nEXP_BITS>
struct TFloat
{
	typedef float TValue;

	TFloat()
		: m_Store(0)
	{}

	TFloat(float fIn)
		: m_Store(FromFloat(fIn))
	{}

	TFloat& operator =(float fIn)
	{
		m_Store = FromFloat(fIn);
		return *this;
	}

	inline operator float() const
		{ return ToFloat(m_Store); }
	inline float operator +() const
		{ return ToFloat(m_Store); }

	inline bool operator !() const
		{ return !m_Store; }

	inline bool operator ==(TFloat x) const
		{ return m_Store == x.m_Store; }
	inline bool operator ==(float x) const
		{ return float(*this) == x; }

	inline TFloat& operator *=(float x)
		{ return *this = *this * x; }

	STATIC_CONST(float, fMAX, ToFloat(TIntTraits<S>::nMAX));
	STATIC_CONST(float, fPOS_MIN, ToFloat(1 << nMANT_BITS));
	STATIC_CONST(float, fMIN, -fMAX() * (float)TIntTraits<S>::bSIGNED);

	CUSTOM_STRUCT_INFO(CCustomInfo)

protected:
	S		m_Store;

	typedef TFloat<S,nEXP_BITS> TThis;

	static const S nBITS = sizeof(S)*8;
	static const S nSIGN = TIntTraits<S>::bSIGNED;
	static const S nMANT_BITS = nBITS - nEXP_BITS - nSIGN;
	static const S nSIGN_MASK = S(nSIGN << (nBITS-1));
	static const S nMANT_MASK = (S(1) << nMANT_BITS) - 1;
	static const S nEXP_MASK = ~S(nMANT_MASK | nSIGN_MASK);
	static const int nEXP_MAX = 1 << (nEXP_BITS-1),
	                 nEXP_MIN = 1 - nEXP_MAX;

	STATIC_CONST(float, fROUNDER, 1.f + fPOS_MIN()*0.5f);

	static inline S FromFloat(float fIn)
	{
		COMPILE_TIME_ASSERT(sizeof(S) <= 4);
		COMPILE_TIME_ASSERT(nEXP_BITS > 0 && nEXP_BITS <= 8 && nEXP_BITS < sizeof(S)*8-4);

		// Clamp to allowed range.
		float fClamped = clamp_tpl(fIn * fROUNDER(), fMIN(), fMAX());

		// Bit shift to convert from IEEE float32.
		uint32 uBits = *(const uint32*)&fClamped;
		
		// Convert exp.
    int32 iExp = (uBits >> 23) & 0xFF;
		iExp -= 127 + nEXP_MIN;
		if (iExp < 0)
			// Underflow.
			return 0;

		// Reduce mantissa.
		uint32 uMant = uBits >> (23 - nMANT_BITS);

		S bits = (uMant & nMANT_MASK)
					 | (iExp << nMANT_BITS)
					 | ((uBits >> (32-nBITS)) & nSIGN_MASK);

		#ifdef _DEBUG
			fIn = clamp_tpl(fIn, fMIN(), fMAX());
			float fErr = fabs(ToFloat(bits) - fIn);
			float fMaxErr = fabs(fIn) / float(1<<nMANT_BITS);
			assert(fErr <= fMaxErr);
		#endif

		return bits;
	}

	static inline float ToFloat(S bits)
	{
		if (bits == 0)
			return 0.f;

		// Extract FP components.
		uint32 uBits = bits & nMANT_MASK,
			     uExp = (bits & ~nSIGN_MASK) >> nMANT_BITS,
			     uSign = bits & nSIGN_MASK;

		// Shift to 32-bit.
		uBits <<= 23 - nMANT_BITS;
		uBits |= (uExp + 127 + nEXP_MIN) << 23;
		uBits |= uSign << (32-nBITS);
		return *(float*)&uBits;
	}

	// TypeInfo implementation.
	struct CCustomInfo: TTypeInfo<float, TThis>
	{
		CCustomInfo()
			: TTypeInfo<float, TThis>("TFloat<>") 
		{}

		virtual bool GetLimits(float& fMin, float& fMax, float& fStep) const
		{
			fMax = fMAX();
			fMin = fMIN();
			fStep = fPOS_MIN();
			return true;
		}

		// Override ToString: Limit to significant digits.
		virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const
		{
			if (!HasString(*(const S*)data, flags, def_data))
				return string();
			static int digits = int_ceil( log10f(1<<nMANT_BITS) );
			return NumToString(*(const TFloat*)data, 1, digits+3, true);
		}
	};
};

// Canonical float16 types, with range ~= 64K.
typedef TFloat<int16,5>		SFloat16;
typedef TFloat<uint16,5>	UFloat16;

#endif
