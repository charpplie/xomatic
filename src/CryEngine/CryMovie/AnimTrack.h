////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   animtrack.h
//  Version:     v1.00
//  Created:     22/4/2002 by Timur.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __animtrack_h__
#define __animtrack_h__

#if _MSC_VER > 1000
#pragma once
#endif

//forward declarations.
#include "IMovieSystem.h"

/** General templated track for event type keys.
		KeyType class must be derived from IKey.
*/
template <class KeyType>
class TAnimTrack : public IAnimTrack
{
public:
	TAnimTrack();

	virtual int GetSubTrackCount() const { return 0; };
	virtual IAnimTrack* GetSubTrack( int nIndex ) const { return 0; };
	virtual const char* GetSubTrackName(int nIndex) const { return NULL; };
	virtual void SetSubTrackName(int nIndex, const char *name) { assert(0); }

	virtual AnimParamType  GetParameterType() const { return m_nParamType; };
	virtual void SetParameterType( AnimParamType type ) { m_nParamType = type; };

	//////////////////////////////////////////////////////////////////////////
	virtual void Release() { if (--m_nRefCounter <= 0) delete this; }
	//////////////////////////////////////////////////////////////////////////

	virtual bool IsKeySelected(int key) const
	{
		assert( key >= 0 && key < (int)m_keys.size() );
		if(m_keys[key].flags & AKEY_SELECTED)
			return true;
		return false;
	}

	virtual void SelectKey(int key, bool select)
	{
		assert( key >= 0 && key < (int)m_keys.size() );
		if(select)
			m_keys[key].flags |= AKEY_SELECTED;
		else
			m_keys[key].flags &= ~AKEY_SELECTED;
	}

	//! Return number of keys in track.
	virtual int GetNumKeys() const { return m_keys.size(); };

	//! Set number of keys in track.
	//! If needed adds empty keys at end or remove keys from end.
	virtual void SetNumKeys( int numKeys ) { m_keys.resize(numKeys); };

	//! Remove specified key.
	virtual void RemoveKey( int num );

	int CreateKey( float time );
	int CloneKey( int fromKey );
	int CopyKey( IAnimTrack *pFromTrack, int nFromKey );

	//! Get key at specified location.
	//! @param key Must be valid pointer to compatible key structure, to be filled with specified key location.
	virtual void GetKey( int index,IKey *key );

	//! Get time of specified key.
	//! @return key time.
	virtual float GetKeyTime( int index ) const;

	//! Find key at given time.
	//! @return Index of found key, or -1 if key with this time not found.
	virtual int FindKey( float time );

	//! Get flags of specified key.
	//! @return key time.
	virtual int GetKeyFlags( int index );

	//! Set key at specified location.
	//! @param key Must be valid pointer to compatible key structure.
	virtual void SetKey( int index,IKey *key );

	//! Set time of specified key.
	virtual void SetKeyTime( int index,float time );

	//! Set flags of specified key.
	virtual void SetKeyFlags( int index,int flags );

	//! Sort keys in track (after time of keys was modified).
	virtual void SortKeys();

	//! Get track flags.
	virtual int GetFlags() { return m_flags; };
	
	//! Set track flags.
	virtual void SetFlags( int flags ) { m_flags = flags; };

	//////////////////////////////////////////////////////////////////////////
	// Get track value at specified time.
	// Interpolates keys if needed.
	//////////////////////////////////////////////////////////////////////////
	virtual void GetValue( float time,float &value ) { assert(0); };
	virtual void GetValue( float time,Vec3 &value ) { assert(0); };
	virtual void GetValue( float time,Quat &value ) { assert(0); };
	virtual void GetValue( float time,bool &value ) { assert(0); };

	//////////////////////////////////////////////////////////////////////////
	// Set track value at specified time.
	// Adds new keys if required.
	//////////////////////////////////////////////////////////////////////////
	virtual void SetValue( float time,const float &value,bool bDefault=false ) { assert(0); };
	virtual void SetValue( float time,const Vec3 &value,bool bDefault=false ) { assert(0); };
	virtual void SetValue( float time,const Quat &value,bool bDefault=false ) { assert(0); };
	virtual void SetValue( float time,const bool &value,bool bDefault=false ) { assert(0); };

	virtual void OffsetKeyPosition( const Vec3 &value ) { assert(0); };

	/** Assign active time range for this track.
	*/
	virtual void SetTimeRange( const Range &timeRange ) { m_timeRange = timeRange; };

