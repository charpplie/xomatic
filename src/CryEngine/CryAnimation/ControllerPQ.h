////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   controllerpq.h
//  Version:     v1.00
//  Created:     4/07/2006 by Alexey.
//  Compilers:   Visual Studio.NET 2005
//  Description: Quat\pos controller implementation.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef _CRYTEK_CONTROLLER_PQ_
#define _CRYTEK_CONTROLLER_PQ_

#include "CGFContent.h"
#include "QuatQuantization.h"
#include "wavelets/Compression.h"


typedef Quat TRotation;
typedef Vec3 TPosition;
typedef Vec3 TScale;




class IPositionController;
class IRotationController;
class IKeyTimesInformation;
class ITrackPositionStorage;
class ITrackRotationStorage;
class IControllerOpt;

namespace ControllerHelper
{

	uint32 GetPositionsFormatSizeOf(uint32 format);
	uint32 GetRotationFormatSizeOf(uint32 format);
	uint32 GetKeyTimesFormatSizeOf(uint32 format);

	ITrackPositionStorage * GetPositionControllerPtr(uint32 format);
	ITrackRotationStorage * GetRotationControllerPtr(uint32 format);
	IKeyTimesInformation * GetKeyTimesControllerPtr(uint32 format);

	// Create array of controllers
	ITrackPositionStorage * GetPositionControllerPtrArray(uint32 format, uint32 size);
	ITrackRotationStorage * GetRotationControllerPtrArray(uint32 format, uint32 size);
	IKeyTimesInformation * GetKeyTimesControllerPtrArray(uint32 format, uint32 size);

	ITrackRotationStorage * GetRotationPtrFromArray(void * ptr,uint32 format, uint32 num);
	IKeyTimesInformation * GetKeyTimesPtrFromArray(void * ptr, uint32 format, uint32 num);
	ITrackPositionStorage * GetPositionPtrFromArray(void * ptr,uint32 format, uint32 num);
	// delete array of controllers
	void DeletePositionControllerPtrArray(ITrackPositionStorage *);
	void DeleteRotationControllerPtrArray(ITrackRotationStorage *);
	void DeleteTimesControllerPtrArray(IKeyTimesInformation *);

	IControllerOpt * CreateController(int iRotKeyType, int iRotType, int iPosKeyType, int iPosType);

	extern SPU_LOCAL const byte m_byteTable[256];
	extern SPU_LOCAL const byte m_byteLowBit[256];
	extern SPU_LOCAL const byte m_byteHighBit[256];
}

// don't add values which incrase the maximum enum value over 255 because this will break IControllerOptNonVirtual!!!!
enum EKeyTimesFormat {
	eF32,
	eUINT16,
	eByte,
	eF32StartStop,
	eUINT16StartStop,
	eByteStartStop,
	eBitset,
	eNoFormat = 255

	//eF32Bitset,
	//eUINT16Bitset,
	//eByteBitset,
};

struct F32Encoder 
{
	typedef f32 TKeyTypeTraits;
	typedef uint32 TKeyTypeCountTraits;

	static EKeyTimesFormat GetFormat() { return eF32; };
};

struct UINT16Encoder
{
	typedef uint16 TKeyTypeTraits;
	typedef uint16 TKeyTypeCountTraits;

	static EKeyTimesFormat GetFormat() { return eUINT16; };
};

struct ByteEncoder
{
	typedef byte TKeyTypeTraits;
	typedef uint16 TKeyTypeCountTraits;

	static EKeyTimesFormat GetFormat() { return eByte; };
};

struct F32StartStopEncoder 
{
	static EKeyTimesFormat GetFormat() { return eF32StartStop; };
};

struct UINT16StartStopEncoder
{
	static EKeyTimesFormat GetFormat() { return eUINT16StartStop; };
};

struct ByteStartStopEncoder
{
	static EKeyTimesFormat GetFormat() { return eByteStartStop; };
};

struct BitsetEncoder 
{
	typedef uint16 TKeyTypeTraits;
	typedef uint16 TKeyTypeCountTraits;

	static EKeyTimesFormat GetFormat() { return eBitset; };
};

class IKeyTimesInformation : public _reference_target_t
{

public:
	virtual ~IKeyTimesInformation() {;};
	//	virtual TKeyTime GetKeyValue(uint32 key) const = 0;
	// return key value in f32 format
	virtual f32 GetKeyValueFloat(uint32 key) const  = 0;
	// return key number for normalized_time
	virtual uint32	GetKey( f32 normalized_time, f32& difference_time) = 0;

	virtual uint32 GetNumKeys() const = 0;

	virtual void AddKeyTime(f32 val) = 0;

	virtual void ReserveKeyTime(uint32 count) = 0;

	virtual void ResizeKeyTime(uint32 count) = 0;

	virtual uint32 GetFormat() = 0;

	virtual char * GetData() = 0;

	virtual uint32 GetDataRawSize() = 0;

	//virtual void AddRef() = 0;

	//virtual void Release() = 0;

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const =0;
};

template <class TKeyTime, class _Encoder>
class CKeyTimesInformation  : public IKeyTimesInformation
{

public:
	CKeyTimesInformation() : m_lastTime(-1) {};

