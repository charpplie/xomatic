////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   controllerpq.h
//  Version:     v1.00
//  Created:     14/01/2007 by Alexey.
//  Compilers:   Visual Studio.NET 2005
//  Description: Optimized Quat\pos controller implementation.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef _CRYTEK_CONTROLLER_OPT_PQ_
#define _CRYTEK_CONTROLLER_OPT_PQ_
#pragma once


#include "CGFContent.h"
#include "QuatQuantization.h"
#include "ControllerPQ.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline uint32 ControllerHash(uint32 a, uint32 b, uint32 c, uint32 d)
{
	return (a & 0xFF) + ((b & 0xFF) << 8) + ((c & 0xFF) << 16) + ((d & 0xFF) << 24);
}

class CEmptyKeyTimesData
{
public:

	f32 GetKeyValueFloat(uint32 key) const
	{
		return 0.0f;
	}

	uint32	GetKey( f32 normalized_time, f32& difference_time)
	{
		// you should not to be here!
		return 0;
	}

	int GetFormat()
	{
		return eNoFormat;
	}

	int GetNumKeys() const
{	
		return 0;
	}

	// set number
	void SetNumKeys(uint32 keys)
	{

	}

	// set pointer to data
	void SetData(char * pData)
	{

	}

	char * GetData()
	{
		return 0;
	}