	/** Serialize this animation track to XML.
			Do not override this method, prefer to override SerializeKey.
	*/
	virtual bool Serialize( XmlNodeRef &xmlNode,bool bLoading, bool bLoadEmptyTracks=true );

	virtual bool SerializeSelection( XmlNodeRef &xmlNode,bool bLoading, bool bCopySelected=false,float fTimeOffset=0);


	/** Serialize single key of this track.
			Override this in derived classes.
			Do not save time attribute, it is already saved in Serialize of the track.
	*/
	virtual void SerializeKey( KeyType &key,XmlNodeRef &keyNode,bool bLoading ) = 0;

	virtual void SetSelected( bool bSelect ) { if (bSelect) m_flags |= ATRACK_SELECTED; else m_flags &= ~ATRACK_SELECTED; }

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	/** Get last key before specified time.
			@return Index of key, or -1 if such key not exist.
	*/
	int GetActiveKey( float time,KeyType *key );

protected:
	void CheckValid()
	{
		if (m_bModified)
			SortKeys();
	};
	void Invalidate() { m_bModified = true; };

	typedef std::vector<KeyType> Keys;
	Keys m_keys;
	Range m_timeRange;

	AnimParamType m_nParamType;
	int m_currKey;
	float m_lastTime;
	int m_flags;
	bool m_bModified;
};

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline TAnimTrack<KeyType>::TAnimTrack()
{
	m_currKey = 0;
	m_flags = 0;
	m_lastTime = -1;
	m_bModified = false;
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline void TAnimTrack<KeyType>::RemoveKey( int index )
{
	assert( index >= 0 && index < (int)m_keys.size() );
	m_keys.erase( m_keys.begin() + index );
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline void TAnimTrack<KeyType>::GetKey( int index,IKey *key )
{
	assert( index >= 0 && index < (int)m_keys.size() );
	assert( key != 0 );
	*(KeyType*)key = m_keys[index];
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline void TAnimTrack<KeyType>::SetKey( int index,IKey *key )
{
	assert( index >= 0 && index < (int)m_keys.size() );
	assert( key != 0 );
	m_keys[index] = *(KeyType*)key;
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline float TAnimTrack<KeyType>::GetKeyTime( int index ) const
{
	assert( index >= 0 && index < (int)m_keys.size() );
	return m_keys[index].time;
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline void TAnimTrack<KeyType>::SetKeyTime( int index,float time )
{
	assert( index >= 0 && index < (int)m_keys.size() );
	m_keys[index].time = time;
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline int TAnimTrack<KeyType>::FindKey( float time )
{
	for (int i = 0; i < (int)m_keys.size(); i++)
	{
		if (m_keys[i].time == time)
			return i;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline int TAnimTrack<KeyType>::GetKeyFlags( int index )
{
	assert( index >= 0 && index < (int)m_keys.size() );
	return m_keys[index].flags;
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline void TAnimTrack<KeyType>::SetKeyFlags( int index,int flags )
{
	assert( index >= 0 && index < (int)m_keys.size() );
	m_keys[index].flags = flags;
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline void TAnimTrack<KeyType>::SortKeys()
{
	std::sort( m_keys.begin(),m_keys.end() );
	m_bModified = false;
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline bool TAnimTrack<KeyType>::Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks )
{
	if (bLoading)
	{
		int num = xmlNode->getChildCount();
		
		Range timeRange;
		int flags = m_flags;
		xmlNode->getAttr( "Flags",flags );
		xmlNode->getAttr( "StartTime",timeRange.start );
		xmlNode->getAttr( "EndTime",timeRange.end );
		SetFlags( flags );
		SetTimeRange(timeRange);

		SetNumKeys( num );
		for (int i = 0; i < num; i++)
		{
			XmlNodeRef keyNode = xmlNode->getChild(i);
			keyNode->getAttr( "time",m_keys[i].time );

			SerializeKey( m_keys[i],keyNode,bLoading );
		}

		if ((!num) && (!bLoadEmptyTracks))
			return false;
	}
	else
	{
		int num = GetNumKeys();
		CheckValid();
		xmlNode->setAttr( "Flags",GetFlags() );
		xmlNode->setAttr( "StartTime",m_timeRange.start );
		xmlNode->setAttr( "EndTime",m_timeRange.end );

		for (int i = 0; i < num; i++)
		{
			XmlNodeRef keyNode = xmlNode->newChild( "Key" );
			keyNode->setAttr( "time",m_keys[i].time );

			SerializeKey( m_keys[i],keyNode,bLoading );
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline bool TAnimTrack<KeyType>::SerializeSelection( XmlNodeRef &xmlNode,bool bLoading, bool bCopySelected,float fTimeOffset)
{
	if (bLoading)
	{
		int numCur = GetNumKeys();
		int num = xmlNode->getChildCount();

		int type;
		xmlNode->getAttr( "TrackType", type);

		if(type!=GetType())
			return false;

		SetNumKeys( num + numCur);
		for (int i = 0; i < num; i++)
		{
			XmlNodeRef keyNode = xmlNode->getChild(i);
			keyNode->getAttr( "time", m_keys[i+numCur].time );
			m_keys[i+numCur].time += fTimeOffset;

			SerializeKey( m_keys[i+numCur],keyNode,bLoading );
			if (bCopySelected)
			{
				m_keys[i+numCur].flags |= AKEY_SELECTED;
			}
		}	
		SortKeys();
	}
	else
	{
		int num = GetNumKeys();
		xmlNode->setAttr( "TrackType",GetType() );

		//CheckValid();

		for (int i = 0; i < num; i++)
		{
			if(!bCopySelected || GetKeyFlags(i) &	AKEY_SELECTED)
			{
				XmlNodeRef keyNode = xmlNode->newChild( "Key" );
				keyNode->setAttr( "time",m_keys[i].time );

				SerializeKey( m_keys[i],keyNode,bLoading );
			}
		}
	}
	return true;
}



//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline int TAnimTrack<KeyType>::CreateKey( float time )
{
	KeyType key,akey;
	int nkey = GetNumKeys();
	SetNumKeys( nkey+1 );	
	key.time = time;
	SetKey( nkey,&key );

	return nkey;
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline int TAnimTrack<KeyType>::CloneKey( int fromKey )
{
	KeyType key;
	GetKey( fromKey,&key );
	int nkey = GetNumKeys();
	SetNumKeys( nkey+1 );
	SetKey( nkey,&key );
	return nkey;
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline int TAnimTrack<KeyType>::CopyKey( IAnimTrack *pFromTrack, int nFromKey )
{
	KeyType key;
	pFromTrack->GetKey( nFromKey,&key );
	int nkey = GetNumKeys();
	SetNumKeys( nkey+1 );
	SetKey( nkey,&key );
	return nkey;
}

//////////////////////////////////////////////////////////////////////////
template <class KeyType>
inline int TAnimTrack<KeyType>::GetActiveKey( float time,KeyType *key )
{
	CheckValid();

	int nkeys = m_keys.size();
	if (nkeys == 0)
	{
		m_lastTime = time;
		m_currKey = -1;
		return m_currKey;
	}

	bool bTimeWrap = false;

	if ((m_flags&ATRACK_CYCLE) || (m_flags&ATRACK_LOOP))
	{
		// Warp time.
		const char *desc = 0;
		float duration = 0;
		GetKeyInfo( nkeys-1,desc,duration );
		float endtime = GetKeyTime(nkeys-1) + duration;
		time = cry_fmod( time,endtime );
		if (time < m_lastTime)
		{
			// Time is wrapped.
			bTimeWrap = true;
		}
	}
	m_lastTime = time;

	// Time is before first key.
	if (m_keys[0].time > time)
	{
		if (bTimeWrap)
		{
			// If time wrapped, active key is last key.
			m_currKey = nkeys-1;
			*key = m_keys[m_currKey];
		}
		else
      m_currKey = -1;
		return m_currKey;
	}

	if (m_currKey < 0)
		m_currKey = 0;

	// Start from current key.
	int i;
	for (i = m_currKey; i < nkeys; i++)
	{
		if (time >= m_keys[i].time)
		{
			if ((i >= nkeys-1) || (time < m_keys[i+1].time))
			{
				m_currKey = i;
				*key = m_keys[m_currKey];
				return m_currKey;
			}
		}
		else
			break;
	}

	// Start from begining.
	for (i = 0; i < nkeys; i++)
	{
		if (time >= m_keys[i].time)
		{
			if ((i >= nkeys-1) || (time < m_keys[i+1].time))
			{
				m_currKey = i;
				*key = m_keys[m_currKey];
				return m_currKey;
			}
		}
		else
			break;
	}
	m_currKey = -1;
	return m_currKey;
}

#endif // __animtrack_h__