	//__declspec(noinline) ~CKeyTimesInformation()
	//{
	//	m_arrKeys.clear();
	//}
	// Return decoded keytime value from array. Useful for compression 
	//	inline f32 DecodeKeyValue(TKeyTime key) const { return key; } // insert decompression code here. Example (f32)m_arrKeys[key]/60 - keytimes stores in short int format
	//Return encoded value for comparison in getKey
	//	inline TKeyTime EncodeKeyValue(f32 val) const { return val; }; // insert compression code here. Example (TKeyTime)(val * 60) - keytimes stores in short int format

	// return key value
	TKeyTime GetKeyValue(uint32 key) const { return m_arrKeys[key]; };

	// return key value in f32 format
	f32 GetKeyValueFloat(uint32 key) const { return /*_Encoder::DecodeKeyValue*/(m_arrKeys[key]); }

	// return key number for normalized_time
	SPU_INDIRECT(CommandBufferExecute(ML))
		SPU_NO_INLINE uint32 GetKey( f32 normalized_time, f32& difference_time)
	{

		f32 realtime = normalized_time; 

		/*
		if (realtime == m_lastTime)
		{
			difference_time =  m_LastDifferenceTime;
			return m_lastKey;
		}*/

		m_lastTime = realtime;


		uint32 numKey = m_arrKeys.size();

		TKeyTime keytime_start = m_arrKeys[0];
		TKeyTime keytime_end = m_arrKeys[numKey-1];

		f32 test_end = keytime_end;
		if( realtime < keytime_start )
			test_end += realtime;

		if( realtime < keytime_start )
		{
			m_lastKey = 0;
			return 0;
		}

		if( realtime >= keytime_end )
		{
			m_lastKey = numKey;
			return numKey;
		}


		int nPos  = numKey>>1;
		int nStep = numKey>>2;

		// use binary search
		//TODO: Need check efficiency of []operator. Maybe wise use pointer
		while(nStep)
		{
			if(realtime < m_arrKeys[nPos])
				nPos = nPos - nStep;
			else
				if(realtime > m_arrKeys[nPos])
					nPos = nPos + nStep;
				else 
					break;

			nStep = nStep>>1;
		}

		// fine-tuning needed since time is not linear
		while(realtime > m_arrKeys[nPos])
			nPos++;

		while(realtime < m_arrKeys[nPos-1])
			nPos--;

		m_lastKey = nPos;

		// possible error if encoder uses nonlinear methods!!!
		m_LastDifferenceTime = difference_time = /*_Encoder::DecodeKeyValue*/(f32)(realtime-(f32)m_arrKeys[nPos-1]) / ((f32)m_arrKeys[nPos] - (f32)m_arrKeys[nPos-1]);

		return nPos;

	}

	SPU_INDIRECT(CommandBufferExecute(M))
		SPU_NO_INLINE uint32 GetNumKeys() const {return m_arrKeys.size(); };

	void AddKeyTime(f32 val)
	{
		m_arrKeys.push_back(/*_Encoder::EncodeKeyValue*/(TKeyTime)(val));
	};

	void ReserveKeyTime(uint32 count)
	{

		m_arrKeys.reserve(count);
	};

	void ResizeKeyTime(uint32 count)
	{

		m_arrKeys.resize(count);
	};

	uint32 GetFormat()
	{
		return _Encoder::GetFormat();
	}

	char * GetData()
	{
		return (char*) m_arrKeys.begin();
	}

	uint32 GetDataRawSize()
	{
		return (uint32)((char*)m_arrKeys.end() - (char*)m_arrKeys.begin());
	}

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_arrKeys);
	}
private:
	DynArray<TKeyTime> m_arrKeys;
	f32 m_lastTime;
	f32 m_LastDifferenceTime;
	uint32 m_lastKey;

};

template <class TKeyTime, class _Encoder>
class CKeyTimesDeleteable  : public CKeyTimesInformation<TKeyTime, _Encoder>
{
public:
	CKeyTimesDeleteable() : m_nRefCounter(0) {}

	virtual void AddRef()
	{
		++m_nRefCounter;
	}

	virtual void Release()
	{
		if (--m_nRefCounter <= 0)
			delete this;
	}
protected:
	uint32 m_nRefCounter;
};

template <class TKeyTime, class _Encoder>
class CKeyTimesDeleteableInplace  : public CKeyTimesInformation<TKeyTime, _Encoder>
{
public:
	virtual void Release()
	{
		//if (--m_nRefCounter <= 0)
		//	this->~CKeyTimesDeleteableInplace();
	}

	virtual void AddRef()
	{
	}

};


typedef _smart_ptr<IKeyTimesInformation>/* IKeyTimesInformation * */KeyTimesInformationPtr;

//typedef CKeyTimesDeleteable<f32, F32Encoder> F32KeyTimesInformation;
//typedef CKeyTimesDeleteable<uint16, UINT16Encoder> UINT16KeyTimesInformation;
//typedef CKeyTimesDeleteable<byte, ByteEncoder> ByteKeyTimesInformation;
//
//typedef CKeyTimesDeleteableInplace<f32, F32Encoder> F32KeyTimesInformationInplace;
//typedef CKeyTimesDeleteableInplace<uint16, UINT16Encoder> UINT16KeyTimesInformationInplace;
//typedef CKeyTimesDeleteableInplace<byte, ByteEncoder> ByteKeyTimesInformationInplace;


typedef CKeyTimesInformation<f32, F32Encoder> F32KeyTimesInformation;
typedef CKeyTimesInformation<uint16, UINT16Encoder> UINT16KeyTimesInformation;
typedef CKeyTimesInformation<byte, ByteEncoder> ByteKeyTimesInformation;

