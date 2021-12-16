/*************************************************************************
 Crytek Source File.
 Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
 $Id$
 $DateTime$
 Description:  splits a compression policy into two pieces, one for the
               witness, and another for all other clients
 -------------------------------------------------------------------------
 History:
 - 02/11/2006   12:34 : Created by Craig Tiller
*************************************************************************/

#include "StdAfx.h"
#include "OwnChannelCompressionPolicy.h"

#define SERIALIZATION_TYPE(T) \
	bool COwnChannelCompressionPolicy::ReadValue( CCommInputStream& in, T& value, CArithModel * pModel, uint32 age, bool own, CByteInputStream * pCurState, CByteOutputStream * pNewState ) const \
	{ \
		return Get(own)->ReadValue(in, value, pModel, age, own, pCurState, pNewState); \
	} \
	bool COwnChannelCompressionPolicy::WriteValue( CCommOutputStream& out, T value, CArithModel * pModel, uint32 age, bool own, CByteInputStream * pCurState, CByteOutputStream * pNewState ) const \
	{ \
		return Get(own)->WriteValue(out, value, pModel, age, own, pCurState, pNewState); \
	}
#include "SerializationTypes.h"
#undef SERIALIZATION_TYPE

bool COwnChannelCompressionPolicy::ReadValue( CCommInputStream& in, SSerializeString& value, CArithModel * pModel, uint32 age, bool own, CByteInputStream * pCurState, CByteOutputStream * pNewState ) const
{
	return Get(own)->ReadValue(in, value, pModel, age, own, pCurState, pNewState);
}

bool COwnChannelCompressionPolicy::WriteValue( CCommOutputStream& out, const SSerializeString& value, CArithModel * pModel, uint32 age, bool own, CByteInputStream * pCurState, CByteOutputStream * pNewState ) const
{
	return Get(own)->WriteValue(out, value, pModel, age, own, pCurState, pNewState);
}

#if NET_PROFILE_ENABLE
// [MF] to the best of my knowledge, this policy is never used or
// created at present, so we return 0 and assert
#define SERIALIZATION_TYPE(T) \
	int COwnChannelCompressionPolicy::GetBitCount(T value) \
	{ \
		assert(0); \
		return 0; \
	}
#include "SerializationTypes.h"
#undef SERIALIZATION_TYPE

int COwnChannelCompressionPolicy::GetBitCount(SSerializeString& value)
{
	assert(0);
	return 0; 
}
#endif