	ILINE void SetKeyTimes(uint32 num, char * pData)
	{

	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template<class _Encoder>
struct CKeyTimesData
{

	typedef typename _Encoder::TKeyTypeTraits TData;
	typedef typename _Encoder::TKeyTypeCountTraits TCount;

public:
	CKeyTimesData() : m_iCount(0), m_pData(0), m_lastKey(~0), m_lastTime(-1) {};

	ILINE void SetKeyTimeNumKeys(uint32 i)
	{
		m_iCount = static_cast<TCount>(i);
	}

	ILINE void SetKeyTimeData(char * pData)
	{
		m_pData = static_cast<TData*>(pData);
	}

	ILINE void SetKeyTimes(uint32 num, char * pData)
	{
		m_iCount = static_cast<TCount>(num);
		m_pData = (TData*)(pData);
	}

	char * GetData()
	{
		return (char*)(m_pData);
	}

	size_t GetKeyTimeSize() const
	{
		return sizeof(TData) * m_iCount;
	}

protected:

	static uint32 GetKeyTimeFormat()
	{
		return _Encoder::GetFormat();
	}

	ILINE TData GetKeyValueFloat(uint32 key)
	{
		return m_pData[key];
	}

	ILINE TData GetValueSafe(uint32 key)
	{
		if (key < m_iCount)
		{
			return m_pData[key];
		}

		return TData(0);
	}

	ILINE uint32 GetKeyTimeNumKeys()
	{
		return m_iCount;
	}

	uint32	GetKey( f32 normalized_time, f32& difference_time)
	{
		f32 realtime = normalized_time;

		if (realtime == m_lastTime)
		{
			difference_time =  m_LastDifferenceTime;
			return m_lastKey;
		}
		m_lastTime = realtime;

		uint32 numKey = (uint32)m_iCount;//m_arrKeys.size();

		TData keytime_start = m_pData[0];
		TData keytime_end = m_pData[numKey-1];

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
			if(realtime < m_pData[nPos])
				nPos = nPos - nStep;
			else
				if(realtime > m_pData[nPos])
					nPos = nPos + nStep;
				else 
					break;

			nStep = nStep>>1;
		}

		// fine-tuning needed since time is not linear
		while(realtime > m_pData[nPos])
			nPos++;

		while(realtime < m_pData[nPos-1])
			nPos--;

		m_lastKey = nPos;
		// possible error if encoder uses nonlinear methods!!!
		m_LastDifferenceTime = difference_time = (f32)(realtime-(f32)m_pData[nPos-1]) / ((f32)m_pData[nPos] - (f32)m_pData[nPos-1]);

		return nPos;
	}

	ILINE uint32 GetNumKeys() const
	{
		return (uint32)m_iCount;
	}

private:
	f32 m_lastTime;
	f32 m_LastDifferenceTime;
	uint32 m_lastKey;

	TCount m_iCount;
	TData  *m_pData;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef CKeyTimesData<ByteEncoder> CKeyTimesByte;
typedef CKeyTimesData<UINT16Encoder> CKeyTimesUINT16;
typedef CKeyTimesData<F32Encoder> CKeyTimesF32;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template<class _Encoder>
struct CKeyTimesDataBitSet
{
	typedef typename _Encoder::TKeyTypeTraits TData;
	typedef typename _Encoder::TKeyTypeCountTraits TCount;
public:
	CKeyTimesDataBitSet() : m_pData(0), m_lastKey(~0), m_lastTime(-1) {};

	/*
	ILINE void SetKeyTimeNumKeys(uint32 i)
	{
	m_iCount = static_cast<TCount>(i);
	}
	*/
	ILINE void SetKeyTimeData(char * pData)
	{
		m_pData = static_cast<TData*>(pData);
	}

	ILINE void SetKeyTimes(uint32 num, char * pData)
	{
		//		m_iCount = static_cast<TCount>(num);
		m_pData = (TData*)(pData);
	}

	char * GetData()
	{
		return (char*)(m_pData);
	}

	size_t GetKeyTimeSize() const
	{
		return GetNumKeys() / 8 + 1;
	}


protected:

	static uint32 GetKeyTimeFormat()
	{
		return _Encoder::GetFormat();
	}

	ILINE TData GetKeyValueFloat(uint32 key)
	{
		if (key == 0) {
			// first one
			return (float)GetHeader()->m_Start;
		}
		else
			if ( key >= GetNumKeys() - 1) {
				// last one
				return (float)GetHeader()->m_End;
			}
			// worse situation
			int c(0);

			int keys = GetNumKeys();
			int count(0);
			for (int i = 0; i < keys; ++i) {
				uint16 bits = GetKeyData(i);

				for (int j = 0; j < 16; ++j) {
					if ((bits >> j) & 1) {
						++count;
						if ((count - 1) == key)
							return (float)(i * 16 + j);
					}
				}
			}

			return 0;

	}
	/*
	ILINE TData GetValueSafe(uint32 key)
	{
	if (key < m_iCount)
	{
	return m_pData[key];
	}

	return TData(0);
	}


	ILINE uint32 GetKeyTimeNumKeys()
	{
	return m_iCount;
	}
	*/

	uint32	GetKey( f32 normalized_time, f32& difference_time)
	{
		f32 realtime = normalized_time;

		if (realtime == m_lastTime) {
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

		if( realtime < keytime_start ) {
			difference_time = 0;
			m_lastKey = 0;
			return 0;
		}

		if( realtime >= keytime_end ) {
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
		while ((wBit = GetFirstHighBit(left)) == 16) {
			--leftPiece;
			left = GetKeyData(leftPiece);
		}
		nearestLeft = leftPiece * 16 + wBit;

		//right
		uint16 right = ((data >> (bit + 1)) & 0xFFFF) << (bit + 1);
		uint16 rigthPiece(piece);
		uint16 nearestRight=0;
		while ((wBit = GetFirstLowBit(right)) == 16) {
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

	ILINE uint32 GetNumKeys() const
	{
		return GetHeader()->m_Size; 
	}

private:

	struct Header 
	{
		uint16		m_Start;
		uint16    m_End;
		uint16		m_Size;
	};

	inline Header* GetHeader() const
	{
		return (Header*)(&m_pData[0]);
	};

	inline uint16 GetKeyData(int i) const
	{
		return m_pData[3 + i];
	};

	f32 m_lastTime;
	f32 m_LastDifferenceTime;
	uint32 m_lastKey;

	TData  *m_pData;
	//	TCount m_iCount;

	//private:
	//	static byte m_bTable[256];
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef CKeyTimesDataBitSet<BitsetEncoder> CKeyTimesBitset;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class KeyTimeClass>
class CEmptyRotationData : public KeyTimeClass
{
public:

	uint32 GetRotationNumCount() const
	{
		return 0;	
	}

	uint32 GetRotationValue( f32 normalized_time, Quat& q)
	{
		return 0;
	}

	void GetRotationValueFromKey(uint32 key, Quat&)
	{

	}

	void SetRotationData(uint32 num, char * m_pData)
	{

	}

	char * GetRotationData()
	{
		return 0;
	}

	char * GetRotKeyTimes()
	{
		return 0;
	}

	void SetRotKeyTimes(uint32 num, char * pData)
	{
	}

	static uint32 GetRotationKeyTimeFormat()
	{
		return eNoFormat;
	}

	static uint32 GetRotationType()
	{
		return eNoFormat;
	}
	size_t GetRotationsSize() const {	return 0; }

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef CEmptyRotationData<CEmptyKeyTimesData> CEmptyRotationController;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct QuatLerpNoCompression
{
	static inline void Blend(Quat& res, const Quat& val1, const Quat& val2, f32 t)
	{
		res.SetNlerp(val1, val2, t);
	}

	static uint32 GetType()
	{
		return eNoCompressQuat;
	}

	typedef NoCompressQuat TDataTypeTraits;
	typedef uint16 TDataTypeCountTraits;
};

struct QuatLerpSmallTree48BitQuat
{
	static inline void Blend(Quat& res, const Quat& val1, const Quat& val2, f32 t)
	{
		res.SetNlerp(val1, val2, t);
	}

	static uint32 GetType()
	{
		return eSmallTree48BitQuat;
	}

	typedef SmallTree48BitQuat TDataTypeTraits;
	typedef uint16 TDataTypeCountTraits;
};

struct QuatLerpSmallTree64BitQuat
{
	static inline void Blend(Quat& res, const Quat& val1, const Quat& val2, f32 t)
	{
		res.SetNlerp(val1, val2, t);
	}

	static uint32 GetType()
	{
		return eSmallTree64BitQuat;
	}

	typedef SmallTree64BitQuat TDataTypeTraits;
	typedef uint16 TDataTypeCountTraits;
};

struct QuatLerpSmallTree64BitExtQuat
{
	static inline void Blend(Quat& res, const Quat& val1, const Quat& val2, f32 t)
	{
		res.SetNlerp(val1, val2, t);
	}
	static uint32 GetType()
	{
		return eSmallTree64BitExtQuat;
	}

	typedef SmallTree64BitExtQuat TDataTypeTraits;
	typedef uint16 TDataTypeCountTraits;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class KeyTimeClass, class _Interpolator>
class CRotationData : public KeyTimeClass
{
public:

	typedef typename _Interpolator::TDataTypeTraits T;
	typedef typename _Interpolator::TDataTypeCountTraits C;

	ILINE uint32 GetRotationNumCount() const
	{
		return this->GetNumKeys();
	}

	ILINE uint32 GetRotationValue( f32 normalized_time, Quat& q)
	{
		q = GetValue( normalized_time);
		return STATUS_O;
	}

	ILINE void GetRotationValueFromKey(uint32 key, Quat& t)
	{
		uint32 num = GetRotationNumCount();
		if (key >= num)
			key = num - 1;

		GetValueFromKey(key, t);
	}

	ILINE void SetRotationData(uint32 num, char * m_pData)
	{
		//		m_iCount = num;
		assert(num == this->GetNumKeys());
		m_arrRots = (T*)m_pData;
	}

	char * GetRotationData()
	{
		return (char *)m_arrRots;
	}

	static uint32 GetRotationType()
	{
		return _Interpolator::GetType();
	}

	static uint32 GetRotationKeyTimeFormat()
	{
		return KeyTimeClass::GetKeyTimeFormat();
	}

	void SetRotKeyTimes(uint32 num, char * pData)
	{
		this->SetKeyTimes(num, pData);
	}

	char * GetRotKeyTimes()
	{
		return this->GetData();
	}

	size_t GetRotationsSize() const {
		return sizeof(T) * GetRotationNumCount() + this->GetKeyTimeSize();
	}

private:

	Quat GetValue ( f32 normalized_time)
	{
		//DEFINE_PROFILER_SECTION("ControllerPQ::GetValue");
		DEFINE_ALIGNED_DATA (Quat, pos, 16);
		//Quat pos;

		f32 t;
		uint32 key = this->GetKey( normalized_time, t);

		if (key == 0)
		{
			DEFINE_ALIGNED_DATA (Quat, p1, 16);
			GetValueFromKey(0, p1);
			pos = p1;
		}
		else
			if (key >= GetRotationNumCount())
			{
				DEFINE_ALIGNED_DATA (Quat, p1, 16);
				assert(key - 1 < GetRotationNumCount());
				GetValueFromKey(GetRotationNumCount()-1, p1);
				pos = p1;
			}
			else
			{
				//	Quat p1, p2;
				DEFINE_ALIGNED_DATA (Quat, p1, 16);
				DEFINE_ALIGNED_DATA (Quat, p2, 16);

				GetValueFromKey(key-1, p1);
				GetValueFromKey(key, p2);

				_Interpolator::Blend(pos, p1, p2, t);
			}

			return pos;
	}

	ILINE void GetValueFromKey(uint32 key, Quat& val)
	{
		m_arrRots[key].ToExternalType(val);
	}

private:
	T * m_arrRots;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
template<class T, class C, class KeyTimeClass, class _Interpolator>
typedef CRotationData<SmallTree64BitExtQuat, uint16, CKeyTimesUINT16, > CRotationNoCompressKeysF32;
SmallTree48BitQuat
NoCompressQuat
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class KeyTimeClass>
class CEmptyPositionData : public KeyTimeClass
{
public:

	uint32 GetPositionNumCount()  const
	{
		return 0;
	}

	uint32 GetPositionValue( f32 normalized_time,Vec3& p)
	{
		return 0;
	}

	void SetPositionData(uint32 num, char * m_pData)
	{

	}

	char * GetPositionData()
	{
		return 0;
	}

	static uint32 GetPositionKeyTimeFormat()
	{
		return eNoFormat;
	}

	static uint32 GetPositionType()
	{
		return eNoFormat;
	}

	void SetPosKeyTimes(uint32 num, char * pData)
	{

	}

	char * GetPosKeyTimes()
	{
		return 0;
	}

	size_t GetPositionsSize() const { return 0; }

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef CEmptyPositionData<CEmptyKeyTimesData> CEmptyPositionController;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Vec3LerpF32
{
	static inline void Blend(Vec3& res, const Vec3& val1, const Vec3& val2, f32 t)
	{
		res.SetLerp(val1, val2, t);
	}

	static uint32 GetType()
	{
		return eNoCompressVec3;
	}

	typedef NoCompressVec3 TDataTypeTraits;
	typedef uint16 TDataTypeCountTraits;

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class KeyTimeClass, class _Interpolator>
class CPositionData : public KeyTimeClass
{
public:
	typedef typename _Interpolator::TDataTypeTraits T;
	typedef typename _Interpolator::TDataTypeCountTraits C;

	ILINE uint32 GetPositionNumCount() const
	{
		return this->GetNumKeys();
	}

	uint32 GetPositionValue( f32 normalized_time,Vec3& p)
	{
		p = GetValue( normalized_time);
		return STATUS_P;
	}

	ILINE void GetPositionValueFromKey(uint32 key, Vec3& t)
	{
		GetValueFromKey(key, t);
	}

	void SetPositionData(uint32 num, char * m_pData)
	{
		assert(num == this->GetNumKeys());
		m_arrKeys = (T*)m_pData;
	}

	static uint32 GetPositionType()
	{
		return _Interpolator::GetType();
	}

	static uint32 GetPositionKeyTimeFormat()
	{
		return KeyTimeClass::GetKeyTimeFormat();
	}

	void SetPosKeyTimes(uint32 num, char * pData)
	{
		this->SetKeyTimes(num, pData);
	}

	char * GetPositionData()
	{
		return (char *)m_arrKeys;
	}

	char * GetPosKeyTimes()
	{
		return this->GetData();
	}

	size_t GetPositionsSize() const {
		return sizeof(T) * GetPositionNumCount()  + this->GetKeyTimeSize();
	}


private:

	Vec3 GetValue ( f32 normalized_time)
	{
		//DEFINE_PROFILER_SECTION("ControllerPQ::GetValue");
		DEFINE_ALIGNED_DATA (Vec3, pos, 16);

		//		Vec3 pos;

		f32 t;
		uint32 key = this->GetKey( normalized_time, t);

		if (key == 0)
		{
			GetValueFromKey(0, pos);
		}
		else
			if (key >= GetPositionNumCount())
			{
				GetValueFromKey(key-1, pos);
			}
			else
			{
				DEFINE_ALIGNED_DATA (Vec3, p1, 16);
				DEFINE_ALIGNED_DATA (Vec3, p2, 16);

				GetValueFromKey(key-1, p1);
				GetValueFromKey(key, p2);

				_Interpolator::Blend(pos, p1, p2, t);
			}

			return pos;
	}

	ILINE void GetValueFromKey(uint32 key, Vec3& val)
	{
		m_arrKeys[key].ToExternalType(val);
	}

private:
	T * m_arrKeys;

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
typedef CPositionData<Vec3, CKeyTimesUINT16> CPositionNoCompressKeysF32;
typedef CPositionData<uint16, CKeyTimesUINT16> CPositionNoCompressKeysUINT16;
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class IControllerOpt : public IController
{
public:
	virtual void SetRotationKeyTimes(uint32 num, char * pData) = 0;
	virtual void SetPositionKeyTimes(uint32 num, char * pData) = 0;
	virtual void SetRotationKeys(uint32 num, char * pData) = 0;
	virtual void SetPositionKeys(uint32 num, char * pData) = 0;

	virtual uint32 GetRotationFormat() = 0;
	virtual uint32 GetRotationKeyTimesFormat() = 0;
	virtual uint32 GetPositionFormat() = 0;
	virtual uint32 GetPositionKeyTimesFormat() = 0;

	virtual char * GetRotationKeyData() = 0;
	virtual char * GetRotationKeyTimesData() = 0;
	virtual char * GetPositionKeyData() = 0;
	virtual char * GetPositionKeyTimesData() = 0;

};

TYPEDEF_AUTOPTR(IControllerOpt);

template <class _PosController, class _RotController>
class CControllerOpt : public IControllerOpt, _PosController, _RotController
{
public:
	//Creation interface

	CControllerOpt(){}

	~CControllerOpt(){}

	uint32 numKeys() const
	{
		// now its hack, because num keys might be different
		return max(this->GetRotationNumCount(), this->GetPositionNumCount());
	}

	Status4 GetOPS ( f32 normalized_time, Quat& quat, Vec3& pos, Diag33& scale)
	{
		Status4 res;
		res.o = GetO( normalized_time, quat) ? 1 : 0;
		res.p = GetP( normalized_time, pos) ? 1 : 0;
		res.s = GetS( normalized_time, scale) ? 1: 0;

		return res; 
	}

	Status4 GetOP( f32 normalized_time, Quat& quat, Vec3& pos)
	{
		Status4 res;
		res.o = GetO( normalized_time, quat) ? 1 : 0;
		res.p = GetP( normalized_time, pos) ? 1 : 0;
		return res; 
	}

	uint32 GetO( f32 normalized_time, Quat& quat)
	{
		return this->GetRotationValue( normalized_time, quat);
	}

	uint32 GetP( f32 normalized_time, Vec3& pos)
	{
		return this->GetPositionValue( normalized_time, pos);
	}

	uint32 GetS( f32 normalized_time, Diag33& pos)
	{
		return 0;
	}


	uint32 HasPositionChannel()
	{
		return 0;
	}


	// returns the start time
	size_t SizeOfController() const
	{
		size_t res(sizeof(*this));
		res += this->GetRotationsSize();
		res += this->GetPositionsSize();
		return  res;
	}

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		//static_cast<_PosController*>(this)->_PosController::GetMemoryUsage(pSizer);
		//static_cast<_RotController*>(this)->_RotController::GetMemoryUsage(pSizer);
	}

	size_t ApproximateSizeOfThis() const 
	{
		size_t res(sizeof(*this));
		res += this->GetRotationsSize();
		res += this->GetPositionsSize();
		return  res;
	}

	size_t GetRotationKeysNum()
	{
		return this->GetRotationNumCount();
	}

	size_t GetPositionKeysNum()
	{
		return this->GetPositionNumCount();
	}	


	CInfo GetControllerInfo() const
	{
		CInfo info;
		info.m_numKeys=0;
		/*
		//		info.numKeys = numKeys();
		info.numKeys = GetRotationNumKeys();
		info.quat = GetRotationValueFromKey(info.numKeys - 1);

		info.etime	 = _RotController::GetKeyValueFloat(info.numKeys - 1);//(int)m_pRotationController->GetTimeFromKey(info.numKeys - 1);
		info.stime	 = _RotController::GetKeyValueFloat(0);


		if (m_pPositionController)
		{
		info.numKeys = m_pPositionController->GetNumKeys();
		m_pPositionController->GetValueFromKey(info.numKeys - 1, info.pos);
		info.etime	 = (int)m_pPositionController->GetTimeFromKey(info.numKeys - 1);
		info.stime	 = (int)m_pPositionController->GetTimeFromKey(0);
		}


		info.realkeys = (info.etime-info.stime)+1;
		*/
		return info;
	}

	virtual EControllerInfo GetControllerType() 
	{
		return eControllerOpt;
	}

	void SetRotationKeyTimes(uint32 num, char * pData)
	{
		this->SetRotKeyTimes/*_RotController::SetKeyTimes*/(num, pData);
	}

	void SetPositionKeyTimes(uint32 num, char * pData)
	{
		this->SetPosKeyTimes/*_PosController::SetKeyTimes*/(num, pData);
	}

	void SetRotationKeys(uint32 num, char * pData)
	{
		this->SetRotationData(num, pData);
	}

	void SetPositionKeys(uint32 num, char * pData)
	{
		this->SetPositionData(num, pData);
	}


	uint32 GetRotationFormat()
	{
		return this->GetRotationType();
	}

	uint32 GetRotationKeyTimesFormat()
	{
		return this->GetRotationKeyTimeFormat();
	}

	uint32 GetPositionFormat()
	{
		return this->GetPositionType();
	}

	uint32 GetPositionKeyTimesFormat()
	{
		return this->GetPositionKeyTimeFormat();
	}

	char * GetRotationKeyData()
	{
		return this->GetRotationData();
	}

	char * GetRotationKeyTimesData()
	{
		return this->GetRotKeyTimes();
	}

	char * GetPositionKeyData()
	{
		return this->GetPositionData();
	}

	char * GetPositionKeyTimesData()
	{
		return this->GetPosKeyTimes();
	}

	IControllerOpt * CreateController()
	{
		return (IControllerOpt *)new CControllerOpt<_PosController, _RotController>();
	}


	//static uint32 CalculateHash()
	//{
	//	uint32 hash =  ControllerHash(GetRotationType(), GetRotationKeyTimeFormat(), GetPositionType(), GetPositionKeyTimeFormat());
	//	return hash;
	//};


};

//template <class _PosController, class _RotController>
//uint32  CControllerOpt<_PosController, _RotController>::CalculateHash()
//{
//	uint32 hash =  ControllerHash(GetRotationType(), _RotController::GetKeyTimeFormat(), GetPositionType(), _PosController::GetKeyTimeFormat());
//	return hash;
//}


typedef class CControllerOpt<CEmptyPositionController, CEmptyRotationController> CEmptyController;


template<class T>
class CControllerFactoryTpl
{
public:
	typedef T * (*CreateCallback)(int);
private:
	typedef std::map<uint32, CreateCallback> CallbackMap;
public:
	//	CControllerFactoryTpl() {};
	bool Register(uint32 Name, CreateCallback CreateFn)
	{
		return callbacks_.insert(std::make_pair(Name, CreateFn)).second;
	}

	bool Unregister(uint32 Name)
	{
		return callbacks_.erase(Name) == 1;
	}

	T * Create(uint32 Name, int count = 1)
	{
		typename CallbackMap::const_iterator i = callbacks_.find(Name);
		if (i == callbacks_.end())
		{
			return NULL;
		}
		return (i->second)(count);
	}

private:
	CallbackMap callbacks_;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef class CControllerFactoryTpl<IControllerOpt> CControllerFactory;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
class CSimpleSingleton 
{
public:
	static T& GetInstance()
	{
		static T inst;
		return inst;
	}

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef CSimpleSingleton<CControllerFactory> CControllerFactoryInst;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define REGISTER_CLASSINFO(ClassName, Id) \
	namespace \
{ \
	IControllerOpt * Create##ClassName(int count) \
{ \
	if (count < 2) \
	return new ClassName; \
	else \
	return new ClassName[count]; \
} \
struct Reg##ClassName \
{ \
	bool bRegistered; \
	Reg##ClassName() \
{ \
	bRegistered = CControllerFactoryInst::GetInstance().Register(Id, Create##ClassName); \
}; \
}; \
	static Reg##ClassName m_reg##ClassName; \
} \

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//ControllerHash(GetRotationType(), GetRotationKeyTimeFormat(), GetPositionType(), GetPositionKeyTimeFormat());
#define REGISTER_COMBINATION(ROTKT,ROTINT,POSKT, POSINT) \
	namespace \
{ \
	typedef class CControllerOpt< CRotationData<ROTKT,ROTINT> , CPositionData<POSKT, POSINT> > CController##ROTKT##ROTINT##POSKT##POSINT;  \
	REGISTER_CLASSINFO(CController##ROTKT##ROTINT##POSKT##POSINT, ControllerHash(CRotationData<ROTKT,ROTINT>::GetRotationType(), CRotationData<ROTKT,ROTINT>::GetRotationKeyTimeFormat(), \
	CPositionData<POSKT, POSINT>::GetPositionType(), CPositionData<POSKT, POSINT>::GetPositionKeyTimeFormat())); \
}; 


//CController##ROTKT##ROTINT##POSKT##POSINT::CalculateHash()); \
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define REGISTER_COMBINATION_NO_POS(ROTKT,ROTINT) \
	namespace \
{ \
	typedef class CControllerOpt< CRotationData<ROTKT,ROTINT> , CEmptyPositionController > CController##ROTKT##ROTINT;  \
	REGISTER_CLASSINFO(CController##ROTKT##ROTINT, ControllerHash(CRotationData<ROTKT,ROTINT>::GetRotationType(), CRotationData<ROTKT,ROTINT>::GetRotationKeyTimeFormat(), \
	CEmptyPositionController::GetPositionType(), CEmptyPositionController::GetPositionKeyTimeFormat())); \
}; 


//CController##ROTKT##ROTINT::CalculateHash()); \
//}; \

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define REGISTER_COMBINATION_NO_ROT(POSKT, POSINT) \
	namespace \
{ \
	typedef class CControllerOpt< CEmptyRotationController , CPositionData<POSKT, POSINT> > CController##POSKT##POSINT;  \
	REGISTER_CLASSINFO(CController##POSKT##POSINT, ControllerHash(CEmptyRotationController::GetRotationType(), CEmptyRotationController::GetRotationKeyTimeFormat(), \
	CPositionData<POSKT, POSINT>::GetPositionType(), CPositionData<POSKT, POSINT>::GetPositionKeyTimeFormat())); \
}; 


//CController##POSKT##POSINT::CalculateHash()); \
//}; \

//CKeyTimesDataBitSet
// Special case! :-)
typedef class CControllerOpt< CEmptyRotationController , CEmptyPositionController > CControllerEmpty;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
typedef CKeyTimesData<ByteEncoder> CKeyTimesByte;
typedef CKeyTimesData<UINT16Encoder> CKeyTimesUINT16;
typedef CKeyTimesData<F32Encoder> CKeyTimesF32;

struct QuatLerpNoCompression
struct QuatLerpSmallTree48BitQuat
struct QuatLerpSmallTree64BitQuat
struct QuatLerpSmallTree64BitExtQuat

Vec3LerpF32
*/


// copy-paste rules!!!
#if !defined(__CRYCG__)
REGISTER_COMBINATION_NO_ROT(CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION_NO_ROT(CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION_NO_ROT(CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION_NO_ROT(CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION_NO_POS(CKeyTimesByte,QuatLerpNoCompression);
REGISTER_COMBINATION_NO_POS(CKeyTimesByte,QuatLerpSmallTree48BitQuat);
REGISTER_COMBINATION_NO_POS(CKeyTimesByte,QuatLerpSmallTree64BitQuat);
REGISTER_COMBINATION_NO_POS(CKeyTimesByte,QuatLerpSmallTree64BitExtQuat);

REGISTER_COMBINATION_NO_POS(CKeyTimesUINT16,QuatLerpNoCompression);
REGISTER_COMBINATION_NO_POS(CKeyTimesUINT16,QuatLerpSmallTree48BitQuat);
REGISTER_COMBINATION_NO_POS(CKeyTimesUINT16,QuatLerpSmallTree64BitQuat);
REGISTER_COMBINATION_NO_POS(CKeyTimesUINT16,QuatLerpSmallTree64BitExtQuat);

REGISTER_COMBINATION_NO_POS(CKeyTimesF32,QuatLerpNoCompression);
REGISTER_COMBINATION_NO_POS(CKeyTimesF32,QuatLerpSmallTree48BitQuat);
REGISTER_COMBINATION_NO_POS(CKeyTimesF32,QuatLerpSmallTree64BitQuat);
REGISTER_COMBINATION_NO_POS(CKeyTimesF32,QuatLerpSmallTree64BitExtQuat);

REGISTER_COMBINATION_NO_POS(CKeyTimesBitset,QuatLerpNoCompression);
REGISTER_COMBINATION_NO_POS(CKeyTimesBitset,QuatLerpSmallTree48BitQuat);
REGISTER_COMBINATION_NO_POS(CKeyTimesBitset,QuatLerpSmallTree64BitQuat);
REGISTER_COMBINATION_NO_POS(CKeyTimesBitset,QuatLerpSmallTree64BitExtQuat);



REGISTER_COMBINATION(CKeyTimesByte,QuatLerpNoCompression,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpNoCompression,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpNoCompression,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpNoCompression,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree48BitQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree48BitQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree48BitQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree48BitQuat,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree64BitQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree64BitQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree64BitQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree64BitQuat,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree64BitExtQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree64BitExtQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree64BitExtQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesByte,QuatLerpSmallTree64BitExtQuat,CKeyTimesBitset,Vec3LerpF32);



REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpNoCompression,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpNoCompression,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpNoCompression,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpNoCompression,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree48BitQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree48BitQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree48BitQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree48BitQuat,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree64BitQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree64BitQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree64BitQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree64BitQuat,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree64BitExtQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree64BitExtQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree64BitExtQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesUINT16,QuatLerpSmallTree64BitExtQuat,CKeyTimesBitset,Vec3LerpF32);



REGISTER_COMBINATION(CKeyTimesF32,QuatLerpNoCompression,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpNoCompression,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpNoCompression,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpNoCompression,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree48BitQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree48BitQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree48BitQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree48BitQuat,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree64BitQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree64BitQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree64BitQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree64BitQuat,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree64BitExtQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree64BitExtQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree64BitExtQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesF32,QuatLerpSmallTree64BitExtQuat,CKeyTimesBitset,Vec3LerpF32);



REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpNoCompression,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpNoCompression,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpNoCompression,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpNoCompression,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree48BitQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree48BitQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree48BitQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree48BitQuat,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree64BitQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree64BitQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree64BitQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree64BitQuat,CKeyTimesBitset,Vec3LerpF32);

REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree64BitExtQuat,CKeyTimesByte,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree64BitExtQuat,CKeyTimesUINT16,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree64BitExtQuat,CKeyTimesF32,Vec3LerpF32);
REGISTER_COMBINATION(CKeyTimesBitset,QuatLerpSmallTree64BitExtQuat,CKeyTimesBitset,Vec3LerpF32);
#endif // !__CRYCG__


// forward declarations
struct ControllerData;
static uint32	GetKeySelector( f32 normalized_time, f32& difference_time, ControllerData &rConData);


struct ControllerData
{
	ControllerData( int iType, int iKeyType ) :
m_pData(0),
m_arrKeys(0),
m_iCount(0),
m_eTimeFormat(iKeyType),
m_eCompressionType(iType)
{
}

// call function to select template implementation of GetKeyData	
uint32	GetKey( f32 normalized_time, f32& difference_time)
{			
	return GetKeySelector( normalized_time, difference_time, *this );

}

template<typename Type>
uint32	GetKeyByteData( f32 normalized_time, f32& difference_time, void* p_data)
{
	Type* data = reinterpret_cast<Type*>(p_data);

	f32 realtimef = normalized_time;
	Type realtime = (Type)realtimef;

	uint32 numKey = GetNumCount();

	Type keytime_start = data[0];
	Type keytime_end = data[numKey-1];			

	if( realtime < keytime_start )
	{
		return 0;
	}

	if( realtime >= keytime_end )
	{
		return numKey;
	}

	//-------------
	int nPos  = numKey>>1;
	int nStep = numKey>>2;

	// use binary search
	//TODO: Need check efficiency of []operator. Maybe wise use pointer
	while(nStep)
	{
		if(realtime < data[nPos])
			nPos = nPos - nStep;
		else
			if(realtime > data[nPos])
				nPos = nPos + nStep;
			else 
				break;

		nStep = nStep>>1;
	}

	// fine-tuning needed since time is not linear
	while(realtime >= data[nPos])
		nPos++;

	while(realtime < data[nPos-1])
		nPos--;

	// possible error if encoder uses nonlinear methods!!!
	if( data[nPos] == data[nPos-1] )
	{
		difference_time = 0.0f;
	}
	else
	{
		f32 prevtime = (f32)data[nPos-1];
		f32 time = (f32)data[nPos];
		difference_time = (realtimef - prevtime) / (time - prevtime);
	}

	assert( difference_time >= 0.0f && difference_time <= 1.0f );
	return nPos;
}


uint32	GetKeyBitData( f32 normalized_time, f32& difference_time )
{
	f32 realtime = normalized_time;

	uint32 numKey = (uint32)GetHeader()->m_Size;//m_arrKeys.size();

	f32 keytime_start = (float)GetHeader()->m_Start;
	f32 keytime_end = (float)GetHeader()->m_End;
	f32 test_end = keytime_end;

	if( realtime < keytime_start )
		test_end += realtime;

	if( realtime < keytime_start ) {
		difference_time = 0;
		return 0;
	}

	if( realtime >= keytime_end ) {
		difference_time = 0;
		return numKey;
	}

	f32 internalTime = realtime - keytime_start;
	uint16 uTime = (uint16)internalTime;
	uint16 piece = (uTime / sizeof(uint16)) >> 3;
	uint16 bit = /*15 - */ (uTime % 16);
	uint16 data = GetKeyData(piece);
	uint16 left = data >> bit;

	//left
	left = data & (0xFFFF >> (15 - bit));
	uint16 leftPiece(piece);
	uint16 nearestLeft=0;
	uint16 wBit;

	while ((wBit = GetFirstHighBit(left)) == 16) {
		--leftPiece;
		left = GetKeyData(leftPiece);
	}
	nearestLeft = leftPiece * 16 + wBit;

	//right
	uint16 right = ((data >> (bit + 1)) & 0xFFFF) << (bit + 1);
	uint16 rigthPiece(piece);
	uint16 nearestRight=0;

	while ((wBit = GetFirstLowBit(right)) == 16) {
		++rigthPiece;
		right = GetKeyData(rigthPiece);
	}

	nearestRight = ((rigthPiece  * sizeof(uint16)) << 3) + wBit;
	difference_time = (f32)(internalTime-(f32)nearestLeft) / ((f32)nearestRight - (f32)nearestLeft);

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

	return nPos - 1;
}

// util functions for bitset encoding
struct Header 
{
	uint16		m_Start;
	uint16    m_End;
	uint16		m_Size;
};

inline Header* GetHeader() const
{
	uint16 * pData = reinterpret_cast<uint16*>(m_pData);
	return (Header*)(&pData[0]);
};

inline uint16 GetKeyData(int i ) const
{
	uint16 * pData = reinterpret_cast<uint16*>(m_pData);
	return pData[3 + i];
};

size_t GetKeysNum() const	{ return GetNumCount(); }
size_t GetNumCount() const 	{ return static_cast<size_t>(getTimeFormat() == eBitset ? GetHeader()->m_Size : m_iCount); }

EKeyTimesFormat getTimeFormat() const { return static_cast<EKeyTimesFormat>(m_eTimeFormat); }
ECompressionInformation getCompressionType() const { return static_cast<ECompressionInformation>(m_eCompressionType); }

void GetMemoryUsage( ICrySizer *pSizer ) const 
{
	pSizer->AddObject(this, sizeof(*this));
}
char* m_pData;
char* m_arrKeys;

uint16 m_iCount; 

// using unsigned chars to store enums to save storage space
unsigned char m_eTimeFormat;
unsigned char m_eCompressionType;


};


class CControllerOptNonVirtual : public IControllerOpt
{
public:

	// ============ new interface ===========//
public:
	CControllerOptNonVirtual(int iRotType, int iRotKeyType, int iPosType, int iPosKeyType ) :
			m_position(iPosType, iPosKeyType ),
				m_rotation(iRotType, iRotKeyType )
			{		
			}

			~CControllerOptNonVirtual(){}

			// returns the orientation,position and scaling of the controller at the given time
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
				res.o = this->CControllerOptNonVirtual::GetO( normalized_time, quat) ? 1 : 0;
				res.p = this->CControllerOptNonVirtual::GetP( normalized_time, pos) ? 1 : 0;
				return res; 
			}

			SPU_INDIRECT(CommandBufferExecute(ML))
				uint32 GetO ( f32 normalized_time, Quat& quat)
			{		
				if( eNoFormat == m_rotation.getTimeFormat() ) return 0;

				quat = this->CControllerOptNonVirtual::GetRotValue( normalized_time);

				return STATUS_O;
			}

			SPU_NO_INLINE uint32 GetP( f32 normalized_time, Vec3& pos)
			{

				if( eNoFormat == m_position.getTimeFormat() ) return 0;

				pos = this->CControllerOptNonVirtual::GetPosValue( normalized_time);

				return STATUS_P;
			}

			uint32 GetS ( f32 normalized_time, Diag33& pos) { return 0; /* ???? */ }

			Vec3 GetPosValue ( f32 normalized_time)
			{
				//DEFINE_PROFILER_SECTION("ControllerPQ::GetValue");
				DEFINE_ALIGNED_DATA (Vec3, pos, 16);

				f32 t;
				uint32 key = m_position.GetKey( normalized_time, t); 

				IF (key == 0, true)
				{
					CryPrefetch( &m_position.m_arrKeys[0] );
					GetPosValueFromKey(0, pos);
				}
		else 
		{
			IF( key < m_position.GetNumCount(), true )
			{
				// assume that the 48bit(6byte) encodings are used(can be wrong but should be right the most time)
				CryPrefetch( &m_position.m_arrKeys[(key-1) * 6] );
				CryPrefetch( &m_position.m_arrKeys[key * 6] );

				DEFINE_ALIGNED_DATA (Vec3, p1, 16);
				DEFINE_ALIGNED_DATA (Vec3, p2, 16);

				GetPosValueFromKey(key-1, p1);
				GetPosValueFromKey(key, p2);

				pos.SetLerp(p1, p2, t); 
			}
				else
				{		
					GetPosValueFromKey(m_position.GetNumCount()-1, pos);
				}
		}

		return pos;
			}


			Quat GetRotValue ( f32 normalized_time)
			{
				//DEFINE_PROFILER_SECTION("ControllerPQ::GetValue");
				DEFINE_ALIGNED_DATA (Quat, pos, 16);

				f32 t;
				uint32 key = m_rotation.GetKey( normalized_time, t  );

				IF (key == 0, true)
				{
					CryPrefetch( &m_rotation.m_arrKeys[0] );

					DEFINE_ALIGNED_DATA (Quat, p1, 16);
					GetRotValueFromKey(0, p1);
					pos = p1;

				}
		else
		{
			IF( key < m_rotation.GetNumCount(), true )
			{
				// assume that the 48bit(6byte) encodings are used(can be wrong but should be right the most time)
				CryPrefetch( &m_rotation.m_arrKeys[(key-1) * 6] );
				CryPrefetch( &m_rotation.m_arrKeys[key * 6] );

				//	Quat p1, p2;
				DEFINE_ALIGNED_DATA (Quat, p1, 16);
				DEFINE_ALIGNED_DATA (Quat, p2, 16);

				GetRotValueFromKey(key-1, p1);
				GetRotValueFromKey(key, p2);

				pos.SetNlerp(p1, p2, t);
			}
				else 
				{  				  				
					DEFINE_ALIGNED_DATA (Quat, p1, 16);
					assert(key - 1 < m_rotation.GetNumCount());
					GetRotValueFromKey( m_rotation.GetNumCount()-1, p1);
					pos = p1;
				}

		}


		return pos;
			}

			template<typename CompressionType, typename ValueType>
			void load_value( uint32 key, char *data, ValueType &val )
			{
				// on spu transfer Compression struct to stack to spare lookups during value load
#if defined(__SPU__)		

				CompressionType *p = reinterpret_cast<CompressionType*>(data);

				CompressionType obj = p[key];

				obj.ToExternalType(val);
#else
				CompressionType *p = reinterpret_cast<CompressionType*>(data);
				p[key].ToExternalType( val );
#endif

			}


			void GetRotValueFromKey(uint32 key, Quat& val)
			{
				ECompressionInformation format = m_rotation.getCompressionType();

				// branches ordered by probability
				IF( format == eSmallTree48BitQuat, true )
				{
					load_value<SmallTree48BitQuat>( key, m_rotation.m_arrKeys, val );
				}
		else
		{
			IF( format == eSmallTree64BitExtQuat, true)
			{
				load_value<SmallTree64BitExtQuat>( key, m_rotation.m_arrKeys, val );				
			}
				else
				{
					IF( format == eSmallTree64BitQuat, true )
					{
						load_value<SmallTree64BitQuat>( key, m_rotation.m_arrKeys, val );					
					}
					else
					{
						IF( format == eNoCompressQuat, true )
						{
							load_value<NoCompressQuat>( key, m_rotation.m_arrKeys, val );
						}
						else
						{
							CryFatalError("Unknown Rotation Compression format %i\n", m_rotation.getCompressionType());
							snPause();//spus should stop if encounter this, will cause DMA crash
						}
					}
				}
			}		

			}

			void GetPosValueFromKey(uint32 key, Vec3& val)
			{
				// branches ordered by probability
				IF( m_position.getCompressionType() == eNoCompressVec3, 1)
				{
					load_value<NoCompressVec3>( key, m_position.m_arrKeys, val );
				}
		else
		{
			val=ZERO;
			//CryFatalError("Unknown Position Compression format %i", m_position.getCompressionType());
		}
			}

			uint32 HasPositionChannel()
			{
				DEFINE_ALIGNED_DATA_STATIC (Vec3, t, 16);
				if( eNoFormat == m_rotation.getTimeFormat()) return 0;
				GetPosValueFromKey(0, t);
				return 0;
			}


			void SetRotationKeyTimes(uint32 num, char * pData)
			{
				if( eNoFormat == m_rotation.getTimeFormat() ) return;

				m_rotation.m_iCount = static_cast<uint16>(num);
				m_rotation.m_pData = pData;

			}

			void SetPositionKeyTimes(uint32 num, char * pData)
			{
				if( eNoFormat == m_position.getTimeFormat() ) return;

				m_position.m_iCount = static_cast<uint16>(num);
				m_position.m_pData = pData;

			}

			void SetRotationKeys(uint32 num, char * pData)
			{
				if( eNoFormat == m_rotation.getTimeFormat() ) return;

				m_rotation.m_arrKeys = pData;

			}

			void SetPositionKeys(uint32 num, char * pData)
			{
				if( eNoFormat == m_position.getTimeFormat() ) return;

				m_position.m_arrKeys = pData;

			}


			uint32 GetRotationFormat()					{ return m_rotation.getCompressionType(); }
			uint32 GetRotationKeyTimesFormat()	{ return m_rotation.getTimeFormat(); }

			uint32 GetPositionFormat() 					{ return m_position.getCompressionType(); }
			uint32 GetPositionKeyTimesFormat() 	{ return m_position.getTimeFormat(); }

			char * GetRotationKeyData() 				{ return static_cast<char*>(m_rotation.m_arrKeys); }
			char * GetRotationKeyTimesData() 		{ return static_cast<char*>(m_rotation.m_pData); }

			char * GetPositionKeyData() 				{ return static_cast<char*>(m_position.m_arrKeys); }
			char * GetPositionKeyTimesData() 		{ return static_cast<char*>(m_position.m_pData); }

			size_t SizeOfController() const		
			{ 
				return sizeof(*this); 
			}
			size_t ApproximateSizeOfThis ()const					
			{
				int sizeOfRotKey = ControllerHelper::GetRotationFormatSizeOf(m_rotation.getCompressionType());
				int sizeOfPosKey = ControllerHelper::GetPositionsFormatSizeOf(m_position.getCompressionType());
			
				return sizeof(*this) + sizeOfRotKey * m_rotation.GetNumCount() + sizeOfPosKey* m_position.GetNumCount() ;
			}
			CInfo GetControllerInfo() const 		{ CInfo info; info.m_numKeys=0;	return info; }		
			//	EControllerInfo GetControllerType();


			virtual size_t GetRotationKeysNum()	{ return m_rotation.GetNumCount(); }
			virtual size_t GetPositionKeysNum() { return m_position.GetNumCount(); }

			IController * CreateController() 
			{ 
				return new CControllerOptNonVirtual( m_rotation.getTimeFormat(),	m_rotation.getCompressionType(),
					m_position.getTimeFormat(),	m_position.getCompressionType() );
			}

			EControllerInfo GetControllerType();

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	ControllerData m_rotation;
	ControllerData m_position;

};


TYPEDEF_AUTOPTR(CControllerOptNonVirtual);

static uint32	GetKeySelector( f32 normalized_time, f32& difference_time, ControllerData &rConData)
{
#if defined(__SPU__)
	typedef vec_uint4 TransferType;
	uint32 numCount = rConData.GetNumCount();
	uint32 max_needed_mem = (numCount * sizeof(f32)) + sizeof(TransferType);
	void* data = SPU_LOCAL_PTR( alloca( max_needed_mem ) );
#else
	void* data = rConData.m_pData;
#endif

	EKeyTimesFormat format = rConData.getTimeFormat();

	// branches ordered by probability
	IF( format == eByte, true )
	{
#if defined(__SPU__)
		memcpy( data, rConData.m_pData, sizeof(byte) * numCount );
#endif
		return rConData.GetKeyByteData<byte>( normalized_time, difference_time, data );
	}
	else
	{
		IF( format == eUINT16, 1 )
		{
#if defined(__SPU__)
			memcpy( data, rConData.m_pData, sizeof(uint16) * numCount );
#endif
			return rConData.GetKeyByteData<uint16>( normalized_time, difference_time, data );
		}
			else
			{
				IF( format == eF32, 1 )
				{
#if defined(__SPU__)
					memcpy( data, rConData.m_pData, sizeof(f32) * numCount );
#endif		
					return rConData.GetKeyByteData<f32>( normalized_time, difference_time, data );
				}
				else
				{
					return rConData.GetKeyBitData( normalized_time, difference_time );
				}
			}
	}
}

// wrapper functions for ps3(depraced, will be removed soon)
// on ps3 the access to the controller functions is devirtualiezed and all controllers of the wrong type are removed
// on other architectures is this not the case since this stops some animations from working in the editor (they are filtered out)
/*ILINE Status4 ControllerGetOP( IController *pController, int GAID, f32 normalized_time, Quat& quat, Vec3& pos)
{
	// nrvo required to prevent reftacker warning (call expression in
	// memory reference)
	Status4 result; 
	result = SPU_MAIN_PTR(pController)->GetOP( normalized_time, quat, pos);
	return result;
}*/

// wrapper function for controller, on spu copy these into a local storage to spare lookups
ILINE IController* LoadController( GlobalAnimationHeaderCAF &rGlobalAnimHeader, uint32 nControllerID)
{
	return rGlobalAnimHeader.GetControllerByJointCRC32( nControllerID );
}

// util function for spus, transfers the SPU lookuptable for controllers
// to spu local storage
// on non-spu system this funciton evaluates to nothing
void InitSpuControllerLookupTable( const DynArray<uint32> &lookupTable ); 


#endif//_CRYTEK_CONTROLLER_OPT_PQ_