typedef CKeyTimesInformation<f32, F32Encoder> F32KeyTimesInformationInplace;
typedef CKeyTimesInformation<uint16, UINT16Encoder> UINT16KeyTimesInformationInplace;
typedef CKeyTimesInformation<byte, ByteEncoder> ByteKeyTimesInformationInplace;


template <class TKeyTime, class _Encoder>
class CKeyTimesInformationStartStop  : public IKeyTimesInformation
{

public:
	CKeyTimesInformationStartStop() {m_arrKeys[0] = (TKeyTime)0; m_arrKeys[1] = (TKeyTime)0;};

	// return key value in f32 format
	f32 GetKeyValueFloat(uint32 key) const { return /*_Encoder::DecodeKeyValue*/(float)(m_arrKeys[0] + key); }

	// return key number for normalized_time
	uint32	GetKey( f32 normalized_time, f32& difference_time)
	{

		f32 realtime = normalized_time;

		if (realtime < m_arrKeys[0])
			return 0;

		if (realtime > m_arrKeys[1])
			return (uint32)(m_arrKeys[1] - m_arrKeys[0]);

		uint32 nKey = (uint32)realtime;
		difference_time = realtime - (float)(nKey);

		return nKey;
	}

	uint32 GetNumKeys() const {return (uint32)(m_arrKeys[1] - m_arrKeys[0]); };

	virtual void Clear()
	{
		//m_arrKeys.clear();
	}

	void AddKeyTime(f32 val)
	{
		//m_arrKeys.push_back(/*_Encoder::EncodeKeyValue*/(TKeyTime)(val));
		if (m_arrKeys[0] > val)
			m_arrKeys[0] = (TKeyTime)val;

		if (m_arrKeys[1] < val)
			m_arrKeys[1] = (TKeyTime)val;
	};

	void ReserveKeyTime(uint32 count)
	{

		//m_arrKeys.reserve(count);
	};

	void ResizeKeyTime(uint32 count)
	{

		//		m_arrKeys.resize(count);
	};

	uint32 GetFormat()
	{
		return _Encoder::GetFormat();
	}

	char * GetData()
	{
		return (char *)(&m_arrKeys[0]);
		//return (char*) m_arrKeys.begin();
	}

	uint32 GetDataRawSize()
	{
		return sizeof(TKeyTime) * 2;
		//;/return (uint32)((char*)m_arrKeys.end() - (char*)m_arrKeys.begin());
	}

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	//DynArray<TKeyTime> m_arrKeys;
	TKeyTime m_arrKeys[2]; // start pos, stop pos.

};

typedef CKeyTimesInformationStartStop<f32, F32StartStopEncoder> F32SSKeyTimesInformation;
typedef CKeyTimesInformationStartStop<uint16, UINT16StartStopEncoder> UINT16SSKeyTimesInformation;
typedef CKeyTimesInformationStartStop<byte, ByteStartStopEncoder> ByteSSKeyTimesInformation;

typedef CKeyTimesInformationStartStop<f32, F32StartStopEncoder> F32SSKeyTimesInformationInplace;
typedef CKeyTimesInformationStartStop<uint16, UINT16StartStopEncoder> UINT16SSKeyTimesInformationInplace;
typedef CKeyTimesInformationStartStop<byte, ByteStartStopEncoder> ByteSSKeyTimesInformationInplace;



inline uint8 GetFirstLowBitInternal(uint8 word)
{
	uint8 c(0);

	if (word & 1)
		return 0;

	uint8 b;
	do 
	{
		word = word >> 1;
		b = word & 1;
		++c;
	} while(!b && c < 8);

	return c; 
}

inline uint8 GetFirstHighBitInternal(uint8 word)
{

	uint8 c(0);

	if (word & 0x80)
		return 7;

	uint8 b;
	do 
	{
		word = word << 1;
		b = word & 0x80;
		++c;
	} while(!b && c < 8);

	if (c == 8)
		return c;
	else
		return 7-c; 
}


//#ifdef NEED_ENDIAN_SWAP
//inline uint16 GetFirstLowBit(uint16 word)
//{
//
//	uint16 res = (ControllerHelper::m_byteLowBit[word && 8]);
//	if ( res == 8) {
//		res = (ControllerHelper::m_byteLowBit[(word >> 8) && 0xFF] + 8);
//	}
//
//	return res;
//}
//
//inline uint16 GetFirstHighBit(uint16 word)
//{
//	uint16 res = (ControllerHelper::m_byteHighBit[word >> 8] + 8) % 16;
//	if ( res == 16) {
//		res = ControllerHelper::m_byteHighBit[word && 0xFF];
//	}
//
//	return res;
//}
//#else

//#endif


#ifdef DO_ASM 

inline uint16 GetFirstLowBit(uint16 _word)
{
	uint16 res = 16;

	__asm
	{
		mov ax, res
			mov bx, _word
			bsf ax, bx
			mov res, ax
	}

	return res;
}

inline uint16 GetFirstHighBit(uint16 _word)
{
	uint16 res = 16;

	__asm
	{
		mov ax, res
			mov bx, _word
			bsr ax, bx
			mov res,ax
	}
	return res;
}


#else


#ifdef WIN32
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)

inline uint16 GetFirstLowBit(uint16 word)
{
	unsigned long lword(word);
	unsigned long index;
	if (_BitScanForward(&index, lword))
		return (uint16)(index);

	return 16;
}

