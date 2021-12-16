#pragma once

#ifndef __AISALTHANDLE
#define __AISALTHANDLE

// This is copy/pasted from the entity system for the sake of some small changes
// That code might well move into CryCommon to service this


// handle can be ==0 nil, or !=0 to store a reference to an object
// this object can be valid or invalid
template <class TSalt=unsigned short, class TIndex=unsigned short>
class CAISaltHandle
{
public:
	// default constructor (set to invalid handle)
	CAISaltHandle()
	{
		SetNil();
	}

	// constructor (set to invalid handle)
	CAISaltHandle( const unsigned int dwValue )
	{
		assert(dwValue==0);
		SetNil();
	}

	// constructor
	CAISaltHandle( TSalt Salt, TIndex Index ) 
		:m_Index(Index), m_Salt(Salt)
	{
	}

	// comparison operator
	bool operator==( const CAISaltHandle<TSalt,TIndex> &rhs ) const
	{
		return m_Salt==rhs.m_Salt && m_Index==rhs.m_Index;
	}

	// comparison operator
	bool operator!=( const CAISaltHandle<TSalt,TIndex> &rhs ) const
	{
		return !(*this==rhs);
	}

	// conversion to bool
	// Isn't this documented the wrong way around?
	// e.g. if(id){ ..nil.. } else { ..valid or not valid.. }
	operator bool() const
	{
		return m_Salt!=0 || m_Index!=0;
	}

	// useful for logging/debugging 
	// and getting a small index to cache data for an entity
	// (check with GetSalt() if the cache element is still valid)
	TIndex GetIndex() const
	{
		return m_Index;
	}

	// used by the SaltBufferArray<>
	TSalt GetSalt() const
	{
		return m_Salt;
	}

	// handle can be nil, or store a reference to an object
	// Exposed this - can't see why it can't be public
	void SetNil()
	{
		m_Salt=0;m_Index=0;
	}

private: // --------------------------------------------------------------

	TSalt										m_Salt;									// 1.. is counting up on every remove, should never wrap around, 0 means invlid handle
	TIndex									m_Index;								// Index in CSaltBufferArray
};


#endif // __AISALTHANDLE