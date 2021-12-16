#ifndef __BOOLCOMPRESS2_H__
#define __BOOLCOMPRESS2_H__

#pragma once

#include "Streams/ByteStream.h"
#include "Streams/CommStream.h"

class CBoolCompress2
{
public:
	CBoolCompress2() {}

	struct TMemento
	{
		uint8 prob;
		bool lastValue;
	};

	static const int AdaptBits = 5;
	static const uint8 StateRange = (1u<<AdaptBits)-1;
	static const uint8 StateMidpoint = (1u<<(AdaptBits-1))-1;

	void ReadMemento( TMemento& memento, CByteInputStream& in ) const;
	void WriteMemento( TMemento memento, CByteOutputStream& out ) const;
	void InitMemento( TMemento& memento ) const;

	void UpdateMemento( TMemento& memento, bool newValue ) const;
	bool ReadValue( TMemento memento, CCommInputStream& in, bool& value ) const;
	bool WriteValue( TMemento memento, CCommOutputStream& out, bool value ) const;
#if NET_PROFILE_ENABLE
	int GetBitCount();
#endif

private:
	static const uint8 LAST_VALUE_BIT = 0x80;

	struct SSymLow;
	SSymLow GetSymLow( bool isLastValue, uint8 prob ) const;
};

#endif