inline uint16 GetFirstHighBit(uint16 word)
{
	unsigned long lword(word);
	unsigned long index;
	if (_BitScanReverse(&index, lword))
		return (uint16)(index);

	return 16;
}

#else


inline uint16 GetFirstLowBitTest(uint16 word)
{
	uint16 c(0);

	if (word & 1)
		return 0;

	uint16 b;
	do 
	{
		word = word >> 1;
		b = word & 1;
		++c;
	} while(!b && c < 16);

	return c; 
}

inline uint16 GetFirstHighBitTest(uint16 word)
{

	uint16 c(0);

	if (word & 0x8000)
		return 15;

	uint16 b;
	do 
	{
		word = word << 1;
		b = word & 0x8000;
		++c;
	} while(!b && c < 16);

	if (c == 16)
		return c;
	else
		return 15-c; 
}


inline uint16 GetFirstLowBit(uint16 word)
{

	uint8 rr = word & 0xFF;
	uint16 res = ControllerHelper::m_byteLowBit[rr];
	if ( res == 8) {
		res = (ControllerHelper::m_byteLowBit[(word >> 8)& 0xFF]) + 8;
	}

	//if (res != GetFirstLowBitTest(word))
	//	int a = 0;
	return res;
}

inline uint16 GetFirstHighBit(uint16 word)
{
	uint16 res = ControllerHelper::m_byteHighBit[(word >> 8)& 0xFF] + 8;
	if ( res == 24) {
		res = (ControllerHelper::m_byteHighBit[word & 0xFF]);
	}
	//if (res != GetFirstHighBitTest(word))
	//	int a = 0;

	return res;
}

#endif
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///template <class TKeyTime, class _Encoder>


class CKeyTimesInformationBitSet  : public IKeyTimesInformation
{
public:
	CKeyTimesInformationBitSet(): m_lastTime(-1)/* : m_lastTime(-1), m_arrKeys(0), m_Start(0),	m_Size(0)*/ 
	{
	};

	//// return key value
	//TKeyTime GetKeyValue(uint32 key) const { return m_arrKeys[key]; };

	// return key value in f32 format
	f32 GetKeyValueFloat(uint32 key) const 
	{ 
		if (key == 0)
		{
			// first one

			return (float)GetHeader()->m_Start;
		}
		else
			if ( key >= GetNumKeys() - 1)
			{
				// last one
				return (float)GetHeader()->m_End;
			}
			// worse situation
			int keys = GetNumKeys();
			int count(0);
			for (int i = 0; i < keys; ++i)
			{
				uint16 bits = GetKeyData(i);

				for (int j = 0; i < 16; ++i)
				{
					if ((bits >> j) & 1)
					{
						++count;
						if ((count - 1) == key)
							return (float)(i * 16 + j);
					}
				}
			}

			return 0;
	}

	// return key number for normalized_time
	uint32	GetKey( f32 normalized_time, f32& difference_time)
	{
		f32 realtime = normalized_time;

		if (realtime == m_lastTime)
		{
			difference_time =  m_LastDifferenceTime;
			return m_lastKey;
		}
		m_lastTime = realtime;
		uint32 numKey = (uint32)GetHeader()->m_Size;//m_arrKeys.size();

		f32 keytime_start = (float)GetHeader()->m_Start;
		f32 keytime_end = (float)GetHeader()->m_End;
		f32 test_end = keytime_end;

		if( realtime < keytime_start )
			test_end += realtime;

		if( realtime < keytime_start )
		{
			difference_time = 0;
			m_lastKey = 0;
			return 0;
		}

		if( realtime >= keytime_end )
		{
			difference_time = 0;
			m_lastKey = numKey;
			return numKey;
		}

		f32 internalTime = realtime - keytime_start;
		uint16 uTime = (uint16)internalTime;
		uint16 piece = (uTime / sizeof(uint16)) >> 3;
		uint16 bit = /*15 - */(uTime % 16);
		uint16 data = GetKeyData(piece);
		uint16 left = data >> bit;

		//left
		left = data & (0xFFFF >> (15 - bit));
		uint16 leftPiece(piece);
		uint16 nearestLeft=0;
		uint16 wBit;
		while ((wBit = GetFirstHighBit(left)) == 16)
		{
			--leftPiece;
			left = GetKeyData(leftPiece);
		}
		nearestLeft = leftPiece * 16 + wBit;

		//right
		uint16 right = ((data >> (bit + 1)) & 0xFFFF) << (bit + 1);
		uint16 rigthPiece(piece);
		uint16 nearestRight=0;
		while ((wBit = GetFirstLowBit(right)) == 16)
		{
			++rigthPiece;
			right = GetKeyData(rigthPiece);
		}

		nearestRight = ((rigthPiece  * sizeof(uint16)) << 3) + wBit;
		m_LastDifferenceTime = difference_time = (f32)(internalTime-(f32)nearestLeft) / ((f32)nearestRight - (f32)nearestLeft);

		// count nPos
		uint32 nPos(0);
		for (uint16 i = 0; i < rigthPiece; ++i)
		{
			uint16 data2 = GetKeyData(i);
			nPos += ControllerHelper::m_byteTable[data2 & 255] + ControllerHelper::m_byteTable[data2 >> 8];
		}

		data = GetKeyData(rigthPiece);
		data = ((data <<  (15 - wBit)) & 0xFFFF) >> (15 - wBit);
		nPos += ControllerHelper::m_byteTable[data & 255] + ControllerHelper::m_byteTable[data >> 8];

		m_lastKey = nPos - 1;

		return m_lastKey;
	}

