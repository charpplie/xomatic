#pragma once


class CryMemBlock;

//namespace sw{

struct TAny
{
	enum EType
	{
		T_int8		,
		T_int16		,
		T_int32		,
		T_int64		,
		T_uint8		,
		T_uint16	,
		T_uint32	,
		T_uint64	,
		T_float		,
		T_double	,
		T_SZ		,
		T_MEM		,
		T_COUNT		,
		T_UNSPECIFIED = -1,
	}vtype;

	template <typename T> struct Type2Enum{ enum{result = T_UNSPECIFIED,}; };
#define TAny_Type2Enum_ENTRY(type) \
	template <> struct Type2Enum<type>{ enum{result = T_##type,}; };
	TAny_Type2Enum_ENTRY(int8	)
	TAny_Type2Enum_ENTRY(int16	)
	TAny_Type2Enum_ENTRY(int32	)
	TAny_Type2Enum_ENTRY(int64	)
	TAny_Type2Enum_ENTRY(uint8	)
	TAny_Type2Enum_ENTRY(uint16	)
	TAny_Type2Enum_ENTRY(uint32	)
	TAny_Type2Enum_ENTRY(uint64	)
	TAny_Type2Enum_ENTRY(float	)
	TAny_Type2Enum_ENTRY(double	)
	template<> struct Type2Enum<char*>{ enum{result = T_SZ,}; };
	template<> struct Type2Enum<CryMemBlock>{ enum{result = T_MEM,}; };

#define TAny_union_ENTRY(type)  type type##Val;
	union{
		TAny_union_ENTRY(int8	)
		TAny_union_ENTRY(int16	)
		TAny_union_ENTRY(int32	)
		TAny_union_ENTRY(int64	)
		TAny_union_ENTRY(uint8	)
		TAny_union_ENTRY(uint16	)
		TAny_union_ENTRY(uint32	)
		TAny_union_ENTRY(uint64	)
		TAny_union_ENTRY(float	)
		TAny_union_ENTRY(double	)
		char* szVal;
		struct { size_t size; byte* ptr; } memVal;
	};

#define TAny_constructor_ENTRY(type) \
	TAny(type a_val):vtype(T_##type),type##Val(a_val){}

	TAny_constructor_ENTRY(int8		)
	TAny_constructor_ENTRY(int16	)
	TAny_constructor_ENTRY(int32	)
	TAny_constructor_ENTRY(int64	)
	TAny_constructor_ENTRY(uint8	)
	TAny_constructor_ENTRY(uint16	)
	TAny_constructor_ENTRY(uint32	)
	TAny_constructor_ENTRY(uint64	)
	TAny_constructor_ENTRY(float	)
	TAny_constructor_ENTRY(double	)
	TAny(char* a_val):vtype(T_SZ),szVal(a_val){}
	TAny(const char* a_val):vtype(T_SZ),szVal(const_cast<char*>(a_val)){}
	TAny(byte* ptr,size_t size):vtype(T_MEM) { memVal.ptr = ptr; memVal.size = size; }
	TAny(const byte* ptr,size_t size):vtype(T_MEM) { memVal.ptr = const_cast<byte*>(ptr); memVal.size = size; }
	TAny():vtype(T_UNSPECIFIED){}
private:
	TAny(void*); //< prevent improper auto converting 


public:
	int SPrintf(char* szOut, char* pEnd) const;

	template<typename T>
	bool SafeGet(T& val){ if (Type2Enum<T>::result != vtype) return false;  val = *reinterpret_cast<T*>(&szVal); return true; }
	template<>
	bool SafeGet(CryMemBlock& val){ CRY_ASSERT_MESSAGE(0,"not implemented");};

	bool IsOK()const { return ((unsigned)vtype) < T_COUNT; }

	void *GetBaseAddr() const { return (void *)&szVal; }
	size_t GetSize()
	{
		switch(vtype)
		{
			case T_int8:
			case T_uint8:
				return sizeof(char);

			case T_int16:
			case T_uint16:
				return sizeof(short);

			case T_int32:
			case T_uint32:
			case T_float:
				return sizeof(int);

			case T_int64:
			case T_uint64:
			case T_double:
				return sizeof(__int64);

			case T_MEM:
				return memVal.size;

			default:
				return 0;
		}
	}

#undef TAny_Type2Enum_ENTRY
#undef TAny_union_ENTRY
#undef TAny_constructor_ENTRY
#undef TAny_sprintf_ENTRY
};

static const TAny ANY_NULL;

typedef std::vector<TAny> TArgStack;


//}// namespace sw