	uint32 GetNumKeys() const {return GetHeader()->m_Size; };

	void AddKeyTime(f32 val)
	{
		//	m_arrKeys.push_back(/*_Encoder::EncodeKeyValue*/(TKeyTime)(val));
	};

	void ReserveKeyTime(uint32 count)
	{

		m_arrKeys.reserve(count);
	};

	void ResizeKeyTime(uint32 count)
	{
		m_arrKeys.resize(count);
	};

	uint32 GetFormat()
	{
		return eBitset;
	}

	char * GetData()
	{
		return (char*)m_arrKeys.begin();
	}

	uint32 GetDataRawSize()
	{
		return (uint32)((char*)m_arrKeys.end() - (char*)m_arrKeys.begin());
	}

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_arrKeys);
	}
protected:

	struct Header 
	{
		uint16		m_Start;
		uint16    m_End;
		uint16		m_Size;
	};

	inline Header* GetHeader() const
	{
		return (Header*)(&m_arrKeys[0]);
	};

	inline uint16 GetKeyData(int i) const
	{
		return m_arrKeys[3 + i];
	};

	DynArray<uint16> m_arrKeys;

	f32 m_lastTime;
	f32 m_LastDifferenceTime;
	uint32 m_lastKey;
};


//CKeyTimesInformationBitSet
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
template<class _Type, class _Interpolator, class _Base, class _Storage >
*/

class ITrackPositionStorage// : public _reference_target_t
{
public:
	virtual ~ITrackPositionStorage() {};
	virtual void AddValue(Vec3& val) = 0;
	virtual char * GetData() = 0;
	virtual uint32 GetDataRawSize() = 0;
	virtual void Resize(uint32 count) = 0;
	virtual void Reserve(uint32 count) = 0;
	virtual uint32 GetFormat() = 0;
	virtual void GetValue(uint32 key, Vec3& val) = 0;
	virtual size_t SizeOfPosController() = 0;
	virtual void GetMemoryUsage(ICrySizer *pSizer ) const =0;
};

typedef /*_smart_ptr<ITrackPositionStorage>*/ITrackPositionStorage * TrackPositionStoragePtr;


class ITrackRotationStorage/// : public _reference_target_t
{
public:
	virtual ~ITrackRotationStorage() {};
	virtual void AddValue(Quat& val) = 0;
	virtual char * GetData() = 0;
	virtual uint32 GetDataRawSize() = 0;
	virtual void Resize(uint32 count) = 0;
	virtual void Reserve(uint32 count) = 0;
	virtual uint32 GetFormat() = 0;
	virtual void GetValue(uint32 key, Quat& val) = 0;
	virtual size_t SizeOfRotController() = 0;

	void GetMemoryUsage( ICrySizer *pSizer ) const { /*nothing*/}
};

typedef ITrackPositionStorage * TrackPositionStoragePtr;
typedef ITrackRotationStorage * TrackRotationStoragePtr;

// specialization 
template<class _Type, class _Storage, class _Base>
class CTrackDataStorageInt : public _Base
{
public:

	//__declspec(noinline) virtual ~CTrackDataStorageInt()
	//{
	//	m_arrKeys.clear();
	//}

	virtual void Resize(uint32 count)
	{
		m_arrKeys.resize(count);
	}

	virtual void Reserve(uint32 count)
	{
		m_arrKeys.reserve(count);
	}

	virtual void AddValue(_Type& val)
	{
		_Storage v;
		v.ToInternalType(val);
		m_arrKeys.push_back(v);
	}

	char * GetData()
	{
		return (char*) m_arrKeys.begin();
	}

	uint32 GetDataRawSize()
	{
		return (uint32)((char*)m_arrKeys.end() - (char*)m_arrKeys.begin());
	}

	SPU_INDIRECT(CommandBufferExecute(ML))
		SPU_NO_INLINE void GetValue(uint32 key, _Type& val)
	{
		m_arrKeys[key].ToExternalType(val);
	}

	virtual uint32 GetFormat()
	{
		return _Storage::GetFormat();
	}

	virtual size_t SizeOfRotController()
	{
		size_t keys = m_arrKeys.size();
		size_t test=sizeof(_Storage);
		size_t s0=sizeof(*this);				assert(s0==8);
		size_t s1=m_arrKeys.get_alloc_size();	
		return s0+s1;
	}
	virtual size_t SizeOfPosController()
	{
		size_t keys = m_arrKeys.size();
		size_t test=sizeof(_Storage);		assert(test==12);
		size_t s0=sizeof(*this);				assert(s0==8);
		size_t s1=m_arrKeys.get_alloc_size();	
		return s0+s1;
	}

	DynArray<_Storage> m_arrKeys;
};

template<class _Type, class _Storage, class _Base>
class CTrackDataStorage : public CTrackDataStorageInt<_Type, _Storage, _Base>
{
public:
	CTrackDataStorage() /*: m_nRefCounter(0)*/ {};

	virtual void AddRef()
	{
		//++m_nRefCounter;
	}

	virtual void Release()
	{
		//if (--m_nRefCounter <= 0)
		delete this;
	}

	virtual void GetMemoryUsage(ICrySizer *pSizer )const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
protected:
	//uint32 m_nRefCounter;
};

template<class _Type, class _Storage, class _Base>
class CTrackDataStorageInplace : public CTrackDataStorageInt<_Type, _Storage, _Base>
{
public:
	virtual void Release()
	{
		//if (--m_nRefCounter <= 0)
		//	this->~CTrackDataStorageInplace();
	}
	virtual void AddRef()
	{
	}
	virtual void GetMemoryUsage(ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

// normal 
//typedef CTrackDataStorage<Vec3, NoCompressVec3, ITrackPositionStorage> NoCompressPosition;
//typedef _smart_ptr<NoCompressPosition> NoCompressPositionPtr;
//
//typedef CTrackDataStorage<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotation;
//typedef _smart_ptr<NoCompressRotation> NoCompressRotationPtr;
//
//typedef CTrackDataStorage<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotation;
//typedef _smart_ptr<SmallTree48BitQuatRotation> SmallTree48BitQuatRotationPtr;
//
//typedef CTrackDataStorage<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotation;
//typedef _smart_ptr<SmallTree64BitQuatRotation> SmallTree64BitQuatRotationPtr;
//
//typedef CTrackDataStorage<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotation;
//typedef _smart_ptr<SmallTree64BitExtQuatRotation> SmallTree64BitExtQuatRotationPtr;
//
//// Inplace copies
//typedef CTrackDataStorageInplace<Vec3, NoCompressVec3, ITrackPositionStorage> NoCompressPositionInplace;
//typedef _smart_ptr<NoCompressPositionInplace> NoCompressPositionInplacePtr;
//
//typedef CTrackDataStorageInplace<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotationInplace;
//typedef _smart_ptr<NoCompressRotationInplace> NoCompressRotationInplacePtr;
//
//typedef CTrackDataStorageInplace<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotationInplace;
//typedef _smart_ptr<SmallTree48BitQuatRotationInplace> SmallTree48BitQuatRotationInplacePtr;
//
//typedef CTrackDataStorageInplace<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotationInplace;
//typedef _smart_ptr<SmallTree64BitQuatRotationInplace> SmallTree64BitQuatRotationInplacePtr;
//
//typedef CTrackDataStorageInplace<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotationInplace;
//typedef _smart_ptr<SmallTree64BitExtQuatRotationInplace> SmallTree64BitExtQuatRotationInplacePtr;

typedef CTrackDataStorage<Vec3, NoCompressVec3, ITrackPositionStorage> NoCompressPosition;
typedef NoCompressPosition NoCompressPositionPtr;

typedef CTrackDataStorage<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotation;
typedef NoCompressRotation * NoCompressRotationPtr;

typedef CTrackDataStorage<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotation;
typedef SmallTree48BitQuatRotation * SmallTree48BitQuatRotationPtr;

typedef CTrackDataStorage<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotation;
typedef SmallTree64BitQuatRotation * SmallTree64BitQuatRotationPtr;

typedef CTrackDataStorage<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotation;
typedef SmallTree64BitExtQuatRotation * SmallTree64BitExtQuatRotationPtr;

// Inplace copies
typedef CTrackDataStorageInplace<Vec3, NoCompressVec3, ITrackPositionStorage> NoCompressPositionInplace;
typedef NoCompressPositionInplace * NoCompressPositionInplacePtr;

typedef CTrackDataStorageInplace<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotationInplace;
typedef NoCompressRotationInplace * NoCompressRotationInplacePtr;

typedef CTrackDataStorageInplace<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotationInplace;
typedef SmallTree48BitQuatRotationInplace * SmallTree48BitQuatRotationInplacePtr;

typedef CTrackDataStorageInplace<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotationInplace;
typedef SmallTree64BitQuatRotationInplace * SmallTree64BitQuatRotationInplacePtr;

typedef CTrackDataStorageInplace<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotationInplace;
typedef SmallTree64BitExtQuatRotationInplace * SmallTree64BitExtQuatRotationInplacePtr;

class ITrackInformation  //: public _reference_target_t
{
public:

	ITrackInformation(): m_pKeyTimes(0) {};
	~ITrackInformation()
	{
		//if (m_pKeyTimes)
		//	delete m_pKeyTimes;
	}

	uint32 GetNumKeys()
	{
		return m_pKeyTimes->GetNumKeys();
	}

	f32 GetTimeFromKey(uint32 key)
	{
		return m_pKeyTimes->GetKeyValueFloat(key);
	}

	void SetKeyTimesInformation(KeyTimesInformationPtr& ptr)
	{
		m_pKeyTimes = ptr;
	}

	KeyTimesInformationPtr& GetKeyTimesInformation()
	{
		return m_pKeyTimes;
	}

	virtual char * GetData() = 0;


protected:

	KeyTimesInformationPtr m_pKeyTimes;

};




//common interface for positions
class IPositionController : public ITrackInformation
{
public:
	virtual ~IPositionController() 
	{
		delete m_pData;
	};
	IPositionController() : m_pData(0) {};

	virtual void GetValue ( f32 normalized_time, Vec3& quat) = 0;
	virtual size_t SizeOfPosController()
	{
		uint32 numTimes   = m_pKeyTimes->GetNumKeys();
		uint32 numFormat  = m_pKeyTimes->GetFormat();
		uint32 numRawSize = m_pKeyTimes->GetDataRawSize();
		size_t s0 = sizeof(IPositionController); 
		size_t s1 = m_pData->SizeOfPosController();
		return s0+s1+numRawSize;
	}
	virtual void GetValueFromKey(uint32 key, Vec3& val);	

	virtual void SetPositionStorage(TrackPositionStoragePtr& ptr)
	{
		m_pData = ptr;
	}

	virtual TrackPositionStoragePtr& GetPositionStorage()
	{
		return m_pData;
	}

	virtual uint32 GetFormat()
	{
		return m_pData->GetFormat();
	}

	char * GetData()
	{
		return m_pData->GetData();
	}

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const { /* finish this controller madness*/}
protected:
	TrackPositionStoragePtr m_pData;
};


// Common interface for rotation information
class IRotationController : public ITrackInformation
{
public:

	IRotationController() : m_pData(0) {};


	virtual ~IRotationController() 
	{
		delete m_pData;
	};

	virtual void GetValue ( f32 normalized_time, Quat& quat) = 0;
	virtual size_t SizeOfRotController()
	{
		uint32 numRawSize = m_pKeyTimes->GetDataRawSize();
		return sizeof(IRotationController) + m_pData->SizeOfRotController() + numRawSize;
	}

	virtual void GetValueFromKey(uint32 key, Quat& val);	

	virtual void SetRotationStorage(TrackRotationStoragePtr& ptr)
	{
		m_pData = ptr;	
	}
	virtual TrackRotationStoragePtr& GetRotationStorage()
	{
		return m_pData;
	}

	virtual uint32 GetFormat()
	{
		return m_pData->GetFormat();

	}

	char *GetData() 
	{
		return m_pData->GetData();
	}

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject( m_pData );
	}
protected:
	TrackRotationStoragePtr m_pData;
};


typedef IRotationController* RotationControllerPtr;
typedef IPositionController* PositionControllerPtr;

struct Vec3Lerp
{
	static inline void Blend(Vec3& res, const Vec3& val1, const Vec3& val2, f32 t)
	{
		res.SetLerp(val1, val2, t);
	}
};

struct QuatLerp
{
	static inline void Blend(Quat& res, const Quat& val1, const Quat& val2, f32 t)
	{
		res.SetNlerp(val1, val2, t);
	}
};


template<class _Type, class _Interpolator, class _Base>
class CAdvancedTrackInformation : public _Base
{
public:
	CAdvancedTrackInformation() 
	{
	}

	~CAdvancedTrackInformation()
	{

	}


	/*
	virtual uint32 GetKeyP()
	{
	return this->GetNumKeys();
	}
	virtual uint32 GetKeyO()
	{
	return this->GetNumKeys();
	}
	*/


	SPU_INDIRECT(CommandBufferExecute(ML))
		SPU_NO_INLINE virtual void GetValue ( f32 normalized_time, _Type& pos)
	{
		//DEFINE_PROFILER_SECTION("ControllerPQ::GetValue");
		f32 t;
		uint32 key = this->m_pKeyTimes->GetKey( normalized_time, t);

		if (key == 0)
		{
			DEFINE_ALIGNED_DATA (_Type, p1, 16);
			GetValueFromKey(0, p1);
			pos = p1;
		}
		else
		{
			uint32 numKeys = this->GetNumKeys();
			if (key >= numKeys )
			{
				DEFINE_ALIGNED_DATA (_Type, p1, 16);
				GetValueFromKey(key-1, p1);
				pos = p1;
			}
			else
			{
				//_Type p1, p2;
				DEFINE_ALIGNED_DATA (_Type, p1, 16);
				DEFINE_ALIGNED_DATA (_Type, p2, 16);
				GetValueFromKey(key-1, p1);
				GetValueFromKey(key, p2);

				_Interpolator::Blend(pos, p1, p2, t);
			}
		}
	}

};

template<class _Type, class _Interpolator, class _Base>
class CAdvancedTrackInformationInplace : public CAdvancedTrackInformation<_Type, _Interpolator, _Base>
{
	virtual void Release()
	{
		//if (--m_nRefCounter <= 0)
		//	delete this;
	}
	virtual void AddRef()
	{
	}

};


typedef CAdvancedTrackInformation<Quat, QuatLerp, IRotationController> RotationTrackInformation;
typedef RotationTrackInformation * RotationTrackInformationPtr;

typedef CAdvancedTrackInformation<Vec3, Vec3Lerp, IPositionController> PositionTrackInformation;
typedef PositionTrackInformation * PositionTrackInformationPtr;

typedef CAdvancedTrackInformationInplace<Quat, QuatLerp, IRotationController> RotationTrackInformationInplace;
typedef RotationTrackInformationInplace * RotationTrackInformationInplacePtr;

typedef CAdvancedTrackInformationInplace<Vec3, Vec3Lerp, IPositionController> PositionTrackInformationInplace;
typedef PositionTrackInformationInplace * PositionTrackInformationInplacePtr;

// My implementation of controllers
class CController : public IController
{
public:
	//Creation interface

	CController() : m_pRotationController(0) , m_pPositionController(0) {}

	/*__declspec(noinline)*/ ~CController()
	{
		if (m_pRotationController)
			delete m_pRotationController;
		if (m_pPositionController)
			delete m_pPositionController;
	}

	uint32 numKeysPQ() const
	{
		// now its hack, because num keys might be different
		if (m_pRotationController)
			return m_pRotationController->GetNumKeys();

		if (m_pPositionController)
			return m_pPositionController->GetNumKeys();

		return 0;
		//return m_arrKeys.size();
	}

	//uint32 GetID () const {return m_nControllerId;}

	Status4 GetOPS ( f32 normalized_time, Quat& quat, Vec3& pos, Diag33& scale)
	{
		Status4 res;
		res.o = GetO( normalized_time, quat) ? 1 : 0;
		res.p = GetP( normalized_time, pos) ? 1 : 0;
		res.s = GetS( normalized_time, scale) ? 1: 0;

		return res; 

	}

	SPU_INDIRECT(CommandBufferExecute(MLL))
		Status4 GetOP ( f32 normalized_time, Quat& quat, Vec3& pos)
	{
		Status4 res;
		res.o = GetO( normalized_time, quat) ? 1 : 0;
		res.p = GetP( normalized_time, pos) ? 1 : 0;

		return res; 
	}

	SPU_INDIRECT(CommandBufferExecute(ML))
		uint32 GetO( f32 normalized_time, Quat& quat)
	{
		if (m_pRotationController)
		{
			m_pRotationController->GetValue( normalized_time, quat);

			return STATUS_O;
		}

		return 0;
	}

	SPU_INDIRECT(CommandBufferExecute(ML))
		uint32 GetP( f32 normalized_time, Vec3& pos)
	{
		if (m_pPositionController)
		{
			m_pPositionController->GetValue( normalized_time, pos);
			return STATUS_P;
		}

		return 0;
	}

	uint32 GetS( f32 normalized_time, Diag33& pos)
	{
		//if (m_pScaleController)
		//{
		//	Vec3 scale;
		//	m_pScaleController->GetValue( normalized_time, scale);

		//	pos.x = scale.x;
		//	pos.y = scale.y;
		//	pos.z = scale.z;

		//	return STATUS_S;
		//}
		return 0;
	}

	int32 GetO_numKey()
	{
		if (m_pRotationController)
			return m_pRotationController->GetNumKeys();
		return -1;
	}
	int32 GetP_numKey()
	{
		if (m_pPositionController)
			return m_pPositionController->GetNumKeys();
		return -1;
	}
#if !defined(__CRYCG__) // crycg runs into an error when scanning these functions(even if they are not used), so they are ifdefed out


	uint32 HasPositionChannel()
	{
		if (m_pPositionController)
			return 1;
		return 0;
	}
#endif



	size_t SizeOfController () const
	{
		size_t res=sizeof(*this);
		if (m_pPositionController)
			res += m_pPositionController->SizeOfPosController();
		if (m_pRotationController)
			res += m_pRotationController->SizeOfRotController();
		return  res;
	}

	size_t ApproximateSizeOfThis () const
	{
		return SizeOfController();
	}



	CInfo GetControllerInfo() const
	{
		CInfo info;
		//		info.numKeys = numKeys();
		if (m_pRotationController)
		{
			info.m_numKeys		= m_pRotationController->GetNumKeys();
			info.m_nEndKey		= (int)m_pRotationController->GetTimeFromKey(info.m_numKeys - 1);
			info.m_nStartKey	= (int)m_pRotationController->GetTimeFromKey(0);
		}

		if (m_pPositionController)
		{
			info.m_numKeys		= m_pPositionController->GetNumKeys();
			info.m_nEndKey		= (int)m_pPositionController->GetTimeFromKey(info.m_numKeys - 1);
			info.m_nStartKey	= (int)m_pPositionController->GetTimeFromKey(0);
		}

		info.m_TotalKeys = (info.m_nEndKey-info.m_nStartKey)/*/0xa0*/+1;
		return info;
	}


	void SetRotationController(RotationControllerPtr& ptr)
	{
		m_pRotationController = ptr;
	}

	void SetPositionController(PositionControllerPtr& ptr)
	{
		m_pPositionController = ptr;
	}

	void SetScaleController(PositionControllerPtr& ptr)
	{
		//		m_pScaleController = ptr;
	}

	RotationControllerPtr& GetRotationController()
	{
		return m_pRotationController;
	}

	PositionControllerPtr& GetPositionController()
	{
		return m_pPositionController;
	}

	//PositionControllerPtr& GetScaleController()
	//{
	//	return 0;//m_pScaleController;
	//}

	virtual EControllerInfo GetControllerType();

	size_t GetRotationKeysNum()
	{
		if (m_pRotationController)
			return m_pRotationController->GetNumKeys();
		return 0;
	}

	size_t GetPositionKeysNum()
	{
		if (m_pPositionController)
			return m_pPositionController->GetNumKeys();
		return 0;
	}	

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_pRotationController);
		pSizer->AddObject(m_pPositionController);
	}

	//	uint32 m_nControllerId;
	//members
protected:
	RotationControllerPtr m_pRotationController;
	PositionControllerPtr m_pPositionController;
	//	PositionControllerPtr m_pScaleController;
};


TYPEDEF_AUTOPTR(CController);

class CControllerInplace : public CController
{
public:
	virtual void AddRef()
	{
	}

	virtual void Release()
	{
		//if (--m_nRefCounter <= 0)
		//	delete this;
	}

};

TYPEDEF_AUTOPTR (CControllerInplace);

class CCompressedController : public CController
{

public:
	void UnCompress();
	void Compress(IController * uncompressedController);//RotationControllerPtr& uncompRotation, PositionControllerPtr& uncompPosition);

	int GetCompressedSize() { return m_CompressedSize;};


public:
	TCompressetWaveletData m_Rotations0, m_Rotations1, m_Rotations2;// [3];
	TCompressetWaveletData m_Positions0, m_Positions1, m_Positions2;//[3];

	int m_CompressedSize;

	uint8 m_iRotationFormat;
	uint8 m_iPositionFormat;

};


TYPEDEF_AUTOPTR(CCompressedController);
#endif

